/*
 *
 * /src/NTP/REPOSITORY/v4/libparse/clk_hopf6021.c,v 3.5 1997/01/19 12:44:37 kardel Exp
 *
 * clk_hopf6021.c,v 3.5 1997/01/19 12:44:37 kardel Exp
 *
 * Radiocode Clocks HOPF Funkuhr 6021 mit serieller Schnittstelle
 * base code version from 24th Nov 1995 - history at end
 *
 * Created by F.Schnekenbuehl <frank@comsys.dofn.de> from clk_rcc8000.c
 * Nortel DASA Network Systems GmbH, Department: ND250
 * A Joint venture of Daimler-Benz Aerospace and Nortel
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && (defined(PARSE) || defined(PARSEPPS)) && defined(CLOCK_HOPF6021)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/errno.h>
#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_calendar.h"

#include "parse.h"

/* 
 * hopf Funkuhr 6021 
 *      used with 9600,8N1,
 *      UTC ueber serielle Schnittstelle 
 *      Sekundenvorlauf ON
 *      ETX zum Sekundenvorlauf ON
 *      Datenstring 6021
 *      Ausgabe Uhrzeit und Datum
 *      Senden mit Steuerzeichen
 *      Senden sekuendlich
 */

/*
 *  Type 6021 Serial Output format
 *
 *      000000000011111111 / char
 *      012345678901234567 \ position
 *      sABHHMMSSDDMMYYnre  Actual
 *       C4110046231195     Parse
 *      s              enr  Check
 *
 *  s = STX (0x02), e = ETX (0x03)
 *  n = NL  (0x0A), r = CR  (0x0D)
 *
 *  A B - Status and weekday
 *
 *  A - Status
 *
 *      8 4 2 1
 *      x x x 0  - no announcement
 *      x x x 1  - Summertime - wintertime - summertime announcement
 *      x x 0 x  - Wintertime
 *      x x 1 x  - Summertime
 *      0 0 x x  - Time/Date invalid
 *      0 1 x x  - Internal clock used 
 *      1 0 x x  - Radio clock
 *      1 1 x x  - Radio clock highprecision
 *
 *  B - 8 4 2 1
 *      0 x x x  - MESZ/MEZ
 *      1 x x x  - UTC
 *      x 0 0 1  - Monday
 *      x 0 1 0  - Tuesday
 *      x 0 1 1  - Wednesday
 *      x 1 0 0  - Thursday
 *      x 1 0 1  - Friday
 *      x 1 1 0  - Saturday
 *      x 1 1 1  - Sunday
 */

#define HOPF_DSTWARN	0x01	/* DST switch warning */
#define HOPF_DST	0x02	/* DST in effect */

#define HOPF_MODE	0x0C	/* operation mode mask */	
#define  HOPF_INVALID	0x00	/* no time code available */
#define  HOPF_INTERNAL	0x04	/* internal clock */
#define  HOPF_RADIO	0x08	/* radio clock */	
#define  HOPF_RADIOHP	0x0C	/* high precision radio clock */

#define HOPF_UTC	0x08	/* time code in UTC */
#define HOPF_WMASK	0x07	/* mask for weekday code */

static struct format hopf6021_fmt =
{
    {
        {  9, 2 }, {11, 2}, { 13, 2}, /* Day, Month, Year */ 
        {  3, 2 }, { 5, 2}, {  7, 2}, /* Hour, Minute, Second */ 
	{  2, 1 }, { 1, 1}, {  0, 0}, /* Weekday, Flags, Zone */
	/* ... */
    },
    "\002              \n\r\003",
    0 
};

#define OFFS(x) format->field_offsets[(x)].offset
#define STOI(x, y) Stoi(&buffer[OFFS(x)], y, format->field_offsets[(x)].length)
#define hexval(x) (('0' <= (x) && (x) <= '9') ? (x) - '0' : \
    ('a' <= (x) && (x) <= 'f') ? (x) - 'a' + 10 : \
    ('A' <= (x) && (x) <= 'F') ? (x) - 'A' + 10 : \
    -1)
            
static unsigned long cvt_hopf6021();

clockformat_t clock_hopf6021 =
{
  (unsigned long (*)())0,       /* no input handling */
  cvt_hopf6021,                 /* Radiocode clock conversion */
  syn_simple,                   /* easy time stamps for RS232 (fallback) */
  (unsigned long (*)())0,       /* no direct PPS monitoring */
  (unsigned long (*)())0,       /* no time code synthesizer monitoring */
  (void *)&hopf6021_fmt,        /* conversion configuration */
  "hopf Funkuhr 6021",          /* clock format name */
  19,                           /* string buffer */
  F_END|SYNC_END,               /* END delimiter, END synchronisation */
  0,                            /* private data length, no private data */
  { 0, 0},                      /* buffer restart after timeout (us) */
  0,                            /* start symbol */
  '\003',                       /* end symbol */
  0                             /* sync symbol */
};

static unsigned long
cvt_hopf6021(buffer, size, format, clock)
    register char          *buffer;
    register int            size;
    register struct format *format;
    register clocktime_t   *clock;
{
    char status,weekday;

    if (!Strok(buffer, format->fixed_string))
    {
        return CVT_NONE;
    }

    if (  STOI(O_DAY,   &clock->day)    ||
          STOI(O_MONTH, &clock->month)  ||
          STOI(O_YEAR,  &clock->year)   ||
          STOI(O_HOUR,  &clock->hour)   ||
          STOI(O_MIN,   &clock->minute) ||
          STOI(O_SEC,   &clock->second)
       )
    {
        return CVT_FAIL|CVT_BADFMT;
    }

    clock->usecond   = 0;
    clock->utcoffset = 0;

    status = hexval(buffer[OFFS(O_FLAGS)]);
    weekday= hexval(buffer[OFFS(O_WDAY)]);

    if ((status == -1) || (weekday == -1))
    {
        return CVT_FAIL|CVT_BADFMT;
    }

    clock->flags  = 0;

    if (weekday & HOPF_UTC)
    {
        clock->flags |= PARSEB_UTC;
    }
    else
    {
	if (status & HOPF_DST)
	{
            clock->flags     |= PARSEB_DST;
	    clock->utcoffset  = -2*60*60; /* MET DST */
	}
	else
	{
	    clock->utcoffset  = -1*60*60; /* MET */
	}
    }

    clock->flags |= (status & HOPF_DSTWARN)  ? PARSEB_ANNOUNCE : 0;

    switch (status & HOPF_MODE)
    {
        case HOPF_INVALID:  /* Time/Date invalid */
            clock->flags |= PARSEB_POWERUP;
            break;

        case HOPF_INTERNAL: /* internal clock */
            clock->flags |= PARSEB_NOSYNC;
            break;

        case HOPF_RADIO:    /* Radio clock */
        case HOPF_RADIOHP:  /* Radio clock high precision */
            break;

        default:
            return CVT_FAIL|CVT_BADFMT;
            break;
    }

    return CVT_OK;
}

#else /* not (REFCLOCK && (PARSE || PARSEPPS) && CLOCK_HOPF6021) */
int clk_hopf6021_bs;
#endif /* not (REFCLOCK && (PARSE || PARSEPPS) && CLOCK_HOPF6021) */

/*
 * History:
 *
 * clk_hopf6021.c,v
 * Revision 3.5  1997/01/19 12:44:37  kardel
 * 3-5.88.1 reconcilation
 *
 * Revision 3.4  1996/11/24 20:09:43  kardel
 * RELEASE_5_86_12_2 reconcilation
 *
 * Revision 3.3  1995/12/17 18:17:21  kardel
 * log entries fixed
 *
 * Revision 3.2  1995/12/17  18:11:46  kardel
 * code cleanup and improved state handling
 *
 * Revision 3.1  1995/12/17  18:08:53  kardel
 * Hopf 6021 added - base code
 *
 */
