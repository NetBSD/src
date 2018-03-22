/*	$NetBSD: imx51var.h,v 1.5.2.1 2018/03/22 01:44:42 pgoyette Exp $ */

/*
 * Copyright (c) 2015 Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX51VAR_H
#define	_ARM_IMX_IMX51VAR_H

struct imxfb_attach_args {
	void			*ifb_fb;
	bus_dma_tag_t		 ifb_dmat;
	bus_dmamap_t		 ifb_dmamap;
	bus_dma_segment_t	*ifb_dmasegs;
	int			 ifb_ndmasegs;
	u_int			 ifb_width;
	u_int			 ifb_height;
	u_int			 ifb_depth;
	u_int			 ifb_stride;
	device_t		 ifb_outputdev;
};

void	imx_genfb_set_videomode(device_t, u_int, u_int);

extern struct bus_space armv7_generic_bs_tag;
extern struct bus_space armv7_generic_a4x_bs_tag;
extern struct arm32_bus_dma_tag arm_generic_dma_tag;
extern struct arm32_bus_dma_tag imx_bus_dma_tag;

void gpio_set_direction(uint32_t, uint32_t);
void gpio_data_write(uint32_t, uint32_t);
bool gpio_data_read(uint32_t);

struct axi_attach_args {
	const char	*aa_name;
	bus_space_tag_t	aa_iot;
	bus_dma_tag_t	aa_dmat;
	bus_addr_t	aa_addr;
	bus_size_t	aa_size;
	int		aa_irq;
	int		aa_irqbase;
};


/* iomux utility functions */
struct iomux_conf {
	u_int pin;
#define	IOMUX_CONF_EOT	((u_int)(-1))
	u_short mux;
	u_short pad;
};

void iomux_set_function(u_int, u_int);
void iomux_set_pad(u_int, u_int);
//void iomux_set_input(u_int, u_int);
void iomux_mux_config(const struct iomux_conf *);

#endif	/* _ARM_IMX_IMX51VAR_H */
