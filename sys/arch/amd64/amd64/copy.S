/*	$NetBSD: copy.S,v 1.16 2009/11/27 03:23:04 rmind Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/asm.h>
#include <machine/frameasm.h>

#define GET_CURPCB(reg)	\
	movq	CPUVAR(CURLWP), reg; \
	movq	L_PCB(reg), reg

/*
 * These are arranged so that the abnormal case is a forwards
 * conditional branch - which will be predicted not-taken by
 * both Intel and AMD processors.
 */
#define DEFERRED_SWITCH_CHECK \
	CHECK_DEFERRED_SWITCH			; \
	jnz	99f				; \
    98:

#define DEFERRED_SWITCH_CALL \
    99:						; \
	call	_C_LABEL(do_pmap_load)		; \
	jmp	98b

/*
 * The following primitives are to copy regions of memory.
 * Label must be before all copy functions.
 */
	.text

x86_copyfunc_start:	.globl	x86_copyfunc_start

/*
 * Handle deferred pmap switch.  We must re-enable preemption without
 * making a function call, so that the program counter is visible to
 * cpu_kpreempt_exit().  It can then know if it needs to restore the
 * pmap on returning, because a preemption occurred within one of the
 * copy functions.
 */
ENTRY(do_pmap_load)
	pushq	%rbp
	movq	%rsp,%rbp
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbx
	movq	CPUVAR(CURLWP), %rbx
1:
	incl	L_NOPREEMPT(%rbx)
	call	_C_LABEL(pmap_load)
	decl	L_NOPREEMPT(%rbx)
	jnz	2f
	cmpl	$0, L_DOPREEMPT(%rbx)
	jz	2f
	xorq	%rdi, %rdi
	call	_C_LABEL(kpreempt)
2:
	cmpl	$0, CPUVAR(WANT_PMAPLOAD)
	jnz	1b
	popq	%rbx
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	leaveq
	ret

/*
 * int kcopy(const void *from, void *to, size_t len);
 * Copy len bytes, abort on fault.
 *
 * Copy routines from and to userland, plus a few more. See the
 * section 9 manpages for info. Some cases can be optimized more.
 *
 * I wonder if it's worthwhile to make these use SSE2 registers?
 * (dsl) Not from info I've read from the AMD guides.
 *
 * Also note that the setup time for 'rep movs' is horrid - especially on P4
 * netburst - but on my AMD X2 it manages one copy (read+write) per clock
 * which can be achieved with a code loop, but is probably impossible to beat.
 * Howver the use of 'rep movsb' for the final bytes should be killed.
 *
 * Newer Intel cpus have a much lower setup time, and may (someday)
 * be ably to do cache-line size copies....
 */

ENTRY(kcopy)
	xchgq	%rdi,%rsi
	movq	%rdx,%rcx
.Lkcopy_start:
	movq	%rdi,%rax
	subq	%rsi,%rax
	cmpq	%rcx,%rax		# overlapping?
	jb	1f
	# nope, copy forward
	shrq	$3,%rcx			# copy by 64-bit words
	rep
	movsq

	movq	%rdx,%rcx
	andl	$7,%ecx			# any bytes left?
	rep
	movsb

	xorq	%rax,%rax
	ret

# Using 'rep movs' to copy backwards is not as fast as for forwards copies
# and ought not be done when the copy doesn't acually overlap.
# However kcopy() isn't used any that looks even vaguely used often.
# I'm also not sure it is ever asked to do overlapping copies!

1:	addq	%rcx,%rdi		# copy backward
	addq	%rcx,%rsi
	std
	andq	$7,%rcx			# any fractional bytes?
	decq	%rdi
	decq	%rsi
	rep
	movsb
	movq	%rdx,%rcx		# copy remainder by 64-bit words
	shrq	$3,%rcx
	subq	$7,%rsi
	subq	$7,%rdi
	rep
	movsq
	cld
.Lkcopy_end:
	xorq	%rax,%rax
	ret

ENTRY(copyout)
	DEFERRED_SWITCH_CHECK

	xchgq	%rdi,%rsi		# kernel address to %rsi, user to %rdi
	movq	%rdx,%rax		# save transfer length (bytes)

	addq	%rdi,%rdx		# end address to %rdx
	jc	_C_LABEL(copy_efault)	# jump if wraps
	movq	$VM_MAXUSER_ADDRESS,%r8
	cmpq	%r8,%rdx
	ja	_C_LABEL(copy_efault)	# jump if end in kernel space

.Lcopyout_start:
	movq	%rax,%rcx		# length
	shrq	$3,%rcx			# count of 8-byte words
	rep
	movsq				# copy from %rsi to %rdi
	movb	%al,%cl
	andb	$7,%cl			# remaining number of bytes
	rep
	movsb				# copy remaining bytes
.Lcopyout_end:
	xorl	%eax,%eax
	ret
	DEFERRED_SWITCH_CALL

ENTRY(copyin)
	DEFERRED_SWITCH_CHECK

	xchgq	%rdi,%rsi
	movq	%rdx,%rax

	addq	%rsi,%rdx		# Check source address not wrapped
	jc	_C_LABEL(copy_efault)
	movq	$VM_MAXUSER_ADDRESS,%r8	
	cmpq	%r8,%rdx
	ja	_C_LABEL(copy_efault)	# j if end in kernel space

.Lcopyin_start:
3:	/* bcopy(%rsi, %rdi, %rax); */
	movq	%rax,%rcx
	shrq	$3,%rcx
	rep
	movsq
	movb	%al,%cl
	andb	$7,%cl
	rep
	movsb
.Lcopyin_end:
	xorl	%eax,%eax
	ret
	DEFERRED_SWITCH_CALL

NENTRY(copy_efault)
	movq	$EFAULT,%rax

/*
 * kcopy_fault is used by kcopy and copy_fault is used by copyin/out.
 *
 * they're distinguished for lazy pmap switching.  see trap().
 */

NENTRY(kcopy_fault)
	ret

NENTRY(copy_fault)
	ret

ENTRY(copyoutstr)
	DEFERRED_SWITCH_CHECK
	xchgq	%rdi,%rsi
	movq	%rdx,%r8
	movq	%rcx,%r9

	/*
	 * Get min(%rdx, VM_MAXUSER_ADDRESS-%rdi).
	 */
	movq	$VM_MAXUSER_ADDRESS,%rax
	subq	%rdi,%rax
	jc	_C_LABEL(copystr_efault)
	cmpq	%rdx,%rax
	jae	1f
	movq	%rax,%rdx
	movq	%rax,%r8
.Lcopyoutstr_start:
1:	incq	%rdx

1:	decq	%rdx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b
.Lcopyoutstr_end:
	/* Success -- 0 byte reached. */
	decq	%rdx
	xorq	%rax,%rax
	jmp	copystr_return

2:	/* rdx is zero -- return EFAULT or ENAMETOOLONG. */
	movq	$VM_MAXUSER_ADDRESS,%r11
	cmpq	%r11,%rdi
	jae	_C_LABEL(copystr_efault)
	movq	$ENAMETOOLONG,%rax
	jmp	copystr_return
	DEFERRED_SWITCH_CALL

ENTRY(copyinstr)
	DEFERRED_SWITCH_CHECK
	xchgq	%rdi,%rsi
	movq	%rdx,%r8
	movq	%rcx,%r9

	/*
	 * Get min(%rdx, VM_MAXUSER_ADDRESS-%rsi).
	 */
	movq	$VM_MAXUSER_ADDRESS,%rax
	subq	%rsi,%rax
	jc	_C_LABEL(copystr_efault)
	cmpq	%rdx,%rax
	jae	1f
	movq	%rax,%rdx
	movq	%rax,%r8
.Lcopyinstr_start:
1:	incq	%rdx

1:	decq	%rdx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b
.Lcopyinstr_end:

	/* Success -- 0 byte reached. */
	decq	%rdx
	xorq	%rax,%rax
	jmp	copystr_return

2:	/* edx is zero -- return EFAULT or ENAMETOOLONG. */
	movq	$VM_MAXUSER_ADDRESS,%r11
	cmpq	%r11,%rsi
	jae	_C_LABEL(copystr_efault)
	movq	$ENAMETOOLONG,%rax
	jmp	copystr_return
	DEFERRED_SWITCH_CALL

ENTRY(copystr_efault)
	movl	$EFAULT,%eax

ENTRY(copystr_fault)
copystr_return:
	/* Set *lencopied and return %eax. */
	testq	%r9,%r9
	jz	8f
	subq	%rdx,%r8
	movq	%r8,(%r9)

8:	ret

ENTRY(copystr)
	xchgq	%rdi,%rsi
	movq	%rdx,%r8

	incq	%rdx

1:	decq	%rdx
	jz	4f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

	/* Success -- 0 byte reached. */
	decq	%rdx
	xorl	%eax,%eax
	jmp	6f

4:	/* edx is zero -- return ENAMETOOLONG. */
	movl	$ENAMETOOLONG,%eax

6:	/* Set *lencopied and return %eax. */
	testq	%rcx,%rcx
	jz	7f
	subq	%rdx,%r8
	movq	%r8,(%rcx)

7:	ret

ENTRY(fuword)
	DEFERRED_SWITCH_CHECK
	movq	$VM_MAXUSER_ADDRESS-4,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusufailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)
	movl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret
	DEFERRED_SWITCH_CALL
	
ENTRY(fusword)
	DEFERRED_SWITCH_CHECK
	movq	$VM_MAXUSER_ADDRESS-2,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusufailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)
	movzwl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret
	DEFERRED_SWITCH_CALL
	
ENTRY(fuswintr)
	cmpl	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
	jnz	_C_LABEL(fusuaddrfault)
	movq	$VM_MAXUSER_ADDRESS-2,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusuintrfailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)
	movzwl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret
	
ENTRY(fubyte)
	DEFERRED_SWITCH_CHECK
	movq	$VM_MAXUSER_ADDRESS-1,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusufailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)
	movzbl	(%rdi),%eax
	movq	$0,PCB_ONFAULT(%rcx)
	ret
	DEFERRED_SWITCH_CALL

ENTRY(suword)
	DEFERRED_SWITCH_CHECK
	movq	$VM_MAXUSER_ADDRESS-4,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)

	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusufailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)

	movq	%rsi,(%rdi)
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%rcx)
	ret
	DEFERRED_SWITCH_CALL
	
ENTRY(susword)
	DEFERRED_SWITCH_CHECK
	movq	$VM_MAXUSER_ADDRESS-2,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)

	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusufailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)

	movw	%si,(%rdi)
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%rcx)
	ret
	DEFERRED_SWITCH_CALL

ENTRY(suswintr)
	cmpl	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
	jnz	_C_LABEL(fusuaddrfault)
	movq	$VM_MAXUSER_ADDRESS-2,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)
	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusuintrfailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)
	movw	%si,(%rdi)
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%rcx)
	ret

ENTRY(subyte)
	DEFERRED_SWITCH_CHECK
	movq	$VM_MAXUSER_ADDRESS-1,%r11
	cmpq	%r11,%rdi
	ja	_C_LABEL(fusuaddrfault)

	GET_CURPCB(%rcx)
	leaq	_C_LABEL(fusufailure)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)

	movb	%sil,(%rdi)
	xorq	%rax,%rax
	movq	%rax,PCB_ONFAULT(%rcx)
	ret
	DEFERRED_SWITCH_CALL

/*
 * These are the same, but must reside at different addresses,
 * because trap.c checks for them.
 */
ENTRY(fusuintrfailure)
	movq	$0,PCB_ONFAULT(%rcx)
	movl	$-1,%eax
	ret

ENTRY(fusufailure)
	movq	$0,PCB_ONFAULT(%rcx)
	movl	$-1,%eax
	ret

ENTRY(fusuaddrfault)
	movl	$-1,%eax
	ret

/*
 * Compare-and-swap the 64-bit integer in the user-space.
 *
 * int	ucas_64(volatile int64_t *uptr, int64_t old, int64_t new, int64_t *ret);
 */
ENTRY(ucas_64)
	DEFERRED_SWITCH_CHECK
	/* Fail if kernel-space */
	movq	$VM_MAXUSER_ADDRESS-8, %r8
	cmpq	%r8, %rdi
	ja	1f
	/* Label for fault handler */
.Lucas64_start:
	/* Perform the CAS */
	lock
	cmpxchgq %rdx, (%rdi)
	/* Clear the fault handler */
	movq	%rax, PCB_ONFAULT(%r8)
.Lucas64_end:
	/*
	 * Note: %rax is "old" value.
	 * Set the return values.
	 */
	movq	%rax, (%rcx)
	xorq	%rax, %rax
	/* Clear the fault handler */
	movq	%rax, PCB_ONFAULT(%r8)
1:
	/* Failure case */
	movq	$EFAULT, %rax
	ret
	DEFERRED_SWITCH_CALL

/*
 * int	ucas_32(volatile int32_t *uptr, int32_t old, int32_t new, int32_t *ret);
 */
ENTRY(ucas_32)
	DEFERRED_SWITCH_CHECK
	/* Fail if kernel-space */
	movq	$VM_MAXUSER_ADDRESS-4, %r8
	cmpq	%r8, %rdi
	ja	1f
	/* Label for fault handler */
.Lucas32_start:
	/* Perform the CAS */
	lock
	cmpxchgl %edx, (%rdi)
.Lucas32_end:
	/*
	 * Note: %eax is "old" value.
	 * Set the return values.
	 */
	movl	%eax, (%rcx)
	xorq	%rax, %rax
1:
	/* Failure case */
	movq	$EFAULT, %rax
	ret
	DEFERRED_SWITCH_CALL

/*
 * Fault handler for ucas_32() and ucas_64().
 * Unset the handler and return the failure.
 */
NENTRY(ucas_fault)
	movq	$EFAULT, %rax
	ret

/*
 * int	ucas_ptr(volatile void *uptr, void *old, void *new, void *ret);
 */
STRONG_ALIAS(ucas_ptr, ucas_64)
STRONG_ALIAS(ucas_int, ucas_32)

/*
 * Label must be after all copy functions.
 */
x86_copyfunc_end:	.globl	x86_copyfunc_end

/*
 * Fault table of copy functions for trap().
 */
	.section ".rodata"
	.globl _C_LABEL(onfault_table)
_C_LABEL(onfault_table):
	.quad .Lcopyin_start
	.quad .Lcopyin_end
	.quad _C_LABEL(copy_fault)

	.quad .Lcopyout_start
	.quad .Lcopyout_end
	.quad _C_LABEL(copy_fault)

	.quad .Lkcopy_start
	.quad .Lkcopy_end
	.quad _C_LABEL(kcopy_fault)

	.quad .Lcopyoutstr_start
	.quad .Lcopyoutstr_end
	.quad _C_LABEL(copystr_fault)

	.quad .Lcopyinstr_start
	.quad .Lcopyinstr_end
	.quad _C_LABEL(copystr_fault)

	.quad .Lucas64_start
	.quad .Lucas64_end
	.quad _C_LABEL(ucas_fault)

	.quad .Lucas32_start
	.quad .Lucas32_end
	.quad _C_LABEL(ucas_fault)

	.quad 0	/* terminate */

	.text
