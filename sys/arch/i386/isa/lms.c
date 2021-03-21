/*	$NetBSD: lms.c,v 1.56.68.1 2021/03/21 21:09:01 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: lms.c,v 1.56.68.1 2021/03/21 21:09:01 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#define	LMS_DATA	0       /* offset for data port, read-only */
#define	LMS_SIGN	1       /* offset for signature port, read-write */
#define	LMS_INTR	2       /* offset for interrupt port, read-only */
#define	LMS_CNTRL	2       /* offset for control port, write-only */
#define	LMS_CONFIG	3	/* for configuration port, read-write */
#define	LMS_NPORTS	4

struct lms_softc {		/* driver status information */
	void *sc_ih;

	bus_space_tag_t sc_iot;		/* bus i/o space identifier */
	bus_space_handle_t sc_ioh;	/* bus i/o handle */

	int sc_enabled; /* device is open */
	int oldbuttons;	/* mouse button status */

	device_t sc_wsmousedev;
};

static int lmsprobe(device_t, cfdata_t, void *);
static void lmsattach(device_t, device_t, void *);
static int lmsintr(void *);

CFATTACH_DECL_NEW(lms, sizeof(struct lms_softc),
    lmsprobe, lmsattach, NULL, NULL);

static int	lms_enable(void *);
static int	lms_ioctl(void *, u_long, void *, int, struct lwp *);
static void	lms_disable(void *);

static const struct wsmouse_accessops lms_accessops = {
	lms_enable,
	lms_ioctl,
	lms_disable,
};

static int
lmsprobe(device_t parent, cfdata_t match, void *aux)
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

	/* Disallow wildcarded i/o base. */
	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;
	if (ia->ia_irq[0].ir_irq == ISA_UNKNOWN_IRQ)
		return 0;

	/* Map the i/o space. */
	if (bus_space_map(iot, ia->ia_io[0].ir_addr, LMS_NPORTS, 0, &ioh))
		return 0;

	rv = 0;

	/* Configure and check for port present. */
	bus_space_write_1(iot, ioh, LMS_CONFIG, 0x91);
	delay(10);
	bus_space_write_1(iot, ioh, LMS_SIGN, 0x0c);
	delay(10);
	if (bus_space_read_1(iot, ioh, LMS_SIGN) != 0x0c)
		goto out;
	bus_space_write_1(iot, ioh, LMS_SIGN, 0x50);
	delay(10);
	if (bus_space_read_1(iot, ioh, LMS_SIGN) != 0x50)
		goto out;

	/* Disable interrupts. */
	bus_space_write_1(iot, ioh, LMS_CNTRL, 0x10);

	rv = 1;
	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = LMS_NPORTS;

	ia->ia_nirq = 1;

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;

out:
	bus_space_unmap(iot, ioh, LMS_NPORTS);
	return rv;
}

static void
lmsattach(device_t parent, device_t self, void *aux)
{
	struct lms_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct wsmousedev_attach_args a;

	aprint_naive(": Mouse\n");
	aprint_normal(": Logitech Mouse\n");

	if (bus_space_map(iot, ia->ia_io[0].ir_addr, LMS_NPORTS, 0, &ioh)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	/* Other initialization was done by lmsprobe. */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_enabled = 0;

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
	    IST_PULSE, IPL_TTY, lmsintr, sc);

	a.accessops = &lms_accessops;
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
lms_enable(void *v)
{
	struct lms_softc *sc = v;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;
	sc->oldbuttons = 0;

	/* Enable interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LMS_CNTRL, 0);

	return 0;
}

static void
lms_disable(void *v)
{
	struct lms_softc *sc = v;

	/* Disable interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LMS_CNTRL, 0x10);

	sc->sc_enabled = 0;
}

static int
lms_ioctl(void *v, u_long cmd, void *data, int flag,
    struct lwp *l)
{
#if 0
	struct lms_softc *sc = v;
#endif

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_LMS;
		return (0);
	}
	return (EPASSTHROUGH);
}

static int
lmsintr(void *arg)
{
	struct lms_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char hi, lo;
	signed char dx, dy;
	u_int buttons;
	int changed;

	if (!sc->sc_enabled)
		/* Interrupts are not expected. */
		return 0;

	bus_space_write_1(iot, ioh, LMS_CNTRL, 0xab);
	hi = bus_space_read_1(iot, ioh, LMS_DATA);
	bus_space_write_1(iot, ioh, LMS_CNTRL, 0x90);
	lo = bus_space_read_1(iot, ioh, LMS_DATA);
	dx = ((hi & 0x0f) << 4) | (lo & 0x0f);
	/* Bounding at -127 avoids a bug in XFree86. */
	dx = (dx == -128) ? -127 : dx;

	bus_space_write_1(iot, ioh, LMS_CNTRL, 0xf0);
	hi = bus_space_read_1(iot, ioh, LMS_DATA);
	bus_space_write_1(iot, ioh, LMS_CNTRL, 0xd0);
	lo = bus_space_read_1(iot, ioh, LMS_DATA);
	dy = ((hi & 0x0f) << 4) | (lo & 0x0f);
	dy = (dy == -128) ? 127 : -dy;

	bus_space_write_1(iot, ioh, LMS_CNTRL, 0);

	buttons = ((hi & 0x80) ? 0 : 0x1) |
		((hi & 0x40) ? 0 : 0x2) |
		((hi & 0x20) ? 0 : 0x4);
	changed = (buttons ^ sc->oldbuttons);
	sc->oldbuttons = buttons;

	if (dx || dy || changed)
		wsmouse_input(sc->sc_wsmousedev,
				buttons,
				dx, dy, 0, 0,
				WSMOUSE_INPUT_DELTA);

	return -1;
}
