/* $NetBSD: macekbc.c,v 1.2.6.2 2007/06/09 21:37:01 ad Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 *        This product includes software developed by Jared D. McNeill.
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
 * SGI MACE PS2 keyboard/mouse controller driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: macekbc.c,v 1.2.6.2 2007/06/09 21:37:01 ad Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <sgimips/mace/macevar.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>
#include <dev/pckbport/pckbportvar.h>

#define	MACEKBC_TX	0x00
#define MACEKBC_RX	0x08
#define MACEKBC_CTRL	0x10
#define		MACEKBC_CTRL_TXCLKOFF	(1 << 0)
#define		MACEKBC_CTRL_TXON	(1 << 1)
#define		MACEKBC_CTRL_TXINTEN	(1 << 2)
#define		MACEKBC_CTRL_RXINTEN	(1 << 3)
#define		MACEKBC_CTRL_RXCLKON	(1 << 4)
#define		MACEKBC_CTRL_RESET	(1 << 5)
#define MACEKBC_STAT	0x18
#define		MACEKBC_STAT_TXEMPTY	(1 << 3)
#define		MACEKBC_STAT_RXFULL	(1 << 4)

struct macekbc_softc {
	struct device 		sc_dev;
	struct macekbc_internal	*sc_id;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

struct macekbc_internal {
	struct macekbc_softc	*t_sc;
	pckbport_tag_t 		t_pt;

	bus_space_tag_t		t_iot;
	bus_space_handle_t	t_ioh[PCKBPORT_NSLOTS];
	int			t_present[PCKBPORT_NSLOTS];

	void			*t_rxih;
};

static int 	macekbc_intr(void *);
static void 	macekbc_reset(struct macekbc_internal *, pckbport_slot_t);

static int 	macekbc_xt_translation(void *, pckbport_slot_t, int);
static int 	macekbc_send_devcmd(void *, pckbport_slot_t, u_char);
static int 	macekbc_poll_data1(void *, pckbport_slot_t);
static void 	macekbc_slot_enable(void *, pckbport_slot_t, int);
static void 	macekbc_intr_establish(void *, pckbport_slot_t);
static void 	macekbc_set_poll(void *, pckbport_slot_t, int);

static int	macekbc_match(struct device *, struct cfdata *, void *);
static void 	macekbc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(macekbc, sizeof(struct macekbc_softc),
    macekbc_match, macekbc_attach, NULL, NULL);

static struct pckbport_accessops macekbc_ops = {
	.t_xt_translation 	= macekbc_xt_translation,
	.t_send_devcmd 		= macekbc_send_devcmd,
	.t_poll_data1 		= macekbc_poll_data1,
	.t_slot_enable 		= macekbc_slot_enable,
	.t_intr_establish 	= macekbc_intr_establish,
	.t_set_poll 		= macekbc_set_poll,
};

static int
macekbc_match(struct device *parent, struct cfdata *match, void *aux)
{
	const char *consdev;

	/* XXX don't bother attaching if we're using a serial console */
	consdev = ARCBIOS->GetEnvironmentVariable("ConsoleIn");
	if (consdev == NULL || strcmp(consdev, "keyboard()") != 0)
		return 0;

	return 1;
}

static void
macekbc_attach(struct device *parent, struct device *self, void *aux)
{
	struct mace_attach_args *maa;
	struct macekbc_softc *sc;
	struct macekbc_internal *t;
	int slot;

	maa = aux;
	sc = device_private(self);

	aprint_normal(": PS2 controller\n");
	aprint_naive("\n");

	t = malloc(sizeof(struct macekbc_internal), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (t == NULL) {
		aprint_error("%s: not enough memory\n", device_xname(self));
		return;
	}
	t->t_iot = maa->maa_st;
	for (slot = 0; slot < PCKBPORT_NSLOTS; slot++)
		t->t_present[slot] = 0;
	if (bus_space_subregion(t->t_iot, maa->maa_sh, maa->maa_offset,
	    0, &t->t_ioh[PCKBPORT_KBD_SLOT]) != 0) {
		aprint_error("%s: couldn't map kbd registers\n",
		    device_xname(self));
		return;
	}
	if (bus_space_subregion(t->t_iot, maa->maa_sh, maa->maa_offset + 32,
	    0, &t->t_ioh[PCKBPORT_AUX_SLOT]) != 0) {
		aprint_error("%s: couldn't map aux registers\n",
		    device_xname(self));
		return;
	}

	if ((t->t_rxih = cpu_intr_establish(maa->maa_intr, maa->maa_intrmask,
	    macekbc_intr, t)) == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    device_xname(self));
		return;
	}
	sc->sc_id = t;
	t->t_sc = sc;

	macekbc_reset(t, PCKBPORT_KBD_SLOT);
	macekbc_reset(t, PCKBPORT_AUX_SLOT);

	t->t_pt = pckbport_attach(t, &macekbc_ops);
	if (pckbport_attach_slot(&sc->sc_dev, t->t_pt, PCKBPORT_KBD_SLOT))
		t->t_present[PCKBPORT_KBD_SLOT] = 1;
	if (pckbport_attach_slot(&sc->sc_dev, t->t_pt, PCKBPORT_AUX_SLOT))
		t->t_present[PCKBPORT_AUX_SLOT] = 1;

	return;
}

static int
macekbc_intr(void *opaque)
{
	struct macekbc_internal	*t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint64_t stat, val;
	pckbport_slot_t slot;
	int rv;

	t = opaque;
	iot = t->t_iot;
	rv = 0;

	for (slot = 0; slot < PCKBPORT_NSLOTS; slot++) {
		if (t->t_present[slot] == 0)
			continue;

		ioh = t->t_ioh[slot];
		stat = bus_space_read_8(iot, ioh, MACEKBC_STAT);
		if (stat & MACEKBC_STAT_RXFULL) {
			val = bus_space_read_8(iot, ioh, MACEKBC_RX);
			pckbportintr(t->t_pt, slot, val & 0xff);
			rv = 1;
		}
	}

	return rv; 
}

static void
macekbc_reset(struct macekbc_internal *t, pckbport_slot_t slot)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint64_t val;

	iot = t->t_iot;
	ioh = t->t_ioh[slot];

	val = bus_space_read_8(iot, ioh, MACEKBC_CTRL);
	val |= MACEKBC_CTRL_TXCLKOFF | MACEKBC_CTRL_RESET;
	bus_space_write_8(iot, ioh, MACEKBC_CTRL, val);

	delay(10000);

	val &= ~(MACEKBC_CTRL_TXCLKOFF | MACEKBC_CTRL_RESET);
	val |= MACEKBC_CTRL_TXON | MACEKBC_CTRL_RXCLKON | MACEKBC_CTRL_RXINTEN;
	bus_space_write_8(iot, ioh, MACEKBC_CTRL, val);

	return;
}

static int
macekbc_wait(struct macekbc_internal *t, pckbport_slot_t slot,
    uint64_t mask, bool set)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint64_t val, tmp;
	int timeout;

	iot = t->t_iot;
	ioh = t->t_ioh[slot];
	val = (set ? mask : 0);
	timeout = 1000;

	while (timeout-- > 0) {
		tmp = bus_space_read_8(iot, ioh, MACEKBC_STAT);
		if ((tmp & mask) == val)
			return 0;
		delay(10);
	}

	return 1;
}

static int
macekbc_xt_translation(void *opaque, pckbport_slot_t port, int on)
{

	if (on)
		return 0;

	return 1;
}

static int
macekbc_send_devcmd(void *opaque, pckbport_slot_t slot, u_char byte)
{
	struct macekbc_internal	*t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	t = opaque;
	iot = t->t_iot;
	ioh = t->t_ioh[slot];

	if (macekbc_wait(t, slot, MACEKBC_STAT_TXEMPTY, 1))
		return 0;

	bus_space_write_8(iot, ioh, MACEKBC_TX, byte & 0xff);

	return 1;
}

static int
macekbc_poll_data1(void *opaque, pckbport_slot_t slot)
{
	struct macekbc_internal	*t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	t = opaque;
	iot = t->t_iot;
	ioh = t->t_ioh[slot];

	if (macekbc_wait(t, slot, MACEKBC_STAT_RXFULL, 1)) /* rx full */
		return -1;

	return bus_space_read_8(iot, ioh, MACEKBC_RX) & 0xff;
}

static void
macekbc_slot_enable(void *opaque, pckbport_slot_t slot, int on)
{
	struct macekbc_internal *t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint64_t val;

	t = opaque;
	iot = t->t_iot;
	ioh = t->t_ioh[slot];

	val = bus_space_read_8(iot, ioh, MACEKBC_CTRL);
	if (on)
		val |= MACEKBC_CTRL_TXON | MACEKBC_CTRL_RXCLKON;
	else
		val &= ~(MACEKBC_CTRL_TXON | MACEKBC_CTRL_RXCLKON);
	bus_space_write_8(iot, ioh, MACEKBC_CTRL, val);

	return;
}

static void
macekbc_intr_establish(void *opaque, pckbport_slot_t slot)
{

	/* XXX */
	macekbc_set_poll(opaque, slot, 0);

	return;
}

static void
macekbc_set_poll(void *opaque, pckbport_slot_t slot, int on)
{
	struct macekbc_internal *t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint64_t val;

	t = opaque;
	iot = t->t_iot;
	ioh = t->t_ioh[slot];

	val = bus_space_read_8(iot, ioh, MACEKBC_CTRL);
	if (on)
		val &= ~MACEKBC_CTRL_RXINTEN;
	else
		val |= MACEKBC_CTRL_RXINTEN;
	bus_space_write_8(iot, ioh, MACEKBC_CTRL, val);

	return;
}
