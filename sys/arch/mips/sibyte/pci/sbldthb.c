/* $NetBSD: sbldthb.c,v 1.1.2.1 2010/01/21 04:22:33 matt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 * 
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 * 
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 * 
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 * 
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* from: $NetBSD: ppb.c,v 1.19 1999/11/04 19:04:04 thorpej Exp */

/*
 * Copyright (c) 1996, 1998 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

/*
 * Driver for SB-1250 LDT Host Bridge.
 *
 * Since the LDT Host Bridge configuration space doesn't have the
 * class/subclass of a PCI-PCI bridge, we have to do the bridging
 * specially here.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>
#include <dev/pci/pcidevs.h>

static int	sbldthb_match(device_t, cfdata_t, void *);
static void	sbldthb_attach(device_t, device_t, void *);
#ifdef not_likely
static int	sbldthb_print(void *, const char *pnp);
#endif

CFATTACH_DECL_NEW(sbldthb, 0, sbldthb_match, sbldthb_attach, NULL, NULL);


int
sbldthb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/* Check for a vendor/device match.  */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SIBYTE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SIBYTE_BCM1250_LDTHB)
		return (1);

	return (0);
}

void
sbldthb_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
//	struct pcibus_attach_args pba;
	pcireg_t busdata;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

printf("\tpc: %"PRIxPTR"\n", (uintptr_t)pc);
printf("\tpa: %"PRIxPTR"\n", (uintptr_t)pa);
printf("\tpa->pa_tag: %"PRIxPTR"\n", (uintptr_t)(pa->pa_tag));
printf("\tPPB_REG_BUSINFO %x\n", PPB_REG_BUSINFO);

printf("Offset 0: ");
busdata = pci_conf_read(pc, pa->pa_tag, 0x0);
printf("Offset 8: ");
busdata = pci_conf_read(pc, pa->pa_tag, 0x8);
printf("Offset 20: ");
busdata = pci_conf_read(pc, pa->pa_tag, 0x20);
printf("Offset 30: ");
busdata = pci_conf_read(pc, pa->pa_tag, 0x30);

pci_conf_write(pc, pa->pa_tag, 0x20, 0x51005000); // 5000_0000 - 5100_0000
pci_conf_write(pc, pa->pa_tag, 0x30, 0x01100100); // DD00_0000 - DD10_0000

	busdata = pci_conf_read(pc, pa->pa_tag, PPB_REG_BUSINFO);
printf("\tbusdata is: %"PRIx32"\n", busdata);


	if (PPB_BUSINFO_PRIMARY(busdata) == 0 &&
	    PPB_BUSINFO_SECONDARY(busdata) == 0) {
		aprint_error_dev(self, "not configured by system firmware\n");
		return;
	}

#if 0
	/*
	 * XXX can't do this, because we're not given our bus number
	 * (we shouldn't need it), and because we've no way to
	 * decompose our tag.
	 */
	/* sanity check. */
	if (pa->pa_bus != PPB_BUSINFO_PRIMARY(busdata))
		panic("sbldthbattach: bus in tag (%d) != bus in reg (%d)",
		    pa->pa_bus, PPB_BUSINFO_PRIMARY(busdata));
#endif
#if not_likely

	/*
	 * Attach the PCI bus than hangs off of it.
	 *
	 * XXX Don't pass-through Memory Read Multiple.  Should we?
	 * XXX Consult the spec...
	 */
//	pba.pba_busname = "pci";	/* XXX should be pci_ppb attachment */
	pba.pba_iot = pa->pa_iot;
	pba.pba_memt = pa->pa_memt;
	pba.pba_dmat64 = pa->pa_dmat64;				/* XXXLDT??? */
	pba.pba_dmat = pa->pa_dmat;				/* XXXLDT??? */
	pba.pba_pc = pc;					/* XXXLDT??? */
	pba.pba_flags = pa->pa_flags & ~PCI_FLAGS_MRM_OKAY;	/* XXXLDT??? */
	pba.pba_bus = PPB_BUSINFO_SECONDARY(busdata);
	pba.pba_intrswiz = pa->pa_intrswiz;			/* XXXLDT??? */
	pba.pba_intrtag = pa->pa_intrtag;			/* XXXLDT??? */

	config_found(self, &pba, sbldthb_print);
#endif
}

#ifdef not_likely
int
sbldthb_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to PPBs; easy. */
	if (pnp)
		printf("pci at %s", pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}
#endif
