/* $NetBSD: isp_netbsd.h,v 1.27 2000/08/01 23:55:11 mjacob Exp $ */
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

#include "opt_isp.h"


#define	ISP_PLATFORM_VERSION_MAJOR	1
#define	ISP_PLATFORM_VERSION_MINOR	0

struct isposinfo {
	struct device		_dev;
	struct scsipi_link	_link;
	struct scsipi_link	_link_b;
	struct scsipi_adapter   _adapter;
	int	splsaved;
	int	mboxwaiting;
	unsigned int
				: 28,
		onintstack	: 1,
		no_mbox_ints	: 1,
		blocked		: 1,
		islocked	: 1;
	union {
		u_int64_t	_wwn;
		u_int16_t	_discovered[2];
#define	wwn_seed	un._wwn
#define	discovered	un._discovered
	} un;
	TAILQ_HEAD(, scsipi_xfer) waitq; 
	struct callout _restart;
};

/*
 * Required Macros/Defines
 */

#define	INLINE			inline

#define	ISP2100_FABRIC		1
#define	ISP2100_SCRLEN		0x400

#define	MEMZERO			bzero
#define	MEMCPY(dst, src, amt)	bcopy((src), (dst), (amt))
#define	SNPRINTF		snprintf
#define	STRNCAT			strncat
#define	USEC_DELAY		DELAY

#define	NANOTIME_T		struct timeval
#define	GET_NANOTIME		microtime
#define	GET_NANOSEC(x)		(((x)->tv_sec * 1000000 + (x)->tv_usec) * 1000)
#define	NANOTIME_SUB		isp_microtime_sub

#define	MAXISPREQUEST(isp)	256

#ifdef	__alpha__
#define	MEMORYBARRIER(isp, type, offset, size)	alpha_mb()
#else
#define	MEMORYBARRIER(isp, type, offset, size)
#endif

#define	MBOX_ACQUIRE(isp)
#define	MBOX_WAIT_COMPLETE	isp_wait_complete

#define	MBOX_NOTIFY_COMPLETE(isp)					\
	if (isp->isp_osinfo.mboxwaiting) {				\
                isp->isp_osinfo.mboxwaiting = 0;			\
                wakeup(&isp->isp_osinfo.mboxwaiting);			\
        }								\
	isp->isp_mboxbsy = 0

#define	MBOX_RELEASE(isp)

#ifndef	SCSI_GOOD
#define	SCSI_GOOD	0x0
#endif
#ifndef	SCSI_CHECK
#define	SCSI_CHECK	0x2
#endif
#ifndef	SCSI_BUSY
#define	SCSI_BUSY	0x8
#endif
#ifndef	SCSI_QFULL
#define	SCSI_QFULL	0x28
#endif

#define	XS_T			struct scsipi_xfer
#define	XS_CHANNEL(xs)		\
	(((int) (xs)->sc_link->scsipi_scsi.channel == SCSI_CHANNEL_ONLY_ONE) ? \
	    0 : (xs)->sc_link->scsipi_scsi.channel)
#define	XS_ISP(xs)		(xs)->sc_link->adapter_softc
#define	XS_LUN(xs)		((int) (xs)->sc_link->scsipi_scsi.lun)
#define	XS_TGT(xs)		((int) (xs)->sc_link->scsipi_scsi.target)
#define	XS_CDBP(xs)		((caddr_t) (xs)->cmd)
#define	XS_CDBLEN(xs)		(xs)->cmdlen
#define	XS_XFRLEN(xs)		(xs)->datalen
#define	XS_TIME(xs)		(xs)->timeout
#define	XS_RESID(xs)		(xs)->resid
#define	XS_STSP(xs)		(&(xs)->status)
#define	XS_SNSP(xs)		(&(xs)->sense.scsi_sense)
#define	XS_SNSLEN(xs)		(sizeof (xs)->sense)
#define	XS_SNSKEY(xs)		((xs)->sense.scsi_sense.flags)
#define	XS_TAG_P(ccb)		(((xs)->xs_control & XS_CTL_POLL) != 0)
#define	XS_TAG_TYPE(xs)	\
	(((xs)->xs_control & XS_CTL_URGENT) ? REQFLAG_HTAG : REQFLAG_OTAG)


#define	XS_SETERR(xs, v)	(xs)->error = v

#	define	HBA_NOERROR		XS_NOERROR
#	define	HBA_BOTCH		XS_DRIVER_STUFFUP
#	define	HBA_CMDTIMEOUT		XS_TIMEOUT
#	define	HBA_SELTIMEOUT		XS_SELTIMEOUT
#	define	HBA_TGTBSY		XS_BUSY
#	define	HBA_BUSRESET		XS_RESET
#	define	HBA_ABORTED		XS_DRIVER_STUFFUP
#	define	HBA_DATAOVR		XS_DRIVER_STUFFUP
#	define	HBA_ARQFAIL		XS_DRIVER_STUFFUP

#define	XS_ERR(xs)		(xs)->error

#define	XS_NOERR(xs)		(xs)->error == XS_NOERROR

#define	XS_INITERR(xs)		(xs)->error = 0, XS_CMD_S_CLEAR(xs)

#define	XS_SAVE_SENSE(xs, sp)				\
	bcopy(sp->req_sense_data, &(xs)->sense,		\
	    imin(XS_SNSLEN(xs), sp->req_sense_len))

#define	XS_SET_STATE_STAT(a, b, c)

#define	DEFAULT_IID(x)		7
#define	DEFAULT_LOOPID(x)	108
#define	DEFAULT_NODEWWN(isp)	(isp)->isp_osinfo.un._wwn
#define	DEFAULT_PORTWWN(isp)	\
	isp_port_from_node_wwn((isp), DEFAULT_NODEWWN(isp))
#define	PORT_FROM_NODE_WWN	isp_port_from_node_wwn

#define	ISP_UNSWIZZLE_AND_COPY_PDBP(isp, dest, src)	\
	if((void *)src != (void *)dest) bcopy(src, dest, sizeof (isp_pdb_t))
#define	ISP_SWIZZLE_ICB(a, b)
#ifdef	__sparc__
#define ISP_SWIZZLE_REQUEST(a, b)			\
	ISP_SBUSIFY_ISPHDR(a, &(b)->req_header);	\
        ISP_SBUSIFY_ISPREQ(a, b)
#define ISP_UNSWIZZLE_RESPONSE(a, b, c)			\
	ISP_SBUSIFY_ISPHDR(a, &(b)->req_header)
#else
#define	ISP_SWIZZLE_REQUEST(a, b)
#define	ISP_UNSWIZZLE_RESPONSE(a, b, c)
#endif
#define	ISP_SWIZZLE_SNS_REQ(a, b)
#define	ISP_UNSWIZZLE_SNS_RSP(a, b, c)
#ifdef	__sparc__
#define	ISP_SWIZZLE_NVRAM_WORD(isp, rp)	\
	{								\
		u_int16_t tmp = *rp >> 8;				\
		tmp |= ((*rp & 0xff) << 8);				\
		*rp = tmp;						\
	}
#else
#define	ISP_SWIZZLE_NVRAM_WORD(isp, rp)
#endif

/*
 * Includes of common header files
 */

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/ic/ispmbox.h>

/*
 * isp_osinfo definitions, extensions and shorthand.
 */
#define	isp_name	isp_osinfo._dev.dv_xname
#define	isp_unit	isp_osinfo._dev.dv_unit

/*
 * Driver prototypes..
 */
void isp_attach __P((struct ispsoftc *));
void isp_uninit __P((struct ispsoftc *));

static inline void isp_lock __P((struct ispsoftc *));
static inline void isp_unlock __P((struct ispsoftc *));
static inline char *strncat __P((char *, const char *, size_t));
static inline u_int64_t
isp_port_from_node_wwn __P((struct ispsoftc *, u_int64_t));
static inline u_int64_t
isp_microtime_sub __P((struct timeval *, struct timeval *));
static void isp_wait_complete __P((struct ispsoftc *));

/*
 * Driver wide data...
 */

/*
 * Locking macros...
 */
#define	ISP_LOCK		isp_lock
#define	ISP_UNLOCK		isp_unlock

/*              
 * Platform private flags                                               
 */

#define	XS_PSTS_INWDOG		0x10000000
#define	XS_PSTS_GRACE		0x20000000
#define	XS_PSTS_ALL		0x30000000

#define	XS_CMD_S_WDOG(xs)	(xs)->xs_status |= XS_PSTS_INWDOG
#define	XS_CMD_C_WDOG(xs)	(xs)->xs_status &= ~XS_PSTS_INWDOG
#define	XS_CMD_WDOG_P(xs)	(((xs)->xs_status & XS_PSTS_INWDOG) != 0)

#define	XS_CMD_S_GRACE(xs)	(xs)->xs_status |= XS_PSTS_GRACE
#define	XS_CMD_C_GRACE(xs)	(xs)->xs_status &= ~XS_PSTS_GRACE
#define	XS_CMD_GRACE_P(xs)	(((xs)->xs_status & XS_PSTS_GRACE) != 0)

#define	XS_CMD_S_DONE(xs)	(xs)->xs_status |= XS_STS_DONE
#define	XS_CMD_C_DONE(xs)	(xs)->xs_status &= ~XS_STS_DONE
#define	XS_CMD_DONE_P(xs)	(((xs)->xs_status & XS_STS_DONE) != 0)

#define	XS_CMD_S_CLEAR(xs)	(xs)->xs_status &= ~XS_PSTS_ALL

/*
 * Platform specific 'inline' or support functions
 */
static inline void
isp_lock(isp)
	struct ispsoftc *isp;
{
	int s = splbio();
	if (isp->isp_osinfo.islocked == 0) {
		isp->isp_osinfo.islocked = 1;
		isp->isp_osinfo.splsaved = s;
	} else {
		splx(s);
	}
}

static inline void
isp_unlock(isp)
	struct ispsoftc *isp;
{
	if (isp->isp_osinfo.islocked) {
		isp->isp_osinfo.islocked = 0;
		splx(isp->isp_osinfo.splsaved);
	}
}

static inline char *
strncat(d, s, c)
	char *d;
	const char *s;
	size_t c;
{
        char *t = d;

        if (c) {
                while (*d)
                        d++;
                while ((*d++ = *s++)) {
                        if (--c == 0) {
                                *d = '\0';
                                break;
                        }
                }
        }
        return (t);
}

static inline u_int64_t
isp_microtime_sub(b, a)
	struct timeval *b;
	struct timeval *a;
{
	struct timeval x;
	u_int64_t elapsed;
	timersub(b, a, &x);
	elapsed = GET_NANOSEC(&x);
	if (elapsed == 0)
		elapsed++;
	return (elapsed);
}

static inline void
isp_wait_complete(isp)
	struct ispsoftc *isp;
{
	if (isp->isp_osinfo.onintstack || isp->isp_osinfo.no_mbox_ints) {
		int usecs = 0;
		while (usecs < 2 * 1000000) {
			(void) isp_intr(isp);
			if (isp->isp_mboxbsy == 0) {
				break;
			}
			USEC_DELAY(500);
			usecs += 500;
		}
		if (isp->isp_mboxbsy != 0) {
			isp_prt(isp, ISP_LOGWARN, "Mailbox Cmd (poll) Timeout");
		}
	} else {
		int rv = 0;
                isp->isp_osinfo.mboxwaiting = 1;
                while (isp->isp_osinfo.mboxwaiting && rv == 0) {
			static struct timeval twosec = { 2, 0 };
			int timo;
			struct timeval tv;
			microtime(&tv);
			timeradd(&tv, &twosec, &tv);
			if ((timo = hzto(&tv)) == 0) {
				timo = 1;
			}
			rv = tsleep(&isp->isp_osinfo.mboxwaiting,
			    PRIBIO, "isp_mboxcmd", timo);
		}
		if (rv == EWOULDBLOCK) {
			isp->isp_mboxbsy = 0;
			isp->isp_osinfo.mboxwaiting = 0;
			isp_prt(isp, ISP_LOGWARN, "Mailbox Cmd (intr) Timeout");
		}
	}
}

static inline u_int64_t
isp_port_from_node_wwn(struct ispsoftc *isp, u_int64_t node_wwn)
{
	u_int64_t rv = node_wwn;
	if ((node_wwn >> 60) == 2) {
		rv = node_wwn | 
		    (((u_int64_t)(isp->isp_unit+1)) << 48);
	}
	return (rv);
}

/*
 * Common inline functions
 */
#define	INLINE	inline
#include <dev/ic/isp_inline.h>

#if	!defined(ISP_DISABLE_FW) && !defined(ISP_COMPILE_FW)
#define	ISP_COMPILE_FW	1
#endif
#endif	/* _ISP_NETBSD_H */
