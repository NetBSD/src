/* $NetBSD: isp_netbsd.c,v 1.35 2000/12/28 08:11:52 mjacob Exp $ */
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

static void ispminphys __P((struct buf *));
static int32_t ispcmd __P((XS_T *));
static int
ispioctl __P((struct scsipi_link *, u_long, caddr_t, int, struct proc *));

static struct scsipi_device isp_dev = { NULL, NULL, NULL, NULL };
static int isp_polled_cmd __P((struct ispsoftc *, XS_T *));
static void isp_dog __P((void *));
static void isp_command_requeue __P((void *));
static void isp_internal_restart __P((void *));

/*
 * Complete attachment of hardware, include subdevices.
 */
void
isp_attach(isp)
	struct ispsoftc *isp;
{
	isp->isp_osinfo._adapter.scsipi_minphys = ispminphys;
	isp->isp_osinfo._adapter.scsipi_ioctl = ispioctl;
	isp->isp_osinfo._adapter.scsipi_cmd = ispcmd;

	isp->isp_state = ISP_RUNSTATE;
	isp->isp_osinfo._link.scsipi_scsi.channel =
	    (IS_DUALBUS(isp))? 0 : SCSI_CHANNEL_ONLY_ONE;
	isp->isp_osinfo._link.adapter_softc = isp;
	isp->isp_osinfo._link.device = &isp_dev;
	isp->isp_osinfo._link.adapter = &isp->isp_osinfo._adapter;
	isp->isp_osinfo._link.openings = isp->isp_maxcmds;
	/*
	 * Until the midlayer is fixed to use REPORT LUNS, limit to 8 luns.
	 */
	isp->isp_osinfo._link.scsipi_scsi.max_lun =
	   (isp->isp_maxluns < 7)? isp->isp_maxluns - 1 : 7;
	TAILQ_INIT(&isp->isp_osinfo.waitq);	/* The 2nd bus will share.. */

	if (IS_FC(isp)) {
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_FC_TARG-1;
	} else {
		sdparam *sdp = isp->isp_param;
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_TARGETS-1;
		isp->isp_osinfo._link.scsipi_scsi.adapter_target =
		    sdp->isp_initiator_id;
		isp->isp_osinfo.discovered[0] = 1 << sdp->isp_initiator_id;
		if (IS_DUALBUS(isp)) {
			isp->isp_osinfo._link_b = isp->isp_osinfo._link;
			sdp++;
			isp->isp_osinfo.discovered[1] =
			    1 << sdp->isp_initiator_id;
			isp->isp_osinfo._link_b.scsipi_scsi.adapter_target =
			    sdp->isp_initiator_id;
			isp->isp_osinfo._link_b.scsipi_scsi.channel = 1;
			isp->isp_osinfo._link_b.scsipi_scsi.max_lun =
			    isp->isp_osinfo._link.scsipi_scsi.max_lun;
		}
	}
	isp->isp_osinfo._link.type = BUS_SCSI;

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
		int defid;
		fcparam *fcp = isp->isp_param;
		delay(2 * 1000000);
		defid = MAX_FC_TARG;
		ISP_LOCK(isp);
		/*
		 * We probably won't have clock interrupts running,
		 * so we'll be really short (smoke test, really)
		 * at this time.
		 */
		if (isp_control(isp, ISPCTL_FCLINK_TEST, NULL)) {
			(void) isp_control(isp, ISPCTL_PDB_SYNC, NULL);
			if (fcp->isp_fwstate == FW_READY &&
			    fcp->isp_loopstate >= LOOP_PDB_RCVD) { 
				defid = fcp->isp_loopid;
			}
		}
		ISP_UNLOCK(isp);
		isp->isp_osinfo._link.scsipi_scsi.adapter_target = defid;
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
	config_found((void *)isp, &isp->isp_osinfo._link, scsiprint);
	if (IS_DUALBUS(isp)) {
		config_found((void *)isp, &isp->isp_osinfo._link_b, scsiprint);
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
ispioctl(sc_link, cmd, addr, flag, p)
	struct scsipi_link *sc_link;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct ispsoftc *isp = sc_link->adapter_softc;
	int s, chan, retval = ENOTTY;
	
	chan = (sc_link->scsipi_scsi.channel == SCSI_CHANNEL_ONLY_ONE)? 0 :
	    sc_link->scsipi_scsi.channel;

	switch (cmd) {
	case SCBUSACCEL:
	{
		struct scbusaccel_args *sp = (struct scbusaccel_args *)addr;
		if (IS_SCSI(isp) && sp->sa_lun == 0) {
			int dflags = 0;
			sdparam *sdp = SDPARAM(isp);

			sdp += chan;
			if (sp->sa_flags & SC_ACCEL_TAGS)
				dflags |= DPARM_TQING;
			if (sp->sa_flags & SC_ACCEL_WIDE)
				dflags |= DPARM_WIDE;
			if (sp->sa_flags & SC_ACCEL_SYNC)
				dflags |= DPARM_SYNC;
			s = splbio();
			sdp->isp_devparam[sp->sa_target].dev_flags |= dflags;
			dflags = sdp->isp_devparam[sp->sa_target].dev_flags;
			sdp->isp_devparam[sp->sa_target].dev_update = 1;
			isp->isp_update |= (1 << chan);
			splx(s);
			isp_prt(isp, ISP_LOGDEBUG1,
			    "ispioctl: device flags 0x%x for %d.%d.X",
			    dflags, chan, sp->sa_target);
		}
		retval = 0;
		break;
	}
	case SCBUSIORESET:
		s = splbio();
		if (isp_control(isp, ISPCTL_RESET_BUS, &chan))
			retval = EIO;
		else
			retval = 0;
		(void) splx(s);
		break;
	default:
		break;
	}
	return (retval);
}


static int32_t
ispcmd(xs)
	XS_T *xs;
{
	struct ispsoftc *isp;
	int result, s;

	isp = XS_ISP(xs);
	s = splbio();
	if (isp->isp_state < ISP_RUNSTATE) {
		DISABLE_INTS(isp);
		isp_init(isp);
                if (isp->isp_state != ISP_INITSTATE) {
			ENABLE_INTS(isp);
                        (void) splx(s);
                        XS_SETERR(xs, HBA_BOTCH);
                        return (COMPLETE);
                }
                isp->isp_state = ISP_RUNSTATE;
		ENABLE_INTS(isp);
        }

	/*
	 * Check for queue blockage...
	 */
	if (isp->isp_osinfo.blocked) {
		if (xs->xs_control & XS_CTL_POLL) {
			xs->error = XS_DRIVER_STUFFUP;
			splx(s);
			return (TRY_AGAIN_LATER);
		}
		TAILQ_INSERT_TAIL(&isp->isp_osinfo.waitq, xs, adapter_q);
		splx(s);
		return (SUCCESSFULLY_QUEUED);
	}

	if (xs->xs_control & XS_CTL_POLL) {
		volatile u_int8_t ombi = isp->isp_osinfo.no_mbox_ints;
		isp->isp_osinfo.no_mbox_ints = 1;
		result = isp_polled_cmd(isp, xs);
		isp->isp_osinfo.no_mbox_ints = ombi;
		(void) splx(s);
		return (result);
	}

	result = isp_start(xs);
#if	0
{
	static int na[16] = { 0 };
	if (na[isp->isp_unit] < isp->isp_nactive) {
		isp_prt(isp, ISP_LOGALL, "active hiwater %d", isp->isp_nactive);
		na[isp->isp_unit] = isp->isp_nactive;
	}
}
#endif
	switch (result) {
	case CMD_QUEUED:
		result = SUCCESSFULLY_QUEUED;
		if (xs->timeout) {
			callout_reset(&xs->xs_callout, _XT(xs), isp_dog, xs);
		}
		break;
	case CMD_EAGAIN:
		result = TRY_AGAIN_LATER;
		break;
	case CMD_RQLATER:
		result = SUCCESSFULLY_QUEUED;
		callout_reset(&xs->xs_callout, hz, isp_command_requeue, xs);
		break;
	case CMD_COMPLETE:
		result = COMPLETE;
		break;
	}
	(void) splx(s);
	return (result);
}

static int
isp_polled_cmd(isp, xs)
	struct ispsoftc *isp;
	XS_T *xs;
{
	int result;
	int infinite = 0, mswait;

	result = isp_start(xs);

	switch (result) {
	case CMD_QUEUED:
		result = SUCCESSFULLY_QUEUED;
		break;
	case CMD_RQLATER:
	case CMD_EAGAIN:
		if (XS_NOERR(xs)) {
			xs->error = XS_DRIVER_STUFFUP;
		}
		result = TRY_AGAIN_LATER;
		break;
	case CMD_COMPLETE:
		result = COMPLETE;
		break;
		
	}

	if (result != SUCCESSFULLY_QUEUED) {
		return (result);
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
	result = COMPLETE;
	return (result);
}

void
isp_done(xs)
	XS_T *xs;
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
isp_dog(arg)
	void *arg;
{
	XS_T *xs = arg;
	struct ispsoftc *isp = XS_ISP(xs);
	u_int32_t handle;

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
isp_uninit(isp)
	struct ispsoftc *isp;
{
	isp_lock(isp);
	/*
	 * Leave with interrupts disabled.
	 */
	DISABLE_INTS(isp);
	isp_unlock(isp);
}

/*
 * Restart function for a command to be requeued later.
 */
static void
isp_command_requeue(arg)
	void *arg;
{
	struct scsipi_xfer *xs = arg;
	struct ispsoftc *isp = XS_ISP(xs);
	ISP_LOCK(isp);
	switch (ispcmd(xs)) {
	case SUCCESSFULLY_QUEUED:
		isp_prt(isp, ISP_LOGINFO,
		    "requeued commands for %d.%d", XS_TGT(xs), XS_LUN(xs));
		if (xs->timeout) {
			callout_reset(&xs->xs_callout, _XT(xs), isp_dog, xs);
		}
		break;
	case TRY_AGAIN_LATER:
		isp_prt(isp, ISP_LOGINFO,
		    "EAGAIN on requeue for %d.%d", XS_TGT(xs), XS_LUN(xs));
		callout_reset(&xs->xs_callout, hz, isp_command_requeue, xs);
		break;
	case COMPLETE:
		/* can only be an error */
		XS_CMD_S_DONE(xs);
		callout_stop(&xs->xs_callout);
		if (XS_NOERR(xs)) {
			XS_SETERR(xs, HBA_BOTCH);
		}
		scsipi_done(xs);
		break;
	}
	ISP_UNLOCK(isp);
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
	int result, nrestarted = 0;

	ISP_LOCK(isp);
	if (isp->isp_osinfo.blocked == 0) {
		struct scsipi_xfer *xs;
		while ((xs = TAILQ_FIRST(&isp->isp_osinfo.waitq)) != NULL) {
			TAILQ_REMOVE(&isp->isp_osinfo.waitq, xs, adapter_q);
			result = isp_start(xs);
			if (result != CMD_QUEUED) {
				isp_prt(isp, ISP_LOGERR,
				    "botched command restart (err=%d)", result);
				XS_CMD_S_DONE(xs);
				if (xs->error == XS_NOERROR)
					xs->error = XS_DRIVER_STUFFUP;
				callout_stop(&xs->xs_callout);
				scsipi_done(xs);
			} else if (xs->timeout) {
				callout_reset(&xs->xs_callout,
				    _XT(xs), isp_dog, xs);
			}
			nrestarted++;
		}
		isp_prt(isp, ISP_LOGINFO,
		    "isp_restart requeued %d commands", nrestarted);
	}
	ISP_UNLOCK(isp);
}

int
isp_async(isp, cmd, arg)
	struct ispsoftc *isp;
	ispasync_t cmd;
	void *arg;
{
	int bus, tgt;
	int s = splbio();
	switch (cmd) {
	case ISPASYNC_NEW_TGT_PARAMS:
	if (IS_SCSI(isp) && isp->isp_dblev) {
		sdparam *sdp = isp->isp_param;
		char *wt;
		int mhz, flags, period;

		tgt = *((int *) arg);
		bus = (tgt >> 16) & 0xffff;
		tgt &= 0xffff;
		sdp += bus;
		flags = sdp->isp_devparam[tgt].cur_dflags;
		period = sdp->isp_devparam[tgt].cur_period;

		if ((flags & DPARM_SYNC) && period &&
		    (sdp->isp_devparam[tgt].cur_offset) != 0) {
			/*
			 * There's some ambiguity about our negotiated speed
			 * if we haven't detected LVD mode correctly (which
			 * seems to happen, unfortunately). If we're in LVD
			 * mode, then different rules apply about speed.
			 */
			if (sdp->isp_lvdmode || period < 0xc) {
				switch (period) {
				case 0x9:
					mhz = 80;
					break;
				case 0xa:
					mhz = 40;
					break;
				case 0xb:
					mhz = 33;
					break;
				case 0xc:
					mhz = 25;
					break;
				default:
					mhz = 1000 / (period * 4);
					break;
				}
			} else {
				mhz = 1000 / (period * 4);
			}
		} else {
			mhz = 0;
		}
		switch (flags & (DPARM_WIDE|DPARM_TQING)) {
		case DPARM_WIDE:
			wt = ", 16 bit wide";
			break;
		case DPARM_TQING:
			wt = ", Tagged Queueing Enabled";
			break;
		case DPARM_WIDE|DPARM_TQING:
			wt = ", 16 bit wide, Tagged Queueing Enabled";
			break;
		default:
			wt = " ";
			break;
		}
		if (mhz) {
			isp_prt(isp, ISP_LOGINFO,
			    "Bus %d Target %d at %dMHz Max Offset %d%s",
			    bus, tgt, mhz, sdp->isp_devparam[tgt].cur_offset,
			    wt);
		} else {
			isp_prt(isp, ISP_LOGINFO,
			    "Bus %d Target %d Async Mode%s", bus, tgt, wt);
		}
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
		isp->isp_osinfo.blocked = 1;
		isp_prt(isp, ISP_LOGINFO, "Loop DOWN");
		break;
        case ISPASYNC_LOOP_UP:
		isp->isp_osinfo.blocked = 0;
		callout_reset(&isp->isp_osinfo._restart, 1,
		    isp_internal_restart, isp);
		isp_prt(isp, ISP_LOGINFO, "Loop UP");
		break;
	case ISPASYNC_PDB_CHANGED:
	if (IS_FC(isp) && isp->isp_dblev) {
		const char *fmt = "Target %d (Loop 0x%x) Port ID 0x%x "
		    "role %s %s\n Port WWN 0x%08x%08x\n Node WWN 0x%08x%08x";
		const static char *roles[4] = {
		    "No", "Target", "Initiator", "Target/Initiator"
		};
		char *ptr;
		fcparam *fcp = isp->isp_param;
		int tgt = *((int *) arg);
		struct lportdb *lp = &fcp->portdb[tgt]; 

		if (lp->valid) {
			ptr = "arrived";
		} else {
			ptr = "disappeared";
		}
		isp_prt(isp, ISP_LOGINFO, fmt, tgt, lp->loopid, lp->portid,
		    roles[lp->roles & 0x3], ptr,
		    (u_int32_t) (lp->port_wwn >> 32),
		    (u_int32_t) (lp->port_wwn & 0xffffffffLL),
		    (u_int32_t) (lp->node_wwn >> 32),
		    (u_int32_t) (lp->node_wwn & 0xffffffffLL));
		break;
	}
#ifdef	ISP2100_FABRIC
	case ISPASYNC_CHANGE_NOTIFY:
		isp_prt(isp, ISP_LOGINFO, "Name Server Database Changed");
		break;
	case ISPASYNC_FABRIC_DEV:
	{
		int target;
		struct lportdb *lp;
		sns_scrsp_t *resp = (sns_scrsp_t *) arg;
		u_int32_t portid;
		u_int64_t wwn;
		fcparam *fcp = isp->isp_param;

		portid =
		    (((u_int32_t) resp->snscb_port_id[0]) << 16) |
		    (((u_int32_t) resp->snscb_port_id[1]) << 8) |
		    (((u_int32_t) resp->snscb_port_id[2]));
		wwn =
		    (((u_int64_t)resp->snscb_portname[0]) << 56) |
		    (((u_int64_t)resp->snscb_portname[1]) << 48) |
		    (((u_int64_t)resp->snscb_portname[2]) << 40) |
		    (((u_int64_t)resp->snscb_portname[3]) << 32) |
		    (((u_int64_t)resp->snscb_portname[4]) << 24) |
		    (((u_int64_t)resp->snscb_portname[5]) << 16) |
		    (((u_int64_t)resp->snscb_portname[6]) <<  8) |
		    (((u_int64_t)resp->snscb_portname[7]));

		isp_prt(isp, ISP_LOGINFO,
		    "Fabric Device (Type 0x%x)@PortID 0x%x WWN 0x%08x%08x",
		    resp->snscb_port_type, portid, ((u_int32_t)(wwn >> 32)),
		    ((u_int32_t)(wwn & 0xffffffff)));

		for (target = FC_SNS_ID+1; target < MAX_FC_TARG; target++) {
			lp = &fcp->portdb[target];
			if (lp->port_wwn == wwn)
				break;
		}
		if (target < MAX_FC_TARG) {
			break;
		}
		for (target = FC_SNS_ID+1; target < MAX_FC_TARG; target++) {
			lp = &fcp->portdb[target];
			if (lp->port_wwn == 0)
				break;
		}
		if (target == MAX_FC_TARG) {
			isp_prt(isp, ISP_LOGWARN,
			    "no more space for fabric devices");
			return (-1);
		}
		lp->port_wwn = lp->node_wwn = wwn;
		lp->portid = portid;
		break;
	}
#endif
	default:
		break;
	}
	(void) splx(s);
	return (0);
}

#include <machine/stdarg.h>
void
#ifdef	__STDC__
isp_prt(struct ispsoftc *isp, int level, const char *fmt, ...)
#else
isp_prt(isp, fmt, va_alist)
	struct ispsoftc *isp;
	char *fmt;
	va_dcl;
#endif
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
