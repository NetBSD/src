/*	$NetBSD: rwhod.c,v 1.37.6.1 2009/05/13 19:20:38 jym Exp $	*/

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
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rwhod.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: rwhod.c,v 1.37.6.1 2009/05/13 19:20:38 jym Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <protocols/rwhod.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <paths.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>

#include "utmpentry.h"

#define CHECK_INTERVAL (3 * 60)

/* Time interval limit; ruptime will think that we are down > than this */
#define MAX_INTERVAL (11 * 60)


static char	myname[MAXHOSTNAMELEN + 1];

/*
 * We communicate with each neighbor in a list constructed at the time we're
 * started up.  Neighbors are currently directly connected via a hardware
 * interface.
 */
struct neighbor {
	struct	neighbor *n_next;
	char	*n_name;		/* interface name */
	struct	sockaddr *n_addr;	/* who to send to */
	int	n_addrlen;		/* size of address */
	int	n_flags;		/* should forward?, interface flags */
};

static struct	neighbor *neighbors;
static struct	whod mywd;
static struct	servent *sp;
static volatile sig_atomic_t  onsighup;

#define	WHDRSIZE	(sizeof(mywd) - sizeof(mywd.wd_we))

static int	 configure(int);
static void	 getboottime(void);
static void	 send_host_information(int);
static void	 sighup(int);
static void	 handleread(int);
static void	 quit(const char *);
static void	 rt_xaddrs(void *, void *, struct rt_addrinfo *);
static int	 drop_privs(char *);
static void	 usage(void) __dead;
static int	 verify(const char *);
#ifdef DEBUG
static char	*interval(int, const char *);
static ssize_t	 Sendto(int, const void *, size_t, int,
    const struct sockaddr *, socklen_t);
#else
#define	 Sendto sendto
#endif

int
main(int argc, char *argv[])
{
	int s, ch;
	int time_interval = 180;	/* Default time (180 seconds) */
	char *cp, *ep;
	socklen_t on = 1;
	struct sockaddr_in sasin;
	struct pollfd pfd[1];
	struct timeval delta, next, now;
	char *newuser = NULL;

	setprogname(argv[0]);

	if (getuid())
		errx(EXIT_FAILURE, "not super user");

	while ((ch = getopt(argc, argv, "i:u:")) != -1) {
		switch (ch) {
		case 'i':
			time_interval = (int)strtol(optarg, &ep, 10);

			switch (*ep) {
			case '\0':
				break;
			case 'm':
			case 'M':
				/* Time in minutes. */
				time_interval *= 60;
				if (ep[1] == '\0')
					break;
				/*FALLTHROUGH*/
			default:
				errx(1, "Invalid argument: `%s'", optarg);
			}

			if (time_interval <= 0)
				errx(1, "Interval must be greater than 0");

			if (time_interval > MAX_INTERVAL)
				errx(1, "Interval cannot be greater than"
				    " %d minutes", MAX_INTERVAL / 60);
			break;

		case 'u':
			newuser = optarg;
			break;
			
		default:
			usage();	
		}
	}

	sp = getservbyname("who", "udp");
	if (sp == NULL)
		errx(EXIT_FAILURE, "udp/who: unknown service");
#ifndef DEBUG
	(void)daemon(1, 0);
	(void)pidfile(NULL);
#endif
	if (chdir(_PATH_RWHODIR) < 0)
		err(EXIT_FAILURE, "%s", _PATH_RWHODIR);
	(void)signal(SIGHUP, sighup);
	openlog(getprogname(), LOG_PID, LOG_DAEMON);
	/*
	 * Establish host name as returned by system.
	 */
	if (gethostname(myname, sizeof(myname) - 1) < 0) {
		syslog(LOG_ERR, "gethostname: %m");
		exit(EXIT_FAILURE);
	}
	myname[sizeof(myname) - 1] = '\0';
	if ((cp = strchr(myname, '.')) != NULL)
		*cp = '\0';
	(void)strncpy(mywd.wd_hostname, myname, sizeof(mywd.wd_hostname) - 1);
	getboottime();
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
		syslog(LOG_ERR, "setsockopt SO_BROADCAST: %m");
		exit(EXIT_FAILURE);
	}
	(void)memset(&sasin, 0, sizeof(sasin));
	sasin.sin_family = AF_INET;
	sasin.sin_port = sp->s_port;
	if (bind(s, (struct sockaddr *)&sasin, sizeof(sasin)) < 0) {
		syslog(LOG_ERR, "bind: %m");
		exit(EXIT_FAILURE);
	}
	if (!configure(s))
		exit(EXIT_FAILURE);

	if (newuser)
		if (!drop_privs(newuser))
			exit(EXIT_FAILURE);

	send_host_information(s);
	delta.tv_sec = time_interval;
	delta.tv_usec = 0;
	gettimeofday(&now, NULL);
	timeradd(&now, &delta, &next);

	pfd[0].fd = s;
	pfd[0].events = POLLIN;

	for (;;) {
		int n;

		n = poll(pfd, 1, 1000);

		if (onsighup) {
			onsighup = 0;
			getboottime();
		}

		if (n == 1)
			handleread(s);

		(void)gettimeofday(&now, NULL);
		if (timercmp(&now, &next, >)) {
			send_host_information(s);
			timeradd(&now, &delta, &next);
		}
	}

	/* NOTREACHED */
	return 0;
}

static void
sighup(int signo __unused)
{
	onsighup = 1;
}

static void
handleread(int s)
{
	struct sockaddr_in from;
	struct stat st;
	char path[64];
	struct whod wd;
	int cc, whod;
	socklen_t len = sizeof(from);

	cc = recvfrom(s, (char *)&wd, sizeof(struct whod), 0,
		(struct sockaddr *)&from, &len);
	if (cc <= 0) {
		if (cc < 0 && errno != EINTR)
			syslog(LOG_WARNING, "recv: %m");
		return;
	}
	if (from.sin_port != sp->s_port) {
		syslog(LOG_WARNING, "%d: bad from port",
			ntohs(from.sin_port));
		return;
	}
	if (cc < (int)WHDRSIZE) {
		syslog(LOG_WARNING, "Short packet from %s",
			inet_ntoa(from.sin_addr));
		return;
	}

	if (wd.wd_vers != WHODVERSION)
		return;
	if (wd.wd_type != WHODTYPE_STATUS)
		return;
	/*
	 * Ensure null termination of the name within the packet.
	 * Otherwise we might overflow or read past the end.
	 */
	wd.wd_hostname[sizeof(wd.wd_hostname)-1] = 0;
	if (!verify(wd.wd_hostname)) {
		syslog(LOG_WARNING, "malformed host name from %s",
		    inet_ntoa(from.sin_addr));
		return;
	}
	(void)snprintf(path, sizeof(path), "whod.%s", wd.wd_hostname);
	/*
	 * Rather than truncating and growing the file each time,
	 * use ftruncate if size is less than previous size.
	 */
	whod = open(path, O_WRONLY | O_CREAT, 0644);
	if (whod < 0) {
		syslog(LOG_WARNING, "%s: %m", path);
		return;
	}
#if ENDIAN != BIG_ENDIAN
	{
		int i, n = (cc - WHDRSIZE) / sizeof(struct whoent);
		struct whoent *we;

		/* undo header byte swapping before writing to file */
		wd.wd_sendtime = ntohl(wd.wd_sendtime);
		for (i = 0; i < 3; i++)
			wd.wd_loadav[i] = ntohl(wd.wd_loadav[i]);
		wd.wd_boottime = ntohl(wd.wd_boottime);
		we = wd.wd_we;
		for (i = 0; i < n; i++) {
			we->we_idle = ntohl(we->we_idle);
			we->we_utmp.out_time =
			    ntohl(we->we_utmp.out_time);
			we++;
		}
	}
#endif
	wd.wd_recvtime = time(NULL);
	(void)write(whod, (char *)&wd, cc);
	if (fstat(whod, &st) < 0 || st.st_size > cc)
		(void)ftruncate(whod, cc);
	(void)close(whod);
}

/*
 * Check out host name for unprintables
 * and other funnies before allowing a file
 * to be created.  Sorry, but blanks aren't allowed.
 */
static int
verify(const char *name)
{
	int size = 0;

	while (*name) {
		if (!isascii((unsigned char)*name) ||
		    !(isalnum((unsigned char)*name) ||
		    ispunct((unsigned char)*name)))
			return 0;
		name++, size++;
	}
	return size > 0;
}

static void
send_host_information(int s)
{
	struct neighbor *np;
	struct whoent *we = mywd.wd_we, *wlast;
	int i, cc, utmpent = 0;
	struct stat stb;
	double avenrun[3];
	time_t now;
	static struct utmpentry *ohead = NULL;
	struct utmpentry *ep;
	static int count = 0;

	now = time(NULL);
	if (count % 10 == 0)
		getboottime();
	count++;

	(void)getutentries(NULL, &ep);
	/* XXX probably should expose utmp mtime, check that instead */
	if (ep != ohead) {
		wlast = &mywd.wd_we[1024 / sizeof(struct whoent) - 1];
		for (; ep; ep = ep->next) {
			(void)strncpy(we->we_utmp.out_line, ep->line,
			    sizeof(we->we_utmp.out_line));
			(void)strncpy(we->we_utmp.out_name, ep->name,
			    sizeof(we->we_utmp.out_name));
			we->we_utmp.out_time = htonl(ep->tv.tv_sec);
			if (we >= wlast)
				break;
			we++;
		}
		utmpent = we - mywd.wd_we;
	}

	/*
	 * The test on utmpent looks silly---after all, if no one is
	 * logged on, why worry about efficiency?---but is useful on
	 * (e.g.) compute servers.
	 */
	if (utmpent && chdir(_PATH_DEV)) {
		syslog(LOG_ERR, "chdir(%s): %m", _PATH_DEV);
		exit(EXIT_FAILURE);
	}
	we = mywd.wd_we;
	for (i = 0; i < utmpent; i++) {
		if (stat(we->we_utmp.out_line, &stb) >= 0)
			we->we_idle = htonl(now - stb.st_atime);
		we++;
	}
	(void)getloadavg(avenrun, sizeof(avenrun)/sizeof(avenrun[0]));
	for (i = 0; i < 3; i++)
		mywd.wd_loadav[i] = htonl((u_long)(avenrun[i] * 100));
	cc = (char *)we - (char *)&mywd;
	mywd.wd_sendtime = htonl(time(0));
	mywd.wd_vers = WHODVERSION;
	mywd.wd_type = WHODTYPE_STATUS;
	for (np = neighbors; np != NULL; np = np->n_next)
		(void)Sendto(s, (char *)&mywd, cc, 0,
				np->n_addr, np->n_addrlen);
	if (utmpent && chdir(_PATH_RWHODIR)) {
		syslog(LOG_ERR, "chdir(%s): %m", _PATH_RWHODIR);
		exit(EXIT_FAILURE);
	}
}

static void
getboottime(void)
{
	int mib[2];
	size_t size;
	struct timeval tm;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof(tm);
	if (sysctl(mib, 2, &tm, &size, NULL, 0) == -1) {
		syslog(LOG_ERR, "cannot get boottime: %m");
		exit(EXIT_FAILURE);
	}
	mywd.wd_boottime = htonl(tm.tv_sec);
}

static void
quit(const char *msg)
{
	syslog(LOG_ERR, "%s", msg);
	exit(EXIT_FAILURE);
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) ((char *)(x) + ROUNDUP((n)->sa_len))

static void
rt_xaddrs(void *cp, void *cplim, struct rt_addrinfo *rtinfo)
{
	struct sockaddr *sa;
	int i;

	(void)memset(rtinfo->rti_info, 0, sizeof(rtinfo->rti_info));
	for (i = 0; (i < RTAX_MAX) && (cp < cplim); i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (struct sockaddr *)cp;
		cp = ADVANCE(cp, sa);
	}
}

/*
 * Figure out device configuration and select
 * networks which deserve status information.
 */
static int
configure(int s)
{
	struct neighbor *np;
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
	struct sockaddr_dl *sdl;
	size_t needed;
	int mib[6], flags = 0, len;
	char *buf, *lim, *next;
	struct rt_addrinfo info;
	struct sockaddr_in dstaddr;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET;
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		quit("route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		quit("malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		quit("actual retrieval of interface table");
	lim = buf + needed;

	sdl = NULL;		/* XXX just to keep gcc -Wall happy */
	for (next = buf; next < lim; next += ifm->ifm_msglen) {
		ifm = (struct if_msghdr *)next;
		if (ifm->ifm_type == RTM_IFINFO) {
			sdl = (struct sockaddr_dl *)(ifm + 1);
			flags = ifm->ifm_flags;
			continue;
		}
		if ((flags & IFF_UP) == 0 ||
		    (flags & (IFF_BROADCAST|IFF_POINTOPOINT)) == 0)
			continue;
		if (ifm->ifm_type != RTM_NEWADDR)
			quit("out of sync parsing NET_RT_IFLIST");
		ifam = (struct ifa_msghdr *)ifm;
		info.rti_addrs = ifam->ifam_addrs;
		rt_xaddrs((ifam + 1), ifam->ifam_msglen + (char *)ifam, &info);
		/* gag, wish we could get rid of Internet dependencies */
		if (info.rti_info[RTAX_BRD] == NULL ||
		    info.rti_info[RTAX_BRD]->sa_family != AF_INET)
			continue;
		(void)memcpy(&dstaddr, info.rti_info[RTAX_BRD],
		    sizeof(dstaddr));
#define IPADDR_SA(x) ((struct sockaddr_in *)(x))->sin_addr.s_addr
#define PORT_SA(x) ((struct sockaddr_in *)(x))->sin_port
		PORT_SA(&dstaddr) = sp->s_port;
		for (np = neighbors; np != NULL; np = np->n_next)
			if (memcmp(sdl->sdl_data, np->n_name,
				   sdl->sdl_nlen) == 0 &&
			    IPADDR_SA(np->n_addr) == IPADDR_SA(&dstaddr))
				break;
		if (np != NULL)
			continue;
		len = sizeof(*np) + dstaddr.sin_len + sdl->sdl_nlen + 1;
		np = (struct neighbor *)malloc(len);
		if (np == NULL)
			quit("malloc of neighbor structure");
		(void)memset(np, 0, len);
		np->n_flags = flags;
		np->n_addr = (struct sockaddr *)(np + 1);
		np->n_addrlen = dstaddr.sin_len;
		np->n_name = np->n_addrlen + (char *)np->n_addr;
		np->n_next = neighbors;
		neighbors = np;
		(void)memcpy(np->n_addr, &dstaddr, np->n_addrlen);
		(void)memcpy(np->n_name, sdl->sdl_data, sdl->sdl_nlen);
	}
	free(buf);
	return (1);
}

#ifdef DEBUG
static ssize_t
Sendto(int s, const void *buf, size_t cc, int flags, const struct sockaddr *to,
    socklen_t tolen)
{
	struct whod *w = (struct whod *)buf;
	struct whoent *we;
	struct sockaddr_in *sasin = (struct sockaddr_in *)to;
	ssize_t ret;

	ret = sendto(s, buf, cc, flags, to, tolen);

	printf("sendto %s.%d\n", inet_ntoa(sasin->sin_addr),
	    ntohs(sasin->sin_port));
	printf("hostname %s %s\n", w->wd_hostname,
	   interval(ntohl(w->wd_sendtime) - ntohl(w->wd_boottime), "  up"));
	printf("load %4.2f, %4.2f, %4.2f\n",
	    ntohl(w->wd_loadav[0]) / 100.0, ntohl(w->wd_loadav[1]) / 100.0,
	    ntohl(w->wd_loadav[2]) / 100.0);
	cc -= WHDRSIZE;
	for (we = w->wd_we, cc /= sizeof(struct whoent); cc > 0; cc--, we++) {
		time_t t = ntohl(we->we_utmp.out_time);
		printf("%-8.8s %s:%s %.12s", we->we_utmp.out_name,
		    w->wd_hostname, we->we_utmp.out_line, ctime(&t)+4);
		we->we_idle = ntohl(we->we_idle) / 60;
		if (we->we_idle) {
			if (we->we_idle >= 100*60)
				we->we_idle = 100*60 - 1;
			if (we->we_idle >= 60)
				printf(" %2d", we->we_idle / 60);
			else
				printf("   ");
			printf(":%02d", we->we_idle % 60);
		}
		printf("\n");
	}
	return ret;
}

static char *
interval(int time, const char *updown)
{
	static char resbuf[32];
	int days, hours, minutes;

	if (time < 0 || time > 3*30*24*60*60) {
		(void)snprintf(resbuf, sizeof(resbuf), "   %s ??:??", updown);
		return (resbuf);
	}
	minutes = (time + 59) / 60;		/* round to minutes */
	hours = minutes / 60; minutes %= 60;
	days = hours / 24; hours %= 24;
	if (days)
		(void)snprintf(resbuf, sizeof(resbuf), "%s %2d+%02d:%02d",
		    updown, days, hours, minutes);
	else
		(void)snprintf(resbuf, sizeof(resbuf), "%s    %2d:%02d",
		    updown, hours, minutes);
	return resbuf;
}
#endif

static int
drop_privs(char *newuser)
{
	struct passwd *pw;
	gid_t gidset[1];
	
	pw = getpwnam(newuser);
	if (pw == NULL) {
		syslog(LOG_ERR, "no user %.100s", newuser);
		return 0;
	}

	endpwent();	

	gidset[0] = pw->pw_gid;
	if (setgroups(1, gidset) == -1) {
		syslog(LOG_ERR, "setgroups: %m");
		return 0;
	}

	if (setgid(pw->pw_gid) == -1) {
		syslog(LOG_ERR, "setgid: %m");
		return 0;
	}

	if (setuid(pw->pw_uid) == -1) {
		syslog(LOG_ERR, "setuid: %m");
		return 0;
	}

	return 1;
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-i interval] [-u user]\n", getprogname());
	exit(EXIT_FAILURE);
}
