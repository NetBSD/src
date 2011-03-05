/*	$NetBSD: rump_allserver.c,v 1.13.2.2 2011/03/05 15:11:00 bouyer Exp $	*/

/*-
 * Copyright (c) 2010, 2011 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rump_allserver.c,v 1.13.2.2 2011/03/05 15:11:00 bouyer Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/signal.h>
#include <sys/module.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-s] [-c ncpu] [-d drivespec] [-l libs] "
	    "[-m modules] bindurl\n", getprogname());
	exit(1);
}

static void
die(int sflag, int error, const char *reason)
{

	warnx("%s: %s", reason, strerror(error));
	if (!sflag)
		rump_daemonize_done(error);
	exit(1);
}

static sem_t sigsem;
static void
sigreboot(int sig)
{

	sem_post(&sigsem);
}

static const char *const disktokens[] = {
#define DKEY 0
	"key",
#define DFILE 1
	"hostpath",
#define DSIZE 2
#define DSIZE_E -1
	"size",
#define DOFFSET 3
	"offset",
#define DLABEL 4
	"disklabel",
#define DTYPE 5
	"type",
	NULL
};

struct etfsreg {
	const char *key;
	const char *hostpath;
	off_t flen;
	off_t foffset;
	char partition;
	enum rump_etfs_type type;
};

struct etfstype {
	const char *name;
	enum rump_etfs_type type;
} etfstypes[] = {
	{ "blk", RUMP_ETFS_BLK },
	{ "chr", RUMP_ETFS_CHR },
	{ "reg", RUMP_ETFS_REG },
};

int
main(int argc, char *argv[])
{
	const char *serverurl;
	char **modarray = NULL;
	unsigned nmods = 0, curmod = 0, i;
	struct etfsreg *etfs = NULL;
	unsigned netfs = 0, curetfs = 0;
	int error;
	int ch, sflag;
	int ncpu;

	setprogname(argv[0]);

	sflag = 0;
	while ((ch = getopt(argc, argv, "c:d:l:m:r:sv")) != -1) {
		switch (ch) {
		case 'c':
			ncpu = atoi(optarg);
			/* XXX: MAXCPUS is from host, not from kernel */
			if (ncpu < 1 || ncpu > MAXCPUS)
				err(1, "CPU count needs to be between "
				    "1 and %d\n", MAXCPUS);
			setenv("RUMP_NCPU", optarg, 1);
			break;
		case 'd': {
			char *options, *value;
			char *key, *hostpath;
			long long flen, foffset;
			char partition;
			int ftype;

			flen = foffset = 0;
			partition = 0;
			key = hostpath = NULL;
			ftype = -1;
			options = optarg;
			while (*options) {
				switch (getsubopt(&options,
				    __UNCONST(disktokens), &value)) {
				case DKEY:
					if (key != NULL) {
						fprintf(stderr,
						    "key already given\n");
						usage();
					}
					key = value;
					break;

				case DFILE:
					if (hostpath != NULL) {
						fprintf(stderr,
						    "hostpath already given\n");
						usage();
					}
					hostpath = value;
					break;

				case DSIZE:
					if (flen != 0) {
						fprintf(stderr,
						    "size already given\n");
						usage();
					}
					if (strcmp(value, "host") == 0) {
						if (foffset != 0) {
							fprintf(stderr,
							    "cannot specify "
							    "offset with "
							    "size=host\n");
							usage();
						}
						flen = DSIZE_E;
					} else {
						/* XXX: off_t max? */
						flen = strsuftoll("-d size",
						    value, 0, LLONG_MAX);
					}
					break;
				case DOFFSET:
					if (foffset != 0) {
						fprintf(stderr,
						    "offset already given\n");
						usage();
					}
					if (flen == DSIZE_E) {
						fprintf(stderr, "cannot "
						    "specify offset with "
						    "size=host\n");
						usage();
					}
					/* XXX: off_t max? */
					foffset = strsuftoll("-d offset", value,
					    0, LLONG_MAX);
					break;

				case DLABEL:
					if (foffset != 0 || flen != 0) {
						fprintf(stderr,
						    "disklabel needs to be "
						    "used alone\n");
						usage();
					}
					if (strlen(value) != 1 ||
					    *value < 'a' || *value > 'z') {
						fprintf(stderr,
						    "invalid label part\n");
						usage();
					}
					partition = *value;
					break;

				case DTYPE:
					if (ftype != -1) {
						fprintf(stderr,
						    "type already specified\n");
						usage();
					}

					for (i = 0;
					    i < __arraycount(etfstypes);
					    i++) {
						if (strcmp(etfstypes[i].name,
						    value) == 0)
							break;
					}
					if (i == __arraycount(etfstypes)) {
						fprintf(stderr,
						    "invalid type %s\n", value);
						usage();
					}
					ftype = etfstypes[i].type;
					break;

				default:
					fprintf(stderr, "invalid dtoken\n");
					usage();
					break;
				}
			}

			if (key == NULL || hostpath == NULL ||
			    (flen == 0 && partition == 0)) {
				fprintf(stderr, "incomplete drivespec\n");
				usage();
			}
			if (ftype == -1)
				ftype = RUMP_ETFS_BLK;

			if (netfs - curetfs == 0) {
				etfs = realloc(etfs, (netfs+16)*sizeof(*etfs));
				if (etfs == NULL)
					err(1, "realloc etfs");
				netfs += 16;
			}

			etfs[curetfs].key = key;
			etfs[curetfs].hostpath = hostpath;
			etfs[curetfs].flen = flen;
			etfs[curetfs].foffset = foffset;
			etfs[curetfs].partition = partition;
			etfs[curetfs].type = ftype;
			curetfs++;

			break;
		}
		case 'l':
			if (dlopen(optarg, RTLD_LAZY|RTLD_GLOBAL) == NULL) {
				char pb[MAXPATHLEN];
				/* try to mimic linker -l syntax */

				snprintf(pb, sizeof(pb), "lib%s.so", optarg);
				if (dlopen(pb, RTLD_LAZY|RTLD_GLOBAL) == NULL) {
					errx(1, "dlopen %s failed: %s",
					    pb, dlerror());
				}
			}
			break;
		case 'm':
			if (nmods - curmod == 0) {
				modarray = realloc(modarray,
				    (nmods+16) * sizeof(char *));
				if (modarray == NULL)
					err(1, "realloc");
				nmods += 16;
			}
			modarray[curmod++] = optarg;
			break;
		case 'r':
			setenv("RUMP_MEMLIMIT", optarg, 1);
			break;
		case 's':
			sflag = 1;
			break;
		case 'v':
			setenv("RUMP_VERBOSE", "1", 1);
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	serverurl = argv[0];

	if (!sflag) {
		error = rump_daemonize_begin();
		if (error)
			errx(1, "rump daemonize: %s", strerror(error));
	}

	error = rump_init();
	if (error)
		die(sflag, error, "rump init failed");

	/* load modules */
	for (i = 0; i < curmod; i++) {
		struct modctl_load ml;

#define ETFSKEY "/module.mod"
		if ((error = rump_pub_etfs_register(ETFSKEY,
		    modarray[0], RUMP_ETFS_REG)) != 0)
			die(sflag, error, "module etfs register failed");
		memset(&ml, 0, sizeof(ml));
		ml.ml_filename = ETFSKEY;
		if (rump_sys_modctl(MODCTL_LOAD, &ml) == -1)
			die(sflag, errno, "module load failed");
		rump_pub_etfs_remove(ETFSKEY);
#undef ETFSKEY
	}

	/* register host drives */
	for (i = 0; i < curetfs; i++) {
		char buf[1<<16];
		struct disklabel dl;
		struct stat sb;
		off_t foffset, flen, fendoff;
		int fd, oflags;

		oflags = etfs[i].flen == DSIZE_E ? 0 : O_CREAT;
		fd = open(etfs[i].hostpath, O_RDWR | oflags, 0644);
		if (fd == -1)
			die(sflag, errno, "etfs hostpath open");

		if (etfs[i].partition) {
			int partition = etfs[i].partition - 'a';

			pread(fd, buf, sizeof(buf), 0);
			if (disklabel_scan(&dl, buf, sizeof(buf)))
				die(sflag, ENOENT, "disklabel not found");

			if (partition >= dl.d_npartitions)
				die(sflag, ENOENT, "partition not available");

			foffset = dl.d_partitions[partition].p_offset
			    << DEV_BSHIFT;
			flen = dl.d_partitions[partition].p_size
			    << DEV_BSHIFT;
		} else {
			foffset = etfs[i].foffset;
			flen = etfs[i].flen;
		}

		if (fstat(fd, &sb) == -1)
			die(sflag, errno, "fstat etfs hostpath");
		if (flen == DSIZE_E) {
			if (sb.st_size == 0)
				die(sflag, EINVAL, "size=host, but cannot "
				    "query non-zero size");
			flen = sb.st_size;
		}
		fendoff = foffset + flen;
		if (S_ISREG(sb.st_mode) && sb.st_size < fendoff) {
			if (ftruncate(fd, fendoff) == -1)
				die(sflag, errno, "truncate");
		}
		close(fd);

		if ((error = rump_pub_etfs_register_withsize(etfs[i].key,
		    etfs[i].hostpath, etfs[i].type, foffset, flen)) != 0)
			die(sflag, error, "etfs register");
	}

	error = rump_init_server(serverurl);
	if (error)
		die(sflag, error, "rump server init failed");

	if (!sflag)
		rump_daemonize_done(RUMP_DAEMONIZE_SUCCESS);

	sem_init(&sigsem, 0, 0);
	signal(SIGTERM, sigreboot);
	signal(SIGINT, sigreboot);
	sem_wait(&sigsem);

	rump_sys_reboot(0, NULL);
	/*NOTREACHED*/

	return 0;
}
