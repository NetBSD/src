/*	$NetBSD: common.c,v 1.20.4.1 2001/04/26 08:12:47 he Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__RCSID("$NetBSD: common.c,v 1.20.4.1 2001/04/26 08:12:47 he Exp $");
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
#include "lp.h"
#include "pathnames.h"

/*
 * Routines and data common to all the line printer functions.
 */

char	*AF;		/* accounting file */
long	 BR;		/* baud rate if lp is a tty */
char	*CF;		/* name of cifplot filter (per job) */
char	*DF;		/* name of tex filter (per job) */
long	 DU;		/* daeomon user-id */
long	 FC;		/* flags to clear if lp is a tty */
char	*FF;		/* form feed string */
long	 FS;		/* flags to set if lp is a tty */
char	*GF;		/* name of graph(1G) filter (per job) */
long	 HL;		/* print header last */
char	*IF;		/* name of input filter (created per job) */
char	*LF;		/* log file for error messages */
char	*LO;		/* lock file name */
char	*LP;		/* line printer device name */
long	 MC;		/* maximum number of copies allowed */
char	*MS;		/* stty flags to set if lp is a tty */
long	 MX;		/* maximum number of blocks to copy */
char	*NF;		/* name of ditroff filter (per job) */
char	*OF;		/* name of output filter (created once) */
char	*PF;		/* name of vrast filter (per job) */
long	 PL;		/* page length */
long	 PW;		/* page width */
long	 PX;		/* page width in pixels */
long	 PY;		/* page length in pixels */
char	*RF;		/* name of fortran text filter (per job) */
char    *RG;		/* resricted group */
char	*RM;		/* remote machine name */
char	*RP;		/* remote printer name */
long	 RS;		/* restricted to those with local accounts */
long	 RW;		/* open LP for reading and writing */
long	 SB;		/* short banner instead of normal header */
long	 SC;		/* suppress multiple copies */
char	*SD;		/* spool directory */
long	 SF;		/* suppress FF on each print job */
long	 SH;		/* suppress header page */
char	*ST;		/* status file name */
char	*TF;		/* name of troff filter (per job) */
char	*TR;		/* trailer string to be output when Q empties */
char	*VF;		/* name of vplot filter (per job) */
long	 XC;		/* flags to clear for local mode */
long	 XS;		/* flags to set for local mode */

char	line[BUFSIZ];
int	remote;		/* true if sending files to a remote host */

extern uid_t	uid, euid;

static int compar __P((const void *, const void *));

/*
 * Create a TCP connection to host "rhost" at port "rport".
 * If rport == 0, then use the printer service port.
 * Most of this code comes from rcmd.c.
 */
int
getport(rhost, rport)
	char *rhost;
	int rport;
{
	struct addrinfo hints, *res, *r;
	u_int timo = 1;
	int s, lport = IPPORT_RESERVED - 1;
	int error;
	int refuse, trial;
	char pbuf[NI_MAXSERV];

	/*
	 * Get the host address and port number to connect to.
	 */
	if (rhost == NULL)
		fatal("no remote host to connect to");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (rport)
		snprintf(pbuf, sizeof(pbuf), "%d", rport);
	else
		snprintf(pbuf, sizeof(pbuf), "printer");
	error = getaddrinfo(rhost, pbuf, &hints, &res);
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
int
getline(cfp)
	FILE *cfp;
{
	int linel = 0, c;
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
getq(namelist)
	struct queue *(*namelist[]);
{
	struct dirent *d;
	struct queue *q, **queue;
	struct stat stbuf;
	DIR *dirp;
	u_int nitems, arraysz;

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
	queue = (struct queue **)malloc(arraysz * sizeof(struct queue *));
	if (queue == NULL)
		goto errdone;

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (d->d_name[0] != 'c' || d->d_name[1] != 'f')
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
			arraysz *= 2;
			queue = (struct queue **)realloc(queue,
				arraysz * sizeof(struct queue *));
			if (queue == NULL)
				goto errdone;
		}
		queue[nitems-1] = q;
	}
	closedir(dirp);
	if (nitems)
		qsort(queue, nitems, sizeof(struct queue *), compar);
	*namelist = queue;
	return(nitems);

errdone:
	closedir(dirp);
	return(-1);
}

/*
 * Compare modification times.
 */
static int
compar(p1, p2)
	const void *p1, *p2;
{
	if ((*(struct queue **)p1)->q_time < (*(struct queue **)p2)->q_time)
		return(-1);
	if ((*(struct queue **)p1)->q_time > (*(struct queue **)p2)->q_time)
		return(1);
	return(0);
}

/*
 * Figure out whether the local machine is the same
 * as the remote machine (RM) entry (if it exists).
 *
 * XXX not really the right way to determine.
 */
char *
checkremote()
{
	char hname[NI_MAXHOST];
	struct addrinfo hints, *res;
	static char errbuf[128];
	int error;

	remote = 0;	/* assume printer is local */
	if (RM != NULL) {
		/* get the official name of the local host */
		gethostname(hname, sizeof(hname));
		hname[sizeof(hname)-1] = '\0';

		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_CANONNAME;
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		res = NULL;
		error = getaddrinfo(hname, NULL, &hints, &res);
		if (error || !res->ai_canonname) {
			(void)snprintf(errbuf, sizeof(errbuf),
			    "unable to get official name for local machine %s: "
			    "%s", hname, gai_strerror(error));
			if (res)
				freeaddrinfo(res);
			return errbuf;
		} else {
			(void)strncpy(hname, res->ai_canonname,
			    sizeof(hname) - 1);
			hname[sizeof(hname) - 1] = '\0';
		}
		freeaddrinfo(res);

		/* get the official name of RM */
		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_CANONNAME;
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		res = NULL;
		error = getaddrinfo(RM, NULL, &hints, &res);
		if (error || !res->ai_canonname) {
			(void)snprintf(errbuf, sizeof(errbuf),
			    "unable to get official name for local machine %s: "
			    "%s", RM, gai_strerror(error));
			if (res)
				freeaddrinfo(res);
			return errbuf;
		}

		/*
		 * if the two hosts are not the same,
		 * then the printer must be remote.
		 */
		if (strcasecmp(hname, res->ai_canonname) != 0)
			remote = 1;

		freeaddrinfo(res);
	}
	return NULL;
}

/* sleep n milliseconds */
void
delay(n)
	int n;
{
	struct timeval tdelay;

	if (n <= 0 || n > 10000)
		fatal("unreasonable delay period (%d)", n);
	tdelay.tv_sec = n / 1000;
	tdelay.tv_usec = n * 1000 % 1000000;
	(void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tdelay);
}
