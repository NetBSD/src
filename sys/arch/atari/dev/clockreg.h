/*	$NetBSD: clockreg.h,v 1.6.22.1 2017/12/03 11:35:57 jdolecek Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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

#ifndef _CLOCKREG_H
#define _CLOCKREG_H
#include "opt_mbtype.h"

/*
 * Atari TT hardware:
 * Motorola MC146818A RealTimeClock
 */

#define	RTC	((struct rtc *)AD_RTC)

struct rtc {
	volatile u_char	rtc_dat[4];
};

#ifdef _ATARIHW_
#define rtc_regno	rtc_dat[1]	/* register nr. select		*/
#define rtc_data	rtc_dat[3]	/* data register		*/
#elif _MILANHW_
#define rtc_regno	rtc_dat[0]	/* register nr. select		*/
#define rtc_data	rtc_dat[1]	/* data register		*/
#endif

/*
 * Pull in general mc146818 definitions
 */
#include <dev/ic/mc146818reg.h>

/*
 * Some useful constants/macros
 */
#define	range_test(n, l, h)	((n) < (l) || (n) > (h))
#define	GEMSTARTOFTIME		((machineid & ATARI_CLKBROKEN) ? 1970 : 1968)
#endif /* _CLOCKREG_H */
