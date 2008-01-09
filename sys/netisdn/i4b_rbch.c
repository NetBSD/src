/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_rbch.c - device driver for raw B channel data
 *	---------------------------------------------------
 *
 *	$Id: i4b_rbch.c,v 1.21.8.1 2008/01/09 01:57:44 matt Exp $
 *
 * $FreeBSD$
 *
 *	last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_rbch.c,v 1.21.8.1 2008/01/09 01:57:44 matt Exp $");

#include "isdnbchan.h"

#if NISDNBCHAN > 0

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/proc.h>
#include <sys/tty.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#if defined (__NetBSD__) || defined (__OpenBSD__)
#define termioschars(t) memcpy((t)->c_cc, &ttydefchars, sizeof((t)->c_cc))
#endif

#ifdef __FreeBSD__

#if defined(__FreeBSD__) && __FreeBSD__ == 3
#include "opt_devfs.h"
#endif

#ifdef DEVFS
#include <sys/devfsext.h>
#endif

#endif /* __FreeBSD__ */

#ifdef __NetBSD__
#include <sys/filio.h>
#endif

#ifdef __FreeBSD__
#include <machine/i4b_ioctl.h>
#include <machine/i4b_rbch_ioctl.h>
#include <machine/i4b_debug.h>
#else
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_rbch_ioctl.h>
#include <netisdn/i4b_debug.h>
#endif

#include <netisdn/i4b_global.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_l3l4.h>

#include <netisdn/i4b_l4.h>

#ifdef __bsdi__
#include <sys/device.h>
#endif

#ifdef OS_USES_POLL
#include <sys/ioccom.h>
#include <sys/poll.h>
#else
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#endif

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/filio.h>
#endif

#define I4BRBCHACCT		1 	/* enable accounting messages */
#define	I4BRBCHACCTINTVL	2	/* accounting msg interval in secs */

static struct rbch_softc {

	int sc_unit;			/* unit number 		*/

	int sc_devstate;		/* state of driver	*/
#define ST_IDLE		0x00
#define ST_CONNECTED	0x01
#define ST_ISOPEN	0x02
#define ST_RDWAITDATA	0x04
#define ST_WRWAITEMPTY	0x08
#define ST_NOBLOCK	0x10

	int sc_bprot;			/* B-ch protocol used	*/

	call_desc_t *sc_cd;		/* Call Descriptor */
	isdn_link_t *sc_ilt;		/* B-channel driver/state */

	struct termios it_in;

	struct ifqueue sc_hdlcq;	/* hdlc read queue	*/
#define I4BRBCHMAXQLEN	10

	struct selinfo selp;		/* select / poll	*/

#if defined(__FreeBSD__) && __FreeBSD__ == 3
#ifdef DEVFS
	void *devfs_token;		/* device filesystem	*/
#endif
#endif

#if I4BRBCHACCT
#if defined(__FreeBSD__)
	struct callout_handle sc_callout;
#endif
#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
	struct callout	sc_callout;
#endif

	int		sc_iinb;	/* isdn driver # of inbytes	*/
	int		sc_ioutb;	/* isdn driver # of outbytes	*/
	int		sc_linb;	/* last # of bytes rx'd		*/
	int		sc_loutb;	/* last # of bytes tx'd 	*/
	int		sc_fn;		/* flag, first null acct	*/
#endif
} rbch_softc[NISDNBCHAN];

static void rbch_rx_data_rdy(void *softc);
static void rbch_tx_queue_empty(void *softc);
static void rbch_connect(void *softc, void *cdp);
static void rbch_disconnect(void *softc, void *cdp);
static void rbch_clrq(void *softc);
static void rbch_activity(void *softc, int rxtx);
static void rbch_dialresponse(void *softc, int status, cause_t cause);
static void rbch_updown(void *softc, int updown);
static void rbch_set_linktab(void *softc, isdn_link_t *ilt);
static void* rbch_get_softc(int unit);


#ifndef __FreeBSD__
#define PDEVSTATIC	/* - not static - */
#define IOCTL_CMD_T	u_long
void isdnbchanattach __P((void));
int isdnbchanopen __P((dev_t dev, int flag, int fmt, struct lwp *l));
int isdnbchanclose __P((dev_t dev, int flag, int fmt, struct lwp *l));
int isdnbchanread __P((dev_t dev, struct uio *uio, int ioflag));
int isdnbchanwrite __P((dev_t dev, struct uio *uio, int ioflag));
int isdnbchanioctl __P((dev_t dev, IOCTL_CMD_T cmd, void *arg, int flag, struct lwp* l));
#ifdef OS_USES_POLL
int isdnbchanpoll __P((dev_t dev, int events, struct lwp *l));
int isdnbchankqfilter __P((dev_t dev, struct knote *kn));
#else
PDEVSTATIC int isdnbchanselect __P((dev_t dev, int rw, struct lwp *l));
#endif
#endif

#ifdef __NetBSD__
const struct cdevsw isdnbchan_cdevsw = {
	isdnbchanopen, isdnbchanclose, isdnbchanread, isdnbchanwrite,
	isdnbchanioctl, nostop, notty, isdnbchanpoll, nommap, nokqfilter,
	D_OTHER
};
#endif /* __NetBSD__ */

#if BSD > 199306 && defined(__FreeBSD__)
#define PDEVSTATIC	static
#define IOCTL_CMD_T	u_long

PDEVSTATIC d_open_t isdnbchanopen;
PDEVSTATIC d_close_t isdnbchanclose;
PDEVSTATIC d_read_t isdnbchanread;
PDEVSTATIC d_read_t isdnbchanwrite;
PDEVSTATIC d_ioctl_t isdnbchanioctl;

#ifdef OS_USES_POLL
PDEVSTATIC d_poll_t isdnbchanpoll;
#define POLLFIELD	isdnbchanpoll
#else
PDEVSTATIC d_select_t isdnbchanselect;
#define POLLFIELD	isdnbchanselect
#endif

#define CDEV_MAJOR 57

#if defined(__FreeBSD__) && __FreeBSD__ >= 4
static struct cdevsw isdnbchan_cdevsw = {
	/* open */      isdnbchanopen,
	/* close */     isdnbchanclose,
	/* read */      isdnbchanread,
	/* write */     isdnbchanwrite,
	/* ioctl */     isdnbchanioctl,
	/* poll */      POLLFIELD,
	/* mmap */      nommap,
	/* strategy */  nostrategy,
	/* name */      "isdnbchan",
	/* maj */       CDEV_MAJOR,
	/* dump */      nodump,
	/* psize */     nopsize,
	/* flags */     0,
	/* bmaj */      -1
};
#else
static struct cdevsw isdnbchan_cdevsw = {
	isdnbchanopen,	isdnbchanclose,	isdnbchanread,	isdnbchanwrite,
  	isdnbchanioctl,	nostop,		noreset,	nodevtotty,
	POLLFIELD,	nommap, 	NULL, "isdnbchan", NULL, -1
};
#endif

static void isdnbchanattach(void *);
PSEUDO_SET(isdnbchanattach, i4b_rbch);

/*===========================================================================*
 *			DEVICE DRIVER ROUTINES
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 *	initialization at kernel load time
 *---------------------------------------------------------------------------*/
static void
isdnbchaninit(void *unused)
{
#if defined(__FreeBSD__) && __FreeBSD__ >= 4
	cdevsw_add(&isdnbchan_cdevsw);
#else
	dev_t dev = makedev(CDEV_MAJOR, 0);
	cdevsw_add(&dev, &isdnbchan_cdevsw, NULL);
#endif
}

SYSINIT(isdnbchandev, SI_SUB_DRIVERS,
	SI_ORDER_MIDDLE+CDEV_MAJOR, &isdnbchaninit, NULL);

#endif /* BSD > 199306 && defined(__FreeBSD__) */

#ifdef __bsdi__
int isdnbchanmatch(struct device *parent, struct cfdata *cf, void *aux);
void dummy_isdnbchanattach(struct device*, struct device *, void *);

#define CDEV_MAJOR 61

static struct cfdriver isdnbchancd =
	{ NULL, "isdnbchan", isdnbchanmatch, dummy_isdnbchanattach, DV_DULL,
	  sizeof(struct cfdriver) };
struct devsw isdnbchansw =
	{ &isdnbchancd,
	  isdnbchanopen,	isdnbchanclose,	isdnbchanread,	isdnbchanwrite,
	  isdnbchanioctl,	seltrue,	nommap,		nostrat,
	  nodump,	nopsize,	0,		nostop
};

int
isdnbchanmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	printf("isdnbchanmatch: aux=0x%x\n", aux);
	return 1;
}
void
dummy_isdnbchanattach(struct device *parent, struct device *self, void *aux)
{
	printf("dummy_isdnbchanattach: aux=0x%x\n", aux);
}
#endif /* __bsdi__ */


static const struct isdn_l4_driver_functions
rbch_driver_functions = {
	rbch_rx_data_rdy,
	rbch_tx_queue_empty,
	rbch_activity,
	rbch_connect,
	rbch_disconnect,
	rbch_dialresponse,
	rbch_updown,
	rbch_get_softc,
	rbch_set_linktab,
	NULL
};

static int rbch_driver_id = -1;

/*---------------------------------------------------------------------------*
 *	interface attach routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
isdnbchanattach(void *dummy)
#else
isdnbchanattach()
#endif
{
	int i;

	rbch_driver_id = isdn_l4_driver_attach("isdnbchan", NISDNBCHAN, &rbch_driver_functions);

	for(i=0; i < NISDNBCHAN; i++)
	{
#if defined(__FreeBSD__)
#if __FreeBSD__ == 3

#ifdef DEVFS
		rbch_softc[i].devfs_token =
			devfs_add_devswf(&isdnbchan_cdevsw, i, DV_CHR,
				     UID_ROOT, GID_WHEEL, 0600,
				     "isdnbchan%d", i);
#endif

#else
		make_dev(&isdnbchan_cdevsw, i,
			UID_ROOT, GID_WHEEL, 0600, "isdnbchan%d", i);
#endif
#endif

#if I4BRBCHACCT
#if defined(__FreeBSD__)
		callout_handle_init(&rbch_softc[i].sc_callout);
#endif
#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
		callout_init(&rbch_softc[i].sc_callout, 0);
#endif
		rbch_softc[i].sc_fn = 1;
#endif
		rbch_softc[i].sc_unit = i;
		rbch_softc[i].sc_devstate = ST_IDLE;
		rbch_softc[i].sc_hdlcq.ifq_maxlen = I4BRBCHMAXQLEN;
		rbch_softc[i].it_in.c_ispeed = rbch_softc[i].it_in.c_ospeed = 64000;
		termioschars(&rbch_softc[i].it_in);
	}
}

/*---------------------------------------------------------------------------*
 *	open rbch device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanopen(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	int unit = minor(dev);

	if(unit >= NISDNBCHAN)
		return(ENXIO);

	if(rbch_softc[unit].sc_devstate & ST_ISOPEN)
		return(EBUSY);

#if 0
	rbch_clrq(unit);
#endif

	rbch_softc[unit].sc_devstate |= ST_ISOPEN;

	NDBGL4(L4_RBCHDBG, "unit %d, open", unit);

	return(0);
}

/*---------------------------------------------------------------------------*
 *	close rbch device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanclose(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	int unit = minor(dev);
	struct rbch_softc *sc = &rbch_softc[unit];

	if(sc->sc_devstate & ST_CONNECTED)
		i4b_l4_drvrdisc(sc->sc_cd->cdid);

	sc->sc_devstate &= ~ST_ISOPEN;

	rbch_clrq(sc);

	NDBGL4(L4_RBCHDBG, "channel %d, closed", unit);

	return(0);
}

/*---------------------------------------------------------------------------*
 *	read from rbch device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanread(dev_t dev, struct uio *uio, int ioflag)
{
	struct mbuf *m;
	int error = 0;
	int unit = minor(dev);
	struct ifqueue *iqp;
	struct rbch_softc *sc = &rbch_softc[unit];

	int s;

	NDBGL4(L4_RBCHDBG, "unit %d, enter read", unit);

	s = splnet();
	if(!(sc->sc_devstate & ST_ISOPEN))
	{
		splx(s);
		NDBGL4(L4_RBCHDBG, "unit %d, read while not open", unit);
		return(EIO);
	}

	if((sc->sc_devstate & ST_NOBLOCK))
	{
		if(!(sc->sc_devstate & ST_CONNECTED)) {
			splx(s);
			return(EWOULDBLOCK);
		}

		if(sc->sc_bprot == BPROT_RHDLC)
			iqp = &sc->sc_hdlcq;
		else
			iqp = sc->sc_ilt->rx_queue;

		if(IF_QEMPTY(iqp) && (sc->sc_devstate & ST_ISOPEN)) {
			splx(s);
			return(EWOULDBLOCK);
	}
	}
	else
	{
		while(!(sc->sc_devstate & ST_CONNECTED))
		{
			NDBGL4(L4_RBCHDBG, "unit %d, wait read init", unit);

			if((error = tsleep((void *) &rbch_softc[unit],
					   TTIPRI | PCATCH,
					   "rrrbch", 0 )) != 0)
			{
				splx(s);
				NDBGL4(L4_RBCHDBG, "unit %d, error %d tsleep", unit, error);
				return(error);
			}
		}

		if(sc->sc_bprot == BPROT_RHDLC)
			iqp = &sc->sc_hdlcq;
		else
			iqp = sc->sc_ilt->rx_queue;

		while(IF_QEMPTY(iqp) && (sc->sc_devstate & ST_ISOPEN))
		{
			sc->sc_devstate |= ST_RDWAITDATA;

			NDBGL4(L4_RBCHDBG, "unit %d, wait read data", unit);

			if((error = tsleep((void *) &sc->sc_ilt->rx_queue,
					   TTIPRI | PCATCH,
					   "rrbch", 0 )) != 0)
			{
				splx(s);
				NDBGL4(L4_RBCHDBG, "unit %d, error %d tsleep read", unit, error);
				sc->sc_devstate &= ~ST_RDWAITDATA;
				return(error);
			} else if (!(sc->sc_devstate & ST_CONNECTED)) {
				splx(s);
				return 0;
			}
		}
	}

	IF_DEQUEUE(iqp, m);

	NDBGL4(L4_RBCHDBG, "unit %d, read %d bytes", unit, m->m_len);

	if(m && m->m_len)
	{
		error = uiomove(m->m_data, m->m_len, uio);
	}
	else
	{
		NDBGL4(L4_RBCHDBG, "unit %d, error %d uiomove", unit, error);
		error = EIO;
	}

	if(m)
		i4b_Bfreembuf(m);

	splx(s);

	return(error);
}

/*---------------------------------------------------------------------------*
 *	write to rbch device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanwrite(dev_t dev, struct uio * uio, int ioflag)
{
	struct mbuf *m;
	int error = 0;
	int unit = minor(dev);
	struct rbch_softc *sc = &rbch_softc[unit];

	int s;

	NDBGL4(L4_RBCHDBG, "unit %d, write", unit);

	s = splnet();
	if(!(sc->sc_devstate & ST_ISOPEN))
	{
		NDBGL4(L4_RBCHDBG, "unit %d, write while not open", unit);
		splx(s);
		return(EIO);
	}

	if((sc->sc_devstate & ST_NOBLOCK))
	{
		if(!(sc->sc_devstate & ST_CONNECTED)) {
			splx(s);
			return(EWOULDBLOCK);
		}
		if(IF_QFULL(sc->sc_ilt->tx_queue) && (sc->sc_devstate & ST_ISOPEN)) {
			splx(s);
			return(EWOULDBLOCK);
	}
	}
	else
	{
		while(!(sc->sc_devstate & ST_CONNECTED))
		{
			NDBGL4(L4_RBCHDBG, "unit %d, write wait init", unit);

			error = tsleep((void *) &rbch_softc[unit],
						   TTIPRI | PCATCH,
						   "wrrbch", 0 );
			if(error == ERESTART) {
				splx(s);
				return (ERESTART);
			}
			else if(error == EINTR)
			{
				splx(s);
				NDBGL4(L4_RBCHDBG, "unit %d, EINTR during wait init", unit);
				return(EINTR);
			}
			else if(error)
			{
				splx(s);
				NDBGL4(L4_RBCHDBG, "unit %d, error %d tsleep init", unit, error);
				return(error);
			}
			tsleep((void *) &rbch_softc[unit], TTIPRI | PCATCH, "xrbch", (hz*1));
		}

		while(IF_QFULL(sc->sc_ilt->tx_queue) && (sc->sc_devstate & ST_ISOPEN))
		{
			sc->sc_devstate |= ST_WRWAITEMPTY;

			NDBGL4(L4_RBCHDBG, "unit %d, write queue full", unit);

			if ((error = tsleep((void *) &sc->sc_ilt->tx_queue,
					    TTIPRI | PCATCH,
					    "wrbch", 0)) != 0) {
				sc->sc_devstate &= ~ST_WRWAITEMPTY;
				if(error == ERESTART)
				{
					splx(s);
					return(ERESTART);
				}
				else if(error == EINTR)
				{
					splx(s);
					NDBGL4(L4_RBCHDBG, "unit %d, EINTR during wait write", unit);
					return(error);
				}
				else if(error)
				{
					splx(s);
					NDBGL4(L4_RBCHDBG, "unit %d, error %d tsleep write", unit, error);
					return(error);
				}
				else if (!(sc->sc_devstate & ST_CONNECTED)) {
					splx(s);
					return 0;
				}
			}
		}
	}

	if(!(sc->sc_devstate & ST_ISOPEN))
	{
		NDBGL4(L4_RBCHDBG, "unit %d, not open anymore", unit);
		splx(s);
		return(EIO);
	}

	if((m = i4b_Bgetmbuf(BCH_MAX_DATALEN)) != NULL)
	{
		m->m_len = min(BCH_MAX_DATALEN, uio->uio_resid);

		NDBGL4(L4_RBCHDBG, "unit %d, write %d bytes", unit, m->m_len);

		error = uiomove(m->m_data, m->m_len, uio);

		if(IF_QFULL(sc->sc_ilt->tx_queue))
		{
			m_freem(m);
		}
		else
		{
			IF_ENQUEUE(sc->sc_ilt->tx_queue, m);
		}

		(*sc->sc_ilt->bchannel_driver->bch_tx_start)(sc->sc_ilt->l1token, sc->sc_ilt->channel);
	}

	splx(s);

	return(error);
}

/*---------------------------------------------------------------------------*
 *	rbch device ioctl handlibg
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanioctl(dev_t dev, IOCTL_CMD_T cmd, void *data, int flag,
	struct lwp *l)
{
	int error = 0;
	int unit = minor(dev);
	struct rbch_softc *sc = &rbch_softc[unit];

	switch(cmd)
	{
		case FIOASYNC:	/* Set async mode */
			if (*(int *)data)
			{
				NDBGL4(L4_RBCHDBG, "unit %d, setting async mode", unit);
			}
			else
			{
				NDBGL4(L4_RBCHDBG, "unit %d, clearing async mode", unit);
			}
			break;

		case FIONBIO:
			if (*(int *)data)
			{
				NDBGL4(L4_RBCHDBG, "unit %d, setting non-blocking mode", unit);
				sc->sc_devstate |= ST_NOBLOCK;
			}
			else
			{
				NDBGL4(L4_RBCHDBG, "unit %d, clearing non-blocking mode", unit);
				sc->sc_devstate &= ~ST_NOBLOCK;
			}
			break;

		case TIOCCDTR:	/* Clear DTR */
			if(sc->sc_devstate & ST_CONNECTED)
			{
				NDBGL4(L4_RBCHDBG, "unit %d, disconnecting for DTR down", unit);
				i4b_l4_drvrdisc(sc->sc_cd->cdid);
			}
			break;

		case I4B_RBCH_DIALOUT:
                {
			size_t x;

			for (x = 0; x < TELNO_MAX && ((char *)data)[x]; x++)
				;
			if (x)
			{
				NDBGL4(L4_RBCHDBG, "%d, attempting dialout to %s", unit, (char *)data);
				i4b_l4_dialoutnumber(rbch_driver_id, unit, x, (char *)data);
				break;
			}
			/* fall through to SDTR */
		}

		case TIOCSDTR:	/* Set DTR */
			NDBGL4(L4_RBCHDBG, "unit %d, attempting dialout (DTR)", unit);
			i4b_l4_dialout(rbch_driver_id, unit);
			break;

		case TIOCSETA:	/* Set termios struct */
			break;

		case TIOCGETA:	/* Get termios struct */
			*(struct termios *)data = sc->it_in;
			break;

		case TIOCMGET:
			*(int *)data = TIOCM_LE|TIOCM_DTR|TIOCM_RTS|TIOCM_CTS|TIOCM_DSR;
			if (sc->sc_devstate & ST_CONNECTED)
				*(int *)data |= TIOCM_CD;
			break;

		case I4B_RBCH_VR_REQ:
                {
			msg_vr_req_t *mvr;

			mvr = (msg_vr_req_t *)data;

			mvr->version = VERSION;
			mvr->release = REL;
			mvr->step = STEP;
			break;
		}

		default:	/* Unknown stuff */
			NDBGL4(L4_RBCHDBG, "(minor=%d) ioctl, unknown cmd %lx", unit, (u_long)cmd);
			error = EINVAL;
			break;
	}
	return(error);
}

#ifdef OS_USES_POLL

/*---------------------------------------------------------------------------*
 *	device driver poll
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanpoll(dev_t dev, int events, struct lwp *l)
{
	int revents = 0;	/* Events we found */
	int s;
	int unit = minor(dev);
	struct rbch_softc *sc = &rbch_softc[unit];

	/* We can't check for anything but IN or OUT */

	s = splhigh();

	if(!(sc->sc_devstate & ST_ISOPEN))
	{
		splx(s);
		return(POLLNVAL);
	}

	/*
	 * Writes are OK if we are connected and the
         * transmit queue can take them
	 */

	if((events & (POLLOUT|POLLWRNORM)) &&
	   (sc->sc_devstate & ST_CONNECTED) &&
	   !IF_QFULL(sc->sc_ilt->tx_queue))
	{
		revents |= (events & (POLLOUT|POLLWRNORM));
	}

	/* ... while reads are OK if we have any data */

	if((events & (POLLIN|POLLRDNORM)) &&
	   (sc->sc_devstate & ST_CONNECTED))
	{
		struct ifqueue *iqp;

		if(sc->sc_bprot == BPROT_RHDLC)
			iqp = &sc->sc_hdlcq;
		else
			iqp = sc->sc_ilt->rx_queue;

		if(!IF_QEMPTY(iqp))
			revents |= (events & (POLLIN|POLLRDNORM));
	}

	if(revents == 0)
		selrecord(l, &sc->selp);

	splx(s);
	return(revents);
}

static void
filt_i4brbchdetach(struct knote *kn)
{
	struct rbch_softc *sc = kn->kn_hook;
	int s;

	s = splhigh();
	SLIST_REMOVE(&sc->selp.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_i4brbchread(struct knote *kn, long hint)
{
	struct rbch_softc *sc = kn->kn_hook;
	struct ifqueue *iqp;

	if ((sc->sc_devstate & ST_CONNECTED) == 0)
		return (0);

	if (sc->sc_bprot == BPROT_RHDLC)
		iqp = &sc->sc_hdlcq;
	else
		iqp = sc->sc_ilt->rx_queue;

	if (IF_QEMPTY(iqp))
		return (0);

	kn->kn_data = 0;	/* XXXLUKEM (thorpej): what to put here? */
	return (1);
}

static const struct filterops i4brbchread_filtops =
	{ 1, NULL, filt_i4brbchdetach, filt_i4brbchread };

static int
filt_i4brbchwrite(struct knote *kn, long hint)
{
	struct rbch_softc *sc = kn->kn_hook;

	if ((sc->sc_devstate & ST_CONNECTED) == 0)
		return (0);

	if (IF_QFULL(sc->sc_ilt->tx_queue))
		return (0);

	kn->kn_data = 0;	/* XXXLUKEM (thorpej): what to put here? */
	return (1);
}

static const struct filterops i4brbchwrite_filtops =
	{ 1, NULL, filt_i4brbchdetach, filt_i4brbchwrite };

int
isdnbchankqfilter(dev_t dev, struct knote *kn)
{
	struct rbch_softc *sc = &rbch_softc[minor(dev)];
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->selp.sel_klist;
		kn->kn_fop = &i4brbchread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sc->selp.sel_klist;
		kn->kn_fop = &i4brbchwrite_filtops;
		break;

	default:
		return (EINVAL);
	}

	kn->kn_hook = sc;

	s = splhigh();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

#else /* OS_USES_POLL */

/*---------------------------------------------------------------------------*
 *	device driver select
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdnbchanselect(dev_t dev, int rw, struct lwp *l)
{
	int unit = minor(dev);
	struct rbch_softc *sc = &rbch_softc[unit];
        int s;

	s = splhigh();

	if(!(sc->sc_devstate & ST_ISOPEN))
	{
		splx(s);
		NDBGL4(L4_RBCHDBG, "(minor=%d) not open anymore", unit);
		return(1);
	}

	if(sc->sc_devstate & ST_CONNECTED)
	{
		struct ifqueue *iqp;

		switch(rw)
		{
			case FREAD:
				if(sc->sc_bprot == BPROT_RHDLC)
					iqp = &sc->sc_hdlcq;
				else
					iqp = isdn_linktab[unit]->rx_queue;

				if(!IF_QEMPTY(iqp))
				{
					splx(s);
					return(1);
				}
				break;

			case FWRITE:
				if(!IF_QFULL(isdn_linktab[unit]->rx_queue))
				{
					splx(s);
					return(1);
				}
				break;

			default:
				splx(s);
				return 0;
		}
	}
	selrecord(l, &sc->selp);
	splx(s);
	return(0);
}

#endif /* OS_USES_POLL */

#if I4BRBCHACCT
/*---------------------------------------------------------------------------*
 *	watchdog routine
 *---------------------------------------------------------------------------*/
static void
rbch_timeout(struct rbch_softc *sc)
{
	bchan_statistics_t bs;

	/* get # of bytes in and out from the HSCX driver */

	(*sc->sc_ilt->bchannel_driver->bch_stat)
		(sc->sc_ilt->l1token, sc->sc_ilt->channel, &bs);

	sc->sc_ioutb += bs.outbytes;
	sc->sc_iinb += bs.inbytes;

	if((sc->sc_iinb != sc->sc_linb) || (sc->sc_ioutb != sc->sc_loutb) || sc->sc_fn)
	{
		int ri = (sc->sc_iinb - sc->sc_linb)/I4BRBCHACCTINTVL;
		int ro = (sc->sc_ioutb - sc->sc_loutb)/I4BRBCHACCTINTVL;

		if((sc->sc_iinb == sc->sc_linb) && (sc->sc_ioutb == sc->sc_loutb))
			sc->sc_fn = 0;
		else
			sc->sc_fn = 1;

		sc->sc_linb = sc->sc_iinb;
		sc->sc_loutb = sc->sc_ioutb;

		if (sc->sc_cd)
			i4b_l4_accounting(sc->sc_cd->cdid, ACCT_DURING,
			    sc->sc_ioutb, sc->sc_iinb, ro, ri, sc->sc_ioutb, sc->sc_iinb);
 	}
	START_TIMER(sc->sc_callout, rbch_timeout, sc, I4BRBCHACCTINTVL*hz);
}
#endif /* I4BRBCHACCT */

/*===========================================================================*
 *			ISDN INTERFACE ROUTINES
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 *	this routine is called from L4 handler at connect time
 *---------------------------------------------------------------------------*/
static void
rbch_connect(void *softc, void *cdp)
{
	call_desc_t *cd = (call_desc_t *)cdp;
	struct rbch_softc *sc = softc;

	sc->sc_bprot = cd->bprot;

#if I4BRBCHACCT
	if(sc->sc_bprot == BPROT_RHDLC)
	{
		sc->sc_iinb = 0;
		sc->sc_ioutb = 0;
		sc->sc_linb = 0;
		sc->sc_loutb = 0;

		START_TIMER(sc->sc_callout, rbch_timeout, sc, I4BRBCHACCTINTVL*hz);
	}
#endif
	if(!(sc->sc_devstate & ST_CONNECTED))
	{
		NDBGL4(L4_RBCHDBG, "B channel %d at ISDN %d, wakeup",
			cd->channelid, cd->isdnif);
		sc->sc_devstate |= ST_CONNECTED;
		sc->sc_cd = cdp;
		wakeup((void *)sc);
		selwakeup(&sc->selp);
	}
}

/*---------------------------------------------------------------------------*
 *	this routine is called from L4 handler at disconnect time
 *---------------------------------------------------------------------------*/
static void
rbch_disconnect(void *softc, void *cdp)
{
	call_desc_t *cd = cdp;
	struct rbch_softc *sc = softc;

	int s;

        if(cd != sc->sc_cd)
	{
		NDBGL4(L4_RBCHDBG, "B channel %d at ISDN %d not active",
		    cd->channelid, cd->isdnif);
		return;
	}

	s = splnet();

	NDBGL4(L4_RBCHDBG, "B channel %d at ISDN %d disconnect",
	    cd->channelid, cd->isdnif);

	sc->sc_devstate &= ~ST_CONNECTED;

#if I4BRBCHACCT
	if (sc->sc_cd)
		i4b_l4_accounting(sc->sc_cd->cdid, ACCT_FINAL,
		    sc->sc_ioutb, sc->sc_iinb, 0, 0, sc->sc_ioutb, sc->sc_iinb);

	STOP_TIMER(sc->sc_callout, rbch_timeout, sc);
#endif

	sc->sc_cd = NULL;
	if (sc->sc_devstate & ST_RDWAITDATA)
		wakeup(&sc->sc_ilt->rx_queue);
	if (sc->sc_devstate & ST_WRWAITEMPTY)
		wakeup(&sc->sc_ilt->tx_queue);

	splx(s);

	selwakeup(&sc->selp);
}

/*---------------------------------------------------------------------------*
 *	feedback from daemon in case of dial problems
 *---------------------------------------------------------------------------*/
static void
rbch_dialresponse(void *softc, int status,
	cause_t cause)
{
}

/*---------------------------------------------------------------------------*
 *	interface up/down
 *---------------------------------------------------------------------------*/
static void
rbch_updown(void *softc, int updown)
{
}

/*---------------------------------------------------------------------------*
 *	this routine is called from the HSCX interrupt handler
 *	when a new frame (mbuf) has been received and is to be put on
 *	the rx queue.
 *---------------------------------------------------------------------------*/
static void
rbch_rx_data_rdy(void *softc)
{
	struct rbch_softc *sc = softc;

	if(sc->sc_bprot == BPROT_RHDLC)
	{
		register struct mbuf *m;

		if((m = *sc->sc_ilt->rx_mbuf) == NULL)
			return;

		m->m_pkthdr.len = m->m_len;

		if(IF_QFULL(&sc->sc_hdlcq))
		{
			NDBGL4(L4_RBCHDBG, "(minor=%d) hdlc rx queue full!", sc->sc_unit);
			m_freem(m);
		}
		else
		{
			IF_ENQUEUE(&sc->sc_hdlcq, m);
		}
	}

	if(sc->sc_devstate & ST_RDWAITDATA)
	{
		NDBGL4(L4_RBCHDBG, "(minor=%d) wakeup", sc->sc_unit);
		sc->sc_devstate &= ~ST_RDWAITDATA;
		wakeup((void *) &sc->sc_ilt->rx_queue);
	}
	else
	{
		NDBGL4(L4_RBCHDBG, "(minor=%d) NO wakeup", sc->sc_unit);
	}
	selnotify(&sc->selp, 0);
}

/*---------------------------------------------------------------------------*
 *	this routine is called from the HSCX interrupt handler
 *	when the last frame has been sent out and there is no
 *	further frame (mbuf) in the tx queue.
 *---------------------------------------------------------------------------*/
static void
rbch_tx_queue_empty(void *softc)
{
	struct rbch_softc *sc = softc;

	if(sc->sc_devstate & ST_WRWAITEMPTY)
	{
		NDBGL4(L4_RBCHDBG, "(minor=%d): wakeup", sc->sc_unit);
		sc->sc_devstate &= ~ST_WRWAITEMPTY;
		wakeup((void *) &sc->sc_ilt->tx_queue);
	}
	else
	{
		NDBGL4(L4_RBCHDBG, "(minor=%d) NO wakeup", sc->sc_unit);
	}
	selnotify(&sc->selp, 0);
}

/*---------------------------------------------------------------------------*
 *	this routine is called from the HSCX interrupt handler
 *	each time a packet is received or transmitted
 *---------------------------------------------------------------------------*/
static void
rbch_activity(void *softc, int rxtx)
{
	struct rbch_softc *sc = softc;

	if (sc->sc_cd)
		sc->sc_cd->last_active_time = SECOND;
	selnotify(&sc->selp, 0);
}

/*---------------------------------------------------------------------------*
 *	clear an hdlc rx queue for a rbch unit
 *---------------------------------------------------------------------------*/
static void
rbch_clrq(void *softc)
{
	struct rbch_softc *sc = softc;
	struct mbuf *m;
	int s;

	for(;;)
	{
		s = splnet();
		IF_DEQUEUE(&sc->sc_hdlcq, m);
		splx(s);

		if(m)
			m_freem(m);
		else
			break;
	}
}

/*---------------------------------------------------------------------------*
 *	setup the isdn_linktab for this driver
 *---------------------------------------------------------------------------*/
static void
rbch_set_linktab(void *softc, isdn_link_t *ilt)
{
	struct rbch_softc *sc = softc;
	sc->sc_ilt = ilt;
}

/*---------------------------------------------------------------------------*
 *	initialize this drivers linktab
 *---------------------------------------------------------------------------*/
static void*
rbch_get_softc(int unit)
{
	return &rbch_softc[unit];
}

/*===========================================================================*/

#endif /* NISDNBCHAN > 0 */
