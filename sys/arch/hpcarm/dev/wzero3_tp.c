/*	$NetBSD: wzero3_tp.c,v 1.1 2010/05/09 10:40:00 nonaka Exp $	*/

/*
 * Copyright (c) 2010 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_tp.c,v 1.1 2010/05/09 10:40:00 nonaka Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/hpc/hpcfbio.h>
#include <dev/hpc/hpctpanelvar.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/xscalereg.h>
#include <arm/xscale/pxa2x0_lcd.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <hpcarm/dev/wzero3_reg.h>
#include <hpcarm/dev/wzero3_sspvar.h>

#ifdef WZERO3TP_DEBUG
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

#define POLL_TIMEOUT_RATE0	((hz * 150)/1000)
#define POLL_TIMEOUT_RATE1	(hz / 100) /* XXX every tick */

/* Settable via sysctl. */
int wzero3tp_rawmode;

static const struct wsmouse_calibcoords ws007sh_default_calib = {
	0, 0, 479, 639,				/* minx, miny, maxx, maxy */
	5,					/* samplelen */
	{
		{ 2050, 2024, 240, 320 },	/* rawx, rawy, x, y */
		{  578, 3471,  48,  64 },
		{  578,  582,  48, 576 },
		{ 3432,  582, 432, 576 },
		{ 3432, 3471, 432,  64 },
	}
};

struct wzero3tp_pos {
	int x;
	int y;
	int z;			/* touch pressure */
};

struct wzero3tp_model;
struct wzero3tp_softc {
	device_t sc_dev;
	const struct wzero3tp_model *sc_model;
	void *sc_gh;
	struct callout sc_tp_poll;
	int sc_enabled;
	int sc_buttons; /* button emulation ? */
	struct device *sc_wsmousedev;
	struct wzero3tp_pos sc_oldpos;
	int sc_resx;
	int sc_resy;
	struct tpcalib_softc sc_tpcalib;
};

static int	wzero3tp_match(device_t, cfdata_t, void *);
static void	wzero3tp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3tp, sizeof(struct wzero3tp_softc),
	wzero3tp_match, wzero3tp_attach, NULL, NULL);

static int	wzero3tp_enable(void *);
static void	wzero3tp_disable(void *);
static bool	wzero3tp_suspend(device_t dv, const pmf_qual_t *);
static bool	wzero3tp_resume(device_t dv, const pmf_qual_t *);
static void	wzero3tp_poll(void *);
static int	wzero3tp_irq(void *);
static int	wzero3tp_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wsmouse_accessops wzero3tp_accessops = {
        wzero3tp_enable,
	wzero3tp_ioctl,
	wzero3tp_disable
};

struct wzero3tp_model {
	platid_mask_t *platid;
	const char *name;
	int resx, resy;
	int intr_pin;
	const struct wsmouse_calibcoords *calib;
} wzero3tp_table[] = {
#if 0
	/* WS003SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS003SH,
		"WS003SH",
		480, 640,
		-1,
		NULL,
	},
	/* WS004SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS004SH,
		"WS004SH",
		480, 640,
		-1,
		NULL,
	},
#endif
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
		"WS007SH",
		480, 640,
		GPIO_WS007SH_TOUCH_PANEL,
		&ws007sh_default_calib,
	},
#if 0
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
		"WS011SH",
		480, 800,
		GPIO_WS011SH_TOUCH_PANEL,
		NULL,
	},
	/* WS0020H */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS020SH,
		"WS020SH",
		480, 800,
		-1,
		NULL,
	},
#endif
	{
		NULL, NULL, -1, -1, -1, NULL,
	},
};


static const struct wzero3tp_model *
wzero3tp_lookup(void)
{
	const struct wzero3tp_model *model;

	for (model = wzero3tp_table; model->platid != NULL; model++) {
		if (platid_match(&platid, model->platid)) {
			return model;
		}
	}
	return NULL;
}

static int
wzero3tp_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(cf->cf_name, "wzero3tp") != 0)
		return 0;
	if (wzero3tp_lookup() == NULL)
		return 0;
	return 1;
}

static void
wzero3tp_attach(device_t parent, device_t self, void *aux)
{
	struct wzero3tp_softc *sc = device_private(self);
	struct wsmousedev_attach_args a;  

	sc->sc_dev = self;

	sc->sc_model = wzero3tp_lookup();
	if (sc->sc_model == NULL)
		return;

	aprint_normal(": touch panel\n");
	aprint_naive("\n");

	callout_init(&sc->sc_tp_poll, 0);
	callout_setfunc(&sc->sc_tp_poll, wzero3tp_poll, sc);

	a.accessops = &wzero3tp_accessops;
	a.accesscookie = sc;

	sc->sc_resx = sc->sc_model->resx;
	sc->sc_resy = sc->sc_model->resy;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	/* Initialize calibration, set default parameters. */
	tpcalib_init(&sc->sc_tpcalib);
	tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
	    __UNCONST(sc->sc_model->calib), 0, 0);
}

static int
wzero3tp_enable(void *v)
{
	struct wzero3tp_softc *sc = (struct wzero3tp_softc *)v;

	DPRINTF(("%s: wzero3tp_enable()\n", device_xname(sc->sc_dev)));

	if (sc->sc_enabled) {
		DPRINTF(("%s: already enabled\n", device_xname(sc->sc_dev)));
		return EBUSY;
	}

	callout_stop(&sc->sc_tp_poll);

	if (!pmf_device_register(sc->sc_dev, wzero3tp_suspend, wzero3tp_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	if (sc->sc_model->intr_pin >= 0) {
		pxa2x0_gpio_set_function(sc->sc_model->intr_pin, GPIO_IN);

		/* XXX */
		if (sc->sc_gh == NULL) {
			sc->sc_gh = pxa2x0_gpio_intr_establish(
			    sc->sc_model->intr_pin, IST_EDGE_FALLING, IPL_TTY,
			    wzero3tp_irq, sc);
		} else {
			pxa2x0_gpio_intr_unmask(sc->sc_gh);
		}
	}

	/* enable interrupts */
	sc->sc_enabled = 1;
	sc->sc_buttons = 0;

	return 0;
}

static void
wzero3tp_disable(void *v)
{
	struct wzero3tp_softc *sc = (struct wzero3tp_softc *)v;

	DPRINTF(("%s: wzero3tp_disable()\n", device_xname(sc->sc_dev)));

	callout_stop(&sc->sc_tp_poll);

	pmf_device_deregister(sc->sc_dev);

	if (sc->sc_gh)
		pxa2x0_gpio_intr_mask(sc->sc_gh);

	/* disable interrupts */
	sc->sc_enabled = 0;
}

static bool
wzero3tp_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct wzero3tp_softc *sc = device_private(dv);

	DPRINTF(("%s: wzero3tp_suspend()\n", device_xname(sc->sc_dev)));

	sc->sc_enabled = 0;
	if (sc->sc_gh)
		pxa2x0_gpio_intr_mask(sc->sc_gh);

	callout_stop(&sc->sc_tp_poll);

	/* Turn off reference voltage but leave ADC on. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846,
	    (1<<ADSCTRL_PD1_SH) | (1<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH));

	if (sc->sc_model->intr_pin >= 0) {
		pxa2x0_gpio_set_function(sc->sc_model->intr_pin,
		    GPIO_OUT|GPIO_SET);
	}

	return true;
}

static bool
wzero3tp_resume(device_t dv, const pmf_qual_t *qual)
{
	struct wzero3tp_softc *sc = device_private(dv);

	DPRINTF(("%s: wzero3tp_resume()\n", device_xname(sc->sc_dev)));

	if (sc->sc_model->intr_pin >= 0)
		pxa2x0_gpio_set_function(sc->sc_model->intr_pin, GPIO_IN);
	if (sc->sc_gh)
		pxa2x0_gpio_intr_mask(sc->sc_gh);

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846,
	    (4<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH));

	if (sc->sc_gh)
		pxa2x0_gpio_intr_unmask(sc->sc_gh);
	sc->sc_enabled = 1;
	
	return true;
}

#define HSYNC()								\
do {									\
	while (pxa2x0_gpio_get_bit(GPIO_WS007SH_HSYNC) == 0)		\
		continue;						\
	while (pxa2x0_gpio_get_bit(GPIO_WS007SH_HSYNC) != 0)		\
		continue;						\
} while (/*CONSTCOND*/0)

static uint32_t wzero3tp_sync_ads784x(int, int, uint32_t);
static int wzero3tp_readpos(struct wzero3tp_pos *);

/*
 * Communicate synchronously with the ADS784x touch screen controller.
 */
static uint32_t
wzero3tp_sync_ads784x(int dorecv/* XXX */, int dosend/* XXX */, uint32_t cmd)
{
	uint32_t rv = 0;

	HSYNC();

	if (dorecv)
		rv = wzero3ssp_ic_stop(WZERO3_SSP_IC_ADS7846);

	if (dosend) {
		/* send dummy command; discard SSDR */
		(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846, cmd);

		/* wait for refresh */
		HSYNC();

		/* send the actual command; keep ADS784x enabled */
		wzero3ssp_ic_start(WZERO3_SSP_IC_ADS7846, cmd);
	}

	return rv;
}

static int
wzero3tp_readpos(struct wzero3tp_pos *pos)
{
	int cmd, cmd0;
	int z0, z1;
	int down;

	cmd0 = (1<<ADSCTRL_STS_SH) | (1<<ADSCTRL_PD0_SH) | (1<<ADSCTRL_PD1_SH);

	/* check that pen is down */
	cmd = cmd0 | (3<<ADSCTRL_ADR_SH);
	z0 = wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846, cmd);
	DPRINTF(("ztp_readpos(): first z0 = %d\n", z0));

	down = (z0 >= 10);
	if (!down)
		goto out;

	/* Y (discard) */
	cmd = cmd0 | (1<<ADSCTRL_ADR_SH);
	(void)wzero3tp_sync_ads784x(0, 1, cmd);

	/* Y */
	cmd = cmd0 | (1<<ADSCTRL_ADR_SH);
	(void)wzero3tp_sync_ads784x(1, 1, cmd);

	/* X */
	cmd = cmd0 | (5<<ADSCTRL_ADR_SH);
	pos->y = wzero3tp_sync_ads784x(1, 1, cmd);
	DPRINTF(("wzero3tp_readpos(): y = %d\n", pos->y));

	/* Z0 */
	cmd = cmd0 | (3<<ADSCTRL_ADR_SH);
	pos->x = wzero3tp_sync_ads784x(1, 1, cmd);
	DPRINTF(("wzero3tp_readpos(): x = %d\n", pos->x));

	/* Z1 */
	cmd = cmd0 | (4<<ADSCTRL_ADR_SH);
	z0 = wzero3tp_sync_ads784x(1, 1, cmd);
	z1 = wzero3tp_sync_ads784x(1, 0, cmd);
	DPRINTF(("wzero3tp_readpos(): z0 = %d, z1 = %d\n", z0, z1));

	/* check that pen is still down */
	if (z0 == 0 || (pos->x * (z1 - z0) / z0) >= 15000)
		down = 0;
	pos->z = down;

out:
	/* Enable automatic low power mode. */
	cmd = (1<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH);
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846, cmd);

	return down;
}

static void
wzero3tp_poll(void *v)
{
	int s;

	s = spltty();
	(void)wzero3tp_irq(v);
	splx(s);
}

static int
wzero3tp_irq(void *v)
{
	struct wzero3tp_softc *sc = (struct wzero3tp_softc *)v;
	struct wzero3tp_pos tp = { 0, 0, 0 };
	int pindown;
	int down;
	int x, y;
	int s;

	if (!sc->sc_enabled)
		return 0;

	s = splhigh();

	if (sc->sc_model->intr_pin >= 0)
		pindown = pxa2x0_gpio_get_bit(sc->sc_model->intr_pin) ? 0 : 1;
	else
		pindown = 0;/*XXX*/
	DPRINTF(("%s: pindown = %d\n", device_xname(sc->sc_dev), pindown));
	if (pindown) {
		pxa2x0_gpio_intr_mask(sc->sc_gh);
		callout_schedule(&sc->sc_tp_poll, POLL_TIMEOUT_RATE1);
	}

	down = wzero3tp_readpos(&tp);
	DPRINTF(("%s: x = %d, y = %d, z = %d, down = %d\n",
	    device_xname(sc->sc_dev), tp.x, tp.y, tp.z, down));

	if (!pindown) {
		pxa2x0_gpio_intr_unmask(sc->sc_gh);
		callout_schedule(&sc->sc_tp_poll, POLL_TIMEOUT_RATE0);
	}
	if (sc->sc_model->intr_pin >= 0)
		pxa2x0_gpio_clear_intr(sc->sc_model->intr_pin);

	splx(s);
	
	if (down) {
		if (!wzero3tp_rawmode) {
			tpcalib_trans(&sc->sc_tpcalib, tp.x, tp.y, &x, &y);
			DPRINTF(("%s: x = %d, y = %d\n",
			    device_xname(sc->sc_dev), x, y));
			tp.x = x;
			tp.y = y;
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
wzero3tp_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wzero3tp_softc *sc = (struct wzero3tp_softc *)v;
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
		snprintf(id->data, WSMOUSE_ID_MAXLEN, "Sharp %s SN000000",
		    sc->sc_model->name);
		id->length = strlen(id->data);
		return 0;

	case WSMOUSEIO_SCALIBCOORDS:
	case WSMOUSEIO_GCALIBCOORDS:
		return tpcalib_ioctl(&sc->sc_tpcalib, cmd, data, flag, l);
	}

	return EPASSTHROUGH;
}
