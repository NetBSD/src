/*	$NetBSD: at.c,v 1.31 2016/03/13 00:32:09 dholland Exp $	*/

/*
 *  at.c : Put file into atrun queue
 *  Copyright (C) 1993, 1994  Thomas Koenig
 *
 *  Atrun & Atq modifications
 *  Copyright (C) 1993  David Parsons
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* System Headers */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

/* Local headers */
#include "at.h"
#include "panic.h"
#include "parsetime.h"
#include "perm.h"
#include "pathnames.h"
#include "stime.h"
#include "privs.h"

/* Macros */
#define ALARMC 10		/* Number of seconds to wait for timeout */

#define TIMESIZE 50

enum { ATQ, ATRM, AT, BATCH, CAT };	/* what program we want to run */

/* File scope variables */
#ifndef lint
#if 0
static char rcsid[] = "$OpenBSD: at.c,v 1.15 1998/06/03 16:20:26 deraadt Exp $";
#else
__RCSID("$NetBSD: at.c,v 1.31 2016/03/13 00:32:09 dholland Exp $");
#endif
#endif

const char *no_export[] = {"TERM", "TERMCAP", "DISPLAY", "_"};
static int send_mail = 0;

/* External variables */

extern char **environ;
bool fcreated = false;
char atfile[FILENAME_MAX];

char *atinput = NULL;		/* where to get input from */
unsigned char atqueue = 0;	/* which queue to examine for jobs (atq) */
char atverify = 0;		/* verify time instead of queuing job */

/* Function declarations */

__dead static void sigc	(int);
__dead static void alarmc	(int);
static char *cwdname	(void);
static int  nextjob	(void);
static void writefile	(time_t, unsigned char);
static void list_jobs	(void);
static void process_jobs (int, char **, int);

/* Signal catching functions */

/*ARGSUSED*/
static void
sigc(int signo)
{

	/* If a signal interrupts us, remove the spool file and exit. */
	if (fcreated) {
		privs_enter();
		(void)unlink(atfile);
		privs_exit();
	}
	(void)raise_default_signal(signo);
	exit(EXIT_FAILURE);
}

/*ARGSUSED*/
static void
alarmc(int signo)
{

	/* Time out after some seconds. */
	warnx("File locking timed out");
	sigc(signo);
}

/* Local functions */

static char *
cwdname(void)
{

	/*
	 * Read in the current directory; the name will be overwritten on
	 * subsequent calls.
	 */
	static char path[MAXPATHLEN];

	return getcwd(path, sizeof(path));
}

static int
nextjob(void)
{
	int jobno;
	FILE *fid;

	if ((fid = fopen(_PATH_SEQFILE, "r+")) != NULL) {
		if (fscanf(fid, "%5x", &jobno) == 1) {
			(void)rewind(fid);
			jobno = (1+jobno) % 0xfffff;	/* 2^20 jobs enough? */
			(void)fprintf(fid, "%05x\n", jobno);
		} else
			jobno = EOF;
		(void)fclose(fid);
		return jobno;
	} else if ((fid = fopen(_PATH_SEQFILE, "w")) != NULL) {
		(void)fprintf(fid, "%05x\n", jobno = 1);
		(void)fclose(fid);
		return 1;
	}
	return EOF;
}

static void
writefile(time_t runtimer, unsigned char queue)
{
	/*
	 * This does most of the work if at or batch are invoked for
	 * writing a job.
	 */
	int jobno;
	char *ap, *ppos;
	const char *mailname;
	struct passwd *pass_entry;
	struct stat statbuf;
	int fdes, lockdes, fd2;
	FILE *fp, *fpin;
	struct sigaction act;
	char **atenv;
	int ch;
	mode_t cmask;
	struct flock lock;

	(void)setlocale(LC_TIME, "");

	/*
	 * Install the signal handler for SIGINT; terminate after removing the
	 * spool file if necessary
	 */
	(void)memset(&act, 0, sizeof(act));
	act.sa_handler = sigc;
	(void)sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	(void)sigaction(SIGINT, &act, NULL);

	(void)strlcpy(atfile, _PATH_ATJOBS, sizeof(atfile));
	ppos = atfile + strlen(atfile);

	/*
	 * Loop over all possible file names for running something at this
	 * particular time, see if a file is there; the first empty slot at
	 * any particular time is used.  Lock the file _PATH_LOCKFILE first
	 * to make sure we're alone when doing this.
	 */

	privs_enter();

	if ((lockdes = open(_PATH_LOCKFILE, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR)) < 0)
		perr("Cannot open lockfile " _PATH_LOCKFILE);

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	act.sa_handler = alarmc;
	(void)sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	/*
	 * Set an alarm so a timeout occurs after ALARMC seconds, in case
	 * something is seriously broken.
	 */
	(void)sigaction(SIGALRM, &act, NULL);
	(void)alarm(ALARMC);
	(void)fcntl(lockdes, F_SETLKW, &lock);
	(void)alarm(0);

	if ((jobno = nextjob()) == EOF)
	    perr("Cannot generate job number");

	(void)snprintf(ppos, sizeof(atfile) - (ppos - atfile),
	    "%c%5x%8lx", queue, jobno, (unsigned long) (runtimer/60));

	for (ap = ppos; *ap != '\0'; ap++)
		if (*ap == ' ')
			*ap = '0';

	if (stat(atfile, &statbuf) == -1)
		if (errno != ENOENT)
			perr("Cannot access " _PATH_ATJOBS);

	/*
	 * Create the file. The x bit is only going to be set after it has
	 * been completely written out, to make sure it is not executed in
	 * the meantime.  To make sure they do not get deleted, turn off
	 * their r bit.  Yes, this is a kluge.
	 */
	cmask = umask(S_IRUSR | S_IWUSR | S_IXUSR);
	if ((fdes = open(atfile, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR)) == -1)
		perr("Cannot create atjob file");

	if ((fd2 = dup(fdes)) == -1)
		perr("Error in dup() of job file");

	if (fchown(fd2, real_uid, real_gid) == -1)
		perr("Cannot give away file");

	privs_exit();

	/*
	 * We've successfully created the file; let's set the flag so it
	 * gets removed in case of an interrupt or error.
	 */
	fcreated = true;

	/* Now we can release the lock, so other people can access it */
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	(void)fcntl(lockdes, F_SETLKW, &lock);
	(void)close(lockdes);

	if ((fp = fdopen(fdes, "w")) == NULL)
		panic("Cannot reopen atjob file");

	/*
	 * Get the userid to mail to, first by trying getlogin(), which reads
	 * /etc/utmp, then from $LOGNAME or $USER, finally from getpwuid().
	 */
	mailname = getlogin();
	if (mailname == NULL && (mailname = getenv("LOGNAME")) == NULL)
		mailname = getenv("USER");

	if (mailname == NULL || mailname[0] == '\0' ||
	    strlen(mailname) > LOGIN_NAME_MAX || getpwnam(mailname) == NULL) {
		pass_entry = getpwuid(real_uid);
		if (pass_entry != NULL)
			mailname = pass_entry->pw_name;
	}

	if (atinput != NULL) {
		fpin = freopen(atinput, "r", stdin);
		if (fpin == NULL)
			perr("Cannot open input file");
	}
	(void)fprintf(fp,
	    "#!/bin/sh\n"
	    "# atrun uid=%u gid=%u\n"
	    "# mail %s %d\n",
	    real_uid, real_gid, mailname, send_mail);

	/* Write out the umask at the time of invocation */
	(void)fprintf(fp, "umask %o\n", cmask);

	/*
	 * Write out the environment. Anything that may look like a special
	 * character to the shell is quoted, except for \n, which is done
	 * with a pair of "'s.  Dont't export the no_export list (such as
	 * TERM or DISPLAY) because we don't want these.
	 */
	for (atenv = environ; *atenv != NULL; atenv++) {
		int export = 1;
		char *eqp;

		eqp = strchr(*atenv, '=');
		if (eqp == NULL)
			eqp = *atenv;
		else {
			size_t i;

			for (i = 0; i < __arraycount(no_export); i++) {
				export = export &&
				    strncmp(*atenv, no_export[i],
					(size_t)(eqp - *atenv)) != 0;
			}
			eqp++;
		}

		if (export) {
			(void)fwrite(*atenv, sizeof(char),
			    (size_t)(eqp - *atenv), fp);
			for (ap = eqp; *ap != '\0'; ap++) {
				if (*ap == '\n')
					(void)fprintf(fp, "\"\n\"");
				else {
					if (!isalnum((unsigned char)*ap)) {
						switch (*ap) {
						case '%': case '/': case '{':
						case '[': case ']': case '=':
						case '}': case '@': case '+':
						case '#': case ',': case '.':
						case ':': case '-': case '_':
							break;
						default:
							(void)fputc('\\', fp);
							break;
						}
					}
					(void)fputc(*ap, fp);
				}
			}
			(void)fputs("; export ", fp);
			(void)fwrite(*atenv, sizeof(char),
			    (size_t)(eqp - *atenv - 1), fp);
			(void)fputc('\n', fp);
		}
	}
	/*
	 * Cd to the directory at the time and write out all the
	 * commands the user supplies from stdin.
	 */
	(void)fputs("cd ", fp);
	for (ap = cwdname(); *ap != '\0'; ap++) {
		if (*ap == '\n')
			(void)fprintf(fp, "\"\n\"");
		else {
			if (*ap != '/' && !isalnum((unsigned char)*ap))
				(void)fputc('\\', fp);

			(void)fputc(*ap, fp);
		}
	}
	/*
	 * Test cd's exit status: die if the original directory has been
	 * removed, become unreadable or whatever.
	 */
	(void)fprintf(fp,
	    " || {\n"
	    "\t echo 'Execution directory inaccessible' >&2\n"
	    "\t exit 1\n"
	    "}\n");

	if ((ch = getchar()) == EOF)
		panic("Input error");

	do {
		(void)fputc(ch, fp);
	} while ((ch = getchar()) != EOF);

	(void)fprintf(fp, "\n");
	if (ferror(fp))
		panic("Output error");

	if (ferror(stdin))
		panic("Input error");

	(void)fclose(fp);

 	privs_enter();

	/*
	 * Set the x bit so that we're ready to start executing
	 */
	if (fchmod(fd2, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
		perr("Cannot give away file");

	privs_exit();

	(void)close(fd2);
	(void)fprintf(stderr,
	    "Job %d will be executed using /bin/sh\n", jobno);
}

static void
list_jobs(void)
{
	/*
	 * List all a user's jobs in the queue, by looping through
	 * _PATH_ATJOBS, or everybody's if we are root
	 */
	struct passwd *pw;
	DIR *spool;
	struct dirent *dirent;
	struct stat buf;
	struct tm runtime;
	unsigned long ctm;
	unsigned char queue;
	int jobno;
	time_t runtimer;
	char timestr[TIMESIZE];
	int first = 1;

	privs_enter();

	if (chdir(_PATH_ATJOBS) == -1)
		perr("Cannot change to " _PATH_ATJOBS);

	if ((spool = opendir(".")) == NULL)
		perr("Cannot open " _PATH_ATJOBS);

	/* Loop over every file in the directory */
	while ((dirent = readdir(spool)) != NULL) {
		if (stat(dirent->d_name, &buf) == -1)
			perr("Cannot stat in " _PATH_ATJOBS);

		/*
		 * See it's a regular file and has its x bit turned on and
		 * is the user's
		 */
		if (!S_ISREG(buf.st_mode)
		    || (buf.st_uid != real_uid && real_uid != 0)
		    || !(S_IXUSR & buf.st_mode || atverify))
			continue;

		if (sscanf(dirent->d_name, "%c%5x%8lx", &queue, &jobno, &ctm) != 3)
			continue;

		if (atqueue && queue != atqueue)
			continue;

		runtimer = 60 * (time_t)ctm;
		runtime = *localtime(&runtimer);
#if 1
		/*
		 * Provide a consistent date/time format instead of a
		 * locale-specific one that might have 2 digit years
		 */
		(void)strftime(timestr, TIMESIZE, "%T %F", &runtime);
#else
		(void)strftime(timestr, TIMESIZE, "%X %x", &runtime);
#endif
		if (first) {
			(void)printf("%-*s  %-*s  %-*s  %s\n",
			    (int)strlen(timestr), "Date",
			    LOGIN_NAME_MAX, "Owner",
			    7, "Queue",
			    "Job");
			first = 0;
		}
		pw = getpwuid(buf.st_uid);

		(void)printf("%s  %-*s  %c%-*s  %d\n",
		    timestr,
		    LOGIN_NAME_MAX, pw ? pw->pw_name : "???",
		    queue,
		    6, (S_IXUSR & buf.st_mode) ? "" : "(done)",
		    jobno);
	}
	(void)closedir(spool);
	privs_exit();
}

static void
process_jobs(int argc, char **argv, int what)
{
	/* Delete every argument (job - ID) given */
	int i;
	struct stat buf;
	DIR *spool;
	struct dirent *dirent;
	unsigned long ctm;
	unsigned char queue;
	int jobno;

	privs_enter();

	if (chdir(_PATH_ATJOBS) == -1)
		perr("Cannot change to " _PATH_ATJOBS);

	if ((spool = opendir(".")) == NULL)
		perr("Cannot open " _PATH_ATJOBS);

	privs_exit();

	/* Loop over every file in the directory */
	while((dirent = readdir(spool)) != NULL) {

		privs_enter();
		if (stat(dirent->d_name, &buf) == -1)
			perr("Cannot stat in " _PATH_ATJOBS);
		privs_exit();

		if (sscanf(dirent->d_name, "%c%5x%8lx", &queue, &jobno, &ctm) !=3)
			continue;

		for (i = optind; i < argc; i++) {
			if (atoi(argv[i]) == jobno) {
				if (buf.st_uid != real_uid && real_uid != 0)
					errx(EXIT_FAILURE,
					    "%s: Not owner", argv[i]);

				switch (what) {
				case ATRM:
					privs_enter();

					if (unlink(dirent->d_name) == -1)
						perr(dirent->d_name);

					privs_exit();
					break;

				case CAT: {
					FILE *fp;
					int ch;

					privs_enter();

					fp = fopen(dirent->d_name, "r");

					privs_exit();

					if (!fp)
						perr("Cannot open file");
					else {
						while((ch = getc(fp)) != EOF)
							(void)putchar(ch);
						(void)fclose(fp);
					}
				}
					break;

				default:
					errx(EXIT_FAILURE,
					    "Internal error, process_jobs = %d",
						what);
					break;
				}
			}
		}
	}
	(void)closedir(spool);
}

/* Global functions */

int
main(int argc, char **argv)
{
	int c;
	unsigned char queue = DEFAULT_AT_QUEUE;
	char queue_set = 0;
	char time_set = 0;
	char *pgm;

	int program = AT;			/* our default program */
	const char *options = "q:f:t:mvldbrVc";	/* default options for at */
	int disp_version = 0;
	time_t timer;

	privs_relinquish();

	/* Eat any leading paths */
	if ((pgm = strrchr(argv[0], '/')) == NULL)
		pgm = argv[0];
	else
		pgm++;

	/* find out what this program is supposed to do */
	if (strcmp(pgm, "atq") == 0) {
		program = ATQ;
		options = "q:vV";
	} else if (strcmp(pgm, "atrm") == 0) {
		program = ATRM;
		options = "V";
	} else if (strcmp(pgm, "batch") == 0) {
		program = BATCH;
		options = "f:q:t:mvV";
	}

	/* process whatever options we can process */
	opterr = 1;
	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {
		case 'v':	/* verify time settings */
			atverify = 1;
			break;

		case 'm':	/* send mail when job is complete */
			send_mail = 1;
			break;

		case 'f':
			atinput = optarg;
			break;

		case 'q':	/* specify queue */
			if (strlen(optarg) > 1)
				usage();

			atqueue = queue = *optarg;
			if (!(islower(queue) || isupper(queue)))
				usage();

			queue_set = 1;
			break;
		case 't':	/* touch(1) date format */
			timer = stime(optarg);
			time_set = 1;
			break;

		case 'd':
		case 'r':
			if (program != AT)
				usage();

			program = ATRM;
			options = "V";
			break;

		case 'l':
			if (program != AT)
				usage();

			program = ATQ;
			options = "q:vV";
			break;

		case 'b':
			if (program != AT)
				usage();

			program = BATCH;
			options = "f:q:mvV";
			break;

		case 'V':
			disp_version = 1;
			break;

		case 'c':
			program = CAT;
			options = "";
			break;

		default:
			usage();
			break;
		}
	} /* end of options eating */

	if (disp_version)
		(void)fprintf(stderr, "%s version %.1f\n", pgm, AT_VERSION);

	if (!check_permission())
		errx(EXIT_FAILURE,
		    "You do not have permission to use %s.", pgm);

	/* select our program */
	switch (program) {
	case ATQ:
		if (optind != argc)
			usage();
		list_jobs();
		break;

	case ATRM:
	case CAT:
		if (optind == argc)
			usage();
		process_jobs(argc, argv, program);
		break;

	case AT:
		if (argc > optind) {
			/* -t and timespec argument are mutually exclusive */
			if (time_set) {
				usage();
				exit(EXIT_FAILURE);
			} else {
				timer = parsetime(argc, argv);
				time_set = 1;
			}
		}

		if (atverify) {
			struct tm *tm = localtime(&timer);
			(void)fprintf(stderr, "%s\n", asctime(tm));
		}
		writefile(timer, queue);
		break;

	case BATCH:
		if (queue_set)
			queue = toupper(queue);
		else
			queue = DEFAULT_BATCH_QUEUE;

		if (argc > optind) {
			/* -t and timespec argument are mutually exclusive */
			if (time_set) {
				usage();
				exit(EXIT_FAILURE);
			} else {
				timer = parsetime(argc, argv);
				time_set = 1;
			}
		} else if (!time_set)
			timer = time(NULL);

		if (atverify) {
			struct tm *tm = localtime(&timer);
			(void)fprintf(stderr, "%s\n", asctime(tm));
		}

		writefile(timer, queue);
		break;

	default:
		panic("Internal error");
		break;
	}
	return EXIT_SUCCESS;
}
