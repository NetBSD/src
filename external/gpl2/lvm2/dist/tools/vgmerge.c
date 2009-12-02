/*	$NetBSD: vgmerge.c,v 1.1.1.2 2009/12/02 00:25:57 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

static struct volume_group *_vgmerge_vg_read(struct cmd_context *cmd,
					     const char *vg_name)
{
	struct volume_group *vg;
	log_verbose("Checking for volume group \"%s\"", vg_name);
	vg = vg_read_for_update(cmd, vg_name, NULL, 0);
	if (vg_read_error(vg)) {
		vg_release(vg);
		return NULL;
	}
	return vg;
}

static int _vgmerge_single(struct cmd_context *cmd, const char *vg_name_to,
			   const char *vg_name_from)
{
	struct volume_group *vg_to, *vg_from;
	struct lv_list *lvl1, *lvl2;
	int r = ECMD_FAILED;
	int lock_vg_from_first = 0;

	if (!strcmp(vg_name_to, vg_name_from)) {
		log_error("Duplicate volume group name \"%s\"", vg_name_from);
		return ECMD_FAILED;
	}

	if (strcmp(vg_name_to, vg_name_from) > 0)
		lock_vg_from_first = 1;

	if (lock_vg_from_first) {
		vg_from = _vgmerge_vg_read(cmd, vg_name_from);
		if (!vg_from) {
			stack;
			return ECMD_FAILED;
		}
		vg_to = _vgmerge_vg_read(cmd, vg_name_to);
		if (!vg_to) {
			stack;
			unlock_and_release_vg(cmd, vg_from, vg_name_from);
			return ECMD_FAILED;
		}
	} else {
		vg_to = _vgmerge_vg_read(cmd, vg_name_to);
		if (!vg_to) {
			stack;
			return ECMD_FAILED;
		}

		vg_from = _vgmerge_vg_read(cmd, vg_name_from);
		if (!vg_from) {
			stack;
			unlock_and_release_vg(cmd, vg_to, vg_name_to);
			return ECMD_FAILED;
		}
	}

	if (!vgs_are_compatible(cmd, vg_from, vg_to))
		goto_bad;

	/* FIXME List arg: vg_show_with_pv_and_lv(vg_to); */

	if (!archive(vg_from) || !archive(vg_to))
		goto_bad;

	drop_cached_metadata(vg_from);

	/* Merge volume groups */
	while (!dm_list_empty(&vg_from->pvs)) {
		struct dm_list *pvh = vg_from->pvs.n;
		struct physical_volume *pv;

		dm_list_move(&vg_to->pvs, pvh);

		pv = dm_list_item(pvh, struct pv_list)->pv;
		pv->vg_name = dm_pool_strdup(cmd->mem, vg_to->name);
	}
	vg_to->pv_count += vg_from->pv_count;

	/* Fix up LVIDs */
	dm_list_iterate_items(lvl1, &vg_to->lvs) {
		union lvid *lvid1 = &lvl1->lv->lvid;
		char uuid[64] __attribute((aligned(8)));

		dm_list_iterate_items(lvl2, &vg_from->lvs) {
			union lvid *lvid2 = &lvl2->lv->lvid;

			if (id_equal(&lvid1->id[1], &lvid2->id[1])) {
				if (!id_create(&lvid2->id[1])) {
					log_error("Failed to generate new "
						  "random LVID for %s",
						  lvl2->lv->name);
					goto bad;
				}
				if (!id_write_format(&lvid2->id[1], uuid,
						     sizeof(uuid)))
					goto_bad;

				log_verbose("Changed LVID for %s to %s",
					    lvl2->lv->name, uuid);
			}
		}
	}

	while (!dm_list_empty(&vg_from->lvs)) {
		struct dm_list *lvh = vg_from->lvs.n;

		dm_list_move(&vg_to->lvs, lvh);
	}

	while (!dm_list_empty(&vg_from->fid->metadata_areas)) {
		struct dm_list *mdah = vg_from->fid->metadata_areas.n;

		dm_list_move(&vg_to->fid->metadata_areas, mdah);
	}

	vg_to->extent_count += vg_from->extent_count;
	vg_to->free_count += vg_from->free_count;

	/* store it on disks */
	log_verbose("Writing out updated volume group");
	if (!vg_write(vg_to) || !vg_commit(vg_to))
		goto_bad;

	/* FIXME Remove /dev/vgfrom */

	backup(vg_to);
	log_print("Volume group \"%s\" successfully merged into \"%s\"",
		  vg_from->name, vg_to->name);
	r = ECMD_PROCESSED;
bad:
	if (lock_vg_from_first) {
		unlock_and_release_vg(cmd, vg_to, vg_name_to);
		unlock_and_release_vg(cmd, vg_from, vg_name_from);
	} else {
		unlock_and_release_vg(cmd, vg_from, vg_name_from);
		unlock_and_release_vg(cmd, vg_to, vg_name_to);
	}
	return r;
}

int vgmerge(struct cmd_context *cmd, int argc, char **argv)
{
	char *vg_name_to, *vg_name_from;
	int opt = 0;
	int ret = 0, ret_max = 0;

	if (argc < 2) {
		log_error("Please enter 2 or more volume groups to merge");
		return EINVALID_CMD_LINE;
	}

	vg_name_to = skip_dev_dir(cmd, argv[0], NULL);
	argc--;
	argv++;

	for (; opt < argc; opt++) {
		vg_name_from = skip_dev_dir(cmd, argv[opt], NULL);

		ret = _vgmerge_single(cmd, vg_name_to, vg_name_from);
		if (ret > ret_max)
			ret_max = ret;
	}

	return ret_max;
}
