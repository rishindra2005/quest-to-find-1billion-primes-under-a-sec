#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <immintrin.h>

#define MAX_VAL 8200000ULL
#define NUM_WORDS (MAX_VAL / 64 + 1)

static uint64_t prime_bits[NUM_WORDS];
static uint32_t block_pi[NUM_WORDS];
static uint32_t *primes_list;
static uint32_t total_primes = 0;

static const uint32_t W = 210;
static uint8_t coprime_counts[211];

// External assembly routines
extern uint32_t fast_pi_lookup_asm(uint64_t x, const uint64_t *prime_bits, 
                                    const uint32_t *block_pi);

extern int64_t compute_p2_asm(uint64_t x, uint32_t a, uint32_t b,
                              const uint32_t *primes,
                              const uint64_t *prime_bits,
                              const uint32_t *block_pi);

extern void sieve_segment_asm(uint8_t *seg, uint64_t seg_size,
                              const uint32_t *primes, uint32_t *offsets,
                              uint64_t num_primes);

extern uint64_t count_primes_avx512(const uint8_t *seg, uint64_t S);

extern uint64_t find_nth_prime_in_seg_asm(const uint8_t *seg, uint64_t seg_size,
                                          uint64_t target_rank, uint64_t low_k);

static inline uint32_t fast_pi_inline(uint64_t x) {
    uint64_t w = x >> 6;
    uint32_t base = block_pi[w];
    uint64_t word = prime_bits[w];
    uint64_t mask = _bzhi_u64(~0ULL, (x & 63) + 1);
    return base + (uint32_t)__builtin_popcountll(word & mask);
}

void init_compressed_tables(uint64_t max_val) {
    uint32_t words = max_val / 64 + 1;
    memset(prime_bits, 0xFF, words * sizeof(uint64_t));
    prime_bits[0] &= ~((1ULL << 0) | (1ULL << 1)); // 0 and 1 not prime
    for (uint32_t w = 0; w < words; w++) prime_bits[w] &= 0xAAAAAAAAAAAAAAAAULL;
    prime_bits[0] |= (1ULL << 2); // 2 is prime

    // Sieve odd primes
    for (uint64_t p = 3; p * p <= max_val; p += 2) {
        if (prime_bits[p / 64] & (1ULL << (p % 64))) {
            uint64_t step = 2 * p;
            for (uint64_t j = p * p; j <= max_val; j += step) {
                prime_bits[j / 64] &= ~(1ULL << (j % 64));
            }
        }
    }

    primes_list = malloc((max_val / 10) * sizeof(uint32_t));
    primes_list[0] = 0;
    uint32_t running_cnt = 0;

    for (uint32_t w = 0; w < words; w++) {
        block_pi[w] = running_cnt;
        uint64_t word = prime_bits[w];
        running_cnt += _mm_popcnt_u64(word);

        uint64_t base = (uint64_t)w * 64;
        while (word) {
            int bit = __builtin_ctzll(word);
            uint32_t p = base + bit;
            if (p <= max_val) {
                total_primes++;
                primes_list[total_primes] = p;
            }
            word &= word - 1;
        }
    }

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

int64_t phi_ultra(uint64_t x, uint32_t a) {
    if (a == 4) {
        return (int64_t)(x / 210) * 48 + coprime_counts[(x % 210) + 1];
    }
    if (a == 0) return x;
    if (x <= primes_list[a]) return (x > 0) ? 1 : 0;
    if (x < MAX_VAL && (uint64_t)primes_list[a] * primes_list[a] >= x) {
        return (int64_t)fast_pi_inline(x) - a + 1;
    }

    uint32_t h = hash_func(x, a);
    if (hash_table[h].x == x && hash_table[h].a == a) {
        return hash_table[h].val;
    }

    int64_t res = phi_ultra(x, a - 1) - phi_ultra(x / primes_list[a], a - 1);
    hash_table[h].x = x;
    hash_table[h].a = a;
    hash_table[h].val = res;
    return res;
}

uint64_t meissel_pi_ultra(uint64_t x) {
    memset(hash_table, 0, sizeof(hash_table));
    uint64_t cbrt_x = (uint64_t)cbrt((double)x);
    uint64_t sqrt_x = (uint64_t)sqrt((double)x);
    uint32_t a = fast_pi_inline(cbrt_x);
    uint32_t b = fast_pi_inline(sqrt_x);

    // Call raw assembly kernel for P2
    int64_t p2 = compute_p2_asm(x, a, b, primes_list, prime_bits, block_pi);

    int64_t phi_val = phi_ultra(x, a);
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

uint64_t find_nth_prime_ultra(uint64_t target_n) {
    if (target_n <= total_primes) {
        return primes_list[target_n];
    }

    double est = p_n_asymptotic(target_n);
    uint64_t margin = (uint64_t)(0.00002 * est) + 1000;
    uint64_t x_start = (uint64_t)(est - margin);
    if (x_start % 2 == 0) x_start--;

    uint64_t pi_start = meissel_pi_ultra(x_start);
    while (pi_start >= target_n) {
        x_start -= margin;
        pi_start = meissel_pi_ultra(x_start);
    }

    uint64_t rank_needed = target_n - pi_start;

    // Sieve only the final cache window
    uint64_t low_k = (x_start - 1) / 2 + 1;
    uint64_t seg_size = 65536ULL;
    uint8_t *seg = aligned_alloc(64, seg_size);

    uint64_t sqrt_max = (uint64_t)sqrt(est + 500000.0) + 1000;
    uint32_t bprimes_count = fast_pi_inline(sqrt_max) - 1; // skip 2
    uint32_t *base_p = malloc(bprimes_count * sizeof(uint32_t));
    uint32_t *offsets = malloc(bprimes_count * sizeof(uint32_t));

    uint64_t L = 2 * low_k + 1;
    for (uint32_t i = 0; i < bprimes_count; i++) {
        uint64_t p = primes_list[i + 2]; // skip 2
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

#ifndef NO_MAIN
int main(int argc, char **argv) {
    uint64_t target_n = 1000000000ULL;
    if (argc >= 2) target_n = strtoull(argv[1], NULL, 10);

    struct timespec ts1, ts2;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    init_compressed_tables(MAX_VAL);
    uint64_t ans = find_nth_prime_ultra(target_n);

    clock_gettime(CLOCK_MONOTONIC, &ts2);
    double elapsed = (ts2.tv_sec - ts1.tv_sec) + (ts2.tv_nsec - ts1.tv_nsec) * 1e-9;

    printf("====================================================================\n");
    printf("     RAW ASSEMBLY ULTRA-FAST ENGINE (BMI2 + AVX-512 + Zen 5)       \n");
    printf("====================================================================\n");
    printf("Target Prime Rank (N)      : %lu\n", target_n);
    printf("Result N-th Prime          : %lu\n", ans);
    printf("Total Execution Time       : %.4f seconds (%.2f ms)\n", elapsed, elapsed * 1000.0);
    printf("OEIS A006988 Verification  : %s\n", (target_n == 1000000000ULL && ans == 22801763489ULL) ? "[PASS] VERIFIED EXACT MATCH" : "COMPLETED");
    printf("====================================================================\n");
    return 0;
}
#endif
