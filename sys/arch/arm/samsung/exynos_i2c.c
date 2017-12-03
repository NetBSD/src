/*	$NetBSD: exynos_i2c.c,v 1.1.6.3 2017/12/03 11:35:56 jdolecek Exp $ */

/*
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 */

#include "opt_exynos.h"
#include "opt_arm_debug.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_i2c.c,v 1.1.6.3 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/exynos_intr.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <dev/fdt/fdtvar.h>

struct exynos_i2c_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void *			sc_ih;
	struct clk *		sc_clk;

	struct fdtbus_pinctrl_pin  *sc_sda;
	struct fdtbus_pinctrl_pin  *sc_scl;
	bool			sc_sda_is_output;

	struct i2c_controller 	sc_ic;
	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;
	device_t		sc_i2cdev;
};

static int	exynos_i2c_intr(void *);

static int	exynos_i2c_acquire_bus(void *, int);
static void	exynos_i2c_release_bus(void *, int);

static int	exynos_i2c_send_start(void *, int);
static int	exynos_i2c_send_stop(void *, int);
static int	exynos_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int	exynos_i2c_read_byte(void *, uint8_t *, int);
static int	exynos_i2c_write_byte(void *, uint8_t , int);

static int	exynos_i2c_wait(struct exynos_i2c_softc *, int);


static int exynos_i2c_match(device_t, cfdata_t, void *);
static void exynos_i2c_attach(device_t, device_t, void *);

static i2c_tag_t exynos_i2c_get_tag(device_t);

struct fdtbus_i2c_controller_func exynos_i2c_funcs = {
	.get_tag = exynos_i2c_get_tag
};

CFATTACH_DECL_NEW(exynos_i2c, sizeof(struct exynos_i2c_softc),
    exynos_i2c_match, exynos_i2c_attach, NULL, NULL);

#define I2C_WRITE(sc, reg, val) \
    bus_space_write_1((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define I2C_READ(sc, reg) \
    bus_space_read_1((sc)->sc_bst, (sc)->sc_bsh, (reg))

#define IICCON  0x00
#define IICSTAT 0x04
#define IICADD  0x08
#define IICDS   0x0C

#define ACKENABLE  (1<<7)
#define TXPRESCALE (1<<6)
#define INTENABLE  (1<<5)
#define IRQPEND    (1<<4)
#define PRESCALE   (0x0f)

#define MODESELECT  (3<<6)
#define BUSYSTART   (1<<5)
#define BUSENABLE   (1<<4)
#define ARBITRATION (1<<3)
#define SLAVESTATUS (1<<2)
#define ZEROSTATUS  (1<<1)
#define LASTBIT     (1<<0)

#define READBIT     (1<<7)

static int
exynos_i2c_match(device_t self, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,s3c2440-i2c", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_i2c_attach(device_t parent, device_t self, void *aux)
{
        struct exynos_i2c_softc * const sc =  device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct i2cbus_attach_args iba;
	prop_dictionary_t devs;
	uint32_t address_cells;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev  = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr,
			     error);
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, device_xname(self));
	aprint_normal(" @ 0x%08x\n", (uint)addr);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, exynos_i2c_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = exynos_i2c_acquire_bus;
	sc->sc_ic.ic_release_bus = exynos_i2c_release_bus;
	sc->sc_ic.ic_send_start  = exynos_i2c_send_start;
	sc->sc_ic.ic_send_stop   = exynos_i2c_send_stop;
	sc->sc_ic.ic_initiate_xfer = exynos_i2c_initiate_xfer;
	sc->sc_ic.ic_read_byte   = exynos_i2c_read_byte;
	sc->sc_ic.ic_write_byte  = exynos_i2c_write_byte;

	fdtbus_register_i2c_controller(self, phandle, &exynos_i2c_funcs);

	devs = prop_dictionary_create();
	if (of_getprop_uint32(phandle, "#address-cells", &address_cells))
		address_cells = 1;
	of_enter_i2c_devs(devs, phandle, address_cells * 4, 0);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_ic;
	iba.iba_child_devices = prop_dictionary_get(devs, "i2c-child-devices");
	if (iba.iba_child_devices != NULL)
		prop_object_retain(iba.iba_child_devices);
	else
		iba.iba_child_devices = prop_array_create();
	prop_object_release(devs);

	sc->sc_i2cdev = config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static i2c_tag_t
exynos_i2c_get_tag(device_t dev)
{
	struct exynos_i2c_softc * const sc = device_private(dev);

	return &sc->sc_ic;
}

static int
exynos_i2c_intr(void *priv)
{
	struct exynos_i2c_softc * const sc = priv;

	uint8_t istatus = I2C_READ(sc, IICCON);
	if (!(istatus & IRQPEND))
		return 0;
	istatus &= ~IRQPEND;
	I2C_WRITE(sc, IICCON, istatus);

	mutex_enter(&sc->sc_lock);
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
exynos_i2c_acquire_bus(void *cookie, int flags)
{
	struct exynos_i2c_softc *i2c_sc = cookie;

	mutex_enter(&i2c_sc->sc_lock);
	return 0;
}

static void
exynos_i2c_release_bus(void *cookie, int flags)
{
	struct exynos_i2c_softc *i2c_sc = cookie;

	mutex_exit(&i2c_sc->sc_lock);
}

static int
exynos_i2c_wait(struct exynos_i2c_softc *sc, int flags)
{
	int error, retry;
	uint8_t stat = 0;

	retry = (flags & I2C_F_POLL) ? 100000 : 100;

	while (--retry > 0) {
		if ((flags & I2C_F_POLL) == 0) {
			error = cv_timedwait_sig(&sc->sc_cv, &sc->sc_lock,
			    max(mstohz(10), 1));
			if (error) {
				return error;
			}
		}
		stat = I2C_READ(sc, IICSTAT);
		if (!(stat & BUSYSTART)) {
			break;
		}
		if (flags & I2C_F_POLL) {
			delay(10);
		}
	}
	if (retry == 0) {
		stat = I2C_READ(sc, IICSTAT);
		device_printf(sc->sc_dev, "timed out, status = %#x\n", stat);
		return ETIMEDOUT;
	}

	return 0;
}


static int
exynos_i2c_send_start(void *cookie, int flags)
{
	struct exynos_i2c_softc *sc = cookie;
	I2C_WRITE(sc, IICSTAT, 0xF0);
	return 0;
}

static int
exynos_i2c_send_stop(void *cookie, int flags)
{
	struct exynos_i2c_softc *sc = cookie;
	I2C_WRITE(sc, IICSTAT, 0xD0);
	return 0;
}

static int
exynos_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	struct exynos_i2c_softc *sc = cookie;
	uint8_t byte = addr & 0x7f;
	if (flags & I2C_F_READ)
		byte |= READBIT;
	else
		byte &= ~READBIT;
	I2C_WRITE(sc, IICADD, addr);
	exynos_i2c_send_start(cookie, flags);
	exynos_i2c_write_byte(cookie, byte, flags);
	return exynos_i2c_wait(cookie, flags);
}

static int
exynos_i2c_read_byte(void *cookie, uint8_t *bytep, int flags)
{
	struct exynos_i2c_softc *sc = cookie;
	int error = exynos_i2c_wait(sc, flags);
	if (error)
		return error;
	*bytep = I2C_READ(sc, IICDS) & 0xff;
	if (flags & I2C_F_STOP)
		exynos_i2c_send_stop(cookie, flags);
	return 0;
}

static int
exynos_i2c_write_byte(void *cookie, uint8_t byte, int flags)
{
	struct exynos_i2c_softc *sc = cookie;
	int error = exynos_i2c_wait(sc, flags);
	if (error)
		return error;
	I2C_WRITE(sc, IICDS, byte);
	return 0;
}
