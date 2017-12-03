/*	$NetBSD: nmi_mainbus.c,v 1.11.18.1 2017/12/03 11:36:48 jdolecek Exp $	   */
/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nmi_mainbus.c,v 1.11.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#define _VAX_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/ka88.h>
#include <machine/mainbus.h>

#include "ioconf.h"

static int
nmi_mainbus_print(void *aux, const char *name)
{
	struct nmi_attach_args *na = aux;
	const char *c;

	if (name) {
		if (na->na_slot < 10)
			c = "bi";
		else if (na->na_slot < 20)
			c = "mem";
		else
			c = "cpu";
		aprint_normal("%s at %s", c, name);
		if (na->na_slot < 10)
			aprint_normal(" slot %d", na->na_slot);
		if (vax_boardtype == VAX_BTYP_8800 && na->na_slot > 20)
			aprint_normal(" (%s)", ka88_confdata & KA88_LEFTPRIM ?
			    "right" : "left");
	}
	return UNCONF;
}

static int
nmi_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp(nmi_cd.cd_name, ma->ma_type);
}

static void
nmi_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	struct nmi_attach_args na;
	int nbia, *r = 0;

	aprint_normal("\n");

	na.na_iot = ma->ma_iot;
	na.na_dmat = ma->ma_dmat;

	/* One CPU is always found */
	na.na_type = "cpu";
	na.na_slot = 20;
	config_found(self, (void *)&na, nmi_mainbus_print);

	/* Check for a second one */
	if (vax_boardtype == VAX_BTYP_8800) {
		na.na_type = "cpu";
		na.na_slot = 21;
		config_found(self, (void *)&na, nmi_mainbus_print);
	}

	/* One memory adapter is also present */
	na.na_type = "mem";
	na.na_slot = 10;
	config_found(self, (void *)&na, nmi_mainbus_print);

	/* Enable BI interrupts */
	mtpr(NICTRL_DEV0|NICTRL_DEV1|NICTRL_MNF, PR_NICTRL);

	/* Search for NBIA/NBIB adapters */
	na.na_type = "bi";
	for (nbia = 0; nbia < 2; nbia++) {
		if (r)
			vax_unmap_physmem((vaddr_t)r, 1);
		r = (int *)vax_map_physmem(NBIA_REGS(nbia), 1);
		if (badaddr((void *)r, 4))
			continue;
		na.na_slot = 2 * nbia;
		if (r[1] & 2)
			config_found(self, (void *)&na, nmi_mainbus_print);
		na.na_slot++;
		if (r[1] & 4)
			config_found(self, (void *)&na, nmi_mainbus_print);
	}
}

CFATTACH_DECL_NEW(nmi_mainbus, 0,
    nmi_mainbus_match, nmi_mainbus_attach, NULL, NULL);
