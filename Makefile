
CC     = gcc
CFLAGS = -Wall -g -std=gnu99

SRCS = main.c list.c hash.c bitmap.c debug.c hex_dump.c
OBJS = $(SRCS:.c=.o)

TARGET = testlib

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o:    main.c    list.h hash.h bitmap.h
list.o:    list.c    list.h
hash.o:    hash.c    hash.h list.h
bitmap.o:  bitmap.c  bitmap.h limits.h round.h hex_dump.h
debug.o:   debug.c   debug.h
hex_dump.o: hex_dump.c hex_dump.h

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
