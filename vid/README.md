# Engineering the Billionth Prime ($p_{10^9} = 22,801,763,489$)
## 10-Minute Manim Documentary & Video Production Pipeline

---

## 🎬 Video Overview

- **Final Master Video**: [`final_billionth_prime_10min.mp4`](file:///home/rishi/Desktop/tmp/1bgem/vid/final_billionth_prime_10min.mp4)
- **Total Runtime**: **11 minutes 13 seconds** ($673.64$ seconds)
- **Format**: H.264 High Profile, $1280 \times 720$, 30.00 fps progressive
- **Audio**: AAC-LC Stereo, $44.1\text{ kHz}$, $192\text{ kbps}$, studio-grade neural voiceover
- **Narrator**: `en-US-ChristopherNeural` (Documentary / Educational authority style)
- **Pedagogical Influences**: **3Blue1Brown** (visual geometry and dynamic equations), **Veritasium** (physical paradoxes and tension), **Terence Tao** (rigorous sieve bounds and prime density), and **Richard Feynman** (direct mechanical intuition of transistors, AGUs, and CPU pipelines).

---

## ⏱️ Chapter Guide & Timestamps

| Timestamp | Act | Topic Covered |
| :--- | :--- | :--- |
| `00:00` | **Act I** | **The Frontier of Primes & The Billionth Target**<br>Prime atoms of arithmetic, Gauss's Prime Number Theorem $\pi(x) \sim x / \ln x$, inverting to $x \approx 22.8 \times 10^9$, OEIS A006988 ($p_{10^9} = 22,801,763,489$), why trial division fails. |
| `01:39` | **Act II** | **The Physical Silicon & The CPU Memory Wall**<br>The Sieve of Eratosthenes, the 23 GB vs 2.85 GB scaling barrier, the physics of 5 GHz CPUs (0.19 ns cycles, light travels 6 cm), L1/L2/L3 cache vs DRAM latency, the 250-cycle Memory Wall. |
| `03:34` | **Act III** | **The Architecture of the Segmented Sieve & Wheel Factorization**<br>Wheel-2 factorization (odds only, search space halved), square root bound ($\sqrt{22.8 \times 10^9} \approx 151,005$), only 13,847 base primes needed (55.4 KB fits in L1), 64 KiB sliding window, zero-division offset propagation ($j - S$). |
| `05:43` | **Act IV** | **The Silicon Battlefield: x86_64 Assembly Optimization**<br>The bit fallacy: `btr [mem], reg` (119M cycles, RMW stall) vs direct byte stores (4.3M cycles, 27.7x faster) via Zen 5 Dual Store AGUs, loop unrolling (16x, 8x, 4x), branchless single-hit stores for $p \ge 65,536$. |
| `07:46` | **Act V** | **The Vector Blitz: 512-Bit AVX-512 & BMI1 Bit Pinpointing**<br>Zero-byte prime counting bottleneck, 512-bit vector registers (`zmm0`), `vmovdqu8` (64 bytes/cycle), `vpcmpeqb` parallel zero comparison, `kmovq` mask transfer, hardware single-cycle `popcnt`, BMI1 pinpointing (`tzcnt` & `blsr`). |
| `09:37` | **Act VI** | **Multi-Core Symphony & The Climax**<br>24 hardware threads, lock-free work-stealing scheduler with atomic fetch-and-add (`__atomic_fetch_add`), empirical benchmarks: 9.28s (1 core) down to **0.898s** (24 threads), 25.38B numbers/sec, 1.11B primes/sec, revelation of $p_{10^9} = 22,801,763,489$. |

---

## 🛠️ Pipeline Architecture & Components

```
1bgem/vid/
├── venv/                           # Dedicated Python 3.12 virtual environment
├── audio/
│   ├── scene1_audio.wav            # Act 1 narration audio (44.1kHz WAV)
│   ├── scene2_audio.wav            # Act 2 narration audio
│   ├── scene3_audio.wav            # Act 3 narration audio
│   ├── scene4_audio.wav            # Act 4 narration audio
│   ├── scene5_audio.wav            # Act 5 narration audio
│   ├── scene6_audio.wav            # Act 6 narration audio
│   └── narration_timings.json      # Exact measured millisecond durations & script text
├── output/
│   ├── scene1_synced.mp4           # Act 1 video + audio synced
│   ├── scene2_synced.mp4           # Act 2 video + audio synced
│   ├── scene3_synced.mp4           # Act 3 video + audio synced
│   ├── scene4_synced.mp4           # Act 4 video + audio synced
│   ├── scene5_synced.mp4           # Act 5 video + audio synced
│   ├── scene6_synced.mp4           # Act 6 video + audio synced
│   └── concat_list.txt             # Demuxer concat manifest
├── frames/                         # High-res extracted snapshot frames from each act
├── scenes.py                       # Manim scene animations (Act1Scene to Act6Scene)
├── generate_audio.py               # Edge-TTS voiceover synthesis & audio processing
├── build_video.py                  # Master build orchestrator (render -> mux -> concat)
├── video_thumbnail.png             # Climax cover thumbnail
└── final_billionth_prime_10min.mp4 # Master 11-minute documentary video file
```

---

## 🚀 How to Re-render or Modify

### 1. Activate the Virtual Environment
```bash
cd /home/rishi/Desktop/tmp/1bgem/vid
source venv/bin/activate
```

### 2. Re-synthesize Narration Audio (Optional)
If you modify the text in `generate_audio.py`:
```bash
python generate_audio.py
```

### 3. Build & Render Master Video
```bash
python build_video.py
```
This script will automatically render any outdated scene with Manim, mux it with the synchronized audio track, and stitch together `final_billionth_prime_10min.mp4`.
