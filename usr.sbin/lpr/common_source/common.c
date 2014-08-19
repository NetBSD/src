/*	$NetBSD: common.c,v 1.40.12.1 2014/08/20 00:05:09 tls Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
#if 0
static char sccsid[] = "@(#)common.c	8.5 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: common.c,v 1.40.12.1 2014/08/20 00:05:09 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include "lp.h"
#include "lp.local.h"
#include "pathnames.h"

/*
 * Routines and data common to all the line printer functions.
 */

const char	*AF;		/* accounting file */
long		 BR;		/* baud rate if lp is a tty */
const char	*CF;		/* name of cifplot filter (per job) */
const char	*DF;		/* name of tex filter (per job) */
long		 DU;		/* daeomon user-id */
long		 FC;		/* flags to clear if lp is a tty */
const char	*FF;		/* form feed string */
long		 FS;		/* flags to set if lp is a tty */
const char	*GF;		/* name of graph(1G) filter (per job) */
long		 HL;		/* print header last */
const char	*IF;		/* name of input filter (created per job) */
const char	*LF;		/* log file for error messages */
const char	*LO;		/* lock file name */
const char	*LP;		/* line printer device name */
long		 MC;		/* maximum number of copies allowed */
const char	*MS;		/* stty flags to set if lp is a tty */
long		 MX;		/* maximum number of blocks to copy */
const char	*NF;		/* name of ditroff filter (per job) */
const char	*OF;		/* name of output filter (created once) */
const char	*PF;		/* name of postscript filter (per job) */
long		 PL;		/* page length */
long		 PW;		/* page width */
long		 PX;		/* page width in pixels */
long		 PY;		/* page length in pixels */
const char	*RF;		/* name of fortran text filter (per job) */
const char	*RG;		/* resricted group */
const char	*RM;		/* remote machine name */
const char	*RP;		/* remote printer name */
long		 RS;		/* restricted to those with local accounts */
long		 RW;		/* open LP for reading and writing */
long		 SB;		/* short banner instead of normal header */
long		 SC;		/* suppress multiple copies */
const char	*SD;		/* spool directory */
long		 SF;		/* suppress FF on each print job */
long		 SH;		/* suppress header page */
const char	*ST;		/* status file name */
const char	*TF;		/* name of troff filter (per job) */
const char	*TR;		/* trailer string to be output when Q empties */
const char	*VF;		/* name of vplot/vrast filter (per job) */
long		 XC;		/* flags to clear for local mode */
long		 XS;		/* flags to set for local mode */

char	line[BUFSIZ];
int	remote;		/* true if sending files to a remote host */

extern uid_t	uid, euid;

static int compar(const void *, const void *);

const char *
gethost(const char *hname)
{
	const char *p = strchr(hname, '@');
	return p ? ++p : hname;
}

/*
 * Create a TCP connection to host "rhost". If "rhost" is of the
 * form port@host, use the specified port. Otherwise use the
 * default printer port. Most of this code comes from rcmd.c.
 */
int
getport(const char *rhost)
{
	struct addrinfo hints, *res, *r;
	u_int timo = 1;
	int s, lport = IPPORT_RESERVED - 1;
	int error;
	int refuse, trial;
	char hbuf[NI_MAXSERV], *ptr;
	const char *port = "printer";
	const char *hostname = rhost;

	/*
	 * Get the host address and port number to connect to.
	 */
	if (rhost == NULL)
		fatal("no remote host to connect to");
	(void)strlcpy(hbuf, rhost, sizeof(hbuf));
	for (ptr = hbuf; *ptr; ptr++) 
		if (*ptr == '@') {
			*ptr++ = '\0';
			port = hbuf;
			hostname = ptr;
			break;
		}
	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(hostname, port, &hints, &res);
	if (error)
		fatal("printer/tcp: %s", gai_strerror(error));

	/*
	 * Try connecting to the server.
	 */
retry:
	s = -1;
	refuse = trial = 0;
	for (r = res; r; r = r->ai_next) {
		trial++;
retryport:
		seteuid(euid);
		s = rresvport_af(&lport, r->ai_family);
		seteuid(uid);
		if (s < 0)
			return(-1);
		if (connect(s, r->ai_addr, r->ai_addrlen) < 0) {
			error = errno;
			(void)close(s);
			s = -1;
			errno = error;
			if (errno == EADDRINUSE) {
				lport--;
				goto retryport;
			} else if (errno == ECONNREFUSED)
				refuse++;
			continue;
		} else
			break;
	}
	if (s < 0 && trial == refuse && timo <= 16) {
		sleep(timo);
		timo *= 2;
		goto retry;
	}
	if (res)
		freeaddrinfo(res);
	return(s);
}

/*
 * Getline reads a line from the control file cfp, removes tabs, converts
 *  new-line to null and leaves it in line.
 * Returns 0 at EOF or the number of characters read.
 */
size_t
get_line(FILE *cfp)
{
	size_t linel = 0;
	int c;
	char *lp = line;

	while ((c = getc(cfp)) != '\n' && linel+1<sizeof(line)) {
		if (c == EOF)
			return(0);
		if (c == '\t') {
			do {
				*lp++ = ' ';
				linel++;
			} while ((linel & 07) != 0 && linel+1 < sizeof(line));
			continue;
		}
		*lp++ = c;
		linel++;
	}
	*lp++ = '\0';
	return(linel);
}

/*
 * Scan the current directory and make a list of daemon files sorted by
 * creation time.
 * Return the number of entries and a pointer to the list.
 */
int
getq(struct queue **namelist[])
{
	struct dirent *d;
	struct queue *q, **queue = NULL, **nqueue;
	struct stat stbuf;
	DIR *dirp;
	u_int nitems = 0, arraysz;

	seteuid(euid);
	dirp = opendir(SD);
	seteuid(uid);
	if (dirp == NULL)
		return(-1);
	if (fstat(dirp->dd_fd, &stbuf) < 0)
		goto errdone;

	/*
	 * Estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (int)(stbuf.st_size / 24);
	queue = calloc(arraysz, sizeof(struct queue *));
	if (queue == NULL)
		goto errdone;

	while ((d = readdir(dirp)) != NULL) {
		if (d->d_name[0] != 'c' || d->d_name[1] != 'f'
		    || d->d_name[2] == '\0')
			continue;	/* daemon control files only */
		seteuid(euid);
		if (stat(d->d_name, &stbuf) < 0) {
			seteuid(uid);
			continue;	/* Doesn't exist */
		}
		seteuid(uid);
		q = (struct queue *)malloc(sizeof(time_t)+strlen(d->d_name)+1);
		if (q == NULL)
			goto errdone;
		q->q_time = stbuf.st_mtime;
		strcpy(q->q_name, d->d_name);	/* XXX: strcpy is safe */
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems > arraysz) {
			nqueue = (struct queue **)realloc(queue,
				arraysz * 2 * sizeof(struct queue *));
			if (nqueue == NULL) {
				free(q);
				goto errdone;
			}
			(void)memset(&nqueue[arraysz], 0,
			    arraysz * sizeof(struct queueue *));
			queue = nqueue;
			arraysz *= 2;
		}
		queue[nitems-1] = q;
	}
	closedir(dirp);
	if (nitems)
		qsort(queue, nitems, sizeof(struct queue *), compar);
	*namelist = queue;
	return(nitems);

errdone:
	freeq(queue, nitems);
	closedir(dirp);
	return(-1);
}

void
freeq(struct queue **namelist, u_int nitems)
{
	u_int i;
	if (namelist == NULL)
		return;
	for (i = 0; i < nitems; i++)
		if (namelist[i])
			free(namelist[i]);
	free(namelist);
}

/*
 * Compare modification times.
 */
static int
compar(const void *p1, const void *p2)
{
	const struct queue *const *q1 = p1;
	const struct queue *const *q2 = p2;
	int j1, j2;

	if ((*q1)->q_time < (*q2)->q_time)
		return -1;
	if ((*q1)->q_time > (*q2)->q_time)
		return 1;

	j1 = atoi((*q1)->q_name+3);
	j2 = atoi((*q2)->q_name+3);

        if (j1 == j2)
                return 0;
        if ((j1 < j2 && j2-j1 < 500) || (j1 > j2 && j1-j2 > 500))
                return -1;
        if ((j1 < j2 && j2-j1 > 500) || (j1 > j2 && j1-j2 < 500))
                return 1;
 
	return 0;
}

/*
 * Figure out whether the local machine is the same
 * as the remote machine (RM) entry (if it exists).
 */
const char *
checkremote(void)
{
	char lname[NI_MAXHOST], rname[NI_MAXHOST];
	struct addrinfo hints, *res, *res0;
	static char errbuf[128];
	int error;
	struct ifaddrs *ifap, *ifa;
	const int niflags = NI_NUMERICHOST;
#ifdef __KAME__
	struct sockaddr_in6 sin6;
	struct sockaddr_in6 *sin6p;
#endif

	remote = 0;	/* assume printer is local on failure */

	if (RM == NULL)
		return NULL;

	/* get the local interface addresses */
	if (getifaddrs(&ifap) < 0) {
		(void)snprintf(errbuf, sizeof(errbuf),
		    "unable to get local interface address: %s",
		    strerror(errno));
		return errbuf;
	}

	/* get the remote host addresses (RM) */
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	res = NULL;
	error = getaddrinfo(gethost(RM), NULL, &hints, &res0);
	if (error) {
		(void)snprintf(errbuf, sizeof(errbuf),
		    "unable to resolve remote machine %s: %s",
		    RM, gai_strerror(error));
		freeifaddrs(ifap);
		return errbuf;
	}

	remote = 1;	/* assume printer is remote */

	for (res = res0; res; res = res->ai_next) {
		if (getnameinfo(res->ai_addr, res->ai_addrlen,
		    rname, sizeof(rname), NULL, 0, niflags) != 0)
			continue;
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			sin6p = (struct sockaddr_in6 *)ifa->ifa_addr;
			if (ifa->ifa_addr->sa_family == AF_INET6 &&
			    ifa->ifa_addr->sa_len == sizeof(sin6)) {
				inet6_getscopeid(sin6p, 3);
				if (getnameinfo((struct sockaddr *)&sin6,
				    sin6.sin6_len, lname, sizeof(lname),
				    NULL, 0, niflags) != 0)
					continue;
				if (strcmp(rname, lname) == 0) {
					remote = 0;
					goto done;
				}
			}
		}
	}
done:
	freeaddrinfo(res0);
	freeifaddrs(ifap);
	return NULL;
}

/* sleep n milliseconds */
void
delay(int n)
{
	struct timespec tdelay;

	if (n <= 0 || n > 10000)
		fatal("unreasonable delay period (%d)", n);
	tdelay.tv_sec = n / 1000;
	tdelay.tv_nsec = (n % 1000) * 1000000;
	nanosleep(&tdelay, NULL);
}

void
getprintcap(const char *pr)
{
	char *cp;
	const char *dp;
	int i;

	if ((i = cgetent(&bp, printcapdb, pr)) == -2)
		fatal("can't open printer description file");
	else if (i == -1)
		fatal("unknown printer: %s", pr);
	else if (i == -3)
		fatal("potential reference loop detected in printcap file");

	LP = cgetstr(bp, DEFLP, &cp) == -1 ? _PATH_DEFDEVLP : cp;
	RP = cgetstr(bp, "rp", &cp) == -1 ? DEFLP : cp;
	SD = cgetstr(bp, "sd", &cp) == -1 ? _PATH_DEFSPOOL : cp;
	LO = cgetstr(bp, "lo", &cp) == -1 ? DEFLOCK : cp;
	ST = cgetstr(bp, "st", &cp) == -1 ? DEFSTAT : cp;
	RM = cgetstr(bp, "rm", &cp) == -1 ? NULL : cp;
	if ((dp = checkremote()) != NULL)
		printf("Warning: %s\n", dp);
	LF = cgetstr(bp, "lf", &cp) == -1 ? _PATH_CONSOLE : cp;
}

/*
 * Make sure there's some work to do before forking off a child
 */
int
ckqueue(char *cap)
{
	struct dirent *d;
	DIR *dirp;
	const char *spooldir;
	char *sd = NULL;
	int rv = 0;

	spooldir = cgetstr(cap, "sd", &sd) == -1 ? _PATH_DEFSPOOL : sd;
	if ((dirp = opendir(spooldir)) == NULL) {
		rv = -1;
		goto out;
	}
	while ((d = readdir(dirp)) != NULL) {
		if (d->d_name[0] != 'c' || d->d_name[1] != 'f')
			continue;	/* daemon control files only */
		rv = 1;
		break;
	}
out:
	if (dirp != NULL)
		closedir(dirp);
	if (spooldir != sd)
		free(sd);
	return (rv);
}
