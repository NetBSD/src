/*
 * Copyright (c) 1993 Paul Mackerras.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied.  The author makes no representations
 * about the suitability of this software for any purpose.
 *
 *	$Id: clockreg.h,v 1.1 1994/02/22 23:49:31 paulus Exp $
 */

#ifndef PRF_INTERVAL
#define PRF_INTERVAL    CLK_INTERVAL
#endif

/*
 * Definitions for DP8570 real-time clock.
 */

typedef unsigned char	uchar;

struct rtc {
	uchar	msr;
	uchar	t0ctrl;
#define rtm	t0ctrl
	uchar	t1ctrl;
#define omr	t1ctrl
	uchar	pfr;
#define ictrl0	pfr
	uchar	irout;
#define ictrl1	irout
	uchar	hunths;
	uchar	secs;
	uchar	mins;
	uchar	hours;
	uchar	days;
	uchar	months;
	uchar	years;
	uchar	jul_lo;
	uchar	jul_hi;
	uchar	dayweek;
	uchar	t0lsb;
	uchar	t0msb;
	uchar	t1lsb;
	uchar	t1msb;
	uchar	ram[13];
};

#define	RTC_PHYS_ADDR	0xE40000
#define EXT_FREQ	1041667		/* Hz */

/* Bits in MSR */
#define PAGE_SEL	0x80
#define REG_SEL		0x40
#define T1_INT		0x20
#define T0_INT		0x10
#define ALARM_INT	8
#define PERIOD_INT	4
#define PFAIL_INT	2
#define INTR		1

/* Bits in PFR */
#define TEST_MODE	0x80
#define OSC_FAIL	0x40
#define SING_SUPPLY	0x40
#define PER_MILLI	0x20
#define PER_10MIL	0x10
#define PER_100MIL	8
#define PER_SECS	4
#define PER_10SEC	2
#define PER_MINS	1

/* Bits in RTM */
#define XTAL_32768	0
#define XTAL_4194K	0x40
#define XTAL_4915K	0x80
#define XTAL_32000	0xC0
#define TIMER_PF	0x20
#define INTR_PF		0x10
#define START		8
#define TWELVE_HR	4
#define LEAP_CTR	3

/* Bits in IROUT */
#define TIME_SAVE	0x80
#define LOW_BATTERY	0x40
#define PFAIL_DELAY	0x20
#define T1_ROUTE	0x10
#define	T0_ROUTE	8
#define ALARM_ROUTE	4
#define PERIOD_ROUTE	2
#define PFAIL_ROUTE	1

/* Bits in t0ctrl and t1ctrl */
#define COUNT_HOLD	0x80
#define TIMER_READ	0x40
#define TIMER_CLOCK	0x38
#define TCLK_EXT	0
#define TCLK_XTAL	8
#define TCLK_XTAL_4	0x10
#define TCLK_10_7K	0x18
#define TCLK_1K		0x20
#define TCLK_100	0x28
#define TCLK_10		0x30
#define TCLK_1		0x38
#define TIMER_MODE	6
#define TMODE_SINGLE	0
#define TMODE_RATE	2
#define TMODE_SQUARE	4
#define TMODE_RETRIG	6
#define TIMER_START	1

/* Bits in ictrl0 */
#define T1_INTEN	0x80
#define T0_INTEN	0x40
/* other bits as in PFR */

/* Bits in ictrl1 */
#define PFAIL_INTEN	0x80
#define ALARM_INTEN	0x40
#define WDAY_COMPARE	0x20
#define MONTH_COMPARE	0x10
#define MDAY_COMPARE	8
#define HOUR_COMPARE	4
#define MINUTE_COMPARE	2
#define SECOND_COMPARE	1

/*
 * Definitions for converting between seconds since the Epoch
 * and year, month, day, hour, minute, second.
 */
struct rtc_tm {
	int	tm_sec;		/* Second 0--59 */
	int	tm_min;		/* Minute 0--59 */
	int	tm_hour;	/* Hour 0--23 */
	int	tm_mday;	/* Day of month 1--31 */
	int	tm_mon;		/* Month 1--12 */
	int	tm_year;	/* Year */
	int	tm_yday;	/* Julian day of year 1--365 */
	int	tm_wday;	/* Day of week 1--7 */
};

#define FEBRUARY	2
#define	STARTOFTIME	1970	/* Epoch was 1 Jan 1970 */
#define START_DAY	4	/* which was a Thursday */
#define SECDAY		86400L
#define SECYR		(SECDAY * 365)

#define	leapyear(year)		((year) % 4 == 0)
#define	range_test(n, l, h)	if ((n) < (l) || (n) > (h)) return(0)
#define	days_in_year(a) 	(leapyear(a) ? 366 : 365)
#define	days_in_month(a) 	(month_days[(a) - 1])
