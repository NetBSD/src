/*	$NetBSD: firepower.c,v 1.2 2002/05/16 01:01:39 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h> 
#include <dev/cons.h>

#include <dev/ofw/openfirm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/bat.h>
#include <machine/intr.h>
#include <machine/platform.h>

#include <powerpc/pio.h>

#include <ofppc/firepower/firepowerreg.h>
#include <ofppc/firepower/firepowervar.h>

void	firepower_init(void);
void	firepower_cons_init(void);
void	firepower_device_register(struct device *, void *);

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

	/*
	 * Map VA==PA the region that includes ISA I/O, PCI Config,
	 * and PCI I/O.
	 *
	 * XXX This is actually a lot larger than 256M.
	 */
	battable[0x8].batl = BATL(0x80000000, BAT_I, BAT_PP_RW);
	battable[0x8].batu = BATU(0x80000000, BAT_BL_256M, BAT_Vs);

	/*
	 * Map VA==PA the region that includes PCI Memory.
	 *
	 * XXX This is actually a lot larger than 256M.
	 */
	battable[0xc].batl = BATL(0xc0000000, BAT_I, BAT_PP_RW);
	battable[0xc].batu = BATU(0xc0000000, BAT_BL_256M, BAT_Vs);

	/*
	 * Map VA==PA the region that includes the System registers.
	 */
	battable[0xf].batl = BATL(0xf0000000, BAT_I, BAT_PP_RW);
	battable[0xf].batu = BATU(0xf0000000, BAT_BL_256M, BAT_Vs);

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
}

/*
 * XXX We may want to move this stuff into a separate "Powerhouse"
 * driver at some point.
 */

int	firepower_match(struct device *, struct cfdata *, void *);
void	firepower_attach(struct device *, struct device *, void *);

struct cfattach firepower_ca = {
	sizeof(struct firepower_softc), firepower_match, firepower_attach,
};

int	firepower_print(void *, const char *);

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
	struct pcibus_attach_args pba;
	const char *name;
	char namestore[sizeof("0xXXXXXXXX")];

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

	pba.pba_busname = "pci";
	pba.pba_iot = &cp->c_iot;
	pba.pba_memt = &cp->c_memt;
	pba.pba_dmat = &cp->c_dmat_pci;
	pba.pba_pc = &cp->c_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	(void) config_found(self, &pba, firepower_print);
}

int
firepower_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
