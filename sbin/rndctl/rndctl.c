/*	$NetBSD: rndctl.c,v 1.16 2003/07/13 07:59:24 itojun Exp $	*/

/*-
 * Copyright (c) 1997 Michael Graff.
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
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: rndctl.c,v 1.16 2003/07/13 07:59:24 itojun Exp $");
#endif


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/rnd.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <string.h>

typedef struct {
	char *a_name;
	u_int32_t a_type;
} arg_t;

arg_t source_types[] = {
	{ "???",     RND_TYPE_UNKNOWN },
	{ "disk",    RND_TYPE_DISK },
	{ "net",     RND_TYPE_NET },
	{ "tape",    RND_TYPE_TAPE },
	{ "tty",     RND_TYPE_TTY },
	{ "rng",     RND_TYPE_RNG },
	{ NULL,      0 }
};

static void usage(void);
u_int32_t find_type(char *name);
char *find_name(u_int32_t);
void do_ioctl(rndctl_t *);
char * strflags(u_int32_t);
void do_list(int, u_int32_t, char *);
void do_stats(void);

static void
usage(void)
{

	fprintf(stderr, "usage: %s -CEce [-t devtype] [-d devname]\n",
	    getprogname());
	fprintf(stderr, "       %s -ls [-t devtype] [-d devname]\n",
	    getprogname());
	exit(1);
}

u_int32_t
find_type(char *name)
{
	arg_t *a;

	a = source_types;

	while (a->a_name != NULL) {
		if (strcmp(a->a_name, name) == 0)
			return (a->a_type);
		a++;
	}

	errx(1, "device name %s unknown", name);
	return (0);
}

char *
find_name(u_int32_t type)
{
	arg_t *a;

	a = source_types;

	while (a->a_name != NULL) {
		if (type == a->a_type)
			return (a->a_name);
		a++;
	}

	warnx("device type %u unknown", type);
	return ("???");
}

void
do_ioctl(rndctl_t *rctl)
{
	int fd;
	int res;

	fd = open("/dev/urandom", O_RDONLY, 0644);
	if (fd < 0)
		err(1, "open");

	res = ioctl(fd, RNDCTL, rctl);
	if (res < 0)
		err(1, "ioctl(RNDCTL)");

	close(fd);
}

char *
strflags(u_int32_t fl)
{
	static char str[512];

	str[0] = 0;
	if (fl & RND_FLAG_NO_ESTIMATE)
		;
	else
		strlcat(str, "estimate", sizeof(str));

	if (fl & RND_FLAG_NO_COLLECT)
		;
	else {
		if (str[0])
			strlcat(str, ", ", sizeof(str));
		strlcat(str, "collect", sizeof(str));
	}

	return (str);
}

#define HEADER "Source                 Bits Type      Flags\n"

void
do_list(int all, u_int32_t type, char *name)
{
	rndstat_t rstat;
	rndstat_name_t rstat_name;
	int fd;
	int res;
	u_int32_t start;

	fd = open("/dev/urandom", O_RDONLY, 0644);
	if (fd < 0)
		err(1, "open");

	if (all == 0 && type == 0xff) {
		strncpy(rstat_name.name, name, sizeof(rstat_name.name));
		res = ioctl(fd, RNDGETSRCNAME, &rstat_name);
		if (res < 0)
			err(1, "ioctl(RNDGETSRCNAME)");
		printf(HEADER);
		printf("%-16s %10u %-4s %s\n",
		    rstat_name.source.name,
		    rstat_name.source.total,
		    find_name(rstat_name.source.type),
		    strflags(rstat_name.source.flags));
		close(fd);
		return;
	}

	/*
	 * Run through all the devices present in the system, and either
	 * print out ones that match, or print out all of them.
	 */
	printf(HEADER);
	start = 0;
	for (;;) {
		rstat.count = RND_MAXSTATCOUNT;
		rstat.start = start;
		res = ioctl(fd, RNDGETSRCNUM, &rstat);
		if (res < 0)
			err(1, "ioctl(RNDGETSRCNUM)");

		if (rstat.count == 0)
			break;

		for (res = 0; res < rstat.count; res++) {
			if (all != 0 ||
			    type == rstat.source[res].type)
				printf("%-16s %10u %-4s %s\n",
				    rstat.source[res].name,
				    rstat.source[res].total,
				    find_name(rstat.source[res].type),
				    strflags(rstat.source[res].flags));
		}
		start += rstat.count;
	}

	close(fd);
}

void
do_stats()
{
	rndpoolstat_t rs;
	int fd;

	fd = open("/dev/urandom", O_RDONLY, 0644);
	if (fd < 0)
		err(1, "open");

	if (ioctl(fd, RNDGETPOOLSTAT, &rs) < 0)
		err(1, "ioctl(RNDGETPOOLSTAT)");

	printf("\t%9u bits mixed into pool\n", rs.added);
	printf("\t%9u bits currently stored in pool (max %u)\n",
	    rs.curentropy, rs.maxentropy);
	printf("\t%9u bits of entropy discarded due to full pool\n",
	    rs.discarded);
	printf("\t%9u hard-random bits generated\n", rs.removed);
	printf("\t%9u pseudo-random bits generated\n", rs.generated);

	close(fd);
}

int
main(int argc, char **argv)
{
	rndctl_t rctl;
	int ch, cmd, lflag, mflag, sflag;
	u_int32_t type;
	char name[16];

	rctl.mask = 0;
	rctl.flags = 0;

	cmd = 0;
	lflag = 0;
	mflag = 0;
	sflag = 0;
	type = 0xff;

	while ((ch = getopt(argc, argv, "CEcelt:d:s")) != -1)
		switch (ch) {
		case 'C':
			rctl.flags |= RND_FLAG_NO_COLLECT;
			rctl.mask |= RND_FLAG_NO_COLLECT;
			mflag++;
			break;
		case 'E':
			rctl.flags |= RND_FLAG_NO_ESTIMATE;
			rctl.mask |= RND_FLAG_NO_ESTIMATE;
			mflag++;
			break;
		case 'c':
			rctl.flags &= ~RND_FLAG_NO_COLLECT;
			rctl.mask |= RND_FLAG_NO_COLLECT;
			mflag++;
			break;
		case 'e':
			rctl.flags &= ~RND_FLAG_NO_ESTIMATE;
			rctl.mask |= RND_FLAG_NO_ESTIMATE;
			mflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 't':
			if (cmd != 0)
				usage();
			cmd = 't';

			type = find_type(optarg);
			break;
		case 'd':
			if (cmd != 0)
				usage();
			cmd = 'd';

			type = 0xff;
			strlcpy(name, optarg, sizeof(name));
			break;
		case 's':
			sflag++;
			break;
		case '?':
		default:
			usage();
		}

	/*
	 * Cannot list and modify at the same time.
	 */
	if ((lflag != 0 || sflag != 0) && mflag != 0)
		usage();

	/*
	 * Bomb out on no-ops.
	 */
	if (lflag == 0 && mflag == 0 && sflag == 0)
		usage();

	/*
	 * If not listing, we need a device name or a type.
	 */
	if (lflag == 0 && cmd == 0 && sflag == 0)
		usage();

	/*
	 * Modify request.
	 */
	if (mflag != 0) {
		rctl.type = type;
		strncpy(rctl.name, name, sizeof(rctl.name));
		do_ioctl(&rctl);

		exit(0);
	}

	/*
	 * List sources.
	 */
	if (lflag != 0)
		do_list(cmd == 0, type, name);

	if (sflag != 0)
		do_stats();

	exit(0);
}
