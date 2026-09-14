#include <immintrin.h>
#include <stdint.h>
#include <iostream>
#include <chrono>
#include <vector>

constexpr uint64_t bitmask_fn(uint64_t n) {
    return ((n + 1) / 2 == 64) ? 0xffffffffffffffffULL : (1ULL << ((n + 1) / 2)) - 1;
}

struct DummySegmentedPi {
    uint64_t low_;
    uint64_t high_;
    std::vector<uint64_t> bits_;
    std::vector<uint64_t> pi_;
    uint64_t unset_larger_[128];

    DummySegmentedPi(size_t size) {
        low_ = 1000000;
        high_ = low_ + size * 128;
        bits_.resize(size, 0xAAAAAAAAAAAAAAAAULL);
        pi_.resize(size, 12345);
        for (int i = 0; i < 128; i++) unset_larger_[i] = bitmask_fn(i);
    }

    __attribute__((always_inline)) inline uint64_t query_scalar(uint64_t x) const {
        if (x < 2) return 0;
        x -= low_;
        uint64_t w = x / 128;
        uint64_t rem = x % 128;
        uint64_t b = bits_[w];
        uint64_t m = unset_larger_[rem];
        return pi_[w] + __builtin_popcountll(b & m);
    }

    __attribute__((target("avx512f,avx512vpopcntdq,avx512vl,avx512dq")))
    inline __m512i query_avx512(__m512i v_q) const {
        __m512i v_low = _mm512_set1_epi64(low_);
        __m512i v_x = _mm512_sub_epi64(v_q, v_low);
        __m512i v_w = _mm512_srli_epi64(v_x, 7);
        __m512i v_rem = _mm512_and_epi64(v_x, _mm512_set1_epi64(127));

        __m512i v_bits = _mm512_i64gather_epi64(v_w, bits_.data(), 8);
        __m512i v_pi = _mm512_i64gather_epi64(v_w, pi_.data(), 8);
        __m512i v_mask = _mm512_i64gather_epi64(v_rem, unset_larger_, 8);

        __m512i v_and = _mm512_and_si512(v_bits, v_mask);
        __m512i v_pop = _mm512_popcnt_epi64(v_and);
        return _mm512_add_epi64(v_pi, v_pop);
    }
};

int main() {
    size_t size = 1024; // 128 KB table (fits in L2)
    DummySegmentedPi table(size);

    const size_t N = 1024;
    uint64_t q[N];
    for (size_t i = 0; i < N; i++) {
        q[i] = table.low_ + (i * 97) % (size * 128 - 200) + 10;
    }

    uint64_t sum_s = 0;
    for (size_t i = 0; i < N; i++) sum_s += table.query_scalar(q[i]);

    uint64_t sum_v = 0;
    for (size_t i = 0; i < N; i += 8) {
        __m512i v_q = _mm512_loadu_si512((const void*)&q[i]);
        __m512i res = table.query_avx512(v_q);
        sum_v += _mm512_reduce_add_epi64(res);
    }

    if (sum_s != sum_v) {
        std::cerr << "Mismatch! " << sum_s << " vs " << sum_v << "\n";
        return 1;
    }

    const int ITERS = 100000;
    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t sink1 = 0;
    for (int it = 0; it < ITERS; it++) {
        for (size_t i = 0; i < N; i++) sink1 += table.query_scalar(q[i]);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    uint64_t sink2 = 0;
    for (int it = 0; it < ITERS; it++) {
        for (size_t i = 0; i < N; i += 8) {
            __m512i v_q = _mm512_loadu_si512((const void*)&q[i]);
            __m512i res = table.query_avx512(v_q);
            sink2 += _mm512_reduce_add_epi64(res);
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double s_time = std::chrono::duration<double>(t1 - t0).count();
    double v_time = std::chrono::duration<double>(t2 - t1).count();
    std::cout << "Scalar time:  " << s_time << " s (sink=" << sink1 << ")\n";
    std::cout << "AVX-512 time: " << v_time << " s (sink=" << sink2 << ")\n";
    std::cout << "Speedup: " << s_time / v_time << "x\n";
    return 0;
}
