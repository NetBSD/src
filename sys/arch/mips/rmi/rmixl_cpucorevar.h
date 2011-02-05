/*	$NetBSD: rmixl_cpucorevar.h,v 1.1.2.5 2011/02/05 06:00:13 cliff Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#ifndef _ARCH_MIPS_RMI_RMIXL_CPUCOREVAR_H_
#define _ARCH_MIPS_RMI_RMIXL_CPUCOREVAR_H_

#include "opt_multiprocessor.h"

struct cpucore_softc {
	device_t	sc_dev;
	bool		sc_attached;
	u_int		sc_core;
#ifdef MULTIPROCESSOR
	bool		sc_hatched;
	struct pmap_tlb_info *sc_tlbinfo;
	struct pmap_tlb_info sc_tlbinfo0;
#endif
};

struct cpucore_attach_args {
	const char     *ca_name;
	int		ca_core;
	int		ca_thread;
};

extern void cpucore_rmixl_hatch(device_t);

#endif	/* _ARCH_MIPS_RMI_RMIXL_CPUCOREVAR_H_ */
