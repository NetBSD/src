%{
/*
**  Originally written by Steven M. Bellovin <smb@research.att.com> while
**  at the University of North Carolina at Chapel Hill.  Later tweaked by
**  a couple of people on Usenet.  Completely overhauled by Rich $alz
**  <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990;
**
**  This grammar has 10 shift/reduce conflicts.
**
**  This code is in the public domain and has no copyright.
*/
/* SUPPRESS 287 on yaccpar_sccsid *//* Unused static variable */
/* SUPPRESS 288 on yyerrlab *//* Label unused */

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: parsedate.y,v 1.37 2022/04/23 13:02:04 christos Exp $");
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <util.h>
#include <stdlib.h>

/* NOTES on rebuilding parsedate.c (particularly for inclusion in CVS
   releases):

   We don't want to mess with all the portability hassles of alloca.
   In particular, most (all?) versions of bison will use alloca in
   their parser.  If bison works on your system (e.g. it should work
   with gcc), then go ahead and use it, but the more general solution
   is to use byacc instead of bison, which should generate a portable
   parser.  I played with adding "#define alloca dont_use_alloca", to
   give an error if the parser generator uses alloca (and thus detect
   unportable parsedate.c's), but that seems to cause as many problems
   as it solves.  */

#define EPOCH		1970
#define HOUR(x)		((time_t)((x) * 60))
#define SECSPERDAY	(24L * 60L * 60L)

#define	MAXREL	16	/* hours mins secs days weeks months years - maybe twice each ...*/

#define USE_LOCAL_TIME	99999 /* special case for Convert() and yyTimezone */

/*
**  An entry in the lexical lookup table.
*/
typedef struct _TABLE {
    const char	*name;
    int		type;
    time_t	value;
} TABLE;


/*
**  Daylight-savings mode:  on, off, or not yet known.
*/
typedef enum _DSTMODE {
    DSTon, DSToff, DSTmaybe
} DSTMODE;

/*
**  Meridian:  am, pm, or 24-hour style (plus "noon" and "midnight").
*/
typedef enum _MERIDIAN {
    MERam, MERpm, MER24, MER_NOON, MER_MN
} MERIDIAN;


struct dateinfo {
	DSTMODE	yyDSTmode;	/* DST on/off/maybe */
	time_t	yyDayOrdinal;
	time_t	yyDayNumber;
	int	yyHaveDate;
	int	yyHaveFullYear;	/* if true, year is not abbreviated. */
				/* if false, need to call AdjustYear(). */
	int	yyHaveDay;
	int	yyHaveRel;
	int	yyHaveTime;
	int	yyHaveZone;
	time_t	yyTimezone;	/* Timezone as minutes ahead/east of UTC */
	time_t	yyDay;		/* Day of month [1-31] */
	time_t	yyHour;		/* Hour of day [0-24] or [1-12] */
	time_t	yyMinutes;	/* Minute of hour [0-59] */
	time_t	yyMonth;	/* Month of year [1-12] */
	time_t	yySeconds;	/* Second of minute [0-60] */
	time_t	yyYear;		/* Year, see also yyHaveFullYear */
	MERIDIAN yyMeridian;	/* Interpret yyHour as AM/PM/24 hour clock */
	struct {
		time_t	yyRelVal;
		int	yyRelMonth;
	} yyRel[MAXREL];
};

static int RelVal(struct dateinfo *, time_t, time_t, int, int);

#define CheckRelVal(a, b, c, d, e) do {				\
		if (!RelVal((a), (b), (c), (d), (e))) {		\
			YYREJECT;				\
		}						\
	} while (0)

%}

%union {
    time_t		Number;
    enum _MERIDIAN	Meridian;
}

%token	tAGO tDAY tDAYZONE tID tMERIDIAN tMINUTE_UNIT tMONTH tMONTH_UNIT
%token	tSEC_UNIT tSNUMBER tUNUMBER tZONE tDST AT_SIGN tTIME

%type	<Number>	tDAY tDAYZONE tMINUTE_UNIT tMONTH tMONTH_UNIT
%type	<Number>	tSEC_UNIT tSNUMBER tUNUMBER tZONE tTIME
%type	<Meridian>	tMERIDIAN

%type	<Number>	at_number
%type	<Meridian>	o_merid

%parse-param	{ struct dateinfo *param }
%parse-param 	{ const char **yyInput }
%lex-param	{ const char **yyInput }
%pure-parser

%%

spec:
	  /* empty */
	| spec item
;

item:
	  time			{ param->yyHaveTime++; }
	| time_numericzone	{ param->yyHaveTime++; param->yyHaveZone++; }
	| zone			{ param->yyHaveZone++; }
	| date			{ param->yyHaveDate++; }
	| day			{ param->yyHaveDay++; }
	| rel			{ param->yyHaveRel++; }
	| cvsstamp		{ param->yyHaveTime++; param->yyHaveDate++;
				  param->yyHaveZone++; }
	| epochdate		{ param->yyHaveTime++; param->yyHaveDate++; 
				  param->yyHaveZone++; }
	| number
;

cvsstamp:
	tUNUMBER '.' tUNUMBER '.' tUNUMBER '.' 
				tUNUMBER '.' tUNUMBER '.' tUNUMBER {
		param->yyYear = $1;
		if (param->yyYear < 100) {
			param->yyYear += 1900;
		}
		param->yyHaveFullYear = 1;
		param->yyMonth = $3;
		param->yyDay = $5;
		param->yyHour = $7;
		param->yyMinutes = $9;
		param->yySeconds = $11;
		param->yyDSTmode = DSToff;
		param->yyTimezone = 0;
	}
;

epochdate:
	AT_SIGN at_number {
		time_t	when = $2;
		struct tm tmbuf;

		if (gmtime_r(&when, &tmbuf) != NULL) {
			param->yyYear = tmbuf.tm_year + 1900;
			param->yyMonth = tmbuf.tm_mon + 1;
			param->yyDay = tmbuf.tm_mday;

			param->yyHour = tmbuf.tm_hour;
			param->yyMinutes = tmbuf.tm_min;
			param->yySeconds = tmbuf.tm_sec;
		} else {
			param->yyYear = EPOCH;
			param->yyMonth = 1;
			param->yyDay = 1;

			param->yyHour = 0;
			param->yyMinutes = 0;
			param->yySeconds = 0;
		}
		param->yyHaveFullYear = 1;
		param->yyDSTmode = DSToff;
		param->yyTimezone = 0;
	}
;

at_number:
	  tUNUMBER
	| tSNUMBER
;

time:
	  tUNUMBER tMERIDIAN {
		if ($1 > 24)
			YYREJECT;
		param->yyMinutes = 0;
		param->yySeconds = 0;
		if ($2 == MER_NOON || $2 == MER_MN) {
			if ($1 == 12) {
				switch ($2) {
				case MER_NOON: param->yyHour = 12; break;
				case MER_MN  : param->yyHour = 0;  break;
				default:	/* impossible */;  break;
				}
				param->yyMeridian = MER24;
			} else
				YYREJECT;
		} else {
			param->yyHour = $1;
			param->yyMeridian = $2;
		}
	  }
	| tUNUMBER ':' tUNUMBER o_merid {
		if ($1 > 24 || $3 >= 60)
			YYREJECT;
		param->yyMinutes = $3;
		param->yySeconds = 0;
		if ($4 == MER_NOON || $4 == MER_MN) {
			if ($1 == 12 && $3 == 0) {
				switch ($4) {
				case MER_NOON: param->yyHour = 12; break;
				case MER_MN  : param->yyHour = 0;  break;
				default:	/* impossible */;  break;
				}
				param->yyMeridian = MER24;
			} else
				YYREJECT;
		} else {
			param->yyHour = $1;
			param->yyMeridian = $4;
		}
	  }
	| tUNUMBER ':' tUNUMBER ':' tUNUMBER o_merid {
		if ($1 > 24 || $3 >= 60 || $5 > 60)
			YYREJECT;
		param->yyMinutes = $3;
		param->yySeconds = $5;
		if ($6 == MER_NOON || $6 == MER_MN) {
			if ($1 == 12 && $3 == 0 && $5 == 0) {
				switch ($6) {
				case MER_NOON: param->yyHour = 12; break;
				case MER_MN  : param->yyHour = 0;  break;
				default:	/* impossible */;  break;
				}
				param->yyMeridian = MER24;
			} else
				YYREJECT;
		} else {
			param->yyHour = $1;
			param->yyMeridian = $6;
		}
	  }
	| tUNUMBER ':' tUNUMBER ':' tUNUMBER '.' tUNUMBER {
		if ($1 > 24 || $3 >= 60 || $5 > 60)
			YYREJECT;
		param->yyHour = $1;
		param->yyMinutes = $3;
		param->yySeconds = $5;
		param->yyMeridian = MER24;
		/* XXX: Do nothing with fractional secs ($7) */
	  }
	| tUNUMBER ':' tUNUMBER ':' tUNUMBER ',' tUNUMBER {
		if ($1 > 24 || $3 >= 60 || $5 > 60)
			YYREJECT;
		param->yyHour = $1;
		param->yyMinutes = $3;
		param->yySeconds = $5;
		param->yyMeridian = MER24;
		/* XXX: Do nothing with fractional seconds ($7) */
	  }
	| tTIME {
		param->yyHour = $1;
		param->yyMinutes = 0;
		param->yySeconds = 0;
		param->yyMeridian = MER24;
		/* Tues midnight --> Weds 00:00, midnight Tues -> Tues 00:00 */
		if ($1 == 0 && param->yyHaveDay)
			param->yyDayNumber++;
	  }
	| tUNUMBER tTIME {
		if ($1 == 12 && ($2 == 0 || $2 == 12)) {
			param->yyHour = $2;
			param->yyMinutes = 0;
			param->yySeconds = 0;
			param->yyMeridian = MER24;
		} else
			YYREJECT;
	  }
;

time_numericzone:
	  tUNUMBER ':' tUNUMBER tSNUMBER {
		if ($4 < -(47 * 100 + 59) || $4 > (47 * 100 + 59))
			YYREJECT;
		if ($1 > 24 || $3 > 59)
			YYREJECT;
		param->yyHour = $1;
		param->yyMinutes = $3;
		param->yyMeridian = MER24;
		param->yyDSTmode = DSToff;
		param->yyTimezone = - ($4 % 100 + ($4 / 100) * 60);
	  }
	| tUNUMBER ':' tUNUMBER ':' tUNUMBER tSNUMBER {
		if ($6 < -(47 * 100 + 59) || $6 > (47 * 100 + 59))
			YYREJECT;
		if ($1 > 24 || $3 > 59 || $5 > 60)
			YYREJECT;
		param->yyHour = $1;
		param->yyMinutes = $3;
		param->yySeconds = $5;
		param->yyMeridian = MER24;
		param->yyDSTmode = DSToff;
		param->yyTimezone = - ($6 % 100 + ($6 / 100) * 60);
	}
;

zone:
	  tZONE		{ param->yyTimezone = $1; param->yyDSTmode = DSToff; }
	| tDAYZONE	{ param->yyTimezone = $1; param->yyDSTmode = DSTon; }
	| tZONE tDST	{ param->yyTimezone = $1; param->yyDSTmode = DSTon; }
	| tSNUMBER	{
			  if (param->yyHaveDate == 0 && param->yyHaveTime == 0)
				YYREJECT;
			  if ($1 < -(47 * 100 + 59) || $1 > (47 * 100 + 59))
				YYREJECT;
			  param->yyTimezone = - ($1 % 100 + ($1 / 100) * 60);
			  param->yyDSTmode = DSTmaybe;
			}
;

day:
	  tDAY		{ param->yyDayOrdinal = 1; param->yyDayNumber = $1; }
	| tDAY ','	{ param->yyDayOrdinal = 1; param->yyDayNumber = $1; }
	| tUNUMBER tDAY	{ param->yyDayOrdinal = $1; param->yyDayNumber = $2; }
;

date:
	  tUNUMBER '/' tUNUMBER {
		if ($1 > 12 || $3 > 31 || $1 == 0 || $3 == 0)
			YYREJECT;
		param->yyMonth = $1;
		param->yyDay = $3;
	  }
	| tUNUMBER '/' tUNUMBER '/' tUNUMBER {
		if ($1 >= 100) {
			if ($3 > 12 || $5 > 31 || $3 == 0 || $5 == 0)
				YYREJECT;
			param->yyYear = $1;
			param->yyMonth = $3;
			param->yyDay = $5;
		} else {
			if ($1 > 12 || $3 > 31 || $1 == 0 || $3 == 0)
				YYREJECT;
			param->yyMonth = $1;
			param->yyDay = $3;
			param->yyYear = $5;
		}
	  }
	| tUNUMBER tSNUMBER tSNUMBER {
		/* ISO 8601 format.  yyyy-mm-dd.  */
		if ($2 >= 0 || $2 < -12 || $3 >= 0 || $3 < -31)
			YYREJECT;
		param->yyYear = $1;
		param->yyHaveFullYear = 1;
		param->yyMonth = -$2;
		param->yyDay = -$3;
	  }
	| tUNUMBER tMONTH tSNUMBER {
		if ($3 > 0 || $1 == 0 || $1 > 31)
			YYREJECT;
		/* e.g. 17-JUN-1992.  */
		param->yyDay = $1;
		param->yyMonth = $2;
		param->yyYear = -$3;
	  }
	| tMONTH tUNUMBER {
		if ($2 == 0 || $2 > 31)
			YYREJECT;
		param->yyMonth = $1;
		param->yyDay = $2;
	  }
	| tMONTH tUNUMBER ',' tUNUMBER {
		if ($2 == 0 || $2 > 31)
			YYREJECT;
		param->yyMonth = $1;
		param->yyDay = $2;
		param->yyYear = $4;
	  }
	| tUNUMBER tMONTH {
		if ($1 == 0 || $1 > 31)
			YYREJECT;
		param->yyMonth = $2;
		param->yyDay = $1;
	  }
	| tUNUMBER tMONTH tUNUMBER {
		if ($1 > 31 && $3 > 31)
			YYREJECT;
		if ($1 < 35) {
			if ($1 == 0)
				YYREJECT;
			param->yyDay = $1;
			param->yyYear = $3;
		} else {
			if ($3 == 0)
				YYREJECT;
			param->yyDay = $3;
			param->yyYear = $1;
		}
		param->yyMonth = $2;
	  }
;

rel:
	  relunit
	| relunit tAGO {
		param->yyRel[param->yyHaveRel].yyRelVal =
		    -param->yyRel[param->yyHaveRel].yyRelVal;
	  }
;

relunit:
	  tUNUMBER tMINUTE_UNIT	{ CheckRelVal(param, $1, $2, 60, 0); }
	| tSNUMBER tMINUTE_UNIT	{ CheckRelVal(param, $1, $2, 60, 0); }
	| tMINUTE_UNIT		{ CheckRelVal(param,  1, $1, 60, 0); }
	| tSNUMBER tSEC_UNIT	{ CheckRelVal(param, $1, 1,  1,  0); }
	| tUNUMBER tSEC_UNIT	{ CheckRelVal(param, $1, 1,  1,  0); }
	| tSEC_UNIT		{ CheckRelVal(param,  1, 1,  1,  0); }
	| tSNUMBER tMONTH_UNIT	{ CheckRelVal(param, $1, $2, 1,  1); }
	| tUNUMBER tMONTH_UNIT	{ CheckRelVal(param, $1, $2, 1,  1); }
	| tMONTH_UNIT		{ CheckRelVal(param,  1, $1, 1,  1); }
;

number:
	tUNUMBER {
		if (param->yyHaveTime && param->yyHaveDate &&
		    !param->yyHaveRel) {
			param->yyYear = $1;
		} else {
			if ($1 > 10000) {
				param->yyHaveDate++;
				param->yyDay = ($1)%100;
				param->yyMonth = ($1/100)%100;
				param->yyYear = $1/10000;
			}
			else {
				param->yyHaveTime++;
				if ($1 < 100) {
					param->yyHour = $1;
					param->yyMinutes = 0;
				}
				else {
					param->yyHour = $1 / 100;
					param->yyMinutes = $1 % 100;
				}
				param->yySeconds = 0;
				param->yyMeridian = MER24;
			}
		}
	}
;

o_merid:
	  /* empty */		{ $$ = MER24; }
	| tMERIDIAN		{ $$ = $1; }
	| tTIME			{ $$ = $1 == 0 ? MER_MN : MER_NOON; }
;

%%

static short DaysInMonth[12] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

/*
 * works with tm.tm_year (ie: rel to 1900)
 */
#define	isleap(yr)  (((yr) & 3) == 0 && (((yr) % 100) != 0 || \
			((1900+(yr)) % 400) == 0))

/* Month and day table. */
static const TABLE MonthDayTable[] = {
    { "january",	tMONTH,  1 },
    { "february",	tMONTH,  2 },
    { "march",		tMONTH,  3 },
    { "april",		tMONTH,  4 },
    { "may",		tMONTH,  5 },
    { "june",		tMONTH,  6 },
    { "july",		tMONTH,  7 },
    { "august",		tMONTH,  8 },
    { "september",	tMONTH,  9 },
    { "sept",		tMONTH,  9 },
    { "october",	tMONTH, 10 },
    { "november",	tMONTH, 11 },
    { "december",	tMONTH, 12 },
    { "sunday",		tDAY, 0 },
    { "su",		tDAY, 0 },
    { "monday",		tDAY, 1 },
    { "mo",		tDAY, 1 },
    { "tuesday",	tDAY, 2 },
    { "tues",		tDAY, 2 },
    { "tu",		tDAY, 2 },
    { "wednesday",	tDAY, 3 },
    { "wednes",		tDAY, 3 },
    { "weds",		tDAY, 3 },
    { "we",		tDAY, 3 },
    { "thursday",	tDAY, 4 },
    { "thurs",		tDAY, 4 },
    { "thur",		tDAY, 4 },
    { "th",		tDAY, 4 },
    { "friday",		tDAY, 5 },
    { "fr",		tDAY, 5 },
    { "saturday",	tDAY, 6 },
    { "sa",		tDAY, 6 },
    { NULL,		0,    0 }
};

/* Time units table. */
static const TABLE UnitsTable[] = {
    { "year",		tMONTH_UNIT,	12 },
    { "month",		tMONTH_UNIT,	1 },
    { "fortnight",	tMINUTE_UNIT,	14 * 24 * 60 },
    { "week",		tMINUTE_UNIT,	7 * 24 * 60 },
    { "day",		tMINUTE_UNIT,	1 * 24 * 60 },
    { "hour",		tMINUTE_UNIT,	60 },
    { "minute",		tMINUTE_UNIT,	1 },
    { "min",		tMINUTE_UNIT,	1 },
    { "second",		tSEC_UNIT,	1 },
    { "sec",		tSEC_UNIT,	1 },
    { NULL,		0,		0 }
};

/* Assorted relative-time words. */
static const TABLE OtherTable[] = {
    { "tomorrow",	tMINUTE_UNIT,	1 * 24 * 60 },
    { "yesterday",	tMINUTE_UNIT,	-1 * 24 * 60 },
    { "today",		tMINUTE_UNIT,	0 },
    { "now",		tMINUTE_UNIT,	0 },
    { "last",		tUNUMBER,	-1 },
    { "this",		tMINUTE_UNIT,	0 },
    { "next",		tUNUMBER,	2 },
    { "first",		tUNUMBER,	1 },
    { "one",		tUNUMBER,	1 },
/*  { "second",		tUNUMBER,	2 }, */
    { "two",		tUNUMBER,	2 },
    { "third",		tUNUMBER,	3 },
    { "three",		tUNUMBER,	3 },
    { "fourth",		tUNUMBER,	4 },
    { "four",		tUNUMBER,	4 },
    { "fifth",		tUNUMBER,	5 },
    { "five",		tUNUMBER,	5 },
    { "sixth",		tUNUMBER,	6 },
    { "six",		tUNUMBER,	6 },
    { "seventh",	tUNUMBER,	7 },
    { "seven",		tUNUMBER,	7 },
    { "eighth",		tUNUMBER,	8 },
    { "eight",		tUNUMBER,	8 },
    { "ninth",		tUNUMBER,	9 },
    { "nine",		tUNUMBER,	9 },
    { "tenth",		tUNUMBER,	10 },
    { "ten",		tUNUMBER,	10 },
    { "eleventh",	tUNUMBER,	11 },
    { "eleven",		tUNUMBER,	11 },
    { "twelfth",	tUNUMBER,	12 },
    { "twelve",		tUNUMBER,	12 },
    { "ago",		tAGO,	1 },
    { NULL,		0,	0 }
};

/* The timezone table. */
/* Some of these are commented out because a time_t can't store a float. */
static const TABLE TimezoneTable[] = {
    { "gmt",	tZONE,     HOUR( 0) },	/* Greenwich Mean */
    { "ut",	tZONE,     HOUR( 0) },	/* Universal (Coordinated) */
    { "utc",	tZONE,     HOUR( 0) },
    { "wet",	tZONE,     HOUR( 0) },	/* Western European */
    { "bst",	tDAYZONE,  HOUR( 0) },	/* British Summer */
    { "wat",	tZONE,     HOUR( 1) },	/* West Africa */
    { "at",	tZONE,     HOUR( 2) },	/* Azores */
#if	0
    /* For completeness.  BST is also British Summer, and GST is
     * also Guam Standard. */
    { "bst",	tZONE,     HOUR( 3) },	/* Brazil Standard */
    { "gst",	tZONE,     HOUR( 3) },	/* Greenland Standard */
#endif
    { "nft",	tZONE,     HOUR(3.5) },	/* Newfoundland */
    { "nst",	tZONE,     HOUR(3.5) },	/* Newfoundland Standard */
    { "ndt",	tDAYZONE,  HOUR(3.5) },	/* Newfoundland Daylight */
    { "ast",	tZONE,     HOUR( 4) },	/* Atlantic Standard */
    { "adt",	tDAYZONE,  HOUR( 4) },	/* Atlantic Daylight */
    { "est",	tZONE,     HOUR( 5) },	/* Eastern Standard */
    { "edt",	tDAYZONE,  HOUR( 5) },	/* Eastern Daylight */
    { "cst",	tZONE,     HOUR( 6) },	/* Central Standard */
    { "cdt",	tDAYZONE,  HOUR( 6) },	/* Central Daylight */
    { "mst",	tZONE,     HOUR( 7) },	/* Mountain Standard */
    { "mdt",	tDAYZONE,  HOUR( 7) },	/* Mountain Daylight */
    { "pst",	tZONE,     HOUR( 8) },	/* Pacific Standard */
    { "pdt",	tDAYZONE,  HOUR( 8) },	/* Pacific Daylight */
    { "yst",	tZONE,     HOUR( 9) },	/* Yukon Standard */
    { "ydt",	tDAYZONE,  HOUR( 9) },	/* Yukon Daylight */
    { "hst",	tZONE,     HOUR(10) },	/* Hawaii Standard */
    { "hdt",	tDAYZONE,  HOUR(10) },	/* Hawaii Daylight */
    { "cat",	tZONE,     HOUR(10) },	/* Central Alaska */
    { "ahst",	tZONE,     HOUR(10) },	/* Alaska-Hawaii Standard */
    { "nt",	tZONE,     HOUR(11) },	/* Nome */
    { "idlw",	tZONE,     HOUR(12) },	/* International Date Line West */
    { "cet",	tZONE,     -HOUR(1) },	/* Central European */
    { "met",	tZONE,     -HOUR(1) },	/* Middle European */
    { "mewt",	tZONE,     -HOUR(1) },	/* Middle European Winter */
    { "mest",	tDAYZONE,  -HOUR(1) },	/* Middle European Summer */
    { "swt",	tZONE,     -HOUR(1) },	/* Swedish Winter */
    { "sst",	tDAYZONE,  -HOUR(1) },	/* Swedish Summer */
    { "fwt",	tZONE,     -HOUR(1) },	/* French Winter */
    { "fst",	tDAYZONE,  -HOUR(1) },	/* French Summer */
    { "eet",	tZONE,     -HOUR(2) },	/* Eastern Europe, USSR Zone 1 */
    { "bt",	tZONE,     -HOUR(3) },	/* Baghdad, USSR Zone 2 */
    { "it",	tZONE,     -HOUR(3.5) },/* Iran */
    { "zp4",	tZONE,     -HOUR(4) },	/* USSR Zone 3 */
    { "zp5",	tZONE,     -HOUR(5) },	/* USSR Zone 4 */
    { "ist",	tZONE,     -HOUR(5.5) },/* Indian Standard */
    { "zp6",	tZONE,     -HOUR(6) },	/* USSR Zone 5 */
#if	0
    /* For completeness.  NST is also Newfoundland Stanard, and SST is
     * also Swedish Summer. */
    { "nst",	tZONE,     -HOUR(6.5) },/* North Sumatra */
    { "sst",	tZONE,     -HOUR(7) },	/* South Sumatra, USSR Zone 6 */
#endif	/* 0 */
    { "ict",	tZONE,     -HOUR(7) },	/* Indo China Time (Thai) */
#if 0	/* this one looks to be bogus */
    { "jt",	tZONE,     -HOUR(7.5) },/* Java (3pm in Cronusland!) */
#endif
    { "wast",	tZONE,     -HOUR(8) },	/* West Australian Standard */
    { "awst",	tZONE,     -HOUR(8) },	/* West Australian Standard */
    { "wadt",	tDAYZONE,  -HOUR(8) },	/* West Australian Daylight */
    { "awdt",	tDAYZONE,  -HOUR(8) },	/* West Australian Daylight */
    { "cct",	tZONE,     -HOUR(8) },	/* China Coast, USSR Zone 7 */
    { "sgt",	tZONE,     -HOUR(8) },	/* Singapore */
    { "hkt",	tZONE,     -HOUR(8) },	/* Hong Kong */
    { "jst",	tZONE,     -HOUR(9) },	/* Japan Standard, USSR Zone 8 */
    { "cast",	tZONE,     -HOUR(9.5) },/* Central Australian Standard */
    { "acst",	tZONE,     -HOUR(9.5) },/* Central Australian Standard */
    { "cadt",	tDAYZONE,  -HOUR(9.5) },/* Central Australian Daylight */
    { "acdt",	tDAYZONE,  -HOUR(9.5) },/* Central Australian Daylight */
    { "east",	tZONE,     -HOUR(10) },	/* Eastern Australian Standard */
    { "aest",	tZONE,     -HOUR(10) },	/* Eastern Australian Standard */
    { "eadt",	tDAYZONE,  -HOUR(10) },	/* Eastern Australian Daylight */
    { "aedt",	tDAYZONE,  -HOUR(10) },	/* Eastern Australian Daylight */
    { "gst",	tZONE,     -HOUR(10) },	/* Guam Standard, USSR Zone 9 */
    { "nzt",	tZONE,     -HOUR(12) },	/* New Zealand */
    { "nzst",	tZONE,     -HOUR(12) },	/* New Zealand Standard */
    { "nzdt",	tDAYZONE,  -HOUR(12) },	/* New Zealand Daylight */
    { "idle",	tZONE,     -HOUR(12) },	/* International Date Line East */
    {  NULL,	0,	    0 }
};

/* Military timezone table. */
static const TABLE MilitaryTable[] = {
    { "a",	tZONE,	HOUR(  1) },
    { "b",	tZONE,	HOUR(  2) },
    { "c",	tZONE,	HOUR(  3) },
    { "d",	tZONE,	HOUR(  4) },
    { "e",	tZONE,	HOUR(  5) },
    { "f",	tZONE,	HOUR(  6) },
    { "g",	tZONE,	HOUR(  7) },
    { "h",	tZONE,	HOUR(  8) },
    { "i",	tZONE,	HOUR(  9) },
    { "k",	tZONE,	HOUR( 10) },
    { "l",	tZONE,	HOUR( 11) },
    { "m",	tZONE,	HOUR( 12) },
    { "n",	tZONE,	HOUR(- 1) },
    { "o",	tZONE,	HOUR(- 2) },
    { "p",	tZONE,	HOUR(- 3) },
    { "q",	tZONE,	HOUR(- 4) },
    { "r",	tZONE,	HOUR(- 5) },
    { "s",	tZONE,	HOUR(- 6) },
    { "t",	tZONE,	HOUR(- 7) },
    { "u",	tZONE,	HOUR(- 8) },
    { "v",	tZONE,	HOUR(- 9) },
    { "w",	tZONE,	HOUR(-10) },
    { "x",	tZONE,	HOUR(-11) },
    { "y",	tZONE,	HOUR(-12) },
    { "z",	tZONE,	HOUR(  0) },
    { NULL,	0,	0 }
};

static const TABLE TimeNames[] = {
    { "midnight",	tTIME,		 0 },
    { "mn",		tTIME,		 0 },
    { "noon",		tTIME,		12 },
    { "midday",		tTIME,		12 },
    { NULL,		0,		 0 }
};



/* ARGSUSED */
static int
yyerror(struct dateinfo *param, const char **inp, const char *s __unused)
{
  return 0;
}

/*
 * Save a relative value, if it fits
 */
static int
RelVal(struct dateinfo *param, time_t num, time_t unit, int scale, int type)
{
	int i;
	time_t v;
	uintmax_t m;
	int sign = 1;

	if ((i = param->yyHaveRel) >= MAXREL)
		return 0;

	if (num < 0) {
		sign = -sign;
		num = -num;
	}
	if (unit < 0) {
		sign = -sign;
		unit = -unit;
	}
	/* scale is always positive */

	m = LLONG_MAX;		/* TIME_T_MAX */
	if (scale > 1)
		m /= scale;
	if (unit > 1)
		m /= unit;
	if ((uintmax_t)num > m)
		return 0;

	m = num * unit * scale;
	v = (time_t) m;
	if (v < 0 || (uintmax_t)v != m)
		return 0;
	if (sign < 0)
		v = -v;

	param->yyRel[i].yyRelMonth = type;
	param->yyRel[i].yyRelVal = v;

	return 1;
}

/*
 * Adjust year from a value that might be abbreviated, to a full value.
 * e.g. convert 70 to 1970.
 * Input Year is either:
 *  - A negative number, which means to use its absolute value (why?)
 *  - A number from 0 to 68, which means a year from 2000 to 2068, 
 *  - A number from 69 to 99, which means a year from 1969 to 1999, or
 *  - The actual year (>=100).
 * Returns the full year.
 */
static time_t
AdjustYear(time_t Year)
{
    /* XXX Y2K */
    if (Year < 0)
	Year = -Year;
    if (Year < 69)	/* POSIX compliant, 0..68 is 2000's, 69-99 1900's */
	Year += 2000;
    else if (Year < 100)
	Year += 1900;
    return Year;
}

static time_t
Convert(
    time_t	Month,		/* month of year [1-12] */
    time_t	Day,		/* day of month [1-31] */
    time_t	Year,		/* year, not abbreviated in any way */
    time_t	Hours,		/* Hour of day [0-24] */
    time_t	Minutes,	/* Minute of hour [0-59] */
    time_t	Seconds,	/* Second of minute [0-60] */
    time_t	Timezone,	/* Timezone as minutes east of UTC,
				 * or USE_LOCAL_TIME special case */
    MERIDIAN	Meridian,	/* Hours are am/pm/24 hour clock */
    DSTMODE	DSTmode		/* DST on/off/maybe */
)
{
    struct tm tm = {.tm_sec = 0};
    struct tm otm;
    time_t result;

    tm.tm_sec = Seconds;
    tm.tm_min = Minutes;
    tm.tm_hour = ((Hours == 12 && Meridian != MER24) ? 0 : Hours) +
	(Meridian == MERpm ? 12 : 0);

    tm.tm_mday = Day;
    tm.tm_mon = Month - 1;
    tm.tm_year = Year - 1900;
    if ((time_t)tm.tm_year + 1900 != Year) {
	errno = EOVERFLOW;
	return -1;
    }
    if (Timezone == USE_LOCAL_TIME) {
	    switch (DSTmode) {
	    case DSTon:  tm.tm_isdst = 1; break;
	    case DSToff: tm.tm_isdst = 0; break;
	    default:     tm.tm_isdst = -1; break;
	    }
	    otm = tm;
	    result = mktime(&tm);
    } else {
	    /* We rely on mktime_z(NULL, ...) working in UTC */
	    tm.tm_isdst = 0;	/* hence cannot be summer time */
	    otm = tm;
	    errno = 0;
	    result = mktime_z(NULL, &tm);
	    if (result != -1 || errno == 0) {
		    result += Timezone * 60;
		    if (DSTmode == DSTon)	/* if specified sumer time */
			result -= 3600;		/* UTC is 1 hour earlier XXX */
	    }
    }

#if PARSEDATE_DEBUG
    fprintf(stderr, "%s(M=%jd D=%jd Y=%jd H=%jd M=%jd S=%jd Z=%jd"
		    " mer=%d DST=%d)",
	__func__,
	(intmax_t)Month, (intmax_t)Day, (intmax_t)Year,
	(intmax_t)Hours, (intmax_t)Minutes, (intmax_t)Seconds,
	(intmax_t)Timezone, (int)Meridian, (int)DSTmode);
    fprintf(stderr, " -> %jd", (intmax_t)result);
    fprintf(stderr, " %s", ctime(&result));
#endif

#define	TM_NE(fld) (otm.tm_ ## fld != tm.tm_ ## fld)
    if (TM_NE(year) || TM_NE(mon) || TM_NE(mday) ||
	TM_NE(hour) || TM_NE(min) || TM_NE(sec)) {
	    /* mktime() "corrected" our tm, so it must have been invalid */
	    result = -1;
	    errno = EAGAIN;
    }
#undef	TM_NE

    return result;
}


static time_t
DSTcorrect(
    time_t	Start,
    time_t	Future
)
{
    time_t	StartDay;
    time_t	FutureDay;
    struct tm	tm;

    if (localtime_r(&Start, &tm) == NULL)
	return -1;
    StartDay = (tm.tm_hour + 1) % 24;

    if (localtime_r(&Future, &tm) == NULL)
	return -1;
    FutureDay = (tm.tm_hour + 1) % 24;

    return (Future - Start) + (StartDay - FutureDay) * 60L * 60L;
}


static time_t
RelativeDate(
    time_t	Start,
    time_t	DayOrdinal,
    time_t	DayNumber
)
{
    struct tm	tm;
    time_t	now;
    time_t	change;

    now = Start;
    if (localtime_r(&now, &tm) == NULL)
	return -1;

    /* should be using TIME_T_MAX but there is no such thing, so just "know" */
    if (llabs(DayOrdinal) >= LLONG_MAX / (7 * SECSPERDAY)) {
	errno = EOVERFLOW;
	return -1;
    }

    change = SECSPERDAY * ((DayNumber - tm.tm_wday + 7) % 7);
    change += 7 * SECSPERDAY * (DayOrdinal <= 0 ? DayOrdinal : DayOrdinal - 1);

    /* same here for _MAX and _MIN */
    if ((change > 0 && LLONG_MAX - change < now) ||
	(change < 0 && LLONG_MIN - change > now)) {
	    errno = EOVERFLOW;
	    return -1;
    }

    now += change;
    return DSTcorrect(Start, now);
}


static time_t
RelativeMonth(
    time_t	Start,
    time_t	RelMonth,
    time_t	Timezone
)
{
    struct tm	tm;
    time_t	Month;
    time_t	Then;
    int		Day;

    if (RelMonth == 0)
	return 0;
    /*
     * It doesn't matter what timezone we use to do this computation,
     * as long as we use the same one to reassemble the time that we
     * used to disassemble it. So always use localtime and mktime. In
     * particular, don't use Convert() to reassemble, because it will
     * not only reassemble with the wrong timezone but it will also
     * fail if we do e.g. three months from March 31 yielding July 1.
     */
    (void)Timezone;

    if (localtime_r(&Start, &tm) == NULL)
	return -1;

    if (RelMonth >= LLONG_MAX - 12*((time_t)tm.tm_year + 1900) - tm.tm_mon) {
	errno = EOVERFLOW;
	return -1;
    }
    Month = 12 * (tm.tm_year + 1900) + tm.tm_mon + RelMonth;
    tm.tm_year = (Month / 12) - 1900;
    /* check for tm_year (an int) overflow */
    if (((time_t)tm.tm_year + 1900) != Month/12) {
	errno = EOVERFLOW;
	return -1;
    }
    tm.tm_mon = Month % 12;
    if (tm.tm_mday > (Day = DaysInMonth[tm.tm_mon] +
	((tm.tm_mon==1) ? isleap(tm.tm_year) : 0)))
	    tm.tm_mday = Day;
    errno = 0;
    Then = mktime(&tm);
    if (Then == -1 && errno != 0)
	return -1;
    return DSTcorrect(Start, Then);
}


static int
LookupWord(YYSTYPE *yylval, char *buff)
{
    register char	*p;
    register char	*q;
    register const TABLE	*tp;
    int			i;
    int			abbrev;

    /* Make it lowercase. */
    for (p = buff; *p; p++)
	if (isupper((unsigned char)*p))
	    *p = tolower((unsigned char)*p);

    if (strcmp(buff, "am") == 0 || strcmp(buff, "a.m.") == 0) {
	yylval->Meridian = MERam;
	return tMERIDIAN;
    }
    if (strcmp(buff, "pm") == 0 || strcmp(buff, "p.m.") == 0) {
	yylval->Meridian = MERpm;
	return tMERIDIAN;
    }

    /* See if we have an abbreviation for a month. */
    if (strlen(buff) == 3)
	abbrev = 1;
    else if (strlen(buff) == 4 && buff[3] == '.') {
	abbrev = 1;
	buff[3] = '\0';
    }
    else
	abbrev = 0;

    for (tp = MonthDayTable; tp->name; tp++) {
	if (abbrev) {
	    if (strncmp(buff, tp->name, 3) == 0) {
		yylval->Number = tp->value;
		return tp->type;
	    }
	}
	else if (strcmp(buff, tp->name) == 0) {
	    yylval->Number = tp->value;
	    return tp->type;
	}
    }

    for (tp = TimezoneTable; tp->name; tp++)
	if (strcmp(buff, tp->name) == 0) {
	    yylval->Number = tp->value;
	    return tp->type;
	}

    if (strcmp(buff, "dst") == 0) 
	return tDST;

    for (tp = TimeNames; tp->name; tp++)
	if (strcmp(buff, tp->name) == 0) {
	    yylval->Number = tp->value;
	    return tp->type;
	}

    for (tp = UnitsTable; tp->name; tp++)
	if (strcmp(buff, tp->name) == 0) {
	    yylval->Number = tp->value;
	    return tp->type;
	}

    /* Strip off any plural and try the units table again. */
    i = strlen(buff) - 1;
    if (buff[i] == 's') {
	buff[i] = '\0';
	for (tp = UnitsTable; tp->name; tp++)
	    if (strcmp(buff, tp->name) == 0) {
		yylval->Number = tp->value;
		return tp->type;
	    }
	buff[i] = 's';		/* Put back for "this" in OtherTable. */
    }

    for (tp = OtherTable; tp->name; tp++)
	if (strcmp(buff, tp->name) == 0) {
	    yylval->Number = tp->value;
	    return tp->type;
	}

    /* Military timezones. */
    if (buff[1] == '\0' && isalpha((unsigned char)*buff)) {
	for (tp = MilitaryTable; tp->name; tp++)
	    if (strcmp(buff, tp->name) == 0) {
		yylval->Number = tp->value;
		return tp->type;
	    }
    }

    /* Drop out any periods and try the timezone table again. */
    for (i = 0, p = q = buff; *q; q++)
	if (*q != '.')
	    *p++ = *q;
	else
	    i++;
    *p = '\0';
    if (i)
	for (tp = TimezoneTable; tp->name; tp++)
	    if (strcmp(buff, tp->name) == 0) {
		yylval->Number = tp->value;
		return tp->type;
	    }

    return tID;
}


static int
yylex(YYSTYPE *yylval, const char **yyInput)
{
    register char	c;
    register char	*p;
    char		buff[20];
    int			Count;
    int			sign;
    const char		*inp = *yyInput;

    for ( ; ; ) {
	while (isspace((unsigned char)*inp))
	    inp++;

	if (isdigit((unsigned char)(c = *inp)) || c == '-' || c == '+') {
	    if (c == '-' || c == '+') {
		sign = c == '-' ? -1 : 1;
		if (!isdigit((unsigned char)*++inp))
		    /* skip the '-' sign */
		    continue;
	    }
	    else
		sign = 0;
	    for (yylval->Number = 0; isdigit((unsigned char)(c = *inp++)); ) {
	        time_t	v;

		v = yylval->Number;
		if (v > LLONG_MAX/10 ||
		    (v == LLONG_MAX/10 && (v * 10 > LLONG_MAX - (c - '0')))) 
			yylval->Number = LLONG_MAX;
		else
			yylval->Number = 10 * yylval->Number + c - '0';
	    }
	    if (sign < 0)
		yylval->Number = -yylval->Number;
	    *yyInput = --inp;
	    return sign ? tSNUMBER : tUNUMBER;
	}
	if (isalpha((unsigned char)c)) {
	    for (p = buff; isalpha((unsigned char)(c = *inp++)) || c == '.'; )
		if (p < &buff[sizeof buff - 1])
		    *p++ = c;
	    *p = '\0';
	    *yyInput = --inp;
	    return LookupWord(yylval, buff);
	}
	if (c == '@') {
	    *yyInput = ++inp;
	    return AT_SIGN;
	}
	if (c != '(') {
	    *yyInput = ++inp;
	    return c;
	}
	Count = 0;
	do {
	    c = *inp++;
	    if (c == '\0')
		return c;
	    if (c == '(')
		Count++;
	    else if (c == ')')
		Count--;
	} while (Count > 0);
    }
}

#define TM_YEAR_ORIGIN 1900

time_t
parsedate(const char *p, const time_t *now, const int *zone)
{
    struct tm		local, *tm;
    time_t		nowt;
    int			zonet;
    time_t		Start;
    time_t		tod, rm;
    struct dateinfo	param;
    int			saved_errno;
    int			i;
    
    saved_errno = errno;
    errno = 0;

    if (now == NULL) {
        now = &nowt;
	(void)time(&nowt);
    }
    if (zone == NULL) {
	zone = &zonet;
	zonet = USE_LOCAL_TIME;
	if ((tm = localtime_r(now, &local)) == NULL)
	    return -1;
    } else {
	/*
	 * Should use the specified zone, not localtime.
	 * Fake it using gmtime and arithmetic.
	 * This is good enough because we use only the year/month/day,
	 * not other fields of struct tm.
	 */
	time_t fake = *now + (*zone * 60);
	if ((tm = gmtime_r(&fake, &local)) == NULL)
	    return -1;
    }
    param.yyYear = tm->tm_year + 1900;
    param.yyMonth = tm->tm_mon + 1;
    param.yyDay = tm->tm_mday;
    param.yyTimezone = *zone;
    param.yyDSTmode = DSTmaybe;
    param.yyHour = 0;
    param.yyMinutes = 0;
    param.yySeconds = 0;
    param.yyMeridian = MER24;
    param.yyHaveDate = 0;
    param.yyHaveFullYear = 0;
    param.yyHaveDay = 0;
    param.yyHaveRel = 0;
    param.yyHaveTime = 0;
    param.yyHaveZone = 0;

    /*
     * This one is too hard to parse using a grammar (the lexer would
     * confuse the 'T' with the Mil format timezone designator)
     * so handle it as a special case.
     */
    do {
	const unsigned char *pp = (const unsigned char *)p;
	char *ep;	/* starts as "expected, becomes "end ptr" */
	static char format[] = "-dd-ddTdd:dd:dd";
	time_t yr;

	while (isdigit(*pp))
		pp++;

	if (pp == (const unsigned char *)p)
		break;

	for (ep = format; *ep; ep++, pp++) {
		switch (*ep) {
		case 'd':
			if (isdigit(*pp))
				continue;
			break;
		case 'T':
			if (*pp == 'T' || *pp == 't' || *pp == ' ')
				continue;
			break;
		default:
			if (*pp == *ep)
				continue;
			break;
		}
		break;
	}
	if (*ep != '\0')
		break;
	if (*pp == '.' || *pp == ',') {
		if (!isdigit(pp[1]))
			break;
		while (isdigit(*++pp))
			continue;
	}
	if (*pp == 'Z' || *pp == 'z')
		pp++;
	else if (isdigit(*pp))
		break;

	if (*pp != '\0' && !isspace(*pp))
		break;

	errno = 0;
	yr = (time_t)strtol(p, &ep, 10);
	if (errno != 0)			/* out of range (can be big number) */
		break;			/* the ones below are all 2 digits */

	/*
	 * This is good enough to commit to there being an ISO format
	 * timestamp leading the input string.   We permit standard
	 * parsedate() modifiers to follow but not precede this string.
	 */
	param.yyHaveTime = 1;
	param.yyHaveDate = 1;
	param.yyHaveFullYear = 1;

	if (pp[-1] == 'Z' || pp[-1] == 'z') {
		param.yyTimezone = 0;
		param.yyHaveZone = 1;
	}

	param.yyYear = yr;
	param.yyMonth = (time_t)strtol(ep + 1, &ep, 10);
	param.yyDay = (time_t)strtol(ep + 1, &ep, 10);
	param.yyHour = (time_t)strtol(ep + 1, &ep, 10);
	param.yyMinutes = (time_t)strtol(ep + 1, &ep, 10);
	param.yySeconds = (time_t)strtol(ep + 1, &ep, 10);
	/* ignore any fractional seconds, no way to return them in a time_t */

	param.yyMeridian = MER24;

	p = (const char *)pp;
    } while (0);

    if (yyparse(&param, &p) || param.yyHaveTime > 1 || param.yyHaveZone > 1 ||
	param.yyHaveDate > 1 || param.yyHaveDay > 1) {
	errno = EINVAL;
	return -1;
    }

    if (param.yyHaveDate || param.yyHaveTime || param.yyHaveDay) {
	if (! param.yyHaveFullYear) {
		param.yyYear = AdjustYear(param.yyYear);
		param.yyHaveFullYear = 1;
	}
	errno = 0;
	Start = Convert(param.yyMonth, param.yyDay, param.yyYear, param.yyHour,
	    param.yyMinutes, param.yySeconds, param.yyTimezone,
	    param.yyMeridian, param.yyDSTmode);
	if (Start == -1 && errno != 0)
	    return -1;
    }
    else {
	Start = *now;
	if (!param.yyHaveRel)
	    Start -= ((tm->tm_hour * 60L + tm->tm_min) * 60L) + tm->tm_sec;
    }

    if (param.yyHaveRel > MAXREL) {
	errno = EINVAL;
	return -1;
    }
    for (i = 0; i < param.yyHaveRel; i++) {
	if (param.yyRel[i].yyRelMonth) {
	    errno = 0;
	    rm = RelativeMonth(Start, param.yyRel[i].yyRelVal, param.yyTimezone);
	    if (rm == -1 && errno != 0)
		return -1;
	    Start += rm;
	} else
	    Start += param.yyRel[i].yyRelVal;
    }

    if (param.yyHaveDay && !param.yyHaveDate) {
	errno = 0;
	tod = RelativeDate(Start, param.yyDayOrdinal, param.yyDayNumber);
	if (tod == -1 && errno != 0)
	    return -1;
	Start += tod;
    }

    errno = saved_errno;
    return Start;
}


#if	defined(TEST)

/* ARGSUSED */
int
main(int ac, char *av[])
{
    char	buff[128];
    time_t	d;

    (void)printf("Enter date, or blank line to exit.\n\t> ");
    (void)fflush(stdout);
    while (fgets(buff, sizeof(buff), stdin) && buff[0] != '\n') {
	errno = 0;
	d = parsedate(buff, NULL, NULL);
	if (d == -1 && errno != 0)
	    (void)printf("Bad format - couldn't convert: %s\n",
	        strerror(errno));
	else
	    (void)printf("%jd\t%s", (intmax_t)d, ctime(&d));
	(void)printf("\t> ");
	(void)fflush(stdout);
    }
    exit(0);
    /* NOTREACHED */
}
#endif	/* defined(TEST) */
