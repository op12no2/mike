/* mike - esp32 side: the body. Line protocol on UART0, see protocol.md. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "driver/uart.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_log.h"

#define PROTO_VER   1
#define UART        UART_NUM_0
#define BUF_SIZE    1024
#define CMD_MAX    120

#define LED_GPIO    38          /* onboard WS2812; GPIO48 on some revisions */

#define THR_LIMIT   400         /* soft throttle clamp: slow and torquey */
#define WD_LEASE_US 300000      /* a drv value is only good for this long */

/* body state */
static int      armed;
static int      thr, str;       /* applied outputs, -1000..1000 */
static unsigned wdtrips;
static int64_t  lease_end;

/* PWM stubs: real LEDC output arrives with the servo/ESC. 0 = neutral. */
static void out_throttle(int v) { thr = v; }
static void out_steer(int v)    { str = v; }

static void wd_check(void)
{
  if (armed && esp_timer_get_time() > lease_end) {
    out_throttle(0);            /* steering holds position */
    armed = 0;
    wdtrips++;
  }
}

/* onboard WS2812 via RMT: 10MHz ticks, bit = 1.2us high/low split */

static rmt_channel_handle_t led_chan;
static rmt_encoder_handle_t led_enc;

static void led_init(void)
{
  rmt_tx_channel_config_t cc = {
    .gpio_num          = LED_GPIO,
    .clk_src           = RMT_CLK_SRC_DEFAULT,
    .resolution_hz     = 10 * 1000 * 1000,
    .mem_block_symbols = 48,
    .trans_queue_depth = 1,
  };
  rmt_bytes_encoder_config_t ec = {
    .bit0 = { .duration0 = 3, .level0 = 1, .duration1 = 9, .level1 = 0 },
    .bit1 = { .duration0 = 9, .level0 = 1, .duration1 = 3, .level1 = 0 },
    .flags.msb_first = 1,
  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&cc, &led_chan));
  ESP_ERROR_CHECK(rmt_new_bytes_encoder(&ec, &led_enc));
  ESP_ERROR_CHECK(rmt_enable(led_chan));
}

static void led_set(int r, int g, int b)
{
  uint8_t grb[3] = { g, r, b };
  rmt_transmit_config_t tc = { 0 };
  rmt_transmit(led_chan, led_enc, grb, sizeof grb, &tc);
  rmt_tx_wait_all_done(led_chan, 100);
}

/* LSM6DSOX IMU: I2C on GPIO8/9, pitch/roll/moving in tel. See protocol.md. */

#define I2C_SDA     8
#define I2C_SCL     9
#define IMU_ADDR    0x6A        /* SA0 low */
#define IMU_WHOAMI  0x6C
#define MOVE_LSB    300         /* raw gyro LSB, ~5 deg/s at 500 dps FS */

static i2c_master_dev_handle_t imu;
static int imu_ok;              /* probed and configured at boot */

static int imu_rd(uint8_t reg, uint8_t *dst, size_t n)
{
  return i2c_master_transmit_receive(imu, &reg, 1, dst, n, 50) != ESP_OK;
}

static int imu_wr(uint8_t reg, uint8_t val)
{
  uint8_t b[2] = { reg, val };
  return i2c_master_transmit(imu, b, sizeof b, 50) != ESP_OK;
}

static void imu_init(void)
{
  i2c_master_bus_config_t bc = {
    .i2c_port          = -1,
    .sda_io_num        = I2C_SDA,
    .scl_io_num        = I2C_SCL,
    .clk_source        = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,     /* the breakout carries the pull-ups */
  };
  i2c_device_config_t dc = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address  = IMU_ADDR,
    .scl_speed_hz    = 400 * 1000,
  };
  i2c_master_bus_handle_t bus;
  uint8_t id;

  if (i2c_new_master_bus(&bc, &bus) != ESP_OK ||
      i2c_master_bus_add_device(bus, &dc, &imu) != ESP_OK ||
      imu_rd(0x0F, &id, 1) || id != IMU_WHOAMI ||  /* WHO_AM_I */
      imu_wr(0x10, 0x48) ||   /* CTRL1_XL: 104 Hz, +-4 g */
      imu_wr(0x11, 0x44))     /* CTRL2_G:  104 Hz, 500 dps */
    return;
  imu_ok = 1;
}

/* one burst read, gyro then accel; raw counts, orientation fixed at mounting */
static int imu_read(int16_t g[3], int16_t a[3])
{
  uint8_t raw[12];
  if (!imu_ok || imu_rd(0x22, raw, sizeof raw))    /* OUTX_L_G onward */
    return -1;
  for (int i = 0; i < 3; i++) {
    g[i] = (int16_t)(raw[i * 2]     | raw[i * 2 + 1] << 8);
    a[i] = (int16_t)(raw[6 + i * 2] | raw[7 + i * 2] << 8);
  }
  return 0;
}

/* protocol */

static void reply(const char *s)
{
  uart_write_bytes(UART, s, strlen(s));
  uart_write_bytes(UART, "\n", 1);
}

/* strict integer: whole token, in range */
static int parse_int(const char *s, long lo, long hi, long *out)
{
  char *end;
  if (!s || !*s)
    return -1;
  long v = strtol(s, &end, 10);
  if (*end || end == s || v < lo || v > hi)
    return -1;
  *out = v;
  return 0;
}

static void dispatch(char *line)
{
  char out[CMD_MAX];
  char *verb = strtok(line, " ");
  if (!verb)
    return;                     /* blank line: no reply */

  if (!strcmp(verb, "ping")) {
    if (strtok(NULL, " ")) { reply("eh?"); return; }
    reply("pong");
    return;
  }

  if (!strcmp(verb, "ver")) {
    if (strtok(NULL, " ")) { reply("eh?"); return; }
    snprintf(out, sizeof out, "mike %d", PROTO_VER);
    reply(out);
    return;
  }

  if (!strcmp(verb, "arm")) {
    if (strtok(NULL, " ")) { reply("eh?"); return; }
    armed = 1;
    lease_end = esp_timer_get_time() + WD_LEASE_US;
    reply("ok");
    return;
  }

  if (!strcmp(verb, "disarm")) {
    if (strtok(NULL, " ")) { reply("eh?"); return; }
    out_throttle(0);
    armed = 0;
    reply("ok");
    return;
  }

  if (!strcmp(verb, "stop")) {
    if (strtok(NULL, " ")) { reply("eh?"); return; }
    out_throttle(0);
    if (armed)
      lease_end = esp_timer_get_time() + WD_LEASE_US;
    reply("ok");
    return;
  }

  if (!strcmp(verb, "drv")) {
    long t, s;
    if (parse_int(strtok(NULL, " "), -1000, 1000, &t) ||
        parse_int(strtok(NULL, " "), -1000, 1000, &s) ||
        strtok(NULL, " ")) { reply("eh?"); return; }
    if (!armed) { reply("no disarmed"); return; }
    if (t >  THR_LIMIT) t =  THR_LIMIT;
    if (t < -THR_LIMIT) t = -THR_LIMIT;
    out_throttle(t);
    out_steer(s);
    lease_end = esp_timer_get_time() + WD_LEASE_US;
    snprintf(out, sizeof out, "ok %ld %ld", t, s);
    reply(out);
    return;
  }

  if (!strcmp(verb, "led")) {
    long r, g, b;
    if (parse_int(strtok(NULL, " "), 0, 255, &r) ||
        parse_int(strtok(NULL, " "), 0, 255, &g) ||
        parse_int(strtok(NULL, " "), 0, 255, &b) ||
        strtok(NULL, " ")) { reply("eh?"); return; }
    led_set(r, g, b);
    reply("ok");
    return;
  }

  if (!strcmp(verb, "tel")) {
    int16_t g[3], a[3];
    if (strtok(NULL, " ")) { reply("eh?"); return; }
    int n = snprintf(out, sizeof out,
             "ok up_ms=%lld armed=%d wdtrips=%u thr=%d str=%d",
             (long long)(esp_timer_get_time() / 1000),
             armed, wdtrips, thr, str);
    if (!imu_read(g, a)) {
      long pitch = lrintf(atan2f(-a[0],
                     sqrtf((float)a[1] * a[1] + (float)a[2] * a[2]))
                     * (180000.0f / (float)M_PI));
      long roll  = lrintf(atan2f(a[1], a[2]) * (180000.0f / (float)M_PI));
      int moving = abs(g[0]) > MOVE_LSB || abs(g[1]) > MOVE_LSB ||
                   abs(g[2]) > MOVE_LSB;
      snprintf(out + n, sizeof out - n,
               " imu=1 pitch_mdeg=%ld roll_mdeg=%ld moving=%d",
               pitch, roll, moving);
    } else {
      snprintf(out + n, sizeof out - n, " imu=0");
    }
    reply(out);
    return;
  }

  reply("Nothing happens.");
}

void app_main(void)
{
  esp_log_level_set("*", ESP_LOG_NONE);   /* UART0 is a command channel */

  uart_config_t cfg = {
    .baud_rate  = 115200,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  uart_driver_install(UART, BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART, &cfg);

  led_init();
  imu_init();

  static char line[CMD_MAX];
  int len = 0, drop = 0;
  uint8_t buf[64];

  for (;;) {
    int n = uart_read_bytes(UART, buf, sizeof buf, 20 / portTICK_PERIOD_MS);
    for (int i = 0; i < n; i++) {
      char c = buf[i];
      if (c == '\n') {
        if (drop) {
          reply("eh?");
          drop = 0;
        } else {
          line[len] = '\0';
          dispatch(line);
        }
        len = 0;
      } else if (c != '\r') {
        if (len < CMD_MAX - 1)
          line[len++] = c;
        else
          drop = 1;             /* overlong: bin to next newline */
      }
    }
    wd_check();
  }
}
