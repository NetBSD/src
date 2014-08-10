/*	$NetBSD: kbd.c,v 1.39.2.1 2014/08/10 06:54:10 tls Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kbd.c,v 1.39.2.1 2014/08/10 06:54:10 tls Exp $");

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
#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/mutex.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/mfp.h>
#include <arch/x68k/dev/itevar.h>

/* for sun-like event mode, if you go thru /dev/kbd. */
#include <arch/x68k/dev/event_var.h>

#include <machine/kbio.h>
#include <machine/kbd.h>
#include <machine/vuid_event.h>

struct kbd_softc {
	device_t sc_dev;
	int sc_event_mode;	/* if true, collect events, else pass to ite */
	struct evvar sc_events; /* event queue state */
	void *sc_softintr_cookie;
	kmutex_t sc_lock;
};

void	kbdenable(int);
int	kbdintr(void *);
void	kbdsoftint(void *);
void	kbd_bell(int);
int	kbdcngetc(void);
void	kbd_setLED(void);
int	kbd_send_command(int);


static int kbdmatch(device_t, cfdata_t, void *);
static void kbdattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(kbd, sizeof(struct kbd_softc),
    kbdmatch, kbdattach, NULL, NULL);

static int kbd_attached;

dev_type_open(kbdopen);
dev_type_close(kbdclose);
dev_type_read(kbdread);
dev_type_ioctl(kbdioctl);
dev_type_poll(kbdpoll);
dev_type_kqfilter(kbdkqfilter);

const struct cdevsw kbd_cdevsw = {
	.d_open = kbdopen,
	.d_close = kbdclose,
	.d_read = kbdread,
	.d_write = nowrite,
	.d_ioctl = kbdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = kbdpoll,
	.d_mmap = nommap,
	.d_kqfilter = kbdkqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static int
kbdmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(aux, "kbd") != 0)
		return (0);
	if (kbd_attached)
		return (0);

	return (1);
}

static void
kbdattach(device_t parent, device_t self, void *aux)
{
	struct kbd_softc *sc = device_private(self);
	struct mfp_softc *mfp = device_private(parent);

	kbd_attached = 1;
	sc->sc_dev = self;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	/* MFP interrupt #12 is for USART receive buffer full */
	intio_intr_establish(mfp->sc_intr + 12, "kbd", kbdintr, sc);
	sc->sc_softintr_cookie = softint_establish(SOFTINT_SERIAL,
	    kbdsoftint, sc);

	kbdenable(1);
	sc->sc_event_mode = 0;
	sc->sc_events.ev_io = 0;

	aprint_normal("\n");
}


/* definitions for x68k keyboard encoding. */
#define KEY_CODE(c)  ((c) & 0x7f)
#define KEY_UP(c)    ((c) & 0x80)

void
kbdenable(int mode)	/* 1: interrupt, 0: poll */
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
		aprint_normal(" (no connected keyboard)");
}

extern struct cfdriver kbd_cd;

int
kbdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct kbd_softc *k;

	k = device_lookup_private(&kbd_cd, minor(dev));
	if (k == NULL)
		return (ENXIO);

	if (k->sc_events.ev_io)
		return (EBUSY);
	k->sc_events.ev_io = l->l_proc;
	ev_init(&k->sc_events, device_xname(k->sc_dev), &k->sc_lock);

	return (0);
}

int
kbdclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct kbd_softc *k = device_lookup_private(&kbd_cd, minor(dev));

	/* Turn off event mode, dump the queue */
	k->sc_event_mode = 0;
	ev_fini(&k->sc_events);
	k->sc_events.ev_io = NULL;

	return (0);
}


int
kbdread(dev_t dev, struct uio *uio, int flags)
{
	struct kbd_softc *k = device_lookup_private(&kbd_cd, minor(dev));

	return ev_read(&k->sc_events, uio, flags);
}

#if NBELL > 0
struct bell_info;
int opm_bell_setup(struct bell_info *);
void opm_bell_on(void);
void opm_bell_off(void);
#endif

int
kbdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct kbd_softc *k = device_lookup_private(&kbd_cd, minor(dev));
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
		return (0);	/* always success */
#endif

	case FIONBIO:		/* we will remove this someday (soon???) */
		return (0);

	case FIOASYNC:
		k->sc_events.ev_async = *(int *)data != 0;
		return (0);

	case FIOSETOWN:
		if (-*(int *)data != k->sc_events.ev_io->p_pgid
		    && *(int *)data != k->sc_events.ev_io->p_pid)
			return (EPERM);
		return 0;

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
kbdpoll(dev_t dev, int events, struct lwp *l)
{
	struct kbd_softc *k;

	k = device_lookup_private(&kbd_cd, minor(dev));
	return (ev_poll(&k->sc_events, events, l));
}

int
kbdkqfilter(dev_t dev, struct knote *kn)
{
	struct kbd_softc *k;

	k = device_lookup_private(&kbd_cd, minor(dev));
	return (ev_kqfilter(&k->sc_events, kn));
}

#define KBDBUFMASK 63
#define KBDBUFSIZ 64
static u_char kbdbuf[KBDBUFSIZ];
static int kbdputoff = 0;
static int kbdgetoff = 0;

int
kbdintr(void *arg)
{
	uint8_t c, st;
	struct kbd_softc *sc = arg;
	struct firm_event *fe;
	int put;

	/* clear receiver error if any */
	st = mfp_get_rsr();

	c = mfp_get_udr();

	if ((st & MFP_RSR_BF) == 0)
		return 0;	/* intr caused by an err -- no char received */

	/* if not in event mode, deliver straight to ite to process key stroke */
	if (!sc->sc_event_mode) {
		kbdbuf[kbdputoff++ & KBDBUFMASK] = c;
		softint_schedule(sc->sc_softintr_cookie);
		return 0;
	}

	/* Keyboard is generating events.  Turn this keystroke into an
	   event and put it in the queue.  If the queue is full, the
	   keystroke is lost (sorry!). */

	put = sc->sc_events.ev_put;
	fe = &sc->sc_events.ev_q[put];
	put = (put + 1) % EV_QSIZE;
	if (put == sc->sc_events.ev_get) {
		log(LOG_WARNING, "keyboard event queue overflow\n"); /* ??? */
		return 0;
	}
	fe->id = KEY_CODE(c);
	fe->value = KEY_UP(c) ? VKEY_UP : VKEY_DOWN;
	firm_gettime(fe);
	sc->sc_events.ev_put = put;
	softint_schedule(sc->sc_softintr_cookie);

	return 0;
}

void
kbdsoftint(void *arg)			/* what if ite is not configured? */
{
	struct kbd_softc *sc = arg;

	if (sc->sc_event_mode)
		ev_wakeup(&sc->sc_events);

	mutex_enter(&sc->sc_lock);
	while (kbdgetoff < kbdputoff) {
		mutex_exit(&sc->sc_lock);
		ite_filter(kbdbuf[kbdgetoff++ & KBDBUFMASK]);
		mutex_enter(&sc->sc_lock);
	}
	kbdgetoff = kbdputoff = 0;

	mutex_exit(&sc->sc_lock);
}

void
kbd_bell(int mode)
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
kbd_setLED(void)
{
	mfp_send_usart(~kbdled | 0x80);
}

int
kbd_send_command(int cmd)
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
#if NITE > 0
int
kbdcngetc(void)
{
	int s;
	u_char ints, c;

	s = splhigh();
	ints = mfp_get_iera();

	mfp_bit_clear_iera(MFP_INTR_RCV_FULL);
	mfp_set_rsr(mfp_get_rsr() | MFP_RSR_RE);
	c = mfp_receive_usart();

	mfp_set_iera(ints);
	splx(s);

	return c;
}
#endif
