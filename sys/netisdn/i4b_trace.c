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
 *	i4btrc - device driver for trace data read device
 *	---------------------------------------------------
 *
 *	$Id: i4b_trace.c,v 1.17.4.1 2007/03/12 06:00:13 rmind Exp $
 *
 *	last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_trace.c,v 1.17.4.1 2007/03/12 06:00:13 rmind Exp $");

#include "isdntrc.h"

#if NISDNTRC > 0

#include <sys/param.h>
#include <sys/systm.h>

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#endif

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <netisdn/i4b_trace.h>
#include <netisdn/i4b_ioctl.h>

#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>

static struct ifqueue trace_queue[NISDNTRC];
static int device_state[NISDNTRC];
#define ST_IDLE		0x00
#define ST_ISOPEN	0x01
#define ST_WAITDATA	0x02

static int analyzemode = 0;		/* we are in anlyzer mode */
static int rxunit = -1;			/* l2 isdnif of receiving driver */
static int txunit = -1;			/* l2 isdnif of transmitting driver */
static int outunit = -1;		/* output device for trace data */

#define	PDEVSTATIC	/* - not static - */
void isdntrcattach __P((void));
int isdntrcopen __P((dev_t dev, int flag, int fmt, struct lwp *l));
int isdntrcclose __P((dev_t dev, int flag, int fmt, struct lwp *l));
int isdntrcread __P((dev_t dev, struct uio * uio, int ioflag));
int isdntrcioctl __P((dev_t dev, u_long cmd, void *data, int flag, struct lwp *l));

#ifdef __NetBSD__
const struct cdevsw isdntrc_cdevsw = {
	isdntrcopen, isdntrcclose, isdntrcread, nowrite, isdntrcioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER
};
#endif /* __NetBSD__ */

/*---------------------------------------------------------------------------*
 *	interface attach routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
isdntrcattach(void *dummy)
#else
isdntrcattach()
#endif
{
	int i;

	for(i=0; i < NISDNTRC; i++)
	{

#if defined(__FreeBSD__)
#if __FreeBSD__ < 4

#ifdef DEVFS
	  	devfs_token[i]
		  = devfs_add_devswf(&i4btrc_cdevsw, i, DV_CHR,
				     UID_ROOT, GID_WHEEL, 0600,
				     "isdntrc%d", i);
#endif

#else
		make_dev(&i4btrc_cdevsw, i,
				     UID_ROOT, GID_WHEEL, 0600, "isdntrc%d", i);
#endif
#endif
		trace_queue[i].ifq_maxlen = IFQ_MAXLEN;
		device_state[i] = ST_IDLE;
	}
}

/*---------------------------------------------------------------------------*
 *	isdn_layer2_trace_ind
 *	---------------------
 *	is called from layer 1, adds timestamp to trace data and puts
 *	it into a queue, from which it can be read from the i4btrc
 *	device. The unit number in the trace header selects the minor
 *	device's queue the data is put into.
 *---------------------------------------------------------------------------*/
int
isdn_layer2_trace_ind(struct l2_softc *sc, struct isdn_l3_driver *drv, i4b_trace_hdr *hdr, size_t len, unsigned char *buf)
{
	struct mbuf *m;
	int isdnif, x;
	int trunc = 0;
	int totlen = len + sizeof(i4b_trace_hdr);

	MICROTIME(hdr->time);
	hdr->isdnif = sc->drv->isdnif;

	/*
	 * for telephony (or better non-HDLC HSCX mode) we get
	 * (MCLBYTE + sizeof(i4b_trace_hdr_t)) length packets
	 * to put into the queue to userland. because of this
	 * we detect this situation, strip the length to MCLBYTES
	 * max size, and infor the userland program of this fact
	 * by putting the no of truncated bytes into hdr->trunc.
	 */

	if(totlen > MCLBYTES)
	{
		trunc = 1;
		hdr->trunc = totlen - MCLBYTES;
		totlen = MCLBYTES;
	}
	else
	{
		hdr->trunc = 0;
	}

	/* set length of trace record */

	hdr->length = totlen;

	/* check valid interface */

	if((isdnif = hdr->isdnif) > NISDNTRC)
	{
		printf("i4b_trace: get_trace_data_from_l1 - isdnif > NISDNTRC!\n");
		return(0);
	}

	/* get mbuf */

	if(!(m = i4b_Bgetmbuf(totlen)))
	{
		printf("i4b_trace: get_trace_data_from_l1 - i4b_getmbuf() failed!\n");
		return(0);
	}

	/* check if we are in analyzemode */

	if(analyzemode && (isdnif == rxunit || isdnif == txunit))
	{
		if(isdnif == rxunit)
			hdr->dir = FROM_NT;
		else
			hdr->dir = FROM_TE;
		isdnif = outunit;
	}

	if(IF_QFULL(&trace_queue[isdnif]))
	{
		struct mbuf *m1;

		x = splnet();
		IF_DEQUEUE(&trace_queue[isdnif], m1);
		splx(x);

		i4b_Bfreembuf(m1);
	}

	/* copy trace header */
	memcpy(m->m_data, hdr, sizeof(i4b_trace_hdr));

	/* copy trace data */
	if(trunc)
		memcpy(&m->m_data[sizeof(i4b_trace_hdr)], buf, totlen-sizeof(i4b_trace_hdr));
	else
		memcpy(&m->m_data[sizeof(i4b_trace_hdr)], buf, len);

	x = splnet();

	IF_ENQUEUE(&trace_queue[isdnif], m);

	if(device_state[isdnif] & ST_WAITDATA)
	{
		device_state[isdnif] &= ~ST_WAITDATA;
		wakeup((void *) &trace_queue[isdnif]);
	}

	splx(x);

	return(1);
}

/*---------------------------------------------------------------------------*
 *	open trace device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdntrcopen(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	int x;
	int unit = minor(dev);

	if(unit >= NISDNTRC)
		return(ENXIO);

	if(device_state[unit] & ST_ISOPEN)
		return(EBUSY);

	if(analyzemode && (unit == outunit || unit == rxunit || unit == txunit))
		return(EBUSY);


	x = splnet();

	device_state[unit] = ST_ISOPEN;

	splx(x);

	return(0);
}

/*---------------------------------------------------------------------------*
 *	close trace device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdntrcclose(dev_t dev, int flag, int fmt,
	struct lwp *l)
{
	int isdnif = minor(dev);
	int x;

	if (analyzemode && (isdnif == outunit)) {
		l2_softc_t * rx_l2sc, * tx_l2sc;
		analyzemode = 0;
		outunit = -1;

		rx_l2sc = (l2_softc_t*)isdn_find_softc_by_isdnif(rxunit);
		tx_l2sc = (l2_softc_t*)isdn_find_softc_by_isdnif(txunit);

		if (rx_l2sc != NULL)
			rx_l2sc->driver->mph_command_req(rx_l2sc->l1_token, CMR_SETTRACE, TRACE_OFF);
		if (tx_l2sc != NULL)
			tx_l2sc->driver->mph_command_req(tx_l2sc->l1_token, CMR_SETTRACE, TRACE_OFF);

		x = splnet();
		device_state[rxunit] = ST_IDLE;
		device_state[txunit] = ST_IDLE;
		splx(x);
		rxunit = -1;
		txunit = -1;
	} else {
		l2_softc_t * l2sc = (l2_softc_t*)isdn_find_softc_by_isdnif(isdnif);
		if (l2sc != NULL) {
			l2sc->driver->mph_command_req(l2sc->l1_token, CMR_SETTRACE, TRACE_OFF);
			x = splnet();
			device_state[isdnif] = ST_IDLE;
			splx(x);
		}
	}
	return(0);
}

/*---------------------------------------------------------------------------*
 *	read from trace device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdntrcread(dev_t dev, struct uio * uio, int ioflag)
{
	struct mbuf *m;
	int x;
	int error = 0;
	int unit = minor(dev);

	if(!(device_state[unit] & ST_ISOPEN))
		return(EIO);

	x = splnet();

	while(IF_QEMPTY(&trace_queue[unit]) && (device_state[unit] & ST_ISOPEN))
	{
		device_state[unit] |= ST_WAITDATA;

		if((error = tsleep((void *) &trace_queue[unit],
					TTIPRI | PCATCH,
					"bitrc", 0 )) != 0)
		{
			device_state[unit] &= ~ST_WAITDATA;
			splx(x);
			return(error);
		}
	}

	IF_DEQUEUE(&trace_queue[unit], m);

	if(m && m->m_len)
		error = uiomove(m->m_data, m->m_len, uio);
	else
		error = EIO;

	if(m)
		i4b_Bfreembuf(m);

	splx(x);

	return(error);
}

#if defined(__FreeBSD__) && defined(OS_USES_POLL)
/*---------------------------------------------------------------------------*
 *	poll device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4btrcpoll(dev_t dev, int events, struct proc *p)
{
	return(ENODEV);
}
#endif

/*---------------------------------------------------------------------------*
 *	device driver ioctl routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
isdntrcioctl(dev_t dev, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	int error = 0;
	int isdnif = minor(dev);
	i4b_trace_setupa_t *tsa;
	l2_softc_t * l2sc = (l2_softc_t*)isdn_find_softc_by_isdnif(isdnif);

	switch(cmd)
	{
		case I4B_TRC_SET:
			if (l2sc == NULL)
				return ENOTTY;
			l2sc->driver->mph_command_req(l2sc->l1_token, CMR_SETTRACE, (void *)*(unsigned long *)data);
			break;

		case I4B_TRC_SETA:
			tsa = (i4b_trace_setupa_t *)data;

			if(tsa->rxunit >= 0 && tsa->rxunit < NISDNTRC)
				rxunit = tsa->rxunit;
			else
				error = EINVAL;

			if(tsa->txunit >= 0 && tsa->txunit < NISDNTRC)
				txunit = tsa->txunit;
			else
				error = EINVAL;

			if(error)
			{
				outunit = -1;
				rxunit = -1;
				txunit = -1;
			}
			else
			{
				l2_softc_t * rx_l2sc, * tx_l2sc;
				rx_l2sc = (l2_softc_t*)(l2_softc_t*)isdn_find_softc_by_isdnif(rxunit);
				tx_l2sc = (l2_softc_t*)(l2_softc_t*)isdn_find_softc_by_isdnif(txunit);

				if (l2sc == NULL || rx_l2sc == NULL || tx_l2sc == NULL)
					return ENOTTY;

				outunit = isdnif;
				analyzemode = 1;
				rx_l2sc->driver->mph_command_req(rx_l2sc->l1_token, CMR_SETTRACE, (void *)(unsigned long)(tsa->rxflags & (TRACE_I | TRACE_D_RX | TRACE_B_RX)));
				tx_l2sc->driver->mph_command_req(tx_l2sc->l1_token, CMR_SETTRACE, (void *)(unsigned long)(tsa->txflags & (TRACE_I | TRACE_D_RX | TRACE_B_RX)));
			}
			break;

		case I4B_TRC_RESETA:
			analyzemode = 0;
			outunit = -1;
			rxunit = -1;
			txunit = -1;
			break;

		default:
			error = ENOTTY;
			break;
	}
	return(error);
}

#endif /* NISDNTRC > 0 */
