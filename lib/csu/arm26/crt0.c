/*	$NetBSD: crt0.c,v 1.1 2000/05/09 21:55:45 bjh21 Exp $	*/

/*
 * Copyright (C) 1997 Mark Brinicombe
 * Copyright (C) 1995 Wolfgang Solfrank.
 * Copyright (C) 1995 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#undef DYNAMIC /* XXX Too complicated for me! */

#include <machine/asm.h>
#include <stdlib.h>

#include "common.h"

extern	void		_start(void);
	void		__start(int, char *[], char *[]);

__asm("
	.text
	.align	0
	.global	_start
_start:
	/* Get ps_strings pointer from kernel */
	teq	r10, #0
	ldr	r3, Lps_strings
	movne	r0, #0
	str	r0, [r3]

	/* Get argc, argv, and envp from stack */
	ldr	r0, [sp, #0x0000]
	add	r1, sp, #0x0004
	add	r2, r1, r0, lsl #2
	add	r2, r2, #0x0004

	b	" ___STRING(_C_FUNC(__start)) "
	.align	0
Lps_strings:
	.word	" ___STRING(_C_LABEL(__ps_strings)) "
");

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.1 2000/05/09 21:55:45 bjh21 Exp $");
#endif /* LIBC_SCCS and not lint */

void
__start(int argc, char **argv, char **envp)
{
	char *ap;

 	environ = envp;

	if ((ap = argv[0]))
		if ((__progname = _strrchr(ap, '/')) == NULL)
			__progname = ap;
		else
			++__progname;

#ifdef	DYNAMIC
	/* ld(1) convention: if DYNAMIC = 0 then staticly linked */
#ifdef	stupid_gcc
	if (&_DYNAMIC)
#else
	if ( ({volatile caddr_t x = (caddr_t)&_DYNAMIC; x; }) )
#endif
		_rtld_setup(&_DYNAMIC);
#endif	/* DYNAMIC */

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&_eprol, (u_long)&_etext);
#endif	/* MCRT0 */

__asm("__callmain:");		/* Defined for the benefit of debuggers */
	exit(main(argc, argv, envp));
}

#include "common.c"

