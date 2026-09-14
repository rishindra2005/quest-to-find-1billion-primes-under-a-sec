#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>

static uint16_t coprime_16[480];
static uint64_t coprime_64[480];

void init_coprime() {
    int wp[5] = {2, 3, 5, 7, 11};
    int idx = 0;
    for (int v = 1; v <= 2310; v++) {
        int ok = 1;
        for (int k = 0; k < 5; k++) {
            if (v % wp[k] == 0) { ok = 0; break; }
        }
        if (ok) {
            coprime_16[idx] = v;
            coprime_64[idx] = v;
            idx++;
        }
    }
}

uint64_t to_number_scalar(uint64_t index) {
    uint64_t q = index / 480;
    uint64_t r = index % 480;
    return 2310 * q + coprime_16[r];
}

__attribute__((target("avx512f,avx512dq"), noinline))
void to_number_avx512(const uint64_t* in_indices, uint64_t* out_numbers) {
    __m512i idx = _mm512_loadu_si512((const void*)in_indices);
    __m512d v_d = _mm512_cvtepi64_pd(idx);
    __m512d v_q = _mm512_mul_pd(v_d, _mm512_set1_pd(1.0 / 480.0));
    __m512i q = _mm512_cvttpd_epu64(v_q);

    __m512i q_480 = _mm512_mullo_epi64(q, _mm512_set1_epi64(480));
    __m512i r = _mm512_sub_epi64(idx, q_480);

    __m512i coprime = _mm512_i64gather_epi64(r, (const long long*)coprime_64, 8);
    __m512i q_2310 = _mm512_mullo_epi64(q, _mm512_set1_epi64(2310));
    __m512i res = _mm512_add_epi64(coprime, q_2310);

    _mm512_storeu_si512((void*)out_numbers, res);
}

int main() {
    init_coprime();
    uint64_t count = 20000000;
    uint64_t* test_in = malloc(count * sizeof(uint64_t));
    uint64_t* test_out = malloc(count * sizeof(uint64_t));
    for (uint64_t i = 0; i < count; i++) test_in[i] = i;

    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    uint64_t sum1 = 0;
    for (uint64_t i = 0; i < count; i++) {
        sum1 += to_number_scalar(test_in[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double scalar_ms = ((t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)*1e-9) * 1000.0;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    uint64_t sum2 = 0;
    for (uint64_t i = 0; i < count; i += 8) {
        to_number_avx512(&test_in[i], &test_out[i]);
    }
    for (uint64_t i = 0; i < count; i++) sum2 += test_out[i];
    clock_gettime(CLOCK_MONOTONIC, &t2);
    double avx_ms = ((t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)*1e-9) * 1000.0;

    printf("Scalar to_number: %.2f ms (sum = %lu)\n", scalar_ms, sum1);
    printf("AVX-512 to_number: %.2f ms (sum = %lu)\n", avx_ms, sum2);
    printf("Speedup: %.2fx faster!\n", scalar_ms / avx_ms);

    free(test_in);
    free(test_out);
    return 0;
}
