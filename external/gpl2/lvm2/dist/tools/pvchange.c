/*	$NetBSD: pvchange.c,v 1.1.1.2 2009/12/02 00:25:54 haad Exp $	*/

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

/* FIXME Locking.  PVs in VG. */

static int _pvchange_single(struct cmd_context *cmd, struct physical_volume *pv,
			    void *handle __attribute((unused)))
{
	struct volume_group *vg = NULL;
	const char *vg_name = NULL;
	struct pv_list *pvl;
	uint64_t sector;
	uint32_t orig_pe_alloc_count;
	/* FIXME Next three only required for format1. */
	uint32_t orig_pe_count, orig_pe_size;
	uint64_t orig_pe_start;

	const char *pv_name = pv_dev_name(pv);
	const char *tag = NULL;
	const char *orig_vg_name;
	char uuid[64] __attribute((aligned(8)));

	int allocatable = 0;
	int tagarg = 0;
	int r = 0;

	if (arg_count(cmd, addtag_ARG))
		tagarg = addtag_ARG;
	else if (arg_count(cmd, deltag_ARG))
		tagarg = deltag_ARG;

	if (arg_count(cmd, allocatable_ARG))
		allocatable = !strcmp(arg_str_value(cmd, allocatable_ARG, "n"),
				      "y");
	else if (tagarg && !(tag = arg_str_value(cmd, tagarg, NULL))) {
		log_error("Failed to get tag");
		return 0;
	}

	/* If in a VG, must change using volume group. */
	if (!is_orphan(pv)) {
		vg_name = pv_vg_name(pv);

		log_verbose("Finding volume group %s of physical volume %s",
			    vg_name, pv_name);
		vg = vg_read_for_update(cmd, vg_name, NULL, 0);
		if (vg_read_error(vg)) {
			vg_release(vg);
			return_0;
		}

		if (!(pvl = find_pv_in_vg(vg, pv_name))) {
			log_error("Unable to find \"%s\" in volume group \"%s\"",
				  pv_name, vg->name);
			goto out;
		}
		if (tagarg && !(vg->fid->fmt->features & FMT_TAGS)) {
			log_error("Volume group containing %s does not "
				  "support tags", pv_name);
			goto out;
		}
		if (arg_count(cmd, uuid_ARG) && lvs_in_vg_activated(vg)) {
			log_error("Volume group containing %s has active "
				  "logical volumes", pv_name);
			goto out;
		}
		pv = pvl->pv;
		if (!archive(vg))
			goto out;
	} else {
		if (tagarg) {
			log_error("Can't change tag on Physical Volume %s not "
				  "in volume group", pv_name);
			return 0;
		}

		vg_name = VG_ORPHANS;

		if (!lock_vol(cmd, vg_name, LCK_VG_WRITE)) {
			log_error("Can't get lock for orphans");
			return 0;
		}

		if (!(pv = pv_read(cmd, pv_name, NULL, &sector, 1, 0))) {
			unlock_vg(cmd, vg_name);
			log_error("Unable to read PV \"%s\"", pv_name);
			return 0;
		}
	}

	if (arg_count(cmd, allocatable_ARG)) {
		if (is_orphan(pv) &&
		    !(pv->fmt->features & FMT_ORPHAN_ALLOCATABLE)) {
			log_error("Allocatability not supported by orphan "
				  "%s format PV %s", pv->fmt->name, pv_name);
			goto out;
		}

		/* change allocatability for a PV */
		if (allocatable && (pv_status(pv) & ALLOCATABLE_PV)) {
			log_error("Physical volume \"%s\" is already "
				  "allocatable", pv_name);
			r = 1;
			goto out;
		}

		if (!allocatable && !(pv_status(pv) & ALLOCATABLE_PV)) {
			log_error("Physical volume \"%s\" is already "
				  "unallocatable", pv_name);
			r = 1;
			goto out;
		}

		if (allocatable) {
			log_verbose("Setting physical volume \"%s\" "
				    "allocatable", pv_name);
			pv->status |= ALLOCATABLE_PV;
		} else {
			log_verbose("Setting physical volume \"%s\" NOT "
				    "allocatable", pv_name);
			pv->status &= ~ALLOCATABLE_PV;
		}
	} else if (tagarg) {
		/* tag or deltag */
		if ((tagarg == addtag_ARG)) {
			if (!str_list_add(cmd->mem, &pv->tags, tag)) {
				log_error("Failed to add tag %s to physical "
					  "volume %s", tag, pv_name);
				goto out;
			}
		} else {
			if (!str_list_del(&pv->tags, tag)) {
				log_error("Failed to remove tag %s from "
					  "physical volume" "%s", tag, pv_name);
				goto out;
			}
		}
	} else {
		/* --uuid: Change PV ID randomly */
		if (!id_create(&pv->id)) {
			log_error("Failed to generate new random UUID for %s.",
				  pv_name);
			goto out;
		}
		if (!id_write_format(&pv->id, uuid, sizeof(uuid)))
			goto_out;
		log_verbose("Changing uuid of %s to %s.", pv_name, uuid);
		if (!is_orphan(pv)) {
			orig_vg_name = pv_vg_name(pv);
			orig_pe_alloc_count = pv_pe_alloc_count(pv);

			/* FIXME format1 pv_write doesn't preserve these. */
			orig_pe_size = pv_pe_size(pv);
			orig_pe_start = pv_pe_start(pv);
			orig_pe_count = pv_pe_count(pv);

			pv->vg_name = pv->fmt->orphan_vg_name;
			pv->pe_alloc_count = 0;
			if (!(pv_write(cmd, pv, NULL, INT64_C(-1)))) {
				log_error("pv_write with new uuid failed "
					  "for %s.", pv_name);
				goto out;
			}
			pv->vg_name = orig_vg_name;
			pv->pe_alloc_count = orig_pe_alloc_count;

			pv->pe_size = orig_pe_size;
			pv->pe_start = orig_pe_start;
			pv->pe_count = orig_pe_count;
		}
	}

	log_verbose("Updating physical volume \"%s\"", pv_name);
	if (!is_orphan(pv)) {
		if (!vg_write(vg) || !vg_commit(vg)) {
			log_error("Failed to store physical volume \"%s\" in "
				  "volume group \"%s\"", pv_name, vg->name);
			goto out;
		}
		backup(vg);
	} else if (!(pv_write(cmd, pv, NULL, INT64_C(-1)))) {
		log_error("Failed to store physical volume \"%s\"",
			  pv_name);
		goto out;
	}

	log_print("Physical volume \"%s\" changed", pv_name);
	r = 1;
out:
	unlock_and_release_vg(cmd, vg, vg_name);
	return r;

}

int pvchange(struct cmd_context *cmd, int argc, char **argv)
{
	int opt = 0;
	int done = 0;
	int total = 0;

	struct physical_volume *pv;
	char *pv_name;

	struct pv_list *pvl;
	struct dm_list *pvslist;
	struct dm_list mdas;

	if (arg_count(cmd, allocatable_ARG) + arg_count(cmd, addtag_ARG) +
	    arg_count(cmd, deltag_ARG) + arg_count(cmd, uuid_ARG) != 1) {
		log_error("Please give exactly one option of -x, -uuid, "
			  "--addtag or --deltag");
		return EINVALID_CMD_LINE;
	}

	if (!(arg_count(cmd, all_ARG)) && !argc) {
		log_error("Please give a physical volume path");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, all_ARG) && argc) {
		log_error("Option a and PhysicalVolumePath are exclusive");
		return EINVALID_CMD_LINE;
	}

	if (argc) {
		log_verbose("Using physical volume(s) on command line");
		for (; opt < argc; opt++) {
			pv_name = argv[opt];
			dm_list_init(&mdas);
			if (!(pv = pv_read(cmd, pv_name, &mdas, NULL, 1, 0))) {
				log_error("Failed to read physical volume %s",
					  pv_name);
				continue;
			}
			/*
			 * If a PV has no MDAs it may appear to be an
			 * orphan until the metadata is read off
			 * another PV in the same VG.  Detecting this
			 * means checking every VG by scanning every
			 * PV on the system.
			 */
			if (is_orphan(pv) && !dm_list_size(&mdas)) {
				if (!scan_vgs_for_pvs(cmd)) {
					log_error("Rescan for PVs without "
						  "metadata areas failed.");
					continue;
				}
				if (!(pv = pv_read(cmd, pv_name,
						   NULL, NULL, 1, 0))) {
					log_error("Failed to read "
						  "physical volume %s",
						  pv_name);
					continue;
				}
			}

			total++;
			done += _pvchange_single(cmd, pv, NULL);
		}
	} else {
		log_verbose("Scanning for physical volume names");
		if (!(pvslist = get_pvs(cmd))) {
			stack;
			return ECMD_FAILED;
		}

		dm_list_iterate_items(pvl, pvslist) {
			total++;
			done += _pvchange_single(cmd, pvl->pv, NULL);
		}
	}

	log_print("%d physical volume%s changed / %d physical volume%s "
		  "not changed",
		  done, done == 1 ? "" : "s",
		  total - done, (total - done) == 1 ? "" : "s");

	return (total == done) ? ECMD_PROCESSED : ECMD_FAILED;
}
