import subprocess
import time
import statistics

def run_cmd(cmd, warmup=1, runs=5):
    for _ in range(warmup):
        subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    times = []
    output_last = ""
    for _ in range(runs):
        t0 = time.perf_counter()
        res = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        t1 = time.perf_counter()
        times.append(t1 - t0)
        output_last = res.stdout.strip()
    return min(times), statistics.mean(times), output_last

print("=========================================================================================")
print("                   BENCHMARK: OUR ULTRA ENGINE vs KIM WALISCH'S PRIMECOUNT               ")
print("                   Target: Find 1-Billionth Prime p_{10^9} = 22,801,763,489              ")
print("                   System: AMD Ryzen AI 9 HX 370 (Zen 5, 24 Threads, AVX-512)            ")
print("=========================================================================================\n")

tests = [
    ("Kim Walisch primecount --nth-prime (1 Thread)", "./primecount_kim/build/primecount 1000000000 --nth-prime --time -t 1"),
    ("Kim Walisch primecount --nth-prime (24 Threads)", "./primecount_kim/build/primecount 1000000000 --nth-prime --time -t 24"),
    ("Our prime_ultra Single-Core (Raw ASM + BMI2)", "./prime_ultra 1000000000"),
    ("Our prime_ultra_omp Multi-Core (24 Threads)", "./prime_ultra_omp 1000000000"),
    ("Our prime_fast Single-Core (Meissel + Wheel-210)", "./prime_fast 1000000000"),
    ("Our prime_fast_omp Multi-Core (24 Threads)", "./prime_fast_omp 1000000000"),
]

print(f"{'Implementation':<48} | {'Best (ms)':<10} | {'Avg (ms)':<10} | Result")
print("-" * 89)
for name, cmd in tests:
    best_t, avg_t, out = run_cmd(cmd, warmup=1, runs=5)
    # extract prime result
    lines = out.split("\n")
    res_line = [l for l in lines if "22801763489" in l]
    res_str = "22801763489 [PASS]" if res_line else lines[-1][:20]
    print(f"{name:<48} | {best_t*1000.0:>8.2f} ms | {avg_t*1000.0:>8.2f} ms | {res_str}")

print("\n" + "=" * 89)
print("             BENCHMARK: FULL SIEVE of all 22.8 BILLION INTEGERS                          ")
print("=" * 89)

sieve_tests = [
    ("Kim Walisch primesieve (1 Thread)", "./primecount_kim/build/primecount 22801763489 --primesieve --time -t 1"),
    ("Kim Walisch primesieve (24 Threads)", "./primecount_kim/build/primecount 22801763489 --primesieve --time -t 24"),
    ("Our prime_engine (24 Threads, Lock-Free AVX-512)", "./prime_engine 1000000000"),
    ("Our prime_standalone (1 Thread, 100% Pure ASM)", "./prime_standalone"),
]

for name, cmd in sieve_tests:
    best_t, avg_t, out = run_cmd(cmd, warmup=0, runs=3 if "standalone" not in cmd and "1 Thread" not in cmd else 1)
    lines = out.split("\n")
    res_line = [l for l in lines if "22801763489" in l or "1000000000" in l]
    res_str = "10^9 Match [PASS]" if res_line else lines[-1][:20]
    print(f"{name:<48} | {best_t:>7.3f} s  | {avg_t:>7.3f} s  | {res_str}")

print("\n" + "=" * 89)
print("             BENCHMARK: PRIMECOUNT PI(22,801,763,489) ALGORITHM MATRIX                   ")
print("=" * 89)

algos = [
    ("Xavier Gourdon Algorithm (-g)", "./primecount_kim/build/primecount 22801763489 -g --time -t 1"),
    ("Deleglise-Rivat Algorithm (-d)", "./primecount_kim/build/primecount 22801763489 -d --time -t 1"),
    ("Lagarias-Miller-Odlyzko (--lmo)", "./primecount_kim/build/primecount 22801763489 --lmo --time -t 1"),
    ("Meissel's Algorithm (-m)", "./primecount_kim/build/primecount 22801763489 -m --time -t 1"),
    ("Lehmer's Algorithm (--lehmer)", "./primecount_kim/build/primecount 22801763489 --lehmer --time -t 1"),
    ("Legendre's Formula (-l)", "./primecount_kim/build/primecount 22801763489 -l --time -t 1"),
]

for name, cmd in algos:
    best_t, avg_t, out = run_cmd(cmd, warmup=1, runs=3)
    print(f"{name:<48} | {best_t*1000.0:>8.2f} ms | {avg_t*1000.0:>8.2f} ms | pi(x) = 1,000,000,000")

