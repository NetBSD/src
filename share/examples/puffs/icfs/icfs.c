/*	$NetBSD: icfs.c,v 1.3 2007/06/24 18:57:26 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
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

/*
 * icfs: layer everything in lowercase
 * (unfinished, as is probably fairly easy to tell)
 *
 * Now, this "layered" file system demostrates a nice gotcha with
 * mangling readdir entries.  In case we can't unmangle the name
 * (cf. rot13fs), we need to map the mangled name back to the real
 * name in lookup and store the actual lower layer name instead of
 * the mangled name.
 *
 * This is mounted nocache now (to disable the namecache since a
 * separate switch doesn't exist).  Otherwise we might have
 * two different nodes for e.g. FoO and foo.
 */

#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

PUFFSOP_PROTOS(ic)

static void usage(void);

static void
usage()
{

	errx(1, "usage: %s [-s] [-o mntopts] icfs mountpath",
	    getprogname());
}

static void
dotolower(char *buf, size_t buflen)
{

	while (buflen--) {
		*buf = tolower((unsigned char)*buf);
		buf++;
	}
}

static int
icpathcmp(struct puffs_usermount *pu, struct puffs_pathobj *c1,
	struct puffs_pathobj *c2, size_t clen, int checkprefix)
{
	struct puffs_pathobj *po_root;
	char *cp1, *cp2;
	size_t rplen;

	po_root = puffs_getrootpathobj(pu);
	rplen = po_root->po_len;

	assert(clen >= rplen);

	cp1 = c1->po_path;
	cp2 = c2->po_path;

	if (strncasecmp(cp1 + rplen, cp2 + rplen, clen - rplen) != 0)
		return 1;

	if (checkprefix == 0)
		return 0;

	if (c1->po_len < c2->po_len)
		return 1;

	if (*(cp1 + clen) != '/')
		return 1;

	return 0;
}

static int
icpathxform(struct puffs_usermount *pu, const struct puffs_pathobj *po_base,
	const struct puffs_cn *pcn, struct puffs_pathobj *po_new)
{
	struct dirent entry, *result;
	char *src;
	size_t srclen;
	DIR *dp;

	dp = opendir(po_base->po_path);
	if (dp == NULL)
		return errno;

	src = pcn->pcn_name;
	srclen = pcn->pcn_namelen;
	for (;;) {
		if (readdir_r(dp, &entry, &result) != 0)
			break;
		if (!result)
			break;
		if (strcasecmp(result->d_name, pcn->pcn_name) == 0) {
			src = result->d_name;
			srclen = strlen(result->d_name);
			break;
		}
	}

	po_new->po_path = strdup(src);
	po_new->po_len = srclen;
	closedir(dp);

	return 0;
}

int
main(int argc, char *argv[])
{
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	struct puffs_pathobj *po_root;
	struct puffs_node *pn_root;
	struct stat sb;
	mntoptparse_t mp;
	int mntflags, pflags, lflags;
	int ch;

	setprogname(argv[0]);

	if (argc < 3)
		usage();

	pflags = lflags = mntflags = 0;
	while ((ch = getopt(argc, argv, "o:s")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 's':
			lflags |= PUFFSLOOP_NODAEMON;
			break;
		}
	}
	pflags |= PUFFS_FLAG_BUILDPATH;
	pflags |= PUFFS_KFLAG_NOCACHE;
	argv += optind;
	argc -= optind;

	if (pflags & PUFFS_FLAG_OPDUMP)
		lflags = PUFFSLOOP_NODAEMON;

	if (argc != 2)
		usage();

	if (lstat(argv[0], &sb) == -1)
		err(1, "stat %s", argv[0]);
	if ((sb.st_mode & S_IFDIR) == 0)
		errx(1, "%s is not a directory", argv[0]);

	PUFFSOP_INIT(pops);
	puffs_null_setops(pops);

	PUFFSOP_SET(pops, ic, node, readdir);

	if ((pu = puffs_init(pops, "ic", NULL, pflags)) == NULL)
		err(1, "mount");

	pn_root = puffs_pn_new(pu, NULL);
	if (pn_root == NULL)
		err(1, "puffs_pn_new");
	puffs_setroot(pu, pn_root);

	po_root = puffs_getrootpathobj(pu);
	if (po_root == NULL)
		err(1, "getrootpathobj");
	po_root->po_path = argv[0];
	po_root->po_len = strlen(argv[0]);
	puffs_stat2vattr(&pn_root->pn_va, &sb);

	puffs_set_pathcmp(pu, icpathcmp);
	puffs_set_pathtransform(pu, icpathxform);

	if (puffs_mount(pu, argv[1], mntflags, pn_root) == -1)
		err(1, "puffs_mount");

	return puffs_mainloop(pu, lflags);
}

int
ic_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *dent,
	off_t *readoff, size_t *reslen, const struct puffs_cred *pcr,
	int *eofflag, off_t *cookies, size_t *ncookies)
{
	struct dirent *dp;
	size_t rl;
	int rv;

	dp = dent;
	rl = *reslen;

	rv = puffs_null_node_readdir(pcc, opc, dent, readoff, reslen, pcr,
	    eofflag, cookies, ncookies);
	if (rv)
		return rv;

	while (rl > *reslen) {
		dotolower(dp->d_name, dp->d_namlen);
		rl -= _DIRENT_SIZE(dp);
		dp = _DIRENT_NEXT(dp);
	}

	return 0;
}
