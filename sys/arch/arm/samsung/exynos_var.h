/* $NetBSD: exynos_var.h,v 1.2 2014/04/13 17:06:02 reinoud Exp $ */
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
#include <arm/samsung/exynos_reg.h>

//#include <dev/gpio/gpiovar.h>

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

#define IS_EXYNOS4_P()	(EXYNOS_PRODUCT_FAMILY(exynos_soc_id) == EXYNOS4_PRODUCT_FAMILY)

#define IS_EXYNOS5410_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe5410)
#define IS_EXYNOS5440_P()	(EXYNOS_PRODUCT_ID(exynos_soc_id) == 0xe5440)

#define IS_EXYNOS5_P()	(EXYNOS_PRODUCT_FAMILY(exynos_soc_id) == EXYNOS5_PRODUCT_FAMILY)

struct exyo_locators {
	const char *loc_name;
	bus_size_t loc_offset;
	bus_size_t loc_size;
	int loc_port;
	int loc_intr;
	int loc_flags;
};

#define EXYO_INTR_DEFAULT 0

#if 0
#define EXYO_E4410	__BIT(0)
#define EXYO_E4412	__BIT(1)
#define EXYO_E5440	__BIT(2)
#define EXYO_E5XXX	__BIT(3)

#define EXYO_ONLY	__BITS(7,0)
#define EXYO_ALL	__BITS(7,0)
#define EXYO_REQUIRED	__BIT(8)
#endif

struct exyo_attach_args {
	struct exyo_locators exyo_loc;
	bus_space_tag_t exyo_core_bst;
	bus_space_tag_t exyo_core_a4x_bst;
	bus_space_handle_t exyo_core_bsh;
	bus_dma_tag_t exyo_dmat;
	bus_dma_tag_t exyo_coherent_dmat;
};


extern struct bus_space exynos_bs_tag;
extern struct bus_space exynos_a4x_bs_tag;
extern bus_space_handle_t exynos_core_bsh;

void exynos_bootstrap(vaddr_t, vaddr_t);
void exynos_device_register(device_t self, void *aux);
void exyo_device_register(device_t self, void *aux);
void exynos_wdt_reset(void);

#endif	/* _ARM_SAMSUNG_EXYNOS_VAR_H_ */
