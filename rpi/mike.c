/* mike - rpi side: first words to the esp32 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORT "/dev/ttyUSB0"

int main(void)
{
  int fd = open(PORT, O_RDWR | O_NOCTTY);
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
  tio.c_cc[VMIN]  = 0;              /* read returns whatever is there... */
  tio.c_cc[VTIME] = 10;             /* ...or gives up after 1.0s */
  tcsetattr(fd, TCSANOW, &tio);

  sleep(2);                         /* esp may auto-reset when port opens */
  tcflush(fd, TCIFLUSH);            /* bin its boot chatter */

  const char *msg = "xyzzy\n";
  write(fd, msg, strlen(msg));

  char buf[256];
  int n = read(fd, buf, sizeof(buf) - 1);

  if (n > 0) {
    buf[n] = '\0';
    printf("mike says: %s", buf);
  } else {
    printf("Nothing happens.\n");   /* for once, the failure case */
  }

  close(fd);
  return 0;
}
