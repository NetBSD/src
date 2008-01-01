/*	$NetBSD: iopaauvar.h,v 1.4.92.1 2008/01/01 15:39:46 chris Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _XSCALE_IOPAAUVAR_H_
#define	_XSCALE_IOPAAUVAR_H_

#include <sys/pool.h>
#include <dev/dmover/dmovervar.h>

#define	AAU_MAX_INPUTS		8

/*
 * Due to the way the AAU's descriptors work, the DMA segments for
 * the inputs and output must all be the same length.  For now, we
 * will enforce this by requiring all I/O to be page aligned, and
 * and not let it cross a page boundary.
 *
 * We could easily shrink this to any power of two.
 */
#define	AAU_IO_BOUNDARY		4096

struct iopaau_softc {
	struct device sc_dev;
	struct dmover_backend sc_dmb;

	struct dmover_request *sc_running;

	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;
	bus_dma_tag_t sc_dmat;

	void *sc_firstdesc;
	void *sc_lastdesc;
	uint32_t sc_firstdesc_pa;

	bus_dmamap_t sc_map_out;
	bus_dmamap_t sc_map_in[AAU_MAX_INPUTS];
};

struct iopaau_function {
	int	(*af_setup)(struct iopaau_softc *, struct dmover_request *);
	pool_cache_t af_desc_cache;
};

extern pool_cache_t iopaau_desc_4_cache;
extern pool_cache_t iopaau_desc_8_cache;

void	iopaau_attach(struct iopaau_softc *);
void	iopaau_process(struct dmover_backend *);
int	iopaau_intr(void *);

int	iopaau_func_zero_setup(struct iopaau_softc *,
	    struct dmover_request *);
int	iopaau_func_fill8_setup(struct iopaau_softc *,
	    struct dmover_request *);
int	iopaau_func_xor_setup(struct iopaau_softc *,
	    struct dmover_request *);

void	iopaau_desc_free(struct pool_cache *, void *);

#endif /* _XSCALE_IOPAAUVAR_H_ */
