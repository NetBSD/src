/* $NetBSD: rtc.c,v 1.2 1996/03/08 17:11:13 mark Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * rtc.c
 *
 * Routines to read and write the RTC and CMOS RAM
 *
 * Created      : 13/10/94
 * Based of kate/display/iiccontrol.c
 */

#include <sys/types.h>
#include <machine/rtc.h>

/* Read a byte from CMOS RAM */

int
cmos_read(location)
	int location;
{
	u_char buff;

/*
 * This commented code dates from when I was translating CMOS address
 * from the RISCOS addresses. Now all addresses are specifed as
 * actual addresses in the CMOS RAM
 */

/* 
 	if (location > 0xF0)
		return(-1);

	if (location < 0xC0)
		buff = location + 0x40;
	else
		buff = location - 0xB0;
*/
	buff = location;

	if (iic_control(RTC_Write, &buff, 1))
		return(-1);
	if (iic_control(RTC_Read, &buff, 1))
		return(-1);

	return(buff);
}


/* Write a byte to CMOS RAM */

int
cmos_write(location, value)
	int location;
	int value;
{
	u_char buff[2];

/*
 * This commented code dates from when I was translating CMOS address
 * from the RISCOS addresses. Now all addresses are specifed as
 * actual addresses in the CMOS RAM
 */

/*	if (location > 0xF0)
		return(-1);

	if (location < 0xC0)
		buff = location + 0x40;
	else
		buff = location - 0xB0;
*/
	buff[0] = location;
	buff[1] = value;

	if (iic_control(RTC_Write, buff, 2))
		return(-1);

	return(0);
}


/* Hex to BCD and BCD to hex conversion routines */

static inline int
hexdectodec(n)
	u_char n;
{
	return(((n >> 4) & 0x0F) * 10 + (n & 0x0F));
}

static inline int
dectohexdec(n)
	u_char n;
{
	return(((n/10) << 4) + (n%10));
}


/* Write the RTC data from an 8 byte buffer */

int
rtc_write(rtc)
	rtc_t *rtc;
{
	u_char buff[8];

	buff[0] = 0;

	buff[1] = dectohexdec(rtc->rtc_micro);
	buff[2] = dectohexdec(rtc->rtc_centi);
	buff[3] = dectohexdec(rtc->rtc_sec);
	buff[4] = dectohexdec(rtc->rtc_min);
	buff[5] = dectohexdec(rtc->rtc_hour);
	buff[6] = dectohexdec(rtc->rtc_day);
	buff[7] = dectohexdec(rtc->rtc_mon);

	if (iic_control(RTC_Write, buff, 8))
		return(0);

	cmos_write(RTC_ADDR_YEAR, rtc->rtc_year);
	cmos_write(RTC_ADDR_CENT, rtc->rtc_cen);

	return(1);
}


/* Read the RTC data into a 8 byte buffer */

int
rtc_read(rtc)
	rtc_t *rtc;
{
	u_char buff[8];
	int byte;
    
	buff[0] = 0;

	if (iic_control(RTC_Write, buff, 1))
		return(0);

	if (iic_control(RTC_Read, buff, 8))
		return(0);

	rtc->rtc_micro = hexdectodec(buff[0] & 0xff);
	rtc->rtc_centi = hexdectodec(buff[1] & 0xff);
	rtc->rtc_sec   = hexdectodec(buff[2] & 0x7f);
	rtc->rtc_min   = hexdectodec(buff[3] & 0x7f);
	rtc->rtc_hour  = hexdectodec(buff[4] & 0x3f);
	rtc->rtc_day   = hexdectodec(buff[5] & 0x3f);
	rtc->rtc_mon   = hexdectodec(buff[6] & 0x1f);

	byte = cmos_read(RTC_ADDR_YEAR);
	if (byte == -1)
		return(0);
	rtc->rtc_year = byte; 

	byte = cmos_read(RTC_ADDR_CENT);
	if (byte == -1)
		return(0);
	rtc->rtc_cen = byte; 

	return(1);
}

/* End of rtc.c */
