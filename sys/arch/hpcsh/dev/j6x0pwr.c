/*	$NetBSD: j6x0pwr.c,v 1.14.18.1 2009/05/13 17:17:47 jym Exp $ */

/*
 * Copyright (c) 2003, 2006 Valeriy E. Ushakov
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
__KERNEL_RCSID(0, "$NetBSD: j6x0pwr.c,v 1.14.18.1 2009/05/13 17:17:47 jym Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <dev/apm/apmbios.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <sh3/exception.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>

#include <sh3/dev/adcvar.h>


/* SH7709 PFC bits pertinent to Jornada 6x0 power */
#define PGDR_MAIN_BATTERY_OUT	0x04
#define PGDR_LID_OPEN		0x01
#define PJDR_AC_POWER_OUT	0x10
#define PLDR_BATTERY_CHARGED	0x20


static inline int __attribute__((__always_inline__))
j6x0pwr_battery_is_absent(void)
{

	return  _reg_read_1(SH7709_PGDR) & PGDR_MAIN_BATTERY_OUT;
}

static inline int __attribute__((__always_inline__))
j6x0pwr_ac_is_off(void)
{

	return _reg_read_1(SH7709_PJDR) & PJDR_AC_POWER_OUT;
}


static inline int __attribute__((__always_inline__))
j6x0pwr_is_not_charging(void)
{

	return _reg_read_1(SH7709_PLDR) & PLDR_BATTERY_CHARGED;
}


/* A/D converter channels to get power stats from */
#define ADC_CHANNEL_BATTERY	3 /* main battery */
#define ADC_CHANNEL_BACKUP	4 /* backup battery - we don't report it */
#define ADC_CHANNEL_CHARGE	5 /* we use PLDR_BATTERY_CHARGED instead */

/*
 * Empirical range of battery values.
 * Thanks to Joseph Heenan for measurements.
 */
#define J6X0PWR_BATTERY_MIN		630
#define J6X0PWR_BATTERY_CRITICAL	635
#define J6X0PWR_BATTERY_LOW		645
#define J6X0PWR_BATTERY_FULL		910 /* can be slightly more */


/* Convenience aliases (XXX: should be provided by config_hook.h) */
#define CONFIG_HOOK_BUTTON_PRESSED	((void *)1)
#define CONFIG_HOOK_BUTTON_RELEASED	((void *)0)



/* allow only one instance */
static int j6x0pwr_attached = 0;

struct j6x0pwr_softc {
	device_t sc_dev;

	void *sc_ih;
	volatile int sc_poweroff;
};

static int	j6x0pwr_match(device_t, cfdata_t, void *);
static void	j6x0pwr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(j6x0pwr, sizeof(struct j6x0pwr_softc),
    j6x0pwr_match, j6x0pwr_attach, NULL, NULL);


static int	j6x0pwr_apm_getpower_hook(void *, int, long, void *);
static int	j6x0pwr_get_battery(void); /* from ADC */

static int	j6x0pwr_intr(void *);
static void	j6x0pwr_sleep(void *);
static int	j6x0pwr_clear_interrupt(void);


static int
j6x0pwr_match(device_t parent, cfdata_t cfp, void *aux)
{

	/*
	 * XXX: does platid_mask_MACH_HP_LX matches _JORNADA_6XX too?
	 * Is 620 wired similarly?
	 */
	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX))
		return 0;

	if (strcmp(cfp->cf_name, "j6x0pwr") != 0)
		return 0;

	/* allow only one instance */
	return !j6x0pwr_attached;
}


static void
j6x0pwr_attach(device_t parent, device_t self, void *aux)
{
	struct j6x0pwr_softc *sc;

	/* XXX: in machdep.c */
	extern void (*__sleep_func)(void *);
	extern void *__sleep_ctx;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	/* allow only one instance */
	j6x0pwr_attached = 1;

	/* arrange for hpcapm to call us when power status is requested */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_ACADAPTER,
		    CONFIG_HOOK_EXCLUSIVE,
		    j6x0pwr_apm_getpower_hook, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CHARGE,
		    CONFIG_HOOK_EXCLUSIVE,
		    j6x0pwr_apm_getpower_hook, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BATTERYVAL,
		    CONFIG_HOOK_EXCLUSIVE,
		    j6x0pwr_apm_getpower_hook, sc);

	/* register sleep function to APM */
	__sleep_func = j6x0pwr_sleep;
	__sleep_ctx = sc;

	sc->sc_poweroff = 0;

	/* drain the old interrupt */
	j6x0pwr_clear_interrupt();

	sc->sc_ih = intc_intr_establish(SH7709_INTEVT2_IRQ0, IST_EDGE, IPL_TTY,
	    j6x0pwr_intr, sc);

	_reg_write_1(SH7709_PKDR, 0);	/* Green LED on */

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "unable to establish power handler\n");
}


/*
 * Triggered when the On/Off button is pressed or the lid is closed.
 * The state of the lid is reflected in the bit PGDR[0].
 * Closing the lid can trigger several consecutive interrupts.
 *
 * XXX: Since we don't put the machine to sleep, I have no idea how
 * wakeup interrupt(s) look like.  Need to revisit when software
 * suspend is added to the kernel (which we need to support ACPI).
 */
static int
j6x0pwr_intr(void *arg)
{
	struct j6x0pwr_softc *sc = arg;
	device_t self = sc->sc_dev;
	uint8_t irr0;
	uint8_t pgdr;

	irr0 = j6x0pwr_clear_interrupt();
	if ((irr0 & IRR0_IRQ0) == 0) {
#ifdef DIAGNOSTIC
		aprint_normal_dev(self, "irr0=0x%02x?\n", irr0);
#endif
		return 0;
	}

	pgdr = _reg_read_1(SH7709_PGDR);
	if ((pgdr & PGDR_LID_OPEN) == 0) {
		aprint_normal_dev(self, "lid closed %d\n", sc->sc_poweroff);
		if (sc->sc_poweroff)
			return 1;
	} else {
		aprint_normal_dev(self, "ON/OFF %d\n", sc->sc_poweroff);
		if (sc->sc_poweroff)
			sc->sc_poweroff = 0;
	}

	config_hook_call(CONFIG_HOOK_BUTTONEVENT,
			 CONFIG_HOOK_BUTTONEVENT_POWER,
			 CONFIG_HOOK_BUTTON_PRESSED);
	/* fake release */
	config_hook_call(CONFIG_HOOK_BUTTONEVENT,
			 CONFIG_HOOK_BUTTONEVENT_POWER,
			 CONFIG_HOOK_BUTTON_RELEASED);

	return 1;
}


static int
j6x0pwr_clear_interrupt(void)
{
	uint8_t irr0;

	irr0 = _reg_read_1(SH7709_IRR0);
	if (irr0 & IRR0_IRQ0)
		_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ0);

	return irr0;
}


void
j6x0pwr_sleep(void *arg)
{
	/* splhigh on entry */
	struct j6x0pwr_softc *sc = arg;
	int s;

	/* Reinstall j6x0pwr_intr as a wakeup handler */
	intc_intr_disestablish(sc->sc_ih);
	sc->sc_ih = intc_intr_establish(SH7709_INTEVT2_IRQ0, IST_EDGE, IPL_HIGH,
	    j6x0pwr_intr, sc);
	sc->sc_poweroff = 1;
	do {
		/* Disable interrupt except for power button. */
		s = _cpu_intr_resume(IPL_CLOCK << 4);
		_reg_write_1(SH7709_PKDR, 0xff);	/* Green LED off */

		__asm volatile("sleep");

		_reg_write_1(SH7709_PKDR, 0);		/* Green LED on */
		_cpu_intr_resume(s);
	} while (sc->sc_poweroff);

	/* Return to normal power button */
	intc_intr_disestablish(sc->sc_ih);
	sc->sc_ih = intc_intr_establish(SH7709_INTEVT2_IRQ0, IST_EDGE, IPL_TTY,
	    j6x0pwr_intr, sc);
	/* splhigh on exit */
}


static int
j6x0pwr_apm_getpower_hook(void *ctx, int type, long id, void *msg)
{
	/* struct j6x0pwr_softc * const sc = ctx; */
	int * const pval = msg;
	int battery, state;

	if (type != CONFIG_HOOK_GET)
		return EINVAL;

	switch (id) {

	case CONFIG_HOOK_ACADAPTER:
		*pval = j6x0pwr_ac_is_off() ? APM_AC_OFF : APM_AC_ON;
		return 0;

	case CONFIG_HOOK_CHARGE:
		if (j6x0pwr_battery_is_absent())
			state = APM_BATT_FLAG_NO_SYSTEM_BATTERY;
		else {
			battery = j6x0pwr_get_battery();
			if (battery < J6X0PWR_BATTERY_CRITICAL)
				state = APM_BATT_FLAG_CRITICAL;
			else if (battery < J6X0PWR_BATTERY_LOW)
				state = APM_BATT_FLAG_LOW;
			else
				state = APM_BATT_FLAG_HIGH; /* XXX? */

			if (!j6x0pwr_is_not_charging())
				state |= APM_BATT_FLAG_CHARGING;
		}
		*pval = state;
		return 0;

	case CONFIG_HOOK_BATTERYVAL:
		if (j6x0pwr_battery_is_absent())
			state = 0;
		else {
			battery = j6x0pwr_get_battery();
			if (battery > J6X0PWR_BATTERY_FULL)
				state = 100;
			else
				state = 100 * (battery - J6X0PWR_BATTERY_MIN)
					/ (J6X0PWR_BATTERY_FULL
					   - J6X0PWR_BATTERY_MIN);
		}
		*pval = state;
		return 0;
	}

	return EINVAL;
}


static int
j6x0pwr_get_battery(void)
{
	int battery;
	int s;

	if (j6x0pwr_battery_is_absent())
		return -1;

	s = spltty();
	battery = adc_sample_channel(ADC_CHANNEL_BATTERY);
	splx(s);

	return battery;
}
