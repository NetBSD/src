/* $NetBSD: clock.c,v 1.1.2.3 2013/01/16 05:33:08 yamt Exp $ */

/*
 * Copyright (c) 2003 Tetsuya Isaki. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
#include <sys/types.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include "iocs.h"
#include "libx68k.h"
#include "consio.h"	/* XXX: for MFP_TIMERC */

/* x68k's RTC is defunct 2079, so there is no y2100 problem. */
#define LEAPYEAR(y)	(((y) % 4) == 0)
#define SECDAY	(24 * 60 * 60)

int rtc_offset;

const int yday[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

satime_t
getsecs(void)
{
	int val;
	int sec, min, hour, day, mon, year;
	int days, y;

	/* Get date & time via IOCS */
	val  = IOCS_DATEBIN(IOCS_BINDATEGET());
	year = ((val & 0x0fff0000) >> 16) + 1980;
	mon  = ((val & 0x0000ff00) >>  8);
	day  =  (val & 0x000000ff);

	val  = IOCS_TIMEBIN(IOCS_TIMEGET());
	hour = ((val & 0x00ff0000) >> 16);
	min  = ((val & 0x0000ff00) >>  8);
	sec  =  (val & 0x000000ff);

	/* simple sanity checks */
	if (mon < 1 || mon > 12 || day < 1 || day > 31)
		return 0;
	if (hour > 23 || min > 59 || sec > 59)
		return 0;

	days = 0;
	for (y = 1970; y < year; y++)
		days += 365 + LEAPYEAR(y);
	days += yday[mon - 1] + day - 1;
	if (LEAPYEAR(y) && mon > 2)
		days++;

	/* now we have days since Jan 1, 1970. the rest is easy... */
	return (days * SECDAY) + (hour * 3600) + (min * 60) + sec
		+ (rtc_offset * 60);
}

void
delay(int us)
{
	int end;

	/* sanity check */
	if (us < 1)
		return;

	/*
	 * assume IPLROM initializes MFP Timer-C as following:
	 *  - free run down count
	 *  - 1/200 presclaer (50us with 4MHz clock)
	 *
	 * Note we can't change MFP_TCDR reload value (200)
	 * because awaitkey_1sec() in consio.c assumes that value.
	 */

	/* handle >5ms delays first */
	for (; us > 5000; us -= 5000) {
		MFP_TIMERC = 200;
		while (MFP_TIMERC >= 100)
			continue;
	}

	/* count rest fractions */
	end = 200 - (us / 50);
	MFP_TIMERC = 200;
	while (MFP_TIMERC >= end)
		continue;
}
