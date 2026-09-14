#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

extern uint64_t find_nth_prime(uint64_t n, int num_threads, uint64_t seg_size);

int main() {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    printf("================================================================================\n");
    printf("     Automated Verification Suite: OEIS A006988 (10^1 to 10^9 Primes)           \n");
    printf("================================================================================\n");
    printf("CPU Hardware Threads Detected: %d\n\n", num_cores);

    uint64_t test_ns[] = {
        10ULL,
        100ULL,
        1000ULL,
        10000ULL,
        100000ULL,
        1000000ULL,
        10000000ULL,
        100000000ULL,
        1000000000ULL
    };

    uint64_t expected[] = {
        29ULL,
        541ULL,
        7919ULL,
        104729ULL,
        1299709ULL,
        15485863ULL,
        179424673ULL,
        2038074743ULL,
        22801763489ULL
    };

    int num_tests = sizeof(test_ns) / sizeof(test_ns[0]);
    int passed = 0;

    for (int i = 0; i < num_tests; i++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        uint64_t result = find_nth_prime(test_ns[i], num_cores, 65536);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed_ms = ((t1.tv_sec - t0.tv_sec) * 1000.0) + ((t1.tv_nsec - t0.tv_nsec) / 1e6);

        int ok = (result == expected[i]);
        if (ok) passed++;

        printf("Test %d/9: N = %10lu | Found: %12lu | Expected: %12lu | %8.2f ms | [%s]\n",
               i + 1, test_ns[i], result, expected[i], elapsed_ms,
               ok ? "\033[1;32mPASS\033[0m" : "\033[1;31mFAIL\033[0m");
    }

    printf("================================================================================\n");
    printf("Verification Summary: %d / %d Tests Passed.\n", passed, num_tests);
    if (passed == num_tests) {
        printf("Status: \033[1;32mALL TESTS VERIFIED SUCCESSFULLY AGAINST OEIS A006988\033[0m\n");
    } else {
        printf("Status: \033[1;31mSOME TESTS FAILED\033[0m\n");
    }
    printf("================================================================================\n");

    return (passed == num_tests) ? 0 : 1;
}
