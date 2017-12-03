/*	$NetBSD: imx6var.h,v 1.7.2.2 2017/12/03 11:35:53 jdolecek Exp $	*/

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

#ifndef _ARM_IMX_IMX6VAR_H
#define _ARM_IMX_IMX6VAR_H

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
extern struct arm32_bus_dma_tag armv7_generic_dma_tag;
extern bus_space_tag_t imx6_armcore_bst;
extern bus_space_handle_t imx6_armcore_bsh;

/* iomux utility functions in imx6_iomux.c */
struct iomux_conf {
	uint32_t pin;	/* ((MUXADDR<<16)|PADADDR) */
#define IOMUX_CONF_EOT	((uint32_t)(-1))
	uint32_t mux;
	uint32_t pad;
};

uint32_t iomux_read(uint32_t);
void iomux_write(uint32_t, uint32_t);
void iomux_set_function(u_int, u_int);
void iomux_set_pad(u_int, u_int);
void iomux_set_input(u_int, u_int);
void iomux_mux_config(const struct iomux_conf *);

/* imx6_board.c */
void imx6_bootstrap(vaddr_t);
psize_t imx6_memprobe(void);
uint32_t imx6_armrootclk(void);
void imx6_reset(void) __dead;
void imx6_device_register(device_t, void *);
void imx6_cpu_hatch(struct cpu_info *);
void imx6_set_gpio(device_t, const char *, int32_t *, int32_t *, u_int);
uint32_t imx6_chip_id(void);
#define CHIPID_MINOR_MASK		0x000000ff
#define CHIPID_MAJOR_MASK		0x00ffff00
#define CHIPID_MAJOR_IMX6SL		0x00600000
#define CHIPID_MAJOR_IMX6DL		0x00610000
#define CHIPID_MAJOR_IMX6SOLO		0x00620000
#define CHIPID_MAJOR_IMX6Q		0x00630000
#define CHIPID_MAJOR_IMX6UL		0x00640000
#define IMX6_CHIPID_MAJOR(v)		((v) & CHIPID_MAJOR_MASK)
#define IMX6_CHIPID_MINOR(v)		((v) & CHIPID_MINOR_MASK)

#endif /* _ARM_IMX_IMX6VAR_H */
