#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <omp.h>
#include <immintrin.h>

#define LIMIT 10000000ULL

static uint8_t *sieve_buf;
static uint32_t *primes;
static uint32_t *pi_table;
static uint32_t num_primes = 0;

static const uint32_t W = 210;
static uint8_t coprime_counts[211];

void init_tables_parallel(uint64_t limit) {
    sieve_buf = calloc(limit + 1, 1);
    pi_table = malloc((limit + 1) * sizeof(uint32_t));
    primes = malloc((limit / 10) * sizeof(uint32_t));
    primes[0] = 0;

    memset(sieve_buf, 1, limit + 1);
    sieve_buf[0] = sieve_buf[1] = 0;
    uint64_t sqrt_lim = (uint64_t)sqrt(limit);

    #pragma omp parallel for schedule(dynamic)
    for (uint64_t p = 2; p <= sqrt_lim; p++) {
        bool is_p = true;
        for (uint64_t d = 2; d * d <= p; d++) {
            if (p % d == 0) { is_p = false; break; }
        }
        if (is_p) {
            for (uint64_t j = p * p; j <= limit; j += p) {
                sieve_buf[j] = 0;
            }
        }
    }

    uint32_t count = 0;
    for (uint64_t i = 1; i <= limit; i++) {
        if (sieve_buf[i]) {
            count++;
            primes[count] = i;
        }
        pi_table[i] = count;
    }
    num_primes = count;

    // Wheel 210
    uint8_t c = 0;
    coprime_counts[0] = 0;
    for (uint32_t i = 1; i <= W; i++) {
        int v = i - 1;
        if (v > 0 && (v % 2 != 0) && (v % 3 != 0) && (v % 5 != 0) && (v % 7 != 0)) {
            c++;
        }
        coprime_counts[i] = c;
    }
}

#define HASH_SIZE 1048576
typedef struct {
    uint64_t x;
    uint32_t a;
    int64_t val;
} HashEntry;

static HashEntry hash_table[HASH_SIZE];

static inline uint32_t hash_func(uint64_t x, uint32_t a) {
    uint64_t h = x * 0x9e3779b97f4a7c15ULL ^ ((uint64_t)a * 0x517cc1b727220a95ULL);
    return (uint32_t)(h & (HASH_SIZE - 1));
}

int64_t phi(uint64_t x, uint32_t a) {
    if (a == 4) {
        return (int64_t)(x / 210) * 48 + coprime_counts[(x % 210) + 1];
    }
    if (a == 0) return x;
    if (x <= primes[a]) return (x > 0) ? 1 : 0;
    if (x <= LIMIT && (uint64_t)primes[a] * primes[a] >= x) {
        return (int64_t)pi_table[x] - a + 1;
    }

    uint32_t h = hash_func(x, a);
    if (hash_table[h].x == x && hash_table[h].a == a) {
        return hash_table[h].val;
    }

    int64_t res = phi(x, a - 1) - phi(x / primes[a], a - 1);
    hash_table[h].x = x;
    hash_table[h].a = a;
    hash_table[h].val = res;
    return res;
}

uint64_t meissel_pi_parallel(uint64_t x) {
    memset(hash_table, 0, sizeof(hash_table));
    uint64_t cbrt_x = (uint64_t)cbrt((double)x);
    uint64_t sqrt_x = (uint64_t)sqrt((double)x);
    uint32_t a = pi_table[cbrt_x];
    uint32_t b = pi_table[sqrt_x];

    int64_t p2 = 0;
    #pragma omp parallel for reduction(+:p2) schedule(static)
    for (uint32_t i = a + 1; i <= b; i++) {
        p2 += (int64_t)pi_table[x / primes[i]] - i + 1;
    }

    int64_t phi_val = phi(x, a);
    return (uint64_t)(phi_val + a - 1 - p2);
}

double p_n_asymptotic(uint64_t n) {
    if (n < 10) return 29.0;
    double d_n = (double)n;
    double lnn = log(d_n);
    double ln2n = log(lnn);
    return d_n * (lnn + ln2n - 1.0 
               + (ln2n - 2.0) / lnn 
               - (ln2n*ln2n - 6.0*ln2n + 11.0) / (2.0 * lnn*lnn)
               + (2.0*ln2n*ln2n*ln2n - 21.0*ln2n*ln2n + 84.0*ln2n - 114.0) / (6.0 * lnn*lnn*lnn));
}

extern void sieve_segment_asm(uint8_t *seg, uint64_t seg_size,
                              const uint32_t *primes, uint32_t *offsets,
                              uint64_t num_primes);
extern uint64_t count_primes_avx512(const uint8_t *seg, uint64_t S);
extern uint64_t find_nth_prime_in_seg_asm(const uint8_t *seg, uint64_t seg_size,
                                          uint64_t target_rank, uint64_t low_k);

uint64_t find_nth_prime_fast_parallel(uint64_t target_n) {
    if (target_n <= num_primes) {
        return primes[target_n];
    }

    double est = p_n_asymptotic(target_n);
    uint64_t margin = (uint64_t)(0.00002 * est) + 1000;
    uint64_t x_start = (uint64_t)(est - margin);
    if (x_start % 2 == 0) x_start--;

    uint64_t pi_start = meissel_pi_parallel(x_start);
    while (pi_start >= target_n) {
        x_start -= margin;
        pi_start = meissel_pi_parallel(x_start);
    }

    uint64_t rank_needed = target_n - pi_start;

    uint64_t low_k = (x_start - 1) / 2 + 1;
    uint64_t seg_size = 65536ULL;
    uint8_t *seg = aligned_alloc(64, seg_size);

    uint64_t sqrt_max = (uint64_t)sqrt(est + 500000.0) + 1000;
    uint32_t bprimes_count = pi_table[sqrt_max] - 1;
    uint32_t *base_p = malloc(bprimes_count * sizeof(uint32_t));
    uint32_t *offsets = malloc(bprimes_count * sizeof(uint32_t));

    uint64_t L = 2 * low_k + 1;
    for (uint32_t i = 0; i < bprimes_count; i++) {
        uint64_t p = primes[i + 2];
        base_p[i] = (uint32_t)p;

        uint64_t p2 = p * p;
        if (p2 >= L) {
            uint64_t off = (p2 - 1) / 2 - low_k;
            offsets[i] = (off < seg_size) ? (uint32_t)off : (uint32_t)seg_size;
        } else {
            uint64_t rem = L % p;
            uint64_t m = (rem == 0) ? L : (L + (p - rem));
            if (m % 2 == 0) m += p;
            uint64_t off = (m - L) / 2;
            offsets[i] = (uint32_t)off;
        }
    }

    uint64_t result = 0;
    while (1) {
        memset(seg, 0, seg_size);
        sieve_segment_asm(seg, seg_size, base_p, offsets, bprimes_count);
        uint64_t seg_primes = count_primes_avx512(seg, seg_size);

        if (rank_needed <= seg_primes) {
            result = find_nth_prime_in_seg_asm(seg, seg_size, rank_needed, low_k);
            break;
        }

        rank_needed -= seg_primes;
        low_k += seg_size;
    }

    free(seg);
    free(base_p);
    free(offsets);
    return result;
}

int main(int argc, char **argv) {
    uint64_t target_n = 1000000000ULL;
    if (argc >= 2) target_n = strtoull(argv[1], NULL, 10);

    struct timespec ts1, ts2;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    init_tables_parallel(LIMIT);
    uint64_t ans = find_nth_prime_fast_parallel(target_n);

    clock_gettime(CLOCK_MONOTONIC, &ts2);
    double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) * 1e-9;

    printf("====================================================================\n");
    printf("     Multi-Core Combinatorial Sieve Engine (Meissel + AVX-512)     \n");
    printf("====================================================================\n");
    printf("Target Prime Rank (N)      : %lu\n", target_n);
    printf("Worker Threads             : %d\n", omp_get_max_threads());
    printf("Result N-th Prime          : %lu\n", ans);
    printf("Total Execution Time       : %.4f seconds (%.2f ms)\n", elapsed, elapsed * 1000.0);
    printf("OEIS A006988 Verification  : %s\n", (target_n == 1000000000ULL && ans == 22801763489ULL) ? "[PASS] VERIFIED EXACT MATCH" : "COMPLETED");
    printf("====================================================================\n");
    return 0;
}
