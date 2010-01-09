/*	$NetBSD: tftpd.c,v 1.36 2010/01/09 10:46:31 mbalmer Exp $	*/

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
static char sccsid[] = "@(#)tftpd.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: tftpd.c,v 1.36 2010/01/09 10:46:31 mbalmer Exp $");
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
#include <time.h>
#include <unistd.h>

#include "tftpsubs.h"

#define	DEFAULTUSER	"nobody"

#define	TIMEOUT		5

static int	peer;
static int	rexmtval = TIMEOUT;
static int	maxtimeout = 5*TIMEOUT;

static char	buf[MAXPKTSIZE];
static char	ackbuf[PKTSIZE];
static char	oackbuf[PKTSIZE];
static struct	sockaddr_storage from;
static socklen_t	fromlen;
static int	debug;

static int	tftp_opt_tsize = 0;
static int	tftp_blksize = SEGSIZE;
static int	tftp_tsize = 0;

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
static char	pathsep = '\0';
static char	*securedir;

struct formats;

static const char *errtomsg(int);
static void	nak(int);
static void	tftp(struct tftphdr *, int);
static void	usage(void) __attribute__((__noreturn__));
static char	*verifyhost(struct sockaddr *);
static void	justquit(int);
static void	recvfile(struct formats *, int, int);
static void	sendfile(struct formats *, int, int);
static void	timer(int);
static const char *opcode(int);
static int	validate_access(char **, int);

static struct formats {
	const char	*f_mode;
	int		(*f_validate)(char **, int);
	void		(*f_send)(struct formats *, int, int);
	void		(*f_recv)(struct formats *, int, int);
	int		f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
	{ .f_mode = NULL }
};

static void
usage(void)
{

	syslog(LOG_ERR,
    "Usage: %s [-dln] [-g group] [-p pathsep] [-s directory] [-u user] [directory ...]",
		    getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct sockaddr_storage me;
	struct passwd	*pwent;
	struct group	*grent;
	struct tftphdr	*tp;
	const char	*tgtuser, *tgtgroup;
	char *ep;
	int	n, ch, on, fd;
	int	soopt;
	socklen_t len;
	uid_t	curuid, tgtuid;
	gid_t	curgid, tgtgid;
	long	nid;

	n = 0;
	fd = 0;
	tzset();
	openlog("tftpd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	tgtuser = DEFAULTUSER;
	tgtgroup = NULL;
	curuid = getuid();
	curgid = getgid();

	while ((ch = getopt(argc, argv, "dg:lnp:s:u:w:")) != -1)
		switch (ch) {
		case 'd':
			debug++;
			break;

		case 'g':
			tgtgroup = optarg;
			break;

		case 'l':
			logging = 1;
			break;

		case 'n':
			suppress_naks = 1;
			break;

		case 'p':
			if (optarg[0] == '\0' || optarg[1] != '\0')
				usage();
			pathsep = optarg[0];
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
		if ((uid_t)nid > UID_MAX) {
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
			if ((uid_t)nid > GID_MAX) {
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

	if (logging)
		syslog(LOG_DEBUG, "running as user `%s' (%d), group `%s' (%d)",
		    tgtuser, tgtuid, tgtgroup ? tgtgroup : "(unspecified)",
		    tgtgid);
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
		int i;
		socklen_t j;

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

	/*
	 * remember what address this was sent to, so we can respond on the
	 * same interface
	 */
	len = sizeof(me);
	if (getsockname(fd, (struct sockaddr *)&me, &len) == 0) {
		switch (me.ss_family) {
		case AF_INET:
			((struct sockaddr_in *)&me)->sin_port = 0;
			break;
		case AF_INET6:
			((struct sockaddr_in6 *)&me)->sin6_port = 0;
			break;
		default:
			/* unsupported */
			break;
		}
	} else {
		memset(&me, 0, sizeof(me));
		me.ss_family = from.ss_family;
		me.ss_len = from.ss_len;
	}

	alarm(0);
	close(fd);
	close(1);
	peer = socket(from.ss_family, SOCK_DGRAM, 0);
	if (peer < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}
	if (bind(peer, (struct sockaddr *)&me, me.ss_len) < 0) {
		syslog(LOG_ERR, "bind: %m");
		exit(1);
	}
	if (connect(peer, (struct sockaddr *)&from, from.ss_len) < 0) {
		syslog(LOG_ERR, "connect: %m");
		exit(1);
	}
	soopt = 65536;	/* larger than we'll ever need */
	if (setsockopt(peer, SOL_SOCKET, SO_SNDBUF, (void *) &soopt, sizeof(soopt)) < 0) {
		syslog(LOG_ERR, "set SNDBUF: %m");
		exit(1);
	}
	if (setsockopt(peer, SOL_SOCKET, SO_RCVBUF, (void *) &soopt, sizeof(soopt)) < 0) {
		syslog(LOG_ERR, "set RCVBUF: %m");
		exit(1);
	}

	tp = (struct tftphdr *)buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
	exit(1);
}

static int
blk_handler(struct tftphdr *tp, char *opt, char *val, char *ack,
	    int *ackl, int *ec)
{
	unsigned long bsize;
	char *endp;
	int l;

	/*
	 * On these failures, we could just ignore the blocksize option.
	 * Perhaps that should be a command-line option.
	 */
	errno = 0;
	bsize = strtoul(val, &endp, 10);
	if ((bsize == ULONG_MAX && errno == ERANGE) || *endp) {
		syslog(LOG_NOTICE, "%s: %s request for %s: "
			"illegal value %s for blksize option",
			verifyhost((struct sockaddr *)&from),
			tp->th_opcode == WRQ ? "write" : "read",
			tp->th_stuff, val);
		return 0;
	}
	if (bsize < 8 || bsize > 65464) {
		syslog(LOG_NOTICE, "%s: %s request for %s: "
			"out of range value %s for blksize option",
			verifyhost((struct sockaddr *)&from),
			tp->th_opcode == WRQ ? "write" : "read",
			tp->th_stuff, val);
		return 0;
	}

	tftp_blksize = bsize;
	strcpy(ack + *ackl, "blksize");
	*ackl += 8;
	l = sprintf(ack + *ackl, "%lu", bsize);
	*ackl += l + 1;

	return 0;
}

static int
timeout_handler(struct tftphdr *tp, char *opt, char *val, char *ack,
		int *ackl, int *ec)
{
	unsigned long tout;
	char *endp;
	int l;

	errno = 0;
	tout = strtoul(val, &endp, 10);
	if ((tout == ULONG_MAX && errno == ERANGE) || *endp) {
		syslog(LOG_NOTICE, "%s: %s request for %s: "
			"illegal value %s for timeout option",
			verifyhost((struct sockaddr *)&from),
			tp->th_opcode == WRQ ? "write" : "read",
			tp->th_stuff, val);
		return 0;
	}
	if (tout < 1 || tout > 255) {
		syslog(LOG_NOTICE, "%s: %s request for %s: "
			"out of range value %s for timeout option",
			verifyhost((struct sockaddr *)&from),
			tp->th_opcode == WRQ ? "write" : "read",
			tp->th_stuff, val);
		return 0;
	}

	rexmtval = tout;
	strcpy(ack + *ackl, "timeout");
	*ackl += 8;
	l = sprintf(ack + *ackl, "%lu", tout);
	*ackl += l + 1;

	/*
	 * Arbitrarily pick a maximum timeout on a request to 3
	 * retransmissions if the interval timeout is more than
	 * one minute.  Longest possible timeout is therefore
	 * 3 * 255 - 1, or 764 seconds.
	 */
	if (rexmtval > 60) {
		maxtimeout = rexmtval * 3;
	} else {
		maxtimeout = rexmtval * 5;
	}

	return 0;
}

static int
tsize_handler(struct tftphdr *tp, char *opt, char *val, char *ack,
	      int *ackl, int *ec)
{
	unsigned long fsize;
	char *endp;

	/*
	 * Maximum file even with extended tftp is 65535 blocks of
	 * length 65464, or 4290183240 octets (4784056 less than 2^32).
	 * unsigned long is at least 32 bits on all NetBSD archs.
	 */

	errno = 0;
	fsize = strtoul(val, &endp, 10);
	if ((fsize == ULONG_MAX && errno == ERANGE) || *endp) {
		syslog(LOG_NOTICE, "%s: %s request for %s: "
			"illegal value %s for tsize option",
			verifyhost((struct sockaddr *)&from),
			tp->th_opcode == WRQ ? "write" : "read",
			tp->th_stuff, val);
		return 0;
	}
	if (fsize > (unsigned long) 65535 * 65464) {
		syslog(LOG_NOTICE, "%s: %s request for %s: "
			"out of range value %s for tsize option",
			verifyhost((struct sockaddr *)&from),
			tp->th_opcode == WRQ ? "write" : "read",
			tp->th_stuff, val);
		return 0;
	}

	tftp_opt_tsize = 1;
	tftp_tsize = fsize;
	/*
	 * We will report this later -- either replying with the fsize (WRQ)
	 * or replying with the actual filesize (RRQ).
	 */

	return 0;
}

static const struct tftp_options {
	const char *o_name;
	int (*o_handler)(struct tftphdr *, char *, char *, char *,
			 int *, int *);
} options[] = {
	{ "blksize", blk_handler },
	{ "timeout", timeout_handler },
	{ "tsize", tsize_handler },
	{ .o_name = NULL }
};

/*
 * Get options for an extended tftp session.  Stuff the ones we
 * recognize in oackbuf.
 */
static int
get_options(struct tftphdr *tp, char *cp, int size, char *ackb,
    int *alen, int *err)
{
	const struct tftp_options *op;
	char *option, *value, *endp;
	int r, rv=0, ec=0;

	endp = cp + size;
	while (cp < endp) {
		option = cp;
		while (*cp && cp < endp) {
			*cp = tolower((unsigned char)*cp);
			cp++;
		}
		if (*cp) {
			/* if we have garbage at the end, just ignore it */
			break;
		}
		cp++;	/* skip over NUL */
		value = cp;
		while (*cp && cp < endp) {
			cp++;
		}
		if (*cp) {
			/* if we have garbage at the end, just ignore it */
			break;
		}
		cp++;
		for (op = options; op->o_name; op++) {
			if (strcmp(op->o_name, option) == 0)
				break;
		}
		if (op->o_name) {
			r = op->o_handler(tp, option, value, ackb, alen, &ec);
			if (r < 0) {
				rv = -1;
				break;
			}
			rv++;
		} /* else ignore unknown options */
	}
	
	if (rv < 0)
		*err = ec;

	return rv;
}

/*
 * Handle initial connection protocol.
 */
static void
tftp(struct tftphdr *tp, int size)
{
	struct formats *pf;
	char	*cp;
	char	*filename, *mode;
	int	 first, ecode, alen, etftp = 0, r;

	ecode = 0;	/* XXX gcc */
	first = 1;
	mode = NULL;

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
		*cp = tolower((unsigned char)*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	/*
	 * cp currently points to the NUL byte following the mode.
	 *
	 * If we have some valid options, then let's assume that we're
	 * now dealing with an extended tftp session.  Note that if we
	 * don't get any options, then we *must* assume that we do not
	 * have an extended tftp session.  If we get options, we fill
	 * in the ack buf to acknowledge them.  If we skip that, then
	 * the client *must* assume that we are not using an extended
	 * session.
	 */
	size -= (++cp - (char *) tp);
	if (size > 0 && *cp) {
		alen = 2; /* Skip over opcode */
		r = get_options(tp, cp, size, oackbuf, &alen, &ecode);
		if (r > 0) {
			etftp = 1;
		} else if (r < 0) {
			nak(ecode);
			exit(1);
		}
	}
	/*
	 * Globally replace the path separator given in the -p option
	 * with / to cope with clients expecting a non-unix path separator.
	 */
	if (pathsep != '\0') {
		for (cp = filename; *cp != '\0'; ++cp) {
			if (*cp == pathsep)
				*cp = '/';
		}
	}
	ecode = (*pf->f_validate)(&filename, tp->th_opcode);
	if (logging) {
		syslog(LOG_INFO, "%s: %s request for %s: %s",
			verifyhost((struct sockaddr *)&from),
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

	if (etftp) {
		struct tftphdr *oack_h;

		if (tftp_opt_tsize) {
			int l;

			strcpy(oackbuf + alen, "tsize");
			alen += 6;
			l = sprintf(oackbuf + alen, "%u", tftp_tsize);
			alen += l + 1;
		}
		oack_h = (struct tftphdr *) oackbuf;
		oack_h->th_opcode = htons(OACK);
	}

	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf, etftp, alen);
	else
		(*pf->f_send)(pf, etftp, alen);
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
validate_access(char **filep, int mode)
{
	struct stat	 stbuf;
	struct dirlist	*dirp;
	static char	 pathname[MAXPATHLEN];
	char		*filename;
	int		 fd;

	filename = *filep;

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

	if (tftp_opt_tsize && mode == RRQ)
		tftp_tsize = (unsigned long) stbuf.st_size;

	fd = open(filename, mode == RRQ ? O_RDONLY : O_WRONLY | O_TRUNC);
	if (fd < 0)
		return (errno + 100);
	file = fdopen(fd, (mode == RRQ)? "r":"w");
	if (file == NULL) {
		close(fd);
		return (errno + 100);
	}
	return (0);
}

static int	timeout;
static jmp_buf	timeoutbuf;

static void
timer(int dummy)
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

static const char *
opcode(int code)
{
	static char obuf[64];

	switch (code) {
	case RRQ:
		return "RRQ";
	case WRQ:
		return "WRQ";
	case DATA:
		return "DATA";
	case ACK:
		return "ACK";
	case ERROR:
		return "ERROR";
	case OACK:
		return "OACK";
	default:
		(void)snprintf(obuf, sizeof(obuf), "*code 0x%x*", code);
		return obuf;
	}
}

/*
 * Send the requested file.
 */
static void
sendfile(struct formats *pf, volatile int etftp, int acklength)
{
	volatile unsigned int block;
	struct tftphdr	*dp;
	struct tftphdr	*ap;    /* ack packet */
	volatile int	 size;
	int n;

	signal(SIGALRM, timer);
	ap = (struct tftphdr *)ackbuf;
	if (etftp) {
		dp = (struct tftphdr *)oackbuf;
		size = acklength - 4;
		block = 0;
	} else {
		dp = r_init();
		size = 0;
		block = 1;
	}

	do {
		if (block > 0) {
			size = readit(file, &dp, tftp_blksize, pf->f_convert);
			if (size < 0) {
				nak(errno + 100);
				goto abort;
			}
			dp->th_opcode = htons((u_short)DATA);
			dp->th_block = htons((u_short)block);
		}
		timeout = 0;
		(void)setjmp(timeoutbuf);

send_data:
		if (!etftp && debug)
			syslog(LOG_DEBUG, "Send DATA %u", block);
		if ((n = send(peer, dp, size + 4, 0)) != size + 4) {
			syslog(LOG_ERR, "tftpd: write: %m");
			goto abort;
		}
		if (block)
			read_ahead(file, tftp_blksize, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf, tftp_blksize, 0);
			alarm(0);
			if (n < 0) {
				syslog(LOG_ERR, "tftpd: read: %m");
				goto abort;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);
			switch (ap->th_opcode) {
			case ERROR:
				goto abort;

			case ACK:
				if (etftp && ap->th_block == 0) {
					etftp = 0;
					acklength = 0;
					dp = r_init();
					goto done;
				}
				if (ap->th_block == (u_short)block)
					goto done;
				if (debug)
					syslog(LOG_DEBUG, "Resync ACK %u != %u",
					    (unsigned int)ap->th_block, block);
				/* Re-synchronize with the other side */
				(void) synchnet(peer, tftp_blksize);
				if (ap->th_block == (u_short)(block - 1))
					goto send_data;
			default:
				syslog(LOG_INFO, "Received %s in sendfile\n",
				    opcode(dp->th_opcode));
			}

		}
done:
		if (debug)
			syslog(LOG_DEBUG, "Received ACK for block %u", block);
		if (block == UINT16_MAX && size == tftp_blksize)
			syslog(LOG_WARNING,
			    "Block number wrapped (hint: increase block size)");
		block++;
	} while (size == tftp_blksize || block == 1);
abort:
	(void) fclose(file);
}

static void
justquit(int dummy)
{

	exit(0);
}

/*
 * Receive a file.
 */
static void
recvfile(struct formats *pf, volatile int etftp, volatile int acklength)
{
	volatile unsigned int block;
	struct tftphdr	*dp;
	struct tftphdr	*ap;    /* ack buffer */
	volatile int size;
	int n;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = (struct tftphdr *)oackbuf;
	block = 0;
	do {
		timeout = 0;
		if (etftp == 0) {
			ap = (struct tftphdr *)ackbuf;
			ap->th_opcode = htons((u_short)ACK);
			ap->th_block = htons((u_short)block);
			acklength = 4;
		}
		if (debug)
			syslog(LOG_DEBUG, "Sending ACK for block %u\n", block);
		if (block == UINT16_MAX)
			syslog(LOG_WARNING,
			    "Block number wrapped (hint: increase block size)");
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		ap = (struct tftphdr *) (etftp ? oackbuf : ackbuf);
		if (send(peer, ap, acklength, 0) != acklength) {
			syslog(LOG_ERR, "tftpd: write: %m");
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, tftp_blksize + 4, 0);
			alarm(0);
			if (n < 0) {            /* really? */
				syslog(LOG_ERR, "tftpd: read: %m");
				goto abort;
			}
			etftp = 0;
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (debug)
				syslog(LOG_DEBUG, "Received %s for block %u",
				    opcode(dp->th_opcode),
				    (unsigned int)dp->th_block);

			switch (dp->th_opcode) {
			case ERROR:
				goto abort;
			case DATA:
				if (dp->th_block == block)
					goto done;   /* normal */
				if (debug)
					syslog(LOG_DEBUG, "Resync %u != %u",
					    (unsigned int)dp->th_block, block);
				/* Re-synchronize with the other side */
				(void) synchnet(peer, tftp_blksize);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
				break;
			default:
				syslog(LOG_INFO, "Received %s in recvfile\n",
				    opcode(dp->th_opcode));
				break;
			}
		}
done:
		if (debug)
			syslog(LOG_DEBUG, "Got block %u", block);
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == tftp_blksize);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	if (debug)
		syslog(LOG_DEBUG, "Send final ACK %u", block);
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
	int		 e_code;
	const char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ EOPTNEG,	"Option negotiation failed" },
	{ -1,		0 }
};

static const char *
errtomsg(int error)
{
	static char ebuf[20];
	const struct errmsg *pe;

	if (error == 0)
		return ("success");
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			return (pe->e_msg);
	snprintf(ebuf, sizeof(ebuf), "error %d", error);
	return (ebuf);
}

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
static void
nak(int error)
{
	const struct errmsg *pe;
	struct tftphdr *tp;
	int	length;
	size_t	msglen;

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	msglen = sizeof(buf) - (&tp->th_msg[0] - buf);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
		strlcpy(tp->th_msg, strerror(error - 100), msglen);
	} else {
		tp->th_code = htons((u_short)error);
		strlcpy(tp->th_msg, pe->e_msg, msglen);
	}
	if (debug)
		syslog(LOG_DEBUG, "Send NACK %s", tp->th_msg);
	length = strlen(tp->th_msg);
	msglen = &tp->th_msg[length + 1] - buf;
	if (send(peer, buf, msglen, 0) != (ssize_t)msglen)
		syslog(LOG_ERR, "nak: %m");
}

static char *
verifyhost(struct sockaddr *fromp)
{
	static char hbuf[MAXHOSTNAMELEN];

	if (getnameinfo(fromp, fromp->sa_len, hbuf, sizeof(hbuf), NULL, 0, 0))
		strlcpy(hbuf, "?", sizeof(hbuf));
	return (hbuf);
}
