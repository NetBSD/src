/*	$NetBSD: cbiisc.c,v 1.16 2003/04/01 21:26:29 thorpej Exp $ */

/*
 * Copyright (c) 1997 Michael L. Hitch
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
 *	This product contains software written by Michael L. Hitch for
 *	the NetBSD project.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cbiisc.c,v 1.16 2003/04/01 21:26:29 thorpej Exp $");

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

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/cpu.h>
#include <machine/param.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <amiga/amiga/isr.h>
#include <amiga/dev/cbiiscvar.h>
#include <amiga/dev/zbusvar.h>

void	cbiiscattach(struct device *, struct device *, void *);
int	cbiiscmatch(struct device *, struct cfdata *, void *);

/* Linkup to the rest of the kernel */
CFATTACH_DECL(cbiisc, sizeof(struct cbiisc_softc),
    cbiiscmatch, cbiiscattach, NULL, NULL);

/*
 * Functions and the switch for the MI code.
 */
u_char	cbiisc_read_reg(struct ncr53c9x_softc *, int);
void	cbiisc_write_reg(struct ncr53c9x_softc *, int, u_char);
int	cbiisc_dma_isintr(struct ncr53c9x_softc *);
void	cbiisc_dma_reset(struct ncr53c9x_softc *);
int	cbiisc_dma_intr(struct ncr53c9x_softc *);
int	cbiisc_dma_setup(struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *);
void	cbiisc_dma_go(struct ncr53c9x_softc *);
void	cbiisc_dma_stop(struct ncr53c9x_softc *);
int	cbiisc_dma_isactive(struct ncr53c9x_softc *);

struct ncr53c9x_glue cbiisc_glue = {
	cbiisc_read_reg,
	cbiisc_write_reg,
	cbiisc_dma_isintr,
	cbiisc_dma_reset,
	cbiisc_dma_intr,
	cbiisc_dma_setup,
	cbiisc_dma_go,
	cbiisc_dma_stop,
	cbiisc_dma_isactive,
	0,
};

/* Maximum DMA transfer length to reduce impact on high-speed serial input */
u_long cbiisc_max_dma = 1024;
extern int ser_open_speed;

u_long cbiisc_cnt_pio = 0;	/* number of PIO transfers */
u_long cbiisc_cnt_dma = 0;	/* number of DMA transfers */
u_long cbiisc_cnt_dma2 = 0;	/* number of DMA transfers broken up */
u_long cbiisc_cnt_dma3 = 0;	/* number of pages combined */

#ifdef DEBUG
struct {
	u_char hardbits;
	u_char status;
	u_char xx;
	u_char yy;
} cbiisc_trace[128];
int cbiisc_trace_ptr = 0;
int cbiisc_trace_enable = 1;
void cbiisc_dump(void);
#endif

/*
 * if we are a Phase5 CyberSCSI II
 */
int
cbiiscmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct zbus_args *zap;
	volatile u_char *regs;

	zap = aux;
	if (zap->manid != 0x2140 || zap->prodid != 25)
		return(0);
	regs = &((volatile u_char *)zap->va)[0x1ff03];
	if (badaddr((caddr_t)regs))
		return(0);
	regs[NCR_CFG1 * 4] = 0;
	regs[NCR_CFG1 * 4] = NCRCFG1_PARENB | 7;
	delay(5);
	if (regs[NCR_CFG1 * 4] != (NCRCFG1_PARENB | 7))
		return(0);
	return(1);
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
cbiiscattach(struct device *parent, struct device *self, void *aux)
{
	struct cbiisc_softc *csc = (void *)self;
	struct ncr53c9x_softc *sc = &csc->sc_ncr53c9x;
	struct zbus_args  *zap;
	extern u_long scsi_nosync;
	extern int shift_nosync;
	extern int ncr53c9x_debug;

	/*
	 * Set up the glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &cbiisc_glue;

	/*
	 * Save the regs
	 */
	zap = aux;
	csc->sc_reg = &((volatile u_char *)zap->va)[0x1ff03];
	csc->sc_dmabase = &csc->sc_reg[0x80];

	sc->sc_freq = 40;		/* Clocked at 40Mhz */

	printf(": address %p", csc->sc_reg);

	sc->sc_id = 7;

	/*
	 * It is necessary to try to load the 2nd config register here,
	 * to find out what rev the FAS chip is, else the ncr53c9x_reset
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

	/*
	 * get flags from -I argument and set cf_flags.
	 * NOTE: low 8 bits are to disable disconnect, and the next
	 *       8 bits are to disable sync.
	 */
	sc->sc_dev.dv_cfdata->cf_flags |= (scsi_nosync >> shift_nosync)
	    & 0xffff;
	shift_nosync += 16;

	/* Use next 16 bits of -I argument to set ncr53c9x_debug flags */
	ncr53c9x_debug |= (scsi_nosync >> shift_nosync) & 0xffff;
	shift_nosync += 16;

#if 1
	if (((scsi_nosync >> shift_nosync) & 0xff00) == 0xff00)
		sc->sc_minsync = 0;
#endif

	/* Really no limit, but since we want to fit into the TCR... */
	sc->sc_maxxfer = 64 * 1024;

	/*
	 * Configure interrupts.
	 */
	csc->sc_isr.isr_intr = ncr53c9x_intr;
	csc->sc_isr.isr_arg  = sc;
	csc->sc_isr.isr_ipl  = 2;
	add_isr(&csc->sc_isr);

	/*
	 * Now try to attach all the sub-devices
	 */
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	sc->sc_adapter.adapt_minphys = minphys;
	ncr53c9x_attach(sc);
}

/*
 * Glue functions.
 */

u_char
cbiisc_read_reg(struct ncr53c9x_softc *sc, int reg)
{
	struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;

	return csc->sc_reg[reg * 4];
}

void
cbiisc_write_reg(struct ncr53c9x_softc *sc, int reg, u_char val)
{
	struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;
	u_char v = val;

	csc->sc_reg[reg * 4] = v;
#ifdef DEBUG
if (cbiisc_trace_enable/* && sc->sc_nexus && sc->sc_nexus->xs->xs_control & XS_CTL_POLL*/ &&
  reg == NCR_CMD/* && csc->sc_active*/) {
  cbiisc_trace[(cbiisc_trace_ptr - 1) & 127].yy = v;
/*  printf(" cmd %x", v);*/
}
#endif
}

int
cbiisc_dma_isintr(struct ncr53c9x_softc *sc)
{
	struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;

	if ((csc->sc_reg[NCR_STAT * 4] & NCRSTAT_INT) == 0)
		return 0;

	if (sc->sc_state == NCR_CONNECTED)
		csc->sc_reg[0x40] = CBIISC_PB_LED;
	else
		csc->sc_reg[0x40] = 0;

#ifdef DEBUG
if (/*sc->sc_nexus && sc->sc_nexus->xs->xs_control & XS_CTL_POLL &&*/ cbiisc_trace_enable) {
  cbiisc_trace[cbiisc_trace_ptr].status = csc->sc_reg[NCR_STAT * 4];
  cbiisc_trace[cbiisc_trace_ptr].xx = csc->sc_reg[NCR_CMD * 4];
  cbiisc_trace[cbiisc_trace_ptr].yy = csc->sc_active;
  cbiisc_trace_ptr = (cbiisc_trace_ptr + 1) & 127;
}
#endif
	return 1;
}

void
cbiisc_dma_reset(struct ncr53c9x_softc *sc)
{
	struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;

	csc->sc_active = 0;
}

int
cbiisc_dma_intr(struct ncr53c9x_softc *sc)
{
	register struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;
	register int	cnt;

	NCR_DMA(("cbiisc_dma_intr: cnt %d int %x stat %x fifo %d ",
	    csc->sc_dmasize, sc->sc_espintr, sc->sc_espstat,
	    csc->sc_reg[NCR_FFLAG * 4] & NCRFIFO_FF));
	if (csc->sc_active == 0) {
		printf("cbiisc_intr--inactive DMA\n");
		return -1;
	}

	/* update sc_dmaaddr and sc_pdmalen */
	cnt = csc->sc_reg[NCR_TCL * 4];
	cnt += csc->sc_reg[NCR_TCM * 4] << 8;
	cnt += csc->sc_reg[NCR_TCH * 4] << 16;
	if (!csc->sc_datain) {
		cnt += csc->sc_reg[NCR_FFLAG * 4] & NCRFIFO_FF;
		csc->sc_reg[NCR_CMD * 4] = NCRCMD_FLUSH;
	}
	cnt = csc->sc_dmasize - cnt;	/* number of bytes transferred */
	NCR_DMA(("DMA xferred %d\n", cnt));
	if (csc->sc_xfr_align) {
		bcopy(csc->sc_alignbuf, *csc->sc_dmaaddr, cnt);
		csc->sc_xfr_align = 0;
	}
	*csc->sc_dmaaddr += cnt;
	*csc->sc_pdmalen -= cnt;
	csc->sc_active = 0;
	return 0;
}

int
cbiisc_dma_setup(struct ncr53c9x_softc *sc, caddr_t *addr, size_t *len,
                 int datain, size_t *dmasize)
{
	struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;
	paddr_t pa;
	u_char *ptr;
	size_t xfer;

	csc->sc_dmaaddr = addr;
	csc->sc_pdmalen = len;
	csc->sc_datain = datain;
	csc->sc_dmasize = *dmasize;
	/*
	 * DMA can be nasty for high-speed serial input, so limit the
	 * size of this DMA operation if the serial port is running at
	 * a high speed (higher than 19200 for now - should be adjusted
	 * based on cpu type and speed?).
	 * XXX - add serial speed check XXX
	 */
	if (ser_open_speed > 19200 && cbiisc_max_dma != 0 &&
	    csc->sc_dmasize > cbiisc_max_dma)
		csc->sc_dmasize = cbiisc_max_dma;
	ptr = *addr;			/* Kernel virtual address */
	pa = kvtop(ptr);		/* Physical address of DMA */
	xfer = min(csc->sc_dmasize, PAGE_SIZE - (pa & (PAGE_SIZE - 1)));
	csc->sc_xfr_align = 0;
	/*
	 * If output and unaligned, stuff odd byte into FIFO
	 */
	if (datain == 0 && (int)ptr & 1) {
		NCR_DMA(("cbiisc_dma_setup: align byte written to fifo\n"));
		pa++;
		xfer--;			/* XXXX CHECK THIS !!!! XXXX */
		csc->sc_reg[NCR_FIFO * 4] = *ptr++;
	}
	/*
	 * If unaligned address, read unaligned bytes into alignment buffer
	 */
	else if ((int)ptr & 1) {
		pa = kvtop((caddr_t)&csc->sc_alignbuf);
		xfer = csc->sc_dmasize = min(xfer, sizeof (csc->sc_alignbuf));
		NCR_DMA(("cbiisc_dma_setup: align read by %d bytes\n", xfer));
		csc->sc_xfr_align = 1;
	}
++cbiisc_cnt_dma;		/* number of DMA operations */

	while (xfer < csc->sc_dmasize) {
		if ((pa + xfer) != kvtop(*addr + xfer))
			break;
		if ((csc->sc_dmasize - xfer) < PAGE_SIZE)
			xfer = csc->sc_dmasize;
		else
			xfer += PAGE_SIZE;
++cbiisc_cnt_dma3;
	}
if (xfer != *len)
  ++cbiisc_cnt_dma2;

	csc->sc_dmasize = xfer;
	*dmasize = csc->sc_dmasize;
	csc->sc_pa = pa;
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		if (csc->sc_xfr_align) {
			dma_cachectl(csc->sc_alignbuf,
			    sizeof(csc->sc_alignbuf));
		}
		else
			dma_cachectl(*csc->sc_dmaaddr, csc->sc_dmasize);
	}
#endif

	if (csc->sc_datain)
		pa &= ~1;
	else
		pa |= 1;
	csc->sc_dmabase[0] = (u_int8_t)(pa >> 24);
	csc->sc_dmabase[4] = (u_int8_t)(pa >> 16);
	csc->sc_dmabase[8] = (u_int8_t)(pa >> 8);
	csc->sc_dmabase[12] = (u_int8_t)(pa);
	csc->sc_active = 1;
	return 0;
}

void
cbiisc_dma_go(struct ncr53c9x_softc *sc)
{
}

void
cbiisc_dma_stop(struct ncr53c9x_softc *sc)
{
}

int
cbiisc_dma_isactive(struct ncr53c9x_softc *sc)
{
	struct cbiisc_softc *csc = (struct cbiisc_softc *)sc;

	return csc->sc_active;
}

#ifdef DEBUG
void
cbiisc_dump(void)
{
	int i;

	i = cbiisc_trace_ptr;
	printf("cbiisc_trace dump: ptr %x\n", cbiisc_trace_ptr);
	do {
		if (cbiisc_trace[i].hardbits == 0) {
			i = (i + 1) & 127;
			continue;
		}
		printf("%02x%02x%02x%02x(", cbiisc_trace[i].hardbits,
		    cbiisc_trace[i].status, cbiisc_trace[i].xx, cbiisc_trace[i].yy);
		if (cbiisc_trace[i].status & NCRSTAT_INT)
			printf("NCRINT/");
		if (cbiisc_trace[i].status & NCRSTAT_TC)
			printf("NCRTC/");
		switch(cbiisc_trace[i].status & NCRSTAT_PHASE) {
		case 0:
			printf("dataout"); break;
		case 1:
			printf("datain"); break;
		case 2:
			printf("cmdout"); break;
		case 3:
			printf("status"); break;
		case 6:
			printf("msgout"); break;
		case 7:
			printf("msgin"); break;
		default:
			printf("phase%d?", cbiisc_trace[i].status & NCRSTAT_PHASE);
		}
		printf(") ");
		i = (i + 1) & 127;
	} while (i != cbiisc_trace_ptr);
	printf("\n");
}
#endif
