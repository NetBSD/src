/* $NetBSD: ansi.h,v 1.4.6.1 2015/09/22 12:05:53 skrll Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARCH_USERMODE_INCLUDE_ANSI_H
#define _ARCH_USERMODE_INCLUDE_ANSI_H

#include <sys/cdefs.h>
#include <machine/int_types.h>

#define _BSD_CLOCK_T_		unsigned int
#define _BSD_TIME_T_		__int64_t
#define _BSD_CLOCKID_T_		int
#define _BSD_TIMER_T_		int
#define _BSD_SUSECONDS_T_	int
#define _BSD_USECONDS_T_	unsigned int
#define _BSD_WCHAR_T_		int
#define _BSD_WINT_T_		int

#if defined(__i386__)
#define _BSD_PTRDIFF_T_		int
#define _BSD_SIZE_T_		unsigned int
#define _BSD_SSIZE_T_		int
#elif defined(__x86_64__)
#define _BSD_PTRDIFF_T_		long
#define _BSD_SIZE_T_		unsigned long
#define _BSD_SSIZE_T_		long
#elif defined(__arm__)
#define _BSD_PTRDIFF_T_		long int
#define _BSD_SIZE_T_		unsigned long int
#define _BSD_SSIZE_T_		long int
#else
#error "platform not supported"
#endif

#endif /* !_ARCH_USERMODE_INCLUDE_ANSI_H */
