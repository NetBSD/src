/* $NetBSD: pl050.c,v 1.2.6.2 2017/12/03 11:37:04 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl050.c,v 1.2.6.2 2017/12/03 11:37:04 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/pckbport/pckbportvar.h>

#include <dev/ic/pl050var.h>

#define	KMICR		0x00
#define	 KMIRXINTREN	__BIT(4)
#define	 KMIEN		__BIT(2)
#define	KMISTAT		0x04
#define	 TXEMPTY	__BIT(6)
#define	 RXFULL		__BIT(4)
#define	KMIDATA		0x08
#define	KMICLKDIV	0x0c
#define	KMIIR		0x10
#define	 KMIRXINTR	__BIT(0)

#define	PLKMI_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PLKMI_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
plkmi_wait(struct plkmi_softc *sc, uint32_t mask, bool set)
{
	int timeout = 1000;

	const uint32_t val = (set ? mask : 0);

	while (timeout-- > 0) {
		const uint32_t stat = PLKMI_READ(sc, KMISTAT);
		if ((stat & mask) == val)
			return 0;
		delay(10);
	}

	return ETIMEDOUT;
}

static int
plkmi_xt_translation(void *priv, pckbport_slot_t port, int on)
{
	if (on)
		return 0;
	return 1;
}

static int
plkmi_send_devcmd(void *priv, pckbport_slot_t slot, u_char byte)
{
	struct plkmi_softc * const sc = priv;

	if (plkmi_wait(sc, TXEMPTY, true))
		return 0;

	PLKMI_WRITE(sc, KMIDATA, byte & 0xff);

	return 1;
}

static int
plkmi_poll_data1(void *priv, pckbport_slot_t slot)
{
	struct plkmi_softc * const sc = priv;

	if (plkmi_wait(sc, RXFULL, true))
		return -1;

	return PLKMI_READ(sc, KMIDATA) & 0xff;
}

static void
plkmi_slot_enable(void *priv, pckbport_slot_t slot, int on)
{
	struct plkmi_softc * const sc = priv;
	uint32_t cr;

	cr = PLKMI_READ(sc, KMICR);
	if (on)
		cr |= KMIEN;
	else
		cr &= ~KMIEN;
	PLKMI_WRITE(sc, KMICR, cr);
}

static void
plkmi_set_poll(void *priv, pckbport_slot_t slot, int on)
{
	struct plkmi_softc * const sc = priv;
	uint32_t cr;

	cr = PLKMI_READ(sc, KMICR);
	if (on)
		cr &= ~KMIRXINTREN;
	else
		cr |= KMIRXINTREN;
	PLKMI_WRITE(sc, KMICR, cr);
}

static void
plkmi_intr_establish(void *priv, pckbport_slot_t slot)
{
	/* XXX */
	plkmi_set_poll(priv, slot, 0);
}

static struct pckbport_accessops plkmi_ops = {
	.t_xt_translation = plkmi_xt_translation,
	.t_send_devcmd = plkmi_send_devcmd,
	.t_poll_data1 = plkmi_poll_data1,
	.t_slot_enable = plkmi_slot_enable,
	.t_set_poll = plkmi_set_poll,
	.t_intr_establish = plkmi_intr_establish,
};

void
plkmi_attach(struct plkmi_softc *sc)
{
	aprint_naive("\n");
	aprint_normal(": PS2 controller\n");

	sc->sc_pt = pckbport_attach(sc, &plkmi_ops);
	sc->sc_slot = -1;

	PLKMI_WRITE(sc, KMICR, KMIEN);

	for (int slot = 0; slot < PCKBPORT_NSLOTS; slot++)
		if (pckbport_attach_slot(sc->sc_dev, sc->sc_pt, slot)) {
			sc->sc_slot = slot;
			break;
		}

	if (sc->sc_slot == PCKBPORT_KBD_SLOT)
		pckbport_cnattach(sc, &plkmi_ops, sc->sc_slot);
}

int
plkmi_intr(void *priv)
{
	struct plkmi_softc * const sc = priv;
	int handled = 0;

	if (sc->sc_slot == -1)
		return 0;

	const uint32_t stat = PLKMI_READ(sc, KMISTAT);
	if (stat & RXFULL) {
		const uint32_t val = PLKMI_READ(sc, KMIDATA);
		pckbportintr(sc->sc_pt, sc->sc_slot, val & 0xff);
		handled = 1;
	}

	return handled;
}
