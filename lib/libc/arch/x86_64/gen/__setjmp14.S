/*	$NetBSD: __setjmp14.S,v 1.3.34.1 2024/08/23 16:56:30 martin Exp $	*/

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


#include <machine/asm.h>
#include <machine/setjmp.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: __setjmp14.S,v 1.3.34.1 2024/08/23 16:56:30 martin Exp $")
#endif

/*
 * C library -- _setjmp, _longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from the last call to
 *	setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is restored.
 */

/*
 * setjmp(jmp_buf env@rdi)
 *
 * ELF symbol: __setjmp14, because the size of jmp_buf changed on some
 * platforms in 1.4.
 */
ENTRY(__setjmp14)
	/*
	 * Save the callee-saves registers: %rbx, %rbp, %r12-r15,
	 * plus %rsp and the return address on the stack since it
	 * will be overwritten if the caller makes any subroutine
	 * calls before longjmp.
	 */
	movq	(%rsp),%r11
	movq	%rbx,(_JB_RBX * 8)(%rdi)
	movq	%rbp,(_JB_RBP * 8)(%rdi)
	movq	%r12,(_JB_R12 * 8)(%rdi)
	movq	%r13,(_JB_R13 * 8)(%rdi)
	movq	%r14,(_JB_R14 * 8)(%rdi)
	movq	%r15,(_JB_R15 * 8)(%rdi)
	movq	%rsp,(_JB_RSP * 8)(%rdi)
	movq	%r11,(_JB_PC  * 8)(%rdi)

	leaq	(_JB_SIGMASK * 8)(%rdi),%rdx	/* oset@rdx */
	xorl	%edi,%edi		/* how@edi := 0 (ignored) */
	xorl	%esi,%esi		/* set@rsi := NULL */

#ifdef __PIC__
	call	PIC_PLT(_C_LABEL(__sigprocmask14))
#else
	call	_C_LABEL(__sigprocmask14)
#endif
	xorl	%eax,%eax
	ret
END(__setjmp14)

/*
 * longjmp(jmp_buf env@rdi, int val@esi)
 *
 * ELF symbol: __longjmp14, because the size of jmp_buf changed on some
 * platforms in 1.4
 */
ENTRY(__longjmp14)
	/*
	 * Restore the callee-saves registers: %rbx, %rbp, %r12-r15,
	 * plus %rsp and the return address on the stack.
	 */
	movq	(_JB_RBX * 8)(%rdi),%rbx
	movq	(_JB_RBP * 8)(%rdi),%rbp
	movq	(_JB_R12 * 8)(%rdi),%r12
	movq	(_JB_R13 * 8)(%rdi),%r13
	movq	(_JB_R14 * 8)(%rdi),%r14
	movq	(_JB_R15 * 8)(%rdi),%r15
	movq	(_JB_RSP * 8)(%rdi),%rsp
	movq	(_JB_PC  * 8)(%rdi),%r11
	movq	%r11,0(%rsp)

	/*
	 * Use  pushq %rsi  instead of  pushl %esi  in order to keep
	 * 16-byte stack alignment, even though we only care about the
	 * 32-bit int in esi.
	 */
	pushq	%rsi		/* save val@esi */

	leaq	(_JB_SIGMASK * 8)(%rdi),%rsi	/* set@rsi */
	movl	$3,%edi		/* how@edi := SIG_SETMASK */
	xorl	%edx,%edx	/* oset@rdx := NULL */

#ifdef __PIC__
	call	PIC_PLT(_C_LABEL(__sigprocmask14))
#else
	call	_C_LABEL(__sigprocmask14)
#endif

	popq	%rax		/* restore val@eax */

	testl	%eax,%eax	/* val@eax == 0? */
	jz	1f		/* jump if val@eax == 0 */
	ret			/* return val@eax */
1:	incl	%eax		/* val@eax := 1 */
	ret			/* return val@eax */
END(__longjmp14)
