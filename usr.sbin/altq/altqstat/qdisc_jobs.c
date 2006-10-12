/*	$NetBSD: qdisc_jobs.c,v 1.2 2006/10/12 19:59:13 peter Exp $	*/
/*	$KAME: qdisc_jobs.c,v 1.2 2002/10/27 03:19:36 kjc Exp $	*/
/*
 * Copyright (c) 2001-2002, by the Rector and Board of Visitors of the 
 * University of Virginia.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer. 
 *
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 *
 * Neither the name of the University of Virginia nor the names 
 * of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written 
 * permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*                                                                     
 * JoBS - altq prototype implementation                                
 *                                                                     
 * Author: Nicolas Christin <nicolas@cs.virginia.edu>
 *
 * JoBS algorithms originally devised and proposed by		       
 * Nicolas Christin and Jorg Liebeherr.
 * Grateful Acknowledgments to Tarek Abdelzaher for his help and       
 * comments, and to Kenjiro Cho for some helpful advice.
 * Contributed by the Multimedia Networks Group at the University
 * of Virginia. 
 *
 * http://qosbox.cs.virginia.edu
 *                                                                      
 */ 							               

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <altq/altq.h>
#include <altq/altq_jobs.h>

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

void
jobs_stat_loop(int fd, const char *ifname, int count, int interval)
{
	struct class_stats	stats1[JOBS_MAXPRI], stats2[JOBS_MAXPRI];
	char			clnames[JOBS_MAXPRI][128];
	struct jobs_class_stats	get_stats;
	struct class_stats	*sp, *lp, *new, *last, *tmp;
	struct timeval		cur_time, last_time;
	int			i;
	double			sec;
	int			cnt = count;
	sigset_t		omask;

	strlcpy(get_stats.iface.jobs_ifname, ifname,
		sizeof(get_stats.iface.jobs_ifname));
	new = &stats1[0];
	last = &stats2[0];

	/* invalidate class handles */
	for (i=0; i<JOBS_MAXPRI; i++)
		last[i].class_handle = JOBS_NULLCLASS_HANDLE;

	while (count == 0 || cnt-- > 0) {
		get_stats.stats = new;
		get_stats.maxpri = JOBS_MAXPRI;
		if (ioctl(fd, JOBS_GETSTATS, &get_stats) < 0)
			err(1, "ioctl JOBS_GETSTATS");

		gettimeofday(&cur_time, NULL);
		sec = calc_interval(&cur_time, &last_time);

		printf("\n%s:\n", ifname);
		printf("pri\tdel\trdc\tviol\tp_i\trlc\tthru\toff_ld\tbc_e\tavg_e\tstdev_e\twc_e\tbc_d\tavg_d\tstdev_d\twc_d\tnr_en\tnr_de\n");

		for (i = get_stats.maxpri; i >= 0; i--) {
			sp = &new[i];
			lp = &last[i];

			if (sp->class_handle == JOBS_NULLCLASS_HANDLE)
				continue;

			if (sp->class_handle != lp->class_handle) {
				quip_chandle2name(ifname, sp->class_handle,
				clnames[i], sizeof(clnames[0]));
				continue;
			}

			printf("%d\t%.0f\t%.2f\t%.3f\t%.2f\t%.2f\t%.3f\t%.2f\t%llu\t%.0f\t%.0f\t%llu\t%llu\t%.0f\t%.0f\t%llu\t%u\t%u\n",
			       i,
			       (sp->rout.packets == 0)?
			       -1.:(double)sp->avgdel/(double)sp->rout.packets,
			       (i > 0)?((sp->rout.packets > 0 && (&new[i-1])->rout.packets > 0)?
					(double)(sp->avgdel*(&new[i-1])->rout.packets)/((double)sp->rout.packets*(&new[i-1])->avgdel):0):-1.0,
			       (sp->arrival.packets == 0)?
			       0:(double)100.*sp->adc_violations/(double)sp->arrival.packets,
			       (sp->arrivalbusy.bytes == 0)?
			       0:(double)(100.*sp->dropcnt.bytes/(double)(sp->arrivalbusy.bytes)),
			       (i > 0)?(
					(sp->arrivalbusy.bytes == 0 || (&new[i-1])->dropcnt.bytes == 0)?
					0:(double)(sp->dropcnt.bytes*(&new[i-1])->arrivalbusy.bytes)
					/(double)(sp->arrivalbusy.bytes*(&new[i-1])->dropcnt.bytes)):-1.0,
			       (sp->rout.bytes > 0)?
			       (double)calc_rate(sp->rout.bytes,
						 0, sec)/1000000.:0,
			       (sp->arrival.bytes-lp->arrival.bytes > 0)?
			       (double)calc_rate(sp->arrival.bytes-lp->arrival.bytes,
						 0, sec)/1000000.:0,
			       (ull)sp->bc_cycles_enqueue,
			       (sp->total_enqueued == 0)?
			       -1:(double)sp->avg_cycles_enqueue/sp->total_enqueued,
			       (sp->total_enqueued == 0)?
			       -1.:
			       sqrt((double)((u_int)((sp->avg_cycles2_enqueue/sp->total_enqueued)
						     -(sp->avg_cycles_enqueue/sp->total_enqueued*sp->avg_cycles_enqueue/sp->total_enqueued)))),
			       (ull)sp->wc_cycles_enqueue,
			       (ull)sp->bc_cycles_dequeue,
			       (sp->total_dequeued == 0)?
			       -1:(double)sp->avg_cycles_dequeue/sp->total_dequeued,
			       (sp->total_dequeued == 0)?
			       -1.:
			       sqrt((double)((u_int)((sp->avg_cycles2_dequeue/sp->total_dequeued)
						     -(sp->avg_cycles_dequeue/sp->total_dequeued*sp->avg_cycles_dequeue/sp->total_dequeued)))),
			       (ull)sp->wc_cycles_dequeue,
			       (u_int)sp->total_enqueued,
			       (u_int)sp->total_dequeued
			       );
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
