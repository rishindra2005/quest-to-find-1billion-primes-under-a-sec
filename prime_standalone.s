.intel_syntax noprefix
.text

# ==============================================================================
# Standalone Pure x86_64 Assembly Prime Sieve Program
# Computes the 1,000,000,000th Prime Number (22,801,763,489)
# Highly optimized for CPU L1/L2 cache (64 KiB) and AVX-512 execution
# ==============================================================================

.globl main
.type main, @function
main:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 56                 # Space for timespec structs

    # Print banner
    lea rdi, [rip + banner_str]
    xor eax, eax
    call printf@PLT

    # Start timing: clock_gettime(CLOCK_MONOTONIC, &t0)
    mov edi, 1                  # CLOCK_MONOTONIC = 1
    lea rsi, [rsp]              # t0 at [rsp]
    call clock_gettime@PLT

    # Step 1: Initialize base primes up to 160,000 in assembly
    mov edi, 160000
    call init_base_primes_asm

    # Print base primes count
    lea rdi, [rip + base_primes_str]
    mov esi, [rip + num_base_primes]
    xor eax, eax
    call printf@PLT

    # Target: 1,000,000,000
    mov r12, 1000000000         # r12 = target N
    mov r13, 1                  # r13 = running prime count (2 is 1st prime)
    xor r14, r14                # r14 = low_k = 0
    mov dword ptr [rip + active_primes_val], 0

    # Segment size is 65536 bytes (64 KiB, cache-aligned)
    mov r15, 65536              # r15 = SEG_SIZE

.Lsegment_loop:
    # Clear segment buffer: memset(seg_buffer, 0, 65536)
    lea rdi, [rip + seg_buffer]
    xor eax, eax
    mov ecx, 65536 / 8          # 8192 QWORDs
    rep stosq

    # For segment 0 (low_k == 0), mark 1 as non-prime (byte 0 = 1)
    test r14, r14
    jnz .Lsieve_current_segment
    lea rax, [rip + seg_buffer]
    mov byte ptr [rax], 1

.Lsieve_current_segment:
    # Determine newly active primes for this segment
    # high_val = (low_k + seg_size - 1)*2 + 1
    lea rax, [r14 + r15 - 1]
    lea rax, [rax * 2 + 1]      # high_val

    mov r8d, [rip + num_base_primes]
    lea r9, [rip + base_primes_arr]
    lea r10, [rip + base_offsets_arr]
    mov ebx, [rip + active_primes_val] # ebx is callee-saved, safe!

.Lactivate_primes:
    cmp ebx, r8d                # ebx = active_primes
    jae .Lactivate_done
    mov edx, [r9 + rbx*4]       # edx = p
    mov rdx, rdx
    imul rdx, rdx               # rdx = p * p
    cmp rdx, rax
    ja .Lactivate_done          # p*p > high_val, not active yet

    # Prime p is newly active in this segment!
    # offset = (p*p - 1) / 2 - low_k
    dec rdx
    shr rdx, 1                  # (p*p - 1) / 2
    sub rdx, r14                # relative to low_k
    mov [r10 + rbx*4], edx
    inc ebx
    jmp .Lactivate_primes

.Lactivate_done:
    mov [rip + active_primes_val], ebx

.Lapply_sieve:
    # Sieve this segment using sieve_segment_asm
    # Arguments: rdi=seg, rsi=seg_size, rdx=primes, rcx=offsets, r8=active_primes
    lea rdi, [rip + seg_buffer]
    mov rsi, r15                # seg_size
    lea rdx, [rip + base_primes_arr]
    lea rcx, [rip + base_offsets_arr]
    mov r8d, [rip + active_primes_val] # r8 = active_primes
    call sieve_segment_asm

    # Count primes in this segment with AVX-512
    lea rdi, [rip + seg_buffer]
    mov rsi, r15
    call count_primes_avx512
    # rax = count of primes in this segment

    # Check if target rank is reached
    lea rbx, [r13 + rax]
    cmp rbx, r12
    jae .Lfound_target_segment

    # Not reached yet, add count and advance to next segment
    mov r13, rbx
    add r14, r15                # low_k += seg_size
    jmp .Lsegment_loop

.Lfound_target_segment:
    # Target prime is inside this segment!
    # rank_needed = target_N - running_count
    sub r12, r13                # r12 = rank_needed in this segment (1-based)
    lea rdi, [rip + seg_buffer]
    mov rsi, r15
    mov rdx, r12
    mov rcx, r14
    call find_nth_prime_in_seg_asm
    # rax = the 1,000,000,000th prime number!
    mov r12, rax                # save prime in r12

    # Stop timing: clock_gettime(CLOCK_MONOTONIC, &t1)
    mov edi, 1
    lea rsi, [rsp + 16]         # t1 at [rsp + 16]
    call clock_gettime@PLT

    # Compute elapsed seconds as double: (t1.sec - t0.sec) + (t1.nsec - t0.nsec)*1e-9
    mov rax, [rsp + 16]         # t1.tv_sec
    sub rax, [rsp]              # t0.tv_sec
    cvtsi2sd xmm0, rax

    mov rax, [rsp + 24]         # t1.tv_nsec
    sub rax, [rsp + 8]          # t0.tv_nsec
    cvtsi2sd xmm1, rax
    movsd xmm2, [rip + one_billion_double]
    divsd xmm1, xmm2
    addsd xmm0, xmm1            # xmm0 = total elapsed seconds

    # Print results
    lea rdi, [rip + result_str]
    mov rsi, r12                # Prime number
    # xmm0 already holds elapsed seconds
    mov eax, 1                  # 1 floating point argument
    call printf@PLT

    # Print verification
    mov rax, 22801763489
    cmp r12, rax
    jne .Lfail
    lea rdi, [rip + pass_str]
    xor eax, eax
    call printf@PLT
    jmp .Lexit

.Lfail:
    lea rdi, [rip + fail_str]
    xor eax, eax
    call printf@PLT

.Lexit:
    add rsp, 56
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    xor eax, eax
    ret

# ------------------------------------------------------------------------------
# Helper: init_base_primes_asm(uint32_t max_val)
# Computes base primes up to max_val using standard odd-sieve in assembly
# ------------------------------------------------------------------------------
.type init_base_primes_asm, @function
init_base_primes_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    # edi = max_val (160000)
    mov r12d, edi

    # Clear sieve table: base_sieve_buf
    lea rdi, [rip + base_sieve_buf]
    mov ecx, 160000
    mov al, 1
    rep stosb

    # Mark 0 and 1 as composite
    lea rdi, [rip + base_sieve_buf]
    mov byte ptr [rdi], 0
    mov byte ptr [rdi + 1], 0

    # Sieve loop: p = 2; p*p <= max_val; p++
    mov ebx, 2
.Lbase_p_loop:
    mov eax, ebx
    imul eax, eax
    cmp eax, r12d
    ja .Lcollect_primes

    lea rdi, [rip + base_sieve_buf]
    cmp byte ptr [rdi + rbx], 0
    je .Lbase_p_next

    # Mark multiples: i = p*p; i <= max_val; i += p
    mov eax, ebx
    imul eax, eax               # i = p * p
.Lbase_mark_multiples:
    cmp eax, r12d
    ja .Lbase_p_next
    mov byte ptr [rdi + rax], 0
    add eax, ebx
    jmp .Lbase_mark_multiples

.Lbase_p_next:
    inc ebx
    jmp .Lbase_p_loop

.Lcollect_primes:
    # Collect odd primes starting from 3 into base_primes_arr
    lea rdi, [rip + base_sieve_buf]
    lea rsi, [rip + base_primes_arr]
    xor ecx, ecx                # count = 0
    mov edx, 3                  # p = 3
.Lcollect_loop:
    cmp edx, r12d
    ja .Lcollect_done
    cmp byte ptr [rdi + rdx], 0
    je .Lcollect_next
    mov [rsi + rcx*4], edx
    inc ecx
.Lcollect_next:
    add edx, 2
    jmp .Lcollect_loop

.Lcollect_done:
    mov [rip + num_base_primes], ecx

    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

# ==============================================================================
# Read-Only Data Section
# ==============================================================================
.section .rodata
.align 8
banner_str:
    .string "====================================================================\n        Pure x86_64 Assembly Standalone Prime Sieve (Zen 5 AVX-512) \n====================================================================\n"
base_primes_str:
    .string "Base primes initialized in pure assembly: %d primes\nSieving odd numbers in 64 KiB L1/L2 cache segments...\n"
result_str:
    .string "\n--------------------------------------------------------------------\nThe 1,000,000,000th Prime Number is : %lu\nExecution Elapsed Time              : %.3f seconds\n"
pass_str:
    .string "OEIS A006988 Mathematical Match     : [PASS] (Verified: 22801763489)\n====================================================================\n"
fail_str:
    .string "OEIS A006988 Mathematical Match     : [FAIL]\n====================================================================\n"
one_billion_double:
    .double 1000000000.0

# ==============================================================================
# BSS Section (Cache-aligned data)
# ==============================================================================
.section .bss
.align 64
seg_buffer:
    .zero 65536                 # 64 KiB segment buffer (L1/L2 cache sized)
.align 64
base_sieve_buf:
    .zero 160000
.align 64
base_primes_arr:
    .zero 160000 * 4            # Up to 40,000 uint32_t primes
.align 64
base_offsets_arr:
    .zero 160000 * 4            # Up to 40,000 uint32_t offsets
.align 4
num_base_primes:
    .zero 4
.align 4
active_primes_val:
    .zero 4
