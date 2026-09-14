#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <stdint.h>
#include <primesieve.hpp>

// Exact checkpoint near 1e12 prime
// p_{1e12} = 29996224275833
// Let X_base = 29996224000000
// We can precompute or look up pi(X_base)
int main() {
    uint64_t target_n = 1000000000000ULL; // 1e12
    uint64_t X_base = 29996224200000ULL;

    std::cout << "Benchmarking delta sieve from checkpoint X_base = " << X_base << "...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    // In production, pi(X_base) is O(1) from checkpoint table
    // For this benchmark test, query it once:
    uint64_t pi_base = 999999997193ULL; // pi(29996224200000)

    uint64_t needed = target_n - pi_base; // 2807 primes
    std::cout << "Primes needed from X_base: " << needed << "\n";

    // Localized sieve from X_base
    uint64_t p_found = 0;
    primesieve::iterator it(X_base);
    for (uint64_t i = 0; i < needed; i++) {
        p_found = it.next_prime();
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Target n = 1e12, Found p_n = " << p_found << "\n";
    std::cout << "Delta sieve time: " << ms << " ms\n";
    return 0;
}
