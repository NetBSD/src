/*	$NetBSD: ncr_sbc.c,v 1.3 1996/03/17 01:33:33 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Scott Reynolds
 * Copyright (c) 1995 David Jones
 * Copyright (c) 1995 Allen Briggs
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
 * 3. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by David Jones, Allen
 *	Briggs and Scott Reynolds.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
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
 * (mac68k), David Jones (sun3), and Leo Weppelman (atari).  Allen
 * supplied some crucial interpretation of the NetBSD 1.1 'ncrscsi'
 * driver.  Allen, Gordon W. Ross, and Jason Thorpe all helped to
 * refine this code, and were considerable sources of moral support.
 *
 * The sbc_options code is based on similar code in Jason's modified
 * NetBSD/sparc 'si' driver.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_debug.h>
#include <scsi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include "ncr_sbcreg.h"
#include "../mac68k/via.h"

#ifdef SBCTEST
# define DEBUG
#endif

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth PDMA overhead)
 */
#define	MIN_DMA_LEN 128

/*
 * Transfers larger than 8192 bytes need to be split up
 * due to the size of the PDMA space.
 */
#define	MAX_DMA_LEN 0x2000

/*
 * From Guide to the Macintosh Family Hardware, p. 137
 * These are offsets from SCSIBase (see pmap_bootstrap.c)
 */
#define	SBC_REGISTER_OFFSET	0x10000
#define	SBC_DMA_DRQ_OFFSET	0x06000
#define	SBC_DMA_NODRQ_OFFSET	0x12000

#ifdef	DEBUG
#define	SBC_DB_INTR	0x01
#define	SBC_DB_DMA	0x02
#define	SBC_DB_BREAK	0x04

int	sbc_debug = SBC_DB_INTR | SBC_DB_DMA | SBC_DB_BREAK;
int	sbc_link_flags = 0 /* | SDEV_DB2 */;
#endif

/*
 * This structure is used to keep track of PDMA requests.
 */
struct sbc_pdma_handle {
	int	dh_flags;	/* flags */
#define	SBC_DH_BUSY	0x01	/* This DH is in use */
#define	SBC_DH_OUT	0x02	/* PDMA does data out (write) */
#define	SBC_DH_XFER	0x04	/* PDMA transfer not completed */
#define	SBC_DH_START	0x08	/* PDMA transfer not yet underway */
	u_char	*dh_addr;	/* data buffer */
	int	dh_len;		/* length of data buffer */
};

/*
 * The first structure member has to be the ncr5380_softc
 * so we can just cast to go back and forth between them.
 */
struct sbc_softc {
	struct ncr5380_softc ncr_sc;
	volatile struct sbc_regs *sc_regs;
	volatile long	*sc_drq_addr;
	volatile u_char	*sc_nodrq_addr;
	volatile u_char	*sc_ienable;
	volatile u_char	*sc_iflag;
	int		sc_options;	/* options for this instance. */
	struct sbc_pdma_handle sc_pdma[SCI_OPENINGS];
#ifdef DEBUG
	char		*sc_vstate;
#endif
};

/*
 * Options.  By default, (pseudo) DMA, DMA interrupts, and reselect are
 * enabled.  You may disable any of these features with the `flags'
 * directive in your kernel's configuration file.
 *
 * Alternatively, you can patch your kernel with DDB or some other
 * mechanism.  The sc_options member of the softc is OR'd with
 * the value in sbc_options.
 */     
#define	SBC_INTR	0x01	/* Allow SCSI IRQ/DRQ interrupts */
#define	SBC_RESELECT	0x02	/* Allow disconnect/reselect */
#define	SBC_OPTIONS_MASK	(SBC_INTR|SBC_RESELECT)
#define	SBC_OPTIONS_BITS	"\10\2RESELECT\1INTR"
int sbc_options = 0;

static	char sbc_name[] = "sbc";
static	int	sbc_match __P(());
static	void	sbc_attach __P(());
static	void	sbc_minphys __P((struct buf *bp));

static	int	sbc_wait_busy __P((struct ncr5380_softc *));
static	int	sbc_ready __P((struct ncr5380_softc *));
static	int	sbc_wait_dreq __P((struct ncr5380_softc *));
static	int	sbc_pdma_in __P((struct ncr5380_softc *, int, int, u_char *));
static	int	sbc_pdma_out __P((struct ncr5380_softc *, int, int, u_char *));

	void	sbc_intr_enable __P((struct ncr5380_softc *));
	void	sbc_intr_disable __P((struct ncr5380_softc *));
	void	sbc_irq_intr __P((void *));
	void	sbc_drq_intr __P((void *));
	void	sbc_dma_alloc __P((struct ncr5380_softc *));
	void	sbc_dma_free __P((struct ncr5380_softc *));
	void	sbc_dma_poll __P((struct ncr5380_softc *));
	void	sbc_dma_setup __P((struct ncr5380_softc *));
	void	sbc_dma_start __P((struct ncr5380_softc *));
	void	sbc_dma_eop __P((struct ncr5380_softc *));
	void	sbc_dma_stop __P((struct ncr5380_softc *));

static struct scsi_adapter	sbc_ops = {
	ncr5380_scsi_cmd,		/* scsi_cmd()		*/
	sbc_minphys,			/* scsi_minphys()	*/
	NULL,				/* open_target_lu()	*/
	NULL,				/* close_target_lu()	*/
};

/* This is copied from julian's bt driver */
/* "so we have a default dev struct for our link struct." */
static struct scsi_device sbc_dev = {
	NULL,		/* Use default error handler.	    */
	NULL,		/* Use default start handler.		*/
	NULL,		/* Use default async handler.	    */
	NULL,		/* Use default "done" routine.	    */
};

struct cfattach sbc_ca = {
	sizeof(struct sbc_softc), sbc_match, sbc_attach
};

struct cfdriver sbc_cd = {
	NULL, sbc_name, DV_DULL, NULL, 0,
};


static int
sbc_print(aux, name)
	void *aux;
	char *name;
{
	if (name != NULL)
		printf("%s: scsibus ", name);
	return UNCONF;
}

static int
sbc_match(parent, vcf, args)
	struct device	*parent;
	void		*vcf, *args;
{
	struct cfdata	*cf = vcf;
	struct confargs *ca = args;

	if (matchbyname(parent, cf, ca) == 0)
		return 0;
	if (!mac68k_machine.scsi80)
		return 0;
	if (cf->cf_unit != 0)
		return 0;
	return 1;
}

static void
sbc_attach(parent, self, args)
	struct device	*parent, *self;
	void		*args;
{
	static int probed = 0;
	struct sbc_softc *sc = (struct sbc_softc *) self;
	struct ncr5380_softc *ncr_sc = (struct ncr5380_softc *) sc;
	struct confargs *ca = args;
	extern vm_offset_t SCSIBase;
	int i;

	/* Pull in the options flags. */ 
	sc->sc_options =
	((ncr_sc->sc_dev.dv_cfdata->cf_flags | sbc_options) & SBC_OPTIONS_MASK);

	/*
	 * Set up base address of 5380
	 */
	sc->sc_regs = (struct sbc_regs *)(SCSIBase + SBC_REGISTER_OFFSET);

	/*
	 * Fill in the prototype scsi_link.
	 */
	ncr_sc->sc_link.adapter_softc = sc;
	ncr_sc->sc_link.adapter_target = 7;
	ncr_sc->sc_link.adapter = &sbc_ops;
	ncr_sc->sc_link.device = &sbc_dev;

	/*
	 * Initialize fields used by the MI code
	 */
	ncr_sc->sci_r0 = &sc->sc_regs->sci_pr0.sci_reg;
	ncr_sc->sci_r1 = &sc->sc_regs->sci_pr1.sci_reg;
	ncr_sc->sci_r2 = &sc->sc_regs->sci_pr2.sci_reg;
	ncr_sc->sci_r3 = &sc->sc_regs->sci_pr3.sci_reg;
	ncr_sc->sci_r4 = &sc->sc_regs->sci_pr4.sci_reg;
	ncr_sc->sci_r5 = &sc->sc_regs->sci_pr5.sci_reg;
	ncr_sc->sci_r6 = &sc->sc_regs->sci_pr6.sci_reg;
	ncr_sc->sci_r7 = &sc->sc_regs->sci_pr7.sci_reg;

	/*
	 * MD function pointers used by the MI code.
	 */
	ncr_sc->sc_pio_out   = sbc_pdma_out;
	ncr_sc->sc_pio_in    = sbc_pdma_in;
	ncr_sc->sc_dma_alloc = NULL;
	ncr_sc->sc_dma_free  = NULL;
	ncr_sc->sc_dma_poll  = NULL;
	ncr_sc->sc_intr_on   = NULL;
	ncr_sc->sc_intr_off  = NULL;
	ncr_sc->sc_dma_setup = NULL;
	ncr_sc->sc_dma_start = NULL;
	ncr_sc->sc_dma_eop   = NULL;
	ncr_sc->sc_dma_stop  = NULL;
	ncr_sc->sc_flags = 0;
	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * MD function pointers used by the MI code.
	 */
	if ((sc->sc_options & SBC_INTR) == 0) {
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;
	} else {
		if (sc->sc_options & SBC_RESELECT)
			ncr_sc->sc_flags |= NCR5380_PERMIT_RESELECT;
		ncr_sc->sc_dma_alloc = sbc_dma_alloc;
		ncr_sc->sc_dma_free  = sbc_dma_free;
		ncr_sc->sc_dma_poll  = sbc_dma_poll;
		ncr_sc->sc_dma_setup = sbc_dma_setup;
		ncr_sc->sc_dma_start = sbc_dma_start;
		ncr_sc->sc_dma_eop   = sbc_dma_eop;
		ncr_sc->sc_dma_stop  = sbc_dma_stop;
		mac68k_register_scsi_drq(sbc_drq_intr, ncr_sc);
		mac68k_register_scsi_irq(sbc_irq_intr, ncr_sc);
	}

	/*
	 * Initialize fields used only here in the MD code.
	 */
	sc->sc_drq_addr = (long *) (SCSIBase + SBC_DMA_DRQ_OFFSET);
	sc->sc_nodrq_addr = (u_char *) (SCSIBase + SBC_DMA_NODRQ_OFFSET);
	if (VIA2 == VIA2OFF) {
		sc->sc_ienable = Via1Base + VIA2 * 0x2000 + vIER;
		sc->sc_iflag   = Via1Base + VIA2 * 0x2000 + vIFR;
	} else {
		sc->sc_ienable = Via1Base + VIA2 * 0x2000 + rIER;
		sc->sc_iflag   = Via1Base + VIA2 * 0x2000 + rIFR;
	}

	if (sc->sc_options)
		printf(": options=%b", sc->sc_options, SBC_OPTIONS_BITS);
	printf("\n");

	/* Now enable SCSI interrupts through VIA2, if appropriate */
	if (sc->sc_options & SBC_INTR)
		sbc_intr_enable(ncr_sc);

#ifdef	DEBUG
	if (sbc_debug)
		printf("%s: softc=%x regs=%x\n", ncr_sc->sc_dev.dv_xname,
		    sc, sc->sc_regs);
	ncr_sc->sc_link.flags |= sbc_link_flags;
#endif

	/*
	 *  Initialize the SCSI controller itself.
	 */
	ncr5380_init(ncr_sc);
	ncr5380_reset_scsibus(ncr_sc);
	config_found(self, &(ncr_sc->sc_link), sbc_print);
}


static void
sbc_minphys(struct buf *bp)
{
	if (bp->b_bcount > MAX_DMA_LEN)
		bp->b_bcount = MAX_DMA_LEN;
	return (minphys(bp));
}


/***
 * General support for Mac-specific SCSI logic.
 ***/

/* These are used in the following inline functions. */
int sbc_wait_busy_timo = 1000 * 5000;	/* X2 = 10 S. */
int sbc_ready_timo = 1000 * 5000;	/* X2 = 10 S. */
int sbc_wait_dreq_timo = 1000 * 5000;	/* X2 = 10 S. */

/* Return zero on success. */
static __inline__ int
sbc_wait_busy(sc)
	struct ncr5380_softc *sc;
{
	register int timo = sbc_wait_busy_timo;
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

static __inline__ int
sbc_ready(sc)
	struct ncr5380_softc *sc;
{
	register int timo = sbc_ready_timo;

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

static __inline__ int
sbc_wait_dreq(sc)
	struct ncr5380_softc *sc;
{
	register int timo = sbc_wait_dreq_timo;

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


/***
 * Macintosh SCSI interrupt support routines.
 ***/

void
sbc_intr_enable(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	register struct sbc_softc *sc = (struct sbc_softc *) ncr_sc;
	int s;

	s = splhigh();
	*sc->sc_iflag   = (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	*sc->sc_ienable = 0x80 | (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	splx(s);
}

void
sbc_intr_disable(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	register struct sbc_softc *sc = (struct sbc_softc *) ncr_sc;
	int s;

	s = splhigh();
	*sc->sc_ienable = (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	splx(s);
}

void
sbc_irq_intr(p)
	void *p;
{
	register struct ncr5380_softc *ncr_sc = p;
	register int claimed = 0;

	/* How we ever arrive here without IRQ set is a mystery... */
	if (*ncr_sc->sci_csr & SCI_CSR_INT) {
		claimed = ncr5380_intr(ncr_sc);
		if (!claimed) {
			if (((*ncr_sc->sci_csr & ~SCI_CSR_PHASE_MATCH) == SCI_CSR_INT)
			    && ((*ncr_sc->sci_bus_csr & ~SCI_BUS_RST) == 0)) {
				SCI_CLR_INTR(ncr_sc);	/* RST interrupt */
			}
#ifdef DEBUG
			else {
				printf("%s: spurious intr\n",
				    ncr_sc->sc_dev.dv_xname);
# ifdef DDB
				if (sbc_debug & SBC_DB_BREAK)
					Debugger();	/* XXX */
# endif
			}
#endif
		}
	}
}


/***
 * The following code implements polled PDMA.
 ***/

static	int
sbc_pdma_out(ncr_sc, phase, count, data)
	struct ncr5380_softc *ncr_sc;
	int phase;
	int count;
	u_char *data;
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	register volatile long *long_data = sc->sc_drq_addr;
	register volatile u_char *byte_data = sc->sc_nodrq_addr;
	register int len = count;

	if (count < ncr_sc->sc_min_dma_len)
		return ncr5380_pio_out(ncr_sc, phase, count, data);

	if (sbc_wait_busy(ncr_sc) == 0) {
		*ncr_sc->sci_mode &= ~SCI_MODE_MONBSY;	/* XXX */
		*ncr_sc->sci_mode |= SCI_MODE_DMA;
		*ncr_sc->sci_icmd |= SCI_ICMD_DATA;
		*ncr_sc->sci_dma_send = 0;

#define W1	*byte_data = *data++
#define W4	*long_data = *((long*)data)++
		while (len >= 64) {
			if (sbc_ready(ncr_sc))
				goto timeout;
			W1;
			if (sbc_ready(ncr_sc))
				goto timeout;
			W1;
			if (sbc_ready(ncr_sc))
				goto timeout;
			W1;
			if (sbc_ready(ncr_sc))
				goto timeout;
			W1;
			if (sbc_ready(ncr_sc))
				goto timeout;
			W4; W4; W4; W4;
			W4; W4; W4; W4;
			W4; W4; W4; W4;
			W4; W4; W4;
			len -= 64;
		}
		while (len) {
			if (sbc_ready(ncr_sc))
				goto timeout;
			W1;
			len--;
		}
#undef  W1
#undef  W4
		if (sbc_wait_dreq(ncr_sc))
			printf("%s: timeout waiting for DREQ.\n",
			    ncr_sc->sc_dev.dv_xname);

		*byte_data = 0;

		SCI_CLR_INTR(ncr_sc);
		*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
		*ncr_sc->sci_icmd = 0;
	}
	return count - len;

timeout:
	printf("pdma_out: timeout len=%d count=%d\n", len, count);
	if ((*ncr_sc->sci_csr & SCI_CSR_PHASE_MATCH) == 0) {
		*ncr_sc->sci_icmd &= ~SCI_ICMD_DATA;
		--len;
	}

	SCI_CLR_INTR(ncr_sc);
	*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
	*ncr_sc->sci_icmd = 0;
	return count - len;
}

static	int
sbc_pdma_in(ncr_sc, phase, count, data)
	struct ncr5380_softc *ncr_sc;
	int phase;
	int count;
	u_char *data;
{
	struct sbc_softc *sc = (struct sbc_softc *)ncr_sc;
	register volatile long *long_data = sc->sc_drq_addr;
	register volatile u_char *byte_data = sc->sc_nodrq_addr;
	register int len = count;

	if (count < ncr_sc->sc_min_dma_len)
		return ncr5380_pio_in(ncr_sc, phase, count, data);

	if (sbc_wait_busy(ncr_sc) == 0) {
		*ncr_sc->sci_mode &= ~SCI_MODE_MONBSY;	/* XXX */
		*ncr_sc->sci_mode |= SCI_MODE_DMA;
		*ncr_sc->sci_icmd |= SCI_ICMD_DATA;
		*ncr_sc->sci_irecv = 0;

#define R4	*((long *)data)++ = *long_data
#define R1	*data++ = *byte_data
		while (len >= 1024) {
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 128 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 256 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 384 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 512 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 640 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 768 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 896 */
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 1024 */
			len -= 1024;
		}
		while (len >= 128) {
			if (sbc_ready(ncr_sc))
				goto timeout;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;
			R4; R4; R4; R4; R4; R4; R4; R4;		/* 128 */
			len -= 128;
		}
		while (len) {
			if (sbc_ready(ncr_sc))
				goto timeout;
			R1;
			len--;
		}
#undef R4
#undef R1
		SCI_CLR_INTR(ncr_sc);
		*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
		*ncr_sc->sci_icmd = 0;
	}
	return count - len;

timeout:
	printf("pdma_in: timeout len=%d count=%d\n", len, count);

	SCI_CLR_INTR(ncr_sc);
	*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
	*ncr_sc->sci_icmd = 0;
	return count - len;
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
sbc_drq_intr(p)
	void *p;
{
	extern	int		*nofault, mac68k_buserr_addr;
	register struct sbc_softc *sc = (struct sbc_softc *) p;
	register struct ncr5380_softc *ncr_sc = (struct ncr5380_softc *) p;
	register struct sci_req *sr = ncr_sc->sc_current;
	register struct sbc_pdma_handle *dh = sr->sr_dma_hand;
	label_t			faultbuf;
	register int		count;
	volatile u_int32_t	*long_drq;
	u_int32_t		*long_data;
	volatile u_int8_t	*drq;
	u_int8_t		*data;

	/*
	 * If we're not ready to xfer data, or have no more, just return.
	 */
	if ((*ncr_sc->sci_csr & SCI_CSR_DREQ) == 0 || dh->dh_len == 0) {
		dh->dh_flags &= ~SBC_DH_XFER;
		return;
	}

	dh->dh_flags &= ~SBC_DH_START;

#ifdef DEBUG
	sc->sc_vstate = "got drq interrupt.";
	if (sbc_debug & SBC_DB_INTR)
		printf("%s: drq intr, dh_len=%d, dh_flags=0x%x\n",
		    ncr_sc->sc_dev.dv_xname, dh->dh_len, dh->dh_flags);
#endif

	/*
	 * Setup for a possible bus error caused by SCSI controller
	 * switching out of DATA-IN/OUT before we're done with the
	 * current transfer.
	 */
	nofault = (int *) &faultbuf;

	if (setjmp((label_t *) nofault)) {
		nofault = (int *) 0;
#ifdef DEBUG
		sc->sc_vstate = "buserr in xfer.";
#endif
		count = (  (u_long) mac68k_buserr_addr
			 - (u_long) sc->sc_drq_addr);
		if ((count < 0) || (count > dh->dh_len)) {
			printf("%s: count = %d (pending count %d)\n",
			    ncr_sc->sc_dev.dv_xname, count, dh->dh_len);
			panic("something is wrong");
		}

		dh->dh_addr += count;
		dh->dh_len -= count;
		dh->dh_flags &= ~SBC_DH_XFER;

#ifdef DEBUG
		sc->sc_vstate = "handled bus error in xfer.";
#endif
		mac68k_buserr_addr = 0;

		ncr5380_intr(ncr_sc);
		return;
	}

	if (dh->dh_flags & SBC_DH_OUT) { /* Data Out */
		int	resid;

		/*
		 * Get the source address aligned.
		 */
		resid = count = min(dh->dh_len,
			4 - (((int) dh->dh_addr) & 0x3));
		if (count && count < 4) {
			data = (u_int8_t *) dh->dh_addr;
			drq = (u_int8_t *) sc->sc_drq_addr;
#define W1		*drq++ = *data++
			while (count) {
				W1; count--;
			}
#undef W1
			dh->dh_addr += resid;
			dh->dh_len -= resid;
		}

		/*
		 * Get ready to start the transfer.
		 */
		while (dh->dh_len) {
			int dcount;

			dcount = count = dh->dh_len;
			long_drq = (volatile u_int32_t *) sc->sc_drq_addr;
			long_data = (u_int32_t *) dh->dh_addr;

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
			data = (u_int8_t *) long_data;
			drq = (u_int8_t *) long_drq;
#define W1		*drq++ = *data++
			while (count) {
				W1; count--;
			}
#undef W1
			dh->dh_len -= dcount;
			dh->dh_addr += dcount;
		}
	} else {	/* Data In */
		int	resid;

		/*
		 * Get the dest address aligned.
		 */
		resid = count = min(dh->dh_len,
			4 - (((int) dh->dh_addr) & 0x3));
		if (count && count < 4) {
			data = (u_int8_t *) dh->dh_addr;
			drq = (u_int8_t *) sc->sc_drq_addr;
#define R1		*data++ = *drq++
			while (count) {
				R1; count--;
			}
#undef R1
			dh->dh_addr += resid;
			dh->dh_len -= resid;
		}

		/*
		 * Get ready to start the transfer.
		 */
		while (dh->dh_len) {
			int dcount;

			dcount = count = dh->dh_len;
			long_drq = (volatile u_int32_t *) sc->sc_drq_addr;
			long_data = (u_int32_t *) dh->dh_addr;

#define R4		*long_data++ = *long_drq++
			while (count >= 512) {
				if ((*ncr_sc->sci_csr & SCI_CSR_DREQ) == 0) {
					nofault = (int *) 0;

					dh->dh_addr += (dcount - count);
					dh->dh_len -= (dcount - count);
					dh->dh_flags &= ~SBC_DH_XFER;
#ifdef DEBUG
					sc->sc_vstate = "drq low";
#endif
					return;
				}
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;	/* 64 */
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;	/* 128 */
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;	/* 256 */
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;
				R4; R4; R4; R4; R4; R4; R4; R4;	/* 512 */
				count -= 512;
			}
			while (count >= 4) {
				R4; count -= 4;
			}
#undef R4
			data = (u_int8_t *) long_data;
			drq = (u_int8_t *) long_drq;
#define R1		*data++ = *drq++
			while (count) {
				R1; count--;
			}
#undef R1
			dh->dh_len -= dcount;
			dh->dh_addr += dcount;
		}
	}

	/*
	 * OK.  No bus error occurred above.  Clear the nofault flag
	 * so we no longer short-circuit bus errors.
	 */
	nofault = (int *) 0;

	dh->dh_flags &= ~SBC_DH_XFER;

#ifdef DEBUG
	sc->sc_vstate = "done in xfer.";
#endif
}

void
sbc_dma_alloc(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sbc_softc *sc = (struct sbc_softc *) ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsi_xfer *xs = sr->sr_xs;
	struct sbc_pdma_handle *dh;
	int		i, xlen;

#ifdef	DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("sbc_dma_alloc: already have DMA handle");
#endif

	/* Polled transfers shouldn't allocate a DMA handle. */
	if (sr->sr_flags & SR_IMMED)
		return;

	xlen = ncr_sc->sc_datalen;

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("sbc_dma_alloc: xlen=0x%x\n", xlen);

	/* XXX - should segment these instead */
	if (xlen > MAX_DMA_LEN) {
		printf("sbc_dma_alloc: excessive xlen=0x%x\n", xlen);
#ifdef DDB
		Debugger();
#endif
		ncr_sc->sc_datalen = xlen = MAX_DMA_LEN;
	}

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
	if (xs->flags & SCSI_DATA_OUT)
		dh->dh_flags |= SBC_DH_OUT;

	sr->sr_dma_hand = dh;
}

void
sbc_dma_free(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;

#ifdef	DIAGNOSTIC
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
sbc_dma_poll(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sbc_softc *sc = (struct sbc_softc *) ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;
	volatile struct sbc_regs *sbc = sc->sc_regs;
	register int s;
	register int timo;

	/*
	 * If we haven't started the transfer yet, DO IT NOW!
	 */
	if (dh->dh_flags & SBC_DH_START)
		sbc_drq_intr(ncr_sc);

	timo = 50000;	/* X100 = 5 sec. */
	for (;;) {
		if ((dh->dh_flags & SBC_DH_XFER) == 0)
			break;
		if (--timo <= 0) {
			printf("%s: PDMA didn't complete (while polling)\n",
			    ncr_sc->sc_dev.dv_xname);
			sr->sr_flags |= SR_OVERDUE;
			break;
		}
		delay(100);
	}

#ifdef	DEBUG
	if (sbc_debug & SBC_DB_DMA)
		printf("%s: poll done, csr=0x%x, bus_csr=0x%x\n",
		    ncr_sc->sc_dev.dv_xname, *ncr_sc->sci_csr,
		    *ncr_sc->sci_bus_csr);
#endif
}

void
sbc_dma_setup(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	/* Not needed; we don't have real DMA */
}

void
sbc_dma_start(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;

	/*
	 * Match bus phase, set DMA mode, and assert data bus,
	 * then start the transfer.
	 */
	if (dh->dh_flags & SBC_DH_OUT) {
		*ncr_sc->sci_tcmd = PHASE_DATA_OUT;
		SCI_CLR_INTR(ncr_sc);
		*ncr_sc->sci_icmd |= SCI_ICMD_DATA;
		*ncr_sc->sci_mode &= ~SCI_MODE_MONBSY;	/* XXX */
		*ncr_sc->sci_mode |= SCI_MODE_DMA;
		*ncr_sc->sci_dma_send = 0;
	} else {
		*ncr_sc->sci_tcmd = PHASE_DATA_IN;
		SCI_CLR_INTR(ncr_sc);
		*ncr_sc->sci_icmd = 0;
		*ncr_sc->sci_mode &= ~SCI_MODE_MONBSY;	/* XXX */
		*ncr_sc->sci_mode |= SCI_MODE_DMA;
		*ncr_sc->sci_irecv = 0;
	}

	/*
	 * Set the SBC_DH_XFER flag so that sbc_dma_poll() will wait
	 * even if the SCSI DRQ service routine hasn't been serviced yet.
	 */
	dh->dh_flags |= (SBC_DH_XFER | SBC_DH_START);;

	ncr_sc->sc_state |= NCR_DOINGDMA;

#ifdef	DEBUG
	if (sbc_debug & SBC_DB_DMA)
		printf("sbc_dma_start: started, va=0x%lx, xlen=0x%x\n",
			(long)dh->dh_addr, dh->dh_len);
#endif
}

void
sbc_dma_eop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	/* Not needed; the EOP pin is wired high (GMFH, pp. 389-390) */
}

void
sbc_dma_stop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sbc_softc *sc = (struct sbc_softc *) ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct sbc_pdma_handle *dh = sr->sr_dma_hand;
	register int ntrans;

	if ((ncr_sc->sc_state & NCR_DOINGDMA) == 0) {
#ifdef DEBUG
		printf("sbc_dma_stop: DMA not running\n");
#endif
		return;
	}
	ncr_sc->sc_state &= ~NCR_DOINGDMA;

	if (ncr_sc->sc_state & NCR_ABORTING)
		goto out;

	ntrans = ncr_sc->sc_datalen - dh->dh_len;

#ifdef DEBUG
	if (sbc_debug & SBC_DB_DMA)
		printf("sbc_dma_stop: ntrans=0x%x\n", ntrans);
#endif

	if (ntrans > ncr_sc->sc_datalen)
		panic("sbc_dma_stop: excess transfer\n");

	/* Adjust data pointer */
	ncr_sc->sc_dataptr += ntrans;
	ncr_sc->sc_datalen -= ntrans;

	/* Clear any pending interrupts. */
	SCI_CLR_INTR(ncr_sc);

out:
	/* Put SBIC back into PIO mode. */
	*ncr_sc->sci_mode &= ~SCI_MODE_DMA;
	*ncr_sc->sci_icmd = 0;

	printf("%s: exit dma_stop, csr=0x%x, bus_csr=0x%x\n",
	    ncr_sc->sc_dev.dv_xname, *ncr_sc->sci_csr,
	    *ncr_sc->sci_bus_csr);
}
