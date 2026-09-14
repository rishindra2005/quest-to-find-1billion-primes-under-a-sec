#include <immintrin.h>
#include <stdint.h>
#include <iostream>

int main() {
    uint32_t p32[8] = { 101, 103, 107, 109, 113, 127, 131, 137 };
    __m256i v32 = _mm256_loadu_si256((const __m256i*)p32);
    __m512d d32 = _mm512_cvtepu32_pd(v32);
    __m512d num = _mm512_set1_pd(100000.0);
    __m512d q_d = _mm512_div_pd(num, d32);
    __m512i q_i = _mm512_cvttpd_epu64(q_d);
    
    uint64_t res[8];
    _mm512_storeu_si512((void*)res, q_i);
    for (int i = 0; i < 8; i++) {
        std::cout << "100000 / " << p32[i] << " = " << res[i] << " (exact: " << (100000 / p32[i]) << ")\n";
    }
    return 0;
}
