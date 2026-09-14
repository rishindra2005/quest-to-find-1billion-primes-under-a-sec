#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

extern void init_compressed_tables(uint64_t max_val);
extern uint64_t find_nth_prime_ultra(uint64_t target_n);

static const struct {
    uint64_t n;
    uint64_t expected_p;
} EXPECTED[] = {
    { 10ULL,         29ULL },
    { 100ULL,        541ULL },
    { 1000ULL,       7919ULL },
    { 10000ULL,      104729ULL },
    { 100000ULL,     1299709ULL },
    { 1000000ULL,    15485863ULL },
    { 10000000ULL,   179424673ULL },
    { 100000000ULL,  2038074743ULL },
    { 1000000000ULL, 22801763489ULL }
};

int main() {
    init_compressed_tables(8200000ULL);
    int all_ok = 1;
    printf("------------------------------------------------------------------------\n");
    printf(" %-12s | %-15s | %-15s | %-10s | %s\n", "Rank (N)", "Computed Prime", "Expected Prime", "Time (ms)", "Status");
    printf("------------------------------------------------------------------------\n");

    for (int i = 0; i < 9; i++) {
        uint64_t n = EXPECTED[i].n;
        uint64_t exp = EXPECTED[i].expected_p;

        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        uint64_t ans = find_nth_prime_ultra(n);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double ms = ((t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)*1e-9) * 1000.0;

        int ok = (ans == exp);
        if (!ok) all_ok = 0;
        printf(" 10^%-10d | %-15lu | %-15lu | %9.3f ms | %s\n",
               i + 1, ans, exp, ms, ok ? "PASS" : "FAIL");
    }
    printf("------------------------------------------------------------------------\n");
    printf("OVERALL VERIFICATION: %s\n", all_ok ? "ALL 9 TESTS PASSED EXACTLY" : "SOME FAILED");
    return all_ok ? 0 : 1;
}
