/* mike - rpi side: line client for the body protocol (protocol/protocol.md).
   Reads protocol lines on stdin, prints the body's replies on stdout. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyUSB0"
#define REPLY_TIMEOUT_DS 10       /* 10 x 0.1s read slices = 1.0s */

static int fd;

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

int main(void)
{
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

  char *cmd = NULL;
  size_t cap = 0;
  char rep[256];
  int faults = 0;

  while (getline(&cmd, &cap, stdin) > 0) {
    cmd[strcspn(cmd, "\r\n")] = '\0';
    if (!*cmd)
      continue;
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

  free(cmd);
  close(fd);
  return faults;
}
