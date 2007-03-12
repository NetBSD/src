/*	$NetBSD: psh3tp.c,v 1.7.2.2 2007/03/12 05:48:16 rmind Exp $	*/
/*
 * Copyright (c) 2005 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include "opt_psh3tp.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/hpc/hpctpanelvar.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/intr.h>

#include <sh3/exception.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>
#include <sh3/adcreg.h>

#include <sh3/dev/adcvar.h>


#ifdef PSH3TP_DEBUG
volatile int psh3tp_debug = 4;
#define DPRINTF_PRINTF		printf_nolog
#define DPRINTF(arg)		if (psh3tp_debug) DPRINTF_PRINTF arg
#define DPRINTFN(n, arg)	if (psh3tp_debug > (n)) DPRINTF_PRINTF arg
#else
#define DPRINTF(arg)		((void)0)
#define DPRINTFN(n, arg)	((void)0)
#endif


/*
 * PFC bits pertinent to PERSONA HPW-50PA touch-panel
 */
#define PHDR_TP_PEN_UP		0x40
#define SCPDR_TP_SCAN_ENABLE	0x20
#define SCPDR_TP_SCAN_DISABLE	0x01
#define SCPDR_TP_SCAN_X		0x06
#define SCPDR_TP_SCAN_Y		0x09

/*
 * A/D converter channels to get x/y from
 */
#define ADC_CHANNEL_TP_X	1
#define ADC_CHANNEL_TP_Y	0

/*
 * Default (read: my device) raw X/Y values for framebuffer edges.
 */
#define PSH3TP_FB_RIGHT		 56
#define PSH3TP_FB_LEFT		969
#define PSH3TP_FB_TOP		848
#define PSH3TP_FB_BOTTOM	121


struct psh3tp_softc {
	struct device sc_dev;

#define PSH3TP_WSMOUSE_ENABLED	0x01
	int sc_enabled;
	struct callout sc_touch_ch;
	struct device *sc_wsmousedev;
	struct tpcalib_softc sc_tpcalib; /* calibration info for wsmouse */
};


/* config machinery */
static int psh3tp_match(struct device *, struct cfdata *, void *);
static void psh3tp_attach(struct device *, struct device *, void *);

/* wsmouse accessops */
static int psh3tp_wsmouse_enable(void *);
static int psh3tp_wsmouse_ioctl(void *, u_long, void *, int, struct lwp *);
static void psh3tp_wsmouse_disable(void *);

/* internal driver routines */
static void psh3tp_enable(struct psh3tp_softc *);
static void psh3tp_disable(struct psh3tp_softc *);
static int psh3tp_set_enable(struct psh3tp_softc *, int, int);
static int psh3tp_intr(void *);
static void psh3tp_start_polling(void *);
static void psh3tp_stop_polling(struct psh3tp_softc *);
static void psh3tp_callout_wsmouse(void *);
static void psh3tp_wsmouse_input(struct psh3tp_softc *, int, int);
static void psh3tp_get_raw_xy(int *, int *);


const struct wsmouse_accessops psh3tp_accessops = {
	psh3tp_wsmouse_enable,
	psh3tp_wsmouse_ioctl,
	psh3tp_wsmouse_disable
};

static const struct wsmouse_calibcoords psh3tp_default_calib = {
	0, 0, 639, 239,
	4,
	{{ PSH3TP_FB_LEFT,  PSH3TP_FB_TOP,      0,   0 },
	 { PSH3TP_FB_RIGHT, PSH3TP_FB_TOP,    639,   0 },
	 { PSH3TP_FB_LEFT,  PSH3TP_FB_BOTTOM,   0, 239 },
	 { PSH3TP_FB_RIGHT, PSH3TP_FB_BOTTOM, 639, 239 }}
};


CFATTACH_DECL(psh3tp, sizeof(struct psh3tp_softc),
    psh3tp_match, psh3tp_attach, NULL, NULL);


/* ARGSUSED */
static int
psh3tp_match(struct device *parent __unused, struct cfdata *cf,
	     void *aux __unused)
{

	if (!platid_match(&platid, &platid_mask_MACH_HITACHI_PERSONA))
		return 0;

	if (strcmp(cf->cf_name, "psh3tp") != 0)
		return 0;

	return 1;
}


/*
 * Attach the touch panel driver and its wsmouse child.
 *
 * Note that we have to use submatch to distinguish between child because
 * wsmouse_match matches unconditionally.
 */
/* ARGSUSED */
static void
psh3tp_attach(struct device *parent __unused, struct device *self,
	      void *aux __unused)
{
	struct psh3tp_softc *sc = device_private(self);
	struct wsmousedev_attach_args wsma;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_enabled = 0;

	/* touch-panel as a pointing device */
	wsma.accessops = &psh3tp_accessops;
	wsma.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(
	    self, "wsmousedev", &wsma, wsmousedevprint);
	if (sc->sc_wsmousedev == NULL)
		return;

	/* init calibration, set default parameters */
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
	    (void *)__UNCONST(&psh3tp_default_calib), 0, 0);

	/* used when in polling mode */
	callout_init(&sc->sc_touch_ch);

	/* establish interrupt handler, but disable until opened */
	intc_intr_establish(SH7709_INTEVT2_IRQ2,
	    IST_EDGE, IPL_TTY, psh3tp_intr, sc);
	intc_intr_disable(SH7709_INTEVT2_IRQ2);
}


/*
 * Enable touch panel:  we start in interrupt mode.
 * Must be called at spltty().
 */
/* ARGSUSED */
static void
psh3tp_enable(struct psh3tp_softc *sc __unused)
{

	DPRINTFN(2, ("%s: enable\n", sc->sc_dev.dv_xname));
	intc_intr_enable(SH7709_INTEVT2_IRQ2);
}


/*
 * Disable touch panel: disable interrupt, cancel pending callout.
 * Must be called at spltty().
 */
static void
psh3tp_disable(struct psh3tp_softc *sc)
{

	DPRINTFN(2, ("%s: disable\n", sc->sc_dev.dv_xname));
	intc_intr_disable(SH7709_INTEVT2_IRQ2);
	callout_stop(&sc->sc_touch_ch);
}


static int
psh3tp_set_enable(struct psh3tp_softc *sc, int on, int child)
{
	int s = spltty();

	if (on) {
		if (!sc->sc_enabled)
			psh3tp_enable(sc);
		sc->sc_enabled |= child;
	} else {
		sc->sc_enabled &= ~child;
		if (!sc->sc_enabled)
			psh3tp_disable(sc);
	}

	splx(s);
	return 0;
}


static int
psh3tp_wsmouse_enable(void *self)
{
	struct psh3tp_softc *sc = (struct psh3tp_softc *)self;

	DPRINTFN(1, ("%s: wsmouse enable\n", sc->sc_dev.dv_xname));
	return psh3tp_set_enable(sc, 1, PSH3TP_WSMOUSE_ENABLED);
}


static void
psh3tp_wsmouse_disable(void *self)
{
	struct psh3tp_softc *sc = (struct psh3tp_softc *)self;

	DPRINTFN(1, ("%s: wsmouse disable\n", sc->sc_dev.dv_xname));
	psh3tp_set_enable(sc, 0, PSH3TP_WSMOUSE_ENABLED);
}


static int
psh3tp_intr(void *self)
{
	struct psh3tp_softc *sc = (struct psh3tp_softc *)self;

	uint8_t irr0;
	uint8_t phdr, touched;
	unsigned int steady, tremor_timeout;

	irr0 = _reg_read_1(SH7709_IRR0);
	if ((irr0 & IRR0_IRQ2) == 0) {
#ifdef DIAGNOSTIC
		printf("%s: irr0 %02x?\n", sc->sc_dev.dv_xname, irr0);
#endif
		return 0;
	}

	if (!sc->sc_enabled) {
		DPRINTFN(1, ("%s: intr: !sc_enabled\n", sc->sc_dev.dv_xname));
		intc_intr_disable(SH7709_INTEVT2_IRQ2);
		goto served;
	}

	/*
	 * Number of times the "touched" bit should be read
	 * consecutively.
	 */
#define TREMOR_THRESHOLD 0x300
	steady = 0;
	tremor_timeout = TREMOR_THRESHOLD * 16;	/* XXX: arbitrary */
	touched = true;		/* we start with "touched" state */

	do {
		uint8_t state;

		phdr = _reg_read_1(SH7709_PHDR);
		state = ((phdr & PHDR_TP_PEN_UP) != PHDR_TP_PEN_UP);

		if (state == touched)
			++steady;
		else {
			steady = 0;
			touched = state;
		}

		if (--tremor_timeout == 0) {
			DPRINTF(("%s: tremor timeout!\n", sc->sc_dev.dv_xname));
			goto served;
		}
	} while (steady < TREMOR_THRESHOLD);

	if (touched) {
		intc_intr_disable(SH7709_INTEVT2_IRQ2);

		/*
		 * ADC readings are not stable yet, so schedule
		 * callout instead of accessing ADC from the interrupt
		 * handler only to immediately delay().
		 */
		callout_reset(&sc->sc_touch_ch,
		    hz/32, psh3tp_start_polling, sc);
	} else
		DPRINTFN(1, ("%s: tremor\n", sc->sc_dev.dv_xname));
served:
	/* clear the interrupt */
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ2);

	return 1;
}


/*
 * Called from the interrupt handler at spltty() upon first touch.
 * Decide if we are going to report this touch as a mouse click/drag.
 */
static void
psh3tp_start_polling(void *self)
{
	struct psh3tp_softc *sc = (struct psh3tp_softc *)self;
	uint8_t phdr;
	int rawx, rawy;

	phdr = _reg_read_1(SH7709_PHDR);
	if ((phdr & PHDR_TP_PEN_UP) == PHDR_TP_PEN_UP) {
		DPRINTFN(2, ("%s: start: pen is not down\n",
		    sc->sc_dev.dv_xname));
		psh3tp_stop_polling(sc);
		return;
	}

	psh3tp_get_raw_xy(&rawx, &rawy);
	DPRINTFN(2, ("%s: start: %4d %4d -> ",
	    sc->sc_dev.dv_xname, rawx, rawy));

	if (sc->sc_enabled & PSH3TP_WSMOUSE_ENABLED) {
		DPRINTFN(2, ("mouse\n"));
		psh3tp_wsmouse_input(sc, rawx, rawy);
		callout_reset(&sc->sc_touch_ch,
		    hz/32, psh3tp_callout_wsmouse, sc);
	} else {
		DPRINTFN(2, ("ignore\n"));
		psh3tp_stop_polling(sc);
	}
}


/*
 * Re-enable touch panel interrupt.
 * Called at spltty() when polling code detects pen-up.
 */
/* ARGSUSED */
static void
psh3tp_stop_polling(struct psh3tp_softc *sc __unused)
{
	uint8_t irr0;

	DPRINTFN(2, ("%s: stop\n", sc->sc_dev.dv_xname));

	/* clear pending interrupt signal before re-enabling the interrupt */
	irr0 = _reg_read_1(SH7709_IRR0);
	if ((irr0 & IRR0_IRQ2) != 0)
		_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ2);

	intc_intr_enable(SH7709_INTEVT2_IRQ2);
}


/*
 * We are reporting this touch as a mouse click/drag.
 */
static void
psh3tp_callout_wsmouse(void *self)
{
	struct psh3tp_softc *sc = (struct psh3tp_softc *)self;
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
	if ((phdr & PHDR_TP_PEN_UP) != PHDR_TP_PEN_UP) {
		psh3tp_get_raw_xy(&rawx, &rawy);
		psh3tp_wsmouse_input(sc, rawx, rawy); /* mouse dragged */
		callout_schedule(&sc->sc_touch_ch, hz/32);
	} else {
		wsmouse_input( /* button up */
		    sc->sc_wsmousedev, 0, 0, 0, 0, 0, WSMOUSE_INPUT_DELTA);
		psh3tp_stop_polling(sc);
	}
	splx(s);
}


/*
 * Report mouse click/drag.
 */
static void
psh3tp_wsmouse_input(struct psh3tp_softc *sc, int rawx, int rawy)
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
 */
static void
psh3tp_get_raw_xy(int *rawxp, int *rawyp)
{
	uint8_t scpdr;

	/* X axis */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr &= ~SCPDR_TP_SCAN_DISABLE;
	scpdr |= (SCPDR_TP_SCAN_ENABLE | SCPDR_TP_SCAN_X);
	_reg_write_1(SH7709_SCPDR, scpdr);
	delay(40);

	*rawxp = adc_sample_channel(ADC_CHANNEL_TP_X);

	/* Y axis */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr &=  ~SCPDR_TP_SCAN_X;
	scpdr |= (SCPDR_TP_SCAN_ENABLE | SCPDR_TP_SCAN_Y);
	_reg_write_1(SH7709_SCPDR, scpdr);
	delay(40);

	*rawyp = adc_sample_channel(ADC_CHANNEL_TP_Y);

	/* restore SCPDR */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr &= ~(SCPDR_TP_SCAN_ENABLE | SCPDR_TP_SCAN_Y);
	scpdr |= SCPDR_TP_SCAN_DISABLE;
	_reg_write_1(SH7709_SCPDR, scpdr);
}


static int
psh3tp_wsmouse_ioctl(void *self, u_long cmd, void *data, int flag,
		     struct lwp *l)
{
	struct psh3tp_softc *sc = (struct psh3tp_softc *)self;

	return hpc_tpanel_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
}
