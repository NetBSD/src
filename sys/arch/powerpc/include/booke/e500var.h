/*	$NetBSD: e500var.h,v 1.6.2.1 2013/02/25 00:28:53 tls Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_E500VAR_H_
#define _POWERPC_BOOKE_E500VAR_H_

#ifdef _KERNEL

#include <sys/device.h>
#include <sys/extent.h>

#ifdef _KERNEL_OPT
#include "locators.h"
#endif

#define	E500_CLOCK_TIMER	0	/* could be 0..3 */

extern const struct intrsw e500_intrsw;
extern struct extent *pcimem_ex;
extern struct extent *pciio_ex;
void	e500_device_register(device_t, void *);
int	e500_clock_intr(void *);
void	e500_cpu_start(void);
void	e500_tlb_init(vaddr_t, psize_t);
void	e500_tlb_minimize(vaddr_t);
bool	e500_device_disabled_p(uint32_t);

struct e500_truthtab {
	uint16_t tt_svrhi;
	uint16_t tt_instance;
	uint32_t tt_mask;
	uint32_t tt_value;
	u_int tt_result;
};

#define	TRUTH_ENCODE(svr, inst, mask, value, result)	\
	{						\
		.tt_svrhi = (svr) >> 16,		\
		.tt_instance = (inst),			\
		.tt_mask = (mask),			\
		.tt_value = (value),			\
		.tt_result = (result),			\
	}

u_int	e500_truth_decode(u_int, uint32_t, const struct e500_truthtab *,
	    size_t, u_int);
uint16_t e500_get_svr(void);
#ifdef _KERNEL_OPT
int	e500_cpunode_submatch(device_t, cfdata_t, const char *, void *);
#endif

/*
 * Used by MP hatch code to fetch the TLB1 entries so they be setup on the
 * just hatched CPU.
 */
void *	e500_tlb1_fetch(size_t);
void	e500_tlb1_sync(void);
void	e500_ipi_halt(void);
void	e500_spinup_trampoline(void);
void	e500_cpu_hatch(struct cpu_info *);
struct e500_xtlb *
	e500_tlb_lookup_xtlb(vaddr_t, u_int *);


void	pq3gpio_attach(device_t, device_t, void *);

/*
 * For a lack of a better place, define this u-boot structure here.
 */

struct uboot_spinup_entry {
	uint32_t entry_addr_upper;
	uint32_t entry_addr_lower;
	uint32_t entry_r3_upper;
	uint32_t entry_r3_lower;
	uint32_t entry__rsvd;
	uint32_t entry_pir;
	uint32_t entry_r6_upper;
	uint32_t entry_r6_lower;
};

#endif /* _KERNEL */

#endif /* !_POWERPC_BOOKE_E500VAR_H_ */
