/* $NetBSD: isp_netbsd.h,v 1.10 1999/02/09 00:37:35 mjacob Exp $ */
/* release_01_29_99 */
/*
 * NetBSD Specific definitions for the Qlogic ISP Host Adapter
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
 */

#ifndef	_ISP_NETBSD_H
#define	_ISP_NETBSD_H

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


#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_debug.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#define	ISP_PLATFORM_VERSION_MAJOR	0
#define	ISP_PLATFORM_VERSION_MINOR	991

#define	ISP_SCSI_XFER_T		struct scsipi_xfer
struct isposinfo {
	struct device		_dev;
	struct scsipi_link	_link;
	struct scsipi_adapter   _adapter;
	int			blocked;
	TAILQ_HEAD(, scsipi_xfer) waitq; 
};
#define	MAXISPREQUEST	256

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/ic/ispmbox.h>

#define	PRINTF			printf
#define	IDPRINTF(lev, x)	if (isp->isp_dblev >= lev) printf x

#define	MEMZERO			bzero
#define	MEMCPY(dst, src, count)	bcopy((src), (dst), (count))

#if	defined(SCSIDEBUG)
#define	DFLT_DBLEVEL		3
#else
#if	defined(DEBUG)
#define	DFLT_DBLEVEL		2
#else
#define	DFLT_DBLEVEL		1
#endif
#endif

#define	ISP_LOCKVAL_DECL	int isp_spl_save
#define	ISP_ILOCKVAL_DECL	ISP_LOCKVAL_DECL
#define	ISP_LOCK(x)		isp_spl_save = splbio()
#define	ISP_UNLOCK(x)		(void) splx(isp_spl_save)
#define	ISP_ILOCK		ISP_LOCK
#define	ISP_IUNLOCK		ISP_UNLOCK


#define	XS_NULL(xs)		xs == NULL || xs->sc_link == NULL
#define	XS_ISP(xs)		(xs)->sc_link->adapter_softc
#define	XS_LUN(xs)		(xs)->sc_link->scsipi_scsi.lun
#define	XS_TGT(xs)		(xs)->sc_link->scsipi_scsi.target
#define	XS_RESID(xs)		(xs)->resid
#define	XS_XFRLEN(xs)		(xs)->datalen
#define	XS_CDBLEN(xs)		(xs)->cmdlen
#define	XS_CDBP(xs)		(xs)->cmd
#define	XS_STS(xs)		(xs)->status
#define	XS_TIME(xs)		(xs)->timeout
#define	XS_SNSP(xs)		(&(xs)->sense.scsi_sense)
#define	XS_SNSLEN(xs)		(sizeof (xs)->sense.scsi_sense)
#define	XS_SNSKEY(xs)		((xs)->sense.scsi_sense.flags)

#define	HBA_NOERROR		XS_NOERROR
#define	HBA_BOTCH		XS_DRIVER_STUFFUP
#define	HBA_CMDTIMEOUT		XS_TIMEOUT
#define	HBA_SELTIMEOUT		XS_SELTIMEOUT
#define	HBA_TGTBSY		XS_BUSY
#ifdef	XS_RESET
#define	HBA_BUSRESET		XS_RESET
#else
#define	HBA_BUSRESET		XS_DRIVER_STUFFUP
#endif
#define	HBA_ABORTED		XS_DRIVER_STUFFUP
#define	HBA_DATAOVR		XS_DRIVER_STUFFUP
#define	HBA_ARQFAIL		XS_DRIVER_STUFFUP

#define	XS_SNS_IS_VALID(xs)	(xs)->error = XS_SENSE
#define	XS_IS_SNS_VALID(xs)	((xs)->error == XS_SENSE)

#define	XS_INITERR(xs)		(xs)->error = 0
#define	XS_SETERR(xs, v)	(xs)->error = v
#define	XS_ERR(xs)		(xs)->error
#define	XS_NOERR(xs)		(xs)->error == XS_NOERROR

#define	XS_CMD_DONE(xs)		(xs)->flags |= ITSDONE, scsipi_done(xs)
#define	XS_IS_CMD_DONE(xs)	(((xs)->flags & ITSDONE) != 0)

/*
 * We use whether or not we're a polled command to decide about tagging.
 */
#define	XS_CANTAG(xs)		(((xs)->flags & SCSI_POLL) != 0)

/*
 * This is our default tag (ordered).
 */
#define	XS_KINDOF_TAG(xs)	\
	(((xs)->flags & SCSI_URGENT)? REQFLAG_HTAG : REQFLAG_STAG)

#define	CMD_COMPLETE		COMPLETE
#define	CMD_EAGAIN		TRY_AGAIN_LATER
#define	CMD_QUEUED		SUCCESSFULLY_QUEUED



#define	isp_name	isp_osinfo._dev.dv_xname


#define	SYS_DELAY(x)	delay(x)

#define	WATCH_INTERVAL	30

extern void isp_attach __P((struct ispsoftc *));
extern void isp_uninit __P((struct ispsoftc *));
#endif	/* _ISP_NETBSD_H */
