/*	$NetBSD: ms_pckbport.c,v 1.3 2006/06/07 22:38:49 kardel Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ms_pckbport.c,v 1.3 2006/06/07 22:38:49 kardel Exp $");

/*
 * Attach PS/2 mouse at pckbport aux port
 * and convert PS/2 mouse protocol to Sun firm events.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/select.h>
#include <sys/proc.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pckbport/pckbportvar.h>
#include <dev/pckbport/pmsreg.h>

#include <machine/vuid_event.h>
#include <dev/sun/event_var.h>
#include <dev/sun/msvar.h>

/*
 * NB: we {re,ab}use ms_softc input translator state and ignore its
 * zs-related members.  Not quite clean, but what the heck.
 */
struct ms_pckbport_softc {
	struct ms_softc sc_ms;

	/* pckbport attachment */
	pckbport_tag_t		sc_kbctag;
	pckbport_slot_t		sc_kbcslot;

	int sc_enabled;			/* input enabled? */
};

static int	ms_pckbport_match(struct device *, struct cfdata *, void *);
static void	ms_pckbport_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ms_pckbport, sizeof(struct ms_pckbport_softc),
    ms_pckbport_match, ms_pckbport_attach, NULL, NULL);


static int	ms_pckbport_iopen(struct device *, int);
static int	ms_pckbport_iclose(struct device *, int);
static void	ms_pckbport_input(void *, int);


static int
ms_pckbport_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pckbport_attach_args *pa = aux;

	return (pa->pa_slot == PCKBPORT_AUX_SLOT);
}


static void
ms_pckbport_attach(struct device *parent, struct device *self, void *aux)
{
	struct ms_pckbport_softc *sc = (struct ms_pckbport_softc *)self;
	struct ms_softc *ms = &sc->sc_ms;
	struct pckbport_attach_args *pa = aux;

	u_char cmd[1], resp[2];
	int res;

	/* save our pckbport attachment */
	sc->sc_kbctag = pa->pa_tag;
	sc->sc_kbcslot = pa->pa_slot;

	/* Hooks called by upper layer on device open/close */
	ms->ms_deviopen = ms_pckbport_iopen;
	ms->ms_deviclose = ms_pckbport_iclose;

	printf("\n");

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbport_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot,
			     cmd, 1, 2, resp, 1);
#ifdef DIAGNOSTIC
	if (res || resp[0] != PMS_RSTDONE || resp[1] != 0) {
		printf("ms_pckbport_attach: reset error\n");
		/* return; */
	}
#endif

	pckbport_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot,
			       ms_pckbport_input, sc, ms->ms_dev.dv_xname);

	/* no interrupts until device is actually opened */
	cmd[0] = PMS_DEV_DISABLE;
	res = pckbport_poll_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd,
			     1, 0, 0, 0);
	if (res)
		printf("ms_pckbport_attach: failed to disable interrupts\n");
	pckbport_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
}


static int
ms_pckbport_iopen(struct device *self, int flags)
{
	struct ms_pckbport_softc *sc = (struct ms_pckbport_softc *)self;
	struct ms_softc *ms = &sc->sc_ms;
	u_char cmd[1];
	int res;

	ms->ms_byteno = 0;
	ms->ms_dx = ms->ms_dy = 0;
	ms->ms_ub = ms->ms_mb = 0;

	pckbport_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

	cmd[0] = PMS_DEV_ENABLE;
	res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				cmd, 1, 0, 1, NULL);
	if (res) {
		printf("pms_enable: command error\n");
		return (res);
	}

	sc->sc_enabled = 1;
	return (0);
}


static int
ms_pckbport_iclose(struct device *self, int flags)
{
	struct ms_pckbport_softc *sc = (struct ms_pckbport_softc *)self;
	u_char cmd[1];
	int res;

	cmd[0] = PMS_DEV_DISABLE;
	res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot,
				cmd, 1, 0, 1, NULL);
	if (res)
		printf("pms_disable: command error\n");

	pckbport_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);

	sc->sc_enabled = 0;
	return (0);
}


/* Masks for the first byte of a PS/2 mouse packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04

/*
 * Got a receive interrupt - pckbport wants to give us a byte.
 */
static void
ms_pckbport_input(void *vsc, int data)
{
	struct ms_pckbport_softc *sc = vsc;
	struct ms_softc *ms = &sc->sc_ms;
	struct firm_event *fe;
	int mb, ub, d, get, put, any;

	/* map changed buttons mask to the highest bit */
	static const char to_one[] = { 1, 2, 2, 4, 4, 4, 4 };

	/* map bits to mouse buttons */
	static const int to_id[] = { MS_LEFT, MS_MIDDLE, 0, MS_RIGHT };

	if (!sc->sc_enabled) {
		/* Interrupts are not expected.  Discard the byte. */
		return;
	}

	switch (ms->ms_byteno) {

	case 0:
		if ((data & 0xc0) == 0) { /* no ovfl, bit 3 == 1 too? */
			ms->ms_mb =
				((data & PS2LBUTMASK) ? 0x1 : 0) |
				((data & PS2MBUTMASK) ? 0x2 : 0) |
				((data & PS2RBUTMASK) ? 0x4 : 0) ;
			++ms->ms_byteno;
		}
		return;

	case 1:
		ms->ms_dx += (int8_t)data;
		++ms->ms_byteno;
		return;

	case 2:
		ms->ms_dy += (int8_t)data;
		ms->ms_byteno = 0;
		break;		/* last byte processed, report changes */
	}

	any = 0;
	get = ms->ms_events.ev_get;
	put = ms->ms_events.ev_put;
	fe = &ms->ms_events.ev_q[put];

	/* NEXT prepares to put the next event, backing off if necessary */
#define	NEXT	do {						\
			if ((++put) % EV_QSIZE == get) {	\
				--put;				\
				goto out;			\
			}					\
		} while (0)

	/* ADVANCE completes the `put' of the event */
#define	ADVANCE do {						\
			++fe;					\
			if (put >= EV_QSIZE) {			\
				put = 0;			\
				fe = &ms->ms_events.ev_q[0];	\
			}					\
			any = 1;				\
		} while (0)

	ub = ms->ms_ub;		/* old buttons state */
	mb = ms->ms_mb;		/* new buttons state */
	while ((d = mb ^ ub) != 0) {
		/*
		 * Mouse button change.  Convert up to three state changes
		 * to the `first' change, and drop it into the event queue.
		 */
		NEXT;
		d = to_one[d - 1];		/* from 1..7 to {1,2,4} */
		fe->id = to_id[d - 1];		/* from {1,2,4} to ID */
		fe->value = (mb & d) ? VKEY_DOWN : VKEY_UP;
		getmicrotime(&fe->time);
		ADVANCE;
		ub ^= d;	/* reflect the button state change */
	}

	if (ms->ms_dx != 0) {
		NEXT;
		fe->id = LOC_X_DELTA;
		fe->value = ms->ms_dx;
		getmicrotime(&fe->time);
		ADVANCE;
		ms->ms_dx = 0;
	}

	if (ms->ms_dy != 0) {
		NEXT;
		fe->id = LOC_Y_DELTA;
		fe->value = ms->ms_dy;
		getmicrotime(&fe->time);
		ADVANCE;
		ms->ms_dy = 0;
	}

  out:
	if (any) {
		ms->ms_ub = ub;	/* save button state */
		ms->ms_events.ev_put = put;
		EV_WAKEUP(&ms->ms_events);
	}
}
