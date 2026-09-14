#include <immintrin.h>
#include <stdint.h>
#include <chrono>
#include <iostream>

__attribute__((target("avx512f,avx512dq,avx512vl")))
void batch_div_avx(uint64_t xp, const uint32_t* m, int64_t* out, size_t count) {
    size_t i = 0;
    __m512d v_xp = _mm512_set1_pd((double)xp);
    for (; i + 15 < count; i += 16) {
        double m0[8], m1[8];
        for (int k = 0; k < 8; k++) m0[k] = (double)m[i + k];
        for (int k = 0; k < 8; k++) m1[k] = (double)m[i + 8 + k];
        __m512d v_m0 = _mm512_loadu_pd(m0);
        __m512d v_m1 = _mm512_loadu_pd(m1);
        __m512d v_q0 = _mm512_div_pd(v_xp, v_m0);
        __m512d v_q1 = _mm512_div_pd(v_xp, v_m1);
        __m512i q0 = _mm512_cvttpd_epu64(v_q0);
        __m512i q1 = _mm512_cvttpd_epu64(v_q1);
        _mm512_storeu_si512((void*)&out[i], q0);
        _mm512_storeu_si512((void*)&out[i + 8], q1);
    }
    for (; i + 7 < count; i += 8) {
        double m0[8];
        for (int k = 0; k < 8; k++) m0[k] = (double)m[i + k];
        __m512d v_m0 = _mm512_loadu_pd(m0);
        __m512d v_q0 = _mm512_div_pd(v_xp, v_m0);
        __m512i q0 = _mm512_cvttpd_epu64(v_q0);
        _mm512_storeu_si512((void*)&out[i], q0);
    }
    for (; i < count; i++) {
        out[i] = xp / m[i];
    }
}

void batch_div_scalar(uint64_t xp, const uint32_t* m, int64_t* out, size_t count) {
    for (size_t i = 0; i < count; i++) {
        out[i] = xp / m[i];
    }
}

int main() {
    const size_t N = 112;
    uint32_t m[N];
    int64_t out_s[N], out_v[N];
    for (size_t i = 0; i < N; i++) m[i] = 1000 + i * 7 + 1;
    uint64_t xp = 1234567890123ULL;

    const int ITERS = 100000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int it = 0; it < ITERS; it++) {
        batch_div_scalar(xp, m, out_s, N);
        asm volatile("" : : "r"(out_s) : "memory");
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int it = 0; it < ITERS; it++) {
        batch_div_avx(xp, m, out_v, N);
        asm volatile("" : : "r"(out_v) : "memory");
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double s_time = std::chrono::duration<double>(t1 - t0).count();
    double v_time = std::chrono::duration<double>(t2 - t1).count();
    std::cout << "Scalar: " << s_time << " s\n";
    std::cout << "AVX-512: " << v_time << " s\n";
    std::cout << "Speedup: " << s_time / v_time << "x\n";
    return 0;
}
