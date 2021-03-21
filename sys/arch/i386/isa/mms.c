/*	$NetBSD: mms.c,v 1.53.68.1 2021/03/21 21:09:01 thorpej Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mms.c,v 1.53.68.1 2021/03/21 21:09:01 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <dev/isa/isavar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#define	MMS_ADDR	0	/* offset for register select */
#define	MMS_DATA	1	/* offset for InPort data */
#define	MMS_IDENT	2	/* offset for identification register */
#define	MMS_NPORTS	4

struct mms_softc {		/* driver status information */
	void *sc_ih;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	int sc_enabled; /* device is open */

	device_t sc_wsmousedev;
};

static int mmsprobe(device_t, cfdata_t, void *);
static void mmsattach(device_t, device_t, void *);
static int mmsintr(void *);

CFATTACH_DECL_NEW(mms, sizeof(struct mms_softc),
    mmsprobe, mmsattach, NULL, NULL);

static int	mms_enable(void *);
static int	mms_ioctl(void *, u_long, void *, int, struct lwp *);
static void	mms_disable(void *);

static const struct wsmouse_accessops mms_accessops = {
	mms_enable,
	mms_ioctl,
	mms_disable,
};

static int
mmsprobe(device_t parent, cfdata_t match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int rv;

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* Disallow wildcarded i/o address. */
	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	if (ia->ia_irq[0].ir_irq == ISA_UNKNOWN_IRQ)
		return 0;

	/* Map the i/o space. */
	if (bus_space_map(iot, ia->ia_io[0].ir_addr, MMS_NPORTS, 0, &ioh))
		return 0;

	rv = 0;

	/* Read identification register to see if present */
	if (bus_space_read_1(iot, ioh, MMS_IDENT) != 0xde)
		goto out;

	/* Seems it was there; reset. */
	bus_space_write_1(iot, ioh, MMS_ADDR, 0x87);

	rv = 1;

	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = MMS_NPORTS;

	ia->ia_nirq = 1;

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;

out:
	bus_space_unmap(iot, ioh, MMS_NPORTS);
	return rv;
}

static void
mmsattach(device_t parent, device_t self, void *aux)
{
	struct mms_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct wsmousedev_attach_args a;

	aprint_naive(": Mouse\n");
	aprint_normal(": Microsoft Mouse\n");

	if (bus_space_map(iot, ia->ia_io[0].ir_addr, MMS_NPORTS, 0, &ioh)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	/* Other initialization was done by mmsprobe. */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_enabled = 0;

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
	    IST_PULSE, IPL_TTY, mmsintr, sc);

	a.accessops = &mms_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wsmouse, saving a handle to it.
	 * Note that we don't need to check this pointer against NULL
	 * here or in psmintr, because if this fails lms_enable() will
	 * never be called, so lmsintr() will never be called.
	 */
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint, CFARG_EOL);
}

static int
mms_enable(void *v)
{
	struct mms_softc *sc = v;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;

	/* Enable interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, MMS_ADDR, 0x07);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, MMS_DATA, 0x09);

	return 0;
}

static void
mms_disable(void *v)
{
	struct mms_softc *sc = v;

	/* Disable interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, MMS_ADDR, 0x87);

	sc->sc_enabled = 0;
}

static int
mms_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#if 0
	struct mms_softc *sc = v;
#endif

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_MMS;
		return (0);
	}
	return (EPASSTHROUGH);
}

static int
mmsintr(void *arg)
{
	struct mms_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char status;
	signed char dx, dy;
	u_int buttons;
	int changed;

	if (!sc->sc_enabled)
		/* Interrupts are not expected. */
		return 0;

	/* Freeze InPort registers (disabling interrupts). */
	bus_space_write_1(iot, ioh, MMS_ADDR, 0x07);
	bus_space_write_1(iot, ioh, MMS_DATA, 0x29);

	bus_space_write_1(iot, ioh, MMS_ADDR, 0x00);
	status = bus_space_read_1(iot, ioh, MMS_DATA);

	if (status & 0x40) {
		bus_space_write_1(iot, ioh, MMS_ADDR, 1);
		dx = bus_space_read_1(iot, ioh, MMS_DATA);
		/* Bounding at -127 avoids a bug in XFree86. */
		dx = (dx == -128) ? -127 : dx;

		bus_space_write_1(iot, ioh, MMS_ADDR, 2);
		dy = bus_space_read_1(iot, ioh, MMS_DATA);
		dy = (dy == -128) ? 127 : -dy;
	} else
		dx = dy = 0;

	/* Unfreeze InPort registers (reenabling interrupts). */
	bus_space_write_1(iot, ioh, MMS_ADDR, 0x07);
	bus_space_write_1(iot, ioh, MMS_DATA, 0x09);

	buttons = ((status & 0x04) ? 0x1 : 0) |
		((status & 0x02) ? 0x2 : 0) |
		((status & 0x01) ? 0x4 : 0);
	changed = status & 0x38;

	if (dx || dy || changed)
		wsmouse_input(sc->sc_wsmousedev,
				buttons,
				dx, dy, 0, 0,
				WSMOUSE_INPUT_DELTA);

	return -1;
}
