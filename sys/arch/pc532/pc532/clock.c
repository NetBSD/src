/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 *
 *	$Id: clock.c,v 1.1.1.1 1993/09/09 23:53:47 phil Exp $
 */

/*
 * Primitive clock interrupt routines.
 */

#include "param.h"
#include "time.h"
#include "kernel.h"
#include "icu.h"

#include "machine/param.h"

void spinwait __P((int));

/* Get access to the time variable. */
extern struct timeval time;

startrtclock()
{
  int s;
  int x;
  int timer = (ICU_CLK_HZ) / hz;

  /* Write the timer values to the ICU. */
  WR_ADR (unsigned short, ICU_ADR + HCSV, timer);
  WR_ADR (unsigned short, ICU_ADR + HCCV, timer);

}

/* convert 2 digit BCD number */
bcd(i)
int i;
{
	return ((i/16)*10 + (i%16));
}

/* convert years to seconds (from 1970) */
unsigned long
ytos(y)
int y;
{
	int i;
	unsigned long ret;

	ret = 0; y = y - 70;
	for(i=0;i<y;i++) {
		if (i % 4) ret += 365*24*60*60;
		else ret += 366*24*60*60;
	}
	return ret;
}

/* convert months to seconds */
unsigned long
mtos(m,leap)
int m,leap;
{
	int i;
	unsigned long ret;

	ret = 0;
	for(i=1;i<m;i++) {
		switch(i){
		case 1: case 3: case 5: case 7: case 8: case 10: case 12:
			ret += 31*24*60*60; break;
		case 4: case 6: case 9: case 11:
			ret += 30*24*60*60; break;
		case 2:
			if (leap) ret += 29*24*60*60;
			else ret += 28*24*60*60;
		}
	}
	return ret;
}

/*
 *
 * 
 */

inittodr(base)
	time_t base;
{
  extern int have_rtc;
  unsigned char buffer[8];
  unsigned int sec;
  int leap;

  if (!have_rtc) return;

  /* Read rtc and convert to seconds since Jan 1, 1970. */

  rw_rtc ( buffer, 0);  /* Read the rtc. */

  leap = (bcd(buffer[7]) % 4) == 0;
  sec = ytos(bcd(buffer[7]))		/* year */
      + mtos(bcd(buffer[6]), leap)	/* month */
      + bcd(buffer[5])*24*60*60		/* days */
      + bcd(buffer[3])*60*60		/* hours */
      + bcd(buffer[2])*60		/* minutes */
      + bcd(buffer[1]);			/* seconds */

  sec -= 24*60*60; /* XXX why ??? Compensate for Jan 1, 1970??? */  

  time.tv_sec = sec;


}


/*
 * Restart the clock.
 */
resettodr()
{
}

/*
 * Wire clock interrupt in.
 */
enablertclock()
{
  /* Set the clock interrupt enable (CICTL) */
  WR_ADR (unsigned char, ICU_ADR +CICTL, 0x30);
  PL_zero |= SPL_CLK | SPL_SOFTCLK | SPL_NET | SPL_IMP;
}

/*
 * Delay for some number of milliseconds.
 */
void
spinwait(int millisecs)
{
	DELAY(1000 * millisecs);
}

DELAY(n)
{ volatile int N = (n); while (--N > 0); }
