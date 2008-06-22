/*	$NetBSD: psrset.c,v 1.1 2008/06/22 13:53:59 ad Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#ifndef lint
__RCSID("$NetBSD: psrset.c,v 1.1 2008/06/22 13:53:59 ad Exp $");
#endif

#include <sys/types.h>
#include <sys/pset.h>
#include <sys/sched.h>
#include <sys/sysctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <ctype.h>

static void	usage(void);
static int	eatopt(char **);
static int	cmd_a(char **, int);
static int	cmd_b(char **, int);
static int	cmd_c(char **, int);
static int	cmd_d(char **, int);
static int	cmd_e(char **, int);
static int	cmd_i(char **, int);
static int	cmd_p(char **, int);
static int	cmd_r(char **, int);
static int	cmd_u(char **, int);
static int	cmd__(char **, int);

static psetid_t	psid;
static int	ncpu;
static cpuset_t	*cpuset;

int
main(int argc, char **argv)
{
	int (*cmd)(char **, int);
	int off;

	ncpu = sysconf(_SC_NPROCESSORS_CONF);
	cpuset = cpuset_create();
	if (cpuset == NULL)
		err(EXIT_FAILURE, "cpuset_create");
	cpuset_zero(cpuset);

	if (argc == 1 || argv[1][0] != '-') {
		cmd = cmd_i;
		off = 1;
	} else if (strncmp(argv[1], "-a", 2) == 0) {
		cmd = cmd_a;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-b", 2) == 0) {
		cmd = cmd_b;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-c", 2) == 0) {
		cmd = cmd_c;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-d", 2) == 0) {
		cmd = cmd_d;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-e", 2) == 0) {
		cmd = cmd_e;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-i", 2) == 0) {
		cmd = cmd_i;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-p", 2) == 0) {
		cmd = cmd_p;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-r", 2) == 0) {
		cmd = cmd_r;
		off = eatopt(argv);
	} else if (strncmp(argv[1], "-u", 2) == 0) {
		cmd = cmd_u;
		off = eatopt(argv);
	} else {
		cmd = cmd__;
		off = 0;
	}

	return (*cmd)(argv + off, argc - off);
}

static int
eatopt(char **argv)
{

	if (argv[1][2] != '\0') {
		argv[1] += 2;
		return 1;
	}
	return 2;
}

static int
getint(char *p)
{
	char *q;
	int rv;

	rv = (int)strtol(p, &q, 10);
	if (q == p || *q != '\0')
		usage();
	return rv;
}

static void
usage(void)
{

	fprintf(stderr, "usage:\n"
	    "\tpsrset [setid ...]\n"
	    "\tpsrset -a setid cpuid ...\n"
	    "\tpsrset -b setid pid ...\n"
	    "\tpsrset -c [cpuid ...]\n"
	    "\tpsrset -d setid\n"
	    "\tpsrset -e setid command\n"
	    "\tpsrset -i [setid ...]\n"
	    "\tpsrset -p\n"
	    "\tpsrset -r cpuid ...\n"
	    "\tpsrset -u pid ...\n");
                                                  
	exit(EXIT_FAILURE);
}

static void
makecpuset(char **argv)
{
	char *p, *q;
	int i, j;

	if (*argv == NULL) {
		for (i = 0; i < ncpu; i++)
			cpuset_set(i, cpuset);
		return;
	}

	for (; *argv != NULL; argv++) {
		if (!isdigit((unsigned)**argv))
			usage();
		i = (int)strtol(*argv, &p, 10);
		if ((*p != '\0' && *p != '-') || p == *argv)
			usage();
		if (*p == '-')  {
			if (!isdigit((unsigned)p[1]))
				usage();
			j = (int)strtol(p + 1, &q, 10);
			if (q == p || *q != '\0')
				usage();
		} else {
			j = i;
		}
		if ((unsigned int)i >= ncpu) {
			errx(EXIT_FAILURE, "value out of range");
		}
		while (i <= j)
			cpuset_set(i++, cpuset);
	}
}

/*
 * Assign processors to set.
 */
static int
cmd_a(char **argv, int argc)
{
	int i;

	if (argc < 2)
		usage();
	psid = getint(argv[0]);
	makecpuset(argv + 1);
	for (i = 0; i < ncpu; i++) {
		if (!cpuset_isset(i, cpuset))
			continue;
		if (pset_assign(psid, i, NULL))
			err(EXIT_FAILURE, "pset_assign");
	}

	return 0;
}

/*
 * Bind LWPs within processes to set.
 */
static int
cmd_b(char **argv, int argc)
{

	if (argc < 2)
		usage();
	psid = getint(*argv);
	for (argv++; *argv != NULL; argv++) {
		if (pset_bind(psid, P_PID, (idtype_t)getint(*argv), NULL))
			err(EXIT_FAILURE, "pset_bind");
	}

	return 0;
}

/*
 * Create set.
 */
static int
cmd_c(char **argv, int argc)
{
	int i;

	if (pset_create(&psid))
		err(EXIT_FAILURE, "pset_create");
	printf("%d\n", (int)psid);
	if (argc != 0)
		makecpuset(argv);
	for (i = 0; i < ncpu; i++) {
		if (!cpuset_isset(i, cpuset))
			continue;
		if (pset_assign(psid, i, NULL))
			err(EXIT_FAILURE, "pset_assign");
	}

	return 0;
}

/*
 * Destroy set.
 */
static int
cmd_d(char **argv, int argc)
{

	if (argc != 1)
		usage();
	if (pset_destroy(getint(argv[0])))
		err(EXIT_FAILURE, "pset_destroy");

	return 0;
}

/*
 * Execute command in set.
 */
static int
cmd_e(char **argv, int argc)
{

	if (argc < 2)
		usage();
	if (pset_bind(getint(argv[0]), P_PID, getpid(), NULL))
		err(EXIT_FAILURE, "pset_bind");
	(void)execvp(argv[1], argv + 1);
	return 1;
}

/*
 * Print info about each set.
 */
static int
cmd_i(char **argv, int argc)
{
	char buf[1024];
	size_t len;
	char *p, *q;
	int i, j, k;

	len = sizeof(buf);
	if (sysctlbyname("kern.pset.list", buf, &len,  NULL, 0) == -1)
		err(EXIT_FAILURE, "kern.pset.list");

	p = buf;
	while ((q = strsep(&p, ",")) != NULL) {
		i = (int)strtol(q, &q, 10);
		if (*q != ':')
			errx(EXIT_FAILURE, "bad kern.pset.list");
		printf("%s processor set %d: ",
		    (atoi(q + 1) == 1 ? "system" : "user"), i);
		for (j = 0, k = 0; j < ncpu; j++) {
			if (pset_assign(PS_QUERY, j, &psid))
				err(EXIT_FAILURE, "pset_assign");
			if (psid == i) {
				if (k++ == 0)
					printf("processor(s)");
				printf(" %d", j);
			}
		}
		if (k == 0)
			printf("empty");
		putchar('\n');
	}

	return 0;
}

/*
 * Print set ID for each processor.
 */
static int
cmd_p(char **argv, int argc)
{
	int i;

	makecpuset(argv);

	for (i = 0; i < ncpu; i++) {
		if (!cpuset_isset(i, cpuset))
			continue;
		if (pset_assign(PS_QUERY, i, &psid))
			err(EXIT_FAILURE, "ioctl");
		printf("processor %d: ", i);
		if (psid == PS_NONE)
			printf("not assigned\n");
		else
			printf("%d\n", (int)psid);
	}

	return 0;
}

/*
 * Remove CPU from set and return to system.
 */
static int
cmd_r(char **argv, int argc)
{
	int i;

	makecpuset(argv);

	for (i = 0; i < ncpu; i++) {
		if (!cpuset_isset(i, cpuset))
			continue;
		if (pset_assign(PS_NONE, i, NULL))
			err(EXIT_FAILURE, "ioctl");
	}

	return 0;
}

/*
 * Unbind LWPs within process.
 */
static int
cmd_u(char **argv, int argc)
{

	if (argc < 1)
		usage();
	for (; *argv != NULL; argv++) {
		if (pset_bind(PS_NONE, P_PID, (idtype_t)getint(*argv), NULL))
			err(EXIT_FAILURE, "pset_bind");
	}

	return 0;
}


/*
 * Dummy.
 */
static int
cmd__(char **argv, int argc)
{

	usage();
	return 0;
}
