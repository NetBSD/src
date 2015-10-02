/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"
#include "gpio.h"

#include "opt_arm_debug.h"
#include "opt_allwinner.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_gpio.c,v 1.19 2015/10/02 14:06:02 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int awin_gpio_match(device_t, cfdata_t, void *);
static void awin_gpio_attach(device_t, device_t, void *);

static int awin_gpio_pin_read(void *, int);
static void awin_gpio_pin_write(void *, int, int);
static void awin_gpio_pin_ctl(void *, int, int);

#if 0
static const int ist_maps[] = {
	[IST_LEVEL_LOW] =	AWIN_PIO_EINT_LOW_LEVEL,
	[IST_LEVEL_HIGH] =	AWIN_PIO_EINT_HIGH_LEVEL,
	[IST_EDGE_FALLING] =	AWIN_PIO_EINT_POSITIVE_EDGE,
	[IST_EDGE_RISING] =	AWIN_PIO_EINT_NEGATIVE_EDGE,
	[IST_EDGE_BOTH] =	AWIN_PIO_EINT_DOUBLE_EDGE,
};
#endif

struct awin_gpio_pin_cfg {
	uint32_t cfg[4];
	uint32_t drv[2];
	uint32_t pul[2];
};

static struct awin_gpio_pin_group {
	bus_addr_t grp_offset;
	uint32_t grp_pin_mask;
	uint32_t grp_pin_inuse_mask;
	bus_space_handle_t grp_bsh;
	struct awin_gpio_pin_cfg grp_cfg;
	struct gpio_chipset_tag grp_gc_tag;
	const char grp_nc_name[6];
} pin_groups[] = {
	[0] = {
		.grp_offset = AWIN_PIO_OFFSET + 0 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PA_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[0],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pa",
	},
	[1] = {
		.grp_offset = AWIN_PIO_OFFSET + 1 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PB_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[1],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pb",
	},
	[2] = {
		.grp_offset = AWIN_PIO_OFFSET + 2 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PC_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[2],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pc",
	},
	[3] = {
		.grp_offset = AWIN_PIO_OFFSET + 3 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PD_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[3],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pd",
	},
	[4] = {
		.grp_offset = AWIN_PIO_OFFSET + 4 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PE_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[4],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pe",
	},
	[5] = {
		.grp_offset = AWIN_PIO_OFFSET + 5 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PF_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[5],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pf",
	},
	[6] = {
		.grp_offset = AWIN_PIO_OFFSET + 6 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PG_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[6],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pg",
	},
	[7] = {
		.grp_offset = AWIN_PIO_OFFSET + 7 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PH_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[7],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-ph",
	},
	[8] = {
		.grp_offset = AWIN_PIO_OFFSET + 8 * AWIN_PIO_GRP_SIZE,
		.grp_pin_mask = __BIT(AWIN_PIO_PI_PINS) - 1,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[8],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_nc_name = "nc-pi",
	},
	[9] = {
		.grp_offset = 0,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[9],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_pin_mask = 0,
		.grp_nc_name = "nc-pj",
	},
	[10] = {
		.grp_offset = 0,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[10],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_pin_mask = 0,
		.grp_nc_name = "nc-pk",
	},
	[11] = {
		.grp_offset = 0,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[11],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_pin_mask = 0,
		.grp_nc_name = "nc-pl",
	},
	[12] = {
		.grp_offset = 0,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[12],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_pin_mask = 0,
		.grp_nc_name = "nc-pm",
	},
	[13] = {
		.grp_offset = 0,
		.grp_gc_tag = {
			.gp_cookie = &pin_groups[13],
			.gp_pin_read = awin_gpio_pin_read,
			.gp_pin_write = awin_gpio_pin_write,
			.gp_pin_ctl = awin_gpio_pin_ctl,
		},
		.grp_pin_mask = 0,
		.grp_nc_name = "nc-pn",
	},
};


static struct awin_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
} awin_gpio_sc = {
	.sc_bst = &armv7_generic_bs_tag,
};

CFATTACH_DECL_NEW(awin_gpio, sizeof(struct awin_gpio_softc),
	awin_gpio_match, awin_gpio_attach, NULL, NULL);

static int
awin_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio __diagused = aux;
	const struct awin_locators * const loc __diagused = &aio->aio_loc;

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(loc->loc_port == AWINIOCF_PORT_DEFAULT);
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT);

	if (awin_gpio_sc.sc_dev != NULL)
		return 0;

	return 1;
}

#if NGPIO > 0
static void
awin_gpio_config_pins(device_t self)
{
	struct awin_gpio_softc * const sc = &awin_gpio_sc;

	/*
	 * First find out how many pins we can offer.
	 */
	size_t pin_count = 0;
	for (u_int i = 0; i < __arraycount(pin_groups); i++) {
		struct awin_gpio_pin_group * const grp = &pin_groups[i];

		pin_count +=
		    popcount32(grp->grp_pin_mask & ~grp->grp_pin_inuse_mask);
	}

	/*
	 * Allocate the pin data.
	 */
	gpio_pin_t * const pins = kmem_zalloc(sizeof(gpio_pin_t) * pin_count,
	    KM_SLEEP);
	KASSERT(pins != NULL);

	const int pincaps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT
		| GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;

	gpio_pin_t *pin = pins;
	for (u_int i = 0; i < __arraycount(pin_groups); i++) {
		struct awin_gpio_pin_group * const grp = &pin_groups[i];
		uint32_t mask = grp->grp_pin_mask & ~grp->grp_pin_inuse_mask;
		device_t gpio;

		/* 
		 * If this group has no bits to provide, skip it.
		 */
		if (mask == 0)
			continue;

		struct gpiobus_attach_args gba = {
			.gba_gc = &grp->grp_gc_tag,
			.gba_pins = pin,
		};

		uint32_t data = bus_space_read_4(sc->sc_bst, grp->grp_bsh,
		    AWIN_PIO_DAT_REG);
		for (int num = 0; mask != 0; mask >>= 1, data >>= 1, num++) {
			if (mask & 1) {
				pin->pin_num = num + (i << 5);
				pin->pin_caps = pincaps;
				pin->pin_flags = pincaps;
				pin->pin_state = (data & 1) != 0;
				pin++;
			}
		}

		gba.gba_npins = pin - gba.gba_pins;
		gpio = config_found_ia(self, "gpiobus", &gba, gpiobus_print);
		aprint_normal_dev(gpio, "port %c\n", 'A' + i);
	}
}
#endif /* NGPIO > 0 */

static void
awin_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct awin_gpio_softc * const sc = &awin_gpio_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t dict = device_properties(self);

	sc->sc_dev = self;

	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	for (u_int i = 0; i < __arraycount(pin_groups); i++) {
		struct awin_gpio_pin_group * const grp = &pin_groups[i];

		/*
		 * See if this group has any unconnected pins and make sure
		 * we won't use them.
		 */
		uint32_t nc;
		if (prop_dictionary_get_uint32(dict, grp->grp_nc_name, &nc)) {
			KASSERT((~grp->grp_pin_mask & nc) == 0);
			KASSERT((grp->grp_pin_inuse_mask & ~nc) == 0);
			grp->grp_pin_mask &= ~nc;
		}
	}

#if NGPIO > 0
	config_defer(self, awin_gpio_config_pins);
#endif
}

static u_int
awin_gpio_get_pin_func(const struct awin_gpio_pin_cfg *cfg, u_int pin)
{
	const u_int shift = (pin & 7) << 2;
	const u_int i = (pin >> 3) & 3;

	return (cfg->cfg[i] >> shift) & 0x0f;
}

static void
awin_gpio_set_pin_func(struct awin_gpio_pin_cfg *cfg, u_int pin, u_int func)
{
	const u_int shift = (pin & 7) << 2;
	const u_int i = (pin >> 3) & 3;
	
	cfg->cfg[i] &= ~(0x0f << shift);
	cfg->cfg[i] |= func << shift;
}

static void
awin_gpio_set_pin_pull(struct awin_gpio_pin_cfg *cfg, u_int pin, u_int pull)
{
	const u_int shift = (pin & 15) << 1;
	const u_int i = (pin >> 4) & 1;
	
	cfg->pul[i] &= ~(0x03 << shift);
	cfg->pul[i] |= pull << shift;
}

static void
awin_gpio_set_pin_drv(struct awin_gpio_pin_cfg *cfg, u_int pin, u_int drv)
{
	const u_int shift = (pin & 15) << 1;
	const u_int i = (pin >> 4) & 1;
	
	cfg->drv[i] &= ~(0x03 << shift);
	cfg->drv[i] |= drv << shift;
}

static void
awin_gpio_update_cfg_regs(bus_space_tag_t bst, struct awin_gpio_pin_group *grp,
    const struct awin_gpio_pin_cfg *ncfg)
{
	for (u_int i = 0; i < 4; i++) {
		if (grp->grp_cfg.cfg[i] != ncfg->cfg[i]) {
			bus_space_write_4(bst, grp->grp_bsh,
			    AWIN_PIO_CFG0_REG + 4 * i, ncfg->cfg[i]);	
			grp->grp_cfg.cfg[i] = ncfg->cfg[i];
		}
	}
	for (u_int i = 0; i < 2; i++) {
		if (grp->grp_cfg.drv[i] != ncfg->drv[i]) {
			bus_space_write_4(bst, grp->grp_bsh,
			    AWIN_PIO_DRV0_REG + 4 * i, ncfg->drv[i]);	
			grp->grp_cfg.drv[i] = ncfg->drv[i];
		}
		if (grp->grp_cfg.pul[i] != ncfg->pul[i]) {
			bus_space_write_4(bst, grp->grp_bsh,
			    AWIN_PIO_PUL0_REG + 4 * i, ncfg->pul[i]);	
			grp->grp_cfg.pul[i] = ncfg->pul[i];
		}
	}
}

void
awin_gpio_init(void)
{
	struct awin_gpio_softc * const sc = &awin_gpio_sc;

#ifdef VERBOSE_INIT_ARM
	printf(" free");
#endif

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		pin_groups[0].grp_pin_mask = __BIT(AWIN_A31_PIO_PA_PINS) - 1;
		pin_groups[1].grp_pin_mask = __BIT(AWIN_A31_PIO_PB_PINS) - 1;
		pin_groups[2].grp_pin_mask = __BIT(AWIN_A31_PIO_PC_PINS) - 1;
		pin_groups[3].grp_pin_mask = __BIT(AWIN_A31_PIO_PD_PINS) - 1;
		pin_groups[4].grp_pin_mask = __BIT(AWIN_A31_PIO_PE_PINS) - 1;
		pin_groups[5].grp_pin_mask = __BIT(AWIN_A31_PIO_PF_PINS) - 1;
		pin_groups[6].grp_pin_mask = __BIT(AWIN_A31_PIO_PG_PINS) - 1;
		pin_groups[7].grp_pin_mask = __BIT(AWIN_A31_PIO_PH_PINS) - 1;
		pin_groups[8].grp_offset = 0;		/* PI */
		pin_groups[8].grp_pin_mask = 0;		/* PI */
		pin_groups[9].grp_offset = 0;		/* PJ */
		pin_groups[9].grp_pin_mask = 0;		/* PJ */
		pin_groups[10].grp_offset = 0;		/* PK */
		pin_groups[10].grp_pin_mask = 0;	/* PK */
		pin_groups[11].grp_offset = AWIN_A31_CPUPIO_OFFSET +
					    0 * AWIN_PIO_GRP_SIZE;
		pin_groups[11].grp_pin_mask = __BIT(AWIN_A31_PIO_PL_PINS) - 1;
		pin_groups[12].grp_offset = AWIN_A31_CPUPIO_OFFSET +
					    1 * AWIN_PIO_GRP_SIZE;
		pin_groups[12].grp_pin_mask = __BIT(AWIN_A31_PIO_PM_PINS) - 1;
		pin_groups[13].grp_offset = 0;		/* PN */
		pin_groups[13].grp_pin_mask = 0;	/* PN */
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		pin_groups[0].grp_pin_mask = __BIT(AWIN_A80_PIO_PA_PINS) - 1;
		pin_groups[0].grp_offset = AWIN_A80_PIO_OFFSET + 
					   0 * AWIN_PIO_GRP_SIZE;
		pin_groups[1].grp_pin_mask = __BIT(AWIN_A80_PIO_PB_PINS) - 1;
		pin_groups[1].grp_offset = AWIN_A80_PIO_OFFSET + 
					   1 * AWIN_PIO_GRP_SIZE;
		pin_groups[2].grp_pin_mask = __BIT(AWIN_A80_PIO_PC_PINS) - 1;
		pin_groups[2].grp_offset = AWIN_A80_PIO_OFFSET + 
					   2 * AWIN_PIO_GRP_SIZE;
		pin_groups[3].grp_pin_mask = __BIT(AWIN_A80_PIO_PD_PINS) - 1;
		pin_groups[3].grp_offset = AWIN_A80_PIO_OFFSET + 
					   3 * AWIN_PIO_GRP_SIZE;
		pin_groups[4].grp_pin_mask = __BIT(AWIN_A80_PIO_PE_PINS) - 1;
		pin_groups[4].grp_offset = AWIN_A80_PIO_OFFSET + 
					   4 * AWIN_PIO_GRP_SIZE;
		pin_groups[5].grp_pin_mask = __BIT(AWIN_A80_PIO_PF_PINS) - 1;
		pin_groups[5].grp_offset = AWIN_A80_PIO_OFFSET + 
					   5 * AWIN_PIO_GRP_SIZE;
		pin_groups[6].grp_pin_mask = __BIT(AWIN_A80_PIO_PG_PINS) - 1;
		pin_groups[6].grp_offset = AWIN_A80_PIO_OFFSET + 
					   6 * AWIN_PIO_GRP_SIZE;
		pin_groups[7].grp_pin_mask = __BIT(AWIN_A80_PIO_PH_PINS) - 1;
		pin_groups[7].grp_offset = AWIN_A80_PIO_OFFSET + 
					   7 * AWIN_PIO_GRP_SIZE;
		pin_groups[8].grp_offset = 0;		/* PI */
		pin_groups[8].grp_pin_mask = 0;		/* PI */
		pin_groups[9].grp_offset = 0;		/* PJ */
		pin_groups[9].grp_pin_mask = 0;		/* PJ */
		pin_groups[10].grp_offset = 0;		/* PK */
		pin_groups[10].grp_pin_mask = 0;	/* PK */
		pin_groups[11].grp_offset = AWIN_A80_RPIO_OFFSET +
					    0 * AWIN_PIO_GRP_SIZE;
		pin_groups[11].grp_pin_mask = __BIT(AWIN_A80_PIO_PL_PINS) - 1;
		pin_groups[12].grp_offset = AWIN_A80_RPIO_OFFSET +
					    1 * AWIN_PIO_GRP_SIZE;
		pin_groups[12].grp_pin_mask = __BIT(AWIN_A80_PIO_PM_PINS) - 1;
		pin_groups[13].grp_offset = AWIN_A80_RPIO_OFFSET +
					    2 * AWIN_PIO_GRP_SIZE;
		pin_groups[13].grp_pin_mask = __BIT(AWIN_A80_PIO_PN_PINS) - 1;
	}

	for (u_int i = 0; i < __arraycount(pin_groups); i++) {
		struct awin_gpio_pin_group * const grp = &pin_groups[i];

		if (grp->grp_offset == 0)
			continue;

#if defined(ALLWINNER_A80)
		if (i >= 11) {
			bus_space_subregion(sc->sc_bst, awin_rcpus_bsh,
			    grp->grp_offset, AWIN_PIO_GRP_SIZE, &grp->grp_bsh);
		} else
#endif
		{
			bus_space_subregion(sc->sc_bst, awin_core_bsh,
			    grp->grp_offset, AWIN_PIO_GRP_SIZE, &grp->grp_bsh);
		}

		for (u_int j = 0; j < 4; j++) {
			grp->grp_cfg.cfg[j] = bus_space_read_4(sc->sc_bst,
			    grp->grp_bsh, AWIN_PIO_CFG0_REG + j * 4);
		}
		grp->grp_cfg.drv[0] = bus_space_read_4(sc->sc_bst,
		    grp->grp_bsh, AWIN_PIO_DRV0_REG);
		grp->grp_cfg.drv[1] = bus_space_read_4(sc->sc_bst,
		    grp->grp_bsh, AWIN_PIO_DRV1_REG);
		grp->grp_cfg.pul[0] = bus_space_read_4(sc->sc_bst,
		    grp->grp_bsh, AWIN_PIO_PUL0_REG);
		grp->grp_cfg.pul[1] = bus_space_read_4(sc->sc_bst,
		    grp->grp_bsh, AWIN_PIO_PUL1_REG);

#if !defined(AWIN_GPIO_IGNORE_FW)
		for (uint32_t j = 0, mask = 1;
		     (mask & grp->grp_pin_mask) != 0;
		     j++, mask <<= 1) {
			u_int func = awin_gpio_get_pin_func(&grp->grp_cfg, j);
			if (func > AWIN_PIO_FUNC_OUTPUT) {
				grp->grp_pin_inuse_mask |= mask;
			}
		}
#endif

#ifdef VERBOSE_INIT_ARM
		printf(" P%c=%d", 'A' + i,
		    popcount32(grp->grp_pin_mask & ~grp->grp_pin_inuse_mask));
#endif
	}
}

bool
awin_gpio_pinset_available(const struct awin_gpio_pinset *req)
{
	KASSERT(req != NULL);

	if (!req->pinset_group)
		return false;

#ifdef DIAGNOSTIC
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		KASSERT(
		    ('A' <= req->pinset_group && req->pinset_group <= 'I') ||
		    ('L' <= req->pinset_group && req->pinset_group <= 'M'));
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		KASSERT(
		    ('A' <= req->pinset_group && req->pinset_group <= 'I') ||
		    ('L' <= req->pinset_group && req->pinset_group <= 'N'));
	} else {
		KASSERT('A' <= req->pinset_group && req->pinset_group <= 'I');
	}
#endif

	struct awin_gpio_pin_group * const grp =
	    &pin_groups[req->pinset_group - 'A'];

	/*
	 * If there are unconnected pins, then they've been remove from
	 * the groups pin mask.  If we want pins that are unconnected,
	 * fail the request.
	 */
	if (req->pinset_mask & ~grp->grp_pin_mask)
		return false;

	/*
	 * If none of the pins are in use, they must be available.
	 */
	if (req->pinset_mask & ~grp->grp_pin_inuse_mask)
		return true;

	/*
	 * Check to see if the pins are already setup for this function. 
	 */
	for (uint32_t j = 0, inuse = req->pinset_mask & grp->grp_pin_inuse_mask;
	     inuse != 0;
	     j++, inuse >>= 1) {
		const u_int n = ffs(inuse) - 1;
		j += n;
		inuse >>= n;
		/*
		 * If this pin is in use but it's for a different
		 * function, fail the request.
		 */
		if (awin_gpio_get_pin_func(&grp->grp_cfg, j) != req->pinset_func)
			return false;
	}

	/*
	 * Nothing incompatible encountered so the pins must be available.
	 */
	return true;
}

void
awin_gpio_pinset_acquire(const struct awin_gpio_pinset *req)
{
	KASSERT(awin_gpio_pinset_available(req));

	struct awin_gpio_pin_group * const grp =
	    &pin_groups[req->pinset_group - 'A'];


	/*
	 * If all the pins already have right function, just return.
	 */
	if ((req->pinset_mask & ~grp->grp_pin_inuse_mask) == 0) {
		return;
	}

	/*
	 * Copy the current config.
	 */
	struct awin_gpio_pin_cfg ncfg = grp->grp_cfg;

	/*
	 * For each pin not inuse, update the cloned config's function for it.
	 */
	for (uint32_t j = 0, todo = req->pinset_mask & ~grp->grp_pin_inuse_mask;
	     todo != 0;
	     j++, todo >>= 1) {
		const u_int n = ffs(todo) - 1;
		j += n;
		todo >>= n;
		/*
		 * Change the function of this pin.
		 */
		awin_gpio_set_pin_func(&ncfg, j, req->pinset_func);

		if (req->pinset_flags & GPIO_PIN_PULLDOWN)
			awin_gpio_set_pin_pull(&ncfg, j, AWIN_PIO_PULL_DOWN);
		else if (req->pinset_flags & GPIO_PIN_PULLUP)
			awin_gpio_set_pin_pull(&ncfg, j, AWIN_PIO_PULL_UP);

		if (req->pinset_drv)
			awin_gpio_set_pin_drv(&ncfg, j, req->pinset_drv);
	}

	/*
	 * Now update any config register that changed.
	 */
	awin_gpio_update_cfg_regs(&armv7_generic_bs_tag, grp, &ncfg);

	/*
	 * Mark all these pins as in use.
	 */
	grp->grp_pin_inuse_mask |= req->pinset_mask;
}

void
awin_gpio_pinset_release(const struct awin_gpio_pinset *req)
{
	KASSERT(awin_gpio_pinset_available(req));

	struct awin_gpio_pin_group * const grp =
	    &pin_groups[req->pinset_group - 'A'];

#if 0
	/*
	 * Copy the current config.
	 */
	struct awin_gpio_pin_cfg ncfg = grp->grp_cfg;

	/*
	 * For each pin not inuse, update the cloned config's function for it.
	 */
	for (uint32_t j = 0, todo = req->pinset_mask;
	     todo != 0;
	     j++, todo >>= 1) {
		const u_int n = ffs(todo) - 1;
		j += n;
		todo >>= n;
		/*
		 * Change the function of this pin.
		 */
		awin_gpio_set_pin_func(&ncfg, AWIN_PIO_FUNC_INPUT);
	}

	/*
	 * Now update any config register that changed.
	 */
	awin_gpio_update_cfg_regs(sc->sc_bst, grp, &ncfg);
#endif

	/*
	 * Clear these pins as being in use.
	 */
	grp->grp_pin_inuse_mask &= ~req->pinset_mask;
}

static int
awin_gpio_pin_read(void *cookie, int pin)
{
	struct awin_gpio_pin_group * const grp = cookie;

	KASSERT(pin < 32);

	return (bus_space_read_4(awin_gpio_sc.sc_bst, grp->grp_bsh,
	    AWIN_PIO_DAT_REG) >> pin) & 1;
}

static void
awin_gpio_pin_write(void *cookie, int pin, int value)
{
	struct awin_gpio_pin_group * const grp = cookie;

	KASSERT(pin < 32);

	awin_reg_set_clear(awin_gpio_sc.sc_bst, grp->grp_bsh,
	    AWIN_PIO_DAT_REG, value ? __BIT(pin) : 0, __BIT(pin));
}

static void
awin_gpio_pin_ctl(void *cookie, int pin, int flags)
{
	struct awin_gpio_pin_group * const grp = cookie;
	struct awin_gpio_pin_cfg ncfg = grp->grp_cfg;

	u_int pull_value = AWIN_PIO_PULL_NONE;
	if (flags & GPIO_PIN_PULLUP) {
		pull_value = AWIN_PIO_PULL_UP;
	} else if (flags & GPIO_PIN_PULLDOWN) {
		pull_value = AWIN_PIO_PULL_DOWN;
	}
	awin_gpio_set_pin_pull(&ncfg, pin, pull_value);

	if (flags & GPIO_PIN_INPUT) {
		awin_gpio_set_pin_func(&ncfg, pin, AWIN_PIO_FUNC_INPUT);
	} else if (flags & GPIO_PIN_OUTPUT) {
		awin_gpio_set_pin_func(&ncfg, pin, AWIN_PIO_FUNC_OUTPUT);
	}

	/*
	 * Now update any config register that changed.
	 */
	awin_gpio_update_cfg_regs(&armv7_generic_bs_tag, grp, &ncfg);
}

bool
awin_gpio_pin_reserve(const char *name, struct awin_gpio_pindata *pd)
{
	struct awin_gpio_softc * const sc = &awin_gpio_sc;
	prop_dictionary_t dict = device_properties(sc->sc_dev);
	const char *pin_data;

	if (!prop_dictionary_get_cstring_nocopy(dict, name, &pin_data))
		return false;

	KASSERT(pin_data[0] == '>' || pin_data[0] == '<');
	KASSERT(pin_data[1] == 'P');

#ifdef DIAGNOSTIC
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		KASSERT(('A' <= pin_data[2] && pin_data[2] <= 'I') ||
			('L' <= pin_data[2] && pin_data[2] <= 'M'));
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		KASSERT(('A' <= pin_data[2] && pin_data[2] <= 'I') ||
			('L' <= pin_data[2] && pin_data[2] <= 'N'));
	} else {
		KASSERT('A' <= pin_data[2] && pin_data[2] <= 'I');
	}
#endif
	struct awin_gpio_pin_group * const grp = &pin_groups[pin_data[2] - 'A'];

	u_int pin = pin_data[3] - '0';
	KASSERT(pin < 10);
	if (pin_data[4] != 0) {
		KASSERT(pin_data[5] == 0);
		pin = pin * 10 + pin_data[4] - '0';
	}

	KASSERT(pin < 32);
	KASSERT(grp->grp_offset != 0);
	KASSERT(grp->grp_pin_mask & __BIT(pin));
	KASSERT((grp->grp_pin_inuse_mask & __BIT(pin)) == 0);

	struct awin_gpio_pin_cfg ncfg = grp->grp_cfg;
	awin_gpio_set_pin_func(&ncfg, pin,
	   pin_data[0] == '<' ? AWIN_PIO_FUNC_INPUT : AWIN_PIO_FUNC_OUTPUT);

	/*
	 * Now update any config register that changed.
	 */
	awin_gpio_update_cfg_regs(sc->sc_bst, grp, &ncfg);

	grp->grp_pin_inuse_mask &= ~__BIT(pin);

	pd->pd_gc = &grp->grp_gc_tag;
	pd->pd_pin = pin;
	return true;
}
