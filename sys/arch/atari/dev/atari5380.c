/*	$NetBSD: atari5380.c,v 1.42.44.2 2009/07/18 14:52:52 yamt Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: atari5380.c,v 1.42.44.2 2009/07/18 14:52:52 yamt Exp $");

#include "opt_atariscsi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsiconf.h>

#include <uvm/uvm_extern.h>

#include <m68k/asm_single.h>
#include <m68k/cpu.h>
#include <m68k/cacheops.h>

#include <atari/atari/stalloc.h>

/*
 * Include the driver definitions
 */
#include <atari/dev/ncr5380reg.h>

#include <machine/stdarg.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/intr.h>

#if defined(FALCON_SCSI)
#include <machine/dma.h>
#endif

/*
 * Set the various driver options
 */
#define	NREQ		18	/* Size of issue queue			*/
#define	AUTO_SENSE	1	/* Automatically issue a request-sense 	*/

#define	DRNAME		ncrscsi	/* used in various prints		*/
#undef	DBG_SEL			/* Show the selection process		*/
#undef	DBG_REQ			/* Show enqueued/ready requests		*/
#undef	DBG_ERR_RET		/* Show requests with != 0 return code	*/
#undef	DBG_NOWRITE		/* Do not allow writes to the targets	*/
#undef	DBG_PIO			/* Show the polled-I/O process		*/
#undef	DBG_INF			/* Show information transfer process	*/
#define	DBG_NOSTATIC		/* No static functions, all in DDB trace*/
#define	DBG_PID		15	/* Keep track of driver			*/
#define	REAL_DMA		/* Use DMA if sensible			*/
#if defined(FALCON_SCSI)
#define	REAL_DMA_POLL	1	/* 1: Poll for end of DMA-transfer	*/
#else
#define	REAL_DMA_POLL	0	/* 1: Poll for end of DMA-transfer	*/
#endif
#undef	USE_PDMA		/* Use special pdma-transfer function	*/
#define MIN_PHYS	65536	/*BARF!!!!*/

/*
 * Include more driver definitions
 */
#include <atari/dev/ncr5380var.h>

/*
 * The atari specific driver options
 */
#undef	NO_TTRAM_DMA		/* Do not use DMA to TT-ram. This	*/
				/*    fails on older atari's		*/
#define	ENABLE_NCR5380(sc)	cur_softc = sc;

static u_char	*alloc_bounceb(u_long);
static void	free_bounceb(u_char *);
static int	machine_match(struct device *, void *, void *,
							struct cfdriver *);
       void	scsi_ctrl(int);
       void	scsi_dma(int);

/*
 * Functions that do nothing on the atari
 */
#define	pdma_ready()		0

#if defined(TT_SCSI)

void	ncr5380_drq_intr(int);

/*
 * Define all the things we need of the DMA-controller
 */
#define	SCSI_DMA	((struct scsi_dma *)AD_SCSI_DMA)
#define	SCSI_5380	((struct scsi_5380 *)AD_NCR5380)

struct scsi_dma {
	volatile u_char		s_dma_ptr[8];	/* use only the odd bytes */
	volatile u_char		s_dma_cnt[8];	/* use only the odd bytes */
	volatile u_char		s_dma_res[4];	/* data residue register  */
	volatile u_char		s_dma_gap;	/* not used		  */
	volatile u_char		s_dma_ctrl;	/* control register	  */
	volatile u_char		s_dma_gap2;	/* not used		  */
	volatile u_char		s_hdma_ctrl;	/* Hades control register */
};

static inline void set_scsi_dma(volatile u_char *, u_long);
static inline u_long get_scsi_dma(volatile u_char *);

static inline void
set_scsi_dma(volatile u_char *addr, u_long val)
{
	volatile u_char *address;

	address = addr + 1;
	__asm("movepl	%0, %1@(0)": :"d" (val), "a" (address));
}

static inline u_long
get_scsi_dma(volatile u_char *addr)
{
	volatile u_char	*address;
	u_long	nval;

	address = addr + 1;
	__asm("movepl	%1@(0), %0": "=d" (nval) : "a" (address));

	return nval;
}

/*
 * Defines for TT-DMA control register
 */
#define	SD_BUSERR	0x80		/* 1 = transfer caused bus error*/
#define	SD_ZERO		0x40		/* 1 = byte counter is zero	*/
#define	SD_ENABLE	0x02		/* 1 = Enable DMA		*/
#define	SD_OUT		0x01		/* Direction: memory to SCSI	*/
#define	SD_IN		0x00		/* Direction: SCSI to memory	*/

/*
 * Defines for Hades-DMA control register
 */
#define	SDH_BUSERR	0x02		/* 1 = Bus error		*/
#define SDH_EOP		0x01		/* 1 = Signal EOP on 5380	*/
#define	SDH_ZERO	0x40		/* 1 = Byte counter is zero	*/

/*
 * Define the 5380 register set
 */
struct scsi_5380 {
	volatile u_char	scsi_5380[16];	/* use only the odd bytes	*/
};
#endif /* TT_SCSI */

/**********************************************
 * Variables present for both TT and Falcon.  *
 **********************************************/

/*
 * Softc of currently active controller (a bit of fake; we only have one)
 */
static struct ncr_softc	*cur_softc;

#if defined(TT_SCSI) && !defined(FALCON_SCSI)
/*
 * We can be more efficient for some functions when only TT_SCSI is selected
 */
#define	GET_5380_REG(rnum)	SCSI_5380->scsi_5380[(rnum << 1) | 1]
#define	SET_5380_REG(rnum,val)	(SCSI_5380->scsi_5380[(rnum << 1) | 1] = val)

#define scsi_mach_init(sc)	scsi_tt_init(sc)
#define scsi_ienable()		scsi_tt_ienable()
#define scsi_idisable()		scsi_tt_idisable()
#define	scsi_clr_ipend()	scsi_tt_clr_ipend()
#define scsi_ipending()		(GET_5380_REG(NCR5380_DMSTAT) & SC_IRQ_SET)
#define scsi_dma_setup(r,p,m)	scsi_tt_dmasetup(r, p, m)
#define wrong_dma_range(r,d)	tt_wrong_dma_range(r, d)
#define poll_edma(reqp)		tt_poll_edma(reqp)
#define get_dma_result(r, b)	tt_get_dma_result(r, b)
#define	can_access_5380()	1
#define emulated_dma()		((machineid & ATARI_HADES) ? 1 : 0)

#define fair_to_keep_dma()	1
#define claimed_dma()		1
#define reconsider_dma()

#endif /* defined(TT_SCSI) && !defined(FALCON_SCSI) */

#if defined(TT_SCSI)

/*
 * Prototype functions defined below 
 */
#ifdef NO_TTRAM_DMA
static int tt_wrong_dma_range(SC_REQ *, struct dma_chain *);
#endif
static void	scsi_tt_init(struct ncr_softc *);
static u_char	get_tt_5380_reg(u_short);
static void	set_tt_5380_reg(u_short, u_short);
static void	scsi_tt_dmasetup(SC_REQ *, u_int, u_char);
static int	tt_poll_edma(SC_REQ *);
static u_char	*ptov(SC_REQ *, u_long*);
static int	tt_get_dma_result(SC_REQ *, u_long *);
       void	scsi_tt_ienable(void);
       void	scsi_tt_idisable(void);
       void	scsi_tt_clr_ipend(void);

/*
 * Define these too, so we can use them locally...
 */
#define	GET_TT_REG(rnum)	SCSI_5380->scsi_5380[(rnum << 1) | 1]
#define	SET_TT_REG(rnum,val)	(SCSI_5380->scsi_5380[(rnum << 1) | 1] = val)

#ifdef NO_TTRAM_DMA
static int
tt_wrong_dma_range(SC_REQ *reqp, struct dma_chain *dm)
{
	if (dm->dm_addr & 0xff000000) {
		reqp->dr_flag |= DRIVER_BOUNCING;
		return(1);
	}
	return(0);
}
#else
#define	tt_wrong_dma_range(reqp, dm)	0
#endif

static void
scsi_tt_init(struct ncr_softc *sc)
{
	/*
	 * Enable SCSI-related interrupts
	 */
	MFP2->mf_aer  |= 0x80;		/* SCSI IRQ goes HIGH!!!!!	*/

	if (machineid & ATARI_TT) {
		/* SCSI-DMA interrupts		*/
		MFP2->mf_ierb |= IB_SCDM;
		MFP2->mf_iprb  = (u_int8_t)~IB_SCDM;
		MFP2->mf_imrb |= IB_SCDM;
	}
	else if (machineid & ATARI_HADES) {
		SCSI_DMA->s_hdma_ctrl = 0;

		if (intr_establish(2, AUTO_VEC, 0,
					(hw_ifun_t)ncr5380_drq_intr,
					NULL) == NULL)
		panic("scsi_tt_init: Can't establish drq-interrupt");
	}
	else panic("scsi_tt_init: should not come here");

	MFP2->mf_iera |= IA_SCSI;	/* SCSI-5380 interrupts		*/
	MFP2->mf_ipra  = (u_int8_t)~IA_SCSI;
	MFP2->mf_imra |= IA_SCSI;

	/*
	 * LWP: DMA transfers to TT-ram causes data to be garbled
	 * without notice on some TT-mainboard revisions.
	 * If programs generate mysterious Segmentations faults,
	 * try enabling NO_TTRAM_DMA.
	 */
#ifdef NO_TTRAM_DMA
	printf(": DMA to TT-RAM is disabled!");
#endif
}

static u_char
get_tt_5380_reg(u_short rnum)
{
	return(SCSI_5380->scsi_5380[(rnum << 1) | 1]);
}

static void
set_tt_5380_reg(u_short rnum, u_short val)
{
	SCSI_5380->scsi_5380[(rnum << 1) | 1] = val;
}

extern inline void
scsi_tt_ienable(void)
{
	if (machineid & ATARI_TT)
		single_inst_bset_b(MFP2->mf_imrb, IB_SCDM);
	single_inst_bset_b(MFP2->mf_imra, IA_SCSI);
}

extern inline void
scsi_tt_idisable(void)
{
	if (machineid & ATARI_TT)
		single_inst_bclr_b(MFP2->mf_imrb, IB_SCDM);
	single_inst_bclr_b(MFP2->mf_imra, IA_SCSI);
}

extern inline void
scsi_tt_clr_ipend(void)
{
	int	tmp;

	SCSI_DMA->s_dma_ctrl = 0;
	tmp = GET_TT_REG(NCR5380_IRCV);
	if (machineid & ATARI_TT)
		MFP2->mf_iprb = (u_int8_t)~IB_SCDM;
	MFP2->mf_ipra = (u_int8_t)~IA_SCSI;

	/*
	 * Remove interrupts already scheduled.
	 */
	rem_sicallback((si_farg)ncr_ctrl_intr);
	rem_sicallback((si_farg)ncr_dma_intr);
}

static void
scsi_tt_dmasetup(SC_REQ *reqp, u_int phase, u_char	mode)
{
	if (PH_IN(phase)) {
		SCSI_DMA->s_dma_ctrl = SD_IN;
		if (machineid & ATARI_HADES)
		    SCSI_DMA->s_hdma_ctrl &= ~(SDH_BUSERR|SDH_EOP);
		set_scsi_dma(SCSI_DMA->s_dma_ptr, reqp->dm_cur->dm_addr);
		set_scsi_dma(SCSI_DMA->s_dma_cnt, reqp->dm_cur->dm_count);
		SET_TT_REG(NCR5380_ICOM, 0);
		SET_TT_REG(NCR5380_MODE, mode);
		SCSI_DMA->s_dma_ctrl = SD_ENABLE;
		SET_TT_REG(NCR5380_IRCV, 0);
	}
	else {
		SCSI_DMA->s_dma_ctrl = SD_OUT;
		if (machineid & ATARI_HADES)
		    SCSI_DMA->s_hdma_ctrl &= ~(SDH_BUSERR|SDH_EOP);
		set_scsi_dma(SCSI_DMA->s_dma_ptr, reqp->dm_cur->dm_addr);
		set_scsi_dma(SCSI_DMA->s_dma_cnt, reqp->dm_cur->dm_count);
		SET_TT_REG(NCR5380_MODE, mode);
		SET_TT_REG(NCR5380_ICOM, SC_ADTB);
		SET_TT_REG(NCR5380_DMSTAT, 0);
		SCSI_DMA->s_dma_ctrl = SD_ENABLE|SD_OUT;
	}
}

static int
tt_poll_edma(SC_REQ *reqp)
{
	u_char	dmstat, dmastat;
	int	timeout = 9000; /* XXX */

	/*
	 * We wait here until the DMA has finished. This can be
	 * achieved by checking the following conditions:
	 *   - 5380:
	 *	- End of DMA flag is set
	 *	- We lost BSY (error!!)
	 *	- A phase mismatch has occurred (partial transfer)
	 *   - DMA-controller:
	 *	- A bus error occurred (Kernel error!!)
	 *	- All bytes are transferred
	 * If one of the terminating conditions was met, we call
	 * 'dma_ready' to check errors and perform the bookkeeping.
	 */

	scsi_tt_idisable();
	for (;;) {
		delay(20);
		if (--timeout <= 0) {
			ncr_tprint(reqp, "timeout on polled transfer\n");
			reqp->xs->error = XS_TIMEOUT;
			scsi_tt_ienable();
			return(0);
		}
		dmstat  = GET_TT_REG(NCR5380_DMSTAT);

		if ((machineid & ATARI_HADES) && (dmstat & SC_DMA_REQ)) {
			ncr5380_drq_intr(1);
			dmstat  = GET_TT_REG(NCR5380_DMSTAT);
		}

		dmastat = SCSI_DMA->s_dma_ctrl;
		if (dmstat & (SC_END_DMA|SC_BSY_ERR|SC_IRQ_SET))
			break;
		if (!(dmstat & SC_PHS_MTCH))
			break;
		if (dmastat & (SD_BUSERR|SD_ZERO))
			break;
	}
	scsi_tt_ienable();
	return(1);
}

/*
 * Convert physical DMA address to a virtual address.
 */
static u_char *
ptov(SC_REQ *reqp, u_long *phaddr)
{
	struct dma_chain	*dm;
	u_char			*vaddr;

	dm = reqp->dm_chain;
	vaddr = reqp->xdata_ptr;
	for(; dm < reqp->dm_cur; dm++)
		vaddr += dm->dm_count;
	vaddr += (u_long)phaddr - dm->dm_addr;
	return(vaddr);
}

static int
tt_get_dma_result(SC_REQ *reqp, u_long *bytes_left)
{
	int	dmastat, dmstat;
	u_char	*byte_p;
	u_long	leftover;

	dmastat = SCSI_DMA->s_dma_ctrl;
	dmstat  = GET_TT_REG(NCR5380_DMSTAT);
	leftover = get_scsi_dma(SCSI_DMA->s_dma_cnt);
	byte_p = (u_char *)get_scsi_dma(SCSI_DMA->s_dma_ptr);

	if (dmastat & SD_BUSERR) {
		/*
		 * The DMA-controller seems to access 8 bytes beyond
		 * it's limits on output. Therefore check also the byte
		 * count. If it's zero, ignore the bus error.
		 */
		if (leftover != 0) {
			ncr_tprint(reqp,
				"SCSI-DMA buserror - accessing 0x%x\n", byte_p);
			reqp->xs->error = XS_DRIVER_STUFFUP;
		}
	}

	/*
	 * We handle the following special condition below:
	 *  -- The device disconnects in the middle of a write operation --
	 * In this case, the 5380 has already pre-fetched the next byte from
	 * the DMA-controller before the phase mismatch occurs. Therefore,
	 * leftover is 1 too low.
	 * This does not always happen! Therefore, we only do this when
	 * leftover is odd. This assumes that DMA transfers are _even_! This
	 * is normally the case on disks and types but might not always be.
	 * XXX: Check if ACK is consistently high on these occasions LWP
	 */
	if ((leftover & 1) && !(dmstat & SC_PHS_MTCH) && PH_OUT(reqp->phase))
		leftover++;

	/*
	 * Check if there are some 'restbytes' left in the DMA-controller.
	 */
	if ((machineid & ATARI_TT) && ((u_long)byte_p & 3)
	    && PH_IN(reqp->phase)) {
		u_char	*p;
		volatile u_char *q;

		p = ptov(reqp, (u_long *)((u_long)byte_p & ~3));
		q = SCSI_DMA->s_dma_res;
		switch ((u_long)byte_p & 3) {
			case 3: *p++ = *q++;
			case 2: *p++ = *q++;
			case 1: *p++ = *q++;
		}
	}
	*bytes_left = leftover;
	return ((dmastat & (SD_BUSERR|SD_ZERO)) ? 1 : 0);
}

static u_char *dma_ptr;
void
ncr5380_drq_intr(int poll)
{
extern	int			*nofault;
	label_t			faultbuf;
	int			write;
	u_long	 		count;
	volatile u_char		*data_p = (volatile u_char *)(stio_addr+0x741);

	/*
	 * Block SCSI interrupts while emulating DMA. They come
	 * at a higher priority.
	 */
	single_inst_bclr_b(MFP2->mf_imra, IA_SCSI);

	/*
	 * Setup for a possible bus error caused by SCSI controller
	 * switching out of DATA-IN/OUT before we're done with the
	 * current transfer.
	 */
	nofault = (int *) &faultbuf;

	if (setjmp((label_t *) nofault)) {
		u_long	cnt, tmp;

		PID("drq berr");
		nofault = (int *) 0;

		/*
		 * Determine number of bytes transferred
		 */
		cnt = (u_long)dma_ptr - get_scsi_dma(SCSI_DMA->s_dma_ptr);

		if (cnt != 0) {
			/*
			 * Update the DMA pointer/count fields
			 */
			set_scsi_dma(SCSI_DMA->s_dma_ptr, (u_long)dma_ptr);
			tmp = get_scsi_dma(SCSI_DMA->s_dma_cnt);
			set_scsi_dma(SCSI_DMA->s_dma_cnt, tmp - cnt);

			if (tmp > cnt) {
				/*
				 * Still more to transfer
				 */
				if (!poll)
				   single_inst_bset_b(MFP2->mf_imra, IA_SCSI);
				return;
			}

			/*
			 * Signal EOP to 5380
			 */
			SCSI_DMA->s_hdma_ctrl |= SDH_EOP;
		}
		else {
			nofault = (int *) &faultbuf;

			/*
			 * Try to figure out if the byte-count was
			 * zero because there was no (more) data or
			 * because the dma_ptr is bogus.
			 */
			if (setjmp((label_t *) nofault)) {
				/*
				 * Set the bus-error bit
				 */
				SCSI_DMA->s_hdma_ctrl |= SDH_BUSERR;
			}
			__asm volatile ("tstb	%0@(0)": : "a" (dma_ptr));
			nofault = (int *)0;
		}

		/*
		 * Schedule an interrupt
		 */
		if (!poll && (SCSI_DMA->s_dma_ctrl & SD_ENABLE))
		    add_sicallback((si_farg)ncr_dma_intr, (void *)cur_softc, 0);

		/*
		 * Clear DMA-mode
		 */
		SCSI_DMA->s_dma_ctrl &= ~SD_ENABLE;
		if (!poll)
			single_inst_bset_b(MFP2->mf_imra, IA_SCSI);

		return;
	}

	write = (SCSI_DMA->s_dma_ctrl & SD_OUT) ? 1 : 0;
#if DBG_PID
	if (write) {
		PID("drq (in)");
	} else {
		PID("drq (out)");
	}
#endif

	count = get_scsi_dma(SCSI_DMA->s_dma_cnt);
	dma_ptr = (u_char *)get_scsi_dma(SCSI_DMA->s_dma_ptr);

	/*
	 * Keep pushing bytes until we're done or a bus-error
	 * signals that the SCSI controller is not ready.
	 * NOTE: I tried some optimalizations in these loops,
	 *       but they had no effect on transfer speed.
	 */
	if (write) {
		while(count--) {
			*data_p = *dma_ptr++;
		}
	}
	else {
		while(count--) {
			*dma_ptr++ = *data_p;
		}
	}

	/*
	 * OK.  No bus error occurred above.  Clear the nofault flag
	 * so we no longer short-circuit bus errors.
	 */
	nofault = (int *) 0;

	/*
	 * Schedule an interrupt
	 */
	if (!poll && (SCSI_DMA->s_dma_ctrl & SD_ENABLE))
	    add_sicallback((si_farg)ncr_dma_intr, (void *)cur_softc, 0);

	/*
	 * Clear DMA-mode
	 */
	SCSI_DMA->s_dma_ctrl &= ~SD_ENABLE;

	/*
	 * Update the DMA 'registers' to reflect that all bytes
	 * have been transfered and tell this to the 5380 too.
	 */
	set_scsi_dma(SCSI_DMA->s_dma_ptr, (u_long)dma_ptr);
	set_scsi_dma(SCSI_DMA->s_dma_cnt, 0);
	SCSI_DMA->s_hdma_ctrl |= SDH_EOP;

	PID("end drq");
	if (!poll)
		single_inst_bset_b(MFP2->mf_imra, IA_SCSI);
	
	return;
}

#endif /* defined(TT_SCSI) */

#if defined(FALCON_SCSI) && !defined(TT_SCSI)

#define	GET_5380_REG(rnum)	get_falcon_5380_reg(rnum)
#define	SET_5380_REG(rnum,val)	set_falcon_5380_reg(rnum, val)
#define scsi_mach_init(sc)	scsi_falcon_init(sc)
#define scsi_ienable()		scsi_falcon_ienable()
#define scsi_idisable()		scsi_falcon_idisable()
#define	scsi_clr_ipend()	scsi_falcon_clr_ipend()
#define	scsi_ipending()		scsi_falcon_ipending()
#define scsi_dma_setup(r,p,m)	scsi_falcon_dmasetup(r, p, m)
#define wrong_dma_range(r,d)	falcon_wrong_dma_range(r, d)
#define poll_edma(reqp)		falcon_poll_edma(reqp)
#define get_dma_result(r, b)	falcon_get_dma_result(r, b)
#define	can_access_5380()	falcon_can_access_5380()
#define	emulated_dma()		0

#define fair_to_keep_dma()	(!st_dmawanted())
#define claimed_dma()		falcon_claimed_dma()
#define reconsider_dma()	falcon_reconsider_dma()

#endif /* defined(FALCON_SCSI) && !defined(TT_SCSI) */

#if defined(FALCON_SCSI)

/*
 * Prototype functions defined below 
 */
static void	scsi_falcon_init(struct ncr_softc *);
static u_char	get_falcon_5380_reg(u_short);
static void	set_falcon_5380_reg(u_short, u_short);
static int	falcon_wrong_dma_range(SC_REQ *, struct dma_chain *);
static void	fal1_dma(u_int, u_int, SC_REQ *);
static void	scsi_falcon_dmasetup(SC_REQ  *, u_int, u_char);
static int	falcon_poll_edma(SC_REQ  *);
static int	falcon_get_dma_result(SC_REQ  *, u_long *);
       int	falcon_can_access_5380(void);
       void	scsi_falcon_clr_ipend(void);
       void	scsi_falcon_idisable(void);
       void	scsi_falcon_ienable(void);
       int	scsi_falcon_ipending(void);
       int	falcon_claimed_dma(void);
       void	falcon_reconsider_dma(void);

static void
scsi_falcon_init(struct ncr_softc *sc)
{
	/*
	 * Enable disk related interrupts
	 */
	MFP->mf_ierb  |= IB_DINT;
	MFP->mf_iprb   = (u_int8_t)~IB_DINT;
	MFP->mf_imrb  |= IB_DINT;
}

static u_char
get_falcon_5380_reg(u_short rnum)
{
	DMA->dma_mode = DMA_SCSI + rnum;
	return(DMA->dma_data);
}

static void
set_falcon_5380_reg(u_short rnum, u_short val)
{
	DMA->dma_mode = DMA_SCSI + rnum;
	DMA->dma_data = val;
}

extern inline void
scsi_falcon_ienable(void)
{
	single_inst_bset_b(MFP->mf_imrb, IB_DINT);
}

extern inline void
scsi_falcon_idisable(void)
{
	single_inst_bclr_b(MFP->mf_imrb, IB_DINT);
}

extern inline void
scsi_falcon_clr_ipend(void)
{
	int	tmp;

	tmp = get_falcon_5380_reg(NCR5380_IRCV);
	rem_sicallback((si_farg)ncr_ctrl_intr);
}

extern inline int
scsi_falcon_ipending(void)
{
	if (connected && (connected->dr_flag & DRIVER_IN_DMA)) {
		/*
		 *  XXX: When DMA is running, we are only allowed to
		 *       check the 5380 when DMA _might_ be finished.
		 */
		if (MFP->mf_gpip & IO_DINT)
		    return (0); /* XXX: Actually: we're not allowed to check */

		/* LWP: 28-06, must be a DMA interrupt! should the
		 * ST-DMA unit be taken out of DMA mode?????
		 */
		DMA->dma_mode = 0x90;

	}
	return(get_falcon_5380_reg(NCR5380_DMSTAT) & SC_IRQ_SET);
}

static int
falcon_wrong_dma_range(SC_REQ *reqp, struct dma_chain *dm)
{
	/*
	 * Do not allow chains yet! See also comment with
	 * falcon_poll_edma() !!!
	 */
	if (((dm - reqp->dm_chain) > 0) || (dm->dm_addr & 0xff000000)) {
		reqp->dr_flag |= DRIVER_BOUNCING;
		return(1);
	}
	/*
	 * Never allow DMA to happen on a Falcon when the transfer
	 * size is no multiple of 512. This is the transfer unit of the
	 * ST DMA-controller.
	 */
	if(dm->dm_count & 511)
		return(1);
	return(0);
}

static	int falcon_lock = 0;

extern inline int
falcon_claimed_dma(void)
{
	if (falcon_lock != DMA_LOCK_GRANT) {
		if (falcon_lock == DMA_LOCK_REQ) {
			/*
			 * DMA access is being claimed.
			 */
			return(0);
		}
		if (!st_dmagrab((dma_farg)ncr_ctrl_intr, (dma_farg)run_main,
						cur_softc, &falcon_lock, 1))
			return(0);
	}
	return(1);
}

extern inline void
falcon_reconsider_dma(void)
{
	if (falcon_lock && (connected == NULL) && (discon_q == NULL)) {
		/*
		 * No need to keep DMA locked by us as we are not currently
		 * connected and no disconnected jobs are pending.
		 */
		rem_sicallback((si_farg)ncr_ctrl_intr);
		st_dmafree(cur_softc, &falcon_lock);
	}

	if (!falcon_lock && (issue_q != NULL)) {
		/*
		 * We must (re)claim DMA access as there are jobs
		 * waiting in the issue queue.
		 */
		st_dmagrab((dma_farg)ncr_ctrl_intr, (dma_farg)run_main,
						cur_softc, &falcon_lock, 0);
	}
}

static void
fal1_dma(u_int dir, u_int nsects, SC_REQ *reqp)
{
	dir <<= 8;
	st_dmaaddr_set((void *)reqp->dm_cur->dm_addr);
	DMA->dma_mode = 0x90 | dir;
	DMA->dma_mode = 0x90 | (dir ^ DMA_WRBIT);
	DMA->dma_mode = 0x90 | dir;
	DMA->dma_data = nsects;
	delay(2);	/* _really_ needed (Thomas Gerner) */
	DMA->dma_mode = 0x10 | dir;
}

static void
scsi_falcon_dmasetup(SC_REQ *reqp, u_int phase, u_char mode)
{
	int	nsects = reqp->dm_cur->dm_count / 512; /* XXX */

	/*
	 * XXX: We should probably clear the fifo before putting the
	 *      5380 into DMA-mode.
	 */
	if (PH_IN(phase)) {
		set_falcon_5380_reg(NCR5380_ICOM, 0);
		set_falcon_5380_reg(NCR5380_MODE, mode);
		set_falcon_5380_reg(NCR5380_IRCV, 0);
		fal1_dma(0, nsects, reqp);
	}
	else {
		set_falcon_5380_reg(NCR5380_MODE, mode);
		set_falcon_5380_reg(NCR5380_ICOM, SC_ADTB);
		set_falcon_5380_reg(NCR5380_DMSTAT, 0);
		fal1_dma(1, nsects, reqp);
	}
}

static int
falcon_poll_edma(SC_REQ *reqp)
{
	int	timeout = 9000; /* XXX */

	/*
	 * Because of the Falcon hardware, it is impossible to reach
	 * the 5380 while doing DMA-transfers. So we have to rely on
	 * the interrupt line to determine if DMA-has finished. the
	 * DMA-controller itself will never fire an interrupt. This means
	 * that 'broken-up' DMA transfers are not (yet) possible on the
	 * Falcon.
	 */
	for (;;) {
		delay(20);
		if (--timeout <= 0) {
			ncr_tprint(reqp, "Timeout on polled transfer\n");
			reqp->xs->error = XS_TIMEOUT;
			return(0);
		}
		if (!(MFP->mf_gpip & IO_DINT))
			break;
	}
	return(1);
}

static int
falcon_get_dma_result(SC_REQ *reqp, u_long *bytes_left)
{
	int	rv = 0;
	int	st_dmastat;
	u_long	bytes_done;

	/*
	 * Select sector counter register first (See Atari docu.)
	 */
	DMA->dma_mode = 0x90;
	if (!(st_dmastat = DMA->dma_stat) & 0x01) {
		/*
		 * Misc. DMA-error according to Atari...
		 */
		ncr_tprint(reqp, "Unknow ST-SCSI error near 0x%x\n",
							st_dmaaddr_get());
		reqp->xs->error = XS_DRIVER_STUFFUP;
		rv = 1;
	}
	/*
	 * Because we NEVER start DMA on the Falcon when the data size
	 * is not a multiple of 512 bytes, we can safely round down the
	 * byte count on writes. We need to because in case of a disconnect,
	 * the DMA has already prefetched the next couple of bytes.
	 * On read, these byte counts are an error. They are logged and
	 * should be handled by the mi-part of the driver.
	 * NOTE: We formerly did this by using the 'byte-count-zero' bit
	 *       of the DMA controller, but this didn't seem to work???
         *       [lwp 29/06/96]
	 */
	bytes_done = st_dmaaddr_get() - reqp->dm_cur->dm_addr;
	if (bytes_done & 511) {
		if (PH_IN(reqp->phase)) {
			ncr_tprint(reqp, "Byte count on read not a multiple "
					 "of 512 (%ld)\n", bytes_done);
		}
		bytes_done &= ~511;
	}
	if ((*bytes_left = reqp->dm_cur->dm_count - bytes_done) == 0)
		rv = 1;
	return(rv);
}

static int
falcon_can_access_5380()
{
	if (connected && (connected->dr_flag & DRIVER_IN_DMA)
		&& (MFP->mf_gpip & IO_DINT))
			return(0);
	return(1);
}

#endif /* defined(FALCON_SCSI) */

#if defined(TT_SCSI) && defined(FALCON_SCSI)
/*
 * Define some functions to support _both_ TT and Falcon SCSI
 */

/*
 * The prototypes first...
 */
static void	scsi_mach_init(struct ncr_softc *);
       void	scsi_ienable(void);
       void	scsi_idisable(void);
       void	scsi_clr_ipend(void);
       int	scsi_ipending(void);
       void	scsi_dma_setup(SC_REQ *, u_int, u_char);
       int	wrong_dma_range(SC_REQ *, struct dma_chain *);
       int	poll_edma(SC_REQ *);
       int	get_dma_result(SC_REQ *, u_long *);
       int	can_access_5380(void);

/*
 * Register access will be done through the following 2 function pointers.
 */
static u_char	(*get_5380_reg)(u_short);
static void	(*set_5380_reg)(u_short, u_short);

#define	GET_5380_REG	(*get_5380_reg)
#define	SET_5380_REG	(*set_5380_reg)

static void
scsi_mach_init(struct ncr_softc *sc)
{
	if (machineid & ATARI_FALCON) {
		get_5380_reg = get_falcon_5380_reg;
		set_5380_reg = set_falcon_5380_reg;
		scsi_falcon_init(sc);
	}
	else {
		get_5380_reg = get_tt_5380_reg;
		set_5380_reg = set_tt_5380_reg;
		scsi_tt_init(sc);
	}
}

extern inline void
scsi_ienable(void)
{
	if (machineid & ATARI_FALCON)
		scsi_falcon_ienable();
	else scsi_tt_ienable();
}

extern inline void
scsi_idisable(void)
{
	if (machineid & ATARI_FALCON)
		scsi_falcon_idisable();
	else scsi_tt_idisable();
}

extern inline void
scsi_clr_ipend(void)
{
	if (machineid & ATARI_FALCON)
		scsi_falcon_clr_ipend();
	else scsi_tt_clr_ipend();
}

extern inline int
scsi_ipending(void)
{
	if (machineid & ATARI_FALCON)
		return(scsi_falcon_ipending());
	else return (GET_TT_REG(NCR5380_DMSTAT) & SC_IRQ_SET);
}

extern inline void
scsi_dma_setup(SC_REQ *reqp, u_int phase, u_char mbase)
{
	if (machineid & ATARI_FALCON)
		scsi_falcon_dmasetup(reqp, phase, mbase);
	else scsi_tt_dmasetup(reqp, phase, mbase);
}

extern inline int
wrong_dma_range(SC_REQ *reqp, struct dma_chain *dm)
{
	if (machineid & ATARI_FALCON)
		return(falcon_wrong_dma_range(reqp, dm));
	else return(tt_wrong_dma_range(reqp, dm));
}

extern inline int
poll_edma(SC_REQ *reqp)
{
	if (machineid & ATARI_FALCON)
		return(falcon_poll_edma(reqp));
	else return(tt_poll_edma(reqp));
}

extern inline int
get_dma_result(SC_REQ *reqp, u_long *bytes_left)
{
	if (machineid & ATARI_FALCON)
		return(falcon_get_dma_result(reqp, bytes_left));
	else return(tt_get_dma_result(reqp, bytes_left));
}

extern inline int
can_access_5380()
{
	if (machineid & ATARI_FALCON)
		return(falcon_can_access_5380());
	return(1);
}

#define emulated_dma()		((machineid & ATARI_HADES) ? 1 : 0)

/*
 * Locking stuff. All turns into NOP's on the TT.
 */
#define	fair_to_keep_dma()	((machineid & ATARI_FALCON) ?		\
						!st_dmawanted() : 1)
#define	claimed_dma()		((machineid & ATARI_FALCON) ?		\
						falcon_claimed_dma() : 1)
#define reconsider_dma()	{					\
					if(machineid & ATARI_FALCON)	\
						falcon_reconsider_dma();\
				}
#endif /* defined(TT_SCSI) && defined(FALCON_SCSI) */

/**********************************************
 * Functions present for both TT and Falcon.  *
 **********************************************/
/*
 * Our autoconfig matching function
 */
static int
machine_match(struct device *pdp, void *match, void *auxp,
						struct cfdriver *cd)
{
	static int	we_matched = 0; /* Only one unit	*/

	if (strcmp(auxp, cd->cd_name) || we_matched)
		return(0);

	we_matched = 1;
	return(1);
}

/*
 * Bounce buffer (de)allocation. Those buffers are gotten from the ST-mem
 * pool. Allocation here is both contiguous and in the lower 16Mb of
 * the address space. Thus being DMA-able for all controllers.
 */
static u_char *
alloc_bounceb(u_long len)
{
	void	*tmp;

	return((u_char *)alloc_stmem(len, &tmp));
}

static void
free_bounceb(u_char *bounceb)
{
	free_stmem(bounceb);
}

/*
 * 5380 interrupt.
 */
void
scsi_ctrl(int sr)
{
	if (GET_5380_REG(NCR5380_DMSTAT) & SC_IRQ_SET) {
		scsi_idisable();
		if (!BASEPRI(sr))
			add_sicallback((si_farg)ncr_ctrl_intr,
						(void *)cur_softc, 0);
		else {
			spl1();
			ncr_ctrl_intr(cur_softc);
			spl0();
		}
	}
}

/*
 * DMA controller interrupt
 */
void
scsi_dma(int sr)
{
	SC_REQ	*reqp;

	if ((reqp = connected) && (reqp->dr_flag & DRIVER_IN_DMA)) {
		scsi_idisable();
		if (!BASEPRI(sr))
			add_sicallback((si_farg)ncr_dma_intr,
					(void *)cur_softc, 0);
		else {
			spl1();
			ncr_dma_intr(cur_softc);
			spl0();
		}
	}
}

/*
 * Last but not least... Include the general driver code
 */
#include <atari/dev/ncr5380.c>
