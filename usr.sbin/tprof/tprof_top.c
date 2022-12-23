/*	$NetBSD: tprof_top.c,v 1.8 2022/12/23 19:37:06 christos Exp $	*/

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
__RCSID("$NetBSD: tprof_top.c,v 1.8 2022/12/23 19:37:06 christos Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/rbtree.h>
#include <sys/select.h>
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
#include <term.h>
#include <termios.h>
#include <unistd.h>
#include <util.h>

#include <dev/tprof/tprof_ioctl.h>
#include "tprof.h"
#include "ksyms.h"

#define SAMPLE_MODE_ACCUMULATIVE	0
#define SAMPLE_MODE_INSTANTANEOUS	1
#define SAMPLE_MODE_NUM			2

#define LINESTR	"-------------------------------------------------------------"
#define SYMBOL_LEN			32	/* symbol and event name */

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
static char *term;
static struct winsize win;
static int nontty;
static struct termios termios_save;
static bool termios_saved;
static long top_interval = 1;
static bool do_redraw;
static u_int nshow;

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
static u_int sample_event_width = 7;
static u_int *sample_cpu_width;					/* [ncpu] */
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

static void
reset_cursor_pos(void)
{
	int i;
	char *p;

	if (nontty || term == NULL)
		return;

	printf("\r");

	/* cursor_up * n */
	if ((p = tigetstr("cuu")) != NULL) {
		putp(tparm(p, win.ws_row - 1, 0, 0, 0, 0, 0, 0, 0, 0));
	} else if ((p = tigetstr("cuu1")) != NULL) {
		for (i = win.ws_row - 1; i > 0; i--)
			putp(p);
	}
}

static void
clr_to_eol(void)
{
	char *p;

	if (nontty || term == NULL)
		return;

	if ((p = tigetstr("el")) != NULL)
		putp(p);
}

/* newline, and clearing to end of line if needed */
static void
lim_newline(int *lim)
{
	if (*lim >= 1)
		clr_to_eol();

	printf("\n");
	*lim = win.ws_col;
}

static int
lim_printf(int *lim, const char *fmt, ...)
{
	va_list ap;
	size_t written;
	char *p;

	if (*lim <= 0)
		return 0;

	p = malloc(*lim + 1);
	if (p == NULL)
		return -1;

	va_start(ap, fmt);
	vsnprintf(p, *lim + 1, fmt, ap);
	va_end(ap);

	written = strlen(p);
	if (written == 0) {
		free(p);
		*lim = 0;
		return 0;
	}

	fwrite(p, written, 1, stdout);
	*lim -= written;

	free(p);
	return written;
}

static void
sigwinch_handler(int signo)
{
	char *p;

	win.ws_col = tigetnum("lines");
	win.ws_row = tigetnum("cols");

	nontty = ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
	if (nontty != 0) {
		nontty = !isatty(STDOUT_FILENO);
		win.ws_col = 65535;
		win.ws_row = 65535;
	}

	if ((p = getenv("LINES")) != NULL)
		win.ws_row = strtoul(p, NULL, 0);
	if ((p = getenv("COLUMNS")) != NULL)
		win.ws_col = strtoul(p, NULL, 0);

	do_redraw = true;
}

static void
tty_setup(void)
{
	struct termios termios;

	term = getenv("TERM");
	if (term != NULL)
		setupterm(term, 0, NULL);

	sigwinch_handler(0);

	if (tcgetattr(STDOUT_FILENO, &termios_save) == 0) {
		termios_saved = true;

		/* stty cbreak */
		termios = termios_save;
		termios.c_iflag |= BRKINT|IXON|IMAXBEL;
		termios.c_oflag |= OPOST;
		termios.c_lflag |= ISIG|IEXTEN;
		termios.c_lflag &= ~(ICANON|ECHO);
		tcsetattr(STDOUT_FILENO, TCSADRAIN, &termios);
	}
}

static void
tty_restore(void)
{
	if (termios_saved) {
		tcsetattr(STDOUT_FILENO, TCSADRAIN, &termios_save);
		termios_saved = false;
	}
}

static void
sigtstp_handler(int signo)
{
	tty_restore();

	signal(SIGWINCH, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);
	nshow = 0;
}

static void
sigalrm_handler(int signo)
{
	sigalrm = 1;
}

__dead static void
die(int signo)
{
	tty_restore();
	printf("\n");

	exit(EXIT_SUCCESS);
}

__dead static void
die_errc(int status, int code, const char *fmt, ...)
{
	va_list ap;

	tty_restore();

	va_start(ap, fmt);
	if (code == 0)
		verrx(status, fmt, ap);
	else
		verrc(status, code, fmt, ap);
	va_end(ap);
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
			die_errc(EXIT_FAILURE, error, "rellocarr failed");
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
	int l, mode, n;
	u_int size;
	char buf[16];

	size = sizeof(struct sample_elm) +
	    sizeof(e->num_cpu[0]) * SAMPLE_MODE_NUM * ncpu;
	sizeof_sample_elm = n_align(size, __alignof(struct sample_elm));

	sample_cpu_width = ecalloc(1, sizeof(*sample_cpu_width) * ncpu);
	for (n = 0; n < ncpu; n++) {
		sample_cpu_width[n] = 5;
		l = snprintf(buf, sizeof(buf), "CPU%d", n);
		if (sample_cpu_width[n] < (u_int)l)
			sample_cpu_width[n] = l;
	}

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
show_tprof_stat(int *lim)
{
	static struct tprof_stat tsbuf[2], *ts0, *ts;
	static u_int ts_i = 0;
	static int tprofstat_width[6];
	int ret, l;
	char tmpbuf[128];

	ts0 = &tsbuf[ts_i++ & 1];
	ts = &tsbuf[ts_i & 1];
	ret = ioctl(devfd, TPROF_IOC_GETSTAT, ts);
	if (ret == -1)
		die_errc(EXIT_FAILURE, errno, "TPROF_IOC_GETSTAT");

#define TS_PRINT(idx, label, _m)					\
	do {								\
		__CTASSERT(idx < __arraycount(tprofstat_width));	\
		lim_printf(lim, "%s", label);			\
		l = snprintf(tmpbuf, sizeof(tmpbuf), "%"PRIu64, ts->_m);\
		if (ts->_m != ts0->_m)					\
			l += snprintf(tmpbuf + l, sizeof(tmpbuf) - l,	\
			    "(+%"PRIu64")", ts->_m - ts0->_m);		\
		assert(l < (int)sizeof(tmpbuf));			\
		if (tprofstat_width[idx] < l)				\
			tprofstat_width[idx] = l;			\
		lim_printf(lim, "%-*.*s  ", tprofstat_width[idx],	\
		    tprofstat_width[idx], tmpbuf);			\
	} while (0)
	lim_printf(lim, "tprof ");
	TS_PRINT(0, "sample:", ts_sample);
	TS_PRINT(1, "overflow:", ts_overflow);
	TS_PRINT(2, "buf:", ts_buf);
	TS_PRINT(3, "emptybuf:", ts_emptybuf);
	TS_PRINT(4, "dropbuf:", ts_dropbuf);
	TS_PRINT(5, "dropbuf_sample:", ts_dropbuf_sample);
}

static void
show_timestamp(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	printf("%-8.8s", &(ctime((time_t *)&tv.tv_sec)[11]));
}

static void
show_counters_alloc(void)
{
	size_t sz = 2 * ncpu * nevent * sizeof(*counters);
	counters = ecalloc(1, sz);
}

static void
show_counters(int *lim)
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
			die_errc(EXIT_FAILURE, errno, "TPROF_IOC_GETCOUNTS");

		for (i = 0; i < nevent; i++)
			c[n * nevent + i] = countsbuf.c_count[i];
	}

	if (do_redraw) {
		lim_printf(lim, "%-22s", "Event counter (delta)");
		for (n = 0; n < ncpu; n++) {
			char cpuname[16];
			snprintf(cpuname, sizeof(cpuname), "CPU%u", n);
			lim_printf(lim, "%11s", cpuname);
		}
		lim_newline(lim);
	} else {
		printf("\n");
	}

	for (i = 0; i < nevent; i++) {
		lim_printf(lim, "%-22.22s", eventname[i]);
		for (n = 0; n < ncpu; n++) {
			lim_printf(lim, "%11"PRIu64,
			    c[n * nevent + i] - c0[n * nevent + i]);
		}
		lim_newline(lim);
	}
	lim_newline(lim);
}

static void
show_count_per_event(int *lim)
{
	u_int i, nsample_total;
	int n, l;
	char buf[32];

	nsample_total = sample_n_kern[opt_mode] + sample_n_user[opt_mode];
	if (nsample_total == 0)
		nsample_total = 1;

	/* calc width in advance */
	for (i = 0; i < nevent; i++) {
		l = snprintf(buf, sizeof(buf), "%"PRIu64,
		    sample_n_per_event[opt_mode][i]);
		if (sample_event_width < (u_int)l) {
			sample_event_width = l;
			do_redraw = true;
		}
	}
	for (n = 0; n < ncpu; n++) {
		uint64_t sum = 0;
		for (i = 0; i < nevent; i++)
			sum += sample_n_per_event_cpu[opt_mode][nevent * n + i];
		l = snprintf(buf, sizeof(buf), "%"PRIu64, sum);
		if (sample_cpu_width[n] < (u_int)l) {
			sample_cpu_width[n] = l;
			do_redraw = true;
		}
	}

	if (do_redraw) {
		lim_printf(lim, "  Rate %*s %-*s",
		    sample_event_width, "Sample#",
		    SYMBOL_LEN, "Eventname");
		for (n = 0; n < ncpu; n++) {
			snprintf(buf, sizeof(buf), "CPU%d", n);
			lim_printf(lim, " %*s", sample_cpu_width[n], buf);
		}
		lim_newline(lim);

		lim_printf(lim, "------ %*.*s %*.*s",
		    sample_event_width, sample_event_width, LINESTR,
		    SYMBOL_LEN, SYMBOL_LEN, LINESTR);
		for (n = 0; n < ncpu; n++) {
			lim_printf(lim, " %*.*s",
			    sample_cpu_width[n], sample_cpu_width[n], LINESTR);
		}
		lim_newline(lim);
	} else {
		printf("\n\n");
	}

	for (i = 0; i < nevent; i++) {
		if (sample_n_per_event[opt_mode][i] >= nsample_total) {
			lim_printf(lim, "%5.1f%%", 100.0 *
			    sample_n_per_event[opt_mode][i] / nsample_total);
		} else {
			lim_printf(lim, "%5.2f%%", 100.0 *
			    sample_n_per_event[opt_mode][i] / nsample_total);
		}
		lim_printf(lim, " %*"PRIu64" ", sample_event_width,
		    sample_n_per_event[opt_mode][i]);

		lim_printf(lim, "%-32.32s", eventname[i]);
		for (n = 0; n < ncpu; n++) {
			lim_printf(lim, " %*"PRIu64, sample_cpu_width[n],
			    sample_n_per_event_cpu[opt_mode][nevent * n + i]);
		}
		lim_newline(lim);
	}
}

static void
sample_show(void)
{
	struct sample_elm *e;
	struct ptrarray *samples;
	u_int nsample_total;
	int i, l, lim, n, ndisp;
	char namebuf[32];
	const char *name;

	if (nshow++ == 0) {
		printf("\n");
		if (!nontty) {
			signal(SIGWINCH, sigwinch_handler);
			signal(SIGINT, die);
			signal(SIGQUIT, die);
			signal(SIGTERM, die);
			signal(SIGTSTP, sigtstp_handler);

			tty_setup();
		}
	} else {
		reset_cursor_pos();
	}

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

	lim = win.ws_col;
	if (opt_mode == SAMPLE_MODE_ACCUMULATIVE)
		lim_printf(&lim, "[Accumulative mode] ");
	show_tprof_stat(&lim);

	if (lim >= 16) {
		l = win.ws_col - lim;
		if (!nontty) {
			clr_to_eol();
			for (; l <= win.ws_col - 17; l = ((l + 8) & -8))
				printf("\t");
		}
		show_timestamp();
	}
	lim_newline(&lim);
	lim_newline(&lim);

	if (opt_showcounter)
		show_counters(&lim);

	show_count_per_event(&lim);
	lim_newline(&lim);

	if (do_redraw) {
		lim_printf(&lim, "  Rate %*s %-*s",
		    sample_event_width, "Sample#",
		    SYMBOL_LEN, "Symbol");
		for (n = 0; n < ncpu; n++) {
			snprintf(namebuf, sizeof(namebuf), "CPU%d", n);
			lim_printf(&lim, " %*s", sample_cpu_width[n], namebuf);
		}
		lim_newline(&lim);

		lim_printf(&lim, "------ %*.*s %*.*s",
		    sample_event_width, sample_event_width, LINESTR,
		    SYMBOL_LEN, SYMBOL_LEN, LINESTR);
		for (n = 0; n < ncpu; n++) {
			lim_printf(&lim, " %*.*s", sample_cpu_width[n],
			    sample_cpu_width[n], LINESTR);
		}
		lim_newline(&lim);
	} else {
		printf("\n\n");
	}

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
			lim_printf(&lim, "%5.1f%%", 100.0 *
			    e->num[opt_mode] / nsample_total);
		} else {
			lim_printf(&lim, "%5.2f%%", 100.0 *
			    e->num[opt_mode] / nsample_total);
		}
		lim_printf(&lim, " %*u %-32.32s", sample_event_width,
		    e->num[opt_mode], name);

		for (n = 0; n < ncpu; n++) {
			if (SAMPLE_ELM_NUM_CPU(e, opt_mode)[n] == 0) {
				lim_printf(&lim, " %*s", sample_cpu_width[n],
				    ".");
			} else {
				lim_printf(&lim, " %*u", sample_cpu_width[n],
				    SAMPLE_ELM_NUM_CPU(e, opt_mode)[n]);
			}
		}
		lim_newline(&lim);
	}

	if ((u_int)ndisp != samples->pa_inuse) {
		lim_printf(&lim, "     : %*s (more %zu symbols omitted)",
		    sample_event_width, ":", samples->pa_inuse - ndisp);
		lim_newline(&lim);
	} else if (!nontty) {
		for (i = ndisp; i <= win.ws_row - margin_lines; i++) {
			printf("~");
			lim_newline(&lim);
		}
	}

	if (do_redraw) {
		lim_printf(&lim, "------ %*.*s %*.*s",
		    sample_event_width, sample_event_width, LINESTR,
		    SYMBOL_LEN, SYMBOL_LEN, LINESTR);
		for (n = 0; n < ncpu; n++) {
			lim_printf(&lim, " %*.*s",
			    sample_cpu_width[n], sample_cpu_width[n], LINESTR);
		}
		lim_newline(&lim);
	} else {
		printf("\n");
	}

	lim_printf(&lim, "Total  %*u %-32.32s",
	    sample_event_width, sample_n_kern[opt_mode], "in-kernel");
	for (n = 0; n < ncpu; n++) {
		lim_printf(&lim, " %*u", sample_cpu_width[n],
		    sample_n_kern_per_cpu[opt_mode][n]);
	}

	if (opt_userland) {
		lim_newline(&lim);
		lim_printf(&lim, "       %*u %-32.32s",
		    sample_event_width, sample_n_user[opt_mode], "userland");
		for (n = 0; n < ncpu; n++) {
			lim_printf(&lim, " %*u", sample_cpu_width[n],
			    sample_n_user_per_cpu[opt_mode][n]);
		}
	}

	if (nontty)
		printf("\n");
	else
		clr_to_eol();
}

__dead static void
tprof_top_usage(void)
{
	fprintf(stderr, "%s top [-acu] [-e name[,scale] [-e ...]]"
	    " [-i interval]\n", getprogname());
	exit(EXIT_FAILURE);
}

__dead void
tprof_top(int argc, char **argv)
{
	tprof_param_t params[TPROF_MAXCOUNTERS];
	struct itimerval it;
	ssize_t tprof_bufsize, len;
	u_int i;
	int ch, ret;
	char *tprof_buf, *p, *errmsg;
	bool noinput = false;

	memset(params, 0, sizeof(params));
	nevent = 0;

	while ((ch = getopt(argc, argv, "ace:i:L:u")) != -1) {
		switch (ch) {
		case 'a':
			opt_mode = SAMPLE_MODE_ACCUMULATIVE;
			break;
		case 'c':
			opt_showcounter = 1;
			break;
		case 'e':
			if (tprof_parse_event(&params[nevent], optarg,
			    TPROF_PARSE_EVENT_F_ALLOWSCALE,
			    &eventname[nevent], &errmsg) != 0) {
				die_errc(EXIT_FAILURE, 0, "%s", errmsg);
			}
			nevent++;
			if (nevent > __arraycount(params) ||
			    nevent > ncounters)
				die_errc(EXIT_FAILURE, 0,
				    "Too many events. Only a maximum of %d "
				    "counters can be used.", ncounters);
			break;
		case 'i':
			top_interval = strtol(optarg, &p, 10);
			if (*p != '\0' || top_interval <= 0)
				die_errc(EXIT_FAILURE, 0,
				    "Bad/invalid interval: %s", optarg);
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
			die_errc(EXIT_FAILURE, 0, "cpu not supported");

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
			die_errc(EXIT_FAILURE, errno,
			    "TPROF_IOC_CONFIGURE_EVENT: %s", eventname[i]);
	}

	tprof_countermask_t mask = TPROF_COUNTERMASK_ALL;
	ret = ioctl(devfd, TPROF_IOC_START, &mask);
	if (ret == -1)
		die_errc(EXIT_FAILURE, errno, "TPROF_IOC_START");

	ksyms = ksymload(&nksyms);

	signal(SIGALRM, sigalrm_handler);

	it.it_interval.tv_sec = it.it_value.tv_sec = top_interval;
	it.it_interval.tv_usec = it.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);

	sample_reset(true);
	printf("collecting samples...");
	fflush(stdout);

	tprof_bufsize = sizeof(tprof_sample_t) * 1024 * 32;
	tprof_buf = emalloc(tprof_bufsize);
	do {
		bool force_update = false;

		while (sigalrm == 0 && !force_update) {
			fd_set r;
			int nfound;
			char c;

			FD_ZERO(&r);
			if (!noinput)
				FD_SET(STDIN_FILENO, &r);
			FD_SET(devfd, &r);
			nfound = select(devfd + 1, &r, NULL, NULL, NULL);
			if (nfound == -1) {
				if (errno == EINTR)
					break;
				die_errc(EXIT_FAILURE, errno, "select");
			}

			if (FD_ISSET(STDIN_FILENO, &r)) {
				len = read(STDIN_FILENO, &c, 1);
				if (len <= 0) {
					noinput = true;
					continue;
				}
				switch (c) {
				case 0x0c:	/* ^L */
					do_redraw = true;
					break;
				case 'a':
					/* toggle mode */
					opt_mode = (opt_mode + 1) %
					    SAMPLE_MODE_NUM;
					do_redraw = true;
					break;
				case 'c':
					/* toggle mode */
					opt_showcounter ^= 1;
					do_redraw = true;
					break;
				case 'q':
					goto done;
				case 'z':
					sample_reset(true);
					break;
				default:
					continue;
				}
				force_update = true;
			}

			if (FD_ISSET(devfd, &r)) {
				len = read(devfd, tprof_buf, tprof_bufsize);
				if (len == -1 && errno != EINTR)
					die_errc(EXIT_FAILURE, errno, "read");
				if (len > 0) {
					tprof_sample_t *s =
					    (tprof_sample_t *)tprof_buf;
					while (s <
					    (tprof_sample_t *)(tprof_buf + len))
						sample_collect(s++);
				}
			}
		}
		sigalrm = 0;

		/* update screen */
		sample_show();
		fflush(stdout);
		do_redraw = false;
		if (force_update)
			continue;

		sample_reset(false);

	} while (!nontty);

 done:
	die(0);
}
