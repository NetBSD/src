/* $NetBSD: cpufeat.h,v 1.4.86.1 2018/03/30 06:20:12 pgoyette Exp $ */
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#ifndef _POWERPC_OEA_OEAFEAT_H_

/* Cpu features for OEA Cpus.
 * These are only features that affect early bootstrap, and decisions
 * that need to be made very early on, like what pmap to use, if bats are
 * available, etc etc.  More can be added later. Some are not yet utilized.
 */

/* 0 means, 32bit, has bats, not a 601, ie, the default 604 case */

#define OEACPU_64		(1 << 0)
#define OEACPU_64_BRIDGE	(1 << 1)
#define OEACPU_NOBAT		(1 << 2)
#define OEACPU_HIGHBAT		(1 << 3)
#define OEACPU_601		(1 << 4)
#define OEACPU_HIGHSPRG		(1 << 5)
#define OEACPU_ALTIVEC		(1 << 6)
#define OEACPU_XBSEN		(1 << 7)	/* BATS > 256MB */

#ifdef _KERNEL
void cpu_model_init(void);
extern unsigned long oeacpufeat;

#define oea_mapiodev(addr, size) ((oeacpufeat & OEACPU_NOBAT) ? \
			mapiodev((paddr_t)(addr), (size), 0) : (void *)(addr))

#endif

#endif /* _POWERPC_OEA_OEAFEAT_H_ */
