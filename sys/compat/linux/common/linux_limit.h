/* 	$NetBSD: linux_limit.h,v 1.5.22.2 2017/12/03 11:36:55 jdolecek Exp $ */

/*-
 * Copyright (c) 1995, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz; by Jason R. Thorpe
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research Center.
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

#ifndef _LINUX_LIMIT_H_
#define _LINUX_LIMIT_H_

static int linux_to_bsd_limit(int);

#ifdef LINUX_LARGEFILE64
#define bsd_to_linux_rlimit1(l, b, f) \
    (l)->f = ((b)->f == RLIM_INFINITY || \
	     ((b)->f & 0x8000000000000000UL) != 0) ? \
    LINUX_RLIM_INFINITY : (b)->f
#else
#define bsd_to_linux_rlimit1(l, b, f) \
    (l)->f = ((b)->f == RLIM_INFINITY || \
	     ((b)->f & 0xffffffff00000000ULL) != 0) ? \
    LINUX_RLIM_INFINITY : (int32_t)(b)->f
#endif
#define bsd_to_linux_rlimit(l, b) \
    bsd_to_linux_rlimit1(l, b, rlim_cur); \
    bsd_to_linux_rlimit1(l, b, rlim_max)

#define linux_to_bsd_rlimit1(b, l, f) \
    (b)->f = (l)->f == LINUX_RLIM_INFINITY ? RLIM_INFINITY : (l)->f
#define linux_to_bsd_rlimit(b, l) \
    linux_to_bsd_rlimit1(b, l, rlim_cur); \
    linux_to_bsd_rlimit1(b, l, rlim_max)

#define linux32_to_bsd_rlimit1(b, l, f) \
    (b)->f = (l)->f == LINUX32_RLIM_INFINITY ? RLIM_INFINITY : (uint32_t)(l)->f
#define linux32_to_bsd_rlimit(b, l) \
    linux32_to_bsd_rlimit1(b, l, rlim_cur); \
    linux32_to_bsd_rlimit1(b, l, rlim_max)

static int
linux_to_bsd_limit(int lim)
{      
	switch (lim) {
	case LINUX_RLIMIT_CPU:
		return RLIMIT_CPU;
	case LINUX_RLIMIT_FSIZE:
		return RLIMIT_FSIZE;
	case LINUX_RLIMIT_DATA:
		return RLIMIT_DATA;
	case LINUX_RLIMIT_STACK:
		return RLIMIT_STACK;
	case LINUX_RLIMIT_CORE:
		return RLIMIT_CORE;
	case LINUX_RLIMIT_RSS:
		return RLIMIT_RSS;
	case LINUX_RLIMIT_NPROC:
		return RLIMIT_NPROC;
	case LINUX_RLIMIT_NOFILE:
		return RLIMIT_NOFILE;
	case LINUX_RLIMIT_MEMLOCK:
		return RLIMIT_MEMLOCK;
	case LINUX_RLIMIT_AS:
		return RLIMIT_AS;
	case LINUX_RLIMIT_LOCKS:
		return -EOPNOTSUPP;
	default:
		return -EINVAL;
	}
}      


#endif /* _LINUX_LIMIT_H_ */
