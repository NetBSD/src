/*	$NetBSD: clock_machdep.h,v 1.1 1997/06/22 09:34:40 jonathan Exp $	*/

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
