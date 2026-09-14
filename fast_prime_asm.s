.intel_syntax noprefix
.text

# ==============================================================================
# Ultra-Fast Assembly Kernels for Prime Counting & Sieving
# Targets: AMD Zen 5 / x86_64 with AVX-512, BMI1/BMI2, and POPCNT
# ==============================================================================

# ------------------------------------------------------------------------------
# Function: fast_pi_lookup_asm
# Signature: uint32_t fast_pi_lookup_asm(uint64_t x, const uint64_t *prime_bits, 
#                                       const uint32_t *block_pi);
# Arguments:
#   rdi = x
#   rsi = prime_bits array
#   rdx = block_pi array
# Returns:
#   eax = pi(x)
# ------------------------------------------------------------------------------
.globl fast_pi_lookup_asm
.type fast_pi_lookup_asm, @function
fast_pi_lookup_asm:
    mov rax, rdi
    shr rax, 6                      # rax = word index w = x / 64
    mov r8d, [rdx + rax*4]          # r8d = block_pi[w]
    mov r9, [rsi + rax*8]           # r9 = prime_bits[w]

    and edi, 63                     # edi = rem = x % 64
    inc edi                         # len = rem + 1
    mov r10, -1
    bzhi r10, r10, rdi              # r10 = mask of (rem+1) bits (BMI2)

    and r9, r10                     # r9 = prime_bits[w] & mask
    popcnt rax, r9                  # rax = popcnt
    add eax, r8d                    # eax = block_pi[w] + popcnt
    ret

# ------------------------------------------------------------------------------
# Function: compute_p2_asm
# Signature: int64_t compute_p2_asm(uint64_t x, uint32_t a, uint32_t b,
#                                  const uint32_t *primes,
#                                  const uint64_t *prime_bits,
#                                  const uint32_t *block_pi);
# Arguments:
#   rdi = x
#   esi = a
#   edx = b
#   rcx = primes array (uint32_t*)
#   r8  = prime_bits array (uint64_t*)
#   r9  = block_pi array (uint32_t*)
# Returns:
#   rax = sum_{i = a+1}^b (pi(x / p_i) - i + 1)
# ------------------------------------------------------------------------------
.globl compute_p2_asm
.type compute_p2_asm, @function
compute_p2_asm:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi                    # r12 = x
    mov r13d, esi                   # r13d = i = a
    inc r13d                        # r13d = a + 1
    mov r14d, edx                   # r14d = b
    mov r15, rcx                    # r15 = primes array

    xor rbp, rbp                    # rbp = accumulator p2 = 0

.Lp2_loop:
    cmp r13d, r14d
    ja .Lp2_done

    # Compute q = x / primes[i]
    mov eax, [r15 + r13*4]          # eax = p_i
    mov ebx, eax                    # ebx = p_i

    mov rax, r12                    # rax = x
    xor edx, edx
    div rbx                         # rax = q = x / p_i

    # Lookup fast_pi(q):
    # inlined fast_pi:
    mov r10, rax
    shr r10, 6                      # r10 = w = q / 64
    mov ecx, [r9 + r10*4]           # ecx = block_pi[w]
    mov r11, [r8 + r10*8]           # r11 = prime_bits[w]

    and eax, 63                     # eax = rem = q % 64
    inc eax                         # len = rem + 1
    mov rbx, -1
    bzhi rbx, rbx, rax              # rbx = mask (BMI2)

    and r11, rbx
    popcnt r11, r11
    add ecx, r11d                   # ecx = pi(q)

    # Accumulate: p2 += pi(q) - i + 1
    sub ecx, r13d
    inc ecx
    add rbp, rcx

    inc r13d                        # i++
    jmp .Lp2_loop

.Lp2_done:
    mov rax, rbp                    # return p2
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
