/*	$NetBSD: kbd.c,v 1.26.4.1 2002/06/06 17:08:33 he Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)kbd.c	8.2 (Berkeley) 10/30/93
 */

/*
 * Keyboard driver (/dev/kbd -- note that we do not have minor numbers
 * [yet?]).  Translates incoming bytes to ASCII or to `firm_events' and
 * passes them up to the appropriate reader.
 */

/*
 * This is the "slave" driver that will be attached to
 * the "zsc" driver for a Sun keyboard.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/poll.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>
#include <machine/vuid_event.h>
#include <machine/kbd.h>
#include <machine/kbio.h>
#include <dev/sun/event_var.h>
#include <dev/sun/kbd_xlate.h>
#include <dev/sun/kbdvar.h>

#include "locators.h"

/*
 * Ideas:
 * /dev/kbd is not a tty (plain device)
 */

/* Prototypes */
static void	kbd_new_layout __P((struct kbd_softc *));
static void	kbd_repeat __P((void *));
static void	kbd_set_leds __P((struct kbd_softc *, int));
static void	kbd_update_leds __P((struct kbd_softc *));
static void	kbd_was_reset __P((struct kbd_softc *));
static int 	kbd_drain_tx __P((struct kbd_softc *));
static int	kbd_iopen __P((struct kbd_softc *));
static int	kbd_iclose __P((struct kbd_softc *));

cdev_decl(kbd);	/* open, close, read, write, ioctl, stop, ... */

extern struct cfdriver kbd_cd;

/****************************************************************
 *  Entry points for /dev/kbd
 *  (open,close,read,write,...)
 ****************************************************************/

/*
 * Open:
 * Check exclusion, open actual device (_iopen),
 * setup event channel, clear ASCII repeat stuff.
 */
int
kbdopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct kbd_softc *k;
	int error, unit;

	unit = minor(dev);
	if (unit >= kbd_cd.cd_ndevs)
		return (ENXIO);
	k = kbd_cd.cd_devs[unit];
	if (k == NULL)
		return (ENXIO);

	/* Exclusive open required for /dev/kbd */
	if (k->k_events.ev_io)
		return (EBUSY);
	k->k_events.ev_io = p;

	if ((error = kbd_iopen(k)) != 0) {
		k->k_events.ev_io = NULL;
		return (error);
	}
	ev_init(&k->k_events);
	k->k_evmode = 1;	/* XXX: OK? */

	if (k->k_repeating) {
		k->k_repeating = 0;
		callout_stop(&k->k_repeat_ch);
	}

	return (0);
}

/*
 * Close:
 * Turn off event mode, dump the queue, and close the keyboard
 * unless it is supplying console input.
 */
int
kbdclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct kbd_softc *k;

	k = kbd_cd.cd_devs[minor(dev)];
	k->k_evmode = 0;
	ev_fini(&k->k_events);
	k->k_events.ev_io = NULL;
	return (0);
}

int
kbdread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct kbd_softc *k;

	k = kbd_cd.cd_devs[minor(dev)];
	return (ev_read(&k->k_events, uio, flags));
}

/* this routine should not exist, but is convenient to write here for now */
int
kbdwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{

	return (EOPNOTSUPP);
}

int
kbdpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct kbd_softc *k;

	k = kbd_cd.cd_devs[minor(dev)];
	return (ev_poll(&k->k_events, events, p));
}


static int kbd_iockeymap __P((struct kbd_state *ks,
	u_long cmd, struct kiockeymap *kio));

static int kbd_iocsled(struct kbd_softc *k, char *data);

#ifdef	KIOCGETKEY
static int kbd_oldkeymap __P((struct kbd_state *ks,
	u_long cmd, struct okiockey *okio));
#endif

int
kbdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct kbd_softc *k;
	struct kbd_state *ks;
	int error = 0;

	k = kbd_cd.cd_devs[minor(dev)];
	ks = &k->k_state;

	switch (cmd) {

	case KIOCTRANS: 	/* Set translation mode */
		/* We only support "raw" mode on /dev/kbd */
		if (*(int *)data != TR_UNTRANS_EVENT)
			error = EINVAL;
		break;

	case KIOCGTRANS:	/* Get translation mode */
		/* We only support "raw" mode on /dev/kbd */
		*(int *)data = TR_UNTRANS_EVENT;
		break;

#ifdef	KIOCGETKEY
	case KIOCGETKEY:	/* Get keymap entry (old format) */
		error = kbd_oldkeymap(ks, cmd, (struct okiockey *)data);
		break;
#endif	KIOCGETKEY */

	case KIOCSKEY:  	/* Set keymap entry */
		/* fallthrough */
	case KIOCGKEY:  	/* Get keymap entry */
		error = kbd_iockeymap(ks, cmd, (struct kiockeymap *)data);
		break;

	case KIOCCMD:	/* Send a command to the keyboard */
		error = kbd_docmd(*(int *)data, 1);
		break;

	case KIOCTYPE:	/* Get keyboard type */
		*(int *)data = ks->kbd_id;
		break;

	case KIOCSDIRECT:	/* where to send input */
		k->k_evmode = *(int *)data;
		break;

	case KIOCLAYOUT:	/* Get keyboard layout */
		*(int *)data = ks->kbd_layout;
		break;

	case KIOCSLED:
		error = kbd_iocsled(k, (char *)data);
		break;

	case KIOCGLED:
		*(char *)data = ks->kbd_leds;
		break;

	case FIONBIO:		/* we will remove this someday (soon???) */
		break;

	case FIOASYNC:
		k->k_events.ev_async = *(int *)data != 0;
		break;

	case TIOCSPGRP:
		if (*(int *)data != k->k_events.ev_io->p_pgid)
			error = EPERM;
		break;

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

/****************************************************************
 * ioctl helpers
 ****************************************************************/

/*
 * Get/Set keymap entry
 */
static int
kbd_iockeymap(ks, cmd, kio)
	struct kbd_state *ks;
	u_long cmd;
	struct kiockeymap *kio;
{
	u_short *km;
	u_int station;

	switch (kio->kio_tablemask) {
	case KIOC_NOMASK:
		km = ks->kbd_k.k_normal;
		break;
	case KIOC_SHIFTMASK:
		km = ks->kbd_k.k_shifted;
		break;
	case KIOC_CTRLMASK:
		km = ks->kbd_k.k_control;
		break;
	case KIOC_UPMASK:
		km = ks->kbd_k.k_release;
		break;
	default:
		/* Silently ignore unsupported masks */
		return (0);
	}

	/* Range-check the table position. */
	station = kio->kio_station;
	if (station >= KEYMAP_SIZE)
		return (EINVAL);

	switch (cmd) {

	case KIOCGKEY:	/* Get keymap entry */
		kio->kio_entry = km[station];
		break;

	case KIOCSKEY:	/* Set keymap entry */
		km[station] = kio->kio_entry;
		break;

	default:
		return(ENOTTY);
	}
	return (0);
}

#ifdef	KIOCGETKEY
/*
 * Get/Set keymap entry,
 * old format (compatibility)
 */
int
kbd_oldkeymap(ks, cmd, kio)
	struct kbd_state *ks;
	u_long cmd;
	struct okiockey *kio;
{
	int error = 0;

	switch (cmd) {

	case KIOCGETKEY:
		if (kio->kio_station == 118) {
			/*
			 * This is X11 asking if a type 3 keyboard is
			 * really a type 3 keyboard.  Say yes, it is,
			 * by reporting key station 118 as a "hole".
			 * Note old (SunOS 3.5) definition of HOLE!
			 */
			kio->kio_entry = 0xA2;
			break;
		}
		/* fall through */

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}
#endif	/* KIOCGETKEY */


/*
 * keyboard command ioctl
 * ``unimplemented commands are ignored'' (blech)
 * This is also export to the fb driver.
 */
int
kbd_docmd(cmd, isuser)
	int cmd;
	int isuser;
{
	struct kbd_softc *k;
	struct kbd_state *ks;
	int error, s;

	error = 0;
	k = kbd_cd.cd_devs[0];
	ks = &k->k_state;

	switch (cmd) {

	case KBD_CMD_BELL:
	case KBD_CMD_NOBELL:
		/* Supported by type 2, 3, and 4 keyboards */
		break;

	case KBD_CMD_CLICK:
	case KBD_CMD_NOCLICK:
		/* Unsupported by type 2 keyboards */
		if (ks->kbd_id <= KB_SUN2)
			return (0);
		ks->kbd_click = (cmd == KBD_CMD_CLICK);
		break;

	default:
		return (0);
	}

	s = spltty();

	if (isuser)
		error = kbd_drain_tx(k);

	if (error == 0) {
		kbd_output(k, cmd);
		kbd_start_tx(k);
	}

	splx(s);

	return (error);
}

/*
 * Set LEDs ioctl.
 */
static int
kbd_iocsled(k, data)
	struct kbd_softc *k;
	char *data;
{
	int leds, error, s;

	leds = *data;

	s = spltty();
	error = kbd_drain_tx(k);
	if (error == 0) {
		kbd_set_leds(k, leds);
	}
	splx(s);

	return (error);
}


/****************************************************************
 * middle layers:
 *  - keysym to ASCII sequence
 *  - raw key codes to keysym
 ****************************************************************/

static void kbd_input_string __P((struct kbd_softc *, char *));
static void kbd_input_funckey __P((struct kbd_softc *, int));
static int  kbd_input_keysym __P((struct kbd_softc *, int));

/*
 * Initialization done by either kdcninit or kbd_iopen
 */
void
kbd_xlate_init(ks)
	struct kbd_state *ks;
{
	struct keyboard *ktbls;
	int id;

	id = ks->kbd_id;
	if (id < KBD_MIN_TYPE)
		id = KBD_MIN_TYPE;
	if (id > kbd_max_type)
		id = kbd_max_type;
	ktbls = keyboards[id];

	ks->kbd_k = *ktbls; 	/* struct assignment */
	ks->kbd_modbits = 0;
}

/*
 * Turn keyboard up/down codes into a KEYSYM.
 * Note that the "kd" driver uses this too!
 */
int
kbd_code_to_keysym(ks, c)
	struct kbd_state *ks;
	int c;
{
	u_short *km;
	int keysym;

	/*
	 * Get keymap pointer.  One of these:
	 * release, control, shifted, normal, ...
	 */
	if (KEY_UP(c))
		km = ks->kbd_k.k_release;
	else if (ks->kbd_modbits & KBMOD_CTRL_MASK)
		km = ks->kbd_k.k_control;
	else if (ks->kbd_modbits & KBMOD_SHIFT_MASK)
		km = ks->kbd_k.k_shifted;
	else
		km = ks->kbd_k.k_normal;

	if (km == NULL) {
		/*
		 * Do not know how to translate yet.
		 * We will find out when a RESET comes along.
		 */
		return (KEYSYM_NOP);
	}
	keysym = km[KEY_CODE(c)];

	/*
	 * Post-processing for Caps-lock
	 */
	if ((ks->kbd_modbits & (1 << KBMOD_CAPSLOCK)) &&
		(KEYSYM_CLASS(keysym) == KEYSYM_ASCII) )
	{
		if (('a' <= keysym) && (keysym <= 'z'))
			keysym -= ('a' - 'A');
	}

	/*
	 * Post-processing for Num-lock.  All "function"
	 * keysyms get indirected through another table.
	 * (XXX: Only if numlock on.  Want off also!)
	 */
	if ((ks->kbd_modbits & (1 << KBMOD_NUMLOCK)) &&
		(KEYSYM_CLASS(keysym) == KEYSYM_FUNC) )
	{
		keysym = kbd_numlock_map[keysym & 0x3F];
	}

	return (keysym);
}

void
kbd_input_string(k, str)
	struct kbd_softc *k;
	char *str;
{

	while (*str) {
		(*k->k_cc->cc_upstream)(*str);
		str++;
	}
}

void
kbd_input_funckey(k, keysym)
	struct kbd_softc *k;
	int keysym;
{
	int n;
	char str[12];

	/*
	 * Format the F-key sequence and send as a string.
	 * XXX: Ugly compatibility mappings.
	 */
	n = 0xC0 + (keysym & 0x3F);
	sprintf(str, "\033[%dz", n);
	kbd_input_string(k, str);
}

/*
 * This is called by kbd_input_raw() or by kb_repeat()
 * to deliver ASCII input.  Called at spltty().
 *
 * Return zero on success, else the keysym that we
 * could not handle (so the caller may complain).
 */
int
kbd_input_keysym(k, keysym)
	struct kbd_softc *k;
	int keysym;
{
	struct kbd_state *ks = &k->k_state;
	int data;

	/* Check if a recipient has been configured */
	if (k->k_cc == NULL)
		return (0);

	switch (KEYSYM_CLASS(keysym)) {

	case KEYSYM_ASCII:
		data = KEYSYM_DATA(keysym);
		if (ks->kbd_modbits & KBMOD_META_MASK)
			data |= 0x80;
		(*k->k_cc->cc_upstream)(data);
		break;

	case KEYSYM_STRING:
		data = keysym & 0xF;
		kbd_input_string(k, kbd_stringtab[data]);
		break;

	case KEYSYM_FUNC:
		kbd_input_funckey(k, keysym);
		break;

	case KEYSYM_CLRMOD:
		data = 1 << (keysym & 0x1F);
		ks->kbd_modbits &= ~data;
		break;

	case KEYSYM_SETMOD:
		data = 1 << (keysym & 0x1F);
		ks->kbd_modbits |= data;
		break;

	case KEYSYM_INVMOD:
		data = 1 << (keysym & 0x1F);
		ks->kbd_modbits ^= data;
		kbd_update_leds(k);
		break;

	case KEYSYM_ALL_UP:
		ks->kbd_modbits &= ~0xFFFF;
		break;

	case KEYSYM_SPECIAL:
		if (keysym == KEYSYM_NOP)
			break;
		/* fall through */
	default:
		/* We could not handle it. */
		return (keysym);
	}
	return (0);
}

/*
 * This is the autorepeat timeout function.
 * Called at splsoftclock().
 */
static void
kbd_repeat(arg)
	void *arg;
{
	struct kbd_softc *k = (struct kbd_softc *)arg;
	int s = spltty();

	if (k->k_repeating && k->k_repeatsym >= 0) {
		(void)kbd_input_keysym(k, k->k_repeatsym);
		callout_reset(&k->k_repeat_ch, k->k_repeat_step,
		    kbd_repeat, k);
	}
	splx(s);
}

/*
 * Called by our kbd_softint() routine on input,
 * which passes the raw hardware scan codes.
 * Called at spltty()
 */
void
kbd_input_raw(k, c)
	struct kbd_softc *k;
	int c;
{
	struct kbd_state *ks = &k->k_state;
	struct firm_event *fe;
	int put, keysym;

	/* XXX - Input errors already handled. */

	/* Are we expecting special input? */
	if (ks->kbd_expect) {
		if (ks->kbd_expect & KBD_EXPECT_IDCODE) {
			/* We read a KBD_RESET last time. */
			ks->kbd_id = c;
			kbd_was_reset(k);
		}
		if (ks->kbd_expect & KBD_EXPECT_LAYOUT) {
			/* We read a KBD_LAYOUT last time. */
			ks->kbd_layout = c;
			kbd_new_layout(k);
		}
		ks->kbd_expect = 0;
		return;
	}

	/* Is this one of the "special" input codes? */
	if (KBD_SPECIAL(c)) {
		switch (c) {
		case KBD_RESET:
			ks->kbd_expect |= KBD_EXPECT_IDCODE;
			/* Fake an "all-up" to resync. translation. */
			c = KBD_IDLE;
			break;

		case KBD_LAYOUT:
			ks->kbd_expect |= KBD_EXPECT_LAYOUT;
			return;

		case KBD_ERROR:
			log(LOG_WARNING, "%s: received error indicator\n",
				k->k_dev.dv_xname);
			return;

		case KBD_IDLE:
			/* Let this go to the translator. */
			break;
		}
	}

	/*
	 * If /dev/kbd is not connected in event mode, 
	 * translate and send upstream (to console).
	 */
	if (!k->k_evmode && k->k_isconsole) {

		/* Any input stops auto-repeat (i.e. key release). */
		if (k->k_repeating) {
			k->k_repeating = 0;
			callout_stop(&k->k_repeat_ch);
		}

		/* Translate this code to a keysym */
		keysym = kbd_code_to_keysym(ks, c);

		/* Pass up to the next layer. */
		if (kbd_input_keysym(k, keysym)) {
			log(LOG_WARNING, "%s: code=0x%x with mod=0x%x"
				" produced unexpected keysym 0x%x\n",
				k->k_dev.dv_xname, c,
				ks->kbd_modbits, keysym);
			/* No point in auto-repeat here. */
			return;
		}

		/* Does this symbol get auto-repeat? */
		if (KEYSYM_NOREPEAT(keysym))
			return;

		/* Setup for auto-repeat after initial delay. */
		k->k_repeating = 1;
		k->k_repeatsym = keysym;
		callout_reset(&k->k_repeat_ch, k->k_repeat_start,
		    kbd_repeat, k);
		return;
	}

	/*
	 * IDLEs confuse the MIT X11R4 server badly, so we must drop them.
	 * This is bad as it means the server will not automatically resync
	 * on all-up IDLEs, but I did not drop them before, and the server
	 * goes crazy when it comes time to blank the screen....
	 */
	if (c == KBD_IDLE)
		return;

	/*
	 * Keyboard is generating events.  Turn this keystroke into an
	 * event and put it in the queue.  If the queue is full, the
	 * keystroke is lost (sorry!).
	 */
	put = k->k_events.ev_put;
	fe = &k->k_events.ev_q[put];
	put = (put + 1) % EV_QSIZE;
	if (put == k->k_events.ev_get) {
		log(LOG_WARNING, "%s: event queue overflow\n",
			k->k_dev.dv_xname); /* ??? */
		return;
	}
	fe->id = KEY_CODE(c);
	fe->value = KEY_UP(c) ? VKEY_UP : VKEY_DOWN;
	fe->time = time;
	k->k_events.ev_put = put;
	EV_WAKEUP(&k->k_events);
}

/****************************************************************/

/*
 * Open/close routines called upon opening /dev/console
 * if we serve console input.
 */
int
kbd_cc_open(cc)
	struct cons_channel *cc;
{
	struct kbd_softc *k = (struct kbd_softc *)cc->cc_dev;
	return (kbd_iopen(k));
}

int
kbd_cc_close(cc)
	struct cons_channel *cc;
{
	struct kbd_softc *k = (struct kbd_softc *)cc->cc_dev;
	return (kbd_iclose(k));
}

/*
 * Initialization to be done at first open.
 * This is called from kbdopen() or kd_cc_open()
 * Called with user context.
 */
int
kbd_iopen(k)
	struct kbd_softc *k;
{
	struct kbd_state *ks;
	int error, s;

	if (k == NULL)
		return (ENXIO);

	ks = &k->k_state;

	/* Tolerate extra calls. */
	if (k->k_isopen)
		return (0);

	s = spltty();

	/* Reset the keyboard and find out its type. */
	kbd_output(k, KBD_CMD_RESET);
	kbd_start_tx(k);
	kbd_drain_tx(k);
	/* The wakeup for this is in kbd_was_reset(). */
	error = tsleep((caddr_t)&ks->kbd_id,
				   PZERO | PCATCH, devopn, hz);
	if (error == EWOULDBLOCK) { 	/* no response */
		error = 0;
		log(LOG_ERR, "%s: reset failed\n",
			k->k_dev.dv_xname);
		/*
		 * Allow the open anyway (to keep getty happy)
		 * but assume the "least common denominator".
		 */
		ks->kbd_id = KB_SUN2;
	}

	/* Initialize the table pointers for this type. */
	kbd_xlate_init(ks);

	/* Earlier than type 4 does not know "layout". */
	if (ks->kbd_id < KB_SUN4)
		goto out;

	/* Ask for the layout. */
	kbd_output(k, KBD_CMD_GETLAYOUT);
	kbd_start_tx(k);
	kbd_drain_tx(k);
	/* The wakeup for this is in kbd_new_layout(). */
	error = tsleep((caddr_t)&ks->kbd_layout,
				   PZERO | PCATCH, devopn, hz);
	if (error == EWOULDBLOCK) { 	/* no response */
		error = 0;
		log(LOG_ERR, "%s: no response to get_layout\n",
			k->k_dev.dv_xname);
		ks->kbd_layout = 0;
	}

out:
	splx(s);

	if (error == 0)
		k->k_isopen = 1;

	return (error);
}

int
kbd_iclose(k)
	struct kbd_softc *k;
{
	/* For now: */ return (0);
}

/*
 * Called by kbd_input_raw, at spltty()
 */
static void
kbd_was_reset(k)
	struct kbd_softc *k;
{
	struct kbd_state *ks = &k->k_state;

	/*
	 * On first identification, wake up anyone waiting for type
	 * and set up the table pointers.
	 */
	wakeup((caddr_t)&ks->kbd_id);

	/* Restore keyclick, if necessary */
	switch (ks->kbd_id) {

	case KB_SUN2:
		/* Type 2 keyboards don't support keyclick */
		break;

	case KB_SUN3:
		/* Type 3 keyboards come up with keyclick on */
		if (!ks->kbd_click) {
			/* turn off the click */
			kbd_output(k, KBD_CMD_NOCLICK);
			kbd_start_tx(k);
		}
		break;

	case KB_SUN4:
		/* Type 4 keyboards come up with keyclick off */
		if (ks->kbd_click) {
			/* turn on the click */
			kbd_output(k, KBD_CMD_CLICK);
			kbd_start_tx(k);
		}
		break;
	}

	/* LEDs are off after reset. */
	ks->kbd_leds = 0;
}

/*
 * Called by kbd_input_raw, at spltty()
 */
static void
kbd_new_layout(k)
	struct kbd_softc *k;
{
	struct kbd_state *ks = &k->k_state;

	/*
	 * On first identification, wake up anyone waiting for type
	 * and set up the table pointers.
	 */
	wakeup((caddr_t)&ks->kbd_layout);

	/* XXX: switch decoding tables? */
}


/*
 * Wait for output to finish.
 * Called at spltty().  Has user context.
 */
static int
kbd_drain_tx(k)
	struct kbd_softc *k;
{
	int error;

	error = 0;

	while (k->k_txflags & K_TXBUSY) {
		k->k_txflags |= K_TXWANT;
		error = tsleep((caddr_t)&k->k_txflags,
					   PZERO | PCATCH, "kbdout", 0);
	}

	return (error);
}

/*
 * Enqueue some output for the keyboard
 * Called at spltty().
 */
void
kbd_output(k, c)
	struct kbd_softc *k;
	int c;	/* the data */
{
	int put;

	put = k->k_tbput;
	k->k_tbuf[put] = (u_char)c;
	put = (put + 1) & KBD_TX_RING_MASK;

	/* Would overrun if increment makes (put==get). */
	if (put == k->k_tbget) {
		log(LOG_WARNING, "%s: output overrun\n",
            k->k_dev.dv_xname);
	} else {
		/* OK, really increment. */
		k->k_tbput = put;
	}
}

/*
 * Start the sending data from the output queue
 * Called at spltty().
 */
void
kbd_start_tx(k)
	struct kbd_softc *k;
{
	int get;
	u_char c;

	if (k->k_txflags & K_TXBUSY)
		return;

	/* Is there anything to send? */
	get = k->k_tbget;
	if (get == k->k_tbput) {
		/* Nothing to send.  Wake drain waiters. */
		if (k->k_txflags & K_TXWANT) {
			k->k_txflags &= ~K_TXWANT;
			wakeup((caddr_t)&k->k_txflags);
		}
		return;
	}

	/* Have something to send. */
	c = k->k_tbuf[get];
	get = (get + 1) & KBD_TX_RING_MASK;
	k->k_tbget = get;
	k->k_txflags |= K_TXBUSY;

	k->k_write_data(k, c);
}

/*
 * Called at spltty by:
 * kbd_update_leds, kbd_iocsled
 */
static void
kbd_set_leds(k, new_leds)
	struct kbd_softc *k;
	int new_leds;
{
	struct kbd_state *ks = &k->k_state;

	/* Don't send unless state changes. */
	if (ks->kbd_leds == new_leds)
		return;

	ks->kbd_leds = new_leds;

	/* Only type 4 and later has LEDs anyway. */
	if (ks->kbd_id < KB_SUN4)
		return;

	kbd_output(k, KBD_CMD_SETLED);
	kbd_output(k, new_leds);
	kbd_start_tx(k);
}

/*
 * Called at spltty by:
 * kbd_input_keysym
 */
static void
kbd_update_leds(k)
    struct kbd_softc *k;
{
	struct kbd_state *ks = &k->k_state;
	char leds;

	leds = ks->kbd_leds;
	leds &= ~(LED_CAPS_LOCK|LED_NUM_LOCK);

	if (ks->kbd_modbits & (1 << KBMOD_CAPSLOCK))
		leds |= LED_CAPS_LOCK;
	if (ks->kbd_modbits & (1 << KBMOD_NUMLOCK))
		leds |= LED_NUM_LOCK;

	kbd_set_leds(k, leds);
}
