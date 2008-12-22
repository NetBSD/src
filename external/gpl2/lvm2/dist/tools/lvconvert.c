/*	$NetBSD: lvconvert.c,v 1.1.1.1 2008/12/22 00:19:01 haad Exp $	*/

/*
 * Copyright (C) 2005-2007 Red Hat, Inc. All rights reserved.
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
#include "polldaemon.h"
#include "lv_alloc.h"

struct lvconvert_params {
	int snapshot;
	int zero;

	const char *origin;
	const char *lv_name;
	const char *lv_name_full;
	const char *vg_name;
	int wait_completion;
	int need_polling;

	uint32_t chunk_size;
	uint32_t region_size;

	uint32_t mirrors;
	sign_t mirrors_sign;

	struct segment_type *segtype;

	alloc_policy_t alloc;

	int pv_count;
	char **pvs;
	struct dm_list *pvh;
};

static int _lvconvert_name_params(struct lvconvert_params *lp,
				  struct cmd_context *cmd,
				  int *pargc, char ***pargv)
{
	char *ptr;
	const char *vg_name = NULL;

	if (lp->snapshot) {
		if (!*pargc) {
			log_error("Please specify a logical volume to act as "
				  "the snapshot origin.");
			return 0;
		}

		lp->origin = *pargv[0];
		(*pargv)++, (*pargc)--;
		if (!(lp->vg_name = extract_vgname(cmd, lp->origin))) {
			log_error("The origin name should include the "
				  "volume group.");
			return 0;
		}

		/* Strip the volume group from the origin */
		if ((ptr = strrchr(lp->origin, (int) '/')))
			lp->origin = ptr + 1;
	}

	if (!*pargc) {
		log_error("Please provide logical volume path");
		return 0;
	}

	lp->lv_name = lp->lv_name_full = (*pargv)[0];
	(*pargv)++, (*pargc)--;

	if (strchr(lp->lv_name_full, '/') &&
	    (vg_name = extract_vgname(cmd, lp->lv_name_full)) &&
	    lp->vg_name && strcmp(vg_name, lp->vg_name)) {
		log_error("Please use a single volume group name "
			  "(\"%s\" or \"%s\")", vg_name, lp->vg_name);
		return 0;
	}

	if (!lp->vg_name)
		lp->vg_name = vg_name;

	if (!validate_name(lp->vg_name)) {
		log_error("Please provide a valid volume group name");
		return 0;
	}

	if ((ptr = strrchr(lp->lv_name_full, '/')))
		lp->lv_name = ptr + 1;

	if (!apply_lvname_restrictions(lp->lv_name))
		return_0;

	return 1;
}

static int _read_params(struct lvconvert_params *lp, struct cmd_context *cmd,
			int argc, char **argv)
{
	int region_size;
	int pagesize = lvm_getpagesize();

	memset(lp, 0, sizeof(*lp));

	if (arg_count(cmd, snapshot_ARG) &&
	    (arg_count(cmd, mirrorlog_ARG) || arg_count(cmd, mirrors_ARG))) {
		log_error("--snapshots argument cannot be mixed "
			  "with --mirrors or --log");
		return 0;
	}

	if (!arg_count(cmd, background_ARG))
		lp->wait_completion = 1;

	if (arg_count(cmd, snapshot_ARG))
		lp->snapshot = 1;

	if (arg_count(cmd, mirrors_ARG)) {
		lp->mirrors = arg_uint_value(cmd, mirrors_ARG, 0);
		lp->mirrors_sign = arg_sign_value(cmd, mirrors_ARG, 0);
	}

	lp->alloc = ALLOC_INHERIT;
	if (arg_count(cmd, alloc_ARG))
		lp->alloc = arg_uint_value(cmd, alloc_ARG, lp->alloc);

	if (lp->snapshot) {
		if (arg_count(cmd, regionsize_ARG)) {
			log_error("--regionsize is only available with mirrors");
			return 0;
		}

		if (arg_sign_value(cmd, chunksize_ARG, 0) == SIGN_MINUS) {
			log_error("Negative chunk size is invalid");
			return 0;
		}
		lp->chunk_size = arg_uint_value(cmd, chunksize_ARG, 8);
		if (lp->chunk_size < 8 || lp->chunk_size > 1024 ||
		    (lp->chunk_size & (lp->chunk_size - 1))) {
			log_error("Chunk size must be a power of 2 in the "
				  "range 4K to 512K");
			return 0;
		}
		log_verbose("Setting chunksize to %d sectors.", lp->chunk_size);

		if (!(lp->segtype = get_segtype_from_string(cmd, "snapshot")))
			return_0;

		lp->zero = strcmp(arg_str_value(cmd, zero_ARG,
						(lp->segtype->flags &
						 SEG_CANNOT_BE_ZEROED) ?
						"n" : "y"), "n");

	} else {	/* Mirrors */
		if (arg_count(cmd, chunksize_ARG)) {
			log_error("--chunksize is only available with "
				  "snapshots");
			return 0;
		}

		if (arg_count(cmd, zero_ARG)) {
			log_error("--zero is only available with snapshots");
			return 0;
		}

		/*
	 	 * --regionsize is only valid if converting an LV into a mirror.
	 	 * Checked when we know the state of the LV being converted.
	 	 */

		if (arg_count(cmd, regionsize_ARG)) {
			if (arg_sign_value(cmd, regionsize_ARG, 0) ==
				    SIGN_MINUS) {
				log_error("Negative regionsize is invalid");
				return 0;
			}
			lp->region_size = arg_uint_value(cmd, regionsize_ARG, 0);
		} else {
			region_size = 2 * find_config_tree_int(cmd,
						"activation/mirror_region_size",
						DEFAULT_MIRROR_REGION_SIZE);
			if (region_size < 0) {
				log_error("Negative regionsize in "
					  "configuration file is invalid");
				return 0;
			}
			lp->region_size = region_size;
		}

		if (lp->region_size % (pagesize >> SECTOR_SHIFT)) {
			log_error("Region size (%" PRIu32 ") must be "
				  "a multiple of machine memory "
				  "page size (%d)",
				  lp->region_size, pagesize >> SECTOR_SHIFT);
			return 0;
		}

		if (lp->region_size & (lp->region_size - 1)) {
			log_error("Region size (%" PRIu32
				  ") must be a power of 2", lp->region_size);
			return 0;
		}

		if (!lp->region_size) {
			log_error("Non-zero region size must be supplied.");
			return 0;
		}

		if (!(lp->segtype = get_segtype_from_string(cmd, "mirror")))
			return_0;
	}

	if (activation() && lp->segtype->ops->target_present &&
	    !lp->segtype->ops->target_present(NULL, NULL)) {
		log_error("%s: Required device-mapper target(s) not "
			  "detected in your kernel", lp->segtype->name);
		return 0;
	}

	if (!_lvconvert_name_params(lp, cmd, &argc, &argv))
		return_0;

	lp->pv_count = argc;
	lp->pvs = argv;

	return 1;
}


static struct volume_group *_get_lvconvert_vg(struct cmd_context *cmd,
					      const char *lv_name)
{
	dev_close_all();

        return vg_lock_and_read(cmd, extract_vgname(cmd, lv_name),
				NULL, LCK_VG_WRITE,
 				CLUSTERED | EXPORTED_VG | LVM_WRITE,
				CORRECT_INCONSISTENT | FAIL_INCONSISTENT);
}

static struct logical_volume *_get_lvconvert_lv(struct cmd_context *cmd __attribute((unused)),
						struct volume_group *vg,
						const char *name,
						uint32_t lv_type __attribute((unused)))
{
	return find_lv(vg, name);
}

static int _update_lvconvert_mirror(struct cmd_context *cmd __attribute((unused)),
				    struct volume_group *vg __attribute((unused)),
				    struct logical_volume *lv __attribute((unused)),
				    struct dm_list *lvs_changed __attribute((unused)),
				    unsigned flags __attribute((unused)))
{
	/* lvconvert mirror doesn't require periodical metadata update */
	return 1;
}

static int _finish_lvconvert_mirror(struct cmd_context *cmd,
				    struct volume_group *vg,
				    struct logical_volume *lv,
				    struct dm_list *lvs_changed __attribute((unused)))
{
	if (!collapse_mirrored_lv(lv)) {
		log_error("Failed to remove temporary sync layer.");
		return 0;
	}

	lv->status &= ~CONVERTING;

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);

	if (!vg_write(vg))
		return_0;

	backup(vg);

	if (!suspend_lv(cmd, lv)) {
		log_error("Failed to lock %s", lv->name);
		vg_revert(vg);
		return 0;
	}

	if (!vg_commit(vg)) {
		resume_lv(cmd, lv);
		return 0;
	}

	log_very_verbose("Updating \"%s\" in kernel", lv->name);

	if (!resume_lv(cmd, lv)) {
		log_error("Problem reactivating %s", lv->name);
		return 0;
	}

	log_print("Logical volume %s converted.", lv->name);

	return 1;
}

static struct poll_functions _lvconvert_mirror_fns = {
	.get_copy_vg = _get_lvconvert_vg,
	.get_copy_lv = _get_lvconvert_lv,
	.update_metadata = _update_lvconvert_mirror,
	.finish_copy = _finish_lvconvert_mirror,
};

int lvconvert_poll(struct cmd_context *cmd, const char *lv_name,
		   unsigned background)
{
	return poll_daemon(cmd, lv_name, background, 0, &_lvconvert_mirror_fns,
			   "Converted");
}

static int _insert_lvconvert_layer(struct cmd_context *cmd,
				   struct logical_volume *lv)
{
	char *format, *layer_name;
	size_t len;
	int i;

	/*
 	 * We would like to give the same number for this layer
 	 * and the newly added mimage.
 	 * However, LV name of newly added mimage is determined *after*
	 * the LV name of this layer is determined.
	 *
	 * So, use generate_lv_name() to generate mimage name first
	 * and take the number from it.
	 */

	len = strlen(lv->name) + 32;
	if (!(format = alloca(len)) ||
	    !(layer_name = alloca(len)) ||
	    dm_snprintf(format, len, "%s_mimage_%%d", lv->name) < 0) {
		log_error("lvconvert: layer name allocation failed.");
		return 0;
	}

	if (!generate_lv_name(lv->vg, format, layer_name, len) ||
	    sscanf(layer_name, format, &i) != 1) {
		log_error("lvconvert: layer name generation failed.");
		return 0;
	}

	if (dm_snprintf(layer_name, len, MIRROR_SYNC_LAYER "_%d", i) < 0) {
		log_error("layer name allocation failed.");
		return 0;
	}

	if (!insert_layer_for_lv(cmd, lv, 0, layer_name)) {
		log_error("Failed to insert resync layer");
		return 0;
	}

	return 1;
}

/* walk down the stacked mirror LV to the original mirror LV */
static struct logical_volume *_original_lv(struct logical_volume *lv)
{
	struct logical_volume *next_lv = lv, *tmp_lv;

	while ((tmp_lv = find_temporary_mirror(next_lv)))
		next_lv = tmp_lv;

	return next_lv;
}

static int lvconvert_mirrors(struct cmd_context * cmd, struct logical_volume * lv,
			     struct lvconvert_params *lp)
{
	struct lv_segment *seg;
	uint32_t existing_mirrors;
	const char *mirrorlog;
	unsigned corelog = 0;
	struct logical_volume *original_lv;

	seg = first_seg(lv);
	existing_mirrors = lv_mirror_count(lv);

	/* If called with no argument, try collapsing the resync layers */
	if (!arg_count(cmd, mirrors_ARG) && !arg_count(cmd, mirrorlog_ARG) &&
	    !arg_count(cmd, corelog_ARG) && !arg_count(cmd, regionsize_ARG)) {
		lp->need_polling = 1;
		return 1;
	}

	/*
	 * Adjust required number of mirrors
	 *
	 * We check mirrors_ARG again to see if it
	 * was supplied.  If not, they want the mirror
	 * count to remain the same.  They may be changing
	 * the logging type.
	 */
	if (!arg_count(cmd, mirrors_ARG))
		lp->mirrors = existing_mirrors;
	else if (lp->mirrors_sign == SIGN_PLUS)
		lp->mirrors = existing_mirrors + lp->mirrors;
	else if (lp->mirrors_sign == SIGN_MINUS)
		lp->mirrors = existing_mirrors - lp->mirrors;
	else
		lp->mirrors += 1;

	/*
	 * Did the user try to subtract more legs than available?
	 */
	if (lp->mirrors < 1) {
		log_error("Logical volume %s only has %" PRIu32 " mirrors.",
			  lv->name, existing_mirrors);
		return 0;
	}

	/*
	 * Adjust log type
	 */
	if (arg_count(cmd, corelog_ARG))
		corelog = 1;

	mirrorlog = arg_str_value(cmd, mirrorlog_ARG,
				  corelog ? "core" : DEFAULT_MIRRORLOG);
	if (!strcmp("disk", mirrorlog)) {
		if (corelog) {
			log_error("--mirrorlog disk and --corelog "
				  "are incompatible");
			return 0;
		}
		corelog = 0;
	} else if (!strcmp("core", mirrorlog))
		corelog = 1;
	else {
		log_error("Unknown mirrorlog type: %s", mirrorlog);
		return 0;
	}

	log_verbose("Setting logging type to %s", mirrorlog);

	/*
	 * Region size must not change on existing mirrors
	 */
	if (arg_count(cmd, regionsize_ARG) && (lv->status & MIRRORED) &&
	    (lp->region_size != seg->region_size)) {
		log_error("Mirror log region size cannot be changed on "
			  "an existing mirror.");
		return 0;
	}

	/*
	 * Converting from mirror to linear
	 */
	if ((lp->mirrors == 1)) {
		if (!(lv->status & MIRRORED)) {
			log_error("Logical volume %s is already not mirrored.",
				  lv->name);
			return 1;
		}

		if (!lv_remove_mirrors(cmd, lv, existing_mirrors - 1, 1,
				       lp->pv_count ? lp->pvh : NULL, 0))
			return_0;
		goto commit_changes;
	}

	/*
	 * Converting from linear to mirror
	 */
	if (!(lv->status & MIRRORED)) {
		/* FIXME Share code with lvcreate */

		/* FIXME Why is this restriction here?  Fix it! */
		dm_list_iterate_items(seg, &lv->segments) {
			if (seg_is_striped(seg) && seg->area_count > 1) {
				log_error("Mirrors of striped volumes are not yet supported.");
				return 0;
			}
		}

		if (!lv_add_mirrors(cmd, lv, lp->mirrors - 1, 1,
				    adjusted_mirror_region_size(
						lv->vg->extent_size,
						lv->le_count,
						lp->region_size),
				    corelog ? 0U : 1U, lp->pvh, lp->alloc,
				    MIRROR_BY_LV))
			return_0;
		if (lp->wait_completion)
			lp->need_polling = 1;
		goto commit_changes;
	}

	/*
	 * Converting from mirror to mirror with different leg count,
	 * or different log type.
	 */
	if (dm_list_size(&lv->segments) != 1) {
		log_error("Logical volume %s has multiple "
			  "mirror segments.", lv->name);
		return 0;
	}

	if (lp->mirrors == existing_mirrors) {
		/*
		 * Convert Mirror log type
		 */
		original_lv = _original_lv(lv);
		if (!first_seg(original_lv)->log_lv && !corelog) {
			if (!add_mirror_log(cmd, original_lv, 1,
					    adjusted_mirror_region_size(
							lv->vg->extent_size,
							lv->le_count,
							lp->region_size),
					    lp->pvh, lp->alloc))
				return_0;
		} else if (first_seg(original_lv)->log_lv && corelog) {
			if (!remove_mirror_log(cmd, original_lv,
					       lp->pv_count ? lp->pvh : NULL))
				return_0;
		} else {
			/* No change */
			log_error("Logical volume %s already has %"
				  PRIu32 " mirror(s).", lv->name,
				  lp->mirrors - 1);
			if (lv->status & CONVERTING)
				lp->need_polling = 1;
			return 1;
		}
	} else if (lp->mirrors > existing_mirrors) {
		if (lv->status & MIRROR_NOTSYNCED) {
			log_error("Not adding mirror to mirrored LV "
				  "without initial resync");
			return 0;
		}
		/*
		 * Log addition/removal should be done before the layer
		 * insertion to make the end result consistent with
		 * linear-to-mirror conversion.
		 */
		original_lv = _original_lv(lv);
		if (!first_seg(original_lv)->log_lv && !corelog) {
			if (!add_mirror_log(cmd, original_lv, 1,
					    adjusted_mirror_region_size(
							lv->vg->extent_size,
							lv->le_count,
							lp->region_size),
					    lp->pvh, lp->alloc))
				return_0;
		} else if (first_seg(original_lv)->log_lv && corelog) {
			if (!remove_mirror_log(cmd, original_lv,
					       lp->pv_count ? lp->pvh : NULL))
				return_0;
		}
		/* Insert a temporary layer for syncing,
		 * only if the original lv is using disk log. */
		if (seg->log_lv && !_insert_lvconvert_layer(cmd, lv)) {
			log_error("Failed to insert resync layer");
			return 0;
		}
		/* FIXME: can't have multiple mlogs. force corelog. */
		if (!lv_add_mirrors(cmd, lv, lp->mirrors - existing_mirrors, 1,
				    adjusted_mirror_region_size(
						lv->vg->extent_size,
						lv->le_count,
						lp->region_size),
				    0U, lp->pvh, lp->alloc,
				    MIRROR_BY_LV))
			return_0;
		lv->status |= CONVERTING;
		lp->need_polling = 1;
	} else {
		/* Reduce number of mirrors */
		if (!lv_remove_mirrors(cmd, lv, existing_mirrors - lp->mirrors,
				       corelog ? 1U : 0U,
				       lp->pv_count ? lp->pvh : NULL, 0))
			return_0;
	}

commit_changes:
	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);

	if (!vg_write(lv->vg))
		return_0;

	backup(lv->vg);

	if (!suspend_lv(cmd, lv)) {
		log_error("Failed to lock %s", lv->name);
		vg_revert(lv->vg);
		return 0;
	}

	if (!vg_commit(lv->vg)) {
		resume_lv(cmd, lv);
		return 0;
	}

	log_very_verbose("Updating \"%s\" in kernel", lv->name);

	if (!resume_lv(cmd, lv)) {
		log_error("Problem reactivating %s", lv->name);
		return 0;
	}

	if (!lp->need_polling)
		log_print("Logical volume %s converted.", lv->name);

	return 1;
}

static int lvconvert_snapshot(struct cmd_context *cmd,
			      struct logical_volume *lv,
			      struct lvconvert_params *lp)
{
	struct logical_volume *org;

	if (!(org = find_lv(lv->vg, lp->origin))) {
		log_error("Couldn't find origin volume '%s'.", lp->origin);
		return 0;
	}

	if (org == lv) {
		log_error("Unable to use \"%s\" as both snapshot and origin.",
			  lv->name);
		return 0;
	}

	if (org->status & (LOCKED|PVMOVE|MIRRORED) || lv_is_cow(org)) {
		log_error("Unable to create a snapshot of a %s LV.",
			  org->status & LOCKED ? "locked" :
			  org->status & PVMOVE ? "pvmove" :
			  org->status & MIRRORED ? "mirrored" :
			  "snapshot");
		return 0;
	}

	if (!lp->zero || !(lv->status & LVM_WRITE))
		log_warn("WARNING: \"%s\" not zeroed", lv->name);
	else if (!set_lv(cmd, lv, UINT64_C(0), 0)) {
		log_error("Aborting. Failed to wipe snapshot "
			  "exception store.");
		return 0;
	}

	if (!deactivate_lv(cmd, lv)) {
		log_error("Couldn't deactivate LV %s.", lv->name);
		return 0;
	}

	if (!vg_add_snapshot(NULL, org, lv, NULL, org->le_count,
			     lp->chunk_size)) {
		log_error("Couldn't create snapshot.");
		return 0;
	}

	/* store vg on disk(s) */
	if (!vg_write(lv->vg))
		return_0;

	backup(lv->vg);

	if (!suspend_lv(cmd, org)) {
		log_error("Failed to suspend origin %s", org->name);
		vg_revert(lv->vg);
		return 0;
	}

	if (!vg_commit(lv->vg))
		return_0;

	if (!resume_lv(cmd, org)) {
		log_error("Problem reactivating origin %s", org->name);
		return 0;
	}

	log_print("Logical volume %s converted to snapshot.", lv->name);

	return 1;
}

static int lvconvert_single(struct cmd_context *cmd, struct logical_volume *lv,
			    void *handle)
{
	struct lvconvert_params *lp = handle;

	if (lv->status & LOCKED) {
		log_error("Cannot convert locked LV %s", lv->name);
		return ECMD_FAILED;
	}

	if (lv_is_origin(lv)) {
		log_error("Can't convert logical volume \"%s\" under snapshot",
			  lv->name);
		return ECMD_FAILED;
	}

	if (lv_is_cow(lv)) {
		log_error("Can't convert snapshot logical volume \"%s\"",
			  lv->name);
		return ECMD_FAILED;
	}

	if (lv->status & PVMOVE) {
		log_error("Unable to convert pvmove LV %s", lv->name);
		return ECMD_FAILED;
	}

	if (lp->snapshot) {
		if (lv->status & MIRRORED) {
			log_error("Unable to convert mirrored LV \"%s\" into a snapshot.", lv->name);
			return ECMD_FAILED;
		}
		if (!archive(lv->vg))
			return ECMD_FAILED;
		if (!lvconvert_snapshot(cmd, lv, lp))
			return ECMD_FAILED;
	} else if (arg_count(cmd, mirrors_ARG) || (lv->status & MIRRORED)) {
		if (!archive(lv->vg))
			return ECMD_FAILED;
		if (!lvconvert_mirrors(cmd, lv, lp))
			return ECMD_FAILED;
	}

	return ECMD_PROCESSED;
}

int lvconvert(struct cmd_context * cmd, int argc, char **argv)
{
	struct volume_group *vg;
	struct lv_list *lvl;
	struct lvconvert_params lp;
	int ret = ECMD_FAILED;
	struct lvinfo info;

	if (!_read_params(&lp, cmd, argc, argv)) {
		stack;
		return EINVALID_CMD_LINE;
	}

	log_verbose("Checking for existing volume group \"%s\"", lp.vg_name);

	if (!(vg = vg_lock_and_read(cmd, lp.vg_name, NULL, LCK_VG_WRITE,
				    CLUSTERED | EXPORTED_VG | LVM_WRITE,
				    CORRECT_INCONSISTENT)))
		return ECMD_FAILED;

	if (!(lvl = find_lv_in_vg(vg, lp.lv_name))) {
		log_error("Logical volume \"%s\" not found in "
			  "volume group \"%s\"", lp.lv_name, lp.vg_name);
		goto bad;
	}

	if (lp.pv_count) {
		if (!(lp.pvh = create_pv_list(cmd->mem, vg, lp.pv_count,
					      lp.pvs, 0)))
			goto_bad;
	} else
		lp.pvh = &vg->pvs;

	ret = lvconvert_single(cmd, lvl->lv, &lp);

bad:
	unlock_vg(cmd, lp.vg_name);

	if (ret == ECMD_PROCESSED && lp.need_polling) {
		if (!lv_info(cmd, lvl->lv, &info, 1, 0) || !info.exists) {
			log_print("Conversion starts after activation");
			return ret;
		}
		ret = lvconvert_poll(cmd, lp.lv_name_full,
				     lp.wait_completion ? 0 : 1U);
	}

	return ret;
}
