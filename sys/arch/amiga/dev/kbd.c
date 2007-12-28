/*	$NetBSD: kbd.c,v 1.50 2007/12/28 20:49:49 joerg Exp $ */

/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	kbd.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kbd.c,v 1.50 2007/12/28 20:49:49 joerg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#ifdef DRACO
#include <m68k/asm_single.h>
#include <amiga/amiga/drcustom.h>
#endif
#include <amiga/amiga/cia.h>
#include <amiga/dev/itevar.h>
#include <amiga/dev/kbdreg.h>
#include <amiga/dev/kbdmap.h>
#include <amiga/dev/kbdvar.h>
#include <amiga/dev/event_var.h>
#include <amiga/dev/vuid_event.h>

#include "kbd.h"
#include "ite.h"

/* WSKBD */

/*
 * If NWSKBD>0 we try to attach an wskbd device to us. What follows
 * is definitions of callback functions and structures that are passed
 * to wscons when initializing.
 */

/*
 * Now with wscons this driver exhibits some weird behaviour.
 * It may act both as a driver of its own and the md part of the
 * wskbd driver. Therefore it can be accessed through /dev/kbd
 * and /dev/wskbd0 both.
 *
 * The data from they keyboard may end up in at least four different
 * places:
 * - If this driver has been opened (/dev/kbd) and the
 *   direct mode (TIOCDIRECT) has been set, data goes to
 *   the process who opened the device. Data will transmit itself
 *   as described by the firm_event structure.
 * - If wskbd support is compiled in and a wskbd driver has been
 *   attached then the data is sent to it. Wskbd in turn may
 *   - Send the data in the wscons_event form to a process that
 *     has opened /dev/wskbd0
 *   - Feed the data to a virtual terminal.
 * - If an ite is present the data may be fed to it.
 */

#include "wskbd.h"

#if NWSKBD>0
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <amiga/dev/wskbdmap_amiga.h>

/* accessops */
int     kbd_enable(void *, int);
void    kbd_set_leds(void *, int);
int     kbd_ioctl(void *, u_long, void *, int, struct lwp *);

/* console ops */
void    kbd_getc(void *, u_int *, int *);
void    kbd_pollc(void *, int);
void    kbd_bell(void *, u_int, u_int, u_int);

static struct wskbd_accessops kbd_accessops = {
	kbd_enable,
	kbd_set_leds,
	kbd_ioctl
};

static struct wskbd_consops kbd_consops = {
	kbd_getc,
	kbd_pollc,
	kbd_bell
};

/*
 * Pointer to keymaps. They are defined in wskbdmap_amiga.c.
 */
static struct wskbd_mapdata kbd_mapdata = {
	amigakbd_keydesctab,
	KB_US
};

#endif /* WSKBD */

struct kbd_softc {
	int k_event_mode;	/* if true, collect events, else pass to ite */
	struct evvar k_events;	/* event queue state */
#ifdef DRACO
	u_char k_rlprfx;	/* MF-II rel. prefix has been seen */
	u_char k_mf2;
#endif

	int k_console;		/* true if used as console keyboard */
#if NWSKBD>0
	struct device *k_wskbddev; /* pointer to wskbd for sending strokes */
	int k_pollingmode;         /* polling mode on? whatever it isss... */
#endif
};
struct kbd_softc kbd_softc;

int kbdmatch(struct device *, struct cfdata *, void *);
void kbdattach(struct device *, struct device *, void *);
void kbdintr(int);
void kbdstuffchar(u_char);

int drkbdgetc(void);
int drkbdrputc(u_int8_t);
int drkbdputc(u_int8_t);
int drkbdputc2(u_int8_t, u_int8_t);
int drkbdwaitfor(int);

CFATTACH_DECL(kbd, sizeof(struct device),
    kbdmatch, kbdattach, NULL, NULL);

dev_type_open(kbdopen);
dev_type_close(kbdclose);
dev_type_read(kbdread);
dev_type_ioctl(kbdioctl);
dev_type_poll(kbdpoll);
dev_type_kqfilter(kbdkqfilter);

const struct cdevsw kbd_cdevsw = {
	kbdopen, kbdclose, kbdread, nowrite, kbdioctl,
	nostop, notty, kbdpoll, nommap, kbdkqfilter,
};

/*ARGSUSED*/
int
kbdmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{

	if (matchname((char *)auxp, "kbd"))
		return(1);
	return(0);
}

/*ARGSUSED*/
void
kbdattach(struct device *pdp, struct device *dp, void *auxp)
{
#ifdef DRACO
	kbdenable();
	if (kbd_softc.k_mf2)
		printf(": QuickLogic type MF-II\n");
	else
		printf(": CIA A type Amiga\n");
#else
	printf(": CIA A type Amiga\n");
#endif

#if NWSKBD>0
	if (dp != NULL) {
		/*
		 * Try to attach the wskbd.
		 */
		struct wskbddev_attach_args waa;
		waa.console = kbd_softc.k_console;
		waa.keymap = &kbd_mapdata;
		waa.accessops = &kbd_accessops;
		waa.accesscookie = NULL;
		kbd_softc.k_wskbddev = config_found(dp, &waa, wskbddevprint);

		kbd_softc.k_pollingmode = 0;
	}
	kbdenable();
#endif /* WSKBD */
}

/*
 * This is called when somebody wants to use kbd as the console keyboard.
 */
void
kbd_cnattach(void)
{
#if NWSKBD>0
	wskbd_cnattach(&kbd_consops, NULL, &kbd_mapdata);
	kbd_softc.k_console = 1;
#endif
}

/* definitions for amiga keyboard encoding. */
#define KEY_CODE(c)  ((c) & 0x7f)
#define KEY_UP(c)    ((c) & 0x80)

#define DATLO single_inst_bclr_b(draco_ioct->io_control, DRCNTRL_KBDDATOUT)
#define DATHI single_inst_bset_b(draco_ioct->io_control, DRCNTRL_KBDDATOUT)

#define CLKLO single_inst_bclr_b(draco_ioct->io_control, DRCNTRL_KBDCLKOUT)
#define CLKHI single_inst_bset_b(draco_ioct->io_control, DRCNTRL_KBDCLKOUT)

void
kbdenable(void)
{
	static int kbd_inited = 0;

	int s;

#ifdef DRACO
	int id;
#endif
	/*
	 * collides with external ints from SCSI, watch out for this when
	 * enabling/disabling interrupts there !!
	 */
	s = splhigh();	/* don't lower; might be called from early ddb */
	if (kbd_inited) {
		splx(s);
		return;
	}
	kbd_inited = 1;
#ifdef DRACO
	if (is_draco()) {

		CLKLO;
		delay(5000);
		draco_ioct->io_kbdrst = 0;

		if (drkbdputc(0xf2))
			goto LnoMFII;

		id = drkbdgetc() << 8;
		id |= drkbdgetc();

		if (id != 0xab83)
			goto LnoMFII;

		if (drkbdputc2(0xf0, 3))	/* mode 3 */
			goto LnoMFII;

		if (drkbdputc(0xf8))		/* make/break, no typematic */
			goto LnoMFII;

		if (drkbdputc(0xf4))		/* enable */
			goto LnoMFII;
		kbd_softc.k_mf2 = 1;
		single_inst_bclr_b(draco_ioct->io_control, DRCNTRL_KBDINTENA);

		ciaa.icr = CIA_ICR_SP;  /* CIA SP interrupt disable */
		ciaa.cra &= ~(1<<6);	/* serial line == input */
		splx(s);
		return;

	LnoMFII:
		kbd_softc.k_mf2 = 0;
		single_inst_bset_b(*draco_intena, DRIRQ_INT2);
		ciaa.icr = CIA_ICR_IR_SC | CIA_ICR_SP;
					/* SP interrupt enable */
		ciaa.cra &= ~(1<<6);	/* serial line == input */
		splx(s);
		return;

	} else {
#endif
	custom.intena = INTF_SETCLR | INTF_PORTS;
	ciaa.icr = CIA_ICR_IR_SC | CIA_ICR_SP;  /* SP interrupt enable */
	ciaa.cra &= ~(1<<6);		/* serial line == input */
#ifdef DRACO
	}
#endif
	kbd_softc.k_event_mode = 0;
	kbd_softc.k_events.ev_io = 0;
	splx(s);
}

#ifdef DRACO
/*
 * call this with kbd interrupt blocked
 */

int
drkbdgetc(void)
{
	u_int8_t in;

	while ((draco_ioct->io_status & DRSTAT_KBDRECV) == 0);
	in = draco_ioct->io_kbddata;
	draco_ioct->io_kbdrst = 0;

	return in;
}

#define WAIT0 if (drkbdwaitfor(0)) goto Ltimeout
#define WAIT1 if (drkbdwaitfor(DRSTAT_KBDCLKIN)) goto Ltimeout

int
drkbdwaitfor(int bit)
{
	int i;



	i = 60000;	/* about 50 ms max */

	do {
		if ((draco_ioct->io_status & DRSTAT_KBDCLKIN) == bit)
			return 0;

	} while (--i >= 0);

	return 1;
}

/*
 * Output a raw byte to the keyboard (+ parity and stop bit).
 * return 0 on success, 1 on timeout.
 */
int
drkbdrputc(u_int8_t c)
{
	u_int8_t parity;
	int bitcnt;

	DATLO; CLKHI; WAIT1;
	parity = 0;

	for (bitcnt=7; bitcnt >= 0; bitcnt--) {
		WAIT0;
		if (c & 1) {
			DATHI;
		} else {
			++parity;
			DATLO;
		}
		c >>= 1;
		WAIT1;
	}
	WAIT0;
	/* parity bit */
	if (parity & 1) {
		DATLO;
	} else {
		DATHI;
	}
	WAIT1;
	/* stop bit */
	WAIT0; DATHI; WAIT1;

	WAIT0; /* XXX should check the ack bit here... */
	WAIT1;
	draco_ioct->io_kbdrst = 0;
	return 0;

Ltimeout:
	DATHI;
	draco_ioct->io_kbdrst = 0;
	return 1;
}

/*
 * Output one cooked byte to the keyboard, with wait for ACK or RESEND,
 * and retry if necessary. 0 == success, 1 == timeout
 */
int
drkbdputc(u_int8_t c)
{
	int rc;

	do {
		if (drkbdrputc(c))
			return(-1);

		rc = drkbdgetc();
	} while (rc == 0xfe);
	return (!(rc == 0xfa));
}

/*
 * same for twobyte sequence
 */

int
drkbdputc2(u_int8_t c1, u_int8_t c2)
{
	int rc;

	do {
		do {
			if (drkbdrputc(c1))
				return(-1);

			rc = drkbdgetc();
		} while (rc == 0xfe);
		if (rc != 0xfa)
			return (-1);

		if (drkbdrputc(c2))
			return(-1);

		rc = drkbdgetc();
	} while (rc == 0xfe);
	return (!(rc == 0xfa));
}
#endif

int
kbdopen(dev_t dev, int flags, int mode, struct lwp *l)
{

	kbdenable();
	if (kbd_softc.k_events.ev_io)
		return EBUSY;

	kbd_softc.k_events.ev_io = l->l_proc;
	ev_init(&kbd_softc.k_events);
	return (0);
}

int
kbdclose(dev_t dev, int flags, int mode, struct lwp *l)
{

	/* Turn off event mode, dump the queue */
	kbd_softc.k_event_mode = 0;
	ev_fini(&kbd_softc.k_events);
	kbd_softc.k_events.ev_io = NULL;
	return (0);
}

int
kbdread(dev_t dev, struct uio *uio, int flags)
{
	return ev_read (&kbd_softc.k_events, uio, flags);
}

int
kbdioctl(dev_t dev, u_long cmd, register void *data, int flag,
         struct lwp *l)
{
	register struct kbd_softc *k = &kbd_softc;

	switch (cmd) {
		case KIOCTRANS:
			if (*(int *)data == TR_UNTRANS_EVENT)
				return 0;
			break;

		case KIOCGTRANS:
			/* Get translation mode */
			*(int *)data = TR_UNTRANS_EVENT;
			return 0;

		case KIOCSDIRECT:
			k->k_event_mode = *(int *)data;
			return 0;

		case FIONBIO:	/* we will remove this someday (soon???) */
			return 0;

		case FIOASYNC:
			k->k_events.ev_async = *(int *)data != 0;
			return 0;

		case FIOSETOWN:
			if (-*(int *)data != k->k_events.ev_io->p_pgid
			    && *(int *)data != k->k_events.ev_io->p_pid)
				return EPERM;
			return 0;

		case TIOCSPGRP:
			if (*(int *)data != k->k_events.ev_io->p_pgid)
				return EPERM;
			return 0;

		default:
			return ENOTTY;
	}

	/* We identified the ioctl, but we do not handle it. */
	return EOPNOTSUPP;	/* misuse, but what the heck */
}

int
kbdpoll(dev_t dev, int events, struct lwp *l)
{
	return ev_poll (&kbd_softc.k_events, events, l);
}

int
kbdkqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{

	return (ev_kqfilter(&kbd_softc.k_events, kn));
}

void
kbdintr(int mask)
{
	u_char c;
#ifdef KBDRESET
	static int reset_warn;
#endif

	/*
	 * now only invoked from generic CIA interrupt handler if there *is*
	 * a keyboard interrupt pending
	 */

	c = ~ciaa.sdr;	/* keyboard data is inverted */
	/* ack */
	ciaa.cra |= (1 << 6);	/* serial line output */
#ifdef KBDRESET
	if (reset_warn && c == 0xf0) {
#ifdef DEBUG
		printf ("kbdintr: !!!! Reset Warning !!!!\n");
#endif
		bootsync();
		reset_warn = 0;
		DELAY(30000000);
	}
#endif
	/* wait 200 microseconds (for bloody Cherry keyboards..) */
	DELAY(2000);			/* fudge delay a bit for some keyboards */
	ciaa.cra &= ~(1 << 6);

	/* process the character */
	c = (c >> 1) | (c << 7);	/* rotate right once */

#ifdef KBDRESET
	if (c == 0x78) {
#ifdef DEBUG
		printf ("kbdintr: Reset Warning started\n");
#endif
		++reset_warn;
		return;
	}
#endif
	kbdstuffchar(c);
}

#ifdef DRACO
/* maps MF-II keycodes to Amiga keycodes */

const u_char drkbdtab[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x50,
	0x45, 0xff, 0xff, 0xff, 0xff, 0x42, 0x00, 0x51,

	0xff, 0x64, 0x60, 0x30, 0x63, 0x10, 0x01, 0x52,
	0xff, 0x66, 0x31, 0x21, 0x20, 0x11, 0x02, 0x53,

	0xff, 0x33, 0x32, 0x22, 0x12, 0x04, 0x03, 0x54,
	0xff, 0x40, 0x34, 0x23, 0x14, 0x13, 0x05, 0x55,

	0xff, 0x36, 0x35, 0x25, 0x24, 0x15, 0x06, 0x56,
	0xff, 0x67, 0x37, 0x26, 0x16, 0x07, 0x08, 0x57,
	/* --- */
	0xff, 0x38, 0x27, 0x17, 0x18, 0x0a, 0x09, 0x58,
	0xff, 0x39, 0x3a, 0x28, 0x29, 0x19, 0x0b, 0x59,

	0xff, 0xff, 0x2a, 0x2b, 0x1a, 0x0c, 0x4b, 0xff,
	0x65, 0x61, 0x44, 0x1b, 0xff, 0xff, 0x6f, 0xff,

	0x4d, 0x4f, 0xff, 0x4c, 0x0d, 0xff, 0x41, 0x46,
	0xff, 0x1d, 0x4e, 0x2d, 0x3d, 0x4a, 0x5f, 0x62,

	0x0f, 0x3c, 0x1e, 0x2e, 0x2f, 0x3e, 0x5a, 0x5b,
	0xff, 0x43, 0x1f, 0xff, 0x5e, 0x3f, 0x5c, 0xff,
	/* --- */
	0xff, 0xff, 0xff, 0xff, 0x5d
};
#endif


int
kbdgetcn(void)
{
	int s;
	u_char ints, mask, c, in;

#ifdef DRACO
	if (is_draco() && kbd_softc.k_mf2) {
		do {
			c = 0;
			s = spltty ();
			while ((draco_ioct->io_status & DRSTAT_KBDRECV) == 0);
			in = draco_ioct->io_kbddata;
			draco_ioct->io_kbdrst = 0;
			if (in == 0xF0) { /* release prefix */
				c = 0x80;
				while ((draco_ioct->io_status &
				    DRSTAT_KBDRECV) == 0);
				in = draco_ioct->io_kbddata;
				draco_ioct->io_kbdrst = 0;
			}
			splx(s);
#ifdef DRACORAWKEYDEBUG
			printf("<%02x>", in);
#endif
			c |= in>=sizeof(drkbdtab) ? 0xff : drkbdtab[in];
		} while (c == 0xff);
		return (c);
	}
#endif
	s = spltty();
	for (ints = 0; ! ((mask = ciaa.icr) & CIA_ICR_SP);
	    ints |= mask) ;

	in = ciaa.sdr;
	c = ~in;

	/* ack */
	ciaa.cra |= (1 << 6);	/* serial line output */
	ciaa.sdr = 0xff;	/* ack */
	/* wait 200 microseconds */
	DELAY(2000);	/* XXXX only works as long as DELAY doesn't
			 * use a timer and waits.. */
	ciaa.cra &= ~(1 << 6);
	ciaa.sdr = in;

	splx (s);
	c = (c >> 1) | (c << 7);

	/* take care that no CIA-interrupts are lost */
	if (ints)
		dispatch_cia_ints (0, ints);

	return c;
}

void
kbdstuffchar(u_char c)
{
	struct firm_event *fe;
	struct kbd_softc *k = &kbd_softc;
	int put;

#if NWSKBD>0
	/*
	 * If we have attached a wskbd and not in polling mode and
	 * nobody has opened us directly, then send the keystroke
	 * to the wskbd.
	 */

	if (kbd_softc.k_pollingmode == 0
	    && kbd_softc.k_wskbddev != NULL
	    && k->k_event_mode == 0) {
		wskbd_input(kbd_softc.k_wskbddev,
			    KEY_UP(c) ?
			    WSCONS_EVENT_KEY_UP :
			    WSCONS_EVENT_KEY_DOWN,
			    KEY_CODE(c));
		return;
	}

#endif /* NWSKBD */

	/*
	 * If not in event mode, deliver straight to ite to process
	 * key stroke
	 */

	if (! k->k_event_mode) {
#if NITE>0
		ite_filter (c, ITEFILT_TTY);
#endif
		return;
	}

	/*
	 * Keyboard is generating events. Turn this keystroke into an
	 * event and put it in the queue. If the queue is full, the
	 * keystroke is lost (sorry!).
	 */

	put = k->k_events.ev_put;
	fe = &k->k_events.ev_q[put];
	put = (put + 1) % EV_QSIZE;
	if (put == k->k_events.ev_get) {
		log(LOG_WARNING, "keyboard event queue overflow\n");
			/* ??? */
		return;
	}
	fe->id = KEY_CODE(c);
	fe->value = KEY_UP(c) ? VKEY_UP : VKEY_DOWN;
	getmicrotime(&fe->time);
	k->k_events.ev_put = put;
	EV_WAKEUP(&k->k_events);
}


#ifdef DRACO
void
drkbdintr(void)
{
	u_char in;
	struct kbd_softc *k = &kbd_softc;

	in = draco_ioct->io_kbddata;
	draco_ioct->io_kbdrst = 0;

	if (in == 0xF0)
		k->k_rlprfx = 0x80;
	else {
		kbdstuffchar(in>=sizeof(drkbdtab) ? 0xff :
		    drkbdtab[in] | k->k_rlprfx);
		k->k_rlprfx = 0;
	}
}

#endif


#if NWSKBD>0
/*
 * These are the callback functions that are passed to wscons.
 * They really don't do anything worth noting, just call the
 * other functions above.
 */

int
kbd_enable(void *c, int on)
{
	/* Wonder what this is supposed to do... */
	return (0);
}

void
kbd_set_leds(void *c, int leds)
{
}

int
kbd_ioctl(void *c, u_long cmd, void *data, int flag, struct lwp *l)
{
	switch (cmd)
	{
	case WSKBDIO_COMPLEXBELL:
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int*)data = 0;
		return 0;
	case WSKBDIO_GTYPE:
		*(u_int*)data = WSKBD_TYPE_AMIGA;
		return 0;
	}

	/*
	 * We are supposed to return EPASSTHROUGH to wscons if we didn't
	 * understand.
	 */
	return (EPASSTHROUGH);
}

void
kbd_getc(void *c, u_int *type, int *data)
{
	int key;

	key = kbdgetcn();

	*data = KEY_CODE(key);
	*type = KEY_UP(key) ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
}

void
kbd_pollc(void *c, int on)
{
	kbd_softc.k_pollingmode = on;
}

void
kbd_bell(void *c, u_int x, u_int y, u_int z)
{
}
#endif /* WSKBD */
