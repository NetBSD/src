/*	$NetBSD: j6x0pwr.c,v 1.3 2003/10/22 23:52:46 uwe Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: j6x0pwr.c,v 1.3 2003/10/22 23:52:46 uwe Exp $");

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
};

static int	j6x0pwr_match(struct device *, struct cfdata *, void *);
static void	j6x0pwr_attach(struct device *, struct device *, void *);

CFATTACH_DECL(j6x0pwr, sizeof(struct j6x0pwr_softc),
    j6x0pwr_match, j6x0pwr_attach, NULL, NULL);


static int	j6x0pwr_intr(void *);
static void	j6x0pwr_poll_callout(void *);


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
	struct j6x0pwr_softc *sc = (struct j6x0pwr_softc *)self;

	intc_intr_establish(SH7709_INTEVT2_IRQ0, IST_EDGE, IPL_TTY,
			    j6x0pwr_intr, sc);

	callout_init(&sc->sc_poll_ch);
	callout_reset(&sc->sc_poll_ch, 5 * hz,
		      j6x0pwr_poll_callout, sc);

	printf("\n");
}


/*
 * Triggered when the On/Off button is pressed or the lid is closed.
 * The state of the lid is reflected in the bit PGDR[2].
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

	irr0 = _reg_read_1(SH7709_IRR0);
	if ((irr0 & IRR0_IRQ0) == 0) {
#ifdef DIAGNOSTIC
		printf_nolog("%s: irr0=0x%02x?\n", sc->sc_dev.dv_xname, irr0);
#endif
		return (0);
	}

	/* clear the interrupt */
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ0);

	pgdr = _reg_read_1(SH7709_PGDR);
	if ((pgdr & PGDR_LID_OPEN) == 0)
		printf("%s: lid closed\n", sc->sc_dev.dv_xname);
	else 
		printf("%s: ON/OFF\n", sc->sc_dev.dv_xname);

	return (1);
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
