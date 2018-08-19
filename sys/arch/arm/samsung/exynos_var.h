/*	$NetBSD: exynos_var.h,v 1.25 2018/08/19 07:27:33 skrll Exp $	*/

/*-
 * Copyright (c) 2013, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 */

#ifndef _ARM_SAMSUNG_EXYNOS_VAR_H_
#define _ARM_SAMSUNG_EXYNOS_VAR_H_

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#include <arm/samsung/exynos_reg.h>

extern uint32_t exynos_soc_id;
extern uint32_t exynos_pop_id;

#define EXYNOS_PRODUCT_FAMILY(soc)	__SHIFTOUT((soc), __BITS(24,31))
#define  EXYNOS4_PRODUCT_FAMILY		0xe4
#define  EXYNOS5_PRODUCT_FAMILY		0xe5
#define EXYNOS_PRODUCT_ID(soc)		__SHIFTOUT((soc), __BITS(12,31))
#define EXYNOS_PRODUCT_PACKAGE(soc)	__SHIFTOUT((soc), __BITS(8,11))
#define EXYNOS_PRODUCT_REV(soc)		__SHIFTOUT((soc), __BITS(4,7))
#define EXYNOS_PRODUCT_SUBREV(soc)	__SHIFTOUT((soc), __BITS(0,3))


#define IS_EXYNOS4410_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe4410)
#define IS_EXYNOS4412_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe4412)
#define IS_EXYNOS4412_R0_P() \
			((EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe4412) && \
			 (EXYNOS_PRODUCT_REV(exynos_soc_id) == 0))
#define IS_EXYNOS4X12_P()	((EXYNOS_PRODUCT_ID(exynos_soc_id) & 0xff0ff) \
			== 0xe4012)

#define IS_EXYNOS4_P()	(EXYNOS_PRODUCT_FAMILY(exynos_soc_id) == EXYNOS4_PRODUCT_FAMILY)

#define IS_EXYNOS5410_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe5410)
#define IS_EXYNOS5422_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe5422)
#define IS_EXYNOS5440_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe5440)

#define IS_EXYNOS5_P()	(EXYNOS_PRODUCT_FAMILY(exynos_soc_id) == EXYNOS5_PRODUCT_FAMILY)


struct exyo_locators {
	const char *loc_name;
	bus_size_t loc_offset;
	bus_size_t loc_size;
	int loc_port;
	int loc_intr;
	int loc_flags;

	/* for i2c: */
	const char *loc_gpio_bus;
	uint8_t loc_sda, loc_slc, loc_func;
};


struct exyo_attach_args {
	struct exyo_locators exyo_loc;
	bus_space_tag_t exyo_core_bst;
	bus_space_tag_t exyo_core_a4x_bst;
	bus_space_handle_t exyo_core_bsh;
	bus_dma_tag_t exyo_dmat;
	bus_dma_tag_t exyo_coherent_dmat;
};

struct exynos_gpio_pinset {
	char pinset_bank[10];
	uint8_t pinset_func;
	uint8_t pinset_mask;
};

struct exynos_gpio_pindata {
	gpio_chipset_tag_t pd_gc;
	int pd_pin;
};

struct exynos_gpio_pin_cfg {
	uint32_t cfg;
	int cfg_valid;
	uint32_t pud;
	int pud_valid;
	uint32_t drv;
	int drv_valid;
	uint32_t conpwd;
	int conpwd_valid;
	uint32_t pudpwd;
	int pudpwd_valid;
};

struct exynos_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct exynos_gpio_bank *sc_bank;
	int			sc_phandle;
};

#define EXYNOS_MAX_IIC_BUSSES 9
struct i2c_controller;
extern struct i2c_controller *exynos_i2cbus[EXYNOS_MAX_IIC_BUSSES];


extern struct bus_space exynos_bs_tag;
extern struct bus_space exynos_a4x_bs_tag;
extern struct arm32_bus_dma_tag exynos_bus_dma_tag;
extern struct arm32_bus_dma_tag exynos_coherent_bus_dma_tag;

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern bus_space_handle_t exynos_core_bsh;
extern bus_space_handle_t exynos_wdt_bsh;
extern bus_space_handle_t exynos_pmu_bsh;
extern bus_space_handle_t exynos_cmu_bsh;
extern bus_space_handle_t exynos_sysreg_bsh;

extern void exynos_bootstrap(vaddr_t);
extern void exynos_dma_bootstrap(psize_t memsize);

struct exynos_pinctrl_softc;
struct exynos_gpio_softc;
struct fdt_attach_args;

extern struct exynos_gpio_softc * exynos_gpio_bank_config(struct exynos_pinctrl_softc *,
				    const struct fdt_attach_args *, int);
extern void exynos_wdt_reset(void);

extern void exynos_init_clkout_for_usb(void);	// board specific

extern void exynos_clocks_bootstrap(void);
extern void exynos_sysctl_cpufreq_init(void);
extern uint64_t exynos_get_cpufreq(void);

extern void exynos_device_register(device_t self, void *aux);
extern void exynos_device_register_post_config(device_t self, void *aux);
extern void exynos_usb_phy_init(bus_space_handle_t usb2phy_bsh);
extern void exynos_usb_soc_powerup(void);

extern void exyo_device_register(device_t self, void *aux);
extern void exyo_device_register_post_config(device_t self, void *aux);

extern struct exynos_gpio_bank *exynos_gpio_bank_lookup(const char *name);
extern bool exynos_gpio_pinset_available(const struct exynos_gpio_pinset *);
extern void exynos_gpio_pinset_acquire(const struct exynos_gpio_pinset *);
extern void exynos_gpio_pinset_release(const struct exynos_gpio_pinset *);
extern void exynos_gpio_pinset_to_pindata(const struct exynos_gpio_pinset *,
	int pinnr, struct exynos_gpio_pindata *);
extern bool exynos_gpio_pin_reserve(const char *, struct exynos_gpio_pindata *);
extern void exynos_gpio_pin_ctl_write(const struct exynos_gpio_bank *,
				      const struct exynos_gpio_pin_cfg *,
				      int);
static inline void
exynos_gpio_pindata_write(const struct exynos_gpio_pindata *pd, int value)
{
        gpiobus_pin_write(pd->pd_gc, pd->pd_pin, value);
}

static inline int
exynos_gpio_pindata_read(const struct exynos_gpio_pindata *pd)
{
        return gpiobus_pin_read(pd->pd_gc, pd->pd_pin);
}

static inline void
exynos_gpio_pindata_ctl(const struct exynos_gpio_pindata *pd, int flags)
{
        gpiobus_pin_ctl(pd->pd_gc, pd->pd_pin, flags);
}


#ifdef ARM_TRUSTZONE_FIRMWARE
/* trustzone calls */
extern int exynos_do_idle(void);
extern int exynos_set_cpu_boot_addr(int cpu, vaddr_t boot_addr);
extern int exynos_cpu_boot(int cpu);
#ifdef EXYNOS4
extern int exynos4_l2cc_init(void);
#endif
#endif

#endif	/* _ARM_SAMSUNG_EXYNOS_VAR_H_ */
