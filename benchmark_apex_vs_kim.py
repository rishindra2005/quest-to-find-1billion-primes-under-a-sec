#!/usr/bin/env python3
"""
Comprehensive Head-to-Head Benchmark:
Our Apex Prime Engine (prime_apex) vs Kim Walisch primecount (primecount_kim)
Testing both Single-Core (1 Thread) and Multi-Core (24 Threads) on AMD Zen 5.
"""

import subprocess
import time
import re
import sys

def run_cmd(cmd):
    t0 = time.perf_counter()
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    t1 = time.perf_counter()
    lines = res.stdout.strip().splitlines()
    ans = lines[0].strip() if lines else ""
    # Try to extract exact internal timer if present
    m = re.search(r"Seconds:\s+([\d\.]+)", res.stdout)
    internal_sec = float(m.group(1)) if m else (t1 - t0)
    return ans, internal_sec, (t1 - t0)

def main():
    kim_bin = "./primecount_kim/build/primecount"
    apex_bin = "./prime_apex/build/prime_apex"

    print("=" * 80)
    print("  APEX PRIME ENGINE vs KIM WALISCH PRIMECOUNT (PRISTINE UPSTREAM)")
    print("  Processor: AMD Ryzen AI 9 HX 370 (Zen 5 Architecture, 24 Threads)")
    print("=" * 80)

    # 1. N-th Prime Multi-Core
    print("\n[PART 1: N-th Prime Multi-Core (24 Threads)]")
    print(f"{'n':<17} | {'p_n (Verified Prime)':<20} | {'Kim Walisch':<12} | {'Prime Apex':<12} | {'Speedup':<8}")
    print("-" * 80)
    for n in [10**9, 10**10, 10**11, 10**12, 10**13, 10**14, 10**15, 10**16]:
        ans_k, sec_k, _ = run_cmd([kim_bin, "--nth-prime", str(n), "--time"])
        ans_a, sec_a, _ = run_cmd([apex_bin, "--nth-prime", str(n), "--time"])
        assert ans_k == ans_a, f"Mismatch at {n}: {ans_k} vs {ans_a}"
        speedup = sec_k / max(sec_a, 1e-6)
        print(f"{n:<17d} | {ans_a:<20s} | {sec_k*1000:9.2f} ms | {sec_a*1000:9.2f} ms | {speedup:6.1f}x")

    # 2. N-th Prime Single-Core
    print("\n[PART 2: N-th Prime Single-Core (1 Thread)]")
    print(f"{'n':<17} | {'p_n (Verified Prime)':<20} | {'Kim (1T)':<12} | {'Apex (1T)':<12} | {'Speedup':<8}")
    print("-" * 80)
    for n in [10**10, 10**11, 10**12, 10**13, 10**14]:
        ans_k, sec_k, _ = run_cmd([kim_bin, "--nth-prime", str(n), "--threads=1", "--time"])
        ans_a, sec_a, _ = run_cmd([apex_bin, "--nth-prime", str(n), "--threads=1", "--time"])
        assert ans_k == ans_a, f"Mismatch at {n}: {ans_k} vs {ans_a}"
        speedup = sec_k / max(sec_a, 1e-6)
        print(f"{n:<17d} | {ans_a:<20s} | {sec_k*1000:9.2f} ms | {sec_a*1000:9.2f} ms | {speedup:6.1f}x")

    # 3. N-th Prime Near Checkpoints with Arbitrary Offsets
    print("\n[PART 3: N-th Prime Arbitrary Offsets (Delta Sieve)]")
    print(f"{'n (Offset)':<17} | {'p_n (Verified Prime)':<20} | {'Kim Walisch':<12} | {'Prime Apex':<12} | {'Speedup':<8}")
    print("-" * 80)
    for n in [10**12 + 50000, 10**13 - 25000, 10**14 + 100000]:
        ans_k, sec_k, _ = run_cmd([kim_bin, "--nth-prime", str(n), "--time"])
        ans_a, sec_a, _ = run_cmd([apex_bin, "--nth-prime", str(n), "--time"])
        assert ans_k == ans_a, f"Mismatch at {n}: {ans_k} vs {ans_a}"
        speedup = sec_k / max(sec_a, 1e-6)
        print(f"{n:<17d} | {ans_a:<20s} | {sec_k*1000:9.2f} ms | {sec_a*1000:9.2f} ms | {speedup:6.1f}x")

    # 4. Prime Counting pi(x) Multi-Core
    print("\n[PART 4: Prime Counting pi(x) Multi-Core (24 Threads)]")
    print(f"{'x':<10} | {'pi(x)':<18} | {'Kim Walisch':<12} | {'Prime Apex':<12} | {'Speedup':<8}")
    print("-" * 72)
    for x in ["1e11", "1e12", "1e13", "1e14", "1e15", "1e16"]:
        ans_k, sec_k, _ = run_cmd([kim_bin, x, "--time"])
        ans_a, sec_a, _ = run_cmd([apex_bin, x, "--time"])
        assert ans_k == ans_a, f"Mismatch at {x}: {ans_k} vs {ans_a}"
        speedup = sec_k / max(sec_a, 1e-6)
        print(f"{x:<10s} | {ans_a:<18s} | {sec_k*1000:9.2f} ms | {sec_a*1000:9.2f} ms | {speedup:6.2f}x")

    print("\n" + "=" * 80)
    print("  BENCHMARK COMPLETE: All mathematical results 100% verified.")
    print("=" * 80)

if __name__ == "__main__":
    main()
