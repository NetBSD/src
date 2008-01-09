/*	$NetBSD: util.c,v 1.147.8.2 2008/01/09 01:28:30 matt Exp $	*/

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

/* util.c -- routines that don't really fit anywhere else... */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <curses.h>
#include <errno.h>
#include <dirent.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

#ifndef MD_SETS_SELECTED
#define MD_SETS_SELECTED SET_KERNEL_1, SET_SYSTEM, SET_X11, SET_MD
#endif
#ifndef MD_SETS_SELECTED_MINIMAL
#define MD_SETS_SELECTED_MINIMAL SET_KERNEL_1, SET_CORE
#endif
#ifndef MD_SETS_VALID
#define MD_SETS_VALID SET_KERNEL, SET_SYSTEM, SET_X11, SET_MD
#endif

static const char *msg_yes, *msg_no, *msg_all, *msg_some, *msg_none;
static const char *msg_cur_distsets_row;
static int select_menu_width;

static uint8_t set_status[SET_GROUP_END];
#define SET_VALID	0x01
#define SET_SELECTED	0x02
#define SET_SKIPPED	0x04
#define SET_INSTALLED	0x08

struct  tarstats {
	int nselected;
	int nfound;
	int nnotfound;
	int nerror;
	int nsuccess;
	int nskipped;
} tarstats;

distinfo dist_list[] = {
#ifdef SET_KERNEL_1_NAME
	{SET_KERNEL_1_NAME,	SET_KERNEL_1,		MSG_set_kernel_1, NULL},
#endif
#ifdef SET_KERNEL_2_NAME
	{SET_KERNEL_2_NAME,	SET_KERNEL_2,		MSG_set_kernel_2, NULL},
#endif
#ifdef SET_KERNEL_3_NAME
	{SET_KERNEL_3_NAME,	SET_KERNEL_3,		MSG_set_kernel_3, NULL},
#endif
#ifdef SET_KERNEL_4_NAME
	{SET_KERNEL_4_NAME,	SET_KERNEL_4,		MSG_set_kernel_4, NULL},
#endif
#ifdef SET_KERNEL_5_NAME
	{SET_KERNEL_5_NAME,	SET_KERNEL_5,		MSG_set_kernel_5, NULL},
#endif
#ifdef SET_KERNEL_6_NAME
	{SET_KERNEL_6_NAME,	SET_KERNEL_6,		MSG_set_kernel_6, NULL},
#endif
#ifdef SET_KERNEL_7_NAME
	{SET_KERNEL_7_NAME,	SET_KERNEL_7,		MSG_set_kernel_7, NULL},
#endif
#ifdef SET_KERNEL_8_NAME
	{SET_KERNEL_8_NAME,	SET_KERNEL_8,		MSG_set_kernel_8, NULL},
#endif

	{"base",		SET_BASE,		MSG_set_base, NULL},
	{"etc",			SET_ETC,		MSG_set_system, NULL},
	{"comp",		SET_COMPILER,		MSG_set_compiler, NULL},
	{"games",		SET_GAMES,		MSG_set_games, NULL},
	{"man",			SET_MAN_PAGES,		MSG_set_man_pages, NULL},
	{"misc",		SET_MISC,		MSG_set_misc, NULL},
	{"tests",		SET_TESTS,		MSG_set_tests, NULL},
	{"text",		SET_TEXT_TOOLS,		MSG_set_text_tools, NULL},

	{NULL,			SET_GROUP,		MSG_set_X11, NULL},
	{"xbase",		SET_X11_BASE,		MSG_set_X11_base, NULL},
	{"xcomp",		SET_X11_PROG,		MSG_set_X11_prog, NULL},
	{"xetc",		SET_X11_ETC,		MSG_set_X11_etc, NULL},
	{"xfont",		SET_X11_FONTS,		MSG_set_X11_fonts, NULL},
	{"xserver",		SET_X11_SERVERS,	MSG_set_X11_servers, NULL},
	{NULL,			SET_GROUP_END,		NULL, NULL},

#ifdef SET_MD_1_NAME
	{SET_MD_1_NAME,		SET_MD_1,		MSG_set_md_1, NULL},
#endif
#ifdef SET_MD_2_NAME
	{SET_MD_2_NAME,		SET_MD_2,		MSG_set_md_2, NULL},
#endif
#ifdef SET_MD_3_NAME
	{SET_MD_3_NAME,		SET_MD_3,		MSG_set_md_3, NULL},
#endif
#ifdef SET_MD_4_NAME
	{SET_MD_4_NAME,		SET_MD_4,		MSG_set_md_4, NULL},
#endif

	{NULL,			SET_LAST,		NULL, NULL},
};

/*
 * local prototypes 
 */

static int check_for(unsigned int mode, const char *pathname);

void
init_set_status(int minimal)
{
	static const uint8_t sets_valid[] = {MD_SETS_VALID};
	static const uint8_t sets_selected_full[] = {MD_SETS_SELECTED};
	static const uint8_t sets_selected_minimal[] = {MD_SETS_SELECTED_MINIMAL};
	static const uint8_t *sets_selected;
	int nelem_selected;
	unsigned int i, len;
	const char *longest;

	if (minimal) {
		sets_selected = sets_selected_minimal;
		nelem_selected = nelem(sets_selected_minimal);
	} else {
		sets_selected = sets_selected_full;
		nelem_selected = nelem(sets_selected_full);
	}

	for (i = 0; i < nelem(sets_valid); i++)
		set_status[sets_valid[i]] = SET_VALID;
	for (i = 0; i < nelem_selected; i++)
		set_status[sets_selected[i]] |= SET_SELECTED;

	set_status[SET_GROUP] = SET_VALID;

	/* Lookup some strings we need lots of times */
	msg_yes = msg_string(MSG_Yes);
	msg_no = msg_string(MSG_No);
	msg_all = msg_string(MSG_All);
	msg_some = msg_string(MSG_Some);
	msg_none = msg_string(MSG_None);
	msg_cur_distsets_row = msg_string(MSG_cur_distsets_row);

	/* Find longest and use it to determine width of selection menu */
	len = strlen(msg_no); longest = msg_no;
	i = strlen(msg_yes); if (i > len) {len = i; longest = msg_yes; }
	i = strlen(msg_all); if (i > len) {len = i; longest = msg_all; }
	i = strlen(msg_some); if (i > len) {len = i; longest = msg_some; }
	i = strlen(msg_none); if (i > len) {len = i; longest = msg_none; }
	select_menu_width = snprintf(NULL, 0, msg_cur_distsets_row, "",longest);
}

int
dir_exists_p(const char *path)
{

	return file_mode_match(path, S_IFDIR);
}

int
file_exists_p(const char *path)
{

	return file_mode_match(path, S_IFREG);
}

int
file_mode_match(const char *path, unsigned int mode)
{
	struct stat st;

	return (stat(path, &st) == 0 && (st.st_mode & S_IFMT) == mode);
}

uint
get_ramsize(void)
{
	uint64_t ramsize;
	size_t len = sizeof ramsize;
	int mib[2] = {CTL_HW, HW_PHYSMEM64};
	
	sysctl(mib, 2, &ramsize, &len, NULL, 0);

	/* Find out how many Megs ... round up. */
	return (ramsize + MEG - 1) / MEG;
}

void
run_makedev(void)
{
	char *owd;

	msg_display_add("\n\n");
	msg_display_add(MSG_makedev);

	owd = getcwd(NULL, 0);

	/* make /dev, in case the user  didn't extract it. */
	make_target_dir("/dev");
	target_chdir_or_die("/dev");
	run_program(0, "/bin/sh MAKEDEV all");

	chdir(owd);
	free(owd);
}

static int
floppy_fetch(const char *set_name)
{
	char post[4];
	msg errmsg;
	int menu;
	int status;
	const char *write_mode = ">";

	strcpy(post, "aa");

	errmsg = "";
	menu = MENU_fdok;
	for (;;) {
		umount_mnt2();
		msg_display(errmsg);
		msg_display_add(MSG_fdmount, set_name, post);
		process_menu(menu, &status);
		if (status != SET_CONTINUE)
			return status;
		menu = MENU_fdremount;
		errmsg = MSG_fdremount;
		if (run_program(0, "/sbin/mount -r -t %s %s /mnt2",
							fd_type, fd_dev))
			continue;
		mnt2_mounted = 1;
		errmsg = MSG_fdnotfound;

		/* Display this because it might take a while.... */
		if (run_program(RUN_DISPLAY,
			    "sh -c '/bin/cat /mnt2/%s.%s %s %s/%s/%s%s'",
			    set_name, post, write_mode,
			    target_prefix(), xfer_dir, set_name, dist_postfix))
			/* XXX: a read error will give a corrupt file! */
			continue;

		/* We got that file, advance to next fragment */
		if (post[1] < 'z')
			post[1]++;
		else
			post[1] = 'a', post[0]++;
		write_mode = ">>";
		errmsg = "";
		menu = MENU_fdok;
	}
}

/*
 * Load files from floppy.  Requires a /mnt2 directory for mounting them.
 */
int
get_via_floppy(void)
{

	process_menu(MENU_floppysource, NULL);

	fetch_fn = floppy_fetch;

	/* Set ext_dir for absolute path. */
	snprintf(ext_dir, sizeof ext_dir, "%s/%s", target_prefix(), xfer_dir);

	return SET_OK;
}

/*
 * Get from a CDROM distribution.
 */
int
get_via_cdrom(void)
{

	/* Get CD-rom device name and path within CD-rom */
	process_menu(MENU_cdromsource, NULL);

	/* Mount it */
	if (run_program(0, "/sbin/mount -rt cd9660 /dev/%s /mnt2",
	    cdrom_dev) != 0)
		return SET_RETRY;

	mnt2_mounted = 1;

	snprintf(ext_dir, sizeof ext_dir, "%s/%s", "/mnt2", set_dir);

	return SET_OK;
}


/*
 * Get from a pathname inside an unmounted local filesystem
 * (e.g., where sets were preloaded onto a local DOS partition) 
 */
int
get_via_localfs(void)
{

	/* Get device, filesystem, and filepath */
	process_menu (MENU_localfssource, NULL);

	/* Mount it */
	if (run_program(0, "/sbin/mount -rt %s /dev/%s /mnt2",
	    localfs_fs, localfs_dev))
		return SET_RETRY;

	mnt2_mounted = 1;

	snprintf(ext_dir, sizeof ext_dir, "%s/%s/%s",
		"/mnt2", localfs_dir, set_dir);

	return SET_OK;
}

/*
 * Get from an already-mounted pathname.
 */

int
get_via_localdir(void)
{

	/* Get filepath */
	process_menu(MENU_localdirsource, NULL);

	/*
	 * We have to have an absolute path ('cos pax runs in a
	 * different directory), make it so.
	 */
	snprintf(ext_dir, sizeof ext_dir, "/%s/%s", localfs_dir, set_dir);

	return SET_OK;
}


/*
 * Support for custom distribution fetches / unpacks.
 */

unsigned int
set_X11_selected(void)
{
	int i;

	for (i = SET_X11_FIRST; ++i < SET_X11_LAST;)
		if (set_status[i] & SET_SELECTED)
			return 1;
	return 0;
}

unsigned int
get_kernel_set(void)
{
	int i;

	for (i = SET_KERNEL_FIRST; ++i < SET_KERNEL_LAST;)
		if (set_status[i] & SET_SELECTED)
			return i;
	return SET_NONE;
}

void
set_kernel_set(unsigned int kernel_set)
{
	int i;

	/* only one kernel set is allowed */
	for (i = SET_KERNEL_FIRST; ++i < SET_KERNEL_LAST;)
		set_status[i] &= ~SET_SELECTED;
	set_status[kernel_set] |= SET_SELECTED;
}

static int
set_toggle(menudesc *menu, void *arg)
{
	distinfo **distp = arg;
	int set = distp[menu->cursel]->set;

	if (set > SET_KERNEL_FIRST && set < SET_KERNEL_LAST &&
	    !(set_status[set] & SET_SELECTED))
		set_kernel_set(set);
	else
		set_status[set] ^= SET_SELECTED;
	return 0;
}

static int
set_all_none(menudesc *menu, void *arg, int set, int clr)
{
	distinfo **distp = arg;
	distinfo *dist = *distp;
	int nested;

	for (nested = 0; dist->set != SET_GROUP_END || nested--; dist++) {
		if (dist->set == SET_GROUP) {
			nested++;
			continue;
		}
		set_status[dist->set] = (set_status[dist->set] & ~clr) | set;
	}
	return 0;
}

static int
set_all(menudesc *menu, void *arg)
{
	return set_all_none(menu, arg, SET_SELECTED, 0);
}

static int
set_none(menudesc *menu, void *arg)
{
	return set_all_none(menu, arg, 0, SET_SELECTED);
}

static void
set_label(menudesc *menu, int opt, void *arg)
{
	distinfo **distp = arg;
	distinfo *dist = distp[opt];
	const char *selected;
	const char *desc;
	int nested;

	desc = dist->desc;

	if (dist->set != SET_GROUP)
		selected = set_status[dist->set] & SET_SELECTED ? msg_yes : msg_no;
	else {
		/* sub menu - display None/Some/All */
		nested = 0;
		selected = "unknown";
		while ((++dist)->set != SET_GROUP_END || nested--) {
			if (dist->set == SET_GROUP) {
				nested++;
				continue;
			}
			if (!(set_status[dist->set] & SET_VALID))
				continue;
			if (set_status[dist->set] & SET_SELECTED) {
				if (selected == msg_none) {
					selected = msg_some;
					break;
				}
				selected = msg_all;
			} else {
				if (selected == msg_all) {
					selected = msg_some;
					break;
				}
				selected = msg_none;
			}
		}
	}

	wprintw(menu->mw, msg_cur_distsets_row, msg_string(desc), selected);
}

static int set_sublist(menudesc *menu, void *arg);

static int
initialise_set_menu(distinfo *dist, menu_ent *me, distinfo **de, int all_none)
{
	int set;
	int sets;
	int nested;

	for (sets = 0; ; dist++) {
		set = dist->set;
		if (set == SET_LAST || set == SET_GROUP_END)
			break;
		if (!(set_status[set] & SET_VALID))
			continue;
		*de = dist;
		me->opt_menu = OPT_NOMENU;
		me->opt_flags = 0;
		me->opt_name = NULL;
		if (set != SET_GROUP)
			me->opt_action = set_toggle;
		else {
			/* Collapse sublist */
			nested = 0;
			while ((++dist)->set != SET_GROUP_END || nested--) {
				if (dist->set == SET_GROUP)
					nested++;
			}
			me->opt_action = set_sublist;
		}
		sets++;
		de++;
		me++;
	}

	if (all_none) {
		me->opt_menu = OPT_NOMENU;
		me->opt_flags = 0;
		me->opt_name = MSG_select_all;
		me->opt_action = set_all;
		me++;
		me->opt_menu = OPT_NOMENU;
		me->opt_flags = 0;
		me->opt_name = MSG_select_none;
		me->opt_action = set_none;
		sets += 2;
	}

	return sets;
}

static int
set_sublist(menudesc *menu, void *arg)
{
	distinfo *de[SET_LAST];
	menu_ent me[SET_LAST];
	distinfo **dist = arg;
	int menu_no;
	int sets;

	sets = initialise_set_menu(dist[menu->cursel] + 1, me, de, 1);

	menu_no = new_menu(NULL, me, sets, 20, 10, 0, select_menu_width,
		MC_SUBMENU | MC_SCROLL | MC_DFLTEXIT,
		NULL, set_label, NULL, NULL,
		MSG_install_selected_sets);

	process_menu(menu_no, de);
	free_menu(menu_no);

	return 0;
}

void
customise_sets(void)
{
	distinfo *de[SET_LAST];
	menu_ent me[SET_LAST];
	int sets;
	int menu_no;

	msg_display(MSG_cur_distsets);
	msg_table_add(MSG_cur_distsets_header);

	sets = initialise_set_menu(dist_list, me, de, 0);

	menu_no = new_menu(NULL, me, sets, 0, 5, 0, select_menu_width,
		MC_SCROLL | MC_NOBOX | MC_DFLTEXIT | MC_NOCLEAR,
		NULL, set_label, NULL, NULL,
		MSG_install_selected_sets);

	process_menu(menu_no, de);
	free_menu(menu_no);
}

static int
ask_verbose_dist(msg setup_done)
{
	int verbose = 0;

	wclear(stdscr);
	wrefresh(stdscr);
	if (setup_done != NULL)
		msg_display(setup_done);
	msg_display_add(MSG_verboseextract);
	process_menu(MENU_extract, &verbose);
	wclear(stdscr);
	wrefresh(stdscr);

	return verbose;
}

/*
 * Extract_file **REQUIRES** an absolute path in ext_dir.  Any code
 * that sets up xfer_dir for use by extract_file needs to put in the
 * full path name to the directory. 
 */

static int
extract_file(distinfo *dist, int update, int verbose)
{
	char path[STRSIZE];
	char *owd;
	int   rval;

	/* If we might need to tidy up, ensure directory exists */
	if (fetch_fn != NULL)
		make_target_dir(xfer_dir);

	(void)snprintf(path, sizeof path, "%s/%s%s",
	    ext_dir, dist->name, dist_postfix);
	
	owd = getcwd(NULL, 0);

	/* Do we need to fetch the file now? */
	if (fetch_fn != NULL) {
		rval = fetch_fn(dist->name);
		if (rval != SET_OK)
			return rval;
	}

	/* check tarfile exists */
	if (!file_exists_p(path)) {
		tarstats.nnotfound++;

		msg_display(MSG_notarfile, path);
		process_menu(MENU_ok, NULL);
		return SET_RETRY;
	}

	tarstats.nfound++;	
	/* cd to the target root. */
	if (update && dist->set == SET_ETC) {
		make_target_dir("/.sysinst");
		target_chdir_or_die("/.sysinst");
	} else
		target_chdir_or_die("/");

	/* now extract set files into "./". */
	if (verbose == 0)
		rval = run_program(RUN_DISPLAY | RUN_PROGRESS, 
				"progress -zf %s tar --chroot -xhepf -", path);
	else if (verbose == 1)
		rval = run_program(RUN_DISPLAY, 
				"tar --chroot -zxhepf %s", path);
	else
		rval = run_program(RUN_DISPLAY | RUN_PROGRESS, 
				"tar --chroot -zxhvepf %s", path);

	chdir(owd);
	free(owd);

	/* Check rval for errors and give warning. */
	if (rval != 0) {
		tarstats.nerror++;
		msg_display(MSG_tarerror, path);
		process_menu(MENU_ok, NULL);
		return SET_RETRY;
	}

	if (fetch_fn != NULL && clean_xfer_dir) {
		run_program(0, "rm %s", path);
		/* Plausibly we should unlink an empty xfer_dir as well */
	}

	if (update && dist->set == SET_ETC) {
		int oldsendmail;
		oldsendmail = run_program(RUN_DISPLAY | RUN_CHROOT |
					  RUN_ERROR_OK | RUN_PROGRESS,
					  "/usr/sbin/postinstall -s /.sysinst -d / check mailerconf");
		if (oldsendmail == 1) {
			msg_display(MSG_oldsendmail);
			process_menu(MENU_yesno, NULL);
			if (yesno) {
				run_program(RUN_DISPLAY | RUN_CHROOT,
					    "/usr/sbin/postinstall -s /.sysinst -d / fix mailerconf");
			}
		}
		run_program(RUN_DISPLAY | RUN_CHROOT,
			"/usr/sbin/postinstall -s /.sysinst -d / fix");
	}

	set_status[dist->set] |= SET_INSTALLED;
	tarstats.nsuccess++;
	return SET_OK;
}

static void
skip_set(distinfo *dist, int skip_type)
{
	int nested;
	int set;

	nested = 0;
	while ((++dist)->set != SET_GROUP_END || nested--) {
		set = dist->set;
		if (set == SET_GROUP) {
			nested++;
			continue;
		}
		if (set == SET_LAST)
			break;
		if (set_status[set] == (SET_SELECTED | SET_VALID))
			set_status[set] |= SET_SKIPPED;
		tarstats.nskipped++;
	}
}

/*
 * Get and unpack the distribution.
 * Show success_msg if installation completes.
 * Otherwise show failure_msg and wait for the user to ack it before continuing.
 * success_msg and failure_msg must both be 0-adic messages.
 */
int
get_and_unpack_sets(int update, msg setupdone_msg, msg success_msg, msg failure_msg)
{
	distinfo *dist;
	int status;
	int verbose;
	int set;

	/* Ensure mountpoint for distribution files exists in current root. */
	(void)mkdir("/mnt2", S_IRWXU| S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH);
	if (scripting)
		(void)fprintf(script, "mkdir -m 755 /mnt2\n");

	/* reset failure/success counters */
	memset(&tarstats, 0, sizeof(tarstats));

	/* Find out which files to "get" if we get files. */

	/* ask user whether to do normal or verbose extraction */
	verbose = ask_verbose_dist(setupdone_msg);

	/* Accurately count selected sets */
	for (dist = dist_list; (set = dist->set) != SET_LAST; dist++) {
		if ((set_status[set] & (SET_VALID | SET_SELECTED))
		    == (SET_VALID | SET_SELECTED))
			tarstats.nselected++;
	}

	status = SET_RETRY;
	for (dist = dist_list; ; dist++) {
		set = dist->set;
		if (set == SET_LAST)
			break;
		if (dist->name == NULL)
			continue;
		if (set_status[set] != (SET_VALID | SET_SELECTED))
			continue;

		if (status != SET_OK) {
			/* This might force a redraw.... */
			clearok(curscr, 1);
			touchwin(stdscr);
			wrefresh(stdscr);
			/* Sort out the location of the set files */
			do {
				umount_mnt2();
				msg_display(MSG_distmedium, tarstats.nselected,
				    tarstats.nsuccess + tarstats.nskipped,
				    dist->name);
				fetch_fn = NULL;
				process_menu(MENU_distmedium, &status);
			} while (status == SET_RETRY);

			if (status == SET_SKIP) {
				set_status[set] |= SET_SKIPPED;
				tarstats.nskipped++;
				continue;
			}
			if (status == SET_SKIP_GROUP) {
				skip_set(dist, status);
				continue;
			}
			if (status != SET_OK) {
				msg_display(failure_msg);
				process_menu(MENU_ok, NULL);
				return 1;
			}
		}

		/* Try to extract this set */
		status = extract_file(dist, update, verbose);
		if (status == SET_RETRY)
			dist--;
	}

	if (tarstats.nerror == 0 && tarstats.nsuccess == tarstats.nselected) {
		msg_display(MSG_endtarok);
		/* Give user a chance to see the success message */
		sleep(1);
	} else {
		/* We encountered errors. Let the user know. */
		msg_display(MSG_endtar,
		    tarstats.nselected, tarstats.nnotfound, tarstats.nskipped,
		    tarstats.nfound, tarstats.nsuccess, tarstats.nerror);
		process_menu(MENU_ok, NULL);
		msg_clear();
	}

	/* Configure the system */
	if (set_status[SET_BASE] & SET_INSTALLED)
		run_makedev();

	/* Save keybard type */
	save_kb_encoding();

	/* Other configuration. */
	mnt_net_config();

	/* Mounted dist dir? */
	umount_mnt2();

	/* Install/Upgrade complete ... reboot or exit to script */
	msg_display(success_msg);
	process_menu(MENU_ok, NULL);
	return 0;
}

void
umount_mnt2(void)
{
	if (!mnt2_mounted)
		return;
	run_program(RUN_SILENT, "/sbin/umount /mnt2");
	mnt2_mounted = 0;
}


/*
 * Do a quick sanity check that  the target can reboot.
 * return 1 if everything OK, 0 if there is a problem.
 * Uses a table of files we expect to find after a base install/upgrade.
 */

/* test flag and pathname to check for after unpacking. */
struct check_table { unsigned int mode; const char *path;} checks[] = {
  { S_IFREG, "/netbsd" },
  { S_IFDIR, "/etc" },
  { S_IFREG, "/etc/fstab" },
  { S_IFREG, "/sbin/init" },
  { S_IFREG, "/bin/sh" },
  { S_IFREG, "/etc/rc" },
  { S_IFREG, "/etc/rc.subr" },
  { S_IFREG, "/etc/rc.conf" },
  { S_IFDIR, "/dev" },
  { S_IFCHR, "/dev/console" },
/* XXX check for rootdev in target /dev? */
  { S_IFREG, "/etc/fstab" },
  { S_IFREG, "/sbin/fsck" },
  { S_IFREG, "/sbin/fsck_ffs" },
  { S_IFREG, "/sbin/mount" },
  { S_IFREG, "/sbin/mount_ffs" },
  { S_IFREG, "/sbin/mount_nfs" },
#if defined(DEBUG) || defined(DEBUG_CHECK)
  { S_IFREG, "/foo/bar" },		/* bad entry to exercise warning */
#endif
  { 0, 0 }
  
};

/*
 * Check target for a single file.
 */
static int
check_for(unsigned int mode, const char *pathname)
{
	int found; 

	found = (target_test(mode, pathname) == 0);
	if (found == 0) 
		msg_display(MSG_rootmissing, pathname);
	return found;
}

/*
 * Check that all the files in check_table are present in the
 * target root. Warn if not found.
 */
int
sanity_check(void)
{
	int target_ok = 1;
	struct check_table *p;

	for (p = checks; p->path; p++) {
		target_ok = target_ok && check_for(p->mode, p->path);
	}
	if (target_ok)
		return 0;	    

	/* Uh, oh. Something's missing. */
	msg_display(MSG_badroot);
	process_menu(MENU_ok, NULL);
	return 1;
}

/*
 * Some globals to pass things back from callbacks
 */
static char zoneinfo_dir[STRSIZE];
static int zonerootlen;
static char *tz_selected;	/* timezonename (relative to share/zoneinfo */
static const char *tz_default;	/* UTC, or whatever /etc/localtime points to */
static char tz_env[STRSIZE];
static int save_cursel, save_topline;

/*
 * Callback from timezone menu
 */
static int
set_tz_select(menudesc *m, void *arg)
{
	time_t t;
	char *new;

	if (m && strcmp(tz_selected, m->opts[m->cursel].opt_name) != 0) {
		/* Change the displayed timezone */
		new = strdup(m->opts[m->cursel].opt_name);
		if (new == NULL)
			return 0;
		free(tz_selected);
		tz_selected = new;
		snprintf(tz_env, sizeof tz_env, "%.*s%s",
			 zonerootlen, zoneinfo_dir, tz_selected);
		setenv("TZ", tz_env, 1);
	}
	if (m)
		/* Warp curser to 'Exit' line on menu */
		m->cursel = -1;

	/* Update displayed time */
	t = time(NULL);
	msg_display(MSG_choose_timezone, 
		    tz_default, tz_selected, ctime(&t), localtime(&t)->tm_zone);
	return 0;
}

static int
set_tz_back(menudesc *m, void *arg)
{

	zoneinfo_dir[zonerootlen] = 0;
	m->cursel = save_cursel;
	m->topline = save_topline;
	return 0;
}

static int
set_tz_dir(menudesc *m, void *arg)
{

	strlcpy(zoneinfo_dir + zonerootlen, m->opts[m->cursel].opt_name,
		sizeof zoneinfo_dir - zonerootlen);
	save_cursel = m->cursel;
	save_topline = m->topline;
	m->cursel = 0;
	m->topline = 0;
	return 0;
}

/*
 * Alarm-handler to update example-display
 */
static void
/*ARGSUSED*/
timezone_sig(int sig)
{

	set_tz_select(NULL, NULL);
	alarm(60);
}

static int
tz_sort(const void *a, const void *b)
{
	return strcmp(((const menu_ent *)a)->opt_name, ((const menu_ent *)b)->opt_name);
}

static void
tzm_set_names(menudesc *m, void *arg)
{
	DIR *dir;
	struct dirent *dp;
	static int nfiles;
	static int maxfiles = 32;
	static menu_ent *tz_menu;
	static char **tz_names;
	void *p;
	int maxfname;
	char *fp;
	struct stat sb;

	if (tz_menu == NULL)
		tz_menu = malloc(maxfiles * sizeof *tz_menu);
	if (tz_names == NULL)
		tz_names = malloc(maxfiles * sizeof *tz_names);
	if (tz_menu == NULL || tz_names == NULL)
		return;	/* error - skip timezone setting */
	while (nfiles > 0)
		free(tz_names[--nfiles]);
	
	dir = opendir(zoneinfo_dir);
	fp = strchr(zoneinfo_dir, 0);
	if (fp != zoneinfo_dir + zonerootlen) {
		tz_names[0] = 0;
		tz_menu[0].opt_name = msg_string(MSG_tz_back);
		tz_menu[0].opt_menu = OPT_NOMENU;
		tz_menu[0].opt_flags = 0;
		tz_menu[0].opt_action = set_tz_back;
		nfiles = 1;
	}
	maxfname = zoneinfo_dir + sizeof zoneinfo_dir - fp - 1;
	if (dir != NULL) {
		while ((dp = readdir(dir)) != NULL) {
			if (dp->d_namlen > maxfname || dp->d_name[0] == '.')
				continue;
			strlcpy(fp, dp->d_name, maxfname);
			if (stat(zoneinfo_dir, &sb) == -1)
				continue;
			if (nfiles >= maxfiles) {
				p = realloc(tz_menu, 2 * maxfiles * sizeof *tz_menu);
				if (p == NULL)
					break;
				tz_menu = p;
				p = realloc(tz_names, 2 * maxfiles * sizeof *tz_names);
				if (p == NULL)
					break;
				tz_names = p;
				maxfiles *= 2;
			}
			if (S_ISREG(sb.st_mode))
				tz_menu[nfiles].opt_action = set_tz_select;
			else if (S_ISDIR(sb.st_mode)) {
				tz_menu[nfiles].opt_action = set_tz_dir;
				strlcat(fp, "/",
				    sizeof(zoneinfo_dir) - (fp - zoneinfo_dir));
			} else
				continue;
			tz_names[nfiles] = strdup(zoneinfo_dir + zonerootlen);
			tz_menu[nfiles].opt_name = tz_names[nfiles];
			tz_menu[nfiles].opt_menu = OPT_NOMENU;
			tz_menu[nfiles].opt_flags = 0;
			nfiles++;
		}
		closedir(dir);
	}
	*fp = 0;

	m->opts = tz_menu;
	m->numopts = nfiles;
	qsort(tz_menu, nfiles, sizeof *tz_menu, tz_sort);
}

/*
 * Choose from the files in usr/share/zoneinfo and set etc/localtime
 */
int
set_timezone(void)
{
	char localtime_link[STRSIZE];
	char localtime_target[STRSIZE];
	int rc;
	time_t t;
	int menu_no;
       
	strlcpy(zoneinfo_dir, target_expand("/usr/share/zoneinfo/"),
	    sizeof zoneinfo_dir - 1);
	zonerootlen = strlen(zoneinfo_dir);
	strlcpy(localtime_link, target_expand("/etc/localtime"),
	    sizeof localtime_link);

	/* Add sanity check that /mnt/usr/share/zoneinfo contains
	 * something useful
	 */

	rc = readlink(localtime_link, localtime_target,
		      sizeof(localtime_target) - 1);
	if (rc < 0) {
		/* error, default to UTC */
		tz_default = "UTC";
	} else {
		localtime_target[rc] = '\0';
		tz_default = strchr(strstr(localtime_target, "zoneinfo"), '/') + 1;
	}

	tz_selected = strdup(tz_default);
	snprintf(tz_env, sizeof(tz_env), "%s%s", zoneinfo_dir, tz_selected);
	setenv("TZ", tz_env, 1);
	t = time(NULL);
	msg_display(MSG_choose_timezone, 
		    tz_default, tz_selected, ctime(&t), localtime(&t)->tm_zone);

	signal(SIGALRM, timezone_sig);
	alarm(60);
	
	menu_no = new_menu(NULL, NULL, 14, 23, 9,
			   12, 32, MC_ALWAYS_SCROLL | MC_NOSHORTCUT,
			   tzm_set_names, NULL, NULL,
			   "\nPlease consult the install documents.", NULL);
	if (menu_no < 0)
		goto done;	/* error - skip timezone setting */
	
	process_menu(menu_no, NULL);

	free_menu(menu_no);

	signal(SIGALRM, SIG_IGN);

	snprintf(localtime_target, sizeof(localtime_target),
		 "/usr/share/zoneinfo/%s", tz_selected);
	unlink(localtime_link);
	symlink(localtime_target, localtime_link);
	
done:
	return 1;
}

int
set_crypt_type(void)
{
	FILE *pwc;
	char *fn;

	msg_display(MSG_choose_crypt);
	process_menu(MENU_crypttype, NULL);
	fn = strdup(target_expand("/etc/passwd.conf"));
	if (fn == NULL)
		return -1;

	switch (yesno) {
	case 0:
		break;
	case 1:	/* DES */
		rename(fn, target_expand("/etc/passwd.conf.pre-sysinst"));
		pwc = fopen(fn, "w");
		fprintf(pwc,
		    "default:\n"
		    "  localcipher = old\n"
		    "  ypcipher = old\n");
		fclose(pwc);
		break;
	case 2:	/* MD5 */
		rename(fn, target_expand("/etc/passwd.conf.pre-sysinst"));
		pwc = fopen(fn, "w");
		fprintf(pwc,
		    "default:\n"
		    "  localcipher = md5\n"
		    "  ypcipher = md5\n");
		fclose(pwc);
		break;
	case 3:	/* blowfish 2^7 */
		rename(fn, target_expand("/etc/passwd.conf.pre-sysinst"));
		pwc = fopen(fn, "w");
		fprintf(pwc,
		    "default:\n"
		    "  localcipher = blowfish,7\n"
		    "  ypcipher = blowfish,7\n");
		fclose(pwc);
		break;
	}

	free(fn);
	return (0);
}

int
set_root_password(void)
{

	msg_display(MSG_rootpw);
	process_menu(MENU_yesno, NULL);
	if (yesno)
		run_program(RUN_DISPLAY | RUN_PROGRESS | RUN_CHROOT,
			    "passwd -l root");
	return 0;
}

int
set_root_shell(void)
{
	const char *shellpath;

	msg_display(MSG_rootsh);
	process_menu(MENU_rootsh, &shellpath);
	run_program(RUN_DISPLAY | RUN_CHROOT, "chpass -s %s root", shellpath);
	return 0;
}

void
scripting_vfprintf(FILE *f, const char *fmt, va_list ap)
{

	if (f)
		(void)vfprintf(f, fmt, ap);
	if (scripting)
		(void)vfprintf(script, fmt, ap);
}

void
scripting_fprintf(FILE *f, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	scripting_vfprintf(f, fmt, ap);
	va_end(ap);
}

void
add_rc_conf(const char *fmt, ...)
{
	FILE *f;
	va_list ap;

	va_start(ap, fmt);
	f = target_fopen("/etc/rc.conf", "a");
	if (f != 0) {
		scripting_fprintf(NULL, "cat <<EOF >>%s/etc/rc.conf\n",
		    target_prefix());
		scripting_vfprintf(f, fmt, ap);
		fclose(f);
		scripting_fprintf(NULL, "EOF\n");
	}
	va_end(ap);
}

void
enable_rc_conf(void)
{
	run_program(RUN_CHROOT,
		    "sed -an -e 's/^rc_configured=NO/rc_configured=YES/;"
				    "H;$!d;g;w /etc/rc.conf' /etc/rc.conf");
}

int
check_lfs_progs(void)
{

	return (access("/sbin/fsck_lfs", X_OK) == 0 &&
		access("/sbin/mount_lfs", X_OK) == 0 &&
		access("/sbin/newfs_lfs", X_OK) == 0);
}
