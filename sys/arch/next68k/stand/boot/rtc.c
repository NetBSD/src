/*      $NetBSD: rtc.c,v 1.4.6.1 2006/04/22 11:37:51 simonb Exp $        */
/*
 * Copyright (c) 1997 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <next68k/dev/clockreg.h>
#include <machine/cpu.h>
#include <stand.h>

u_char rtc_read(u_char);
void rtc_init(void);
time_t getsecs(void);


/* ### where shall I put this definition? */
#define	DELAY(n)	{ register int N = (n); while (--N > 0); }

static volatile u_int *scr2 = (u_int *)NEXT_P_SCR2_CON;
static u_char new_clock;

u_char
rtc_read(u_char reg)
{
	int i;
	u_int tmp;
	u_char val;
    
	*scr2 = (*scr2 & ~(SCR2_RTDATA | SCR2_RTCLK)) | SCR2_RTCE;
	DELAY(1);

	val = reg;
	for (i=0; i<8; i++) {
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);
		if (val & 0x80)
			tmp |= SCR2_RTDATA;

		*scr2 = tmp;
		DELAY(1);
		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		val <<= 1;
	}

	val = 0;			/* should be anyway */
	for (i=0; i<8; i++) {
		val <<= 1;
	
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);

		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		if (*scr2 & SCR2_RTDATA)
			val |= 1;
	}

	*scr2 &= ~(SCR2_RTDATA|SCR2_RTCLK|SCR2_RTCE);
	DELAY(1);

	return val;
}

void
rtc_init(void)
{
	u_char val;
	
	val = rtc_read(RTC_STATUS);
	new_clock = (val & RTC_NEW_CLOCK) ? 1 : 0;
}

time_t
getsecs(void)
{
	u_int secs;
	
	if (new_clock) {
		secs = rtc_read(RTC_CNTR0) << 24 |
		       rtc_read(RTC_CNTR1) << 16 |
		       rtc_read(RTC_CNTR2) << 8  |
		       rtc_read(RTC_CNTR3);
	} else {
		u_char d,h,m,s;
#define BCD_DECODE(x) (((x) >> 4) * 10 + ((x) & 0xf))

		d = rtc_read(RTC_DAY);
		h = rtc_read(RTC_HRS);
		m = rtc_read(RTC_MIN);
		s = rtc_read(RTC_SEC);
		secs = BCD_DECODE(d) * (60*60*24) +
		       BCD_DECODE(h) * (60*60) +
		       BCD_DECODE(m) * 60 +
		       BCD_DECODE(s);
	}
	
	return secs;
}
