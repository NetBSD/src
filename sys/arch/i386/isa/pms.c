/*	$NetBSD: pms.c,v 1.47 2000/06/05 22:20:56 sommerfeld Exp $	*/

/*-
 * Copyright (c) 1994, 1997 Charles M. Hannum.
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
 * XXXX
 * This is a hack.  This driver should really be combined with the
 * keyboard driver, since they go through the same buffer and use the
 * same I/O ports.  Frobbing the mouse and keyboard at the same time
 * may result in dropped characters and/or corrupted mouse events.
 */

#include "opms.h"
#if (NOPMS_HACK + NOPMS_PCKBC) > 1
#error Only one PS/2 style mouse may be configured into your system.
#endif

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

#if (NOPMS_HACK > 0)
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>
#endif
#if (NOPMS_PCKBC > 0)
#include <machine/bus.h>
#include <dev/ic/pckbcvar.h>
#endif
#include <machine/mouse.h>
#include <machine/conf.h>

#if (NOPMS_HACK > 0)

#include <dev/isa/isavar.h>

#define	PMS_DATA	0x60	/* offset for data port, read-write */
#define	PMS_CNTRL	0x64	/* offset for control port, write-only */
#define	PMS_STATUS	0x64	/* offset for status port, read-only */
#define	PMS_NPORTS	8

/* status bits */
#define	PMS_OBUF_FULL	0x01
#define	PMS_IBUF_FULL	0x02

/* controller commands */
#define	PMS_INT_ENABLE	0x47	/* enable controller interrupts */
#define	PMS_INT_DISABLE	0x45	/* disable controller interrupts */
#define	PMS_AUX_ENABLE	0xa8	/* enable auxiliary port */
#define	PMS_AUX_DISABLE	0xa7	/* disable auxiliary port */
#define	PMS_AUX_TEST	0xa9	/* test auxiliary port */

#endif /* NOPMS_HACK > 0 */

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
#define	PMS_BSIZE	1020	/* buffer size */

struct opms_softc {		/* driver status information */
	struct device sc_dev;
#if (NOPMS_HACK > 0)
	void *sc_ih;
#endif
#if (NOPMS_PCKBC > 0)
	pckbc_tag_t sc_kbctag;
	pckbc_slot_t sc_kbcslot;
#endif

	struct clist sc_q;
	struct selinfo sc_rsel;
	u_char sc_state;	/* mouse driver state */
#define	PMS_OPEN	0x01	/* device is open */
#define	PMS_ASLP	0x02	/* waiting for mouse data */
	u_char sc_status;	/* mouse button status */
	int sc_x, sc_y;		/* accumulated motion in the X,Y axis */
};

#if (NOPMS_HACK > 0)
int opms_hack_probe __P((struct device *, struct cfdata *, void *));
void opms_hack_attach __P((struct device *, struct device *, void *));
int opmsintr __P((void *));

struct cfattach opms_hack_ca = {
	sizeof(struct opms_softc), opms_hack_probe, opms_hack_attach,
};
#endif
#if (NOPMS_PCKBC > 0)
int opms_pckbc_probe __P((struct device *, struct cfdata *, void *));
void opms_pckbc_attach __P((struct device *, struct device *, void *));

struct cfattach opms_pckbc_ca = {
	sizeof(struct opms_softc), opms_pckbc_probe, opms_pckbc_attach,
};
#endif

void opmsinput __P((void *, int));

extern struct cfdriver opms_cd;

#define	PMSUNIT(dev)	(minor(dev))

#if (NOPMS_HACK > 0)

static __inline int pms_wait_output __P((void));
static __inline int pms_wait_input __P((void));
static __inline void pms_flush_input __P((void));
static __inline void pms_dev_cmd __P((u_char));
static __inline void pms_pit_cmd __P((u_char));
static __inline void pms_aux_cmd __P((u_char));

#define	PMS_DELAY \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; } \
	{ u_char x = inb(0x84); (void) x; }

static __inline int
pms_wait_output()
{
	u_int i;

	for (i = 100000; i; i--)
		if ((inb(PMS_STATUS) & PMS_IBUF_FULL) == 0) {
			PMS_DELAY;
			return (1);
		}
	return (0);
}

static __inline int
pms_wait_input()
{
	u_int i;

	for (i = 100000; i; i--)
		if ((inb(PMS_STATUS) & PMS_OBUF_FULL) != 0) {
			PMS_DELAY;
			return (1);
		}
	return (0);
}

static __inline void
pms_flush_input()
{
	u_int i;

	pms_wait_output();
	delay(10000);
	for (i = 10; i; i--) {
		if ((inb(PMS_STATUS) & PMS_OBUF_FULL) == 0)
			return;
		PMS_DELAY;
		(void) inb(PMS_DATA);
	}
}

static __inline void
pms_dev_cmd(value)
	u_char value;
{
	int s;

	s = spltty();
	pms_wait_output();
	outb(PMS_CNTRL, 0xd4);
	pms_wait_output();
	outb(PMS_DATA, value);
	splx(s);
}

static __inline void
pms_aux_cmd(value)
	u_char value;
{
	int s;

	s = spltty();
	pms_wait_output();
	outb(PMS_CNTRL, value);
	splx(s);
}

static __inline void
pms_pit_cmd(value)
	u_char value;
{
	int s;

	s = spltty();
	pms_wait_output();
	outb(PMS_CNTRL, 0x60);
	pms_wait_output();
	outb(PMS_DATA, value);
	splx(s);
}

/*
 * XXX needs more work yet.  We should have a `pckbd_attach_args' that
 * provides the parent's io port and our irq.
 */
int
opms_hack_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	u_char x;

	/*
	 * We only attach to the keyboard controller via
	 * the console drivers. (We really wish we could be the
	 * child of a real keyboard controller driver.)
	 */
	if ((parent == NULL) ||
	   ((strcmp(parent->dv_cfdata->cf_driver->cd_name, "pc") != 0) &&
	    (strcmp(parent->dv_cfdata->cf_driver->cd_name, "vt") != 0)))
		return (0);

	/* Can't wildcard IRQ. */
	if (cf->cf_loc[PCKBCPORTCF_IRQ] == PCKBCPORTCF_IRQ_DEFAULT)
		return (0);

	pms_flush_input();
#if 0
	pms_dev_cmd(PMS_RESET);
	if (!pms_wait_input())
		return 0;
	x = inb(PMS_DATA);
#endif
	pms_aux_cmd(PMS_AUX_TEST);
	if (!pms_wait_input())
		return 0;
	x = inb(PMS_DATA);
	pms_pit_cmd(PMS_INT_DISABLE);
	if (x & 0x04)
		return 0;

	return 1;
}

void
opms_hack_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct opms_softc *sc = (void *)self;
	int irq = self->dv_cfdata->cf_loc[PCKBCPORTCF_IRQ];
	isa_chipset_tag_t ic = aux;			/* XXX */

	printf(" irq %d\n", irq);

	/* Other initialization was done by pmsprobe. */
	sc->sc_state = 0;

	sc->sc_ih = isa_intr_establish(ic, irq, IST_EDGE, IPL_TTY,
	    opmsintr, sc);
}

#endif /* NOPMS_HACK > 0 */

#if (NOPMS_PCKBC > 0)
int
opms_pckbc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pckbc_attach_args *pa = aux;
	u_char cmd[1], resp[2];
	int res;

	if (pa->pa_slot != PCKBC_AUX_SLOT)
		return (0);

	/* Flush any garbage. */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 2, resp, 1);
	if (res) {
#ifdef DEBUG
		printf("opmsprobe: reset error\n");
#endif
		return (0);
	}
	if (resp[0] != 0xaa) {
		printf("opmsprobe: reset response 0x%x\n", resp[0]);
		return (0);
	}

	/* get type number (0 = mouse) */
	if (resp[1] != 0) {
#ifdef DEBUG
		printf("opmsprobe: type 0x%x\n", resp[1]);
#endif
		return (0);
	}

	return (1);
}

void
opms_pckbc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct opms_softc *sc = (void *)self;
	struct pckbc_attach_args *pa = aux;
	u_char cmd[1];
	int res;

	sc->sc_kbctag = pa->pa_tag;
	sc->sc_kbcslot = pa->pa_slot;

	printf("\n");

	/* Other initialization was done by pmsprobe. */
	sc->sc_state = 0;

	pckbc_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot,
			       opmsinput, sc, sc->sc_dev.dv_xname);

	/* no interrupts until enabled */
	cmd[0] = PMS_DEV_DISABLE;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 0, 0, 0);
	if (res)
		printf("opmsattach: disable error\n");
	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
}
#endif

int
pmsopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = PMSUNIT(dev);
	struct opms_softc *sc;
#if (NOPMS_PCKBC > 0)
	u_char cmd[1];
	int res;
#endif

	if (unit >= opms_cd.cd_ndevs)
		return ENXIO;
	sc = opms_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_state & PMS_OPEN)
		return EBUSY;

	if (clalloc(&sc->sc_q, PMS_BSIZE, 0) == -1)
		return ENOMEM;

	sc->sc_state |= PMS_OPEN;
	sc->sc_status = 0;
	sc->sc_x = sc->sc_y = 0;

	/* Enable interrupts. */
#if (NOPMS_HACK > 0)
	pms_aux_cmd(PMS_AUX_ENABLE);
	pms_pit_cmd(PMS_INT_ENABLE);
	pms_dev_cmd(PMS_DEV_ENABLE);
	if (pms_wait_input())
		pms_flush_input();
#endif
#if (NOPMS_PCKBC > 0)
	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

	cmd[0] = PMS_DEV_ENABLE;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				cmd, 1, 0, 1, 0);
	if (res) {
		printf("opms_enable: command error\n");
		return (res);
	}
#endif

#if 0
	pms_dev_cmd(PMS_SET_RES);
	pms_dev_cmd(3);		/* 8 counts/mm */
	pms_dev_cmd(PMS_SET_SCALE21);
	pms_dev_cmd(PMS_SET_SAMPLE);
	pms_dev_cmd(100);	/* 100 samples/sec */
	pms_dev_cmd(PMS_SET_STREAM);
#endif

	return 0;
}

int
pmsclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
#if (NOPMS_PCKBC > 0)
	u_char cmd[1];
	int res;
#endif

	/* Disable interrupts. */
#if (NOPMS_HACK > 0)
	pms_dev_cmd(PMS_DEV_DISABLE);
	if (pms_wait_input())
		pms_flush_input();
	pms_pit_cmd(PMS_INT_DISABLE);
	pms_aux_cmd(PMS_AUX_DISABLE);
#endif
#if (NOPMS_PCKBC > 0)
	cmd[0] = PMS_DEV_DISABLE;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				cmd, 1, 0, 1, 0);
	if (res)
		printf("opms_disable: command error\n");

	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
#endif

	sc->sc_state &= ~PMS_OPEN;

	clfree(&sc->sc_q);

	return 0;
}

int
pmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	int s;
	int error = 0;
	size_t length;
	u_char buffer[PMS_CHUNK];

	/* Block until mouse activity occured. */

	s = spltty();
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= PMS_ASLP;
		error = tsleep((caddr_t)sc, PZERO | PCATCH, "pmsrea", 0);
		if (error) {
			sc->sc_state &= ~PMS_ASLP;
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
pmsioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
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

/* Masks for the first byte of a packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04

void
opmsinput(arg, data)
	void *arg;
	int data;
{
	struct opms_softc *sc = arg;
	static int state = 0;
	static u_char buttons;
	u_char changed;
	static char dx, dy;
	u_char buffer[5];

	if ((sc->sc_state & PMS_OPEN) == 0) {
		/* Interrupts are not expected.  Discard the byte. */
		return;
	}

	switch (state) {

	case 0:
		buttons = data;
		if ((buttons & 0xc0) == 0)
			++state;
		break;

	case 1:
		dx = data;
		/* Bounding at -127 avoids a bug in XFree86. */
		dx = (dx == -128) ? -127 : dx;
		++state;
		break;

	case 2:
		dy = data;
		dy = (dy == -128) ? -127 : dy;
		state = 0;

		buttons = ((buttons & PS2LBUTMASK) << 2) |
			  ((buttons & (PS2RBUTMASK | PS2MBUTMASK)) >> 1);
		changed = ((buttons ^ sc->sc_status) & BUTSTATMASK) << 3;
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

			if (sc->sc_state & PMS_ASLP) {
				sc->sc_state &= ~PMS_ASLP;
				wakeup((caddr_t)sc);
			}
			selwakeup(&sc->sc_rsel);
		}

		break;
	}
}

#if (NOPMS_HACK > 0)
int
opmsintr(arg)
	void *arg;
{
	struct opms_softc *sc = arg;
	int data;

	if ((sc->sc_state & PMS_OPEN) == 0) {
		/* Interrupts are not expected.  Discard the byte. */
		pms_flush_input();
		return (0);
	}

	data = inb(PMS_DATA);
	opmsinput(arg, data);
	return (-1);
}
#endif

int
pmspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
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
