/*	$NetBSD: crt0.c,v 1.13 2002/02/24 01:06:18 matt Exp $	*/

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

extern void	start __P((void)) asm("start");
struct kframe {
	int	kargc;
	char	*kargv[1];	/* size depends on kargc */
	char	kargstr[1];	/* size varies */
	char	kenvstr[1];	/* size varies */
};

	asm("	.text");
	asm("	.align 2");
	asm("	.globl start");
	asm("	.type start,@function");
	asm("	start:");
	asm("		.word 0x0101");		/* two nops just in case */
	asm("		pushl %sp");		/* no registers to save */
	asm("		calls $1,___start");	/* do the real start */
	asm("		halt");

void
__start(kfp)
	struct kframe *kfp;
{
	/*
	 *	ALL REGISTER VARIABLES!!!
	 */
	char **argv, *ap;

	argv = &kfp->kargv[0];
	environ = argv + kfp->kargc + 1;

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

asm("eprol:");

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&eprol, (u_long)&etext);
#endif MCRT0

asm ("__callmain:");		/* Defined for the benefit of debuggers */
	exit(main(kfp->kargc, argv, environ));
}

#ifdef DYNAMIC
	asm("	___syscall:");
	asm("		.word 0");		/* no registers to save */
	asm("		addl2 $4,%ap");		/* n-1 args to syscall */
	asm("		movl (%ap),r0");	/* get syscall number */
	asm("		subl3 $1,-4(%ap),(%ap)"); /* n-1 args to syscall */
	asm("		chmk %r0");		/* do system call */
	asm("		jcc 1f");		/* check error */
	asm("		mnegl $1,%r0");
	asm("		ret");
	asm("	1:	movpsl -(%sp)");	/* flush the icache */
	asm("		pushab 2f");		/* by issuing an REI */
	asm("		rei");
	asm("	2:	ret");

#endif /* DYNAMIC */

#include "common.c"

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.13 2002/02/24 01:06:18 matt Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef MCRT0
asm ("	.text");
asm ("_eprol:");
#endif
