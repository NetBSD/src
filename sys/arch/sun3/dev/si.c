/*
 * Copyright (C) 1994 Adam Glass, Gordon W. Ross
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: si.c,v 1.7 1994/09/20 16:21:46 gwr Exp $
 */

/* #define DEBUG 1 */

/* XXX - Need to add support for real DMA. -gwr */
/* #define PSEUDO_DMA 1 (broken) */

static int si_debug=0;

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/isr.h>
#include <machine/obio.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_debug.h>
#include <scsi/scsiconf.h>

#include "scsi_defs.h"
#include "scsi_5380.h"

#define SCI_PHASE_DISC		0	/* sort of ... */
#define SCI_CLR_INTR(regs)	{register int temp = regs->sci_iack;}
#define SCI_ACK(ptr,phase)	(ptr)->sci_tcmd = (phase)
#define SCSI_TIMEOUT_VAL	10000000
#define WAIT_FOR_NOT_REQ(ptr) {	\
	int scsi_timeout = SCSI_TIMEOUT_VAL; \
	while ( ((ptr)->sci_bus_csr & SCI_BUS_REQ) && \
		 ((ptr)->sci_bus_csr & SCI_BUS_REQ) && \
		 ((ptr)->sci_bus_csr & SCI_BUS_REQ) && \
		 (--scsi_timeout) ); \
	if (!scsi_timeout) { \
		printf("scsi timeout--WAIT_FOR_NOT_REQ---%s, line %d.\n", \
			__FILE__, __LINE__); \
		goto scsi_timeout_error; \
	} \
	}
#define WAIT_FOR_REQ(ptr) {	\
	int scsi_timeout = SCSI_TIMEOUT_VAL; \
	while ( (((ptr)->sci_bus_csr & SCI_BUS_REQ) == 0) && \
		(((ptr)->sci_bus_csr & SCI_BUS_REQ) == 0) && \
		(((ptr)->sci_bus_csr & SCI_BUS_REQ) == 0) && \
		 (--scsi_timeout) ); \
	if (!scsi_timeout) { \
		printf("scsi timeout--WAIT_FOR_REQ---%s, line %d.\n", \
			__FILE__, __LINE__); \
		goto scsi_timeout_error; \
	} \
	}
#define WAIT_FOR_BSY(ptr) {	\
	int scsi_timeout = SCSI_TIMEOUT_VAL; \
	while ( (((ptr)->sci_bus_csr & SCI_BUS_BSY) == 0) && \
		(((ptr)->sci_bus_csr & SCI_BUS_BSY) == 0) && \
		(((ptr)->sci_bus_csr & SCI_BUS_BSY) == 0) && \
		 (--scsi_timeout) ); \
	if (!scsi_timeout) { \
		printf("scsi timeout--WAIT_FOR_BSY---%s, line %d.\n", \
			__FILE__, __LINE__); \
		goto scsi_timeout_error; \
	} \
	}

#ifdef DDB
int Debugger();
#else
#define Debugger() panic("Should call Debugger here %s:%d", \
			 __FILE__, __LINE__)
#endif

struct ncr5380_softc {
    struct device sc_dev;
    void *sc_regs;
    int sc_adapter_target;
    struct scsi_link sc_link;
};

static u_int	ncr5380_adapter_info(struct ncr5380_softc *ncr5380);
static void		ncr5380_minphys(struct buf *bp);
static int		ncr5380_scsi_cmd(struct scsi_xfer *xs);

static int		ncr5380_show_scsi_cmd(struct scsi_xfer *xs);
static int		ncr5380_reset_target(int adapter, int target);
static int		ncr5380_poll(int adapter, int timeout);
static int		ncr5380_send_cmd(struct scsi_xfer *xs);

void		ncr5380_intr(int adapter);

static int	si_generic(int adapter, int id, int lun,
			 struct scsi_generic *cmd, int cmdlen,
			 void *databuf, int datalen);
static int	si_group0(int adapter, int id, int lun,
			    int opcode, int addr, int len,
			    int flags, caddr_t databuf, int datalen);

static char scsi_name[] = "si";

struct scsi_adapter	ncr5380_switch = {
	ncr5380_scsi_cmd,		/* scsi_cmd()		*/
	ncr5380_minphys,		/* scsi_minphys()	*/
	0,				/* open_target_lu()	*/
	0,				/* close_target_lu()	*/
	ncr5380_adapter_info,		/* adapter_info()	*/
	scsi_name,			/* name			*/
	0, 0				/* spare[2]		*/
};

/* This is copied from julian's bt driver */
/* "so we have a default dev struct for our link struct." */
struct scsi_device ncr_dev = {
	NULL,		/* Use default error handler.	    */
	NULL,		/* Use default start handler.		*/
	NULL,		/* Use default async handler.	    */
	NULL,		/* Use default "done" routine.	    */
	"si",		/* name of device type */
	0,			/* device type dependent flags */
	0, 0		/* spare[2] */
};

extern int	matchbyname();
static int	si_match();
static void	si_attach();

struct cfdriver sicd = {
	NULL, "si", si_match, si_attach, DV_DULL,
	sizeof(struct ncr5380_softc), NULL, 0,
};

static int
si_print(aux, name)
	void *aux;
	char *name;
{
	if (name)
		printf("%s: (sc_link = 0x%x)", name, (int) aux);
	return UNCONF;
}

static int
si_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
    caddr_t si_addr;
    struct obio_cf_loc *obio_loc = (struct obio_cf_loc *) CFDATA_LOC(cf);

    si_addr = OBIO_DEFAULT_PARAM(caddr_t, obio_loc->obio_addr, OBIO_NCR_SCSI);
    return !obio_probe_byte(si_addr);
}

static void
si_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
    caddr_t dvma_malloc();
	int si_addr, level;
	int unit = DEVICE_UNIT(self);
    struct ncr5380_softc *ncr5380 = (struct ncr5380_softc *) self;
    struct obio_cf_loc *obio_loc = OBIO_LOC(self);
	struct cfdata *new_match;

    si_addr = OBIO_DEFAULT_PARAM(int, obio_loc->obio_addr, OBIO_NCR_SCSI);
    ncr5380->sc_regs = (sci_regmap_t *)
        obio_alloc(si_addr, OBIO_NCR_SCSI_SIZE);

    level = OBIO_DEFAULT_PARAM(int, obio_loc->obio_level, 2);

    ncr5380->sc_link.scsibus = unit;	/* needed? */
    ncr5380->sc_link.adapter_softc = ncr5380;
    ncr5380->sc_link.adapter_targ = 7;
    ncr5380->sc_link.adapter = &ncr5380_switch;
    ncr5380->sc_link.device = &ncr_dev;
    
    obio_print(si_addr, level);
    printf("\n");

    config_found(self, &(ncr5380->sc_link), si_print);
}

static u_int
ncr5380_adapter_info(struct ncr5380_softc *ncr5380)
{
	return 1;
}

#define MIN_PHYS	65536	/*BARF!!!!*/
static void
ncr5380_minphys(struct buf *bp)
{
	if (bp->b_bcount > MIN_PHYS) {
		printf("Uh-oh...  ncr5380_minphys setting bp->b_bcount = %x.\n", MIN_PHYS);
		bp->b_bcount = MIN_PHYS;
	}
}
#undef MIN_PHYS

static int
ncr5380_scsi_cmd(struct scsi_xfer *xs)
{
	int flags, s, r;

	flags = xs->flags;
	if (xs->bp) flags |= (SCSI_NOSLEEP);
	if ( flags & ITSDONE ) {
		printf("Already done?");
		xs->flags &= ~ITSDONE;
	}
	if ( ! ( flags & INUSE ) ) {
		printf("Not in use?");
		xs->flags |= INUSE;
	}

	if ( flags & SCSI_RESET ) {
		printf("flags & SCSIRESET.\n");
		if ( ! ( flags & SCSI_NOSLEEP ) ) {
			s = splbio();
			ncr5380_reset_target(xs->sc_link->scsibus, xs->sc_link->target);
			splx(s);
			return(SUCCESSFULLY_QUEUED);
		} else {
			ncr5380_reset_target(xs->sc_link->scsibus, xs->sc_link->target);
			if (ncr5380_poll(xs->sc_link->scsibus, xs->timeout)) {
				return (HAD_ERROR);
			}
			return (COMPLETE);
		}
	}
#if 0
	/*
	 * OK.  Now that that's over with, let's pack up that
	 * SCSI puppy and send it off.  If we can, we'll just
	 * queue and go; otherwise, we'll wait for the command
	 * to finish.
	 */
	if ( ! ( flags & SCSI_NOSLEEP ) ) {
		s = splbio();
		ncr5380_send_cmd(xs);
		splx(s);
		return(SUCCESSFULLY_QUEUED);
	}
#endif

	r = ncr5380_send_cmd(xs);
	xs->flags |= ITSDONE;
	scsi_done(xs);
	switch(r) {
		case COMPLETE:
		case SUCCESSFULLY_QUEUED:
			r = SUCCESSFULLY_QUEUED;
			if (xs->flags&SCSI_NOMASK)
				r = COMPLETE;
			break;
		default:
			break;
	}
	return r;
#if 0
	do {
		if (ncr5380_poll(xs->sc_link->scsibus, xs->timeout)) {
			if ( ! ( xs->flags & SCSI_SILENT ) )
				printf("cmd fail.\n");
			cmd_cleanup
			xs->error = XS_DRIVER_STUFFUP;
			splx(s);
		}
	} while ( ! ( xs->flags & ITSDONE ) );
#endif
}

static int
ncr5380_show_scsi_cmd(struct scsi_xfer *xs)
{
	u_char	*b = (u_char *) xs->cmd;
	int	i  = 0;

	if ( ! ( xs->flags & SCSI_RESET ) ) {
		printf("si(%d:%d:%d)-",
			   xs->sc_link->scsibus,
			   xs->sc_link->target,
			   xs->sc_link->lun);
		while (i < xs->cmdlen) {
			if (i) printf(",");
			printf("%x",b[i++]);
		}
		printf("-\n");
	} else {
		printf("si(%d:%d:%d)-RESET-\n",
			   xs->sc_link->scsibus,
			   xs->sc_link->target,
			   xs->sc_link->lun);
	}
}

/*
 * Actual chip control.
 */

void
ncr5380_intr(int adapter)
{
    register struct ncr5380_softc *ncr5380 = sicd.cd_devs[adapter];
    register volatile sci_regmap_t *regs = ncr5380->sc_regs;

    SCI_CLR_INTR(regs);
    regs->sci_mode    = 0x00;
#ifdef	DEBUG
	printf ("ncr_intr\n");
#endif
}

int
scsi_irq_intr(void)
{
#if 0	/* XXX - no way to get regs */
    register volatile sci_regmap_t *regs = ncr;
	if (regs->sci_csr != SCI_CSR_PHASE_MATCH)
		printf("scsi_irq_intr called (not just phase match -- "
			"csr = 0x%x, bus_csr = 0x%x).\n",
			regs->sci_csr, regs->sci_bus_csr);
	ncr5380_intr(0);
#endif
	return 1;
}

int
scsi_drq_intr(void)
{
#if 0
	printf("scsi_drq_intr called.\n");
	ncr5380_intr(0);
#endif
	return 1;
}

static int
ncr5380_reset_target(int adapter, int target)
{
	register struct ncr5380_softc *ncr5380 = sicd.cd_devs[adapter];
	register volatile sci_regmap_t *regs = ncr5380->sc_regs;
	int dummy;

	regs->sci_icmd = SCI_ICMD_TEST;
	regs->sci_icmd = SCI_ICMD_TEST | SCI_ICMD_RST;
	delay(2500);
	regs->sci_icmd = 0;

	regs->sci_mode = 0;
	regs->sci_tcmd = SCI_PHASE_DISC;
	regs->sci_sel_enb = 0;

	SCI_CLR_INTR(regs);
	SCI_CLR_INTR(regs);
}

static int
ncr5380_poll(int adapter, int timeout)
{
}

static int
ncr5380_send_cmd(struct scsi_xfer *xs)
{
	int	s;
	int	sense;

/*	ncr5380_show_scsi_cmd(xs); */
	s = splbio();
	sense = si_generic( xs->sc_link->scsibus, xs->sc_link->target,
			  xs->sc_link->lun, xs->cmd, xs->cmdlen,
			  xs->data, xs->datalen );
	splx(s);
	if (sense) {
		switch (sense) {
			case 0x02:	/* Check condition */
#ifdef	DEBUG
				printf("check cond. target %d.\n",
					   xs->sc_link->target);
#endif
				delay(10);	/* Phil's fix for slow devices. */
				s = splbio();
				si_group0(xs->sc_link->scsibus,
					    xs->sc_link->target,
					    xs->sc_link->lun,
					    0x3, 0x0,
					    sizeof(struct scsi_sense_data),
					    0, (caddr_t) &(xs->sense),
					    sizeof(struct scsi_sense_data));
				splx(s);
				xs->error = XS_SENSE;
				return HAD_ERROR;
			case 0x08:	/* Busy */
				xs->error = XS_BUSY;
				return HAD_ERROR;
			default:
				xs->error = XS_DRIVER_STUFFUP;
				return HAD_ERROR;
		}
	}
	xs->error = XS_NOERROR;
	return (COMPLETE);
}

static int
si_select_target(register volatile sci_regmap_t *regs,
	      u_char myid, u_char tid, int with_atn)
{
	register u_char	bid, icmd;
	int		ret = SCSI_RET_RETRY;

	if ((regs->sci_bus_csr & (SCI_BUS_BSY|SCI_BUS_SEL)) &&
	    (regs->sci_bus_csr & (SCI_BUS_BSY|SCI_BUS_SEL)) &&
	    (regs->sci_bus_csr & (SCI_BUS_BSY|SCI_BUS_SEL)))
		return ret;

	/* for our purposes.. */
	myid = 1 << myid;
	tid = 1 << tid;

	regs->sci_sel_enb = 0; /* we don't want any interrupts. */
	regs->sci_tcmd = 0;	/* get into a harmless state */
	regs->sci_mode = 0;	/* get into a harmless state */

	regs->sci_odata = myid;
	regs->sci_mode = SCI_MODE_ARB;
/*	regs->sci_mode |= SCI_MODE_ARB;	XXX? */
	/* AIP might not set if BSY went true after we checked */
	for (bid = 0; bid < 20; bid++)	/* 20usec circa */
		if (regs->sci_icmd & SCI_ICMD_AIP)
			break;
	if ((regs->sci_icmd & SCI_ICMD_AIP) == 0) {
		goto lost;
	}

	delay(2200);	/* 2.2 millisecond arbitration delay */

	if (regs->sci_icmd & SCI_ICMD_LST) {
#ifdef	DEBUG
		printf ("lost 1\n");
#endif
		goto lost;
	}

	regs->sci_mode &= ~SCI_MODE_PAR_CHK;
	bid = regs->sci_data;

	if ((bid & ~myid) > myid) {
#ifdef	DEBUG
		printf ("lost 2\n");
#endif
		goto lost;
	}
	if (regs->sci_icmd & SCI_ICMD_LST) {
#ifdef	DEBUG
		printf ("lost 3\n");
#endif
		goto lost;
	}

	/* Won arbitration, enter selection phase now */	
	icmd = regs->sci_icmd & ~(SCI_ICMD_DIFF|SCI_ICMD_TEST);
	icmd |= (with_atn ? (SCI_ICMD_SEL|SCI_ICMD_ATN) : SCI_ICMD_SEL);
	icmd |= SCI_ICMD_BSY;
	regs->sci_icmd = icmd;

	if (regs->sci_icmd & SCI_ICMD_LST) {
#ifdef	DEBUG
		printf ("nosel\n");
#endif
		goto nosel;
	}

	/* XXX a target that violates specs might still drive the bus XXX */
	/* XXX should put our id out, and after the delay check nothi XXX */
	/* XXX ng else is out there.				      XXX */

	delay2us();

	regs->sci_tcmd = 0;
	regs->sci_odata = myid | tid;
	regs->sci_sel_enb = 0;

/*	regs->sci_mode &= ~SCI_MODE_ARB;	 2 deskew delays, too */
	regs->sci_mode = 0;			/* 2 deskew delays, too */
	
	icmd |= SCI_ICMD_DATA;
	icmd &= ~(SCI_ICMD_BSY);

	regs->sci_icmd = icmd;

	/* bus settle delay, 400ns */
	delay2us(); /* too much (was 2) ? */

/*	regs->sci_mode |= SCI_MODE_PAR_CHK; */

	{
		register int timeo  = 2500;/* 250 msecs in 100 usecs chunks */
		while ((regs->sci_bus_csr & SCI_BUS_BSY) == 0) {
			if (--timeo > 0) {
				delay(100);
			} else {
				goto nodev;
			}
		}
	}

	icmd &= ~(SCI_ICMD_DATA|SCI_ICMD_SEL);
	regs->sci_icmd = icmd;
/*	regs->sci_sel_enb = myid;*/	/* looks like we should NOT have it */
	return SCSI_RET_SUCCESS;
nodev:
	ret = SCSI_RET_DEVICE_DOWN;
	regs->sci_sel_enb = myid;
nosel:
	icmd &= ~(SCI_ICMD_DATA|SCI_ICMD_SEL|SCI_ICMD_ATN);
	regs->sci_icmd = icmd;
lost:
	regs->sci_mode = 0;

	return ret;
}

sci_data_out(regs, phase, count, data)
	register volatile sci_regmap_t	*regs;
	unsigned char		*data;
{
	register unsigned char	icmd;
	register int		cnt=0;

	/* ..checks.. */

	icmd = regs->sci_icmd & ~(SCI_ICMD_DIFF|SCI_ICMD_TEST);
loop:
	if (SCI_CUR_PHASE(regs->sci_bus_csr) != phase)
		return cnt;

	WAIT_FOR_REQ(regs);
	icmd |= SCI_ICMD_DATA;
	regs->sci_icmd = icmd;
	regs->sci_odata = *data++;
	icmd |= SCI_ICMD_ACK;
	regs->sci_icmd = icmd;

	icmd &= ~(SCI_ICMD_DATA|SCI_ICMD_ACK);
	WAIT_FOR_NOT_REQ(regs);
	regs->sci_icmd = icmd;
	++cnt;
	if (--count > 0)
		goto loop;
scsi_timeout_error:
	return cnt;
}

sci_data_in(regs, phase, count, data)
	register volatile sci_regmap_t	*regs;
	unsigned char		*data;
{
	register unsigned char	icmd;
	register int		cnt=0;

	/* ..checks.. */

	icmd = regs->sci_icmd & ~(SCI_ICMD_DIFF|SCI_ICMD_TEST);

loop:
	if (SCI_CUR_PHASE(regs->sci_bus_csr) != phase)
		return cnt;

	WAIT_FOR_REQ(regs);
	*data++ = regs->sci_data;
	icmd |= SCI_ICMD_ACK;
	regs->sci_icmd = icmd;

	icmd &= ~SCI_ICMD_ACK;
	WAIT_FOR_NOT_REQ(regs);
	regs->sci_icmd = icmd;
	++cnt;
	if (--count > 0)
		goto loop;

scsi_timeout_error:
	return cnt;
}

static int
si_command_transfer(register volatile sci_regmap_t *regs,
		 int maxlen, u_char *data, u_char *status, u_char *msg)
{
	int	xfer=0, phase;

/*	printf("command_transfer called for 0x%x.\n", *data); */

	regs->sci_icmd = 0;

	while (1) {

		WAIT_FOR_REQ(regs);

		phase = SCI_CUR_PHASE(regs->sci_bus_csr);

		switch (phase) {
			case SCSI_PHASE_CMD:
				SCI_ACK(regs,SCSI_PHASE_CMD);
				xfer += sci_data_out(regs, SCSI_PHASE_CMD,
						   	maxlen, data);
				return xfer;
			case SCSI_PHASE_DATA_IN:
				printf("Data in phase in command_transfer?\n");
				return 0;
			case SCSI_PHASE_DATA_OUT:
				printf("Data out phase in command_transfer?\n");
				return 0;
			case SCSI_PHASE_STATUS:
				SCI_ACK(regs,SCSI_PHASE_STATUS);
				printf("status in command_transfer.\n");
				sci_data_in(regs, SCSI_PHASE_STATUS,
					  	1, status);
				break;
			case SCSI_PHASE_MESSAGE_IN:
				SCI_ACK(regs,SCSI_PHASE_MESSAGE_IN);
				printf("msgin in command_transfer.\n");
				sci_data_in(regs, SCSI_PHASE_MESSAGE_IN,
					  	1, msg);
				break;
			case SCSI_PHASE_MESSAGE_OUT:
				SCI_ACK(regs,SCSI_PHASE_MESSAGE_OUT);
				sci_data_out(regs, SCSI_PHASE_MESSAGE_OUT,
					  	1, msg);
				break;
			default:
				printf("Unexpected phase 0x%x in "
					"command_transfer().\n", phase);
scsi_timeout_error:
				return xfer;
				break;
		}
	}
}

static int
si_data_transfer(register volatile sci_regmap_t *regs,
	      int maxlen, u_char *data, u_char *status, u_char *msg)
{
	int	retlen = 0, xfer, phase;

	regs->sci_icmd = 0;

	*status = 0;

	while (1) {

		WAIT_FOR_REQ(regs);

		phase = SCI_CUR_PHASE(regs->sci_bus_csr);

		switch (phase) {
			case SCSI_PHASE_CMD:
				printf("Command phase in data_transfer().\n");
				return retlen;
			case SCSI_PHASE_DATA_IN:
				SCI_ACK(regs,SCSI_PHASE_DATA_IN);
#if PSEUDO_DMA
				xfer = sci_pdma_in(regs, SCSI_PHASE_DATA_IN,
						  	maxlen, data);
#else
				xfer = sci_data_in(regs, SCSI_PHASE_DATA_IN,
						  	maxlen, data);
#endif
				retlen += xfer;
				maxlen -= xfer;
				break;
			case SCSI_PHASE_DATA_OUT:
				SCI_ACK(regs,SCSI_PHASE_DATA_OUT);
#if PSEUDO_DMA
				xfer = sci_pdma_out(regs, SCSI_PHASE_DATA_OUT,
						   	maxlen, data);
#else
				xfer = sci_data_out(regs, SCSI_PHASE_DATA_OUT,
						   	maxlen, data);
#endif
				retlen += xfer;
				maxlen -= xfer;
				break;
			case SCSI_PHASE_STATUS:
				SCI_ACK(regs,SCSI_PHASE_STATUS);
				sci_data_in(regs, SCSI_PHASE_STATUS,
					  	1, status);
				break;
			case SCSI_PHASE_MESSAGE_IN:
				SCI_ACK(regs,SCSI_PHASE_MESSAGE_IN);
				sci_data_in(regs, SCSI_PHASE_MESSAGE_IN,
					  	1, msg);
				if (*msg == 0) {
					return retlen;
				} else {
					printf( "message 0x%x in "
						"data_transfer.\n", *msg);
				}
				break;
			case SCSI_PHASE_MESSAGE_OUT:
				SCI_ACK(regs,SCSI_PHASE_MESSAGE_OUT);
				sci_data_out(regs, SCSI_PHASE_MESSAGE_OUT,
					  	1, msg);
				break;
			default:
				printf( "Unexpected phase 0x%x in "
					"data_transfer().\n", phase);
scsi_timeout_error:
				return retlen;
				break;
		}
	}
}

static int
si_dorequest(register volatile sci_regmap_t *regs,
		int target, int lun, u_char *cmd, int cmdlen,
		char *databuf, int datalen, int *sent, int *ret)
{
/* Returns 0 on success, -1 on internal error, or the status byte */
	int	cmd_bytes_sent, r;
	u_char	stat, msg, c;

	*sent = 0;

	if ( ( r = si_select_target(regs, 7, target, 1) ) != SCSI_RET_SUCCESS) {
		*ret = r;
		SCI_CLR_INTR(regs);
		switch (r) {
		case SCSI_RET_RETRY:
			return 0x08;
		default:
			printf("si_select_target(target %d, lun %d) failed(%d).\n",
				target, lun, r);
		case SCSI_RET_DEVICE_DOWN:
			return -1;
		}
	}

	c = 0x80 | lun;

	if ((cmd_bytes_sent = si_command_transfer(regs, cmdlen,
				(u_char *) cmd, &stat, &c))
	     != cmdlen) {
		SCI_CLR_INTR(regs);
		*ret = SCSI_RET_COMMAND_FAIL;
		printf("Data underrun sending CCB (%d bytes of %d, sent).\n",
			cmd_bytes_sent, cmdlen);
		return -1;
	}

	*sent=si_data_transfer(regs, datalen, (u_char *)databuf,
				  &stat, &msg);

	*ret = 0;
	return stat;
}

static int
si_generic(int adapter, int id, int lun, struct scsi_generic *cmd,
  	 int cmdlen, void *databuf, int datalen)
{
	register struct ncr5380_softc *ncr5380 = sicd.cd_devs[adapter];
	register volatile sci_regmap_t *regs = ncr5380->sc_regs;
	int i,j,sent,ret;

	if (cmd->opcode == TEST_UNIT_READY)	/* XXX */
		cmd->bytes[0] = ((u_char) lun << 5);

	i = si_dorequest(regs, id, lun, (u_char *) cmd, cmdlen,
					 databuf, datalen, &sent, &ret);

	return i;
}

static int
si_group0(int adapter, int id, int lun, int opcode, int addr, int len,
		int flags, caddr_t databuf, int datalen)
{
	register struct ncr5380_softc *ncr5380 = sicd.cd_devs[adapter];
	register volatile sci_regmap_t *regs = ncr5380->sc_regs;
	unsigned char cmd[6];
	int i,j,sent,ret;

	cmd[0] = opcode;		/* Operation code           		*/
	cmd[1] = (lun << 5) | ((addr >> 16) & 0x1F);	/* Lun & MSB of addr	*/
	cmd[2] = (addr >> 8) & 0xFF;	/* addr					*/
	cmd[3] = addr & 0xFF;		/* LSB of addr				*/
	cmd[4] = len;			/* Allocation length			*/
	cmd[5] = flags;		/* Link/Flag				*/

	i = si_dorequest(regs, id, lun, cmd, 6, databuf, datalen, &sent, &ret);

	return i;
}
