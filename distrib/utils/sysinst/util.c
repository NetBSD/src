/*	$NetBSD: util.c,v 1.68 2002/05/24 08:07:50 itojun Exp $	*/

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
#include <fts.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

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
int	cleanup_dist (const char *path);
int	distribution_sets_exist_p (const char *path);
static int check_for (unsigned int mode, const char *pathname);

int
dir_exists_p(path)
	const char *path;
{
	return file_mode_match(path, S_IFDIR);
}

int
file_exists_p(path)
	const char *path;
{
	return file_mode_match(path, S_IFREG);
}

int
file_mode_match(path, mode)
	const char *path;
	unsigned int mode;
{
	struct stat st;

	return (stat(path, &st) == 0 && (st.st_mode & mode) != 0);
}

int
distribution_sets_exist_p(path)
	const char *path;
{
	char buf[STRSIZE];
	int result;

	result = 1;
	snprintf(buf, STRSIZE, "%s/%s", path, "etc.tgz");
	result = result && file_exists_p(buf);

	snprintf(buf, STRSIZE, "%s/%s", path, "base.tgz");
	result = result && file_exists_p(buf);

	return(result);
}


void
get_ramsize()
{
	long len = sizeof(long);
	int mib[2] = {CTL_HW, HW_PHYSMEM};
	
	sysctl(mib, 2, (void *)&ramsize, (size_t *)&len, NULL, 0);

	/* Find out how many Megs ... round up. */
	rammb = (ramsize + MEG - 1) / MEG;
}

static int asked = 0;

void
ask_sizemult(cylsize)
	int cylsize;
{
	current_cylsize = cylsize;	/* XXX */

	if (!asked) {
		msg_display(MSG_sizechoice);
		process_menu(MENU_sizechoice);
	}
	asked = 1;
}

void
reask_sizemult(cylsize)
	int cylsize;
{

	asked = 0;
	ask_sizemult(cylsize);
}

void
run_makedev()
{
	char *owd;

	wclear(stdscr);
	wrefresh(stdscr);
	msg_display(MSG_makedev);
	sleep (1);

	owd = getcwd(NULL,0);

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
get_via_floppy()
{
	char distname[STRSIZE];
	char fddev[STRSIZE] = "/dev/fd0a";
	char fname[STRSIZE];
	char fullname[STRSIZE];
	char catcmd[STRSIZE];
	distinfo *list;
	char post[4];
	int  mounted = 0;
	int  first;
	struct stat sb;

	cd_dist_dir("unloading from floppy");

	msg_prompt_add(MSG_fddev, fddev, fddev, STRSIZE);

	list = dist_list;
	while (list->name) {
		strcpy(post, ".aa");
		snprintf(distname, STRSIZE, "%s%s", list->name, dist_postfix);
		while (list->getit && strcmp(&post[1],list->fdlast) <= 0) {
			snprintf(fname, STRSIZE, "%s%s", list->name, post);
			snprintf(fullname, STRSIZE, "/mnt2/%s", fname);
			first = 1;
			while (!mounted || stat(fullname, &sb)) {
 				if (mounted) 
				  run_prog(0, NULL, "/sbin/umount /mnt2");
				if (first)
					msg_display(MSG_fdmount, fname);
				else
					msg_display(MSG_fdnotfound, fname);
				process_menu(MENU_fdok);
				if (!yesno)
					return 0;
				while (run_prog(0, NULL, 
				    "/sbin/mount -r -t %s %s /mnt2",
				    fdtype, fddev)) {
					msg_display(MSG_fdremount, fname);
					process_menu(MENU_fdremount);
					if (!yesno)
						return 0;
				}
				mounted = 1;
				first = 0;
			}
			sprintf(catcmd, "/bin/cat %s >> %s",
				fullname, distname);
			if (logging)
				(void)fprintf(log, "%s\n", catcmd);
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
get_via_cdrom()
{
	char tmpdir[STRSIZE];

	/*
	 * Fill in final default path, similar to ftp path
	 * because we expect the CDROM structure to be the
	 * same as the ftp site.
	 */
	strncat(cdrom_dir, machine, STRSIZE - strlen(cdrom_dir));
	strncat(cdrom_dir, ftp_prefix, STRSIZE - strlen(cdrom_dir));

	/* Get CD-rom device name and path within CD-rom */
	process_menu(MENU_cdromsource);

again:
	run_prog(0, NULL, "/sbin/umount /mnt2");

	/* Mount it */
	if (run_prog(0, NULL,
	    "/sbin/mount -rt cd9660 /dev/%sa /mnt2", cdrom_dev)) {
		msg_display(MSG_badsetdir, cdrom_dev);
		process_menu(MENU_cdrombadmount);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	snprintf(tmpdir, STRSIZE, "%s/%s", "/mnt2", cdrom_dir);

	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(tmpdir) == 0) {
		msg_display(MSG_badsetdir, tmpdir);
		process_menu(MENU_cdrombadmount);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}

	/* return location, don't clean... */
	strncpy(ext_dir, tmpdir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}


/*
 * Get from a pathname inside an unmounted local filesystem
 * (e.g., where sets were preloaded onto a local DOS partition) 
 */
int
get_via_localfs()
{
	char tmpdir[STRSIZE];

	/* Get device, filesystem, and filepath */
	process_menu (MENU_localfssource);

again:
	run_prog(0, NULL, "/sbin/umount /mnt2");

	/* Mount it */
	if (run_prog(0, NULL, "/sbin/mount -rt %s /dev/%s /mnt2",
	    localfs_fs, localfs_dev)) {

		msg_display(MSG_localfsbadmount, localfs_dir, localfs_dev); 
		process_menu(MENU_localfsbadmount);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	snprintf(tmpdir, STRSIZE, "%s/%s", "/mnt2", localfs_dir);

	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(tmpdir) == 0) {
		msg_display(MSG_badsetdir, tmpdir);
		process_menu(MENU_localfsbadmount);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	/* return location, don't clean... */
	strncpy(ext_dir, tmpdir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}

/*
 * Get from an already-mounted pathname.
 */

int get_via_localdir(void)
{

	/* Get device, filesystem, and filepath */
	process_menu(MENU_localdirsource);

again:
	/* Complain if not a directory */
	if (dir_exists_p(localfs_dir) == 0) {

		msg_display(MSG_badlocalsetdir, localfs_dir);
		process_menu(MENU_localdirbad);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}
	
	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(localfs_dir) == 0) {
		msg_display(MSG_badsetdir, localfs_dir);
		process_menu(MENU_localdirbad);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}

	/* return location, don't clean... */
	strncpy(ext_dir, localfs_dir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 0;
	return 1;
}


void
cd_dist_dir(forwhat)
	char *forwhat;
{
	char *cwd;

	/* ask user for the mountpoint. */
	msg_prompt(MSG_distdir, dist_dir, dist_dir, STRSIZE, forwhat);

	/* make sure the directory exists. */
	make_target_dir(dist_dir);

	clean_dist_dir = 1;
	target_chdir_or_die(dist_dir);

	/* Set ext_dir for absolute path. */
	cwd = getcwd(NULL,0);
	strncpy(ext_dir, cwd, STRSIZE);
	free (cwd);
}


/*
 * Support for custom distribution fetches / unpacks.
 */
void
toggle_getit(num)
	int num;
{

	dist_list[num].getit ^= 1;
}

void
show_cur_distsets()
{
	distinfo *list;

	msg_display(MSG_cur_distsets);
	msg_table_add(MSG_cur_distsets_header);
	list = dist_list;
	while (list->name) {
		msg_table_add(MSG_cur_distsets_row, list->desc,
		    list->getit ? msg_string(MSG_yes) : msg_string(MSG_no));
		list++;
	}
}

/* Do we want a verbose extract? */
static	int verbose = -1;

void
ask_verbose_dist()
{

	if (verbose < 0) {
		msg_display(MSG_verboseextract);
		process_menu(MENU_noyes);
		verbose = yesno;
		wclear(stdscr);
		wrefresh(stdscr);
	}
}

int
extract_file(path)
	char *path;
{
	char *owd;
	int   tarexit, rv;
	
	owd = getcwd (NULL,0);

	/* check tarfile exists */
	if (!file_exists_p(path)) {
		tarstats.nnotfound++;

		msg_display(MSG_notarfile, path);
		process_menu(MENU_noyes);
		return (yesno == 0);
	}

	tarstats.nfound++;	
	/* cd to the target root. */
	target_chdir_or_die("/");	

	/* now extract set files files into "./". */
	tarexit = run_prog(RUN_DISPLAY, NULL,
	    "pax -zr%spe -f %s", verbose ? "v" : "", path);

	/* Check tarexit for errors and give warning. */
	if (tarexit) {
		tarstats.nerror++;

		msg_display(MSG_tarerror, path);
		process_menu(MENU_noyes);
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
extract_dist()
{
	char distname[STRSIZE];
	char fname[STRSIZE];
	distinfo *list;
	int punt;

	/* reset failure/success counters */
	memset(&tarstats, 0, sizeof(tarstats));

	/*endwin();*/
	for (punt = 0, list = dist_list; list->name != NULL; list++) {
		if (list->getit) {
			tarstats.nselected++;
			if (punt) {
				tarstats.nskipped++;
				continue;
			}
			if (cleanup_dist(list->name) == 0) {
				msg_display(MSG_cleanup_warn);
				process_menu(MENU_ok);
			}
			(void)snprintf(distname, STRSIZE, "%s%s", list->name,
			    dist_postfix);
			(void)snprintf(fname, STRSIZE, "%s/%s", ext_dir,
			    distname);

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
		process_menu(MENU_ok);
		return 0;
	} else {
		/* We encountered  errors. Let the user know. */
		msg_display(MSG_endtar,
		    tarstats.nselected, tarstats.nnotfound, tarstats.nskipped,
		    tarstats.nfound, tarstats.nsuccess, tarstats.nerror);
		process_menu(MENU_ok);
		return 1;
	}
}

/*
 * Do pre-extract cleanup for set 'name':
 * open a file named '/dist/<name>_obsolete file', which contain a list of
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
cleanup_dist(name)
	const char *name;
{
	char file_path[MAXPATHLEN];
	char file_name[MAXPATHLEN];
	FILE *list_file;
	struct filelist *head = NULL;
	struct filelist *current;
	int saved_errno;
	struct stat st;
	int retval = 1;
	int needok = 0;

	snprintf(file_path, MAXPATHLEN, "/dist/%s_obsolete", name);
	list_file = fopen(file_path, "r");
	if (list_file == NULL) {
		saved_errno = errno;
		if (logging)
			fprintf(log, "Open of %s failed: %s\n", file_path,
			    strerror(saved_errno));
		if (saved_errno == ENOENT)
			return 1;
		msg_display_add(MSG_openfail, name, strerror(saved_errno));
		process_menu(MENU_ok);
		return 0;
	}
	while (fgets(file_name, MAXPATHLEN, list_file)) {
		/* ignore lines that don't begin with '/' */
		if (file_name[0] != '/')
			continue;
		/* Remove trailing \n if any */
		if (file_name[strlen(file_name)-1] == '\n')
			file_name[strlen(file_name)-1] = '\0';
		snprintf(file_path, MAXPATHLEN, "%s%s", target_prefix(),
		    file_name);
		if (lstat(file_path, &st) != 0) {
			saved_errno = errno;
			if (logging)
				fprintf(log, "stat() of %s failed: %s\n",
				    file_path, strerror(saved_errno));
			if (saved_errno == ENOENT)
				continue;
			msg_display_add(MSG_statfail, file_path,
			    strerror(saved_errno));
			process_menu(MENU_ok);
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
			if (head == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(1);
			}
			current = current->next;
		}
		current->next = NULL;
		snprintf(current->name, MAXPATHLEN, "%s", file_path);
		current->type = st.st_mode & S_IFMT;
		if (logging)
			fprintf(log, "Adding file %s, type %d to list of "
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
	process_menu(MENU_ok);
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
				fprintf(log, "rm %s failed: %s\n",
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
				fprintf(log, "dir %s not empty, "
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
					fprintf(log, "mv %s %s failed: %s\n", 
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
				fprintf(log, "rm %s failed: %s\n",
				    current->name, strerror(saved_errno));
			msg_display_add(MSG_unlink_fail, current->name,
			    strerror(saved_errno));
			retval = 0;
			needok = 1;
		}
	}
	if (needok)
		process_menu(MENU_ok);
	return retval;
}

/*
 * Get and unpack the distribution.
 * show success_msg if installation completes. Otherwise,,
 * show failure_msg and wait for the user to ack it before continuing.
 * success_msg and failure_msg must both be 0-adic messages.
 */
int
get_and_unpack_sets(success_msg, failure_msg)
	msg success_msg;
	msg failure_msg;
{

	/* Ensure mountpoint for distribution files exists in current root. */
	(void) mkdir("/mnt2", S_IRWXU| S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH);
	if (scripting)
		(void)fprintf(script, "mkdir /mnt2\nchmod 755 /mnt2\n");

	/* Find out which files to "get" if we get files. */
	wclear(stdscr);
	wrefresh(stdscr);
	process_menu(MENU_distset);

	/* ask user whether to do normal or verbose extraction */
	ask_verbose_dist();

	/* Get the distribution files */
	do {
		got_dist = 0;
		process_menu(MENU_distmedium);
	} while (got_dist == -1);

	if (nodist)
		return 1;

	if (got_dist) {

		/* Extract the distribution, abort on errors. */
		if (extract_dist())
			return 1;

		/* Configure the system */
		run_makedev();

		/* Other configuration. */
		mnt_net_config();
		
		/* Clean up dist dir (use absolute path name) */
		if (clean_dist_dir)
			run_prog(0, NULL, "/bin/rm -rf %s", ext_dir);

		/* Mounted dist dir? */
		if (mnt2_mounted)
			run_prog(0, NULL, "/sbin/umount /mnt2");

		/* Install/Upgrade complete ... reboot or exit to script */
		msg_display(success_msg);
		process_menu(MENU_ok);
		return 0;
	}

	msg_display(failure_msg);
	process_menu(MENU_ok);
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
check_for(mode, pathname)
	unsigned int mode;
	const char *pathname;
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
sanity_check()
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
	process_menu(MENU_ok);
	return 1;
}

/* set reverse to 1 to default to no */
int askyesno(int reverse)
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
	while ((c = getchar())) {
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

/*
 * Some globals to pass things back from callbacks
 */
static char zoneinfo_dir[STRSIZE];
static char *tz_selected;	/* timezonename (relative to share/zoneinfo */
static char *tz_default;	/* UTC, or whatever /etc/localtime points to */
static char tz_env[STRSIZE];

/*
 * Callback from timezone menu
 */
static int
set_timezone_select(menudesc *m)
{
	time_t t;

	if (m)
		tz_selected = m->opts[m->cursel].opt_name;
	snprintf(tz_env, sizeof(tz_env), "%s/%s",
		 zoneinfo_dir, tz_selected);
	setenv("TZ", tz_env, 1);
	t = time(NULL);
	msg_display(MSG_choose_timezone, 
		    tz_default, tz_selected, ctime(&t), localtime(&t)->tm_zone);
	return 0;
}

/*
 * Alarm-handler to update example-display
 */
static void
timezone_sig(int sig)
{
	set_timezone_select(NULL);
	alarm(1);
}

/*
 * Choose from the files in usr/share/zoneinfo and set etc/localtime
 */
int
set_timezone()
{
	char localtime_link[STRSIZE];
	char localtime_target[STRSIZE];
	int rc;
	time_t t;
	sig_t oldalrm;
	FTS *tree;
	FTSENT *entry;
	int rval;
	char *argv[2];
	int skip;
	struct stat sb;
	int nfiles, n;
	int menu_no;
	menu_ent *tz_menu;

	oldalrm = signal(SIGALRM, timezone_sig);
	alarm(1);
       
	strncpy(zoneinfo_dir, target_expand("/usr/share/zoneinfo"), STRSIZE);
	strncpy(localtime_link, target_expand("/etc/localtime"), STRSIZE);

	/* Add sanity check that /mnt/usr/share/zoneinfo contains
	 * something useful */

	rc = readlink(localtime_link, localtime_target,
		      sizeof(localtime_target));
	if (rc < 0) {
		/* error, default to UTC */
		tz_default = "UTC";
	} else {
		localtime_target[rc] = '\0';
		tz_default = strchr(strstr(localtime_target, "zoneinfo"), '/')+1;
	}

	tz_selected=tz_default;
	snprintf(tz_env, sizeof(tz_env), "%s/%s",
		 zoneinfo_dir, tz_selected);
	setenv("TZ", tz_env, 1);
	t = time(NULL);
	msg_display(MSG_choose_timezone, 
		    tz_default, tz_selected, ctime(&t), localtime(&t)->tm_zone);

	skip = strlen(zoneinfo_dir);
	argv[0] = zoneinfo_dir;
	argv[1] = NULL;
	if (!(tree = fts_open(argv, FTS_LOGICAL, NULL))) {
		return 1;	/* error - skip timezone setting */
	}
	for (nfiles = 0; (entry = fts_read(tree)) != NULL;) {
		stat(entry->fts_accpath, &sb);
		if (S_ISREG(sb.st_mode))
			nfiles++;
	}
	if (errno) {
		return 1;	/* error - skip timezone setting */
	}
	(void)fts_close(tree);
	
	tz_menu = malloc(nfiles * sizeof(struct menu_ent));
	if (tz_menu == NULL) {
		return 1;	/* error - skip timezone setting */
	}
	
	if (!(tree = fts_open(argv, FTS_LOGICAL, NULL))) {
		return 1;	/* error - skip timezone setting */
	}
	n = 0;
	for (rval = 0; (entry = fts_read(tree)) != NULL; ) {
		stat(entry->fts_accpath, &sb);
		if (S_ISREG(sb.st_mode)) {
			tz_menu[n].opt_name = strdup(entry->fts_accpath+skip+1);
			tz_menu[n].opt_menu = OPT_NOMENU;
			tz_menu[n].opt_flags = 0;
			tz_menu[n].opt_action = set_timezone_select;

			n++;
		}
	}
	if (errno) {
		return 1;	/* error - skip timezone setting */
	}
	(void)fts_close(tree);  
	
	menu_no = new_menu(NULL, tz_menu, nfiles, 23, 9,
			   12, 32, MC_SCROLL|MC_NOSHORTCUT, NULL, NULL,
			   "\nPlease consult the install documents.");
	if (menu_no < 0) {
		return 1;	/* error - skip timezone setting */
	}
	process_menu(menu_no);

	free_menu(menu_no);
	for(n=0; n < nfiles; n++)
		free(tz_menu[n].opt_name);
	free(tz_menu);

	signal(SIGALRM, SIG_IGN);

	snprintf(localtime_target, sizeof(localtime_target),
		 "/usr/share/zoneinfo/%s", tz_selected);
	unlink(localtime_link);
	symlink(localtime_target, localtime_link);
	
	return 1;
}

int
set_crypt_type(void)
{
	FILE *pwc;
	char *fn;

	msg_display(MSG_choose_crypt);
	process_menu(MENU_crypttype);
	fn = strdup(target_expand("/etc/passwd.conf"));
	rename(fn, target_expand("/etc/passwd.conf.pre-sysinst"));

	switch (yesno) {
	case 0:	/* MD5 */
		pwc = fopen(fn, "w");
		fprintf(pwc,
		    "default:\n"
		    "  localcipher = md5\n"
		    "  ypcipher = md5\n");
		fclose(pwc);
		break;
	case 1:	/* DES */
		break;
	case 2:	/* blowfish 2^7 */
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
set_root_password()
{
	msg_display(MSG_rootpw);
	process_menu(MENU_yesno);
	if (yesno)
		run_prog(RUN_DISPLAY|RUN_CHROOT, NULL, "passwd -l root");
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

/*
 * check that there is at least a / somewhere.
 */
int
check_partitions()
{
	int i;

	for (i = 0; i < getmaxpartitions(); i++)
		if (PI_ISBSDFS(&bsdlabel[i]) &&
		    fsmount[i][0] == '/' && fsmount[i][1] == '\0')
			return 1;
	msg_display(MSG_no_root_fs);
	getchar();
	return 0;
}

void
set_sizemultname_cyl()
{

	sizemult = dlcylsize; 
	multname = msg_string(MSG_cylname);
}

void
set_sizemultname_meg()
{

	sizemult = MEG / sectorsize;
	multname = msg_string(MSG_megname);
}

int
check_lfs_progs()
{

	return (access("/sbin/dump_lfs", X_OK) == 0 &&
		access("/sbin/fsck_lfs", X_OK) == 0 &&
		access("/sbin/mount_lfs", X_OK) == 0 &&
		access("/sbin/newfs_lfs", X_OK) == 0);
}
