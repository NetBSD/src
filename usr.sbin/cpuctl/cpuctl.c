/*	$NetBSD: cpuctl.c,v 1.13.2.1 2009/05/13 19:20:20 jym Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: cpuctl.c,v 1.13.2.1 2009/05/13 19:20:20 jym Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/cpuio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <time.h>
#include <sched.h>

#include "cpuctl.h"

u_int	getcpuid(char **);
int	main(int, char **);
void	usage(void);

void	cpu_identify(char **);
void	cpu_list(char **);
void	cpu_offline(char **);
void	cpu_online(char **);
void	cpu_intr(char **);
void	cpu_nointr(char **);

struct cmdtab {
	const char	*label;
	int	takesargs;
	void	(*func)(char **);
} const cpu_cmdtab[] = {
	{ "identify", 1, cpu_identify },
	{ "list", 0, cpu_list },
	{ "offline", 1, cpu_offline },
	{ "online", 1, cpu_online },
	{ "intr", 1, cpu_intr },
	{ "nointr", 1, cpu_nointr },
	{ NULL, 0, NULL },
};

int	fd;

int
main(int argc, char **argv)
{
	const struct cmdtab *ct;

	if (argc < 2)
		usage();

	if ((fd = open("/dev/cpuctl", O_RDWR)) < 0)
		err(EXIT_FAILURE, "/dev/cpuctl");

	for (ct = cpu_cmdtab; ct->label != NULL; ct++) {
		if (strcmp(argv[1], ct->label) == 0) {
			if ((ct->takesargs == 0) ^ (argv[2] == NULL))
			    	usage();
			(*ct->func)(argv + 2);
			break;
		}
	}

	if (ct->label == NULL)
		errx(EXIT_FAILURE, "unknown command ``%s''", argv[optind]);

	close(fd);
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s identify cpuno\n", progname);
	fprintf(stderr, "       %s list\n", progname);
	fprintf(stderr, "       %s offline cpuno\n", progname);
	fprintf(stderr, "       %s online cpuno\n", progname);
	fprintf(stderr, "       %s intr cpuno\n", progname);
	fprintf(stderr, "       %s nointr cpuno\n", progname);
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

void
cpu_online(char **argv)
{
	cpustate_t cs;

	cs.cs_id = getcpuid(argv);
	if (ioctl(fd, IOC_CPU_GETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_GETSTATE");
	cs.cs_online = true;
	if (ioctl(fd, IOC_CPU_SETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_SETSTATE");
}

void
cpu_offline(char **argv)
{
	cpustate_t cs;

	cs.cs_id = getcpuid(argv);
	if (ioctl(fd, IOC_CPU_GETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_GETSTATE");
	cs.cs_online = false;
	if (ioctl(fd, IOC_CPU_SETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_SETSTATE");
}

void
cpu_intr(char **argv)
{
	cpustate_t cs;

	cs.cs_id = getcpuid(argv);
	if (ioctl(fd, IOC_CPU_GETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_GETSTATE");
	cs.cs_intr = true;
	if (ioctl(fd, IOC_CPU_SETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_SETSTATE");
}

void
cpu_nointr(char **argv)
{
	cpustate_t cs;

	cs.cs_id = getcpuid(argv);
	if (ioctl(fd, IOC_CPU_GETSTATE, &cs) < 0)
		err(EXIT_FAILURE, "IOC_CPU_GETSTATE");
	cs.cs_intr = false;
	if (ioctl(fd, IOC_CPU_SETSTATE, &cs) < 0) {
		if (errno == EOPNOTSUPP) {
			warnx("interrupt control not supported on "
			    "this platform");
		} else
			err(EXIT_FAILURE, "IOC_CPU_SETSTATE");
	}
}

void
cpu_identify(char **argv)
{
	char name[32];
	unsigned int id, np;
	cpuset_t *cpuset;

	np = sysconf(_SC_NPROCESSORS_CONF);
	id = getcpuid(argv);
	snprintf(name, sizeof(name), "cpu%u", id);

	if (np != 0) {
		cpuset = cpuset_create();
		if (cpuset == NULL)
			err(EXIT_FAILURE, "cpuset_create");
		cpuset_zero(cpuset);
		cpuset_set(id, cpuset);
		if (_sched_setaffinity(0, 0, cpuset_size(cpuset), cpuset) < 0) {
			if (errno == EPERM) {
				printf("Cannot bind to target CPU.  Output "
				    "may not accurately describe the target.\n"
				    "Run as root to allow binding.\n\n");
			} else { 
				err(EXIT_FAILURE, "_sched_setaffinity");
			}
		}
		cpuset_destroy(cpuset);
	}
	identifycpu(name);
}

u_int
getcpuid(char **argv)
{
	char *argp;
	u_int id;
	long np;

	id = (u_int)strtoul(argv[0], &argp, 0);
	if (*argp != '\0')
		usage();

	np = sysconf(_SC_NPROCESSORS_CONF);
	if (id >= (u_long)np)
		errx(EXIT_FAILURE, "Invalid CPU number");

	return id;
}

void
cpu_list(char **argv)
{
	const char *state, *intr;
	cpustate_t cs;
	u_int cnt, i;
	time_t lastmod;
	char ibuf[16], *ts;
	
	if (ioctl(fd, IOC_CPU_GETCOUNT, &cnt) < 0)
		err(EXIT_FAILURE, "IOC_CPU_GETCOUNT");

	printf(
"Num  HwId Unbound LWPs Interrupts Last change              #Intr\n"
"---- ---- ------------ ---------- ------------------------ -----\n");

	for (i = 0; i < cnt; i++) {
		cs.cs_id = i;
		if (ioctl(fd, IOC_CPU_GETSTATE, &cs) < 0)
			err(EXIT_FAILURE, "IOC_CPU_GETINFO");
		if (ioctl(fd, IOC_CPU_MAPID, &cs.cs_id) < 0)
			err(EXIT_FAILURE, "IOC_CPU_MAPID");
		if (cs.cs_online)
			state = "online";
		else
			state = "offline";
		if (cs.cs_intr)
			intr = "intr";
		else
			intr = "nointr";
		if (cs.cs_intrcnt == 0)
			strcpy(ibuf, "?");
		else
			snprintf(ibuf, sizeof(ibuf), "%d", cs.cs_intrcnt - 1);
		lastmod = (time_t)cs.cs_lastmod |
		    ((time_t)cs.cs_lastmodhi << 32);
		ts = asctime(localtime(&lastmod));
		ts[strlen(ts) - 1] = '\0';
		printf("%-4d %-4x %-12s %-10s %s %s\n", i, cs.cs_id, state,
		   intr, ts, ibuf);
	}
}

int
aprint_normal(const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return rv;
}
__strong_alias(aprint_verbose,aprint_normal)
__strong_alias(aprint_error,aprint_normal)

int
aprint_normal_dev(const char *dev, const char *fmt, ...)
{
	va_list ap;
	int rv;

	printf("%s: ", dev);
	va_start(ap, fmt);
	rv = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return rv;
}
__strong_alias(aprint_verbose_dev,aprint_normal_dev)
__strong_alias(aprint_error_dev,aprint_normal_dev)
