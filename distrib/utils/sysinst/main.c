/*	$NetBSD: main.c,v 1.11 1999/01/21 08:02:18 garbled Exp $	*/

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

/* main sysinst program. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <stdio.h>
#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>

#define MAIN
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

int main __P((int argc, char **argv));
static void usage __P((void));
static void inthandler __P((int));
static void cleanup __P((void));
static void process_f_flag __P((char *));

static int exit_cleanly = 0;	/* Did we finish nicely? */
int logging;			/* are we logging everything? */
int scripting;			/* are we building a script? */
FILE *log;			/* log file */
FILE *script;			/* script file */

int
main(argc, argv)
	int argc;
	char **argv;
{
	WINDOW *win;
	int ch;

	logging = 0; /* shut them off unless turned on by the user */
	scripting = 0;

	/* Check for TERM ... */
	if (!getenv("TERM")) {
		(void)fprintf(stderr,
			 "sysinst: environment varible TERM not set.\n");
		exit(1);
	}

	/* argv processing */
	while ((ch = getopt(argc, argv, "r:f:")) != -1)
		switch(ch) {
		case 'r':
			/* Release name other than compiled in release. */
			strncpy(rel, optarg, SSTRSIZE);
			break;
		case 'f':
			/* Definition file to read. */
			process_f_flag (optarg);
			break;
		case '?':
		default:
			usage();
		}
	

	/* initialize message window */
	win = newwin(22, 78, 1, 1);	/* XXX BOGUS XXX */
       	msg_window(win);

	/* Watch for SIGINT and clean up */
	(void)signal(SIGINT, inthandler);
	(void)atexit(cleanup);

	/* Menu processing */
	process_menu(MENU_netbsd);
	
	exit_cleanly = 1;
	exit(0);
}
	

/* toplevel menu handler ... */
void
toplevel()
{

	/* Display banner message in (english, francais, deutche..) */
	msg_display(MSG_hello);

	/* 
	 * Undo any stateful side-effects of previous menu choices.
	 * XXX must be idempotent, since we get run each time the main
	 *     menu is displayed.
	 */
	unwind_mounts();
	/* ... */
}


/* The usage ... */

static void
usage()
{

	(void)fprintf(stderr, msg_string(MSG_usage));
	exit(1);
}

/* ARGSUSED */
static void
inthandler(notused)
	int notused;
{

	/* atexit() wants a void function, so inthandler() just calls cleanup */
	cleanup();
	exit(1);
}

static void
cleanup()
{
	time_t tloc;

	(void)time(&tloc);
	unwind_mounts();
	run_prog(0, 0, "/sbin/umount /mnt2");
	endwin();
	if (logging) {
		fprintf(log, "Log ended at: %s\n", asctime(localtime(&tloc)));
		fflush(log);
		fclose(log);
	}
	if (scripting) {
		fprintf(script, "# Script ended at: %s\n", asctime(localtime(&tloc)));
		fflush(script);
		fclose(script);
	}

	if (!exit_cleanly)
		fprintf(stderr, "\n\n sysinst terminated.\n");
}


/* Stuff for processing the -f file argument. */

/* Data definitions ... */

static char *rel_ptr = rel;
static char *machine_ptr = machine;
static char *dist_dir_ptr = dist_dir;
static char *ext_dir_ptr = ext_dir;
static char *ftp_host_ptr = ftp_host;
static char *ftp_dir_ptr = ftp_dir;
static char *ftp_prefix_ptr = ftp_prefix;
static char *ftp_user_ptr = ftp_user;
static char *ftp_pass_ptr = ftp_pass;
static char *nfs_host_ptr = nfs_host;
static char *nfs_dir_ptr = nfs_dir;
static char *cdrom_dev_ptr = cdrom_dev;
static char *cdrom_dir_ptr = cdrom_dir;
static char *localfs_dev_ptr = localfs_dev;
static char *localfs_fs_ptr = localfs_fs;
static char *localfs_dir_ptr = localfs_dir;
static char *targetroot_mnt_ptr = targetroot_mnt;
static char *distfs_mnt_ptr = distfs_mnt;
static char *dist_postfix_ptr = dist_postfix;

struct lookfor fflagopts[] = {
	{"release", "release = %s", "a $0", &rel_ptr, 1, SSTRSIZE, NULL},
	{"machine", "machine = %s", "a $0", &machine_ptr, 1, SSTRSIZE, NULL},
	{"dist dir", "dist dir = %s", "a $0", &dist_dir_ptr, 1, STRSIZE, NULL},
	{"ext dir", "ext dir = %s", "a $0", &ext_dir_ptr, 1, STRSIZE, NULL},
	{"ftp host", "ftp host = %s", "a $0", &ftp_host_ptr, 1, STRSIZE, NULL},
	{"ftp dir", "ftp dir = %s", "a $0", &ftp_dir_ptr, 1, STRSIZE, NULL},
	{"ftp prefix", "ftp prefix = %s", "a $0", &ftp_prefix_ptr, 1,
		STRSIZE, NULL},
	{"ftp user", "ftp user = %s", "a $0", &ftp_user_ptr, 1, STRSIZE, NULL},
	{"ftp pass", "ftp pass = %s", "a $0", &ftp_pass_ptr, 1, STRSIZE, NULL},
	{"nfs host", "nfs host = %s", "a $0", &nfs_host_ptr, 1, STRSIZE, NULL},
	{"nfs dir", "ftp dir = %s", "a $0", &nfs_dir_ptr, 1, STRSIZE, NULL},
	{"cd dev", "cd dev = %s", "a $0", &cdrom_dev_ptr, 1, STRSIZE, NULL},
	{"cd dir", "cd dir = %s", "a $0", &cdrom_dir_ptr, 1, STRSIZE, NULL},
	{"local dev", "local dev = %s", "a $0", &localfs_dev_ptr, 1, STRSIZE,
		NULL},
	{"local fs", "local fs = %s", "a $0", &localfs_fs_ptr, 1, STRSIZE,
		NULL},
	{"local dir", "local dir = %s", "a $0", &localfs_dir_ptr, 1, STRSIZE,
		NULL},
	{"targetroot mount", "targetroot mount = %s", "a $0",
		&targetroot_mnt_ptr, 1, STRSIZE, NULL},
	{"distfs mount", "distfs mount = %s", "a $0", &distfs_mnt_ptr, 1,
		STRSIZE, NULL},
	{"dist postfix", "dist postfix = %s", "a $0", &dist_postfix_ptr, 1,
		STRSIZE, NULL},
};

/* process function ... */

void process_f_flag (char *f_name)
{
  char *buffer;
  struct stat statinfo;
  int fd;

  /* stat the file (error reported) */

  if (stat(f_name, &statinfo) < 0) {
	perror (f_name);			/* XXX -- better message? */
	exit (1);
  }

  if ((statinfo.st_mode & S_IFMT) != S_IFREG) {
	fprintf (stderr, msg_string(MSG_not_regular_file), f_name);
  	exit (1);
  }

  /* allocate buffer (error reported) */
  buffer = (char *) malloc ((size_t)statinfo.st_size+1);
  if (buffer == NULL) {
	fprintf (stderr, msg_string(MSG_out_of_memory));
  	exit (1); 
  }

  /* open the file */
  fd = open (f_name, O_RDONLY, 0);
  if (fd < 0) {
	fprintf (stderr, msg_string(MSG_config_open_error), f_name);
  	exit (1);
  }

  /* read the file */
  if (read (fd,buffer,(size_t)statinfo.st_size) != (size_t)statinfo.st_size) {
	fprintf (stderr, msg_string(MSG_config_read_error), f_name);
  	exit (1);
  }
  buffer[statinfo.st_size] = 0;

  /* close the file */
  close (fd);

  /* Walk the buffer */
  walk (buffer, (size_t)statinfo.st_size, fflagopts,
	sizeof(fflagopts)/sizeof(struct lookfor));

  /* free the buffer */
  free (buffer);
}
