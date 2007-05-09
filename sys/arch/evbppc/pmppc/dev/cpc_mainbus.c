/*	$NetBSD: cpc_mainbus.c,v 1.1.2.2 2007/05/09 09:02:51 garbled Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: cpc_mainbus.c,v 1.1.2.2 2007/05/09 09:02:51 garbled Exp $");

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include "locators.h"

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700var.h>
#include <dev/ic/cpc700uic.h>

#include <machine/pmppc.h>
#include <arch/evbppc/pmppc/dev/mainbus.h>

struct genppc_pci_chipset *genppc_pct;

void
cpc_attach(struct device *self, pci_chipset_tag_t pc, bus_space_tag_t mem,
	   bus_space_tag_t pciio, bus_dma_tag_t tag, int attachpci,
	   uint freq);

static int	cpc_mainbus_match(struct device *, struct cfdata *, void *);
static void	cpc_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpc_mainbus, sizeof(struct device),
    cpc_mainbus_match, cpc_mainbus_attach, NULL, NULL);

int
cpc_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return (strcmp(maa->mb_name, "cpc") == 0);
}

void
cpc_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct genppc_pci_chipset_businfo *pbi;

	genppc_pct = malloc(sizeof(struct genppc_pci_chipset), M_DEVBUF,
	    M_NOWAIT);
	pmppc_pci_get_chipset_tag(genppc_pct);
	pbi = malloc(sizeof(struct genppc_pci_chipset_businfo),
	    M_DEVBUF, M_NOWAIT);
	KASSERT(pbi != NULL);
	pbi->pbi_properties = prop_dictionary_create();
	KASSERT(pbi->pbi_properties != NULL);
	SIMPLEQ_INIT(&genppc_pct->pc_pbi);
	SIMPLEQ_INSERT_TAIL(&genppc_pct->pc_pbi, pbi, next);

	cpc_attach(self, genppc_pct, &pmppc_mem_tag, &pmppc_pci_io_tag,
		   &pci_bus_dma_tag, a_config.a_is_monarch,
		   a_config.a_bus_freq);

	if (!a_config.a_is_monarch)
		printf("%s: not Monarch, pci not attached\n", self->dv_xname);
}
