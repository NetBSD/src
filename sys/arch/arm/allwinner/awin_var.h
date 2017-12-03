/* $NetBSD: awin_var.h,v 1.10.4.3 2017/12/03 11:35:51 jdolecek Exp $ */
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
#define	AWINIO_ONLY_A80		__BIT(3)
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
	bus_space_handle_t aio_a80_usb_bsh;
	bus_space_handle_t aio_a80_core2_bsh;
	bus_space_handle_t aio_a80_rcpus_bsh;
	bus_dma_tag_t aio_dmat;
	bus_dma_tag_t aio_coherent_dmat;
};

struct awinfb_attach_args {
	void *afb_fb;
	uint32_t afb_width;
	uint32_t afb_height;
	bus_dma_tag_t afb_dmat;
	bus_dma_segment_t *afb_dmasegs;
	int afb_ndmasegs;
};

struct awin_gpio_pinset {
	uint8_t pinset_group;
	uint8_t pinset_func;
	uint32_t pinset_mask;
	int pinset_flags;
	int pinset_drv;
};

struct awin_gpio_pindata {
	gpio_chipset_tag_t pd_gc;
	int pd_pin;
};

struct awin_dma_channel;

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern bus_space_handle_t awin_core_bsh;
#if defined(ALLWINNER_A80)
extern bus_space_handle_t awin_core2_bsh;
extern bus_space_handle_t awin_rcpus_bsh;
#endif
extern struct arm32_bus_dma_tag awin_dma_tag;
extern struct arm32_bus_dma_tag awin_coherent_dma_tag;

psize_t awin_memprobe(void);
void	awin_bootstrap(vaddr_t, vaddr_t);
void	awin_dma_bootstrap(psize_t);
void	awin_pll2_enable(void);
void	awin_pll3_enable(void);
void	awin_pll6_enable(void);
void	awin_pll7_enable(void);
void	awin_pll3_set_rate(uint32_t);
void	awin_pll7_set_rate(uint32_t);
uint32_t awin_pll5x_get_rate(void);
uint32_t awin_pll6_get_rate(void);
uint32_t awin_periph0_get_rate(void);
void	awin_cpu_hatch(struct cpu_info *);

#define AWIN_CHIP_ID_A10	AWIN_SRAM_VER_KEY_A10
#define AWIN_CHIP_ID_A13	AWIN_SRAM_VER_KEY_A13
#define AWIN_CHIP_ID_A31	AWIN_SRAM_VER_KEY_A31
#define AWIN_CHIP_ID_A23	AWIN_SRAM_VER_KEY_A23
#define AWIN_CHIP_ID_A20	AWIN_SRAM_VER_KEY_A20
#define AWIN_CHIP_ID_A80	AWIN_SRAM_VER_KEY_A80
uint16_t awin_chip_id(void);
const char *awin_chip_name(void);

void	awin_gpio_init(void);
bool	awin_gpio_pinset_available(const struct awin_gpio_pinset *);
void	awin_gpio_pinset_acquire(const struct awin_gpio_pinset *);
void	awin_gpio_pinset_release(const struct awin_gpio_pinset *);
bool	awin_gpio_pin_reserve(const char *, struct awin_gpio_pindata *);

void *	awin_dma_alloc(const char *, void (*)(void *), void *);
void	awin_dma_free(void *);
uint32_t awin_dma_get_config(void *);
void	awin_dma_set_config(void *, uint32_t);
int	awin_dma_transfer(void *, paddr_t, paddr_t, size_t);
void	awin_dma_halt(void *);

struct videomode;
unsigned int awin_tcon_get_clk_pll(int);
unsigned int awin_tcon_get_clk_div(int);
bool	awin_tcon_get_clk_dbl(int);
void	awin_tcon1_set_videomode(int, const struct videomode *);
void	awin_tcon1_enable(int, bool);
void	awin_tcon_setvideo(int, bool);
void	awin_debe_set_videomode(int, const struct videomode *);
void	awin_debe_enable(int, bool);
int	awin_debe_ioctl(device_t, u_long, void *);
int	awin_mp_ioctl(device_t, u_long, void *);
void	awin_mp_setbase(device_t, paddr_t, size_t);

struct awin_hdmi_info {
	bool	display_connected;
	char	display_vendor[16];
	char	display_product[16];
	bool	display_hdmimode;
};
void	awin_hdmi_get_info(struct awin_hdmi_info *);
void	awin_hdmi_poweron(bool);

void	awin_fb_set_videomode(device_t, u_int, u_int);
void	awin_fb_ddb_trap_callback(int);

void	awin_wdog_reset(void);
void	awin_tmr_cpu_init(struct cpu_info *);

int	awin_set_mpu_volt(int, bool);

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

struct i2c_controller *awin_twi_get_controller(device_t);

#endif /* _ARM_ALLWINNER_AWIN_VAR_H_ */
