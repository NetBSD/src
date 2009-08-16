/*	$NetBSD: ldconfig.c,v 1.46 2009/08/16 18:01:49 martin Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: ldconfig.c,v 1.46 2009/08/16 18:01:49 martin Exp $");
#endif


#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/exec_aout.h>
#include <a.out.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <link_aout.h>
#include <paths.h>

#include "shlib.h"

#define _PATH_LD_SO_CONF "/etc/ld.so.conf"

#undef major
#undef minor

static int			verbose;
static int			nostd;
static int			noconf;
static int			justread;
static int			merge;

struct shlib_list {
	/* Internal list of shared libraries found */
	char			*name;
	char			*path;
	int			dewey[MAXDEWEY];
	int			ndewey;
#define major dewey[0]
#define minor dewey[1]
	struct shlib_list	*next;
};

static struct shlib_list	*shlib_head = NULL, **shlib_tail = &shlib_head;
static char			*dir_list;

static void	enter(char *, char *, char *, int *, int);
static int	dodir(char *, int, int);
static int	do_conf(void);
static int	buildhints(void);
static int	readhints(void);
static void	listhints(void);
static int	hinthash(char *, int, int);

int
main(int argc, char *argv[])
{
	int	i, c;
	int	rval = 0;

	while ((c = getopt(argc, argv, "cmrsSv")) != -1) {
		switch (c) {
		case 'c':
			noconf = 1;
			break;
		case 'm':
			merge = 1;
			break;
		case 'r':
			justread = 1;
			break;
		case 's':
			nostd = 1;
			noconf = 1;
			break;
		case 'S':
			nostd = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			errx(1, "usage: %s [-c][-m][-r][-s][-S][-v][dir ...]",
				getprogname());
			break;
		}
	}

	dir_list = xmalloc(1);
	*dir_list = '\0';

	if (justread || merge) {
		if ((rval = readhints()) != 0)
			return (rval);
		if (justread) {
			listhints();
			return (rval);
		}
	}

	if (!nostd && !merge)
		std_search_path();

	for (i = 0; i < n_search_dirs; i++)
		rval |= dodir(search_dirs[i], 1, 0);

	if (!noconf && !merge)
		rval |= do_conf();

	for (i = optind; i < argc; i++) {
		rval |= dodir(argv[i], 0, 1);
	}

	rval |= buildhints();

	return (rval);
}

int
do_conf(void)
{
	FILE		*conf;
	char		*line, *c;
	char		*cline = NULL;
	size_t		len;
	int		rval = 0;
#ifdef __ELF__
	char		*aout_conf;

	aout_conf = xmalloc(sizeof(_PATH_EMUL_AOUT) +
	    strlen(_PATH_LD_SO_CONF));
	strcpy(aout_conf, _PATH_EMUL_AOUT);
	strcat(aout_conf, _PATH_LD_SO_CONF);
	if ((conf = fopen(aout_conf, "r")) == NULL) {
		if (verbose)
			warnx("can't open `%s'", aout_conf);
		free(aout_conf);
		return (0);
	}
	free(aout_conf);
#else
	if ((conf = fopen(_PATH_LD_SO_CONF, "r")) == NULL) {
		if (verbose) {
			warnx("can't open `%s'", _PATH_LD_SO_CONF);
		}
		return (0);
	}
#endif

	while ((line = fgetln(conf, &len)) != NULL) {
		if (*line != '/')
			continue;

		if (line[len-1] == '\n') {
			line[--len] = '\0';
		} else {
			cline = xmalloc(len+1);
			memcpy(cline, line, len);
			line = cline;
			line[len] = '\0';
		}

		while (isblank(*line)) { line++; len--; }
		if ((c = strchr(line, '#')) == NULL)
			c = line + len;
		while (--c >= line && isblank(*c)) continue;
		if (c >= line) {
			*++c = '\0';
			rval |= dodir(line, 0, 1);
		}

		if (cline) {
			free(cline);
			cline = NULL;
		}
	}

	(void) fclose(conf);

	return (rval);
}

int
dodir(char *dir, int silent, int update_dir_list)
{
	DIR		*dd;
	struct dirent	*dp;
	char		name[MAXPATHLEN];
	int		dewey[MAXDEWEY], ndewey;

	if ((dd = opendir(dir)) == NULL) {
		/* /emul/aout directories are allowed to not exist.
		 */
		if (!strncmp(dir, _PATH_EMUL_AOUT, sizeof(_PATH_EMUL_AOUT)-1))
			return 0;
		if (!silent || errno != ENOENT)
			warn("%s", dir);
		return (-1);
	}

	if (update_dir_list) {
		/* Check for duplicates? */
		char *cp = concat(dir_list, *dir_list?":":"", dir);
		free(dir_list);
		dir_list = cp;
	}

	while ((dp = readdir(dd)) != NULL) {
		int n;
		char *cp, *path;
		FILE *fp;
		struct exec ex;

		/* Check for `lib' prefix */
		if (dp->d_name[0] != 'l' ||
		    dp->d_name[1] != 'i' ||
		    dp->d_name[2] != 'b')
			continue;

		/* Copy the entry minus prefix */
		(void)strcpy(name, dp->d_name + 3);
		n = strlen(name);
		if (n < 4)
			continue;

		/* Find ".so." in name */
		for (cp = name + n - 4; cp > name; --cp) {
			if (cp[0] == '.' &&
			    cp[1] == 's' &&
			    cp[2] == 'o' &&
			    cp[3] == '.')
				break;
		}
		if (cp <= name)
			continue;

		path = concat(dir, "/", dp->d_name);
		fp = fopen(path, "r");
		free(path);
		if (fp == NULL)
			continue;
		n = fread(&ex, 1, sizeof(ex), fp);
		fclose(fp);
		if (n != sizeof(ex)
		    || N_GETMAGIC(ex) != ZMAGIC
		    || (N_GETFLAG(ex) & EX_DYNAMIC) == 0)
			continue;

		*cp = '\0';
		if (!isdigit((unsigned char)*(cp+4)))
			continue;

		memset(dewey, 0, sizeof(dewey));
		ndewey = getdewey(dewey, cp + 4);
		enter(dir, dp->d_name, name, dewey, ndewey);
	}

	(void) closedir(dd);

	return (0);
}

static void
enter(char *dir, char *file, char *name, int dewey[], int ndewey)
{
	struct shlib_list	*shp;

	for (shp = shlib_head; shp; shp = shp->next) {
		if (strcmp(name, shp->name) != 0 || major != shp->major)
			continue;

		/* Name matches existing entry */
		if (cmpndewey(dewey, ndewey, shp->dewey, shp->ndewey) > 0) {

			/* Update this entry with higher versioned lib */
			if (verbose)
				printf("Updating lib%s.%d.%d to %s/%s\n",
					shp->name, shp->major, shp->minor,
					dir, file);

			free(shp->name);
			shp->name = strdup(name);
			free(shp->path);
			shp->path = concat(dir, "/", file);
			memcpy(shp->dewey, dewey, sizeof(shp->dewey));
			shp->ndewey = ndewey;
		}
		break;
	}

	if (shp)
		/* Name exists: older version or just updated */
		return;

	/* Allocate new list element */
	if (verbose)
		printf("Adding %s/%s\n", dir, file);

	shp = (struct shlib_list *)xmalloc(sizeof *shp);
	shp->name = strdup(name);
	shp->path = concat(dir, "/", file);
	memcpy(shp->dewey, dewey, sizeof(shp->dewey));
	shp->ndewey = ndewey;
	shp->next = NULL;

	*shlib_tail = shp;
	shlib_tail = &shp->next;
}


#if DEBUG
/* test */
#undef _PATH_LD_HINTS
#define _PATH_LD_HINTS		"./ld.so.hints"
#endif

/* XXX - should be a common function with rtld.c */
int
hinthash(char *cp, int vmajor, int vminor)
{
	int	k = 0;

	while (*cp)
		k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;

	k = (((k << 1) + (k >> 14)) ^ (vmajor*257)) & 0x3fff;
#if 0
	k = (((k << 1) + (k >> 14)) ^ (vminor*167)) & 0x3fff;
#endif

	return (k);
}

int
buildhints(void)
{
	struct hints_header	hdr;
	struct hints_bucket	*blist;
	struct shlib_list	*shp;
	char			*strtab;
	int			i, n, str_index = 0;
	int			strtab_sz = 0;	/* Total length of strings */
	int			nhints = 0;	/* Total number of hints */
	int			fd;
	char			*tempfile;

	for (shp = shlib_head; shp; shp = shp->next) {
		strtab_sz += 1 + strlen(shp->name);
		strtab_sz += 1 + strlen(shp->path);
		nhints++;
	}

	/* Fill hints file header */
	hdr.hh_magic = HH_MAGIC;
	hdr.hh_version = LD_HINTS_VERSION_2;
	hdr.hh_nbucket = 1 * nhints;
	n = hdr.hh_nbucket * sizeof(struct hints_bucket);
	hdr.hh_hashtab = sizeof(struct hints_header);
	hdr.hh_strtab = hdr.hh_hashtab + n;
	hdr.hh_dirlist = strtab_sz;
	strtab_sz += 1 + strlen(dir_list);
	hdr.hh_strtab_sz = strtab_sz;
	hdr.hh_ehints = hdr.hh_strtab + hdr.hh_strtab_sz;

	if (verbose)
		printf("Totals: entries %d, buckets %ld, string size %d\n",
					nhints, hdr.hh_nbucket, strtab_sz);

	/* Allocate buckets and string table */
	blist = (struct hints_bucket *)xmalloc(n);
	memset(blist, 0, n);
	for (i = 0; i < hdr.hh_nbucket; i++)
		/* Empty all buckets */
		blist[i].hi_next = -1;

	strtab = (char *)xmalloc(strtab_sz);

	/* Enter all */
	for (shp = shlib_head; shp; shp = shp->next) {
		struct hints_bucket	*bp;

		bp = blist +
		  (hinthash(shp->name, shp->major, shp->minor) % hdr.hh_nbucket);

		if (bp->hi_pathx) {
			for (i = 0; i < hdr.hh_nbucket; i++) {
				if (blist[i].hi_pathx == 0)
					break;
			}
			if (i == hdr.hh_nbucket) {
				warnx("Bummer!");
				goto out;
			}
			while (bp->hi_next != -1)
				bp = &blist[bp->hi_next];
			bp->hi_next = i;
			bp = blist + i;
		}

		/* Insert strings in string table */
		bp->hi_namex = str_index;
		strcpy(strtab + str_index, shp->name);
		str_index += 1 + strlen(shp->name);

		bp->hi_pathx = str_index;
		strcpy(strtab + str_index, shp->path);
		str_index += 1 + strlen(shp->path);

		/* Copy versions */
		memcpy(bp->hi_dewey, shp->dewey, sizeof(bp->hi_dewey));
		bp->hi_ndewey = shp->ndewey;
	}

	/* Copy search directories */
	strcpy(strtab + str_index, dir_list);
	str_index += 1 + strlen(dir_list);

	/* Sanity check */
	if (str_index != strtab_sz) {
		errx(1, "str_index(%d) != strtab_sz(%d)", str_index, strtab_sz);
	}

	tempfile = concat(_PATH_LD_HINTS, ".XXXXXX", "");
	if ((fd = mkstemp(tempfile)) == -1) {
		warn("%s", tempfile);
		goto out;
	}

	if (write(fd, &hdr, sizeof(struct hints_header)) !=
	    sizeof(struct hints_header)) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}
	if ((size_t)write(fd, blist, hdr.hh_nbucket * sizeof(struct hints_bucket)) !=
		  hdr.hh_nbucket * sizeof(struct hints_bucket)) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}
	if (write(fd, strtab, strtab_sz) != strtab_sz) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}
	if (fchmod(fd, 0444) == -1) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}
	if (close(fd) != 0) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}

	/* Install it */
	if (unlink(_PATH_LD_HINTS) != 0 && errno != ENOENT) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}

	if (rename(tempfile, _PATH_LD_HINTS) != 0) {
		warn("%s", _PATH_LD_HINTS);
		goto out;
	}

	free(blist);
	free(strtab);
	return 0;
out:
	free(blist);
	free(strtab);
	return -1;
}

static int
readhints(void)
{
	int			fd;
	void			*addr = (void *) -1;
	size_t			msize = 0;
	struct hints_header	*hdr;
	struct hints_bucket	*blist;
	char			*strtab;
	struct shlib_list	*shp;
	int			i;
	struct			stat st;
	int			error = 0;

	if ((fd = open(_PATH_LD_HINTS, O_RDONLY, 0)) == -1) {
		warn("%s", _PATH_LD_HINTS);
		goto cleanup;
	}

	if (fstat(fd, &st) == -1) {
		warn("%s", _PATH_LD_HINTS);
		goto cleanup;
	}

	msize = (size_t)st.st_size;
	if (msize < sizeof(*hdr)) {
		warnx("%s: File too short", _PATH_LD_HINTS);
		goto cleanup;
	}

	addr = mmap(0, msize, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);

	if (addr == (void *)-1) {
		warn("%s", _PATH_LD_HINTS);
		goto cleanup;
	}

	hdr = (struct hints_header *)addr;
	if (HH_BADMAG(*hdr)) {
		warnx("%s: Bad magic: %lo",
		    _PATH_LD_HINTS, hdr->hh_magic);
		goto cleanup;
	}

	if (hdr->hh_version != LD_HINTS_VERSION_2) {
		warnx("Unsupported version: %ld", hdr->hh_version);
		goto cleanup;
	}


	blist = (struct hints_bucket *)((char *)addr + hdr->hh_hashtab);
	strtab = ((char *)addr + hdr->hh_strtab);

	for (i = 0; i < hdr->hh_nbucket; i++) {
		struct hints_bucket	*bp = &blist[i];

		/* Sanity check */
		if (bp->hi_namex >= hdr->hh_strtab_sz) {
			warnx("Bad name index: %#x", bp->hi_namex);
			goto cleanup;
		}
		if (bp->hi_pathx >= hdr->hh_strtab_sz) {
			warnx("Bad path index: %#x", bp->hi_pathx);
			goto cleanup;
		}

		/* Allocate new list element */
		shp = (struct shlib_list *)xmalloc(sizeof *shp);
		shp->name = strdup(strtab + bp->hi_namex);
		shp->path = strdup(strtab + bp->hi_pathx);
		memcpy(shp->dewey, bp->hi_dewey, sizeof(shp->dewey));
		shp->ndewey = bp->hi_ndewey;
		shp->next = NULL;

		*shlib_tail = shp;
		shlib_tail = &shp->next;
	}
	dir_list = strdup(strtab + hdr->hh_dirlist);
	goto done;
cleanup:
	error = 1;
done:
	if (fd != -1)
		close(fd);
	if (addr != (void *)-1)
		munmap(addr, msize);
	return error;

}

static void
listhints(void)
{
	struct shlib_list	*shp;
	int			i;

	printf("%s:\n", _PATH_LD_HINTS);
	printf("\tsearch directories: %s\n", dir_list);

	for (i = 0, shp = shlib_head; shp; i++, shp = shp->next)
		printf("\t%d:-l%s.%d.%d => %s\n",
			i, shp->name, shp->major, shp->minor, shp->path);

	return;
}
