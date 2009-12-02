/*	$NetBSD: format1.c,v 1.1.1.2 2009/12/02 00:26:48 haad Exp $	*/

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
#include "disk-rep.h"
#include "limits.h"
#include "display.h"
#include "toolcontext.h"
#include "lvm1-label.h"
#include "format1.h"
#include "segtype.h"

/* VG consistency checks */
static int _check_vgs(struct dm_list *pvs)
{
	struct dm_list *pvh, *t;
	struct disk_list *dl = NULL;
	struct disk_list *first = NULL;

	uint32_t pv_count = 0;
	uint32_t exported = 0;
	int first_time = 1;

	/*
	 * If there are exported and unexported PVs, ignore exported ones.
	 * This means an active VG won't be affected if disks are inserted
	 * bearing an exported VG with the same name.
	 */
	dm_list_iterate_items(dl, pvs) {
		if (first_time) {
			exported = dl->pvd.pv_status & VG_EXPORTED;
			first_time = 0;
			continue;
		}

		if (exported != (dl->pvd.pv_status & VG_EXPORTED)) {
			/* Remove exported PVs */
			dm_list_iterate_safe(pvh, t, pvs) {
				dl = dm_list_item(pvh, struct disk_list);
				if (dl->pvd.pv_status & VG_EXPORTED)
					dm_list_del(pvh);
			}
			break;
		}
	}

	/* Remove any PVs with VG structs that differ from the first */
	dm_list_iterate_safe(pvh, t, pvs) {
		dl = dm_list_item(pvh, struct disk_list);

		if (!first)
			first = dl;

		else if (memcmp(&first->vgd, &dl->vgd, sizeof(first->vgd))) {
			log_error("VG data differs between PVs %s and %s",
				  dev_name(first->dev), dev_name(dl->dev));
			log_debug("VG data on %s: %s %s %" PRIu32 " %" PRIu32
				  "  %" PRIu32 " %" PRIu32 " %" PRIu32 " %"
				  PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32
				  " %" PRIu32 " %" PRIu32 " %" PRIu32 " %"
				  PRIu32 " %" PRIu32 " %" PRIu32,
				  dev_name(first->dev), first->vgd.vg_uuid,
				  first->vgd.vg_name_dummy,
				  first->vgd.vg_number, first->vgd.vg_access,
				  first->vgd.vg_status, first->vgd.lv_max,
				  first->vgd.lv_cur, first->vgd.lv_open,
				  first->vgd.pv_max, first->vgd.pv_cur,
				  first->vgd.pv_act, first->vgd.dummy,
				  first->vgd.vgda, first->vgd.pe_size,
				  first->vgd.pe_total, first->vgd.pe_allocated,
				  first->vgd.pvg_total);
			log_debug("VG data on %s: %s %s %" PRIu32 " %" PRIu32
				  "  %" PRIu32 " %" PRIu32 " %" PRIu32 " %"
				  PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32
				  " %" PRIu32 " %" PRIu32 " %" PRIu32 " %"
				  PRIu32 " %" PRIu32 " %" PRIu32,
				  dev_name(dl->dev), dl->vgd.vg_uuid,
				  dl->vgd.vg_name_dummy, dl->vgd.vg_number,
				  dl->vgd.vg_access, dl->vgd.vg_status,
				  dl->vgd.lv_max, dl->vgd.lv_cur,
				  dl->vgd.lv_open, dl->vgd.pv_max,
				  dl->vgd.pv_cur, dl->vgd.pv_act, dl->vgd.dummy,
				  dl->vgd.vgda, dl->vgd.pe_size,
				  dl->vgd.pe_total, dl->vgd.pe_allocated,
				  dl->vgd.pvg_total);
			dm_list_del(pvh);
			return 0;
		}
		pv_count++;
	}

	/* On entry to fn, list known to be non-empty */
	if (pv_count != first->vgd.pv_cur) {
		log_error("%d PV(s) found for VG %s: expected %d",
			  pv_count, first->pvd.vg_name, first->vgd.pv_cur);
	}

	return 1;
}

static struct volume_group *_build_vg(struct format_instance *fid,
				      struct dm_list *pvs,
				      struct dm_pool *mem)
{
	struct volume_group *vg = dm_pool_alloc(mem, sizeof(*vg));
	struct disk_list *dl;

	if (!vg)
		goto_bad;

	if (dm_list_empty(pvs))
		goto_bad;

	memset(vg, 0, sizeof(*vg));

	vg->cmd = fid->fmt->cmd;
	vg->vgmem = mem;
	vg->fid = fid;
	vg->seqno = 0;
	dm_list_init(&vg->pvs);
	dm_list_init(&vg->lvs);
	dm_list_init(&vg->tags);
	dm_list_init(&vg->removed_pvs);

	if (!_check_vgs(pvs))
		goto_bad;

	dl = dm_list_item(pvs->n, struct disk_list);

	if (!import_vg(mem, vg, dl))
		goto_bad;

	if (!import_pvs(fid->fmt, mem, vg, pvs, &vg->pvs, &vg->pv_count))
		goto_bad;

	if (!import_lvs(mem, vg, pvs))
		goto_bad;

	if (!import_extents(fid->fmt->cmd, vg, pvs))
		goto_bad;

	if (!import_snapshots(mem, vg, pvs))
		goto_bad;

	return vg;

      bad:
	dm_pool_free(mem, vg);
	return NULL;
}

static struct volume_group *_format1_vg_read(struct format_instance *fid,
				     const char *vg_name,
				     struct metadata_area *mda __attribute((unused)))
{
	struct dm_pool *mem = dm_pool_create("lvm1 vg_read", VG_MEMPOOL_CHUNK);
	struct dm_list pvs;
	struct volume_group *vg = NULL;
	dm_list_init(&pvs);

	if (!mem)
		return_NULL;

	/* Strip dev_dir if present */
	vg_name = strip_dir(vg_name, fid->fmt->cmd->dev_dir);

	if (!read_pvs_in_vg
	    (fid->fmt, vg_name, fid->fmt->cmd->filter, mem, &pvs))
		goto_bad;

	if (!(vg = _build_vg(fid, &pvs, mem)))
		goto_bad;

	return vg;
bad:
	dm_pool_destroy(mem);
	return NULL;
}

static struct disk_list *_flatten_pv(struct format_instance *fid,
				     struct dm_pool *mem, struct volume_group *vg,
				     struct physical_volume *pv,
				     const char *dev_dir)
{
	struct disk_list *dl = dm_pool_alloc(mem, sizeof(*dl));

	if (!dl)
		return_NULL;

	dl->mem = mem;
	dl->dev = pv->dev;

	dm_list_init(&dl->uuids);
	dm_list_init(&dl->lvds);

	if (!export_pv(fid->fmt->cmd, mem, vg, &dl->pvd, pv) ||
	    !export_vg(&dl->vgd, vg) ||
	    !export_uuids(dl, vg) ||
	    !export_lvs(dl, vg, pv, dev_dir) || !calculate_layout(dl)) {
		dm_pool_free(mem, dl);
		return_NULL;
	}

	return dl;
}

static int _flatten_vg(struct format_instance *fid, struct dm_pool *mem,
		       struct volume_group *vg,
		       struct dm_list *pvds, const char *dev_dir,
		       struct dev_filter *filter)
{
	struct pv_list *pvl;
	struct disk_list *data;

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (!(data = _flatten_pv(fid, mem, vg, pvl->pv, dev_dir)))
			return_0;

		dm_list_add(pvds, &data->list);
	}

	export_numbers(pvds, vg);
	export_pv_act(pvds);

	if (!export_vg_number(fid, pvds, vg->name, filter))
		return_0;

	return 1;
}

static int _format1_vg_write(struct format_instance *fid, struct volume_group *vg,
		     struct metadata_area *mda __attribute((unused)))
{
	struct dm_pool *mem = dm_pool_create("lvm1 vg_write", VG_MEMPOOL_CHUNK);
	struct dm_list pvds;
	int r = 0;

	if (!mem)
		return_0;

	dm_list_init(&pvds);

	r = (_flatten_vg(fid, mem, vg, &pvds, fid->fmt->cmd->dev_dir,
			 fid->fmt->cmd->filter) &&
	     write_disks(fid->fmt, &pvds));

	lvmcache_update_vg(vg, 0);
	dm_pool_destroy(mem);
	return r;
}

static int _format1_pv_read(const struct format_type *fmt, const char *pv_name,
		    struct physical_volume *pv, struct dm_list *mdas __attribute((unused)),
		    int scan_label_only __attribute((unused)))
{
	struct dm_pool *mem = dm_pool_create("lvm1 pv_read", 1024);
	struct disk_list *dl;
	struct device *dev;
	int r = 0;

	log_very_verbose("Reading physical volume data %s from disk", pv_name);

	if (!mem)
		return_0;

	if (!(dev = dev_cache_get(pv_name, fmt->cmd->filter)))
		goto_out;

	if (!(dl = read_disk(fmt, dev, mem, NULL)))
		goto_out;

	if (!import_pv(fmt, fmt->cmd->mem, dl->dev, NULL, pv, &dl->pvd, &dl->vgd))
		goto_out;

	pv->fmt = fmt;

	r = 1;

      out:
	dm_pool_destroy(mem);
	return r;
}

static int _format1_pv_setup(const struct format_type *fmt,
		     uint64_t pe_start, uint32_t extent_count,
		     uint32_t extent_size,
		     unsigned long data_alignment __attribute((unused)),
		     unsigned long data_alignment_offset __attribute((unused)),
		     int pvmetadatacopies __attribute((unused)),
		     uint64_t pvmetadatasize __attribute((unused)), struct dm_list *mdas __attribute((unused)),
		     struct physical_volume *pv, struct volume_group *vg __attribute((unused)))
{
	if (pv->size > MAX_PV_SIZE)
		pv->size--;
	if (pv->size > MAX_PV_SIZE) {
		log_error("Physical volumes cannot be bigger than %s",
			  display_size(fmt->cmd, (uint64_t) MAX_PV_SIZE));
		return 0;
	}

	/* Nothing more to do if extent size isn't provided */
	if (!extent_size)
		return 1;

	/*
	 * This works out pe_start and pe_count.
	 */
	if (!calculate_extent_count(pv, extent_size, extent_count, pe_start))
		return_0;

	/* Retain existing extent locations exactly */
	if (((pe_start || extent_count) && (pe_start != pv->pe_start)) ||
	    (extent_count && (extent_count != pv->pe_count))) {
		log_error("Metadata would overwrite physical extents");
		return 0;
	}

	return 1;
}

static int _format1_lv_setup(struct format_instance *fid, struct logical_volume *lv)
{
	uint64_t max_size = UINT_MAX;

	if (!*lv->lvid.s)
		lvid_from_lvnum(&lv->lvid, &lv->vg->id, find_free_lvnum(lv));

	if (lv->le_count > MAX_LE_TOTAL) {
		log_error("logical volumes cannot contain more than "
			  "%d extents.", MAX_LE_TOTAL);
		return 0;
	}
	if (lv->size > max_size) {
		log_error("logical volumes cannot be larger than %s",
			  display_size(fid->fmt->cmd, max_size));
		return 0;
	}

	return 1;
}

static int _format1_pv_write(const struct format_type *fmt, struct physical_volume *pv,
		     struct dm_list *mdas __attribute((unused)), int64_t sector __attribute((unused)))
{
	struct dm_pool *mem;
	struct disk_list *dl;
	struct dm_list pvs;
	struct label *label;
	struct lvmcache_info *info;

	if (!(info = lvmcache_add(fmt->labeller, (char *) &pv->id, pv->dev,
				  pv->vg_name, NULL, 0)))
		return_0;
	label = info->label;
	info->device_size = pv->size << SECTOR_SHIFT;
	info->fmt = fmt;

	dm_list_init(&info->mdas);

	dm_list_init(&pvs);

	/* Ensure any residual PE structure is gone */
	pv->pe_size = pv->pe_count = 0;
	pv->pe_start = LVM1_PE_ALIGN;

	if (!(mem = dm_pool_create("lvm1 pv_write", 1024)))
		return_0;

	if (!(dl = dm_pool_alloc(mem, sizeof(*dl))))
		goto_bad;

	dl->mem = mem;
	dl->dev = pv->dev;

	if (!export_pv(fmt->cmd, mem, NULL, &dl->pvd, pv))
		goto_bad;

	/* must be set to be able to zero gap after PV structure in
	   dev_write in order to make other disk tools happy */
	dl->pvd.pv_on_disk.base = METADATA_BASE;
	dl->pvd.pv_on_disk.size = PV_SIZE;
	dl->pvd.pe_on_disk.base = LVM1_PE_ALIGN << SECTOR_SHIFT;

	dm_list_add(&pvs, &dl->list);
	if (!write_disks(fmt, &pvs))
		goto_bad;

	dm_pool_destroy(mem);
	return 1;

      bad:
	dm_pool_destroy(mem);
	return 0;
}

static int _format1_vg_setup(struct format_instance *fid, struct volume_group *vg)
{
	/* just check max_pv and max_lv */
	if (!vg->max_lv || vg->max_lv >= MAX_LV)
		vg->max_lv = MAX_LV - 1;

	if (!vg->max_pv || vg->max_pv >= MAX_PV)
		vg->max_pv = MAX_PV - 1;

	if (vg->extent_size > MAX_PE_SIZE || vg->extent_size < MIN_PE_SIZE) {
		log_error("Extent size must be between %s and %s",
			  display_size(fid->fmt->cmd, (uint64_t) MIN_PE_SIZE),
			  display_size(fid->fmt->cmd, (uint64_t) MAX_PE_SIZE));

		return 0;
	}

	if (vg->extent_size % MIN_PE_SIZE) {
		log_error("Extent size must be multiple of %s",
			  display_size(fid->fmt->cmd, (uint64_t) MIN_PE_SIZE));
		return 0;
	}

	/* Redundant? */
	if (vg->extent_size & (vg->extent_size - 1)) {
		log_error("Extent size must be power of 2");
		return 0;
	}

	return 1;
}

static int _format1_segtype_supported(struct format_instance *fid __attribute((unused)),
				      const struct segment_type *segtype)
{
	if (!(segtype->flags & SEG_FORMAT1_SUPPORT))
		return_0;

	return 1;
}

static struct metadata_area_ops _metadata_format1_ops = {
	.vg_read = _format1_vg_read,
	.vg_write = _format1_vg_write,
};

static struct format_instance *_format1_create_instance(const struct format_type *fmt,
						const char *vgname __attribute((unused)),
						const char *vgid __attribute((unused)),
						void *private __attribute((unused)))
{
	struct format_instance *fid;
	struct metadata_area *mda;

	if (!(fid = dm_pool_alloc(fmt->cmd->mem, sizeof(*fid))))
		return_NULL;

	fid->fmt = fmt;
	dm_list_init(&fid->metadata_areas);

	/* Define a NULL metadata area */
	if (!(mda = dm_pool_alloc(fmt->cmd->mem, sizeof(*mda)))) {
		dm_pool_free(fmt->cmd->mem, fid);
		return_NULL;
	}

	mda->ops = &_metadata_format1_ops;
	mda->metadata_locn = NULL;
	dm_list_add(&fid->metadata_areas, &mda->list);

	return fid;
}

static void _format1_destroy_instance(struct format_instance *fid __attribute((unused)))
{
	return;
}

static void _format1_destroy(const struct format_type *fmt)
{
	dm_free((void *) fmt);
}

static struct format_handler _format1_ops = {
	.pv_read = _format1_pv_read,
	.pv_setup = _format1_pv_setup,
	.pv_write = _format1_pv_write,
	.lv_setup = _format1_lv_setup,
	.vg_setup = _format1_vg_setup,
	.segtype_supported = _format1_segtype_supported,
	.create_instance = _format1_create_instance,
	.destroy_instance = _format1_destroy_instance,
	.destroy = _format1_destroy,
};

#ifdef LVM1_INTERNAL
struct format_type *init_lvm1_format(struct cmd_context *cmd)
#else				/* Shared */
struct format_type *init_format(struct cmd_context *cmd);
struct format_type *init_format(struct cmd_context *cmd)
#endif
{
	struct format_type *fmt = dm_malloc(sizeof(*fmt));

	if (!fmt)
		return_NULL;

	fmt->cmd = cmd;
	fmt->ops = &_format1_ops;
	fmt->name = FMT_LVM1_NAME;
	fmt->alias = NULL;
	fmt->orphan_vg_name = FMT_LVM1_ORPHAN_VG_NAME;
	fmt->features = FMT_RESTRICTED_LVIDS | FMT_ORPHAN_ALLOCATABLE |
			FMT_RESTRICTED_READAHEAD;
	fmt->private = NULL;

	if (!(fmt->labeller = lvm1_labeller_create(fmt))) {
		log_error("Couldn't create lvm1 label handler.");
		return NULL;
	}

	if (!(label_register_handler(FMT_LVM1_NAME, fmt->labeller))) {
		log_error("Couldn't register lvm1 label handler.");
		return NULL;
	}

	log_very_verbose("Initialised format: %s", fmt->name);

	return fmt;
}
