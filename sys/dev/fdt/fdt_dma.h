/*	$NetBSD: fdt_dma.h,v 1.1 2025/09/06 20:11:30 thorpej Exp $	*/

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */     

#ifndef _DEV_FDT_FDT_DMA_H_
#define	_DEV_FDT_FDT_DMA_H_

#include <sys/bus.h>
#include <sys/device.h>

struct fdtbus_dma_controller;

struct fdtbus_dma {
	struct fdtbus_dma_controller *dma_dc;
	void *dma_priv;
};

enum fdtbus_dma_dir {
	FDT_DMA_READ,		/* device -> memory */
	FDT_DMA_WRITE		/* memory -> device */
};

struct fdtbus_dma_opt {
	int opt_bus_width;	/* Bus width */
	int opt_burst_len;	/* Burst length */
	int opt_swap;		/* Enable data swapping */
	int opt_dblbuf;		/* Enable double buffering */
	int opt_wrap_len;	/* Address wrap-around window */
};

struct fdtbus_dma_req {
	bus_dma_segment_t *dreq_segs;	/* Memory */
	int dreq_nsegs;

	bus_addr_t dreq_dev_phys;	/* Device */
	int dreq_sel;			/* Device selector */

	enum fdtbus_dma_dir dreq_dir;	/* Transfer direction */

	int dreq_block_irq;		/* Enable IRQ at end of block */
	int dreq_block_multi;		/* Enable multiple block transfers */
	int dreq_flow;			/* Enable flow control */

	struct fdtbus_dma_opt dreq_mem_opt;	/* Memory options */
	struct fdtbus_dma_opt dreq_dev_opt;	/* Device options */
};

struct fdtbus_dma_controller_func {
	void *	(*acquire)(device_t, const void *, size_t,
			   void (*)(void *), void *);
	void	(*release)(device_t, void *);
	int	(*transfer)(device_t, void *, struct fdtbus_dma_req *);
	void	(*halt)(device_t, void *);
};

int		fdtbus_register_dma_controller(device_t, int,
		    const struct fdtbus_dma_controller_func *);

struct fdtbus_dma *
		fdtbus_dma_get(int, const char *, void (*)(void *), void *);
struct fdtbus_dma *
		fdtbus_dma_get_index(int, u_int, void (*)(void *), void *);
void		fdtbus_dma_put(struct fdtbus_dma *);
int		fdtbus_dma_transfer(struct fdtbus_dma *,
		    struct fdtbus_dma_req *);
void		fdtbus_dma_halt(struct fdtbus_dma *);

#endif /* _DEV_FDT_FDT_DMA_H_ */
