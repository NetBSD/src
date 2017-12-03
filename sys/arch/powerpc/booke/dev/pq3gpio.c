/*	$NetBSD: pq3gpio.c,v 1.8.2.1 2017/12/03 11:36:36 jdolecek Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#define	GLOBAL_PRIVATE
#define	GPIO_PRIVATE

#include "opt_mpc85xx.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pq3gpio.c,v 1.8.2.1 2017/12/03 11:36:36 jdolecek Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>

#include "ioconf.h"

#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/gpio/gpiovar.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/spr.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

struct pq3gpio_group {
	struct gpio_chipset_tag gc_tag;
	gpio_pin_t gc_pins[32];
	bus_space_tag_t gc_bst;
	bus_space_handle_t gc_bsh;
	bus_size_t gc_reg;
};

struct pq3gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	SIMPLEQ_HEAD(,pq3gpio_group) sc_gpios;
};

static int
pq3gpio_pin_read(void *v, int num)
{
	struct pq3gpio_group * const gc = v;

	uint32_t data = bus_space_read_4(gc->gc_bst, gc->gc_bsh, gc->gc_reg);

	return (data >> (gc->gc_pins[num].pin_num ^ 31)) & 1;
}

static void
pq3gpio_pin_write(void *v, int num, int val)
{
	struct pq3gpio_group * const gc = v;
	const u_int mask = 1 << (gc->gc_pins[num].pin_num ^ 31);

	val = val ? mask : 0;
	u_int data = bus_space_read_4(gc->gc_bst, gc->gc_bsh, gc->gc_reg);
	if ((data & mask) != val) {
		data = (data & ~mask) | val;
		bus_space_write_4(gc->gc_bst, gc->gc_bsh, gc->gc_reg, data);
	}
}

#if defined(MPC8548) || defined(MPC8555) || defined(MPC8544)
static void
pq3gpio_null_pin_ctl(void *v, int num, int ctl)
{
}
#endif

#if defined(P1025)
/*
 * P1025 has controllable input/output pins
 */
static void
pq3gpio_pin_ctl(void *v, int num, int ctl)
{
	struct pq3gpio_group * const gc = v;
	const size_t shift = gc->gc_pins[num].pin_num ^ 31;
  
	uint64_t old_dir =
	    ((uint64_t)bus_space_read_4(gc->gc_bst, gc->gc_bsh, CPDIR1) << 32)
	    | (bus_space_read_4(gc->gc_bst, gc->gc_bsh, CPDIR2) << 0);

	uint32_t dir = 0;
	switch (ctl & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_INPUT|GPIO_PIN_OUTPUT:	dir = CPDIR_INOUT; break;
	case GPIO_PIN_OUTPUT:			dir = CPDIR_OUT; break;
	case GPIO_PIN_INPUT:			dir = CPDIR_INOUT; break;
	case 0:					dir = CPDIR_DIS; break;
	}

	uint64_t new_dir = (old_dir & (3ULL << (2 * shift)))
	    | ((uint64_t)dir << (2 * shift));

	if ((uint32_t)old_dir != (uint32_t)new_dir)
		bus_space_write_4(gc->gc_bst, gc->gc_bsh, CPDIR2,
		    (uint32_t)new_dir);
	new_dir >>= 32;
	old_dir >>= 32;
	if ((uint32_t)old_dir != (uint32_t)new_dir)
		bus_space_write_4(gc->gc_bst, gc->gc_bsh, CPDIR1,
		    (uint32_t)new_dir);

	/*
	 * Now handle opendrain
	 */
	uint32_t old_odr = bus_space_read_4(gc->gc_bst, gc->gc_bsh, CPODR);
	uint32_t new_odr = old_odr;
	uint32_t odr_mask = 1UL << shift;

	if (ctl & GPIO_PIN_OPENDRAIN) {
		new_odr |= odr_mask;
	} else {
		new_odr &= ~odr_mask;
	}

	if (old_odr != new_odr)
		bus_space_write_4(gc->gc_bst, gc->gc_bsh, CPODR, new_odr);
}
#endif

#if defined(MPC8536) || defined(P2020) || defined(P1023)
/*
 * MPC8536 / P20x0 / P1023 have controllable input/output pins
 */
static void
pq3gpio_pin_ctl(void *v, int num, int ctl)
{
	struct pq3gpio_group * const gc = v;
	const u_int mask = 1 << (gc->gc_pins[num].pin_num ^ 31);
  
	uint32_t old_dir = bus_space_read_4(gc->gc_bst, gc->gc_bsh, GPDIR);
	uint32_t new_dir = old_dir;
	switch (ctl & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_OUTPUT:	new_dir |= mask; break;
	case GPIO_PIN_INPUT:	new_dir &= ~mask; break;
	default:		return;
	}
	if (old_dir != new_dir)
		bus_space_write_4(gc->gc_bst, gc->gc_bsh, GPDIR, new_dir);

	/*
	 * Now handle opendrain
	 */
	uint32_t old_odr = bus_space_read_4(gc->gc_bst, gc->gc_bsh, GPODR);
	uint32_t new_odr = old_odr;

	if (ctl & GPIO_PIN_OPENDRAIN) {
		new_odr |= mask;
	} else {
		new_odr &= ~mask;
	}

	if (old_odr != new_odr)
		bus_space_write_4(gc->gc_bst, gc->gc_bsh, GPODR, new_odr);
}
#endif

static void
pq3gpio_group_create(device_t self, bus_space_tag_t bst, bus_space_handle_t bsh,
	bus_size_t reg, uint32_t pinmask, int pincaps,
	void (*pin_ctl)(void *, int, int))
{
	struct pq3gpio_group * const gc = kmem_zalloc(sizeof(*gc), KM_SLEEP);

	gc->gc_bst = bst;
	gc->gc_bsh = bsh;
	gc->gc_reg = reg;
	gc->gc_tag.gp_cookie = gc;
#if 0
	gc->gc_tag.gp_gc_open = pq3gpio_gc_open;
	gc->gc_tag.gp_gc_close = pq3gpio_gc_close;
#endif
	gc->gc_tag.gp_pin_read = pq3gpio_pin_read;
	gc->gc_tag.gp_pin_write = pq3gpio_pin_write;
	gc->gc_tag.gp_pin_ctl = pin_ctl;

	u_int data = bus_space_read_4(gc->gc_bst, gc->gc_bsh, reg);
	u_int mask = __BIT(31);
	gpio_pin_t *pin = gc->gc_pins;
	for (u_int i = 0; mask != 0; i++, mask >>= 1) {
		if (mask & pinmask) {
			pin->pin_num = i;
			pin->pin_caps = pincaps;
			pin->pin_flags = pincaps;
			pin->pin_state = (data & mask) != 0;
			pin++;
		}
	}

	struct gpiobus_attach_args gba = {
		.gba_gc = &gc->gc_tag,
		.gba_pins = gc->gc_pins,
		.gba_npins = pin - gc->gc_pins,
	};

	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}

#ifdef MPC8536
static void
pq3gpio_mpc8536_attach(device_t self, bus_space_tag_t bst,
	bus_space_handle_t bsh, u_int svr)
{
	static const uint8_t gpio2pmuxcr_map[] = {
		[0] = ilog2(PMUXCR_PCI_REQGNT3),
		[1] = ilog2(PMUXCR_PCI_REQGNT4),
		[2] = ilog2(PMUXCR_PCI_REQGNT3),
		[3] = ilog2(PMUXCR_PCI_REQGNT4),
		[4] = ilog2(PMUXCR_SDHC_CD),
		[5] = ilog2(PMUXCR_SDHC_WP),
		[6] = ilog2(PMUXCR_USB1),
		[7] = ilog2(PMUXCR_USB1),
		[8] = ilog2(PMUXCR_USB2),
		[9] = ilog2(PMUXCR_USB2),
		[10] = ilog2(PMUXCR_DMA0),
		[11] = ilog2(PMUXCR_DMA1),
		[12] = ilog2(PMUXCR_DMA0),
		[13] = ilog2(PMUXCR_DMA1),
		[14] = ilog2(PMUXCR_DMA0),
		[15] = ilog2(PMUXCR_DMA1),
	};
	
	uint32_t pinmask = 0xffff0000;	/* assume all bits are valid */
	uint32_t gpiomask = __BIT(31);
	size_t pincnt = 16;
	const uint32_t pmuxcr = cpu_read_4(GLOBAL_BASE + PMUXCR);
	for (size_t i = 0; i < __arraycount(gpio2pmuxcr_map);
	     i++, gpiomask >>= 1) {
		if (pmuxcr & __BIT(gpio2pmuxcr_map[i])) {
			pinmask &= ~gpiomask;
			pincnt--;
		}
	}

	/*
	 * Create GPIO pin groups
	 */
	aprint_normal_dev(self, "%zu input/output/opendrain pins\n",
	    pincnt);
	pq3gpio_group_create(self, bst, bsh, GPDAT, pinmask,
	    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN,
	    pq3gpio_pin_ctl);
}
#endif /* MPC8536 */

#ifdef MPC8544
static void
pq3gpio_mpc8544_attach(device_t self, bus_space_tag_t bst,
	bus_space_handle_t bsh, u_int svr)
{
	/*
	 * Enable GPOUT
	 */
	uint32_t gpiocr = bus_space_read_4(bst, bsh, GPIOCR);
	gpiocr |= GPIOCR_GPOUT;
	bus_space_write_4(bst, bsh, GPIOCR, gpiocr);

	aprint_normal_dev(self, "8 input pins, 8 output pins\n");

	/*
	 * Create GPIO pin groups
	 */
	pq3gpio_group_create(self, bst, bsh, GPINDR, 0xff000000,
	    GPIO_PIN_INPUT, pq3gpio_null_pin_ctl);
	pq3gpio_group_create(self, bst, bsh, GPOUTDR, 0xff000000,
	    GPIO_PIN_OUTPUT, pq3gpio_null_pin_ctl);
}
#endif /* MPC8544 */

#if defined(MPC8548) || defined(MPC8555)
static void
pq3gpio_mpc8548_attach(device_t self, bus_space_tag_t bst,
	bus_space_handle_t bsh, u_int svr)
{
	const uint32_t pordevsr = bus_space_read_4(bst, bsh, PORDEVSR);
	const uint32_t devdisr = bus_space_read_4(bst, bsh, DEVDISR);
	uint32_t gpiocr = bus_space_read_4(bst, bsh, GPIOCR);

	uint32_t inmask = 0;
	uint32_t outmask = 0;

	size_t ipins = 0;
	size_t opins = 0;

	aprint_normal_dev(self, "GPIOCR %#x, DEVDISR %#x, PORDEVSR %#x\n",
	    gpiocr, devdisr, pordevsr);
	aprint_normal_dev(self, "GPINDR %#x, GPOUTDR %#x, GPPORCR %#x\n",
	    bus_space_read_4(bst, bsh, GPINDR),
	    bus_space_read_4(bst, bsh, GPOUTDR),
	    bus_space_read_4(bst, bsh, GPPORCR));

	/*
	 * Use PCI2 AD[15:0] as GPIO if PCI2 is disabled and
	 * PCI1 is either disabled or not 64bits wide.
	 */
	if ((devdisr & DEVDISR_PCI2) &&
	    ((devdisr & DEVDISR_PCI1) || (pordevsr & PORDEVSR_PCI32))) {
		gpiocr |= GPIOCR_PCIOUT;
		gpiocr |= GPIOCR_PCIIN;
		outmask |= 0x00ff0000;
		inmask |= 0x00ff0000;
		opins += 8;
		ipins += 8;
	}
	if (devdisr & DEVDISR_TSEC2) {
		gpiocr |= GPIOCR_TX2;
		gpiocr |= GPIOCR_RX2;
		outmask |= 0xff000000;
		inmask |= 0xff000000;
		opins += 8;
		ipins += 8;
	}
	if (svr != (SVR_MPC8555v1 >> 16)) {
		gpiocr |= GPIOCR_GPOUT;
		outmask |= 0x000000ff;
		opins += 8;
	}
#if 1
	aprint_normal_dev(self, "GPIOCR: %#x\n", gpiocr);
#else
	bus_space_write_4(bst, bsh, GPIOCR, gpiocr);
#endif

	/*
	 * Create GPIO pin groups
	 */
	aprint_normal_dev(self, "%zu input pins, %zu output pins\n",
	    ipins, opins);

	if (inmask)
		pq3gpio_group_create(self, bst, bsh, GPINDR, inmask,
		    GPIO_PIN_INPUT, pq3gpio_null_pin_ctl);
	if (outmask)
		pq3gpio_group_create(self, bst, bsh, GPOUTDR, outmask,
		    GPIO_PIN_OUTPUT, pq3gpio_null_pin_ctl);
}
#endif /* MPC8548 */

#ifdef P1025
static void
pq3gpio_p1025_attach(device_t self, bus_space_tag_t bst,
	bus_space_handle_t bsh, u_int svr)
{
	static const uint32_t gpio2pmuxcr_map[][4] = {
		{ 0, __BIT(12), 0, PMUXCR_SDHC_WP },
		{ __BIT(15), __BIT(8), 0, PMUXCR_USB1 },
		{ __BITS(14,4)|__BIT(16)|__BITS(27,17)|__BIT(30),
		  __BIT(1)|__BITS(3,2), 0, PMUXCR_QE0 },
		{ __BITS(3,1), 0, 0, PMUXCR_QE3 },
		{ 0, __BITS(17,14), 0, PMUXCR_QE8 },
		{ __BIT(29), __BITS(19,18), 0, PMUXCR_QE9 },
		{ 0, __BITS(22,21), 0, PMUXCR_QE10 },
		{ 0, __BITS(28,23), 0, PMUXCR_QE11 },
		{ 0, __BIT(20), 0, PMUXCR_QE12 },
	};
	
	uint32_t pinmask[3] = {
		 0xffffffff, 0xffffffff, 0xffffffff
	};	/* assume all bits are valid */
	const uint32_t pmuxcr = cpu_read_4(GLOBAL_BASE + PMUXCR);
	for (size_t i = 0; i < __arraycount(gpio2pmuxcr_map); i++) {
		if (pmuxcr & gpio2pmuxcr_map[i][3]) {
			pinmask[0] &= ~gpio2pmuxcr_map[i][0];
			pinmask[1] &= ~gpio2pmuxcr_map[i][1];
			pinmask[2] &= ~gpio2pmuxcr_map[i][2];
		}
	}

	/*
	 * Create GPIO pin groups
	 */
	for (size_t i = 0; i < 3; i++) {
		if (pinmask[i]) {
			bus_space_handle_t bsh2;
			aprint_normal_dev(self,
			    "gpio[%c]: %zu input/output/opendrain pins\n",
			    "abc"[i], popcount32(pinmask[i]));
			bus_space_subregion(bst, bsh, CPBASE(i), 0x20, &bsh2);
			pq3gpio_group_create(self, bst, bsh2, CPDAT,
			    pinmask[0],
			    GPIO_PIN_INPUT|GPIO_PIN_OUTPUT|GPIO_PIN_OPENDRAIN,
			    pq3gpio_pin_ctl);
		}
	}
}
#endif /* P1025 */

#ifdef P2020
static void
pq3gpio_p20x0_attach(device_t self, bus_space_tag_t bst,
	bus_space_handle_t bsh, u_int svr)
{
	static const uint32_t gpio2pmuxcr_map[][2] = {
		{ __BIT(8), PMUXCR_SDHC_CD },
		{ __BIT(9), PMUXCR_SDHC_WP },
		/*
		 * These are really two bits but the low bit MBZ so we ignore
		 * it.
		 */
		{ __BIT(10), PMUXCR_TSEC3_TS },
		{ __BIT(11), PMUXCR_TSEC3_TS },
	};
	
	uint32_t pinmask = 0xffff0000;	/* assume all bits are valid */
	size_t pincnt = 16;
	const uint32_t pmuxcr = cpu_read_4(GLOBAL_BASE + PMUXCR);
	for (size_t i = 0; i < __arraycount(gpio2pmuxcr_map); i++) {
		if (pmuxcr & gpio2pmuxcr_map[i][1]) {
			pinmask &= ~gpio2pmuxcr_map[i][0];
			pincnt--;
		}
	}

	/*
	 * Create GPIO pin groups
	 */
	aprint_normal_dev(self, "%zu input/output/opendrain pins\n",
	    pincnt);
	pq3gpio_group_create(self, bst, bsh, GPDAT, pinmask,
	    GPIO_PIN_INPUT|GPIO_PIN_OUTPUT|GPIO_PIN_OPENDRAIN,
	    pq3gpio_pin_ctl);
}
#endif /* P2020 */

#ifdef P1023
static void
pq3gpio_p1023_attach(device_t self, bus_space_tag_t bst,
	bus_space_handle_t bsh, u_int svr)
{
	static const uint32_t gpio2pmuxcr2_map[][3] = {
		{ __PPCBITS( 0, 1), __PPCBITS( 0, 1), 0 },	/* GPIO_1 */
		{ __PPCBIT(2),      __PPCBITS( 2, 3), 0 },	/* GPUO_2 */
		{ __PPCBITS( 4, 5), __PPCBITS( 4, 5), 0 },	/* GPUO_3 */
		{ __PPCBITS( 6, 7), __PPCBITS( 6, 7), 0 },	/* GPUO_4 */
		{ __PPCBITS( 8, 9), __PPCBITS( 8, 9), 0 },	/* GPUO_5 */
		{ __PPCBITS(10,11), __PPCBITS(10,11), 0 },	/* GPUO_6 */
		{ __PPCBITS(12,13), __PPCBITS(12,13), 0 },	/* GPUO_7 */
		{ __PPCBITS(14,15), __PPCBITS(14,15), 0 },	/* GPUO_8 */
		{ __PPCBIT(3),      __PPCBITS(18,19), 0 },	/* GPUO_9 */
	};

	uint32_t pinmask = 0xffff0000;	/* assume all bits are valid */
	size_t pincnt = 16;
	const uint32_t pmuxcr2 = cpu_read_4(GLOBAL_BASE + PMUXCR2);
	for (size_t i = 0; i < __arraycount(gpio2pmuxcr2_map); i++) {
		const uint32_t *map = gpio2pmuxcr2_map[i];
		if ((pmuxcr2 & map[1]) != map[2]) {
			pinmask &= ~map[0];
			pincnt--;
		}
	}

	/*
	 * Create GPIO pin groups
	 */
	aprint_normal_dev(self, "%zu input/output/opendrain pins\n", pincnt);
	pq3gpio_group_create(self, bst, bsh, GPDAT, pinmask,
	    GPIO_PIN_INPUT|GPIO_PIN_OUTPUT|GPIO_PIN_OPENDRAIN, pq3gpio_pin_ctl);
}
#endif /* P1023 */

static const struct pq3gpio_svr_info {
	uint16_t si_svr;
	void (*si_attach)(device_t, bus_space_tag_t, bus_space_handle_t, u_int);
	bus_addr_t si_base;
	bus_size_t si_size;
} pq3gpio_svrs[] = {
#ifdef MPC8548
	{ SVR_MPC8548v2 >> 16, pq3gpio_mpc8548_attach,
	    GLOBAL_BASE, GLOBAL_SIZE },
#endif
#ifdef MPC8555
	{ SVR_MPC8555v1 >> 16, pq3gpio_mpc8548_attach,
	    GLOBAL_BASE, GLOBAL_SIZE },
#endif
#ifdef MPC8544
	{ SVR_MPC8544v1 >> 16, pq3gpio_mpc8544_attach,
	    GLOBAL_BASE, GLOBAL_SIZE },
#endif
#ifdef MPC8536
	{ SVR_MPC8536v1 >> 16, pq3gpio_mpc8536_attach,
	    GPIO_BASE, GPIO_SIZE },
#endif
#ifdef P1025
	{ SVR_P1025v1 >> 16, pq3gpio_p1025_attach,
	    GLOBAL_BASE, GLOBAL_SIZE },
#endif
#ifdef P2020
	{ SVR_P2020v2 >> 16, pq3gpio_p20x0_attach,
	    GPIO_BASE, GPIO_SIZE },
#endif
#ifdef P1023
	{ SVR_P1023v1 >> 16, pq3gpio_p1023_attach,
	    GPIO_BASE, GPIO_SIZE },
#endif
};

void
pq3gpio_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	bus_space_tag_t bst = ma->ma_memt;
	bus_space_handle_t bsh;

	const uint16_t svr = e500_get_svr();
	for (u_int i = 0; i < __arraycount(pq3gpio_svrs); i++) {
		const struct pq3gpio_svr_info * const si = &pq3gpio_svrs[i];
		if (si->si_svr == svr) {
			int error = bus_space_map(bst, si->si_base,
			    si->si_size, 0, &bsh);
			if (error) {
				aprint_error_dev(self,
				    "can't map global registers for gpio: %d\n",
				    error);
				return;
			}
			(*si->si_attach)(self, bst, bsh, svr);
			return;
		}
	}
	aprint_normal_dev(self,
	    "0 input groups, 0 output groups (unknown svr %#x)\n",
	    svr);
}
