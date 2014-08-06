/*	$NetBSD: main.c,v 1.3 2014/08/06 09:11:46 martin Exp $	*/

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

/* main sysinst program. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/uio.h>
#include <stdio.h>
#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <locale.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

int main(int, char **);
static void select_language(void);
__dead static void usage(void);
__dead static void miscsighandler(int);
static void ttysighandler(int);
static void cleanup(void);
static void process_f_flag(char *);

static int exit_cleanly = 0;	/* Did we finish nicely? */
FILE *logfp;			/* log file */
FILE *script;			/* script file */

#ifdef DEBUG
extern int log_flip(void);
#endif

/* Definion for colors */

struct {
	unsigned int bg;
	unsigned int fg;
} clr_arg;

/* String defaults and stuff for processing the -f file argument. */

static char bsddiskname[DISKNAME_SIZE]; /* default name for fist selected disk */

struct f_arg {
	const char *name;
	const char *dflt;
	char *var;
	int size;
};

static const struct f_arg fflagopts[] = {
	{"release", REL, rel, sizeof rel},
	{"machine", MACH, machine, sizeof machine},
	{"xfer dir", "/usr/INSTALL", xfer_dir, sizeof xfer_dir},
	{"ext dir", "", ext_dir_bin, sizeof ext_dir_bin},
	{"ext src dir", "", ext_dir_src, sizeof ext_dir_src},
	{"ftp host", SYSINST_FTP_HOST, ftp.host, sizeof ftp.host},
	{"ftp dir", SYSINST_FTP_DIR, ftp.dir, sizeof ftp.dir},
	{"ftp prefix", "/" MACH "/binary/sets", set_dir_bin, sizeof set_dir_bin},
	{"ftp src prefix", "/source/sets", set_dir_src, sizeof set_dir_src},
	{"ftp user", "ftp", ftp.user, sizeof ftp.user},
	{"ftp pass", "", ftp.pass, sizeof ftp.pass},
	{"ftp proxy", "", ftp.proxy, sizeof ftp.proxy},
	{"nfs host", "", nfs_host, sizeof nfs_host},
	{"nfs dir", "/bsd/release", nfs_dir, sizeof nfs_dir},
	{"cd dev", 0, cdrom_dev, sizeof cdrom_dev}, /* default filled in init */
	{"fd dev", "/dev/fd0a", fd_dev, sizeof fd_dev},
	{"local dev", "", localfs_dev, sizeof localfs_dev},
	{"local fs", "ffs", localfs_fs, sizeof localfs_fs},
	{"local dir", "release", localfs_dir, sizeof localfs_dir},
	{"targetroot mount", "/targetroot", targetroot_mnt, sizeof targetroot_mnt},
	{"dist postfix", ".tgz", dist_postfix, sizeof dist_postfix},
	{"diskname", "mydisk", bsddiskname, sizeof bsddiskname},
	{"pkg host", SYSINST_PKG_HOST, pkg.host, sizeof pkg.host},
	{"pkg dir", SYSINST_PKG_DIR, pkg.dir, sizeof pkg.dir},
	{"pkg prefix", "/" MACH "/" REL "/All", pkg_dir, sizeof pkg_dir},
	{"pkg user", "ftp", pkg.user, sizeof pkg.user},
	{"pkg pass", "", pkg.pass, sizeof pkg.pass},
	{"pkg proxy", "", pkg.proxy, sizeof pkg.proxy},
	{"pkgsrc host", SYSINST_PKGSRC_HOST, pkgsrc.host, sizeof pkgsrc.host},
	{"pkgsrc dir", "", pkgsrc.dir, sizeof pkgsrc.dir},
	{"pkgsrc prefix", "pub/pkgsrc/stable", pkgsrc_dir, sizeof pkgsrc_dir},
	{"pkgsrc user", "ftp", pkgsrc.user, sizeof pkgsrc.user},
	{"pkgsrc pass", "", pkgsrc.pass, sizeof pkgsrc.pass},
	{"pkgsrc proxy", "", pkgsrc.proxy, sizeof pkgsrc.proxy},

	{NULL, NULL, NULL, 0}
};

static void
init(void)
{
	const struct f_arg *arg;
	
	sizemult = 1;
	multname = msg_string(MSG_secname);
	tmp_ramdisk_size = 0;
	clean_xfer_dir = 0;
	mnt2_mounted = 0;
	fd_type = "msdos";
	layoutkind = LY_SETNEW;

	pm_head = (struct pm_head_t) SLIST_HEAD_INITIALIZER(pm_head);
	SLIST_INIT(&pm_head);
	pm_new = malloc (sizeof (pm_devs_t));
	memset(pm_new, 0, sizeof *pm_new);

	for (arg = fflagopts; arg->name != NULL; arg++) {
		if (arg->var == cdrom_dev)
			strlcpy(arg->var, get_default_cdrom(), arg->size);
		else
			strlcpy(arg->var, arg->dflt, arg->size);
	}
	strlcpy(pm_new->bsddiskname, bsddiskname, sizeof pm_new->bsddiskname);
	pkg.xfer_type = pkgsrc.xfer_type = "http";
	
	clr_arg.bg=COLOR_BLUE;
	clr_arg.fg=COLOR_WHITE;
}

__weakref_visible void prelim_menu(void)
    __weak_reference(md_prelim_menu);

int
main(int argc, char **argv)
{
	int ch;

	init();
#ifdef DEBUG
	log_flip();
#endif

	/* Check for TERM ... */
	if (!getenv("TERM")) {
		(void)fprintf(stderr,
			 "sysinst: environment variable TERM not set.\n");
		exit(4);
	}

	/* argv processing */
	while ((ch = getopt(argc, argv, "Dr:f:C:p")) != -1)
		switch(ch) {
		case 'D':	/* set to get past certain errors in testing */
			debug = 1;
			break;
		case 'r':
			/* Release name other than compiled in release. */
			strncpy(rel, optarg, sizeof rel);
			break;
		case 'f':
			/* Definition file to read. */
			process_f_flag(optarg);
			break;
		case 'C':
			/* Define colors */
			sscanf(optarg, "%u:%u", &clr_arg.bg, &clr_arg.fg);
			break;
		case 'p':
			/* Partition tool */
			partman_go = 1;
			break;
		case '?':
		default:
			usage();
		}

	md_init();

	/* initialize message window */
	if (menu_init()) {
		__menu_initerror();
		exit(4);
	}

	/*
	 * Put 'messages' in a window that has a one-character border
	 * on the real screen.
	 */
	mainwin = newwin(getmaxy(stdscr) - 2, getmaxx(stdscr) - 2, 1, 1);
	if (mainwin == NULL) {
		(void)fprintf(stderr,
			 "sysinst: screen too small\n");
		exit(1);
	}
	if (has_colors()) {
		start_color();
		do_coloring(clr_arg.fg,clr_arg.bg);
	} else {
		remove_color_options();
	}
	msg_window(mainwin);

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
	get_kb_encoding();

#ifdef __weak_reference
	/* if md wants to ask anything before we start, do it now */
	if (prelim_menu != 0)
		prelim_menu();
#endif

	/* Menu processing */
	if (partman_go)
		partman();
	else
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
	char prefix[PATH_MAX], fname[PATH_MAX];
	int max_lang = 16, num_lang = 0;
	const char *cp;
	menu_ent *opt = 0;
	int lang_menu = -1;
	int lang;

#ifdef CATALOG_DIR
	strcpy(prefix, CATALOG_DIR "/");
	dir = opendir(CATALOG_DIR);
	if (!dir) {
		strcpy(prefix, "./");
		dir = opendir(".");
	}
#else
	dir = opendir(".");
	strcpy(prefix, "./");
#endif
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
		strcpy(fname, prefix);
		strcat(fname, dirent->d_name);
		if (msg_file(fname))
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
		fnames[num_lang] = strdup(fname);
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

	/* set locale according to selected language */
	cp = msg_string(MSG_sysinst_message_locale);
	if (cp) {
		setlocale(LC_CTYPE, cp);
		setenv("LC_CTYPE", cp, 1);
	}
}

#ifndef md_may_remove_boot_medium
#define md_may_remove_boot_medium()	(boot_media_still_needed()<=0)
#endif

/* toplevel menu handler ... */
void
toplevel(void)
{
	/*
	 * Undo any stateful side-effects of previous menu choices.
	 * XXX must be idempotent, since we get run each time the main
	 *     menu is displayed.
	 */
	chdir(getenv("HOME"));
	unwind_mounts();

	/* Display banner message in (english, francais, deutsch..) */
	msg_display(MSG_hello);
	msg_display_add(MSG_md_hello);
	if (md_may_remove_boot_medium())
		msg_display_add(MSG_md_may_remove_boot_medium);
	msg_display_add(MSG_thanks);
}


/* The usage ... */

static void
usage(void)
{

	(void)fprintf(stderr, "%s", msg_string(MSG_usage));
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

#if 0
	restore_etc();
#endif
	/* Ensure we aren't inside the target tree */
	chdir(getenv("HOME"));
	unwind_mounts();
	umount_mnt2();

	endwin();

	if (logfp) {
		fprintf(logfp, "Log ended at: %s\n", asctime(localtime(&tloc)));
		fflush(logfp);
		fclose(logfp);
		logfp = NULL;
	}
	if (script) {
		fprintf(script, "# Script ended at: %s\n",
		    asctime(localtime(&tloc)));
		fflush(script);
		fclose(script);
		script = NULL;
	}

	if (!exit_cleanly)
		fprintf(stderr, "\n\nsysinst terminated.\n");
}


/* process function ... */

void
process_f_flag(char *f_name)
{
	char buffer[STRSIZE];
	int len;
	const struct f_arg *arg;
	FILE *fp;
	char *cp, *cp1;

	/* open the file */
	fp = fopen(f_name, "r");
	if (fp == NULL) {
		fprintf(stderr, msg_string(MSG_config_open_error), f_name);
		exit(1);
	}

	while (fgets(buffer, sizeof buffer, fp) != NULL) {
		cp = buffer + strspn(buffer, " \t");
		if (strchr("#\r\n", *cp) != NULL)
			continue;
		for (arg = fflagopts; arg->name != NULL; arg++) {
			len = strlen(arg->name);
			if (memcmp(cp, arg->name, len) != 0)
				continue;
			cp1 = cp + len;
			cp1 += strspn(cp1, " \t");
			if (*cp1++ != '=')
				continue;
			cp1 += strspn(cp1, " \t");
			len = strcspn(cp1, " \n\r\t");
			cp1[len] = 0;
			strlcpy(arg->var, cp1, arg->size);
			break;
		}
	}
	strlcpy(pm_new->bsddiskname, bsddiskname, sizeof pm_new->bsddiskname);

	fclose(fp);
}
