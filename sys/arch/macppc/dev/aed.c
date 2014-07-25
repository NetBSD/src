/*	$NetBSD: aed.c,v 1.29 2014/07/25 08:10:34 dholland Exp $	*/

/*
 * Copyright (C) 1994	Bradley A. Grantham
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aed.c,v 1.29 2014/07/25 08:10:34 dholland Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/keyboard.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/aedvar.h>
#include <macppc/dev/akbdvar.h>

#define spladb splhigh

/*
 * Function declarations.
 */
static int	aedmatch(device_t, cfdata_t, void *);
static void	aedattach(device_t, device_t, void *);
static void	aed_emulate_mouse(adb_event_t *event);
static void	aed_kbdrpt(void *kstate);
static void	aed_dokeyupdown(adb_event_t *event);
static void	aed_handoff(adb_event_t *event);
static void	aed_enqevent(adb_event_t *event);

/*
 * Global variables.
 */
extern int adb_polling;			/* Are we polling?  (Debugger mode) */

/*
 * Local variables.
 */
static struct aed_softc *aed_sc = NULL;
static int aed_options = 0; /* | AED_MSEMUL; */

/* Driver definition */
CFATTACH_DECL_NEW(aed, sizeof(struct aed_softc),
    aedmatch, aedattach, NULL, NULL);

extern struct cfdriver aed_cd;

dev_type_open(aedopen);
dev_type_close(aedclose);
dev_type_read(aedread);
dev_type_ioctl(aedioctl);
dev_type_poll(aedpoll);
dev_type_kqfilter(aedkqfilter);

const struct cdevsw aed_cdevsw = {
	.d_open = aedopen,
	.d_close = aedclose,
	.d_read = aedread,
	.d_write = nullwrite,
	.d_ioctl = aedioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = aedpoll,
	.d_mmap = nommap,
	.d_kqfilter = aedkqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static int
aedmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct adb_attach_args *aa_args = (struct adb_attach_args *)aux;
	static int aed_matched = 0;

	/* Allow only one instance. */
        if ((aa_args->origaddr == 0) && (!aed_matched)) {
		aed_matched = 1;
                return (1);
        } else
                return (0);
}

static void
aedattach(device_t parent, device_t self, void *aux)
{
	struct adb_attach_args *aa_args = (struct adb_attach_args *)aux;
	struct aed_softc *sc = device_private(self);

	callout_init(&sc->sc_repeat_ch, 0);
	selinit(&sc->sc_selinfo);

	sc->origaddr = aa_args->origaddr;
	sc->adbaddr = aa_args->adbaddr;
	sc->handler_id = aa_args->handler_id;

	sc->sc_evq_tail = 0;
	sc->sc_evq_len = 0;

	sc->sc_rptdelay = 20;
	sc->sc_rptinterval = 6;
	sc->sc_repeating = -1;          /* not repeating */

	/* Pull in the options flags. */ 
	sc->sc_options = (device_cfdata(self)->cf_flags | aed_options);

	sc->sc_ioproc = NULL;
	
	sc->sc_buttons = 0;

	sc->sc_open = 0;

	aed_sc = sc;

	printf("ADB Event device\n");

	return;
}

/*
 * Given a keyboard ADB event, record the keycode and call the key 
 * repeat handler, optionally passing the event through the mouse
 * button emulation handler first.  Pass mouse events directly to
 * the handoff function.
 */
void
aed_input(adb_event_t *event)
{
        adb_event_t new_event = *event;

	switch (event->def_addr) {
	case ADBADDR_KBD:
		if (aed_sc->sc_options & AED_MSEMUL)
			aed_emulate_mouse(&new_event);
		else
			aed_dokeyupdown(&new_event);
		break;
	case ADBADDR_MS:
		new_event.u.m.buttons |= aed_sc->sc_buttons;
		aed_handoff(&new_event);
		break;
	default:                /* God only knows. */
#ifdef DIAGNOSTIC
		panic("aed: received event from unsupported device!");
#endif
		break;
	}

}

/*
 * Handles mouse button emulation via the keyboard.  If the emulation
 * modifier key is down, left and right arrows will generate 2nd and
 * 3rd mouse button events while the 1, 2, and 3 keys will generate
 * the corresponding mouse button event.
 */
static void 
aed_emulate_mouse(adb_event_t *event)
{
	static int emulmodkey_down = 0;
	adb_event_t new_event;

	if (event->u.k.key == ADBK_KEYDOWN(ADBK_OPTION)) {
		emulmodkey_down = 1;
	} else if (event->u.k.key == ADBK_KEYUP(ADBK_OPTION)) {
		/* key up */
		emulmodkey_down = 0;
		if (aed_sc->sc_buttons & 0xfe) {
			aed_sc->sc_buttons &= 1;
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
		}
	} else if (emulmodkey_down) {
		switch(event->u.k.key) {
#ifdef ALTXBUTTONS
		case ADBK_KEYDOWN(ADBK_1):
			aed_sc->sc_buttons |= 1;	/* left down */
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
			break;
		case ADBK_KEYUP(ADBK_1):
			aed_sc->sc_buttons &= ~1;	/* left up */
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
			break;
#endif
		case ADBK_KEYDOWN(ADBK_LEFT):
#ifdef ALTXBUTTONS
		case ADBK_KEYDOWN(ADBK_2):
#endif
			aed_sc->sc_buttons |= 2;	/* middle down */
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
			break;
		case ADBK_KEYUP(ADBK_LEFT):
#ifdef ALTXBUTTONS
		case ADBK_KEYUP(ADBK_2):
#endif
			aed_sc->sc_buttons &= ~2;	/* middle up */
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
			break;
		case ADBK_KEYDOWN(ADBK_RIGHT):
#ifdef ALTXBUTTONS
		case ADBK_KEYDOWN(ADBK_3):
#endif
			aed_sc->sc_buttons |= 4;	/* right down */
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
			break;
		case ADBK_KEYUP(ADBK_RIGHT):
#ifdef ALTXBUTTONS
		case ADBK_KEYUP(ADBK_3):
#endif
			aed_sc->sc_buttons &= ~4;	/* right up */
			new_event.def_addr = ADBADDR_MS;
			new_event.u.m.buttons = aed_sc->sc_buttons;
			new_event.u.m.dx = new_event.u.m.dy = 0;
			microtime(&new_event.timestamp);
			aed_handoff(&new_event);
			break;
		case ADBK_KEYUP(ADBK_SHIFT):
		case ADBK_KEYDOWN(ADBK_SHIFT):
		case ADBK_KEYUP(ADBK_CONTROL):
		case ADBK_KEYDOWN(ADBK_CONTROL):
		case ADBK_KEYUP(ADBK_FLOWER):
		case ADBK_KEYDOWN(ADBK_FLOWER):
			/* ctrl, shift, cmd */
			aed_dokeyupdown(event);
			break;
		default:
			if (event->u.k.key & 0x80)
				/* ignore keyup */
				break;

			/* key down */
			new_event = *event;

			/* send option-down */
			new_event.u.k.key = ADBK_KEYDOWN(ADBK_OPTION);
			new_event.bytes[0] = new_event.u.k.key;
			microtime(&new_event.timestamp);
			aed_dokeyupdown(&new_event);

			/* send key-down */
			new_event.u.k.key = event->bytes[0];
			new_event.bytes[0] = new_event.u.k.key;
			microtime(&new_event.timestamp);
			aed_dokeyupdown(&new_event);

			/* send key-up */
			new_event.u.k.key =
				ADBK_KEYUP(ADBK_KEYVAL(event->bytes[0]));
			microtime(&new_event.timestamp);
			new_event.bytes[0] = new_event.u.k.key;
			aed_dokeyupdown(&new_event);

			/* send option-up */
			new_event.u.k.key = ADBK_KEYUP(ADBK_OPTION);
			new_event.bytes[0] = new_event.u.k.key;
			microtime(&new_event.timestamp);
			aed_dokeyupdown(&new_event);
			break;
		}
	} else {
		aed_dokeyupdown(event);
	}
}

/*
 * Keyboard autorepeat timeout function.  Sends key up/down events
 * for the repeating key and schedules the next call at sc_rptinterval
 * ticks in the future.
 */
static void 
aed_kbdrpt(void *kstate)
{
	struct aed_softc *sc = (struct aed_softc *)kstate;

	sc->sc_rptevent.bytes[0] |= 0x80;
	microtime(&sc->sc_rptevent.timestamp);
	aed_handoff(&sc->sc_rptevent);	/* do key up */

	sc->sc_rptevent.bytes[0] &= 0x7f;
	microtime(&sc->sc_rptevent.timestamp);
	aed_handoff(&sc->sc_rptevent);	/* do key down */

	if (sc->sc_repeating == sc->sc_rptevent.u.k.key) {
		callout_reset(&sc->sc_repeat_ch, sc->sc_rptinterval,
		    aed_kbdrpt, kstate);
	}
}


/*
 * Cancels the currently repeating key event if there is one, schedules
 * a new repeating key event if needed, and hands the event off to the
 * appropriate subsystem.
 */
static void 
aed_dokeyupdown(adb_event_t *event)
{
	int     kbd_key;

	kbd_key = ADBK_KEYVAL(event->u.k.key);
	if (ADBK_PRESS(event->u.k.key) && keyboard[kbd_key][0] != 0) {
		/* ignore shift & control */
		if (aed_sc->sc_repeating != -1) {
			callout_stop(&aed_sc->sc_repeat_ch);
		}
		aed_sc->sc_rptevent = *event;
		aed_sc->sc_repeating = kbd_key;
		callout_reset(&aed_sc->sc_repeat_ch, aed_sc->sc_rptdelay,
		    aed_kbdrpt, (void *)aed_sc);
	} else {
		if (aed_sc->sc_repeating != -1) {
			aed_sc->sc_repeating = -1;
			callout_stop(&aed_sc->sc_repeat_ch);
		}
		aed_sc->sc_rptevent = *event;
	}
	aed_handoff(event);
}

/*
 * Place the event in the event queue if a requesting device is open
 * and we are not polling.
 */
static void
aed_handoff(adb_event_t *event)
{
	if (aed_sc->sc_open && !adb_polling)
		aed_enqevent(event);
}

/*
 * Place the event in the event queue and wakeup any waiting processes.
 */
static void 
aed_enqevent(adb_event_t *event)
{
	int     s;

	s = spladb();

#ifdef DIAGNOSTIC
	if (aed_sc->sc_evq_tail < 0 || aed_sc->sc_evq_tail >= AED_MAX_EVENTS)
		panic("adb: event queue tail is out of bounds");

	if (aed_sc->sc_evq_len < 0 || aed_sc->sc_evq_len > AED_MAX_EVENTS)
		panic("adb: event queue len is out of bounds");
#endif

	if (aed_sc->sc_evq_len == AED_MAX_EVENTS) {
		splx(s);
		return;		/* Oh, well... */
	}
	aed_sc->sc_evq[(aed_sc->sc_evq_len + aed_sc->sc_evq_tail) %
	    AED_MAX_EVENTS] = *event;
	aed_sc->sc_evq_len++;

	selnotify(&aed_sc->sc_selinfo, 0, 0);
	if (aed_sc->sc_ioproc)
		psignal(aed_sc->sc_ioproc, SIGIO);

	splx(s);
}

int 
aedopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit;
	int error = 0;
	int s;

	unit = minor(dev);

	if (unit != 0)
		return (ENXIO);

	s = spladb();
	if (aed_sc->sc_open) {
		splx(s);
		return (EBUSY);
	}
	aed_sc->sc_evq_tail = 0;
	aed_sc->sc_evq_len = 0;
	aed_sc->sc_open = 1;
	aed_sc->sc_ioproc = l->l_proc;
	splx(s);

	return (error);
}


int 
aedclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int s = spladb();

	aed_sc->sc_open = 0;
	aed_sc->sc_ioproc = NULL;
	splx(s);

	return (0);
}


int 
aedread(dev_t dev, struct uio *uio, int flag)
{
	int s, error;
	int willfit;
	int total;
	int firstmove;
	int moremove;

	if (uio->uio_resid < sizeof(adb_event_t))
		return (EMSGSIZE);	/* close enough. */

	s = spladb();
	if (aed_sc->sc_evq_len == 0) {
		splx(s);
		return (0);
	}
	willfit = howmany(uio->uio_resid, sizeof(adb_event_t));
	total = (aed_sc->sc_evq_len < willfit) ? aed_sc->sc_evq_len : willfit;

	firstmove = (aed_sc->sc_evq_tail + total > AED_MAX_EVENTS)
	    ? (AED_MAX_EVENTS - aed_sc->sc_evq_tail) : total;

	error = uiomove((void *) & aed_sc->sc_evq[aed_sc->sc_evq_tail],
	    firstmove * sizeof(adb_event_t), uio);
	if (error) {
		splx(s);
		return (error);
	}
	moremove = total - firstmove;

	if (moremove > 0) {
		error = uiomove((void *) & aed_sc->sc_evq[0],
		    moremove * sizeof(adb_event_t), uio);
		if (error) {
			splx(s);
			return (error);
		}
	}
	aed_sc->sc_evq_tail = (aed_sc->sc_evq_tail + total) % AED_MAX_EVENTS;
	aed_sc->sc_evq_len -= total;
	splx(s);
	return (0);
}

int 
aedioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	switch (cmd) {
	case ADBIOCDEVSINFO: {
		adb_devinfo_t *di;
		ADBDataBlock adbdata;
		int totaldevs;
		int adbaddr;
		int i;

		di = (void *)data;

		/* Initialize to no devices */
		for (i = 0; i < 16; i++)
			di->dev[i].addr = -1;

		totaldevs = CountADBs();
		for (i = 1; i <= totaldevs; i++) {
			adbaddr = GetIndADB(&adbdata, i);
			di->dev[adbaddr].addr = adbaddr;
			di->dev[adbaddr].default_addr = (int)(adbdata.origADBAddr);
			di->dev[adbaddr].handler_id = (int)(adbdata.devType);
			}

		/* Must call ADB Manager to get devices now */
		break;
	}

	case ADBIOCGETREPEAT:{
		adb_rptinfo_t *ri;

		ri = (void *)data;
		ri->delay_ticks = aed_sc->sc_rptdelay;
		ri->interval_ticks = aed_sc->sc_rptinterval;
		break;
	}

	case ADBIOCSETREPEAT:{
		adb_rptinfo_t *ri;

		ri = (void *) data;
		aed_sc->sc_rptdelay = ri->delay_ticks;
		aed_sc->sc_rptinterval = ri->interval_ticks;
		break;
	}

	case ADBIOCRESET:
		/* Do nothing for now */
		break;

	case ADBIOCLISTENCMD:{
		adb_listencmd_t *lc;

		lc = (void *)data;
	}

	default:
		return (EINVAL);
	}
	return (0);
}


int 
aedpoll(dev_t dev, int events, struct lwp *l)
{
	int s, revents;

	revents = events & (POLLOUT | POLLWRNORM);
	
	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	s = spladb();
	if (aed_sc->sc_evq_len > 0)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(l, &aed_sc->sc_selinfo);
	splx(s);

	return (revents);
}

static void
filt_aedrdetach(struct knote *kn)
{
	int s;

	s = spladb();
	SLIST_REMOVE(&aed_sc->sc_selinfo.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_aedread(struct knote *kn, long hint)
{

	kn->kn_data = aed_sc->sc_evq_len * sizeof(adb_event_t);
	return (kn->kn_data > 0);
}

static const struct filterops aedread_filtops =
	{ 1, NULL, filt_aedrdetach, filt_aedread };

static const struct filterops aed_seltrue_filtops =
	{ 1, NULL, filt_aedrdetach, filt_seltrue };

int
aedkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &aed_sc->sc_selinfo.sel_klist;
		kn->kn_fop = &aedread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &aed_sc->sc_selinfo.sel_klist;
		kn->kn_fop = &aed_seltrue_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = NULL;

	s = spladb();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}
