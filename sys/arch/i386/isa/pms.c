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
 *
 *  Cloned from the Microsoft Bus Mouse driver, also by Erik Forsberg, by
 *  Andrew Herbert - 12 June 1993 
 *
 *  Modified for PS/2 mouse by Charles Hannum <mycroft@ai.mit.edu>
 *  - 13 June 1993
 */

/* define this if you're using 386BSD rather than NetBSD */
/* #define 386BSD_KERNEL */

#include "pms.h"

#if NPMS > 0

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

#define DATA	0       /* Offset for data port, read-write */
#define CNTRL	4       /* Offset for control port, write-only */
#define STATUS	4	/* Offset for status port, read-only */

/* status bits */
#define	PMS_OUTPUT_ACK	0x02	/* output acknowledge */

/* controller commands */
#define	PMS_ENABLE	0xa7	/* enable auxiliary port */
#define	PMS_DISABLE	0xa8	/* disable auxiliary port */
#define	PMS_INT_ENABLE	0x47	/* enable controller interrupts */
#define	PMS_INT_DISABLE	0x65	/* disable controller interrupts */

/* mouse commands */
#define	PMS_SET_RES	0xe8	/* set resolution */
#define	PMS_SET_SCALE	0xe9	/* set scaling factor */
#define	PMS_SET_STREAM	0xea	/* set streaming mode */
#define	PMS_SET_SAMPLE	0xf3	/* set sampling rate */
#define	PMS_DEV_ENABLE	0xf4	/* mouse on */
#define	PMS_DEV_DISABLE	0xf5	/* mouse off */
#define	PMS_RESET	0xff	/* reset */

#define PMSUNIT(dev)	(minor(dev) >> 1)

#ifndef min
#define min(x,y) (x < y ? x : y)
#endif  min

int pmsprobe (struct isa_device *);
int pmsattach (struct isa_device *);

static int pmsaddr[NPMS];	/* Base I/O port addresses per unit */

#define MSBSZ	1024		/* Output queue size (pwr of 2 is best) */

struct ringbuf {
	int count, first, last;
	char queue[MSBSZ];
};

static struct pms_softc {	/* Driver status information */
	struct ringbuf inq;	/* Input queue */
#ifdef 386BSD_KERNEL
	pid_t	rsel;		/* Process selecting for Input */
#else
	struct selinfo rsel;
#endif
	unsigned char state;	/* Mouse driver state */
	unsigned char status;	/* Mouse button status */
	unsigned char button;	/* Previous mouse button status bits */
	int x, y;		/* accumulated motion in the X,Y axis */
} pms_softc[NPMS];

#define OPEN	1		/* Device is open */
#define ASLP	2		/* Waiting for mouse data */

struct isa_driver pmsdriver = { pmsprobe, pmsattach, "pms" };

int pmsprobe(struct isa_device *dvp)
{
	/* XXX: Needs a real probe routine. */

	return (1);
}

static inline void pms_write(int ioport, u_char value)
{
	outb(0xd4, ioport+CNTRL);
	outb(value, ioport+DATA);
	while (!(inb(ioport+STATUS) & PMS_OUTPUT_ACK));
}

static inline void pms_command(int ioport, u_char value)
{
	outb(0x60, ioport+CNTRL);
	outb(value, ioport+DATA);
}

int pmsattach(struct isa_device *dvp)
{
	int unit = dvp->id_unit;
	int ioport = dvp->id_iobase;
	struct pms_softc *sc = &pms_softc[unit];

	/* Save I/O base address */

	pmsaddr[unit] = ioport;

	/* Disable mouse interrupts */

	pms_write(ioport, PMS_SET_RES);
	pms_write(ioport, 0x03);	/* 8 counts/mm */
	pms_write(ioport, PMS_SET_SCALE);
	pms_write(ioport, 0x02);	/* 2:1 */
	pms_write(ioport, PMS_SET_SAMPLE);
	pms_write(ioport, 0x64);	/* 100 samples/sec */
	pms_write(ioport, PMS_SET_STREAM);

	/* Setup initial state */

	sc->state = 0;

	/* Done */

	return(0);
}

int pmsopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit = PMSUNIT(dev);
	struct pms_softc *sc;
	int ioport;

	/* Validate unit number */

	if (unit >= NPMS)
		return(ENXIO);

	/* Get device data */

	sc = &pms_softc[unit];
	ioport = pmsaddr[unit];

	/* If device does not exist */

	if (ioport == 0)
		return(ENXIO);

	/* Disallow multiple opens */

	if (sc->state & OPEN)
		return(EBUSY);

	/* Initialize state */

	sc->state |= OPEN;
#ifdef 386BSD_KERNEL
	sc->rsel = 0;
#else
	sc->rsel.si_pid = 0;
	sc->rsel.si_coll = 0;
#endif
	sc->status = 0;
	sc->button = 0;
	sc->x = 0;
	sc->y = 0;

	/* Allocate and initialize a ring buffer */

	sc->inq.count = sc->inq.first = sc->inq.last = 0;

	/* Enable Bus Mouse interrupts */

	pms_write(ioport, PMS_DEV_ENABLE);
	pms_command(ioport, PMS_INT_ENABLE);
	pms_command(ioport, PMS_ENABLE);

	/* Successful open */

	return(0);
}

int pmsclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	int unit, ioport;
	struct pms_softc *sc;

	/* Get unit and associated info */

	unit = PMSUNIT(dev);
	sc = &pms_softc[unit];
	ioport = pmsaddr[unit];

	/* Disable further mouse interrupts */

	pms_command(ioport, PMS_DISABLE);
	pms_command(ioport, PMS_INT_DISABLE);
	pms_write(ioport, PMS_DEV_DISABLE);

	/* Complete the close */

	sc->state &= ~OPEN;

	/* close is almost always successful */

	return(0);
}

int pmsread(dev_t dev, struct uio *uio, int flag)
{
	int s;
	int error = 0;	/* keep compiler quiet, even though initialisation
			   is unnecessary */
	unsigned length;
	struct pms_softc *sc;
	unsigned char buffer[100];

	/* Get device information */

	sc = &pms_softc[PMSUNIT(dev)];

	/* Block until mouse activity occured */

	s = spltty();
	while (sc->inq.count == 0) {
		if (minor(dev) & 0x1) {
			splx(s);
			return(EWOULDBLOCK);
		}
		sc->state |= ASLP;
		error = tsleep(sc, PZERO | PCATCH, "pmsrea", 0);
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

int pmsioctl(dev_t dev, caddr_t addr, int cmd, int flag, struct proc *p)
{
	struct pms_softc *sc;
	struct mouseinfo info;
	int s, error;

	/* Get device information */

	sc = &pms_softc[PMSUNIT(dev)];

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

void pmsintr(unit)
	int unit;
{
	struct pms_softc *sc = &pms_softc[unit];
	int ioport = pmsaddr[unit];
	static int state = 0;
	static char buttons, dx, dy;
	char changed;

	switch (state) {

	case 0:
		buttons = inb(ioport + DATA);
		if (!(buttons & 0xc0))
			++state;
		buttons = ~(((buttons&1) << 2) | (buttons&2));
		break;

	case 1:
		dx = inb(ioport + DATA) << 2;
		dx >>= 2;
		++state;
		break;

	case 2:
		dy = inb(ioport + DATA) << 2;
		dy >>= 2;
		state = 0;

		dy = -dy;

		changed = buttons ^ sc->button;
		sc->button = buttons;
		sc->status = buttons | (sc->status & ~BUTSTATMASK) | (changed << 3);

		/* Update accumulated movements */

		sc->x += dx;
		sc->y += dy;

		/* If device in use and a change occurred... */

		if (sc->state & OPEN && (dx || dy || changed)) {
			sc->inq.queue[sc->inq.last++] = 0x40 |
							(buttons ^ BUTSTATMASK);
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

		break;
	}
}

int pmsselect(dev_t dev, int rw, struct proc *p)
{
	int s, ret;
	struct pms_softc *sc = &pms_softc[PMSUNIT(dev)];

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
