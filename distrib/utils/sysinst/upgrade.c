/*	$NetBSD: upgrade.c,v 1.45 2004/04/18 21:34:56 dsl Exp $	*/

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

/* upgrade.c -- upgrade an installation. */

#include <stdio.h>
#include <curses.h>
#include <errno.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local prototypes
 */
static int save_etc(void);
static int merge_etc(void);
static int save_X(void);
static int merge_X(void);

static int etc_saved;

/*
 * Do the system upgrade.
 */
void
do_upgrade(void)
{

	msg_display(MSG_upgradeusure);
	process_menu(MENU_noyes, NULL);
	if (!yesno)
		return;

	get_ramsize();

	if (find_disks(msg_string(MSG_upgrade)) < 0)
		return;

	if (md_pre_update() < 0)
		return;

	process_menu(MENU_distset, NULL);

	if (mount_disks() != 0)
		return;


	/*
	 * Save X symlink, ...
	 */
	if (save_X())
		return;

#if 0
	/*
	 * Move target /etc -> target /etc.old so existing configuration
	 * isn't overwritten by upgrade.
	 */
	if (sets_selected & SET_ETC && save_etc()) {
		merge_X();
		return;
	}
#endif

	/* Do any md updating of the file systems ... e.g. bootblocks,
	   copy file systems ... */
	if (!md_update())
		return;

	/* Done with disks. Ready to get and unpack tarballs. */
	msg_display(MSG_disksetupdoneupdate);
	getchar();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	if (get_and_unpack_sets(1, MSG_upgrcomplete, MSG_abortupgr) != 0)
		return;

#if 0
	/* Copy back any files we should restore after the upgrade.*/
	if (sets_installed & SET_ETC)
		merge_etc();
#endif
	merge_X();

	sanity_check();
}

#if 0
/*
 * save target /etc files.
 * if target /etc.old exists, print a warning message and give up.
 * otherwise move /etc into target /etc.old, and then copy
 * back files we might want during the installation --  in case 
 * we are upgrading the target root.
 */
static int
save_etc(void)
{

	if (target_dir_exists_p("/etc.old")) {
		msg_display(MSG_etc_oldexists);
		process_menu(MENU_ok, NULL);
		return EEXIST;
	}

#ifdef DEBUG
	printf("saving /etc as /etc.old...");
#endif

	/* Move target /etc to /etc.old.  Abort on error. */
	mv_within_target_or_die("/etc", "/etc.old");

	etc_saved = 1;

	/* now make an /etc that should let the user reboot. */
	make_target_dir("/etc");

#ifdef DEBUG
	printf("Copying essential files back into new /etc...");
#endif
	/* essential stuff */
	cp_within_target("/etc.old/ld.so.cache", "/etc/", 1);
	cp_within_target("/etc.old/ld.so.conf", "/etc/", 0);
	cp_within_target("/etc.old/resolv.conf", "/etc/", 1);

	/* 
	 * do NOT create fstab so that restarting an incomplete
	 * upgrade (eg., after power failure) will fail, and
	 * force the user to check and restore their old /etc.
	 */

	/* general config */
	cp_within_target("/etc.old/rc", "/etc/", 0);
	cp_within_target("/etc.old/rc.conf", "/etc/", 0);
	cp_within_target("/etc.old/rc.local", "/etc/", 0);

	/* network config */
	cp_within_target("/etc.old/ifconfig.*", "/etc/", 0);
	/* these should become parts of rc.conf */
	cp_within_target("/etc.old/myname", "/etc/", 1);
	cp_within_target("/etc.old/mygate", "/etc/", 1);
	cp_within_target("/etc.old/defaultdomain", "/etc/", 1);

	/* old-style network config */
	cp_within_target("/etc.old/hostname.*", "/etc/", 1);

	return 0;
}

/*
 * Attempt to undo save_etc() if the install fails
 */

void
restore_etc(void)
{
	const char *tp;

	if (!etc_saved)
		return;
	if (sets_installed & SET_ETC)
		return;

	tp = target_prefix();
	run_program(0, "mv -f %s/etc.old/* %s/etc", tp, tp);
	/* rmdir isn't in our root, chroot and use the one in the target */
	run_program(RUN_CHROOT, "rmdir /etc.old");
	etc_saved = 0;
}
#endif

/*
 * Save X symlink to X.old so it can be recovered later
 */
static int
save_X(void)
{
	/* Only care for X if it's a symlink */
	if (target_symlink_exists_p("/usr/X11R6/bin/X")) {
		if (target_symlink_exists_p("/usr/X11R6/bin/X.old")) {
			msg_display(MSG_X_oldexists);
			process_menu(MENU_ok, NULL);
			return EEXIST;
		}

#ifdef DEBUG
		printf("saving /usr/X11R6/bin/X as .../X.old ...");
#endif

		/* Move target .../X to .../X.old.  Abort on error. */
		mv_within_target_or_die("/usr/X11R6/bin/X",
					"/usr/X11R6/bin/X.old");
	}

	return 0;
}

/*
 * Merge back saved target /etc files after unpacking the new
 * sets has completed.
 */
static int
merge_etc(void)
{

	if (etc_saved == 0)
		return 0;
	etc_saved = 0;

	/* just move back fstab, so we can boot cleanly.  */
	cp_within_target("/etc.old/fstab", "/etc/", 0);

	return 0;	
}

/*
 * Merge back saved target X files after unpacking the new
 * sets has completed.
 */
static int
merge_X(void)
{
	if (target_symlink_exists_p("/usr/X11R6/bin/X.old")) {
		/* Only move back X if it's a symlink - we don't want
		 * to restore old binaries */
		mv_within_target_or_die("/usr/X11R6/bin/X.old",
					"/usr/X11R6/bin/X");
	}

	return 0;	
}

/*
 * Unpacks sets,  clobbering existing contents.
 */
void
do_reinstall_sets(void)
{

	unwind_mounts();
	msg_display(MSG_reinstallusure);
	process_menu(MENU_noyes, NULL);
	if (!yesno)
		return;

	if (find_disks(msg_string(MSG_reinstall)) < 0)
		return;

	process_menu(MENU_distset, NULL);

	if (mount_disks() != 0)
		return;

	/* Unpack the distribution. */
	if (get_and_unpack_sets(0, MSG_unpackcomplete, MSG_abortunpack) != 0)
		return;

	sanity_check();
}
