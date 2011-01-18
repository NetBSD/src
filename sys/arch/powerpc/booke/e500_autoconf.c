/*	$NetBSD: e500_autoconf.c,v 1.2 2011/01/18 01:02:52 matt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: e500_autoconf.c,v 1.2 2011/01/18 01:02:52 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#define	GLOBAL_PRIVATE

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

void
e500_device_register(device_t dev, void *aux)
{
}

/*
 * Let's see if the DEVDISR bit encoded in the low 6 bits of cnl_flags is
 * set.  If it is, the device is disabled.
 */
bool
e500_device_disabled_p(uint32_t flags)
{
	if ((flags-- & 63) == 0)
		return false;

	const uint32_t v = cpu_read_4(GLOBAL_BASE + DEVDISR);
	return (v & (1 << (flags & 31))) != 0;
}

uint16_t
e500_get_svr(void)
{
	uint32_t svr = (mfspr(SPR_SVR) >> 16) & ~8;

	switch (svr) {
	case SVR_MPC8543v1 >> 16:
		return SVR_MPC8548v1 >> 16;
	case SVR_MPC8541v1 >> 16:
		return SVR_MPC8555v1 >> 16;
	default:
		return svr;
	}
}

u_int
e500_truth_decode(u_int instance, uint32_t data,
	const struct e500_truthtab *tab, size_t ntab, u_int default_value)
{
	const uint16_t svr = e500_get_svr();

	for (u_int i = ntab; i-- > 0; tab++) {
#if 0
		printf("%s: [%u] = %x/%x %u/%u (%#x & %#x) = %#x (%#x)\n",
		    __func__, i, tab->tt_svrhi, svr,
		    tab->tt_instance, instance,
		    data, tab->tt_mask, data & tab->tt_mask, tab->tt_value);
#endif
		if (tab->tt_svrhi == svr
		    && tab->tt_instance == instance
		    && (data & tab->tt_mask) == tab->tt_value)
			return tab->tt_result;
	}
	return default_value;
}

int
e500_cpunode_submatch(device_t parent, cfdata_t cf, const char *name, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;

	if (!board_info_get_bool("pq3")
	    || strcmp(cnl->cnl_name, name) != 0
	    || e500_device_disabled_p(cnl->cnl_flags)
	    || (cna->cna_childmask & psc->sc_children)
	    || (cf->cf_loc[CPUNODECF_INSTANCE] != CPUNODECF_INSTANCE_DEFAULT
		&& cf->cf_loc[CPUNODECF_INSTANCE] != cnl->cnl_instance))
		return 0;

	return 1;
}
