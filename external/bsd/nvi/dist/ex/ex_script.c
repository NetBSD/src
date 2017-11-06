/*	$NetBSD: ex_script.c,v 1.8 2017/11/06 03:27:34 rin Exp $ */
/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Brian Hirt.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#include <sys/cdefs.h>
#if 0
#ifndef lint
static const char sccsid[] = "Id: ex_script.c,v 10.38 2001/06/25 15:19:19 skimo Exp  (Berkeley) Date: 2001/06/25 15:19:19 ";
#endif /* not lint */
#else
__RCSID("$NetBSD: ex_script.c,v 1.8 2017/11/06 03:27:34 rin Exp $");
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/stat.h>
#if defined(HAVE_SYS5_PTY)
#include <sys/stropts.h>
#endif
#include <sys/time.h>
#include <sys/wait.h>

#include <bitstring.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>		/* XXX: OSF/1 bug: include before <grp.h> */
#include <grp.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#ifdef HAVE_UTIL_H
#include <util.h>
#endif

#include "../common/common.h"
#include "../vi/vi.h"
#include "script.h"
#include "pathnames.h"

static void	sscr_check __P((SCR *));
static int	sscr_getprompt __P((SCR *));
static int	sscr_init __P((SCR *));
static int	sscr_insert __P((SCR *));
#ifdef HAVE_OPENPTY
#define	sscr_pty openpty
#else
static int	sscr_pty __P((int *, int *, char *, struct termios *, void *));
#endif
static int	sscr_setprompt __P((SCR *, char *, size_t));

/*
 * ex_script -- : sc[ript][!] [file]
 *	Switch to script mode.
 *
 * PUBLIC: int ex_script __P((SCR *, EXCMD *));
 */
int
ex_script(SCR *sp, EXCMD *cmdp)
{
	/* Vi only command. */
	if (!F_ISSET(sp, SC_VI)) {
		msgq(sp, M_ERR,
		    "150|The script command is only available in vi mode");
		return (1);
	}

	/* Avoid double run. */
	if (F_ISSET(sp, SC_SCRIPT)) {
		msgq(sp, M_ERR,
		    "The script command is already runninng");
		return (1);
	}

	/* We're going to need a shell. */
	if (opts_empty(sp, O_SHELL, 0))
		return (1);

	/* Switch to the new file. */
	if (cmdp->argc != 0 && ex_edit(sp, cmdp))
		return (1);

	/* Create the shell, figure out the prompt. */
	if (sscr_init(sp))
		return (1);

	return (0);
}

/*
 * sscr_init --
 *	Create a pty setup for a shell.
 */
static int
sscr_init(SCR *sp)
{
	SCRIPT *sc;
	const char *sh, *sh_path;

	MALLOC_RET(sp, sc, SCRIPT *, sizeof(SCRIPT));
	sp->script = sc;
	sc->sh_prompt = NULL;
	sc->sh_prompt_len = 0;

	/*
	 * There are two different processes running through this code.
	 * They are the shell and the parent.
	 */
	sc->sh_master = sc->sh_slave = -1;

	if (tcgetattr(STDIN_FILENO, &sc->sh_term) == -1) {
		msgq(sp, M_SYSERR, "tcgetattr");
		goto err;
	}

	/*
	 * Turn off output postprocessing and echo.
	 */
	sc->sh_term.c_oflag &= ~OPOST;
	sc->sh_term.c_cflag &= ~(ECHO|ECHOE|ECHONL|ECHOK);

#ifdef TIOCGWINSZ
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &sc->sh_win) == -1) {
		msgq(sp, M_SYSERR, "tcgetattr");
		goto err;
	}

	if (sscr_pty(&sc->sh_master,
	    &sc->sh_slave, sc->sh_name, &sc->sh_term, &sc->sh_win) == -1) {
		msgq(sp, M_SYSERR, "pty");
		goto err;
	}
#else
	if (sscr_pty(&sc->sh_master,
	    &sc->sh_slave, sc->sh_name, &sc->sh_term, NULL) == -1) {
		msgq(sp, M_SYSERR, "pty");
		goto err;
	}
#endif

	/*
	 * __TK__ huh?
	 * Don't use vfork() here, because the signal semantics differ from
	 * implementation to implementation.
	 */
	switch (sc->sh_pid = fork()) {
	case -1:			/* Error. */
		msgq(sp, M_SYSERR, "fork");
err:		if (sc->sh_master != -1)
			(void)close(sc->sh_master);
		if (sc->sh_slave != -1)
			(void)close(sc->sh_slave);
		return (1);
	case 0:				/* Utility. */
		/*
		 * XXX
		 * So that shells that do command line editing turn it off.
		 */
		(void)setenv("TERM", "emacs", 1);
		(void)setenv("TERMCAP", "emacs:", 1);
		(void)setenv("EMACS", "t", 1);

		(void)setsid();
#ifdef TIOCSCTTY
		/*
		 * 4.4BSD allocates a controlling terminal using the TIOCSCTTY
		 * ioctl, not by opening a terminal device file.  POSIX 1003.1
		 * doesn't define a portable way to do this.  If TIOCSCTTY is
		 * not available, hope that the open does it.
		 */
		(void)ioctl(sc->sh_slave, TIOCSCTTY, 0);
#endif
		(void)close(sc->sh_master);
		(void)dup2(sc->sh_slave, STDIN_FILENO);
		(void)dup2(sc->sh_slave, STDOUT_FILENO);
		(void)dup2(sc->sh_slave, STDERR_FILENO);
		(void)close(sc->sh_slave);

		/* Assumes that all shells have -i. */
		sh_path = O_STR(sp, O_SHELL);
		if ((sh = strrchr(sh_path, '/')) == NULL)
			sh = sh_path;
		else
			++sh;
		execl(sh_path, sh, "-i", NULL);
		msgq_str(sp, M_SYSERR, sh_path, "execl: %s");
		_exit(127);
	default:			/* Parent. */
		break;
	}

	if (sscr_getprompt(sp))
		return (1);

	F_SET(sp, SC_SCRIPT);
	F_SET(sp->gp, G_SCRWIN);
	return (0);
}

/*
 * sscr_getprompt --
 *	Eat lines printed by the shell until a line with no trailing
 *	carriage return comes; set the prompt from that line.
 */
static int
sscr_getprompt(SCR *sp)
{
	struct timeval tv;
	fd_set fdset;
	int master;

	/* Wait up to a second for characters to read. */
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	master = sp->script->sh_master;
	FD_ZERO(&fdset);
	FD_SET(master, &fdset);
	switch (select(master + 1, &fdset, NULL, NULL, &tv)) {
	case -1:		/* Error or interrupt. */
		msgq(sp, M_SYSERR, "select");
		break;
	case  0:		/* Timeout */
		msgq(sp, M_ERR, "Error: timed out");
		break;
	case  1:		/* Characters to read. */
		return (sscr_insert(sp) || sp->script == NULL);
	}

	sscr_end(sp);
	return (1);
}

/*
 * sscr_exec --
 *	Take a line and hand it off to the shell.
 *
 * PUBLIC: int sscr_exec __P((SCR *, db_recno_t));
 */
int
sscr_exec(SCR *sp, db_recno_t lno)
{
	SCRIPT *sc;
	db_recno_t last_lno;
	size_t blen, len, last_len;
	int isempty, matchprompt, rval;
	ssize_t nw;
	char *bp = NULL;
	const char *p;
	const CHAR_T *ip;
	size_t ilen;

	sc = sp->script;

	/* If there's a prompt on the last line, append the command. */
	if (db_last(sp, &last_lno))
		return (1);
	if (db_get(sp, last_lno, DBG_FATAL, __UNCONST(&ip), &ilen))
		return (1);
	INT2CHAR(sp, ip, ilen, p, last_len);
	if (last_len == sc->sh_prompt_len &&
	    memcmp(p, sc->sh_prompt, last_len) == 0) {
		matchprompt = 1;
		GET_SPACE_RETC(sp, bp, blen, last_len + 128);
		memmove(bp, p, last_len);
	} else
		matchprompt = 0;

	/* Get something to execute. */
	if (db_eget(sp, lno, __UNCONST(&ip), &ilen, &isempty)) {
		if (isempty)
			goto empty;
		goto err1;
	}

	/* Empty lines aren't interesting. */
	if (ilen == 0)
		goto empty;
	INT2CHAR(sp, ip, ilen, p, len);

	/* Delete any prompt. */
	if (len >= sc->sh_prompt_len &&
	    memcmp(p, sc->sh_prompt, sc->sh_prompt_len) == 0) {
		len -= sc->sh_prompt_len;
		if (len == 0) {
empty:			msgq(sp, M_BERR, "151|No command to execute");
			goto err1;
		}
		p += sc->sh_prompt_len;
	}

	/* Push the line to the shell. */
	if ((size_t)(nw = write(sc->sh_master, p, len)) != len)
		goto err2;
	rval = 0;
	if (write(sc->sh_master, "\n", 1) != 1) {
err2:		if (nw == 0)
			errno = EIO;
		msgq(sp, M_SYSERR, "shell");
		goto err1;
	}

	if (matchprompt) {
		ADD_SPACE_GOTO(sp, char, bp, blen, last_len + len);
		memmove(bp + last_len, p, len);
		CHAR2INT(sp, bp, last_len + len, ip, ilen);
		if (db_set(sp, last_lno, ip, ilen))
err1:			rval = 1;
	}
	if (matchprompt)
alloc_err:	FREE_SPACE(sp, bp, blen);
	return (rval);
}

/*
 * sscr_check_input -
 *	Check whether any input from shell or passed set.
 *
 * PUBLIC: int sscr_check_input __P((SCR *sp, fd_set *rdfd, int maxfd));
 */
int
sscr_check_input(SCR *sp, fd_set *fdset, int maxfd)
{
	fd_set rdfd;
	SCR *tsp;
	WIN *wp;

	wp = sp->wp;

loop:	memcpy(&rdfd, fdset, sizeof(fd_set));

	TAILQ_FOREACH(tsp, &wp->scrq, q)
		if (F_ISSET(sp, SC_SCRIPT)) {
			FD_SET(sp->script->sh_master, &rdfd);
			if (sp->script->sh_master > maxfd)
				maxfd = sp->script->sh_master;
		}
	switch (select(maxfd + 1, &rdfd, NULL, NULL, NULL)) {
	case 0:
		abort();
	case -1:
		return 1;
	default:
		break;
	}
	TAILQ_FOREACH(tsp, &wp->scrq, q)
		if (F_ISSET(sp, SC_SCRIPT) &&
		    FD_ISSET(sp->script->sh_master, &rdfd)) {
			if (sscr_input(sp))
				return 1;
			goto loop;
		}
	return 0;
}

/*
 * sscr_input --
 *	Read any waiting shell input.
 *
 * PUBLIC: int sscr_input __P((SCR *));
 */
int
sscr_input(SCR *sp)
{
	WIN *wp;
	struct timeval poll;
	fd_set rdfd;
	int maxfd;

	wp = sp->wp;

loop:	maxfd = 0;
	FD_ZERO(&rdfd);
	poll.tv_sec = 0;
	poll.tv_usec = 0;

	/* Set up the input mask. */
	TAILQ_FOREACH(sp, &wp->scrq, q)
		if (F_ISSET(sp, SC_SCRIPT)) {
			FD_SET(sp->script->sh_master, &rdfd);
			if (sp->script->sh_master > maxfd)
				maxfd = sp->script->sh_master;
		}

	/* Check for input. */
	switch (select(maxfd + 1, &rdfd, NULL, NULL, &poll)) {
	case -1:
		msgq(sp, M_SYSERR, "select");
		return (1);
	case 0:
		return (0);
	default:
		break;
	}

	/* Read the input. */
	TAILQ_FOREACH(sp, &wp->scrq, q)
		if (F_ISSET(sp, SC_SCRIPT) &&
		    FD_ISSET(sp->script->sh_master, &rdfd) && 
		    sscr_insert(sp))
			return (1);
	goto loop;
}

/*
 * sscr_insert --
 *	Take a line from the shell and insert it into the file.
 */
static int
sscr_insert(SCR *sp)
{
	struct timeval tv;
	char *endp, *p, *t;
	SCRIPT *sc;
	fd_set rdfd;
	db_recno_t lno;
	size_t len;
	ssize_t nr;
	char bp[1024];
	const CHAR_T *ip;
	size_t ilen = 0;

	/* Find out where the end of the file is. */
	if (db_last(sp, &lno))
		return (1);

	endp = bp;

	/* Read the characters. */
	sc = sp->script;
more:	switch (nr = read(sc->sh_master, endp, bp + sizeof(bp) - endp)) {
	case  0:			/* EOF; shell just exited. */
		sscr_end(sp);
		return (0);
	case -1:			/* Error or interrupt. */
		msgq(sp, M_SYSERR, "shell");
		return (1);
	default:
		endp += nr;
		break;
	}

	/* Append the lines into the file. */
	for (p = t = bp; p < endp; ++p) {
		if (*p == '\r' || *p == '\n') {
			len = p - t;
			if (CHAR2INT(sp, t, len, ip, ilen) ||
			    db_append(sp, 1, lno++, ip, ilen))
				return (1);
			t = p + 1;
		}
	}
	/*
	 * If the last thing from the shell isn't another prompt, wait up to
	 * 1/10 of a second for more stuff to show up, so that we don't break
	 * the output into two separate lines.  Don't want to hang indefinitely
	 * because some program is hanging, confused the shell, or whatever.
	 * Note that sc->sh_prompt can be NULL here.
	 */
	len = p - t;
	if (sc->sh_prompt == NULL || len != sc->sh_prompt_len ||
	    memcmp(t, sc->sh_prompt, len) != 0) {
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		FD_ZERO(&rdfd);
		FD_SET(sc->sh_master, &rdfd);
		if (select(sc->sh_master + 1, &rdfd, NULL, NULL, &tv) == 1) {
			if (len == sizeof(bp)) {
				if (CHAR2INT(sp, t, len, ip, ilen) ||
				    db_append(sp, 1, lno++, ip, ilen))
					return (1);
				endp = bp;
			} else {
				memmove(bp, t, len);
				endp = bp + len;
			}
			goto more;
		}
		if (sscr_setprompt(sp, t, len))
			return (1);
	}

	/* Append the remains into the file, and the cursor moves to EOF. */
	if (len > 0) {
		if (CHAR2INT(sp, t, len, ip, ilen) ||
		    db_append(sp, 1, lno++, ip, ilen))
			return (1);
		sp->cno = ilen - 1;
	} else
		sp->cno = 0;
	sp->lno = lno;
	return (vs_refresh(sp, 1));
}

/*
 * sscr_setprompt --
 *
 * Set the prompt in external ("char") encoding.
 *
 */
static int
sscr_setprompt(SCR *sp, char *buf, size_t len)
{
	SCRIPT *sc;

	sc = sp->script;
	if (sc->sh_prompt)
		free(sc->sh_prompt);
	MALLOC(sp, sc->sh_prompt, char *, len + 1);
	if (sc->sh_prompt == NULL) {
		sscr_end(sp);
		return (1);
	}
	memmove(sc->sh_prompt, buf, len);
	sc->sh_prompt_len = len;
	sc->sh_prompt[len] = '\0';
	return (0);
}

/*
 * sscr_end --
 *	End the pipe to a shell.
 *
 * PUBLIC: int sscr_end __P((SCR *));
 */
int
sscr_end(SCR *sp)
{
	SCRIPT *sc;

	if ((sc = sp->script) == NULL)
		return (0);

	/* Turn off the script flags. */
	F_CLR(sp, SC_SCRIPT);
	sscr_check(sp);

	/* Close down the parent's file descriptors. */
	if (sc->sh_master != -1)
	    (void)close(sc->sh_master);
	if (sc->sh_slave != -1)
	    (void)close(sc->sh_slave);

	/* This should have killed the child. */
	(void)proc_wait(sp, (long)sc->sh_pid, "script-shell", 0, 0);

	/* Free memory. */
	free(sc->sh_prompt);
	free(sc);
	sp->script = NULL;

	return (0);
}

/*
 * sscr_check --
 *	Set/clear the global scripting bit.
 */
static void
sscr_check(SCR *sp)
{
	GS *gp;
	WIN *wp;

	gp = sp->gp;
	wp = sp->wp;
	TAILQ_FOREACH(sp, &wp->scrq, q)
		if (F_ISSET(sp, SC_SCRIPT)) {
			F_SET(gp, G_SCRWIN);
			return;
		}
	F_CLR(gp, G_SCRWIN);
}

#ifndef HAVE_OPENPTY
#ifdef HAVE_SYS5_PTY
static int ptys_open __P((int, char *));
static int ptym_open __P((char *));

static int
sscr_pty(int *amaster, int *aslave, char *name, struct termios *termp, void *winp)
{
	int master, slave;

	/* open master terminal */
	if ((master = ptym_open(name)) < 0)  {
		errno = ENOENT;	/* out of ptys */
		return (-1);
	}

	/* open slave terminal */
	if ((slave = ptys_open(master, name)) >= 0) {
		*amaster = master;
		*aslave = slave;
	} else {
		errno = ENOENT;	/* out of ptys */
		return (-1);
	}

	if (termp)
		(void) tcsetattr(slave, TCSAFLUSH, termp);
#ifdef TIOCSWINSZ
	if (winp != NULL)
		(void) ioctl(slave, TIOCSWINSZ, (struct winsize *)winp);
#endif
	return (0);
}

/*
 * ptym_open --
 *	This function opens a master pty and returns the file descriptor
 *	to it.  pts_name is also returned which is the name of the slave.
 */
static int
ptym_open(char *pts_name)
{
	int fdm;
	char *ptr;

	strcpy(pts_name, _PATH_SYSV_PTY);
	if ((fdm = open(pts_name, O_RDWR)) < 0 )
		return (-1);

	if (grantpt(fdm) < 0) {
		close(fdm);
		return (-2);
	}

	if (unlockpt(fdm) < 0) {
		close(fdm);
		return (-3);
	}

	if (unlockpt(fdm) < 0) {
		close(fdm);
		return (-3);
	}

	/* get slave's name */
	if ((ptr = ptsname(fdm)) == NULL) {
		close(fdm);
		return (-3);
	}
	strcpy(pts_name, ptr);
	return (fdm);
}

/*
 * ptys_open --
 *	This function opens the slave pty.
 */
static int
ptys_open(int fdm, char *pts_name)
{
	int fds;

	if ((fds = open(pts_name, O_RDWR)) < 0) {
		close(fdm);
		return (-5);
	}

#ifdef I_PUSH
	if (ioctl(fds, I_PUSH, "ptem") < 0) {
		close(fds);
		close(fdm);
		return (-6);
	}

	if (ioctl(fds, I_PUSH, "ldterm") < 0) {
		close(fds);
		close(fdm);
		return (-7);
	}

	if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
		close(fds);
		close(fdm);
		return (-8);
	}
#endif /* I_PUSH */

	return (fds);
}

#else /* !HAVE_SYS5_PTY */

static int
sscr_pty(amaster, aslave, name, termp, winp)
	int *amaster, *aslave;
	char *name;
	struct termios *termp;
	void *winp;
{
	static char line[] = "/dev/ptyXX";
	const char *cp1, *cp2;
	int master, slave, ttygid;
	struct group *gr;

	if ((gr = getgrnam("tty")) != NULL)
		ttygid = gr->gr_gid;
	else
		ttygid = -1;

	for (cp1 = "pqrs"; *cp1; cp1++) {
		line[8] = *cp1;
		for (cp2 = "0123456789abcdef"; *cp2; cp2++) {
			line[5] = 'p';
			line[9] = *cp2;
			if ((master = open(line, O_RDWR, 0)) == -1) {
				if (errno == ENOENT)
					return (-1);	/* out of ptys */
			} else {
				line[5] = 't';
				(void) chown(line, getuid(), ttygid);
				(void) chmod(line, S_IRUSR|S_IWUSR|S_IWGRP);
#ifdef HAVE_REVOKE
				(void) revoke(line);
#endif
				if ((slave = open(line, O_RDWR, 0)) != -1) {
					*amaster = master;
					*aslave = slave;
					if (name)
						strcpy(name, line);
					if (termp)
						(void) tcsetattr(slave, 
							TCSAFLUSH, termp);
#ifdef TIOCSWINSZ
					if (winp)
						(void) ioctl(slave, TIOCSWINSZ, 
							(char *)winp);
#endif
					return (0);
				}
				(void) close(master);
			}
		}
	}
	errno = ENOENT;	/* out of ptys */
	return (-1);
}

#endif /* HAVE_SYS5_PTY */
#endif /* !HAVE_OPENPTY */
