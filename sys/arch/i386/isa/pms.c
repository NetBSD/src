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
 *	$Id: pms.c,v 1.8 1993/11/02 23:59:34 mycroft Exp $
 */

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
#ifdef NetBSD
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
#define	PMS_OBUF_FULL	0x21
#define	PMS_IBUF_FULL	0x02

/* controller commands */
#define	PMS_INT_ENABLE	0x47	/* enable controller interrupts */
#define	PMS_INT_DISABLE	0x65	/* disable controller interrupts */
#define	PMS_AUX_DISABLE	0xa7	/* enable auxiliary port */
#define	PMS_AUX_ENABLE	0xa8	/* disable auxiliary port */
#define	PMS_MAGIC_1	0xa9	/* XXX */
#define	PMS_MAGIC_2	0xaa	/* XXX */

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
	struct selinfo rsel;
	unsigned char state;	/* Mouse driver state */
	unsigned char status;	/* Mouse button status */
	unsigned char button;	/* Previous mouse button status bits */
	int x, y;		/* accumulated motion in the X,Y axis */
} pms_softc[NPMS];

#define OPEN	1		/* Device is open */
#define ASLP	2		/* Waiting for mouse data */

struct isa_driver pmsdriver = { pmsprobe, pmsattach, "pms" };

static inline void pms_flush(int ioport)
{
	u_char c;
	while (c = inb(ioport+STATUS) & 0x03)
		if ((c & PMS_OBUF_FULL) == PMS_OBUF_FULL)
			(void) inb(ioport+DATA);
}

static inline void pms_dev_cmd(int ioport, u_char value)
{
	pms_flush(ioport);
	outb(ioport+CNTRL, 0xd4);
	pms_flush(ioport);
	outb(ioport+DATA, value);
}

static inline void pms_aux_cmd(int ioport, u_char value)
{
	pms_flush(ioport);
	outb(ioport+CNTRL, value);
}

static inline void pms_pit_cmd(int ioport, u_char value)
{
	pms_flush(ioport);
	outb(ioport+CNTRL, 0x60);
	pms_flush(ioport);
	outb(ioport+DATA, value);
}

int pmsprobe(struct isa_device *dvp)
{
	int unit = dvp->id_unit;
	int ioport = dvp->id_iobase;
	u_char c;

	pms_dev_cmd(ioport, PMS_RESET);
	pms_aux_cmd(ioport, PMS_MAGIC_1);
	pms_aux_cmd(ioport, PMS_MAGIC_2);
	c = inb(ioport+DATA);
	pms_pit_cmd(ioport, PMS_INT_DISABLE);
	if (c & 0x04)
		return 0;
	return 1;
}

int pmsattach(struct isa_device *dvp)
{
	int unit = dvp->id_unit;
	int ioport = dvp->id_iobase;
	struct pms_softc *sc = &pms_softc[unit];

	/* Save I/O base address */
	pmsaddr[unit] = ioport;

	/* Disable mouse interrupts and initialize */
	pms_pit_cmd(ioport, PMS_INT_DISABLE);
	pms_aux_cmd(ioport, PMS_AUX_ENABLE);
	pms_dev_cmd(ioport, PMS_DEV_ENABLE);
	pms_dev_cmd(ioport, PMS_SET_RES);
	pms_dev_cmd(ioport, 3);		/* 8 counts/mm */
	pms_dev_cmd(ioport, PMS_SET_SCALE21);
	pms_dev_cmd(ioport, PMS_SET_SAMPLE);
	pms_dev_cmd(ioport, 100);	/* 100 samples/sec */
	pms_dev_cmd(ioport, PMS_SET_STREAM);
	pms_dev_cmd(ioport, PMS_DEV_DISABLE);
	pms_aux_cmd(ioport, PMS_AUX_DISABLE);

	/* Setup initial driver state */
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
	sc->status = 0;
	sc->button = 0;
	sc->x = 0;
	sc->y = 0;

	/* Allocate and initialize a ring buffer */
	sc->inq.count = sc->inq.first = sc->inq.last = 0;

	/* Enable Bus Mouse interrupts */
	pms_aux_cmd(ioport, PMS_AUX_ENABLE);
	pms_pit_cmd(ioport, PMS_INT_ENABLE);
	pms_dev_cmd(ioport, PMS_DEV_ENABLE);

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
	pms_dev_cmd(ioport, PMS_DEV_DISABLE);
	pms_pit_cmd(ioport, PMS_INT_DISABLE);
	pms_aux_cmd(ioport, PMS_AUX_DISABLE);

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
		error = tsleep((caddr_t)sc, PZERO | PCATCH, "pmsrea", 0);
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
		break;

	case 1:
		dx = inb(ioport + DATA);
		++state;
		break;

	case 2:
		dy = inb(ioport + DATA);
		state = 0;

		if (buttons & 0x10)
			dx -= 256;
		if (buttons & 0x20)
			dy -= 256;

		if (dx == -128)
			dx = -127;
		if (dy == -128)
			dy = 127;
		else
			dy = -dy;

		buttons = ~(((buttons&1) << 2) | (buttons&3));

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
				wakeup((caddr_t)sc);
			}
			selwakeup(&sc->rsel);
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
		selrecord(p, &sc->rsel);
		ret = 0;
	}
	splx(s);

	return(ret);
}
#endif
