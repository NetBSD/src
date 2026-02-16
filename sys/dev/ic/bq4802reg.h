/*	$NetBSD: bq4802reg.h,v 1.2 2026/02/16 16:29:59 jdc Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 * Registers and register values taken from the Texas Instruments
 * bq4802Y/bq4802LY data sheet 2002.
 */

#define	BQ4802_SEC		0x00	/* Seconds */
#define	BQ4802_SEC_ALM		0x01	/* Seconds alarm */
#define	BQ4802_MIN		0x02	/* Minutes */
#define	BQ4802_MIN_ALM		0x03	/* Minutes alarm */
#define	BQ4802_HOUR		0x04	/* Hours */
#define	BQ4802_HOUR_ALM		0x05	/* Hours alarm */
#define	BQ4802_DAY		0x06	/* Day */
#define	BQ4802_DAY_ALM		0x07	/* Day alarm */
#define	BQ4802_WDAY		0x08	/* Day of week */
#define	BQ4802_MONTH		0x09	/* Month */
#define	BQ4802_YEAR		0x0a	/* Year */
#define	BQ4802_RATES		0x0b	/* Rates */
#define	BQ4802_ENABLES		0x0c	/* Enables */
#define	BQ4802_FLAGS		0x0d	/* Flags */
#define	BQ4802_CTRL		0x0e	/* Control */
#define	BQ4802_CENT		0x0f	/* Century */

/* Seconds and Seconds alarm (00-59) */
#define	BQ4802_SECS_1		0x0f	/* 1-second digit */
#define	BQ4802_SECS_10		0x70	/* 10-second digit */
#define	BQ4802_SECS_ALM0	0x40	/* seconds alarm mask 0 */
#define	BQ4802_SECS_ALM1	0x80	/* seconds alarm mask 1 */

/* Minutes and Minutes alarm (00-59) */
#define	BQ4802_MINS_1		0x0f	/* 1-minute digit */
#define	BQ4802_MINS_10		0x70	/* 10-minute digit */
#define	BQ4802_MINS_ALM0	0x40	/* minutes alarm mask 0 */
#define	BQ4802_MINS_ALM1	0x80	/* minutes alarm mask 1 */

/* Hours and Hours alarm (01-24 or 01-12 AM / 81-92 PM) */
#define	BQ4802_HOURS_1		0x0f	/* 1-hour digit */
#define	BQ4802_HOURS_10		0x30	/* 10-hour digit */
#define	BQ4802_HOURS_AM		0x00	/* AM/PM bit */
#define	BQ4802_HOURS_PM		0x80	/* AM/PM bit */
#define	BQ4802_HOURS_ALM0	0x40	/* hours alarm mask 0 */
#define	BQ4802_HOURS_ALM1	0x80	/* hours alarm mask 1 */

/* Days and Days alarm (01-31) */
#define	BQ4802_DAYS_1		0x0f	/* 1-day digit */
#define	BQ4802_DAYS_10		0x30	/* 10-day digit */
#define	BQ4802_DAYS_ALM0	0x40	/* days alarm mask 0 */
#define	BQ4802_DAYS_ALM1	0x80	/* days alarm mask 1 */

/* Day of week (01-07) */
#define	BQ4802_WDAYS_1		0x07	/* day of week digit */

/* Month (01-12) */
#define	BQ4802_MONTHS_1		0x0f	/* 1-month digit */
#define	BQ4802_MONTHS_10	0x30	/* 10-month digit */

/* Year (00-99) */
#define	BQ4802_YEARS_1		0x0f	/* 1-year digit */
#define	BQ4802_YEARS_10		0xf0	/* 10-year digit */

/* Century (00-99) */
#define	BQ4802_CENTS_1		0x0f	/* 1-century digit */
#define	BQ4802_CENTS_10		0xf0	/* 10-century digit */

/* Rates */
#define	BQ4802_RATE_RS0	0x01	/* RS0 */
#define	BQ4802_RATE_RS1	0x02	/* RS1 */
#define	BQ4802_RATE_RS2	0x04	/* RS2 */
#define	BQ4802_RATE_RS3	0x08	/* RS3 */
#define	BQ4802_RATE_WD0	0x10	/* WD0 */
#define	BQ4802_RATE_WD1	0x20	/* WD1 */
#define	BQ4802_RATE_WD2	0x40	/* WD2 */

/* Enables */
#define	BQ4802_EN_ABE	0x01	/* Alarm enable in battery-backup mode */
#define	BQ4802_EN_PWRIE	0x02	/* Power-fail interrupt enable */
#define	BQ4802_EN_PIE	0x04	/* Periodic interrupt enable */
#define	BQ4802_EN_AIE	0x08	/* Alarm interrupt enable */

/* Flags */
#define	BQ4802_FLG_BVF	0x01	/* Battery-valid flag */
#define	BQ4802_FLG_PWRF	0x02	/* Power-fail interrupt flag */
#define	BQ4802_FLG_PF	0x04	/* Periodic interrupt flag */
#define	BQ4802_FLG_AF	0x08	/* Alarm interrupt flag */

/* Control */
#define	BQ4802_CTRL_DSE	0x01	/* Daylight savings enable */
#define	BQ4802_CTRL_24	0x02	/* 1 for 24-hour, 0 for 12-hour */
#define	BQ4802_CTRL_STP	0x04	/* 0 = stop when in battery-backup mode */
#define	BQ4802_CTRL_UTI	0x08	/* Update transfer inhibit */

/* Max number of registers we need to read/write for the TOD. */
#define	BQ4802_NREGS	0x10
typedef uint8_t bq4802_regs[BQ4802_NREGS];	/* sparse */
static int bq4802_tods[BQ4802_NREGS] =		/* TOD registers */
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1 };

/* Register read and write functions -- machine-dependent. */
uint8_t bq4802_read(void *, uint8_t);
void bq4802_write(void *, uint8_t, uint8_t);

/*
 * Get all of the TOD registers.
 * Called at splclock(), and with the RTC set up.
 */
#define	BQ4802_GETTOD(sc, regs)						\
	do {								\
		int i;							\
		uint8_t ctrl;						\
									\
		/* disable register updates */				\
		ctrl = bq4802_read(sc, BQ4802_CTRL);			\
		bq4802_write(sc, BQ4802_CTRL, ctrl | BQ4802_CTRL_UTI);	\
									\
		/* read the TOD registers */				\
		for (i = 0; i < BQ4802_NREGS; i++) 			\
			if (bq4802_tods[i])				\
				(*regs)[i] = bq4802_read(sc, i);	\
									\
		/* re-enable register updates */			\
		bq4802_write(sc, BQ4802_CTRL, ctrl);			\
	} while (0);

/*
 * Set all of the TOD registers.
 * Called at splclock(), and with the RTC set up.
 */
#define	BQ4802_SETTOD(sc, regs)						\
	do {								\
		int i;							\
		uint8_t ctrl;						\
									\
		/* disable register updates */				\
		ctrl = bq4802_read(sc, BQ4802_CTRL);			\
		bq4802_write(sc, BQ4802_CTRL, ctrl | BQ4802_CTRL_UTI);	\
									\
		/* write the TOD registers */				\
		for (i = 0; i < BQ4802_NREGS; i++) 			\
			if (bq4802_tods[i])				\
				bq4802_write(sc, i, (*regs)[i]);	\
									\
		/* re-enable register updates */			\
		bq4802_write(sc, BQ4802_CTRL, ctrl);			\
	} while (0);
