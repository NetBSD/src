#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && (defined(PARSE) || defined(PARSEPPS)) && defined(CLOCK_COMPUTIME)
/*
 * clk_computime.c,v 1.8 1997/01/19 12:44:35 kardel Exp
 * 
 * Supports Diem's Computime Radio Clock
 * 
 * Used the Meinberg clock as a template for Diem's Computime Radio Clock
 *
 * adapted by Alois Camenzind <alois.camenzind@ubs.ch>
 * 
 * Copyright (C) 1992-1996 by Frank Kardel
 * Friedrich-Alexander Universität Erlangen-Nürnberg, Germany
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */


#include "sys/types.h"
#include "sys/time.h"
#include "sys/errno.h"
#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_calendar.h"


#include "parse.h"

/*
 * The Computime receiver sends a datagram in the following format every minute
 * 
 * Timestamp	T:YY:MM:MD:WD:HH:MM:SSCRLF 
 * Pos          0123456789012345678901 2 3
 *              0000000000111111111122 2 2
 * Parse        T:  :  :  :  :  :  :  rn
 * 
 * T	Startcharacter "T" specifies start of the timestamp 
 * YY	Year MM	Month 1-12 
 * MD	Day of the month 
 * WD	Day of week 
 * HH	Hour 
 * MM   Minute 
 * SS   Second
 * CR   Carriage return 
 * LF   Linefeed
 * 
 */

static struct format computime_fmt =
{
	{
		{8, 2},  {5,  2}, {2,  2},	/* day, month, year */
		{14, 2}, {17, 2}, {20, 2},	/* hour, minute, second */
		{11, 2},                        /* dayofweek,  */
	},
	"T:  :  :  :  :  :  :  \r\n",
	0
};

static u_long   cvt_computime();

clockformat_t   clock_computime =
{
	(unsigned long (*) ()) 0,	/* no input handling */
	cvt_computime,			/* Computime conversion */
	syn_simple,			/* easy time stamps for RS232 (fallback) */
	(u_long (*)())0,		/* no PPS monitoring */
	(u_long(*) ())0,		/* no time code synthesizer monitoring */
	(void *)&computime_fmt,		/* conversion configuration */
	"Diem's Computime Radio Clock",	/* Computime Radio Clock */
	24,				/* string buffer */
	F_START|F_END|SYNC_START,	/* START/END delimiter, START synchronisation */
	0,				/* no private data (complete pakets) */
	{0, 0},
	'T',
	'\n',
	'0'
};

/*
 * cvt_computime
 * 
 * convert simple type format
 */
static          u_long
cvt_computime(buffer, size, format, clock)
	register char  *buffer;
	register int    size;
	register struct format *format;
	register clocktime_t *clock;
{

	if (!Strok(buffer, format->fixed_string)) { 
		return CVT_NONE;
	} else {
		if (Stoi(&buffer[format->field_offsets[O_DAY].offset], &clock->day,
				format->field_offsets[O_DAY].length) ||
			Stoi(&buffer[format->field_offsets[O_MONTH].offset], &clock->month,
				format->field_offsets[O_MONTH].length) ||
			Stoi(&buffer[format->field_offsets[O_YEAR].offset], &clock->year,
				format->field_offsets[O_YEAR].length) ||
			Stoi(&buffer[format->field_offsets[O_HOUR].offset], &clock->hour,
				format->field_offsets[O_HOUR].length) ||
			Stoi(&buffer[format->field_offsets[O_MIN].offset], &clock->minute,
				format->field_offsets[O_MIN].length) ||
			Stoi(&buffer[format->field_offsets[O_SEC].offset], &clock->second,
				format->field_offsets[O_SEC].length)) { 
			return CVT_FAIL | CVT_BADFMT;
		} else {

			clock->flags = 0;
			clock->utcoffset = 0;	/* We have UTC time */

			return CVT_OK;
		}
	}
}

#else /* not (REFCLOCK && (PARSE || PARSEPPS) && CLOCK_COMPUTIME) */
int clk_computime_bs;
#endif /* not (REFCLOCK && (PARSE || PARSEPPS) && CLOCK_COMPUTIME) */

/*
 * clk_computime.c,v
 * Revision 1.8  1997/01/19 12:44:35  kardel
 * 3-5.88.1 reconcilation
 *
 * Revision 1.7  1996/12/01 16:04:12  kardel
 * freeze for 5.86.12.2 PARSE-Patch
 *
 * Revision 1.6  1996/12/01 12:57:26  kardel
 * more standard string escapes
 *
 * Revision 1.5  1996/11/24 23:16:30  kardel
 * checkpoint - partial autoconfigure update for parse modules
 *
 * Revision 1.4  1996/11/24 17:34:43  kardel
 * updated copyright
 *
 * Revision 1.3  1996/11/16 19:44:36  kardel
 * Log entry added
 *
 */
