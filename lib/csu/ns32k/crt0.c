/*	$NetBSD: crt0.c,v 1.16 2002/02/22 13:46:30 matthias Exp $	*/

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

/*
 * Stack layout provided to start:
 *
 * environ[n]
 * ...
 * environ[0]
 * argv[argc-1]
 * ...
 * argv[1]
 * argv[0]
 * argc
 * saved FP
 *
 * The address of PS_STRINGS is passed in r7 by the kernel.
 */

register struct ps_strings *kps_strings __asm("r7");

#include <stdlib.h>

#include "common.h"

extern void	start __P((char *)) __asm("start");

void
start(arg0)
	char *arg0;
{
	int argc;
	char **argv;
	char *ap;

	argv = &arg0;
	argc = ((int *)argv)[-1];
	environ = argv + argc + 1;
	__ps_strings = kps_strings;

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
#endif MCRT0

__asm("__callmain:");		/* Defined for the benefit of debuggers */
	exit(main(argc, argv, environ));
}

/*
 * RCSid. Place after __start for programs that assume start of text
 *  is the entrypoint. (Not really necessary, just to avoid confusion).
 */
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.16 2002/02/22 13:46:30 matthias Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef DYNAMIC
__asm("
	.text
	.align	2
___syscall:
	movd tos,r1		/* return address */
	movd tos,r0		/* syscall number */
	movd r1,tos
	svc			/* do system call */
	bcc 1f			/* check error */
	movqd -1,r0
1:	jump 0(0(sp))		/* return */
");

#ifndef ntohl
__asm("
	.text
	.align	2
_ntohl:	movd 4(sp),r0
	rotw 8,r0
	rotd 16,r0
	rotw 8,r0
	ret 0
");
#endif

#endif /* DYNAMIC */

#include "common.c"

#ifdef MCRT0
__asm("
	.text
	.align	2
eprol:
");
#endif
