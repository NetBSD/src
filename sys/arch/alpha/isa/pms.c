/* $NetBSD: pms.c,v 1.6.2.4 1997/06/01 04:12:54 cgd Exp $ */

/*-
 * Copyright (c) 1994 Charles Hannum.
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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pms.c,v 1.6.2.4 1997/06/01 04:12:54 cgd Exp $");

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

#include <machine/intr.h>
#include <dev/isa/isavar.h>
#include <machine/wsconsio.h>
#include <alpha/wscons/wsmousevar.h>
#include <alpha/isa/pcppivar.h>

#define	PMS_DATA	0x0	/* offset for data port, read-write */
#define	PMS_CNTRL	0x4	/* offset for control port, write-only */
#define	PMS_STATUS	0x4	/* offset for status port, read-only */

/* status bits */
#define	PMS_OBUF_FULL	0x01
#define	PMS_IBUF_FULL	0x02

/* controller commands */
#define	PMS_INT_ENABLE	0x47	/* enable controller interrupts */
#define	PMS_INT_DISABLE	0x65	/* disable controller interrupts */
#define	PMS_AUX_ENABLE	0xa8	/* enable auxiliary port */
#define	PMS_AUX_DISABLE	0xa7	/* disable auxiliary port */
#define	PMS_AUX_TEST	0xa9	/* test auxiliary port */

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

struct pms_softc {		/* driver status information */
	struct device sc_dev;

	void *sc_ih;

	int sc_enabled;		/* input enabled? */
	u_int sc_buttonstatus;	/* mouse button status */

	struct device *sc_wsmousedev;
};

bus_space_tag_t pms_iot;
bus_space_handle_t pms_ioh;
isa_chipset_tag_t pms_ic;

int pmsprobe __P((struct device *, struct cfdata *, void *));
void pmsattach __P((struct device *, struct device *, void *));
int pmsintr __P((void *));

struct cfattach pms_ca = {
	sizeof(struct pms_softc), pmsprobe, pmsattach,
};

struct cfdriver pms_cd = {
	NULL, "pms", DV_TTY,
};

#define	PMSUNIT(dev)	(minor(dev))

int	pms_enable __P((void *));
int	pms_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
void	pms_disable __P((void *));

const struct wsmouse_accessops pms_accessops = {
	pms_enable,
	pms_ioctl,
	pms_disable,
};

static void pms_flush __P((void));
static void pms_dev_cmd __P((u_char));
static void pms_aux_cmd __P((u_char));
static void pms_pit_cmd __P((u_char));

static inline void
pms_flush()
{
	u_char c;

	while ((c = bus_space_read_1(pms_iot, pms_ioh, PMS_STATUS) & 0x03) != 0)
		if ((c & PMS_OBUF_FULL) == PMS_OBUF_FULL) {
			/* XXX - delay is needed to prevent some keyboards from
			   wedging when the system boots */
			delay(6);
			(void) bus_space_read_1(pms_iot, pms_ioh, PMS_DATA);
		}
}

static inline void
pms_dev_cmd(value)
	u_char value;
{

	pms_flush();
	bus_space_write_1(pms_iot, pms_ioh, PMS_CNTRL, 0xd4);
	pms_flush();
	bus_space_write_1(pms_iot, pms_ioh, PMS_DATA, value);
}

static inline void
pms_aux_cmd(value)
	u_char value;
{

	pms_flush();
	bus_space_write_1(pms_iot, pms_ioh, PMS_CNTRL, value);
}

static inline void
pms_pit_cmd(value)
	u_char value;
{

	pms_flush();
	bus_space_write_1(pms_iot, pms_ioh, PMS_CNTRL, 0x60);
	pms_flush();
	bus_space_write_1(pms_iot, pms_ioh, PMS_DATA, value);
}

int
pmsprobe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcppi_attach_args *pa = aux;
	u_char x;

	if (pa->pa_slot != PCPPI_AUX_SLOT)
		return 0;

	pms_iot = pa->pa_iot;
	pms_ioh = pa->pa_ioh;

	pms_dev_cmd(PMS_RESET);
	pms_aux_cmd(PMS_AUX_TEST);
	delay(1000);
	x = bus_space_read_1(pms_iot, pms_ioh, PMS_DATA);
	pms_pit_cmd(PMS_INT_DISABLE);
	if (x & 0x04)
		return 0;

	return 1;
}

void
pmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pms_softc *sc = (void *)self;
	struct pcppi_attach_args *pa = aux;
	struct wsmousedev_attach_args a;

	pms_iot = pa->pa_iot;
	pms_ioh = pa->pa_ioh;
	pms_ic = pa->pa_ic;

	printf("\n");

	/* Other initialization was done by pmsprobe. */
	sc->sc_buttonstatus = 0;

	sc->sc_ih = isa_intr_establish(pms_ic, 12, IST_EDGE, IPL_TTY,
	    pmsintr, sc);

	a.accessops = &pms_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wsmouse, saving a handle to it.
	 * Note that we don't need to check this pointer against NULL
	 * here or in pmsintr, because if this fails pms_enable() will
	 * never be called, so pmsintr() will never be called.
	 */
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}

int
pms_enable(v)
	void *v;
{
	struct pms_softc *sc = v;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;
	sc->sc_buttonstatus = 0;

	/* Enable interrupts. */
	pms_dev_cmd(PMS_DEV_ENABLE);
	pms_aux_cmd(PMS_AUX_ENABLE);
#if 0
	pms_dev_cmd(PMS_SET_RES);
	pms_dev_cmd(3);		/* 8 counts/mm */
	pms_dev_cmd(PMS_SET_SCALE21);
	pms_dev_cmd(PMS_SET_SAMPLE);
	pms_dev_cmd(100);	/* 100 samples/sec */
	pms_dev_cmd(PMS_SET_STREAM);
#endif
	pms_pit_cmd(PMS_INT_ENABLE);

	return 0;
}

int
pms_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
#if 0
	struct pms_softc *sc = v;
#endif

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		return (0);
	}
	return (-1);
}
void
pms_disable(v)
	void *v;
{
	struct pms_softc *sc = v;

	/* Disable interrupts. */
	pms_dev_cmd(PMS_DEV_DISABLE);
	pms_pit_cmd(PMS_INT_DISABLE);
	pms_aux_cmd(PMS_AUX_DISABLE);

	sc->sc_enabled = 0;
}

/* Masks for the first byte of a packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04

int
pmsintr(arg)
	void *arg;
{
	struct pms_softc *sc = arg;
	static int state = 0;
	static u_int buttons;
	static signed char dx, dy;
	u_int changed;

	if (!sc->sc_enabled) {
		/* Interrupts are not expected.  Discard the byte. */
		pms_flush();
		return (0);
	}

	switch (state) {

	case 0:
		buttons = bus_space_read_1(pms_iot, pms_ioh, PMS_DATA);
		if ((buttons & 0xc0) == 0)
			++state;
		break;

	case 1:
		dx = bus_space_read_1(pms_iot, pms_ioh, PMS_DATA);
		/* Bounding at -127 avoids a bug in XFree86. */
		dx = (dx == -128) ? -127 : dx;
		++state;
		break;

	case 2:
		dy = bus_space_read_1(pms_iot, pms_ioh, PMS_DATA);
		dy = (dy == -128) ? -127 : dy;
		state = 0;

		buttons = ((buttons & PS2LBUTMASK) ? 0x1 : 0) |
		    ((buttons & PS2MBUTMASK) ? 0x2 : 0) |
		    ((buttons & PS2RBUTMASK) ? 0x4 : 0);
		changed = (buttons ^ sc->sc_buttonstatus);
		sc->sc_buttonstatus = buttons;

		if (dx || dy || changed)
			wsmouse_input(sc->sc_wsmousedev, buttons, dx, dy);
		break;
	}

	return -1;
}
