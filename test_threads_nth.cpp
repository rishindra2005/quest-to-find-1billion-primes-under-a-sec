#include <iostream>
#include <chrono>
#include <primecount.hpp>

int main() {
    for (int t : {1, 2, 4, 8, 12, 16, 24}) {
        primecount::set_num_threads(t);
        auto t0 = std::chrono::high_resolution_clock::now();
        int64_t ans = primecount::nth_prime(1000000000ULL);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "Threads " << t << ": " << ms << " ms (ans = " << ans << ")" << std::endl;
    }
    return 0;
}
