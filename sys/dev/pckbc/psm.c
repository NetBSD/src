/* $NetBSD: psm.c,v 1.15 2002/03/10 22:21:21 christos Exp $ */

/*-
 * Copyright (c) 1994 Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: psm.c,v 1.15 2002/03/10 22:21:21 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kthread.h>

#include <machine/bus.h>

#include <dev/ic/pckbcvar.h>

#include <dev/pckbc/psmreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

enum pms_protocol { PMS_UNKNOWN, PMS_STANDARD, PMS_SCROLL3, PMS_SCROLL5 };

static const char *pms_protocol_name[] = {
	"unknown",
	"standard (no scroll wheel)",
	"scroll wheel (3 buttons)",
	"scroll wheel (5 buttons)",
};

struct pms_softc {		/* driver status information */
	struct device sc_dev;

	pckbc_tag_t sc_kbctag;
	int sc_kbcslot;

	int sc_enabled;		/* input enabled? */
#ifndef PMS_DISABLE_POWERHOOK
	void *sc_powerhook;	/* cookie from power hook */
#endif /* !PMS_DISABLE_POWERHOOK */
	int inputstate;
	u_int buttons, oldbuttons;	/* mouse button status */
	signed char dx, dy, dz;
	enum pms_protocol protocol;
	char inbuf[4];

	struct device *sc_wsmousedev;
	struct proc *sc_event_thread;
	int sc_reset_flag;
	unsigned long packet;
	struct timeval last, current;
};

int pmsprobe __P((struct device *, struct cfdata *, void *));
void pmsattach __P((struct device *, struct device *, void *));
void pmsinput __P((void *, int));

struct cfattach pms_ca = {
	sizeof(struct pms_softc), pmsprobe, pmsattach,
};

static int	pms_protocol __P((pckbc_tag_t, pckbc_slot_t));
static void	do_enable __P((struct pms_softc *));
static void	do_disable __P((struct pms_softc *));
static void	pms_reset_thread __P((void*));
static void	pms_spawn_reset_thread __P((void*));
int	pms_enable __P((void *));
int	pms_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
void	pms_disable __P((void *));
#ifndef PMS_DISABLE_POWERHOOK
void	pms_power __P((int, void *));
#endif /* !PMS_DISABLE_POWERHOOK */

const struct wsmouse_accessops pms_accessops = {
	pms_enable,
	pms_ioctl,
	pms_disable,
};

static int
pms_protocol(tag, slot)
	pckbc_tag_t tag;
	pckbc_slot_t slot;
{
	u_char cmd[2], resp[1];
	int i, j, res;
	static const struct {
		int rates[3], response;
		enum pms_protocol p;
	} protocols[] = {
		{ { 200, 200, 80 }, 4, PMS_SCROLL5 },
		{ { 200, 100, 80 }, 3, PMS_SCROLL3 },
		{ { 0, 0, 0 }, 0, PMS_STANDARD },
	};

	for (j = 0; protocols[j].rates[0]; ++j) {
		cmd[0] = PMS_SET_SAMPLE;
		for (i = 0; i < 3; i++) {
			cmd[1] = protocols[j].rates[i];
			res = pckbc_poll_cmd(tag, slot, cmd, 2, 0, 0, 0);
			if (res)
				return 0;
		}

		cmd[0] = PMS_SEND_DEV_ID;
		res = pckbc_poll_cmd(tag, slot, cmd, 1, 1, resp, 0);
		if (res)
			return 0;
		if (resp[0] == protocols[j].response) {
#ifdef PS2DEBUG
			printf("returning protocol %d\n", protocols[j].p);
#endif
			return protocols[j].p;
		}
	}
#ifdef PS2DEBUG
	printf("standard protocol\n");
#endif
	return PMS_STANDARD;
}

int
pmsprobe(parent, match, aux)
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
		printf("pmsprobe: reset error %d\n", res);
#endif
		return (0);
	}
	if (resp[0] != PMS_RSTDONE) {
		printf("pmsprobe: reset response 0x%x\n", resp[0]);
		return (0);
	}

	/* get type number (0 = mouse) */
	if (resp[1] != 0) {
#ifdef DEBUG
		printf("pmsprobe: type 0x%x\n", resp[1]);
#endif
		return (0);
	}

	return (10);
}

void
pmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pms_softc *sc = (void *)self;
	struct pckbc_attach_args *pa = aux;
	struct wsmousedev_attach_args a;
	u_char cmd[1], resp[2];
	int res;

	sc->sc_kbctag = pa->pa_tag;
	sc->sc_kbcslot = pa->pa_slot;

	sc->last.tv_sec = sc->current.tv_sec = 0;
	sc->last.tv_usec = sc->current.tv_usec = 0;
	sc->packet = 0;

	/* Flush any garbage. */
	pckbc_flush(pa->pa_tag, pa->pa_slot);

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 2, resp, 1);
#ifdef DEBUG
	if (res || resp[0] != PMS_RSTDONE || resp[1] != 0) {
		printf("pmsattach: reset error\n");
		return;
	}
#endif
	res = pms_protocol(pa->pa_tag, pa->pa_slot);
	if (!res) {
#ifdef DEBUG
		printf("pmsattach: error setting protocol\n");
#endif
		sc->protocol = PMS_STANDARD;
	} else {
		sc->protocol = res;
	}
	printf(" %s\n", pms_protocol_name[sc->protocol]);

	sc->inputstate = 0;
	sc->oldbuttons = 0;

	pckbc_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot,
			       pmsinput, sc, sc->sc_dev.dv_xname);

	a.accessops = &pms_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wsmouse, saving a handle to it.
	 * Note that we don't need to check this pointer against NULL
	 * here or in pmsintr, because if this fails pms_enable() will
	 * never be called, so pmsinput() will never be called.
	 */
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	/* no interrupts until enabled */
	cmd[0] = PMS_DEV_DISABLE;
	res = pckbc_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 0, 0, 0);
	if (res)
		printf("pmsattach: disable error\n");
	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);

	sc->sc_reset_flag = 0;

	kthread_create(pms_spawn_reset_thread, sc);

#ifndef PMS_DISABLE_POWERHOOK
	sc->sc_powerhook = powerhook_establish(pms_power, sc);
#endif /* !PMS_DISABLE_POWERHOOK */
}

static void
do_enable(sc)
	struct pms_softc *sc;
{
	u_char cmd[1];
	int res;

	sc->inputstate = 0;
	sc->oldbuttons = 0;

	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

	cmd[0] = PMS_DEV_ENABLE;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd, 1, 0, 1, 0);
	if (res)
		printf("pms_enable: command error\n");

	res = pms_protocol(sc->sc_kbctag, sc->sc_kbcslot);
	if (res) {
		sc->protocol = res;
#ifdef DEBUG
		printf("psm_enable: using %s protocol\n",
		    pms_protocol_name[sc->protocol]);
	} else {
		printf("psm_enable: couldn't verify protocol\n");

#endif
	}
#if 0
	{
		u_char scmd[2];

		scmd[0] = PMS_SET_RES;
		scmd[1] = 3; /* 8 counts/mm */
		res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
		    2, 0, 1, 0);
		if (res)
			printf("pms_enable: setup error1 (%d)\n", res);

		scmd[0] = PMS_SET_SCALE21;
		res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
		    1, 0, 1, 0);
		if (res)
			printf("pms_enable: setup error2 (%d)\n", res);

		scmd[0] = PMS_SET_SAMPLE;
		scmd[1] = 100; /* 100 samples/sec */
		res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
		    2, 0, 1, 0);
		if (res)
			printf("pms_enable: setup error3 (%d)\n", res);
	}
#endif
}

static void
do_disable(sc)
	struct pms_softc *sc;
{
	u_char cmd[1];
	int res;

	cmd[0] = PMS_DEV_DISABLE;
	res = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd, 1, 0, 1, 0);
	if (res)
		printf("pms_disable: command error\n");

	pckbc_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
}

int
pms_enable(v)
	void *v;
{
	struct pms_softc *sc = v;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;

	do_enable(sc);

	return 0;
}

void
pms_disable(v)
	void *v;
{
	struct pms_softc *sc = v;

	do_disable(sc);

	sc->sc_enabled = 0;
}

#ifndef PMS_DISABLE_POWERHOOK
void
pms_power(why, v)
	int why;
	void *v;
{
	struct pms_softc *sc = v;

	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		if (sc->sc_enabled)
			do_disable(sc);
		break;
	case PWR_RESUME:
		if (sc->sc_enabled)
			do_enable(sc);
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
}
#endif /* !PMS_DISABLE_POWERHOOK */

int
pms_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pms_softc *sc = v;
	u_char kbcmd[2];
	int i;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		break;
		
	case WSMOUSEIO_SRES:
		i = (*(u_int *)data - 12) / 25;
		
		if (i < 0)
			i = 0;
			
		if (i > 3)
			i = 3;

		kbcmd[0] = PMS_SET_RES;
		kbcmd[1] = i;			
		i = pckbc_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, kbcmd, 
		    2, 0, 1, 0);
		
		if (i)
			printf("pms_ioctl: SET_RES command error\n");
		break;
		
	default:
		return (-1);
	}
	return (0);
}

static void
pms_spawn_reset_thread(arg)
	void *arg;
{
	struct pms_softc *sc = arg;
	kthread_create1(pms_reset_thread, sc, &sc->sc_event_thread,
	    "pms reset thread");
}

static void
pms_reset_thread(arg)
	void *arg;
{
	struct pms_softc *sc = arg;
	for (;;) {
		tsleep(&sc->sc_reset_flag, PWAIT, "pmsreset", 0);
		do_disable(sc);
		do_enable(sc);
	}
}

/* Masks for the first byte of a packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04
#define PS24BUTMASK 0x10
#define PS25BUTMASK 0x20

void pmsinput(vsc, data)
void *vsc;
int data;
{
	struct pms_softc *sc = vsc;
	u_int changed;

	if (!sc->sc_enabled) {
		/* Interrupts are not expected.	 Discard the byte. */
		return;
	}

	microtime(&sc->current);
	if (sc->inputstate != 0 && ((sc->current.tv_sec != sc->last.tv_sec ||
	    (sc->current.tv_usec - sc->last.tv_usec) > 10000))) {
#if DEBUG
		printf("psm_input: unusual delay, spawning reset thread\n");
#endif
		wakeup(&sc->sc_reset_flag);
		sc->inputstate = 0;
		return;
	}
	sc->last = sc->current;
	switch (sc->inputstate) {
	case 0:
		sc->packet = (data & 0xff) << 24;
		if ((data & 0xc0) == 0) { /* no ovfl, bit 3 == 1 too? */
			sc->buttons = ((data & PS2LBUTMASK) ? 0x1 : 0) |
			    ((data & PS2MBUTMASK) ? 0x2 : 0) |
			    ((data & PS2RBUTMASK) ? 0x4 : 0);
			++sc->inputstate;
#ifdef PS2DEBUG
		} else {
  			printf("got data 0x%x\n", data);
#endif
  		}
		break;

	case 1:
		sc->packet |= (data & 0xff) << 16;
		sc->dx = data;
		/* Bounding at -127 avoids a bug in XFree86. */
		sc->dx = (sc->dx == -128) ? -127 : sc->dx;
		++sc->inputstate;
		break;

	case 2:
		sc->packet |= (data & 0xff) << 8;
		sc->dy = data;
		sc->dy = (sc->dy == -128) ? -127 : sc->dy;

		if (sc->protocol == PMS_STANDARD) {
#ifdef PS2DEBUG
			printf("packet: 0x%08lx\n", packet);
#endif
			sc->inputstate = 0;
			changed = (sc->buttons ^ sc->oldbuttons);
			sc->oldbuttons = sc->buttons;

			if (sc->dx || sc->dy || changed) {
				wsmouse_input(sc->sc_wsmousedev,
				    sc->buttons, sc->dx, sc->dy, 0,
				    WSMOUSE_INPUT_DELTA);
			}
		} else
			++sc->inputstate;
		break;

	case 3:
		sc->packet |= (data & 0xff);
		sc->inputstate = 0;
		if (sc->protocol == PMS_SCROLL5) {
			sc->dz = data & 0xf;
			if (sc->dz & 0x8)
				sc->dz -= 16;
			if (data & PS24BUTMASK)
				sc->buttons |= 0x8;
			if (data & PS25BUTMASK)
				sc->buttons |= 0x10;
		} else {
			sc->dz = data;
			if (sc->dz == -128)
				sc->dz = -127;
		}
#ifdef PS2DEBUG
		printf("packet: 0x%08lx\n", packet);
#endif

		changed = (sc->buttons ^ sc->oldbuttons);
		sc->oldbuttons = sc->buttons;

		if (sc->dx || sc->dy || sc->dz || changed) {
			wsmouse_input(sc->sc_wsmousedev,
			    sc->buttons, sc->dx, sc->dy, sc->dz,
			    WSMOUSE_INPUT_DELTA);
		}
		break;
	}
}
