/*	$NetBSD: crt0.c,v 1.25 1997/06/19 06:02:06 mikel Exp $	*/

/*
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1993 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
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


#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: crt0.c,v 1.25 1997/06/19 06:02:06 mikel Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>

#include <stdlib.h>

#include "common.h"

extern unsigned char	etext;
extern unsigned char	eprol asm("eprol");
extern void		start __P((void)) asm("start");

__asm("
	.text
	.align	2
	.globl	start
start:
	movl	%ebx,___ps_strings
	movl	(%esp),%eax
	leal	8(%esp,%eax,4),%ecx
	leal	4(%esp),%edx
	pushl	%ecx
	pushl	%edx
	pushl	%eax
	call	___start
");
	
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
asm("
	.text
	.align	2
___syscall:
	popl	%ecx
	popl	%eax
	pushl	%ecx
	int	$0x80
	jc	1f
	jmp	%ecx
1:
	movl	$-1,%eax
	jmp	%ecx
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
