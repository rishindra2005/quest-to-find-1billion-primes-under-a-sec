import subprocess
import time
import re

def run_primecount(cmd):
    # Run multiple times and take minimum to avoid thermal/frequency governor jitter
    times = []
    output_res = None
    for _ in range(3):
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        # Parse Seconds: X.XXX
        m = re.search(r"Seconds:\s+([\d\.]+)", res.stdout)
        if m:
            times.append(float(m.group(1)))
            output_res = res.stdout.strip().split("\n")[0]
        else:
            # Fallback to wall-clock if no internal timer printed
            pass
    return min(times) if times else 0.0, output_res

print("=" * 70)
print("       HIGH-PERFORMANCE AVX-512 PRIME COMPUTATION BENCHMARK")
print("=" * 70)

scales_pi = [
    ("1e10", "455052511"),
    ("1e11", "4118054813"),
    ("1e12", "37607912018"),
    ("1e13", "346065536839"),
    ("1e14", "3204941750802"),
    ("1e15", "29844570422669"),
    ("1e16", "279238341033925"),
]

print("\n[1] PI(X) BENCHMARKS (Multithreaded: 24 Threads):")
print(f"{'Scale':<8} | {'Expected Pi(x)':<18} | {'Compute Time':<14} | {'Status'}")
print("-" * 55)
for scale, expected in scales_pi:
    t, out = run_primecount(["./primecount_kim/build/primecount", scale, "--time"])
    status = "EXACT MATCH" if expected in out else f"MISMATCH ({out})"
    print(f"{scale:<8} | {expected:<18} | {t*1000:8.2f} ms   | {status}")

scales_nth = [
    ("1e9", "22801763489"),
    ("1e10", "252097800623"),
    ("1e11", "2760727302517"),
    ("1e12", "29996224275833"),
    ("1e13", "323780508946331"),
    ("1e14", "3475385758524527"),
]

print("\n[2] N-TH PRIME BENCHMARKS (Multithreaded: 24 Threads):")
print(f"{'n':<8} | {'Expected p_n':<18} | {'Compute Time':<14} | {'Status'}")
print("-" * 55)
for scale, expected in scales_nth:
    t, out = run_primecount(["./primecount_kim/build/primecount", "--nth-prime", scale, "--time"])
    status = "EXACT MATCH" if expected in out else f"MISMATCH ({out})"
    print(f"{scale:<8} | {expected:<18} | {t*1000:8.2f} ms   | {status}")

print("\n[3] SINGLE-THREAD PI(X) BENCHMARKS (1 Thread):")
scales_pi_single = [
    ("1e10", "455052511"),
    ("1e11", "4118054813"),
    ("1e12", "37607912018"),
    ("1e13", "346065536839"),
    ("1e14", "3204941750802"),
]
print(f"{'Scale':<8} | {'Expected Pi(x)':<18} | {'Compute Time':<14} | {'Status'}")
print("-" * 55)
for scale, expected in scales_pi_single:
    t, out = run_primecount(["./primecount_kim/build/primecount", scale, "--threads=1", "--time"])
    status = "EXACT MATCH" if expected in out else f"MISMATCH ({out})"
    print(f"{scale:<8} | {expected:<18} | {t*1000:8.2f} ms   | {status}")

print("\n" + "=" * 70)
