/* $NetBSD: isp_netbsd.c,v 1.53.2.1 2002/06/20 16:33:12 gehenna Exp $ */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isp_netbsd.c,v 1.53.2.1 2002/06/20 16:33:12 gehenna Exp $");

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

static void isp_config_interrupts(struct device *);
static void ispminphys_1020(struct buf *);
static void ispminphys(struct buf *);
static INLINE void ispcmd(struct ispsoftc *, XS_T *);
static void isprequest(struct scsipi_channel *, scsipi_adapter_req_t, void *);
static int
ispioctl(struct scsipi_channel *, u_long, caddr_t, int, struct proc *);

static void isp_polled_cmd(struct ispsoftc *, XS_T *);
static void isp_dog(void *);
static void isp_create_fc_worker(void *);
static void isp_fc_worker(void *);

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
	if (isp->isp_type <= ISP_HA_SCSI_1020A) {
		isp->isp_osinfo._adapter.adapt_minphys = ispminphys_1020;
	} else {
		isp->isp_osinfo._adapter.adapt_minphys = ispminphys;
	}

	isp->isp_osinfo._chan.chan_adapter = &isp->isp_osinfo._adapter;
	isp->isp_osinfo._chan.chan_bustype = &scsi_bustype;
	isp->isp_osinfo._chan.chan_channel = 0;

	/*
	 * Until the midlayer is fixed to use REPORT LUNS, limit to 8 luns.
	 */
	isp->isp_osinfo._chan.chan_nluns = min(isp->isp_maxluns, 8);

	if (IS_FC(isp)) {
		isp->isp_osinfo._chan.chan_ntargets = MAX_FC_TARG;
		isp->isp_osinfo._chan.chan_id = MAX_FC_TARG;
		isp->isp_osinfo.threadwork = 1;
		/*
		 * Note that isp_create_fc_worker won't get called
		 * until much much later (after proc0 is created).
		 */
		kthread_create(isp_create_fc_worker, isp);
#ifdef	ISP_FW_CRASH_DUMP
		if (IS_2200(isp)) {
			FCPARAM(isp)->isp_dump_data =
			    malloc(QLA2200_RISC_IMAGE_DUMP_SIZE, M_DEVBUF,
				M_NOWAIT);
		} else if (IS_23XX(isp)) {
			FCPARAM(isp)->isp_dump_data =
			    malloc(QLA2300_RISC_IMAGE_DUMP_SIZE, M_DEVBUF,
				M_NOWAIT);
		}
		if (FCPARAM(isp)->isp_dump_data)
			FCPARAM(isp)->isp_dump_data[0] = 0;
#endif
	} else {
		int bus = 0;
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
		ISP_LOCK(isp);
		(void) isp_control(isp, ISPCTL_RESET_BUS, &bus);
		if (IS_DUALBUS(isp)) {
			bus++;
			(void) isp_control(isp, ISPCTL_RESET_BUS, &bus);
		}
		ISP_UNLOCK(isp);
	}


	/*
         * Defer enabling mailbox interrupts until later.
         */
        config_interrupts((struct device *) isp, isp_config_interrupts);

	/*
	 * And attach children (if any).
	 */
	config_found((void *)isp, &isp->isp_chanA, scsiprint);
	if (IS_DUALBUS(isp)) {
		config_found((void *)isp, &isp->isp_chanB, scsiprint);
	}
}


static void
isp_config_interrupts(struct device *self)
{
        struct ispsoftc *isp = (struct ispsoftc *) self;

	/*
	 * After this point, we'll be doing the new configuration
	 * schema which allows interrups, so we can do tsleep/wakeup
	 * for mailbox stuff at that point.
	 */
	isp->isp_osinfo.no_mbox_ints = 0;
}


/*
 * minphys our xfers
 */

static void
ispminphys_1020(struct buf *bp)
{
	if (bp->b_bcount >= (1 << 24)) {
		bp->b_bcount = (1 << 24);
	}
	minphys(bp);
}

static void
ispminphys(struct buf *bp)
{
	if (bp->b_bcount >= (1 << 30)) {
		bp->b_bcount = (1 << 30);
	}
	minphys(bp);
}

static int      
ispioctl(struct scsipi_channel *chan, u_long cmd, caddr_t addr, int flag,
	struct proc *p)
{
	struct ispsoftc *isp = (void *)chan->chan_adapter->adapt_dev;
	int retval = ENOTTY;
	
	switch (cmd) {
#ifdef	ISP_FW_CRASH_DUMP
	case ISP_GET_FW_CRASH_DUMP:
	{
		u_int16_t *ptr = FCPARAM(isp)->isp_dump_data;
		size_t sz;

		retval = 0;
		if (IS_2200(isp))
			sz = QLA2200_RISC_IMAGE_DUMP_SIZE;
		else
			sz = QLA2300_RISC_IMAGE_DUMP_SIZE;
		ISP_LOCK(isp);
		if (ptr && *ptr) {
			void *uaddr = *((void **) addr);
			if (copyout(ptr, uaddr, sz)) {
				retval = EFAULT;
			} else {
				*ptr = 0;
			}
		} else {
			retval = ENXIO;
		}
		ISP_UNLOCK(isp);
		break;
	}

	case ISP_FORCE_CRASH_DUMP:
		ISP_LOCK(isp);
		if (isp->isp_osinfo.blocked == 0) {
                        isp->isp_osinfo.blocked = 1;
                        scsipi_channel_freeze(&isp->isp_chanA, 1);
                }
		isp_fw_dump(isp);
		isp_reinit(isp);
		ISP_UNLOCK(isp);
		retval = 0;
		break;
#endif
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
	case ISP_RESCAN:
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
	case ISP_GET_STATS:
	{
		isp_stats_t *sp = (isp_stats_t *) addr;

		MEMZERO(sp, sizeof (*sp));
		sp->isp_stat_version = ISP_STATS_VERSION;
		sp->isp_type = isp->isp_type;
		sp->isp_revision = isp->isp_revision;
		ISP_LOCK(isp);
		sp->isp_stats[ISP_INTCNT] = isp->isp_intcnt;
		sp->isp_stats[ISP_INTBOGUS] = isp->isp_intbogus;
		sp->isp_stats[ISP_INTMBOXC] = isp->isp_intmboxc;
		sp->isp_stats[ISP_INGOASYNC] = isp->isp_intoasync;
		sp->isp_stats[ISP_RSLTCCMPLT] = isp->isp_rsltccmplt;
		sp->isp_stats[ISP_FPHCCMCPLT] = isp->isp_fphccmplt;
		sp->isp_stats[ISP_RSCCHIWAT] = isp->isp_rscchiwater;
		sp->isp_stats[ISP_FPCCHIWAT] = isp->isp_fpcchiwater;
		ISP_UNLOCK(isp);
		retval = 0;
		break;
	}
	case ISP_CLR_STATS:
		ISP_LOCK(isp);
		isp->isp_intcnt = 0;
		isp->isp_intbogus = 0;
		isp->isp_intmboxc = 0;
		isp->isp_intoasync = 0;
		isp->isp_rsltccmplt = 0;
		isp->isp_fphccmplt = 0;
		isp->isp_rscchiwater = 0;
		isp->isp_fpcchiwater = 0;
		ISP_UNLOCK(isp);
		retval = 0;
		break;
	case ISP_FC_GETHINFO:
	{
		struct isp_hba_device *hba = (struct isp_hba_device *) addr;
		MEMZERO(hba, sizeof (*hba));
		ISP_LOCK(isp);
		hba->fc_speed = FCPARAM(isp)->isp_gbspeed;
		hba->fc_scsi_supported = 1;
		hba->fc_topology = FCPARAM(isp)->isp_topo + 1;
		hba->fc_loopid = FCPARAM(isp)->isp_loopid;
		hba->active_node_wwn = FCPARAM(isp)->isp_nodewwn;
		hba->active_port_wwn = FCPARAM(isp)->isp_portwwn;
		ISP_UNLOCK(isp);
		break;
	}
	case SCBUSIORESET:
		ISP_LOCK(isp);
		if (isp_control(isp, ISPCTL_RESET_BUS, &chan->chan_channel))
			retval = EIO;
		else
			retval = 0;
		ISP_UNLOCK(isp);
		break;
	default:
		break;
	}
	return (retval);
}

static INLINE void
ispcmd(struct ispsoftc *isp, XS_T *xs)
{
	ISP_LOCK(isp);
	if (isp->isp_state < ISP_RUNSTATE) {
		DISABLE_INTS(isp);
		isp_init(isp);
		if (isp->isp_state != ISP_INITSTATE) {
			ENABLE_INTS(isp);
			ISP_UNLOCK(isp);
			XS_SETERR(xs, HBA_BOTCH);
			scsipi_done(xs);
			return;
		}
		isp->isp_state = ISP_RUNSTATE;
		ENABLE_INTS(isp);
	}
	/*
	 * Handle the case of a FC card where the FC thread hasn't
	 * fired up yet and we have loop state to clean up. If we
	 * can't clear things up and we've never seen loop up, bounce
	 * the command.
	 */
	if (IS_FC(isp) && isp->isp_osinfo.threadwork &&
	    isp->isp_osinfo.thread == 0) {
		volatile u_int8_t ombi = isp->isp_osinfo.no_mbox_ints;
		int delay_time;

		if (xs->xs_control & XS_CTL_POLL) {
			isp->isp_osinfo.no_mbox_ints = 1;
		}

		if (isp->isp_osinfo.loop_checked == 0) {
			delay_time = 10 * 1000000;
			isp->isp_osinfo.loop_checked = 1;
		} else {
			delay_time = 250000;
		}

		if (isp_fc_runstate(isp, delay_time) != 0) {
			if (xs->xs_control & XS_CTL_POLL) {
				isp->isp_osinfo.no_mbox_ints = ombi;
			}
			if (FCPARAM(isp)->loop_seen_once == 0) {
				XS_SETERR(xs, HBA_SELTIMEOUT);
				scsipi_done(xs);
				ISP_UNLOCK(isp);
				return;
			}
			/*
			 * Otherwise, fall thru to be queued up for later.
			 */
		} else {
			int wasblocked =
			    (isp->isp_osinfo.blocked || isp->isp_osinfo.paused);
			isp->isp_osinfo.threadwork = 0;
			isp->isp_osinfo.blocked =
			    isp->isp_osinfo.paused = 0;
			if (wasblocked) {
				scsipi_channel_thaw(&isp->isp_chanA, 1);
			}
		}
		if (xs->xs_control & XS_CTL_POLL) {
			isp->isp_osinfo.no_mbox_ints = ombi;
		}
	}

	if (isp->isp_osinfo.paused) {
		isp_prt(isp, ISP_LOGWARN, "I/O while paused");
		xs->error = XS_RESOURCE_SHORTAGE;
		scsipi_done(xs);
		ISP_UNLOCK(isp);
		return;
	}
	if (isp->isp_osinfo.blocked) {
		isp_prt(isp, ISP_LOGWARN, "I/O while blocked");
		xs->error = XS_REQUEUE;
		scsipi_done(xs);
		ISP_UNLOCK(isp);
		return;
	}

	if (xs->xs_control & XS_CTL_POLL) {
		volatile u_int8_t ombi = isp->isp_osinfo.no_mbox_ints;
		isp->isp_osinfo.no_mbox_ints = 1;
		isp_polled_cmd(isp, xs);
		isp->isp_osinfo.no_mbox_ints = ombi;
		ISP_UNLOCK(isp);
		return;
	}

	switch (isp_start(xs)) {
	case CMD_QUEUED:
		if (xs->timeout) {
			callout_reset(&xs->xs_callout, _XT(xs), isp_dog, xs);
		}
		break;
	case CMD_EAGAIN:
		isp->isp_osinfo.paused = 1;
		xs->error = XS_RESOURCE_SHORTAGE;
		scsipi_channel_freeze(&isp->isp_chanA, 1);
		if (IS_DUALBUS(isp)) {
			scsipi_channel_freeze(&isp->isp_chanB, 1);
		}
		scsipi_done(xs);
		break;
	case CMD_RQLATER:
		/*
		 * We can only get RQLATER from FC devices (1 channel only)
		 *
		 * Also, if we've never seen loop up, bounce the command
		 * (somebody has booted with no FC cable connected)
		 */
		if (FCPARAM(isp)->loop_seen_once == 0) {
			XS_SETERR(xs, HBA_SELTIMEOUT);
			scsipi_done(xs);
			break;
		}
		if (isp->isp_osinfo.blocked == 0) {
			isp->isp_osinfo.blocked = 1;
			scsipi_channel_freeze(&isp->isp_chanA, 1);
		}
		xs->error = XS_REQUEUE;
		scsipi_done(xs);
		break;
	case CMD_COMPLETE:
		scsipi_done(xs);
		break;
	}
	ISP_UNLOCK(isp);
}

static void
isprequest(struct scsipi_channel *chan, scsipi_adapter_req_t req, void *arg)
{
	struct ispsoftc *isp = (void *)chan->chan_adapter->adapt_dev;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		ispcmd(isp, (XS_T *) arg);
		break;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* Not supported. */
		break;

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
		ISP_LOCK(isp);
		sdp->isp_devparam[xm->xm_target].goal_flags |= dflags;
		dflags = sdp->isp_devparam[xm->xm_target].goal_flags;
		sdp->isp_devparam[xm->xm_target].dev_update = 1;
		isp->isp_update |= (1 << chan->chan_channel);
		ISP_UNLOCK(isp);
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
isp_polled_cmd(struct ispsoftc *isp, XS_T *xs)
{
	int result;
	int infinite = 0, mswait;

	result = isp_start(xs);

	switch (result) {
	case CMD_QUEUED:
		break;
	case CMD_RQLATER:
		if (XS_NOERR(xs)) {
			xs->error = XS_REQUEUE;
		}
	case CMD_EAGAIN:
		if (XS_NOERR(xs)) {
			xs->error = XS_RESOURCE_SHORTAGE;
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
		u_int16_t isr, sema, mbox;
		if (ISP_READ_ISR(isp, &isr, &sema, &mbox)) {
			isp_intr(isp, isr, sema, mbox);
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
		/*
		 * Fixup- if we get a QFULL, we need
		 * to set XS_BUSY as the error.
		 */
		if (xs->status == SCSI_QUEUE_FULL) {
			xs->error = XS_BUSY;
		}
		if (isp->isp_osinfo.paused) {
			isp->isp_osinfo.paused = 0;
			scsipi_channel_timed_thaw(&isp->isp_chanA);
			if (IS_DUALBUS(isp)) {
				scsipi_channel_timed_thaw(&isp->isp_chanB);
			}
		}
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
		u_int16_t isr, mbox, sema;

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

		if (ISP_READ_ISR(isp, &isr, &sema, &mbox)) {
			isp_intr(isp, isr, sema, mbox);

		}
		if (XS_CMD_DONE_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1,
			    "watchdog cleanup for handle 0x%x", handle);
			XS_CMD_C_WDOG(xs);
			isp_done(xs);
		} else if (XS_CMD_GRACE_P(xs)) {
			isp_prt(isp, ISP_LOGDEBUG1,
			    "watchdog timeout for handle 0x%x", handle);
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
			u_int16_t nxti, optr;
			ispreq_t local, *mp = &local, *qe;
			isp_prt(isp, ISP_LOGDEBUG2,
			    "possible command timeout on handle %x", handle);
			XS_CMD_C_WDOG(xs);
			callout_reset(&xs->xs_callout, hz, isp_dog, xs);
			if (isp_getrqentry(isp, &nxti, &optr, (void **) &qe)) {
				ISP_UNLOCK(isp);
				return;
			}
			XS_CMD_S_GRACE(xs);
			MEMZERO((void *) mp, sizeof (*mp));
			mp->req_header.rqs_entry_count = 1;
			mp->req_header.rqs_entry_type = RQSTYPE_MARKER;
			mp->req_modifier = SYNC_ALL;
			mp->req_target = XS_CHANNEL(xs) << 7;
			isp_put_request(isp, mp, qe);
			ISP_ADD_REQUEST(isp, nxti);
		}
	} else {
		isp_prt(isp, ISP_LOGDEBUG0, "watchdog with no command");
	}
	ISP_IUNLOCK(isp);
}

/*
 * Fibre Channel state cleanup thread
 */
static void
isp_create_fc_worker(void *arg)
{
	struct ispsoftc *isp = arg;

	if (kthread_create1(isp_fc_worker, isp, &isp->isp_osinfo.thread,
	    "%s:fc_thrd", isp->isp_name)) {
		isp_prt(isp, ISP_LOGERR, "unable to create FC worker thread");
		panic("isp_create_fc_worker");
	}

}

static void
isp_fc_worker(void *arg)
{
	void scsipi_run_queue(struct scsipi_channel *);
	struct ispsoftc *isp = arg;
	
	for (;;) {
		int s;

		/*
		 * Note we do *not* use the ISP_LOCK/ISP_UNLOCK macros here.
		 */
		s = splbio();
		while (isp->isp_osinfo.threadwork) {
			isp->isp_osinfo.threadwork = 0;
			if (isp_fc_runstate(isp, 10 * 1000000) == 0) {
				break;
			}
			if  (isp->isp_osinfo.loop_checked &&
			     FCPARAM(isp)->loop_seen_once == 0) {
				splx(s);
				goto skip;
			}
			isp->isp_osinfo.threadwork = 1;
			splx(s);
			delay(500 * 1000);
			s = splbio();
		}
		if (FCPARAM(isp)->isp_fwstate != FW_READY ||
		    FCPARAM(isp)->isp_loopstate != LOOP_READY) {
			isp_prt(isp, ISP_LOGINFO, "isp_fc_runstate in vain");
			isp->isp_osinfo.threadwork = 1;
			splx(s);
			continue;
		}

		if (isp->isp_osinfo.blocked) {
			isp->isp_osinfo.blocked = 0;
			isp_prt(isp, ISP_LOGDEBUG0,
			    "restarting queues (freeze count %d)",
			    isp->isp_chanA.chan_qfreeze);
			scsipi_channel_thaw(&isp->isp_chanA, 1);
		}

		if (isp->isp_osinfo.thread == NULL)
			break;

skip:
		(void) tsleep(&isp->isp_osinfo.thread, PRIBIO, "fcclnup", 0);

		splx(s);
	}

	/* In case parent is waiting for us to exit. */
	wakeup(&isp->isp_osinfo.thread);

	kthread_exit(0);
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
		flags = sdp->isp_devparam[tgt].actv_flags;

		xm.xm_mode = 0;
		xm.xm_period = sdp->isp_devparam[tgt].actv_period;
		xm.xm_offset = sdp->isp_devparam[tgt].actv_offset;
		xm.xm_target = tgt;

		if ((flags & DPARM_SYNC) && xm.xm_period && xm.xm_offset)
			xm.xm_mode |= PERIPH_CAP_SYNC;
		if (flags & DPARM_WIDE)
			xm.xm_mode |= PERIPH_CAP_WIDE16;
		if (flags & DPARM_TQING)
			xm.xm_mode |= PERIPH_CAP_TQING;
		scsipi_async_event(bus? &isp->isp_chanB : &isp->isp_chanA,
		    ASYNC_EVENT_XFER_MODE, &xm);
		break;
	}
	case ISPASYNC_BUS_RESET:
		bus = *((int *) arg);
		scsipi_async_event(bus? &isp->isp_chanB : &isp->isp_chanA,
		    ASYNC_EVENT_RESET, NULL);
		isp_prt(isp, ISP_LOGINFO, "SCSI bus %d reset detected", bus);
		break;
	case ISPASYNC_LIP:
		/*
		 * Don't do queue freezes or blockage until we have the
		 * thread running that can unfreeze/unblock us.
		 */
		if (isp->isp_osinfo.blocked == 0)  {
			if (isp->isp_osinfo.thread) {
				isp->isp_osinfo.blocked = 1;
				scsipi_channel_freeze(&isp->isp_chanA, 1);
			}
		}
		isp_prt(isp, ISP_LOGINFO, "LIP Received");
		break;
	case ISPASYNC_LOOP_RESET:
		/*
		 * Don't do queue freezes or blockage until we have the
		 * thread running that can unfreeze/unblock us.
		 */
		if (isp->isp_osinfo.blocked == 0) {
			if (isp->isp_osinfo.thread) {
				isp->isp_osinfo.blocked = 1;
				scsipi_channel_freeze(&isp->isp_chanA, 1);
			}
		}
		isp_prt(isp, ISP_LOGINFO, "Loop Reset Received");
		break;
	case ISPASYNC_LOOP_DOWN:
		/*
		 * Don't do queue freezes or blockage until we have the
		 * thread running that can unfreeze/unblock us.
		 */
		if (isp->isp_osinfo.blocked == 0) {
			if (isp->isp_osinfo.thread) {
				isp->isp_osinfo.blocked = 1;
				scsipi_channel_freeze(&isp->isp_chanA, 1);
			}
		}
		isp_prt(isp, ISP_LOGINFO, "Loop DOWN");
		break;
        case ISPASYNC_LOOP_UP:
		/*
		 * Let the subsequent ISPASYNC_CHANGE_NOTIFY invoke
		 * the FC worker thread. When the FC worker thread
		 * is done, let *it* call scsipi_channel_thaw...
		 */
		isp_prt(isp, ISP_LOGINFO, "Loop UP");
		break;
	case ISPASYNC_PROMENADE:
	if (IS_FC(isp) && isp->isp_dblev) {
		static const char fmt[] = "Target %d (Loop 0x%x) Port ID 0x%x "
		    "(role %s) %s\n Port WWN 0x%08x%08x\n Node WWN 0x%08x%08x";
		const static char *const roles[4] = {
		    "None", "Target", "Initiator", "Target/Initiator"
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
		if (arg == ISPASYNC_CHANGE_PDB) {
			isp_prt(isp, ISP_LOGINFO, "Port Database Changed");
		} else if (arg == ISPASYNC_CHANGE_SNS) {
			isp_prt(isp, ISP_LOGINFO,
			    "Name Server Database Changed");
		}

		/*
		 * We can set blocked here because we know it's now okay
		 * to try and run isp_fc_runstate (in order to build loop
		 * state). But we don't try and freeze the midlayer's queue
		 * if we have no thread that we can wake to later unfreeze
		 * it.
		 */
		if (isp->isp_osinfo.blocked == 0) {
			isp->isp_osinfo.blocked = 1;
			if (isp->isp_osinfo.thread) {
				scsipi_channel_freeze(&isp->isp_chanA, 1);
			}
		}
		/*
		 * Note that we have work for the thread to do, and
		 * if the thread is here already, wake it up.
		 */
		isp->isp_osinfo.threadwork++;
		if (isp->isp_osinfo.thread) {
			wakeup(&isp->isp_osinfo.thread);
		} else {
			isp_prt(isp, ISP_LOGDEBUG1, "no FC thread yet");
		}
		break;
	case ISPASYNC_FABRIC_DEV:
	{
		int target, base, lim;
		fcparam *fcp = isp->isp_param;
		struct lportdb *lp = NULL;
		struct lportdb *clp = (struct lportdb *) arg;
		char *pt;

		switch (clp->port_type) {
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
			pt = " ";
			break;
		}

		isp_prt(isp, ISP_LOGINFO,
		    "%s Fabric Device @ PortID 0x%x", pt, clp->portid);

		/*
		 * If we don't have an initiator role we bail.
		 *
		 * We just use ISPASYNC_FABRIC_DEV for announcement purposes.
		 */

		if ((isp->isp_role & ISP_ROLE_INITIATOR) == 0) {
			break;
		}

		/*
		 * Is this entry for us? If so, we bail.
		 */

		if (fcp->isp_portid == clp->portid) {
			break;
		}

		/*
		 * Else, the default policy is to find room for it in
		 * our local port database. Later, when we execute
		 * the call to isp_pdb_sync either this newly arrived
		 * or already logged in device will be (re)announced.
		 */

		if (fcp->isp_topo == TOPO_FL_PORT)
			base = FC_SNS_ID+1;
		else
			base = 0;

		if (fcp->isp_topo == TOPO_N_PORT)
			lim = 1;
		else
			lim = MAX_FC_TARG;

		/*
		 * Is it already in our list?
		 */
		for (target = base; target < lim; target++) {
			if (target >= FL_PORT_ID && target <= FC_SNS_ID) {
				continue;
			}
			lp = &fcp->portdb[target];
			if (lp->port_wwn == clp->port_wwn &&
			    lp->node_wwn == clp->node_wwn) {
				lp->fabric_dev = 1;
				break;
			}
		}
		if (target < lim) {
			break;
		}
		for (target = base; target < lim; target++) {
			if (target >= FL_PORT_ID && target <= FC_SNS_ID) {
				continue;
			}
			lp = &fcp->portdb[target];
			if (lp->port_wwn == 0) {
				break;
			}
		}
		if (target == lim) {
			isp_prt(isp, ISP_LOGWARN,
			    "out of space for fabric devices");
			break;
		}
		lp->port_type = clp->port_type;
		lp->fc4_type = clp->fc4_type;
		lp->node_wwn = clp->node_wwn;
		lp->port_wwn = clp->port_wwn;
		lp->portid = clp->portid;
		lp->fabric_dev = 1;
		break;
	}
	case ISPASYNC_FW_CRASH:
	{
		u_int16_t mbox1, mbox6;
		mbox1 = ISP_READ(isp, OUTMAILBOX1);
		if (IS_DUALBUS(isp)) { 
			mbox6 = ISP_READ(isp, OUTMAILBOX6);
		} else {
			mbox6 = 0;
		}
                isp_prt(isp, ISP_LOGERR,
                    "Internal Firmware Error on bus %d @ RISC Address 0x%x",
                    mbox6, mbox1);
		isp_reinit(isp);
		break;
	}
	default:
		break;
	}
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
