/* $NetBSD: iomdkbc.c,v 1.3 2006/10/21 22:45:03 bjh21 Exp $ */

/*-
 * Copyright (c) 2004 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iomdkbc.c,v 1.3 2006/10/21 22:45:03 bjh21 Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/pckbport/pckbportvar.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arch/arm/iomd/iomdreg.h>
#include <arch/arm/iomd/iomdvar.h>
#include <arch/arm/iomd/iomdkbcvar.h>

/* XXX belongs in iomdreg.h */
#define IOMDKBC_TXE	0x80
#define IOMDKBC_TXB	0x40
#define IOMDKBC_RXF	0x20
#define IOMDKBC_RXB	0x10
#define IOMDKBC_ENA	0x08
#define IOMDKBC_RXP	0x04
#define IOMDKBC_DATA	0x02
#define IOMDKBC_CLK	0x01

#define IOMDKBC_DR	0x00
#define IOMDKBC_CR	0x01

#define KBC_DEVCMD_ACK 0xfa
#define KBC_DEVCMD_RESEND 0xfe

struct iomdkbc_softc {
	struct device	sc_dev;
	struct iomdkbc_internal *sc_id;
};

struct iomdkbc_internal {
	struct iomdkbc_softc	*t_sc;
	struct pckbport_tag	*t_pt;

	int			t_haveport[PCKBPORT_NSLOTS];
	bus_space_tag_t		t_iot;
	bus_space_handle_t	t_ioh[PCKBPORT_NSLOTS];

	void	*t_rxih[PCKBPORT_NSLOTS];
	int	t_rxirq[PCKBPORT_NSLOTS];
};

static int iomdkbc_match(struct device *, struct cfdata *, void *);
static void iomdkbc_attach(struct device *, struct device *, void *);

static int iomdkbc_xt_translation(void *, pckbport_slot_t, int);
static int iomdkbc_send_devcmd(void *, pckbport_slot_t, u_char);
static int iomdkbc_poll_data1(void *, pckbport_slot_t);
static void iomdkbc_slot_enable(void *, pckbport_slot_t, int);
static void iomdkbc_intr_establish(void *, pckbport_slot_t);
static void iomdkbc_set_poll(void *, pckbport_slot_t, int);

static int iomdkbc_intr(void *);

CFATTACH_DECL(iomdkbc, sizeof(struct iomdkbc_softc),
    iomdkbc_match, iomdkbc_attach, NULL, NULL);

static struct pckbport_accessops const iomdkbc_ops = {
	iomdkbc_xt_translation,
	iomdkbc_send_devcmd,
	iomdkbc_poll_data1,
	iomdkbc_slot_enable,
	iomdkbc_intr_establish,
	iomdkbc_set_poll
};

static struct iomdkbc_internal iomdkbc_cntag;

static int
iomdkbc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct kbd_attach_args *ka = aux;
	struct opms_attach_args *pa = aux;

	if (strcmp(ka->ka_name, "kbd") == 0 ||
	    strcmp(pa->pa_name, "opms") == 0)
		return 1;
	return 0;
}

static void
iomdkbc_attach(struct device *parent, struct device *self, void *aux)
{
	struct kbd_attach_args *ka = aux;
	struct opms_attach_args *pa = aux;
	struct iomdkbc_softc *sc = (struct iomdkbc_softc *)self;
	struct iomdkbc_internal *t;

	printf("\n");

	t = NULL;
	if (strcmp(ka->ka_name, "kbd") == 0) {
		/* XXX not really right, but good enough. */
		if (iomdkbc_cntag.t_haveport[PCKBPORT_KBD_SLOT]) {
			/* Have an iomdkbc as console.  Assume it's this one.*/
			t = &iomdkbc_cntag;
		} else {
			t = malloc(sizeof(struct iomdkbc_internal), M_DEVBUF,
			    M_NOWAIT | M_ZERO);
			if (t == NULL) {
				aprint_error(": no memory");
				return;
			}
			t->t_haveport[PCKBPORT_KBD_SLOT] = 1;
			t->t_iot = ka->ka_iot;
			t->t_ioh[PCKBPORT_KBD_SLOT] = ka->ka_ioh;
		}
		t->t_rxih[PCKBPORT_KBD_SLOT] = intr_claim(ka->ka_rxirq,
		    IPL_TTY, sc->sc_dev.dv_xname, iomdkbc_intr, t);
		t->t_rxirq[PCKBPORT_KBD_SLOT] = ka->ka_rxirq;
		disable_irq(t->t_rxirq[PCKBPORT_KBD_SLOT]);
		sc->sc_id = t;
		t->t_sc = sc;
		t->t_pt = pckbport_attach(t, &iomdkbc_ops);
		pckbport_attach_slot(&sc->sc_dev, t->t_pt, PCKBPORT_KBD_SLOT);
	}

	if (strcmp(pa->pa_name, "opms") == 0) {
		if (t == NULL) {
			t = malloc(sizeof(struct iomdkbc_internal), M_DEVBUF,
			    M_NOWAIT | M_ZERO);
			if (t == NULL) {
				aprint_error(": no memory");
				return;
			}
		}
		t->t_haveport[PCKBPORT_AUX_SLOT] = 1;
		t->t_iot = pa->pa_iot;
		t->t_ioh[PCKBPORT_AUX_SLOT] = pa->pa_ioh;
		t->t_rxih[PCKBPORT_AUX_SLOT] = intr_claim(pa->pa_irq,
		    IPL_TTY, sc->sc_dev.dv_xname, iomdkbc_intr, t);
		t->t_rxirq[PCKBPORT_AUX_SLOT] = pa->pa_irq;
		disable_irq(t->t_rxirq[PCKBPORT_AUX_SLOT]);
		sc->sc_id = t;
		t->t_sc = sc;
		if (t->t_pt == NULL)
			t->t_pt = pckbport_attach(t, &iomdkbc_ops);
		pckbport_attach_slot(&sc->sc_dev, t->t_pt, PCKBPORT_AUX_SLOT);
	}
}

static int
iomdkbc_send_devcmd(void *cookie, pckbport_slot_t slot, u_char cmd)
{
	struct iomdkbc_internal *t = cookie;
	bus_space_tag_t iot = t->t_iot;
	bus_space_handle_t ioh = t->t_ioh[slot];
	int timeout;

	timeout = 10000;
	while ((bus_space_read_1(iot, ioh, IOMDKBC_CR) &
		   IOMDKBC_TXE) == 0) {
		DELAY(10);
		if (--timeout == 0) return 0;
	}

        bus_space_write_1(iot, ioh, IOMDKBC_DR, cmd);
	return 1;
}

static int
iomdkbc_poll_data1(void *cookie, pckbport_slot_t slot)
{
	struct iomdkbc_internal *t = cookie;
	bus_space_tag_t iot = t->t_iot;
	bus_space_handle_t ioh = t->t_ioh[slot];
	int timeout;

	timeout = 10000;
	while ((bus_space_read_1(iot, ioh, IOMDKBC_CR) &
		   IOMDKBC_RXF) == 0) {
		DELAY(10);
		if (--timeout == 0) return -1;
	}

        return bus_space_read_1(iot, ioh, IOMDKBC_DR);
}

/*
 * switch scancode translation on / off
 * return nonzero on success
 */
static int
iomdkbc_xt_translation(void *cookie, pckbport_slot_t slot, int on)
{

	if (on)
		return 0; /* Can't do XT translation */
	else
		return 1;
}

static void
iomdkbc_slot_enable(void *cookie, pckbport_slot_t slot, int on)
{
	struct iomdkbc_internal *t = cookie;
	bus_space_tag_t iot = t->t_iot;
	bus_space_handle_t ioh = t->t_ioh[slot];

	bus_space_write_1(iot, ioh, IOMDKBC_CR, on ? IOMDKBC_ENA : 0);
}


static void
iomdkbc_intr_establish(void *cookie, pckbport_slot_t slot)
{
	struct iomdkbc_internal *t = cookie;

	enable_irq(t->t_rxirq[slot]);
}

static void
iomdkbc_set_poll(void *cookie, pckbport_slot_t slot, int on)
{
	struct iomdkbc_internal *t = cookie;

	if (on)
		disable_irq(t->t_rxirq[slot]);
	else
		enable_irq(t->t_rxirq[slot]);
}

static int
iomdkbc_intr(void *self)
{
	struct iomdkbc_internal *t = self;
	bus_space_tag_t iot = t->t_iot;
	bus_space_handle_t ioh;
	unsigned i;
	int stat, ret;

	ret = 0;
	for (i = 0; i < PCKBPORT_NSLOTS; i++)
		if (t->t_haveport[i]) {
			ioh = t->t_ioh[i];
			stat = bus_space_read_1(iot, ioh, IOMDKBC_CR);
			if ((stat & IOMDKBC_RXF) == 0)
				continue;
			pckbportintr(t->t_pt, i,
			    bus_space_read_1(iot, ioh, IOMDKBC_DR));
			ret = 1;
		}

	return ret;
}

int
iomdkbc_cnattach(bus_space_tag_t iot, bus_addr_t addr, int slot)
{
	struct iomdkbc_internal *t = &iomdkbc_cntag;

	t->t_iot = iot;
	bus_space_map(iot, addr, 2, 0, &t->t_ioh[slot]);
	t->t_haveport[slot] = 1;
	iomdkbc_slot_enable(t, slot, 1);
	return pckbport_cnattach(t, &iomdkbc_ops, slot);
}
