/*	$NetBSD: main.c,v 1.41 2003/07/28 11:32:21 dsl Exp $	*/

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

/* main sysinst program. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <stdio.h>
#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#define MAIN
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

int main(int, char **);
static void select_language(void);
static void usage(void);
static void miscsighandler(int);
static void ttysighandler(int);
static void cleanup(void);
static void set_defaults(void);
static void process_f_flag(char *);

static int exit_cleanly = 0;	/* Did we finish nicely? */
int logging;			/* are we logging everything? */
int scripting;			/* are we building a script? */
FILE *logfp;			/* log file */
FILE *script;			/* script file */

#ifdef DEBUG
extern int log_flip(void);
#endif

int
main(int argc, char **argv)
{
	WINDOW *win;
	int ch;

	logging = 0; /* shut them off unless turned on by the user */
#ifdef DEBUG
	log_flip();
#endif
	scripting = 0;

	/* Check for TERM ... */
	if (!getenv("TERM")) {
		(void)fprintf(stderr,
			 "sysinst: environment variable TERM not set.\n");
		exit(1);
	}

	/* argv processing */
	while ((ch = getopt(argc, argv, "Dr:f:")) != -1)
		switch(ch) {
		case 'D':	/* set to get past certain errors in testing */
			debug = 1;
			break;
		case 'r':
			/* Release name other than compiled in release. */
			strncpy(rel, optarg, SSTRSIZE);
			break;
		case 'f':
			/* Definition file to read. */
			process_f_flag(optarg);
			break;
		case '?':
		default:
			usage();
		}

	set_defaults();

	md_init();

	/* initialize message window */
	if (menu_init()) {
		__menu_initerror();
		exit(1);
	}

	/*
	 * XXX the following is bogus.  if screen is too small, message
	 * XXX window will be overwritten by menus.
	 */
	win = newwin(getmaxy(stdscr) - 2, getmaxx(stdscr) - 2, 1, 1);
	if (win == NULL) {
		(void)fprintf(stderr,
			 "sysinst: screen too small\n");
		exit(1);
	}
	if (has_colors()) {
		/*
		 * XXX This color trick should be done so much better,
		 * but is it worth it?
		 */
		wbkgd(win, COLOR_PAIR(1));
		wattrset(win, COLOR_PAIR(1));
	}
	msg_window(win);

	/* Watch for signals and clean up */
	(void)atexit(cleanup);
	(void)signal(SIGINT, ttysighandler);
	(void)signal(SIGQUIT, ttysighandler);
	(void)signal(SIGHUP, miscsighandler);

	/* redraw screen */
	touchwin(stdscr);
	refresh();

	/* Ensure we have mountpoint for target filesystems */
	mkdir(targetroot_mnt, S_IRWXU| S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH);

	select_language();

	/* Menu processing */
	process_menu(MENU_netbsd, NULL);
	
	exit_cleanly = 1;
	return 0;
}

static int
set_language(menudesc *m, void *arg)
{
	char **fnames = arg;

	msg_file(fnames[m->cursel]);
	return 1;
}

static void
select_language(void)
{
	DIR *dir;
	struct dirent *dirent;
	char **lang_msg, **fnames;
	int max_lang = 16, num_lang = 0;
	const char *cp;
	menu_ent *opt = 0;
	int lang_menu = -1;
	int lang;

	dir = opendir(".");
	if (!dir)
		return;

	lang_msg = malloc(max_lang * sizeof *lang_msg);
	fnames = malloc(max_lang * sizeof *fnames);
	if (!lang_msg || !fnames)
		goto done;

	lang_msg[0] = strdup(msg_string(MSG_sysinst_message_language));
	fnames[0] = 0;
	num_lang = 1;

	while ((dirent = readdir(dir)) != 0) {
		if (memcmp(dirent->d_name, "sysinstmsgs.", 12))
			continue;
		if (msg_file(dirent->d_name))
			continue;
		cp = msg_string(MSG_sysinst_message_language);
		if (!strcmp(cp, lang_msg[0]))
			continue;
		if (num_lang == max_lang) {
			char **new;
			max_lang *= 2;
			new = realloc(lang_msg, max_lang * sizeof *lang_msg);
			if (!new)
				break;
			lang_msg = new;
			new = realloc(fnames, max_lang * sizeof *fnames);
			if (!new)
				break;
			fnames = new;
		}
		fnames[num_lang] = strdup(dirent->d_name);
		lang_msg[num_lang++] = strdup(cp);
	}
	msg_file(0);
	closedir(dir);
	dir = 0;

	if (num_lang == 1)
		goto done;

	opt = calloc(num_lang, sizeof *opt);
	if (!opt)
		goto done;

	for (lang = 0; lang < num_lang; lang++) {
		opt[lang].opt_name = lang_msg[lang];
		opt[lang].opt_menu = OPT_NOMENU;
		opt[lang].opt_action = set_language;
	}

	lang_menu = new_menu(NULL, opt, num_lang, -1, 12, 0, 0, MC_NOEXITOPT,
		NULL, NULL, NULL, NULL, NULL);

	if (lang_menu != -1) {
		msg_display(MSG_hello);
		process_menu(lang_menu, fnames);
	}

    done:
	if (dir)
		closedir(dir);
	if (lang_menu != -1)
		free_menu(lang_menu);
	free(opt);
	while (num_lang) {
		free(lang_msg[--num_lang]);
		free(fnames[num_lang]);
	}
	free(lang_msg);
	free(fnames);
}

/* toplevel menu handler ... */
void
toplevel(void)
{

	/* Display banner message in (english, francais, deutsch..) */
	msg_display(MSG_hello);
	msg_display_add(MSG_md_hello);
	msg_display_add(MSG_thanks);

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
usage(void)
{

	(void)fprintf(stderr, msg_string(MSG_usage));
	exit(1);
}

/* ARGSUSED */
static void
miscsighandler(int signo)
{

	/*
	 * we need to cleanup(), but it was already scheduled with atexit(),
	 * so it'll be invoked on exit().
	 */
	exit(1);
}

static void
ttysighandler(int signo)
{

	/*
	 * if we want to ignore a TTY signal (SIGINT or SIGQUIT), then we
	 * just return.  If we want to forward a TTY signal, we forward it
	 * to the specified process group.
	 *
	 * This functionality is used when setting up and displaying child
	 * output so that the child gets the signal and presumably dies,
	 * but sysinst continues.  We use this rather than actually ignoring
	 * the signals, because that will be be passed on to a child
	 * through fork/exec, whereas special handlers get reset on exec..
	 */
	if (ttysig_ignore)
		return;
	if (ttysig_forward) {
		killpg(ttysig_forward, signo);
		return;
	}

	/*
	 * we need to cleanup(), but it was already scheduled with atexit(),
	 * so it'll be invoked on exit().
	 */
	exit(1);
}

static void
cleanup(void)
{
	time_t tloc;

	(void)time(&tloc);

	restore_etc();
	/* Ensure we aren't inside the target tree */
	chdir(getenv("HOME"));
	unwind_mounts();
	run_prog(0, NULL, "/sbin/umount /mnt2");

	endwin();

	if (logging) {
		fprintf(logfp, "Log ended at: %s\n", asctime(localtime(&tloc)));
		fflush(logfp);
		fclose(logfp);
	}
	if (scripting) {
		fprintf(script, "# Script ended at: %s\n",
		    asctime(localtime(&tloc)));
		fflush(script);
		fclose(script);
	}

	if (!exit_cleanly)
		fprintf(stderr, "\n\nsysinst terminated.\n");
}

static void
set_defaults(void)
{

	/*
	 * Set defaults for ftp_dir & cdrom_dir, by appending ftp_prefix.
	 * This occurs even when the settings are read in from
	 * "-f definition-file".
	 *
	 * Default values (can be changed in definition-file):
	 *	ftp_dir			SYSINST_FTP_DIR
	 *	SYSINST_FTP_DIR		"pub/NetBSD/NetBSD-" + REL + "/" MACH
	 *			
	 *	cdrom_dir		SYSINST_CDROM_DIR
	 #	SYSINST_CDROM_DIR	"/" + MACH
	 *
	 *	ftp_prefix		"/binary/sets"
	 */
	
		/* ftp_dir += ftp_prefix */
	strlcat(ftp_dir, ftp_prefix, STRSIZE);

		/* cdrom_dir += ftp_prefix */
	strlcat(cdrom_dir, ftp_prefix, STRSIZE);
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
static char *ftp_proxy_ptr = ftp_proxy;
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
	{"ftp proxy", "ftp proxy = %s", "a $0", &ftp_proxy_ptr, 1, STRSIZE,
		NULL},
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

void
process_f_flag(char *f_name)
{
	char *buffer;
	struct stat statinfo;
	int fd;

	/* stat the file (error reported) */

	if (stat(f_name, &statinfo) < 0) {
		perror(f_name);			/* XXX -- better message? */
		exit(1);
	}

	if ((statinfo.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, msg_string(MSG_not_regular_file), f_name);
		exit(1);
	}

	/* allocate buffer (error reported) */
	buffer = malloc((size_t)statinfo.st_size + 1);
	if (buffer == NULL) {
		fprintf(stderr, msg_string(MSG_out_of_memory));
		exit(1); 
	}

	/* open the file */
	fd = open(f_name, O_RDONLY, 0);
	if (fd < 0) {
		fprintf(stderr, msg_string(MSG_config_open_error), f_name);
		exit(1);
	}

	/* read the file */
	if (read(fd,buffer, (size_t)statinfo.st_size)
						!= (size_t)statinfo.st_size) {
		fprintf(stderr, msg_string(MSG_config_read_error), f_name);
		exit(1);
	}
	buffer[(size_t)statinfo.st_size] = 0;

	/* close the file */
	close(fd);

	/* Walk the buffer */
	walk(buffer, (size_t)statinfo.st_size, fflagopts,
	    sizeof(fflagopts)/sizeof(struct lookfor));

	/* free the buffer */
	free(buffer);
}
