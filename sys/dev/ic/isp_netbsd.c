/* $NetBSD: isp_netbsd.c,v 1.5.2.2 1998/11/07 05:50:35 cgd Exp $ */
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

struct cfdriver isp_cd = {
	NULL, "isp", DV_DULL
};

static void ispminphys __P((struct buf *));
static int32_t ispcmd __P((ISP_SCSI_XFER_T *));

static struct scsipi_adapter isp_switch = {
	ispcmd,			/* scsipi_cmd */
	ispminphys,		/* scsipi_minphys */
	NULL,			/* scsipi_ioctl */
};

static struct scsipi_device isp_dev = { NULL, NULL, NULL, NULL };
static int isp_poll __P((struct ispsoftc *, ISP_SCSI_XFER_T *, int));

/*
 * Complete attachment of hardware, include subdevices.
 */
void
isp_attach(isp)
	struct ispsoftc *isp;
{
	isp->isp_state = ISP_RUNSTATE;
	isp->isp_osinfo._link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	isp->isp_osinfo._link.adapter_softc = isp;
	isp->isp_osinfo._link.device = &isp_dev;
	isp->isp_osinfo._link.adapter = &isp_switch;

	if (isp->isp_type & ISP_HA_FC) {
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_FC_TARG-1;
		isp->isp_osinfo._link.openings =
			RQUEST_QUEUE_LEN / (MAX_FC_TARG-1);
		isp->isp_osinfo._link.scsipi_scsi.adapter_target = 0xff;
	} else {
		isp->isp_osinfo._link.openings =
			RQUEST_QUEUE_LEN / (MAX_TARGETS-1);
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_TARGETS-1;
		isp->isp_osinfo._link.scsipi_scsi.adapter_target =
			((sdparam *)isp->isp_param)->isp_initiator_id;
	}
	if (isp->isp_osinfo._link.openings < 2)
		isp->isp_osinfo._link.openings = 2;
	isp->isp_osinfo._link.type = BUS_SCSI;
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
	ISP_LOCKVAL_DECL;

	isp = XS_ISP(xs);
	ISP_LOCK(isp);
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
#ifdef	SDEV_NOSYNC
			if (xs->sc_link->quirks & SDEV_NOSYNC)
				f &= ~DPARM_SYNC;
#endif
#ifdef	SDEV_NOWIDE
			if (xs->sc_link->quirks & SDEV_NOWIDE)
				f &= ~DPARM_WIDE;
#endif
#ifdef	SDEV_NOTAG
			if (xs->sc_link->quirks & SDEV_NOTAG)
				f &= ~DPARM_TQING;
#endif
			sdp->isp_devparam[XS_TGT(xs)].dev_flags &=
				~(DPARM_WIDE|DPARM_SYNC|DPARM_TQING);
			sdp->isp_devparam[XS_TGT(xs)].dev_flags |= f;
			sdp->isp_devparam[XS_TGT(xs)].dev_update = 1;
			isp->isp_update = 1;
		}
	}
	result = ispscsicmd(xs);
	if (result != CMD_QUEUED || (xs->flags & SCSI_POLL) == 0) {
		ISP_UNLOCK(isp);
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
	ISP_UNLOCK(isp);
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
