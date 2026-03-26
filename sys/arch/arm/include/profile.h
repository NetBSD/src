/*	$NetBSD: profile.h,v 1.21 2026/03/26 18:06:59 skrll Exp $	*/

/*
 * Copyright (c) 2001 Ben Harris
 * Copyright (c) 1995-1996 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define	_MCOUNT_DECL void _mcount

/*
 * Cannot implement mcount in C as GCC will trash the ip register when it
 * pushes a trapframe. Pity we cannot insert assembly before the function
 * prologue.
 */

#define	PLTSYM

#if defined (_ARM_ARCH_4T)
# define RET		"bx	ip"
#else
# define RET		"mov	pc, ip"
#endif

#if defined(__ARM_DWARF_EH__)
#define _PROF_UNWINDER_SAVE	""
#define _PROF_UNWINDER_START	""
#define _PROF_UNWINDER_END	""
#else
#define _PROF_UNWINDER_SAVE	".save {r0-r3, lr}\n"
#define _PROF_UNWINDER_START	".fnstart\n"
#define _PROF_UNWINDER_END	".fnend\n"
#endif

#define MCOUNT_ASM_NAME "__gnu_mcount_nc"
#define	MCOUNT								\
	__asm(".text");							\
	__asm(".align	0");						\
	__asm(".arm");							\
	__asm(".type	" MCOUNT_ASM_NAME ",%function");		\
	__asm(".global	" MCOUNT_ASM_NAME);				\
	__asm(MCOUNT_ASM_NAME ":");					\
	__asm(_PROF_UNWINDER_START);					\
	__asm(".cfi_startproc");					\
	/*								\
	 * Preserve registers that are trashed during mcount		\
	 */								\
	__asm("push	{r0-r3, lr}");					\
	__asm(_PROF_UNWINDER_SAVE);					\
	__asm(".cfi_def_cfa_offset 20");				\
	__asm(".cfi_offset 14, -4");					\
	__asm(".cfi_offset 3, -8");					\
	__asm(".cfi_offset 2, -12");					\
	__asm(".cfi_offset 1, -16");					\
	__asm(".cfi_offset 0, -20");					\
	/*								\
	 * find the return address for mcount,				\
	 * and the return address for mcount's caller.			\
	 *								\
	 * frompcindex = pc pushed by call into self.			\
	 */								\
	__asm("ldr	r0, [sp, #20]");				\
	/*								\
	 * selfpc = pc pushed by mcount call				\
	 */								\
	__asm("mov	r1, lr");					\
	/*								\
	 * Call the real mcount code					\
	 */								\
	__asm("bl	" ___STRING(_C_LABEL(_mcount)) PLTSYM);		\
	/*								\
	 * Restore registers that were trashed during mcount		\
	 */								\
	__asm("pop	{r0-r3, ip, lr}");				\
	__asm(RET);							\
	__asm(".cfi_endproc");						\
	__asm(".size	" MCOUNT_ASM_NAME ", .-" MCOUNT_ASM_NAME);

#ifdef _KERNEL
#include <arm/cpufunc.h>
/*
 * splhigh() and splx() are heavyweight, and call mcount().  Therefore
 * we disabled interrupts (IRQ, but not FIQ) directly on the CPU.
 *
 * We're lucky that the CPSR and 's' both happen to be 'int's.
 */
#define	MCOUNT_ENTER	s = __set_cpsr_c(0x0080, 0x0080);	/* kill IRQ */
#define	MCOUNT_EXIT	__set_cpsr_c(0xffffffff, s);	/* restore old value */
#endif /* _KERNEL */
