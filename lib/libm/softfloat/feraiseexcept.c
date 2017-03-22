/*	$NetBSD: feraiseexcept.c,v 1.1 2017/03/22 23:11:09 chs Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: feraiseexcept.c,v 1.1 2017/03/22 23:11:09 chs Exp $");

#include "namespace.h"

#include <fenv.h>
#include <ieeefp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(feraiseexcept,_feraiseexcept)
#endif

#include <stdio.h>

int
feraiseexcept(int excepts)
{

	fpsetsticky(fpgetsticky() | __FPE(excepts));
	excepts &= __FEE(fpgetmask());
	if (excepts) {
		siginfo_t info;
		memset(&info, 0, sizeof info);
		info.si_signo = SIGFPE;
		info.si_pid = getpid();
		info.si_uid = geteuid();
		if (excepts & FE_UNDERFLOW)
			info.si_code = FPE_FLTUND;
		else if (excepts & FE_OVERFLOW)
			info.si_code = FPE_FLTOVF;
		else if (excepts & FE_DIVBYZERO)
			info.si_code = FPE_FLTDIV;
		else if (excepts & FE_INVALID)
			info.si_code = FPE_FLTINV;
		else if (excepts & FE_INEXACT)
			info.si_code = FPE_FLTRES;
		sigqueueinfo(getpid(), &info);
	}
	return 0;
}
