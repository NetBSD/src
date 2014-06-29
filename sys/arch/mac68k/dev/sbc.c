/*	$NetBSD: sbc.c,v 1.56 2014/06/29 12:18:42 martin Exp $	*/

/*
 * Copyright (C) 1996 Scott Reynolds.  All rights reserved.
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
 *    derived from this software without specific prior written permission
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

/*
 * This file contains only the machine-dependent parts of the mac68k
 * NCR 5380 SCSI driver.  (Autoconfig stuff and PDMA functions.)
 * The machine-independent parts are in ncr5380sbc.c
 *
 * Supported hardware includes:
 * Macintosh II family 5380-based controller
 *
 * Credits, history:
 *
 * Scott Reynolds wrote this module, based on work by Allen Briggs
 * (mac68k), Gordon W. Ross and David Jones (sun3), and Leo Weppelman
 * (atari).  Thanks to Allen for supplying crucial interpretation of the
 * NetBSD/mac68k 1.1 'ncrscsi' driver.  Also, Allen, Gordon, and Jason
 * Thorpe all helped to refine this code, and were considerable sources
 * of moral support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbc.c,v 1.56 2014/06/29 12:18:42 martin Exp $");

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/cpu.h>
#include <machine/viareg.h>

#include <mac68k/dev/sbcreg.h>
#include <mac68k/dev/sbcvar.h>

/* SBC_DEBUG --  relies on DDB */
#ifdef SBC_DEBUG
# define	SBC_DB_INTR	0x01
# define	SBC_DB_DMA	0x02
# define	SBC_DB_REG	0x04
# define	SBC_DB_BREAK	0x08
# ifndef DDB
#  define	Debugger()	printf("Debug: sbc.c:%d\n", __LINE__)
# endif
# define	SBC_BREAK \
		do { if (sbc_debug & SBC_DB_BREAK) Debugger(); } while (0)
#else
# define	SBC_BREAK
#endif


int	sbc_debug = 0 /* | SBC_DB_INTR | SBC_DB_DMA */;
int	sbc_link_flags = 0 /* | SDEV_DB2 */;
int	sbc_options = 0 /* | SBC_PDMA */;

extern label_t	*nofault;
extern void *	m68k_fault_addr;

static	int	sbc_wait_busy(struct ncr5380_softc *);
static	int	sbc_ready(struct ncr5380_softc *);
static	int	sbc_wait_dreq(struct ncr5380_softc *);


/***
 * General support for Mac-specific SCSI logic.
 ***/

/* These are used in the following inline functions. */
int sbc_wait_busy_timo = 1000 * 5000;	/* X2 = 10 S. */
int sbc_ready_timo = 1000 * 5000;	/* X2 = 10 S. */
int sbc_wait_dreq_timo = 1000 * 5000;	/* X2 = 10 S. */

/* Return zero on success. */
static inline int
sbc_wait_busy(struct ncr5380_softc *sc)
{
	int timo = sbc_wait_busy_timo;
	for (;;) {
		if (SCI_BUSY(sc)) {
			timo = 0;	/* return 0 */
			break;
		}
		if (--timo < 0)
			break;	/* return -1 */
		delay(2);
	}
	return (timo);
}

static inline int
sbc_ready(struct ncr5380_softc *sc)
{
	int timo = sbc_ready_timo;

	for (;;) {
		if ((*sc->sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH))
		    == (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			timo = 0;
			break;
		}
		if (((*sc->sci_csr & SCI_CSR_PHASE_MATCH) == 0)
		    || (SCI_BUSY(sc) == 0)) {
			timo = -1;
			break;
		}
		if (--timo < 0)
			break;	/* return -1 */
		delay(2);
	}
	return (timo);
}

static inline int
sbc_wait_dreq(struct ncr5380_softc *sc)
{
	int timo = sbc_wait_dreq_timo;

	for (;;) {
		if ((*sc->sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH))
		    == (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			timo = 0;
			break;
		}
		if (--timo < 0)
			break;	/* return -1 */
		delay(2);
	}
	return (timo);
}

void
sbc_irq_intr(void *p)
{
	struct ncr5380_softc *ncr_sc = p;
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	int claimed = 0;

	/* How we ever arrive here without IRQ set is a mystery... */
	if (*ncr_sc->sci_csr & SCI_CSR_INT) {
#ifdef SBC_DEBUG
		if (sbc_debug & SBC_DB_INTR)
			decode_5380_intr(ncr_sc);
#endif
		if (!cold)
			claimed = ncr5380_intr(ncr_sc);
		if (!claimed) {
			if (((*ncr_sc->sci_csr & ~SCI_CSR_PHASE_MATCH) == SCI_CSR_INT)
			    && ((*ncr_sc->sci_bus_csr & ~SCI_BUS_RST) == 0)) {
				SCI_CLR_INTR(ncr_sc);	/* RST interrupt */
				if (sc->sc_clrintr)
					(*sc->sc_clrintr)(ncr_sc);
			}
#ifdef SBC_DEBUG
			else {
				printf("%s: spurious intr\n",
				    device_xname(ncr_sc->sc_dev));
				SBC_BREAK;
			}
#endif
		}
	}
}

#ifdef SBC_DEBUG
void
decode_5380_intr(struct ncr5380_softc *ncr_sc)
{
	u_int8_t csr = *ncr_sc->sci_csr;
	u_int8_t bus_csr = *ncr_sc->sci_bus_csr;

	if (((csr & ~(SCI_CSR_PHASE_MATCH | SCI_CSR_ATN)) == SCI_CSR_INT) &&
	    ((bus_csr & ~(SCI_BUS_MSG | SCI_BUS_CD | SCI_BUS_IO | SCI_BUS_DBP)) == SCI_BUS_SEL)) {
		if (csr & SCI_BUS_IO)
			printf("%s: reselect\n", device_xname(ncr_sc->sc_dev));
		else
			printf("%s: select\n", device_xname(ncr_sc->sc_dev));
	} else if (((csr & ~SCI_CSR_ACK) == (SCI_CSR_DONE | SCI_CSR_INT)) &&
	    ((bus_csr & (SCI_BUS_RST | SCI_BUS_BSY | SCI_BUS_SEL)) == SCI_BUS_BSY))
		printf("%s: DMA eop\n", device_xname(ncr_sc->sc_dev));
	else if (((csr & ~SCI_CSR_PHASE_MATCH) == SCI_CSR_INT) &&
	    ((bus_csr & ~SCI_BUS_RST) == 0))
		printf("%s: bus reset\n", device_xname(ncr_sc->sc_dev));
	else if (((csr & ~(SCI_CSR_DREQ | SCI_CSR_ATN | SCI_CSR_ACK)) == (SCI_CSR_PERR | SCI_CSR_INT | SCI_CSR_PHASE_MATCH)) &&
	    ((bus_csr & (SCI_BUS_RST | SCI_BUS_BSY | SCI_BUS_SEL)) == SCI_BUS_BSY))
		printf("%s: parity error\n", device_xname(ncr_sc->sc_dev));
	else if (((csr & ~SCI_CSR_ATN) == SCI_CSR_INT) &&
	    ((bus_csr & (SCI_BUS_RST | SCI_BUS_BSY | SCI_BUS_REQ | SCI_BUS_SEL)) == (SCI_BUS_BSY | SCI_BUS_REQ)))
		printf("%s: phase mismatch\n", device_xname(ncr_sc->sc_dev));
	else if (((csr & ~SCI_CSR_PHASE_MATCH) == (SCI_CSR_INT | SCI_CSR_DISC)) &&
	    (bus_csr == 0))
		printf("%s: disconnect\n", device_xname(ncr_sc->sc_dev));
	else
		printf("%s: unknown intr: csr=%x, bus_csr=%x\n",
		    device_xname(ncr_sc->sc_dev), csr, bus_csr);
}
#endif


/***
 * The following code implements polled PDMA.
 ***/

int
sbc_pdma_in(struct ncr5380_softc *ncr_sc, int phase, int datalen, u_char *data)
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	volatile u_int32_t *long_data = (u_int32_t *)sc->sc_drq_addr;
	volatile u_int8_t *byte_data = (u_int8_t *)sc->sc_nodrq_addr;
	label_t faultbuf;
	int resid, s;

	if (datalen < ncr_sc->sc_min_dma_len ||
	    (sc->sc_options & SBC_PDMA) == 0)
		return ncr5380_pio_in(ncr_sc, phase, datalen, data);

	s = splbio();
	if (sbc_wait_busy(ncr_sc)) {
		splx(s);
		return 0;
	}

	*ncr_sc->sci_mode |= SCI_MODE_DMA;
	*ncr_sc->sci_irecv = 0;

	resid = datalen;

	/*
	 * Setup for a possible bus error caused by SCSI controller
	 * switching out of DATA OUT before we're done with the
	 * current transfer.  (See comment before sbc_drq_intr().)
	 */
	nofault = &faultbuf;
	if (setjmp(nofault)) {
		goto interrupt;
	}

#define R4	*(u_int32_t *)data = *long_data, data += 4;
	for (; resid >= 128; resid -= 128) {
		if (sbc_ready(ncr_sc))
			goto interrupt;
		R4; R4; R4; R4; R4; R4; R4; R4;
		R4; R4; R4; R4; R4; R4; R4; R4;
		R4; R4; R4; R4; R4; R4; R4; R4;
		R4; R4; R4; R4; R4; R4; R4; R4;		/* 128 */
	}
	while (resid) {
		if (sbc_ready(ncr_sc))
			goto interrupt;
		*(u_int8_t *)data = *byte_data, data += 1;
		resid--;
	}
#undef R4

interrupt:
	nofault = NULL;
	SCI_CLR_INTR(ncr_sc);
	*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
	*ncr_sc->sci_icmd = 0;
	splx(s);
	return (datalen - resid);
}

int
sbc_pdma_out(struct ncr5380_softc *ncr_sc, int phase, int datalen, u_char *data)
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	volatile u_int32_t *long_data = (u_int32_t *)sc->sc_drq_addr;
	volatile u_int8_t *byte_data = (u_int8_t *)sc->sc_nodrq_addr;
	label_t faultbuf;
	int resid, s;
	u_int8_t icmd;

#if 1
	/* Work around lame gcc initialization bug */
	(void)&data;
#endif

	if (datalen < ncr_sc->sc_min_dma_len ||
	    (sc->sc_options & SBC_PDMA) == 0)
		return ncr5380_pio_out(ncr_sc, phase, datalen, data);

	s = splbio();
	if (sbc_wait_busy(ncr_sc)) {
		splx(s);
		return 0;
	}

	icmd = *(ncr_sc->sci_icmd) & SCI_ICMD_RMASK;
	*ncr_sc->sci_icmd = icmd | SCI_ICMD_DATA;
	*ncr_sc->sci_mode |= SCI_MODE_DMA;
	*ncr_sc->sci_dma_send = 0;

	/*
	 * Setup for a possible bus error caused by SCSI controller
	 * switching out of DATA OUT before we're done with the
	 * current transfer.  (See comment before sbc_drq_intr().)
	 */
	nofault = &faultbuf;

	if (setjmp(nofault)) {
		printf("buf = 0x%lx, fault = 0x%lx\n",
		    (u_long)sc->sc_drq_addr, (u_long)m68k_fault_addr);
		panic("Unexpected bus error in sbc_pdma_out()");
	}

#define W1	*byte_data = *(u_int8_t *)data, data += 1
#define W4	*long_data = *(u_int32_t *)data, data += 4
	for (resid = datalen; resid >= 64; resid -= 64) {
		if (sbc_ready(ncr_sc))
			goto interrupt;
		W1;
		if (sbc_ready(ncr_sc))
			goto interrupt;
		W1;
		if (sbc_ready(ncr_sc))
			goto interrupt;
		W1;
		if (sbc_ready(ncr_sc))
			goto interrupt;
		W1;
		if (sbc_ready(ncr_sc))
			goto interrupt;
		W4; W4; W4; W4;
		W4; W4; W4; W4;
		W4; W4; W4; W4;
		W4; W4; W4;
	}
	while (resid) {
		if (sbc_ready(ncr_sc))
			goto interrupt;
		W1;
		resid--;
	}
#undef  W1
#undef  W4
	if (sbc_wait_dreq(ncr_sc))
		printf("%s: timeout waiting for DREQ.\n",
		    device_xname(ncr_sc->sc_dev));

	*byte_data = 0;
	goto done;

interrupt:
	if ((*ncr_sc->sci_csr & SCI_CSR_PHASE_MATCH) == 0) {
		*ncr_sc->sci_icmd = icmd & ~SCI_ICMD_DATA;
		--resid;
	}

done:
	SCI_CLR_INTR(ncr_sc);
	*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
	*ncr_sc->sci_icmd = icmd;
	splx(s);
	return (datalen - resid);
}


/***
 * The following code implements interrupt-driven PDMA.
 ***/

/*
 * This is the meat of the PDMA transfer.
 * When we get here, we shove data as fast as the mac can take it.
 * We depend on several things:
 *   * All macs after the Mac Plus that have a 5380 chip should have a general
 *     logic IC that handshakes data for blind transfers.
 *   * If the SCSI controller finishes sending/receiving data before we do,
 *     the same general logic IC will generate a /BERR for us in short order.
 *   * The fault address for said /BERR minus the base address for the
 *     transfer will be the amount of data that was actually written.
 *
 * We use the nofault flag and the setjmp/longjmp in locore.s so we can
 * detect and handle the bus error for early termination of a command.
 * This is usually caused by a disconnecting target.
 */
void
sbc_drq_intr(void *p)
{
	struct sbc_softc *sc = (struct sbc_softc *)p;
	struct ncr5380_softc *ncr_sc = (struct ncr5380_softc *)p;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;
	label_t faultbuf;
	volatile u_int32_t *long_drq;
	u_int32_t *long_data;
	volatile u_int8_t *drq = 0;	/* XXX gcc4 -Wuninitialized */
	u_int8_t *data;
	int count, dcount, resid;

	/*
	 * If we're not ready to xfer data, or have no more, just return.
	 */
	if ((*ncr_sc->sci_csr & SCI_CSR_DREQ) == 0 || dh->dh_len == 0)
		return;

#ifdef SBC_DEBUG
	if (sbc_debug & SBC_DB_INTR)
		printf("%s: drq intr, dh_len=0x%x, dh_flags=0x%x\n",
		    device_xname(ncr_sc->sc_dev), dh->dh_len, dh->dh_flags);
#endif

	/*
	 * Setup for a possible bus error caused by SCSI controller
	 * switching out of DATA-IN/OUT before we're done with the
	 * current transfer.
	 */
	nofault = &faultbuf;

	if (setjmp(nofault)) {
		nofault = (label_t *)0;
		if ((dh->dh_flags & SBC_DH_DONE) == 0) {
			count = ((  (u_long)m68k_fault_addr
				  - (u_long)sc->sc_drq_addr));

			if ((count < 0) || (count > dh->dh_len)) {
				printf("%s: complete=0x%x (pending 0x%x)\n",
				    device_xname(ncr_sc->sc_dev), count,
				    dh->dh_len);
				panic("something is wrong");
			}

			dh->dh_addr += count;
			dh->dh_len -= count;
		} else
			count = 0;

#ifdef SBC_DEBUG
		if (sbc_debug & SBC_DB_INTR)
			printf("%s: drq /berr, complete=0x%x (pending 0x%x)\n",
			   device_xname(ncr_sc->sc_dev), count, dh->dh_len);
#endif
		m68k_fault_addr = 0;

		return;
	}

	if (dh->dh_flags & SBC_DH_OUT) { /* Data Out */
		dcount = 0;

		/*
		 * Get the source address aligned.
		 */
		resid =
		    count = min(dh->dh_len, 4 - (((int)dh->dh_addr) & 0x3));
		if (count && count < 4) {
			drq = (volatile u_int8_t *)sc->sc_drq_addr;
			data = (u_int8_t *)dh->dh_addr;

#define W1		*drq++ = *data++
			while (count) {
				W1; count--;
			}
#undef W1
			dh->dh_addr += resid;
			dh->dh_len -= resid;
		}

		/*
		 * Start the transfer.
		 */
		while (dh->dh_len) {
			dcount = count = min(dh->dh_len, MAX_DMA_LEN);
			long_drq = (volatile u_int32_t *)sc->sc_drq_addr;
			long_data = (u_int32_t *)dh->dh_addr;

#define W4		*long_drq++ = *long_data++
			while (count >= 64) {
				W4; W4; W4; W4; W4; W4; W4; W4;
				W4; W4; W4; W4; W4; W4; W4; W4; /*  64 */
				count -= 64;
			}
			while (count >= 4) {
				W4; count -= 4;
			}
#undef W4
			data = (u_int8_t *)long_data;
			drq = (volatile u_int8_t *)long_drq;

#define W1		*drq++ = *data++
			while (count) {
				W1; count--;
			}
#undef W1
			dh->dh_len -= dcount;
			dh->dh_addr += dcount;
		}
		dh->dh_flags |= SBC_DH_DONE;

		/*
		 * XXX -- Read a byte from the SBC to trigger a /BERR.
		 * This seems to be necessary for us to notice that
		 * the target has disconnected.  Ick.  06 jun 1996 (sr)
		 */
		if (dcount >= MAX_DMA_LEN)
			drq = (volatile u_int8_t *)sc->sc_drq_addr;
		(void)*drq;
	} else {	/* Data In */
		/*
		 * Get the dest address aligned.
		 */
		resid =
		    count = min(dh->dh_len, 4 - (((int)dh->dh_addr) & 0x3));
		if (count && count < 4) {
			data = (u_int8_t *)dh->dh_addr;
			drq = (volatile u_int8_t *)sc->sc_drq_addr;
			while (count) {
				*data++ = *drq++;
				count--;
			}
			dh->dh_addr += resid;
			dh->dh_len -= resid;
		}

		/*
		 * Start the transfer.
		 */
		while (dh->dh_len) {
			dcount = count = min(dh->dh_len, MAX_DMA_LEN);
			long_data = (u_int32_t *)dh->dh_addr;
			long_drq = (volatile u_int32_t *)sc->sc_drq_addr;

#define R4		*long_data++ = *long_drq++
			while (count >= 64) {
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;	/* 64 */
				count -= 64;
			}
			while (count >= 4) {
				R4; count -= 4;
			}
#undef R4
			data = (u_int8_t *)long_data;
			drq = (volatile u_int8_t *)long_drq;
			while (count) {
				*data++ = *drq++;
				count--;
			}
			dh->dh_len -= dcount;
			dh->dh_addr += dcount;
		}
		dh->dh_flags |= SBC_DH_DONE;
	}

	/*
	 * OK.  No bus error occurred above.  Clear the nofault flag
	 * so we no longer short-circuit bus errors.
	 */
	nofault = (label_t *)0;

#ifdef SBC_DEBUG
	if (sbc_debug & (SBC_DB_REG | SBC_DB_INTR))
		printf("%s: drq intr complete: csr=0x%x, bus_csr=0x%x\n",
		    device_xname(ncr_sc->sc_dev), *ncr_sc->sci_csr,
		    *ncr_sc->sci_bus_csr);
#endif
}

void
sbc_dma_alloc(struct ncr5380_softc *ncr_sc)
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsipi_xfer *xs = sr->sr_xs;
	struct sbc_pdma_handle *dh;
	int		i, xlen;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("sbc_dma_alloc: already have PDMA handle");
#endif

	/* Polled transfers shouldn't allocate a PDMA handle. */
	if (sr->sr_flags & SR_IMMED)
		return;

	xlen = ncr_sc->sc_datalen;

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("sbc_dma_alloc: len=0x%x", xlen);

	/*
	 * Find free PDMA handle.  Guaranteed to find one since we
	 * have as many PDMA handles as the driver has processes.
	 * (instances?)
	 */
	 for (i = 0; i < SCI_OPENINGS; i++) {
		if ((sc->sc_pdma[i].dh_flags & SBC_DH_BUSY) == 0)
			goto found;
	}
	panic("sbc: no free PDMA handles");
found:
	dh = &sc->sc_pdma[i];
	dh->dh_flags = SBC_DH_BUSY;
	dh->dh_addr = ncr_sc->sc_dataptr;
	dh->dh_len = xlen;

	/* Copy the 'write' flag for convenience. */
	if (xs->xs_control & XS_CTL_DATA_OUT)
		dh->dh_flags |= SBC_DH_OUT;

	sr->sr_dma_hand = dh;
}

void
sbc_dma_free(struct ncr5380_softc *ncr_sc)
{
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;

#ifdef DIAGNOSTIC
	if (sr->sr_dma_hand == NULL)
		panic("sbc_dma_free: no DMA handle");
#endif

	if (ncr_sc->sc_state & NCR_DOINGDMA)
		panic("sbc_dma_free: free while in progress");

	if (dh->dh_flags & SBC_DH_BUSY) {
		dh->dh_flags = 0;
		dh->dh_addr = NULL;
		dh->dh_len = 0;
	}
	sr->sr_dma_hand = NULL;
}

void
sbc_dma_poll(struct ncr5380_softc *ncr_sc)
{
	struct sci_req *sr = ncr_sc->sc_current;

	/*
	 * We shouldn't arrive here; if SR_IMMED is set, then
	 * dma_alloc() should have refused to allocate a handle
	 * for the transfer.  This forces the polled PDMA code
	 * to handle the request...
	 */
#ifdef SBC_DEBUG
	if (sbc_debug & SBC_DB_DMA)
		printf("%s: lost DRQ interrupt?\n",
		    device_xname(ncr_sc->sc_dev));
#endif
	sr->sr_flags |= SR_OVERDUE;
}

void
sbc_dma_setup(struct ncr5380_softc *ncr_sc)
{
	/* Not needed; we don't have real DMA */
}

void
sbc_dma_start(struct ncr5380_softc *ncr_sc)
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;

	/*
	 * Match bus phase, clear pending interrupts, set DMA mode, and
	 * assert data bus (for writing only), then start the transfer.
	 */
	if (dh->dh_flags & SBC_DH_OUT) {
		*ncr_sc->sci_tcmd = PHASE_DATA_OUT;
		SCI_CLR_INTR(ncr_sc);
		if (sc->sc_clrintr)
			(*sc->sc_clrintr)(ncr_sc);
		*ncr_sc->sci_mode |= SCI_MODE_DMA;
		*ncr_sc->sci_icmd = SCI_ICMD_DATA;
		*ncr_sc->sci_dma_send = 0;
	} else {
		*ncr_sc->sci_tcmd = PHASE_DATA_IN;
		SCI_CLR_INTR(ncr_sc);
		if (sc->sc_clrintr)
			(*sc->sc_clrintr)(ncr_sc);
		*ncr_sc->sci_mode |= SCI_MODE_DMA;
		*ncr_sc->sci_icmd = 0;
		*ncr_sc->sci_irecv = 0;
	}
	ncr_sc->sc_state |= NCR_DOINGDMA;

#ifdef SBC_DEBUG
	if (sbc_debug & SBC_DB_DMA)
		printf("%s: PDMA started, va=%p, len=0x%x\n",
		    device_xname(ncr_sc->sc_dev), dh->dh_addr, dh->dh_len);
#endif
}

void
sbc_dma_eop(struct ncr5380_softc *ncr_sc)
{
	/* Not used; the EOP pin is wired high (GMFH, pp. 389-390) */
}

void
sbc_dma_stop(struct ncr5380_softc *ncr_sc)
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;
	int ntrans;

	if ((ncr_sc->sc_state & NCR_DOINGDMA) == 0) {
#ifdef SBC_DEBUG
		if (sbc_debug & SBC_DB_DMA)
			printf("%s: dma_stop: DMA not running\n",
			    device_xname(ncr_sc->sc_dev));
#endif
		return;
	}
	ncr_sc->sc_state &= ~NCR_DOINGDMA;

	if ((ncr_sc->sc_state & NCR_ABORTING) == 0) {
		ntrans = ncr_sc->sc_datalen - dh->dh_len;

#ifdef SBC_DEBUG
		if (sbc_debug & SBC_DB_DMA)
			printf("%s: dma_stop: ntrans=0x%x\n",
			    device_xname(ncr_sc->sc_dev), ntrans);
#endif

		if (ntrans > ncr_sc->sc_datalen)
			panic("sbc_dma_stop: excess transfer");

		/* Adjust data pointer */
		ncr_sc->sc_dataptr += ntrans;
		ncr_sc->sc_datalen -= ntrans;

		/* Clear any pending interrupts. */
		SCI_CLR_INTR(ncr_sc);
		if (sc->sc_clrintr)
			(*sc->sc_clrintr)(ncr_sc);
	}

	/* Put SBIC back into PIO mode. */
	*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
	*ncr_sc->sci_icmd = 0;

#ifdef SBC_DEBUG
	if (sbc_debug & SBC_DB_REG)
		printf("%s: dma_stop: csr=0x%x, bus_csr=0x%x\n",
		    device_xname(ncr_sc->sc_dev), *ncr_sc->sci_csr,
		    *ncr_sc->sci_bus_csr);
#endif
}
