/*	$NetBSD: util.c,v 1.15 2003/05/30 18:14:13 kristerw Exp $	*/
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: util.c,v 1.15 2003/05/30 18:14:13 kristerw Exp $");
#endif /* not lint */

#include <sys/param.h>

#include "EXTERN.h"
#include "common.h"
#include "INTERN.h"
#include "util.h"
#include "backupfile.h"

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Rename a file, copying it if necessary.
 */
int
move_file(char *from, char *to)
{
	char bakname[MAXPATHLEN];
	char *s;
	size_t i;
	int fromfd;

	/* to stdout? */

	if (strEQ(to, "-")) {
#ifdef DEBUGGING
		if (debug & 4)
			say("Moving %s to stdout.\n", from);
#endif
		fromfd = open(from, 0);
		if (fromfd < 0)
			pfatal("internal error, can't reopen %s", from);
		while ((i = read(fromfd, buf, sizeof buf)) > 0)
			if (write(1, buf, i) != 1)
				pfatal("write failed");
		Close(fromfd);
		return 0;
	}

	if (origprae) {
		Strcpy(bakname, origprae);
		Strcat(bakname, to);
	} else {
#ifndef NODIR
		char *backupname = find_backup_file_name(to);
		Strcpy(bakname, backupname);
		free(backupname);
#else /* NODIR */
		Strcpy(bakname, to);
    		Strcat(bakname, simple_backup_suffix);
#endif /* NODIR */
	}

	if (stat(to, &filestat) == 0) {	/* output file exists */
		dev_t to_device = filestat.st_dev;
		ino_t to_inode  = filestat.st_ino;
		char *simplename = bakname;
	
		for (s = bakname; *s; s++) {
			if (*s == '/')
				simplename = s + 1;
		}
		/*
		 * Find a backup name that is not the same file.
		 * Change the first lowercase char into uppercase;
		 * if that isn't sufficient, chop off the first char
		 * and try again.
		 */
		while (stat(bakname, &filestat) == 0 &&
		       to_device == filestat.st_dev &&
		       to_inode == filestat.st_ino) {
			/* Skip initial non-lowercase chars. */
			for (s = simplename;
			     *s && !islower((unsigned char)*s);
			     s++)
				;
			if (*s)
				*s = toupper(*s);
			else
				Strcpy(simplename, simplename + 1);
		}
		while (unlink(bakname) >= 0)
			;	/* while() is for benefit of Eunice */
#ifdef DEBUGGING
		if (debug & 4)
			say("Moving %s to %s.\n", to, bakname);
#endif
		if (link(to, bakname) < 0) {
			/*
			 * Maybe `to' is a symlink into a different file
			 * system. Copying replaces the symlink with a file;
			 * using rename would be better.
			 */
			int tofd;
			int bakfd;

			bakfd = creat(bakname, 0666);
			if (bakfd < 0) {
				say("Can't backup %s, output is in %s: %s\n",
				    to, from, strerror(errno));
				return -1;
			}
			tofd = open(to, 0);
			if (tofd < 0)
				pfatal("internal error, can't open %s", to);
			while ((i = read(tofd, buf, sizeof buf)) > 0)
				if (write(bakfd, buf, i) != i)
					pfatal("write failed");
			Close(tofd);
			Close(bakfd);
		}
		while (unlink(to) >= 0) ;
	}
#ifdef DEBUGGING
	if (debug & 4)
		say("Moving %s to %s.\n", from, to);
#endif
	if (link(from, to) < 0) {		/* different file system? */
		int tofd;

		tofd = creat(to, 0666);
		if (tofd < 0) {
			say("Can't create %s, output is in %s: %s\n",
			    to, from, strerror(errno));
			return -1;
		}
		fromfd = open(from, 0);
		if (fromfd < 0)
			pfatal("internal error, can't reopen %s", from);
		while ((i = read(fromfd, buf, sizeof buf)) > 0)
			if (write(tofd, buf, i) != i)
				pfatal("write failed");
		Close(fromfd);
		Close(tofd);
	}
	Unlink(from);
	return 0;
}

/*
 * Copy a file.
 */
void
copy_file(char *from, char *to)
{
	int tofd;
	int fromfd;
	size_t i;

	tofd = creat(to, 0666);
	if (tofd < 0)
		pfatal("can't create %s", to);
	fromfd = open(from, 0);
	if (fromfd < 0)
		pfatal("internal error, can't reopen %s", from);
	while ((i = read(fromfd, buf, sizeof buf)) > 0)
		if (write(tofd, buf, i) != i)
			pfatal("write to %s failed", to);
	Close(fromfd);
	Close(tofd);
}

/*
 * malloc with result test.
 */
void *
xmalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		fatal("out of memory\n");
	return p;
}

/*
 * realloc with result test.
 */
void *
xrealloc(void *ptr, size_t size)
{
	void *p;

	if ((p = realloc(ptr, size)) == NULL)
		fatal("out of memory\n");
	return p;
}

/*
 * strdup with result test.
 */
char *
xstrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		fatal("out of memory\n");
	return p;
}

/*
 * Vanilla terminal output.
 */
void
say(const char *pat, ...)
{
	va_list ap;
	va_start(ap, pat);
	
	vfprintf(stderr, pat, ap);
	va_end(ap);
	Fflush(stderr);
}

/*
 * Terminal output, pun intended.
 */
void				/* very void */
fatal(const char *pat, ...)
{
	va_list ap;
	va_start(ap, pat);
	
	fprintf(stderr, "patch: **** ");
	vfprintf(stderr, pat, ap);
	va_end(ap);
	my_exit(1);
}

/*
 * Say something from patch, something from the system, then silence...
 */
void				/* very void */
pfatal(const char *pat, ...)
{
	va_list ap;
	int errnum = errno;
	va_start(ap, pat);
	
	fprintf(stderr, "patch: **** ");
	vfprintf(stderr, pat, ap);
	fprintf(stderr, ": %s\n", strerror(errnum));
	va_end(ap);
	my_exit(1);
}

/*
 * Get a response from the user, somehow or other.
 */
void
ask(const char *pat, ...)
{
	int ttyfd;
	int r;
	bool tty2 = isatty(2);
	va_list ap;
	va_start(ap, pat);

	(void)vsprintf(buf, pat, ap);
	va_end(ap);
	Fflush(stderr);
	write(2, buf, strlen(buf));
	if (tty2) {			/* might be redirected to a file */
		r = read(2, buf, sizeof buf);
	} else if (isatty(1)) {		/* this may be new file output */
		Fflush(stdout);
		write(1, buf, strlen(buf));
		r = read(1, buf, sizeof buf);
	} else if ((ttyfd = open("/dev/tty", 2)) >= 0 && isatty(ttyfd)) {
					/* might be deleted or unwritable */
		write(ttyfd, buf, strlen(buf));
		r = read(ttyfd, buf, sizeof buf);
		Close(ttyfd);
	} else if (isatty(0)) {		/* this is probably patch input */
		Fflush(stdin);
		write(0, buf, strlen(buf));
		r = read(0, buf, sizeof buf);
	} else {			/* no terminal at all--default it */
		buf[0] = '\n';
		r = 1;
	}
	if (r <= 0)
		buf[0] = 0;
	else
		buf[r] = '\0';
	if (!tty2)
		say("%s", buf);
}

/*
 * How to handle certain events when not in a critical region.
 */
void
set_signals(int reset)
{
	static void (*hupval)(int);
	static void (*intval)(int);

	if (!reset) {
		hupval = signal(SIGHUP, SIG_IGN);
		if (hupval != SIG_IGN)
			hupval = my_exit;
		intval = signal(SIGINT, SIG_IGN);
		if (intval != SIG_IGN)
			intval = my_exit;
	}
	Signal(SIGHUP, hupval);
	Signal(SIGINT, intval);
}

/*
 * How to handle certain events when in a critical region.
 */
void
ignore_signals()
{
	Signal(SIGHUP, SIG_IGN);
	Signal(SIGINT, SIG_IGN);
}

/*
 * Make sure we'll have the directories to create a file.
 * If `striplast' is TRUE, ignore the last element of `filename'.
 */
void
makedirs(char *filename, bool striplast)
{
	char tmpbuf[MAXPATHLEN];
	char *s = tmpbuf;
	char *dirv[MAXPATHLEN];	/* Point to the NULs between elements.  */
	int i;
	int dirvp = 0;		/* Number of finished entries in dirv. */

	/*
	 * Copy `filename' into `tmpbuf' with a NUL instead of a slash
	 * between the directories.
	 */
	while (*filename) {
		if (*filename == '/') {
			filename++;
			dirv[dirvp++] = s;
			*s++ = '\0';
		} else {
			*s++ = *filename++;
		}
	}
	*s = '\0';
	dirv[dirvp] = s;
	if (striplast)
		dirvp--;
	if (dirvp < 0)
		return;

	strcpy(buf, "mkdir");
	s = buf;
	for (i = 0; i <= dirvp; i++) {
		struct stat sbuf;

		if (stat(tmpbuf, &sbuf) && errno == ENOENT) {
			while (*s)
				s++;
			*s++ = ' ';
			strcpy(s, tmpbuf);
		}
		*dirv[i] = '/';
	}
	if (s != buf)
		system(buf);
}

/*
 * Make filenames more reasonable.
 */
char *
fetchname(char *at, int strip_leading, int assume_exists)
{
	char *fullname;
	char *name;
	char *t;
	char tmpbuf[MAXPATHLEN];
	int sleading = strip_leading;

	if (!at)
		return NULL;
	while (isspace((unsigned char)*at))
		at++;
#ifdef DEBUGGING
	if (debug & 128)
		say("fetchname %s %d %d\n", at, strip_leading, assume_exists);
#endif
	filename_is_dev_null = FALSE;
	if (strnEQ(at, "/dev/null", 9)) {
		/* So files can be created by diffing against /dev/null. */
		filename_is_dev_null = TRUE;
		return NULL;
	}
	name = fullname = t = xstrdup(at);

	/* Strip off up to `sleading' leading slashes and null terminate. */
	for (; *t && !isspace((unsigned char)*t); t++)
		if (*t == '/')
			if (--sleading >= 0)
				name = t + 1;
	*t = '\0';

	/*
	 * If no -p option was given (957 is the default value!),
	 * we were given a relative pathname,
	 * and the leading directories that we just stripped off all exist,
	 * put them back on. 
	 */
	if (strip_leading == 957 && name != fullname && *fullname != '/') {
		name[-1] = '\0';
		if (stat(fullname, &filestat) == 0 &&
		    S_ISDIR(filestat.st_mode)) {
			name[-1] = '/';
			name = fullname;
		}
	}

	name = xstrdup(name);
	free(fullname);

	if (stat(name, &filestat) && !assume_exists) {
		char *filebase = basename(name);
		size_t pathlen = filebase - name;

		/* Put any leading path into `tmpbuf'. */
		strncpy(tmpbuf, name, pathlen);

#define try(f, a1, a2) \
    (Sprintf(tmpbuf + pathlen, f, a1, a2), stat(tmpbuf, &filestat) == 0)
#define try1(f, a1) \
    (Sprintf(tmpbuf + pathlen, f, a1), stat(tmpbuf, &filestat) == 0)
		if (try("RCS/%s%s", filebase, RCSSUFFIX) ||
		    try1("RCS/%s"  , filebase) ||
		    try(    "%s%s", filebase, RCSSUFFIX) ||
		    try("SCCS/%s%s", SCCSPREFIX, filebase) ||
		    try(     "%s%s", SCCSPREFIX, filebase))
			return name;
		free(name);
		name = NULL;
	}

	return name;
}
