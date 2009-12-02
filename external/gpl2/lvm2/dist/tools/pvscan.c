/*	$NetBSD: pvscan.c,v 1.1.1.2 2009/12/02 00:25:54 haad Exp $	*/

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

int pv_max_name_len = 0;
int vg_max_name_len = 0;

static void _pvscan_display_single(struct cmd_context *cmd,
				   struct physical_volume *pv,
				   void *handle __attribute((unused)))
{
	char uuid[64] __attribute((aligned(8)));
	unsigned vg_name_len = 0;

	char pv_tmp_name[NAME_LEN] = { 0, };
	char vg_tmp_name[NAME_LEN] = { 0, };
	char vg_name_this[NAME_LEN] = { 0, };

	/* short listing? */
	if (arg_count(cmd, short_ARG) > 0) {
		log_print("%s", pv_dev_name(pv));
		return;
	}

	if (arg_count(cmd, verbose_ARG) > 1) {
		/* FIXME As per pv_display! Drop through for now. */
		/* pv_show(pv); */

		/* FIXME - Moved to Volume Group structure */
		/* log_print("System Id             %s", pv->vg->system_id); */

		/* log_print(" "); */
		/* return; */
	}

	memset(pv_tmp_name, 0, sizeof(pv_tmp_name));

	vg_name_len = strlen(pv_vg_name(pv)) + 1;

	if (arg_count(cmd, uuid_ARG)) {
		if (!id_write_format(&pv->id, uuid, sizeof(uuid))) {
			stack;
			return;
		}

		sprintf(pv_tmp_name, "%-*s with UUID %s",
			pv_max_name_len - 2, pv_dev_name(pv), uuid);
	} else {
		sprintf(pv_tmp_name, "%s", pv_dev_name(pv));
	}

	if (is_orphan(pv)) {
		log_print("PV %-*s    %-*s %s [%s]",
			  pv_max_name_len, pv_tmp_name,
			  vg_max_name_len, " ",
			  pv->fmt ? pv->fmt->name : "    ",
			  display_size(cmd, pv_size(pv)));
		return;
	}

	if (pv_status(pv) & EXPORTED_VG) {
		strncpy(vg_name_this, pv_vg_name(pv), vg_name_len);
		log_print("PV %-*s  is in exported VG %s "
			  "[%s / %s free]",
			  pv_max_name_len, pv_tmp_name,
			  vg_name_this,
			  display_size(cmd, (uint64_t) pv_pe_count(pv) *
				       pv_pe_size(pv)),
			  display_size(cmd, (uint64_t) (pv_pe_count(pv) -
						pv_pe_alloc_count(pv))
				       * pv_pe_size(pv)));
		return;
	}

	sprintf(vg_tmp_name, "%s", pv_vg_name(pv));
	log_print("PV %-*s VG %-*s %s [%s / %s free]", pv_max_name_len,
		  pv_tmp_name, vg_max_name_len, vg_tmp_name,
		  pv->fmt ? pv->fmt->name : "    ",
		  display_size(cmd, (uint64_t) pv_pe_count(pv) *
					       pv_pe_size(pv)),
		  display_size(cmd, (uint64_t) (pv_pe_count(pv) -
						pv_pe_alloc_count(pv)) *
					   pv_pe_size(pv)));
	return;
}

int pvscan(struct cmd_context *cmd, int argc __attribute((unused)),
	   char **argv __attribute((unused)))
{
	int new_pvs_found = 0;
	int pvs_found = 0;

	struct dm_list *pvslist;
	struct pv_list *pvl;
	struct physical_volume *pv;

	uint64_t size_total = 0;
	uint64_t size_new = 0;

	int len = 0;
	pv_max_name_len = 0;
	vg_max_name_len = 0;

	if (arg_count(cmd, novolumegroup_ARG) && arg_count(cmd, exported_ARG)) {
		log_error("Options -e and -n are incompatible");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, exported_ARG) || arg_count(cmd, novolumegroup_ARG))
		log_warn("WARNING: only considering physical volumes %s",
			  arg_count(cmd, exported_ARG) ?
			  "of exported volume group(s)" : "in no volume group");

	if (!lock_vol(cmd, VG_GLOBAL, LCK_VG_WRITE)) {
		log_error("Unable to obtain global lock.");
		return ECMD_FAILED;
	}

	persistent_filter_wipe(cmd->filter);
	lvmcache_destroy(cmd, 1);

	log_verbose("Walking through all physical volumes");
	if (!(pvslist = get_pvs(cmd))) {
		unlock_vg(cmd, VG_GLOBAL);
		stack;
		return ECMD_FAILED;
	}

	/* eliminate exported/new if required */
	dm_list_iterate_items(pvl, pvslist) {
		pv = pvl->pv;

		if ((arg_count(cmd, exported_ARG)
		     && !(pv_status(pv) & EXPORTED_VG))
		    || (arg_count(cmd, novolumegroup_ARG) && (!is_orphan(pv)))) {
			dm_list_del(&pvl->list);
			continue;
		}

		/* Also check for MD use? */
/*******
		if (MAJOR(pv_create_kdev_t(pv[p]->pv_name)) != MD_MAJOR) {
			log_print
			    ("WARNING: physical volume \"%s\" belongs to a meta device",
			     pv[p]->pv_name);
		}
		if (MAJOR(pv[p]->pv_dev) != MD_MAJOR)
			continue;
********/
		pvs_found++;

		if (is_orphan(pv)) {
			new_pvs_found++;
			size_new += pv_size(pv);
			size_total += pv_size(pv);
		} else
			size_total += pv_pe_count(pv) * pv_pe_size(pv);
	}

	/* find maximum pv name length */
	pv_max_name_len = vg_max_name_len = 0;
	dm_list_iterate_items(pvl, pvslist) {
		pv = pvl->pv;
		len = strlen(pv_dev_name(pv));
		if (pv_max_name_len < len)
			pv_max_name_len = len;
		len = strlen(pv_vg_name(pv));
		if (vg_max_name_len < len)
			vg_max_name_len = len;
	}
	pv_max_name_len += 2;
	vg_max_name_len += 2;

	dm_list_iterate_items(pvl, pvslist)
	    _pvscan_display_single(cmd, pvl->pv, NULL);

	if (!pvs_found) {
		log_print("No matching physical volumes found");
		unlock_vg(cmd, VG_GLOBAL);
		return ECMD_PROCESSED;
	}

	log_print("Total: %d [%s] / in use: %d [%s] / in no VG: %d [%s]",
		  pvs_found,
		  display_size(cmd, size_total),
		  pvs_found - new_pvs_found,
		  display_size(cmd, (size_total - size_new)),
		  new_pvs_found, display_size(cmd, size_new));

	unlock_vg(cmd, VG_GLOBAL);

	return ECMD_PROCESSED;
}
