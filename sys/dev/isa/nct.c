/*	$NetBSD: nct.c,v 1.5 2021/08/07 16:19:12 thorpej Exp $	*/

/*-
 * Copyright (c) 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Nuvoton NCT5104D
 *
 * - GPIO: full support.
 * - Watchdog: no support.  Watchdog uses GPIO pins.
 * - UARTS: handled by com driver.  3rd & 4th UARTs use GPIO pins.
 *
 * If asked to probe with a wildcard address, we'll only do so if known to
 * be running on a PC Engines system.  Probe is invasive.
 *
 * Register access on Super I/O chips typically involves one or two levels
 * of indirection, so we try hard to avoid needless register access.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nct.c,v 1.5 2021/08/07 16:19:12 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <machine/autoconf.h>

#include <dev/isa/isavar.h>

#include <dev/gpio/gpiovar.h>

/*
 * Hardware interface definition (enough for GPIO only).
 */

/* I/O basics */
#define	NCT_IOBASE_A		0x2e
#define	NCT_IOBASE_B		0x4e
#define	NCT_IOSIZE		2
#define	NCT_CHIP_ID_1		0x1061
#define	NCT_CHIP_ID_2		0xc452	/* PC Engines APU */
#define	NCT_NUM_PINS		17

/* Enable/disable keys */
#define NCT_KEY_UNLOCK		0x87
#define NCT_KEY_LOCK		0xaa

/* I/O ports */
#define	NCT_PORT_SELECT		0
#define	NCT_PORT_DATA		1

/* Global registers */
#define	GD_DEVSEL		0x0007	/* logical device select */
#define	GD_MULTIFUN		0x001c	/* multi function selection */
#define		GD_MULTIFUN_GPIO1	0x04	/* clr: gpio1 available */
#define		GD_MULTIFUN_GPIO0	0x08	/* clr: gpio0 available */
#define		GD_MULTIFUN_GPIO67	0x10	/* set: gpio67 available */
#define	GD_GLOBOPT		0x0027	/* global option */
#define 	GD_GLOBOPT_GPIO67	0x04	/* clr: gpio67 available */
#define	GD_ID_HIGH		0x0020	/* ID high byte */
#define	GD_ID_LOW		0x0021	/* ID low byte */

/* Logical device 7 */
#define	LD7_ENABLE		0x0730	/* GPIO function enable */
#define		LD7_ENABLE_GPIO0	0x01
#define		LD7_ENABLE_GPIO1	0x02
#define		LD7_ENABLE_GPIO67	0x40
#define	LD7_GPIO0_DIRECTION	0x07e0	/* clr for output, set for input */
#define	LD7_GPIO0_DATA		0x07e1	/* current status */
#define	LD7_GPIO0_INVERSION	0x07e2	/* set to invert i/o */
#define	LD7_GPIO0_STATUS	0x07e3	/* edge detect, reading clears */
#define	LD7_GPIO1_DIRECTION	0x07e4	/* clr for output, set for input */
#define	LD7_GPIO1_DATA		0x07e5	/* current status */
#define	LD7_GPIO1_INVERSION	0x07e6	/* set to invert i/o */
#define	LD7_GPIO1_STATUS	0x07e7	/* edge detect, reading clears */
#define	LD7_GPIO67_DIRECTION	0x07f8	/* clr for output, set for input */
#define	LD7_GPIO67_DATA		0x07f9	/* current status */
#define	LD7_GPIO67_INVERSION	0x07fa	/* set to invert i/o */
#define	LD7_GPIO67_STATUS	0x07fb	/* edge detect, reading clears */

/* Logical device 8 */
#define	LD8_DEVCFG		0x0830	/* WDT/GPIO device config */
#define	LD8_GPIO0_MULTIFUNC	0x08e0	/* clr: gpio, set: pin unusable */
#define	LD8_GPIO1_MULTIFUNC	0x08e1	/* clr: gpio, set: pin unusable */
#define	LD8_GPIO67_MULTIFUNC	0x08e7	/* clr: gpio, set: pin unusable */

/* Logical device 10 */
#define	LDA_UARTC_ENABLE	0x0a30	/* bit 0: UARTC active */

/* Logical device 11 */
#define	LDB_UARTD_ENABLE	0x0b30	/* bit 0: UARTD active */

/* Logical device 15 */
#define	LDF_GPIO0_OUTMODE	0x0fe0	/* clr: push/pull, set: open drain */
#define	LDF_GPIO1_OUTMODE	0x0fe1	/* clr: push/pull, set: open drain */
#define	LDF_GPIO67_OUTMODE	0x0fe6	/* clr: push/pull, set: open drain */

/*
 * Internal GPIO bank description, including register addresses and cached
 * register content.
 */
struct nct_bank {
	/* Pin descriptions */
	u_int8_t	nb_firstpin;
	u_int8_t	nb_numpins;
	u_int8_t	nb_enabled;

	/* Cached values */
	u_int8_t	nb_val_dir;
	u_int8_t	nb_val_inv;
	u_int8_t	nb_val_mode;

	/* Register addresses */
	u_int16_t	nb_reg_dir;
	u_int16_t	nb_reg_data;
	u_int16_t	nb_reg_inv;
	u_int16_t	nb_reg_stat;
	u_int16_t	nb_reg_mode;
};

/*
 * Driver instance.
 */
struct nct_softc {
	device_t		sc_dev;			/* MI device */
	bus_space_tag_t		sc_iot;			/* I/O tag */
	bus_space_handle_t	sc_ioh;			/* I/O handle */
	struct gpio_chipset_tag	 sc_gc;			/* GPIO tag */
	gpio_pin_t		sc_pins[NCT_NUM_PINS];	/* GPIO pin descr. */

	/* Access to the remaining members is covered by sc_lock. */
	kmutex_t		sc_lock;		/* Serialization */
	int			sc_curdev;		/* Cur. logical dev */
	int			sc_curreg;		/* Cur. register */
	struct nct_bank		sc_bank[3];		/* Bank descriptions */
};

static void	nct_attach(device_t, device_t, void *);
static int	nct_detach(device_t, int);
static void	nct_gpio_ctl(void *, int, int);
static int	nct_gpio_read(void *, int);
static void	nct_gpio_write(void *, int, int);
static int	nct_match(device_t, cfdata_t , void *);
static u_int8_t	nct_rd(struct nct_softc *, int);
static struct	nct_bank *nct_sel(struct nct_softc *, int, u_int8_t *);
static void	nct_wr(struct nct_softc *, int, u_int8_t);

static inline void
nct_outb(struct nct_softc *sc, int reg, u_int8_t data)
{

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, reg, data);
}

static inline u_int8_t
nct_inb(struct nct_softc *sc, int reg)
{

	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, reg);
}

CFATTACH_DECL_NEW(nct,
		  sizeof(struct nct_softc),
		  nct_match,
		  nct_attach,
		  nct_detach,
		  NULL);

MODULE(MODULE_CLASS_DRIVER, nct, "gpio");

/*
 * Module linkage.
 */
#ifdef _MODULE
#include "ioconf.c"
#endif

static int
nct_modcmd(modcmd_t cmd, void *priv)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_nct,
		    cfattach_ioconf_nct, cfdata_ioconf_nct);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_nct,
		    cfattach_ioconf_nct, cfdata_ioconf_nct);
#endif
		return error;
	default:
		return ENOTTY;
	}
}

/*
 * Probe for device.
 */
static int
nct_match(device_t parent, cfdata_t match, void *aux)
{
	int ioaddrs[2] = { 0x2e, 0x4e };
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;
	int nioaddr, i;
	u_int8_t low, high;
	u_int16_t id;
	const char *vendor;

	/*
	 * Allow override of I/O base address.  If no I/O base address is
	 * provided, proceed to probe if running on a PC Engines system.
	 */
	if (ia->ia_nio > 0 && ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT) {
		ioaddrs[0] = ia->ia_io[0].ir_addr;
		nioaddr = 1;
	} else {
		vendor = pmf_get_platform("system-vendor");
		if (vendor != NULL && strstr(vendor, "PC Engines") != NULL) {
			nioaddr = __arraycount(ioaddrs);
		} else {
			nioaddr = 0;
		}
	}

	/*
	 * Probe at the selected addresses, if any.
	 */
	for (i = 0; i < nioaddr; i++) {
		if (bus_space_map(ia->ia_iot, ioaddrs[i], NCT_IOSIZE, 0,
		    &ioh) != 0) {
		    continue;
		}
                /* Unlock chip */
		bus_space_write_1(ia->ia_iot, ioh, NCT_PORT_SELECT,
		    NCT_KEY_UNLOCK);
		bus_space_write_1(ia->ia_iot, ioh, NCT_PORT_SELECT,
		    NCT_KEY_UNLOCK);
		/* Read ID */
		bus_space_write_1(ia->ia_iot, ioh, NCT_PORT_SELECT, GD_ID_LOW);
		low = bus_space_read_1(ia->ia_iot, ioh, NCT_PORT_DATA);
		bus_space_write_1(ia->ia_iot, ioh, NCT_PORT_SELECT, GD_ID_HIGH);
		high = bus_space_read_1(ia->ia_iot, ioh, NCT_PORT_DATA);
		id = (u_int16_t)low | ((u_int16_t)high << 8);
		bus_space_unmap(ia->ia_iot, ioh, NCT_IOSIZE);
		if (id == NCT_CHIP_ID_1 || id == NCT_CHIP_ID_2) {
			ia->ia_nirq = 0;
			ia->ia_ndrq = 0;
			ia->ia_niomem = 0;
			ia->ia_nio = 1;
			ia->ia_io[0].ir_size = NCT_IOSIZE;
			ia->ia_io[0].ir_addr = ioaddrs[i];
			return 1;
		}
	}
	return 0;
}

/*
 * Attach device instance.
 */
static void
nct_attach(device_t parent, device_t self, void *aux)
{
	struct nct_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	struct gpiobus_attach_args gba;
	struct nct_bank *nb;
	u_int8_t multifun, enable;
	int i, j;

	/*
	 * Set up register space and basics of our state.
	 */
	if (bus_space_map(ia->ia_iot, ia->ia_io[0].ir_addr,
	    ia->ia_io[0].ir_size, 0, &sc->sc_ioh) != 0) {
		aprint_normal(": can't map i/o space\n");
		return;
	}
	aprint_normal(": Nuvoton NCT5104D GPIO\n");
	sc->sc_dev = self;
	sc->sc_iot = ia->ia_iot;
	sc->sc_curdev = -1;
	sc->sc_curreg = -1;

        /*
         * All pin access is funneled through a common, indirect register
         * interface.  The gpio framework doesn't serialize calls to our
         * access methods, so do it internally.  This is likely such a
         * common requirement that it should be factored out as is done for
         * audio devices, allowing the driver to specify the appropriate
         * locks.  Anyhow, acquire the lock immediately to pacify locking
         * assertions.
         */
        mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	mutex_spin_enter(&sc->sc_lock);

        /*
         * Disable watchdog timer and GPIO alternate I/O mapping.
         */
        nct_wr(sc, LD8_DEVCFG, 0);

	/*
	 * The BIOS doesn't set things up the way we want.  Pfft.
	 * Enable all GPIO0/GPIO1 pins.
	 */
	multifun = nct_rd(sc, GD_MULTIFUN);
	nct_wr(sc, GD_MULTIFUN,
	    multifun & ~(GD_MULTIFUN_GPIO0 | GD_MULTIFUN_GPIO1));
	nct_wr(sc, LDA_UARTC_ENABLE, 0);
	nct_wr(sc, LD8_GPIO0_MULTIFUNC, 0);
	nct_wr(sc, LDB_UARTD_ENABLE, 0);
	nct_wr(sc, LD8_GPIO1_MULTIFUNC, 0);
	multifun = nct_rd(sc, GD_MULTIFUN);
	enable = nct_rd(sc, LD7_ENABLE) | LD7_ENABLE_GPIO0 | LD7_ENABLE_GPIO1;

	nb = &sc->sc_bank[0];
	nb->nb_firstpin = 0;
	nb->nb_numpins = 8;
	nb->nb_enabled = 0xff;
	nb->nb_reg_dir = LD7_GPIO0_DIRECTION;
	nb->nb_reg_data = LD7_GPIO0_DATA;
	nb->nb_reg_inv = LD7_GPIO0_INVERSION;
	nb->nb_reg_stat = LD7_GPIO0_STATUS;
	nb->nb_reg_mode = LDF_GPIO0_OUTMODE;

	nb = &sc->sc_bank[1];
	nb->nb_firstpin = 8;
	nb->nb_numpins = 8;
	nb->nb_enabled = 0xff;
	nb->nb_reg_dir = LD7_GPIO1_DIRECTION;
	nb->nb_reg_data = LD7_GPIO1_DATA;
	nb->nb_reg_inv = LD7_GPIO1_INVERSION;
	nb->nb_reg_stat = LD7_GPIO1_STATUS;
	nb->nb_reg_mode = LDF_GPIO1_OUTMODE;

	nb = &sc->sc_bank[2];
	nb->nb_firstpin = 16;
	nb->nb_numpins = 1;
	nb->nb_reg_dir = LD7_GPIO67_DIRECTION;
	nb->nb_reg_data = LD7_GPIO67_DATA;
	nb->nb_reg_stat = LD7_GPIO67_STATUS;
	nb->nb_reg_mode = LDF_GPIO67_OUTMODE;
	if ((multifun & GD_MULTIFUN_GPIO67) != 0 &&
	    (nct_rd(sc, GD_GLOBOPT) & GD_GLOBOPT_GPIO67) == 0) {
		nct_wr(sc, LD8_GPIO67_MULTIFUNC, 0);
		nb->nb_enabled = 0x01;
		enable |= LD7_ENABLE_GPIO67;
	} else {
		sc->sc_bank[2].nb_enabled = 0;
	}

	/*
	 * Display enabled pins and enable GPIO devices accordingly.
	 */
	nct_wr(sc, LD7_ENABLE, enable);
	mutex_spin_exit(&sc->sc_lock);

	aprint_normal_dev(self,
	    "enabled pins: GPIO0(%02x) GPIO1(%02x) GPIO67(%01x)\n",
	    (unsigned)sc->sc_bank[0].nb_enabled,
	    (unsigned)sc->sc_bank[1].nb_enabled,
	    (unsigned)sc->sc_bank[2].nb_enabled);

	/*
	 * Fill pin descriptions and initialize registers.
	 */
	memset(sc->sc_pins, 0, sizeof(sc->sc_pins));
	for (i = 0; i < __arraycount(sc->sc_bank); i++) {
		nb = &sc->sc_bank[i];
		mutex_spin_enter(&sc->sc_lock);
		nb->nb_val_dir = nct_rd(sc, nb->nb_reg_dir);
		nb->nb_val_inv = nct_rd(sc, nb->nb_reg_inv);
		nb->nb_val_mode = nct_rd(sc, nb->nb_reg_mode);
		mutex_spin_exit(&sc->sc_lock);
		for (j = 0; j < nb->nb_numpins; j++) {
			gpio_pin_t *pin = &sc->sc_pins[nb->nb_firstpin + j];
			pin->pin_num = nb->nb_firstpin + j;
			/* Skip pin if not configured as GPIO. */
			if ((nb->nb_enabled & (1 << j)) == 0) {
				continue;
			}
			pin->pin_caps =
			    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
			    GPIO_PIN_OPENDRAIN |
			    GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
			    GPIO_PIN_INVIN | GPIO_PIN_INVOUT;
			pin->pin_flags =
			    GPIO_PIN_INPUT | GPIO_PIN_OPENDRAIN;
			nct_gpio_ctl(sc, pin->pin_num, pin->pin_flags);
			pin->pin_state = nct_gpio_read(sc, pin->pin_num);
		}
	}

	/*
	 * Attach to gpio framework, and attach all pins unconditionally. 
	 * If the pins are disabled, we'll ignore any access later.
	 */
	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = nct_gpio_read;
	sc->sc_gc.gp_pin_write = nct_gpio_write;
	sc->sc_gc.gp_pin_ctl = nct_gpio_ctl;

	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = NCT_NUM_PINS;

	(void)config_found(sc->sc_dev, &gba, gpiobus_print, CFARGS_NONE);
}

/*
 * Detach device instance.
 */
static int
nct_detach(device_t self, int flags)
{
	struct nct_softc *sc = device_private(self);

	bus_space_unmap(sc->sc_iot, sc->sc_ioh, NCT_IOSIZE);
	mutex_destroy(&sc->sc_lock);
	return 0;
}

/*
 * Read byte from specified register.
 */
static u_int8_t
nct_rd(struct nct_softc *sc, int reg)
{
	int dev;

	KASSERT(mutex_owned(&sc->sc_lock));
	
	dev = reg >> 8;
	reg &= 0xff;

	if (dev != sc->sc_curdev && dev != 0x00) {
		sc->sc_curdev = dev;
		sc->sc_curreg = reg;
		nct_outb(sc, NCT_PORT_SELECT, GD_DEVSEL);
		nct_outb(sc, NCT_PORT_DATA, dev);
		nct_outb(sc, NCT_PORT_SELECT, reg);
		return nct_inb(sc, NCT_PORT_DATA);
	} else if (reg != sc->sc_curreg) {
		sc->sc_curreg = reg;
		nct_outb(sc, NCT_PORT_SELECT, reg);
		return nct_inb(sc, NCT_PORT_DATA);
	} else {
		return nct_inb(sc, NCT_PORT_DATA);
	}
}

/*
 * Write byte to specified register.
 */
static void
nct_wr(struct nct_softc *sc, int reg, u_int8_t data)
{
	int dev;

	KASSERT(mutex_owned(&sc->sc_lock));
	
	dev = reg >> 8;
	reg &= 0xff;

	if (dev != sc->sc_curdev && dev != 0x00) {
		sc->sc_curdev = dev;
		sc->sc_curreg = reg;
		nct_outb(sc, NCT_PORT_SELECT, GD_DEVSEL);
		nct_outb(sc, NCT_PORT_DATA, dev);
		nct_outb(sc, NCT_PORT_SELECT, reg);
		nct_outb(sc, NCT_PORT_DATA, data);
	} else if (reg != sc->sc_curreg) {
		sc->sc_curreg = reg;
		nct_outb(sc, NCT_PORT_SELECT, reg);
		nct_outb(sc, NCT_PORT_DATA, data);
	} else {
		nct_outb(sc, NCT_PORT_DATA, data);
	}
}

/*
 * Given pin number, return bank and pin mask.  This alters no state and so
 * can safely be called without the mutex held.
 */
static struct nct_bank *
nct_sel(struct nct_softc *sc, int pin, u_int8_t *mask)
{
	struct nct_bank *nb;

	KASSERT(pin >= 0 && pin < NCT_NUM_PINS);
	nb = &sc->sc_bank[pin >> 3];
	KASSERT(pin >= nb->nb_firstpin);
	KASSERT((pin & 7) < nb->nb_numpins);
	*mask = (u_int8_t)(1 << (pin & 7)) & nb->nb_enabled;
	return nb;
}

/*
 * GPIO hook: read pin.
 */
static int
nct_gpio_read(void *arg, int pin)
{
	struct nct_softc *sc = arg;
	struct nct_bank *nb;
	u_int8_t data, mask;
	int rv = GPIO_PIN_LOW;

	nb = nct_sel(sc, pin, &mask);
	if (__predict_true(mask != 0)) {
		mutex_spin_enter(&sc->sc_lock);
		data = nct_rd(sc, nb->nb_reg_data);
		if ((data & mask) != 0) {
			rv = GPIO_PIN_HIGH;
		}
		mutex_spin_exit(&sc->sc_lock);
	}
	return rv;
}

/*
 * GPIO hook: write pin.
 */
static void
nct_gpio_write(void *arg, int pin, int val)
{
	struct nct_softc *sc = arg;
	struct nct_bank *nb;
	u_int8_t data, mask;

	nb = nct_sel(sc, pin, &mask);
	if (__predict_true(mask != 0)) {
		mutex_spin_enter(&sc->sc_lock);
		data = nct_rd(sc, nb->nb_reg_data);
		if (val == GPIO_PIN_LOW) {
			data &= ~mask;
		} else if (val == GPIO_PIN_HIGH) {
			data |= mask;
		}
		nct_wr(sc, nb->nb_reg_data, data);
		mutex_spin_exit(&sc->sc_lock);
	}
}

/*
 * GPIO hook: change pin parameters.
 */
static void
nct_gpio_ctl(void *arg, int pin, int flg)
{
	struct nct_softc *sc = arg;
	struct nct_bank *nb;
	u_int8_t data, mask;

	nb = nct_sel(sc, pin, &mask);
	if (__predict_false(mask == 0)) {
		return;
	}

	/*
	 * Set input direction early to avoid perturbation.
	 */
	mutex_spin_enter(&sc->sc_lock);
	data = nb->nb_val_dir;
	if ((flg & (GPIO_PIN_INPUT | GPIO_PIN_TRISTATE)) != 0) {
		data |= mask;
	}
	if (data != nb->nb_val_dir) {
		nct_wr(sc, nb->nb_reg_dir, data);
		nb->nb_val_dir = data;
	}

	/*
	 * Set inversion.
	 */
	data = nb->nb_val_inv;
	if ((flg & (GPIO_PIN_OUTPUT | GPIO_PIN_INVOUT)) ==
	             (GPIO_PIN_OUTPUT | GPIO_PIN_INVOUT) ||
            (flg & (GPIO_PIN_INPUT | GPIO_PIN_INVIN)) ==
  	             (GPIO_PIN_INPUT | GPIO_PIN_INVIN)) {
		data |= mask;
	} else {
		data &= ~mask;
	}
	if (data != nb->nb_val_inv) {
		nct_wr(sc, nb->nb_reg_inv, data);
		nb->nb_val_inv = data;
	}

	/*
	 * Set drain mode.
	 */
	data = nb->nb_val_mode;
	if ((flg & GPIO_PIN_PUSHPULL) != 0) {
		data |= mask;
	} else /* GPIO_PIN_OPENDRAIN */ {
		data &= ~mask;
	}
	if (data != nb->nb_val_mode) {
		nct_wr(sc, nb->nb_reg_mode, data);
		nb->nb_val_mode = data;
	}

	/*
	 * Set output direction late to avoid perturbation.
	 */
	data = nb->nb_val_dir;
	if ((flg & (GPIO_PIN_OUTPUT | GPIO_PIN_TRISTATE)) == GPIO_PIN_OUTPUT) {
		data &= ~mask;
	}
	if (data != nb->nb_val_dir) {
		nct_wr(sc, nb->nb_reg_dir, data);
		nb->nb_val_dir = data;
	}
	mutex_spin_exit(&sc->sc_lock);
}
