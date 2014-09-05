/*	$NetBSD: exynos_gpio.c,v 1.9 2014/09/05 08:01:05 skrll Exp $	*/

/*-
* Copyright (c) 2014 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Reinoud Zandijk
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

#include "opt_exynos.h"
#include "opt_arm_debug.h"
#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_gpio.c,v 1.9 2014/09/05 08:01:05 skrll Exp $");

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

static int exynos_gpio_match(device_t, cfdata_t, void *);
static void exynos_gpio_attach(device_t, device_t, void *);

static int exynos_gpio_pin_read(void *, int);
static void exynos_gpio_pin_write(void *, int, int);
static void exynos_gpio_pin_ctl(void *, int, int);

struct exynos_gpio_pin_cfg {
	uint32_t cfg;
	uint32_t pud;
	uint32_t drv;
	uint32_t conpwd;
	uint32_t pudpwd;
};

struct exynos_gpio_pin_group {
	const char		grp_name[6];
	const bus_addr_t	grp_core_offset;
	const uint8_t		grp_bits;

	uint8_t			grp_pin_mask;
	uint8_t			grp_pin_inuse_mask;
	bus_space_handle_t	grp_bsh;
	struct exynos_gpio_pin_cfg grp_cfg;
	struct gpio_chipset_tag grp_gc_tag;
};


#define GPIO_REG(v,s,o) (EXYNOS##v##_GPIO_##s##_OFFSET + (o))
#define GPIO_GRP(v, s, o, n, b) \
	{ \
		.grp_name = #n, \
		.grp_core_offset = GPIO_REG(v,s,o), \
		.grp_bits = b,\
	}

#ifdef EXYNOS4
/*
 * Exynos 4412 contains 304 multi-functional input/output port pins and 164
 * memory port pins. There are 37 general port groups and two memory port
 * groups. They are:
 *
 *  GPA0, GPA1: 14 in/out ports-3xUART with flow control, UART without flow
 *  control, and/or 2xI2C
 *
 *  GPB: 8 in/out ports-2xSPI and/or 2xI2C and/ or IEM
 *
 *  GPC0, GPC1: 10 in/out ports-2xI2S, and/or 2xPCM, and/or AC97, SPDIF, I2C,
 *  and/or SPI
 *
 *  GPD0, GPD1: 8 in/out ports-PWM, 2xI2C, and/ or LCD I/F, MIPI
 *
 *  GPM0, GPM1, GPM2, GPM3, GPM4: 35 in/out ports-CAM I/F, and/ or TS I/F,
 *  HSI, and/ or Trace I/F
 *
 *  GPF0, GPF1, GPF2, GPF3: 30 in/out ports-LCD I/F
 *
 *  GPJ0, GPJ1: 13 in/out ports-CAM I/F
 *
 *  GPK0, GPK1, GPK2, GPK3: 28 in/out ports-4xMMC (4-bit MMC), and/or 2xMMC
 *  (8-bit MMC)), and/or GPS debugging I/F
 *
 *  GPL0, GPL1: 11 in/out ports-GPS I/F
 *
 *  GPL2: 8 in/out ports-GPS debugging I/F or Key pad I/F
 *
 *  GPX0, GPX1, GPX2, GPX3: 32 in/out ports-External wake-up, and/or Key pad
 *  I/F
 *
 *  GPZ: 7 in/out ports-low Power I2S and/or PCM
 *
 *  GPY0, GPY1, GPY2: 16 in/out ports-Control signals of EBI (SROM, NF, One
 *  NAND)
 *
 *  GPY3, GPY4, GPY5, GPY6: 32 in/out memory ports-EBI (For more information
 *  about EBI configuration, refer to Chapter 5, and 6)
 *
 *  MP1_0-MP1_9: 78 DRAM1 ports. NOTE: GPIO registers does not control these
 *  ports.
 *
 *  MP2_0-MP2_9: 78 DRAM2 ports. NOTE: GPIO registers does not control these
 *  ports.
 *
 *  ETC0, ETC1, ETC6: 18 in/out ETC ports-JTAG, SLIMBUS, RESET, CLOCK
 *
 *  ETC7, ETC8 : 4 clock port for C2C
 *
 */

static struct exynos_gpio_pin_group exynos4_pin_groups[] = {
	GPIO_GRP(4, LEFT,  0x0000, GPA0, 8),
	GPIO_GRP(4, LEFT,  0x0020, GPA1, 6),
	GPIO_GRP(4, LEFT,  0x0040, GPB,  8),
	GPIO_GRP(4, LEFT,  0x0060, GPC0, 5),
	GPIO_GRP(4, LEFT,  0x0080, GPC1, 5),
	GPIO_GRP(4, LEFT,  0x00A0, GPD0, 4),
	GPIO_GRP(4, LEFT,  0x00C0, GPD1, 4),
	GPIO_GRP(4, LEFT,  0x0180, GPF0, 8),
	GPIO_GRP(4, LEFT,  0x01A0, GPF1, 8),
	GPIO_GRP(4, LEFT,  0x01C0, GPF2, 8),
	GPIO_GRP(4, LEFT,  0x01E0, GPF3, 8),
	GPIO_GRP(4, LEFT,  0x0240, GPJ0, 8),
	GPIO_GRP(4, LEFT,  0x0260, GPJ1, 5),
	/* EXTINT skipped */

	GPIO_GRP(4, RIGHT, 0x0040, GPK0, 8),
	GPIO_GRP(4, RIGHT, 0x0060, GPK1, 8),
	GPIO_GRP(4, RIGHT, 0x0080, GPK2, 7),
	GPIO_GRP(4, RIGHT, 0x00A0, GPK3, 7),
	GPIO_GRP(4, RIGHT, 0x00C0, GPL0, 7),
	GPIO_GRP(4, RIGHT, 0x00E0, GPL1, 2),
	GPIO_GRP(4, RIGHT, 0x0100, GPL2, 8),
	GPIO_GRP(4, RIGHT, 0x0120, GPY0, 6),
	GPIO_GRP(4, RIGHT, 0x0140, GPY1, 4),
	GPIO_GRP(4, RIGHT, 0x0160, GPY2, 6),
	GPIO_GRP(4, RIGHT, 0x0180, GPY3, 8),
	GPIO_GRP(4, RIGHT, 0x01A0, GPY4, 8),
	GPIO_GRP(4, RIGHT, 0x01C0, GPY5, 8),
	GPIO_GRP(4, RIGHT, 0x01E0, GPY6, 8),
	GPIO_GRP(4, RIGHT, 0x0200, ETC0, 6),
	GPIO_GRP(4, RIGHT, 0x0220, ETC6, 7),
	GPIO_GRP(4, RIGHT, 0x0260, GPM0, 8),
	GPIO_GRP(4, RIGHT, 0x0280, GPM1, 7),
	GPIO_GRP(4, RIGHT, 0x02A0, GPM2, 5),
	GPIO_GRP(4, RIGHT, 0x02C0, GPM3, 8),
	GPIO_GRP(4, RIGHT, 0x02E0, GPM4, 8),
	/* EXTINT skipped */
	GPIO_GRP(4, RIGHT, 0x0C00, GPX0, 8),
	GPIO_GRP(4, RIGHT, 0x0C20, GPX1, 8),
	GPIO_GRP(4, RIGHT, 0x0C40, GPX2, 8),
	GPIO_GRP(4, RIGHT, 0x0C60, GPX3, 8),
	/* EXTINT skipped */

	GPIO_GRP(4, I2S0,  0x0000, GPZ,  8),
	/* EXTINT skipped */

	GPIO_GRP(4, C2C,   0x0000, GPV0, 8),
	GPIO_GRP(4, C2C,   0x0020, GPV1, 8),
	GPIO_GRP(4, C2C,   0x0040, ETC7, 2),
	GPIO_GRP(4, C2C,   0x0060, GPV2, 8),
	GPIO_GRP(4, C2C,   0x0080, GPV3, 8),
	GPIO_GRP(4, C2C,   0x00A0, ETC8, 2),
	GPIO_GRP(4, C2C,   0x00C0, GPV4, 2),
	/* EXTINT skipped */
};
#endif


#ifdef EXYNOS5

/*
 * Exynos 5250 contains 253 multi-functional input/output port pins and 160
 * memory port pins. There are 39 general port groups and 2 memory port
 * groups. They are:
 *
 * GPA0, GPA1: 14 in/out ports-2xUART with flow control, UART without flow
 * control, and/or 2xI2C , and/or2xHS-I2C
 *
 * GPA2: 8 in/out ports-2xSPI, and/or I2C
 *
 * GPB0, GPB1: 10 in/out ports-2xI2S, and/or 2xPCM, and/or AC97, SPDIF, I2C,
 * and/or SPI
 *
 * GPB2, GPB3: 8 in/out ports-PWM, I2C, and/or I2C ,and/or HS-I2C
 *
 * GPC0, GPC1: 11 in/out ports-1xMMC (8-bit MMC) I/F
 *
 * GPC2: 7 in/out ports-1xMMC (4-bit MMC) I/F
 *
 * GPC3, GPC4: 14 in/out ports-2xMMC (4-bit MMC) and/or 1xMMC (8-bit MMC) I/F
 *
 * GPD0: 4 pin/out ports-1xUART with flow control I/F
 *
 * GPD1: 8 pin/out ports-HSI I/F
 *
 * GPE0, GPE1, GPF0, GPF1, GPG0, GPG1, GPG2, GPH0, GPH1: 48 in/out ports-CAM
 * I/F, and/or Trace I/F
 *
 * GPV0, GPV1, GPV2, GPV3, GPV4: 34 in/out ports-C2C I/F
 *
 * GPX0, 1, 2, 3: 32 in/out port-external wake-up interrupts (up-to 32-bit),
 * and/or AUD I/F, and/or MFC I/F (GPX groups are in alive region)
 *
 * GPY0, GPY1, GPY2: 16 in/out ports-control signals of EBI (SROM)
 *
 * GPY3, GPY4, GPY5, GPY6: 32 in/out memory ports-EBI
 *
 * GPZ: 7 in/out ports-low power I2S and/or PCM
 *
 * MP1_0-MP1_10: 80 DRAM1 ports NOTE: GPIO registers do not control these
 * ports.
 *
 * MP2_0-MP2_10: 80 DRAM2 ports NOTE: GPIO registers do not control these
 * ports.
 * 
 * ETC0, ETC5, ETC6, ETC7, ETC8: 22 in/out ETC ports-JTAG, C2C_CLK (Rx),
 * RESET, CLOCK, USBOTG and USB3, C2C_CLK (Tx)
 */

static struct exynos_gpio_pin_group exynos5_pin_groups[] = {
	GPIO_GRP(5, LEFT,  0x0000, GPA0, 8),
	GPIO_GRP(5, LEFT,  0x0020, GPA1, 6),
	GPIO_GRP(5, LEFT,  0x0040, GPA2, 8),
	GPIO_GRP(5, LEFT,  0x0060, GPB0, 5),
	GPIO_GRP(5, LEFT,  0x0080, GPB1, 5),
	GPIO_GRP(5, LEFT,  0x00A0, GPB2, 4),
	GPIO_GRP(5, LEFT,  0x00C0, GPB3, 4),
	GPIO_GRP(5, LEFT,  0x00E0, GPC0, 7),
	GPIO_GRP(5, LEFT,  0x0100, GPC1, 4),
	GPIO_GRP(5, LEFT,  0x0120, GPC2, 7),
	GPIO_GRP(5, LEFT,  0x0140, GPC3, 7),
	GPIO_GRP(5, LEFT,  0x0160, GPD0, 4),
	GPIO_GRP(5, LEFT,  0x0180, GPD1, 8),
	GPIO_GRP(5, LEFT,  0x01A0, GPY0, 6),
	GPIO_GRP(5, LEFT,  0x01C0, GPY1, 4),
	GPIO_GRP(5, LEFT,  0x01E0, GPY2, 6),
	GPIO_GRP(5, LEFT,  0x0200, GPY3, 8),
	GPIO_GRP(5, LEFT,  0x0220, GPY4, 8),
	GPIO_GRP(5, LEFT,  0x0240, GPY5, 8),
	GPIO_GRP(5, LEFT,  0x0260, GPY6, 8),
	GPIO_GRP(5, LEFT,  0x0280, ETC0, 6),
	GPIO_GRP(5, LEFT,  0x02A0, ETC6, 7),
	GPIO_GRP(5, LEFT,  0x02C0, ETC7, 5),
	GPIO_GRP(5, LEFT,  0x02E0, GPC4, 7),
	/* EXTINT skipped */
	GPIO_GRP(5, LEFT,  0x0C00, GPX0, 8),
	GPIO_GRP(5, LEFT,  0x0C20, GPX1, 8),
	GPIO_GRP(5, LEFT,  0x0C40, GPX2, 8),
	GPIO_GRP(5, LEFT,  0x0C60, GPX3, 8),
	/* EXTINT skipped */

	GPIO_GRP(5, RIGHT, 0x0000, GPE0, 8),
	GPIO_GRP(5, RIGHT, 0x0020, GPE1, 2),
	GPIO_GRP(5, RIGHT, 0x0040, GPF0, 4),
	GPIO_GRP(5, RIGHT, 0x0060, GPF1, 4),
	GPIO_GRP(5, RIGHT, 0x0080, GPG0, 8),
	GPIO_GRP(5, RIGHT, 0x00A0, GPG1, 8),
	GPIO_GRP(5, RIGHT, 0x00C0, GPG2, 2),
	GPIO_GRP(5, RIGHT, 0x00E0, GPH0, 4),
	GPIO_GRP(5, RIGHT, 0x0100, GPH1, 8),
	/* EXTINT skipped */

	GPIO_GRP(5, C2C,   0x0000, GPV0, 8),
	GPIO_GRP(5, C2C,   0x0020, GPV1, 8),
	GPIO_GRP(5, C2C,   0x0040, ETC5, 2),
	GPIO_GRP(5, C2C,   0x0060, GPV2, 8),
	GPIO_GRP(5, C2C,   0x0080, GPV3, 8),
	GPIO_GRP(5, C2C,   0x00A0, ETC8, 2),
	GPIO_GRP(5, C2C,   0x00C0, GPV4, 2),
	/* EXTINT skipped */

	GPIO_GRP(5, I2S,   0x0000, GPZ,  7),
	/* EXTINT skipped */
};
#endif


struct exynos_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};


/* force these structures in DATA segment */
static struct exynos_gpio_pin_group *exynos_pin_groups = NULL;
static int exynos_n_pin_groups = 0;

static struct exynos_gpio_softc exynos_gpio_sc = {};


CFATTACH_DECL_NEW(exynos_gpio, sizeof(struct exynos_gpio_softc),
	exynos_gpio_match, exynos_gpio_attach, NULL, NULL);


static int
exynos_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
#ifdef DIAGNOSTIC
	struct exyo_attach_args * const exyoaa = aux;
	struct exyo_locators *loc = &exyoaa->exyo_loc;
#endif

	/* no locators expected */
	KASSERT(loc->loc_offset == 0);
	KASSERT(loc->loc_size   == 0);
	KASSERT(loc->loc_port   == EXYOCF_PORT_DEFAULT);

	/* there can only be one */
	if (exynos_gpio_sc.sc_dev != NULL)
		return 0;
	return 1;
}


#if NGPIO > 0
static void
exynos_gpio_config_pins(device_t self)
{
	struct exynos_gpio_softc * const sc = &exynos_gpio_sc;
	struct exynos_gpio_pin_group *grp;
	struct gpiobus_attach_args gba;
	gpio_pin_t *pin, *pins;
	size_t pin_count = 0;
	int i, bit, mask, pincaps, data;

	if (exynos_n_pin_groups == 0)
		return;

	/* find out how many pins we can offer */
	pin_count = 0;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		mask = grp->grp_pin_mask & ~grp->grp_pin_inuse_mask;
		pin_count += popcount32(mask);
	}

	/* if no pins available, don't proceed */
	if (pin_count == 0)
		return;
	
	/* allocate pin data */
	pins = kmem_zalloc(sizeof(gpio_pin_t) * pin_count, KM_SLEEP);
	KASSERT(pins);

	pincaps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;

	/* add all pins */
	pin = pins;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		mask = grp->grp_pin_mask & ~grp->grp_pin_inuse_mask;
		if (mask == 0)
			continue;
		gba.gba_gc = &grp->grp_gc_tag;
		gba.gba_pins = pin;
		data = bus_space_read_1(sc->sc_bst, grp->grp_bsh,
				EXYNOS_GPIO_DAT);
		for (bit = 0; mask != 0; mask >>= 1, data >>= 1, bit++) {
			if (mask & 1) {
				pin->pin_num = bit + (i << 3);
				pin->pin_caps = pincaps;
				pin->pin_flags = pincaps;
				pin->pin_state = (data & 1) != 0;
				pin++;
			}
		}
		gba.gba_npins = pin - gba.gba_pins;
		config_found_ia(self, "gpiobus", &gba, gpiobus_print);
	}
}
#endif


static void
exynos_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_gpio_softc * const sc = &exynos_gpio_sc;
	struct exyo_attach_args * const exyoaa = aux;
	struct exynos_gpio_pin_group *grp;
	prop_dictionary_t dict = device_properties(self);
	uint32_t nc;
	char scrap[16];
	int i;

	/* construct softc */
	sc->sc_dev = self;

	/* we use the core bushandle here */
	sc->sc_bst = exyoaa->exyo_core_bst;
	sc->sc_bsh = exyoaa->exyo_core_bsh;

	exynos_gpio_bootstrap();
	if (exynos_n_pin_groups == 0) {
		printf(": disabled, no pins defined\n");
		return;
	}

	KASSERT(exynos_pin_groups);
	KASSERT(exynos_n_pin_groups);

	aprint_naive("\n");
	aprint_normal("\n");

	/* go trough all pin groups */
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		snprintf(scrap, sizeof(scrap), "nc-%s", grp->grp_name);
		if (prop_dictionary_get_uint32(dict, scrap, &nc)) {
			KASSERT((~grp->grp_pin_mask & nc) == 0);
			/* switch off the pins we have signalled NC */
			grp->grp_pin_mask &= ~nc;
#if 0
			printf("%s: %-4s inuse_mask %02x, pin_mask %02x\n",
			    __func__, grp->grp_name,
			    grp->grp_pin_inuse_mask, grp->grp_pin_mask);
#endif
		}
	}

#if NGPIO > 0
	config_defer(self, exynos_gpio_config_pins);
#endif
}


/* pin access functions */
static u_int
exynos_gpio_get_pin_func(const struct exynos_gpio_pin_cfg *cfg, int pin)
{
	const u_int shift = (pin & 7) << 2;

	return (cfg->cfg >> shift) & 0x0f;
}


static void
exynos_gpio_set_pin_func(struct exynos_gpio_pin_cfg *cfg,
	int pin, int func)
{
	const u_int shift = (pin & 7) << 2;

	cfg->cfg &= ~(0x0f << shift);
	cfg->cfg |= func << shift;
}


static void
exynos_gpio_set_pin_pull(struct exynos_gpio_pin_cfg *cfg, int pin, int pull)
{
	const u_int shift = (pin & 7) << 1;

	cfg->pud &= ~(0x3 << shift);
	cfg->pud |= pull << shift;
}


static int
exynos_gpio_pin_read(void *cookie, int pin)
{
	struct exynos_gpio_pin_group * const grp = cookie;

	KASSERT(pin < grp->grp_bits);
	return (bus_space_read_1(exynos_gpio_sc.sc_bst, grp->grp_bsh,
		EXYNOS_GPIO_DAT) >> pin) & 1;
}


static void
exynos_gpio_pin_write(void *cookie, int pin, int value)
{
	struct exynos_gpio_pin_group * const grp = cookie;
	int val;

	KASSERT(pin < grp->grp_bits);
	val = bus_space_read_1(exynos_gpio_sc.sc_bst, grp->grp_bsh,
		EXYNOS_GPIO_DAT);
	val &= ~__BIT(pin);
	if (value)
		val |= __BIT(pin);
	bus_space_write_1(exynos_gpio_sc.sc_bst, grp->grp_bsh,
		EXYNOS_GPIO_DAT, val);
}


static void
exynos_gpio_update_cfg_regs(struct exynos_gpio_pin_group *grp,
	const struct exynos_gpio_pin_cfg *ncfg)
{
	bus_space_tag_t bst = &exynos_bs_tag;

	if (grp->grp_cfg.cfg != ncfg->cfg) {
		bus_space_write_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_CON, ncfg->cfg);
		grp->grp_cfg.cfg = ncfg->cfg;
	}
	if (grp->grp_cfg.pud != ncfg->pud) {
		bus_space_write_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_PUD, ncfg->pud);
		grp->grp_cfg.pud = ncfg->pud;
	}

	/* the following attributes are not yet setable */
#if 0
	if (grp->grp_cfg.drv != ncfg->drv) {
		bus_space_write_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_DRV, ncfg->drv);
		grp->grp_cfg.drv = ncfg->drv;
	}
	if (grp->grp_cfg.conpwd != ncfg->conpwd) {
		bus_space_write_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_CONPWD, ncfg->conpwd);
		grp->grp_cfg.conpwd = ncfg->conpwd;
	}
	if (grp->grp_cfg.pudpwd != ncfg->pudpwd) {
		bus_space_write_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_PUDPWD, ncfg->pudpwd);
		grp->grp_cfg.pudpwd = ncfg->pudpwd;
	}
#endif
}


static void
exynos_gpio_pin_ctl(void *cookie, int pin, int flags)
{
	struct exynos_gpio_pin_group * const grp = cookie;
	struct exynos_gpio_pin_cfg ncfg = grp->grp_cfg;
	int pull;

	/* honour pullup requests */
	pull = EXYNOS_GPIO_PIN_FLOAT;
	if (flags & GPIO_PIN_PULLUP)
		pull = EXYNOS_GPIO_PIN_PULL_UP;
	if (flags & GPIO_PIN_PULLDOWN)
		pull = EXYNOS_GPIO_PIN_PULL_DOWN;
	exynos_gpio_set_pin_pull(&ncfg, pin, pull);

	/* honour i/o */
	if (flags & GPIO_PIN_INPUT)
		exynos_gpio_set_pin_func(&ncfg, pin, EXYNOS_GPIO_FUNC_INPUT);
	if (flags & GPIO_PIN_OUTPUT)
		exynos_gpio_set_pin_func(&ncfg, pin, EXYNOS_GPIO_FUNC_OUTPUT);

	/* update any config registers that changed */
	exynos_gpio_update_cfg_regs(grp, &ncfg);
}


bool
exynos_gpio_pinset_available(const struct exynos_gpio_pinset *req)
{
	struct exynos_gpio_pin_group *grp;
	int i, n, inuse;

	KASSERT(req);
	if (exynos_n_pin_groups == 0)
		return false;

	/* we need a pinset group */
	if (strlen(req->pinset_group) == 0)
		return false;

	/* determine which group is requested */
	grp = NULL;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		if (strcmp(req->pinset_group, grp->grp_name) == 0)
			break;
	}
	/* found? */
	if (i == exynos_n_pin_groups)
		return false;
	KASSERT(grp);

	/* fail unconnected pins */
	if (req->pinset_mask & ~grp->grp_pin_mask)
		return false;

	/* if none in use, they are available */
	if (req->pinset_mask & ~grp->grp_pin_inuse_mask)
		return true;

	/* OK, so some are in use; now see if the request is compatible */
	inuse = req->pinset_mask & grp->grp_pin_inuse_mask;
	for (i = 0; inuse; i++, inuse >>= 1) {
		/* try to be smart by skipping zero's */
		n = ffs(inuse) -1;
		i += n;
		inuse >>= n;
		/* this pin is in use, check its usage */
		if (exynos_gpio_get_pin_func(&grp->grp_cfg, i) != req->pinset_func)
			return false;
	}

	/* seems to be OK */
	return true;
}


void
exynos_gpio_pinset_acquire(const struct exynos_gpio_pinset *req)
{
	struct exynos_gpio_pin_group *grp;
	struct exynos_gpio_pin_cfg ncfg;
	int i, n, todo;

	KASSERT(req);
	KASSERT(exynos_gpio_pinset_available(req));

	/* determine which group is requested */
	grp = NULL;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		if (strcmp(req->pinset_group, grp->grp_name) == 0)
			break;
	}
	KASSERT(grp);

	/* check if all the pins have the right function */
	if ((req->pinset_mask & ~grp->grp_pin_inuse_mask) == 0)
		return;

	/* copy current config for update routine */
	ncfg = grp->grp_cfg;

	/* update the function of each pin that is not in use */
	todo = req->pinset_mask & grp->grp_pin_inuse_mask;
	for (i = 0; todo; i++, todo >>= 1) {
		/* try to be smart by skipping zero's */
		n = ffs(todo) -1;
		i += n;
		todo >>= n;
		/* change the function of this pin */
		exynos_gpio_set_pin_func(&ncfg, i, req->pinset_func);
	}

	/* update config registers */
	exynos_gpio_update_cfg_regs(grp, &ncfg);

	/* mark pins in use */
	grp->grp_pin_inuse_mask |= req->pinset_mask;
}


/* get a pindata structure from a pinset structure */
void
exynos_gpio_pinset_to_pindata(const struct exynos_gpio_pinset *req, int pinnr,
	struct exynos_gpio_pindata *pd)
{
	struct exynos_gpio_pin_group *grp;
	int i;

	KASSERT(req);
	KASSERT(pd);
	KASSERT(req->pinset_mask & __BIT(pinnr));

	/* determine which group is requested */
	grp = NULL;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		if (strcmp(req->pinset_group, grp->grp_name) == 0)
			break;
	}
	KASSERT(grp);

	pd->pd_gc = &grp->grp_gc_tag;
	pd->pd_pin = pinnr;
}


/* XXXRPZ This release doesn't grock multiple usages! */
void
exynos_gpio_pinset_release(const struct exynos_gpio_pinset *req)
{
	struct exynos_gpio_pin_group *grp;
	int i;

	KASSERT(!exynos_gpio_pinset_available(req));

	/* determine which group is requested */
	grp = NULL;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		if (strcmp(req->pinset_group, grp->grp_name) == 0)
			break;
	}
	KASSERT(grp);

	/* bluntly mark as not being in use */
	grp->grp_pin_inuse_mask &= ~req->pinset_mask;
}


/*
 * name convention :
 * pin   = <func><groupname><pinnr>[<pud>]
 * func  = '<' | '>'
 * pinnr = '['['0'-'7']']'
 * pud   =  'F' | 'U' | 'D'
 *
 * example "<GPC1[0]", ">GPB[0]"
 */

bool
exynos_gpio_pin_reserve(const char *name, struct exynos_gpio_pindata *pd)
{
	struct exynos_gpio_softc * const sc = &exynos_gpio_sc;
	struct exynos_gpio_pin_group *grp;
	struct exynos_gpio_pin_cfg ncfg;
	prop_dictionary_t dict = device_properties(sc->sc_dev);
	const char *pin_data;
	char grp_name[15], *pos;
	int func, pud, pinnr;
	int pi, i;

	if (exynos_n_pin_groups == 0)
		return false;

	/* do we have a named pin description? */
	if (!prop_dictionary_get_cstring_nocopy(dict, name, &pin_data))
		return false;

	KASSERT(strlen(pin_data) < 10);
	if (!(pin_data[0] == '>' || pin_data[0] == '<')) {
		printf("%s: malformed pin data in '%s', missing direction\n",
			__func__, pin_data);
		return false;
	}

	func = (pin_data[0] == '<') ?
		EXYNOS_GPIO_FUNC_INPUT : EXYNOS_GPIO_FUNC_OUTPUT;

	/* find groupname */
	pi = 1; pos = grp_name;
	while (pin_data[pi] && pin_data[pi] != '[') {
		*pos++ = pin_data[pi++];
	}
	if (pin_data[pi] != '[') {
		printf("%s: malformed pin data in '%s', missing '['\n",
			__func__, pin_data);
		return false;
	}
	*pos++ = (char) 0;

	/* skip '[' */
	pi++;
	if (!(pin_data[pi] >= '0' && pin_data[pi] <= '7')) {
		printf("%s: malformed pin data in '%s', bad pin number\n",
			__func__, pin_data);
		return false;
	}
	pinnr = pin_data[pi] - '0';

	/* skip digit */
	pi++;
	if ((pin_data[pi] != ']')) {
		printf("%s: malformed pin data in '%s', missing end ']'\n",
			__func__, pin_data);
		return false;
	}

	/* skip ']' */
	pi++;
	pud = EXYNOS_GPIO_PIN_FLOAT;
	switch (tolower(pin_data[pi])) {
		case (char) 0:
			break;
		case 'f':
			pud = EXYNOS_GPIO_PIN_FLOAT;
			break;
		case 'u':
			pud = EXYNOS_GPIO_PIN_PULL_UP;
			break;
		case 'd':
			pud = EXYNOS_GPIO_PIN_PULL_DOWN;
			break;
		default:
			printf("%s: malformed pin data in '%s', expecting "
				"optional pull up/down or float argument\n",
				__func__, pin_data);
		return false;
	}

	/* determine which group is requested */
	grp = NULL;
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		if (strcmp(grp_name, grp->grp_name) == 0)
			break;
	}

	/* found? */
	if (i >= exynos_n_pin_groups) {
		printf("%s: malformed pin data in '%s', "
			"no such pin group name\n",
			__func__, grp_name);
		return false;
	}
	KASSERT(grp);

	/* in range? */
	if (pinnr >= grp->grp_bits)
		return false;

	/* marked as connected? */
	if ((grp->grp_pin_mask & __BIT(pinnr)) == 0)
		return false;

	/* it better not be used!! this is not taken lightly */
	KASSERT((grp->grp_pin_inuse_mask & __BIT(pinnr)) == 0);

	/* update our pin configuration */
	ncfg = grp->grp_cfg;
	exynos_gpio_set_pin_func(&ncfg, pinnr, func);
	exynos_gpio_set_pin_pull(&ncfg, pinnr, pud);
	exynos_gpio_update_cfg_regs(grp, &ncfg);

	grp->grp_pin_inuse_mask |= __BIT(pinnr);
	grp->grp_pin_mask &= ~__BIT(pinnr);

	pd->pd_gc = &grp->grp_gc_tag;
	pd->pd_pin = pinnr;

	return true;
}


/* bootstrapping */
void
exynos_gpio_bootstrap(void)
{
	bus_space_tag_t bst = &exynos_bs_tag;
	struct exynos_gpio_pin_group *grp;
	struct gpio_chipset_tag *gc_tag;
	int i;

	/* determine what we're running on */
#ifdef EXYNOS4
	if (IS_EXYNOS4_P()) {
		exynos_pin_groups = exynos4_pin_groups;
		exynos_n_pin_groups = __arraycount(exynos4_pin_groups);
	}
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) {
		exynos_pin_groups = exynos5_pin_groups;
		exynos_n_pin_groups = __arraycount(exynos5_pin_groups);
	}
#endif

	if (exynos_n_pin_groups == 0)
		return;

	/* init groups */
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		gc_tag = &grp->grp_gc_tag;

		bus_space_subregion(&exynos_bs_tag, exynos_core_bsh,
			grp->grp_core_offset, EXYNOS_GPIO_GRP_SIZE,
			&grp->grp_bsh);
		KASSERT(&grp->grp_bsh);

		grp->grp_pin_mask = __BIT(grp->grp_bits) - 1;
		grp->grp_pin_inuse_mask = 0;

		gc_tag->gp_cookie = grp;
		gc_tag->gp_pin_read  = exynos_gpio_pin_read;
		gc_tag->gp_pin_write = exynos_gpio_pin_write;
		gc_tag->gp_pin_ctl   = exynos_gpio_pin_ctl;

		/* read in our initial settings */
		grp->grp_cfg.cfg = bus_space_read_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_CON);
		grp->grp_cfg.pud = bus_space_read_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_PUD);
		grp->grp_cfg.drv = bus_space_read_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_DRV);
		grp->grp_cfg.conpwd = bus_space_read_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_CONPWD);
		grp->grp_cfg.pudpwd = bus_space_read_4(bst, grp->grp_bsh,
			EXYNOS_GPIO_PUDPWD);

		/*
		 * Normally we would count the busy pins.
		 *
		 * We can't check inuse here since uboot has used pins for its
		 * own use and left them configured forbidding us to use pins
		 * for our own sake.
		 */
#if 0
		for (int j = 0, int mask = 1;
		     (mask & grp->grp_pin_mask) != 0;
		     j++, mask <<= 1) {
			int func = exynos_gpio_get_pin_func(&grp->grp_cfg, j);
			if (func > EXYNOS_GPIO_FUNC_INPUT) {
				printf("%s: %s[%d] func %d\n", __func__,
				    grp->grp_name, j, func);
			}
		}
#endif
	}
#if 0
	printf("\n");
	printf("default NC pin list generated: \n");
	/* enable this for default NC pins list generation */
	for (i = 0; i < exynos_n_pin_groups; i++) {
		grp = &exynos_pin_groups[i];
		printf("prop_dictionary_set_uint32(dict, \"nc-%s\", "
			"0x%02x - 0b00000000);\n",
			grp->grp_name, grp->grp_pin_mask);
	}
#endif
}

