/*	$NetBSD: aac_pci.c,v 1.7 2003/01/31 00:07:39 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
 * Copyright (c) 2000 Niklas Hallqvist
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from FreeBSD: aac_pci.c,v 1.1 2000/09/13 03:20:34 msmith Exp
 * via OpenBSD: aac_pci.c,v 1.7 2002/03/14 01:26:58 millert Exp
 */

/*
 * PCI front-end for the `aac' driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aac_pci.c,v 1.7 2003/01/31 00:07:39 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/endian.h>
#include <machine/intr.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/ic/aacreg.h>
#include <dev/ic/aacvar.h>

int	aac_pci_match(struct device *, struct cfdata *, void *);
void	aac_pci_attach(struct device *, struct device *, void *);
const struct	aac_ident *aac_find_ident(struct pci_attach_args *);

/* i960Rx interface */    
int	aac_rx_get_fwstatus(struct aac_softc *);
void	aac_rx_qnotify(struct aac_softc *, int);
int	aac_rx_get_istatus(struct aac_softc *);
void	aac_rx_clear_istatus(struct aac_softc *, int);
void	aac_rx_set_mailbox(struct aac_softc *, u_int32_t, u_int32_t,
			   u_int32_t, u_int32_t, u_int32_t);
int	aac_rx_get_mailboxstatus(struct aac_softc *);
void	aac_rx_set_interrupts(struct aac_softc *, int);

/* StrongARM interface */
int	aac_sa_get_fwstatus(struct aac_softc *);
void	aac_sa_qnotify(struct aac_softc *, int);
int	aac_sa_get_istatus(struct aac_softc *);
void	aac_sa_clear_istatus(struct aac_softc *, int);
void	aac_sa_set_mailbox(struct aac_softc *, u_int32_t, u_int32_t,
			   u_int32_t, u_int32_t, u_int32_t);
int	aac_sa_get_mailboxstatus(struct aac_softc *);
void	aac_sa_set_interrupts(struct aac_softc *, int);

const struct aac_interface aac_rx_interface = {
	aac_rx_get_fwstatus,
	aac_rx_qnotify,
	aac_rx_get_istatus,
	aac_rx_clear_istatus,
	aac_rx_set_mailbox,
	aac_rx_get_mailboxstatus,
	aac_rx_set_interrupts
};

const struct aac_interface aac_sa_interface = {
	aac_sa_get_fwstatus,
	aac_sa_qnotify,
	aac_sa_get_istatus,
	aac_sa_clear_istatus,
	aac_sa_set_mailbox,
	aac_sa_get_mailboxstatus,
	aac_sa_set_interrupts
};

struct aac_ident {
	u_short	vendor;
	u_short	device;
	u_short	subvendor;
	u_short	subdevice;
	u_short	hwif;
	u_short	quirks;
	const char	*prodstr;
} const aac_ident[] = {
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_2SI,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_2SI,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 2/Si"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_SUB2,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_SUB3,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_2,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_2_SUB,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
        {
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_3,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_3_SUB,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_3,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_3_SUB2,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_3,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3DI_3_SUB3,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Di"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3SI,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3SI,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Si"
	},
	{
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3SI_2,
		PCI_VENDOR_DELL,
		PCI_PRODUCT_DELL_PERC_3SI_2_SUB,
		AAC_HWIF_I960RX,
		0,
		"Dell PERC 3/Si"
	},
	{
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_AAC2622,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_AAC2622,
		AAC_HWIF_I960RX,
		0,
		"Adaptec ADP-2622"
	},
	{
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_ASR2200S,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_ASR2200S,
		AAC_HWIF_I960RX,
		0,
		"Adaptec ASR-2200S"
	},
	{
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_ASR2200S,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_ASR2120S,
		AAC_HWIF_I960RX,
		0,
		"Adaptec ASR-2120S"
	},
	{
		PCI_VENDOR_DEC,
		PCI_PRODUCT_DEC_21554,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_AAC364,
		AAC_HWIF_STRONGARM,
		0,
		"Adaptec AAC-364"
	},
	{
		PCI_VENDOR_DEC,
		PCI_PRODUCT_DEC_21554,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_ASR5400S,
		AAC_HWIF_STRONGARM,
		0,
		"Adaptec ASR-5400S"
	},
	{
		PCI_VENDOR_DEC,
		PCI_PRODUCT_DEC_21554,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_PERC_2QC,
		AAC_HWIF_STRONGARM,
		AAC_QUIRK_PERC2QC,
		"Dell PERC 2/QC"
	},
	{
		PCI_VENDOR_DEC,
		PCI_PRODUCT_DEC_21554,
		PCI_VENDOR_ADP2,
		PCI_PRODUCT_ADP2_PERC_3QC,
		AAC_HWIF_STRONGARM,
		0,
		"Dell PERC 3/QC"
	},
	{
		PCI_VENDOR_DEC,
		PCI_PRODUCT_DEC_21554,
		PCI_VENDOR_HP,
		PCI_PRODUCT_HP_NETRAID_4M,
		AAC_HWIF_STRONGARM,
		0,
		"HP NetRAID-4M"
	},
};

CFATTACH_DECL(aac_pci, sizeof(struct aac_softc),
    aac_pci_match, aac_pci_attach, NULL, NULL);

const struct aac_ident *
aac_find_ident(struct pci_attach_args *pa)
{
	const struct aac_ident *m, *mm;
	u_int32_t subsysid;

	m = aac_ident;
	mm = aac_ident + (sizeof(aac_ident) / sizeof(aac_ident[0]));

	while (m < mm) {
		if (m->vendor == PCI_VENDOR(pa->pa_id) &&
		    m->device == PCI_PRODUCT(pa->pa_id)) {
			subsysid = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    PCI_SUBSYS_ID_REG);
			if (m->subvendor == PCI_VENDOR(subsysid) &&
			    m->subdevice == PCI_PRODUCT(subsysid))
				return (m);
		}
		m++;
	}

	return (NULL);
}

int
aac_pci_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa;

	pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_I2O)
		return (0);

	return (aac_find_ident(pa) != NULL);
}

void
aac_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa;
	pci_chipset_tag_t pc;
	struct aac_softc *sc;
	u_int16_t command;
	bus_addr_t membase;
	bus_size_t memsize;
	pci_intr_handle_t ih;
	const char *intrstr;
	int state;
	const struct aac_ident *m;

	pa = aux;
	pc = pa->pa_pc;
	sc = (struct aac_softc *)self;
	state = 0;

	aprint_naive(": RAID controller\n");
	aprint_normal(": ");

	/*
	 * Verify that the adapter is correctly set up in PCI space.
	 */
	command = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	command |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);
	command = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	AAC_DPRINTF(AAC_D_MISC, ("pci command status reg 0x08x "));

	if ((command & PCI_COMMAND_MASTER_ENABLE) == 0) {
		aprint_error("can't enable bus-master feature\n");
		goto bail_out;
	}

	if ((command & PCI_COMMAND_MEM_ENABLE) == 0) {
		aprint_error("memory window not available\n");
		goto bail_out;
	}

	/*
	 * Map control/status registers.
	 */
	if (pci_mapreg_map(pa, PCI_MAPREG_START,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0, &sc->sc_memt,
	    &sc->sc_memh, &membase, &memsize)) {
		aprint_error("can't find mem space\n");
		goto bail_out;
	}
	state++;

	if (pci_intr_map(pa, &ih)) {
		aprint_error("couldn't map interrupt\n");
		goto bail_out;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, aac_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		goto bail_out;
	}
	state++;

	sc->sc_dmat = pa->pa_dmat;

	m = aac_find_ident(pa);
	aprint_normal("%s\n", m->prodstr);
	if (intrstr != NULL)
		aprint_normal("%s: interrupting at %s\n",
		    sc->sc_dv.dv_xname, intrstr);

	sc->sc_hwif = m->hwif;
	sc->sc_quirks = m->quirks;
	switch (sc->sc_hwif) {
		case AAC_HWIF_I960RX:
			AAC_DPRINTF(AAC_D_MISC,
			    ("set hardware up for i960Rx"));
			sc->sc_if = aac_rx_interface;
			break;

		case AAC_HWIF_STRONGARM:
			AAC_DPRINTF(AAC_D_MISC,
			    ("set hardware up for StrongARM"));
			sc->sc_if = aac_sa_interface;
			break;
	}

	if (!aac_attach(sc))
		return;

 bail_out:
	if (state > 1)
		pci_intr_disestablish(pc, sc->sc_ih);
	if (state > 0)
		bus_space_unmap(sc->sc_memt, sc->sc_memh, memsize);
}

/*
 * Read the current firmware status word.
 */
int
aac_sa_get_fwstatus(struct aac_softc *sc)
{

	return (AAC_GETREG4(sc, AAC_SA_FWSTATUS));
}

int
aac_rx_get_fwstatus(struct aac_softc *sc)
{

	return (AAC_GETREG4(sc, AAC_RX_FWSTATUS));
}

/*
 * Notify the controller of a change in a given queue
 */

void
aac_sa_qnotify(struct aac_softc *sc, int qbit)
{

	AAC_SETREG2(sc, AAC_SA_DOORBELL1_SET, qbit);
}

void
aac_rx_qnotify(struct aac_softc *sc, int qbit)
{

	AAC_SETREG4(sc, AAC_RX_IDBR, qbit);
}

/*
 * Get the interrupt reason bits
 */
int
aac_sa_get_istatus(struct aac_softc *sc)
{

	return (AAC_GETREG2(sc, AAC_SA_DOORBELL0));
}

int
aac_rx_get_istatus(struct aac_softc *sc)
{

	return (AAC_GETREG4(sc, AAC_RX_ODBR));
}

/*
 * Clear some interrupt reason bits
 */
void
aac_sa_clear_istatus(struct aac_softc *sc, int mask)
{

	AAC_SETREG2(sc, AAC_SA_DOORBELL0_CLEAR, mask);
}

void
aac_rx_clear_istatus(struct aac_softc *sc, int mask)
{

	AAC_SETREG4(sc, AAC_RX_ODBR, mask);
}

/*
 * Populate the mailbox and set the command word
 */
void
aac_sa_set_mailbox(struct aac_softc *sc, u_int32_t command,
		   u_int32_t arg0, u_int32_t arg1, u_int32_t arg2,
		   u_int32_t arg3)
{

	AAC_SETREG4(sc, AAC_SA_MAILBOX, command);
	AAC_SETREG4(sc, AAC_SA_MAILBOX + 4, arg0);
	AAC_SETREG4(sc, AAC_SA_MAILBOX + 8, arg1);
	AAC_SETREG4(sc, AAC_SA_MAILBOX + 12, arg2);
	AAC_SETREG4(sc, AAC_SA_MAILBOX + 16, arg3);
}

void
aac_rx_set_mailbox(struct aac_softc *sc, u_int32_t command,
		   u_int32_t arg0, u_int32_t arg1, u_int32_t arg2,
		   u_int32_t arg3)
{

	AAC_SETREG4(sc, AAC_RX_MAILBOX, command);
	AAC_SETREG4(sc, AAC_RX_MAILBOX + 4, arg0);
	AAC_SETREG4(sc, AAC_RX_MAILBOX + 8, arg1);
	AAC_SETREG4(sc, AAC_RX_MAILBOX + 12, arg2);
	AAC_SETREG4(sc, AAC_RX_MAILBOX + 16, arg3);
}

/*
 * Fetch the immediate command status word
 */
int
aac_sa_get_mailboxstatus(struct aac_softc *sc)
{

	return (AAC_GETREG4(sc, AAC_SA_MAILBOX));
}

int
aac_rx_get_mailboxstatus(struct aac_softc *sc)
{

	return (AAC_GETREG4(sc, AAC_RX_MAILBOX));
}

/*
 * Set/clear interrupt masks
 */
void
aac_sa_set_interrupts(struct aac_softc *sc, int enable)
{

	if (enable)
		AAC_SETREG2((sc), AAC_SA_MASK0_CLEAR, AAC_DB_INTERRUPTS);
	else
		AAC_SETREG2((sc), AAC_SA_MASK0_SET, ~0);
}

void
aac_rx_set_interrupts(struct aac_softc *sc, int enable)
{

	if (enable)
		AAC_SETREG4(sc, AAC_RX_OIMR, ~AAC_DB_INTERRUPTS);
	else
		AAC_SETREG4(sc, AAC_RX_OIMR, ~0);
}
