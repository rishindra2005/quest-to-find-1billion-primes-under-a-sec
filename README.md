# The Quest to Find the 1-Billionth Prime Under a Second
### Hardware-Accelerated x86_64 Assembly, AVX-512 Vectorization, and Zen 5 Cache Optimization

[![OEIS A006988](https://img.shields.io/badge/OEIS-A006988%20Exact%20Match-brightgreen?style=for-the-badge)](https://oeis.org/A006988)
[![x86_64 Assembly](https://img.shields.io/badge/Assembly-x86__64%20Pure-blue?style=for-the-badge&logo=assemblyscript)](./sieve_kernel.s)
[![AVX-512](https://img.shields.io/badge/SIMD-AVX--512%20%2B%20BMI1-orange?style=for-the-badge)](./sieve_kernel.s)
[![Throughput](https://img.shields.io/badge/Throughput-25.38%20Billion%20num%2Fs-purple?style=for-the-badge)](#empirical-performance--benchmarks)
[![Runtime](https://img.shields.io/badge/Runtime-0.898s%20(24%20Threads)-red?style=for-the-badge)](#empirical-performance--benchmarks)

---

## 🎯 The Milestone

$$\mathbf{p_{1,000,000,000} = 22,801,763,489}$$

Finding the **one-billionth prime number** requires sieving past **22.8 billion integers**. Conventional approaches fail catastrophically:
- **Trial division** requires trillions of modulo checks—taking **centuries** of CPU time.
- **Monolithic sieve arrays** demand **23 Gigabytes** of RAM, blowing past CPU cache and stalling on the high-latency DRAM **Memory Wall**.

By synthesizing the **Segmented Sieve of Eratosthenes**, **Wheel-2 factorization**, strict **L1/L2 CPU cache residency (64 KiB segments)**, hand-crafted **x86_64 assembly with prime-classified loop unrolling**, native **512-bit AVX-512 zero-byte vector counting**, **BMI1 bit-scanning**, and **lock-free dynamic chunking**, we compute $p_{10^9}$ in **under one second**.

---

## ⚡ Executive Performance Highlights

| Engine & Mode | Algorithm & Vectorization | Hardware Threads | Runtime ($N = 10^9$) | Status |
| :--- | :--- | :---: | :---: | :---: |
| **Combinatorial Skip Sieve** (`prime_fast`) | **Meissel-Lehmer + Wheel-210 + AVX-512** | **1 (Single Core)** | **0.061 seconds (61 ms)** | **PASS** |
| **Multi-Core Combinatorial** (`prime_fast_omp`) | **Parallel Meissel + Wheel-210 + AVX-512** | **24 Threads** | **0.064 seconds (64 ms)** | **PASS** |
| **Full Parallel Sieve Engine** (`prime_engine`) | **Segmented Sieve + AVX-512 + Lock-Free** | **24 Threads** | **0.898 seconds (898 ms)** | **PASS** |
| **Pure Assembly Standalone** (`prime_standalone`)| **100% x86_64 Hand-Crafted Assembly** | **1 (Single Core)** | **9.141 seconds** | **PASS** |

*Target System: AMD Ryzen AI 9 HX 370 (Zen 5 microarchitecture, 12 cores, 24 threads, 5.16 GHz, 48 KiB L1d/core, 1,024 KiB L2/core, 24 MiB L3, AVX-512, BMI1/BMI2).*

---

## 🎬 11-Minute Manim Documentary Video

A complete 11-minute educational documentary video explaining every step of this engineering feat is rendered and available in [`vid/`](./vid):

[![Video Thumbnail](./vid/video_thumbnail.png)](./vid/final_billionth_prime_10min.mp4)

- **Video File**: [`vid/final_billionth_prime_10min.mp4`](./vid/final_billionth_prime_10min.mp4)
- **Runtime**: **11 minutes 13 seconds** (720p 30fps H.264, 44.1kHz Stereo)
- **Style**: Synthesis of **3Blue1Brown** (visual geometry & animations), **Veritasium** (physical silicon paradoxes), **Terence Tao** (rigorous prime bounds), and **Richard Feynman** (mechanical micro-op intuition).
- **Narrator**: Studio Neural Voiceover (`en-US-ChristopherNeural`).
- **Chapter Guide**:
  - `00:00` — **Act I**: The Frontier of Primes & The Billionth Target
  - `01:39` — **Act II**: The Physical Silicon & The CPU Memory Wall
  - `03:34` — **Act III**: The Architecture of the Segmented Sieve & Wheel Factorization
  - `05:43` — **Act IV**: The Silicon Battlefield: x86_64 Assembly Optimization
  - `07:46` — **Act V**: The Vector Blitz: 512-Bit AVX-512 & BMI1 Bit Pinpointing
  - `09:37` — **Act VI**: Multi-Core Symphony & The Climax

---

## 📖 11-Page Comprehensive LaTeX Documentation Guide

An academic, 11-page technical guide complete with TikZ diagrams, listings, and mathematical proofs is compiled in [`prime_1b_guide.pdf`](./prime_1b_guide.pdf):

<p align="center">
  <img src="./page-01.png" width="30%" alt="Page 1 Preview" />
  <img src="./page-03.png" width="30%" alt="Page 3 Preview" />
  <img src="./page-04.png" width="30%" alt="Page 4 Preview" />
</p>

*Complete document sections: §1 Problem Statement, §2 Why Naive Fails, §3 Segmented Sieve & Wheel-2, §4 Zen 5 Microarchitecture & Memory Wall, §5 Assembly Kernel Deep-Dive, §6 Native AVX-512 Vectorization, §7 BMI1 Prime Pinpointing, §8 Lock-Free Parallel Scheduling, §9 Step-by-Step Miniature Walkthrough, §10 Benchmarks, §11 Architectural Lessons, §12 Verification & Primality Proof.*

---

## 🔬 Core Architectural Innovations

### 1. Wheel-2 Factorization & Odds-Only Arithmetic
All primes except $2$ are odd. We map odd integers directly to indices:
$$X = 2k + 1 \iff k = \frac{X - 1}{2}$$
This instantly halves the search space from $22.8 \times 10^9$ down to $11.4 \times 10^9$ integers.

### 2. The Square Root Bound: Why We Only Need Primes to 151,005
Any composite $M \le 22.8 \times 10^9$ must possess a prime factor $p \le \sqrt{M}$:
$$\sqrt{22,801,763,489} \approx \mathbf{151,002.5} \implies \pi(151,005) = \mathbf{13,847}\text{ primes}$$
Storing 13,847 uint32 primes takes **merely 55.4 Kilobytes of RAM**. The entire base table remains permanently resident in CPU L1/L2 cache for the entire lifetime of the program!

### 3. Slicing in 64 KiB L1/L2 Cache Segments & Zero-Division Offset Propagation
Rather than allocating gigabytes, we slide a 64 KiB buffer ($S = 65,536$ odd numbers = $131,072$ integer span) across the number line. When prime $p$ crosses the segment boundary at index $j \ge S$, its starting offset in the next segment is:
$$\text{next\_offset} = \mathbf{j - S}$$
**Zero divisions and zero modulos** are executed during sieving!

### 4. The Bit Paradox: Why Byte Stores Beat Bit Operations by 27.7×
Textbooks recommend bit-packing with `btr` (Bit Test & Reset) to save RAM:
- **`btr [mem], reg`**: Took **119,299,860 cycles**. It requires a serialized Read-Modify-Write microcode sequence that locks the cache line and stalls the pipeline.
- **`mov byte ptr [mem], 1`**: Took **4,304,220 cycles** (**27.7× Faster!**). Modern Zen 5 cores feature **Dual Store AGUs** that retire 2 stores per clock cycle directly into the store buffer.

### 5. Loop Unrolling Regimes in Hand-Crafted Assembly
In [`sieve_kernel.s`](./sieve_kernel.s), primes are dynamically partitioned to eliminate branch mispredictions:
- $p \le 16$: **16-way unrolled** (zero branch overhead for dense small primes).
- $16 < p \le 64$: **8-way unrolled**.
- $64 < p \le 512$: **4-way unrolled**.
- $p \ge 65,536$: **Branchless single-hit conditional store** (`cmp`, `jb`).

### 6. Native 512-Bit AVX-512 Counting & BMI1 Pinpointing
- **Zero-Byte Counting**: Loads 64 bytes via `vmovdqu8`, compares against zero using `vpcmpeqb`, extracts the 64-bit mask into a scalar register via `kmovq`, and counts prime bits in 1 cycle using hardware `popcnt`. 4-way unrolled to process **256 bytes per loop iteration**.
- **BMI1 Prime Pinpointing**: When the cumulative counter crosses 1 billion, we deploy `tzcnt` (trailing zero count) and `blsr` (reset lowest set bit) to pinpoint the exact 1-billionth prime in single-digit nanoseconds without any linear byte search.

### 7. Lock-Free Dynamic Chunking
The number line is divided into dynamic chunks of $2^{20}$ odd numbers. Threads retrieve work units lock-free via `__atomic_fetch_add`, completely avoiding mutex contention and cache-line bouncing.

### 8. Combinatorial Skip Sieve: Wheel-210 & Meissel-Lehmer (Sub-100ms Breakthrough)
Instead of linearly visiting all 22.8 billion integers, the Combinatorial Skip Sieve jumps directly to the target segment:
1. **Higher-Order Asymptotic Approximation**: Uses Dusart/Axler asymptotic expansions to approximate $p_n$ to within $0.001\%$, placing a starting baseline $x_{\text{start}}$ immediately prior to $p_n$.
2. **Meissel-Lehmer Formula with Wheel-210**: Evaluates the exact prime counting function $\pi(x_{\text{start}})$:
   $$\pi(x) = \Phi(x, a) + a - 1 - P_2(x, a)$$
   where $\Phi(x, 4)$ is accelerated using a **Wheel-210** lookup ($2 \times 3 \times 5 \times 7 = 210$, 48 coprime residues).
3. **AVX-512 Assembly Final Segment Sieve**: Sieves only the tiny remaining window ($< 100\text{k}$ integers) with `sieve_kernel.s` to locate the exact prime.
4. **Result**: Computes the 1-billionth prime in **0.061 seconds (61 ms)** on a **single core** and **0.064 seconds (64 ms)** on **multi-core**!

---

## 🚀 Quick Start & Usage

### Prerequisites
- Linux x86_64 system (Ubuntu / Debian / Arch / Fedora).
- GCC and Make (`sudo apt install build-essential`).
- CPU supporting AVX-512 and BMI1 (e.g. AMD Zen 4 / Zen 5, Intel Xeon Scalable / 11th+ Gen Core).

### Build & Run
```bash
# Clone the repository
git clone git@github.com:rishindra2005/quest-to-find-1billion-primes-under-a-sec.git
cd quest-to-find-1billion-primes-under-a-sec

# Compile all binaries with Zen 5 native optimizations (-O3 -march=native)
make all

# [NEW] Run the ultra-fast Combinatorial Sieve (< 0.07s on 1 Core!)
make run-fast

# [NEW] Run the multi-core Combinatorial Sieve (< 0.07s on 24 Threads!)
make run-fast-omp

# Run the full parallel segmented sieve engine (< 0.9s across all 22.8B integers)
make run

# Run the 100% pure x86_64 standalone assembly implementation (~9.1s)
make run-standalone

# Run the automated OEIS A006988 verification test suite
make test
```

---

## 🧪 Verification & Test Suite

The test suite [`test_suite.c`](./test_suite.c) verifies the algorithm across 9 orders of magnitude against the OEIS A006988 table of $n$-th prime numbers:

```
======================================================================
     Automated Verification Suite: OEIS A006988 (10^1 to 10^9 Primes)
======================================================================
CPU Hardware Threads Detected: 24

Test 1/9: N =         10 | Found:             29 | Expected:             29 |   0.87 ms | [PASS]
Test 2/9: N =        100 | Found:            541 | Expected:            541 |   0.37 ms | [PASS]
Test 3/9: N =       1000 | Found:           7919 | Expected:           7919 |   0.33 ms | [PASS]
Test 4/9: N =      10000 | Found:         104729 | Expected:         104729 |   0.41 ms | [PASS]
Test 5/9: N =     100000 | Found:        1299709 | Expected:        1299709 |   1.10 ms | [PASS]
Test 6/9: N =    1000000 | Found:       15485863 | Expected:       15485863 |   3.49 ms | [PASS]
Test 7/9: N =   10000000 | Found:      179424673 | Expected:      179424673 |   9.15 ms | [PASS]
Test 8/9: N =  100000000 | Found:     2038074743 | Expected:     2038074743 |  65.08 ms | [PASS]
Test 9/9: N = 1000000000 | Found:    22801763489 | Expected:    22801763489 | 890.51 ms | [PASS]
======================================================================
Verification Summary: 9 / 9 Tests Passed (100% Exact Match).
```

---

## 📂 Repository Structure

```
.
├── Makefile                     # Automated build targets (make all, run, test, clean)
├── README.md                    # Project documentation, benchmarks, and architecture
├── prime_1b_guide.pdf           # 11-page comprehensive academic LaTeX PDF guide
├── prime_1b_guide.tex           # LaTeX source with TikZ diagrams and listings
├── page-01.png ... page-11.png  # High-resolution rendered preview of all 11 PDF pages
├── sieve_kernel.s               # Core assembly kernel: unrolled sieving, AVX-512 & BMI1
├── prime_standalone.s           # 100% pure assembly standalone binary (main to exit)
├── prime_engine.c               # Multi-threaded lock-free coordinator & chunk scheduler
├── test_suite.c                 # Automated verification harness for OEIS A006988
└── vid/                         # 11-Minute Manim Documentary Video Suite
    ├── README.md                # Video production documentation & chapter timestamps
    ├── final_billionth_prime_10min.mp4 # Master rendered 11-minute video file
    ├── video_thumbnail.png      # Cover art thumbnail
    ├── scenes.py                # Manim scene scripts (Acts I through VI)
    ├── generate_audio.py        # Neural TTS voiceover generator
    ├── build_video.py           # Video rendering and audio muxing pipeline
    ├── audio/                   # Narration audio files and timing metadata
    └── frames/                  # High-res extracted snapshot frames for each act
```

---

## 📜 License

MIT License. Designed and engineered for high-performance computing research and educational exploration.
