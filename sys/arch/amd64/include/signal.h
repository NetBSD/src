/*	$NetBSD: signal.h,v 1.8.26.1 2007/02/27 16:48:55 yamt Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991 Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)signal.h	7.16 (Berkeley) 3/17/91
 */

#ifndef _AMD64_SIGNAL_H_
#define _AMD64_SIGNAL_H_

#include <sys/featuretest.h>

typedef int sig_atomic_t;

#if defined(_NETBSD_SOURCE)
/*
 * Get the "code" values
 */
#include <machine/trap.h>
#include <machine/fpu.h>
#include <machine/mcontext.h>

#ifdef _KERNEL
#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#endif
#if defined(COMPAT_16) && defined(COMPAT_NETBSD32)
#define SIGTRAMP_VALID(vers)	((unsigned)(vers) <= 2)
#else
#define SIGTRAMP_VALID(vers)	((vers) == 2)
#endif
#endif

#endif	/* _NETBSD_SOURCE */
#endif	/* !_AMD64_SIGNAL_H_ */
