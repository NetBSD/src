/*	$NetBSD: copy.S,v 1.9 2008/02/06 22:27:09 dsl Exp $	*/

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
	movq	L_ADDR(reg), reg

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

ENTRY(do_pmap_load)
#if 1
	pushq	%rbp
	movq	%rsp,%rbp
#endif
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	call	_C_LABEL(pmap_load)
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
#if 1
	leaveq
#endif
	ret

/*
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

/*
 * XXXfvdl appears only to be used by pccons.
 *
 * fillw(short pattern, void *addr, size_t len);
 * Write len copies of pattern at addr.
 * appears to be used by pccons.
 */
ENTRY(fillw)
	movl	%edi,%eax
	movq	%rsi,%rdi
	movw	%ax,%cx
	rorl	$16,%eax
	movw	%cx,%ax
	movq	%rdx,%rcx
	shrq	%rcx
	rep
	stosl
	movq	%rdx,%rcx
	andq	$1,%rcx
	rep
	stosw
	ret

ENTRY(kcopy)
	GET_CURPCB(%rax)
	pushq	PCB_ONFAULT(%rax)
	leaq	_C_LABEL(kcopy_fault)(%rip),%r11
	movq	%r11, PCB_ONFAULT(%rax)

	xchgq	%rdi,%rsi
	movq	%rdx,%rcx

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

	GET_CURPCB(%rdx)
	popq	PCB_ONFAULT(%rdx)
	xorq	%rax,%rax
	ret

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

	GET_CURPCB(%rdx)
	popq	PCB_ONFAULT(%rdx)
	xorq	%rax,%rax
	ret

ENTRY(copyout)
	DEFERRED_SWITCH_CHECK
	pushq	$0

	xchgq	%rdi,%rsi
	movq	%rdx,%rax
	
	movq	%rdi,%rdx
	addq	%rax,%rdx
	jc	_C_LABEL(copy_efault)
	movq	$VM_MAXUSER_ADDRESS,%r8
	cmpq	%r8,%rdx
	ja	_C_LABEL(copy_efault)

	GET_CURPCB(%rdx)
	leaq	_C_LABEL(copy_fault)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rdx)

	movq	%rax,%rcx
	shrq	$3,%rcx
	rep
	movsq
	movb	%al,%cl
	andb	$7,%cl
	rep
	movsb

	popq	PCB_ONFAULT(%rdx)
	xorl	%eax,%eax
	ret
	DEFERRED_SWITCH_CALL

ENTRY(copyin)
	DEFERRED_SWITCH_CHECK
	GET_CURPCB(%rax)
	pushq	$0
	leaq	_C_LABEL(copy_fault)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rax)

	xchgq	%rdi,%rsi
	movq	%rdx,%rax

	movq	%rsi,%rdx
	addq	%rax,%rdx
	jc	_C_LABEL(copy_efault)
	movq	$VM_MAXUSER_ADDRESS,%r8
	cmpq	%r8,%rdx
	ja	_C_LABEL(copy_efault)

3:	/* bcopy(%rsi, %rdi, %rax); */
	movq	%rax,%rcx
	shrq	$3,%rcx
	rep
	movsq
	movb	%al,%cl
	andb	$7,%cl
	rep
	movsb

	GET_CURPCB(%rdx)
	popq	PCB_ONFAULT(%rdx)
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
	GET_CURPCB(%rdx)
	popq	PCB_ONFAULT(%rdx)
	ret

NENTRY(copy_fault)
	GET_CURPCB(%rdx)
	popq	PCB_ONFAULT(%rdx)
	ret

ENTRY(copyoutstr)
	DO_DEFERRED_SWITCH
	xchgq	%rdi,%rsi
	movq	%rdx,%r8
	movq	%rcx,%r9

5:	GET_CURPCB(%rax)
	leaq	_C_LABEL(copystr_fault)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rax)
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

1:	incq	%rdx

1:	decq	%rdx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

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

ENTRY(copyinstr)
	DO_DEFERRED_SWITCH
	xchgq	%rdi,%rsi
	movq	%rdx,%r8
	movq	%rcx,%r9

	GET_CURPCB(%rcx)
	leaq	_C_LABEL(copystr_fault)(%rip),%r11
	movq	%r11,PCB_ONFAULT(%rcx)

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

1:	incq	%rdx

1:	decq	%rdx
	jz	2f
	lodsb
	stosb
	testb	%al,%al
	jnz	1b

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

ENTRY(copystr_efault)
	movl	$EFAULT,%eax

ENTRY(copystr_fault)
copystr_return:
	/* Set *lencopied and return %eax. */
	GET_CURPCB(%rcx)
	movq	$0,PCB_ONFAULT(%rcx)
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
	cmpq	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
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
	leaq	_C_LABEL(fusuintrfailure)(%rip),%r11
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
	cmpq	$TLBSTATE_VALID, CPUVAR(TLBSTATE)
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
