/*	$NetBSD: kbd.c,v 1.3 1996/10/11 00:39:33 christos Exp $	*/

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
 */

#include "ite.h"
#include "bell.h"

#if NITE > 0
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <x68k/dev/itevar.h>
#include <x68k/x68k/iodevice.h>

/* for sun-like event mode, if you go thru /dev/kbd. */
#include <x68k/dev/event_var.h>
#include <machine/kbio.h>
#include <machine/kbd.h>
#include <machine/vuid_event.h>

struct kbd_softc {
	int k_event_mode;  	 /* if true, collect events, else pass to ite */
	struct evvar k_events; /* event queue state */
} kbd_softc;

/*
 * Called from main() during pseudo-device setup.  If this keyboard is
 * the console, this is our chance to open the underlying serial port and
 * send a RESET, so that we can find out what kind of keyboard it is.
 */
void
kbdattach(kbd)
	int kbd;
{
}

/* definitions for x68k keyboard encoding. */
#define KEY_CODE(c)  ((c) & 0x7f)
#define KEY_UP(c)    ((c) & 0x80)

void
kbdenable()
{
	int s = spltty();

	sysport.keyctrl = 8;	/* send character from keyboard enable */
	mfp.iera &= (~0x11);	/* MPSC RBF, Timer-B interrupt disable */
	mfp.tbcr = MFP_TIMERB_RESET | MFP_TIMERB_STOP;    /* Timer-B stop */
	mfp.tbdr = 13;		/* Timer-B 38400 Hz clock (interrupt 76800Hz) */
	mfp.tbcr = 1;		/* Timer-B deray mode, prescale 1/4 */
	mfp.ucr = MFP_UCR_CLKX16 | MFP_UCR_RW_8 | MFP_UCR_ONESB;
	mfp.rsr = MFP_RSR_RE;	/* USART receive enable */
	mfp.iera |= 0x11;	/* MPSC RBF, Timer-B interrupt enable */

	while(!(mfp.tsr & MFP_TSR_BE)) ;
	mfp.udr = 0x49;     /* send character from keyboard enable */
	kbdled = 0;	    /* all keyboard LED turn off. */
	kbd_setLED();

	if (!(sysport.keyctrl & 8))
		kprintf("WARNING: no connected keyboard\n");
	kbd_softc.k_event_mode = 0;
	kbd_softc.k_events.ev_io = 0;
	splx(s);
}

int
kbdopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	if (kbd_softc.k_events.ev_io)
		return (EBUSY);
	kbd_softc.k_events.ev_io = p;
	ev_init(&kbd_softc.k_events);
	return (0);
}

int
kbdclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	/* Turn off event mode, dump the queue */
	kbd_softc.k_event_mode = 0;
	ev_fini(&kbd_softc.k_events);
	kbd_softc.k_events.ev_io = NULL;
	return (0);
}

int
kbdread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return ev_read(&kbd_softc.k_events, uio, flags);
}

/* this routine should not exist, but is convenient to write here for now */
int
kbdwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return EOPNOTSUPP;
}

int
kbdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct kbd_softc *k = &kbd_softc;
	int cmd_data;

	switch (cmd) {
	case KIOCTRANS:
		if (*(int *)data == TR_UNTRANS_EVENT)
			return (0);
		break;

	case KIOCGTRANS:
		/*
		 * Get translation mode
		 */
		*(int *)data = TR_UNTRANS_EVENT;
		return (0);

	case KIOCSDIRECT:
		k->k_event_mode = *(int *)data;
		return (0);

	case KIOCCMD:
		cmd_data = *(int *)data;
		return kbd_send_command(cmd_data);

	case KIOCSLED:
		kbdled = *(char *)data;
		kbd_setLED();
		return (0);

	case KIOCGLED:
		*(char *)data = kbdled;
		return (0);

	case KIOCSBELL:
#if NBELL > 0
		/* XXX - so tricky! */
		return opm_bell_setup((struct kbiocbell *)data);
#else
		return (0);	/* allways success */
#endif

	case FIONBIO:		/* we will remove this someday (soon???) */
		return (0);

	case FIOASYNC:
		k->k_events.ev_async = *(int *)data != 0;
		return (0);

	case TIOCSPGRP:
		if (*(int *)data != k->k_events.ev_io->p_pgid)
			return (EPERM);
		return (0);

	default:
		return (ENOTTY);
	}

	/*
	 * We identified the ioctl, but we do not handle it.
	 */
	return (EOPNOTSUPP);		/* misuse, but what the heck */
}

int
kbdselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	return ev_select (&kbd_softc.k_events, rw, p);
}

#define KBDBUFMASK 63
#define KBDBUFSIZ 64
static u_char kbdbuf[KBDBUFSIZ];
static int kbdputoff = 0;
static int kbdgetoff = 0;

void
kbdintr()
{
	u_char c, in;
	struct kbd_softc *k = &kbd_softc;
	struct firm_event *fe;
	int put;

	c = in = mfp.udr;

	/* if not in event mode, deliver straight to ite to process key stroke */
	if (! k->k_event_mode) {
		kbdbuf[kbdputoff++ & KBDBUFMASK] = c;
		setsoftkbd();
		return;
	}

	/* Keyboard is generating events.  Turn this keystroke into an
	   event and put it in the queue.  If the queue is full, the
	   keystroke is lost (sorry!). */
  
	put = k->k_events.ev_put;
	fe = &k->k_events.ev_q[put];
	put = (put + 1) % EV_QSIZE;
	if (put == k->k_events.ev_get) {
		log(LOG_WARNING, "keyboard event queue overflow\n"); /* ??? */
		return;
	}
	fe->id = KEY_CODE(c);
	fe->value = KEY_UP(c) ? VKEY_UP : VKEY_DOWN;
	fe->time = time;
	k->k_events.ev_put = put;
	EV_WAKEUP(&k->k_events);
}

void
kbdsoftint()
{
	int s = splhigh();

	while(kbdgetoff < kbdputoff)
		ite_filter(kbdbuf[kbdgetoff++ & KBDBUFMASK], ITEFILT_TTY);
	kbdgetoff = kbdputoff = 0;

	splx(s);
}

void
kbd_bell(mode)
	int mode;
{
#if NBELL > 0
	if (mode)
		opm_bell_on();
	else
		opm_bell_off();
#endif
}

int
kbdgetcn()
{
	int s = spltty();
	u_char ints, c, in;

	ints = mfp.iera;

	mfp.iera &= 0xef;
	mfp.rsr |= 1;
	while (!(mfp.rsr & MFP_RSR_BF))
		asm("nop");
	in = c = mfp.udr;

	mfp.iera = ints;
	splx (s);

	return c;
}

void
kbd_setLED()
{
	while(!(mfp.tsr & MFP_TSR_BE)) ;
        mfp.udr = ~kbdled | 0x80;
}

int
kbd_send_command(cmd)
	int cmd;
{
	switch (cmd) {
	case KBD_CMD_RESET:
		/* XXX */
		return 0;

	case KBD_CMD_BELL:
		kbd_bell(1);
		return 0;

	case KBD_CMD_NOBELL:
		kbd_bell(0);
		return 0;

	default:
		return ENOTTY;
	}
}

#endif
