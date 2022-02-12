/* $NetBSD: pcagpio.c,v 1.12 2022/02/12 03:24:35 riastradh Exp $ */

/*-
 * Copyright (c) 2020 Michael Lorenz
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
 * a driver for Philips Semiconductor PCA9555 GPIO controllers
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcagpio.c,v 1.12 2022/02/12 03:24:35 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#ifdef PCAGPIO_DEBUG
#include <sys/kernel.h>
#endif
#include <sys/conf.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>
#include <dev/led.h>

#ifdef PCAGPIO_DEBUG
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

/* commands */
#define PCAGPIO_INPUT	0x00	/* line status */
#define PCAGPIO_OUTPUT	0x01	/* output status */
#define PCAGPIO_REVERT	0x02	/* revert input if set */
#define PCAGPIO_CONFIG	0x03	/* input if set, output if not */

static int	pcagpio_match(device_t, cfdata_t, void *);
static void	pcagpio_attach(device_t, device_t, void *);
static int	pcagpio_detach(device_t, int);
#ifdef PCAGPIO_DEBUG
static void	pcagpio_timeout(void *);
#endif

/* we can only pass one cookie to led_attach() but we need several values... */
struct pcagpio_led {
	void *cookie;
	struct led_device *led;
	uint32_t mask, v_on, v_off;
};

struct pcagpio_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	int		sc_is_16bit;
	uint32_t	sc_state;
	struct pcagpio_led sc_leds[16];
	int		sc_nleds;

#ifdef PCAGPIO_DEBUG
	uint32_t	sc_dir, sc_in;
	callout_t	sc_timer;
#endif
};


static void 	pcagpio_writereg(struct pcagpio_softc *, int, uint32_t);
static uint32_t pcagpio_readreg(struct pcagpio_softc *, int);
static void	pcagpio_attach_led(
			struct pcagpio_softc *, char *, int, int, int);
static int	pcagpio_get(void *);
static void	pcagpio_set(void *, int);

CFATTACH_DECL_NEW(pcagpio, sizeof(struct pcagpio_softc),
    pcagpio_match, pcagpio_attach, pcagpio_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "i2c-pca9555",	.value = 1 },
	{ .compat = "pca9555",		.value = 1 },
	{ .compat = "i2c-pca9556",	.value = 0 },
	{ .compat = "pca9556",		.value = 0 },
	DEVICE_COMPAT_EOL
};

static int
pcagpio_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

#ifdef PCAGPIO_DEBUG
static void
printdir(char* name, uint32_t val, uint32_t mask, char letter)
{
	char flags[17], bits[17];
	uint32_t bit = 0x8000;
	int i;

	val &= mask;
	for (i = 0; i < 16; i++) {
		flags[i] = (mask & bit) ? letter : '-';
		bits[i] = (val & bit) ? 'X' : ' ';
		bit = bit >> 1;
	}
	flags[16] = 0;
	bits[16] = 0;
	printf("%s: dir: %s\n", name, flags);
	printf("%s: lvl: %s\n", name, bits);
}	
#endif

static void
pcagpio_attach(device_t parent, device_t self, void *aux)
{
	struct pcagpio_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	const struct device_compatible_entry *dce;
	prop_dictionary_t dict = device_properties(self);
	prop_array_t pins;
	prop_dictionary_t pin;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_nleds = 0;

	aprint_naive("\n");
	sc->sc_is_16bit = 0;
	if ((dce = iic_compatible_lookup(ia, compat_data)) != NULL)
		sc->sc_is_16bit = dce->value;

	aprint_normal(": %s\n", sc->sc_is_16bit ? "PCA9555" : "PCA9556");

	sc->sc_state = pcagpio_readreg(sc, PCAGPIO_OUTPUT);

#ifdef PCAGPIO_DEBUG
	uint32_t in, out;
	sc->sc_dir = pcagpio_readreg(sc, PCAGPIO_CONFIG);
	sc->sc_in = pcagpio_readreg(sc, PCAGPIO_INPUT);
	in = sc-> sc_in;
	out = sc->sc_state;

	out &= ~sc->sc_dir;
	in &= sc->sc_dir;

	printdir(device_xname(sc->sc_dev), in, sc->sc_dir, 'I');
	printdir(device_xname(sc->sc_dev), out, ~sc->sc_dir, 'O');

	callout_init(&sc->sc_timer, CALLOUT_MPSAFE);
	callout_reset(&sc->sc_timer, hz*20, pcagpio_timeout, sc);

#endif

	pins = prop_dictionary_get(dict, "pins");
	if (pins != NULL) {
		int i, num, def;
		char name[32];
		const char *spptr, *nptr;
		bool ok = TRUE, act;

		for (i = 0; i < prop_array_count(pins); i++) {
			nptr = NULL;
			pin = prop_array_get(pins, i);
			ok &= prop_dictionary_get_string(pin, "name",
			    &nptr);
			ok &= prop_dictionary_get_uint32(pin, "pin", &num);
			ok &= prop_dictionary_get_bool( pin, "active_high",
			    &act);
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
			if (!strncmp(nptr, "LED ", 4))
				pcagpio_attach_led(sc, name, num, act, def);
		}
	}
}

static int
pcagpio_detach(device_t self, int flags)
{
	struct pcagpio_softc *sc = device_private(self);
	int i;

	for (i = 0; i < sc->sc_nleds; i++)
		led_detach(sc->sc_leds[i].led);

#ifdef PCAGPIO_DEBUG
	callout_halt(&sc->sc_timer, NULL);
	callout_destroy(&sc->sc_timer);
#endif

	return 0;
}

#ifdef PCAGPIO_DEBUG
static void
pcagpio_timeout(void *v)
{
	struct pcagpio_softc *sc = v;
	uint32_t out, dir, in, o_out, o_in;

	out = pcagpio_readreg(sc, PCAGPIO_OUTPUT);
	dir = pcagpio_readreg(sc, PCAGPIO_CONFIG);
	in = pcagpio_readreg(sc, PCAGPIO_INPUT);
	if (out != sc->sc_state || dir != sc->sc_dir || in != sc->sc_in) {
		aprint_normal_dev(sc->sc_dev, "status change\n");
		o_out = sc->sc_state;
		o_in = sc->sc_in;
		o_out &= ~sc->sc_dir;
		o_in &= sc->sc_dir;
		printdir(device_xname(sc->sc_dev), o_in, sc->sc_dir, 'I');
		printdir(device_xname(sc->sc_dev), o_out, ~sc->sc_dir, 'O');
		sc->sc_state = out;
		sc->sc_dir = dir;
		sc->sc_in = in;
		out &= ~sc->sc_dir;
		in &= sc->sc_dir;
		printdir(device_xname(sc->sc_dev), in, sc->sc_dir, 'I');
		printdir(device_xname(sc->sc_dev), out, ~sc->sc_dir, 'O');
	}
	callout_reset(&sc->sc_timer, hz*60, pcagpio_timeout, sc);
}
#endif

static void
pcagpio_writereg(struct pcagpio_softc *sc, int reg, uint32_t val)
{
	uint8_t cmd;

	iic_acquire_bus(sc->sc_i2c, 0);
	if (sc->sc_is_16bit) {
		uint16_t creg;
		cmd = reg << 1;
		creg = htole16(val);
		iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 2, 0);
	} else {
		uint8_t creg;
		cmd = reg;
		creg = (uint8_t)val;
		iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 1, 0);
	}
	if (reg == PCAGPIO_OUTPUT) sc->sc_state = val;
	iic_release_bus(sc->sc_i2c, 0);
}		

static uint32_t pcagpio_readreg(struct pcagpio_softc *sc, int reg)
{
	uint8_t cmd;
	uint32_t ret;

	iic_acquire_bus(sc->sc_i2c, 0);
	if (sc->sc_is_16bit) {
		uint16_t creg;
		cmd = reg << 1;
		iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 2, 0);
		ret = le16toh(creg);
	} else {
		uint8_t creg;
		cmd = reg;
		iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
		    sc->sc_addr, &cmd, 1, &creg, 1, 0);
		ret = creg;
	}
	iic_release_bus(sc->sc_i2c, 0);
	return ret;
}

static void
pcagpio_attach_led(struct pcagpio_softc *sc, char *n, int pin, int act, int def)
{
	struct pcagpio_led *l;

	l = &sc->sc_leds[sc->sc_nleds];
	l->cookie = sc;
	l->mask = 1 << pin;
	l->v_on = act ? l->mask : 0;
	l->v_off = act ? 0 : l->mask;
	l->led = led_attach(n, l, pcagpio_get, pcagpio_set);
	if (def != -1) pcagpio_set(l, def);
	DPRINTF("%s: %04x %04x %04x def %d\n",
	    __func__, l->mask, l->v_on, l->v_off, def);
	sc->sc_nleds++;
}

static int
pcagpio_get(void *cookie)
{
	struct pcagpio_led *l = cookie;
	struct pcagpio_softc *sc = l->cookie;

	return ((sc->sc_state & l->mask) == l->v_on);
}

static void
pcagpio_set(void *cookie, int val)
{
	struct pcagpio_led *l = cookie;
	struct pcagpio_softc *sc = l->cookie;
	uint32_t newstate;	

	newstate = sc->sc_state & ~l->mask;
	newstate |= val ? l->v_on : l->v_off;
	DPRINTF("%s: %04x -> %04x, %04x %04x %04x\n", __func__,
	    sc->sc_state, newstate, l->mask, l->v_on, l->v_off);
	if (newstate != sc->sc_state)
		pcagpio_writereg(sc, PCAGPIO_OUTPUT, newstate);
}
