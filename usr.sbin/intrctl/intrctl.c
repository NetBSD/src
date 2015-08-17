/*	$NetBSD: intrctl.c,v 1.1 2015/08/17 06:42:46 knakahara Exp $	*/

/*
 * Copyright (c) 2015 Internet Initiative Japan Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: intrctl.c,v 1.1 2015/08/17 06:42:46 knakahara Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/intrio.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intrctl_io.h"

__dead static void	usage(void);

int		verbose;

static void	intrctl_list(int, char **);
static void	intrctl_affinity(int, char **);
static void	intrctl_intr(int, char **);
static void	intrctl_nointr(int, char **);

static struct cmdtab {
	const char	*label;
	void	(*func)(int, char **);
} const intrctl_cmdtab[] = {
	{ "list", intrctl_list },
	{ "affinity", intrctl_affinity },
	{ "intr", intrctl_intr },
	{ "nointr", intrctl_nointr },
	{ NULL, NULL },
};

int
main(int argc, char **argv)
{
	const struct cmdtab *ct;
	char *cmdname;

	if (argc < 2)
		usage();

	cmdname = argv[1];
	argv += 1;
	argc -= 1;

	for (ct = intrctl_cmdtab; ct->label != NULL; ct++) {
		if (strcmp(cmdname, ct->label) == 0) {
			break;
		}
	}
	if (ct->label == NULL)
		errx(EXIT_FAILURE, "unknown command ``%s''", cmdname);

	(*ct->func)(argc, argv);
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s list\n", progname);
	fprintf(stderr, "       %s affinity -i interrupt_name -c cpu_index\n", progname);
	fprintf(stderr, "       %s intr -c cpu_index\n", progname);
	fprintf(stderr, "       %s nointr -c cpu_index\n", progname);
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

static int intrctl_io_alloc_retry_count = 4;

static void
intrctl_list(int argc, char **argv)
{
	struct intrio_list_line *illine;
	int i, ncpus;
	void *handle;

	handle = intrctl_io_alloc(intrctl_io_alloc_retry_count);
	if (handle == NULL)
		err(EXIT_FAILURE, "intrctl_io_alloc");

	/* header */
	ncpus = intrctl_io_ncpus(handle);
	printf("interrupt id\t");
	for (i = 0; i < ncpus; i++) {
		printf("  CPU#%02u\t", i);
	}
	printf("device name(s)\n");

	/* body */
	illine = intrctl_io_firstline(handle);
	for (; illine != NULL; illine = intrctl_io_nextline(handle, illine)) {
		printf("%s\t", illine->ill_intrid);
		for (i = 0; i < ncpus; i++) {
			struct intrio_list_line_cpu *illc = &illine->ill_cpu[i];
			printf("%8" PRIu64 "%c\t", illc->illc_count,
			    illc->illc_assigned ? '*' : ' ');
		}

		printf("%s\n", illine->ill_xname);
	}

	intrctl_io_free(handle);
}

static void
intrctl_affinity(int argc, char **argv)
{
	struct intrio_set iset;
	cpuset_t *cpuset;
	unsigned long index;
	int ch, error;

	index = ULONG_MAX;
	memset(&iset.intrid, 0, sizeof(iset.intrid));

	while ((ch = getopt(argc, argv, "c:i:")) != -1) {
		switch (ch) {
		case 'c':
			index = strtoul(optarg, NULL, 10);
			break;
		case 'i':
			if (strnlen(optarg, ARG_MAX) > INTRIDBUF)
				usage();
			strlcpy(iset.intrid, optarg, INTRIDBUF);
			break;
		default:
			usage();
		}
	}

	if (iset.intrid[0] == '\0' || index == ULONG_MAX)
		usage();

	if (index >= (u_long)sysconf(_SC_NPROCESSORS_CONF))
		err(EXIT_FAILURE, "invalid cpu index");

	cpuset = cpuset_create();
	if (cpuset == NULL)
		err(EXIT_FAILURE, "create_cpuset()");

	cpuset_zero(cpuset);
	cpuset_set(index, cpuset);
	iset.cpuset = cpuset;
	iset.cpuset_size = cpuset_size(cpuset);
	error = sysctlbyname("kern.intr.affinity", NULL, NULL, &iset, sizeof(iset));
	cpuset_destroy(cpuset);
	if (error < 0)
		err(EXIT_FAILURE, "sysctl kern.intr.affinity");
}

static void
intrctl_intr(int argc, char **argv)
{
	struct intrio_set iset;
	cpuset_t *cpuset;
	unsigned long index;
	int ch, error;

	index = ULONG_MAX;

	while ((ch = getopt(argc, argv, "c:")) != -1) {
		switch (ch) {
		case 'c':
			index = strtoul(optarg, NULL, 10);
			break;
		default:
			usage();
		}
	}

	if (index == ULONG_MAX)
		usage();

	if (index >= (u_long)sysconf(_SC_NPROCESSORS_CONF))
		err(EXIT_FAILURE, "invalid cpu index");

	cpuset = cpuset_create();
	if (cpuset == NULL)
		err(EXIT_FAILURE, "create_cpuset()");

	cpuset_zero(cpuset);
	cpuset_set(index, cpuset);
	iset.cpuset = cpuset;
	iset.cpuset_size = cpuset_size(cpuset);
	error = sysctlbyname("kern.intr.intr", NULL, NULL, &iset, sizeof(iset));
	cpuset_destroy(cpuset);
	if (error < 0)
		err(EXIT_FAILURE, "sysctl kern.intr.intr");
}

static void
intrctl_nointr(int argc, char **argv)
{
	struct intrio_set iset;
	cpuset_t *cpuset;
	unsigned long index;
	int ch, error;

	index = ULONG_MAX;

	while ((ch = getopt(argc, argv, "c:")) != -1) {
		switch (ch) {
		case 'c':
			index = strtoul(optarg, NULL, 10);
			break;
		default:
			usage();
		}
	}

	if (index == ULONG_MAX)
		usage();

	if (index >= (u_long)sysconf(_SC_NPROCESSORS_CONF))
		err(EXIT_FAILURE, "invalid cpu index");

	cpuset = cpuset_create();
	if (cpuset == NULL)
		err(EXIT_FAILURE, "create_cpuset()");

	cpuset_zero(cpuset);
	cpuset_set(index, cpuset);
	iset.cpuset = cpuset;
	iset.cpuset_size = cpuset_size(cpuset);
	error = sysctlbyname("kern.intr.nointr", NULL, NULL, &iset, sizeof(iset));
	cpuset_destroy(cpuset);
	if (error < 0)
		err(EXIT_FAILURE, "sysctl kern.intr.nointr");
}
