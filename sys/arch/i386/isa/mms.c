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
 */

/*
 *  Ported to 386bsd Oct 17, 1992 
 *  Sandi Donno, Computer Science, University of Cape Town, South Africa
 *  Please send bug reports to sandi@cs.uct.ac.za
 *
 *  Thanks are also due to Rick Macklem, rick@snowhite.cis.uoguelph.ca -
 *  although I was only partially successful in getting the alpha release
 *  of his "driver for the Logitech and ATI Inport Bus mice for use with 
 *  386bsd and the X386 port" to work with my Microsoft mouse, I nevertheless 
 *  found his code to be an invaluable reference when porting this driver 
 *  to 386bsd.
 *
 *  Further modifications for latest 386BSD+patchkit and port to NetBSD,
 *  Andrew Herbert <andrew@werple.apana.org.au> - 8 June 1993
 */

/* define this if you're using 386BSD rather than NetBSD */
/* #define 386BSD_KERNEL */

#include "mms.h"

#if NMMS > 0

#include "param.h"
#include "kernel.h"
#include "systm.h"
#include "buf.h"
#include "malloc.h"
#include "ioctl.h"
#include "tty.h"
#include "file.h"
#ifndef 386BSD_KERNEL
#include "select.h"
#endif
#include "proc.h"
#include "vnode.h"

#include "i386/include/mouse.h"
#include "i386/include/pio.h"		/* Julian's fast IO macros */
#include "i386/isa/isa_device.h"

#define ADDR	0	/* Offset for register select */
#define DATA	1	/* Offset for InPort data */
#define IDENT	2	/* Offset for identification register */

#define MMSUNIT(dev)	(minor(dev) >> 1)

#ifndef min
#define min(x,y) (x < y ? x : y)
#endif  min

int mmsprobe (struct isa_device *);
int mmsattach (struct isa_device *);

static int mmsaddr[NMMS];	/* Base I/O port addresses per unit */

#define MSBSZ	1024		/* Output queue size (pwr of 2 is best) */

struct ringbuf {
	int count, first, last;
	char queue[MSBSZ];
};

static struct mms_softc {	/* Driver status information */
	struct ringbuf inq;	/* Input queue */
#ifdef 386BSD_KERNEL
	pid_t	rsel;		/* Process selecting for Input */
#else
	struct selinfo rsel;
#endif
	unsigned char state;	/* Mouse driver state */
	unsigned char status;	/* Mouse button status */
	int x, y;		/* accumulated motion in the X,Y axis */
} mms_softc[NMMS];

#define OPEN	1		/* Device is open */
#define ASLP	2		/* Waiting for mouse data */

struct isa_driver mmsdriver = { mmsprobe, mmsattach, "mms" };

int mmsprobe(struct isa_device *dvp)
{
	int ioport = dvp->id_iobase;

	/* Read identification register to see if present */

	if (inb(ioport+IDENT) != 0xDE)
		return(0);

	/* Seems it was there; reset */

	outb(ioport+ADDR, 0x87);
	return(1);
}

int mmsattach(struct isa_device *dvp)
{
	int unit = dvp->id_unit;
	int ioport = dvp->id_iobase;
	struct mms_softc *sc = &mms_softc[unit];

	/* Save I/O base address */

	mmsaddr[unit] = ioport;

	/* Setup initial state */

	sc->state = 0;

	/* Done */

	return(0);
}

int mmsopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit = MMSUNIT(dev);
	struct mms_softc *sc;
	int ioport;

	/* Validate unit number */

	if (unit >= NMMS)
		return(ENXIO);

	/* Get device data */

	sc = &mms_softc[unit];
	ioport = mmsaddr[unit];

	/* If device does not exist */

	if (ioport == 0)
		return(ENXIO);

	/* Disallow multiple opens */

	if (sc->state & OPEN)
		return(ENXIO);

	/* Initialize state */

	sc->state |= OPEN;
#ifdef 386BSD_KERNEL
	sc->rsel = 0;
#else
	sc->rsel.si_pid = 0;
	sc->rsel.si_coll = 0;
#endif
	sc->status = 0;
	sc->x = 0;
	sc->y = 0;

	/* Allocate and initialize a ring buffer */

	sc->inq.count = sc->inq.first = sc->inq.last = 0;

	/* Setup Bus Mouse */

	outb(ioport+ADDR, 7);
	outb(ioport+DATA, 0x09);

	/* Successful open */

	return(0);
}

int mmsclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit, ioport;
	struct mms_softc *sc;

	/* Get unit and associated info */

	unit = MMSUNIT(dev);
	sc = &mms_softc[unit];
	ioport = mmsaddr[unit];

	/* Reset Bus Mouse */

	outb(ioport+ADDR, 0x87);

	/* Complete the close */

	sc->state &= ~OPEN;

	/* close is almost always successful */

	return(0);
}

int mmsread(dev_t dev, struct uio *uio, int flag)
{
	int s, error = 0;
	unsigned length;
	struct mms_softc *sc;
	unsigned char buffer[100];

	/* Get device information */

	sc = &mms_softc[MMSUNIT(dev)];

	/* Block until mouse activity occured */

	s = spltty();
	while (sc->inq.count == 0) {
		if (minor(dev) & 0x1) {
			splx(s);
			return(EWOULDBLOCK);
		}
		sc->state |= ASLP;
		error = tsleep(sc, PZERO | PCATCH, "mmsrea", 0);
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

int mmsioctl(dev_t dev, caddr_t addr, int cmd, int flag, struct proc *p)
{
	struct mms_softc *sc;
	struct mouseinfo info;
	int s, error;

	/* Get device information */

	sc = &mms_softc[MMSUNIT(dev)];

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
		else if (sc->x < -128)
			info.xmotion = -128;
		else
			info.xmotion = sc->x;

		if (sc->y > 127)
			info.ymotion = 127;
		else if (sc->y < -128)
			info.ymotion = -128;
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

void mmsintr(unit)
	int unit;
{
	struct mms_softc *sc = &mms_softc[unit];
	int ioport = mmsaddr[unit];
	char dx, dy, status;

	/* Freeze InPort registers (disabling interrupts) */

	outb(ioport+ADDR, 7);
	outb(ioport+DATA, 0x29);

	/* Read mouse status */

	outb(ioport+ADDR, 0);
	status = inb(ioport+DATA);

	/* Check if any movement detected */

	if (status & 0x40) {
		outb(ioport+ADDR, 1);
		dx = inb(ioport+DATA);
		outb(ioport+ADDR, 2);
		dy = inb(ioport+DATA);
		dy = (dy == -128) ? 127 : -dy;
	}
	else
		dx = dy = 0;

	/* Unfreeze InPort Registers (re-enables interrupts) */

	outb(ioport+ADDR, 7);
	outb(ioport+DATA, 0x09);

	/* Update accumulated movements */

	sc->x += dx;
	sc->y += dy;

	/* Inclusive OR status changes, but always save only last state */

	sc->status |= status & BUTCHNGMASK;
	sc->status = (sc->status & ~BUTSTATMASK) | (status & BUTSTATMASK);

	/* If device in use and a change occurred... */

	if (sc->state & OPEN && status & 0x78 && sc->inq.count < (MSBSZ-5)) {
		status &= BUTSTATMASK;
		sc->inq.queue[sc->inq.last++] = 0x80 | (status ^ BUTSTATMASK);
		sc->inq.queue[sc->inq.last++ % MSBSZ] = dx;
		sc->inq.queue[sc->inq.last++ % MSBSZ] = dy;
		sc->inq.queue[sc->inq.last++ % MSBSZ] = 0;
		sc->inq.queue[sc->inq.last++ % MSBSZ] = 0;
		sc->inq.last = sc->inq.last % MSBSZ;
		sc->inq.count += 5;

		if (sc->state & ASLP) {
			sc->state &= ~ASLP;
			wakeup(sc);
		}
#ifdef 386BSD_KERNEL
		if (sc->rsel) {
			selwakeup(&sc->rsel, 0);
			sc->rsel = 0;
		}
#else
		selwakeup(&sc->rsel);
#endif
		}
}

int mmsselect(dev_t dev, int rw, struct proc *p)
{
	int s, ret;
	struct mms_softc *sc = &mms_softc[MMSUNIT(dev)];

	/* Silly to select for output */

	if (rw == FWRITE)
		return(0);

	/* Return true if a mouse event available */

	s = spltty();
	if (sc->inq.count)
		ret = 1;
	else {
#ifdef 386BSD_KERNEL
		sc->rsel = p->p_pid;
#else
		selrecord(p, &sc->rsel);
#endif
		ret = 0;
	}
	splx(s);

	return(ret);
}
#endif
