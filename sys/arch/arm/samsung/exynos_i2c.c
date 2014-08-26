/*	$NetBSD: exynos_i2c.c,v 1.2 2014/08/26 11:45:49 reinoud Exp $ */

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
#include "exynos_iic.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_i2c.c,v 1.2 2014/08/26 11:45:49 reinoud Exp $");


#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_io.h>
#include <arm/samsung/exynos_intr.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>


struct exynos_iic_dev_softc {
	int			isc_bus;
	bool			isc_isgpio;

	bus_space_tag_t		isc_bst;
	bus_space_handle_t	isc_bsh;

	struct exynos_gpio_pinset  isc_pinset;
	struct exynos_gpio_pindata isc_sda;
	struct exynos_gpio_pindata isc_slc;
	bool			isc_sda_is_output;

	kmutex_t		isc_buslock;
	struct i2c_controller 	isc_i2cbus;
};


#if NEXYNOS_IIC > 0
static int	exynos_iic_acquire_bus(void *, int);
static void	exynos_iic_release_bus(void *, int);

static int	exynos_iic_send_start(void *, int);
static int	exynos_iic_send_stop(void *, int);
static int	exynos_iic_initiate_xfer(void *, i2c_addr_t, int);
static int	exynos_iic_read_byte(void *, uint8_t *, int);
static int	exynos_iic_write_byte(void *, uint8_t , int);

static bool exynos_iic_attach_i2cbus(struct exynos_iic_dev_softc *,
	struct i2c_controller *, struct exyo_locators const *);
#endif


struct i2c_controller *exynos_i2cbus[EXYNOS_MAX_IIC_BUSSES];
static int exynos_iic_match(device_t, cfdata_t, void *);
static void exynos_iic_attach(device_t, device_t, void *);

struct exynos_iic_softc {
	device_t		sc_dev;

	struct exynos_iic_dev_softc sc_idevs[EXYNOS_MAX_IIC_BUSSES];
	struct i2c_controller       sc_i2cbus[EXYNOS_MAX_IIC_BUSSES];
} exynos_iic_sc;


CFATTACH_DECL_NEW(exynos_iic, sizeof(struct exynos_iic_softc),
    exynos_iic_match, exynos_iic_attach, NULL, NULL);


static int
exynos_iic_match(device_t self, cfdata_t cf, void *aux)
{
#ifdef DIAGNOSTIC
	struct exyo_attach_args *exyoaa = aux;
	struct exyo_locators *loc = &exyoaa->exyo_loc;
#endif
	int i;

	/* no locators expected */
	KASSERT(loc->loc_offset == 0);
	KASSERT(loc->loc_size   == 0);
	KASSERT(loc->loc_port   == EXYOCF_PORT_DEFAULT);

	if (exynos_iic_sc.sc_dev != NULL)
		return 0;
	for (i = 0; i < EXYNOS_MAX_IIC_BUSSES; i++)
		exynos_i2cbus[i] = NULL;
#if NEXYNOS_IIC > 0
	return 1;
#else
	return 0;
#endif
}


static void
exynos_iic_attach(device_t parent, device_t self, void *aux)
{
#if NEXYNOS_IIC > 0
	prop_dictionary_t dict = device_properties(self);
        struct exynos_iic_softc * const sc =  device_private(self);
	struct exyo_attach_args * const exyoaa = aux;
        struct exynos_iic_dev_softc *ei2c_sc;
	struct i2c_controller *i2c_cntr;
	struct i2cbus_attach_args iba;
	struct exyo_locinfo const *locs;
	struct exyo_locators const *loc;
	bool enable;
	char scrap[strlen("iic??_enable")];
	int i;

	locs = NULL;
#ifdef EXYNOS4
	if (IS_EXYNOS4_P())
		locs = &exynos4_i2c_locinfo;
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P())
		locs = &exynos5_i2c_locinfo;
#endif
	KASSERT(locs);

	sc->sc_dev  = self;
	if (locs->nlocators == 0) {
		aprint_error(": no i2c busses defined\n");
		return;
	}

	aprint_normal("\n");

	for (i = 0; i < locs->nlocators; i++) {
		/* get subdriver type : either gpio or hw */
		snprintf(scrap, sizeof(scrap), "iic%d_enable", i);
		if (prop_dictionary_get_bool(dict, scrap, &enable)) {
			if (!enable)
				continue;
			/* found an iic device */
			ei2c_sc   = &sc->sc_idevs[i];
			i2c_cntr = &sc->sc_i2cbus[i];
			loc = &locs->locators[i];

			ei2c_sc->isc_bus = i;
			ei2c_sc->isc_isgpio = (loc->loc_flags > 0);
			mutex_init(&ei2c_sc->isc_buslock, MUTEX_DEFAULT, IPL_NONE);

			ei2c_sc->isc_bst  = exyoaa->exyo_core_bst;
			if (bus_space_subregion(ei2c_sc->isc_bst,
			    exyoaa->exyo_core_bsh,
			    loc->loc_offset, loc->loc_size,
			    &ei2c_sc->isc_bsh)) {
				aprint_error_dev(self,
				    ": failed to map registers for i2cbus%d\n",
				    i);
				continue;
			}

			if (!exynos_iic_attach_i2cbus(ei2c_sc, i2c_cntr, loc))
				continue;

			exynos_i2cbus[i] = i2c_cntr;
			iba.iba_tag = i2c_cntr;
 			(void) config_found_ia(sc->sc_dev, "i2cbus", &iba,
					iicbus_print);
		}
	}
#endif
}


#if NEXYNOS_IIC > 0
static bool
exynos_iic_attach_i2cbus(struct exynos_iic_dev_softc *ei2c_sc,
	struct i2c_controller *i2c_cntr, struct exyo_locators const *loc)
{
	struct exynos_gpio_pinset *pinset;

	/* reserve our pins */
	pinset = &ei2c_sc->isc_pinset;
	strcpy(pinset->pinset_group, loc->loc_gpio_bus);
	pinset->pinset_mask = __BIT(loc->loc_sda) | __BIT(loc->loc_slc);
	pinset->pinset_func = loc->loc_func;

	i2c_cntr->ic_cookie = ei2c_sc;
	i2c_cntr->ic_acquire_bus = exynos_iic_acquire_bus;
	i2c_cntr->ic_release_bus = exynos_iic_release_bus;
	i2c_cntr->ic_send_start  = exynos_iic_send_start;
	i2c_cntr->ic_send_stop   = exynos_iic_send_stop;
	i2c_cntr->ic_initiate_xfer = exynos_iic_initiate_xfer;
	i2c_cntr->ic_read_byte   = exynos_iic_read_byte;
	i2c_cntr->ic_write_byte  = exynos_iic_write_byte;
	
	exynos_gpio_pinset_acquire(pinset);
	if (ei2c_sc->isc_isgpio) {
		/* get sda and slc pins */
		exynos_gpio_pinset_to_pindata(pinset,
			loc->loc_sda, &ei2c_sc->isc_sda);
		exynos_gpio_pinset_to_pindata(pinset,
			loc->loc_slc, &ei2c_sc->isc_slc);
		ei2c_sc->isc_sda_is_output = false;
		exynos_gpio_pindata_ctl(&ei2c_sc->isc_sda, GPIO_PIN_INPUT);
		exynos_gpio_pindata_ctl(&ei2c_sc->isc_slc, GPIO_PIN_OUTPUT);
		return 1;
	} else {
		/* TBD: attach hardware driver */
		aprint_normal("i2cbus%d: would attach native i2c driver\n",
		    ei2c_sc->isc_bus);
		return 0;
	}
}


#define EXYNOS_IIC_BB_SDA	__BIT(1)
#define EXYNOS_IIC_BB_SCL	__BIT(2)
#define EXYNOS_IIC_BB_SDA_OUT	__BIT(3)
#define EXYNOS_IIC_BB_SDA_IN	0

static void
exynos_iic_bb_set_bits(void *cookie, uint32_t bits)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;
	int sda, slc;

	sda = (bits & EXYNOS_IIC_BB_SDA) ? true : false;
	slc = (bits & EXYNOS_IIC_BB_SCL) ? true : false;

	if (i2c_sc->isc_sda_is_output)
		exynos_gpio_pindata_write(&i2c_sc->isc_sda, sda);
	exynos_gpio_pindata_write(&i2c_sc->isc_slc, slc);
}

static uint32_t
exynos_iic_bb_read_bits(void *cookie)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;
	int sda, slc;

	sda = 0;
	if (!i2c_sc->isc_sda_is_output)
		sda = exynos_gpio_pindata_read(&i2c_sc->isc_sda);
	slc = exynos_gpio_pindata_read(&i2c_sc->isc_slc);

	return (sda ? EXYNOS_IIC_BB_SDA : 0) | (slc ? EXYNOS_IIC_BB_SCL : 0);
}


static void
exynos_iic_bb_set_dir(void *cookie, uint32_t bits)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;
	int flags;

	flags = GPIO_PIN_INPUT | GPIO_PIN_TRISTATE;
	i2c_sc->isc_sda_is_output = ((bits & EXYNOS_IIC_BB_SDA_OUT) != 0);
	if (i2c_sc->isc_sda_is_output) 
		flags = GPIO_PIN_OUTPUT | GPIO_PIN_TRISTATE;

	exynos_gpio_pindata_ctl(&i2c_sc->isc_sda, flags);
}


static const struct i2c_bitbang_ops exynos_iic_bbops = {
	exynos_iic_bb_set_bits,
	exynos_iic_bb_set_dir,
	exynos_iic_bb_read_bits,
	{
		EXYNOS_IIC_BB_SDA,
		EXYNOS_IIC_BB_SCL,
		EXYNOS_IIC_BB_SDA_OUT,
		EXYNOS_IIC_BB_SDA_IN,
	}
};


static int
exynos_iic_acquire_bus(void *cookie, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	/* XXX what to do in polling case? could another cpu help */
	if (flags & I2C_F_POLL)
		return 0;
	mutex_enter(&i2c_sc->isc_buslock);
	return 0;
}

static void
exynos_iic_release_bus(void *cookie, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	/* XXX what to do in polling case? could another cpu help */
	if (flags & I2C_F_POLL)
		return;
	mutex_exit(&i2c_sc->isc_buslock);
}

static int
exynos_iic_send_start(void *cookie, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	if (i2c_sc->isc_isgpio)
		return i2c_bitbang_send_start(cookie, flags, &exynos_iic_bbops);
	panic("%s: not implemented for non gpio case\n", __func__);
	return EINVAL;
}

static int	
exynos_iic_send_stop(void *cookie, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	if (i2c_sc->isc_isgpio)
		return i2c_bitbang_send_stop(cookie, flags, &exynos_iic_bbops);
	panic("%s: not implemented for non gpio case\n", __func__);
	return EINVAL;
}

static int	
exynos_iic_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	if (i2c_sc->isc_isgpio)
		return i2c_bitbang_initiate_xfer(cookie, addr, flags,
							&exynos_iic_bbops);
	panic("%s: not implemented for non gpio case\n", __func__);
	return EINVAL;
}

static int
exynos_iic_read_byte(void *cookie, uint8_t *bytep, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	if (i2c_sc->isc_isgpio)
		return i2c_bitbang_read_byte(cookie, bytep, flags,
							&exynos_iic_bbops);
	panic("%s: not implemented for non gpio case\n", __func__);
	return EINVAL;
}

static int	
exynos_iic_write_byte(void *cookie, uint8_t byte, int flags)
{
	struct exynos_iic_dev_softc *i2c_sc = cookie;

	if (i2c_sc->isc_isgpio)
		return i2c_bitbang_write_byte(cookie, byte, flags,
							&exynos_iic_bbops);
	panic("%s: not implemented for non gpio case\n", __func__);
	return EINVAL;
}

#endif /* NEXYNOS_IIC > 0 */

