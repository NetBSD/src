/*	$NetBSD: j720tp.c,v 1.10 2009/05/29 14:15:45 rjs Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Peter Postma.
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

/* Jornada 720 touch-panel driver. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720tp.c,v 1.10 2009/05/29 14:15:45 rjs Exp $");

#ifdef _KERNEL_OPT
#include "opt_j720tp.h"
#include "opt_wsdisplay_compat.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/callout.h>
#include <sys/sysctl.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/hpc/hpctpanelvar.h>

#include <hpcarm/dev/j720sspvar.h>

#ifdef WSDISPLAY_COMPAT_RAWKBD
#include <dev/hpc/pckbd_encode.h>
#endif

#define arraysize(ary)	(sizeof(ary) / sizeof(ary[0]))

#ifdef J720TP_DEBUG
int j720tp_debug = 0;
#define DPRINTF(arg)		if (j720tp_debug) aprint_normal arg
#define DPRINTFN(n, arg)	if (j720tp_debug >= (n)) aprint_normal arg
#else
#define DPRINTF(arg)		/* nothing */
#define DPRINTFN(n, arg)	/* nothing */
#endif

struct j720tp_softc {
	device_t		sc_dev;

#define J720TP_WSMOUSE_ENABLED	0x01
#define J720TP_WSKBD_ENABLED	0x02
	int			sc_enabled;
	int			sc_hard_icon;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int			sc_rawkbd;
#endif

	struct callout		sc_tpcallout;
	struct j720ssp_softc	*sc_ssp;

	device_t		sc_wsmousedev;
	device_t		sc_wskbddev;

	struct tpcalib_softc	sc_tpcalib;
};

static int	j720tp_match(device_t, cfdata_t, void *);
static void	j720tp_attach(device_t, device_t, void *);

static int	j720tp_wsmouse_enable(void *);
static int	j720tp_wsmouse_ioctl(void *, u_long, void *, int,
			struct lwp *);
static void	j720tp_wsmouse_disable(void *);

static int	j720tp_wskbd_enable(void *, int);
static void	j720tp_wskbd_set_leds(void *, int);
static int	j720tp_wskbd_ioctl(void *, u_long, void *, int, struct lwp *);

static void	j720tp_enable_intr(struct j720tp_softc *);
static void	j720tp_disable_intr(struct j720tp_softc *);
static int	j720tp_intr(void *);
static int	j720tp_get_rawxy(struct j720tp_softc *, int *, int *);
static int	j720tp_get_hard_icon(struct j720tp_softc *, int, int);
static void	j720tp_wsmouse_input(struct j720tp_softc *, int, int);
static void	j720tp_wsmouse_callout(void *);
static void	j720tp_wskbd_input(struct j720tp_softc *, u_int);
static void	j720tp_wskbd_callout(void *);

const struct wsmouse_accessops j720tp_wsmouse_accessops = {
	j720tp_wsmouse_enable,
	j720tp_wsmouse_ioctl,
	j720tp_wsmouse_disable
};

static const struct wsmouse_calibcoords j720tp_default_calib = {
	0, 0, 639, 239,
	4,
	{{ 988,  80,   0,   0 },
	 {  88,  84, 639,   0 },
	 { 988, 927,   0, 239 },
	 {  88, 940, 639, 239 }}
};

const struct wskbd_accessops j720tp_wskbd_accessops = {
	j720tp_wskbd_enable,
	j720tp_wskbd_set_leds,
	j720tp_wskbd_ioctl
};

#ifndef J720TP_SETTINGS_ICON_KEYSYM
#define J720TP_SETTINGS_ICON_KEYSYM	KS_Home
#endif
#ifndef J720TP_BACKUP_ICON_KEYSYM
#define J720TP_BACKUP_ICON_KEYSYM	KS_Prior
#endif
#ifndef J720TP_DIALUP_ICON_KEYSYM
#define J720TP_DIALUP_ICON_KEYSYM	KS_Next
#endif
#ifndef J720TP_MEDIA_ICON_KEYSYM
#define J720TP_MEDIA_ICON_KEYSYM	KS_End
#endif

/* Max Y value of the n'th hard icon. */
#define J720TP_HARD_ICON_MAX_Y(n) \
	(((j720tp_hard_icon_bottom - j720tp_hard_icon_top) / 4) * (n)) + \
	j720tp_hard_icon_top

/* Default raw X/Y values of the hard icon area. */
static int j720tp_hard_icon_left = 70;
static int j720tp_hard_icon_right = 20;
static int j720tp_hard_icon_top = 70;
static int j720tp_hard_icon_bottom = 940;

/* Maps the icon number to a raw keycode. */
static const int j720tp_wskbd_keys[] = {
	/* Icon 1 */ 199,
	/* Icon 2 */ 201,
	/* Icon 3 */ 209,
	/* Icon 4 */ 207
};

static const keysym_t j720tp_wskbd_keydesc[] = {
	KS_KEYCODE(199), J720TP_SETTINGS_ICON_KEYSYM,
	KS_KEYCODE(201), J720TP_BACKUP_ICON_KEYSYM,
	KS_KEYCODE(209), J720TP_DIALUP_ICON_KEYSYM,
	KS_KEYCODE(207), J720TP_MEDIA_ICON_KEYSYM
};

const struct wscons_keydesc j720tp_wskbd_keydesctab[] = {
	{ KB_US, 0,
	  sizeof(j720tp_wskbd_keydesc) / sizeof(keysym_t),
	  j720tp_wskbd_keydesc
	},
	{ 0, 0, 0, 0 }
};

const struct wskbd_mapdata j720tp_wskbd_keymapdata = {
	j720tp_wskbd_keydesctab, KB_US
};

CFATTACH_DECL_NEW(j720tp, sizeof(struct j720tp_softc),
    j720tp_match, j720tp_attach, NULL, NULL);


static int
j720tp_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_7XX))
		return 0;
	if (strcmp(cf->cf_name, "j720tp") != 0)
		return 0;

	return 1;
}

static void
j720tp_attach(device_t parent, device_t self, void *aux)
{
	struct j720tp_softc *sc = device_private(self);
	struct wsmousedev_attach_args wsma;
	struct wskbddev_attach_args wska;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_ssp = device_private(parent);
	sc->sc_enabled = 0;
	sc->sc_hard_icon = 0;
	sc->sc_rawkbd = 0;

	/* Touch-panel as a pointing device. */
	wsma.accessops = &j720tp_wsmouse_accessops;
	wsma.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &wsma,
	    wsmousedevprint);
	if (sc->sc_wsmousedev == NULL)
		return;

	/* Initialize calibration, set default parameters. */
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
	    __UNCONST(&j720tp_default_calib), 0, 0);

	callout_init(&sc->sc_tpcallout, 0);

	j720tp_wsmouse_disable(sc);

	/* On-screen "hard icons" as a keyboard device. */
	wska.console = 0;
	wska.keymap = &j720tp_wskbd_keymapdata;
	wska.accessops = &j720tp_wskbd_accessops;
	wska.accesscookie = sc;

	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &wska,
	    wskbddevprint);

	/* Setup touch-panel interrupt. */
	sa11x0_intr_establish(0, 9, 1, IPL_TTY, j720tp_intr, sc);
}

static int
j720tp_wsmouse_enable(void *self)
{
	struct j720tp_softc *sc = self;
	int s;

	s = spltty();

	j720tp_enable_intr(sc);

	sc->sc_enabled |= J720TP_WSMOUSE_ENABLED;

	splx(s);
	return 0;
}

static int
j720tp_wsmouse_ioctl(void *self, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct j720tp_softc *sc = self;

	return hpc_tpanel_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
}

static void
j720tp_wsmouse_disable(void *self)
{
	struct j720tp_softc *sc = self;
	int s;

	s = spltty();

	j720tp_disable_intr(sc);
	callout_stop(&sc->sc_tpcallout);

	sc->sc_enabled &= ~J720TP_WSMOUSE_ENABLED;

	splx(s);
}

static int
j720tp_wskbd_enable(void *self, int on)
{
	struct j720tp_softc *sc = self;
	int s;

	s = spltty();
	sc->sc_enabled |= J720TP_WSKBD_ENABLED;
	splx(s);

	return 0;
}

static void
j720tp_wskbd_set_leds(void *self, int leds)
{
	/* nothing to do */
}

static int
j720tp_wskbd_ioctl(void *self, u_long cmd, void *data, int flag,
    struct lwp *l)
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct j720tp_softc *sc = self;
#endif

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HPC_BTN;
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = (*(int *)data == WSKBD_RAW);
		return 0;
#endif
	}

	return EPASSTHROUGH;
}

/*
 * Enable touch-panel interrupt.  Must be called at spltty().
 */
static void
j720tp_enable_intr(struct j720tp_softc *sc)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	uint32_t er;

	er = bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER);
	er |= 1 << 9;
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER, er);
}

/*
 * Disable touch-panel interrupt.  Must be called at spltty().
 */
static void
j720tp_disable_intr(struct j720tp_softc *sc)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	uint32_t er;

	er = bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER);
	er &= ~(1 << 9);
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER, er);
}

static int
j720tp_intr(void *arg)
{
	struct j720tp_softc *sc = arg;
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int rawx, rawy;

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_EDR, 1 << 9);

	if (!(sc->sc_enabled & J720TP_WSMOUSE_ENABLED)) {
		DPRINTF(("j720tp_intr: !sc_enabled\n"));
		return 0;
	}

	j720tp_disable_intr(sc);

	if (j720tp_get_rawxy(sc, &rawx, &rawy)) {
		sc->sc_hard_icon = j720tp_get_hard_icon(sc, rawx, rawy);

		if (sc->sc_hard_icon > 0) {
			j720tp_wskbd_input(sc, WSCONS_EVENT_KEY_DOWN);
			callout_reset(&sc->sc_tpcallout, hz / 32,
			    j720tp_wskbd_callout, sc);
		} else {
			j720tp_wsmouse_input(sc, rawx, rawy);
			callout_reset(&sc->sc_tpcallout, hz / 32,
			    j720tp_wsmouse_callout, sc);
		}
	}

	return 1;
}

static int
j720tp_get_rawxy(struct j720tp_softc *sc, int *rawx, int *rawy)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int buf[8], data, i;

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PCR, 0x2000000);

	/* Send read touch-panel command. */
	if (j720ssp_readwrite(ssp, 1, 0xa0, &data, 100) < 0 || data != 0x11) {
		DPRINTF(("j720tp_get_rawxy: no dummy received\n"));
		goto out;
	}

	for (i = 0; i < 8; i++) {
		if (j720ssp_readwrite(ssp, 0, 0x11, &data, 100) < 0)
			goto out;
		buf[i] = data;
	}

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	buf[6] <<= 8;
	buf[7] <<= 8;
	for (i = 0; i < 3; i++) {
		buf[i] |= buf[6] & 0x300;
		buf[6] >>= 2;
		buf[i + 3] |= buf[7] & 0x300;
		buf[7] >>= 2;
	}

	DPRINTFN(2, ("j720tp_get_rawxy: %d:%d:%d:%d:%d:%d:%d:%d\n",
	    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));

	*rawx = buf[1];
	*rawy = buf[4];

	return 1;
out:
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x387);

	DPRINTF(("j720tp_get_rawxy: error %x\n", data));
	return 0;
}

static int
j720tp_get_hard_icon(struct j720tp_softc *sc, int rawx, int rawy)
{
	int icon = 0;

	if (!(sc->sc_enabled & J720TP_WSKBD_ENABLED))
		return 0;
	/* Check if the touch was in the hard icons area. */
	if (rawx > j720tp_hard_icon_left)
		return 0;

	if (rawy < J720TP_HARD_ICON_MAX_Y(1))
		icon = 1;
	else if (rawy < J720TP_HARD_ICON_MAX_Y(2))
		icon = 2;
	else if (rawy < J720TP_HARD_ICON_MAX_Y(3))
		icon = 3;
	else if (rawy < J720TP_HARD_ICON_MAX_Y(4))
		icon = 4;

	return icon;
}

static void
j720tp_wsmouse_input(struct j720tp_softc *sc, int rawx, int rawy)
{
	int x, y;

	tpcalib_trans(&sc->sc_tpcalib, rawx, rawy, &x, &y);
	wsmouse_input(sc->sc_wsmousedev, 1, x, y, 0, 0,
	    WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
}

static void
j720tp_wsmouse_callout(void *arg)
{
	struct j720tp_softc *sc = arg;
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int rawx, rawy, s;

	s = spltty();

	if (!(sc->sc_enabled & J720TP_WSMOUSE_ENABLED)) {
		DPRINTF(("j720tp_wsmouse_callout: !sc_enabled\n"));
		splx(s);
		return;
	}

	if (bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PLR) & (1<<9)) {
		wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0, 0);
		j720tp_enable_intr(sc);
	} else {
		if (j720tp_get_rawxy(sc, &rawx, &rawy))
			j720tp_wsmouse_input(sc, rawx, rawy);
		callout_schedule(&sc->sc_tpcallout, hz / 32);
	}

	splx(s);
}

static void
j720tp_wskbd_input(struct j720tp_softc *sc, u_int evtype)
{
	int key = j720tp_wskbd_keys[sc->sc_hard_icon - 1];

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (sc->sc_rawkbd) {
		int n;
		u_char data[16];

		n = pckbd_encode(evtype, key, data);
		wskbd_rawinput(sc->sc_wskbddev, data, n);
	} else 
#endif
		wskbd_input(sc->sc_wskbddev, evtype, key);
}

static void
j720tp_wskbd_callout(void *arg)
{
	struct j720tp_softc *sc = arg;
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int s;

	s = spltty();

	if (!sc->sc_enabled) {
		DPRINTF(("j720tp_wskbd_callout: !sc_enabled\n"));
		splx(s);
		return;
	}

	if (bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PLR) & (1<<9)) {
		j720tp_wskbd_input(sc, WSCONS_EVENT_KEY_UP);
		j720tp_enable_intr(sc);
	} else {
		callout_schedule(&sc->sc_tpcallout, hz / 32);
	}

	splx(s);
}

SYSCTL_SETUP(sysctl_j720tp, "sysctl j720tp subtree setup")
{
	const struct sysctlnode *rnode;
	int rc;

	if ((rc = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "j720tp",
	    SYSCTL_DESCR("Jornada 720 touch panel controls"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

#ifdef J720TP_DEBUG
	if ((rc = sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT, "debug",
	    SYSCTL_DESCR("Verbosity of debugging messages"),
	    NULL, 0, &j720tp_debug, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif /* J720TP_DEBUG */

	if ((rc = sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hard_icons",
	    SYSCTL_DESCR("Touch panel hard icons controls"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT, "left",
	    SYSCTL_DESCR("Raw left X value of the hard icon area"),
	    NULL, 0, &j720tp_hard_icon_left, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT, "right",
	    SYSCTL_DESCR("Raw right X value of the hard icon area"),
	    NULL, 0, &j720tp_hard_icon_right, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT, "top",
	    SYSCTL_DESCR("Raw top Y value of the hard icon area"),
	    NULL, 0, &j720tp_hard_icon_top, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT, "bottom",
	    SYSCTL_DESCR("Raw bottom Y value of the hard icon area"),
	    NULL, 0, &j720tp_hard_icon_bottom, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return;
err:
	aprint_normal("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}
