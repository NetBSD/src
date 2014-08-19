/*	$NetBSD: autoconf.c,v 1.7.2.1 2014/08/20 00:02:59 tls Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.7.2.1 2014/08/20 00:02:59 tls Exp $");

#define __INTR_PRIVATE

#include "locators.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <powerpc/booke/cpuvar.h>

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{

	intr_init();
	calc_delayconst();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("%s: mainbus not configured", __func__);

	spl0();
}

static volatile int rootconf_timo = 1;

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf(void)
{
	/*
	 * We wait up to 10 seconds for a bootable device to be found.
	 */
	while (rootconf_timo > 0) {
		if (booted_device != NULL) {
			aprint_normal_dev(booted_device, "boot device\n");
			break;
		}

		if (root_string[0] != '\0'
		    && (booted_device = device_find_by_xname(root_string)) != NULL) {
			aprint_normal_dev(booted_device, "boot device\n");
			break;
		}

		if (EWOULDBLOCK == kpause("autoconf", true, 1, NULL)) {
			rootconf_timo--;
		}
	}

	rootconf();
}

void
device_register(device_t dev, void *aux)
{
	if (cpu_md_ops.md_device_register != NULL)
		(*cpu_md_ops.md_device_register)(dev, aux);

	if (booted_device == NULL) {
		if (root_string[0] != '\0'
		    && !strcmp(device_xname(dev), root_string)) {
			aprint_normal_dev(dev, "boot device\n");
			booted_device = dev;
		} else {
			rootconf_timo = 5 * hz;
		}
	}
}

static bool mainbus_found;

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma = aux;

	if (pnp != NULL)
		return QUIET;

	if (pnp)
		aprint_normal("%s at %s", ma->ma_name, pnp);
	if (ma->ma_node != MAINBUSCF_NODE_DEFAULT)
		aprint_normal(" node %d", ma->ma_node);

	return (UNCONF);
}

static int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	return mainbus_found == false;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args ma;

	mainbus_found = true;

	aprint_normal("\n");

	ma.ma_name = "cpunode";
	ma.ma_node = 0;
	ma.ma_memt = curcpu()->ci_softc->cpu_bst;
	ma.ma_le_memt = curcpu()->ci_softc->cpu_le_bst;
	ma.ma_dmat = &booke_bus_dma_tag;

	config_found_sm_loc(self, "mainbus", NULL, &ma, mainbus_print, NULL);
}

CFATTACH_DECL_NEW(mainbus, 0, mainbus_match, mainbus_attach, NULL, NULL);
