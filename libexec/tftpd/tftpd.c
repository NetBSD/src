/*	$NetBSD: tftpd.c,v 1.16 1999/02/07 21:38:44 aidan Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)tftpd.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: tftpd.c,v 1.16 1999/02/07 21:38:44 aidan Exp $");
#endif
#endif /* not lint */

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton
 * <guyton@rand-unix>.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <signal.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/tftp.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "tftpsubs.h"

#define	DEFAULTUSER	"nobody"

#define	TIMEOUT		5

extern	char *__progname;
int	peer;
int	rexmtval = TIMEOUT;
int	maxtimeout = 5*TIMEOUT;

#define	PKTSIZE	SEGSIZE+4
char	buf[PKTSIZE];
char	ackbuf[PKTSIZE];
struct	sockaddr_in from;
int	fromlen;

/*
 * Null-terminated directory prefix list for absolute pathname requests and
 * search list for relative pathname requests.
 *
 * MAXDIRS should be at least as large as the number of arguments that
 * inetd allows (currently 20).
 */
#define MAXDIRS	20
static struct dirlist {
	char	*name;
	int	len;
} dirs[MAXDIRS+1];
static int	suppress_naks;
static int	logging;
static int	secure;
static char	*securedir;

struct formats;

static void tftp __P((struct tftphdr *, int));
static const char *errtomsg __P((int));
static void nak __P((int));
static char *verifyhost __P((struct sockaddr_in *));
static void usage __P((void));
void timer __P((int));
void sendfile __P((struct formats *));
void recvfile __P((struct formats *));
void justquit __P((int));
int validate_access __P((char **, int));
int main __P((int, char **));

struct formats {
	char	*f_mode;
	int	(*f_validate) __P((char **, int));
	void	(*f_send) __P((struct formats *));
	void	(*f_recv) __P((struct formats *));
	int	f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
	{ 0 }
};

static void
usage()
{
	syslog(LOG_ERR,
    "Usage: %s [-ln] [-u user] [-g group] [-s directory] [directory ...]",
		    __progname);
	exit(1);
}

int
main(argc, argv)
	int    argc;
	char **argv;
{
	struct passwd *pwent;
	struct group *grent;
	struct tftphdr *tp;
	int n = 0;
	int ch, on;
	int fd = 0;
	struct sockaddr_in sin;
	int len;
	char *tgtuser, *tgtgroup, *ep;
	uid_t curuid, tgtuid;
	gid_t curgid, tgtgid;
	long nid;

	openlog("tftpd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	tgtuser = DEFAULTUSER;
	tgtgroup = NULL;
	curuid = getuid();
	curgid = getgid();

	while ((ch = getopt(argc, argv, "g:lns:u:")) != -1)
		switch (ch) {

		case 'g':
			tgtgroup = optarg;
			break;

		case 'l':
			logging = 1;
			break;

		case 'n':
			suppress_naks = 1;
			break;

		case 's':
			secure = 1;
			securedir = optarg;
			break;

		case 'u':
			tgtuser = optarg;
			break;

		default:
			usage();
			break;
		}

	if (optind < argc) {
		struct dirlist *dirp;

		/* Get list of directory prefixes. Skip relative pathnames. */
		for (dirp = dirs; optind < argc && dirp < &dirs[MAXDIRS];
		     optind++) {
			if (argv[optind][0] == '/') {
				dirp->name = argv[optind];
				dirp->len  = strlen(dirp->name);
				dirp++;
			}
		}
	}

	if (*tgtuser == '\0' || (tgtgroup != NULL && *tgtgroup == '\0'))
		usage();

	nid = (strtol(tgtuser, &ep, 10));
	if (*ep == '\0') {
		if (nid > UID_MAX) {
			syslog(LOG_ERR, "uid %ld is too large", nid);
			exit(1);
		}
		pwent = getpwuid((uid_t)nid);
	} else
		pwent = getpwnam(tgtuser);
	if (pwent == NULL) {
		syslog(LOG_ERR, "unknown user `%s'", tgtuser);
		exit(1);
	}
	tgtuid = pwent->pw_uid;
	tgtgid = pwent->pw_gid;

	if (tgtgroup != NULL) {
		nid = (strtol(tgtgroup, &ep, 10));
		if (*ep == '\0') {
			if (nid > GID_MAX) {
				syslog(LOG_ERR, "gid %ld is too large", nid);
				exit(1);
			}
			grent = getgrgid((gid_t)nid);
		} else
			grent = getgrnam(tgtgroup);
		if (grent != NULL)
			tgtgid = grent->gr_gid;
		else {
			syslog(LOG_ERR, "unknown group `%s'", tgtgroup);
			exit(1);
		}
	}

	if (secure) {
		if (chdir(securedir) < 0) {
			syslog(LOG_ERR, "chdir %s: %m", securedir);
			exit(1);
		}
		if (chroot(".")) {
			syslog(LOG_ERR, "chroot: %m");
			exit(1);
		}
	}

	syslog(LOG_DEBUG, "running as user `%s' (%d), group `%s' (%d)",
	    tgtuser, tgtuid, tgtgroup ? tgtgroup : "(unspecified)" , tgtgid);
	if (curgid != tgtgid) {
		if (setgid(tgtgid)) {
			syslog(LOG_ERR, "setgid to %d: %m", (int)tgtgid);
			exit(1);
		}
		if (setgroups(0, NULL)) {
			syslog(LOG_ERR, "setgroups: %m");
			exit(1);
		}
	}

	if (curuid != tgtuid) {
		if (setuid(tgtuid)) {
			syslog(LOG_ERR, "setuid to %d: %m", (int)tgtuid);
			exit(1);
		}
	}

	on = 1;
	if (ioctl(fd, FIONBIO, &on) < 0) {
		syslog(LOG_ERR, "ioctl(FIONBIO): %m");
		exit(1);
	}
	fromlen = sizeof (from);
	n = recvfrom(fd, buf, sizeof (buf), 0,
	    (struct sockaddr *)&from, &fromlen);
	if (n < 0) {
		syslog(LOG_ERR, "recvfrom: %m");
		exit(1);
	}
	/*
	 * Now that we have read the message out of the UDP
	 * socket, we fork and exit.  Thus, inetd will go back
	 * to listening to the tftp port, and the next request
	 * to come in will start up a new instance of tftpd.
	 *
	 * We do this so that inetd can run tftpd in "wait" mode.
	 * The problem with tftpd running in "nowait" mode is that
	 * inetd may get one or more successful "selects" on the
	 * tftp port before we do our receive, so more than one
	 * instance of tftpd may be started up.  Worse, if tftpd
	 * break before doing the above "recvfrom", inetd would
	 * spawn endless instances, clogging the system.
	 */
	{
		int pid;
		int i, j;

		for (i = 1; i < 20; i++) {
		    pid = fork();
		    if (pid < 0) {
				sleep(i);
				/*
				 * flush out to most recently sent request.
				 *
				 * This may drop some request, but those
				 * will be resent by the clients when
				 * they timeout.  The positive effect of
				 * this flush is to (try to) prevent more
				 * than one tftpd being started up to service
				 * a single request from a single client.
				 */
				j = sizeof from;
				i = recvfrom(fd, buf, sizeof (buf), 0,
				    (struct sockaddr *)&from, &j);
				if (i > 0) {
					n = i;
					fromlen = j;
				}
		    } else {
				break;
		    }
		}
		if (pid < 0) {
			syslog(LOG_ERR, "fork: %m");
			exit(1);
		} else if (pid != 0) {
			exit(0);
		}
	}

	from.sin_len = sizeof(struct sockaddr_in);
	from.sin_family = AF_INET;

	/*
	 * remember what address this was sent to, so we can respond on the
	 * same interface
	 */
	len = sizeof(sin);
	if (getsockname(fd, (struct sockaddr *)&sin, &len) == 0)
		sin.sin_port = 0;
	else {
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
	}

	alarm(0);
	close(fd);
	close(1);
	peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}
	if (bind(peer, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		syslog(LOG_ERR, "bind: %m");
		exit(1);
	}
	if (connect(peer, (struct sockaddr *)&from, sizeof(from)) < 0) {
		syslog(LOG_ERR, "connect: %m");
		exit(1);
	}
	tp = (struct tftphdr *)buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
	exit(1);
}

/*
 * Handle initial connection protocol.
 */
static void
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	char *cp;
	int first = 1, ecode;
	struct formats *pf;
	char *filename, *mode = NULL; /* XXX gcc */

	filename = cp = tp->th_stuff;
again:
	while (cp < buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	ecode = (*pf->f_validate)(&filename, tp->th_opcode);
	if (logging) {
		syslog(LOG_INFO, "%s: %s request for %s: %s",
			verifyhost(&from),
			tp->th_opcode == WRQ ? "write" : "read",
			filename, errtomsg(ecode));
	}
	if (ecode) {
		/*
		 * Avoid storms of naks to a RRQ broadcast for a relative
		 * bootfile pathname from a diskless Sun.
		 */
		if (suppress_naks && *filename != '/' && ecode == ENOTFOUND)
			exit(0);
		nak(ecode);
		exit(1);
	}
	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf);
	else
		(*pf->f_send)(pf);
	exit(0);
}


FILE *file;

/*
 * Validate file access.  Since we
 * have no uid or gid, for now require
 * file to exist and be publicly
 * readable/writable.
 * If we were invoked with arguments
 * from inetd then the file must also be
 * in one of the given directory prefixes.
 */
int
validate_access(filep, mode)
	char **filep;
	int mode;
{
	struct stat stbuf;
	int	fd;
	struct dirlist *dirp;
	static char pathname[MAXPATHLEN];
	char *filename = *filep;

	/*
	 * Prevent tricksters from getting around the directory restrictions
	 */
	if (strstr(filename, "/../"))
		return (EACCESS);

	if (*filename == '/') {
		/*
		 * Allow the request if it's in one of the approved locations.
		 * Special case: check the null prefix ("/") by looking
		 * for length = 1 and relying on the arg. processing that
		 * it's a /.
		 */
		for (dirp = dirs; dirp->name != NULL; dirp++) {
			if (dirp->len == 1 ||
			    (!strncmp(filename, dirp->name, dirp->len) &&
			     filename[dirp->len] == '/'))
				    break;
		}
		/* If directory list is empty, allow access to any file */
		if (dirp->name == NULL && dirp != dirs)
			return (EACCESS);
		if (stat(filename, &stbuf) < 0)
			return (errno == ENOENT ? ENOTFOUND : EACCESS);
		if (!S_ISREG(stbuf.st_mode))
			return (ENOTFOUND);
		if (mode == RRQ) {
			if ((stbuf.st_mode & S_IROTH) == 0)
				return (EACCESS);
		} else {
			if ((stbuf.st_mode & S_IWOTH) == 0)
				return (EACCESS);
		}
	} else {
		/*
		 * Relative file name: search the approved locations for it.
		 */

		if (!strncmp(filename, "../", 3))
			return (EACCESS);

		/*
		 * Find the first file that exists in any of the directories,
		 * check access on it.
		 */
		if (dirs[0].name != NULL) {
			for (dirp = dirs; dirp->name != NULL; dirp++) {
				snprintf(pathname, sizeof pathname, "%s/%s",
				    dirp->name, filename);
				if (stat(pathname, &stbuf) == 0 &&
				    (stbuf.st_mode & S_IFMT) == S_IFREG) {
					break;
				}
			}
			if (dirp->name == NULL)
				return (ENOTFOUND);
			if (mode == RRQ && !(stbuf.st_mode & S_IROTH))
				return (EACCESS);
			if (mode == WRQ && !(stbuf.st_mode & S_IWOTH))
				return (EACCESS);
			*filep = filename = pathname;
		} else {
			/*
			 * If there's no directory list, take our cue from the
			 * absolute file request check above (*filename == '/'),
			 * and allow access to anything.
			 */
			if (stat(filename, &stbuf) < 0)
				return (errno == ENOENT ? ENOTFOUND : EACCESS);
			if (!S_ISREG(stbuf.st_mode))
				return (ENOTFOUND);
			if (mode == RRQ) {
				if ((stbuf.st_mode & S_IROTH) == 0)
					return (EACCESS);
			} else {
				if ((stbuf.st_mode & S_IWOTH) == 0)
					return (EACCESS);
			}
			*filep = filename;
		}
	}
	fd = open(filename, mode == RRQ ? 0 : 1);
	if (fd < 0)
		return (errno + 100);
	file = fdopen(fd, (mode == RRQ)? "r":"w");
	if (file == NULL) {
		close(fd);
		return errno+100;
	}
	return (0);
}

int	timeout;
jmp_buf	timeoutbuf;

void
timer(dummy)
	int dummy;
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
void
sendfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp;
	struct tftphdr *ap;    /* ack packet */
	int size, n;
	volatile int block;

	signal(SIGALRM, timer);
	dp = r_init();
	ap = (struct tftphdr *)ackbuf;
	block = 1;
	do {
		size = readit(file, &dp, pf->f_convert);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		timeout = 0;
		(void)setjmp(timeoutbuf);

send_data:
		if (send(peer, dp, size + 4, 0) != size + 4) {
			syslog(LOG_ERR, "tftpd: write: %m");
			goto abort;
		}
		read_ahead(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf, sizeof (ackbuf), 0);
			alarm(0);
			if (n < 0) {
				syslog(LOG_ERR, "tftpd: read: %m");
				goto abort;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);

			if (ap->th_opcode == ERROR)
				goto abort;

			if (ap->th_opcode == ACK) {
				if (ap->th_block == block)
					break;
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (ap->th_block == (block -1))
					goto send_data;
			}

		}
		block++;
	} while (size == SEGSIZE);
abort:
	(void) fclose(file);
}

void
justquit(dummy)
	int dummy;
{
	exit(0);
}

/*
 * Receive a file.
 */
void
recvfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp;
	struct tftphdr *ap;    /* ack buffer */
	int n, size;
	volatile int block;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	block = 0;
	do {
		timeout = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		if (send(peer, ackbuf, 4, 0) != 4) {
			syslog(LOG_ERR, "tftpd: write: %m");
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, PKTSIZE, 0);
			alarm(0);
			if (n < 0) {            /* really? */
				syslog(LOG_ERR, "tftpd: read: %m");
				goto abort;
			}
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (dp->th_opcode == ERROR)
				goto abort;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
			}
		}
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == SEGSIZE);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(peer, ackbuf, 4, 0);

	signal(SIGALRM, justquit);      /* just quit on timeout */
	alarm(rexmtval);
	n = recv(peer, buf, sizeof (buf), 0); /* normally times out and quits */
	alarm(0);
	if (n >= 4 &&                   /* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(peer, ackbuf, 4, 0);     /* resend final ack */
	}
abort:
	return;
}

const struct errmsg {
	int	e_code;
	const char *e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

static const char *
errtomsg(error)
	int error;
{
	static char buf[20];
	const struct errmsg *pe;

	if (error == 0)
		return "success";
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			return pe->e_msg;
	sprintf(buf, "error %d", error);
	return buf;
}

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
static void
nak(error)
	int error;
{
	struct tftphdr *tp;
	int length;
	const struct errmsg *pe;

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
		strcpy(tp->th_msg, strerror(error - 100));
	} else {
		tp->th_code = htons((u_short)error);
		strcpy(tp->th_msg, pe->e_msg);
	}
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (send(peer, buf, length, 0) != length)
		syslog(LOG_ERR, "nak: %m");
}

static char *
verifyhost(fromp)
	struct sockaddr_in *fromp;
{
	struct hostent *hp;

	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof (fromp->sin_addr),
			    fromp->sin_family);
	if (hp)
		return hp->h_name;
	else
		return inet_ntoa(fromp->sin_addr);
}
