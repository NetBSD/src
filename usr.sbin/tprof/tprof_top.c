/*	$NetBSD: tprof_top.c,v 1.3 2022/12/09 01:55:46 ryo Exp $	*/

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
__RCSID("$NetBSD: tprof_top.c,v 1.3 2022/12/09 01:55:46 ryo Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/rbtree.h>
#include <sys/ioctl.h>
#include <sys/time.h>

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

#define SAMPLE_MODE_ACCUMULATIVE	0
#define SAMPLE_MODE_INSTANTANEOUS	1
#define SAMPLE_MODE_NUM			2

struct sample_elm {
	struct rb_node node;
	uint64_t addr;
	const char *name;
	uint32_t flags;
#define SAMPLE_ELM_FLAGS_USER	0x00000001
	uint32_t num[SAMPLE_MODE_NUM];
	uint32_t num_cpu[];	/* [SAMPLE_MODE_NUM][ncpu] */
#define SAMPLE_ELM_NUM_CPU(e, k)		\
	((e)->num_cpu + (k) *  ncpu)
};

struct ptrarray {
	void **pa_ptrs;
	size_t pa_allocnum;
	size_t pa_inuse;
};

static int opt_mode = SAMPLE_MODE_INSTANTANEOUS;
static int opt_userland = 0;
static int opt_showcounter = 0;

/* for display */
static struct winsize win;
static int nontty;
static long top_interval = 1;

/* for profiling and counting samples */
static sig_atomic_t sigalrm;
static struct sym **ksyms;
static size_t nksyms;
static u_int nevent;
static const char *eventname[TPROF_MAXCOUNTERS];
static size_t sizeof_sample_elm;
static rb_tree_t rb_tree_sample;
struct ptrarray sample_list[SAMPLE_MODE_NUM];
static u_int sample_n_kern[SAMPLE_MODE_NUM];
static u_int sample_n_user[SAMPLE_MODE_NUM];
static uint32_t *sample_n_kern_per_cpu[SAMPLE_MODE_NUM];	/* [ncpu] */
static uint32_t *sample_n_user_per_cpu[SAMPLE_MODE_NUM];	/* [ncpu] */
static uint64_t *sample_n_per_event[SAMPLE_MODE_NUM];		/* [nevent] */
static uint64_t *sample_n_per_event_cpu[SAMPLE_MODE_NUM];	/* [ncpu] */

/* raw event counter */
static uint64_t *counters;	/* counters[2][ncpu][nevent] */
static u_int counters_i;

static const char *
cycle_event_name(void)
{
	const char *cycleevent;

	switch (tprof_info.ti_ident) {
	case TPROF_IDENT_INTEL_GENERIC:
		cycleevent = "unhalted-core-cycles";
		break;
	case TPROF_IDENT_AMD_GENERIC:
		cycleevent = "LsNotHaltedCyc";
		break;
	case TPROF_IDENT_ARMV8_GENERIC:
	case TPROF_IDENT_ARMV7_GENERIC:
		cycleevent = "CPU_CYCLES";
		break;
	default:
		cycleevent = NULL;
		break;
	}
	return cycleevent;
}

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

static void
ptrarray_push(struct ptrarray *ptrarray, void *ptr)
{
	int error;

	if (ptrarray->pa_inuse >= ptrarray->pa_allocnum) {
		/* increase buffer */
		ptrarray->pa_allocnum += 1024;
		error = reallocarr(&ptrarray->pa_ptrs, ptrarray->pa_allocnum,
		    sizeof(*ptrarray->pa_ptrs));
		if (error != 0)
			errc(EXIT_FAILURE, error, "rellocarr failed");
	}
	ptrarray->pa_ptrs[ptrarray->pa_inuse++] = ptr;
}

static void
ptrarray_iterate(struct ptrarray *ptrarray, void (*ifunc)(void *))
{
	size_t i;

	for (i = 0; i < ptrarray->pa_inuse; i++) {
		(*ifunc)(ptrarray->pa_ptrs[i]);
	}
}

static void
ptrarray_clear(struct ptrarray *ptrarray)
{
	ptrarray->pa_inuse = 0;
}

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

static u_int
n_align(u_int n, u_int align)
{
	return (n + align - 1) / align * align;
}

static void
sample_init(void)
{
	const struct sample_elm *e;
	int mode;

	u_int size = sizeof(struct sample_elm) +
	    sizeof(e->num_cpu[0]) * SAMPLE_MODE_NUM * ncpu;
	sizeof_sample_elm = n_align(size, __alignof(struct sample_elm));

	for (mode = 0; mode < SAMPLE_MODE_NUM; mode++) {
		sample_n_kern_per_cpu[mode] = ecalloc(1,
		    sizeof(typeof(*sample_n_kern_per_cpu[mode])) * ncpu);
		sample_n_user_per_cpu[mode] = ecalloc(1,
		    sizeof(typeof(*sample_n_user_per_cpu[mode])) * ncpu);
		sample_n_per_event[mode] = ecalloc(1,
		    sizeof(typeof(*sample_n_per_event[mode])) * nevent);
		sample_n_per_event_cpu[mode] = ecalloc(1,
		    sizeof(typeof(*sample_n_per_event_cpu[mode])) *
		    nevent * ncpu);
	}
}

static void
sample_clear_instantaneous(void *arg)
{
	struct sample_elm *e = (void *)arg;

	e->num[SAMPLE_MODE_INSTANTANEOUS] = 0;
	memset(SAMPLE_ELM_NUM_CPU(e, SAMPLE_MODE_INSTANTANEOUS),
	    0, sizeof(e->num_cpu[0]) * ncpu);
}

static void
sample_reset(bool reset_accumulative)
{
	int mode;

	for (mode = 0; mode < SAMPLE_MODE_NUM; mode++) {
		if (mode == SAMPLE_MODE_ACCUMULATIVE && !reset_accumulative)
			continue;

		sample_n_kern[mode] = 0;
		sample_n_user[mode] = 0;
		memset(sample_n_kern_per_cpu[mode], 0,
		    sizeof(typeof(*sample_n_kern_per_cpu[mode])) * ncpu);
		memset(sample_n_user_per_cpu[mode], 0,
		    sizeof(typeof(*sample_n_user_per_cpu[mode])) * ncpu);
		memset(sample_n_per_event[mode], 0,
		    sizeof(typeof(*sample_n_per_event[mode])) * nevent);
		memset(sample_n_per_event_cpu[mode], 0,
		    sizeof(typeof(*sample_n_per_event_cpu[mode])) *
		    nevent * ncpu);
	}

	if (reset_accumulative) {
		rb_tree_init(&rb_tree_sample, &sample_ops);
		ptrarray_iterate(&sample_list[SAMPLE_MODE_ACCUMULATIVE], free);
		ptrarray_clear(&sample_list[SAMPLE_MODE_ACCUMULATIVE]);
		ptrarray_clear(&sample_list[SAMPLE_MODE_INSTANTANEOUS]);
	} else {
		ptrarray_iterate(&sample_list[SAMPLE_MODE_INSTANTANEOUS],
		    sample_clear_instantaneous);
		ptrarray_clear(&sample_list[SAMPLE_MODE_INSTANTANEOUS]);
	}
}

static int __unused
sample_sortfunc_accumulative(const void *a, const void *b)
{
	struct sample_elm * const *ea = a;
	struct sample_elm * const *eb = b;
	return (*eb)->num[SAMPLE_MODE_ACCUMULATIVE] -
	    (*ea)->num[SAMPLE_MODE_ACCUMULATIVE];
}

static int
sample_sortfunc_instantaneous(const void *a, const void *b)
{
	struct sample_elm * const *ea = a;
	struct sample_elm * const *eb = b;
	return (*eb)->num[SAMPLE_MODE_INSTANTANEOUS] -
	    (*ea)->num[SAMPLE_MODE_INSTANTANEOUS];
}

static void
sample_sort_accumulative(void)
{
	qsort(sample_list[SAMPLE_MODE_ACCUMULATIVE].pa_ptrs,
	    sample_list[SAMPLE_MODE_ACCUMULATIVE].pa_inuse,
	    sizeof(struct sample_elm *), sample_sortfunc_accumulative);
}

static void
sample_sort_instantaneous(void)
{
	qsort(sample_list[SAMPLE_MODE_INSTANTANEOUS].pa_ptrs,
	    sample_list[SAMPLE_MODE_INSTANTANEOUS].pa_inuse,
	    sizeof(struct sample_elm *), sample_sortfunc_instantaneous);
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
	int mode;

	eventid = __SHIFTOUT(s->s_flags, TPROF_SAMPLE_COUNTER_MASK);
	cpuid = s->s_cpuid;

	if (eventid >= nevent)	/* unknown event from tprof? */
		return;

	for (mode = 0; mode < SAMPLE_MODE_NUM; mode++) {
		sample_n_per_event[mode][eventid]++;
		sample_n_per_event_cpu[mode][nevent * cpuid + eventid]++;
	}

	if ((s->s_flags & TPROF_SAMPLE_INKERNEL) == 0) {
		sample_n_user[SAMPLE_MODE_ACCUMULATIVE]++;
		sample_n_user[SAMPLE_MODE_INSTANTANEOUS]++;
		sample_n_user_per_cpu[SAMPLE_MODE_ACCUMULATIVE][cpuid]++;
		sample_n_user_per_cpu[SAMPLE_MODE_INSTANTANEOUS][cpuid]++;

		name = NULL;
		addr = s->s_pid;	/* XXX */
		flags |= SAMPLE_ELM_FLAGS_USER;

		if (!opt_userland)
			return;
	} else {
		sample_n_kern[SAMPLE_MODE_ACCUMULATIVE]++;
		sample_n_kern[SAMPLE_MODE_INSTANTANEOUS]++;
		sample_n_kern_per_cpu[SAMPLE_MODE_ACCUMULATIVE][cpuid]++;
		sample_n_kern_per_cpu[SAMPLE_MODE_INSTANTANEOUS][cpuid]++;

		name = ksymlookup(s->s_pc, &offset, &symid);
		if (name != NULL) {
			addr = ksyms[symid]->value;
		} else {
			addr = s->s_pc;
		}
	}

	e = ecalloc(1, sizeof_sample_elm);
	e->addr = addr;
	e->name = name;
	e->flags = flags;
	e->num[SAMPLE_MODE_ACCUMULATIVE] = 1;
	e->num[SAMPLE_MODE_INSTANTANEOUS] = 1;
	SAMPLE_ELM_NUM_CPU(e, SAMPLE_MODE_ACCUMULATIVE)[cpuid] = 1;
	SAMPLE_ELM_NUM_CPU(e, SAMPLE_MODE_INSTANTANEOUS)[cpuid] = 1;
	o = rb_tree_insert_node(&rb_tree_sample, e);
	if (o == e) {
		/* new symbol. add to list for sort */
		ptrarray_push(&sample_list[SAMPLE_MODE_ACCUMULATIVE], o);
		ptrarray_push(&sample_list[SAMPLE_MODE_INSTANTANEOUS], o);
	} else {
		/* already exists */
		free(e);

		o->num[SAMPLE_MODE_ACCUMULATIVE]++;
		if (o->num[SAMPLE_MODE_INSTANTANEOUS]++ == 0) {
			/* new instantaneous symbols. add to list for sort */
			ptrarray_push(&sample_list[SAMPLE_MODE_INSTANTANEOUS],
			    o);
		}
		SAMPLE_ELM_NUM_CPU(o, SAMPLE_MODE_ACCUMULATIVE)[cpuid]++;
		SAMPLE_ELM_NUM_CPU(o, SAMPLE_MODE_INSTANTANEOUS)[cpuid]++;
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

static void
show_counters_alloc(void)
{
	size_t sz = 2 * ncpu * nevent * sizeof(*counters);
	counters = ecalloc(1, sz);
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

	nsample_total = sample_n_kern[opt_mode] + sample_n_user[opt_mode];

	for (i = 0; i < nevent; i++) {
		if (sample_n_per_event[opt_mode][i] >= nsample_total) {
			printf("%5.1f%%", sample_n_per_event[opt_mode][i] *
			    100.00 / nsample_total);
		} else {
			printf("%5.2f%%", sample_n_per_event[opt_mode][i] *
			    100.00 / nsample_total);
		}
		printf("%8"PRIu64" ", sample_n_per_event[opt_mode][i]);

		printf("%-32.32s", eventname[i]);
		for (n = 0; n < ncpu; n++) {
			printf("%6"PRIu64,
			    sample_n_per_event_cpu[opt_mode][nevent * n + i]);
		}
		printf("\n");
	}
}

static void
sample_show(void)
{
	static u_int nshow;

	struct sample_elm *e;
	struct ptrarray *samples;
	u_int nsample_total;
	int i, n, ndisp;
	char namebuf[32];
	const char *name;

	int margin_lines = 7;

	margin_lines += 3 + nevent;	/* show_counter_per_event() */

	if (opt_mode == SAMPLE_MODE_INSTANTANEOUS)
		sample_sort_instantaneous();
	else
		sample_sort_accumulative();
	samples = &sample_list[opt_mode];

	if (opt_showcounter)
		margin_lines += 2 + nevent;
	if (opt_userland)
		margin_lines += 1;

	ndisp = samples->pa_inuse;
	if (!nontty && ndisp > (win.ws_row - margin_lines))
		ndisp = win.ws_row - margin_lines;

	cursor_home();
	if (nshow++ == 0)
		cls_eos();


	if (opt_mode == SAMPLE_MODE_ACCUMULATIVE)
		printf("[Accumulative mode] ");

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
		e = (struct sample_elm *)samples->pa_ptrs[i];
		name = e->name;
		if (name == NULL) {
			if (e->flags & SAMPLE_ELM_FLAGS_USER) {
				snprintf(namebuf, sizeof(namebuf),
				    "<PID:%"PRIu64">", e->addr);
			} else {
				snprintf(namebuf, sizeof(namebuf),
				    "0x%016"PRIx64, e->addr);
			}
			name = namebuf;
		}

		nsample_total = sample_n_kern[opt_mode];
		if (opt_userland)
			nsample_total += sample_n_user[opt_mode];
		/*
		 * even when only kernel mode events are configured,
		 * interrupts may still occur in the user mode state.
		 */
		if (nsample_total == 0)
			nsample_total = 1;

		if (e->num[opt_mode] >= nsample_total) {
			printf("%5.1f%%",
			    e->num[opt_mode] * 100.00 / nsample_total);
		} else {
			printf("%5.2f%%",
			    e->num[opt_mode] * 100.00 / nsample_total);
		}
		printf("%8u %-32.32s", e->num[opt_mode], name);

		for (n = 0; n < ncpu; n++) {
			if (SAMPLE_ELM_NUM_CPU(e, opt_mode)[n] == 0) {
				printf("     .");
			} else {
				printf("%6u",
				    SAMPLE_ELM_NUM_CPU(e, opt_mode)[n]);
			}
		}
		printf("\n");
	}

	if ((u_int)ndisp != samples->pa_inuse) {
		printf("     :       : (more %zu symbols omitted)\n",
		    samples->pa_inuse - ndisp);
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

	printf("Total %8u %-32.32s", sample_n_kern[opt_mode], "in-kernel");
	for (n = 0; n < ncpu; n++) {
		printf("%6u", sample_n_kern_per_cpu[opt_mode][n]);
	}

	if (opt_userland) {
		printf("\n");
		printf("      %8u %-32.32s",
		    sample_n_user[opt_mode], "userland");
		for (n = 0; n < ncpu; n++) {
			printf("%6u", sample_n_user_per_cpu[opt_mode][n]);
		}
	}

	cls_eos();
}

__dead static void
tprof_top_usage(void)
{
	fprintf(stderr, "%s top [-acu] [-e name[,scale] [-e ...]]"
	    " [-i interval]\n", getprogname());
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
	ssize_t tprof_bufsize;
	u_int i;
	int ch, ret;
	char *tprof_buf, *tokens[2], *p;

	memset(params, 0, sizeof(params));
	nevent = 0;

	while ((ch = getopt(argc, argv, "ace:i:u")) != -1) {
		switch (ch) {
		case 'a':
			opt_mode = SAMPLE_MODE_ACCUMULATIVE;
			break;
		case 'c':
			opt_showcounter = 1;
			break;
		case 'e':
			p = estrdup(optarg);
			tokens[0] = strtok(p, ",");
			tokens[1] = strtok(NULL, ",");
			tprof_event_lookup(tokens[0], &params[nevent]);
			if (tokens[1] != NULL) {
				if (parse_event_scale(&params[nevent],
				    tokens[1]) != 0) {
					errx(EXIT_FAILURE, "invalid scale: %s",
					    tokens[1]);
				}
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

	if (nevent == 0) {
		const char *defaultevent = cycle_event_name();
		if (defaultevent == NULL)
			errx(EXIT_FAILURE, "cpu not supported");

		tprof_event_lookup(defaultevent, &params[nevent]);
		eventname[nevent] = defaultevent;
		nevent++;
	}

	sample_init();
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

	sample_reset(true);
	printf("collecting samples...");
	fflush(stdout);

	tprof_bufsize = sizeof(tprof_sample_t) * 8192;
	tprof_buf = emalloc(tprof_bufsize);
	do {
		/* continue to accumulate tprof_sample until alarm arrives */
		while (sigalrm == 0) {
			ssize_t len = read(devfd, tprof_buf, tprof_bufsize);
			if (len == -1 && errno != EINTR)
				err(EXIT_FAILURE, "read");
			if (len > 0) {
				tprof_sample_t *s = (tprof_sample_t *)tprof_buf;
				while (s < (tprof_sample_t *)(tprof_buf + len))
					sample_collect(s++);
			}
		}
		sigalrm = 0;

		/* update screen */
		sample_show();
		fflush(stdout);

		sample_reset(false);
	} while (!nontty);

	printf("\n");
}
