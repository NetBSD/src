/*
 * Copyright (c) 2001, 2002 Greg Hughes (greg@netbsd.org). All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Wscons mouse driver for DSIU TrackPoint on IBM WorkPad z50 by
 * Greg Hughes (greg@netbsd.org).
 *
 * Template for interrupt/device registration taken from vrdsu.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vrdsiu_mouse.c,v 1.4 2003/07/15 02:29:35 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrdsiureg.h>
#include <hpcmips/vr/cmureg.h>

enum vrdsiu_mouse_stat {
	VRDSIU_MOUSE_STAT_DISABLE,
	VRDSIU_MOUSE_STAT_ENABLE
};

enum vrdsiu_ps2_input_state {
	VRDSIU_PS2_INPUT_STATE_BYTE0,
	VRDSIU_PS2_INPUT_STATE_BYTE1,
	VRDSIU_PS2_INPUT_STATE_BYTE2
};

struct vrdsiu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_unit;
	void *sc_handler;
	vrip_chipset_tag_t sc_vrip;

	enum vrdsiu_mouse_stat sc_mouse_stat;

	struct device *sc_wsmousedev;
};

static int asimOld = 0;

static int vrdsiu_match __P((struct device *, struct cfdata *, void *));
static void vrdsiu_attach __P((struct device *, struct device *, void *));

static void vrdsiu_write __P((struct vrdsiu_softc *, int, unsigned short));
static unsigned short vrdsiu_read __P((struct vrdsiu_softc *, int));

/* Interrupt handlers */
static int vrdsiu_intr __P((void *));
static void vrdsiu_mouse_intr __P((struct vrdsiu_softc *));

/* Enable/disable DSIU handling */
static int vrdsiu_mouse_enable __P((void *));
static int vrdsiu_mouse_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static void vrdsiu_mouse_disable __P((void *));

/* wsmouse access ops */
const struct wsmouse_accessops vrdsiu_accessops = {
	vrdsiu_mouse_enable,
	vrdsiu_mouse_ioctl,
	vrdsiu_mouse_disable
};

CFATTACH_DECL(vrdsiu_mouse, sizeof(struct vrdsiu_softc),
    vrdsiu_match, vrdsiu_attach, NULL, NULL);

static inline void
vrdsiu_write(sc, port, val)
	struct vrdsiu_softc *sc;
	int port;
	unsigned short val;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline unsigned short
vrdsiu_read(sc, port)
	struct vrdsiu_softc *sc;
	int port;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, port);
}

static int
vrdsiu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

static void
vrdsiu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrdsiu_softc *sc = (struct vrdsiu_softc *)self;
	struct vrip_attach_args *va = aux;
	struct wsmousedev_attach_args wsmaa;
        int res;

	bus_space_tag_t iot = va->va_iot;

        if (va->va_parent_ioh != NULL)
                res = bus_space_subregion(iot, va->va_parent_ioh, va->va_addr,
                    va->va_size, &sc->sc_ioh);
        else
                res = bus_space_map(iot, va->va_addr, va->va_size, 0,
                    &sc->sc_ioh);
	if (res != 0) {
		printf(": can't map bus space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_unit = va->va_unit;
	sc->sc_vrip = va->va_vc;

	/* install interrupt handler and enable interrupt */
	if (!(sc->sc_handler =
            vrip_intr_establish(sc->sc_vrip, sc->sc_unit, 0, IPL_TTY,
                vrdsiu_intr, sc))) {
		printf(": can't map interrupt line\n");
		return;
	}

	/* disable the handler initially */
	vrdsiu_mouse_disable(sc);

	printf("\n");

	/* attach the mouse */
	wsmaa.accessops = &vrdsiu_accessops;
	wsmaa.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &wsmaa, wsmousedevprint);

	/*
	 * TODO: Initialize the DSIU ourselves.
	 *       We currently assume WinCE has set up the DSIU properly
	 */

	asimOld = vrdsiu_read(sc, DSIUASIM00_REG_W);

	/* supply clock to the DSIU */
	vrip_power(sc->sc_vrip, sc->sc_unit, 1);
}

int
vrdsiu_mouse_enable(v)
	void *v;
{
	struct vrdsiu_softc *sc = v;

	/* ensure the DSIU mouse is currently disabled! */
	if (sc->sc_mouse_stat != VRDSIU_MOUSE_STAT_DISABLE)
		return EBUSY;

	/* enable the DSIU mouse */
	sc->sc_mouse_stat = VRDSIU_MOUSE_STAT_ENABLE;

	/* unmask interrupts */
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, (1 << 8) | (1 << 9) | (1 << 10), 1);
	vrip_power(sc->sc_vrip, sc->sc_unit, 1);

	return 0;
}

void
vrdsiu_mouse_disable(v)
	void *v;
{
	struct vrdsiu_softc *sc = v;

	/* mask interrupts */
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, (1 << 8) | (1 << 9) | (1 << 10), 0);

	/* disable the DSIU mouse */
	sc->sc_mouse_stat = VRDSIU_MOUSE_STAT_DISABLE;
}

int
vrdsiu_mouse_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	/*struct vrdsiu_softc *sc = v;*/

	switch (cmd)
	{
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		break;

	case WSMOUSEIO_SRES:
		/*
		 * TODO: Handle setting mouse resolution
		 */
		break;

	default:
		return -1;
	}

	return 0;
}

int
vrdsiu_intr(arg)
	void *arg;
{
	struct vrdsiu_softc *sc = arg;

	vrdsiu_mouse_intr(sc);

	return 0;
}

/*
 * PS/2 protocol defines
 */
#define PS2_L_BUTTON_MASK (1 << 0)
#define PS2_R_BUTTON_MASK (1 << 1)
#define PS2_M_BUTTON_MASK (1 << 2)
#define PS2_BYTE0_BIT3_MASK (1 << 3)
#define PS2_DX_SIGN_MASK (1 << 4)
#define PS2_DY_SIGN_MASK (1 << 5)
#define PS2_DX_OVERFLOW_MASK (1 << 6)
#define PS2_DY_OVERFLOW_MASK (1 << 7)

/*
 * WSCONS defines
 */
#define WSC_L_BUTTON 0x01
#define WSC_M_BUTTON 0x02
#define WSC_R_BUTTON 0x04

void
vrdsiu_mouse_intr(sc)
	struct vrdsiu_softc *sc;
{
	u_int intrReason;
	unsigned char b;
	
	static int dx;
	static int dy;
	static u_char buttons;
	static enum vrdsiu_ps2_input_state ps2_state = 0;

	/* What caused the interrupt? */
	intrReason = vrdsiu_read(sc, DSIUINTR0_REG_W);
		
	/*
	 * TODO: Check for error conditions; specifically need to handle
	 *       overruns (which currently mess up the mouse).
	 */

	/* disable reception */
	vrdsiu_write(sc, DSIUASIM00_REG_W, asimOld & ~DSIUASIM00_RXE0);

	if (intrReason & DSIUINTR0_INTSER0)
		ps2_state = 0;

	/* Ignore everything except receive notifications */
	if ((intrReason & DSIUINTR0_INTSR0) == 0)
		goto done;

	/* read from DSIU */
	b = (unsigned char)vrdsiu_read(sc, DSIURXB0L_REG_W);

	/* check if the DSIU is enabled */
	if (sc->sc_mouse_stat == VRDSIU_MOUSE_STAT_DISABLE)
	{
		/* Throw away input to keep the DSIU's buffer clean */
		goto done;
	}

	/*
	 * PS/2 protocol interpretation
	 *
	 * A PS/2 packet consists of 3 bytes.  We read them in, in order, and
	 * piece together the mouse info.
	 *
	 * Byte 0 contains button info and dx/dy signedness
	 * Byte 1 contains dx info
	 * Byte 2 contains dy info
	 *
	 * Please see PS/2 specs for detailed information; brief descriptions
	 * are provided below.
	 *
	 * PS/2 mouse specs for the TrackPoint from IBM's TrackPoint Engineering
	 * Specification Version 4.0.
	 */
	switch (ps2_state)
	{
	case VRDSIU_PS2_INPUT_STATE_BYTE0:
		/* Bit 3 of byte 0 is always 1; we use that info to sync input */
		if ((b & PS2_BYTE0_BIT3_MASK) == 0)
			goto done;

		if (b & (PS2_M_BUTTON_MASK | PS2_DX_OVERFLOW_MASK |
			PS2_DY_OVERFLOW_MASK))
			goto done;

		/* Extract button state */
		buttons = ((b & PS2_L_BUTTON_MASK) ? WSC_L_BUTTON : 0)
			| ((b & PS2_M_BUTTON_MASK) ? WSC_M_BUTTON : 0)
			| ((b & PS2_R_BUTTON_MASK) ? WSC_R_BUTTON : 0);

		/* Extract dx/dy signedness -- 9-bit 2's comp */
		/* dx = (b & PS2_DX_SIGN_MASK) ? 0xFFFFFFFF : 0;
		dy = (b & PS2_DY_SIGN_MASK) ? 0xFFFFFFFF : 0; */

		ps2_state = VRDSIU_PS2_INPUT_STATE_BYTE1;
		break;

	case VRDSIU_PS2_INPUT_STATE_BYTE1:
		/* put in the lower 8 bits of dx */
		dx = (signed char)b;
		if (dx == -128)
			dx = -127;
		
		ps2_state = VRDSIU_PS2_INPUT_STATE_BYTE2;
		break;

	case VRDSIU_PS2_INPUT_STATE_BYTE2:
		/* put in the lower 8 bits of dy */
		dy = (signed char)b;
		if (dy == -128)
			dy = -127;

		/* We now have a complete packet; send to wscons */
		wsmouse_input(sc->sc_wsmousedev,
			buttons,
			dx,
			dy,
			0,
			WSMOUSE_INPUT_DELTA);

		ps2_state = VRDSIU_PS2_INPUT_STATE_BYTE0;
		break;
	}

done:
	/* clear the interrupt */
	vrdsiu_write(sc, DSIUINTR0_REG_W, intrReason);

	/* enable reception */
	vrdsiu_write(sc, DSIUASIM00_REG_W, asimOld | DSIUASIM00_RXE0);
}
