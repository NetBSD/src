/* $NetBSD: gicv3.h,v 1.4 2018/11/10 11:46:31 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_CORTEX_GICV3_H
#define _ARM_CORTEX_GICV3_H

#include <sys/intr.h>

struct gicv3_dma {
	bus_dma_segment_t	segs[1];
	bus_dmamap_t		map;
	uint8_t			*base;
	bus_size_t		len;
};

struct gicv3_lpi_callback {
	void			(*cpu_init)(void *, struct cpu_info *);
	void			(*get_affinity)(void *, size_t, kcpuset_t *);
	int			(*set_affinity)(void *, size_t, const kcpuset_t *);

	void			*priv;

	LIST_ENTRY(gicv3_lpi_callback) list;
};

struct gicv3_softc {
	struct pic_softc	sc_pic;		/* SGI/PPI/SGIs */
	struct pic_softc	sc_lpi;		/* LPIs */
	device_t		sc_dev;

	bus_space_tag_t		sc_bst;
	bus_dma_tag_t		sc_dmat;

	bus_space_handle_t	sc_bsh_d;	/* GICD */
	bus_space_handle_t	*sc_bsh_r;	/* GICR */
	u_int			sc_bsh_r_count;

	uint32_t		sc_enabled_sgippi;
	uint64_t		sc_irouter[MAXCPUS];

	/* LPI configuration table */
	struct gicv3_dma	sc_lpiconf;

	/* LPI pending tables */
	struct gicv3_dma	sc_lpipend[MAXCPUS];

	/* Unique identifier for PEs */
	u_int			sc_processor_id[MAXCPUS];

	/* Callbacks */
	LIST_HEAD(, gicv3_lpi_callback) sc_lpi_callbacks;
};

int	gicv3_init(struct gicv3_softc *);
void	gicv3_dma_alloc(struct gicv3_softc *, struct gicv3_dma *, bus_size_t, bus_size_t);
void	gicv3_irq_handler(void *);

#endif /* _ARM_CORTEX_GICV3_H */
