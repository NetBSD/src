/*	$NetBSD: uha_eisa.c,v 1.15 2000/03/23 07:01:28 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/eisa/eisavar.h>
#include <dev/eisa/eisadevs.h>

#include <dev/ic/uhareg.h>
#include <dev/ic/uhavar.h>

#define	UHA_EISA_SLOT_OFFSET	0xc80
#define	UHA_EISA_IOSIZE		0x020

int	uha_eisa_match __P((struct device *, struct cfdata *, void *));
void	uha_eisa_attach __P((struct device *, struct device *, void *));

struct cfattach uha_eisa_ca = {
	sizeof(struct uha_softc), uha_eisa_match, uha_eisa_attach
};

#ifndef	DDB
#define Debugger() panic("should call debugger here (uha_eisa.c)")
#endif /* ! DDB */

int	u24_find __P((bus_space_tag_t, bus_space_handle_t,
	    struct uha_probe_data *));
void	u24_start_mbox __P((struct uha_softc *, struct uha_mscp *));
int	u24_poll __P((struct uha_softc *, struct scsipi_xfer *, int));
int	u24_intr __P((void *));
void	u24_init __P((struct uha_softc *));

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
uha_eisa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct eisa_attach_args *ea = aux;
	bus_space_tag_t iot = ea->ea_iot;
	bus_space_handle_t ioh;
	int rv;

	/* must match one of our known ID strings */
	if (strncmp(ea->ea_idstring, "USC024", 6))
		return (0);

	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot) +
	    UHA_EISA_SLOT_OFFSET, UHA_EISA_IOSIZE, 0, &ioh))
		return (0);

	rv = u24_find(iot, ioh, NULL);

	bus_space_unmap(iot, ioh, UHA_EISA_IOSIZE);

	return (rv);
}

/*
 * Attach all the sub-devices we can find
 */
void
uha_eisa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct eisa_attach_args *ea = aux;
	struct uha_softc *sc = (void *)self;
	bus_space_tag_t iot = ea->ea_iot;
	bus_dma_tag_t dmat = ea->ea_dmat;
	bus_space_handle_t ioh;
	struct uha_probe_data upd;
	eisa_chipset_tag_t ec = ea->ea_ec;
	eisa_intr_handle_t ih;
	const char *model, *intrstr;

	if (!strncmp(ea->ea_idstring, "USC024", 6))
		model = EISA_PRODUCT_USC0240;
	else
		model = "unknown model!";
	printf(": %s\n", model);

	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot) +
	    UHA_EISA_SLOT_OFFSET, UHA_EISA_IOSIZE, 0, &ioh))
		panic("uha_eisa_attach: could not map I/O addresses");

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = dmat;
	if (!u24_find(iot, ioh, &upd))
		panic("uha_eisa_attach: u24_find failed!");

	sc->sc_dmaflags = 0;

	if (eisa_intr_map(ec, upd.sc_irq, &ih)) {
		printf("%s: couldn't map interrupt (%d)\n",
		    sc->sc_dev.dv_xname, upd.sc_irq);
		return;
	}
	intrstr = eisa_intr_string(ec, ih);
	sc->sc_ih = eisa_intr_establish(ec, ih, IST_LEVEL, IPL_BIO,
	    u24_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* Save function pointers for later use. */
	sc->start_mbox = u24_start_mbox;
	sc->poll = u24_poll;
	sc->init = u24_init;

	uha_attach(sc, &upd);
}

int
u24_find(iot, ioh, sc)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct uha_probe_data *sc;
{
	u_int8_t config0, config1, config2;
	int irq, drq;
	int resetcount = 4000;	/* 4 secs? */

	config0 = bus_space_read_1(iot, ioh, U24_CONFIG + 0);
	config1 = bus_space_read_1(iot, ioh, U24_CONFIG + 1);
	config2 = bus_space_read_1(iot, ioh, U24_CONFIG + 2);
	if ((config0 & U24_MAGIC1) == 0 ||
	    (config1 & U24_MAGIC2) == 0)
		return (0);

	drq = -1;

	switch (config0 & U24_IRQ_MASK) {
	case U24_IRQ10:
		irq = 10;
		break;
	case U24_IRQ11:
		irq = 11;
		break;
	case U24_IRQ14:
		irq = 14;
		break;
	case U24_IRQ15:
		irq = 15;
		break;
	default:
		printf("u24_find: illegal irq setting %x\n",
		    config0 & U24_IRQ_MASK);
		return (0);
	}

	bus_space_write_1(iot, ioh, U24_LINT, UHA_ASRST);

	while (--resetcount) {
		if (bus_space_read_1(iot, ioh, U24_LINT))
			break;
		delay(1000);	/* 1 mSec per loop */
	}
	if (!resetcount) {
		printf("u24_find: board timed out during reset\n");
		return (0);
	}

	/* if we want to fill in softc, do so now */
	if (sc) {
		sc->sc_irq = irq;
		sc->sc_drq = drq;
		sc->sc_scsi_dev = config2 & U24_HOSTID_MASK;
	}

	return (1);
}

void
u24_start_mbox(sc, mscp)
	struct uha_softc *sc;
	struct uha_mscp *mscp;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int spincount = 100000;	/* 1s should be enough */

	while (--spincount) {
		if ((bus_space_read_1(iot, ioh, U24_LINT) & U24_LDIP) == 0)
			break;
		delay(100);
	}
	if (!spincount) {
		printf("%s: uha_start_mbox, board not responding\n",
		    sc->sc_dev.dv_xname);
		Debugger();
	}

	bus_space_write_4(iot, ioh, U24_OGMPTR,
	    sc->sc_dmamap_mscp->dm_segs[0].ds_addr + UHA_MSCP_OFF(mscp));
	if (mscp->flags & MSCP_ABORT)
		bus_space_write_1(iot, ioh, U24_OGMCMD, 0x80);
	else
		bus_space_write_1(iot, ioh, U24_OGMCMD, 0x01);
	bus_space_write_1(iot, ioh, U24_LINT, U24_OGMFULL);

	if ((mscp->xs->xs_control & XS_CTL_POLL) == 0)
		callout_reset(&mscp->xs->xs_callout,
		    (mscp->timeout * hz) / 1000, uha_timeout, mscp);
}

int
u24_poll(sc, xs, count)
	struct uha_softc *sc;
	struct scsipi_xfer *xs;
	int count;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	while (count) {
		/*
		 * If we had interrupts enabled, would we
		 * have got an interrupt?
		 */
		if (bus_space_read_1(iot, ioh, U24_SINT) & U24_SDIP)
			u24_intr(sc);
		if (xs->xs_status & XS_STS_DONE)
			return (0);
		delay(1000);
		count--;
	}
	return (1);
}

int
u24_intr(arg)
	void *arg;
{
	struct uha_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct uha_mscp *mscp;
	u_char uhastat;
	u_long mboxval;

#ifdef	UHADEBUG
	printf("%s: uhaintr ", sc->sc_dev.dv_xname);
#endif /*UHADEBUG */

	if ((bus_space_read_1(iot, ioh, U24_SINT) & U24_SDIP) == 0)
		return (0);

	for (;;) {
		/*
		 * First get all the information and then
		 * acknowledge the interrupt
		 */
		uhastat = bus_space_read_1(iot, ioh, U24_SINT);
		mboxval = bus_space_read_4(iot, ioh, U24_ICMPTR);
		bus_space_write_1(iot, ioh, U24_SINT, U24_ICM_ACK);
		bus_space_write_1(iot, ioh, U24_ICMCMD, 0);

#ifdef	UHADEBUG
		printf("status = 0x%x ", uhastat);
#endif /*UHADEBUG*/

		/*
		 * Process the completed operation
		 */
		mscp = uha_mscp_phys_kv(sc, mboxval);
		if (!mscp) {
			printf("%s: BAD MSCP RETURNED!\n",
			    sc->sc_dev.dv_xname);
			continue;	/* whatever it was, it'll timeout */
		}
		callout_stop(&mscp->xs->xs_callout);
		uha_done(sc, mscp);

		if ((bus_space_read_1(iot, ioh, U24_SINT) & U24_SDIP) == 0)
			return (1);
	}
}

void
u24_init(sc)
	struct uha_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* free OGM and ICM */
	bus_space_write_1(iot, ioh, U24_OGMCMD, 0);
	bus_space_write_1(iot, ioh, U24_ICMCMD, 0);
	/* make sure interrupts are enabled */
#ifdef UHADEBUG
	printf("u24_init: lmask=%02x, smask=%02x\n",
	    bus_space_read_1(iot, ioh, U24_LMASK),
	    bus_space_read_1(iot, ioh, U24_SMASK));
#endif
	bus_space_write_1(iot, ioh, U24_LMASK, 0xd2);	/* XXX */
	bus_space_write_1(iot, ioh, U24_SMASK, 0x92);	/* XXX */
}
