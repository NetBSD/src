/* $NetBSD: crt0.c,v 1.10 2000/06/14 06:48:59 cgd Exp $ */

/*
 * Copyright (c) 1998 Christos Zoulas
 * Copyright (c) 1995 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include "common.h"

void ___start __P((int, char **, char **, void (*cleanup) __P((void)),
    const Obj_Entry *, struct ps_strings *));

__asm("
	.text
	.align	4
	.globl	__start
	.globl	_start
_start:
__start:
	pushl	%ebx			# ps_strings
	pushl	%ecx			# obj
	pushl	%edx			# cleanup
	movl	12(%esp),%eax
	leal	20(%esp,%eax,4),%ecx
	leal	16(%esp),%edx
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	call	___start
");

void
___start(argc, argv, envp, cleanup, obj, ps_strings)
	int argc;
	char **argv;
	char **envp;
	void (*cleanup) __P((void));		/* from shared loader */
	const Obj_Entry *obj;			/* from shared loader */
	struct ps_strings *ps_strings;
{
	environ = envp;

	if ((__progname = argv[0]) != NULL) {	/* NULL ptr if argc = 0 */
		if ((__progname = _strrchr(__progname, '/')) == NULL)
			__progname = argv[0];
		else
			__progname++;
	}

	if (ps_strings != (struct ps_strings *)0)
		__ps_strings = ps_strings;

#ifdef DYNAMIC
	if (&_DYNAMIC != NULL)
		_rtld_setup(cleanup, obj);
#endif

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&_eprol, (u_long)&_etext);
#endif

	atexit(_fini);
	_init();

	exit(main(argc, argv, environ));
}

/*
 * NOTE: Leave the RCS ID _after_ __start(), in case it gets placed in .text.
 */
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.10 2000/06/14 06:48:59 cgd Exp $");
#endif /* LIBC_SCCS and not lint */

#include "common.c"
