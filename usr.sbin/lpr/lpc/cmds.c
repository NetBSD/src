/*	$NetBSD: cmds.c,v 1.25 2016/12/16 04:45:04 mrg Exp $	*/
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
#if 0
static char sccsid[] = "@(#)cmds.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: cmds.c,v 1.25 2016/12/16 04:45:04 mrg Exp $");
#endif
#endif /* not lint */

/*
 * lpc -- line printer control program -- commands:
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lp.h"
#include "lp.local.h"
#include "lpc.h"
#include "extern.h"
#include "pathnames.h"

extern uid_t	uid, euid;

static void	abortpr(int);
static void	cleanpr(void);
static void	disablepr(void);
static int	doarg(const char *);
static int	doselect(const struct dirent *);
static void	enablepr(void);
static void	prstat(void);
static void	putmsg(int, char **);
static int	sortq(const struct dirent **, const struct dirent **);
static void	startpr(int);
static void	stoppr(void);
static int	touch(struct queue *);
static void	unlinkf(const char *);
static void	upstat(const char *);
static int 	getcapdesc(void);
static void 	getcaps(void);

/*
 * kill an existing daemon and disable printing.
 */
void
doabort(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: abort {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			abortpr(1);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		abortpr(1);
	}
}

static void
abortpr(int dis)
{
	FILE *fp;
	struct stat stbuf;
	int pid, fd;

	getcaps();
	printf("%s:\n", printer);

	/*
	 * Turn on the owner execute bit of the lock file to disable printing.
	 */
	if (dis) {
		seteuid(euid);
		if (stat(line, &stbuf) >= 0) {
			if (chmod(line, (stbuf.st_mode & 0777) | 0100) < 0)
				printf("\tcannot disable printing\n");
			else {
				upstat("printing disabled\n");
				printf("\tprinting disabled\n");
			}
		} else if (errno == ENOENT) {
			if ((fd = open(line, O_WRONLY|O_CREAT, 0760)) < 0)
				printf("\tcannot create lock file\n");
			else {
				(void)close(fd);
				upstat("printing disabled\n");
				printf("\tprinting disabled\n");
				printf("\tno daemon to abort\n");
			}
			goto out;
		} else {
			printf("\tcannot stat lock file\n");
			goto out;
		}
	}
	/*
	 * Kill the current daemon to stop printing now.
	 */
	if ((fp = fopen(line, "r")) == NULL) {
		printf("\tcannot open lock file\n");
		goto out;
	}
	if (!get_line(fp) || flock(fileno(fp), LOCK_SH|LOCK_NB) == 0) {
		(void)fclose(fp);	/* unlocks as well */
		printf("\tno daemon to abort\n");
		goto out;
	}
	(void)fclose(fp);
	if (kill(pid = atoi(line), SIGTERM) < 0) {
		if (errno == ESRCH)
			printf("\tno daemon to abort\n");
		else
			printf("\tWarning: daemon (pid %d) not killed\n", pid);
	} else
		printf("\tdaemon (pid %d) killed\n", pid);
out:
	seteuid(uid);
}

/*
 * Write a message into the status file.
 */
static void
upstat(const char *msg)
{
	int fd;
	char statfile[MAXPATHLEN];

	getcaps();
	(void)snprintf(statfile, sizeof(statfile), "%s/%s", SD, ST);
	umask(0);
	fd = open(statfile, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0) {
		printf("\tcannot create status file\n");
		if (fd >= 0)
			(void)close(fd);
		return;
	}
	(void)ftruncate(fd, 0);
	if (msg == NULL)
		(void)write(fd, "\n", 1);
	else
		(void)write(fd, msg, strlen(msg));
	(void)close(fd);
}

/*
 * Remove all spool files and temporaries from the spooling area.
 */
void
clean(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: clean {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			cleanpr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		cleanpr();
	}
}

static int
doselect(const struct dirent *d)
{
	int c = d->d_name[0];

	if ((c == 't' || c == 'c' || c == 'd') && d->d_name[1] == 'f')
		return(1);
	return(0);
}

/*
 * Comparison routine for scandir. Sort by job number and machine, then
 * by `cf', `tf', or `df', then by the sequence letter A-Z, a-z.
 */
static int
sortq(const struct dirent **d1, const struct dirent **d2)
{
	int c1, c2;

	if ((c1 = strcmp((*d1)->d_name + 3, (*d2)->d_name + 3)) != 0)
		return(c1);
	c1 = (*d1)->d_name[0];
	c2 = (*d2)->d_name[0];
	if (c1 == c2)
		return((*d1)->d_name[2] - (*d2)->d_name[2]);
	if (c1 == 'c')
		return(-1);
	if (c1 == 'd' || c2 == 'c')
		return(1);
	return(-1);
}

/*
 * Remove incomplete jobs from spooling area.
 */
static void
cleanpr(void)
{
	int i, n;
	char *cp1, *lp, *ep;
	const char *cp;
	struct dirent **queue;
	int nitems;

	getcaps();
	printf("%s:\n", printer);

	/* XXX depends on SD being non nul */
	ep = line + sizeof(line);
	for (lp = line, cp = SD; (size_t)(lp - line) < sizeof(line) &&
	    (*lp++ = *cp++) != '\0'; )
		;
	lp[-1] = '/';

	seteuid(euid);
	nitems = scandir(SD, &queue, doselect, sortq);
	seteuid(uid);
	if (nitems < 0) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	if (nitems == 0)
		return;
	i = 0;
	do {
		cp = queue[i]->d_name;
		if (*cp == 'c') {
			n = 0;
			while (i + 1 < nitems) {
				cp1 = queue[i + 1]->d_name;
				if (*cp1 != 'd' || strcmp(cp + 3, cp1 + 3))
					break;
				i++;
				n++;
			}
			if (n == 0) {
				strlcpy(lp, cp, ep - lp);
				unlinkf(line);
			}
		} else {
			/*
			 * Must be a df with no cf (otherwise, it would have
			 * been skipped above) or a tf file (which can always
			 * be removed).
			 */
			strlcpy(lp, cp, ep - lp);
			unlinkf(line);
		}
     	} while (++i < nitems);
}
 
static void
unlinkf(const char *name)
{
	seteuid(euid);
	if (unlink(name) < 0)
		printf("\tcannot remove %s\n", name);
	else
		printf("\tremoved %s\n", name);
	seteuid(uid);
}

/*
 * Enable queuing to the printer (allow lpr's).
 */
void
enable(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: enable {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			enablepr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		enablepr();
	}
}

static int
getcapdesc(void)
{
	int st;
	if ((st = cgetent(&bp, printcapdb, printer)) == -2) {
		printf("cannot open printer description file\n");
		return 0;
	} else if (st == -1) {
		printf("unknown printer %s\n", printer);
		return 0;
	} else if (st == -3)
		fatal("potential reference loop detected in printcap file");
	return 1;
}

static void
enablepr(void)
{
	struct stat stbuf;

	getcaps();
	printf("%s:\n", printer);

	/*
	 * Turn off the group execute bit of the lock file to enable queuing.
	 */
	seteuid(euid);
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, stbuf.st_mode & 0767) < 0)
			printf("\tcannot enable queuing\n");
		else
			printf("\tqueuing enabled\n");
	}
	seteuid(uid);
}

/*
 * Disable queuing.
 */
void
disable(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: disable {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			disablepr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		disablepr();
	}
}

static void
disablepr(void)
{
	int fd;
	struct stat stbuf;

	getcaps();
	printf("%s:\n", printer);
	/*
	 * Turn on the group execute bit of the lock file to disable queuing.
	 */
	seteuid(euid);
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 010) < 0)
			printf("\tcannot disable queuing\n");
		else
			printf("\tqueuing disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0670)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void)close(fd);
			printf("\tqueuing disabled\n");
		}
	} else
		printf("\tcannot stat lock file\n");
	seteuid(uid);
}

/*
 * Disable queuing and printing and put a message into the status file
 * (reason for being down).
 */
void
down(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: down {all | printer} [message ...]\n");
		return;
	}
	if (!strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			putmsg(argc - 2, argv + 2);
		}
		return;
	}
	printer = argv[1];
	if (!getcapdesc())
		return;
	putmsg(argc - 2, argv + 2);
}

static void
getcaps(void)
{
	char *cp;
	SD = cgetstr(bp, "sd", &cp) == -1 ? _PATH_DEFSPOOL : cp;
	LO = cgetstr(bp, "lo", &cp) == -1 ?  DEFLOCK : cp;
	ST = cgetstr(bp, "st", &cp) == -1 ? DEFSTAT : cp;
	(void)snprintf(line, sizeof(line), "%s/%s", SD, LO);
}

static void
putmsg(int argc, char **argv)
{
	int fd;
	char *cp1, *cp2;
	char buf[1024];
	struct stat stbuf;

	printf("%s:\n", printer);
	/*
	 * Turn on the group execute bit of the lock file to disable queuing and
	 * turn on the owner execute bit of the lock file to disable printing.
	 */
	seteuid(euid);
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 0110) < 0)
			printf("\tcannot disable queuing\n");
		else
			printf("\tprinter and queuing disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0770)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void)close(fd);
			printf("\tprinter and queuing disabled\n");
		}
		seteuid(uid);
		return;
	} else
		printf("\tcannot stat lock file\n");
	/*
	 * Write the message into the status file.
	 */
	(void)snprintf(line, sizeof(line), "%s/%s", SD, ST);
	fd = open(line, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0) {
		printf("\tcannot create status file\n");
		seteuid(uid);
		if (fd >= 0)
			(void)close(fd);
		return;
	}
	seteuid(uid);
	(void)ftruncate(fd, 0);
	if (argc <= 0) {
		(void)write(fd, "\n", 1);
		(void)close(fd);
		return;
	}
	cp1 = buf;
	while (--argc >= 0) {
		cp2 = *argv++;
		while ((size_t)(cp1 - buf) < sizeof(buf) && (*cp1++ = *cp2++))
			;
		cp1[-1] = ' ';
	}
	cp1[-1] = '\n';
	*cp1 = '\0';
	(void)write(fd, buf, strlen(buf));
	(void)close(fd);
}

/*
 * Exit lpc
 */
void
quit(int argc, char *argv[])
{
	exit(0);
}

/*
 * Kill and restart the daemon.
 */
void
restart(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: restart {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			abortpr(0);
			startpr(0);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		abortpr(0);
		startpr(0);
	}
}

/*
 * Enable printing on the specified printer and startup the daemon.
 */
void
startcmd(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: start {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			startpr(1);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		startpr(1);
	}
}

static void
startpr(int ena)
{
	struct stat stbuf;

	getcaps();
	printf("%s:\n", printer);

	/*
	 * Turn off the owner execute bit of the lock file to enable printing.
	 */
	seteuid(euid);
	if (ena && stat(line, &stbuf) >= 0) {
		if (chmod(line, stbuf.st_mode & (ena == 2 ? 0666 : 0677)) < 0)
			printf("\tcannot enable printing\n");
		else
			printf("\tprinting enabled\n");
	}
	if (!startdaemon(printer))
		printf("\tcouldn't start daemon\n");
	else
		printf("\tdaemon started\n");
	seteuid(uid);
}

/*
 * Print the status of each queue listed or all the queues.
 */
void
status(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1 || (argc == 2 && strcmp(argv[1], "all") == 0)) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			prstat();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		prstat();
	}
}

/*
 * Print the status of the printer queue.
 */
static void
prstat(void)
{
	struct stat stbuf;
	int fd, i;
	struct dirent *dp;
	DIR *dirp;

	getcaps();
	printf("%s:\n", printer);
	if (stat(line, &stbuf) >= 0) {
		printf("\tqueuing is %s\n",
			(stbuf.st_mode & 010) ? "disabled" : "enabled");
		printf("\tprinting is %s\n",
			(stbuf.st_mode & 0100) ? "disabled" : "enabled");
	} else {
		printf("\tqueuing is enabled\n");
		printf("\tprinting is enabled\n");
	}
	if ((dirp = opendir(SD)) == NULL) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	i = 0;
	while ((dp = readdir(dirp)) != NULL) {
		if (*dp->d_name == 'c' && dp->d_name[1] == 'f')
			i++;
	}
	closedir(dirp);
	if (i == 0)
		printf("\tno entries\n");
	else if (i == 1)
		printf("\t1 entry in spool area\n");
	else
		printf("\t%d entries in spool area\n", i);
	fd = open(line, O_RDONLY);
	if (fd < 0 || flock(fd, LOCK_SH|LOCK_NB) == 0) {
		if (fd >= 0)
			(void)close(fd);	/* unlocks as well */
		printf("\tprinter idle\n");
		return;
	}
	(void)close(fd);
	(void)snprintf(line, sizeof(line), "%s/%s", SD, ST);
	fd = open(line, O_RDONLY);
	if (fd >= 0) {
		(void)flock(fd, LOCK_SH);
		(void)fstat(fd, &stbuf);
		if (stbuf.st_size > 0) {
			putchar('\t');
			while ((i = read(fd, line, sizeof(line))) > 0)
				(void)fwrite(line, 1, i, stdout);
		}
		(void)close(fd);	/* unlocks as well */
	}
}

/*
 * Stop the specified daemon after completing the current job and disable
 * printing.
 */
void
stop(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: stop {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			stoppr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		stoppr();
	}
}

static void
stoppr(void)
{
	int fd;
	struct stat stbuf;

	getcaps();
	printf("%s:\n", printer);

	/*
	 * Turn on the owner execute bit of the lock file to disable printing.
	 */
	seteuid(euid);
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 0100) < 0)
			printf("\tcannot disable printing\n");
		else {
			upstat("printing disabled\n");
			printf("\tprinting disabled\n");
		}
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0760)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void)close(fd);
			upstat("printing disabled\n");
			printf("\tprinting disabled\n");
		}
	} else
		printf("\tcannot stat lock file\n");
	seteuid(uid);
}

struct	queue **queue;
int	nitems;
time_t	mtime;

/*
 * Put the specified jobs at the top of printer queue.
 */
void
topq(int argc, char *argv[])
{
	int i;
	struct stat stbuf;
	int changed;

	if (argc < 3) {
		printf("Usage: topq printer [jobnum ...] [user ...]\n");
		return;
	}

	--argc;
	printer = *++argv;
	if (!getcapdesc())
		return;

	getcaps();
	printf("%s:\n", printer);

	seteuid(euid);
	if (chdir(SD) < 0) {
		printf("\tcannot chdir to %s\n", SD);
		goto out;
	}
	seteuid(uid);
	nitems = getq(&queue);
	if (nitems == 0)
		return;
	changed = 0;
	mtime = queue[0]->q_time;
	for (i = argc; --i; ) {
		if (doarg(argv[i]) == 0) {
			printf("\tjob %s is not in the queue\n", argv[i]);
			continue;
		} else
			changed++;
	}
	for (i = 0; i < nitems; i++)
		free(queue[i]);
	free(queue);
	if (!changed) {
		printf("\tqueue order unchanged\n");
		return;
	}
	/*
	 * Turn on the public execute bit of the lock file to
	 * get lpd to rebuild the queue after the current job.
	 */
	seteuid(euid);
	if (changed && stat(LO, &stbuf) >= 0)
		(void)chmod(LO, (stbuf.st_mode & 0777) | 01);

out:
	seteuid(uid);
} 

/*
 * Reposition the job by changing the modification time of
 * the control file.
 */
static int
touch(struct queue *q)
{
	struct timeval tvp[2];
	int ret;

	tvp[0].tv_sec = tvp[1].tv_sec = --mtime;
	tvp[0].tv_usec = tvp[1].tv_usec = 0;
	seteuid(euid);
	ret = utimes(q->q_name, tvp);
	seteuid(uid);
	return (ret);
}

/*
 * Checks if specified job name is in the printer's queue.
 * Returns:  negative (-1) if argument name is not in the queue.
 */
static int
doarg(const char *job)
{
	struct queue **qq;
	int jobnum, n;
	char *cp;
	const char *machine;
	int cnt = 0;
	FILE *fp;

	/*
	 * Look for a job item consisting of system name, colon, number 
	 * (example: ucbarpa:114)  
	 */
	if ((cp = strchr(job, ':')) != NULL) {
		machine = job;
		*cp++ = '\0';
		job = cp;
	} else
		machine = NULL;

	/*
	 * Check for job specified by number (example: 112 or 235ucbarpa).
	 */
	if (isdigit((unsigned char)*job)) {
		jobnum = 0;
		do
			jobnum = jobnum * 10 + (*job++ - '0');
		while (isdigit((unsigned char)*job));
		for (qq = queue + nitems; --qq >= queue; ) {
			n = 0;
			for (cp = (*qq)->q_name+3; isdigit((unsigned char)*cp); )
				n = n * 10 + (*cp++ - '0');
			if (jobnum != n)
				continue;
			if (*job && strcmp(job, cp) != 0)
				continue;
			if (machine != NULL && strcmp(machine, cp) != 0)
				continue;
			if (touch(*qq) == 0) {
				printf("\tmoved %s\n", (*qq)->q_name);
				cnt++;
			}
		}
		return(cnt);
	}
	/*
	 * Process item consisting of owner's name (example: henry).
	 */
	for (qq = queue + nitems; --qq >= queue; ) {
		seteuid(euid);
		fp = fopen((*qq)->q_name, "r");
		seteuid(uid);
		if (fp == NULL)
			continue;
		while (get_line(fp) > 0)
			if (line[0] == 'P')
				break;
		(void)fclose(fp);
		if (line[0] != 'P' || strcmp(job, line+1) != 0)
			continue;
		if (touch(*qq) == 0) {
			printf("\tmoved %s\n", (*qq)->q_name);
			cnt++;
		}
	}
	return(cnt);
}

/*
 * Enable everything and start printer (undo `down').
 */
void
up(int argc, char *argv[])
{
	int c;
	char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: up {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (cgetnext(&bp, printcapdb) > 0) {
			cp1 = prbuf;
			cp2 = bp;
			while ((c = *cp2++) && c != '|' && c != ':' &&
			    (size_t)(cp1 - prbuf) < sizeof(prbuf))
				*cp1++ = c;
			*cp1 = '\0';
			startpr(2);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
		if (!getcapdesc())
			continue;
		startpr(2);
	}
}
