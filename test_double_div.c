#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
#include <math.h>

int main() {
    uint64_t xp = 12345678901234ULL; // ~1.2e13
    uint64_t failures = 0;
    for (uint64_t p = 3; p < 2000000; p += 2) {
        uint64_t exact = xp / p;
        double d_res = (double)xp / (double)p;
        uint64_t f_res = (uint64_t)d_res;
        if (exact != f_res) {
            // Check if off by 1 due to floating point precision
            failures++;
            if (failures <= 5) {
                printf("Mismatch for p=%lu: exact=%lu, float=%lu\n", p, exact, f_res);
            }
        }
    }
    printf("Total failures for xp=%lu up to p=2M: %lu\n", xp, failures);
    return 0;
}
