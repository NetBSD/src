/*
 * /src/NTP/REPOSITORY/v4/xntpd/refclock_parse.c,v 3.98 1997/01/19 14:11:59 kardel Exp
 *
 * refclock_parse.c,v 3.98 1997/01/19 14:11:59 kardel Exp
 *
 * generic reference clock driver for receivers
 *
 * make use of a STREAMS module for input processing where
 * available and configured. Currently the STREAMS module
 * is only available for Suns running SunOS 4.x and SunOS5.x
 *
 * Copyright (c) 1989,1990,1991,1992,1993,1994,1995,1996, 1997 by Frank Kardel
 * Friedrich-Alexander Universität Erlangen-Nürnberg, Germany
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined(REFCLOCK) && defined(PARSE)

/*
 * Defines:
 *  REFCLOCK && (PARSE||PARSEPPS)
 *                    - enable this mess
 *  STREAM            - allow for STREAMS modules
 *                      ("parse", "ppsclocd", "ppsclock")
 *  PPS		      - use ppsclock module ioctl
 *
 * TTY defines:
 *  HAVE_BSD_TTYS     - currently unsupported
 *  HAVE_SYSV_TTYS    - will use termio.h
 *  HAVE_TERMIOS      - will use termios.h
 *  STREAM            - will use streams and implies HAVE_TERMIOS
 */

/*
 * This driver currently provides the support for
 *   - Meinberg DCF77 receiver DCF77 PZF 535 (TCXO version) (DCF)
 *   - Meinberg DCF77 receiver DCF77 PZF 535 (OCXO version) (DCF)
 *   - Meinberg DCF77 receiver DCF77 AM receivers           (DCF)
 *   - IGEL CLOCK                                           (DCF)
 *   - ELV DCF7000                                          (DCF)
 *   - Schmid clock                                         (DCF)
 *   - Conrad DCF77 receiver module                         (DCF)
 *   - FAU DCF77 NTP receiver (TimeBrick)                   (DCF)
 *
 *   - Meinberg GPS166                                      (GPS)
 *   - Trimble SV6 (TSIP and TAIP protocol)                 (GPS)
 *
 *   - RCC8000 MSF Receiver                                 (MSF)
 */

/*
 * Meinberg receivers are connected via a 9600 baud serial line
 *
 * Receivers that do NOT support:
 *          - leap second indication
 * 	DCF U/A 31
 *	DCF PZF535 (stock version)
 *
 * CORRECTION: Meinberg will include LEAP SECOND anounncements in the
 * telegram. This might not be true for all of their products, but
 * some of them will. So you should definitely ask for NTP and
 * LEAP SECOND announcements when ordering receivers from them.
 *
 * so...
 *          - for PZF535 please ask for revision PZFUERL4.6 or higher
 *            (support for leap second and alternate antenna)
 *
 *          - LEAP SECOND announcement / NTP support for the other
 *            receivers
 *
 * The Meinberg GPS receiver also has a special NTP time stamp
 * format. The firmware release is Uni-Erlangen. Only this
 * firmware release is supported by xntp3.
 *
 * Meinberg generic receiver setup:
 *	output time code every second
 *	Baud rate 9600 7E2S
 */

#include "ntpd.h"
#include "ntp_refclock.h"
#include "ntp_unixtime.h"	/* includes <sys/time.h> */
#include "ntp_control.h"

#include <stdio.h>
#include <ctype.h>
#ifndef TM_IN_SYS_TIME
# include <time.h>
#endif

#include <sys/errno.h>
extern int errno;

#if !defined(STREAM) && !defined(HAVE_SYSV_TTYS) && !defined(HAVE_BSD_TTYS) && !defined(HAVE_TERMIOS)
# include "Bletch:  Define one of {STREAM,HAVE_SYSV_TTYS,HAVE_TERMIOS}"
#endif

#ifdef STREAM
# include <sys/stream.h>
# include <sys/stropts.h>
# ifndef HAVE_TERMIOS
#  define HAVE_TERMIOS
# endif
#endif

#ifdef HAVE_TERMIOS
# include <termios.h>
# define TTY_GETATTR(_FD_, _ARG_) tcgetattr((_FD_), (_ARG_))
# define TTY_SETATTR(_FD_, _ARG_) tcsetattr((_FD_), TCSANOW, (_ARG_))
# undef HAVE_SYSV_TTYS
#endif

#ifdef HAVE_SYSV_TTYS
# include <termio.h>
# define TTY_GETATTR(_FD_, _ARG_) ioctl((_FD_), TCGETA, (_ARG_))
# define TTY_SETATTR(_FD_, _ARG_) ioctl((_FD_), TCSETAW, (_ARG_))
#endif

#ifdef HAVE_BSD_TTYS
/* #error CURRENTLY NO BSD TTY SUPPORT */
# include "Bletch: BSD TTY not currently supported"
#endif

#if	!defined(O_RDWR)	/* XXX SOLARIS */
# include <fcntl.h>
#endif	/* !def(O_RDWR) */

#ifdef PPS
# include <sys/ppsclock.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include "ntp_io.h"
#include "ntp_select.h"
#include "ntp_stdlib.h"

#include "parse.h"

#if !defined(NO_SCCSID) && !defined(lint) && !defined(__GNUC__)
static char rcsid[]="refclock_parse.c,v 3.98 1997/01/19 14:11:59 kardel Exp";
#endif

/**===========================================================================
 ** external interface to xntp mechanism
 **/

static	void	parse_init	P((void));
static	int	parse_start	P((int, struct peer *));
static	void	parse_shutdown	P((int, struct peer *));
static	void	parse_poll	P((int, struct peer *));
static	void	parse_control	P((int, struct refclockstat *, struct refclockstat *));

#define	parse_buginfo	noentry

struct	refclock refclock_parse = {
	parse_start,
	parse_shutdown,
	parse_poll,
	parse_control,
	parse_init,
	parse_buginfo,
	NOFLAGS
};

/*
 * the unit field selects for one the prototype to be used (lower 4 bits)
 * and for the other the clock type in case of different but similar
 * receivers (bits 4-6)
 * the most significant bit encodes PPS support
 * when the most significant bit is set the pps telegrams will be used
 * for controlling the local clock (ntp_loopfilter.c)
 * receiver specific configration data is kept in the parse_clockinfo field.
 */

/*
 * Definitions
 */
#define	MAXUNITS	4	/* maximum number of "PARSE" units permitted */
#define PARSEDEVICE	"/dev/refclock-%d" /* device to open %d is unit number */

/**===========================================================================
 ** function vector for dynamically binding io handling mechanism
 **/

typedef struct bind
{
  char   *bd_description;	/* name of type of binding */
  int	(*bd_init)();		/* initialize */
  void	(*bd_end)();		/* end */
  int   (*bd_setcs)();		/* set character size */
  int	(*bd_disable)();	/* disable */
  int	(*bd_enable)();		/* enable */
  int	(*bd_getfmt)();		/* get format */
  int	(*bd_setfmt)();		/* setfmt */
  int	(*bd_timecode)();	/* get time code */
  void	(*bd_receive)();	/* receive operation */
  void	(*bd_poll)();		/* poll operation */
} bind_t;

#define PARSE_END(_X_)			(*(_X_)->binding->bd_end)(_X_)
#define PARSE_SETCS(_X_, _CS_)		(*(_X_)->binding->bd_setcs)(_X_, _CS_)
#define PARSE_ENABLE(_X_)		(*(_X_)->binding->bd_enable)(_X_)
#define PARSE_DISABLE(_X_)		(*(_X_)->binding->bd_disable)(_X_)
#define PARSE_GETFMT(_X_, _DCT_)	(*(_X_)->binding->bd_getfmt)(_X_, _DCT_)
#define PARSE_SETFMT(_X_, _DCT_)	(*(_X_)->binding->bd_setfmt)(_X_, _DCT_)
#define PARSE_GETTIMECODE(_X_, _DCT_)	(*(_X_)->binding->bd_timecode)(_X_, _DCT_)
#define PARSE_POLL(_X_)			(*(_X_)->binding->bd_poll)(_X_)

/*
 * io modes
 */
#define PARSE_F_NOPOLLONLY	0x0001 /* always do async io (possible PPS support via PARSE) */
#define PARSE_F_POLLONLY	0x0002 /* never do async io  (no PPS support via PARSE) */
#define PARSE_F_PPSPPS		0x0004 /* use loopfilter PPS code (CIOGETEV) */
#define PARSE_F_PPSONSECOND	0x0008 /* PPS pulses are on second */


/**===========================================================================
 ** error message regression handling
 **
 ** there are quite a few errors that can occur in rapid succession such as
 ** noisy input data or no data at all. in order to reduce the amount of
 ** syslog messages in such case, we are using a backoff algorithm. We limit
 ** the number of error messages of a certain class to 1 per time unit. if a
 ** configurable number of messages is displayed that way, we move on to the
 ** next time unit / count for that class. a count of messages that have been
 ** suppressed is held and displayed whenever a corresponding message is
 ** displayed. the time units for a message class will also be displayed.
 ** whenever an error condition clears we reset the error message state,
 ** thus we would still generate much output on pathological conditions
 ** where the system oscillates between OK and NOT OK states. coping
 ** with that condition is currently considered too complicated.
 **/

#define ERR_ALL	       ~0	/* "all" errors */
#define ERR_BADDATA	0	/* unusable input data/conversion errors */
#define ERR_NODATA	1	/* no input data */
#define ERR_BADIO	2	/* read/write/select errors */
#define ERR_BADSTATUS	3	/* unsync states */
#define ERR_BADEVENT	4	/* non nominal events */
#define ERR_INTERNAL	5	/* internal error */
#define ERR_CNT		(ERR_INTERNAL+1)

#define ERR(_X_)	if (list_err(parse, (_X_)))

struct errorregression
{
  u_long err_count;	/* number of repititions per class */
  u_long err_delay;	/* minimum delay between messages */
};

static struct errorregression
err_baddata[] =			/* error messages for bad input data */
{
  { 1,       0 },		/* output first message immediately */
  { 5,      60 },		/* output next five messages in 60 second intervals */
  { 3,    3600 },		/* output next 3 messages in hour intervals */
  { 0, 12*3600 }		/* repeat messages only every 12 hours */
};

static struct errorregression
err_nodata[] =			/* error messages for missing input data */
{
  { 1,       0 },		/* output first message immediately */
  { 5,      60 },		/* output next five messages in 60 second intervals */
  { 3,    3600 },		/* output next 3 messages in hour intervals */
  { 0, 12*3600 }		/* repeat messages only every 12 hours */
};

static struct errorregression
err_badstatus[] =		/* unsynchronized state messages */
{
  { 1,       0 },		/* output first message immediately */
  { 5,      60 },		/* output next five messages in 60 second intervals */
  { 3,    3600 },		/* output next 3 messages in hour intervals */
  { 0, 12*3600 }		/* repeat messages only every 12 hours */
};

static struct errorregression
err_badio[] =			/* io failures (bad reads, selects, ...) */
{
  { 1,       0 },		/* output first message immediately */
  { 5,      60 },		/* output next five messages in 60 second intervals */
  { 5,    3600 },		/* output next 3 messages in hour intervals */
  { 0, 12*3600 }		/* repeat messages only every 12 hours */
};

static struct errorregression
err_badevent[] =		/* non nominal events */
{
  { 20,      0 },		/* output first message immediately */
  { 6,      60 },		/* output next five messages in 60 second intervals */
  { 5,    3600 },		/* output next 3 messages in hour intervals */
  { 0, 12*3600 }		/* repeat messages only every 12 hours */
};

static struct errorregression
err_internal[] =		/* really bad things - basically coding/OS errors */
{
  { 0,       0 },		/* output all messages immediately */
};

static struct errorregression *
err_tbl[] =
{
  err_baddata,
  err_nodata,
  err_badio,
  err_badstatus,
  err_badevent,
  err_internal
};

struct errorinfo
{
  u_long err_started;	/* begin time (xntp) of error condition */
  u_long err_last;	/* last time (xntp) error occurred */
  u_long err_cnt;	/* number of error repititions */
  u_long err_suppressed;	/* number of suppressed messages */
  struct errorregression *err_stage; /* current error stage */
};

/**===========================================================================
 ** refclock instance data
 **/

struct parseunit
{
  /*
   * XNTP management
   */
  struct peer         *peer;		/* backlink to peer structure - refclock inactive if 0  */
  struct refclockproc *generic;		/* backlink to refclockproc structure */

  /*
   * PARSE io
   */
  bind_t	     *binding;	        /* io handling binding */

  /*
   * parse state
   */
  parse_t	      parseio;	        /* io handling structure (user level parsing) */

  /*
   * type specific parameters
   */
  struct parse_clockinfo   *parse_type;	        /* link to clock description */

  /*
   * clock state handling/reporting
   */
  u_char	      flags;	        /* flags (leap_control) */
  u_char              lastevent; 	/* last not NORMAL status */
  u_long	      lastchange;       /* time (xntp) when last state change accured */
  u_long	      statetime[CEVNT_MAX+1]; /* accumulated time of clock states */
  struct event        stattimer;        /* statistics timer */

  u_char              pollonly;		/* 1 for polling only (no PPS mode) */
  u_char              pollneeddata; 	/* 1 for receive sample expected in PPS mode */
  u_long              laststatus;       /* last packet status (error indication) */
  u_short	      lastformat;       /* last format used */
  u_long              lastsync;		/* time (xntp) when clock was last seen fully synchronized */
  u_long              lastmissed;       /* time (xntp) when poll didn't get data (powerup heuristic) */
  u_long              ppsserial;        /* magic cookie for ppsclock serials (avoids stale ppsclock data) */
  parsetime_t         time;		/* last (parse module) data */
  void               *localdata;        /* optional local data */
  struct errorinfo    errors[ERR_CNT];  /* error state table for suppressing excessive error messages */
};


/**===========================================================================
 ** Clockinfo section all parameter for specific clock types
 ** includes NTP parameters, TTY parameters and IO handling parameters
 **/

static	void	poll_dpoll	P((struct parseunit *));
static	void	poll_poll	P((struct parseunit *));
static	int	poll_init	P((struct parseunit *));
static	void	poll_end	P((struct parseunit *));

typedef struct poll_info
{
  u_long rate;			/* poll rate - once every "rate" seconds - 0 off */
  char * string;		/* string to send for polling */
  u_long count;			/* number of charcters in string */
} poll_info_t;

#define NO_FLAGS	0
#define NO_POLL		(void (*)())0
#define NO_INIT		(int  (*)())0
#define NO_END		(void (*)())0
#define NO_EVENT	(void (*)())0
#define NO_DATA		(void *)0
#define NO_FORMAT	""
#define NO_PPSDELAY     0

#define DCF_ID		"DCF"	/* generic DCF */
#define DCF_A_ID	"DCFa"	/* AM demodulation */
#define DCF_P_ID	"DCFp"	/* psuedo random phase shift */
#define GPS_ID		"GPS"	/* GPS receiver */

#define	NOCLOCK_ROOTDELAY	0x00000000
#define	NOCLOCK_BASEDELAY	0x00000000
#define	NOCLOCK_DESCRIPTION	((char *)0)
#define NOCLOCK_MAXUNSYNC       0
#define NOCLOCK_CFLAG           0
#define NOCLOCK_IFLAG           0
#define NOCLOCK_OFLAG           0
#define NOCLOCK_LFLAG           0
#define NOCLOCK_ID		"TILT"
#define NOCLOCK_POLL		NO_POLL
#define NOCLOCK_INIT		NO_INIT
#define NOCLOCK_END		NO_END
#define NOCLOCK_DATA		NO_DATA
#define NOCLOCK_FORMAT		NO_FORMAT
#define NOCLOCK_TYPE		CTL_SST_TS_UNSPEC
#define NOCLOCK_SAMPLES		0
#define NOCLOCK_KEEP		0 

#define DCF_TYPE		CTL_SST_TS_LF
#define GPS_TYPE		CTL_SST_TS_UHF

/*
 * receiver specific constants
 */
#define MBG_SPEED		(B9600)
#define MBG_CFLAG		(CS7|PARENB|CREAD|CLOCAL|HUPCL)
#define MBG_IFLAG		(IGNBRK|IGNPAR|ISTRIP)
#define MBG_OFLAG		0
#define MBG_LFLAG		0
#define MBG_FLAGS               PARSE_F_NOPOLLONLY

/*
 * Meinberg DCF77 receivers
 */
#define	DCFUA31_ROOTDELAY	0x00000000  /* 0 */
#define	DCFUA31_BASEDELAY	0x02C00000  /* 10.7421875ms: 10 ms (+/- 3 ms) */
#define	DCFUA31_DESCRIPTION	"Meinberg DCF77 UA31/C51 or compatible"
#define DCFUA31_MAXUNSYNC       60*30       /* only trust clock for 1/2 hour */
#define DCFUA31_SPEED		MBG_SPEED
#define DCFUA31_CFLAG           MBG_CFLAG
#define DCFUA31_IFLAG           MBG_IFLAG
#define DCFUA31_OFLAG           MBG_OFLAG
#define DCFUA31_LFLAG           MBG_LFLAG
#define DCFUA31_SAMPLES		5
#define DCFUA31_KEEP		3

/*
 * Meinberg DCF PZF535/TCXO (FM/PZF) receiver
 */
#define	DCFPZF535_ROOTDELAY	0x00000000
#define	DCFPZF535_BASEDELAY	0x00800000  /* 1.968ms +- 104us (oscilloscope) - relative to start (end of STX) */
#define	DCFPZF535_DESCRIPTION	"Meinberg DCF PZF 535/TCXO"
#define DCFPZF535_MAXUNSYNC     60*60*12           /* only trust clock for 12 hours
						    * @ 5e-8df/f we have accumulated
						    * at most 2.16 ms (thus we move to
						    * NTP synchronisation */
#define DCFPZF535_SPEED		MBG_SPEED
#define DCFPZF535_CFLAG         MBG_CFLAG
#define DCFPZF535_IFLAG         MBG_IFLAG
#define DCFPZF535_OFLAG         MBG_OFLAG
#define DCFPZF535_LFLAG         MBG_LFLAG
#define DCFPZF535_SAMPLES		5
#define DCFPZF535_KEEP			3


/*
 * Meinberg DCF PZF535/OCXO receiver
 */
#define	DCFPZF535OCXO_ROOTDELAY	0x00000000
#define	DCFPZF535OCXO_BASEDELAY	0x00800000 /* 1.968ms +- 104us (oscilloscope) - relative to start (end of STX) */
#define	DCFPZF535OCXO_DESCRIPTION "Meinberg DCF PZF 535/OCXO"
#define DCFPZF535OCXO_MAXUNSYNC     60*60*96       /* only trust clock for 4 days
						    * @ 5e-9df/f we have accumulated
						    * at most an error of 1.73 ms
						    * (thus we move to NTP synchronisation) */
#define DCFPZF535OCXO_SPEED	    MBG_SPEED
#define DCFPZF535OCXO_CFLAG         MBG_CFLAG
#define DCFPZF535OCXO_IFLAG         MBG_IFLAG
#define DCFPZF535OCXO_OFLAG         MBG_OFLAG
#define DCFPZF535OCXO_LFLAG         MBG_LFLAG
#define DCFPZF535OCXO_SAMPLES		    5
#define DCFPZF535OCXO_KEEP	            3

/*
 * Meinberg GPS166 receiver
 */
#define	GPS166_ROOTDELAY	0x00000000         /* nothing here */
#define	GPS166_BASEDELAY	0x00800000         /* XXX to be fixed ! 1.968ms +- 104us (oscilloscope) - relative to start (end of STX) */
#define	GPS166_DESCRIPTION      "Meinberg GPS166 receiver"
#define GPS166_MAXUNSYNC        60*60*96       /* only trust clock for 4 days
						* @ 5e-9df/f we have accumulated
						* at most an error of 1.73 ms
						* (thus we move to NTP synchronisation) */
#define GPS166_SPEED		MBG_SPEED
#define GPS166_CFLAG            MBG_CFLAG
#define GPS166_IFLAG            MBG_IFLAG
#define GPS166_OFLAG            MBG_OFLAG
#define GPS166_LFLAG            MBG_LFLAG
#define GPS166_POLL		NO_POLL
#define GPS166_INIT		NO_INIT
#define GPS166_END		NO_END
#define GPS166_DATA		NO_DATA
#define GPS166_ID		GPS_ID
#define GPS166_FORMAT		NO_FORMAT
#define GPS166_SAMPLES		5
#define GPS166_KEEP		3

/*
 * ELV DCF7000 Wallclock-Receiver/Switching Clock (Kit)
 *
 * This is really not the hottest clock - but before you have nothing ...
 */
#define DCF7000_ROOTDELAY	0x00000000 /* 0 */
#define DCF7000_BASEDELAY	0x67AE0000 /* 405 ms - slow blow */
#define DCF7000_DESCRIPTION	"ELV DCF7000"
#define DCF7000_MAXUNSYNC	(60*5) /* sorry - but it just was not build as a clock */
#define DCF7000_SPEED		(B9600)
#define DCF7000_CFLAG           (CS8|CREAD|PARENB|PARODD|CLOCAL|HUPCL)
#define DCF7000_IFLAG		(IGNBRK)
#define DCF7000_OFLAG		0
#define DCF7000_LFLAG		0
#define DCF7000_SAMPLES		6
#define DCF7000_KEEP		4

/*
 * Schmid DCF Receiver Kit
 *
 * When the WSDCF clock is operating optimally we want the primary clock
 * distance to come out at 300 ms.  Thus, peer.distance in the WSDCF peer
 * structure is set to 290 ms and we compute delays which are at least
 * 10 ms long.  The following are 290 ms and 10 ms expressed in u_fp format
 */
#define WS_POLLRATE	1	/* every second - watch interdependency with poll routine */
#define WS_POLLCMD	"\163"
#define WS_CMDSIZE	1

static poll_info_t wsdcf_pollinfo = { WS_POLLRATE, WS_POLLCMD, WS_CMDSIZE };

#define WSDCF_INIT		poll_init
#define WSDCF_POLL		poll_dpoll
#define WSDCF_END		poll_end
#define WSDCF_DATA		((void *)(&wsdcf_pollinfo))
#define	WSDCF_ROOTDELAY		0X00000000	/* 0 */
#define	WSDCF_BASEDELAY	 	0x028F5C29	/*  ~  10ms */
#define WSDCF_DESCRIPTION	"WS/DCF Receiver"
#define WSDCF_FORMAT		"Schmid"
#define WSDCF_MAXUNSYNC		(60*60)	/* assume this beast hold at 1 h better than 2 ms XXX-must verify */
#define WSDCF_SPEED		(B1200)
#define WSDCF_CFLAG		(CS8|CREAD|CLOCAL)
#define WSDCF_IFLAG		0
#define WSDCF_OFLAG		0
#define WSDCF_LFLAG		0
#define WSDCF_SAMPLES		6
#define WSDCF_KEEP		4

/*
 * RAW DCF77 - input of DCF marks via RS232 - many variants
 */
#define RAWDCF_FLAGS		PARSE_F_NOPOLLONLY
#define RAWDCF_ROOTDELAY	0x00000000 /* 0 */
#define RAWDCF_FORMAT		"RAW DCF77 Timecode"
#define RAWDCF_MAXUNSYNC	(0) /* sorry - its a true receiver - no signal - no time */
#define RAWDCF_SPEED		(B50)
#ifdef NO_PARENB_IGNPAR /* Was: defined(SYS_IRIX4) || defined(SYS_IRIX5) */
/* somehow doesn't grok PARENB & IGNPAR (mj) */
# define RAWDCF_CFLAG            (CS8|CREAD|CLOCAL)
#else
# define RAWDCF_CFLAG            (CS8|CREAD|CLOCAL|PARENB)
#endif
#ifdef RAWDCF_NO_IGNPAR /* Was: defined(SYS_LINUX) && defined(CLOCK_RAWDCF) */
# define RAWDCF_IFLAG		0
#else
# define RAWDCF_IFLAG		(IGNPAR)
#endif
#define RAWDCF_OFLAG		0
#define RAWDCF_LFLAG		0
#define RAWDCF_SAMPLES		6
#define RAWDCF_KEEP		4
#if defined(RAWDCF_SETDTR)
static	int	rawdcf_init	P((struct parseunit *));
#define RAWDCF_INIT		rawdcf_init
#else
#define RAWDCF_INIT		NO_INIT
#endif

/*
 * RAW DCF variants
 */
/*
 * Conrad receiver
 *
 * simplest (cheapest) DCF clock - e. g. DCF77 receiver by Conrad
 * (~40DM - roughly $30 ) followed by a level converter for RS232
 */
#define CONRAD_BASEDELAY	0x420C49B0 /* ~258 ms - Conrad receiver @ 50 Baud on a Sun */
#define CONRAD_DESCRIPTION	"RAW DCF77 CODE (Conrad DCF77 receiver module)"

/*
 * TimeBrick receiver
 */
#define TIMEBRICK_BASEDELAY	0x35C29000 /* ~210 ms - TimeBrick @ 50 Baud on a Sun */
#define TIMEBRICK_DESCRIPTION	"RAW DCF77 CODE (TimeBrick)"

/*
 * IGEL:clock receiver
 */
#define IGELCLOCK_BASEDELAY	0x420C49B0 /* ~258 ms - IGEL:clock receiver */
#define IGELCLOCK_DESCRIPTION	"RAW DCF77 CODE (IGEL:clock)"
#define IGELCLOCK_SPEED		(B1200)
#define IGELCLOCK_CFLAG		(CS8|CREAD|HUPCL|CLOCAL)

/*
 * Trimble SV6 GPS receivers (TAIP and TSIP protocols)
 */
#define ETX	0x03
#define DLE	0x10

#ifndef TRIM_POLLRATE
#define TRIM_POLLRATE	0	/* only true direct polling */
#endif

#define TRIM_TAIPPOLLCMD	">SRM;FR_FLAG=F;EC_FLAG=F<>QTM<"
#define TRIM_TAIPCMDSIZE	(sizeof(TRIM_TAIPPOLLCMD)-1)

static poll_info_t trimbletaip_pollinfo = { TRIM_POLLRATE, TRIM_TAIPPOLLCMD, TRIM_TAIPCMDSIZE };
static	int	trimbletaip_init	P((struct parseunit *));
static	void	trimbletaip_event	P((struct parseunit *, int));

/* query time & UTC correction data */
static char tsipquery[] = { DLE, 0x21, DLE, ETX, DLE, 0x2F, DLE, ETX };

static poll_info_t trimbletsip_pollinfo = { TRIM_POLLRATE, tsipquery, sizeof(tsipquery) };
static	int	trimbletsip_init	P((struct parseunit *));

#define TRIMBLETAIP_SPEED	    (B4800)
#define TRIMBLETAIP_CFLAG           (CS8|CREAD|CLOCAL)
#define TRIMBLETAIP_IFLAG           (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON)
#define TRIMBLETAIP_OFLAG           (OPOST|ONLCR)
#define TRIMBLETAIP_LFLAG           (0)
#define TRIMBLETSIP_SPEED	    (B9600)
#define TRIMBLETSIP_CFLAG           (CS8|CLOCAL|CREAD|PARENB|PARODD)
#define TRIMBLETSIP_IFLAG           (IGNBRK)
#define TRIMBLETSIP_OFLAG           (0)
#define TRIMBLETSIP_LFLAG           (0)

#define TRIMBLETSIP_SAMPLES	    5
#define TRIMBLETSIP_KEEP	    3
#define TRIMBLETAIP_SAMPLES	    5
#define TRIMBLETAIP_KEEP	    3

#define TRIMBLETAIP_FLAGS	    (PARSE_F_PPSONSECOND)
#define TRIMBLETSIP_FLAGS	    (TRIMBLETAIP_FLAGS|PARSE_F_NOPOLLONLY)

#define TRIMBLETAIP_POLL	    poll_dpoll
#define TRIMBLETSIP_POLL	    poll_dpoll

#define TRIMBLETAIP_INIT	    trimbletaip_init
#define TRIMBLETSIP_INIT	    trimbletsip_init

#define TRIMBLETAIP_EVENT	    trimbletaip_event   

#define TRIMBLETAIP_END		    poll_end
#define TRIMBLETSIP_END		    poll_end

#define TRIMBLETAIP_DATA	    ((void *)(&trimbletaip_pollinfo))
#define TRIMBLETSIP_DATA	    ((void *)(&trimbletsip_pollinfo))

#define TRIMBLETAIP_ID		    GPS_ID
#define TRIMBLETSIP_ID		    GPS_ID

#define TRIMBLETAIP_FORMAT	    NO_FORMAT
#define TRIMBLETSIP_FORMAT	    "Trimble SV6/TSIP"

#define TRIMBLETAIP_ROOTDELAY        0x0
#define TRIMBLETSIP_ROOTDELAY        0x0

#define TRIMBLETAIP_BASEDELAY        0x0
#define TRIMBLETSIP_BASEDELAY        0x51EB852	/* 20 ms as a l_uf - avg GPS time message latency */

#define TRIMBLETAIP_DESCRIPTION      "Trimble GPS (TAIP) receiver"
#define TRIMBLETSIP_DESCRIPTION      "Trimble GPS (TSIP) receiver"

#define TRIMBLETAIP_MAXUNSYNC        0
#define TRIMBLETSIP_MAXUNSYNC        0

#define TRIMBLETAIP_EOL		    '<'

/*
 * RadioCode Clocks RCC 800 receiver
 */
#define RCC_POLLRATE   0       /* only true direct polling */
#define RCC_POLLCMD    "\r"
#define RCC_CMDSIZE    1

static poll_info_t rcc8000_pollinfo = { RCC_POLLRATE, RCC_POLLCMD, RCC_CMDSIZE };
#define RCC8000_FLAGS
#define RCC8000_POLL            poll_dpoll
#define RCC8000_INIT            poll_init
#define RCC8000_END             poll_end
#define RCC8000_DATA            ((void *)(&rcc8000_pollinfo))
#define RCC8000_ROOTDELAY       0x0
#define RCC8000_BASEDELAY       0x0
#define RCC8000_ID              "MSF"
#define RCC8000_DESCRIPTION     "RCC 8000 MSF Receiver"
#define RCC8000_FORMAT          NO_FORMAT
#define RCC8000_MAXUNSYNC       (60*60) /* should be ok for an hour */
#define RCC8000_SPEED		(B2400)
#define RCC8000_CFLAG           (CS8|CREAD|CLOCAL)
#define RCC8000_IFLAG           (IGNBRK|IGNPAR)
#define RCC8000_OFLAG           0
#define RCC8000_LFLAG           0
#define RCC8000_SAMPLES         5
#define RCC8000_KEEP	        3

/*
 * Hopf Radio clock 6021 Format 
 *
 */
#define HOPF6021_ROOTDELAY	0x00000000 /* 0 */
#define HOPF6021_BASEDELAY	0x00000000 /* 0 */
#define HOPF6021_DESCRIPTION	"HOPF 6021"
#define HOPF6021_FORMAT          NO_FORMAT
#define HOPF6021_MAXUNSYNC	(60*60)  /* should be ok for an hour */
#define HOPF6021_SPEED         (B9600)
#define HOPF6021_CFLAG          (CS8|CREAD|CLOCAL)
#define HOPF6021_IFLAG		(IGNBRK|ISTRIP)
#define HOPF6021_OFLAG		0
#define HOPF6021_LFLAG		0
#define HOPF6021_FLAGS          PARSE_F_NOPOLLONLY
#define HOPF6021_SAMPLES        5
#define HOPF6021_KEEP	        3

/*
 * Diem's Computime Radio Clock Receiver
 */
#define COMPUTIME_FLAGS       PARSE_F_NOPOLLONLY
#define COMPUTIME_ROOTDELAY   0x00000000  /* 0 */
#define COMPUTIME_BASEDELAY   0x00000000  /* 0 */
#define COMPUTIME_ID          DCF_ID
#define COMPUTIME_DESCRIPTION "Diem's Computime receiver"
#define COMPUTIME_FORMAT      "Diem's Computime Radio Clock"
#define COMPUTIME_TYPE        DCF_TYPE
#define COMPUTIME_MAXUNSYNC   (60*60)       /* only trust clock for 1 hour */
#define COMPUTIME_SPEED       (B9600)
#define COMPUTIME_CFLAG       (CSTOPB|CS7|CREAD|CLOCAL)
#define COMPUTIME_IFLAG       (IGNBRK|IGNPAR|ISTRIP)
#define COMPUTIME_OFLAG       0
#define COMPUTIME_LFLAG       0
#define COMPUTIME_SAMPLES     5
#define COMPUTIME_KEEP        3

static struct parse_clockinfo
{
  u_long  cl_flags;		/* operation flags (io modes) */
  void  (*cl_poll)();		/* active poll routine */
  int   (*cl_init)();		/* active poll init routine */
  void  (*cl_event)();		/* special event handling (e.g. reset clock) */
  void  (*cl_end)();		/* active poll end routine */
  void   *cl_data;		/* local data area for "poll" mechanism */
  u_fp    cl_rootdelay;		/* rootdelay */
  u_long  cl_basedelay;		/* current offset - unsigned l_fp fractional part */
  u_long  cl_ppsdelay;		/* current PPS offset - unsigned l_fp fractional part */
  char   *cl_id;		/* ID code */
  char   *cl_description;	/* device name */
  char   *cl_format;		/* fixed format */
  u_char  cl_type;		/* clock type (ntp control) */
  u_long  cl_maxunsync;		/* time to trust oscillator after loosing synch */
  u_long  cl_speed;		/* terminal input & output baudrate */
  u_long  cl_cflag;             /* terminal control flags */
  u_long  cl_iflag;             /* terminal input flags */
  u_long  cl_oflag;             /* terminal output flags */
  u_long  cl_lflag;             /* terminal local flags */
  u_long  cl_samples;		/* samples for median filter */
  u_long  cl_keep;		/* samples for median filter to keep */
} parse_clockinfo[] =
{
  {				/* mode 0 */
    MBG_FLAGS,
    NO_POLL,
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    DCFPZF535_ROOTDELAY,
    DCFPZF535_BASEDELAY,
    NO_PPSDELAY,
    DCF_P_ID,
    DCFPZF535_DESCRIPTION,
    NO_FORMAT,
    DCF_TYPE,
    DCFPZF535_MAXUNSYNC,
    DCFPZF535_SPEED,
    DCFPZF535_CFLAG,
    DCFPZF535_IFLAG,
    DCFPZF535_OFLAG,
    DCFPZF535_LFLAG,
    DCFPZF535_SAMPLES,
    DCFPZF535_KEEP
  },
  {				/* mode 1 */

    MBG_FLAGS,
    NO_POLL,
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    DCFPZF535OCXO_ROOTDELAY,
    DCFPZF535OCXO_BASEDELAY,
    NO_PPSDELAY,
    DCF_P_ID,
    DCFPZF535OCXO_DESCRIPTION,
    NO_FORMAT,
    DCF_TYPE,
    DCFPZF535OCXO_MAXUNSYNC,
    DCFPZF535OCXO_SPEED,
    DCFPZF535OCXO_CFLAG,
    DCFPZF535OCXO_IFLAG,
    DCFPZF535OCXO_OFLAG,
    DCFPZF535OCXO_LFLAG,
    DCFPZF535OCXO_SAMPLES,
    DCFPZF535OCXO_KEEP
  },
  {				/* mode 2 */
    MBG_FLAGS,
    NO_POLL,
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    DCFUA31_ROOTDELAY,
    DCFUA31_BASEDELAY,
    NO_PPSDELAY,
    DCF_A_ID,
    DCFUA31_DESCRIPTION,
    NO_FORMAT,
    DCF_TYPE,
    DCFUA31_MAXUNSYNC,
    DCFUA31_SPEED,
    DCFUA31_CFLAG,
    DCFUA31_IFLAG,
    DCFUA31_OFLAG,
    DCFUA31_LFLAG,
    DCFUA31_SAMPLES,
    DCFUA31_KEEP
  },
  {				/* mode 3 */
    MBG_FLAGS,
    NO_POLL,
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    DCF7000_ROOTDELAY,
    DCF7000_BASEDELAY,
    NO_PPSDELAY,
    DCF_A_ID,
    DCF7000_DESCRIPTION,
    NO_FORMAT,
    DCF_TYPE,
    DCF7000_MAXUNSYNC,
    DCF7000_SPEED,
    DCF7000_CFLAG,
    DCF7000_IFLAG,
    DCF7000_OFLAG,
    DCF7000_LFLAG,
    DCF7000_SAMPLES,
    DCF7000_KEEP
  },
  {				/* mode 4 */
    NO_FLAGS,
    WSDCF_POLL,
    WSDCF_INIT,
    NO_EVENT,
    WSDCF_END,
    WSDCF_DATA,
    WSDCF_ROOTDELAY,
    WSDCF_BASEDELAY,
    NO_PPSDELAY,
    DCF_A_ID,
    WSDCF_DESCRIPTION,
    WSDCF_FORMAT,
    DCF_TYPE,
    WSDCF_MAXUNSYNC,
    WSDCF_SPEED,
    WSDCF_CFLAG,
    WSDCF_IFLAG,
    WSDCF_OFLAG,
    WSDCF_LFLAG,
    WSDCF_SAMPLES,
    WSDCF_KEEP
  },
  {				/* mode 5 */
    RAWDCF_FLAGS,
    NO_POLL,
    RAWDCF_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    RAWDCF_ROOTDELAY,
    CONRAD_BASEDELAY,
    NO_PPSDELAY,
    DCF_A_ID,
    CONRAD_DESCRIPTION,
    RAWDCF_FORMAT,
    DCF_TYPE,
    RAWDCF_MAXUNSYNC,
    RAWDCF_SPEED,
    RAWDCF_CFLAG,
    RAWDCF_IFLAG,
    RAWDCF_OFLAG,
    RAWDCF_LFLAG,
    RAWDCF_SAMPLES,
    RAWDCF_KEEP
  },
  {				/* mode 6 */
    RAWDCF_FLAGS,
    NO_POLL,
    RAWDCF_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    RAWDCF_ROOTDELAY,
    TIMEBRICK_BASEDELAY,
    NO_PPSDELAY,
    DCF_A_ID,
    TIMEBRICK_DESCRIPTION,
    RAWDCF_FORMAT,
    DCF_TYPE,
    RAWDCF_MAXUNSYNC,
    RAWDCF_SPEED,
    RAWDCF_CFLAG,
    RAWDCF_IFLAG,
    RAWDCF_OFLAG,
    RAWDCF_LFLAG,
    RAWDCF_SAMPLES,
    RAWDCF_KEEP
  },
  {				/* mode 7 */
    MBG_FLAGS,
    GPS166_POLL,
    GPS166_INIT,
    NO_EVENT,
    GPS166_END,
    GPS166_DATA,
    GPS166_ROOTDELAY,
    GPS166_BASEDELAY,
    NO_PPSDELAY,
    GPS166_ID,
    GPS166_DESCRIPTION,
    GPS166_FORMAT,
    GPS_TYPE,
    GPS166_MAXUNSYNC,
    GPS166_SPEED,
    GPS166_CFLAG,
    GPS166_IFLAG,
    GPS166_OFLAG,
    GPS166_LFLAG,
    GPS166_SAMPLES,
    GPS166_KEEP
  },
  {				/* mode 8 */
    RAWDCF_FLAGS,
    NO_POLL,
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    RAWDCF_ROOTDELAY,
    IGELCLOCK_BASEDELAY,
    NO_PPSDELAY,
    DCF_A_ID,
    IGELCLOCK_DESCRIPTION,
    RAWDCF_FORMAT,
    DCF_TYPE,
    RAWDCF_MAXUNSYNC,
    IGELCLOCK_SPEED,
    IGELCLOCK_CFLAG,
    RAWDCF_IFLAG,
    RAWDCF_OFLAG,
    RAWDCF_LFLAG,
    RAWDCF_SAMPLES,
    RAWDCF_KEEP
  },
  {				/* mode 9 */
    TRIMBLETAIP_FLAGS,
#if TRIM_POLLRATE		/* DHD940515: Allow user config */
    NO_POLL,
#else
    TRIMBLETAIP_POLL,
#endif
    TRIMBLETAIP_INIT,
    TRIMBLETAIP_EVENT,
    TRIMBLETAIP_END,
    TRIMBLETAIP_DATA,
    TRIMBLETAIP_ROOTDELAY,
    TRIMBLETAIP_BASEDELAY,
    NO_PPSDELAY,
    TRIMBLETAIP_ID,
    TRIMBLETAIP_DESCRIPTION,
    TRIMBLETAIP_FORMAT,
    GPS_TYPE,
    TRIMBLETAIP_MAXUNSYNC,
    TRIMBLETAIP_SPEED,
    TRIMBLETAIP_CFLAG,
    TRIMBLETAIP_IFLAG,
    TRIMBLETAIP_OFLAG,
    TRIMBLETAIP_LFLAG,
    TRIMBLETAIP_SAMPLES,
    TRIMBLETAIP_KEEP
  },
  {				/* mode 10 */
    TRIMBLETSIP_FLAGS,
#if TRIM_POLLRATE		/* DHD940515: Allow user config */
    NO_POLL,
#else
    TRIMBLETSIP_POLL,
#endif
    TRIMBLETSIP_INIT,
    NO_EVENT,
    TRIMBLETSIP_END,
    TRIMBLETSIP_DATA,
    TRIMBLETSIP_ROOTDELAY,
    TRIMBLETSIP_BASEDELAY,
    NO_PPSDELAY,
    TRIMBLETSIP_ID,
    TRIMBLETSIP_DESCRIPTION,
    TRIMBLETSIP_FORMAT,
    GPS_TYPE,
    TRIMBLETSIP_MAXUNSYNC,
    TRIMBLETSIP_SPEED,
    TRIMBLETSIP_CFLAG,
    TRIMBLETSIP_IFLAG,
    TRIMBLETSIP_OFLAG,
    TRIMBLETSIP_LFLAG,
    TRIMBLETSIP_SAMPLES,
    TRIMBLETSIP_KEEP
  },
  {                             /* mode 11 */
    NO_FLAGS,
    RCC8000_POLL,
    RCC8000_INIT,
    NO_EVENT,
    RCC8000_END,
    RCC8000_DATA,
    RCC8000_ROOTDELAY,
    RCC8000_BASEDELAY,
    NO_PPSDELAY,
    RCC8000_ID,
    RCC8000_DESCRIPTION,
    RCC8000_FORMAT,
    DCF_TYPE,
    RCC8000_MAXUNSYNC,
    RCC8000_SPEED,
    RCC8000_CFLAG,
    RCC8000_IFLAG,
    RCC8000_OFLAG,
    RCC8000_LFLAG,
    RCC8000_SAMPLES,
    RCC8000_KEEP
  },
  {                             /* mode 12 */
    HOPF6021_FLAGS,
    NO_POLL,     
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    HOPF6021_ROOTDELAY,
    HOPF6021_BASEDELAY,
    NO_PPSDELAY,
    DCF_ID,
    HOPF6021_DESCRIPTION,
    HOPF6021_FORMAT,
    DCF_TYPE,
    HOPF6021_MAXUNSYNC,
    HOPF6021_SPEED,
    HOPF6021_CFLAG,
    HOPF6021_IFLAG,
    HOPF6021_OFLAG,
    HOPF6021_LFLAG,
    HOPF6021_SAMPLES,
    HOPF6021_KEEP
  },
  {                            /* mode 13 */
    COMPUTIME_FLAGS,
    NO_POLL,
    NO_INIT,
    NO_EVENT,
    NO_END,
    NO_DATA,
    COMPUTIME_ROOTDELAY,
    COMPUTIME_BASEDELAY,
    NO_PPSDELAY,
    COMPUTIME_ID,
    COMPUTIME_DESCRIPTION,
    COMPUTIME_FORMAT,
    COMPUTIME_TYPE,
    COMPUTIME_MAXUNSYNC,
    COMPUTIME_SPEED,
    COMPUTIME_CFLAG,
    COMPUTIME_IFLAG,
    COMPUTIME_OFLAG,
    COMPUTIME_LFLAG,
    COMPUTIME_SAMPLES,
    COMPUTIME_KEEP
  }
};

static int ncltypes = sizeof(parse_clockinfo) / sizeof(struct parse_clockinfo);

#define CLK_REALTYPE(x) ((int)(((x)->ttl) & 0x7F))
#define CLK_TYPE(x)	((CLK_REALTYPE(x) >= ncltypes) ? ~0 : CLK_REALTYPE(x))
#define CLK_UNIT(x)	(REFCLOCKUNIT(&(x)->srcadr))
#define CLK_PPS(x)	(((x)->ttl) & 0x80)

/*
 * Other constant stuff
 */
#define	PARSEHSREFID	0x7f7f08ff	/* 127.127.8.255 refid for hi strata */

#define PARSESTATISTICS   (60*60)	        /* output state statistics every hour */

static struct parseunit *parseunits[MAXUNITS];

extern u_long current_time;
extern s_char sys_precision;
extern struct event timerqueue[];
#ifdef ATOM
extern int fdpps;
extern int pps_sample P((l_fp *));
#endif

static int notice = 0;

#define PARSE_STATETIME(parse, i) ((parse->generic->currentstatus == i) ? parse->statetime[i] + current_time - parse->lastchange : parse->statetime[i])

static void parse_event   P((struct parseunit *, int));
static void parse_process P((struct parseunit *, parsetime_t *));
static void clear_err     P((struct parseunit *, u_long));
static int  list_err      P((struct parseunit *, u_long));
static char * l_mktime    P((u_long));

/**===========================================================================
 ** implementation error message regression module
 **/
static void
clear_err(parse, state)
     struct parseunit *parse;
     u_long            state;
{
  if (state == ERR_ALL)
    {
      int i;

      for (i = 0; i < ERR_CNT; i++)
	{
	  parse->errors[i].err_stage   = err_tbl[i];
	  parse->errors[i].err_cnt     = 0;
	  parse->errors[i].err_last    = 0;
	  parse->errors[i].err_started = 0;
	  parse->errors[i].err_suppressed = 0;
	}
    }
  else
    {
      parse->errors[state].err_stage   = err_tbl[state];
      parse->errors[state].err_cnt     = 0;
      parse->errors[state].err_last    = 0;
      parse->errors[state].err_started = 0;
      parse->errors[state].err_suppressed = 0;
    }
}

static int
list_err(parse, state)
     struct parseunit *parse;
     u_long            state;
{
  int do_it;
  struct errorinfo *err = &parse->errors[state];

  if (err->err_started == 0)
    {
      err->err_started = current_time;
    }

  do_it = (current_time - err->err_last) >= err->err_stage->err_delay;

  if (do_it)
    err->err_cnt++;
  
  if (err->err_stage->err_count &&
      (err->err_cnt >= err->err_stage->err_count))
    {
      err->err_stage++;
      err->err_cnt = 0;
    }

  if (!err->err_cnt && do_it)
    msyslog(LOG_INFO, "PARSE receiver #%d: interval for following error message class is at least %s",
	   CLK_UNIT(parse->peer), l_mktime(err->err_stage->err_delay));

  if (!do_it)
    err->err_suppressed++;
  else
    err->err_last = current_time;

  if (do_it && err->err_suppressed)
    {
      msyslog(LOG_INFO, "PARSE receiver #%d: %d message%s suppressed, error condition class persists for %s",
	     CLK_UNIT(parse->peer), err->err_suppressed, (err->err_suppressed == 1) ? " was" : "s where",
	     l_mktime(current_time - err->err_started));
      err->err_suppressed = 0;
    }
  
  return do_it;
}

/**===========================================================================
 ** implementation of i/o handling methods
 ** (all STREAM, partial STREAM, user level)
 **/

/*
 * define possible io handling methods
 */
#ifdef STREAM
static int  ppsclock_init   P((struct parseunit *));
static int  stream_init     P((struct parseunit *));
static void stream_end      P((struct parseunit *));
static int  stream_enable   P((struct parseunit *));
static int  stream_disable  P((struct parseunit *));
static int  stream_setcs    P((struct parseunit *, parsectl_t *));
static int  stream_getfmt   P((struct parseunit *, parsectl_t *));
static int  stream_setfmt   P((struct parseunit *, parsectl_t *));
static int  stream_timecode P((struct parseunit *, parsectl_t *));
static void stream_receive  P((struct recvbuf *));
static void stream_poll     P((struct parseunit *));
#endif
					 
static int  local_init     P((struct parseunit *));
static void local_end      P((struct parseunit *));
static int  local_nop      P((struct parseunit *));
static int  local_setcs    P((struct parseunit *, parsectl_t *));
static int  local_getfmt   P((struct parseunit *, parsectl_t *));
static int  local_setfmt   P((struct parseunit *, parsectl_t *));
static int  local_timecode P((struct parseunit *, parsectl_t *));
static void local_receive  P((struct recvbuf *));
static void local_poll     P((struct parseunit *));

static bind_t io_bindings[] =
{
#ifdef STREAM
  {
    "parse STREAM",
    stream_init,
    stream_end,
    stream_setcs,
    stream_disable,
    stream_enable,
    stream_getfmt,
    stream_setfmt,
    stream_timecode,
    stream_receive,
    stream_poll
  },
  {
    "ppsclock STREAM",
    ppsclock_init,
    local_end,
    local_setcs,
    local_nop,
    local_nop,
    local_getfmt,
    local_setfmt,
    local_timecode,
    local_receive,
    local_poll
  },
#endif
  {
    "normal",
    local_init,
    local_end,
    local_setcs,
    local_nop,
    local_nop,
    local_getfmt,
    local_setfmt,
    local_timecode,
    local_receive,
    local_poll
  },
  {
    (char *)0,
  }
};

#ifdef STREAM

#define fix_ts(_X_) \
  if ((&(_X_))->tv.tv_usec >= 1000000)                \
    {                                                 \
      (&(_X_))->tv.tv_usec -= 1000000;                \
      (&(_X_))->tv.tv_sec  += 1;                      \
    }

#define cvt_ts(_X_, _Y_) \
  {                                                   \
    l_fp ts;                                          \
                                                      \
    fix_ts((_X_));                                    \
    if (!buftvtots((const char *)&(&(_X_))->tv, &ts)) \
      {                                               \
	ERR(ERR_BADDATA)			      \
	  msyslog(LOG_ERR,"parse: stream_receive: timestamp conversion error (buftvtots) (%s) (%d.%06d) ", (_Y_), (&(_X_))->tv.tv_sec, (&(_X_))->tv.tv_usec);\
	return;                                       \
      }                                               \
    else                                              \
      {                                               \
	(&(_X_))->fp = ts;                            \
      }                                               \
  }

/*--------------------------------------------------
 * ppsclock STREAM init
 */
static int
ppsclock_init(parse)
     struct parseunit *parse;
{
  /*
   * now push the parse streams module
   * it will ensure exclusive access to the device
   */
  if (ioctl(parse->generic->io.fd, I_PUSH, (caddr_t)"ppsclocd") == -1 &&
      ioctl(parse->generic->io.fd, I_PUSH, (caddr_t)"ppsclock") == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: ppsclock_init: ioctl(fd, I_PUSH, \"ppsclock\"): %m",
	     CLK_UNIT(parse->peer));
      return 0;
    }
  if (!local_init(parse))
    {
      (void)ioctl(parse->generic->io.fd, I_POP, (caddr_t)0);
      return 0;
    }

  parse->flags |= PARSE_PPSCLOCK;
  return 1;
}

/*--------------------------------------------------
 * parse STREAM init
 */
static int
stream_init(parse)
     struct parseunit *parse;
{
  /*
   * now push the parse streams module
   * to test whether it is there (Oh boy - neat kernel interface)
   */
  if (ioctl(parse->generic->io.fd, I_PUSH, (caddr_t)"parse") == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: stream_init: ioctl(fd, I_PUSH, \"parse\"): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  else
    {
      while(ioctl(parse->generic->io.fd, I_POP, (caddr_t)0) == 0)
	/* empty loop */;

      /*
       * now push it a second time after we have removed all
       * module garbage
       */
      if (ioctl(parse->generic->io.fd, I_PUSH, (caddr_t)"parse") == -1)
	{
	  msyslog(LOG_ERR, "PARSE receiver #%d: stream_init: ioctl(fd, I_PUSH, \"parse\"): %m", CLK_UNIT(parse->peer));
	  return 0;
	}
      else
	{
	  return 1;
        }
    }
}

/*--------------------------------------------------
 * parse STREAM end
 */
static void
stream_end(parse)
     struct parseunit *parse;
{
  while(ioctl(parse->generic->io.fd, I_POP, (caddr_t)0) == 0)
    /* empty loop */;
}

 /*--------------------------------------------------
 * STREAM setcs
 */
static int
stream_setcs(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  struct strioctl strioc;
  
  strioc.ic_cmd     = PARSEIOC_SETCS;
  strioc.ic_timout  = 0;
  strioc.ic_dp      = (char *)tcl;
  strioc.ic_len     = sizeof (*tcl);

  if (ioctl(parse->generic->io.fd, I_STR, (caddr_t)&strioc) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: stream_setcs: ioctl(fd, I_STR, PARSEIOC_SETCS): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  return 1;
}

/*--------------------------------------------------
 * STREAM enable
 */
static int
stream_enable(parse)
     struct parseunit *parse;
{
  struct strioctl strioc;
  
  strioc.ic_cmd     = PARSEIOC_ENABLE;
  strioc.ic_timout  = 0;
  strioc.ic_dp      = (char *)0;
  strioc.ic_len     = 0;

  if (ioctl(parse->generic->io.fd, I_STR, (caddr_t)&strioc) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: stream_enable: ioctl(fd, I_STR, PARSEIOC_ENABLE): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  parse->generic->io.clock_recv = stream_receive; /* ok - parse input in kernel */
  return 1;
}

/*--------------------------------------------------
 * STREAM disable
 */
static int
stream_disable(parse)
     struct parseunit *parse;
{
  struct strioctl strioc;
  
  strioc.ic_cmd     = PARSEIOC_DISABLE;
  strioc.ic_timout  = 0;
  strioc.ic_dp      = (char *)0;
  strioc.ic_len     = 0;

  if (ioctl(parse->generic->io.fd, I_STR, (caddr_t)&strioc) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: stream_disable: ioctl(fd, I_STR, PARSEIOC_DISABLE): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  parse->generic->io.clock_recv = local_receive; /* ok - parse input in daemon */
  return 1;
}

/*--------------------------------------------------
 * STREAM getfmt
 */
static int
stream_getfmt(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  struct strioctl strioc;
  
  strioc.ic_cmd     = PARSEIOC_GETFMT;
  strioc.ic_timout  = 0;
  strioc.ic_dp      = (char *)tcl;
  strioc.ic_len     = sizeof (*tcl);
  if (ioctl(parse->generic->io.fd, I_STR, (caddr_t)&strioc) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: ioctl(fd, I_STR, PARSEIOC_GETFMT): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  return 1;
}

/*--------------------------------------------------
 * STREAM setfmt
 */
static int
stream_setfmt(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  struct strioctl strioc;
  
  strioc.ic_cmd     = PARSEIOC_SETFMT;
  strioc.ic_timout  = 0;
  strioc.ic_dp      = (char *)tcl;
  strioc.ic_len     = sizeof (*tcl);

  if (ioctl(parse->generic->io.fd, I_STR, (caddr_t)&strioc) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: stream_setfmt: ioctl(fd, I_STR, PARSEIOC_SETFMT): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  return 1;
}


/*--------------------------------------------------
 * STREAM timecode
 */
static int
stream_timecode(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  struct strioctl strioc;
  
  strioc.ic_cmd     = PARSEIOC_TIMECODE;
  strioc.ic_timout  = 0;
  strioc.ic_dp      = (char *)tcl;
  strioc.ic_len     = sizeof (*tcl);
	
  if (ioctl(parse->generic->io.fd, I_STR, (caddr_t)&strioc) == -1)
    {
      ERR(ERR_INTERNAL)
	msyslog(LOG_ERR, "PARSE receiver #%d: stream_timecode: ioctl(fd, I_STR, PARSEIOC_TIMECODE): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  clear_err(parse, ERR_INTERNAL);
  return 1;
}

/*--------------------------------------------------
 * STREAM receive
 */
static void
stream_receive(rbufp)
     struct recvbuf *rbufp;
{
  struct parseunit *parse = (struct parseunit *)rbufp->recv_srcclock;
  parsetime_t parsetime;

  if (!parse->peer)
    return;

  if (rbufp->recv_length != sizeof(parsetime_t))
    {
      ERR(ERR_BADIO)
	msyslog(LOG_ERR,"PARSE receiver #%d: parse_receive: bad size (got %d expected %d)",
	     CLK_UNIT(parse->peer), rbufp->recv_length, sizeof(parsetime_t));
      parse->generic->baddata++;
      parse_event(parse, CEVNT_BADREPLY);
      return;
    }
  clear_err(parse, ERR_BADIO);
  
  memmove((caddr_t)&parsetime,
	  (caddr_t)rbufp->recv_buffer,
	  sizeof(parsetime_t));

  /*
   * switch time stamp world - be sure to normalize small usec field
   * errors.
   */

  cvt_ts(parsetime.parse_stime, "parse_stime");

  if (PARSE_TIMECODE(parsetime.parse_state))
    {
      cvt_ts(parsetime.parse_time, "parse_time");
    }

  if (PARSE_PPS(parsetime.parse_state))
    cvt_ts(parsetime.parse_ptime, "parse_ptime");

  parse_process(parse, &parsetime);
}

/*--------------------------------------------------
 * STREAM poll
 */
static void
stream_poll(parse)
     struct parseunit *parse;
{
  register int fd, i, rtc;
  fd_set fdmask;
  struct timeval timeout, starttime, curtime, selecttime;
#ifdef HAVE_GETCLOCK
  struct timespec ts;
#endif
  parsetime_t parsetime;

  /*
   * now we do the following:
   *    - read the first packet from the parse module  (OLD !!!)
   *    - read the second packet from the parse module (fresh)
   *    - compute values for xntp
   */
	
  FD_ZERO(&fdmask);
  fd = parse->generic->io.fd;
  FD_SET(fd, &fdmask);
  timeout.tv_sec = 0;
  timeout.tv_usec = 500000;	/* 0.5 sec */

  if (parse->parse_type->cl_poll)
    {
      parse->parse_type->cl_poll(parse);
    }

#ifdef HAVE_GETCLOCK
  if (getclock(TIMEOFDAY, &ts) == -1)
    {
      msyslog(LOG_ERR,"getclock failed: %m");
      exit(1);
    }
  starttime.tv_sec = ts.tv_sec;
  starttime.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
  if (GETTIMEOFDAY(&starttime, 0L) == -1)
    {
      msyslog(LOG_ERR,"gettimeofday failed: %m");
      exit(1);
    }
#endif /* not HAVE_GETCLOCK */

  selecttime = timeout;

  while ((rtc = select(fd + 1, &fdmask, 0, 0, &selecttime)) != 1)
    {
      /* no data from the radio clock */

      if (rtc == -1)
	{
	  if (errno == EINTR)
	    {
#ifdef HAVE_GETCLOCK
	      if (getclock(TIMEOFDAY, &ts) == -1)
                {
                  msyslog(LOG_ERR,"getclock failed: %m");
                  exit(1);
                }
	      curtime.tv_sec = ts.tv_sec;
	      curtime.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
	      if (GETTIMEOFDAY(&curtime, 0L) == -1)
		{
		  msyslog(LOG_ERR,"gettimeofday failed: %m");
		  exit(1);
		}
#endif /* not HAVE_GETCLOCK */
	      selecttime.tv_sec = curtime.tv_sec - starttime.tv_sec;
	      if (curtime.tv_usec < starttime.tv_usec)
		{
		  selecttime.tv_sec  -= 1;
		  selecttime.tv_usec  = 1000000 + curtime.tv_usec - starttime.tv_usec;
		}
	      else
		{
		  selecttime.tv_usec = curtime.tv_usec - starttime.tv_usec;
		}
	

	      if (timercmp(&selecttime, &timeout, >))
		{
		  /*
		   * elapsed real time passed timeout value - consider it timed out
		   */
		  break;
		}

	      /*
	       * calculate residual timeout value
	       */
	      selecttime.tv_sec = timeout.tv_sec - selecttime.tv_sec;

	      if (selecttime.tv_usec > timeout.tv_usec)
		{
		  selecttime.tv_sec -= 1;
		  selecttime.tv_usec = 1000000 + timeout.tv_usec - selecttime.tv_usec;
		}
	      else
		{
		  selecttime.tv_usec = timeout.tv_usec - selecttime.tv_usec;
		}
	
	      FD_SET(fd, &fdmask);
	      continue;
	    }
	  else
	    {
	      ERR(ERR_BADIO)
		msyslog(LOG_WARNING, "PARSE receiver #%d: no data[old] from device (select() error: %m)", CLK_UNIT(parse->peer));
	    }
	}
      else
	{
	  ERR(ERR_NODATA)
	    msyslog(LOG_WARNING, "PARSE receiver #%d: no data[old] from device (check receiver / cableling)", CLK_UNIT(parse->peer));
	}
      parse->generic->noreply++;
      parse->lastmissed = current_time;
      parse_event(parse, CEVNT_TIMEOUT);

      return;
    }

  while (((i = read(fd, (char *)&parsetime, sizeof(parsetime))) < sizeof(parsetime)))
    {
      /* bad packet */
      if ( i == -1)
	{
	  if (errno == EINTR)
	    {
	      continue;
	    }
	  else
	    {
	      ERR(ERR_BADIO)
		msyslog(LOG_WARNING, "PARSE receiver #%d: bad read[old] from streams module (read() error: %m)", CLK_UNIT(parse->peer), i, sizeof(parsetime));
	    }
	}
      else
	{
	  ERR(ERR_BADIO)
	    msyslog(LOG_WARNING, "PARSE receiver #%d: bad read[old] from streams module (got %d bytes - expected %d bytes)", CLK_UNIT(parse->peer), i, sizeof(parsetime));
	}
      parse->generic->baddata++;
      parse_event(parse, CEVNT_BADREPLY);

      return;
    }

  if (parse->parse_type->cl_poll)
    {
      parse->parse_type->cl_poll(parse);
    }

  timeout.tv_sec = 1;
  timeout.tv_usec = 500000;	/* 1.500 sec */
  FD_ZERO(&fdmask);
  FD_SET(fd, &fdmask);

#ifdef HAVE_GETCLOCK
  if (getclock(TIMEOFDAY, &ts) == -1)
    {
      msyslog(LOG_ERR,"getclock failed: %m");
      exit(1);
    }
  starttime.tv_sec = ts.tv_sec;
  starttime.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
  if (GETTIMEOFDAY(&starttime, 0L) == -1)
    {
      msyslog(LOG_ERR,"gettimeofday failed: %m");
      exit(1);
    }
#endif /* not HAVE_GETCLOCK */

  selecttime = timeout;

  while ((rtc = select(fd + 1, &fdmask, 0, 0, &selecttime)) != 1)
    {
      /* no data from the radio clock */

      if (rtc == -1)
	{
	  if (errno == EINTR)
	    {
#ifdef HAVE_GETCLOCK
	      if (getclock(TIMEOFDAY, &ts) == -1)
                {
                  msyslog(LOG_ERR, "getclock failed: %m");
                  exit(1);
                }
	      curtime.tv_sec = ts.tv_sec;
	      curtime.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
	      if (GETTIMEOFDAY(&curtime, 0L) == -1)
		{
		  msyslog(LOG_ERR,"gettimeofday failed: %m");
		  exit(1);
		}
#endif /* not HAVE_GETCLOCK */
	      selecttime.tv_sec = curtime.tv_sec - starttime.tv_sec;
	      if (curtime.tv_usec < starttime.tv_usec)
		{
		  selecttime.tv_sec  -= 1;
		  selecttime.tv_usec  = 1000000 + curtime.tv_usec - starttime.tv_usec;
		}
	      else
		{
		  selecttime.tv_usec = curtime.tv_usec - starttime.tv_usec;
		}
	

	      if (timercmp(&selecttime, &timeout, >))
		{
		  /*
		   * elapsed real time passed timeout value - consider it timed out
		   */
		  break;
		}

	      /*
	       * calculate residual timeout value
	       */
	      selecttime.tv_sec = timeout.tv_sec - selecttime.tv_sec;

	      if (selecttime.tv_usec > timeout.tv_usec)
		{
		  selecttime.tv_sec -= 1;
		  selecttime.tv_usec = 1000000 + timeout.tv_usec - selecttime.tv_usec;
		}
	      else
		{
		  selecttime.tv_usec = timeout.tv_usec - selecttime.tv_usec;
		}
	
	      FD_SET(fd, &fdmask);
	      continue;
	    }
	  else
	    {
	      ERR(ERR_BADIO)
		msyslog(LOG_WARNING, "PARSE receiver #%d: no data[new] from device (select() error: %m)", CLK_UNIT(parse->peer));
	    }
	}
      else
	{
	  ERR(ERR_NODATA)
	    msyslog(LOG_WARNING, "PARSE receiver #%d: no data[new] from device (check receiver / cableling) ", CLK_UNIT(parse->peer));
	}
	
      /*
       * we will return here iff we got a good old sample as this would
       * be misinterpreted. bad samples are passed on to be logged into the
       * state statistics
       */
      if ((parsetime.parse_status & CVT_MASK) == CVT_OK)
	{
	  parse->generic->noreply++;
	  parse->lastmissed = current_time;
	  parse_event(parse, CEVNT_TIMEOUT);
	  return;
	}
    }

  /*
   * we get here either by a possible read() (rtc == 1 - while assertion)
   * or by a timeout or a system call error. when a read() is possible we
   * get the new data, otherwise we stick with the old
   */
  if ((rtc == 1) && ((i = read(fd, (char *)&parsetime, sizeof(parsetime))) < sizeof(parsetime)))
    {
      /* bad packet */
      if ( i== -1)
	{
	  ERR(ERR_BADIO)
	    msyslog(LOG_WARNING, "PARSE receiver #%d: bad read[new] from streams module (read() error: %m)", CLK_UNIT(parse->peer), i, sizeof(parsetime));
	}
      else
	{
	  ERR(ERR_BADIO)
	    msyslog(LOG_WARNING, "PARSE receiver #%d: bad read[new] from streams module (got %d bytes - expected %d bytes)", CLK_UNIT(parse->peer), i, sizeof(parsetime));
	}
      parse->generic->baddata++;
      parse_event(parse, CEVNT_BADREPLY);

      return;
    }

  cvt_ts(parsetime.parse_stime, "parse_stime");

  if (PARSE_TIMECODE(parsetime.parse_state))
    {
      cvt_ts(parsetime.parse_time, "parse_time");
    }

  if (PARSE_PPS(parsetime.parse_state))
    cvt_ts(parsetime.parse_ptime, "parse_ptime");

  /*
   * process what we got
   */
  parse_process(parse, &parsetime);
}
#endif

/*--------------------------------------------------
 * local init
 */
static int
local_init(parse)
     struct parseunit *parse;
{
  return parse_ioinit(&parse->parseio);
}

/*--------------------------------------------------
 * local end
 */
static void
local_end(parse)
     struct parseunit *parse;
{
  parse_ioend(&parse->parseio);
}


/*--------------------------------------------------
 * local nop
 */
static int
local_nop(parse)
     struct parseunit *parse;
{
  return 1;
}

/*--------------------------------------------------
 * local setcs
 */
static int
local_setcs(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  return parse_setcs(tcl, &parse->parseio);
}

/*--------------------------------------------------
 * local getfmt
 */
static int
local_getfmt(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  return parse_getfmt(tcl, &parse->parseio);
}

/*--------------------------------------------------
 * local setfmt
 */
static int
local_setfmt(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  return parse_setfmt(tcl, &parse->parseio);
}

/*--------------------------------------------------
 * local timecode
 */
static int
local_timecode(parse, tcl)
     struct parseunit *parse;
     parsectl_t  *tcl;
{
  return parse_timecode(tcl, &parse->parseio);
}


/*--------------------------------------------------
 * local receive
 */
static void
local_receive(rbufp)
     struct recvbuf *rbufp;
{
  struct parseunit *parse = (struct parseunit *)rbufp->recv_srcclock;
  register int count;
  register unsigned char *s;
  timestamp_t ts;

  if (!parse->peer)
    return;

  /*
   * eat all characters, parsing then and feeding complete samples
   */
  count = rbufp->recv_length;
  s = (unsigned char *)rbufp->recv_buffer;
  ts.fp = rbufp->recv_time;

  while (count--)
    {
      if (parse_ioread(&parse->parseio, (unsigned int)(*s++), &ts))
	{
	  /*
	   * got something good to eat
	   */
#ifdef PPS
	  if (!PARSE_PPS(parse->parseio.parse_dtime.parse_state) &&
	      (parse->flags & PARSE_PPSCLOCK))
	    {
	      l_fp ts;
	      struct ppsclockev ev;

	      if (ioctl(parse->generic->io.fd, CIOGETEV, (caddr_t)&ev) == 0)
		{
		  if (ev.serial != parse->ppsserial)
		    {
		      /*
                       * add PPS time stamp if available via ppsclock module
		       * and not supplied already.
		       */
		      if (!buftvtots((const char *)&ev.tv, &ts))
			{
			  ERR(ERR_BADDATA)
			    msyslog(LOG_ERR,"parse: local_receive: timestamp conversion error (buftvtots) (ppsclockev.tv)");
			}
		      else
			{
		          parse->parseio.parse_dtime.parse_ptime.fp = ts;
			  parse->parseio.parse_dtime.parse_state |= PARSEB_PPS|PARSEB_S_PPS;
			}
		    }
		  parse->ppsserial = ev.serial;
	       }
	    }
#endif
	  parse_process(parse, &parse->parseio.parse_dtime);
	  parse_iodone(&parse->parseio);
	}
    }
}

/*--------------------------------------------------
 * local poll
 */
static void
local_poll(parse)
     struct parseunit *parse;
{
  register int fd, i, rtc;
  fd_set fdmask;
  struct timeval timeout, starttime, curtime, selecttime;
#ifdef HAVE_GETCLOCK
        struct timespec ts;
#endif
  static struct timeval null_time = { 0, 0};
  timestamp_t st;

  FD_ZERO(&fdmask);
  fd = parse->generic->io.fd;
  FD_SET(fd, &fdmask);
  timeout.tv_sec  = 1;
  timeout.tv_usec = 500000;	/* 1.5 sec */

  if (parse->parse_type->cl_poll)
    {
      parse->parse_type->cl_poll(parse);
    }

#ifdef HAVE_GETCLOCK
  if (getclock(TIMEOFDAY, &ts) == -1)
    {
      msyslog(LOG_ERR, "getclock failed: %m");
      exit(1);
    }
  starttime.tv_sec = ts.tv_sec;
  starttime.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
  if (GETTIMEOFDAY(&starttime, 0L) == -1)
    {
      msyslog(LOG_ERR,"gettimeofday failed: %m");
      exit(1);
    }
#endif /* not HAVE_GETCLOCK */

  selecttime = timeout;

  do
    {
      while ((rtc = select(fd + 1, &fdmask, 0, 0, &selecttime)) != 1)
	{
	  /* no data from the radio clock */

	  if (rtc == -1)
	    {
	      if (errno == EINTR)
		{
#ifdef HAVE_GETCLOCK
		  if (getclock(TIMEOFDAY, &ts) == -1)
                    {
                      msyslog(LOG_ERR, "getclock failed: %m");
                      exit(1);
                    }
		  curtime.tv_sec = ts.tv_sec;
		  curtime.tv_usec = ts.tv_nsec / 1000;
#else /*  not HAVE_GETCLOCK */
		  if (GETTIMEOFDAY(&curtime, 0L) == -1)
		    {
		      msyslog(LOG_ERR,"gettimeofday failed: %m");
		      exit(1);
		    }
#endif /* not HAVE_GETCLOCK */
		  selecttime.tv_sec = curtime.tv_sec - starttime.tv_sec;
		  if (curtime.tv_usec < starttime.tv_usec)
		    {
		      selecttime.tv_sec  -= 1;
		      selecttime.tv_usec  = 1000000 + curtime.tv_usec - starttime.tv_usec;
		    }
		  else
		    {
		      selecttime.tv_usec = curtime.tv_usec - starttime.tv_usec;
		    }
	

		  if (!timercmp(&selecttime, &timeout, >))
		    {
		      /*
		       * calculate residual timeout value
		       */
		      selecttime.tv_sec = timeout.tv_sec - selecttime.tv_sec;

		      if (selecttime.tv_usec > timeout.tv_usec)
			{
			  selecttime.tv_sec -= 1;
			  selecttime.tv_usec = 1000000 + timeout.tv_usec - selecttime.tv_usec;
			}
		      else
			{
			  selecttime.tv_usec = timeout.tv_usec - selecttime.tv_usec;
			}
	
		      FD_SET(fd, &fdmask);
		      continue;
		    }
		}
	      else
		{
		  ERR(ERR_BADIO)
		    msyslog(LOG_WARNING, "PARSE receiver #%d: no data from device (select() error: %m)", CLK_UNIT(parse->peer));
		}
	    }
	  else
	    {
	      ERR(ERR_NODATA)
		msyslog(LOG_WARNING, "PARSE receiver #%d: no data from device (check receiver / cableling) ", CLK_UNIT(parse->peer));
	    }

	  parse->generic->noreply++;
	  parse->lastmissed = current_time;
	  parse_event(parse, CEVNT_TIMEOUT);

	  return;
	}

      /*
       * at least 1 character is available - gobble everthing up that is available
       */
      do
	{
	  char inbuf[256];

	  register char *s = inbuf;

	  rtc = i = read(fd, inbuf, sizeof(inbuf));

	  get_systime(&st.fp);

	  while (i-- > 0)
	    {
	      if (parse_ioread(&parse->parseio, (unsigned int)(*s++), &st))
		{
		  /*
		   * got something good to eat
		   */
		  parse_process(parse, &parse->parseio.parse_dtime);
		  parse_iodone(&parse->parseio);
		  /*
		   * done if no more characters are available
		   */
		  FD_SET(fd, &fdmask);
		  if ((i == 0) &&
		      (select(fd + 1, &fdmask, 0, 0, &null_time) == 0))
		    return;
		}
	    }
	  FD_SET(fd, &fdmask);
	} while ((rtc = select(fd + 1, &fdmask, 0, 0, &null_time)) == 1);
      FD_SET(fd, &fdmask);
    } while (1);
}

/*--------------------------------------------------
 * init_iobinding - find and initialize lower layers
 */
static bind_t *
init_iobinding(parse)
     struct parseunit *parse;
{
  register bind_t *b = io_bindings;

  while (b->bd_description != (char *)0)
    {
      if ((*b->bd_init)(parse))
	{
	  return b;
	}
      b++;
    }
  return (bind_t *)0;
}

/**===========================================================================
 ** support routines
 **/

/*--------------------------------------------------
 * convert a flag field to a string
 */
static char *
parsestate(state, buffer)
  u_long state;
  char *buffer;
{
  static struct bits
    {
      u_long bit;
      char         *name;
    } flagstrings[] =
    {
      { PARSEB_ANNOUNCE, "DST SWITCH WARNING" },
      { PARSEB_POWERUP,  "NOT SYNCHRONIZED" },
      { PARSEB_NOSYNC,   "TIME CODE NOT CONFIRMED" },
      { PARSEB_DST,      "DST" },
      { PARSEB_UTC,      "UTC DISPLAY" },
      { PARSEB_LEAPADD,  "LEAP ADD WARNING" },
      { PARSEB_LEAPDEL,  "LEAP DELETE WARNING" },
      { PARSEB_LEAPSECOND, "LEAP SECOND" },
      { PARSEB_ALTERNATE,"ALTERNATE ANTENNA" },
      { PARSEB_TIMECODE, "TIME CODE" },
      { PARSEB_PPS,      "PPS" },
      { PARSEB_POSITION, "POSITION" },
      { 0 }
    };

  static struct sbits
    {
      u_long bit;
      char         *name;
    } sflagstrings[] =
    {
      { PARSEB_S_LEAP,     "LEAP INDICATION" },
      { PARSEB_S_PPS,      "PPS SIGNAL" },
      { PARSEB_S_ANTENNA,  "ANTENNA" },
      { PARSEB_S_POSITION, "POSITION" },
      { 0 }
    };
  int i;

  *buffer = '\0';

  i = 0;
  while (flagstrings[i].bit)
    {
      if (flagstrings[i].bit & state)
	{
	  if (buffer[0])
	    strcat(buffer, "; ");
	  strcat(buffer, flagstrings[i].name);
	}
      i++;
    }

  if (state & (PARSEB_S_LEAP|PARSEB_S_ANTENNA|PARSEB_S_PPS|PARSEB_S_POSITION))
    {
      register char *s, *t;

      if (buffer[0])
	strcat(buffer, "; ");

      strcat(buffer, "(");

      t = s = buffer + strlen(buffer);

      i = 0;
      while (sflagstrings[i].bit)
	{
	  if (sflagstrings[i].bit & state)
	    {
	      if (t != s)
		{
		  strcpy(t, "; ");
		  t += 2;
		}
	
	      strcpy(t, sflagstrings[i].name);
	      t += strlen(t);
	    }
	  i++;
	}
      strcpy(t, ")");
    }
  return buffer;
}

/*--------------------------------------------------
 * convert a status flag field to a string
 */
static char *
parsestatus(state, buffer)
  u_long state;
  char *buffer;
{
  static struct bits
    {
      u_long bit;
      char         *name;
    } flagstrings[] =
    {
      { CVT_OK,      "CONVERSION SUCCESSFUL" },
      { CVT_NONE,    "NO CONVERSION" },
      { CVT_FAIL,    "CONVERSION FAILED" },
      { CVT_BADFMT,  "ILLEGAL FORMAT" },
      { CVT_BADDATE, "DATE ILLEGAL" },
      { CVT_BADTIME, "TIME ILLEGAL" },
      { 0 }
    };
  int i;

  *buffer = '\0';

  i = 0;
  while (flagstrings[i].bit)
    {
      if (flagstrings[i].bit & state)
	{
	  if (buffer[0])
	    strcat(buffer, "; ");
	  strcat(buffer, flagstrings[i].name);
	}
      i++;
    }

  return buffer;
}

/*--------------------------------------------------
 * convert a clock status flag field to a string
 */
static char *
clockstatus(state)
  u_long state;
{
  static char buffer[20];
  static struct status
    {
      u_long value;
      char         *name;
    } flagstrings[] =
    {
      { CEVNT_NOMINAL, "NOMINAL" },
      { CEVNT_TIMEOUT, "NO RESPONSE" },
      { CEVNT_BADREPLY,"BAD FORMAT" },
      { CEVNT_FAULT,   "FAULT" },
      { CEVNT_PROP,    "PROPAGATION DELAY" },
      { CEVNT_BADDATE, "ILLEGAL DATE" },
      { CEVNT_BADTIME, "ILLEGAL TIME" },
      { ~0 }
    };
  int i;

  i = 0;
  while (flagstrings[i].value != ~0)
    {
      if (flagstrings[i].value == state)
	{
	  return flagstrings[i].name;
	}
      i++;
    }

  sprintf(buffer, "unknown #%ld", (u_long)state);

  return buffer;
}

/*--------------------------------------------------
 * mkascii - make a printable ascii string
 * assumes (unless defined better) 7-bit ASCII
 */
#ifndef isprint
#define isprint(_X_) (((_X_) > 0x1F) && ((_X_) < 0x7F))
#endif

static char *
mkascii(buffer, blen, src, srclen)
  register char  *buffer;
  register long  blen;
  register char  *src;
  register long  srclen;
{
  register char *b    = buffer;
  register char *endb = (char *)0;

  if (blen < 4)
    return (char *)0;		/* don't bother with mini buffers */

  endb = buffer + blen - 4;

  blen--;			/* account for '\0' */

  while (blen && srclen--)
    {
      if ((*src != '\\') && isprint(*src))
	{			/* printables are easy... */
	  *buffer++ = *src++;
	  blen--;
	}
      else
	{
	  if (blen < 4)
	    {
	      while (blen--)
		{
		  *buffer++ = '.';
		}
	      *buffer = '\0';
	      return b;
	    }
	  else
	    {
	      if (*src == '\\')
		{
		  strcpy(buffer,"\\\\");
		  buffer += 2;
		  blen   -= 2;
		}
	      else
		{
		  sprintf(buffer, "\\x%02x", *src++);
		  blen   -= 4;
		  buffer += 4;
		}
	    }
	}
      if (srclen && !blen && endb) /* overflow - set last chars to ... */
	strcpy(endb, "...");
    }

  *buffer = '\0';
  return b;
}


/*--------------------------------------------------
 * l_mktime - make representation of a relative time
 */
static char *
l_mktime(delta)
  u_long delta;
{
  u_long tmp, m, s;
  static char buffer[40];

  buffer[0] = '\0';

  if ((tmp = delta / (60*60*24)) != 0)
    {
      sprintf(buffer, "%ldd+", (u_long)tmp);
      delta -= tmp * 60*60*24;
    }

  s = delta % 60;
  delta /= 60;
  m = delta % 60;
  delta /= 60;

  sprintf(buffer+strlen(buffer), "%02d:%02d:%02d",
	  (int)delta, (int)m, (int)s);

  return buffer;
}


/*--------------------------------------------------
 * parse_statistics - list summary of clock states
 */
static void
parse_statistics(parse)
  register struct parseunit *parse;
{
  register int i;

  NLOG(NLOG_CLOCKSTATIST) /* conditional if clause for conditional syslog */
    {
      msyslog(LOG_INFO, "PARSE receiver #%d: running time: %s",
	     CLK_UNIT(parse->peer),
	     l_mktime(current_time - parse->generic->timestarted));

      msyslog(LOG_INFO, "PARSE receiver #%d: current status: %s",
	     CLK_UNIT(parse->peer),
	     clockstatus(parse->generic->currentstatus));

      for (i = 0; i <= CEVNT_MAX; i++)
	{
	  register u_long stime;
	  register u_long percent, div = current_time - parse->generic->timestarted;

	  percent = stime = PARSE_STATETIME(parse, i);

	  while (((u_long)(~0) / 10000) < percent)
	    {
	      percent /= 10;
	      div     /= 10;
	    }

	  if (div)
	    percent = (percent * 10000) / div;
	  else
	    percent = 10000;

	  if (stime)
	    msyslog(LOG_INFO, "PARSE receiver #%d: state %18s: %13s (%3d.%02d%%)",
		   CLK_UNIT(parse->peer),
		   clockstatus(i),
		   l_mktime(stime),
		   percent / 100, percent % 100);
	}
    }
}

/*--------------------------------------------------
 * cparse_statistics - wrapper for statistics call
 */
static void
cparse_statistics(peer)
  register struct peer *peer;
{
  register struct parseunit *parse = (struct parseunit *)peer;

  parse_statistics(parse);
  parse->stattimer.event_time    = current_time + PARSESTATISTICS;
  TIMER_ENQUEUE(timerqueue, &parse->stattimer);
}

/**===========================================================================
 ** xntp interface routines
 **/

/*--------------------------------------------------
 * parse_init - initialize internal parse driver data
 */
static void
parse_init()
{
  memset((caddr_t)parseunits, 0, sizeof parseunits);
}


/*--------------------------------------------------
 * parse_shutdown - shut down a PARSE clock
 */
static void
parse_shutdown(unit, peer)
     int unit;
     struct peer *peer;
{
  register struct parseunit *parse;

  unit = CLK_UNIT(peer);
	
  if (unit >= MAXUNITS)
    {
      msyslog(LOG_ERR,
	     "PARSE receiver #%d: parse_shutdown: INTERNAL ERROR, unit invalid (max %d)",
	     unit,MAXUNITS);
      return;
    }

  parse = parseunits[unit];
	
  if (parse && !parse->peer)
    {
      msyslog(LOG_ERR,
	     "PARSE receiver #%d: parse_shutdown: INTERNAL ERROR, unit not in use", unit);
      return;
    }

  /*
   * print statistics a last time and
   * stop statistics machine
   */
  parse_statistics(parse);
  TIMER_DEQUEUE(&parse->stattimer);
	
#if ATOM
  {
    /*
     * kill possible PPS association
     */
    if (fdpps == parse->generic->io.fd)
      fdpps = -1;
  }
#endif

  if (parse->parse_type->cl_end)
    {
      parse->parse_type->cl_end(parse);
    }
	
  if (parse->binding)
    PARSE_END(parse);

  /*
   * Tell the I/O module to turn us off.  We're history.
   */
  if (!parse->pollonly)
    io_closeclock(&parse->generic->io);
  else
    (void) close(parse->generic->io.fd);

  NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
    msyslog(LOG_INFO, "PARSE receiver #%d: reference clock \"%s\" removed",
	 CLK_UNIT(parse->peer), parse->parse_type->cl_description);

  parse->peer = (struct peer *)0; /* unused now */
}

/*--------------------------------------------------
 * parse_start - open the PARSE devices and initialize data for processing
 */
static int
parse_start(sysunit, peer)
     int sysunit;
     struct peer *peer;
{
  u_int unit;
  int fd232, i;
#ifdef HAVE_TERMIOS
  struct termios tio;		/* NEEDED FOR A LONG TIME ! */
#endif
#ifdef HAVE_SYSV_TTYS
  struct termio tio;		/* NEEDED FOR A LONG TIME ! */
#endif
  struct parseunit * parse;
  char parsedev[sizeof(PARSEDEVICE)+20];
  parsectl_t tmp_ctl;
  u_int type;

  type = CLK_TYPE(peer);
  unit = CLK_UNIT(peer);

  if (unit >= MAXUNITS)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: unit number invalid (max %d)",
	     unit, MAXUNITS-1);
      return 0;
    }

  if ((type == ~0) || (parse_clockinfo[type].cl_description == (char *)0))
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: unsupported clock type %d (max %d)",
	     unit, CLK_REALTYPE(peer), ncltypes-1);
      return 0;
    }

  if (parseunits[unit] && parseunits[unit]->peer)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: unit in use", unit);
      return 0;
    }

  /*
   * Unit okay, attempt to open the device.
   */
  (void) sprintf(parsedev, PARSEDEVICE, unit);

#ifndef O_NOCTTY
#define O_NOCTTY 0
#endif

  fd232 = open(parsedev, O_RDWR | O_NOCTTY
#ifdef O_NONBLOCK
	       | O_NONBLOCK
#endif
	       , 0777);

  if (fd232 == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: open of %s failed: %m", unit, parsedev);
      return 0;
    }

  /*
   * Looks like this might succeed.  Find memory for the structure.
   * Look to see if there are any unused ones, if not we malloc()
   * one.
   */
  if (parseunits[unit])
    {
      parse = parseunits[unit];	/* The one we want is okay - and free */
    }
  else
    {
      for (i = 0; i < MAXUNITS; i++)
	{
	  if (parseunits[i] && !parseunits[i]->peer)
	    break;
	}
      if (i < MAXUNITS)
	{
	  /*
	   * Reclaim this one
	   */
	  parse = parseunits[i];
	  parseunits[i] = (struct parseunit *)0;
	}
      else
	{
	  parse = (struct parseunit *)
	    emalloc(sizeof(struct parseunit));
	}
    }

  memset((char *)parse, 0, sizeof(struct parseunit));
  parseunits[unit] = parse;

  parse->generic = peer->procptr;

  /*
   * Set up the structures
   */
  parse->generic->timestarted    = current_time;
  parse->lastchange     = current_time;
  /*
   * we want to filter input for the sake of
   * getting an impression on dispersion
   * also we like to average the median range
   */
  parse->generic->currentstatus	        = CEVNT_TIMEOUT; /* expect the worst */

  parse->flags          = 0;
  parse->pollneeddata   = 0;
  parse->pollonly       = 1;	/* go for default polling mode */
  parse->lastformat     = ~0;	/* assume no format known */
  parse->laststatus     = ~0;	/* be sure to mark initial status change */
  parse->lastmissed     = 0;	/* assume got everything */
  parse->ppsserial      = 0;
  parse->localdata      = (void *)0;

  clear_err(parse, ERR_ALL);
  
  parse->parse_type     = &parse_clockinfo[type];

  parse->generic->fudgetime1.l_ui = 0;	/* we can only pre-configure delays less than 1 second */
  parse->generic->fudgetime1.l_uf = parse->parse_type->cl_basedelay;

  parse->generic->fudgetime2.l_ui  = 0;	/* we can only pre-configure delays less than 1 second */
  parse->generic->fudgetime2.l_uf  = parse->parse_type->cl_ppsdelay;

  parse->generic->clockdesc = parse->parse_type->cl_description;

  peer->rootdelay       = parse->parse_type->cl_rootdelay;
  peer->sstclktype      = parse->parse_type->cl_type;
  peer->precision       = sys_precision;
  peer->stratum         = STRATUM_REFCLOCK;
  if (peer->stratum <= 1)
    memmove((char *)&parse->generic->refid, parse->parse_type->cl_id, 4);
  else
    parse->generic->refid = htonl(PARSEHSREFID);
	
  parse->generic->io.fd = fd232;
	
  parse->peer = peer;		/* marks it also as busy */

  parse->binding = init_iobinding(parse);

  if (parse->binding == (bind_t *)0)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: io sub system initialisation failed.");
      parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
      return 0;			/* well, ok - special initialisation broke */
    }      

  /*
   * configure terminal line
   */
  if (TTY_GETATTR(fd232, &tio) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: tcgetattr(%d, &tio): %m", unit, fd232);
      parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
      return 0;
    }
  else
    {
#ifndef _PC_VDISABLE
      memset((char *)tio.c_cc, 0, sizeof(tio.c_cc));
#else
      int disablec;
      errno = 0;		/* pathconf can deliver -1 without changing errno ! */

      disablec = fpathconf(parse->generic->io.fd, _PC_VDISABLE);
      if (disablec == -1 && errno)
	{
          msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: fpathconf(fd, _PC_VDISABLE): %m", CLK_UNIT(parse->peer));
          memset((char *)tio.c_cc, 0, sizeof(tio.c_cc)); /* best guess */
	}
      else
	if (disablec != -1)
	  memset((char *)tio.c_cc, disablec, sizeof(tio.c_cc));
#endif

#if defined (VMIN) || defined(VTIME)
      if ((parse_clockinfo[type].cl_lflag & ICANON) == 0)
	{
#ifdef VMIN
          tio.c_cc[VMIN]   = 1;
#endif
#ifdef VTIME
          tio.c_cc[VTIME]  = 0;
#endif
        }
#endif

      tio.c_cflag = parse_clockinfo[type].cl_cflag;
      tio.c_iflag = parse_clockinfo[type].cl_iflag;
      tio.c_oflag = parse_clockinfo[type].cl_oflag;
      tio.c_lflag = parse_clockinfo[type].cl_lflag;
	

#ifdef HAVE_TERMIOS
      if ((cfsetospeed(&tio, parse_clockinfo[type].cl_speed) == -1) ||
	  (cfsetispeed(&tio, parse_clockinfo[type].cl_speed) == -1))
	{
	  msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: tcset{i,o}speed(&tio, speed): %m", unit);
	  parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
	  return 0;
	}
#else
      tio.c_cflag     |= parse_clockinfo[type].cl_speed;
#endif

      if (TTY_SETATTR(fd232, &tio) == -1)
	{
	  msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: tcsetattr(%d, &tio): %m", unit, fd232);
	  parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
	  return 0;
	}
    }

  /*
   * as we always(?) get 8 bit chars we want to be
   * sure, that the upper bits are zero for less
   * than 8 bit I/O - so we pass that information on.
   * note that there can be only one bit count format
   * per file descriptor
   */

  switch (tio.c_cflag & CSIZE)
    {
    case CS5:
      tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS5;
      break;

    case CS6:
      tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS6;
      break;

    case CS7:
      tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS7;
      break;

    case CS8:
      tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS8;
      break;
    }

  if (!PARSE_SETCS(parse, &tmp_ctl))
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: parse_setcs() FAILED.", unit);
      parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
      return 0;			/* well, ok - special initialisation broke */
    }
  
  strcpy(tmp_ctl.parseformat.parse_buffer, parse->parse_type->cl_format);
  tmp_ctl.parseformat.parse_count = strlen(tmp_ctl.parseformat.parse_buffer);

  if (!PARSE_SETFMT(parse, &tmp_ctl))
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: parse_setfmt() FAILED.", unit);
      parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
      return 0;			/* well, ok - special initialisation broke */
    }
  
  /*
   * get rid of all IO accumulated so far
   */
#ifdef HAVE_TERMIOS
  (void) tcflush(parse->generic->io.fd, TCIOFLUSH);
#else
#ifdef TCFLSH
  {
#ifndef TCIOFLUSH
#define TCIOFLUSH 2
#endif
    int flshcmd = TCIOFLUSH;

    (void) ioctl(parse->generic->io.fd, TCFLSH, (caddr_t)&flshcmd);
  }
#endif
#endif
  
  /*
   * try to do any special initializations
   */
  if (parse->parse_type->cl_init)
    {
      if (parse->parse_type->cl_init(parse))
	{
	  parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
	  return 0;		/* well, ok - special initialisation broke */
	}
    }

  if (!(parse->parse_type->cl_flags & PARSE_F_POLLONLY) &&
      (CLK_PPS(parse->peer) || (parse->parse_type->cl_flags & PARSE_F_NOPOLLONLY)))
    {
      /*
       * Insert in async io device list.
       */
      parse->generic->io.clock_recv = parse->binding->bd_receive; /* pick correct receive routine */
      parse->generic->io.srcclock = (caddr_t)parse;
      parse->generic->io.datalen = 0;

      if (!io_addclock(&parse->generic->io))
	{
	  if (parse->parse_type->cl_flags & PARSE_F_NOPOLLONLY)
	    {
	      msyslog(LOG_ERR,
		     "PARSE receiver #%d: parse_start: addclock %s fails (ABORT - clock type requires async io)", CLK_UNIT(parse->peer), parsedev);
	      parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
	      return 0;
	    }
	  else
	    {
	      msyslog(LOG_ERR,
		     "PARSE receiver #%d: parse_start: addclock %s fails (switching to polling mode)", CLK_UNIT(parse->peer), parsedev);
	    }
	}
      else
	{
	  parse->pollonly = 0;	/*
				 * update at receipt of time_stamp - also
				 * supports PPS processing
				 */
	}
    }

#ifdef ATOM
  if (parse->pollonly && (parse->parse_type->cl_flags & PARSE_F_PPSPPS))
    {
      if (fdpps == -1)
	{
	  fdpps = parse->generic->io.fd;
	  if (!PARSE_DISABLE(parse))
	    {
	      msyslog(LOG_ERR, "PARSE receiver #%d: parse_start: parse_disable() FAILED", CLK_UNIT(parse->peer));
	      parse_shutdown(CLK_UNIT(parse->peer), peer); /* let our cleaning staff do the work */
	      return 0;
	    }
	}
      else
	{
	  msyslog(LOG_NOTICE, "PARSE receiver #%d: parse_start: loopfilter PPS already active - no PPS via CIOGETEV", CLK_UNIT(parse->peer));
	}
    }
#endif

  /*
   * wind up statistics timer
   */
  parse->stattimer.peer = (struct peer *)parse; /* we know better, but what the heck */
  parse->stattimer.event_handler = cparse_statistics;
  parse->stattimer.event_time    = current_time + PARSESTATISTICS;
  TIMER_ENQUEUE(timerqueue, &parse->stattimer);

  /*
   * get out Copyright information once
   */
  if (!notice)
    {
      NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
	msyslog(LOG_INFO, "NTP PARSE support: Copyright (c) 1989-1996, Frank Kardel");
      notice = 1;
    }

  /*
   * print out configuration
   */
  NLOG(NLOG_CLOCKINFO)
    {
      /* conditional if clause for conditional syslog */
      msyslog(LOG_INFO, "PARSE receiver #%d: reference clock \"%s\" (device %s) added",
	     CLK_UNIT(parse->peer),
	     parse->parse_type->cl_description, parsedev);

      msyslog(LOG_INFO, "PARSE receiver #%d:  Stratum %d, %sPPS support, trust time %s, precision %d",
	     CLK_UNIT(parse->peer),
	     parse->peer->stratum, (parse->pollonly || !CLK_PPS(parse->peer)) ? "no " : "",
	     l_mktime(parse->parse_type->cl_maxunsync), parse->peer->precision);

      msyslog(LOG_INFO, "PARSE receiver #%d:  rootdelay %s s, phaseadjust %s s, %s IO handling",
	     CLK_UNIT(parse->peer),
	     ufptoa(parse->parse_type->cl_rootdelay, 6),
	     lfptoa(&parse->generic->fudgetime1, 8),
	     parse->binding->bd_description);

      msyslog(LOG_INFO, "PARSE receiver #%d:  Format recognition: %s", CLK_UNIT(parse->peer),
	     !(*parse->parse_type->cl_format) ? "<AUTOMATIC>" : parse->parse_type->cl_format);
#ifdef ATOM
      msyslog(LOG_INFO, "PARSE receiver #%d: %sCD PPS support",
	     CLK_UNIT(parse->peer),
	     (fdpps == parse->generic->io.fd) ? "" : "NO ");
#endif
    }

  return 1;
}

/*--------------------------------------------------
 * parse_poll - called by the transmit procedure
 */
static void
parse_poll(unit, peer)
	int unit;
	struct peer *peer;
{
  register struct parseunit *parse;

  unit = CLK_UNIT(peer);

  if (unit >= MAXUNITS)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: poll: INTERNAL: unit invalid",
	     unit);
      return;
    }

  parse = parseunits[unit];

  if (!parse->peer)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: poll: INTERNAL: unit unused",
	     unit);
      return;
    }

  if (peer != parse->peer)
    {
      msyslog(LOG_ERR,
	     "PARSE receiver #%d: poll: INTERNAL: peer incorrect",
	     unit);
      return;
    }

  /*
   * Update clock stat counters
   */
  parse->generic->polls++;

  /*
   * in PPS mode we just mark that we want the next sample
   * for the clock filter
   */
  if (!parse->pollonly)
    {
      if (parse->pollneeddata)
	{
	  /*
	   * bad news - didn't get a response last time
	   */
	  parse->generic->noreply++;
	  parse->lastmissed = current_time;
	  parse_event(parse, CEVNT_TIMEOUT);

	  ERR(ERR_NODATA)
	    msyslog(LOG_WARNING, "PARSE receiver #%d: no data from device within poll interval (check receiver / cableling)", CLK_UNIT(parse->peer));
	}
      parse->pollneeddata = 1;
      if (parse->parse_type->cl_poll)
	{
	  parse->parse_type->cl_poll(parse);
	}
      return;
    }

  /*
   * the following code is only executed only when polling is used
   */

  PARSE_POLL(parse);
}

/*--------------------------------------------------
 * parse_control - set fudge factors, return statistics
 */
static void
parse_control(unit, in, out)
  int unit;
  struct refclockstat *in;
  struct refclockstat *out;
{
  register struct parseunit *parse;
  parsectl_t tmpctl;
  u_long type;
  static char outstatus[400];	/* status output buffer */

  if (out)
    {
      out->lencode       = 0;
      out->lastcode      = 0;
      out->kv_list       = (struct ctl_var *)0;
    }

  if (unit >= MAXUNITS)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_control: unit invalid (max %d)",
	     unit, MAXUNITS-1);
      return;
    }


  /*
   * XXX - UGLY - unit needs to be eliminated in favor of a peer *
   */

  parse = parseunits[unit];

  if (!parse || !parse->peer)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: parse_control: unit invalid (UNIT INACTIVE)",
	     unit);
      return;
    }

  type = CLK_TYPE(parse->peer);
  unit = CLK_UNIT(parse->peer);

  if (in)
    {
      if (in->haveflags & (CLK_HAVEFLAG1|CLK_HAVEFLAG2|CLK_HAVEFLAG3|CLK_HAVEFLAG4))
	{
	  parse->flags = in->flags & (CLK_FLAG1|CLK_FLAG2|CLK_FLAG3|CLK_FLAG4);
	}
    }

  if (out)
    {
      register u_long sum = 0;
      register char *t, *tt;
      register struct tm *tm;
      register short utcoff;
      register char sign;
      register int i;
      time_t tim;

      outstatus[0] = '\0';

      out->type       = REFCLK_PARSE;
      out->haveflags |= CLK_HAVETIME2;

      /*
       * figure out skew between PPS and RS232 - just for informational
       * purposes - returned in time2 value
       */
       if (PARSE_SYNC(parse->time.parse_state))
	 {
	   if (PARSE_PPS(parse->time.parse_state) && PARSE_TIMECODE(parse->time.parse_state))
	     {
	       l_fp off;

	       /*
		* we have a PPS and RS232 signal - calculate the skew
		* WARNING: assumes on TIMECODE == PULSE (timecode after pulse)
		*/
	       off = parse->time.parse_stime.fp;
	       L_SUB(&off, &parse->time.parse_ptime.fp); /* true offset */
	       tt = add_var(&out->kv_list, 80, RO);
	       sprintf(tt, "refclock_ppsskew=%s", lfptoms(&off, 6));
	     }
	 }

      if (PARSE_PPS(parse->time.parse_state))
	{
	  tt = add_var(&out->kv_list, 80, RO|DEF);
	  sprintf(tt, "refclock_ppstime=\"%s\"", prettydate(&parse->time.parse_ptime.fp));
	}

      /*
       * all this for just finding out the +-xxxx part (there are always
       * new and changing fields in the standards 8-().
       *
       * but we do it for the human user...
       */
      tim  = parse->time.parse_time.fp.l_ui - JAN_1970;
      tm = gmtime(&tim);
      utcoff = tm->tm_hour * 60 + tm->tm_min;
      tm = localtime(&tim);
      utcoff = tm->tm_hour * 60 + tm->tm_min - utcoff + 12 * 60;
      utcoff += 24 * 60;
      utcoff %= 24 * 60;
      utcoff -= 12 * 60;
      if (utcoff < 0)
	{
	  utcoff = -utcoff;
	  sign = '-';
	}
      else
	{
	  sign = '+';
	}

      tt = add_var(&out->kv_list, 128, RO|DEF);
      sprintf(tt, "refclock_time=\"");
      tt += strlen(tt);

      if (parse->time.parse_time.fp.l_ui == 0)
	{
	  strcpy(tt, "<UNDEFINED>\"");
	}
      else
	{
	  strcpy(tt, prettydate(&parse->time.parse_time.fp));
	  t = tt + strlen(tt);
	  
	  sprintf(t, " (%c%02d%02d)\"", sign, utcoff / 60, utcoff % 60);
	}

      if (!PARSE_GETTIMECODE(parse, &tmpctl))
	{
	  ERR(ERR_INTERNAL)
	    msyslog(LOG_ERR, "PARSE receiver #%d: parse_control: parse_timecode() FAILED", unit);
	}
      else
	{
	  tt = add_var(&out->kv_list, 512, RO|DEF);
	  sprintf(tt, "refclock_status=\"");
	  tt += strlen(tt);

	  /*
	   * copy PPS flags from last read transaction (informational only)
	   */
	  tmpctl.parsegettc.parse_state |= parse->time.parse_state &
					   (PARSEB_PPS|PARSEB_S_PPS);

	  (void) parsestate(tmpctl.parsegettc.parse_state, tt);

	  strcat(tt, "\"");

	  if (tmpctl.parsegettc.parse_count)
	    mkascii(outstatus+strlen(outstatus), sizeof(outstatus)- strlen(outstatus) - 1,
		    tmpctl.parsegettc.parse_buffer, tmpctl.parsegettc.parse_count - 1);

	  parse->generic->badformat += tmpctl.parsegettc.parse_badformat;
	}
	
      tmpctl.parseformat.parse_format = tmpctl.parsegettc.parse_format;
	
      if (!PARSE_GETFMT(parse, &tmpctl))
	{
	  ERR(ERR_INTERNAL)
	    msyslog(LOG_ERR, "PARSE receiver #%d: parse_control: parse_getfmt() FAILED", unit);
	}
      else
	{
	  tt = add_var(&out->kv_list, 80, RO|DEF);
	  sprintf(tt, "refclock_format=\"");

	  strncat(tt, tmpctl.parseformat.parse_buffer, tmpctl.parseformat.parse_count);
	  strcat(tt,"\"");
	}

      /*
       * gather state statistics
       */

      tt = add_var(&out->kv_list, 200, RO|DEF);
      strcpy(tt, "refclock_states=\"");
      tt += strlen(tt);

      for (i = 0; i <= CEVNT_MAX; i++)
	{
	  register u_long stime;
	  register u_long div = current_time - parse->generic->timestarted;
	  register u_long percent;
	
	  percent = stime = PARSE_STATETIME(parse, i);

	  while (((u_long)(~0) / 10000) < percent)
	    {
	      percent /= 10;
	      div     /= 10;
	    }
	
	  if (div)
	    percent = (percent * 10000) / div;
	  else
	    percent = 10000;

	  if (stime)
	    {
	      sprintf(tt, "%s%s%s: %s (%d.%02d%%)",
		      sum ? "; " : "",
                      (parse->generic->currentstatus == i) ? "*" : "",
		      clockstatus(i),
		      l_mktime(stime),
		      (int)(percent / 100), (int)(percent % 100));
	      sum += stime;
	      tt  += strlen(tt);
	    }
	}

      sprintf(tt, "; running time: %s\"", l_mktime(sum));

      tt = add_var(&out->kv_list, 32, RO);
      sprintf(tt, "refclock_id=\"%s\"", parse->parse_type->cl_id);

      tt = add_var(&out->kv_list, 80, RO);
      sprintf(tt, "refclock_iomode=\"%s\"", parse->binding->bd_description);

      tt = add_var(&out->kv_list, 128, RO);
      sprintf(tt, "refclock_driver_version=\"refclock_parse.c,v 3.98 1997/01/19 14:11:59 kardel Exp\"");

      out->lencode       = strlen(outstatus);
      out->lastcode      = outstatus;
    }
}

/**===========================================================================
 ** processing routines
 **/

/*--------------------------------------------------
 * event handling - note that nominal events will also be posted
 */
static void
parse_event(parse, event)
  struct parseunit *parse;
  int event;
{
  if (parse->generic->currentstatus != (u_char) event)
    {
      parse->statetime[parse->generic->currentstatus] += current_time - parse->lastchange;
      parse->lastchange              = current_time;

      parse->generic->currentstatus    = (u_char)event;

      if (parse->parse_type->cl_event)
	parse->parse_type->cl_event(parse, event);
      
      if (event != CEVNT_NOMINAL)
	{
	  parse->lastevent = parse->generic->currentstatus;
	}
      else
	{
	  NLOG(NLOG_CLOCKSTATUS)
	    msyslog(LOG_INFO, "PARSE receiver #%d: SYNCHRONIZED",
		   CLK_UNIT(parse->peer));
	}

      if (event == CEVNT_FAULT)
	{
	  NLOG(NLOG_CLOCKEVENT) /* conditional if clause for conditional syslog */
	    ERR(ERR_BADEVENT)
	      msyslog(LOG_ERR,
		     "clock %s fault '%s' (0x%02x)", refnumtoa(parse->peer->srcadr.sin_addr.s_addr), ceventstr(event),
		     (u_int)event);
	}
      else
	{
	  NLOG(NLOG_CLOCKEVENT) /* conditional if clause for conditional syslog */
	    if (event == CEVNT_NOMINAL || list_err(parse, ERR_BADEVENT))
	      msyslog(LOG_INFO,
		     "clock %s event '%s' (0x%02x)", refnumtoa(parse->peer->srcadr.sin_addr.s_addr), ceventstr(event),
		     (u_int)event);
	}

      report_event(EVNT_PEERCLOCK, parse->peer);
      report_event(EVNT_CLOCKEXCPT, parse->peer);
    }
}

/*--------------------------------------------------
 * process a PARSE time sample
 */
static void
parse_process(parse, parsetime)
  struct parseunit *parse;
  parsetime_t      *parsetime;
{
  unsigned char leap;
  l_fp off, rectime, reftime;

  /*
   * check for changes in conversion status
   * (only one for each new status !)
   */
  if (parse->laststatus != parsetime->parse_status)
    {
      char buffer[200];

      NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
	msyslog(LOG_WARNING, "PARSE receiver #%d: conversion status \"%s\"",
	     CLK_UNIT(parse->peer), parsestatus(parsetime->parse_status, buffer));

      if ((parsetime->parse_status & CVT_MASK) == CVT_FAIL)
	{
	  /*
	   * tell more about the story - list time code
	   * there is a slight change for a race condition and
	   * the time code might be overwritten by the next packet
	   */
	  parsectl_t tmpctl;

	  if (!PARSE_GETTIMECODE(parse, &tmpctl))
	    {
	      ERR(ERR_INTERNAL)
		msyslog(LOG_ERR, "PARSE receiver #%d: parse_process: parse_timecode() FAILED", CLK_UNIT(parse->peer));
	    }
	  else
	    {
	      ERR(ERR_BADDATA)
		msyslog(LOG_WARNING, "PARSE receiver #%d: FAILED TIMECODE: \"%s\" (check receiver configuration / cableling)",
		     CLK_UNIT(parse->peer), mkascii(buffer, sizeof buffer, tmpctl.parsegettc.parse_buffer, tmpctl.parsegettc.parse_count - 1));
	      parse->generic->badformat += tmpctl.parsegettc.parse_badformat;
	    }
	}

      parse->laststatus = parsetime->parse_status;
    }

  /*
   * examine status and post appropriate events
   */
  if ((parsetime->parse_status & CVT_MASK) != CVT_OK)
    {
      /*
       * got bad data - tell the rest of the system
       */
      switch (parsetime->parse_status & CVT_MASK)
	{
	case CVT_NONE:
	  break;		/* well, still waiting - timeout is handled at higher levels */

	case CVT_FAIL:
	  parse->generic->badformat++;
	  if (parsetime->parse_status & CVT_BADFMT)
	    {
	      parse_event(parse, CEVNT_BADREPLY);
	    }
	  else
	    if (parsetime->parse_status & CVT_BADDATE)
	      {
		parse_event(parse, CEVNT_BADDATE);
	      }
	    else
	      if (parsetime->parse_status & CVT_BADTIME)
		{
		  parse_event(parse, CEVNT_BADTIME);
		}
	      else
		{
		  parse_event(parse, CEVNT_BADREPLY); /* for the lack of something better */
		}
	}
      return;			/* skip the rest - useless */
    }

  /*
   * check for format changes
   * (in case somebody has swapped clocks 8-)
   */
  if (parse->lastformat != parsetime->parse_format)
    {
      parsectl_t tmpctl;
	
      tmpctl.parseformat.parse_format = parsetime->parse_format;

      if (!PARSE_GETFMT(parse, &tmpctl))
	{
	  ERR(ERR_INTERNAL)
	    msyslog(LOG_ERR, "PARSE receiver #%d: parse_getfmt() FAILED", CLK_UNIT(parse->peer));
	}
      else
	{
	  NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
	    msyslog(LOG_INFO, "PARSE receiver #%d: new packet format \"%s\"",
		 CLK_UNIT(parse->peer), tmpctl.parseformat.parse_buffer);
	}
      parse->lastformat = parsetime->parse_format;
    }

  /*
   * now, any changes ?
   */
  if (parse->time.parse_state != parsetime->parse_state)
    {
      char tmp1[200];
      char tmp2[200];
      /*
       * something happend
       */
	
      (void) parsestate(parsetime->parse_state, tmp1);
      (void) parsestate(parse->time.parse_state, tmp2);
	
      NLOG(NLOG_CLOCKINFO) /* conditional if clause for conditional syslog */
	msyslog(LOG_INFO,"PARSE receiver #%d: STATE CHANGE: %s -> %s",
	     CLK_UNIT(parse->peer), tmp2, tmp1);
    }

  /*
   * remember for future
   */
  parse->time = *parsetime;

  /*
   * check to see, whether the clock did a complete powerup or lost PZF signal
   * and post correct events for current condition
   */
  if (PARSE_POWERUP(parsetime->parse_state))
    {
      /*
       * this is bad, as we have completely lost synchronisation
       * well this is a problem with the receiver here
       * for PARSE Meinberg DCF77 receivers the lost synchronisation
       * is true as it is the powerup state and the time is taken
       * from a crude real time clock chip
       * for the PZF series this is only partly true, as
       * PARSE_POWERUP only means that the pseudo random
       * phase shift sequence cannot be found. this is only
       * bad, if we have never seen the clock in the SYNC
       * state, where the PHASE and EPOCH are correct.
       * for reporting events the above business does not
       * really matter, but we can use the time code
       * even in the POWERUP state after having seen
       * the clock in the synchronized state (PZF class
       * receivers) unless we have had a telegram disruption
       * after having seen the clock in the SYNC state. we
       * thus require having seen the clock in SYNC state
       * *after* having missed telegrams (noresponse) from
       * the clock. one problem remains: we might use erroneously
       * POWERUP data if the disruption is shorter than 1 polling
       * interval. fortunately powerdowns last usually longer than 64
       * seconds and the receiver is at least 2 minutes in the
       * POWERUP or NOSYNC state before switching to SYNC
       */
      parse_event(parse, CEVNT_FAULT);
      NLOG(NLOG_CLOCKSTATUS)
	ERR(ERR_BADSTATUS)
	  msyslog(LOG_ERR,"PARSE receiver #%d: NOT SYNCHRONIZED",
		         CLK_UNIT(parse->peer));
    }
  else
    {
      /*
       * we have two states left
       *
       * SYNC:
       *  this state means that the EPOCH (timecode) and PHASE
       *  information has be read correctly (at least two
       *  successive PARSE timecodes were received correctly)
       *  this is the best possible state - full trust
       *
       * NOSYNC:
       *  The clock should be on phase with respect to the second
       *  signal, but the timecode has not been received correctly within
       *  at least the last two minutes. this is a sort of half baked state
       *  for PARSE Meinberg DCF77 clocks this is bad news (clock running
       *  without timecode confirmation)
       *  PZF 535 has also no time confirmation, but the phase should be
       *  very precise as the PZF signal can be decoded
       */

      if (PARSE_SYNC(parsetime->parse_state))
	{
	  /*
	   * currently completely synchronized - best possible state
	   */
	  parse->lastsync = current_time;
	  clear_err(parse, ERR_BADSTATUS);
	}
      else
	{
	  /*
	   * we have had some problems receiving the time code
	   */
	  parse_event(parse, CEVNT_PROP);
	  NLOG(NLOG_CLOCKSTATUS)
	    ERR(ERR_BADSTATUS)
	      msyslog(LOG_ERR,"PARSE receiver #%d: TIMECODE NOT CONFIRMED",
			     CLK_UNIT(parse->peer));
	}
    }

  if (PARSE_TIMECODE(parsetime->parse_state))
    {
      l_fp offset;

      /*
       * calculate time offset including systematic delays
       * off = PARSE-timestamp + propagation delay - kernel time stamp
       */
      offset = parse->generic->fudgetime1;
    
      off = parsetime->parse_time.fp;

      reftime = off;

      L_ADD(&off, &offset);
      rectime = off;		/* this makes org time and xmt time somewhat artificial */
    
      L_SUB(&off, &parsetime->parse_stime.fp);
    }

  if (PARSE_PPS(parsetime->parse_state) && CLK_PPS(parse->peer))
    {
      l_fp offset;

      /*
       * we have a PPS signal - much better than the RS232 stuff (we hope)
       */
      offset = parsetime->parse_ptime.fp;

      L_ADD(&offset, &parse->generic->fudgetime2);

      if (PARSE_TIMECODE(parsetime->parse_state))
	{
	  if (M_ISGEQ(off.l_i, off.l_f, -1, 0x80000000) &&
	      M_ISGEQ(0, 0x7fffffff, off.l_i, off.l_f))
	    {
	      /*
	       * RS232 offsets within [-0.5..0.5[ - take PPS offsets
	       */

	      if (parse->parse_type->cl_flags & PARSE_F_PPSONSECOND)
		{
		  reftime = off = offset;
		  rectime = offset;
		  /*
		   * implied on second offset
		   */
		  off.l_uf = ~off.l_uf; /* map [0.5..1[ -> [-0.5..0[ */
		  off.l_ui = (off.l_f < 0) ? ~0 : 0; /* sign extend */
		}
	      else
		{
		  /*
		   * time code describes pulse
		   */
		  off = parsetime->parse_time.fp;

		  rectime = reftime = off; /* take reference time - fake rectime */

		  L_SUB(&off, &offset); /* true offset */
		}
	    }
	  /*
	   * take RS232 offset when PPS when out of bounds
	   */
	}
      else
	{
	  /*
	   * Well, no time code to guide us - assume on second pulse
	   * and pray, that we are within [-0.5..0.5[
	   */
	  reftime = off = offset;
	  rectime = offset;
	  /*
	   * implied on second offset
	   */
	  off.l_uf = ~off.l_uf; /* map [0.5..1[ -> [-0.5..0[ */
	  off.l_ui = (off.l_f < 0) ? ~0 : 0; /* sign extend */
	}
    }
  else
    {
      if (!PARSE_TIMECODE(parsetime->parse_state))
	{
	  /*
	   * Well, no PPS, no TIMECODE, no more work ...
	   */
	  return;
	}
    }

    parse->generic->lasttime = current_time;

    if (!refclock_sample(&off, parse->generic, parse->parse_type->cl_samples, parse->parse_type->cl_keep))
      {
	parse_event(parse, CEVNT_BADTIME);
	return;
      }

#if defined(ATOM)
  if (CLK_PPS(parse->peer) && !parse->pollonly && PARSE_SYNC(parsetime->parse_state))
    {
      /*
       * only provide PPS information when clock
       * is in sync
       * thus PHASE and EPOCH are correct and PPS is not
       * done via the CIOGETEV loopfilter mechanism
       */
      if (fdpps != parse->generic->io.fd)
	(void) pps_sample(&parse->generic->offset);
    }
#endif /* ATOM */

  /*
   * ready, unless the machine wants a sample
   */
  if (!parse->pollonly && !parse->pollneeddata)
    return;

  if (PARSE_SYNC(parsetime->parse_state))
    {
      /*
       * log OK status
       */
       parse_event(parse, CEVNT_NOMINAL);
    }

  parse->pollneeddata = 0;

  clear_err(parse, ERR_BADIO);
  clear_err(parse, ERR_BADDATA);
  clear_err(parse, ERR_NODATA);
  clear_err(parse, ERR_INTERNAL);
  
  parse->generic->lastrec = rectime;
  parse->generic->lastref = reftime;
  
  /*
   * and now stick it into the clock machine
   * samples are only valid iff lastsync is not too old and
   * we have seen the clock in sync at least once
   * after the last time we didn't see an expected data telegram
   * see the clock states section above for more reasoning
   */
  if (((current_time - parse->lastsync) > parse->parse_type->cl_maxunsync) ||
      (parse->lastsync <= parse->lastmissed))
    {
      leap = LEAP_NOTINSYNC;
    }
  else
    {
      if (PARSE_LEAPADD(parsetime->parse_state))
	{
	  /*
	   * we pick this state also for time code that pass leap warnings
	   * without direction information (as earth is currently slowing
	   * down).
	   */
	  leap = (parse->flags & PARSE_LEAP_DELETE) ? LEAP_DELSECOND : LEAP_ADDSECOND;
	}
      else
        if (PARSE_LEAPDEL(parsetime->parse_state))
	  {
	    leap = LEAP_DELSECOND;
	  }
	else
	  {
	    leap = LEAP_NOWARNING;
	  }
    }
  
  refclock_receive(parse->peer, &parse->generic->offset, 0,
		   parse->generic->dispersion, &reftime, &rectime,
		   leap | REFCLOCK_OWN_STATES);
}

/**===========================================================================
 ** clock polling support
 **/

struct poll_timer
{
  struct event timer;		/* we'd like to poll a a higher rate than 1/64s */
};

typedef struct poll_timer poll_timer_t;

/*--------------------------------------------------
 * direct poll routine
 */
static void
poll_dpoll(parse)
  struct parseunit *parse;
{
  register int rtc;
  register char *ps = ((poll_info_t *)parse->parse_type->cl_data)->string;
  register int   ct = ((poll_info_t *)parse->parse_type->cl_data)->count;

  rtc = write(parse->generic->io.fd, ps, ct);
  if (rtc < 0)
    {
      ERR(ERR_BADIO)
	msyslog(LOG_ERR, "PARSE receiver #%d: poll_dpoll: failed to send cmd to clock: %m", CLK_UNIT(parse->peer));
    }
  else
    if (rtc != ct)
      {
	ERR(ERR_BADIO)
	  msyslog(LOG_ERR, "PARSE receiver #%d: poll_dpoll: failed to send cmd incomplete (%d of %d bytes sent)", CLK_UNIT(parse->peer), rtc, ct);
      }
  clear_err(parse, ERR_BADIO);
}

/*--------------------------------------------------
 * periodic poll routine
 */
static void
poll_poll(parse)
  struct parseunit *parse;
{
  register poll_timer_t *pt = (poll_timer_t *)parse->localdata;

  poll_dpoll(parse);

  if (pt != (poll_timer_t *)0)
    {
      pt->timer.event_time = current_time + ((poll_info_t *)parse->parse_type->cl_data)->rate;
      TIMER_ENQUEUE(timerqueue, &pt->timer);
    }
}

/*--------------------------------------------------
 * init routine - setup timer
 */
static int
poll_init(parse)
  struct parseunit *parse;
{
  register poll_timer_t *pt;

  if (((poll_info_t *)parse->parse_type->cl_data)->rate)
    {
      parse->localdata = (void *)emalloc(sizeof(poll_timer_t));
      memset((char *)parse->localdata, 0, sizeof(poll_timer_t));
  
      pt = (poll_timer_t *)parse->localdata;
      
      pt->timer.peer          = (struct peer *)parse; /* well, only we know what it is */
      pt->timer.event_handler = (void (*) P((struct peer *))) poll_poll;
      poll_poll(parse);
    }
  else
    {
      parse->localdata = (void *)0;
    }

  return 0;
}

/*--------------------------------------------------
 * end routine - clean up timer
 */
static void
poll_end(parse)
  struct parseunit *parse;
{
  if (parse->localdata != (void *)0)
    {
      TIMER_DEQUEUE(&((poll_timer_t *)parse->localdata)->timer);
      free((char *)parse->localdata);
      parse->localdata = (void *)0;
    }
}

/**===========================================================================
 ** special code for special clocks
 **/


/*--------------------------------------------------
 * trimble TAIP init routine - setup EOL and then do poll_init.
 */
static int
trimbletaip_init(parse)
  struct parseunit *parse;
{
#ifdef HAVE_TERMIOS
  struct termios tio;
#endif
#ifdef HAVE_SYSV_TTYS
  struct termio tio;
#endif
  /*
   * configure terminal line for trimble receiver
   */
  if (TTY_GETATTR(parse->generic->io.fd, &tio) == -1)
    {
      msyslog(LOG_ERR, "PARSE receiver #%d: trimbletaip_init: tcgetattr(fd, &tio): %m", CLK_UNIT(parse->peer));
      return 0;
    }
  else
    {
      tio.c_cc[VEOL] = TRIMBLETAIP_EOL;
	
      if (TTY_SETATTR(parse->generic->io.fd, &tio) == -1)
	{
	  msyslog(LOG_ERR, "PARSE receiver #%d: trimbletaip_init: tcsetattr(fd, &tio): %m", CLK_UNIT(parse->peer));
	  return 0;
	}
    }
  return poll_init(parse);
}

/*--------------------------------------------------
 * trimble TAIP event routine - reset receiver upon data format trouble
 */
static char *taipinit[] = {
  ">FPV00000000<",
  ">SRM;ID_FLAG=F;CS_FLAG=T;EC_FLAG=F;FR_FLAG=T;CR_FLAG=F<",
  ">FTM00020001<",
  (char *)0
};
      
static void
trimbletaip_event(parse, event)
  struct parseunit *parse;
  int event;
{
  switch (event)
    {
    case CEVNT_BADREPLY:	/* reset on garbled input */
    case CEVNT_TIMEOUT:		/* reset on no input */
      {
	register char **iv;

	iv = taipinit;
	while (*iv)
	  {
	    register int rtc = write(parse->generic->io.fd, *iv, strlen(*iv));
	    if (rtc < 0)
	      {
		msyslog(LOG_ERR, "PARSE receiver #%d: trimbletaip_event: failed to send cmd to clock: %m", CLK_UNIT(parse->peer));
		return;
	      }
	    else
	      {
		if (rtc != strlen(*iv))
		  {
		    msyslog(LOG_ERR, "PARSE receiver #%d: trimbletaip_event: failed to send cmd incomplete (%d of %d bytes sent)",
			   CLK_UNIT(parse->peer), rtc, strlen(*iv));
		    return;
		  }
	      }
	    iv++;
	  }

	NLOG(NLOG_CLOCKINFO)
	  ERR(ERR_BADIO)
	    msyslog(LOG_ERR, "PARSE receiver #%d: trimbletaip_event: RECEIVER INITIALIZED",
		   CLK_UNIT(parse->peer));
      }
      break;

    default:			/* ignore */
      break;
    }
}

/*
 * This driver supports the Trimble SVee Six Plus GPS receiver module.
 * It should support other Trimble receivers which use the Trimble Standard
 * Interface Protocol (see below).
 *
 * The module has a serial I/O port for command/data and a 1 pulse-per-second
 * output, about 1 microsecond wide. The leading edge of the pulse is
 * coincident with the change of the GPS second. This is the same as
 * the change of the UTC second +/- ~1 microsecond. Some other clocks
 * specifically use a feature in the data message as a timing reference, but
 * the SVee Six Plus does not do this. In fact there is considerable jitter
 * on the timing of the messages, so this driver only supports the use
 * of the PPS pulse for accurate timing. Where it is determined that
 * the offset is way off, when first starting up xntpd for example,
 * the timing of the data stream is used until the offset becomes low enough
 * (|offset| < CLOCK_MAX), at which point the pps offset is used.
 *
 * It can use either option for receiving PPS information - the 'ppsclock'
 * stream pushed onto the serial data interface to timestamp the Carrier
 * Detect interrupts, where the 1PPS connects to the CD line. This only
 * works on SunOS 4.1.x currently. To select this, define PPSPPS in
 * Config.local. The other option is to use a pulse-stretcher/level-converter
 * to convert the PPS pulse into a RS232 start pulse & feed this into another
 * tty port. To use this option, define PPSCLK in Config.local. The pps input,
 * by whichever method, is handled in ntp_loopfilter.c
 *
 * The receiver uses a serial message protocol called Trimble Standard
 * Interface Protocol (it can support others but this driver only supports
 * TSIP). Messages in this protocol have the following form:
 *
 * <DLE><id> ... <data> ... <DLE><ETX>
 *
 * Any bytes within the <data> portion of value 10 hex (<DLE>) are doubled
 * on transmission and compressed back to one on reception. Otherwise
 * the values of data bytes can be anything. The serial interface is RS-422
 * asynchronous using 9600 baud, 8 data bits with odd party (**note** 9 bits
 * in total!), and 1 stop bit. The protocol supports byte, integer, single,
 * and double datatypes. Integers are two bytes, sent most significant first.
 * Singles are IEEE754 single precision floating point numbers (4 byte) sent
 * sign & exponent first. Doubles are IEEE754 double precision floating point
 * numbers (8 byte) sent sign & exponent first.
 * The receiver supports a large set of messages, only a small subset of
 * which are used here. From driver to receiver the following are used:
 *
 *  ID    Description
 *
 *  21    Request current time
 *  22    Mode Select
 *  2C    Set/Request operating parameters
 *  2F    Request UTC info
 *  35    Set/Request I/O options

 * From receiver to driver the following are recognised:
 *
 *  ID    Description
 *
 *  41    GPS Time
 *  44    Satellite selection, PDOP, mode
 *  46    Receiver health
 *  4B    Machine code/status
 *  4C    Report operating parameters (debug only)
 *  4F    UTC correction data (used to get leap second warnings)
 *  55    I/O options (debug only)
 *
 * All others are accepted but ignored.
 *
 */

#define PI		3.1415926535898	/* lots of sig figs */
#define D2R		PI/180.0

/*-------------------------------------------------------------------
 * sendcmd, sendbyte, sendetx, sendflt, sendint implement the command
 * interface to the receiver.
 *
 * CAVEAT: the sendflt, sendint routines are byte order dependend and
 * float implementation dependend - these must be converted to portable
 * versions !
 */

union {
    u_char  bd[8];
    int     iv;
    float   fv;
    double  dv;
}  uval;
  
struct txbuf
{
  short idx;			/* index to first unused byte */
  u_char *txt;			/* pointer to actual data buffer */
};

void
sendcmd(buf, c)
  struct txbuf *buf;
  u_char c;
{
  buf->txt[0] = DLE;
  buf->txt[1] = c;
  buf->idx = 2;
}

void sendbyte(buf, b)
  struct txbuf *buf;
  u_char b;
{
  if (b == DLE)
    buf->txt[buf->idx++] = DLE;
  buf->txt[buf->idx++] = b;
}

void
sendetx(buf, parse)
  struct txbuf *buf;
  struct parseunit *parse;
{
  buf->txt[buf->idx++] = DLE;
  buf->txt[buf->idx++] = ETX;

  if (write(parse->generic->io.fd, buf->txt, buf->idx) != buf->idx)
    {
      ERR(ERR_BADIO)
	msyslog(LOG_ERR, "PARSE receiver #%d: sendetx: failed to send cmd to clock: %m", CLK_UNIT(parse->peer));
    }
  else
    {
      clear_err(parse, ERR_BADIO);
    }
}

void  
sendint(buf, a)
  struct txbuf *buf;
  int a;
{
  /* send 16bit int, msbyte first */
  sendbyte(buf, (a>>8) & 0xff);
  sendbyte(buf, a & 0xff);
}

void
sendflt(buf, a)
  struct txbuf *buf;
  float a;
{
  int i;

  uval.fv = a;
#ifdef XNTP_BIG_ENDIAN
  for (i=0; i<=3; i++)
#else
  for (i=3; i>=0; i--)
#endif
    sendbyte(buf, uval.bd[i]);
}

/*--------------------------------------------------
 * trimble TSIP init routine
 */
static int
trimbletsip_init(parse)
  struct parseunit *parse;
{
  u_char buffer[256];
  struct txbuf buf;

  buf.txt = buffer;
  
  if (!poll_init(parse))
    {
      sendcmd(&buf, 0x1f);	/* request software versions */
      sendetx(&buf, parse);

      sendcmd(&buf, 0x2c);	/* set operating parameters */
      sendbyte(&buf, 4);	/* static */
      sendflt(&buf, 5.0*D2R);	/* elevation angle mask = 10 deg XXX */
      sendflt(&buf, 4.0);	/* s/n ratio mask = 6 XXX */
      sendflt(&buf, 12.0);	/* PDOP mask = 12 */
      sendflt(&buf, 8.0);	/* PDOP switch level = 8 */
      sendetx(&buf, parse);

      sendcmd(&buf, 0x22);	/* fix mode select */
      sendbyte(&buf, 0);	/* automatic */
      sendetx(&buf, parse);

      sendcmd(&buf, 0x28);	/* request system message */
      sendetx(&buf, parse);

      sendcmd(&buf, 0x8e);	/* superpacket fix */
      sendbyte(&buf, 0x2);	/* binary mode */
      sendetx(&buf, parse);

      sendcmd(&buf, 0x35);	/* set I/O options */
      sendbyte(&buf, 0);	/* no position output */
      sendbyte(&buf, 0);	/* no velocity output */
      sendbyte(&buf, 7);	/* UTC, compute on seconds, send only on request */
      sendbyte(&buf, 0);	/* no raw measurements */
      sendetx(&buf, parse);

      sendcmd(&buf, 0x2f);	/* request UTC correction data */
      sendetx(&buf, parse);
      return 0;
    }
  else
    return 1;
}

#if defined(RAWDCF_SETDTR)
/*--------------------------------------------------
 * rawdcf_init - set up modem lines for RAWDCF receivers
 */
#if defined(TIOCMSET) && (defined(TIOCM_DTR) || defined(CIOCM_DTR))
static int
rawdcf_init(parse)
  struct parseunit *parse;
{
  /*
   * You can use the RS232 to supply the power for a DCF77 receiver.
   * Here a voltage between the DTR and the RTS line is used. Unfortunately
   * the name has changed from CIOCM_DTR to TIOCM_DTR recently.
   */
	
#ifdef TIOCM_DTR
  int sl232 = TIOCM_DTR;	/* turn on DTR for power supply */
#else
  int sl232 = CIOCM_DTR;	/* turn on DTR for power supply */
#endif

  if (ioctl(parse->generic->io.fd, TIOCMSET, &sl232) == -1)
    {
      msyslog(LOG_NOTICE, "PARSE receiver #%d: rawdcf_init: WARNING: ioctl(fd, TIOCMSET, [C|T]IOCM_DTR): %m", CLK_UNIT(parse->peer));
    }
  return 0;
}
#else
static int
rawdcf_init(parse)
  struct parseunit *parse;
{
  msyslog(LOG_NOTICE, "PARSE receiver #%d: rawdcf_init: WARNING: OS interface incapable of setting DTR to power DCF modules", CLK_UNIT(parse->peer));
  return 0;
}
#endif  /* DTR initialisation type */

#endif  /* RAWDCF SET DTR option */

#else	/* defined(REFCLOCK) && defined(PARSE) */
int refclock_parse_bs;
#endif	/* defined(REFCLOCK) && defined(PARSE) */

/*
 * History:
 *
 * refclock_parse.c,v
 * Revision 3.98  1997/01/19 14:11:59  kardel
 * removed superfluous functions
 *
 * Revision 3.97  1997/01/19 12:46:01  kardel
 * 3-5.88.1 reconcilation
 *
 * Revision 3.96  1996/12/01 16:05:49  kardel
 * freeze for 5.86.12.2 PARSE-Patch
 *
 * Revision 3.95  1996/12/01 13:13:47  kardel
 * POP "parse" streams module at shutdown
 *
 * Revision 3.94  1996/11/24 20:10:54  kardel
 * RELEASE_5_86_12_2 reconcilation
 *
 * Revision 3.93  1996/11/16 19:13:47  kardel
 * Added DIEM receiver
 *
 * Revision 3.92  1996/10/05 13:30:29  kardel
 * general update
 *
 * Revision 3.91  1996/06/01 17:05:53  kardel
 * cut back revision log
 *
 * Revision 3.90  1996/06/01 16:45:19  kardel
 * cleaned up DTR setup for RAWDCF
 *
 * Revision 3.88  1995/12/18 00:11:24  kardel
 * undo RCC8000 modification
 *
 * Revision 3.87  1995/12/17  22:54:34  kardel
 * fixed HOPF description
 *
 * Revision 3.86  1995/12/17  22:24:29  kardel
 * HOPF 6021 Funkuhr added
 *
 * Revision 3.85  1995/10/16  01:49:50  duwe
 * look for sys/ppsclock.h in the right place
 *
 * Revision 3.84  1995/10/06  15:58:01  kardel
 * auto init for Trimble TAIP
 *
 * Revision 3.83  1995/09/09  16:52:26  kardel
 * more space for refclock variables
 *
 * Revision 3.82  1995/09/03  17:54:47  kardel
 * use unsigned characters for input
 *
 * Revision 3.81  1995/08/26  07:27:20  kardel
 * Trimble TAIP NOPOLL + RECOVERY
 *
 * Revision 3.80  1995/07/29  08:04:17  kardel
 * unsigned char/int problem
 *
 * Revision information 3.1 - 3.79 from log deleted 1996/06/01 kardel
 *
 */
 
