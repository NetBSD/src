/* $NetBSD: isp_netbsd.c,v 1.18.2.2 1999/10/19 20:02:23 thorpej Exp $ */
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

static void ispminphys __P((struct buf *));
static void isp_scsipi_request __P((struct scsipi_channel *,
	scsipi_adapter_req_t, void *));
static int
ispioctl __P((struct scsipi_channel *, u_long, caddr_t, int, struct proc *));

static int isp_poll __P((struct ispsoftc *, ISP_SCSI_XFER_T *, int));
static void isp_watch __P((void *));
static void isp_command_requeue __P((void *));
static void isp_internal_restart __P((void *));

/*
 * Complete attachment of hardware, include subdevices.
 */
void
isp_attach(isp)
	struct ispsoftc *isp;
{
	struct scsipi_adapter *adapt = &isp->isp_osinfo._adapter;
	struct scsipi_channel *chan;
	int i;

	isp->isp_state = ISP_RUNSTATE;
	TAILQ_INIT(&isp->isp_osinfo.waitq);	/* XXX 2nd Bus? */

	/*
	 * Fill in the scsipi_adapter.
	 */
	adapt->adapt_dev = &isp->isp_osinfo._dev;
	if (IS_FC(isp) == 0 && IS_12X0(isp) != 0)
		adapt->adapt_nchannels = 2;
	else
		adapt->adapt_nchannels = 1;
	adapt->adapt_openings = isp->isp_maxcmds;
	adapt->adapt_max_periph = adapt->adapt_openings;
	adapt->adapt_request = isp_scsipi_request;
	adapt->adapt_minphys = ispminphys;
	adapt->adapt_ioctl = ispioctl;

	/*
	 * Fill in the scsipi_channel(s).
	 */
	for (i = 0; i < adapt->adapt_nchannels; i++) {
		chan = &isp->isp_osinfo._channels[i];
		chan->chan_adapter = adapt;
		chan->chan_bustype = &scsi_bustype;
		chan->chan_channel = i;

		if (IS_FC(isp)) {
			fcparam *fcp = isp->isp_param;
			fcp = &fcp[i];

			/*
			 * Give it another chance here to come alive...
			 */
			if (fcp->isp_fwstate != FW_READY) {
				(void) isp_control(isp, ISPCTL_FCLINK_TEST,
				    NULL);
			}
			chan->chan_ntargets = MAX_FC_TARG;
#ifdef	ISP2100_SCCLUN
			/*
			 * 16 bits worth, but let's be reasonable..
			 */
			chan->chan_nluns = 256;
#else
			chan->chan_nluns = 16;
#endif
			chan->chan_id = fcp->isp_loopid;
		} else {
			sdparam *sdp = isp->isp_param;
			sdp = &sdp[i];

			chan->chan_ntargets = MAX_TARGETS;
			if (isp->isp_bustype == ISP_BT_SBUS)
				chan->chan_nluns = 8;
			else {
#if 0
				/* Too many broken targets... */
				if (isp->isp_fwrev >= ISP_FW_REV(7,55,0))
					chan->chan_nluns = 32;
				else
#endif
					chan->chan_nluns = 7;
			}
			chan->chan_id = sdp->isp_initiator_id;
		}
	}

	/*
	 * Send a SCSI Bus Reset (used to be done as part of attach,
	 * but now left to the OS outer layers).
	 */
	if (IS_SCSI(isp)) {
		for (i = 0; i < adapt->adapt_nchannels; i++)
			(void) isp_control(isp, ISPCTL_RESET_BUS, &i);
		SYS_DELAY(2*1000000);
	}

	/*
	 * Start the watchdog.
	 */
	isp->isp_dogactive = 1;
	timeout(isp_watch, isp, WATCH_INTERVAL * hz);

	/*
	 * And attach children (if any).
	 */
	for (i = 0; i < adapt->adapt_nchannels; i++)
		(void) config_found((void *)isp, &isp->isp_osinfo._channels[i],
		    scsiprint);
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
ispioctl(chan, cmd, addr, flag, p)
	struct scsipi_channel *chan;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct ispsoftc *isp = (void *)chan->chan_adapter->adapt_dev;
	int s, ch, retval = ENOTTY;
	
	switch (cmd) {
	case SCBUSIORESET:
		ch = chan->chan_channel;
		s = splbio();
		if (isp_control(isp, ISPCTL_RESET_BUS, &ch))
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

static void
isp_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_xfer *xs;
	struct ispsoftc *isp = (void *)chan->chan_adapter->adapt_dev;
	int result, s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		s = splbio();
		if (isp->isp_state < ISP_RUNSTATE) {
			DISABLE_INTS(isp);
			isp_init(isp);
			if (isp->isp_state != ISP_INITSTATE) {
				ENABLE_INTS(isp);
				(void) splx(s);
				XS_SETERR(xs, HBA_BOTCH);
				XS_CMD_DONE(xs);
				return;
			}
			isp->isp_state = ISP_RUNSTATE;
			ENABLE_INTS(isp);
		}

		/*
		 * Check for queue blockage...
		 * XXX Should be done in the mid-layer.
		 */
		if (isp->isp_osinfo.blocked) {
			if (xs->xs_control & XS_CTL_POLL) {
				xs->error = XS_BUSY;
				scsipi_done(xs);
				splx(s);
				return;
			}
			TAILQ_INSERT_TAIL(&isp->isp_osinfo.waitq, xs,
			    channel_q);
			splx(s);
			return;
		}

		DISABLE_INTS(isp);
		result = ispscsicmd(xs);
		ENABLE_INTS(isp);

		switch (result) {
		case CMD_QUEUED:
			/*
			 * If we're not polling for completion, just return.
			 */
			if ((xs->xs_control & XS_CTL_POLL) == 0) {
				(void) splx(s);
				return;
			}
			break;

		case CMD_EAGAIN:
			/*
			 * Adapter resource shortage of some sort.  Should
			 * retry later.
			 */
			XS_SETERR(xs, XS_RESOURCE_SHORTAGE);
			XS_CMD_DONE(xs);
			(void) splx(s);
			return;

		case CMD_RQLATER:
			/*
			 * XXX I think what we should do here is freeze
			 * XXX the channel queue, and do a timed thaw
			 * XXX of it.  Need to add this to the mid-layer.
			 */
			if ((xs->xs_control & XS_CTL_POLL) != 0) {
				XS_SETERR(xs, XS_DRIVER_STUFFUP);
				XS_CMD_DONE(xs);
			} else
				timeout(isp_command_requeue, xs, hz);
			(void) splx(s);
			return;

		case CMD_COMPLETE:
			/*
			 * Something went horribly wrong, xs->error is set,
			 * and we just need to finish it off.
			 */
			XS_CMD_DONE(xs);
			(void) splx(s);
			return;
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
				if (isp_control(isp, ISPCTL_ABORT_CMD, xs)) {
					isp_restart(isp);
				}
				if (XS_NOERR(xs)) {
					XS_SETERR(xs, HBA_BOTCH);
				}
				XS_CMD_DONE(xs);
			}
		}
		(void) splx(s);
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		if (isp->isp_type & ISP_HA_SCSI) {
			sdparam *sdp = isp->isp_param;
			struct scsipi_periph *periph = arg;
			u_int16_t flags;

			sdp = &sdp[periph->periph_channel->chan_channel];

			flags =
			    sdp->isp_devparam[periph->periph_target].dev_flags;
			flags &= ~(DPARM_WIDE|DPARM_SYNC|DPARM_TQING);

			if (periph->periph_cap & PERIPH_CAP_SYNC)
				flags |= DPARM_SYNC;

			if (periph->periph_cap & PERIPH_CAP_WIDE16)
				flags |= DPARM_WIDE;

			if (periph->periph_cap & PERIPH_CAP_TQING)
				flags |= DPARM_TQING;

			sdp->isp_devparam[periph->periph_target].dev_flags =
			    flags;
			sdp->isp_devparam[periph->periph_target].dev_update = 1;
			isp->isp_update |=
			    (1 << periph->periph_channel->chan_channel);
			(void) isp_control(isp, ISPCTL_UPDATE_PARAMS, NULL);
		}
		return;

	case ADAPTER_REQ_GET_XFER_MODE:
		if (isp->isp_type & ISP_HA_SCSI) {
			sdparam *sdp = isp->isp_param;
			struct scsipi_periph *periph = arg;
			u_int16_t flags;

			sdp = &sdp[periph->periph_channel->chan_channel];

			periph->periph_mode = 0;
			periph->periph_period = 0;
			periph->periph_offset = 0;

			flags =
			    sdp->isp_devparam[periph->periph_target].cur_dflags;

			if (flags & DPARM_SYNC) {
				periph->periph_mode |= PERIPH_CAP_SYNC;
				periph->periph_period =
			    sdp->isp_devparam[periph->periph_target].cur_period;
				periph->periph_offset =
			    sdp->isp_devparam[periph->periph_target].cur_offset;
			}
			if (flags & DPARM_WIDE)
				periph->periph_mode |= PERIPH_CAP_WIDE16;
			if (flags & DPARM_TQING)
				periph->periph_mode |= PERIPH_CAP_TQING;

			periph->periph_flags |= PERIPH_MODE_VALID;
		}
		return;
	}
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
	struct scsipi_xfer *xs;
	int s;

	/*
	 * Look for completely dead commands (but not polled ones).
	 */
	s = splbio();
	for (i = 0; i < isp->isp_maxcmds; i++) {
		xs = isp->isp_xflist[i];
		if (xs == NULL) {
			continue;
		}
		if (xs->timeout == 0 || (xs->xs_control & XS_CTL_POLL)) {
			continue;
		}
		xs->timeout -= (WATCH_INTERVAL * 1000);
		/*
		 * Avoid later thinking that this
		 * transaction is not being timed.
		 * Then give ourselves to watchdog
		 * periods of grace.
		 */
		if (xs->timeout == 0) {
			xs->timeout = 1;
		} else if (xs->timeout > -(2 * WATCH_INTERVAL * 1000)) {
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
	(void) splx(s);
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
 * Restart function for a command to be requeued later.
 */
static void
isp_command_requeue(arg)
	void *arg;
{
	struct scsipi_xfer *xs = arg;
	int s;

	s = splbio();
	scsipi_adapter_request(xs->xs_periph->periph_channel,
	    ADAPTER_REQ_RUN_XFER, xs);
	(void) splx(s);
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
	int result, nrestarted = 0, s;

	s = splbio();
	if (isp->isp_osinfo.blocked == 0) {
		struct scsipi_xfer *xs;
		while ((xs = TAILQ_FIRST(&isp->isp_osinfo.waitq)) != NULL) {
			TAILQ_REMOVE(&isp->isp_osinfo.waitq, xs, channel_q);
			DISABLE_INTS(isp);
			result = ispscsicmd(xs);
			ENABLE_INTS(isp);
			if (result != CMD_QUEUED) {
				printf("%s: botched command restart (0x%x)\n",
				    isp->isp_name, result);
				if (XS_ERR(xs) == XS_NOERROR)
					XS_SETERR(xs, HBA_BOTCH);
				XS_CMD_DONE(xs);
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
	int bus;
	int s = splbio();
	switch (cmd) {
	case ISPASYNC_NEW_TGT_PARAMS:
		/*
		 * XXX Should we really do anything here?
		 */
		break;
	case ISPASYNC_BUS_RESET:
		if (arg)
			bus = *((int *) arg);
		else
			bus = 0;
		printf("%s: SCSI bus %d reset detected\n", isp->isp_name, bus);
		break;
	case ISPASYNC_LOOP_DOWN:
		/*
		 * Hopefully we get here in time to minimize the number
		 * of commands we are firing off that are sure to die.
		 */
		isp->isp_osinfo.blocked = 1;
		printf("%s: Loop DOWN\n", isp->isp_name);
		break;
        case ISPASYNC_LOOP_UP:
		isp->isp_osinfo.blocked = 0;
		timeout(isp_internal_restart, isp, 1);
		printf("%s: Loop UP\n", isp->isp_name);
		break;
	case ISPASYNC_PDB_CHANGED:
	if (IS_FC(isp) && isp->isp_dblev) {
		const char *fmt = "%s: Target %d (Loop 0x%x) Port ID 0x%x "
		    "role %s %s\n Port WWN 0x%08x%08x\n Node WWN 0x%08x%08x\n";
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
		printf(fmt, isp->isp_name, tgt, lp->loopid, lp->portid,
		    roles[lp->roles & 0x3], ptr,
		    (u_int32_t) (lp->port_wwn >> 32),
		    (u_int32_t) (lp->port_wwn & 0xffffffffLL),
		    (u_int32_t) (lp->node_wwn >> 32),
		    (u_int32_t) (lp->node_wwn & 0xffffffffLL));
		break;
	}
#ifdef	ISP2100_FABRIC
	case ISPASYNC_CHANGE_NOTIFY:
		printf("%s: Name Server Database Changed\n", isp->isp_name);
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
		printf("%s: Fabric Device (Type 0x%x)@PortID 0x%x WWN "
		    "0x%08x%08x\n", isp->isp_name, resp->snscb_port_type,
		    portid, ((u_int32_t)(wwn >> 32)),
		    ((u_int32_t)(wwn & 0xffffffff)));
		if (resp->snscb_port_type != 2)
			break;
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
			printf("%s: no more space for fabric devices\n",
			    isp->isp_name);
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
