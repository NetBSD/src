/*	$NetBSD: cmds.c,v 1.33.2.1 2012/04/17 00:09:40 yamt Exp $	*/

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
#if 0
static char sccsid[] = "@(#)cmds.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: cmds.c,v 1.33.2.1 2012/04/17 00:09:40 yamt Exp $");
#endif /* not lint */

#include "tip.h"
#include "pathnames.h"

/*
 * tip
 *
 * miscellaneous commands
 */

int	quant[] = { 60, 60, 24 };

char	null = '\0';
const char	*sep[] = { "second", "minute", "hour" };
static	char *argv[10];		/* argument vector for take and put */

int	args(char *, char **);
int	anyof(char *, const char *);
void	execute(char *);
__dead static void	intcopy(int);
void	prtime(const char *, time_t);
void	stopsnd(int);
void	transfer(char *, int, const char *);
void	transmit(FILE *, const char *, char *);

/*
 * FTP - remote ==> local
 *  get a file from the remote host
 */
void
getfl(char c)
{
	char buf[256], *cp;

	(void)putchar(c);
	/*
	 * get the UNIX receiving file's name
	 */
	if (prompt("Local file name? ", copyname, sizeof copyname))
		return;
	cp = expand(copyname);
	if ((sfd = open(cp, O_RDWR|O_CREAT, 0666)) < 0) {
		(void)printf("\r\n%s: cannot create\r\n", copyname);
		return;
	}

	/*
	 * collect parameters
	 */
	if (prompt("List command for remote system? ", buf,
	    sizeof buf)) {
		(void)unlink(copyname);
		return;
	}
	transfer(buf, sfd, value(EOFREAD));
}

/*
 * Cu-like take command
 */
/* ARGSUSED */
void
cu_take(char dummy __unused)
{
	int fd, argc;
	char line[BUFSIZ], *cp;

	if (prompt("[take] ", copyname, sizeof copyname))
		return;
	if ((argc = args(copyname, argv)) < 1 || argc > 2) {
		(void)printf("usage: <take> from [to]\r\n");
		return;
	}
	if (argc == 1)
		argv[1] = argv[0];
	cp = expand(argv[1]);
	if ((fd = open(cp, O_RDWR|O_CREAT, 0666)) < 0) {
		(void)printf("\r\n%s: cannot create\r\n", argv[1]);
		return;
	}
	(void)snprintf(line, sizeof line, "cat %s;echo \01", argv[0]);
	transfer(line, fd, "\01");
}

static	jmp_buf intbuf;
/*
 * Bulk transfer routine --
 *  used by getfl(), cu_take(), and pipefile()
 */
void
transfer(char *buf, int fd, const char *eofchars)
{
	int ct;
	char c, buffer[BUFSIZ];
	char * volatile p;
	int cnt, eof;
	time_t start;
	sig_t f;
	char r;

	p = buffer;
	xpwrite(FD, buf, strlen(buf));
	quit = 0;
	(void)write(attndes[1], "W", 1);	/* Put TIPOUT into a wait state */
	(void)read(repdes[0], (char *)&ccc, 1);  /* Wait until read process stops */

	/*
	 * finish command
	 */
	r = '\r';
	xpwrite(FD, &r, 1);
	do
		(void)read(FD, &c, 1);
	while ((c&STRIP_PAR) != '\n');
	(void)tcsetattr(0, TCSAFLUSH, &defchars);

	(void) setjmp(intbuf);
	f = signal(SIGINT, intcopy);
	start = time(0);
	for (ct = 0; !quit;) {
		eof = read(FD, &c, 1) <= 0;
		c &= STRIP_PAR;
		if (quit)
			continue;
		if (eof || any(c, eofchars))
			break;
		if (c == 0)
			continue;	/* ignore nulls */
		if (c == '\r')
			continue;
		*p++ = c;

		if (c == '\n' && boolean(value(VERBOSE)))
			(void)printf("\r%d", ++ct);
		if ((cnt = (p-buffer)) == number(value(FRAMESIZE))) {
			if (write(fd, buffer, (size_t)cnt) != cnt) {
				(void)printf("\r\nwrite error\r\n");
				quit = 1;
			}
			p = buffer;
		}
	}
	if ((cnt = (p-buffer)) != 0)
		if (write(fd, buffer, (size_t)cnt) != cnt)
			(void)printf("\r\nwrite error\r\n");

	if (boolean(value(VERBOSE)))
		prtime(" lines transferred in ", time(0)-start);
	(void)tcsetattr(0, TCSAFLUSH, &term);
	(void)write(fildes[1], (char *)&ccc, 1);
	(void)signal(SIGINT, f);
	(void)close(fd);
}

/*
 * FTP - remote ==> local process
 *   send remote input to local process via pipe
 */
/* ARGSUSED */
void
pipefile(char dummy __unused)
{
	int cpid, pdes[2];
	char buf[256];
	int status, p;

	if (prompt("Local command? ", buf, sizeof buf))
		return;

	if (pipe(pdes)) {
		(void)printf("can't establish pipe\r\n");
		return;
	}

	if ((cpid = fork()) < 0) {
		(void)printf("can't fork!\r\n");
		return;
	} else if (cpid) {
		if (prompt("List command for remote system? ", buf,
		    sizeof buf)) {
			(void)close(pdes[0]);
			(void)close(pdes[1]);
			(void)kill(cpid, SIGKILL);
		} else {
			(void)close(pdes[0]);
			(void)signal(SIGPIPE, intcopy);
			transfer(buf, pdes[1], value(EOFREAD));
			(void)signal(SIGPIPE, SIG_DFL);
			while ((p = wait(&status)) > 0 && p != cpid)
				;
		}
	} else {
		(void)dup2(pdes[0], 0);
		(void)close(pdes[0]);
		(void)closefrom(3);
		execute(buf);
		(void)printf("can't execl!\r\n");
		exit(0);
	}
}

/*
 * Interrupt service routine for FTP
 */
/* ARGSUSED */
void
stopsnd(int dummy __unused)
{

	stop = 1;
	(void)signal(SIGINT, SIG_IGN);
}

/*
 * FTP - local ==> remote
 *  send local file to remote host
 *  terminate transmission with pseudo EOF sequence
 */
void
sendfile(char cc)
{
	FILE *fd;
	char *fnamex;

	(void)putchar(cc);
	/*
	 * get file name
	 */
	if (prompt("Local file name? ", fname, sizeof fname))
		return;

	/*
	 * look up file
	 */
	fnamex = expand(fname);
	if ((fd = fopen(fnamex, "r")) == NULL) {
		(void)printf("%s: cannot open\r\n", fname);
		return;
	}
	transmit(fd, value(EOFWRITE), NULL);
	if (!boolean(value(ECHOCHECK)))
		(void)tcdrain(FD);
}

/*
 * Bulk transfer routine to remote host --
 *   used by sendfile() and cu_put()
 */
void
transmit(FILE *fd, const char *eofchars, char *command)
{
	const char *pc;
	char lastc;
	int c;
	int ccount, lcount;
	time_t start_t, stop_t;
	sig_t f;

	(void)write(attndes[1], "W", 1);	/* put TIPOUT into a wait state */
	stop = 0;
	f = signal(SIGINT, stopsnd);
	(void)tcsetattr(0, TCSAFLUSH, &defchars);
	(void)read(repdes[0], (char *)&ccc, 1);
	if (command != NULL) {
		for (pc = command; *pc; pc++)
			sendchar(*pc);
		if (boolean(value(ECHOCHECK)))
			(void)read(FD, &c, (size_t)1);	/* trailing \n */
		else {
			(void)tcdrain(FD);
			(void)sleep(5); /* wait for remote stty to take effect */
		}
	}
	lcount = 0;
	lastc = '\0';
	start_t = time(0);
	/* CONSTCOND */
	while (1) {
		ccount = 0;
		do {
			c = getc(fd);
			if (stop)
				goto out;
			if (c == EOF)
				goto out;
			if (c == 0177 && !boolean(value(RAWFTP)))
				continue;
			lastc = c;
			if (c < 040) {
				if (c == '\n') {
					if (!boolean(value(RAWFTP)))
						c = '\r';
				}
				else if (c == '\t') {
					if (!boolean(value(RAWFTP))) {
						if (boolean(value(TABEXPAND))) {
							sendchar(' ');
							while ((++ccount % 8) != 0)
								sendchar(' ');
							continue;
						}
					}
				} else
					if (!boolean(value(RAWFTP)))
						continue;
			}
			sendchar(c);
		} while (c != '\r' && !boolean(value(RAWFTP)));
		if (boolean(value(VERBOSE)))
			(void)printf("\r%d", ++lcount);
		if (boolean(value(ECHOCHECK))) {
			timedout = 0;
			(void)alarm((unsigned int)number(value(ETIMEOUT)));
			do {	/* wait for prompt */
				(void)read(FD, &c, (size_t)1);
				if (timedout || stop) {
					if (timedout)
						(void)printf(
						    "\r\ntimed out at eol\r\n");
					(void)alarm(0);
					goto out;
				}
			} while ((c&STRIP_PAR) != character(value(PROMPT)));
			(void)alarm(0);
		}
	}
out:
	if (lastc != '\n' && !boolean(value(RAWFTP)))
		sendchar('\r');
	if (eofchars) {
		for (pc = eofchars; *pc; pc++)
			sendchar(*pc);
	}
	stop_t = time(0);
	(void)fclose(fd);
	(void)signal(SIGINT, f);
	if (boolean(value(VERBOSE))) {
		if (boolean(value(RAWFTP)))
			prtime(" chars transferred in ", stop_t-start_t);
		else
			prtime(" lines transferred in ", stop_t-start_t);
	}
	(void)write(fildes[1], (char *)&ccc, 1);
	(void)tcsetattr(0, TCSAFLUSH, &term);
}

/*
 * Cu-like put command
 */
/* ARGSUSED */
void
cu_put(char dummy __unused)
{
	FILE *fd;
	char line[BUFSIZ];
	int argc;
	char *copynamex;

	if (prompt("[put] ", copyname, sizeof copyname))
		return;
	if ((argc = args(copyname, argv)) < 1 || argc > 2) {
		(void)printf("usage: <put> from [to]\r\n");
		return;
	}
	if (argc == 1)
		argv[1] = argv[0];
	copynamex = expand(argv[0]);
	if ((fd = fopen(copynamex, "r")) == NULL) {
		(void)printf("%s: cannot open\r\n", copynamex);
		return;
	}
	if (boolean(value(ECHOCHECK)))
		(void)snprintf(line, sizeof line, "cat>%s\r", argv[1]);
	else
		(void)snprintf(line, sizeof line, "stty -echo;cat>%s;stty echo\r", argv[1]);
	transmit(fd, "\04", line);
}

/*
 * FTP - send single character
 *  wait for echo & handle timeout
 */
void
sendchar(char c)
{
	char cc;
	int retry = 0;

	cc = c;
	xpwrite(FD, &cc, 1);
#ifdef notdef
	if (number(value(CDELAY)) > 0 && c != '\r')
		nap(number(value(CDELAY)));
#endif
	if (!boolean(value(ECHOCHECK))) {
#ifdef notdef
		if (number(value(LDELAY)) > 0 && c == '\r')
			nap(number(value(LDELAY)));
#endif
		return;
	}
tryagain:
	timedout = 0;
	(void)alarm((unsigned int)number(value(ETIMEOUT)));
	(void)read(FD, &cc, 1);
	(void)alarm(0);
	if (timedout) {
		(void)printf("\r\ntimeout error (%s)\r\n", ctrl(c));
		if (retry++ > 3)
			return;
		xpwrite(FD, &null, 1); /* poke it */
		goto tryagain;
	}
}

/* ARGSUSED */
void
alrmtimeout(int dummy __unused)
{

	(void)signal(SIGALRM, alrmtimeout);
	timedout = 1;
}

/*
 * Stolen from consh() -- puts a remote file on the output of a local command.
 *	Identical to consh() except for where stdout goes.
 */
void
pipeout(char c)
{
	char buf[256];
	int cpid, status, p;
	time_t start = 0;

	(void)putchar(c);
	if (prompt("Local command? ", buf, sizeof buf))
		return;
	(void)write(attndes[1], "W", 1);	/* put TIPOUT into a wait state */
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)tcsetattr(0, TCSAFLUSH, &defchars);
	(void)read(repdes[0], (char *)&ccc, 1);
	/*
	 * Set up file descriptors in the child and
	 *  let it go...
	 */
	if ((cpid = fork()) < 0)
		(void)printf("can't fork!\r\n");
	else if (cpid) {
		start = time(0);
		while ((p = wait(&status)) > 0 && p != cpid)
			;
	} else {
		(void)dup2(FD, 1);
		(void)closefrom(3);
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		execute(buf);
		(void)printf("can't find `%s'\r\n", buf);
		exit(0);
	}
	if (boolean(value(VERBOSE)))
		prtime("away for ", time(0)-start);
	(void)write(fildes[1], (char *)&ccc, 1);
	(void)tcsetattr(0, TCSAFLUSH, &term);
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
}

/*
 * Fork a program with:
 *  0 <-> remote tty in
 *  1 <-> remote tty out
 *  2 <-> local tty out
 */
void
consh(char c)
{
	char buf[256];
	int cpid, status, p;
	time_t start = 0;

	(void)putchar(c);
	if (prompt("Local command? ", buf, sizeof buf))
		return;
	(void)write(attndes[1], "W", 1);	/* put TIPOUT into a wait state */
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)tcsetattr(0, TCSAFLUSH, &defchars);
	(void)read(repdes[0], (char *)&ccc, 1);
	/*
	 * Set up file descriptors in the child and
	 *  let it go...
	 */
	if ((cpid = fork()) < 0)
		(void)printf("can't fork!\r\n");
	else if (cpid) {
		start = time(0);
		while ((p = wait(&status)) > 0 && p != cpid)
			;
	} else {
		(void)dup2(FD, 0);
		(void)dup2(FD, 1);
		(void)closefrom(3);
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		execute(buf);
		(void)printf("can't find `%s'\r\n", buf);
		exit(0);
	}
	if (boolean(value(VERBOSE)))
		prtime("away for ", time(0)-start);
	(void)write(fildes[1], (char *)&ccc, 1);
	(void)tcsetattr(0, TCSAFLUSH, &term);
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
}

/*
 * Escape to local shell
 */
/* ARGSUSED */
void
shell(char dummy __unused)
{
	int shpid, status;
	const char *cp;

	(void)printf("[sh]\r\n");
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	unraw();
	switch (shpid = fork()) {
	default:
		while (shpid != wait(&status));
		raw();
		(void)printf("\r\n!\r\n");
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		break;
	case 0:
		(void)signal(SIGQUIT, SIG_DFL);
		(void)signal(SIGINT, SIG_DFL);
		if ((cp = strrchr(value(SHELL), '/')) == NULL)
			cp = value(SHELL);
		else
			cp++;
		(void)execl(value(SHELL), cp, NULL);
		(void)fprintf(stderr, "\r\n");
		err(1, "can't execl");
		/* NOTREACHED */
	case -1:
		(void)fprintf(stderr, "\r\n");
		err(1, "can't fork");
		/* NOTREACHED */
	}
}

/*
 * TIPIN portion of scripting
 *   initiate the conversation with TIPOUT
 */
void
setscript(void)
{
	char c;
	/*
	 * enable TIPOUT side for dialogue
	 */
	(void)write(attndes[1], "S", 1);
	if (boolean(value(SCRIPT)) && strlen(value(RECORD)))
		(void)write(fildes[1], value(RECORD), strlen(value(RECORD)));
	(void)write(fildes[1], "\n", 1);
	/*
	 * wait for TIPOUT to finish
	 */
	(void)read(repdes[0], &c, 1);
	if (c == 'n')
		(void)printf("can't create %s\r\n", (char *)value(RECORD));
}

/*
 * Change current working directory of
 *   local portion of tip
 */
/* ARGSUSED */
void
chdirectory(char dummy __unused)
{
	char dirnam[80];
	const char *cp = dirnam;

	if (prompt("[cd] ", dirnam, sizeof dirnam)) {
		if (stoprompt)
			return;
		cp = value(HOME);
	}
	if (chdir(cp) < 0)
		(void)printf("%s: bad directory\r\n", cp);
	(void)printf("!\r\n");
}

void
tipabort(const char *msg)
{

	(void)kill(pid, SIGTERM);
	disconnect(msg);
	if (msg != NULL)
		(void)printf("\r\n%s", msg);
	(void)printf("\r\n[EOT]\r\n");
	unraw();
	exit(0);
}

/* ARGSUSED */
void
finish(char dummy __unused)
{
	const char *dismsg;

	dismsg = value(DISCONNECT);
	if (dismsg != NULL && dismsg[0] != '\0') {
		(void)write(FD, dismsg, strlen(dismsg));
		(void)sleep(5);
	}
	tipabort(NULL);
}

/* ARGSUSED */
void
intcopy(int dummy __unused)
{

	raw();
	quit = 1;
	longjmp(intbuf, 1);
}

void
execute(char *s)
{
	const char *cp;

	if ((cp = strrchr(value(SHELL), '/')) == NULL)
		cp = value(SHELL);
	else
		cp++;
	(void)execl(value(SHELL), cp, "-c", s, NULL);
}

int
args(char *buf, char *a[])
{
	char *p = buf, *start;
	char **parg = a;
	int n = 0;

	do {
		while (*p && (*p == ' ' || *p == '\t'))
			p++;
		start = p;
		if (*p)
			*parg = p;
		while (*p && (*p != ' ' && *p != '\t'))
			p++;
		if (p != start)
			parg++, n++;
		if (*p)
			*p++ = '\0';
	} while (*p);

	return(n);
}

void
prtime(const char *s, time_t a)
{
	int i;
	int nums[3];

	for (i = 0; i < 3; i++) {
		nums[i] = (int)(a % quant[i]);
		a /= quant[i];
	}
	(void)printf("%s", s);
	while (--i >= 0)
		if (nums[i] || (i == 0 && nums[1] == 0 && nums[2] == 0))
			(void)printf("%d %s%c ", nums[i], sep[i],
				nums[i] == 1 ? '\0' : 's');
	(void)printf("\r\n!\r\n");
}

/* ARGSUSED */
void
variable(char dummy __unused)
{
	char	buf[256];

	if (prompt("[set] ", buf, sizeof buf))
		return;
	vlex(buf);
	if (vtable[BEAUTIFY].v_access&CHANGED) {
		vtable[BEAUTIFY].v_access &= ~CHANGED;
		(void)write(attndes[1], "B", 1);	/* Tell TIPOUT to toggle */
	}
	if (vtable[SCRIPT].v_access&CHANGED) {
		vtable[SCRIPT].v_access &= ~CHANGED;
		setscript();
		/*
		 * So that "set record=blah script" doesn't
		 *  cause two transactions to occur.
		 */
		if (vtable[RECORD].v_access&CHANGED)
			vtable[RECORD].v_access &= ~CHANGED;
	}
	if (vtable[RECORD].v_access&CHANGED) {
		vtable[RECORD].v_access &= ~CHANGED;
		if (boolean(value(SCRIPT)))
			setscript();
	}
	if (vtable[TAND].v_access&CHANGED) {
		vtable[TAND].v_access &= ~CHANGED;
		if (boolean(value(TAND)))
			tandem("on");
		else
			tandem("off");
	}
 	if (vtable[LECHO].v_access&CHANGED) {
 		vtable[LECHO].v_access &= ~CHANGED;
 		HD = boolean(value(LECHO));
 	}
	if (vtable[PARITY].v_access&CHANGED) {
		vtable[PARITY].v_access &= ~CHANGED;
		setparity(NULL);	/* XXX what is the correct arg? */
	}
	if (vtable[HARDWAREFLOW].v_access&CHANGED) {
		vtable[HARDWAREFLOW].v_access &= ~CHANGED;
		if (boolean(value(HARDWAREFLOW)))
			hardwareflow("on");
		else
			hardwareflow("off");
	}
}

/*
 * Turn tandem mode on or off for remote tty.
 */
void
tandem(const char *option)
{
	struct termios	rmtty;

	(void)tcgetattr(FD, &rmtty);
	if (strcmp(option, "on") == 0) {
		rmtty.c_iflag |= IXOFF;
		term.c_iflag |= IXOFF;
	} else {
		rmtty.c_iflag &= ~IXOFF;
		term.c_iflag &= ~IXOFF;
	}
	(void)tcsetattr(FD, TCSADRAIN, &rmtty);
	(void)tcsetattr(0, TCSADRAIN, &term);
}

/*
 * Turn hardware flow control on or off for remote tty.
 */
void
hardwareflow(const char *option)
{
	struct termios	rmtty;

	(void)tcgetattr(FD, &rmtty);
	if (strcmp(option, "on") == 0)
		rmtty.c_cflag |= CRTSCTS;
	else
		rmtty.c_cflag &= ~CRTSCTS;
	(void)tcsetattr(FD, TCSADRAIN, &rmtty);
}

/*
 * Send a break.
 */
/* ARGSUSED */
void
genbrk(char dummy __unused)
{

	(void)ioctl(FD, TIOCSBRK, NULL);
	(void)sleep(1);
	(void)ioctl(FD, TIOCCBRK, NULL);
}

/*
 * Suspend tip
 */
void
suspend(char c)
{

	unraw();
	(void)kill(c == CTRL('y') ? getpid() : 0, SIGTSTP);
	raw();
}

/*
 *	expand a file name if it includes shell meta characters
 */

char *
expand(char aname[])
{
	static char xname[BUFSIZ];
	char * volatile name;
	char cmdbuf[BUFSIZ];
	int mypid, l;
	char *cp;
	const char *Shell;
	int s, pivec[2];

	name = aname;
	if (!anyof(name, "~{[*?$`'\"\\"))
		return(name);
	if (pipe(pivec) < 0) {
		warn("pipe");
		return(name);
	}
	(void)snprintf(cmdbuf, sizeof cmdbuf, "echo %s", name);
	if ((mypid = vfork()) == 0) {
		Shell = value(SHELL);
		if (Shell == NULL)
			Shell = _PATH_BSHELL;
		(void)close(pivec[0]);
		(void)close(1);
		(void)dup(pivec[1]);
		(void)close(pivec[1]);
		(void)close(2);
		(void)execl(Shell, Shell, "-c", cmdbuf, NULL);
		_exit(1);
	}
	if (mypid == -1) {
		warn("fork");
		(void)close(pivec[0]);
		(void)close(pivec[1]);
		return(NULL);
	}
	(void)close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	(void)close(pivec[0]);
	while (wait(&s) != mypid)
		continue;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		(void)fprintf(stderr, "\"Echo\" failed\n");
		return(NULL);
	}
	if (l < 0) {
		warn("read");
		return(NULL);
	}
	if (l == 0) {
		(void)fprintf(stderr, "\"%s\": No match\n", name);
		return(NULL);
	}
	if (l == BUFSIZ) {
		(void)fprintf(stderr, "Buffer overflow expanding \"%s\"\n", name);
		return(NULL);
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	return(xname);
}

/*
 * Are any of the characters in the two strings the same?
 */

int
anyof(char *s1, const char *s2)
{
	int c;

	while ((c = *s1++) != '\0')
		if (any(c, s2))
			return(1);
	return(0);
}
