/*	$NetBSD: crt0.c,v 1.4 2001/08/17 00:14:38 bjh21 Exp $	*/

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
#include <sys/param.h>

#include <machine/asm.h>
#include <stdlib.h>

#include "common.h"

extern	void		_start(void);
	void		___start(int, char *[], char *[], struct ps_strings *,
				const Obj_Entry *, void (*)());

__asm("
	.text
	.align	0
	.globl	_start
	.globl	__start
_start:
__start:
	mov	r5, r2		/* cleanup */
	mov	r4, r1		/* obj_main */
	mov	r3, r0		/* ps_strings */
	/* Get argc, argv, and envp from stack */
	ldr	r0, [sp, #0x0000]
	add	r1, sp, #0x0004
	add	r2, r1, r0, lsl #2
	add	r2, r2, #0x0004

	/* Ensure the stack is properly aligned before calling C code. */
	bic	sp, sp, #" ___STRING(STACKALIGNBYTES) "
	sub	sp, sp, #8
	str	r5, [sp, #4]
	str	r4, [sp, #0]

	b	" ___STRING(_C_LABEL(___start)) "
");

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.4 2001/08/17 00:14:38 bjh21 Exp $");
#endif /* LIBC_SCCS and not lint */

void
___start(int argc, char **argv, char **envp, struct ps_strings *ps_strings,
	const Obj_Entry *obj, void (*cleanup)(void))
{
	char *ap;

 	environ = envp;
	__ps_strings = ps_strings;

	if ((ap = argv[0]))
		if ((__progname = _strrchr(ap, '/')) == NULL)
			__progname = ap;
		else
			++__progname;

#ifdef	DYNAMIC
	/* ld(1) convention: if DYNAMIC = 0 then statically linked */
#ifdef	stupid_gcc
	if (&_DYNAMIC)
#else
	if ( ({volatile caddr_t x = (caddr_t)&_DYNAMIC; x; }) )
#endif
                _rtld_setup(cleanup, obj);
#endif	/* DYNAMIC */

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&_eprol, (u_long)&_etext);
#endif	/* MCRT0 */

	atexit(_fini);
	_init();

__asm("__callmain:");		/* Defined for the benefit of debuggers */
	exit(main(argc, argv, envp));
}

#include "common.c"

