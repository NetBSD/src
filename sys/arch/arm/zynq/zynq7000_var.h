/*	$NetBSD: zynq7000_var.h,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef _ARM_ZYNQ_ZYNQ7000_VAR_H_
#define _ARM_ZYNQ_ZYNQ7000_VAR_H_

#include <sys/bus.h>
#include <sys/cpu.h>

struct axi_attach_args {
	const char	*aa_name;
	bus_space_tag_t	aa_iot;
	bus_dma_tag_t	aa_dmat;
	bus_addr_t	aa_addr;
	bus_size_t	aa_size;
	int		aa_irq;
	int		aa_irqbase;
};

struct zynq7000_clock_info {
	uint32_t clk_ps;	/* Reference */
	uint32_t clk_arm_base;
	uint32_t clk_ddr_base;
	uint32_t clk_io_base;
	uint32_t clk_cpu_6x4x;	/* ARM PLL */
	uint32_t clk_cpu_3x2x;	/* ARM PLL */
	uint32_t clk_cpu_2x;	/* ARM PLL */
	uint32_t clk_cpu_1x;	/* ARM PLL */
	uint32_t clk_ddr_3x;	/* DDR PLL */
	uint32_t clk_ddr_2x;	/* DDR PLL */
	uint32_t clk_ddr_dci;	/* DDR PLL */
	uint32_t clk_smc;	/* IO PLL */
	uint32_t clk_qspi;	/* IO PLL */
	uint32_t clk_gige0;	/* IO PLL */
	uint32_t clk_gige1;	/* IO PLL */
	uint32_t clk_sdio;	/* IO PLL */
	uint32_t clk_uart;	/* IO PLL */
	uint32_t clk_spi;	/* IO PLL */
	uint32_t clk_can;	/* IO PLL */
	uint32_t clk_pcap;	/* IO PLL */
	uint32_t clk_dbg;	/* IO PLL */
	uint32_t clk_fclk0;	/* IO PLL */
	uint32_t clk_fclk1;	/* IO PLL */
	uint32_t clk_fclk2;	/* IO PLL */
	uint32_t clk_fclk3;	/* IO PLL */
};

struct cpu_softc {
	struct cpu_info *cpu_ci;
	bus_space_tag_t cpu_ioreg_bst;
	bus_space_tag_t cpu_armcore_bst;
	bus_space_handle_t cpu_ioreg_bsh;
	bus_space_handle_t cpu_armcore_bsh;

	struct zynq7000_clock_info cpu_clk;
};

/* Clock */
#define	ZYNQ7000_PS_CLK		(33333333) /* 33.333 MHz */

#ifdef _KERNEL
void zynq7000_bootstrap(vaddr_t);
psize_t	zynq7000_memprobe(void);
void zynq7000_device_register(device_t, void *);
void zynq7000_cpu_hatch(struct cpu_info *);

extern struct bus_space zynq_bs_tag;
extern struct arm32_bus_dma_tag zynq_bus_dma_tag;
extern struct zynq7000_clock_info clk_info;

extern bus_space_tag_t zynq7000_armcore_bst;
extern bus_space_handle_t zynq7000_armcore_bsh;
#endif

#endif /* _ARM_ZYNQ_ZYNQ7000_VAR_H_ */
