/*	$NetBSD: if_em.c,v 1.1 1997/10/15 00:29:25 mark Exp $	*/

/*
 * Copyright (C) 1997 Mark Brinicombe
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
 *      This product includes software developed by Mark Brinicombe for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * This driver uses the generic dp8390 IC driver
 *
 * Currently supports:
 *	ANT EtherM
 */
/*#define NEW
#define DP8390_DEBUG*/

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/irqhandler.h>
#include <machine/io.h>
#include <machine/bootconfig.h>
#include <machine/katelib.h>

#include <dev/ic/dp8390reg.h>
/*#include <dev/ic/dp8390var.h>*/	/* Temporary till we merge */
#include <arch/arm32/dev/dp8390var.h>
#include <arch/arm32/podulebus/podulebus.h>
#include <arch/arm32/podulebus/podules.h>
#include <arch/arm32/podulebus/if_emreg.h>
#include <arch/arm32/dev/dp8390_pio.h>

/*
 * em_softc: per line info and status
 */
struct em_softc {
	struct	dp8390_softc	sc_dp8390;
	int			sc_podule_number;
	podule_t		*sc_podule;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct bus_space	sc_tag;
};

#define CARDNAME "EtherM"

#ifdef DP8390_DEBUG
#define	DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

static int  emprobe	__P((struct device *, struct cfdata *, void *));
static void emattach	__P((struct device *, struct device *, void *));

struct cfattach em_ca = {
	sizeof(struct em_softc), emprobe, emattach
};

struct cfdriver em_cd = {
	NULL, "em", DV_IFNET
};

static struct bus_space em_bs_tag;

/*
 * Determine if the device is present.
 */
static int
emprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *) aux;

	/* Look for a network slot interface */
	if (matchpodule(pa, MANUFACTURER_ANT, PODULE_ANT_ETHERM, -1) == 0)
		return(0);

	return(1);
}

/*
 * Install interface into kernel networking data structures.
 */
static void
emattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct em_softc *emsc = (void *)self;
	struct dp8390_softc *sc = &emsc->sc_dp8390;
	struct cfdata *cf = sc->sc_dev.dv_cfdata;
	int dsr;
	int loop;

	/* Check a few things about the attach args */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	emsc->sc_podule_number = pa->pa_podule_number;
	emsc->sc_podule = pa->pa_podule;
	podules[emsc->sc_podule_number].attached = 1;		/* XXX */

	/*
	 * Ok we need our own bus tag as the register spacing
	 * is not the default.
	 *
	 * For the podulebus the bus tag cookie is the shift
	 * to apply to registers
	 * So duplicate the bus space tag and change the
	 * cookie.
	 */

	emsc->sc_tag = *pa->pa_iot;
	emsc->sc_tag.bs_cookie = (void *) 5;
	emsc->sc_iot = &emsc->sc_tag;

	/* Map all the EtherM I/O space */
	if (bus_space_map(emsc->sc_iot, emsc->sc_podule->fast_base + EM_IO_OFFSET,
	    EM_IO_SIZE, 0, &emsc->sc_ioh))
		panic("%s: Cannot map IO space\n", self->dv_xname);

	/* Inherit flags from the config */
	sc->sc_flags = self->dv_cfdata->cf_flags;
	sc->sc_flags |= DP8390_FORCE_PIO | DP8390_FORCE_16BIT_MODE;

	/* Set up the vendor, type and name */
	sc->vendor = DP8390_VENDOR_ANT;
	sc->type = 0;
	if ((sc->type_str = malloc((u_long)(sizeof(CARDNAME) + 1), M_DEVBUF, M_NOWAIT))) {
		strncpy(sc->type_str, CARDNAME, sizeof(CARDNAME));
		sc->type_str[sizeof(CARDNAME)] = '\0';
	}

	/* Set up various softc defaults */
	sc->is790 = 0;

	/* Map register offsets */
	for (loop = 0; loop < 16; loop++)
		sc->sc_reg_map[loop] = loop;

	sc->dcr_reg = (ED_DCR_FT1 | ED_DCR_WTS | ED_DCR_LS);

	/* Map the NIC registers */
	sc->sc_regt = emsc->sc_iot;
	if (bus_space_subregion(emsc->sc_iot, emsc->sc_ioh,
	    EM_NIC_OFFSET, EM_NIC_SIZE, &sc->sc_regh)) {
		panic(" failed to map register space\n");
	}

	/* Set up the packet buffer memory */
	sc->page_offset = 0x40;
	sc->mem_start = 0x4000;
	sc->mem_size = 0x4000;
	
	/* Map the PIO data register for the buffer memory */
	sc->sc_buft = emsc->sc_iot;
	if (bus_space_subregion(emsc->sc_iot, emsc->sc_ioh,
	    EM_DATA_OFFSET, sc->mem_size, &sc->sc_bufh)) {
		panic(" failed to map register space\n");
	}

	/* Build station address from machine ID */
	sc->sc_enaddr[0] = 0x00;
	sc->sc_enaddr[1] = 0x00;
	sc->sc_enaddr[2] = 0xa4;
	sc->sc_enaddr[3] = bootconfig.machine_id[2] + 0x10;
	sc->sc_enaddr[4] = bootconfig.machine_id[1];
	sc->sc_enaddr[5] = bootconfig.machine_id[0];

	/*
	 * Override test_mem, write_mbuf,ring_copy and read_hdr functions;
	 * other defaults already work properly.
	 */
	sc->test_mem = dp8390_pio_test_mem;
	sc->write_mbuf = dp8390_pio_write_mbuf;
	sc->ring_copy = dp8390_pio_ring_copy;
	sc->read_hdr= dp8390_pio_read_hdr;

	if (dp8390_config(sc)) {
		bus_space_unmap(emsc->sc_iot, emsc->sc_ioh, EM_IO_SIZE);
		return;
	}

	/* Report packet buffer memory size */
	printf("%dKB buffer memory", sc->mem_size / 1024);

	/* Get the Diagnostic Status Register */
	dsr = bus_space_read_1(emsc->sc_iot, emsc->sc_ioh, EM_DSR_REG);

	/* Check for bits that indicate a fault */
	if (!(dsr & EM_DSR_20M))
		printf(", VCO faulty");
	if (!(dsr & EM_DSR_TCOK))
		printf(", TxClk faulty");

	/* Report status of card */
	if (dsr & EM_DSR_POL)
		printf(", UTP reverse polarity");
	if (dsr & EM_DSR_JAB)
		printf(", jabber");
	if (dsr & EM_DSR_LNK)
		printf(", link OK");
	if (dsr & EM_DSR_LBK)
		printf(", loopback");
	if (dsr & EM_DSR_UTP)
		printf(", UTP");
	printf("\n");

	/* Install an interrupt handler */
	sc->sc_ih = intr_claim(IRQ_NETSLOT, IPL_NET, "net: em", dp8390_pio_intr, sc);
	if (sc->sc_ih == NULL)
		panic("%s: Cannot install IRQ handler", self->dv_xname);
}


#if 0
static int
em_intr(arg)
	void *arg;
{
	struct dp8390_softc *sc = arg;

	if (dp8390_intr(sc, 0))
		return(1);
	return(0);
}
#endif
