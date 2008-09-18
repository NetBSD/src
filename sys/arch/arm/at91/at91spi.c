/*	$Id: at91spi.c,v 1.2.4.2 2008/09/18 04:33:20 wrstuden Exp $	*/
/*	$NetBSD: at91spi.c,v 1.2.4.2 2008/09/18 04:33:20 wrstuden Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/mips/alchemy/dev/auspi.c,
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: at91spi.c,v 1.2.4.2 2008/09/18 04:33:20 wrstuden Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/vmparam.h>
#include <sys/inttypes.h>

#include <arm/at91/at91var.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91spivar.h>
#include <arm/at91/at91spireg.h>

#define	at91spi_select(sc, slave)	\
	(sc)->sc_md->select_slave((sc), (slave))

#define	STATIC

//#define	AT91SPI_DEBUG	4

#ifdef	AT91SPI_DEBUG
int at91spi_debug = AT91SPI_DEBUG;
#define	DPRINTFN(n,x)	if (at91spi_debug>(n)) printf x;
#else
#define	DPRINTFN(n,x)
#endif

STATIC int at91spi_intr(void *);

/* SPI service routines */
STATIC int at91spi_configure(void *, int, int, int);
STATIC int at91spi_transfer(void *, struct spi_transfer *);
STATIC void at91spi_xfer(struct at91spi_softc *sc, int start);

/* internal stuff */
STATIC void at91spi_done(struct at91spi_softc *, int);
STATIC void at91spi_send(struct at91spi_softc *);
STATIC void at91spi_recv(struct at91spi_softc *);
STATIC void at91spi_sched(struct at91spi_softc *);

#define	GETREG(sc, x)					\
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, x)
#define	PUTREG(sc, x, v)				\
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, x, v)

void
at91spi_attach_common(device_t parent, device_t self, void *aux,
		      at91spi_machdep_tag_t md)
{
	struct at91spi_softc *sc = device_private(self);
	struct at91bus_attach_args *sa = aux;
	struct spibus_attach_args sba;
	bus_dma_segment_t segs;
	int rsegs, err;

	aprint_normal(": AT91 SPI Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;
	sc->sc_pid = sa->sa_pid;
	sc->sc_dmat = sa->sa_dmat;
	sc->sc_md = md;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	/* we want to use dma, so allocate dma memory: */
	err = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, 0, PAGE_SIZE,
			       &segs, 1, &rsegs, BUS_DMA_WAITOK);
	if (err == 0) {
		err = bus_dmamem_map(sc->sc_dmat, &segs, 1, PAGE_SIZE,
				     &sc->sc_dmapage,
				     BUS_DMA_WAITOK);
	}
	if (err == 0) {
		err = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1,
					 PAGE_SIZE, 0, BUS_DMA_WAITOK,
					 &sc->sc_dmamap);
	}
	if (err == 0) {
		err = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
				      sc->sc_dmapage, PAGE_SIZE, NULL,
				      BUS_DMA_WAITOK);
	}
	if (err != 0) {
		panic("%s: Cannot get DMA memory", device_xname(sc->sc_dev));
	}
	sc->sc_dmaaddr = sc->sc_dmamap->dm_segs[0].ds_addr;

	/*
	 * Initialize SPI controller
	 */
	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = at91spi_configure;
	sc->sc_spi.sct_transfer = at91spi_transfer;

	//sc->sc_spi.sct_nslaves must have been initialized by machdep code
	if (!sc->sc_spi.sct_nslaves) {
		aprint_error("%s: no slaves!\n", device_xname(sc->sc_dev));
	}

	sba.sba_controller = &sc->sc_spi;

	/* initialize the queue */
	SIMPLEQ_INIT(&sc->sc_q);

	/* reset the SPI */
	at91_peripheral_clock(sc->sc_pid, 1);
	PUTREG(sc, SPI_CR, SPI_CR_SWRST);
	delay(100);

	/* be paranoid and make sure the PDC is dead */
	PUTREG(sc, SPI_PDC_BASE + PDC_PTCR, PDC_PTCR_TXTDIS | PDC_PTCR_RXTDIS);
	PUTREG(sc, SPI_PDC_BASE + PDC_RNCR, 0);
	PUTREG(sc, SPI_PDC_BASE + PDC_RCR, 0);
	PUTREG(sc, SPI_PDC_BASE + PDC_TNCR, 0);
	PUTREG(sc, SPI_PDC_BASE + PDC_TCR, 0);

	// configure SPI:
	PUTREG(sc, SPI_IDR, -1);
	PUTREG(sc, SPI_CSR(0), SPI_CSR_SCBR | SPI_CSR_BITS_8);
	PUTREG(sc, SPI_CSR(1), SPI_CSR_SCBR | SPI_CSR_BITS_8);
	PUTREG(sc, SPI_CSR(2), SPI_CSR_SCBR | SPI_CSR_BITS_8);
	PUTREG(sc, SPI_CSR(3), SPI_CSR_SCBR | SPI_CSR_BITS_8);
	PUTREG(sc, SPI_MR, SPI_MR_MODFDIS/* <- machdep? */ | SPI_MR_MSTR);

	/* enable device interrupts */
	sc->sc_ih = at91_intr_establish(sc->sc_pid, IPL_BIO, INTR_HIGH_LEVEL,
					at91spi_intr, sc);

	/* enable SPI */
	PUTREG(sc, SPI_CR, SPI_CR_SPIEN);
	if (GETREG(sc, SPI_SR) & SPI_SR_RDRF)
		(void)GETREG(sc, SPI_RDR);

	PUTREG(sc, SPI_PDC_BASE + PDC_PTCR, PDC_PTCR_TXTEN | PDC_PTCR_RXTEN);

	/* attach slave devices */
	(void) config_found_ia(sc->sc_dev, "spibus", &sba, spibus_print);
}

int
at91spi_configure(void *arg, int slave, int mode, int speed)
{
	struct at91spi_softc *sc = arg;
	uint		scbr;
	uint32_t	csr;

	/* setup interrupt registers */
	PUTREG(sc, SPI_IDR, -1);	/* disable interrupts for now	*/

	csr = GETREG(sc, SPI_CSR(0));	/* read register		*/
	csr &= SPI_CSR_RESERVED;	/* keep reserved bits		*/
	csr |= SPI_CSR_BITS_8;		/* assume 8 bit transfers	*/

	/*
	 * Calculate clock divider
	 */
	scbr = speed ? ((AT91_MSTCLK + speed - 1) / speed + 1) & ~1 : -1;
	if (scbr > 0xFF) {
		aprint_error("%s: speed %d not supported\n",
		    device_xname(sc->sc_dev), speed);
		return EINVAL;
	}
	csr |= scbr << SPI_CSR_SCBR_SHIFT;

	/*
	 * I'm not entirely confident that these values are correct.
	 * But at least mode 0 appears to work properly with the
	 * devices I have tested.  The documentation seems to suggest
	 * that I have the meaning of the clock delay bit inverted.
	 */
	switch (mode) {
	case SPI_MODE_0:
		csr |= SPI_CSR_NCPHA;		/* CPHA = 0, CPOL = 0 */
		break;
	case SPI_MODE_1:
		csr |= 0;			/* CPHA = 1, CPOL = 0 */
		break;
	case SPI_MODE_2:
		csr |= SPI_CSR_NCPHA		/* CPHA = 0, CPOL = 1 */
		       | SPI_CSR_CPOL;
		break;
	case SPI_MODE_3:
		csr |= SPI_CSR_CPOL;		/* CPHA = 1, CPOL = 1 */
		break;
	default:
		return EINVAL;
	}

	PUTREG(sc, SPI_CSR(0), csr);

	DPRINTFN(3, ("%s: slave %d mode %d speed %d, csr=0x%08"PRIX32"\n",
		     __FUNCTION__, slave, mode, speed, csr));

#if 0
	// wait until ready!?
	for (i = 1000000; i; i -= 10) {
		if (GETREG(sc, AUPSC_SPISTAT) & SPISTAT_DR) {
			return 0;
		}
	}

	return ETIMEDOUT;
#else
	return 0;
#endif
}

#define	HALF_BUF_SIZE	(PAGE_SIZE / 2)

void
at91spi_xfer(struct at91spi_softc *sc, int start)
{
	struct spi_chunk	*chunk;
	int			len;
	uint32_t		sr;

	DPRINTFN(3, ("%s: sc=%p start=%d\n", __FUNCTION__, sc, start));

	/* so ready to transmit more / anything received? */
	if (((sr = GETREG(sc, SPI_SR)) & (SPI_SR_ENDTX | SPI_SR_ENDRX)) != (SPI_SR_ENDTX | SPI_SR_ENDRX)) {
		/* not ready, get out */
		DPRINTFN(3, ("%s: sc=%p start=%d sr=%"PRIX32"\n", __FUNCTION__, sc, start, sr));
		return;
	}

	DPRINTFN(3, ("%s: sr=%"PRIX32"\n", __FUNCTION__, sr));

	if (!start) {
		// ok, something has been transfered, synchronize..
		int offs = sc->sc_dmaoffs ^ HALF_BUF_SIZE;
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, offs, HALF_BUF_SIZE,
				BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);

		if ((chunk = sc->sc_rchunk) != NULL) {
			if ((len = chunk->chunk_rresid) > HALF_BUF_SIZE)
				len = HALF_BUF_SIZE;
			if (chunk->chunk_rptr && len > 0) {
				memcpy(chunk->chunk_rptr, (const uint8_t *)sc->sc_dmapage + offs, len);
				chunk->chunk_rptr += len;
			}
			if ((chunk->chunk_rresid -= len) <= 0) {
				// done with this chunk, get next
				sc->sc_rchunk = chunk->chunk_next;
			}
		}
	}

	/* start transmitting next chunk: */
	if ((chunk = sc->sc_wchunk) != NULL) {

		/* make sure we transmit just half buffer at a time */
		len = MIN(chunk->chunk_wresid, HALF_BUF_SIZE);

		// setup outgoing data
		if (chunk->chunk_wptr && len > 0) {
			memcpy((uint8_t *)sc->sc_dmapage + sc->sc_dmaoffs, chunk->chunk_wptr, len);
			chunk->chunk_wptr += len;
		} else {
			memset((uint8_t *)sc->sc_dmapage + sc->sc_dmaoffs, 0, len);
		}

		/* advance to next transfer if it's time to */
		if ((chunk->chunk_wresid -= len) <= 0) {
			sc->sc_wchunk = sc->sc_wchunk->chunk_next;
		}

		/* determine which interrupt to get */
		if (sc->sc_wchunk) {
			/* just wait for next buffer to free */
			PUTREG(sc, SPI_IER, SPI_SR_ENDRX);
		} else {
			/* must wait until transfer has completed */
			PUTREG(sc, SPI_IDR, SPI_SR_ENDRX);
			PUTREG(sc, SPI_IER, SPI_SR_RXBUFF);
		}

		DPRINTFN(3, ("%s: dmaoffs=%d len=%d wchunk=%p (%p:%d) rchunk=%p (%p:%d) mr=%"PRIX32" sr=%"PRIX32" imr=%"PRIX32" csr0=%"PRIX32"\n",
			     __FUNCTION__, sc->sc_dmaoffs, len, sc->sc_wchunk,
			     sc->sc_wchunk ? sc->sc_wchunk->chunk_wptr : NULL,
			     sc->sc_wchunk ? sc->sc_wchunk->chunk_wresid : -1,
			     sc->sc_rchunk,
			     sc->sc_rchunk ? sc->sc_rchunk->chunk_rptr : NULL,
			     sc->sc_rchunk ? sc->sc_rchunk->chunk_rresid : -1,
			     GETREG(sc, SPI_MR), GETREG(sc, SPI_SR), 
			     GETREG(sc, SPI_IMR), GETREG(sc, SPI_CSR(0))));

		// prepare DMA
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, sc->sc_dmaoffs, len,
				BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		// and start transmitting / receiving
		PUTREG(sc, SPI_PDC_BASE + PDC_RNPR, sc->sc_dmaaddr + sc->sc_dmaoffs);
		PUTREG(sc, SPI_PDC_BASE + PDC_RNCR, len);
		PUTREG(sc, SPI_PDC_BASE + PDC_TNPR, sc->sc_dmaaddr + sc->sc_dmaoffs);
		PUTREG(sc, SPI_PDC_BASE + PDC_TNCR, len);

		// swap buffer
		sc->sc_dmaoffs ^= HALF_BUF_SIZE;

		// get out
		return;
	} else {
		DPRINTFN(3, ("%s: nothing to write anymore\n", __FUNCTION__));
		return;
	}
}

void
at91spi_sched(struct at91spi_softc *sc)
{
	struct spi_transfer	*st;
	int			err;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {

		DPRINTFN(2, ("%s: st=%p\n", __FUNCTION__, st));

		/* remove the item */
		spi_transq_dequeue(&sc->sc_q);

		/* note that we are working on it */
		sc->sc_transfer = st;

		if ((err = at91spi_select(sc, st->st_slave)) != 0) {
			spi_done(st, err);
			continue;
		}

		/* setup chunks */
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;

		/* now kick the master start to get the chip running */
		at91spi_xfer(sc, TRUE);

		/* enable error interrupts too: */
		PUTREG(sc, SPI_IER, SPI_SR_MODF | SPI_SR_OVRES);

		sc->sc_running = TRUE;
		return;
	}
	DPRINTFN(2, ("%s: nothing to do anymore\n", __FUNCTION__));
	PUTREG(sc, SPI_IDR, -1);	/* disable interrupts */
	at91spi_select(sc, -1);
	sc->sc_running = FALSE;
}

void
at91spi_done(struct at91spi_softc *sc, int err)
{
	struct spi_transfer	*st;

	/* called from interrupt handler */
	if ((st = sc->sc_transfer) != NULL) {
		sc->sc_transfer = NULL;
		DPRINTFN(2, ("%s: st %p finished with error code %d\n", __FUNCTION__, st, err));
		spi_done(st, err);
	}
	/* make sure we clear these bits out */
	sc->sc_wchunk = sc->sc_rchunk = NULL;
	at91spi_sched(sc);
}

int
at91spi_intr(void *arg)
{
	struct at91spi_softc	*sc = arg;
	uint32_t		imr, sr;
	int			err = 0;

	if ((imr = GETREG(sc, SPI_IMR)) == 0) {
		/* interrupts are not enabled, get out */
		DPRINTFN(4, ("%s: interrupts are not enabled\n", __FUNCTION__));
		return 0;
	}

	sr = GETREG(sc, SPI_SR);
	if (!(sr & imr)) {
		/* interrupt did not happen, get out */
		DPRINTFN(3, ("%s: interrupts are not enabled, sr=%08"PRIX32" imr=%08"PRIX32"\n",
			     __FUNCTION__, sr, imr));
		return 0;
	}

	DPRINTFN(3, ("%s: sr=%08"PRIX32" imr=%08"PRIX32"\n",
		     __FUNCTION__, sr, imr));

	if (sr & imr & SPI_SR_MODF) {
		printf("%s: mode fault!\n", device_xname(sc->sc_dev));
		err = EIO;
	}

	if (sr & imr & SPI_SR_OVRES) {
		printf("%s: overrun error!\n", device_xname(sc->sc_dev));
		err = EIO;
	}
	if (err) {
		/* clear errors */
		/* complete transfer */
		at91spi_done(sc, err);
	} else {
		/* do all data exchanges */
		at91spi_xfer(sc, FALSE);

		/*
		 * if the master done bit is set, make sure we do the
		 * right processing.
		 */
		if (sr & imr & SPI_SR_RXBUFF) {
			if ((sc->sc_wchunk != NULL) ||
			    (sc->sc_rchunk != NULL)) {
				printf("%s: partial transfer?\n",
				    device_xname(sc->sc_dev));
				err = EIO;
			} 
			at91spi_done(sc, err);
		}

	}

	return 1;
}

int
at91spi_transfer(void *arg, struct spi_transfer *st)
{
	struct at91spi_softc	*sc = arg;
	int			s;

	/* make sure we select the right chip */
	s = splbio();
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == 0) {
		at91spi_sched(sc);
	}
	splx(s);
	return 0;
}

