/*	$NetBSD: clockreg.h,v 1.1.1.1 1995/03/26 07:12:15 leo Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman.
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

#ifndef _CLOCKREG_H
#define _CLOCKREG_H
/*
 * Atari TT hardware:
 * Motorola MC146818A RealTimeClock
 */

#define	RTC	((struct rtc *)AD_RTC)

struct rtc {
	volatile u_char	rtc_dat[4];
};

#define rtc_regno	rtc_dat[1]	/* register nr. select		*/
#define rtc_data	rtc_dat[3]	/* data register		*/

/*
 * Register number definitions
 */
#define	RTC_SEC		0
#define	RTC_ASEC	1
#define	RTC_MIN		2
#define	RTC_A_MIN	3
#define	RTC_HOUR	4
#define	RTC_A_HOUR	5
#define	RTC_WDAY	6
#define	RTC_DAY		7
#define	RTC_MONTH	8
#define	RTC_YEAR	9
#define	RTC_REGA	10
#define	RTC_REGB	11
#define	RTC_REGC	12
#define	RTC_REGD	13
#define	RTC_RAMBOT	14	/* Reg. offset of nv-RAM		*/
#define	RTC_RAMSIZ	50	/* #bytes of nv-RAM available		*/

/*
 * Define fields for register A
 */
#define	RTC_A_UIP	0x80	/* Update In Progress			*/
#define	RTC_A_DV2	0x40	/* Divider select			*/
#define	RTC_A_DV1	0x20	/* Divider select			*/
#define	RTC_A_DV0	0x10	/* Divider select			*/
#define	RTC_A_RS3	0x08	/* Rate Select				*/
#define	RTC_A_RS2	0x04	/* Rate Select				*/
#define	RTC_A_RS1	0x02	/* Rate Select				*/
#define	RTC_A_RS0	0x01	/* Rate Select				*/

/*
 * Define fields for register B
 */
#define	RTC_B_SET	0x80	/* SET date/time				*/
#define	RTC_B_PIE	0x40	/* Periodic Int. Enable			*/
#define	RTC_B_AIE	0x20	/* Alarm Int. Enable			*/
#define	RTC_B_UIE	0x10	/* Update Ended Int. Enable		*/
#define	RTC_B_SQWE	0x08	/* Square Wave Output Enable		*/
#define	RTC_B_DM	0x04	/* Binary Data mode			*/
#define	RTC_B_24_12	0x02	/* 24 Hour mode				*/
#define	RTC_B_DSE	0x01	/* DST Enable				*/

/*
 * Define fields for register C
 */
#define	RTC_C_IRQF	0x80	/* IRQ flag				*/
#define	RTC_C_PF	0x40	/* Periodic Int. flag			*/
#define	RTC_C_AF	0x20	/* Alarm Int. flag			*/
#define	RTC_C_UF	0x10	/* Update Ended Int. flag		*/

/*
 * Define fields for register D
 */
#define	RTC_D_VRT	0x80	/* Valid Ram and Time			*/

/*
 * Some useful constants/macros
 */
#define	is_leap(x)		(!(x % 4) && ((x % 100) || !(x % 1000)))
#define	range_test(n, l, h)	((n) < (l) || (n) > (h))
#define	SECS_DAY		86400L
#define	SECS_HOUR		3600L
#define	STARTOFTIME		1970
#endif /* _CLOCKREG_H */
