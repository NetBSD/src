/*	$NetBSD: rstat_proc.c,v 1.35 2000/06/29 06:26:33 mrg Exp $	*/

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
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)rpc.rstatd.c 1.1 86/09/25 Copyr 1984 Sun Micro";
static char sccsid[] = "from: @(#)rstat_proc.c	2.2 88/08/01 4.0 RPCSRC";
#else
__RCSID("$NetBSD: rstat_proc.c,v 1.35 2000/06/29 06:26:33 mrg Exp $");
#endif
#endif

/*
 * rstat service:  built with rstat.x and derived from rpc.rstatd.c
 *
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/sched.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <syslog.h>
#ifdef BSD
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h>
#include <sys/dkstat.h>
#include "dkstats.h"
#else
#include <sys/dk.h>
#endif

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

#ifdef BSD
#define BSD_CPUSTATES	5	/* Use protocol's idea of CPU states */
int	cp_xlat[CPUSTATES] = { CP_USER, CP_NICE, CP_SYS, CP_IDLE };
#endif

struct nlist nl[] = {
#define	X_IFNET		0
	{ "_ifnet" },
	{ NULL },
};

extern int dk_ndrive;		/* From dkstats.c */
extern struct _disk cur, last;
int hz;
char *memf = NULL, *nlistf = NULL;

struct ifnet_head ifnetq;	/* chain of ethernet interfaces */
int numintfs;

extern int from_inetd;
int sincelastreq = 0;		/* number of alarms since last request */
extern int closedown;
kvm_t *kfd;

union {
	struct stats s1;
	struct statsswtch s2;
	struct statstime s3;
} stats_all;

extern void dkreadstats(void);
extern int dkinit(int, gid_t);

void updatestat(int);
void setup(void);
void setup_kd_once(void);
void stat_init(void);
int havedisk(void);
void rstat_service(struct svc_req *, SVCXPRT *);

static int stat_is_init = 0;

#ifndef FSCALE
#define FSCALE (1 << 8)
#endif

void
stat_init()
{
	stat_is_init = 1;
	setup();
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
	long off;
	int i;
	size_t len;
	int mib[2];
	struct uvmexp uvmexp;
	struct ifnet ifnet;
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
	 * dkreadstats reads in the "disk_count" as well as the "disklist"
	 * statistics.  It also retrieves "hz" and the "cp_time" array.
	 */
	dkreadstats();
	memset(stats_all.s3.dk_xfer, 0, sizeof(stats_all.s3.dk_xfer));
	for (i = 0; i < dk_ndrive && i < DK_NDRIVE; i++)
		stats_all.s3.dk_xfer[i] = cur.dk_xfer[i];

#ifdef BSD
	for (i = 0; i < CPUSTATES; i++)
		stats_all.s3.cp_time[i] = cur.cp_time[cp_xlat[i]];
#else
 	if (kvm_read(kfd, (long)nl[X_CPTIME].n_value,
		     (char *)stats_all.s3.cp_time,
		     sizeof (stats_all.s3.cp_time))
	    != sizeof (stats_all.s3.cp_time)) {
		syslog(LOG_ERR, "can't read cp_time from kmem");
		exit(1);
	}
#endif
#ifdef BSD
        (void)getloadavg(avrun, sizeof(avrun) / sizeof(avrun[0]));
#endif
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
	mib[1] = VM_UVMEXP;
	len = sizeof(uvmexp);
	if (sysctl(mib, 2, &uvmexp, &len, NULL, 0) < 0) {
		syslog(LOG_ERR, "can't sysctl vm.uvmexp");
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
	for (off = (long)ifnetq.tqh_first, i = 0; off && i < numintfs; i++) {
		if (kvm_read(kfd, off, (char *)&ifnet, sizeof ifnet) !=
		    sizeof ifnet) {
			syslog(LOG_ERR, "can't read ifnet from kmem");
			exit(1);
		}
		stats_all.s3.if_ipackets += ifnet.if_data.ifi_ipackets;
		stats_all.s3.if_opackets += ifnet.if_data.ifi_opackets;
		stats_all.s3.if_ierrors += ifnet.if_data.ifi_ierrors;
		stats_all.s3.if_oerrors += ifnet.if_data.ifi_oerrors;
		stats_all.s3.if_collisions += ifnet.if_data.ifi_collisions;
		off = (long)ifnet.if_list.tqe_next;
	}
	gettimeofday((struct timeval *)&stats_all.s3.curtime,
		(struct timezone *) 0);
	alarm(1);
}

void
setup_kd_once()
{
        char errbuf[_POSIX2_LINE_MAX];
        kfd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
        if (kfd == NULL) {
                syslog(LOG_ERR, "%s", errbuf);
                exit (1);
        }
}

void
setup()
{
	struct ifnet ifnet;
	long off;
        static int is_kfd_setup = 0;

        /*  setup() is called after each dormant->active
         *  transition.  Since we never close the kvm files
         *  (there's no reason), make sure we don't open them
         *  each time, as that can lead to exhaustion of all open
         *  files!  */
        if (!is_kfd_setup) {
                setup_kd_once();
                is_kfd_setup = 1;
	}

	if (kvm_nlist(kfd, nl) != 0) {
		syslog(LOG_ERR, "can't get namelist");
		exit (1);
        }

	if (kvm_read(kfd, (long)nl[X_IFNET].n_value, &ifnetq,
                     sizeof ifnetq) != sizeof ifnetq)  {
		syslog(LOG_ERR, "can't read ifnet queue head from kmem");
		exit(1);
        }

	numintfs = 0;
	for (off = (long)ifnetq.tqh_first; off;) {
		if (kvm_read(kfd, off, (char *)&ifnet, sizeof ifnet) !=
		    sizeof ifnet) {
			syslog(LOG_ERR, "can't read ifnet from kmem");
			exit(1);
		}
		numintfs++;
		off = (long)ifnet.if_list.tqe_next;
	}
	dkinit(0, getgid());
}

/*
 * returns true if have a disk
 */
int
havedisk()
{
	return dk_ndrive != 0;
}

void
rstat_service(struct svc_req *rqstp, SVCXPRT *transp)
{
	union {
		int fill;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	char *(*local) __P((void *, struct svc_req *));

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
		goto leave;

	case RSTATPROC_STATS:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_statstime;
                switch (rqstp->rq_vers) {
                case RSTATVERS_ORIG:
                        local = (char *(*) __P((void *, struct svc_req *)))
				rstatproc_stats_1_svc;
                        break;
                case RSTATVERS_SWTCH:
                        local = (char *(*) __P((void *, struct svc_req *)))
				rstatproc_stats_2_svc;
                        break;
                case RSTATVERS_TIME:
                        local = (char *(*) __P((void *, struct svc_req *)))
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
                        local = (char *(*) __P((void *, struct svc_req *)))
				rstatproc_havedisk_1_svc;
                        break;
                case RSTATVERS_SWTCH:
                        local = (char *(*) __P((void *, struct svc_req *)))
				rstatproc_havedisk_2_svc;
                        break;
                case RSTATVERS_TIME:
                        local = (char *(*) __P((void *, struct svc_req *)))
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
