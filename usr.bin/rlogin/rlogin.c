/*	$NetBSD: rlogin.c,v 1.29 2003/08/07 11:15:41 agc Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rlogin.c	8.4 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: rlogin.c,v 1.29 2003/08/07 11:15:41 agc Exp $");
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
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

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

#ifdef KERBEROS
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#include <kerberosIV/kstream.h>

#include "krb.h"

CREDENTIALS cred;
Key_schedule schedule;
MSG_DAT msg_data;
struct sockaddr_in local, foreign;
int use_kerberos = 1, doencrypt;
kstream krem;
#endif

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

int eight, rem;
struct termios deftty;

int noescape;
u_char escapechar = '~';

#ifdef OLDSUN
struct winsize {
	unsigned short ws_row, ws_col;
	unsigned short ws_xpixel, ws_ypixel;
};
#else
#define	get_window_size(fd, wp)	ioctl(fd, TIOCGWINSZ, wp)
#endif
struct	winsize winsize;

void		catch_child(int);
void		copytochild(int);
void		doit(sigset_t *);
void		done(int);
void		echo(int);
u_int		getescape(char *);
void		lostpeer(int);
int		main(int, char **);
void		mode(int);
void		msg(char *);
void		oob(int);
int		reader(sigset_t *);
void		sendwindow(void);
void		setsignal(int);
int		speed(int);
void		sigwinch(int);
void		stop(int);
void		usage(void);
void		writer(void);
void		writeroob(int);

#ifdef	KERBEROS
void		warning(const char *, ...);
#endif
#ifdef OLDSUN
int		get_window_size(int, struct winsize *);
#endif

int
main(int argc, char *argv[])
{
	struct passwd *pw;
	struct servent *sp;
	struct termios tty;
	sigset_t smask;
	int argoff, ch, dflag, one, uid;
	int i, len, len2;
	char *host, *p, *user, *name, term[1024] = "network";
	speed_t ospeed;
	struct sigaction sa;
	char *service=NULL;
	struct rlimit rlim;
#ifdef KERBEROS
	KTEXT_ST ticket;
	int sock;
	long authopts;
	int through_once = 0;
	extern int _kstream_des_debug_OOB;
	char *dest_realm = NULL;
#endif

	argoff = dflag = 0;
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

#ifdef KERBEROS
#define	OPTIONS	"8EKLde:p:k:l:x"
#else
#define	OPTIONS	"8EKLde:p:l:"
#endif
	while ((ch = getopt(argc - argoff, argv + argoff, OPTIONS)) != -1)
		switch(ch) {
		case '8':
			eight = 1;
			break;
		case 'E':
			noescape = 1;
			break;
#ifdef KERBEROS
		case 'K':
			use_kerberos = 0;
			break;
#endif
		case 'd':
#ifdef KERBEROS
			_kstream_des_debug_OOB = 1;
#endif
			dflag = 1;
			break;
		case 'e':
			noescape = 0;
			escapechar = getescape(optarg);
			break;
#ifdef KERBEROS
		case 'k':
			dest_realm = optarg;
			break;
#endif
		case 'l':
			user = optarg;
			break;
		case 'p':
			/*HF*/
			service = optarg;
			sp = getservbyname(service, "tcp");
			if (sp == NULL) {       /* number given, no name */
				sp = malloc(sizeof(*sp));
				memset(sp, 0, sizeof(*sp));
				sp->s_name = service;
				sp->s_port = atoi(service);
				if (sp->s_port <= 0 || sp->s_port > IPPORT_ANONMAX)   
					errx(1,"port must be between 1 and %d", IPPORT_ANONMAX);
			}
			break;
#ifdef CRYPT
#ifdef KERBEROS
		case 'x':
			doencrypt = 1;
			break;
#endif
#endif
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

#ifdef KERBEROS
	if (use_kerberos) {
		if (sp == NULL) {
			sp = getservbyname((doencrypt ? "eklogin" : "klogin"), "tcp");
		}
		if (sp == NULL) {
			use_kerberos = 0;
			warning("can't get entry for %s/tcp service",
			    doencrypt ? "eklogin" : "klogin");
		}
	}
#endif
	if (sp == NULL)
		sp = getservbyname("login", "tcp");
	if (sp == NULL)
		errx(1, "login/tcp: unknown service.");

	if ((p = getenv("TERM")) != NULL)
		(void)strlcpy(term, p, sizeof(term));
	len = strlen(term);
	if (len < (sizeof(term) - 1) && tcgetattr(0, &tty) == 0) {
		/* start at 2 to include the / */
		for (ospeed = i = cfgetospeed(&tty), len2 = 2; i > 9; len2++)
			i /= 10;

		if (len + len2 < sizeof(term))
			(void)snprintf(term + len, len2 + 1, "/%d", ospeed);
	}

	(void)get_window_size(0, &winsize);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = lostpeer;
	(void)sigaction(SIGPIPE, &sa, (struct sigaction *)0);
	/* will use SIGUSR1 for window size hack, so hold it off */
	sigemptyset(&smask);
	sigaddset(&smask, SIGURG);
	sigaddset(&smask, SIGUSR1);
	(void)sigprocmask(SIG_SETMASK, &smask, &smask);
	/*
	 * We set SIGURG and SIGUSR1 below so that an
	 * incoming signal will be held pending rather than being
	 * discarded. Note that these routines will be ready to get
	 * a signal by the time that they are unblocked below.;
	 */
	sa.sa_handler = copytochild;
	(void)sigaction(SIGURG, &sa, (struct sigaction *) 0);
	sa.sa_handler = writeroob;
	(void)sigaction(SIGUSR1, &sa, (struct sigaction *) 0);
	
	/* don't dump core */
	rlim.rlim_cur = rlim.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &rlim) < 0)
		warn("setrlimit");

#ifdef KERBEROS
try_connect:
	if (use_kerberos) {
		struct hostent *hp;

		/* Fully qualify hostname (needed for krb_realmofhost). */
		hp = gethostbyname(host);
		if (hp != NULL && !(host = strdup(hp->h_name)))
			errx(1, "%s", strerror(ENOMEM));

		rem = KSUCCESS;
		errno = 0;
#ifdef CRYPT
		if (doencrypt)
			authopts = KOPT_DO_MUTUAL;
		else
#endif /* CRYPT */
			authopts = 0L;

		if (dest_realm == NULL) {
			/* default this now, once. */
			if (!(dest_realm = krb_realmofhost (host))) {
				warnx("Unknown realm for host %s.", host);
				use_kerberos = 0;
				if (service != NULL)
					sp = getservbyname("login", "tcp");
				goto try_connect;
			}
		}

		rem = kcmd(&sock, &host, sp->s_port, name, user, 
			   term, 0, &ticket, "rcmd", dest_realm,
			   &cred, schedule, &msg_data, &local, &foreign,
			   authopts);

		if (rem != KSUCCESS) {
			switch(rem) {

				case KDC_PR_UNKNOWN:
					warnx("Host %s not registered for %s",
				       	      host, "Kerberos rlogin service");
					use_kerberos = 0;
					if (service != NULL) 
						sp = getservbyname("login", "tcp");
					goto try_connect;
				case NO_TKT_FIL:
					if (through_once++) {
						use_kerberos = 0;
						if (service != NULL) 
							sp = getservbyname("login", "tcp");
						goto try_connect;
					}
#ifdef notyet
				krb_get_pw_in_tkt(user, krb_realm, "krbtgt",
						  krb_realm,
					          DEFAULT_TKT_LIFE/5, 0);
				goto try_connect;
#endif
			default:
				warnx("Kerberos rcmd failed: %s",
				      (rem == -1) ? "rcmd protocol failure" :
				      krb_err_txt[rem]);
				use_kerberos = 0;
				if (service != NULL) 
					sp = getservbyname("login", "tcp");
				goto try_connect;
			}
		}
		rem = sock;
		if (doencrypt)
			krem = kstream_create_rlogin_from_fd(rem, &schedule,
							     &cred.session);
		else
			krem = kstream_create_from_fd(rem, 0, 0);
			kstream_set_buffer_mode(krem, 0);
	} else {
#ifdef CRYPT
		if (doencrypt)
			errx(1, "the -x flag requires Kerberos authentication.");
#endif /* CRYPT */
		rem = rcmd_af(&host, sp->s_port, name, user, term, 0,
		    PF_UNSPEC);
		if (rem < 0)
			exit(1);
	}
#else
	rem = rcmd_af(&host, sp->s_port, name, user, term, 0, PF_UNSPEC);

#endif /* KERBEROS */

	if (rem < 0)
		exit(1);

	if (dflag &&
	    setsockopt(rem, SOL_SOCKET, SO_DEBUG, &one, sizeof(one)) < 0)
		warn("setsockopt DEBUG (ignored)");
    {
	struct sockaddr_storage ss;
	int sslen;
	sslen = sizeof(ss);
	if (getsockname(rem, (struct sockaddr *)&ss, &sslen) == 0
	 && ((struct sockaddr *)&ss)->sa_family == AF_INET) {
		one = IPTOS_LOWDELAY;
		if (setsockopt(rem, IPPROTO_IP, IP_TOS, (char *)&one,
				sizeof(int)) < 0) {
			warn("setsockopt TOS (ignored)");
		}
	}
    }

	(void)setuid(uid);
	doit(&smask);
	/*NOTREACHED*/
	return (0);
}

int
speed(int fd)
{
	struct termios tt;

	(void)tcgetattr(fd, &tt);

	return ((int)cfgetispeed(&tt));
}

pid_t child;
struct termios deftt;
struct termios nott;

void
doit(sigset_t *smask)
{
	int i;
	struct sigaction sa;

	for (i = 0; i < NCCS; i++)
		nott.c_cc[i] = _POSIX_VDISABLE;
	tcgetattr(0, &deftt);
	nott.c_cc[VSTART] = deftt.c_cc[VSTART];
	nott.c_cc[VSTOP] = deftt.c_cc[VSTOP];
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_IGN;
	(void)sigaction(SIGINT, &sa, (struct sigaction *) 0);
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
	(void)sigprocmask(SIG_SETMASK, smask, (sigset_t *) 0);
	sa.sa_handler = catch_child;
	(void)sigaction(SIGCHLD, &sa, (struct sigaction *) 0);
	writer();
	msg("closed connection.");
	done(0);
}

/* trap a signal, unless it is being ignored. */
void
setsignal(int sig)
{
	struct sigaction sa;
	sigset_t sigs;

	sigemptyset(&sigs);
	sigaddset(&sigs, sig);
	sigprocmask(SIG_BLOCK, &sigs, &sigs);

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = exit;
	sa.sa_flags = SA_RESTART;
	(void)sigaction(sig, &sa, &sa);
	if (sa.sa_handler == SIG_IGN)
		(void)sigaction(sig, &sa, (struct sigaction *) 0);

	(void)sigprocmask(SIG_SETMASK, &sigs, (sigset_t *) 0);
}

void
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
		(void)sigaction(SIGCHLD, &sa, (struct sigaction *) 0);
		if (kill(child, SIGKILL) >= 0)
			while ((w = wait(&wstatus)) > 0 && w != child)
				continue;
	}
	exit(status);
}

int dosigwinch;

/*
 * This is called when the reader process gets the out-of-band (urgent)
 * request to turn on the window-changing protocol.
 */
void
writeroob(int signo)
{
	struct sigaction sa;

	if (dosigwinch == 0) {
		sendwindow();
		sigemptyset(&sa.sa_mask);
		sa.sa_handler = sigwinch;
		sa.sa_flags = SA_RESTART;
		(void)sigaction(SIGWINCH, &sa, (struct sigaction *) 0);
	}
	dosigwinch = 1;
}

void
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
void
writer(void)
{
	int bol, local, n;
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
#ifdef KERBEROS
				if (use_kerberos)
					(void)kstream_write(krem,
					    (char *)&escapechar, 1);
				else
#endif
					(void)write(rem, &escapechar, 1);
			}
		}

#ifdef KERBEROS
		if (use_kerberos) {
			if (kstream_write(krem, &c, 1) == 0) {
					msg("line gone");
					break;
			}
		}
		else
#endif
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

void
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

void
stop(int all)
{
	struct sigaction sa;

	mode(0);
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	(void)sigaction(SIGCHLD, &sa, (struct sigaction *) 0);
	(void)kill(all ? 0 : getpid(), SIGTSTP);
	sa.sa_handler = catch_child;
	(void)sigaction(SIGCHLD, &sa, (struct sigaction *) 0);
	mode(1);
	sigwinch(0);			/* check for size changes */
}

void
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
void
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

#ifdef KERBEROS
		if (use_kerberos)
			(void)kstream_write(krem, obuf, sizeof(obuf));
		else
#endif
		(void)write(rem, obuf, sizeof(obuf));
}

/*
 * reader: read from remote: line -> 1
 */
#define	READING	1
#define	WRITING	2

jmp_buf rcvtop;
pid_t ppid;
int rcvcnt, rcvstate;
char rcvbuf[8 * 1024];

void
oob(int signo)
{
	struct termios tty;
	int atmark, n, rcvd;
	char waste[BUFSIZ], mark;

	rcvd = 0;
	while (recv(rem, &mark, 1, MSG_OOB) < 0) {
		switch (errno) {
		case EWOULDBLOCK:
			/*
			 * Urgent data not here yet.  It may not be possible
			 * to send it yet if we are blocked for output and
			 * our input buffer is full.
			 */
			if (rcvcnt < sizeof(rcvbuf)) {
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
int
reader(sigset_t *smask)
{
	pid_t pid;
	int n, remaining;
	char *bufp;
	struct sigaction sa;

	pid = getpid();		/* modern systems use positives for pid */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_IGN;
	(void)sigaction(SIGTTOU, &sa, (struct sigaction *) 0);
	sa.sa_handler = oob;
	(void)sigaction(SIGURG, &sa, (struct sigaction *) 0);
	ppid = getppid();
	(void)fcntl(rem, F_SETOWN, pid);
	(void)setjmp(rcvtop);
	(void)sigprocmask(SIG_SETMASK, smask, (sigset_t *) 0);
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

#ifdef KERBEROS
			if (use_kerberos)
				rcvcnt = kstream_read(krem, rcvbuf, sizeof(rcvbuf));
			else
#endif
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

void
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

void
lostpeer(int signo)
{
	struct sigaction sa;
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_IGN;
	(void)sigaction(SIGPIPE, &sa, (struct sigaction *)0);
	msg("\aconnection closed.");
	done(1);
}

/* copy SIGURGs to the child process. */
void
copytochild(int signo)
{

	(void)kill(child, SIGURG);
}

void
msg(char *str)
{

	(void)fprintf(stderr, "rlogin: %s\r\n", str);
}

#ifdef KERBEROS
/* VARARGS */
void
warning(const char *fmt, ...)
{
	va_list ap;

	(void)fprintf(stderr, "rlogin: warning, using standard rlogin: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, ".\n");
}
#endif

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: rlogin [ -%s]%s[-e char] [ -l username ] [-p port] [username@]host\n",
#ifdef KERBEROS
#ifdef CRYPT
	    "8EKLdx", " [-k realm] ");
#else
	    "8EKLd", " [-k realm] ");
#endif
#else
	    "8ELd", " ");
#endif
	exit(1);
}

/*
 * The following routine provides compatibility (such as it is) between older
 * Suns and others.  Suns have only a `ttysize', so we convert it to a winsize.
 */
#ifdef OLDSUN
int
get_window_size(fd, wp)
	int fd;
	struct winsize *wp;
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

u_int
getescape(char *p)
{
	long val;
	int len;

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
