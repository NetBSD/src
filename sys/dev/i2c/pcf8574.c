/* $NetBSD: pcf8574.c,v 1.5 2020/12/06 10:09:36 jdc Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

/*
 * A driver for Philips Semiconductor (NXP) PCF8574/PCF857A GPIO's.
 * Uses device properties to connect pins to the appropriate subsystem.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcf8574.c,v 1.5 2020/12/06 10:09:36 jdc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/led.h>

#ifdef PCF8574_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

struct pcf8574_led {
	void *cookie;
	struct led_device *led;
	uint8_t mask, v_on, v_off;
};

#define PCF8574_NPINS	8
struct pcf8574_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;
	uint8_t		sc_state;
	uint8_t		sc_mask;

	int		sc_nleds;
	struct pcf8574_led sc_leds[PCF8574_NPINS];

	struct sysmon_envsys *sc_sme;
	envsys_data_t	sc_sensor[PCF8574_NPINS];
	int		sc_pin_sensor[PCF8574_NPINS];
	int		sc_pin_active[PCF8574_NPINS];

#ifdef PCF8574_DEBUG
	callout_t	sc_timer;
#endif
};

static int	pcf8574_match(device_t, cfdata_t, void *);
static void	pcf8574_attach(device_t, device_t, void *);
static int	pcf8574_detach(device_t, int);

static int	pcf8574_read(struct pcf8574_softc *sc, uint8_t *val);
static int	pcf8574_write(struct pcf8574_softc *sc, uint8_t val);
static void	pcf8574_attach_led(
			struct pcf8574_softc *, char *, int, int, int);
void		pcf8574_refresh(struct sysmon_envsys *, envsys_data_t *);
int		pcf8574_get_led(void *);
void		pcf8574_set_led(void *, int);

#ifdef PCF8574_DEBUG
static void pcf8574_timeout(void *);
#endif

CFATTACH_DECL_NEW(pcf8574io, sizeof(struct pcf8574_softc),
	pcf8574_match, pcf8574_attach, pcf8574_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ "i2c-pcf8574",		0 },
	{ NULL,				0 }
};

static int
pcf8574_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result))
		return match_result;

	/* We don't support indirect matches */
	return 0;
}

static void
pcf8574_attach(device_t parent, device_t self, void *aux)
{
	struct pcf8574_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	prop_dictionary_t dict = device_properties(self);
	prop_array_t pins;
	prop_dictionary_t pin;
	int i, num, def, envc = 0;
	char name[32];
	const char *nptr = NULL, *spptr;
	bool ok = TRUE, act, sysmon = FALSE;

	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_dev = self;

	/*
	 * The PCF8574 requires input pins to be written with the value 1,
	 * and then read.  Assume that all pins are input initially.
	 * We'll use the mask when we write, because we have to write a
	 * value for every pin, and we don't want to change input pins.
	 */
	sc->sc_mask = 0xff;

	/* Try a read, and fail if this component isn't present */
	if (pcf8574_read(sc, &sc->sc_state)) {
		aprint_normal(": read failed\n");
		return;
	}

#ifdef PCF8574_DEBUG
	aprint_normal(": GPIO: state = 0x%02x\n", sc->sc_state);

	callout_init(&sc->sc_timer, CALLOUT_MPSAFE);
	callout_reset(&sc->sc_timer, hz*30, pcf8574_timeout, sc);
#else
	aprint_normal(": GPIO\n");
#endif

	pins = prop_dictionary_get(dict, "pins");
	if (pins == NULL)
		return;

	for (i = 0; i < prop_array_count(pins); i++) {
		pin = prop_array_get(pins, i);
		ok &= prop_dictionary_get_cstring_nocopy(pin, "name", &nptr);
		ok &= prop_dictionary_get_uint32(pin, "pin", &num);
		ok &= prop_dictionary_get_bool(pin, "active_high", &act);
		/* optional default state */
		def = -1;
		prop_dictionary_get_int32(pin, "default_state", &def);
		if (!ok)
			continue;
		/* Extract pin type from the name */
		spptr = strstr(nptr, " ");
		if (spptr == NULL)
			continue;
		spptr += 1;
		strncpy(name, spptr, 31);
		sc->sc_pin_active[i] = act;
		if (!strncmp(nptr, "LED ", 4)) {
			sc->sc_mask &= ~(1 << num);
			pcf8574_attach_led(sc, name, num, act, def);
		}
		if (!strncmp(nptr, "INDICATOR ", 4)) {
			if (!sysmon) {
				sc->sc_sme = sysmon_envsys_create();
				sysmon = TRUE;
			}
			/* envsys sensor # to pin # mapping */
			sc->sc_pin_sensor[envc] = num;
			sc->sc_sensor[i].state = ENVSYS_SINVALID;
			sc->sc_sensor[i].units = ENVSYS_INDICATOR;
			strlcpy(sc->sc_sensor[i].desc, name,
			    sizeof(sc->sc_sensor[i].desc));
			if (sysmon_envsys_sensor_attach(sc->sc_sme,
			    &sc->sc_sensor[i])) {
				sysmon_envsys_destroy(sc->sc_sme);
				sc->sc_sme = NULL;
				aprint_error_dev(self,
				    "unable to attach pin %d at sysmon\n", i);
				return;
			}
			DPRINTF("%s: added indicator: pin %d sensor %d (%s)\n",
			    device_xname(sc->sc_dev), num, envc, name);
			envc++;
		}
	}

	if (sysmon) {
		sc->sc_sme->sme_name = device_xname(self);
		sc->sc_sme->sme_cookie = sc;
		sc->sc_sme->sme_refresh = pcf8574_refresh;
		if (sysmon_envsys_register(sc->sc_sme)) {
			aprint_error_dev(self,
			    "unable to register with sysmon\n");
			sysmon_envsys_destroy(sc->sc_sme);
			sc->sc_sme = NULL;
			return;
		}
	}
}

static int
pcf8574_detach(device_t self, int flags)
{
	struct pcf8574_softc *sc = device_private(self);
	int i;

	if (sc->sc_sme != NULL) {
		sysmon_envsys_unregister(sc->sc_sme);
		sc->sc_sme = NULL;
	}

	for (i = 0; i < sc->sc_nleds; i++)
		led_detach(sc->sc_leds[i].led);

#ifdef PCF8574_DEBUG
	callout_halt(&sc->sc_timer, NULL);
	callout_destroy(&sc->sc_timer);
#endif
	return 0;
}

static int
pcf8574_read(struct pcf8574_softc *sc, uint8_t *val)
{
	int err = 0;

	if ((err = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return err;
	err = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
	    sc->sc_addr, NULL, 0, val, 1, 0);
	iic_release_bus(sc->sc_tag, 0);
	return err;
}

static int
pcf8574_write(struct pcf8574_softc *sc, uint8_t val)
{
	int err = 0;

	if ((err = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return err;
	err = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    &val, 1, NULL, 0, 0);
	iic_release_bus(sc->sc_tag, 0);
	return err;
}

static void
pcf8574_attach_led(struct pcf8574_softc *sc, char *n, int pin, int act, int def)
{
	struct pcf8574_led *l;

	l = &sc->sc_leds[sc->sc_nleds];
	l->cookie = sc;
	l->mask = 1 << pin;
	l->v_on = act ? l->mask : 0;
	l->v_off = act ? 0 : l->mask;
	led_attach(n, l, pcf8574_get_led, pcf8574_set_led);
	if (def != -1)
		pcf8574_set_led(l, def);
	DPRINTF("%s: attached LED: %02x %02x %02x def %d\n",
	    device_xname(sc->sc_dev), l->mask, l->v_on, l->v_off, def);
	sc->sc_nleds++;
}

void
pcf8574_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct pcf8574_softc *sc = sme->sme_cookie;
	int pin = sc->sc_pin_sensor[edata->sensor];
	int act = sc->sc_pin_active[pin];

	pcf8574_read(sc, &sc->sc_state);
	if (act)
		edata->value_cur = sc->sc_state & 1 << pin ? TRUE : FALSE;
	else
		edata->value_cur = sc->sc_state & 1 << pin ? FALSE : TRUE;
	edata->state = ENVSYS_SVALID;
}

int
pcf8574_get_led(void *cookie)
{
	struct pcf8574_led *l = cookie;
	struct pcf8574_softc *sc = l->cookie;

	return ((sc->sc_state & l->mask) == l->v_on);
}

void
pcf8574_set_led(void *cookie, int val)
{
	struct pcf8574_led *l = cookie;
	struct pcf8574_softc *sc = l->cookie;
	uint32_t newstate;	

	newstate = sc->sc_state & ~l->mask;
	newstate |= val ? l->v_on : l->v_off;
	DPRINTF("%s: set LED: %02x -> %02x, %02x %02x %02x\n",
	    device_xname(sc->sc_dev), sc->sc_state, newstate, l->mask, l->v_on, l->v_off);
	if (newstate != sc->sc_state) {
		pcf8574_write(sc, newstate | sc->sc_mask);
		pcf8574_read(sc, &sc->sc_state);
	}
}

#ifdef PCF8574_DEBUG
static void
pcf8574_timeout(void *v)
{
	struct pcf8574_softc *sc = v;
	uint8_t val;

	pcf8574_read(sc, &val);
	if (val != sc->sc_state)
		aprint_normal_dev(sc->sc_dev,
		    "status change: 0x%02x > 0x%02x\n", sc->sc_state, val);
	sc->sc_state = val;

	callout_reset(&sc->sc_timer, hz*60, pcf8574_timeout, sc);
}
#endif
