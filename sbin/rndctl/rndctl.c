/*	$NetBSD: rndctl.c,v 1.25.2.1 2014/08/20 00:02:27 tls Exp $	*/

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
#include <sys/types.h>
#include <sha1.h>

#ifndef lint
__RCSID("$NetBSD: rndctl.c,v 1.25.2.1 2014/08/20 00:02:27 tls Exp $");
#endif


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/rnd.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <paths.h>
#include <string.h>

typedef struct {
	const char *a_name;
	u_int32_t a_type;
} arg_t;

static const arg_t source_types[] = {
	{ "???",     RND_TYPE_UNKNOWN },
	{ "disk",    RND_TYPE_DISK },
	{ "net",     RND_TYPE_NET },
	{ "tape",    RND_TYPE_TAPE },
	{ "tty",     RND_TYPE_TTY },
	{ "rng",     RND_TYPE_RNG },
	{ "skew",    RND_TYPE_SKEW },
	{ "env",     RND_TYPE_ENV },
	{ "vm",      RND_TYPE_VM },
	{ "power",   RND_TYPE_POWER },
	{ NULL,      0 }
};

__dead static void usage(void);
static u_int32_t find_type(const char *name);
static const char *find_name(u_int32_t);
static void do_ioctl(rndctl_t *);
static char * strflags(u_int32_t);
static void do_list(int, u_int32_t, char *);
static void do_stats(void);

static int vflag;

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-CEce] [-d devname | -t devtype]\n",
	    getprogname());
	fprintf(stderr, "       %s [-lsv] [-d devname | -t devtype]\n",
	    getprogname());
	fprintf(stderr, "	%s -[L|S] save-file\n", getprogname());
	exit(1);
}

static u_int32_t
find_type(const char *name)
{
	const arg_t *a;

	a = source_types;

	while (a->a_name != NULL) {
		if (strcmp(a->a_name, name) == 0)
			return (a->a_type);
		a++;
	}

	errx(1, "device name %s unknown", name);
	return (0);
}

static const char *
find_name(u_int32_t type)
{
	const arg_t *a;

	a = source_types;

	while (a->a_name != NULL) {
		if (type == a->a_type)
			return (a->a_name);
		a++;
	}

	warnx("device type %u unknown", type);
	return ("???");
}

static void
do_save(const char *const filename)
{
	int est1, est2;
	rndpoolstat_t rp;
	rndsave_t rs;
	SHA1_CTX s;

	int fd;

	fd = open(_PATH_URANDOM, O_RDONLY, 0644);
	if (fd < 0) {
		err(1, "device open");
	}

	if (ioctl(fd, RNDGETPOOLSTAT, &rp) < 0) {
		err(1, "ioctl(RNDGETPOOLSTAT)");
	}

	est1 = rp.curentropy;

	if (read(fd, rs.data, sizeof(rs.data)) != sizeof(rs.data)) {
		err(1, "entropy read");
	}

	if (ioctl(fd, RNDGETPOOLSTAT, &rp) < 0) {
		err(1, "ioctl(RNDGETPOOLSTAT)");
	}

	est2 = rp.curentropy;

	if (est1 - est2 < 0) {
		rs.entropy = 0;
	} else {
		rs.entropy = est1 - est2;
	}

	SHA1Init(&s);
	SHA1Update(&s, (uint8_t *)&rs.entropy, sizeof(rs.entropy));
	SHA1Update(&s, rs.data, sizeof(rs.data));
	SHA1Final(rs.digest, &s);

	close(fd);
	unlink(filename);
	fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0600);
	if (fd < 0) {
		err(1, "output open");
	}

	if (write(fd, &rs, sizeof(rs)) != sizeof(rs)) {
		unlink(filename);
		fsync_range(fd, FDATASYNC|FDISKSYNC, (off_t)0, (off_t)0);
		err(1, "write");
	}
	fsync_range(fd, FDATASYNC|FDISKSYNC, (off_t)0, (off_t)0);
	close(fd);
}

static void
do_load(const char *const filename)
{
	int fd;
	rndsave_t rs, rszero;
	rnddata_t rd;
	SHA1_CTX s;
	uint8_t digest[SHA1_DIGEST_LENGTH];

	fd = open(filename, O_RDWR, 0600);
	if (fd < 0) {
		err(1, "input open");
	}

	unlink(filename);

	if (read(fd, &rs, sizeof(rs)) != sizeof(rs)) {
		err(1, "read");
	}

	memset(&rszero, 0, sizeof(rszero));
	if (pwrite(fd, &rszero, sizeof(rszero), (off_t)0) != sizeof(rszero))
		err(1, "overwrite");
	fsync_range(fd, FDATASYNC|FDISKSYNC, (off_t)0, (off_t)0);
	close(fd);

	SHA1Init(&s);
	SHA1Update(&s, (uint8_t *)&rs.entropy, sizeof(rs.entropy));
	SHA1Update(&s, rs.data, sizeof(rs.data));
	SHA1Final(digest, &s);

	if (memcmp(digest, rs.digest, sizeof(digest))) {
		errx(1, "bad digest");
	}

	rd.len = MIN(sizeof(rd.data), sizeof(rs.data));
	rd.entropy = rs.entropy;
	memcpy(rd.data, rs.data, MIN(sizeof(rd.data), sizeof(rs.data)));

	fd = open(_PATH_URANDOM, O_RDWR, 0644);
	if (fd < 0) {
		err(1, "device open");
	}

	if (ioctl(fd, RNDADDDATA, &rd) < 0) {
		err(1, "ioctl");
	}
	close(fd);
}

static void
do_ioctl(rndctl_t *rctl)
{
	int fd;
	int res;

	fd = open(_PATH_URANDOM, O_RDONLY, 0644);
	if (fd < 0)
		err(1, "open");

	res = ioctl(fd, RNDCTL, rctl);
	if (res < 0)
		err(1, "ioctl(RNDCTL)");

	close(fd);
}

static char *
strflags(u_int32_t fl)
{
	static char str[512];

	str[0] = '\0';
	if (fl & RND_FLAG_NO_ESTIMATE)
		;
	else
		strlcat(str, "estimate, ", sizeof(str));

	if (fl & RND_FLAG_NO_COLLECT)
		;
	else
		strlcat(str, "collect, ", sizeof(str));

	if (fl & RND_FLAG_COLLECT_VALUE)
		strlcat(str, "v, ", sizeof(str));
	if (fl & RND_FLAG_COLLECT_TIME)
		strlcat(str, "t, ", sizeof(str));
	if (fl & RND_FLAG_ESTIMATE_VALUE)
		strlcat(str, "dv, ", sizeof(str));
	if (fl & RND_FLAG_ESTIMATE_TIME)
		strlcat(str, "dt, ", sizeof(str));

	if (str[strlen(str) - 2] == ',')
		str[strlen(str) - 2] = '\0';

	return (str);
}

#define HEADER "Source                 Bits Type      Flags\n"

static void
do_list(int all, u_int32_t type, char *name)
{
	rndstat_est_t rstat;
	rndstat_est_name_t rstat_name;
	int fd;
	int res;
	uint32_t i;
	u_int32_t start;

	fd = open(_PATH_URANDOM, O_RDONLY, 0644);
	if (fd < 0)
		err(1, "open");

	if (all == 0 && type == 0xff) {
		strncpy(rstat_name.name, name, sizeof(rstat_name.name));
		res = ioctl(fd, RNDGETESTNAME, &rstat_name);
		if (res < 0)
			err(1, "ioctl(RNDGETESTNAME)");
		printf(HEADER);
		printf("%-16s %10u %-4s %s\n",
		    rstat_name.source.rt.name,
		    rstat_name.source.rt.total,
		    find_name(rstat_name.source.rt.type),
		    strflags(rstat_name.source.rt.flags));
		if (vflag) {
			printf("\tDt samples = %d\n",
			       rstat_name.source.dt_samples);
			printf("\tDt bits = %d\n",
			       rstat_name.source.dt_total);
			printf("\tDv samples = %d\n",
				rstat_name.source.dv_samples);
			printf("\tDv bits = %d\n",
			       rstat_name.source.dv_total);
		}
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
		res = ioctl(fd, RNDGETESTNUM, &rstat);
		if (res < 0)
			err(1, "ioctl(RNDGETESTNUM)");

		if (rstat.count == 0)
			break;

		for (i = 0; i < rstat.count; i++) {
			if (all != 0 ||
			    type == rstat.source[i].rt.type)
				printf("%-16s %10u %-4s %s\n",
				    rstat.source[i].rt.name,
				    rstat.source[i].rt.total,
				    find_name(rstat.source[i].rt.type),
				    strflags(rstat.source[i].rt.flags));
			if (vflag) {
				printf("\tDt samples = %d\n",
				       rstat.source[i].dt_samples);
				printf("\tDt bits = %d\n",
				       rstat.source[i].dt_total);
				printf("\tDv samples = %d\n",
				       rstat.source[i].dv_samples);
				printf("\tDv bits = %d\n",
				       rstat.source[i].dv_total);
			}
                }
		start += rstat.count;
	}

	close(fd);
}

static void
do_stats(void)
{
	rndpoolstat_t rs;
	int fd;

	fd = open(_PATH_URANDOM, O_RDONLY, 0644);
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
	const char *filename = NULL;

	rctl.mask = 0;
	rctl.flags = 0;

	cmd = 0;
	lflag = 0;
	mflag = 0;
	sflag = 0;
	type = 0xff;

	while ((ch = getopt(argc, argv, "CES:L:celt:d:sv")) != -1) {
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
		case 'L':
			if (cmd != 0)
				usage();
			cmd = 'L';
			filename = optarg;
			break;
		case 'S':
			if (cmd != 0)
				usage();
			cmd = 'S';
			filename = optarg;
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
		case 'v':
			vflag++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * No leftover non-option arguments.
	 */
	if (argc > 0)
		usage();

	/*
	 * Save.
	 */
	if (cmd == 'S') {
		do_save(filename);
		exit(0);
	}

	/*
	 * Load.
	 */
	if (cmd == 'L') {
		do_load(filename);
		exit(0);
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
