/*	$NetBSD: util.c,v 1.23 1997/12/25 17:54:41 fvdl Exp $	*/

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

/* util.c -- routines that don't really fit anywhere else... */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <curses.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local prototypes 
 */
struct  tarstats{
	int nselected;
	int nfound;
	int nnotfound;
	int nerror;
	int nsuccess;
} tarstats;

void	extract_file __P((char *path));
int	extract_dist __P((void));
int	distribution_sets_exist_p __P((const char *path));
static int check_for __P((const char *type, const char *pathname));


int dir_exists_p(const char *path)
{
	register int result;
	result = (run_prog("test -d %s", path) == 0);
	return (result);
}

int file_exists_p(const char *path)
{
	register int result;
	result = (run_prog("test -f %s", path) == 0);
	return (result);
}

int distribution_sets_exist_p (const char *path)
{
	char buf[STRSIZE];
	int result;

	result = 1;
	snprintf(buf, STRSIZE, "%s/%s", path, "kern.tgz");
	result = result && file_exists_p(buf);

	snprintf(buf, STRSIZE, "%s/%s", path, "etc.tgz");
	result = result && file_exists_p(buf);

	return(result);
}


void get_ramsize(void)
{
	long len=sizeof(long);
	int mib[2] = {CTL_HW, HW_PHYSMEM};
	
	sysctl(mib, 2, (void *)&ramsize, (size_t *)&len, NULL, 0);

	/* Find out how many Megs ... round up. */
	rammb = (ramsize + MEG - 1) / MEG;
}


static int asked = 0;

void ask_sizemult (void)
{
	if (!asked) {
		msg_display (MSG_sizechoice, dlcylsize);
		process_menu (MENU_sizechoice);
	}
	asked = 1;
}

void reask_sizemult (void)
{
	asked = 0;
	ask_sizemult ();
}

/* Returns 1 for "y" or "Y" and "n" otherwise.  CR => default. */
int
ask_ynquestion (char *quest, char def, ...)
{
	char line[STRSIZE];
	va_list ap;
	char c;

	va_start(ap, def);
	vsnprintf (line, STRSIZE, quest, ap);
	va_end(ap);

	if (def)
		printf ("%s [%c]: ", line, def);
	else
		printf ("%s: ", line);
	c = getchar();
	if (c == '\n')
		return def == 'y';

	while (getchar() != '\n') /* eat characters */;

	return c == 'y' || c == 'Y';
}

void run_makedev (void)
{
	char *owd;

	msg_display (MSG_makedev);
	sleep (1);

	owd = getcwd (NULL,0);

	/* make /dev, in case the user  didn't extract it. */
	make_target_dir("/dev");
	target_chdir_or_die("/dev");
	run_prog ("/bin/sh MAKEDEV all");

	chdir(owd);
	free(owd);
}


/* Load files from floppy.  Requires a /mnt2 directory for mounting them. */
int get_via_floppy (void)
{
	char distname[STRSIZE];
	char fddev[STRSIZE] = "/dev/fd0a";
	char fname[STRSIZE];
	char fullname[STRSIZE];
	distinfo *list;
	char post[4];
	int  mounted = 0;
	int  first;
	struct stat sb;


	cd_dist_dir ("unloading from floppy");

	msg_prompt_add (MSG_fddev, fddev, fddev, STRSIZE);

	list = dist_list;
	while (list->name) {
		strcpy (post, ".aa");
		snprintf (distname, STRSIZE, "%s%s", list->name, dist_postfix);
		while (list->getit && strcmp(&post[1],list->fdlast) <= 0) {
			snprintf (fname, STRSIZE, "%s%s", list->name, post);
			snprintf (fullname, STRSIZE, "/mnt2/%s", fname);
			first = 1;
			while (!mounted || stat(fullname, &sb)) {
 				if (mounted) 
					run_prog ("/sbin/umount /mnt2 "
						  "2>/dev/null");
				if (first)
					msg_display (MSG_fdmount, fname);
				else
					msg_display (MSG_fdnotfound, fname);
				process_menu (MENU_fdok);
				if (!yesno)
					return 0;
				while (run_prog("/sbin/mount -t %s %s /mnt2",
						 fdtype, fddev)) {
					msg_display (MSG_fdremount, fname);
					process_menu (MENU_fdremount);
					if (!yesno)
						return 0;
				}
				mounted = 1;
				first = 0;
			}
			run_prog ("/bin/cat %s >> %s", fullname, distname);
			if (post[2] < 'z')
				post[2]++;
			else
				post[2]='a', post[1]++;
		}
		run_prog ("/sbin/umount /mnt2 2>/dev/null");
		mounted = 0;
		list++;
	}
#ifndef DEBUG
	chdir("/");	/* back to current real root */
#endif
	return 1;
}

/* Get from a CDROM distribution. */

int
get_via_cdrom(void)
{
	char tmpdir[STRSIZE];

	/* Get CD-rom device name and path within CD-rom */
	process_menu (MENU_cdromsource);

	/* Fill in final default path. */
	strncat (cdrom_dir, rel, STRSIZE-strlen(cdrom_dir));
	strcat  (cdrom_dir, "/");
	strncat (cdrom_dir, machine, STRSIZE-strlen(cdrom_dir));

again:
	run_prog("/sbin/umount /mnt2  2> /dev/null");

	/* Mount it */
	if (run_prog ("/sbin/mount -rt cd9660 /dev/%sa /mnt2", cdrom_dev)) {
		msg_display(MSG_badsetdir, cdrom_dev);
		process_menu (MENU_cdrombadmount);
		if (!yesno)
			return 0;
		if (!ignorerror)
			goto again;
	}

	snprintf(tmpdir, STRSIZE, "%s/%s", "/mnt2", cdrom_dir);

	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(tmpdir) == 0) {
		msg_display(MSG_badsetdir, tmpdir);
		process_menu (MENU_cdrombadmount);
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
int get_via_localfs(void)
{
	char tmpdir[STRSIZE];

	/* Get device, filesystem, and filepath */
	process_menu (MENU_localfssource);

again:

	run_prog("/sbin/umount /mnt2  2> /dev/null");

	/* Mount it */
	if (run_prog ("/sbin/mount -rt %s /dev/%s /mnt2", localfs_fs,
	    localfs_dev)) {

		msg_display (MSG_localfsbadmount, localfs_dir, localfs_dev); 
		process_menu (MENU_localfsbadmount);
		if (!yesno)
			return 0;
		if (!ignorerror)
		goto again;
	}

	snprintf(tmpdir, STRSIZE, "%s/%s", "/mnt2", localfs_dir);

	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(tmpdir) == 0) {
		msg_display(MSG_badsetdir, tmpdir);
		process_menu (MENU_localfsbadmount);
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



/* Get from an already-mounted  pathname. */

int get_via_localdir(void)
{


	/* Get device, filesystem, and filepath */
	process_menu (MENU_localdirsource);

again:
	/* Complain if not a directory */
	if (dir_exists_p(localfs_dir) == 0) {

		msg_display (MSG_badlocalsetdir, localfs_dir);
		process_menu (MENU_localdirbad);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}
	
	/* Verify distribution files exist.  */
	if (distribution_sets_exist_p(localfs_dir) == 0) {
		msg_display(MSG_badsetdir, localfs_dir);
		process_menu (MENU_localdirbad);
		if (!yesno)
			return (0);
		if (!ignorerror)
			goto again;
	}


	/* return location, don't clean... */
	strncpy (ext_dir, localfs_dir, STRSIZE);
	clean_dist_dir = 0;
	mnt2_mounted = 0;
	return 1;
}


void cd_dist_dir (char *forwhat)
{
	char *cwd;

	/* ask user for the mountpoint. */
	msg_prompt (MSG_distdir, dist_dir, dist_dir, STRSIZE, forwhat);

	/* make sure the directory exists. */
	make_target_dir(dist_dir);

	clean_dist_dir = 1;
	target_chdir_or_die(dist_dir);

	/* Set ext_dir for absolute path. */
	cwd = getcwd (NULL,0);
	strncpy (ext_dir, cwd, STRSIZE);
	free (cwd);
}


/* Support for custom distribution fetches / unpacks. */

void toggle_getit (int num)
{
	dist_list[num].getit ^= 1;
}

void show_cur_distsets (void)
{
	distinfo *list;

	msg_display (MSG_cur_distsets);
	list = dist_list;
	while (list->name) {
		msg_printf_add ("%s%s\n", list->desc, list->getit ?
				msg_string(MSG_yes) : msg_string(MSG_no));
		list++;
	}
}


/* Do we want a verbose extract? */
static	int verbose = -1;

void
ask_verbose_dist (void)
{
	if (verbose < 0) {
		msg_display (MSG_verboseextract);
		process_menu (MENU_noyes);
		verbose = yesno;
	}
}

void
extract_file (char *path)
{
	char *owd;
	int   tarexit;
	
	owd = getcwd (NULL,0);

	/* check tarfile exists */
	if (!file_exists_p(path)) {
		tarstats.nnotfound++;
		ask_ynquestion(msg_string(MSG_notarfile), 0, path);
		return;
	}

	tarstats.nfound++;	
	/* cd to the target root. */
	target_chdir_or_die("/");	

	/* now extract set files files into "./". */
	(void)printf (msg_string(MSG_extracting), path);
	tarexit = run_prog ("/usr/bin/tar --unlink -xpz%s -f %s",
			    verbose ? "v":"", path);

	/* Check tarexit for errors and give warning. */
	if (tarexit) {
		tarstats.nerror++;
		ask_ynquestion (msg_string(MSG_tarerror), 0, path);
		sleep(3);
	} else {
		tarstats.nsuccess++;
		sleep(1);
	}
	
	chdir (owd);
	free (owd);
}


/* Extract_dist **REQUIRES** an absolute path in ext_dir.  Any code
 * that sets up dist_dir for use by extract_dist needs to put in the
 * full path name to the directory. 
 */

int
extract_dist (void)
{
	char distname[STRSIZE];
	char fname[STRSIZE];
	distinfo *list;

	/* reset failure/success counters */
	bzero(&tarstats, sizeof(tarstats));

	endwin();
	list = dist_list;
	while (list->name) {
		if (list->getit) {
			tarstats.nselected++;
			(void)snprintf (distname, STRSIZE, "%s%s", list->name,
					dist_postfix);
			(void)snprintf (fname, STRSIZE, "%s/%s", ext_dir,
					distname);
			extract_file (fname);
		}
		list++;
	}

	puts(CL);
	wrefresh(stdscr);

	if (tarstats.nerror == 0 && tarstats.nsuccess == tarstats.nselected) {
		msg_display (MSG_endtarok);
		process_menu (MENU_ok);
		return 0;
	} else {
		/* We encountered  errors. Let the user know. */
		msg_display(MSG_endtar,
		    tarstats.nselected, tarstats.nnotfound,
		    tarstats.nfound, tarstats.nsuccess, tarstats.nerror);
		process_menu (MENU_ok);
		return 1;
	}
}


/*
 * Get  and unpack the distribution.
 * show success_msg if installation  completes. Otherwise,,
 * sHow failure_msg and wait for the user to ack it before continuing.
 * success_msg and failure_msg must both be 0-adic messages.
 */
void get_and_unpack_sets(int success_msg, int failure_msg)
{
	/* Ensure mountpoint for distribution files exists in current root. */
	(void) mkdir("/mnt2", S_IRWXU| S_IRGRP|S_IXGRP | S_IXOTH|S_IXOTH);

	/* Get the distribution files */
	process_menu (MENU_distmedium);
	if (nodist)
		return;


	if (got_dist) {

		/* ask user  whether to do normal or verbose extraction */
		ask_verbose_dist ();

		/* Extract the distribution, abort on errors. */
		if (extract_dist ()) {
			goto bad;
		}

		/* Configure the system */
		run_makedev ();

		/* Other configuration. */
		mnt_net_config();
		
		/* Clean up dist dir (use absolute path name) */
		if (clean_dist_dir)
			run_prog ("/bin/rm -rf %s", ext_dir);

		/* Mounted dist dir? */
		if (mnt2_mounted)
			run_prog ("/sbin/umount /mnt2");

		/* Install/Upgrade  complete ... reboot or exit to script */
		msg_display (success_msg);
		process_menu (MENU_ok);
		return;
	}

bad:
		msg_display (failure_msg);
		process_menu (MENU_ok);

}



/*
 * Do a quick sanity check that  the target can reboot.
 * return 1 if everything OK, 0 if there is a problem.
 * Uses a table of files we expect to find after a base install/upgrade.
 */



/* test flag and pathname to check for after unpacking. */
struct check_table { const char *testarg; const char *path;} checks[] = {
  { "-f", "/netbsd" },
  { "-d ""/etc" },
  { "-f", "/etc/fstab" },
  { "-f", "/sbin/init" },
  { "-f", "/bin/sh" },
  { "-f", "/etc/rc" },
  { "-f", "/etc/rc.subr" },
  { "-f", "/etc/rc.conf" },
  { "-d" "/dev" },
  { "-c", "/dev/console" },
/* XXX check for rootdev in target /dev? */
  { "-f", "/etc/fstab" },
  { "-f", "/sbin/fsck" },
  { "-f", "/sbin/fsck_ffs" },
  { "-f", "/sbin/mount" },
  { "-f", "/sbin/mount_ffs" },
  { "-f", "/sbin/mount_nfs" },
#if defined(DEBUG) || defined(DEBUG_CHECK)
  { "-f", "/foo/bar" },		/* bad entry to exercise warning */
#endif
  { 0, 0 }
  
};


/*
 * Check target for a single file.
 */
static int check_for(const char *type, const char *pathname)
{
	int found; 

	found = (target_test(type, pathname) == 0);
	if (found == 0) 
		msg_display(MSG_rootmissing, pathname);
	return found;
}

int
sanity_check()
{

	int target_ok = 1;
	struct check_table *p;

	for (p = checks; p->path; p++) {
		target_ok = target_ok && check_for(p->testarg, p->path);
	}
	if (target_ok)
		return 0;	    

	/* Uh, oh. Something's missing. */
	msg_display(MSG_badroot);
	process_menu(MENU_ok);
	return 1;
}
