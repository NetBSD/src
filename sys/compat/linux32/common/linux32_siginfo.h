/* 	$NetBSD: linux32_siginfo.h,v 1.1.8.2 2012/04/17 00:07:19 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _LINUX32_SIGINFO_H
#define _LINUX32_SIGINFO_H

#if defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_siginfo.h>
#endif

/* si_code values for non signal */
#define	LINUX32_SI_USER		0
#define	LINUX32_SI_KERNEL		0x80
#define	LINUX32_SI_QUEUE		-1
#ifndef LINUX32_SI_TIMER /* all except mips */
#define	LINUX32_SI_TIMER		-2
#define	LINUX32_SI_MESGQ		-3
#define	LINUX32_SI_ASYNCIO	-4
#endif /* LINUX32_SI_TIMER */
#define	LINUX32_SI_SIGIO		-5
#define	LINUX32_SI_TKILL		-6
#define	LINUX32_SI_DETHREAD	-7

/* si_code values for SIGILL */
#define	LINUX32_ILL_ILLOPC	1
#define	LINUX32_ILL_ILLOPN	2
#define	LINUX32_ILL_ILLADR	3
#define	LINUX32_ILL_ILLTRP	4
#define	LINUX32_ILL_PRVOPC	5
#define	LINUX32_ILL_PRVREG	6
#define	LINUX32_ILL_COPROC	7
#define	LINUX32_ILL_BADSTK	8

/* si_code values for SIGFPE */
#define	LINUX32_FPE_INTDIV	1
#define	LINUX32_FPE_INTOVF	2
#define	LINUX32_FPE_FLTDIV	3
#define	LINUX32_FPE_FLTOVF	4
#define	LINUX32_FPE_FLTUND	5
#define	LINUX32_FPE_FLTRES	6
#define	LINUX32_FPE_FLTINV	7
#define	LINUX32_FPE_FLTSUB	8

/* si_code values for SIGSEGV */
#define	LINUX32_SEGV_MAPERR	1
#define	LINUX32_SEGV_ACCERR	2

/* si_code values for SIGBUS */
#define	LINUX32_BUS_ADRALN	1
#define	LINUX32_BUS_ADRERR	2
#define	LINUX32_BUS_OBJERR	3

/* si_code values for SIGTRAP */
#define	LINUX32_TRAP_BRKPT	1
#define	LINUX32_TRAP_TRACE	2

/* si_code values for SIGCHLD */
#define	LINUX32_CLD_EXITED	1
#define	LINUX32_CLD_KILLED	2
#define	LINUX32_CLD_DUMPED	3
#define	LINUX32_CLD_TRAPPED	4
#define	LINUX32_CLD_STOPPED	5
#define	LINUX32_CLD_CONTINUED	6

/* si_code values for SIGPOLL */
#define	LINUX32_POLL_IN		1
#define	LINUX32_POLL_OUT		2
#define	LINUX32_POLL_MSG		3
#define	LINUX32_POLL_ERR		4
#define	LINUX32_POLL_PRI		5
#define	LINUX32_POLL_HUP		6

#define	LINUX32_SI_FROMUSER(sp)	((sp)->si_code <= 0)
#define	LINUX32_SI_FROMKERNEL(sp)	((sp)->si_code > 0)

#define lsi_pid		_sidata._kill._pid
#define lsi_uid		_sidata._kill._uid
#define lsi_status      _sidata._sigchld._status
#define lsi_utime       _sidata._sigchld._utime
#define lsi_stime       _sidata._sigchld._stime
#define lsi_value       _sidata._rt._sigval
#define lsi_sival_int   _sidata._rt._sigval.sival_int
#define lsi_sival_ptr   _sidata._rt._sigval.sival_ptr
#define lsi_addr        _sidata._sigfault._addr
#define lsi_band        _sidata._sigpoll._band
#define lsi_fd          _sidata._sigpoll._fd

void native_to_linux32_siginfo(linux32_siginfo_t *, const struct _ksiginfo *);

#endif /* !_LINUX32_SIGINFO_H */
