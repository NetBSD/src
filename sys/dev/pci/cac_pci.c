/*	$NetBSD: cac_pci.c,v 1.3 2000/03/23 11:33:35 ad Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andy Doran.
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

/*
 * PCI front-end for cac(4) driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cac_pci.c,v 1.3 2000/03/23 11:33:35 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <machine/endian.h>
#include <machine/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <dev/ic/cacreg.h>
#include <dev/ic/cacvar.h>

#define	PCI_CBIO	0x10	/* Configuration base I/O address */
#define	PCI_CBMA	0x14	/* Configuration base memory address */

static int	cac_pci_match __P((struct device *, struct cfdata *, void *));
static void	cac_pci_attach __P((struct device *, struct device *, void *));

static void	cac_pci_l0_submit __P((struct cac_softc *, paddr_t));
static paddr_t	cac_pci_l0_completed __P((struct cac_softc *));
static int	cac_pci_l0_intr_pending __P((struct cac_softc *));
static void	cac_pci_l0_intr_enable __P((struct cac_softc *, int));
static int	cac_pci_l0_fifo_full __P((struct cac_softc *));

static void	cac_pci_l1_submit __P((struct cac_softc *, paddr_t));
static paddr_t	cac_pci_l1_completed __P((struct cac_softc *));
static int	cac_pci_l1_intr_pending __P((struct cac_softc *));
static void	cac_pci_l1_intr_enable __P((struct cac_softc *, int));
static int	cac_pci_l1_fifo_full __P((struct cac_softc *));

struct cfattach cac_pci_ca = {
	sizeof(struct cac_softc), cac_pci_match, cac_pci_attach
};

static struct cac_linkage cac_pci_l0 = {
	cac_pci_l0_submit,
	cac_pci_l0_completed,
	cac_pci_l0_intr_pending,
	cac_pci_l0_intr_enable,
	cac_pci_l0_fifo_full
};

static struct cac_linkage cac_pci_l1 = {
	cac_pci_l1_submit,
	cac_pci_l1_completed,
	cac_pci_l1_intr_pending,
	cac_pci_l1_intr_enable,
	cac_pci_l1_fifo_full
};

/* This block of code inspired by Compaq's Linux driver. */
struct cac_type {
	int	ct_subsysid;			/* PCI subsystem ID */
	int	ct_mmreg;			/* Memory mapped registers */
	struct	cac_linkage *ct_linkage;	/* Command interface */
	char	*ct_typestr;			/* Textual description */
} static cac_type[] = {
#ifdef notdef
	{ 0x0040110e, 0, &cac_pci_lX, "IDA" },
	{ 0x0140110e, 0, &cac_pci_lX, "IDA 2" },
	{ 0x1040110e, 0, &cac_pci_lX, "IAES" },
	{ 0x2040110e, 0, &cac_pci_lX, "SMART" },
#endif
	{ 0x3040110e, 0, &cac_pci_l0, "SMART-2/E" },
	{ 0x40300e11, 1, &cac_pci_l0, "SMART-2/P" },
	{ 0x40310e11, 1, &cac_pci_l0, "SMART-2SL" },
	{ 0x40320e11, 1, &cac_pci_l0, "Smart Array 3200" },
	{ 0x40330e11, 1, &cac_pci_l0, "Smart Array 3100ES" },
	{ 0x40340e11, 1, &cac_pci_l0, "Smart Array 221" },
	{ 0x40400e11, 1, &cac_pci_l1, "Integrated Array" },
	{ 0x40500e11, 1, &cac_pci_l1, "Smart Array 4200" },
	{ 0x40510e11, 1, &cac_pci_l1, "Smart Array 4200ES" },
	{ -1, 1, &cac_pci_l0, "array controller (unknown type)" },
};

static int
cac_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa;
	
	pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_COMPAQ && 
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_COMPAQ_SMART2P)
		return (1);

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_DEC && 
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_DEC_CPQ42XX)
		return (1);

	return (0);
}

static void
cac_pci_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pci_attach_args *pa;
	struct cac_type *ct;
	struct cac_softc *sc;
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	const char *intrstr;
	pcireg_t csr, subsysid;
	int mmreg;
	
	sc = (struct cac_softc *)self;
	pa = (struct pci_attach_args *)aux;
	pc = pa->pa_pc;
	mmreg = 0;
	
	printf(": ");

	subsysid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);

	for (ct = cac_type; ct->ct_subsysid != -1; ct++)
		if (subsysid == ct->ct_subsysid)
			break;

	if ((mmreg = ct->ct_mmreg) != 0)
		if (pci_mapreg_map(pa, PCI_CBMA, PCI_MAPREG_TYPE_MEM, 0,
		    &sc->sc_iot, &sc->sc_ioh, NULL, NULL))
		    	mmreg = 0;
	
	if (mmreg == 0)
		if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
		    &sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
			printf("can't map i/o space\n");
			return;
		}
	
	sc->sc_dmat = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("can't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, cac_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("can't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}

	/* Now attach to the bus-independent code */
	sc->sc_typestr = ct->ct_typestr;
	sc->sc_cl = ct->ct_linkage;
	cac_init(sc, intrstr);
}

static void
cac_pci_l0_submit(sc, addr)
	struct cac_softc *sc;
	paddr_t addr;
{

	cac_outl(sc, CAC_REG_CMD_FIFO, addr);
}

static paddr_t
cac_pci_l0_completed(sc)
	struct cac_softc *sc;
{

	return (cac_inl(sc, CAC_REG_DONE_FIFO));
}

static int
cac_pci_l0_intr_pending(sc)
	struct cac_softc *sc;
{

	return (cac_inl(sc, CAC_REG_INT_PENDING));
}

static void
cac_pci_l0_intr_enable(sc, state)
	struct cac_softc *sc;
	int state;
{

	cac_outl(sc, CAC_REG_INT_MASK, state);
}

static int
cac_pci_l0_fifo_full(sc)
	struct cac_softc *sc;
{

	return (cac_inl(sc, CAC_REG_CMD_FIFO) == 0);
}

static void
cac_pci_l1_submit(sc, addr)
	struct cac_softc *sc;
	paddr_t addr;
{

	cac_outl(sc, CAC_42REG_CMD_FIFO, addr);
}

static paddr_t
cac_pci_l1_completed(sc)
	struct cac_softc *sc;
{
	int32_t val;

	if ((val = cac_inl(sc, CAC_42REG_DONE_FIFO)) == -1)
		return (0);
	
	cac_outl(sc, CAC_42REG_DONE_FIFO, 0);	
	return ((paddr_t)val);
}

static int
cac_pci_l1_intr_pending(sc)
	struct cac_softc *sc;
{

	return (cac_inl(sc, CAC_42REG_INT_PENDING) & 
	    cac_inl(sc, CAC_42REG_STATUS));
}

static void
cac_pci_l1_intr_enable(sc, state)
	struct cac_softc *sc;
	int state;
{

	cac_outl(sc, CAC_REG_INT_MASK, (state ? 0 : 8));	/* XXX */
}

static int
cac_pci_l1_fifo_full(sc)
	struct cac_softc *sc;
{

	return (~cac_inl(sc, CAC_42REG_CMD_FIFO));
}
