/*	$NetBSD: olms.c,v 1.1.24.1 2001/09/08 21:38:31 thorpej Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles M. Hannum.
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
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/mouse.h>
#include <machine/conf.h>

#include <dev/isa/isavar.h>

#define	LMS_DATA	0       /* offset for data port, read-only */
#define	LMS_SIGN	1       /* offset for signature port, read-write */
#define	LMS_INTR	2       /* offset for interrupt port, read-only */
#define	LMS_CNTRL	2       /* offset for control port, write-only */
#define	LMS_CONFIG	3	/* for configuration port, read-write */
#define	LMS_NPORTS	4

#define	LMS_CHUNK	128	/* chunk size for read */
#define	LMS_BSIZE	1020	/* buffer size */

struct olms_softc {		/* driver status information */
	struct device sc_dev;
	void *sc_ih;

	bus_space_tag_t sc_iot;		/* bus i/o space identifier */
	bus_space_handle_t sc_ioh;	/* bus i/o handle */

	struct clist sc_q;
	struct selinfo sc_rsel;
	u_char sc_state;	/* mouse driver state */
#define	LMS_OPEN	0x01	/* device is open */
#define	LMS_ASLP	0x02	/* waiting for mouse data */
	u_char sc_status;	/* mouse button status */
	int sc_x, sc_y;		/* accumulated motion in the X,Y axis */
};

int olmsprobe __P((struct device *, struct cfdata *, void *));
void olmsattach __P((struct device *, struct device *, void *));
int olmsintr __P((void *));

struct cfattach olms_ca = {
	sizeof(struct olms_softc), olmsprobe, olmsattach
};

extern struct cfdriver olms_cd;

#define	LMSUNIT(dev)	(minor(dev))

int
olmsprobe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int rv;
	
	/* Disallow wildcarded i/o base. */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT)
		return 0;

	/* Map the i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, LMS_NPORTS, 0, &ioh))
		return 0;

	rv = 0;

	/* Configure and check for port present. */
	bus_space_write_1(iot, ioh, LMS_CONFIG, 0x91);
	delay(10);
	bus_space_write_1(iot, ioh, LMS_SIGN, 0x0c);
	delay(10);
	if (bus_space_read_1(iot, ioh, LMS_SIGN) != 0x0c)
		goto out;
	bus_space_write_1(iot, ioh, LMS_SIGN, 0x50);
	delay(10);
	if (bus_space_read_1(iot, ioh, LMS_SIGN) != 0x50)
		goto out;

	/* Disable interrupts. */
	bus_space_write_1(iot, ioh, LMS_CNTRL, 0x10);

	rv = 1;
	ia->ia_iosize = LMS_NPORTS;
	ia->ia_msize = 0;

out:
	bus_space_unmap(iot, ioh, LMS_NPORTS);
	return rv;
}

void
olmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct olms_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;

	printf("\n");

	if (bus_space_map(iot, ia->ia_iobase, LMS_NPORTS, 0, &ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Other initialization was done by olmsprobe. */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_state = 0;

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_PULSE,
	    IPL_TTY, olmsintr, sc);
}

int
lmsopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LMSUNIT(dev);
	struct olms_softc *sc;

	if (unit >= olms_cd.cd_ndevs)
		return ENXIO;
	sc = olms_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_state & LMS_OPEN)
		return EBUSY;

	if (clalloc(&sc->sc_q, LMS_BSIZE, 0) == -1)
		return ENOMEM;

	sc->sc_state |= LMS_OPEN;
	sc->sc_status = 0;
	sc->sc_x = sc->sc_y = 0;

	/* Enable interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LMS_CNTRL, 0);

	return 0;
}

int
lmsclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct olms_softc *sc = olms_cd.cd_devs[LMSUNIT(dev)];

	/* Disable interrupts. */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, LMS_CNTRL, 0x10);

	sc->sc_state &= ~LMS_OPEN;

	clfree(&sc->sc_q);

	return 0;
}

int
lmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct olms_softc *sc = olms_cd.cd_devs[LMSUNIT(dev)];
	int s;
	int error = 0;
	size_t length;
	u_char buffer[LMS_CHUNK];

	/* Block until mouse activity occured. */

	s = spltty();
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= LMS_ASLP;
		error = tsleep((caddr_t)sc, PZERO | PCATCH, "lmsrea", 0);
		if (error) {
			sc->sc_state &= ~LMS_ASLP;
			splx(s);
			return error;
		}
	}
	splx(s);

	/* Transfer as many chunks as possible. */

	while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_q.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from the input queue. */
		(void) q_to_b(&sc->sc_q, buffer, length);

		/* Copy the data to the user process. */
		if ((error = uiomove(buffer, length, uio)) != 0)
			break;
	}

	return error;
}

int
lmsioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct olms_softc *sc = olms_cd.cd_devs[LMSUNIT(dev)];
	struct mouseinfo info;
	int s;
	int error;

	switch (cmd) {
	case MOUSEIOCREAD:
		s = spltty();

		info.status = sc->sc_status;
		if (sc->sc_x || sc->sc_y)
			info.status |= MOVEMENT;

		if (sc->sc_x > 127)
			info.xmotion = 127;
		else if (sc->sc_x < -127)
			/* Bounding at -127 avoids a bug in XFree86. */
			info.xmotion = -127;
		else
			info.xmotion = sc->sc_x;

		if (sc->sc_y > 127)
			info.ymotion = 127;
		else if (sc->sc_y < -127)
			info.ymotion = -127;
		else
			info.ymotion = sc->sc_y;

		/* Reset historical information. */
		sc->sc_x = sc->sc_y = 0;
		sc->sc_status &= ~BUTCHNGMASK;
		ndflush(&sc->sc_q, sc->sc_q.c_cc);

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
olmsintr(arg)
	void *arg;
{
	struct olms_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char hi, lo, buttons, changed;
	char dx, dy;
	u_char buffer[5];

	if ((sc->sc_state & LMS_OPEN) == 0)
		/* Interrupts are not expected. */
		return 0;

	bus_space_write_1(iot, ioh, LMS_CNTRL, 0xab);
	hi = bus_space_read_1(iot, ioh, LMS_DATA);
	bus_space_write_1(iot, ioh, LMS_CNTRL, 0x90);
	lo = bus_space_read_1(iot, ioh, LMS_DATA);
	dx = ((hi & 0x0f) << 4) | (lo & 0x0f);
	/* Bounding at -127 avoids a bug in XFree86. */
	dx = (dx == -128) ? -127 : dx;

	bus_space_write_1(iot, ioh, LMS_CNTRL, 0xf0);
	hi = bus_space_read_1(iot, ioh, LMS_DATA);
	bus_space_write_1(iot, ioh, LMS_CNTRL, 0xd0);
	lo = bus_space_read_1(iot, ioh, LMS_DATA);
	dy = ((hi & 0x0f) << 4) | (lo & 0x0f);
	dy = (dy == -128) ? 127 : -dy;

	bus_space_write_1(iot, ioh, LMS_CNTRL, 0);

	buttons = (~hi >> 5) & 0x07;
	changed = ((buttons ^ sc->sc_status) & 0x07) << 3;
	sc->sc_status = buttons | (sc->sc_status & ~BUTSTATMASK) | changed;

	if (dx || dy || changed) {
		/* Update accumulated movements. */
		sc->sc_x += dx;
		sc->sc_y += dy;

		/* Add this event to the queue. */
		buffer[0] = 0x80 | (buttons ^ BUTSTATMASK);
		buffer[1] = dx;
		buffer[2] = dy;
		buffer[3] = buffer[4] = 0;
		(void) b_to_q(buffer, sizeof buffer, &sc->sc_q);

		if (sc->sc_state & LMS_ASLP) {
			sc->sc_state &= ~LMS_ASLP;
			wakeup((caddr_t)sc);
		}
		selnotify(&sc->sc_rsel, 0);
	}

	return -1;
}

int
lmspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct olms_softc *sc = olms_cd.cd_devs[LMSUNIT(dev)];
	int revents = 0;
	int s = spltty();

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_q.c_cc > 0)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}

	splx(s);
	return (revents);
}

static void
filt_lmsrdetach(struct knote *kn)
{
	struct olms_softc *sc = (void *) kn->kn_hook;
	int s;

	s = spltty();
	SLIST_REMOVE(&sc->sc_rsel.si_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_lmsread(struct knote *kn, long hint)
{
	struct olms_softc *sc = (void *) kn->kn_hook;

	kn->kn_data = sc->sc_q.c_cc;
	return (kn->kn_data > 0);
}

static const struct filterops lmsread_filtops =
	{ 1, NULL, filt_lmsrdetach, filt_lmsread };

int
lmskqfilter(dev_t dev, struct knote *kn)
{
	struct olms_softc *sc = olms_cd.cd_devs[LMSUNIT(dev)];
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.si_klist;
		kn->kn_fop = &lmsread_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = (void *) sc;

	s = spltty();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}
