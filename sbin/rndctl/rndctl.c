/*	$NetBSD: rndctl.c,v 1.5.8.1 2000/06/22 16:05:45 minoura Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/rnd.h>

typedef struct {
	char *name;
	u_int32_t   type;
} arg_t;

arg_t source_types[] = {
	{ "???",     RND_TYPE_UNKNOWN },
	{ "disk",    RND_TYPE_DISK },
	{ "net",     RND_TYPE_NET },
	{ "net",     RND_TYPE_NET },
	{ "tape",    RND_TYPE_TAPE },
	{ "tty",     RND_TYPE_TTY },
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
	fprintf(stderr, "usage: rndctl -CEce [-t devtype] [-d devname]\n");
	fprintf(stderr, "       rndctl -l [-t devtype] [-d devname]\n");
	exit(1);
}

u_int32_t
find_type(char *name)
{
	arg_t *a;

	a = source_types;
	
	while (a->name != NULL) {
		if (strcmp(a->name, name) == 0)
			return a->type;
		a++;
	}

	errx(1, "Error:  Device type %s unknown", name);
	return 0;
}

char *
find_name(u_int32_t type)
{
	arg_t *a;

	a = source_types;
	
	while (a->name != NULL) {
		if (type == a->type)
			return a->name;
		a++;
	}

	errx(1, "Error:  Device type %u unknown", type);
	return 0;
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
		strcat(str, "estimate");
	
	if (fl & RND_FLAG_NO_COLLECT)
		;
	else {
		if (str[0])
			strcat(str, ", ");
		strcat(str, "collect");
	}
	
	return str;
}

#define HEADER "Source                 Bits Type      Flags\n"

void
do_list(int all, u_int32_t type, char *name)
{
	rndstat_t       rstat;
	rndstat_name_t  rstat_name;
	int             fd;
	int             res;
	u_int32_t	start;

	fd = open("/dev/urandom", O_RDONLY, 0644);
	if (fd < 0)
		err(1, "open");

	if (all == 0 && type == 0xff) {
		strncpy(rstat_name.name, name, 16);
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
	 * run through all the devices present in the system, and either
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
                        
		for (res = 0 ; res < rstat.count ; res++) {
			if ((all != 0)
			    || (type == rstat.source[res].type))
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

	printf("\t%9d bits mixed into pool\n", rs.added);
	printf("\t%9d bits currently stored in pool (max %d)\n",
	    rs.curentropy, rs.maxentropy);
	printf("\t%9d bits of entropy discarded due to full pool\n",
	    rs.discarded);
	printf("\t%9d hard-random bits generated\n", rs.removed);
	printf("\t%9d pseudo-random bits generated\n", rs.generated);

	close(fd);
}


int
main(int argc, char **argv)
{
	rndctl_t  rctl;
	int       ch;
	int       cmd;
	int       lflag;
	int       mflag;
	int       sflag;
	u_int32_t type;
	char      name[16];

	rctl.mask = 0;
	rctl.flags = 0;

	cmd = 0;
	lflag = 0;
	mflag = 0;
	type = 0xff;

	while ((ch = getopt(argc, argv, "CEcelt:d:s")) != -1)
		switch(ch) {
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
			strncpy(name, optarg, 16);
			break;
		case 's':
			sflag++;
			break;
		case '?':
		default:
			usage();
		}

	/*
	 * cannot list and modify at the same time
	 */
	if ((lflag != 0 || sflag != 0) && mflag != 0)
		usage();

	/*
	 * bomb out on no-ops
	 */
	if (lflag == 0 && mflag == 0 && sflag == 0)
		usage();

	/*
	 * if not listing, we need a device name or a type
	 */
	if (lflag == 0 && cmd == 0 && sflag == 0)
		usage();

	/*
	 * modify request
	 */
	if (mflag != 0) {
		rctl.type = type;
		strncpy(rctl.name, name, 16);
		do_ioctl(&rctl);

		exit(0);
	}

	/*
	 * list sources
	 */
	if (lflag != 0)
		do_list(cmd == 0, type, name);

	if (sflag != 0)
		do_stats();
	
	return 0;
}
