/*	$NetBSD: install.c,v 1.22 2022/01/29 16:01:16 martin Exp $	*/

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

#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * helper function: call the md pre/post disklabel callback for all involved
 * inner partitions and write them back.
 * return net result
 */
static bool
write_all_parts(struct install_partition_desc *install)
{
	struct disk_partitions **allparts, *parts;
#ifndef NO_CLONES
	struct selected_partition *src;
#endif
	size_t num_parts, i, j;
	bool found, res;

	/* pessimistic assumption: all partitions on different devices */
	allparts = calloc(install->num + install->num_write_back,
	    sizeof(*allparts));
	if (allparts == NULL)
		return false;

	/* collect all different partition sets */
	num_parts = 0;
	for (i = 0; i < install->num_write_back; i++) {
		parts = install->write_back[i];
		if (parts == NULL)
			continue;
		found = false;
		for (j = 0; j < num_parts; j++) {
			if (allparts[j] == parts) {
				found = true;
				break;
			}
		}
		if (found)
			continue;
		allparts[num_parts++] = parts;
	}
	for (i = 0; i < install->num; i++) {
		parts = install->infos[i].parts;
		if (parts == NULL)
			continue;
		found = false;
		for (j = 0; j < num_parts; j++) {
			if (allparts[j] == parts) {
				found = true;
				break;
			}
		}
		if (found)
			continue;
		allparts[num_parts++] = parts;
	}

	/* do multiple phases, abort anytime and go out, returning res */

	res = true;

	/* phase 1: pre disklabel - used to write MBR and similar */
	for (i = 0; i < num_parts; i++) {
		if (!md_pre_disklabel(install, allparts[i])) {
			res = false;
			goto out;
		}
	}

	/* phase 2: write our partitions (used to be: disklabel) */
	for (i = 0; i < num_parts; i++) {
		if (!allparts[i]->pscheme->write_to_disk(allparts[i])) {
			res = false;
			goto out;
		}
	}

	/* phase 3: now we may have a first chance to enable swap space */
	set_swap_if_low_ram(install);

#ifndef NO_CLONES
	/* phase 4: copy any cloned partitions data (if requested) */
	for (i = 0; i < install->num; i++) {
		if ((install->infos[i].flags & PUIFLG_CLONE_PARTS) == 0
		    || install->infos[i].clone_src == NULL
		    || !install->infos[i].clone_src->with_data)
			continue;
		src = &install->infos[i].clone_src
		    ->selection[install->infos[i].clone_ndx];
		clone_partition_data(install->infos[i].parts,
		    install->infos[i].cur_part_id,
		    src->parts, src->id);
	}
#endif

	/* phase 5: post disklabel (used for updating boot loaders) */
	for (i = 0; i < num_parts; i++) {
		if (!md_post_disklabel(install, allparts[i])) {
			res = false;
			goto out;
		}
	}

out:
	free(allparts);

	return res;
}

/* Do the system install. */

void
do_install(void)
{
	int find_disks_ret;
	int retcode = 0, res;
	struct install_partition_desc install = {};

#ifndef NO_PARTMAN
	partman_go = -1;
#else
	partman_go = 0;
#endif

#ifndef DEBUG
	msg_display(MSG_installusure);
	if (!ask_noyes(NULL))
		return;
#endif

	memset(&install, 0, sizeof install);

	/* Create and mount partitions */
	find_disks_ret = find_disks(msg_string(MSG_install), false);
	if (partman_go == 1) {
		if (partman() < 0) {
			hit_enter_to_continue(MSG_abort_part, NULL);
			return;
		}
	} else if (find_disks_ret < 0)
		return;
	else {
	/* Classical partitioning wizard */
		partman_go = 0;
		clear();
		refresh();

		if (check_swap(pm->diskdev, 0) > 0) {
			hit_enter_to_continue(MSG_swapactive, NULL);
			if (check_swap(pm->diskdev, 1) < 0) {
				hit_enter_to_continue(MSG_swapdelfailed, NULL);
				if (!debug)
					return;
			}
		}

		for (;;) {
			if (md_get_info(&install)) {
				res = md_make_bsd_partitions(&install);
				if (res == -1) {
					pm->parts = NULL;
					continue;
				} else if (res == 1) {
					break;
				}
			}
			hit_enter_to_continue(MSG_abort_inst, NULL);
			goto error;
		}

		/* Last chance ... do you really want to do this? */
		clear();
		refresh();
		msg_fmt_display(MSG_lastchance, "%s", pm->diskdev);
		if (!ask_noyes(NULL))
			goto error;

		if ((!pm->no_part && !write_all_parts(&install)) ||
		    make_filesystems(&install) ||
		    make_fstab(&install) != 0 ||
		    md_post_newfs(&install) != 0)
		goto error;
	}

	/* Unpack the distribution. */
	process_menu(MENU_distset, &retcode);
	if (retcode == 0)
		goto error;
	if (get_and_unpack_sets(0, MSG_disksetupdone,
	    MSG_extractcomplete, MSG_abortinst) != 0)
		goto error;

	if (md_post_extract(&install, false) != 0)
		goto error;

	do_configmenu(&install);

	sanity_check();

	md_cleanup_install(&install);

	hit_enter_to_continue(MSG_instcomplete, NULL);

error:
	free_install_desc(&install);
}
