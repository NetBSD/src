/*      $NetBSD: run.c,v 1.2 2001/01/10 03:05:48 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* XXX write return codes ignored. XXX */

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
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
#include <sys/param.h>

#include "sushi.h"
#include "run.h"

#define MAXBUF 2048

char *savelines[1000];

extern int scripting;
extern int logging;
extern FILE *logfile;
extern FILE *script;
extern nl_catd catalog;

/*
 * local prototypes 
 */
int launch_subwin(WINDOW *actionwin, char **args, struct winsize win, int display);

#define BUFSIZE 4096

/*
 * launch a program inside a subwindow, and report it's return status when done
 */

int
launch_subwin(actionwin, args, win, display)
	WINDOW *actionwin;
	char **args;
	struct winsize win;
	int display;
{
	int xcor,ycor;
	int i, j, x, y;
	int selectfailed, multiloop, cols;
	int status, master, slave;
	fd_set active_fd_set, read_fd_set;
	int dataflow[2];
	pid_t child, subchild, pid;
	ssize_t n;
	char ibuf[MAXBUF], obuf[MAXBUF];
	char *command, *p, *argzero, **origargs;
	struct termios rtt;
	struct termios tt;

	pipe(dataflow);

	argzero = *args;
	origargs = args;
	x = -1; /* we inc x before using it */

	command = (char *)malloc(MAXBUF * sizeof(char));
	for (p = *args; p != NULL; p = *++args) {
		strcat(command, p);
		strcat(command, " ");
	}
	(void)tcgetattr(STDIN_FILENO, &tt);
	if (openpty(&master, &slave, NULL, &tt, &win) == -1)
		bailout("openpty: %s", strerror(errno));

	rtt = tt;
	rtt.c_lflag &= ~ECHO; 
	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &rtt);

	/* ignore tty signals until we're done with subprocess setup */
#if 0
	savetty();
	reset_shell_mode();
#endif
	endwin();
#if 0
	ttysig_ignore = 1;
#endif

	switch(child=fork()) {
	case -1:
#if 0
		ttysig_ignore = 0;
#endif
		refresh();
		bailout("fork: %s", strerror(errno));
		/* NOTREACHED */
		break;
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
		login_tty(slave);
		if (logging) {
			fprintf(logfile, "%s: %s\n",
			    catgets(catalog, 3, 1, "executing"), command);
			fflush(logfile);
			fclose(logfile);
		}
		if (scripting) {
			fprintf(script, "%s\n", command);
			fflush(script);
			fclose(script);
		}
		execvp(argzero, origargs);
		/* the parent will see this as the output from the
		   child */
		warn("execvp %s", argzero);
		_exit(EXIT_FAILURE);
		break; /* end of child */
	default:
		/*
		 * we've set up the subprocess.  forward tty signals to its			 * process group.
		 */
#if 0
		ttysig_forward = child;
		ttysig_ignore = 0;
#endif
		refresh();
		break;
	}
#if 0
	resetty();
#endif
	close(dataflow[1]);
	FD_ZERO(&active_fd_set);
	FD_SET(dataflow[0], &active_fd_set);
	FD_SET(STDIN_FILENO, &active_fd_set);

	cols = 0;
	for (selectfailed = 0,multiloop=0;;) {
again:
		if (multiloop > 10)
			goto loop; /* XXX */
		if (selectfailed) {
			char *msg = catgets(catalog, 1, 7,
			    "select failed but no child died");
			if(logging)
				(void)fprintf(logfile, msg);
			bailout(msg);
		}
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				goto loop;
			perror("select");
			if (logging)
				(void)fprintf(logfile, "%s: %s\n",
				    catgets(catalog, 1, 17, "select failure"),
				    strerror(errno));
			++selectfailed;
		} else for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET (i, &read_fd_set)) {
				n = read(i, ibuf, MAXBUF);
				if (n)
					multiloop=0;
				if (i == STDIN_FILENO)
					(void)write(master, ibuf, (size_t)n);
				for (j=0; j < n; j++) {
					if (display) {
						cols++;
						if (cols == getmaxx(actionwin) && ibuf[j] != '\n') {
							cols = 0;
							getyx(actionwin, ycor, xcor);
							if (ycor + 1 >= getmaxy(actionwin)) {
								if (x == 999) {
									for (y=1; y<1000; y++)
										savelines[y-1] = savelines[y];
								} else
									x++;
								savelines[x] = malloc(sizeof(char *) * win.ws_col);
								mvwinstr(actionwin, 0, 0, savelines[x]);
								wmove(actionwin, getmaxy(actionwin) - 1, 0);
								scroll(actionwin);
								wmove(actionwin, getmaxy(actionwin) - 1, 0);
							} else
								wmove(actionwin, ycor + 1, 0);
						}
						switch (ibuf[j]) {
						case '\n':
							cols = 0;
							getyx(actionwin, ycor, xcor);
							if (ycor + 1 >= getmaxy(actionwin)) {
								if (x == 999) {
									for (y=1; y<1000; y++)
										savelines[y-1] = savelines[y];
								} else
									x++;
								savelines[x] = malloc(sizeof(char *) * win.ws_col);
								mvwinstr(actionwin, 0, 0, savelines[x]);
								wmove(actionwin, getmaxy(actionwin) - 1, 0);
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
							putc(ibuf[j], logfile);
					}
				}
				if (display)
					wrefresh(actionwin);
				if (logging)
					fflush(logfile);
			}
		}
		multiloop++;
		goto again;
loop:
		pid = wait4(child, &status, WNOHANG, 0);
 		if (pid == child && (WIFEXITED(status) || WIFSIGNALED(status)))
			break;
	}
	close(dataflow[0]); /* clean up our leaks */
	close(master);
	close(slave);
	if (logging)
		fflush(logfile);

#if 0
	/* from here on out, we take tty signals ourselves */
	ttysig_forward = 0;
#endif

	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);

	savelines[x+1] = NULL;

	if (WIFEXITED(status))
		return(WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		return(WTERMSIG(status));
	else
		return(0);
}

/*
 * generic program runner.  display can be set to
 * 1 if you wish the output to be displayed.
 */

int
run_prog(int display, char **args)
{
	struct winsize win;
	int ret, i, done, numlines, x, y, curline;
	WINDOW *actionwin, *statuswin, *boxwin;
	char *command;
	char buf2[MAXBUF];

	command = buf2;
	sprintf(buf2, "%s ", args[0]);
	for (i=1; args[i] != NULL; i++) {
		command = strcat(command, args[i]);
		command = strcat(command, " ");
	}

	(void)ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	/* Apparently, we sometimes get 0x0 back, and that's not useful */
	if (win.ws_row == 0)
		win.ws_row = 24;
	if (win.ws_col == 0)
		win.ws_col = 80;

	if (!display) {
		ret = launch_subwin(NULL, args, win, 0);
		return(ret);
	}
	wclear(stdscr);
	clearok(stdscr, 1);
	refresh();

	statuswin = subwin(stdscr, 4, win.ws_col, 0, 0);
	if (statuswin == NULL)
		bailout(catgets(catalog, 1, 8,
		    "failed to allocate status window"));

	boxwin = subwin(stdscr, 1, win.ws_col, 4, 0);
	if (boxwin == NULL)
		bailout(catgets(catalog, 1, 9,
		    "failed to allocate status box"));

	actionwin = subwin(stdscr, win.ws_row - 5, win.ws_col, 5, 0);
	if (actionwin == NULL)
		bailout(catgets(catalog, 1, 10,
		    "failed to allocate output window"));

	scrollok(actionwin, TRUE);

	win.ws_row -= 5;

	wmove(statuswin, 0, 11 - (int)strlen(catgets(catalog, 3, 2, "Status")));
	waddstr(statuswin, catgets(catalog, 3, 2, "Status"));
	waddstr(statuswin, ": ");
	wstandout(statuswin);
	waddstr(statuswin, catgets(catalog, 3, 3, "Running"));
	wstandend(statuswin);
	wmove(statuswin, 1, 11 - (int)strlen(catgets(catalog, 3, 4, "Command")));
	waddstr(statuswin, catgets(catalog, 3, 4, "Command"));
	waddstr(statuswin, ": ");
	wstandout(statuswin);
	if (strlen(command) > (win.ws_col - 14)) {
		command[win.ws_col - 17] = '.';
		command[win.ws_col - 16] = '.';
		command[win.ws_col - 15] = '.';
		command[win.ws_col - 14] = '\0';
	}
	waddstr(statuswin, command);
	wstandend(statuswin);
	mvwaddstr(statuswin, 0,
	    win.ws_col - 12 - (int)strlen(catgets(catalog, 3, 5, "Lines")),
	    catgets(catalog, 3, 5, "Lines"));
	waddstr(statuswin, " (   0/   0)");
	wstandout(statuswin);
	mvwaddstr(statuswin, 3, 0, catgets(catalog, 3, 9,
	    "Use HOME,END,PGUP,PDGN,UP/DOWN Arrow keys to scroll.  ESC,"
	    " F3 exits, F10 quits."));
	wstandend(statuswin);
	wrefresh(statuswin);

	wmove(boxwin, 0, 0);
	{
		int n;
		for (n = 0; n < win.ws_col; n++)
			waddch(boxwin, ACS_HLINE);
	}
	wrefresh(boxwin);

	wrefresh(actionwin);

	ret = launch_subwin(actionwin, args, win, 1);

	wmove(statuswin, 0, 13);
	wstandout(statuswin);
	waddstr(statuswin, ret ? catgets(catalog, 3, 6, "Failed") :
	    catgets(catalog, 3, 7, "Finished"));
	wstandend(statuswin);
	waddstr(statuswin, "  ");
	wmove(statuswin, 2, 5);
	waddstr(statuswin, catgets(catalog, 3, 8, "Press any key to continue"));
	wrefresh(statuswin);

	/* process the scroller */
	noecho();
	keypad(stdscr, TRUE);
	done = 0;
	for (numlines = 0; savelines[numlines] != NULL; numlines++);
	for (x = 0; x < win.ws_row; x++) {
		if (numlines == 999) {
			for (y=1; y<1000; y++)
				savelines[y-1] = savelines[y];
		} else
			savelines[numlines] =
			    malloc(sizeof(char *) * win.ws_col);
		mvwinstr(actionwin, x, 0, savelines[numlines]);
		numlines++;
	}
	savelines[numlines] = NULL;
	numlines --;
	curline = numlines - win.ws_row+1;
	mvwprintw(statuswin, 0, win.ws_col-10, "%4d/%4d", curline, numlines);
	wrefresh(statuswin);
	while (!done) {
		i = getch();
		switch (i) {
		case KEY_F(10):
			endwin();
			exit(EXIT_SUCCESS);
			break;
		case KEY_HOME:
			wclear(actionwin);
			for (x = 0; x < win.ws_row-1; x++)
				if (savelines[x] != NULL)
					mvwaddstr(actionwin, x, 0,
					    savelines[x]);
			curline = 0;
			mvwprintw(statuswin, 0, win.ws_col-10, "%4d",
			    curline);
			wrefresh(statuswin);
			wrefresh(actionwin);
			break;
		case KEY_END:
			wclear(actionwin);
			curline = numlines - win.ws_row+1;
			for (x = numlines; x > numlines-win.ws_row; x--)
				if (savelines[x] != NULL)
					mvwaddstr(actionwin, x-curline,
					    0, savelines[x]);
			mvwprintw(statuswin, 0, win.ws_col-10, "%4d",
			    curline);
			wrefresh(statuswin);
			wrefresh(actionwin);
			break;
		case KEY_UP:
			if (curline == 0)
				break;
			wclear(actionwin);
			curline--;
			for (x = 0; x < win.ws_row-1; x++, curline++)
				if (savelines[curline] != NULL)
					mvwaddstr(actionwin, x, 0,
					    savelines[curline]);
			curline -= win.ws_row-1;
			mvwprintw(statuswin, 0, win.ws_col-10, "%4d", curline);
			wrefresh(statuswin);
			wrefresh(actionwin);
			break;
		case KEY_DOWN:
			if (curline + win.ws_row >= numlines)
				break;
			wclear(actionwin);
			curline++;
			for (x = curline; x < curline+win.ws_row; x++)
				if (savelines[x] != NULL)
					mvwaddstr(actionwin, x-curline, 0,
					    savelines[x]);
			mvwprintw(statuswin, 0, win.ws_col-10, "%4d", curline);
			wrefresh(statuswin);
			wrefresh(actionwin);
			break;
		case KEY_PPAGE:
			if (curline - win.ws_row < 0) {
				wclear(actionwin);
				for (x = 0; x < win.ws_row-1; x++)
					if (savelines[x] != NULL)
						mvwaddstr(actionwin, x, 0,
						    savelines[x]);
				curline = 0;
				mvwprintw(statuswin, 0, win.ws_col-10, "%4d",
				    curline);
				wrefresh(statuswin);
				wrefresh(actionwin);
			} else {
				wclear(actionwin);
				curline -= win.ws_row-1;
				for (x = 0; x < win.ws_row-1; x++, curline++)
					if (savelines[x] != NULL)
						mvwaddstr(actionwin, x, 0,
						    savelines[curline]);
				curline -= win.ws_row-1;
				mvwprintw(statuswin, 0, win.ws_col-10, "%4d",
				    curline);
				wrefresh(statuswin);
				wrefresh(actionwin);
			}
			break;
		case KEY_NPAGE:
			if (curline + win.ws_row == numlines) {
				;
			} else if (curline + win.ws_row*2-2 > numlines) {
				wclear(actionwin);
				curline = numlines - win.ws_row+1;
				for (x = numlines; x > numlines-win.ws_row; x--)
					if (savelines[x] != NULL)
						mvwaddstr(actionwin, x-curline,
						    0, savelines[x]);
				mvwprintw(statuswin, 0, win.ws_col-10, "%4d",
				    curline);
				wrefresh(statuswin);
				wrefresh(actionwin);
			} else {
				wclear(actionwin);
				for (x = 0; x < win.ws_row-1; x++, curline++)
					if (savelines[x] != NULL) {
						mvwaddstr(actionwin, x, 0,
						 savelines[curline+win.ws_row]);
						wrefresh(actionwin);
					}
				mvwprintw(statuswin, 0, win.ws_col-10, "%4d",
				    curline);
				wrefresh(statuswin);
				wrefresh(actionwin);
			}
			break;
		default:
			done = 1;
			break;
		}
	}
	/* clean things up */
	echo();
	delwin(actionwin);
	delwin(boxwin);
	delwin(statuswin);

	wclear(stdscr);
	clearok(stdscr, 1);
	refresh();

	return(ret);
}
