/*	$NetBSD: nmi_mainbus.c,v 1.6 2003/07/15 02:15:05 lukem Exp $	   */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: nmi_mainbus.c,v 1.6 2003/07/15 02:15:05 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#define	_VAX_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/cpu.h>
#include <machine/ka88.h>


extern	struct vax_bus_space vax_mem_bus_space;
extern	struct vax_bus_dma_tag vax_bus_dma_tag;

static int
nmi_mainbus_print(void *aux, const char *name)
{
	struct nmi_attach_args *na = aux;
	char *c;

	if (name) {
		if (na->slot < 10)
			c = "bi";
		else if (na->slot < 20)
			c = "mem";
		else
			c = "cpu";
		aprint_normal("%s at %s", c, name);
		if (na->slot < 10)
			aprint_normal(" slot %d", na->slot);
		if (vax_boardtype == VAX_BTYP_8800 && na->slot > 20)
			aprint_normal(" (%s)", ka88_confdata & KA88_LEFTPRIM ?
			    "right" : "left");
	}
	return UNCONF;
}

static int
nmi_mainbus_match(struct device *parent, struct cfdata *vcf, void *aux)
{
	if (vax_bustype == VAX_NBIBUS)
		return 1;
	return 0;
}

static void
nmi_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct nmi_attach_args na;
	int nbia, *r = 0;

	printf("\n");

	/* One CPU is always found */
	na.slot = 20;
	config_found(self, (void *)&na, nmi_mainbus_print);

	/* Check for a second one */
	if (vax_boardtype == VAX_BTYP_8800) {
		na.slot = 21;
		config_found(self, (void *)&na, nmi_mainbus_print);
	}

	/* One memory adapter is also present */
	na.slot = 10;
	config_found(self, (void *)&na, nmi_mainbus_print);

	/* Enable BI interrupts */
	mtpr(NICTRL_DEV0|NICTRL_DEV1|NICTRL_MNF, PR_NICTRL);

	/* Search for NBIA/NBIB adapters */
	for (nbia = 0; nbia < 2; nbia++) {
		if (r)
			vax_unmap_physmem((vaddr_t)r, 1);
		r = (int *)vax_map_physmem(NBIA_REGS(nbia), 1);
		if (badaddr((caddr_t)r, 4))
			continue;
		na.slot = 2 * nbia;
		if (r[1] & 2)
			config_found(self, (void *)&na, nmi_mainbus_print);
		na.slot++;
		if (r[1] & 4)
			config_found(self, (void *)&na, nmi_mainbus_print);
	}
}

CFATTACH_DECL(nmi_mainbus, sizeof(struct device),
    nmi_mainbus_match, nmi_mainbus_attach, NULL, NULL);
