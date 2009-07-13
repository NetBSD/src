/*	$NetBSD: printjob.c,v 1.55 2009/07/13 19:05:42 roy Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)printjob.c	8.7 (Berkeley) 5/10/95";
#else
__RCSID("$NetBSD: printjob.c,v 1.55 2009/07/13 19:05:42 roy Exp $");
#endif
#endif /* not lint */


/*
 * printjob -- print jobs in the queue.
 *
 *	NOTE: the lock file is used to pass information to lpq and lprm.
 *	it does not need to be removed because file locks are dynamic.
 */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

#include <pwd.h>
#include <unistd.h>
#include <sys/uio.h>
#include <signal.h>
#include <termios.h>
#include <syslog.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lp.h"
#include "lp.local.h"
#include "pathnames.h"
#include "extern.h"

#define DORETURN	0	/* absorb fork error */
#define DOABORT		1	/* abort if dofork fails */

/*
 * Error tokens
 */
#define REPRINT		-2
#define ERROR		-1
#define	OK		0
#define	FATALERR	1
#define	NOACCT		2
#define	FILTERERR	3
#define	ACCESS		4

static dev_t	fdev;		/* device of file pointed to by symlink */
static ino_t	fino;		/* inode of file pointed to by symlink */
static FILE	*cfp;		/* control file */
static int	child;		/* id of any filters */
static int	lfd;		/* lock file descriptor */
static int	ofd;		/* output filter file descriptor */
static int	ofilter;	/* id of output filter, if any */
static int	pfd;		/* printer file descriptor */
static int	pid;		/* pid of lpd process */
static int	prchild;	/* id of pr process */
static char	title[80];	/* ``pr'' title */
static int	tof;		/* true if at top of form */

static char	class[32];		/* classification field */
static char	fromhost[32];		/* user's host machine */
				/* indentation size in static characters */
static char	indent[10] = "-i0"; 
static char	jobname[100];		/* job or file name */
static char	length[10] = "-l";	/* page length in lines */
static char	logname[32];		/* user's login name */
static char	pxlength[10] = "-y";	/* page length in pixels */
static char	pxwidth[10] = "-x";	/* page width in pixels */
static char	tempfile[] = "errsXXXXXX"; /* file name for filter output */
static char	tempremote[] = "remoteXXXXXX"; /* file name for remote filter */
static char	width[10] = "-w";	/* page width in static characters */

static void	abortpr(int);
static void	banner(char *, char *);
static int	dofork(int);
static int	dropit(int);
static void	init(void);
static void	setup_ofilter(int);
static void	close_ofilter(void);
static void	openpr(void);
static void	opennet(void);
static void	opentty(void);
static void	openrem(void);
static int	print(int, char *);
static int	printit(char *);
static void	pstatus(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));
static char	response(void);
static void	scan_out(int, char *, int);
static char	*scnline(int, char *, int);
static int	sendfile(int, char *);
static int	sendit(char *);
static void	sendmail(char *, int);
static void	setty(void);
static void	alarmer(int);

void
printjob(void)
{
	struct stat stb;
	struct queue *q, **qp;
	struct queue **queue;
	int i, nitems, fd;
	off_t pidoff;
	int errcnt, count = 0;

	init();					/* set up capabilities */
	(void)write(STDOUT_FILENO, "", 1);	/* ack that daemon is started */

	/* set up log file */
	if ((fd = open(LF, O_WRONLY|O_APPEND, 0664)) < 0) {
		syslog(LOG_ERR, "%s: %m", LF);
		fd = open(_PATH_DEVNULL, O_WRONLY);
	}
	if (fd > 0) {
		(void) dup2(fd, STDERR_FILENO);
		(void) close(fd);
	} else
		(void)close(STDERR_FILENO);

	setgid(getegid());
	pid = getpid();				/* for use with lprm */
	setpgrp(0, pid);
	signal(SIGHUP, abortpr);
	signal(SIGINT, abortpr);
	signal(SIGQUIT, abortpr);
	signal(SIGTERM, abortpr);

	/*
	 * uses short form file names
	 */
	if (chdir(SD) < 0) {
		syslog(LOG_ERR, "%s: %m", SD);
		exit(1);
	}
	if (stat(LO, &stb) == 0 && (stb.st_mode & S_IXUSR))
		exit(0);		/* printing disabled */
	lfd = open(LO, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active daemon present */
			exit(0);
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	pidoff = i = snprintf(line, sizeof(line), "%u\n", pid);
	if (write(lfd, line, i) != i) {
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}

	/*
	 * create the temp filenames.
	 * XXX	arguably we should keep the fds open and fdopen(3) dup()s,
	 * XXX	but we're in a protected directory so it shouldn't matter.
	 */
	if ((fd = mkstemp(tempfile)) != -1) {
		(void)close(fd);
		(void)unlink(tempfile);
	}
	if ((fd = mkstemp(tempremote)) != -1) {
		(void)close(fd);
		(void)unlink(tempremote);
	}

	/*
	 * search the spool directory for work and sort by queue order.
	 */
	if ((nitems = getq(&queue)) < 0) {
		syslog(LOG_ERR, "%s: can't scan %s", printer, SD);
		exit(1);
	}
	if (nitems == 0)		/* no work to do */
		exit(0);
	if (stb.st_mode & S_IXOTH) {	/* reset queue flag */
		stb.st_mode &= ~S_IXOTH;
		if (fchmod(lfd, stb.st_mode & 0777) < 0)
			syslog(LOG_ERR, "%s: %s: %m", printer, LO);
	}
	openpr();			/* open printer or remote */
again:
	/*
	 * we found something to do now do it --
	 *    write the name of the current control file into the lock file
	 *    so the spool queue program can tell what we're working on
	 */
	for (qp = queue; nitems--; free((char *) q)) {
		q = *qp++;
		if (stat(q->q_name, &stb) < 0)
			continue;
		errcnt = 0;
	restart:
		(void)lseek(lfd, pidoff, 0);
		i = snprintf(line, sizeof(line), "%s\n", q->q_name);
		if (write(lfd, line, i) != i)
			syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		if (!remote)
			i = printit(q->q_name);
		else
			i = sendit(q->q_name);
		/*
		 * Check to see if we are supposed to stop printing or
		 * if we are to rebuild the queue.
		 */
		if (fstat(lfd, &stb) == 0) {
			/* stop printing before starting next job? */
			if (stb.st_mode & S_IXUSR)
				goto done;
			/* rebuild queue (after lpc topq) */
			if (stb.st_mode & S_IXOTH) {
				for (free((char *) q); nitems--; free((char *) q))
					q = *qp++;
				stb.st_mode &= ~S_IXOTH;
				if (fchmod(lfd, stb.st_mode & 0777) < 0)
					syslog(LOG_WARNING, "%s: %s: %m",
						printer, LO);
				break;
			}
		}
		if (i == OK)		/* file ok and printed */
			count++;
		else if (i == REPRINT && ++errcnt < 5) {
			/* try reprinting the job */
			syslog(LOG_INFO, "restarting %s", printer);
			if (ofilter > 0)
				close_ofilter();
			(void)close(pfd);	/* close printer */
			if (ftruncate(lfd, pidoff) < 0)
				syslog(LOG_WARNING, "%s: %s: %m", printer, LO);
			openpr();		/* try to reopen printer */
			goto restart;
		} else {
			syslog(LOG_WARNING, "%s: job could not be %s (%s)", printer,
				remote ? "sent to remote host" : "printed", q->q_name);
			if (i == REPRINT) {
				/* ensure we don't attempt this job again */
				(void) unlink(q->q_name);
				q->q_name[0] = 'd';
				(void) unlink(q->q_name);
				if (logname[0])
					sendmail(logname, FATALERR);
			}
		}
	}
	free(queue);
	queue = NULL;
	/*
	 * search the spool directory for more work.
	 */
	if ((nitems = getq(&queue)) < 0) {
		syslog(LOG_ERR, "%s: can't scan %s", printer, SD);
		exit(1);
	}
	if (nitems == 0) {		/* no more work to do */
	done:
		if (count > 0) {	/* Files actually printed */
			if (!SF && !tof)
				(void)write(ofd, FF, strlen(FF));
			if (TR != NULL)		/* output trailer */
				(void)write(ofd, TR, strlen(TR));
		}
		(void)unlink(tempfile);
		(void)unlink(tempremote);
		exit(0);
	}
	goto again;
}

#define FONTLEN	50
char	fonts[4][FONTLEN];	/* fonts for troff */

char ifonts[4][40] = {
	_PATH_VFONTR,
	_PATH_VFONTI,
	_PATH_VFONTB,
	_PATH_VFONTS,
};

/*
 * The remaining part is the reading of the control file (cf)
 * and performing the various actions.
 */
static int
printit(char *file)
{
	int i;
	char *cp;
	int bombed = OK;

	/*
	 * open control file; ignore if no longer there.
	 */
	if ((cfp = fopen(file, "r")) == NULL) {
		syslog(LOG_INFO, "%s: %s: %m", printer, file);
		return(OK);
	}
	/*
	 * Reset troff fonts.
	 */
	for (i = 0; i < 4; i++)
		strlcpy(fonts[i], ifonts[i], sizeof(fonts[i]));
	(void)snprintf(&width[2], sizeof(width) - 2, "%ld", PW);
	indent[2] = '0';
	indent[3] = '\0';

	/*
	 *      read the control file for work to do
	 *
	 *      file format -- first character in the line is a command
	 *      rest of the line is the argument.
	 *      valid commands are:
	 *
	 *		S -- "stat info" for symbolic link protection
	 *		J -- "job name" on banner page
	 *		C -- "class name" on banner page
	 *              L -- "literal" user's name to print on banner
	 *		T -- "title" for pr
	 *		H -- "host name" of machine where lpr was done
	 *              P -- "person" user's login name
	 *              I -- "indent" amount to indent output
	 *		R -- laser dpi "resolution"
	 *              f -- "file name" name of text file to print
	 *		l -- "file name" text file with control chars
	 *		p -- "file name" text file to print with pr(1)
	 *		t -- "file name" troff(1) file to print
	 *		n -- "file name" ditroff(1) file to print
	 *		d -- "file name" dvi file to print
	 *		g -- "file name" plot(1G) file to print
	 *		v -- "file name" plain raster file to print
	 *		c -- "file name" cifplot file to print
	 *		o -- "file name" postscript file to print
	 *		1 -- "R font file" for troff
	 *		2 -- "I font file" for troff
	 *		3 -- "B font file" for troff
	 *		4 -- "S font file" for troff
	 *		N -- "name" of file (used by lpq)
	 *              U -- "unlink" name of file to remove
	 *                    (after we print it. (Pass 2 only)).
	 *		M -- "mail" to user when done printing
	 *
	 *      get_line reads a line and expands tabs to blanks
	 */

	/* pass 1 */

	while (get_line(cfp))
		switch (line[0]) {
		case 'H':
			strlcpy(fromhost, line+1, sizeof(fromhost));
			if (class[0] == '\0')
				strlcpy(class, line+1, sizeof(class));
			continue;

		case 'P':
			strlcpy(logname, line+1, sizeof(logname));
			if (RS) {			/* restricted */
				if (getpwnam(logname) == NULL) {
					bombed = NOACCT;
					sendmail(line+1, bombed);
					goto pass2;
				}
			}
			continue;

		case 'S':
			cp = line+1;
			i = 0;
			while (*cp >= '0' && *cp <= '9')
				i = i * 10 + (*cp++ - '0');
			fdev = i;
			cp++;
			i = 0;
			while (*cp >= '0' && *cp <= '9')
				i = i * 10 + (*cp++ - '0');
			fino = i;
			continue;

		case 'J':
			if (line[1] != '\0')
				strlcpy(jobname, line+1, sizeof(jobname));
			else {
				jobname[0] = ' ';
				jobname[1] = '\0';
			}
			continue;

		case 'C':
			if (line[1] != '\0')
				strlcpy(class, line+1, sizeof(class));
			else if (class[0] == '\0') {
				gethostname(class, sizeof(class));
				class[sizeof(class) - 1] = '\0';
			}
			continue;

		case 'T':	/* header title for pr */
			strlcpy(title, line+1, sizeof(title));
			continue;

		case 'L':	/* identification line */
			if (!SH && !HL)
				banner(line+1, jobname);
			continue;

		case '1':	/* troff fonts */
		case '2':
		case '3':
		case '4':
			if (line[1] != '\0') {
				strlcpy(fonts[line[0]-'1'], line+1,
				    sizeof(fonts[line[0]-'1']));
			}
			continue;

		case 'W':	/* page width */
			strlcpy(width+2, line+1, sizeof(width) - 2);
			continue;

		case 'I':	/* indent amount */
			strlcpy(indent+2, line+1, sizeof(indent) - 2);
			continue;

		default:	/* some file to print */
			switch (i = print(line[0], line+1)) {
			case ERROR:
				if (bombed == OK)
					bombed = FATALERR;
				break;
			case REPRINT:
				(void)fclose(cfp);
				return(REPRINT);
			case FILTERERR:
			case ACCESS:
				bombed = i;
				sendmail(logname, bombed);
			}
			title[0] = '\0';
			continue;

		case 'N':
		case 'U':
		case 'M':
		case 'R':
			continue;
		}

	/* pass 2 */

pass2:
	fseek(cfp, 0L, 0);
	while (get_line(cfp))
		switch (line[0]) {
		case 'L':	/* identification line */
			if (!SH && HL)
				banner(line+1, jobname);
			continue;

		case 'M':
			if (bombed < NOACCT)	/* already sent if >= NOACCT */
				sendmail(line+1, bombed);
			continue;

		case 'U':
			if (strchr(line+1, '/'))
				continue;
			(void)unlink(line+1);
		}
	/*
	 * clean-up in case another control file exists
	 */
	(void)fclose(cfp);
	(void)unlink(file);
	return(bombed == OK ? OK : ERROR);
}

/*
 * Print a file.
 * Set up the chain [ PR [ | {IF, OF} ] ] or {IF, RF, TF, NF, DF, CF, VF}.
 * Return -1 if a non-recoverable error occurred,
 * 2 if the filter detected some errors (but printed the job anyway),
 * 1 if we should try to reprint this job and
 * 0 if all is well.
 * Note: all filters take stdin as the file, stdout as the printer,
 * stderr as the log file, and must not ignore SIGINT.
 */
static int
print(int format, char *file)
{
	FILE *fp;
	int status;
	struct stat stb;
	const char *prog, *av[17];
	char buf[BUFSIZ];
	int n, fi, fo, child_pid, p[2], stopped = 0, nofile;

	if (lstat(file, &stb) < 0 || (fi = open(file, O_RDONLY)) < 0)
		return(ERROR);
	/*
	 * Check to see if data file is a symbolic link. If so, it should
	 * still point to the same file or someone is trying to print
	 * something he shouldn't.
	 */
	if (S_ISLNK(stb.st_mode) && fstat(fi, &stb) == 0 &&
	    (stb.st_dev != fdev || stb.st_ino != fino))
		return(ACCESS);
	if (!SF && !tof) {		/* start on a fresh page */
		(void)write(ofd, FF, strlen(FF));
		tof = 1;
	}
	if (IF == NULL && (format == 'f' || format == 'l')) {
		tof = 0;
		while ((n = read(fi, buf, BUFSIZ)) > 0)
			if (write(ofd, buf, n) != n) {
				(void)close(fi);
				return(REPRINT);
			}
		(void)close(fi);
		return(OK);
	}
	switch (format) {
	case 'p':	/* print file using 'pr' */
		if (IF == NULL) {	/* use output filter */
			prog = _PATH_PR;
			av[0] = "pr";
			av[1] = width;
			av[2] = length;
			av[3] = "-h";
			av[4] = *title ? title : " ";
			av[5] = 0;
			fo = ofd;
			goto start;
		}
		pipe(p);
		if ((prchild = dofork(DORETURN)) == 0) {	/* child */
			dup2(fi, 0);		/* file is stdin */
			dup2(p[1], 1);		/* pipe is stdout */
			closelog();
			nofile = sysconf(_SC_OPEN_MAX);
			for (n = 3; n < nofile; n++)
				(void)close(n);
			execl(_PATH_PR, "pr", width, length,
			    "-h", *title ? title : " ", NULL);
			syslog(LOG_ERR, "cannot execl %s", _PATH_PR);
			exit(2);
		}
		(void)close(p[1]);		/* close output side */
		(void)close(fi);
		if (prchild < 0) {
			prchild = 0;
			(void)close(p[0]);
			return(ERROR);
		}
		fi = p[0];			/* use pipe for input */
	case 'f':	/* print plain text file */
		prog = IF;
		av[1] = width;
		av[2] = length;
		av[3] = indent;
		n = 4;
		break;
	case 'o':	/* print a postscript file */
		if (PF == NULL) {
			/* if PF is not set, handle it like an 'l' */
			prog = IF;
			av[1] = "-c";
			av[2] = width;
			av[3] = length;
			av[4] = indent;
			n = 5;
			break;
		} else {
			prog = PF;
			av[1] = pxwidth;
			av[2] = pxlength;
			n = 3;
			break;
		}
	case 'l':	/* like 'f' but pass control characters */
		prog = IF;
		av[1] = "-c";
		av[2] = width;
		av[3] = length;
		av[4] = indent;
		n = 5;
		break;
	case 'r':	/* print a fortran text file */
		prog = RF;
		av[1] = width;
		av[2] = length;
		n = 3;
		break;
	case 't':	/* print troff output */
	case 'n':	/* print ditroff output */
	case 'd':	/* print tex output */
		(void)unlink(".railmag");
		if ((fo = creat(".railmag", FILMOD)) < 0) {
			syslog(LOG_ERR, "%s: cannot create .railmag", printer);
			(void)unlink(".railmag");
		} else {
			for (n = 0; n < 4; n++) {
				if (fonts[n][0] != '/')
					(void)write(fo, _PATH_VFONT,
					    sizeof(_PATH_VFONT) - 1);
				(void)write(fo, fonts[n], strlen(fonts[n]));
				(void)write(fo, "\n", 1);
			}
			(void)close(fo);
		}
		prog = (format == 't') ? TF : (format == 'n') ? NF : DF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	case 'c':	/* print cifplot output */
		prog = CF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	case 'g':	/* print plot(1G) output */
		prog = GF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	case 'v':	/* print raster output */
		prog = VF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	default:
		(void)close(fi);
		syslog(LOG_ERR, "%s: illegal format character '%c'",
			printer, format);
		return(ERROR);
	}
	if (prog == NULL) {
		(void)close(fi);
		syslog(LOG_ERR,
		    "%s: no filter found in printcap for format character '%c'",
		    printer, format);
		return (ERROR);
	}
	if ((av[0] = strrchr(prog, '/')) != NULL)
		av[0]++;
	else
		av[0] = prog;
	av[n++] = "-n";
	av[n++] = logname;
	if (*jobname != '\0' && strcmp(jobname, " ") != 0) {
		av[n++] = "-j";
		av[n++] = jobname;
	}
	av[n++] = "-h";
	av[n++] = fromhost;
	av[n++] = AF;
	av[n] = 0;
	fo = pfd;
	if (ofilter > 0) {		/* stop output filter */
		write(ofd, "\031\1", 2);
		while ((child_pid =
		    wait3(&status, WUNTRACED, 0)) > 0 && child_pid != ofilter)
			;
		if (WIFSTOPPED(status) == 0) {
			(void)close(fi);
			syslog(LOG_WARNING,
			    "%s: output filter died (retcode=%d termsig=%d)",
				printer, WEXITSTATUS(status), WTERMSIG(status));
			return(REPRINT);
		}
		stopped++;
	}
start:
	if ((child = dofork(DORETURN)) == 0) {	/* child */
		dup2(fi, 0);
		dup2(fo, 1);
		unlink(tempfile);
		n = open(tempfile, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0664);
		if (n >= 0)
			dup2(n, 2);
		closelog();
		nofile = sysconf(_SC_OPEN_MAX);
		for (n = 3; n < nofile; n++)
			(void)close(n);
		execv(prog, __UNCONST(av));
		syslog(LOG_ERR, "cannot execv %s", prog);
		exit(2);
	}
	if (child < 0) {
		child = 0;
		prchild = 0;
		tof = 0;
		syslog(LOG_ERR, "cannot start child process: %m");
		return (ERROR);
	}
	(void)close(fi);
	while ((child_pid = wait(&status)) > 0 && child_pid != child)
		;
	child = 0;
	prchild = 0;
	if (stopped) {		/* restart output filter */
		if (kill(ofilter, SIGCONT) < 0) {
			syslog(LOG_ERR, "cannot restart output filter");
			exit(1);
		}
	}
	tof = 0;

	/* Copy filter output to "lf" logfile */
	if ((fp = fopen(tempfile, "r")) != NULL) {
		while (fgets(buf, sizeof(buf), fp))
			fputs(buf, stderr);
		fclose(fp);
	}

	if (!WIFEXITED(status)) {
		syslog(LOG_WARNING,
		    "%s: Daemon filter '%c' terminated (pid=%d) (termsig=%d)",
			printer, format, (int)child_pid, WTERMSIG(status));
		return(ERROR);
	}
	switch (WEXITSTATUS(status)) {
	case 0:
		tof = 1;
		return(OK);
	case 1:
		return(REPRINT);
	case 2:
		return(ERROR);
	default:
		syslog(LOG_WARNING, "%s: filter '%c' exited (retcode=%d)",
			printer, format, WEXITSTATUS(status));
		return(FILTERERR);
	}
}

/*
 * Send the daemon control file (cf) and any data files.
 * Return -1 if a non-recoverable error occurred, 1 if a recoverable error and
 * 0 if all is well.
 */
static int
sendit(char *file)
{
	int i, err = OK;
	char *cp, last[BUFSIZ];

	/*
	 * open control file
	 */
	if ((cfp = fopen(file, "r")) == NULL)
		return(OK);
	/*
	 *      read the control file for work to do
	 *
	 *      file format -- first character in the line is a command
	 *      rest of the line is the argument.
	 *      commands of interest are:
	 *
	 *            a-z -- "file name" name of file to print
	 *              U -- "unlink" name of file to remove
	 *                    (after we print it. (Pass 2 only)).
	 */

	/*
	 * pass 1
	 */
	while (get_line(cfp)) {
	again:
		if (line[0] == 'S') {
			cp = line+1;
			i = 0;
			while (*cp >= '0' && *cp <= '9')
				i = i * 10 + (*cp++ - '0');
			fdev = i;
			cp++;
			i = 0;
			while (*cp >= '0' && *cp <= '9')
				i = i * 10 + (*cp++ - '0');
			fino = i;
			continue;
		}
		if (line[0] >= 'a' && line[0] <= 'z') {
			strlcpy(last, line, sizeof(last));
			while ((i = get_line(cfp)) != 0)
				if (strcmp(last, line))
					break;
			switch (sendfile('\3', last+1)) {
			case OK:
				if (i)
					goto again;
				break;
			case REPRINT:
				(void)fclose(cfp);
				return(REPRINT);
			case ACCESS:
				sendmail(logname, ACCESS);
			case ERROR:
				err = ERROR;
			}
			break;
		}
	}
	if (err == OK && sendfile('\2', file) > 0) {
		(void)fclose(cfp);
		return(REPRINT);
	}
	/*
	 * pass 2
	 */
	fseek(cfp, 0L, 0);
	while (get_line(cfp))
		if (line[0] == 'U' && strchr(line+1, '/') == 0)
			(void)unlink(line+1);
	/*
	 * clean-up in case another control file exists
	 */
	(void)fclose(cfp);
	(void)unlink(file);
	return(err);
}

/*
 * Send a data file to the remote machine and spool it.
 * Return positive if we should try resending.
 */
static int
sendfile(int type, char *file)
{
	int f, i, amt;
	struct stat stb;
	char buf[BUFSIZ];
	int sizerr, resp;
	extern int rflag;
	char *save_file;

	save_file = file;
	if (type == '\3' && rflag && (OF || IF)) {
		int	save_pfd = pfd;

		(void)unlink(tempremote);
		pfd = open(tempremote, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0664);
		if (pfd == -1) {
			pfd = save_pfd;
			return ERROR;
		}
		setup_ofilter(1);
		switch (i = print('f', file)) {
		case ERROR:
		case REPRINT:
		case FILTERERR:
		case ACCESS:
			return(i);
		}
		close_ofilter();
		pfd = save_pfd;
		file = tempremote;
	}

	if (lstat(file, &stb) < 0 || (f = open(file, O_RDONLY)) < 0)
		return(ERROR);
	/*
	 * Check to see if data file is a symbolic link. If so, it should
	 * still point to the same file or someone is trying to print something
	 * he shouldn't.
	 */
	if (S_ISLNK(stb.st_mode) && fstat(f, &stb) == 0 &&
	    (stb.st_dev != fdev || stb.st_ino != fino))
		return(ACCESS);

	amt = snprintf(buf, sizeof(buf), "%c%lld %s\n", type,
	    (long long)stb.st_size, save_file);
	for (i = 0; ; i++) {
		if (write(pfd, buf, amt) != amt ||
		    (resp = response()) < 0 || resp == '\1') {
			(void)close(f);
			return(REPRINT);
		} else if (resp == '\0')
			break;
		if (i == 0)
			pstatus("no space on remote; waiting for queue to drain");
		if (i == 10)
			syslog(LOG_ALERT, "%s: can't send to %s; queue full",
				printer, RM);
		sleep(5 * 60);
	}
	if (i)
		pstatus("sending to %s", RM);
	sizerr = 0;
	for (i = 0; i < stb.st_size; i += BUFSIZ) {
		struct sigaction osa, nsa;

		amt = BUFSIZ;
		if (i + amt > stb.st_size)
			amt = stb.st_size - i;
		if (sizerr == 0 && read(f, buf, amt) != amt)
			sizerr = 1;
		nsa.sa_handler = alarmer;
		sigemptyset(&nsa.sa_mask);
		sigaddset(&nsa.sa_mask, SIGALRM);
		nsa.sa_flags = 0;
		(void)sigaction(SIGALRM, &nsa, &osa);
		alarm(wait_time);
		if (write(pfd, buf, amt) != amt) {
			alarm(0);
			(void)sigaction(SIGALRM, &osa, NULL);
			(void)close(f);
			return(REPRINT);
		}
		alarm(0);
		(void)sigaction(SIGALRM, &osa, NULL);
	}

	(void)close(f);
	if (sizerr) {
		syslog(LOG_INFO, "%s: %s: changed size", printer, file);
		/* tell recvjob to ignore this file */
		(void)write(pfd, "\1", 1);
		return(ERROR);
	}
	if (write(pfd, "", 1) != 1 || response())
		return(REPRINT);
	return(OK);
}

/*
 * Check to make sure there have been no errors and that both programs
 * are in sync with eachother.
 * Return non-zero if the connection was lost.
 */
static char
response(void)
{
	struct sigaction osa, nsa;
	char resp;

	nsa.sa_handler = alarmer;
	sigemptyset(&nsa.sa_mask);
	sigaddset(&nsa.sa_mask, SIGALRM);
	nsa.sa_flags = 0;
	(void)sigaction(SIGALRM, &nsa, &osa);
	alarm(wait_time);
	if (read(pfd, &resp, 1) != 1) {
		syslog(LOG_INFO, "%s: lost connection", printer);
		resp = -1;
	}
	alarm(0);
	(void)sigaction(SIGALRM, &osa, NULL);
	return (resp);
}

/*
 * Banner printing stuff
 */
static void
banner(char *name1, char *name2)
{
	time_t tvec;

	time(&tvec);
	if (!SF && !tof)
		(void)write(ofd, FF, strlen(FF));
	if (SB) {	/* short banner only */
		if (class[0]) {
			(void)write(ofd, class, strlen(class));
			(void)write(ofd, ":", 1);
		}
		(void)write(ofd, name1, strlen(name1));
		(void)write(ofd, "  Job: ", 7);
		(void)write(ofd, name2, strlen(name2));
		(void)write(ofd, "  Date: ", 8);
		(void)write(ofd, ctime(&tvec), 24);
		(void)write(ofd, "\n", 1);
	} else {	/* normal banner */
		(void)write(ofd, "\n\n\n", 3);
		scan_out(ofd, name1, '\0');
		(void)write(ofd, "\n\n", 2);
		scan_out(ofd, name2, '\0');
		if (class[0]) {
			(void)write(ofd,"\n\n\n",3);
			scan_out(ofd, class, '\0');
		}
		(void)write(ofd, "\n\n\n\n\t\t\t\t\tJob:  ", 15);
		(void)write(ofd, name2, strlen(name2));
		(void)write(ofd, "\n\t\t\t\t\tDate: ", 12);
		(void)write(ofd, ctime(&tvec), 24);
		(void)write(ofd, "\n", 1);
	}
	if (!SF)
		(void)write(ofd, FF, strlen(FF));
	tof = 1;
}

static char *
scnline(int key, char *p, int c)
{
	int scnwidth;

	for (scnwidth = WIDTH; --scnwidth;) {
		key <<= 1;
		*p++ = key & 0200 ? c : BACKGND;
	}
	return (p);
}

#define TRC(q)	(((q)-' ')&0177)

static void
scan_out(int scfd, char *scsp, int dlm)
{
	char *strp;
	int nchrs, j;
	char outbuf[LINELEN+1], *sp, c, cc;
	int d, scnhgt;
	extern const char scnkey[][HEIGHT];	/* in lpdchar.c */

	for (scnhgt = 0; scnhgt++ < HEIGHT+DROP; ) {
		strp = &outbuf[0];
		sp = scsp;
		for (nchrs = 0; ; ) {
			d = dropit(c = TRC(cc = *sp++));
			if ((!d && scnhgt > HEIGHT) || (scnhgt <= DROP && d))
				for (j = WIDTH; --j;)
					*strp++ = BACKGND;
			else
				strp = scnline(scnkey[(int)c][scnhgt-1-d],
				    strp, cc);
			if (*sp == dlm || *sp == '\0' ||
			    nchrs++ >= PW/(WIDTH+1)-1)
				break;
			*strp++ = BACKGND;
			*strp++ = BACKGND;
		}
		while (*--strp == BACKGND && strp >= outbuf)
			;
		strp++;
		*strp++ = '\n';	
		(void)write(scfd, outbuf, strp-outbuf);
	}
}

static int
dropit(int c)
{
	switch(c) {

	case TRC('_'):
	case TRC(';'):
	case TRC(','):
	case TRC('g'):
	case TRC('j'):
	case TRC('p'):
	case TRC('q'):
	case TRC('y'):
		return (DROP);

	default:
		return (0);
	}
}

/*
 * sendmail ---
 *   tell people about job completion
 */
static void
sendmail(char *user, int bombed)
{
	int i, p[2], s, nofile;
	const char *cp = NULL; /* XXX gcc */
	struct stat stb;
	FILE *fp;

	if (user[0] == '-' || user[0] == '/' || !isprint((unsigned char)user[0]))
		return;
	pipe(p);
	if ((s = dofork(DORETURN)) == 0) {		/* child */
		dup2(p[0], 0);
		closelog();
		nofile = sysconf(_SC_OPEN_MAX);
		for (i = 3; i < nofile; i++)
			(void)close(i);
		if ((cp = strrchr(_PATH_SENDMAIL, '/')) != NULL)
			cp++;
		else
			cp = _PATH_SENDMAIL;
		execl(_PATH_SENDMAIL, cp, "-t", NULL);
		_exit(0);
	} else if (s > 0) {				/* parent */
		dup2(p[1], 1);
		printf("To: %s@%s\n", user, fromhost);
		printf("Subject: %s printer job \"%s\"\n", printer,
			*jobname ? jobname : "<unknown>");
		printf("Reply-To: root@%s\n\n", host);
		printf("Your printer job ");
		if (*jobname)
			printf("(%s) ", jobname);
		switch (bombed) {
		case OK:
			printf("\ncompleted successfully\n");
			cp = "OK";
			break;
		default:
		case FATALERR:
			printf("\ncould not be printed\n");
			cp = "FATALERR";
			break;
		case NOACCT:
			printf("\ncould not be printed without an account on %s\n", host);
			cp = "NOACCT";
			break;
		case FILTERERR:
			cp = "FILTERERR";
			if (stat(tempfile, &stb) < 0 || stb.st_size == 0 ||
			    (fp = fopen(tempfile, "r")) == NULL) {
				printf("\nhad some errors and may not have printed\n");
				break;
			}
			printf("\nhad the following errors and may not have printed:\n");
			while ((i = getc(fp)) != EOF)
				putchar(i);
			(void)fclose(fp);
			break;
		case ACCESS:
			printf("\nwas not printed because it was not linked to the original file\n");
			cp = "ACCESS";
		}
		fflush(stdout);
		(void)close(1);
	} else {
		syslog(LOG_ERR, "fork for sendmail failed: %m");
	}
	(void)close(p[0]);
	(void)close(p[1]);
	if (s > 0) {
		wait(NULL);
		syslog(LOG_INFO, "mail sent to user %s about job %s on "
		    "printer %s (%s)", user, *jobname ? jobname : "<unknown>",
		    printer, cp);
	}
}

/*
 * dofork - fork with retries on failure
 */
static int
dofork(int action)
{
	int i, child_pid;
	struct passwd *pw;

	for (i = 0; i < 20; i++) {
		if ((child_pid = fork()) < 0) {
			sleep((unsigned)(i*i));
			continue;
		}
		/*
		 * Child should run as daemon instead of root
		 */
		if (child_pid == 0) {
			pw = getpwuid(DU);
			if (pw == 0) {
				syslog(LOG_ERR, "uid %ld not in password file",
				    DU);
				break;
			}
			initgroups(pw->pw_name, pw->pw_gid);
			setgid(pw->pw_gid);
			setuid(DU);
			signal(SIGCHLD, SIG_DFL);
		}
		return (child_pid);
	}
	syslog(LOG_ERR, "can't fork");

	switch (action) {
	case DORETURN:
		return (-1);
	default:
		syslog(LOG_ERR, "bad action (%d) to dofork", action);
		/*FALL THRU*/
	case DOABORT:
		exit(1);
	}
	/*NOTREACHED*/
}

/*
 * Kill child processes to abort current job.
 */
static void
abortpr(int signo)
{
	(void)unlink(tempfile);
	(void)unlink(tempremote);
	kill(0, SIGINT);
	if (ofilter > 0)
		kill(ofilter, SIGCONT);
	while (wait(NULL) > 0)
		;
	exit(0);
}

static void
init(void)
{
	char *s;

	getprintcap(printer);

	FF = cgetstr(bp, "ff", &s) == -1 ? DEFFF : s;

	if (cgetnum(bp, "du", &DU) < 0)
		DU = DEFUID;
	if (cgetnum(bp, "pw", &PW) < 0)
		PW = DEFWIDTH;
	(void)snprintf(&width[2], sizeof(width) - 2, "%ld", PW);
	if (cgetnum(bp, "pl", &PL) < 0)
		PL = DEFLENGTH;
	(void)snprintf(&length[2], sizeof(length) - 2, "%ld", PL);
	if (cgetnum(bp,"px", &PX) < 0)
		PX = 0;
	(void)snprintf(&pxwidth[2], sizeof(pxwidth) - 2, "%ld", PX);
	if (cgetnum(bp, "py", &PY) < 0)
		PY = 0;
	(void)snprintf(&pxlength[2], sizeof(pxlength) - 2, "%ld", PY);

	AF = cgetstr(bp, "af", &s) == -1 ? NULL : s;
	OF = cgetstr(bp, "of", &s) == -1 ? NULL : s;
	IF = cgetstr(bp, "if", &s) == -1 ? NULL : s;
	RF = cgetstr(bp, "rf", &s) == -1 ? NULL : s;
	TF = cgetstr(bp, "tf", &s) == -1 ? NULL : s;
	NF = cgetstr(bp, "nf", &s) == -1 ? NULL : s;
	DF = cgetstr(bp, "df", &s) == -1 ? NULL : s;
	GF = cgetstr(bp, "gf", &s) == -1 ? NULL : s;
	VF = cgetstr(bp, "vf", &s) == -1 ? NULL : s;
	CF = cgetstr(bp, "cf", &s) == -1 ? NULL : s;
	PF = cgetstr(bp, "pf", &s) == -1 ? NULL : s;
	TR = cgetstr(bp, "tr", &s) == -1 ? NULL : s;

	RS = (cgetcap(bp, "rs", ':') != NULL);
	SF = (cgetcap(bp, "sf", ':') != NULL);
	SH = (cgetcap(bp, "sh", ':') != NULL);
	SB = (cgetcap(bp, "sb", ':') != NULL);
	HL = (cgetcap(bp, "hl", ':') != NULL);
	RW = (cgetcap(bp, "rw", ':') != NULL);

	cgetnum(bp, "br", &BR);
	if (cgetnum(bp, "fc", &FC) < 0)
		FC = 0;
	if (cgetnum(bp, "fs", &FS) < 0)
		FS = 0;
	if (cgetnum(bp, "xc", &XC) < 0)
		XC = 0;
	if (cgetnum(bp, "xs", &XS) < 0)
		XS = 0;
	MS = cgetstr(bp, "ms", &s) == -1 ? NULL : s;

	tof = (cgetcap(bp, "fo", ':') == NULL);
}

/*      
 * Setup output filter - called once for local printer, or (if -r given to lpd)
 * once per file for remote printers
 */     
static void
setup_ofilter(int check_rflag)
{
	extern int rflag;

	if (OF && (!remote || (check_rflag && rflag))) {
		int p[2];
		int i, nofile;
		const char *cp;

		pipe(p);
		if ((ofilter = dofork(DOABORT)) == 0) {	/* child */
			dup2(p[0], 0);		/* pipe is std in */
			dup2(pfd, 1);		/* printer is std out */
			closelog();
			nofile = sysconf(_SC_OPEN_MAX);
			for (i = 3; i < nofile; i++)
				(void)close(i);
			if ((cp = strrchr(OF, '/')) == NULL)
				cp = OF;
			else
				cp++;
			execl(OF, cp, width, length, NULL);
			syslog(LOG_ERR, "%s: %s: %m", printer, OF);
			exit(1);
		}
		(void)close(p[0]);		/* close input side */
		ofd = p[1];			/* use pipe for output */
	} else {
		ofd = pfd;
		ofilter = 0;
	}
}

/*
 * Close the output filter and reset ofd back to the main pfd descriptor
 */
static void
close_ofilter(void)
{
	int i;

	if (ofilter) {
		kill(ofilter, SIGCONT);	/* to be sure */
		(void)close(ofd);
		ofd = pfd;
		while ((i = wait(NULL)) > 0 && i != ofilter)
			;
		ofilter = 0;
	}
}

/*
 * Acquire line printer or remote connection.
 */
static void
openpr(void)
{
	if (!remote && *LP) {
		if (strchr(LP, '@') != NULL)
			opennet();
		else
			opentty();
	} else if (remote) {
		openrem();
	} else {
		syslog(LOG_ERR, "%s: no line printer device or host name",
			printer);
		exit(1);
	}

	/*
	 * Start up an output filter, if needed.
	 */
	setup_ofilter(0);
}

/*
 * Printer connected directly to the network
 * or to a terminal server on the net
 */
static void
opennet(void)
{
	int i;
	int resp;

	for (i = 1; ; i = i < 256 ? i << 1 : i) {
		resp = -1;
		pfd = getport(LP);
		if (pfd < 0 && errno == ECONNREFUSED)
			resp = 1;
		else if (pfd >= 0) {
			/*
			 * need to delay a bit for rs232 lines
			 * to stabilize in case printer is
			 * connected via a terminal server
			 */
			delay(500);
			break;
		}
		if (i == 1) {
		   if (resp < 0)
			pstatus("waiting for %s to come up", LP);
		   else
			pstatus("waiting for access to printer on %s", LP);
		}
		sleep(i);
	}
	pstatus("sending to %s", LP);
}

/*
 * Printer is connected to an RS232 port on this host
 */
static void
opentty(void)
{
	int i;

	for (i = 1; ; i = i < 32 ? i << 1 : i) {
		pfd = open(LP, RW ? O_RDWR : O_WRONLY);
		if (pfd >= 0) {
			delay(500);
			break;
		}
		if (errno == ENOENT) {
			syslog(LOG_ERR, "%s: %m", LP);
			exit(1);
		}
		if (i == 1)
			pstatus("waiting for %s to become ready (offline ?)",
				printer);
		sleep(i);
	}
	if (isatty(pfd))
		setty();
	pstatus("%s is ready and printing", printer);
}

/*
 * Printer is on a remote host
 */
static void
openrem(void)
{
	int i, n;
	int resp;

	for (i = 1; ; i = i < 256 ? i << 1 : i) {
		resp = -1;
		pfd = getport(RM);
		if (pfd >= 0) {
			n = snprintf(line, sizeof(line), "\2%s\n", RP);
			if (write(pfd, line, n) == n &&
			    (resp = response()) == '\0')
				break;
			(void) close(pfd);
		}
		if (i == 1) {
			if (resp < 0)
				pstatus("waiting for %s to come up", RM);
			else {
				pstatus("waiting for queue to be enabled on %s",
					RM);
				i = 256;
			}
		}
		sleep(i);
	}
	pstatus("sending to %s", RM);
}

static void
alarmer(int s)
{
	/* nothing */
}

#if !defined(__NetBSD__)
struct bauds {
	int	baud;
	int	speed;
} bauds[] = {
	50,	B50,
	75,	B75,
	110,	B110,
	134,	B134,
	150,	B150,
	200,	B200,
	300,	B300,
	600,	B600,
	1200,	B1200,
	1800,	B1800,
	2400,	B2400,
	4800,	B4800,
	9600,	B9600,
	19200,	B19200,
	38400,	B38400,
	57600,	B57600,
	115200,	B115200,
	0,	0
};
#endif

/*
 * setup tty lines.
 */
static void
setty(void)
{
	struct info i;
	char **argv, **ap, *p, *val;

	i.fd = pfd;
	i.set = i.wset = 0;
	if (ioctl(i.fd, TIOCEXCL, (char *)0) < 0) {
		syslog(LOG_ERR, "%s: ioctl(TIOCEXCL): %m", printer);
		exit(1);
	}
	if (tcgetattr(i.fd, &i.t) < 0) {
		syslog(LOG_ERR, "%s: tcgetattr: %m", printer);
		exit(1);
	}
	if (BR > 0) {
#if !defined(__NetBSD__)
		struct bauds *bp;
		for (bp = bauds; bp->baud; bp++)
			if (BR == bp->baud)
				break;
		if (!bp->baud) {
			syslog(LOG_ERR, "%s: illegal baud rate %d", printer, BR);
			exit(1);
		}
		cfsetspeed(&i.t, bp->speed);
#else
		cfsetspeed(&i.t, BR);
#endif
		i.set = 1;
	}
	if (MS) {
		if (ioctl(i.fd, TIOCGETD, &i.ldisc) < 0) {
			syslog(LOG_ERR, "%s: ioctl(TIOCGETD): %m", printer);
			exit(1);
		}
		if (ioctl(i.fd, TIOCGWINSZ, &i.win) < 0)
			syslog(LOG_INFO, "%s: ioctl(TIOCGWINSZ): %m",
			       printer);

		argv = (char **)calloc(256, sizeof(char *));
		if (argv == NULL) {
			syslog(LOG_ERR, "%s: calloc: %m", printer);
			exit(1);
		}
		p = strdup(MS);
		ap = argv;
		while ((val = strsep(&p, " \t,")) != NULL) {
			*ap++ = strdup(val);
		}

		for (; *argv; ++argv) {
			if (ksearch(&argv, &i))
				continue;
			if (msearch(&argv, &i))
				continue;
			syslog(LOG_INFO, "%s: unknown stty flag: %s",
			       printer, *argv);
		}
	} else {
		if (FC) {
			sttyclearflags(&i.t, FC);
			i.set = 1;
		}
		if (FS) {
			sttysetflags(&i.t, FS);
			i.set = 1;
		}
		if (XC) {
			sttyclearlflags(&i.t, XC);
			i.set = 1;
		}
		if (XS) {
			sttysetlflags(&i.t, XS);
			i.set = 1;
		}
	}

	if (i.set && tcsetattr(i.fd, TCSANOW, &i.t) < 0) {
		syslog(LOG_ERR, "%s: tcsetattr: %m", printer);
		exit(1);
	}
	if (i.wset && ioctl(i.fd, TIOCSWINSZ, &i.win) < 0)
		syslog(LOG_INFO, "%s: ioctl(TIOCSWINSZ): %m", printer);
	return;
}

#include <stdarg.h>

static void
pstatus(const char *msg, ...)
{
	int fd;
	char *buf;
	va_list ap;
	struct iovec iov[2];

	umask(0);
	fd = open(ST, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0) {
		syslog(LOG_ERR, "%s: %s: %m", printer, ST);
		exit(1);
	}
	ftruncate(fd, 0);
	va_start(ap, msg);
	(void)vasprintf(&buf, msg, ap);
	va_end(ap);

	iov[0].iov_base = buf;
	iov[0].iov_len = strlen(buf);
	iov[1].iov_base = __UNCONST("\n");
	iov[1].iov_len = 1;
	(void)writev(fd, iov, 2);
	(void)close(fd);
	free(buf);
}
