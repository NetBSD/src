/*	$NetBSD: cpuctl.c,v 1.1 2007/08/04 11:03:05 ad Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: cpuctl.c,v 1.1 2007/08/04 11:03:05 ad Exp $");
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

u_int	getcpuid(char **);
int	main(int, char **);
void	usage(void);

void	cpu_list(char **);
void	cpu_offline(char **);
void	cpu_online(char **);

struct cmdtab {
	const char	*label;
	int	takesargs;
	void	(*func)(char **);
} const cpu_cmdtab[] = {
	{ "list", 0, cpu_list },
	{ "offline", 1, cpu_offline },
	{ "online", 1, cpu_online },
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

	(void)fprintf(stderr, "usage: %s <command> [arguments]\n",
	    getprogname());
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

u_int
getcpuid(char **argv)
{
	char *argp;
	u_int id;

	id = (int)strtoul(argv[0], &argp, 0);
	if (*argp != '\0')
		usage();

	return id;
}

void
cpu_list(char **argv)
{
	const char *state, *intr;
	cpustate_t cs;
	u_int cnt, i;
	
	if (ioctl(fd, IOC_CPU_GETCOUNT, &cnt) < 0)
		err(EXIT_FAILURE, "IOC_CPU_GETCOUNT");

	printf("ID   Unbound LWPs Interrupts     Last change\n");
 	printf("---- ------------ -------------- ----------------------------\n");

	for (i = 0; i < cnt; i++) {
		cs.cs_id = i;
		if (ioctl(fd, IOC_CPU_MAPID, &cs.cs_id) < 0)
			err(EXIT_FAILURE, "IOC_CPU_MAPID");
		if (ioctl(fd, IOC_CPU_GETSTATE, &cs) < 0)
			err(EXIT_FAILURE, "IOC_CPU_GETINFO");
		if (cs.cs_online)
			state = "online";
		else
			state = "offline";
		if (cs.cs_intr)
			intr = "intr";
		else
			intr = "nointr";
		printf("%-4d %-12s %-12s   %s", cs.cs_id, state,
		   intr, asctime(localtime(&cs.cs_lastmod)));
	}
}
