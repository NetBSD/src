/* Measure the difference between our clock and some other,
 *	using ICMP timestamp messages and the time service,
 *	or using serial I/O, and adjust our clock with the result.
 *
 *	We can talk to either a typical TCP machine, or to a Precision
 *	Time Standard WWV receiver.
 *
 * Copyright 1991, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */


#ident "$Revision: 1.1 $"

#include <stdio.h>
#include <math.h>
#include <values.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include <sys/schedctl.h>
#include <sys/syssgi.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <bstring.h>
#include <sys/clock.h>

extern int getopt();
extern char *optarg;
extern int optind;

#define min(a,b)	((a)>(b) ? (b) : (a))


#define MSECDAY (SECDAY*1000)		/* msec/day */

#define BU ((unsigned long)2208988800)	/* seconds before UNIX epoch */

#define	USEC_PER_SEC	1000000
#define F_USEC_PER_SEC	(USEC_PER_SEC*1.0)	/* reduce typos */
#define F_NSEC_PER_SEC	(F_USEC_PER_SEC*1000.0)


#define	MAXADJ 20			/* max adjtime(2) correction in sec */

#define MIN_ROUND (1.0/HZ)		/* best expected ICMP round usec */
double avg_round = SECDAY;		/* minimum round trip */
int clr_round = 0;

#define MAX_NET_DELAY 20		/* max one-way packet delay (sec) */
#define MAX_MAX_ICMP_TRIALS 200		/* try this hard to read the clock */
int icmp_max_sends;
int icmp_min_diffs;			/* require this many responses */
int icmp_wait = 1;			/* this many seconds/ICMP try */

#define MAX_TIME_TRIALS 3
int date_wait = 4;			/* wait for date (sec) */

#define VDAMP 32			/* damping for error variation */
#define DISCARD_MEAS 4			/* discard this many wild deltas */
#define MAX_CDAMP 32			/* damping for changes */

#define INIT_PASSES 11			/* this many initial, quick fixes */

#define DRIFT_DAMP 4			/* damp drift for these hours */
#define DRIFT_TICKS (DRIFT_DAMP*SECHR*HZ*1.0)
double drift_err = 0.0;			/* sec/tick of adjustments */
double drift_ticks = SECHR*HZ*1.0;	/* total HZ of good measurements
					 *  preload to avoid division by 0
					 *  and for initial stability
					 */
#define MAX_DRIFT 3000000.0		/* max drift in nsec/sec, 0.3% */
#define MAX_TRIM MAX_DRIFT		/* max trim in nsec/sec */
double max_drift;			/* max clock drift, sec/tick */
double neg_max_drift;

int chg_smooth;				/* spread changes out this far */

#define USEC_PER_C (USEC_PER_SEC/960)	/* usec/char for the WWV clock */

clock_t rep_rate;			/* check this often (in HZ) */
#define DEF_REP_RATE (210*HZ)
#define INIT_REP_RATE (90*HZ)		/* run fast at first */

struct sockaddr_in masaddr;		/* master's address */
char *masname;				/* master's name */

unsigned char debug = 0;

unsigned char background = 1;		/* fork into the background */

unsigned char quiet = 0;		/* do not complain about dead server */

unsigned char measure_only;		/* <>0 to only measure, not adjust */

int mtype = -1;				/* 0=have netmaster, 1=have clock */

char *pgmname;				/* our name */

struct wwv_qa {				/* bytes from the WWV receiver */
    u_char ok;				/*	for a 'QA' command */
    u_char pm;
    u_char ms_hi;
    u_char ms_lo;
    u_char ss;
    u_char mm;
    u_char hh;
    u_char dd_hi;
    u_char dd_lo;
    u_char yy;
    u_char resvd[2];
    u_char cr;
};
#define WWV_QA_SIZE 13			/* sizeof() gives the wrong answer */


#define USAGE "%s: Usage: timeslave [-md[d]Bq] [-T max-trials] [-t min-trials]\n    [-w ICMP-wait] [-W date-wait] [-r rate] [-p port] [-D max-drift]\n    [-P parm-file] -H net-master|-C clock-device\n"
#define PROTO "udp"
#define PORT "time"

char *timetrim_fn;
FILE *timetrim_st;
char *timetrim_wpat = "long timetrim = %ld;\ndouble tot_adj = %.0f;\ndouble tot_ticks = %.0f;\n/* timeslave version 2 */\n";
char *timetrim_rpat = "long timetrim = %ld;\ndouble tot_adj = %lf;\ndouble tot_ticks = %lf;";
long timetrim;
double tot_adj, hr_adj;			/* totals in nsec */
double tot_ticks, hr_ticks;

double experr = 0.0;			/* expected error */
double varerr = 1.0;			/* variation in error in sec/tick */
double skip_varerr;
double chgs[MAX_CDAMP];			/* committed changes */
unsigned int cur_chg = 0;
unsigned int skip = 0;
unsigned int pass = 0;
double prev_ticks = 0;
double prev_drift_ticks = 0;
double prev_drift;
int headings;


/* change debugging, probably for a signal
 */
static void
moredebug()
{
	if (!debug)
		headings = 0;
	debug++;
	syslog(LOG_DEBUG, "debugging increased to %d", debug);
}

static void
lessdebug()
{
	if (debug > 0)
		debug--;
	syslog(LOG_DEBUG, "debugging decreased to %d", debug);
}


/* 'fix' a time value
 */
static void
fix_time(struct timeval *tp)
{
    for (;;) {
	if (tp->tv_usec > USEC_PER_SEC) {
	    tp->tv_sec++;
	    tp->tv_usec -= USEC_PER_SEC;
	    continue;
	}
	if (tp->tv_usec < -USEC_PER_SEC
	    || (tp->tv_usec < 0 && tp->tv_sec > 0)) {
	    tp->tv_sec--;
	    tp->tv_usec += USEC_PER_SEC;
	    continue;
	}
	return;
    }
}



/* Checksum routine for Internet Protocol family headers
 */
static int
in_cksum(u_short *addr, int len)
{
    u_short *w = addr;
    u_short answer;
    int sum = 0;

    while( len > 1 )  {
	sum += *w++;
	len -= 2;
    }

    /* mop up an odd byte, if necessary */
    if( len == 1 )
	sum += (*(u_char *)w) << 8;

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
    sum += (sum >> 16);			/* add carry */
    answer = ~sum;			/* truncate to 16 bits */
    return (answer);
}


/* clear the input queue of a socket */
int					/* <0 on error */
clear_soc(int soc)			/* this socket */
{
    int i;
    struct timeval tout;
    fd_set ready;
    char buf[1024];

    for (;;) {
	FD_ZERO(&ready);
	FD_SET(soc, &ready);
	tout.tv_sec = tout.tv_usec = 0;
	i = select(soc+1, &ready, (fd_set *)0, (fd_set *)0, &tout);
	if (i < 0) {
	    if (errno == EINTR)
		continue;
	    syslog(LOG_ERR, "select(ICMP clear): %m");
	    return -1;
	}
	if (i == 0)
	    return 0;

	i = recv(soc, &buf[0], sizeof(buf), 0);
	if (i < 0) {
	    syslog(LOG_ERR, "recv(clear_soc): %m");
	    return -1;
	}
    }
}


#ifdef NOTDEF
double
float_time(struct timeval *tp)
{
    return (double)tp->tv_sec + tp->tv_usec/F_USEC_PER_SEC;
}
#else
#define float_time(tp) ((double)(tp)->tv_sec + (tp)->tv_usec/F_USEC_PER_SEC)
#endif


static void
set_tval(struct timeval *tp,		/* create time structure */
	 double f_sec)			/* from this number of seconds */
{
    f_sec = rint(f_sec * USEC_PER_SEC);

    tp->tv_sec = f_sec/USEC_PER_SEC;
    tp->tv_usec = f_sec - (tp->tv_sec * F_USEC_PER_SEC);
}


/* compute the current drift */
double					/* predicted seconds of drift */
drift(double delta)			/* for this many ticks */
{
    double ndrift;

    if (drift_ticks >= DRIFT_TICKS) {	/* age the old drift value */
	    drift_ticks -= drift_ticks/16;
	    drift_err -= drift_err/16;
    }

    ndrift = drift_err/drift_ticks;
    if (ndrift < neg_max_drift)		/* do not go crazy */
	ndrift = neg_max_drift;
    else if (ndrift > max_drift)
	ndrift = max_drift;

    return ndrift*delta;
}


/* guess at a measurement */
void
guess_adj(struct timeval *adjp)		/* put adjustment here */
{
    struct tms tm;
    double cur_ticks;
    double cur_drift;
    double tmp;

    cur_ticks = times(&tm);
    cur_drift = drift(cur_ticks-prev_drift_ticks);
    prev_drift_ticks = cur_ticks;

    prev_drift += cur_drift;
    tmp = chgs[cur_chg];
    experr -= tmp;
    tmp += cur_drift;
    set_tval(adjp, tmp);

    if (debug != 0)
	syslog(LOG_DEBUG, "guess %.3f%+.3f=%.3f",
	       cur_drift*1000.0, chgs[cur_chg]*1000.0,
	       float_time(adjp)*1000.0);

    chgs[cur_chg] = 0;
    cur_chg = (cur_chg+1) % MAX_CDAMP;
    skip++;
}


long min_idelta, min_odelta, min_rdelta;    /* preferred differences */
long diffs, tot_idelta, tot_odelta, tot_rdelta;	/* for averages */

/* accumulate a difference
 */
static double				/* return sec of difference */
diffaccum(long rcvtime,			/* when it got back here */
	  u_long ttime,			/* received by other guy */
	  u_long rtime,			/* sent by the other guy */
	  u_long otime,			/* when it was sent by us */
	  u_short seq)			/* packet sequence # */
{
    long idelta, odelta, rdelta;
    double diff;

    rdelta = rcvtime-otime;		/* total round trip */
    idelta = rcvtime-ttime;		/* apparent return time */
    odelta = rtime-otime;		/* apparent tranmission time */

    /* do not be confused by midnight */
    if (idelta < -MSECDAY/2) idelta += MSECDAY;
    else if (idelta > MSECDAY/2) idelta -= MSECDAY;

    if (odelta < -MSECDAY/2) odelta += MSECDAY;
    else if (odelta > MSECDAY/2) odelta -= MSECDAY;

    if (rdelta > MSECDAY/2) rdelta -= MSECDAY;

    tot_idelta += idelta;
    tot_odelta += odelta;
    tot_rdelta += rdelta;

    diff = (odelta - idelta)/2000.0;

    if (idelta < min_idelta)
	min_idelta = idelta;
    if (odelta < min_odelta)
	min_odelta = odelta;
    if (rdelta < min_rdelta)
	min_rdelta = rdelta;

    /* Compute real the round trip delay to the server.
     * Shrink the computed value quickly.
     */
    if (rdelta < avg_round*(.75*1000.0)) {
	avg_round = rdelta/1000.0;
	clr_round = 0;
    }
    if (rdelta < avg_round*(2*1000.0)) {
	avg_round = (rdelta/1000.0 + avg_round*7)/8;
    }

    if (debug > 2)
	syslog(LOG_DEBUG, "#%d rt%-+5d out%-+5d back%-+5d  %+.1f",
	       seq, rdelta, odelta, idelta, diff*1000.0);

    return ((tot_odelta-tot_idelta)/2000.0)/diffs;
}



/* Measure the difference between our clock and some other machine,
 *	using ICMP timestamp messages.
 */
int					/* <0 on trouble */
secdiff(int rawsoc,
	double *diffp)			/* signed sec difference */
{
    static int icmp_logged = 0;		/* <>0 if master complaint logged */
    static short seqno = 1;

    fd_set ready;
    int i;
    int sends, waits;
    double diff, rt_excess, excess_err;
    short seqno0;
    struct timeval sdelay, prevtv, tv;
    struct sockaddr_in from;
    int fromlen;
    struct icmp opkt;
    time_t recvtime;
    union {
	char b[1024];
	struct ip ip;
    } ipkt;
    struct icmp *ipp;


    opkt.icmp_type = ICMP_TSTAMP;	/* make an ICMP packet to send */
    opkt.icmp_code = 0;
    opkt.icmp_id = getpid();
    opkt.icmp_rtime = 0;
    opkt.icmp_ttime = 0;
    if (seqno > 16000)			/* keep the sequence number */
	seqno = 1;			/*	from wrapping */
    seqno0 = seqno-1;

    *diffp = 0.0;

    if (clear_soc(rawsoc) < 0)		/* empty the input queue */
	return -1;

    sginap(1);				/* start at clock tick */

    min_idelta = MSECDAY;
    min_odelta = MSECDAY;
    min_rdelta = MSECDAY;
    tot_idelta = 0;
    tot_odelta = 0;
    tot_rdelta = 0;
    diffs = 0;
    sends = 0;
    while (sends < icmp_max_sends && diffs < icmp_min_diffs) {

	(void)gettimeofday(&tv, 0);

	if (sends < icmp_max_sends) {	/* limit total ICMPs sent */
	    sends++;
	    opkt.icmp_cksum = 0;
	    opkt.icmp_seq = seqno++;
	    opkt.icmp_otime = htonl((tv.tv_sec % SECDAY)*1000
				    + (tv.tv_usec+500)/1000);
	    opkt.icmp_cksum = in_cksum((u_short *)&opkt, sizeof(opkt));
	    i = send(rawsoc, (char*)&opkt, sizeof(opkt), 0);
	    if (i != sizeof(opkt)) {
		if (!icmp_logged) {
		    icmp_logged = 1;
		    if (i < 0)
			syslog(LOG_ERR, "send(ICMP): %m");
		    else
			syslog(LOG_ERR, "send(ICMP): result=%d", i);
		}
		return -1;
	    }
	}

	/* get ICMP packets until we get the right one */
	prevtv = tv;
	for (;;) {
	    sdelay.tv_sec = icmp_wait + prevtv.tv_sec - tv.tv_sec;
	    sdelay.tv_usec = prevtv.tv_usec - tv.tv_usec;
	    fix_time(&sdelay);
	    if (sdelay.tv_sec < 0) {
		sdelay.tv_usec = 0;
		sdelay.tv_sec = 0;
	    }
	    ipp = 0;
	    FD_ZERO(&ready);
	    FD_SET(rawsoc, &ready);
	    i = select(rawsoc+1, &ready, (fd_set *)0, (fd_set *)0, &sdelay);
	    (void)gettimeofday(&tv, 0);
	    if (i < 0) {
		if (errno == EINTR)
		    continue;
		syslog(LOG_ERR, "select(ICMP): %m");
		icmp_logged = 1;
		return -1;
	    }
	    if (i == 0)			/* send after waiting a while */
		break;

	    fromlen = sizeof(from);
	    if (0 > recvfrom(rawsoc,(char*)&ipkt,sizeof(ipkt),
			     0, &from,&fromlen)) {
		syslog(LOG_ERR, "recvfrom(ICMP): %m");
		icmp_logged = 1;
		return -1;
	    }

	    /* accept replies to any of the packets we have sent */
	    ipp = (struct icmp *)&ipkt.b[ipkt.ip.ip_hl<<2];
	    if ((ipp->icmp_type != ICMP_TSTAMPREPLY)
		|| ipp->icmp_id != opkt.icmp_id
		|| ipp->icmp_seq > seqno
		|| ipp->icmp_seq < seqno0)
		continue;

	    recvtime = ((tv.tv_sec % SECDAY)*1000
			+ (tv.tv_usec+500)/1000);

	    ipp->icmp_otime = ntohl(ipp->icmp_otime);
	    ipp->icmp_rtime = ntohl(ipp->icmp_rtime);
	    ipp->icmp_ttime = ntohl(ipp->icmp_ttime);
	    if (recvtime < ipp->icmp_otime)
		continue;		/* do not worry about midnight */

	    diffs++;
	    diff = diffaccum(recvtime, ipp->icmp_ttime,
			     ipp->icmp_rtime, ipp->icmp_otime,
			     ipp->icmp_seq - seqno0);

	    /* exchange timestamps as fast as the other side is willing.
	     */
	    if (ipp->icmp_seq == seqno-1)
		break;
	}
    }
    if (diffs < icmp_min_diffs) {
	if (!icmp_logged && (!quiet || debug > 0)) {
	    icmp_logged = 1;
	    syslog(LOG_ERR, "%d responses in %d trials from %s",
		   diffs, sends, masname);
	}
	return -1;
    }

    /* if the round trip time for the measurement is more than 25% high,
     * and if the error is unexpectedly large by a similar amount,
     * then assume the path delay has become asymmetric because of
     * asymmetric loads, and adjust the computed difference by the inferred
     * asymmetry.
     *
     * The asymmetry biases the measurement by half of its value.
     *
     * If the current estimate of the minmum round trip time is persistently
     * exceeded, assume the route has changed, and start looking for a new
     * round trip time.
     */
    rt_excess = (tot_rdelta/1000.0)/diffs - avg_round;
    excess_err = diff - experr;
    if (rt_excess <= avg_round/4+MIN_ROUND) {
	clr_round = 0;

    } else if (abs(excess_err) < rt_excess/2) {
	clr_round = 0;
	if (excess_err > 0)
	    diff -= rt_excess/2;
	else
	    diff += rt_excess/2;
	if (debug > 0)
	    syslog(LOG_DEBUG, "trim asymmetry of %+.1f to get %+.1f",
		   rt_excess*1000.0/2, diff*1000.0);

    } else if (++clr_round > 5) {
	avg_round = SECDAY;
	clr_round = 0;
	if (debug > 0)
	    syslog(LOG_DEBUG, "resetting minimum round trip measure");
    }


    if (debug > 1)
	syslog(LOG_DEBUG, "error=%+.1f  %3d values  %2d trials",
	       diff*1000.0, diffs, sends);

    if (0 != icmp_logged) {		/* after a successful measurement, */
	icmp_logged = 0;		/*	complain again */
	syslog(LOG_ERR, "Time measurements from %s are working again.",
	       masname);
    }
    *diffp = diff;
    return 0;
}



/* compute the difference between our date and the masters.
 */
double					/* return error in seconds */
daydiff(int udpsoc)
{
    static int udp_logged = 0;
    int i;
    int trials;
    struct timeval tout, now;
    fd_set ready;
    struct sockaddr_in from;
    int fromlen;
    double sec;
    unsigned long buf;

    if (clear_soc(udpsoc) < 0)		/* get rid of drek */
	return 0.0;

    tout.tv_sec = date_wait;
    tout.tv_usec = 0;
    for (trials = 0; trials < MAX_TIME_TRIALS; trials++) {
	/* ask for the time */
	buf = 0;
	if (send(udpsoc, (char *)&buf, sizeof(buf), 0) < 0) {
	    if (!udp_logged) {
		udp_logged = 1;
		syslog(LOG_ERR, "send(udpsoc): %m");
		return 0.0;
	    }
	}

	for (;;) {
	    FD_ZERO(&ready);
	    FD_SET(udpsoc, &ready);
	    i = select(udpsoc+1, &ready, (fd_set *)0, (fd_set *)0, &tout);
	    if (i < 0) {
		if (errno == EINTR)
		    continue;
		syslog(LOG_ERR, "select(date read): %m");
		return 0.0;
	    }
	    if (0 == i)
		break;

	    fromlen = sizeof(from);
	    errno = 0;
	    i = recvfrom(udpsoc,(char*)&buf,sizeof(buf),0, &from,&fromlen);
	    if (sizeof(buf) != i) {
		syslog(LOG_ERR, "recvfrom(date read)=%d: %m", i);
		return 0.0;
	    }

	    sec = (double)ntohl(buf) - (double)BU - (double)time(0);
	    if (sec > (1990-1970)*365*SECDAY) {
		if (!udp_logged) {
		    udp_logged = 1;
		    syslog(LOG_ERR,
			   "%s says the date is before 1990: %u or %.0f",
			   masname, buf, sec);
		}
		return 0.0;
	    }
	    if (sec > (2000-1970)*365*SECDAY) {
		if (!udp_logged) {
		    udp_logged = 1;
		    syslog(LOG_ERR,
			   "%s says the date is after 1999: %u or %.0f",
			   masname, buf, sec);
		}
		return 0.0;
	    }

	    if (udp_logged) {
		udp_logged = 0;
		syslog(LOG_ERR,
		       "Date measurements from %s are working again.",
		       masname);
	    }
	    return sec;
	}
    }

    /* if we get here, we tried too many times */
    if (!udp_logged && (!quiet || debug > 0)) {
	udp_logged = 1;
	syslog(LOG_ERR, "%s will not tell us the date", masname);
    }
    return 0.0;
}



/* Compute a new adjustment for our clock.  The new adjustment
 *  should be chosen so that the next time we check, the difference
 *  between our clock and the masters will be zero.  Unfortunately,
 *  the measurement of the differences between our clocks is affected
 *  by systematic errors.  These errors are produced by intervals when
 *  the network delays to the master and back are asymmetric.
 *
 * If the error is much larger than the maximum network delay, we
 *  should not try to damp the change.  The master might have reset its
 *  clock.
 *
 * We should not try to damp the change very much until we have a good
 *  baseline.
 *
 * Large but isolated measured errors should be ignored.
 *
 * We want to choose adjustments so that each measured delta is close
 *  to zero.  That is, we should predict the drift for the next period
 *  and adjust the clock for that as well as for the currently measured
 *  error.
 *
 * The adjustment we compute should completely fix any error attributed
 *  to the long term drift of the local clock.  We should correct only part
 *  of rest of the error.
 */


/* smooth a measurement
 */
smoothadj(struct timeval *adjp,		/* put adjustment here */
	  double err)			/* error in sec */
{
    struct tms tm;
    double cur_ticks;			/* current time */
    double chg;				/* change in error since last time */
    double abschg;			/* change in error in sec/tick */
    double old_experr;
    double delta_ticks;
    double cur_drift;
    double tmp;
    int i;
    double *fp;


    chg = err - experr;			/* get change in error */
    old_experr = experr;
    cur_ticks = times(&tm);
    if (++pass == 1)
	prev_drift_ticks = prev_ticks = cur_ticks-1;
    delta_ticks = cur_ticks - prev_ticks;
    abschg = fabs(chg)/delta_ticks;	/* get scaled absolute value */
    if (pass <= 2)
	varerr = abschg;

    cur_drift = drift(cur_ticks-prev_drift_ticks);
    prev_drift_ticks = cur_ticks;

    /* After we have some good data, discard any measurement that
     * would change the average error too much.  This is because
     * the measurements are aflicted by periods when the transmission
     * delays are asymmetric.  This can come from asymmetric loads
     * on a SLIP line, or from scheduling delays for this process.
     * The former can cause errors of either sign, but the latter
     * causes only negative errors.
     *
     * Notice the binary exponential hack.  One good reading counts
     * against lots of bad ones.
     */

    /* get limit on variation, always > 1 tick */
    tmp = varerr*1.5 + (1.0/HZ)/delta_ticks;

    if (++skip > DISCARD_MEAS
	&& abschg > tmp			/* enough wildness to open filter */
	&& !measure_only) {
	if (debug != 0)
	    syslog(LOG_DEBUG, "opening filter for %+1.f", err*1000.0);
	varerr = skip_varerr;
	tmp = abschg;
	skip = 1;
    }


    if (abschg <= tmp
	|| measure_only
	|| pass <= INIT_PASSES) {
	if (abschg > (MAXADJ*1.0)/HZ) {	/* handle changes abruptly */
	    if (pass > INIT_PASSES) {
		pass = INIT_PASSES+1;
	    } else {
		pass--;
	    }
	    if (debug != 0)
		syslog(LOG_DEBUG, "clearing smoother for change of %+1.f",
		       chg*1000.0);
	    bzero((char*)&chgs[0], sizeof(chgs));
	    experr = 0;
	    tmp = err;

	} else {			/* average ordinary errors */
	    if (pass > 2) {
		varerr += (abschg-varerr)/VDAMP;
		if (varerr <= abschg)
		    varerr = abschg;
		drift_ticks += delta_ticks;
		drift_err += chg+prev_drift;
		/* The "chg+" term cancels the bad coupling from the changing
		 * of the drift to the next change.  Otherwise, it would
		 * oscillate.
		 */
	    }
	    i = min(chg_smooth, pass);
	    tmp = chg / i;
	    fp = &chgs[cur_chg];
	    do {
		*fp += tmp;
		if (++fp >= &chgs[MAX_CDAMP])
		    fp = &chgs[0];
	    } while (--i > 0);
	    tmp = chgs[cur_chg];
	    chgs[cur_chg] = 0;
	    cur_chg = (cur_chg+1) % MAX_CDAMP;
	    experr = err-tmp;
	}
	tmp += cur_drift;
	skip_varerr = varerr;
	prev_drift = cur_drift;
	prev_ticks = cur_ticks;
	skip /= 2;

    } else {
	if (debug != 0)
	    syslog(LOG_DEBUG, "skip error %+.1f", err*1000.0);
	if (skip_varerr <= abschg) {
		skip_varerr = abschg;
	} else {
		skip_varerr += (abschg-skip_varerr)/VDAMP;
	}
	prev_drift += cur_drift;
	tmp = chgs[cur_chg];
	chgs[cur_chg] = 0;
	cur_chg = (cur_chg+1) % MAX_CDAMP;
	experr -= tmp;
	tmp += cur_drift;
    }
    set_tval(adjp, tmp);


    if (debug != 0) {
	if (!headings)
	    syslog(LOG_DEBUG, "    err exp-err     adj  drift  var");
	    /*		      "+xxxx.x +xxxx.x +xxx.xx +x.xxx x.xx c" */
	syslog(LOG_DEBUG, (pass<=INIT_PASSES
	       ?	      "%+7.1f %+7.1f %+7.2f %+6.3f %4.2f %x"
	       :              "%+7.1f %+7.1f %+7.2f %+6.3f %4.2f"),
	       err*1000.0,
	       old_experr*1000.0,
	       float_time(adjp)*1000.0,
	       drift(HZ*1000),
	       varerr*(HZ*1000.0),
	       pass);
    } else if (measure_only) {
	if (!headings)
	    syslog(LOG_DEBUG, "      host     err     adj  drift    trim");
	    /*		      "ssssssssss +xxxx.x +xxx.xx +x.xxx +xxxxxx" */
	syslog(LOG_DEBUG,            "%10s %+7.1f %+7.2f %+6.3f %+7f",
	       masname,
	       err*1000.0,
	       float_time(adjp)*1000.0,
	       drift(HZ*1000),
	       drift(HZ*1000)*F_USEC_PER_SEC + timetrim);
    }
    if (++headings >= 20)
	headings = 0;
}


/* get a smoothed difference from the network
 */
static int				/* -1=failed */
get_netadj(int rawsoc,
	   int udpsoc,			/* use these sockets */
	   struct timeval *adjp)	/* put adjustment here */
{
    double secs;			/* error in seconds */


    /* Check the date with master.
    /* If within 12 hours, use ICMP timestamps to compute the delta.
     * If not, just change the time.
     */
    secs = daydiff(udpsoc);
    if (secs < -SECDAY/2 || secs > SECDAY/2) {
	adjp->tv_sec = secs;
	adjp->tv_usec = 0;
	if (debug != 0)
	    syslog(LOG_DEBUG, "   chg %+.3f sec", float_time(adjp));
	return -1;
    }

    if (secdiff(rawsoc,&secs) < 0) {
	guess_adj(adjp);
	return -1;
    }

    smoothadj(adjp,secs);		/* smooth the answer */
    return 0;
}


/* get a smoothed difference from WWV
 */
static int				/* -1=failed */
get_wwvadj(int wwvfd,
	   struct timeval *adjp)	/* put adjustment here */
{
    static int wwv_logged = 0;
    int i, j, k;
    struct timeval start, now, delta;
    int msec, yy;
    time_t tclock;
    union {
	struct wwv_qa s;
	u_char b[WWV_QA_SIZE+1];
	unsigned long l[WWV_QA_SIZE/4 + 1];
    } ans;
    fd_set ready;
    struct timeval tout;
    static char qstring[] = "QA0000";
#   define QLEN (sizeof(qstring)-1)
#define WWV_TIME (QLEN+WWV_QA_SIZE)*USEC_PER_C+USEC_PER_SEC/HZ


    bzero(&ans, sizeof(ans));
    FD_ZERO(&ready);
    k = 0;
    for (;;) {
	/* ask for the time and date
	 * start the measurement at a clock tick when our notion of the time
	 *	is most accurately represented by the values in the kernel
	 */
	sginap(1);
	if (0 > ioctl(wwvfd, TCFLSH, 2)) {
	    syslog(LOG_ERR, "ioctl(TCFLSH): %m");
	    return -1;
	}

	i = write(wwvfd, qstring, QLEN);
	(void)gettimeofday(&start, 0);
	if (i != QLEN) {
	    if (i < 0)
		syslog(LOG_ERR, "write(wwvfd): %m");
	    else
		syslog(LOG_ERR, "write(wwvfd)==%d", i);
	    guess_adj(adjp);
	    return -1;
	}

	/* read the answer
	 *	Take several tries as necessary, so that we get all we want
	 *	and do not wait forever.
	 *
	 *	We have to use select(2) because VMIN>0 says to wait one
	 *	for at least one character, and we don't want to die if
	 *	a character gets lost.
	 */
	i = 0;
	do {
	    FD_SET(wwvfd, &ready);
	    tout.tv_sec = 0;
	    tout.tv_usec = WWV_TIME;
	    j = select(wwvfd+1, &ready, 0,0, &tout);
	    if (j < 0) {
		if (errno == EINTR)
		    continue;
		syslog(LOG_ERR, "select(wwvfd): %m");
		guess_adj(adjp);
		return -1;
	    }
	    if (j == 0) {
		(void)gettimeofday(&now, 0);
		delta.tv_sec = now.tv_sec - start.tv_sec;
		delta.tv_usec = now.tv_usec - start.tv_usec;
		fix_time(&delta);
		if (delta.tv_sec == 0
		    && delta.tv_usec <= WWV_TIME+USEC_PER_SEC/HZ)
			continue;
		break;
	    }
	    j = read(wwvfd, &ans.b[i], WWV_QA_SIZE-i+1);
	    if (j < 0) {
		syslog(LOG_ERR, "read(wwvfd): %m");
		guess_adj(adjp);
		return -1;
	    }
	    i += j;
	} while (i < WWV_QA_SIZE);

	/* the preceding was time-critical.  From now on we deal with
	 *	deltas, and so do not have to worry.
	 */

	/* See that the clock answered (and we heard) quickly enough
	 *
	 * The manual does not say, but people at Precision Time Standards
	 * have said that the time reported by the clock is correct at the
	 * last bit of the message.
	 *
	 * Require that we finished within 2 character times of when the
	 * clock finished.
	 */
	(void)gettimeofday(&now, 0);
	delta.tv_sec = now.tv_sec - start.tv_sec;
	delta.tv_usec = now.tv_usec - start.tv_usec;
	fix_time(&delta);

	if (i == 0) {
	    if (!wwv_logged) {
		syslog(LOG_ERR, "The WWV receiver is not responding.");
		wwv_logged = 1;
	    }

	} else if (i != WWV_QA_SIZE) {
	    syslog(LOG_ERR, "read only %d bytes=%08x %08x %08x %08x",
		   i, ans.l[0], ans.l[1], ans.l[2], ans.l[3]);

	} else if (ans.s.ok != 0x08	/* validate the answer */
	    || ans.s.cr != '\r'
	    || ans.s.pm != 0) {
	    syslog(LOG_ERR, "read nonsense %d bytes=%08x %08x %08x %08x",
		   i, ans.l[0], ans.l[1], ans.l[2], ans.l[3]);

	} else if (delta.tv_sec != 0
		   || delta.tv_usec > WWV_TIME+USEC_PER_SEC/HZ) {
	    syslog(LOG_ERR, "slow receiver took %.1f msec on try #%d",
		   float_time(&delta)*1000.0, k);

	} else {
	    if (debug > 1)
		syslog(LOG_DEBUG, "WWV receiver finished in %.3f msec",
		       float_time(&delta)*1000.0);
	    break;
	}

	if (++k == 10) {
	    guess_adj(adjp);
	    return -1;
	}
    }


    /* Decode the answer.
     * Use our notion of the year, because people tend to forget to
     *	change the switch in the clock
     */
    msec = ((ans.s.ms_hi/2) << 6) + ans.s.ms_lo/2;
    tclock = -SECDAY;			/* day #1 = 1st day of the year */
    yy = ans.s.yy/2 + 86-70;
    tclock += yy*SECYR;			/* count seconds since the epoch */
    tclock += ((yy+1)/4)*SECDAY;	/*	until first of the year */

    tclock += (((ans.s.dd_hi/2) << 6) + ans.s.dd_lo/2)*SECDAY;
    tclock += (ans.s.hh/2)*SECHR;
    tclock += (ans.s.mm/2)*60;
    tclock += ans.s.ss/2;

    /* compute the error */
    adjp->tv_sec = tclock - now.tv_sec;
    adjp->tv_usec = msec*1000 - now.tv_usec;
    fix_time(adjp);
    if (debug > 1)
	syslog(LOG_DEBUG, "delta=%+1.f msec",
	       adjp->tv_sec*1000.0 + adjp->tv_usec/1000.0);

    /* If we are off by a multiple of years, and it is near the first of the
     *	    year, assume the clock is wrong.  Assume someone forget to
     *	    change the year jumpers in the clock or changed the jumpers
     *	    too soon.
     */
    if (adjp->tv_sec < -SECYR
	&& (-adjp->tv_sec) % SECYR < SECDAY
	&& (now.tv_sec % SECYR < 14*SECDAY
	    || now.tv_sec % SECYR > 360*SECDAY)) {
	syslog(LOG_ERR, "year in the receiver is %4D", yy+1970);
	adjp->tv_sec %= SECDAY;
    }

    /* if too much to smooth, simply set the time */
    if (adjp->tv_sec <= -MAXADJ
	|| adjp->tv_sec >= MAXADJ) {
	if (debug != 0 || measure_only)
	    syslog(LOG_DEBUG, "   chg %+.3f", float_time(adjp));
	return 0;
    }

    if (0 != wwv_logged) {		/* after a successful measurement, */
	wwv_logged = 0;			/*	resume complaining */
	syslog(LOG_ERR, "The WWV receiver is working again.");
    }

    smoothadj(adjp,			/* smooth the answer */
	      adjp->tv_sec*1000.0 + adjp->tv_usec/F_USEC_PER_SEC);
	 return 0;
}



/* adjust the clock
 */
int					/* delay this long afterwards */
adjclock(int rawsoc,			/* use these sockets */
	 int udpsoc,
	 int wwvfd)			/* or this file descriptor */
{
    static int max_fuzz;
    static int passes = 0;
    struct timeval now, adj;
    int meas_ok;
    clock_t rep;
    double delta;
    struct tms tm;
    double cur_ticks;


    /* choose a random time to try again later
     * to try to keep a gaggle of clients from asking simultaneously
     */
    if (passes < INIT_PASSES) {
	if (passes < 1) {
	    (void)gettimeofday(&now, 0);
	    srandom((unsigned)(now.tv_sec + now.tv_usec));
	    max_fuzz = rep_rate/10;
	    if (max_fuzz < 2)
		max_fuzz = 2;
	}
	/* Choose the speed at which to correct the clock
	 * Always get at least 25% of the error when listening to the radio.
	 * Get it all done within 60 minutes.
	 */
	chg_smooth = (SECHR*HZ)/rep_rate;
	if (chg_smooth < 1)
	    chg_smooth = 1;
	if (mtype != 0 && chg_smooth > 4)
	    chg_smooth = 4;
	if (chg_smooth > MAX_CDAMP)
	    chg_smooth = MAX_CDAMP;
	rep = (rep_rate > INIT_REP_RATE) ? INIT_REP_RATE : rep_rate;
    } else {
	rep = rep_rate + (random() % max_fuzz);
    }

    if (mtype==0) {			/* get an adjustment */
	meas_ok = get_netadj(rawsoc,udpsoc, &adj);
    } else {
	meas_ok = get_wwvadj(wwvfd, &adj);
    }
    if (meas_ok >= 0)
	passes++;

    if (!measure_only) {
	delta = adj.tv_sec*F_USEC_PER_SEC + adj.tv_usec;

	/* If our time is far from the masters, smash it.
	 */
	if (adj.tv_sec <= -MAXADJ || adj.tv_sec >= MAXADJ
	    || (adj.tv_sec != 0 && passes < INIT_PASSES)) {
	    char olddate[32];
	    (void)gettimeofday(&now, 0);
	    (void)cftime(olddate, "%D %T", &now.tv_sec);
	    now.tv_sec += adj.tv_sec;
	    now.tv_usec += adj.tv_usec;
	    fix_time(&now);
	    if (settimeofday(&now,0) < 0) {
		syslog(LOG_ERR, "settimeofday(): %m");
		exit(1);
	    }
	    syslog(LOG_NOTICE, "date changed from %s", olddate);

	} else {
	    /* always adjust the clock to keep the TOD chip out of it
	     */
	    if (adjtime(&adj, 0) < 0) {
		syslog(LOG_ERR, "adjtime(): %m");
		exit(1);
	    }
	}

	cur_ticks = times(&tm);
	if (passes <= INIT_PASSES) {
		/* just mark time until time is stable */
		tot_ticks += cur_ticks-hr_ticks;
		hr_ticks = cur_ticks;
	} else {
	    static double nag_tick;
	    double hr_delta_ticks, tot_delta_ticks;
	    double tru_tot_adj, tru_hr_adj; /* nsecs of adjustment */
	    double tot_trim, hr_trim;	/* nsec/sec */

	    tot_delta_ticks = cur_ticks-tot_ticks;
	    tot_adj += delta*1000.0;
	    hr_adj += delta*1000.0;

	    if (tot_delta_ticks >= 16*SECDAY*HZ) {
		tot_adj -= rint(tot_adj/16);
		tot_ticks += rint(tot_delta_ticks/16);
		tot_delta_ticks = cur_ticks-tot_ticks;
	    }
	    hr_delta_ticks = cur_ticks-hr_ticks;

	    tru_hr_adj = hr_adj + timetrim*rint(hr_delta_ticks/HZ);
	    tru_tot_adj = tot_adj + timetrim*rint(tot_delta_ticks/HZ);

	    if (meas_ok >= 0
		&& (hr_delta_ticks >= SECDAY*HZ
		    || (tot_delta_ticks < 4*SECDAY*HZ
			&& hr_delta_ticks >= SECHR*HZ)
		    || (debug > 2
			&& hr_delta_ticks >= (SECHR/10)*HZ))) {

		hr_trim = rint(tru_hr_adj*HZ/hr_delta_ticks);
		tot_trim = rint(tru_tot_adj*HZ/tot_delta_ticks);

		if (debug != 0
		    || (abs(timetrim - hr_trim) > 100000.0
			&& 0 == timetrim_fn
			&& (cur_ticks - nag_tick) >= 24*SECDAY*HZ)) {
		    nag_tick = cur_ticks;
		    syslog(LOG_INFO,
		   "%+.3f/%.2f or %+.3f/%.2f sec/hr; timetrim=%+.0f or %+.0f",
			   tru_tot_adj/F_NSEC_PER_SEC,
			   tot_delta_ticks/(SECHR*HZ*1.0),
			   tru_hr_adj/F_NSEC_PER_SEC,
			   hr_delta_ticks/(SECHR*HZ*1.0),
			   tot_trim,
			   hr_trim);
		}

		if (tot_trim < -MAX_TRIM || tot_trim > MAX_TRIM) {
		    tot_ticks = hr_ticks;
		    tot_adj = hr_adj;
		    if (debug)
			syslog(LOG_DEBUG, "trimming tot_adj=%d, tot_ticks=%d",
			       tot_adj, tot_ticks);
		} else if (0 > syssgi(SGI_SETTIMETRIM, (long)tot_trim)) {
		    syslog(LOG_ERR,"SETTIMETRIM(%d): %m",
			   (long)tot_trim);
		} else {
		    if (0 != timetrim_fn) {
			timetrim_st = fopen(timetrim_fn, "w");
			if (0 == timetrim_st) {
			    syslog(LOG_ERR, "fopen(%s): %m",
				   timetrim_fn);
			} else {
			    if (0 > fprintf(timetrim_st,
					    timetrim_wpat,
					    (long)tot_trim,
					    tru_tot_adj,
					    tot_delta_ticks)) {
				syslog(LOG_ERR, "fprintf(%s): %m",
				       timetrim_fn);
			    }
			    (void)fclose(timetrim_st);
			}
		    }

		    /* The drift is being changed by the change in
		     * timetrim.  Convert nsec/sec of trim to ticks/tick.
		     */
		    drift_err -= ((tot_trim - (double)timetrim)
				    * drift_ticks
				    / (F_NSEC_PER_SEC*HZ));

		    /* The total adjustment is being moved to timetrim
		     */
		    tot_adj -= ((tot_trim - timetrim)
				* rint(tot_delta_ticks / HZ));
		    timetrim = tot_trim;
		}

		hr_ticks = cur_ticks;
		hr_adj = 0;
	    }
	}
    }

    return rep;
}



/* open a TTY connected to a WWV clock
 */
int					/* <0 on error */
get_wwvfd(int *wwvfdp)
{
    struct termio ttyb;

    *wwvfdp = open(masname, O_RDWR, 0);
    if (0 > *wwvfdp) {
	(void)fprintf(stderr, "open(%s): %s\n",
		      masname, strerror(errno));
	return -1;
    }

    if (0 > ioctl(*wwvfdp, TCGETA, &ttyb)) {
	(void)fprintf(stderr, "ioctl(%s,TCGETA): %s\n",
		      masname, strerror(errno));
	return -1;
    }

    ttyb.c_iflag = 0;
    ttyb.c_oflag = 0;
    ttyb.c_cflag = (ttyb.c_cflag & ~CBAUD) | B9600 | HUPCL;
    ttyb.c_lflag = 0;
    ttyb.c_cc[VMIN] = 0;
    ttyb.c_cc[VTIME] = 1;

    if (0 > ioctl(*wwvfdp, TCSETA, &ttyb)) {
	(void)fprintf(stderr, "ioctl(%s,TCSETA): %s\n",
		      masname, strerror(errno));
	return -1;
    }

    return 0;
}



/* get sockets connected to the master
 */
int					/* <0 on error */
get_masname(char *port,
	    int *rawsocp,
	    int *udpsocp)
{
    struct protoent *proto;
    char *p;


    bzero((char *)&masaddr, sizeof(masaddr));
    masaddr.sin_family = AF_INET;
    masaddr.sin_addr.s_addr = inet_addr(masname);
    if (masaddr.sin_addr.s_addr == -1) {
	struct hostent *hp;
	hp = gethostbyname(masname);
	if (hp) {
	    masaddr.sin_family = hp->h_addrtype;
	    bcopy(hp->h_addr, (caddr_t)&masaddr.sin_addr,
		  hp->h_length);
	} else {
	    (void)fprintf(stderr, "%s: unknown host name %s\n",
			  pgmname, masname);

	}
    }

    if ((proto = getprotobyname("icmp")) == NULL) {
	(void)fprintf(stderr, "%s: icmp is an unknown protocol\n", pgmname);
	return -1;
    }
    if ((*rawsocp = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
	(void)fprintf(stderr, "%s: socket(RAW): %s\n",
		      pgmname, strerror(errno));
	return -1;
    }
    if (connect(*rawsocp, (struct sockaddr*)&masaddr, sizeof(masaddr)) < 0) {
	(void)fprintf(stderr, "%s: connect(RAW): %s\n",
		      pgmname, strerror(errno));
	return -1;
    }

    masaddr.sin_port = strtol(port,&p,0);
    if (*p != '\0' || masaddr.sin_port == 0) {
	struct servent *sp;
	sp = getservbyname(PORT, "udp");
	if (!sp) {
	    (void)fprintf(stderr, "%s: %s/%s is an unknown service\n",
			  pgmname, PORT, PROTO);
	    return -1;
	}
	masaddr.sin_port = sp->s_port;
    }
    if ((proto = getprotobyname(PROTO)) == NULL) {
	(void)fprintf(stderr, "%s: %s is an unknown protocol\n",
		      pgmname, PROTO);
	return -1;
    }
    if ((*udpsocp = socket(AF_INET, SOCK_DGRAM, proto->p_proto)) < 0) {
	(void)fprintf(stderr, "%s: socket(UDP): %s\n",
		      pgmname, strerror(errno));
	return -1;
    }
    if (connect(*udpsocp, (struct sockaddr*)&masaddr, sizeof(masaddr)) < 0) {
	(void)fprintf(stderr, "%s: connect(UDP): %s\n",
		      pgmname, strerror(errno));
	return -1;
    }

    return 0;
}



main(int argc,
     char *argv[])
{
    int i;
    char *port = "time";
    char *p;
    int udpsoc = -1;
    int rawsoc = -1;
    int wwvfd = -1;
    struct tms tm;


    pgmname = argv[0];
    if (0 > syssgi(SGI_GETTIMETRIM, &timetrim)) {
	(void)fprintf(stderr, "%s: syssgi(GETTIMETRIM): %s\n",
		      pgmname, strerror(errno));
	timetrim = 0;
    }
    tot_ticks = hr_ticks = times(&tm);

    while ((i = getopt(argc, argv, "mdBqT:t:W:w:r:p:i:D:H:C:P:")) != EOF)
	switch (i) {
	case 'm':
	    measure_only = 1;
	    break;

	case 'd':
	    debug++;
	    break;

	case 'B':
	    background = 0;
	    break;

	case 'q':
	    quiet = 1;
	    break;

	case 'T':
	    icmp_max_sends = strtol(optarg,&p,0);
	    if (*p != '\0'
		|| icmp_max_sends < icmp_min_diffs
		|| icmp_max_sends > MAX_MAX_ICMP_TRIALS) {
		(void)fprintf(stderr,
			      "%s: invalid max # of ICMP measurements: %s\n",
			      pgmname, optarg);
		exit(1);
	    }
	    break;

	case 't':
	    icmp_min_diffs = strtol(optarg,&p,0);
	    if (*p != '\0'
		|| icmp_min_diffs < 1
		|| (icmp_max_sends > 0
		    && icmp_min_diffs > icmp_max_sends)) {
		(void)fprintf(stderr,
			      "%s: invalid min # of ICMP measurements: %s\n",
			      pgmname, optarg);
		exit(1);
	    }
	    break;

	case 'W':
	    date_wait = strtol(optarg,&p,0);
	    if (*p != '\0'
		|| date_wait < 1) {
		(void)fprintf(stderr,
			      "%s: invalid date/time wait-time: %s\n",
			      pgmname, optarg);
		exit(1);
	    }
	    break;

	case 'w':
	    icmp_wait = strtol(optarg,&p,0);
	    if (*p != '\0'
		|| icmp_wait < 1 || icmp_wait > MAX_NET_DELAY*2) {
		(void)fprintf(stderr,
			      "%s: invalid ICMP wait-time: %s\n",
			      optarg, optarg);
		exit(1);
	    }
	    break;

	case 'r':
	    rep_rate = strtol(optarg,&p,0)*HZ;
	    if (*p != '\0'
		|| rep_rate < 5*HZ) {
		(void)fprintf(stderr, "%s: repetition rate %s bad\n",
			      pgmname, optarg);
		exit(1);
	    }
	    break;

	case 'p':
	    port = optarg;
	    break;

	case 'i':			/* initial nsec/sec of drift */
	    i = strtol(optarg,&p,0);
	    if (*p != '\0'
		|| i > 1000000		/* at most 0.1% or 1 msec/sec */
		|| i < -1000000) {	/* or 1 million nsec/sec */
		(void)fprintf(stderr,
		    "%s: initial drift %s is not between 1 & 1000000 nsec\n",
			      pgmname, optarg);
		exit(1);
	    }
	    drift_err = (i/(F_NSEC_PER_SEC))*(DRIFT_TICKS/HZ);
	    break;

	case 'D':			/* get usec/sec max drift */
	    i = strtol(optarg,&p,0);
	    if (*p != '\0'
		|| i < 1 || i > MAX_DRIFT) {  /* at most 0.3% or 3 msec/sec */
		(void)fprintf(stderr,
		    "%s: max-drift %s is not between 1 & 3000 usec\n",
			      pgmname, optarg);
		exit(1);
	    }
	    max_drift = i/(F_NSEC_PER_SEC*HZ);	/* convert to sec/HZ */
	    break;

	case 'H':
	    if (mtype >= 0) {
		(void)fprintf(stderr, USAGE, pgmname);
		exit(1);
	    }
	    mtype = 0;
	    masname = optarg;
	    break;

	case 'C':
	    if (mtype >= 0) {
		(void)fprintf(stderr, USAGE, pgmname);
		exit(1);
	    }
	    mtype = 1;
	    masname = optarg;
	    break;

	case 'P':
	    timetrim_fn = optarg;
	    break;

	default:
	    (void)fprintf(stderr, USAGE, pgmname);
	    exit(1);
    }
    if (argc != optind) {
	(void)fprintf(stderr, USAGE, pgmname);
	exit(1);
    }

    switch (mtype) {
    case 0:
	if (get_masname(port,&rawsoc,&udpsoc) < 0)
	    exit(1);
	break;
    case 1:
	if (get_wwvfd(&wwvfd) < 0)
	    exit(1);
	break;
    default:
	(void)fprintf(stderr, USAGE, pgmname);
	exit(1);
    }

    if (0 == rep_rate)
	rep_rate = DEF_REP_RATE;

    if (0 == icmp_max_sends)
	icmp_max_sends = (icmp_min_diffs > 0 ? icmp_min_diffs*4 : 16);
    if (0 == icmp_min_diffs)
	icmp_min_diffs = (icmp_max_sends > 16 ? icmp_max_sends/4 : 4);

    if (0 == max_drift) {		/* assume a max. drift of 1% */
	max_drift = 0.01/HZ;
    }
    neg_max_drift = -max_drift;


    (void)_daemonize(!background ? _DF_NOFORK : 0, udpsoc, rawsoc, wwvfd);

    (void)signal(SIGUSR1, moredebug);
    (void)signal(SIGUSR2, lessdebug);

    openlog("timeslave", LOG_CONS|LOG_PID, LOG_DAEMON);

    if (timetrim_fn != 0) {
	timetrim_st = fopen(timetrim_fn, "r+");
	if (0 == timetrim_st) {
	    if (errno != ENOENT) {
		(void)fprintf(stderr, "%s: %s: %s\n",
			      pgmname,timetrim_fn,strerror(errno));
		timetrim_fn = 0;
	    }
	} else {
	    long trim;
	    double adj, ticks;

	    i = fscanf(timetrim_st, timetrim_rpat,
		       &trim, &adj, &ticks);
	    if (i < 1
		|| trim > MAX_TRIM
		|| trim < -MAX_TRIM
		|| ticks < 0
		|| i == 2
		|| (i == 3
		    && trim != rint(adj*HZ/ticks))) {
		if (i != EOF)
		    (void)fprintf(stderr,
				  "%s: %s: unrecognized contents\n",
				  pgmname, timetrim_fn);
		if (debug > 0)
		    syslog(LOG_DEBUG, "ignoring %s", timetrim_fn);
	    } else {
		if (0 > syssgi(SGI_SETTIMETRIM, trim)) {
		    (void)fprintf(stderr, "%s: syssgi(SETTIMETRIM): %s\n",
				  pgmname, strerror(errno));
		} else {
		    timetrim = trim;
		}
		if (i == 3) {
		    tot_ticks -= ticks;
		    if (debug > 0)
			syslog(LOG_DEBUG, "start tot_ticks=%.0f", ticks);
		}
	    }
	    (void)fclose(timetrim_st);
	}
    }

    (void)schedctl(NDPRI,0,NDPHIMAX);	/* run fast to get good time */

    for (;;) {
	i = adjclock(rawsoc,udpsoc,wwvfd);  /* set the date */
	if (debug > 1)
	    syslog(LOG_DEBUG, "delaying for %.2f sec",
		   ((float)i)/HZ);
	sginap(i);
    }
}
