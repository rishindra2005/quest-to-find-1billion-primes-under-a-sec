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

    // Interleaved 4-way unrolled query to maximize ILP in CPU out-of-order execution
    __attribute__((always_inline)) inline uint64_t query4(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3) const {
        uint64_t d0 = x0 - low_, d1 = x1 - low_, d2 = x2 - low_, d3 = x3 - low_;
        uint64_t w0 = d0 >> 7, w1 = d1 >> 7, w2 = d2 >> 7, w3 = d3 >> 7;
        uint64_t r0 = d0 & 127, r1 = d1 & 127, r2 = d2 & 127, r3 = d3 & 127;
        
        uint64_t b0 = bits_[w0], b1 = bits_[w1], b2 = bits_[w2], b3 = bits_[w3];
        uint64_t m0 = unset_larger_[r0], m1 = unset_larger_[r1], m2 = unset_larger_[r2], m3 = unset_larger_[r3];

        uint64_t p0 = pi_[w0] + __builtin_popcountll(b0 & m0);
        uint64_t p1 = pi_[w1] + __builtin_popcountll(b1 & m1);
        uint64_t p2 = pi_[w2] + __builtin_popcountll(b2 & m2);
        uint64_t p3 = pi_[w3] + __builtin_popcountll(b3 & m3);

        return p0 + p1 + p2 + p3;
    }
};

int main() {
    size_t size = 1024;
    DummySegmentedPi table(size);

    const size_t N = 1024;
    uint64_t q[N];
    for (size_t i = 0; i < N; i++) {
        q[i] = table.low_ + (i * 97) % (size * 128 - 200) + 10;
    }

    const int ITERS = 100000;
    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t sink1 = 0;
    for (int it = 0; it < ITERS; it++) {
        for (size_t i = 0; i < N; i++) sink1 += table.query_scalar(q[i]); asm volatile("" : : "r"(sink1) : "memory");
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    uint64_t sink2 = 0;
    for (int it = 0; it < ITERS; it++) {
        for (size_t i = 0; i < N; i += 4) {
            sink2 += table.query4(q[i], q[i+1], q[i+2], q[i+3]); asm volatile("" : : "r"(sink2) : "memory");
        }
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double s_time = std::chrono::duration<double>(t1 - t0).count();
    double i_time = std::chrono::duration<double>(t2 - t1).count();
    std::cout << "Sequential scalar: " << s_time << " s\n";
    std::cout << "Interleaved 4-way: " << i_time << " s\n";
    std::cout << "Speedup: " << s_time / i_time << "x\n";
    return 0;
}
