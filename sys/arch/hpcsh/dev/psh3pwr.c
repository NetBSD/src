/*	$NetBSD: psh3pwr.c,v 1.1 2007/09/24 16:16:42 kiyohara Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: psh3pwr.c,v 1.1 2007/09/24 16:16:42 kiyohara Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/sysmon/sysmonvar.h>

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

/* warn that main battery is low after drops below this value */
#define PSH3PWR_BATTERY_WARNING_THRESHOLD	200


struct psh3pwr_softc {
	struct device sc_dev;

	struct callout sc_poll_ch;
	void *sc_ih_pin;
	void *sc_ih_pout;

	int sc_plug;		/* In/Out flug */
	int sc_capacity;

	struct sysmon_envsys sc_sysmon;
	struct envsys_data sc_data;
};

static int psh3pwr_match(struct device *, struct cfdata *, void *);
static void psh3pwr_attach(struct device *, struct device *, void *);

CFATTACH_DECL(psh3pwr, sizeof(struct psh3pwr_softc),
    psh3pwr_match, psh3pwr_attach, NULL, NULL);

static int psh3pwr_intr_plug_out(void *); 
static int psh3pwr_intr_plug_in(void *);
static void psh3pwr_poll_callout(void *);
static void psh3pwr_sleep(void *);
static int psh3pwr_gtredata(struct sysmon_envsys *, envsys_data_t *);


static int
psh3pwr_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HITACHI_PERSONA))
		return 0;

	if (strcmp(cfp->cf_name, "psh3pwr") != 0)
		return 0;

	return 1;
}


static void
psh3pwr_attach(struct device *parent, struct device *self, void *aux)
{
	extern void (*__sleep_func)(void *);
	extern void *__sleep_ctx;
	struct psh3pwr_softc *sc = (struct psh3pwr_softc *)self;
	uint8_t phdr, scpdr;

	/* regisiter sleep function to APM */
	__sleep_func = psh3pwr_sleep;
	__sleep_ctx = self;

	callout_init(&sc->sc_poll_ch, 0);
	callout_setfunc(&sc->sc_poll_ch, psh3pwr_poll_callout, sc);

	phdr = _reg_read_1(SH7709_PHDR);
	_reg_write_1(SH7709_PHDR, phdr | PSH3_GREEN_LED_ON);

	printf("\n");

	sc->sc_ih_pout = intc_intr_establish(SH7709_INTEVT2_IRQ0,
	    IST_EDGE, IPL_TTY, psh3pwr_intr_plug_out, sc);
	sc->sc_ih_pin = intc_intr_establish(SH7709_INTEVT2_IRQ1,
	    IST_EDGE, IPL_TTY, psh3pwr_intr_plug_in, sc);

	/* XXXX: WindowsCE sets this bit. */
	scpdr = _reg_read_1(SH7709_SCPDR);
	if (scpdr & PSH3PWR_PLUG_OUT) {
		sc->sc_plug = 0;
		printf("%s: plug status: out\n", device_xname(&sc->sc_dev));
	} else {
		sc->sc_plug = 1;
		printf("%s: plug status: in\n", device_xname(&sc->sc_dev));
	}
	psh3pwr_poll_callout(sc);

	sc->sc_data.sensor = 0;
	sc->sc_data.units = ENVSYS_INDICATOR;
	sc->sc_data.state = ENVSYS_SVALID;
	sc->sc_data.value_cur = sc->sc_plug;
	snprintf(sc->sc_data.desc, sizeof(sc->sc_data.desc),
	    "%s %s", device_xname(&sc->sc_dev), "plug");

	sc->sc_sysmon.sme_sensor_data = &sc->sc_data;
	sc->sc_sysmon.sme_name = device_xname(&sc->sc_dev);
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = psh3pwr_gtredata;
	sc->sc_sysmon.sme_nsensors =
	    sizeof(sc->sc_data) / sizeof(struct envsys_data);

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    device_xname(&sc->sc_dev));
}


static int      
psh3pwr_intr_plug_out(void *self)
{
	struct psh3pwr_softc *sc = (struct psh3pwr_softc *)self;
	uint8_t irr0, scpdr;

	irr0 = _reg_read_1(SH7709_IRR0);
	if (!(irr0 & IRR0_IRQ0)) {
		return 0;
	}
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ0);

	/* XXXX: WindowsCE sets this bit. */
	scpdr = _reg_read_1(SH7709_SCPDR);
	_reg_write_1(SH7709_SCPDR, scpdr | PSH3PWR_PLUG_OUT);

	sc->sc_plug = 0;
	DPRINTF(("%s: plug out\n", device_xname(&sc->sc_dev)));

	return 1;
}

static int 
psh3pwr_intr_plug_in(void *self)
{
	struct psh3pwr_softc *sc = (struct psh3pwr_softc *)self;
	uint8_t irr0, scpdr;

	irr0 = _reg_read_1(SH7709_IRR0);
	if (!(irr0 & IRR0_IRQ1))
		return 0;
	_reg_write_1(SH7709_IRR0, irr0 & ~IRR0_IRQ1);

	/* XXXX: WindowsCE sets this bit. */
	scpdr = _reg_read_1(SH7709_SCPDR);
	_reg_write_1(SH7709_SCPDR, scpdr & ~PSH3PWR_PLUG_OUT);

	sc->sc_plug = 1;
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

volatile int psh3pwr_poll_verbose = 0; /* XXX: tweak from ddb */

static void
psh3pwr_poll_callout(void *self)
{
	struct psh3pwr_softc *sc = (struct psh3pwr_softc *)self;
	int s;

	s = spltty();
	sc->sc_capacity = adc_sample_channel(ADC_CHANNEL_BATTERY);
	splx(s);

	if (psh3pwr_poll_verbose != 0)
		printf_nolog("%s: main=%-4d\n",
		    device_xname(&sc->sc_dev), sc->sc_capacity);

	if (!sc->sc_plug && sc->sc_capacity < PSH3PWR_BATTERY_WARNING_THRESHOLD)
		printf("%s: WARNING: main battery %d is low!\n",
		    device_xname(&sc->sc_dev), sc->sc_capacity);

	callout_schedule(&sc->sc_poll_ch, 5 * hz);
}

static int
psh3pwr_gtredata(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct psh3pwr_softc *sc = sme->sme_cookie;

	edata->value_cur = sc->sc_plug;

	return 0;
}
