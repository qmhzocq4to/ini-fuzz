CC ?= clang
CFLAGS ?= -O1 -g -fno-omit-frame-pointer -fsanitize=fuzzer,address -Wall -Wextra
SRC = src/ini.c src/ini_fuzz.c

all: ini-fuzz

ini-fuzz: $(SRC) src/ini.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: ini-fuzz
	./ini-fuzz -max_total_time=60 corpus

corpus:
	mkdir -p corpus

clean:
	rm -f ini-fuzz

.PHONY: all run clean
