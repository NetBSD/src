/* $NetBSD: isp_netbsd.c,v 1.10 1999/02/09 00:42:22 mjacob Exp $ */
/* release_02_05_99 */
/*
 * Platform (NetBSD) dependent common attachment code for Qlogic adapters.
 *
 *---------------------------------------
 * Copyright (c) 1997, 1998 by Matthew Jacob
 * NASA/Ames Research Center
 * All rights reserved.
 *---------------------------------------
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
 *
 * The author may be reached via electronic communications at
 *
 *  mjacob@nas.nasa.gov
 *  mjacob@feral.com
 *
 * or, via United States Postal Address
 *
 *  Matthew Jacob
 *  Feral Software
 *  2339 3rd Street
 *  Suite 24
 *  San Francisco, CA, 94107
 */

#include <dev/ic/isp_netbsd.h>

static void ispminphys __P((struct buf *));
static int32_t ispcmd __P((ISP_SCSI_XFER_T *));

static struct scsipi_device isp_dev = { NULL, NULL, NULL, NULL };
static int isp_poll __P((struct ispsoftc *, ISP_SCSI_XFER_T *, int));
static void isp_watch __P((void *));
static void isp_internal_restart __P((void *));

#define	FC_OPENINGS	RQUEST_QUEUE_LEN / (MAX_FC_TARG-1)
#define	PI_OPENINGS	RQUEST_QUEUE_LEN / (MAX_TARGETS-1)

/*
 * Complete attachment of hardware, include subdevices.
 */
void
isp_attach(isp)
	struct ispsoftc *isp;
{

	isp->isp_osinfo._adapter.scsipi_cmd = ispcmd;
	isp->isp_osinfo._adapter.scsipi_minphys = ispminphys;

	isp->isp_state = ISP_RUNSTATE;
	isp->isp_osinfo._link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	isp->isp_osinfo._link.adapter_softc = isp;
	isp->isp_osinfo._link.device = &isp_dev;
	isp->isp_osinfo._link.adapter = &isp->isp_osinfo._adapter;
	TAILQ_INIT(&isp->isp_osinfo.waitq);

	if (isp->isp_type & ISP_HA_FC) {
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_FC_TARG-1;
#ifdef	SCCLUN
		/*
		 * 16 bits worth, but let's be reasonable..
		 */
		isp->isp_osinfo._link.scsipi_scsi.max_lun = 255;
#else
		isp->isp_osinfo._link.scsipi_scsi.max_lun = 15;
#endif
		isp->isp_osinfo._link.openings = FC_OPENINGS;
		isp->isp_osinfo._link.scsipi_scsi.adapter_target =
			((fcparam *)isp->isp_param)->isp_loopid;
	} else {
		isp->isp_osinfo._link.openings = PI_OPENINGS;
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_TARGETS-1;
		if (isp->isp_bustype == ISP_BT_SBUS) {
			isp->isp_osinfo._link.scsipi_scsi.max_lun = 7;
		} else {
			/*
			 * Too much target breakage at present.
			 */
#if	0
			if (isp->isp_fwrev >= ISP_FW_REV(7,55))
				isp->isp_osinfo._link.scsipi_scsi.max_lun = 31;
			else
#endif
				isp->isp_osinfo._link.scsipi_scsi.max_lun = 7;
		}
		isp->isp_osinfo._link.scsipi_scsi.adapter_target =
			((sdparam *)isp->isp_param)->isp_initiator_id;
	}
	if (isp->isp_osinfo._link.openings < 2)
		isp->isp_osinfo._link.openings = 2;
	isp->isp_osinfo._link.type = BUS_SCSI;

	/*
	 * Send a SCSI Bus Reset (used to be done as part of attach,
	 * but now left to the OS outer layers).
	 *
	 * XXX: For now, skip resets for FC because the method by which
	 * XXX: we deal with loop down after issuing resets (which causes
	 * XXX: port logouts for all devices) needs interrupts to run so
	 * XXX: that async events happen.
	 */
	
	if (isp->isp_type & ISP_HA_SCSI)
		(void) isp_control(isp, ISPCTL_RESET_BUS, NULL);

	/*
	 * Start the watchdog.
	 */
	isp->isp_dogactive = 1;
	timeout(isp_watch, isp, 30 * hz);

	/*
	 * And attach children (if any).
	 */
	config_found((void *)isp, &isp->isp_osinfo._link, scsiprint);
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

static int
ispcmd(xs)
	ISP_SCSI_XFER_T *xs;
{
	struct ispsoftc *isp;
	int result;
	int s;

	isp = XS_ISP(xs);
	s = splbio();
	/*
	 * This is less efficient than I would like in that the
	 * majority of cases will have to do some pointer deferences
	 * to find out that things don't need to be updated.
	 */
	if ((xs->flags & SCSI_AUTOCONF) == 0 && (isp->isp_type & ISP_HA_SCSI)) {
		sdparam *sdp = isp->isp_param;
		if (sdp->isp_devparam[XS_TGT(xs)].dev_flags !=
		    sdp->isp_devparam[XS_TGT(xs)].cur_dflags) {
			u_int16_t f = DPARM_WIDE|DPARM_SYNC|DPARM_TQING;
			if (xs->sc_link->quirks & SDEV_NOSYNC)
				f &= ~DPARM_SYNC;
			if (xs->sc_link->quirks & SDEV_NOWIDE)
				f &= ~DPARM_WIDE;
			if (xs->sc_link->quirks & SDEV_NOTAG)
				f &= ~DPARM_TQING;
			sdp->isp_devparam[XS_TGT(xs)].dev_flags &=
				~(DPARM_WIDE|DPARM_SYNC|DPARM_TQING);
			sdp->isp_devparam[XS_TGT(xs)].dev_flags |= f;
			sdp->isp_devparam[XS_TGT(xs)].dev_update = 1;
			isp->isp_update = 1;
		}
	}
	/*
	 * Check for queue blockage...
	 */

	if (isp->isp_osinfo.blocked) {
		if (xs->flags & SCSI_POLL) {
			xs->error = XS_DRIVER_STUFFUP;
			splx(s);
			return (TRY_AGAIN_LATER);
		}
		TAILQ_INSERT_TAIL(&isp->isp_osinfo.waitq, xs, adapter_q);
		splx(s);
		return (CMD_QUEUED);
	}
	
	result = ispscsicmd(xs);
	if (result != CMD_QUEUED || (xs->flags & SCSI_POLL) == 0) {
		(void) splx(s);
		return (result);
	}

	/*
	 * If we can't use interrupts, poll on completion.
	 */
	if (isp_poll(isp, xs, XS_TIME(xs))) {
		/*
		 * If no other error occurred but we didn't finish,
		 * something bad happened.
		 */
		if (XS_IS_CMD_DONE(xs) == 0) {
			isp->isp_nactive--;
			if (isp->isp_nactive < 0)
				isp->isp_nactive = 0;
			if (XS_NOERR(xs)) {
				isp_lostcmd(isp, xs);
				XS_SETERR(xs, HBA_BOTCH);
			}
		}
	}
	(void) splx(s);
	return (CMD_COMPLETE);
}

static int
isp_poll(isp, xs, mswait)
	struct ispsoftc *isp;
	ISP_SCSI_XFER_T *xs;
	int mswait;
{

	while (mswait) {
		/* Try the interrupt handling routine */
		(void)isp_intr((void *)isp);

		/* See if the xs is now done */
		if (XS_IS_CMD_DONE(xs)) {
			return (0);
		}
		SYS_DELAY(1000);	/* wait one millisecond */
		mswait--;
	}
	return (1);
}

static void
isp_watch(arg)
	void *arg;
{
	int i;
	struct ispsoftc *isp = arg;
	ISP_SCSI_XFER_T *xs;
	ISP_ILOCKVAL_DECL;

	/*
	 * Look for completely dead commands (but not polled ones).
	 */
	ISP_ILOCK(isp);
	for (i = 0; i < RQUEST_QUEUE_LEN; i++) {
		if ((xs = (ISP_SCSI_XFER_T *) isp->isp_xflist[i]) == NULL) {
			continue;
		}
		if (XS_TIME(xs) == 0) {
			continue;
		}
		XS_TIME(xs) -= (WATCH_INTERVAL * 1000);
		/*
		 * Avoid later thinking that this
		 * transaction is not being timed.
		 * Then give ourselves to watchdog
		 * periods of grace.
		 */
		if (XS_TIME(xs) == 0) {
			XS_TIME(xs) = 1;
		} else if (XS_TIME(xs) > -(2 * WATCH_INTERVAL * 1000)) {
			continue;
		}
		if (isp_control(isp, ISPCTL_ABORT_CMD, xs)) {
			printf("%s: isp_watch failed to abort command\n",
			    isp->isp_name);
			isp_restart(isp);
			break;
		}
	}
	timeout(isp_watch, isp, WATCH_INTERVAL * hz);
	isp->isp_dogactive = 1;
	ISP_IUNLOCK(isp);
}

/*
 * Free any associated resources prior to decommissioning and
 * set the card to a known state (so it doesn't wake up and kick
 * us when we aren't expecting it to).
 *
 * Locks are held before coming here.
 */
void
isp_uninit(isp)
	struct ispsoftc *isp;
{
	ISP_ILOCKVAL_DECL;
	ISP_ILOCK(isp);
	/*
	 * Leave with interrupts disabled.
	 */
	DISABLE_INTS(isp);

	/*
	 * Turn off the watchdog (if active).
	 */
	if (isp->isp_dogactive) {
		untimeout(isp_watch, isp);
		isp->isp_dogactive = 0;
	}

	ISP_IUNLOCK(isp);
}

/*
 * Restart function after a LOOP UP event (e.g.),
 * done as a timeout for some hysteresis.
 */
static void
isp_internal_restart(arg)
	void *arg;
{
	struct ispsoftc *isp = arg;
	int result, nrestarted = 0, s = splbio();

	if (isp->isp_osinfo.blocked == 0) {
		struct scsipi_xfer *xs;
		while ((xs = TAILQ_FIRST(&isp->isp_osinfo.waitq)) != NULL) {
			TAILQ_REMOVE(&isp->isp_osinfo.waitq, xs, adapter_q);
			if ((result = ispscsicmd(xs)) != CMD_QUEUED) {
				printf("%s: botched command restart (0x%x)\n",
				    isp->isp_name, result);
				xs->flags |= ITSDONE;
				if (xs->error == XS_NOERROR)
					xs->error = XS_DRIVER_STUFFUP;
				scsipi_done(xs);
			}
			nrestarted++;
		}
		printf("%s: requeued %d commands\n", isp->isp_name, nrestarted);
	}
	(void) splx(s);
}
	
int
isp_async(isp, cmd, arg)
	struct ispsoftc *isp;
	ispasync_t cmd;
	void *arg;
{
	int s = splbio();
	switch (cmd) {
	case ISPASYNC_LOOP_DOWN:
		/*
		 * Hopefully we get here in time to minimize the number
		 * of commands we are firing off that are sure to die.
		 */
		isp->isp_osinfo.blocked = 1;
		break;
        case ISPASYNC_LOOP_UP:
		isp->isp_osinfo.blocked = 0;
		timeout(isp_internal_restart, isp, 1);
		break;
	case ISPASYNC_NEW_TGT_PARAMS:
		if (isp->isp_type & ISP_HA_SCSI) {
			sdparam *sdp = isp->isp_param;
			char *wt;
			int ns, flags, tgt;

			tgt = *((int *) arg);
	
			flags = sdp->isp_devparam[tgt].dev_flags;
			if (flags & DPARM_SYNC) {
				ns = sdp->isp_devparam[tgt].sync_period * 4;
			} else {
				ns = 0;
			}
			switch (flags & (DPARM_WIDE|DPARM_TQING)) {
			case DPARM_WIDE:
				wt = ", 16 bit wide\n";
				break;
			case DPARM_TQING:
				wt = ", Tagged Queueing Enabled\n";
				break;
			case DPARM_WIDE|DPARM_TQING:
				wt = ", 16 bit wide, Tagged Queueing Enabled\n";
				break;
			default:
				wt = "\n";
				break;
			}
			if (ns) {
				printf("%s: Target %d at %dMHz Max Offset %d%s",
				    isp->isp_name, tgt, 1000 / ns,
				    sdp->isp_devparam[tgt].sync_offset, wt);
			} else {
				printf("%s: Target %d Async Mode%s",
				    isp->isp_name, tgt, wt);
			}
		}
		break;
	default:
		break;
	}
	(void) splx(s);
	return (0);
}
