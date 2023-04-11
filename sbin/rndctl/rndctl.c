/*	$NetBSD: rndctl.c,v 1.41 2023/04/11 13:17:32 riastradh Exp $	*/

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
__RCSID("$NetBSD: rndctl.c,v 1.41 2023/04/11 13:17:32 riastradh Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/rndio.h>
#include <sys/sha3.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <sha1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
static char * strflags(uint32_t, u_int32_t);
static void do_list(int, u_int32_t, char *);
static void do_print_source(rndsource_est_t *);
static void do_print_source_verbose(rndsource_est_t *);
static void do_stats(void);

static int iflag;
static int vflag;

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-CEce] [-d devname | -t devtype]\n",
	    getprogname());
	fprintf(stderr, "       %s [-lsv] [-d devname | -t devtype]\n",
	    getprogname());
	fprintf(stderr, "       %s [-i] -L save-file\n", getprogname());
	fprintf(stderr, "       %s -S save-file\n", getprogname());
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

static int
update_seed(const char *filename, int fd_seed, const char *tmp,
    const void *extra, size_t nextra, uint32_t extraentropy)
{
	uint32_t systementropy;
	uint8_t buf[32];
	SHAKE128_CTX shake128;
	rndsave_t rs;
	SHA1_CTX s;
	ssize_t nread, nwrit;
	int fd_random;

	/* Paranoia: Avoid stack memory disclosure.  */
	memset(&rs, 0, sizeof rs);

	/* Open /dev/urandom to read data from the system.  */
	if ((fd_random = open(_PATH_URANDOM, O_RDONLY)) == -1) {
		warn("open /dev/urandom");
		return -1;
	}

	/* Find how much entropy is in the pool.  */
	if (ioctl(fd_random, RNDGETENTCNT, &systementropy) == -1) {
		warn("ioctl(RNDGETENTCNT)");
		systementropy = 0;
	}

	/* Read some data from /dev/urandom.  */
	if ((size_t)(nread = read(fd_random, buf, sizeof buf)) != sizeof buf) {
		if (nread == -1)
			warn("read");
		else
			warnx("truncated read");
		return -1;
	}

	/* Close /dev/urandom; we're done with it.  */
	if (close(fd_random) == -1)
		warn("close");
	fd_random = -1;		/* paranoia */

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
	 * Compute the checksum on the 32-bit entropy count, followed
	 * by the seed data.
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
	if ((size_t)(nwrit = write(fd_seed, &rs, sizeof rs)) != sizeof rs) {
		int error = errno;
		if (unlink(tmp) == -1)
			warn("unlink");
		if (nwrit == -1)
			warnc(error, "write");
		else
			warnx("truncated write");
		return -1;
	}
	explicit_memset(&rs, 0, sizeof rs); /* paranoia */
	if (fsync_range(fd_seed, FDATASYNC|FDISKSYNC, 0, 0) == -1) {
		int error = errno;
		if (unlink(tmp) == -1)
			warn("unlink");
		warnc(error, "fsync_range");
		return -1;
	}
	if (close(fd_seed) == -1)
		warn("close");

	/* Rename it over the original file to commit.  */
	if (rename(tmp, filename) == -1) {
		warn("rename");
		return -1;
	}

	/* Success!  */
	return 0;
}

static void
do_save(const char *filename)
{
	char tmp[PATH_MAX];
	int fd_seed;

	/* Consolidate any pending samples.  */
	if (sysctlbyname("kern.entropy.consolidate", NULL, NULL,
		(const int[]){1}, sizeof(int)) == -1)
		warn("consolidate entropy");

	/* Format the temporary file name.  */
	if (snprintf(tmp, sizeof tmp, "%s.tmp", filename) >= PATH_MAX)
		errx(1, "path too long");

	/* Create a temporary seed file.  */
	if ((fd_seed = open(tmp, O_CREAT|O_TRUNC|O_WRONLY, 0600)) == -1)
		err(1, "open seed file to save");

	/* Update the seed.  Abort on failure.  */
	if (update_seed(filename, fd_seed, tmp, NULL, 0, 0) == -1)
		exit(1);
}

static void
do_load(const char *filename)
{
	char tmp[PATH_MAX];
	int fd_new, fd_old, fd_random;
	rndsave_t rs;
	rnddata_t rd;
	ssize_t nread, nwrit;
	SHA1_CTX s;
	uint8_t digest[SHA1_DIGEST_LENGTH];
	int ro = 0, fail = 0;
	int error;

	/*
	 * 1. Load the old seed.
	 * 2. Feed the old seed into the kernel.
	 * 3. Generate and write a new seed.
	 * 4. Erase the old seed if we can.
	 *
	 * We follow the procedure in
	 *
	 *	Niels Ferguson, Bruce Schneier, and Tadayoshi Kohno,
	 *	_Cryptography Engineering_, Wiley, 2010, Sec. 9.6.2
	 *	`Update Seed File'.
	 *
	 * Additionally, we zero the seed's stored entropy estimate if
	 * it appears to be on a read-only medium.
	 */

	/* Format the temporary file name.  */
	if (snprintf(tmp, sizeof tmp, "%s.tmp", filename) >= PATH_MAX)
		errx(1, "path too long");

	/* Create a new seed file or determine the medium is read-only. */
	if ((fd_new = open(tmp, O_CREAT|O_TRUNC|O_WRONLY, 0600)) == -1) {
		warn("update seed file");
		ro = 1;
	}

	/*
	 * 1. Load the old seed.
	 */
	if ((fd_old = open(filename, O_RDWR)) == -1) {
		error = errno;
		if ((error != EPERM && error != EROFS) ||
		    (fd_old = open(filename, O_RDONLY)) == -1)
			err(1, "open seed file to load");
		if (fd_new != -1)
			warnc(error, "can't overwrite old seed file");
		ro = 1;
	}
	if ((size_t)(nread = read(fd_old, &rs, sizeof rs)) != sizeof rs) {
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

	/*
	 * If the entropy is insensibly large, try byte-swapping.
	 * Otherwise assume the file is corrupted and act as though it
	 * has zero entropy.
	 */
	if (howmany(rs.entropy, NBBY) > sizeof(rs.data)) {
		rs.entropy = bswap32(rs.entropy);
		if (howmany(rs.entropy, NBBY) > sizeof(rs.data)) {
			warnx("bad entropy estimate");
			rs.entropy = 0;
		}
	}

	/* If the medium can't be updated, zero the entropy estimate.  */
	if (ro)
		rs.entropy = 0;

	/* Fail later on if there's no entropy in the seed.  */
	if (rs.entropy == 0) {
		warnx("no entropy in seed");
		fail = 1;
	}

	/* If the user asked, zero the entropy estimate, but succeed.  */
	if (iflag)
		rs.entropy = 0;

	/*
	 * 2. Feed the old seed into the kernel.
	 *
	 * This also has the effect of consolidating pending samples,
	 * whether or not there are enough samples from sources deemed
	 * to have full entropy, so that the updated seed will
	 * incorporate them.
	 */
	rd.len = MIN(sizeof(rd.data), sizeof(rs.data));
	rd.entropy = rs.entropy;
	memcpy(rd.data, rs.data, rd.len);
	explicit_memset(&rs, 0, sizeof rs); /* paranoia */
	if ((fd_random = open(_PATH_URANDOM, O_WRONLY)) == -1)
		err(1, "open /dev/urandom");
	if (ioctl(fd_random, RNDADDDATA, &rd) == -1)
		err(1, "RNDADDDATA");
	explicit_memset(&rd, 0, sizeof rd); /* paranoia */
	if (close(fd_random) == -1)
		warn("close /dev/urandom");
	fd_random = -1;		/* paranoia */

	/*
	 * 3. Generate and write a new seed.
	 */
	if (fd_new == -1 ||
	    update_seed(filename, fd_new, tmp, rs.data, sizeof(rs.data),
		rs.entropy) == -1)
		fail = 1;

	/*
	 * 4. Erase the old seed.
	 *
	 * Only effective if we're on a fixed-address file system like
	 * ffs -- doesn't help to erase the data on lfs, but doesn't
	 * hurt either.  No need to unlink because update_seed will
	 * have already renamed over it.
	 */
	if (!ro) {
		memset(&rs, 0, sizeof rs);
		if ((size_t)(nwrit = pwrite(fd_old, &rs, sizeof rs, 0)) !=
		    sizeof rs) {
			if (nwrit == -1)
				err(1, "overwrite old seed");
			else
				errx(1, "truncated overwrite");
		}
		if (fsync_range(fd_old, FDATASYNC|FDISKSYNC, 0, 0) == -1)
			err(1, "fsync_range");
	}

	/* Fail noisily if anything went wrong.  */
	if (fail)
		exit(1);
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
strflags(uint32_t totalbits, u_int32_t fl)
{
	static char str[512];

	str[0] = '\0';
	if (totalbits > 0 && (fl & RND_FLAG_NO_ESTIMATE) == 0)
		strlcat(str, "estimate, ", sizeof(str));

	if ((fl & RND_FLAG_NO_COLLECT) == 0)
		strlcat(str, "collect, ", sizeof(str));

	if (fl & RND_FLAG_COLLECT_VALUE)
		strlcat(str, "v, ", sizeof(str));
	if (fl & RND_FLAG_COLLECT_TIME)
		strlcat(str, "t, ", sizeof(str));

	if (str[strlen(str) - 2] == ',')
		str[strlen(str) - 2] = '\0';

	return (str);
}

#define HEADER "Source       Estimated bits    Samples Type   Flags\n"

static void
do_print_source(rndsource_est_t *source)
{
	printf("%-16s ", source->rt.name);
	printf("%10" PRIu32 " ", source->rt.total);
	printf("%10" PRIu32 " ", source->dt_samples + source->dv_samples);
	printf("%-6s ", find_name(source->rt.type));
	printf("%s\n", strflags(source->rt.total, source->rt.flags));
}

static void
do_print_source_verbose(rndsource_est_t *source)
{
	printf("\tDt samples = %d\n", source->dt_samples);
	printf("\tDt bits = %d\n", source->dt_total);
	printf("\tDv samples = %d\n", source->dv_samples);
	printf("\tDv bits = %d\n", source->dv_total);
}

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

	if (!all && type == 0xff) {
		strncpy(rstat_name.name, name, sizeof(rstat_name.name));
		res = ioctl(fd, RNDGETESTNAME, &rstat_name);
		if (res < 0)
			err(1, "ioctl(RNDGETESTNAME)");
		printf(HEADER);
		do_print_source(&rstat_name.source);
		if (vflag)
			do_print_source_verbose(&rstat_name.source);
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
			if (all || type == rstat.source[i].rt.type) {
				do_print_source(&rstat.source[i]);
				if (vflag)
					do_print_source_verbose(&rstat.source[i]);
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

	printf("\t%9u bits currently stored in pool (max %u)\n",
	    rs.curentropy, rs.maxentropy);

	close(fd);
}

int
main(int argc, char **argv)
{
	rndctl_t rctl;
	int ch, cmd, lflag, mflag, sflag;
	u_int32_t type;
	char name[16] = "";
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

	while ((ch = getopt(argc, argv, "CES:L:celit:d:sv")) != -1) {
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
		case 'i':
			iflag = 1;
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
	 * -i makes sense only with -L.
	 */
	if (iflag && cmd != 'L')
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
