/*	$NetBSD: kbd.c,v 1.8.14.1 1999/12/27 18:34:16 wrstuden Exp $	*/

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
#include <sys/signalvar.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/mfp.h>
#include <arch/x68k/dev/itevar.h>

/* for sun-like event mode, if you go thru /dev/kbd. */
#include <arch/x68k/dev/event_var.h>

#include <machine/kbio.h>
#include <machine/kbd.h>
#include <machine/vuid_event.h>

struct kbd_softc {
	struct device	sc_dev;

	int sc_event_mode;	/* if true, collect events, else pass to ite */
	struct evvar sc_events; /* event queue state */
};

void	kbdenable	__P((int));
int	kbdopen 	__P((dev_t, int, int, struct proc *));
int	kbdclose	__P((dev_t, int, int, struct proc *));
int	kbdread 	__P((dev_t, struct uio *, int));
int	kbdwrite	__P((dev_t, struct uio *, int));
int	kbdioctl	__P((dev_t, u_long, caddr_t, int, struct proc *));
int	kbdpoll 	__P((dev_t, int, struct proc *));
int	kbdintr 	__P((void *));
void	kbdsoftint	__P((void));
void	kbd_bell	__P((int));
int	kbdcngetc	__P((void));
void	kbd_setLED	__P((void));
int	kbd_send_command __P((int));


static int kbdmatch	__P((struct device *, struct cfdata *, void *));
static void kbdattach	__P((struct device *, struct device *, void *));

struct cfattach kbd_ca = {
	sizeof(struct kbd_softc), kbdmatch, kbdattach
};


static int
kbdmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (strcmp(aux, "kbd") != 0)
		return (0);
	if (cf->cf_unit != 0)
		return (0);

	return (1);
}

static void
kbdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct kbd_softc *k = (void*) self;
	struct mfp_softc *mfp = (void*) parent;
	int s = spltty();

	/* MFP interrupt #12 is for USART recieve buffer full */
	intio_intr_establish(mfp->sc_intr + 12, "kbd", kbdintr, self);

	kbdenable(1);
	k->sc_event_mode = 0;
	k->sc_events.ev_io = 0;
	splx(s);

	printf("\n");
}


/* definitions for x68k keyboard encoding. */
#define KEY_CODE(c)  ((c) & 0x7f)
#define KEY_UP(c)    ((c) & 0x80)

void
kbdenable(mode)
	int mode;		/* 1: interrupt, 0: poll */
{
	intio_set_sysport_keyctrl(8);
	mfp_bit_clear_iera(MFP_INTR_RCV_FULL | MFP_INTR_TIMER_B);
	mfp_set_tbcr(MFP_TIMERB_RESET | MFP_TIMERB_STOP);
	mfp_set_tbdr(13);	/* Timer B interrupt interval */
	mfp_set_tbcr(1);	/* 1/4 delay mode */
	mfp_set_ucr(MFP_UCR_CLKX16 | MFP_UCR_RW_8 | MFP_UCR_ONESB);
	mfp_set_rsr(MFP_RSR_RE); /* USART receive enable */
	mfp_set_tsr(MFP_TSR_TE); /* USART transmit enable */

	if (mode) {
		mfp_bit_set_iera(MFP_INTR_RCV_FULL);
		/*
		 * Perform null read in case that an input byte is in the
		 * receiver buffer, which prevents further interrupts.
		 * We could save the input, but probably not so valuable.
		 */
		(void) mfp_get_udr();
	}

	kbdled = 0;		/* all keyboard LED turn off. */
	kbd_setLED();

	if (!(intio_get_sysport_keyctrl() & 8))
		printf(" (no connected keyboard)");
}

extern struct cfdriver kbd_cd;

int
kbdopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct kbd_softc *k;
	int unit = minor(dev);

	if (unit >= kbd_cd.cd_ndevs)
		return (ENXIO);
	k = kbd_cd.cd_devs[minor(dev)];
	if (k == NULL)
		return (ENXIO);

	if (k->sc_events.ev_io)
		return (EBUSY);
	k->sc_events.ev_io = p;
	ev_init(&k->sc_events);

	return (0);
}

int
kbdclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct kbd_softc *k = kbd_cd.cd_devs[minor(dev)];

	/* Turn off event mode, dump the queue */
	k->sc_event_mode = 0;
	ev_fini(&k->sc_events);
	k->sc_events.ev_io = NULL;

	return (0);
}


int
kbdread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct kbd_softc *k = kbd_cd.cd_devs[minor(dev)];

	return ev_read(&k->sc_events, uio, flags);
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

#if NBELL > 0
struct bell_info;
int opm_bell_setup __P((struct bell_info *));
void opm_bell_on __P((void));
void opm_bell_off __P((void));
#endif

int
kbdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct kbd_softc *k = kbd_cd.cd_devs[minor(dev)];
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
		k->sc_event_mode = *(int *)data;
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
		return opm_bell_setup((struct bell_info *)data);
#else
		return (0);	/* allways success */
#endif

	case FIONBIO:		/* we will remove this someday (soon???) */
		return (0);

	case FIOASYNC:
		k->sc_events.ev_async = *(int *)data != 0;
		return (0);

	case TIOCSPGRP:
		if (*(int *)data != k->sc_events.ev_io->p_pgid)
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
kbdpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct kbd_softc *k;

	k = kbd_cd.cd_devs[minor(dev)];
	return (ev_poll(&k->sc_events, events, p));
}


#define KBDBUFMASK 63
#define KBDBUFSIZ 64
static u_char kbdbuf[KBDBUFSIZ];
static int kbdputoff = 0;
static int kbdgetoff = 0;

int
kbdintr(arg)
	void *arg;
{
	u_char c, in;
	struct kbd_softc *k = arg; /* XXX */
	struct firm_event *fe;
	int put;

	c = in = mfp_get_udr();

	/* if not in event mode, deliver straight to ite to process key stroke */
	if (! k->sc_event_mode) {
		kbdbuf[kbdputoff++ & KBDBUFMASK] = c;
		setsoftkbd();
		return 0;
	}

	/* Keyboard is generating events.  Turn this keystroke into an
	   event and put it in the queue.  If the queue is full, the
	   keystroke is lost (sorry!). */

	put = k->sc_events.ev_put;
	fe = &k->sc_events.ev_q[put];
	put = (put + 1) % EV_QSIZE;
	if (put == k->sc_events.ev_get) {
		log(LOG_WARNING, "keyboard event queue overflow\n"); /* ??? */
		return 0;
	}
	fe->id = KEY_CODE(c);
	fe->value = KEY_UP(c) ? VKEY_UP : VKEY_DOWN;
	fe->time = time;
	k->sc_events.ev_put = put;
	EV_WAKEUP(&k->sc_events);

	return 0;
}

void
kbdsoftint()			/* what if ite is not configured? */
{
	int s = spltty();

	while(kbdgetoff < kbdputoff)
		ite_filter(kbdbuf[kbdgetoff++ & KBDBUFMASK]);
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

unsigned char kbdled;
void
kbd_setLED()
{
        mfp_send_usart(~kbdled | 0x80);
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

/*
 * for console
 */
#include "ite.h"
#if NITE > 0
int
kbdcngetc()
{
	int s;
	u_char ints, c;

	s = splhigh();
	ints = mfp_get_iera();

	mfp_bit_clear_iera(MFP_INTR_RCV_FULL);
	mfp_set_rsr(mfp_get_rsr() | MFP_RSR_RE);
	c = mfp_recieve_usart();

	mfp_set_iera(ints);
	splx(s);

	return c;
}
#endif
