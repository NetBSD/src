/*	$NetBSD: run.c,v 1.43 2003/07/10 13:36:48 dsl Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
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
#include <util.h>
#include <err.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "defs.h"

#include "menu_defs.h"
#include "msg_defs.h"

#define MAXBUF 256

#ifdef DEBUG
#define Xsystem(y) printf ("%s\n", y), 0
#else
#define Xsystem(y) system(y)
#endif

/*
 * local prototypes 
 */
static int launch_subwin (WINDOW *actionwin, char **args, struct winsize *win,
				int display, const char **errstr);
int log_flip (menudesc *, menu_ent *, void *);
int script_flip (menudesc *, menu_ent *, void *);

#define BUFSIZE 4096

char log_text[2][30] = {"Logging: Off", "Scripting: Off"};

menu_ent logmenu [2] = {
	{ log_text[0], OPT_NOMENU, 0, log_flip},
	{ log_text[1], OPT_NOMENU, 0, script_flip} };
	

void
do_logging(void)
{
	int menu_no;

	menu_no = new_menu ("Logging Functions", logmenu, 2, -1, 12,
		0, 20, MC_SCROLL, NULL, NULL, NULL,
		"Pick an option to turn on or off.\n", NULL);

	if (menu_no < 0) {
		(void)fprintf(stderr, "Dynamic menu creation failed.\n");
		if (logging)
			(void)fprintf(logfp, "Dynamic menu creation failed.\n");
		exit(EXIT_FAILURE);
	}
	process_menu(menu_no, NULL);
	free_menu(menu_no);
}

int
/*ARGSUSED*/
log_flip(menudesc *m, menu_ent *opt, void *arg)
{
	time_t tloc;

	(void)time(&tloc);
	if (logging == 1) {
		sprintf(log_text[0], "Logging: Off");
		logging = 0;
		fprintf(logfp, "Log ended at: %s\n", asctime(localtime(&tloc)));
		fflush(logfp);
		fclose(logfp);
	} else {
		logfp = fopen("sysinst.log", "a");
		if (logfp != NULL) {
			sprintf(log_text[0], "Logging: On");
			logging = 1;
			fprintf(logfp,
			    "Log started at: %s\n", asctime(localtime(&tloc)));
			fflush(logfp);		
		} else {
			msg_display(MSG_openfail, "log file", strerror(errno));
		}
	}
	return(0);
}

int
/*ARGSUSED*/
script_flip(menudesc *m, menu_ent *opt, void *arg)
{
	time_t tloc;

	(void)time(&tloc);
	if (scripting == 1) {
		sprintf(log_text[1], "Scripting: Off");
		scripting_fprintf(NULL, "# Script ended at: %s\n", asctime(localtime(&tloc)));
		scripting = 0;
		fflush(script);
		fclose(script);
	} else {
		script = fopen("sysinst.sh", "w");
		if (script != NULL) {
			sprintf(log_text[1], "Scripting: On");
			scripting = 1;
			scripting_fprintf(NULL, "#!/bin/sh\n");
			scripting_fprintf(NULL, "# Script started at: %s\n",
			    asctime(localtime(&tloc)));
			fflush(script);		
		} else {
			msg_display(MSG_openfail, "script file", strerror(errno));
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
	char fileorcmd [STRSIZE];
	va_list ap;

	va_start(ap, name);
	vsnprintf(fileorcmd, STRSIZE, name, ap);
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
	*buffer = (char *)malloc(fbytes + 1);
	if (!*buffer) 
		return -1;

	/* Read the buffer. */
	nbytes = 0;
	while (nbytes < fbytes && (ch = fgetc(f)) != EOF)
		(*buffer)[nbytes++] = ch;

	(*buffer)[nbytes] = 0;

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

/*
 * launch a program inside a subwindow, and report it's return status when done
 */
static int
launch_subwin(WINDOW *actionwin, char **args, struct winsize *win, int flags,
	const char **errstr)
{
	int xcor,ycor;
	int n, i, j;
	int selectfailed;
	int status, master, slave;
	fd_set active_fd_set, read_fd_set;
	int dataflow[2];
	pid_t child, subchild, pid;
	char ibuf[MAXBUF], obuf[MAXBUF];
	char pktdata;
	struct termios rtt;
	struct termios tt;
	struct timeval tmo;

	if (pipe(dataflow) < 0) {
		*errstr = "pipe() failed";
		return (1);
	}

	(void)tcgetattr(STDIN_FILENO, &tt);
	if (openpty(&master, &slave, NULL, &tt, win) == -1) {
		*errstr = "openpty() failed";
		return(1);
	}

#if 0
	rtt = tt;
	rtt.c_lflag |= (ICANON|ECHO); 
	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &rtt);
#endif
	rtt = tt;

	/* ignore tty signals until we're done with subprocess setup */
	endwin();
	ttysig_ignore = 1;
	ioctl(master, TIOCPKT, &ttysig_ignore);

	switch(child=fork()) {
	case -1:
		ttysig_ignore = 0;
		refresh();
		*errstr = "fork() failed";
		return -1;
	case 0:
		(void)close(STDIN_FILENO);
		subchild = fork();
		if (subchild == 0) {
			close(dataflow[0]);
			for (;;) {
				n = read(master, obuf, sizeof(obuf));
				if (n <= 0)
					break;
				write(dataflow[1], obuf, (size_t)n);
			} /* while spinning */
			_exit(EXIT_SUCCESS);
		} /* subchild, child forks */
		(void)close(master);
		close(dataflow[1]);
		close(dataflow[0]);
		rtt = tt;
		rtt.c_lflag |= (ICANON|ECHO); 
		(void)tcsetattr(slave, TCSANOW, &rtt);
		login_tty(slave);
		if (logging) {
			fprintf(logfp, "executing:");
			for (i = 0; args[i]; i++)
				fprintf(logfp, " %s", args[i]);
			fprintf(logfp, "\n");
			fclose(logfp);
		}
		if (scripting) {
			for (i = 0; args[i]; i++)
				fprintf(script, "%s ", args[i]);
			fprintf(script, "\n");
			fclose(script);
		}
		/*
		 * If target_prefix == "", the chroot will fail, but
		 * that's ok, since we don't need it then.
		 */
		if ((flags & RUN_CHROOT) != 0)
			chroot(target_prefix());
		execvp(*args, args);
		/* the parent will see this as the output from the
		   child */
		warn("execvp %s", *args);
		_exit(EXIT_FAILURE);
		break; /* end of child */
	default:
		/*
		 * we've set up the subprocess.  forward tty signals to its			 * process group.
		 */
		ttysig_forward = child;
		ttysig_ignore = 0;
		refresh();
		break;
	}
	close(dataflow[1]);
	FD_ZERO(&active_fd_set);
	FD_SET(dataflow[0], &active_fd_set);
	FD_SET(STDIN_FILENO, &active_fd_set);

	for (selectfailed = 0;;) {
		if (selectfailed) {
			char *mmsg = "select(2) failed but no child died?";
			if(logging)
				(void)fprintf(logfp, mmsg);
			errx(1, mmsg);
		}
		read_fd_set = active_fd_set;
		tmo.tv_sec = 1;
		tmo.tv_usec = 0;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &tmo) < 0) {
			if (errno == EINTR)
				goto loop;
			warn("select");
			if (logging)
				(void)fprintf(logfp,
				    "select failure: %s\n", strerror(errno));
			++selectfailed;
		} else for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET (i, &read_fd_set)) {
				n = read(i, ibuf, MAXBUF);
				if (n <= 0) {
					if (n < 0)
						warn("read");
					continue;
				}
				if (i == STDIN_FILENO) {
					(void)write(master, ibuf, (size_t)n);
					if ((rtt.c_lflag & ECHO) == 0)
						goto enddisp;
				}
				pktdata = ibuf[0];
				if (pktdata != 0 && i != STDIN_FILENO) {
					if (pktdata & TIOCPKT_IOCTL)
						memcpy(&rtt, ibuf, sizeof(rtt));
					goto enddisp;
				}
				for (j=1; j < n; j++) {
					if ((flags & RUN_DISPLAY) != 0) {
						switch (ibuf[j]) {
						case '\n':
							getyx(actionwin, ycor, xcor);
							if (ycor + 1 >= getmaxy(actionwin)) {
								scroll(actionwin);
								wmove(actionwin, getmaxy(actionwin) - 1, 0);
							} else
								wmove(actionwin, ycor + 1, 0);
							break;
						case '\r':
							getyx(actionwin, ycor, xcor);
							wmove(actionwin, ycor, 0);
							break;
						case '\b':
							getyx(actionwin, ycor, xcor);
							if (xcor > 0)
								wmove(actionwin, ycor, xcor - 1);
							break;
						default:
							waddch(actionwin, ibuf[j]);
							break;
						}
						if (logging)
							putc(ibuf[j], logfp);
					}
				}
enddisp:
				if ((flags & RUN_DISPLAY) != 0)
					wrefresh(actionwin);
				if (logging)
					fflush(logfp);
			}
		}
loop:
		pid = wait4(child, &status, WNOHANG, 0);
 		if (pid == child && (WIFEXITED(status) || WIFSIGNALED(status)))
			break;
	}
	close(dataflow[0]); /* clean up our leaks */
	close(master);
	close(slave);
	if (logging)
		fflush(logfp);

	/* from here on out, we take tty signals ourselves */
	ttysig_forward = 0;

	reset_prog_mode();

	if (WIFEXITED(status)) {
		*errstr = "command failed";
		return(WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		*errstr = "command ended on signal";
		return(WTERMSIG(status));
	} else
		return(0);
}

/*
 * generic program runner.  fatal and display can be set to
 * 1 if you wish the output to be displayed, or an error to be
 * fatal.
 */

int
run_prog(int flags, msg errmsg, const char *cmd, ...)
{
	va_list ap;
	struct winsize win;
	int ret;
	WINDOW *actionwin, *statuswin, *boxwin;
	char buf2[MAXBUF], scmd[MAXBUF];
	char *p, *args[256], **aps;
	const char *errstr;

	va_start(ap,cmd);
	vsnprintf(buf2, sizeof(buf2), cmd, ap);
	strcpy(scmd, buf2);
	p = buf2;

	/* 51 strings and it's blown! */
	for (aps = args; aps < &args[sizeof(args) / sizeof(args[0])] &&
	    (*aps = strsep(&p, " ")) != NULL;)
		if (**aps != '\0')
			++aps;

	/* Make curses save tty settings */
	def_prog_mode();

	(void)ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	/* Apparently, we sometimes get 0x0 back, and that's not useful */
	if (win.ws_row == 0)
		win.ws_row = 24;
	if (win.ws_col == 0)
		win.ws_col = 80;

	if ((flags & RUN_SYSTEM) != 0) {
		if ((flags & RUN_CHROOT) != 0)
			chroot(target_prefix());
		ret = system(scmd);
	} else if ((flags & RUN_DISPLAY) != 0) {
		wclear(stdscr);
		clearok(stdscr, 1);
		touchwin(stdscr);
		refresh();

		if ((flags & RUN_FULLSCREEN) != 0) {
			ret = launch_subwin(stdscr, args, &win, flags, &errstr);
			if (ret != 0) {
				waddstr(stdscr, "Press any key to continue");
				wrefresh(stdscr);
				getchar();
			}
			goto done;
		}

		statuswin = subwin(stdscr, 3, win.ws_col, 0, 0);
		if (statuswin == NULL) {
			fprintf(stderr, "sysinst: failed to allocate"
			    " status window.\n");
			exit(1);
		}

		boxwin = subwin(stdscr, 1, win.ws_col, 3, 0);
		if (boxwin == NULL) {
			fprintf(stderr, "sysinst: failed to allocate"
			    " status box.\n");
			exit(1);
		}

		actionwin = subwin(stdscr, win.ws_row - 4, win.ws_col, 4, 0);
		if (actionwin == NULL) {
			fprintf(stderr, "sysinst: failed to allocate"
				    " output window.\n");
			exit(1);
		}
		scrollok(actionwin, TRUE);

		win.ws_row -= 4;

		wmove(statuswin, 0, 5);
		waddstr(statuswin, "Status: ");
		wstandout(statuswin);
		waddstr(statuswin, "Running");
		wstandend(statuswin);
		wmove(statuswin, 1, 4);
		waddstr(statuswin, "Command: ");
		wstandout(statuswin);
		waddstr(statuswin, scmd);
		wstandend(statuswin);
		wrefresh(statuswin);

		wmove(boxwin, 0, 0);
		{
			int n, m;
			for (n = win.ws_col; (m = min(n, 30)) > 0; n -= m)
				waddstr(boxwin,
				    "------------------------------" + 30 - m);
		}
		wrefresh(boxwin);

		wrefresh(actionwin);

		ret = launch_subwin(actionwin, args, &win, flags, &errstr);

		wmove(statuswin, 0, 13);
		wstandout(statuswin);
		if (ret) {
			waddstr(statuswin, "Failed: ");
			waddstr(statuswin, errstr);
		} else
			waddstr(statuswin, "Finished");
		wstandend(statuswin);
		waddstr(statuswin, "  ");
		wmove(statuswin, 2, 5);
		if (ret != 0)
			waddstr(statuswin, "Press any key to continue");
		wrefresh(statuswin);
		if (ret != 0)
			(void)getchar();

		/* clean things up */
		delwin(actionwin);
		delwin(boxwin);
		delwin(statuswin);
done:
		wclear(stdscr);
		touchwin(stdscr);
		clearok(stdscr, 1);
		refresh();
	} else { /* display */
		ret = launch_subwin(NULL, args, &win, flags, &errstr);
	}
	va_end(ap);
	/* restore tty setting we saved earlier */
	reset_prog_mode();
	if ((flags & RUN_FATAL) != 0 && ret != 0)
		exit(ret);
	if (ret && errmsg != MSG_NONE) {
		msg_display(errmsg, scmd);
		process_menu(MENU_ok, NULL);
	}
	return (ret);
}
