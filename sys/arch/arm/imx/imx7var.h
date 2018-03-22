/*	$NetBSD: imx7var.h,v 1.2.4.1 2018/03/22 01:44:42 pgoyette Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX7VAR_H
#define _ARM_IMX_IMX7VAR_H

#include <sys/cdefs.h>

struct axi_attach_args {
	const char *aa_name;
	bus_space_tag_t aa_iot;
	bus_dma_tag_t aa_dmat;
	bus_addr_t aa_addr;
	bus_size_t aa_size;
	int aa_irq;
	int aa_irqbase;
};

extern struct bus_space armv7_generic_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern bus_space_tag_t imx7_armcore_bst;
extern bus_space_handle_t imx7_armcore_bsh;

/* iomux utility functions in imx7_iomux.c */
struct iomux_conf {
	uint32_t muxaddr;
	uint32_t padaddr;
	uint32_t inputaddr;
	uint32_t mux:8,
	         pad:8,
	         input:8,
	         none:8;
};

#define IOMUXCONF_MUX(name, muxval)					\
	{ .muxaddr = IOMUXC_SW_MUX_CTL_PAD_##name,			\
	  .mux = muxval							\
	}
#define IOMUXCONF_PAD(name, padval)					\
	{ .padaddr = IOMUXC_SW_PAD_CTL_PAD_##name,			\
	  .pad = padval							\
	}
#define IOMUXCONF_INPUT(name, inputval)					\
	{ .inputaddr = IOMUXC_##name##_SELECT_INPUT,			\
	  .input = inputval						\
	}
#define IOMUXCONF_MUX_PAD(name, muxval, padval)				\
	{ .muxaddr = IOMUXC_SW_MUX_CTL_PAD_##name,			\
	  .padaddr = IOMUXC_SW_PAD_CTL_PAD_##name,			\
	  .mux = muxval,						\
	  .pad = padval							\
	}
#define IOMUXCONF_MUX_INPUT(name, muxval, inputval)			\
	{ .muxaddr = IOMUXC_SW_MUX_CTL_PAD_##name,			\
	  .inputaddr = IOMUXC_##name##_SELECT_INPUT,			\
	  .mux = muxval,						\
	  .input = inputval						\
	}
#define IOMUXCONF_PAD_INPUT(name, padval, inputval)			\
	{ .padaddr = IOMUXC_SW_MUX_CTL_PAD_##name,			\
	  .inputaddr = IOMUXC_##name##_SELECT_INPUT,			\
	  .pad = padval,						\
	  .input = inputval						\
	}
#define IOMUXCONF_MUX_PAD_INPUT(name, muxval, padval, inputval)		\
	{ .muxaddr = IOMUXC_SW_MUX_CTL_PAD_##name,			\
	  .padaddr = IOMUXC_SW_PAD_CTL_PAD_##name,			\
	  .inputaddr = IOMUXC_##name##_SELECT_INPUT,			\
	  .mux = muxval,						\
	  .pad = padval,						\
	  .input = inputval						\
	}

#define IOMUXLPSRCONF_MUX(name, muxval)					\
	{ .muxaddr = IOMUXC_LPSR_SW_MUX_CTL_PAD_##name,			\
	  .mux = muxval							\
	}
#define IOMUXLPSRCONF_PAD(name, padval)					\
	{ .padaddr = IOMUXC_LPSR_SW_PAD_CTL_PAD_##name,			\
	  .pad = padval							\
	}
#define IOMUXLPSRCONF_MUX_PAD(name, muxval, padval)			\
	{ .muxaddr = IOMUXC_LPSR_SW_MUX_CTL_PAD_##name,			\
	  .padaddr = IOMUXC_LPSR_SW_PAD_CTL_PAD_##name,			\
	  .mux = muxval,						\
	  .pad = padval							\
	}


uint32_t iomux_read(uint32_t);
void iomux_write(uint32_t, uint32_t);
void iomux_set_function(u_int, u_int);
void iomux_set_pad(u_int, u_int);
void iomux_set_input(u_int, u_int);
void iomux_mux_config(const struct iomux_conf *);

/* system-reset register read/write in imx7_src.c */
uint32_t imxsrc_read(uint32_t);
void imxsrc_write(uint32_t, uint32_t);

/* imx7_board.c */
void imx7_bootstrap(vaddr_t);
uint32_t imx7_armrootclk(void);
void imx7_reset(void) __dead;
void imx7_device_register(device_t, void *);
void imx7_cpu_hatch(struct cpu_info *);

uint32_t imx7_chip_id(void);
#define CHIPID_MINOR_MASK		0x0000ffff
#define CHIPID_MAJOR_MASK		0xffff0000
#define CHIPID_MAJOR_IMX7D		0x00720000
#define IMX7_CHIPID_MAJOR(v)		((v) & CHIPID_MAJOR_MASK)
#define IMX7_CHIPID_MINOR(v)		((v) & CHIPID_MINOR_MASK)

#endif /* _ARM_IMX_IMX7VAR_H */
