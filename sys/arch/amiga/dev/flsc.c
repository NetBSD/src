/*	$NetBSD: flsc.c,v 1.24 1999/09/25 21:47:10 is Exp $	*/

/*
 * Copyright (c) 1997 Michael L. Hitch
 * Copyright (c) 1995 Daniel Widenfalk
 * Copyright (c) 1994 Christian E. Hopps
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Daniel Widenfalk
 *	and Michael L. Hitch.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Initial amiga Fastlane driver by Daniel Widenfalk.  Conversion to
 * 53c9x MI driver by Michael L. Hitch (mhitch@montana.edu).
 */

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/cpu.h>
#include <machine/param.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <amiga/amiga/isr.h>
#include <amiga/dev/flscvar.h>
#include <amiga/dev/zbusvar.h>

void	flscattach	__P((struct device *, struct device *, void *));
int	flscmatch	__P((struct device *, struct cfdata *, void *));

/* Linkup to the rest of the kernel */
struct cfattach flsc_ca = {
	sizeof(struct flsc_softc), flscmatch, flscattach
};

struct scsipi_device flsc_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

/*
 * Functions and the switch for the MI code.
 */
u_char	flsc_read_reg __P((struct ncr53c9x_softc *, int));
void	flsc_write_reg __P((struct ncr53c9x_softc *, int, u_char));
int	flsc_dma_isintr __P((struct ncr53c9x_softc *));
void	flsc_dma_reset __P((struct ncr53c9x_softc *));
int	flsc_dma_intr __P((struct ncr53c9x_softc *));
int	flsc_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *));
void	flsc_dma_go __P((struct ncr53c9x_softc *));
void	flsc_dma_stop __P((struct ncr53c9x_softc *));
int	flsc_dma_isactive __P((struct ncr53c9x_softc *));
void	flsc_clear_latched_intr __P((struct ncr53c9x_softc *));

struct ncr53c9x_glue flsc_glue = {
	flsc_read_reg,
	flsc_write_reg,
	flsc_dma_isintr,
	flsc_dma_reset,
	flsc_dma_intr,
	flsc_dma_setup,
	flsc_dma_go,
	flsc_dma_stop,
	flsc_dma_isactive,
	flsc_clear_latched_intr,
};

/* Maximum DMA transfer length to reduce impact on high-speed serial input */
u_long flsc_max_dma = 1024;
extern int ser_open_speed;

extern int ncr53c9x_debug;
extern u_long scsi_nosync;
extern int shift_nosync;

/*
 * if we are an Advanced Systems & Software FastlaneZ3
 */
int
flscmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct zbus_args *zap;

	if (!is_a4000() && !is_a3000())
		return(0);

	zap = aux;
	if (zap->manid == 0x2140 && zap->prodid == 11
	    && iszthreepa(zap->pa))
		return(1);

	return(0);
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
flscattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct flsc_softc *fsc = (void *)self;
	struct ncr53c9x_softc *sc = &fsc->sc_ncr53c9x;
	struct zbus_args  *zap;

	/*
	 * Set up the glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &flsc_glue;

	/*
	 * Save the regs
	 */
	zap = aux;
	fsc->sc_dmabase = (volatile u_char *)zap->va;
	fsc->sc_reg = &((volatile u_char *)zap->va)[0x1000001];

	sc->sc_freq = 40;		/* Clocked at 40Mhz */

	printf(": address %p", fsc->sc_reg);

	sc->sc_id = 7;

	/*
	 * It is necessary to try to load the 2nd config register here,
	 * to find out what rev the flsc chip is, else the flsc_reset
	 * will not set up the defaults correctly.
	 */
	sc->sc_cfg1 = sc->sc_id | NCRCFG1_PARENB;
	sc->sc_cfg2 = NCRCFG2_SCSI2 | NCRCFG2_FE;
	sc->sc_cfg3 = 0x08 /*FCLK*/ | NCRESPCFG3_FSCSI | NCRESPCFG3_CDB;
	sc->sc_rev = NCR_VARIANT_FAS216;

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	sc->sc_minsync = 1000 / sc->sc_freq;

	if (((scsi_nosync >> shift_nosync) & 0xff00) == 0xff00)
		sc->sc_minsync = 0;

	/* Really no limit, but since we want to fit into the TCR... */
	sc->sc_maxxfer = 64 * 1024;

	fsc->sc_portbits = 0xa0 | FLSC_PB_EDI | FLSC_PB_ESI;
	fsc->sc_hardbits = fsc->sc_reg[0x40];

	fsc->sc_alignbuf = (char *)((u_long)fsc->sc_unalignbuf & -4);

	sc->sc_dev.dv_cfdata->cf_flags |= (scsi_nosync >> shift_nosync) & 0xffff;
	shift_nosync += 16;
	ncr53c9x_debug |= (scsi_nosync >> shift_nosync) & 0xffff;
	shift_nosync += 16;

	/*
	 * Configure interrupts.
	 */
	fsc->sc_isr.isr_intr = (int (*)(void *))ncr53c9x_intr;
	fsc->sc_isr.isr_arg  = sc;
	fsc->sc_isr.isr_ipl  = 2;
	add_isr(&fsc->sc_isr);

	fsc->sc_reg[0x40] = fsc->sc_portbits;

	/*
	 * Now try to attach all the sub-devices
	 */
	sc->sc_adapter.scsipi_cmd = ncr53c9x_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = minphys; 
	ncr53c9x_attach(sc, &flsc_dev);
}

/*
 * Glue functions.
 */

u_char
flsc_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;

	return fsc->sc_reg[reg * 4];
}

void
flsc_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;
	struct ncr53c9x_tinfo *ti;
	u_char v = val;

	if (fsc->sc_piomode && reg == NCR_CMD &&
	    v == (NCRCMD_TRANS|NCRCMD_DMA)) {
		v = NCRCMD_TRANS;
	}
	/*
	 * Can't do synchronous transfers in SCSI_POLL mode:
	 * If starting SCSI_POLL command, clear defer sync negotiation
	 * by clearing the T_NEGOTIATE flag.  If starting SCSI_POLL and
	 * the device is currently running synchronous, force another
	 * T_NEGOTIATE with 0 offset.
	 */
	if (reg == NCR_SELID) {
		ti = &sc->sc_tinfo[
		    sc->sc_nexus->xs->sc_link->scsipi_scsi.target];
		if (sc->sc_nexus->xs->flags & SCSI_POLL) {
			if (ti->flags & T_SYNCMODE) {
				ti->flags ^= T_SYNCMODE | T_NEGOTIATE;
			} else if (ti->flags & T_NEGOTIATE) {
				ti->flags ^= T_NEGOTIATE | T_SYNCHOFF;
				/* save T_NEGOTIATE in private flags? */
			}
		} else {
			/*
			 * If we haven't attempted sync negotiation yet,
			 * do it now.
			 */
			if ((ti->flags & (T_SYNCMODE | T_SYNCHOFF)) ==
			    T_SYNCHOFF &&
			    sc->sc_minsync != 0)	/* XXX */
				ti->flags ^= T_NEGOTIATE | T_SYNCHOFF;
		}
	}
	if (reg == NCR_CMD && v == NCRCMD_SETATN  &&
	    sc->sc_flags & NCR_SYNCHNEGO &&
	     sc->sc_nexus->xs->flags & SCSI_POLL) {
		ti = &sc->sc_tinfo[
		    sc->sc_nexus->xs->sc_link->scsipi_scsi.target];
		ti->offset = 0;
	}
	fsc->sc_reg[reg * 4] = v;
}

int
flsc_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;
	unsigned hardbits;

	hardbits = fsc->sc_reg[0x40];
	if (hardbits & FLSC_HB_IACT)
		return (fsc->sc_csr = 0);

	if (sc->sc_state == NCR_CONNECTED || sc->sc_state == NCR_SELECTING)
		fsc->sc_portbits |= FLSC_PB_LED;
	else
		fsc->sc_portbits &= ~FLSC_PB_LED;

	if ((hardbits & FLSC_HB_CREQ) && !(hardbits & FLSC_HB_MINT) &&
	    fsc->sc_reg[NCR_STAT * 4] & NCRSTAT_INT) {
		return 1;
	}
	/* Do I still need this? */
	if (fsc->sc_piomode && fsc->sc_reg[NCR_STAT * 4] & NCRSTAT_INT &&
	    !(hardbits & FLSC_HB_MINT))
		return 1;

	fsc->sc_reg[0x40] = fsc->sc_portbits & ~FLSC_PB_INT_BITS;
	fsc->sc_reg[0x40] = fsc->sc_portbits;
	return 0;
}

void
flsc_clear_latched_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;

	fsc->sc_reg[0x40] = fsc->sc_portbits & ~FLSC_PB_INT_BITS;
	fsc->sc_reg[0x40] = fsc->sc_portbits;
}

void
flsc_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;
struct ncr53c9x_tinfo *ti;

if (sc->sc_nexus)
  ti = &sc->sc_tinfo[sc->sc_nexus->xs->sc_link->scsipi_scsi.target];
else
  ti = &sc->sc_tinfo[1];	/* XXX */
if (fsc->sc_active) {
  printf("dmaaddr %p dmasize %d stat %x flags %x off %d per %d ff %x",
     *fsc->sc_dmaaddr, fsc->sc_dmasize, fsc->sc_reg[NCR_STAT * 4],
     ti->flags, ti->offset, ti->period, fsc->sc_reg[NCR_FFLAG * 4]);
  printf(" intr %x\n", fsc->sc_reg[NCR_INTR * 4]);
#ifdef DDB
  Debugger();
#endif
}
	fsc->sc_portbits &= ~FLSC_PB_DMA_BITS;
	fsc->sc_reg[0x40] = fsc->sc_portbits;
	fsc->sc_reg[0x80] = 0;
	*((u_long *)fsc->sc_dmabase) = 0;
	fsc->sc_active = 0;
	fsc->sc_piomode = 0;
}

int
flsc_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	register struct flsc_softc *fsc = (struct flsc_softc *)sc;
	register u_char	*p;
	volatile u_char *cmdreg, *intrreg, *statreg, *fiforeg;
	register u_int	flscphase, flscstat, flscintr;
	register int	cnt;

	NCR_DMA(("flsc_dma_intr: pio %d cnt %d int %x stat %x fifo %d ",
	    fsc->sc_piomode, fsc->sc_dmasize, sc->sc_espintr, sc->sc_espstat,
	    fsc->sc_reg[NCR_FFLAG * 4] & NCRFIFO_FF));
	if (!(fsc->sc_reg[0x40] & FLSC_HB_CREQ))
		printf("flsc_dma_intr: csr %x stat %x intr %x\n", fsc->sc_csr,
		    sc->sc_espstat, sc->sc_espintr);
	if (fsc->sc_active == 0) {
		printf("flsc_intr--inactive DMA\n");
		return -1;
	}

/* if DMA transfer, update sc_dmaaddr and sc_pdmalen, else PIO xfer */
	if (fsc->sc_piomode == 0) {
		fsc->sc_portbits &= ~FLSC_PB_DMA_BITS;
		fsc->sc_reg[0x40] = fsc->sc_portbits;
		fsc->sc_reg[0x80] = 0;
		*((u_long *)fsc->sc_dmabase) = 0;
		cnt = fsc->sc_reg[NCR_TCL * 4];
		cnt += fsc->sc_reg[NCR_TCM * 4] << 8;
		cnt += fsc->sc_reg[NCR_TCH * 4] << 16;
		if (!fsc->sc_datain) {
			cnt += fsc->sc_reg[NCR_FFLAG * 4] & NCRFIFO_FF;
			fsc->sc_reg[NCR_CMD * 4] = NCRCMD_FLUSH;
		}
		cnt = fsc->sc_dmasize - cnt;	/* number of bytes transferred */
		NCR_DMA(("DMA xferred %d\n", cnt));
		if (fsc->sc_xfr_align) {
			int i;
			for (i = 0; i < cnt; ++i)
				(*fsc->sc_dmaaddr)[i] = fsc->sc_alignbuf[i];
			fsc->sc_xfr_align = 0;
		}
		*fsc->sc_dmaaddr += cnt;
		*fsc->sc_pdmalen -= cnt;
		fsc->sc_active = 0;
		return 0;
	}

	if ((sc->sc_espintr & NCRINTR_BS) == 0) {
		fsc->sc_active = 0;
		fsc->sc_piomode = 0;
		NCR_DMA(("no NCRINTR_BS\n"));
		return 0;
	}

	cnt = fsc->sc_dmasize;
#if 0
	if (cnt == 0) {
		printf("data interrupt, but no count left.");
	}
#endif

	p = *fsc->sc_dmaaddr;
	flscphase = sc->sc_phase;
	flscstat = (u_int) sc->sc_espstat;
	flscintr = (u_int) sc->sc_espintr;
	cmdreg = fsc->sc_reg + NCR_CMD * 4;
	fiforeg = fsc->sc_reg + NCR_FIFO * 4;
	statreg = fsc->sc_reg + NCR_STAT * 4;
	intrreg = fsc->sc_reg + NCR_INTR * 4;
	NCR_DMA(("PIO %d datain %d phase %d stat %x intr %x\n",
	    cnt, fsc->sc_datain, flscphase, flscstat, flscintr));
	do {
		if (fsc->sc_datain) {
			*p++ = *fiforeg;
			cnt--;
			if (flscphase == DATA_IN_PHASE) {
				*cmdreg = NCRCMD_TRANS;
			} else {
				fsc->sc_active = 0;
			}
	 	} else {
NCR_DMA(("flsc_dma_intr: PIO out- phase %d cnt %d active %d\n", flscphase, cnt,
    fsc->sc_active));
			if (   (flscphase == DATA_OUT_PHASE)
			    || (flscphase == MESSAGE_OUT_PHASE)) {
				int n;
				n = 16 - (fsc->sc_reg[NCR_FFLAG * 4] & NCRFIFO_FF);
				if (n > cnt)
					n = cnt;
				cnt -= n;
				while (n-- > 0)
					*fiforeg = *p++;
				*cmdreg = NCRCMD_TRANS;
			} else {
				fsc->sc_active = 0;
			}
		}

		if (fsc->sc_active && cnt) {
			while (!(*statreg & 0x80));
			flscstat = *statreg;
			flscintr = *intrreg;
			flscphase = (flscintr & NCRINTR_DIS)
				    ? /* Disconnected */ BUSFREE_PHASE
				    : flscstat & PHASE_MASK;
		}
	} while (cnt && fsc->sc_active && (flscintr & NCRINTR_BS));
#if 1
if (fsc->sc_dmasize < 8 && cnt)
  printf("flsc_dma_intr: short transfer: dmasize %d cnt %d\n",
    fsc->sc_dmasize, cnt);
#endif
	NCR_DMA(("flsc_dma_intr: PIO transfer [%d], %d->%d phase %d stat %x intr %x\n",
	    *fsc->sc_pdmalen, fsc->sc_dmasize, cnt, flscphase, flscstat, flscintr));
	sc->sc_phase = flscphase;
	sc->sc_espstat = (u_char) flscstat;
	sc->sc_espintr = (u_char) flscintr;
	*fsc->sc_dmaaddr = p;
	*fsc->sc_pdmalen -= fsc->sc_dmasize - cnt;
	fsc->sc_dmasize = cnt;

	if (*fsc->sc_pdmalen == 0) {
		sc->sc_espstat |= NCRSTAT_TC;
		fsc->sc_piomode = 0;
	}
	return 0;
}

int
flsc_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;
	paddr_t pa;
	u_char *ptr;
	size_t xfer;

	fsc->sc_dmaaddr = addr;
	fsc->sc_pdmalen = len;
	fsc->sc_datain = datain;
	fsc->sc_dmasize = *dmasize;
	if (sc->sc_nexus->xs->flags & SCSI_POLL) {
		/* polling mode, use PIO */
		*dmasize = fsc->sc_dmasize;
		NCR_DMA(("pfsc_dma_setup: PIO %p/%d [%d]\n", *addr,
		    fsc->sc_dmasize, *len));
		fsc->sc_piomode = 1;
		if (datain == 0) {
			int n;
			n = fsc->sc_dmasize;
			if (n > 16)
				n = 16;
			while (n-- > 0) {
				fsc->sc_reg[NCR_FIFO * 4] = **fsc->sc_dmaaddr;
				(*fsc->sc_pdmalen)--;
				(*fsc->sc_dmaaddr)++;
				--fsc->sc_dmasize;
			}
		}
		return 0;
	}
	/*
	 * DMA can be nasty for high-speed serial input, so limit the
	 * size of this DMA operation if the serial port is running at
	 * a high speed (higher than 19200 for now - should be adjusted
	 * based on cpu type and speed?).
	 * XXX - add serial speed check XXX
	 */
	if (ser_open_speed > 19200 && flsc_max_dma != 0 &&
	    fsc->sc_dmasize > flsc_max_dma)
		fsc->sc_dmasize = flsc_max_dma;
	ptr = *addr;			/* Kernel virtual address */
	pa = kvtop(ptr);		/* Physical address of DMA */
	xfer = min(fsc->sc_dmasize, NBPG - (pa & (NBPG - 1)));
	fsc->sc_xfr_align = 0;
	fsc->sc_piomode = 0;
	fsc->sc_portbits &= ~FLSC_PB_DMA_BITS;
	fsc->sc_reg[0x40] = fsc->sc_portbits;
	fsc->sc_reg[0x80] = 0;
	*((u_long *)fsc->sc_dmabase) = 0;

	/*
	 * If output and length < 16, copy to fifo
	 */
	if (datain == 0 && fsc->sc_dmasize < 16) {
		int n;
		for (n = 0; n < fsc->sc_dmasize; ++n)
			fsc->sc_reg[NCR_FIFO * 4] = *ptr++;
		NCR_DMA(("flsc_dma_setup: %d bytes written to fifo\n", n));
		fsc->sc_piomode = 1;
		fsc->sc_active = 1;
		*fsc->sc_pdmalen -= fsc->sc_dmasize;
		*fsc->sc_dmaaddr += fsc->sc_dmasize;
		*dmasize = fsc->sc_dmasize;
		fsc->sc_dmasize = 0;
		return 0;		/* All done */
	}
	/*
	 * If output and unaligned, copy unaligned data to fifo
	 */
	else if (datain == 0 && (int)ptr & 3) {
		int n = 4 - ((int)ptr & 3);
		NCR_DMA(("flsc_dma_setup: align %d bytes written to fifo\n", n));
		pa += n;
		xfer -= n;
		while (n--)
			fsc->sc_reg[NCR_FIFO * 4] = *ptr++;
	}
	/*
	 * If unaligned address, read unaligned bytes into alignment buffer
	 */
	else if ((int)ptr & 3 || xfer & 3) {
		pa = kvtop((caddr_t)fsc->sc_alignbuf);
		xfer = fsc->sc_dmasize = min(xfer, sizeof (fsc->sc_unalignbuf));
		NCR_DMA(("flsc_dma_setup: align read by %d bytes\n", xfer));
		fsc->sc_xfr_align = 1;
	}
	/*
	 * If length smaller than longword, read into alignment buffer
	 * XXX doesn't work for 1 or 2 bytes !!!!
	 */
	else if (fsc->sc_dmasize < 4) {
		NCR_DMA(("flsc_dma_setup: read remaining %d bytes\n",
		    fsc->sc_dmasize));
		pa = kvtop((caddr_t)fsc->sc_alignbuf);
		fsc->sc_xfr_align = 1;
	}
	/*
	 * Finally, limit transfer length to multiple of 4 bytes.
	 */
	else {
		fsc->sc_dmasize &= -4;
		xfer &= -4;
	}

	while (xfer < fsc->sc_dmasize) {
		if ((pa + xfer) != kvtop(*addr + xfer))
			break;
		if ((fsc->sc_dmasize - xfer) < NBPG)
			xfer = fsc->sc_dmasize;
		else
			xfer += NBPG;
	}

	fsc->sc_dmasize = xfer;
	*dmasize = fsc->sc_dmasize;
	fsc->sc_pa = pa;
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		if (fsc->sc_xfr_align) {
			int n;
			for (n = 0; n < sizeof (fsc->sc_unalignbuf); ++n)
				fsc->sc_alignbuf[n] = n | 0x80;
			dma_cachectl(fsc->sc_alignbuf,
			    sizeof(fsc->sc_unalignbuf));
		}
		else
			dma_cachectl(*fsc->sc_dmaaddr, fsc->sc_dmasize);
	}
#endif
	fsc->sc_reg[0x80] = 0;
	*((u_long *)(fsc->sc_dmabase + (pa & 0x00fffffc))) = pa;
	fsc->sc_portbits &= ~FLSC_PB_DMA_BITS;
	fsc->sc_portbits |= FLSC_PB_ENABLE_DMA |
	    (fsc->sc_datain ? FLSC_PB_DMA_READ : FLSC_PB_DMA_WRITE);
	fsc->sc_reg[0x40] = fsc->sc_portbits;
	NCR_DMA(("flsc_dma_setup: DMA %p->%lx/%d [%d]\n",
	    ptr, pa, fsc->sc_dmasize, *len));
	fsc->sc_active = 1;
	return 0;
}

void
flsc_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;

	NCR_DMA(("flsc_dma_go: datain %d size %d\n", fsc->sc_datain,
	    fsc->sc_dmasize));
	if (sc->sc_nexus->xs->flags & SCSI_POLL) {
		fsc->sc_active = 1;
		return;
	} else if (fsc->sc_piomode == 0) {
		fsc->sc_portbits &= ~FLSC_PB_DMA_BITS;
		fsc->sc_portbits |= FLSC_PB_ENABLE_DMA |
		    (fsc->sc_datain ? FLSC_PB_DMA_READ : FLSC_PB_DMA_WRITE);
		fsc->sc_reg[0x40] = fsc->sc_portbits;
	}
}

void
flsc_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;

	fsc->sc_portbits &= ~FLSC_PB_DMA_BITS;
	fsc->sc_reg[0x40] = fsc->sc_portbits;

	fsc->sc_reg[0x80] = 0;
	*((u_long *)fsc->sc_dmabase) = 0;
	fsc->sc_piomode = 0;
}

int
flsc_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct flsc_softc *fsc = (struct flsc_softc *)sc;

	return fsc->sc_active;
}
