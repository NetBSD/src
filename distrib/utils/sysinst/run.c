/*	$NetBSD: run.c,v 1.7 1999/01/21 08:02:18 garbled Exp $	*/

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
 *      This product includes software develooped for the NetBSD Project by
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

extern int errno;

/*
 * local prototypes 
 */
int do_system __P((const char *cmdstr));
char* va_prog_cmdstr __P((char *cmd, va_list ap));
int launch_subwin __P((WINDOW *actionwin, char **args, struct winsize win, int display));
int log_flip __P((void));
int script_flip __P((void));

#define BUFSIZE 4096

char log_text[2][30] = {"Logging: Off", "Scripting: Off"};

menu_ent logmenu [2] = {
	{ log_text[0], OPT_NOMENU, 0, log_flip},
	{ log_text[1], OPT_NOMENU, 0, script_flip} };
	

void
do_logging(void)
{
	int menu_no;

	menu_no = new_menu (" Logging Functions ", logmenu, 2, 13, 12,
		0, 55, MC_SCROLL, NULL, NULL, "Pick an option to turn on or off.\n");

	if (menu_no < 0) {
		(void)fprintf(stderr, "Dynamic menu creation failed.\n");
		if (logging)
			(void)fprintf(log, "Dynamic menu creation failed.\n");
		exit(EXIT_FAILURE);
	}
	process_menu(menu_no);
	free_menu(menu_no);
}

int
log_flip(void)
{
	time_t tloc;

	(void)time(&tloc);
	if (logging == 1) {
		sprintf(log_text[0], "Logging: Off");
		logging = 0;
		fprintf(log, "Log ended at: %s\n", asctime(localtime(&tloc)));
		fflush(log);
		fclose(log);
	} else {
		sprintf(log_text[0], "Logging: On");
		logging = 1;
		log = fopen("sysinst.log", "a");
		fprintf(log, "Log started at: %s\n", asctime(localtime(&tloc)));
		fflush(log);		
	}
	return(0);
}

int
script_flip(void)
{
	time_t tloc;

	(void)time(&tloc);
	if (scripting == 1) {
		sprintf(log_text[1], "Scripting: Off");
		scripting = 0;
		fprintf(script, "# Script ended at: %s\n", asctime(localtime(&tloc)));
		fflush(script);
		fclose(script);
	} else {
		sprintf(log_text[1], "Scripting: On");
		scripting = 1;
		script = fopen("sysinst.sh", "w");
		fprintf(script, "#!/bin/sh\n");
		fprintf(script, "# Script started at: %s\n", asctime(localtime(&tloc)));
		fflush(script);		
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
do_system(execstr)
	const char *execstr;
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
 *  build command tring for do_system() from anonymous args.
 *  XXX return result is in a static buffer.
 */
char *
va_prog_cmdstr(char *cmd, va_list ap)
{
	static char command[STRSIZE];

	memset(command, 0, STRSIZE);
	(void)vsnprintf(command, STRSIZE, cmd, ap);
	return (command);
}


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
	int n, i, j;
	int status, master, slave;
	fd_set active_fd_set, read_fd_set;
	int dataflow[2];
	pid_t child, subchild, pid;
	char ibuf[MAXBUF], obuf[MAXBUF];
	char *command, *p, *argzero, **origargs;
	struct termios rtt;
	struct termios tt;

	pipe(dataflow);

	argzero = *args;
	origargs = args;

	command = (char *)malloc(MAXBUF * sizeof(char));
	for (p = *args; p != NULL; p = *++args) {
		strcat(command, p);
		strcat(command, " ");
	}
	(void)tcgetattr(STDIN_FILENO, &tt);
	if (openpty(&master, &slave, NULL, &tt, &win) == -1)
		return(1);
	rtt = tt;
	rtt.c_lflag &= ~ECHO; 
	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &rtt);

	switch(child=fork()) {
	case -1:
		return -1;
		break;
	case 0: {
		(void)close(STDIN_FILENO);
		subchild = fork();
		if (subchild == 0) {
			close(dataflow[0]);
			for (;;) {
				n = read(master, obuf, sizeof(obuf));
				if (n <= 0)
					break;
				write(dataflow[1], obuf, n);
			} /* while spinning */
			nonl();
			_exit(EXIT_SUCCESS);
		} /* subchild, child forks */
		(void)close(master);
		close(dataflow[1]);
		close(dataflow[0]);
		login_tty(slave);
		if (logging) {
			fprintf(log, "executing: %s\n", command);
			fflush(log);
			fclose(log);
		}
		if (scripting) {
			fprintf(script, "%s\n", command);
			fflush(script);
			fclose(script);
		}
		execvp(argzero, origargs);
	} break; /* end of child */
	default: break;
	}
	close(dataflow[1]);
	FD_ZERO(&active_fd_set);
	FD_SET(dataflow[0], &active_fd_set);
	FD_SET(STDIN_FILENO, &active_fd_set);

	pid = wait4(child, &status, WNOHANG, 0);
	for (;;) {
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			perror("select");
			if (logging)
				(void)fprintf(log, "select failure: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		for (i = 0; i < FD_SETSIZE; ++i)
			if (FD_ISSET (i, &read_fd_set)) {
				n = read(i, ibuf, MAXBUF);
				if (i == STDIN_FILENO)
					(void)write(master, ibuf, n);
				for (j=0; j < n; j++) {
					if (display) {
						if (ibuf[j] == '\n' || ibuf[j] == '\r') {
							if (ibuf[j] == '\n') {
								getyx(actionwin, ycor, xcor);
								if (ycor + 1 >= actionwin->maxy) {
									scroll(actionwin);
									wmove(actionwin, actionwin->maxy - 1, 1);
								} else
									wmove(actionwin, ycor + 1, 1);
								if (logging)
									fprintf(log, "\n");
							}
						} else {
							getyx(actionwin, ycor, xcor);
							if (xcor == 0)
							wmove(actionwin, ycor, xcor + 1);
							waddch(actionwin, ibuf[j]);
							wrefresh(actionwin);
							if (logging) {
								putc(ibuf[j], log);
								fflush(log);
							}
						}
					} else {  /* display is off */
						if (logging) {
							putc(ibuf[j], log);
							fflush(log);
						}
					}
				}
			}
		pid = wait4(child, &status, WNOHANG, 0);
 		if (pid == child && (WIFEXITED(status) || WIFSIGNALED(status)))
			break;
	}
	close(dataflow[0]); /* clean up our leaks */
	close(master);
	close(slave);
	if (logging)
		fflush(log);

	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);
	if (WIFEXITED(status))
		return(WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		return(WTERMSIG(status));
	else
		return(0);
}

/*
 * generic program runner.  fatal and display can be set to
 * 1 if you wish the output to be displayed, or an error to be
 * fatal.
 */

int
run_prog(int fatal, int display, char *cmd, ...)
{
	va_list ap;
	struct winsize win;
	int ret;
	WINDOW *actionwin, *statuswin, *boxwin;
	char buf2[MAXBUF];
	char *command, *p, *args[51], **aps;

	va_start(ap,cmd);
	sprintf(buf2,"%s",va_prog_cmdstr(cmd,ap));
	p = buf2;
	command = strdup(buf2);

	/* 51 strings and it's blown! */
	for (aps = args; (*aps = strsep(&p, " ")) != NULL;)
		if (**aps != '\0')
			++aps;

	(void)ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	if (display) {
		statuswin = subwin(stdscr, win.ws_row, win.ws_col, 0, 0);
		boxwin = subwin(statuswin, win.ws_row - 3, win.ws_col, 3, 0);
		actionwin = subwin(statuswin, win.ws_row - 5, win.ws_col - 3, 4, 1);
		scrollok(actionwin, TRUE);

		win.ws_col -= 3;
		win.ws_row -= 5;

		wclear(statuswin);
		wrefresh(statuswin);

		wclear(boxwin);
		box(boxwin, 124, 45);
		wrefresh(boxwin);

		wclear(actionwin);
		wrefresh(actionwin);

		wmove(statuswin, 0 , 5);
		waddstr(statuswin, "Status: ");
		wstandout(statuswin);
		waddstr(statuswin, "Running");
		wstandend(statuswin);

		wmove(statuswin, 1, 4);
		waddstr(statuswin, "Command: ");
		wstandout(statuswin);
		waddstr(statuswin, command);
		wstandend(statuswin);
		wrefresh(statuswin);

		ret = launch_subwin(actionwin, args, win, 1);

		wmove(statuswin, 0, 13);
		waddstr(statuswin, "          ");
		wmove(statuswin, 0, 13);
		wstandout(statuswin);
		if (ret != 0)
			waddstr(statuswin, "Failed");
		else
			waddstr(statuswin, "Finished");
		wstandend(statuswin);
		wmove(statuswin, 2, 5);
		waddstr(statuswin, "Press any key to continue");
		wrefresh(statuswin);
		(void)getchar();

		/* clean things up */
		wclear(actionwin);
		wrefresh(actionwin);
		delwin(actionwin);
		wclear(boxwin);
		wrefresh(boxwin);
		delwin(boxwin);
		wclear(statuswin);
		wrefresh(statuswin);
		delwin(statuswin);
		refresh();
	} else { /* display */
		ret = launch_subwin(NULL, args, win, 0);
	}
	va_end(ap);
	if (fatal)
		exit(ret);
	else
		return(ret);
}
