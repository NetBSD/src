/*      $NetBSD: rtc.c,v 1.1.1.1 1998/06/09 07:53:06 dbj Exp $        */
/*
 * Copyright (c) 1998 Darrin Jewell
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

/* Now using this in the kernel.  This should be turned into a device
 * Darrin B Jewell <jewell@mit.edu>  Tue Jan 27 20:59:25 1998
 */

#include <sys/param.h>
#include <sys/cdefs.h>          /* for __P */
#include <sys/systm.h>          /* for panic */

#include <machine/cpu.h>

#include <next68k/dev/clockreg.h>

void rtc_init __P((void));
u_char rtc_read __P((u_char));
void rtc_write __P((u_char, u_char));
void poweroff __P((void));
time_t getsecs __P((void));

u_char new_clock;
volatile u_int *scr2 = (u_int *)NEXT_P_SCR2; /* will get memory mapped in rtc_init */

void
rtc_init(void)
{
	u_char val;
	
        scr2 = (u_int *)IIOV(NEXT_P_SCR2);
	val = rtc_read(RTC_STATUS);
	new_clock = (val & RTC_NEW_CLOCK) ? 1 : 0;

#if 0
        if (!new_clock) {
          /* set up interrupts, this should be in clock.c */
          rtc_write(RTC_INTRCTL,RTC_64HZ);
        }
#endif

        printf("Looks like a %s clock chip.\n",
               (new_clock?
                "MCS1850 (new style)":
                "MC68HC68T1 (old style)"));
        print_rtc();
        
}


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
rtc_write(u_char reg, u_char v)
{
	int i;
	u_int tmp;
	u_char val;
    
	*scr2 = (*scr2 & ~(SCR2_RTDATA | SCR2_RTCLK)) | SCR2_RTCE;
	DELAY(1);

	val = reg|RTC_WRITE;

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

        DELAY(1);

	for (i=0; i<8; i++) {
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);
		if (v & 0x80)
			tmp |= SCR2_RTDATA;

		*scr2 = tmp;
		DELAY(1);
		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		v <<= 1;
	}

	*scr2 &= ~(SCR2_RTDATA|SCR2_RTCLK|SCR2_RTCE);
	DELAY(1);
}

void
poweroff(void)
{
	int reg, t;

	if(new_clock) {
          reg = RTC_CNTR3;
        } else {
          reg = RTC_CNTR0;
        }

	t = rtc_read(reg);	/* seconds */
	/* wait for clock to tick */
	while(t == rtc_read(reg));

	DELAY(850000);	/* hardware bug workaround ? */

	if(new_clock) {
          reg = RTC_CONTROL;
        } else {
          reg = RTC_INTRCTL;
        }

	rtc_write(reg, rtc_read(reg)|(RTC_PDOWN));

        panic("Failed to poweroff!\n");
}


time_t
getsecs(void)
{
	u_int secs = 0;
	
	if (new_clock) {
		secs = rtc_read(RTC_CNTR3) << 24 |
		       rtc_read(RTC_CNTR2) << 16 |
		       rtc_read(RTC_CNTR1) << 8  |
		       rtc_read(RTC_CNTR0);
	} else {

		u_char y,o,d,t,h,m,s;
#define BCD_DECODE(x) (((x) >> 4) * 10 + ((x) & 0xf))
                y = rtc_read(RTC_YR);
                o = rtc_read(RTC_MON);
                t = rtc_read(RTC_DATE);
		d = rtc_read(RTC_DAY);
		h = rtc_read(RTC_HRS);
		m = rtc_read(RTC_MIN);
		s = rtc_read(RTC_SEC);
                printf("RTC: years = %d, months = %d, date = %d, day = %d\n",
                       BCD_DECODE(y),BCD_DECODE(o),
                       BCD_DECODE(t),BCD_DECODE(d));
                printf("RTC: hours = %d, minutes = %d, seconds = %d\n",
                       BCD_DECODE(h),BCD_DECODE(m),BCD_DECODE(s));
                printf ("WARNING: Don't really know how to convert rtc to secs\n");
                secs = (d*24*60*60)+(h*60*60)+(m*60)+s;
	}
	
	return secs;
}

void
setsecs(time_t t)
{
  u_int secs = t;

  /* Stop the clock */
  rtc_write(RTC_CONTROL,rtc_read(RTC_CONTROL) & ~RTC_START);
  if (new_clock) {
    rtc_write(RTC_CNTR3, (secs << 24) & 0xff);
    rtc_write(RTC_CNTR2, (secs << 16) & 0xff);
    rtc_write(RTC_CNTR1, (secs <<  8) & 0xff);
    rtc_write(RTC_CNTR0, (secs) & 0xff);

  } else {
    printf("WARNING: Don't know how to set old real time clock\n");
#if 0
    u_char yr, mn, dy, hr, mi, se;
    
#define BCD_ENCODE(v) ((v) % 10) | ((((v) / 10) % 10)<<4)

    rtc_write(RTC_SEC,);
    rtc_write(RTC_MIN,);
    rtc_write(RTC_HRS,);
    rtc_write(RTC_DAY,);
    rtc_write(RTC_DATE,);
    rtc_write(RTC_MON,);
    rtc_write(RTC_YR,);
#endif
  }

}

#define RTC_PRINT(x)  printf("\t%s = 0x%x\n",#x, rtc_read(x))

void
print_rtc(void)
{
  if (new_clock) {
    RTC_PRINT(RTC_RAM);
    RTC_PRINT(RTC_CNTR0);
    RTC_PRINT(RTC_CNTR1);
    RTC_PRINT(RTC_CNTR2);
    RTC_PRINT(RTC_CNTR3);
    RTC_PRINT(RTC_ALARM0);
    RTC_PRINT(RTC_ALARM1);
    RTC_PRINT(RTC_ALARM2);
    RTC_PRINT(RTC_ALARM3);
    RTC_PRINT(RTC_STATUS);
    RTC_PRINT(RTC_CONTROL);
  } else {
    RTC_PRINT(RTC_RAM);
    RTC_PRINT(RTC_SEC);
    RTC_PRINT(RTC_MIN);
    RTC_PRINT(RTC_HRS);
    RTC_PRINT(RTC_DAY);
    RTC_PRINT(RTC_DATE);
    RTC_PRINT(RTC_MON);
    RTC_PRINT(RTC_YR);
    RTC_PRINT(RTC_ALARM_SEC);
    RTC_PRINT(RTC_ALARM_MIN);
    RTC_PRINT(RTC_ALARM_HR);
    RTC_PRINT(RTC_STATUS);
    RTC_PRINT(RTC_CONTROL);
    RTC_PRINT(RTC_INTRCTL);
  }
}
