/* $NetBSD: isp_netbsd.c,v 1.18.2.16 2001/04/23 00:57:48 mjacob Exp $ */
/*
 * This driver, which is contained in NetBSD in the files:
 *
 *	sys/dev/ic/isp.c
 *	sys/dev/ic/isp_inline.h
 *	sys/dev/ic/isp_netbsd.c
 *	sys/dev/ic/isp_netbsd.h
 *	sys/dev/ic/isp_target.c
 *	sys/dev/ic/isp_target.h
 *	sys/dev/ic/isp_tpublic.h
 *	sys/dev/ic/ispmbox.h
 *	sys/dev/ic/ispreg.h
 *	sys/dev/ic/ispvar.h
 *	sys/microcode/isp/asm_sbus.h
 *	sys/microcode/isp/asm_1040.h
 *	sys/microcode/isp/asm_1080.h
 *	sys/microcode/isp/asm_12160.h
 *	sys/microcode/isp/asm_2100.h
 *	sys/microcode/isp/asm_2200.h
 *	sys/pci/isp_pci.c
 *	sys/sbus/isp_sbus.c
 *
 * Is being actively maintained by Matthew Jacob (mjacob@netbsd.org).
 * This driver also is shared source with FreeBSD, OpenBSD, Linux, Solaris,
 * Linux versions. This tends to be an interesting maintenance problem.
 *
 * Please coordinate with Matthew Jacob on changes you wish to make here.
 */
/*
 * Platform (NetBSD) dependent common attachment code for Qlogic adapters.
 * Matthew Jacob <mjacob@nas.nasa.gov>
 */
/*
 * Copyright (C) 1997, 1998, 1999 National Aeronautics & Space Administration
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

#include <dev/ic/isp_netbsd.h>
#include <sys/scsiio.h>


/*
 * Set a timeout for the watchdogging of a command.
 *
 * The dimensional analysis is
 *
 *	milliseconds * (seconds/millisecond) * (ticks/second) = ticks
 *
 *			=
 *
 *	(milliseconds / 1000) * hz = ticks
 *
 *
 * For timeouts less than 1 second, we'll get zero. Because of this, and
 * because we want to establish *our* timeout to be longer than what the
 * firmware might do, we just add 3 seconds at the back end.
 */
#define	_XT(xs)	((((xs)->timeout/1000) * hz) + (3 * hz))

static void ispminphys(struct buf *);
static void isprequest (struct scsipi_channel *, scsipi_adapter_req_t, void *);
static int
ispioctl(struct scsipi_channel *, u_long, caddr_t, int, struct proc *);

static void isp_polled_cmd(struct ispsoftc *, XS_T *);
static void isp_dog(void *);

/*
 * Complete attachment of hardware, include subdevices.
 */
void
isp_attach(struct ispsoftc *isp)
{
	isp->isp_state = ISP_RUNSTATE;

	isp->isp_osinfo._adapter.adapt_dev = &isp->isp_osinfo._dev;
	isp->isp_osinfo._adapter.adapt_nchannels = IS_DUALBUS(isp) ? 2 : 1;
	isp->isp_osinfo._adapter.adapt_openings = isp->isp_maxcmds;
	/*
	 * It's not stated whether max_periph is limited by SPI
	 * tag uage, but let's assume that it is.
	 */
	isp->isp_osinfo._adapter.adapt_max_periph = min(isp->isp_maxcmds, 255);
	isp->isp_osinfo._adapter.adapt_ioctl = ispioctl;
	isp->isp_osinfo._adapter.adapt_request = isprequest;
	isp->isp_osinfo._adapter.adapt_minphys = ispminphys;

	isp->isp_osinfo._chan.chan_adapter = &isp->isp_osinfo._adapter;
	isp->isp_osinfo._chan.chan_bustype = &scsi_bustype;
	isp->isp_osinfo._chan.chan_channel = 0;

	/*
	 * Until the midlayer is fixed to use REPORT LUNS, limit to 8 luns.
	 */
	isp->isp_osinfo._chan.chan_nluns = min(isp->isp_maxluns, 8);

	TAILQ_INIT(&isp->isp_osinfo.waitq);	/* The 2nd bus will share.. */

	if (IS_FC(isp)) {
		isp->isp_osinfo._chan.chan_ntargets = MAX_FC_TARG;
		isp->isp_osinfo._chan.chan_id = MAX_FC_TARG;
	} else {
		sdparam *sdp = isp->isp_param;
		isp->isp_osinfo._chan.chan_ntargets = MAX_TARGETS;
		isp->isp_osinfo._chan.chan_id = sdp->isp_initiator_id;
		isp->isp_osinfo.discovered[0] = 1 << sdp->isp_initiator_id;
		if (IS_DUALBUS(isp)) {
			isp->isp_osinfo._chan_b = isp->isp_osinfo._chan;
			sdp++;
			isp->isp_osinfo.discovered[1] =
			    1 << sdp->isp_initiator_id;
			isp->isp_osinfo._chan_b.chan_id = sdp->isp_initiator_id;
			isp->isp_osinfo._chan_b.chan_channel = 1;
		}
	}

	/*
	 * Send a SCSI Bus Reset.
	 */
	if (IS_SCSI(isp)) {
		int bus = 0;
		ISP_LOCK(isp);
		(void) isp_control(isp, ISPCTL_RESET_BUS, &bus);
		if (IS_DUALBUS(isp)) {
			bus++;
			(void) isp_control(isp, ISPCTL_RESET_BUS, &bus);
		}
		ISP_UNLOCK(isp);
	} else {
		ISP_LOCK(isp);
		isp_fc_runstate(isp, 2 * 1000000);
		ISP_UNLOCK(isp);
	}

	/*
	 * After this point, we'll be doing the new configuration
	 * schema which allows interrups, so we can do tsleep/wakeup
	 * for mailbox stuff at that point.
	 */
	isp->isp_osinfo.no_mbox_ints = 0;

	/*
	 * And attach children (if any).
	 */
	config_found((void *)isp, &isp->isp_osinfo._chan, scsiprint);
	if (IS_DUALBUS(isp)) {
		config_found((void *)isp, &isp->isp_osinfo._chan_b, scsiprint);
	}
}

/*
 * minphys our xfers
 *
 * Unfortunately, the buffer pointer describes the target device- not the
 * adapter device, so we can't use the pointer to find out what kind of
 * adapter we are and adjust accordingly.
 */

static void
ispminphys(struct buf *bp)
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
ispioctl(struct scsipi_channel *chan, u_long cmd, caddr_t addr, int flag,
	struct proc *p)
{
	struct ispsoftc *isp = (void *)chan->chan_adapter->adapt_dev;
	int s, retval = ENOTTY;
	
	switch (cmd) {
	case SCBUSIORESET:
		s = splbio();
		if (isp_control(isp, ISPCTL_RESET_BUS, &chan->chan_channel))
			retval = EIO;
		else
			retval = 0;
		(void) splx(s);
		break;
	case ISP_SDBLEV:
	{
		int olddblev = isp->isp_dblev;
		isp->isp_dblev = *(int *)addr;
		*(int *)addr = olddblev;
		retval = 0;
		break;
	}
	case ISP_RESETHBA:
		ISP_LOCK(isp);
		isp_reinit(isp);
		ISP_UNLOCK(isp);
		retval = 0;
		break;
	case ISP_FC_RESCAN:
		if (IS_FC(isp)) {
			ISP_LOCK(isp);
			if (isp_fc_runstate(isp, 5 * 1000000)) {
				retval = EIO;
			} else {
				retval = 0;
			}
			ISP_UNLOCK(isp);
		}
		break;
	case ISP_FC_LIP:
		if (IS_FC(isp)) {
			ISP_LOCK(isp);
			if (isp_control(isp, ISPCTL_SEND_LIP, 0)) {
				retval = EIO;
			} else {
				retval = 0;
			}
			ISP_UNLOCK(isp);
		}
		break;
	case ISP_FC_GETDINFO:
	{
		struct isp_fc_device *ifc = (struct isp_fc_device *) addr;
		struct lportdb *lp;

		if (ifc->loopid < 0 || ifc->loopid >= MAX_FC_TARG) {
			retval = EINVAL;
			break;
		}
		ISP_LOCK(isp);
		lp = &FCPARAM(isp)->portdb[ifc->loopid];
		if (lp->valid) {
			ifc->loopid = lp->loopid;
			ifc->portid = lp->portid;
			ifc->node_wwn = lp->node_wwn;
			ifc->port_wwn = lp->port_wwn;
			retval = 0;
		} else {
			retval = ENODEV;
		}
		ISP_UNLOCK(isp);
		break;
	}
	default:
		break;
	}
	return (retval);
}


static void
isprequest(struct scsipi_channel *chan, scsipi_adapter_req_t req, void *arg)
{
	struct scsipi_periph *periph;
	struct ispsoftc *isp = (void *)chan->chan_adapter->adapt_dev;
	XS_T *xs;
	int s, result;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		s = splbio();
		if (isp->isp_state < ISP_RUNSTATE) {
			DISABLE_INTS(isp);
			isp_init(isp);
			if (isp->isp_state != ISP_INITSTATE) {
				ENABLE_INTS(isp);
				(void) splx(s);
      			        XS_SETERR(xs, HBA_BOTCH);
				scsipi_done(xs);
				return;
			}
			isp->isp_state = ISP_RUNSTATE;
			ENABLE_INTS(isp);
		}

		if (xs->xs_control & XS_CTL_POLL) {
			volatile u_int8_t ombi = isp->isp_osinfo.no_mbox_ints;
			isp->isp_osinfo.no_mbox_ints = 1;
			isp_polled_cmd(isp, xs);
			isp->isp_osinfo.no_mbox_ints = ombi;
			(void) splx(s);
			return;
		}

		result = isp_start(xs);
		switch (result) {
		case CMD_QUEUED:
			if (xs->timeout) {
				callout_reset(&xs->xs_callout, _XT(xs),
				    isp_dog, xs);
			}
			break;
		case CMD_EAGAIN:
			xs->error = XS_REQUEUE;
			scsipi_done(xs);
			break;
		case CMD_RQLATER:
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			break;
		case CMD_COMPLETE:
			scsipi_done(xs);
			break;
		}
		(void) splx(s);
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
	if (IS_SCSI(isp)) {
		struct scsipi_xfer_mode *xm = arg;
		int dflags = 0;
		sdparam *sdp = SDPARAM(isp);

		sdp += chan->chan_channel;
		if (xm->xm_mode & PERIPH_CAP_TQING)
			dflags |= DPARM_TQING;
		if (xm->xm_mode & PERIPH_CAP_WIDE16)
			dflags |= DPARM_WIDE;
		if (xm->xm_mode & PERIPH_CAP_SYNC)
			dflags |= DPARM_SYNC;
		s = splbio();
		sdp->isp_devparam[xm->xm_target].dev_flags |= dflags;
		dflags = sdp->isp_devparam[xm->xm_target].dev_flags;
		sdp->isp_devparam[xm->xm_target].dev_update = 1;
		isp->isp_update |= (1 << chan->chan_channel);
		splx(s);
		isp_prt(isp, ISP_LOGDEBUG1,
		    "ispioctl: device flags 0x%x for %d.%d.X",
		    dflags, chan->chan_channel, xm->xm_target);
		break;
	}
	default:
		break;
	}
}

static void
isp_polled_cmd( struct ispsoftc *isp, XS_T *xs)
{
	int result;
	int infinite = 0, mswait;

	result = isp_start(xs);

	switch (result) {
	case CMD_QUEUED:
		break;
	case CMD_RQLATER:
	case CMD_EAGAIN:
		if (XS_NOERR(xs)) {
			xs->error = XS_REQUEUE;
		}
		/* FALLTHROUGH */
	case CMD_COMPLETE:
		scsipi_done(xs);
		return;
		
	}

	/*
	 * If we can't use interrupts, poll on completion.
	 */
	if ((mswait = XS_TIME(xs)) == 0)
		infinite = 1;

	while (mswait || infinite) {
		if (isp_intr((void *)isp)) {
			if (XS_CMD_DONE_P(xs)) {
				break;
			}
		}
		USEC_DELAY(1000);
		mswait -= 1;
	}

	/*
	 * If no other error occurred but we didn't finish,
	 * something bad happened.
	 */
	if (XS_CMD_DONE_P(xs) == 0) {
		if (isp_control(isp, ISPCTL_ABORT_CMD, xs)) {
			isp_reinit(isp);
		}
		if (XS_NOERR(xs)) {
			XS_SETERR(xs, HBA_BOTCH);
		}
	}
	scsipi_done(xs);
}

void
isp_done(XS_T *xs)
{
	XS_CMD_S_DONE(xs);
	if (XS_CMD_WDOG_P(xs) == 0) {
		struct ispsoftc *isp = XS_ISP(xs);
		callout_stop(&xs->xs_callout);
		if (XS_CMD_GRACE_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1,
			    "finished command on borrowed time");
		}
		XS_CMD_S_CLEAR(xs);
		scsipi_done(xs);
	}
}

static void
isp_dog(void *arg)
{
	XS_T *xs = arg;
	struct ispsoftc *isp = XS_ISP(xs);
	u_int16_t handle;

	ISP_ILOCK(isp);
	/*
	 * We've decided this command is dead. Make sure we're not trying
	 * to kill a command that's already dead by getting it's handle and
	 * and seeing whether it's still alive.
	 */
	handle = isp_find_handle(isp, xs);
	if (handle) {
		u_int16_t r, r1, i;

		if (XS_CMD_DONE_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1,
			    "watchdog found done cmd (handle 0x%x)", handle);
			ISP_IUNLOCK(isp);
			return;
		}

		if (XS_CMD_WDOG_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1,
			    "recursive watchdog (handle 0x%x)", handle);
			ISP_IUNLOCK(isp);
			return;
		}

		XS_CMD_S_WDOG(xs);

		i = 0;
		do {
			r = ISP_READ(isp, BIU_ISR);
			USEC_DELAY(1);
			r1 = ISP_READ(isp, BIU_ISR);
		} while (r != r1 && ++i < 1000);

		if (INT_PENDING(isp, r) && isp_intr(isp) && XS_CMD_DONE_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1, "watchdog cleanup (%x, %x)",
			    handle, r);
			XS_CMD_C_WDOG(xs);
			isp_done(xs);
		} else if (XS_CMD_GRACE_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1, "watchdog timeout (%x, %x)",
			    handle, r);
			/*
			 * Make sure the command is *really* dead before we
			 * release the handle (and DMA resources) for reuse.
			 */
			(void) isp_control(isp, ISPCTL_ABORT_CMD, arg);

			/*
			 * After this point, the comamnd is really dead.
			 */
			if (XS_XFRLEN(xs)) {
				ISP_DMAFREE(isp, xs, handle);
			}
			isp_destroy_handle(isp, handle);
			XS_SETERR(xs, XS_TIMEOUT);
			XS_CMD_S_CLEAR(xs);
			isp_done(xs);
		} else {
			u_int16_t iptr, optr;
			ispreq_t *mp;
			isp_prt(isp, ISP_LOGDEBUG2,
			    "possible command timeout (%x, %x)", handle, r);
			XS_CMD_C_WDOG(xs);
			callout_reset(&xs->xs_callout, hz, isp_dog, xs);
			if (isp_getrqentry(isp, &iptr, &optr, (void **) &mp)) {
				ISP_UNLOCK(isp);
				return;
			}
			XS_CMD_S_GRACE(xs);
			MEMZERO((void *) mp, sizeof (*mp));
			mp->req_header.rqs_entry_count = 1;
			mp->req_header.rqs_entry_type = RQSTYPE_MARKER;
			mp->req_modifier = SYNC_ALL;
			mp->req_target = XS_CHANNEL(xs) << 7;
			ISP_SWIZZLE_REQUEST(isp, mp);
			ISP_ADD_REQUEST(isp, iptr);
		}
	} else {
		isp_prt(isp, ISP_LOGDEBUG0, "watchdog with no command");
	}
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
isp_uninit(struct ispsoftc *isp)
{
	isp_lock(isp);
	/*
	 * Leave with interrupts disabled.
	 */
	DISABLE_INTS(isp);
	isp_unlock(isp);
}

int
isp_async(struct ispsoftc *isp, ispasync_t cmd, void *arg)
{
	int bus, tgt;
	int s = splbio();
	switch (cmd) {
	case ISPASYNC_NEW_TGT_PARAMS:
	if (IS_SCSI(isp) && isp->isp_dblev) {
		sdparam *sdp = isp->isp_param;
		int flags;
		struct scsipi_xfer_mode xm;

		tgt = *((int *) arg);
		bus = (tgt >> 16) & 0xffff;
		tgt &= 0xffff;
		sdp += bus;
		flags = sdp->isp_devparam[tgt].cur_dflags;

		xm.xm_mode = 0;
		xm.xm_period = sdp->isp_devparam[tgt].cur_period;
		xm.xm_offset = sdp->isp_devparam[tgt].cur_offset;
		xm.xm_target = tgt;

		if ((flags & DPARM_SYNC) && xm.xm_period && xm.xm_offset)
			xm.xm_mode |= PERIPH_CAP_SYNC;
		if (flags & DPARM_WIDE)
			xm.xm_mode |= PERIPH_CAP_WIDE16;
		if (flags & DPARM_TQING)
			xm.xm_mode |= PERIPH_CAP_TQING;
		scsipi_async_event(
		    bus ? (&isp->isp_osinfo._chan_b) : (&isp->isp_osinfo._chan),
		    ASYNC_EVENT_XFER_MODE,
		    &xm);
		break;
	}
	case ISPASYNC_BUS_RESET:
		if (arg)
			bus = *((int *) arg);
		else
			bus = 0;
		isp_prt(isp, ISP_LOGINFO, "SCSI bus %d reset detected", bus);
		break;
	case ISPASYNC_LOOP_DOWN:
		/*
		 * Hopefully we get here in time to minimize the number
		 * of commands we are firing off that are sure to die.
		 */
		scsipi_channel_freeze(&isp->isp_osinfo._chan, 1);
		if (IS_DUALBUS(isp))
			scsipi_channel_freeze(&isp->isp_osinfo._chan_b, 1);
		isp_prt(isp, ISP_LOGINFO, "Loop DOWN");
		break;
        case ISPASYNC_LOOP_UP:
		callout_reset(&isp->isp_osinfo._restart, 1,
		    scsipi_channel_timed_thaw, &isp->isp_osinfo._chan);
		if (IS_DUALBUS(isp)) {
			callout_reset(&isp->isp_osinfo._restart, 1,
			    scsipi_channel_timed_thaw,
			    &isp->isp_osinfo._chan_b);
		}
		isp_prt(isp, ISP_LOGINFO, "Loop UP");
		break;
	case ISPASYNC_PROMENADE:
	if (IS_FC(isp) && isp->isp_dblev) {
		const char fmt[] = "Target %d (Loop 0x%x) Port ID 0x%x "
		    "(role %s) %s\n Port WWN 0x%08x%08x\n Node WWN 0x%08x%08x";
		const static char *roles[4] = {
		    "No", "Target", "Initiator", "Target/Initiator"
		};
		fcparam *fcp = isp->isp_param;
		int tgt = *((int *) arg);
		struct lportdb *lp = &fcp->portdb[tgt]; 

		isp_prt(isp, ISP_LOGINFO, fmt, tgt, lp->loopid, lp->portid,
		    roles[lp->roles & 0x3],
		    (lp->valid)? "Arrived" : "Departed",
		    (u_int32_t) (lp->port_wwn >> 32),
		    (u_int32_t) (lp->port_wwn & 0xffffffffLL),
		    (u_int32_t) (lp->node_wwn >> 32),
		    (u_int32_t) (lp->node_wwn & 0xffffffffLL));
		break;
	}
	case ISPASYNC_CHANGE_NOTIFY:
		if (arg == (void *) 1) {
			isp_prt(isp, ISP_LOGINFO,
			    "Name Server Database Changed");
		} else {
			isp_prt(isp, ISP_LOGINFO,
			    "Name Server Database Changed");
		}
		break;
	case ISPASYNC_FABRIC_DEV:
	{
		int target, lrange;
		struct lportdb *lp = NULL;
		char *pt;
		sns_ganrsp_t *resp = (sns_ganrsp_t *) arg;
		u_int32_t portid;
		u_int64_t wwpn, wwnn;
		fcparam *fcp = isp->isp_param;

		portid =
		    (((u_int32_t) resp->snscb_port_id[0]) << 16) |
		    (((u_int32_t) resp->snscb_port_id[1]) << 8) |
		    (((u_int32_t) resp->snscb_port_id[2]));

		wwpn =
		    (((u_int64_t)resp->snscb_portname[0]) << 56) |
		    (((u_int64_t)resp->snscb_portname[1]) << 48) |
		    (((u_int64_t)resp->snscb_portname[2]) << 40) |
		    (((u_int64_t)resp->snscb_portname[3]) << 32) |
		    (((u_int64_t)resp->snscb_portname[4]) << 24) |
		    (((u_int64_t)resp->snscb_portname[5]) << 16) |
		    (((u_int64_t)resp->snscb_portname[6]) <<  8) |
		    (((u_int64_t)resp->snscb_portname[7]));

		wwnn =
		    (((u_int64_t)resp->snscb_nodename[0]) << 56) |
		    (((u_int64_t)resp->snscb_nodename[1]) << 48) |
		    (((u_int64_t)resp->snscb_nodename[2]) << 40) |
		    (((u_int64_t)resp->snscb_nodename[3]) << 32) |
		    (((u_int64_t)resp->snscb_nodename[4]) << 24) |
		    (((u_int64_t)resp->snscb_nodename[5]) << 16) |
		    (((u_int64_t)resp->snscb_nodename[6]) <<  8) |
		    (((u_int64_t)resp->snscb_nodename[7]));
		if (portid == 0 || wwpn == 0) {
			break;
		}

		switch (resp->snscb_port_type) {
		case 1:
			pt = "   N_Port";
			break;
		case 2:
			pt = "  NL_Port";
			break;
		case 3:
			pt = "F/NL_Port";
			break;
		case 0x7f:
			pt = "  Nx_Port";
			break;
		case 0x81:
			pt = "  F_port";
			break;
		case 0x82:
			pt = "  FL_Port";
			break;
		case 0x84:
			pt = "   E_port";
			break;
		default:
			pt = "?";
			break;
		}
		isp_prt(isp, ISP_LOGINFO,
		    "%s @ 0x%x, Node 0x%08x%08x Port %08x%08x",
		    pt, portid, ((u_int32_t) (wwnn >> 32)), ((u_int32_t) wwnn),
		    ((u_int32_t) (wwpn >> 32)), ((u_int32_t) wwpn));
		/*
		 * We're only interested in SCSI_FCP types (for now)
		 */
		if ((resp->snscb_fc4_types[2] & 1) == 0) {
			break;
		}
		if (fcp->isp_topo != TOPO_F_PORT)
			lrange = FC_SNS_ID+1;
		else
			lrange = 0;
		/*
		 * Is it already in our list?
		 */
		for (target = lrange; target < MAX_FC_TARG; target++) {
			if (target >= FL_PORT_ID && target <= FC_SNS_ID) {
				continue;
			}
			lp = &fcp->portdb[target];
			if (lp->port_wwn == wwpn && lp->node_wwn == wwnn) {
				lp->fabric_dev = 1;
				break;
			}
		}
		if (target < MAX_FC_TARG) {
			break;
		}
		for (target = lrange; target < MAX_FC_TARG; target++) {
			if (target >= FL_PORT_ID && target <= FC_SNS_ID) {
				continue;
			}
			lp = &fcp->portdb[target];
			if (lp->port_wwn == 0) {
				break;
			}
		}
		if (target == MAX_FC_TARG) {
			isp_prt(isp, ISP_LOGWARN,
			    "no more space for fabric devices");
			break;
		}
		lp->node_wwn = wwnn;
		lp->port_wwn = wwpn;
		lp->portid = portid;
		lp->fabric_dev = 1;
		break;
	}
	default:
		break;
	}
	(void) splx(s);
	return (0);
}

#include <machine/stdarg.h>
void
isp_prt(struct ispsoftc *isp, int level, const char *fmt, ...)
{
	va_list ap;
	if (level != ISP_LOGALL && (level & isp->isp_dblev) == 0) {
		return;
	}
	printf("%s: ", isp->isp_name);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}
