/*      $NetBSD: sa1111_kbc.c,v 1.15.6.2 2014/08/20 00:02:47 tls Exp $ */

/*
 * Copyright (c) 2004  Ben Harris.
 * Copyright (c) 2002, 2004  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Driver for keyboard controller in SA-1111 companion chip.
 */
/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa1111_kbc.c,v 1.15.6.2 2014/08/20 00:02:47 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/bus.h>
#include <sys/rnd.h>

#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>

#include <dev/pckbport/pckbportvar.h>		/* for prototypes */

#include "pckbd.h"
#include "locators.h"

struct sackbc_softc {
	device_t dev;

	bus_space_tag_t    iot;
	bus_space_handle_t ioh;

	void	*ih_rx;			/* receive interrupt */
	int	intr;			/* interrupt number */
	int	slot;		/* KBD_SLOT or AUX_SLOT */

	int	polling;	/* don't process data in interrupt handler */
	int	poll_stat;	/* data read from inr handler if polling */
	int	poll_data;	/* status read from intr handler if polling */

	krndsource_t	rnd_source;
	pckbport_tag_t pt;
};

static int	sackbc_match(device_t, cfdata_t, void *);
static void	sackbc_attach(device_t, device_t, void *);

static int	sackbc_xt_translation(void *, pckbport_slot_t, int);
#define sackbc_send_devcmd	sackbc_send_cmd
static int	sackbc_send_devcmd(void *, pckbport_slot_t, u_char);
static int	sackbc_poll_data1(void *, pckbport_slot_t);
static void	sackbc_slot_enable(void *, pckbport_slot_t, int);
static void	sackbc_intr_establish(void *, pckbport_slot_t);
static void	sackbc_set_poll(void *, pckbport_slot_t, int);

CFATTACH_DECL_NEW(sackbc, sizeof(struct sackbc_softc), sackbc_match,
    sackbc_attach, NULL, NULL);

static struct pckbport_accessops const sackbc_ops = {
	sackbc_xt_translation,
	sackbc_send_devcmd,
	sackbc_poll_data1,
	sackbc_slot_enable,
	sackbc_intr_establish,
	sackbc_set_poll
};

#define	KBD_DELAY	DELAY(8)

/*#define SACKBCDEBUG*/

#ifdef SACKBCDEBUG
#define DPRINTF(arg)  printf arg
#else
#define DPRINTF(arg)
#endif


static int
sackbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sa1111_attach_args *aa = (struct sa1111_attach_args *)aux;

	switch (aa->sa_addr) {
	case SACC_KBD0:
	case SACC_KBD1:
		return 1;
	}
	return 0;
}

#if 0
static int
sackbc_txint(void *cookie)
{
	struct sackbc_softc *sc = cookie;

	bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);

	return 0;
}
#endif

static int
sackbc_rxint(void *cookie)
{
	struct sackbc_softc *sc = cookie;
	int stat, code = -1;

	stat = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);
	DPRINTF(("sackbc_rxint stat=%x\n", stat));
	if (stat & KBDSTAT_RXF) {
		code = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_DATA);

		rnd_add_uint32(&sc->rnd_source, (stat<<8)|code);

		if (sc->polling) {
			sc->poll_data = code;
			sc->poll_stat = stat;
		} else
			pckbportintr(sc->pt, sc->slot, code);
		return 1;
	}

	return 0;
}

static void
sackbc_intr_establish(void *cookie, pckbport_slot_t slot)
{
	struct sackbc_softc *sc = cookie;

	if (!(sc->polling) && sc->ih_rx == NULL) {
		sc->ih_rx = sacc_intr_establish(
			(sacc_chipset_tag_t *) 
			  device_private(device_parent(sc->dev)), 
			sc->intr+1, IST_EDGE_RAISE, IPL_TTY, sackbc_rxint, sc);
		if (sc->ih_rx == NULL) {
			aprint_normal_dev(sc->dev, "can't establish interrupt\n");
		}
	}
}

static void
sackbc_disable_intrhandler(struct sackbc_softc *sc)
{
	if (sc->polling && sc->ih_rx) {
		sacc_intr_disestablish(
			(sacc_chipset_tag_t *)
			  device_private(device_parent(sc->dev)),
			sc->ih_rx);
		sc->ih_rx = NULL;
	}
}

static	void	
sackbc_attach(device_t parent, device_t self, void *aux)
{
	struct sackbc_softc *sc = device_private(self);
	struct sacc_softc *psc = device_private(parent);
	struct sa1111_attach_args *aa = (struct sa1111_attach_args *)aux;
	device_t child;
	uint32_t tmp, clock_bit;
	int intr, slot;

	switch (aa->sa_addr) {
	case SACC_KBD0: clock_bit = (1<<6); intr = 21; break;
	case SACC_KBD1: clock_bit = (1<<5); intr = 18; break;
	default:
		return;
	}

	if (aa->sa_size <= 0)
		aa->sa_size = SACCKBD_SIZE;
	if (aa->sa_intr == SACCCF_INTR_DEFAULT)
		aa->sa_intr = intr;

	sc->dev = self;
	sc->iot = psc->sc_iot;
	if (bus_space_subregion(psc->sc_iot, psc->sc_ioh,
	    aa->sa_addr, aa->sa_size, &sc->ioh)) {
		aprint_normal(": can't map subregion\n");
		return;
	}

	/* enable clock for PS/2 kbd or mouse */
	tmp = bus_space_read_4(psc->sc_iot, psc->sc_ioh, SACCSC_SKPCR);
	bus_space_write_4(psc->sc_iot, psc->sc_ioh, SACCSC_SKPCR,
	    tmp | clock_bit);

	sc->ih_rx = NULL;
	sc->intr = aa->sa_intr;
	sc->polling = 0;

	tmp = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_CR);
	bus_space_write_4(sc->iot, sc->ioh, SACCKBD_CR, tmp | KBDCR_ENA);

	/* XXX: this is necessary to get keyboard working. but I don't know why */
	bus_space_write_4(sc->iot, sc->ioh, SACCKBD_CLKDIV, 2);

	tmp = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);
	if ((tmp & KBDSTAT_ENA) == 0) {
		printf("??? can't enable KBD controller\n");
		return;
	}

	printf("\n");

	sc->pt = pckbport_attach(sc, &sackbc_ops);

	/*
	 * Although there is no such thing as SLOT for SA-1111 kbd
	 * controller, pckbd and pms drivers require it.
	 */
	for (slot = PCKBPORT_KBD_SLOT; slot <= PCKBPORT_AUX_SLOT; ++slot) {
		child = pckbport_attach_slot(self, sc->pt, slot);

		if (child == NULL)
			continue;
		sc->slot = slot;
		rnd_attach_source(&sc->rnd_source, device_xname(child),
		    RND_TYPE_TTY, RND_FLAG_DEFAULT|RND_FLAG_ESTIMATE_VALUE);
		/* only one of KBD_SLOT or AUX_SLOT is used. */
		break;			
	}
}


static inline int
sackbc_wait_output(struct sackbc_softc *sc)
{
	u_int i, stat;

	for (i = 100000; i; i--){
		stat = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);
		delay(100);
		if (stat & KBDSTAT_TXE)
			return 1;
	}
	return 0;
}

static int
sackbc_poll_data1(void *cookie, pckbport_slot_t slot)
{
	struct sackbc_softc *sc = cookie;
	int i, s, stat, c = -1;

	s = spltty();

	if (sc->polling){
		stat	= sc->poll_stat;
		c	= sc->poll_data;
		sc->poll_data = -1;
		sc->poll_stat = -1;
		if (stat >= 0 &&
		    (stat & (KBDSTAT_RXF|KBDSTAT_STP)) == KBDSTAT_RXF) {
			splx(s);
			return c;
		}
	}

	/* if 1 port read takes 1us (?), this polls for 100ms */
	for (i = 100000; i; i--) {
		stat = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);
		if ((stat & (KBDSTAT_RXF|KBDSTAT_STP)) == KBDSTAT_RXF) {
			KBD_DELAY;
			c = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_DATA);
			break;	
		}
	}

	splx(s);
	return c;
}

static int
sackbc_send_cmd(void *cookie, pckbport_slot_t slot, u_char val)
{
	struct sackbc_softc *sc = cookie;

	if (!sackbc_wait_output(sc))
		return 0;
	bus_space_write_1(sc->iot, sc->ioh, SACCKBD_DATA, val);
	return 1;
}


/*
 * Glue functions for pckbd on sackbc.
 * These functions emulate those in dev/ic/pckbc.c.
 * 
 */

/*
 * switch scancode translation on / off
 * return nonzero on success
 */
static int
sackbc_xt_translation(void *self, pckbport_slot_t slot, int on)
{
	/* KBD/Mouse controller doesn't have scancode translation */
	return !on;
}

static void
sackbc_slot_enable(void *self, pckbport_slot_t slot, int on)
{
#if 0
	struct sackbc_softc *sc = device_private(self);
	int cmd;

	cmd = on ? KBC_KBDENABLE : KBC_KBDDISABLE;
	if (!sackbc_send_cmd(sc, cmd))
		printf("sackbc_slot_enable(%d) failed\n", on);
#endif
}


static void
sackbc_set_poll(void *self, pckbport_slot_t slot, int on)
{
	struct sackbc_softc *sc = device_private(self);
	int s;

	s = spltty();

	if (sc->polling != on) {

		sc->polling = on;

		if (on) {
			sc->poll_data = sc->poll_stat = -1;
			sackbc_disable_intrhandler(sc);
		} else {
			/*
			 * If disabling polling on a device that's
			 * been configured, make sure there are no
			 * bytes left in the FIFO, holding up the
			 * interrupt line.  Otherwise we won't get any
			 * further interrupts.
			 */
			sackbc_rxint(sc);
			sackbc_intr_establish(sc, sc->slot);
		}
	}
	splx(s);
}
