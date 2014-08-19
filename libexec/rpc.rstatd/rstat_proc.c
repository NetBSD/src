/*	$NetBSD: rstat_proc.c,v 1.47.2.1 2014/08/20 00:02:23 tls Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <sys/cdefs.h>
#if 0
static char sccsid[] =
	"from: @(#)rpc.rstatd.c 1.1 86/09/25 Copyr 1984 Sun Micro";
static char sccsid[] =
	"from: @(#)rstat_proc.c	2.2 88/08/01 4.0 RPCSRC";
#endif
__RCSID("$NetBSD: rstat_proc.c,v 1.47.2.1 2014/08/20 00:02:23 tls Exp $");

/*
 * rstat service:  built with rstat.x and derived from rpc.rstatd.c
 *
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/sched.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <syslog.h>
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h>
#include "drvstats.h"
#include <net/if.h>

/*
 * XXX
 *
 * this is a huge hack to stop `struct pmap' being
 * defined twice!
 */
#define _RPC_PMAP_PROT_H_
#include <rpc/rpc.h>

#undef FSHIFT			 /* Use protocol's shift and scale values */
#undef FSCALE
#undef DK_NDRIVE
#undef CPUSTATES
#undef if_ipackets
#undef if_ierrors
#undef if_opackets
#undef if_oerrors
#undef if_collisions
#include <rpcsvc/rstat.h>

#define BSD_CPUSTATES	5	/* Use protocol's idea of CPU states */
int	cp_xlat[CPUSTATES] = { CP_USER, CP_NICE, CP_SYS, CP_IDLE };

int hz;

extern int from_inetd;
int sincelastreq = 0;		/* number of alarms since last request */
extern int closedown;

union {
	struct stats s1;
	struct statsswtch s2;
	struct statstime s3;
} stats_all;

void updatestat(int);
void stat_init(void);
int havedisk(void);
void rstat_service(struct svc_req *, SVCXPRT *);

static int stat_is_init = 0;

#ifndef FSCALE
#define FSCALE (1 << 8)
#endif

void
stat_init(void)
{
	stat_is_init = 1;
	drvinit(0);
	updatestat(0);
	(void) signal(SIGALRM, updatestat);
	alarm(1);
}

statstime *
rstatproc_stats_3_svc(void *arg, struct svc_req *rqstp)
{
	if (!stat_is_init)
	        stat_init();
	sincelastreq = 0;
	return (&stats_all.s3);
}

statsswtch *
rstatproc_stats_2_svc(void *arg, struct svc_req *rqstp)
{
	if (!stat_is_init)
	        stat_init();
	sincelastreq = 0;
	stats_all.s2.if_opackets = stats_all.s3.if_opackets;
	return (&stats_all.s2);
}

stats *
rstatproc_stats_1_svc(void *arg, struct svc_req *rqstp)
{
	if (!stat_is_init)
	        stat_init();
	sincelastreq = 0;
	stats_all.s1.if_opackets = stats_all.s3.if_opackets;
	return (&stats_all.s1);
}

u_int *
rstatproc_havedisk_3_svc(void *arg, struct svc_req *rqstp)
{
	static u_int have;

	if (!stat_is_init)
	        stat_init();
	sincelastreq = 0;
	have = havedisk();
	return (&have);
}

u_int *
rstatproc_havedisk_2_svc(void *arg, struct svc_req *rqstp)
{
	return (rstatproc_havedisk_3_svc(arg, rqstp));
}

u_int *
rstatproc_havedisk_1_svc(void *arg, struct svc_req *rqstp)
{
	return (rstatproc_havedisk_3_svc(arg, rqstp));
}

void
updatestat(int dummy)
{
	struct if_nameindex *ifps;
	struct ifdatareq ifdr;
	size_t i, len;
	int mib[2], s;
	struct uvmexp_sysctl uvmexp;
	double avrun[3];
	struct timeval tm, btm;

#ifdef DEBUG
	syslog(LOG_DEBUG, "entering updatestat");
#endif
	if (sincelastreq >= closedown) {
#ifdef DEBUG
                syslog(LOG_DEBUG, "about to closedown");
#endif
                if (from_inetd)
                        exit(0);
                else {
                        stat_is_init = 0;
                        return;
                }
	}
	sincelastreq++;

	/* 
	 * drvreadstats reads in the "disk_count" as well as the "disklist"
	 * statistics.  It also retrieves "hz" and the "cp_time" array.
	 */
	drvreadstats();
	memset(stats_all.s3.dk_xfer, 0, sizeof(stats_all.s3.dk_xfer));
	for (i = 0; i < ndrive && i < DK_NDRIVE; i++)
		stats_all.s3.dk_xfer[i] = cur.rxfer[i] + cur.wxfer[i];

	for (i = 0; i < CPUSTATES; i++)
		stats_all.s3.cp_time[i] = cur.cp_time[cp_xlat[i]];
        (void)getloadavg(avrun, sizeof(avrun) / sizeof(avrun[0]));
	stats_all.s3.avenrun[0] = avrun[0] * FSCALE;
	stats_all.s3.avenrun[1] = avrun[1] * FSCALE;
	stats_all.s3.avenrun[2] = avrun[2] * FSCALE;
	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	len = sizeof(btm);
	if (sysctl(mib, 2, &btm, &len, NULL, 0) < 0) {
		syslog(LOG_ERR, "can't sysctl kern.boottime");
		exit(1);
	}
	stats_all.s3.boottime.tv_sec = btm.tv_sec;
	stats_all.s3.boottime.tv_usec = btm.tv_usec;


#ifdef DEBUG
	syslog(LOG_DEBUG, "%d %d %d %d %d\n", stats_all.s3.cp_time[0],
	    stats_all.s3.cp_time[1], stats_all.s3.cp_time[2],
	    stats_all.s3.cp_time[3], stats_all.s3.cp_time[4]);
#endif

	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP2;
	len = sizeof(uvmexp);
	if (sysctl(mib, 2, &uvmexp, &len, NULL, 0) < 0) {
		syslog(LOG_ERR, "can't sysctl vm.uvmexp2");
		exit(1);
	}
	stats_all.s3.v_pgpgin = uvmexp.fltanget;
	stats_all.s3.v_pgpgout = uvmexp.pdpageouts;
	stats_all.s3.v_pswpin = uvmexp.swapins;
	stats_all.s3.v_pswpout = uvmexp.swapouts;
	stats_all.s3.v_intr = uvmexp.intrs;
	stats_all.s3.v_swtch = uvmexp.swtch;
	gettimeofday(&tm, (struct timezone *) 0);
	stats_all.s3.v_intr -= hz*(tm.tv_sec - btm.tv_sec) +
	    hz*(tm.tv_usec - btm.tv_usec)/1000000;
	
	stats_all.s3.if_ipackets = 0;
	stats_all.s3.if_opackets = 0;
	stats_all.s3.if_ierrors = 0;
	stats_all.s3.if_oerrors = 0;
	stats_all.s3.if_collisions = 0;

	ifps = if_nameindex();
	if (ifps == NULL) {
		syslog(LOG_ERR, "can't read interface list from kernel");
		exit(1);
	}
	s = socket(AF_INET, SOCK_DGRAM, 0);
	for (i = 0; s != -1 && ifps[i].if_name != NULL; ++i) {
		strlcpy(ifdr.ifdr_name, ifps[i].if_name, sizeof(ifdr.ifdr_name));
		if (ioctl(s, SIOCGIFDATA, &ifdr) != 0)
			continue;
		stats_all.s3.if_ipackets += ifdr.ifdr_data.ifi_ipackets;
		stats_all.s3.if_opackets += ifdr.ifdr_data.ifi_opackets;
		stats_all.s3.if_ierrors += ifdr.ifdr_data.ifi_ierrors;
		stats_all.s3.if_oerrors += ifdr.ifdr_data.ifi_oerrors;
		stats_all.s3.if_collisions += ifdr.ifdr_data.ifi_collisions;
	}
	if (s != -1)
		close(s);
	if_freenameindex(ifps);

	stats_all.s3.curtime.tv_sec = tm.tv_sec;
	stats_all.s3.curtime.tv_usec = tm.tv_usec;
	alarm(1);
}

/*
 * returns true if have a disk
 */
int
havedisk(void)
{
	return ndrive != 0;
}

void
rstat_service(struct svc_req *rqstp, SVCXPRT *transp)
{
	union {
		int fill;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	char *(*local)(void *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, (xdrproc_t)xdr_void, NULL);
		goto leave;

	case RSTATPROC_STATS:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_statstime;
                switch (rqstp->rq_vers) {
                case RSTATVERS_ORIG:
                        local = (char *(*)(void *, struct svc_req *))
				rstatproc_stats_1_svc;
                        break;
                case RSTATVERS_SWTCH:
                        local = (char *(*)(void *, struct svc_req *))
				rstatproc_stats_2_svc;
                        break;
                case RSTATVERS_TIME:
                        local = (char *(*)(void *, struct svc_req *))
				rstatproc_stats_3_svc;
                        break;
                default:
                        svcerr_progvers(transp, RSTATVERS_ORIG, RSTATVERS_TIME);
                        goto leave;
                }
		break;

	case RSTATPROC_HAVEDISK:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_u_int;
                switch (rqstp->rq_vers) {
                case RSTATVERS_ORIG:
                        local = (char *(*)(void *, struct svc_req *))
				rstatproc_havedisk_1_svc;
                        break;
                case RSTATVERS_SWTCH:
                        local = (char *(*)(void *, struct svc_req *))
				rstatproc_havedisk_2_svc;
                        break;
                case RSTATVERS_TIME:
                        local = (char *(*)(void *, struct svc_req *))
				rstatproc_havedisk_3_svc;
                        break;
                default:
                        svcerr_progvers(transp, RSTATVERS_ORIG, RSTATVERS_TIME);
                        goto leave;
                }
		break;

	default:
		svcerr_noproc(transp);
		goto leave;
	}
	memset((char *)&argument, 0, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		goto leave;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		(void)fprintf(stderr, "unable to free arguments\n");
		exit(1);
	}
leave:
        if (from_inetd)
                exit(0);
}
