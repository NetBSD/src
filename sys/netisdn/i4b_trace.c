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
 *	$Id: i4b_trace.c,v 1.5.4.1 2001/10/10 11:57:06 fvdl Exp $
 *
 *	last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *
 *---------------------------------------------------------------------------*/

#include "i4btrc.h"

#if NI4BTRC > 0

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
#include <sys/vnode.h>

#include <netisdn/i4b_trace.h>
#include <netisdn/i4b_ioctl.h>

#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_l2.h>

static struct ifqueue trace_queue[NI4BTRC];
static int device_state[NI4BTRC];
#define ST_IDLE		0x00
#define ST_ISOPEN	0x01
#define ST_WAITDATA	0x02

static int analyzemode = 0;			/* we are in anlyzer mode */
static int rxunit = -1;				/* l2 bri of receiving driver */
static int txunit = -1;				/* l2 bri of transmitting driver */
static int outunit = -1;			/* output device for trace data */

#define	PDEVSTATIC	/* - not static - */
void i4btrcattach __P((void));
int i4btrcopen __P((struct vnode * dev, int flag, int fmt, struct proc *p));
int i4btrcclose __P((struct vnode * dev, int flag, int fmt, struct proc *p));
int i4btrcread __P((struct vnode * dev, struct uio * uio, int ioflag));
int i4btrcioctl __P((struct vnode * dev, u_long cmd, caddr_t data, int flag, struct proc *p));

/*---------------------------------------------------------------------------*
 *	interface attach routine
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
i4btrcattach(void *dummy)
#else
i4btrcattach()
#endif
{
	int i;
	
#ifndef HACK_NO_PSEUDO_ATTACH_MSG
	printf("i4btrc: %d ISDN trace device(s) attached\n", NI4BTRC);
#endif
	
	for(i=0; i < NI4BTRC; i++)
	{

#if defined(__FreeBSD__)
#if __FreeBSD__ < 4

#ifdef DEVFS
	  	devfs_token[i]
		  = devfs_add_devswf(&i4btrc_cdevsw, i, DV_CHR,
				     UID_ROOT, GID_WHEEL, 0600,
				     "i4btrc%d", i);
#endif

#else
		make_dev(&i4btrc_cdevsw, i,
				     UID_ROOT, GID_WHEEL, 0600, "i4btrc%d", i);
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
isdn_layer2_trace_ind(isdn_layer2token t, i4b_trace_hdr *hdr, size_t len, unsigned char *buf)
{
	struct l2_softc *sc = (struct l2_softc*)t;
	struct mbuf *m;
	int bri, x;
	int trunc = 0;
	int totlen = len + sizeof(i4b_trace_hdr);

	MICROTIME(hdr->time);
	hdr->bri = sc->bri;

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
	
	if((bri = hdr->bri) > NI4BTRC)
	{
		printf("i4b_trace: get_trace_data_from_l1 - bri > NI4BTRC!\n"); 
		return(0);
	}

	/* get mbuf */
	
	if(!(m = i4b_Bgetmbuf(totlen)))
	{
		printf("i4b_trace: get_trace_data_from_l1 - i4b_getmbuf() failed!\n");
		return(0);
	}

	/* check if we are in analyzemode */
	
	if(analyzemode && (bri == rxunit || bri == txunit))
	{
		if(bri == rxunit)
			hdr->dir = FROM_NT;
		else
			hdr->dir = FROM_TE;
		bri = outunit;			
	}

	if(IF_QFULL(&trace_queue[bri]))
	{
		struct mbuf *m1;

		x = splnet();
		IF_DEQUEUE(&trace_queue[bri], m1);
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
	
	IF_ENQUEUE(&trace_queue[bri], m);
	
	if(device_state[bri] & ST_WAITDATA)
	{
		device_state[bri] &= ~ST_WAITDATA;
		wakeup((caddr_t) &trace_queue[bri]);
	}

	splx(x);
	
	return(1);
}

/*---------------------------------------------------------------------------*
 *	open trace device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4btrcopen(struct vnode *devvp, int flag, int fmt, struct proc *p)
{
	dev_t dev = vdev_rdev(devvp);
	int x;
	int unit = minor(dev);

	if(unit >= NI4BTRC)
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
i4btrcclose(struct vnode *devvp, int flag, int fmt, struct proc *p)
{
	dev_t dev = vdev_rdev(devvp);
	int bri = minor(dev);
	int x;

	if (analyzemode && (bri == outunit)) {
		l2_softc_t * rx_l2sc, * tx_l2sc;
		analyzemode = 0;		
		outunit = -1;
		
		rx_l2sc = (l2_softc_t*)isdn_find_l2_by_bri(rxunit);
		tx_l2sc = (l2_softc_t*)isdn_find_l2_by_bri(txunit);

		if (rx_l2sc != NULL)
			rx_l2sc->driver->n_mgmt_command(rx_l2sc->l1_token, CMR_SETTRACE, TRACE_OFF);
		if (tx_l2sc != NULL)
			tx_l2sc->driver->n_mgmt_command(tx_l2sc->l1_token, CMR_SETTRACE, TRACE_OFF);

		x = splnet();
		device_state[rxunit] = ST_IDLE;
		device_state[txunit] = ST_IDLE;
		splx(x);
		rxunit = -1;
		txunit = -1;
	} else {
		l2_softc_t * l2sc = (l2_softc_t*)isdn_find_l2_by_bri(bri);
		if (l2sc != NULL) {
			l2sc->driver->n_mgmt_command(l2sc->l1_token, CMR_SETTRACE, TRACE_OFF);
			x = splnet();
			device_state[bri] = ST_IDLE;
			splx(x);
		}
	}
	return(0);
}

/*---------------------------------------------------------------------------*
 *	read from trace device
 *---------------------------------------------------------------------------*/
PDEVSTATIC int
i4btrcread(struct vnode *devvp, struct uio * uio, int ioflag)
{
	dev_t dev = vdev_rdev(devvp);
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
		
		if((error = tsleep((caddr_t) &trace_queue[unit],
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
i4btrcioctl(struct vnode *devvp, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	dev_t dev = vdev_rdev(devvp);
	int error = 0;
	int bri = minor(dev);
	i4b_trace_setupa_t *tsa;
	l2_softc_t * l2sc = isdn_find_l2_by_bri(bri);

	switch(cmd)
	{
		case I4B_TRC_SET:
			if (l2sc == NULL)
				return ENOTTY;
			l2sc->driver->n_mgmt_command(l2sc->l1_token, CMR_SETTRACE, (void *)*(unsigned long *)data);
			break;

		case I4B_TRC_SETA:
			tsa = (i4b_trace_setupa_t *)data;

			if(tsa->rxunit >= 0 && tsa->rxunit < NI4BTRC)
				rxunit = tsa->rxunit;
			else
				error = EINVAL;

			if(tsa->txunit >= 0 && tsa->txunit < NI4BTRC)
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
				rx_l2sc = (l2_softc_t*)isdn_find_l2_by_bri(rxunit);
				tx_l2sc = (l2_softc_t*)isdn_find_l2_by_bri(txunit);

				if (l2sc == NULL || rx_l2sc == NULL || tx_l2sc == NULL)
					return ENOTTY;
					
				outunit = bri;
				analyzemode = 1;
				rx_l2sc->driver->n_mgmt_command(rx_l2sc->l1_token, CMR_SETTRACE, (void *)(unsigned long)(tsa->rxflags & (TRACE_I | TRACE_D_RX | TRACE_B_RX)));
				tx_l2sc->driver->n_mgmt_command(tx_l2sc->l1_token, CMR_SETTRACE, (void *)(unsigned long)(tsa->txflags & (TRACE_I | TRACE_D_RX | TRACE_B_RX)));
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

#endif /* NI4BTRC > 0 */
