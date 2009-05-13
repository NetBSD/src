/* $NetBSD: gsckbc.c,v 1.3.60.1 2009/05/13 17:17:43 jym Exp $ */
/*
 * Copyright (c) 2004 Jochen Kunz.
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
 * 3. The name of Jochen Kunz may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOCHEN KUNZ
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL JOCHEN KUNZ
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * hp700 GSC bus MD pckbport(9) frontend for the PS/2 ports found in LASI chips.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gsckbc.c,v 1.3.60.1 2009/05/13 17:17:43 jym Exp $");

/* autoconfig and device stuff */
#include <sys/param.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>
#include <hp700/hp700/machdep.h>
#include "locators.h"
#include "ioconf.h"

/* bus_space / bus_dma etc. */
#include <machine/bus.h>
#include <machine/intr.h>

/* general system data and functions */
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/types.h>

/* pckbport interface */
#include <dev/pckbport/pckbportvar.h>

/* register offsets */
#define	REG_ID		0x0	/* R: ID register */
#define	REG_RESET	0x0	/* W: reset port */
#define	REG_RCVDATA	0x4	/* R: received data (4 byte FIFO) */
#define	REG_XMITDATA	0x4	/* W: data to transmit */
#define	REG_CONTROL	0x8	/* Controll Bits */
#define	REG_STATUS	0xc	/* Status Bits */
#define	REG_SZ		0xc	/* Size of register set */
#define	REG_OFFSET	0x100	/* Address Offset of the two ports */

/* ID values for REG_ID */
#define	ID_KBD		0	/* keyboard port */
#define	ID_AUX		1	/* mouse / aux port */

/* Control Register Bits (R/W) */
#define	CTRL_ENBL	0x01	/* Enable */
#define	CTRL_LPBXR	0x02	/* Loopback Xmt/Rcv mode */
#define	CTRL_DIAG	0x20	/* Diagnostic mode */
#define	CTRL_DATDIR	0x40	/* External data line direct control */
#define	CTRL_CLKDIR	0x80	/* External clock line direct control */

/* Status Register Bits (RO) */
#define	STAT_RBNE	0x01	/* Receive buffer not empty */
#define	STAT_TBNE	0x02	/* Transmit buffer not empty */
#define	STAT_TERR	0x04	/* Timeout Error */
#define	STAT_PERR	0x08	/* Parity Error */
#define	STAT_CMPINTR	0x10	/* Composite interrupt */
#define	STAT_DATSHD	0x40	/* Data line shadow */
#define	STAT_CLKSHD	0x80	/* Clock line shadow */



/* autoconfig stuff */
static int gsckbc_match(device_t, cfdata_t, void *);
static void gsckbc_attach(device_t, device_t, void *);

static int gsckbc_xt_translation(void *, pckbport_slot_t, int);
static int gsckbc_send_devcmd(void *, pckbport_slot_t, u_char);
static int gsckbc_poll_data1(void *, pckbport_slot_t);
static void gsckbc_slot_enable(void *, pckbport_slot_t, int);
static void gsckbc_intr_establish(void *, pckbport_slot_t);
static void gsckbc_set_poll(void *, pckbport_slot_t, int);

static int gsckbc_intr(void *);


struct gsckbc_softc {
	device_t sc_dev;			/* general dev info */
	bus_space_tag_t sc_iot;			/* bus_space(9) tag */
	bus_space_handle_t sc_ioh;		/* bus_space(9) handle */
	struct gsckbc_softc *sc_op;		/* other port */
	void *sc_ih;				/* interrupt handle */
	pckbport_slot_t	sc_slot;		/* kbd or mouse / aux slot */
	pckbport_tag_t sc_pckbport;		/* port tag */
	device_t sc_child;			/* our child devices */
	int sc_poll;				/* if != 0 then pooling mode */
	int sc_enable;				/* if != 0 then enable */
};


CFATTACH_DECL_NEW(gsckbc, sizeof(struct gsckbc_softc), gsckbc_match, gsckbc_attach,
	NULL, NULL);


const struct pckbport_accessops gsckbc_accessops = {
	gsckbc_xt_translation,
	gsckbc_send_devcmd,
	gsckbc_poll_data1,
	gsckbc_slot_enable,
	gsckbc_intr_establish,
	gsckbc_set_poll
};


static int
gsckbc_xt_translation(void *cookie, pckbport_slot_t slot, int on)
{
	return 0;
}


static int
gsckbc_send_devcmd(void *cookie, pckbport_slot_t slot, u_char byte)
{
	struct gsckbc_softc *sc = (struct gsckbc_softc *) cookie;

	if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_STATUS) & STAT_TBNE)
	    != 0)
		DELAY(100);
	if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_STATUS) & STAT_TBNE)
	    != 0)
		return 0;
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, REG_XMITDATA, byte);
	return 1;
}


static int
gsckbc_poll_data1(void *cookie, pckbport_slot_t slot)
{
	struct gsckbc_softc *sc = (struct gsckbc_softc *) cookie;
	int i;

	for (i = 0; i < 1000; i++) {
		if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_STATUS) &
		    STAT_RBNE) != 0 || sc->sc_poll == 0) {
			return bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    REG_RCVDATA);
		}
		DELAY(100);
	}
	return -1;
}


static void
gsckbc_slot_enable(void *cookie, pckbport_slot_t slot, int on)
{
	struct gsckbc_softc *sc = (struct gsckbc_softc *) cookie;

	sc->sc_enable = on;
}


static void
gsckbc_intr_establish(void *cookie, pckbport_slot_t slot)
{
	return;
}


static void
gsckbc_set_poll(void *cookie, pckbport_slot_t slot, int on)
{
	struct gsckbc_softc *sc = (struct gsckbc_softc *) cookie;

	sc->sc_poll = on;
}


static int
gsckbc_intr(void *arg)
{
	struct gsckbc_softc *sc = (struct gsckbc_softc *) arg;
	int data;

	while ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_STATUS)
	    & STAT_RBNE) != 0 && sc->sc_poll == 0) {
		data = bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_RCVDATA);
		if (sc->sc_enable != 0)
			pckbportintr(sc->sc_pckbport, sc->sc_slot, data);
	}
	while ((bus_space_read_1(sc->sc_op->sc_iot, sc->sc_op->sc_ioh,
	    REG_STATUS) & STAT_RBNE) != 0 && sc->sc_op->sc_poll == 0) {
		data = bus_space_read_1(sc->sc_op->sc_iot, sc->sc_op->sc_ioh,
		    REG_RCVDATA);
		if (sc->sc_op->sc_enable != 0)
			pckbportintr(sc->sc_op->sc_pckbport, sc->sc_op->sc_slot,
			    data);
	}
	return 1;
}


static int
gsckbc_match(device_t parent, cfdata_t match, void *aux)
{
	struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type == HPPA_TYPE_FIO
	    && ga->ga_type.iodc_sv_model == HPPA_FIO_GPCIO)
		return(1);
	return(0);
}


static void
gsckbc_attach(device_t parent, device_t self, void *aux)
{
	struct gsckbc_softc *sc = device_private(self);
	struct gsc_attach_args *ga = aux;
	static struct gsckbc_softc *master_sc;
	int pagezero_cookie;
	int i;

	/*
	 * On hp700 bus_space_map(9) mapes whole pages. (surprise, surprise)
	 * The registers are within the same page so we can do only a single
	 * mapping for both devices. Also both devices use the same IRQ.
	 * Actually you can think of the two PS/2 ports to be a single
	 * device. The firmware lists them as individual devices in the
	 * firmware device tree so we keep this illusion to map the firmware
	 * device tree as close as possible to the kernel device tree.
	 * So we do one mapping and IRQ for both devices. The first device
	 * is caled "master", gets the IRQ and the other is the "slave".
	 *
	 * Assumption: Master attaches first, gets the IRQ and has lower HPA.
	 */
	sc->sc_dev = self;
	sc->sc_iot = ga->ga_iot;
	if (ga->ga_irq >= 0) {
		if (bus_space_map(sc->sc_iot, ga->ga_hpa, REG_SZ + REG_OFFSET,
		    0, &sc->sc_ioh)) {
			aprint_normal(": gsckbc_attach: can't map I/O space\n");
			return;
		}
		aprint_debug(" (master)");
		sc->sc_ih = hp700_intr_establish(sc->sc_dev, IPL_TTY,
		    gsckbc_intr, sc, ga->ga_int_reg, ga->ga_irq);
		master_sc = sc;
	} else {
		if (master_sc == NULL) {
			aprint_normal(": can't find master device\n");
			return;
		}
		sc->sc_op = master_sc;
		master_sc->sc_op = sc;
		sc->sc_ioh = sc->sc_op->sc_ioh + REG_OFFSET;
		aprint_debug(" (slave)");
	}
	/* We start in polling mode. */
	sc->sc_poll = 1;
	/* Reset port. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, REG_RESET, 0);
	/* Enable port hardware. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, REG_CONTROL, CTRL_ENBL);
	/* Flush RX FIFO. */
	for (i = 0; i < 5; i++)
		bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_RCVDATA);
	if (bus_space_read_1(sc->sc_iot, sc->sc_ioh, REG_ID) == ID_KBD) {
		aprint_normal(": keyboard\n");
		sc->sc_slot = PCKBPORT_KBD_SLOT;
		pagezero_cookie = hp700_pagezero_map();
		if ((hppa_hpa_t)PAGE0->mem_kbd.pz_hpa == ga->ga_hpa) {
			if (pckbport_cnattach(sc, &gsckbc_accessops,
			    sc->sc_slot) != 0)
				aprint_normal("Failed to attach console "
				    "keyboard!\n");
			else
				sc->sc_enable = 1;
		}
		hp700_pagezero_unmap(pagezero_cookie);
	} else {
		aprint_normal(": mouse\n");
		sc->sc_slot = PCKBPORT_AUX_SLOT;
	}
	sc->sc_pckbport = pckbport_attach(sc, &gsckbc_accessops);
	if (sc->sc_pckbport != NULL)
		sc->sc_child = pckbport_attach_slot(self, sc->sc_pckbport,
		    sc->sc_slot);
	sc->sc_poll = 0;
}

