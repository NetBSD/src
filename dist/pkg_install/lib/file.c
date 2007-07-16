/*	$NetBSD: file.c,v 1.1.1.1 2007/07/16 13:01:47 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: file.c,v 1.29 1997/10/08 07:47:54 charnier Exp";
#else
__RCSID("$NetBSD: file.c,v 1.1.1.1 2007/07/16 13:01:47 joerg Exp $");
#endif
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
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
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * Miscellaneous file access utilities.
 *
 */

#include "lib.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_GLOB_H
#include <glob.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif


/*
 * Quick check to see if a file (or dir ...) exists
 */
Boolean
fexists(const char *fname)
{
	struct stat dummy;
	if (!lstat(fname, &dummy))
		return TRUE;
	return FALSE;
}

/*
 * Quick check to see if something is a directory
 */
Boolean
isdir(const char *fname)
{
	struct stat sb;

	if (lstat(fname, &sb) != FAIL && S_ISDIR(sb.st_mode))
		return TRUE;
	else
		return FALSE;
}

/*
 * Check if something is a link to a directory
 */
Boolean
islinktodir(const char *fname)
{
	struct stat sb;

	if (lstat(fname, &sb) != FAIL && S_ISLNK(sb.st_mode)) {
		if (stat(fname, &sb) != FAIL && S_ISDIR(sb.st_mode))
			return TRUE;	/* link to dir! */
		else
			return FALSE;	/* link to non-dir */
	} else
		return FALSE;	/* non-link */
}

/*
 * Check to see if file is a dir, and is empty
 */
Boolean
isemptydir(const char *fname)
{
	if (isdir(fname) || islinktodir(fname)) {
		DIR    *dirp;
		struct dirent *dp;

		dirp = opendir(fname);
		if (!dirp)
			return FALSE;	/* no perms, leave it alone */
		for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
			if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {
				closedir(dirp);
				return FALSE;
			}
		}
		(void) closedir(dirp);
		return TRUE;
	}
	return FALSE;
}

/*
 * Check if something is a regular file
 */
Boolean
isfile(const char *fname)
{
	struct stat sb;
	if (stat(fname, &sb) != FAIL && S_ISREG(sb.st_mode))
		return TRUE;
	return FALSE;
}

/*
 * Check to see if file is a file and is empty. If nonexistent or not
 * a file, say "it's empty", otherwise return TRUE if zero sized.
 */
Boolean
isemptyfile(const char *fname)
{
	struct stat sb;
	if (stat(fname, &sb) != FAIL && S_ISREG(sb.st_mode)) {
		if (sb.st_size != 0)
			return FALSE;
	}
	return TRUE;
}

/* This struct defines the leading part of a valid URL name */
typedef struct url_t {
	char   *u_s;		/* the leading part of the URL */
	int     u_len;		/* its length */
}       url_t;

/* A table of valid leading strings for URLs */
static const url_t urls[] = {
	{"ftp://", 6},
	{"http://", 7},
	{NULL}
};

/*
 * Returns length of leading part of any URL from urls table, or -1
 */
int
URLlength(const char *fname)
{
	const url_t *up;
	int     i;

	if (fname != (char *) NULL) {
		for (i = 0; isspace((unsigned char) *fname); i++) {
			fname++;
		}
		for (up = urls; up->u_s; up++) {
			if (strncmp(fname, up->u_s, up->u_len) == 0) {
				return i + up->u_len;    /* ... + sizeof(up->u_s);  - HF */
			}
		}
	}
	return -1;
}

/*
 * Returns the host part of a URL
 */
const char *
fileURLHost(const char *fname, char *where, int max)
{
	const char   *ret;
	int     i;

	assert(where != NULL);
	assert(max > 0);

	if ((i = URLlength(fname)) < 0) {	/* invalid URL? */
		errx(EXIT_FAILURE, "fileURLhost called with a bad URL: `%s'", fname);
	}
	fname += i;
	/* Do we have a place to stick our work? */
	ret = where;
	while (*fname && *fname != '/' && --max)
		*where++ = *fname++;
	*where = '\0';

	return ret;
}

/*
 * Returns the filename part of a URL
 */
const char *
fileURLFilename(const char *fname, char *where, int max)
{
	const char *ret;
	int     i;

	assert(where != NULL);
	assert(max > 0);

	if ((i = URLlength(fname)) < 0) {	/* invalid URL? */
		errx(EXIT_FAILURE, "fileURLFilename called with a bad URL: `%s'", fname);
	}
	fname += i;
	/* Do we have a place to stick our work? */
	ret = where;
	while (*fname && *fname != '/')
		++fname;
	if (*fname == '/') {
		while (*fname && --max)
			*where++ = *fname++;
	}
	*where = '\0';

	return ret;
}

/*
 * Try and fetch a file by URL, returning the directory name for where
 * it's unpacked, if successful. To be handed to leave_playpen() later.
 */
char   *
fileGetURL(const char *spec)
{
	char    host[MAXHOSTNAMELEN], file[MaxPathSize];
	const char *cp;
	char   *rp;
	char    pen[MaxPathSize];
	int     rc;

	rp = NULL;
	if (!IS_URL(spec)) {
		errx(EXIT_FAILURE, "fileGetURL was called with non-URL arg '%s'", spec);
	}

 	/* Some sanity checks on the URL */
	cp = fileURLHost(spec, host, MAXHOSTNAMELEN);
	if (!*cp) {
		warnx("URL `%s' has bad host part!", spec);
		return NULL;
	}
	cp = fileURLFilename(spec, file, MaxPathSize);
	if (!*cp) {
		warnx("URL `%s' has bad filename part!", spec);
		return NULL;
	}

	if (Verbose)
		printf("Trying to fetch %s.\n", spec);
	
	pen[0] = '\0';
	rp = make_playpen(pen, sizeof(pen), 0);
	if (rp == NULL) {
		printf("Error: Unable to construct a new playpen for FTP!\n");
		return NULL;
	}

	rp = strdup(pen);
	rc = unpackURL(spec, pen);
	if (rc < 0) {
		leave_playpen(rp); /* Don't leave dir hang around! */
		
		printf("Error on unpackURL('%s', '%s')\n", spec, pen);
		return NULL;
	}
	return rp;
}

static char *
resolvepattern1(const char *name)
{
	static char tmp[MaxPathSize];
	char *cp;

	if (IS_URL(name)) {
		/* some package depends on a wildcard pkg */
		int rc;

		rc = expandURL(tmp, name);
		if (rc < 0) {
			return NULL;
		}
		if (Verbose)
			printf("'%s' expanded to '%s'\n", name, tmp);
		return tmp;    /* return expanded URL w/ corrent pkg */
	}
	else if (ispkgpattern(name)) {
		cp = findbestmatchingname(
			dirname_of(name), basename_of(name));
		if (cp) {
			snprintf(tmp, sizeof(tmp), "%s/%s", dirname_of(name), cp);
			free(cp);
			return tmp;
		}
	} else {
		if (isfile(name)) {
			strlcpy(tmp, name, sizeof(tmp));
			return tmp;
		}
	}

	return NULL;
}

static char *
resolvepattern(const char *name)
{
	char tmp[MaxPathSize];
	char *cp;
	const char *suf;

	cp = resolvepattern1(name);
	if (cp != NULL)
		return cp;

	if (ispkgpattern(name))
		return NULL;

	suf = suffix_of(name);
	if (!strcmp(suf, "tbz") || !strcmp(suf, "tgz"))
		return NULL;

	/* add suffix and try */
	snprintf(tmp, sizeof(tmp), "%s.tbz", name);
	cp = resolvepattern1(tmp);
	if (cp != NULL)
		return cp;
	snprintf(tmp, sizeof(tmp), "%s.tgz", name);
	cp = resolvepattern1(tmp);
	if (cp != NULL)
		return cp;

	/* add version number wildcard and try */
	snprintf(tmp, sizeof(tmp), "%s-[0-9]*.t[bg]z", name);
	return resolvepattern1(tmp);
}

/*
 *  Look for filename/pattern "fname" in
 * Returns a full path/URL where the pkg can be found
 */
char   *
fileFindByPath(const char *fname)
{
	char    tmp[MaxPathSize];
	struct path *path;

	/*
	 * 1. if fname is an absolute pathname or a URL,
	 *    just use it.
	 */
	if (IS_FULLPATH(fname) || IS_URL(fname))
		return resolvepattern(fname);

	/*
	 * 2. otherwise, use PKG_PATH.
	 */
	TAILQ_FOREACH(path, &PkgPath, pl_entry) {
		char *cp;
		const char *cp2 = path->pl_path;

		if (Verbose)
			printf("trying PKG_PATH %s\n", cp2);

		if (IS_FULLPATH(cp2) || IS_URL(cp2)) {
			snprintf(tmp, sizeof(tmp), "%s/%s", cp2, fname);
		}
		else {
			char cwdtmp[MaxPathSize];
			if (getcwd(cwdtmp, sizeof(cwdtmp)) == NULL)
				errx(EXIT_FAILURE, "getcwd");
			snprintf(tmp, sizeof(tmp), "%s/%s/%s", cwdtmp, cp2, fname);
		}
		cp = resolvepattern(tmp);
		if (cp)
			return cp;
	}

#if 0
	/*
	 * 3. finally, search current directory.
	 */
	snprintf(tmp, sizeof(tmp), "./%s", fname);
	return resolvepattern(tmp);
#else
	return NULL;
#endif
}

/*
 *  Expect "fname" to point at a file, and read it into
 *  the buffer returned.
 */
char   *
fileGetContents(char *fname)
{
	char   *contents;
	struct stat sb;
	int     fd;

	if (stat(fname, &sb) == FAIL) {
		cleanup(0);
		errx(2, "can't stat '%s'", fname);
	}

	contents = (char *) malloc((size_t) (sb.st_size) + 1);
	fd = open(fname, O_RDONLY, 0);
	if (fd == FAIL) {
		cleanup(0);
		errx(2, "unable to open '%s' for reading", fname);
	}
	if (read(fd, contents, (size_t) sb.st_size) != (size_t) sb.st_size) {
		cleanup(0);
		errx(2, "short read on '%s' - did not get %lld bytes",
		    fname, (long long) sb.st_size);
	}
	close(fd);
	contents[(size_t) sb.st_size] = '\0';
	return contents;
}

/*
 * Takes a filename and package name, returning (in "try") the canonical
 * "preserve" name for it.
 */
Boolean
make_preserve_name(char *try, size_t max, char *name, char *file)
{
	int     len, i;

	if ((len = strlen(file)) == 0)
		return FALSE;
	else
		i = len - 1;
	strncpy(try, file, max);
	if (try[i] == '/')	/* Catch trailing slash early and save checking in the loop */
		--i;
	for (; i; i--) {
		if (try[i] == '/') {
			try[i + 1] = '.';
			strncpy(&try[i + 2], &file[i + 1], max - i - 2);
			break;
		}
	}
	if (!i) {
		try[0] = '.';
		strncpy(try + 1, file, max - 1);
	}
	/* I should probably be called rude names for these inline assignments */
	strncat(try, ".", max -= strlen(try));
	strncat(try, name, max -= strlen(name));
	strncat(try, ".", max--);
	strncat(try, "backup", max -= 6);
	return TRUE;
}

/*
 * Write the contents of "str" to a file
 */
void
write_file(char *name, char *str)
{
	size_t  len;
	FILE   *fp;

	if ((fp = fopen(name, "w")) == (FILE *) NULL) {
		cleanup(0);
		errx(2, "cannot fopen '%s' for writing", name);
	}
	len = strlen(str);
	if (fwrite(str, 1, len, fp) != len) {
		cleanup(0);
		errx(2, "short fwrite on '%s', tried to write %ld bytes",
		    name, (long) len);
	}
	if (fclose(fp)) {
		cleanup(0);
		errx(2, "failure to fclose '%s'", name);
	}
}

void
copy_file(char *dir, char *fname, char *to)
{
	char    fpath[MaxPathSize];

	(void) snprintf(fpath, sizeof(fpath), "%s%s%s",
			(fname[0] != '/') ? dir : "",
			(fname[0] != '/') ? "/" : "",
			fname);
	if (fexec("cp", "-r", fpath, to, NULL)) {
		cleanup(0);
		errx(2, "could not perform 'cp -r %s %s'", fpath, to);
	}
}

void
move_file(char *dir, char *fname, char *to)
{
	char    fpath[MaxPathSize];

	(void) snprintf(fpath, sizeof(fpath), "%s%s%s",
			(fname[0] != '/') ? dir : "",
			(fname[0] != '/') ? "/" : "",
			fname);
	if (fexec("mv", fpath, to, NULL)) {
		cleanup(0);
		errx(2, "could not perform 'mv %s %s'", fpath, to);
	}
}

void
move_files(const char *dir, const char *pattern, const char *to)
{
	char	fpath[MaxPathSize];
	glob_t	globbed;
	size_t	i;

	(void) snprintf(fpath, sizeof(fpath), "%s/%s", dir, pattern);
	if ((i=glob(fpath, GLOB_NOSORT, NULL, &globbed)) != 0) {
		switch(i) {
		case GLOB_NOMATCH:
			warn("no files matching ``%s'' found", fpath);
			break;
		case GLOB_ABORTED:
			warn("globbing aborted");
			break;
		case GLOB_NOSPACE:
			warn("out-of-memory during globbing");
			break;
		default:
			warn("unknown error during globbing");
			break;
		}
		return;
	}

	/* Moving globbed files -- we just use mv(1) to do the job */
	for (i=0; i<globbed.gl_pathc; i++)
		if (fexec("mv", globbed.gl_pathv[i], to, NULL)) {
			cleanup(0);
			errx(2, "could not perform 'mv %s %s'", globbed.gl_pathv[i], to);
		}

	return;
}

void
remove_files(const char *path, const char *pattern)
{
	char	fpath[MaxPathSize];
	glob_t	globbed;
	int	i;

	(void) snprintf(fpath, sizeof(fpath), "%s/%s", path, pattern);
	if ((i=glob(fpath, GLOB_NOSORT, NULL, &globbed)) != 0) {
		switch(i) {
		case GLOB_NOMATCH:
			warn("no files matching ``%s'' found", fpath);
			break;
		case GLOB_ABORTED:
			warn("globbing aborted");
			break;
		case GLOB_NOSPACE:
			warn("out-of-memory during globbing");
			break;
		default:
			warn("unknown error during globbing");
			break;
		}
		return;
	}

	/* deleting globbed files */
	for (i=0; i<globbed.gl_pathc; i++)
		if (unlink(globbed.gl_pathv[i]) < 0)
			warn("can't delete ``%s''", globbed.gl_pathv[i]);

	return;
}

/*
 * Unpack a tar file
 */
int
unpack(const char *pkg, const lfile_head_t *filesp)
{
	const char *decompress_cmd = NULL;
	const char *suf;
	int count = 0;
	lfile_t	*lfp;
	char **up_argv;
	int up_argc = 7;
	int i = 0;
	int result;

	if (filesp != NULL)
		TAILQ_FOREACH(lfp, filesp, lf_link)
			count++;
	up_argc += count;
	up_argv = malloc((count + up_argc + 1) * sizeof(char *));
	if (!IS_STDIN(pkg)) {
		suf = suffix_of(pkg);
		if (!strcmp(suf, "tbz") || !strcmp(suf, "bz2"))
			decompress_cmd = BZIP2_CMD;
		else if (!strcmp(suf, "tgz") || !strcmp(suf, "gz"))
			decompress_cmd = GZIP_CMD;
		else if (!strcmp(suf, "tar"))
			; /* do nothing */
		else
			errx(EXIT_FAILURE, "don't know how to decompress %s, sorry", pkg);
	} else
		decompress_cmd = GZIP_CMD;

	up_argv[i] = (char *)strrchr(TAR_CMD, '/');
	if (up_argv[i] == NULL)
		up_argv[i] = TAR_CMD;
	else
		up_argv[i]++;  /* skip / character */
	if (count > 0)
		up_argv[++i] = "--fast-read";
	if (decompress_cmd != NULL) {
		up_argv[++i] = "--use-compress-program";
		up_argv[++i] = (char *)decompress_cmd;
	}
	up_argv[++i] = "-xpf";
	up_argv[++i] = (char *)pkg;
	if (count > 0)
		TAILQ_FOREACH(lfp, filesp, lf_link)
			up_argv[++i] = lfp->lf_name;
	up_argv[++i] = NULL;

	if (Verbose) {
		printf("running: %s", TAR_CMD);
		for (i = 1; up_argv[i] != NULL; i++)
			printf(" %s", up_argv[i]);
		printf("\n");
	}

	result = pfcexec(NULL, TAR_CMD, (const char **)up_argv);
	free(up_argv);
	if (result != 0) {
		warnx("extract of %s failed", pkg);
		return 1;
	}

	return 0;
}

/*
 * Using fmt, replace all instances of:
 *
 * %F	With the parameter "name"
 * %D	With the parameter "dir"
 * %B	Return the directory part ("base") of %D/%F
 * %f	Return the filename part of %D/%F
 *
 * Check that no overflows can occur.
 */
void
format_cmd(char *buf, size_t size, char *fmt, char *dir, char *name)
{
	char    scratch[MaxPathSize * 2];
	char   *bufp;
	char   *cp;

	for (bufp = buf; (int) (bufp - buf) < size && *fmt;) {
		if (*fmt == '%') {
			if (*++fmt != 'D' && name == NULL) {
				cleanup(0);
				errx(2, "no last file available for '%s' command", buf);
			}
			switch (*fmt) {
			case 'F':
				strlcpy(bufp, name, size - (int) (bufp - buf));
				bufp += strlen(bufp);
				break;

			case 'D':
				strlcpy(bufp, dir, size - (int) (bufp - buf));
				bufp += strlen(bufp);
				break;

			case 'B':
				(void) snprintf(scratch, sizeof(scratch), "%s/%s", dir, name);
				if ((cp = strrchr(scratch, '/')) == (char *) NULL) {
					cp = scratch;
				}
				*cp = '\0';
				strlcpy(bufp, scratch, size - (int) (bufp - buf));
				bufp += strlen(bufp);
				break;

			case 'f':
				(void) snprintf(scratch, sizeof(scratch), "%s/%s", dir, name);
				if ((cp = strrchr(scratch, '/')) == (char *) NULL) {
					cp = scratch;
				} else {
					cp++;
				}
				strlcpy(bufp, cp, size - (int) (bufp - buf));
				bufp += strlen(bufp);
				break;

			default:
				*bufp++ = '%';
				*bufp++ = *fmt;
				break;
			}
			++fmt;
		} else {
			*bufp++ = *fmt++;
		}
	}
	*bufp = '\0';
}
