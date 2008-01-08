/*	$NetBSD: kbd.c,v 1.30.32.1 2008/01/08 22:09:35 bouyer Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman
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
__KERNEL_RCSID(0, "$NetBSD: kbd.c,v 1.30.32.1 2008/01/08 22:09:35 bouyer Exp $");

#include "mouse.h"
#include "ite.h"
#include "wskbd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/syslog.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/acia.h>
#include <atari/dev/itevar.h>
#include <atari/dev/event_var.h>
#include <atari/dev/vuid_event.h>
#include <atari/dev/ym2149reg.h>
#include <atari/dev/kbdreg.h>
#include <atari/dev/kbdvar.h>
#include <atari/dev/kbdmap.h>
#include <atari/dev/msvar.h>

#if NWSKBD>0
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <atari/dev/wskbdmap_atari.h>
#endif

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

u_char			kbd_modifier;	/* Modifier mask		*/

static u_char		kbd_ring[KBD_RING_SIZE];
static volatile u_int	kbd_rbput = 0;	/* 'put' index			*/
static u_int		kbd_rbget = 0;	/* 'get' index			*/
static u_char		kbd_soft  = 0;	/* 1: Softint has been scheduled*/

static struct kbd_softc kbd_softc;

/* {b,c}devsw[] function prototypes */
dev_type_open(kbdopen);
dev_type_close(kbdclose);
dev_type_read(kbdread);
dev_type_ioctl(kbdioctl);
dev_type_poll(kbdpoll);
dev_type_kqfilter(kbdkqfilter);

/* Interrupt handler */
void	kbdintr __P((int));

static void kbdsoft __P((void *, void *));
static void kbdattach __P((struct device *, struct device *, void *));
static int  kbdmatch __P((struct device *, struct cfdata *, void *));
#if NITE>0
static int  kbd_do_modifier __P((u_char));
#endif
static int  kbd_write_poll __P((u_char *, int));
static void kbd_pkg_start __P((struct kbd_softc *, u_char));

CFATTACH_DECL(kbd, sizeof(struct device),
    kbdmatch, kbdattach, NULL, NULL);

const struct cdevsw kbd_cdevsw = {
	kbdopen, kbdclose, kbdread, nowrite, kbdioctl,
	nostop, notty, kbdpoll, nommap, kbdkqfilter,
};

#if NWSKBD>0
/* accessops */
static int	kbd_enable(void *, int);
static void	kbd_set_leds(void *, int);
static int	kbd_ioctl(void *, u_long, void *, int, struct lwp *);

/* console ops */
static void	kbd_getc(void *, u_int *, int *);
static void	kbd_pollc(void *, int);
static void	kbd_bell(void *, u_int, u_int, u_int);

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

/* Pointer to keymaps. */
static struct wskbd_mapdata kbd_mapdata = {
        atarikbd_keydesctab,
	KB_US
};
#endif /* WSKBD */

/*ARGSUSED*/
static	int
kbdmatch(pdp, cfp, auxp)
struct	device	*pdp;
struct	cfdata	*cfp;
void		*auxp;
{
	if (!strcmp((char *)auxp, "kbd"))
		return (1);
	return (0);
}

/*ARGSUSED*/
static void
kbdattach(pdp, dp, auxp)
struct	device *pdp, *dp;
void	*auxp;
{
	int	timeout;
	u_char	kbd_rst[]  = { 0x80, 0x01 };
	u_char	kbd_icmd[] = { 0x12, 0x15 };

	/*
	 * Disable keyboard interrupts from MFP
	 */
	MFP->mf_ierb &= ~IB_AINT;

	/*
	 * Reset ACIA and intialize to:
	 *    divide by 16, 8 data, 1 stop, no parity, enable RX interrupts
	 */
	KBD->ac_cs = A_RESET;
	delay(100);	/* XXX: enough? */
	KBD->ac_cs = kbd_softc.k_soft_cs = KBD_INIT | A_RXINT;

	/*
	 * Clear error conditions
	 */
	while (KBD->ac_cs & (A_IRQ|A_RXRDY))
		timeout = KBD->ac_da;

	/*
	 * Now send the reset string, and read+ignore it's response
	 */
	if (!kbd_write_poll(kbd_rst, 2))
		printf("kbd: error cannot reset keyboard\n");
	for (timeout = 1000; timeout > 0; timeout--) {
		if (KBD->ac_cs & (A_IRQ|A_RXRDY)) {
			timeout = KBD->ac_da;
			timeout = 100;
		}
		delay(100);
	}
	/*
	 * Send init command: disable mice & joysticks
	 */
	kbd_write_poll(kbd_icmd, sizeof(kbd_icmd));

	printf("\n");

#if NWSKBD>0
	if (dp != NULL) {
		/*
		 * Try to attach the wskbd.
		 */
		struct wskbddev_attach_args waa;

		/* Maybe should be done before this?... */
		wskbd_cnattach(&kbd_consops, NULL, &kbd_mapdata);

		waa.console = 1;
		waa.keymap = &kbd_mapdata;
		waa.accessops = &kbd_accessops;
		waa.accesscookie = NULL;
		kbd_softc.k_wskbddev = config_found(dp, &waa, wskbddevprint);

		kbd_softc.k_pollingmode = 0;

		kbdenable();
	}
#endif /* WSKBD */
}

void
kbdenable()
{
	int	s, code;

	s = spltty();

	/*
	 * Clear error conditions...
	 */
	while (KBD->ac_cs & (A_IRQ|A_RXRDY))
		code = KBD->ac_da;
	/*
	 * Enable interrupts from MFP
	 */
	MFP->mf_iprb  = (u_int8_t)~IB_AINT;
	MFP->mf_ierb |= IB_AINT;
	MFP->mf_imrb |= IB_AINT;

	kbd_softc.k_event_mode   = 0;
	kbd_softc.k_events.ev_io = 0;
	kbd_softc.k_pkg_size     = 0;
	splx(s);
}

int kbdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
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
	return ev_read(&kbd_softc.k_events, uio, flags);
}

int
kbdioctl(dev_t dev,u_long cmd,register void *data,int flag,struct lwp *l)
{
	register struct kbd_softc *k = &kbd_softc;
	struct kbdbell	*kb;

	switch (cmd) {
		case KIOCTRANS:
			if (*(int *)data == TR_UNTRANS_EVENT)
				return 0;
			break;

		case KIOCGTRANS:
			/*
			 * Get translation mode
			 */
			*(int *)data = TR_UNTRANS_EVENT;
			return 0;

		case KIOCSDIRECT:
			k->k_event_mode = *(int *)data;
			return 0;
		
		case KIOCRINGBELL:
			kb = (struct kbdbell *)data;
			if (kb)
				kbd_bell_sparms(kb->volume, kb->pitch,
							kb->duration);
			kbdbell();
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

	/*
	 * We identified the ioctl, but we do not handle it.
	 */
	return EOPNOTSUPP;		/* misuse, but what the heck */
}

int
kbdpoll (dev_t dev, int events, struct lwp *l)
{
  return ev_poll (&kbd_softc.k_events, events, l);
}

int
kbdkqfilter(dev_t dev, struct knote *kn)
{

	return (ev_kqfilter(&kbd_softc.k_events, kn));
}

/*
 * Keyboard interrupt handler called straight from MFP at spl6.
 */
void
kbdintr(sr)
int sr;	/* sr at time of interrupt	*/
{
	int	code;
	int	got_char = 0;

	/*
	 * There may be multiple keys available. Read them all.
	 */
	while (KBD->ac_cs & (A_RXRDY|A_OE|A_PE)) {
		got_char = 1;
		if (KBD->ac_cs & (A_OE|A_PE)) {
			code = KBD->ac_da;	/* Silently ignore errors */
			continue;
		}
		kbd_ring[kbd_rbput++ & KBD_RING_MASK] = KBD->ac_da;
	}

	/*
	 * If characters are waiting for transmit, send them.
	 */
	if ((kbd_softc.k_soft_cs & A_TXINT) && (KBD->ac_cs & A_TXRDY)) {
		if (kbd_softc.k_sendp != NULL)
			KBD->ac_da = *kbd_softc.k_sendp++;
		if (--kbd_softc.k_send_cnt <= 0) {
			/*
			 * The total package has been transmitted,
			 * wakeup anyone waiting for it.
			 */
			KBD->ac_cs = (kbd_softc.k_soft_cs &= ~A_TXINT);
			kbd_softc.k_sendp    = NULL;
			kbd_softc.k_send_cnt = 0;
			wakeup((void *)&kbd_softc.k_send_cnt);
		}
	}

	/*
	 * Activate software-level to handle possible input.
	 */
	if (got_char) {
		if (!BASEPRI(sr)) {
			if (!kbd_soft++)
				add_sicallback(kbdsoft, 0, 0);
		} else {
			spl1();
			kbdsoft(NULL, NULL);
		}
	}
}

/*
 * Keyboard soft interrupt handler
 */
void
kbdsoft(junk1, junk2)
void	*junk1, *junk2;
{
	int			s;
	u_char			code;
	struct kbd_softc	*k = &kbd_softc;
	struct firm_event	*fe;
	int			put;
	int			n, get;

	kbd_soft = 0;
	get      = kbd_rbget;

	for (;;) {
		n = kbd_rbput;
		if (get == n) /* We're done	*/
			break;
		n -= get;
		if (n > KBD_RING_SIZE) { /* Ring buffer overflow	*/
			get += n - KBD_RING_SIZE;
			n    = KBD_RING_SIZE;
		}
		while (--n >= 0) {
			code = kbd_ring[get++ & KBD_RING_MASK];

			/*
			 * If collecting a package, stuff it in and
			 * continue.
			 */
			if (k->k_pkg_size && (k->k_pkg_idx < k->k_pkg_size)) {
			    k->k_package[k->k_pkg_idx++] = code;
			    if (k->k_pkg_idx == k->k_pkg_size) {
				/*
				 * Package is complete.
				 */
				switch(k->k_pkg_type) {
#if NMOUSE > 0
				    case KBD_AMS_PKG:
				    case KBD_RMS_PKG:
				    case KBD_JOY1_PKG:
			 		mouse_soft((REL_MOUSE *)k->k_package,
						k->k_pkg_size, k->k_pkg_type);
#endif /* NMOUSE */
				}
				k->k_pkg_size = 0;
			    }
			    continue;
			}
			/*
			 * If this is a package header, init pkg. handling.
			 */
			if (!KBD_IS_KEY(code)) {
				kbd_pkg_start(k, code);
				continue;
			}
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
					    KBD_RELEASED(code) ?
					    WSCONS_EVENT_KEY_UP :
					    WSCONS_EVENT_KEY_DOWN,
					    KBD_SCANCODE(code));
				continue;
			}
#endif /* NWSKBD */
#if NITE>0
			if (kbd_do_modifier(code) && !k->k_event_mode)
				continue;
#endif
			
			/*
			 * if not in event mode, deliver straight to ite to
			 * process key stroke
			 */
			if (!k->k_event_mode) {
				/* Gets to spltty() by itself	*/
#if NITE>0
				ite_filter(code, ITEFILT_TTY);
#endif
				continue;
			}

			/*
			 * Keyboard is generating events.  Turn this keystroke
			 * into an event and put it in the queue.  If the queue
			 * is full, the keystroke is lost (sorry!).
			 */
			s = spltty();
			put = k->k_events.ev_put;
			fe  = &k->k_events.ev_q[put];
			put = (put + 1) % EV_QSIZE;
			if (put == k->k_events.ev_get) {
				log(LOG_WARNING,
					"keyboard event queue overflow\n");
				splx(s);
				continue;
			}
			fe->id    = KBD_SCANCODE(code);
			fe->value = KBD_RELEASED(code) ? VKEY_UP : VKEY_DOWN;
			getmicrotime(&fe->time);
			k->k_events.ev_put = put;
			EV_WAKEUP(&k->k_events);
			splx(s);
		}
		kbd_rbget = get;
	}
}

static	u_char sound[] = {
	0xA8,0x01,0xA9,0x01,0xAA,0x01,0x00,
	0xF8,0x10,0x10,0x10,0x00,0x20,0x03
};

void
kbdbell()
{
	register int	i, sps;

	sps = splhigh();
	for (i = 0; i < sizeof(sound); i++) {
		YM2149->sd_selr = i;
		YM2149->sd_wdat = sound[i];
	}
	splx(sps);
}


/*
 * Set the parameters of the 'default' beep.
 */

#define KBDBELLCLOCK	125000	/* 2MHz / 16 */
#define KBDBELLDURATION	128	/* 256 / 2MHz */

void
kbd_bell_gparms(volume, pitch, duration)
	u_int	*volume, *pitch, *duration;
{
	u_int	tmp;

	tmp = sound[11] | (sound[12] << 8);
	*duration = (tmp * KBDBELLDURATION) / 1000;

	tmp = sound[0] | (sound[1] << 8);
	*pitch = KBDBELLCLOCK / tmp;
	
	*volume = 0;
}

void
kbd_bell_sparms(volume, pitch, duration)
	u_int	volume, pitch, duration;
{
	u_int	f, t;

	f = pitch > 10 ? pitch : 10;	/* minimum pitch */
	if (f > 20000)
		f = 20000;		/* maximum pitch */

	f = KBDBELLCLOCK / f;

	t = (duration * 1000) / KBDBELLDURATION;

	sound[ 0] = f & 0xff;
	sound[ 1] = (f >> 8) & 0xf;
	f -= 1;
	sound[ 2] = f & 0xff;
	sound[ 3] = (f >> 8) & 0xf;
	f += 2;
	sound[ 4] = f & 0xff;
	sound[ 5] = (f >> 8) & 0xf;
	
	sound[11] = t & 0xff;
	sound[12] = (t >> 8) & 0xff;

	sound[13] = 0x03;
}

int
kbdgetcn()
{
	u_char	code;
	int	s = spltty();
	int	ints_active;

	ints_active = 0;
	if (MFP->mf_imrb & IB_AINT) {
		ints_active   = 1;
		MFP->mf_imrb &= ~IB_AINT;
	}
	for (;;) {
		while (!((KBD->ac_cs & (A_IRQ|A_RXRDY)) == (A_IRQ|A_RXRDY)))
			;	/* Wait for key	*/
		if (KBD->ac_cs & (A_OE|A_PE)) {
			code = KBD->ac_da;	/* Silently ignore errors */
			continue;
		}
		code = KBD->ac_da;
#if NITE>0
		if (!kbd_do_modifier(code))
#endif
			break;
	}

	if (ints_active) {
		MFP->mf_iprb  = (u_int8_t)~IB_AINT;
		MFP->mf_imrb |=  IB_AINT;
	}

	splx (s);
	return code;
}

/*
 * Write a command to the keyboard in 'polled' mode.
 */
static int
kbd_write_poll(cmd, len)
u_char	*cmd;
int	len;
{
	int	timeout;

	while (len-- > 0) {
		KBD->ac_da = *cmd++;
		for (timeout = 100; !(KBD->ac_cs & A_TXRDY); timeout--)
			delay(10);
		if (!(KBD->ac_cs & A_TXRDY))
			return (0);
	}
	return (1);
}

/*
 * Write a command to the keyboard. Return when command is send.
 */
void
kbd_write(cmd, len)
u_char	*cmd;
int	len;
{
	struct kbd_softc	*k = &kbd_softc;
	int			sps;

	/*
	 * Get to splhigh, 'real' interrupts arrive at spl6!
	 */
	sps = splhigh();

	/*
	 * Make sure any privious write has ended...
	 */
	while (k->k_sendp != NULL)
		tsleep((void *)&k->k_sendp, TTOPRI, "kbd_write1", 0);

	/*
	 * If the KBD-acia is not currently busy, send the first
	 * character now.
	 */
	KBD->ac_cs = (k->k_soft_cs |= A_TXINT);
	if (KBD->ac_cs & A_TXRDY) {
		KBD->ac_da = *cmd++;
		len--;
	}

	/*
	 * If we're not yet done, wait until all characters are send.
	 */
	if (len > 0) {
		k->k_sendp    = cmd;
		k->k_send_cnt = len;
		tsleep((void *)&k->k_send_cnt, TTOPRI, "kbd_write2", 0);
	}
	splx(sps);

	/*
	 * Wakeup all procs waiting for us.
	 */
	wakeup((void *)&k->k_sendp);
}

/*
 * Setup softc-fields to assemble a keyboard package.
 */
static void
kbd_pkg_start(kp, msg_start)
struct kbd_softc *kp;
u_char		 msg_start;
{
	kp->k_pkg_idx    = 1;
	kp->k_package[0] = msg_start;
	switch (msg_start) {
		case 0xf6:
			kp->k_pkg_type = KBD_MEM_PKG;
			kp->k_pkg_size = 8;
			break;
		case 0xf7:
			kp->k_pkg_type = KBD_AMS_PKG;
			kp->k_pkg_size = 6;
			break;
		case 0xf8:
		case 0xf9:
		case 0xfa:
		case 0xfb:
			kp->k_pkg_type = KBD_RMS_PKG;
			kp->k_pkg_size = 3;
			break;
		case 0xfc:
			kp->k_pkg_type = KBD_CLK_PKG;
			kp->k_pkg_size = 7;
			break;
		case 0xfe:
			kp->k_pkg_type = KBD_JOY0_PKG;
			kp->k_pkg_size = 2;
			break;
		case 0xff:
			kp->k_pkg_type = KBD_JOY1_PKG;
			kp->k_pkg_size = 2;
			break;
		default:
			printf("kbd: Unknown packet 0x%x\n", msg_start);
			break;
	}
}

#if NITE>0
/*
 * Modifier processing
 */
static int
kbd_do_modifier(code)
u_char	code;
{
	u_char	up, mask;

	up   = KBD_RELEASED(code);
	mask = 0;

	switch(KBD_SCANCODE(code)) {
		case KBD_LEFT_SHIFT:
			mask = KBD_MOD_LSHIFT;
			break;
		case KBD_RIGHT_SHIFT:
			mask = KBD_MOD_RSHIFT;
			break;
		case KBD_CTRL:
			mask = KBD_MOD_CTRL;
			break;
		case KBD_ALT:
			mask = KBD_MOD_ALT;
			break;
		case KBD_CAPS_LOCK:
			/* CAPSLOCK is a toggle */
			if(!up)
				kbd_modifier ^= KBD_MOD_CAPS;
			return 1;
	}
	if(mask) {
		if(up)
			kbd_modifier &= ~mask;
		else
			kbd_modifier |= mask;
		return 1;
	}
	return 0;
}	
#endif

#if NWSKBD>0
/*
 * These are the callback functions that are passed to wscons.
 * They really don't do anything worth noting, just call the
 * other functions above.
 */

static int
kbd_enable(void *c, int on)
{
        /* Wonder what this is supposed to do... */
	return 0;
}

static void
kbd_set_leds(void *c, int leds)
{
        /* we can not set the leds */
}

static int
kbd_ioctl(void *c, u_long cmd, void *data, int flag, struct proc *p)
{
	struct wskbd_bell_data *kd;

	switch (cmd)
	{
	case WSKBDIO_COMPLEXBELL:
		kd = (struct wskbd_bell_data *)data;
		kbd_bell(0, kd->pitch, kd->period, kd->volume);
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int*)data = 0;
		return 0;
	case WSKBDIO_GTYPE:
		*(u_int*)data = WSKBD_TYPE_ATARI;
		return 0;
	}

	/*
	 * We are supposed to return EPASSTHROUGH to wscons if we didn't
	 * understand.
	 */
	return (EPASSTHROUGH);
}

static void
kbd_getc(void *c, u_int *type, int *data)
{
	int key;
	key = kbdgetcn();

	*data = KBD_SCANCODE(key);
	*type = KBD_RELEASED(key) ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
}

static void
kbd_pollc(void *c, int on)
{
        kbd_softc.k_pollingmode = on;
}

static void
kbd_bell(void *v, u_int pitch, u_int duration, u_int volume)
{
        kbd_bell_sparms(volume, pitch, duration);
	kbdbell();
}
#endif /* NWSKBD */
