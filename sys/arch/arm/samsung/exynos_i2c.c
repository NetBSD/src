/*	$NetBSD: exynos_i2c.c,v 1.2.2.2 2015/12/27 12:09:32 skrll Exp $ */

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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
__KERNEL_RCSID(0, "$NetBSD: exynos_i2c.c,v 1.2.2.2 2015/12/27 12:09:32 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <arm/samsung/exynos_reg.h>
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
	u_int			sc_port;

	struct fdtbus_gpio_pin  *sc_sda;
	struct fdtbus_gpio_pin  *sc_scl;
	bool			sc_sda_is_output;
	struct i2c_controller 	sc_ic;
	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;
	device_t		sc_i2cdev;
};

static u_int i2c_port;

static int	exynos_i2c_intr(void *);

static int	exynos_i2c_acquire_bus(void *, int);
static void	exynos_i2c_release_bus(void *, int);

static int	exynos_i2c_send_start(void *, int);
static int	exynos_i2c_send_stop(void *, int);
static int	exynos_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int	exynos_i2c_read_byte(void *, uint8_t *, int);
static int	exynos_i2c_write_byte(void *, uint8_t , int);

static bool exynos_i2c_attach_i2cbus(struct exynos_i2c_softc *,
				     struct i2c_controller *);

static int exynos_i2c_match(device_t, cfdata_t, void *);
static void exynos_i2c_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exynos_i2c, sizeof(struct exynos_i2c_softc),
    exynos_i2c_match, exynos_i2c_attach, NULL, NULL);

#define I2C_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define I2C_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

#define IICON 0
#define IRQPEND (1<<4)

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

	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	char result[64];
	int i2c_handle;
	int len;
	int handle;
	int func /*, pud, drv */;

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

	sc->sc_port = i2c_port++;
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

	len = OF_getprop(phandle, "pinctrl-0", (char *)&handle,
			 sizeof(handle));
	if (len != sizeof(int)) {
		aprint_error_dev(self, "couldn't get pinctrl-0.\n");
		return;
	}

	i2c_handle = fdtbus_get_phandle_from_native(be32toh(handle));
	len = OF_getprop(i2c_handle, "samsung,pins", result, sizeof(result));
	if (len <= 0) {
		aprint_error_dev(self, "couldn't get pins.\n");
		return;
	}
	
	len = OF_getprop(i2c_handle, "samsung,pin-function",
			 &handle, sizeof(handle));
	if (len <= 0) {
		aprint_error_dev(self, "couldn't get pin-function.\n");
		return;
	} else
		func = be32toh(handle);

	sc->sc_sda = fdtbus_gpio_acquire(phandle, &result[0], func);
	sc->sc_scl = fdtbus_gpio_acquire(phandle, &result[7], func);

	/* MJF: Need fdtbus_gpio_configure */
#if 0
	len = OF_getprop(i2c_handle, "samsung,pin-pud", &handle,
			 sizeof(&handle));
	if (len <= 0) {
		aprint_error_dev(self, "couldn't get pin-pud.\n");
		return;
	} else
		pud = be32toh(handle);

	len = OF_getprop(i2c_handle, "samsung,pin-drv", &handle,
			 sizeof(&handle));
	if (len <= 0) {
		aprint_error_dev(self, "couldn't get pin-drv.\n");
		return;
	} else
		drv = be32toh(handle);

#endif
	if (!exynos_i2c_attach_i2cbus(sc, &sc->sc_ic))
		return;

	sc->sc_i2cdev = config_found_ia(self, "i2cbus", &iba, iicbus_print);

}

static bool
exynos_i2c_attach_i2cbus(struct exynos_i2c_softc *i2c_sc,
			 struct i2c_controller *i2c_cntr)
{
	i2c_cntr->ic_cookie = i2c_sc;
	i2c_cntr->ic_acquire_bus = exynos_i2c_acquire_bus;
	i2c_cntr->ic_release_bus = exynos_i2c_release_bus;
	i2c_cntr->ic_send_start  = exynos_i2c_send_start;
	i2c_cntr->ic_send_stop   = exynos_i2c_send_stop;
	i2c_cntr->ic_initiate_xfer = exynos_i2c_initiate_xfer;
	i2c_cntr->ic_read_byte   = exynos_i2c_read_byte;
	i2c_cntr->ic_write_byte  = exynos_i2c_write_byte;

	return 1;
}

#define EXYNOS_I2C_BB_SDA	__BIT(1)
#define EXYNOS_I2C_BB_SCL	__BIT(2)
#define EXYNOS_I2C_BB_SDA_OUT	__BIT(3)
#define EXYNOS_I2C_BB_SDA_IN	0

static void
exynos_i2c_bb_set_bits(void *cookie, uint32_t bits)
{
	struct exynos_i2c_softc *i2c_sc = cookie;
	int sda, scl;

	sda = (bits & EXYNOS_I2C_BB_SDA) ? true : false;
	scl = (bits & EXYNOS_I2C_BB_SCL) ? true : false;

	if (i2c_sc->sc_sda_is_output)
		fdtbus_gpio_write(i2c_sc->sc_sda, sda);
	fdtbus_gpio_write(i2c_sc->sc_scl, scl);
}

static uint32_t
exynos_i2c_bb_read_bits(void *cookie)
{
	struct exynos_i2c_softc *i2c_sc = cookie;
	int sda, scl;

	sda = 0;
	if (!i2c_sc->sc_sda_is_output)
		sda = fdtbus_gpio_read(i2c_sc->sc_sda);
	scl = fdtbus_gpio_read(i2c_sc->sc_scl);

	return (sda ? EXYNOS_I2C_BB_SDA : 0) | (scl ? EXYNOS_I2C_BB_SCL : 0);
}

static void
exynos_i2c_bb_set_dir(void *cookie, uint32_t bits)
{
	struct exynos_i2c_softc *i2c_sc = cookie;
	int flags;

	flags = GPIO_PIN_INPUT | GPIO_PIN_TRISTATE;
	i2c_sc->sc_sda_is_output = ((bits & EXYNOS_I2C_BB_SDA_OUT) != 0);
	if (i2c_sc->sc_sda_is_output)
		flags = GPIO_PIN_OUTPUT | GPIO_PIN_TRISTATE;

	/* MJF: This is wrong but fdtbus has no ctrl operation */
	fdtbus_gpio_write(i2c_sc->sc_sda, flags);
}

static const struct i2c_bitbang_ops exynos_i2c_bbops = {
	exynos_i2c_bb_set_bits,
	exynos_i2c_bb_set_dir,
	exynos_i2c_bb_read_bits,
	{
		EXYNOS_I2C_BB_SDA,
		EXYNOS_I2C_BB_SCL,
		EXYNOS_I2C_BB_SDA_OUT,
		EXYNOS_I2C_BB_SDA_IN,
	}
};

static int
exynos_i2c_intr(void *priv)
{
	struct exynos_i2c_softc * const sc = priv;

	uint32_t istatus = I2C_READ(sc, IICON);
	if (!(istatus & IRQPEND))
		return 0;
	istatus &= ~IRQPEND;
	I2C_WRITE(sc, IICON, istatus);

	mutex_enter(&sc->sc_lock);
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
exynos_i2c_acquire_bus(void *cookie, int flags)
{
	struct exynos_i2c_softc *i2c_sc = cookie;

	/* XXX what to do in polling case? could another cpu help */
	if (flags & I2C_F_POLL)
		return 0;
	mutex_enter(&i2c_sc->sc_lock);
	return 0;
}

static void
exynos_i2c_release_bus(void *cookie, int flags)
{
	struct exynos_i2c_softc *i2c_sc = cookie;

	/* XXX what to do in polling case? could another cpu help */
	if (flags & I2C_F_POLL)
		return;
	mutex_exit(&i2c_sc->sc_lock);
}

static int
exynos_i2c_send_start(void *cookie, int flags)
{
	return i2c_bitbang_send_start(cookie, flags, &exynos_i2c_bbops);
}

static int
exynos_i2c_send_stop(void *cookie, int flags)
{
	return i2c_bitbang_send_stop(cookie, flags, &exynos_i2c_bbops);
}

static int
exynos_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return i2c_bitbang_initiate_xfer(cookie, addr, flags,
					 &exynos_i2c_bbops);
}

static int
exynos_i2c_read_byte(void *cookie, uint8_t *bytep, int flags)
{
	return i2c_bitbang_read_byte(cookie, bytep, flags,
				     &exynos_i2c_bbops);
}

static int
exynos_i2c_write_byte(void *cookie, uint8_t byte, int flags)
{
	return i2c_bitbang_write_byte(cookie, byte, flags,
				      &exynos_i2c_bbops);
}
