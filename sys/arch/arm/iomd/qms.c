/*	$NetBSD: qms.c,v 1.1 2001/10/05 22:27:42 reinoud Exp $	*/

/*
 * Copyright (c) Scott Stevens 1995 All rights reserved
 * Copyright (c) Melvin Tang-Richardson 1995 All rights reserved
 * Copyright (c) Mark Brinicombe 1995 All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Quadrature mouse driver
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <dev/cons.h>
#include <sys/fcntl.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/time.h>
#include <sys/poll.h>

#include <machine/bus.h>
#include <machine/conf.h>
#include <machine/mouse.h>
#include <arm/iomd/qmsvar.h>

#define MOUSE_IOC_ACK

#define QMOUSE_BSIZE 12*64

#ifdef MOUSE_IOC_ACK
static void qmsputbuffer	__P((struct qms_softc *sc, struct mousebufrec *buf));
#endif

extern struct cfdriver qms_cd;

/* qms device structure */

/* Offsets of hardware registers */
#define QMS_MOUSEX	0		/* 16 bit X register */
#define QMS_MOUSEY	1		/* 16 bit Y register */

#define QMS_BUTTONS	0		/* mouse buttons register */

/*
 * generic attach routine. This does the generic part of the driver
 * attachment and is called from the bus specific attach routine.
 */

void
qmsattach(sc)
	struct qms_softc *sc;
{
	/* Set up origin and multipliers */
	sc->origx = 0;
	sc->origy = 0;
	sc->xmult = 2;
	sc->ymult = 2;

	/* Set up bounding box */
	sc->boundx = -4095;
	sc->boundy = -4095;
	sc->bounda = 4096;
	sc->boundb = 4096;

	sc->sc_state = 0;

	/* Set the mouse X & Y registers to a known state */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX, sc->origx);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX, sc->origx);
}

int
qmsopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct qms_softc *sc;
	int unit = minor(dev);
 
 	/* validate the unit and softc */
	if (unit >= qms_cd.cd_ndevs)
		return(ENXIO);

	sc = qms_cd.cd_devs[unit];
    
	if (!sc) return(ENXIO);

	/* check if we are already open */
	if (sc->sc_state & QMOUSE_OPEN) return(EBUSY);

	/* update softc */
	sc->sc_proc = p;
    
	sc->lastx = -1;
	sc->lasty = -1;
	sc->lastb = -1;

	/* initialise buffer */
	if (clalloc(&sc->sc_buffer, QMOUSE_BSIZE, 0) == -1)
		return(ENOMEM);

	/* set mode and state */
	sc->sc_mode = MOUSEMODE_ABS;
	sc->sc_state |= QMOUSE_OPEN;

	/* enable interrupts */
	sc->sc_intenable(sc, 1);

	return(0);
}


int
qmsclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct qms_softc *sc = qms_cd.cd_devs[unit];

 	/* disable interrupts */
	sc->sc_intenable(sc, 0);

	/* clean up */
	sc->sc_proc = NULL;
	sc->sc_state = 0;

	clfree(&sc->sc_buffer);

	return(0);
}

int
qmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int unit = minor(dev);
	struct qms_softc *sc = qms_cd.cd_devs[unit];
	int error;
	int s;
	int length;
	u_char buffer[128];

	error = 0;
	s = spltty();
	while (sc->sc_buffer.c_cc == 0) {
		if (flag & IO_NDELAY) {
			(void)splx(s);
			return(EWOULDBLOCK);
		}
		sc->sc_state |= QMOUSE_ASLEEP;
		if ((error = tsleep((caddr_t)sc, PZERO | PCATCH, "qmsread", 0))) {
			sc->sc_state &= ~QMOUSE_ASLEEP;
			(void)splx(s);
			return(error);
		}
	}
	
	while (sc->sc_buffer.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_buffer.c_cc, uio->uio_resid);
		if(length>sizeof(buffer))
			length=sizeof(buffer);

		(void)q_to_b(&sc->sc_buffer, buffer, length);

		if ((error = (uiomove(buffer, length, uio))))
			break;
	}
	(void)splx(s);
	return(error);
}


#define FMT_START								\
	int x = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX) & 0xffff;	\
	int y = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY) & 0xffff;	\
	int b = bus_space_read_1(sc->sc_iot, sc->sc_butioh, QMS_BUTTONS) & 0x70;\
	if (x & 0x8000) x |= 0xffff0000;					\
	if (y & 0x8000) y |= 0xffff0000;					\
	x = (x - sc->origx);							\
	y = (y - sc->origy);							\
	if (x < (sc->boundx)) x = sc->boundx;					\
	if (x > (sc->bounda)) x = sc->bounda;					\
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX, x + sc->origx);	\
	if (y < (sc->boundy)) y = sc->boundy;					\
	if (y > (sc->boundb)) y = sc->boundb;					\
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY, y + sc->origy);	\
	x = x * sc->xmult;							\
	y = y * sc->ymult;

#define	FMT_END


int
qmsioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct qms_softc *sc = qms_cd.cd_devs[minor(dev)];

	switch (cmd) {
	case MOUSEIOC_WRITEX:
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX,
		    *(int *)data + sc->origx);
		return 0;
	case MOUSEIOC_WRITEY:
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY,
		    *(int *)data + sc->origy);
		return 0;
	case MOUSEIOC_SETSTATE:
	{
		struct mouse_state *co = (void *)data;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX, co->x);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY, co->y);
		return 0;
	}
	case MOUSEIOC_SETBOUNDS:
	{
		struct mouse_boundingbox *bo = (void *)data;
		struct mousebufrec buffer;
#ifdef MOUSE_IOC_ACK
		int s;

		s = spltty();
#endif

		sc->boundx = bo->x;
		sc->boundy = bo->y;
		sc->bounda = bo->a;
		sc->boundb = bo->b;

		buffer.status = IOC_ACK;
		buffer.x = sc->origx;
		buffer.y = sc->origy;
#ifdef MOUSE_IOC_ACK
		if (sc->sc_buffer.c_cc > 0)
			printf("%s: setting bounding with non empty buffer (%d)\n",
			    sc->sc_device.dv_xname, sc->sc_buffer.c_cc);
		qmsputbuffer(sc, &buffer);
		(void)splx(s);
#endif
		return 0;
	}
	case MOUSEIOC_SETMODE:
	{
		struct mousebufrec buffer;
#ifdef MOUSE_IOC_ACK
		int s;

		s = spltty();
#endif
		sc->sc_mode = *(int *)data;

		buffer.status = IOC_ACK;
		buffer.x = sc->origx;
		buffer.y = sc->origy;
#ifdef MOUSE_IOC_ACK
		if (sc->sc_buffer.c_cc > 0)
			printf("%s: setting mode with non empty buffer (%d)\n",
			    sc->sc_device.dv_xname, sc->sc_buffer.c_cc);
		qmsputbuffer(sc, &buffer);
		(void)splx(s);
#endif
		return 0;
	}
	case MOUSEIOC_SETORIGIN:
	{
		struct mouse_origin *oo = (void *)data;
		struct mousebufrec buffer;
#ifdef MOUSE_IOC_ACK
		int s;

		s = spltty();
#endif
		/* Need to fix up! */
		sc->origx = oo->x;
		sc->origy = oo->y;

		buffer.status = IOC_ACK;
		buffer.x = sc->origx;
		buffer.y = sc->origy;
#ifdef MOUSE_IOC_ACK
		if (sc->sc_buffer.c_cc > 0)
			printf("%s: setting origin with non empty buffer (%d)\n",
			    sc->sc_device.dv_xname, sc->sc_buffer.c_cc);
		qmsputbuffer(sc, &buffer);
		(void)splx(s);
#endif
		return 0;
	}
	case MOUSEIOC_GETSTATE:
	{
		struct mouse_state *co = (void *)data;
		FMT_START
		co->x = x;
		co->y = y;
		co->buttons = b ^ 0x70;
		FMT_END
		return 0;
	}
	case MOUSEIOC_GETBOUNDS:
	{
		struct mouse_boundingbox *bo = (void *)data;
		bo->x = sc->boundx;
		bo->y = sc->boundy;
		bo->a = sc->bounda;
		bo->b = sc->boundb;
		return 0;
	}
	case MOUSEIOC_GETORIGIN:
	{
		struct mouse_origin *oo = (void *)data;
		oo->x = sc->origx;
		oo->y = sc->origy;
		return 0;
	}
	}   

	return (EINVAL);
}


int
qmsintr(arg)
	void *arg;
{
	struct qms_softc *sc = arg;
	int s;
	struct mousebufrec buffer;
	int dosignal=0;

	FMT_START

	b &= 0x70;
	b >>= 4;
        if (x != sc->lastx || y != sc->lasty || b != sc->lastb) {
		/* Mouse state changed */
		buffer.status = b | ( b ^ sc->lastb) << 3 | (((x==sc->lastx) && (y==sc->lasty))?0:MOVEMENT);
		if(sc->sc_mode == MOUSEMODE_REL) {
			sc->origx = sc->origy = 0;
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX, sc->origx);
			bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY, sc->origy);
		}
		buffer.x = x;
		buffer.y = y;
		microtime(&buffer.event_time);

		if (sc->sc_buffer.c_cc == 0)
			dosignal = 1;

		s = spltty();
		(void)b_to_q((char *)&buffer, sizeof(buffer), &sc->sc_buffer);
		(void)splx(s);
		selwakeup(&sc->sc_rsel);

		if (sc->sc_state & QMOUSE_ASLEEP) {
			sc->sc_state &= ~QMOUSE_ASLEEP;
			wakeup((caddr_t)sc);
		}

/*		if (dosignal)*/
			psignal(sc->sc_proc, SIGIO);
		
		sc->lastx = x;
		sc->lasty = y;
		sc->lastb = b;
	}

	FMT_END
	return(0);	/* Pass interrupt on down the chain */
}


int
qmspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct qms_softc *sc = qms_cd.cd_devs[minor(dev)];
	int revents = 0;
	int s = spltty();

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_buffer.c_cc > 0)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}

	(void)splx(s);
	return (revents);
}


#ifdef MOUSE_IOC_ACK
static void
qmsputbuffer(sc, buffer)
	struct qms_softc *sc;
	struct mousebufrec *buffer;
{
	int s;
	int dosignal = 0;

	/* Time stamp the buffer */
	microtime(&buffer->event_time);

	if (sc->sc_buffer.c_cc == 0)
		dosignal=1;

	s = spltty();
	(void)b_to_q((char *)buffer, sizeof(*buffer), &sc->sc_buffer);
	(void)splx(s);
	selwakeup(&sc->sc_rsel);

	if (sc->sc_state & QMOUSE_ASLEEP) {
		sc->sc_state &= ~QMOUSE_ASLEEP;
		wakeup((caddr_t)sc);
	}

	if (dosignal)
		psignal(sc->sc_proc, SIGIO);
}
#endif

/* End of qms.c */
