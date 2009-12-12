/*	$NetBSD: ztp.c,v 1.8 2009/12/12 07:49:31 nonaka Exp $	*/
/* $OpenBSD: zts.c,v 1.9 2005/04/24 18:55:49 uwe Exp $ */

/*
 * Copyright (c) 2005 Dale Rahn <drahn@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ztp.c,v 1.8 2009/12/12 07:49:31 nonaka Exp $");

#include "lcd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/hpc/hpcfbio.h>		/* XXX: for tpctl */
#include <dev/hpc/hpctpanelvar.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/xscalereg.h>
#include <arm/xscale/pxa2x0_lcd.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <zaurus/dev/zsspvar.h>

#ifdef ZTP_DEBUG
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)
#endif

/*
 * ADS784x touch screen controller
 */
#define ADSCTRL_PD0_SH          0       /* PD0 bit */
#define ADSCTRL_PD1_SH          1       /* PD1 bit */
#define ADSCTRL_DFR_SH          2       /* SER/DFR bit */
#define ADSCTRL_MOD_SH          3       /* Mode bit */
#define ADSCTRL_ADR_SH          4       /* Address setting */
#define ADSCTRL_STS_SH          7       /* Start bit */

#define GPIO_TP_INT_C3K		11
#define GPIO_HSYNC_C3K		22

#define POLL_TIMEOUT_RATE0	((hz * 150)/1000)
#define POLL_TIMEOUT_RATE1	(hz / 100) /* XXX every tick */

#define CCNT_HS_400_VGA_C3K 6250	/* 15.024us */

/* XXX need to ask zaurus_lcd.c for the screen dimension */
#define CURRENT_DISPLAY (&sharp_zaurus_C3000)
extern const struct lcd_panel_geometry sharp_zaurus_C3000;

/* Settable via sysctl. */
int ztp_rawmode;

static const struct wsmouse_calibcoords ztp_default_calib = {
	0, 0, 479, 639,				/* minx, miny, maxx, maxy */
	5,					/* samplelen */
	{
		{ 1929, 2021, 240, 320 },	/* rawx, rawy, x, y */
		{  545, 3464,  48,  64 },
		{ 3308, 3452,  48, 576 },
		{ 2854,  768, 432, 576 },
		{  542,  593, 432,  64 }
	}
};

struct ztp_pos {
	int x;
	int y;
	int z;			/* touch pressure */
};

struct ztp_softc {
	device_t sc_dev;
	struct callout sc_tp_poll;
	void *sc_gh;
	int sc_enabled;
	int sc_buttons; /* button emulation ? */
	struct device *sc_wsmousedev;
	struct ztp_pos sc_oldpos;
	int sc_resx;
	int sc_resy;
	struct tpcalib_softc sc_tpcalib;
};

static int	ztp_match(device_t, cfdata_t, void *);
static void	ztp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ztp, sizeof(struct ztp_softc),
	ztp_match, ztp_attach, NULL, NULL);

static int	ztp_enable(void *);
static void	ztp_disable(void *);
static bool	ztp_suspend(device_t dv PMF_FN_ARGS);
static bool	ztp_resume(device_t dv PMF_FN_ARGS);
static void	ztp_poll(void *);
static int	ztp_irq(void *);
static int	ztp_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wsmouse_accessops ztp_accessops = {
        ztp_enable,
	ztp_ioctl,
	ztp_disable
};

static int
ztp_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
ztp_attach(device_t parent, device_t self, void *aux)
{
	struct ztp_softc *sc = device_private(self);
	struct wsmousedev_attach_args a;  

	sc->sc_dev = self;

	aprint_normal("\n");
	aprint_naive("\n");

	callout_init(&sc->sc_tp_poll, 0);
	callout_setfunc(&sc->sc_tp_poll, ztp_poll, sc);

	/* Initialize ADS7846 Difference Reference mode */
	(void)zssp_ic_send(ZSSP_IC_ADS7846,
	    (1<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH));
	delay(5000);
	(void)zssp_ic_send(ZSSP_IC_ADS7846,
	    (3<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH));
	delay(5000);
	(void)zssp_ic_send(ZSSP_IC_ADS7846,
	    (4<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH));
	delay(5000);
	(void)zssp_ic_send(ZSSP_IC_ADS7846,
	    (5<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH));
	delay(5000);

	a.accessops = &ztp_accessops;
	a.accesscookie = sc;

#if NLCD > 0
	sc->sc_resx = CURRENT_DISPLAY->panel_height;
	sc->sc_resy = CURRENT_DISPLAY->panel_width;
#else
	sc->sc_resx = 480;	/* XXX */
	sc->sc_resy = 640;	/* XXX */
#endif

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	/* Initialize calibration, set default parameters. */
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
	    __UNCONST(&ztp_default_calib), 0, 0);
}

static int
ztp_enable(void *v)
{
	struct ztp_softc *sc = (struct ztp_softc *)v;

	DPRINTF(("%s: ztp_enable()\n", device_xname(sc->sc_dev)));

	if (sc->sc_enabled) {
		DPRINTF(("%s: already enabled\n", device_xname(sc->sc_dev)));
		return EBUSY;
	}

	callout_stop(&sc->sc_tp_poll);

	if (!pmf_device_register(sc->sc_dev, ztp_suspend, ztp_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	pxa2x0_gpio_set_function(GPIO_TP_INT_C3K, GPIO_IN);

	/* XXX */
	if (sc->sc_gh == NULL) {
		sc->sc_gh = pxa2x0_gpio_intr_establish(GPIO_TP_INT_C3K,
		    IST_EDGE_FALLING, IPL_TTY, ztp_irq, sc);
	} else {
		pxa2x0_gpio_intr_unmask(sc->sc_gh);
	}

	/* enable interrupts */
	sc->sc_enabled = 1;
	sc->sc_buttons = 0;

	return 0;
}

static void
ztp_disable(void *v)
{
	struct ztp_softc *sc = (struct ztp_softc *)v;

	DPRINTF(("%s: ztp_disable()\n", device_xname(sc->sc_dev)));

	callout_stop(&sc->sc_tp_poll);

	pmf_device_deregister(sc->sc_dev);

	if (sc->sc_gh) {
		pxa2x0_gpio_intr_mask(sc->sc_gh);
	}

	/* disable interrupts */
	sc->sc_enabled = 0;
}

static bool
ztp_suspend(device_t dv PMF_FN_ARGS)
{
	struct ztp_softc *sc = device_private(dv);

	DPRINTF(("%s: ztp_suspend()\n", device_xname(sc->sc_dev)));

	sc->sc_enabled = 0;
	pxa2x0_gpio_intr_mask(sc->sc_gh);

	callout_stop(&sc->sc_tp_poll);

	/* Turn off reference voltage but leave ADC on. */
	(void)zssp_ic_send(ZSSP_IC_ADS7846, (1 << ADSCTRL_PD1_SH) |
	    (1 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH));

	pxa2x0_gpio_set_function(GPIO_TP_INT_C3K, GPIO_OUT | GPIO_SET);

	return true;
}

static bool
ztp_resume(device_t dv PMF_FN_ARGS)
{
	struct ztp_softc *sc = device_private(dv);

	DPRINTF(("%s: ztp_resume()\n", device_xname(sc->sc_dev)));

	pxa2x0_gpio_set_function(GPIO_TP_INT_C3K, GPIO_IN);
	pxa2x0_gpio_intr_mask(sc->sc_gh);

	/* Enable automatic low power mode. */
	(void)zssp_ic_send(ZSSP_IC_ADS7846,
	    (4 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH));

	pxa2x0_gpio_intr_unmask(sc->sc_gh);
	sc->sc_enabled = 1;
	
	return true;
}

#define HSYNC()								\
do {									\
	while (pxa2x0_gpio_get_bit(GPIO_HSYNC_C3K) == 0)		\
		continue;						\
	while (pxa2x0_gpio_get_bit(GPIO_HSYNC_C3K) != 0)		\
		continue;						\
} while (/*CONSTCOND*/0)

static inline uint32_t pxa2x0_ccnt_enable(uint32_t);
static inline uint32_t pxa2x0_read_ccnt(void);
static uint32_t ztp_sync_ads784x(int, int, uint32_t);
static void ztp_sync_send(uint32_t);
static int ztp_readpos(struct ztp_pos *);

static inline uint32_t
pxa2x0_ccnt_enable(uint32_t reg)
{
	uint32_t rv;

	__asm volatile("mrc p14, 0, %0, c0, c1, 0" : "=r" (rv));
	__asm volatile("mcr p14, 0, %0, c0, c1, 0" : : "r" (reg));

	return rv;
}

static inline uint32_t
pxa2x0_read_ccnt(void)
{
	uint32_t rv;

	__asm volatile("mrc p14, 0, %0, c1, c1, 0" : "=r" (rv));

	return rv;
}

/*
 * Communicate synchronously with the ADS784x touch screen controller.
 */
static uint32_t
ztp_sync_ads784x(int dorecv/* XXX */, int dosend/* XXX */, uint32_t cmd)
{
	uint32_t ccen;
	uint32_t rv = 0;

	/* XXX poll hsync only if LCD is enabled */

	/* start clock counter */
	ccen = pxa2x0_ccnt_enable(PMNC_E);

	HSYNC();

	if (dorecv) {
		/* read SSDR and disable ADS784x */
		rv = zssp_ic_stop(ZSSP_IC_ADS7846);
	}

	if (dosend) {
		ztp_sync_send(cmd);
	}

	/* stop clock counter */
	pxa2x0_ccnt_enable(ccen);

	return rv;
}

void
ztp_sync_send(uint32_t cmd)
{
	volatile uint32_t base, now;
	uint32_t tck;

	/* XXX */
	tck = CCNT_HS_400_VGA_C3K - 151;

	/* send dummy command; discard SSDR */
	(void)zssp_ic_send(ZSSP_IC_ADS7846, cmd);

	/* wait for refresh */
	HSYNC();

	/* wait after refresh */
	base = pxa2x0_read_ccnt();
	now = pxa2x0_read_ccnt();
	while ((now - base) < tck)
		now = pxa2x0_read_ccnt();

	/* send the actual command; keep ADS784x enabled */
	zssp_ic_start(ZSSP_IC_ADS7846, cmd);
}

static int
ztp_readpos(struct ztp_pos *pos)
{
	int cmd;
	int t0, t1;
	int down;

	/* XXX */
	pxa2x0_gpio_set_function(GPIO_HSYNC_C3K, GPIO_IN);

	/* check that pen is down */
	cmd = (1 << ADSCTRL_PD0_SH) | (1 << ADSCTRL_PD1_SH) |
	    (3 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	t0 = zssp_ic_send(ZSSP_IC_ADS7846, cmd);
	DPRINTF(("ztp_readpos(): t0 = %d\n", t0));

	down = (t0 >= 10);
	if (down == 0)
		goto out;

	/* Y */
	cmd = (1 << ADSCTRL_PD0_SH) | (1 << ADSCTRL_PD1_SH) |
	    (1 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	(void)ztp_sync_ads784x(0, 1, cmd);

	/* Y */
	cmd = (1 << ADSCTRL_PD0_SH) | (1 << ADSCTRL_PD1_SH) |
	    (1 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	(void)ztp_sync_ads784x(1, 1, cmd);

	/* X */
	cmd = (1 << ADSCTRL_PD0_SH) | (1 << ADSCTRL_PD1_SH) |
	    (5 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	pos->y = ztp_sync_ads784x(1, 1, cmd);
	DPRINTF(("ztp_readpos(): y = %d\n", pos->y));

	/* T0 */
	cmd = (1 << ADSCTRL_PD0_SH) | (1 << ADSCTRL_PD1_SH) |
	    (3 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	pos->x = ztp_sync_ads784x(1, 1, cmd);
	DPRINTF(("ztp_readpos(): x = %d\n", pos->x));

	/* T1 */
	cmd = (1 << ADSCTRL_PD0_SH) | (1 << ADSCTRL_PD1_SH) |
	    (4 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	t0 = ztp_sync_ads784x(1, 1, cmd);
	t1 = ztp_sync_ads784x(1, 0, cmd);
	DPRINTF(("ztp_readpos(): t0 = %d, t1 = %d\n", t0, t1));

	/* check that pen is still down */
	/* XXX pressure sensitivity varies with X or what? */
	if (t0 == 0 || (pos->x * (t1 - t0) / t0) >= 15000)
		down = 0;
	pos->z = down;

out:
	/* Enable automatic low power mode. */
        cmd = (4 << ADSCTRL_ADR_SH) | (1 << ADSCTRL_STS_SH);
	(void)zssp_ic_send(ZSSP_IC_ADS7846, cmd);

	return down;
}

static void
ztp_poll(void *v)
{
	int s;

	s = spltty();
	(void)ztp_irq(v);
	splx(s);
}

static int
ztp_irq(void *v)
{
	extern int zkbd_modstate;
	struct ztp_softc *sc = (struct ztp_softc *)v;
	struct ztp_pos tp = { 0, 0, 0 };
	int pindown;
	int down;
	int x, y;
	int s;

	if (!sc->sc_enabled)
		return 0;

	s = splhigh();

	pindown = pxa2x0_gpio_get_bit(GPIO_TP_INT_C3K) ? 0 : 1;
	DPRINTF(("%s: pindown = %d\n", device_xname(sc->sc_dev), pindown));
	if (pindown) {
		pxa2x0_gpio_intr_mask(sc->sc_gh);
		callout_schedule(&sc->sc_tp_poll, POLL_TIMEOUT_RATE1);
	}

	down = ztp_readpos(&tp);
	DPRINTF(("%s: x = %d, y = %d, z = %d, down = %d\n",
	    device_xname(sc->sc_dev), tp.x, tp.y, tp.z, down));

	if (!pindown) {
		pxa2x0_gpio_intr_unmask(sc->sc_gh);
		callout_schedule(&sc->sc_tp_poll, POLL_TIMEOUT_RATE0);
	}
	pxa2x0_gpio_clear_intr(GPIO_TP_INT_C3K);

	splx(s);
	
	if (down) {
		if (!ztp_rawmode) {
			tpcalib_trans(&sc->sc_tpcalib, tp.x, tp.y, &x, &y);
			DPRINTF(("%s: x = %d, y = %d\n",
			    device_xname(sc->sc_dev), x, y));
			tp.x = x;
			tp.y = y;
		}
	}

	if (zkbd_modstate != 0 && down) {
		if (zkbd_modstate & (1 << 1)) {
			/* Fn */
			down = 2;
		} else if (zkbd_modstate & (1 << 2)) {
			/* 'Alt' */
			down = 4;
		}
	}
	if (!down) {
		/* x/y values are not reliable when pen is up */
		tp = sc->sc_oldpos;
	}

	if (down || sc->sc_buttons != down) {
		wsmouse_input(sc->sc_wsmousedev, down, tp.x, tp.y, 0, 0,
		    WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
		sc->sc_buttons = down;
		sc->sc_oldpos = tp;
	}

	return 1;
}

static int
ztp_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ztp_softc *sc = (struct ztp_softc *)v;
	struct wsmouse_id *id;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		return 0;

	case WSMOUSEIO_GETID:
		/*
		 * return unique ID string,
		 * "<vendor> <model> <serial number>"
		 */
		id = (struct wsmouse_id *)data;
		if (id->type != WSMOUSE_ID_TYPE_UIDSTR)
			return EINVAL;
		strlcpy(id->data, "Sharp SL-C3x00 SN000000", WSMOUSE_ID_MAXLEN);
		id->length = strlen(id->data);
		return 0;

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
		return tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
	}

	return EPASSTHROUGH;
}
