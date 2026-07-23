/* mike - rpi side: line client and telemetry poller (protocol.md).
   Forwards stdin lines to the body and prints the replies; between
   commands it polls each sensor at its own rate. `-l` or the stdin
   command `/log` echoes poll replies to stdout, one line each,
   prefixed with the poll command. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <termios.h>

#define PORT "/dev/ttyUSB0"
#define REPLY_TIMEOUT_DS 10       /* 10 x 0.1s read slices = 1.0s */
#define PROTO_VER 2

static int fd;
static int log_on;
static int faults;

/* each sensor is polled on its own clock; add a row when one lands */
static struct {
  const char *cmd;
  int         period_ms;
  int64_t     due;
  int         down;               /* no-reply state, for edge-only noise */
} sensors[] = {
  { "tel", 1000, 0, 0 },          /* body state: armed, outputs, wdtrips */
  { "imu",  200, 0, 0 },          /* tilt and motion */
};

#define NSENSORS (sizeof sensors / sizeof sensors[0])

static int64_t ms_now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* accumulate one reply line; -1 on ~1s of silence */
static int read_reply(char *buf, size_t cap)
{
  size_t len = 0;
  int idle = 0;

  while (len < cap - 1) {
    char c;
    int n = read(fd, &c, 1);
    if (n == 1) {
      idle = 0;
      if (c == '\n') {
        buf[len] = '\0';
        return 0;
      }
      if (c != '\r')
        buf[len++] = c;
    } else if (++idle >= REPLY_TIMEOUT_DS) {
      break;
    }
  }
  buf[len] = '\0';
  return -1;
}

/* refuse to talk to a body we don't understand (protocol.md) */
static int check_ver(void)
{
  char rep[256], want[16];

  snprintf(want, sizeof want, "mike %d", PROTO_VER);
  for (int try = 0; try < 3; try++) {
    write(fd, "ver\n", 4);
    while (read_reply(rep, sizeof rep) == 0) {  /* skim boot chatter */
      if (!strcmp(rep, want))
        return 0;
      if (!strncmp(rep, "mike ", 5)) {
        fprintf(stderr, "mike: body speaks \"%s\", want \"%s\"\n", rep, want);
        return -1;
      }
    }
  }
  fprintf(stderr, "mike: no version reply from body\n");
  return -1;
}

static void poll_sensor(size_t i)
{
  char rep[256];

  write(fd, sensors[i].cmd, strlen(sensors[i].cmd));
  write(fd, "\n", 1);
  if (read_reply(rep, sizeof rep) == 0) {
    if (sensors[i].down) {
      fprintf(stderr, "mike: %s: recovered\n", sensors[i].cmd);
      sensors[i].down = 0;
    }
    if (log_on) {
      printf("%s %s\n", sensors[i].cmd, rep);
      fflush(stdout);
    }
  } else if (!sensors[i].down) {
    fprintf(stderr, "mike: %s: no reply\n", sensors[i].cmd);
    sensors[i].down = 1;
  }
}

static void handle_line(char *cmd)
{
  char rep[256];

  if (!*cmd)
    return;
  if (!strcmp(cmd, "/log")) {
    log_on = !log_on;
    fprintf(stderr, "mike: log %s\n", log_on ? "on" : "off");
    return;
  }
  write(fd, cmd, strlen(cmd));
  write(fd, "\n", 1);
  if (read_reply(rep, sizeof rep) == 0) {
    printf("%s\n", rep);
    fflush(stdout);
  } else {
    fprintf(stderr, "mike: no reply\n");
    faults = 1;
  }
}

int main(int argc, char **argv)
{
  if (argc == 2 && !strcmp(argv[1], "-l"))
    log_on = 1;
  else if (argc != 1) {
    fprintf(stderr, "usage: mike [-l]\n");
    return 2;
  }

  fd = open(PORT, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(PORT);
    return 1;
  }

  struct termios tio;
  tcgetattr(fd, &tio);
  cfmakeraw(&tio);                  /* no echo, no line editing, no CR/LF games */
  cfsetispeed(&tio, B115200);
  cfsetospeed(&tio, B115200);
  tio.c_cflag |= CLOCAL | CREAD;    /* ignore modem lines, enable receiver */
  tio.c_cflag &= ~HUPCL;            /* keep DTR up on exit: no reset surprise */
  tio.c_cc[VMIN]  = 0;              /* read returns whatever is there... */
  tio.c_cc[VTIME] = 1;              /* ...in 0.1s slices */
  tcsetattr(fd, TCSANOW, &tio);

  sleep(2);                         /* esp may auto-reset when port opens */
  tcflush(fd, TCIFLUSH);            /* bin its boot chatter */

  if (check_ver()) {
    close(fd);
    return 1;
  }

  /* raw stdin line assembly: stdio buffering and poll() don't mix */
  static char line[4096];
  size_t len = 0;
  int eof = 0;

  while (!eof) {
    int64_t now = ms_now();
    int64_t next = now + 1000;

    for (size_t i = 0; i < NSENSORS; i++) {
      if (sensors[i].due <= now) {
        poll_sensor(i);
        sensors[i].due = ms_now() + sensors[i].period_ms;
      }
      if (sensors[i].due < next)
        next = sensors[i].due;
    }

    struct pollfd pin = { .fd = 0, .events = POLLIN };
    now = ms_now();
    if (poll(&pin, 1, next > now ? (int)(next - now) : 0) <= 0)
      continue;

    char buf[256];
    ssize_t n = read(0, buf, sizeof buf);
    if (n <= 0) {
      eof = 1;
    } else {
      for (ssize_t i = 0; i < n; i++) {
        char c = buf[i];
        if (c == '\n') {
          line[len] = '\0';
          len = 0;
          handle_line(line);
        } else if (c != '\r' && len < sizeof line - 1) {
          line[len++] = c;
        }
      }
    }
  }

  close(fd);
  return faults;
}
