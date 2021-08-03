/*	$NetBSD: rlogin.c,v 1.48 2021/08/03 23:21:07 chs Exp $	*/

/*
 * Copyright (c) 1983, 1990, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rlogin.c	8.4 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: rlogin.c,v 1.48 2021/08/03 23:21:07 chs Exp $");
#endif
#endif /* not lint */

/*
 * rlogin - remote login
 */
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "getport.h"

#ifndef TIOCPKT_WINDOW
#define	TIOCPKT_WINDOW	0x80
#endif

/* concession to Sun */
#ifndef SIGUSR1
#define	SIGUSR1	30
#endif

#ifndef CCEQ
#define CCEQ(val, c)	(c == val ? val != _POSIX_VDISABLE : 0)
#endif

static int eight, rem;
static struct termios deftty;

static int noescape;
static u_char escapechar = '~';

#ifdef OLDSUN
struct winsize {
	unsigned short ws_row, ws_col;
	unsigned short ws_xpixel, ws_ypixel;
};
#else
#define	get_window_size(fd, wp)	ioctl(fd, TIOCGWINSZ, wp)
#endif
static struct	winsize winsize;

static void		catch_child(int);
static void		copytochild(int);
__dead static void	doit(sigset_t *);
__dead static void	done(int);
static void		echo(int);
static u_int		getescape(char *);
__dead static void	lostpeer(int);
static void		mode(int);
static void		msg(const char *);
static void		oob(int);
static int		reader(sigset_t *);
static void		sendwindow(void);
static void		setsignal(int);
static void		sigwinch(int);
static void		stop(int);
__dead static void	usage(void);
static void		writer(void);
static void		writeroob(int);

#ifdef OLDSUN
static int		get_window_size(int, struct winsize *);
#endif

int
main(int argc, char *argv[])
{
	struct passwd *pw;
	struct servent *sp;
	struct termios tty;
	sigset_t imask, omask;
	uid_t uid;
	int argoff, ch, dflag, nflag, one;
	int i, len, len2;
	int family = AF_UNSPEC;
	char *host, *p, *user, *name, term[1024] = "network";
	speed_t ospeed;
	struct sigaction sa;
	char *service = NULL;
	struct rlimit rlim;

	argoff = dflag = nflag = 0;
	one = 1;
	host = user = NULL;
	sp = NULL;

	if (strcmp(getprogname(), "rlogin") != 0) {
		host = strdup(getprogname());
		if (host == NULL)
			err(1, NULL);
	}

	/* handle "rlogin host flags" */
	if (!host && argc > 2 && argv[1][0] != '-') {
		host = argv[1];
		argoff = 1;
	}

#define	OPTIONS	"468dEe:l:np:"
	while ((ch = getopt(argc - argoff, argv + argoff, OPTIONS)) != -1)
		switch(ch) {
		case '4':
			family = AF_INET;
			break;
		case '6':
			family = AF_INET6;
			break;
		case '8':
			eight = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'E':
			noescape = 1;
			break;
		case 'e':
			noescape = 0;
			escapechar = getescape(optarg);
			break;
		case 'l':
			user = optarg;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'p':
			sp = getport(service = optarg, "tcp");
			break;
		case '?':
		default:
			usage();
		}
	optind += argoff;
	argc -= optind;
	argv += optind;

	/* if haven't gotten a host yet, do so */
	if (!host && !(host = *argv++))
		usage();

	if (*argv)
		usage();

	if (!(pw = getpwuid(uid = getuid())))
		errx(1, "unknown user id.");
	/* Accept user1@host format, though "-l user2" overrides user1 */
	p = strchr(host, '@');
	if (p) {
		*p = '\0';
		if (!user && p > host)
			user = host;
		host = p + 1;
		if (*host == '\0')
			usage();
	}
	if ((name = strdup(pw->pw_name)) == NULL)
		err(1, "malloc");
	if (!user)
		user = name;

	if (sp == NULL)
		sp = getservbyname("login", "tcp");
	if (sp == NULL)
		errx(1, "login/tcp: unknown service.");

	if ((p = getenv("TERM")) != NULL)
		(void)strlcpy(term, p, sizeof(term));
	len = strlen(term);
	if (len < (int)(sizeof(term) - 1) && tcgetattr(0, &tty) == 0) {
		/* start at 2 to include the / */
		for (ospeed = i = cfgetospeed(&tty), len2 = 2; i > 9; len2++)
			i /= 10;

		if (len + len2 < (int)sizeof(term))
			(void)snprintf(term + len, len2 + 1, "/%d", ospeed);
	}

	(void)get_window_size(0, &winsize);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = lostpeer;
	(void)sigaction(SIGPIPE, &sa, NULL);
	/* will use SIGUSR1 for window size hack, so hold it off */
	sigemptyset(&imask);
	sigaddset(&imask, SIGURG);
	sigaddset(&imask, SIGUSR1);
	(void)sigprocmask(SIG_SETMASK, &imask, &omask);
	/*
	 * We set SIGURG and SIGUSR1 below so that an
	 * incoming signal will be held pending rather than being
	 * discarded. Note that these routines will be ready to get
	 * a signal by the time that they are unblocked below.
	 */
	sa.sa_handler = copytochild;
	(void)sigaction(SIGURG, &sa, NULL);
	sa.sa_handler = writeroob;
	(void)sigaction(SIGUSR1, &sa, NULL);

	/* don't dump core */
	rlim.rlim_cur = rlim.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &rlim) < 0)
		warn("setrlimit");

	rem = rcmd_af(&host, sp->s_port, name, user, term, 0, family);
	if (rem < 0)
		exit(1);

	if (dflag &&
	    setsockopt(rem, SOL_SOCKET, SO_DEBUG, &one, sizeof(one)) < 0)
		warn("setsockopt DEBUG (ignored)");
	if (nflag &&
	    setsockopt(rem, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
		warn("setsockopt NODELAY (ignored)");

	{
		struct sockaddr_storage ss;
		socklen_t sslen = sizeof(ss);
		if (getsockname(rem, (struct sockaddr *)&ss, &sslen) == 0
		     && ((struct sockaddr *)&ss)->sa_family == AF_INET) {
			one = IPTOS_LOWDELAY;
			if (setsockopt(rem, IPPROTO_IP, IP_TOS, (char *)&one,
			    sizeof(int)) < 0)
				warn("setsockopt TOS (ignored)");
		}
	}

	(void)setuid(uid);
	doit(&omask);
	/*NOTREACHED*/
	return (0);
}

static pid_t child;

static void
doit(sigset_t *smask)
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_IGN;
	(void)sigaction(SIGINT, &sa, NULL);
	setsignal(SIGHUP);
	setsignal(SIGQUIT);
	mode(1);
	child = fork();
	if (child == -1) {
		warn("fork");
		done(1);
	}
	if (child == 0) {
		mode(1);
		if (reader(smask) == 0) {
			msg("connection closed.");
			exit(0);
		}
		sleep(1);
		msg("\aconnection closed.");
		exit(1);
	}

	/*
	 * We may still own the socket, and may have a pending SIGURG (or might
	 * receive one soon) that we really want to send to the reader.  When
	 * one of these comes in, the trap copytochild simply copies such
	 * signals to the child. We can now unblock SIGURG and SIGUSR1
	 * that were set above.
	 */
	(void)sigprocmask(SIG_SETMASK, smask, NULL);
	sa.sa_handler = catch_child;
	(void)sigaction(SIGCHLD, &sa, NULL);
	writer();
	msg("closed connection.");
	done(0);
}

/* trap a signal, unless it is being ignored. */
static void
setsignal(int sig)
{
	struct sigaction isa, osa;
	sigset_t isigs, osigs;

	sigemptyset(&isigs);
	sigaddset(&isigs, sig);
	sigprocmask(SIG_BLOCK, &isigs, &osigs);

	sigemptyset(&isa.sa_mask);
	isa.sa_handler = exit;
	isa.sa_flags = SA_RESTART;
	(void)sigaction(sig, &isa, &osa);
	if (osa.sa_handler == SIG_IGN)
		(void)sigaction(sig, &osa, NULL);

	(void)sigprocmask(SIG_SETMASK, &osigs, NULL);
}

static void
done(int status)
{
	pid_t w;
	int wstatus;
	struct sigaction sa;

	mode(0);
	if (child > 0) {
		/* make sure catch_child does not snap it up */
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = SIG_DFL;
		sa.sa_flags = 0;
		(void)sigaction(SIGCHLD, &sa, NULL);
		if (kill(child, SIGKILL) >= 0)
			while ((w = wait(&wstatus)) > 0 && w != child)
				continue;
	}
	exit(status);
}

static int dosigwinch;

/*
 * This is called when the reader process gets the out-of-band (urgent)
 * request to turn on the window-changing protocol.
 */
static void
writeroob(int signo)
{
	struct sigaction sa;

	if (dosigwinch == 0) {
		sendwindow();
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = sigwinch;
		sa.sa_flags = SA_RESTART;
		(void)sigaction(SIGWINCH, &sa, NULL);
	}
	dosigwinch = 1;
}

static void
catch_child(int signo)
{
	int status;
	pid_t pid;

	for (;;) {
		pid = waitpid(-1, &status, WNOHANG|WUNTRACED);
		if (pid == 0)
			return;
		/* if the child (reader) dies, just quit */
		if (pid < 0 || (pid == child && !WIFSTOPPED(status)))
			done(WEXITSTATUS(status) | WTERMSIG(status));
	}
	/* NOTREACHED */
}

/*
 * writer: write to remote: 0 -> line.
 * ~.				terminate
 * ~^Z				suspend rlogin process.
 * ~<delayed-suspend char>	suspend rlogin process, but leave reader alone.
 */
static void
writer(void)
{
	int bol, local;
	ssize_t n;
	char c;

	bol = 1;			/* beginning of line */
	local = 0;
	for (;;) {
		n = read(STDIN_FILENO, &c, 1);
		if (n <= 0) {
			if (n < 0 && errno == EINTR)
				continue;
			break;
		}
		/*
		 * If we're at the beginning of the line and recognize a
		 * command character, then we echo locally.  Otherwise,
		 * characters are echo'd remotely.  If the command character
		 * is doubled, this acts as a force and local echo is
		 * suppressed.
		 */
		if (bol) {
			bol = 0;
			if (!noescape && c == escapechar) {
				local = 1;
				continue;
			}
		} else if (local) {
			local = 0;
			if (c == '.' || CCEQ(deftty.c_cc[VEOF], c)) {
				echo((int)c);
				break;
			}
			if (CCEQ(deftty.c_cc[VSUSP], c)) {
				bol = 1;
				echo((int)c);
				stop(1);
				continue;
			}
			if (CCEQ(deftty.c_cc[VDSUSP], c)) {
				bol = 1;
				echo((int)c);
				stop(0);
				continue;
			}
			if (c != escapechar) {
				(void)write(rem, &escapechar, 1);
			}
		}

		if (write(rem, &c, 1) == 0) {
			msg("line gone");
			break;
		}

		bol = CCEQ(deftty.c_cc[VKILL], c) ||
		    CCEQ(deftty.c_cc[VEOF], c) ||
		    CCEQ(deftty.c_cc[VINTR], c) ||
		    CCEQ(deftty.c_cc[VSUSP], c) ||
		    c == '\r' || c == '\n';
	}
}

static void
echo(int i)
{
	char c = (char)i;
	char *p;
	char buf[8];

	p = buf;
	c &= 0177;
	*p++ = escapechar;
	if (c < ' ') {
		*p++ = '^';
		*p++ = c + '@';
	} else if (c == 0177) {
		*p++ = '^';
		*p++ = '?';
	} else
		*p++ = c;
	*p++ = '\r';
	*p++ = '\n';
	(void)write(STDOUT_FILENO, buf, p - buf);
}

static void
stop(int all)
{
	struct sigaction sa;

	mode(0);
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	(void)sigaction(SIGCHLD, &sa, NULL);
	(void)kill(all ? 0 : getpid(), SIGTSTP);
	sa.sa_handler = catch_child;
	(void)sigaction(SIGCHLD, &sa, NULL);
	mode(1);
	sigwinch(0);			/* check for size changes */
}

static void
sigwinch(int signo)
{
	struct winsize ws;

	if (dosigwinch && get_window_size(0, &ws) == 0 &&
	    memcmp(&ws, &winsize, sizeof(ws))) {
		winsize = ws;
		sendwindow();
	}
}

/*
 * Send the window size to the server via the magic escape
 */
static void
sendwindow(void)
{
	struct winsize *wp;
	char obuf[4 + sizeof (struct winsize)];

	wp = (struct winsize *)(obuf+4);
	obuf[0] = 0377;
	obuf[1] = 0377;
	obuf[2] = 's';
	obuf[3] = 's';
	wp->ws_row = htons(winsize.ws_row);
	wp->ws_col = htons(winsize.ws_col);
	wp->ws_xpixel = htons(winsize.ws_xpixel);
	wp->ws_ypixel = htons(winsize.ws_ypixel);

	(void)write(rem, obuf, sizeof(obuf));
}

/*
 * reader: read from remote: line -> 1
 */
#define	READING	1
#define	WRITING	2

static jmp_buf rcvtop;
static pid_t ppid;
static ssize_t rcvcnt, rcvstate;
static char rcvbuf[8 * 1024];

static void
oob(int signo)
{
	struct termios tty;
	int atmark;
	ssize_t n, rcvd;
	char waste[BUFSIZ], mark;

	rcvd = 0;
	while (recv(rem, &mark, 1, MSG_OOB) == -1) {
		switch (errno) {
		case EWOULDBLOCK:
			/*
			 * Urgent data not here yet.  It may not be possible
			 * to send it yet if we are blocked for output and
			 * our input buffer is full.
			 */
			if (rcvcnt < (ssize_t)sizeof(rcvbuf)) {
				n = read(rem, rcvbuf + rcvcnt,
				    sizeof(rcvbuf) - rcvcnt);
				if (n <= 0)
					return;
				rcvd += n;
			} else {
				n = read(rem, waste, sizeof(waste));
				if (n <= 0)
					return;
			}
			continue;
		default:
			return;
		}
	}
	if (mark & TIOCPKT_WINDOW) {
		/* Let server know about window size changes */
		(void)kill(ppid, SIGUSR1);
	}
	if (!eight && (mark & TIOCPKT_NOSTOP)) {
		(void)tcgetattr(0, &tty);
		tty.c_iflag &= ~IXON;
		(void)tcsetattr(0, TCSANOW, &tty);
	}
	if (!eight && (mark & TIOCPKT_DOSTOP)) {
		(void)tcgetattr(0, &tty);
		tty.c_iflag |= (deftty.c_iflag & IXON);
		(void)tcsetattr(0, TCSANOW, &tty);
	}
	if (mark & TIOCPKT_FLUSHWRITE) {
		(void)tcflush(1, TCIOFLUSH);
		for (;;) {
			if (ioctl(rem, SIOCATMARK, &atmark) < 0) {
				warn("ioctl SIOCATMARK (ignored)");
				break;
			}
			if (atmark)
				break;
			n = read(rem, waste, sizeof (waste));
			if (n <= 0)
				break;
		}
		/*
		 * Don't want any pending data to be output, so clear the recv
		 * buffer.  If we were hanging on a write when interrupted,
		 * don't want it to restart.  If we were reading, restart
		 * anyway.
		 */
		rcvcnt = 0;
		longjmp(rcvtop, 1);
	}

	/* oob does not do FLUSHREAD (alas!) */

	/*
	 * If we filled the receive buffer while a read was pending, longjmp
	 * to the top to restart appropriately.  Don't abort a pending write,
	 * however, or we won't know how much was written.
	 */
	if (rcvd && rcvstate == READING)
		longjmp(rcvtop, 1);
}

/* reader: read from remote: line -> 1 */
static int
reader(sigset_t *smask)
{
	pid_t pid;
	ssize_t n, remaining;
	char *bufp;
	struct sigaction sa;

	pid = getpid();		/* modern systems use positives for pid */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_IGN;
	(void)sigaction(SIGTTOU, &sa, NULL);
	sa.sa_handler = oob;
	(void)sigaction(SIGURG, &sa, NULL);
	ppid = getppid();
	(void)fcntl(rem, F_SETOWN, pid);
	(void)setjmp(rcvtop);
	(void)sigprocmask(SIG_SETMASK, smask, NULL);
	bufp = rcvbuf;
	for (;;) {
		while ((remaining = rcvcnt - (bufp - rcvbuf)) > 0) {
			rcvstate = WRITING;
			n = write(STDOUT_FILENO, bufp, remaining);
			if (n < 0) {
				if (errno != EINTR)
					return (-1);
				continue;
			}
			bufp += n;
		}
		bufp = rcvbuf;
		rcvcnt = 0;
		rcvstate = READING;

		rcvcnt = read(rem, rcvbuf, sizeof (rcvbuf));
		if (rcvcnt == 0)
			return (0);
		if (rcvcnt < 0) {
			if (errno == EINTR)
				continue;
			warn("read");
			return (-1);
		}
	}
}

static void
mode(int f)
{
	struct termios tty;

	switch (f) {
	case 0:
		(void)tcsetattr(0, TCSANOW, &deftty);
		break;
	case 1:
		(void)tcgetattr(0, &deftty);
		tty = deftty;
		/* This is loosely derived from sys/compat/tty_compat.c. */
		tty.c_lflag &= ~(ECHO|ICANON|ISIG|IEXTEN);
		tty.c_iflag &= ~ICRNL;
		tty.c_oflag &= ~OPOST;
		tty.c_cc[VMIN] = 1;
		tty.c_cc[VTIME] = 0;
		if (eight) {
			tty.c_iflag &= IXOFF;
			tty.c_cflag &= ~(CSIZE|PARENB);
			tty.c_cflag |= CS8;
		}
		(void)tcsetattr(0, TCSANOW, &tty);
		break;

	default:
		return;
	}
}

static void
lostpeer(int signo)
{
	struct sigaction sa;
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	(void)sigaction(SIGPIPE, &sa, NULL);
	msg("\aconnection closed.");
	done(1);
}

/* copy SIGURGs to the child process. */
static void
copytochild(int signo)
{

	(void)kill(child, SIGURG);
}

static void
msg(const char *str)
{

	(void)fprintf(stderr, "rlogin: %s\r\n", str);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-468dEn] [-e char] [-l username] [-p port] "
	    "[username@]host\n", getprogname());
	exit(1);
}

/*
 * The following routine provides compatibility (such as it is) between older
 * Suns and others.  Suns have only a `ttysize', so we convert it to a winsize.
 */
#ifdef OLDSUN
static int
get_window_size(int fd, struct winsize *wp)
{
	struct ttysize ts;
	int error;

	if ((error = ioctl(0, TIOCGSIZE, &ts)) != 0)
		return (error);
	wp->ws_row = ts.ts_lines;
	wp->ws_col = ts.ts_cols;
	wp->ws_xpixel = 0;
	wp->ws_ypixel = 0;
	return (0);
}
#endif

static u_int
getescape(char *p)
{
	long val;
	size_t len;

	if ((len = strlen(p)) == 1)	/* use any single char, including '\' */
		return ((u_int)*p);
					/* otherwise, \nnn */
	if (*p == '\\' && len >= 2 && len <= 4) {
		val = strtol(++p, NULL, 8);
		for (;;) {
			if (!*++p)
				return ((u_int)val);
			if (*p < '0' || *p > '8')
				break;
		}
	}
	msg("illegal option value -- e");
	usage();
	/* NOTREACHED */
	return (0);
}
