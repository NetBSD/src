/*	$NetBSD: tprof_top.c,v 1.2 2022/12/01 03:32:24 ryo Exp $	*/

/*-
 * Copyright (c) 2022 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: tprof_top.c,v 1.2 2022/12/01 03:32:24 ryo Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/rbtree.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/tprof/tprof_ioctl.h>
#include "tprof.h"
#include "ksyms.h"

static struct sym **ksyms;
static size_t nksyms;
static sig_atomic_t sigalrm;
static struct winsize win;
static long top_interval = 1;
static u_int nevent;
static const char *eventname[TPROF_MAXCOUNTERS];
static int nontty;

/* XXX: use terminfo or curses */
static void
cursor_address(u_int x, u_int y)
{
	if (nontty)
		return;
	printf("\e[%u;%uH", y - 1, x - 1);
}

static void
cursor_home(void)
{
	if (nontty)
		return;
	printf("\e[H");
}

static void
cls_eol(void)
{
	if (nontty)
		return;
	printf("\e[K");
}

static void
cls_eos(void)
{
	if (nontty)
		return;
	printf("\e[J");
}

static void
sigwinch_handler(int signo)
{
	nontty = ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
}

static void
sigalrm_handler(int signo)
{
	sigalrm = 1;
}

struct sample_elm {
	struct rb_node node;
	uint64_t addr;
	const char *name;
	uint32_t flags;
#define SAMPLE_ELM_FLAGS_USER	0x00000001
	uint32_t num;
	uint32_t numcpu[];
};

static size_t sizeof_sample_elm;
static rb_tree_t rb_tree_sample;
static char *samplebuf;
static u_int sample_nused;
static u_int sample_kern_nsample;
static u_int sample_user_nsample;
static u_int sample_max = 1024 * 512;	/* XXX */
static uint32_t *sample_nsample_kern_per_cpu;
static uint32_t *sample_nsample_user_per_cpu;
static uint64_t *sample_nsample_per_event;
static uint64_t *sample_nsample_per_event_cpu;
static uint64_t collect_overflow;

static int opt_userland = 0;
static int opt_showcounter = 0;

static int
sample_compare_key(void *ctx, const void *n1, const void *keyp)
{
	const struct sample_elm *a1 = n1;
	const struct sample_elm *a2 = (const struct sample_elm *)keyp;
	return a1->addr - a2->addr;
}

static signed int
sample_compare_nodes(void *ctx, const void *n1, const void *n2)
{
	const struct addr *a2 = n2;
	return sample_compare_key(ctx, n1, a2);
}

static const rb_tree_ops_t sample_ops = {
	.rbto_compare_nodes = sample_compare_nodes,
	.rbto_compare_key = sample_compare_key
};

static void
sample_init(void)
{
	sizeof_sample_elm = sizeof(struct sample_elm) + sizeof(uint32_t) * ncpu;
}

static void
sample_reset(void)
{
	if (samplebuf == NULL) {
		samplebuf = emalloc(sizeof_sample_elm * sample_max);
		sample_nsample_kern_per_cpu = emalloc(sizeof(uint32_t) * ncpu);
		sample_nsample_user_per_cpu = emalloc(sizeof(uint32_t) * ncpu);
		sample_nsample_per_event = emalloc(sizeof(uint64_t) * nevent);
		sample_nsample_per_event_cpu =
		    emalloc(sizeof(uint64_t) * nevent * ncpu);
	}
	sample_nused = 0;
	sample_kern_nsample = 0;
	sample_user_nsample = 0;
	memset(sample_nsample_kern_per_cpu, 0, sizeof(uint32_t) * ncpu);
	memset(sample_nsample_user_per_cpu, 0, sizeof(uint32_t) * ncpu);
	memset(sample_nsample_per_event, 0, sizeof(uint64_t) * nevent);
	memset(sample_nsample_per_event_cpu, 0,
	    sizeof(uint64_t) * nevent * ncpu);

	rb_tree_init(&rb_tree_sample, &sample_ops);
}

static struct sample_elm *
sample_alloc(void)
{
	struct sample_elm *e;

	if (sample_nused >= sample_max) {
		errx(EXIT_FAILURE, "sample buffer overflow");
		return NULL;
	}

	e = (struct sample_elm *)(samplebuf + sizeof_sample_elm *
	    sample_nused++);
	memset(e, 0, sizeof_sample_elm);
	return e;
}

static void
sample_takeback(struct sample_elm *e)
{
	assert(sample_nused > 0);
	sample_nused--;
}

static int
sample_sortfunc(const void *a, const void *b)
{
	const struct sample_elm *ea = a;
	const struct sample_elm *eb = b;
	return eb->num - ea->num;
}

static void
sample_sort(void)
{
	qsort(samplebuf, sample_nused, sizeof_sample_elm, sample_sortfunc);
}

static void
sample_collect(tprof_sample_t *s)
{
	struct sample_elm *e, *o;
	const char *name;
	size_t symid;
	uint64_t addr, offset;
	uint32_t flags = 0;
	uint32_t eventid, cpuid;

	eventid = __SHIFTOUT(s->s_flags, TPROF_SAMPLE_COUNTER_MASK);
	cpuid = s->s_cpuid;

	sample_nsample_per_event[eventid]++;
	sample_nsample_per_event_cpu[nevent * cpuid + eventid]++;

	if ((s->s_flags & TPROF_SAMPLE_INKERNEL) == 0) {
		sample_user_nsample++;
		sample_nsample_user_per_cpu[cpuid]++;

		name = NULL;
		addr = s->s_pid;	/* XXX */
		flags |= SAMPLE_ELM_FLAGS_USER;

		if (!opt_userland)
			return;
	} else {
		sample_kern_nsample++;
		sample_nsample_kern_per_cpu[cpuid]++;

		name = ksymlookup(s->s_pc, &offset, &symid);
		if (name != NULL) {
			addr = ksyms[symid]->value;
		} else {
			addr = s->s_pc;
		}
	}

	e = sample_alloc();
	if (e == NULL) {
		fprintf(stderr, "cannot allocate sample\n");
		collect_overflow++;
		return;
	}

	e->addr = addr;
	e->name = name;
	e->flags = flags;
	e->num = 1;
	e->numcpu[cpuid] = 1;
	o = rb_tree_insert_node(&rb_tree_sample, e);
	if (o != e) {
		/* already exists */
		sample_takeback(e);
		o->num++;
		o->numcpu[cpuid]++;
	}
}

static void
show_tprof_stat(void)
{
	static struct tprof_stat tsbuf[2], *ts0, *ts;
	static u_int ts_i = 0;
	int ret;

	ts0 = &tsbuf[ts_i++ & 1];
	ts = &tsbuf[ts_i & 1];
	ret = ioctl(devfd, TPROF_IOC_GETSTAT, ts);
	if (ret == -1)
		err(EXIT_FAILURE, "TPROF_IOC_GETSTAT");

#define TS_PRINT(label, _m)				\
	do {						\
		printf(label "%" PRIu64, ts->_m);	\
		if (ts->_m != ts0->_m)			\
			printf("(+%"PRIu64")",		\
			    ts->_m - ts0->_m);		\
		printf(" ");				\
	} while (0)
	TS_PRINT("tprof sample:", ts_sample);
	TS_PRINT(" overflow:", ts_overflow);
	TS_PRINT(" buf:", ts_buf);
	TS_PRINT(" emptybuf:", ts_emptybuf);
	TS_PRINT(" dropbuf:", ts_dropbuf);
	TS_PRINT(" dropbuf_sample:", ts_dropbuf_sample);
}

static void
show_timestamp(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	cursor_address(win.ws_col - 7, 0);
	printf("%-8.8s", &(ctime((time_t *)&tv.tv_sec)[11]));
}


static uint64_t *counters;	/* counters[2][ncpu][nevent] */
static u_int counters_i;

static void
show_counters_alloc(void)
{
	size_t sz = 2 * ncpu * nevent * sizeof(uint64_t);
	counters = emalloc(sz);
	memset(counters, 0, sz);
}

static void
show_counters(void)
{
	tprof_counts_t countsbuf;
	uint64_t *cn[2], *c0, *c;
	u_int i;
	int n, ret;

	cn[0] = counters;
	cn[1] = counters + ncpu * nevent;
	c0 = cn[counters_i++ & 1];
	c = cn[counters_i & 1];

	for (n = 0; n < ncpu; n++) {
		countsbuf.c_cpu = n;
		ret = ioctl(devfd, TPROF_IOC_GETCOUNTS, &countsbuf);
		if (ret == -1)
			err(EXIT_FAILURE, "TPROF_IOC_GETCOUNTS");

		for (i = 0; i < nevent; i++)
			c[n * nevent + i] = countsbuf.c_count[i];
	}

	printf("%-22s", "Event counter (delta)");
	for (n = 0; n < ncpu; n++) {
		char cpuname[16];
		snprintf(cpuname, sizeof(cpuname), "CPU%u", n);
		printf("%11s", cpuname);
	}
	printf("\n");

	for (i = 0; i < nevent; i++) {
		printf("%-22.22s", eventname[i]);
		for (n = 0; n < ncpu; n++) {
			printf("%11"PRIu64,
			    c[n * nevent + i] - c0[n * nevent + i]);
		}
		printf("\n");
	}
	printf("\n");
}

static void
show_count_per_event(void)
{
	u_int i, nsample_total;
	int n;

	nsample_total = sample_kern_nsample + sample_user_nsample;

	for (i = 0; i < nevent; i++) {
		if (sample_nsample_per_event[i] >= nsample_total) {
			printf("%5.1f%%", sample_nsample_per_event[i] *
			    100.00 / nsample_total);
		} else {
			printf("%5.2f%%", sample_nsample_per_event[i] *
			    100.00 / nsample_total);
		}
		printf("%8"PRIu64" ", sample_nsample_per_event[i]);

		printf("%-32.32s", eventname[i]);
		for (n = 0; n < ncpu; n++) {
			printf("%6"PRIu64,
			    sample_nsample_per_event_cpu[nevent * n + i]);
		}
		printf("\n");
	}
}

static void
sample_show(void)
{
	static u_int nshow;

	struct sample_elm *e;
	u_int nsample_total;
	int i, n, ndisp;
	char namebuf[32];
	const char *name;

	int margin_lines = 7;

	margin_lines += 3 + nevent;	/* show_counter_per_event() */

	if (opt_showcounter)
		margin_lines += 2 + nevent;
	if (opt_userland)
		margin_lines += 1;

	ndisp = sample_nused;
	if (!nontty && ndisp > (win.ws_row - margin_lines))
		ndisp = win.ws_row - margin_lines;

	cursor_home();
	if (nshow++ == 0)
		cls_eos();


	show_tprof_stat();
	cls_eol();

	show_timestamp();
	printf("\n");
	printf("\n");

	if (opt_showcounter)
		show_counters();

	printf("  Rate Sample# Eventname                       ");
	for (n = 0; n < ncpu; n++) {
		if (n >= 1000) {
			snprintf(namebuf, sizeof(namebuf), "%d", n);
		} else if (n >= 100) {
			snprintf(namebuf, sizeof(namebuf), "#%d", n);
		} else {
			snprintf(namebuf, sizeof(namebuf), "CPU%d", n);
		}
		printf(" %5s", namebuf);
	}
	printf("\n");
	printf("------ ------- --------------------------------");
	for (n = 0; n < ncpu; n++) {
		printf(" -----");
	}
	printf("\n");

	show_count_per_event();
	printf("\n");

	printf("  Rate Sample# Symbol                          ");
	for (n = 0; n < ncpu; n++) {
		if (n >= 1000) {
			snprintf(namebuf, sizeof(namebuf), "%d", n);
		} else if (n >= 100) {
			snprintf(namebuf, sizeof(namebuf), "#%d", n);
		} else {
			snprintf(namebuf, sizeof(namebuf), "CPU%d", n);
		}
		printf(" %5s", namebuf);
	}
	printf("\n");
	printf("------ ------- --------------------------------");
	for (n = 0; n < ncpu; n++) {
		printf(" -----");
	}
	printf("\n");

	for (i = 0; i < ndisp; i++) {
		e = (struct sample_elm *)(samplebuf + sizeof_sample_elm * i);
		name = e->name;
		if (name == NULL) {
			if (e->flags & SAMPLE_ELM_FLAGS_USER) {
				snprintf(namebuf, sizeof(namebuf), "<PID:%"PRIu64">",
				    e->addr);
			} else {
				snprintf(namebuf, sizeof(namebuf), "0x%016"PRIx64,
				    e->addr);
			}
			name = namebuf;
		}

		nsample_total = sample_kern_nsample;
		if (opt_userland)
			nsample_total += sample_user_nsample;
		/*
		 * even when only kernel mode events are configured,
		 * interrupts may still occur in the user mode state.
		 */
		if (nsample_total == 0)
			nsample_total = 1;

		if (e->num >= nsample_total) {
			printf("%5.1f%%", e->num * 100.00 / nsample_total);
		} else {
			printf("%5.2f%%", e->num * 100.00 / nsample_total);
		}
		printf("%8u %-32.32s", e->num, name);

		for (n = 0; n < ncpu; n++) {
			if (e->numcpu[n] == 0)
				printf("     .");
			else
				printf("%6u", e->numcpu[n]);
		}
		printf("\n");
	}

	if ((u_int)ndisp != sample_nused) {
		printf("     :       : (more %u symbols omitted)\n",
		    sample_nused - ndisp);
	} else {
		for (i = ndisp; i <= win.ws_row - margin_lines; i++) {
			printf("~");
			cls_eol();
			printf("\n");
		}
	}


	printf("------ ------- --------------------------------");
	for (n = 0; n < ncpu; n++) {
		printf(" -----");
	}
	printf("\n");

	printf("Total %8u %-32.32s", sample_kern_nsample, "in-kernel");
	for (n = 0; n < ncpu; n++) {
		printf("%6u", sample_nsample_kern_per_cpu[n]);
	}

	if (opt_userland) {
		printf("\n");
		printf("      %8u %-32.32s", sample_user_nsample, "userland");
		for (n = 0; n < ncpu; n++) {
			printf("%6u", sample_nsample_user_per_cpu[n]);
		}
	}

	cls_eos();
}

__dead static void
tprof_top_usage(void)
{
	fprintf(stderr, "%s top [-e name[,scale] [-e ...]]"
	    " [-i interval] [-c] [-u]\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
parse_event_scale(tprof_param_t *param, const char *str)
{
	double d;
	uint64_t n;
	char *p;

	if (str[0] == '=') {
		str++;
		n = strtoull(str, &p, 0);
		if (*p != '\0')
			return -1;
		param->p_value2 = n;
		param->p_flags |= TPROF_PARAM_VALUE2_TRIGGERCOUNT;
	} else {
		if (strncasecmp("0x", str, 2) == 0)
			d = strtol(str, &p, 0);
		else
			d = strtod(str, &p);
		if (*p != '\0')
			return -1;
		param->p_value2 = 0x100000000ULL / d;
		param->p_flags |= TPROF_PARAM_VALUE2_SCALE;
	}
	return 0;
}

void
tprof_top(int argc, char **argv)
{
	tprof_param_t params[TPROF_MAXCOUNTERS];
	struct sigaction sa;
	struct itimerval it;
	ssize_t bufsize;
	u_int i;
	int ch, ret;
	char *buf, *tokens[2], *p;

	memset(params, 0, sizeof(params));
	nevent = 0;

	while ((ch = getopt(argc, argv, "ce:i:u")) != -1) {
		switch (ch) {
		case 'c':
			opt_showcounter = 1;
			break;
		case 'e':
			p = estrdup(optarg);
			tokens[0] = strtok(p, ",");
			tokens[1] = strtok(NULL, ",");
			tprof_event_lookup(tokens[0], &params[nevent]);
			if (tokens[1] != NULL) {
				if (parse_event_scale(&params[nevent], tokens[1]) != 0)
					errx(EXIT_FAILURE, "invalid scale: %s", tokens[1]);
			}
			eventname[nevent] = tokens[0];
			nevent++;
			if (nevent > __arraycount(params) ||
			    nevent > ncounters)
				errx(EXIT_FAILURE, "Too many events. Only a"
				    " maximum of %d counters can be used.",
				    ncounters);
			break;
		case 'i':
			top_interval = strtol(optarg, &p, 10);
			if (*p != '\0' || top_interval <= 0)
				errx(EXIT_FAILURE, "Bad/invalid interval: %s",
				    optarg);
			break;
		case 'u':
			opt_userland = 1;
			break;
		default:
			tprof_top_usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		tprof_top_usage();

	bufsize = sizeof(tprof_sample_t) * 8192;
	buf = emalloc(bufsize);

	sample_init();

	if (nevent == 0) {
		const char *defaultevent;

		switch (tprof_info.ti_ident) {
		case TPROF_IDENT_INTEL_GENERIC:
			defaultevent = "unhalted-core-cycles";
			break;
		case TPROF_IDENT_AMD_GENERIC:
			defaultevent = "LsNotHaltedCyc";
			break;
		case TPROF_IDENT_ARMV8_GENERIC:
		case TPROF_IDENT_ARMV7_GENERIC:
			defaultevent = "CPU_CYCLES";
			break;
		default:
			defaultevent = NULL;
			break;
		}
		if (defaultevent == NULL)
			errx(EXIT_FAILURE, "cpu not supported");

		tprof_event_lookup(defaultevent, &params[nevent]);
		eventname[nevent] = defaultevent;
		nevent++;
	}

	show_counters_alloc();

	for (i = 0; i < nevent; i++) {
		params[i].p_counter = i;
		params[i].p_flags |= TPROF_PARAM_KERN | TPROF_PARAM_PROFILE;
		if (opt_userland)
			params[i].p_flags |= TPROF_PARAM_USER;
		ret = ioctl(devfd, TPROF_IOC_CONFIGURE_EVENT, &params[i]);
		if (ret == -1)
			err(EXIT_FAILURE, "TPROF_IOC_CONFIGURE_EVENT: %s",
			    eventname[i]);
	}

	tprof_countermask_t mask = TPROF_COUNTERMASK_ALL;
	ret = ioctl(devfd, TPROF_IOC_START, &mask);
	if (ret == -1)
		err(EXIT_FAILURE, "TPROF_IOC_START");


	sigwinch_handler(0);
	ksyms = ksymload(&nksyms);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sigalrm_handler;
	sigaction(SIGALRM, &sa, NULL);

	it.it_interval.tv_sec = it.it_value.tv_sec = top_interval;
	it.it_interval.tv_usec = it.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);

	sample_reset();
	printf("collecting samples...");
	fflush(stdout);

	do {
		/* continue to accumulate tprof_sample until alarm arrives */
		while (sigalrm == 0) {
			ssize_t len = read(devfd, buf, bufsize);
			if (len == -1 && errno != EINTR)
				err(EXIT_FAILURE, "read");
			if (len > 0) {
				tprof_sample_t *s = (tprof_sample_t *)buf;
				while (s < (tprof_sample_t *)(buf + len)) {
					sample_collect(s);
					s++;
				}
			}
		}
		sigalrm = 0;

		/* update screen */
		sample_sort();
		sample_show();
		fflush(stdout);

		sample_reset();
	} while (!nontty);

	printf("\n");
}
