/*	$NetBSD: clock_machdep.h,v 1.2 1997/06/22 22:41:33 jonathan Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * System-dependent clock declarations for the ``cpu-independent''
 * clock interface.
 *
 * This file must prototype or define the folluwing functions of
 * macros (one or more of which may be no-ops):
 *
 * CLOCK_RATE	default rate at which clock runs. Some platforms
 *		run the RTC at a fixed rate, independent of
 *		the acutal RTC hardware in use. clock
 *
 * clk_rtc_to_systime(struct clocktime *ct, base)
 * 		hook called just after getting a time from the RTC but
 * 		before sanity checks, and before (*ct) is written to
 * 		the RTC hardware.
 *     		Allows machine-dependent ransformation to the clocktime
 *		(e.g., for compatibility with PROMS which filter out
 *		high-order RTC bits.)
 * 
 * clk_systime_to_rtc(struct clocktime *ct, base)
 *
 *              hook called just before *ct is written to the RTC hardware.
 *		Allows 	machine-dependent transformation of the RTC 
 *		(e.g., forcing the non-volatile RTC to keep time-of-year
 * 		even when the hardware has more higher-order bits.)
 *
 */


/* The  default clock rate on a pmax is 256 Hz. */
#define CLOCK_RATE	256


/*  
 * Experiments (and  passing years) show that Decstation PROMS
 * assume the kernel uses the clock chip as a time-of-year clock.
 * The PROM assumes the clock is always set to 1972 or 1973, and contains
 * time-of-year in seconds.   The PROM checks the clock at boot time,
 * and if it's outside that range, sets it to 1972-01-01.
 * Use clk_systime_to_rtc() and clk_rtc_to_systime to hide that.
 *
 * XXX should be at the mc146818 layer?
 * XXX should use filesystem base time to convert to a TOY, and
 *    get rid of the fixed offset which needs changing every two years.
*/

#define	DEC_DAY_OFFSET	0	/* maybe 1 */
#define	DEC_YR_OFFSET	25	/* good until Dec 31, 1998 */

/*
 * convert RTC time to system time.
 * On a pmax,  add the current year less 1972 to the RTC clock time.
 * (should be 1973 in leapyears, but we don't care.)
 */

#define clk_rtc_to_systime(ct, base) \
    do { (ct)->year += DEC_YR_OFFSET; (ct)->day += DEC_DAY_OFFSET; } while(0)

/*
 * convert RTC time to system time.
 * On a pmax,  subtract the current year less 1972 from the RTC clock time,
 * to give a time in 1972 or 1973..
 * (should use 1973 only for  leapyears but we don't care.)
 */
#define clk_systime_to_rtc(ct, base) \
    do { (ct)->year -= DEC_YR_OFFSET; (ct)->day -= DEC_DAY_OFFSET; } while(0)
