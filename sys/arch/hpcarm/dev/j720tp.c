/*	$NetBSD: j720tp.c,v 1.1.8.2 2006/04/22 11:37:28 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/* Jornada 720 touch-panel driver. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720tp.c,v 1.1.8.2 2006/04/22 11:37:28 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/callout.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/hpc/hpctpanelvar.h>

#include <hpcarm/dev/j720sspvar.h>

#include "opt_j720tp.h"

#ifdef J720TP_DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

struct j720tp_softc {
	struct device		sc_dev;

	int			sc_enabled;

	struct callout		sc_tpcallout;
	struct j720ssp_softc	*sc_ssp;

	struct device		*sc_wsmousedev;

	struct tpcalib_softc	sc_tpcalib;
};

static int	j720tp_match(struct device *, struct cfdata *, void *);
static void	j720tp_attach(struct device *, struct device *, void *);

static int	j720tp_wsmouse_enable(void *);
static int	j720tp_wsmouse_ioctl(void *, u_long, caddr_t, int,
			struct lwp *);
static void	j720tp_wsmouse_disable(void *);

static void	j720tp_enable_intr(struct j720tp_softc *);
static void	j720tp_disable_intr(struct j720tp_softc *);
static int	j720tp_intr(void *);
static int	j720tp_get_rawxy(struct j720tp_softc *, int *, int *);
static void	j720tp_wsmouse_input(struct j720tp_softc *, int, int);
static void	j720tp_wsmouse_callout(void *);


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

CFATTACH_DECL(j720tp, sizeof(struct j720tp_softc),
    j720tp_match, j720tp_attach, NULL, NULL);


static int
j720tp_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_7XX))
		return 0;
	if (strcmp(cf->cf_name, "j720tp") != 0)
		return 0;

	return 1;
}

static void
j720tp_attach(struct device *parent, struct device *self, void *aux)
{
        struct j720tp_softc *sc = (struct j720tp_softc *)self;
	struct wsmousedev_attach_args wsma;

	printf("\n");

	sc->sc_ssp = (struct j720ssp_softc *)parent;
	sc->sc_enabled = 0;

	/* Touch-panel as a pointing device. */
	wsma.accessops = &j720tp_wsmouse_accessops;
	wsma.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &wsma,
	    wsmousedevprint);

	/* Initialize calibration, set default parameters. */
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
	    __UNCONST(&j720tp_default_calib), 0, 0);

	j720tp_wsmouse_disable(sc);
	callout_init(&sc->sc_tpcallout);

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

	sc->sc_enabled = 1;

	splx(s);
	return 0;
}

static int
j720tp_wsmouse_ioctl(void *self, u_long cmd, caddr_t data, int flag,
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

	sc->sc_enabled = 0;

	splx(s);
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

	if (!sc->sc_enabled) {
		DPRINTF(("j720tp_intr: !sc_enabled\n"));
		return 0;
	}

	j720tp_disable_intr(sc);

	if (j720tp_get_rawxy(sc, &rawx, &rawy))
		j720tp_wsmouse_input(sc, rawx, rawy);

	callout_reset(&sc->sc_tpcallout, hz / 32, j720tp_wsmouse_callout, sc);

	return 1;
}

static int
j720tp_get_rawxy(struct j720tp_softc *sc, int *rawx, int *rawy)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int buf[8], data, i;

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PCR, 0x2000000);

	/* Send read touch-panel command. */
	if (j720ssp_readwrite(ssp, 1, 0x500, &data, 100) < 0 || data != 0x88) {
		DPRINTF(("j720tp_get_rawxy: no dummy received\n"));
		goto out;
	}

	for (i = 0; i < 8; i++) {
		if (j720ssp_readwrite(ssp, 0, 0x8800, &data, 100) < 0)
			goto out;
		J720SSP_INVERT(data);
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

#if 0
	DPRINTF(("j720tp_get_rawxy: %d:%d:%d:%d:%d:%d:%d:%d\n", buf[0], buf[1],
	    buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));
#endif

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

static void
j720tp_wsmouse_input(struct j720tp_softc *sc, int rawx, int rawy)
{
	int x, y;

	tpcalib_trans(&sc->sc_tpcalib, rawx, rawy, &x, &y);
	wsmouse_input(sc->sc_wsmousedev, 1, x, y, 0,
	    WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
}

static void
j720tp_wsmouse_callout(void *arg)
{
	struct j720tp_softc *sc = arg;
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int rawx, rawy, s;

	s = spltty();

	if (!sc->sc_enabled) {
		DPRINTF(("j720tp_wsmouse_callout: !sc_enabled\n"));
		splx(s);
		return;
	}

	if (bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PLR) & (1<<9)) {
		wsmouse_input(sc->sc_wsmousedev, 0, 0, 0, 0, 0);
		j720tp_enable_intr(sc);
	} else {
		if (j720tp_get_rawxy(sc, &rawx, &rawy))
			j720tp_wsmouse_input(sc, rawx, rawy);

		callout_schedule(&sc->sc_tpcallout, hz / 32);
	}

	splx(s);
}
