/*	$NetBSD: osf1_signo.c,v 1.1.8.2 2002/06/23 17:44:38 jdolecek Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_signo.c,v 1.1.8.2 2002/06/23 17:44:38 jdolecek Exp $");

#include <sys/types.h>
#include <sys/signal.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_cvt.h>

/*
 * These tables are used to translate between NetBSD and OSF/1 signal
 * numbers.
 *
 * XXX IT IS NOT UP TO DATE.
 */

const int native_to_osf1_signo[] = {
	0,
	OSF1_SIGHUP,
	OSF1_SIGINT,
	OSF1_SIGQUIT,
	OSF1_SIGILL,
	OSF1_SIGTRAP,
	OSF1_SIGABRT,
	OSF1_SIGEMT,
	OSF1_SIGFPE,
	OSF1_SIGKILL,
	OSF1_SIGBUS,
	OSF1_SIGSEGV,
	OSF1_SIGSYS,
	OSF1_SIGPIPE,
	OSF1_SIGALRM,
	OSF1_SIGTERM,
	OSF1_SIGURG,
	OSF1_SIGSTOP,
	OSF1_SIGTSTP,
	OSF1_SIGCONT,
	OSF1_SIGCHLD,
	OSF1_SIGTTIN,
	OSF1_SIGTTOU,
	OSF1_SIGIO,
	OSF1_SIGXCPU,
	OSF1_SIGXFSZ,
	OSF1_SIGVTALRM,
	OSF1_SIGPROF,
	OSF1_SIGWINCH,
	OSF1_SIGINFO,
	OSF1_SIGUSR1,
	OSF1_SIGUSR2,
};

const int osf1_to_native_signo[] = {
	0,
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGABRT,
	SIGEMT,
	SIGFPE,
	SIGKILL,
	SIGBUS,
	SIGSEGV,
	SIGSYS,
	SIGPIPE,
	SIGALRM,
	SIGTERM,
	SIGURG,
	SIGSTOP,
	SIGTSTP,
	SIGCONT,
	SIGCHLD,
	SIGTTIN,
	SIGTTOU,
	SIGIO,
	SIGXCPU,
	SIGXFSZ,
	SIGVTALRM,
	SIGPROF,
	SIGWINCH,
	SIGINFO,
	SIGUSR1,
	SIGUSR2,
};
