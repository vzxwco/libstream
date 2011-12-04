CC = gcc
CFLAGS = -W -Wall

SRCS = $(shell ls *.c)
HDRS = $(SRCS:.c=.h)
OBJS = $(SRCS:.c=.o)

.PHONY: clean love

all: $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o *~ core a.out

love:
	@echo not war.

