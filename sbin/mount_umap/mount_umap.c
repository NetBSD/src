/*	$NetBSD: mount_umap.c,v 1.19 2006/10/16 03:37:43 christos Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_umap.c	8.5 (Berkeley) 4/26/95";
#else
__RCSID("$NetBSD: mount_umap.c,v 1.19 2006/10/16 03:37:43 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <miscfs/umapfs/umap.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mntopts.h>

#define ROOTUSER 0
/*
 * This define controls whether any user but the superuser can own and
 * write mapfiles.  If other users can, system security can be gravely
 * compromised.  If this is not a concern, undefine SECURITY.
 */
#define MAPSECURITY 1

/*
 * This routine provides the user interface to mounting a umap layer.
 * It takes 4 mandatory parameters.  The mandatory arguments are the place 
 * where the next lower level is mounted, the place where the umap layer is to
 * be mounted, the name of the user mapfile, and the name of the group
 * mapfile.  The routine checks the ownerships and permissions on the
 * mapfiles, then opens and reads them.  Then it calls mount(), which
 * will, in turn, call the umap version of mount. 
 */

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_NULL,
};

int	mount_umap(int argc, char **argv);
static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_umap(argc, argv);
}
#endif

int
mount_umap(int argc, char *argv[])
{
	static char not[] = "; not mounted.";
	struct stat statbuf;
	struct umap_args args;
	FILE *fp, *gfp;
	long d1, d2;
	u_long mapdata[MAPFILEENTRIES][2];
	u_long gmapdata[GMAPFILEENTRIES][2];
	int ch, count, gnentries, mntflags, nentries;
	char *gmapfile, *mapfile, buf[20];
	char source[MAXPATHLEN], target[MAXPATHLEN];
	mntoptparse_t mp;

	mntflags = 0;
	mapfile = gmapfile = NULL;
	while ((ch = getopt(argc, argv, "g:o:u:")) != -1)
		switch (ch) {
		case 'g':
			gmapfile = optarg;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 'u':
			mapfile = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2 || mapfile == NULL || gmapfile == NULL)
		usage();

	if (realpath(argv[0], source) == NULL)        /* Check source path */
		err(1, "realpath %s", argv[0]);
	if (strncmp(argv[0], source, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[0]);
		warnx("using \"%s\" instead.", source);
	}

	if (realpath(argv[1], target) == NULL)        /* Check mounton path */
		err(1, "realpath %s", argv[1]);
	if (strncmp(argv[1], target, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[1]);
		warnx("using \"%s\" instead.", target);
	}

	/* Read in uid mapping data. */
	if ((fp = fopen(mapfile, "r")) == NULL)
		err(1, "%s%s", mapfile, not);

#ifdef MAPSECURITY
	/*
	 * Check that group and other don't have write permissions on
	 * this mapfile, and that the mapfile belongs to root. 
	 */
	if (fstat(fileno(fp), &statbuf))
		err(1, "%s%s", mapfile, not);
	if (statbuf.st_mode & S_IWGRP || statbuf.st_mode & S_IWOTH) {
		strmode(statbuf.st_mode, buf);
		err(1, "%s: improper write permissions (%s)%s",
		    mapfile, buf, not);
	}
	if (statbuf.st_uid != ROOTUSER)
		errx(1, "%s does not belong to root%s", mapfile, not);
#endif /* MAPSECURITY */

	if ((fscanf(fp, "%d\n", &nentries)) != 1)
		errx(1, "%s: nentries not found%s", mapfile, not);
	if (nentries > MAPFILEENTRIES)
		errx(1,
		    "maximum number of entries is %d%s", MAPFILEENTRIES, not);
#if 0
	(void)printf("reading %d entries\n", nentries);
#endif
	for (count = 0; count < nentries; ++count) {
		if ((fscanf(fp, "%lu %lu\n", &d1, &d2)) != 2) {
			if (ferror(fp))
				err(1, "%s%s", mapfile, not);
			if (feof(fp))
				errx(1, "%s: unexpected end-of-file%s",
				    mapfile, not);
			errx(1, "%s: illegal format (line %d)%s",
			    mapfile, count + 2, not);
		}
		mapdata[count][0] = d1;
		mapdata[count][1] = d2;
#if 0
		/* Fix a security hole. */
		if (mapdata[count][1] == 0)
			errx(1, "mapping id 0 not permitted (line %d)%s",
			    count + 2, not);
#endif
	}

	/* Read in gid mapping data. */
	if ((gfp = fopen(gmapfile, "r")) == NULL)
		err(1, "%s%s", gmapfile, not);

#ifdef MAPSECURITY
	/*
	 * Check that group and other don't have write permissions on
	 * this group mapfile, and that the file belongs to root. 
	 */
	if (fstat(fileno(gfp), &statbuf))
		err(1, "%s%s", gmapfile, not);
	if (statbuf.st_mode & S_IWGRP || statbuf.st_mode & S_IWOTH) {
		strmode(statbuf.st_mode, buf);
		err(1, "%s: improper write permissions (%s)%s",
		    gmapfile, buf, not);
	}
	if (statbuf.st_uid != ROOTUSER)
		errx(1, "%s does not belong to root%s", gmapfile, not);
#endif /* MAPSECURITY */

	if ((fscanf(gfp, "%d\n", &gnentries)) != 1)
		errx(1, "nentries not found%s", not);
	if (gnentries > MAPFILEENTRIES)
		errx(1,
		    "maximum number of entries is %d%s", GMAPFILEENTRIES, not);
#if 0
	(void)printf("reading %d group entries\n", gnentries);
#endif

	for (count = 0; count < gnentries; ++count) {
		if ((fscanf(gfp, "%lu %lu\n",
		    &(gmapdata[count][0]), &(gmapdata[count][1]))) != 2) {
			if (ferror(gfp))
				err(1, "%s%s", gmapfile, not);
			if (feof(gfp))
				errx(1, "%s: unexpected end-of-file%s",
				    gmapfile, not);
			errx(1, "%s: illegal format (line %d)%s",
			    gmapfile, count + 2, not);
		}
	}


	/* Setup mount call args. */
	args.la.target = source;
	args.nentries = nentries;
	args.mapdata = mapdata;
	args.gnentries = gnentries;
	args.gmapdata = gmapdata;

	if (mount(MOUNT_UMAP, target, mntflags, &args))
		err(1, "%s on %s", source, target);
	if (mntflags & MNT_GETARGS) {
		printf("nentries=%d, gnentries=%d\n", args.nentries,
		    args.gnentries);
	}
	exit(0);
}

static void
usage(void)
{
	(void)fprintf(stderr,
"usage: mount_umap [-o options] -g groupmap -u usermap target_fs mount_point\n");
	exit(1);
}
