/* $NetBSD: identd.c,v 1.23 2004/08/05 18:05:22 kim Exp $ */

/*
 * identd.c - TCP/IP Ident protocol server.
 *
 * This software is in the public domain.
 * Written by Peter Postma <peter@pointless.nl>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

__RCSID("$NetBSD: identd.c,v 1.23 2004/08/05 18:05:22 kim Exp $");

#define OPSYS_NAME      "UNIX"
#define IDENT_SERVICE   "auth"
#define TIMEOUT         30      /* seconds */

static int   idhandle(int, const char *, const char *, const char *,
		const char *, int);
static void  idparse(int, int, int, const char *, const char *, const char *);
static void  iderror(int, int, int, const char *);
static const char *gethost(struct sockaddr_storage *);
static int  *socketsetup(const char *, const char *, int);
static int   sysctl_getuid(struct sockaddr_storage *, socklen_t, uid_t *);
static int   check_noident(const char *);
static int   check_userident(const char *, char *, size_t);
static void  random_string(char *, size_t);
static void  change_format(const char *, struct passwd *, char *, size_t);
static void  timeout_handler(int);
static void  waitchild(int);
static void  fatal(const char *);

static int   bflag, eflag, fflag, Fflag, iflag, Iflag;
static int   lflag, Lflag, nflag, Nflag, rflag;

int
main(int argc, char *argv[])
{
	int IPv4or6, ch, *socks, timeout;
	char *address, *charset, *fmt;
	const char *osname, *portno;
	char *p;
	char user[LOGIN_NAME_MAX];
	struct group *grp;
	struct passwd *pw;
	gid_t gid;
	uid_t uid;

	IPv4or6 = AF_UNSPEC;
	osname = OPSYS_NAME;
	portno = IDENT_SERVICE;
	timeout = TIMEOUT;
	address = NULL;
	charset = NULL;
	fmt = NULL;
	socks = NULL;
	gid = uid = 0;
	bflag = eflag = fflag = Fflag = iflag = Iflag = 0;
	lflag = Lflag = nflag = Nflag = rflag = 0;

	/* Started from a tty? then run as daemon */
	if (isatty(0))
		bflag = 1;

	/* Parse arguments */
	while ((ch = getopt(argc, argv, "46a:bceF:f:g:IiL:lNno:p:rt:u:")) != -1)
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
			Fflag = 1;
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
					errx(1, "No such group '%s'", optarg);
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
			lflag = 1;
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
		case 'p':
			portno = optarg;
			break;
		case 'r':
			rflag = 1;
			break;
		case 't':
			timeout = (int)strtol(optarg, &p, 0);
			if (*p != '\0') 
				errx(1, "Invalid timeout value '%s'", optarg);
			break;
		case 'u':
			uid = (uid_t)strtol(optarg, &p, 0);
			if (*p != '\0') {
				if ((pw = getpwnam(optarg)) != NULL) {
					uid = pw->pw_uid;
					gid = pw->pw_gid;
				} else
					errx(1, "No such user '%s'", optarg);
			}
			break;
		default:
			exit(1);
		}

	if (lflag)
		openlog("identd", LOG_PID, LOG_DAEMON);

	/* Setup sockets if -b flag */
	if (bflag) {
		socks = socketsetup(address, portno, IPv4or6);
		if (socks == NULL)
			return 1;
	}

	/* Switch to another uid/gid ? */
	if (gid && setgid(gid) == -1) {
		if (lflag)
			syslog(LOG_ERR, "setgid: %m");
		if (bflag)
			warn("setgid");
		exit(1);
	}
	if (uid && setuid(uid) == -1) {
		if (lflag)
			syslog(LOG_ERR, "setuid: %m");
		if (bflag)
			warn("setuid");
		exit(1);
	}

	/* Daemonize, setup pollfds and start mainloop if -b flag */
	if (bflag) {
		int fd, i, nfds, rv;
		struct pollfd *rfds;

		(void)signal(SIGCHLD, waitchild);
		(void)daemon(0, 0);

		rfds = malloc(*socks * sizeof(struct pollfd));
		if (rfds == NULL)
			fatal("malloc");
		nfds = *socks;
		for (i = 0; i < nfds; i++) {
			rfds[i].fd = socks[i+1];
			rfds[i].events = POLLIN;
			rfds[i].revents = 0;
		}
		/* Mainloop for daemon */
		for (;;) {
			rv = poll(rfds, nfds, INFTIM);
			if (rv < 0 && errno == EINTR)
				continue;
			if (rv < 0)
				fatal("poll");
			for (i = 0; i < nfds; i++) {
				if (rfds[i].revents & POLLIN) {
					fd = accept(rfds[i].fd, NULL, NULL);
					if (fd < 0) {
						if (lflag)
							syslog(LOG_ERR,
							    "accept: %m");
						continue;
					}
					switch (fork()) {
					case -1:	/* error */
						if (lflag)
							syslog(LOG_ERR,
							    "fork: %m");
						(void)sleep(1);
						break;
					case 0:		/* child */
						(void)idhandle(fd, charset,
						    fmt, osname, user, timeout);
						_exit(0);
					default:	/* parent */
						(void)close(fd);
					}
				}
			}
		}
	} else
		(void)idhandle(STDIN_FILENO, charset, fmt, osname, user,
		    timeout);

	return 0;
}

static int
idhandle(int fd, const char *charset, const char *fmt, const char *osname,
    const char *user, int timeout)
{
	struct sockaddr_storage ss[2];
	char userbuf[LOGIN_NAME_MAX];	/* actual user name (or numeric uid) */
	char idbuf[LOGIN_NAME_MAX];	/* name to be used in response */
	char buf[BUFSIZ], *p;
	int n, lport, fport;
	struct passwd *pw;
	socklen_t len;
	uid_t uid;

	lport = fport = 0;

	(void)strlcpy(idbuf, user, sizeof(idbuf));
	(void)signal(SIGALRM, timeout_handler);
	(void)alarm(timeout);

	/* Get foreign internet address */
	len = sizeof(ss[0]);
	if (getpeername(fd, (struct sockaddr *)&ss[0], &len) < 0)
		fatal("getpeername");

	if (lflag)
		syslog(LOG_INFO, "Connection from %s", gethost(&ss[0]));

	/* Get local internet address */
	len = sizeof(ss[1]);
	if (getsockname(fd, (struct sockaddr *)&ss[1], &len) < 0)
		fatal("getsockname");

	/* Be sure to have the same address family's */
	if (ss[0].ss_family != ss[1].ss_family) {
		if (lflag)
			syslog(LOG_ERR, "Foreign/local AF are different!");
		return 1;
	}

	/* Receive data from the client */
	if ((n = recv(fd, buf, sizeof(buf) - 1, 0)) < 0) {
		fatal("recv");
	} else if (n == 0) {
		if (lflag)
			syslog(LOG_NOTICE, "recv: EOF");
		iderror(fd, 0, 0, "UNKNOWN-ERROR");
		return 1;
	}
	buf[n] = '\0';

	/* Get local and remote ports from the received data */
	p = buf;
	while (*p != '\0' && isspace(*p))
		p++;
	if ((p = strtok(p, " \t,")) != NULL) {
		lport = atoi(p);
		if ((p = strtok(NULL, " \t,")) != NULL)
			fport = atoi(p);
	}

	/* Are the ports valid? */
	if (lport < 1 || lport > 65535 || fport < 1 || fport > 65535) {
		if (lflag)
			syslog(LOG_NOTICE, "Invalid port(s): %d, %d from %s",
			    lport, fport, gethost(&ss[0]));
		iderror(fd, 0, 0, eflag ? "UNKNOWN-ERROR" : "INVALID-PORT");
		return 1;
	}

	/* If there is a 'lie' user enabled, then handle it now and quit */
	if (Lflag) {
		if (lflag)
			syslog(LOG_NOTICE, "Lying with name %s to %s",
			    idbuf, gethost(&ss[0]));
		idparse(fd, lport, fport, charset, osname, idbuf);
		return 0;
	}

	/* Protocol dependent stuff */
	switch (ss[0].ss_family) {
	case AF_INET:
		((struct sockaddr_in *)&ss[0])->sin_port = htons(fport);
		((struct sockaddr_in *)&ss[1])->sin_port = htons(lport);
		break;
	case AF_INET6:
		((struct sockaddr_in6 *)&ss[0])->sin6_port = htons(fport);
		((struct sockaddr_in6 *)&ss[1])->sin6_port = htons(lport);
		break;
	default:
		if (lflag)
			syslog(LOG_ERR, "Unsupported protocol, proto no. %d",
			    ss[0].ss_family);
		return 1;
	}

	/* Do sysctl call */
	if (sysctl_getuid(ss, sizeof(ss), &uid) == -1) {
		if (lflag)
			syslog(LOG_ERR, "sysctl: %m");
		if (fflag) {
			if (lflag)
				syslog(LOG_NOTICE, "Using fallback name %s "
				    "to %s", idbuf, gethost(&ss[0]));
			idparse(fd, lport, fport, charset, osname, idbuf);
			return 0;
		}
		iderror(fd, lport, fport, eflag ? "UNKNOWN-ERROR" : "NO-USER");
		return 1;
	}

	/* Fill in userbuf with user name if possible, else numeric uid */
	if ((pw = getpwuid(uid)) == NULL) {
		if (lflag)
			syslog(LOG_ERR, "Couldn't map uid (%u) to name", uid);
		(void)snprintf(userbuf, sizeof(userbuf), "%u", uid);
	} else {
		if (lflag)
		    syslog(LOG_INFO, "Successfull lookup: %d, %d: %s for %s",
			lport, fport, pw->pw_name, gethost(&ss[0]));
		(void)strlcpy(userbuf, pw->pw_name, sizeof(userbuf));
	}

	/* No ident enabled? */
	if (Nflag && pw && check_noident(pw->pw_dir)) {
		if (lflag)
			syslog(LOG_NOTICE, "Returning HIDDEN-USER for user %s"
			    " to %s", pw->pw_name, gethost(&ss[0]));
		iderror(fd, lport, fport, "HIDDEN-USER");
		return 1;
	}

	/* User ident enabled ? */
	if (iflag && pw && check_userident(pw->pw_dir, idbuf, sizeof(idbuf))) {
		if (!Iflag) {
			if ((strspn(idbuf, "0123456789") &&
			    getpwuid(atoi(idbuf)) != NULL)
			    || (getpwnam(idbuf) != NULL)) {
				if (lflag)
					syslog(LOG_NOTICE,
					    "Ignoring user-specified '%s' for "
					    "user %s", idbuf, userbuf);
				(void)strlcpy(idbuf, userbuf, sizeof(idbuf));
			}
		}
		if (lflag)
			syslog(LOG_NOTICE, "Returning user-specified '%s' for "
			    "user %s to %s", idbuf, userbuf, gethost(&ss[0]));
		idparse(fd, lport, fport, charset, osname, idbuf);
		return 0;
	}

	/* Send random crap? */
	if (rflag) {
		/* Random number or string? */
		if (nflag)
			(void)snprintf(idbuf, sizeof(idbuf), "%u",
			    (unsigned int)(arc4random() % 65535));
		else
			random_string(idbuf, sizeof(idbuf));

		if (lflag)
			syslog(LOG_NOTICE, "Returning random '%s' for user %s"
			    " to %s", idbuf, userbuf, gethost(&ss[0]));
		idparse(fd, lport, fport, charset, osname, idbuf);
		return 0;
	}

	/* Return number? */
	if (nflag)
		(void)snprintf(idbuf, sizeof(idbuf), "%u", uid);
	else
		(void)strlcpy(idbuf, userbuf, sizeof(idbuf));

	if (Fflag) {
		/* RFC 1413 says that 512 is the limit */
		change_format(fmt, pw, buf, 512);
		idparse(fd, lport, fport, charset, osname, buf);
	} else
		idparse(fd, lport, fport, charset, osname, idbuf);

	return 0;
}

/* Send/parse the ident result */
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

/* Return a specified ident error */
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

/* Return the IP address of the connecting host */
static const char *
gethost(struct sockaddr_storage *ss)
{
	static char host[NI_MAXHOST];

	if (getnameinfo((struct sockaddr *)ss, ss->ss_len, host,
	    sizeof(host), NULL, 0, NI_NUMERICHOST) == 0)
		return host;

	return "UNKNOWN";
}

/* Setup sockets, for daemon mode */
static int *
socketsetup(const char *address, const char *port, int af)
{
	struct addrinfo hints, *res, *res0;
	int error, maxs, *s, *socks, y = 1;
	const char *cause = NULL;

	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(address, port, &hints, &res0);
	if (error) {
		if (lflag)
			syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(error));
		errx(1, "%s", gai_strerror(error));
		/*NOTREACHED*/
	}

	/* Count max number of sockets we may open */
	for (maxs = 0, res = res0; res != NULL; res = res->ai_next)
		maxs++;

	socks = malloc((maxs + 1) * sizeof(int));
	if (socks == NULL) {
		if (lflag)
			syslog(LOG_ERR, "malloc: %m");
		err(1, "malloc");
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
		if (lflag)
			syslog(LOG_ERR, "%s: %m", cause);
		err(1, "%s", cause);
		/* NOTREACHED */
	}
	if (res0)
		freeaddrinfo(res0);

	return socks;
}

/* Return the UID for the connection owner */
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

/* Check if a .noident file exists in the user home directory */
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
	int fd, n;

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

	if ((p = strpbrk(username, "\r\n")))
		*p = '\0';

	(void)close(fd);
	free(path);
	return 1;
}

/* Generate a random string */
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

/* Change the output format */
static void
change_format(const char *format, struct passwd *pw, char *dest, size_t len)
{
	struct group *gr;
	const char *cp;
	char **gmp;
	int bp;

	if (len == 0)
		return;
	if ((gr = getgrgid(pw->pw_gid)) == NULL)
		return;

	for (bp = 0, cp = format; *cp != '\0' && bp < 490; cp++) {
		if (*cp != '%') {
			dest[bp++] = *cp;
			continue;
		}
		if (*++cp == '\0')
			break;
		switch (*cp) {
		case 'u':
			(void)snprintf(&dest[bp], len - bp, "%.*s", 490 - bp,
			    pw->pw_name);
			break;
		case 'U':
			(void)snprintf(&dest[bp], len - bp, "%d", pw->pw_uid);
			break;
		case 'g':
			(void)snprintf(&dest[bp], len - bp, "%.*s", 490 - bp,
			    gr->gr_name);
			break;
		case 'G':
			(void)snprintf(&dest[bp], len - bp, "%d", gr->gr_gid);
			break;
		case 'l':
			(void)snprintf(&dest[bp], len - bp, "%.*s", 490 - bp,
			    gr->gr_name);
			bp += strlen(&dest[bp]);
			if (bp >= 490)
				break;
			setgrent();
			while ((gr = getgrent()) != NULL) {
				if (gr->gr_gid == pw->pw_gid)
					continue;
				for (gmp = gr->gr_mem; *gmp && **gmp; gmp++) {
					if (strcmp(*gmp, pw->pw_name) == 0) {
						(void)snprintf(&dest[bp],
						    len - bp, ",%.*s",
						    490 - bp, gr->gr_name);
						bp += strlen(&dest[bp]);
						break;
					}
				}
				if (bp >= 490)
					break;
			}
			endgrent();
			break;
		case 'L':
			(void)snprintf(&dest[bp], len - bp, "%u", gr->gr_gid);
			bp += strlen(&dest[bp]);
			if (bp >= 490)
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
				if (bp >= 490)
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
	if (bp >= 490) {
		(void)snprintf(&dest[490], len - 490, "...");
		bp = 493;
	}
	dest[bp] = '\0';
}

/* Just exit when we caught SIGALRM */
static void
timeout_handler(int s)
{
	if (lflag)
		syslog(LOG_DEBUG, "SIGALRM triggered, exiting...");
	exit(1);
}

/* This is to clean up zombie processes when in daemon mode */
static void
waitchild(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		continue;
}

/* Report errno through syslog and quit */
static void
fatal(const char *func)
{
	if (lflag)
		syslog(LOG_ERR, "%s: %m", func);
	exit(1);
}
