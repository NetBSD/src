/*	$NetBSD: install.c,v 1.28 2002/04/04 14:26:44 ad Exp $	*/

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

/* install.c -- system installation. */

#include <stdio.h>
#include <curses.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* Do the system install. */

void
do_install()
{
	doingwhat = msg_string(MSG_install);

	msg_display(MSG_installusure);
	process_menu(MENU_noyes);
	if (!yesno)
		return;
#if 0
	if (!askyesno(0))
		return;
#endif
	
	get_ramsize();

	if (find_disks() < 0)
		return;

	if (!strcmp(diskdev, swapdev)) {
		msg_display(MSG_swapactive);
		process_menu(MENU_ok);
		if (set_swap(swapdev, NULL, 0) < 0) {
			msg_display(MSG_swapdelfailed);
			process_menu(MENU_ok);
			return;
		}
	}

	/* if we need the user to mount root, ask them to. */
	if (must_mount_root()) {
		msg_display(MSG_pleasemountroot, diskdev, diskdev, diskdev);
		process_menu(MENU_ok);
		return;
	}

	if (!md_get_info()) {
		msg_display(MSG_abort);
		process_menu(MENU_ok);
		return;
	}

	if (md_make_bsd_partitions() == 0) {
		msg_display(MSG_abort);
		process_menu(MENU_ok);
		return;
	}

	/* Last chance ... do you really want to do this? */
	clear();
	refresh();
	msg_display(MSG_lastchance, diskdev);
	process_menu(MENU_noyes);
	if (!yesno)
		return;

	if (md_pre_disklabel() != 0)
		return;

	if (write_disklabel() != 0)
		return;
	
	if (md_post_disklabel() != 0)
		return;

	if (make_filesystems())
		return;

	if (md_copy_filesystem() != 0)
		return;

	if (make_fstab() != 0)
		return;

	if (md_post_newfs() != 0)
		return;

	/* Done to here. */
	msg_display(MSG_disksetupdone);

	getchar();
	wrefresh(curscr);
	wmove(stdscr,0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	/* Unpack the distribution. */
	if (get_and_unpack_sets(MSG_extractcomplete, MSG_abortinst) != 0)
		return;

	set_timezone();

	set_crypt_type();

	set_root_password();
	
	sanity_check();

	md_cleanup_install();

	msg_display(MSG_instcomplete);
	process_menu(MENU_ok);
}
