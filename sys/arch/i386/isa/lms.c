/*-
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: lms.c,v 1.7 1993/12/20 09:06:17 mycroft Exp $
 */

#include "lms.h"
#if NLMS > 0

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#ifdef NetBSD
#include <sys/select.h>
#endif
#include <sys/proc.h>
#include <sys/vnode.h>

#include <machine/mouse.h>
#include <machine/pio.h>

#include <i386/isa/isa_device.h>

#define DATA	0       /* Offset for data port, read-only */
#define SIGN	1       /* Offset for signature port, read-write */
#define INTR	2       /* Offset for interrupt port, read-only */
#define CNTRL	2       /* Offset for control port, write-only */
#define CONFIG	3	/* for configuration port, read-write */

#define LMSUNIT(dev)	(minor(dev) >> 1)

#ifndef min
#define min(x,y) (x < y ? x : y)
#endif  min

int lmsprobe (struct isa_device *);
int lmsattach (struct isa_device *);

static int lmsaddr[NLMS];	/* Base I/O port addresses per unit */

#define MSBSZ	1024		/* Output queue size (pwr of 2 is best) */

struct ringbuf {
	int count, first, last;
	char queue[MSBSZ];
};

static struct lms_softc {	/* Driver status information */
	struct ringbuf inq;	/* Input queue */
#ifdef NetBSD
	struct selinfo rsel;
#else
	pid_t	rsel;		/* Process selecting for Input */
#endif
	unsigned char state;	/* Mouse driver state */
	unsigned char status;	/* Mouse button status */
	unsigned char button;	/* Previous mouse button status bits */
	int x, y;		/* accumulated motion in the X,Y axis */
} lms_softc[NLMS];

#define OPEN	1		/* Device is open */
#define ASLP	2		/* Waiting for mouse data */

struct isa_driver lmsdriver = { lmsprobe, lmsattach, "lms" };

int lmsprobe(struct isa_device *dvp)
{
	int ioport = dvp->id_iobase;
	int val;

	/* Configure and check for port present */

	outb(ioport+CONFIG, 0x91);
	DELAY(10);
	outb(ioport+SIGN, 0x0C);
	DELAY(10);
	val = inb(ioport+SIGN);
	DELAY(10);
	outb(ioport+SIGN, 0x50);

	/* Check if something is out there */

	if (val == 0x0C && inb(ioport+SIGN) == 0x50)
		return(4);

	/* Not present */

	return(0);
}

int lmsattach(struct isa_device *dvp)
{
	int unit = dvp->id_unit;
	int ioport = dvp->id_iobase;
	struct lms_softc *sc = &lms_softc[unit];

	/* Save I/O base address */

	lmsaddr[unit] = ioport;

	/* Disable mouse interrupts */

	outb(ioport+CNTRL, 0x10);

	/* Setup initial state */

	sc->state = 0;

	/* Done */

	return(0);
}

int lmsopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit = LMSUNIT(dev);
	struct lms_softc *sc;
	int ioport;

	/* Validate unit number */

	if (unit >= NLMS)
		return(ENXIO);

	/* Get device data */

	sc = &lms_softc[unit];
	ioport = lmsaddr[unit];

	/* If device does not exist */

	if (ioport == 0)
		return(ENXIO);

	/* Disallow multiple opens */

	if (sc->state & OPEN)
		return(EBUSY);

	/* Initialize state */

	sc->state |= OPEN;
#ifdef NetBSD
	sc->rsel.si_pid = 0;
	sc->rsel.si_coll = 0;
#else
	sc->rsel = 0;
#endif
	sc->status = 0;
	sc->button = 0;
	sc->x = 0;
	sc->y = 0;

	/* Allocate and initialize a ring buffer */

	sc->inq.count = sc->inq.first = sc->inq.last = 0;

	/* Enable Bus Mouse interrupts */

	outb(ioport+CNTRL, 0);

	/* Successful open */

	return(0);
}

int lmsclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit, ioport;
	struct lms_softc *sc;

	/* Get unit and associated info */

	unit = LMSUNIT(dev);
	sc = &lms_softc[unit];
	ioport = lmsaddr[unit];

	/* Disable further mouse interrupts */

	outb(ioport+CNTRL, 0x10);

	/* Complete the close */

	sc->state &= ~OPEN;

	/* close is almost always successful */

	return(0);
}

int lmsread(dev_t dev, struct uio *uio, int flag)
{
	int s;
	int error = 0;	/* keep compiler quiet, even though initialisation
			   is unnecessary */
	unsigned length;
	struct lms_softc *sc;
	unsigned char buffer[100];

	/* Get device information */

	sc = &lms_softc[LMSUNIT(dev)];

	/* Block until mouse activity occured */

	s = spltty();
	while (sc->inq.count == 0) {
		if (minor(dev) & 0x1) {
			splx(s);
			return(EWOULDBLOCK);
		}
		sc->state |= ASLP;
		error = tsleep((caddr_t)sc, PZERO | PCATCH, "lmsrea", 0);
		if (error != 0) {
			splx(s);
			return(error);
		}
	}

	/* Transfer as many chunks as possible */

	while (sc->inq.count > 0 && uio->uio_resid > 0) {
		length = min(sc->inq.count, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from input queue */

		if (sc->inq.first + length >= MSBSZ) {
			bcopy(&sc->inq.queue[sc->inq.first], 
		 	      buffer, MSBSZ - sc->inq.first);
			bcopy(sc->inq.queue, &buffer[MSBSZ-sc->inq.first], 
			      length - (MSBSZ - sc->inq.first));
		}
		else
			bcopy(&sc->inq.queue[sc->inq.first], buffer, length);
	
		sc->inq.first = (sc->inq.first + length) % MSBSZ;
		sc->inq.count -= length;

		/* Copy data to user process */

		error = uiomove(buffer, length, uio);
		if (error)
			break;
	}

	sc->x = sc->y = 0;

	/* Allow interrupts again */

	splx(s);
	return(error);
}

int lmsioctl(dev_t dev, caddr_t addr, int cmd, int flag, struct proc *p)
{
	struct lms_softc *sc;
	struct mouseinfo info;
	int s, error;

	/* Get device information */

	sc = &lms_softc[LMSUNIT(dev)];

	/* Perform IOCTL command */

	switch (cmd) {

	case MOUSEIOCREAD:

		/* Don't modify info while calculating */

		s = spltty();

		/* Build mouse status octet */

		info.status = sc->status;
		if (sc->x || sc->y)
			info.status |= MOVEMENT;

		/* Encode X and Y motion as good as we can */

		if (sc->x > 127)
			info.xmotion = 127;
		else if (sc->x < -127)
			info.xmotion = -127;
		else
			info.xmotion = sc->x;

		if (sc->y > 127)
			info.ymotion = 127;
		else if (sc->y < -127)
			info.ymotion = -127;
		else
			info.ymotion = sc->y;

		/* Reset historical information */

		sc->x = 0;
		sc->y = 0;
		sc->status &= ~BUTCHNGMASK;

		/* Allow interrupts and copy result buffer */

		splx(s);
		error = copyout(&info, addr, sizeof(struct mouseinfo));
		break;

	default:
		error = EINVAL;
		break;
		}

	/* Return error code */

	return(error);
}

void lmsintr(unit)
	int unit;
{
	struct lms_softc *sc = &lms_softc[unit];
	int ioport = lmsaddr[unit];
	char hi, lo, dx, dy, buttons, changed;

	outb(ioport+CNTRL, 0xAB);
	hi = inb(ioport+DATA) & 15;
	outb(ioport+CNTRL, 0x90);
	lo = inb(ioport+DATA) & 15;
	dx = (hi << 4) | lo;
	dx = (dx == -128) ? -127 : dx;

	outb(ioport+CNTRL, 0xF0);
	hi = inb(ioport+DATA);
	outb(ioport+CNTRL, 0xD0);
	lo = inb(ioport+DATA);
	outb(ioport+CNTRL, 0);
	dy = ((hi & 15) << 4) | (lo & 15);
	dy = (dy == -128) ? 127 : -dy;

	buttons = (~hi >> 5) & 7;
	changed = buttons ^ sc->button;
	sc->button = buttons;
	sc->status = buttons | (sc->status & ~BUTSTATMASK) | (changed << 3);

	/* Update accumulated movements */

	sc->x += dx;
	sc->y += dy;

	/* If device in use and a change occurred... */

	if (sc->state & OPEN && (dx || dy || changed)) {
		sc->inq.queue[sc->inq.last++] = 0x80 | (buttons ^ BUTSTATMASK);
		sc->inq.queue[sc->inq.last++ % MSBSZ] = dx;
		sc->inq.queue[sc->inq.last++ % MSBSZ] = dy;
		sc->inq.queue[sc->inq.last++ % MSBSZ] = 0;
		sc->inq.queue[sc->inq.last++ % MSBSZ] = 0;
		sc->inq.last = sc->inq.last % MSBSZ;
		sc->inq.count += 5;

		if (sc->state & ASLP) {
			sc->state &= ~ASLP;
			wakeup((caddr_t)sc);
		}
#ifdef NetBSD
		selwakeup(&sc->rsel);
#else
		if (sc->rsel) {
			selwakeup(sc->rsel, 0);
			sc->rsel = 0;
		}
#endif
	}
}

int lmsselect(dev_t dev, int rw, struct proc *p)
{
	int s, ret;
	struct lms_softc *sc = &lms_softc[LMSUNIT(dev)];

	/* Silly to select for output */

	if (rw == FWRITE)
		return(0);

	/* Return true if a mouse event available */

	s = spltty();
	if (sc->inq.count)
		ret = 1;
	else {
#ifdef NetBSD
		selrecord(p, &sc->rsel);
#else
		sc->rsel = p->p_pid;
#endif
		ret = 0;
	}
	splx(s);

	return(ret);
}
#endif
