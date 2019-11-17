/*	$NetBSD: run.c,v 1.12.2.1 2019/11/17 13:45:26 msaitoh Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* run.c -- routines to interact with other programs. */

/* XXX write return codes ignored. XXX */

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <curses.h>
#include <termios.h>
#include <dirent.h>
#include <util.h>
#include <signal.h>
#include <err.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "defs.h"

#include "menu_defs.h"
#include "msg_defs.h"

#define MAXBUF 256

#if defined(DEBUG) && defined(DEBUG_SYSTEM)
static inline int
Xsystem(const char *y)
{
	printf ("%s\n", y);
	return 0;
}
#else
#define Xsystem(y) system(y)
#endif

/*
 * local prototypes
 */
int log_flip (menudesc *, void *);
static int script_flip (menudesc *, void *);

#define BUFSIZE 4096

menu_ent logmenu [2] = {
	{ .opt_action=log_flip},
	{ .opt_action=script_flip}
};

static void
log_menu_label(menudesc *m, int opt, void *arg)
{
	wprintw(m->mw, "%s: %s",
		msg_string(opt ? MSG_Scripting : MSG_Logging),
		msg_string((opt ? script != NULL : logfp != NULL) ?
		    MSG_On : MSG_Off));
}

void
do_logging(void)
{
	int menu_no;

	menu_no = new_menu(MSG_Logging_functions, logmenu, 2, -1, 12,
		0, 20, MC_SCROLL, NULL, log_menu_label, NULL,
		MSG_Pick_an_option, MSG_exit_menu_generic);

	if (menu_no < 0) {
		(void)fprintf(stderr, "Dynamic menu creation failed.\n");
		if (logfp)
			(void)fprintf(logfp, "Dynamic menu creation failed.\n");
		exit(EXIT_FAILURE);
	}
	process_menu(menu_no, NULL);
	free_menu(menu_no);
}

int
/*ARGSUSED*/
log_flip(menudesc *m, void *arg)
{
	time_t tloc;

	(void)time(&tloc);
	if (logfp) {
		fprintf(logfp, "Log ended at: %s\n", safectime(&tloc));
		fflush(logfp);
		fclose(logfp);
		logfp = NULL;
	} else {
		logfp = fopen("/tmp/sysinst.log", "a");
		if (logfp != NULL) {
			fprintf(logfp,
			    "Log started at: %s\n", safectime(&tloc));
			fflush(logfp);
		} else {
			if (mainwin) {
				msg_fmt_display(MSG_openfail, "%s%s",
				    "log file", strerror(errno));
			} else {
				fprintf(stderr, "could not open /tmp/sysinst.log: %s\n",
				    strerror(errno));
				exit(1);
			}
		}
	}
	return(0);
}

static int
/*ARGSUSED*/
script_flip(menudesc *m, void *arg)
{
	time_t tloc;

	(void)time(&tloc);
	if (script) {
		scripting_fprintf(NULL, "# Script ended at: %s\n",
		    safectime(&tloc));
		fflush(script);
		fclose(script);
		script = NULL;
	} else {
		script = fopen("/tmp/sysinst.sh", "w");
		if (script != NULL) {
			scripting_fprintf(NULL, "#!/bin/sh\n");
			scripting_fprintf(NULL, "# Script started at: %s\n",
			    safectime(&tloc));
			fflush(script);
		} else {
			msg_fmt_display(MSG_openfail, "%s%s", "script file",
			    strerror(errno));
		}
	}
	return(0);
}

int
collect(int kind, char **buffer, const char *name, ...)
{
	size_t nbytes;		/* Number of bytes in buffer. */
	size_t fbytes;		/* Number of bytes in file. */
	struct stat st;		/* stat information. */
	int ch;
	FILE *f;
	char fileorcmd[STRSIZE];
	va_list ap;
	char *cp;

	va_start(ap, name);
	vsnprintf(fileorcmd, sizeof fileorcmd, name, ap);
	va_end(ap);

	if (kind == T_FILE) {
		/* Get the file information. */
		if (stat(fileorcmd, &st)) {
			*buffer = NULL;
			return -1;
		}
		fbytes = (size_t)st.st_size;

		/* Open the file. */
		f = fopen(fileorcmd, "r");
		if (f == NULL) {
			*buffer = NULL;
			return -1;
		}
	} else {
		/* Open the program. */
		f = popen(fileorcmd, "r");
		if (f == NULL) {
			*buffer = NULL;
			return -1;
		}
		fbytes = BUFSIZE;
	}

	if (fbytes == 0)
		fbytes = BUFSIZE;

	/* Allocate the buffer size. */
	*buffer = cp = malloc(fbytes + 1);
	if (!cp)
		nbytes =  -1;
	else {
		/* Read the buffer. */
		nbytes = 0;
		while (nbytes < fbytes && (ch = fgetc(f)) != EOF)
			cp[nbytes++] = ch;
		cp[nbytes] = 0;
	}

	if (kind == T_FILE)
		fclose(f);
	else
		pclose(f);

	return nbytes;
}


/*
 * system(3), but with a debug wrapper.
 * use only for curses sub-applications.
 */
int
do_system(const char *execstr)
{
	register int ret;

	/*
	 * The following may be more than one function call.  Can't just
	 * "return Xsystem (command);"
	 */

	ret = Xsystem(execstr);
	return (ret);

}

static char **
make_argv(const char *cmd)
{
	char **argv = 0;
	int argc = 0;
	const char *cp;
	char *dp, *fn;
	DIR *dir;
	struct dirent *dirent;
	int l;

	for (; *cmd != 0; cmd = cp + strspn(cp, " "), argc++) {
		if (*cmd == '\'')
			cp = strchr(++cmd, '\'');
		else
			cp = strchr(cmd, ' ');
		if (cp == NULL)
			cp = strchr(cmd, 0);
		argv = realloc(argv, (argc + 2) * sizeof *argv);
		if (argv == NULL)
			err(1, "realloc(argv) for %s", cmd);
		asprintf(argv + argc, "%.*s", (int)(cp - cmd), cmd);
		/* Hack to remove %xx encoded ftp password */
		dp = strstr(cmd, ":%");
		if (dp != NULL && dp < cp) {
			for (fn = dp + 4; *fn == '%'; fn += 3)
				continue;
			if (*fn == '@')
				memset(dp + 1, '*', fn - dp - 1);
		}
		if (*cp == '\'')
			cp++;
		if (cp[-1] != '*')
			continue;
		/* do limited filename globbing */
		dp = argv[argc];
		fn = strrchr(dp, '/');
		if (fn != NULL)
			*fn = 0;
		dir = opendir(dp);
		if (fn != NULL)
			*fn++ = '/';
		else
			fn = dp;
		if (dir == NULL)
			continue;
		l = strlen(fn) - 1;
		while ((dirent = readdir(dir))) {
			if (dirent->d_name[0] == '.')
				continue;
			if (strncmp(dirent->d_name, fn, l) != 0)
				continue;
			if (dp != argv[argc])
				argc++;
			argv = realloc(argv, (argc + 2) * sizeof *argv);
			if (argv == NULL)
				err(1, "realloc(argv) for %s", cmd);
			asprintf(argv + argc, "%.*s%s", (int)(fn - dp), dp,
				dirent->d_name);
		}
		if (dp != argv[argc])
			free(dp);
		closedir(dir);
	}
	argv[argc] = NULL;
	return argv;
}

static void
free_argv(char **argv)
{
	char **n, *a;

	for (n = argv; (a = *n++);)
		free(a);
	free(argv);
}

static WINDOW *
show_cmd(const char *scmd, struct winsize *win)
{
	WINDOW *actionwin;
	int nrow;

	wclear(stdscr);
	clearok(stdscr, 1);
	touchwin(stdscr);
	refresh();

	mvaddstr(0, 4, msg_string(MSG_Status));
	standout();
	addstr(msg_string(MSG_Running));
	standend();
	mvaddstr(1, 4, msg_string(MSG_Command));
	standout();
	printw("%s", scmd);
	standend();
	addstr("\n\n");
	hline(0, win->ws_col);
	refresh();

	nrow = getcury(stdscr) + 1;

	actionwin = subwin(stdscr, win->ws_row - nrow, win->ws_col, nrow, 0);
	if (actionwin == NULL) {
		fprintf(stderr, "sysinst: failed to allocate output window.\n");
		exit(1);
	}
	scrollok(actionwin, TRUE);
	if (has_colors()) {
		wbkgd(actionwin, getbkgd(stdscr));
		wattrset(actionwin, getattrs(stdscr));
	}

	wmove(actionwin, 0, 0);
	wrefresh(actionwin);

	return actionwin;
}

/*
 * launch a program inside a subwindow, and report its return status when done
 */
static int
launch_subwin(WINDOW **actionwin, char **args, struct winsize *win, int flags,
    const char *scmd, const char **errstr)
{
	int n, i;
	int selectfailed;
	int status, master, slave;
	fd_set active_fd_set, read_fd_set;
	pid_t child, pid;
	char ibuf[MAXBUF];
	char pktdata;
	char *cp, *ncp;
	struct termios rtt, tt;
	struct timeval tmo;
	static int do_tioccons = 2;

	(void)tcgetattr(STDIN_FILENO, &tt);
	if (openpty(&master, &slave, NULL, &tt, win) == -1) {
		*errstr = "openpty() failed";
		return -1;
	}

	rtt = tt;

	/* ignore tty signals until we're done with subprocess setup */
	ttysig_ignore = 1;
	ioctl(master, TIOCPKT, &ttysig_ignore);

	/* Try to get console output into our pipe */
	if (do_tioccons) {
		if (ioctl(slave, TIOCCONS, &do_tioccons) == 0
		    && do_tioccons == 2) {
			/* test our output - we don't want it grabbed */
			write(1, " \b", 2);
			ioctl(master, FIONREAD, &do_tioccons);
			if (do_tioccons != 0) {
				do_tioccons = 0;
				ioctl(slave, TIOCCONS, &do_tioccons);
			} else
				do_tioccons = 1;
		}
	}

	if (logfp)
		fflush(logfp);
	if (script)
		fflush(script);

	child = fork();
	switch (child) {
	case -1:
		ttysig_ignore = 0;
		refresh();
		*errstr = "fork() failed";
		return -1;
	case 0:	/* child */
		(void)close(STDIN_FILENO);
		/* silently stop curses */
		(void)close(STDOUT_FILENO);
		(void)open("/dev/null", O_RDWR, 0);
		dup2(STDIN_FILENO, STDOUT_FILENO);
		endwin();
		(void)close(master);
		rtt = tt;
		rtt.c_lflag |= (ICANON|ECHO);
		(void)tcsetattr(slave, TCSANOW, &rtt);
		login_tty(slave);
		if (logfp) {
			fprintf(logfp, "executing: %s\n", scmd);
			fclose(logfp);
			logfp = NULL;
		}
		if (script) {
			fprintf(script, "%s\n", scmd);
			fclose(script);
			script = NULL;
		}
		if (strcmp(args[0], "cd") == 0 && strcmp(args[2], "&&") == 0) {
			target_chdir_or_die(args[1]);
			args += 3;
		}
		if (flags & RUN_XFER_DIR)
			target_chdir_or_die(xfer_dir);
		/*
		 * If target_prefix == "", the chroot will fail, but
		 * that's ok, since we don't need it then.
		 */
		if (flags & RUN_CHROOT && *target_prefix()
		    && chroot(target_prefix()) != 0)
			warn("chroot(%s) for %s", target_prefix(), *args);
		else {
			execvp(*args, args);
			warn("execvp %s", *args);
		}
		_exit(EXIT_FAILURE);
		// break; /* end of child */
	default:
		/*
		 * parent: we've set up the subprocess.
		 * forward tty signals to its process group.
		 */
		ttysig_forward = child;
		ttysig_ignore = 0;
		break;
	}

	/*
	 * Now loop transferring program output to screen, and keyboard
	 * input to the program.
	 */

	FD_ZERO(&active_fd_set);
	FD_SET(master, &active_fd_set);
	FD_SET(STDIN_FILENO, &active_fd_set);

	for (selectfailed = 0;;) {
		if (selectfailed) {
			const char mmsg[] =
			    "select(2) failed but no child died?";
			if (logfp)
				(void)fprintf(logfp, mmsg);
			errx(1, mmsg);
		}
		read_fd_set = active_fd_set;
		tmo.tv_sec = flags & RUN_SILENT ? 20 : 2;
		tmo.tv_usec = 0;
		i = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &tmo);
		if (i == 0 && *actionwin == NULL && (flags & RUN_SILENT) == 0)
			*actionwin = show_cmd(scmd, win);
		if (i < 0) {
			if (errno != EINTR) {
				warn("select");
				if (logfp)
					(void)fprintf(logfp,
					    "select failure: %s\n",
					    strerror(errno));
				selectfailed = 1;
			}
		} else for (i = 0; i < FD_SETSIZE; ++i) {
			if (!FD_ISSET(i, &read_fd_set))
				continue;
			n = read(i, ibuf, sizeof ibuf - 1);
			if (n <= 0) {
				if (n < 0)
					warn("read");
				continue;
			}
			ibuf[n] = 0;
			cp = ibuf;
			if (i == STDIN_FILENO) {
				(void)write(master, ibuf, (size_t)n);
				if (!(rtt.c_lflag & ECHO))
					continue;
			} else {
				pktdata = ibuf[0];
				if (pktdata != 0) {
					if (pktdata & TIOCPKT_IOCTL)
						memcpy(&rtt, ibuf, sizeof(rtt));
					continue;
				}
				cp += 1;
			}
			if (*cp == 0 || flags & RUN_SILENT)
				continue;
			if (logfp) {
				fprintf(logfp, "%s", cp);
				fflush(logfp);
			}
			if (*actionwin == NULL)
				*actionwin = show_cmd(scmd, win);
			/* posix curses is braindead wrt \r\n so... */
			for (ncp = cp; (ncp = strstr(ncp, "\r\n")); ncp += 2) {
				ncp[0] = '\n';
				ncp[1] = '\r';
			}
			waddstr(*actionwin, cp);
			wrefresh(*actionwin);
		}
		pid = wait4(child, &status, WNOHANG, 0);
 		if (pid == child && (WIFEXITED(status) || WIFSIGNALED(status)))
			break;
	}
	close(master);
	close(slave);
	if (logfp)
		fflush(logfp);

	/* from here on out, we take tty signals ourselves */
	ttysig_forward = 0;

	reset_prog_mode();

	if (WIFEXITED(status)) {
		*errstr = msg_string(MSG_Command_failed);
		return WEXITSTATUS(status);
	}
	if (WIFSIGNALED(status)) {
		*errstr = msg_string(MSG_Command_ended_on_signal);
		return WTERMSIG(status);
	}
	return 0;
}

/*
 * generic program runner.
 * flags:
 *	RUN_DISPLAY	display command name and output
 *	RUN_FATAL	program errors are fatal
 *	RUN_CHROOT	chroot to target before the exec
 *	RUN_FULLSCREEN	display output only
 *	RUN_SILENT	do not display program output
 *	RUN_ERROR_OK	don't wait for key if program fails
 *	RUN_PROGRESS	don't wait for key if program has output
 * If both RUN_DISPLAY and RUN_SILENT are clear then the program name will
 * be displayed as soon as it generates output.
 * Steps are taken to collect console messages, they will be interleaved
 * into the program output - but not upset curses.
 */

int
run_program(int flags, const char *cmd, ...)
{
	va_list ap;
	struct winsize win;
	int ret;
	WINDOW *actionwin = NULL;
	char *scmd;
	char **args;
	const char *errstr = NULL;

	va_start(ap, cmd);
	vasprintf(&scmd, cmd, ap);
	va_end(ap);
	if (scmd == NULL)
		err(1, "vasprintf(&scmd, \"%s\", ...)", cmd);

	args = make_argv(scmd);

	/* Make curses save tty settings */
	def_prog_mode();

	(void)ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	/* Apparently, we sometimes get 0x0 back, and that's not useful */
	if (win.ws_row == 0)
		win.ws_row = 24;
	if (win.ws_col == 0)
		win.ws_col = 80;

	if ((flags & RUN_DISPLAY) != 0) {
		if (flags & RUN_FULLSCREEN) {
			wclear(stdscr);
			clearok(stdscr, 1);
			touchwin(stdscr);
			refresh();
			actionwin = stdscr;
		} else
			actionwin = show_cmd(scmd, &win);
	} else
		win.ws_row -= 4;

	ret = launch_subwin(&actionwin, args, &win, flags, scmd, &errstr);
	fpurge(stdin);

	/* If the command failed, show command name */
	if (actionwin == NULL && ret != 0 && !(flags & RUN_ERROR_OK))
		actionwin = show_cmd(scmd, &win);

	if (actionwin != NULL) {
		int y, x;
		getyx(actionwin, y, x);
		if (actionwin != stdscr)
			mvaddstr(0, 4, msg_string(MSG_Status));
		if (ret != 0) {
			if (actionwin == stdscr && x != 0)
				addstr("\n");
			x = 1;	/* force newline below */
			standout();
			addstr(errstr);
			standend();
		} else {
			if (actionwin != stdscr) {
				standout();
				addstr(msg_string(MSG_Finished));
				standend();
			}
		}
		clrtoeol();
		refresh();
		if ((ret != 0 && !(flags & RUN_ERROR_OK)) ||
		    (y + x != 0 && !(flags & RUN_PROGRESS))) {
			if (actionwin != stdscr)
				move(getbegy(actionwin) - 2, 5);
			else if (x != 0)
				addstr("\n");
			addstr(msg_string(MSG_Hit_enter_to_continue));
			refresh();
			getchar();
		} else {
			if (y + x != 0) {
				/* give user 1 second to see messages */
				refresh();
				sleep(1);
			}
		}
	}

	/* restore tty setting we saved earlier */
	reset_prog_mode();

	/* clean things up */
	if (actionwin != NULL) {
		if (actionwin != stdscr)
			delwin(actionwin);
		if (errstr == 0 || !(flags & RUN_NO_CLEAR)) {
			wclear(stdscr);
			touchwin(stdscr);
			clearok(stdscr, 1);
			refresh();
		}
	}

	free(scmd);
	free_argv(args);

	if (ret != 0 && flags & RUN_FATAL)
		exit(ret);
	return ret;
}
