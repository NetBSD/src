/*	$NetBSD: cpuio.h,v 1.3.2.1 2009/05/13 17:23:03 jym Exp $	*/

/*-
 * Copyright (c) 2007, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#if !defined(_SYS_CPUIO_H_)
#define	_SYS_CPUIO_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioccom.h>

#ifndef _KERNEL
#include <stdbool.h>
#endif

/*
 * This is not a great place to describe CPU properties, those
 * are better returned via autoconf.
 */
typedef struct cpustate {
	u_int		cs_id;		/* matching ci_cpuid */
	bool		cs_online;	/* running unbound LWPs */
	bool		cs_intr;	/* fielding interrupts */
	bool		cs_unused[2];	/* reserved */
	int32_t		cs_lastmod;	/* time of last state change */
	char		cs_name[16];	/* reserved */
	int32_t		cs_lastmodhi;	/* time of last state change */
	uint32_t	cs_intrcnt;	/* count of interrupt handlers + 1 */
	uint32_t	cs_reserved[2];	/* reserved */
} cpustate_t;

#define	IOC_CPU_SETSTATE	_IOW('c', 0, cpustate_t)
#define	IOC_CPU_GETSTATE	_IOWR('c', 1, cpustate_t)
#define	IOC_CPU_GETCOUNT	_IOR('c', 2, int)
#define	IOC_CPU_MAPID		_IOWR('c', 3, int)

#endif /* !_SYS_CPUIO_H_ */
