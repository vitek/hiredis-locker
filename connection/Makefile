CC = gcc
CFLAGS = -g3 -W -Wall -O0
LDLIBS = -lhiredis

all: example

example: redis-connection.o example.o


clean:
	rm -f example *.o
