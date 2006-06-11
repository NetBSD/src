/*	$NetBSD: refclock_wwv.c,v 1.5 2006/06/11 19:34:13 kardel Exp $	*/

/*
 * refclock_wwv - clock driver for NIST WWV/H time/frequency station
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && defined(CLOCK_WWV)

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_refclock.h"
#include "ntp_calendar.h"
#include "ntp_stdlib.h"
#include "audio.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */

#define ICOM 1

#ifdef ICOM
#include "icom.h"
#endif /* ICOM */

/*
 * Audio WWV/H demodulator/decoder
 *
 * This driver synchronizes the computer time using data encoded in
 * radio transmissions from NIST time/frequency stations WWV in Boulder,
 * CO, and WWVH in Kauai, HI. Transmissions are made continuously on
 * 2.5, 5, 10, 15 and 20 MHz in AM mode. An ordinary shortwave receiver
 * can be tuned manually to one of these frequencies or, in the case of
 * ICOM receivers, the receiver can be tuned automatically using this
 * program as propagation conditions change throughout the day and
 * night.
 *
 * The driver receives, demodulates and decodes the radio signals when
 * connected to the audio codec of a workstation running Solaris, SunOS
 * FreeBSD or Linux, and with a little help, other workstations with
 * similar codecs or sound cards. In this implementation, only one audio
 * driver and codec can be supported on a single machine.
 *
 * The demodulation and decoding algorithms used in this driver are
 * based on those developed for the TAPR DSP93 development board and the
 * TI 320C25 digital signal processor described in: Mills, D.L. A
 * precision radio clock for WWV transmissions. Electrical Engineering
 * Report 97-8-1, University of Delaware, August 1997, 25 pp., available
 * from www.eecis.udel.edu/~mills/reports.htm. The algorithms described
 * in this report have been modified somewhat to improve performance
 * under weak signal conditions and to provide an automatic station
 * identification feature.
 *
 * The ICOM code is normally compiled in the driver. It isn't used,
 * unless the mode keyword on the server configuration command specifies
 * a nonzero ICOM ID select code. The C-IV trace is turned on if the
 * debug level is greater than one.
 *
 * Fudge factors
 *
 * Fudge flag4 causes the dubugging output described above to be
 * recorded in the clockstats file. Fudge flag2 selects the audio input
 * port, where 0 is the mike port (default) and 1 is the line-in port.
 * It does not seem useful to select the compact disc player port. Fudge
 * flag3 enables audio monitoring of the input signal. For this purpose,
 * the monitor gain is set to a default value.
 */
/*
 * General definitions. Note the DGAIN parameter might need to be
 * changed to fit the audio response of the radio at 100 Hz. The
 * WWV/WWVH data subcarrier is transmitted at 10 dB down from 100
 * percent modulation; however, the matched filter boosts it by a factor
 * of 17 and the receiver bandpass does what it does. The compromise
 * value here is five. Your milege may vary.. The FREQ_OFFSET parameter
 * can be used as a frequency vernier to correct codec frequency if
 * greater than MAXFREQ.
 */
#define	DEVICE_AUDIO	"/dev/audio" /* audio device name */
#define	AUDIO_BUFSIZ	320	/* audio buffer size (50 ms) */
#define	PRECISION	(-10)	/* precision assumed (about 1 ms) */
#define	DESCRIPTION	"WWV/H Audio Demodulator/Decoder" /* WRU */
#define SECOND		8000	/* second epoch (sample rate) (Hz) */
#define MINUTE		(SECOND * 60) /* minute epoch */
#define OFFSET		128	/* companded sample offset */
#define SIZE		256	/* decompanding table size */
#define	MAXSIG		6000.	/* max signal level reference */
#define	MAXCLP		100	/* max clips above reference per s */
#define MAXSNR		40.	/* max SNR reference */
#define MAXFREQ		1.5	/* max frequency tolerance (187 PPM) */
#define DATCYC		170	/* data filter cycles */
#define DATSIZ		(DATCYC * MS) /* data filter size */
#define SYNCYC		800	/* minute filter cycles */
#define SYNSIZ		(SYNCYC * MS) /* minute filter size */
#define TCKCYC		5	/* tick filter cycles */
#define TCKSIZ		(TCKCYC * MS) /* tick filter size */
#define NCHAN		5	/* number of radio channels */
#define DCHAN		3	/* default radio channel (15 Mhz) */
#define	AUDIO_PHI	5e-6	/* dispersion growth factor */
#define DGAIN		5.	/* subcarrier gain */
#define	FREQ_OFFSET	0	/* codec frequency correction (PPM) */

/*
 * General purpose status bits (status)
 *
 * SELV and/or SELH are set when WWV or WWVH has been heard and cleared
 * on signal loss. SSYNC is set when the second sync pulse has been
 * acquired and cleared by signal loss. MSYNC is set when the minute
 * sync pulse has been acquired. DSYNC is set when the units digit has
 * has reached the threshold and INSYNC is set when all nine digits have
 * reached the threshold. The MSYNC, DSYNC and INSYNC bits are cleared
 * only by timeout, upon which the driver starts over from scratch.
 *
 * DGATE is lit if the data bit amplitude or SNR is below thresholds and
 * BGATE is lit if a the pulse width amplitude or SNR is below
 * thresolds. LEPSEC is set during the last minute of the leap day At
 * the end of this minute the driver inserts second 60 in the seconds
 * state machine and the minute sync slips a second.
 */
#define MSYNC		0x0001	/* minute epoch sync */
#define SSYNC		0x0002	/* second epoch sync */
#define DSYNC		0x0004	/* minute units sync */
#define INSYNC		0x0008	/* clock synchronized */
#define FGATE		0x0010	/* frequency gate */
#define DGATE		0x0020	/* data pulse amplitude error */
#define BGATE		0x0040	/* data pulse width error */
#define LEPSEC		0x1000	/* leap minute */

/*
 * Station scoreboard bits
 *
 * These are used to establish the signal quality for each of the five
 * frequencies and two stations.
 */
#define SELV		0x0100	/* WWV station select */
#define SELH		0x0200	/* WWVH station select */

/*
 * Alarm status bits (alarm)
 *
 * These bits indicate various alarm conditions, which are decoded to
 * form the quality character included in the timecode.
 */
#define CMPERR		1	/* BCD digit compare error */
#define LOWERR		2	/* low bit or digit amplitude or SNR */
#define NINERR		4	/* less than nine digits in minute */
#define SYNERR		8	/* not tracking second sync */

/*
 * Watchcat timeouts (watch)
 *
 * If these timeouts expire, the status bits are mashed to zero and the
 * driver starts from scratch. Suitably more refined procedures may be
 * developed in future. All these are in minutes.
 */
#define ACQSN		5	/* station acquisition timeout */
#define DATA		10	/* unit minutes timeout */
#define SYNCH		30	/* station sync timeout */
#define PANIC		(2 * 1440) /* panic timeout */

/*
 * Thresholds. These establish the minimum signal level, minimum SNR and
 * maximum jitter thresholds which establish the error and false alarm
 * rates of the driver. The values defined here may be on the
 * adventurous side in the interest of the highest sensitivity.
 */
#define MTHR		13.	/* acquisition signal gate (percent) */
#define TTHR		50.	/* tracking signal gate (percent) */
#define AWND		20	/* acquisition jitter threshold (ms) */
#define ATHR		3000.	/* QRZ minute sync threshold */
#define ASNR		20.	/* QRZ minute sync SNR threshold (dB) */
#define QTHR		3000.	/* QSY minute sync threshold */
#define QSNR		20.	/* QSY minute sync SNR threshold (dB) */
#define STHR		3000.	/* second sync threshold */
#define	SSNR		15.	/* second sync SNR threshold (dB) */
#define SCMP		10 	/* second sync compare threshold */
#define DTHR		1000.	/* bit threshold */
#define DSNR		10.	/* bit SNR threshold (dB) */
#define AMIN		3	/* min bit count */
#define AMAX		6	/* max bit count */
#define BTHR		1000.	/* digit threshold */
#define BSNR		3.	/* digit likelihood threshold (dB) */
#define BCMP		3	/* digit compare threshold */
#define	MAXERR		20	/* maximum error alarm */

/*
 * Tone frequency definitions. The increments are for 4.5-deg sine
 * table.
 */
#define MS		(SECOND / 1000) /* samples per millisecond */
#define IN100		((100 * 80) / SECOND) /* 100 Hz increment */
#define IN1000		((1000 * 80) / SECOND) /* 1000 Hz increment */
#define IN1200		((1200 * 80) / SECOND) /* 1200 Hz increment */

/*
 * Acquisition and tracking time constants
 */
#define MINAVG		8	/* min averaging time */
#define MAXAVG		1024	/* max averaging time */
#define FCONST		3	/* frequency time constant */
#define TCONST		16	/* data bit/digit time constant */

/*
 * Miscellaneous status bits (misc)
 *
 * These bits correspond to designated bits in the WWV/H timecode. The
 * bit probabilities are exponentially averaged over several minutes and
 * processed by a integrator and threshold.
 */
#define DUT1		0x01	/* 56 DUT .1 */
#define DUT2		0x02	/* 57 DUT .2 */
#define DUT4		0x04	/* 58 DUT .4 */
#define DUTS		0x08	/* 50 DUT sign */
#define DST1		0x10	/* 55 DST1 leap warning */
#define DST2		0x20	/* 2 DST2 DST1 delayed one day */
#define SECWAR		0x40	/* 3 leap second warning */

/*
 * The on-time synchronization point for the driver is the second epoch
 * sync pulse produced by the FIR matched filters. As the 5-ms delay of
 * these filters is compensated, the program delay is 1.1 ms due to the
 * 600-Hz IIR bandpass filter. The measured receiver delay is 4.7 ms and
 * the codec delay less than 0.2 ms. The additional propagation delay
 * specific to each receiver location can be programmed in the fudge
 * time1 and time2 values for WWV and WWVH, respectively.
 */
#define PDELAY	(.0011 + .0047 + .0002)	/* net system delay (s) */

/*
 * Table of sine values at 4.5-degree increments. This is used by the
 * synchronous matched filter demodulators.
 */
double sintab[] = {
 0.000000e+00,  7.845910e-02,  1.564345e-01,  2.334454e-01, /* 0-3 */
 3.090170e-01,  3.826834e-01,  4.539905e-01,  5.224986e-01, /* 4-7 */
 5.877853e-01,  6.494480e-01,  7.071068e-01,  7.604060e-01, /* 8-11 */
 8.090170e-01,  8.526402e-01,  8.910065e-01,  9.238795e-01, /* 12-15 */
 9.510565e-01,  9.723699e-01,  9.876883e-01,  9.969173e-01, /* 16-19 */
 1.000000e+00,  9.969173e-01,  9.876883e-01,  9.723699e-01, /* 20-23 */
 9.510565e-01,  9.238795e-01,  8.910065e-01,  8.526402e-01, /* 24-27 */
 8.090170e-01,  7.604060e-01,  7.071068e-01,  6.494480e-01, /* 28-31 */
 5.877853e-01,  5.224986e-01,  4.539905e-01,  3.826834e-01, /* 32-35 */
 3.090170e-01,  2.334454e-01,  1.564345e-01,  7.845910e-02, /* 36-39 */
-0.000000e+00, -7.845910e-02, -1.564345e-01, -2.334454e-01, /* 40-43 */
-3.090170e-01, -3.826834e-01, -4.539905e-01, -5.224986e-01, /* 44-47 */
-5.877853e-01, -6.494480e-01, -7.071068e-01, -7.604060e-01, /* 48-51 */
-8.090170e-01, -8.526402e-01, -8.910065e-01, -9.238795e-01, /* 52-55 */
-9.510565e-01, -9.723699e-01, -9.876883e-01, -9.969173e-01, /* 56-59 */
-1.000000e+00, -9.969173e-01, -9.876883e-01, -9.723699e-01, /* 60-63 */
-9.510565e-01, -9.238795e-01, -8.910065e-01, -8.526402e-01, /* 64-67 */
-8.090170e-01, -7.604060e-01, -7.071068e-01, -6.494480e-01, /* 68-71 */
-5.877853e-01, -5.224986e-01, -4.539905e-01, -3.826834e-01, /* 72-75 */
-3.090170e-01, -2.334454e-01, -1.564345e-01, -7.845910e-02, /* 76-79 */
 0.000000e+00};						    /* 80 */

/*
 * Decoder operations at the end of each second are driven by a state
 * machine. The transition matrix consists of a dispatch table indexed
 * by second number. Each entry in the table contains a case switch
 * number and argument.
 */
struct progx {
	int sw;			/* case switch number */
	int arg;		/* argument */
};

/*
 * Case switch numbers
 */
#define IDLE		0	/* no operation */
#define COEF		1	/* BCD bit */
#define COEF1		2	/* BCD bit for minute unit */
#define COEF2		3	/* BCD bit not used */
#define DECIM9		4	/* BCD digit 0-9 */
#define DECIM6		5	/* BCD digit 0-6 */
#define DECIM3		6	/* BCD digit 0-3 */
#define DECIM2		7	/* BCD digit 0-2 */
#define MSCBIT		8	/* miscellaneous bit */
#define MSC20		9	/* miscellaneous bit */		
#define MSC21		10	/* QSY probe channel */		
#define MIN1		11	/* latch time */		
#define MIN2		12	/* leap second */
#define SYNC2		13	/* latch minute sync pulse */		
#define SYNC3		14	/* latch data pulse */		

/*
 * Offsets in decoding matrix
 */
#define MN		0	/* minute digits (2) */
#define HR		2	/* hour digits (2) */
#define DA		4	/* day digits (3) */
#define YR		7	/* year digits (2) */

struct progx progx[] = {
	{SYNC2,	0},		/* 0 latch minute sync pulse */
	{SYNC3,	0},		/* 1 latch data pulse */
	{MSCBIT, DST2},		/* 2 dst2 */
	{MSCBIT, SECWAR},	/* 3 lw */
	{COEF,	0},		/* 4 1 year units */
	{COEF,	1},		/* 5 2 */
	{COEF,	2},		/* 6 4 */
	{COEF,	3},		/* 7 8 */
	{DECIM9, YR},		/* 8 */
	{IDLE,	0},		/* 9 p1 */
	{COEF1,	0},		/* 10 1 minute units */
	{COEF1,	1},		/* 11 2 */
	{COEF1,	2},		/* 12 4 */
	{COEF1,	3},		/* 13 8 */
	{DECIM9, MN},		/* 14 */
	{COEF,	0},		/* 15 10 minute tens */
	{COEF,	1},		/* 16 20 */
	{COEF,	2},		/* 17 40 */
	{COEF2,	3},		/* 18 80 (not used) */
	{DECIM6, MN + 1},	/* 19 p2 */
	{COEF,	0},		/* 20 1 hour units */
	{COEF,	1},		/* 21 2 */
	{COEF,	2},		/* 22 4 */
	{COEF,	3},		/* 23 8 */
	{DECIM9, HR},		/* 24 */
	{COEF,	0},		/* 25 10 hour tens */
	{COEF,	1},		/* 26 20 */
	{COEF2,	2},		/* 27 40 (not used) */
	{COEF2,	3},		/* 28 80 (not used) */
	{DECIM2, HR + 1},	/* 29 p3 */
	{COEF,	0},		/* 30 1 day units */
	{COEF,	1},		/* 31 2 */
	{COEF,	2},		/* 32 4 */
	{COEF,	3},		/* 33 8 */
	{DECIM9, DA},		/* 34 */
	{COEF,	0},		/* 35 10 day tens */
	{COEF,	1},		/* 36 20 */
	{COEF,	2},		/* 37 40 */
	{COEF,	3},		/* 38 80 */
	{DECIM9, DA + 1},	/* 39 p4 */
	{COEF,	0},		/* 40 100 day hundreds */
	{COEF,	1},		/* 41 200 */
	{COEF2,	2},		/* 42 400 (not used) */
	{COEF2,	3},		/* 43 800 (not used) */
	{DECIM3, DA + 2},	/* 44 */
	{IDLE,	0},		/* 45 */
	{IDLE,	0},		/* 46 */
	{IDLE,	0},		/* 47 */
	{IDLE,	0},		/* 48 */
	{IDLE,	0},		/* 49 p5 */
	{MSCBIT, DUTS},		/* 50 dut+- */
	{COEF,	0},		/* 51 10 year tens */
	{COEF,	1},		/* 52 20 */
	{COEF,	2},		/* 53 40 */
	{COEF,	3},		/* 54 80 */
	{MSC20, DST1},		/* 55 dst1 */
	{MSCBIT, DUT1},		/* 56 0.1 dut */
	{MSCBIT, DUT2},		/* 57 0.2 */
	{MSC21, DUT4},		/* 58 0.4 QSY probe channel */
	{MIN1,	0},		/* 59 p6 latch time */
	{MIN2,	0}		/* 60 leap second */
};

/*
 * BCD coefficients for maximum likelihood digit decode
 */
#define P15	1.		/* max positive number */
#define N15	-1.		/* max negative number */

/*
 * Digits 0-9
 */
#define P9	(P15 / 4)	/* mark (+1) */
#define N9	(N15 / 4)	/* space (-1) */

double bcd9[][4] = {
	{N9, N9, N9, N9}, 	/* 0 */
	{P9, N9, N9, N9}, 	/* 1 */
	{N9, P9, N9, N9}, 	/* 2 */
	{P9, P9, N9, N9}, 	/* 3 */
	{N9, N9, P9, N9}, 	/* 4 */
	{P9, N9, P9, N9}, 	/* 5 */
	{N9, P9, P9, N9}, 	/* 6 */
	{P9, P9, P9, N9}, 	/* 7 */
	{N9, N9, N9, P9}, 	/* 8 */
	{P9, N9, N9, P9}, 	/* 9 */
	{0, 0, 0, 0}		/* backstop */
};

/*
 * Digits 0-6 (minute tens)
 */
#define P6	(P15 / 3)	/* mark (+1) */
#define N6	(N15 / 3)	/* space (-1) */

double bcd6[][4] = {
	{N6, N6, N6, 0}, 	/* 0 */
	{P6, N6, N6, 0}, 	/* 1 */
	{N6, P6, N6, 0}, 	/* 2 */
	{P6, P6, N6, 0}, 	/* 3 */
	{N6, N6, P6, 0}, 	/* 4 */
	{P6, N6, P6, 0}, 	/* 5 */
	{N6, P6, P6, 0}, 	/* 6 */
	{0, 0, 0, 0}		/* backstop */
};

/*
 * Digits 0-3 (day hundreds)
 */
#define P3	(P15 / 2)	/* mark (+1) */
#define N3	(N15 / 2)	/* space (-1) */

double bcd3[][4] = {
	{N3, N3, 0, 0}, 	/* 0 */
	{P3, N3, 0, 0}, 	/* 1 */
	{N3, P3, 0, 0}, 	/* 2 */
	{P3, P3, 0, 0}, 	/* 3 */
	{0, 0, 0, 0}		/* backstop */
};

/*
 * Digits 0-2 (hour tens)
 */
#define P2	(P15 / 2)	/* mark (+1) */
#define N2	(N15 / 2)	/* space (-1) */

double bcd2[][4] = {
	{N2, N2, 0, 0}, 	/* 0 */
	{P2, N2, 0, 0}, 	/* 1 */
	{N2, P2, 0, 0}, 	/* 2 */
	{0, 0, 0, 0}		/* backstop */
};

/*
 * DST decode (DST2 DST1) for prettyprint
 */
char dstcod[] = {
	'S',			/* 00 standard time */
	'I',			/* 01 set clock ahead at 0200 local */
	'O',			/* 10 set clock back at 0200 local */
	'D'			/* 11 daylight time */
};

/*
 * The decoding matrix consists of nine row vectors, one for each digit
 * of the timecode. The digits are stored from least to most significant
 * order. The maximum likelihood timecode is formed from the digits
 * corresponding to the maximum likelihood values reading in the
 * opposite order: yy ddd hh:mm.
 */
struct decvec {
	int radix;		/* radix (3, 4, 6, 10) */
	int digit;		/* current clock digit */
	int mldigit;		/* maximum likelihood digit */
	int count;		/* match count */
	double digprb;		/* max digit probability */
	double digsnr;		/* likelihood function (dB) */
	double like[10];	/* likelihood integrator 0-9 */
};

/*
 * The station structure (sp) is used to acquire the minute pulse from
 * WWV and/or WWVH. These stations are distinguished by the frequency
 * used for the second and minute sync pulses, 1000 Hz for WWV and 1200
 * Hz for WWVH. Other than frequency, the format is the same.
 */
struct sync {
	double	epoch;		/* accumulated epoch differences */
	double	maxeng;		/* sync max energy */
	double	noieng;		/* sync noise energy */
	long	pos;		/* max amplitude position */
	long	lastpos;	/* last max position */
	long	mepoch;		/* minute synch epoch */

	double	amp;		/* sync signal */
	double	syneng;		/* sync signal max 800 ms */
	double	synmax;		/* sync signal max 0 s */
	double	synsnr;		/* sync signal SNR */
	double	metric;		/* signal quality metric */
	int	reach;		/* reachability register */
	int	count;		/* bit counter */
	int	select;		/* select bits */
	char	refid[5];	/* reference identifier */
};

/*
 * The channel structure (cp) is used to mitigate between channels.
 */
struct chan {
	int	gain;		/* audio gain */
	struct sync wwv;	/* wwv station */
	struct sync wwvh;	/* wwvh station */
};

/*
 * WWV unit control structure (up)
 */
struct wwvunit {
	l_fp	timestamp;	/* audio sample timestamp */
	l_fp	tick;		/* audio sample increment */
	double	phase, freq;	/* logical clock phase and frequency */
	double	monitor;	/* audio monitor point */
#ifdef ICOM
	int	fd_icom;	/* ICOM file descriptor */
#endif /* ICOM */
	int	errflg;		/* error flags */
	int	watch;		/* watchcat */

	/*
	 * Audio codec variables
	 */
	double	comp[SIZE];	/* decompanding table */
	int	port;		/* codec port */
	int	gain;		/* codec gain */
	int	mongain;	/* codec monitor gain */
	int	clipcnt;	/* sample clipped count */

	/*
	 * Variables used to establish basic system timing
	 */
	int	avgint;		/* master time constant */
	int	tepoch;		/* sync epoch median */
	int	yepoch;		/* sync epoch */
	int	repoch;		/* buffered sync epoch */
	double	epomax;		/* second sync amplitude */
	double	eposnr;		/* second sync SNR */
	double	irig;		/* data I channel amplitude */
	double	qrig;		/* data Q channel amplitude */
	int	datapt;		/* 100 Hz ramp */
	double	datpha;		/* 100 Hz VFO control */
	int	rphase;		/* second sample counter */
	long	mphase;		/* minute sample counter */

	/*
	 * Variables used to mitigate which channel to use
	 */
	struct chan mitig[NCHAN]; /* channel data */
	struct sync *sptr;	/* station pointer */
	int	dchan;		/* data channel */
	int	schan;		/* probe channel */
	int	achan;		/* active channel */

	/*
	 * Variables used by the clock state machine
	 */
	struct decvec decvec[9]; /* decoding matrix */
	int	rsec;		/* seconds counter */
	int	digcnt;		/* count of digits synchronized */

	/*
	 * Variables used to estimate signal levels and bit/digit
	 * probabilities
	 */
	double	datsig;		/* data signal max */
	double	datsnr;		/* data signal SNR (dB) */

	/*
	 * Variables used to establish status and alarm conditions
	 */
	int	status;		/* status bits */
	int	alarm;		/* alarm flashers */
	int	misc;		/* miscellaneous timecode bits */
	int	errcnt;		/* data bit error counter */
};

/*
 * Function prototypes
 */
static	int	wwv_start	P((int, struct peer *));
static	void	wwv_shutdown	P((int, struct peer *));
static	void	wwv_receive	P((struct recvbuf *));
static	void	wwv_poll	P((int, struct peer *));

/*
 * More function prototypes
 */
static	void	wwv_epoch	P((struct peer *));
static	void	wwv_rf		P((struct peer *, double));
static	void	wwv_endpoc	P((struct peer *, int));
static	void	wwv_rsec	P((struct peer *, double));
static	void	wwv_qrz		P((struct peer *, struct sync *, int));
static	void	wwv_corr4	P((struct peer *, struct decvec *,
				    double [], double [][4]));
static	void	wwv_gain	P((struct peer *));
static	void	wwv_tsec	P((struct peer *));
static	int	timecode	P((struct wwvunit *, char *));
static	double	wwv_snr		P((double, double));
static	int	carry		P((struct decvec *));
static	int	wwv_newchan	P((struct peer *));
static	void	wwv_newgame	P((struct peer *, int));
static	double	wwv_metric	P((struct sync *));
static	void	wwv_clock	P((struct peer *));
#ifdef ICOM
static	int	wwv_qsy		P((struct peer *, int));
#endif /* ICOM */

static double qsy[NCHAN] = {2.5, 5, 10, 15, 20}; /* frequencies (MHz) */

/*
 * Transfer vector
 */
struct	refclock refclock_wwv = {
	wwv_start,		/* start up driver */
	wwv_shutdown,		/* shut down driver */
	wwv_poll,		/* transmit poll message */
	noentry,		/* not used (old wwv_control) */
	noentry,		/* initialize driver (not used) */
	noentry,		/* not used (old wwv_buginfo) */
	NOFLAGS			/* not used */
};


/*
 * wwv_start - open the devices and initialize data for processing
 */
static int
wwv_start(
	int	unit,		/* instance number (used by PCM) */
	struct peer *peer	/* peer structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
#ifdef ICOM
	int	temp;
#endif /* ICOM */

	/*
	 * Local variables
	 */
	int	fd;		/* file descriptor */
	int	i;		/* index */
	double	step;		/* codec adjustment */

	/*
	 * Open audio device
	 */
	fd = audio_init(DEVICE_AUDIO, AUDIO_BUFSIZ, unit);
	if (fd < 0)
		return (0);
#ifdef DEBUG
	if (debug)
		audio_show();
#endif

	/*
	 * Allocate and initialize unit structure
	 */
	if (!(up = (struct wwvunit *)emalloc(sizeof(struct wwvunit)))) {
		close(fd);
		return (0);
	}
	memset(up, 0, sizeof(struct wwvunit));
	pp = peer->procptr;
	pp->unitptr = (caddr_t)up;
	pp->io.clock_recv = wwv_receive;
	pp->io.srcclock = (caddr_t)peer;
	pp->io.datalen = 0;
	pp->io.fd = fd;
	if (!io_addclock(&pp->io)) {
		close(fd);
		free(up);
		return (0);
	}

	/*
	 * Initialize miscellaneous variables
	 */
	peer->precision = PRECISION;
	pp->clockdesc = DESCRIPTION;

	/*
	 * The companded samples are encoded sign-magnitude. The table
	 * contains all the 256 values in the interest of speed.
	 */
	up->comp[0] = up->comp[OFFSET] = 0.;
	up->comp[1] = 1.; up->comp[OFFSET + 1] = -1.;
	up->comp[2] = 3.; up->comp[OFFSET + 2] = -3.;
	step = 2.;
	for (i = 3; i < OFFSET; i++) {
		up->comp[i] = up->comp[i - 1] + step;
		up->comp[OFFSET + i] = -up->comp[i];
                if (i % 16 == 0)
		    step *= 2.;
	}
	DTOLFP(1. / SECOND, &up->tick);

	/*
	 * Initialize the decoding matrix with the radix for each digit
	 * position.
	 */
	up->decvec[MN].radix = 10;	/* minutes */
	up->decvec[MN + 1].radix = 6;
	up->decvec[HR].radix = 10;	/* hours */
	up->decvec[HR + 1].radix = 3;
	up->decvec[DA].radix = 10;	/* days */
	up->decvec[DA + 1].radix = 10;
	up->decvec[DA + 2].radix = 4;
	up->decvec[YR].radix = 10;	/* years */
	up->decvec[YR + 1].radix = 10;

#ifdef ICOM
	/*
	 * Initialize autotune if available. Note that the ICOM select
	 * code must be less than 128, so the high order bit can be used
	 * to select the line speed 0 (9600 bps) or 1 (1200 bps).
	 */
	temp = 0;
#ifdef DEBUG
	if (debug > 1)
		temp = P_TRACE;
#endif
	if (peer->ttl != 0) {
		if (peer->ttl & 0x80)
			up->fd_icom = icom_init("/dev/icom", B1200,
			    temp);
		else
			up->fd_icom = icom_init("/dev/icom", B9600,
			    temp);
		if (up->fd_icom < 0) {
			NLOG(NLOG_SYNCEVENT | NLOG_SYSEVENT)
			    msyslog(LOG_NOTICE,
			    "icom: %m");
			up->errflg = CEVNT_FAULT;
		}
	}
	if (up->fd_icom > 0) {
		if (wwv_qsy(peer, DCHAN) != 0) {
			NLOG(NLOG_SYNCEVENT | NLOG_SYSEVENT)
			    msyslog(LOG_NOTICE,
			    "icom: radio not found");
			up->errflg = CEVNT_FAULT;
			close(up->fd_icom);
			up->fd_icom = 0;
		} else {
			NLOG(NLOG_SYNCEVENT | NLOG_SYSEVENT)
			    msyslog(LOG_NOTICE,
			    "icom: autotune enabled");
		}
	}
#endif /* ICOM */

	/*
	 * Let the games begin.
	 */
	wwv_newgame(peer, DCHAN);
	return (1);
}


/*
 * wwv_shutdown - shut down the clock
 */
static void
wwv_shutdown(
	int	unit,		/* instance number (not used) */
	struct peer *peer	/* peer structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;
	io_closeclock(&pp->io);
#ifdef ICOM
	if (up->fd_icom > 0)
		close(up->fd_icom);
#endif /* ICOM */
	free(up);
}


/*
 * wwv_receive - receive data from the audio device
 *
 * This routine reads input samples and adjusts the logical clock to
 * track the A/D sample clock by dropping or duplicating codec samples.
 * It also controls the A/D signal level with an AGC loop to mimimize
 * quantization noise and avoid overload.
 */
static void
wwv_receive(
	struct recvbuf *rbufp	/* receive buffer structure pointer */
	)
{
	struct peer *peer;
	struct refclockproc *pp;
	struct wwvunit *up;

	/*
	 * Local variables
	 */
	double	sample;		/* codec sample */
	u_char	*dpt;		/* buffer pointer */
	int	bufcnt;		/* buffer counter */
	l_fp	ltemp;

	peer = (struct peer *)rbufp->recv_srcclock;
	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Main loop - read until there ain't no more. Note codec
	 * samples are bit-inverted.
	 */
	DTOLFP((double)rbufp->recv_length / SECOND, &ltemp);
	L_SUB(&rbufp->recv_time, &ltemp);
	up->timestamp = rbufp->recv_time;
	dpt = rbufp->recv_buffer;
	for (bufcnt = 0; bufcnt < rbufp->recv_length; bufcnt++) {
		sample = up->comp[~*dpt++ & 0xff];

		/*
		 * Clip noise spikes greater than MAXSIG. If no clips,
		 * increase the gain a tad; if the clips are too high, 
		 * decrease a tad.
		 */
		if (sample > MAXSIG) {
			sample = MAXSIG;
			up->clipcnt++;
		} else if (sample < -MAXSIG) {
			sample = -MAXSIG;
			up->clipcnt++;
		}

		/*
		 * Variable frequency oscillator. The codec oscillator
		 * runs at the nominal rate of 8000 samples per second,
		 * or 125 us per sample. A frequency change of one unit
		 * results in either duplicating or deleting one sample
		 * per second, which results in a frequency change of
		 * 125 PPM.
		 */
		up->phase += up->freq / SECOND;
		up->phase += FREQ_OFFSET / 1e6;
		if (up->phase >= .5) {
			up->phase -= 1.;
		} else if (up->phase < -.5) {
			up->phase += 1.;
			wwv_rf(peer, sample);
			wwv_rf(peer, sample);
		} else {
			wwv_rf(peer, sample);
		}
		L_ADD(&up->timestamp, &up->tick);
	}

	/*
	 * Set the input port and monitor gain for the next buffer.
	 */
	if (pp->sloppyclockflag & CLK_FLAG2)
		up->port = 2;
	else
		up->port = 1;
	if (pp->sloppyclockflag & CLK_FLAG3)
		up->mongain = MONGAIN;
	else
		up->mongain = 0;
}


/*
 * wwv_poll - called by the transmit procedure
 *
 * This routine keeps track of status. If no offset samples have been
 * processed during a poll interval, a timeout event is declared. If
 * errors have have occurred during the interval, they are reported as
 * well. Once the clock is set, it always appears reachable, unless
 * reset by watchcat timeout.
 */
static void
wwv_poll(
	int	unit,		/* instance number (not used) */
	struct peer *peer	/* peer structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;
	if (pp->coderecv == pp->codeproc)
		up->errflg = CEVNT_TIMEOUT;
	if (up->errflg)
		refclock_report(peer, up->errflg);
	up->errflg = 0;
	pp->polls++;
}


/*
 * wwv_rf - process signals and demodulate to baseband
 *
 * This routine grooms and filters decompanded raw audio samples. The
 * output signals include the 100-Hz baseband data signal in quadrature
 * form, plus the epoch index of the second sync signal and the second
 * index of the minute sync signal.
 *
 * There are two 1-s ramps used by this program. Both count the 8000
 * logical clock samples spanning exactly one second. The epoch ramp
 * counts the samples starting at an arbitrary time. The rphase ramp
 * counts the samples starting at the 5-ms second sync pulse found
 * during the epoch ramp.
 *
 * There are two 1-m ramps used by this program. The mphase ramp counts
 * the 480,000 logical clock samples spanning exactly one minute and
 * starting at an arbitrary time. The rsec ramp counts the 60 seconds of
 * the minute starting at the 800-ms minute sync pulse found during the
 * mphase ramp. The rsec ramp drives the seconds state machine to
 * determine the bits and digits of the timecode. 
 *
 * Demodulation operations are based on three synthesized quadrature
 * sinusoids: 100 Hz for the data signal, 1000 Hz for the WWV sync
 * signal and 1200 Hz for the WWVH sync signal. These drive synchronous
 * matched filters for the data signal (170 ms at 100 Hz), WWV minute
 * sync signal (800 ms at 1000 Hz) and WWVH minute sync signal (800 ms
 * at 1200 Hz). Two additional matched filters are switched in
 * as required for the WWV second sync signal (5 ms at 1000 Hz) and
 * WWVH second sync signal (5 ms at 1200 Hz).
 */
static void
wwv_rf(
	struct peer *peer,	/* peerstructure pointer */
	double isig		/* input signal */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	struct sync *sp, *rp;

	static double lpf[5];	/* 150-Hz lpf delay line */
	double data;		/* lpf output */
	static double bpf[9];	/* 1000/1200-Hz bpf delay line */
	double syncx;		/* bpf output */
	static double mf[41];	/* 1000/1200-Hz mf delay line */
	double mfsync;		/* mf output */

	static int iptr;	/* data channel pointer */
	static double ibuf[DATSIZ]; /* data I channel delay line */
	static double qbuf[DATSIZ]; /* data Q channel delay line */

	static int jptr;	/* sync channel pointer */
	static int kptr;	/* tick channel pointer */

	static int csinptr;	/* wwv channel phase */
	static double cibuf[SYNSIZ]; /* wwv I channel delay line */
	static double cqbuf[SYNSIZ]; /* wwv Q channel delay line */
	static double ciamp;	/* wwv I channel amplitude */
	static double cqamp;	/* wwv Q channel amplitude */

	static double csibuf[TCKSIZ]; /* wwv I tick delay line */
	static double csqbuf[TCKSIZ]; /* wwv Q tick delay line */
	static double csiamp;	/* wwv I tick amplitude */
	static double csqamp;	/* wwv Q tick amplitude */

	static int hsinptr;	/* wwvh channels phase */
	static double hibuf[SYNSIZ]; /* wwvh I channel delay line */
	static double hqbuf[SYNSIZ]; /* wwvh Q channel delay line */
	static double hiamp;	/* wwvh I channel amplitude */
	static double hqamp;	/* wwvh Q channel amplitude */

	static double hsibuf[TCKSIZ]; /* wwvh I tick delay line */
	static double hsqbuf[TCKSIZ]; /* wwvh Q tick delay line */
	static double hsiamp;	/* wwvh I tick amplitude */
	static double hsqamp;	/* wwvh Q tick amplitude */

	static double epobuf[SECOND]; /* epoch sync comb filter */
	static double epomax;	/* epoch sync amplitude buffer */
	static int epopos;	/* epoch sync position buffer */

	static int iniflg;	/* initialization flag */
	int	epoch;		/* comb filter index */
	int	pdelay;		/* propagation delay (samples) */
	double	dtemp;
	int	i;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	if (!iniflg) {
		iniflg = 1;
		memset((char *)lpf, 0, sizeof(lpf));
		memset((char *)bpf, 0, sizeof(bpf));
		memset((char *)mf, 0, sizeof(mf));
		memset((char *)ibuf, 0, sizeof(ibuf));
		memset((char *)qbuf, 0, sizeof(qbuf));
		memset((char *)cibuf, 0, sizeof(cibuf));
		memset((char *)cqbuf, 0, sizeof(cqbuf));
		memset((char *)csibuf, 0, sizeof(csibuf));
		memset((char *)csqbuf, 0, sizeof(csqbuf));
		memset((char *)hibuf, 0, sizeof(hibuf));
		memset((char *)hqbuf, 0, sizeof(hqbuf));
		memset((char *)hsibuf, 0, sizeof(hsibuf));
		memset((char *)hsqbuf, 0, sizeof(hsqbuf));
		memset((char *)epobuf, 0, sizeof(epobuf));
	}

	/*
	 * Baseband data demodulation. The 100-Hz subcarrier is
	 * extracted using a 150-Hz IIR lowpass filter. This attenuates
	 * the 1000/1200-Hz sync signals, as well as the 440-Hz and
	 * 600-Hz tones and most of the noise and voice modulation
	 * components.
	 *
	 * The subcarrier is transmitted 10 dB down from the carrier.
	 * The DGAIN parameter can be adjusted for this and to
	 * compensate for the radio audio response at 100 Hz.
	 *
	 * Matlab IIR 4th-order IIR elliptic, 150 Hz lowpass, 0.2 dB
	 * passband ripple, -50 dB stopband ripple.
	 */
	data = (lpf[4] = lpf[3]) * 8.360961e-01;
	data += (lpf[3] = lpf[2]) * -3.481740e+00;
	data += (lpf[2] = lpf[1]) * 5.452988e+00;
	data += (lpf[1] = lpf[0]) * -3.807229e+00;
	lpf[0] = isig * DGAIN - data;
	data = lpf[0] * 3.281435e-03
	    + lpf[1] * -1.149947e-02
	    + lpf[2] * 1.654858e-02
	    + lpf[3] * -1.149947e-02
	    + lpf[4] * 3.281435e-03;

	/*
	 * The 100-Hz data signal is demodulated using a pair of
	 * quadrature multipliers, matched filters and a phase lock
	 * loop. The I and Q quadrature data signals are produced by
	 * multiplying the filtered signal by 100-Hz sine and cosine
	 * signals, respectively. The signals are processed by 170-ms
	 * synchronous matched filters to produce the amplitude and
	 * phase signals used by the demodulator. The signals are scaled
	 * to produce unit energy at the maximum value.
	 */
	i = up->datapt;
	up->datapt = (up->datapt + IN100) % 80;
	dtemp = sintab[i] * data / (MS / 2. * DATCYC);
	up->irig -= ibuf[iptr];
	ibuf[iptr] = dtemp;
	up->irig += dtemp;

	i = (i + 20) % 80;
	dtemp = sintab[i] * data / (MS / 2. * DATCYC);
	up->qrig -= qbuf[iptr];
	qbuf[iptr] = dtemp;
	up->qrig += dtemp;
	iptr = (iptr + 1) % DATSIZ;

	/*
	 * Baseband sync demodulation. The 1000/1200 sync signals are
	 * extracted using a 600-Hz IIR bandpass filter. This removes
	 * the 100-Hz data subcarrier, as well as the 440-Hz and 600-Hz
	 * tones and most of the noise and voice modulation components.
	 *
	 * Matlab 4th-order IIR elliptic, 800-1400 Hz bandpass, 0.2 dB
	 * passband ripple, -50 dB stopband ripple.
	 */
	syncx = (bpf[8] = bpf[7]) * 4.897278e-01;
	syncx += (bpf[7] = bpf[6]) * -2.765914e+00;
	syncx += (bpf[6] = bpf[5]) * 8.110921e+00;
	syncx += (bpf[5] = bpf[4]) * -1.517732e+01;
	syncx += (bpf[4] = bpf[3]) * 1.975197e+01;
	syncx += (bpf[3] = bpf[2]) * -1.814365e+01;
	syncx += (bpf[2] = bpf[1]) * 1.159783e+01;
	syncx += (bpf[1] = bpf[0]) * -4.735040e+00;
	bpf[0] = isig - syncx;
	syncx = bpf[0] * 8.203628e-03
	    + bpf[1] * -2.375732e-02
	    + bpf[2] * 3.353214e-02
	    + bpf[3] * -4.080258e-02
	    + bpf[4] * 4.605479e-02
	    + bpf[5] * -4.080258e-02
	    + bpf[6] * 3.353214e-02
	    + bpf[7] * -2.375732e-02
	    + bpf[8] * 8.203628e-03;

	/*
	 * The 1000/1200 sync signals are demodulated using a pair of
	 * quadrature multipliers and matched filters. However,
	 * synchronous demodulation at these frequencies is impractical,
	 * so only the signal amplitude is used. The I and Q quadrature
	 * sync signals are produced by multiplying the filtered signal
	 * by 1000-Hz (WWV) and 1200-Hz (WWVH) sine and cosine signals,
	 * respectively. The WWV and WWVH signals are processed by 800-
	 * ms synchronous matched filters and combined to produce the
	 * minute sync signal and detect which one (or both) the WWV or
	 * WWVH signal is present. The WWV and WWVH signals are also
	 * processed by 5-ms synchronous matched filters and combined to
	 * produce the second sync signal. The signals are scaled to
	 * produce unit energy at the maximum value.
	 *
	 * Note the master timing ramps, which run continuously. The
	 * minute counter (mphase) counts the samples in the minute,
	 * while the second counter (epoch) counts the samples in the
	 * second.
	 */
	up->mphase = (up->mphase + 1) % MINUTE;
	epoch = up->mphase % SECOND;

	/*
	 * WWV
	 */
	i = csinptr;
	csinptr = (csinptr + IN1000) % 80;

	dtemp = sintab[i] * syncx / (MS / 2.);
	ciamp -= cibuf[jptr];
	cibuf[jptr] = dtemp;
	ciamp += dtemp;
	csiamp -= csibuf[kptr];
	csibuf[kptr] = dtemp;
	csiamp += dtemp;

	i = (i + 20) % 80;
	dtemp = sintab[i] * syncx / (MS / 2.);
	cqamp -= cqbuf[jptr];
	cqbuf[jptr] = dtemp;
	cqamp += dtemp;
	csqamp -= csqbuf[kptr];
	csqbuf[kptr] = dtemp;
	csqamp += dtemp;

	sp = &up->mitig[up->achan].wwv;
	sp->amp = sqrt(ciamp * ciamp + cqamp * cqamp) / SYNCYC;
	if (!(up->status & MSYNC))
		wwv_qrz(peer, sp, (int)(pp->fudgetime1 * SECOND));

	/*
	 * WWVH
	 */
	i = hsinptr;
	hsinptr = (hsinptr + IN1200) % 80;

	dtemp = sintab[i] * syncx / (MS / 2.);
	hiamp -= hibuf[jptr];
	hibuf[jptr] = dtemp;
	hiamp += dtemp;
	hsiamp -= hsibuf[kptr];
	hsibuf[kptr] = dtemp;
	hsiamp += dtemp;

	i = (i + 20) % 80;
	dtemp = sintab[i] * syncx / (MS / 2.);
	hqamp -= hqbuf[jptr];
	hqbuf[jptr] = dtemp;
	hqamp += dtemp;
	hsqamp -= hsqbuf[kptr];
	hsqbuf[kptr] = dtemp;
	hsqamp += dtemp;

	rp = &up->mitig[up->achan].wwvh;
	rp->amp = sqrt(hiamp * hiamp + hqamp * hqamp) / SYNCYC;
	if (!(up->status & MSYNC))
		wwv_qrz(peer, rp, (int)(pp->fudgetime2 * SECOND));
	jptr = (jptr + 1) % SYNSIZ;
	kptr = (kptr + 1) % TCKSIZ;

	/*
	 * The following section is called once per minute. It does
	 * housekeeping and timeout functions and empties the dustbins.
	 */
	if (up->mphase == 0) {
		up->watch++;
		if (!(up->status & MSYNC)) {

			/*
			 * If minute sync has not been acquired before
			 * timeout, or if no signal is heard, the
			 * program cycles to the next frequency and
			 * tries again.
			 */
			if (!wwv_newchan(peer) || up->watch > ACQSN) {
#ifdef ICOM
				if (up->fd_icom > 0)
					up->dchan = (up->dchan + 1) %
					    NCHAN;
#endif /* ICOM */
				wwv_newgame(peer, up->dchan);
			}
		} else {

			/*
			 * If the leap bit is set, set the minute epoch
			 * back one second so the station processes
			 * don't miss a beat.
			 */
			if (up->status & LEPSEC) {
				up->mphase -= SECOND;
				if (up->mphase < 0)
					up->mphase += MINUTE;
			}
		}
	}

	/*
	 * When the channel metric reaches threshold and the second
	 * counter matches the minute epoch within the second, the
	 * driver has synchronized to the station. The second number is
	 * the remaining seconds until the next minute epoch, while the
	 * sync epoch is zero. Watch out for the first second; if
	 * already synchronized to the second, the buffered sync epoch
	 * must be set.
	 */
	if (up->status & MSYNC) {
		wwv_epoch(peer);
	} else if (up->sptr != NULL) {
		struct chan *cp;

		sp = up->sptr;
		if (sp->metric >= TTHR && epoch == sp->mepoch % SECOND)
		    {
			up->rsec = (60 - sp->mepoch / SECOND) % 60;
			up->rphase = 0;
			up->status |= MSYNC;
			up->watch = 0;
			if (!(up->status & SSYNC))
				up->repoch = up->yepoch = epoch;
			else
				up->repoch = up->yepoch;

			/*
			 * Clear the crud and initialize fairly.
			 */
			for (i = 0; i < NCHAN; i++) {
				cp = &up->mitig[i];
				cp->wwv.count = cp->wwv.reach = 0;
				cp->wwvh.count = cp->wwvh.reach = 0;
			}
			sp->count = sp->reach = 1;
		}
	}

	/*
	 * The second sync pulse is extracted using 5-ms (40 sample) FIR
	 * matched filters at 1000 Hz for WWV or 1200 Hz for WWVH. This
	 * pulse is used for the most precise synchronization, since if
	 * provides a resolution of one sample (125 us). The filters run
	 * only if the station has been reliably determined.
	 */
	if (up->status & SELV) {
		pdelay = (int)(pp->fudgetime1 * SECOND);
		mfsync = sqrt(csiamp * csiamp + csqamp * csqamp) /
		    TCKCYC;
	} else if (up->status & SELH) {
		pdelay = (int)(pp->fudgetime2 * SECOND);
		mfsync = sqrt(hsiamp * hsiamp + hsqamp * hsqamp) /
		    TCKCYC;
	} else {
		pdelay = 0;
		mfsync = 0;
	}

	/*
	 * Enhance the seconds sync pulse using a 1-s (8000-sample) comb
	 * filter. Correct for the FIR matched filter delay, which is 5
	 * ms for both the WWV and WWVH filters, and also for the
	 * propagation delay. Once each second look for second sync. If
	 * not in minute sync, fiddle the codec gain. Note the SNR is
	 * computed from the maximum sample and the envelope of the
	 * sample 10 ms before it, so if we slip more than a cycle the
	 * SNR should plummet. The signal is scaled to produce unit
	 * energy at the maximum value.
	 */
	dtemp = (epobuf[epoch] += (mfsync - epobuf[epoch]) /
	    up->avgint);
	if (dtemp > epomax) {
		epomax = dtemp;
		epopos = epoch;
	}
	if (epoch == 0) {
		int j;

		up->epomax = epomax;
		dtemp = 0;
		j = epopos - 10 * MS;
		if (j < 0)
			j += SECOND;
		up->eposnr = wwv_snr(epomax, epobuf[j]);
		epopos -= pdelay + TCKCYC * MS;
		if (epopos < 0)
			epopos += SECOND;
		wwv_endpoc(peer, epopos);
		if (!(up->status & SSYNC))
			up->alarm |= SYNERR;
		epomax = 0;
		if (!(up->status & MSYNC))
			wwv_gain(peer);
	}
}


/*
 * wwv_qrz - identify and acquire WWV/WWVH minute sync pulse
 *
 * This routine implements a virtual station process used to acquire
 * minute sync and to mitigate among the ten frequency and station
 * combinations. During minute sync acquisition the process probes each
 * frequency in turn for the minute pulse from either station, which
 * involves searching through the entire minute of samples. After
 * finding a candidate, the process searches only the seconds before and
 * after the candidate for the signal and all other seconds for the
 * noise.
 *
 * Students of radar receiver technology will discover this algorithm
 * amounts to a range gate discriminator. The discriminator requires
 * that the peak minute pulse amplitude be at least 2000 and the SNR be
 * at least 6 dB. In addition after finding a candidate, The peak second
 * pulse amplitude must be at least 2000, the SNR at least 6 dB and the
 * difference between the current and previous epoch must be less than
 * 7.5 ms, which corresponds to a frequency error of 125 PPM. A compare
 * counter keeps track of the number of successive intervals which
 * satisfy these criteria.
 *
 * Note that, while the minute pulse is found by by the discriminator,
 * the actual value is determined from the second epoch. The assumption
 * is that the discriminator peak occurs about 800 ms into the second,
 * so the timing is retarted to the previous second epoch.
 */
static void
wwv_qrz(
	struct peer *peer,	/* peer structure pointer */
	struct sync *sp,	/* sync channel structure */
	int	pdelay		/* propagation delay (samples) */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	char tbuf[80];		/* monitor buffer */
	long epoch, fpoch;
	int isgood;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Find the sample with peak energy, which defines the minute
	 * epoch. If a sample has been found with good energy,
	 * accumulate the noise energy for all except the second before
	 * and after that position.
	 *
	 * If the seconds sync lamp is lit, use that as the sample epoch
	 * within the second; otherwise, use the minutes sync peak.
	 * Little bit of class here.
	 */
	if (up->epomax > STHR && up->eposnr > SSNR) {
		fpoch = up->mphase % SECOND - up->tepoch;
		if (fpoch < 0)
			fpoch += SECOND;
	} else {
		fpoch = pdelay + SYNSIZ;
	}
	epoch = up->mphase - fpoch;
	if (epoch < 0)
		epoch += MINUTE;
	if (sp->amp > sp->maxeng) {
		sp->maxeng = sp->amp;
		sp->pos = epoch;
	}
	if (abs((epoch - sp->lastpos) % MINUTE) > SECOND)
		sp->noieng += sp->amp;

	/*
	 * At the end of the minute, determine the epoch of the
	 * sync pulse, as well as the SNR and difference between
	 * the current and previous epoch, which represents the
	 * intrinsic frequency error plus jitter.
	 */
	if (up->mphase == 0) {
		sp->synmax = sp->maxeng;
		sp->synsnr = wwv_snr(sp->synmax, sp->noieng / (MINUTE -
		    2. * SECOND));
		epoch = (sp->pos - sp->lastpos) % MINUTE;

		/*
		 * If not yet in minute sync, we have to do a little
		 * dance to find a valid minute sync pulse, emphasis
		 * valid.
		 */
		isgood = sp->synmax > ATHR && sp->synsnr > ASNR;
		switch (sp->count) {

		/*
		 * In state 0 the station was not heard during the
		 * previous probe. Look for the biggest blip greater
		 * than the amplitude threshold in the minute and assume
		 * that the minute sync pulse. We're fishing here, since
		 * the range gate has not yet been determined. If found,
		 * bump to state 1.
		 */
		case 0:
			if (sp->synmax >= ATHR)
				sp->count++;
			break;

		/*
		 * In state 1 a candidate blip has been found and the
		 * next minute has been searched for another blip. If
		 * none are found acceptable, drop back to state 0 and
		 * hunt some more. Otherwise, a legitimate minute pulse
		 * may have been found, so bump to state 2.
		 */
		case 1:
			if (!isgood) {
				sp->count = 0;
				break;
			}
			sp->count++;
			break;

		/*
		 * In states 2 and above, continue to groom samples as
		 * before and drop back to state 0 if the groom fails.
		 * If it succeeds, set the epoch and bump to the next
		 * state until reaching the threshold, if ever.
		 */
		default:
			if (!isgood || abs(epoch) > AWND * MS) {
				sp->count = 0;
				break;
			}
			sp->mepoch = sp->pos;
			sp->count++;
			break;
		}
		sp->metric = wwv_metric(sp);
		if (pp->sloppyclockflag & CLK_FLAG4) {
			sprintf(tbuf,
			    "wwv8 %d %3d %s %d %5.0f %5.1f %5.0f %5ld %5d %ld",
			    up->port, up->gain, sp->refid, sp->count,
			    sp->synmax, sp->synsnr, sp->metric, sp->pos,
			    up->tepoch, epoch);
			record_clock_stats(&peer->srcadr, tbuf);
#ifdef DEBUG
			if (debug)
				printf("%s\n", tbuf);
#endif
		}
		sp->lastpos = sp->pos;
		sp->maxeng = sp->noieng = 0;
	}
}


/*
 * wwv_endpoc - identify and acquire second sync pulse
 *
 * This routine is called at the end of the second sync interval. It
 * determines the second sync epoch position within the interval and
 * disciplines the sample clock using a frequency-lock loop (FLL).
 *
 * Second sync is determined in the RF input routine as the maximum
 * over all 8000 samples in the second comb filter. To assure accurate
 * and reliable time and frequency discipline, this routine performs a
 * great deal of heavy-handed heuristic data filtering and grooming.
 */
static void
wwv_endpoc(
	struct peer *peer,	/* peer structure pointer */
	int epopos		/* epoch max position */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	static int epoch_mf[3]; /* epoch median filter */
 	static int xepoch;	/* last second epoch */
 	static int zepoch;	/* last averaging interval epoch */
	static int syncnt;	/* run length counter */
	static int maxrun;	/* longest run length */
	static int mepoch;	/* longest run epoch */
	static int avgcnt;	/* averaging interval counter */
	static int avginc;	/* averaging ratchet */
	static int iniflg;	/* initialization flag */
	char tbuf[80];		/* monitor buffer */
	double dtemp;
	int tmp2;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;
	if (!iniflg) {
		iniflg = 1;
		memset((char *)epoch_mf, 0, sizeof(epoch_mf));
	}

	/*
	 * A three-stage median filter is used to help denoise the
	 * second sync pulse. The median sample becomes the candidate
	 * epoch.
	 */
	epoch_mf[2] = epoch_mf[1];
	epoch_mf[1] = epoch_mf[0];
	epoch_mf[0] = epopos;
	if (epoch_mf[0] > epoch_mf[1]) {
		if (epoch_mf[1] > epoch_mf[2])
			up->tepoch = epoch_mf[1];	/* 0 1 2 */
		else if (epoch_mf[2] > epoch_mf[0])
			up->tepoch = epoch_mf[0];	/* 2 0 1 */
		else
			up->tepoch = epoch_mf[2];	/* 0 2 1 */
	} else {
		if (epoch_mf[1] < epoch_mf[2])
			up->tepoch = epoch_mf[1];	/* 2 1 0 */
		else if (epoch_mf[2] < epoch_mf[0])
			up->tepoch = epoch_mf[0];	/* 1 0 2 */
		else
			up->tepoch = epoch_mf[2];	/* 1 2 0 */
	}

	/*
	 * If the signal amplitude or SNR fall below thresholds, dim the
	 * second sync lamp and start over. If no stations are heard we
	 * are either in a probe cycle or the ions are dim. In that case
	 * allow the amplitude and SNR to discharge and go no further. 
	 */
	if (up->epomax < STHR || up->eposnr < SSNR) {
		up->status &= ~(SSYNC | FGATE);
		avgcnt = syncnt = maxrun = 0;
		return;
	}
	if (!(up->status & (SELV | SELH)))
		return;

	avgcnt++;

	/*
	 * If the epoch candidate is the same as the last one, increment
	 * the compare counter. If not, save the length and epoch of the
	 * current run for use later and reset the counter.
	 */
	tmp2 = (up->tepoch - xepoch) % SECOND;
	if (tmp2 == 0) {
		syncnt++;
	} else {
		if (maxrun > 0 && mepoch == xepoch) {
			maxrun += syncnt;
		} else if (syncnt > maxrun) {
			maxrun = syncnt;
			mepoch = xepoch;
		}
		syncnt = 0;
	}
	if ((pp->sloppyclockflag & CLK_FLAG4) && !(up->status & (SSYNC |
	    MSYNC))) {
		sprintf(tbuf,
		    "wwv1 %04x %5.0f %5.1f %5d %5d %4d %4d",
		    up->status, up->epomax, up->eposnr, up->tepoch,
		    tmp2, avgcnt, syncnt);
		record_clock_stats(&peer->srcadr, tbuf);
#ifdef DEBUG
		if (debug)
			printf("%s\n", tbuf);
#endif /* DEBUG */
	}

	/*
	 * The sample clock frequency is disciplined using a first order
	 * feedback loop with time constant consistent with the Allan
	 * intercept of typical computer clocks.
	 *
	 * The frequency update is calculated from the epoch change in
	 * 125-us units divided by the averaging interval in seconds.
	 * The averaging interval affects other receiver functions,
	 * including the the 1000/1200-Hz comb filter and codec clock
	 * loop. It also affects the 100-Hz subcarrier loop and the bit
	 * and digit comparison counter thresholds.
	 */
	if (avgcnt < up->avgint) {
		xepoch = up->tepoch;
		return;
	}

	/*
	 * During the averaging interval the longest run of identical
	 * epoches is determined. If the longest run is at least 10
	 * seconds, the SSYNC bit is lit and the value becomes the
	 * reference epoch for the next interval. If not, the second
	 * sync lamp is dark and flashers set.
	 */
	if (maxrun > 0 && mepoch == xepoch) {
		maxrun += syncnt;
	} else if (syncnt > maxrun) {
		maxrun = syncnt;
		mepoch = xepoch;
	}
	xepoch = up->tepoch;
	if (maxrun > SCMP) {
		up->status |= SSYNC;
		up->yepoch = mepoch;
	} else {
		up->status &= ~SSYNC;
	}

	/*
	 * If the epoch change over the averaging interval is less than
	 * 1 ms, the frequency is adjusted, but clamped at +-125 PPM. If
	 * greater than 1 ms, the counter is decremented. If the epoch
	 * change is less than 0.5 ms, the counter is incremented. If
	 * the counter increments to +3, the averaging interval is
	 * doubled and the counter set to zero; if it decrements to -3,
	 * the interval is halved and the counter set to zero.
	 */
	if (maxrun == 0)
		mepoch = up->tepoch;
	dtemp = (mepoch - zepoch) % SECOND;
	if (up->status & FGATE) {
		if (abs(dtemp) < MAXFREQ * MINAVG) {
			up->freq += dtemp / (avgcnt * FCONST);
			if (up->freq > MAXFREQ)
				up->freq = MAXFREQ;
			else if (up->freq < -MAXFREQ)
				up->freq = -MAXFREQ;
			if (abs(dtemp) < MAXFREQ * MINAVG / 2.) {
				if (avginc < 3) {
					avginc++;
				} else {
					if (up->avgint < MAXAVG) {
						up->avgint <<= 1;
						avginc = 0;
					}
				}
			}
		} else {
			if (avginc > -3) {
				avginc--;
			} else {
				if (up->avgint > MINAVG) {
					up->avgint >>= 1;
					avginc = 0;
				}
			}
		}
	}
	if (pp->sloppyclockflag & CLK_FLAG4) {
		sprintf(tbuf,
		    "wwv2 %04x %4.0f %4d %4d %2d %4d %4.0f %7.2f",
		    up->status, up->epomax, mepoch, maxrun, avginc,
		    avgcnt, dtemp, up->freq * 1e6 / SECOND);
		record_clock_stats(&peer->srcadr, tbuf);
#ifdef DEBUG
		if (debug)
			printf("%s\n", tbuf);
#endif /* DEBUG */
	}
	up->status |= FGATE;
	zepoch = mepoch;
	avgcnt = syncnt = maxrun = 0;
}


/*
 * wwv_epoch - epoch scanner
 *
 * This routine extracts data signals from the 100-Hz subcarrier. It
 * scans the receiver second epoch to determine the signal amplitudes
 * and pulse timings. Receiver synchronization is determined by the
 * minute sync pulse detected in the wwv_rf() routine and the second
 * sync pulse detected in the wwv_epoch() routine. The transmitted
 * signals are delayed by the propagation delay, receiver delay and
 * filter delay of this program. Delay corrections are introduced
 * separately for WWV and WWVH. 
 *
 * Most communications radios use a highpass filter in the audio stages,
 * which can do nasty things to the subcarrier phase relative to the
 * sync pulses. Therefore, the data subcarrier reference phase is
 * disciplined using the hardlimited quadrature-phase signal sampled at
 * the same time as the in-phase signal. The phase tracking loop uses
 * phase adjustments of plus-minus one sample (125 us). Since this might
 * result in a phase slip of one cycle, the matched filter peak is the
 * maximum at the epoch and one cycle ahead or behind it. 
 */
static void
wwv_epoch(
	struct peer *peer	/* peer structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	struct chan *cp;
	static double sigmin, sigzer, sigone, engmax, engmin;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Sample the minute sync pulse energy at epoch 800 for both the
	 * WWV and WWVH stations. This will be used later for channel
	 * and station mitigation. Note that the seconds epoch is set
	 * here well before the end of the second to make sure we never
	 * set the epoch backwards.
	 */
	if (up->rphase == 800 * MS) {
		up->repoch = up->yepoch;
		cp = &up->mitig[up->achan];
		cp->wwv.syneng = cp->wwv.amp;
		cp->wwvh.syneng = cp->wwvh.amp;
	}

	/*
	 * Sample the I and Q data channels at epoch 200 ms. Use the
	 * signal energy as the peak to compute the SNR. Use the Q
	 * sample to adjust the 100-Hz reference oscillator phase.
	 */
	if (up->rphase == 200 * MS) {
		engmax = sqrt(up->irig * up->irig + up->qrig *
		    up->qrig);
		up->datpha = up->qrig / up->avgint;
		if (up->datpha >= 0) {
			up->datapt++;
			if (up->datapt >= 80)
				up->datapt -= 80;
		} else {
			up->datapt--;
			if (up->datapt < 0)
				up->datapt += 80;
		}
	}

	/*
	 * Use the signal amplitude at epoch 15 ms as the noise floor.
	 * This give a guard time of +-15 ms from the beginning of the
	 * second until the pulse rises at 30 ms. There is a compromise
	 * here; we want to delay the sample as long as possible to give
	 * the radio time to change frequency and the AGC to stabilize,
	 * but as early as possible if the second epoch is not exact.
	 */
	if (up->rphase == 15 * MS)
		sigmin = sigzer = sigone = up->irig;

	/*
	 * The zero bit is defined as the maximum data signal over the
	 * interval 190-210 ms. Keep this around until the end of the
	 * second.
	 */
	else if (up->rphase >= 190 * MS && up->rphase < 210 * MS &&
	    up->irig > sigzer) 
		sigzer = up->irig;

	/*
	 * The one bit is defined as the maximum data signal over the
	 * interval 490-510 ms. Keep this around until the end of the
	 * second.
	 */
	else if (up->rphase >= 490 * MS && up->rphase < 510 * MS &&
	    up->irig > sigone)
		sigone = up->irig;

	/*
	 * At the end of the second crank the clock state machine and
	 * adjust the codec gain. Note the epoch is buffered from the
	 * center of the second in order to avoid jitter while the
	 * seconds synch is diddling the epoch. Then, determine the true
	 * offset and update the median filter in the driver interface.
	 *
	 * Use the energy as the noise to compute the SNR for the pulse.
	 * This gives a better measurement than the beginning of the
	 * second, especially when returning from the probe channel.
	 * This gives a guard time of 30 ms from the decay of the
	 * longest pulse to the rise of the next pulse.
	 */
	up->rphase++;
	if (up->mphase % SECOND == up->repoch) {
		up->status &= ~(DGATE | BGATE);
		engmin = sqrt(up->irig * up->irig + up->qrig *
		    up->qrig);
		up->datsig = engmax;
		up->datsnr = wwv_snr(engmax, engmin);

		/*
		 * If the amplitude or SNR is below threshold, average a
		 * 0 in the the integrators; otherwise, average the
		 * bipolar signal. This is done to avoid noise polution.
		 */
		if (engmax < DTHR || up->datsnr < DSNR) {
			up->status |= DGATE;
			wwv_rsec(peer, 0);
		} else {
			sigzer -= sigone;
			sigone -= sigmin;
			wwv_rsec(peer, sigone - sigzer);
		}
		if (up->status & (DGATE | BGATE))
			up->errcnt++;
		if (up->errcnt > MAXERR)
			up->alarm |= LOWERR;
		wwv_gain(peer);
		up->rphase = 0;
	}
}


/*
 * wwv_rsec - process receiver second
 *
 * This routine is called at the end of each receiver second to
 * implement the per-second state machine. The machine assembles BCD
 * digit bits, decodes miscellaneous bits and dances the leap seconds.
 *
 * Normally, the minute has 60 seconds numbered 0-59. If the leap
 * warning bit is set, the last minute (1439) of 30 June (day 181 or 182
 * for leap years) or 31 December (day 365 or 366 for leap years) is
 * augmented by one second numbered 60. This is accomplished by
 * extending the minute interval by one second and teaching the state
 * machine to ignore it.
 */
static void
wwv_rsec(
	struct peer *peer,	/* peer structure pointer */
	double bit
	)
{
	static int iniflg;	/* initialization flag */
	static double bcddld[4]; /* BCD data bits */
	static double bitvec[61]; /* bit integrator for misc bits */
	struct refclockproc *pp;
	struct wwvunit *up;
	struct chan *cp;
	struct sync *sp, *rp;
	char	tbuf[80];	/* monitor buffer */
	int	sw, arg, nsec;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;
	if (!iniflg) {
		iniflg = 1;
		memset((char *)bitvec, 0, sizeof(bitvec));
	}

	/*
	 * The bit represents the probability of a hit on zero (negative
	 * values), a hit on one (positive values) or a miss (zero
	 * value). The likelihood vector is the exponential average of
	 * these probabilities. Only the bits of this vector
	 * corresponding to the miscellaneous bits of the timecode are
	 * used, but it's easier to do them all. After that, crank the
	 * seconds state machine.
	 */
	nsec = up->rsec;
	up->rsec++;
	bitvec[nsec] += (bit - bitvec[nsec]) / TCONST;
	sw = progx[nsec].sw;
	arg = progx[nsec].arg;

	/*
	 * The minute state machine. Fly off to a particular section as
	 * directed by the transition matrix and second number.
	 */
	switch (sw) {

	/*
	 * Ignore this second.
	 */
	case IDLE:			/* 9, 45-49 */
		break;

	/*
	 * Probe channel stuff
	 *
	 * The WWV/H format contains data pulses in second 59 (position
	 * identifier) and second 1, but not in second 0. The minute
	 * sync pulse is contained in second 0. second 0. At the end of
	 * second 58 QSY to the probe channel, which rotates in turn
	 * over all WWV/H frequencies. At the end of second 0 measure
	 * the minute sync pulse. At the end of second 1 measure the
	 * data pulse and QSY back to the data channel. Note that the
	 * seconds numbering here applies to the end of the second, so
	 * appears one second earlier than the clock displayed on the
	 * radio itself.
	 *
	 * At the end of second 0 save the minute sync pulse peak
	 * amplitude previously latched at 800 ms. 
	 */
	case SYNC2:			/* 0 */
		cp = &up->mitig[up->achan];
		cp->wwv.synmax = cp->wwv.syneng;
		cp->wwvh.synmax = cp->wwvh.syneng;
		break;

	/*
	 * At the end of second 1 measure the minute sync pulse
	 * minimum amplitude and calculate the SNR. If the minute sync
	 * pulse and SNR are above threshold and the data pulse
	 * amplitude and SNR are above thresold and, shift a 1 into the
	 * station reachability register; otherwise, shift a 0. The
	 * number of 1 bits in the last six intervals is a component of
	 * the channel metric computed by the mitigation routine.
	 * Finally, QSY back to the data channel.
	 */
	case SYNC3:			/* 1 */
		cp = &up->mitig[up->achan];

		/*
		 * WWV station
		 */
		sp = &cp->wwv;
		sp->synsnr = wwv_snr(sp->synmax, sp->syneng);
		sp->reach <<= 1;
		if (sp->reach & (1 << AMAX))
			sp->count--;
		if (sp->synmax >= QTHR && sp->synsnr >= QSNR &&
		    ~(up->status & (DGATE | BGATE))) {
			sp->reach |= 1;
			sp->count++;
		}
		sp->metric = wwv_metric(sp);

		/*
		 * WWVH station
		 */
		rp = &cp->wwvh;
		rp->synsnr = wwv_snr(rp->synmax, rp->syneng);
		rp->reach <<= 1;
		if (rp->reach & (1 << AMAX))
			rp->count--;
		if (rp->synmax >= QTHR && rp->synsnr >= QSNR &&
		    ~(up->status & (DGATE | BGATE))) {
			rp->reach |= 1;
			rp->count++;
		}
		rp->metric = wwv_metric(rp);
		if (pp->sloppyclockflag & CLK_FLAG4) {
			sprintf(tbuf,
			    "wwv5 %04x %3d %4d %.0f/%.1f %.0f/%.1f %s %04x %.0f %.0f/%.1f %s %04x %.0f %.0f/%.1f",
			    up->status, up->gain, up->yepoch,
			    up->epomax, up->eposnr, up->datsig,
			    up->datsnr,
			    sp->refid, sp->reach & 0xffff,
			    sp->metric, sp->synmax, sp->synsnr,
			    rp->refid, rp->reach & 0xffff,
			    rp->metric, rp->synmax, rp->synsnr);
			record_clock_stats(&peer->srcadr, tbuf);
#ifdef DEBUG
			if (debug)
				printf("%s\n", tbuf);
#endif /* DEBUG */
		}
		up->errcnt = up->digcnt = up->alarm = 0;

		/*
		 * We now begin the minute scan. Before first
		 * synchronizing to a station, reset and step to the
		 * next channel immediately if no station has been
		 * heard. Reset and step after the DATA timeout (4 min)
		 * if a station has been heard, but too few good data
		 * bits have been found. In any case, reset and step
		 * after the SYNCH timeout (30 min).
		 *
		 * After synchronizing to a station, reset and step
		 * after the PANIC timeout (2 days).
		 */
		if (!(up->status & INSYNC)) {
			if (!wwv_newchan(peer)) {
				wwv_newgame(peer, DCHAN);
				return;
			}
			if (up->watch > DATA && !(up->status & DSYNC)) {
				wwv_newgame(peer, DCHAN);
				return;
			}
			if (up->watch > SYNCH) {
				wwv_newgame(peer, DCHAN);
				return;
			}
		} else {
			wwv_newchan(peer);
			if (up->watch > PANIC) {
				wwv_newgame(peer, DCHAN);
				return;
			}
		}
#ifdef ICOM
		if (up->fd_icom > 0)
			wwv_qsy(peer, up->dchan);
#endif /* ICOM */
		break;

	/*
	 * Save the bit probability in the BCD data vector at the index
	 * given by the argument. Bits not used in the digit are forced
	 * to zero.
	 */
	case COEF1:			/* 4-7 */ 
		bcddld[arg] = bit;
		break;

	case COEF:			/* 10-13, 15-17, 20-23, 25-26,
					   30-33, 35-38, 40-41, 51-54 */
		if (up->status & DSYNC) 
			bcddld[arg] = bit;
		else
			bcddld[arg] = 0;
		break;

	case COEF2:			/* 18, 27-28, 42-43 */
		bcddld[arg] = 0;
		break;

	/*
	 * Correlate coefficient vector with each valid digit vector and
	 * save in decoding matrix. We step through the decoding matrix
	 * digits correlating each with the coefficients and saving the
	 * greatest and the next lower for later SNR calculation.
	 */
	case DECIM2:			/* 29 */
		wwv_corr4(peer, &up->decvec[arg], bcddld, bcd2);
		break;

	case DECIM3:			/* 44 */
		wwv_corr4(peer, &up->decvec[arg], bcddld, bcd3);
		break;

	case DECIM6:			/* 19 */
		wwv_corr4(peer, &up->decvec[arg], bcddld, bcd6);
		break;

	case DECIM9:			/* 8, 14, 24, 34, 39 */
		wwv_corr4(peer, &up->decvec[arg], bcddld, bcd9);
		break;

	/*
	 * Miscellaneous bits. If above the positive threshold, declare
	 * 1; if below the negative threshold, declare 0; otherwise
	 * raise the BGATE bit. The design is intended to avoid
	 * integrating noise under low SNR conditions.
	 */
	case MSC20:			/* 55 */
		wwv_corr4(peer, &up->decvec[YR + 1], bcddld, bcd9);
		/* fall through */

	case MSCBIT:			/* 2-3, 50, 56-57 */
		if (bitvec[nsec] > BTHR)
			up->misc |= arg;
		else if (bitvec[nsec] < -BTHR)
			up->misc &= ~arg;
		else
			up->status |= BGATE;
		break;

	/*
	 * Save the data channel gain, then QSY to the probe channel and
	 * dim the seconds comb filters. The newchan() routine will
	 * light them back up.
	 */
	case MSC21:			/* 58 */
		if (bitvec[nsec] > BTHR)
			up->misc |= arg;
		else if (bitvec[nsec] < -BTHR)
			up->misc &= ~arg;
		else
			up->status |= BGATE;
		up->status &= ~(SELV | SELH);
#ifdef ICOM
		if (up->fd_icom > 0) {
			up->schan = (up->schan + 1) % NCHAN;
			wwv_qsy(peer, up->schan);
		}
#endif /* ICOM */
		break;

	/*
	 * The endgames
	 *
	 * During second 59 the receiver and codec AGC are settling
	 * down, so the data pulse is unusable as quality metric. If
	 * LEPSEC is set on the last minute of 30 June or 31 December,
	 * the transmitter and receiver insert an extra second (60) in
	 * the timescale and the minute sync repeats the second. Once we
	 * tested this wrinkle at intervals of about 18 months, but
	 * strangely enough a leap has not been declared since the end
	 * of 1998.
	 */
	case MIN1:			/* 59 */
		if (up->status & LEPSEC)
			break;

		/* fall through */

	case MIN2:			/* 60 */
		up->status &= ~LEPSEC;
		wwv_tsec(peer);
		up->rsec = 0;
		wwv_clock(peer);
		break;
	}
	if ((pp->sloppyclockflag & CLK_FLAG4) && !(up->status &
	    DSYNC)) {
		sprintf(tbuf,
		    "wwv3 %2d %04x %3d %4d %5.0f %5.1f %5.0f %5.1f %5.0f",
		    nsec, up->status, up->gain, up->yepoch, up->epomax,
		    up->eposnr, up->datsig, up->datsnr, bit);
		record_clock_stats(&peer->srcadr, tbuf);
#ifdef DEBUG
		if (debug)
			printf("%s\n", tbuf);
#endif /* DEBUG */
	}
	pp->disp += AUDIO_PHI;
}

/*
 * If victory has been declared and seconds sync is lit, strike
 * a timestamp. It should not be a surprise, especially if the
 * radio is not tunable, that sometimes no stations are above
 * the noise and the reference ID set to NONE.
 */
static void
wwv_clock(
	struct peer *peer	/* peer unit pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	l_fp	offset;		/* offset in NTP seconds */

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;
	if (!(up->status & SSYNC))
		up->alarm |= SYNERR;
	if (up->digcnt < 9)
		up->alarm |= NINERR;
	if (!(up->alarm)) {
		up->status |= INSYNC;
		if (up->misc & SECWAR)
			pp->leap = LEAP_ADDSECOND;
		else
			pp->leap = LEAP_NOWARNING;
		pp->second = up->rsec;
		pp->minute = up->decvec[MN].digit + up->decvec[MN +
		    1].digit * 10;
		pp->hour = up->decvec[HR].digit + up->decvec[HR +
		    1].digit * 10;
		pp->day = up->decvec[DA].digit + up->decvec[DA +
		    1].digit * 10 + up->decvec[DA + 2].digit * 100;
		pp->year = up->decvec[YR].digit + up->decvec[YR +
		    1].digit * 10;
		pp->year += 2000;
		L_CLR(&offset);
		if (!clocktime(pp->day, pp->hour, pp->minute,
		    pp->second, GMT, up->timestamp.l_ui,
		    &pp->yearstart, &offset.l_ui)) {
			up->errflg = CEVNT_BADTIME;
		} else {
			up->watch = 0;
			pp->disp = 0;
			pp->lastref = up->timestamp;
			refclock_process_offset(pp, offset,
			    up->timestamp, PDELAY);
			refclock_receive(peer);
		}
	}
	pp->lencode = timecode(up, pp->a_lastcode);
	record_clock_stats(&peer->srcadr, pp->a_lastcode);
#ifdef DEBUG
	if (debug)
		printf("wwv: timecode %d %s\n", pp->lencode,
		    pp->a_lastcode);
#endif /* DEBUG */
}


/*
 * wwv_corr4 - determine maximum likelihood digit
 *
 * This routine correlates the received digit vector with the BCD
 * coefficient vectors corresponding to all valid digits at the given
 * position in the decoding matrix. The maximum value corresponds to the
 * maximum likelihood digit, while the ratio of this value to the next
 * lower value determines the likelihood function. Note that, if the
 * digit is invalid, the likelihood vector is averaged toward a miss.
 */
static void
wwv_corr4(
	struct peer *peer,	/* peer unit pointer */
	struct decvec *vp,	/* decoding table pointer */
	double	data[],		/* received data vector */
	double	tab[][4]	/* correlation vector array */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	double	topmax, nxtmax;	/* metrics */
	double	acc;		/* accumulator */
	char	tbuf[80];	/* monitor buffer */
	int	mldigit;	/* max likelihood digit */
	int	i, j;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Correlate digit vector with each BCD coefficient vector. If
	 * any BCD digit bit is bad, consider all bits a miss. Until the
	 * minute units digit has been resolved, don't to anything else.
	 */
	mldigit = 0;
	topmax = nxtmax = -MAXSIG;
	for (i = 0; tab[i][0] != 0; i++) {
		acc = 0;
		for (j = 0; j < 4; j++)
			acc += data[j] * tab[i][j];
		acc = (vp->like[i] += (acc - vp->like[i]) / TCONST);
		if (acc > topmax) {
			nxtmax = topmax;
			topmax = acc;
			mldigit = i;
		} else if (acc > nxtmax) {
			nxtmax = acc;
		}
	}
	vp->digprb = topmax;
	vp->digsnr = wwv_snr(topmax, nxtmax);

	/*
	 * The current maximum likelihood digit is compared to the last
	 * maximum likelihood digit. If different, the compare counter
	 * and maximum likelihood digit are reset. When the compare
	 * counter reaches the BCMP (5) threshold, the digit is assumed
	 * correct. When the compare counter of all nine digits have
	 * reached threshold, the clock is assumed correct.
	 *
	 * Note that the clock display digit is set before the compare
	 * counter has reached threshold; however, the clock display is
	 * not considered correct until all nine clock digits have
	 * reached threshold. This is intended as eye candy, but avoids
	 * mistakes when the signal is low and the SNR is very marginal.
	 */
	vp->mldigit = mldigit;
	if (vp->digprb < BTHR || vp->digsnr < BSNR) {
		vp->count = 0;
		up->status |= BGATE;
	} else {
		up->status |= DSYNC;
		if (vp->digit != mldigit) {
			vp->count = 0;
			up->alarm |= CMPERR;
			vp->digit = mldigit;
		} else {
			if (vp->count < BCMP)
				vp->count++;
			else
				up->digcnt++;
		}
	}
	if ((pp->sloppyclockflag & CLK_FLAG4) && !(up->status &
	    INSYNC)) {
		sprintf(tbuf,
		    "wwv4 %2d %04x %3d %4d %5.0f %2d %d %d %d %5.0f %5.1f",
		    up->rsec - 1, up->status, up->gain, up->yepoch,
		    up->epomax, vp->radix, vp->digit, vp->mldigit,
		    vp->count, vp->digprb, vp->digsnr);
		record_clock_stats(&peer->srcadr, tbuf);
#ifdef DEBUG
		if (debug)
			printf("%s\n", tbuf);
#endif /* DEBUG */
	}
}


/*
 * wwv_tsec - transmitter minute processing
 *
 * This routine is called at the end of the transmitter minute. It
 * implements a state machine that advances the logical clock subject to
 * the funny rules that govern the conventional clock and calendar.
 */
static void
wwv_tsec(
	struct peer *peer	/* driver structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	int minute, day, isleap;
	int temp;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Advance minute unit of the day. Don't propagate carries until
	 * the unit minute digit has been found.
	 */
	temp = carry(&up->decvec[MN]);	/* minute units */
	if (!(up->status & DSYNC))
		return;

	/*
	 * Propagate carries through the day.
	 */ 
	if (temp == 0)			/* carry minutes */
		temp = carry(&up->decvec[MN + 1]);
	if (temp == 0)			/* carry hours */
		temp = carry(&up->decvec[HR]);
	if (temp == 0)
		temp = carry(&up->decvec[HR + 1]);

	/*
	 * Decode the current minute and day. Set leap day if the
	 * timecode leap bit is set on 30 June or 31 December. Set leap
	 * minute if the last minute on leap day, but only if the clock
	 * is syncrhronized. This code fails in 2400 AD.
	 */
	minute = up->decvec[MN].digit + up->decvec[MN + 1].digit *
	    10 + up->decvec[HR].digit * 60 + up->decvec[HR +
	    1].digit * 600;
	day = up->decvec[DA].digit + up->decvec[DA + 1].digit * 10 +
	    up->decvec[DA + 2].digit * 100;

	/*
	 * Set the leap bit on the last minute of the leap day.
	 */
	isleap = up->decvec[YR].digit & 0x3;
	if (up->misc & SECWAR && up->status & INSYNC) {
		if ((day == (isleap ? 182 : 183) || day == (isleap ?
		    365 : 366)) && minute == 1439)
			up->status |= LEPSEC;
	}

	/*
	 * Roll the day if this the first minute and propagate carries
	 * through the year.
	 */
	if (minute != 1440)
		return;
	minute = 0;
	while (carry(&up->decvec[HR]) != 0); /* advance to minute 0 */
	while (carry(&up->decvec[HR + 1]) != 0);
	day++;
	temp = carry(&up->decvec[DA]);	/* carry days */
	if (temp == 0)
		temp = carry(&up->decvec[DA + 1]);
	if (temp == 0)
		temp = carry(&up->decvec[DA + 2]);

	/*
	 * Roll the year if this the first day and propagate carries
	 * through the century.
	 */
	if (day != (isleap ? 365 : 366))
		return;
	day = 1;
	while (carry(&up->decvec[DA]) != 1); /* advance to day 1 */
	while (carry(&up->decvec[DA + 1]) != 0);
	while (carry(&up->decvec[DA + 2]) != 0);
	temp = carry(&up->decvec[YR]);	/* carry years */
	if (temp == 0)
		carry(&up->decvec[YR + 1]);
}


/*
 * carry - process digit
 *
 * This routine rotates a likelihood vector one position and increments
 * the clock digit modulo the radix. It returns the new clock digit or
 * zero if a carry occurred. Once synchronized, the clock digit will
 * match the maximum likelihood digit corresponding to that position.
 */
static int
carry(
	struct decvec *dp	/* decoding table pointer */
	)
{
	int temp;
	int j;

	dp->digit++;
	if (dp->digit == dp->radix)
		dp->digit = 0;
	temp = dp->like[dp->radix - 1];
	for (j = dp->radix - 1; j > 0; j--)
		dp->like[j] = dp->like[j - 1];
	dp->like[0] = temp;
	return (dp->digit);
}


/*
 * wwv_snr - compute SNR or likelihood function
 */
static double
wwv_snr(
	double signal,		/* signal */
	double noise		/* noise */
	)
{
	double rval;

	/*
	 * This is a little tricky. Due to the way things are measured,
	 * either or both the signal or noise amplitude can be negative
	 * or zero. The intent is that, if the signal is negative or
	 * zero, the SNR must always be zero. This can happen with the
	 * subcarrier SNR before the phase has been aligned. On the
	 * other hand, in the likelihood function the "noise" is the
	 * next maximum down from the peak and this could be negative.
	 * However, in this case the SNR is truly stupendous, so we
	 * simply cap at MAXSNR dB.
	 */
	if (signal <= 0) {
		rval = 0;
	} else if (noise <= 0) {
		rval = MAXSNR;
	} else {
		rval = 20. * log10(signal / noise);
		if (rval > MAXSNR)
			rval = MAXSNR;
	}
	return (rval);
}


/*
 * wwv_newchan - change to new data channel
 *
 * The radio actually appears to have ten channels, one channel for each
 * of five frequencies and each of two stations (WWV and WWVH), although
 * if not tunable only the 15 MHz channels appear live. While the radio
 * is tuned to the working data channel frequency and station for most
 * of the minute, during seconds 59, 0 and 1 the radio is tuned to a
 * probe frequency in order to search for minute sync pulse and data
 * subcarrier from other transmitters.
 *
 * The search for WWV and WWVH operates simultaneously, with WWV minute
 * sync pulse at 1000 Hz and WWVH at 1200 Hz. The probe frequency
 * rotates each minute over 2.5, 5, 10, 15 and 20 MHz in order and yes,
 * we all know WWVH is dark on 20 MHz, but few remember when WWV was lit
 * on 25 MHz.
 *
 * This routine selects the best channel using a metric computed from
 * the reachability register and minute pulse amplitude. Normally, the
 * award goes to the the channel with the highest metric; but, in case
 * of ties, the award goes to the channel with the highest minute sync
 * pulse amplitude and then to the highest frequency.
 *
 * The routine performs an important squelch function to keep dirty data
 * from polluting the integrators. During acquisition and until the
 * clock is synchronized, the signal metric must be at least MTR (13);
 * after that the metrict must be at least TTHR (50). If either of these
 * is not true, the station select bits are cleared so the second sync
 * is disabled and the data bit integrators averaged to a miss. 
 */
static int
wwv_newchan(
	struct peer *peer	/* peer structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	struct sync *sp, *rp;
	double rank, dtemp;
	int i, j;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Search all five station pairs looking for the channel with
	 * maximum metric. If no station is found above thresholds, tune
	 * to WWV on 15 MHz, set the reference ID to NONE and wait for
	 * hotter ions.
	 */
	sp = NULL;
	j = 0;
	rank = 0;
	for (i = 0; i < NCHAN; i++) {
		rp = &up->mitig[i].wwvh;
		dtemp = rp->metric;
		if (dtemp >= rank) {
			rank = dtemp;
			sp = rp;
			j = i;
		}
		rp = &up->mitig[i].wwv;
		dtemp = rp->metric;
		if (dtemp >= rank) {
			rank = dtemp;
			sp = rp;
			j = i;
		}
	}

	/*
	 * If the strongest signal is less than threshold, we are
	 * beneath the waves. if the first second in the minute has been
	 * found, tune to 15 MHz and wait for sunrise. If strongest
	 * signal is greater than threshold, tune to that frequency and
	 * the transmitter.
	 */
	if (rank < MTHR) {
		if (up->status & MSYNC) {
			up->dchan = DCHAN;
			sp = &up->mitig[DCHAN].wwv;
			up->sptr = sp ;
			memcpy(&pp->refid, "NONE", 4);
			up->status |= sp->select & (SELV | SELH);
		} else {
			memcpy(&pp->refid, "SCAN", 4);
			up->status &= ~(SELV | SELH);
		}
	} else {
		up->dchan = j;
		up->sptr = sp;
		memcpy(&pp->refid, sp->refid, 4);
		up->status |= sp->select & (SELV | SELH);
	}
	peer->refid = pp->refid;
	return (up->status & (SELV | SELH));
}


/*
 * wwv_newgame - reset and start over
 *
 * There are four conditions resulting in a new game:
 *
 * 1	During initial acquisition (MSYNC dark) going 5 minutes (ACQSN)
 *	without reliably finding the minute pulse (MSYNC lit).
 *
 * 2	After finding the minute pulse (MSYNC lit), going 5 minutes
 *	(DATA) without finding the unit seconds digit.
 *
 * 3	After finding good data (DATA lit), going more than 30 minutes
 *	(SYNCH) without finding station sync (INSYNC lit).
 *
 * 4	After finding station sync (INSYNC lit), going more than two
 *	days (PANIC) without finding station sync again. 
 */
static void
wwv_newgame(
	struct peer *peer,	/* peer structure pointer */
	int	chan		/* start channel scan */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;
	struct chan *cp;
	int i;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Initialize strategic values. Note we set the leap bits
	 * NOTINSYNC and the refid "NONE".
	 */
	peer->leap = LEAP_NOTINSYNC;
	up->watch = up->status = up->alarm = 0;
	up->avgint = MINAVG;
	up->freq = 0;
	up->gain = MAXGAIN / 2;

	/*
	 * Initialize the station processes for audio gain, select bit,
	 * station/frequency identifier and reference identifier. Start
	 * probing at the next channel after the data channel.
	 */
	memset(up->mitig, 0, sizeof(up->mitig));
	for (i = 0; i < NCHAN; i++) {
		cp = &up->mitig[i];
		cp->gain = up->gain;
		cp->wwv.select = SELV;
		sprintf(cp->wwv.refid, "WV%.0f", floor(qsy[i])); 
		cp->wwvh.select = SELH;
		sprintf(cp->wwvh.refid, "WH%.0f", floor(qsy[i])); 
	}
	wwv_newchan(peer);
	up->dchan = up->achan = up->schan = chan;
#ifdef ICOM
	if (up->fd_icom > 0)
		wwv_qsy(peer, chan);
#endif /* ICOM */
}

/*
 * wwv_metric - compute station metric
 *
 * The most significant bits represent the number of ones in the
 * station reachability register. The least significant bits represent
 * the minute sync pulse amplitude. The combined value is scaled 0-100.
 */
double
wwv_metric(
	struct sync *sp		/* station pointer */
	)
{
	double	dtemp;

	dtemp = sp->count * MAXSIG;
	if (sp->synmax < MAXSIG)
		dtemp += sp->synmax;
	else
		dtemp += MAXSIG - 1;
	dtemp /= (AMAX + 1) * MAXSIG;
	return (dtemp * 100.);
}


#ifdef ICOM
/*
 * wwv_qsy - Tune ICOM receiver
 *
 * This routine saves the AGC for the current channel, switches to a new
 * channel and restores the AGC for that channel. If a tunable receiver
 * is not available, just fake it.
 */
static int
wwv_qsy(
	struct peer *peer,	/* peer structure pointer */
	int	chan		/* channel */
	)
{
	int rval = 0;
	struct refclockproc *pp;
	struct wwvunit *up;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;
	if (up->fd_icom > 0) {
		up->mitig[up->achan].gain = up->gain;
		rval = icom_freq(up->fd_icom, peer->ttl & 0x7f,
		    qsy[chan]);
		up->achan = chan;
		up->gain = up->mitig[up->achan].gain;
	}
	return (rval);
}
#endif /* ICOM */


/*
 * timecode - assemble timecode string and length
 *
 * Prettytime format - similar to Spectracom
 *
 * sq yy ddd hh:mm:ss ld dut lset agc iden sig errs freq avgt
 *
 * s	sync indicator ('?' or ' ')
 * q	error bits (hex 0-F)
 * yyyy	year of century
 * ddd	day of year
 * hh	hour of day
 * mm	minute of hour
 * ss	second of minute)
 * l	leap second warning (' ' or 'L')
 * d	DST state ('S', 'D', 'I', or 'O')
 * dut	DUT sign and magnitude (0.1 s)
 * lset	minutes since last clock update
 * agc	audio gain (0-255)
 * iden	reference identifier (station and frequency)
 * sig	signal quality (0-100)
 * errs	bit errors in last minute
 * freq	frequency offset (PPM)
 * avgt	averaging time (s)
 */
static int
timecode(
	struct wwvunit *up,	/* driver structure pointer */
	char *ptr		/* target string */
	)
{
	struct sync *sp;
	int year, day, hour, minute, second, dut;
	char synchar, leapchar, dst;
	char cptr[50];
	

	/*
	 * Common fixed-format fields
	 */
	synchar = (up->status & INSYNC) ? ' ' : '?';
	year = up->decvec[YR].digit + up->decvec[YR + 1].digit * 10 +
	    2000;
	day = up->decvec[DA].digit + up->decvec[DA + 1].digit * 10 +
	    up->decvec[DA + 2].digit * 100;
	hour = up->decvec[HR].digit + up->decvec[HR + 1].digit * 10;
	minute = up->decvec[MN].digit + up->decvec[MN + 1].digit * 10;
	second = 0;
	leapchar = (up->misc & SECWAR) ? 'L' : ' ';
	dst = dstcod[(up->misc >> 4) & 0x3];
	dut = up->misc & 0x7;
	if (!(up->misc & DUTS))
		dut = -dut;
	sprintf(ptr, "%c%1X", synchar, up->alarm);
	sprintf(cptr, " %4d %03d %02d:%02d:%02d %c%c %+d",
	    year, day, hour, minute, second, leapchar, dst, dut);
	strcat(ptr, cptr);

	/*
	 * Specific variable-format fields
	 */
	sp = up->sptr;
	sprintf(cptr, " %d %d %s %.0f %d %.1f %d", up->watch,
	    up->mitig[up->dchan].gain, sp->refid, sp->metric,
	    up->errcnt, up->freq / SECOND * 1e6, up->avgint);
	strcat(ptr, cptr);
	return (strlen(ptr));
}


/*
 * wwv_gain - adjust codec gain
 *
 * This routine is called at the end of each second. It counts the
 * number of signal clips above the MAXSIG threshold during the previous
 * second. If there are no clips, the gain is bumped up; if too many
 * clips, it is bumped down. The decoder is relatively insensitive to
 * amplitude, so this crudity works just fine. The input port is set and
 * the error flag is cleared, mostly to be ornery.
 */
static void
wwv_gain(
	struct peer *peer	/* peer structure pointer */
	)
{
	struct refclockproc *pp;
	struct wwvunit *up;

	pp = peer->procptr;
	up = (struct wwvunit *)pp->unitptr;

	/*
	 * Apparently, the codec uses only the high order bits of the
	 * gain control field. Thus, it may take awhile for changes to
	 * wiggle the hardware bits.
	 */
	if (up->clipcnt == 0) {
		up->gain += 4;
		if (up->gain > MAXGAIN)
			up->gain = MAXGAIN;
	} else if (up->clipcnt > MAXCLP) {
		up->gain -= 4;
		if (up->gain < 0)
			up->gain = 0;
	}
	audio_gain(up->gain, up->mongain, up->port);
	up->clipcnt = 0;
#if DEBUG
	if (debug > 1)
		audio_show();
#endif
}


#else
int refclock_wwv_bs;
#endif /* REFCLOCK */
