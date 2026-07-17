/* mike - esp32 side: echo everything received on UART0 */

#include "driver/uart.h"

#define BUF_SIZE 1024

void app_main(void)
{
  uart_config_t cfg = {
    .baud_rate  = 115200,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_0, &cfg);

  uint8_t buf[BUF_SIZE];

  for (;;) {
    int n = uart_read_bytes(UART_NUM_0, buf, BUF_SIZE, 20 / portTICK_PERIOD_MS);
    if (n > 0)
      uart_write_bytes(UART_NUM_0, buf, n);
  }
}
