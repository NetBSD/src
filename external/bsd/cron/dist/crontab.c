/*	$NetBSD: crontab.c,v 1.7.10.1 2014/09/08 19:15:03 msaitoh Exp $	*/

/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1997,2000 by Internet Software Consortium, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/cdefs.h>
#if !defined(lint) && !defined(LINT)
#if 0
static char rcsid[] = "Id: crontab.c,v 1.12 2004/01/23 18:56:42 vixie Exp";
#else
__RCSID("$NetBSD: crontab.c,v 1.7.10.1 2014/09/08 19:15:03 msaitoh Exp $");
#endif
#endif

/* crontab - install and manage per-user crontab files
 * vix 02may87 [RCS has the rest of the log]
 * vix 26jan87 [original]
 */

#define	MAIN_PROGRAM

#include "cron.h"

#define NHEADER_LINES 3

enum opt_t	{ opt_unknown, opt_list, opt_delete, opt_edit, opt_replace };

#if DEBUGGING
static const char	*Options[] = {
    "???", "list", "delete", "edit", "replace" };
static const char	*getoptargs = "u:lerx:";
#else
static const char	*getoptargs = "u:ler";
#endif

static	PID_T		Pid;
static	char		User[MAX_UNAME], RealUser[MAX_UNAME];
static	char		Filename[MAX_FNAME], TempFilename[MAX_FNAME];
static	FILE		*NewCrontab;
static	int		CheckErrorCount;
static	enum opt_t	Option;
static	struct passwd	*pw;
static	void		list_cmd(void),
			delete_cmd(void),
			edit_cmd(void),
			poke_daemon(void),
			check_error(const char *),
			parse_args(int c, char *v[]);
static	int		replace_cmd(void);
static  int		allowed(const char *, const char *, const char *);
static  int		in_file(const char *, FILE *, int);
static  int 		relinguish_priv(void);
static  int 		regain_priv(void);

static void
usage(const char *msg) {
	(void)fprintf(stderr, "%s: usage error: %s\n", getprogname(), msg);
	(void)fprintf(stderr, "usage:\t%s [-u user] file\n", getprogname());
	(void)fprintf(stderr, "\t%s [-u user] [ -e | -l | -r ]\n", getprogname());
	(void)fprintf(stderr, "\t\t(default operation is replace, per 1003.2)\n");
	(void)fprintf(stderr, "\t-e\t(edit user's crontab)\n");
	(void)fprintf(stderr, "\t-l\t(list user's crontab)\n");
	(void)fprintf(stderr, "\t-r\t(delete user's crontab)\n");
	exit(ERROR_EXIT);
}

static uid_t euid, ruid;
static gid_t egid, rgid;

int
main(int argc, char *argv[]) {
	int exitstatus;

	setprogname(argv[0]);
	Pid = getpid();
	(void)setlocale(LC_ALL, "");

	euid = geteuid();
	egid = getegid();
	ruid = getuid();
	rgid = getgid();

	if (euid == ruid && ruid != 0)
		errx(ERROR_EXIT, "Not installed setuid root");

	(void)setvbuf(stderr, NULL, _IOLBF, 0);
	parse_args(argc, argv);		/* sets many globals, opens a file */
	set_cron_cwd();
	if (!allowed(RealUser, CRON_ALLOW, CRON_DENY)) {
		(void)fprintf(stderr,
			"You `%s' are not allowed to use this program `%s'\n",
			User, getprogname());
		(void)fprintf(stderr, "See crontab(1) for more information\n");
		log_it(RealUser, Pid, "AUTH", "crontab command not allowed");
		exit(ERROR_EXIT);
	}
	exitstatus = OK_EXIT;
	switch (Option) {
	case opt_unknown:
		usage("unrecognized option");
		exitstatus = ERROR_EXIT;
		break;
	case opt_list:
		list_cmd();
		break;
	case opt_delete:
		delete_cmd();
		break;
	case opt_edit:
		edit_cmd();
		break;
	case opt_replace:
		if (replace_cmd() < 0)
			exitstatus = ERROR_EXIT;
		break;
	default:
		abort();
	}
	exit(exitstatus);
	/*NOTREACHED*/
}

static void
get_time(const struct stat *st, struct timespec *ts)
{
	ts[0].tv_sec = st->st_atime;
	ts[0].tv_nsec = st->st_atimensec;
	ts[1].tv_sec = st->st_mtime;
	ts[1].tv_nsec = st->st_mtimensec;
}

static int
change_time(const char *name, const struct timespec *ts)
{
#if defined(HAVE_UTIMENSAT)
	return utimensat(AT_FDCWD, name, ts, 0);
#elif defined(HAVE_UTIMES)
	struct timeval tv[2];
	TIMESPEC_TO_TIMEVAL(&tv[0], &ts[0]);
	TIMESPEC_TO_TIMEVAL(&tv[1], &ts[1]);
	return utimes(name, tv);
#else
	struct utimbuf ut;
	ut.actime = ts[0].tv_sec;
	ut.modtime = ts[1].tv_sec;
	return utime(name, &ut);
#endif
}

static int
compare_time(const struct stat *st, const struct timespec *ts2)
{
	struct timespec ts1[2];
	get_time(st, ts1);
	
	return ts1[1].tv_sec == ts2[1].tv_sec
#if defined(HAVE_UTIMENSAT)
	    && ts1[1].tv_nsec == ts2[1].tv_nsec
#elif defined(HAVE_UTIMES)
	    && ts1[1].tv_nsec / 1000 == ts2[1].tv_nsec / 1000
#endif
	;
}

static void
parse_args(int argc, char *argv[]) {
	int argch;

	if (!(pw = getpwuid(getuid()))) {
		errx(ERROR_EXIT,
		    "your UID isn't in the passwd file. bailingo out");
	}
	if (strlen(pw->pw_name) >= sizeof User) {
		errx(ERROR_EXIT, "username too long");
	}
	(void)strlcpy(User, pw->pw_name, sizeof(User));
	(void)strlcpy(RealUser, User, sizeof(RealUser));
	Filename[0] = '\0';
	Option = opt_unknown;
	while (-1 != (argch = getopt(argc, argv, getoptargs))) {
		switch (argch) {
#if DEBUGGING
		case 'x':
			if (!set_debug_flags(optarg))
				usage("bad debug option");
			break;
#endif
		case 'u':
			if (MY_UID(pw) != ROOT_UID) {
				errx(ERROR_EXIT,
				    "must be privileged to use -u");
			}
			if (!(pw = getpwnam(optarg))) {
				errx(ERROR_EXIT, "user `%s' unknown", optarg);
			}
			if (strlen(optarg) >= sizeof User)
				usage("username too long");
			(void) strlcpy(User, optarg, sizeof(User));
			break;
		case 'l':
			if (Option != opt_unknown)
				usage("only one operation permitted");
			Option = opt_list;
			break;
		case 'r':
			if (Option != opt_unknown)
				usage("only one operation permitted");
			Option = opt_delete;
			break;
		case 'e':
			if (Option != opt_unknown)
				usage("only one operation permitted");
			Option = opt_edit;
			break;
		default:
			usage("unrecognized option");
		}
	}

	endpwent();

	if (Option != opt_unknown) {
		if (argv[optind] != NULL)
			usage("no arguments permitted after this option");
	} else {
		if (argv[optind] != NULL) {
			Option = opt_replace;
			if (strlen(argv[optind]) >= sizeof Filename)
				usage("filename too long");
			(void)strlcpy(Filename, argv[optind], sizeof(Filename));
		} else
			usage("file name must be specified for replace");
	}

	if (Option == opt_replace) {
		/* we have to open the file here because we're going to
		 * chdir(2) into /var/cron before we get around to
		 * reading the file.
		 */
		if (!strcmp(Filename, "-"))
			NewCrontab = stdin;
		else {
			/* relinquish the setuid status of the binary during
			 * the open, lest nonroot users read files they should
			 * not be able to read.  we can't use access() here
			 * since there's a race condition.  thanks go out to
			 * Arnt Gulbrandsen <agulbra@pvv.unit.no> for spotting
			 * the race.
			 */

			if (relinguish_priv() < OK) {
				err(ERROR_EXIT, "swapping uids");
			}
			if (!(NewCrontab = fopen(Filename, "r"))) {
				err(ERROR_EXIT, "cannot open `%s'", Filename);
			}
			if (regain_priv() < OK) {
				err(ERROR_EXIT, "swapping uids back");
			}
		}
	}

	Debug(DMISC, ("user=%s, file=%s, option=%s\n",
		      User, Filename, Options[(int)Option]));
}

static void
skip_header(int *pch, FILE *f)
{
	int ch;
	int x;
	
	/* ignore the top few comments since we probably put them there.
	 */
	for (x = 0;  x < NHEADER_LINES;  x++) {
		ch = get_char(f);
		if (EOF == ch)
			break;
		if ('#' != ch)
			break;
		while (EOF != (ch = get_char(f)))
			if (ch == '\n')
				break;
		if (EOF == ch)
			break;
	}
	if (ch == '\n')
		ch = get_char(f);

	*pch = ch;
}

static void
list_cmd(void) {
	char n[MAX_FNAME];
	FILE *f;
	int ch;

	log_it(RealUser, Pid, "LIST", User);
	if (!glue_strings(n, sizeof n, SPOOL_DIR, User, '/')) {
		errx(ERROR_EXIT, "path too long");
	}
	if (!(f = fopen(n, "r"))) {
		if (errno == ENOENT)
			errx(ERROR_EXIT, "no crontab for `%s'", User);
		else
			err(ERROR_EXIT, "Cannot open `%s'", n);
	}

	/* file is open. copy to stdout, close.
	 */
	Set_LineNum(1);
	skip_header(&ch, f);
	for (; EOF != ch;  ch = get_char(f))
		(void)putchar(ch);
	(void)fclose(f);
}

static void
delete_cmd(void) {
	char n[MAX_FNAME];

	log_it(RealUser, Pid, "DELETE", User);
	if (!glue_strings(n, sizeof n, SPOOL_DIR, User, '/')) {
		errx(ERROR_EXIT, "path too long");
	}
	if (unlink(n) != 0) {
		if (errno == ENOENT)
			errx(ERROR_EXIT, "no crontab for `%s'", User);
		else
			err(ERROR_EXIT, "cannot unlink `%s'", n);
	}
	poke_daemon();
}

static void
check_error(const char *msg) {
	CheckErrorCount++;
	(void)fprintf(stderr, "\"%s\":%d: %s\n", Filename, LineNumber-1, msg);
}

static void
edit_cmd(void) {
	char n[MAX_FNAME], q[MAX_TEMPSTR];
	const char *editor;
	FILE *f;
	int ch, t, x;
	sig_t oint, oabrt, oquit, ohup;
	struct stat statbuf;
	WAIT_T waiter;
	PID_T pid, xpid;
	struct timespec ts[2];

	log_it(RealUser, Pid, "BEGIN EDIT", User);
	if (!glue_strings(n, sizeof n, SPOOL_DIR, User, '/')) {
		errx(ERROR_EXIT, "path too long");
	}
	if (!(f = fopen(n, "r"))) {
		if (errno != ENOENT) {
			err(ERROR_EXIT, "cannot open `%s'", n);
		}
		warnx("no crontab for `%s' - using an empty one", User);
		if (!(f = fopen(_PATH_DEVNULL, "r"))) {
			err(ERROR_EXIT, "cannot open `%s'", _PATH_DEVNULL);
		}
	}

	if (fstat(fileno(f), &statbuf) < 0) {
		warn("cannot stat crontab file");
		goto fatal;
	}
	get_time(&statbuf, ts);

	/* Turn off signals. */
	ohup = signal(SIGHUP, SIG_IGN);
	oint = signal(SIGINT, SIG_IGN);
	oquit = signal(SIGQUIT, SIG_IGN);
	oabrt = signal(SIGABRT, SIG_IGN);

	if (!glue_strings(Filename, sizeof Filename, _PATH_TMP,
	    "crontab.XXXXXXXXXX", '/')) {
		warnx("path too long");
		goto fatal;
	}
	if (-1 == (t = mkstemp(Filename))) {
		warn("cannot create `%s'", Filename);
		goto fatal;
	}
#ifdef HAS_FCHOWN
	x = fchown(t, MY_UID(pw), MY_GID(pw));
#else
	x = chown(Filename, MY_UID(pw), MY_GID(pw));
#endif
	if (x < 0) {
		warn("cannot chown `%s'", Filename);
		goto fatal;
	}
	if (!(NewCrontab = fdopen(t, "r+"))) {
		warn("cannot open fd");
		goto fatal;
	}

	Set_LineNum(1);

	skip_header(&ch, f);

	/* copy the rest of the crontab (if any) to the temp file.
	 */
	for (; EOF != ch; ch = get_char(f))
		(void)putc(ch, NewCrontab);
	(void)fclose(f);
	if (fflush(NewCrontab) < OK) {
		err(ERROR_EXIT, "cannot flush output for `%s'", Filename);
	}
	if (change_time(Filename, ts) == -1)
		err(ERROR_EXIT, "cannot set time info for `%s'", Filename);
 again:
	rewind(NewCrontab);
	if (ferror(NewCrontab)) {
		warn("error while writing new crontab to `%s'", Filename);
 fatal:
		(void)unlink(Filename);
		exit(ERROR_EXIT);
	}

	if (((editor = getenv("VISUAL")) == NULL || *editor == '\0') &&
	    ((editor = getenv("EDITOR")) == NULL || *editor == '\0')) {
		editor = EDITOR;
	}

	/* we still have the file open.  editors will generally rewrite the
	 * original file rather than renaming/unlinking it and starting a
	 * new one; even backup files are supposed to be made by copying
	 * rather than by renaming.  if some editor does not support this,
	 * then don't use it.  the security problems are more severe if we
	 * close and reopen the file around the edit.
	 */

	switch (pid = fork()) {
	case -1:
		warn("cannot fork");
		goto fatal;
	case 0:
		/* child */
		if (setgid(MY_GID(pw)) < 0) {
			err(ERROR_EXIT, "cannot setgid(getgid())");
		}
		if (setuid(MY_UID(pw)) < 0) {
			err(ERROR_EXIT, "cannot setuid(getuid())");
		}
		if (chdir(_PATH_TMP) < 0) {
			err(ERROR_EXIT, "cannot chdir to `%s'", _PATH_TMP);
		}
		if (!glue_strings(q, sizeof q, editor, Filename, ' ')) {
			errx(ERROR_EXIT, "editor command line too long");
		}
		(void)execlp(_PATH_BSHELL, _PATH_BSHELL, "-c", q, (char *)0);
		err(ERROR_EXIT, "cannot start `%s'", editor);
		/*NOTREACHED*/
	default:
		/* parent */
		break;
	}

	/* parent */
	for (;;) {
		xpid = waitpid(pid, &waiter, WUNTRACED);
		if (xpid == -1) {
			if (errno != EINTR)
				warn("waitpid() failed waiting for PID %ld "
				    "from `%s'", (long)pid, editor);
		} else if (xpid != pid) {
			warnx("wrong PID (%ld != %ld) from `%s'",
			    (long)xpid, (long)pid, editor);
			goto fatal;
		} else if (WIFSTOPPED(waiter)) {
			(void)kill(getpid(), WSTOPSIG(waiter));
		} else if (WIFEXITED(waiter) && WEXITSTATUS(waiter)) {
			warnx("`%s' exited with status %d\n",
			    editor, WEXITSTATUS(waiter));
			goto fatal;
		} else if (WIFSIGNALED(waiter)) {
			warnx("`%s' killed; signal %d (%score dumped)",
			    editor, WTERMSIG(waiter),
			    WCOREDUMP(waiter) ? "" : "no ");
			goto fatal;
		} else
			break;
	}
	(void)signal(SIGHUP, ohup);
	(void)signal(SIGINT, oint);
	(void)signal(SIGQUIT, oquit);
	(void)signal(SIGABRT, oabrt);

	if (fstat(t, &statbuf) < 0) {
		warn("cannot stat `%s'", Filename);
		goto fatal;
	}
	if (compare_time(&statbuf, ts)) {
		warnx("no changes made to crontab");
		goto remove;
	}
	warnx("installing new crontab");
	switch (replace_cmd()) {
	case 0:
		break;
	case -1:
		for (;;) {
			(void)fpurge(stdin);
			(void)printf("Do you want to retry the same edit? ");
			(void)fflush(stdout);
			q[0] = '\0';
			(void) fgets(q, (int)sizeof(q), stdin);
			switch (q[0]) {
			case 'y':
			case 'Y':
				goto again;
			case 'n':
			case 'N':
				goto abandon;
			default:
				(void)printf("Enter Y or N\n");
			}
		}
		/*NOTREACHED*/
	case -2:
	abandon:
		warnx("edits left in `%s'", Filename);
		goto done;
	default:
		warnx("panic: bad switch() in replace_cmd()");
		goto fatal;
	}
 remove:
	(void)unlink(Filename);
 done:
	log_it(RealUser, Pid, "END EDIT", User);
}

/* returns	0	on success
 *		-1	on syntax error
 *		-2	on install error
 */
static int
replace_cmd(void) {
	char n[MAX_FNAME], n2[MAX_FNAME], envstr[MAX_ENVSTR], lastch;
	FILE *tmp, *fmaxtabsize;
	int ch, eof, fd;
	int error = 0;
	entry *e;
	sig_t oint, oabrt, oquit, ohup;
	uid_t file_owner;
	time_t now = time(NULL);
	char **envp = env_init();
	size_t	maxtabsize;
	struct	stat statbuf;

	if (envp == NULL) {
		warn("Cannot allocate memory.");
		return (-2);
	}

	if (!glue_strings(TempFilename, sizeof TempFilename, SPOOL_DIR,
	    "tmp.XXXXXXXXXX", '/')) {
		TempFilename[0] = '\0';
		warnx("path too long");
		return (-2);
	}
	if ((fd = mkstemp(TempFilename)) == -1 || !(tmp = fdopen(fd, "w+"))) {
		warn("cannot create `%s'", TempFilename);
		if (fd != -1) {
			(void)close(fd);
			(void)unlink(TempFilename);
		}
		TempFilename[0] = '\0';
		return (-2);
	}

	ohup = signal(SIGHUP, SIG_IGN);
	oint = signal(SIGINT, SIG_IGN);
	oquit = signal(SIGQUIT, SIG_IGN);
	oabrt = signal(SIGABRT, SIG_IGN);

	/* Make sure that the crontab is not an unreasonable size.
	 *
	 * XXX This is subject to a race condition--the user could
	 * add stuff to the file after we've checked the size but
	 * before we slurp it in and write it out. We can't just move
	 * the test to test the temp file we later create, because by
	 * that time we've already filled up the crontab disk. Probably
	 * the right thing to do is to do a bytecount in the copy loop
	 * rather than stating the file we're about to read.
	 */
	(void)snprintf(n2, sizeof(n2), "%s/%s", CRONDIR, MAXTABSIZE_FILE);
	if ((fmaxtabsize = fopen(n2, "r")) != NULL)  {
	    if (fgets(n2, (int)sizeof(n2), fmaxtabsize) == NULL)  {
		maxtabsize = 0;
	    } else {
		maxtabsize = atoi(n2);
	    }
	    (void)fclose(fmaxtabsize);
	} else {
	    maxtabsize = MAXTABSIZE_DEFAULT;
	}

	if (fstat(fileno(NewCrontab), &statbuf))  {
	    warn("error stat'ing crontab input");
	    error = -2;
	    goto done;
	}
	if ((uintmax_t)statbuf.st_size > (uintmax_t)maxtabsize)  {
	    warnx("%ld bytes is larger than the maximum size of %ld bytes",
		(long) statbuf.st_size, (long) maxtabsize);
	    error = -2;
	    goto done;
	}

	/* write a signature at the top of the file.
	 *
	 * VERY IMPORTANT: make sure NHEADER_LINES agrees with this code.
	 */
	(void)fprintf(tmp, "# DO NOT EDIT THIS FILE - edit the master and reinstall.\n");
	(void)fprintf(tmp, "# (%s installed on %-24.24s)\n", Filename, ctime(&now));
	(void)fprintf(tmp, "# (Cron version %s -- %s)\n", CRON_VERSION, "$NetBSD: crontab.c,v 1.7.10.1 2014/09/08 19:15:03 msaitoh Exp $");

	/* copy the crontab to the tmp
	 */
	(void)rewind(NewCrontab);
	Set_LineNum(1);
	lastch = EOF;
	while (EOF != (ch = get_char(NewCrontab)))
		(void)putc(lastch = ch, tmp);

	if (lastch != (char)EOF && lastch != '\n') {
		warnx("missing trailing newline in `%s'", Filename);
		error = -1;
		goto done;
	}

	if (ferror(NewCrontab)) {
		warn("error while reading `%s'", Filename);
		error = -2;
		goto done;
	}

	(void)ftruncate(fileno(tmp), ftell(tmp));
	/* XXX this should be a NOOP - is */
	(void)fflush(tmp);

	if (ferror(tmp)) {
		(void)fclose(tmp);
		warn("error while writing new crontab to `%s'", TempFilename);
		error = -2;
		goto done;
	}

	/* check the syntax of the file being installed.
	 */

	/* BUG: was reporting errors after the EOF if there were any errors
	 * in the file proper -- kludged it by stopping after first error.
	 *		vix 31mar87
	 */
	Set_LineNum(1 - NHEADER_LINES);
	CheckErrorCount = 0;  eof = FALSE;
	rewind(tmp);
	while (!CheckErrorCount && !eof) {
		switch (load_env(envstr, tmp)) {
		case ERR:
			/* check for data before the EOF */
			if (envstr[0] != '\0') {
				Set_LineNum(LineNumber + 1);
				check_error("premature EOF");
			}
			eof = TRUE;
			break;
		case FALSE:
			e = load_entry(tmp, check_error, pw, envp);
			if (e)
				free(e);
			break;
		case TRUE:
			break;
		}
	}

	if (CheckErrorCount != 0) {
		warnx("errors in crontab file, can't install.");
		(void)fclose(tmp);
		error = -1;
		goto done;
	}

	file_owner = (getgid() == getegid()) ? ROOT_UID : pw->pw_uid;

#ifdef HAVE_FCHOWN
	error = fchown(fileno(tmp), file_owner, (uid_t)-1);
#else
	error = chown(TempFilename, file_owner, (gid_t)-1);
#endif
	if (error < OK) {
		warn("cannot chown `%s'", TempFilename);
		(void)fclose(tmp);
		error = -2;
		goto done;
	}

	if (fclose(tmp) == EOF) {
		warn("error closing file");
		error = -2;
		goto done;
	}

	if (!glue_strings(n, sizeof n, SPOOL_DIR, User, '/')) {
		warnx("path too long");
		error = -2;
		goto done;
	}
	if (rename(TempFilename, n)) {
		warn("error renaming `%s' to `%s'", TempFilename, n);
		error = -2;
		goto done;
	}
	TempFilename[0] = '\0';
	log_it(RealUser, Pid, "REPLACE", User);

	poke_daemon();

done:
	(void)signal(SIGHUP, ohup);
	(void)signal(SIGINT, oint);
	(void)signal(SIGQUIT, oquit);
	(void)signal(SIGABRT, oabrt);
	if (TempFilename[0]) {
		(void) unlink(TempFilename);
		TempFilename[0] = '\0';
	}
	return (error);
}

static void
poke_daemon(void) {
	struct timespec ts[2];
	(void) clock_gettime(CLOCK_REALTIME, ts);
	ts[1] = ts[0];
	if (change_time(SPOOL_DIR, ts) == -1)
		warn("can't update times on spooldir %s", SPOOL_DIR);
}

/* int allowed(const char *username, const char *allow_file, const char *deny_file)
 *	returns TRUE if (allow_file exists and user is listed)
 *	or (deny_file exists and user is NOT listed).
 *	root is always allowed.
 */
static int
allowed(const char *username, const char *allow_file, const char *deny_file) {
	FILE	*fp;
	int	isallowed;

	if (strcmp(username, ROOT_USER) == 0)
		return (TRUE);
#ifdef ALLOW_ONLY_ROOT
	isallowed = FALSE;
#else
	isallowed = TRUE;
#endif
	if ((fp = fopen(allow_file, "r")) != NULL) {
		isallowed = in_file(username, fp, FALSE);
		(void)fclose(fp);
	} else if ((fp = fopen(deny_file, "r")) != NULL) {
		isallowed = !in_file(username, fp, FALSE);
		(void)fclose(fp);
	}
	return (isallowed);
}
/* int in_file(const char *string, FILE *file, int error)
 *	return TRUE if one of the lines in file matches string exactly,
 *	FALSE if no lines match, and error on error.
 */
static int
in_file(const char *string, FILE *file, int error)
{
	char line[MAX_TEMPSTR];
	char *endp;

	if (fseek(file, 0L, SEEK_SET))
		return (error);
	while (fgets(line, MAX_TEMPSTR, file)) {
		if (line[0] != '\0') {
			endp = &line[strlen(line) - 1];
			if (*endp != '\n')
				return (error);
			*endp = '\0';
			if (0 == strcmp(line, string))
				return (TRUE);
		}
	}
	if (ferror(file))
		return (error);
	return (FALSE);
}

#ifdef HAVE_SAVED_UIDS

static int relinguish_priv(void) {
	return (setegid(rgid) || seteuid(ruid)) ? -1 : 0;
}

static int regain_priv(void) {
	return (setegid(egid) || seteuid(euid)) ? -1 : 0;
}

#else /*HAVE_SAVED_UIDS*/

static int relinguish_priv(void) {
	return (setregid(egid, rgid) || setreuid(euid, ruid)) ? -1 : 0;
}

static int regain_priv(void) {
	return (setregid(rgid, egid) || setreuid(ruid, euid)) ? -1 : 0;
}
#endif /*HAVE_SAVED_UIDS*/
