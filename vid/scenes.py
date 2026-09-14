"""
Manim Scenes for 10-Minute Video: "Engineering the Billionth Prime"
Style: 3Blue1Brown, Veritasium, Terence Tao, and Richard Feynman
Synchronized to narration audio tracks.
"""

from manim import *
import json
import os

# Load narration timings
TIMINGS_PATH = "/home/rishi/Desktop/tmp/1bgem/vid/audio/narration_timings.json"
with open(TIMINGS_PATH, "r") as f:
    TIMINGS = json.load(f)

# Palette
DARK_BG = "#0b0f19"
CYAN = "#38bdf8"
GOLD = "#fbbf24"
EMERALD = "#34d399"
ROSE = "#f43f5e"
PURPLE = "#c084fc"
GRAY_TEXT = "#94a3b8"

class BaseDocumentaryScene(Scene):
    def setup(self):
        self.camera.background_color = DARK_BG

    def create_act_header(self, act_num, act_title):
        tag = Text(f"ACT {act_num}", font_size=20, color=GOLD, weight=BOLD)
        title = Text(act_title, font_size=28, color=WHITE, weight=BOLD)
        header = VGroup(tag, title).arrange(RIGHT, buff=0.4).to_edge(UP, buff=0.5)
        line = Line(LEFT*6.5, RIGHT*6.5, color=GRAY_TEXT, stroke_width=1.5).next_to(header, DOWN, buff=0.2)
        return VGroup(header, line)


# ==============================================================================
# ACT I: The Frontier of Primes & The Billionth Target
# Target duration: ~99.22s
# ==============================================================================
class Act1Scene(BaseDocumentaryScene):
    def construct(self):
        dur = TIMINGS["scene1"]["duration"] # 99.22s
        header = self.create_act_header("I", "The Frontier of Primes & The Billionth Target")
        self.play(FadeIn(header), run_time=1.5)

        # 1. Prime Atoms Visualization (0 - 18s)
        intro_text = Text("Primes: The Indivisible Atoms of Arithmetic", font_size=32, color=CYAN)
        intro_text.next_to(header, DOWN, buff=0.8)
        self.play(Write(intro_text), run_time=2.0)

        # Prime numbers appearing on a line
        num_line = NumberLine(x_range=[1, 20, 1], length=11, include_numbers=True, font_size=20)
        num_line.next_to(intro_text, DOWN, buff=0.8)
        self.play(Create(num_line), run_time=2.0)

        primes = [2, 3, 5, 7, 11, 13, 17, 19]
        dots = VGroup()
        for p in primes:
            dot = Dot(num_line.n2p(p), color=GOLD, radius=0.12)
            glow = Dot(num_line.n2p(p), color=GOLD, radius=0.25).set_opacity(0.3)
            dots.add(VGroup(glow, dot))
        
        self.play(LaggedStart(*[FadeIn(d, scale=1.5) for d in dots], lag_ratio=0.2), run_time=4.0)
        self.wait(8.0) # Total so far: 17.5s

        # 2. Gauss & The Prime Number Theorem (18 - 45s)
        self.play(FadeOut(num_line), FadeOut(dots), FadeOut(intro_text), run_time=1.5)
        
        gauss_title = Text("Carl Friedrich Gauss (1792): Density of Primes", font_size=26, color=EMERALD)
        gauss_title.next_to(header, DOWN, buff=0.6)
        self.play(Write(gauss_title), run_time=1.5)

        pnt_formula = MathTex(r"\pi(x) \sim \frac{x}{\ln x}", font_size=54, color=GOLD)
        pnt_formula.next_to(gauss_title, DOWN, buff=0.7)
        self.play(Write(pnt_formula), run_time=2.0)

        density_desc = Text(
            "The probability that a random integer x is prime drops as 1 / ln(x)",
            font_size=22, color=GRAY_TEXT
        ).next_to(pnt_formula, DOWN, buff=0.5)
        self.play(FadeIn(density_desc), run_time=1.5)
        self.wait(18.0) # Total so far: 42.0s

        # 3. The 1-Billionth Prime Target (45 - 75s)
        self.play(FadeOut(gauss_title), FadeOut(pnt_formula), FadeOut(density_desc), run_time=1.5)

        target_title = Text("The Question: What is the 1,000,000,000th Prime?", font_size=28, color=ROSE, weight=BOLD)
        target_title.next_to(header, DOWN, buff=0.6)
        self.play(Write(target_title), run_time=1.5)

        inv_pnt = MathTex(
            r"\pi(x) = 10^9 \implies x \approx 10^9 \cdot \ln(10^9) \approx 22.8 \times 10^9",
            font_size=36, color=CYAN
        ).next_to(target_title, DOWN, buff=0.6)
        self.play(Write(inv_pnt), run_time=2.5)

        exact_box = RoundedRectangle(corner_radius=0.2, height=1.6, width=10, color=GOLD, fill_color="#1e293b", fill_opacity=0.9)
        exact_box.next_to(inv_pnt, DOWN, buff=0.6)
        oeis_tag = Text("OEIS A006988 Confirmed Answer:", font_size=20, color=GRAY_TEXT).move_to(exact_box.get_top() + DOWN*0.35)
        exact_prime = Text("p₁₀₉ = 22,801,763,489", font_size=42, color=GOLD, weight=BOLD).move_to(exact_box.get_bottom() + UP*0.45)
        
        self.play(FadeIn(exact_box), Write(oeis_tag), Write(exact_prime), run_time=2.5)
        self.wait(20.0) # Total so far: 69.5s

        # 4. Why Brute Force Fails (75 - 99.2s)
        self.play(FadeOut(inv_pnt), FadeOut(exact_box), FadeOut(oeis_tag), FadeOut(exact_prime), FadeOut(target_title), run_time=1.5)

        trial_title = Text("The Failure of Brute Force (Trial Division)", font_size=28, color=ROSE)
        trial_title.next_to(header, DOWN, buff=0.6)
        
        stat_1 = Text("• Candidate count: 22.8 Billion numbers", font_size=24, color=WHITE)
        stat_2 = Text("• Division checks: Trillions of modulos", font_size=24, color=WHITE)
        stat_3 = Text("• Compute time at 5 GHz: Centuries!", font_size=24, color=ROSE, weight=BOLD)
        stat_group = VGroup(stat_1, stat_2, stat_3).arrange(DOWN, aligned_edge=LEFT, buff=0.4).next_to(trial_title, DOWN, buff=0.8)

        self.play(Write(trial_title), run_time=1.5)
        self.play(LaggedStart(*[FadeIn(s, shift=RIGHT*0.5) for s in stat_group], lag_ratio=0.4), run_time=3.0)

        remaining_time = max(1.0, dur - 75.5)
        self.wait(remaining_time)


# ==============================================================================
# ACT II: The Physical Silicon & The CPU Memory Wall
# Target duration: ~114.65s
# ==============================================================================
class Act2Scene(BaseDocumentaryScene):
    def construct(self):
        dur = TIMINGS["scene2"]["duration"] # 114.65s
        header = self.create_act_header("II", "The Physical Silicon & The CPU Memory Wall")
        self.play(FadeIn(header), run_time=1.5)

        # 1. Eratosthenes Sieve Concept (0 - 25s)
        sieve_title = Text("The Sieve of Eratosthenes (276–194 BC)", font_size=28, color=CYAN)
        sieve_title.next_to(header, DOWN, buff=0.6)
        self.play(Write(sieve_title), run_time=1.5)

        grid = VGroup()
        for i in range(1, 21):
            sq = Square(side_length=0.7, stroke_color=GRAY_TEXT, stroke_width=1, fill_color="#1e293b", fill_opacity=0.8)
            num = Text(str(i), font_size=20, color=WHITE).move_to(sq.get_center())
            grid.add(VGroup(sq, num))
        grid.arrange_in_grid(rows=4, cols=5, buff=0.15).next_to(sieve_title, DOWN, buff=0.6)
        self.play(Create(grid), run_time=2.5)

        # Strike composites (2 is prime, strike 4,6,8,10,12,14,16,18,20)
        strike_lines = VGroup()
        evens = [4, 6, 8, 10, 12, 14, 16, 18, 20]
        for e in evens:
            cell = grid[e-1]
            l = Line(cell.get_corner(UL), cell.get_corner(DR), color=ROSE, stroke_width=3)
            strike_lines.add(l)
        self.play(grid[1][0].animate.set_fill(GOLD, opacity=0.8), run_time=1.0)
        self.play(Create(strike_lines), run_time=2.0)
        self.wait(14.0) # Total so far: 22.5s

        # 2. Memory Scaling Barrier (25 - 50s)
        self.play(FadeOut(sieve_title), FadeOut(grid), FadeOut(strike_lines), run_time=1.5)

        mem_title = Text("The Sieve Scaling Catastrophe at 22.8 Billion", font_size=28, color=ROSE, weight=BOLD)
        mem_title.next_to(header, DOWN, buff=0.6)
        
        m_byte = Text("1 byte per number  ➔  22.8 Gigabytes RAM", font_size=24, color=WHITE)
        m_bit  = Text("1 bit per number   ➔   2.85 Gigabytes RAM", font_size=24, color=GOLD)
        m_warn = Text("Both far exceed CPU on-chip cache capacities!", font_size=24, color=ROSE)
        mem_group = VGroup(m_byte, m_bit, m_warn).arrange(DOWN, aligned_edge=LEFT, buff=0.5).next_to(mem_title, DOWN, buff=0.8)

        self.play(Write(mem_title), run_time=1.5)
        self.play(LaggedStart(*[FadeIn(m, shift=UP*0.3) for m in mem_group], lag_ratio=0.4), run_time=3.0)
        self.wait(18.0) # Total so far: 46.5s

        # 3. CPU Clock vs Speed of Light & Cache Pyramid (50 - 90s)
        self.play(FadeOut(mem_title), FadeOut(mem_group), run_time=1.5)

        pyramid_title = Text("Modern CPU Memory Hierarchy (AMD Zen 5)", font_size=26, color=CYAN)
        pyramid_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(pyramid_title), run_time=1.5)

        # Draw pyramid blocks
        l1 = Rectangle(width=3.0, height=0.7, color=EMERALD, fill_color=EMERALD, fill_opacity=0.3)
        l1_txt = Text("L1 Data Cache (48 KB) ~4 cycles (0.9 ns)", font_size=16, color=WHITE).move_to(l1)

        l2 = Rectangle(width=5.0, height=0.7, color=CYAN, fill_color=CYAN, fill_opacity=0.3).next_to(l1, DOWN, buff=0.1)
        l2_txt = Text("L2 Cache (1,024 KB) ~14 cycles (3.5 ns)", font_size=16, color=WHITE).move_to(l2)

        l3 = Rectangle(width=7.2, height=0.7, color=PURPLE, fill_color=PURPLE, fill_opacity=0.3).next_to(l2, DOWN, buff=0.1)
        l3_txt = Text("L3 Shared Cache (24 MB) ~45 cycles (11 ns)", font_size=16, color=WHITE).move_to(l3)

        dram = Rectangle(width=9.5, height=0.8, color=ROSE, fill_color=ROSE, fill_opacity=0.3).next_to(l3, DOWN, buff=0.1)
        dram_txt = Text("Main DRAM (32 GB) ~250 cycles (65 ns) ➔ MEMORY WALL", font_size=16, color=GOLD, weight=BOLD).move_to(dram)

        pyr = VGroup(l1, l1_txt, l2, l2_txt, l3, l3_txt, dram, dram_txt).next_to(pyramid_title, DOWN, buff=0.4)
        self.play(FadeIn(pyr), run_time=3.0)
        self.wait(28.0) # Total so far: 80.5s

        # 4. The Memory Wall Conclusion (90 - 114.6s)
        rule_box = RoundedRectangle(corner_radius=0.2, height=1.2, width=11, color=GOLD, fill_color="#1e293b", fill_opacity=0.9)
        rule_box.next_to(pyr, DOWN, buff=0.4)
        rule_text = Text(
            "Golden Rule: The entire working dataset must live 100% inside CPU Cache!",
            font_size=20, color=GOLD, weight=BOLD
        ).move_to(rule_box)

        self.play(FadeIn(rule_box), Write(rule_text), run_time=2.0)
        remaining_time = max(1.0, dur - 82.5)
        self.wait(remaining_time)


# ==============================================================================
# ACT III: The Architecture of the Segmented Sieve & Wheel Factorization
# Target duration: ~129.14s
# ==============================================================================
class Act3Scene(BaseDocumentaryScene):
    def construct(self):
        dur = TIMINGS["scene3"]["duration"] # 129.14s
        header = self.create_act_header("III", "The Architecture of the Segmented Sieve")
        self.play(FadeIn(header), run_time=1.5)

        # 1. Wheel-2 Factorization (0 - 35s)
        w2_title = Text("Wheel-2 Factorization: Odds-Only Arithmetic", font_size=28, color=CYAN)
        w2_title.next_to(header, DOWN, buff=0.6)
        self.play(Write(w2_title), run_time=1.5)

        w2_formula = MathTex(r"X = 2k + 1 \iff k = \frac{X - 1}{2}", font_size=42, color=GOLD)
        w2_formula.next_to(w2_title, DOWN, buff=0.6)
        self.play(Write(w2_formula), run_time=2.0)

        w2_desc = Text("Eliminates all even numbers upfront: 23 Billion ➔ 11.4 Billion numbers!", font_size=22, color=EMERALD)
        w2_desc.next_to(w2_formula, DOWN, buff=0.5)
        self.play(FadeIn(w2_desc), run_time=1.5)
        self.wait(25.0) # Total so far: 31.5s

        # 2. Square Root Theorem: Why We Only Need Primes to 151,005 (35 - 75s)
        self.play(FadeOut(w2_title), FadeOut(w2_formula), FadeOut(w2_desc), run_time=1.5)

        sqrt_title = Text("The Square Root Bound (Fundamental Theorem of Sieving)", font_size=26, color=GOLD)
        sqrt_title.next_to(header, DOWN, buff=0.6)
        self.play(Write(sqrt_title), run_time=1.5)

        sqrt_eq = MathTex(
            r"\text{Composite } M \le 22.8 \times 10^9 \implies \text{has prime factor } p \le \sqrt{M}",
            font_size=32, color=WHITE
        ).next_to(sqrt_title, DOWN, buff=0.6)
        
        sqrt_calc = MathTex(
            r"\sqrt{22,801,763,489} \approx \mathbf{151,005}",
            font_size=44, color=CYAN
        ).next_to(sqrt_eq, DOWN, buff=0.5)

        prime_count = MathTex(
            r"\pi(151,005) = \mathbf{13,847} \text{ base primes}",
            font_size=38, color=EMERALD
        ).next_to(sqrt_calc, DOWN, buff=0.5)

        size_card = Text("13,847 primes × 4 bytes = 55.4 Kilobytes RAM (Fits entirely in L1 Cache!)", font_size=22, color=GOLD)
        size_card.next_to(prime_count, DOWN, buff=0.5)

        self.play(Write(sqrt_eq), run_time=2.0)
        self.play(Write(sqrt_calc), run_time=1.5)
        self.play(Write(prime_count), run_time=1.5)
        self.play(FadeIn(size_card), run_time=1.5)
        self.wait(27.0) # Total so far: 66.5s

        # 3. Slicing into 64 KiB Sliding Segments (75 - 105s)
        self.play(FadeOut(sqrt_title), FadeOut(sqrt_eq), FadeOut(sqrt_calc), FadeOut(prime_count), FadeOut(size_card), run_time=1.5)

        seg_title = Text("Segmented Slicing: Sliding 64 KiB Window Over Number Line", font_size=26, color=CYAN)
        seg_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(seg_title), run_time=1.5)

        # Sliding window visual
        s0 = Rectangle(width=2.8, height=1.4, color=CYAN, fill_color=CYAN, fill_opacity=0.2)
        s0_t = Text("Seg 0 [1, 131K]\n(64 KiB Buffer)", font_size=16, color=WHITE).move_to(s0)

        s1 = Rectangle(width=2.8, height=1.4, color=EMERALD, fill_color=EMERALD, fill_opacity=0.2).next_to(s0, RIGHT, buff=0.3)
        s1_t = Text("Seg 1 [131K, 262K]\n(Reused Buffer)", font_size=16, color=WHITE).move_to(s1)

        sk = Rectangle(width=2.8, height=1.4, color=GOLD, fill_color=GOLD, fill_opacity=0.2).next_to(s1, RIGHT, buff=0.3)
        sk_t = Text("Seg k [...]\n(100% Cache Hit)", font_size=16, color=WHITE).move_to(sk)

        segs = VGroup(VGroup(s0, s0_t), VGroup(s1, s1_t), VGroup(sk, sk_t)).move_to(ORIGIN)
        self.play(FadeIn(segs), run_time=2.5)
        self.wait(22.0) # Total so far: 94.0s

        # 4. Zero Division Offset Propagation (105 - 129.1s)
        self.play(FadeOut(segs), run_time=1.5)

        offset_title = Text("Continuous Offset Tracking: Zero Div, Zero Modulo!", font_size=26, color=GOLD)
        offset_title.next_to(seg_title, DOWN, buff=0.8)

        offset_formula = MathTex(
            r"\text{Next Segment Offset} = \mathbf{j - S}",
            font_size=52, color=EMERALD
        ).next_to(offset_title, DOWN, buff=0.8)

        offset_sub = Text(
            "When crossing segment boundary S at index j, simply subtract S! Zero arithmetic overhead.",
            font_size=20, color=GRAY_TEXT
        ).next_to(offset_formula, DOWN, buff=0.5)

        self.play(Write(offset_title), Write(offset_formula), FadeIn(offset_sub), run_time=2.5)
        remaining_time = max(1.0, dur - 98.0)
        self.wait(remaining_time)


# ==============================================================================
# ACT IV: The Silicon Battlefield: x86_64 Assembly Optimization
# Target duration: ~122.95s
# ==============================================================================
class Act4Scene(BaseDocumentaryScene):
    def construct(self):
        dur = TIMINGS["scene4"]["duration"] # 122.95s
        header = self.create_act_header("IV", "The Silicon Battlefield: x86_64 Assembly Optimization")
        self.play(FadeIn(header), run_time=1.5)

        # 1. The Textbook Bit Paradox: BTR vs Direct Store (0 - 55s)
        paradox_title = Text("The Bit Fallacy: Theory vs Physical Silicon", font_size=28, color=ROSE, weight=BOLD)
        paradox_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(paradox_title), run_time=1.5)

        # Comparison boxes
        box_btr = RoundedRectangle(corner_radius=0.2, width=5.2, height=3.5, color=ROSE, fill_color="#1e293b", fill_opacity=0.9)
        btr_h = Text("Textbook Bit-Packing\n'btr [mem], reg'", font_size=20, color=ROSE).move_to(box_btr.get_top() + DOWN*0.6)
        btr_stat = Text("119,299,860 cycles\n(1x Baseline)", font_size=22, color=WHITE).move_to(box_btr.get_center())
        btr_why = Text("Serialized Read-Modify-Write\nMicrocode Stall!", font_size=16, color=GRAY_TEXT).move_to(box_btr.get_bottom() + UP*0.6)
        grp_btr = VGroup(box_btr, btr_h, btr_stat, btr_why).to_edge(LEFT, buff=1.0)

        box_mov = RoundedRectangle(corner_radius=0.2, width=5.2, height=3.5, color=EMERALD, fill_color="#1e293b", fill_opacity=0.9)
        mov_h = Text("Direct Byte Store\n'mov byte ptr [mem], 1'", font_size=20, color=EMERALD).move_to(box_mov.get_top() + DOWN*0.6)
        mov_stat = Text("4,304,220 cycles\n(27.7× FASTER!)", font_size=22, color=GOLD, weight=BOLD).move_to(box_mov.get_center())
        mov_why = Text("Dual Store AGUs on Zen 5\n2 stores / clock cycle!", font_size=16, color=GRAY_TEXT).move_to(box_mov.get_bottom() + UP*0.6)
        grp_mov = VGroup(box_mov, mov_h, mov_stat, mov_why).to_edge(RIGHT, buff=1.0)

        self.play(FadeIn(grp_btr), FadeIn(grp_mov), run_time=3.0)
        self.wait(38.0) # Total so far: 44.0s

        # 2. Sieve Loop Unrolling Strategy (55 - 122.9s)
        self.play(FadeOut(paradox_title), FadeOut(grp_btr), FadeOut(grp_mov), run_time=1.5)

        unroll_title = Text("Eliminating Branch Misprediction via Prime Classification", font_size=26, color=CYAN)
        unroll_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(unroll_title), run_time=1.5)

        u1 = Text("• p ≤ 16:      16-Way Unrolled (Zero branch overhead for tiny primes)", font_size=20, color=WHITE)
        u2 = Text("• 16 < p ≤ 64:  8-Way Unrolled", font_size=20, color=WHITE)
        u3 = Text("• 64 < p ≤ 512: 4-Way Unrolled", font_size=20, color=WHITE)
        u4 = Text("• p ≥ 65,536:   Branchless Single-Hit Store (Single CMP + JB)", font_size=20, color=GOLD, weight=BOLD)
        unroll_group = VGroup(u1, u2, u3, u4).arrange(DOWN, aligned_edge=LEFT, buff=0.45).next_to(unroll_title, DOWN, buff=0.7)

        self.play(LaggedStart(*[FadeIn(u, shift=RIGHT*0.5) for u in unroll_group], lag_ratio=0.4), run_time=3.0)

        code_box = RoundedRectangle(corner_radius=0.2, width=10.5, height=1.4, color=CYAN, fill_color="#0f172a", fill_opacity=0.9)
        code_box.next_to(unroll_group, DOWN, buff=0.5)
        asm_snippet = Text("cmp rax, 65536 ; jb 1f ; mov byte ptr [rdi + rax], 1 ; 1: sub rax, 65536", font_size=17, color=EMERALD).move_to(code_box)

        self.play(FadeIn(code_box), Write(asm_snippet), run_time=2.0)
        remaining_time = max(1.0, dur - 52.0)
        self.wait(remaining_time)


# ==============================================================================
# ACT V: The Vector Blitz: 512-Bit AVX-512 & BMI1 Bit Pinpointing
# Target duration: ~111.60s
# ==============================================================================
class Act5Scene(BaseDocumentaryScene):
    def construct(self):
        dur = TIMINGS["scene5"]["duration"] # 111.60s
        header = self.create_act_header("V", "The Vector Blitz: 512-Bit AVX-512 & BMI1")
        self.play(FadeIn(header), run_time=1.5)

        # 1. The Vector Sieve Counting Bottleneck (0 - 30s)
        cnt_title = Text("Counting Primes: 64 KiB Buffer Zero-Detection", font_size=28, color=CYAN)
        cnt_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(cnt_title), run_time=1.5)

        scalar_vs_simd = Text("Scalar loop: 65,536 iterations ➔ Slow!    SIMD: 64 bytes at a time!", font_size=22, color=GOLD)
        scalar_vs_simd.next_to(cnt_title, DOWN, buff=0.4)
        self.play(FadeIn(scalar_vs_simd), run_time=1.5)
        self.wait(20.0) # Total so far: 24.5s

        # 2. AVX-512 Register Pipeline Animation (30 - 75s)
        self.play(FadeOut(cnt_title), FadeOut(scalar_vs_simd), run_time=1.5)

        pipe_title = Text("AVX-512 Zero-Byte Counting Pipeline", font_size=26, color=GOLD)
        pipe_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(pipe_title), run_time=1.5)

        # Steps in pipeline
        st1 = Rectangle(width=10.5, height=0.8, color=CYAN, fill_color="#1e293b", fill_opacity=0.9)
        st1_t = Text("1. VMOVDQU8 zmm0, [rdi] ➔ Load 64 bytes (512 bits) in 1 cycle", font_size=18, color=WHITE).move_to(st1)

        st2 = Rectangle(width=10.5, height=0.8, color=EMERALD, fill_color="#1e293b", fill_opacity=0.9).next_to(st1, DOWN, buff=0.2)
        st2_t = Text("2. VPCMPEQB k1, zmm0, zmm_zero ➔ Compare 64 bytes against 0 simultaneously", font_size=18, color=WHITE).move_to(st2)

        st3 = Rectangle(width=10.5, height=0.8, color=GOLD, fill_color="#1e293b", fill_opacity=0.9).next_to(st2, DOWN, buff=0.2)
        st3_t = Text("3. KMOVQ rax, k1 ➔ Move 64-bit comparison mask to integer register", font_size=18, color=WHITE).move_to(st3)

        st4 = Rectangle(width=10.5, height=0.8, color=PURPLE, fill_color="#1e293b", fill_opacity=0.9).next_to(st3, DOWN, buff=0.2)
        st4_t = Text("4. POPCNT rdx, rax ➔ Count all surviving prime bits in 1 single clock cycle!", font_size=18, color=GOLD, weight=BOLD).move_to(st4)

        pipe_grp = VGroup(VGroup(st1, st1_t), VGroup(st2, st2_t), VGroup(st3, st3_t), VGroup(st4, st4_t)).next_to(pipe_title, DOWN, buff=0.5)
        self.play(LaggedStart(*[FadeIn(s, shift=DOWN*0.3) for s in pipe_grp], lag_ratio=0.3), run_time=3.5)
        self.wait(32.0) # Total so far: 63.0s

        # 3. BMI1 Bit Pinpointing (75 - 111.6s)
        self.play(FadeOut(pipe_title), FadeOut(pipe_grp), run_time=1.5)

        bmi_title = Text("Bit-Level Pinpointing with BMI1 (TZCNT & BLSR)", font_size=26, color=CYAN)
        bmi_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(bmi_title), run_time=1.5)

        b1 = Text("• When running counter crosses 1 Billion: Pinpoint exact prime without linear loops!", font_size=20, color=WHITE)
        b2 = Text("• TZCNT (Trailing Zero Count): Instantly returns bit index of next prime", font_size=20, color=EMERALD)
        b3 = Text("• BLSR (Reset Lowest Set Bit): Peels off primes until rank_needed = 0", font_size=20, color=GOLD)
        b4 = Text("➔ Pinpoints 1-billionth prime in single-digit nanoseconds!", font_size=22, color=ROSE, weight=BOLD)
        bmi_grp = VGroup(b1, b2, b3, b4).arrange(DOWN, aligned_edge=LEFT, buff=0.45).next_to(bmi_title, DOWN, buff=0.7)

        self.play(LaggedStart(*[FadeIn(b, shift=RIGHT*0.5) for b in bmi_grp], lag_ratio=0.4), run_time=3.0)
        remaining_time = max(1.0, dur - 69.0)
        self.wait(remaining_time)


# ==============================================================================
# ACT VI: Multi-Core Symphony & The Climax
# Target duration: ~96.12s
# ==============================================================================
class Act6Scene(BaseDocumentaryScene):
    def construct(self):
        dur = TIMINGS["scene6"]["duration"] # 96.12s
        header = self.create_act_header("VI", "Multi-Core Symphony & The Climax")
        self.play(FadeIn(header), run_time=1.5)

        # 1. 24 Hardware Threads & Lock-Free Atomic Scheduling (0 - 30s)
        mc_title = Text("Parallelizing Across 24 Hardware Threads", font_size=28, color=CYAN)
        mc_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(mc_title), run_time=1.5)

        mc_desc = Text(
            "Lock-Free Work Stealing with Atomic Fetch-and-Add (__atomic_fetch_add)",
            font_size=22, color=GOLD
        ).next_to(mc_title, DOWN, buff=0.4)
        self.play(FadeIn(mc_desc), run_time=1.5)

        # Thread blocks visual
        threads = VGroup()
        for t in range(24):
            tb = Rectangle(width=0.4, height=0.7, color=EMERALD, fill_color=EMERALD, fill_opacity=0.4)
            tt = Text(str(t), font_size=12, color=WHITE).move_to(tb)
            threads.add(VGroup(tb, tt))
        threads.arrange(RIGHT, buff=0.08).next_to(mc_desc, DOWN, buff=0.6)
        self.play(Create(threads), run_time=2.5)
        self.wait(18.0) # Total so far: 25.0s

        # 2. The Benchmark Revelation (30 - 65s)
        self.play(FadeOut(mc_title), FadeOut(mc_desc), FadeOut(threads), run_time=1.5)

        bench_title = Text("Empirical Benchmark Results (AMD Ryzen AI 9 HX 370)", font_size=26, color=GOLD)
        bench_title.next_to(header, DOWN, buff=0.5)
        self.play(Write(bench_title), run_time=1.5)

        res_box = RoundedRectangle(corner_radius=0.2, width=11, height=2.8, color=CYAN, fill_color="#1e293b", fill_opacity=0.9)
        res_box.next_to(bench_title, DOWN, buff=0.4)

        r1 = Text("• Standalone Pure Assembly (1 thread):   9.28 seconds", font_size=20, color=WHITE)
        r2 = Text("• Multi-Core AVX-512 Engine (24 threads): 0.898 seconds!", font_size=22, color=GOLD, weight=BOLD)
        r3 = Text("• Sieving Speed: 25.38 Billion numbers / second", font_size=20, color=EMERALD)
        r4 = Text("• Prime Discovery Rate: 1.11 Billion primes / second", font_size=20, color=CYAN)
        res_group = VGroup(r1, r2, r3, r4).arrange(DOWN, aligned_edge=LEFT, buff=0.25).move_to(res_box)

        self.play(FadeIn(res_box), LaggedStart(*[FadeIn(r) for r in res_group], lag_ratio=0.3), run_time=3.0)
        self.wait(26.0) # Total so far: 55.5s

        # 3. The Climax & Final Reveal (65 - 96.1s)
        self.play(FadeOut(bench_title), FadeOut(res_box), FadeOut(res_group), run_time=1.5)

        final_title = Text("THE 1,000,000,000th PRIME NUMBER", font_size=32, color=GOLD, weight=BOLD)
        final_title.next_to(header, DOWN, buff=0.8)

        final_box = RoundedRectangle(corner_radius=0.3, width=11, height=1.8, color=GOLD, fill_color="#0f172a", fill_opacity=0.95)
        final_box.next_to(final_title, DOWN, buff=0.5)

        final_num = Text("22,801,763,489", font_size=60, color=GOLD, weight=BOLD).move_to(final_box)

        phil = Text(
            "Where Ancient Number Theory meets Modern Silicon.",
            font_size=24, color=CYAN, slant=ITALIC
        ).next_to(final_box, DOWN, buff=0.8)

        self.play(Write(final_title), run_time=1.5)
        self.play(FadeIn(final_box, scale=1.1), Write(final_num), run_time=2.0)
        self.play(FadeIn(phil), run_time=1.5)

        remaining_time = max(1.0, dur - 62.0)
        self.wait(remaining_time)
