/*	$NetBSD: j6x0tp.c,v 1.15.4.1 2007/03/12 05:48:16 rmind Exp $ */

/*
 * Copyright (c) 2003 Valeriy E. Ushakov
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
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: j6x0tp.c,v 1.15.4.1 2007/03/12 05:48:16 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include "opt_j6x0tp.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/hpc/hpctpanelvar.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/intr.h>

#include <sh3/exception.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>
#include <sh3/adcreg.h>

#include <sh3/dev/adcvar.h>


#define J6X0TP_DEBUG
#if 0 /* XXX: disabled in favor of local version that uses printf_nolog */
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	j6x0tp_debug
#define DPRINTF_LEVEL	0
#include <machine/debug.h>
#else
#ifdef J6X0TP_DEBUG
volatile int j6x0tp_debug = 0;
#define DPRINTF_PRINTF		printf_nolog
#define DPRINTF(arg)		if (j6x0tp_debug) DPRINTF_PRINTF arg
#define DPRINTFN(n, arg)	if (j6x0tp_debug > (n)) DPRINTF_PRINTF arg
#else
#define DPRINTF(arg)		((void)0)
#define DPRINTFN(n, arg)	((void)0)
#endif
#endif


/*
 * PFC bits pertinent to Jornada 6x0 touchpanel
 */
#define PHDR_TP_PEN_DOWN	0x08

#define SCPDR_TP_SCAN_ENABLE	0x20
#define SCPDR_TP_SCAN_Y		0x02
#define SCPDR_TP_SCAN_X		0x01

/*
 * A/D converter channels to get x/y from
 */
#define ADC_CHANNEL_TP_Y	1
#define ADC_CHANNEL_TP_X	2

/*
 * Default (read: my device :) raw X/Y values for framebuffer edges.
 * XXX: defopt these?
 */
#define J6X0TP_FB_LEFT		 38
#define J6X0TP_FB_RIGHT		950
#define J6X0TP_FB_TOP		 80
#define J6X0TP_FB_BOTTOM	900

/*
 * Bottom of the n'th hard icon (n = 1..4)
 */
#define J6X0TP_HARD_ICON_MAX_Y(n) \
	(J6X0TP_FB_TOP + ((J6X0TP_FB_BOTTOM - J6X0TP_FB_TOP) / 4) * (n))


struct j6x0tp_softc {
	struct device sc_dev;

#define J6X0TP_WSMOUSE_ENABLED	0x01
#define J6X0TP_WSKBD_ENABLED	0x02
	int sc_enabled;

	int sc_hard_icon;

	struct callout sc_touch_ch;

	struct device *sc_wsmousedev;
	struct device *sc_wskbddev;

	struct tpcalib_softc sc_tpcalib; /* calibration info for wsmouse */
};


/* config machinery */
static int	j6x0tp_match(struct device *, struct cfdata *, void *);
static void	j6x0tp_attach(struct device *, struct device *, void *);

/* wsmouse accessops */
static int	j6x0tp_wsmouse_enable(void *);
static int	j6x0tp_wsmouse_ioctl(void *, u_long, void *, int,
				     struct lwp *);
static void	j6x0tp_wsmouse_disable(void *);

/* wskbd accessops */
static int	j6x0tp_wskbd_enable(void *, int);
static void	j6x0tp_wskbd_set_leds(void *, int);
static int	j6x0tp_wskbd_ioctl(void *, u_long, void *, int,
				   struct lwp *);

/* internal driver routines */
static void	j6x0tp_enable(struct j6x0tp_softc *);
static void	j6x0tp_disable(struct j6x0tp_softc *);
static int	j6x0tp_set_enable(struct j6x0tp_softc *, int, int);
static int	j6x0tp_intr(void *);
static void	j6x0tp_start_polling(void *);
static void	j6x0tp_stop_polling(struct j6x0tp_softc *);
static void	j6x0tp_callout_wsmouse(void *);
static void	j6x0tp_callout_wskbd(void *);
static void	j6x0tp_wsmouse_input(struct j6x0tp_softc *, int, int);
static void	j6x0tp_get_raw_xy(int *, int *);
static int	j6x0tp_get_hard_icon(int, int);


static const struct wsmouse_accessops j6x0tp_accessops = {
	j6x0tp_wsmouse_enable,
	j6x0tp_wsmouse_ioctl,
	j6x0tp_wsmouse_disable
};

static const struct wsmouse_calibcoords j6x0tp_default_calib = {
	0, 0, 639, 239,
	4,
	{{ J6X0TP_FB_LEFT,  J6X0TP_FB_TOP,      0,   0 },
	 { J6X0TP_FB_RIGHT, J6X0TP_FB_TOP,    639,   0 },
	 { J6X0TP_FB_LEFT,  J6X0TP_FB_BOTTOM,   0, 239 },
	 { J6X0TP_FB_RIGHT, J6X0TP_FB_BOTTOM, 639, 239 }}
};

static const struct wskbd_accessops j6x0tp_wskbd_accessops = {
	j6x0tp_wskbd_enable,
	j6x0tp_wskbd_set_leds,
	j6x0tp_wskbd_ioctl
};


#ifndef J6X0TP_SETTINGS_ICON_KEYSYM
#define J6X0TP_SETTINGS_ICON_KEYSYM	KS_Home
#endif
#ifndef J6X0TP_PGUP_ICON_KEYSYM
#define J6X0TP_PGUP_ICON_KEYSYM		KS_Prior
#endif
#ifndef J6X0TP_PGDN_ICON_KEYSYM
#define J6X0TP_PGDN_ICON_KEYSYM		KS_Next
#endif
#ifndef J6X0TP_SWITCH_ICON_KEYSYM
#define J6X0TP_SWITCH_ICON_KEYSYM	KS_End
#endif

static const keysym_t j6x0tp_wskbd_keydesc[] = {
	KS_KEYCODE(1), J6X0TP_SETTINGS_ICON_KEYSYM,
	KS_KEYCODE(2), J6X0TP_PGUP_ICON_KEYSYM,
	KS_KEYCODE(3), J6X0TP_PGDN_ICON_KEYSYM,
	KS_KEYCODE(4), J6X0TP_SWITCH_ICON_KEYSYM
};

static const struct wscons_keydesc j6x0tp_wskbd_keydesctab[] = {
	{ KB_US, 0,
	  sizeof(j6x0tp_wskbd_keydesc)/sizeof(keysym_t),
	  j6x0tp_wskbd_keydesc
	},
	{0, 0, 0, 0}
};

static const struct wskbd_mapdata j6x0tp_wskbd_keymapdata = {
        j6x0tp_wskbd_keydesctab, KB_US
};


CFATTACH_DECL(j6x0tp, sizeof(struct j6x0tp_softc),
    j6x0tp_match, j6x0tp_attach, NULL, NULL);


static int
j6x0tp_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/*
	 * XXX: platid_mask_MACH_HP_LX also matches 360LX.  It's not
	 * confirmed whether touch panel in 360LX is connected this
	 * way.  We may need to regroup platid masks.
	 */
	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX)
	    && !platid_match(&platid, &platid_mask_MACH_HP_LX))
		return (0);

	if (strcmp(cf->cf_name, "j6x0tp") != 0)
		return (0);

	return (1);
}


/*
 * Attach the touch panel driver and its ws* children.
 *
 * Note that we have to use submatch to distinguish between children
 * because ws{kbd,mouse}_match match unconditionally.
 */
static void
j6x0tp_attach(struct device *parent, struct device *self, void *aux)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	struct wsmousedev_attach_args wsma;
	struct wskbddev_attach_args wska;

	printf("\n");

	sc->sc_enabled = 0;
	sc->sc_hard_icon = 0;

	/* touch-panel as a pointing device */
	wsma.accessops = &j6x0tp_accessops;
	wsma.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &wsma,
					    wsmousedevprint);
	if (sc->sc_wsmousedev == NULL)
		return;

	/* on-screen "hard icons" as a keyboard device */
	wska.console = 0;
	wska.keymap = &j6x0tp_wskbd_keymapdata;
	wska.accessops = &j6x0tp_wskbd_accessops;
	wska.accesscookie = sc;

	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &wska,
					  wskbddevprint);

	/* init calibration, set default parameters */
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		      (void *)__UNCONST(&j6x0tp_default_calib), 0, 0);

	/* used when in polling mode */
	callout_init(&sc->sc_touch_ch);

	/* establish interrupt handler, but disable until opened */
	intc_intr_establish(SH7709_INTEVT2_IRQ3, IST_EDGE, IPL_TTY,
			    j6x0tp_intr, sc);
	intc_intr_disable(SH7709_INTEVT2_IRQ3);
}


/*
 * Enable touch panel:  we start in interrupt mode.
 * Must be called as spltty().
 */
static void
j6x0tp_enable(struct j6x0tp_softc *sc)
{

	DPRINTFN(2, ("%s: enable\n", sc->sc_dev.dv_xname));
	intc_intr_enable(SH7709_INTEVT2_IRQ3);
}


/*
 * Disable touch panel: disable interrupt, cancel pending callout.
 * Must be called as spltty().
 */
static void
j6x0tp_disable(struct j6x0tp_softc *sc)
{

	DPRINTFN(2, ("%s: disable\n", sc->sc_dev.dv_xname));
	intc_intr_disable(SH7709_INTEVT2_IRQ3);
	callout_stop(&sc->sc_touch_ch);
}


static int
j6x0tp_set_enable(struct j6x0tp_softc *sc, int on, int child)
{
	int s = spltty();

	if (on) {
		if (!sc->sc_enabled)
			j6x0tp_enable(sc);
		sc->sc_enabled |= child;
	} else {
		sc->sc_enabled &= ~child;
		if (!sc->sc_enabled)
			j6x0tp_disable(sc);
	}

	splx(s);
	return (0);
}


static int
j6x0tp_wsmouse_enable(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;

	DPRINTFN(1, ("%s: wsmouse enable\n", sc->sc_dev.dv_xname));
	return (j6x0tp_set_enable(sc, 1, J6X0TP_WSMOUSE_ENABLED));
}


static void
j6x0tp_wsmouse_disable(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;

	DPRINTFN(1, ("%s: wsmouse disable\n", sc->sc_dev.dv_xname));
	j6x0tp_set_enable(sc, 0, J6X0TP_WSMOUSE_ENABLED);
}


static int
j6x0tp_wskbd_enable(void *self, int on)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;

	DPRINTFN(1, ("%s: wskbd %sable\n", sc->sc_dev.dv_xname,
		     on ? "en" : "dis"));
	return (j6x0tp_set_enable(sc, on, J6X0TP_WSKBD_ENABLED));
}


static int
j6x0tp_intr(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;

	uint8_t irr0;
	uint8_t phdr, touched;
	unsigned int steady, tremor_timeout;

	irr0 = _reg_read_1(SH7709_IRR0);
	if ((irr0 & IRR0_IRQ3) == 0) {
#ifdef DIAGNOSTIC
		printf("%s: irr0 %02x?\n", sc->sc_dev.dv_xname, irr0);
#endif
		return (0);
	}

	if (!sc->sc_enabled) {
		DPRINTFN(1, ("%s: intr: !sc_enabled\n", sc->sc_dev.dv_xname));
		intc_intr_disable(SH7709_INTEVT2_IRQ3);
		goto served;
	}


	/*
	 * Number of times the "touched" bit should be read
	 * consecutively.
	 */
#	define TREMOR_THRESHOLD 0x300

	steady = 0;
	tremor_timeout = TREMOR_THRESHOLD * 16;	/* XXX: arbitrary */
	touched = PHDR_TP_PEN_DOWN;	/* we start with "touched" state */

	do {
		phdr = _reg_read_1(SH7709_PHDR);

		if ((phdr & PHDR_TP_PEN_DOWN) == touched)
			++steady;
		else {
			steady = 0;
			touched = phdr & PHDR_TP_PEN_DOWN;
		}

		if (--tremor_timeout == 0) {
			DPRINTF(("%s: tremor timeout!\n",
				 sc->sc_dev.dv_xname));
			goto served;
		}
	} while (steady < TREMOR_THRESHOLD);

	if (touched) {
		intc_intr_disable(SH7709_INTEVT2_IRQ3);

		/*
		 * ADC readings are not stable yet, so schedule
		 * callout instead of accessing ADC from the interrupt
		 * handler only to immediately delay().
		 */
		callout_reset(&sc->sc_touch_ch, hz/32,
			      j6x0tp_start_polling, sc);
	} else
		DPRINTFN(1, ("%s: tremor\n", sc->sc_dev.dv_xname));
  served:
	/* clear the interrupt (XXX: protect access?) */
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ3);

	return (1);
}


/*
 * Called from the interrupt handler at spltty() upon first touch.
 * Decide if we are going to report this touch as a mouse click/drag
 * or as a key press.
 */
static void
j6x0tp_start_polling(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	uint8_t phdr;
	int do_mouse, do_kbd;
	int rawx, rawy;
	int icon;

	phdr = _reg_read_1(SH7709_PHDR);
	if ((phdr & PHDR_TP_PEN_DOWN) == 0) {
		DPRINTFN(2, ("%s: start: pen is not down\n",
			     sc->sc_dev.dv_xname));
		j6x0tp_stop_polling(sc);
	}

	j6x0tp_get_raw_xy(&rawx, &rawy);
	DPRINTFN(2, ("%s: start: %4d %4d -> ",
		     sc->sc_dev.dv_xname, rawx, rawy));

	do_mouse = sc->sc_enabled & J6X0TP_WSMOUSE_ENABLED;
#ifdef J6X0TP_WSMOUSE_EXCLUSIVE
	if (do_mouse)
		do_kbd = 0;
	else
#endif
		do_kbd = sc->sc_enabled & J6X0TP_WSKBD_ENABLED;

	icon = 0;
	if (do_kbd)
		icon = j6x0tp_get_hard_icon(rawx, rawy);

	if (icon != 0) {
		DPRINTFN(2, ("icon %d\n", icon));
		sc->sc_hard_icon = icon;
		wskbd_input(sc->sc_wskbddev, WSCONS_EVENT_KEY_DOWN, icon);
		callout_reset(&sc->sc_touch_ch, hz/32,
			      j6x0tp_callout_wskbd, sc);
	} else if (do_mouse) {
		DPRINTFN(2, ("mouse\n"));
		j6x0tp_wsmouse_input(sc, rawx, rawy);
		callout_reset(&sc->sc_touch_ch, hz/32,
			      j6x0tp_callout_wsmouse, sc);
	} else {
		DPRINTFN(2, ("ignore\n"));
		j6x0tp_stop_polling(sc);
	}
}


/*
 * Re-enable touch panel interrupt.
 * Called as spltty() when polling code detects pen-up.
 */
static void
j6x0tp_stop_polling(struct j6x0tp_softc *sc)
{
	uint8_t irr0;

	DPRINTFN(2, ("%s: stop\n", sc->sc_dev.dv_xname));

	/* clear pending interrupt signal before re-enabling the interrupt */
	irr0 = _reg_read_1(SH7709_IRR0);
	if ((irr0 & IRR0_IRQ3) != 0)
		_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ3);

	intc_intr_enable(SH7709_INTEVT2_IRQ3);
}


/*
 * We are reporting this touch as a keyboard event.
 * Poll touch screen waiting for pen-up.
 */
static void
j6x0tp_callout_wskbd(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	uint8_t phdr;
	int s;

	s = spltty();

	if (!sc->sc_enabled) {
		DPRINTFN(1, ("%s: wskbd callout: !sc_enabled\n",
			     sc->sc_dev.dv_xname));
		splx(s);
		return;
	}

	phdr = _reg_read_1(SH7709_PHDR);
	if ((phdr & PHDR_TP_PEN_DOWN) != 0) {
		/*
		 * Pen is still down, continue polling.  Wskbd's
		 * auto-repeat takes care of repeating the key.
		 */
		callout_schedule(&sc->sc_touch_ch, hz/32);
	} else {
		wskbd_input(sc->sc_wskbddev,
			    WSCONS_EVENT_KEY_UP, sc->sc_hard_icon);
		j6x0tp_stop_polling(sc);
	}
	splx(s);
}


/*
 * We are reporting this touch as a mouse click/drag.
 */
static void
j6x0tp_callout_wsmouse(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	uint8_t phdr;
	int rawx, rawy;
	int s;

	s = spltty();

	if (!sc->sc_enabled) {
		DPRINTFN(1, ("%s: wsmouse callout: !sc_enabled\n",
			     sc->sc_dev.dv_xname));
		splx(s);
		return;
	}

	phdr = _reg_read_1(SH7709_PHDR);
	if ((phdr & PHDR_TP_PEN_DOWN) != 0) {
		j6x0tp_get_raw_xy(&rawx, &rawy);
		j6x0tp_wsmouse_input(sc, rawx, rawy); /* mouse dragged */
		callout_schedule(&sc->sc_touch_ch, hz/32);
	} else {
		wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0, /* button up */
			      WSMOUSE_INPUT_DELTA);
		j6x0tp_stop_polling(sc);
	}
	splx(s);
}


/*
 * Report mouse click/drag.
 */
static void
j6x0tp_wsmouse_input(struct j6x0tp_softc *sc, int rawx, int rawy)
{
	int x, y;

	tpcalib_trans(&sc->sc_tpcalib, rawx, rawy, &x, &y);
		
	DPRINTFN(3, ("%s: %4d %4d -> %3d %3d\n",
		     sc->sc_dev.dv_xname, rawx, rawy, x, y));

	wsmouse_input(sc->sc_wsmousedev,
			1,	/* button */
			x, y, 0, 0,
			WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
}


/*
 * Read raw X/Y coordinates from the ADC.
 * XXX: protect accesses to SCPDR?
 */
static void
j6x0tp_get_raw_xy(int *rawxp, int *rawyp)
{
	uint8_t scpdr;

	/* Y axis */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr |=  SCPDR_TP_SCAN_ENABLE;
	scpdr &= ~SCPDR_TP_SCAN_Y; /* pull low to scan */
	_reg_write_1(SH7709_SCPDR, scpdr);
	delay(10);

	*rawyp = adc_sample_channel(ADC_CHANNEL_TP_Y);

	/* X axis */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr |=  SCPDR_TP_SCAN_Y;
	scpdr &= ~SCPDR_TP_SCAN_X; /* pull low to scan */
	_reg_write_1(SH7709_SCPDR, scpdr);
	delay(10);

	*rawxp = adc_sample_channel(ADC_CHANNEL_TP_X);

	/* restore SCPDR */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr |=  SCPDR_TP_SCAN_X;
	scpdr &= ~SCPDR_TP_SCAN_ENABLE;
	_reg_write_1(SH7709_SCPDR, scpdr);
}


/*
 * Check if the (rawx, rawy) is inside one of the 4 hard icons.
 * Return the icon number 1..4, or 0 if not inside an icon.
 */
static int
j6x0tp_get_hard_icon(int rawx, int rawy)
{
	if (rawx <= J6X0TP_FB_RIGHT)
		return (0);

	if (rawy < J6X0TP_HARD_ICON_MAX_Y(1))
		return (1);
	else if (rawy < J6X0TP_HARD_ICON_MAX_Y(2))
		return (2);
	else if (rawy < J6X0TP_HARD_ICON_MAX_Y(3))
		return (3);
	else
		return (4);
}


static int
j6x0tp_wsmouse_ioctl(void *self, u_long cmd, void *data, int flag,
		     struct lwp *l)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;

	return hpc_tpanel_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
}


static int
j6x0tp_wskbd_ioctl(void *self, u_long cmd, void *data, int flag,
		     struct lwp *l)
{
	/* struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self; */

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HPC_BTN; /* may be use new type? */
		return (0);

	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return (0);

	default:
		return (EPASSTHROUGH);
	}
}


static void
j6x0tp_wskbd_set_leds(void *self, int leds)
{

	/* nothing to do*/
	return;
}
