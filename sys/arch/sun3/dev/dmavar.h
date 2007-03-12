/*	$NetBSD: dmavar.h,v 1.7.2.1 2007/03/12 05:51:05 rmind Exp $ */

/*
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
 * 4. The name of the author may not be used to endorse or promote products
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
 */

struct dma_softc {
	struct device sc_dev;			/* us as a device */
	bus_space_tag_t sc_bst;			/* bus space tag */
	bus_dma_tag_t sc_dmatag;		/* bus dma tag */

	bus_space_handle_t sc_bsh;		/* bus space handle */
	void *sc_client;			/* my client */

	int	sc_active;			/* DMA active ? */
	bus_dmamap_t sc_dmamap;			/* bus dma map */
	u_int	sc_rev;				/* revision */
	int	sc_burst;			/* DVMA burst size in effect */
	size_t	sc_dmasize;
	void 	**sc_dmaaddr;
	size_t  *sc_dmalen;
#if 0
	void (*reset)(struct dma_softc *);	/* reset routine */
	int (*intr)(struct dma_softc *);	/* interrupt ! */
	int (*setup)(struct dma_softc *, void **, size_t *, int, size_t *);
#endif
};

#define DMA_GCSR(sc)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, DMA_REG_CSR)
#define DMA_SCSR(sc, csr)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, DMA_REG_CSR, (csr))

/*
 * DMA engine interface functions.
 */
#if 0
#define DMA_RESET(sc)			(((sc)->reset)(sc))
#define DMA_INTR(sc)			(((sc)->intr)(sc))
#define DMA_SETUP(sc, a, l, d, s)	(((sc)->setup)(sc, a, l, d, s))
#endif

#define DMA_ISACTIVE(sc)		((sc)->sc_active)

#define DMA_ENINTR(sc) do {			\
	uint32_t _csr = DMA_GCSR(sc);		\
	_csr |= D_INT_EN;			\
	DMA_SCSR(sc, _csr);			\
} while (/* CONSTCOND */0)

#define DMA_ISINTR(sc)	(DMA_GCSR(sc) & (D_INT_PEND|D_ERR_PEND))

#define DMA_GO(sc) do {				\
	uint32_t _csr = DMA_GCSR(sc);		\
	_csr |= D_EN_DMA;			\
	DMA_SCSR(sc, _csr);			\
	sc->sc_active = 1;			\
} while (/* CONSTCOND */0)

#define DMA_STOP(sc) do {			\
	uint32_t _csr = DMA_GCSR(sc);		\
	_csr &= ~D_EN_DMA;			\
	DMA_SCSR(sc, _csr);			\
	sc->sc_active = 0;			\
} while (/* CONSTCOND */0)

struct dma_softc *espdmafind(int);
int espdmaintr(struct dma_softc *);

void dma_reset(struct dma_softc *);
int  dma_setup(struct dma_softc *, void **, size_t *, int, size_t *);

