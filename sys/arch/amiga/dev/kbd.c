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

void
kbdenable ()
{
  int s = spltty();

  /* collides with external ints from SCSI, watch out for this when
     enabling/disabling interrupts there !! */
  custom.intena = INTF_SETCLR | INTF_PORTS;
  ciaa.icr = (1<<7) | (1<<3);	/* SP interrupt enable */
  ciaa.cra &= ~(1<<6);		/* serial line == input */
  
  splx (s);
}


int
kbdintr ()
{
  /* THIS CLEARS ALL INTERRUPTS FROM CIAA! Will need a special cia
     interrupt handler to be able to use the ciaa timers besides driving
     the keyboard !! */
  u_char ints = ciaa.icr;
  u_char c, in;
 
  if (! (ints & (1<<3)))
    return 0;
    
  in = ciaa.sdr;
  /* ack */
  ciaa.cra |= (1 << 6);	/* serial line output */
  /* wait 85 microseconds */
  DELAY(85);
  ciaa.cra &= ~(1 << 6);

  c = ~in;	/* keyboard data is inverted */

  /* process the character. 
  
     Should implement RAW mode for X11 later here !! */
  c = (c >> 1) | (c << 7);	/* rotate right once */

  itefilter (c, ITEFILT_TTY);
}


int
kbdbell()
{

 /* not yet.. */

}


int
kbdgetcn ()
{
  int s = spltty ();
  u_char ints, c, in;

  /* this is very very nasty... */
  while (! (ciaa.icr & (1<<3))) ;

  in = ciaa.sdr;
  c = ~in;
  
  /* ack */
  ciaa.cra |= (1 << 6);	/* serial line output */
  ciaa.sdr = 0xff;		/* ack */
  /* wait 85 microseconds */
  DELAY(85);    /* XXXX only works as long as DELAY doesn't use a timer and waits.. */
  ciaa.cra &= ~(1 << 6);
  ciaa.sdr = in;

  splx (s);
  c = (c >> 1) | (c << 7);
  return c;
}

#endif
