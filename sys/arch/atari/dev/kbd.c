/*	$NetBSD: kbd.c,v 1.1.1.1 1995/03/26 07:12:12 leo Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <dev/cons.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/acia.h>
#include <machine/video.h>
#include <atari/dev/itevar.h>
#include <atari/dev/kbdreg.h>
#include <atari/dev/event_var.h>
#include <atari/dev/vuid_event.h>

/*
 * The ringbuffer is the interface between the hard and soft interrupt handler.
 * The hard interrupt runs straight from the MFP interrupt.
 */
#define KBD_RING_SIZE	16    /* Size of the ring buffer, must be power of 2 */
#define KBD_RING_MASK	15    /* Modulo mask for above			     */

static u_char		kbd_ring[KBD_RING_SIZE];
static volatile u_int	kbd_rbput = 0;	/* 'put' index			*/
static u_int		kbd_rbget = 0;	/* 'get' index			*/
static u_char		kbd_soft  = 0;	/* 1: Softint has been scheduled*/

struct kbd_softc {
	int		k_event_mode;	/* if 1, collect events,	*/
					/*   else pass to ite		*/
	struct evvar	k_events;	/* event queue state		*/
};

static struct kbd_softc kbd_softc;

static void kbdsoft __P((void));
static void kbdattach __P((struct device *, struct device *, void *));
static int  kbdmatch __P((struct device *, struct cfdata *, void *));

struct cfdriver kbdcd = {
	NULL, "kbd", (cfmatch_t)kbdmatch, kbdattach,
	DV_DULL, sizeof(struct device), NULL, 0 };


/*ARGSUSED*/
static	int
kbdmatch(pdp, cfp, auxp)
struct	device *pdp;
struct	cfdata *cfp;
void	*auxp;
{
	if(!strcmp((char *)auxp, "kbd"))
		return(1);
	return(0);
}

/*ARGSUSED*/
static void
kbdattach(pdp, dp, auxp)
struct	device *pdp, *dp;
void	*auxp;
{
	printf("\n");
}

/* definitions for atari keyboard encoding. */
#define KEY_CODE(c)  ((c) & 0x7f)
#define KEY_UP(c)    ((c) & 0x80)

void
kbdenable()
{
	int s, code;

	s = spltty();
	/*
	 * Initialize ACIA port
	 */
	code = KBD->ac_da;	/* Clear error conditions	*/

	/* divide by 16, 8 data, 1 stop, no parity, enable interrupts */
	KBD->ac_cs = KBD_INIT | A_RXINT;
#if 0 /* XXX Turn off mouse??? */
	KBD->ac_da = 0x12;
#endif

	/*
	 * Enable interrupts from MFP
	 */
	MFP->mf_iprb &= ~IB_AINT;
	MFP->mf_ierb |= IB_AINT;
	MFP->mf_imrb |= IB_AINT;
	code = KBD->ac_da;	/* Clear error conditions	*/

	kbd_softc.k_event_mode   = 0;
	kbd_softc.k_events.ev_io = 0;
	splx(s);
}

int kbdopen(dev_t dev, int flags, int mode, struct proc *p)
{
	int s, error;

	if(kbd_softc.k_events.ev_io)
		return EBUSY;

	kbd_softc.k_events.ev_io = p;
	ev_init(&kbd_softc.k_events);
	return (0);
}

int
kbdclose(dev_t dev, int flags, int mode, struct proc *p)
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

/* this routine should not exist, but is convenient to write here for now */
int kbdwrite(dev_t dev, struct uio *uio, int flags)
{
	return EOPNOTSUPP;
}

int
kbdioctl(dev_t dev,u_long cmd,register caddr_t data,int flag,struct proc *p)
{
	register struct kbd_softc *k = &kbd_softc;

	switch (cmd) {
		case KIOCTRANS:
			if(*(int *)data == TR_UNTRANS_EVENT)
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

		case FIONBIO:	/* we will remove this someday (soon???) */
			return 0;

		case FIOASYNC:
			k->k_events.ev_async = *(int *)data != 0;
				return 0;

		case TIOCSPGRP:
			if(*(int *)data != k->k_events.ev_io->p_pgid)
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
kbdselect (dev_t dev, int rw, struct proc *p)
{
  return ev_select (&kbd_softc.k_events, rw, p);
}

/*
 * Keyboard interrupt handler called straight from MFP.
 */
int
kbdintr(sr)
int sr;	/* sr at time of interrupt	*/
{
	int	code;

	/*
	 * There may be multiple keys available. Read them all.
	 */
	while(KBD->ac_cs & (A_IRQ|A_RXRDY)) {
		if(KBD->ac_cs & (A_OE|A_PE)) {
			code = KBD->ac_da;	/* Silently ignore overruns */
			continue;
		}
		kbd_ring[kbd_rbput++ & KBD_RING_MASK] = KBD->ac_da;
	}
	if(!BASEPRI(sr)) {
		if(!kbd_soft++)
			add_sicallback(kbdsoft, 0, 0);
	}
	else {
		spl1();
		kbdsoft();
	}
}

/*
 * Keyboard soft interrupt handler
 */
void
kbdsoft()
{
	int			s;
	u_char			code;
	struct kbd_softc	*k = &kbd_softc;
	struct firm_event	*fe;
	int			put;
	int			n, get;

	kbd_soft = 0;
	get      = kbd_rbget;

	for(;;) {
		n = kbd_rbput;
		if(get == n) /* We're done	*/
			break;
		n -= get;
		if(n > KBD_RING_SIZE) { /* Ring buffer overflow	*/
			get += n - KBD_RING_SIZE;
			n    = KBD_RING_SIZE;
		}
		while(--n >= 0) {
			code = kbd_ring[get++ & KBD_RING_MASK];

			/*
			 * if not in event mode, deliver straight to ite to
			 * process key stroke
			 */
			if(!k->k_event_mode) {
				/* Gets to spltty() by itself	*/
				ite_filter(code, ITEFILT_TTY);
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
			if(put == k->k_events.ev_get) {
				log(LOG_WARNING,
					"keyboard event queue overflow\n");
				splx(s);
				continue;
			}
			fe->id             = KEY_CODE(code);
			fe->value          = KEY_UP(code) ? VKEY_UP : VKEY_DOWN;
			fe->time           = time;
			k->k_events.ev_put = put;
			EV_WAKEUP(&k->k_events);
			splx(s);
		}
		kbd_rbget = get;
	}
}

static	char sound[] = {
	0xA8,0x01,0xA9,0x01,0xAA,0x01,0x00,
	0xF8,0x10,0x10,0x10,0x00,0x20,0x03
};

int
kbdbell()
{
  register int	i, sps;

  sps = spltty();
  for(i = 0; i < sizeof(sound); i++) {
	SOUND->sd_selr = i;
	SOUND->sd_wdat = sound[i];
  }
  splx(sps);
}

int
kbdgetcn()
{
	u_char	code;
	int		s = spltty();

	MFP->mf_imrb &= ~IB_AINT;
	while(!(KBD->ac_cs & A_IRQ))
		;	/* Wait for key	*/

	MFP->mf_iprb &= ~IB_AINT;
	MFP->mf_imrb |=  IB_AINT;

	code = KBD->ac_da;
	splx (s);
	return code;
}
