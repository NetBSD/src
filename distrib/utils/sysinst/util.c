/*	$NetBSD: util.c,v 1.17 1997/11/05 23:32:54 phil Exp $	*/

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
static int check_for __P((const char *type, const char *pathname));


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
	msg_display (MSG_makedev);
	sleep (1);

	/* make /dev, in case the user  didn't extract it. */
	make_target_dir("/dev");
	target_chdir_or_die("/dev");
	run_prog ("/bin/sh MAKEDEV all");
}


/* Load files from floppy. */
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
		snprintf (distname, STRSIZE, list->name, rels, dist_postfix);
		while (list->getit && strcmp(&post[1],list->fdlast) <= 0) {
			snprintf (fname, STRSIZE, list->name, rels, post);
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
	/* Get server and filepath */
	process_menu (MENU_cdromsource);

	/* Fill in final default path. */
	strncat (ftp_dir, rels, STRSIZE-strlen(ftp_dir));
	strcat  (ftp_dir, "/");
	strncat (ftp_dir, machine, STRSIZE-strlen(ftp_dir));

	/* Mount it */
	while (run_prog ("/sbin/mount -rt cd9660 /dev/%sa /mnt2", cdrom_dev)) {
		process_menu (MENU_cdrombadmount);
		if (!yesno)
			return 0;
		/* Verify distribution files exist.  XXX */
	}

	/* return location, don't clean... */
	strcpy (dist_dir, "/mnt2");
	strncat (dist_dir, cdrom_dir, STRSIZE-strlen(dist_dir)-1);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}

int
get_via_localfs(void)
{
	/* Get device, filesystem, and filepath */
	process_menu (MENU_localfssource);

	/* Mount it */
	while (run_prog ("/sbin/mount -rt %s /dev/%s /mnt2", localfs_fs,
	    localfs_dev)) {
		process_menu (MENU_localfsbadmount);
		if (!yesno)
			return 0;
		/* Verify distribution files exist.  XXX */
	}

	/* return location, don't clean... */
	strcpy (dist_dir, "/mnt2");
	strncat (dist_dir, localfs_dir, STRSIZE-strlen(dist_dir)-1);
	clean_dist_dir = 0;
	mnt2_mounted = 1;
	return 1;
}

void cd_dist_dir (char *forwhat)
{
	/* ask user for the mountpoint. */
	msg_prompt (MSG_distdir, dist_dir, dist_dir, STRSIZE, forwhat);

	/* make sure the directory exists. */
	make_target_dir(dist_dir);

	clean_dist_dir = 1;
	target_chdir_or_die(dist_dir);
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

	/* cd to the target root. */
	target_chdir_or_die("/");	

	/* now extract set files files into "./". */
	(void)printf (msg_string(MSG_extracting), path);
	tarexit = run_prog ("/usr/bin/tar --unlink -xpz%s -f %s",
			    verbose ? "v":"", path);
	/* Check tarexit for errors and give warning. */
	if (tarexit)
		ask_ynquestion (msg_string(MSG_tarerror), 0, path);

	chdir (owd);
	free (owd);
}

void
extract_dist (void)
{
	char distname[STRSIZE];
	char fname[STRSIZE];
	distinfo *list;
	char extdir[STRSIZE];

	/* For NFS,  distdir is mounted in the _current_ root.  */
	strncpy(extdir, dist_dir, STRSIZE);

	endwin();
	list = dist_list;
	while (list->name) {
		if (list->getit) {
			(void)snprintf (distname, STRSIZE, list->name, rels,
					dist_postfix);
			(void)snprintf (fname, STRSIZE, "%s/%s", extdir,
					distname);
			extract_file (fname);
		}
		list++;
	}
	puts(CL);
	wrefresh(stdscr);
}


/*
 * Get  and unpack the distribution.
 * show success_msg if installation  completes. Otherwise,,
 * sHow failure_msg and wait for the user to ack it before continuing.
 * success_msg and failure_msg must both be 0-adic messages.
 */
void get_and_unpack_sets(int success_msg, int failure_msg)
{
	/* Get the distribution files */
	process_menu (MENU_distmedium);
	if (nodist)
		return;

	ask_verbose_dist ();

	if (got_dist) {
		/* Extract the distribution */
		extract_dist ();

		/* Configure the system */
		run_makedev ();

		/* Other configuration. */
		mnt_net_config();
		
		/* Clean up ... */
		if (clean_dist_dir)
			run_prog ("/bin/rm -rf %s", dist_dir);

		/* Mounted dist dir? */
		if (mnt2_mounted)
			run_prog ("/sbin/umount /mnt2");

		/* Install/Upgrade  complete ... reboot or exit to script */
		msg_display (success_msg);
		process_menu (MENU_ok);
	} else {
		msg_display (failure_msg);
		process_menu (MENU_ok);
	}
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
  { "-d" "/dev" },
  { "-c", "/dev/console" },
/* XXX check for rootdev in target /dev? */
  { "-f", "/etc/fstab" },
  { "-f", "/sbin/fsck" },
  { "-f", "/sbin/fsck_ffs" },
  { "-f", "/sbin/mount" },
  { "-f", "/sbin/mount_ffs" },
  { "-f", "/sbin/mount_nfs" },
#if defined(DEBUG) || 1
  { "-f", "/foo/bar" },		/* XXX */
#endif
  { 0, 0 }
  
};


/*
 * Check target for a single file.
 */
static int check_for(const char *type, const char *pathname)
{
	int result; 

	result = (target_test(type, pathname) == 0);
	if (result != 0) 
		msg_display(MSG_rootmissing, pathname);
	return result;
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
