/*	$NetBSD: crt0.c,v 1.8 1999/01/22 11:45:16 mycroft Exp $	*/

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

#include <stdlib.h>

#include "common.h"

#undef mmap
#define mmap(addr, len, prot, flags, fd, off)   		\
	__syscall(SYS_mmap, (addr), (len), (prot), (flags),	\
	(fd), 0, (off_t)(off)) 

extern	void		start __P((void)) asm("start");
	void		__start __P((int, char *[], char *[]));

__asm("
	.text
	.align	0
	.global	start
start:
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

	b	___start	

	.align	0
Lps_strings:
	.word	___ps_strings
");

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.8 1999/01/22 11:45:16 mycroft Exp $");
#endif /* LIBC_SCCS and not lint */

void
__start(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	char *ap;

 	environ = envp;

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
		__load_rtld(&_DYNAMIC);
#endif	/* DYNAMIC */

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&eprol, (u_long)&etext);
#endif	/* MCRT0 */

__asm("__callmain:");		/* Defined for the benefit of debuggers */
	exit(main(argc, argv, envp));
}

#ifndef ntohl
inline in_addr_t
__crt0_swap(x)
	in_addr_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return(	  ((x & 0x000000ff) << 24)
	    	| ((x & 0x0000ff00) << 8)
		| ((x & 0x00ff0000) >> 8)
		| ((x & 0xff000000) >> 24));
#else
	return x;
#endif	/* BYTE_ORDER */
}

#define	ntohl(x)	__crt0_swap(x)
#define	htonl(x)	__crt0_swap(x)
#endif	/* ntohl */

#ifdef	DYNAMIC
__asm("
	.text
	.align	0
___syscall:
	swi	0
	mvncs	r0, #0
	mov	pc, lr
");
#endif	/* DYNAMIC */

#include "common.c"

#ifdef MCRT0
__asm("
	.text
	.align 0
eprol:
");
#endif	/* MCRT0 */
