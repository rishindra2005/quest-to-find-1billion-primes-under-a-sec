#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main() {
    uint64_t failures = 0;
    uint64_t tests = 0;
    for (uint64_t xp = 1000000000ULL; xp <= 1000000000000ULL; xp *= 10) {
        for (uint64_t p = 1001; p < 100000; p += 10) {
            uint64_t exact = xp / p;
            double d = (double)xp / (double)p;
            uint64_t f = (uint64_t)d;
            if (exact != f) failures++;
            tests++;
        }
    }
    printf("Tested %lu divisions, failures = %lu\n", tests, failures);
    return 0;
}
