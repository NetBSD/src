/*	$NetBSD: linux_types.h,v 1.11.4.3 2002/06/23 17:44:29 jdolecek Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_types.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_types.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_types.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_types.h>
/*
 * The rest are not written yet:
 */
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_types.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_types.h>
#else
typedef unsigned long linux_clock_t;
typedef unsigned long linux_time_t;
#endif

/*
 * Structure for uname(2)
 */
struct linux_utsname {
	char l_sysname[65];
	char l_nodename[65];
	char l_release[65];
	char l_version[65];
	char l_machine[65];
	char l_domainname[65];
};

extern char linux_sysname[];
extern char linux_release[];
extern char linux_version[];

struct linux_tms {
	linux_clock_t ltms_utime;	
	linux_clock_t ltms_stime;
	linux_clock_t ltms_cutime;
	linux_clock_t ltms_cstime;
};

struct linux_utimbuf {
	linux_time_t l_actime;
	linux_time_t l_modtime;
};

struct linux___sysctl {
	int          *name;
	int           nlen;
	void         *oldval;
	size_t       *oldlenp;
	void         *newval;
	size_t        newlen;
	unsigned long __unused[4];
};

#include <compat/linux/common/linux_siginfo.h>

#endif /* !_LINUX_TYPES_H */
