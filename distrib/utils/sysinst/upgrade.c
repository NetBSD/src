/*	$NetBSD: upgrade.c,v 1.18 1999/06/20 06:08:15 cgd Exp $	*/

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
void 	check_prereqs __P((void));
int	save_etc __P((void));
int	merge_etc __P((void));

/*
 * Do the system upgrade.
 */
void
do_upgrade()
{

	doingwhat = msg_string(MSG_upgrade);

	msg_display(MSG_upgradeusure);
	process_menu(MENU_noyes);
	if (!yesno)
		return;

	get_ramsize();

	if (find_disks() < 0)
		return;

	/* if we need the user to mount root, ask them to. */
	if (must_mount_root()) {
		msg_display(MSG_pleasemountroot, diskdev, diskdev, diskdev);
		process_menu(MENU_ok);
		return;
	}

	if (!fsck_disks())
		return;

	/*
	 * Move target /etc -> target /etc.old so existing configuration
	 * isn't overwritten by upgrade.
	 */
	if (save_etc())
		return;


	/* Do any md updating of the file systems ... e.g. bootblocks,
	   copy file systems ... */
	if (!md_update())
		return;

	/* Done with disks. Ready to get and unpack tarballs. */
	msg_display(MSG_disksetupdone);
	getchar();
	puts(CL); /* just to make sure */
	wclear(stdscr);
	wrefresh(stdscr);

	get_and_unpack_sets(MSG_upgrcomplete, MSG_abortupgr);

	/* Copy back any files we should restore after the upgrade.*/
	merge_etc();

	sanity_check();
}

/*
 * save target /etc files.
 * if target /etc.old exists, print a warning message and give up.
 * otherwise move /etc into target /etc.old, and then copy
 * back files we might want during the installation --  in case 
 * we are upgrading the target root.
 */
int
save_etc()
{

	if (target_dir_exists_p("/etc.old")) {
		msg_display(MSG_etc_oldexists);
		process_menu(MENU_ok);
		return EEXIST;
	}

#ifdef DEBUG
	printf("saving /etc as /etc.old...");
#endif

	/* Move target /etc to /etc.old.  Abort on error. */
	mv_within_target_or_die("/etc", "/etc.old");

	/* now make an /etc that should let the user reboot. */
	make_target_dir("/etc");

#ifdef DEBUG
	printf("Copying essential files back into new /etc...");
#endif
	/* essential stuff */
	cp_within_target("/etc.old/ld.so.cache", "/etc/");
	cp_within_target("/etc.old/ld.so.conf", "/etc/");
	cp_within_target("/etc.old/resolv.conf", "/etc/");

	/* 
	 * do NOT create fstab so that restarting an incomplete
	 * upgrade (eg., after power failure) will fail, and
	 * force the user to check and restore their old /etc.
	 */

	/* general config */
	cp_within_target("/etc.old/rc", "/etc/");
	cp_within_target("/etc.old/rc.conf", "/etc/");
	cp_within_target("/etc.old/rc.local", "/etc/");

	/* network config */
	cp_within_target("/etc.old/ifconfig.*", "/etc/");
	cp_within_target("/etc.old/myname", "/etc/");
	cp_within_target("/etc.old/mygate", "/etc/");
	cp_within_target("/etc.old/defaultdomain", "/etc/");

	/* old-style network config */
	cp_within_target("/etc.old/hostname.*", "/etc/");

	return 0;
}

/*
 * Merge back saved target /etc files after unpacking the new
 * sets has completed.
 */
int
merge_etc()
{

	/* just move back fstab, so we can boot cleanly.  */
	cp_within_target("/etc.old/fstab", "/etc/");

	return 0;	
}

/*
 * Unpacks sets,  clobbering existintg contents.
 */
void
do_reinstall_sets()
{
	doingwhat = msg_string(MSG_reinstall);

	unwind_mounts();
	msg_display(MSG_reinstallusure);
	process_menu(MENU_noyes);
	if (!yesno)
		return;

	if (find_disks() < 0)
		return;

	/* if we need the user to mount root, ask them to. */
	if (must_mount_root()) {
		msg_display(MSG_pleasemountroot, diskdev, diskdev, diskdev);
		process_menu(MENU_ok);
		return;
	}

	if (!fsck_disks())
		return;

	fflush(stdout);
	puts(CL);
	wrefresh(stdscr);

	/* Unpack the distribution. */
	get_and_unpack_sets(MSG_unpackcomplete, MSG_abortunpack);

	sanity_check();

}
