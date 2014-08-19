/*	$NetBSD: bcm53xx_var.h,v 1.1.2.3 2014/08/20 00:02:45 tls Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

#ifndef _ARM_BROADCOM_BCM53XX_VAR_H_
#define _ARM_BROADCOM_BCM53XX_VAR_H_

#include <sys/bus.h>
#include <sys/cpu.h>

struct bcm_locators {
	const char *loc_name;
	bus_size_t loc_offset;
	bus_size_t loc_size;
	int loc_port;
	uint8_t loc_nintrs;
	u_int loc_intrs[8];
	int loc_mdio;
	int loc_phy;
};

struct bcmccb_attach_args {
	bus_space_tag_t ccbaa_ccb_bst;
	bus_space_handle_t ccbaa_ccb_bsh;
	bus_dma_tag_t ccbaa_dmat;

	struct bcm_locators ccbaa_loc;
};

struct bcm53xx_clock_info {
	uint32_t clk_ref;	// 25MHZ
	uint32_t clk_sys;	// 200MHZ
	uint32_t clk_ddr;
	uint32_t clk_ddr_mhz;
	uint32_t clk_cpu;
	uint32_t clk_apb;
	uint32_t clk_lcpll;	// LCPLL out
	uint32_t clk_pcie_ref;	// LCPLL CH0
	uint32_t clk_sdio;	// LCPLL CH1
	uint32_t clk_ddr_ref;	// LCPLL CH2
	uint32_t clk_axi;	// LCPLL CH3
	uint32_t clk_genpll;	// GENPLL out
	uint32_t clk_mac;	// GENPLL CH0
	uint32_t clk_robo;	// GENPLL CH1
	uint32_t clk_usb2;	// GENPLL CH2
	uint32_t clk_iproc;	// GENPLL CH3
	uint32_t clk_usb_ref;	// 1920MHz USB Refernce Clock
};

/*
 * Used by BCMETH (gmac)
 */
struct bcmmdio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t *sc_lock;
};

int	bcmmdio_mii_readreg(device_t, int, int);
void	bcmmdio_mii_writereg(device_t, int, int, int);


struct cpu_softc {
	struct cpu_info *cpu_ci;
	bus_space_tag_t cpu_ioreg_bst;
	bus_space_tag_t cpu_armcore_bst;
	bus_space_handle_t cpu_ioreg_bsh;
	bus_space_handle_t cpu_armcore_bsh;

	struct bcm53xx_clock_info cpu_clk;
	uint32_t cpu_chipid;
};


#ifdef _KERNEL
void	bcm53xx_bootstrap(vaddr_t);
void	bcm53xx_cpu_hatch(struct cpu_info *);
void	bcm53xx_cpu_softc_init(struct cpu_info *);
bool	bcm53xx_idm_device_init(const struct bcm_locators *, bus_space_tag_t,
	    bus_space_handle_t);
void	bcm53xx_device_register(device_t, void *);
psize_t	bcm53xx_memprobe(void);
void	bcm53xx_dma_bootstrap(psize_t);
void	bcm53xx_print_clocks(void);
void	bcm53xx_rng_start(bus_space_tag_t, bus_space_handle_t);
void	bcm53xx_srab_init(void);
uint32_t	bcm53xx_srab_read_4(u_int);
uint64_t	bcm53xx_srab_read_8(u_int);
void	bcm53xx_srab_write_4(u_int, uint32_t);
void	bcm53xx_srab_write_8(u_int, uint64_t);
extern struct arm32_bus_dma_tag bcm53xx_dma_tag, bcm53xx_coherent_dma_tag;
#ifdef _ARM32_NEED_BUS_DMA_BOUNCE
extern struct arm32_bus_dma_tag bcm53xx_bounce_dma_tag;
#endif
extern struct bus_space bcmgen_bs_tag;
extern bus_space_tag_t bcm53xx_ioreg_bst;
extern bus_space_handle_t bcm53xx_ioreg_bsh;
extern bus_space_tag_t bcm53xx_armcore_bst;
extern bus_space_handle_t bcm53xx_armcore_bsh;
#endif

#endif /* _ARM_BROADCOM_BCM53XX_VAR_H_ */
