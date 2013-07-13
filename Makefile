CC = gcc
CFLAGS = -g3 -W -Wall -O2
LDLIBS = -lhiredis

all: bench_locker bench_native bench_incr example

bench_locker: redis-locker.o bench_locker.o
example: redis-locker.o example.o

redis-locker.o: lua_scripts.h redis-locker.h
lua_scripts.h: scripts/lock.lua scripts/unlock.lua mkscripts.py
	python mkscripts.py > $@

bench_native bench_incr: bench_common.h
bench_locker.o: bench_common.h

clean:
	rm -f bench_native bench_locker bench_incr example *.o
