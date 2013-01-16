/*	$NetBSD: pxa2x0_i2s.h,v 1.2.2.3 2013/01/16 05:32:51 yamt Exp $	*/
/*	$OpenBSD: pxa2x0_i2s.h,v 1.3 2006/04/04 11:45:40 pascoe Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _PXA2X0_I2S_H_
#define _PXA2X0_I2S_H_

#include <sys/bus.h>

struct pxa2x0_i2s_dma;
struct audio_params;

struct pxa2x0_i2s_softc {
	kmutex_t *sc_intr_lock;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_size;
	bus_dma_tag_t sc_dmat;

	int sc_open;
	uint32_t sc_sadiv;

	struct pxa2x0_i2s_dma *sc_dmas;

	/* Dummy DMA segment which points to the I2S SADR */
	bus_dma_segment_t sc_dr;

	/* PCM Output (Tx) state */
	struct pxa2x0_i2s_dma *sc_txdma;
	void (*sc_txfunc)(void *);
	void *sc_txarg;

	/* PCM Input (Rx) state */
	struct pxa2x0_i2s_dma *sc_rxdma;
	void (*sc_rxfunc)(void *);
	void *sc_rxarg;
};

void	pxa2x0_i2s_init(struct pxa2x0_i2s_softc *sc);
int	pxa2x0_i2s_attach_sub(struct pxa2x0_i2s_softc *);
int	pxa2x0_i2s_detach_sub(struct pxa2x0_i2s_softc *);
void	pxa2x0_i2s_open(struct pxa2x0_i2s_softc *);
void	pxa2x0_i2s_close(struct pxa2x0_i2s_softc *);
void	pxa2x0_i2s_write(struct pxa2x0_i2s_softc *, uint32_t);

void	pxa2x0_i2s_setspeed(struct pxa2x0_i2s_softc *, u_int *);

void *	pxa2x0_i2s_allocm(void *, int, size_t);
void	pxa2x0_i2s_freem(void  *, void *, size_t);
paddr_t	pxa2x0_i2s_mappage(void *, void *, off_t, int);
int	pxa2x0_i2s_round_blocksize(void *, int, int, const struct audio_params *);
size_t	pxa2x0_i2s_round_buffersize(void *, int, size_t);
int	pxa2x0_i2s_halt_output(void *);
int	pxa2x0_i2s_halt_input(void *);
int	pxa2x0_i2s_start_output(void *, void *, int, void (*)(void *), void *);
int	pxa2x0_i2s_start_input(void *, void *, int, void (*)(void *), void *);

#endif
