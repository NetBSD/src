/*	$NetBSD: file.c,v 1.32.4.1 1999/12/27 18:38:02 wrstuden Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: file.c,v 1.29 1997/10/08 07:47:54 charnier Exp";
#else
__RCSID("$NetBSD: file.c,v 1.32.4.1 1999/12/27 18:38:02 wrstuden Exp $");
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

#include <sys/wait.h>

#include <assert.h>
#include <err.h>
#include <netdb.h>
#include <pwd.h>
#include <time.h>
#include <fcntl.h>

/*
 * This is as ftpGetURL from FreeBSD's ftpio.c, except that it uses
 * NetBSD's ftp command to do all FTP, which will DTRT for proxies,
 * etc.
 */
static FILE *
ftpGetURL(char *url, int *retcode)
{
	FILE   *ftp;
	pid_t   pid_ftp;
	int     p[2];

	*retcode = 0;

	if (pipe(p) < 0) {
		*retcode = 1;
		return NULL;
	}

	pid_ftp = fork();
	if (pid_ftp < 0) {
		*retcode = 1;
		return NULL;
	}
	if (pid_ftp == 0) {
		/* child */
		dup2(p[1], 1);
		close(p[1]);

		execl("/usr/bin/ftp", "ftp", "-V", "-o", "-", url, NULL);
		exit(1);
	} else {
		/* parent */
		ftp = fdopen(p[0], "r");

		close(p[1]);

		if (ftp == (FILE *) NULL) {
			*retcode = 1;
			return NULL;
		}
	}
	return ftp;
}

/*
 * Quick check to see if a file (or dir ...) exists
 */
Boolean
fexists(char *fname)
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
isdir(char *fname)
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
islinktodir(char *fname)
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
isemptydir(char *fname)
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
isfile(char *fname)
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
isemptyfile(char *fname)
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
static url_t urls[] = {
	{"ftp://", 6},
	{"http://", 7},
	{NULL}
};

/*
 * Returns length of leading part of any URL from urls table, or -1
 */
int
URLlength(char *fname)
{
	url_t  *up;
	int     i;

	if (fname != (char *) NULL) {
		for (i = 0; isspace((unsigned char) *fname); i++) {
			fname++;
		}
		for (up = urls; up->u_s; up++) {
			if (strncmp(fname, up->u_s, up->u_len) == 0) {
				return i + up->u_len;
			}
		}
	}
	return -1;
}

/*
 * Returns the host part of a URL
 */
char   *
fileURLHost(char *fname, char *where, int max)
{
	char   *ret;
	int     i;

	if ((i = URLlength(fname)) < 0) {	/* invalid URL? */
		errx(1, "fileURLhost called with a bad URL: `%s'", fname);
	}
	fname += i;
	/* Do we have a place to stick our work? */
	if ((ret = where) != NULL) {
		while (*fname && *fname != '/' && max--)
			*where++ = *fname++;
		*where = '\0';
		return ret;
	}
	/* If not, they must really want us to stomp the original string */
	ret = fname;
	while (*fname && *fname != '/')
		++fname;
	*fname = '\0';
	return ret;
}

/*
 * Returns the filename part of a URL
 */
char   *
fileURLFilename(char *fname, char *where, int max)
{
	char   *ret;
	int     i;

	if ((i = URLlength(fname)) < 0) {	/* invalid URL? */
		errx(1, "fileURLhost called with a bad URL: `%s'", fname);
	}
	fname += i;
	/* Do we have a place to stick our work? */
	if ((ret = where) != NULL) {
		while (*fname && *fname != '/')
			++fname;
		if (*fname == '/') {
			while (*fname && max--)
				*where++ = *fname++;
		}
		*where = '\0';
		return ret;
	}
	/* If not, they must really want us to stomp the original string */
	while (*fname && *fname != '/')
		++fname;
	return fname;
}

/*
 * Try and fetch a file by URL, returning the directory name for where
 * it's unpacked, if successful.
 */
char   *
fileGetURL(char *base, char *spec)
{
	char    host[MAXHOSTNAMELEN], file[FILENAME_MAX];
	char   *cp, *rp;
	char    fname[FILENAME_MAX];
	char    pen[FILENAME_MAX];
	FILE   *ftp;
	pid_t   tpid;
	int     i, status;
	char   *hint;

	rp = NULL;
	/* Special tip that sysinstall left for us */
	hint = getenv("PKG_ADD_BASE");
	if (!IS_URL(spec)) {
		if (!base && !hint)
			return NULL;
		/* We've been given an existing URL (that's known-good) and
		 * now we need to construct a composite one out of that and
		 * the basename we were handed as a dependency. */
		if (base) {
			strcpy(fname, base);
			/* Advance back two slashes to get to the root of the package hierarchy */
			cp = strrchr(fname, '/');
			if (cp) {
				*cp = '\0';	/* chop name */
				cp = strrchr(fname, '/');
			}
			if (cp) {
				*(cp + 1) = '\0';
				strcat(cp, "All/");
				strcat(cp, spec);
				strcat(cp, ".tgz");
			} else
				return NULL;
		} else {
			/* Otherwise, we've been given an environment variable hinting at the right location from sysinstall */
			strcpy(fname, hint);
			strcat(fname, spec);
			strcat(fname, ".tgz");
		}
	} else
		strcpy(fname, spec);
	cp = fileURLHost(fname, host, MAXHOSTNAMELEN);
	if (!*cp) {
		warnx("URL `%s' has bad host part!", fname);
		return NULL;
	}

	cp = fileURLFilename(fname, file, FILENAME_MAX);
	if (!*cp) {
		warnx("URL `%s' has bad filename part!", fname);
		return NULL;
	}

	if (Verbose)
		printf("Trying to fetch %s.\n", fname);
	ftp = ftpGetURL(fname, &status);
	if (ftp) {
		pen[0] = '\0';
		if ((rp = make_playpen(pen, sizeof(pen), 0)) != NULL) {
			rp = strdup(pen);	/* be safe for nested calls */
			if (Verbose)
				printf("Extracting from FTP connection into %s\n", pen);
			tpid = fork();
			if (!tpid) {
				dup2(fileno(ftp), 0);
				i = execl(TAR_FULLPATHNAME, TAR_CMD, Verbose ? "-xzvf" : "-xzf", "-", 0);
				err(i, TAR_FULLPATHNAME " failed");
			} else {
				int     pstat;

				fclose(ftp);
				tpid = waitpid(tpid, &pstat, 0);
				if (Verbose)
					printf("%s command returns %d status\n", TAR_CMD, WEXITSTATUS(pstat));
			}
		} else
			printf("Error: Unable to construct a new playpen for FTP!\n");
		fclose(ftp);
	} else
		printf("Error: FTP Unable to get %s: %s\n",
		    fname,
		    status ? "Error while performing FTP" :
		    hstrerror(h_errno));
	return rp;
}

/*
 *  Look for filename/pattern "fname" in
 *   - current dir, and if not found there, look
 *   - $base/../All
 *   - all dirs in $PKG_PATH
 */
char   *
fileFindByPath(char *base, char *fname)
{
	static char tmp[FILENAME_MAX];
	char   *cp;

	if (ispkgpattern(fname)) {
		if ((cp = findbestmatchingname(".", fname)) != NULL) {
			strcpy(tmp, cp);
			return tmp;
		}
	} else {
		if (fexists(fname) && isfile(fname)) {
			strcpy(tmp, fname);
			return tmp;
		}
	}

	if (base) {
		strcpy(tmp, base);

		cp = strrchr(tmp, '/');
		if (cp) {
			*cp = '\0';	/* chop name */
			cp = strrchr(tmp, '/');
		}
		if (cp) {
			*(cp + 1) = '\0';
			strcat(cp, "All/");
			strcat(cp, fname);
			strcat(cp, ".tgz");
			if (ispkgpattern(tmp)) {
				cp = findbestmatchingname(dirname_of(tmp), basename_of(tmp));
				if (cp) {
					char   *s;
					s = strrchr(tmp, '/');
					assert(s != NULL);
					strcpy(s + 1, cp);
					return tmp;
				}
			} else {
				if (fexists(tmp)) {
					return tmp;
				}
			}
		}
	}

	cp = getenv("PKG_PATH");
	while (cp) {
		char   *cp2 = strsep(&cp, ":");

		(void) snprintf(tmp, sizeof(tmp), "%s/%s.tgz", cp2 ? cp2 : cp, fname);
		if (ispkgpattern(tmp)) {
			char   *s;
			s = findbestmatchingname(dirname_of(tmp), basename_of(tmp));
			if (s) {
				char   *t;
				t = strrchr(tmp, '/');
				strcpy(t + 1, s);
				return tmp;
			}
		} else {
			if (fexists(tmp) && isfile(tmp)) {
				return tmp;
			}
		}
	}

	return NULL;
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
		errx(2, "short read on '%s' - did not get %qd bytes",
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
	char    cmd[FILENAME_MAX];

	if (fname[0] == '/')
		(void) snprintf(cmd, sizeof(cmd), "cp -r %s %s", fname, to);
	else
		(void) snprintf(cmd, sizeof(cmd), "cp -r %s/%s %s", dir, fname, to);
	if (vsystem(cmd)) {
		cleanup(0);
		errx(2, "could not perform '%s'", cmd);
	}
}

void
move_file(char *dir, char *fname, char *to)
{
	char    cmd[FILENAME_MAX];

	if (fname[0] == '/')
		(void) snprintf(cmd, sizeof(cmd), "mv %s %s", fname, to);
	else
		(void) snprintf(cmd, sizeof(cmd), "mv %s/%s %s", dir, fname, to);
	if (vsystem(cmd)) {
		cleanup(0);
		errx(2, "could not perform '%s'", cmd);
	}
}

/*
 * Unpack a tar file
 */
int
unpack(char *pkg, char *flist)
{
	char    args[10], suff[80], *cp;

	args[0] = '\0';
	/*
         * Figure out by a crude heuristic whether this or not this is probably
         * compressed.
         */
	if (strcmp(pkg, "-")) {
		cp = strrchr(pkg, '.');
		if (cp) {
			strcpy(suff, cp + 1);
			if (strchr(suff, 'z') || strchr(suff, 'Z'))
				strcpy(args, "-z");
		}
	} else
		strcpy(args, "z");
	strcat(args, "xpf");
	if (vsystem("%s %s %s %s", TAR_CMD, args, pkg, flist ? flist : "")) {
		warnx("%s extract of %s failed!", TAR_CMD, pkg);
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
	char    scratch[FILENAME_MAX * 2];
	char   *bufp;
	char   *cp;

	for (bufp = buf; (int) (bufp - buf) < size && *fmt;) {
		if (*fmt == '%') {
			switch (*++fmt) {
			case 'F':
				strnncpy(bufp, size - (int) (bufp - buf), name, strlen(name));
				bufp += strlen(bufp);
				break;

			case 'D':
				strnncpy(bufp, size - (int) (bufp - buf), dir, strlen(dir));
				bufp += strlen(bufp);
				break;

			case 'B':
				(void) snprintf(scratch, sizeof(scratch), "%s/%s", dir, name);
				if ((cp = strrchr(scratch, '/')) == (char *) NULL) {
					cp = scratch;
				}
				strnncpy(bufp, size - (int) (bufp - buf), scratch, (size_t) (cp - scratch));
				bufp += strlen(bufp);
				break;

			case 'f':
				(void) snprintf(scratch, sizeof(scratch), "%s/%s", dir, name);
				if ((cp = strrchr(scratch, '/')) == (char *) NULL) {
					cp = scratch;
				} else {
					cp++;
				}
				strnncpy(bufp, size - (int) (bufp - buf), cp, strlen(cp));
				bufp += strlen(bufp);
				break;

			default:
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
