CC ?= gcc
AS ?= as
CFLAGS ?= -O3 -march=native -pthread -Wall -Wextra
LDFLAGS ?= -lm -pthread

TARGETS = prime_fast prime_fast_omp prime_engine prime_standalone test_suite

all: $(TARGETS)

sieve_kernel.o: sieve_kernel.s
	$(AS) sieve_kernel.s -o sieve_kernel.o

prime_fast: prime_fast.c sieve_kernel.o
	$(CC) $(CFLAGS) prime_fast.c sieve_kernel.o -o prime_fast $(LDFLAGS)

prime_fast_omp: prime_fast_omp.c sieve_kernel.o
	$(CC) $(CFLAGS) -fopenmp prime_fast_omp.c sieve_kernel.o -o prime_fast_omp $(LDFLAGS)

prime_engine: prime_engine.c sieve_kernel.o
	$(CC) $(CFLAGS) prime_engine.c sieve_kernel.o -o prime_engine $(LDFLAGS)

prime_standalone: prime_standalone.s sieve_kernel.o
	$(CC) -no-pie prime_standalone.s sieve_kernel.o -o prime_standalone $(LDFLAGS)

test_suite: test_suite.c prime_engine.c sieve_kernel.o
	$(CC) $(CFLAGS) -DNO_MAIN test_suite.c prime_engine.c sieve_kernel.o -o test_suite $(LDFLAGS)

run-fast: prime_fast
	./prime_fast 1000000000

run-fast-omp: prime_fast_omp
	./prime_fast_omp 1000000000

run: prime_engine
	./prime_engine 1000000000

run-standalone: prime_standalone
	./prime_standalone

test: test_suite
	./test_suite

clean:
	rm -f $(TARGETS) test_meissel *.o

.PHONY: all run run-fast run-fast-omp run-standalone test clean
