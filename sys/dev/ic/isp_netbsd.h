/* $NetBSD: isp_netbsd.h,v 1.18.2.8 2000/12/13 15:50:03 bouyer Exp $ */
/*
 * This driver, which is contained in NetBSD in the files:
 *
 *	sys/dev/ic/isp.c
 *	sys/dev/ic/ic/isp.c
 *	sys/dev/ic/ic/isp_inline.h
 *	sys/dev/ic/ic/isp_netbsd.c
 *	sys/dev/ic/ic/isp_netbsd.h
 *	sys/dev/ic/ic/isp_target.c
 *	sys/dev/ic/ic/isp_target.h
 *	sys/dev/ic/ic/isp_tpublic.h
 *	sys/dev/ic/ic/ispmbox.h
 *	sys/dev/ic/ic/ispreg.h
 *	sys/dev/ic/ic/ispvar.h
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
	u_int32_t	islocked;
	u_int32_t	onintstack;
	unsigned int		: 30,
		no_mbox_ints	: 1,
		blocked		: 1;
	union {
		u_int64_t	_wwn;
		u_int16_t	_discovered[2];
#define	wwn_seed	un._wwn
#define	discovered	un._discovered
	} un;
	TAILQ_HEAD(, scsipi_xfer) waitq; 
	struct callout _restart;
};
#define	ISP_MUSTPOLL(isp)	\
	(isp->isp_osinfo.onintstack || isp->isp_osinfo.no_mbox_ints)

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
#define	USEC_SLEEP(isp, x)		\
	if (!ISP_MUSTPOLL(isp))		\
		ISP_UNLOCK(isp);	\
	DELAY(x);			\
	if (!ISP_MUSTPOLL(isp))		\
		ISP_LOCK(isp)

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
	(((xs)->xs_control & XS_CTL_URGENT) ? REQFLAG_HTAG : \
	((xs)->bp && xs->bp->b_flags & B_ASYNC) ? REQFLAG_STAG : REQFLAG_OTAG)

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
#define	DEFAULT_PORTWWN(isp)	(isp)->isp_osinfo.un._wwn
#define	ISP_NODEWWN(isp)	FCPARAM(isp)->isp_nodewwn
#define	ISP_PORTWWN(isp)	FCPARAM(isp)->isp_portwwn

#if	_BYTE_ORDER == _BIG_ENDIAN
#define	ISP_SWIZZLE_REQUEST		isp_swizzle_request
#define	ISP_UNSWIZZLE_RESPONSE		isp_unswizzle_response
#define	ISP_SWIZZLE_ICB			isp_swizzle_icb
#define	ISP_UNSWIZZLE_AND_COPY_PDBP	isp_unswizzle_and_copy_pdbp
#define	ISP_SWIZZLE_SNS_REQ		isp_swizzle_sns_req
#define	ISP_UNSWIZZLE_SNS_RSP		isp_unswizzle_sns_rsp
#define	ISP_SWIZZLE_NVRAM_WORD(isp, rp)	*rp = bswap16(*rp)
#else
#define	ISP_SWIZZLE_REQUEST(a, b)
#define	ISP_UNSWIZZLE_RESPONSE(a, b, c)
#define	ISP_SWIZZLE_ICB(a, b)
#define	ISP_UNSWIZZLE_AND_COPY_PDBP(isp, dest, src)	\
	if((void *)src != (void *)dest) bcopy(src, dest, sizeof (isp_pdb_t))
#define	ISP_SWIZZLE_SNS_REQ(a, b)
#define	ISP_UNSWIZZLE_SNS_RSP(a, b, c)
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
isp_microtime_sub __P((struct timeval *, struct timeval *));
static void isp_wait_complete __P((struct ispsoftc *));
#if	_BYTE_ORDER == _BIG_ENDIAN
static inline void isp_swizzle_request(struct ispsoftc *, ispreq_t *);
static inline void isp_unswizzle_response(struct ispsoftc *, void *, u_int16_t);
static inline void isp_swizzle_icb(struct ispsoftc *, isp_icb_t *);
static inline void
isp_unswizzle_and_copy_pdbp(struct ispsoftc *, isp_pdb_t *, void *);
static inline void isp_swizzle_sns_req(struct ispsoftc *, sns_screq_t *);
static inline void isp_unswizzle_sns_rsp(struct ispsoftc *, sns_scrsp_t *, int);
#endif

/*
 * Driver wide data...
 */

/*
 * Locking macros...
 */
#define	ISP_LOCK		isp_lock
#define	ISP_UNLOCK		isp_unlock
#define	ISP_ILOCK(x)		isp_lock(x); isp->isp_osinfo.onintstack++
#define	ISP_IUNLOCK(x)		isp->isp_osinfo.onintstack--; isp_unlock(x)

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
	if (isp->isp_osinfo.islocked++ == 0) {
		isp->isp_osinfo.splsaved = s;
	} else {
		splx(s);
	}
}

static inline void
isp_unlock(isp)
	struct ispsoftc *isp;
{
	if (isp->isp_osinfo.islocked-- <= 1) {
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

#if	_BYTE_ORDER == _BIG_ENDIAN
static inline void
isp_swizzle_request(struct ispsoftc *isp, ispreq_t *rq)
{
	if (IS_FC(isp)) {
		ispreqt2_t *tq = (ispreqt2_t *) rq;
		tq->req_handle = bswap32(tq->req_handle);
		tq->req_scclun = bswap16(tq->req_scclun);
		tq->req_flags = bswap16(tq->req_flags);
		tq->req_time = bswap16(tq->req_time);
		tq->req_totalcnt = bswap32(tq->req_totalcnt);
	} else if (isp->isp_bustype == ISP_BT_SBUS) {
		_ISP_SWAP8(rq->req_header.rqs_entry_count,
		    rq->req_header.rqs_entry_type);
		_ISP_SWAP8(rq->req_header.rqs_flags, rq->req_header.rqs_seqno);
		_ISP_SWAP8(rq->req_target, rq->req_lun_trn);
	} else {
		rq->req_handle = bswap32(rq->req_handle);
		rq->req_cdblen = bswap16(rq->req_cdblen);
		rq->req_flags = bswap16(rq->req_flags);
		rq->req_time = bswap16(rq->req_time);
	}
}

static inline void
isp_unswizzle_response(struct ispsoftc *isp, void *rp, u_int16_t optr)
{
	ispstatusreq_t *sp = rp;
	MEMORYBARRIER(isp, SYNC_RESPONSE, optr * QENTRY_LEN, QENTRY_LEN);
	if (isp->isp_bustype == ISP_BT_SBUS) {
		_ISP_SWAP8(sp->req_header.rqs_entry_count,
		    sp->req_header.rqs_entry_type);
		_ISP_SWAP8(sp->req_header.rqs_flags, sp->req_header.rqs_seqno);
	} else switch (sp->req_header.rqs_entry_type) {
	case RQSTYPE_RESPONSE:
		sp->req_handle = bswap32(sp->req_handle);
		sp->req_scsi_status = bswap16(sp->req_scsi_status);
		sp->req_completion_status = bswap16(sp->req_completion_status);
		sp->req_state_flags = bswap16(sp->req_state_flags);
		sp->req_status_flags = bswap16(sp->req_status_flags);
		sp->req_time = bswap16(sp->req_time);
		sp->req_sense_len = bswap16(sp->req_sense_len);
		sp->req_resid = bswap32(sp->req_resid);
		break;
	default:
		break;
	}
}

static inline void
isp_swizzle_icb(struct ispsoftc *isp, isp_icb_t *icbp)
{
	_ISP_SWAP8(icbp->icb_version, icbp->_reserved0);
	_ISP_SWAP8(icbp->icb_retry_count, icbp->icb_retry_delay);
	_ISP_SWAP8(icbp->icb_iqdevtype, icbp->icb_logintime);
	_ISP_SWAP8(icbp->icb_ccnt, icbp->icb_icnt);
	_ISP_SWAP8(icbp->icb_racctimer, icbp->icb_idelaytimer);
	icbp->icb_fwoptions = bswap16(icbp->icb_fwoptions);
	icbp->icb_maxfrmlen = bswap16(icbp->icb_maxfrmlen);
	icbp->icb_maxalloc = bswap16(icbp->icb_maxalloc);
	icbp->icb_execthrottle = bswap16(icbp->icb_execthrottle);
	icbp->icb_hardaddr = bswap16(icbp->icb_hardaddr);
	icbp->icb_rqstout = bswap16(icbp->icb_rqstout);
	icbp->icb_rspnsin = bswap16(icbp->icb_rspnsin);
	icbp->icb_rqstqlen = bswap16(icbp->icb_rqstqlen);
	icbp->icb_rsltqlen = bswap16(icbp->icb_rsltqlen);
	icbp->icb_lunenables = bswap16(icbp->icb_lunenables);
	icbp->icb_lunetimeout = bswap16(icbp->icb_lunetimeout);
	icbp->icb_xfwoptions = bswap16(icbp->icb_xfwoptions);
	icbp->icb_zfwoptions = bswap16(icbp->icb_zfwoptions);
	icbp->icb_rqstaddr[0] = bswap16(icbp->icb_rqstaddr[0]);
	icbp->icb_rqstaddr[1] = bswap16(icbp->icb_rqstaddr[1]);
	icbp->icb_rqstaddr[2] = bswap16(icbp->icb_rqstaddr[2]);
	icbp->icb_rqstaddr[3] = bswap16(icbp->icb_rqstaddr[3]);
	icbp->icb_respaddr[0] = bswap16(icbp->icb_respaddr[0]);
	icbp->icb_respaddr[1] = bswap16(icbp->icb_respaddr[1]);
	icbp->icb_respaddr[2] = bswap16(icbp->icb_respaddr[2]);
	icbp->icb_respaddr[3] = bswap16(icbp->icb_respaddr[3]);
}

static inline void
isp_unswizzle_and_copy_pdbp(struct ispsoftc *isp, isp_pdb_t *dst, void *src)
{
	isp_pdb_t *pdbp = src;
	dst->pdb_options = bswap16(pdbp->pdb_options);
	dst->pdb_mstate = pdbp->pdb_sstate;
	dst->pdb_sstate = pdbp->pdb_mstate;
	dst->pdb_hardaddr_bits[0] = pdbp->pdb_hardaddr_bits[0];
	dst->pdb_hardaddr_bits[1] = pdbp->pdb_hardaddr_bits[1];
	dst->pdb_hardaddr_bits[2] = pdbp->pdb_hardaddr_bits[2];
	dst->pdb_hardaddr_bits[3] = pdbp->pdb_hardaddr_bits[3];
	dst->pdb_portid_bits[0] = pdbp->pdb_portid_bits[0];
	dst->pdb_portid_bits[1] = pdbp->pdb_portid_bits[1];
	dst->pdb_portid_bits[2] = pdbp->pdb_portid_bits[2];
	dst->pdb_portid_bits[3] = pdbp->pdb_portid_bits[3];
	dst->pdb_nodename[0] = pdbp->pdb_nodename[0];
	dst->pdb_nodename[1] = pdbp->pdb_nodename[1];
	dst->pdb_nodename[2] = pdbp->pdb_nodename[2];
	dst->pdb_nodename[3] = pdbp->pdb_nodename[3];
	dst->pdb_nodename[4] = pdbp->pdb_nodename[4];
	dst->pdb_nodename[5] = pdbp->pdb_nodename[5];
	dst->pdb_nodename[6] = pdbp->pdb_nodename[6];
	dst->pdb_nodename[7] = pdbp->pdb_nodename[7];
	dst->pdb_portname[0] = pdbp->pdb_portname[0];
	dst->pdb_portname[1] = pdbp->pdb_portname[1];
	dst->pdb_portname[2] = pdbp->pdb_portname[2];
	dst->pdb_portname[3] = pdbp->pdb_portname[3];
	dst->pdb_portname[4] = pdbp->pdb_portname[4];
	dst->pdb_portname[5] = pdbp->pdb_portname[5];
	dst->pdb_portname[6] = pdbp->pdb_portname[6];
	dst->pdb_portname[7] = pdbp->pdb_portname[7];
	dst->pdb_execthrottle = bswap16(pdbp->pdb_execthrottle);
	dst->pdb_exec_count = bswap16(pdbp->pdb_exec_count);
	dst->pdb_resalloc = bswap16(pdbp->pdb_resalloc);
	dst->pdb_curalloc = bswap16(pdbp->pdb_curalloc);
	dst->pdb_qhead = bswap16(pdbp->pdb_qhead);
	dst->pdb_qtail = bswap16(pdbp->pdb_qtail);
	dst->pdb_tl_next = bswap16(pdbp->pdb_tl_next);
	dst->pdb_tl_last = bswap16(pdbp->pdb_tl_last);
	dst->pdb_features = bswap16(pdbp->pdb_features);
	dst->pdb_pconcurrnt = bswap16(pdbp->pdb_pconcurrnt);
	dst->pdb_roi = bswap16(pdbp->pdb_roi);
	dst->pdb_rdsiz = bswap16(pdbp->pdb_rdsiz);
	dst->pdb_ncseq = bswap16(pdbp->pdb_ncseq);
	dst->pdb_noseq = bswap16(pdbp->pdb_noseq);
	dst->pdb_labrtflg = bswap16(pdbp->pdb_labrtflg);
	dst->pdb_lstopflg = bswap16(pdbp->pdb_lstopflg);
	dst->pdb_sqhead = bswap16(pdbp->pdb_sqhead);
	dst->pdb_sqtail = bswap16(pdbp->pdb_sqtail);
	dst->pdb_ptimer = bswap16(pdbp->pdb_ptimer);
	dst->pdb_nxt_seqid = bswap16(pdbp->pdb_nxt_seqid);
	dst->pdb_fcount = bswap16(pdbp->pdb_fcount);
	dst->pdb_prli_len = bswap16(pdbp->pdb_prli_len);
	dst->pdb_prli_svc0 = bswap16(pdbp->pdb_prli_svc0);
	dst->pdb_prli_svc3 = bswap16(pdbp->pdb_prli_svc3);
	dst->pdb_loopid = bswap16(pdbp->pdb_loopid);
	dst->pdb_il_ptr = bswap16(pdbp->pdb_il_ptr);
	dst->pdb_sl_ptr = bswap16(pdbp->pdb_sl_ptr);
	dst->pdb_retry_count = pdbp->pdb_retry_delay;
	dst->pdb_retry_delay = pdbp->pdb_retry_count;
	dst->pdb_target = pdbp->pdb_initiator;
	dst->pdb_initiator = pdbp->pdb_target;
}

static inline void
isp_swizzle_sns_req(struct ispsoftc *isp, sns_screq_t *reqp)
{
	u_int16_t index, nwords = reqp->snscb_sblen;
	reqp->snscb_rblen = bswap16(reqp->snscb_rblen);
	reqp->snscb_addr[0] = bswap16(reqp->snscb_addr[0]);
	reqp->snscb_addr[1] = bswap16(reqp->snscb_addr[1]);
	reqp->snscb_addr[2] = bswap16(reqp->snscb_addr[2]);
	reqp->snscb_addr[3] = bswap16(reqp->snscb_addr[3]);
	reqp->snscb_sblen = bswap16(reqp->snscb_sblen);
	for (index = 0; index < nwords; index++) {
		reqp->snscb_data[index] = bswap16(reqp->snscb_data[index]);
	}
}

static inline void
isp_unswizzle_sns_rsp(struct ispsoftc *isp, sns_scrsp_t *resp, int nwords)
{
	int index;
	for (index = 0; index < nwords; index++) {
		resp->snscb_data[index] = bswap16(resp->snscb_data[index]);
	}
}
#endif

/*
 * Common inline functions
 */
#define	INLINE	inline
#include <dev/ic/isp_inline.h>

#if	!defined(ISP_DISABLE_FW) && !defined(ISP_COMPILE_FW)
#define	ISP_COMPILE_FW	1
#endif
#endif	/* _ISP_NETBSD_H */
