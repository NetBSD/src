/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	$Id: mms.c,v 1.6.2.9 1993/10/16 06:39:39 mycroft Exp $
 */

#include "param.h"
#include "kernel.h"
#include "systm.h"
#include "buf.h"
#include "malloc.h"
#include "ioctl.h"
#include "tty.h"
#include "file.h"
#include "select.h"
#include "proc.h"
#include "vnode.h"
#include "sys/device.h"

#include "machine/cpu.h"
#include "machine/mouse.h"
#include "machine/pio.h"

#include "i386/isa/isavar.h"
#include "i386/isa/icu.h"

#define MMS_ADDR	0	/* Offset for register select */
#define MMS_DATA	1	/* Offset for InPort data */
#define MMS_IDENT	2	/* Offset for identification register */
#define	MMS_NPORTS	4

#define	MMS_CHUNK	128	/* chunk size for read */
#define	MMS_BSIZE	1020	/* buffer size */

struct mms_softc {		/* Driver status information */
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	struct	clist sc_q;
	struct	selinfo sc_rsel;
	u_short	sc_iobase;	/* I/O port base */
	u_char	sc_state;	/* Mouse driver state */
#define MMS_OPEN	0x01	/* Device is open */
#define MMS_ASLP	0x02	/* Waiting for mouse data */
	u_char	sc_status;	/* Mouse button status */
	int	sc_x, sc_y;	/* accumulated motion in the X,Y axis */
};

static int mmsprobe __P((struct device *, struct cfdata *, void *));
static void mmsforceintr __P((void *));
static void mmsattach __P((struct device *, struct device *, void *));
static int mmsintr __P((void *));

struct cfdriver mmscd =
{ NULL, "mms", mmsprobe, mmsattach, DV_TTY, sizeof (struct mms_softc) };

#define MMSUNIT(dev)	(minor(dev))

static int
mmsprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

	if (iobase == IOBASEUNK)
		return 0;

	/* Read identification register to see if present */
	if (inb(iobase + MMS_IDENT) != 0xde)
		return 0;

	/* Seems it was there; reset */
	outb(iobase + MMS_ADDR, 0x87);

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(mmsforceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;
	}

	/* reset again to disable interrupts */
	outb(iobase + MMS_ADDR, 0x87);

	ia->ia_iosize = MMS_NPORTS;
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	return 1;
}

static void
mmsforceintr(aux)
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

	/* enable interrupts; expect to get one in 1/60 second */
	outb(iobase + MMS_ADDR, 0x07);
	outb(iobase + MMS_DATA, 0x09);
}

static void
mmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct	mms_softc *sc = (struct mms_softc *)self;
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

	/* other initialization done by mmsprobe */
	sc->sc_iobase = iobase;
	sc->sc_state = 0;

	printf(": Microsoft mouse\n");
	isa_establish(&sc->sc_id, &sc->sc_dev);

	sc->sc_ih.ih_fun = mmsintr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
}

int
mmsopen(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = MMSUNIT(dev);
	struct	mms_softc *sc;
	u_short	iobase;

	if (unit >= mmscd.cd_ndevs)
		return ENXIO;
	sc = mmscd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_state & MMS_OPEN)
		return EBUSY;

	if (clalloc(&sc->sc_q, MMS_BSIZE, 0) == -1)
		return ENOMEM;

	sc->sc_state |= MMS_OPEN;
	sc->sc_status = 0;
	sc->sc_x = sc->sc_y = 0;

	/* enable interrupts */
	iobase = sc->sc_iobase;
	outb(iobase + MMS_ADDR, 0x07);
	outb(iobase + MMS_DATA, 0x09);

	return 0;
}

int
mmsclose(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = MMSUNIT(dev);
	struct	mms_softc *sc = mmscd.cd_devs[unit];

	/* disable interrupts */
	outb(sc->sc_iobase + MMS_ADDR, 0x87);

	sc->sc_state &= ~MMS_OPEN;

	clfree(&sc->sc_q);

	return 0;
}

int
mmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int	unit = MMSUNIT(dev);
	struct	mms_softc *sc = mmscd.cd_devs[unit];
	int	s;
	int	error;
	size_t	length;
	u_char	buffer[MMS_CHUNK];

	/* Block until mouse activity occured */

	s = spltty();
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= MMS_ASLP;
		if (error = tsleep((caddr_t)sc, PZERO | PCATCH, "mmsrea", 0)) {
			sc->sc_state &= ~MMS_ASLP;
			splx(s);
			return error;
		}
	}

	/* Transfer as many chunks as possible */

	while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_q.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* remove a small chunk from input queue */
		(void) q_to_b(&sc->sc_q, buffer, length);

		/* copy data to user process */
		if (error = uiomove(buffer, length, uio))
			break;
	}

	/* reset counters */
	sc->sc_x = sc->sc_y = 0;

	splx(s);
	return error;
}

int
mmsioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	int	unit = MMSUNIT(dev);
	struct	mms_softc *sc = mmscd.cd_devs[unit];
	struct	mouseinfo info;
	int	s;
	int	error;

	switch (cmd) {
	    case MOUSEIOCREAD:
		s = spltty();

		info.status = sc->sc_status;
		if (sc->sc_x || sc->sc_y)
			info.status |= MOVEMENT;

		if (sc->sc_x > 127)
			info.xmotion = 127;
		else if (sc->sc_x < -127)
			/* bounding at -127 avoids a bug in XFree86 */
			info.xmotion = -127;
		else
			info.xmotion = sc->sc_x;

		if (sc->sc_y > 127)
			info.ymotion = 127;
		else if (sc->sc_y < -127)
			info.ymotion = -127;
		else
			info.ymotion = sc->sc_y;

		/* reset historical information */
		sc->sc_x = 0;
		sc->sc_y = 0;
		sc->sc_status &= ~BUTCHNGMASK;
		flushq(&sc->sc_q);

		splx(s);
		error = copyout(&info, addr, sizeof(struct mouseinfo));
		break;

	    default:
		error = EINVAL;
		break;
	}

	return error;
}

int
mmsintr(arg)
	void *arg;
{
	struct	mms_softc *sc = (struct mms_softc *)arg;
	u_short	iobase = sc->sc_iobase;
	u_char	buttons, changed, status;
	char	dx, dy;
	u_char	buffer[5];

	if ((sc->sc_state & MMS_OPEN) == 0)
		/* interrupts not expected */
		return 0;

	/* Freeze InPort registers (disabling interrupts) */
	outb(iobase + MMS_ADDR, 0x07);
	outb(iobase + MMS_DATA, 0x29);

	outb(iobase + MMS_ADDR, 0);
	status = inb(iobase + MMS_DATA);

	if (status & 0x40) {
		outb(iobase + MMS_ADDR, 1);
		dx = inb(iobase + MMS_DATA);
		dx = (dx == -128) ? -127 : dx;
		outb(iobase + MMS_ADDR, 2);
		dy = inb(iobase + MMS_DATA);
		dy = (dy == -128) ? 127 : -dy;
	} else
		dx = dy = 0;

	/* Unfreeze InPort Registers (re-enables interrupts) */
	outb(iobase + MMS_ADDR, 0x07);
	outb(iobase + MMS_DATA, 0x09);

	buttons = status & BUTSTATMASK;
	changed = status & BUTCHNGMASK;
	sc->sc_status = buttons | (sc->sc_status & ~BUTSTATMASK) | changed;

	if (dx || dy || changed) {
		/* update accumulated movements */
		sc->sc_x += dx;
		sc->sc_y += dy;

		buffer[0] = 0x80 | (~status & BUTSTATMASK);
		buffer[1] = dx;
		buffer[2] = dy;
		buffer[3] = buffer[4] = 0;
		(void) b_to_q(buffer, sizeof buffer, &sc->sc_q);

		if (sc->sc_state & MMS_ASLP) {
			sc->sc_state &= ~MMS_ASLP;
			wakeup((caddr_t)sc);
		}
		selwakeup(&sc->sc_rsel);
	}

	return 1;
}

int
mmsselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	int	unit = MMSUNIT(dev);
	struct	mms_softc *sc = mmscd.cd_devs[unit];
	int	s;
	int	ret;

	if (rw == FWRITE)
		return 0;

	s = spltty();
	if (sc->sc_q.c_cc)
		ret = 1;
	else {
		selrecord(p, &sc->sc_rsel);
		ret = 0;
	}
	splx(s);

	return ret;
}
