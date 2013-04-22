/*	$NetBSD: mount.c,v 1.4 2013/04/22 13:28:28 yamt Exp $	*/

/*-
 * Copyright (c)2010,2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount.c,v 1.4 2013/04/22 13:28:28 yamt Exp $");
#endif /* not lint */

#include <err.h>
#include <errno.h>
#include <locale.h>
#include <mntopts.h>
#include <paths.h>
#include <puffs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "pgfs.h"
#include "pgfs_db.h"

#define	PGFS_MNT_ALT_DUMMY	1
#define	PGFS_MNT_ALT_DEBUG	2

static char *
xstrcpy(const char *str)
{
	char *n;
	size_t len;

	if (str == NULL) {
		return NULL;
	}
	len = strlen(str);
	n = emalloc(len + 1);
	memcpy(n, str, len + 1);
	return n;
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	mntoptparse_t mp;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	int mntflags;
	int altmntflags;
	int ch;
	int error;
	const char *dbname = NULL;
	const char *dbuser = NULL;
	static const struct mntopt mopts[] = {
		MOPT_STDOPTS,
		MOPT_SYNC,
		{ .m_option = "dbname", .m_inverse = 0,
		  .m_flag = PGFS_MNT_ALT_DUMMY, .m_altloc = 1, },
		{ .m_option = "dbuser", .m_inverse = 0,
		  .m_flag = PGFS_MNT_ALT_DUMMY, .m_altloc = 1, },
		{ .m_option = "debug", .m_inverse = 0,
		  .m_flag = PGFS_MNT_ALT_DEBUG, .m_altloc = 1, },
		{ .m_option = "nconn", .m_inverse = 0,
		  .m_flag = PGFS_MNT_ALT_DUMMY, .m_altloc = 1, },
		MOPT_NULL,
	};
	uint32_t pflags = PUFFS_KFLAG_IAONDEMAND;
	unsigned int nconn = 8;
	bool debug = false;
	bool dosync;

	setlocale(LC_ALL, "");

	mntflags = 0;
	altmntflags = 0;
	while ((ch = getopt(argc, argv, "o:")) != -1) {
		long v;

		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags,
			    &altmntflags);
			if (mp == NULL) {
				err(EXIT_FAILURE, "getmntopts");
			}
			getmnt_silent = 1; /* XXX silly api */
			dbname = xstrcpy(getmntoptstr(mp, "dbname"));
			dbuser = xstrcpy(getmntoptstr(mp, "dbuser"));
			v = getmntoptnum(mp, "nconn");
			getmnt_silent = 0;
			if (v != -1) {
				nconn = v;
			}
			if ((altmntflags & PGFS_MNT_ALT_DEBUG) != 0) {
				debug = true;
			}
			freemntopts(mp);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	PUFFSOP_INIT(pops);
	PUFFSOP_SETFSNOP(pops, unmount);
	PUFFSOP_SETFSNOP(pops, sync);
	PUFFSOP_SET(pops, pgfs, fs, statvfs);
	PUFFSOP_SET(pops, pgfs, node, readdir);
	PUFFSOP_SET(pops, pgfs, node, getattr);
	PUFFSOP_SET(pops, pgfs, node, lookup);
	PUFFSOP_SET(pops, pgfs, node, mkdir);
	PUFFSOP_SET(pops, pgfs, node, create);
	PUFFSOP_SET(pops, pgfs, node, read);
	PUFFSOP_SET(pops, pgfs, node, write);
	PUFFSOP_SET(pops, pgfs, node, link);
	PUFFSOP_SET(pops, pgfs, node, remove);
	PUFFSOP_SET(pops, pgfs, node, rmdir);
	PUFFSOP_SET(pops, pgfs, node, inactive);
	PUFFSOP_SET(pops, pgfs, node, setattr);
	PUFFSOP_SET(pops, pgfs, node, rename);
	PUFFSOP_SET(pops, pgfs, node, symlink);
	PUFFSOP_SET(pops, pgfs, node, readlink);
	PUFFSOP_SET(pops, pgfs, node, access);
	dosync = (mntflags & MNT_SYNCHRONOUS) != 0;
	if (!dosync) {
		PUFFSOP_SET(pops, pgfs, node, fsync);
	}
	if (debug) {
		pflags |= PUFFS_FLAG_OPDUMP;
	}
	pu = puffs_init(pops, _PATH_PUFFS, "pgfs", NULL, pflags);
	if (pu == NULL) {
		err(EXIT_FAILURE, "puffs_init");
	}
	error = pgfs_connectdb(pu, dbname, dbuser, debug, dosync, nconn);
	free(__UNCONST(dbname));
	free(__UNCONST(dbuser));
	if (error != 0) {
		errno = error;
		err(EXIT_FAILURE, "pgfs_connectdb");
	}
	if (!debug) {
		if (puffs_daemon(pu, 1, 1)) {
			err(EXIT_FAILURE, "puffs_daemon");
		}
	}
	if (puffs_mount(pu, argv[1], mntflags, pgfs_root_cookie()) == -1) {
		err(EXIT_FAILURE, "puffs_mount");
	}
	if (!debug) {
		char tmpl[] = "/tmp/pgfs.XXXXXXXX";
		const char *path;
		int fd;
		int ret;

		path = mkdtemp(tmpl);
		if (path == NULL) {
			err(EXIT_FAILURE, "mkdtemp");
		}
		fd = open(path, O_RDONLY | O_DIRECTORY);
		if (fd == -1) {
			err(EXIT_FAILURE, "open %s", path);
		}
		ret = rmdir(path);
		if (ret != 0) {
			err(EXIT_FAILURE, "rmdir %s", path);
		}
		ret = fchroot(fd);
		if (ret != 0) {
			err(EXIT_FAILURE, "fchroot");
		}
		ret = close(fd);
		if (ret != 0) {
			err(EXIT_FAILURE, "close");
		}
	}
	if (puffs_mainloop(pu) == -1) {
		err(EXIT_FAILURE, "puffs_mainloop");
	}
	exit(EXIT_SUCCESS);
}
