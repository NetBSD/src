/*	$NetBSD: firepower.c,v 1.16.32.1 2007/05/22 17:27:19 matt Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support routines for Firepower systems.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: firepower.c,v 1.16.32.1 2007/05/22 17:27:19 matt Exp $");

#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h> 
#include <dev/cons.h>

#include <dev/ofw/openfirm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/platform.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pio.h>

#include <ofppc/firepower/firepowerreg.h>
#include <ofppc/firepower/firepowervar.h>

void	firepower_init(void);
void	firepower_cons_init(void);
void	firepower_device_register(struct device *, void *);

extern char bootpath[];
extern char cbootpath[];

/*
 * firepower_init:
 *
 *	Platform initialization function.
 */
void
firepower_init(void)
{

	platform.cons_init = firepower_cons_init;
	platform.device_register = firepower_device_register;
	platform.softintr_init = firepower_softintr_init;

	/*
	 * Map VA==PA the region that includes ISA I/O, PCI Config,
	 * and PCI I/O.
	 *
	 * XXX This is actually a lot larger than 256M.
	 */
	oea_iobat_add(0x80000000, BAT_BL_256M);

	/*
	 * Map VA==PA the region that includes PCI Memory.
	 *
	 * XXX This is actually a lot larger than 256M.
	 */
	oea_iobat_add(0xc0000000, BAT_BL_256M);

	/*
	 * Map VA==PA the region that includes the System registers.
	 */
	oea_iobat_add(0xf0000000, BAT_BL_256M);

	firepower_intr_init();
}

/*
 * firepower_cons_init:
 *
 *	Initialize the console on a Firepower system.
 */
void
firepower_cons_init(void)
{
}

/*
 * firepower_device_register:
 *
 *	A device has been attached; notify machine-dependent code.
 *	We use this to find the boot device.
 */
void
firepower_device_register(struct device *dev, void *aux)
{
	static struct device *parent;
	static char *bp = bootpath + 1, *cp = cbootpath + 1;
	unsigned long addr;
	char *pnext, *paddr;
	int clen;

	if (booted_device)
		return;

	/* Skip over devices not represented in the OF tree. */
	if (device_is_a(dev, "mainbus")) {
		parent = dev;
		return;
	}
	if (device_is_a(dev, "atapibus") || device_is_a(dev, "scsibus"))
		return;

	if (device_is_a(device_parent(dev), "atapibus") ||
	    device_is_a(device_parent(dev), "scsibus")) {
		if (device_parent(device_parent(dev)) != parent)
			return;
	} else if (device_parent(dev) != parent) {
		return;
	}

	/*
	 * Get the address part of the current path component.
	 */
	pnext = strchr(cp, '/');
	if (pnext) {
		clen = pnext - cp;
		pnext++;
	} else {
		clen = strlen(cp);
	}
	addr = 0;
	paddr = strchr(cp, '@');
	if (pnext && paddr > pnext) {
		paddr = NULL;
	} else if (!paddr && bp) {
		paddr = strchr(bp, '@');
	}
	if (paddr) {
		addr = strtoul(paddr + 1, NULL, 0x10);
	}

	if (device_is_a(device_parent(dev), "mainbus")) {
		struct ofbus_attach_args *oba = aux;
		
		if (strcmp(oba->oba_busname, "cpu") == 0)
			return;
	} else if (device_is_a(device_parent(dev), "ofbus")) {
		struct ofbus_attach_args *oba = aux;

		if (strncmp(oba->oba_ofname, cp, clen))
			return;
	} else if (device_is_a(device_parent(dev), "pci")) {
		struct pci_attach_args *pa = aux;

		if (addr != pa->pa_device)
			return;
	} else if (device_is_a(device_parent(dev), "scsibus") ||
		   device_is_a(device_parent(dev), "atapibus")) {
		struct scsipibus_attach_args *sa = aux;

		/* periph_target is target for scsi, drive # for atapi */
		if (addr != sa->sa_periph->periph_target)
			return;
	} else
		return;

	/*
	 * If we reach this point, then dev is a match for the current
	 * path component.
	 */

	if (pnext && *pnext) {
		parent = dev;
		cp = pnext;
		bp = strchr(bp, '/');
		if (bp)
			bp++;
		return;
	} else {
		booted_device = dev;
		return;
	}
}

/*
 * XXX We may want to move this stuff into a separate "Powerhouse"
 * driver at some point.
 */

int	firepower_match(struct device *, struct cfdata *, void *);
void	firepower_attach(struct device *, struct device *, void *);

CFATTACH_DECL(firepower, sizeof(struct firepower_softc),
    firepower_match, firepower_attach, NULL, NULL);

/* There can be only one. */
struct firepower_config firepower_configuration;
int firepower_found;

/*
 * firepower_chipset_init:
 *
 *	Set up the Firepower's chipset function pointers.
 */
void
firepower_chipset_init(struct firepower_config *cp, int mallocsafe)
{

	cp->c_mallocsafe = mallocsafe;

	cp->c_type = CSR_READ2(FPR_PCI_DEVICE_ID);

	if (cp->c_initted == 0) {
		/* don't do these twice since they set up extents */
		firepower_bus_io_init(&cp->c_iot, cp);
		firepower_bus_mem_init(&cp->c_memt, cp);
	}
	cp->c_mallocsafe = mallocsafe;

	firepower_pci_init(&cp->c_pc, cp);
	firepower_pci_intr_init(&cp->c_pc, cp);

	cp->c_initted = 1;
}

int
firepower_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	char name[32];

	switch (platid) {
	case PLATID_FIREPOWER_ES:
	case PLATID_FIREPOWER_MX:
	case PLATID_FIREPOWER_LX:
		break;

	default:
		return (0);
	}

	if (strcmp(oba->oba_busname, "ofw") != 0)
		return (0);

	if (OF_getprop(oba->oba_phandle, "device_type", name,
	    sizeof(name)) <= 3)
		return (0);

	if (strcmp(name, "pci") == 0) {
		if (firepower_found)
			panic("firepower_match: already found?");
		return (10);	/* beat ofbus */
	}

	return (0);
}

void
firepower_attach(struct device *parent, struct device *self, void *aux)
{
	struct firepower_softc *sc = (void *) self;
	struct firepower_config *cp;
	const char *name;
	char namestore[sizeof("0xXXXXXXXX")];
#if NPCI > 0
	struct pcibus_attach_args pba;
#endif

	/* Note that we've attached the chipset; can't have two. */
	firepower_found = 1;

	/*
	 * Set up the chipset's info; done once at console init time
	 * (maybe), but doesn't hurt to do it twice.
	 */
	cp = sc->sc_cp = &firepower_configuration;
	firepower_chipset_init(cp, 1);

	switch (cp->c_type) {
	case PCI_PRODUCT_POWERHOUSE_POWERPRO:
		name = "PowerPro";
		break;

	case PCI_PRODUCT_POWERHOUSE_POWERTOP:
		name = "PowerTop";
		break;

	default:
		sprintf(namestore, "0x%08x", cp->c_type);
		name = namestore;
		break;
	}

	printf(": Powerhouse %s Core Logic chipset, rev. %u\n",
	    name, CSR_READ1(FPR_PCI_REVISION));

	firepower_dma_init(cp);

#if NPCI > 0
	pba.pba_iot = &cp->c_iot;
	pba.pba_memt = &cp->c_memt;
	pba.pba_dmat = &cp->c_dmat_pci;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &cp->c_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	(void) config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif /* NPCI > 0 */
}
