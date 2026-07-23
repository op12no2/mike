CC     = gcc
CFLAGS = -Wall -Wextra -O2

all: mike voice

mike: mike.c
	$(CC) $(CFLAGS) -o mike mike.c

voice: voice.c
	$(CC) $(CFLAGS) -o voice voice.c -lm

clean:
	rm -f mike voice
