/*	$NetBSD: cpunode.c,v 1.2 2011/01/18 01:02:53 matt Exp $	*/
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

__KERNEL_RCSID(0, "$NetBSD: cpunode.c,v 1.2 2011/01/18 01:02:53 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include "ioconf.h"

#include <powerpc/booke/cpuvar.h>

static int cpunode_match(device_t, cfdata_t, void *);
static void cpunode_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpunode, sizeof(struct cpunode_softc),
    cpunode_match, cpunode_attach, NULL, NULL);

static u_int nodes;

static int
cpunode_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	if (strcmp(ma->ma_name, cpunode_cd.cd_name) != 0)
		return 0;

	if (ma->ma_node > 8 || (nodes & (1 << ma->ma_node)))
		return 0;

	return 1;
}

static int
cpunode_print(void *aux, const char *pnp)
{
	struct cpunode_attach_args *cna = aux;

	if (pnp)
#if 0
		return QUIET;
#else
		aprint_normal("%s at %s", cna->cna_locs.cnl_name, pnp);
#endif

	if (cna->cna_locs.cnl_instance != 0)
		aprint_normal(" instance %d", cna->cna_locs.cnl_instance);

	return UNCONF;
}

static void
cpunode_attach(device_t parent, device_t self, void *aux)
{
	const struct cpunode_locators *cnl = cpu_md_ops.md_cpunode_locs;
	struct cpunode_softc * const sc = device_private(self);
	struct mainbus_attach_args * const ma = aux;
	struct cpunode_attach_args cna;

	sc->sc_dev = self;

	aprint_normal("\n");
	aprint_normal_dev(self,
	    "%"PRIu64"KB/%"PRIu64"B %"PRIu64"-banked %"PRIu64"-way unified L2 cache\n",
	    board_info_get_number("l2-cache-size") / 1024,
	    board_info_get_number("l2-cache-line-size"),
	    board_info_get_number("l2-cache-banks"),
	    board_info_get_number("l2-cache-ways"));

	nodes |= 1 << ma->ma_node;

	for (u_int childmask = 1; cnl->cnl_name != NULL; cnl++, childmask <<= 1) {
		cna.cna_busname = "cpunode";
		cna.cna_memt = ma->ma_memt;
		cna.cna_dmat = ma->ma_dmat;
		cna.cna_childmask = childmask;
		cna.cna_locs = *cnl;

#if DEBUG > 1
		aprint_normal_dev(self, "dev=%s[%u], addr=%x@%x",
		    cnl->cnl_name, cnl->cnl_instance, cnl->cnl_size,
		    cnl->cnl_addr);
		if (cnl->cnl_nintr > 0) {
			aprint_normal(", intrs=%u", cnl->cnl_intrs[0]);
			for (u_int i = 1; i < cnl->cnl_nintr; i++)
				aprint_normal(",%u", cnl->cnl_intrs[i]);
		}
		aprint_normal("\n");
#endif
		(void)config_found_sm_loc(self, "cpunode", NULL, &cna,
		    cpunode_print, NULL);
	}
	/*
	 * Anything MD left to do?
	 */
	if (cpu_md_ops.md_cpunode_attach != NULL)
		(*cpu_md_ops.md_cpunode_attach)(parent, self, aux);
}

static int cpu_match(device_t, cfdata_t, void *);
static void cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpu_match, cpu_attach, NULL, NULL);

static int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct cpunode_attach_args * const cna = aux;

	if (strcmp(cna->cna_locs.cnl_name, cpu_cd.cd_name) != 0)
		return 0;

	if (psc->sc_children & cna->cna_childmask)
		return 0;

	return 1;
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct cpunode_attach_args * const cna = aux;

	psc->sc_children |= cna->cna_childmask;

	aprint_normal("\n");

	(*cpu_md_ops.md_cpu_attach)(self, cna->cna_locs.cnl_instance);
}
