/*	$NetBSD: upgrade.c,v 1.20 2022/04/21 17:30:15 martin Exp $	*/

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

/* upgrade.c -- upgrade an installation. */

#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include <errno.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local prototypes
 */
static int save_X(const char *);
static int merge_X(const char *);

/*
 * Do the system upgrade.
 */
void
do_upgrade(void)
{
	struct install_partition_desc install = {};
	int retcode = 0;
	partman_go = 0;

	msg_display(MSG_upgradeusure);
	if (!ask_noyes(NULL))
		return;

	if (find_disks(msg_string(MSG_upgrade), !root_is_read_only()) < 0)
		return;

	if (pm->parts == NULL && !pm->cur_system && !pm->no_part) {
		hit_enter_to_continue(MSG_noroot, NULL);
		return;
	}

	if (!pm->cur_system && pm->parts != NULL) {
		if (pm->parts->pscheme->pre_update_verify) {
			if (pm->parts->pscheme->pre_update_verify(pm->parts))
				pm->parts->pscheme->write_to_disk(pm->parts);
		}

		install_desc_from_parts(&install, pm->parts);
	} else if (pm->cur_system) {
		install.cur_system = true;
	}

	set_swap_if_low_ram(&install);

	if (md_pre_update(&install) < 0)
		goto free_install;

	if (mount_disks(&install) != 0)
		goto free_install;


	/*
	 * Save X symlink, ...
	 */
	if (save_X("/usr/X11R6"))
		goto free_install;
	if (save_X("/usr/X11R7"))
		goto free_install;

#ifdef AOUT2ELF
	move_aout_libs();
#endif
	/* Do any md updating of the file systems ... e.g. bootblocks,
	   copy file systems ... */
	if (!md_update(&install))
		goto free_install;

	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);

	/* Done with disks. Ready to get and unpack tarballs. */
	process_menu(MENU_distset, &retcode);
	if (retcode == 0)
		goto free_install;
	if (get_and_unpack_sets(1, MSG_disksetupdoneupdate,
	    MSG_upgrcomplete, MSG_abortupgr) != 0)
		goto free_install;

	if (md_post_extract(&install, true))
		goto free_install;

	merge_X("/usr/X11R6");
	merge_X("/usr/X11R7");

#if CHECK_ENTROPY
	do_add_entropy();
#endif

	sanity_check();

free_install:
	free_install_desc(&install);
}

/*
 * Save X symlink to X.old so it can be recovered later
 */
static int
save_X(const char *xroot)
{
	char newx[MAXPATHLEN], oldx[MAXPATHLEN];

	strlcpy(newx, xroot, sizeof(newx));
	strlcat(newx, "/bin/X", sizeof(newx));
	strlcpy(oldx, newx, sizeof(oldx));
	strlcat(oldx, ".old", sizeof(oldx));

	/* Only care for X if it's a symlink */
	if (target_symlink_exists_p(newx)) {
		if (target_symlink_exists_p(oldx)) {
			msg_fmt_display(MSG_X_oldexists,
			    "%s%s%s%s%s%s%s%s%s%s%s",
			    xroot, xroot, xroot, xroot, xroot, xroot, xroot,
			    xroot, xroot, xroot, xroot);
			hit_enter_to_continue(NULL, NULL);
			return EEXIST;
		}

#ifdef DEBUG
		printf("saving %s as %s ...", newx, oldx);
#endif

		/* Move target .../X to .../X.old.  Abort on error. */
		mv_within_target_or_die(newx, oldx);
	}

	return 0;
}

/*
 * Merge back saved target X files after unpacking the new
 * sets has completed.
 */
static int
merge_X(const char *xroot)
{
	char newx[MAXPATHLEN], oldx[MAXPATHLEN];

	strlcpy(newx, xroot, sizeof(newx));
	strlcat(newx, "/bin/X", sizeof(newx));
	strlcpy(oldx, newx, sizeof(oldx));
	strlcat(oldx, ".old", sizeof(oldx));

	if (target_symlink_exists_p(oldx)) {
		/* Only move back X if it's a symlink - we don't want
		 * to restore old binaries */
		mv_within_target_or_die(oldx, newx);
	}

	return 0;
}

/*
 * Unpacks sets,  clobbering existing contents.
 */
void
do_reinstall_sets()
{
	struct install_partition_desc install = {};
	int retcode = 0;
	partman_go = 0;

	unwind_mounts();
	msg_display(MSG_reinstallusure);
	if (!ask_noyes(NULL))
		return;

	if (find_disks(msg_string(MSG_reinstall), !root_is_read_only()) < 0)
		return;

	if (!pm->cur_system && pm->parts != NULL) {
		install_desc_from_parts(&install, pm->parts);
	} else if (pm->cur_system) {
		install.cur_system = true;
	}

	if (mount_disks(&install) != 0)
		goto free_install;

	/* Unpack the distribution. */
	process_menu(MENU_distset, &retcode);
	if (retcode == 0)
		goto free_install;
	if (get_and_unpack_sets(0, NULL, MSG_unpackcomplete, MSG_abortunpack) != 0)
		goto free_install;

#if CHECK_ENTROPY
	do_add_entropy();
#endif

	sanity_check();

free_install:
	free_install_desc(&install);
}
