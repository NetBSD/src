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
 *	$Id: lms.c,v 1.6.2.3 1993/09/29 08:28:18 mycroft Exp $
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

#include "i386/isa/isavar.h"
#include "i386/include/mouse.h"
#include "i386/include/pio.h"

#define LMS_DATA	0       /* Offset for data port, read-only */
#define LMS_SIGN	1       /* Offset for signature port, read-write */
#define LMS_INTR	2       /* Offset for interrupt port, read-only */
#define LMS_CNTRL	2       /* Offset for control port, write-only */
#define LMS_CONFIG	3	/* for configuration port, read-write */
#define	LMS_NPORTS	4

#define	LMS_CHUNK	128	/* chunk size for read */
#define	LMS_BSIZE	1024	/* buffer size */

struct lms_softc {		/* Driver status information */
	struct ringbuf {	/* Input queue */
		int rb_count, rb_first, rb_last;
		char rb_data[LMS_BSIZE];
	} sc_q;
	struct	selinfo sc_rsel;
	u_short	sc_iobase;	/* I/O port base */
	u_char	sc_flags;	/* Driver flags */
#define	LMS_NOBLOCK	0x01
	u_char	sc_state;	/* Mouse driver state */
#define LMS_OPEN	0x01	/* Device is open */
#define LMS_ASLP	0x02	/* Waiting for mouse data */
	u_char	sc_status;	/* Mouse button status */
	u_char	sc_button;	/* Previous mouse button status bits */
	int	sc_x, sc_y;	/* accumulated motion in the X,Y axis */
};

static int lmsprobe __P((struct device *, struct cfdata *, void *));
static void lmsforceintr __P((void *));
static void lmsattach __P((struct device *, struct device *, void *));
static int lmsintr __P((void *));

struct cfdriver lmscd =
{ NULL, "lms", lmsprobe, lmsattach, sizeof (struct lms_softc) };

#define LMSUNIT(dev)	(minor(dev) >> 1)
#define	LMSFLAGS(dev)	(minor(dev) & 0x01)

static int
lmsprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

	/* Configure and check for port present */
	outb(iobase + LMS_CONFIG, 0x91);
	delay(10);
	outb(iobase + LMS_SIGN, 0x0c);
	delay(10);
	if (inb(iobase + LMS_SIGN) != 0x0c)
		return 0;
	outb(iobase + LMS_SIGN, 0x50);
	delay(10);
	if (inb(iobase + LMS_SIGN) != 0x50)
		return 0;

	/* XXXX isa_discoverintr */

	/* disable interrupts */
	outb(iobase + LMS_CNTRL, 0x10);

	ia->ia_iosize = LMS_NPORTS;
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	return 1;
}

static void
lmsforceintr(aux)
	void *aux;
{
	struct	isa_attach_args *ia = aux;

	/* enable interrupts; expect to get one in 1/60 second */
	outb(ia->ia_iobase + LMS_CNTRL, 0);
}

static void
lmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct	lms_softc *sc = (struct lms_softc *)self;
	struct	isa_attach_args *ia = aux;
	u_short	iobase = ia->ia_iobase;

	/* other initialization done by lmsprobe */
	sc->sc_iobase = iobase;
	sc->sc_state = 0;

	/* XXXX isa_establishintr */
}

int
lmsopen(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = LMSUNIT(dev);
	struct	lms_softc *sc;

	if (unit >= lmscd.cd_ndevs)
		return ENXIO;
	sc = (struct lms_softc *)lmscd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_state & LMS_OPEN)
		return EBUSY;

	sc->sc_state |= LMS_OPEN;
	sc->sc_status = 0;
	sc->sc_button = 0;
	sc->sc_x = sc->sc_y = 0;
	sc->sc_q.rb_count = sc->sc_q.rb_first = sc->sc_q.rb_last = 0;

	/* enable interrupts */
	outb(sc->sc_iobase + LMS_CNTRL, 0);

	return 0;
}

int
lmsclose(dev, flag)
	dev_t dev;
	int flag;
{
	int	unit = LMSUNIT(dev);
	struct	lms_softc *sc = (struct lms_softc *)lmscd.cd_devs[unit];

	/* disable interrupts */
	outb(sc->sc_iobase + LMS_CNTRL, 0x10);

	sc->sc_state &= ~LMS_OPEN;

	return 0;
}

int
lmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	int	unit = LMSUNIT(dev);
	struct	lms_softc *sc = (struct lms_softc *)lmscd.cd_devs[unit];
	int	s;
	int	error;
	size_t	length;
	u_char	buffer[LMS_CHUNK];

	/* Block until mouse activity occured */

	s = spltty();
	while (sc->sc_q.rb_count == 0) {
		if (sc->sc_flags & LMS_NOBLOCK) {
			splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= LMS_ASLP;
		if (error = tsleep((caddr_t)sc, PZERO | PCATCH, "lmsrea", 0)) {
			sc->sc_state &= ~LMS_ASLP;
			splx(s);
			return error;
		}
	}

	/* Transfer as many chunks as possible */

	while (sc->sc_q.rb_count > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_q.rb_count, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from input queue */

		if (sc->sc_q.rb_first + length >= LMS_BSIZE) {
			size_t	left = LMS_BSIZE - sc->sc_q.rb_first;
			bcopy(&sc->sc_q.rb_data[sc->sc_q.rb_first], buffer,
			      left);
			bcopy(sc->sc_q.rb_data, &buffer[left], length - left);
		} else
			bcopy(&sc->sc_q.rb_data[sc->sc_q.rb_first], buffer,
			      length);
	
		sc->sc_q.rb_first = (sc->sc_q.rb_first + length) % LMS_BSIZE;
		sc->sc_q.rb_count -= length;

		/* Copy data to user process */

		if (error = uiomove(buffer, length, uio))
			break;
	}

	/* reset counters */
	sc->sc_x = sc->sc_y = 0;

	splx(s);
	return error;
}

int
lmsioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	int	unit = LMSUNIT(dev);
	struct	lms_softc *sc = (struct lms_softc *)lmscd.cd_devs[unit];
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

		sc->sc_x = 0;
		sc->sc_y = 0;
		sc->sc_status &= ~BUTCHNGMASK;

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
lmsintr(arg)
	void *arg;
{
	struct	lms_softc *sc = (struct lms_softc *)arg;
	u_short	iobase = sc->sc_iobase;
	u_char	hi, lo, buttons, changed;
	char	dx, dy;

	if ((sc->sc_state & LMS_OPEN) == 0)
		/* interrupts not expected */
		return 0;

	outb(iobase + LMS_CNTRL, 0xab);
	hi = inb(iobase + LMS_DATA);
	outb(iobase + LMS_CNTRL, 0x90);
	lo = inb(iobase + LMS_DATA);
	dx = ((hi & 0x0f) << 4) | (lo & 0x0f);
	/* bounding at -127 avoids a bug in XFree86 */
	dx = (dx == -128) ? -127 : dx;

	outb(iobase + LMS_CNTRL, 0xf0);
	hi = inb(iobase + LMS_DATA);
	outb(iobase + LMS_CNTRL, 0xd0);
	lo = inb(iobase + LMS_DATA);
	dy = ((hi & 0x0f) << 4) | (lo & 0x0f);
	dy = (dy == -128) ? 127 : -dy;

	outb(iobase + LMS_CNTRL, 0);

	buttons = (~hi >> 5) & 7;
	changed = buttons ^ sc->sc_button;
	sc->sc_button = buttons;
	sc->sc_status = buttons | (sc->sc_status & ~BUTSTATMASK) |
			(changed << 3);

	if ((sc->sc_state & LMS_OPEN) && (dx || dy || changed)) {
		int	last = sc->sc_q.rb_last;
		char	*cp = &sc->sc_q.rb_data[last];
		int	count = sc->sc_q.rb_count;

		/* Update accumulated movements */
		sc->sc_x += dx;
		sc->sc_y += dy;

		if ((count += 5) > LMS_BSIZE)
			return 1;
		sc->sc_q.rb_count = count;

#define	next() \
		if (++last >= LMS_BSIZE) {	\
			last = 0;		\
			cp = sc->sc_q.rb_data;	\
		} else				\
			cp++;
		*cp = 0x80 | (buttons ^ BUTSTATMASK);
		next();
		*cp = dx;
		next();
		*cp = dy;
		next();
		*cp = 0;
		next();
		*cp = 0;
		next();
		sc->sc_q.rb_last = last;

		if (sc->sc_state & LMS_ASLP) {
			sc->sc_state &= ~LMS_ASLP;
			wakeup((caddr_t)sc);
		}
		selwakeup(&sc->sc_rsel);
	}

	return 1;
}

int
lmsselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	int	unit = LMSUNIT(dev);
	struct	lms_softc *sc = (struct lms_softc *)lmscd.cd_devs[unit];
	int	s;
	int	ret;

	if (rw == FWRITE)
		return 0;

	s = spltty();
	if (sc->sc_q.rb_count)
		ret = 1;
	else {
		selrecord(p, &sc->sc_rsel);
		ret = 0;
	}
	splx(s);

	return ret;
}
