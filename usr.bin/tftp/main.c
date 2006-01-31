/*	$NetBSD: main.c,v 1.21 2006/01/31 17:36:56 christos Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: main.c,v 1.21 2006/01/31 17:36:56 christos Exp $");
#endif
#endif /* not lint */

/* Many bug fixes are from Jim Guyton <guyton@rand-unix> */

/*
 * TFTP User Program -- Command Interface.
 */
#include <sys/types.h>
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
int	trace;
int	verbose;
int	tsize=0;
int	tout=0;
size_t	def_blksize=SEGSIZE;
size_t	blksize=SEGSIZE;
int	connected;
char	mode[32];
char	line[LBUFLEN];
int	margc;
char	*margv[20];
const	char *prompt = "tftp";
jmp_buf	toplevel;

void	get __P((int, char **));
void	help __P((int, char **));
void	modecmd __P((int, char **));
void	put __P((int, char **));
void	quit __P((int, char **));
void	setascii __P((int, char **));
void	setbinary __P((int, char **));
void	setpeer0 __P((const char *, const char *));
void	setpeer __P((int, char **));
void	setrexmt __P((int, char **));
void	settimeout __P((int, char **));
void	settrace __P((int, char **));
void	setverbose __P((int, char **));
void	setblksize __P((int, char **));
void	settsize __P((int, char **));
void	settimeoutopt __P((int, char **));
void	status __P((int, char **));
char	*tail __P((char *));
int	main __P((int, char *[]));
void	intr __P((int));
const	struct cmd *getcmd __P((char *));

static __dead void command __P((void));

static void getusage __P((char *));
static void makeargv __P((void));
static void putusage __P((char *));
static void settftpmode __P((const char *));

#define HELPINDENT (sizeof("connect"))

struct cmd {
	const char *name;
	const char *help;
	void	(*handler) __P((int, char **));
};

const char vhelp[] = "toggle verbose mode";
const char thelp[] = "toggle packet tracing";
const char tshelp[] = "toggle extended tsize option";
const char tohelp[] = "toggle extended timeout option";
const char blhelp[] = "set an alternative blocksize (def. 512)";
const char chelp[] = "connect to remote tftp";
const char qhelp[] = "exit tftp";
const char hhelp[] = "print help information";
const char shelp[] = "send file";
const char rhelp[] = "receive file";
const char mhelp[] = "set file transfer mode";
const char sthelp[] = "show current status";
const char xhelp[] = "set per-packet retransmission timeout";
const char ihelp[] = "set total retransmission timeout";
const char ashelp[] = "set mode to netascii";
const char bnhelp[] = "set mode to octet";

const struct cmd cmdtab[] = {
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
	{ 0 }
};

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int	c;

	f = -1;
	strcpy(mode, "netascii");
	signal(SIGINT, intr);

	setprogname(argv[0]);
	while ((c = getopt(argc, argv, "e")) != -1) {
		switch (c) {
		case 'e':
			blksize = MAXSEGSIZE;
			strcpy(mode, "octet");
			tsize = 1;
			tout = 1;
			break;
		default:
			printf("usage: %s [-e] host-name [port]\n",
				getprogname());
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
	return (0);
}

char    hostname[100];

void
setpeer0(host, port)
	const char *host;
	const char *port;
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

	memset(&hints, 0, sizeof(hints));
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
		if (f < 0) {
			cause = "socket";
			continue;
		}

		memset(&ss, 0, sizeof(ss));
		ss.ss_family = res->ai_family;
		ss.ss_len = res->ai_addrlen;
		if (bind(f, (struct sockaddr *)(void *)&ss,
		    (socklen_t)ss.ss_len) < 0) {
			cause = "bind";
			close(f);
			f = -1;
			continue;
		}

		break;
	}

	if (f >= 0) {
		soopt = 65536;
		if (setsockopt(f, SOL_SOCKET, SO_SNDBUF, &soopt, sizeof(soopt))
		    < 0) {
			close(f);
			f = -1;
			cause = "setsockopt SNDBUF";
		}
		if (setsockopt(f, SOL_SOCKET, SO_RCVBUF, &soopt, sizeof(soopt))
		    < 0) {
			close(f);
			f = -1;
			cause = "setsockopt RCVBUF";
		}
	}

	if (f < 0)
		warn("%s", cause);
	else {
		/* res->ai_addr <= sizeof(peeraddr) is guaranteed */
		memcpy(&peeraddr, res->ai_addr, res->ai_addrlen);
		if (res->ai_canonname) {
			(void) strlcpy(hostname, res->ai_canonname,
			    sizeof(hostname));
		} else
			(void) strlcpy(hostname, host, sizeof(hostname));
		connected = 1;
	}

	freeaddrinfo(res0);
}

void
setpeer(argc, argv)
	int argc;
	char *argv[];
{

	if (argc < 2) {
		strcpy(line, "Connect ");
		printf("(to) ");
		fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if ((argc < 2) || (argc > 3)) {
		printf("usage: %s [-e] host-name [port]\n", getprogname());
		return;
	}
	if (argc == 2)
		setpeer0(argv[1], NULL);
	else
		setpeer0(argv[1], argv[2]);
}

struct	modes {
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

void
modecmd(argc, argv)
	int argc;
	char *argv[];
{
	struct modes *p;
	const char *sep;

	if (argc < 2) {
		printf("Using %s mode to transfer files.\n", mode);
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
		printf("%s: unknown mode\n", argv[1]);
		/* drop through and print usage message */
	}

	printf("usage: %s [", argv[0]);
	sep = " ";
	for (p = modes; p->m_name; p++) {
		printf("%s%s", sep, p->m_name);
		if (*sep == ' ')
			sep = " | ";
	}
	printf(" ]\n");
	return;
}

void
/*ARGSUSED*/
setbinary(argc, argv)
	int argc;
	char *argv[];
{      

	settftpmode("octet");
}

void
/*ARGSUSED*/
setascii(argc, argv)
	int argc;
	char *argv[];
{

	settftpmode("netascii");
}

static void
settftpmode(newmode)
	const char *newmode;
{
	strcpy(mode, newmode);
	if (verbose)
		printf("mode set to %s\n", mode);
}


/*
 * Send file(s).
 */
void
put(argc, argv)
	int argc;
	char *argv[];
{
	int fd;
	int n;
	char *targ, *p;

	if (argc < 2) {
		strcpy(line, "send ");
		printf("(file) ");
		fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		putusage(argv[0]);
		return;
	}
	targ = argv[argc - 1];
	if (strrchr(argv[argc - 1], ':')) {
		char *cp;

		for (n = 1; n < argc - 1; n++)
			if (strchr(argv[n], ':')) {
				putusage(argv[0]);
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
		printf("No target machine specified.\n");
		return;
	}
	if (argc < 4) {
		char *cp = argc == 2 ? tail(targ) : argv[1];
		fd = open(cp, O_RDONLY);
		if (fd < 0) {
			warn("%s", cp);
			return;
		}
		if (verbose)
			printf("putting %s to %s:%s [%s]\n",
				cp, hostname, targ, mode);
		sendfile(fd, targ, mode);
		return;
	}
				/* this assumes the target is a directory */
				/* on a remote unix system.  hmmmm.  */
	p = strchr(targ, '\0'); 
	*p++ = '/';
	for (n = 1; n < argc - 1; n++) {
		strcpy(p, tail(argv[n]));
		fd = open(argv[n], O_RDONLY);
		if (fd < 0) {
			warn("%s", argv[n]);
			continue;
		}
		if (verbose)
			printf("putting %s to %s:%s [%s]\n",
				argv[n], hostname, targ, mode);
		sendfile(fd, targ, mode);
	}
}

static void
putusage(s)
	char *s;
{
	printf("usage: %s file ... host:target, or\n", s);
	printf("       %s file ... target (when already connected)\n", s);
}

/*
 * Receive file(s).
 */
void
get(argc, argv)
	int argc;
	char *argv[];
{
	int fd;
	int n;
	char *p;
	char *src;

	if (argc < 2) {
		strcpy(line, "get ");
		printf("(files) ");
		fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		getusage(argv[0]);
		return;
	}
	if (!connected) {
		for (n = 1; n < argc ; n++)
			if (strrchr(argv[n], ':') == 0) {
				getusage(argv[0]);
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
			if (fd < 0) {
				warn("%s", cp);
				return;
			}
			if (verbose)
				printf("getting from %s:%s to %s [%s]\n",
					hostname, src, cp, mode);
			recvfile(fd, src, mode);
			break;
		}
		p = tail(src);         /* new .. jdg */
		fd = creat(p, 0644);
		if (fd < 0) {
			warn("%s", p);
			continue;
		}
		if (verbose)
			printf("getting from %s:%s to %s [%s]\n",
				hostname, src, p, mode);
		recvfile(fd, src, mode);
	}
}

static void
getusage(s)
	char *s;
{
	printf("usage: %s host:file host:file ... file, or\n", s);
	printf("       %s file file ... file if connected\n", s);
}

void
setblksize(argc, argv)
	int argc;
	char *argv[];
{
	int t;

	if (argc < 2) {
		strcpy(line, "blksize ");
		printf("(blksize) ");
		fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 8 || t > 65464)
		printf("%s: bad value\n", argv[1]);
	else
		blksize = t;
}

unsigned int	def_rexmtval = TIMEOUT;
unsigned int	rexmtval = TIMEOUT;

void
setrexmt(argc, argv)
	int argc;
	char *argv[];
{
	int t;

	if (argc < 2) {
		strcpy(line, "Rexmt-timeout ");
		printf("(value) ");
		fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 0)
		printf("%s: bad value\n", argv[1]);
	else
		rexmtval = t;
}

int	maxtimeout = 5 * TIMEOUT;

void
settimeout(argc, argv)
	int argc;
	char *argv[];
{
	int t;

	if (argc < 2) {
		strcpy(line, "Maximum-timeout ");
		printf("(value) ");
		fgets(&line[strlen(line)], (int)(LBUFLEN-strlen(line)), stdin);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 0)
		printf("%s: bad value\n", argv[1]);
	else
		maxtimeout = t;
}

void
/*ARGSUSED*/
status(argc, argv)
	int argc;
	char *argv[];
{
	if (connected)
		printf("Connected to %s.\n", hostname);
	else
		printf("Not connected.\n");
	printf("Mode: %s Verbose: %s Tracing: %s\n", mode,
		verbose ? "on" : "off", trace ? "on" : "off");
	printf("Rexmt-interval: %d seconds, Max-timeout: %d seconds\n",
		rexmtval, maxtimeout);
}

void
/*ARGSUSED*/
intr(dummy)
	int dummy;
{

	signal(SIGALRM, SIG_IGN);
	alarm(0);
	longjmp(toplevel, -1);
}

char *
tail(filename)
	char *filename;
{
	char *s;
	
	while (*filename) {
		s = strrchr(filename, '/');
		if (s == NULL)
			break;
		if (s[1])
			return (s + 1);
		*s = '\0';
	}
	return (filename);
}

/*
 * Command parser.
 */
static __dead void
command()
{
	const struct cmd *c;

	for (;;) {
		printf("%s> ", prompt);
		if (fgets(line, LBUFLEN, stdin) == 0) {
			if (feof(stdin)) {
				exit(0);
			} else {
				continue;
			}
		}
		if ((line[0] == 0) || (line[0] == '\n'))
			continue;
		makeargv();
		if (margc == 0)
			continue;
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			printf("?Ambiguous command\n");
			continue;
		}
		if (c == 0) {
			printf("?Invalid command\n");
			continue;
		}
		(*c->handler)(margc, margv);
	}
}

const struct cmd *
getcmd(name)
	char *name;
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
				return (c);
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
		return ((struct cmd *)-1);
	return (found);
}

/*
 * Slice a string up into argc/argv.
 */
static void
makeargv()
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
	*argp++ = 0;
}

void
/*ARGSUSED*/
quit(argc, argv)
	int argc;
	char *argv[];
{

	exit(0);
}

/*
 * Help command.
 */
void
help(argc, argv)
	int argc;
	char *argv[];
{
	const struct cmd *c;

	if (argc == 1) {
		printf("Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c->name; c++)
			printf("%-*s\t%s\n", (int)HELPINDENT, c->name, c->help);
		return;
	}
	while (--argc > 0) {
		char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (c == (struct cmd *)-1)
			printf("?Ambiguous help command %s\n", arg);
		else if (c == (struct cmd *)0)
			printf("?Invalid help command %s\n", arg);
		else
			printf("%s\n", c->help);
	}
}

void
/*ARGSUSED*/
settrace(argc, argv)
	int argc;
	char **argv;
{
	trace = !trace;
	printf("Packet tracing %s.\n", trace ? "on" : "off");
}

void
/*ARGSUSED*/
setverbose(argc, argv)
	int argc;
	char **argv;
{
	verbose = !verbose;
	printf("Verbose mode %s.\n", verbose ? "on" : "off");
}

void
/*ARGSUSED*/
settsize(argc, argv)
	int argc;
	char **argv;
{
	tsize = !tsize;
	printf("Tsize mode %s.\n", tsize ? "on" : "off");
}

void
/*ARGSUSED*/
settimeoutopt(argc, argv)
	int argc;
	char **argv;
{
	tout = !tout;
	printf("Timeout option %s.\n", tout ? "on" : "off");
}
