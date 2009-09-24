/* $NetBSD: haltwovar.h,v 1.6 2009/09/24 14:09:18 tsutsui Exp $ */

/*
 * Copyright (c) 2003 Ilpo Ruotsalainen
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#ifndef _ARCH_SGIMIPS_HPC_HALTWOVAR_H_
#define _ARCH_SGIMIPS_HPC_HALTWOVAR_H_

/* XXX Quite arbitrary? XXX */
#define HALTWO_MAX_DMASEGS	16

/* Mixer indices */
#define HALTWO_MASTER_VOL	0
#define HALTWO_OUTPUT_CLASS	1

struct haltwo_dmabuf {
	bus_dmamap_t dma_map;
	bus_dma_segment_t dma_segs[HALTWO_MAX_DMASEGS];
	int dma_segcount;
	size_t size;
	void *kern_addr;

	struct haltwo_dmabuf *next;
};

struct haltwo_codec {
	bus_dmamap_t dma_map;
	bus_dma_segment_t dma_seg;

	struct hpc_dma_desc *dma_descs;

	void (*intr)(void *);
	void *intr_arg;
};

struct haltwo_softc {
	struct device sc_dev;

	bus_space_tag_t sc_st;

	bus_space_handle_t sc_dma_sh;

	bus_dma_tag_t sc_dma_tag;

	struct haltwo_dmabuf *sc_dma_bufs;

	struct haltwo_codec sc_dac;
#if 0
	struct haltwo_codec sc_adc;
#endif
	uint8_t sc_vol_left;
	uint8_t sc_vol_right;

	bus_space_handle_t sc_ctl_sh;
	bus_space_handle_t sc_aes_sh;
	bus_space_handle_t sc_vol_sh;
	bus_space_handle_t sc_syn_sh;
};

#endif
