/*	$NetBSD: wzero3_tp.c,v 1.6 2010/07/19 15:20:21 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: wzero3_tp.c,v 1.6 2010/07/19 15:20:21 tsutsui Exp $");

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

#define POLL_TIMEOUT_RATE0	((hz * 150) / 1000)
#define POLL_TIMEOUT_RATE1	((hz + 99) / 100) /* XXX every tick */

/* Settable via sysctl. */
int wzero3tp_rawmode;

static const struct wsmouse_calibcoords ws003sh_default_calib = {
	0, 0, 479, 639,				/* minx, miny, maxx, maxy */
	5,					/* samplelen */
	{
		{ 2028, 2004, 240, 320 },	/* rawx, rawy, x, y */
		{ 3312,  705,  48,  64 },
		{ 3316, 3371,  48, 576 },
		{  739, 3392, 432, 576 },
		{  749,  673, 432,  64 },
	}
};

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

static const struct wsmouse_calibcoords ws011sh_default_calib = {
	0, 0, 479, 799,				/* minx, miny, maxx, maxy */
	5,					/* samplelen */
	{
		{ 2126, 2048, 240, 400 },	/* rawx, rawy, x, y */
		{  527, 3464,  48,  80 },
		{ 3628, 3376,  48, 720 },
		{ 3351,  595, 432, 720 },
		{  554,  562, 432,  80 },
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
	struct tpcalib_softc sc_tpcalib;
};

static void nulldrv_init(void);
static int nulldrv_readpos(struct wzero3tp_pos *);
static void nulldrv_suspend(void);
static void nulldrv_resume(void);

static void ws007sh_wait_for_hsync(void);

void max1233_init(void);
int max1233_readpos(struct wzero3tp_pos *);
void max1233_suspend(void);
void max1233_resume(void);

void ads7846_init(void);
int ads7846_readpos(struct wzero3tp_pos *);
void ads7846_suspend(void);
void ads7846_resume(void);
extern void (*ads7846_wait_for_hsync)(void);

void ak4184_init(void);
int ak4184_readpos(struct wzero3tp_pos *);
void ak4184_suspend(void);
void ak4184_resume(void);

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
	int intr_pin;
	const struct wsmouse_calibcoords *calib;
	void (*wait_for_hsync)(void);
	struct chip {
		void (*init)(void);
		int (*readpos)(struct wzero3tp_pos *);
		void (*suspend)(void);
		void (*resume)(void);
	} chip;
} wzero3tp_table[] = {
	/* WS003SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS003SH,
		"WS003SH",
		GPIO_WS003SH_TOUCH_PANEL,
		&ws003sh_default_calib,
		NULL,
		{ max1233_init, max1233_readpos,
		  max1233_suspend, max1233_resume, },
	},
	/* WS004SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS004SH,
		"WS004SH",
		GPIO_WS003SH_TOUCH_PANEL,
		&ws003sh_default_calib,
		NULL,
		{ max1233_init, max1233_readpos,
		  max1233_suspend, max1233_resume, },
	},
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
		"WS007SH",
		GPIO_WS007SH_TOUCH_PANEL,
		&ws007sh_default_calib,
		ws007sh_wait_for_hsync,
		{ ads7846_init, ads7846_readpos,
		  ads7846_suspend, ads7846_resume, },
	},
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
		"WS011SH",
		GPIO_WS011SH_TOUCH_PANEL,
		&ws011sh_default_calib,
		NULL,
		{ ak4184_init, ak4184_readpos,
		  ak4184_suspend, ak4184_resume, },
	},
#if 0
	/* WS0020H */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS020SH,
		"WS020SH",
		-1,
		NULL,
		NULL,
		{ nulldrv_init, nulldrv_readpos,
		  nulldrv_suspend, nulldrv_resume, },
	},
#endif
	{
		NULL, NULL, -1, NULL, NULL,
		{ nulldrv_init, nulldrv_readpos,
		  nulldrv_suspend, nulldrv_resume, },
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

	aprint_normal(": touch panel\n");
	aprint_naive("\n");

	sc->sc_model = wzero3tp_lookup();
	if (sc->sc_model == NULL) {
		aprint_error_dev(self, "unknown model\n");
		return;
	}

	callout_init(&sc->sc_tp_poll, 0);
	callout_setfunc(&sc->sc_tp_poll, wzero3tp_poll, sc);

	if (sc->sc_model->wait_for_hsync)
		ads7846_wait_for_hsync = sc->sc_model->wait_for_hsync;

	(*sc->sc_model->chip.init)();

	a.accessops = &wzero3tp_accessops;
	a.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	/* Initialize calibration, set default parameters. */
	tpcalib_init(&sc->sc_tpcalib);
	if (sc->sc_model->calib) {
		tpcalib_ioctl(&sc->sc_tpcalib, WSMOUSEIO_SCALIBCOORDS,
		    __UNCONST(sc->sc_model->calib), 0, 0);
	}
}

static int
wzero3tp_enable(void *v)
{
	struct wzero3tp_softc *sc = (struct wzero3tp_softc *)v;

	DPRINTF(("%s: %s\n", device_xname(sc->sc_dev), __func__));

	if (sc->sc_enabled) {
		DPRINTF(("%s: already enabled\n", device_xname(sc->sc_dev)));
		return EBUSY;
	}

	callout_stop(&sc->sc_tp_poll);

	if (!pmf_device_register(sc->sc_dev, wzero3tp_suspend, wzero3tp_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	pxa2x0_gpio_set_function(sc->sc_model->intr_pin, GPIO_IN);

	/* XXX */
	if (sc->sc_gh == NULL) {
		sc->sc_gh = pxa2x0_gpio_intr_establish(
		    sc->sc_model->intr_pin, IST_EDGE_FALLING, IPL_TTY,
		    wzero3tp_irq, sc);
	} else {
		pxa2x0_gpio_intr_unmask(sc->sc_gh);
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

	DPRINTF(("%s: %s\n", device_xname(sc->sc_dev), __func__));

	callout_stop(&sc->sc_tp_poll);

	pmf_device_deregister(sc->sc_dev);

	pxa2x0_gpio_intr_mask(sc->sc_gh);

	/* disable interrupts */
	sc->sc_enabled = 0;
}

static bool
wzero3tp_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct wzero3tp_softc *sc = device_private(dv);

	DPRINTF(("%s: %s\n", device_xname(sc->sc_dev), __func__));

	sc->sc_enabled = 0;
	pxa2x0_gpio_intr_mask(sc->sc_gh);

	callout_stop(&sc->sc_tp_poll);

	(*sc->sc_model->chip.suspend)();

	pxa2x0_gpio_set_function(sc->sc_model->intr_pin, GPIO_OUT|GPIO_SET);

	return true;
}

static bool
wzero3tp_resume(device_t dv, const pmf_qual_t *qual)
{
	struct wzero3tp_softc *sc = device_private(dv);

	DPRINTF(("%s: %s\n", device_xname(sc->sc_dev), __func__));

	pxa2x0_gpio_set_function(sc->sc_model->intr_pin, GPIO_IN);
	pxa2x0_gpio_intr_mask(sc->sc_gh);

	(*sc->sc_model->chip.resume)();

	pxa2x0_gpio_intr_unmask(sc->sc_gh);
	sc->sc_enabled = 1;
	
	return true;
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

	pindown = pxa2x0_gpio_get_bit(sc->sc_model->intr_pin) ? 0 : 1;
	DPRINTF(("%s: pindown = %d\n", device_xname(sc->sc_dev), pindown));
	if (pindown) {
		pxa2x0_gpio_intr_mask(sc->sc_gh);
		callout_schedule(&sc->sc_tp_poll, POLL_TIMEOUT_RATE1);
	}

	down = (*sc->sc_model->chip.readpos)(&tp);
	DPRINTF(("%s: x = %d, y = %d, z = %d, down = %d\n",
	    device_xname(sc->sc_dev), tp.x, tp.y, tp.z, down));

	if (!pindown) {
		pxa2x0_gpio_intr_unmask(sc->sc_gh);
		callout_schedule(&sc->sc_tp_poll, POLL_TIMEOUT_RATE0);
	}
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

/*----------------------------------------------------------------------------
 * null driver
 */
static void
nulldrv_init(void)
{

	/* Nothing to do */
}

static int
nulldrv_readpos(struct wzero3tp_pos *pos)
{

	pos->x = 0;
	pos->y = 0;
	pos->z = 0;

	return 0;
}

static void
nulldrv_suspend(void)
{

	/* Nothing to do */
}

static void
nulldrv_resume(void)
{

	/* Nothing to do */
}

/*----------------------------------------------------------------------------
 * model specific functions
 */
static void
ws007sh_wait_for_hsync(void)
{

	while (pxa2x0_gpio_get_bit(GPIO_WS007SH_HSYNC) == 0)
		continue;
	while (pxa2x0_gpio_get_bit(GPIO_WS007SH_HSYNC) != 0)
		continue;
}

/*----------------------------------------------------------------------------
 * MAX1233 touch screen controller for WS003SH/WS004SH
 */
#define	MAXCTRL_ADDR_SH		0	/* Address bit[5:0] */
#define	MAXCTRL_PAGE_SH		6	/* Page bit (0:Data/1:Control) */
#define	MAXCTRL_RW_SH		15	/* R/W bit (0:Write/1:Read) */

/* VREF=2.5V, sets interrupt initiated touch-screen scans
 * 3.5us/sample, 16 data ave., 12 bit, settling time: 100us */
#define	MAX1233_ADCCTRL		0x8be3

void
max1233_init(void)
{

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),
	    0x0001);
	/* Wait for touch */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),
	    MAX1233_ADCCTRL);
	/* DAC on */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (2<<MAXCTRL_ADDR_SH),
	    0x0000);
}

void
max1233_suspend(void)
{

	/* power down. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),
	    0xc000);
	/* DAC off */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (2<<MAXCTRL_ADDR_SH),
	    0x8000);
}

void
max1233_resume(void)
{

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),
	    0x0001);
	/* Wait for touch */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),
	    MAX1233_ADCCTRL);
	/* DAC on */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (2<<MAXCTRL_ADDR_SH),
	    0x0000);
}

int
max1233_readpos(struct wzero3tp_pos *pos)
{
	uint32_t x, y, z1 = 0, z2 = 0, rt;
	uint32_t status;
	int down;

	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (0<<MAXCTRL_RW_SH) | (1<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),
	    0x0bf3);
	DELAY(300);

	while ((status = (wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
		    (1<<MAXCTRL_RW_SH)
		    | (1<<MAXCTRL_PAGE_SH)
		    | (0<<MAXCTRL_ADDR_SH), 0) & 0x4000)) != 0x4000) {
		DPRINTF(("%s: status=%#x\n", __func__, status));
		DELAY(10);
	}
	DPRINTF(("%s: status=%#x\n", __func__, status));

	x = wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (1<<MAXCTRL_RW_SH) | (0<<MAXCTRL_PAGE_SH) | (0<<MAXCTRL_ADDR_SH),0);
	DPRINTF(("%s: x=%d\n", __func__, x));
	y = wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (1<<MAXCTRL_RW_SH) | (0<<MAXCTRL_PAGE_SH) | (1<<MAXCTRL_ADDR_SH),0);
	DPRINTF(("%s: y=%d\n", __func__, y));
	z1 = wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (1<<MAXCTRL_RW_SH) | (0<<MAXCTRL_PAGE_SH) | (2<<MAXCTRL_ADDR_SH),0);
	DPRINTF(("%s: z1=%d\n", __func__, z1));
	z2 = wzero3ssp_ic_send(WZERO3_SSP_IC_MAX1233,
	    (1<<MAXCTRL_RW_SH) | (0<<MAXCTRL_PAGE_SH) | (3<<MAXCTRL_ADDR_SH),0);
	DPRINTF(("%s: z2=%d\n", __func__, z2));

	if (z1 >= 10) {
		rt = 400 /* XXX: X plate ohms */;
		rt *= x;
		rt *= (z2 / z1) - 1;
		rt >>= 12;
	} else
		rt = 0;
	DPRINTF(("%s: rt=%d\n", __func__, rt));

	down = (rt != 0);
	if (down) {
		pos->x = x;
		pos->y = y;
	}
	pos->z = down;

	return down;
}

/*----------------------------------------------------------------------------
 * ADS7846/TSC2046 touch screen controller for WS007SH
 */
#define ADSCTRL_PD0_SH		0	/* PD0 bit */
#define ADSCTRL_PD1_SH		1	/* PD1 bit */
#define ADSCTRL_DFR_SH		2	/* SER/DFR bit */
#define ADSCTRL_MOD_SH		3	/* Mode bit */
#define ADSCTRL_ADR_SH		4	/* Address setting */
#define ADSCTRL_STS_SH		7	/* Start bit */

static uint32_t ads7846_sync(int, int, uint32_t);

void
ads7846_init(void)
{

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846,
	    (4<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH), 0);
}

void
ads7846_suspend(void)
{

	/* Turn off reference voltage but leave ADC on. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846,
	    (1<<ADSCTRL_PD1_SH) | (1<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH), 0);
}

void
ads7846_resume(void)
{

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846,
	    (4<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH), 0);
}

int
ads7846_readpos(struct wzero3tp_pos *pos)
{
	int cmd, cmd0;
	int z0, z1;
	int down;

	cmd0 = (1<<ADSCTRL_STS_SH) | (1<<ADSCTRL_PD0_SH) | (1<<ADSCTRL_PD1_SH);

	/* check that pen is down */
	cmd = cmd0 | (3<<ADSCTRL_ADR_SH);
	z0 = wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846, cmd, 0);
	DPRINTF(("%s: first z0 = %d\n", __func__, z0));

	down = (z0 >= 10);
	if (!down)
		goto out;

	/* Y (discard) */
	cmd = cmd0 | (1<<ADSCTRL_ADR_SH);
	(void)ads7846_sync(0, 1, cmd);

	/* Y */
	cmd = cmd0 | (1<<ADSCTRL_ADR_SH);
	(void)ads7846_sync(1, 1, cmd);

	/* X */
	cmd = cmd0 | (5<<ADSCTRL_ADR_SH);
	pos->y = ads7846_sync(1, 1, cmd);
	DPRINTF(("%s: y = %d\n", __func__, pos->y));

	/* Z0 */
	cmd = cmd0 | (3<<ADSCTRL_ADR_SH);
	pos->x = ads7846_sync(1, 1, cmd);
	DPRINTF(("%s: x = %d\n", __func__, pos->x));

	/* Z1 */
	cmd = cmd0 | (4<<ADSCTRL_ADR_SH);
	z0 = ads7846_sync(1, 1, cmd);
	z1 = ads7846_sync(1, 0, cmd);
	DPRINTF(("%s: z0 = %d, z1 = %d\n", __func__, z0, z1));

	/* check that pen is still down */
	if (z0 == 0 || (pos->x * (z1 - z0) / z0) >= 15000)
		down = 0;
	pos->z = down;

out:
	/* Enable automatic low power mode. */
	cmd = (1<<ADSCTRL_ADR_SH) | (1<<ADSCTRL_STS_SH);
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846, cmd, 0);

	return down;
}

void (*ads7846_wait_for_hsync)(void);

/*
 * Communicate synchronously with the ADS784x touch screen controller.
 */
static uint32_t
ads7846_sync(int dorecv, int dosend, uint32_t cmd)
{
	uint32_t rv = 0;

	if (ads7846_wait_for_hsync)
		(*ads7846_wait_for_hsync)();

	if (dorecv)
		rv = wzero3ssp_ic_stop(WZERO3_SSP_IC_ADS7846);

	if (dosend) {
		/* send dummy command; discard SSDR */
		(void)wzero3ssp_ic_send(WZERO3_SSP_IC_ADS7846, cmd, 0);

		/* wait for refresh */
		if (ads7846_wait_for_hsync)
			(*ads7846_wait_for_hsync)();

		/* send the actual command; keep ADS784x enabled */
		wzero3ssp_ic_start(WZERO3_SSP_IC_ADS7846, cmd);
	}

	return rv;
}

/*----------------------------------------------------------------------------
 * AK4184 touch screen controller for WS011SH
 */
#define AKMCTRL_PD_SH	12	/* Power down bit */
#define AKMCTRL_ADR_SH	13	/* Address setting bits */
#define AKMCTRL_STS_SH	15	/* Start bit */

static uint32_t ak4184_sync(int, int, uint32_t);

void
ak4184_init(void)
{

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_TP,
	    (1<<AKMCTRL_STS_SH) | (0<<AKMCTRL_ADR_SH) | (0<<AKMCTRL_PD_SH), 0);
}

int
ak4184_readpos(struct wzero3tp_pos *pos)
{
	u_int rt;
	int z1, z2;

	/* X (discard) */
	(void)ak4184_sync(0, 1,
	    (1<<AKMCTRL_STS_SH) | (0<<AKMCTRL_ADR_SH) | (1<<AKMCTRL_PD_SH));

	/* X */
	(void)ak4184_sync(1, 1,
	    (1<<AKMCTRL_STS_SH) | (0<<AKMCTRL_ADR_SH) | (1<<AKMCTRL_PD_SH));

	/* Y */
	pos->x = ak4184_sync(1, 1,
	    (1<<AKMCTRL_STS_SH) | (1<<AKMCTRL_ADR_SH) | (1<<AKMCTRL_PD_SH));
	DPRINTF(("%s: x=%d\n", __func__, pos->x));
	pos->y = ak4184_sync(1, 1,
	    (1<<AKMCTRL_STS_SH) | (2<<AKMCTRL_ADR_SH) | (1<<AKMCTRL_PD_SH));
	DPRINTF(("%s: y=%d\n", __func__, pos->y));
	z1 = ak4184_sync(1, 1,
	    (1<<AKMCTRL_STS_SH) | (3<<AKMCTRL_ADR_SH) | (1<<AKMCTRL_PD_SH));
	DPRINTF(("%s: z1=%d\n", __func__, z1));
	z2 = ak4184_sync(1, 0,
	    (1<<AKMCTRL_STS_SH) | (3<<AKMCTRL_ADR_SH) | (1<<AKMCTRL_PD_SH));
	DPRINTF(("%s: z2=%d\n", __func__, z2));

	if (z1 >= 10) {
		rt = 400 /* XXX: X plate ohms */;
		rt *= pos->x;
		rt *= (z2 / z1) - 1;
		rt >>= 12;
	} else
		rt = 0;
	DPRINTF(("%s: rt=%d\n", __func__, rt));

	/* check that pen is still down */
	if (z1 == 0 || rt == 0)
		pos->z = 0;
	else
		pos->z = 1;

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_TP,
	    (1<<AKMCTRL_STS_SH) | (0<<AKMCTRL_ADR_SH) | (0<<AKMCTRL_PD_SH), 0);

	return pos->z;
}

void
ak4184_suspend(void)
{

	/* Nothing to do */
}

void
ak4184_resume(void)
{

	/* Enable automatic low power mode. */
	(void)wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_TP,
	    (1<<AKMCTRL_STS_SH) | (0<<AKMCTRL_ADR_SH) | (0<<AKMCTRL_PD_SH), 0);
}

static uint32_t
ak4184_sync(int dorecv, int dosend, uint32_t cmd)
{
	uint32_t rv = 0;

	if (dorecv)
		rv = wzero3ssp_ic_stop(WZERO3_SSP_IC_AK4184_TP);

	if (dosend) {
		/* send dummy command; discard SSDR */
		(void)wzero3ssp_ic_send(WZERO3_SSP_IC_AK4184_TP, cmd, 0);

		/* send the actual command; keep AK4184 enabled */
		wzero3ssp_ic_start(WZERO3_SSP_IC_AK4184_TP, cmd);
	}

	return rv;
}
