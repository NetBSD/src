/*	$NetBSD: hpux_signo.c,v 1.1 2002/03/31 22:22:43 christos Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: hpux_sig.c 1.4 92/01/20$
 *
 *	@(#)hpux_sig.c	8.2 (Berkeley) 9/23/93
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_signo.c,v 1.1 2002/03/31 22:22:43 christos Exp $");

#include <sys/types.h>
#include <sys/signal.h>


/* indexed by HPUX signal number */
const int hpux_to_native_signo[NSIG] = {
	0,
/*01*/	SIGHUP,  SIGINT, SIGQUIT, SIGILL,   SIGTRAP, SIGIOT,  SIGEMT,   SIGFPE,
/*09*/  SIGKILL, SIGBUS, SIGSEGV, SIGSYS,   SIGPIPE, SIGALRM, SIGTERM,  SIGUSR1,
/*17*/  SIGUSR2, SIGCHLD, 0,      SIGVTALRM,SIGPROF, SIGIO,   SIGWINCH, SIGSTOP,
/*25*/	SIGTSTP, SIGCONT,SIGTTIN, SIGTTOU,  SIGURG,  0,       0,        0
};

/* indexed by BSD signal number */
const int native_to_hpux_signo[NSIG] = {
	 0,
/*01*/	 1,  2,  3,  4,  5,  6,  7,  8,
/*09*/   9, 10, 11, 12, 13, 14, 15, 29,
/*17*/  24, 25, 26, 18, 27, 28, 22,  0,
/*25*/	 0, 20, 21, 23,  0, 16, 17,  0
};
