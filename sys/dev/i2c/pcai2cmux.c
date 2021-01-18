/*	$NetBSD: pcai2cmux.c,v 1.3 2021/01/18 15:28:21 thorpej Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: pcai2cmux.c,v 1.3 2021/01/18 15:28:21 thorpej Exp $");

/*
 * Driver for NXP PCA954x / PCA984x I2C switches and multiplexers.
 *
 * There are two flavors of this device:
 *
 * - Multiplexers, which connect the upstream bus to one downstream bus
 *   at a time.
 *
 * - Switches, which can connect the upstream bus to one or more downstream
 *   busses at a time (which is useful when using an all-call address for
 *   a large array of PCA9685 LED controllers, for example).
 *
 * Alas, the device tree bindings don't have anything specifically for
 * switches, so we treat the switch variants as basic multiplexers,
 * only enabling one downstream bus at a time.
 *
 * Note that some versions of these chips also have interrupt mux
 * capability.  XXX We do not support this yet.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/i2c/i2cmuxvar.h>

/* There are a maximum of 8 busses supported. */
#define	PCAIICMUX_MAX_BUSSES	8

struct pcaiicmux_type {
	unsigned int	nchannels;	/* # of downstream channels */
	uint8_t		enable_bit;	/* if 0, chip is switch type */
};

static const struct pcaiicmux_type mux2_type = {
	.nchannels = 2,
	.enable_bit = __BIT(2),
};

static const struct pcaiicmux_type switch2_type = {
	.nchannels = 2,
	.enable_bit = 0,
};

static const struct pcaiicmux_type mux4_type = {
	.nchannels = 4,
	.enable_bit = __BIT(2),
};

static const struct pcaiicmux_type switch4_type = {
	.nchannels = 4,
	.enable_bit = 0,
};

static const struct pcaiicmux_type mux8_type = {
	.nchannels = 8,
	.enable_bit = __BIT(3),
};

static const struct pcaiicmux_type switch8_type = {
	.nchannels = 8,
	.enable_bit = 0,
};

static const struct device_compatible_entry compat_data[] = {
	/* PCA9540 - 2 channel i2c mux */
	{ .compat = "nxp,pca9540",
	  .data = &mux2_type },

	/* PCA9542 - 2 channel i2c mux with interrupts */
	{ .compat = "nxp,pca9542",
	  .data = &mux2_type },

	/* PCA9543 - 2 channel i2c switch with interrupts */
	{ .compat = "nxp,pca9543",
	  .data = &switch2_type },

	/* PCA9544 - 4 channel i2c mux with interrupts */
	{ .compat = "nxp,pca9544",
	  .data = &mux4_type },

	/* PCA9545 - 4 channel i2c switch with interrupts */
	{ .compat = "nxp,pca9545",
	  .data = &switch4_type },

	/* PCA9546 - 4 channel i2c switch */
	{ .compat = "nxp,pca9546",
	  .data = &switch4_type },

	/* PCA9547 - 8 channel i2c mux */
	{ .compat = "nxp,pca9547",
	  .data = &mux8_type },

	/* PCA9548 - 8 channel i2c switch */
	{ .compat = "nxp,pca9548",
	  .data = &switch8_type },

	/* PCA9846 - 4 channel i2c switch */
	{ .compat = "nxp,pca9846",
	  .data = &switch4_type },

	/* PCA9847 - 8 channel i2c mux */
	{ .compat = "nxp,pca9847",
	  .data = &mux8_type },

	/* PCA9848 - 8 channel i2c switch */
	{ .compat = "nxp,pca9848",
	  .data = &switch8_type },

	/* PCA9849 - 4 channel i2c mux */
	{ .compat = "nxp,pca9849",
	  .data = &mux4_type },

	{ 0 }
};

struct pcaiicmux_softc {
	struct iicmux_softc	sc_iicmux;

	i2c_addr_t		sc_addr;
	int			sc_cur_value;

	const struct pcaiicmux_type *sc_type;
	struct fdtbus_gpio_pin *sc_reset_gpio;

	bool			sc_idle_disconnect;

	struct pcaiicmux_bus_info {
		uint8_t		enable_value;
	} sc_bus_info[PCAIICMUX_MAX_BUSSES];
};

static int
pcaiicmux_write(struct pcaiicmux_softc * const sc, uint8_t const val,
    int const flags)
{
	if ((int)val == sc->sc_cur_value) {
		return 0;
	}
	sc->sc_cur_value = (int)val;

	int const error =
	    iic_smbus_send_byte(sc->sc_iicmux.sc_i2c_parent, sc->sc_addr, val,
				flags & ~I2C_F_SPEED);
	if (error) {
		sc->sc_cur_value = -1;
	}

	return error;
}

/*****************************************************************************/

static void *
pcaiicmux_get_mux_info(struct iicmux_softc * const iicmux)
{
	return container_of(iicmux, struct pcaiicmux_softc, sc_iicmux);
}

static void *
pcaiicmux_get_bus_info(struct iicmux_bus * const bus)
{
	struct iicmux_softc * const iicmux = bus->mux;
	struct pcaiicmux_softc * const sc = iicmux->sc_mux_data;
	bus_addr_t addr;
	int error;

	if (bus->busidx >= sc->sc_type->nchannels) {
		aprint_error_dev(iicmux->sc_dev,
		    "device tree error: bus index %d out of range\n",
		    bus->busidx);
		return NULL;
	}

	struct pcaiicmux_bus_info * const bus_info =
	    &sc->sc_bus_info[bus->busidx];

	error = fdtbus_get_reg(bus->phandle, 0, &addr, NULL);
	if (error) {
		aprint_error_dev(iicmux->sc_dev,
		    "unable to get reg property for bus %d\n", bus->busidx);
		return NULL;
	}

	if (addr >= sc->sc_type->nchannels) {
		aprint_error_dev(iicmux->sc_dev,
		    "device tree error: reg property %llu out of range\n",
		    (unsigned long long)addr);
		return NULL;
	}

	/*
	 * If it's a mux type, the enable value is the channel number
	 * (from the reg property) OR'd with the enable bit.
	 *
	 * If it's a switch type, the enable value is 1 << channel number
	 * (from the reg property).
	 */
	if (sc->sc_type->enable_bit) {
		bus_info->enable_value =
		    (uint8_t)addr | sc->sc_type->enable_bit;
	} else {
		bus_info->enable_value = 1 << addr;
	}

	return bus_info;
}

static int
pcaiicmux_acquire_bus(struct iicmux_bus * const bus, int const flags)
{
	struct pcaiicmux_softc * const sc = bus->mux->sc_mux_data;
	struct pcaiicmux_bus_info * const bus_info = bus->bus_data;

	return pcaiicmux_write(sc, bus_info->enable_value, flags);
}

static void
pcaiicmux_release_bus(struct iicmux_bus * const bus, int const flags)
{
	struct pcaiicmux_softc * const sc = bus->mux->sc_mux_data;

	if (sc->sc_idle_disconnect) {
		(void) pcaiicmux_write(sc, 0, flags);
	}
}

static const struct iicmux_config pcaiicmux_config = {
	.desc = "PCA954x",
	.get_mux_info = pcaiicmux_get_mux_info,
	.get_bus_info = pcaiicmux_get_bus_info,
	.acquire_bus = pcaiicmux_acquire_bus,
	.release_bus = pcaiicmux_release_bus,
};

/*****************************************************************************/

static const struct pcaiicmux_type *
pcaiicmux_type_by_compat(const struct i2c_attach_args * const ia)
{
	const struct pcaiicmux_type *type = NULL;
	const struct device_compatible_entry *dce;

	if ((dce = iic_compatible_lookup(ia, compat_data)) != NULL)
		type = dce->data;

	return type;
}

static int
pcaiicmux_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args * const ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result)) {
		return match_result;
	}
	
	/* This device is direct-config only. */

	return 0;
}

static void
pcaiicmux_attach(device_t parent, device_t self, void *aux)
{
	struct pcaiicmux_softc * const sc = device_private(self);
	struct i2c_attach_args * const ia = aux;
	const int phandle = (int)ia->ia_cookie;
	int error;

	sc->sc_iicmux.sc_dev = self;
	sc->sc_iicmux.sc_phandle = phandle;
	sc->sc_iicmux.sc_config = &pcaiicmux_config;
	sc->sc_iicmux.sc_i2c_parent = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	sc->sc_type = pcaiicmux_type_by_compat(ia);
	KASSERT(sc->sc_type != NULL);

	aprint_naive("\n");
	aprint_normal(": PCA954x I2C %s\n",
	    sc->sc_type->enable_bit ? "mux" : "switch");

	if (of_hasprop(phandle, "i2c-mux-idle-disconnect")) {
		sc->sc_idle_disconnect = true;
	}

	/* Reset the mux if a reset GPIO is specified. */
	sc->sc_reset_gpio =
	    fdtbus_gpio_acquire(phandle, "reset-gpios", GPIO_PIN_OUTPUT);
	if (sc->sc_reset_gpio) {
		fdtbus_gpio_write(sc->sc_reset_gpio, 1);
		delay(10);
		fdtbus_gpio_write(sc->sc_reset_gpio, 0);
		delay(10);
	}

	/* Force the mux into a disconnected state. */
	sc->sc_cur_value = -1;
	error = iic_acquire_bus(ia->ia_tag, 0);
	if (error) {
		aprint_error_dev(self, "failed to acquire I2C bus\n");
		return;
	}
	error = pcaiicmux_write(sc, 0, 0);
	iic_release_bus(ia->ia_tag, 0);
	if (error) {
		aprint_error_dev(self,
		    "failed to set mux to disconnected state\n");
		return;
	}

	iicmux_attach(&sc->sc_iicmux);
}

CFATTACH_DECL_NEW(pcaiicmux, sizeof(struct pcaiicmux_softc),
    pcaiicmux_match, pcaiicmux_attach, NULL, NULL);
