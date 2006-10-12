/* $NetBSD: nsclpcsio_isa.c,v 1.14 2006/10/12 01:31:17 christos Exp $ */

/*
 * Copyright (c) 2002
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nsclpcsio_isa.c,v 1.14 2006/10/12 01:31:17 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/gpio.h>
#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include "gpio.h"
#if NGPIO > 0
#include <dev/gpio/gpiovar.h>
#endif
#include <dev/sysmon/sysmonvar.h>

static int nsclpcsio_isa_match(struct device *, struct cfdata *, void *);
static void nsclpcsio_isa_attach(struct device *, struct device *, void *);

#define GPIO_NPINS 29
#define	SIO_GPIO_CONF_OUTPUTEN	(1 << 0)
#define	SIO_GPIO_CONF_PUSHPULL	(1 << 1)
#define	SIO_GPIO_CONF_PULLUP	(1 << 2)

struct nsclpcsio_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot, sc_gpio_iot, sc_tms_iot;
	bus_space_handle_t sc_ioh, sc_gpio_ioh, sc_tms_ioh;

	struct envsys_tre_data sc_data[3];
	struct envsys_basic_info sc_info[3];
	struct sysmon_envsys sc_sysmon;
	struct simplelock sc_lock;

#if NGPIO > 0
	/* GPIO */
	struct gpio_chipset_tag sc_gpio_gc;
	struct gpio_pin sc_gpio_pins[GPIO_NPINS];
#endif
};

#define GPIO_READ(sc, reg)			\
	bus_space_read_1((sc)->sc_gpio_iot,	\
	    (sc)->sc_gpio_ioh, (reg))
#define GPIO_WRITE(sc, reg, val)		\
	bus_space_write_1((sc)->sc_gpio_iot,	\
	    (sc)->sc_gpio_ioh, (reg), (val))

CFATTACH_DECL(nsclpcsio_isa, sizeof(struct nsclpcsio_softc),
    nsclpcsio_isa_match, nsclpcsio_isa_attach, NULL, NULL);

static const struct envsys_range tms_ranges[] = {
	{ 0, 2, ENVSYS_STEMP },
};

static u_int8_t nsread(bus_space_tag_t, bus_space_handle_t, int);
static void nswrite(bus_space_tag_t, bus_space_handle_t, int, u_int8_t);
static int nscheck(bus_space_tag_t, int);

static void tms_update(struct nsclpcsio_softc *, int);
static int tms_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);
static int tms_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);

#if NGPIO > 0
static void nsclpcsio_gpio_init(struct nsclpcsio_softc *);
static void nsclpcsio_gpio_pin_select(struct nsclpcsio_softc *, int);
static void nsclpcsio_gpio_pin_write(void *, int, int);
static int nsclpcsio_gpio_pin_read(void *, int);
static void nsclpcsio_gpio_pin_ctl(void *, int, int);
#endif

static u_int8_t
nsread(iot, ioh, idx)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int idx;
{

	bus_space_write_1(iot, ioh, 0, idx);
	return (bus_space_read_1(iot, ioh, 1));
}

static void
nswrite(iot, ioh, idx, data)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int idx;
	u_int8_t data;
{

	bus_space_write_1(iot, ioh, 0, idx);
	bus_space_write_1(iot, ioh, 1, data);
}

static int
nscheck(iot, base)
	bus_space_tag_t iot;
	int base;
{
	bus_space_handle_t ioh;
	int rv = 0;

	if (bus_space_map(iot, base, 2, 0, &ioh))
		return (0);

	/* XXX this is for PC87366 only for now */
	if (nsread(iot, ioh, 0x20) == 0xe9)
		rv = 1;

	bus_space_unmap(iot, ioh, 2);
	return (rv);
}

static int
nsclpcsio_isa_match(struct device *parent __unused,
    struct cfdata *match __unused, void *aux)
{
	struct isa_attach_args *ia = aux;
	int iobase;

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	if (ia->ia_nio > 0 && ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT) {
		/* XXX check for legal iobase ??? */
		if (nscheck(ia->ia_iot, ia->ia_io[0].ir_addr)) {
			iobase = ia->ia_io[0].ir_addr;
			goto found;
		}
		return (0);
	}

	/* PC87366 has two possible locations depending on wiring */
	if (nscheck(ia->ia_iot, 0x2e)) {
		iobase = 0x2e;
		goto found;
	}
	if (nscheck(ia->ia_iot, 0x4e)) {
		iobase = 0x4e;
		goto found;
	}
	return (0);

found:
	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = iobase;
	ia->ia_io[0].ir_size = 2;
	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;
	return (1);
}

static void
nsclpcsio_isa_attach(struct device *parent __unused, struct device *self,
    void *aux)
{
	struct nsclpcsio_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
#endif
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t val;
	int tms_iobase, gpio_iobase = 0;
	int i;

	sc->sc_iot = iot = ia->ia_iot;
	if (bus_space_map(ia->ia_iot, ia->ia_io[0].ir_addr, 2, 0, &ioh)) {
		printf(": can't map i/o space\n");
		return;
	}
	sc->sc_ioh = ioh;
	printf(": NSC PC87366 rev. %d\n", nsread(iot, ioh, 0x27));

	simple_lock_init(&sc->sc_lock);

	nswrite(iot, ioh, 0x07, 0x07); /* select gpio */

	val = nsread(iot, ioh, 0x30); /* control register */
	if (!(val & 1)) {
		printf("%s: GPIO disabled\n", sc->sc_dev.dv_xname);
	} else {
		gpio_iobase = (nsread(iot, ioh, 0x60) << 8) |
			       nsread(iot, ioh, 0x61);
		sc->sc_gpio_iot = iot;
		if (bus_space_map(iot, gpio_iobase, 0x2c, 0,
		    &sc->sc_gpio_ioh)) {
			printf("%s: can't map GPIO i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		printf("%s: GPIO at 0x%x\n", sc->sc_dev.dv_xname, gpio_iobase);

#if NGPIO > 0
		nsclpcsio_gpio_init(sc);
#endif
	}

	nswrite(iot, ioh, 0x07, 0x0e); /* select tms */

	val = nsread(iot, ioh, 0x30); /* control register */
	if (!(val & 1)) {
		printf("%s: TMS disabled\n", sc->sc_dev.dv_xname);
		return;
	}

	tms_iobase = (nsread(iot, ioh, 0x60) << 8) | nsread(iot, ioh, 0x61);
	sc->sc_tms_iot = iot;
	if (bus_space_map(iot, tms_iobase, 16, 0, &sc->sc_tms_ioh)) {
		printf("%s: can't map TMS i/o space\n", sc->sc_dev.dv_xname);
		return;
	}
	printf("%s: TMS at 0x%x\n", sc->sc_dev.dv_xname, tms_iobase);

	if (bus_space_read_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x08) & 1) {
		printf("%s: TMS in standby mode\n", sc->sc_dev.dv_xname);

		/* Wake up the TMS and enable all temperature sensors. */
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x08, 0x00);
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x09, 0x00);
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x0a, 0x01);
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x09, 0x01);
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x0a, 0x01);
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x09, 0x02);
		bus_space_write_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x0a, 0x01);

		if (!(bus_space_read_1(sc->sc_tms_iot, sc->sc_tms_ioh, 0x08)
		      & 1)) {
			printf("%s: TMS awoken\n", sc->sc_dev.dv_xname);
		} else {
			return;
		}
	}

	/* Initialize sensor meta data */
	for (i = 0; i < 3; i++) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].units = sc->sc_info[i].units = ENVSYS_STEMP;
	}
	strcpy(sc->sc_info[0].desc, "TSENS1");
	strcpy(sc->sc_info[1].desc, "TSENS2");
	strcpy(sc->sc_info[2].desc, "TNSC");

	/* Get initial set of sensor values. */
	for (i = 0; i < 3; i++)
		tms_update(sc, i);

	/*
	 * Hook into the System Monitor.
	 */
	sc->sc_sysmon.sme_ranges = tms_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_cookie = sc;

	sc->sc_sysmon.sme_gtredata = tms_gtredata;
	sc->sc_sysmon.sme_streinfo = tms_streinfo;

	sc->sc_sysmon.sme_nsensors = 3;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

#if NGPIO > 0
	/* attach GPIO framework */
	if (gpio_iobase != 0) {
		gba.gba_gc = &sc->sc_gpio_gc;
		gba.gba_pins = sc->sc_gpio_pins;
		gba.gba_npins = GPIO_NPINS;
		config_found_ia(&sc->sc_dev, "gpiobus", &gba, NULL);
	}
#endif
	return;
}

static void
tms_update(sc, chan)
	struct nsclpcsio_softc *sc;
	int chan;
{
	bus_space_tag_t iot = sc->sc_tms_iot;
	bus_space_handle_t ioh = sc->sc_tms_ioh;
	u_int8_t status;
	int8_t temp, ctemp; /* signed!! */

	simple_lock(&sc->sc_lock);

	nswrite(iot, ioh, 0x07, 0x0e); /* select tms */

	bus_space_write_1(iot, ioh, 0x09, chan); /* select */

	status = bus_space_read_1(iot, ioh, 0x0a); /* config/status */
	if (status & 0x01) {
		/* enabled */
		sc->sc_info[chan].validflags = ENVSYS_FVALID;
	}else {
		sc->sc_info[chan].validflags = 0;
		simple_unlock(&sc->sc_lock);
		return;
	}

	/*
	 * If the channel is enabled, it is considered valid.
	 * An "open circuit" might be temporary.
	 */
	sc->sc_data[chan].validflags = ENVSYS_FVALID;
	if (status & 0x40) {
		/*
		 * open circuit
		 * XXX should have a warning for it
		 */
		sc->sc_data[chan].warnflags = ENVSYS_WARN_OK; /* XXX */
		simple_unlock(&sc->sc_lock);
		return;
	}

	/* get current temperature in signed degree celsius */
	temp = bus_space_read_1(iot, ioh, 0x0b);
	sc->sc_data[chan].cur.data_us = (int)temp * 1000000 + 273150000;
	sc->sc_data[chan].validflags |= ENVSYS_FCURVALID;

	if (status & 0x0e) { /* any temperature warning? */
		/*
		 * XXX the chip documentation is a bit fuzzy - it doesn't state
		 * that the hardware OTS output depends on the "overtemp"
		 * warning bit.
		 * It seems the output gets cleared if the warning bit is reset.
		 * This sucks.
		 * The hardware might do something useful with output pins, eg
		 * throttling the CPU, so we must do the comparision in
		 * software, and only reset the bits if the reason is gone.
		 */
		if (status & 0x02) { /* low limit */
			sc->sc_data[chan].warnflags = ENVSYS_WARN_UNDER;
			/* read low limit */
			ctemp = bus_space_read_1(iot, ioh, 0x0d);
			if (temp <= ctemp) /* still valid, don't reset */
				status &= ~0x02;
		}
		if (status & 0x04) { /* high limit */
			sc->sc_data[chan].warnflags = ENVSYS_WARN_OVER;
			/* read high limit */
			ctemp = bus_space_read_1(iot, ioh, 0x0c);
			if (temp >= ctemp) /* still valid, don't reset */
				status &= ~0x04;
		}
		if (status & 0x08) { /* overtemperature */
			sc->sc_data[chan].warnflags = ENVSYS_WARN_CRITOVER;
			/* read overtemperature limit */
			ctemp = bus_space_read_1(iot, ioh, 0x0e);
			if (temp >= ctemp) /* still valid, don't reset */
				status &= ~0x08;
		}

		/* clear outdated warnings */
		if (status & 0x0e)
			bus_space_write_1(iot, ioh, 0x0a, status);
	}

	simple_unlock(&sc->sc_lock);

	return;
}

static int
tms_gtredata(sme, data)
	struct sysmon_envsys *sme;
	struct envsys_tre_data *data;
{
	struct nsclpcsio_softc *sc = sme->sme_cookie;

	tms_update(sc, data->sensor);

	*data = sc->sc_data[data->sensor];
	return (0);
}

static int
tms_streinfo(struct sysmon_envsys *sme __unused,
    struct envsys_basic_info *info)
{
#if 0
	struct nsclpcsio_softc *sc = sme->sme_cookie;
#endif
	/* XXX Not implemented */
	info->validflags = 0;

	return (0);
}

#if NGPIO > 0
static void
nsclpcsio_gpio_pin_select(struct nsclpcsio_softc *sc, int pin)
{
	u_int8_t v;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	v = ((pin / 8) << 4) | (pin % 8);

	nswrite(iot, ioh, 0x07, 0x07); /* select gpio */
	nswrite(iot, ioh, 0xf0, v);

	return;
}

static void
nsclpcsio_gpio_init(struct nsclpcsio_softc *sc)
{
	int i;

	for (i = 0; i < GPIO_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN |
		    GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
		    GPIO_PIN_PULLUP;
		/* safe defaults */
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_TRISTATE;
		sc->sc_gpio_pins[i].pin_state = GPIO_PIN_LOW;
		nsclpcsio_gpio_pin_ctl(sc, i, sc->sc_gpio_pins[i].pin_flags);
		nsclpcsio_gpio_pin_write(sc, i, sc->sc_gpio_pins[i].pin_state);
	}

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = nsclpcsio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = nsclpcsio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = nsclpcsio_gpio_pin_ctl;
}

static int
nsclpcsio_gpio_pin_read(void *aux, int pin)
{
	struct nsclpcsio_softc *sc = (struct nsclpcsio_softc *)aux;
	int port, shift, reg;
	u_int8_t v;

	reg = 0x00;
	port = pin / 8;
	shift = pin % 8;

	switch (port) {
	case 0: reg = 0x00; break;
	case 1: reg = 0x04; break;
	case 2: reg = 0x08; break;
	case 3: reg = 0x0a; break;
	}

	v = GPIO_READ(sc, reg);

	return ((v >> shift) & 0x1);
}

static void
nsclpcsio_gpio_pin_write(void *aux, int pin, int v)
{
	struct nsclpcsio_softc *sc = (struct nsclpcsio_softc *)aux;
	int port, shift, reg;
	u_int8_t d;

	port = pin / 8;
	shift = pin % 8;

	switch (port) {
	case 0: reg = 0x00; break;
	case 1: reg = 0x04; break;
	case 2: reg = 0x08; break;
	case 3: reg = 0x0a; break;
	default: reg = 0x00; break; /* shouldn't happen */
	}

	d = GPIO_READ(sc, reg);
	if (v == 0)
		d &= ~(1 << shift);
	else if (v == 1)
		d |= (1 << shift);
	GPIO_WRITE(sc, reg, d);

	return;
}

void
nsclpcsio_gpio_pin_ctl(void *aux, int pin, int flags)
{
	struct nsclpcsio_softc *sc = (struct nsclpcsio_softc *)aux;
	u_int8_t conf;

	simple_lock(&sc->sc_lock);

	nswrite(sc->sc_iot, sc->sc_ioh, 0x07, 0x07); /* select gpio */
	nsclpcsio_gpio_pin_select(sc, pin);
	conf = nsread(sc->sc_iot, sc->sc_ioh, 0xf1);

	conf &= ~(SIO_GPIO_CONF_OUTPUTEN | SIO_GPIO_CONF_PUSHPULL |
	    SIO_GPIO_CONF_PULLUP);
	if ((flags & GPIO_PIN_TRISTATE) == 0)
		conf |= SIO_GPIO_CONF_OUTPUTEN;
	if (flags & GPIO_PIN_PUSHPULL)
		conf |= SIO_GPIO_CONF_PUSHPULL;
	if (flags & GPIO_PIN_PULLUP)
		conf |= SIO_GPIO_CONF_PULLUP;

	nswrite(sc->sc_iot, sc->sc_ioh, 0xf1, conf);

	simple_unlock(&sc->sc_lock);

	return;
}
#endif /* NGPIO */
