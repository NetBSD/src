/*	$NetBSD: if_esh_pci.c,v 1.20 2006/10/12 01:31:29 christos Exp $	*/

/*
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code contributed to The NetBSD Foundation by Kevin M. Lahey
 * of the Numerical Aerospace Simulation Facility, NASA Ames Research
 * Center.
 *
 * Partially based on a HIPPI driver written by Essential Communications
 * Corporation.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: if_esh_pci.c,v 1.20 2006/10/12 01:31:29 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_hippi.h>
#include <net/if_media.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/rrunnerreg.h>
#include <dev/ic/rrunnervar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI constants.
 * XXX These should be in a common file!
 */
#define PCI_CONN		0x48    /* Connector type */
#define PCI_CBIO		0x10    /* Configuration Base IO Address */

#define MEM_MAP_REG	0x10

static int	esh_pci_match(struct device *, struct cfdata *, void *);
static void	esh_pci_attach(struct device *, struct device *, void *);
static u_int8_t	esh_pci_bist_read(struct esh_softc *);
static void	esh_pci_bist_write(struct esh_softc *, u_int8_t);


CFATTACH_DECL(esh_pci, sizeof(struct esh_softc),
    esh_pci_match, esh_pci_attach, NULL, NULL);

static int
esh_pci_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_ESSENTIAL)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_ESSENTIAL_RR_HIPPI:
	case PCI_PRODUCT_ESSENTIAL_RR_GIGE:
		break;
	default:
		return 0;
	}
	return 1;
}

static void
esh_pci_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct esh_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *model;
	const char *intrstr = NULL;

	aprint_naive(": HIPPI controller\n");

	if (pci_mapreg_map(pa, MEM_MAP_REG,
			   PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
			   &sc->sc_iot, &sc->sc_ioh, NULL, NULL) != 0) {
	    aprint_error(": unable to map memory device registers\n");
	    return;
	}

	sc->sc_dmat = pa->pa_dmat;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_ESSENTIAL_RR_HIPPI:
		model = "RoadRunner HIPPI";
		break;
	case PCI_PRODUCT_ESSENTIAL_RR_GIGE:
		model = "RoadRunner Gig-E";
		break;
	default:
		model = "unknown model";
		break;
	}

	aprint_normal(": %s\n", model);

	sc->sc_bist_read = esh_pci_bist_read;
	sc->sc_bist_write = esh_pci_bist_write;

	eshconfig(sc);

	/* Enable the card. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: couldn't map interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, eshintr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);
}

static u_int8_t
esh_pci_bist_read(struct esh_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t pci_bist;

	pci_bist = bus_space_read_4(iot, ioh, RR_PCI_BIST);

	return ((u_int8_t) (pci_bist >> 24));
}

static void
esh_pci_bist_write(struct esh_softc *sc, u_int8_t value)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int32_t pci_bist;
	u_int32_t new_bist;

	pci_bist = bus_space_read_4(iot, ioh, RR_PCI_BIST);
	new_bist = ((u_int32_t) value << 24) | (pci_bist & 0x00ffffff);

	bus_space_write_4(iot, ioh, RR_PCI_BIST, new_bist);
}
