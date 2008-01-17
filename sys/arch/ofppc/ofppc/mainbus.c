/*	$NetBSD: mainbus.c,v 1.19 2008/01/17 23:42:58 garbled Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.19 2008/01/17 23:42:58 garbled Exp $");

#include "opt_interrupt.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>
#include <arch/powerpc/pic/picvar.h>
#include <machine/pci_machdep.h>
#include <machine/autoconf.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>


int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int mainbus_found = 0;
struct pic_ops *isa_pic;

#ifdef PIC_PREPIVR
vaddr_t prep_intr_reg;
uint32_t prep_intr_reg_off;
#endif

extern ofw_pic_node_t picnodes[8];
extern int nrofpics;
extern int primary_pic;

#ifdef PIC_PREPIVR
static struct pic_ops *
init_prepivr(int node)
{
	int pcinode;
	uint32_t ivr;

	pcinode = OF_finddevice("/pci");
	if (OF_getprop(pcinode, "8259-interrupt-acknowledge", &ivr,
		sizeof(ivr)) != sizeof(ivr)) {
		aprint_error("Incorrectly identified i8259 as prepivr\n");
		return setup_i8259();
	}
	prep_intr_reg = (vaddr_t)mapiodev(ivr, sizeof(uint32_t));
	prep_intr_reg_off = 0; /* hack */
	if (!prep_intr_reg)
		panic("startup: no room for interrupt register");

	return setup_prepivr(PIC_IVR_MOT);
}
#endif

static int
init_openpic(int node)
{
	struct ofw_pci_register aadr;
	struct ranges {
		uint32_t pci_hi, pci_mid, pci_lo;
		uint32_t host;
		uint32_t size_hi, size_lo;
	} ranges[6], *rp = ranges;
	unsigned char *baseaddr = NULL;
	uint32_t reg[12];
	int parent, len;
#ifdef PIC_DISTOPENPIC
	unsigned char *isu[OPENPIC_MAX_ISUS];
	int i, j;
	int isumap[OPENPIC_MAX_ISUS];
#endif

	if (OF_getprop(node, "assigned-addresses", &aadr, sizeof(aadr))
	    != sizeof(aadr))
		goto noaadr;

	parent = OF_parent(node);
	len = OF_getprop(parent, "ranges", ranges, sizeof(ranges));
	if (len == -1)
		goto noaadr;

#ifdef PIC_OPENPIC
	while (len >= sizeof(ranges[0])) {
		if ((rp->pci_hi & OFW_PCI_PHYS_HI_SPACEMASK) ==
		    (aadr.phys_hi & OFW_PCI_PHYS_HI_SPACEMASK) &&
		    (aadr.size_lo + aadr.phys_lo <= rp->size_lo)) {
			baseaddr = (unsigned char *)mapiodev(
			    rp->host | aadr.phys_lo, aadr.size_lo);
			aprint_normal("Found openpic at %08x\n",
			    rp->host | aadr.phys_lo);
			setup_openpic(baseaddr, 0);
			return TRUE;
		}
		/* XXX I think this is correct */
		rp++;
		len -= sizeof(ranges[0]);
	}
#endif
	rp = 0; /* satisfy -Wall */
	return FALSE;
 noaadr:
	/* this isn't a PCI-attached openpic */
	len = OF_getprop(node, "reg", &reg, sizeof(reg));
	if (len < sizeof(int)*2)
		return FALSE;

	if (len == sizeof(int)*2) {	
		baseaddr = (unsigned char *)mapiodev(reg[0], reg[1]);
		aprint_verbose("Found openpic at %08x\n", reg[0]);
#ifdef PIC_OPENPIC
		(void)setup_openpic(baseaddr, 0);
		return TRUE;
#else
		aprint_error("No openpic support compiled into kernel!");
		return FALSE;
#endif
	}

#ifdef PIC_DISTOPENPIC
	/* otherwise, we have a distributed openpic */
	i = len/(sizeof(int)*2) - 1;
	if (i == 0)
		return FALSE;
	if (i > OPENPIC_MAX_ISUS)
		aprint_error("Increase OPENPIC_MAX_ISUS to %d\n", i);
	
	baseaddr = (unsigned char *)mapiodev(reg[0], 0x40000);
	aprint_verbose("Found openpic at %08x\n", reg[0]);

	for (j=0; j < i; j++) {
		isu[j] = (unsigned char *)mapiodev(reg[(j+1)*2],
		    reg[(j+1)*2+1]);
		isumap[j] = reg[(j+1)*2+1];
	}
	(void)setup_distributed_openpic(baseaddr, i, (void **)isu, isumap);
	return TRUE;
#endif
	aprint_error("PIC support not present or PIC error\n");
	return FALSE;
}


/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	if (mainbus_found)
		return 0;
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ofbus_attach_args oba;
	struct confargs ca;
	int node, i;
	u_int32_t reg[4];
	char name[32];

	mainbus_found = 1;

	aprint_normal("\n");

	for (i = 0; i < 2; i++) {
		ca.ca_name = "cpu";
		ca.ca_reg = reg;
		reg[0] = i;
		config_found(self, &ca, NULL);
	}

	node = OF_peer(0);
	if (node) {
		oba.oba_busname = "ofw";
		oba.oba_phandle = node;
		config_found(self, &oba, NULL);
	}

	/* this primarily searches for pci bridges on the root bus */
	for (node = OF_child(OF_finddevice("/")); node; node = OF_peer(node)) {
		memset(name, 0, sizeof(name));
		if (OF_getprop(node, "name", name, sizeof(name)) == -1)
			continue;

		ca.ca_name = name;
		ca.ca_node = node;
		ca.ca_nreg = OF_getprop(node, "reg", reg, sizeof(reg));
		ca.ca_reg  = reg;
		config_found(self, &ca, NULL);
	}
}

void
init_interrupt(void)
{
	/* Do nothing, not ready yet */
}

void
init_ofppc_interrupt(void)
{
	int node, i, isa_cascade = 0;

	/* Now setup the PIC's */
	node = OF_finddevice("/");
	if (node <= 0)
		panic("Can't find root OFW device node\n");
	genofw_find_ofpics(node);
	genofw_fixup_picnode_offsets();
	pic_init();

	/* find ISA first */
	for (i = 0; i < nrofpics; i++) {
		if (picnodes[i].type == PICNODE_TYPE_8259) {
			aprint_debug("calling i8259 setup\n");
			isa_pic = setup_i8259();
		}
		if (picnodes[i].type == PICNODE_TYPE_IVR) {
			aprint_debug("calling prepivr setup\n");
#ifdef PIC_PREPIVR
			isa_pic = init_prepivr(picnodes[i].node);
#else
			isa_pic = setup_i8259();
#endif
		}
	}
	for (i = 0; i < nrofpics; i++) {
		if (picnodes[i].type == PICNODE_TYPE_8259)
			continue;
		if (picnodes[i].type == PICNODE_TYPE_IVR)
			continue;
		if (picnodes[i].type == PICNODE_TYPE_OPENPIC) {
			aprint_debug("calling openpic setup\n");
			if (isa_pic != NULL)
				isa_cascade = 1;
			(void)init_openpic(picnodes[i].node);
		} else
			aprint_error("Unhandled pic node type node=%x\n",
			    picnodes[i].node);
	}
	if (isa_cascade) {
		primary_pic = 1;
		intr_establish(16, IST_LEVEL, IPL_NONE, pic_handle_intr,
		    isa_pic);
	}
}
