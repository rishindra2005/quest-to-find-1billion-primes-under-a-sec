.intel_syntax noprefix
.text

# ==============================================================================
# Sieve Kernel in Pure x86_64 Assembly (Intel Syntax)
# Hand-crafted and tuned for AMD Zen 5 / x86_64 architectures
# Utilizes L1/L2 cache-resident segments, AVX-512, and BMI1/BMI2 instructions
# ==============================================================================

# ------------------------------------------------------------------------------
# Function: sieve_segment_asm
# Signature: void sieve_segment_asm(uint8_t *seg, uint64_t seg_size,
#                                  const uint32_t *primes, uint32_t *offsets,
#                                  uint64_t num_primes);
#
# Arguments (SysV x86_64 ABI):
#   rdi = seg        (pointer to segment buffer, aligned to 64 bytes)
#   rsi = seg_size   (number of bytes/odd numbers in segment, e.g. 65536)
#   rdx = primes     (pointer to uint32_t array of base primes)
#   rcx = offsets    (pointer to uint32_t array of relative offsets)
#   r8  = num_primes (number of base primes to sieve)
# ------------------------------------------------------------------------------
.globl sieve_segment_asm
.type sieve_segment_asm, @function
sieve_segment_asm:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    xor r9, r9                  # r9 = prime index i = 0

.Lprime_loop:
    cmp r9, r8
    jae .Ldone_sieve

    mov r10d, [rdx + r9*4]      # r10 = prime p
    mov r11d, [rcx + r9*4]      # r11 = offset off

    # If offset >= seg_size, prime does not hit this segment
    cmp r11, rsi
    jae .Lskip_no_hit

    # Check if prime is >= seg_size (large prime: at most 1 hit in segment)
    cmp r10, rsi
    jae .Llarge_prime_hit

    # Dispatch based on prime size for loop unrolling
    cmp r10, 16
    jbe .Lunroll16
    cmp r10, 64
    jbe .Lunroll8
    cmp r10, 512
    jbe .Lunroll4
    jmp .Lsingle_stride

# ------------------------------------------------------------------------------
# Unrolled 16x loop for very small primes (p <= 16: p = 3, 5, 7, 11, 13)
# These primes account for the vast majority of store instructions.
# ------------------------------------------------------------------------------
.Lunroll16:
    lea rax, [r10 * 8]          # 8 * p
    lea rax, [rax + rax]        # 16 * p
.Lunroll16_loop:
    lea r12, [r11 + rax]
    cmp r12, rsi
    jae .Lunroll8

    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    jmp .Lunroll16_loop

# ------------------------------------------------------------------------------
# Unrolled 8x loop for primes (16 < p <= 64)
# ------------------------------------------------------------------------------
.Lunroll8:
    lea rax, [r10 * 8]          # 8 * p
.Lunroll8_loop:
    lea r12, [r11 + rax]
    cmp r12, rsi
    jae .Lunroll4

    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    jmp .Lunroll8_loop

# ------------------------------------------------------------------------------
# Unrolled 4x loop for primes (64 < p <= 512)
# ------------------------------------------------------------------------------
.Lunroll4:
    lea rax, [r10 * 4]          # 4 * p
.Lunroll4_loop:
    lea r12, [r11 + rax]
    cmp r12, rsi
    jae .Lsingle_stride

    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    mov byte ptr [rdi + r11], 1
    add r11, r10
    jmp .Lunroll4_loop

# ------------------------------------------------------------------------------
# Single stride loop for medium primes (512 < p < seg_size)
# ------------------------------------------------------------------------------
.Lsingle_stride:
    cmp r11, rsi
    jae .Lfinish_prime
    mov byte ptr [rdi + r11], 1
    add r11, r10
    jmp .Lsingle_stride

.Lfinish_prime:
    # r11 >= rsi. Offset for next segment is r11 - rsi
    sub r11, rsi
    mov [rcx + r9*4], r11d
    inc r9
    jmp .Lprime_loop

# ------------------------------------------------------------------------------
# Fast path for large primes (p >= seg_size)
# Exactly 1 hit in segment: mark once, advance by p, and subtract seg_size
# ------------------------------------------------------------------------------
.Llarge_prime_hit:
    mov byte ptr [rdi + r11], 1
    add r11, r10
    sub r11, rsi
    mov [rcx + r9*4], r11d
    inc r9
    jmp .Lprime_loop

.Lskip_no_hit:
    sub r11, rsi
    mov [rcx + r9*4], r11d
    inc r9
    jmp .Lprime_loop

.Ldone_sieve:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

# ------------------------------------------------------------------------------
# Function: count_primes_avx512
# Signature: uint64_t count_primes_avx512(const uint8_t *seg, uint64_t seg_size);
#
# Counts bytes with value 0 (0 = prime, 1 = composite)
# 4-way unrolled AVX-512 vector comparisons (256 bytes per iteration)
# Arguments:
#   rdi = seg (64-byte aligned)
#   rsi = seg_size
# Returns:
#   rax = count of primes (zeros)
# ------------------------------------------------------------------------------
.globl count_primes_avx512
.type count_primes_avx512, @function
count_primes_avx512:
    xor rax, rax                # total count = 0
    xor rdx, rdx                # current byte index = 0
    vpxord zmm1, zmm1, zmm1     # zmm1 = 512-bit vector of zeros

.Lavx512_256b_loop:
    lea r8, [rdx + 256]
    cmp r8, rsi
    ja .Lavx512_64b_loop

    # Load 4x 64-byte chunks
    vmovdqa64 zmm2, [rdi + rdx]
    vmovdqa64 zmm3, [rdi + rdx + 64]
    vmovdqa64 zmm4, [rdi + rdx + 128]
    vmovdqa64 zmm5, [rdi + rdx + 192]

    # Compare against 0 -> produces bitmask (1 if byte == 0)
    vpcmpeqb k1, zmm2, zmm1
    vpcmpeqb k2, zmm3, zmm1
    vpcmpeqb k3, zmm4, zmm1
    vpcmpeqb k4, zmm5, zmm1

    kmovq r8, k1
    kmovq r9, k2
    kmovq r10, k3
    kmovq r11, k4

    popcnt r8, r8
    popcnt r9, r9
    popcnt r10, r10
    popcnt r11, r11

    add rax, r8
    add rax, r9
    add rax, r10
    add rax, r11

    add rdx, 256
    jmp .Lavx512_256b_loop

.Lavx512_64b_loop:
    lea r8, [rdx + 64]
    cmp r8, rsi
    ja .Lavx512_scalar_tail

    vmovdqa64 zmm2, [rdi + rdx]
    vpcmpeqb k1, zmm2, zmm1
    kmovq r8, k1
    popcnt r8, r8
    add rax, r8

    add rdx, 64
    jmp .Lavx512_64b_loop

.Lavx512_scalar_tail:
    cmp rdx, rsi
    jae .Lcount_done
    movzx r8d, byte ptr [rdi + rdx]
    test r8d, r8d
    jnz .Lscalar_skip
    inc rax
.Lscalar_skip:
    inc rdx
    jmp .Lavx512_scalar_tail

.Lcount_done:
    vzeroupper
    ret

# ------------------------------------------------------------------------------
# Function: find_nth_prime_in_seg_asm
# Signature: uint64_t find_nth_prime_in_seg_asm(const uint8_t *seg,
#                                               uint64_t seg_size,
#                                               uint64_t target_rank,
#                                               uint64_t low_k);
#
# Locates the target_rank-th prime (zero byte) within the segment.
# Uses AVX-512 + BMI1 instructions (tzcnt, blsr) to pinpoint the exact byte.
#
# Arguments:
#   rdi = seg
#   rsi = seg_size
#   rdx = target_rank (1-based index within this segment)
#   rcx = low_k (odd number base index, where odd number = 2*low_k + 1)
# Returns:
#   rax = 64-bit prime number
# ------------------------------------------------------------------------------
.globl find_nth_prime_in_seg_asm
.type find_nth_prime_in_seg_asm, @function
find_nth_prime_in_seg_asm:
    push rbx
    push r12
    push r13
    push r14

    xor r8, r8                  # accumulated zeros = 0
    xor r9, r9                  # byte offset = 0
    vpxord zmm1, zmm1, zmm1     # zero vector

.Lfind_chunk_loop:
    lea rax, [r9 + 64]
    cmp rax, rsi
    ja .Lfind_scalar

    vmovdqa64 zmm0, [rdi + r9]
    vpcmpeqb k1, zmm0, zmm1
    kmovq r10, k1               # r10 = bitmask of zeros
    popcnt r11, r10             # r11 = count of zeros in this 64-byte chunk

    lea r12, [r8 + r11]
    cmp r12, rdx
    jae .Lfound_in_chunk        # Target rank is inside this 64-byte block!

    mov r8, r12
    add r9, 64
    jmp .Lfind_chunk_loop

.Lfound_in_chunk:
    # Target prime is inside this 64-byte chunk (mask in r10)
    # Use tzcnt and blsr to examine bit by bit
.Lbit_scan_loop:
    tzcnt rax, r10              # rax = bit index (byte offset within chunk)
    blsr r10, r10               # clear lowest set bit
    inc r8
    cmp r8, rdx
    je .Lcalculate_prime
    jmp .Lbit_scan_loop

.Lfind_scalar:
    # Fallback scalar scan for remaining bytes
    cmp r9, rsi
    jae .Lnot_found
    movzx r10d, byte ptr [rdi + r9]
    test r10d, r10d
    jnz .Lfind_scalar_next
    inc r8
    cmp r8, rdx
    je .Lscalar_calculate
.Lfind_scalar_next:
    inc r9
    jmp .Lfind_scalar

.Lscalar_calculate:
    # prime = 2 * (low_k + r9) + 1
    lea rax, [rcx + r9]
    lea rax, [rax * 2 + 1]
    jmp .Lfind_done

.Lcalculate_prime:
    # prime = 2 * (low_k + r9 + rax) + 1
    add rax, r9                 # rax = total byte offset in segment
    add rax, rcx                # rax = global k index
    lea rax, [rax * 2 + 1]      # rax = 2*k + 1 (the prime value!)
    jmp .Lfind_done

.Lnot_found:
    xor rax, rax

.Lfind_done:
    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
