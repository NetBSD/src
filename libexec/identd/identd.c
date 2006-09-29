/* $NetBSD: identd.c,v 1.30 2006/09/29 15:49:29 christos Exp $ */

/*
 * identd.c - TCP/IP Ident protocol server.
 *
 * This software is in the public domain.
 * Written by Peter Postma <peter@NetBSD.org>
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: identd.c,v 1.30 2006/09/29 15:49:29 christos Exp $");

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <netdb.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "identd.h"

#define	OPSYS_NAME	"UNIX"
#define	IDENT_SERVICE	"auth"
#define	TIMEOUT		30	/* seconds */

static int   idhandle(int, const char *, const char *, const char *,
		const char *, struct sockaddr *, int);
static void  idparse(int, int, int, const char *, const char *, const char *);
static void  iderror(int, int, int, const char *);
static const char *gethost(struct sockaddr *);
static int  *socketsetup(const char *, const char *, int);
static int   ident_getuid(struct sockaddr_storage *, socklen_t,
		struct sockaddr *, uid_t *);
static int   sysctl_getuid(struct sockaddr_storage *, socklen_t, uid_t *);
static int   sysctl_proxy_getuid(struct sockaddr_storage *,
		struct sockaddr *, uid_t *);
static int   forward(int, struct sockaddr *, int, int, int);
static int   check_noident(const char *);
static int   check_userident(const char *, char *, size_t);
static void  random_string(char *, size_t);
static int   change_format(const char *, struct passwd *, char *, size_t);
static void  timeout_handler(int);
static void  fatal(const char *);
static void  die(const char *, ...);

static int   bflag, eflag, fflag, iflag, Iflag;
static int   lflag, Lflag, nflag, Nflag, rflag;

/* NAT lookup function pointer. */
static int  (*nat_lookup)(struct sockaddr_storage *, struct sockaddr *, int *);

/* Packet filters. */
static const struct {
	const char *name;
	int (*fn)(struct sockaddr_storage *, struct sockaddr *, int *);
} filters[] = {
#ifdef WITH_PF
	{ "pf", pf_natlookup },
#endif
#ifdef WITH_IPF
	{ "ipfilter", ipf_natlookup },
#endif
	{ NULL, NULL }
};

int
main(int argc, char *argv[])
{
	int IPv4or6, ch, error, i, *socks, timeout;
	const char *filter, *osname, *portno, *proxy;
	char *address, *charset, *fmt, *p;
	char user[LOGIN_NAME_MAX];
	struct addrinfo *ai, hints;
	struct sockaddr *proxy_addr;
	struct group *grp;
	struct passwd *pw;
	gid_t gid;
	uid_t uid;

	socks = NULL;
	IPv4or6 = AF_UNSPEC;
	osname = OPSYS_NAME;
	portno = IDENT_SERVICE;
	timeout = TIMEOUT;
	nat_lookup = NULL;
	proxy_addr = NULL;
	filter = proxy = NULL;
	address = charset = fmt = NULL;
	uid = gid = 0;
	bflag = eflag = fflag = iflag = Iflag = 0;
	lflag = Lflag = nflag = Nflag = rflag = 0;

	/* Started from a tty? then run as daemon. */
	if (isatty(STDIN_FILENO))
		bflag = 1;

	/* Parse command line arguments. */
	while ((ch = getopt(argc, argv,
	    "46a:bceF:f:g:IiL:lm:Nno:P:p:rt:u:")) != -1) {
		switch (ch) {
		case '4':
			IPv4or6 = AF_INET;
			break;
		case '6':
			IPv4or6 = AF_INET6;
			break;
		case 'a':
			address = optarg;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			charset = optarg;
			break;
		case 'e':
			eflag = 1;
			break;
		case 'F':
			fmt = optarg;
			break;
		case 'f':
			fflag = 1;
			(void)strlcpy(user, optarg, sizeof(user));
			break;
		case 'g':
			gid = (gid_t)strtol(optarg, &p, 0);
			if (*p != '\0') {
				if ((grp = getgrnam(optarg)) != NULL)
					gid = grp->gr_gid;
				else
					die("No such group `%s'", optarg);
			}
			break;
		case 'I':
			Iflag = 1;
			/* FALLTHROUGH */
		case 'i':
			iflag = 1;
			break;
		case 'L':
			Lflag = 1;
			(void)strlcpy(user, optarg, sizeof(user));
			break;
		case 'l':
			if (!lflag)
				openlog("identd", LOG_PID, LOG_DAEMON);
			lflag = 1;
			break;
		case 'm':
			filter = optarg;
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'o':
			osname = optarg;
			break;
		case 'P':
			proxy = optarg;
			break;
		case 'p':
			portno = optarg;
			break;
		case 'r':
			rflag = 1;
			break;
		case 't':
			timeout = (int)strtol(optarg, &p, 0);
			if (*p != '\0' || timeout < 1) 
				die("Invalid timeout value `%s'", optarg);
			break;
		case 'u':
			uid = (uid_t)strtol(optarg, &p, 0);
			if (*p != '\0') {
				if ((pw = getpwnam(optarg)) != NULL) {
					uid = pw->pw_uid;
					gid = pw->pw_gid;
				} else
					die("No such user `%s'", optarg);
			}
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	/* Verify proxy address, if enabled. */
	if (proxy != NULL) {
		(void)memset(&hints, 0, sizeof(hints));
		hints.ai_family = IPv4or6;
		hints.ai_socktype = SOCK_STREAM;
		error = getaddrinfo(proxy, NULL, &hints, &ai);
		if (error != 0)
			die("Bad proxy `%s': %s", proxy, gai_strerror(error));
		if (ai->ai_next != NULL)
			die("Bad proxy `%s': resolves to multiple addresses",
			    proxy);
		proxy_addr = ai->ai_addr;
	}

	/* Verify filter, if enabled. */
	if (filter != NULL) {
		for (i = 0; filters[i].name != NULL; i++) {
			if (strcasecmp(filter, filters[i].name) == 0) {
				nat_lookup = filters[i].fn;
				break;
			}
		}
		if (nat_lookup == NULL)
			die("Packet filter `%s' is not supported", filter);
	}

	/* Setup sockets when running in the background. */
	if (bflag)
		socks = socketsetup(address, portno, IPv4or6);

	/* Switch to another uid/gid? */
	if (gid && setgid(gid) == -1)
		die("Failed to set GID to `%d': %s", gid, strerror(errno));
	if (uid && setuid(uid) == -1)
		die("Failed to set UID to `%d': %s", uid, strerror(errno));

	/*
	 * When running as daemon: daemonize, setup pollfds and go into
	 * the mainloop.  Otherwise, just read the input from stdin and
	 * let inetd handle the sockets.
	 */
	if (bflag) {
		int fd, nfds, rv;
		struct pollfd *rfds;

		if (daemon(0, 0) < 0)
			die("daemon: %s", strerror(errno));

		rfds = malloc(*socks * sizeof(struct pollfd));
		if (rfds == NULL)
			fatal("malloc");
		nfds = *socks;
		for (i = 0; i < nfds; i++) {
			rfds[i].fd = socks[i+1];
			rfds[i].events = POLLIN;
			rfds[i].revents = 0;
		}
		/* Mainloop for daemon. */
		for (;;) {
			rv = poll(rfds, nfds, INFTIM);
			if (rv < 0) {
				if (errno == EINTR)
					continue;
				fatal("poll");
			}
			for (i = 0; i < nfds; i++) {
				if (rfds[i].revents & POLLIN) {
					fd = accept(rfds[i].fd, NULL, NULL);
					if (fd < 0) {
						maybe_syslog(LOG_ERR,
						    "accept: %m");
						continue;
					}
					switch (fork()) {
					case -1:	/* error */
						maybe_syslog(LOG_ERR,
						    "fork: %m");
						(void)sleep(1);
						break;
					case 0:		/* child */
						(void)idhandle(fd, charset,
						    fmt, osname, user,
						    proxy_addr, timeout);
						_exit(EXIT_SUCCESS);
					default:	/* parent */
						(void)signal(SIGCHLD, SIG_IGN);
						(void)close(fd);
					}
				}
			}
		}
	} else
		(void)idhandle(STDIN_FILENO, charset, fmt, osname, user,
		    proxy_addr, timeout);

	return 0;
}

/*
 * Handle a request on the ident port.  Returns 0 on success or 1 on
 * failure.  The return values are currently ignored.
 */
static int
idhandle(int fd, const char *charset, const char *fmt, const char *osname,
    const char *user, struct sockaddr *proxy, int timeout)
{
	struct sockaddr_storage ss[2];
	char userbuf[LOGIN_NAME_MAX];	/* actual user name (or numeric uid) */
	char idbuf[LOGIN_NAME_MAX];	/* name to be used in response */
	char buf[BUFSIZ], *p;
	struct passwd *pw;
	int lport, fport;
	socklen_t len;
	uid_t uid;
	ssize_t n;
	ssize_t qlen;

	lport = fport = 0;

	(void)strlcpy(idbuf, user, sizeof(idbuf));
	(void)signal(SIGALRM, timeout_handler);
	(void)alarm(timeout);

	/* Get foreign internet address. */
	len = sizeof(ss[0]);
	if (getpeername(fd, (struct sockaddr *)&ss[0], &len) < 0)
		fatal("getpeername");

	maybe_syslog(LOG_INFO, "Connection from %s",
	    gethost((struct sockaddr *)&ss[0]));

	/* Get local internet address. */
	len = sizeof(ss[1]);
	if (getsockname(fd, (struct sockaddr *)&ss[1], &len) < 0)
		fatal("getsockname");

	/* Be sure to have the same address families. */
	if (ss[0].ss_family != ss[1].ss_family) {
		maybe_syslog(LOG_ERR, "Different foreign/local address family");
		return 1;
	}

	/* Receive data from the client. */
	qlen = 0;
	for (;;) {
		if ((n = recv(fd, &buf[qlen], sizeof(buf) - qlen, 0)) < 0) {
			fatal("recv");
		} else if (n == 0) {
			maybe_syslog(LOG_NOTICE, "recv: EOF");
			iderror(fd, 0, 0, "UNKNOWN-ERROR");
			return 1;
		}
		/*
		 * 1413 is not clear on what to do if data follows the first
		 * CRLF before we respond.  We do not consider the query
		 * complete until we get a CRLF _at the end of the buffer_.
		 */
		qlen += n;
		if ((qlen >= 2) && (buf[qlen - 2] == '\r') &&
		    (buf[qlen - 1] == '\n'))
			break;
	}
	buf[qlen - 2] = '\0';

	/* Get local and remote ports from the received data. */
	p = buf;
	while (*p != '\0' && isspace((unsigned char)*p))
		p++;
	if ((p = strtok(p, " \t,")) != NULL) {
		lport = atoi(p);
		if ((p = strtok(NULL, " \t,")) != NULL)
			fport = atoi(p);
	}

	/* Are the ports valid? */
	if (lport < 1 || lport > 65535 || fport < 1 || fport > 65535) {
		maybe_syslog(LOG_NOTICE, "Invalid port(s): %d, %d from %s",
		    lport, fport, gethost((struct sockaddr *)&ss[0]));
		iderror(fd, 0, 0, eflag ? "UNKNOWN-ERROR" : "INVALID-PORT");
		return 1;
	}

	/* If there is a 'lie' user enabled, then handle it now and stop. */
	if (Lflag) {
		maybe_syslog(LOG_NOTICE, "Lying with name %s to %s",
		    idbuf, gethost((struct sockaddr *)&ss[0]));
		idparse(fd, lport, fport, charset, osname, idbuf);
		return 0;
	}

	/* Protocol dependent stuff. */
	switch (ss[0].ss_family) {
	case AF_INET:
		satosin(&ss[0])->sin_port = htons(fport);
		satosin(&ss[1])->sin_port = htons(lport);
		break;
	case AF_INET6:
		satosin6(&ss[0])->sin6_port = htons(fport);
		satosin6(&ss[1])->sin6_port = htons(lport);
		break;
	default:
		maybe_syslog(LOG_ERR, "Unsupported protocol (no. %d)",
		    ss[0].ss_family);
		return 1;
	}

	/* Try to get the UID of the connection owner using sysctl. */
	if (ident_getuid(ss, sizeof(ss), proxy, &uid) == -1) {
		/* Lookup failed, try to forward if enabled. */
		if (nat_lookup != NULL) {
			struct sockaddr nat_addr;
			int nat_lport;

			(void)memset(&nat_addr, 0, sizeof(nat_addr));

			if ((*nat_lookup)(ss, &nat_addr, &nat_lport) &&
			    forward(fd, &nat_addr, nat_lport, fport, lport)) {
				maybe_syslog(LOG_INFO,
				    "Succesfully forwarded the request to %s",
				    gethost(&nat_addr));
				return 0;
			}
		}
		/* Fall back to a default name? */
		if (fflag) {
			maybe_syslog(LOG_NOTICE, "Using fallback name %s to %s",
			    idbuf, gethost((struct sockaddr *)&ss[0]));
			idparse(fd, lport, fport, charset, osname, idbuf);
			return 0;
		}
		maybe_syslog(LOG_ERR, "Lookup failed, returning error to %s",
		    gethost((struct sockaddr *)&ss[0]));
		iderror(fd, lport, fport, eflag ? "UNKNOWN-ERROR" : "NO-USER");
		return 1;
	}

	/* Fill in userbuf with user name if possible, else numeric UID. */
	if ((pw = getpwuid(uid)) == NULL) {
		maybe_syslog(LOG_ERR, "Couldn't map uid (%u) to name", uid);
		(void)snprintf(userbuf, sizeof(userbuf), "%u", uid);
	} else {
		maybe_syslog(LOG_INFO, "Successful lookup: %d, %d: %s for %s",
		    lport, fport, pw->pw_name,
		    gethost((struct sockaddr *)&ss[0]));
		(void)strlcpy(userbuf, pw->pw_name, sizeof(userbuf));
	}

	/* No ident enabled? */
	if (Nflag && pw && check_noident(pw->pw_dir)) {
		maybe_syslog(LOG_NOTICE, "Returning HIDDEN-USER for user %s"
		    " to %s", pw->pw_name, gethost((struct sockaddr *)&ss[0]));
		iderror(fd, lport, fport, "HIDDEN-USER");
		return 1;
	}

	/* User ident enabled? */
	if (iflag && pw && check_userident(pw->pw_dir, idbuf, sizeof(idbuf))) {
		if (!Iflag) {
			if ((strspn(idbuf, "0123456789") &&
			     getpwuid(atoi(idbuf)) != NULL) ||
			    (getpwnam(idbuf) != NULL)) {
				maybe_syslog(LOG_NOTICE,
				    "Ignoring user-specified '%s' for user %s",
				    idbuf, userbuf);
				(void)strlcpy(idbuf, userbuf, sizeof(idbuf));
			}
		}
		maybe_syslog(LOG_NOTICE,
		    "Returning user-specified '%s' for user %s to %s",
		    idbuf, userbuf, gethost((struct sockaddr *)&ss[0]));
		idparse(fd, lport, fport, charset, osname, idbuf);
		return 0;
	}

	/* Send a random message? */
	if (rflag) {
		/* Random number or string? */
		if (nflag)
			(void)snprintf(idbuf, sizeof(idbuf), "%u",
			    (unsigned int)(arc4random() % 65535));
		else
			random_string(idbuf, sizeof(idbuf));

		maybe_syslog(LOG_NOTICE,
		    "Returning random '%s' for user %s to %s",
		    idbuf, userbuf, gethost((struct sockaddr *)&ss[0]));
		idparse(fd, lport, fport, charset, osname, idbuf);
		return 0;
	}

	/* Return numberic user ID? */
	if (nflag)
		(void)snprintf(idbuf, sizeof(idbuf), "%u", uid);
	else
		(void)strlcpy(idbuf, userbuf, sizeof(idbuf));

	/*
	 * Change the output format?  Note that 512 is the maximum
	 * size of the result according to RFC 1413.
	 */
	if (fmt && change_format(fmt, pw, buf, 512 + 1))
		idparse(fd, lport, fport, charset, osname, buf);
	else
		idparse(fd, lport, fport, charset, osname, idbuf);

	return 0;
}

/* Send/parse the ident result. */
static void
idparse(int fd, int lport, int fport, const char *charset, const char *osname,
    const char *user)
{
	char *p;

	if (asprintf(&p, "%d,%d:USERID:%s%s%s:%s\r\n", lport, fport,
	    osname, charset ? "," : "", charset ? charset : "", user) < 0)
		fatal("asprintf");
	if (send(fd, p, strlen(p), 0) < 0) {
		free(p);
		fatal("send");
	}
	free(p);
}

/* Return a specified ident error. */
static void
iderror(int fd, int lport, int fport, const char *error)
{
	char *p;

	if (asprintf(&p, "%d,%d:ERROR:%s\r\n", lport, fport, error) < 0)
		fatal("asprintf");
	if (send(fd, p, strlen(p), 0) < 0) {
		free(p);
		fatal("send");
	}
	free(p);
}

/* Return the IP address of the connecting host. */
static const char *
gethost(struct sockaddr *sa)
{
	static char host[NI_MAXHOST];

	if (getnameinfo(sa, sa->sa_len, host, sizeof(host),
	    NULL, 0, NI_NUMERICHOST) == 0)
		return host;

	return "UNKNOWN";
}

/* Setup sockets, for daemon mode. */
static int *
socketsetup(const char *address, const char *port, int af)
{
	struct addrinfo hints, *res, *res0;
	int error, maxs, *s, *socks;
	const char *cause = NULL;
	socklen_t y = 1;

	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(address, port, &hints, &res0);
	if (error) {
		die("getaddrinfo: %s", gai_strerror(error));
		/* NOTREACHED */
	}

	/* Count max number of sockets we may open. */
	for (maxs = 0, res = res0; res != NULL; res = res->ai_next)
		maxs++;

	socks = malloc((maxs + 1) * sizeof(int));
	if (socks == NULL) {
		die("malloc: %s", strerror(errno));
		/* NOTREACHED */
	}

	*socks = 0;
	s = socks + 1;
	for (res = res0; res != NULL; res = res->ai_next) {
		*s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (*s < 0) {
			cause = "socket";
			continue;
		}
		(void)setsockopt(*s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y));
		if (bind(*s, res->ai_addr, res->ai_addrlen) < 0) {
			cause = "bind";
			(void)close(*s);
			continue;
		}
		if (listen(*s, 5) < 0) {
			cause = "listen";
			(void)close(*s);
			continue;
		}
		*socks = *socks + 1;
		s++;
	}

	if (*socks == 0) {
		free(socks);
		die("%s: %s", cause, strerror(errno));
		/* NOTREACHED */
	}
	if (res0)
		freeaddrinfo(res0);

	return socks;
}

/* UID lookup wrapper. */
static int
ident_getuid(struct sockaddr_storage *ss, socklen_t len,
    struct sockaddr *proxy, uid_t *uid)
{
	int rc;

	rc = sysctl_getuid(ss, len, uid);
	if (rc == -1 && proxy != NULL)
		rc = sysctl_proxy_getuid(ss, proxy, uid);

	return rc;
}

/* Try to get the UID of the connection owner using sysctl. */
static int
sysctl_getuid(struct sockaddr_storage *ss, socklen_t len, uid_t *uid)
{
	int mib[4];
	uid_t myuid;
	size_t uidlen;

	uidlen = sizeof(myuid);

	mib[0] = CTL_NET;
	mib[1] = ss->ss_family;
	mib[2] = IPPROTO_TCP;
	mib[3] = TCPCTL_IDENT;

	if (sysctl(mib, sizeof(mib)/ sizeof(int), &myuid, &uidlen, ss, len) < 0)
		return -1;
	*uid = myuid;

	return 0;
}

/* Try to get the UID of the connection owner using sysctl (proxy version). */
static int
sysctl_proxy_getuid(struct sockaddr_storage *ss, struct sockaddr *proxy,
    uid_t *uid)
{
	struct sockaddr_storage new[2];
	int i, rc, name[CTL_MAXNAME];
	struct kinfo_pcb *kp;
	size_t sz, len;
	const char *list;

	rc = -1;
	sz = CTL_MAXNAME;
	list = NULL;

	/* Retrieve a list of sockets. */
	switch (ss[0].ss_family) {
	case AF_INET:
		/* We only accept queries from the proxy. */
		if (in_hosteq(satosin(&ss[0])->sin_addr,
		    satosin(proxy)->sin_addr))
			list = "net.inet.tcp.pcblist";
		break;
	case AF_INET6:
		/* We only accept queries from the proxy. */
		if (IN6_ARE_ADDR_EQUAL(&satosin6(&ss[0])->sin6_addr,
		    &satosin6(proxy)->sin6_addr))
			list = "net.inet6.tcp.pcblist";	
		break;
	default:
		maybe_syslog(LOG_ERR, "Unsupported protocol for proxy (no. %d)",
		    ss[0].ss_family);
	}
	if (list != NULL)
		rc = sysctlnametomib(list, &name[0], &sz);
	if (rc == -1)
		return -1;
	len = sz;

	name[len++] = PCB_ALL;
	name[len++] = 0;
	name[len++] = sizeof(struct kinfo_pcb);
	name[len++] = INT_MAX;

	kp = NULL;
	sz = 0;
	do {
		rc = sysctl(&name[0], len, kp, &sz, NULL, 0);
		if (rc == -1 && errno != ENOMEM)
			return -1;
		if (kp == NULL) {
			kp = malloc(sz);
			rc = -1;
		}
		if (kp == NULL)
			return -1;
	} while (rc == -1);

	rc = -1;
	/*
	 * Walk through the list of sockets and try to find a match.
	 * We don't know who has sent the query (we only know that the
	 * proxy has forwarded to us) so just try to match the ports and
	 * the local address.
	 */
	for (i = 0; i < sz / sizeof(struct kinfo_pcb); i++) {
		switch (ss[0].ss_family) {
		case AF_INET:
			/* Foreign and local ports must match. */
			if (satosin(&ss[0])->sin_port != 
			    satosin(&kp[i].ki_src)->sin_port)
				continue;
			if (satosin(&ss[1])->sin_port !=
			    satosin(&kp[i].ki_dst)->sin_port)
				continue;
			/* Foreign address may not match proxy address. */
			if (in_hosteq(satosin(proxy)->sin_addr,
			    satosin(&kp[i].ki_dst)->sin_addr))
				continue;
			/* Local addresses must match. */
			if (!in_hosteq(satosin(&ss[1])->sin_addr,
			    satosin(&kp[i].ki_src)->sin_addr))
				continue;
			break;
		case AF_INET6:
			/* Foreign and local ports must match. */
			if (satosin6(&ss[0])->sin6_port !=
			    satosin6(&kp[i].ki_src)->sin6_port)
				continue;
			if (satosin6(&ss[1])->sin6_port !=
			    satosin6(&kp[i].ki_dst)->sin6_port)
				continue;
			/* Foreign address may not match proxy address. */
			if (IN6_ARE_ADDR_EQUAL(&satosin6(proxy)->sin6_addr,
			    &satosin6(&kp[i].ki_dst)->sin6_addr))
				continue;
			/* Local addresses must match. */
			if (!IN6_ARE_ADDR_EQUAL(&satosin6(&ss[1])->sin6_addr,
			    &satosin6(&kp[i].ki_src)->sin6_addr))
				continue;
			break;
		}

		/*
		 * We have found the foreign address, copy it to a new
		 * struct and retrieve the UID of the connection owner.
		 */
		(void)memcpy(&new[0], &kp[i].ki_dst, kp[i].ki_dst.sa_len);
		(void)memcpy(&new[1], &kp[i].ki_src, kp[i].ki_src.sa_len);

		rc = sysctl_getuid(new, sizeof(new), uid);

		/* Done. */
		break;
	}

	free(kp);
	return rc;
}

/* Forward ident queries. Returns 1 when succesful, or zero if not. */
static int
forward(int fd, struct sockaddr *nat_addr, int nat_lport, int fport, int lport)
{
	char buf[BUFSIZ], reply[BUFSIZ], *p;
	ssize_t n;
	int sock;

	/* Connect to the NAT host. */
	sock = socket(nat_addr->sa_family, SOCK_STREAM, 0);
	if (sock < 0) {
		maybe_syslog(LOG_ERR, "socket: %m");
		return 0;
	}
	if (connect(sock, nat_addr, nat_addr->sa_len) < 0) {
		maybe_syslog(LOG_ERR, "Can't connect to %s: %m",
		    gethost(nat_addr));
		(void)close(sock);
		return 0;
	}

	/*
	 * Send the ident query to the NAT host, but use as local port
	 * the port of the NAT host.
	 */
	(void)snprintf(buf, sizeof(buf), "%d , %d\r\n", nat_lport, fport);
	if (send(sock, buf, strlen(buf), 0) < 0) {
		maybe_syslog(LOG_ERR, "send: %m");
		(void)close(sock);
		return 0;
	}

	/* Read the reply from the NAT host. */
	if ((n = recv(sock, reply, sizeof(reply) - 1, 0)) < 0) {
		maybe_syslog(LOG_ERR, "recv: %m");
		(void)close(sock);
		return 0;
	} else if (n == 0) {
		maybe_syslog(LOG_NOTICE, "recv: EOF");
		(void)close(sock);
		return 0;
	}
	reply[n] = '\0';
	(void)close(sock);

	/* Extract everything after the port specs from the ident reply. */
	for (p = reply; *p != '\0' && *p != ':'; p++)
		continue;
	if (*p == '\0' || *++p == '\0') {
		maybe_syslog(LOG_ERR, "Malformed ident reply from %s",
		    gethost(nat_addr));
		return 0;
	}
	/* Build reply for the requesting host, use the original local port. */
	(void)snprintf(buf, sizeof(buf), "%d,%d:%s", lport, fport, p);

	/* Send the reply from the NAT host back to the requesting host. */
	if (send(fd, buf, strlen(buf), 0) < 0) {
		maybe_syslog(LOG_ERR, "send: %m");
		return 0;
	}

	return 1;
}

/* Check if a .noident file exists in the user home directory. */
static int
check_noident(const char *homedir)
{
	struct stat sb;
	char *path;
	int ret;

	if (homedir == NULL)
		return 0;
	if (asprintf(&path, "%s/.noident", homedir) < 0)
		return 0;
	ret = stat(path, &sb);

	free(path);
	return (ret == 0);
}

/*
 * Check if a .ident file exists in the user home directory and
 * return the contents of that file. 
 */
static int
check_userident(const char *homedir, char *username, size_t len)
{
	struct stat sb;
	char *path, *p;
	ssize_t n;
	int fd;

	if (len == 0 || homedir == NULL)
		return 0;
	if (asprintf(&path, "%s/.ident", homedir) < 0)
		return 0;
	if ((fd = open(path, O_RDONLY|O_NONBLOCK|O_NOFOLLOW, 0)) < 0) {
		free(path);
		return 0;
	}
	if (fstat(fd, &sb) < 0 || !S_ISREG(sb.st_mode)) {
		(void)close(fd);
		free(path);
		return 0;
	}
	if ((n = read(fd, username, len - 1)) < 1) {
		(void)close(fd);
		free(path);
		return 0;
	}
	username[n] = '\0';

	if ((p = strpbrk(username, "\r\n")) != NULL)
		*p = '\0';

	(void)close(fd);
	free(path);
	return 1;
}

/* Generate a random string. */
static void
random_string(char *str, size_t len)
{
	static const char chars[] = "abcdefghijklmnopqrstuvwxyz1234567890";
	char *p;

	if (len == 0)
		return;
	for (p = str; len > 1; len--)
		*p++ = chars[arc4random() % (sizeof(chars) - 1)];
	*p = '\0';
}

/* Change the output format. */
static int
change_format(const char *format, struct passwd *pw, char *dest, size_t len)
{
	struct group *gr;
	const char *cp;
	char **gmp;
	size_t bp;

	if (len == 0 || ((gr = getgrgid(pw->pw_gid)) == NULL))
		return 0;

	for (bp = 0, cp = format; *cp != '\0' && bp < len - 1; cp++) {
		if (*cp != '%') {
			dest[bp++] = *cp;
			continue;
		}
		if (*++cp == '\0')
			break;
		switch (*cp) {
		case 'u':
			(void)snprintf(&dest[bp], len - bp, "%s", pw->pw_name);
			break;
		case 'U':
			(void)snprintf(&dest[bp], len - bp, "%d", pw->pw_uid);
			break;
		case 'g':
			(void)snprintf(&dest[bp], len - bp, "%s", gr->gr_name);
			break;
		case 'G':
			(void)snprintf(&dest[bp], len - bp, "%d", gr->gr_gid);
			break;
		case 'l':
			(void)snprintf(&dest[bp], len - bp, "%s", gr->gr_name);
			bp += strlen(&dest[bp]);
			if (bp >= len)
				break;
			setgrent();
			while ((gr = getgrent()) != NULL) {
				if (gr->gr_gid == pw->pw_gid)
					continue;
				for (gmp = gr->gr_mem; *gmp && **gmp; gmp++) {
					if (strcmp(*gmp, pw->pw_name) == 0) {
						(void)snprintf(&dest[bp],
						    len - bp, ",%s",
						    gr->gr_name);
						bp += strlen(&dest[bp]);
						break;
					}
				}
				if (bp >= len)
					break;
			}
			endgrent();
			break;
		case 'L':
			(void)snprintf(&dest[bp], len - bp, "%u", gr->gr_gid);
			bp += strlen(&dest[bp]);
			if (bp >= len)
				break;
			setgrent();
			while ((gr = getgrent()) != NULL) {
				if (gr->gr_gid == pw->pw_gid)
					continue;
				for (gmp = gr->gr_mem; *gmp && **gmp; gmp++) {
					if (strcmp(*gmp, pw->pw_name) == 0) {
						(void)snprintf(&dest[bp],
						    len - bp, ",%u",
						    gr->gr_gid);
						bp += strlen(&dest[bp]);
						break;
					}
				}
				if (bp >= len)
					break;
			}
			endgrent();
			break;
		default:
			dest[bp] = *cp;
			dest[bp+1] = '\0';
			break;
		}
		bp += strlen(&dest[bp]);
	}
	dest[bp] = '\0';

	return 1;
}

/* Just exit when we caught SIGALRM. */
static void
timeout_handler(int __unused s)
{
	maybe_syslog(LOG_INFO, "Timeout for request, closing connection...");
	exit(EXIT_FAILURE);
}

/* Report error message string through syslog and quit. */
static void
fatal(const char *func)
{
	maybe_syslog(LOG_ERR, "%s: %m", func);
	exit(EXIT_FAILURE);
}

/*
 * Report an error through syslog and/or stderr and quit.  Only used when
 * running identd in the background and when it isn't a daemon yet.
 */
static void
die(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	if (bflag)
		vwarnx(message, ap);
	if (lflag)
		vsyslog(LOG_ERR, message, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

/* Log using syslog, but only if enabled with the -l flag. */
void
maybe_syslog(int priority, const char *message, ...)
{
	va_list ap;

	if (lflag) {
		va_start(ap, message);
		vsyslog(priority, message, ap);
		va_end(ap);
	}
}
