/*	$NetBSD: j6x0pwr.c,v 1.8 2005/12/24 23:24:00 perry Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: j6x0pwr.c,v 1.8 2005/12/24 23:24:00 perry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/callout.h>
#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <sh3/exception.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>

#include <sh3/dev/adcvar.h>


/* SH7709_PGDR bits pertinent to Jornada 6x0 power */
#define PGDR_MAIN_BATTERY_OUT	0x04
#define PGDR_LID_OPEN		0x01

/* A/D covnerter channels to get power stats from */
#define ADC_CHANNEL_BATTERY	3
#define ADC_CHANNEL_BACKUP	4
#define ADC_CHANNEL_CHARGE	5

/* warn that main battery is low after drops below this value */
#define J6X0PWR_BATTERY_WARNING_THRESHOLD	200


struct j6x0pwr_softc {
	struct device sc_dev;

	struct callout sc_poll_ch;
	void *sc_ih;
	volatile int sc_poweroff;
};

static int	j6x0pwr_match(struct device *, struct cfdata *, void *);
static void	j6x0pwr_attach(struct device *, struct device *, void *);

CFATTACH_DECL(j6x0pwr, sizeof(struct j6x0pwr_softc),
    j6x0pwr_match, j6x0pwr_attach, NULL, NULL);


static int	j6x0pwr_intr(void *);
static void	j6x0pwr_poll_callout(void *);
static void	j6x0pwr_sleep(void *);
static int	j6x0pwr_clear_interrupt(void);

static int
j6x0pwr_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	/*
	 * XXX: does platid_mask_MACH_HP_LX matches _JORNADA_6XX too?
	 * Is 620 wired similarly?
	 */
	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_6XX))
		return (0);

	if (strcmp(cfp->cf_name, "j6x0pwr") != 0)
		return (0);

	return (1);
}


static void
j6x0pwr_attach(struct device *parent, struct device *self, void *aux)
{
	extern void (*__sleep_func)(void *);
	extern void *__sleep_ctx;
	struct j6x0pwr_softc *sc = (struct j6x0pwr_softc *)self;

	/* regsiter sleep function to APM */
	__sleep_func = j6x0pwr_sleep;
	__sleep_ctx = self;
	sc->sc_poweroff = 0;

	/* drain the old interrupt */
	j6x0pwr_clear_interrupt();

	sc->sc_ih = intc_intr_establish(SH7709_INTEVT2_IRQ0, IST_EDGE, IPL_TTY,
	    j6x0pwr_intr, sc);

	callout_init(&sc->sc_poll_ch);
	callout_reset(&sc->sc_poll_ch, 5 * hz,
		      j6x0pwr_poll_callout, sc);

	_reg_write_1(SH7709_PKDR, 0);	/* Green LED on */
	printf("\n");
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
j6x0pwr_intr(void *self)
{
	struct j6x0pwr_softc *sc = (struct j6x0pwr_softc *)self;
	uint8_t irr0;
	uint8_t pgdr;

	if (((irr0 = j6x0pwr_clear_interrupt()) & IRR0_IRQ0) == 0) {
#ifdef DIAGNOSTIC
		printf_nolog("%s: irr0=0x%02x?\n", sc->sc_dev.dv_xname, irr0);
#endif
		return (0);
	}

	pgdr = _reg_read_1(SH7709_PGDR);
	if ((pgdr & PGDR_LID_OPEN) == 0) {
		printf("%s: lid closed %d\n", sc->sc_dev.dv_xname,
		    sc->sc_poweroff);
		if (sc->sc_poweroff)
			return 1;
	} else {
		printf("%s: ON/OFF %d\n", sc->sc_dev.dv_xname, sc->sc_poweroff);
		if (sc->sc_poweroff)
			sc->sc_poweroff = 0;
	}

	/* push */
	config_hook_call(CONFIG_HOOK_BUTTONEVENT, CONFIG_HOOK_BUTTONEVENT_POWER,
	    (void *)1);
	/* release (fake) */
	config_hook_call(CONFIG_HOOK_BUTTONEVENT, CONFIG_HOOK_BUTTONEVENT_POWER,
	    (void *)0);

	return (1);
}

static int
j6x0pwr_clear_interrupt()
{
	uint8_t irr0;

	irr0 = _reg_read_1(SH7709_IRR0);
	if (irr0 & IRR0_IRQ0)
		_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ0);

	return irr0;
}

void
j6x0pwr_sleep(void *self)
{
	/* splhigh on entry */
	struct j6x0pwr_softc *sc = self;
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

volatile int j6x0pwr_poll_verbose = 0; /* XXX: tweak from ddb */

static void
j6x0pwr_poll_callout(void *self)
{
	struct j6x0pwr_softc *sc = (struct j6x0pwr_softc *)self;
	int battery, backup, charging;
	uint8_t pgdr;
	int s;

	pgdr = _reg_read_1(SH7709_PGDR);

	/* just check main battery charge if not verbose and battery is in */
	if (!j6x0pwr_poll_verbose) {
		if (pgdr & PGDR_MAIN_BATTERY_OUT)
			goto reschedule;
		else {
			s = spltty();
			battery = adc_sample_channel(ADC_CHANNEL_BATTERY);
			splx(s);
			goto check_battery;
		}
	}

	s = spltty();
	battery = adc_sample_channel(ADC_CHANNEL_BATTERY);
	splx(s);

	s = spltty();
	backup = adc_sample_channel(ADC_CHANNEL_BACKUP);
	splx(s);

	s = spltty();
	charging = adc_sample_channel(ADC_CHANNEL_CHARGE);
	splx(s);

	printf_nolog("%s: main=", sc->sc_dev.dv_xname);
	if (pgdr & PGDR_MAIN_BATTERY_OUT)
		printf_nolog("<no>");
	else
		printf_nolog("%-4d", battery);

	printf_nolog(" bkup=%-4d", backup);

	/*
	 * When main battery is being charged, ADC_CHANNEL_CHARGE
	 * samples at almost zero.  It samples at 2^10 otherwise.
	 */
	if (charging < 8)
		printf_nolog("charging");

	printf_nolog("\n");

  check_battery:
	if (!(pgdr & PGDR_MAIN_BATTERY_OUT)
	    && (battery < J6X0PWR_BATTERY_WARNING_THRESHOLD))
		printf("%s: WARNING: main battery %d is low!\n",
		       sc->sc_dev.dv_xname, battery);

  reschedule:
	callout_schedule(&sc->sc_poll_ch, 5*hz);
}
