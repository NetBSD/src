/*	$NetBSD: qdisc_hfsc.c,v 1.5 2006/10/12 19:59:13 peter Exp $	*/
/*	$KAME: qdisc_hfsc.c,v 1.8 2003/07/10 12:09:38 kjc Exp $	*/
/*
 * Copyright (C) 1999-2000
 *	Sony Computer Science Laboratories, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <altq/altq.h>
#include <altq/altq_hfsc.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <err.h>

#include "quip_client.h"
#include "altqstat.h"

#define NCLASSES	64

void
hfsc_stat_loop(int fd, const char *ifname, int count, int interval)
{
	struct hfsc_classstats	stats1[NCLASSES], stats2[NCLASSES];
	char			clnames[NCLASSES][128];
	struct hfsc_class_stats	get_stats;
	struct hfsc_classstats	*sp, *lp, *new, *last, *tmp;
	struct timeval		cur_time, last_time;
	int			i;
	double			sec;
	int			cnt = count;
	sigset_t		omask;
	u_int64_t		stattime;
	u_int32_t		machclk_freq;
	
	strlcpy(get_stats.iface.hfsc_ifname, ifname,
		sizeof(get_stats.iface.hfsc_ifname));
	new = &stats1[0];
	last = &stats2[0];

	/* invalidate class ids */
	for (i=0; i<NCLASSES; i++)
		last[i].class_id = 999999; /* XXX */

	while (count == 0 || cnt-- > 0) {
		get_stats.nskip = 0;
		get_stats.nclasses = NCLASSES;
		get_stats.stats = new;
	
		if (ioctl(fd, HFSC_GETSTATS, &get_stats) < 0)
			err(1, "ioctl HFSC_GETSTATS");

		gettimeofday(&cur_time, NULL);
		sec = calc_interval(&cur_time, &last_time);

		stattime = get_stats.cur_time;
		machclk_freq = get_stats.machclk_freq;
		printf("\ncur_time:%#llx %u classes %u packets in the tree\n",
		    (ull)stattime,
		    get_stats.hif_classes, get_stats.hif_packets);

		for (i=0; i<get_stats.nclasses; i++) {
			sp = &new[i];
			lp = &last[i];

			if (sp->class_id != lp->class_id) {
				quip_chandle2name(ifname, sp->class_handle,
						  clnames[i], sizeof(clnames[0]));
				continue;
			}

			printf("[%2d %s] handle:%#x [rt %s %ums %s][ls %s %ums %s]",
			       sp->class_id, clnames[i], sp->class_handle,
			       rate2str((double)sp->rsc.m1), sp->rsc.d,
			       rate2str((double)sp->rsc.m2),
			       rate2str((double)sp->fsc.m1), sp->fsc.d,
			       rate2str((double)sp->fsc.m2));
			if (sp->usc.m1 != 0 || sp->usc.m2 != 0)
				printf("[ul %s %ums %s]\n",
				       rate2str((double)sp->usc.m1), sp->usc.d,
				       rate2str((double)sp->usc.m2));
			else
				 printf("\n");
			printf("  measured: %sbps [rt:%s ls:%s] qlen:%2d period:%u\n",
			       rate2str(calc_rate(sp->total, lp->total, sec)),
			       rate2str(calc_rate(sp->cumul, lp->cumul, sec)),
			       rate2str(calc_rate(sp->total - sp->cumul,
						  lp->total - lp->cumul, sec)),
			       sp->qlength, sp->period);
			printf("     packets:%llu (%llu bytes) drops:%llu (%llu bytes) \n",
			       (ull)sp->xmit_cnt.packets,
			       (ull)sp->xmit_cnt.bytes,
			       (ull)sp->drop_cnt.packets,
			       (ull)sp->drop_cnt.bytes);
			printf("     cumul:%#llx total:%#llx\n",
			       (ull)sp->cumul, (ull)sp->total);
			printf("     e:%.2fus d:%.2fus f:%.2fus vt:%#llx\n",
			    sp->e == 0 ? 0 : ((double)sp->e - (double)stattime)
			    		     / machclk_freq * 1000000,
			    sp->d == 0 ? 0 : ((double)sp->d - (double)stattime)
			    		     / machclk_freq * 1000000,
			    sp->f == 0 ? 0 : ((double)sp->f - (double)stattime)
			    		     / machclk_freq * 1000000,
			    (ull)sp->vt);
			printf("     vtperiod:%u parentperiod:%u nactive:%d\n",
			    sp->vtperiod, sp->parentperiod, sp->nactive);
			printf("     initvt:%#llx cvtmax:%#llx vtoff:%#llx\n",
			    (ull)sp->initvt, (ull)sp->cvtmax, (ull)sp->vtoff);

			printf("     myf:%#llx cfmin:%#llx cvtmin:%#llx",
			    (ull)sp->myf, (ull)sp->cfmin, (ull)sp->cvtmin);
			printf(" myfadj:%#llx vtadj:%#llx\n",
			    (ull)sp->myfadj, (ull)sp->vtadj);
			if (sp->qtype == Q_RED)
				print_redstats(sp->red);
			else if (sp->qtype == Q_RIO)
				print_riostats(sp->red);
		}

		/* swap the buffer pointers */
		tmp = last;
		last = new;
		new = tmp;

		last_time = cur_time;

		/* wait for alarm signal */
		if (sigprocmask(SIG_BLOCK, NULL, &omask) == 0)
			sigsuspend(&omask);
	}
}
