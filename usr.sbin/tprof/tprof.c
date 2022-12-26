/*	$NetBSD: tprof.c,v 1.20 2022/12/26 08:00:13 ryo Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c)2008 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: tprof.c,v 1.20 2022/12/26 08:00:13 ryo Exp $");
#endif /* not lint */

#include <sys/atomic.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <dev/tprof/tprof_ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include "tprof.h"

#define	_PATH_TPROF	"/dev/tprof"

struct tprof_info tprof_info;
u_int ncounters;
int devfd;
int outfd;
int ncpu;
u_int nevent;
double interval = 0xffffffff;	/* XXX */
const char *eventname[TPROF_MAXCOUNTERS];
u_int eventnamewidth[TPROF_MAXCOUNTERS];
#define	COUNTER_COLUMNS_WIDTH	11

static void tprof_list(int, char **);
static void tprof_monitor_common(bool, int, char **) __dead;
static void tprof_monitor(int, char **) __dead;
static void tprof_count(int, char **) __dead;

static struct cmdtab {
	const char *label;
	bool takesargs;
	bool argsoptional;
	void (*func)(int, char **);
} const tprof_cmdtab[] = {
	{ "list",	false, false, tprof_list },
	{ "monitor",	true,  false, tprof_monitor },
	{ "count",	true,  false, tprof_count },
	{ "analyze",	true,  true,  tprof_analyze },
	{ "top",	true,  true,  tprof_top },
	{ NULL,		false, false, NULL },
};

__dead static void
usage(void)
{

	fprintf(stderr, "%s op [arguments]\n", getprogname());
	fprintf(stderr, "\n");
	fprintf(stderr, "\tlist\n");
	fprintf(stderr, "\t\tList the available events.\n");
	fprintf(stderr, "\tmonitor -e name[:option] [-e ...] [-o outfile]"
	    " command\n");
	fprintf(stderr, "\t\tMonitor the event 'name' with option 'option'\n"
	    "\t\tcounted during the execution of 'command'.\n");
	fprintf(stderr, "\tcount -e name[:option] [-e ...] [-i interval]"
	    " command\n");
	fprintf(stderr, "\t\tSame as monitor, but does not profile,"
	    " only outputs a counter.\n");
	fprintf(stderr, "\tanalyze [-CkLPs] [-p pid] file\n");
	fprintf(stderr, "\t\tAnalyze the samples of the file 'file'.\n");
	fprintf(stderr, "\ttop [-e name [-e ...]] [-i interval] [-acu]\n");
	fprintf(stderr, "\t\tDisplay profiling results in real-time.\n");
	exit(EXIT_FAILURE);
}

static int
getncpu(void)
{
	size_t size;
	int mib[2];

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	size = sizeof(ncpu);
	if (sysctl(mib, 2, &ncpu, &size, NULL, 0) == -1)
		ncpu = 1;
	return ncpu;
}

static void *
process_samples(void *dummy)
{

	for (;;) {
		char buf[4096];
		const char *cp;
		ssize_t ssz;

		ssz = read(devfd, buf, sizeof(buf));
		if (ssz == -1) {
			err(EXIT_FAILURE, "read");
		}
		if (ssz == 0) {
			break;
		}
		cp = buf;
		while (ssz) {
			ssize_t wsz;

			wsz = write(outfd, cp, ssz);
			if (wsz == -1) {
				err(EXIT_FAILURE, "write");
			}
			ssz -= wsz;
			cp += wsz;
		}
	}
	return NULL;
}

static void
show_counters(void)
{
	unsigned int i;
	int n, ret;

	fprintf(stderr, "      ");
	for (i = 0; i < nevent; i++)
		fprintf(stderr, " %*s", eventnamewidth[i], eventname[i]);
	fprintf(stderr, "\n");

	for (n = 0; n < ncpu; n++) {
		tprof_counts_t counts;

		memset(&counts, 0, sizeof(counts));
		counts.c_cpu = n;
		ret = ioctl(devfd, TPROF_IOC_GETCOUNTS, &counts);
		if (ret == -1)
			err(EXIT_FAILURE, "TPROF_IOC_GETCOUNTS");

		fprintf(stderr, "CPU%-3d", n);
		for (i = 0; i < nevent; i++) {
			fprintf(stderr, " %*"PRIu64,
			    eventnamewidth[i], counts.c_count[i]);
		}
		fprintf(stderr, "\n");
	}
}

/* XXX: avoid mixing with the output of the child process SIGINFO handler... */
static void
output_delay(void)
{
	struct timespec delay_ts;

	delay_ts.tv_sec = 0;
	delay_ts.tv_nsec = 100000000;
	nanosleep(&delay_ts, NULL);
}

static void
siginfo_nothing(int signo)
{
	__nothing;
}

static void
siginfo_showcount(int signo)
{
	output_delay();
	show_counters();
}

static void *
process_stat(void *arg)
{
	unsigned int *done = arg;
	double ival, fval;
	struct timespec ts;

	ival = floor(interval);
	fval = (1000000000 * (interval - ival));
	ts.tv_sec = ival;
	ts.tv_nsec = fval;

	while (atomic_add_int_nv(done, 0) == 0) {
		show_counters();
		nanosleep(&ts, NULL);
		if (errno == EINTR)	/* interrupted by SIGINFO? */
			output_delay();
	}
	return NULL;
}

static void
tprof_list(int argc, char **argv)
{
	printf("%u events can be counted at the same time\n", ncounters);
	tprof_event_list();
}

int
tprof_parse_event(tprof_param_t *param, const char *str, uint32_t flags,
    const char **eventnamep, char **errmsgp)
{
	double d;
	uint64_t n;
	int error = 0;
	char *p, *event = NULL, *opt = NULL, *scale = NULL;
	bool allow_option, allow_scale;
	static char errmsgbuf[128];

	allow_option = flags & TPROF_PARSE_EVENT_F_ALLOWOPTION;
	allow_scale = flags & TPROF_PARSE_EVENT_F_ALLOWSCALE;

	p = estrdup(str);
	event = p;
	if (allow_option) {
		opt = strchr(p, ':');
		if (opt != NULL) {
			*opt++ = '\0';
			p = opt;
		}
	}
	if (allow_scale) {
		scale = strchr(p, ',');
		if (scale != NULL)
			*scale++ = '\0';
	}

	tprof_event_lookup(event, param);

	if (opt != NULL) {
		while (*opt != '\0') {
			switch (*opt) {
			case 'u':
				param->p_flags |= TPROF_PARAM_USER;
				break;
			case 'k':
				param->p_flags |= TPROF_PARAM_KERN;
				break;
			default:
				error = -1;
				snprintf(errmsgbuf, sizeof(errmsgbuf),
				    "invalid option: '%c'", *opt);
				goto done;
			}
			opt++;
		}
	} else if (allow_option) {
		param->p_flags |= TPROF_PARAM_USER;
		param->p_flags |= TPROF_PARAM_KERN;
	}

	if (scale != NULL) {
		if (*scale == '=') {
			scale++;
			n = strtoull(scale, &p, 0);
			if (*p != '\0') {
				error = -1;
			} else {
				param->p_value2 = n;
				param->p_flags |=
				    TPROF_PARAM_VALUE2_TRIGGERCOUNT;
			}
		} else {
			if (strncasecmp("0x", scale, 2) == 0)
				d = strtol(scale, &p, 0);
			else
				d = strtod(scale, &p);
			if (*p != '\0' || d <= 0) {
				error = -1;
			} else {
				param->p_value2 = 0x100000000ULL / d;
				param->p_flags |= TPROF_PARAM_VALUE2_SCALE;
			}
		}

		if (error != 0) {
			snprintf(errmsgbuf, sizeof(errmsgbuf),
			    "invalid scale: %s", scale);
			goto done;
		}
	}

 done:
	if (eventnamep != NULL)
		*eventnamep = event;
	if (error != 0 && errmsgp != NULL)
		*errmsgp = errmsgbuf;
	return error;
}

static void
tprof_monitor_common(bool do_profile, int argc, char **argv)
{
	const char *outfile = "tprof.out";
	struct tprof_stat ts;
	tprof_param_t params[TPROF_MAXCOUNTERS];
	pid_t pid;
	pthread_t pt;
	int ret, ch, i;
	char *p, *errmsg;
	tprof_countermask_t mask = TPROF_COUNTERMASK_ALL;

	memset(params, 0, sizeof(params));

	while ((ch = getopt(argc, argv, do_profile ? "o:e:" : "e:i:")) != -1) {
		switch (ch) {
		case 'o':
			outfile = optarg;
			break;
		case 'i':
			interval = strtod(optarg, &p);
			if (*p != '\0' || interval <= 0)
				errx(EXIT_FAILURE, "Bad/invalid interval: %s",
				    optarg);
			break;
		case 'e':
			if (tprof_parse_event(&params[nevent], optarg,
			    TPROF_PARSE_EVENT_F_ALLOWOPTION |
			    (do_profile ? TPROF_PARSE_EVENT_F_ALLOWSCALE : 0),
			    &eventname[nevent], &errmsg) != 0) {
				errx(EXIT_FAILURE, "%s", errmsg);
			}
			eventnamewidth[nevent] = strlen(eventname[nevent]);
			if (eventnamewidth[nevent] < COUNTER_COLUMNS_WIDTH)
				eventnamewidth[nevent] = COUNTER_COLUMNS_WIDTH;
			nevent++;
			if (nevent > __arraycount(params) ||
			    nevent > ncounters)
				errx(EXIT_FAILURE, "Too many events. Only a"
				    " maximum of %d counters can be used.",
				    ncounters);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0 || nevent == 0) {
		usage();
	}

	if (do_profile) {
		outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (outfd == -1) {
			err(EXIT_FAILURE, "%s", outfile);
		}
	}

	for (i = 0; i < (int)nevent; i++) {
		params[i].p_counter = i;
		if (do_profile)
			params[i].p_flags |= TPROF_PARAM_PROFILE;
		ret = ioctl(devfd, TPROF_IOC_CONFIGURE_EVENT, &params[i]);
		if (ret == -1) {
			err(EXIT_FAILURE, "TPROF_IOC_CONFIGURE_EVENT: %s",
			    eventname[i]);
		}
	}

	ret = ioctl(devfd, TPROF_IOC_START, &mask);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_START");
	}

	pid = fork();
	switch (pid) {
	case -1:
		err(EXIT_FAILURE, "fork");
	case 0:
		close(devfd);
		execvp(argv[0], argv);
		_Exit(EXIT_FAILURE);
	}

	signal(SIGINT, SIG_IGN);
	if (do_profile)
		signal(SIGINFO, siginfo_showcount);
	else
		signal(SIGINFO, siginfo_nothing);

	unsigned int done = 0;
	if (do_profile)
		ret = pthread_create(&pt, NULL, process_samples, NULL);
	else
		ret = pthread_create(&pt, NULL, process_stat, &done);
	if (ret != 0)
		errx(1, "pthread_create: %s", strerror(ret));

	for (;;) {
		int status;

		pid = wait4(-1, &status, 0, NULL);
		if (pid == -1) {
			if (errno == ECHILD) {
				break;
			}
			err(EXIT_FAILURE, "wait4");
		}
		if (pid != 0 && WIFEXITED(status)) {
			break;
		}
	}

	ret = ioctl(devfd, TPROF_IOC_STOP, &mask);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_STOP");
	}

	if (!do_profile) {
		atomic_add_int(&done, 1);	/* terminate thread */
		kill(0, SIGINFO);
	}

	pthread_join(pt, NULL);

	if (do_profile) {
		ret = ioctl(devfd, TPROF_IOC_GETSTAT, &ts);
		if (ret == -1)
			err(EXIT_FAILURE, "TPROF_IOC_GETSTAT");

		fprintf(stderr, "\n%s statistics:\n", getprogname());
		fprintf(stderr, "\tsample %" PRIu64 "\n", ts.ts_sample);
		fprintf(stderr, "\toverflow %" PRIu64 "\n", ts.ts_overflow);
		fprintf(stderr, "\tbuf %" PRIu64 "\n", ts.ts_buf);
		fprintf(stderr, "\temptybuf %" PRIu64 "\n", ts.ts_emptybuf);
		fprintf(stderr, "\tdropbuf %" PRIu64 "\n", ts.ts_dropbuf);
		fprintf(stderr, "\tdropbuf_sample %" PRIu64 "\n",
		    ts.ts_dropbuf_sample);

		fprintf(stderr, "\n");
	}
	show_counters();

	exit(EXIT_SUCCESS);
}

static void
tprof_monitor(int argc, char **argv)
{
	tprof_monitor_common(true, argc, argv);
}

static void
tprof_count(int argc, char **argv)
{
	tprof_monitor_common(false, argc, argv);
}

int
main(int argc, char *argv[])
{
	const struct cmdtab *ct;
	int ret;

	getncpu();
	setprogname(argv[0]);
	argv += 1, argc -= 1;

	devfd = open(_PATH_TPROF, O_RDWR);
	if (devfd == -1) {
		err(EXIT_FAILURE, "%s", _PATH_TPROF);
	}

	ret = ioctl(devfd, TPROF_IOC_GETINFO, &tprof_info);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_GETINFO");
	}
	if (tprof_info.ti_version != TPROF_VERSION) {
		errx(EXIT_FAILURE, "version mismatch: version=%d, expected=%d",
		    tprof_info.ti_version, TPROF_VERSION);
	}
	if (tprof_event_init(tprof_info.ti_ident) == -1) {
		errx(EXIT_FAILURE, "cpu not supported");
	}

	ret = ioctl(devfd, TPROF_IOC_GETNCOUNTERS, &ncounters);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_GETNCOUNTERS");
	}
	if (ncounters == 0) {
		errx(EXIT_FAILURE, "no available counters");
	}

	if (argc == 0)
		usage();

	for (ct = tprof_cmdtab; ct->label != NULL; ct++) {
		if (strcmp(argv[0], ct->label) == 0) {
			if (!ct->argsoptional &&
			    ((ct->takesargs == 0) ^ (argv[1] == NULL)))
			{
				usage();
			}
			(*ct->func)(argc, argv);
			break;
		}
	}
	if (ct->label == NULL) {
		usage();
	}
}
