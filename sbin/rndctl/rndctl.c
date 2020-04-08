/*	$NetBSD: rndctl.c,v 1.30.16.1 2020/04/08 14:07:20 martin Exp $	*/

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
__RCSID("$NetBSD: rndctl.c,v 1.30.16.1 2020/04/08 14:07:20 martin Exp $");
#endif


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/rndio.h>
#include <sys/sha3.h>

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
do_save(const char *filename, const void *extra, size_t nextra,
    uint32_t extraentropy)
{
	char tmp[PATH_MAX];
	uint32_t systementropy;
	uint8_t buf[32];
	SHAKE128_CTX shake128;
	rndsave_t rs;
	SHA1_CTX s;
	ssize_t nread, nwrit;
	int fd;

	/* Paranoia: Avoid stack memory disclosure.  */
	memset(&rs, 0, sizeof rs);

	/* Format the temporary file name.  */
	if (snprintf(tmp, sizeof tmp, "%s.tmp", filename) >= PATH_MAX)
		errx(1, "path too long");

	/* Open /dev/urandom.  */
	if ((fd = open(_PATH_URANDOM, O_RDONLY)) == -1)
		err(1, "device open");

	/* Find how much entropy is in the pool.  */
	if (ioctl(fd, RNDGETENTCNT, &systementropy) == -1)
		err(1, "ioctl(RNDGETENTCNT)");

	/* Read some data from /dev/urandom.  */
	if ((size_t)(nread = read(fd, buf, sizeof buf)) != sizeof buf) {
		if (nread == -1)
			err(1, "read");
		else
			errx(1, "truncated read");
	}

	/* Close /dev/urandom; we're done with it.  */
	if (close(fd) == -1)
		warn("close");
	fd = -1;		/* paranoia */

	/*
	 * Hash what we read together with the extra input to generate
	 * the seed data.
	 */
	SHAKE128_Init(&shake128);
	SHAKE128_Update(&shake128, buf, sizeof buf);
	SHAKE128_Update(&shake128, extra, nextra);
	SHAKE128_Final(rs.data, sizeof(rs.data), &shake128);
	explicit_memset(&shake128, 0, sizeof shake128); /* paranoia */

	/*
	 * Report an upper bound on the min-entropy of the seed data.
	 * We take the larger of the system entropy and the extra
	 * entropy -- the system state and the extra input may or may
	 * not be independent, so we can't add them -- and clamp to the
	 * size of the data.
	 */
	systementropy = MIN(systementropy,
	    MIN(sizeof(buf), UINT32_MAX/NBBY)*NBBY);
	extraentropy = MIN(extraentropy, MIN(nextra, UINT32_MAX/NBBY)*NBBY);
	rs.entropy = MIN(MAX(systementropy, extraentropy),
	    MIN(sizeof(rs.data), UINT32_MAX/NBBY)*NBBY);

	/*
	 * Compute the checksum on the 32-bit entropy count, in host
	 * byte order (XXX this means it is not portable across
	 * different-endian platforms!), followed by the seed data.
	 */
	SHA1Init(&s);
	SHA1Update(&s, (const uint8_t *)&rs.entropy, sizeof(rs.entropy));
	SHA1Update(&s, rs.data, sizeof(rs.data));
	SHA1Final(rs.digest, &s);
	explicit_memset(&s, 0, sizeof s); /* paranoia */

	/*
	 * Write it to a temporary file and sync it before we commit.
	 * This way either the old seed or the new seed is completely
	 * written in the expected location on disk even if the system
	 * crashes as long as the file system doesn't get corrupted too
	 * badly.
	 *
	 * If interrupted after this point and the temporary file is
	 * disclosed, no big deal -- either the pool was predictable to
	 * begin with in which case we're hosed either way, or we've
	 * just revealed some output which is not a problem.
	 */
	if ((fd = open(tmp, O_CREAT|O_TRUNC|O_WRONLY, 0600)) == -1)
		err(1, "open seed file to save");
	if ((size_t)(nwrit = write(fd, &rs, sizeof rs)) != sizeof rs) {
		int error = errno;
		if (unlink(tmp) == -1)
			warn("unlink");
		if (nwrit == -1)
			errc(1, error, "write");
		else
			errx(1, "truncated write");
	}
	explicit_memset(&rs, 0, sizeof rs); /* paranoia */
	if (fsync_range(fd, FDATASYNC|FDISKSYNC, 0, 0) == -1) {
		int error = errno;
		if (unlink(tmp) == -1)
			warn("unlink");
		errc(1, error, "fsync_range");
	}
	if (close(fd) == -1)
		warn("close");

	/* Rename it over the original file to commit.  */
	if (rename(tmp, filename) == -1)
		err(1, "rename");
}

static void
do_load(const char *filename)
{
	char tmp[PATH_MAX];
	int fd_seed, fd_random;
	rndsave_t rs;
	rnddata_t rd;
	ssize_t nread, nwrit;
	SHA1_CTX s;
	uint8_t digest[SHA1_DIGEST_LENGTH];

	/*
	 * The order of operations is important here:
	 *
	 * 1. Load the old seed.
	 * 2. Feed the old seed into the kernel.
	 * 3. Generate and write a new seed.
	 * 4. Erase the old seed.
	 *
	 * This follows the procedure in
	 *
	 *	Niels Ferguson, Bruce Schneier, and Tadayoshi Kohno,
	 *	_Cryptography Engineering_, Wiley, 2010, Sec. 9.6.2
	 *	`Update Seed File'.
	 *
	 * There is a race condition: If another process generates a
	 * key from /dev/urandom after step (2) but before step (3),
	 * and if the machine crashes before step (3), an adversary who
	 * can read the disk after the crash can probably guess the
	 * complete state of the entropy pool and thereby predict the
	 * key.
	 *
	 * There's not much we can do here without some kind of
	 * systemwide lock on /dev/urandom and without introducing an
	 * opportunity for a crash to wipe out the entropy altogether.
	 * To avoid this race, you should ensure that any key
	 * generation happens _after_ `rndctl -L' has completed.
	 */

	/* Format the temporary file name.  */
	if (snprintf(tmp, sizeof tmp, "%s.tmp", filename) >= PATH_MAX)
		errx(1, "path too long");

	/* 1. Load the old seed.  */
	if ((fd_seed = open(filename, O_RDWR)) == -1)
		err(1, "open seed file to load");
	if ((size_t)(nread = read(fd_seed, &rs, sizeof rs)) != sizeof rs) {
		if (nread == -1)
			err(1, "read seed");
		else
			errx(1, "seed too short");
	}

	/* Verify its checksum.  */
	SHA1Init(&s);
	SHA1Update(&s, (const uint8_t *)&rs.entropy, sizeof(rs.entropy));
	SHA1Update(&s, rs.data, sizeof(rs.data));
	SHA1Final(digest, &s);
	if (!consttime_memequal(digest, rs.digest, sizeof(digest))) {
		/*
		 * If the checksum doesn't match, doesn't hurt to feed
		 * the seed in anyway, but act as though it has zero
		 * entropy in case it was corrupted with predictable
		 * garbage.
		 */
		warnx("bad checksum");
		rs.entropy = 0;
	}

	/* Format the ioctl request.  */
	rd.len = MIN(sizeof(rd.data), sizeof(rs.data));
	rd.entropy = rs.entropy;
	memcpy(rd.data, rs.data, rd.len);
	explicit_memset(&rs, 0, sizeof rs); /* paranoia */

	/* 2. Feed the old seed into the kernel.  */
	if ((fd_random = open(_PATH_URANDOM, O_WRONLY)) == -1)
		err(1, "open /dev/urandom");
	if (ioctl(fd_random, RNDADDDATA, &rd) == -1)
		err(1, "RNDADDDATA");
	if (close(fd_random) == -1)
		warn("close /dev/urandom");
	fd_random = -1;		/* paranoia */

	/*
	 * 3. Generate and write a new seed.  Note that we hash the old
	 * seed together with whatever /dev/urandom returns in do_save.
	 * Why?  After RNDADDDATA, the input may not be distributed
	 * immediately to /dev/urandom.
	 */
	do_save(filename, rd.data, rd.len, rd.entropy);
	explicit_memset(&rd, 0, sizeof rd); /* paranoia */

	/*
	 * 4. Erase the old seed.  Only effective if we're on a
	 * fixed-address file system like ffs -- doesn't help to erase
	 * the data on lfs, but doesn't hurt either.  No need to unlink
	 * because do_save will have already overwritten it.
	 */
	memset(&rs, 0, sizeof rs);
	if ((size_t)(nwrit = pwrite(fd_seed, &rs, sizeof rs, 0)) !=
	    sizeof rs) {
		if (nwrit == -1)
			err(1, "overwrite old seed");
		else
			errx(1, "truncated overwrite");
	}
	if (fsync_range(fd_seed, FDATASYNC|FDISKSYNC, 0, 0) == -1)
		err(1, "fsync_range");
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

	if (SHA3_Selftest() != 0)
		errx(1, "SHA-3 self-test failed");

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
		do_save(filename, NULL, 0, 0);
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
