/*	$NetBSD: psh3pwr.c,v 1.4 2009/12/19 07:09:28 kiyohara Exp $	*/
/*
 * Copyright (c) 2005, 2007 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: psh3pwr.c,v 1.4 2009/12/19 07:09:28 kiyohara Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/apm/apmbios.h>

#include <sh3/exception.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>

#include <sh3/dev/adcvar.h>


#ifdef PSH3PWR_DEBUG
#define DPRINTF(arg)            printf arg
#else
#define DPRINTF(arg)            ((void)0)
#endif


/* A/D covnerter channels to get power stats from */
#define ADC_CHANNEL_BATTERY	3

/* On/Off bit for Green LED. pin 7 in SH7709 GPIO port H. */
#define PSH3_GREEN_LED_ON	0x80

/*
 * XXXX:
 *   WindowsCE seem to be using this as a flag.
 *   pin 6 in SH7709 GPIO port SCPDR.
 */
#define PSH3PWR_PLUG_OUT	0x40


static inline int __attribute__((__always_inline__))
psh3pwr_ac_is_off(void)
{

	return _reg_read_1(SH7709_SCPDR) & PSH3PWR_PLUG_OUT;
}


/*
 * Empirical range of battery values.
 * Thanks to Joseph Heenan for measurements.
 */
#define PSH3PWR_BATTERY_MIN		630
#define PSH3PWR_BATTERY_CRITICAL	635
#define PSH3PWR_BATTERY_LOW		645
#define PSH3PWR_BATTERY_FULL		910 /* can be slightly more */


struct psh3pwr_softc {
	device_t sc_dev;

	void *sc_ih_pin;
	void *sc_ih_pout;
};

static int psh3pwr_match(device_t, struct cfdata *, void *);
static void psh3pwr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(psh3pwr, sizeof(struct psh3pwr_softc),
    psh3pwr_match, psh3pwr_attach, NULL, NULL);

static int psh3pwr_intr_plug_out(void *);
static int psh3pwr_intr_plug_in(void *);
static void psh3pwr_sleep(void *);
static int psh3pwr_apm_getpower_hook(void *, int, long, void *);
static int psh3pwr_get_battery(void);


static int
psh3pwr_match(device_t parent, struct cfdata *cfp, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HITACHI_PERSONA))
		return 0;

	if (strcmp(cfp->cf_name, "psh3pwr") != 0)
		return 0;

	return 1;
}


static void
psh3pwr_attach(device_t parent, device_t self, void *aux)
{
	extern void (*__sleep_func)(void *);
	extern void *__sleep_ctx;
	struct psh3pwr_softc *sc = device_private(self);
	uint8_t phdr;

	sc->sc_dev = self;

	/* arrange for hpcapm to call us when power status is requested */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_ACADAPTER,
	    CONFIG_HOOK_EXCLUSIVE, psh3pwr_apm_getpower_hook, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CHARGE,
	    CONFIG_HOOK_EXCLUSIVE, psh3pwr_apm_getpower_hook, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BATTERYVAL,
	    CONFIG_HOOK_EXCLUSIVE, psh3pwr_apm_getpower_hook, sc);

	/* regisiter sleep function to APM */
	__sleep_func = psh3pwr_sleep;
	__sleep_ctx = self;

	phdr = _reg_read_1(SH7709_PHDR);
	_reg_write_1(SH7709_PHDR, phdr | PSH3_GREEN_LED_ON);

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_ih_pout = intc_intr_establish(SH7709_INTEVT2_IRQ0,
	    IST_EDGE, IPL_TTY, psh3pwr_intr_plug_out, sc);
	sc->sc_ih_pin = intc_intr_establish(SH7709_INTEVT2_IRQ1,
	    IST_EDGE, IPL_TTY, psh3pwr_intr_plug_in, sc);

	/* XXXX: WindowsCE sets this bit. */
	aprint_normal_dev(self, "plug status: %s\n",
	    psh3pwr_ac_is_off() ? "out" : "in");

 	if (!pmf_device_register(self, NULL, NULL))
 		aprint_error_dev(self, "unable to establish power handler\n");
}


static int
psh3pwr_intr_plug_out(void *self)
{
	struct psh3pwr_softc *sc __attribute__((__unused__)) =
	    (struct psh3pwr_softc *)self;
	uint8_t irr0, scpdr;

	irr0 = _reg_read_1(SH7709_IRR0);
	if (!(irr0 & IRR0_IRQ0)) {
		return 0;
	}
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ0);

	/* XXXX: WindowsCE sets this bit. */
	scpdr = _reg_read_1(SH7709_SCPDR);
	_reg_write_1(SH7709_SCPDR, scpdr | PSH3PWR_PLUG_OUT);

	DPRINTF(("%s: plug out\n", device_xname(&sc->sc_dev)));

	return 1;
}

static int
psh3pwr_intr_plug_in(void *self)
{
	struct psh3pwr_softc *sc __attribute__((__unused__)) =
	    (struct psh3pwr_softc *)self;
	uint8_t irr0, scpdr;

	irr0 = _reg_read_1(SH7709_IRR0);
	if (!(irr0 & IRR0_IRQ1))
		return 0;
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ1);

	/* XXXX: WindowsCE sets this bit. */
	scpdr = _reg_read_1(SH7709_SCPDR);
	_reg_write_1(SH7709_SCPDR, scpdr & ~PSH3PWR_PLUG_OUT);

	DPRINTF(("%s: plug in\n", device_xname(&sc->sc_dev)));

	return 1;
}

void
psh3pwr_sleep(void *self)
{
	/* splhigh on entry */
	extern void pfckbd_poll_hitachi_power(void);

	uint8_t phdr;

	phdr = _reg_read_1(SH7709_PHDR);
	_reg_write_1(SH7709_PHDR, phdr & ~PSH3_GREEN_LED_ON);

	pfckbd_poll_hitachi_power();

	phdr = _reg_read_1(SH7709_PHDR);
	_reg_write_1(SH7709_PHDR, phdr | PSH3_GREEN_LED_ON);
}

static int
psh3pwr_apm_getpower_hook(void *ctx, int type, long id, void *msg)
{
	/* struct psh0pwr_softc * const sc = ctx; */
	int * const pval = msg;
	int battery, state;

	if (type != CONFIG_HOOK_GET)
		return EINVAL;

	switch (id) {

	case CONFIG_HOOK_ACADAPTER:
		*pval = psh3pwr_ac_is_off() ? APM_AC_OFF : APM_AC_ON;
		return 0;

	case CONFIG_HOOK_CHARGE:
		battery = psh3pwr_get_battery();
		if (battery < PSH3PWR_BATTERY_CRITICAL)
			state = APM_BATT_FLAG_CRITICAL;
		else if (battery < PSH3PWR_BATTERY_LOW)
			state = APM_BATT_FLAG_LOW;
		else
			state = APM_BATT_FLAG_HIGH; /* XXX? */
		*pval = state;
		return 0;

	case CONFIG_HOOK_BATTERYVAL:
		battery = psh3pwr_get_battery();
		if (battery > PSH3PWR_BATTERY_FULL)
			state = 100;
		else
			state = 100 * (battery - PSH3PWR_BATTERY_MIN) /
			    (PSH3PWR_BATTERY_FULL - PSH3PWR_BATTERY_MIN);
		*pval = state;
		return 0;
	}

	return EINVAL;
}


static int
psh3pwr_get_battery(void)
{
	int battery;
	int s;

	s = spltty();
	battery = adc_sample_channel(ADC_CHANNEL_BATTERY);
	splx(s);

	return battery;
}
