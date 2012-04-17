/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/callout.h>
#include <sys/kernel.h>

#include <sys/bus.h>

#include <arm/s3c2xx0/s3c24x0var.h>
#include <arm/s3c2xx0/s3c2440var.h>
#include <arm/s3c2xx0/s3c2440reg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/tpcalibvar.h>

#include <lib/libsa/qsort.c>

#include <dev/hpc/hpcfbio.h>

#define MAX_SAMPLES 20

struct sstouch_softc {
	device_t		dev;

	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;

	uint32_t		next_stylus_intr;

	device_t		wsmousedev;

	struct tpcalib_softc	tpcalib;

	int			sample_count;
	int			samples_x[MAX_SAMPLES];
	int			samples_y[MAX_SAMPLES];

	callout_t		callout;
};

/* Basic Driver Stuff */
static int	sstouch_match	(struct device *, struct cfdata *, void *);
static void	sstouch_attach	(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(sstouch, sizeof(struct sstouch_softc), sstouch_match,
	      sstouch_attach, NULL, NULL);

/* wsmousedev */
int	sstouch_enable(void *);
int	sstouch_ioctl(void *, u_long, void *, int, struct lwp *);
void	sstouch_disable(void *);

const struct wsmouse_accessops sstouch_accessops = {
	sstouch_enable,
	sstouch_ioctl,
	sstouch_disable
};

/* Interrupt Handlers */
int sstouch_tc_intr(void *arg);
int sstouch_adc_intr(void *arg);

void sstouch_callout(void *arg);
int sstouch_filter_values(int *vals, int val_count);
void sstouch_initialize(struct sstouch_softc *sc);

#define STYLUS_DOWN	0
#define STYLUS_UP	ADCTSC_UD_SEN

static struct wsmouse_calibcoords default_calib = {
	.minx = 0,
	.miny = 0,
	.maxx = 0,
	.maxy = 0,
	.samplelen = WSMOUSE_CALIBCOORDS_RESET
};

/* IMPLEMENTATION PART */
int
sstouch_match(struct device *parent, struct cfdata *match, void *aux)
{
	/* XXX: Check CPU type? */
	return 1;
}

void
sstouch_attach(struct device	*parent,
	       struct device	*self,
	       void		*aux)
{
	struct sstouch_softc		*sc = device_private(self);
	struct s3c2xx0_attach_args	*sa = (struct s3c2xx0_attach_args*)aux;
	struct wsmousedev_attach_args	mas;

	sc->dev = self;
	sc->iot = sa->sa_iot;

	if (bus_space_map(sc->iot, S3C2440_ADC_BASE,
			  S3C2440_ADC_SIZE, 0, &sc->ioh)) {
		aprint_error(": failed to map registers");
		return;
	}

	sc->next_stylus_intr = STYLUS_DOWN;


	/* XXX: Is IPL correct? */
	s3c24x0_intr_establish(S3C2440_INT_TC, IPL_BIO, IST_EDGE_RISING,
			       sstouch_tc_intr, sc);
	s3c24x0_intr_establish(S3C2440_INT_ADC, IPL_BIO, IST_EDGE_RISING,
			       sstouch_adc_intr, sc);

	aprint_normal("\n");

	mas.accessops = &sstouch_accessops;
	mas.accesscookie = sc;

	sc->wsmousedev = config_found_ia(self, "wsmousedev", &mas,
					 wsmousedevprint);

	tpcalib_init(&sc->tpcalib);
	tpcalib_ioctl(&sc->tpcalib, WSMOUSEIO_SCALIBCOORDS,
		      (void*)&default_calib, 0, 0);

	sc->sample_count = 0;

	/* Add CALLOUT_MPSAFE to avoid holding the global kernel lock */
	callout_init(&sc->callout, 0);
	callout_setfunc(&sc->callout, sstouch_callout, sc);

	/* Actual initialization is performed by sstouch_initialize(),
	   which is called by sstouch_enable() */
}

/* sstouch_tc_intr is the TC interrupt handler.
   The TC interrupt is generated when the stylus changes up->down,
   or down->up state (depending on configuration of ADC_ADCTSC).
*/
int
sstouch_tc_intr(void *arg)
{
	struct sstouch_softc *sc = (struct sstouch_softc*)arg;
	uint32_t reg;

	/*aprint_normal("%s\n", __func__);*/

	/* Figure out if the stylus was lifted or lowered */
	reg = bus_space_read_4(sc->iot, sc->ioh, ADC_ADCUPDN);
	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCUPDN, 0x0);
	if( sc->next_stylus_intr == STYLUS_DOWN && (reg & ADCUPDN_TSC_DN) ) {
		sc->next_stylus_intr = STYLUS_UP;

		sstouch_callout(sc);

	} else if (sc->next_stylus_intr == STYLUS_UP && (reg & ADCUPDN_TSC_UP)) {
		uint32_t adctsc = 0;
		sc->next_stylus_intr = STYLUS_DOWN;

		wsmouse_input(sc->wsmousedev, 0x0, 0, 0, 0, 0, 0);

		sc->sample_count = 0;

		adctsc |= ADCTSC_YM_SEN | ADCTSC_YP_SEN | ADCTSC_XP_SEN |
			sc->next_stylus_intr |
			3; /* 3 selects "Waiting for Interrupt Mode" */
		bus_space_write_4(sc->iot, sc->ioh, ADC_ADCTSC, adctsc);
	}

	return 1;
}

/* sstouch_adc_intr is ADC interrupt handler.
   ADC interrupt is triggered when the ADC controller has a measurement ready.
*/
int
sstouch_adc_intr(void *arg)
{
	struct sstouch_softc *sc = (struct sstouch_softc*)arg;
	uint32_t reg;
	uint32_t adctsc = 0;
	int x, y;

	reg = bus_space_read_4(sc->iot, sc->ioh, ADC_ADCDAT0);
	y = reg & ADCDAT_DATAMASK;

	reg = bus_space_read_4(sc->iot, sc->ioh, ADC_ADCDAT1);
	x = reg & ADCDAT_DATAMASK;


	sc->samples_x[sc->sample_count] = x;
	sc->samples_y[sc->sample_count] = y;

	sc->sample_count++;

	x = sstouch_filter_values(sc->samples_x, sc->sample_count);
	y = sstouch_filter_values(sc->samples_y, sc->sample_count);

	if (x == -1 || y == -1) {
		/* If we do not have enough measurements, make some more. */
		sstouch_callout(sc);
		return 1;
	}

	sc->sample_count = 0;

	tpcalib_trans(&sc->tpcalib, x, y, &x, &y);

	wsmouse_input(sc->wsmousedev, 0x1, x, y, 0, 0,
		      WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);

	/* Schedule a new adc measurement, unless the stylus has been lifed */
	if (sc->next_stylus_intr == STYLUS_UP) {
		callout_schedule(&sc->callout, hz/50);
	}

	/* Until measurement is to be performed, listen for stylus up-events */
	adctsc |= ADCTSC_YM_SEN | ADCTSC_YP_SEN | ADCTSC_XP_SEN |
		sc->next_stylus_intr |
		3; /* 3 selects "Waiting for Interrupt Mode" */
	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCTSC, adctsc);


	return 1;
}

int
sstouch_enable(void *arg)
{
	struct sstouch_softc *sc = arg;

	sstouch_initialize(sc);

	return 0;
}

int
sstouch_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct sstouch_softc *sc = v;

	aprint_normal("%s\n", __func__);

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(uint *)data = WSMOUSE_TYPE_PSEUDO;
		break;
	case WSMOUSEIO_GCALIBCOORDS:
	case WSMOUSEIO_SCALIBCOORDS:
		return tpcalib_ioctl(&sc->tpcalib, cmd, data, flag, l);
	default:
		return EPASSTHROUGH;
	}

	return 0;
}

void
sstouch_disable(void *arg)
{
	struct sstouch_softc *sc = (struct sstouch_softc*)arg;

	/* By setting ADCCON register to 0, we also disable
	   the prescaler, which should disable any interrupts.
	 */
	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCCON, 0);
}

void
sstouch_callout(void *arg)
{
	struct sstouch_softc *sc = (struct sstouch_softc*)arg;

	/* If stylus is down, perform a measurement */
	if (sc->next_stylus_intr == STYLUS_UP) {
		uint32_t reg;
		bus_space_write_4(sc->iot, sc->ioh, ADC_ADCTSC,
				  ADCTSC_YM_SEN | ADCTSC_YP_SEN |
				  ADCTSC_XP_SEN | ADCTSC_PULL_UP |
				  ADCTSC_AUTO_PST);

		reg = bus_space_read_4(sc->iot, sc->ioh, ADC_ADCCON);
		bus_space_write_4(sc->iot, sc->ioh, ADC_ADCCON,
				  reg | ADCCON_ENABLE_START);
	}

}

/* Do some very simple filtering on the measured values */
int
sstouch_filter_values(int *vals, int val_count)
{
	int sum = 0;

	if (val_count < 5)
		return -1;

	for (int i=0; i<val_count; i++) {
		sum += vals[i];
	}

	return sum/val_count;
}

void
sstouch_initialize(struct sstouch_softc *sc)
{
	int prescaler;
	uint32_t adccon = 0;
	uint32_t adctsc = 0;

	/* ADC Conversion rate is calculated by:
	   f(ADC) = PCLK/(prescaler+1)

	   The ADC can operate at a maximum frequency of 2.5MHz for
	   500 KSPS.
	*/

	/* Set f(ADC) = 50MHz / 256 = 1,95MHz */
	prescaler = 0xff;

	adccon |= ((prescaler<<ADCCON_PRSCVL_SHIFT) &
		   ADCCON_PRSCVL_MASK);
	adccon |= ADCCON_PRSCEN;
	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCCON, adccon);

	/* Use Auto Sequential measurement of X and Y positions */
	adctsc |= ADCTSC_YM_SEN | ADCTSC_YP_SEN | ADCTSC_XP_SEN |
		sc->next_stylus_intr |
		3; /* 3 selects "Waiting for Interrupt Mode" */
	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCTSC, adctsc);

	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCUPDN, 0x0);

	/* Time used to measure each X/Y position value? */
	bus_space_write_4(sc->iot, sc->ioh, ADC_ADCDLY, 10000);
}
