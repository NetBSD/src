/*	$NetBSD: pms.c,v 1.22 2001/03/18 17:00:56 rearnsha Exp $	*/

/*-
 * Copyright (c) 1996 D.C. Tsen
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the RiscBSD team.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from:pms.c,v 1.24 1995/12/24 02:30:28 mycroft Exp
 */

/*
 * Ported from 386 version of PS/2 mouse driver.
 * D.C. Tsen
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <machine/bus.h>
#include <machine/conf.h>
#include <machine/mouse.h>
#include <arm32/dev/pmsvar.h>

#define MOUSE_IOC_ACK

/* mouse commands */
#define	PMS_SET_SCALE11	0xe6	/* set scaling 1:1 */
#define	PMS_SET_SCALE21 0xe7	/* set scaling 2:1 */
#define	PMS_SET_RES	0xe8	/* set resolution */
#define	PMS_GET_SCALE	0xe9	/* get scaling factor */
#define	PMS_SET_STREAM	0xea	/* set streaming mode */
#define	PMS_SET_SAMPLE	0xf3	/* set sampling rate */
#define	PMS_DEV_ENABLE	0xf4	/* mouse on */
#define	PMS_DEV_DISABLE	0xf5	/* mouse off */
#define	PMS_RESET	0xff	/* reset */

#define	PMS_CHUNK	128	/* chunk size for read */
#define	PMS_BSIZE	(20*64)	/* buffer size */

/* function prototypes */

void pmswatchdog	__P((void *));
#ifdef MOUSE_IOC_ACK
static void pmsputbuffer __P((struct pms_softc *sc, struct mousebufrec *buf));
#endif
static __inline void pms_flush __P((struct pms_softc *sc));

extern struct cfdriver opms_cd;

/* pms device driver structure */

#define PMS_DATA	0
#define PMS_CR		1
#define PMS_STATUS	1

#define PMS_CR_ENABLE	0x08
#define PMS_CR_KDATAO	0x02
#define PMS_CR_KCLKO	0x01

#define PMS_ST_TXE	0x80
#define PMS_ST_TXB	0x40
#define PMS_ST_RXF	0x20
#define PMS_ST_RXB	0x10
#define PMS_ST_ENABLE	0x08
#define PMS_ST_RXPARITY	0x04
#define PMS_ST_KDATAI	0x02
#define PMS_ST_KCLKI	0x01

#define	PMSUNIT(dev)	(minor(dev))

/*
 * Flush any pending mouse data
 */

static __inline void
pms_flush(sc)
	struct pms_softc *sc;
{
	int n = 1000;

	while (n-- && (bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_STATUS)
	    & PMS_ST_RXF)) {
		delay(6);
		(void) bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
		delay(6);
		(void) bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
		delay(6);
		(void) bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
	}
}

/*
 * Send a command to the mouse
 */

static int
cmd_mouse(sc, cmd)
	struct pms_softc * sc;
	u_char cmd;
{
	int c;
	int i = 0;
	int retry = 10;

	for (i = 0; i < 1000; i++) {
		if (bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_STATUS) & PMS_ST_TXE)
			break;
		delay(2);
	}
	if (i == 1000)
		printf("Mouse transmit not ready\n");

resend:
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, PMS_DATA, cmd);
	delay(2);
	c = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_STATUS);
	i = 1000;
	while (i-- && !(c & PMS_ST_RXF)) {
		delay(1);
		c = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_STATUS);
	}

	delay(10000);

	c = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
	if ((c == 0xFA) || (c == 0xEE))
		return(0);

	if (--retry) {
		pms_flush(sc);
		goto resend;
	}

	printf("%s: Mouse cmd failed, cmd = %x, status = %x\n", sc->sc_dev.dv_xname, cmd, c);
	return(1);
}

/*
 * initialise the mouse
 *
 *
 * The sets up parts of the softc and then tests the mouse
 * to make sure it is responding.
 * It then configures the mouse as required.
 */

int
pmsinit(sc)
	struct pms_softc *sc;
{
	int i, j;
	int mid;

	/* Set up softc */
	callout_init(&sc->sc_watchdog_ch);
	sc->sc_state = 0;
	sc->origx = 0;
	sc->origy = 0;
	sc->boundx = -4095;
	sc->boundy = -4095;
	sc->bounda = 4096;
	sc->boundb = 4096;

	/* Enable the mouse */

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, PMS_CR, PMS_CR_ENABLE);

	i = 0;
	while ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_STATUS) & 0x03)
	     != 0x03) {
		if (i++ > 10) {
			printf("Mouse not found, status = <%x>.\n",
			    bus_space_read_1(sc->sc_iot, sc->sc_ioh,
			    PMS_STATUS));
			return(0);
		}
		pms_flush(sc);
		delay(2);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, PMS_CR, PMS_CR_ENABLE);
	}

	pms_flush(sc);

	/*
	 * Disable, reset and enable the mouse.
	 */
	if (cmd_mouse(sc, PMS_DEV_DISABLE))
		return(0);

	cmd_mouse(sc, PMS_RESET);
	delay(300000);
	j = 10;
	i = 0;
	while ((mid = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA)) != 0xAA) {
		if (++i > 500) {
			if (--j < 0) {
				printf("Mouse Reset failed, status = <%x>.\n", mid);
				return(0);
			}
			pms_flush(sc);
			cmd_mouse(sc, PMS_RESET);
			i = 0;
		}
		delay(100000);
	}
	mid = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
#if 0
	cmd_mouse(sc, PMS_SET_RES);
	cmd_mouse(sc, 3);		/* 8 counts/mm */
	cmd_mouse(sc, PMS_SET_SCALE21);
#endif
	cmd_mouse(sc, PMS_SET_SAMPLE);
	cmd_mouse(sc, 40);	/* 40 samples/sec */
	cmd_mouse(sc, PMS_SET_STREAM);
	cmd_mouse(sc, PMS_DEV_ENABLE);
	return(1);
}

/*
 * device open routine
 */
int
pmsopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = PMSUNIT(dev);
	struct pms_softc *sc;

	/* validate the unit and the softc */
	if (unit >= opms_cd.cd_ndevs)
		return ENXIO;
	sc = opms_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	/* Are we already open ? */
	if (sc->sc_state & PMS_OPEN)
		return EBUSY;

	/* initialise buffer */
	if (clalloc(&sc->sc_q, PMS_BSIZE, 0) == -1)
		return ENOMEM;

	/* set up the softc structure */
	sc->sc_proc = p;
	sc->sc_mode = MOUSEMODE_ABS;
	sc->sc_state |= PMS_OPEN;
	sc->sc_status = 0;
	sc->sc_x = sc->sc_y = 0;
	sc->lastx = -1;
	sc->lasty = -1;
	sc->lastb = -1;

	/* enable interrupts */
	sc->sc_intenable(sc, 1);
	
	/* install watchdog timeout */
	callout_reset(&sc->sc_watchdog_ch, 30 * hz,
	    pmswatchdog, sc);

	return(0);
}

/*
 * driver close function
 */
int
pmsclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct pms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];

	/* remove the timeout */
	callout_stop(&sc->sc_watchdog_ch);

	/* disable interrupts */
	sc->sc_intenable(sc, 0);
 
 	/* clean up*/
	sc->sc_proc = NULL;
	sc->sc_state &= ~PMS_OPEN;
	sc->sc_x = sc->sc_y = 0;

	clfree(&sc->sc_q);

	return 0;
}

int
pmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct pms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	int s;
	int error = 0;
	size_t length;
	u_char buffer[PMS_CHUNK];

	/* Block until mouse activity occured. */

	s = spltty();
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			(void)splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= PMS_ASLP;
		if ((error = tsleep((caddr_t)sc, (PZERO | PCATCH), "pmsread", 0))) {
			sc->sc_state &= ~PMS_ASLP;
			(void)splx(s);
			return error;
		}
	}

	/* Transfer as many chunks as possible. */

	while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_q.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from the input queue. */
		(void) q_to_b(&sc->sc_q, buffer, length);

		/* Copy the data to the user process. */
		if ((error = uiomove(buffer, length, uio)))
			break;
	}
	(void)splx(s);

	return error;
}

int
pmsioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct pms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	struct mouseinfo info;
	int s;
	int error = 0;

	s = spltty();

	switch (cmd) {
	case MOUSEIOC_SETSTATE:
		printf("MOUSEIOC_SETSTATE called\n");
		break;
	case MOUSEIOC_WRITEX:
		sc->sc_x = *(int *) addr;
		break;
	case MOUSEIOC_WRITEY:
		sc->sc_y = *(int *) addr;
		break;
	case MOUSEIOC_SETBOUNDS:
	{
		register struct mouse_boundingbox *bo = (void *) addr;
		struct mousebufrec buffer;

		sc->boundx = bo->x; sc->boundy = bo->y;
		sc->bounda = bo->a; sc->boundb = bo->b;

		buffer.status = IOC_ACK;
		buffer.x = sc->origx;
		buffer.y = sc->origy;
#ifdef MOUSE_IOC_ACK
		pmsputbuffer(sc, &buffer);
#endif
		break;
	}
	case MOUSEIOC_SETMODE:
	{
		struct mousebufrec buffer;
		sc->sc_mode = *(int *)addr;

		buffer.status = IOC_ACK;
		buffer.x = sc->origx;
		buffer.y = sc->origy;
#ifdef MOUSE_IOC_ACK
		if (sc->sc_q.c_cc > 0)
			printf("%s: setting mode with non empty buffer (%d)\n",
			    sc->sc_dev.dv_xname, sc->sc_q.c_cc);
		pmsputbuffer(sc, &buffer);
#endif
		break;
	}
	case MOUSEIOC_SETORIGIN:
	{
		register struct mouse_origin *oo = (void *) addr;
		struct mousebufrec buffer;

		sc->origx = oo->x;
		sc->origy = oo->y;

		buffer.status = IOC_ACK;
		buffer.x = sc->origx;
		buffer.y = sc->origy;
#ifdef MOUSE_IOC_ACK
		pmsputbuffer(sc, &buffer);
#endif
		break;
	}
	case MOUSEIOC_GETBOUNDS:
	{
		register struct mouse_boundingbox *bo = (void *) addr;
		bo->x = sc->boundx; bo->y = sc->boundy;
		bo->a = sc->bounda; bo->b = sc->boundb;
		break;
	}
	case MOUSEIOC_GETORIGIN:
	{
		register struct mouse_origin *oo = (void *) addr;
		oo->x = sc->origx;
		oo->y = sc->origy;
		break;
	}
	case MOUSEIOC_GETSTATE:
		info.status = sc->sc_status;
		if (sc->sc_x || sc->sc_y)
			info.status |= MOVEMENT;

#if 1
		info.xmotion = sc->sc_x;
		info.ymotion = sc->sc_y;
#else
		if (sc->sc_x > sc->bounda)
			info.xmotion = sc->bounda;
		else if (sc->sc_x < sc->boundx)
			info.xmotion = sc->boundx;
		else
			info.xmotion = sc->sc_x;

		if (sc->sc_y > sc->boundb)
			info.ymotion = sc->boundb;
		else if (sc->sc_y < sc->boundy)
			info.ymotion = sc->boundy;
		else
			info.ymotion = sc->sc_y;
#endif

		/* Reset historical information. */
		sc->sc_x = sc->sc_y = 0;
		sc->sc_status &= ~BUTCHNGMASK;
		ndflush(&sc->sc_q, sc->sc_q.c_cc);

		error = copyout(&info, addr, sizeof(struct mouseinfo));
		break;

	default:
		error = EINVAL;
		break;
	}
	(void)splx(s);

	return error;
}

/* Masks for the first byte of a packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04

#define XNEG_MASK	0x10
#define YNEG_MASK	0x20

int
pmsintr(arg)
	void *arg;
{
	struct pms_softc *sc = arg;
	static int state = 0;
	static u_char buttons;
	u_char changed;
	static int dx, dy;
	int dosignal = 0;
	int s;
	u_char b;
	struct mousebufrec mbuffer;

	if ((sc->sc_state & PMS_OPEN) == 0) {
		/* Interrupts are not expected.  Discard the byte. */
		pms_flush(sc);
		return(-1);	/* Could have been ours but pass it on */
	}

	switch (state) {
	case 0:
		buttons = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
		if ((buttons & 0xc0) == 0)
			++state;
		break;

	case 1:
		dx = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
		dx = (buttons & XNEG_MASK) ? -dx : dx;
		++state;
		break;

	case 2:
		dy = bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_DATA);
		dy = (buttons & YNEG_MASK) ? -dy : dy;
		state = 0;

		buttons = ((buttons & PS2LBUTMASK) << 2) |
			  ((buttons & (PS2RBUTMASK | PS2MBUTMASK)) >> 1);
		/*
		 * Ahhh, the Xarm server interrupt button bits reversed.
		 * If bit is set to 1, it means button is not pressed which is
		 * not what PS/2 mouse button bits said.  Since we are simulating
		 * the quadmouse, that's not too picky about it.
		 */
		buttons ^= BUTSTATMASK;
		changed = ((buttons ^ sc->sc_status) & BUTSTATMASK) << 3;
		sc->sc_status = buttons | (sc->sc_status & ~BUTSTATMASK) | changed;

		if (dx || dy || changed) {
			if (dx < 0)
				dx = -(256 + dx);
			if (dy < 0)
				dy = -(256 + dy);

			/* Update accumulated movements. */
			sc->sc_x += dx;
			sc->sc_y += dy;

			if (sc->sc_x > sc->bounda)
				sc->sc_x = sc->bounda;
			else if (sc->sc_x < sc->boundx)
				sc->sc_x = sc->boundx;

			if (sc->sc_y > sc->boundb)
				sc->sc_y = sc->boundb;
			else if (sc->sc_y < sc->boundy)
				sc->sc_y = sc->boundy;

			if (sc->sc_q.c_cc == 0)
				dosignal = 1;

			/* Add this event to the queue. */
			b = buttons & BUTSTATMASK;
			mbuffer.status = b | (b ^ sc->lastb) << 3
			    | (((sc->sc_x==sc->lastx) && (sc->sc_y==sc->lasty))?0:MOVEMENT);
			mbuffer.x = sc->sc_x * 2;
			mbuffer.y = sc->sc_y * 2;
			microtime(&mbuffer.event_time);
			s = spltty();
			(void) b_to_q((char *)&mbuffer, sizeof(mbuffer), &sc->sc_q);
			(void) splx(s);
			sc->lastx = sc->sc_x;
			sc->lasty = sc->sc_y;
			sc->lastb = b;

			if(sc->sc_mode == MOUSEMODE_REL) {
				sc->origx = sc->origy = 0;
				sc->sc_x = sc->sc_y = 0;
			}

			selwakeup(&sc->sc_rsel);
			if (sc->sc_state & PMS_ASLP) {
				sc->sc_state &= ~PMS_ASLP;
				wakeup((caddr_t)sc);
			}
/*			if (dosignal)*/
				psignal(sc->sc_proc, SIGIO);
		}

		break;
	}

	return(1);	/* Claim interrupt */
}


int
pmspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct pms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	int revents = 0;
	int s = spltty();

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_q.c_cc > 0)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}

	(void)splx(s);
	return (revents);
}


void
pmswatchdog(arg)
	void *arg;
{
	struct pms_softc *sc = arg;
	int s;

	if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, PMS_STATUS) & 0x03)
	    != 0x03) {
		printf("%s: Mouse is dead (%x), restart it\n",
		    sc->sc_dev.dv_xname, bus_space_read_1(sc->sc_iot,
		    sc->sc_ioh, PMS_STATUS));
		s = spltty();
		pmsinit(sc);
		(void)splx(s);
	}
}

#ifdef MOUSE_IOC_ACK
static void
pmsputbuffer(sc, buffer)
	struct pms_softc *sc;
	struct mousebufrec *buffer;
{
	int s;
	int dosignal = 0;

	/* Time stamp the buffer */
	microtime(&buffer->event_time);

	if (sc->sc_q.c_cc == 0)
		dosignal = 1;

	s=spltty();
	(void)b_to_q((char *)buffer, sizeof(*buffer), &sc->sc_q);
	(void)splx(s);
	selwakeup(&sc->sc_rsel);

	if (sc->sc_state & PMS_ASLP) {
		sc->sc_state &= ~PMS_ASLP;
		wakeup((caddr_t)sc);
	}

	if (dosignal)
		psignal(sc->sc_proc, SIGIO);
}
#endif

/* End of pms.c */
