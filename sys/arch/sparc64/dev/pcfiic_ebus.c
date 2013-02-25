/*	$NetBSD: pcfiic_ebus.c,v 1.3.2.1 2013/02/25 00:28:59 tls Exp $	*/
/*	$OpenBSD: pcfiic_ebus.c,v 1.13 2008/06/08 03:07:40 deraadt Exp $ */

/*
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcfiic_ebus.c,v 1.3.2.1 2013/02/25 00:28:59 tls Exp $");

/*
 * Device specific driver for the EBus i2c devices found on some sun4u
 * systems. On systems not having a boot-bus controller the i2c devices
 * are PCF8584.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/rwlock.h>

#include <sys/bus.h>
#include <machine/openfirm.h>
#include <machine/autoconf.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ic/pcf8584var.h>

int	pcfiic_ebus_match(device_t, struct cfdata *, void *);
void	pcfiic_ebus_attach(device_t, device_t, void *);

struct pcfiic_ebus_softc {
	struct pcfiic_softc	esc_sc;

	int			esc_node;
	void			*esc_ih;
};

CFATTACH_DECL_NEW(pcfiic, sizeof(struct pcfiic_ebus_softc),
	pcfiic_ebus_match, pcfiic_ebus_attach, NULL, NULL);

static prop_array_t create_dict(device_t);
static void add_prop(prop_array_t, const char *, const char *, u_int, int);
static void envctrl_props(prop_array_t, int);
static void envctrltwo_props(prop_array_t, int);

int
pcfiic_ebus_match(device_t parent, struct cfdata *match, void *aux)
{
	struct ebus_attach_args		*ea = aux;
	char				compat[32];

	if (strcmp(ea->ea_name, "SUNW,envctrl") == 0 ||
	    strcmp(ea->ea_name, "SUNW,envctrltwo") == 0)
		return (1);

	if (strcmp(ea->ea_name, "i2c") != 0)
		return (0);

	if (OF_getprop(ea->ea_node, "compatible", compat, sizeof(compat)) == -1)
		return (0);

	if (strcmp(compat, "pcf8584") == 0 ||
	    strcmp(compat, "i2cpcf,8584") == 0 ||
	    strcmp(compat, "SUNW,i2c-pic16f747") == 0 ||
	    strcmp(compat, "SUNW,bbc-i2c") == 0)
		return (1);

	return (0);
}

void
pcfiic_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct pcfiic_ebus_softc	*esc = device_private(self);
	struct pcfiic_softc		*sc = &esc->esc_sc;
	struct ebus_attach_args		*ea = aux;
	char				compat[32];
	u_int64_t			addr;
	u_int8_t			clock = PCF_CLOCK_12 | PCF_FREQ_90;
	int				swapregs = 0;

	if (ea->ea_nreg < 1 || ea->ea_nreg > 2) {
		printf(": expected 1 or 2 registers, got %d\n", ea->ea_nreg);
		return;
	}

	sc->sc_dev = self;
	if (OF_getprop(ea->ea_node, "compatible", compat, sizeof(compat)) > 0 &&
	    strcmp(compat, "SUNW,bbc-i2c") == 0) {
		/*
		 * On BBC-based machines, Sun swapped the order of
		 * the registers on their clone pcf, plus they feed
		 * it a non-standard clock.
		 */
		int clk = prom_getpropint(findroot(), "clock-frequency", 0);

		if (clk < 105000000)
			clock = PCF_CLOCK_3 | PCF_FREQ_90;
		else if (clk < 160000000)
			clock = PCF_CLOCK_4_43 | PCF_FREQ_90;
		swapregs = 1;
	}

	if (OF_getprop(ea->ea_node, "own-address", &addr, sizeof(addr)) == -1) {
		addr = 0xaa;
	} else if (addr == 0x00 || addr > 0xff) {
		printf(": invalid address on I2C bus");
		return;
	}

	if (bus_space_map(ea->ea_bustag,
	    EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
	    ea->ea_reg[0].size, 0, &sc->sc_ioh) == 0) {
		sc->sc_iot = ea->ea_bustag;
	} else {
		printf(": can't map register space\n");
               	return;
	}

	if (ea->ea_nreg == 2) {
		/*
		 * Second register only occurs on BBC-based machines,
		 * and is likely not prom mapped
		*/
		if (bus_space_map(sc->sc_iot, EBUS_ADDR_FROM_REG(&ea->ea_reg[1]),
		    ea->ea_reg[1].size, 0, &sc->sc_ioh2) != 0) {
			printf(": can't map 2nd register space\n");
			return;
		}
		sc->sc_master = 1;
		printf(": iic mux present");
	}

	if (ea->ea_nintr >= 1)
		esc->esc_ih = bus_intr_establish(sc->sc_iot, ea->ea_intr[0],
		    IPL_BIO, pcfiic_intr, sc);
	else
		esc->esc_ih = NULL;


	if (esc->esc_ih == NULL)
		sc->sc_poll = 1;

	if (strcmp(ea->ea_name, "SUNW,envctrl") == 0) {
		envctrl_props(create_dict(self), ea->ea_node);
		pcfiic_attach(sc, 0x55, PCF_CLOCK_12 | PCF_FREQ_45, 0);
	} else if (strcmp(ea->ea_name, "SUNW,envctrltwo") == 0) {
		envctrltwo_props(create_dict(self), ea->ea_node);
		pcfiic_attach(sc, 0x55, PCF_CLOCK_12 | PCF_FREQ_45, 0);
	} else
		pcfiic_attach(sc, (i2c_addr_t)(addr >> 1), clock, swapregs);
}

static prop_array_t
create_dict(device_t parent)
{
	prop_dictionary_t props = device_properties(parent);
	prop_array_t cfg = prop_dictionary_get(props, "i2c-child-devices");
	if (cfg) return cfg;
	cfg = prop_array_create();
	prop_dictionary_set(props, "i2c-child-devices", cfg);
	prop_object_release(cfg);
	return cfg;
}

static void
add_prop(prop_array_t c, const char *name, const char *compat, u_int addr,
	int node)
{
	prop_dictionary_t dev;
	prop_data_t data;

	dev = prop_dictionary_create();
	prop_dictionary_set_cstring(dev, "name", name);
	data = prop_data_create_data(compat, strlen(compat)+1);
	prop_dictionary_set(dev, "compatible", data);
	prop_object_release(data);
	prop_dictionary_set_uint32(dev, "addr", addr);
	prop_dictionary_set_uint64(dev, "cookie", node);
	prop_array_add(c, dev);
	prop_object_release(dev);
}

static void
envctrl_props(prop_array_t c, int node)
{
	/* Power supply 1 temperature. */
	add_prop(c, "PSU-1", "ecadc", 0x48, node);

	/* Power supply 2 termperature. */
	add_prop(c, "PSU-2", "ecadc", 0x49, node);

	/* Power supply 3 tempterature. */
	add_prop(c, "PSU-3", "ecadc", 0x4a, node);

	/* Ambient tempterature. */
	add_prop(c, "ambient", "i2c-lm75", 0x4d, node);

	/* CPU temperatures. */
	add_prop(c, "CPU", "ecadc", 0x4f, node);
}

static void
envctrltwo_props(prop_array_t c, int node)
{
	add_prop(c, "PSU", "ecadc", 0x4a, node);
	add_prop(c, "CPU", "ecadc", 0x4f, node);
}
