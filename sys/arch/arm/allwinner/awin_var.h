/* $NetBSD: awin_var.h,v 1.16 2014/09/11 02:16:15 jmcneill Exp $ */
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

#ifndef _ARM_ALLWINNER_AWIN_VAR_H_
#define _ARM_ALLWINNER_AWIN_VAR_H_

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

struct awin_locators {
	const char *loc_name;
	bus_size_t loc_offset;
	bus_size_t loc_size;
	int loc_port; 
	int loc_intr;
#define	AWINIO_INTR_DEFAULT	0
	int loc_flags;
#define	AWINIO_REQUIRED		__BIT(8)
#define	AWINIO_ONLY		__BITS(7,0)
#define	AWINIO_ONLY_A31		__BIT(2)
#define	AWINIO_ONLY_A20		__BIT(1)
#define	AWINIO_ONLY_A10		__BIT(0)
};

struct awinio_attach_args {
	struct awin_locators aio_loc;
	bus_space_tag_t aio_core_bst;
	bus_space_tag_t aio_core_a4x_bst;
	bus_space_handle_t aio_core_bsh;
	bus_space_handle_t aio_ccm_bsh;
	bus_dma_tag_t aio_dmat;
	bus_dma_tag_t aio_coherent_dmat;
};

struct awin_gpio_pinset {
	uint8_t pinset_group;
	uint8_t pinset_func;
	uint32_t pinset_mask;
};

struct awin_gpio_pindata {
	gpio_chipset_tag_t pd_gc;
	int pd_pin;
};

enum awin_dma_type {
	AWIN_DMA_TYPE_NDMA,
	AWIN_DMA_TYPE_DDMA,
};

struct awin_dma_channel;

extern struct bus_space awin_bs_tag;
extern struct bus_space awin_a4x_bs_tag;
extern bus_space_handle_t awin_core_bsh;
extern struct arm32_bus_dma_tag awin_dma_tag;
extern struct arm32_bus_dma_tag awin_coherent_dma_tag;

psize_t awin_memprobe(void);
void	awin_bootstrap(vaddr_t, vaddr_t); 
void	awin_dma_bootstrap(psize_t);
void	awin_pll2_enable(void);
void	awin_pll6_enable(void);
void	awin_pll7_enable(void);
void	awin_cpu_hatch(struct cpu_info *);

#define AWIN_CHIP_ID_A10	0x1623
#define AWIN_CHIP_ID_A13	0x1625
#define AWIN_CHIP_ID_A31	0x1633
#define AWIN_CHIP_ID_A23	0x1650
#define AWIN_CHIP_ID_A20	0x1651
uint16_t awin_chip_id(void);
const char *awin_chip_name(void);

void	awin_gpio_init(void);
bool	awin_gpio_pinset_available(const struct awin_gpio_pinset *);
void	awin_gpio_pinset_acquire(const struct awin_gpio_pinset *);
void	awin_gpio_pinset_release(const struct awin_gpio_pinset *);
bool	awin_gpio_pin_reserve(const char *, struct awin_gpio_pindata *);

struct awin_dma_channel *awin_dma_alloc(enum awin_dma_type,
					      void (*)(void *), void *);
void	awin_dma_free(struct awin_dma_channel *);
uint32_t awin_dma_get_config(struct awin_dma_channel *);
void	awin_dma_set_config(struct awin_dma_channel *, uint32_t);
int	awin_dma_transfer(struct awin_dma_channel *, paddr_t, paddr_t, size_t);
void	awin_dma_halt(struct awin_dma_channel *);

void	awin_wdog_reset(void);
void	awin_tmr_cpu_init(struct cpu_info *);

static inline void
awin_gpio_pindata_write(const struct awin_gpio_pindata *pd, int value)
{
	gpiobus_pin_write(pd->pd_gc, pd->pd_pin, value);
}

static inline int
awin_gpio_pindata_read(const struct awin_gpio_pindata *pd)
{
	return gpiobus_pin_read(pd->pd_gc, pd->pd_pin);
}

static void inline
awin_reg_set_clear(bus_space_tag_t bst, bus_space_handle_t bsh,
    bus_size_t o, uint32_t set_mask, uint32_t clr_mask)
{
	const uint32_t old = bus_space_read_4(bst, bsh, o);
	const uint32_t new = set_mask | (old & ~clr_mask);
	if (old != new) {
		bus_space_write_4(bst, bsh, o, new);
	}
}

#endif /* _ARM_ALLWINNER_AWIN_VAR_H_ */
