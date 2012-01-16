/*	$NetBSD: main.c,v 1.30 2012/01/16 17:38:16 christos Exp $	*/

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
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: main.c,v 1.30 2012/01/16 17:38:16 christos Exp $");
#endif
#endif /* not lint */

/* Many bug fixes are from Jim Guyton <guyton@rand-unix> */

/*
 * TFTP User Program -- Command Interface.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>
#include <arpa/tftp.h>

#include <ctype.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define	TIMEOUT		5		/* secs between rexmt's */
#define	LBUFLEN		200		/* size of input buffer */

struct	sockaddr_storage peeraddr;
int	f;
int	mf;
int	trace;
int	verbose;
int	tsize=0;
int	tout=0;
size_t	def_blksize = SEGSIZE;
size_t	blksize = SEGSIZE;
in_addr_t	mcaddr = INADDR_NONE;
uint16_t	mcport;
u_int	def_rexmtval = TIMEOUT;
u_int	rexmtval = TIMEOUT;
ushort	mcmasterslave;
int	maxtimeout = 5 * TIMEOUT;

static int	connected;
static char	mode[32];
static char	line[LBUFLEN];
static int	margc;
static char	*margv[20];
static const	char *prompt = "tftp";
static char    hostname[MAXHOSTNAMELEN];
static jmp_buf	toplevel;

static void	get(int, char **);
static void	help(int, char **);
static void	modecmd(int, char **);
static void	put(int, char **);
static __dead void quit(int, char **);
static void	setascii(int, char **);
static void	setbinary(int, char **);
static void	setpeer0(const char *, const char *);
static void	setpeer(int, char **);
static void	setrexmt(int, char **);
static void	settimeout(int, char **);
static void	settrace(int, char **);
static void	setverbose(int, char **);
static void	setblksize(int, char **);
static void	settsize(int, char **);
static void	settimeoutopt(int, char **);
static void	status(int, char **);
static char	*tail(char *);
static __dead void intr(int);
static const	struct cmd *getcmd(const char *);

static __dead void command(void);

static void getUsage(char *);
static void makeargv(void);
static void putUsage(const char *);
static void settftpmode(const char *);

#define HELPINDENT sizeof("connect")

struct cmd {
	const char *name;
	const char *help;
	void	(*handler)(int, char **);
};

static const char vhelp[] = "toggle verbose mode";
static const char thelp[] = "toggle packet tracing";
static const char tshelp[] = "toggle extended tsize option";
static const char tohelp[] = "toggle extended timeout option";
static const char blhelp[] = "set an alternative blocksize (def. 512)";
static const char chelp[] = "connect to remote tftp";
static const char qhelp[] = "exit tftp";
static const char hhelp[] = "print help information";
static const char shelp[] = "send file";
static const char rhelp[] = "receive file";
static const char mhelp[] = "set file transfer mode";
static const char sthelp[] = "show current status";
static const char xhelp[] = "set per-packet retransmission timeout";
static const char ihelp[] = "set total retransmission timeout";
static const char ashelp[] = "set mode to netascii";
static const char bnhelp[] = "set mode to octet";

static const struct cmd cmdtab[] = {
	{ "connect",	chelp,		setpeer },
	{ "mode",       mhelp,          modecmd },
	{ "put",	shelp,		put },
	{ "get",	rhelp,		get },
	{ "quit",	qhelp,		quit },
	{ "verbose",	vhelp,		setverbose },
	{ "blksize",	blhelp,		setblksize },
	{ "tsize",	tshelp,		settsize },
	{ "trace",	thelp,		settrace },
	{ "status",	sthelp,		status },
	{ "binary",     bnhelp,         setbinary },
	{ "ascii",      ashelp,         setascii },
	{ "rexmt",	xhelp,		setrexmt },
	{ "timeout",	ihelp,		settimeout },
	{ "tout",	tohelp,		settimeoutopt },
	{ "?",		hhelp,		help },
	{ .name = NULL }
};

static struct	modes {
	const char *m_name;
	const char *m_mode;
} modes[] = {
	{ "ascii",	"netascii" },
	{ "netascii",   "netascii" },
	{ "binary",     "octet" },
	{ "image",      "octet" },
	{ "octet",     "octet" },
/*      { "mail",       "mail" },       */
	{ 0,		0 }
};

int
main(int argc, char *argv[])
{
	int	c;

	f = mf = -1;
	(void)strlcpy(mode, "netascii", sizeof(mode));
	(void)signal(SIGINT, intr);

	setprogname(argv[0]);
	while ((c = getopt(argc, argv, "e")) != -1) {
		switch (c) {
		case 'e':
			blksize = MAXSEGSIZE;
			(void)strlcpy(mode, "octet", sizeof(mode));
			tsize = 1;
			tout = 1;
			break;
		default:
			(void)fprintf(stderr,
			    "Usage: %s [-e] host-name [port]\n", getprogname());
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc >= 1) {
		if (setjmp(toplevel) != 0)
			exit(0);
		argc++;
		argv--;
		setpeer(argc, argv);
	}
	if (setjmp(toplevel) != 0)
		(void)putchar('\n');
	command();
	return 0;
}

static void
getmore(const char *cmd, const char *prm)
{
	(void)strlcpy(line, cmd, sizeof(line));
	(void)printf("%s", prm);
	(void)fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
	makeargv();
}

static void
setpeer0(const char *host, const char *port)
{
	struct addrinfo hints, *res0, *res;
	int error, soopt;
	struct sockaddr_storage ss;
	const char *cause = "unknown";

	if (connected) {
		(void)close(f);
		f = -1;
	}
	connected = 0;

	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_CANONNAME;
	if (!port)
		port = "tftp";
	error = getaddrinfo(host, port, &hints, &res0);
	if (error) {
		warnx("%s", gai_strerror(error));
		return;
	}

	for (res = res0; res; res = res->ai_next) {
		if (res->ai_addrlen > sizeof(peeraddr))
			continue;
		f = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (f == -1) {
			cause = "socket";
			continue;
		}

		(void)memset(&ss, 0, sizeof(ss));
		ss.ss_family = res->ai_family;
		ss.ss_len = res->ai_addrlen;
		if (bind(f, (struct sockaddr *)(void *)&ss,
		    (socklen_t)ss.ss_len) == -1) {
			cause = "bind";
			(void)close(f);
			f = -1;
			continue;
		}

		break;
	}

	if (f >= 0) {
		soopt = 65536;
		if (setsockopt(f, SOL_SOCKET, SO_SNDBUF, &soopt, sizeof(soopt))
		    == -1) {
			(void)close(f);
			f = -1;
			cause = "setsockopt SNDBUF";
		}
		else if (setsockopt(f, SOL_SOCKET, SO_RCVBUF, &soopt,
		    sizeof(soopt)) == -1) {
			(void)close(f);
			f = -1;
			cause = "setsockopt RCVBUF";
		}
	}

	if (f == -1 || res == NULL)
		warn("%s", cause);
	else {
		/* res->ai_addr <= sizeof(peeraddr) is guaranteed */
		(void)memcpy(&peeraddr, res->ai_addr, res->ai_addrlen);
		if (res->ai_canonname) {
			(void)strlcpy(hostname, res->ai_canonname,
			    sizeof(hostname));
		} else
			(void)strlcpy(hostname, host, sizeof(hostname));
		connected = 1;
	}

	freeaddrinfo(res0);
}

static void
setpeer(int argc, char *argv[])
{

	if (argc < 2) {
		getmore("Connect ", "(to) ");
		argc = margc;
		argv = margv;
	}
	if (argc < 2 || argc > 3) {
		(void)printf("Usage: %s [-e] host-name [port]\n",
		    getprogname());
		return;
	}
	if (argc == 2)
		setpeer0(argv[1], NULL);
	else
		setpeer0(argv[1], argv[2]);
}

static void
modecmd(int argc, char *argv[])
{
	struct modes *p;
	const char *sep;

	if (argc < 2) {
		(void)printf("Using %s mode to transfer files.\n", mode);
		return;
	}
	if (argc == 2) {
		for (p = modes; p->m_name; p++)
			if (strcmp(argv[1], p->m_name) == 0)
				break;
		if (p->m_name) {
			settftpmode(p->m_mode);
			return;
		}
		(void)printf("%s: unknown mode\n", argv[1]);
		/* drop through and print Usage message */
	}

	(void)printf("Usage: %s [", argv[0]);
	sep = " ";
	for (p = modes; p->m_name; p++) {
		(void)printf("%s%s", sep, p->m_name);
		if (*sep == ' ')
			sep = " | ";
	}
	(void)printf(" ]\n");
	return;
}

static void
/*ARGSUSED*/
setbinary(int argc, char *argv[])
{      

	settftpmode("octet");
}

static void
/*ARGSUSED*/
setascii(int argc, char *argv[])
{

	settftpmode("netascii");
}

static void
settftpmode(const char *newmode)
{
	(void)strlcpy(mode, newmode, sizeof(mode));
	if (verbose)
		(void)printf("mode set to %s\n", mode);
}


/*
 * Send file(s).
 */
static void
put(int argc, char *argv[])
{
	int fd;
	int n;
	char *targ, *p;

	if (argc < 2) {
		getmore("send ", "(file) ");
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		putUsage(argv[0]);
		return;
	}
	targ = argv[argc - 1];
	if (strrchr(argv[argc - 1], ':')) {
		char *cp;

		for (n = 1; n < argc - 1; n++)
			if (strchr(argv[n], ':')) {
				putUsage(argv[0]);
				return;
			}
		cp = argv[argc - 1];
		targ = strrchr(cp, ':');
		*targ++ = 0;
		if (cp[0] == '[' && cp[strlen(cp) - 1] == ']') {
			cp[strlen(cp) - 1] = '\0';
			cp++;
		}
		setpeer0(cp, NULL);
	}
	if (!connected) {
		(void)printf("No target machine specified.\n");
		return;
	}
	if (argc < 4) {
		char *cp = argc == 2 ? tail(targ) : argv[1];
		fd = open(cp, O_RDONLY);
		if (fd == -1) {
			warn("%s", cp);
			return;
		}
		if (verbose)
			(void)printf("putting %s to %s:%s [%s]\n",
				cp, hostname, targ, mode);
		sendfile(fd, targ, mode);
		return;
	}
				/* this assumes the target is a directory */
				/* on a remote unix system.  hmmmm.  */
	p = strchr(targ, '\0'); 
	*p++ = '/';
	for (n = 1; n < argc - 1; n++) {
		(void)strcpy(p, tail(argv[n]));
		fd = open(argv[n], O_RDONLY);
		if (fd == -1) {
			warn("%s", argv[n]);
			continue;
		}
		if (verbose)
			(void)printf("putting %s to %s:%s [%s]\n",
				argv[n], hostname, targ, mode);
		sendfile(fd, targ, mode);
	}
}

static void
putUsage(const char *s)
{
	(void)printf("Usage: %s file ... host:target, or\n", s);
	(void)printf("       %s file ... target (when already connected)\n", s);
}

/*
 * Receive file(s).
 */
static void
get(int argc, char *argv[])
{
	int fd;
	int n;
	char *p;
	char *src;

	if (argc < 2) {
		getmore("get ", "(files) ");
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		getUsage(argv[0]);
		return;
	}
	if (!connected) {
		for (n = 1; n < argc ; n++)
			if (strrchr(argv[n], ':') == 0) {
				getUsage(argv[0]);
				return;
			}
	}
	for (n = 1; n < argc ; n++) {
		src = strrchr(argv[n], ':');
		if (src == NULL)
			src = argv[n];
		else {
			char *cp;
			*src++ = 0;
			cp = argv[n];
			if (cp[0] == '[' && cp[strlen(cp) - 1] == ']') {
				cp[strlen(cp) - 1] = '\0';
				cp++;
			}
			setpeer0(cp, NULL);
			if (!connected)
				continue;
		}
		if (argc < 4) {
			char *cp = argc == 3 ? argv[2] : tail(src);
			fd = creat(cp, 0644);
			if (fd == -1) {
				warn("%s", cp);
				return;
			}
			if (verbose)
				(void)printf("getting from %s:%s to %s [%s]\n",
					hostname, src, cp, mode);
			recvfile(fd, src, mode);
			break;
		}
		p = tail(src);         /* new .. jdg */
		fd = creat(p, 0644);
		if (fd == -1) {
			warn("%s", p);
			continue;
		}
		if (verbose)
			(void)printf("getting from %s:%s to %s [%s]\n",
				hostname, src, p, mode);
		recvfile(fd, src, mode);
	}
}

static void
getUsage(s)
	char *s;
{
	(void)printf("Usage: %s host:file host:file ... file, or\n", s);
	(void)printf("       %s file file ... file if connected\n", s);
}

void
setblksize(argc, argv)
	int argc;
	char *argv[];
{
	int t;

	if (argc < 2) {
		getmore("blksize ", "(blksize) ");
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		(void)printf("Usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 8 || t > 65464)
		(void)printf("%s: bad value\n", argv[1]);
	else
		blksize = t;
}

static void
setrexmt(int argc, char *argv[])
{
	int t;

	if (argc < 2) {
		getmore("Rexmt-timeout ", "(value) ");
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		(void)printf("Usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 0)
		(void)printf("%s: bad value\n", argv[1]);
	else
		rexmtval = t;
}

static void
settimeout(int argc, char *argv[])
{
	int t;

	if (argc < 2) {
		getmore("Maximum-timeout ", "(value) ");
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		(void)printf("Usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 0)
		(void)printf("%s: bad value\n", argv[1]);
	else
		maxtimeout = t;
}

static void
/*ARGSUSED*/
status(int argc, char *argv[])
{
	if (connected)
		(void)printf("Connected to %s.\n", hostname);
	else
		(void)printf("Not connected.\n");
	(void)printf("Mode: %s Verbose: %s Tracing: %s\n", mode,
		verbose ? "on" : "off", trace ? "on" : "off");
	(void)printf("Rexmt-interval: %d seconds, Max-timeout: %d seconds\n",
		rexmtval, maxtimeout);
}

static void
/*ARGSUSED*/
intr(int dummy)
{

	(void)signal(SIGALRM, SIG_IGN);
	(void)alarm(0);
	longjmp(toplevel, -1);
}

static char *
tail(char *filename)
{
	char *s;
	
	while (*filename) {
		s = strrchr(filename, '/');
		if (s == NULL)
			break;
		if (s[1])
			return s + 1;
		*s = '\0';
	}
	return filename;
}

/*
 * Command parser.
 */
static __dead void
command(void)
{
	const struct cmd *c;

	for (;;) {
		(void)printf("%s> ", prompt);
		if (fgets(line, LBUFLEN, stdin) == NULL) {
			if (feof(stdin)) {
				exit(0);
			} else {
				continue;
			}
		}
		if (line[0] == '\0' || line[0] == '\n')
			continue;
		makeargv();
		if (margc == 0)
			continue;
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			(void)printf("?Ambiguous command\n");
			continue;
		}
		if (c == 0) {
			(void)printf("?Invalid command\n");
			continue;
		}
		(*c->handler)(margc, margv);
	}
}

static const struct cmd *
getcmd(const char *name)
{
	const char *p, *q;
	const struct cmd *c, *found;
	int nmatches, longest;

	longest = 0;
	nmatches = 0;
	found = 0;
	for (c = cmdtab; (p = c->name) != NULL; c++) {
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return c;
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return (struct cmd *)-1;
	return found;
}

/*
 * Slice a string up into argc/argv.
 */
static void
makeargv(void)
{
	char *cp;
	char **argp = margv;

	margc = 0;
	for (cp = line; *cp;) {
		while (isspace((unsigned char)*cp))
			cp++;
		if (*cp == '\0')
			break;
		*argp++ = cp;
		margc += 1;
		while (*cp != '\0' && !isspace((unsigned char)*cp))
			cp++;
		if (*cp == '\0')
			break;
		*cp++ = '\0';
	}
	*argp++ = NULL;
}

static void
/*ARGSUSED*/
quit(int argc, char *argv[])
{

	exit(0);
}

/*
 * Help command.
 */
static void
help(int argc, char *argv[])
{
	const struct cmd *c;

	if (argc == 1) {
		(void)printf("Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c->name; c++)
			(void)printf("%-*s\t%s\n", (int)HELPINDENT, c->name,
			    c->help);
		return;
	}
	while (--argc > 0) {
		char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (c == (struct cmd *)-1)
			(void)printf("?Ambiguous help command %s\n", arg);
		else if (c == NULL)
			(void)printf("?Invalid help command %s\n", arg);
		else
			(void)printf("%s\n", c->help);
	}
}

static void
/*ARGSUSED*/
settrace(int argc, char **argv)
{
	trace = !trace;
	(void)printf("Packet tracing %s.\n", trace ? "on" : "off");
}

static void
/*ARGSUSED*/
setverbose(int argc, char **argv)
{
	verbose = !verbose;
	(void)printf("Verbose mode %s.\n", verbose ? "on" : "off");
}

static void
/*ARGSUSED*/
settsize(int argc, char **argv)
{
	tsize = !tsize;
	(void)printf("Tsize mode %s.\n", tsize ? "on" : "off");
}

static void
/*ARGSUSED*/
settimeoutopt(int argc, char **argv)
{
	tout = !tout;
	(void)printf("Timeout option %s.\n", tout ? "on" : "off");
}
