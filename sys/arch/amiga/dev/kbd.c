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
 *
 *	kbd.c
 *	$Id: kbd.c,v 1.6 1994/02/11 07:01:55 chopps Exp $
 */

#include "ite.h"

#if NITE > 0
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/proc.h"
#include "sys/conf.h"
#include "sys/file.h"
#include "sys/uio.h"
#include "sys/kernel.h"
#include "sys/syslog.h"

#include "device.h"
#include "kbdreg.h"
#include "itevar.h"
#include "machine/cpu.h"

#include "../amiga/custom.h"
#include "../amiga/cia.h"

/* for sun-like event mode, if you go thru /dev/kbd. */
#include "event_var.h"
#include "vuid_event.h"

struct kbd_softc {
  int k_event_mode;  	 /* if true, collect events, else pass to ite */
  struct evvar k_events; /* event queue state */
} kbd_softc;

/* definitions for amiga keyboard encoding. */
#define KEY_CODE(c)  ((c) & 0x7f)
#define KEY_UP(c)    ((c) & 0x80)

void
kbdenable ()
{
  int s = spltty();

  /* collides with external ints from SCSI, watch out for this when
     enabling/disabling interrupts there !! */
  custom.intena = INTF_SETCLR | INTF_PORTS;
  ciaa.icr = CIA_ICR_IR_SC | CIA_ICR_SP;  /* SP interrupt enable */
  ciaa.cra &= ~(1<<6);		/* serial line == input */
  kbd_softc.k_event_mode = 0;
  kbd_softc.k_events.ev_io = 0;
  
  splx (s);
}


int
kbdopen (dev_t dev, int flags, int mode, struct proc *p)
{
  int s, error;

  if (kbd_softc.k_events.ev_io)
    return EBUSY;

  kbd_softc.k_events.ev_io = p;
  ev_init(&kbd_softc.k_events);
  return (0);
}

int
kbdclose (dev_t dev, int flags, int mode, struct proc *p)
{
  /* Turn off event mode, dump the queue */
  kbd_softc.k_event_mode = 0;
  ev_fini(&kbd_softc.k_events);
  kbd_softc.k_events.ev_io = NULL;
  return (0);
}

int
kbdread (dev_t dev, struct uio *uio, int flags)
{
  return ev_read (&kbd_softc.k_events, uio, flags);
}

/* this routine should not exist, but is convenient to write here for now */
int
kbdwrite (dev_t dev, struct uio *uio, int flags)
{
  return EOPNOTSUPP;
}

int
kbdioctl (dev_t dev, int cmd, register caddr_t data, int flag, struct proc *p)
{
  register struct kbd_softc *k = &kbd_softc;

  switch (cmd) 
    {
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

    case FIONBIO:		/* we will remove this someday (soon???) */
      return 0;

    case FIOASYNC:
      k->k_events.ev_async = *(int *)data != 0;
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
kbdselect (dev_t dev, int rw, struct proc *p)
{
  return ev_select (&kbd_softc.k_events, rw, p);
}


int
kbdintr (mask)
     int mask;
{
  u_char c, in;
  struct kbd_softc *k = &kbd_softc;
  struct firm_event *fe;
  int put;
 
  /* now only invoked from generic CIA interrupt handler if there *is*
     a keyboard interrupt pending */
    
  in = ciaa.sdr;
  /* ack */
  ciaa.cra |= (1 << 6);	/* serial line output */
  /* wait 200 microseconds (for bloody Cherry keyboards..) */
  DELAY(200);
  ciaa.cra &= ~(1 << 6);

  c = ~in;	/* keyboard data is inverted */

  /* process the character */
  
  c = (c >> 1) | (c << 7);	/* rotate right once */

  
  /* XXX THIS IS WRONG!!! The screenblanker should route thru ite.c, which 
     should call thru it's driver table, ie. we need a new driver-dependant
     function for this feature! */

  cc_unblank ();

  /* if not in event mode, deliver straight to ite to process key stroke */
  if (! k->k_event_mode)
    {
      itefilter (c, ITEFILT_TTY);
      return;
    }

  /* Keyboard is generating events.  Turn this keystroke into an
     event and put it in the queue.  If the queue is full, the
     keystroke is lost (sorry!). */
  
  put = k->k_events.ev_put;
  fe = &k->k_events.ev_q[put];
  put = (put + 1) % EV_QSIZE;
  if (put == k->k_events.ev_get) 
    {
      log(LOG_WARNING, "keyboard event queue overflow\n"); /* ??? */
      return;
    }
  fe->id = KEY_CODE(c);
  fe->value = KEY_UP(c) ? VKEY_UP : VKEY_DOWN;
  fe->time = time;
  k->k_events.ev_put = put;
  EV_WAKEUP(&k->k_events);
}


int
kbdbell()
{
  /* nice, mykes provided audio-support! */
  cc_bell ();
}


int
kbdgetcn ()
{
  int s = spltty ();
  u_char ints, mask, c, in;

  for (ints = 0; ! ((mask = ciaa.icr) & CIA_ICR_SP); ints |= mask) ;

  in = ciaa.sdr;
  c = ~in;
  
  /* ack */
  ciaa.cra |= (1 << 6);	/* serial line output */
  ciaa.sdr = 0xff;		/* ack */
  /* wait 200 microseconds */
  DELAY(200);    /* XXXX only works as long as DELAY doesn't use a timer and waits.. */
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
kbdattach() {}
#endif
