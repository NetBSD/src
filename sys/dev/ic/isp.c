/*	$NetBSD: isp.c,v 1.9.2.1 1997/08/23 07:12:56 thorpej Exp $	*/

/*
 * Machine Independent (well, as best as possible)
 * code for the Qlogic ISP SCSI adapters.
 *
 * Specific probe attach and support routines for Qlogic ISP SCSI adapters.
 *
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Inspiration and ideas about this driver are from Erik Moe's Linux driver
 * (qlogicisp.c) and Dave Miller's SBus version of same (qlogicisp.c)
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>  
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h> 
#include <sys/proc.h>
#include <sys/user.h>


#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <scsi/scsi_message.h>
#include <scsi/scsi_debug.h>
#include <scsi/scsiconf.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/ic/ispmbox.h>

#define	MBOX_DELAY_COUNT	1000000 / 100

struct cfdriver isp_cd = {
	NULL, "isp", DV_DULL
};

static void	ispminphys __P((struct buf *));
static int32_t	ispscsicmd __P((struct scsi_xfer *xs));
static void	isp_mboxcmd __P((struct ispsoftc *, mbreg_t *));

static struct scsi_adapter isp_switch = {
	ispscsicmd, ispminphys, 0, 0
};

static struct scsi_device isp_dev = { NULL, NULL, NULL, NULL };

#define	IDPRINTF(lev, x)	if (isp->isp_dblev >= lev) printf x

static int isp_poll __P((struct ispsoftc *, struct scsi_xfer *, int));	
static int isp_parse_status
__P((struct ispsoftc *, ispstatusreq_t *, struct scsi_xfer *));
static void isp_lostcmd
__P((struct ispsoftc *, struct scsi_xfer *, ispreq_t *));
static void isp_fibre_init __P((struct ispsoftc *));
static void isp_fw_state __P((struct ispsoftc *));
static void isp_dumpregs __P((struct ispsoftc *, const char *));
static void isp_setdparm __P((struct ispsoftc *));

#define	WATCHI	(30 * hz)
static void isp_watch __P((void *));
/*
 * Reset Hardware.
 */
void
isp_reset(isp)
	struct ispsoftc *isp;
{
	mbreg_t mbs;
	int loops, i, cf_flags, dodnld = 1;
	char *revname;

	revname = "(unknown)";

	isp->isp_state = ISP_NILSTATE;
	cf_flags = isp->isp_dev.dv_cfdata->cf_flags; 

	/*
	 * Basic types have been set in the MD code.
	 * See if we can't figure out more here.
	 */
	if (isp->isp_type & ISP_HA_FC) {
		isp->isp_dblev = 2;
		revname = "2100";
	} else {
		isp->isp_dblev = 1;
		i = ISP_READ(isp, BIU_CONF0) & BIU_CONF0_HW_MASK;
		switch (i) {
		default:
			printf("%s: unknown ISP type %x\n", isp->isp_name, i);
			break;
		case 1:
			revname = "1020";
			isp->isp_type = ISP_HA_SCSI_1020;
			break;
		case 3:
			revname = "1040A";
			isp->isp_type = ISP_HA_SCSI_1040A;
			break;
		case 5:
			revname = "1040B";
			isp->isp_type = ISP_HA_SCSI_1040B;
			break;
		}
	}

	/*
	 * Do MD specific pre initialization
	 */
	ISP_RESET0(isp);
	isp_setdparm(isp);

	/*
	 * Hit the chip over the head with hammer,
	 * and give the ISP a chance to recover.
	 */

	if (isp->isp_type & ISP_HA_SCSI) {
		ISP_WRITE(isp, BIU_ICR, BIU_ICR_SOFT_RESET);
		/*
		 * A slight delay...
		 */
		delay(100);

		/*
		 * Clear data && control DMA engines.
		 */
		ISP_WRITE(isp, CDMA_CONTROL,
		      DMA_CNTRL_CLEAR_CHAN | DMA_CNTRL_RESET_INT);
		ISP_WRITE(isp, DDMA_CONTROL,
		      DMA_CNTRL_CLEAR_CHAN | DMA_CNTRL_RESET_INT);
	} else {
		ISP_WRITE(isp, BIU2100_CSR, BIU2100_SOFT_RESET);
		/*
		 * A slight delay...
		 */
		delay(100);
		ISP_WRITE(isp, CDMA2100_CONTROL,
			DMA_CNTRL2100_CLEAR_CHAN | DMA_CNTRL2100_RESET_INT);
		ISP_WRITE(isp, TDMA2100_CONTROL,
			DMA_CNTRL2100_CLEAR_CHAN | DMA_CNTRL2100_RESET_INT);
		ISP_WRITE(isp, RDMA2100_CONTROL,
			DMA_CNTRL2100_CLEAR_CHAN | DMA_CNTRL2100_RESET_INT);
	}

	/*
	 * Wait for ISP to be ready to go...
	 */
	loops = MBOX_DELAY_COUNT;
	for (;;) {
		if (isp->isp_type & ISP_HA_SCSI) {
			if (!(ISP_READ(isp, BIU_ICR) & BIU_ICR_SOFT_RESET))
				break;
		} else {
			if (!(ISP_READ(isp, BIU2100_CSR) & BIU2100_SOFT_RESET))
				break;
		}
		delay(100);
		if (--loops < 0) {
			isp_dumpregs(isp, "chip reset timed out");
			return;
		}
	}
	/*
	 * More initialization
	 */
	if (isp->isp_type & ISP_HA_SCSI) {      
		ISP_WRITE(isp, BIU_CONF1, 0);
	} else {
		ISP_WRITE(isp, BIU2100_CSR, 0);
		ISP_WRITE(isp, RISC_MTR2100, 0x1212);	/* FM */
	}

	ISP_WRITE(isp, HCCR, HCCR_CMD_RESET);
	delay(100);

	if (isp->isp_type & ISP_HA_SCSI) {
		ISP_SETBITS(isp, BIU_CONF1, isp->isp_mdvec->dv_conf1);
		if (isp->isp_mdvec->dv_conf1 & BIU_BURST_ENABLE) {
			ISP_SETBITS(isp, CDMA_CONF, DMA_ENABLE_BURST);
			ISP_SETBITS(isp, DDMA_CONF, DMA_ENABLE_BURST);
		}
	}
	ISP_WRITE(isp, HCCR, HCCR_CMD_RELEASE); /* release paused processor */

	/*
	 * Do MD specific post initialization
	 */
	ISP_RESET1(isp);

	/*
	 * Enable interrupts
	 */
	ENABLE_INTS(isp);

	/*
	 * Do some sanity checking.
	 */
	mbs.param[0] = MBOX_NO_OP;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		isp_dumpregs(isp, "NOP test failed");
		return;
	}

	if (isp->isp_type & ISP_HA_SCSI) {
		mbs.param[0] = MBOX_MAILBOX_REG_TEST;
		mbs.param[1] = 0xdead;
		mbs.param[2] = 0xbeef;
		mbs.param[3] = 0xffff;
		mbs.param[4] = 0x1111;
		mbs.param[5] = 0xa5a5;
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			isp_dumpregs(isp,
				"Mailbox Register test didn't complete");
			return;
		}
		if (mbs.param[1] != 0xdead || mbs.param[2] != 0xbeef ||
		    mbs.param[3] != 0xffff || mbs.param[4] != 0x1111 ||
		    mbs.param[5] != 0xa5a5) {
			isp_dumpregs(isp, "Register Test Failed");
			return;
		}

	}

	/*
	 * Download new Firmware, unless requested not to
	 * or not appropriate to do so.
	 */
	if ((isp->isp_fwrev >= isp->isp_mdvec->dv_fwrev) ||
	    (cf_flags & 0x80) != 0) {
		dodnld = 0;
	}

	if (dodnld) {
		for (i = 0; i < isp->isp_mdvec->dv_fwlen; i++) {
			mbs.param[0] = MBOX_WRITE_RAM_WORD;
			mbs.param[1] = isp->isp_mdvec->dv_codeorg + i;
			mbs.param[2] = isp->isp_mdvec->dv_ispfw[i];
			isp_mboxcmd(isp, &mbs);
			if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
				isp_dumpregs(isp, "f/w download failed");

				return;
			}
		}

		if (isp->isp_mdvec->dv_fwlen) {
			/*
			 * Verify that it downloaded correctly.
			 */
			mbs.param[0] = MBOX_VERIFY_CHECKSUM;
			mbs.param[1] = isp->isp_mdvec->dv_codeorg;
			isp_mboxcmd(isp, &mbs);
			if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
				isp_dumpregs(isp, "ram checksum failure");
				return;
			}
		}
	} else {
		IDPRINTF(2, ("%s: skipping f/w download\n", isp->isp_name));
	}

	/*
	 * Now start it rolling.
	 *
	 * If we didn't actually download f/w,
	 * we still need to (re)start it.
	 */

	mbs.param[0] = MBOX_EXEC_FIRMWARE;
	mbs.param[1] = isp->isp_mdvec->dv_codeorg;
	isp_mboxcmd(isp, &mbs);

	if (isp->isp_type & ISP_HA_SCSI) {
		/*
		 * Set CLOCK RATE
		 */
		if (((sdparam *)isp->isp_param)->isp_clock) {
			mbs.param[0] = MBOX_SET_CLOCK_RATE;
			mbs.param[1] = ((sdparam *)isp->isp_param)->isp_clock;
			isp_mboxcmd(isp, &mbs);
			if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
				isp_dumpregs(isp, "failed to set CLOCKRATE");
				return;
			}
		}
	}
	mbs.param[0] = MBOX_ABOUT_FIRMWARE;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		isp_dumpregs(isp, "ABOUT FIRMWARE command failed");
		return;
	}
	printf("%s: Board Revision %s, %s F/W Revision %d.%d\n",
		isp->isp_name, revname, dodnld? "loaded" : "ROM",
		mbs.param[1], mbs.param[2]);
	isp_fw_state(isp);
	isp->isp_state = ISP_RESETSTATE;
}

/*
 * Initialize Hardware to known state
 */
void
isp_init(isp)
	struct ispsoftc *isp;
{
	sdparam *sdp;
	mbreg_t mbs;
	int s, i, l;

	if (isp->isp_type & ISP_HA_FC) {
		isp_fibre_init(isp);
		return;
	}

	sdp = isp->isp_param;

	/*
	 * Try and figure out if we're connected to a differential bus.
	 * You have to pause the RISC processor to read SXP registers.
	 */
	s = splbio();
	ISP_WRITE(isp, HCCR, HCCR_CMD_PAUSE);
	if (ISP_READ(isp, SXP_PINS_DIFF) & SXP_PINS_DIFF_SENSE) {
		sdp->isp_diffmode = 1;
		printf("%s: Differential Mode\n", isp->isp_name);
	} else {
		/*
		 * Force pullups on.
		 */
		sdp->isp_req_ack_active_neg = 1;
		sdp->isp_data_line_active_neg = 1;
		sdp->isp_diffmode = 0;
	}
	ISP_WRITE(isp, HCCR, HCCR_CMD_RELEASE); /* release paused processor */

	mbs.param[0] = MBOX_GET_INIT_SCSI_ID;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to get initiator id");
		return;
	}
	if (mbs.param[1] != sdp->isp_initiator_id) {
		printf("%s: setting Initiator ID to %d\n", isp->isp_name,
			sdp->isp_initiator_id);
		mbs.param[0] = MBOX_SET_INIT_SCSI_ID;
		mbs.param[1] = sdp->isp_initiator_id;
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			(void) splx(s);
			isp_dumpregs(isp, "failed to set initiator id");
			return;
		}
	} else {
		IDPRINTF(2, ("%s: leaving Initiator ID at %d\n", isp->isp_name,
			sdp->isp_initiator_id));
	}

	mbs.param[0] = MBOX_SET_RETRY_COUNT;
	mbs.param[1] = sdp->isp_retry_count;
	mbs.param[2] = sdp->isp_retry_delay;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to set retry count and delay");
		return;
	}

	mbs.param[0] = MBOX_SET_ASYNC_DATA_SETUP_TIME;
	mbs.param[1] = sdp->isp_async_data_setup;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to set async data setup time");
		return;
	}

	mbs.param[0] = MBOX_SET_ACTIVE_NEG_STATE;
	mbs.param[1] =	(sdp->isp_req_ack_active_neg << 4) |
			(sdp->isp_data_line_active_neg << 5);
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to set active neg state");
		return;
	}

	mbs.param[0] = MBOX_SET_TAG_AGE_LIMIT;
	mbs.param[1] = sdp->isp_tag_aging;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to set tag age limit");
		return;
	}

	mbs.param[0] = MBOX_SET_SELECT_TIMEOUT;
	mbs.param[1] = sdp->isp_selection_timeout;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to set selection timeout");
		return;
	}

#ifdef	0
	printf("%s: device parameters, W=wide, S=sync, T=TagEnable\n",
		isp->isp_name);
#endif

	for (i = 0; i < MAX_TARGETS; i++) {
#ifdef	0
		char bz[8];

		if (sdp->isp_devparam[i].dev_flags & DPARM_SYNC) {
			u_int16_t cj = (sdp->isp_devparam[i].sync_offset << 8) |
					(sdp->isp_devparam[i].sync_period);
			if (cj == ISP_20M_SYNCPARMS) {
				cj = 20;
			} else if (ISP_10M_SYNCPARMS) {
				cj = 20;
			} else if (ISP_08M_SYNCPARMS) {
				cj = 20;
			} else if (ISP_05M_SYNCPARMS) {
				cj = 20;
			} else if (ISP_04M_SYNCPARMS) {
				cj = 20;
			} else {
				cj = 0;
			}
			if (sdp->isp_devparam[i].dev_flags & DPARM_WIDE)
				cj <<= 1;
			sprintf(bz, "%02dMBs", cj);
		} else {
			sprintf(bz, "Async");
		}
		if (sdp->isp_devparam[i].dev_flags & DPARM_WIDE)
			bz[5] = 'W';
		else
			bz[5] = ' ';
		if (sdp->isp_devparam[i].dev_flags & DPARM_TQING)
			bz[6] = 'T';
		else
			bz[6] = ' ';
		bz[7] = 0;
		printf(" Tgt%x:%s", i, bz);
		if (((i+1) & 0x3) == 0)
			printf("\n");
#endif
		if (sdp->isp_devparam[i].dev_enable == 0)
			continue;

		mbs.param[0] = MBOX_SET_TARGET_PARAMS;
		mbs.param[1] = i << 8;
		mbs.param[2] = sdp->isp_devparam[i].dev_flags << 8;
		mbs.param[3] =
			(sdp->isp_devparam[i].sync_offset << 8) |
			(sdp->isp_devparam[i].sync_period);

		IDPRINTF(5, ("%s: target %d flags %x offset %x period %x\n",
			     isp->isp_name, i, sdp->isp_devparam[i].dev_flags,
			     sdp->isp_devparam[i].sync_offset,
			     sdp->isp_devparam[i].sync_period));
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			printf("%s: failed to set parameters for target %d\n",
				isp->isp_name, i);
			printf("%s: flags %x offset %x period %x\n",
				isp->isp_name, sdp->isp_devparam[i].dev_flags,
				sdp->isp_devparam[i].sync_offset,
				sdp->isp_devparam[i].sync_period);
			mbs.param[0] = MBOX_SET_TARGET_PARAMS;
			mbs.param[1] = i << 8;
			mbs.param[2] = DPARM_DEFAULT << 8;
			mbs.param[3] = ISP_10M_SYNCPARMS;
			isp_mboxcmd(isp, &mbs);
			if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
				(void) splx(s);
				printf("%s: failed even to set defaults\n",
					isp->isp_name);
				return;
			}
		}
		for (l = 0; l < MAX_LUNS; l++) {
			mbs.param[0] = MBOX_SET_DEV_QUEUE_PARAMS;
			mbs.param[1] = (i << 8) | l;
			mbs.param[2] = sdp->isp_max_queue_depth;
			mbs.param[3] = sdp->isp_devparam[i].exc_throttle;
			isp_mboxcmd(isp, &mbs);
			if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
				(void) splx(s);
				isp_dumpregs(isp, "failed to set device queue "
				       "parameters");
				return;
			}
		}
	}

	/*
	 * Set up DMA for the request and result mailboxes.
	 */
	if (ISP_MBOXDMASETUP(isp)) {
		(void) splx(s);
		printf("%s: can't setup dma mailboxes\n", isp->isp_name);
		return;
	}
		
	mbs.param[0] = MBOX_INIT_RES_QUEUE;
	mbs.param[1] = RESULT_QUEUE_LEN(isp);
	mbs.param[2] = (u_int16_t) (isp->isp_result_dma >> 16);
	mbs.param[3] = (u_int16_t) (isp->isp_result_dma & 0xffff);
	mbs.param[4] = 0;
	mbs.param[5] = 0;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "set of response queue failed");
		return;
	}
	isp->isp_residx = 0;

	mbs.param[0] = MBOX_INIT_REQ_QUEUE;
	mbs.param[1] = RQUEST_QUEUE_LEN(isp);
	mbs.param[2] = (u_int16_t) (isp->isp_rquest_dma >> 16);
	mbs.param[3] = (u_int16_t) (isp->isp_rquest_dma & 0xffff);
	mbs.param[4] = 0;
	mbs.param[5] = 0;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "set of request queue failed");
		return;
	}
	isp->isp_reqidx = 0;

	/*	
	 * Unfortunately, this is the only way right now for
	 * forcing a sync renegotiation. If we boot off of
	 * an Alpha, it's put the chip in SYNC mode, but we
	 * haven't necessarily set up the parameters the
	 * same, so we'll have to yank the reset line to
	 * get everyone to renegotiate.
	 */

	mbs.param[0] = MBOX_BUS_RESET;
	mbs.param[1] = 2;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "SCSI bus reset failed");
	}
	/*
	 * This is really important to have set after a bus reset.
	 */
	isp->isp_sendmarker = 1;
	(void) splx(s);
	isp->isp_state = ISP_INITSTATE;
}

static void
isp_fibre_init(isp)
	struct ispsoftc *isp;
{
	fcparam *fcp;
	isp_icb_t *icbp;
	mbreg_t mbs;
	int s, count;

	fcp = isp->isp_param;

	fcp->isp_retry_count = 0;
	fcp->isp_retry_delay = 1;

	s = splbio();
	mbs.param[0] = MBOX_SET_RETRY_COUNT;
	mbs.param[1] = fcp->isp_retry_count;
	mbs.param[2] = fcp->isp_retry_delay;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "failed to set retry count and delay");
		return;
	}

	if (ISP_MBOXDMASETUP(isp)) {
		(void) splx(s);
		printf("%s: can't setup DMA for mailboxes\n", isp->isp_name);
		return;
	}

	icbp = (isp_icb_t *) fcp->isp_scratch;
	bzero(icbp, sizeof (*icbp));
#if 0
	icbp->icb_maxfrmlen = ICB_DFLT_FRMLEN;
	MAKE_NODE_NAME(isp, icbp);
	icbp->icb_rqstqlen = RQUEST_QUEUE_LEN(isp);
	icbp->icb_rsltqlen = RESULT_QUEUE_LEN(isp);
	icbp->icb_rqstaddr[0] = (u_int16_t) (isp->isp_rquest_dma & 0xffff);
	icbp->icb_rqstaddr[1] = (u_int16_t) (isp->isp_rquest_dma >> 16);
	icbp->icb_respaddr[0] = (u_int16_t) (isp->isp_result_dma & 0xffff);
	icbp->icb_respaddr[1] = (u_int16_t) (isp->isp_result_dma >> 16);
#endif
	icbp->icb_version = 1;
	icbp->icb_maxfrmlen = ICB_DFLT_FRMLEN;
	icbp->icb_maxalloc = 256;
	icbp->icb_execthrottle = 16;
	icbp->icb_retry_delay = 5;
	icbp->icb_retry_count = 0;
	MAKE_NODE_NAME(isp, icbp);
	icbp->icb_rqstqlen = RQUEST_QUEUE_LEN(isp);
	icbp->icb_rsltqlen = RESULT_QUEUE_LEN(isp);
	icbp->icb_rqstaddr[0] = (u_int16_t) (isp->isp_rquest_dma & 0xffff);
	icbp->icb_rqstaddr[1] = (u_int16_t) (isp->isp_rquest_dma >> 16);
	icbp->icb_respaddr[0] = (u_int16_t) (isp->isp_result_dma & 0xffff);
	icbp->icb_respaddr[1] = (u_int16_t) (isp->isp_result_dma >> 16);

	mbs.param[0] = MBOX_INIT_FIRMWARE;
	mbs.param[1] = 0;
	mbs.param[2] = (u_int16_t) (fcp->isp_scdma >> 16);
	mbs.param[3] = (u_int16_t) (fcp->isp_scdma & 0xffff);
	mbs.param[4] = 0;
	mbs.param[5] = 0;
	mbs.param[6] = 0;
	mbs.param[7] = 0;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		(void) splx(s);
		isp_dumpregs(isp, "INIT FIRMWARE failed");
		return;
	}
	isp->isp_reqidx = 0;
	isp->isp_residx = 0;

	/*
	 * Wait up to 3 seconds for FW to go to READY state.
	 *
	 * This is all very much not right. The problem here
	 * is that the cable may not be plugged in, or there
	 * may be many many members of the loop that haven't
	 * been logged into.
	 *
	 * This model of doing things doesn't support dynamic
	 * attachment, so we just plain lose (for now).
	 */
	for (count = 0; count < 3000; count++) {
		isp_fw_state(isp);
		if (fcp->isp_fwstate == FW_READY)
			break;
		delay(1000);		/* wait one millisecond */
	}

isp->isp_sendmarker = 1;

	(void) splx(s);
	isp->isp_state = ISP_INITSTATE;
}

/*
 * Complete attachment of Hardware, include subdevices.
 */
void
isp_attach(isp)
	struct ispsoftc *isp;
{
	/*
	 * Start the watchdog timer.
	 */
	timeout(isp_watch, isp, WATCHI);

	/*
	 * And complete initialization
	 */
	isp->isp_state = ISP_RUNSTATE;
	isp->isp_link.channel = SCSI_CHANNEL_ONLY_ONE;
	isp->isp_link.adapter_softc = isp;
	isp->isp_link.device = &isp_dev;
	isp->isp_link.adapter = &isp_switch;

	if (isp->isp_type & ISP_HA_FC) {
		fcparam *fcp = isp->isp_param;
		mbreg_t mbs;
		int s;

		isp->isp_link.max_target = MAX_FC_TARG-1;
#if	0
		isp->isp_link.openings = RQUEST_QUEUE_LEN(isp)/(MAX_FC_TARG-1);
#else
		isp->isp_link.openings = 8;
#endif
		s = splbio();
		mbs.param[0] = MBOX_GET_LOOP_ID;
		isp_mboxcmd(isp, &mbs);
		(void) splx(s);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			isp_dumpregs(isp, "GET LOOP ID failed");
			return;
		}
		fcp->isp_loopid = mbs.param[1];
		if (fcp->isp_loopid) {
			printf("%s: Loop ID 0x%x\n", isp->isp_name,
				fcp->isp_loopid);
		}
		isp->isp_link.adapter_target = 0xff;
	} else {
		isp->isp_link.openings = RQUEST_QUEUE_LEN(isp)/(MAX_TARGETS-1);
		isp->isp_link.max_target = MAX_TARGETS-1;
		isp->isp_link.adapter_target =
			((sdparam *)isp->isp_param)->isp_initiator_id;
	}
	if (isp->isp_link.openings < 2)
		isp->isp_link.openings = 2;
	config_found((void *)isp, &isp->isp_link, scsiprint);
}


/*
 * Free any associated resources prior to decommissioning.
 */
void
isp_uninit(isp)
	struct ispsoftc *isp;
{
	untimeout(isp_watch, isp);
}

/*
 * minphys our xfers
 *
 * Unfortunately, the buffer pointer describes the target device- not the
 * adapter device, so we can't use the pointer to find out what kind of
 * adapter we are and adjust accordingly.
 */

static void
ispminphys(bp)
	struct buf *bp;
{
	/*
	 * XX: Only the 1020 has a 24 bit limit.
	 */
	if (bp->b_bcount >= (1 << 24)) {
		bp->b_bcount = (1 << 24);
	}
	minphys(bp);
}

/*
 * start an xfer
 */
static int32_t
ispscsicmd(xs)
	struct scsi_xfer *xs;
{
	struct ispsoftc *isp;
	u_int8_t iptr, optr;
	union {
		ispreq_t *_reqp;
		ispreqt2_t *_t2reqp;
	} _u;
#define	reqp	_u._reqp
#define	t2reqp	_u._t2reqp
	int s, i;

	isp = xs->sc_link->adapter_softc;

	if (isp->isp_type & ISP_HA_FC) {
#if	0
		printf("%s: not doing FC now\n", isp->isp_name);
		xs->error = XS_SELTIMEOUT;
		xs->flags |= ITSDONE;
		return (COMPLETE);
#endif
		if (xs->cmdlen > 12) {
			printf("%s: unsupported cdb for fibre (%d)\n", 
				isp->isp_name, xs->cmdlen);
			xs->error = XS_DRIVER_STUFFUP;
			return (COMPLETE);
		}
if (isp->isp_type & ISP_HA_FC)
	DISABLE_INTS(isp);
	}
	optr = ISP_READ(isp, OUTMAILBOX4);
	iptr = isp->isp_reqidx;

	reqp = (ispreq_t *) ISP_QUEUE_ENTRY(isp->isp_rquest, iptr);
	iptr = (iptr + 1) & (RQUEST_QUEUE_LEN(isp) - 1);
	if (iptr == optr) {
		printf("%s: Request Queue Overflow\n", isp->isp_name);
		xs->error = XS_DRIVER_STUFFUP;
		return (TRY_AGAIN_LATER);
	}

	s = splbio();
if (isp->isp_type & ISP_HA_FC)
	DISABLE_INTS(isp);
	if (isp->isp_sendmarker) {
		ispmarkreq_t *marker = (ispmarkreq_t *) reqp;

		bzero((void *) marker, sizeof (*marker));
		marker->req_header.rqs_entry_count = 1;
		marker->req_header.rqs_entry_type = RQSTYPE_MARKER;
		marker->req_modifier = SYNC_ALL;

		isp->isp_sendmarker = 0;

		if (((iptr + 1) & (RQUEST_QUEUE_LEN(isp) - 1)) == optr) {
			ISP_WRITE(isp, INMAILBOX4, iptr);
			isp->isp_reqidx = iptr;
if (isp->isp_type & ISP_HA_FC)
	ENABLE_INTS(isp);
			(void) splx(s);
			printf("%s: Request Queue Overflow+\n", isp->isp_name);
			xs->error = XS_DRIVER_STUFFUP;
			return (TRY_AGAIN_LATER);
		}
		reqp = (ispreq_t *) ISP_QUEUE_ENTRY(isp->isp_rquest, iptr);
		iptr = (iptr + 1) & (RQUEST_QUEUE_LEN(isp) - 1);
	}

	bzero((void *) reqp, sizeof (_u));
	reqp->req_header.rqs_entry_count = 1;
	if (isp->isp_type & ISP_HA_FC) {
		reqp->req_header.rqs_entry_type = RQSTYPE_T2RQS;
	} else {
		reqp->req_header.rqs_entry_type = RQSTYPE_REQUEST;
	}
	reqp->req_header.rqs_flags = 0;
	reqp->req_header.rqs_seqno = isp->isp_seqno++;

	for (i = 0; i < RQUEST_QUEUE_LEN(isp); i++) {
		if (isp->isp_xflist[i] == NULL)
			break;
	}
	if (i == RQUEST_QUEUE_LEN(isp)) {
		panic("%s: ran out of xflist pointers\n", isp->isp_name);
		/* NOTREACHED */
	} else {
		/*
		 * Never have a handle that is zero, so
		 * set req_handle off by one.
		 */
		isp->isp_xflist[i] = xs;
		reqp->req_handle = i+1;
	}

	if (isp->isp_type & ISP_HA_FC) {
		/*
		 * See comment in isp_intr
		 */
		xs->resid = 0;
		/*
		 * The QL2100 always requires some kind of tag.
		 */
		if (xs->flags & SCSI_POLL) {
			t2reqp->req_flags = REQFLAG_STAG;
		} else {
			t2reqp->req_flags = REQFLAG_OTAG;
		}
	} else {
		if (xs->flags & SCSI_POLL) {
			reqp->req_flags = 0;
		} else {
			reqp->req_flags = REQFLAG_OTAG;
		}
	}
	reqp->req_lun_trn = xs->sc_link->lun;
	reqp->req_target = xs->sc_link->target;
	if (isp->isp_type & ISP_HA_SCSI) {
		reqp->req_cdblen = xs->cmdlen;
	}
	bcopy((void *)xs->cmd, reqp->req_cdb, xs->cmdlen);

	IDPRINTF(5, ("%s(%d.%d): START%d cmd 0x%x datalen %d\n", isp->isp_name,
		xs->sc_link->target, xs->sc_link->lun,
		reqp->req_header.rqs_seqno, *(u_char *) xs->cmd, xs->datalen));

	reqp->req_time = xs->timeout / 1000;
	if (reqp->req_time == 0 && xs->timeout)
		reqp->req_time = 1;
	if (ISP_DMASETUP(isp, xs, reqp, &iptr, optr)) {
if (isp->isp_type & ISP_HA_FC)
	ENABLE_INTS(isp);
		(void) splx(s);
		xs->error = XS_DRIVER_STUFFUP;
		return (COMPLETE);
	}
	xs->error = 0;
	ISP_WRITE(isp, INMAILBOX4, iptr);
	isp->isp_reqidx = iptr;
if (isp->isp_type & ISP_HA_FC)
	ENABLE_INTS(isp);
	(void) splx(s);
	if ((xs->flags & SCSI_POLL) == 0) {
		return (SUCCESSFULLY_QUEUED);
	}

	/*
	 * If we can't use interrupts, poll on completion.
	 */
	if (isp_poll(isp, xs, xs->timeout)) {
#if 0
		/* XXX try to abort it, or whatever */
		if (isp_poll(isp, xs, xs->timeout) {
			/* XXX really nuke it */
		}
#endif
		/*
		 * If no other error occurred but we didn't finish,
		 * something bad happened.
		 */
		if ((xs->flags & ITSDONE) == 0 && xs->error == XS_NOERROR) {
			isp_lostcmd(isp, xs, reqp);
			xs->error = XS_DRIVER_STUFFUP;
		}
	}
	return (COMPLETE);
#undef	reqp
#undef	t2reqp
}

/*
 * Interrupt Service Routine(s)
 */

int
isp_poll(isp, xs, mswait)
	struct ispsoftc *isp;
	struct scsi_xfer *xs;
	int mswait;
{

	while (mswait) {
		/* Try the interrupt handling routine */
		(void)isp_intr((void *)isp);

		/* See if the xs is now done */
		if (xs->flags & ITSDONE)
			return (0);
		delay(1000);		/* wait one millisecond */
		mswait--;
	}
	return (1);
}

int
isp_intr(arg)
	void *arg;
{
	struct scsi_xfer *xs;
	struct ispsoftc *isp = arg;
	u_int16_t iptr, optr, isr;

	isr = ISP_READ(isp, BIU_ISR);
	if (isp->isp_type & ISP_HA_FC) {
		if (isr == 0 || (isr & BIU2100_ISR_RISC_INT) == 0) {
			if (isr) {
				IDPRINTF(3, ("%s: isp_intr isr=%x\n",
					     isp->isp_name, isr));
			}
			return (0);
		}
	} else {
		if (isr == 0 || (isr & BIU_ISR_RISC_INT) == 0) {
			if (isr) {
				IDPRINTF(3, ("%s: isp_intr isr=%x\n",
					     isp->isp_name, isr));
			}
			return (0);
		}
	}

	optr = isp->isp_residx;
	if (ISP_READ(isp, BIU_SEMA) & 1) {
		u_int16_t mbox0 = ISP_READ(isp, OUTMAILBOX0);
		switch (mbox0) {
		case ASYNC_BUS_RESET:
		case ASYNC_TIMEOUT_RESET:
			printf("%s: bus or timeout reset\n", isp->isp_name);
			isp->isp_sendmarker = 1;
			break;
		case ASYNC_LIP_OCCURRED:
			printf("%s: LIP occurred\n", isp->isp_name);
			break;
		case ASYNC_LOOP_UP:
			printf("%s: Loop UP\n", isp->isp_name);
			break;
		case ASYNC_LOOP_DOWN:
			printf("%s: Loop DOWN\n", isp->isp_name);
			break;
		default:
			printf("%s: async %x\n", isp->isp_name, mbox0);
			break;
		}
		ISP_WRITE(isp, BIU_SEMA, 0);
	}

	ISP_WRITE(isp, HCCR, HCCR_CMD_CLEAR_RISC_INT);
	iptr = ISP_READ(isp, OUTMAILBOX5);
	if (optr == iptr) {
		IDPRINTF(3, ("why intr? isr %x iptr %x optr %x\n",
			isr, optr, iptr));
	}
	ENABLE_INTS(isp);

	while (optr != iptr) {
		ispstatusreq_t *sp;
		int buddaboom = 0;

		sp = (ispstatusreq_t *) ISP_QUEUE_ENTRY(isp->isp_result, optr);

		optr = (optr + 1) & (RESULT_QUEUE_LEN(isp)-1);
		if (sp->req_header.rqs_entry_type != RQSTYPE_RESPONSE) {
			printf("%s: not RESPONSE in RESPONSE Queue (0x%x)\n",
				isp->isp_name, sp->req_header.rqs_entry_type);
			if (sp->req_header.rqs_entry_type != RQSTYPE_REQUEST) {
				ISP_WRITE(isp, INMAILBOX5, optr);
				continue;
			}
			buddaboom = 1;
		}

		if (sp->req_header.rqs_flags & 0xf) {
			if (sp->req_header.rqs_flags & RQSFLAG_CONTINUATION) {
				ISP_WRITE(isp, INMAILBOX5, optr);
				continue;
			}
			printf("%s: rqs_flags=%x\n", isp->isp_name,
				sp->req_header.rqs_flags & 0xf);
		}
		if (sp->req_handle > RQUEST_QUEUE_LEN(isp) ||
		    sp->req_handle < 1) {
			printf("%s: bad request handle %d\n", isp->isp_name,
				sp->req_handle);
			ISP_WRITE(isp, INMAILBOX5, optr);
			continue;
		}
		xs = (struct scsi_xfer *) isp->isp_xflist[sp->req_handle - 1];
		if (xs == NULL) {
			printf("%s: NULL xs in xflist\n", isp->isp_name);
			ISP_WRITE(isp, INMAILBOX5, optr);
			continue;
		}
		isp->isp_xflist[sp->req_handle - 1] = NULL;
		if (sp->req_status_flags & RQSTF_BUS_RESET) {
			isp->isp_sendmarker = 1;
		}
		if (buddaboom) {
			xs->error = XS_DRIVER_STUFFUP;
		}
		xs->status = sp->req_scsi_status & 0xff;
		if (isp->isp_type & ISP_HA_SCSI) {
			if (sp->req_state_flags & RQSF_GOT_SENSE) {
				bcopy(sp->req_sense_data, &xs->sense,
					sizeof (xs->sense));
				xs->error = XS_SENSE;
			}
		} else {
			if (xs->status == SCSI_CHECK) {
				xs->error = XS_SENSE;
				bcopy(sp->req_sense_data, &xs->sense,
					sizeof (xs->sense));
				sp->req_state_flags |= RQSF_GOT_SENSE;
			}
		}
		if (xs->error == 0 && xs->status == SCSI_BUSY) {
			xs->error = XS_BUSY;
		}

		if (sp->req_header.rqs_entry_type == RQSTYPE_RESPONSE) {
			if (xs->error == 0 && sp->req_completion_status)
				xs->error = isp_parse_status(isp, sp, xs);
		} else {
			printf("%s: unknown return %x\n", isp->isp_name,
				sp->req_header.rqs_entry_type);
			if (xs->error == 0)
				xs->error = XS_DRIVER_STUFFUP;
		}
		if (isp->isp_type & ISP_HA_SCSI) {
			xs->resid = sp->req_resid;
		} else if (sp->req_scsi_status & RQCS_RU) {
			xs->resid = sp->req_resid;
			IDPRINTF(3, ("%s: cnt %d rsd %d\n", isp->isp_name,
				xs->datalen, sp->req_resid));
		}
		xs->flags |= ITSDONE;
		if (xs->datalen) {
			ISP_DMAFREE(isp, xs, sp->req_handle - 1);
		}
		if (isp->isp_dblev >= 5) {
			printf("%s(%d.%d): FINISH%d cmd 0x%x resid %d STS %x",
			       isp->isp_name, xs->sc_link->target,
			       xs->sc_link->lun, sp->req_header.rqs_seqno,
			       *(u_char *) xs->cmd, xs->resid, xs->status);
			if (sp->req_state_flags & RQSF_GOT_SENSE) {
				printf(" Skey: %x", xs->sense.flags);
				if (xs->error != XS_SENSE) {
					printf(" BUT NOT SET");
				}
			}
			printf(" xs->error %d\n", xs->error);
		}
		ISP_WRITE(isp, INMAILBOX5, optr);
		scsi_done(xs);
	}
	isp->isp_residx = optr;
	return (1);
}

/*
 * Support routines.
 */

static int
isp_parse_status(isp, sp, xs)
	struct ispsoftc *isp;
	ispstatusreq_t *sp;
	struct scsi_xfer *xs;
{
	switch (sp->req_completion_status) {
	case RQCS_COMPLETE:
		return (XS_NOERROR);
		break;

	case RQCS_INCOMPLETE:
		if ((sp->req_state_flags & RQSF_GOT_TARGET) == 0) {
			return (XS_SELTIMEOUT);
		}
		printf("%s: incomplete, state %x\n",
			isp->isp_name, sp->req_state_flags);
		break;
	case RQCS_DATA_OVERRUN:
		if (isp->isp_type & ISP_HA_FC) {
			xs->resid = sp->req_resid;
			break;
		}
		return (XS_NOERROR);

	case RQCS_DATA_UNDERRUN:
		if (isp->isp_type & ISP_HA_FC) {
			xs->resid = sp->req_resid;
			break;
		}
		return (XS_NOERROR);

	case RQCS_TIMEOUT:
		return (XS_TIMEOUT);

	case RQCS_RESET_OCCURRED:
		printf("%s: reset occurred\n", isp->isp_name);
		isp->isp_sendmarker = 1;
		break;

	case RQCS_ABORTED:
		printf("%s: command aborted\n", isp->isp_name);
		isp->isp_sendmarker = 1;
		break;

	case RQCS_PORT_UNAVAILABLE:
		/*
		 * No such port on the loop. Moral equivalent of SELTIMEO
		 */
		return (XS_SELTIMEOUT);

	case RQCS_PORT_LOGGED_OUT:
		printf("%s: port logout for target %d\n",
			isp->isp_name, xs->sc_link->target);
		break;

	case RQCS_PORT_CHANGED:
		printf("%s: port changed for target %d\n",
			isp->isp_name, xs->sc_link->target);
		break;

	case RQCS_PORT_BUSY:
		printf("%s: port busy for target %d\n",
			isp->isp_name, xs->sc_link->target);
		return (XS_BUSY);

	default:
		printf("%s: comp status %x\n", isp->isp_name,
		       sp->req_completion_status);
		break;
	}
	return (XS_DRIVER_STUFFUP);
}

#define	HINIB(x)			((x) >> 0x4)
#define	LONIB(x)			((x)  & 0xf)
#define MAKNIB(a, b)			(((a) << 4) | (b))
static u_int8_t mbpcnt[] = {
	MAKNIB(1, 1),	/* 0x00: MBOX_NO_OP */
	MAKNIB(5, 5),	/* 0x01: MBOX_LOAD_RAM */
	MAKNIB(2, 0),	/* 0x02: MBOX_EXEC_FIRMWARE */
	MAKNIB(5, 5),	/* 0x03: MBOX_DUMP_RAM */
	MAKNIB(3, 3),	/* 0x04: MBOX_WRITE_RAM_WORD */
	MAKNIB(2, 3),	/* 0x05: MBOX_READ_RAM_WORD */
	MAKNIB(6, 6),	/* 0x06: MBOX_MAILBOX_REG_TEST */
	MAKNIB(2, 3),	/* 0x07: MBOX_VERIFY_CHECKSUM	*/
	MAKNIB(1, 3),	/* 0x08: MBOX_ABOUT_FIRMWARE */
	MAKNIB(0, 0),	/* 0x09: */
	MAKNIB(0, 0),	/* 0x0a: */
	MAKNIB(0, 0),	/* 0x0b: */
	MAKNIB(0, 0),	/* 0x0c: */
	MAKNIB(0, 0),	/* 0x0d: */
	MAKNIB(1, 2),	/* 0x0e: MBOX_CHECK_FIRMWARE */
	MAKNIB(0, 0),	/* 0x0f: */
	MAKNIB(5, 5),	/* 0x10: MBOX_INIT_REQ_QUEUE */
	MAKNIB(6, 6),	/* 0x11: MBOX_INIT_RES_QUEUE */
	MAKNIB(4, 4),	/* 0x12: MBOX_EXECUTE_IOCB */
	MAKNIB(2, 2),	/* 0x13: MBOX_WAKE_UP	*/
	MAKNIB(1, 6),	/* 0x14: MBOX_STOP_FIRMWARE */
	MAKNIB(4, 4),	/* 0x15: MBOX_ABORT */
	MAKNIB(2, 2),	/* 0x16: MBOX_ABORT_DEVICE */
	MAKNIB(3, 3),	/* 0x17: MBOX_ABORT_TARGET */
	MAKNIB(2, 2),	/* 0x18: MBOX_BUS_RESET */
	MAKNIB(2, 3),	/* 0x19: MBOX_STOP_QUEUE */
	MAKNIB(2, 3),	/* 0x1a: MBOX_START_QUEUE */
	MAKNIB(2, 3),	/* 0x1b: MBOX_SINGLE_STEP_QUEUE */
	MAKNIB(2, 3),	/* 0x1c: MBOX_ABORT_QUEUE */
	MAKNIB(2, 4),	/* 0x1d: MBOX_GET_DEV_QUEUE_STATUS */
	MAKNIB(0, 0),	/* 0x1e: */
	MAKNIB(1, 3),	/* 0x1f: MBOX_GET_FIRMWARE_STATUS */
	MAKNIB(1, 2),	/* 0x20: MBOX_GET_INIT_SCSI_ID */
	MAKNIB(1, 2),	/* 0x21: MBOX_GET_SELECT_TIMEOUT */
	MAKNIB(1, 3),	/* 0x22: MBOX_GET_RETRY_COUNT	*/
	MAKNIB(1, 2),	/* 0x23: MBOX_GET_TAG_AGE_LIMIT */
	MAKNIB(1, 2),	/* 0x24: MBOX_GET_CLOCK_RATE */
	MAKNIB(1, 2),	/* 0x25: MBOX_GET_ACT_NEG_STATE */
	MAKNIB(1, 2),	/* 0x26: MBOX_GET_ASYNC_DATA_SETUP_TIME */
	MAKNIB(1, 3),	/* 0x27: MBOX_GET_PCI_PARAMS */
	MAKNIB(2, 4),	/* 0x28: MBOX_GET_TARGET_PARAMS */
	MAKNIB(2, 4),	/* 0x29: MBOX_GET_DEV_QUEUE_PARAMS */
	MAKNIB(0, 0),	/* 0x2a: */
	MAKNIB(0, 0),	/* 0x2b: */
	MAKNIB(0, 0),	/* 0x2c: */
	MAKNIB(0, 0),	/* 0x2d: */
	MAKNIB(0, 0),	/* 0x2e: */
	MAKNIB(0, 0),	/* 0x2f: */
	MAKNIB(2, 2),	/* 0x30: MBOX_SET_INIT_SCSI_ID */
	MAKNIB(2, 2),	/* 0x31: MBOX_SET_SELECT_TIMEOUT */
	MAKNIB(3, 3),	/* 0x32: MBOX_SET_RETRY_COUNT	*/
	MAKNIB(2, 2),	/* 0x33: MBOX_SET_TAG_AGE_LIMIT */
	MAKNIB(2, 2),	/* 0x34: MBOX_SET_CLOCK_RATE */
	MAKNIB(2, 2),	/* 0x35: MBOX_SET_ACTIVE_NEG_STATE */
	MAKNIB(2, 2),	/* 0x36: MBOX_SET_ASYNC_DATA_SETUP_TIME */
	MAKNIB(3, 3),	/* 0x37: MBOX_SET_PCI_CONTROL_PARAMS */
	MAKNIB(4, 4),	/* 0x38: MBOX_SET_TARGET_PARAMS */
	MAKNIB(4, 4),	/* 0x39: MBOX_SET_DEV_QUEUE_PARAMS */
	MAKNIB(0, 0),	/* 0x3a: */
	MAKNIB(0, 0),	/* 0x3b: */
	MAKNIB(0, 0),	/* 0x3c: */
	MAKNIB(0, 0),	/* 0x3d: */
	MAKNIB(0, 0),	/* 0x3e: */
	MAKNIB(0, 0),	/* 0x3f: */
	MAKNIB(1, 2),	/* 0x40: MBOX_RETURN_BIOS_BLOCK_ADDR */
	MAKNIB(6, 1),	/* 0x41: MBOX_WRITE_FOUR_RAM_WORDS */
	MAKNIB(2, 3),	/* 0x42: MBOX_EXEC_BIOS_IOCB */
	MAKNIB(0, 0),	/* 0x43: */
	MAKNIB(0, 0),	/* 0x44: */
	MAKNIB(0, 0),	/* 0x45: */
	MAKNIB(0, 0),	/* 0x46: */
	MAKNIB(0, 0),	/* 0x47: */
	MAKNIB(0, 0),	/* 0x48: */
	MAKNIB(0, 0),	/* 0x49: */
	MAKNIB(0, 0),	/* 0x4a: */
	MAKNIB(0, 0),	/* 0x4b: */
	MAKNIB(0, 0),	/* 0x4c: */
	MAKNIB(0, 0),	/* 0x4d: */
	MAKNIB(0, 0),	/* 0x4e: */
	MAKNIB(0, 0),	/* 0x4f: */
	MAKNIB(0, 0),	/* 0x50: */
	MAKNIB(0, 0),	/* 0x51: */
	MAKNIB(0, 0),	/* 0x52: */
	MAKNIB(0, 0),	/* 0x53: */
	MAKNIB(8, 0),	/* 0x54: MBOX_EXEC_COMMAND_IOCB_A64 */
	MAKNIB(0, 0),	/* 0x55: */
	MAKNIB(0, 0),	/* 0x56: */
	MAKNIB(0, 0),	/* 0x57: */
	MAKNIB(0, 0),	/* 0x58: */
	MAKNIB(0, 0),	/* 0x59: */
	MAKNIB(0, 0),	/* 0x5a: */
	MAKNIB(0, 0),	/* 0x5b: */
	MAKNIB(0, 0),	/* 0x5c: */
	MAKNIB(0, 0),	/* 0x5d: */
	MAKNIB(0, 0),	/* 0x5e: */
	MAKNIB(0, 0),	/* 0x5f: */
	MAKNIB(8, 6),	/* 0x60: MBOX_INIT_FIRMWARE */
	MAKNIB(0, 0),	/* 0x60: MBOX_GET_INIT_CONTROL_BLOCK  (FORMAT?) */
	MAKNIB(2, 1),	/* 0x62: MBOX_INIT_LIP */
	MAKNIB(8, 1),	/* 0x63: MBOX_GET_FC_AL_POSITION_MAP */
	MAKNIB(8, 1),	/* 0x64: MBOX_GET_PORT_DB */
	MAKNIB(3, 1),	/* 0x65: MBOX_CLEAR_ACA */
	MAKNIB(3, 1),	/* 0x66: MBOX_TARGET_RESET */
	MAKNIB(3, 1),	/* 0x67: MBOX_CLEAR_TASK_SET */
	MAKNIB(3, 1),	/* 0x69: MBOX_ABORT_TASK_SET */
	MAKNIB(1, 2),	/* 0x69: MBOX_GET_FIRMWARE_STATE */
	MAKNIB(0, 0),	/* 0x6a: */
	MAKNIB(0, 0),	/* 0x6b: */
	MAKNIB(0, 0),	/* 0x6c: */
	MAKNIB(0, 0),	/* 0x6d: */
	MAKNIB(0, 0),	/* 0x6e: */
	MAKNIB(0, 0),	/* 0x6f: */
	MAKNIB(0, 0),	/* 0x70: */
	MAKNIB(0, 0),	/* 0x71: */
	MAKNIB(0, 0),	/* 0x72: */
	MAKNIB(0, 0),	/* 0x73: */
	MAKNIB(0, 0),	/* 0x74: */
	MAKNIB(0, 0),	/* 0x75: */
	MAKNIB(0, 0),	/* 0x76: */
	MAKNIB(0, 0),	/* 0x77: */
	MAKNIB(0, 0),	/* 0x78: */
	MAKNIB(0, 0),	/* 0x79: */
	MAKNIB(0, 0),	/* 0x7a: */
	MAKNIB(0, 0),	/* 0x7b: */
	MAKNIB(0, 0),	/* 0x7c: */
	MAKNIB(0, 0),	/* 0x7d: */
	MAKNIB(0, 0),	/* 0x7e: */
	MAKNIB(0, 0),	/* 0x7f: */
	MAKNIB(8, 6),	/* 0x80: MBOX_INIT_FIRMWARE */
	MAKNIB(2, 1),	/* 0x81: MBOX_INIT_LIP */
	MAKNIB(8, 1),	/* 0x82: MBOX_GET_FC_AL_POSITION_MAP */
	MAKNIB(8, 1),	/* 0x83: MBOX_GET_PORT_DB */
	MAKNIB(2, 1),	/* 0x84: MBOX_QCTRL */
	MAKNIB(2, 1),	/* 0x85: MBOX_PCTRL */
	MAKNIB(3, 1),	/* 0x86: MBOX_CLEAR_ACA */
	MAKNIB(3, 1),	/* 0x87: MBOX_TARGET_RESET */
	MAKNIB(3, 1),	/* 0x88: MBOX_CLEAR_TASK_SET */
	MAKNIB(3, 1)	/* 0x89: MBOX_ABORT_TASK_SET */
};
#define	NMBCOM	(sizeof (mbpcnt) / sizeof (mbpcnt[0]))

static void
isp_mboxcmd(isp, mbp)
	struct ispsoftc *isp;
	mbreg_t *mbp;
{
	int outparam, inparam;
	int loops;
	u_int8_t opcode;

	if (mbp->param[0] == ISP2100_SET_PCI_PARAM) {
		opcode = mbp->param[0] = MBOX_SET_PCI_PARAMETERS;
		inparam = 4;
		outparam = 4;
		goto command_known;
	} else if (mbp->param[0] > NMBCOM) {
		printf("%s: bad command %x\n", isp->isp_name, mbp->param[0]);
		return;
	}

	opcode = mbp->param[0];
	inparam = HINIB(mbpcnt[mbp->param[0]]);
	outparam =  LONIB(mbpcnt[mbp->param[0]]);

	if (inparam == 0 && outparam == 0) {
		printf("%s: no parameters for %x\n", isp->isp_name,
			mbp->param[0]);
		return;
	}


command_known:
	/*
	 * Make sure we can send some words..
	 */

	loops = MBOX_DELAY_COUNT;
	while ((ISP_READ(isp, HCCR) & HCCR_HOST_INT) != 0) {
		delay(100);
		if (--loops < 0) {
			printf("%s: isp_mboxcmd timeout #1\n", isp->isp_name);
			return;
		}
	}

	/*
	 * Write input parameters
	 */
	switch (inparam) {
	case 8: ISP_WRITE(isp, INMAILBOX7, mbp->param[7]); mbp->param[7] = 0;
	case 7: ISP_WRITE(isp, INMAILBOX6, mbp->param[6]); mbp->param[6] = 0;
	case 6: ISP_WRITE(isp, INMAILBOX5, mbp->param[5]); mbp->param[5] = 0;
	case 5: ISP_WRITE(isp, INMAILBOX4, mbp->param[4]); mbp->param[4] = 0;
	case 4: ISP_WRITE(isp, INMAILBOX3, mbp->param[3]); mbp->param[3] = 0;
	case 3: ISP_WRITE(isp, INMAILBOX2, mbp->param[2]); mbp->param[2] = 0;
	case 2: ISP_WRITE(isp, INMAILBOX1, mbp->param[1]); mbp->param[1] = 0;
	case 1: ISP_WRITE(isp, INMAILBOX0, mbp->param[0]); mbp->param[0] = 0;
	}

	/*
	 * Clear semaphore on mailbox registers
	 */
	ISP_WRITE(isp, BIU_SEMA, 0);

	/*
	 * Clear RISC int condition.
	 */
	ISP_WRITE(isp, HCCR, HCCR_CMD_CLEAR_RISC_INT);

	/*
	 * Set Host Interrupt condition so that RISC will pick up mailbox regs.
	 */
	ISP_WRITE(isp, HCCR, HCCR_CMD_SET_HOST_INT);

	/*
	 * Wait until RISC int is set, except 2100
	 */
	if ((isp->isp_type & ISP_HA_FC) == 0) {
		loops = MBOX_DELAY_COUNT;
		while ((ISP_READ(isp, BIU_ISR) & BIU_ISR_RISC_INT) == 0) {
			delay(100);
			if (--loops < 0) {
				printf("%s: isp_mboxcmd timeout #2\n",
				    isp->isp_name);
				return;
			}
		}
	}

	/*
	 * Check to make sure that the semaphore has been set.
	 */
	loops = MBOX_DELAY_COUNT;
	while ((ISP_READ(isp, BIU_SEMA) & 1) == 0) {
		delay(100);
		if (--loops < 0) {
			printf("%s: isp_mboxcmd timeout #3\n", isp->isp_name);
			return;
		}
	}

	/*
	 * Make sure that the MBOX_BUSY has gone away
	 */
	loops = MBOX_DELAY_COUNT;
	while (ISP_READ(isp, OUTMAILBOX0) == MBOX_BUSY) {
		delay(100);
		if (--loops < 0) {
			printf("%s: isp_mboxcmd timeout #4\n", isp->isp_name);
			return;
		}
	}


	/*
	 * Pick up output parameters.
	 */
	switch (outparam) {
	case 8: mbp->param[7] = ISP_READ(isp, OUTMAILBOX7);
	case 7: mbp->param[6] = ISP_READ(isp, OUTMAILBOX6);
	case 6: mbp->param[5] = ISP_READ(isp, OUTMAILBOX5);
	case 5: mbp->param[4] = ISP_READ(isp, OUTMAILBOX4);
	case 4: mbp->param[3] = ISP_READ(isp, OUTMAILBOX3);
	case 3: mbp->param[2] = ISP_READ(isp, OUTMAILBOX2);
	case 2: mbp->param[1] = ISP_READ(isp, OUTMAILBOX1);
	case 1: mbp->param[0] = ISP_READ(isp, OUTMAILBOX0);
	}

	/*
	 * Clear RISC int.
	 */
	ISP_WRITE(isp, HCCR, HCCR_CMD_CLEAR_RISC_INT);

	/*
	 * Release semaphore on mailbox registers
	 */
	ISP_WRITE(isp, BIU_SEMA, 0);

	/*
	 * Just to be chatty here...
	 */
	switch(mbp->param[0]) {
	case MBOX_COMMAND_COMPLETE:
		break;
	case MBOX_INVALID_COMMAND:
		/*
		 * GET_CLOCK_RATE can fail a lot
		 * So can a couple of other commands.
		 */
		if (isp->isp_dblev > 1  && opcode != MBOX_GET_CLOCK_RATE) {
			printf("%s: mbox cmd %x failed with INVALID_COMMAND\n",
				isp->isp_name, opcode);
		}
		break;
	case MBOX_HOST_INTERFACE_ERROR:
		printf("%s: mbox cmd %x failed with HOST_INTERFACE_ERROR\n",
			isp->isp_name, opcode);
		break;
	case MBOX_TEST_FAILED:
		printf("%s: mbox cmd %x failed with TEST_FAILED\n",
			isp->isp_name, opcode);
		break;
	case MBOX_COMMAND_ERROR:
		printf("%s: mbox cmd %x failed with COMMAND_ERROR\n",
			isp->isp_name, opcode);
		break;
	case MBOX_COMMAND_PARAM_ERROR:
		printf("%s: mbox cmd %x failed with COMMAND_PARAM_ERROR\n",
			isp->isp_name, opcode);
		break;

	case ASYNC_LIP_OCCURRED:
		break;

	default:
		/*
		 * The expected return of EXEC_FIRMWARE is zero.
		 */
		if ((opcode == MBOX_EXEC_FIRMWARE && mbp->param[0] != 0) ||
		    (opcode != MBOX_EXEC_FIRMWARE)) {
			printf("%s: mbox cmd %x failed with error %x\n",
				isp->isp_name, opcode, mbp->param[0]);
		}
		break;
	}
}

static void
isp_lostcmd(struct ispsoftc *isp, struct scsi_xfer *xs, ispreq_t *req)
{
	mbreg_t mbs;

	mbs.param[0] = MBOX_GET_FIRMWARE_STATUS;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		isp_dumpregs(isp, "couldn't GET FIRMWARE STATUS");
		return;
	}
	if (mbs.param[1]) {
		printf("%s: %d commands on completion queue\n",
		       isp->isp_name, mbs.param[1]);
	}
	if (xs == NULL || xs->sc_link == NULL)
		return;

	mbs.param[0] = MBOX_GET_DEV_QUEUE_STATUS;
	mbs.param[1] = xs->sc_link->target << 8 | xs->sc_link->lun;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		isp_dumpregs(isp, "couldn't GET DEVICE QUEUE STATUS");
		return;
	}
	printf("%s: lost command for target %d lun %d, %d active of %d, "
		"Queue State: %x\n", isp->isp_name, xs->sc_link->target,
		xs->sc_link->lun, mbs.param[2], mbs.param[3], mbs.param[1]);

	isp_dumpregs(isp, "lost command");
	/*
	 * XXX: Need to try and do something to recover.
	 */
#if	0
	mbs.param[0] = MBOX_STOP_QUEUE;
	mbs.param[1] = xs->sc_link->target << 8 | xs->sc_link->lun;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) { 
		isp_dumpregs(isp, "couldn't stop device queue");
		return;
	}
	printf("%s: tgt %d lun %d, state %x\n", isp->isp_name,
		xs->sc_link->target, xs->sc_link->lun, mbs.param[2] & 0xff);

	/*
	 * If Queue Aborted, need to do a SendMarker
	 */
	if (mbs.param[1] & 0x1)
		isp->isp_sendmarker = 1;
	if (req == NULL)
		return;

	isp->isp_sendmarker = 1;

	mbs.param[0] = MBOX_ABORT;
	mbs.param[1] = (xs->sc_link->target << 8) | xs->sc_link->lun;
	mbs.param[2] = (req->req_handle - 1) >> 16;
	mbs.param[3] = (req->req_handle - 1) & 0xffff;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		printf("%s: couldn't abort command\n", isp->isp_name );
		mbs.param[0] = MBOX_ABORT_DEVICE;
		mbs.param[1] = (xs->sc_link->target << 8) | xs->sc_link->lun;
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			printf("%s: couldn't abort device\n", isp->isp_name );
		} else {
			if (isp_poll(isp, xs, xs->timeout)) {
				isp_lostcmd(isp, xs, NULL);
			}
		}
	} else {
		if (isp_poll(isp, xs, xs->timeout)) {
			isp_lostcmd(isp, xs, NULL);
		}
	}
	mbs.param[0] = MBOX_START_QUEUE;
	mbs.param[1] = xs->sc_link->target << 8 | xs->sc_link->lun;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) { 
		isp_dumpregs(isp, "couldn't start device queue");
	}
#endif
}

static void
isp_dumpregs(struct ispsoftc *isp, const char *msg)
{
	printf("%s: %s\n", isp->isp_name, msg);
	if (isp->isp_type & ISP_HA_SCSI)
		printf("\tbiu_conf1=%x", ISP_READ(isp, BIU_CONF1));
	else
		printf("\tbiu_csr=%x", ISP_READ(isp, BIU2100_CSR));
	printf(" biu_icr=%x biu_isr=%x biu_sema=%x ", ISP_READ(isp, BIU_ICR),
	       ISP_READ(isp, BIU_ISR), ISP_READ(isp, BIU_SEMA));
	printf("risc_hccr=%x\n", ISP_READ(isp, HCCR));

	if (isp->isp_type & ISP_HA_SCSI) {
		ISP_WRITE(isp, HCCR, HCCR_CMD_PAUSE);
		printf("\tcdma_conf=%x cdma_sts=%x cdma_fifostat=%x\n",
			ISP_READ(isp, CDMA_CONF), ISP_READ(isp, CDMA_STATUS),
			ISP_READ(isp, CDMA_FIFO_STS));
		printf("\tddma_conf=%x ddma_sts=%x ddma_fifostat=%x\n",
			ISP_READ(isp, DDMA_CONF), ISP_READ(isp, DDMA_STATUS),
			ISP_READ(isp, DDMA_FIFO_STS));
		printf("\tsxp_int=%x sxp_gross=%x sxp(scsi_ctrl)=%x\n",
			ISP_READ(isp, SXP_INTERRUPT),
			ISP_READ(isp, SXP_GROSS_ERR),
			ISP_READ(isp, SXP_PINS_CONTROL));
		ISP_WRITE(isp, HCCR, HCCR_CMD_RELEASE);
	}
	ISP_DUMPREGS(isp);
}

static void
isp_fw_state(struct ispsoftc *isp)
{
	mbreg_t mbs;
	if (isp->isp_type & ISP_HA_FC) {
		int once = 0;
		fcparam *fcp = isp->isp_param;
again:
		mbs.param[0] = MBOX_GET_FW_STATE;
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			if (mbs.param[0] == ASYNC_LIP_OCCURRED) {
				if (!once++) {
					goto again;
				}
			}
			isp_dumpregs(isp, "GET FIRMWARE STATE failed");
			return;
		}
		fcp->isp_fwstate = mbs.param[1];
	}
}

static void
isp_setdparm(struct ispsoftc *isp)
{
	int i;
	mbreg_t mbs;
	sdparam *sdp;

	isp->isp_fwrev = 0;
	if (isp->isp_type & ISP_HA_FC) {
		/*
		 * ROM in 2100 doesn't appear to support ABOUT_FIRMWARE
		 */
		return;
	}

	mbs.param[0] = MBOX_ABOUT_FIRMWARE;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		IDPRINTF(2, ("1st ABOUT FIRMWARE command failed"));
	} else {
		isp->isp_fwrev =
			(((u_int16_t) mbs.param[1]) << 10) + mbs.param[2];
	}


	sdp = (sdparam *) isp->isp_param;
	/*
	 * Try and get old clock rate out before we hit the
	 * chip over the head- but if and only if we don't
	 * know our desired clock rate.
	 */
	if (isp->isp_mdvec->dv_clock == 0) {
		mbs.param[0] = MBOX_GET_CLOCK_RATE;
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] == MBOX_COMMAND_COMPLETE) {
			sdp->isp_clock = mbs.param[1];
			printf("%s: using board clock 0x%x\n",
				isp->isp_name, sdp->isp_clock);
		}
	} else {
		sdp->isp_clock = isp->isp_mdvec->dv_clock;
	}

	mbs.param[0] = MBOX_GET_ACT_NEG_STATE;
	isp_mboxcmd(isp, &mbs);
	if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
		IDPRINTF(5, ("could not GET ACT NEG STATE"));
		sdp->isp_req_ack_active_neg = 1;
		sdp->isp_data_line_active_neg = 1;
	} else {
		sdp->isp_req_ack_active_neg = (mbs.param[1] >> 4) & 0x1;
		sdp->isp_data_line_active_neg = (mbs.param[1] >> 5) & 0x1;
	}
	for (i = 0; i < MAX_TARGETS; i++) {
		mbs.param[0] = MBOX_GET_TARGET_PARAMS;
		mbs.param[1] = i << 8;
		isp_mboxcmd(isp, &mbs);
		if (mbs.param[0] != MBOX_COMMAND_COMPLETE) {
			IDPRINTF(5, ("cannot get params for target %d", i));
			sdp->isp_devparam[i].sync_period =
				ISP_10M_SYNCPARMS & 0xff;
			sdp->isp_devparam[i].sync_offset =
				ISP_10M_SYNCPARMS >> 8;
			sdp->isp_devparam[i].dev_flags = DPARM_DEFAULT;
		} else {
#if	0
			printf("%s: target %d - flags 0x%x, sync %x\n",
			       isp->isp_name, i, mbs.param[2], mbs.param[3]);
#endif
			sdp->isp_devparam[i].dev_flags = mbs.param[2] >> 8;
			/*
			 * The maximum period we can really see
			 * here is 100 (decimal), or 400 ns.
			 * For some unknown reason we sometimes
			 * get back wildass numbers from the
			 * boot device's paramaters.
			 */
			if ((mbs.param[3] & 0xff) <= 0x64) {
				sdp->isp_devparam[i].sync_period = 
					mbs.param[3] & 0xff;
				sdp->isp_devparam[i].sync_offset =
					mbs.param[3] >> 8; 
			}
		}
	}

	/*
	 * Set Default Host Adapter Parameters
	 * XXX: Should try and get them out of NVRAM
	 */
	sdp->isp_adapter_enabled = 1;
	sdp->isp_cmd_dma_burst_enable = 1;
	sdp->isp_data_dma_burst_enabl = 1;
	sdp->isp_fifo_threshold = 2;
	sdp->isp_initiator_id = 7;
	sdp->isp_async_data_setup = 6;
	sdp->isp_selection_timeout = 250;
	sdp->isp_max_queue_depth = 256;
	sdp->isp_tag_aging = 8;
	sdp->isp_bus_reset_delay = 3;
	sdp->isp_retry_count = 0;
	sdp->isp_retry_delay = 1;

	for (i = 0; i < MAX_TARGETS; i++) {
		sdp->isp_devparam[i].exc_throttle = 16;
		sdp->isp_devparam[i].dev_enable = 1;
	}
}

static void
isp_phoenix(struct ispsoftc *isp)
{
	struct scsi_xfer *tlist[MAXISPREQUEST], *xs;
	int i;

	for (i = 0; i < RQUEST_QUEUE_LEN(isp); i++) {
		tlist[i] = (struct scsi_xfer *) isp->isp_xflist[i];
	}
	isp_reset(isp);
	isp_init(isp);
	isp->isp_state = ISP_RUNSTATE;

	for (i = 0; i < RQUEST_QUEUE_LEN(isp); i++) {
		xs = tlist[i];
		if (xs == NULL)
			continue;
		xs->resid = xs->datalen;
		xs->error = XS_DRIVER_STUFFUP;
		xs->flags |= ITSDONE;
		scsi_done(xs);
	}
}

static void
isp_watch(void *arg)
{
	int s, i;
	struct ispsoftc *isp = arg;
	struct scsi_xfer *xs;

	/*
	 * Look for completely dead commands (but not polled ones)
	 */
	s = splbio();
	for (i = 0; i < RQUEST_QUEUE_LEN(isp); i++) {
		if ((xs = (struct scsi_xfer *) isp->isp_xflist[i]) == NULL) {
			continue;
		}
		if (xs->flags & SCSI_POLL)
			continue;
		if (xs->timeout == 0) {
			continue;
		}
		xs->timeout -= (WATCHI * 1000);
		if (xs->timeout > -(2 * WATCHI * 1000)) {
			continue;
		}
		printf("%s: commands really timed out!\n", isp->isp_name);

		isp_phoenix(isp);
		break;
	}
	(void) splx(s);
	timeout(isp_watch, arg, WATCHI);
}
