/*	$NetBSD: if_ntwoc_pci.c,v 1.2 1999/02/15 04:54:35 hubertf Exp $	*/

/*
 * Copyright (c) 1998 Vixie Enterprises
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Vixie Enterprises nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY VIXIE ENTERPRISES AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL VIXIE ENTERPRISES OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for Vixie Enterprises by Michael Graff
 * <explorer@flame.org>.  To learn more about Vixie Enterprises, see
 * ``http://www.vix.com''.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/socket.h>

#include <net/if.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/hd64570reg.h>
#include <dev/ic/hd64570var.h>

#include <dev/pci/if_ntwoc_pcireg.h>

#if 0
#define NTWO_DEBUG
#endif

#ifdef NTWO_DEBUG
#define NTWO_DPRINTF(x) printf x
#else
#define NTWO_DPRINTF(x)
#endif

/*
 * Card specific config register location
 */
#define PCI_CBMA_ASIC	0x10	/* Configuration Base Memory Address */
#define PCI_CBMA_SCA	0x18

struct ntwoc_pci_softc {
	/* Generic device stuff */
	struct device sc_dev;		/* Common to all devices */

	/* PCI chipset glue */
	pci_intr_handle_t *sc_ih;	/* Interrupt handler */
	pci_chipset_tag_t sc_sr;	/* PCI chipset handle */

	bus_space_tag_t sc_asic_iot;	/* space cookie (for ASIC) */
	bus_space_handle_t sc_asic_ioh;	/* bus space handle (for ASIC) */

	struct sca_softc sc_sca;	/* the SCA itself */
};

static  int ntwoc_pci_match __P((struct device *, struct cfdata *, void *));
static  void ntwoc_pci_attach __P((struct device *, struct device *, void *));

static	int ntwoc_intr __P((void *));
static	void ntwoc_shutdown __P((void *sc));
static	void ntwoc_dtr_callback __P((void *, int, int));

struct cfattach ntwoc_pci_ca = {
  sizeof(struct ntwoc_pci_softc), ntwoc_pci_match, ntwoc_pci_attach,
};

/*
 * Names for daughter card types.  These match the NTWOC_DB_* defines.
 */
char *ntwoc_db_names[] = {
	"V.35", "Unknown 0x01", "Test", "Unknown 0x03",
	"RS232", "Unknown 0x05", "RS422", "None"
};

static int
ntwoc_pci_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RISCOM)
	    && (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RISCOM_N2))
		return 1;

	return 0;
}

static void
ntwoc_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ntwoc_pci_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	struct sca_softc *sca = &sc->sc_sca;
	pci_intr_handle_t ih;
	const char *intrstr;
	pcireg_t csr;
	u_int16_t frontend_cr;
	u_int16_t db0, db1;
	u_int numports;

	printf(": N2 Serial Interface\n");

	/*
	 * Map in the ASIC configuration space
	 */
	if (pci_mapreg_map(pa, PCI_CBMA_ASIC, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->sc_asic_iot, &sc->sc_asic_ioh, NULL, NULL)) {
		printf("%s: Can't map register space (ASIC)\n",
		       sc->sc_dev.dv_xname);
		return;
	}
	/*
	 * Map in the serial controller configuration space
	 */
	if (pci_mapreg_map(pa, PCI_CBMA_SCA, PCI_MAPREG_TYPE_MEM, 0,
			   &sca->sc_iot, &sca->sc_ioh, NULL, NULL)) {
		printf("%s: Can't map register space (SCA)\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Enable the card
	 */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, csr);

	/*
	 * Map and establish the interrupt
	 */
	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
			 pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_NET, ntwoc_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		       sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * Perform total black magic.  This is not only extremely
	 * disgusting, but it should be explained a lot more in the
	 * card's documentation.
	 *
	 * From what I gather, this does nothing more than configure the
	 * PCI to ISA translator ASIC the N2pci card uses.
	 *
	 * From the FreeBSD driver:
	 * offset
	 *  0x00 - Map Range    - Mem-mapped to locate anywhere
	 *  0x04 - Re-Map       - PCI address decode enable
	 *  0x18 - Bus Region   - 32-bit bus, ready enable
	 *  0x1c - Master Range - include all 16 MB
	 *  0x20 - Master RAM   - Map SCA Base at 0
	 *  0x28 - Master Remap - direct master memory enable
	 *  0x68 - Interrupt    - Enable interrupt (0 to disable)
	 */
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x00, 0xfffff000);
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x04, 1);
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x18, 0x40030043);
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x1c, 0xff000000);
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x20, 0);
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x28, 0xe9);
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x68, 0x10900);

	/*
	 * pass the dma tag to the SCA
	 */
	sca->sc_dmat = pa->pa_dmat;

	/*
	 * Read the configuration information off the daughter card.
	 */
	frontend_cr = bus_space_read_2(sca->sc_iot, sca->sc_ioh, NTWOC_FECR);
	NTWO_DPRINTF(("%s: frontend_cr = 0x%04x\n",
		      sc->sc_dev.dv_xname, frontend_cr));

	db0 = (frontend_cr & NTWOC_FECR_ID0) >> NTWOC_FECR_ID0_SHIFT;
	db1 = (frontend_cr & NTWOC_FECR_ID1) >> NTWOC_FECR_ID1_SHIFT;

	/*
	 * Port 1 HAS to be present.  If it isn't, don't attach anything.
	 */
	if (db0 == NTWOC_FE_ID_NONE) {
		printf("%s: no ports available\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Port 1 is present.  Now, check to see if port 2 is also
	 * present.
	 */
	numports = 1;
	if (db1 != NTWOC_FE_ID_NONE)
		numports++;

	printf("%s: %d port%s\n", sc->sc_dev.dv_xname, numports,
	       (numports > 1 ? "s" : ""));
	printf("%s: port 0 interface card: %s\n", sc->sc_dev.dv_xname,
	       ntwoc_db_names[db0]);
	if (numports > 1)
		printf("%s: port 1 interface card: %s\n", sc->sc_dev.dv_xname,
		       ntwoc_db_names[db1]);

	/*
	 * enable the RS422 tristate transmit
	 * diable clock output (use receiver clock for both)
	 */
	frontend_cr |= (NTWOC_FECR_TE0 | NTWOC_FECR_TE1);
	frontend_cr &= ~(NTWOC_FECR_ETC0 | NTWOC_FECR_ETC1);
	bus_space_write_2(sc->sc_sca.sc_iot, sc->sc_sca.sc_ioh,
			  NTWOC_FECR, frontend_cr);

	/*
	 * initialize the SCA.  This will allocate DMAable memory based
	 * on the number of ports we passed in, the size of each
	 * buffer, and the number of buffers per port.
	 */
	sca->parent = &sc->sc_dev;
	sca->dtr_callback = ntwoc_dtr_callback;
	sca->dtr_aux = sc;
	sca_init(sca, numports);

	/*
	 * always initialize port 0, since we have to have found it to
	 * get this far.  If we have two ports, attach the second
	 * as well.
	 */
	sca_port_attach(sca, 0);
	if (numports == 2)
		sca_port_attach(sca, 1);

	/*
	 * Add shutdown hook so that DMA is disabled prior to reboot. Not
	 * doing do could allow DMA to corrupt kernel memory during the
	 * reboot before the driver initializes.
	 */
	shutdownhook_establish(ntwoc_shutdown, sc);
}

static int
ntwoc_intr(void *arg)
{
	struct ntwoc_pci_softc *sc = (struct ntwoc_pci_softc *)arg;

	return sca_hardintr(&sc->sc_sca);
}

/*
 * shut down interrupts and DMA, so we don't trash the kernel on warm
 * boot.  Also, lower DTR on each port and disable card interrupts.
 */
static void
ntwoc_shutdown(void *aux)
{
	struct ntwoc_pci_softc *sc = aux;
	u_int16_t fecr;

	/*
	 * shut down the SCA ports
	 */
	sca_shutdown(&sc->sc_sca);

	/*
	 * disable interupts for the whole card.  Black magic, see comment
	 * above.
	 */
	bus_space_write_4(sc->sc_asic_iot, sc->sc_asic_ioh,
			  0x68, 0x10900);

	/*
	 * lower DTR on both ports
	 */
	fecr = bus_space_read_2(sc->sc_sca.sc_iot,
				sc->sc_sca.sc_ioh, NTWOC_FECR);
	fecr |= (NTWOC_FECR_DTR0 | NTWOC_FECR_DTR1);
	bus_space_write_2(sc->sc_sca.sc_iot, sc->sc_sca.sc_ioh,
			  NTWOC_FECR, fecr);
}

static void
ntwoc_dtr_callback(void *aux, int port, int state)
{
	struct ntwoc_pci_softc *sc = aux;
	u_int16_t fecr;

	fecr = bus_space_read_2(sc->sc_sca.sc_iot,
				sc->sc_sca.sc_ioh, NTWOC_FECR);

	NTWO_DPRINTF(("port == %d, state == %d, old fecr:  0x%04x\n",
		       port, state, fecr));

	if (port == 0) {
		if (state == 0)
			fecr |= NTWOC_FECR_DTR0;
		else
			fecr &= ~NTWOC_FECR_DTR0;
	} else {
		if (state == 0)
			fecr |= NTWOC_FECR_DTR1;
		else
			fecr &= ~NTWOC_FECR_DTR1;
	}

	NTWO_DPRINTF(("new fecr:  0x%04x\n", fecr));

	bus_space_write_2(sc->sc_sca.sc_iot, sc->sc_sca.sc_ioh,
			  NTWOC_FECR, fecr);
}
