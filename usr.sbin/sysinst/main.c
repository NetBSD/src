/*	$NetBSD: main.c,v 1.28 2022/01/28 19:27:43 martin Exp $	*/

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

int debug;
char machine[SSTRSIZE];
int ignorerror;
int ttysig_ignore;
pid_t ttysig_forward;
uint sizemult;
int partman_go;
FILE *logfp;
FILE *script;
daddr_t root_limit;
struct pm_head_t pm_head;
struct pm_devs *pm;
struct pm_devs *pm_new;
char xfer_dir[STRSIZE];
int  clean_xfer_dir;
char ext_dir_bin[STRSIZE];
char ext_dir_src[STRSIZE];
char ext_dir_pkgsrc[STRSIZE];
char set_dir_bin[STRSIZE];
char set_dir_src[STRSIZE];
char pkg_dir[STRSIZE];
char pkgsrc_dir[STRSIZE];
const char *ushell;
struct ftpinfo ftp, pkg, pkgsrc;
int (*fetch_fn)(const char *);
char nfs_host[STRSIZE];
char nfs_dir[STRSIZE];
char cdrom_dev[SSTRSIZE];
char fd_dev[SSTRSIZE];
const char *fd_type;
char localfs_dev[SSTRSIZE];
char localfs_fs[SSTRSIZE];
char localfs_dir[STRSIZE];
char targetroot_mnt[SSTRSIZE];
int  mnt2_mounted;
char dist_postfix[SSTRSIZE];
char dist_tgz_postfix[SSTRSIZE];
WINDOW *mainwin;

static void select_language(const char*);
__dead static void usage(void);
__dead static void miscsighandler(int);
static void ttysighandler(int);
static void cleanup(void);
static void process_f_flag(char *);

static int exit_cleanly = 0;	/* Did we finish nicely? */
FILE *logfp;			/* log file */
FILE *script;			/* script file */

const char *multname;
const char *err_outofmem;

#ifdef DEBUG
extern int log_flip(void);
#endif

/* Definion for colors */

struct {
	unsigned int bg;
	unsigned int fg;
} clr_arg;

/* String defaults and stuff for processing the -f file argument. */

struct f_arg {
	const char *name;
	const char *dflt;
	char *var;
	int size;
};

static const struct f_arg fflagopts[] = {
	{"release", REL, NULL, 0},
	{"machine", MACH, machine, sizeof machine},
	{"xfer dir", "/usr/INSTALL", xfer_dir, sizeof xfer_dir},
	{"ext dir", "", ext_dir_bin, sizeof ext_dir_bin},
	{"ext src dir", "", ext_dir_src, sizeof ext_dir_src},
	{"ftp host", SYSINST_FTP_HOST, ftp.xfer_host[XFER_FTP], sizeof ftp.xfer_host[XFER_FTP]},
	{"http host", SYSINST_HTTP_HOST, ftp.xfer_host[XFER_HTTP], sizeof ftp.xfer_host[XFER_HTTP]},
	{"ftp dir", SYSINST_FTP_DIR, ftp.dir, sizeof ftp.dir},
	{"ftp prefix", "/" ARCH_SUBDIR "/binary/sets", set_dir_bin, sizeof set_dir_bin},
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
	{"dist postfix", "." SETS_TAR_SUFF, dist_postfix, sizeof dist_postfix},
	{"dist tgz postfix", ".tgz", dist_tgz_postfix, sizeof dist_tgz_postfix},
	{"pkg host", SYSINST_PKG_HOST, pkg.xfer_host[XFER_FTP], sizeof pkg.xfer_host[XFER_FTP]},
	{"pkg http host", SYSINST_PKG_HTTP_HOST, pkg.xfer_host[XFER_HTTP], sizeof pkg.xfer_host[XFER_HTTP]},
	{"pkg dir", SYSINST_PKG_DIR, pkg.dir, sizeof pkg.dir},
	{"pkg prefix", "/" PKG_ARCH_SUBDIR "/" PKG_SUBDIR "/All", pkg_dir, sizeof pkg_dir},
	{"pkg user", "ftp", pkg.user, sizeof pkg.user},
	{"pkg pass", "", pkg.pass, sizeof pkg.pass},
	{"pkg proxy", "", pkg.proxy, sizeof pkg.proxy},
	{"pkgsrc host", SYSINST_PKGSRC_HOST, pkgsrc.xfer_host[XFER_FTP], sizeof pkgsrc.xfer_host[XFER_FTP]},
	{"pkgsrc http host", SYSINST_PKGSRC_HTTP_HOST, pkgsrc.xfer_host[XFER_HTTP], sizeof pkgsrc.xfer_host[XFER_HTTP]},
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
	clean_xfer_dir = 0;
	mnt2_mounted = 0;
	fd_type = "msdos";

	pm_head = (struct pm_head_t) SLIST_HEAD_INITIALIZER(pm_head);
	SLIST_INIT(&pm_head);
	pm_new = malloc(sizeof (struct pm_devs));
	memset(pm_new, 0, sizeof *pm_new);

	for (arg = fflagopts; arg->name != NULL; arg++) {
		if (arg->var == NULL)
			continue;
		if (arg->var == cdrom_dev)
			get_default_cdrom(arg->var, arg->size);
		else
			strlcpy(arg->var, arg->dflt, arg->size);
	}
	pkg.xfer = pkgsrc.xfer = XFER_HTTP;

	clr_arg.bg=COLOR_BLUE;
	clr_arg.fg=COLOR_WHITE;
}

static void
init_lang(void)
{
	sizemult = 1;
	err_outofmem = msg_string(MSG_out_of_memory);
	multname = msg_string(MSG_secname);
}

int
main(int argc, char **argv)
{
	int ch;
	const char *msg_cat_dir = NULL;

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
	while ((ch = getopt(argc, argv, "Dr:f:C:m:"
#ifndef NO_PARTMAN
	    "p"
#endif
	    )) != -1)
		switch(ch) {
		case 'D':	/* set to get past certain errors in testing */
			debug = 1;
			break;
		case 'r':
			/* Release name - ignore for compatibility with older versions */
			break;
		case 'f':
			/* Definition file to read. */
			process_f_flag(optarg);
			break;
		case 'C':
			/* Define colors */
			sscanf(optarg, "%u:%u", &clr_arg.bg, &clr_arg.fg);
			break;
		case 'm':
			/* set message catalog directory */
			msg_cat_dir = optarg;
			break;
#ifndef NO_PARTMAN
		case 'p':
			/* Partition tool */
			partman_go = 1;
			break;
#endif
		case '?':
		default:
			usage();
		}

	md_init();

	/* Initialize the partitioning subsystem */
	partitions_init();

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
	mkdir(targetroot_mnt, S_IRWXU | S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH);

	select_language(msg_cat_dir);
	get_kb_encoding();
	init_lang();

	/* Menu processing */
	if (partman_go)
		partman();
	else
		process_menu(MENU_netbsd, NULL);

#ifndef NO_PARTMAN
	/* clean up internal storage */
	pm_destroy_all();
#endif

	partitions_cleanup();

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

/*
 * Search for sysinstmsg.* files in the given dir, collect
 * their names and return the number of files found.
 * fnames[0] is preallocated and duplicates are ignored.
 */
struct found_msgs {
	char **lang_msg, **fnames;
	int max_lang, num_lang;

};
static void
find_language_files(const char *path, struct found_msgs *res)
{
	DIR *dir;
	struct dirent *dirent;
	char fname[PATH_MAX];
	const char *cp;

	res->num_lang = 0;
	dir = opendir(path);
	if (!dir)
		return;

	while ((dirent = readdir(dir)) != 0) {
		if (memcmp(dirent->d_name, "sysinstmsgs.", 12))
			continue;

		if (res->num_lang == 0)
			res->num_lang = 1;
		strcpy(fname, path);
		strcat(fname, "/");
		strcat(fname, dirent->d_name);
		if (msg_file(fname))
			continue;
		cp = msg_string(MSG_sysinst_message_language);
		if (!strcmp(cp, res->lang_msg[0]))
			continue;
		if (res->num_lang == res->max_lang) {
			char **new;
			res->max_lang *= 2;
			new = realloc(res->lang_msg,
			    res->max_lang * sizeof *res->lang_msg);
			if (!new)
				break;
			res->lang_msg = new;
			new = realloc(res->fnames,
			    res->max_lang * sizeof *res->fnames);
			if (!new)
				break;
			res->fnames = new;
		}
		res->fnames[res->num_lang] = strdup(fname);
		res->lang_msg[res->num_lang++] = strdup(cp);
	}

	closedir(dir);
}

static void
select_language(const char *msg_cat_path)
{
	struct found_msgs found;
	menu_ent *opt = 0;
	const char *cp;
	int lang_menu = -1;
	int lang;

	found.max_lang = 16;
	found.num_lang = 0;
	found.lang_msg = malloc(found.max_lang * sizeof *found.lang_msg);
	found.fnames = malloc(found.max_lang * sizeof *found.fnames);
	if (!found.lang_msg || !found.fnames)
		goto done;
	found.lang_msg[0] = strdup(msg_string(MSG_sysinst_message_language));
	found.fnames[0] = NULL;

	if (msg_cat_path != NULL)
		find_language_files(msg_cat_path, &found);
	if (found.num_lang == 0)
		find_language_files(".", &found);
#ifdef CATALOG_DIR
	if (found.num_lang == 0)
		find_language_files(CATALOG_DIR, &found);
#endif

	msg_file(0);

	if (found.num_lang <= 1)
		goto done;

	opt = calloc(found.num_lang, sizeof *opt);
	if (!opt)
		goto done;

	for (lang = 0; lang < found.num_lang; lang++) {
		opt[lang].opt_name = found.lang_msg[lang];
		opt[lang].opt_action = set_language;
	}

	lang_menu = new_menu(NULL, opt, found.num_lang, -1, 12, 0, 0,
	    MC_NOEXITOPT, NULL, NULL, NULL, NULL, NULL);

	if (lang_menu != -1) {
		msg_display(MSG_hello);
		process_menu(lang_menu, found.fnames);
	}

    done:
	if (lang_menu != -1)
		free_menu(lang_menu);
	free(opt);
	for (int i = 0; i < found.num_lang; i++) {
		free(found.lang_msg[i]);
		free(found.fnames[i]);
	}
	free(found.lang_msg);
	free(found.fnames);

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
	char *home = getenv("HOME");
	if (home != NULL)
		if (chdir(home) != 0)
			(void)chdir("/");
	unwind_mounts();
	clear_swap();

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

	(void)fprintf(stderr, "usage: sysinst [-D] [-f definition_file] "
	    "[-r release] [-C bg:fg]"
#ifndef NO_PARTMAN
	    " [-p]"
#endif
	    "\n"
	    "where:\n"
	    "\t-D\n\t\trun in debug mode\n"
	    "\t-f definition_file\n\t\toverride built-in defaults from file\n"
	    "\t-m msg_catalog_dir\n\t\tuse translation files from msg_catalog_dir\n"
	    "\t-r release\n\t\toverride release name\n"
	    "\t-C bg:fg\n\t\tuse different color scheme\n"
#ifndef NO_PARTMAN
	    "\t-p\n\t\tonly run the partition editor, no installation\n"
#endif
	    );

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
	 * the signals, because that will be passed on to a child
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
	clear_swap();

	endwin();

	if (logfp) {
		fprintf(logfp, "Log ended at: %s\n", safectime(&tloc));
		fflush(logfp);
		fclose(logfp);
		logfp = NULL;
	}
	if (script) {
		fprintf(script, "# Script ended at: %s\n", safectime(&tloc));
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
	char *cp, *cp1, *err;

	/* open the file */
	fp = fopen(f_name, "r");
	if (fp == NULL) {
		const char *args[] = { f_name };
		err = str_arg_subst(msg_string(MSG_config_open_error),
		    __arraycount(args), args);
		fprintf(stderr, "%s\n", err);
		free(err);
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
			if (arg->var == NULL || arg->size == 0)
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

	fclose(fp);
}
