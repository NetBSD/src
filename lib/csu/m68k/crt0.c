/*	$NetBSD: crt0.c,v 1.21 1999/01/22 11:29:16 mycroft Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and Paul Kranenburg.
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

extern	void		start __P((void)) asm("start");
	void		__start __P((int, char *[], char *[]));

__asm("
	.text
	.align	2
	.globl	start
start:
	movel	a2,___ps_strings
	movel	sp@,d0
	lea	sp@(8,d0:l:4),a0
	lea	sp@(4),a1
	movel	a0,sp@-
	movel	a1,sp@-
	movel	d0,sp@-
	jsr	___start
");

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.21 1999/01/22 11:29:16 mycroft Exp $");
#endif /* LIBC_SCCS and not lint */

void
__start(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	char *ap;

	environ = envp;

	if (ap = argv[0])
		if ((__progname = _strrchr(ap, '/')) == NULL)
			__progname = ap;
		else
			++__progname;

#ifdef DYNAMIC
	/* ld(1) convention: if DYNAMIC = 0 then statically linked */
#ifdef stupid_gcc
	if (&_DYNAMIC)
#else
	if ( ({volatile caddr_t x = (caddr_t)&_DYNAMIC; x; }) )
#endif
		__load_rtld(&_DYNAMIC);
#endif /* DYNAMIC */

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&eprol, (u_long)&etext);
#endif /* MCRT0 */

__asm("__callmain:");		/* Defined for the benefit of debuggers */
	exit(main(argc, argv, envp));
}

#ifdef DYNAMIC
__asm("
	.text
	.align	2
___syscall:
	movel	a7@+,a0
	movel	a7@,d0
	movel	a0,a7@
	trap	#0
	bcs	1f
	jmp	a0@
1:
	moveq	#-1,d0
	jmp	a0@
");
#endif /* DYNAMIC */

#include "common.c"

#ifdef MCRT0
__asm("
	.text
	.align	2
eprol:
");
#endif /* MCRT0 */
