/*	$NetBSD: pvdisplay.c,v 1.1.1.2 2009/12/02 00:25:54 haad Exp $	*/

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

static int _pvdisplay_single(struct cmd_context *cmd,
			     struct volume_group *vg,
			     struct physical_volume *pv, void *handle)
{
	struct pv_list *pvl;
	int ret = ECMD_PROCESSED;
	uint64_t size;
	struct volume_group *old_vg = vg;

	const char *pv_name = pv_dev_name(pv);
	const char *vg_name = NULL;

	if (!is_orphan(pv) && !vg) {
		vg_name = pv_vg_name(pv);
		vg = vg_read(cmd, vg_name, (char *)&pv->vgid, 0);
		if (vg_read_error(vg)) {
			log_error("Skipping volume group %s", vg_name);
			vg_release(vg);
			/* FIXME If CLUSTERED should return ECMD_PROCESSED here */
			return ECMD_FAILED;
		}

	 	/*
		 * Replace possibly incomplete PV structure with new one
		 * allocated in vg_read_internal() path.
		 */
		 if (!(pvl = find_pv_in_vg(vg, pv_name))) {
			 log_error("Unable to find \"%s\" in volume group \"%s\"",
				   pv_name, vg->name);
			 ret = ECMD_FAILED;
			 goto out;
		 }

		 pv = pvl->pv;
	}

	if (is_orphan(pv))
		size = pv_size(pv);
	else
		size = (pv_pe_count(pv) - pv_pe_alloc_count(pv)) *
			pv_pe_size(pv);

	if (arg_count(cmd, short_ARG)) {
		log_print("Device \"%s\" has a capacity of %s", pv_name,
			  display_size(cmd, size));
		goto out;
	}

	if (pv_status(pv) & EXPORTED_VG)
		log_print("Physical volume \"%s\" of volume group \"%s\" "
			  "is exported", pv_name, pv_vg_name(pv));

	if (is_orphan(pv))
		log_print("\"%s\" is a new physical volume of \"%s\"",
			  pv_name, display_size(cmd, size));

	if (arg_count(cmd, colon_ARG)) {
		pvdisplay_colons(pv);
		goto out;
	}

	pvdisplay_full(cmd, pv, handle);

	if (arg_count(cmd, maps_ARG))
		pvdisplay_segments(pv);

out:
	if (vg_name)
		unlock_vg(cmd, vg_name);
	if (!old_vg)
		vg_release(vg);

	return ret;
}

int pvdisplay(struct cmd_context *cmd, int argc, char **argv)
{
	if (arg_count(cmd, columns_ARG)) {
		if (arg_count(cmd, colon_ARG) || arg_count(cmd, maps_ARG) ||
		    arg_count(cmd, short_ARG)) {
			log_error("Incompatible options selected");
			return EINVALID_CMD_LINE;
		}
		return pvs(cmd, argc, argv);
	} else if (arg_count(cmd, aligned_ARG) ||
		   arg_count(cmd, all_ARG) ||
		   arg_count(cmd, noheadings_ARG) ||
		   arg_count(cmd, options_ARG) ||
		   arg_count(cmd, separator_ARG) ||
		   arg_count(cmd, sort_ARG) || arg_count(cmd, unbuffered_ARG)) {
		log_error("Incompatible options selected");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, colon_ARG) && arg_count(cmd, maps_ARG)) {
		log_error("Option -v not allowed with option -c");
		return EINVALID_CMD_LINE;
	}

	return process_each_pv(cmd, argc, argv, NULL, 0, 0, NULL,
			       _pvdisplay_single);
}
