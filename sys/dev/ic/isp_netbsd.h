/* $NetBSD: isp_netbsd.h,v 1.18.2.3 1999/10/20 20:41:27 thorpej Exp $ */
/* release_6_5_99 */
/*
 * NetBSD Specific definitions for the Qlogic ISP Host Adapter
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

#include "opt_isp.h"

#define	ISP_PLATFORM_VERSION_MAJOR	0
#define	ISP_PLATFORM_VERSION_MINOR	997

#define	ISP_SCSI_XFER_T		struct scsipi_xfer
struct isposinfo {
	struct device		_dev;
	struct scsipi_adapter   _adapter;
	struct scsipi_channel	_channels[2];
	int			seed;
};

#define	MAXISPREQUEST	256
#ifdef	ISP2100_FABRIC
#define	ISP2100_SCRLEN		0x400
#else
#define	ISP2100_SCRLEN		0x100
#endif

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/ic/ispmbox.h>

#define	PRINTF			printf
#define	IDPRINTF(lev, x)	if (isp->isp_dblev >= lev) printf x
#ifdef	DIAGNOSTIC
#else
#endif

#define	MEMZERO			bzero
#define	MEMCPY(dst, src, count)	bcopy((src), (dst), (count))
#ifdef	__alpha__
#define	MemoryBarrier	alpha_mb
#else
#define	MemoryBarrier()
#endif

#define	DMA_MSW(x)	(((x) >> 16) & 0xffff)
#define	DMA_LSW(x)	(((x) & 0xffff))

#if	defined(SCSIDEBUG)
#define	DFLT_DBLEVEL		3
#define	CFGPRINTF		printf
#else
#if	defined(DIAGNOSTIC) || defined(DEBUG)
#define	DFLT_DBLEVEL		1
#define	CFGPRINTF		printf
#else
#define	DFLT_DBLEVEL		0
#define	CFGPRINTF		if (0) printf
#endif
#endif

#define	ISP_LOCKVAL_DECL	int isp_spl_save
#define	ISP_ILOCKVAL_DECL	ISP_LOCKVAL_DECL
#define	ISP_LOCK(x)		isp_spl_save = splbio()
#define	ISP_UNLOCK(x)		(void) splx(isp_spl_save)
#define	ISP_ILOCK		ISP_LOCK
#define	ISP_IUNLOCK		ISP_UNLOCK


#define	XS_NULL(xs)		xs == NULL || xs->xs_periph == NULL
#define	XS_ISP(xs)		\
    (void *)(xs)->xs_periph->periph_channel->chan_adapter->adapt_dev
#define	XS_LUN(xs)		((int) (xs)->xs_periph->periph_lun)
#define	XS_TGT(xs)		((int) (xs)->xs_periph->periph_target)
#define	XS_CHANNEL(xs)		\
    (xs)->xs_periph->periph_channel->chan_channel
#define	XS_RESID(xs)		(xs)->resid
#define	XS_XFRLEN(xs)		(xs)->datalen
#define	XS_CDBLEN(xs)		(xs)->cmdlen
#define	XS_CDBP(xs)		((caddr_t) (xs)->cmd)
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

#define	XS_CMD_DONE(xs)		scsipi_done(xs)
#define	XS_IS_CMD_DONE(xs)	(((xs)->xs_status & XS_STS_DONE) != 0)

/*
 * Can we use tagging?
 */
#define	XS_CANTAG(xs)		(XS_CTL_TAGTYPE(xs) != 0)

/*
 * The kind of tag to use for this command.
 */
#define	XS_KINDOF_TAG(xs)						\
	(XS_CTL_TAGTYPE(xs) == XS_CTL_SIMPLE_TAG ? REQFLAG_STAG :	\
	 (XS_CTL_TAGTYPE(xs) == XS_CTL_HEAD_TAG ? REQFLAG_HTAG :	\
	  REQFLAG_OTAG))

/*
 * These get turned into NetBSD midlayer codes
 */
#define	CMD_COMPLETE		100
#define	CMD_EAGAIN		101
#define	CMD_QUEUED		102
#define	CMD_RQLATER		103


#define	isp_name	isp_osinfo._dev.dv_xname
#define	isp_unit	isp_osinfo._dev.dv_unit

#define	SCSI_QFULL	0x28

#define	SYS_DELAY(x)	delay(x)

#define	WATCH_INTERVAL	30

#define	FC_FW_READY_DELAY	(12 * 1000000)
#define	DEFAULT_LOOPID(x)	108
#define	DEFAULT_WWN(x)		(0x1000beed00000000LL + (x)->isp_osinfo.seed)

extern void isp_attach __P((struct ispsoftc *));
extern void isp_uninit __P((struct ispsoftc *));

#define ISP_UNSWIZZLE_AND_COPY_PDBP(isp, dest, src)	\
        bcopy(src, dest, sizeof (isp_pdb_t))
#define ISP_SWIZZLE_ICB(a, b)
#ifdef	__sparc__
#define ISP_SWIZZLE_REQUEST(a, b)			\
	ISP_SBUSIFY_ISPHDR(a, &(b)->req_header);	\
        ISP_SBUSIFY_ISPREQ(a, b)
#define ISP_UNSWIZZLE_RESPONSE(a, b)			\
	ISP_SBUSIFY_ISPHDR(a, &(b)->req_header)
#else
#define ISP_SWIZZLE_REQUEST(a, b)
#define ISP_UNSWIZZLE_RESPONSE(a, b)
#endif
#define	ISP_SWIZZLE_SNS_REQ(a, b)
#define	ISP_UNSWIZZLE_SNS_RSP(a, b, c)

#define	INLINE	inline
#include <dev/ic/isp_inline.h>

#endif	/* _ISP_NETBSD_H */
