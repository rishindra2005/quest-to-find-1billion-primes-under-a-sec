#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <immintrin.h>

#define N 10000000

static uint64_t primes[N];
static uint64_t bits[N / 128 + 1000];
static uint64_t pi_table[N / 128 + 1000];
static uint64_t unset_larger[128];

void init_tables() {
    for (int i = 0; i < N; i++) primes[i] = 10000 + i * 2 + 1;
    for (int i = 0; i < N / 128 + 1000; i++) {
        bits[i] = 0x5555555555555555ULL ^ i;
        pi_table[i] = i * 32;
    }
    for (int i = 0; i < 128; i++) {
        unset_larger[i] = (i == 0) ? 0 : ((1ULL << (i / 2)) - 1);
    }
}

// Kim Walisch's scalar loop (4x unrolled)
uint64_t scalar_loop(uint64_t xp, uint64_t low, uint64_t count) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i + 3 < count; i += 4) {
        uint64_t xpq0 = xp / primes[i];
        uint64_t xpq1 = xp / primes[i+1];
        uint64_t xpq2 = xp / primes[i+2];
        uint64_t xpq3 = xp / primes[i+3];

        uint64_t x0 = xpq0 - low;
        uint64_t x1 = xpq1 - low;
        uint64_t x2 = xpq2 - low;
        uint64_t x3 = xpq3 - low;

        sum += (pi_table[x0 / 128] + __builtin_popcountll(bits[x0 / 128] & unset_larger[x0 % 128])) * 2;
        sum += (pi_table[x1 / 128] + __builtin_popcountll(bits[x1 / 128] & unset_larger[x1 % 128])) * 2;
        sum += (pi_table[x2 / 128] + __builtin_popcountll(bits[x2 / 128] & unset_larger[x2 % 128])) * 2;
        sum += (pi_table[x3 / 128] + __builtin_popcountll(bits[x3 / 128] & unset_larger[x3 % 128])) * 2;
    }
    return sum;
}

// AVX-512 Vectorized Loop (8x unrolled with vdivpd and vpopcntq)
__attribute__((target("avx512f,avx512vpopcntdq")))
uint64_t avx512_loop(uint64_t xp, uint64_t low, uint64_t count) {
    uint64_t sum = 0;
    __m512d v_xp = _mm512_set1_pd((double)xp);
    __m512i v_low = _mm512_set1_epi64(low);
    __m512i v_127 = _mm512_set1_epi64(127);
    __m512i v_sum = _mm512_setzero_si512();

    for (uint64_t i = 0; i + 7 < count; i += 8) {
        // Load 8 primes as 64-bit ints
        __m512i v_p_int = _mm512_loadu_si512((const void*)&primes[i]);
        // Convert to double
        __m512d v_p_dbl = _mm512_cvtepi64_pd(v_p_int);
        // Vector division: 8 divisions at once!
        __m512d v_div = _mm512_div_pd(v_xp, v_p_dbl);
        // Truncate to 64-bit int
        __m512i v_xpq = _mm512_cvttpd_epu64(v_div);

        // x = xpq - low
        __m512i v_x = _mm512_sub_epi64(v_xpq, v_low);
        __m512i v_idx = _mm512_srli_epi64(v_x, 7);
        __m512i v_rem = _mm512_and_epi64(v_x, v_127);

        // Extract and compute (or gather)
        uint64_t idx[8], rem[8];
        _mm512_storeu_si512((void*)idx, v_idx);
        _mm512_storeu_si512((void*)rem, v_rem);

        uint64_t b0 = bits[idx[0]] & unset_larger[rem[0]];
        uint64_t b1 = bits[idx[1]] & unset_larger[rem[1]];
        uint64_t b2 = bits[idx[2]] & unset_larger[rem[2]];
        uint64_t b3 = bits[idx[3]] & unset_larger[rem[3]];
        uint64_t b4 = bits[idx[4]] & unset_larger[rem[4]];
        uint64_t b5 = bits[idx[5]] & unset_larger[rem[5]];
        uint64_t b6 = bits[idx[6]] & unset_larger[rem[6]];
        uint64_t b7 = bits[idx[7]] & unset_larger[rem[7]];

        __m512i v_b = _mm512_set_epi64(b7, b6, b5, b4, b3, b2, b1, b0);
        __m512i v_cnt = _mm512_popcnt_epi64(v_b);

        __m512i v_pi = _mm512_set_epi64(
            pi_table[idx[7]], pi_table[idx[6]], pi_table[idx[5]], pi_table[idx[4]],
            pi_table[idx[3]], pi_table[idx[2]], pi_table[idx[1]], pi_table[idx[0]]
        );

        __m512i v_res = _mm512_add_epi64(v_pi, v_cnt);
        v_sum = _mm512_add_epi64(v_sum, _mm512_slli_epi64(v_res, 1));
    }
    sum = _mm512_reduce_add_epi64(v_sum);
    return sum;
}

int main() {
    init_tables();
    uint64_t xp = 1000000000000ULL; // 1e12
    uint64_t count = 2000000;       // 2 million primes

    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    uint64_t s1 = scalar_loop(xp, 0, count);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double scalar_ms = ((t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)*1e-9) * 1000.0;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    uint64_t s2 = avx512_loop(xp, 0, count);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double avx_ms = ((t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)*1e-9) * 1000.0;

    printf("Scalar loop (Kim Walisch): %.2f ms (sum = %lu)\n", scalar_ms, s1);
    printf("AVX-512 loop (Our Vector):  %.2f ms (sum = %lu)\n", avx_ms, s2);
    printf("Speedup: %.2fx faster!\n", scalar_ms / avx_ms);
    return 0;
}
