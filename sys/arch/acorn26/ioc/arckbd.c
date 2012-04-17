/* $NetBSD: arckbd.c,v 1.20.2.1 2012/04/17 00:05:51 yamt Exp $ */
/*-
 * Copyright (c) 1998, 1999, 2000 Ben Harris
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
/*
 * arckbd.c - Archimedes keyboard driver
 */

/*
 * Most of the information used to write this driver came from the
 * A3000 Technical Reference Manual (ISBN 1-85250-074-3).
 *
 * We keep a queue of one command in the softc for use by the receive
 * interrupt handler in case it finds the KART is already transmitting
 * a command (presumably as a consequence of a user request) when it
 * wants to.  I think this is safe in all cases, and it will never
 * happen more than once at a time (I hope).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arckbd.c,v 1.20.2.1 2012/04/17 00:05:51 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/reboot.h>	/* For bootverbose */
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <machine/intr.h>
#include <machine/irq.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsmousevar.h>

#include <arch/acorn26/iobus/iocreg.h>
#include <arch/acorn26/iobus/iocvar.h>
#include <arch/acorn26/ioc/arckbdreg.h>
#include <arch/acorn26/ioc/arckbdvar.h>

#include "wskbd.h"
#include "wsmouse.h"


#include <sys/rnd.h>

/* #define ARCKBD_DEBUG */

enum arckbd_state {
	AS_HRST, AS_RAK1, AS_RAK2, /* reset protocol */
	AS_IDLE, /* idle, waiting for data */
	AS_KDDA, AS_KUDA, AS_MDAT /* Receiving two-byte message */
};

static const char *arckbd_statenames[] = {
	"hrst", "rak1", "rak2", "idle", "kdda", "kuda", "mdat"
};

static int arckbd_match(device_t parent, cfdata_t cf, void *aux);
static void arckbd_attach(device_t parent, device_t self, void *aux);
#if 0 /* XXX should be used */
static kbd_t arckbd_pick_layout(int kbid);
#endif

static int arckbd_rint(void *self);
static int arckbd_xint(void *self);
static void arckbd_mousemoved(device_t self, int byte1, int byte2);
static void arckbd_keyupdown(device_t self, int byte1, int byte2);
static int arckbd_send(device_t self, int data,
    enum arckbd_state newstate, int waitok);

static int arckbd_enable(void *cookie, int on);
static int arckbd_led_encode(int);
static int arckbd_led_decode(int);
static void arckbd_set_leds(void *cookie, int new_state);
static int arckbd_ioctl(void *cookie, u_long cmd, void *data, int flag,
    struct lwp *l);
#if NWSKBD > 0
static void arckbd_getc(void *cookie, u_int *typep, int *valuep);
static void arckbd_pollc(void *cookie, int poll);
#endif

static int arcmouse_enable(void *cookie);
static int arcmouse_ioctl(void *cookie, u_long cmd, void *data, int flag,
    struct lwp *l);
static void arcmouse_disable(void *cookie);

struct arckbd_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	u_int			sc_mouse_buttons;
	enum arckbd_state	sc_state;
	u_char			sc_byteone;
	u_char			sc_kbid;
	device_t		sc_wskbddev;
	device_t		sc_wsmousedev;
	struct wskbd_mapdata	sc_mapdata;
	int			sc_cmdqueue;  /* Single-command queue */
	enum arckbd_state	sc_statequeue;
	int			sc_cmdqueued;
	int			sc_flags;
	int			sc_leds;
	u_int			sc_poll_type;
	int			sc_poll_value;
	struct irq_handler	*sc_xirq;
	struct evcnt		sc_xev;
	struct irq_handler	*sc_rirq;
	struct evcnt		sc_rev;
	krndsource_t	sc_rnd_source;
};

#define AKF_WANTKBD	0x01
#define AKF_WANTMOUSE	0x02
#define AKF_SENTRQID	0x04
#define AKF_SENTLEDS	0x08
#define AKF_POLLING	0x10

CFATTACH_DECL_NEW(arckbd, sizeof(struct arckbd_softc),
    arckbd_match, arckbd_attach, NULL, NULL);

static struct wskbd_accessops arckbd_accessops = {
	arckbd_enable, arckbd_set_leds, arckbd_ioctl
};

#if NWSKBD > 0
static struct wskbd_consops arckbd_consops = {
	arckbd_getc, arckbd_pollc
};
#endif

static struct wsmouse_accessops arcmouse_accessops = {
	arcmouse_enable, arcmouse_ioctl, arcmouse_disable
};

/* ARGSUSED */
static int
arckbd_match(device_t parent, cfdata_t cf, void *aux)
{

	/* Assume presence for now */
	return 1;
}

static void
arckbd_attach(device_t parent, device_t self, void *aux)
{
	struct arckbd_softc *sc = device_private(self);
	struct ioc_attach_args *ioc = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	struct wskbddev_attach_args wskbdargs;
	struct wsmousedev_attach_args wsmouseargs;

	bst = sc->sc_bst = ioc->ioc_fast_t;
	bsh = sc->sc_bsh = ioc->ioc_fast_h; 

	sc->sc_dev = self;
	evcnt_attach_dynamic(&sc->sc_rev, EVCNT_TYPE_INTR, NULL,
	    device_xname(sc->sc_dev), "rx intr");
	sc->sc_rirq = irq_establish(IOC_IRQ_SRX, IPL_TTY, arckbd_rint, self,
	    &sc->sc_rev);
	aprint_verbose("\n%s: interrupting at %s (rx)", device_xname(self),
	    irq_string(sc->sc_rirq));

	evcnt_attach_dynamic(&sc->sc_xev, EVCNT_TYPE_INTR, NULL,
	    device_xname(sc->sc_dev), "tx intr");
	sc->sc_xirq = irq_establish(IOC_IRQ_STX, IPL_TTY, arckbd_xint, self,
	    &sc->sc_xev);
	irq_disable(sc->sc_xirq);
	aprint_verbose(" and %s (tx)", irq_string(sc->sc_xirq));

       	/* Initialisation of IOC KART per IOC Data Sheet section 6.2.3. */

       	/* Set up IOC counter 3 */
	/* k_BAUD = 1/((latch+1)*16) MHz */
	ioc_counter_start(parent, 3, 62500 / ARCKBD_BAUD - 1);

	/* Read from Rx register and discard. */
	(void)bus_space_read_1(bst, bsh, 0);

	/* Kick the keyboard into life */
	arckbd_send(self, ARCKBD_HRST, AS_HRST, 0);

	sc->sc_mapdata = arckbd_mapdata_default;
	sc->sc_mapdata.layout = KB_UK; /* Reasonable default */

	/* Attach the wskbd console */
	arckbd_cnattach(self);

	aprint_normal("\n");

	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_TTY, 0);

	wskbdargs.console = 1; /* XXX FIXME */
	wskbdargs.keymap = &sc->sc_mapdata;
	wskbdargs.accessops = &arckbd_accessops;
	wskbdargs.accesscookie = sc;
	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &wskbdargs, NULL);

	wsmouseargs.accessops = &arcmouse_accessops;
	wsmouseargs.accesscookie = sc;
	sc->sc_wsmousedev =
	    config_found_ia(self, "wsmousedev", &wsmouseargs, NULL);
}

#if 0 /* XXX should be used */
static kbd_t
arckbd_pick_layout(int kbid)
{
	int i;

	for (i = 0; arckbd_kbidtab[i].kbid != 0; i++) {
		if (arckbd_kbidtab[i].kbid == kbid)
			return arckbd_kbidtab[i].layout;
	}
	return KB_UK;
}
#endif

/*
 * We don't really _need_ a console keyboard before
 * autoconfiguration's finished, so for now this function's written to
 * be called from arckbd_attach.  The console functions should still
 * try not to rely on much, and perhaps one day we should make it
 * happen in consinit instead.
 */

void
arckbd_cnattach(device_t self)
{
#if NWSKBD > 0
	struct arckbd_softc *sc = device_private(self);

	wskbd_cnattach(&arckbd_consops, sc, &arckbd_mapdata_default);
#endif
}

#if NWSKBD > 0
static void
arckbd_getc(void *cookie, u_int *typep, int *valuep)
{
 	struct arckbd_softc *sc = cookie;
	int s;

	if (!(sc->sc_flags & AKF_POLLING))
		panic("%s: arckbd_getc called with polling disabled",
		      device_xname(sc->sc_dev));
	while (sc->sc_poll_type == 0) {
		if (ioc_irq_status(IOC_IRQ_STX))
			arckbd_xint(sc->sc_dev);
		if (ioc_irq_status(IOC_IRQ_SRX))
			arckbd_rint(sc->sc_dev);
	}
	s = spltty();
	*typep = sc->sc_poll_type;
	*valuep = sc->sc_poll_value;
	sc->sc_poll_type = 0;
	sc->sc_poll_value = 0;
	splx(s);
}

static void
arckbd_pollc(void *cookie, int poll)
{
	struct arckbd_softc *sc = cookie;
	int s;

	s = spltty();
	if (poll) {
		sc->sc_flags |= AKF_POLLING;
		irq_disable(sc->sc_rirq);
		irq_disable(sc->sc_xirq);
	} else {
		sc->sc_flags &= ~AKF_POLLING;
		irq_enable(sc->sc_rirq);
		irq_enable(sc->sc_xirq);
	}
	splx(s);
}
#endif

static int
arckbd_send(device_t self, int data, enum arckbd_state newstate, int waitok)
{
	struct arckbd_softc *sc = device_private(self);
	int s, res;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	s = spltty();
	if (waitok) {
		while (!ioc_irq_status(IOC_IRQ_STX))
			if ((sc->sc_flags & AKF_POLLING) == 0) {
				res = tsleep(arckbd_send, PWAIT, "kbdsend", 0);
				if (res != 0)
					return res;
			}
	} else if (!ioc_irq_status(IOC_IRQ_STX)) {
		if (sc->sc_cmdqueued)
			panic("%s: queue overflow", device_xname(sc->sc_dev));
		else {
			sc->sc_cmdqueue = data;
			sc->sc_statequeue = newstate;
			sc->sc_cmdqueued = 1;
			return 0;
		}
	}
	bus_space_write_1(bst, bsh, 0, data);
	sc->sc_state = newstate;
#ifdef ARCKBD_DEBUG
	log(LOG_DEBUG, "%s: sent 0x%02x.  now in state %s\n",
	    device_xname(sc->sc_dev), data, arckbd_statenames[newstate]);
#endif
	wakeup(&sc->sc_state);
	splx(s);
	if ((sc->sc_flags & AKF_POLLING) == 0)
		irq_enable(sc->sc_xirq);
	return 0;
}

static int
arckbd_xint(void *cookie)
{
	struct arckbd_softc *sc = device_private(cookie);

	irq_disable(sc->sc_xirq);
	/* First, process queued commands (acks from the last receive) */
	if (sc->sc_cmdqueued) {
		sc->sc_cmdqueued = 0;
		arckbd_send(sc->sc_dev, sc->sc_cmdqueue, sc->sc_statequeue, 0);
	} else if (sc->sc_state == AS_IDLE) {
	/* Do things that need doing after a reset */
		if (!(sc->sc_flags & AKF_SENTRQID)) {
			arckbd_send(sc->sc_dev, ARCKBD_RQID, AS_IDLE, 0);
			sc->sc_flags |= AKF_SENTRQID;
		} else if (!(sc->sc_flags & AKF_SENTLEDS)) {
			arckbd_send(sc->sc_dev, ARCKBD_LEDS | sc->sc_leds,
				    AS_IDLE, 0);
			sc->sc_flags |= AKF_SENTLEDS;
		}
	} else if ((sc->sc_flags & AKF_POLLING) == 0)
		wakeup(&arckbd_send);
	return IRQ_HANDLED;
}

static int
arckbd_rint(void *cookie)
{
	device_t self = cookie;
	struct arckbd_softc *sc = device_private(self);
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int data;

	data = bus_space_read_1(bst, bsh, 0);
#ifdef ARCKBD_DEBUG
	log(LOG_DEBUG, "%s: got 0x%02x in state %s\n", device_xname(self), data,
	    arckbd_statenames[sc->sc_state]);
#endif
	/* Reset protocol */
	if (data == ARCKBD_HRST && sc->sc_state == AS_HRST)
		arckbd_send(self, ARCKBD_RAK1, AS_RAK1, 0);
	else if (data == ARCKBD_RAK1 &&
		 (sc->sc_state == AS_RAK1 || sc->sc_state == AS_HRST))
		arckbd_send(self, ARCKBD_RAK2, AS_RAK2, 0);
	else if (data == ARCKBD_RAK2 && sc->sc_state == AS_RAK2)
		arckbd_send(self, ARCKBD_SMAK, AS_IDLE, 0);

	/*
	 * Note that for data messages, we acknowledge first and
	 * _then_ process the data.  This is important because the
	 * processing may end up trying to use the keyboard in polled
	 * mode (e.g. through DDB) and we'd like its state to be
	 * self-consistent.
	 */

	/* Mouse data */
	else if (ARCKBD_IS_MDAT(data) && sc->sc_state == AS_IDLE) {
		arckbd_send(self, ARCKBD_BACK, AS_MDAT, 0);
		sc->sc_byteone = data;
	} else if (ARCKBD_IS_MDAT(data) && sc->sc_state == AS_MDAT) {
		arckbd_send(self, ARCKBD_SMAK, AS_IDLE, 0);
		arckbd_mousemoved(self, sc->sc_byteone, data);
	}

	/* Key down data */
	else if (ARCKBD_IS_KDDA(data) && sc->sc_state == AS_IDLE) {
		arckbd_send(self, ARCKBD_BACK, AS_KDDA, 0);
		sc->sc_byteone = data;
	} else if (ARCKBD_IS_KDDA(data) && sc->sc_state == AS_KDDA) {
		arckbd_send(self, ARCKBD_SMAK, AS_IDLE, 0);
		arckbd_keyupdown(self, sc->sc_byteone, data);
	}

	/* Key up data */
	else if (ARCKBD_IS_KUDA(data) && sc->sc_state == AS_IDLE) {
		arckbd_send(self, ARCKBD_BACK, AS_KUDA, 0);
		sc->sc_byteone = data;
	} else if (ARCKBD_IS_KUDA(data) && sc->sc_state == AS_KUDA) {
		arckbd_send(self, ARCKBD_SMAK, AS_IDLE, 0);
		arckbd_keyupdown(self, sc->sc_byteone, data);
	}

	/* Other cruft */
	else if (ARCKBD_IS_KBID(data)) {
		arckbd_send(self, ARCKBD_SMAK, AS_IDLE, 0);
		if (sc->sc_kbid != data) {
			printf("%s: layout %d\n",
			    device_xname(self), data & ~ARCKBD_KBID);
			sc->sc_kbid = data;
		}
	} else if (ARCKBD_IS_PDAT(data))
		/* unused -- ignore it */;
	else {
		/* Protocol error */
		log(LOG_WARNING, "%s: protocol error: got 0x%02x in state %s\n",
		    device_xname(self), data, arckbd_statenames[sc->sc_state]);
		arckbd_send(self, ARCKBD_HRST, AS_HRST, 0);
	}
	return IRQ_HANDLED;
}

static void
arckbd_mousemoved(device_t self, int byte1, int byte2)
{
	struct arckbd_softc *sc = device_private(self);

	rnd_add_uint32(&sc->sc_rnd_source, byte1);
#if NWSMOUSE > 0
	if (sc->sc_wsmousedev != NULL) {
		int dx, dy;

		/* deltas are 7-bit signed */
		dx = byte1 < 0x40 ? byte1 : byte1 - 0x80;
		dy = byte2 < 0x40 ? byte2 : byte2 - 0x80;
		wsmouse_input(sc->sc_wsmousedev,
				sc->sc_mouse_buttons,
				dx, dy, 0, 0,
				WSMOUSE_INPUT_DELTA);
	}
#endif
}

static void
arckbd_keyupdown(device_t self, int byte1, int byte2)
{
	struct arckbd_softc *sc = device_private(self);
	u_int type;
	int value;

	rnd_add_uint32(&sc->sc_rnd_source, byte1);
	if ((byte1 & 0x0f) == 7) {
		/* Mouse button event */
		/*
		 * This is all very silly, as the wsmouse driver then
		 * differentiates the button state to see if there's
		 * an event worth passing to the user.
		 * 
		 * Oh well, at least NetBSD and Acorn number their
		 * mouse buttons the same way.
		 */
		if (ARCKBD_IS_KDDA(byte1))
			sc->sc_mouse_buttons |= (1 << (byte2 & 0x0f));
		else
			sc->sc_mouse_buttons &= ~(1 << (byte2 & 0x0f));
#if NWSMOUSE > 0
		if (sc->sc_wsmousedev != NULL)
			wsmouse_input(sc->sc_wsmousedev,
					sc->sc_mouse_buttons,
					0, 0, 0, 0,
					WSMOUSE_INPUT_DELTA);
#endif
	} else {
		type = ARCKBD_IS_KDDA(byte1) ?
			WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
		value = ((byte1 & 0x0f) << 4) | (byte2 & 0x0f);
		if (sc->sc_flags & AKF_POLLING) {
			sc->sc_poll_type = type;
			sc->sc_poll_value = value;
		}
#if NWSKBD > 0
		else if (sc->sc_wskbddev != NULL)
			wskbd_input(sc->sc_wskbddev, type, value);
#endif
	}
}

/*
 * Keyboard access functions
 */

static int
arckbd_enable(void *cookie, int on)
{
	struct arckbd_softc *sc = cookie;

	if (on) {
		sc->sc_flags |= AKF_WANTKBD;
		/* XXX send RQMP? */
	} else
		sc->sc_flags &= ~ AKF_WANTKBD;
	return 0;
}

static int
arckbd_led_encode(int wsleds)
{
	int arcleds;

	arcleds = 0;
	if (wsleds & WSKBD_LED_CAPS)
		arcleds |= ARCKBD_LEDS_CAPSLOCK;
	if (wsleds & WSKBD_LED_NUM)
		arcleds |= ARCKBD_LEDS_NUMLOCK;
	if (wsleds & WSKBD_LED_SCROLL)
		arcleds |= ARCKBD_LEDS_SCROLLLOCK;
	/* No "compose" LED */
	return arcleds;
}

static int
arckbd_led_decode(int arcleds)
{
	int wsleds;

	wsleds = 0;
	if (arcleds & ARCKBD_LEDS_CAPSLOCK)
		wsleds |= WSKBD_LED_CAPS;
	if (arcleds & ARCKBD_LEDS_NUMLOCK)
		wsleds |= WSKBD_LED_NUM;
	if (arcleds & ARCKBD_LEDS_SCROLLLOCK)
		wsleds |= WSKBD_LED_SCROLL;
	/* No "compose" LED */
	return wsleds;
}

/*
 * Set the LEDs to the requested state.
 *
 * Be warned: This function gets called from interrupts.
 */
static void
arckbd_set_leds(void *cookie, int new_state)
{
	struct arckbd_softc *sc = cookie;
	int s;

	s = spltty();
	sc->sc_leds = arckbd_led_encode(new_state);
	if (arckbd_send(sc->sc_dev, ARCKBD_LEDS | sc->sc_leds, AS_IDLE, 0) == 0)
		sc->sc_flags |= AKF_SENTLEDS;
	splx(s);
}

/* ARGSUSED */
static int
arckbd_ioctl(void *cookie, u_long cmd, void *data, int flag, struct lwp *l)
{
 	struct arckbd_softc *sc = cookie;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_ARCHIMEDES;
		return 0;
	case WSKBDIO_SETLEDS:
		arckbd_set_leds(sc, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = arckbd_led_decode(sc->sc_leds);
		return 0;
	}
	return EPASSTHROUGH;
}

/*
 * Mouse access functions
 */

static int
arcmouse_enable(void *cookie)
{
	struct arckbd_softc *sc = cookie;

	sc->sc_flags |= AKF_WANTMOUSE;
	/* XXX send RQMP */
	return 0;
}

/* ARGSUSED */
static int
arcmouse_ioctl(void *cookie, u_long cmd, void *data, int flag, struct lwp *l)
{
/*	struct arckbd_softc *sc = cookie; */

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(int *)data = WSMOUSE_TYPE_ARCHIMEDES;
		return 0;
	}
	return EPASSTHROUGH;
}

static void
arcmouse_disable(void *cookie)
{
	struct arckbd_softc *sc = cookie;

	sc->sc_flags &= ~AKF_WANTMOUSE;
}
