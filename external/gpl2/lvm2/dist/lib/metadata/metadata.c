/*	$NetBSD: metadata.c,v 1.1.1.3 2009/12/02 00:26:39 haad Exp $	*/

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

#include "lib.h"
#include "device.h"
#include "metadata.h"
#include "toolcontext.h"
#include "lvm-string.h"
#include "lvm-file.h"
#include "lvmcache.h"
#include "memlock.h"
#include "str_list.h"
#include "pv_alloc.h"
#include "segtype.h"
#include "activate.h"
#include "display.h"
#include "locking.h"
#include "archiver.h"
#include "defaults.h"
#include "filter-persistent.h"

#include <sys/param.h>

/*
 * FIXME: Check for valid handle before dereferencing field or log error?
 */
#define pv_field(handle, field)				\
	(((const struct physical_volume *)(handle))->field)

static struct physical_volume *_pv_read(struct cmd_context *cmd,
					struct dm_pool *pvmem,
					const char *pv_name,
					struct dm_list *mdas,
					uint64_t *label_sector,
					int warnings, int scan_label_only);

static struct physical_volume *_find_pv_by_name(struct cmd_context *cmd,
			 			const char *pv_name);

static struct pv_list *_find_pv_in_vg(const struct volume_group *vg,
				      const char *pv_name);

static struct physical_volume *_find_pv_in_vg_by_uuid(const struct volume_group *vg,
						      const struct id *id);

static uint32_t _vg_bad_status_bits(const struct volume_group *vg,
				    uint32_t status);

const char _really_init[] =
    "Really INITIALIZE physical volume \"%s\" of volume group \"%s\" [y/n]? ";

unsigned long set_pe_align(struct physical_volume *pv, unsigned long data_alignment)
{
	if (pv->pe_align)
		goto out;

	if (data_alignment)
		pv->pe_align = data_alignment;
	else
		pv->pe_align = MAX(65536UL, lvm_getpagesize()) >> SECTOR_SHIFT;

	if (!pv->dev)
		goto out;

	/*
	 * Align to stripe-width of underlying md device if present
	 */
	if (find_config_tree_bool(pv->fmt->cmd, "devices/md_chunk_alignment",
				  DEFAULT_MD_CHUNK_ALIGNMENT))
		pv->pe_align = MAX(pv->pe_align,
				   dev_md_stripe_width(pv->fmt->cmd->sysfs_dir,
						       pv->dev));

	/*
	 * Align to topology's minimum_io_size or optimal_io_size if present
	 * - minimum_io_size - the smallest request the device can perform
	 *   w/o incurring a read-modify-write penalty (e.g. MD's chunk size)
	 * - optimal_io_size - the device's preferred unit of receiving I/O
	 *   (e.g. MD's stripe width)
	 */
	if (find_config_tree_bool(pv->fmt->cmd,
				  "devices/data_alignment_detection",
				  DEFAULT_DATA_ALIGNMENT_DETECTION)) {
		pv->pe_align = MAX(pv->pe_align,
				   dev_minimum_io_size(pv->fmt->cmd->sysfs_dir,
						       pv->dev));

		pv->pe_align = MAX(pv->pe_align,
				   dev_optimal_io_size(pv->fmt->cmd->sysfs_dir,
						       pv->dev));
	}

	log_very_verbose("%s: Setting PE alignment to %lu sectors.",
			 dev_name(pv->dev), pv->pe_align);

out:
	return pv->pe_align;
}

unsigned long set_pe_align_offset(struct physical_volume *pv,
				  unsigned long data_alignment_offset)
{
	if (pv->pe_align_offset)
		goto out;

	if (data_alignment_offset)
		pv->pe_align_offset = data_alignment_offset;

	if (!pv->dev)
		goto out;

	if (find_config_tree_bool(pv->fmt->cmd,
				  "devices/data_alignment_offset_detection",
				  DEFAULT_DATA_ALIGNMENT_OFFSET_DETECTION))
		pv->pe_align_offset =
			MAX(pv->pe_align_offset,
			    dev_alignment_offset(pv->fmt->cmd->sysfs_dir,
						 pv->dev));

	log_very_verbose("%s: Setting PE alignment offset to %lu sectors.",
			 dev_name(pv->dev), pv->pe_align_offset);

out:
	return pv->pe_align_offset;
}

/**
 * add_pv_to_vg - Add a physical volume to a volume group
 * @vg - volume group to add to
 * @pv_name - name of the pv (to be removed)
 * @pv - physical volume to add to volume group
 *
 * Returns:
 *  0 - failure
 *  1 - success
 * FIXME: remove pv_name - obtain safely from pv
 */
int add_pv_to_vg(struct volume_group *vg, const char *pv_name,
		 struct physical_volume *pv)
{
	struct pv_list *pvl;
	struct format_instance *fid = vg->fid;
	struct dm_pool *mem = vg->vgmem;

	log_verbose("Adding physical volume '%s' to volume group '%s'",
		    pv_name, vg->name);

	if (!(pvl = dm_pool_zalloc(mem, sizeof(*pvl)))) {
		log_error("pv_list allocation for '%s' failed", pv_name);
		return 0;
	}

	if (!is_orphan_vg(pv->vg_name)) {
		log_error("Physical volume '%s' is already in volume group "
			  "'%s'", pv_name, pv->vg_name);
		return 0;
	}

	if (pv->fmt != fid->fmt) {
		log_error("Physical volume %s is of different format type (%s)",
			  pv_name, pv->fmt->name);
		return 0;
	}

	/* Ensure PV doesn't depend on another PV already in the VG */
	if (pv_uses_vg(pv, vg)) {
		log_error("Physical volume %s might be constructed from same "
			  "volume group %s", pv_name, vg->name);
		return 0;
	}

	if (!(pv->vg_name = dm_pool_strdup(mem, vg->name))) {
		log_error("vg->name allocation failed for '%s'", pv_name);
		return 0;
	}

	memcpy(&pv->vgid, &vg->id, sizeof(vg->id));

	/* Units of 512-byte sectors */
	pv->pe_size = vg->extent_size;

	/*
	 * pe_count must always be calculated by pv_setup
	 */
	pv->pe_alloc_count = 0;

	if (!fid->fmt->ops->pv_setup(fid->fmt, UINT64_C(0), 0,
				     vg->extent_size, 0, 0, 0UL, UINT64_C(0),
				     &fid->metadata_areas, pv, vg)) {
		log_error("Format-specific setup of physical volume '%s' "
			  "failed.", pv_name);
		return 0;
	}

	if (_find_pv_in_vg(vg, pv_name)) {
		log_error("Physical volume '%s' listed more than once.",
			  pv_name);
		return 0;
	}

	if (vg->pv_count && (vg->pv_count == vg->max_pv)) {
		log_error("No space for '%s' - volume group '%s' "
			  "holds max %d physical volume(s).", pv_name,
			  vg->name, vg->max_pv);
		return 0;
	}

	if (!alloc_pv_segment_whole_pv(mem, pv))
		return_0;

	pvl->pv = pv;
	dm_list_add(&vg->pvs, &pvl->list);

	if ((uint64_t) vg->extent_count + pv->pe_count > UINT32_MAX) {
		log_error("Unable to add %s to %s: new extent count (%"
			  PRIu64 ") exceeds limit (%" PRIu32 ").",
			  pv_name, vg->name,
			  (uint64_t) vg->extent_count + pv->pe_count,
			  UINT32_MAX);
		return 0;
	}

	vg->pv_count++;
	vg->extent_count += pv->pe_count;
	vg->free_count += pv->pe_count;

	return 1;
}

static int _copy_pv(struct dm_pool *pvmem,
		    struct physical_volume *pv_to,
		    struct physical_volume *pv_from)
{
	memcpy(pv_to, pv_from, sizeof(*pv_to));

	if (!(pv_to->vg_name = dm_pool_strdup(pvmem, pv_from->vg_name)))
		return_0;

	if (!str_list_dup(pvmem, &pv_to->tags, &pv_from->tags))
		return_0;

	if (!peg_dup(pvmem, &pv_to->segments, &pv_from->segments))
		return_0;

	return 1;
}

static struct pv_list *_copy_pvl(struct dm_pool *pvmem, struct pv_list *pvl_from)
{
	struct pv_list *pvl_to = NULL;

	if (!(pvl_to = dm_pool_zalloc(pvmem, sizeof(*pvl_to))))
		return_NULL;

	if (!(pvl_to->pv = dm_pool_alloc(pvmem, sizeof(*pvl_to->pv))))
		goto_bad;

	if(!_copy_pv(pvmem, pvl_to->pv, pvl_from->pv))
		goto_bad;

	return pvl_to;
bad:
	dm_pool_free(pvmem, pvl_to);
	return NULL;
}

int get_pv_from_vg_by_id(const struct format_type *fmt, const char *vg_name,
			 const char *vgid, const char *pvid,
			 struct physical_volume *pv)
{
	struct volume_group *vg;
	struct pv_list *pvl;
	int r = 0, consistent = 0;

	if (!(vg = vg_read_internal(fmt->cmd, vg_name, vgid, &consistent))) {
		log_error("get_pv_from_vg_by_id: vg_read_internal failed to read VG %s",
			  vg_name);
		return 0;
	}

	if (!consistent)
		log_warn("WARNING: Volume group %s is not consistent",
			 vg_name);

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (id_equal(&pvl->pv->id, (const struct id *) pvid)) {
			if (!_copy_pv(fmt->cmd->mem, pv, pvl->pv)) {
				log_error("internal PV duplication failed");
				r = 0;
				goto out;
			}
			r = 1;
			goto out;
		}
	}
out:
	vg_release(vg);
	return r;
}

int move_pv(struct volume_group *vg_from, struct volume_group *vg_to,
	    const char *pv_name)
{
	struct physical_volume *pv;
	struct pv_list *pvl;

	/* FIXME: handle tags */
	if (!(pvl = find_pv_in_vg(vg_from, pv_name))) {
		log_error("Physical volume %s not in volume group %s",
			  pv_name, vg_from->name);
		return 0;
	}

	if (_vg_bad_status_bits(vg_from, RESIZEABLE_VG) ||
	    _vg_bad_status_bits(vg_to, RESIZEABLE_VG))
		return 0;

	dm_list_move(&vg_to->pvs, &pvl->list);

	vg_from->pv_count--;
	vg_to->pv_count++;

	pv = pvl->pv;

	vg_from->extent_count -= pv_pe_count(pv);
	vg_to->extent_count += pv_pe_count(pv);

	vg_from->free_count -= pv_pe_count(pv) - pv_pe_alloc_count(pv);
	vg_to->free_count += pv_pe_count(pv) - pv_pe_alloc_count(pv);

	return 1;
}

int move_pvs_used_by_lv(struct volume_group *vg_from,
			struct volume_group *vg_to,
			const char *lv_name)
{
	struct lv_segment *lvseg;
	unsigned s;
	struct lv_list *lvl;
	struct logical_volume *lv;

	/* FIXME: handle tags */
	if (!(lvl = find_lv_in_vg(vg_from, lv_name))) {
		log_error("Logical volume %s not in volume group %s",
			  lv_name, vg_from->name);
		return 0;
	}

	if (_vg_bad_status_bits(vg_from, RESIZEABLE_VG) ||
	    _vg_bad_status_bits(vg_to, RESIZEABLE_VG))
		return 0;

	dm_list_iterate_items(lvseg, &lvl->lv->segments) {
		if (lvseg->log_lv)
			if (!move_pvs_used_by_lv(vg_from, vg_to,
						     lvseg->log_lv->name))
				return_0;
		for (s = 0; s < lvseg->area_count; s++) {
			if (seg_type(lvseg, s) == AREA_PV) {
				if (!move_pv(vg_from, vg_to,
					      pv_dev_name(seg_pv(lvseg, s))))
					return_0;
			} else if (seg_type(lvseg, s) == AREA_LV) {
				lv = seg_lv(lvseg, s);
				if (!move_pvs_used_by_lv(vg_from, vg_to,
							     lv->name))
				    return_0;
			}
		}
	}
	return 1;
}

static int validate_new_vg_name(struct cmd_context *cmd, const char *vg_name)
{
	char vg_path[PATH_MAX];

	if (!validate_name(vg_name))
		return_0;

	snprintf(vg_path, PATH_MAX, "%s%s", cmd->dev_dir, vg_name);
	if (path_exists(vg_path)) {
		log_error("%s: already exists in filesystem", vg_path);
		return 0;
	}

	return 1;
}

int validate_vg_rename_params(struct cmd_context *cmd,
			      const char *vg_name_old,
			      const char *vg_name_new)
{
	unsigned length;
	char *dev_dir;

	dev_dir = cmd->dev_dir;
	length = strlen(dev_dir);

	/* Check sanity of new name */
	if (strlen(vg_name_new) > NAME_LEN - length - 2) {
		log_error("New volume group path exceeds maximum length "
			  "of %d!", NAME_LEN - length - 2);
		return 0;
	}

	if (!validate_new_vg_name(cmd, vg_name_new)) {
		log_error("New volume group name \"%s\" is invalid",
			  vg_name_new);
		return 0;
	}

	if (!strcmp(vg_name_old, vg_name_new)) {
		log_error("Old and new volume group names must differ");
		return 0;
	}

	return 1;
}

int vg_rename(struct cmd_context *cmd, struct volume_group *vg,
	      const char *new_name)
{
	struct dm_pool *mem = vg->vgmem;
	struct pv_list *pvl;

	if (!(vg->name = dm_pool_strdup(mem, new_name))) {
		log_error("vg->name allocation failed for '%s'", new_name);
		return 0;
	}

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (!(pvl->pv->vg_name = dm_pool_strdup(mem, new_name))) {
			log_error("pv->vg_name allocation failed for '%s'",
				  pv_dev_name(pvl->pv));
			return 0;
		}
	}

	return 1;
}

int remove_lvs_in_vg(struct cmd_context *cmd,
		     struct volume_group *vg,
		     force_t force)
{
	struct dm_list *lst;
	struct lv_list *lvl;

	while ((lst = dm_list_first(&vg->lvs))) {
		lvl = dm_list_item(lst, struct lv_list);
		if (!lv_remove_with_dependencies(cmd, lvl->lv, force))
		    return 0;
	}

	return 1;
}

int vg_remove_check(struct volume_group *vg)
{
	unsigned lv_count;
	struct pv_list *pvl, *tpvl;

	if (vg_read_error(vg) || vg_missing_pv_count(vg)) {
		log_error("Volume group \"%s\" not found, is inconsistent "
			  "or has PVs missing.", vg ? vg->name : "");
		log_error("Consider vgreduce --removemissing if metadata "
			  "is inconsistent.");
		return 0;
	}

	if (!vg_check_status(vg, EXPORTED_VG))
		return 0;

	lv_count = vg_visible_lvs(vg);

	if (lv_count) {
		log_error("Volume group \"%s\" still contains %u "
			  "logical volume(s)", vg->name, lv_count);
		return 0;
	}

	if (!archive(vg))
		return 0;

	dm_list_iterate_items_safe(pvl, tpvl, &vg->pvs) {
		dm_list_del(&pvl->list);
		dm_list_add(&vg->removed_pvs, &pvl->list);
	}
	return 1;
}

int vg_remove(struct volume_group *vg)
{
	struct physical_volume *pv;
	struct pv_list *pvl;
	int ret = 1;

	if (!lock_vol(vg->cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		return 0;
	}

	if (!vg_remove_mdas(vg)) {
		log_error("vg_remove_mdas %s failed", vg->name);
		unlock_vg(vg->cmd, VG_ORPHANS);
		return 0;
	}

	/* init physical volumes */
	dm_list_iterate_items(pvl, &vg->removed_pvs) {
		pv = pvl->pv;
		log_verbose("Removing physical volume \"%s\" from "
			    "volume group \"%s\"", pv_dev_name(pv), vg->name);
		pv->vg_name = vg->fid->fmt->orphan_vg_name;
		pv->status = ALLOCATABLE_PV;

		if (!dev_get_size(pv_dev(pv), &pv->size)) {
			log_error("%s: Couldn't get size.", pv_dev_name(pv));
			ret = 0;
			continue;
		}

		/* FIXME Write to same sector label was read from */
		if (!pv_write(vg->cmd, pv, NULL, INT64_C(-1))) {
			log_error("Failed to remove physical volume \"%s\""
				  " from volume group \"%s\"",
				  pv_dev_name(pv), vg->name);
			ret = 0;
		}
	}

	backup_remove(vg->cmd, vg->name);

	if (ret)
		log_print("Volume group \"%s\" successfully removed", vg->name);
	else
		log_error("Volume group \"%s\" not properly removed", vg->name);

	unlock_vg(vg->cmd, VG_ORPHANS);
	return ret;
}

/*
 * Extend a VG by a single PV / device path
 *
 * Parameters:
 * - vg: handle of volume group to extend by 'pv_name'
 * - pv_name: device path of PV to add to VG
 * - pp: parameters to pass to implicit pvcreate; if NULL, do not pvcreate
 *
 */
static int vg_extend_single_pv(struct volume_group *vg, char *pv_name,
			       struct pvcreate_params *pp)
{
	struct physical_volume *pv;

	pv = pv_by_path(vg->fid->fmt->cmd, pv_name);
	if (!pv && !pp) {
		log_error("%s not identified as an existing "
			  "physical volume", pv_name);
		return 0;
	} else if (!pv && pp) {
		pv = pvcreate_single(vg->cmd, pv_name, pp);
		if (!pv)
			return 0;
	}
	if (!add_pv_to_vg(vg, pv_name, pv))
		return 0;
	return 1;
}

/*
 * Extend a VG by a single PV / device path
 *
 * Parameters:
 * - vg: handle of volume group to extend by 'pv_name'
 * - pv_count: count of device paths of PVs
 * - pv_names: device paths of PVs to add to VG
 * - pp: parameters to pass to implicit pvcreate; if NULL, do not pvcreate
 *
 */
int vg_extend(struct volume_group *vg, int pv_count, char **pv_names,
	      struct pvcreate_params *pp)
{
	int i;

	if (_vg_bad_status_bits(vg, RESIZEABLE_VG))
		return 0;

	/* attach each pv */
	for (i = 0; i < pv_count; i++) {
		if (!vg_extend_single_pv(vg, pv_names[i], pp))
			goto bad;
	}

/* FIXME Decide whether to initialise and add new mdahs to format instance */

	return 1;

      bad:
	log_error("Unable to add physical volume '%s' to "
		  "volume group '%s'.", pv_names[i], vg->name);
	return 0;
}

/* FIXME: use this inside vgreduce_single? */
int vg_reduce(struct volume_group *vg, char *pv_name)
{
	struct physical_volume *pv;
	struct pv_list *pvl;

	if (_vg_bad_status_bits(vg, RESIZEABLE_VG))
		return 0;

	if (!archive(vg))
		goto bad;

	/* remove each pv */
	if (!(pvl = find_pv_in_vg(vg, pv_name))) {
		log_error("Physical volume %s not in volume group %s.",
			  pv_name, vg->name);
		goto bad;
	}

	pv = pvl->pv;

	if (pv_pe_alloc_count(pv)) {
		log_error("Physical volume %s still in use.",
			  pv_name);
		goto bad;
	}

	if (!dev_get_size(pv_dev(pv), &pv->size)) {
		log_error("%s: Couldn't get size.", pv_name);
		goto bad;
	}

	vg->pv_count--;
	vg->free_count -= pv_pe_count(pv) - pv_pe_alloc_count(pv);
	vg->extent_count -= pv_pe_count(pv);

	/* add pv to the remove_pvs list */
	dm_list_del(&pvl->list);
	dm_list_add(&vg->removed_pvs, &pvl->list);

	return 1;

      bad:
	log_error("Unable to remove physical volume '%s' from "
		  "volume group '%s'.", pv_name, vg->name);
	return 0;
}

const char *strip_dir(const char *vg_name, const char *dev_dir)
{
	size_t len = strlen(dev_dir);
	if (!strncmp(vg_name, dev_dir, len))
		vg_name += len;

	return vg_name;
}

/*
 * Validate parameters to vg_create() before calling.
 * FIXME: Move inside vg_create library function.
 * FIXME: Change vgcreate_params struct to individual gets/sets
 */
int vgcreate_params_validate(struct cmd_context *cmd,
			     struct vgcreate_params *vp)
{
	if (!validate_new_vg_name(cmd, vp->vg_name)) {
		log_error("New volume group name \"%s\" is invalid",
			  vp->vg_name);
		return 1;
	}

	if (vp->alloc == ALLOC_INHERIT) {
		log_error("Volume Group allocation policy cannot inherit "
			  "from anything");
		return 1;
	}

	if (!vp->extent_size) {
		log_error("Physical extent size may not be zero");
		return 1;
	}

	if (!(cmd->fmt->features & FMT_UNLIMITED_VOLS)) {
		if (!vp->max_lv)
			vp->max_lv = 255;
		if (!vp->max_pv)
			vp->max_pv = 255;
		if (vp->max_lv > 255 || vp->max_pv > 255) {
			log_error("Number of volumes may not exceed 255");
			return 1;
		}
	}

	return 0;
}

/*
 * Create a (struct volume_group) volume group handle from a struct volume_group pointer and a
 * possible failure code or zero for success.
 */
static struct volume_group *_vg_make_handle(struct cmd_context *cmd,
			     struct volume_group *vg,
			     uint32_t failure)
{
	struct dm_pool *vgmem;

	if (!vg) {
		if (!(vgmem = dm_pool_create("lvm2 vg_handle", VG_MEMPOOL_CHUNK)) ||
		    !(vg = dm_pool_zalloc(vgmem, sizeof(*vg)))) {
			log_error("Error allocating vg handle.");
			if (vgmem)
				dm_pool_destroy(vgmem);
			return_NULL;
		}
		vg->vgmem = vgmem;
	}

	vg->read_status = failure;

	return (struct volume_group *)vg;
}

int lv_has_unknown_segments(const struct logical_volume *lv)
{
	struct lv_segment *seg;
	/* foreach segment */
	dm_list_iterate_items(seg, &lv->segments)
		if (seg_unknown(seg))
			return 1;
	return 0;
}

int vg_has_unknown_segments(const struct volume_group *vg)
{
	struct lv_list *lvl;

	/* foreach LV */
	dm_list_iterate_items(lvl, &vg->lvs)
		if (lv_has_unknown_segments(lvl->lv))
			return 1;
	return 0;
}

/*
 * Create a VG with default parameters.
 * Returns:
 * - struct volume_group* with SUCCESS code: VG structure created
 * - NULL or struct volume_group* with FAILED_* code: error creating VG structure
 * Use vg_read_error() to determine success or failure.
 * FIXME: cleanup usage of _vg_make_handle()
 */
struct volume_group *vg_create(struct cmd_context *cmd, const char *vg_name)
{
	struct volume_group *vg;
	int consistent = 0;
	struct dm_pool *mem;
	uint32_t rc;

	if (!validate_name(vg_name)) {
		log_error("Invalid vg name %s", vg_name);
		/* FIXME: use _vg_make_handle() w/proper error code */
		return NULL;
	}

	rc = vg_lock_newname(cmd, vg_name);
	if (rc != SUCCESS)
		/* NOTE: let caller decide - this may be check for existence */
		return _vg_make_handle(cmd, NULL, rc);

	/* FIXME: Is this vg_read_internal necessary? Move it inside
	   vg_lock_newname? */
	/* is this vg name already in use ? */
	if ((vg = vg_read_internal(cmd, vg_name, NULL, &consistent))) {
		log_error("A volume group called '%s' already exists.", vg_name);
		unlock_and_release_vg(cmd, vg, vg_name);
		return _vg_make_handle(cmd, NULL, FAILED_EXIST);
	}

	if (!(mem = dm_pool_create("lvm2 vg_create", VG_MEMPOOL_CHUNK)))
		goto_bad;

	if (!(vg = dm_pool_zalloc(mem, sizeof(*vg))))
		goto_bad;

	if (!id_create(&vg->id)) {
		log_error("Couldn't create uuid for volume group '%s'.",
			  vg_name);
		goto bad;
	}

	/* Strip dev_dir if present */
	vg_name = strip_dir(vg_name, cmd->dev_dir);

	vg->vgmem = mem;
	vg->cmd = cmd;

	if (!(vg->name = dm_pool_strdup(mem, vg_name)))
		goto_bad;

	vg->seqno = 0;

	vg->status = (RESIZEABLE_VG | LVM_READ | LVM_WRITE);
	if (!(vg->system_id = dm_pool_alloc(mem, NAME_LEN)))
		goto_bad;

	*vg->system_id = '\0';

	vg->extent_size = DEFAULT_EXTENT_SIZE * 2;
	vg->extent_count = 0;
	vg->free_count = 0;

	vg->max_lv = DEFAULT_MAX_LV;
	vg->max_pv = DEFAULT_MAX_PV;

	vg->alloc = DEFAULT_ALLOC_POLICY;

	vg->pv_count = 0;
	dm_list_init(&vg->pvs);

	dm_list_init(&vg->lvs);

	dm_list_init(&vg->tags);

	/* initialize removed_pvs list */
	dm_list_init(&vg->removed_pvs);

	if (!(vg->fid = cmd->fmt->ops->create_instance(cmd->fmt, vg_name,
						       NULL, NULL))) {
		log_error("Failed to create format instance");
		goto bad;
	}

	if (vg->fid->fmt->ops->vg_setup &&
	    !vg->fid->fmt->ops->vg_setup(vg->fid, vg)) {
		log_error("Format specific setup of volume group '%s' failed.",
			  vg_name);
		goto bad;
	}
	return _vg_make_handle(cmd, vg, SUCCESS);

bad:
	unlock_and_release_vg(cmd, vg, vg_name);
	/* FIXME: use _vg_make_handle() w/proper error code */
	return NULL;
}

uint64_t extents_from_size(struct cmd_context *cmd, uint64_t size,
			   uint32_t extent_size)
{
	if (size % extent_size) {
		size += extent_size - size % extent_size;
		log_print("Rounding up size to full physical extent %s",
			  display_size(cmd, size));
	}

	if (size > (uint64_t) UINT32_MAX * extent_size) {
		log_error("Volume too large (%s) for extent size %s. "
			  "Upper limit is %s.",
			  display_size(cmd, size),
			  display_size(cmd, (uint64_t) extent_size),
			  display_size(cmd, (uint64_t) UINT32_MAX *
				       extent_size));
		return 0;
	}

	return (uint64_t) size / extent_size;
}

static int _recalc_extents(uint32_t *extents, const char *desc1,
			   const char *desc2, uint32_t old_size,
			   uint32_t new_size)
{
	uint64_t size = (uint64_t) old_size * (*extents);

	if (size % new_size) {
		log_error("New size %" PRIu64 " for %s%s not an exact number "
			  "of new extents.", size, desc1, desc2);
		return 0;
	}

	size /= new_size;

	if (size > UINT32_MAX) {
		log_error("New extent count %" PRIu64 " for %s%s exceeds "
			  "32 bits.", size, desc1, desc2);
		return 0;
	}

	*extents = (uint32_t) size;

	return 1;
}

int vg_set_extent_size(struct volume_group *vg, uint32_t new_size)
{
	uint32_t old_size = vg->extent_size;
	struct pv_list *pvl;
	struct lv_list *lvl;
	struct physical_volume *pv;
	struct logical_volume *lv;
	struct lv_segment *seg;
	struct pv_segment *pvseg;
	uint32_t s;

	if (!vg_is_resizeable(vg)) {
		log_error("Volume group \"%s\" must be resizeable "
			  "to change PE size", vg->name);
		return 0;
	}

	if (!new_size) {
		log_error("Physical extent size may not be zero");
		return 0;
	}

	if (new_size == vg->extent_size)
		return 1;

	if (new_size & (new_size - 1)) {
		log_error("Physical extent size must be a power of 2.");
		return 0;
	}

	if (new_size > vg->extent_size) {
		if ((uint64_t) vg_size(vg) % new_size) {
			/* FIXME Adjust used PV sizes instead */
			log_error("New extent size is not a perfect fit");
			return 0;
		}
	}

	vg->extent_size = new_size;

	if (vg->fid->fmt->ops->vg_setup &&
	    !vg->fid->fmt->ops->vg_setup(vg->fid, vg))
		return_0;

	if (!_recalc_extents(&vg->extent_count, vg->name, "", old_size,
			     new_size))
		return_0;

	if (!_recalc_extents(&vg->free_count, vg->name, " free space",
			     old_size, new_size))
		return_0;

	/* foreach PV */
	dm_list_iterate_items(pvl, &vg->pvs) {
		pv = pvl->pv;

		pv->pe_size = new_size;
		if (!_recalc_extents(&pv->pe_count, pv_dev_name(pv), "",
				     old_size, new_size))
			return_0;

		if (!_recalc_extents(&pv->pe_alloc_count, pv_dev_name(pv),
				     " allocated space", old_size, new_size))
			return_0;

		/* foreach free PV Segment */
		dm_list_iterate_items(pvseg, &pv->segments) {
			if (pvseg_is_allocated(pvseg))
				continue;

			if (!_recalc_extents(&pvseg->pe, pv_dev_name(pv),
					     " PV segment start", old_size,
					     new_size))
				return_0;
			if (!_recalc_extents(&pvseg->len, pv_dev_name(pv),
					     " PV segment length", old_size,
					     new_size))
				return_0;
		}
	}

	/* foreach LV */
	dm_list_iterate_items(lvl, &vg->lvs) {
		lv = lvl->lv;

		if (!_recalc_extents(&lv->le_count, lv->name, "", old_size,
				     new_size))
			return_0;

		dm_list_iterate_items(seg, &lv->segments) {
			if (!_recalc_extents(&seg->le, lv->name,
					     " segment start", old_size,
					     new_size))
				return_0;

			if (!_recalc_extents(&seg->len, lv->name,
					     " segment length", old_size,
					     new_size))
				return_0;

			if (!_recalc_extents(&seg->area_len, lv->name,
					     " area length", old_size,
					     new_size))
				return_0;

			if (!_recalc_extents(&seg->extents_copied, lv->name,
					     " extents moved", old_size,
					     new_size))
				return_0;

			/* foreach area */
			for (s = 0; s < seg->area_count; s++) {
				switch (seg_type(seg, s)) {
				case AREA_PV:
					if (!_recalc_extents
					    (&seg_pe(seg, s),
					     lv->name,
					     " pvseg start", old_size,
					     new_size))
						return_0;
					if (!_recalc_extents
					    (&seg_pvseg(seg, s)->len,
					     lv->name,
					     " pvseg length", old_size,
					     new_size))
						return_0;
					break;
				case AREA_LV:
					if (!_recalc_extents
					    (&seg_le(seg, s), lv->name,
					     " area start", old_size,
					     new_size))
						return_0;
					break;
				case AREA_UNASSIGNED:
					log_error("Unassigned area %u found in "
						  "segment", s);
					return 0;
				}
			}
		}

	}

	return 1;
}

int vg_set_max_lv(struct volume_group *vg, uint32_t max_lv)
{
	if (!vg_is_resizeable(vg)) {
		log_error("Volume group \"%s\" must be resizeable "
			  "to change MaxLogicalVolume", vg->name);
		return 0;
	}

	if (!(vg->fid->fmt->features & FMT_UNLIMITED_VOLS)) {
		if (!max_lv)
			max_lv = 255;
		else if (max_lv > 255) {
			log_error("MaxLogicalVolume limit is 255");
			return 0;
		}
	}

	if (max_lv && max_lv < vg_visible_lvs(vg)) {
		log_error("MaxLogicalVolume is less than the current number "
			  "%d of LVs for %s", vg_visible_lvs(vg),
			  vg->name);
		return 0;
	}
	vg->max_lv = max_lv;

	return 1;
}

int vg_set_max_pv(struct volume_group *vg, uint32_t max_pv)
{
	if (!vg_is_resizeable(vg)) {
		log_error("Volume group \"%s\" must be resizeable "
			  "to change MaxPhysicalVolumes", vg->name);
		return 0;
	}

	if (!(vg->fid->fmt->features & FMT_UNLIMITED_VOLS)) {
		if (!max_pv)
			max_pv = 255;
		else if (max_pv > 255) {
			log_error("MaxPhysicalVolume limit is 255");
			return 0;
		}
	}

	if (max_pv && max_pv < vg->pv_count) {
		log_error("MaxPhysicalVolumes is less than the current number "
			  "%d of PVs for \"%s\"", vg->pv_count,
			  vg->name);
		return 0;
	}
	vg->max_pv = max_pv;
	return 1;
}

int vg_set_alloc_policy(struct volume_group *vg, alloc_policy_t alloc)
{
	if (alloc == ALLOC_INHERIT) {
		log_error("Volume Group allocation policy cannot inherit "
			  "from anything");
		return 0;
	}

	if (alloc == vg->alloc)
		return 1;

	vg->alloc = alloc;
	return 1;
}

int vg_set_clustered(struct volume_group *vg, int clustered)
{
	struct lv_list *lvl;
	if (clustered) {
		dm_list_iterate_items(lvl, &vg->lvs) {
			if (lv_is_origin(lvl->lv) || lv_is_cow(lvl->lv)) {
				log_error("Volume group %s contains snapshots "
					  "that are not yet supported.",
					  vg->name);
				return 0;
			}
		}
	}

	if (clustered)
		vg->status |= CLUSTERED;
	else
		vg->status &= ~CLUSTERED;
	return 1;
}

/*
 * Separate metadata areas after splitting a VG.
 * Also accepts orphan VG as destination (for vgreduce).
 */
int vg_split_mdas(struct cmd_context *cmd __attribute((unused)),
		  struct volume_group *vg_from, struct volume_group *vg_to)
{
	struct metadata_area *mda, *mda2;
	struct dm_list *mdas_from, *mdas_to;
	int common_mda = 0;

	mdas_from = &vg_from->fid->metadata_areas;
	mdas_to = &vg_to->fid->metadata_areas;

	dm_list_iterate_items_safe(mda, mda2, mdas_from) {
		if (!mda->ops->mda_in_vg) {
			common_mda = 1;
			continue;
		}

		if (!mda->ops->mda_in_vg(vg_from->fid, vg_from, mda)) {
			if (is_orphan_vg(vg_to->name))
				dm_list_del(&mda->list);
			else
				dm_list_move(mdas_to, &mda->list);
		}
	}

	if (dm_list_empty(mdas_from) ||
	    (!is_orphan_vg(vg_to->name) && dm_list_empty(mdas_to)))
		return common_mda;

	return 1;
}

/*
 * See if we may pvcreate on this device.
 * 0 indicates we may not.
 */
static int pvcreate_check(struct cmd_context *cmd, const char *name,
			  struct pvcreate_params *pp)
{
	struct physical_volume *pv;
	struct device *dev;
	uint64_t md_superblock, swap_signature;
	int wipe_md, wipe_swap;

	/* FIXME Check partition type is LVM unless --force is given */

	/* Is there a pv here already? */
	pv = pv_read(cmd, name, NULL, NULL, 0, 0);

	/*
	 * If a PV has no MDAs it may appear to be an orphan until the
	 * metadata is read off another PV in the same VG.  Detecting
	 * this means checking every VG by scanning every PV on the
	 * system.
	 */
	if (pv && is_orphan(pv)) {
		if (!scan_vgs_for_pvs(cmd))
			return_0;
		pv = pv_read(cmd, name, NULL, NULL, 0, 0);
	}

	/* Allow partial & exported VGs to be destroyed. */
	/* We must have -ff to overwrite a non orphan */
	if (pv && !is_orphan(pv) && pp->force != DONT_PROMPT_OVERRIDE) {
		log_error("Can't initialize physical volume \"%s\" of "
			  "volume group \"%s\" without -ff", name, pv_vg_name(pv));
		return 0;
	}

	/* prompt */
	if (pv && !is_orphan(pv) && !pp->yes &&
	    yes_no_prompt(_really_init, name, pv_vg_name(pv)) == 'n') {
		log_print("%s: physical volume not initialized", name);
		return 0;
	}

	if (sigint_caught())
		return 0;

	dev = dev_cache_get(name, cmd->filter);

	/* Is there an md superblock here? */
	if (!dev && md_filtering()) {
		unlock_vg(cmd, VG_ORPHANS);

		persistent_filter_wipe(cmd->filter);
		lvmcache_destroy(cmd, 1);

		init_md_filtering(0);
		if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
			log_error("Can't get lock for orphan PVs");
			init_md_filtering(1);
			return 0;
		}
		dev = dev_cache_get(name, cmd->filter);
		init_md_filtering(1);
	}

	if (!dev) {
		log_error("Device %s not found (or ignored by filtering).", name);
		return 0;
	}

	/*
	 * This test will fail if the device belongs to an MD array.
	 */
	if (!dev_test_excl(dev)) {
		/* FIXME Detect whether device-mapper itself is still using it */
		log_error("Can't open %s exclusively.  Mounted filesystem?",
			  name);
		return 0;
	}

	/* Wipe superblock? */
	if ((wipe_md = dev_is_md(dev, &md_superblock)) == 1 &&
	    ((!pp->idp && !pp->restorefile) || pp->yes ||
	     (yes_no_prompt("Software RAID md superblock "
			    "detected on %s. Wipe it? [y/n] ", name) == 'y'))) {
		log_print("Wiping software RAID md superblock on %s", name);
		if (!dev_set(dev, md_superblock, 4, 0)) {
			log_error("Failed to wipe RAID md superblock on %s",
				  name);
			return 0;
		}
	}

	if (wipe_md == -1) {
		log_error("Fatal error while trying to detect software "
			  "RAID md superblock on %s", name);
		return 0;
	}

	if ((wipe_swap = dev_is_swap(dev, &swap_signature)) == 1 &&
	    ((!pp->idp && !pp->restorefile) || pp->yes ||
	     (yes_no_prompt("Swap signature detected on %s. Wipe it? [y/n] ",
			    name) == 'y'))) {
		log_print("Wiping swap signature on %s", name);
		if (!dev_set(dev, swap_signature, 10, 0)) {
			log_error("Failed to wipe swap signature on %s", name);
			return 0;
		}
	}

	if (wipe_swap == -1) {
		log_error("Fatal error while trying to detect swap "
			  "signature on %s", name);
		return 0;
	}

	if (sigint_caught())
		return 0;

	if (pv && !is_orphan(pv) && pp->force) {
		log_warn("WARNING: Forcing physical volume creation on "
			  "%s%s%s%s", name,
			  !is_orphan(pv) ? " of volume group \"" : "",
			  !is_orphan(pv) ? pv_vg_name(pv) : "",
			  !is_orphan(pv) ? "\"" : "");
	}

	return 1;
}

void pvcreate_params_set_defaults(struct pvcreate_params *pp)
{
	memset(pp, 0, sizeof(*pp));
	pp->zero = 1;
	pp->size = 0;
	pp->data_alignment = UINT64_C(0);
	pp->data_alignment_offset = UINT64_C(0);
	pp->pvmetadatacopies = DEFAULT_PVMETADATACOPIES;
	pp->pvmetadatasize = DEFAULT_PVMETADATASIZE;
	pp->labelsector = DEFAULT_LABELSECTOR;
	pp->idp = 0;
	pp->pe_start = 0;
	pp->extent_count = 0;
	pp->extent_size = 0;
	pp->restorefile = 0;
	pp->force = PROMPT;
	pp->yes = 0;
}

/*
 * pvcreate_single() - initialize a device with PV label and metadata area
 *
 * Parameters:
 * - pv_name: device path to initialize
 * - pp: parameters to pass to pv_create; if NULL, use default values
 *
 * Returns:
 * NULL: error
 * struct physical_volume * (non-NULL): handle to physical volume created
 */
struct physical_volume * pvcreate_single(struct cmd_context *cmd,
					 const char *pv_name,
					 struct pvcreate_params *pp)
{
	void *pv;
	struct device *dev;
	struct dm_list mdas;
	struct pvcreate_params default_pp;
	char buffer[64] __attribute((aligned(8)));

	pvcreate_params_set_defaults(&default_pp);
	if (!pp)
		pp = &default_pp;

	if (pp->idp) {
		if ((dev = device_from_pvid(cmd, pp->idp)) &&
		    (dev != dev_cache_get(pv_name, cmd->filter))) {
			if (!id_write_format((const struct id*)&pp->idp->uuid,
			    buffer, sizeof(buffer)))
				return_NULL;
			log_error("uuid %s already in use on \"%s\"", buffer,
				  dev_name(dev));
			return NULL;
		}
	}

	if (!pvcreate_check(cmd, pv_name, pp))
		goto error;

	if (sigint_caught())
		goto error;

	if (!(dev = dev_cache_get(pv_name, cmd->filter))) {
		log_error("%s: Couldn't find device.  Check your filters?",
			  pv_name);
		goto error;
	}

	dm_list_init(&mdas);
	if (!(pv = pv_create(cmd, dev, pp->idp, pp->size,
			     pp->data_alignment, pp->data_alignment_offset,
			     pp->pe_start, pp->extent_count, pp->extent_size,
			     pp->pvmetadatacopies,
			     pp->pvmetadatasize,&mdas))) {
		log_error("Failed to setup physical volume \"%s\"", pv_name);
		goto error;
	}

	log_verbose("Set up physical volume for \"%s\" with %" PRIu64
		    " available sectors", pv_name, pv_size(pv));

	/* Wipe existing label first */
	if (!label_remove(pv_dev(pv))) {
		log_error("Failed to wipe existing label on %s", pv_name);
		goto error;
	}

	if (pp->zero) {
		log_verbose("Zeroing start of device %s", pv_name);
		if (!dev_open_quiet(dev)) {
			log_error("%s not opened: device not zeroed", pv_name);
			goto error;
		}

		if (!dev_set(dev, UINT64_C(0), (size_t) 2048, 0)) {
			log_error("%s not wiped: aborting", pv_name);
			dev_close(dev);
			goto error;
		}
		dev_close(dev);
	}

	log_very_verbose("Writing physical volume data to disk \"%s\"",
			 pv_name);
	if (!(pv_write(cmd, (struct physical_volume *)pv, &mdas,
		       pp->labelsector))) {
		log_error("Failed to write physical volume \"%s\"", pv_name);
		goto error;
	}

	log_print("Physical volume \"%s\" successfully created", pv_name);

	return pv;

      error:
	return NULL;
}

static void _free_pv(struct dm_pool *mem, struct physical_volume *pv)
{
	dm_pool_free(mem, pv);
}

static struct physical_volume *_alloc_pv(struct dm_pool *mem, struct device *dev)
{
	struct physical_volume *pv = dm_pool_zalloc(mem, sizeof(*pv));

	if (!pv)
		return_NULL;

	if (!(pv->vg_name = dm_pool_zalloc(mem, NAME_LEN))) {
		dm_pool_free(mem, pv);
		return NULL;
	}

	pv->pe_size = 0;
	pv->pe_start = 0;
	pv->pe_count = 0;
	pv->pe_alloc_count = 0;
	pv->pe_align = 0;
	pv->pe_align_offset = 0;
	pv->fmt = NULL;
	pv->dev = dev;

	pv->status = ALLOCATABLE_PV;

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	return pv;
}

/**
 * pv_create - initialize a physical volume for use with a volume group
 *
 * @fmt: format type
 * @dev: PV device to initialize
 * @size: size of the PV in sectors
 * @data_alignment: requested alignment of data
 * @data_alignment_offset: requested offset to aligned data
 * @pe_start: physical extent start
 * @existing_extent_count
 * @existing_extent_size
 * @pvmetadatacopies
 * @pvmetadatasize
 * @mdas
 *
 * Returns:
 *   PV handle - physical volume initialized successfully
 *   NULL - invalid parameter or problem initializing the physical volume
 *
 * Note:
 *   FIXME: shorten argument list and replace with explict 'set' functions
 */
struct physical_volume *pv_create(const struct cmd_context *cmd,
				  struct device *dev,
				  struct id *id, uint64_t size,
				  unsigned long data_alignment,
				  unsigned long data_alignment_offset,
				  uint64_t pe_start,
				  uint32_t existing_extent_count,
				  uint32_t existing_extent_size,
				  int pvmetadatacopies,
				  uint64_t pvmetadatasize, struct dm_list *mdas)
{
	const struct format_type *fmt = cmd->fmt;
	struct dm_pool *mem = fmt->cmd->mem;
	struct physical_volume *pv = _alloc_pv(mem, dev);

	if (!pv)
		return NULL;

	if (id)
		memcpy(&pv->id, id, sizeof(*id));
	else if (!id_create(&pv->id)) {
		log_error("Failed to create random uuid for %s.",
			  dev_name(dev));
		goto bad;
	}

	if (!dev_get_size(pv->dev, &pv->size)) {
		log_error("%s: Couldn't get size.", pv_dev_name(pv));
		goto bad;
	}

	if (size) {
		if (size > pv->size)
			log_warn("WARNING: %s: Overriding real size. "
				  "You could lose data.", pv_dev_name(pv));
		log_verbose("%s: Pretending size is %" PRIu64 " sectors.",
			    pv_dev_name(pv), size);
		pv->size = size;
	}

	if (pv->size < PV_MIN_SIZE) {
		log_error("%s: Size must exceed minimum of %ld sectors.",
			  pv_dev_name(pv), PV_MIN_SIZE);
		goto bad;
	}

	if (pv->size < data_alignment) {
		log_error("%s: Data alignment must not exceed device size.",
			  pv_dev_name(pv));
		goto bad;
	}

	pv->fmt = fmt;
	pv->vg_name = fmt->orphan_vg_name;

	if (!fmt->ops->pv_setup(fmt, pe_start, existing_extent_count,
				existing_extent_size, data_alignment,
				data_alignment_offset,
				pvmetadatacopies, pvmetadatasize, mdas,
				pv, NULL)) {
		log_error("%s: Format-specific setup of physical volume "
			  "failed.", pv_dev_name(pv));
		goto bad;
	}

	return pv;

      bad:
	_free_pv(mem, pv);
	return NULL;
}

/* FIXME: liblvm todo - make into function that returns handle */
struct pv_list *find_pv_in_vg(const struct volume_group *vg,
			      const char *pv_name)
{
	return _find_pv_in_vg(vg, pv_name);
}

static struct pv_list *_find_pv_in_vg(const struct volume_group *vg,
				      const char *pv_name)
{
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs)
		if (pvl->pv->dev == dev_cache_get(pv_name, vg->cmd->filter))
			return pvl;

	return NULL;
}

struct pv_list *find_pv_in_pv_list(const struct dm_list *pl,
				   const struct physical_volume *pv)
{
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, pl)
		if (pvl->pv == pv)
			return pvl;

	return NULL;
}

int pv_is_in_vg(struct volume_group *vg, struct physical_volume *pv)
{
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs)
		if (pv == pvl->pv)
			 return 1;

	return 0;
}

/**
 * find_pv_in_vg_by_uuid - Find PV in VG by PV UUID
 * @vg: volume group to search
 * @id: UUID of the PV to match
 *
 * Returns:
 *   PV handle - if UUID of PV found in VG
 *   NULL - invalid parameter or UUID of PV not found in VG
 *
 * Note
 *   FIXME - liblvm todo - make into function that takes VG handle
 */
struct physical_volume *find_pv_in_vg_by_uuid(const struct volume_group *vg,
			    const struct id *id)
{
	return _find_pv_in_vg_by_uuid(vg, id);
}


static struct physical_volume *_find_pv_in_vg_by_uuid(const struct volume_group *vg,
						      const struct id *id)
{
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs)
		if (id_equal(&pvl->pv->id, id))
			return pvl->pv;

	return NULL;
}

struct lv_list *find_lv_in_vg(const struct volume_group *vg,
			      const char *lv_name)
{
	struct lv_list *lvl;
	const char *ptr;

	/* Use last component */
	if ((ptr = strrchr(lv_name, '/')))
		ptr++;
	else
		ptr = lv_name;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (!strcmp(lvl->lv->name, ptr))
			return lvl;

	return NULL;
}

struct lv_list *find_lv_in_lv_list(const struct dm_list *ll,
				   const struct logical_volume *lv)
{
	struct lv_list *lvl;

	dm_list_iterate_items(lvl, ll)
		if (lvl->lv == lv)
			return lvl;

	return NULL;
}

struct lv_list *find_lv_in_vg_by_lvid(struct volume_group *vg,
				      const union lvid *lvid)
{
	struct lv_list *lvl;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (!strncmp(lvl->lv->lvid.s, lvid->s, sizeof(*lvid)))
			return lvl;

	return NULL;
}

struct logical_volume *find_lv(const struct volume_group *vg,
			       const char *lv_name)
{
	struct lv_list *lvl = find_lv_in_vg(vg, lv_name);
	return lvl ? lvl->lv : NULL;
}

struct physical_volume *find_pv(struct volume_group *vg, struct device *dev)
{
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs)
		if (dev == pvl->pv->dev)
			return pvl->pv;

	return NULL;
}

/* FIXME: liblvm todo - make into function that returns handle */
struct physical_volume *find_pv_by_name(struct cmd_context *cmd,
					const char *pv_name)
{
	return _find_pv_by_name(cmd, pv_name);
}


static struct physical_volume *_find_pv_by_name(struct cmd_context *cmd,
			 			const char *pv_name)
{
	struct physical_volume *pv;

	if (!(pv = _pv_read(cmd, cmd->mem, pv_name, NULL, NULL, 1, 0))) {
		log_error("Physical volume %s not found", pv_name);
		return NULL;
	}

	if (is_orphan_vg(pv->vg_name)) {
		/* If a PV has no MDAs - need to search all VGs for it */
		if (!scan_vgs_for_pvs(cmd))
			return_NULL;
		if (!(pv = _pv_read(cmd, cmd->mem, pv_name, NULL, NULL, 1, 0))) {
			log_error("Physical volume %s not found", pv_name);
			return NULL;
		}
	}

	if (is_orphan_vg(pv->vg_name)) {
		log_error("Physical volume %s not in a volume group", pv_name);
		return NULL;
	}

	return pv;
}

/* Find segment at a given logical extent in an LV */
struct lv_segment *find_seg_by_le(const struct logical_volume *lv, uint32_t le)
{
	struct lv_segment *seg;

	dm_list_iterate_items(seg, &lv->segments)
		if (le >= seg->le && le < seg->le + seg->len)
			return seg;

	return NULL;
}

struct lv_segment *first_seg(const struct logical_volume *lv)
{
	struct lv_segment *seg;

	dm_list_iterate_items(seg, &lv->segments)
		return seg;

	return NULL;
}

/* Find segment at a given physical extent in a PV */
struct pv_segment *find_peg_by_pe(const struct physical_volume *pv, uint32_t pe)
{
	struct pv_segment *peg;

	dm_list_iterate_items(peg, &pv->segments)
		if (pe >= peg->pe && pe < peg->pe + peg->len)
			return peg;

	return NULL;
}

int vg_remove_mdas(struct volume_group *vg)
{
	struct metadata_area *mda;

	/* FIXME Improve recovery situation? */
	/* Remove each copy of the metadata */
	dm_list_iterate_items(mda, &vg->fid->metadata_areas) {
		if (mda->ops->vg_remove &&
		    !mda->ops->vg_remove(vg->fid, vg, mda))
			return_0;
	}

	return 1;
}

unsigned snapshot_count(const struct volume_group *vg)
{
	struct lv_list *lvl;
	unsigned num_snapshots = 0;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (lv_is_cow(lvl->lv))
			num_snapshots++;

	return num_snapshots;
}

unsigned vg_visible_lvs(const struct volume_group *vg)
{
	struct lv_list *lvl;
	unsigned lv_count = 0;

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (lv_is_visible(lvl->lv))
			lv_count++;
	}

	return lv_count;
}

/*
 * Determine whether two vgs are compatible for merging.
 */
int vgs_are_compatible(struct cmd_context *cmd __attribute((unused)),
		       struct volume_group *vg_from,
		       struct volume_group *vg_to)
{
	struct lv_list *lvl1, *lvl2;
	struct pv_list *pvl;
	char *name1, *name2;

	if (lvs_in_vg_activated(vg_from)) {
		log_error("Logical volumes in \"%s\" must be inactive",
			  vg_from->name);
		return 0;
	}

	/* Check compatibility */
	if (vg_to->extent_size != vg_from->extent_size) {
		log_error("Extent sizes differ: %d (%s) and %d (%s)",
			  vg_to->extent_size, vg_to->name,
			  vg_from->extent_size, vg_from->name);
		return 0;
	}

	if (vg_to->max_pv &&
	    (vg_to->max_pv < vg_to->pv_count + vg_from->pv_count)) {
		log_error("Maximum number of physical volumes (%d) exceeded "
			  " for \"%s\" and \"%s\"", vg_to->max_pv, vg_to->name,
			  vg_from->name);
		return 0;
	}

	if (vg_to->max_lv &&
	    (vg_to->max_lv < vg_visible_lvs(vg_to) + vg_visible_lvs(vg_from))) {
		log_error("Maximum number of logical volumes (%d) exceeded "
			  " for \"%s\" and \"%s\"", vg_to->max_lv, vg_to->name,
			  vg_from->name);
		return 0;
	}

	/* Metadata types must be the same */
	if (vg_to->fid->fmt != vg_from->fid->fmt) {
		log_error("Metadata types differ for \"%s\" and \"%s\"",
			  vg_to->name, vg_from->name);
		return 0;
	}

	/* Clustering attribute must be the same */
	if (vg_is_clustered(vg_to) != vg_is_clustered(vg_from)) {
		log_error("Clustered attribute differs for \"%s\" and \"%s\"",
			  vg_to->name, vg_from->name);
		return 0;
	}

	/* Check no conflicts with LV names */
	dm_list_iterate_items(lvl1, &vg_to->lvs) {
		name1 = lvl1->lv->name;

		dm_list_iterate_items(lvl2, &vg_from->lvs) {
			name2 = lvl2->lv->name;

			if (!strcmp(name1, name2)) {
				log_error("Duplicate logical volume "
					  "name \"%s\" "
					  "in \"%s\" and \"%s\"",
					  name1, vg_to->name, vg_from->name);
				return 0;
			}
		}
	}

	/* Check no PVs are constructed from either VG */
	dm_list_iterate_items(pvl, &vg_to->pvs) {
		if (pv_uses_vg(pvl->pv, vg_from)) {
			log_error("Physical volume %s might be constructed "
				  "from same volume group %s.",
				  pv_dev_name(pvl->pv), vg_from->name);
			return 0;
		}
	}

	dm_list_iterate_items(pvl, &vg_from->pvs) {
		if (pv_uses_vg(pvl->pv, vg_to)) {
			log_error("Physical volume %s might be constructed "
				  "from same volume group %s.",
				  pv_dev_name(pvl->pv), vg_to->name);
			return 0;
		}
	}

	return 1;
}

struct _lv_postorder_baton {
	int (*fn)(struct logical_volume *lv, void *data);
	void *data;
};

static int _lv_postorder_visit(struct logical_volume *,
			       int (*fn)(struct logical_volume *lv, void *data),
			       void *data);

static int _lv_postorder_level(struct logical_volume *lv, void *data)
{
	struct _lv_postorder_baton *baton = data;
	if (lv->status & POSTORDER_OPEN_FLAG)
		return 1; // a data structure loop has closed...
	lv->status |= POSTORDER_OPEN_FLAG;
	int r =_lv_postorder_visit(lv, baton->fn, baton->data);
	lv->status &= ~POSTORDER_OPEN_FLAG;
	lv->status |= POSTORDER_FLAG;
	return r;
};

static int _lv_each_dependency(struct logical_volume *lv,
			       int (*fn)(struct logical_volume *lv, void *data),
			       void *data)
{
	int i, s;
	struct lv_segment *lvseg;

	struct logical_volume *deps[] = {
		lv->snapshot ? lv->snapshot->origin : 0,
		lv->snapshot ? lv->snapshot->cow : 0 };
	for (i = 0; i < sizeof(deps) / sizeof(*deps); ++i) {
		if (deps[i] && !fn(deps[i], data))
			return_0;
	}

	dm_list_iterate_items(lvseg, &lv->segments) {
		if (lvseg->log_lv && !fn(lvseg->log_lv, data))
			return_0;
		for (s = 0; s < lvseg->area_count; ++s) {
			if (seg_type(lvseg, s) == AREA_LV && !fn(seg_lv(lvseg,s), data))
				return_0;
		}
	}
	return 1;
}

static int _lv_postorder_cleanup(struct logical_volume *lv, void *data)
{
	if (!(lv->status & POSTORDER_FLAG))
		return 1;
	lv->status &= ~POSTORDER_FLAG;

	if (!_lv_each_dependency(lv, _lv_postorder_cleanup, data))
		return_0;
	return 1;
}

static int _lv_postorder_visit(struct logical_volume *lv,
			       int (*fn)(struct logical_volume *lv, void *data),
			       void *data)
{
	struct _lv_postorder_baton baton;
	int r;

	if (lv->status & POSTORDER_FLAG)
		return 1;

	baton.fn = fn;
	baton.data = data;
	r = _lv_each_dependency(lv, _lv_postorder_level, &baton);
	if (r)
		r = fn(lv, data);

	return r;
}

/*
 * This will walk the LV dependency graph in depth-first order and in the
 * postorder, call a callback function "fn". The void *data is passed along all
 * the calls. The callback may return zero to indicate an error and terminate
 * the depth-first walk. The error is propagated to return value of
 * _lv_postorder.
 */
static int _lv_postorder(struct logical_volume *lv,
			       int (*fn)(struct logical_volume *lv, void *data),
			       void *data)
{
	int r;
	r = _lv_postorder_visit(lv, fn, data);
	_lv_postorder_cleanup(lv, 0);
	return r;
}

struct _lv_mark_if_partial_baton {
	int partial;
};

static int _lv_mark_if_partial_collect(struct logical_volume *lv, void *data)
{
	struct _lv_mark_if_partial_baton *baton = data;
	if (lv->status & PARTIAL_LV)
		baton->partial = 1;

	return 1;
}

static int _lv_mark_if_partial_single(struct logical_volume *lv, void *data)
{
	int s;
	struct _lv_mark_if_partial_baton baton;
	struct lv_segment *lvseg;

	dm_list_iterate_items(lvseg, &lv->segments) {
		for (s = 0; s < lvseg->area_count; ++s) {
			if (seg_type(lvseg, s) == AREA_PV) {
				if (seg_pv(lvseg, s)->status & MISSING_PV)
					lv->status |= PARTIAL_LV;
			}
		}
	}

	baton.partial = 0;
	_lv_each_dependency(lv, _lv_mark_if_partial_collect, &baton);

	if (baton.partial)
		lv->status |= PARTIAL_LV;

	return 1;
}

static int _lv_mark_if_partial(struct logical_volume *lv)
{
	return _lv_postorder(lv, _lv_mark_if_partial_single, NULL);
}

/*
 * Mark LVs with missing PVs using PARTIAL_LV status flag. The flag is
 * propagated transitively, so LVs referencing other LVs are marked
 * partial as well, if any of their referenced LVs are marked partial.
 */
static int _vg_mark_partial_lvs(struct volume_group *vg)
{
	struct logical_volume *lv;
	struct lv_list *lvl;

	dm_list_iterate_items(lvl, &vg->lvs) {
		lv = lvl->lv;
		if (!_lv_mark_if_partial(lv))
			return_0;
	}
	return 1;
}

/*
 * Be sure that all PV devices have cached read ahead in dev-cache
 * Currently it takes read_ahead from first PV segment only
 */
static int _lv_read_ahead_single(struct logical_volume *lv, void *data)
{
	struct lv_segment *seg = first_seg(lv);
	uint32_t seg_read_ahead = 0, *read_ahead = data;

	if (seg && seg->area_count && seg_type(seg, 0) == AREA_PV)
		dev_get_read_ahead(seg_pv(seg, 0)->dev, &seg_read_ahead);

	if (seg_read_ahead > *read_ahead)
		*read_ahead = seg_read_ahead;

	return 1;
}

/*
 * Calculate readahead for logical volume from underlying PV devices.
 * If read_ahead is NULL, only ensure that readahead of PVs are preloaded
 * into PV struct device in dev cache.
 */
void lv_calculate_readahead(const struct logical_volume *lv, uint32_t *read_ahead)
{
	uint32_t _read_ahead = 0;

	if (lv->read_ahead == DM_READ_AHEAD_AUTO)
		_lv_postorder((struct logical_volume *)lv, _lv_read_ahead_single, &_read_ahead);

	if (read_ahead) {
		log_debug("Calculated readahead of LV %s is %u", lv->name, _read_ahead);
		*read_ahead = _read_ahead;
	}
}

int vg_validate(struct volume_group *vg)
{
	struct pv_list *pvl, *pvl2;
	struct lv_list *lvl, *lvl2;
	char uuid[64] __attribute((aligned(8)));
	int r = 1;
	uint32_t hidden_lv_count = 0;

	/* FIXME Also check there's no data/metadata overlap */

	dm_list_iterate_items(pvl, &vg->pvs) {
		dm_list_iterate_items(pvl2, &vg->pvs) {
			if (pvl == pvl2)
				break;
			if (id_equal(&pvl->pv->id,
				     &pvl2->pv->id)) {
				if (!id_write_format(&pvl->pv->id, uuid,
						     sizeof(uuid)))
					 stack;
				log_error("Internal error: Duplicate PV id "
					  "%s detected for %s in %s.",
					  uuid, pv_dev_name(pvl->pv),
					  vg->name);
				r = 0;
			}
		}

		if (strcmp(pvl->pv->vg_name, vg->name)) {
			log_error("Internal error: VG name for PV %s is corrupted",
				  pv_dev_name(pvl->pv));
			r = 0;
		}
	}

	if (!check_pv_segments(vg)) {
		log_error("Internal error: PV segments corrupted in %s.",
			  vg->name);
		r = 0;
	}

	/*
	 * Count all non-snapshot invisible LVs
	 */
	dm_list_iterate_items(lvl, &vg->lvs) {
		if (lvl->lv->status & VISIBLE_LV)
			continue;

		/* snapshots */
		if (lv_is_cow(lvl->lv))
			continue;

		/* virtual origins are always hidden */
		if (lv_is_origin(lvl->lv) && !lv_is_virtual_origin(lvl->lv))
			continue;

		/* count other non-snapshot invisible volumes */
		hidden_lv_count++;

		/*
		 *  FIXME: add check for unreferenced invisible LVs
		 *   - snapshot cow & origin
		 *   - mirror log & images
		 *   - mirror conversion volumes (_mimagetmp*)
		 */
	}

	/*
	 * all volumes = visible LVs + snapshot_cows + invisible LVs
	 */
	if (((uint32_t) dm_list_size(&vg->lvs)) !=
	    vg_visible_lvs(vg) + snapshot_count(vg) + hidden_lv_count) {
		log_error("Internal error: #internal LVs (%u) != #LVs (%"
			  PRIu32 ") + #snapshots (%" PRIu32 ") + #internal LVs %u in VG %s",
			  dm_list_size(&vg->lvs), vg_visible_lvs(vg),
			  snapshot_count(vg), hidden_lv_count, vg->name);
		r = 0;
	}

	dm_list_iterate_items(lvl, &vg->lvs) {
		dm_list_iterate_items(lvl2, &vg->lvs) {
			if (lvl == lvl2)
				break;
			if (!strcmp(lvl->lv->name, lvl2->lv->name)) {
				log_error("Internal error: Duplicate LV name "
					  "%s detected in %s.", lvl->lv->name,
					  vg->name);
				r = 0;
			}
			if (id_equal(&lvl->lv->lvid.id[1],
				     &lvl2->lv->lvid.id[1])) {
				if (!id_write_format(&lvl->lv->lvid.id[1], uuid,
						     sizeof(uuid)))
					 stack;
				log_error("Internal error: Duplicate LV id "
					  "%s detected for %s and %s in %s.",
					  uuid, lvl->lv->name, lvl2->lv->name,
					  vg->name);
				r = 0;
			}
		}
	}

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (!check_lv_segments(lvl->lv, 1)) {
			log_error("Internal error: LV segments corrupted in %s.",
				  lvl->lv->name);
			r = 0;
		}
	}

	if (!(vg->fid->fmt->features & FMT_UNLIMITED_VOLS) &&
	    (!vg->max_lv || !vg->max_pv)) {
		log_error("Internal error: Volume group %s has limited PV/LV count"
			  " but limit is not set.", vg->name);
		r = 0;
	}

	if (vg_max_lv_reached(vg))
		stack;

	return r;
}

/*
 * After vg_write() returns success,
 * caller MUST call either vg_commit() or vg_revert()
 */
int vg_write(struct volume_group *vg)
{
	struct dm_list *mdah;
	struct metadata_area *mda;

	if (!vg_validate(vg))
		return_0;

	if (vg->status & PARTIAL_VG) {
		log_error("Cannot update partial volume group %s.", vg->name);
		return 0;
	}

	if (vg_missing_pv_count(vg) && !vg->cmd->handles_missing_pvs) {
		log_error("Cannot update volume group %s while physical "
			  "volumes are missing.", vg->name);
		return 0;
	}

	if (vg_has_unknown_segments(vg) && !vg->cmd->handles_unknown_segments) {
		log_error("Cannot update volume group %s with unknown segments in it!",
			  vg->name);
		return 0;
	}


	if (dm_list_empty(&vg->fid->metadata_areas)) {
		log_error("Aborting vg_write: No metadata areas to write to!");
		return 0;
	}

	if (!drop_cached_metadata(vg)) {
		log_error("Unable to drop cached metadata for VG %s.", vg->name);
		return 0;
	}

	vg->seqno++;

	/* Write to each copy of the metadata area */
	dm_list_iterate_items(mda, &vg->fid->metadata_areas) {
		if (!mda->ops->vg_write) {
			log_error("Format does not support writing volume"
				  "group metadata areas");
			/* Revert */
			dm_list_uniterate(mdah, &vg->fid->metadata_areas, &mda->list) {
				mda = dm_list_item(mdah, struct metadata_area);

				if (mda->ops->vg_revert &&
				    !mda->ops->vg_revert(vg->fid, vg, mda)) {
					stack;
				}
			}
			return 0;
		}
		if (!mda->ops->vg_write(vg->fid, vg, mda)) {
			stack;
			/* Revert */
			dm_list_uniterate(mdah, &vg->fid->metadata_areas, &mda->list) {
				mda = dm_list_item(mdah, struct metadata_area);

				if (mda->ops->vg_revert &&
				    !mda->ops->vg_revert(vg->fid, vg, mda)) {
					stack;
				}
			}
			return 0;
		}
	}

	/* Now pre-commit each copy of the new metadata */
	dm_list_iterate_items(mda, &vg->fid->metadata_areas) {
		if (mda->ops->vg_precommit &&
		    !mda->ops->vg_precommit(vg->fid, vg, mda)) {
			stack;
			/* Revert */
			dm_list_iterate_items(mda, &vg->fid->metadata_areas) {
				if (mda->ops->vg_revert &&
				    !mda->ops->vg_revert(vg->fid, vg, mda)) {
					stack;
				}
			}
			return 0;
		}
	}

	return 1;
}

/* Commit pending changes */
int vg_commit(struct volume_group *vg)
{
	struct metadata_area *mda;
	int cache_updated = 0;
	int failed = 0;

	if (!vgname_is_locked(vg->name)) {
		log_error("Internal error: Attempt to write new VG metadata "
			  "without locking %s", vg->name);
		return cache_updated;
	}

	/* Commit to each copy of the metadata area */
	dm_list_iterate_items(mda, &vg->fid->metadata_areas) {
		failed = 0;
		if (mda->ops->vg_commit &&
		    !mda->ops->vg_commit(vg->fid, vg, mda)) {
			stack;
			failed = 1;
		}
		/* Update cache first time we succeed */
		if (!failed && !cache_updated) {
			lvmcache_update_vg(vg, 0);
			cache_updated = 1;
		}
	}

	/* If update failed, remove any cached precommitted metadata. */
	if (!cache_updated && !drop_cached_metadata(vg))
		log_error("Attempt to drop cached metadata failed "
			  "after commit for VG %s.", vg->name);

	/* If at least one mda commit succeeded, it was committed */
	return cache_updated;
}

/* Don't commit any pending changes */
int vg_revert(struct volume_group *vg)
{
	struct metadata_area *mda;

	dm_list_iterate_items(mda, &vg->fid->metadata_areas) {
		if (mda->ops->vg_revert &&
		    !mda->ops->vg_revert(vg->fid, vg, mda)) {
			stack;
		}
	}

	if (!drop_cached_metadata(vg))
		log_error("Attempt to drop cached metadata failed "
			  "after reverted update for VG %s.", vg->name);

	return 1;
}

/* Make orphan PVs look like a VG */
static struct volume_group *_vg_read_orphans(struct cmd_context *cmd,
					     const char *orphan_vgname)
{
	struct lvmcache_vginfo *vginfo;
	struct lvmcache_info *info;
	struct pv_list *pvl;
	struct volume_group *vg;
	struct physical_volume *pv;
	struct dm_pool *mem;

	lvmcache_label_scan(cmd, 0);

	if (!(vginfo = vginfo_from_vgname(orphan_vgname, NULL)))
		return_NULL;

	if (!(mem = dm_pool_create("vg_read orphan", VG_MEMPOOL_CHUNK)))
		return_NULL;

	if (!(vg = dm_pool_zalloc(mem, sizeof(*vg)))) {
		log_error("vg allocation failed");
		return NULL;
	}
	dm_list_init(&vg->pvs);
	dm_list_init(&vg->lvs);
	dm_list_init(&vg->tags);
	dm_list_init(&vg->removed_pvs);
	vg->vgmem = mem;
	vg->cmd = cmd;
	if (!(vg->name = dm_pool_strdup(mem, orphan_vgname))) {
		log_error("vg name allocation failed");
		goto bad;
	}

	/* create format instance with appropriate metadata area */
	if (!(vg->fid = vginfo->fmt->ops->create_instance(vginfo->fmt,
							  orphan_vgname, NULL,
							  NULL))) {
		log_error("Failed to create format instance");
		goto bad;
	}

	dm_list_iterate_items(info, &vginfo->infos) {
		if (!(pv = _pv_read(cmd, mem, dev_name(info->dev), NULL, NULL, 1, 0))) {
			continue;
		}
		if (!(pvl = dm_pool_zalloc(mem, sizeof(*pvl)))) {
			log_error("pv_list allocation failed");
			goto bad;
		}
		pvl->pv = pv;
		dm_list_add(&vg->pvs, &pvl->list);
		vg->pv_count++;
	}

	return vg;
bad:
	dm_pool_destroy(mem);
	return NULL;
}

static int _update_pv_list(struct dm_pool *pvmem, struct dm_list *all_pvs, struct volume_group *vg)
{
	struct pv_list *pvl, *pvl2;

	dm_list_iterate_items(pvl, &vg->pvs) {
		dm_list_iterate_items(pvl2, all_pvs) {
			if (pvl->pv->dev == pvl2->pv->dev)
				goto next_pv;
		}

		/*
		 * PV is not on list so add it.
		 */
		if (!(pvl2 = _copy_pvl(pvmem, pvl))) {
			log_error("pv_list allocation for '%s' failed",
				  pv_dev_name(pvl->pv));
			return 0;
		}
		dm_list_add(all_pvs, &pvl2->list);
  next_pv:
		;
	}

	return 1;
}

int vg_missing_pv_count(const struct volume_group *vg)
{
	int ret = 0;
	struct pv_list *pvl;
	dm_list_iterate_items(pvl, &vg->pvs) {
		if (pvl->pv->status & MISSING_PV)
			++ ret;
	}
	return ret;
}

/* Caller sets consistent to 1 if it's safe for vg_read_internal to correct
 * inconsistent metadata on disk (i.e. the VG write lock is held).
 * This guarantees only consistent metadata is returned.
 * If consistent is 0, caller must check whether consistent == 1 on return
 * and take appropriate action if it isn't (e.g. abort; get write lock
 * and call vg_read_internal again).
 *
 * If precommitted is set, use precommitted metadata if present.
 *
 * Either of vgname or vgid may be NULL.
 */
static struct volume_group *_vg_read(struct cmd_context *cmd,
				     const char *vgname,
				     const char *vgid,
				     int *consistent, unsigned precommitted)
{
	struct format_instance *fid;
	const struct format_type *fmt;
	struct volume_group *vg, *correct_vg = NULL;
	struct metadata_area *mda;
	struct lvmcache_info *info;
	int inconsistent = 0;
	int inconsistent_vgid = 0;
	int inconsistent_pvs = 0;
	unsigned use_precommitted = precommitted;
	unsigned saved_handles_missing_pvs = cmd->handles_missing_pvs;
	struct dm_list *pvids;
	struct pv_list *pvl, *pvl2;
	struct dm_list all_pvs;
	char uuid[64] __attribute((aligned(8)));

	if (is_orphan_vg(vgname)) {
		if (use_precommitted) {
			log_error("Internal error: vg_read_internal requires vgname "
				  "with pre-commit.");
			return NULL;
		}
		*consistent = 1;
		return _vg_read_orphans(cmd, vgname);
	}

	if ((correct_vg = lvmcache_get_vg(vgid, precommitted))) {
		if (vg_missing_pv_count(correct_vg)) {
			log_verbose("There are %d physical volumes missing.",
				    vg_missing_pv_count(correct_vg));
			_vg_mark_partial_lvs(correct_vg);
		}
		*consistent = 1;
		return correct_vg;
	}

	/* Find the vgname in the cache */
	/* If it's not there we must do full scan to be completely sure */
	if (!(fmt = fmt_from_vgname(vgname, vgid))) {
		lvmcache_label_scan(cmd, 0);
		if (!(fmt = fmt_from_vgname(vgname, vgid))) {
			if (memlock())
				return_NULL;
			lvmcache_label_scan(cmd, 2);
			if (!(fmt = fmt_from_vgname(vgname, vgid)))
				return_NULL;
		}
	}

	/* Now determine the correct vgname if none was supplied */
	if (!vgname && !(vgname = vgname_from_vgid(cmd->mem, vgid)))
		return_NULL;

	if (use_precommitted && !(fmt->features & FMT_PRECOMMIT))
		use_precommitted = 0;

	/* create format instance with appropriate metadata area */
	if (!(fid = fmt->ops->create_instance(fmt, vgname, vgid, NULL))) {
		log_error("Failed to create format instance");
		return NULL;
	}

	/* Store pvids for later so we can check if any are missing */
	if (!(pvids = lvmcache_get_pvids(cmd, vgname, vgid)))
		return_NULL;

	/* Ensure contents of all metadata areas match - else do recovery */
	dm_list_iterate_items(mda, &fid->metadata_areas) {
		if ((use_precommitted &&
		     !(vg = mda->ops->vg_read_precommit(fid, vgname, mda))) ||
		    (!use_precommitted &&
		     !(vg = mda->ops->vg_read(fid, vgname, mda)))) {
			inconsistent = 1;
			vg_release(vg);
			continue;
		}
		if (!correct_vg) {
			correct_vg = vg;
			continue;
		}

		/* FIXME Also ensure contents same - checksum compare? */
		if (correct_vg->seqno != vg->seqno) {
			inconsistent = 1;
			if (vg->seqno > correct_vg->seqno) {
				vg_release(correct_vg);
				correct_vg = vg;
			}
		}

		if (vg != correct_vg)
			vg_release(vg);
	}

	/* Ensure every PV in the VG was in the cache */
	if (correct_vg) {
		/*
		 * If the VG has PVs without mdas, they may still be
		 * orphans in the cache: update the cache state here.
		 */
		if (!inconsistent &&
		    dm_list_size(&correct_vg->pvs) > dm_list_size(pvids)) {
			dm_list_iterate_items(pvl, &correct_vg->pvs) {
				if (!pvl->pv->dev) {
					inconsistent_pvs = 1;
					break;
				}

				if (str_list_match_item(pvids, pvl->pv->dev->pvid))
					continue;

				/*
				 * PV not marked as belonging to this VG in cache.
				 * Check it's an orphan without metadata area.
				 */
				if (!(info = info_from_pvid(pvl->pv->dev->pvid, 1)) ||
				   !info->vginfo || !is_orphan_vg(info->vginfo->vgname) ||
				   dm_list_size(&info->mdas)) {
					inconsistent_pvs = 1;
					break;
				}
			}

			/* If the check passed, let's update VG and recalculate pvids */
			if (!inconsistent_pvs) {
				log_debug("Updating cache for PVs without mdas "
					  "in VG %s.", vgname);
				lvmcache_update_vg(correct_vg, use_precommitted);

				if (!(pvids = lvmcache_get_pvids(cmd, vgname, vgid)))
					return_NULL;
			}
		}

		if (dm_list_size(&correct_vg->pvs) != dm_list_size(pvids)
		    + vg_missing_pv_count(correct_vg)) {
			log_debug("Cached VG %s had incorrect PV list",
				  vgname);

			if (memlock())
				inconsistent = 1;
			else {
				vg_release(correct_vg);
				correct_vg = NULL;
			}
		} else dm_list_iterate_items(pvl, &correct_vg->pvs) {
			if (pvl->pv->status & MISSING_PV)
				continue;
			if (!str_list_match_item(pvids, pvl->pv->dev->pvid)) {
				log_debug("Cached VG %s had incorrect PV list",
					  vgname);
				vg_release(correct_vg);
				correct_vg = NULL;
				break;
			}
		}
	}

	dm_list_init(&all_pvs);

	/* Failed to find VG where we expected it - full scan and retry */
	if (!correct_vg) {
		inconsistent = 0;

		if (memlock())
			return_NULL;
		lvmcache_label_scan(cmd, 2);
		if (!(fmt = fmt_from_vgname(vgname, vgid)))
			return_NULL;

		if (precommitted && !(fmt->features & FMT_PRECOMMIT))
			use_precommitted = 0;

		/* create format instance with appropriate metadata area */
		if (!(fid = fmt->ops->create_instance(fmt, vgname, vgid, NULL))) {
			log_error("Failed to create format instance");
			return NULL;
		}

		/* Ensure contents of all metadata areas match - else recover */
		dm_list_iterate_items(mda, &fid->metadata_areas) {
			if ((use_precommitted &&
			     !(vg = mda->ops->vg_read_precommit(fid, vgname,
								mda))) ||
			    (!use_precommitted &&
			     !(vg = mda->ops->vg_read(fid, vgname, mda)))) {
				inconsistent = 1;
				continue;
			}
			if (!correct_vg) {
				correct_vg = vg;
				if (!_update_pv_list(cmd->mem, &all_pvs, correct_vg)) {
					vg_release(vg);
					return_NULL;
				}
				continue;
			}

			if (strncmp((char *)vg->id.uuid,
			    (char *)correct_vg->id.uuid, ID_LEN)) {
				inconsistent = 1;
				inconsistent_vgid = 1;
			}

			/* FIXME Also ensure contents same - checksums same? */
			if (correct_vg->seqno != vg->seqno) {
				inconsistent = 1;
				if (!_update_pv_list(cmd->mem, &all_pvs, vg)) {
					vg_release(vg);
					vg_release(correct_vg);
					return_NULL;
				}
				if (vg->seqno > correct_vg->seqno) {
					vg_release(correct_vg);
					correct_vg = vg;
				}
			}

			if (vg != correct_vg)
				vg_release(vg);
		}

		/* Give up looking */
		if (!correct_vg)
			return_NULL;
	}

	lvmcache_update_vg(correct_vg, use_precommitted);

	if (inconsistent) {
		/* FIXME Test should be if we're *using* precommitted metadata not if we were searching for it */
		if (use_precommitted) {
			log_error("Inconsistent pre-commit metadata copies "
				  "for volume group %s", vgname);
			vg_release(correct_vg);
			return NULL;
		}

		if (!*consistent)
			return correct_vg;

		/* Don't touch if vgids didn't match */
		if (inconsistent_vgid) {
			log_error("Inconsistent metadata UUIDs found for "
				  "volume group %s", vgname);
			*consistent = 0;
			return correct_vg;
		}

		log_warn("WARNING: Inconsistent metadata found for VG %s - updating "
			 "to use version %u", vgname, correct_vg->seqno);

		cmd->handles_missing_pvs = 1;
		if (!vg_write(correct_vg)) {
			log_error("Automatic metadata correction failed");
			vg_release(correct_vg);
			cmd->handles_missing_pvs = saved_handles_missing_pvs;
			return NULL;
		}
		cmd->handles_missing_pvs = saved_handles_missing_pvs;

		if (!vg_commit(correct_vg)) {
			log_error("Automatic metadata correction commit "
				  "failed");
			vg_release(correct_vg);
			return NULL;
		}

		dm_list_iterate_items(pvl, &all_pvs) {
			dm_list_iterate_items(pvl2, &correct_vg->pvs) {
				if (pvl->pv->dev == pvl2->pv->dev)
					goto next_pv;
			}
			if (!id_write_format(&pvl->pv->id, uuid, sizeof(uuid))) {
				vg_release(correct_vg);
				return_NULL;
			}
			log_error("Removing PV %s (%s) that no longer belongs to VG %s",
				  pv_dev_name(pvl->pv), uuid, correct_vg->name);
			if (!pv_write_orphan(cmd, pvl->pv)) {
				vg_release(correct_vg);
				return_NULL;
			}
      next_pv:
			;
		}
	}

	if (vg_missing_pv_count(correct_vg)) {
		log_verbose("There are %d physical volumes missing.",
			    vg_missing_pv_count(correct_vg));
		_vg_mark_partial_lvs(correct_vg);
	}

	if ((correct_vg->status & PVMOVE) && !pvmove_mode()) {
		log_error("WARNING: Interrupted pvmove detected in "
			  "volume group %s", correct_vg->name);
		log_error("Please restore the metadata by running "
			  "vgcfgrestore.");
		vg_release(correct_vg);
		return NULL;
	}

	*consistent = 1;
	return correct_vg;
}

struct volume_group *vg_read_internal(struct cmd_context *cmd, const char *vgname,
			     const char *vgid, int *consistent)
{
	struct volume_group *vg;
	struct lv_list *lvl;

	if (!(vg = _vg_read(cmd, vgname, vgid, consistent, 0)))
		return NULL;

	if (!check_pv_segments(vg)) {
		log_error("Internal error: PV segments corrupted in %s.",
			  vg->name);
		vg_release(vg);
		return NULL;
	}

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (!check_lv_segments(lvl->lv, 1)) {
			log_error("Internal error: LV segments corrupted in %s.",
				  lvl->lv->name);
			vg_release(vg);
			return NULL;
		}
	}

	return vg;
}

void vg_release(struct volume_group *vg)
{
	if (!vg || !vg->vgmem)
		return;

	if (vg->cmd && vg->vgmem == vg->cmd->mem)
		log_error("Internal error: global memory pool used for VG %s",
			  vg->name);

	dm_pool_destroy(vg->vgmem);
}

/* This is only called by lv_from_lvid, which is only called from
 * activate.c so we know the appropriate VG lock is already held and
 * the vg_read_internal is therefore safe.
 */
static struct volume_group *_vg_read_by_vgid(struct cmd_context *cmd,
					    const char *vgid,
					    unsigned precommitted)
{
	const char *vgname;
	struct dm_list *vgnames;
	struct volume_group *vg = NULL;
	struct lvmcache_vginfo *vginfo;
	struct str_list *strl;
	int consistent = 0;

	/* Is corresponding vgname already cached? */
	if ((vginfo = vginfo_from_vgid(vgid)) &&
	    vginfo->vgname && !is_orphan_vg(vginfo->vgname)) {
		if ((vg = _vg_read(cmd, NULL, vgid,
				   &consistent, precommitted)) &&
		    !strncmp((char *)vg->id.uuid, vgid, ID_LEN)) {

			if (!consistent) {
				log_error("Volume group %s metadata is "
					  "inconsistent", vg->name);
			}
			return vg;
		}
		vg_release(vg);
	}

	/* Mustn't scan if memory locked: ensure cache gets pre-populated! */
	if (memlock())
		goto out;

	/* FIXME Need a genuine read by ID here - don't vg_read_internal by name! */
	/* FIXME Disabled vgrenames while active for now because we aren't
	 *       allowed to do a full scan here any more. */

	// The slow way - full scan required to cope with vgrename
	if (!(vgnames = get_vgnames(cmd, 2))) {
		log_error("vg_read_by_vgid: get_vgnames failed");
		goto out;
	}

	dm_list_iterate_items(strl, vgnames) {
		vgname = strl->str;
		if (!vgname || is_orphan_vg(vgname))
			continue;	// FIXME Unnecessary?
		consistent = 0;
		if ((vg = _vg_read(cmd, vgname, vgid, &consistent,
				   precommitted)) &&
		    !strncmp((char *)vg->id.uuid, vgid, ID_LEN)) {

			if (!consistent) {
				log_error("Volume group %s metadata is "
					  "inconsistent", vgname);
				goto out;
			}
			return vg;
		}
	}

out:
	vg_release(vg);
	return NULL;
}

/* Only called by activate.c */
struct logical_volume *lv_from_lvid(struct cmd_context *cmd, const char *lvid_s,
				    unsigned precommitted)
{
	struct lv_list *lvl;
	struct volume_group *vg;
	const union lvid *lvid;

	lvid = (const union lvid *) lvid_s;

	log_very_verbose("Finding volume group for uuid %s", lvid_s);
	if (!(vg = _vg_read_by_vgid(cmd, (char *)lvid->id[0].uuid, precommitted))) {
		log_error("Volume group for uuid not found: %s", lvid_s);
		return NULL;
	}

	log_verbose("Found volume group \"%s\"", vg->name);
	if (vg->status & EXPORTED_VG) {
		log_error("Volume group \"%s\" is exported", vg->name);
		goto out;
	}
	if (!(lvl = find_lv_in_vg_by_lvid(vg, lvid))) {
		log_very_verbose("Can't find logical volume id %s", lvid_s);
		goto out;
	}

	return lvl->lv;
out:
	vg_release(vg);
	return NULL;
}

/**
 * pv_read - read and return a handle to a physical volume
 * @cmd: LVM command initiating the pv_read
 * @pv_name: full device name of the PV, including the path
 * @mdas: list of metadata areas of the PV
 * @label_sector: sector number where the PV label is stored on @pv_name
 * @warnings:
 *
 * Returns:
 *   PV handle - valid pv_name and successful read of the PV, or
 *   NULL - invalid parameter or error in reading the PV
 *
 * Note:
 *   FIXME - liblvm todo - make into function that returns handle
 */
struct physical_volume *pv_read(struct cmd_context *cmd, const char *pv_name,
				struct dm_list *mdas, uint64_t *label_sector,
				int warnings, int scan_label_only)
{
	return _pv_read(cmd, cmd->mem, pv_name, mdas, label_sector, warnings, scan_label_only);
}

/* FIXME Use label functions instead of PV functions */
static struct physical_volume *_pv_read(struct cmd_context *cmd,
					struct dm_pool *pvmem,
					const char *pv_name,
					struct dm_list *mdas,
					uint64_t *label_sector,
					int warnings, int scan_label_only)
{
	struct physical_volume *pv;
	struct label *label;
	struct lvmcache_info *info;
	struct device *dev;

	if (!(dev = dev_cache_get(pv_name, cmd->filter)))
		return_NULL;

	if (!(label_read(dev, &label, UINT64_C(0)))) {
		if (warnings)
			log_error("No physical volume label read from %s",
				  pv_name);
		return NULL;
	}

	info = (struct lvmcache_info *) label->info;
	if (label_sector && *label_sector)
		*label_sector = label->sector;

	if (!(pv = dm_pool_zalloc(pvmem, sizeof(*pv)))) {
		log_error("pv allocation for '%s' failed", pv_name);
		return NULL;
	}

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	/* FIXME Move more common code up here */
	if (!(info->fmt->ops->pv_read(info->fmt, pv_name, pv, mdas,
	      scan_label_only))) {
		log_error("Failed to read existing physical volume '%s'",
			  pv_name);
		return NULL;
	}

	if (!pv->size)
		return NULL;

	if (!alloc_pv_segment_whole_pv(pvmem, pv))
		return_NULL;

	return pv;
}

/* May return empty list */
struct dm_list *get_vgnames(struct cmd_context *cmd, int full_scan)
{
	return lvmcache_get_vgnames(cmd, full_scan);
}

struct dm_list *get_vgids(struct cmd_context *cmd, int full_scan)
{
	return lvmcache_get_vgids(cmd, full_scan);
}

static int _get_pvs(struct cmd_context *cmd, struct dm_list **pvslist)
{
	struct str_list *strl;
	struct dm_list * uninitialized_var(results);
	const char *vgname, *vgid;
	struct pv_list *pvl, *pvl_copy;
	struct dm_list *vgids;
	struct volume_group *vg;
	int consistent = 0;
	int old_pvmove;

	lvmcache_label_scan(cmd, 0);

	if (pvslist) {
		if (!(results = dm_pool_alloc(cmd->mem, sizeof(*results)))) {
			log_error("PV list allocation failed");
			return 0;
		}

		dm_list_init(results);
	}

	/* Get list of VGs */
	if (!(vgids = get_vgids(cmd, 0))) {
		log_error("get_pvs: get_vgids failed");
		return 0;
	}

	/* Read every VG to ensure cache consistency */
	/* Orphan VG is last on list */
	old_pvmove = pvmove_mode();
	init_pvmove(1);
	dm_list_iterate_items(strl, vgids) {
		vgid = strl->str;
		if (!vgid)
			continue;	/* FIXME Unnecessary? */
		consistent = 0;
		if (!(vgname = vgname_from_vgid(NULL, vgid))) {
			stack;
			continue;
		}
		if (!(vg = vg_read_internal(cmd, vgname, vgid, &consistent))) {
			stack;
			continue;
		}
		if (!consistent)
			log_warn("WARNING: Volume Group %s is not consistent",
				 vgname);

		/* Move PVs onto results list */
		if (pvslist)
			dm_list_iterate_items(pvl, &vg->pvs) {
				if (!(pvl_copy = _copy_pvl(cmd->mem, pvl))) {
					log_error("PV list allocation failed");
					vg_release(vg);
					return 0;
				}
				dm_list_add(results, &pvl_copy->list);
			}
		vg_release(vg);
	}
	init_pvmove(old_pvmove);

	if (pvslist)
		*pvslist = results;
	else
		dm_pool_free(cmd->mem, vgids);

	return 1;
}

struct dm_list *get_pvs(struct cmd_context *cmd)
{
	struct dm_list *results;

	if (!_get_pvs(cmd, &results))
		return NULL;

	return results;
}

int scan_vgs_for_pvs(struct cmd_context *cmd)
{
	return _get_pvs(cmd, NULL);
}

int pv_write(struct cmd_context *cmd __attribute((unused)),
	     struct physical_volume *pv,
	     struct dm_list *mdas, int64_t label_sector)
{
	if (!pv->fmt->ops->pv_write) {
		log_error("Format does not support writing physical volumes");
		return 0;
	}

	if (!is_orphan_vg(pv->vg_name) || pv->pe_alloc_count) {
		log_error("Assertion failed: can't _pv_write non-orphan PV "
			  "(in VG %s)", pv->vg_name);
		return 0;
	}

	if (!pv->fmt->ops->pv_write(pv->fmt, pv, mdas, label_sector))
		return_0;

	return 1;
}

int pv_write_orphan(struct cmd_context *cmd, struct physical_volume *pv)
{
	const char *old_vg_name = pv->vg_name;

	pv->vg_name = cmd->fmt->orphan_vg_name;
	pv->status = ALLOCATABLE_PV;
	pv->pe_alloc_count = 0;

	if (!dev_get_size(pv->dev, &pv->size)) {
		log_error("%s: Couldn't get size.", pv_dev_name(pv));
		return 0;
	}

	if (!pv_write(cmd, pv, NULL, INT64_C(-1))) {
		log_error("Failed to clear metadata from physical "
			  "volume \"%s\" after removal from \"%s\"",
			  pv_dev_name(pv), old_vg_name);
		return 0;
	}

	return 1;
}

/**
 * is_orphan_vg - Determine whether a vg_name is an orphan
 * @vg_name: pointer to the vg_name
 */
int is_orphan_vg(const char *vg_name)
{
	return (vg_name && vg_name[0] == ORPHAN_PREFIX[0]) ? 1 : 0;
}

/**
 * is_orphan - Determine whether a pv is an orphan based on its vg_name
 * @pv: handle to the physical volume
 */
int is_orphan(const struct physical_volume *pv)
{
	return is_orphan_vg(pv_field(pv, vg_name));
}

/**
 * is_pv - Determine whether a pv is a real pv or dummy one
 * @pv: handle to device
 */
int is_pv(struct physical_volume *pv)
{
	return (pv_field(pv, vg_name) ? 1 : 0);
}

/*
 * Returns:
 *  0 - fail
 *  1 - success
 */
int pv_analyze(struct cmd_context *cmd, const char *pv_name,
	       uint64_t label_sector)
{
	struct label *label;
	struct device *dev;
	struct metadata_area *mda;
	struct lvmcache_info *info;

	dev = dev_cache_get(pv_name, cmd->filter);
	if (!dev) {
		log_error("Device %s not found (or ignored by filtering).",
			  pv_name);
		return 0;
	}

	/*
	 * First, scan for LVM labels.
	 */
	if (!label_read(dev, &label, label_sector)) {
		log_error("Could not find LVM label on %s",
			  pv_name);
		return 0;
	}

	log_print("Found label on %s, sector %"PRIu64", type=%s",
		  pv_name, label->sector, label->type);

	/*
	 * Next, loop through metadata areas
	 */
	info = label->info;
	dm_list_iterate_items(mda, &info->mdas)
		mda->ops->pv_analyze_mda(info->fmt, mda);

	return 1;
}

/* FIXME: remove / combine this with locking? */
int vg_check_write_mode(struct volume_group *vg)
{
	if (vg->open_mode != 'w') {
		log_errno(EPERM, "Attempt to modify a read-only VG");
		return 0;
	}
	return 1;
}

/*
 * Performs a set of checks against a VG according to bits set in status
 * and returns FAILED_* bits for those that aren't acceptable.
 *
 * FIXME Remove the unnecessary duplicate definitions and return bits directly.
 */
static uint32_t _vg_bad_status_bits(const struct volume_group *vg,
				    uint32_t status)
{
	uint32_t failure = 0;

	if ((status & CLUSTERED) &&
	    (vg_is_clustered(vg)) && !locking_is_clustered()) {
		log_error("Skipping clustered volume group %s", vg->name);
		/* Return because other flags are considered undefined. */
		return FAILED_CLUSTERED;
	}

	if ((status & EXPORTED_VG) &&
	    vg_is_exported(vg)) {
		log_error("Volume group %s is exported", vg->name);
		failure |= FAILED_EXPORTED;
	}

	if ((status & LVM_WRITE) &&
	    !(vg->status & LVM_WRITE)) {
		log_error("Volume group %s is read-only", vg->name);
		failure |= FAILED_READ_ONLY;
	}

	if ((status & RESIZEABLE_VG) &&
	    !vg_is_resizeable(vg)) {
		log_error("Volume group %s is not resizeable.", vg->name);
		failure |= FAILED_RESIZEABLE;
	}

	return failure;
}

/**
 * vg_check_status - check volume group status flags and log error
 * @vg - volume group to check status flags
 * @status - specific status flags to check (e.g. EXPORTED_VG)
 */
int vg_check_status(const struct volume_group *vg, uint32_t status)
{
	return !_vg_bad_status_bits(vg, status);
}

static struct volume_group *_recover_vg(struct cmd_context *cmd, const char *lock_name,
			 const char *vg_name, const char *vgid,
			 uint32_t lock_flags)
{
	int consistent = 1;
	struct volume_group *vg;

	lock_flags &= ~LCK_TYPE_MASK;
	lock_flags |= LCK_WRITE;

	unlock_vg(cmd, lock_name);

	dev_close_all();

	if (!lock_vol(cmd, lock_name, lock_flags))
		return_NULL;

	if (!(vg = vg_read_internal(cmd, vg_name, vgid, &consistent)))
		return_NULL;

	if (!consistent) {
		vg_release(vg);
		return_NULL;
	}

	return (struct volume_group *)vg;
}

/*
 * Consolidated locking, reading, and status flag checking.
 *
 * If the metadata is inconsistent, setting READ_ALLOW_INCONSISTENT in
 * misc_flags will return it with FAILED_INCONSISTENT set instead of 
 * giving you nothing.
 *
 * Use vg_read_error(vg) to determine the result.  Nonzero means there were
 * problems reading the volume group.
 * Zero value means that the VG is open and appropriate locks are held.
 */
static struct volume_group *_vg_lock_and_read(struct cmd_context *cmd, const char *vg_name,
			       const char *vgid, uint32_t lock_flags,
			       uint32_t status_flags, uint32_t misc_flags)
{
	struct volume_group *vg = NULL;
	const char *lock_name;
 	int consistent = 1;
	int consistent_in;
	uint32_t failure = 0;
	int already_locked;

	if (misc_flags & READ_ALLOW_INCONSISTENT || !(lock_flags & LCK_WRITE))
		consistent = 0;

	if (!validate_name(vg_name) && !is_orphan_vg(vg_name)) {
		log_error("Volume group name %s has invalid characters",
			  vg_name);
		return NULL;
	}

	lock_name = is_orphan_vg(vg_name) ? VG_ORPHANS : vg_name;
	already_locked = vgname_is_locked(lock_name);

	if (!already_locked && !(misc_flags & READ_WITHOUT_LOCK) &&
	    !lock_vol(cmd, lock_name, lock_flags)) {
		log_error("Can't get lock for %s", vg_name);
		return _vg_make_handle(cmd, vg, FAILED_LOCKING);
	}

	if (is_orphan_vg(vg_name))
		status_flags &= ~LVM_WRITE;

	consistent_in = consistent;

	/* If consistent == 1, we get NULL here if correction fails. */
	if (!(vg = vg_read_internal(cmd, vg_name, vgid, &consistent))) {
		if (consistent_in && !consistent) {
			log_error("Volume group \"%s\" inconsistent.", vg_name);
			failure |= FAILED_INCONSISTENT;
			goto_bad;
		}

		log_error("Volume group \"%s\" not found", vg_name);

		failure |= FAILED_NOTFOUND;
		goto_bad;
	}

	if (vg_is_clustered(vg) && !locking_is_clustered()) {
		log_error("Skipping clustered volume group %s", vg->name);
		failure |= FAILED_CLUSTERED;
		goto_bad;
	}

	/* consistent == 0 when VG is not found, but failed == FAILED_NOTFOUND */
	if (!consistent && !failure) {
		vg_release(vg);
		if (!(vg = _recover_vg(cmd, lock_name, vg_name, vgid, lock_flags))) {
			log_error("Recovery of volume group \"%s\" failed.",
				  vg_name);
			failure |= FAILED_INCONSISTENT;
			goto_bad;
		}
	}

	/*
	 * Check that the tool can handle tricky cases -- missing PVs and
	 * unknown segment types.
	 */

	if (!cmd->handles_missing_pvs && vg_missing_pv_count(vg) &&
	    (lock_flags & LCK_WRITE)) {
		log_error("Cannot change VG %s while PVs are missing.", vg->name);
		log_error("Consider vgreduce --removemissing.");
		failure |= FAILED_INCONSISTENT; /* FIXME new failure code here? */
		goto_bad;
	}

	if (!cmd->handles_unknown_segments && vg_has_unknown_segments(vg) &&
	    (lock_flags & LCK_WRITE)) {
		log_error("Cannot change VG %s with unknown segments in it!",
			  vg->name);
		failure |= FAILED_INCONSISTENT; /* FIXME new failure code here? */
		goto_bad;
	}

	failure |= _vg_bad_status_bits(vg, status_flags);
	if (failure)
		goto_bad;

	return _vg_make_handle(cmd, vg, failure);

bad:
	if (!already_locked && !(misc_flags & READ_WITHOUT_LOCK))
		unlock_vg(cmd, lock_name);

	return _vg_make_handle(cmd, vg, failure);
}

/*
 * vg_read: High-level volume group metadata read function.
 *
 * vg_read_error() must be used on any handle returned to check for errors.
 *
 *  - metadata inconsistent and automatic correction failed: FAILED_INCONSISTENT
 *  - VG is read-only: FAILED_READ_ONLY
 *  - VG is EXPORTED, unless flags has READ_ALLOW_EXPORTED: FAILED_EXPORTED
 *  - VG is not RESIZEABLE: FAILED_RESIZEABLE
 *  - locking failed: FAILED_LOCKING
 *
 * On failures, all locks are released, unless one of the following applies:
 *  - vgname_is_locked(lock_name) is true
 * FIXME: remove the above 2 conditions if possible and make an error always
 * release the lock.
 *
 * Volume groups are opened read-only unless flags contains READ_FOR_UPDATE.
 *
 * Checking for VG existence:
 *
 * FIXME: We want vg_read to attempt automatic recovery after acquiring a
 * temporary write lock: if that fails, we bail out as usual, with failed &
 * FAILED_INCONSISTENT. If it works, we are good to go. Code that's been in
 * toollib just set lock_flags to LCK_VG_WRITE and called vg_read_internal with
 * *consistent = 1.
 */
struct volume_group *vg_read(struct cmd_context *cmd, const char *vg_name,
	      const char *vgid, uint32_t flags)
{
	uint32_t status = 0;
	uint32_t lock_flags = LCK_VG_READ;

	if (flags & READ_FOR_UPDATE) {
		status |= EXPORTED_VG | LVM_WRITE;
		lock_flags = LCK_VG_WRITE;
	}

	if (flags & READ_ALLOW_EXPORTED)
		status &= ~EXPORTED_VG;

	return _vg_lock_and_read(cmd, vg_name, vgid, lock_flags, status, flags);
}

/*
 * A high-level volume group metadata reading function. Open a volume group for
 * later update (this means the user code can change the metadata and later
 * request the new metadata to be written and committed).
 */
struct volume_group *vg_read_for_update(struct cmd_context *cmd, const char *vg_name,
			 const char *vgid, uint32_t flags)
{
	return vg_read(cmd, vg_name, vgid, flags | READ_FOR_UPDATE);
}

/*
 * Test the validity of a VG handle returned by vg_read() or vg_read_for_update().
 */
uint32_t vg_read_error(struct volume_group *vg_handle)
{
	if (!vg_handle)
		return FAILED_ALLOCATION;

	return vg_handle->read_status;
}

/*
 * Lock a vgname and/or check for existence.
 * Takes a WRITE lock on the vgname before scanning.
 * If scanning fails or vgname found, release the lock.
 * NOTE: If you find the return codes confusing, you might think of this
 * function as similar to an open() call with O_CREAT and O_EXCL flags
 * (open returns fail with -EEXIST if file already exists).
 *
 * Returns:
 * FAILED_LOCKING - Cannot lock name
 * FAILED_EXIST - VG name already exists - cannot reserve
 * SUCCESS - VG name does not exist in system and WRITE lock held
 */
uint32_t vg_lock_newname(struct cmd_context *cmd, const char *vgname)
{
	if (!lock_vol(cmd, vgname, LCK_VG_WRITE)) {
		return FAILED_LOCKING;
	}

	/* Find the vgname in the cache */
	/* If it's not there we must do full scan to be completely sure */
	if (!fmt_from_vgname(vgname, NULL)) {
		lvmcache_label_scan(cmd, 0);
		if (!fmt_from_vgname(vgname, NULL)) {
			if (memlock()) {
				/*
				 * FIXME: Disallow calling this function if
				 * memlock() is true.
				 */
				unlock_vg(cmd, vgname);
				return FAILED_LOCKING;
			}
			lvmcache_label_scan(cmd, 2);
			if (!fmt_from_vgname(vgname, NULL)) {
				/* vgname not found after scanning */
				return SUCCESS;
			}
		}
	}

	/* Found vgname so cannot reserve. */
	unlock_vg(cmd, vgname);
	return FAILED_EXIST;
}

/*
 * Gets/Sets for external LVM library
 */
struct id pv_id(const struct physical_volume *pv)
{
	return pv_field(pv, id);
}

const struct format_type *pv_format_type(const struct physical_volume *pv)
{
	return pv_field(pv, fmt);
}

struct id pv_vgid(const struct physical_volume *pv)
{
	return pv_field(pv, vgid);
}

struct device *pv_dev(const struct physical_volume *pv)
{
	return pv_field(pv, dev);
}

const char *pv_vg_name(const struct physical_volume *pv)
{
	return pv_field(pv, vg_name);
}

const char *pv_dev_name(const struct physical_volume *pv)
{
	return dev_name(pv_dev(pv));
}

uint64_t pv_size(const struct physical_volume *pv)
{
	return pv_field(pv, size);
}

uint32_t pv_status(const struct physical_volume *pv)
{
	return pv_field(pv, status);
}

uint32_t pv_pe_size(const struct physical_volume *pv)
{
	return pv_field(pv, pe_size);
}

uint64_t pv_pe_start(const struct physical_volume *pv)
{
	return pv_field(pv, pe_start);
}

uint32_t pv_pe_count(const struct physical_volume *pv)
{
	return pv_field(pv, pe_count);
}

uint32_t pv_pe_alloc_count(const struct physical_volume *pv)
{
	return pv_field(pv, pe_alloc_count);
}

uint32_t pv_mda_count(const struct physical_volume *pv)
{
	struct lvmcache_info *info;

	info = info_from_pvid((const char *)&pv->id.uuid, 0);
	return info ? dm_list_size(&info->mdas) : UINT64_C(0);
}

uint32_t vg_seqno(const struct volume_group *vg)
{
	return vg->seqno;
}

uint32_t vg_status(const struct volume_group *vg)
{
	return vg->status;
}

uint64_t vg_size(const struct volume_group *vg)
{
	return (uint64_t) vg->extent_count * vg->extent_size;
}

uint64_t vg_free(const struct volume_group *vg)
{
	return (uint64_t) vg->free_count * vg->extent_size;
}

uint64_t vg_extent_size(const struct volume_group *vg)
{
	return (uint64_t) vg->extent_size;
}

uint64_t vg_extent_count(const struct volume_group *vg)
{
	return (uint64_t) vg->extent_count;
}

uint64_t vg_free_count(const struct volume_group *vg)
{
	return (uint64_t) vg->free_count;
}

uint64_t vg_pv_count(const struct volume_group *vg)
{
	return (uint64_t) vg->pv_count;
}

uint64_t vg_max_pv(const struct volume_group *vg)
{
	return (uint64_t) vg->max_pv;
}

uint64_t vg_max_lv(const struct volume_group *vg)
{
	return (uint64_t) vg->max_lv;
}

uint32_t vg_mda_count(const struct volume_group *vg)
{
	return dm_list_size(&vg->fid->metadata_areas);
}

uint64_t lv_size(const struct logical_volume *lv)
{
	return lv->size;
}

/**
 * pv_by_path - Given a device path return a PV handle if it is a PV
 * @cmd - handle to the LVM command instance
 * @pv_name - device path to read for the PV
 *
 * Returns:
 *  NULL - device path does not contain a valid PV
 *  non-NULL - PV handle corresponding to device path
 *
 * FIXME: merge with find_pv_by_name ?
 */
struct physical_volume *pv_by_path(struct cmd_context *cmd, const char *pv_name)
{
	struct dm_list mdas;

	dm_list_init(&mdas);
	return _pv_read(cmd, cmd->mem, pv_name, &mdas, NULL, 1, 0);
}
