/*	$NetBSD: si_sebuf.c,v 1.10 2000/03/18 16:13:25 mycroft Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Sun3/E SCSI driver (machine-dependent portion).
 * The machine-independent parts are in ncr5380sbc.c
 *
 * XXX - Mostly from the si driver.  Merge?
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>

/* #define DEBUG XXX */

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include "sereg.h"
#include "sevar.h"

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth DMA overhead)
 */
#define	MIN_DMA_LEN 128

/*
 * Transfers lager than 65535 bytes need to be split-up.
 * (Some of the FIFO logic has only 16 bits counters.)
 * Make the size an integer multiple of the page size
 * to avoid buf/cluster remap problems.  (paranoid?)
 */
#define	MAX_DMA_LEN 0xE000

/*
 * This structure is used to keep track of mapped DMA requests.
 */
struct se_dma_handle {
	int 		dh_flags;
#define	SIDH_BUSY	1		/* This DH is in use */
#define	SIDH_OUT	2		/* DMA does data out (write) */
	u_char *	dh_addr;	/* KVA of start of buffer */
	int 		dh_maplen;	/* Length of KVA mapping. */
	long		dh_dma; 	/* Offset in DMA buffer. */
};

/*
 * The first structure member has to be the ncr5380_softc
 * so we can just cast to go back and fourth between them.
 */
struct se_softc {
	struct ncr5380_softc	ncr_sc;
	volatile struct se_regs	*sc_regs;
	int		sc_adapter_type;
	int		sc_adapter_iv;		/* int. vec */
	int 	sc_options;			/* options for this instance */
	int 	sc_reqlen;  		/* requested transfer length */
	struct se_dma_handle *sc_dma;
	/* DMA command block for the OBIO controller. */
	void *sc_dmacmd;
};

/* Options for disconnect/reselect, DMA, and interrupts. */
#define SE_NO_DISCONNECT    0xff
#define SE_NO_PARITY_CHK  0xff00
#define SE_FORCE_POLLING 0x10000
#define SE_DISABLE_DMA   0x20000

void se_dma_alloc __P((struct ncr5380_softc *));
void se_dma_free __P((struct ncr5380_softc *));
void se_dma_poll __P((struct ncr5380_softc *));

void se_dma_setup __P((struct ncr5380_softc *));
void se_dma_start __P((struct ncr5380_softc *));
void se_dma_eop __P((struct ncr5380_softc *));
void se_dma_stop __P((struct ncr5380_softc *));

void se_intr_on  __P((struct ncr5380_softc *));
void se_intr_off __P((struct ncr5380_softc *));

static int  se_intr __P((void *));
static void se_reset __P((struct ncr5380_softc *));

/*
 * New-style autoconfig attachment
 */

static int	se_match __P((struct device *, struct cfdata *, void *));
static void	se_attach __P((struct device *, struct device *, void *));

struct cfattach si_sebuf_ca = {
	sizeof(struct se_softc), se_match, se_attach
};

static void	se_minphys __P((struct buf *));

/* Options for disconnect/reselect, DMA, and interrupts. */
int se_options = SE_DISABLE_DMA | SE_FORCE_POLLING | 0xff;

/* How long to wait for DMA before declaring an error. */
int se_dma_intr_timo = 500;	/* ticks (sec. X 100) */

int se_debug = 0;
#ifdef	DEBUG
static int se_link_flags = 0 /* | SDEV_DB2 */ ;
#endif


static int
se_match(parent, cf, args)
	struct device	*parent;
	struct cfdata *cf;
	void *args;
{
	struct sebuf_attach_args *aa = args;

	/* Match by name. */
	if (strcmp(aa->name, "se"))
		return (0);

	/* Anyting else to check? */

	return (1);
}

static void
se_attach(parent, self, args)
	struct device	*parent, *self;
	void		*args;
{
	struct se_softc *sc = (struct se_softc *) self;
	struct ncr5380_softc *ncr_sc = &sc->ncr_sc;
	struct cfdata *cf = self->dv_cfdata;
	struct sebuf_attach_args *aa = args;
	volatile struct se_regs *regs;
	int i;

	/* Get options from config flags if specified. */
	if (cf->cf_flags)
		sc->sc_options = cf->cf_flags;
	else
		sc->sc_options = se_options;

	printf(": options=0x%x\n", sc->sc_options);

	sc->sc_adapter_type = aa->ca.ca_bustype;
	sc->sc_adapter_iv = aa->ca.ca_intvec;
	sc->sc_regs = regs = aa->regs;

	/*
	 * MD function pointers used by the MI code.
	 */
	ncr_sc->sc_pio_out = ncr5380_pio_out;
	ncr_sc->sc_pio_in =  ncr5380_pio_in;

#if 0	/* XXX - not yet... */
	ncr_sc->sc_dma_alloc = se_dma_alloc;
	ncr_sc->sc_dma_free  = se_dma_free;
	ncr_sc->sc_dma_setup = se_dma_setup;
	ncr_sc->sc_dma_start = se_dma_start;
	ncr_sc->sc_dma_poll  = se_dma_poll;
	ncr_sc->sc_dma_eop   = se_dma_eop;
	ncr_sc->sc_dma_stop  = se_dma_stop;
	ncr_sc->sc_intr_on   = se_intr_on;
	ncr_sc->sc_intr_off  = se_intr_off;
#endif	/* XXX */

	/* Attach interrupt handler. */
	isr_add_vectored(se_intr, (void *)sc,
		aa->ca.ca_intpri, aa->ca.ca_intvec);

	/* Reset the hardware. */
	se_reset(ncr_sc);

	/* Do the common attach stuff. */

	/*
	 * Support the "options" (config file flags).
	 * Disconnect/reselect is a per-target mask.
	 * Interrupts and DMA are per-controller.
	 */
	ncr_sc->sc_no_disconnect =
		(sc->sc_options & SE_NO_DISCONNECT);
	ncr_sc->sc_parity_disable = 
		(sc->sc_options & SE_NO_PARITY_CHK) >> 8;
	if (sc->sc_options & SE_FORCE_POLLING)
		ncr_sc->sc_flags |= NCR5380_FORCE_POLLING;

#if 1	/* XXX - Temporary */
	/* XXX - In case we think DMA is completely broken... */
	if (sc->sc_options & SE_DISABLE_DMA) {
		/* Override this function pointer. */
		ncr_sc->sc_dma_alloc = NULL;
	}
#endif
	ncr_sc->sc_min_dma_len = MIN_DMA_LEN;

#ifdef	DEBUG
	if (se_debug)
		printf("se: Set TheSoftC=%p TheRegs=%p\n", sc, regs);
	ncr_sc->sc_link.flags |= se_link_flags;
#endif

	/*
	 * Initialize fields used by the MI code
	 */
	ncr_sc->sci_r0 = &regs->ncrregs[0];
	ncr_sc->sci_r1 = &regs->ncrregs[1];
	ncr_sc->sci_r2 = &regs->ncrregs[2];
	ncr_sc->sci_r3 = &regs->ncrregs[3];
	ncr_sc->sci_r4 = &regs->ncrregs[4];
	ncr_sc->sci_r5 = &regs->ncrregs[5];
	ncr_sc->sci_r6 = &regs->ncrregs[6];
	ncr_sc->sci_r7 = &regs->ncrregs[7];

	/*
	 * Allocate DMA handles.
	 */
	i = SCI_OPENINGS * sizeof(struct se_dma_handle);
	sc->sc_dma = (struct se_dma_handle *)
		malloc(i, M_DEVBUF, M_WAITOK);
	if (sc->sc_dma == NULL)
		panic("se: dma_malloc failed\n");
	for (i = 0; i < SCI_OPENINGS; i++)
		sc->sc_dma[i].dh_flags = 0;

	ncr_sc->sc_adapter.scsipi_scsi.adapter_target = 7;
	ncr_sc->sc_adapter.scsipi_minphys = se_minphys;

	/*
	 *  Initialize se board itself.
	 */
	ncr5380_attach(ncr_sc);
}

static void
se_reset(struct ncr5380_softc *ncr_sc)
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	volatile struct se_regs *se = sc->sc_regs;

#ifdef	DEBUG
	if (se_debug) {
		printf("se_reset\n");
	}
#endif

	/* The reset bits in the CSR are active low. */
	se->se_csr = 0;
	delay(10);
	se->se_csr = SE_CSR_SCSI_RES /* | SE_CSR_INTR_EN */ ;
	delay(10);

	/* Make sure the DMA engine is stopped. */
	se->dma_addr = 0;
	se->dma_cntr = 0;
	se->se_ivec = sc->sc_adapter_iv;
}

/*
 * This is called when the bus is going idle,
 * so we want to enable the SBC interrupts.
 * That is controlled by the DMA enable!
 * Who would have guessed!
 * What a NASTY trick!
 */
void
se_intr_on(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	volatile struct se_regs *se = sc->sc_regs;

	/* receive mode should be safer */
	se->se_csr &= ~SE_CSR_SEND;

	/* Clear the count so nothing happens. */
	se->dma_cntr = 0;
	
	/* Clear the start address too. (paranoid?) */
	se->dma_addr = 0;

	/* Finally, enable the DMA engine. */
	se->se_csr |= SE_CSR_INTR_EN;
}

/*
 * This is called when the bus is idle and we are
 * about to start playing with the SBC chip.
 */
void
se_intr_off(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	volatile struct se_regs *se = sc->sc_regs;

	se->se_csr &= ~SE_CSR_INTR_EN;
}

/*
 * This function is called during the COMMAND or MSG_IN phase
 * that preceeds a DATA_IN or DATA_OUT phase, in case we need
 * to setup the DMA engine before the bus enters a DATA phase.
 *
 * On the VME version, setup the start addres, but clear the
 * count (to make sure it stays idle) and set that later.
 * XXX: The VME adapter appears to suppress SBC interrupts
 * when the FIFO is not empty or the FIFO count is non-zero!
 * XXX: Need to copy data into the DMA buffer...
 */
void
se_dma_setup(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct se_dma_handle *dh = sr->sr_dma_hand;
	volatile struct se_regs *se = sc->sc_regs;
	long data_pa;
	int xlen;

	/*
	 * Get the DMA mapping for this segment.
	 * XXX - Should separate allocation and mapin.
	 */
	data_pa = 0; /* XXX se_dma_kvtopa(dh->dh_dma); */
	data_pa += (ncr_sc->sc_dataptr - dh->dh_addr);
	if (data_pa & 1)
		panic("se_dma_start: bad pa=0x%lx", data_pa);
	xlen = ncr_sc->sc_datalen;
	xlen &= ~1;				/* XXX: necessary? */
	sc->sc_reqlen = xlen; 	/* XXX: or less? */

#ifdef	DEBUG
	if (se_debug & 2) {
		printf("se_dma_setup: dh=%p, pa=0x%lx, xlen=0x%x\n",
			   dh, data_pa, xlen);
	}
#endif

	/* Set direction (send/recv) */
	if (dh->dh_flags & SIDH_OUT) {
		se->se_csr |= SE_CSR_SEND;
	} else {
		se->se_csr &= ~SE_CSR_SEND;
	}

	/* Load the start address. */
	se->dma_addr = (ushort)(data_pa & 0xFFFF);

	/*
	 * Keep the count zero or it may start early!
	 */
	se->dma_cntr = 0;
}


void
se_dma_start(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct se_dma_handle *dh = sr->sr_dma_hand;
	volatile struct se_regs *se = sc->sc_regs;
	int s, xlen;

	xlen = sc->sc_reqlen;

	/* This MAY be time critical (not sure). */
	s = splhigh();

	se->dma_cntr = (ushort)(xlen & 0xFFFF);

	/*
	 * Acknowledge the phase change.  (After DMA setup!)
	 * Put the SBIC into DMA mode, and start the transfer.
	 */
	if (dh->dh_flags & SIDH_OUT) {
		*ncr_sc->sci_tcmd = PHASE_DATA_OUT;
		SCI_CLR_INTR(ncr_sc);
		*ncr_sc->sci_icmd = SCI_ICMD_DATA;
		*ncr_sc->sci_mode |= (SCI_MODE_DMA | SCI_MODE_DMA_IE);
		*ncr_sc->sci_dma_send = 0;	/* start it */
	} else {
		*ncr_sc->sci_tcmd = PHASE_DATA_IN;
		SCI_CLR_INTR(ncr_sc);
		*ncr_sc->sci_icmd = 0;
		*ncr_sc->sci_mode |= (SCI_MODE_DMA | SCI_MODE_DMA_IE);
		*ncr_sc->sci_irecv = 0;	/* start it */
	}

	/* Let'er rip! */
	se->se_csr |= SE_CSR_INTR_EN;

	splx(s);
	ncr_sc->sc_state |= NCR_DOINGDMA;

#ifdef	DEBUG
	if (se_debug & 2) {
		printf("se_dma_start: started, flags=0x%x\n",
			   ncr_sc->sc_state);
	}
#endif
}


void
se_dma_eop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{

	/* Not needed - DMA was stopped prior to examining sci_csr */
}


void
se_dma_stop(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct se_dma_handle *dh = sr->sr_dma_hand;
	volatile struct se_regs *se = sc->sc_regs;
	int resid, ntrans;

	if ((ncr_sc->sc_state & NCR_DOINGDMA) == 0) {
#ifdef	DEBUG
		printf("se_dma_stop: dma not running\n");
#endif
		return;
	}
	ncr_sc->sc_state &= ~NCR_DOINGDMA;

	/* First, halt the DMA engine. */
	se->se_csr &= ~SE_CSR_INTR_EN;	/* VME only */

	/* Set an impossible phase to prevent data movement? */
	*ncr_sc->sci_tcmd = PHASE_INVALID;

	/* Note that timeout may have set the error flag. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		goto out;

	/* XXX: Wait for DMA to actually finish? */

	/*
	 * Now try to figure out how much actually transferred
	 */
	resid = se->dma_cntr & 0xFFFF;
	if (dh->dh_flags & SIDH_OUT)
		if ((resid > 0) && (resid < sc->sc_reqlen))
			resid++;
	ntrans = sc->sc_reqlen - resid;

#ifdef	DEBUG
	if (se_debug & 2) {
		printf("se_dma_stop: resid=0x%x ntrans=0x%x\n",
		       resid, ntrans);
	}
#endif

	if (ntrans < MIN_DMA_LEN) {
		printf("se: fifo count: 0x%x\n", resid);
		ncr_sc->sc_state |= NCR_ABORTING;
		goto out;
	}
	if (ntrans > ncr_sc->sc_datalen)
		panic("se_dma_stop: excess transfer");

	/* Adjust data pointer */
	ncr_sc->sc_dataptr += ntrans;
	ncr_sc->sc_datalen -= ntrans;

out:
	se->dma_addr = 0;
	se->dma_cntr = 0;

	/* Put SBIC back in PIO mode. */
	*ncr_sc->sci_mode &= ~(SCI_MODE_DMA | SCI_MODE_DMA_IE);
	*ncr_sc->sci_icmd = 0;
}

/*****************************************************************/

static void
se_minphys(struct buf *bp)
{

	if (bp->b_bcount > MAX_DMA_LEN)
		bp->b_bcount = MAX_DMA_LEN;

	return (minphys(bp));
}


int
se_intr(void *arg)
{
	struct se_softc *sc = arg;
	volatile struct se_regs *se = sc->sc_regs;
	int dma_error, claimed;
	u_short csr;

	claimed = 0;
	dma_error = 0;

	/* SBC interrupt? DMA interrupt? */
	csr = se->se_csr;
	NCR_TRACE("se_intr: csr=0x%x\n", csr);

	if (csr & SE_CSR_SBC_IP) {
		claimed = ncr5380_intr(&sc->ncr_sc);
#ifdef	DEBUG
		if (!claimed) {
			printf("se_intr: spurious from SBC\n");
		}
#endif
		/* Yes, we DID cause this interrupt. */
		claimed = 1;
	}

	return (claimed);
}


/*****************************************************************
 * Common functions for DMA
 ****************************************************************/

/*
 * Allocate a DMA handle and put it in sc->sc_dma.  Prepare
 * for DMA transfer.  On the Sun3/E, this means we have to
 * allocate space in the DMA buffer for this transfer.
 */
void
se_dma_alloc(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	struct scsipi_xfer *xs = sr->sr_xs;
	struct se_dma_handle *dh;
	int i, xlen;
	u_long addr;

#ifdef	DIAGNOSTIC
	if (sr->sr_dma_hand != NULL)
		panic("se_dma_alloc: already have DMA handle");
#endif

	addr = (u_long) ncr_sc->sc_dataptr;
	xlen = ncr_sc->sc_datalen;

	/* If the DMA start addr is misaligned then do PIO */
	if ((addr & 1) || (xlen & 1)) {
		printf("se_dma_alloc: misaligned.\n");
		return;
	}

	/* Make sure our caller checked sc_min_dma_len. */
	if (xlen < MIN_DMA_LEN)
		panic("se_dma_alloc: xlen=0x%x\n", xlen);

	/*
	 * Never attempt single transfers of more than 63k, because
	 * our count register may be only 16 bits (an OBIO adapter).
	 * This should never happen since already bounded by minphys().
	 * XXX - Should just segment these...
	 */
	if (xlen > MAX_DMA_LEN) {
		printf("se_dma_alloc: excessive xlen=0x%x\n", xlen);
		ncr_sc->sc_datalen = xlen = MAX_DMA_LEN;
	}

	/* Find free DMA handle.  Guaranteed to find one since we have
	   as many DMA handles as the driver has processes. */
	for (i = 0; i < SCI_OPENINGS; i++) {
		if ((sc->sc_dma[i].dh_flags & SIDH_BUSY) == 0)
			goto found;
	}
	panic("se: no free DMA handles.");
found:

	dh = &sc->sc_dma[i];
	dh->dh_flags = SIDH_BUSY;

	/* Copy the "write" flag for convenience. */
	if (xs->xs_control & XS_CTL_DATA_OUT)
		dh->dh_flags |= SIDH_OUT;

	dh->dh_addr = (u_char*) addr;
	dh->dh_maplen  = xlen;
	dh->dh_dma = 0;	/* XXX - Allocate space in DMA buffer. */
	/* XXX: dh->dh_dma = alloc(xlen) */
	if (!dh->dh_dma) {
		/* Can't remap segment */
		printf("se_dma_alloc: can't remap %p/0x%x\n",
			dh->dh_addr, dh->dh_maplen);
		dh->dh_flags = 0;
		return;
	}

	/* success */
	sr->sr_dma_hand = dh;

	return;
}


void
se_dma_free(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct sci_req *sr = ncr_sc->sc_current;
	struct se_dma_handle *dh = sr->sr_dma_hand;

#ifdef	DIAGNOSTIC
	if (dh == NULL)
		panic("se_dma_free: no DMA handle");
#endif

	if (ncr_sc->sc_state & NCR_DOINGDMA)
		panic("se_dma_free: free while in progress");

	if (dh->dh_flags & SIDH_BUSY) {
		/* XXX: Should separate allocation and mapping. */
		/* XXX: Give back the DMA space. */
		/* XXX: free((caddr_t)dh->dh_dma, dh->dh_maplen); */
		dh->dh_dma = 0;
		dh->dh_flags = 0;
	}
	sr->sr_dma_hand = NULL;
}


#define	CSR_MASK SE_CSR_SBC_IP
#define	POLL_TIMO	50000	/* X100 = 5 sec. */

/*
 * Poll (spin-wait) for DMA completion.
 * Called right after xx_dma_start(), and
 * xx_dma_stop() will be called next.
 * Same for either VME or OBIO.
 */
void
se_dma_poll(ncr_sc)
	struct ncr5380_softc *ncr_sc;
{
	struct se_softc *sc = (struct se_softc *)ncr_sc;
	struct sci_req *sr = ncr_sc->sc_current;
	volatile struct se_regs *se = sc->sc_regs;
	int tmo;

	/* Make sure DMA started successfully. */
	if (ncr_sc->sc_state & NCR_ABORTING)
		return;

	/*
	 * XXX: The Sun driver waits for ~SE_CSR_DMA_ACTIVE here
	 * XXX: (on obio) or even worse (on vme) a 10mS. delay!
	 * XXX: I really doubt that is necessary...
	 */

	/* Wait for any "dma complete" or error bits. */
	tmo = POLL_TIMO;
	for (;;) {
		if (se->se_csr & CSR_MASK)
			break;
		if (--tmo <= 0) {
			printf("se: DMA timeout (while polling)\n");
			/* Indicate timeout as MI code would. */
			sr->sr_flags |= SR_OVERDUE;
			break;
		}
		delay(100);
	}
	NCR_TRACE("se_dma_poll: waited %d\n",
			  POLL_TIMO - tmo);

#ifdef	DEBUG
	if (se_debug & 2) {
		printf("se_dma_poll: done, csr=0x%x\n", se->se_csr);
	}
#endif
}

