/*	$NetBSD: crt0.c,v 1.27 1999/01/22 11:29:17 mycroft Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

#include "common.h"

extern void		start __P((void)) __asm__ ("start");

#if defined(sun) && defined(sparc)
static void		__call __P((void));
#endif

#ifdef __NetBSD__
#undef mmap
#define mmap(addr, len, prot, flags, fd, off)	\
    __syscall2((quad_t)SYS_mmap, (addr), (len), (prot), (flags), \
	(fd), 0, (off_t)(off))
extern int		__syscall2 __P((quad_t, ...));
#endif

__asm__("
	.text
	.align 4
	.global start
start:
	mov	0, %fp
	ld	[%sp + 64], %l0		! get argc
	add	%sp, 68, %l1		! get argv
	sll	%l0, 2,	%l2		!
	add	%l2, 4,	%l2		! envp = argv + (argc << 2) + 4
	add	%l1, %l2, %l2		!
	sethi	%hi(_environ), %l3
	st	%l2, [%l3+%lo(_environ)]	! *environ = l2
");

/*
 * We get a pointer to PSSTRINGS in %g1.
 */
__asm__("
	cmp	%g1, 0
	be	1f
	sethi	%hi(___ps_strings), %l3
	st	%g1, [%l3+%lo(___ps_strings)]
1:
");

/*
 * Finish diddling with stack.
 */
__asm__("
	andn	%sp, 7,	%sp		! align
	sub	%sp, 24, %sp		! expand to standard stack frame size
");

/*
 * Set __progname:
 *	if (argv[0])
 *		if ((__progname = _strrchr(argv[0], '/')) == NULL)
 *			__progname = argv[0];
 *		else
 *			++__progname;
 */
__asm__("
	ld	[%l1], %o0
	cmp	%o0, 0
	mov	%o0, %l6
	be	1f
	 sethi	%hi(___progname), %l7
");
#ifdef DYNAMIC
__asm__("call	__strrchr");
#else
__asm__("call	_strrchr");
#endif
__asm__("
	mov	47, %o1
	cmp	%o0, 0
	be,a	1f
	 st	%l6, [%l7+%lo(___progname)]
	add	%o0, 1, %o0
	st	%o0, [%l7+%lo(___progname)]
1:
");

#ifdef DYNAMIC
/* Resolve symbols in dynamic libraries */
__asm__("
	sethi	%hi(__DYNAMIC), %o0
	orcc	%o0, %lo(__DYNAMIC), %o0
	be	1f
	 nop
	call	___load_rtld
	 nop
1:
");
#endif

/* From here, all symbols should have been resolved, so we can use libc */
#ifdef MCRT0
/*
 * atexit(_mcleanup);
 * monstartup((u_long)&eprol, (u_long)&etext);
 */
__asm__("
	sethi	%hi(__mcleanup), %o0
	call	_atexit
	 or	%o0, %lo(__mcleanup), %o0
	sethi	%hi(_eprol), %o0
	or	%o0, %lo(_eprol), %o0
	sethi	%hi(_etext), %o1
	call	_monstartup
	 or	%o1, %lo(_etext), %o1
");
#endif

#ifdef sun
/*
 * SunOS compatibility
 */
__asm__("
	call	start_float
	 nop
");
#endif

/*
 * Move `argc', `argv', and `envp' from locals to parameters for `main'.
 */
__asm__("
	mov	%l0,%o0
	mov	%l1,%o1
__callmain:
	call	_main
	 mov	%l2,%o2

	call	_exit
	 nop
");

#ifdef DYNAMIC
/*
 * System call entry
 */
__asm__("
	.set	SYSCALL_G2RFLAG, 0x400
	.set	SYS___syscall, 198
___syscall2:
	sethi	%hi(SYS___syscall), %g1
	ba	1f
	 or	%g1, %lo(SYS___syscall), %g1
___syscall:
	clr	%g1
1:
	or	%g1, SYSCALL_G2RFLAG, %g1
	add	%o7, 8, %g2
	ta	%g0
	mov	-0x1, %o0
	jmp	%o7 + 0x8
	 mov	-0x1, %o1
");
#endif

#ifdef sun
static
__call()
{
	/*
	 * adjust the C generated pointer to the crt struct to the
	 * likings of ld.so, which is an offset relative to its %fp
	 */
	__asm__("
		mov	%i0, %o0
		mov	%i1, %o1
		call	%i2
		 sub	%o1, %sp, %o1
	");
	/*NOTREACHED, control is transferred directly to our caller */
}
#endif

#include "common.c"

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.27 1999/01/22 11:29:17 mycroft Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef MCRT0
__asm__ ("
	.text
	_eprol:
");
#endif
