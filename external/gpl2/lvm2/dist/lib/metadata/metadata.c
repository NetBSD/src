/*	$NetBSD: metadata.c,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

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
#include "activate.h"
#include "display.h"
#include "locking.h"
#include "archiver.h"
#include "defaults.h"

#include <sys/param.h>

/*
 * FIXME: Check for valid handle before dereferencing field or log error?
 */
#define pv_field(handle, field)				\
	(((const struct physical_volume *)(handle))->field)

static struct physical_volume *_pv_read(struct cmd_context *cmd,
					const char *pv_name,
					struct dm_list *mdas,
					uint64_t *label_sector,
					int warnings);

static struct physical_volume *_pv_create(const struct format_type *fmt,
				  struct device *dev,
				  struct id *id, uint64_t size,
				  uint64_t pe_start,
				  uint32_t existing_extent_count,
				  uint32_t existing_extent_size,
				  int pvmetadatacopies,
				  uint64_t pvmetadatasize, struct dm_list *mdas);

static int _pv_write(struct cmd_context *cmd __attribute((unused)),
		     struct physical_volume *pv,
	     	     struct dm_list *mdas, int64_t label_sector);

static struct physical_volume *_find_pv_by_name(struct cmd_context *cmd,
			 			const char *pv_name);

static struct pv_list *_find_pv_in_vg(const struct volume_group *vg,
				      const char *pv_name);

static struct physical_volume *_find_pv_in_vg_by_uuid(const struct volume_group *vg,
						      const struct id *id);

unsigned long pe_align(struct physical_volume *pv)
{
	if (pv->pe_align)
		goto out;

	pv->pe_align = MAX(65536UL, lvm_getpagesize()) >> SECTOR_SHIFT;

	/*
	 * Align to chunk size of underlying md device if present
	 */
	if (!pv->dev)
		goto out;

	if (find_config_tree_bool(pv->fmt->cmd, "devices/md_chunk_alignment",
				  DEFAULT_MD_CHUNK_ALIGNMENT))
		pv->pe_align = MAX(pv->pe_align,
				   dev_md_chunk_size(pv->fmt->cmd->sysfs_dir,
						     pv->dev));

	log_very_verbose("%s: Setting PE alignment to %lu sectors.",
			 dev_name(pv->dev), pv->pe_align);

out:
	return pv->pe_align;
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
	struct dm_pool *mem = fid->fmt->cmd->mem;

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

	/* FIXME Do proper rounding-up alignment? */
	/* Reserved space for label; this holds 0 for PVs created by LVM1 */
	if (pv->pe_start < pe_align(pv))
		pv->pe_start = pe_align(pv);

	/*
	 * pe_count must always be calculated by pv_setup
	 */
	pv->pe_alloc_count = 0;

	if (!fid->fmt->ops->pv_setup(fid->fmt, UINT64_C(0), 0,
				     vg->extent_size, 0, UINT64_C(0),
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

static int _copy_pv(struct physical_volume *pv_to,
		    struct physical_volume *pv_from)
{
	memcpy(pv_to, pv_from, sizeof(*pv_to));

	if (!str_list_dup(pv_to->fmt->cmd->mem, &pv_to->tags, &pv_from->tags)) {
		log_error("PV tags duplication failed");
		return 0;
	}

	if (!peg_dup(pv_to->fmt->cmd->mem, &pv_to->segments,
		     &pv_from->segments))
		return_0;

	return 1;
}

int get_pv_from_vg_by_id(const struct format_type *fmt, const char *vg_name,
			 const char *vgid, const char *pvid,
			 struct physical_volume *pv)
{
	struct volume_group *vg;
	struct pv_list *pvl;
	int consistent = 0;

	if (!(vg = vg_read(fmt->cmd, vg_name, vgid, &consistent))) {
		log_error("get_pv_from_vg_by_id: vg_read failed to read VG %s",
			  vg_name);
		return 0;
	}

	if (!consistent)
		log_warn("WARNING: Volume group %s is not consistent",
			 vg_name);

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (id_equal(&pvl->pv->id, (const struct id *) pvid)) {
			if (!_copy_pv(pv, pvl->pv))
				return_0;
			return 1;
		}
	}

	return 0;
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
	struct dm_pool *mem = cmd->mem;
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

static int remove_lvs_in_vg(struct cmd_context *cmd,
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

/* FIXME: remove redundant vg_name */
int vg_remove_single(struct cmd_context *cmd, const char *vg_name,
		     struct volume_group *vg, int consistent,
		     force_t force __attribute((unused)))
{
	struct physical_volume *pv;
	struct pv_list *pvl;
	unsigned lv_count;
	int ret = 1;

	if (!vg || !consistent || vg_missing_pv_count(vg)) {
		log_error("Volume group \"%s\" not found, is inconsistent "
			  "or has PVs missing.", vg_name);
		log_error("Consider vgreduce --removemissing if metadata "
			  "is inconsistent.");
		return 0;
	}

	if (!vg_check_status(vg, EXPORTED_VG))
		return 0;

	lv_count = displayable_lvs_in_vg(vg);

	if (lv_count) {
		if ((force == PROMPT) &&
		    (yes_no_prompt("Do you really want to remove volume "
				   "group \"%s\" containing %u "
				   "logical volumes? [y/n]: ",
				   vg_name, lv_count) == 'n')) {
			log_print("Volume group \"%s\" not removed", vg_name);
			return 0;
		}
		if (!remove_lvs_in_vg(cmd, vg, force))
			return 0;
	}

	lv_count = displayable_lvs_in_vg(vg);
	
	if (lv_count) {
		log_error("Volume group \"%s\" still contains %u "
			  "logical volume(s)", vg_name, lv_count);
		return 0;
	}

	if (!archive(vg))
		return 0;

	if (!vg_remove(vg)) {
		log_error("vg_remove %s failed", vg_name);
		return 0;
	}

	/* init physical volumes */
	dm_list_iterate_items(pvl, &vg->pvs) {
		pv = pvl->pv;
		log_verbose("Removing physical volume \"%s\" from "
			    "volume group \"%s\"", pv_dev_name(pv), vg_name);
		pv->vg_name = vg->fid->fmt->orphan_vg_name;
		pv->status = ALLOCATABLE_PV;

		if (!dev_get_size(pv_dev(pv), &pv->size)) {
			log_error("%s: Couldn't get size.", pv_dev_name(pv));
			ret = 0;
			continue;
		}

		/* FIXME Write to same sector label was read from */
		if (!pv_write(cmd, pv, NULL, INT64_C(-1))) {
			log_error("Failed to remove physical volume \"%s\""
				  " from volume group \"%s\"",
				  pv_dev_name(pv), vg_name);
			ret = 0;
		}
	}

	backup_remove(cmd, vg_name);

	if (ret)
		log_print("Volume group \"%s\" successfully removed", vg_name);
	else
		log_error("Volume group \"%s\" not properly removed", vg_name);

	return ret;
}

int vg_extend(struct volume_group *vg, int pv_count, char **pv_names)
{
	int i;
	struct physical_volume *pv;

	/* attach each pv */
	for (i = 0; i < pv_count; i++) {
		if (!(pv = pv_by_path(vg->fid->fmt->cmd, pv_names[i]))) {
			log_error("%s not identified as an existing "
				  "physical volume", pv_names[i]);
			goto bad;
		}
		
		if (!add_pv_to_vg(vg, pv_names[i], pv))
			goto bad;
	}

/* FIXME Decide whether to initialise and add new mdahs to format instance */

	return 1;
	
      bad:
	log_error("Unable to add physical volume '%s' to "
		  "volume group '%s'.", pv_names[i], vg->name);
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
int validate_vg_create_params(struct cmd_context *cmd,
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

struct volume_group *vg_create(struct cmd_context *cmd, const char *vg_name,
			       uint32_t extent_size, uint32_t max_pv,
			       uint32_t max_lv, alloc_policy_t alloc,
			       int pv_count, char **pv_names)
{
	struct volume_group *vg;
	struct dm_pool *mem = cmd->mem;
	int consistent = 0;

	if (!(vg = dm_pool_zalloc(mem, sizeof(*vg))))
		return_NULL;

	/* is this vg name already in use ? */
	if (vg_read(cmd, vg_name, NULL, &consistent)) {
		log_err("A volume group called '%s' already exists.", vg_name);
		goto bad;
	}

	if (!id_create(&vg->id)) {
		log_err("Couldn't create uuid for volume group '%s'.", vg_name);
		goto bad;
	}

	/* Strip dev_dir if present */
	vg_name = strip_dir(vg_name, cmd->dev_dir);

	vg->cmd = cmd;

	if (!(vg->name = dm_pool_strdup(mem, vg_name)))
		goto_bad;

	vg->seqno = 0;

	vg->status = (RESIZEABLE_VG | LVM_READ | LVM_WRITE);
	if (!(vg->system_id = dm_pool_alloc(mem, NAME_LEN)))
		goto_bad;

	*vg->system_id = '\0';

	vg->extent_size = extent_size;
	vg->extent_count = 0;
	vg->free_count = 0;

	vg->max_lv = max_lv;
	vg->max_pv = max_pv;

	vg->alloc = alloc;

	vg->pv_count = 0;
	dm_list_init(&vg->pvs);

	vg->lv_count = 0;
	dm_list_init(&vg->lvs);

	vg->snapshot_count = 0;

	dm_list_init(&vg->tags);

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

	/* attach the pv's */
	if (!vg_extend(vg, pv_count, pv_names))
		goto_bad;

	return vg;

      bad:
	dm_pool_free(mem, vg);
	return NULL;
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

int vg_change_pesize(struct cmd_context *cmd __attribute((unused)),
		     struct volume_group *vg, uint32_t new_size)
{
	uint32_t old_size = vg->extent_size;
	struct pv_list *pvl;
	struct lv_list *lvl;
	struct physical_volume *pv;
	struct logical_volume *lv;
	struct lv_segment *seg;
	struct pv_segment *pvseg;
	uint32_t s;

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

/**
 * pv_create - initialize a physical volume for use with a volume group
 * @fmt: format type
 * @dev: PV device to initialize
 * @id: PV UUID to use for initialization
 * @size: size of the PV in sectors
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
 *   FIXME - liblvm todo - tidy up arguments for external use (fmt, mdas, etc)
 */
pv_t *pv_create(const struct cmd_context *cmd,
		struct device *dev,
		struct id *id, uint64_t size,
		uint64_t pe_start,
		uint32_t existing_extent_count,
		uint32_t existing_extent_size,
		int pvmetadatacopies,
		uint64_t pvmetadatasize, struct dm_list *mdas)
{
	return _pv_create(cmd->fmt, dev, id, size, pe_start,
			  existing_extent_count,
			  existing_extent_size,
			  pvmetadatacopies,
			  pvmetadatasize, mdas);
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
	pv->fmt = NULL;
	pv->dev = dev;

	pv->status = ALLOCATABLE_PV;

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	return pv;
}

/* Sizes in sectors */
static struct physical_volume *_pv_create(const struct format_type *fmt,
				  struct device *dev,
				  struct id *id, uint64_t size,
				  uint64_t pe_start,
				  uint32_t existing_extent_count,
				  uint32_t existing_extent_size,
				  int pvmetadatacopies,
				  uint64_t pvmetadatasize, struct dm_list *mdas)
{
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

	pv->fmt = fmt;
	pv->vg_name = fmt->orphan_vg_name;

	if (!fmt->ops->pv_setup(fmt, pe_start, existing_extent_count,
				existing_extent_size,
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
pv_t *find_pv_in_vg_by_uuid(const struct volume_group *vg,
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

	if (!(pv = _pv_read(cmd, pv_name, NULL, NULL, 1))) {
		log_error("Physical volume %s not found", pv_name);
		return NULL;
	}

	if (is_orphan_vg(pv->vg_name)) {
		/* If a PV has no MDAs - need to search all VGs for it */
		if (!scan_vgs_for_pvs(cmd))
			return_NULL;
		if (!(pv = _pv_read(cmd, pv_name, NULL, NULL, 1))) {
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
	struct lv_segment *seg = NULL;

	dm_list_iterate_items(seg, &lv->segments)
		break;

	return seg;
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

int vg_remove(struct volume_group *vg)
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

unsigned displayable_lvs_in_vg(const struct volume_group *vg)
{
	struct lv_list *lvl;
	unsigned lv_count = 0;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (lv_is_displayable(lvl->lv))
			lv_count++;

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
	    (vg_to->max_lv < vg_to->lv_count + vg_from->lv_count)) {
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
	if (r) {
		r = fn(lv, data);
		log_verbose("visited %s", lv->name);
	}
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

int vg_validate(struct volume_group *vg)
{
	struct pv_list *pvl, *pvl2;
	struct lv_list *lvl, *lvl2;
	char uuid[64] __attribute((aligned(8)));
	int r = 1;
	uint32_t lv_count;

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

	if ((lv_count = (uint32_t) dm_list_size(&vg->lvs)) !=
	    vg->lv_count + 2 * vg->snapshot_count) {
		log_error("Internal error: #internal LVs (%u) != #LVs (%"
			  PRIu32 ") + 2 * #snapshots (%" PRIu32 ") in VG %s",
			  dm_list_size(&vg->lvs), vg->lv_count,
			  vg->snapshot_count, vg->name);
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

	lvmcache_label_scan(cmd, 0);

	if (!(vginfo = vginfo_from_vgname(orphan_vgname, NULL)))
		return_NULL;

	if (!(vg = dm_pool_zalloc(cmd->mem, sizeof(*vg)))) {
		log_error("vg allocation failed");
		return NULL;
	}
	dm_list_init(&vg->pvs);
	dm_list_init(&vg->lvs);
	dm_list_init(&vg->tags);
	vg->cmd = cmd;
	if (!(vg->name = dm_pool_strdup(cmd->mem, orphan_vgname))) {
		log_error("vg name allocation failed");
		return NULL;
	}

	/* create format instance with appropriate metadata area */
	if (!(vg->fid = vginfo->fmt->ops->create_instance(vginfo->fmt,
							  orphan_vgname, NULL,
							  NULL))) {
		log_error("Failed to create format instance");
		dm_pool_free(cmd->mem, vg);
		return NULL;
	}

	dm_list_iterate_items(info, &vginfo->infos) {
		if (!(pv = _pv_read(cmd, dev_name(info->dev), NULL, NULL, 1))) {
			continue;
		}
		if (!(pvl = dm_pool_zalloc(cmd->mem, sizeof(*pvl)))) {
			log_error("pv_list allocation failed");
			return NULL;
		}
		pvl->pv = pv;
		dm_list_add(&vg->pvs, &pvl->list);
		vg->pv_count++;
	}

	return vg;
}

static int _update_pv_list(struct dm_list *all_pvs, struct volume_group *vg)
{
	struct pv_list *pvl, *pvl2;

	dm_list_iterate_items(pvl, &vg->pvs) {
		dm_list_iterate_items(pvl2, all_pvs) {
			if (pvl->pv->dev == pvl2->pv->dev)
				goto next_pv;
		}
		/* PV is not on list so add it.  Note that we don't copy it. */
       		if (!(pvl2 = dm_pool_zalloc(vg->cmd->mem, sizeof(*pvl2)))) {
			log_error("pv_list allocation for '%s' failed",
				  pv_dev_name(pvl->pv));
			return 0;
		}
		pvl2->pv = pvl->pv;
		dm_list_add(all_pvs, &pvl2->list);
  next_pv:
		;
	}

	return 1;
}

int vg_missing_pv_count(const vg_t *vg)
{
	int ret = 0;
	struct pv_list *pvl;
	dm_list_iterate_items(pvl, &vg->pvs) {
		if (pvl->pv->status & MISSING_PV)
			++ ret;
	}
	return ret;
}

/* Caller sets consistent to 1 if it's safe for vg_read to correct
 * inconsistent metadata on disk (i.e. the VG write lock is held).
 * This guarantees only consistent metadata is returned.
 * If consistent is 0, caller must check whether consistent == 1 on return
 * and take appropriate action if it isn't (e.g. abort; get write lock
 * and call vg_read again).
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
	struct dm_list *pvids;
	struct pv_list *pvl, *pvl2;
	struct dm_list all_pvs;
	char uuid[64] __attribute((aligned(8)));

	if (is_orphan_vg(vgname)) {
		if (use_precommitted) {
			log_error("Internal error: vg_read requires vgname "
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
			continue;
		}
		if (!correct_vg) {
			correct_vg = vg;
			continue;
		}
		/* FIXME Also ensure contents same - checksum compare? */
		if (correct_vg->seqno != vg->seqno) {
			inconsistent = 1;
			if (vg->seqno > correct_vg->seqno)
				correct_vg = vg;
		}
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
			else
				correct_vg = NULL;
		} else dm_list_iterate_items(pvl, &correct_vg->pvs) {
			if (pvl->pv->status & MISSING_PV)
				continue;
			if (!str_list_match_item(pvids, pvl->pv->dev->pvid)) {
				log_debug("Cached VG %s had incorrect PV list",
					  vgname);
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
				if (!_update_pv_list(&all_pvs, correct_vg))
					return_NULL;
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
				if (!_update_pv_list(&all_pvs, vg))
					return_NULL;
				if (vg->seqno > correct_vg->seqno)
					correct_vg = vg;
			}
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

		if (!vg_write(correct_vg)) {
			log_error("Automatic metadata correction failed");
			return NULL;
		}

		if (!vg_commit(correct_vg)) {
			log_error("Automatic metadata correction commit "
				  "failed");
			return NULL;
		}

		dm_list_iterate_items(pvl, &all_pvs) {
			dm_list_iterate_items(pvl2, &correct_vg->pvs) {
				if (pvl->pv->dev == pvl2->pv->dev)
					goto next_pv;
			}
			if (!id_write_format(&pvl->pv->id, uuid, sizeof(uuid)))
				return_NULL;
			log_error("Removing PV %s (%s) that no longer belongs to VG %s",
				  pv_dev_name(pvl->pv), uuid, correct_vg->name);
			if (!pv_write_orphan(cmd, pvl->pv))
				return_NULL;
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
		return NULL;
	}

	*consistent = 1;
	return correct_vg;
}

struct volume_group *vg_read(struct cmd_context *cmd, const char *vgname,
			     const char *vgid, int *consistent)
{
	struct volume_group *vg;
	struct lv_list *lvl;

	if (!(vg = _vg_read(cmd, vgname, vgid, consistent, 0)))
		return NULL;

	if (!check_pv_segments(vg)) {
		log_error("Internal error: PV segments corrupted in %s.",
			  vg->name);
		return NULL;
	}

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (!check_lv_segments(lvl->lv, 1)) {
			log_error("Internal error: LV segments corrupted in %s.",
				  lvl->lv->name);
			return NULL;
		}
	}

	return vg;
}

/* This is only called by lv_from_lvid, which is only called from
 * activate.c so we know the appropriate VG lock is already held and
 * the vg_read is therefore safe.
 */
static struct volume_group *_vg_read_by_vgid(struct cmd_context *cmd,
					    const char *vgid,
					    unsigned precommitted)
{
	const char *vgname;
	struct dm_list *vgnames;
	struct volume_group *vg;
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
	}

	/* Mustn't scan if memory locked: ensure cache gets pre-populated! */
	if (memlock())
		return NULL;

	/* FIXME Need a genuine read by ID here - don't vg_read by name! */
	/* FIXME Disabled vgrenames while active for now because we aren't
	 *       allowed to do a full scan here any more. */

	// The slow way - full scan required to cope with vgrename
	if (!(vgnames = get_vgs(cmd, 2))) {
		log_error("vg_read_by_vgid: get_vgs failed");
		return NULL;
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
				return NULL;
			}
			return vg;
		}
	}

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
		return NULL;
	}
	if (!(lvl = find_lv_in_vg_by_lvid(vg, lvid))) {
		log_very_verbose("Can't find logical volume id %s", lvid_s);
		return NULL;
	}

	return lvl->lv;
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
				int warnings)
{
	return _pv_read(cmd, pv_name, mdas, label_sector, warnings);
}

/* FIXME Use label functions instead of PV functions */
static struct physical_volume *_pv_read(struct cmd_context *cmd,
					const char *pv_name,
					struct dm_list *mdas,
					uint64_t *label_sector,
					int warnings)
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

	if (!(pv = dm_pool_zalloc(cmd->mem, sizeof(*pv)))) {
		log_error("pv allocation for '%s' failed", pv_name);
		return NULL;
	}

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	/* FIXME Move more common code up here */
	if (!(info->fmt->ops->pv_read(info->fmt, pv_name, pv, mdas))) {
		log_error("Failed to read existing physical volume '%s'",
			  pv_name);
		return NULL;
	}

	if (!pv->size)
		return NULL;
	
	if (!alloc_pv_segment_whole_pv(cmd->mem, pv))
		return_NULL;

	return pv;
}

/* May return empty list */
struct dm_list *get_vgs(struct cmd_context *cmd, int full_scan)
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
	struct dm_list *pvh, *tmp;
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
		log_error("get_pvs: get_vgs failed");
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
		if (!(vg = vg_read(cmd, vgname, vgid, &consistent))) {
			stack;
			continue;
		}
		if (!consistent)
			log_warn("WARNING: Volume Group %s is not consistent",
				 vgname);

		/* Move PVs onto results list */
		if (pvslist)
			dm_list_iterate_safe(pvh, tmp, &vg->pvs)
				dm_list_add(results, pvh);
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

/* FIXME: liblvm todo - make into function that takes handle */
int pv_write(struct cmd_context *cmd __attribute((unused)),
	     struct physical_volume *pv,
	     struct dm_list *mdas, int64_t label_sector)
{
	return _pv_write(cmd, pv, mdas, label_sector);
}

static int _pv_write(struct cmd_context *cmd __attribute((unused)),
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

	if (!_pv_write(cmd, pv, NULL, INT64_C(-1))) {
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
int is_orphan(const pv_t *pv)
{
	return is_orphan_vg(pv_field(pv, vg_name));
}

/**
 * is_pv - Determine whether a pv is a real pv or dummy one
 * @pv: handle to device
 */
int is_pv(pv_t *pv)
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



/**
 * vg_check_status - check volume group status flags and log error
 * @vg - volume group to check status flags
 * @status - specific status flags to check (e.g. EXPORTED_VG)
 *
 * Returns:
 * 0 - fail
 * 1 - success
 */
int vg_check_status(const struct volume_group *vg, uint32_t status)
{
	if ((status & CLUSTERED) &&
	    (vg_is_clustered(vg)) && !locking_is_clustered() &&
	    !lockingfailed()) {
		log_error("Skipping clustered volume group %s", vg->name);
		return 0;
	}

	if ((status & EXPORTED_VG) &&
	    (vg->status & EXPORTED_VG)) {
		log_error("Volume group %s is exported", vg->name);
		return 0;
	}

	if ((status & LVM_WRITE) &&
	    !(vg->status & LVM_WRITE)) {
		log_error("Volume group %s is read-only", vg->name);
		return 0;
	}
	if ((status & RESIZEABLE_VG) &&
	    !(vg->status & RESIZEABLE_VG)) {
		log_error("Volume group %s is not resizeable.", vg->name);
		return 0;
	}

	return 1;
}

/*
 * vg_lock_and_read - consolidate vg locking, reading, and status flag checking
 *
 * Returns:
 * NULL - failure
 * non-NULL - success; volume group handle
 */
vg_t *vg_lock_and_read(struct cmd_context *cmd, const char *vg_name,
		       const char *vgid,
		       uint32_t lock_flags, uint32_t status_flags,
		       uint32_t misc_flags)
{
	struct volume_group *vg;
	int consistent = 1;

	if (!(misc_flags & CORRECT_INCONSISTENT))
		consistent = 0;

	if (!validate_name(vg_name)) {
		log_error("Volume group name %s has invalid characters",
			  vg_name);
		return NULL;
	}

	if (!lock_vol(cmd, vg_name, lock_flags)) {
		log_error("Can't get lock for %s", vg_name);
		return NULL;
	}

	if (!(vg = vg_read(cmd, vg_name, vgid, &consistent)) ||
	    ((misc_flags & FAIL_INCONSISTENT) && !consistent)) {
		log_error("Volume group \"%s\" not found", vg_name);
		unlock_vg(cmd, vg_name);
		return NULL;
	}

	if (!vg_check_status(vg, status_flags)) {
		unlock_vg(cmd, vg_name);
		return NULL;
	}

	return vg;
}

/*
 * Gets/Sets for external LVM library
 */
struct id pv_id(const pv_t *pv)
{
	return pv_field(pv, id);
}

const struct format_type *pv_format_type(const pv_t *pv)
{
	return pv_field(pv, fmt);
}

struct id pv_vgid(const pv_t *pv)
{
	return pv_field(pv, vgid);
}

struct device *pv_dev(const pv_t *pv)
{
	return pv_field(pv, dev);
}

const char *pv_vg_name(const pv_t *pv)
{
	return pv_field(pv, vg_name);
}

const char *pv_dev_name(const pv_t *pv)
{
	return dev_name(pv_dev(pv));
}

uint64_t pv_size(const pv_t *pv)
{
	return pv_field(pv, size);
}

uint32_t pv_status(const pv_t *pv)
{
	return pv_field(pv, status);
}

uint32_t pv_pe_size(const pv_t *pv)
{
	return pv_field(pv, pe_size);
}

uint64_t pv_pe_start(const pv_t *pv)
{
	return pv_field(pv, pe_start);
}

uint32_t pv_pe_count(const pv_t *pv)
{
	return pv_field(pv, pe_count);
}

uint32_t pv_pe_alloc_count(const pv_t *pv)
{
	return pv_field(pv, pe_alloc_count);
}

uint32_t vg_status(const vg_t *vg)
{
	return vg->status;
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
pv_t *pv_by_path(struct cmd_context *cmd, const char *pv_name)
{
	struct dm_list mdas;
	
	dm_list_init(&mdas);
	return _pv_read(cmd, pv_name, &mdas, NULL, 1);
}
