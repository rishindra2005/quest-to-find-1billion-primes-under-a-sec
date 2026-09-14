#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <x86intrin.h>

#define DEFAULT_SEG_SIZE (65536) // 64 KiB = 65,536 odd numbers = 131,072 integer span

// Pure x86_64 Assembly Functions (defined in sieve_kernel.s)
extern void sieve_segment_asm(uint8_t *seg, uint64_t seg_size,
                              const uint32_t *primes, uint32_t *offsets,
                              uint64_t num_primes);

extern uint64_t count_primes_avx512(const uint8_t *seg, uint64_t seg_size);

extern uint64_t find_nth_prime_in_seg_asm(const uint8_t *seg, uint64_t seg_size,
                                          uint64_t target_rank, uint64_t low_k);

// Global Base Primes Table (up to sqrt(upper_bound))
static uint32_t *base_primes = NULL;
static int num_base_primes = 0;

// Dynamic Task Queue for Parallel Sieving
typedef struct {
    uint64_t low_k;
    uint64_t high_k;
    uint64_t count;
} ChunkTask;

static ChunkTask *tasks = NULL;
static int total_chunks = 0;
static int next_chunk_idx = 0;
static uint64_t segment_size_cfg = DEFAULT_SEG_SIZE;

// Generate base primes up to max_val using standard sieve
int init_base_primes(uint32_t max_val) {
    uint8_t *is_prime = malloc(max_val + 1);
    if (!is_prime) return 0;
    memset(is_prime, 1, max_val + 1);
    is_prime[0] = is_prime[1] = 0;

    for (uint32_t p = 2; (uint64_t)p * p <= max_val; p++) {
        if (is_prime[p]) {
            for (uint32_t i = p * p; i <= max_val; i += p) {
                is_prime[i] = 0;
            }
        }
    }

    int count = 0;
    for (uint32_t p = 3; p <= max_val; p += 2) {
        if (is_prime[p]) count++;
    }

    base_primes = malloc(count * sizeof(uint32_t));
    int idx = 0;
    for (uint32_t p = 3; p <= max_val; p += 2) {
        if (is_prime[p]) base_primes[idx++] = p;
    }

    free(is_prime);
    num_base_primes = count;
    return count;
}

// Sieve an arbitrary odd-number range [low_k, high_k] and count primes
static uint64_t sieve_range_count(uint64_t low_k, uint64_t high_k, uint64_t seg_size) {
    if (low_k > high_k) return 0;

    uint8_t *seg = aligned_alloc(64, seg_size);
    uint32_t *offsets = malloc(num_base_primes * sizeof(uint32_t));

    uint64_t L_odd = 2 * low_k + 1;
    uint64_t high_odd = 2 * high_k + 1;

    // Determine active base primes and their first relative offsets in this chunk
    int active_primes = 0;
    for (int i = 0; i < num_base_primes; i++) {
        uint64_t p = base_primes[i];
        if (p * p > high_odd) break;
        active_primes++;

        uint64_t start_val = (p * p > L_odd) ? (p * p) : L_odd;
        uint64_t off;
        if (start_val == p * p) {
            off = (p * p - 1) / 2 - low_k;
        } else {
            uint64_t rem = L_odd % p;
            uint64_t m = (rem == 0) ? L_odd : (L_odd + p - rem);
            if ((m & 1) == 0) m += p;
            off = (m - L_odd) / 2;
        }
        offsets[i] = (uint32_t)off;
    }

    uint64_t current_k = low_k;
    uint64_t count = 0;

    while (current_k <= high_k) {
        uint64_t cur_seg = seg_size;
        if (current_k + cur_seg > high_k + 1) {
            cur_seg = (high_k + 1) - current_k;
        }

        memset(seg, 0, cur_seg);
        if (current_k == 0) {
            seg[0] = 1; // 1 is not prime
        }

        sieve_segment_asm(seg, cur_seg, base_primes, offsets, active_primes);
        count += count_primes_avx512(seg, cur_seg);

        current_k += cur_seg;
    }

    free(seg);
    free(offsets);
    return count;
}

// Locate the target_rank-th prime in [low_k, high_k]
static uint64_t sieve_range_find_nth(uint64_t low_k, uint64_t high_k, uint64_t seg_size, uint64_t target_rank) {
    uint8_t *seg = aligned_alloc(64, seg_size);
    uint32_t *offsets = malloc(num_base_primes * sizeof(uint32_t));

    uint64_t L_odd = 2 * low_k + 1;
    uint64_t high_odd = 2 * high_k + 1;

    int active_primes = 0;
    for (int i = 0; i < num_base_primes; i++) {
        uint64_t p = base_primes[i];
        if (p * p > high_odd) break;
        active_primes++;

        uint64_t start_val = (p * p > L_odd) ? (p * p) : L_odd;
        uint64_t off;
        if (start_val == p * p) {
            off = (p * p - 1) / 2 - low_k;
        } else {
            uint64_t rem = L_odd % p;
            uint64_t m = (rem == 0) ? L_odd : (L_odd + p - rem);
            if ((m & 1) == 0) m += p;
            off = (m - L_odd) / 2;
        }
        offsets[i] = (uint32_t)off;
    }

    uint64_t current_k = low_k;
    uint64_t accumulated = 0;
    uint64_t found_prime = 0;

    while (current_k <= high_k) {
        uint64_t cur_seg = seg_size;
        if (current_k + cur_seg > high_k + 1) {
            cur_seg = (high_k + 1) - current_k;
        }

        memset(seg, 0, cur_seg);
        if (current_k == 0) {
            seg[0] = 1;
        }

        sieve_segment_asm(seg, cur_seg, base_primes, offsets, active_primes);
        uint64_t seg_cnt = count_primes_avx512(seg, cur_seg);

        if (accumulated + seg_cnt >= target_rank) {
            uint64_t rank_in_seg = target_rank - accumulated;
            found_prime = find_nth_prime_in_seg_asm(seg, cur_seg, rank_in_seg, current_k);
            break;
        }
        accumulated += seg_cnt;
        current_k += cur_seg;
    }

    free(seg);
    free(offsets);
    return found_prime;
}

// Worker thread: fetch-and-add task distribution
static void *worker_thread(void *arg) {
    (void)arg;
    while (1) {
        int idx = __atomic_fetch_add(&next_chunk_idx, 1, __ATOMIC_RELAXED);
        if (idx >= total_chunks) break;
        tasks[idx].count = sieve_range_count(tasks[idx].low_k, tasks[idx].high_k, segment_size_cfg);
    }
    return NULL;
}

// Find the N-th prime number
uint64_t find_nth_prime(uint64_t n, int num_threads, uint64_t seg_size) {
    if (n == 1) return 2;
    if (n == 2) return 3;
    if (n == 3) return 5;
    if (n == 4) return 7;
    if (n == 5) return 11;

    segment_size_cfg = seg_size;

    // Tight Dusart (2010) upper bound
    double logn = log((double)n);
    double loglogn = log(logn);
    uint64_t upper_bound;
    if (n < 1000) {
        upper_bound = n * 15;
    } else {
        upper_bound = (uint64_t)(n * (logn + loglogn - 0.93)) + 1000;
    }

    // Precompute base primes up to 200,000 (covers N up to 40 billion)
    if (!base_primes) {
        init_base_primes(200000);
    }

    uint64_t max_k = (upper_bound - 1) / 2;

    // Chunk size tuned for ~480-960 tasks across cores (smooth work distribution)
    uint64_t chunk_size = 524288ULL * 32; // ~33.5M integers per chunk
    total_chunks = (max_k + chunk_size) / chunk_size;
    if (total_chunks < num_threads * 4) {
        chunk_size = 524288ULL * 4;
        total_chunks = (max_k + chunk_size) / chunk_size;
    }

    tasks = malloc(total_chunks * sizeof(ChunkTask));
    for (int i = 0; i < total_chunks; i++) {
        tasks[i].low_k = i * chunk_size;
        tasks[i].high_k = (i + 1) * chunk_size - 1;
        if (tasks[i].high_k > max_k) tasks[i].high_k = max_k;
        tasks[i].count = 0;
    }
    next_chunk_idx = 0;

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);

    // Prefix sum to locate the target chunk
    uint64_t total = 1; // 2 is the 1st prime
    int target_chunk = -1;
    uint64_t rank_needed = 0;

    for (int i = 0; i < total_chunks; i++) {
        if (total + tasks[i].count >= n) {
            target_chunk = i;
            rank_needed = n - total;
            break;
        }
        total += tasks[i].count;
    }

    uint64_t ans = 0;
    if (target_chunk != -1) {
        ans = sieve_range_find_nth(tasks[target_chunk].low_k,
                                   tasks[target_chunk].high_k,
                                   seg_size,
                                   rank_needed);
    }

    free(tasks);
    tasks = NULL;
    return ans;
}

#ifndef NO_MAIN
int main(int argc, char **argv) {
    uint64_t target_n = 1000000000ULL; // 1 billionth prime by default
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    uint64_t seg_size = DEFAULT_SEG_SIZE;

    if (argc >= 2) {
        target_n = strtoull(argv[1], NULL, 10);
    }
    if (argc >= 3) {
        num_threads = atoi(argv[2]);
        if (num_threads < 1) num_threads = 1;
    }
    if (argc >= 4) {
        seg_size = strtoull(argv[3], NULL, 10) * 1024; // KB
    }

    printf("================================================================================\n");
    printf("        High-Performance Multi-Core AVX-512 Assembly Prime Engine               \n");
    printf("================================================================================\n");
    printf("Target Prime Rank (N)  : %lu\n", target_n);
    printf("Worker Threads         : %d\n", num_threads);
    printf("L1/L2 Segment Cache    : %lu KiB (%lu odd-numbers / segment)\n", seg_size / 1024, seg_size);
    printf("Sieve Kernel           : Pure Hand-Crafted x86_64 Assembly (.intel_syntax)\n");
    printf("Vector Architecture    : AVX-512 (vpcmpeqb, kmovq, native 512-bit registers)\n");
    printf("Bit Operations         : Hardware POPCNT, BMI1 (tzcnt, blsr)\n");
    printf("--------------------------------------------------------------------------------\n");

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    uint64_t start_cycles = __rdtsc();

    uint64_t prime = find_nth_prime(target_n, num_threads, seg_size);

    uint64_t end_cycles = __rdtsc();
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double total_sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    uint64_t cycles = end_cycles - start_cycles;

    printf("\n");
    printf("================================================================================\n");
    printf("                              RESULT & METRICS                                  \n");
    printf("================================================================================\n");
    printf("The %lu-th Prime Number is : \033[1;32m%lu\033[0m\n", target_n, prime);
    printf("--------------------------------------------------------------------------------\n");
    printf("Elapsed Execution Time     : \033[1;36m%.4f seconds\033[0m (%.2f ms)\n", total_sec, total_sec * 1000.0);
    printf("Total CPU Cycles (RDTSC)   : %lu cycles\n", cycles);
    if (total_sec > 0.0) {
        printf("Sieving Speed              : %.2f Billion numbers / sec\n", (prime / total_sec) / 1e9);
        printf("Prime Discovery Throughput : %.2f Million primes / sec\n", (target_n / total_sec) / 1e6);
    }

    // Expected value verification for 1 billion
    if (target_n == 1000000000ULL) {
        const uint64_t expected_1b = 22801763489ULL;
        printf("OEIS A006988 Verification : %s (Expected: %lu)\n",
               (prime == expected_1b ? "\033[1;32mVERIFIED EXACT MATCH [PASS]\033[0m" : "\033[1;31mMISMATCH [FAIL]\033[0m"),
               expected_1b);
    }
    printf("================================================================================\n");

    if (base_primes) free(base_primes);
    return 0;
}
#endif

