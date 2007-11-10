/*	$NetBSD: omap_rtc.c,v 1.1.6.2.4.1 2007/11/10 02:56:50 matt Exp $	*/

/*
 * OMAP RTC driver, based on i80321_timer.c.
 *
 * Copyright (c) 2007 Danger Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap_rtc.c,v 1.1.6.2.4.1 2007/11/10 02:56:50 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>

/* RTC year values relative to this year */

#define	BASEYEAR		2000

/* Minimum plausible year for times. */

#define SANEYEAR		2006

/* Register offsets and bit fields. */

#define SECONDS_REG		0x00
#define MINUTES_REG		0x04
#define HOURS_REG		0x08
#define DAYS_REG		0x0c
#define MONTHS_REG		0x10
#define YEARS_REG		0x14
#define WEEKS_REG		0x18
#define ALARM_SECONDS_REG	0x20
#define ALARM_MINUTES_REG	0x24
#define ALARM_HOURS_REG		0x28
#define ALARM_DAYS_REG		0x2c
#define ALARM_MONTHS_REG	0x30
#define ALARM_YEARS_REG		0x34

#define RTC_CTRL_REG		0x40
#define STOP_RTC		0

#define RTC_STATUS_REG		0x44
#define ALARM			6
#define BUSY			0

#define RTC_INTERRUPTS_REG	0x48
#define IT_ALARM		3

static int	omaprtc_match(struct device *, struct cfdata *, void *);
static void	omaprtc_attach(struct device *, struct device *, void *);

struct omaprtc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_intr;
	void			*sc_irqcookie;
};

static struct omaprtc_softc *rtc_sc = NULL;

CFATTACH_DECL(omaprtc, sizeof(struct omaprtc_softc),
    omaprtc_match, omaprtc_attach, NULL, NULL);

int omaprtc_gettime(volatile struct timeval *tv);
int omaprtc_settime(volatile struct timeval *tv);
int omaprtc_setalarmtime(struct timeval *tv);

#define rtc_is_busy()	(bus_space_read_1(rtc_sc->sc_iot, rtc_sc->sc_ioh, \
					  RTC_STATUS_REG) & 1<<BUSY)

int
omaprtc_gettime(volatile struct timeval *tv)
{
	struct clock_ymdhms dt;
	int s;

	/* Wait for RTC_STATUS_REG:BUSY to go low to
	 * guarantee our read is correct.  BUSY will
	 * only be high for one 32kHz period (30.5us)
	 * each second, so we'll usually pass right
	 * through.
	 *
	 * Watch RTC_CTRL_REG:STOP_RTC as well so
	 * we don't spin forever if someone disables the RTC.
	 *
	 * Turn interrupts off, because we are only allowed
	 * to read/write the registers for 1/2 of a 32kHz
	 * clock period (= 15us) after detecting that BUSY
	 * is low.
	 */

	s = disable_interrupts(I32_bit);

	while (rtc_is_busy()) {
		;
	}

	dt.dt_year =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 YEARS_REG)) + BASEYEAR;
	dt.dt_mon =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 MONTHS_REG));
	dt.dt_wday =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 WEEKS_REG) & 0x0f);
	dt.dt_day =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 DAYS_REG));
	dt.dt_sec =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 SECONDS_REG));
	dt.dt_hour =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 HOURS_REG));
	dt.dt_min =
		FROMBCD(bus_space_read_1(rtc_sc->sc_iot,
					 rtc_sc->sc_ioh,
					 MINUTES_REG));
	restore_interrupts(s);
	tv->tv_sec = clock_ymdhms_to_secs(&dt);
        tv->tv_usec = 0;
        return 0;
}

int
omaprtc_settime(volatile struct timeval *tv)
{
	struct clock_ymdhms dt;
	int s;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	/* Cancel any pending alarm since it may fire at the wrong time
	   after the time change. */

	omaprtc_setalarmtime(NULL);
	s = disable_interrupts(I32_bit);

	while (rtc_is_busy()) {
		;
	}

	/* It's ok to write these without stopping the
	 * RTC, because the BUSY mechanism lets us guarantee
	 * that we're not in the middle of, e.g., rolling
	 * seconds into minutes.
	 */

	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  YEARS_REG, TOBCD(dt.dt_year - BASEYEAR));
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  MONTHS_REG, TOBCD(dt.dt_mon));
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  WEEKS_REG, TOBCD(dt.dt_wday & 0x0f));
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  DAYS_REG, TOBCD(dt.dt_day));
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  SECONDS_REG, TOBCD(dt.dt_sec));
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  HOURS_REG, TOBCD(dt.dt_hour));
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  MINUTES_REG, TOBCD(dt.dt_min));
	restore_interrupts(s);
        return 0;
}

int
omaprtc_setalarmtime(struct timeval *tv)
{
	struct clock_ymdhms dt;
	unsigned char val, intval;
	int s;

	if (tv)
		clock_secs_to_ymdhms(tv->tv_sec, &dt);

	s = disable_interrupts(I32_bit);

	/* disarm alarm interrupt */
	intval = bus_space_read_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			       RTC_INTERRUPTS_REG);
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  RTC_INTERRUPTS_REG, intval & ~(1<<IT_ALARM));

	/* clear pending alarm */
	/* todo: correct?  write 1 to clear interrupt? */
	val = bus_space_read_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			       RTC_STATUS_REG);
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  RTC_STATUS_REG, val & ~(1<<ALARM));

	/* TODO: Need to clear in interrupt controller? */

	if (tv) {
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  ALARM_YEARS_REG,
				  TOBCD(dt.dt_year - BASEYEAR));
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  ALARM_MONTHS_REG, TOBCD(dt.dt_mon));
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  ALARM_DAYS_REG, TOBCD(dt.dt_day));
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  ALARM_SECONDS_REG, TOBCD(dt.dt_sec));
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  ALARM_HOURS_REG, TOBCD(dt.dt_hour));
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  ALARM_MINUTES_REG, TOBCD(dt.dt_min));

		/* Enable the interrupt */
		bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
				  RTC_INTERRUPTS_REG, intval | (1<<IT_ALARM));
	}

	restore_interrupts(s);
        return 0;
}

static int
omaprtc_alarmintr(void *arg)
{
	unsigned char val;

	/* De-assert the IRQ_ALARM_CHIP line
	 * by writing into RTC_STATUS_REG:ALARM.
	 */
	val = bus_space_read_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			       RTC_STATUS_REG);
	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh,
			  RTC_STATUS_REG, val | (1<<ALARM));
	printf("RTC alarm interrupt.\n");
	return 1;
}


/*
 * inittodr:
 *
 *	Initialize time from the time-of-day register.
 */

void
inittodr(time_t base)
{
	time_t deltat;
	int badbase;

	if (base < (SANEYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = (SANEYEAR - 1970) * SECYR;
		badbase = 1;
        } else {
                badbase = 0;
	}

	time.tv_sec = base;
	time.tv_usec = 0;

        if (omaprtc_gettime(&time) != 0 ||
	    (time.tv_sec < (SANEYEAR - 1970) * SECYR)) {
                /*
                 * Believe the time in the file system for lack of
                 * anything better, resetting the TODR.
                 */
                time.tv_sec = base;
                time.tv_usec = 0;
                if (!badbase) {
                        printf("WARNING: preposterous clock chip time\n");
                        resettodr();
                }
                goto bad;
        }

        if (!badbase) {
                /*
                 * See if we gained/lost two or more days; if
                 * so, assume something is amiss.
                 */
                deltat = time.tv_sec - base;
                if (deltat < 0)
                        deltat = -deltat;
                if (deltat < 2 * SECDAY)
                        return;         /* all is well */
                printf("WARNING: clock %s %ld days\n",
                    time.tv_sec < base ? "lost" : "gained",
                    (long)deltat / SECDAY);
        }
 bad:
        printf("WARNING: CHECK AND RESET THE DATE!\n");


}

/*
 * resettodr:
 *
 *	Reset the time-of-day register with the current time.
 */
void
resettodr(void)
{
        if (time.tv_sec == 0)
                return;

        if (omaprtc_settime(&time) != 0)
                printf("resettodr: failed to set time\n");
}

static int
omaprtc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1)
	    panic("omaprtc must have addr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 2048;	/* Per the OMAP TRM. */

	/* We implicitly trust the config file. */
	return (1);
}

void
omaprtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct omaprtc_softc *sc = (struct omaprtc_softc*)self;
	struct tipb_attach_args *tipb = aux;

	sc->sc_iot = tipb->tipb_iot;
	sc->sc_intr = tipb->tipb_intr;

	if (bus_space_map(tipb->tipb_iot, tipb->tipb_addr, tipb->tipb_size, 0,
			 &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	aprint_normal(": OMAP RTC\n");
	aprint_naive("\n");
	rtc_sc = sc;

	if (sc->sc_intr != -1) {
		sc->sc_irqcookie = omap_intr_establish(sc->sc_intr, IPL_CLOCK,
						       sc->sc_dev.dv_xname,
						       omaprtc_alarmintr, sc);
		if (sc->sc_irqcookie == NULL) {
			aprint_error("%s: Failed to install alarm interrupt"
				     " handler.\n",
				     sc->sc_dev.dv_xname);
		}
	}

	/*
	 * Start RTC (STOP_RTC=1 starts it, go figure), 24 hour mode,
	 * no autocompensation, no rounding, 32KHz clock enabled,
	 * cannot use split power, no test mode.
	 */

	bus_space_write_1(rtc_sc->sc_iot, rtc_sc->sc_ioh, RTC_CTRL_REG,
			  1 << STOP_RTC);
}
