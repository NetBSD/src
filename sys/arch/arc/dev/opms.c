/*	$NetBSD: opms.c,v 1.3 2001/09/28 11:59:51 chs Exp $	*/
/*	$OpenBSD: pccons.c,v 1.22 1999/01/30 22:39:37 imp Exp $	*/
/*	NetBSD: pms.c,v 1.21 1995/04/18 02:25:18 mycroft Exp	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/poll.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/kbdreg.h>
#include <machine/mouse.h>

#include <arc/dev/pcconsvar.h>
#include <arc/dev/opmsvar.h>

#define	PMSUNIT(dev)	(minor(dev))

/* status bits */
#define	PMS_OBUF_FULL	0x01
#define	PMS_IBUF_FULL	0x02

/* controller commands */
#define	PMS_INT_ENABLE	0x47	/* enable controller interrupts */
#define	PMS_INT_DISABLE	0x65	/* disable controller interrupts */
#define	PMS_AUX_ENABLE	0xa7	/* enable auxiliary port */
#define	PMS_AUX_DISABLE	0xa8	/* disable auxiliary port */
#define	PMS_MAGIC_1	0xa9	/* XXX */

#define	PMS_8042_CMD	0x65

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

#define	FLUSHQ(q) { if((q)->c_cc) ndflush(q, (q)->c_cc); }

extern struct cfdriver opms_cd;

int opmsopen __P((dev_t, int));
int opmsclose __P((dev_t, int));
int opmsread __P((dev_t, struct uio *, int));
int opmsioctl __P((dev_t, u_long, caddr_t, int));
int opmsselect __P((dev_t, int, struct proc *));
int opmspoll __P((dev_t, int, struct proc *));
static __inline void pms_dev_cmd __P((u_char));
static __inline void pms_aux_cmd __P((u_char));
static __inline void pms_pit_cmd __P((u_char));

static __inline void
pms_dev_cmd(value)
	u_char value;
{
	kbd_flush_input();
	kbd_cmd_write_1(0xd4);
	kbd_flush_input();
	kbd_data_write_1(value);
}

static __inline void
pms_aux_cmd(value)
	u_char value;
{
	kbd_flush_input();
	kbd_cmd_write_1(value);
}

static __inline void
pms_pit_cmd(value)
	u_char value;
{
	kbd_flush_input();
	kbd_cmd_write_1(0x60);
	kbd_flush_input();
	kbd_data_write_1(value);
}

int opms_common_match(kbd_iot, config)
	bus_space_tag_t kbd_iot;
	struct pccons_config *config;
{
	u_char x;

	kbd_context_init(kbd_iot, config);

	pms_dev_cmd(KBC_RESET);
	pms_aux_cmd(PMS_MAGIC_1);
	delay(10000);
	x = kbd_data_read_1();
	pms_pit_cmd(PMS_INT_DISABLE);
	if (x & 0x04)
		return 0;

	return 1;
}

void
opms_common_attach(sc, opms_iot, config)
	struct opms_softc *sc;
	bus_space_tag_t opms_iot;
	struct pccons_config *config;
{
	kbd_context_init(opms_iot, config);

	/* Other initialization was done by opmsprobe. */
	sc->sc_state = 0;
}

int
opmsopen(dev, flag)
	dev_t dev;
	int flag;
{
	int unit = PMSUNIT(dev);
	struct opms_softc *sc;

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
	pms_dev_cmd(PMS_DEV_ENABLE);
	pms_aux_cmd(PMS_AUX_ENABLE);
	pms_dev_cmd(PMS_SET_RES);
	pms_dev_cmd(3);		/* 8 counts/mm */
	pms_dev_cmd(PMS_SET_SCALE21);
#if 0
	pms_dev_cmd(PMS_SET_SAMPLE);
	pms_dev_cmd(100);	/* 100 samples/sec */
	pms_dev_cmd(PMS_SET_STREAM);
#endif
	pms_pit_cmd(PMS_INT_ENABLE);

	return 0;
}

int
opmsclose(dev, flag)
	dev_t dev;
	int flag;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];

	/* Disable interrupts. */
	pms_dev_cmd(PMS_DEV_DISABLE);
	pms_pit_cmd(PMS_INT_DISABLE);
	pms_aux_cmd(PMS_AUX_DISABLE);

	sc->sc_state &= ~PMS_OPEN;

	clfree(&sc->sc_q);

	return 0;
}

int
opmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	int s;
	int error = 0;
	size_t length;
	u_char buffer[PMS_CHUNK];

	/* Block until mouse activity occurred. */

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
		error = uiomove(buffer, length, uio);
		if (error)
			break;
	}

	return error;
}

int
opmsioctl(dev, cmd, addr, flag)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
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

int
opmsintr(arg)
	void *arg;
{
	struct opms_softc *sc = arg;
	static int state = 0;
	static u_char buttons;
	u_char changed;
	static char dx, dy;
	u_char buffer[5];

	if ((sc->sc_state & PMS_OPEN) == 0) {
		/* Interrupts are not expected.  Discard the byte. */
		kbd_flush_input();
		return 0;
	}

	switch (state) {

	case 0:
		buttons = kbd_data_read_1();
		if ((buttons & 0xc0) == 0)
			++state;
		break;

	case 1:
		dx = kbd_data_read_1();
		/* Bounding at -127 avoids a bug in XFree86. */
		dx = (dx == -128) ? -127 : dx;
		++state;
		break;

	case 2:
		dy = kbd_data_read_1();
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
			buffer[0] = 0x80 | (buttons & BUTSTATMASK);
			if(dx < 0)
				buffer[0] |= 0x10;
			buffer[1] = dx & 0x7f;
			if(dy < 0)
				buffer[0] |= 0x20;
			buffer[2] = dy & 0x7f;
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
	return -1;
}

int
opmspoll(dev, events, p)
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
