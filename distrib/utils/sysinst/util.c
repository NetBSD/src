/*	$NetBSD: util.c,v 1.103 2003/07/10 13:36:48 dsl Exp $	*/

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

distinfo dist_list[] = {
#ifdef SET_KERNEL_1_NAME
	{SET_KERNEL_1_NAME,	SET_KERNEL_1,		MSG_set_kernel_1},
#endif
#ifdef SET_KERNEL_2_NAME
	{SET_KERNEL_2_NAME,	SET_KERNEL_2,		MSG_set_kernel_2},
#endif
#ifdef SET_KERNEL_3_NAME
	{SET_KERNEL_3_NAME,	SET_KERNEL_3,		MSG_set_kernel_3},
#endif
#ifdef SET_KERNEL_4_NAME
	{SET_KERNEL_4_NAME,	SET_KERNEL_4,		MSG_set_kernel_4},
#endif
#ifdef SET_KERNEL_5_NAME
	{SET_KERNEL_5_NAME,	SET_KERNEL_5,		MSG_set_kernel_5},
#endif
#ifdef SET_KERNEL_6_NAME
	{SET_KERNEL_6_NAME,	SET_KERNEL_6,		MSG_set_kernel_6},
#endif
#ifdef SET_KERNEL_7_NAME
	{SET_KERNEL_7_NAME,	SET_KERNEL_7,		MSG_set_kernel_7},
#endif
#ifdef SET_KERNEL_8_NAME
	{SET_KERNEL_8_NAME,	SET_KERNEL_8,		MSG_set_kernel_8},
#endif
	{"base",		SET_BASE,		MSG_set_base},
	{"etc",			SET_ETC,		MSG_set_system},
	{"comp",		SET_COMPILER,		MSG_set_compiler},
	{"games",		SET_GAMES,		MSG_set_games},
	{"man",			SET_MAN_PAGES,		MSG_set_man_pages},
	{"misc",		SET_MISC,		MSG_set_misc},
	{"text",		SET_TEXT_TOOLS,		MSG_set_text_tools},
	{NULL,			SET_X11,		MSG_set_X11},
	{"xbase",		SET_X11_BASE,		MSG_set_X11_base},
	{"xfont",		SET_X11_FONTS,		MSG_set_X11_fonts},
	{"xserver",		SET_X11_SERVERS,	MSG_set_X11_servers},
	{"xcontrib",		SET_X_CONTRIB,		MSG_set_X_contrib},
	{"xcomp",		SET_X11_PROG,		MSG_set_X11_prog},
	{"xmisc",		SET_X11_MISC,		MSG_set_X11_misc},
#ifdef SET_MD_1_NAME
	{SET_MD_1_NAME,		SET_MD_1,		MSG_set_md_1},
#endif
#ifdef SET_MD_2_NAME
	{SET_MD_2_NAME,		SET_MD_2,		MSG_set_md_2},
#endif
#ifdef SET_MD_3_NAME
	{SET_MD_3_NAME,		SET_MD_3,		MSG_set_md_3},
#endif
#ifdef SET_MD_4_NAME
	{SET_MD_4_NAME,		SET_MD_4,		MSG_set_md_4},
#endif
	{NULL,			0,			NULL},
};

/*
 * local prototypes 
 */
struct  tarstats {
	int nselected;
	int nfound;
	int nnotfound;
	int nerror;
	int nsuccess;
	int nskipped;
} tarstats;

int	extract_file (char *path);
int	extract_dist (void);
int	distribution_sets_exist_p (const char *path);
static int check_for (unsigned int mode, const char *pathname);

#ifndef MD_SETS_SELECTED
#define MD_SETS_SELECTED (SET_KERNEL_1 | SET_SYSTEM | SET_X11 | SET_MD)
#endif
#ifndef MD_SETS_VALID
#define MD_SETS_VALID (SET_KERNEL | SET_SYSTEM | SET_X11 | SET_MD)
#endif

unsigned int sets_valid = MD_SETS_VALID;
unsigned int sets_selected = (MD_SETS_SELECTED) & (MD_SETS_VALID);


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

	return (stat(path, &st) == 0 && (st.st_mode & mode) != 0);
}

int
distribution_sets_exist_p(const char *path)
{
	char buf[STRSIZE];
	int result;

	result = 1;
	snprintf(buf, sizeof buf, "%s/%s", path, "etc.tgz");
	result = result && file_exists_p(buf);

	snprintf(buf, sizeof buf, "%s/%s", path, "base.tgz");
	result = result && file_exists_p(buf);

	return(result);
}


void
get_ramsize(void)
{
	size_t len = sizeof(ramsize);
	int mib[2] = {CTL_HW, HW_PHYSMEM};
	
	sysctl(mib, 2, &ramsize, &len, NULL, 0);

	/* Find out how many Megs ... round up. */
	rammb = ((unsigned int)ramsize + MEG - 1) / MEG;
}

static int asked = 0;

void
ask_sizemult(int cylsize)
{

	current_cylsize = cylsize;	/* XXX */

	if (!asked) {
		msg_display(MSG_sizechoice);
		process_menu(MENU_sizechoice, NULL);
	}
	asked = 1;
}

void
reask_sizemult(int cylsize)
{

	asked = 0;
	ask_sizemult(cylsize);
}

void
run_makedev(void)
{
	char *owd;

	wclear(stdscr);
	wrefresh(stdscr);
	msg_display(MSG_makedev);
	sleep (1);

	owd = getcwd(NULL, 0);

	/* make /dev, in case the user  didn't extract it. */
	make_target_dir("/dev");
	target_chdir_or_die("/dev");
	run_prog(0, NULL, "/bin/sh MAKEDEV all");

	chdir(owd);
	free(owd);
}


/*
 * Load files from floppy.  Requires a /mnt2 directory for mounting them.
 */
int
get_via_floppy(void)
{
	char distname[STRSIZE];
	char fddev[STRSIZE] = "/dev/fd0a";
	char fname[STRSIZE];
	char full_name[STRSIZE];
	char catcmd[STRSIZE];
	distinfo *list;
	char post[4];
	int  mounted = 0;
	int  first;
	struct stat sb;

	cd_dist_dir("unloading from floppy");

	msg_prompt_add(MSG_fddev, fddev, fddev, STRSIZE);

	list = dist_list;
	while (list->desc) {
		if (list->name == NULL) {
			list++;
			continue;
		}
		strcpy(post, ".aa");
		snprintf(distname, sizeof distname, "%s%s",
		    list->name, dist_postfix);
		while (sets_selected & list->set) {
			snprintf(fname, sizeof fname, "%s%s", list->name, post);
			snprintf(full_name, sizeof full_name, "/mnt2/%s",
				 fname);
			first = 1;
			while (!mounted || stat(full_name, &sb)) {
 				if (mounted) 
				  run_prog(0, NULL, "/sbin/umount /mnt2");
				if (first)
					msg_display(MSG_fdmount, fname);
				else
					msg_display(MSG_fdnotfound, fname);
				process_menu(MENU_fdok, NULL);
				if (!yesno)
					return 0;
				else if (yesno == 2)
					return 1;
				while (run_prog(0, NULL, 
				    "/sbin/mount -r -t %s %s /mnt2",
				    fdtype, fddev)) {
					msg_display(MSG_fdremount, fname);
					process_menu(MENU_fdremount, NULL);
					if (!yesno)
						return 0;
					else if (yesno == 2)
						return 1;
				}
				mounted = 1;
				first = 0;
			}
			sprintf(catcmd, "/bin/cat %s >> %s",
				full_name, distname);
			if (logging)
				(void)fprintf(logfp, "%s\n", catcmd);
			if (scripting)
				(void)fprintf(script, "%s\n", catcmd);
			do_system(catcmd);
			if (post[2] < 'z')
				post[2]++;
			else
				post[2] = 'a', post[1]++;
		}
		run_prog(0, NULL, "/sbin/umount /mnt2");
		mounted = 0;
		list++;
	}
#ifndef DEBUG
	chdir("/");	/* back to current real root */
#endif
	return 1;
}

/*
 * Get from a CDROM distribution.
 */
int
get_via_cdrom(void)
{
	char tmpdir[STRSIZE];
	int retries = 0;

	/* Get CD-rom device name and path within CD-rom */
	process_menu(MENU_cdromsource, NULL);

again:
	run_prog(0, NULL, "/sbin/umount /mnt2");

	/* Mount it */
	if (run_prog(0, NULL, "/sbin/mount -rt cd9660 /dev/%s%c /mnt2",
	    cdrom_dev, 'a' + getrawpartition())) {
		if (retries++ < 5) {
			sleep(1);
			goto again;
		}
		msg_display(MSG_badsetdir, cdrom_dev);
		process_menu(MENU_cdrombadmount, NULL);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	snprintf(tmpdir, sizeof tmpdir, "%s/%s", "/mnt2", cdrom_dir);

	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(tmpdir) == 0) {
		msg_display(MSG_badsetdir, tmpdir);
		process_menu(MENU_cdrombadmount, NULL);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}

	/* return location, don't clean... */
	strlcpy(ext_dir, tmpdir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}


/*
 * Get from a pathname inside an unmounted local filesystem
 * (e.g., where sets were preloaded onto a local DOS partition) 
 */
int
get_via_localfs(void)
{
	char tmpdir[STRSIZE];

	/* Get device, filesystem, and filepath */
	process_menu (MENU_localfssource, NULL);

again:
	run_prog(0, NULL, "/sbin/umount /mnt2");

	/* Mount it */
	if (run_prog(0, NULL, "/sbin/mount -rt %s /dev/%s /mnt2",
	    localfs_fs, localfs_dev)) {

		msg_display(MSG_localfsbadmount, localfs_dir, localfs_dev); 
		process_menu(MENU_localfsbadmount, NULL);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	snprintf(tmpdir, sizeof tmpdir, "%s/%s", "/mnt2", localfs_dir);

	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(tmpdir) == 0) {
		msg_display(MSG_badsetdir, tmpdir);
		process_menu(MENU_localfsbadmount, NULL);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	/* return location, don't clean... */
	strlcpy(ext_dir, tmpdir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}

/*
 * Get from an already-mounted pathname.
 */

int
get_via_localdir(void)
{

	/* Get device, filesystem, and filepath */
	process_menu(MENU_localdirsource, NULL);

again:
	/* Complain if not a directory */
	if (dir_exists_p(localfs_dir) == 0) {

		msg_display(MSG_badlocalsetdir, localfs_dir);
		process_menu(MENU_localdirbad, NULL);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}
	
	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(localfs_dir) == 0) {
		msg_display(MSG_badsetdir, localfs_dir);
		process_menu(MENU_localdirbad, NULL);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}

	/* return location, don't clean... */
	strlcpy(ext_dir, localfs_dir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 0;
	return 1;
}


void
cd_dist_dir(char *forwhat)
{

	/* ask user for the mountpoint. */
	msg_prompt(MSG_distdir, dist_dir, dist_dir, STRSIZE, forwhat);

	/* make sure the directory exists. */
	make_target_dir(dist_dir);

	clean_dist_dir = 1;
	target_chdir_or_die(dist_dir);

	/* Set ext_dir for absolute path. */
	getcwd(ext_dir, sizeof ext_dir);
}


/*
 * Support for custom distribution fetches / unpacks.
 */

typedef struct {
	distinfo 		*dist;
	unsigned int		sets;
	struct info {
	    unsigned int	set;
	    char		label[44];
	} i[32];
} set_menu_info_t;

static int
set_toggle(menudesc *menu, menu_ent *ent, void *arg)
{
	set_menu_info_t *i = arg;
	int set = i->i[ent - menu->opts].set;

	if (set & SET_KERNEL)
		/* only one kernel set is allowed */
		sets_selected &= ~SET_KERNEL | set;
	sets_selected ^= set;
	return 0;
}

static int
set_all(menudesc *menu, menu_ent *ent, void *arg)
{
	set_menu_info_t *i = arg;

	sets_selected |= i->sets;
	return 1;
}

static int
set_none(menudesc *menu, menu_ent *ent, void *arg)
{
	set_menu_info_t *i = arg;

	sets_selected &= ~i->sets;
	return 1;
}

static int set_sublist(menudesc *menu, menu_ent *ent, void *arg);

static void
set_selected_sets(menudesc *menu, void *arg)
{
	distinfo *list;
	static const char *yes, *no, *all, *some, *none;
	const char *selected;
	menu_ent *m;
	set_menu_info_t *menu_info = arg;
	struct info *i = menu_info->i;
	unsigned int set;

	if (yes == NULL) {
		yes = msg_string(MSG_Yes);
		no = msg_string(MSG_No);
		all = msg_string(MSG_All);
		some = msg_string(MSG_Some);
		none = msg_string(MSG_None);
	}

	msg_display(MSG_cur_distsets);
	msg_table_add(MSG_cur_distsets_header);

	m = menu->opts;
	for (list = menu_info->dist; list->desc; list++) {
		if (!(menu_info->sets & list->set))
			break;
		if (!(sets_valid & list->set))
			continue;
		i->set = list->set;
		m->opt_menu = OPT_NOMENU;
		m->opt_flags = 0;
		m->opt_name = i->label;
		m->opt_action = set_toggle;
		if (list->set & (list->set - 1)) {
			/* multiple bits possible */
			set = list->set & sets_valid;
			selected = (set & sets_selected) == 0 ? none :
				(set & sets_selected) == set ? all : some;
		} else {
			selected = list->set & sets_selected ? yes : no;;
		}
		snprintf(i->label, sizeof i->label,
			msg_string(MSG_cur_distsets_row),
			msg_string(list->desc), selected);
		m++;
		i++;
		if (list->name != NULL)
			continue;
		m[-1].opt_action = set_sublist;
		/* collapsed sublist */
		set = list->set;
		while (list[1].set & set)
			list++;
	}

	if (menu_info->sets == ~0u)
		return;

	m->opt_menu = OPT_NOMENU;
	m->opt_flags = 0;
	m->opt_name = MSG_select_all;
	m->opt_action = set_all;
	m++;
	m->opt_menu = OPT_NOMENU;
	m->opt_flags = 0;
	m->opt_name = MSG_select_none;
	m->opt_action = set_none;
}

static int
set_sublist(menudesc *menu, menu_ent *ent, void *arg)
{
	distinfo *list;
	menu_ent me[32];
	set_menu_info_t set_menu_info;
	int sets;
	int menu_no;
	unsigned int set;
	set_menu_info_t *i = arg;

	set = i->i[ent - menu->opts].set;
	set_menu_info.sets = set;

	/* Count number of entries we require */
	for (list = dist_list; list->set != set; list++)
		if (list->desc == NULL)
			return 0;
	set_menu_info.dist = ++list;
	for (sets = 2; list->set & set; list++)
		if (sets_valid & list->set)
			sets++;

	if (sets > nelem(me)) {
		/* panic badly */
		return 0;
	}

	menu_no = new_menu(NULL, me, sets, 20, 10, 0, 44,
		MC_SCROLL | MC_DFLTEXIT,
		set_selected_sets, NULL, NULL, NULL, MSG_install_selected_sets);

	if (menu_no == -1)
		return 0;

	process_menu(menu_no, &set_menu_info);
	free_menu(menu_no);

	return 0;
}

void
customise_sets(void)
{
	distinfo *list;
	menu_ent me[32];
	set_menu_info_t set_menu_info;
	int sets;
	int menu_no;
	unsigned int set, valid = 0;

	/* Count number of entries we require */
	for (sets = 0, list = dist_list; list->desc != NULL; list++) {
		if (!(sets_valid & list->set))
			continue;
		sets++;
		if (list->name != NULL) {
			valid |= list->set;
			continue;
		}
		/* collapsed sublist */
		set = list->set;
		while (list[1].set & set) {
			valid |= list[1].set;
			list++;
		}
	}
	if (sets > nelem(me)) {
		/* panic badly */
		return;
	}

	/* Static initialisation is lazy, fix it now */
	sets_valid &= valid;
	sets_selected &= valid;

	menu_no = new_menu(NULL, me, sets, 0, 5, 0, 44,
		MC_SCROLL | MC_NOBOX | MC_DFLTEXIT | MC_NOCLEAR,
		set_selected_sets, NULL, NULL, NULL, MSG_install_selected_sets);

	if (menu_no == -1)
		return;

	set_menu_info.dist = dist_list;
	set_menu_info.sets = ~0u;
	process_menu(menu_no, &set_menu_info);
	free_menu(menu_no);
}

/* Do we want a verbose extract? */
static	int verbose = -1;

void
ask_verbose_dist(void)
{

	if (verbose < 0) {
		msg_display(MSG_verboseextract);
		process_menu(MENU_extract, NULL);
		verbose = yesno;
		wclear(stdscr);
		wrefresh(stdscr);
	}
}

int
extract_file(char *path)
{
	char *owd;
	int   tarexit, rv;
	
	owd = getcwd (NULL,0);

	/* check tarfile exists */
	if (!file_exists_p(path)) {
		tarstats.nnotfound++;

		msg_display(MSG_notarfile, path);
		process_menu(MENU_noyes, NULL);
		return (yesno == 0);
	}

	tarstats.nfound++;	
	/* cd to the target root. */
	target_chdir_or_die("/");	

	/* now extract set files files into "./". */
	if (verbose==1)
	  tarexit = run_prog(RUN_DISPLAY, NULL,
	    "progress -zf %s tar -xepf -", path);
	else if (verbose==2)
	  tarexit = run_prog(RUN_DISPLAY, NULL,
	    "tar -zxvepf %s", path);
	else
	  tarexit = run_prog(RUN_DISPLAY, NULL,
	    "tar -zxepf %s", path);

	/* Check tarexit for errors and give warning. */
	if (tarexit) {
		tarstats.nerror++;

		msg_display(MSG_tarerror, path);
		process_menu(MENU_noyes, NULL);
		rv = (yesno == 0);
	} else {
		tarstats.nsuccess++;
		rv = 0;
	}
	
	chdir(owd);
	free(owd);

	return (rv);
}


/*
 * Extract_dist **REQUIRES** an absolute path in ext_dir.  Any code
 * that sets up dist_dir for use by extract_dist needs to put in the
 * full path name to the directory. 
 */

int
extract_dist(void)
{
	char distname[STRSIZE];
	char fname[STRSIZE];
	distinfo *list;
	int punt;

	/* reset failure/success counters */
	memset(&tarstats, 0, sizeof(tarstats));

	/*endwin();*/
	for (punt = 0, list = dist_list; list->desc != NULL; list++) {
		if (list->name == NULL)
			continue;
		if (sets_selected & list->set) {
			tarstats.nselected++;
			if (punt) {
				tarstats.nskipped++;
				continue;
			}
#if 0
			if (cleanup_dist(list->name) == 0) {
				msg_display(MSG_cleanup_warn);
				process_menu(MENU_ok, NULL);
			}
#endif
			(void)snprintf(distname, sizeof distname, "%s%s",
			    list->name, dist_postfix);
			(void)snprintf(fname, sizeof fname, "%s/%s",
			    ext_dir, distname);

			/* if extraction failed and user aborted, punt. */
			punt = extract_file(fname);
		}
	}

	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	if (tarstats.nerror == 0 && tarstats.nsuccess == tarstats.nselected) {
		msg_display(MSG_endtarok);
		process_menu(MENU_ok, NULL);
		return 0;
	} else {
		/* We encountered  errors. Let the user know. */
		msg_display(MSG_endtar,
		    tarstats.nselected, tarstats.nnotfound, tarstats.nskipped,
		    tarstats.nfound, tarstats.nsuccess, tarstats.nerror);
		process_menu(MENU_ok, NULL);
		return 1;
	}
}

#if 0	/* { NOMORE */

/*
 * Do pre-extract cleanup for set 'name':
 * open a file named '/var/db/obsolete/<name>', which contain a list of
 * files to kill from the target. For each file, test if it is present on
 * the target. Then display the list of files which will be removed,
 * ask user for confirmation, and process.
 * Non-empty directories will be renamed to <directory.old>.
 */

/* definition for a list of files. */
struct filelist {
	struct filelist *next;
	char name[MAXPATHLEN];
	mode_t type;
};

int 
cleanup_dist(const char *name)
{
	char file_path[MAXPATHLEN];
	char file_name[MAXPATHLEN];
	const char *file_prefix;
	FILE *list_file;
	struct filelist *head = NULL;
	struct filelist *current;
	int saved_errno;
	struct stat st;
	int retval = 1;
	int needok = 0;

	snprintf(file_path, MAXPATHLEN, "/var/db/obsolete/%s", name);
	list_file = fopen(file_path, "r");
	if (list_file == NULL) {
		saved_errno = errno;
		if (logging)
			fprintf(logfp, "Open of %s failed: %s\n", file_path,
			    strerror(saved_errno));
		if (saved_errno == ENOENT)
			return 1;
		msg_display_add(MSG_openfail, name, strerror(saved_errno));
		process_menu(MENU_ok, NULL);
		return 0;
	}
	file_prefix = target_prefix();
	while (fgets(file_name, MAXPATHLEN, list_file)) {
		/* Remove trailing \n if any */
		if (file_name[strlen(file_name)-1] == '\n')
			file_name[strlen(file_name)-1] = '\0';
		snprintf(file_path, MAXPATHLEN, "%s/%s", file_prefix,
		    file_name);
		if (lstat(file_path, &st) != 0) {
			saved_errno = errno;
			if (logging)
				fprintf(logfp, "stat() of %s failed: %s\n",
				    file_path, strerror(saved_errno));
			if (saved_errno == ENOENT)
				continue;
			msg_display_add(MSG_statfail, file_path,
			    strerror(saved_errno));
			process_menu(MENU_ok, NULL);
			return 0;
		}
		if (head == NULL) {
			head = current = malloc(sizeof(struct filelist));
			if (head == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(1);
			}
		} else {
			current->next = malloc(sizeof(struct filelist));
			if (current->next == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(1);
			}
			current = current->next;
		}
		current->next = NULL;
		snprintf(current->name, MAXPATHLEN, "%s", file_path);
		current->type = st.st_mode & S_IFMT;
		if (logging)
			fprintf(logfp, "Adding file %s, type %d to list of "
			    "obsolete file\n", current->name, current->type);
	}
	fclose(list_file);
	if (head == NULL)
		return 1;
#if 0
	/* XXX doesn't work, too many files printed ! */
	msg_display(MSG_deleting_files);
	for (current = head; current != NULL; current = current->next) {
		if (current->type != S_IFDIR) {
			/* XXX msg_printf_add going/gone away */
			msg_printf_add("%s ", current->name);
		}
	}
	msg_display_add(MSG_deleting_dirs);
	for (current = head; current != NULL; current = current->next) {
		if (current->type == S_IFDIR) {
			/* XXX msg_printf_add going/gone away */
			msg_printf_add("%s ", current->name);
		}
	}
	process_menu(MENU_ok, NULL);
#endif
	/* first remove files */
	for (current = head; current != NULL; current = current->next) {
		if (current->type == S_IFDIR)
			continue;
		if (scripting)
			(void)fprintf(script, "rm %s\n", current->name);
		if (unlink(current->name) != 0) {
			saved_errno = errno;
			if (saved_errno == ENOENT)
				continue;	/* don't worry about
						   non-existing files */
			if (logging)
				fprintf(logfp, "rm %s failed: %s\n",
				    current->name, strerror(saved_errno));
			msg_display_add(MSG_unlink_fail, current->name,
			    strerror(saved_errno));
			retval = 0;
			needok = 1;
		}

	}
	/* now dirs */
	for (current = head; current != NULL; current = current->next) {
		if (current->type != S_IFDIR)
			continue;
		if (rmdir(current->name) == 0) {
			if (scripting)
				(void)fprintf(script, "rmdir %s\n",
				    current->name);
			continue;
		}
		saved_errno = errno;
		if (saved_errno == ENOTEMPTY) {
			if (logging)
				fprintf(logfp, "dir %s not empty, "
				    "trying to rename to %s.old\n",
				    current->name, current->name);
			snprintf(file_path, MAXPATHLEN,
			    "%s.old", current->name);
			if (scripting)
				(void)fprintf(script, "mv %s %s\n",
				    current->name, file_path);
			needok = 1;
			if (rename(current->name, file_path) != 0) {
				saved_errno = errno;
				if (logging)
					fprintf(logfp, "mv %s %s failed: %s\n", 
					    current->name, file_path,
					    strerror(saved_errno));
				msg_display_add(MSG_rename_fail, current->name,
				    file_path, strerror(errno));
				 retval = 0;
			}
			msg_display_add(MSG_renamed_dir, current->name,
			    file_path);
		} else { /* rmdir error */
			/*
			 * Don't worry about non-existing directories.
			 */
			if (saved_errno == ENOENT)
				continue;
			if (logging)
				fprintf(logfp, "rm %s failed: %s\n",
				    current->name, strerror(saved_errno));
			msg_display_add(MSG_unlink_fail, current->name,
			    strerror(saved_errno));
			retval = 0;
			needok = 1;
		}
	}
	if (needok)
		process_menu(MENU_ok, NULL);
	return retval;
}
#endif	/* } NOMORE */

/*
 * Get and unpack the distribution.
 * show success_msg if installation completes. Otherwise,,
 * show failure_msg and wait for the user to ack it before continuing.
 * success_msg and failure_msg must both be 0-adic messages.
 */
int
get_and_unpack_sets(msg success_msg, msg failure_msg)
{

	/* Ensure mountpoint for distribution files exists in current root. */
	(void) mkdir("/mnt2", S_IRWXU| S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH);
	if (scripting)
		(void)fprintf(script, "mkdir /mnt2\nchmod 755 /mnt2\n");

	/* Find out which files to "get" if we get files. */
	wclear(stdscr);
	wrefresh(stdscr);

	/* ask user whether to do normal or verbose extraction */
	ask_verbose_dist();

	/* Get the distribution files */
	do {
		got_dist = 0;
		process_menu(MENU_distmedium, NULL);
	} while (got_dist == -1);

	if (nodist)
		return 1;

	if (got_dist) {

		/* Extract the distribution, abort on errors. */
		if (extract_dist())
			return 1;

		/* Configure the system */
		if (sets_selected & SET_ETC)
			run_makedev();

		/* Other configuration. */
		mnt_net_config();
		
		/* Clean up dist dir (use absolute path name) */
		if (clean_dist_dir && ext_dir[0] == '/' && ext_dir[1] != 0)
			run_prog(0, NULL, "/bin/rm -rf %s", ext_dir);

		/* Mounted dist dir? */
		if (mnt2_mounted)
			run_prog(0, NULL, "/sbin/umount /mnt2");

		/* Install/Upgrade complete ... reboot or exit to script */
		msg_display(success_msg);
		process_menu(MENU_ok, NULL);
		return 0;
	}

	msg_display(failure_msg);
	process_menu(MENU_ok, NULL);
	return 1;
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

#ifdef notdef
/* set reverse to 1 to default to no */
int
askyesno(int reverse)
{
	WINDOW *yesnowin;
	int c, found;

	yesnowin = subwin(stdscr, 5, 20, getmaxy(stdscr)/2 - 2, getmaxx(stdscr)/2 - 10);
	if (yesnowin == NULL) {
		fprintf(stderr, "sysinst: failed to allocate yes/no box\n");
		exit(1);
	}
	box(yesnowin, '*', '*');
	wmove(yesnowin, 2,2);
	
	if (reverse)
		waddstr(yesnowin, "Yes or No: [N]");
	else
		waddstr(yesnowin, "Yes or No: [Y]");

	wrefresh(yesnowin);
	while ((c = getchar()) != 0) {
		if (c == 'y' || c == 'Y') {
			found = 1;
			break;
		} else if (c == 'n' || c == 'N' ) {
			found = 0;
			break;
		} else if (c == '\n' || c == '\r') {
			if (reverse)
				found = 0;
			else
				found = 1;
			break;
		}
	}
	wclear(yesnowin);
	wrefresh(yesnowin);
	delwin(yesnowin);
	refresh();
	return(found);
}
#endif

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
set_tz_select(menudesc *m, menu_ent *opt, void *arg)
{
	time_t t;
	char *new;

	if (m) {
		new = strdup(m->opts[m->cursel].opt_name);
		if (new == NULL)
			return 0;
		free(tz_selected);
		tz_selected = new;
	}
	snprintf(tz_env, sizeof tz_env, "%*s%s",
		 zonerootlen, zoneinfo_dir, tz_selected);
	setenv("TZ", tz_env, 1);
	t = time(NULL);
	msg_display(MSG_choose_timezone, 
		    tz_default, tz_selected, ctime(&t), localtime(&t)->tm_zone);
	return 0;
}

static int
set_tz_back(menudesc *m, menu_ent *opt, void *arg)
{

	zoneinfo_dir[zonerootlen] = 0;
	m->cursel = save_cursel;
	m->topline = save_topline;
	return 0;
}

static int
set_tz_dir(menudesc *m, menu_ent *opt, void *arg)
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

	set_tz_select(NULL, NULL, NULL);
	alarm(60);
}

static int
tz_sort(const void *a, const void *b)
{
	return strcmp(((menu_ent *)a)->opt_name, ((menu_ent *)b)->opt_name);
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
				strcat(fp, "/");
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
			   12, 32, MC_SCROLL | MC_NOSHORTCUT,
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
		run_prog(RUN_DISPLAY|RUN_CHROOT, NULL, "passwd -l root");
	return 0;
}

int
set_root_shell(void)
{

	msg_display(MSG_rootsh);
	process_menu(MENU_rootsh, NULL);
	run_prog(RUN_DISPLAY|RUN_CHROOT, NULL, "chpass -s %s root", shellpath);
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

int
check_lfs_progs(void)
{

	return (access("/sbin/dump_lfs", X_OK) == 0 &&
		access("/sbin/fsck_lfs", X_OK) == 0 &&
		access("/sbin/mount_lfs", X_OK) == 0 &&
		access("/sbin/newfs_lfs", X_OK) == 0);
}
