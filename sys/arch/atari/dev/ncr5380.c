/*	$NetBSD: ncr5380.c,v 1.1.1.1 1995/03/26 07:12:10 leo Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <scsi/scsi_all.h>
#include <scsi/scsi_message.h>
#include <scsi/scsiconf.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <atari/dev/ncr5380reg.h>

/*
 * This is crap, but because the interrupts now run at MFP spl-level (6),
 * splbio() is not enough at some places. The code should be checked to
 * find out where splhigh() is needed and where splbio() should be used.
 * Now that I use this interrupt sceme, the spl values are fake!
 */
#undef splbio()
#define splbio()	splhigh()

/*
 * SCSI completion status codes, should move to sys/scsi/????
 */
#define SCSMASK		0x1e	/* status code mask			*/
#define SCSGOOD		0x00	/* good status				*/
#define SCSCHKC		0x02	/* check condition			*/
#define SCSBUSY		0x08	/* busy status				*/
#define SCSCMET		0x04	/* condition met / good			*/

/********************************************************************/

#define	NREQ		18	/* Size of issue queue			*/
#define	AUTO_SENSE	1	/* Automatically issue a request-sense 	*/

#undef	DBG_SEL			/* Show the selection process		*/
#undef	DBG_REQ			/* Show enqueued/ready requests		*/
#undef	DBG_NOWRITE		/* Do not allow writes to the targets	*/
#undef	DBG_PIO			/* Show the polled-I/O process		*/
#undef	DBG_INF			/* Show information transfer process	*/
#undef	DBG_NOSTATIC		/* No static functions, all in DDB trace*/
#define	DBG_PID			/* Keep track of driver			*/
#define	REAL_DMA		/* Use DMA if sensible			*/
#define	REAL_DMA_POLL	0	/* 1: Poll for end of DMA-transfer	*/
#undef	NO_TTRAM_DMA		/* Do not use DMA to TT-ram. This	*/
				/*    fails on older atari's		*/

#ifdef DBG_NOSTATIC
#	define	static
#endif
#ifdef DBG_SEL
#	define	DBG_SELPRINT(a,b)	printf(a,b)
#else
#	define DBG_SELPRINT(a,b)
#endif
#ifdef DBG_PIO
#	define DBG_PIOPRINT(a,b,c) 	printf(a,b,c)
#else
#	define DBG_PIOPRINT(a,b,c)
#endif
#ifdef DBG_INF
#	define DBG_INFPRINT(a,b,c)	a(b,c)
#else
#	define DBG_INFPRINT(a,b,c)
#endif
#ifdef DBG_PID
	static	char	*last_hit = NULL;
#	define	PID(a)	last_hit = a
#else
#	define	PID(a)
#endif

/*
 * Return values of check_intr()
 */
#define	INTR_SPURIOUS	0
#define	INTR_RESEL	2
#define	INTR_DMA	3

/*
 * Set bit for target when parity checking must be disabled.
 * My (LWP) Maxtor 7245S  seems to generate parity errors on about 50%
 * of all transfers while the data is correct!?
 */
u_char	ncr5380_no_parchk = 0;

/*
 * This is the default sense-command we send.
 */
static	u_char	sense_cmd[] = {
		REQUEST_SENSE, 0, 0, 0, sizeof(struct scsi_sense), 0
};

/*
 * True if the main co-routine is running
 */
static volatile int	main_running = 0;

/*
 * True if callback to softint scheduled.
 */
static volatile int	callback_scheduled = 0;

/*
 * Mask of targets selected
 */
u_char	busy;

struct	ncr_softc {
	struct	device		sc_dev;
	struct	scsi_link	sc_link;
};

static void	ncr5380_minphys(struct buf *bp);
static int	ncr5380_scsi_cmd(struct scsi_xfer *xs);
static int	ncr5380_show_scsi_cmd(struct scsi_xfer *xs);

struct scsi_adapter ncr5380_switch = {
	ncr5380_scsi_cmd,		/* scsi_cmd()			*/
	ncr5380_minphys,		/* scsi_minphys()		*/
	0,				/* open_target_lu()		*/
	0				/* close_target_lu()		*/
};

struct scsi_device ncr5380_dev = {
	NULL,		/* use default error handler		*/
	NULL,		/* do not have a start functio		*/
	NULL,		/* have no async handler		*/
	NULL		/* Use default done routine		*/
};

/*
 * Max. number of dma-chains per request
 */
#define	MAXDMAIO	(MAXPHYS/NBPG + 1)

/*
 * Some requests are not contiguous in physical memory. We need to break them
 * up into contiguous parts for DMA.
 */
struct dma_chain {
	u_int	dm_count;
	u_long	dm_addr;
};

/*
 * Define our issue, free and disconnect queue's.
 */
typedef struct	req_q {
    struct req_q	*next;	    /* next in free, issue or discon queue  */
    struct req_q	*link;	    /* next linked command to execute       */
    struct scsi_xfer	*xs;	    /* request from high-level driver       */
    u_char		dr_flag;    /* driver state			    */
    u_char		phase;	    /* current SCSI phase		    */
    u_char		msgout;	    /* message to send when requested       */
    u_char		targ_id;    /* target for command		    */
    u_char		status;	    /* returned status byte		    */
    u_char		message;    /* returned message byte		    */
    struct dma_chain	dm_chain[MAXDMAIO];
    struct dma_chain	*dm_cur;    /* current dma-request		    */
    struct dma_chain	*dm_last;   /* last dma-request			    */
    long		xdata_len;  /* length of transfer		    */
    u_char		*xdata_ptr; /* physical address of transfer	    */
    struct scsi_generic	xcmd;	    /* command to execute		    */
} SC_REQ;

/*
 * Values for dr_flag:
 */
#define	DRIVER_IN_DMA	1	/* Non-polled DMA activated		*/
#define	DRIVER_AUTOSEN	2	/* Doing automatic sense		*/
#define	DRIVER_NOINT	4	/* We are booting: no interrupts	*/
#define	DRIVER_DMAOK	8	/* DMA can be used on this request	*/


static SC_REQ	req_queue[NREQ];
static SC_REQ	*free_head = NULL;	/* Free request structures	*/
static SC_REQ	*issue_q   = NULL;	/* Commands waiting to be issued*/
static SC_REQ	*discon_q  = NULL;	/* Commands disconnected	*/
static SC_REQ	*connected = NULL;	/* Command currently connected	*/

/*
 * Function decls:
 */
static int  transfer_pio __P((u_char *, u_char *, u_long *));
static int  wait_req_true __P((void));
static int  wait_req_false __P((void));
static int  scsi_select __P((SC_REQ *, int));
static int  handle_message __P((SC_REQ *, u_int));
static int  information_transfer __P((void));
static void  reselect __P((void));
static int  dma_ready __P((int, int));
static void transfer_dma __P((SC_REQ *, u_int, int));
static int  check_autosense __P((SC_REQ *, int));
static int  reach_msg_out __P((u_long));
static void timer __P((void));
static int  check_intr __P((void));
static void scsi_reset __P((void));
static void scsi_main __P((void));
static int  scsi_dmaok __P((SC_REQ *));
static void run_main __P((void));

static void show_request __P((SC_REQ *, char *));
static void show_phase __P((SC_REQ *, int));
static void show_signals __P((void));

/*
 * Inline functions:
 */
extern __inline__ void scsi_ienable()
{
	MFP2->mf_ierb |= IB_SCDM;
	MFP2->mf_iera |= IA_SCSI;
	MFP2->mf_imra |= IA_SCSI;
}

extern __inline__ scsi_idisable()
{
	int	sps = splbio();
	MFP2->mf_ierb &= ~IB_SCDM;
	MFP2->mf_iera &= ~IA_SCSI;
	splx(sps);
}

/*
 * Determine the size of a SCSI command.
 */
extern __inline__ int command_size(opcode)
u_char	opcode;
{
	switch((opcode >> 4) & 0xf) {
		case 0:
		case 1:
			return(6);
		case 2:
		case 3:
			return(10);
	}
	return(12);
}


/*
 * Wait for request-line to become active. When it doesn't return 0.
 * Otherwise return != 0.
 * The timeouts in the 'wait_req_*' functions are arbitrary and rather
 * large. In 99% of the invocations nearly no timeout is needed but in
 * some cases (especially when using my tapedrive, a Tandberg 3600) the
 * device is busy internally and the first SCSI-phase will be delayed.
 */
extern __inline__ int wait_req_true(void)
{
	int	timeout = 25000;

	while(!(SCSI_5380->scsi_idstat & SC_S_REQ) && --timeout)
		delay(1);
	return(SCSI_5380->scsi_idstat & SC_S_REQ);
}

/*
 * Wait for request-line to become inactive. When it doesn't return 0.
 * Otherwise return != 0.
 */
extern __inline__ int wait_req_false(void)
{
	int	timeout = 25000;

	while((SCSI_5380->scsi_idstat & SC_S_REQ) && --timeout)
		delay(1);
	return(!(SCSI_5380->scsi_idstat & SC_S_REQ));
}

extern __inline__ void finish_req(SC_REQ *reqp)
{
	int			sps;
	struct scsi_xfer	*xs = reqp->xs;

	/*
	 * Return request to free-q
	 */
	sps = splbio();
	reqp->next = free_head;
	free_head  = reqp;
	splx(sps);

	xs->flags |= ITSDONE;
	scsi_done(xs);
}

/*
 * Auto config stuff....
 */
int	ncr_print __P((void *auxp, char *));
void	ncr_attach __P((struct device *, struct device *, void *));
int	ncr_match __P((struct device *, struct cfdata *, void *));

struct cfdriver ncrscsicd = {
	NULL, "ncrscsi", (cfmatch_t)ncr_match, ncr_attach, 
	DV_DULL, sizeof(struct ncr_softc), NULL, 0 };

int
ncr_match(pdp, cdp, auxp)
struct device	*pdp;
struct cfdata	*cdp;
void		*auxp;
{
	if(strcmp(auxp, ncrscsicd.cd_name))
		return(0);
	if(cdp->cf_unit != 0)	/* Only one unit	*/
		return(0);
	return(1);
}

void
ncr_attach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct ncr_softc	*sc;
	int					i;

	sc = (struct ncr_softc *)dp;

	sc->sc_link.adapter_softc   = sc;
	sc->sc_link.adapter_target  = 7;
	sc->sc_link.adapter         = &ncr5380_switch;
	sc->sc_link.device          = &ncr5380_dev;
	sc->sc_link.openings        = NREQ - 1;

	/*
	 * Enable SCSI-related interrupts
	 */
	MFP2->mf_aer |= 0x80; /* SCSI IRQ goes HIGH!!!!! */

	MFP2->mf_ierb |= IB_SCDM;	/* SCSI-dma interrupts	*/
	MFP2->mf_iprb &= ~IB_SCDM;
	MFP2->mf_imrb |= IB_SCDM;

	MFP2->mf_iera |= IA_SCSI;	/* SCSI-5380 interrupts	*/
	MFP2->mf_ipra &= ~IA_SCSI;
	MFP2->mf_imra |= IA_SCSI;

	/*
	 * Initialize request queue freelist.
	 */
	for(i = 0; i < NREQ; i++) {
		req_queue[i].next = free_head;
		free_head = &req_queue[i];
	}

	/*
	 * Initialize the host adapter
	 */
	scsi_idisable();
	SCSI_5380->scsi_icom   = 0;
	SCSI_5380->scsi_mode   = IMODE_BASE;
	SCSI_5380->scsi_tcom   = 0;
	SCSI_5380->scsi_idstat = 0;

	printf("\n");


	/*
	 * attach all scsi units on us
	 */
	config_found(dp, &sc->sc_link, ncr_print);
}

/*
 * print diag if name is NULL else just extra
 */
int
ncr_print(auxp, name)
void	*auxp;
char	*name;
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}
/*
 * End of auto config stuff....
 */

/*
 * Carry out a request from the high level driver.
 */
static int
ncr5380_scsi_cmd(struct scsi_xfer *xs)
{
	int	sps;
	SC_REQ	*reqp;
	int	flags = xs->flags;

	/*
	 * Sanity check on flags...
	 */
	if(flags & ITSDONE) {
		printf("ncr5380_scsi_cmd: command already done.....\n");
		xs->flags &= ~ITSDONE;
	}
	if(!(flags & INUSE)) {
		printf("ncr5380_scsi_cmd: command not in use.....\n");
		xs->flags |= ~INUSE;
	}

	/*
	 * LWP: No lun support yet XXX
	 */
	if(xs->sc_link->lun != 0) {
		xs->error = XS_DRIVER_STUFFUP; /* XXX */
		return(COMPLETE);
	}

	/*
	 * We do not queue RESET commands
	 */
	if(flags & SCSI_RESET) {
		scsi_reset();
		return(COMPLETE);
	}

	/*
	 * Get a request block
	 */
	sps = splbio();
	if((reqp = free_head) == 0) {
		splx(sps);
		return(TRY_AGAIN_LATER);
	}
	free_head  = reqp->next;
	reqp->next = NULL;
	splx(sps);

	/*
	 * Initialize our private fields
	 */
	reqp->dr_flag   = (xs->flags & SCSI_POLL) ? DRIVER_NOINT : 0;
	reqp->phase     = NR_PHASE;
	reqp->msgout    = MSG_NOOP;
	reqp->status    = SCSGOOD;
	reqp->link      = NULL;
	reqp->xs        = xs;
	reqp->targ_id   = xs->sc_link->target;
	reqp->xdata_ptr = (u_char*)xs->data;
	reqp->xdata_len = xs->datalen;
	memcpy(&reqp->xcmd, xs->cmd, sizeof(struct scsi_generic));

	/*
	 * Check if DMA can be used on this request
	 */
	if(!(xs->flags & SCSI_POLL) && scsi_dmaok(reqp))
		reqp->dr_flag |= DRIVER_DMAOK;

	/*
	 * Insert the command into the issue queue. Note that 'REQUEST SENSE'
	 * commands are inserted at the head of the queue since any command
	 * will clear the existing contingent allegience condition and the sense
	 * data is only valid while the condition exists.
	 * When possible, link the command to a previous command to the same
	 * target. This is not very sensible when AUTO_SENSE is not defined!
	 * Interrupts are disabled while we are fiddling with the issue-queue.
	 */
	sps = splbio();
	if((issue_q == NULL) || (reqp->xcmd.opcode == REQUEST_SENSE)) {
		reqp->next = issue_q;
		issue_q    = reqp;
	}
	else {
		SC_REQ	*tmp, *link;

		tmp  = issue_q;
		link = NULL;
		do {
		    if(!link && (tmp->targ_id == reqp->targ_id) && !tmp->link)
				link = tmp;
		} while(tmp->next && (tmp = tmp->next));
		tmp->next = reqp;
#ifdef AUTO_SENSE
		if(link) {
			link->link = reqp;
			link->xcmd.bytes[link->xs->cmdlen-1] |= 1;
		}
#endif
	}
	splx(sps);

#ifdef DBG_REQ
	show_request(reqp,(reqp->xcmd.opcode == REQUEST_SENSE) ? "HEAD":"TAIL");
#endif

	run_main();

	if(xs->flags & SCSI_POLL)
		return(COMPLETE);	/* We're booting */
	return(SUCCESSFULLY_QUEUED);
}

#define MIN_PHYS	65536	/*BARF!!!!*/
static void
ncr5380_minphys(struct buf *bp)
{
    if(bp->b_bcount > MIN_PHYS) {
	printf("Uh-oh...  ncr5380_minphys setting bp->b_bcount=%x.\n",MIN_PHYS);
	bp->b_bcount = MIN_PHYS;
    }
}
#undef MIN_PHYS

static int
ncr5380_show_scsi_cmd(struct scsi_xfer *xs)
{
	u_char	*b = (u_char *) xs->cmd;
	int	i  = 0;

	if(!(xs->flags & SCSI_RESET)) {
		printf("ncr5380(%d:%d:%d,0x%x)-", xs->sc_link->scsibus,
		    xs->sc_link->target, xs->sc_link->lun, xs->sc_link->flags);
		while(i < xs->cmdlen) {
			if(i)
				printf(",");
			printf("%x",b[i++]);
		}
		printf("-\n");
	}
	else {
		printf("ncr5380(%d:%d:%d)-RESET-\n",
		    xs->sc_link->scsibus,xs->sc_link->target, xs->sc_link->lun);
	}
}

/*
 * The body of the driver.
 */
static void
scsi_main()
{
	SC_REQ	*req, *prev;
	int	itype;
	int	sps;

	/*
	 * While running in the driver SCSI-interrupts are disabled.
	 */
	scsi_idisable();

	PID(")");
	for(;;) {
	    sps = splbio();
	    if(!connected) {

		/*
		 * Search through the issue-queue for a command
		 * destined for a target that isn't busy.
		 */
		prev = NULL;
		for(req=issue_q; req != NULL; prev = req, req = req->next) {
			if(!(busy & (1 << req->targ_id))) {
				/*
				 * Found one, remove it from the issue queue
				 */
				if(prev == NULL) 
					issue_q = req->next;
				else prev->next = req->next;
				req->next = NULL;
				break;
			}
		}

		/*
		 * When a request has just ended, we get here before an other
		 * device detects that the bus is free and that it can
		 * reconnect. The problem is that when this happens, we always
		 * baffle the device because our (initiator) id is higher. This
		 * can cause a sort of starvation on slow devices. So we check
		 * for a pending reselection here.
		 * Note that 'connected' will be non-null if the reselection
		 * succeeds.
		 */
		if((SCSI_5380->scsi_idstat&(SC_S_SEL|SC_S_IO))
						== (SC_S_SEL|SC_S_IO)){
			if(req != NULL) {
				req->next = issue_q; 
				issue_q = req;
			}
			splx(sps);

			reselect();
			SC_CLINT;
			goto connected;
		}

		/*
		 * The host is not connected and there is no request
		 * pending, exit.
		 */
		if(req == NULL) {
			PID("{");
			goto main_exit;
		}

		/*
		 * Re-enable interrupts before handling the request.
		 */
		splx(sps);

#ifdef DBG_REQ
		show_request(req, "TARGET");
#endif
		/*
		 * We found a request. Try to connect to the target. If the
		 * initiator fails arbitration, the command is put back in the
		 * issue queue.
		 */
		if(scsi_select(req, 0)) {
			sps = splbio();
			req->next = issue_q; 
			issue_q = req;
			splx(sps);
#ifdef DBG_REQ
			printf("Select failed on target %d\n", req->targ_id);
#endif
		}
	    }
	    else splx(sps);
connected:
	    if(connected) {
		/*
		 * If the host is currently connected but a 'real-dma' transfer
		 * is in progress, the 'end-of-dma' interrupt restarts main.
		 * So quit.
		 */
		sps = splbio();
		if(connected && (connected->dr_flag & DRIVER_IN_DMA)) {
			PID("[");
			goto main_exit;
		}
		splx(sps);

		/*
		 * Let the target guide us through the bus-phases
		 */
		while(information_transfer() == -1)
			;
	    }
	}
	/* NEVER TO REACH HERE */
	panic("TTSCSI: not designed to come here");

main_exit:
	/*
	 * We enter here with interrupts disabled. We are about to exit main
	 * so interrupts should be re-enabled. Because interrupts are edge
	 * triggered, we could already have missed the interrupt. Therefore
	 * we check the IRQ-line here and re-enter when we really missed a
	 * valid interrupt.
	 */
	PID("S");
	scsi_ienable();
	SCSI_5380->scsi_idstat = SC_HOST_ID;
	if(SCSI_5380->scsi_dmstat & SC_IRQ_SET) {
		if((itype = check_intr()) != INTR_SPURIOUS) {
			scsi_idisable();
			splx(sps);

			if(itype == INTR_RESEL)
				reselect();
			else dma_ready(0, 0);
			SC_CLINT;
			goto connected;
		}
	}
	main_running = 0;
	splx(sps);
	PID("R");
}

/*
 * The SCSI-DMA interrupt.
 * This interrupt can only be triggered when running in non-polled DMA
 * mode. When DMA is not active, it will be silently ignored, it is usually
 * to late because the EOP interrupt of the controller happens just a tiny
 * bit earlier. It might become usefull when scatter/gather is implemented,
 * because in that case only part of the DATAIN/DATAOUT transfer is taken
 * out of a single buffer.
 */
scsi_dma(sr)
int	sr;	/* sr at time of interrupt */
{
	SC_REQ	*reqp;
	int	dma_done;

	PID("9");
	if((reqp = connected) && (reqp->dr_flag & DRIVER_IN_DMA)) {
		scsi_idisable();
		if(!(dma_done = dma_ready(0, 0))) {
			scsi_ienable();
			transfer_dma(reqp, reqp->phase, 0);
			return;
		}
		PID("!");
		if(!BASEPRI(sr)) {
			if(!callback_scheduled++)
				add_sicallback(run_main, 0, 0);
		}
		else {
			spl1();
			run_main();
		}
	}
	/* PID("#"); */
}

/*
 * The SCSI-controller interrupt. This interrupt occurs on reselections and
 * at the end of non-polled DMA-interrupts.
 */
scsi_ctrl(sr)
int	sr;	/* sr at time of interrupt */
{
	int	itype;
	int	dma_done;

	/* PID("$"); */
	while(SCSI_5380->scsi_dmstat & SC_IRQ_SET) {
		scsi_idisable();
		/* PID("&"); */
		if((itype = check_intr()) != INTR_SPURIOUS) {
			/* PID("o"); */
			if(itype == INTR_RESEL)
				reselect();
			else {
			    if(!(dma_done = dma_ready(0, 0))) {
				scsi_ienable();
				transfer_dma(connected, connected->phase, 0);
				return;
			    }
			}
			SC_CLINT;
		}
		/* PID("*"); */
		if(!BASEPRI(sr)) {
			if(!callback_scheduled++)
				add_sicallback(run_main, 0, 0);
		}
		else {
			spl1();
			run_main();
		}
		return;
	}
	PID("(");
}

/*
 * Initiate a connection path between the host and the target. The function
 * first goes into arbitration for the SCSI-bus. When this succeeds, the target
 * is selected and an 'IDENTIFY' message is send.
 * Returns -1 when the arbitration failed. Otherwise 0 is returned. When
 * the target does not respond (to either selection or 'MESSAGE OUT') the
 * 'done' function is executed.
 * The result code given by the driver can be influenced by setting 'code'
 * to a non-zero value. This is the case when 'select' is called by abort.
 */
static int
scsi_select(reqp, code)
SC_REQ	*reqp;
{
	u_long	timeout;
	u_char	tmp[1];
	u_char	phase;
	u_long	cnt;
	int	sps;

	DBG_SELPRINT ("Starting arbitration\n", 0);
	PID("T");

	sps = splbio();

	/*
	 * Prevent a race condition here. If a reslection interrupt occurred
	 * between the decision to pick a new request and the call to select,
	 * we abort the selection.
	 * Interrupts are lowered when the 5380 is setup to arbitrate for the
	 * bus.
	 */
	if(connected || (SCSI_5380->scsi_idstat & SC_S_BSY)) {
		splx(sps);
		PID("Y");
		return(-1);
	}

	/*
	 * Set phase bits to 0, otherwise the 5380 won't drive the bus during
	 * selection.
	 */
	SCSI_5380->scsi_tcom = 0;
	SCSI_5380->scsi_icom = 0;

	/*
	 * Arbitrate for the bus.
	 */
	SCSI_5380->scsi_data = SC_HOST_ID;
	SCSI_5380->scsi_mode = SC_ARBIT;
 
	splx(sps);

	cnt = 10000;
	while(!(SCSI_5380->scsi_icom & SC_AIP) && --cnt)
		delay(1);

	if(!(SCSI_5380->scsi_icom & SC_AIP)) {
		SCSI_5380->scsi_mode = IMODE_BASE;
		delay(1);
		PID("U");

		if(SCSI_5380->scsi_idstat & SC_S_BSY) {
			/*
			 * Damn, we have a connected target that we don't know
			 * of. Some targets seem to respond to a selection
			 * AFTER the selection-timeout. Try to get the target
			 * into the Message-out phase so we can send an ABORT
			 * message. We try to avoid resetting the SCSI-bus!
			 */
			if(!reach_msg_out(sizeof(struct scsi_generic))) {
				u_long	len   = 1;
				u_char	phase = PH_MSGOUT;
				u_char	msg   = MSG_ABORT;

				transfer_pio(&phase, &msg, &len);
			}
			else if(SCSI_5380->scsi_idstat & SC_S_BSY)
					scsi_reset();
		}
		PID("I");
		return(-1);
	}

	/* The arbitration delay is 2.2 usecs */
	delay(3);

	/*
	 * Check the result of the arbitration. If we failed, return -1.
	 */
	if(SCSI_5380->scsi_icom & SC_LA) {
		/*
		 * The spec requires that we should read the data register to
		 * check for higher id's and check the SC_LA again.
		 */
		tmp[0] = SCSI_5380->scsi_data;
		if(SCSI_5380->scsi_icom & SC_LA) {
			SCSI_5380->scsi_mode = IMODE_BASE;
			SCSI_5380->scsi_icom = 0;
			DBG_SELPRINT ("Arbitration lost,deassert SC_ARBIT\n",0);
			PID("O");
			return(-1);
		}
	}
	SCSI_5380->scsi_icom = SC_A_SEL | SC_A_BSY;
	if(SCSI_5380->scsi_icom & SC_LA) {
		SCSI_5380->scsi_mode = IMODE_BASE;
		SCSI_5380->scsi_icom = 0;
		DBG_SELPRINT ("Arbitration lost, deassert SC_A_SEL\n", 0);
		PID("P");
		return(-1);
	}
	/* Bus settle delay + Bus clear delay = 1.2 usecs */
	delay(2);
	DBG_SELPRINT ("Arbitration complete\n", 0);

	/*
	 * Now that we won the arbitration, start the selection.
	 */
	SCSI_5380->scsi_data = SC_HOST_ID | (1 << reqp->targ_id);

	/*
	 * Raise ATN while SEL is true before BSY goes false from arbitration,
	 * since this is the only way to guarantee that we'll get a MESSAGE OUT
	 * phase immediately after the selection.
	 */
	SCSI_5380->scsi_icom = SC_A_BSY | SC_A_SEL | SC_A_ATN | SC_ADTB;
	SCSI_5380->scsi_mode = IMODE_BASE;

	/*
	 * Turn off reselection interrupts
	 */
	SCSI_5380->scsi_idstat = 0;

	/*
	 * Reset BSY. The delay following it, surpresses a glitch in the
	 * 5380 which causes us to see our own BSY signal instead of that of
	 * the target.
	 */
	SCSI_5380->scsi_icom  = SC_A_SEL | SC_A_ATN | SC_ADTB;
	delay(1);

	/*
	 * Wait for the target to react, the specs call for a timeout of
	 * 250 ms.
	 */
	cnt = 250000 /* 250 */;
	while(!(SCSI_5380->scsi_idstat & SC_S_BSY) && --cnt)
		delay(1);

	if(!(SCSI_5380->scsi_idstat & SC_S_BSY)) {
		/*
		 * There is no reaction from the target, start the selection
		 * timeout procedure. We release the databus but keep SEL
		 * asserted. After that we wait a 'selection abort time' (200
		 * usecs) and 2 deskew delays (90 ns) and check BSY again.
		 * When BSY is asserted, we assume the selection succeeded,
		 * otherwise we release the bus.
		 */
		SCSI_5380->scsi_icom  = SC_A_SEL | SC_A_ATN;
		delay(201);
		if(!(SCSI_5380->scsi_idstat & SC_S_BSY)) {
			SCSI_5380->scsi_icom = 0;
			reqp->xs->error      = code ? code : XS_SELTIMEOUT;
			DBG_SELPRINT ("Target %d not responding to sel\n",
								reqp->targ_id);
			finish_req(reqp);
			PID("A");
			return(0);
		}
	}
	SCSI_5380->scsi_icom = SC_A_ATN;

	DBG_SELPRINT ("Target %d responding to select.\n", reqp->targ_id);

	/*
	 * The SCSI-interrupts are disabled while a request is being handled.
	 */
	scsi_idisable();

	/*
	 * Since we followed the SCSI-spec and raised ATN while SEL was true
	 * but before BSY was false during the selection, a 'MESSAGE OUT'
	 * phase should follow. Here we send an 'IDENTIFY' message.
	 * Allow disconnect only when interrups are allowed.
	 */
	tmp[0] = MSG_IDENTIFY(0, (reqp->dr_flag & DRIVER_NOINT) ? 0 : 1);
	cnt    = 1;
	phase  = PH_MSGOUT;
	if(transfer_pio(&phase, tmp, &cnt) || cnt) {
		DBG_SELPRINT ("Target %d: failed to send identify\n",
							reqp->targ_id);
		/*
		 * Try to disconnect from the target. We cannot leave it just
		 * hanging here.
		 */
		if(!reach_msg_out(sizeof(struct scsi_generic))) {
			u_long	len   = 1;
			u_char	phase = PH_MSGOUT;
			u_char	msg   = MSG_ABORT;

			transfer_pio(&phase, &msg, &len);
		}
		else scsi_reset();

		SCSI_5380->scsi_icom = 0;
		reqp->xs->error = code ? code : XS_DRIVER_STUFFUP;
		finish_req(reqp);
		PID("S");
		return(0);
	}
	reqp->phase = PH_MSGOUT;

#ifdef notyet /* LWP: Do we need timeouts in the driver? */
	/*
	 * Command is connected, start timer ticking.
	 */
	ccb_p->xtimeout = ccb_p->timeout + Lbolt;
#endif

	connected  = reqp;
	busy      |= 1 << reqp->targ_id;
	PID("D");
	return(0);
}

/*
 * Return codes:
 *	-1: quit main, trigger on interrupt
 *   0: keep on running main.
 */
static int
information_transfer()
{
	SC_REQ	*reqp = connected;
	u_char	tmp, phase;
	u_long	len;

	PID("F");
	/*
	 * Clear pending interrupts from 5380-chip.
	 */
	SC_CLINT;

	/*
	 * We only have a valid SCSI-phase when REQ is asserted. Something
	 * is deadly wrong when BSY has dropped.
	 */
	tmp = SCSI_5380->scsi_idstat;

	if(!(tmp & SC_S_BSY)) {
		busy           &= ~(1 << reqp->targ_id);
		connected       = NULL;
		reqp->xs->error = XS_BUSY;
		finish_req(reqp);
		PID("G");
		return(0);
	}

	if(tmp & SC_S_REQ) {
		phase = (tmp >> 2) & 7;
		if(phase != reqp->phase) {
			reqp->phase = phase;
			DBG_INFPRINT(show_phase, reqp, phase);
		}
	}
	else return(-1);

	switch(phase) {
	    case PH_DATAOUT:

#ifdef DBG_NOWRITE
		printf("NOWRITE set -- write attempt aborted.");
		reqp->msgout = MSG_ABORT;
		SCSI_5380->scsi_icom = SC_A_ATN;
		return(-1);
#endif /* DBG_NOWRITE */

	   case PH_DATAIN:
#ifdef REAL_DMA
		if(reqp->dr_flag & DRIVER_DMAOK) {
			int poll = REAL_DMA_POLL|(reqp->dr_flag & DRIVER_NOINT);
			transfer_dma(reqp, phase, poll);
			if(!poll)
				return(0);
		}
		else
#endif
		{
			PID("H");
			len = reqp->xdata_len;
			transfer_pio(&phase, reqp->xdata_ptr, &len);
			reqp->xdata_ptr += reqp->xdata_len - len;
			reqp->xdata_len  = len;
		}
		return(-1);
	   case PH_MSGIN:
		/*
		 * We only expect single byte messages here.
		 */
		len = 1;
		transfer_pio(&phase, &tmp, &len);
		reqp->message = tmp;
		return(handle_message(reqp, tmp));
	   case PH_MSGOUT:
		len = 1;
		transfer_pio(&phase, &reqp->msgout, &len);
		if(reqp->msgout == MSG_ABORT) {
			busy     &= ~(1 << reqp->targ_id);
			connected = NULL;
			reqp->xs->error = XS_DRIVER_STUFFUP;
			finish_req(reqp);
			PID("J");
			return(0);
		}
		reqp->msgout = MSG_NOOP;
		return(-1);
	   case PH_CMD :
		len = command_size(reqp->xcmd.opcode);
		transfer_pio(&phase, (u_char *)&reqp->xcmd, &len);
		PID("K");
		return(-1);
	   case PH_STATUS:
		len = 1;
		transfer_pio(&phase, &tmp, &len);
		reqp->status = tmp;
		PID("L");
		return(-1);
	   default :
		printf("SCSI: unknown phase on target %d\n", reqp->targ_id);
	}
	PID(":");
	return(-1);
}

/*
 * Handle the message 'msg' send to us by the target.
 * Return values:
 *	 0 : The current command has completed.
 *	-1 : Get on to the next phase.
 */
static int
handle_message(reqp, msg)
SC_REQ	*reqp;
u_int	msg;
{
	int	sps;

	PID(";");
	switch(msg) {
		/*
		 * Linking lets us reduce the time required to get
		 * the next command to the device, skipping the arbitration
		 * and selection time. In the current implementation,
		 * we merely have to start the next command pointed
		 * to by 'next_link'.
		 */
		case MSG_LINK_CMD_COMPLETE:
		case MSG_LINK_CMD_COMPLETEF:
			if(reqp->link == NULL) {
				printf("SCSI: no link for linked command"
					"on target %d\n", reqp->targ_id);
				reqp->msgout = MSG_ABORT;
				SCSI_5380->scsi_icom = SC_A_ATN;
				PID("@");
				return(-1);
			}
			reqp->xs->error = 0;

#ifdef AUTO_SENSE
			if(check_autosense(reqp, 0) == -1)
				return(-1);
#endif /* AUTO_SENSE */

#ifdef DBG_REQ
			show_request(reqp->link, "LINK");
#endif
			connected = reqp->link;
			finish_req(reqp);
			PID("'");
			return(-1);
		case MSG_ABORT:
		case MSG_CMDCOMPLETE:
#ifdef DBG_REQ
			show_request(reqp, "DONE");
#endif
			connected = NULL;	
			busy     &= ~(1 << reqp->targ_id);
			if(!(reqp->dr_flag & DRIVER_AUTOSEN))
				reqp->xs->resid = reqp->xdata_len;
				reqp->xs->error = 0;

#ifdef AUTO_SENSE
			if(check_autosense(reqp, 0) == -1) {
				PID("Z");
				return(0);
			}
#endif /* AUTO_SENSE */

			finish_req(reqp);
			PID("X");
			return(0);
		case MSG_MESSAGE_REJECT:
			PID("C");
			return(-1);
		case MSG_DISCONNECT:
#ifdef DBG_REQ
			show_request(reqp, "DISCON");
#endif
			sps = splbio();
			connected  = NULL;
			reqp->next = discon_q;
			discon_q   = reqp;
			splx(sps);
			PID("V");
			return(0);
		case MSG_SAVEDATAPOINTER:
		case MSG_RESTOREPOINTERS:
			/*
			 * We save pointers implicitely at disconnect.
			 * So we can ignore these messages.
			 */
			PID("B");
			return(-1);
		default: 
			printf("SCSI: unkown message %x on target %d\n", msg,
								reqp->targ_id);
			return(-1);
	}
	PID("N");
	return(-1);
}

/*
 * Handle reselection. If a valid reconnection occurs, connected
 * points at the reconnected command. The command is removed from the
 * disconnected queue.
 */
static void
reselect()
{
	u_char	phase;
	u_long	len;
	u_char	msg;
	u_char	target_mask;
	int	abort = 0;
	SC_REQ	*tmp, *prev;

	PID("M");
	target_mask = SCSI_5380->scsi_data & ~SC_HOST_ID;

	/*
	 * At this point, we have detected that our SCSI-id is on the bus,
	 * SEL is true and BSY was false for at least one bus settle
	 * delay (400 ns.).
	 * We must assert BSY ourselves, until the target drops the SEL signal.
	 */
	SCSI_5380->scsi_icom = SC_A_BSY;
	while(SCSI_5380->scsi_idstat & SC_S_SEL)
		;
	
	SCSI_5380->scsi_icom = 0;
	
	/*
	 * Get the expected identify message.
	 */
	phase = PH_MSGIN;
	len   = 1;
	transfer_pio(&phase, &msg, &len);
	if(len || !MSG_ISIDENTIFY(msg)) {
		printf("SCSI: expecting IDENTIFY, got 0x%x\n", msg);
		abort = 1;
	}
	else {
	    /*
	     * Find the command reconnecting
	     */
	    for(tmp = discon_q, prev = NULL; tmp; prev = tmp, tmp = tmp->next){
		if(target_mask == (1 << tmp->targ_id)) {
			if(prev)
				prev->next = tmp->next;
			else discon_q = tmp->next;
			tmp->next = NULL;
			break;
		}
	    }
	    if(tmp == NULL) {
	      printf("SCSI: no disconnected job for targetmask %x\n",
								target_mask);
	      abort = 1;
	    }
	}
	if(abort) {
		msg   = MSG_ABORT;
		len   = 1;
		phase = PH_MSGOUT;

		SCSI_5380->scsi_icom = SC_A_ATN;
		transfer_pio(&phase, &msg, &len);
	}
	else {
		connected = tmp;
#ifdef DBG_REQ
		show_request(tmp, "RECON");
#endif
	}
	PID("<");
}

/*
 * Transfer data in a given phase using polled I/O.
 * Returns -1 when a different phase is entered without transferring the
 * maximum number of bytes, 0 if all bytes or exit is in the same
 * phase.
 */
static int
transfer_pio(phase, data, len)
u_char	*phase;
u_char	*data;
u_long	*len;
{
	u_int	cnt = *len;
	u_char	ph  = *phase;
	u_char	tmp;

	DBG_PIOPRINT ("SCSI: transfer_pio start: phase: %d, len: %d\n", ph,cnt);
	PID(",");
	SCSI_5380->scsi_tcom = ph;
	do {
	    if(!wait_req_true()) {
		DBG_PIOPRINT ("SCSI: transfer_pio: missing REQ\n", 0, 0);
		break;
	    }
	    if(((SCSI_5380->scsi_idstat >> 2) & 7) != ph) {
		DBG_PIOPRINT ("SCSI: transfer_pio: phase mismatch\n", 0, 0);
		break;
	    }
	    if(PH_IN(ph)) {
		*data++ = SCSI_5380->scsi_data;
		SCSI_5380->scsi_icom = SC_A_ACK;
	    }
	    else {
		SCSI_5380->scsi_data = *data++;

		/*
		 * The SCSI-standard suggests that in the 'MESSAGE OUT' phase,
		 * the initiator should drop ATN on the last byte of the
		 * message phase after REQ has been asserted for the handshake
		 * but before the initiator raises ACK.
		 */
		if(!( (ph == PH_MSGOUT) && (cnt > 1) )) {
			SCSI_5380->scsi_icom = SC_ADTB;
			SCSI_5380->scsi_icom = SC_ADTB | SC_A_ACK;
		}
		else {
			SCSI_5380->scsi_icom = SC_ADTB | SC_A_ATN;
			SCSI_5380->scsi_icom = SC_ADTB | SC_A_ATN | SC_A_ACK;
		}
	    }
	    if(!wait_req_false()) {
		DBG_PIOPRINT ("SCSI: transfer_pio - REQ not dropping\n", 0, 0);
		break;
	    }

	    if(!( (ph == PH_MSGOUT) && (cnt > 1) ))
		SCSI_5380->scsi_icom = 0;
	    else SCSI_5380->scsi_icom = SC_A_ATN;
	} while(--cnt);

	if((tmp = SCSI_5380->scsi_idstat) & SC_S_REQ)
		*phase = (tmp >> 2) & 7;
	else *phase = NR_PHASE;
	*len = cnt;
	DBG_PIOPRINT ("SCSI: transfer_pio done: phase: %d, len: %d\n",
								*phase, cnt);
	PID(">");
	if(!cnt || (*phase == ph))
		return(0);
	return(-1);
}

/*
 * Start a DMA-transfer on the device using the current pointers.
 * If 'poll' is true, the function waits until DMA-has completed.
 */
static void
transfer_dma(reqp, phase, poll)
SC_REQ	*reqp;
u_int	phase;
int	poll;
{
	int	dmastat;
	u_char	mbase = 0;
	int	sps;

again:
	PID("?");
	/*
	 * We should be in phase, otherwise we are not allowed to
	 * drive the bus.
	 */
	SCSI_5380->scsi_tcom = phase;

	/*
	 * Defer interrupts until DMA is fully running.
	 */
	sps = splbio();

	/*
	 * Clear pending interrupts and parity errors.
	 */
	SCSI_DMA->s_dma_ctrl = 0;
	SC_CLINT;

	if(!poll) {
		/*
		 * Enable SCSI interrupts and set IN_DMA flag, set 'mbase'
		 * to the interrupts we want enabled.
		 */
		scsi_ienable();
		reqp->dr_flag |= DRIVER_IN_DMA;
		mbase = SC_E_EOPI | SC_MON_BSY;
	}

	if(PH_IN(phase)) {
		SCSI_DMA->s_dma_ctrl = SD_IN;
		set_scsi_dma(&(SCSI_DMA->s_dma_ptr), reqp->dm_cur->dm_addr);
		set_scsi_dma(&(SCSI_DMA->s_dma_cnt), reqp->dm_cur->dm_count);
		SCSI_5380->scsi_icom  = 0;
		SCSI_5380->scsi_mode  = IMODE_BASE | mbase | SC_M_DMA;
		SCSI_DMA->s_dma_ctrl  = SD_ENABLE;
		SCSI_5380->scsi_ircv  = 0;
	}
	else {
		SCSI_DMA->s_dma_ctrl = SD_OUT;
		set_scsi_dma(&(SCSI_DMA->s_dma_ptr), reqp->dm_cur->dm_addr);
		set_scsi_dma(&(SCSI_DMA->s_dma_cnt), reqp->dm_cur->dm_count);
		SCSI_5380->scsi_mode   = IMODE_BASE | mbase | SC_M_DMA;
		SCSI_5380->scsi_icom   = SC_ADTB;
		SCSI_5380->scsi_dmstat = 0;
		SCSI_DMA->s_dma_ctrl   = SD_ENABLE|SD_OUT;
	}
	splx(sps);

	if(poll) {
		/*
		 * We wait here until the DMA has finished. This can be
		 * achieved by checking the following conditions:
		 *   - 5380:
		 *	- End of DMA flag is set
		 *	- We lost BSY (error!!)
		 *	- A phase mismatch has occured (partial transfer)
		 *   - DMA-controller:
		 *	- A bus error occurred (Kernel error!!)
		 *	- All bytes are transferred
		 * We one of the terminating conditions was met, we call
		 * 'dma_ready' to check errors and perform the bookkeeping.
		 */
		u_char	dmstat, dmastat;
		int	dma_done;

		PID("/");
		for(;;) {
			dmstat  = SCSI_5380->scsi_dmstat;
			dmastat = SCSI_DMA->s_dma_ctrl;
			if(dmstat & (SC_END_DMA|SC_BSY_ERR|SC_IRQ_SET))
				break;
			if(!(dmstat & SC_PHS_MTCH))
				break;
			if(dmastat & (SD_BUSERR|SD_ZERO))
				break;
		}
		PID("q");
		if(dmastat & (SD_BUSERR|SD_ZERO))
			dma_done = dma_ready(1, dmastat);
		else dma_done = dma_ready(0, 0);
		if(!dma_done)
			goto again;
		
	}
	PID("w");
}

/*
 * Check results of a DMA data-transfer.
 */
static int
dma_ready(has_dmastat, dmastat)
int	has_dmastat, dmastat;
{
	int	dmstat, phase;
	long	tmp, bytes_left, bytes_done;
	int	i;
	SC_REQ	*reqp = connected;

	/*
	 * Get values of the status registers of the 5380 & DMA-controller.
	 */
	dmstat  = SCSI_5380->scsi_dmstat;
	if(!has_dmastat)
		dmastat = SCSI_DMA->s_dma_ctrl;

	/*
	 * Check if the call is sensible and not caused by any spurious
	 * interrupt.
	 */
	if(    !(dmastat & (SD_BUSERR|SD_ZERO))
		&& !(dmstat & (SC_END_DMA|SC_BSY_ERR))
		&&  (dmstat & SC_PHS_MTCH) ) {
		printf("dma_ready: spurious call (dma:%x,dm:%x,last_hit: %s)\n",
						dmastat, dmstat, last_hit);
		return(0);
	}

	/*
	 * If end of a DMA transfer, check it's results.
	 */
	get_scsi_dma(SCSI_DMA->s_dma_cnt, bytes_left);

	if(dmastat & SD_BUSERR) {
		/*
		 * The DMA-controller seems to access 8 bytes beyond
		 * it's limits on output. Therefore check also the byte
		 * count. If it's zero, ignore the bus error.
		 */
		if(bytes_left != 0) {
			get_scsi_dma(SCSI_DMA->s_dma_ptr, tmp);
			printf("SCSI-DMA buserror - accessing 0x%x\n", tmp);
			panic("SCSI");
		}
	}

	if(bytes_left != 0) {
		/*
		 * The byte-count is not zero. This is not always an error,on
		 * tape drives this usually means EOF. We check that the
		 * residu is a multiple of 512-bytes, when we know the device
		 * type it should be included in the check.
		 * A severe complication is that if a device wants to
		 * disconnect in the middle of a DMA-transfer, the 5380 has
		 * already prefetched the next byte from the DMA-controller
		 * before the phase mismatch occurs. I don't know how this
		 * fact can be uniquely identified. For the moment, we round
		 * up the residual if it is odd.
		 */
		if(PH_OUT(reqp->phase) && (bytes_left & 1))
				bytes_left++;

	}
	else {
	    if(PH_IN(reqp->phase)) {
		/*
		 * Check for bytes not transfered. This should never occur
		 * because the buffer-pool is aligned on a 4-byte boundary.
		 */
		u_char	*p, *q;

		if(bytes_left > 3) {
			scsi_show();
			printf("SCSI: %d bytes not transferred\n", bytes_left);
			panic("SCSI");
		}
		p = (u_char*)(bytes_left & ~3);
		q = (u_char*)&(SCSI_DMA->s_dma_res);
		switch(bytes_left & 3) {
			case 3: *p++ = *q++;
			case 2: *p++ = *q++;
			case 1: *p++ = *q++;
				printf("SCSI: dma residue count != 0, ");
				printf("Check buffer alignment\n");
		}
	    }
	}

	/*
	 * Update various transfer-pointers/lengths
	 */
	bytes_done = reqp->dm_cur->dm_count - bytes_left;

	reqp->xdata_ptr  = &reqp->xdata_ptr[bytes_done];	/* XXX */
	reqp->xdata_len -= bytes_done;				/* XXX */
	if((reqp->dm_cur->dm_count -= bytes_done) == 0)
		reqp->dm_cur++;
	else reqp->dm_cur->dm_addr += bytes_done;

	if(PH_IN(reqp->phase) && (dmstat & SC_PAR_ERR)) {
		if(!(ncr5380_no_parchk & (1 << reqp->targ_id)))
			/* XXX: Should be parity error ???? */
			reqp->xs->error = XS_DRIVER_STUFFUP;
	}

	/*
	 * DMA mode should always be reset even when we will continue with the
	 * next chain.
	 */
	SCSI_5380->scsi_mode &= ~SC_M_DMA;

	if((dmstat & SC_BSY_ERR) || !(dmstat & SC_PHS_MTCH)
				 || (reqp->dm_cur > reqp->dm_last)) {
		/*
		 * Tell interrupt functions DMA mode has ended.
		 */
		reqp->dr_flag &= ~DRIVER_IN_DMA;

		/*
		 * Turn off DMA-mode and clear icom
		 */
		SCSI_5380->scsi_mode &=
				~(SC_M_DMA|SC_E_EOPI|SC_MON_BSY|SC_E_PARI);
		SCSI_5380->scsi_icom  = 0;

		/*
		 * Clear all (pending) interrupts.
		 */
		SCSI_DMA->s_dma_ctrl  = 0;
		SC_CLINT;

		if(dmstat & SC_BSY_ERR) {
			if(!reqp->xs->error)
				reqp->xs->error = XS_BUSY;
			finish_req(reqp);
			PID("r");
			return(1);
		}

		if(reqp->xs->error != 0) {
printf("dma-ready: code = %d\n", reqp->xs->error); /* LWP */
			reqp->msgout = MSG_ABORT;
			SCSI_5380->scsi_icom = SC_A_ATN;
		}
		PID("t");
		return(1);
	}
	return(0);
}

static int
check_autosense(reqp, linked)
SC_REQ	*reqp;
int	linked;
{
	int	sps;

	/*
	 * If we not executing an auto-sense and the status code
	 * is request-sense, we automatically issue a request
	 * sense command.
	 */
	PID("y");
	if(!(reqp->dr_flag & DRIVER_AUTOSEN)) {
		if(reqp->status == SCSCHKC) {
			memcpy(&reqp->xcmd, sense_cmd, sizeof(sense_cmd));
			reqp->xdata_ptr = (u_char *)&reqp->xs->sense;
			reqp->xdata_len = sizeof(reqp->xs->sense);
			reqp->dr_flag  |= DRIVER_AUTOSEN;
			reqp->dr_flag  &= ~DRIVER_DMAOK;
			if(!linked) {
				sps = splbio();
				reqp->next = issue_q;
				issue_q    = reqp;
				splx(sps);
			}
			else reqp->xcmd.bytes[4] |= 1;

#ifdef DBG_REQ
			show_request(reqp, "AUTO-SENSE");
#endif
			PID("u");
			return(-1);
		}
	}
	else {
		/*
		 * An auto-sense has finished
		 */
		if((reqp->status & SCSMASK) != SCSGOOD)
			reqp->xs->error = XS_DRIVER_STUFFUP; /* SC_E_AUTOSEN; */
		else reqp->xs->error = XS_SENSE;
		reqp->status = SCSCHKC;
	}
	PID("i");
	return(0);
}

static int
reach_msg_out(len)
u_long	len;
{
	u_char	phase;
	u_char	data;

	printf("SCSI: Trying to reach Message-out phase\n");
	if((phase = SCSI_5380->scsi_idstat) & SC_S_REQ)
		phase = (phase >> 2) & 7;
	else return(-1);
	printf("SCSI: Trying to reach Message-out phase, now: %d\n", phase);
	if(phase == PH_MSGOUT)
		return(0);

	SCSI_5380->scsi_tcom = phase;

	do {
		if(!wait_req_true()) {
			break;
		}
		if(((SCSI_5380->scsi_idstat >> 2) & 7) != phase) {
			break;
		}
		if(PH_IN(phase)) {
			data = SCSI_5380->scsi_data;
			SCSI_5380->scsi_icom = SC_A_ACK | SC_A_ATN;
		}
		else {
			SCSI_5380->scsi_data = 0;
			SCSI_5380->scsi_icom = SC_ADTB | SC_A_ATN | SC_A_ACK;
		}
		if(!wait_req_false()) {
			break;
		}
		SCSI_5380->scsi_icom = SC_A_ATN;
	} while(--len);

	if((phase = SCSI_5380->scsi_idstat) & SC_S_REQ) {
		phase = (phase >> 2) & 7;
		if(phase == PH_MSGOUT) {
			printf("SCSI: Message-out phase reached.\n");
			return(0);
		}
	}
	return(-1);
}

static void
scsi_reset()
{
	SC_REQ	*tmp, *next;
	int		sps;

	printf("SCSI: resetting SCSI-bus\n");

	PID("7");
	sps = splbio();
	SCSI_5380->scsi_icom = SC_A_RST;
	delay(1);
	SCSI_5380->scsi_icom = 0;

	/*
	 * None of the jobs in the discon_q will ever be reconnected,
	 * notify this to the higher level code.
	 */
	for(tmp = discon_q; tmp ;) {
		next = tmp->next;
		tmp->next = NULL;
		tmp->xs->error = XS_TIMEOUT;
		busy &= ~(1 << tmp->targ_id);
		finish_req(tmp);
		tmp = next;
	}
	discon_q = NULL;

	/*
	 * The current job will never finish either.
	 * The problem is that we can't finish the job because an instance
	 * of main is running on it. Our best guess is that the job is currently
	 * doing REAL-DMA. In that case 'dma_ready()' should correctly finish
	 * the job because it detects BSY-loss.
	 */
	if(tmp = connected) {
		if(tmp->dr_flag & DRIVER_IN_DMA) {
			tmp->xs->error = XS_DRIVER_STUFFUP;
			dma_ready(0, 0);
		}
	}
	splx(sps);
	PID("8");
}

/*
 * Check validity of the IRQ set by the 5380. If the interrupt is valid,
 * the appropriate action is carried out (reselection or DMA ready) and
 * INTR_RESEL or INTR_DMA is returned. Otherwise a console notice is written
 * and INTR_SPURIOUS is returned.
 */
static int
check_intr()
{
	SC_REQ	*reqp;

	if((SCSI_5380->scsi_idstat & (SC_S_SEL|SC_S_IO))==(SC_S_SEL|SC_S_IO))
		return(INTR_RESEL);
	else {
		if((reqp = connected) && (reqp->dr_flag & DRIVER_IN_DMA)){
			reqp->dr_flag &= ~DRIVER_IN_DMA;
			return(INTR_DMA);
		}
	}
	SC_CLINT;
	printf("-->");
	scsi_show();
	printf("SCSI: spurious interrupt.\n");
	return(INTR_SPURIOUS);
}

/*
 * Check if DMA can be used for this request. This function also builds
 * the dma-chain.
 */
static int
scsi_dmaok(reqp)
SC_REQ	*reqp;
{
	u_long			phy_buf;
	u_long			phy_len;
	void			*req_addr;
	u_long			req_len;
	struct dma_chain	*dm;

	/*
	 * Initialize locals and requests' DMA-chain.
	 */
	req_len  = reqp->xdata_len;
	req_addr = (void*)reqp->xdata_ptr;
	dm       = reqp->dm_cur = reqp->dm_last = reqp->dm_chain;

	dm->dm_count = dm->dm_addr = 0;

	/*
	 * Do not accept zero length DMA.
	 */
	if(req_len == 0)
		return(0);

	/*
	 * LWP: I think that this restriction is not strictly nessecary.
	 */
	if((req_len & 0x1) || ((u_int)req_addr & 0x3))
		return(0);

	/*
	 * Build the DMA-chain.
	 */
	dm->dm_addr = phy_buf = kvtop(req_addr);
	while(req_len) {
		if(req_len < (phy_len = NBPG - ((u_long)req_addr & PGOFSET)))
			phy_len = req_len;

		req_addr     += phy_len;
		req_len      -= phy_len;
		dm->dm_count += phy_len;

#ifdef NO_TTRAM_DMA
		/*
		 * LWP: DMA transfers to TT-ram causes data to be garbeled
		 * without notice on some revisons of the TT-mainboard.
		 * When program's generate misterious Segmentations faults,
		 * try turning on NO_TTRAM_DMA.
		 */
		if(dm->dm_addr > 0x1000000) /* XXX: should be a define??? */
			return(0);
#endif

		if(req_len) {
			u_long	tmp = kvtop(req_addr);

			if((phy_buf + phy_len) != tmp) {
			    if(++dm >= &reqp->dm_chain[MAXDMAIO]) {
				printf("ncr5380_dmaok: DMA chain too long!\n");
				return(0);
			    }
			    dm->dm_count = 0;
			    dm->dm_addr  = tmp;
			}
			phy_buf = tmp;
		}
	}
	reqp->dm_last = dm;
	return(1);
}

static void
run_main(void)
{
	int	sps = splbio();

	callback_scheduled = 0;
	if(!main_running) {
		main_running = 1;
		splx(sps);
		scsi_main();
	}
	else splx(sps);
}

/****************************************************************************
 *		Start Debugging Functions				    *
 ****************************************************************************/
static void
show_data_sense(xs)
struct scsi_xfer	*xs;
{
	u_char	*b;
	int	i;
	int	sz;

	b = (u_char *) xs->cmd;
	printf("cmd[%d,%d]: ", xs->cmdlen, sz = command_size(*b));
	for(i = 0; i < sz; i++)
		printf("%x ", b[i]);
	printf("\nsense: ");
	b = (u_char *)&xs->sense;
	for(i = 0; i < sizeof(xs->sense); i++)
		printf("%x ", b[i]);
	printf("\n");
}

static void
show_request(reqp, qtxt)
SC_REQ	*reqp;
char	*qtxt;
{
	printf("REQ-%s: %d %x[%d] cmd[0]=%x S=%x M=%x R=%x %s\n", qtxt,
			reqp->targ_id, reqp->xdata_ptr, reqp->xdata_len,
			reqp->xcmd.opcode, reqp->status, reqp->message,
			reqp->xs->error, reqp->link ? "L" : "");
	if(reqp->status == SCSCHKC)
		show_data_sense(reqp->xs);
}

scsi_show()
{
	SC_REQ	*tmp;
	int	sps = splhigh();

#ifndef DBG_PID
	#define	last_hit	""
#endif

	for(tmp = issue_q; tmp; tmp = tmp->next)
		show_request(tmp, "ISSUED");
	for(tmp = discon_q; tmp; tmp = tmp->next)
		show_request(tmp, "DISCONNECTED");
	if(connected)
		show_request(connected, "CONNECTED");
	/* show_signals(); */
	if(connected)
		printf("phase = %d, ", connected->phase);
	printf("busy:%x, last_hit:%s, spl:%04x\n", busy, last_hit, sps);

	splx(sps);
}
