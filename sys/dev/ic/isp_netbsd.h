/* $NetBSD: isp_netbsd.h,v 1.48 2002/02/21 22:32:42 mjacob Exp $ */
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
#include <sys/kthread.h>

#include <machine/bus.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_debug.h>

#include "opt_isp.h"

/*
 * Efficiency- get rid of SBus code && tests unless we need them.
 */
#if	defined(__sparcv9__ ) || defined(__sparc__)
#define	ISP_SBUS_SUPPORTED	1
#else
#define	ISP_SBUS_SUPPORTED	0
#endif

#define	ISP_PLATFORM_VERSION_MAJOR	2
#define	ISP_PLATFORM_VERSION_MINOR	1

struct isposinfo {
	struct device		_dev;
	struct scsipi_channel	_chan;
	struct scsipi_channel	_chan_b;
	struct scsipi_adapter   _adapter;
	bus_dma_tag_t		dmatag;
	bus_dmamap_t		rqdmap;
	bus_dmamap_t		rsdmap;
	bus_dmamap_t		scdmap;	/* FC only */
	int			splsaved;
	int			mboxwaiting;
	u_int32_t		islocked;
	u_int32_t		onintstack;
	unsigned int		: 26,
		loop_checked	: 1,
		mbox_wanted	: 1,
		mbox_locked	: 1,
		no_mbox_ints	: 1,
		paused		: 1,
		blocked		: 1;
	union {
		u_int64_t	_wwn;
		u_int16_t	_discovered[2];
#define	wwn_seed	un._wwn
#define	discovered	un._discovered
	} un;
	u_int32_t		threadwork;
	struct proc *		thread;
};
#define	isp_dmatag		isp_osinfo.dmatag
#define	isp_rqdmap		isp_osinfo.rqdmap
#define	isp_rsdmap		isp_osinfo.rsdmap
#define	isp_scdmap		isp_osinfo.scdmap

#define	ISP_MUSTPOLL(isp)	\
	(isp->isp_osinfo.onintstack || isp->isp_osinfo.no_mbox_ints)

#define	HANDLE_LOOPSTATE_IN_OUTER_LAYERS	1

/*
 * Required Macros/Defines
 */

#define	INLINE			__inline

#define	ISP2100_SCRLEN		0x400

#define	MEMZERO(dst, amt)	memset((dst), 0, (amt))
#define	MEMCPY(dst, src, amt)	memcpy((dst), (src), (amt))
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


#define	MEMORYBARRIER(isp, type, offset, size)			\
switch (type) {							\
case SYNC_REQUEST:						\
{								\
	off_t off = (off_t) offset * QENTRY_LEN;		\
	bus_dmamap_sync(isp->isp_dmatag, isp->isp_rqdmap,	\
	    off, size, BUS_DMASYNC_PREWRITE);			\
	break;							\
}								\
case SYNC_RESULT:						\
{								\
	off_t off = (off_t) offset * QENTRY_LEN;		\
	off += ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN(isp));		\
	bus_dmamap_sync(isp->isp_dmatag, isp->isp_rsdmap,	\
	    off, size, BUS_DMASYNC_POSTREAD);			\
	break;							\
}								\
case SYNC_SFORDEV:						\
{								\
	off_t off =						\
	    ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN(isp)) +		\
	    ISP_QUEUE_SIZE(RESULT_QUEUE_LEN(isp)) + offset;	\
	bus_dmamap_sync(isp->isp_dmatag, isp->isp_scdmap,	\
	    off, size, BUS_DMASYNC_PREWRITE);			\
	break;							\
}								\
case SYNC_SFORCPU:						\
{								\
	off_t off =						\
	    ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN(isp)) +		\
	    ISP_QUEUE_SIZE(RESULT_QUEUE_LEN(isp)) + offset;	\
	bus_dmamap_sync(isp->isp_dmatag, isp->isp_scdmap,	\
	    off, size, BUS_DMASYNC_POSTREAD);			\
	break;							\
}								\
case SYNC_REG:							\
default:							\
	break;							\
}

#define	MBOX_ACQUIRE(isp)						\
	if (isp->isp_osinfo.onintstack) {				\
		if (isp->isp_osinfo.mbox_locked) {			\
			mbp->param[0] = MBOX_HOST_INTERFACE_ERROR;	\
			isp_prt(isp, ISP_LOGERR, "cannot acquire MBOX lock"); \
			return;						\
		}							\
	} else {							\
		while (isp->isp_osinfo.mbox_locked) {			\
			isp->isp_osinfo.mbox_wanted = 1;		\
			(void) tsleep(&isp->isp_osinfo.mboxwaiting+1,	\
			    PRIBIO, "mbox_acquire", 0);			\
		}							\
	}								\
	isp->isp_osinfo.mbox_locked = 1

#define	MBOX_WAIT_COMPLETE	isp_wait_complete

#define	MBOX_NOTIFY_COMPLETE(isp)					\
	if (isp->isp_osinfo.mboxwaiting) {				\
                isp->isp_osinfo.mboxwaiting = 0;			\
                wakeup(&isp->isp_osinfo.mboxwaiting);			\
        }								\
	isp->isp_mboxbsy = 0

#define	MBOX_RELEASE(isp)						\
	if (isp->isp_osinfo.mbox_wanted) {				\
		isp->isp_osinfo.mbox_wanted = 0;			\
		wakeup(&isp->isp_osinfo.mboxwaiting+1);			\
	}								\
	isp->isp_osinfo.mbox_locked = 0

#define	FC_SCRATCH_ACQUIRE(isp)
#define	FC_SCRATCH_RELEASE(isp)

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
#define	XS_CHANNEL(xs)		((int) (xs)->xs_periph->periph_channel->chan_channel)
#define	XS_ISP(xs)		\
	((void *)(xs)->xs_periph->periph_channel->chan_adapter->adapt_dev)
#define	XS_LUN(xs)		((int) (xs)->xs_periph->periph_lun)
#define	XS_TGT(xs)		((int) (xs)->xs_periph->periph_target)
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
	if (xs->error == XS_NOERROR) {			\
		xs->error = XS_SENSE;			\
	}						\
	memcpy(&(xs)->sense, sp->req_sense_data,	\
	    imin(XS_SNSLEN(xs), sp->req_sense_len))

#define	XS_SET_STATE_STAT(a, b, c)

#define	DEFAULT_IID(x)		7
#define	DEFAULT_LOOPID(x)	108
#define	DEFAULT_NODEWWN(isp)	(isp)->isp_osinfo.un._wwn
#define	DEFAULT_PORTWWN(isp)	(isp)->isp_osinfo.un._wwn
#define	ISP_NODEWWN(isp)	FCPARAM(isp)->isp_nodewwn
#define	ISP_PORTWWN(isp)	FCPARAM(isp)->isp_portwwn

#if	_BYTE_ORDER == _BIG_ENDIAN
#ifdef	ISP_SBUS_SUPPORTED
#define	ISP_IOXPUT_8(isp, s, d)		*(d) = s
#define	ISP_IOXPUT_16(isp, s, d)				\
	*(d) = (isp->isp_bustype == ISP_BT_SBUS)? s : bswap16(s)
#define	ISP_IOXPUT_32(isp, s, d)				\
	*(d) = (isp->isp_bustype == ISP_BT_SBUS)? s : bswap32(s)

#define	ISP_IOXGET_8(isp, s, d)		d = (*((u_int8_t *)s))
#define	ISP_IOXGET_16(isp, s, d)				\
	d = (isp->isp_bustype == ISP_BT_SBUS)?			\
	*((u_int16_t *)s) : bswap16(*((u_int16_t *)s))
#define	ISP_IOXGET_32(isp, s, d)				\
	d = (isp->isp_bustype == ISP_BT_SBUS)?			\
	*((u_int32_t *)s) : bswap32(*((u_int32_t *)s))
#else
#define	ISP_IOXPUT_8(isp, s, d)		*(d) = s
#define	ISP_IOXPUT_16(isp, s, d)	*(d) = bswap16(s)
#define	ISP_IOXPUT_32(isp, s, d)	*(d) = bswap32(s)
#define	ISP_IOXGET_8(isp, s, d)		d = (*((u_int8_t *)s))
#define	ISP_IOXGET_16(isp, s, d)	d = bswap16(*((u_int16_t *)s))
#define	ISP_IOXGET_32(isp, s, d)	d = bswap32(*((u_int32_t *)s))
#endif
#define	ISP_SWIZZLE_NVRAM_WORD(isp, rp)	*rp = bswap16(*rp)
#else
#define	ISP_IOXPUT_8(isp, s, d)		*(d) = s
#define	ISP_IOXPUT_16(isp, s, d)	*(d) = s
#define	ISP_IOXPUT_32(isp, s, d)	*(d) = s
#define	ISP_IOXGET_8(isp, s, d)		d = *(s)
#define	ISP_IOXGET_16(isp, s, d)	d = *(s)
#define	ISP_IOXGET_32(isp, s, d)	d = *(s)
#define	ISP_SWIZZLE_NVRAM_WORD(isp, rp)
#endif

/*
 * Includes of common header files
 */

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/ic/ispmbox.h>
#include <dev/ic/isp_ioctl.h>

/*
 * isp_osinfo definitions, extensions and shorthand.
 */
#define	isp_name	isp_osinfo._dev.dv_xname
#define	isp_unit	isp_osinfo._dev.dv_unit
#define	isp_chanA	isp_osinfo._chan
#define	isp_chanB	isp_osinfo._chan_b


/*
 * Driver prototypes..
 */
void isp_attach(struct ispsoftc *);
void isp_uninit(struct ispsoftc *);

static INLINE void isp_lock(struct ispsoftc *);
static INLINE void isp_unlock(struct ispsoftc *);
static INLINE char *strncat(char *, const char *, size_t);
static INLINE u_int64_t
isp_microtime_sub(struct timeval *, struct timeval *);
static void isp_wait_complete(struct ispsoftc *);

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
static INLINE void
isp_lock(struct ispsoftc *isp)
{
	int s = splbio();
	if (isp->isp_osinfo.islocked++ == 0) {
		isp->isp_osinfo.splsaved = s;
	} else {
		splx(s);
	}
}

static INLINE void
isp_unlock(struct ispsoftc *isp)
{
	if (isp->isp_osinfo.islocked-- <= 1) {
		isp->isp_osinfo.islocked = 0;
		splx(isp->isp_osinfo.splsaved);
	}
}

static INLINE char *
strncat(char *d, const char *s, size_t c)
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

static INLINE u_int64_t
isp_microtime_sub(struct timeval *b, struct timeval *a)
{
	struct timeval x;
	u_int64_t elapsed;
	timersub(b, a, &x);
	elapsed = GET_NANOSEC(&x);
	if (elapsed == 0)
		elapsed++;
	return (elapsed);
}

static INLINE void
isp_wait_complete(struct ispsoftc *isp)
{
	int lim;
	if (isp->isp_mbxwrk0)
		lim = 60;
	else
		lim = 5;
	if (isp->isp_osinfo.onintstack || isp->isp_osinfo.no_mbox_ints) {
		int useclim = 1000000 * lim, usecs = 0;
		/*
		 * For sanity's sake, we don't delay longer
		 * than 5 seconds for polled commands.
		 */
		while (usecs < useclim) {
			u_int16_t isr, sema, mbox;
			if (isp->isp_mboxbsy == 0) {
				break;
			}
			if (ISP_READ_ISR(isp, &isr, &sema, &mbox)) {
				isp_intr(isp, isr, sema, mbox);
				if (isp->isp_mboxbsy == 0) {
					break;
				}
			}
			USEC_DELAY(500);
			usecs += 500;
		}
		if (isp->isp_mboxbsy != 0) {
			isp_prt(isp, ISP_LOGWARN,
			    "Polled Mailbox Command (0x%x) Timeout",
			    isp->isp_lastmbxcmd);
		}
	} else {
		int rv = 0;
                isp->isp_osinfo.mboxwaiting = 1;
                while (isp->isp_osinfo.mboxwaiting && rv == 0) {
			struct timeval dtime;
			int timo;
			struct timeval tv;

			dtime.tv_sec = lim;
			dtime.tv_usec = 0;
			microtime(&tv);
			timeradd(&tv, &dtime, &tv);
			if ((timo = hzto(&tv)) == 0) {
				timo = 1;
			}
			rv = tsleep(&isp->isp_osinfo.mboxwaiting,
			    PRIBIO, "isp_mboxcmd", timo);
		}
		if (rv == EWOULDBLOCK) {
			isp->isp_mboxbsy = 0;
			isp->isp_osinfo.mboxwaiting = 0;
			isp_prt(isp, ISP_LOGWARN,
			    "Interrupting Mailbox Command (0x%x) Timeout",
			    isp->isp_lastmbxcmd);
		}
	}
}


/*
 * Common inline functions
 */
#include <dev/ic/isp_inline.h>

#if	!defined(ISP_DISABLE_FW) && !defined(ISP_COMPILE_FW)
#define	ISP_COMPILE_FW	1
#endif
#endif	/* _ISP_NETBSD_H */
