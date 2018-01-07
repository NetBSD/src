/*	$NetBSD: dkctl.c,v 1.26 2018/01/07 12:29:25 kre Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * dkctl(8) -- a program to manipulate disks.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: dkctl.c,v 1.26 2018/01/07 12:29:25 kre Exp $");
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#include <sys/queue.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>     
#include <unistd.h>
#include <util.h>

#define	YES	1
#define	NO	0

/* I don't think nl_langinfo is suitable in this case */
#define	YES_STR	"yes"
#define	NO_STR	"no"
#define YESNO_ARG	YES_STR " | " NO_STR

#ifndef PRIdaddr
#define PRIdaddr PRId64
#endif

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func)(int, char *[]);
	int open_flags;
};

static struct command *lookup(const char *);
__dead static void	usage(void);
static void	run(int, char *[]);
static void	showall(void);

static int	fd;				/* file descriptor for device */
static const	char *dvname;			/* device name */
static char	dvname_store[MAXPATHLEN];	/* for opendisk(3) */

static int dkw_sort(const void *, const void *);
static int yesno(const char *);

static void	disk_getcache(int, char *[]);
static void	disk_setcache(int, char *[]);
static void	disk_synccache(int, char *[]);
static void	disk_keeplabel(int, char *[]);
static void	disk_badsectors(int, char *[]);

static void	disk_addwedge(int, char *[]);
static void	disk_delwedge(int, char *[]);
static void	disk_getwedgeinfo(int, char *[]);
static void	disk_listwedges(int, char *[]);
static void	disk_makewedges(int, char *[]);
static void	disk_strategy(int, char *[]);

static struct command commands[] = {
	{ "addwedge",
	  "name startblk blkcnt ptype",
	  disk_addwedge,
	  O_RDWR },

	{ "badsector",
	  "flush | list | retry",
	   disk_badsectors,
	   O_RDWR },

	{ "delwedge",
	  "dk",
	  disk_delwedge,
	  O_RDWR },

	{ "getcache",
	  "",
	  disk_getcache,
	  O_RDONLY },

	{ "getwedgeinfo",
	  "",
	  disk_getwedgeinfo,
	  O_RDONLY },

	{ "keeplabel",
	  YESNO_ARG,
	  disk_keeplabel,
	  O_RDWR },

	{ "listwedges",
	  "",
	  disk_listwedges,
	  O_RDONLY },

	{ "makewedges",
	  "",
	  disk_makewedges,
	  O_RDWR },

	{ "setcache",
	  "none | r | w | rw [save]",
	  disk_setcache,
	  O_RDWR },

	{ "strategy",
	  "[name]",
	  disk_strategy,
	  O_RDWR },

	{ "synccache",
	  "[force]",
	  disk_synccache,
	  O_RDWR },

	{ NULL,
	  NULL,
	  NULL,
	  0 },
};

int
main(int argc, char *argv[])
{

	/* Must have at least: device command */
	if (argc < 2)
		usage();

	dvname = argv[1];
	if (argc == 2)
		showall();
	else
		run(argc - 2, argv + 2);

	return EXIT_SUCCESS;
}

static void
run(int argc, char *argv[])
{
	struct command *command;

	command = lookup(argv[0]);

	/* Open the device. */
	fd = opendisk(dvname, command->open_flags, dvname_store,
	    sizeof(dvname_store), 0);
	if (fd == -1)
		err(EXIT_FAILURE, "%s", dvname);
	dvname = dvname_store;

	(*command->cmd_func)(argc, argv);

	/* Close the device. */
	(void)close(fd);
}

static struct command *
lookup(const char *name)
{
	int i;

	/* Look up the command. */
	for (i = 0; commands[i].cmd_name != NULL; i++)
		if (strcmp(name, commands[i].cmd_name) == 0)
			break;
	if (commands[i].cmd_name == NULL)
		errx(EXIT_FAILURE, "unknown command: %s", name);

	return &commands[i];
}

static void
usage(void)
{
	int i;

	fprintf(stderr,
	    "Usage: %s device\n"
	    "       %s device command [arg [...]]\n",
	    getprogname(), getprogname());

	fprintf(stderr, "   Available commands:\n");
	for (i = 0; commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", commands[i].cmd_name,
		    commands[i].arg_names);

	exit(EXIT_FAILURE);
}

static void
showall(void)
{
	static const char *cmds[] = { "strategy", "getcache", "listwedges" };
	size_t i;
	char *args[2];

	args[1] = NULL;
	for (i = 0; i < __arraycount(cmds); i++) {
		printf("%s:\n", cmds[i]);
		args[0] = __UNCONST(cmds[i]);
		run(1, args);
		putchar('\n');
	}
}

static void
disk_strategy(int argc, char *argv[])
{
	struct disk_strategy odks;
	struct disk_strategy dks;

	memset(&dks, 0, sizeof(dks));
	if (ioctl(fd, DIOCGSTRATEGY, &odks) == -1) {
		err(EXIT_FAILURE, "%s: DIOCGSTRATEGY", dvname);
	}

	memset(&dks, 0, sizeof(dks));
	switch (argc) {
	case 1:
		/* show the buffer queue strategy used */
		printf("%s: %s\n", dvname, odks.dks_name);
		return;
	case 2:
		/* set the buffer queue strategy */
		strlcpy(dks.dks_name, argv[1], sizeof(dks.dks_name));
		if (ioctl(fd, DIOCSSTRATEGY, &dks) == -1) {
			err(EXIT_FAILURE, "%s: DIOCSSTRATEGY", dvname);
		}
		printf("%s: %s -> %s\n", dvname, odks.dks_name, argv[1]);
		break;
	default:
		usage();
		/* NOTREACHED */
	}
}

static void
disk_getcache(int argc, char *argv[])
{
	int bits;

	if (ioctl(fd, DIOCGCACHE, &bits) == -1)
		err(EXIT_FAILURE, "%s: getcache", dvname);

	if ((bits & (DKCACHE_READ|DKCACHE_WRITE)) == 0)
		printf("%s: No caches enabled\n", dvname);
	else {
		if (bits & DKCACHE_READ)
			printf("%s: read cache enabled\n", dvname);
		if (bits & DKCACHE_WRITE)
			printf("%s: write-back cache enabled\n", dvname);
	}

	printf("%s: read cache enable is %schangeable\n", dvname,
	    (bits & DKCACHE_RCHANGE) ? "" : "not ");
	printf("%s: write cache enable is %schangeable\n", dvname,
	    (bits & DKCACHE_WCHANGE) ? "" : "not ");

	printf("%s: cache parameters are %ssavable\n", dvname,
	    (bits & DKCACHE_SAVE) ? "" : "not ");

#ifdef DKCACHE_FUA
	printf("%s: cache Force Unit Access (FUA) %ssupported\n", dvname,
	    (bits & DKCACHE_FUA) ? "" : "not ");
#endif /* DKCACHE_FUA */

#ifdef DKCACHE_DPO
	printf("%s: cache Disable Page Out (DPO) %ssupported\n", dvname,
	    (bits & DKCACHE_DPO) ? "" : "not ");
#endif /* DKCACHE_DPO */
}

static void
disk_setcache(int argc, char *argv[])
{
	int bits;

	if (argc > 3 || argc == 1)
		usage();

	if (strcmp(argv[1], "none") == 0)
		bits = 0;
	else if (strcmp(argv[1], "r") == 0)
		bits = DKCACHE_READ;
	else if (strcmp(argv[1], "w") == 0)
		bits = DKCACHE_WRITE;
	else if (strcmp(argv[1], "rw") == 0)
		bits = DKCACHE_READ|DKCACHE_WRITE;
	else
		usage();

	if (argc == 3) {
		if (strcmp(argv[2], "save") == 0)
			bits |= DKCACHE_SAVE;
		else
			usage();
	}

	if (ioctl(fd, DIOCSCACHE, &bits) == -1)
		err(EXIT_FAILURE, "%s: %s", dvname, argv[0]);
}

static void
disk_synccache(int argc, char *argv[])
{
	int force;

	switch (argc) {
	case 1:
		force = 0;
		break;

	case 2:
		if (strcmp(argv[1], "force") == 0)
			force = 1;
		else
			usage();
		break;

	default:
		usage();
	}

	if (ioctl(fd, DIOCCACHESYNC, &force) == -1)
		err(EXIT_FAILURE, "%s: %s", dvname, argv[0]);
}

static void
disk_keeplabel(int argc, char *argv[])
{
	int keep;
	int yn;

	if (argc != 2)
		usage();

	yn = yesno(argv[1]);
	if (yn < 0)
		usage();

	keep = yn == YES;

	if (ioctl(fd, DIOCKLABEL, &keep) == -1)
		err(EXIT_FAILURE, "%s: %s", dvname, argv[0]);
}


static void
disk_badsectors(int argc, char *argv[])
{
	struct disk_badsectors *dbs, *dbs2, buffer[200];
	SLIST_HEAD(, disk_badsectors) dbstop;
	struct disk_badsecinfo dbsi;
	daddr_t blk, totbad, bad;
	u_int32_t count;
	struct stat sb;
	u_char *block;
	time_t tm;

	if (argc != 2)
		usage();

	if (strcmp(argv[1], "list") == 0) {
		/*
		 * Copy the list of kernel bad sectors out in chunks that fit
		 * into buffer[].  Updating dbsi_skip means we don't sit here
		 * forever only getting the first chunk that fit in buffer[].
		 */
		dbsi.dbsi_buffer = (caddr_t)buffer;
		dbsi.dbsi_bufsize = sizeof(buffer);
		dbsi.dbsi_skip = 0;
		dbsi.dbsi_copied = 0;
		dbsi.dbsi_left = 0;

		do {
			if (ioctl(fd, DIOCBSLIST, (caddr_t)&dbsi) == -1)
				err(EXIT_FAILURE, "%s: badsectors list", dvname);

			dbs = (struct disk_badsectors *)dbsi.dbsi_buffer;
			for (count = dbsi.dbsi_copied; count > 0; count--) {
				tm = dbs->dbs_failedat.tv_sec;
				printf("%s: blocks %" PRIdaddr " - %" PRIdaddr " failed at %s",
					dvname, dbs->dbs_min, dbs->dbs_max,
					ctime(&tm));
				dbs++;
			}
			dbsi.dbsi_skip += dbsi.dbsi_copied;
		} while (dbsi.dbsi_left != 0);

	} else if (strcmp(argv[1], "flush") == 0) {
		if (ioctl(fd, DIOCBSFLUSH) == -1)
			err(EXIT_FAILURE, "%s: badsectors flush", dvname);

	} else if (strcmp(argv[1], "retry") == 0) {
		/*
		 * Enforce use of raw device here because the block device
		 * causes access to blocks to be clustered in a larger group,
		 * making it impossible to determine which individual sectors
		 * are the cause of a problem.
		 */ 
		if (fstat(fd, &sb) == -1)
			err(EXIT_FAILURE, "fstat");

		if (!S_ISCHR(sb.st_mode)) {
			fprintf(stderr, "'badsector retry' must be used %s\n",
				"with character device");
			exit(1);
		}

		SLIST_INIT(&dbstop);

		/*
		 * Build up a copy of the in-kernel list in a number of stages.
		 * That the list we build up here is in the reverse order to
		 * the kernel's is of no concern.
		 */
		dbsi.dbsi_buffer = (caddr_t)buffer;
		dbsi.dbsi_bufsize = sizeof(buffer);
		dbsi.dbsi_skip = 0;
		dbsi.dbsi_copied = 0;
		dbsi.dbsi_left = 0;

		do {
			if (ioctl(fd, DIOCBSLIST, (caddr_t)&dbsi) == -1)
				err(EXIT_FAILURE, "%s: badsectors list", dvname);

			dbs = (struct disk_badsectors *)dbsi.dbsi_buffer;
			for (count = dbsi.dbsi_copied; count > 0; count--) {
				dbs2 = malloc(sizeof *dbs2);
				if (dbs2 == NULL)
					err(EXIT_FAILURE, NULL);
				*dbs2 = *dbs;
				SLIST_INSERT_HEAD(&dbstop, dbs2, dbs_next);
				dbs++;
			}
			dbsi.dbsi_skip += dbsi.dbsi_copied;
		} while (dbsi.dbsi_left != 0);

		/*
		 * Just calculate and print out something that will hopefully
		 * provide some useful information about what's going to take
		 * place next (if anything.)
		 */
		bad = 0;
		totbad = 0;
		if ((block = calloc(1, DEV_BSIZE)) == NULL)
			err(EXIT_FAILURE, NULL);
		SLIST_FOREACH(dbs, &dbstop, dbs_next) {
			bad++;
			totbad += dbs->dbs_max - dbs->dbs_min + 1;
		}

		printf("%s: bad sector clusters %"PRIdaddr
		    " total sectors %"PRIdaddr"\n", dvname, bad, totbad);

		/*
		 * Clear out the kernel's list of bad sectors, ready for us
		 * to test all those it thought were bad.
		 */
		if (ioctl(fd, DIOCBSFLUSH) == -1)
			err(EXIT_FAILURE, "%s: badsectors flush", dvname);

		printf("%s: bad sectors flushed\n", dvname);

		/*
		 * For each entry we obtained from the kernel, retry each
		 * individual sector recorded as bad by seeking to it and
		 * attempting to read it in.  Print out a line item for each
		 * bad block we verify.
		 *
		 * PRIdaddr is used here because the type of dbs_max is daddr_t
		 * and that may be either a 32bit or 64bit number(!)
		 */
		SLIST_FOREACH(dbs, &dbstop, dbs_next) {
			printf("%s: Retrying %"PRIdaddr" - %"
			    PRIdaddr"\n", dvname, dbs->dbs_min, dbs->dbs_max);

			for (blk = dbs->dbs_min; blk <= dbs->dbs_max; blk++) {
				if (lseek(fd, (off_t)blk * DEV_BSIZE,
				    SEEK_SET) == -1) {
					warn("%s: lseek block %" PRIdaddr "",
					    dvname, blk);
					continue;
				}
				printf("%s: block %"PRIdaddr" - ", dvname, blk);
				if (read(fd, block, DEV_BSIZE) != DEV_BSIZE)
					printf("failed\n");
				else
					printf("ok\n");
				fflush(stdout);
			}
		}
	}
}

static void
disk_addwedge(int argc, char *argv[])
{
	struct dkwedge_info dkw;
	char *cp;
	daddr_t start;
	uint64_t size;

	if (argc != 5)
		usage();

	/* XXX Unicode: dkw_wname is supposed to be utf-8 */
	if (strlcpy((char *)dkw.dkw_wname, argv[1], sizeof(dkw.dkw_wname)) >=
	    sizeof(dkw.dkw_wname))
		errx(EXIT_FAILURE, "Wedge name too long; max %zd characters",
		    sizeof(dkw.dkw_wname) - 1);

	if (strlcpy(dkw.dkw_ptype, argv[4], sizeof(dkw.dkw_ptype)) >=
	    sizeof(dkw.dkw_ptype))
		errx(EXIT_FAILURE, "Wedge partition type too long; max %zd characters",
		    sizeof(dkw.dkw_ptype) - 1);

	errno = 0;
	start = strtoll(argv[2], &cp, 0);
	if (*cp != '\0')
		errx(EXIT_FAILURE, "Invalid start block: %s", argv[2]);
	if (errno == ERANGE && (start == LLONG_MAX ||
				start == LLONG_MIN))
		errx(EXIT_FAILURE, "Start block out of range.");
	if (start < 0)
		errx(EXIT_FAILURE, "Start block must be >= 0.");

	errno = 0;
	size = strtoull(argv[3], &cp, 0);
	if (*cp != '\0')
		errx(EXIT_FAILURE, "Invalid block count: %s", argv[3]);
	if (errno == ERANGE && (size == ULLONG_MAX))
		errx(EXIT_FAILURE, "Block count out of range.");

	dkw.dkw_offset = start;
	dkw.dkw_size = size;

	if (ioctl(fd, DIOCAWEDGE, &dkw) == -1)
		err(EXIT_FAILURE, "%s: %s", dvname, argv[0]);
	else
		printf("%s created successfully.\n", dkw.dkw_devname);

}

static void
disk_delwedge(int argc, char *argv[])
{
	struct dkwedge_info dkw;

	if (argc != 2)
		usage();

	if (strlcpy(dkw.dkw_devname, argv[1], sizeof(dkw.dkw_devname)) >=
	    sizeof(dkw.dkw_devname))
		errx(EXIT_FAILURE, "Wedge dk name too long; max %zd characters",
		    sizeof(dkw.dkw_devname) - 1);

	if (ioctl(fd, DIOCDWEDGE, &dkw) == -1)
		err(EXIT_FAILURE, "%s: %s", dvname, argv[0]);
}

static void
disk_getwedgeinfo(int argc, char *argv[])
{
	struct dkwedge_info dkw;

	if (argc != 1)
		usage();

	if (ioctl(fd, DIOCGWEDGEINFO, &dkw) == -1)
		err(EXIT_FAILURE, "%s: getwedgeinfo", dvname);

	printf("%s at %s: %s\n", dkw.dkw_devname, dkw.dkw_parent,
	    dkw.dkw_wname);	/* XXX Unicode */
	printf("%s: %"PRIu64" blocks at %"PRId64", type: %s\n",
	    dkw.dkw_devname, dkw.dkw_size, dkw.dkw_offset, dkw.dkw_ptype);
}

static void
disk_listwedges(int argc, char *argv[])
{
	struct dkwedge_info *dkw;
	struct dkwedge_list dkwl;
	size_t bufsize;
	u_int i;
	int c;
	bool error, quiet;

	optreset = 1;
	optind = 1;
	quiet = error = false;
	while ((c = getopt(argc, argv, "qe")) != -1)
		switch (c) {
		case 'e':
			error = true;
			break;
		case 'q':
			quiet = true;
			break;
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	dkw = NULL;
	dkwl.dkwl_buf = dkw;
	dkwl.dkwl_bufsize = 0;

	for (;;) {
		if (ioctl(fd, DIOCLWEDGES, &dkwl) == -1)
			err(EXIT_FAILURE, "%s: listwedges", dvname);
		if (dkwl.dkwl_nwedges == dkwl.dkwl_ncopied)
			break;
		bufsize = dkwl.dkwl_nwedges * sizeof(*dkw);
		if (dkwl.dkwl_bufsize < bufsize) {
			dkw = realloc(dkwl.dkwl_buf, bufsize);
			if (dkw == NULL)
				errx(EXIT_FAILURE, "%s: listwedges: unable to "
				    "allocate wedge info buffer", dvname);
			dkwl.dkwl_buf = dkw;
			dkwl.dkwl_bufsize = bufsize;
		}
	}

	if (dkwl.dkwl_nwedges == 0) {
		if (!quiet)
			printf("%s: no wedges configured\n", dvname);
		if (error)
			exit(EXIT_FAILURE);
		return;
	}

	if (quiet)
		return;

	qsort(dkw, dkwl.dkwl_nwedges, sizeof(*dkw), dkw_sort);

	printf("%s: %u wedge%s:\n", dvname, dkwl.dkwl_nwedges,
	    dkwl.dkwl_nwedges == 1 ? "" : "s");
	for (i = 0; i < dkwl.dkwl_nwedges; i++) {
		printf("%s: %s, %"PRIu64" blocks at %"PRId64", type: %s\n",
		    dkw[i].dkw_devname,
		    dkw[i].dkw_wname,	/* XXX Unicode */
		    dkw[i].dkw_size, dkw[i].dkw_offset, dkw[i].dkw_ptype);
	}
}

static void
disk_makewedges(int argc, char *argv[])
{
	int bits;

	if (argc != 1)
		usage();

	if (ioctl(fd, DIOCMWEDGES, &bits) == -1)
		err(EXIT_FAILURE, "%s: %s", dvname, argv[0]);
	else
		printf("successfully scanned %s.\n", dvname);
}

static int
dkw_sort(const void *a, const void *b)
{
	const struct dkwedge_info *dkwa = a, *dkwb = b;
	const daddr_t oa = dkwa->dkw_offset, ob = dkwb->dkw_offset;

	return (oa < ob) ? -1 : (oa > ob) ? 1 : 0;
}

/*
 * return YES, NO or -1.
 */
static int
yesno(const char *p)
{

	if (!strcmp(p, YES_STR))
		return YES;
	if (!strcmp(p, NO_STR))
		return NO;
	return -1;
}
