/*	$NetBSD: rtclock_var.h,v 1.4.8.1 2002/06/20 03:42:32 nathanw Exp $	*/

/*
 * Copyright 1993, 1994 Masaru Oki
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
 *      This product includes software developed by Masaru Oki.
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

/*
 * Should be splitted to _reg.h and _var.h
 */

#ifndef _RTCLOCKVAR_H_
#define _RTCLOCKVAR_H_

struct rtc_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bht;
};

/*
 * commands written to mode, HOLD before reading the clock,
 * FREE after done reading.
 */

#define RTC_HOLD_CLOCK	0
#define RTC_FREE_CLOCK	8

#define RTC_REG(x) (bus_space_read_1(rtc->sc_bst, rtc->sc_bht, (x)) & 0x0f)
#define RTC_WRITE(x,v) bus_space_write_1(rtc->sc_bst, rtc->sc_bht, (x), (v))

#define RTC_ADDR	0xe8a000

/* RTC register bank 0 */
#define RTC_SEC		0x01
#define RTC_SEC10	0x03
#define RTC_MIN		0x05
#define RTC_MIN10	0x07
#define RTC_HOUR	0x09
#define RTC_HOUR10	0x0b
#define RTC_WEEK	0x0d
#define RTC_DAY		0x0f
#define RTC_DAY10	0x11
#define RTC_MON		0x13
#define RTC_MON10	0x15
#define RTC_YEAR	0x17
#define RTC_YEAR10	0x19
#define RTC_MODE	0x1b
#define RTC_TEST	0x1d
#define RTC_RESET	0x1f

/* RTC register bank 1 */
#define RTC_CLKOUT	0x01
#define RTC_ADJUST	0x03
#define RTC_AL_MIN	0x05
#define RTC_AL_MIN10	0x07
#define RTC_AL_HOUR	0x09
#define RTC_AL_HOUR10	0x0b
#define RTC_AL_WEEK	0x0d
#define RTC_AL_DAY	0x0f
#define RTC_AL_DAY10	0x11
#define RTC_UNUSED1	0x13
#define RTC_AMPM	0x15
#define RTC_LEAP	0x17
#define RTC_UNUSED2	0x19

#define RTC_BASE_YEAR	1980

#define	range_test(n, l, h)	if ((n) < (l) || (n) > (h)) return(0)
#define	range_test0(n, h)	if ((unsigned)(n) > (h)) return(0)
				/* cast to unsigned in case n is signed */

#ifdef _KERNEL
extern time_t (*gettod) __P((void));
extern int (*settod) __P((long));
#endif
#endif /* _RTCLOCKVAR_H_ */
