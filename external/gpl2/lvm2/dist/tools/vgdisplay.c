/*	$NetBSD: vgdisplay.c,v 1.1.1.2 2009/12/02 00:25:57 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tools.h"

static int vgdisplay_single(struct cmd_context *cmd, const char *vg_name,
			    struct volume_group *vg,
			    void *handle __attribute((unused)))
{
	/* FIXME Do the active check here if activevolumegroups_ARG ? */
	vg_check_status(vg, EXPORTED_VG);

	if (arg_count(cmd, colon_ARG)) {
		vgdisplay_colons(vg);
		return ECMD_PROCESSED;
	}

	if (arg_count(cmd, short_ARG)) {
		vgdisplay_short(vg);
		return ECMD_PROCESSED;
	}

	vgdisplay_full(vg);	/* was vg_show */

	if (arg_count(cmd, verbose_ARG)) {
		vgdisplay_extents(vg);

		process_each_lv_in_vg(cmd, vg, NULL, NULL, NULL,
				      (process_single_lv_fn_t)lvdisplay_full);

		log_print("--- Physical volumes ---");
		process_each_pv_in_vg(cmd, vg, NULL, NULL,
				      (process_single_pv_fn_t)pvdisplay_short);
	}

	check_current_backup(vg);

	return ECMD_PROCESSED;
}

int vgdisplay(struct cmd_context *cmd, int argc, char **argv)
{
	if (arg_count(cmd, columns_ARG)) {
		if (arg_count(cmd, colon_ARG) ||
		    arg_count(cmd, activevolumegroups_ARG) ||
		    arg_count(cmd, short_ARG)) {
			log_error("Incompatible options selected");
			return EINVALID_CMD_LINE;
		}
		return vgs(cmd, argc, argv);
	} else if (arg_count(cmd, aligned_ARG) ||
		   arg_count(cmd, noheadings_ARG) ||
		   arg_count(cmd, options_ARG) ||
		   arg_count(cmd, separator_ARG) ||
		   arg_count(cmd, sort_ARG) || arg_count(cmd, unbuffered_ARG)) {
		log_error("Incompatible options selected");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, colon_ARG) && arg_count(cmd, short_ARG)) {
		log_error("Option -c is not allowed with option -s");
		return EINVALID_CMD_LINE;
	}

	if (argc && arg_count(cmd, activevolumegroups_ARG)) {
		log_error("Option -A is not allowed with volume group names");
		return EINVALID_CMD_LINE;
	}

/********* FIXME: Do without this - or else 2(+) passes!
	   Figure out longest volume group name
	for (c = opt; opt < argc; opt++) {
		len = strlen(argv[opt]);
		if (len > max_len)
			max_len = len;
	}
**********/

	return process_each_vg(cmd, argc, argv, 0, NULL,
			       vgdisplay_single);

/******** FIXME Need to count number processed
	  Add this to process_each_vg if arg_count(cmd,activevolumegroups_ARG) ?

	if (opt == argc) {
		log_print("no ");
		if (arg_count(cmd,activevolumegroups_ARG))
			printf("active ");
		printf("volume groups found\n\n");
		return LVM_E_NO_VG;
	}
************/
}
