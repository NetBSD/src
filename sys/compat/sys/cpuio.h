/*	$NetBSD: cpuio.h,v 1.1 2009/01/19 17:39:02 christos Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#if !defined(_COMPAT_SYS_CPUIO_H_)
#define	_COMPAT_SYS_CPUIO_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioccom.h>

#ifndef _KERNEL
#include <stdbool.h>
#endif

typedef struct cpustate50 {
	u_int		cs_id;		/* matching ci_cpuid */
	bool		cs_online;	/* running unbound LWPs */
	bool		cs_intr;	/* fielding interrupts */
	bool		cs_unused[2];	/* reserved */
	int32_t		cs_lastmod;	/* time of last state change */
	char		cs_name[16];	/* reserved */
	uint32_t	cs_reserved[4];	/* reserved */
} cpustate50_t;

static __inline
void cpustate_to_cpustate50(const cpustate_t *cp, cpustate50_t *cp50)
{
	cp50->cs_id = (int32_t)cp->cs_id;
	cp50->cs_online = (int32_t)cp->cs_online;
	cp50->cs_intr = (int32_t)cp->cs_intr;
	memcpy(cp50->cs_unused, cp->cs_unused, sizeof(cp50->cs_unused));
	cp50->cs_lastmod = (int32_t)cp->cs_lastmod;
	memcpy(cp50->cs_name, cp->cs_name, sizeof(cp50->cs_name));
	memcpy(cp50->cs_reserved, cp->cs_reserved, sizeof(cp50->cs_reserved));
}

static __inline
void cpustate50_to_cpustate(const cpustate50_t *cp50, cpustate_t *cp)
{
	cp->cs_id = cp50->cs_id;
	cp->cs_online = cp50->cs_online;
	cp->cs_intr = cp50->cs_intr;
	memcpy(cp->cs_unused, cp50->cs_unused, sizeof(cp->cs_unused));
	cp->cs_lastmod = cp50->cs_lastmod;
	memcpy(cp->cs_name, cp50->cs_name, sizeof(cp->cs_name));
	memcpy(cp->cs_reserved, cp50->cs_reserved, sizeof(cp->cs_reserved));
}

#define	IOC_CPU_OSETSTATE	_IOW('c', 0, cpustate50_t)
#define	IOC_CPU_OETSTATE	_IOWR('c', 1, cpustate50_t)

#endif /* !_COMPAT_SYS_CPUIO_H_ */
