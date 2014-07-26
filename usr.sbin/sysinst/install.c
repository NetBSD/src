/*	$NetBSD: install.c,v 1.1 2014/07/26 19:30:44 dholland Exp $	*/

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

/* install.c -- system installation. */

#include <stdio.h>
#include <curses.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* Do the system install. */

void
do_install(void)
{

	msg_display(MSG_installusure);
	process_menu(MENU_noyes, NULL);
	if (!yesno)
		return;

	get_ramsize();

	if (find_disks(msg_string(MSG_install)) < 0)
		return;
	clear();
	refresh();

	if (check_swap(diskdev, 0) > 0) {
		msg_display(MSG_swapactive);
		process_menu(MENU_ok, NULL);
		if (check_swap(diskdev, 1) < 0) {
			msg_display(MSG_swapdelfailed);
			process_menu(MENU_ok, NULL);
			if (!debug)
				return;
		}
	}

	process_menu(MENU_distset, NULL);

	if (!md_get_info()) {
		msg_display(MSG_abort);
		process_menu(MENU_ok, NULL);
		return;
	}

	if (md_make_bsd_partitions() == 0) {
		msg_display(MSG_abort);
		process_menu(MENU_ok, NULL);
		return;
	}

	/* Last chance ... do you really want to do this? */
	clear();
	refresh();
	msg_display(MSG_lastchance, diskdev);
	process_menu(MENU_noyes, NULL);
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

	if (make_fstab() != 0)
		return;

	if (md_post_newfs() != 0)
		return;

	/* Unpack the distribution. */
	if (get_and_unpack_sets(0, MSG_disksetupdone,
	    MSG_extractcomplete, MSG_abortinst) != 0)
		return;

	if (md_post_extract() != 0)
		return;

	do_configmenu();

	sanity_check();

	md_cleanup_install();

	msg_display(MSG_instcomplete);
	process_menu(MENU_ok, NULL);
}
