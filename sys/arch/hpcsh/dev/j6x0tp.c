/*	$NetBSD: j6x0tp.c,v 1.1 2003/10/19 02:20:25 uwe Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: j6x0tp.c,v 1.1 2003/10/19 02:20:25 uwe Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/callout.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/hpc/tpcalibvar.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <machine/intr.h>

#include <sh3/exception.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>
#include <sh3/adcreg.h>

#define J6X0TP_DEBUG
#if 0 /* XXX: disabled in favor of local version that uses printf_nolog */
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	j6x0tp_debug
#define DPRINTF_LEVEL	0
#include <machine/debug.h>
#else
#ifdef J6X0TP_DEBUG
volatile int j6x0tp_debug = 0;
#define	DPRINTF(arg)		if (j6x0tp_debug) printf_nolog arg
#define DPRINTFN(n, arg)	if (j6x0tp_debug > (n)) printf_nolog arg
#else
#define	DPRINTF(arg)		((void)0)
#define DPRINTFN(n, arg)	((void)0)
#endif
#endif


/* PFC bits pertinent to Jornada 6x0 touchpanel */
#define PHDR_TP_PEN_DOWN	0x08

#define SCPDR_TP_SCAN_ENABLE	0x20
#define SCPDR_TP_SCAN_Y		0x02
#define SCPDR_TP_SCAN_X		0x01

/* A/D covnerter channels to get x/y from */
#define ADC_CHANNEL_TP_Y	1
#define ADC_CHANNEL_TP_X	2

extern int	adc_sample_channel(int); /* XXX: adcvar.h */


struct j6x0tp_softc {
	struct device sc_dev;

	struct callout sc_touch_ch;
	int sc_intrlevel;
	int sc_enabled;

	struct device *sc_wsmousedev;
	struct tpcalib_softc sc_tpcalib;
};

static int	j6x0tp_match(struct device *, struct cfdata *, void *);
static void	j6x0tp_attach(struct device *, struct device *, void *);

CFATTACH_DECL(j6x0tp, sizeof(struct j6x0tp_softc),
    j6x0tp_match, j6x0tp_attach, NULL, NULL);


static int  j6x0tp_enable(void *);
static int  j6x0tp_ioctl(void *, u_long, caddr_t, int, struct proc *);
static void j6x0tp_disable(void *);

const struct wsmouse_accessops j6x0tp_accessops = {
	j6x0tp_enable,
	j6x0tp_ioctl,
	j6x0tp_disable,
};

static const struct wsmouse_calibcoords j6x0tp_default_calib = {
	0, 0, 639, 239,
	4,
	{{  38,  80,   0,   0 }, /* upper left  */
	 { 950,  80, 639,   0 }, /* upper right */
	 {  38, 900,   0, 239 }, /* lower left  */
	 { 950, 900, 639, 239 }} /* lower right */
};


static int	j6x0tp_intr(void *);
static void	j6x0tp_poll_callout(void *self);


static int
j6x0tp_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	/*
	 * XXX: does platid_mask_MACH_HP_LX matches _JORNADA_6XX too?
	 * Is 620 wired similarly?
	 */
	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX))
		return (0);

	if (strcmp(cfp->cf_name, "j6x0tp") != 0)
		return (0);

	return (1);
}


static void
j6x0tp_attach(struct device *parent, struct device *self, void *aux)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	struct wsmousedev_attach_args wsma;

	printf("\n");

	sc->sc_enabled = 0;

	wsma.accessops = &j6x0tp_accessops;
	wsma.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &wsma, wsmousedevprint);
	if (sc->sc_wsmousedev == NULL)
		return;

	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		      (caddr_t)&j6x0tp_default_calib, 0, 0);

	callout_init(&sc->sc_touch_ch);

	intc_intr_establish(SH7709_INTEVT2_IRQ3, IST_EDGE, IPL_TTY,
			    j6x0tp_intr, sc);
	sc->sc_intrlevel = intc_intr_disable(SH7709_INTEVT2_IRQ3);
}


static int
j6x0tp_enable(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	int s;

	DPRINTFN(1, ("%s: enable\n", sc->sc_dev.dv_xname));

	s = spltty();

	sc->sc_enabled = 1;
	intc_intr_enable(SH7709_INTEVT2_IRQ3, sc->sc_intrlevel);

	splx(s);
	return (0);
}


static void
j6x0tp_disable(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	int s;

	s = spltty();

	DPRINTFN(1, ("%s: disable\n", sc->sc_dev.dv_xname));

	sc->sc_enabled = 0;
	intc_intr_disable(SH7709_INTEVT2_IRQ3);
	callout_stop(&sc->sc_touch_ch);

	splx(s);
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

	if (touched == 0) {
		DPRINTFN(1, ("%s: tremor\n", sc->sc_dev.dv_xname));
		goto served;
	}

	intc_intr_disable(SH7709_INTEVT2_IRQ3);

	if (sc->sc_enabled)
		callout_reset(&sc->sc_touch_ch, hz/32,
			      j6x0tp_poll_callout, sc);
	else
		DPRINTFN(1, ("%s: intr: !sc_enabled\n", sc->sc_dev.dv_xname));

  served:
	/* clear the interrupt */
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ3);

	return (1);
}


static void
j6x0tp_poll_callout(void *self)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;
	uint8_t phdr, scpdr;
	uint8_t irr0;
	int rawx, rawy, x, y;
	int s;

	s = spltty();

	if (!sc->sc_enabled) {
		DPRINTFN(1, ("%s: callout: !sc_enabled\n",
			     sc->sc_dev.dv_xname));
		splx(s);
		return;
	}

	phdr = _reg_read_1(SH7709_PHDR);
	if ((phdr & PHDR_TP_PEN_DOWN) == 0) {
		wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0); /* mouse up */

		intc_intr_enable(SH7709_INTEVT2_IRQ3, sc->sc_intrlevel);

		irr0 = _reg_read_1(SH7709_IRR0);
		if ((irr0 & IRR0_IRQ3) != 0)
			_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ3);

		splx(s);
		return;
	}

	/* XXX: protect accesses to SCPDR? */

	/* Y axis */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr |=  SCPDR_TP_SCAN_ENABLE;
	scpdr &= ~SCPDR_TP_SCAN_Y; /* pull low to scan */
	_reg_write_1(SH7709_SCPDR, scpdr);
	delay(10);
	rawy = adc_sample_channel(ADC_CHANNEL_TP_Y);

	/* X axis */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr |=  SCPDR_TP_SCAN_Y;
	scpdr &= ~SCPDR_TP_SCAN_X; /* pull low to scan */
	_reg_write_1(SH7709_SCPDR, scpdr);
	delay(10);
	rawx = adc_sample_channel(ADC_CHANNEL_TP_X);

	/* restore SCPDR */
	scpdr = _reg_read_1(SH7709_SCPDR);
	scpdr |=  SCPDR_TP_SCAN_X;
	scpdr &= ~SCPDR_TP_SCAN_ENABLE;
	_reg_write_1(SH7709_SCPDR, scpdr);

	tpcalib_trans(&sc->sc_tpcalib, rawx, rawy, &x, &y);

	DPRINTFN(2, ("%s: %4d %4d -> %3d %3d\n",
		     sc->sc_dev.dv_xname, rawx, rawy, x, y));

	wsmouse_input(sc->sc_wsmousedev, 1, x, y, 0,
		      WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);

	callout_schedule(&sc->sc_touch_ch, hz/32);
	splx(s);
}


static int
j6x0tp_ioctl(void *self, u_long cmd, caddr_t data, int flag,
		     struct proc *p)
{
	struct j6x0tp_softc *sc = (struct j6x0tp_softc *)self;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		return (0);

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
	case WSMOUSEIO_GETID:
		return (tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, p));

	default:
		return (EPASSTHROUGH);
	}
}
