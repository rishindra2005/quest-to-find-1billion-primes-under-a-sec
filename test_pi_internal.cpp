#include <iostream>
#include <chrono>
#include <primecount.hpp>

int main() {
    primecount::set_num_threads(24);
    primecount::pi(1000000);

    for (int i = 10; i <= 15; i++) {
        uint64_t x = 1;
        for (int k = 0; k < i; k++) x *= 10;

        auto t0 = std::chrono::high_resolution_clock::now();
        int64_t ans = primecount::pi(x);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << "pi(10^" << i << ") = " << ans << " in " << ms << " ms" << std::endl;
    }
    return 0;
}
