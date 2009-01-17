/* 	$NetBSD: linux_siginfo.h,v 1.10.70.2 2009/01/17 13:28:45 mjf Exp $	*/

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

#ifndef _LINUX_SIGINFO_H
#define _LINUX_SIGINFO_H

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_siginfo.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_siginfo.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_siginfo.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_siginfo.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_siginfo.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_siginfo.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_siginfo.h>
#endif

/* si_code values for non signal */
#define	LINUX_SI_USER		0
#define	LINUX_SI_KERNEL		0x80
#define	LINUX_SI_QUEUE		-1
#ifndef LINUX_SI_CODE_ARCH /* all except mips */
#define	LINUX_SI_TIMER		-2
#define	LINUX_SI_MESGQ		-3
#define	LINUX_SI_ASYNCIO	-4
#endif /* LINUX_SI_CODE_ARCH */
#define	LINUX_SI_SIGIO		-5
#define	LINUX_SI_TKILL		-6
#define	LINUX_SI_DETHREAD	-7

/* si_code values for SIGILL */
#define	LINUX_ILL_ILLOPC	1
#define	LINUX_ILL_ILLOPN	2
#define	LINUX_ILL_ILLADR	3
#define	LINUX_ILL_ILLTRP	4
#define	LINUX_ILL_PRVOPC	5
#define	LINUX_ILL_PRVREG	6
#define	LINUX_ILL_COPROC	7
#define	LINUX_ILL_BADSTK	8

/* si_code values for SIGFPE */
#define	LINUX_FPE_INTDIV	1
#define	LINUX_FPE_INTOVF	2
#define	LINUX_FPE_FLTDIV	3
#define	LINUX_FPE_FLTOVF	4
#define	LINUX_FPE_FLTUND	5
#define	LINUX_FPE_FLTRES	6
#define	LINUX_FPE_FLTINV	7
#define	LINUX_FPE_FLTSUB	8

/* si_code values for SIGSEGV */
#define	LINUX_SEGV_MAPERR	1
#define	LINUX_SEGV_ACCERR	2

/* si_code values for SIGBUS */
#define	LINUX_BUS_ADRALN	1
#define	LINUX_BUS_ADRERR	2
#define	LINUX_BUS_OBJERR	3

/* si_code values for SIGTRAP */
#define	LINUX_TRAP_BRKPT	1
#define	LINUX_TRAP_TRACE	2

/* si_code values for SIGCHLD */
#define	LINUX_CLD_EXITED	1
#define	LINUX_CLD_KILLED	2
#define	LINUX_CLD_DUMPED	3
#define	LINUX_CLD_TRAPPED	4
#define	LINUX_CLD_STOPPED	5
#define	LINUX_CLD_CONTINUED	6

/* si_code values for SIGPOLL */
#define	LINUX_POLL_IN		1
#define	LINUX_POLL_OUT		2
#define	LINUX_POLL_MSG		3
#define	LINUX_POLL_ERR		4
#define	LINUX_POLL_PRI		5
#define	LINUX_POLL_HUP		6

#define	LINUX_SI_FROMUSER(sp)	((sp)->si_code <= 0)
#define	LINUX_SI_FROMKERNEL(sp)	((sp)->si_code > 0)

#endif /* !_LINUX_SIGINFO_H */
