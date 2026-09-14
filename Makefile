CC ?= gcc
AS ?= as
CFLAGS ?= -O3 -march=native -pthread -Wall -Wextra
LDFLAGS ?= -lm -pthread

TARGETS = prime_ultra prime_ultra_omp prime_fast prime_fast_omp prime_engine prime_standalone test_suite test_ultra_suite

all: $(TARGETS)

fast_prime_asm.o: fast_prime_asm.s
	$(AS) fast_prime_asm.s -o fast_prime_asm.o

sieve_kernel.o: sieve_kernel.s
	$(AS) sieve_kernel.s -o sieve_kernel.o

prime_ultra: prime_ultra.c fast_prime_asm.o sieve_kernel.o
	$(CC) $(CFLAGS) prime_ultra.c fast_prime_asm.o sieve_kernel.o -o prime_ultra $(LDFLAGS)

prime_ultra_omp: prime_ultra_omp.c fast_prime_asm.o sieve_kernel.o
	$(CC) $(CFLAGS) -fopenmp prime_ultra_omp.c fast_prime_asm.o sieve_kernel.o -o prime_ultra_omp $(LDFLAGS)

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

test_ultra_suite: test_ultra_suite.c prime_ultra.c fast_prime_asm.o sieve_kernel.o
	$(CC) $(CFLAGS) -DNO_MAIN test_ultra_suite.c prime_ultra.c fast_prime_asm.o sieve_kernel.o -o test_ultra_suite $(LDFLAGS)

run-ultra: prime_ultra
	./prime_ultra 1000000000

run-ultra-omp: prime_ultra_omp
	./prime_ultra_omp 1000000000

run-fast: prime_fast
	./prime_fast 1000000000

run-fast-omp: prime_fast_omp
	./prime_fast_omp 1000000000

run: prime_engine
	./prime_engine 1000000000

run-standalone: prime_standalone
	./prime_standalone

test: test_suite test_ultra_suite
	./test_ultra_suite

apex:
	cmake -B prime_apex/build -S prime_apex -DCMAKE_BUILD_TYPE=Release
	cmake --build prime_apex/build -j

run-apex: apex
	./prime_apex/build/prime_apex 100000000000000 --time

benchmark-apex: apex
	python3 benchmark_apex_vs_kim.py

clean:
	rm -f $(TARGETS) *.o
	rm -rf prime_apex/build

.PHONY: all run run-ultra run-ultra-omp run-fast run-fast-omp run-standalone test clean apex run-apex benchmark-apex
