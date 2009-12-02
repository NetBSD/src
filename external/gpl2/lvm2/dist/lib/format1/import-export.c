/*	$NetBSD: import-export.c,v 1.1.1.2 2009/12/02 00:26:49 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
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

/*
 * Translates between disk and in-core formats.
 */

#include "lib.h"
#include "disk-rep.h"
#include "lvm-string.h"
#include "filter.h"
#include "toolcontext.h"
#include "segtype.h"
#include "pv_alloc.h"
#include "display.h"
#include "lvmcache.h"
#include "metadata.h"

#include <time.h>

static int _check_vg_name(const char *name)
{
	return strlen(name) < NAME_LEN;
}

/*
 * Extracts the last part of a path.
 */
static char *_create_lv_name(struct dm_pool *mem, const char *full_name)
{
	const char *ptr = strrchr(full_name, '/');

	if (!ptr)
		ptr = full_name;
	else
		ptr++;

	return dm_pool_strdup(mem, ptr);
}

int import_pv(const struct format_type *fmt, struct dm_pool *mem,
	      struct device *dev, struct volume_group *vg,
	      struct physical_volume *pv, struct pv_disk *pvd,
	      struct vg_disk *vgd)
{
	uint64_t size;

	memset(pv, 0, sizeof(*pv));
	memcpy(&pv->id, pvd->pv_uuid, ID_LEN);

	pv->dev = dev;
	if (!*pvd->vg_name)
		pv->vg_name = fmt->orphan_vg_name;
	else if (!(pv->vg_name = dm_pool_strdup(mem, (char *)pvd->vg_name))) {
		log_error("Volume Group name allocation failed.");
		return 0;
	}

	memcpy(&pv->vgid, vgd->vg_uuid, sizeof(vg->id));

	/* Store system_id from first PV if PV belongs to a VG */
	if (vg && !*vg->system_id)
		strncpy(vg->system_id, (char *)pvd->system_id, NAME_LEN);

	if (vg &&
	    strncmp(vg->system_id, (char *)pvd->system_id, sizeof(pvd->system_id)))
		    log_very_verbose("System ID %s on %s differs from %s for "
				     "volume group", pvd->system_id,
				     pv_dev_name(pv), vg->system_id);

	/*
	 * If exported, we still need to flag in pv->status too because
	 * we don't always have a struct volume_group when we need this.
	 */
	if (pvd->pv_status & VG_EXPORTED)
		pv->status |= EXPORTED_VG;

	if (pvd->pv_allocatable)
		pv->status |= ALLOCATABLE_PV;

	pv->size = pvd->pv_size;
	pv->pe_size = pvd->pe_size;
	pv->pe_start = pvd->pe_start;
	pv->pe_count = pvd->pe_total;
	pv->pe_alloc_count = 0;
	pv->pe_align = 0;

	/* Fix up pv size if missing or impossibly large */
	if (!pv->size || pv->size > (1ULL << 62)) {
		if (!dev_get_size(dev, &pv->size)) {
			log_error("%s: Couldn't get size.", pv_dev_name(pv));
			return 0;
		}
		log_verbose("Fixing up missing format1 size (%s) "
			    "for PV %s", display_size(fmt->cmd, pv->size),
			    pv_dev_name(pv));
		if (vg) {
			size = pv->pe_count * (uint64_t) vg->extent_size +
			       pv->pe_start;
			if (size > pv->size)
				log_error("WARNING: Physical Volume %s is too "
					  "large for underlying device",
					  pv_dev_name(pv));
		}
	}

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	if (!alloc_pv_segment_whole_pv(mem, pv))
		return_0;

	return 1;
}

static int _system_id(struct cmd_context *cmd, char *s, const char *prefix)
{

	if (dm_snprintf(s, NAME_LEN, "%s%s%lu",
			 prefix, cmd->hostname, time(NULL)) < 0) {
		log_error("Generated system_id too long");
		return 0;
	}

	return 1;
}

int export_pv(struct cmd_context *cmd, struct dm_pool *mem __attribute((unused)),
	      struct volume_group *vg,
	      struct pv_disk *pvd, struct physical_volume *pv)
{
	memset(pvd, 0, sizeof(*pvd));

	pvd->id[0] = 'H';
	pvd->id[1] = 'M';
	pvd->version = 1;

	memcpy(pvd->pv_uuid, pv->id.uuid, ID_LEN);

	if (pv->vg_name && !is_orphan(pv)) {
		if (!_check_vg_name(pv->vg_name))
			return_0;
		strncpy((char *)pvd->vg_name, pv->vg_name, sizeof(pvd->vg_name));
	}

	/* Preserve existing system_id if it exists */
	if (vg && *vg->system_id)
		strncpy((char *)pvd->system_id, vg->system_id, sizeof(pvd->system_id));

	/* Is VG already exported or being exported? */
	if (vg && vg_is_exported(vg)) {
		/* Does system_id need setting? */
		if (!*vg->system_id ||
		    strncmp(vg->system_id, EXPORTED_TAG,
			    sizeof(EXPORTED_TAG) - 1)) {
			if (!_system_id(cmd, (char *)pvd->system_id, EXPORTED_TAG))
				return_0;
		}
		if (strlen((char *)pvd->vg_name) + sizeof(EXPORTED_TAG) >
		    sizeof(pvd->vg_name)) {
			log_error("Volume group name %s too long to export",
				  pvd->vg_name);
			return 0;
		}
		strcat((char *)pvd->vg_name, EXPORTED_TAG);
	}

	/* Is VG being imported? */
	if (vg && !vg_is_exported(vg) && *vg->system_id &&
	    !strncmp(vg->system_id, EXPORTED_TAG, sizeof(EXPORTED_TAG) - 1)) {
		if (!_system_id(cmd, (char *)pvd->system_id, IMPORTED_TAG))
			return_0;
	}

	/* Generate system_id if PV is in VG */
	if (!pvd->system_id || !*pvd->system_id)
		if (!_system_id(cmd, (char *)pvd->system_id, ""))
			return_0;

	/* Update internal system_id if we changed it */
	if (vg &&
	    (!*vg->system_id ||
	     strncmp(vg->system_id, (char *)pvd->system_id, sizeof(pvd->system_id))))
		    strncpy(vg->system_id, (char *)pvd->system_id, NAME_LEN);

	//pvd->pv_major = MAJOR(pv->dev);

	if (pv->status & ALLOCATABLE_PV)
		pvd->pv_allocatable = PV_ALLOCATABLE;

	pvd->pv_size = pv->size;
	pvd->lv_cur = 0;	/* this is set when exporting the lv list */
	if (vg)
		pvd->pe_size = vg->extent_size;
	else
		pvd->pe_size = pv->pe_size;
	pvd->pe_total = pv->pe_count;
	pvd->pe_allocated = pv->pe_alloc_count;
	pvd->pe_start = pv->pe_start;

	return 1;
}

int import_vg(struct dm_pool *mem,
	      struct volume_group *vg, struct disk_list *dl)
{
	struct vg_disk *vgd = &dl->vgd;
	memcpy(vg->id.uuid, vgd->vg_uuid, ID_LEN);

	if (!_check_vg_name((char *)dl->pvd.vg_name))
		return_0;

	if (!(vg->name = dm_pool_strdup(mem, (char *)dl->pvd.vg_name)))
		return_0;

	if (!(vg->system_id = dm_pool_alloc(mem, NAME_LEN)))
		return_0;

	*vg->system_id = '\0';

	if (vgd->vg_status & VG_EXPORTED)
		vg->status |= EXPORTED_VG;

	if (vgd->vg_status & VG_EXTENDABLE)
		vg->status |= RESIZEABLE_VG;

	if (vgd->vg_access & VG_READ)
		vg->status |= LVM_READ;

	if (vgd->vg_access & VG_WRITE)
		vg->status |= LVM_WRITE;

	if (vgd->vg_access & VG_CLUSTERED)
		vg->status |= CLUSTERED;

	if (vgd->vg_access & VG_SHARED)
		vg->status |= SHARED;

	vg->extent_size = vgd->pe_size;
	vg->extent_count = vgd->pe_total;
	vg->free_count = vgd->pe_total;
	vg->max_lv = vgd->lv_max;
	vg->max_pv = vgd->pv_max;
	vg->alloc = ALLOC_NORMAL;

	return 1;
}

int export_vg(struct vg_disk *vgd, struct volume_group *vg)
{
	memset(vgd, 0, sizeof(*vgd));
	memcpy(vgd->vg_uuid, vg->id.uuid, ID_LEN);

	if (vg->status & LVM_READ)
		vgd->vg_access |= VG_READ;

	if (vg->status & LVM_WRITE)
		vgd->vg_access |= VG_WRITE;

	if (vg_is_clustered(vg))
		vgd->vg_access |= VG_CLUSTERED;

	if (vg->status & SHARED)
		vgd->vg_access |= VG_SHARED;

	if (vg_is_exported(vg))
		vgd->vg_status |= VG_EXPORTED;

	if (vg_is_resizeable(vg))
		vgd->vg_status |= VG_EXTENDABLE;

	vgd->lv_max = vg->max_lv;
	vgd->lv_cur = vg_visible_lvs(vg) + snapshot_count(vg);

	vgd->pv_max = vg->max_pv;
	vgd->pv_cur = vg->pv_count;

	vgd->pe_size = vg->extent_size;
	vgd->pe_total = vg->extent_count;
	vgd->pe_allocated = vg->extent_count - vg->free_count;

	return 1;
}

int import_lv(struct cmd_context *cmd, struct dm_pool *mem,
	      struct logical_volume *lv, struct lv_disk *lvd)
{
	if (!(lv->name = _create_lv_name(mem, (char *)lvd->lv_name)))
		return_0;

	lv->status |= VISIBLE_LV;

	if (lvd->lv_status & LV_SPINDOWN)
		lv->status |= SPINDOWN_LV;

	if (lvd->lv_status & LV_PERSISTENT_MINOR) {
		lv->status |= FIXED_MINOR;
		lv->minor = MINOR(lvd->lv_dev);
		lv->major = MAJOR(lvd->lv_dev);
	} else {
		lv->major = -1;
		lv->minor = -1;
	}

	if (lvd->lv_access & LV_READ)
		lv->status |= LVM_READ;

	if (lvd->lv_access & LV_WRITE)
		lv->status |= LVM_WRITE;

	if (lvd->lv_badblock)
		lv->status |= BADBLOCK_ON;

	/* Drop the unused LV_STRICT here */
	if (lvd->lv_allocation & LV_CONTIGUOUS)
		lv->alloc = ALLOC_CONTIGUOUS;
	else
		lv->alloc = ALLOC_NORMAL;

	if (!lvd->lv_read_ahead)
		lv->read_ahead = cmd->default_settings.read_ahead;
	else
		lv->read_ahead = lvd->lv_read_ahead;

	lv->size = lvd->lv_size;
	lv->le_count = lvd->lv_allocated_le;

	return 1;
}

static void _export_lv(struct lv_disk *lvd, struct volume_group *vg,
		       struct logical_volume *lv, const char *dev_dir)
{
	memset(lvd, 0, sizeof(*lvd));
	snprintf((char *)lvd->lv_name, sizeof(lvd->lv_name), "%s%s/%s",
		 dev_dir, vg->name, lv->name);

	strcpy((char *)lvd->vg_name, vg->name);

	if (lv->status & LVM_READ)
		lvd->lv_access |= LV_READ;

	if (lv->status & LVM_WRITE)
		lvd->lv_access |= LV_WRITE;

	if (lv->status & SPINDOWN_LV)
		lvd->lv_status |= LV_SPINDOWN;

	if (lv->status & FIXED_MINOR) {
		lvd->lv_status |= LV_PERSISTENT_MINOR;
		lvd->lv_dev = MKDEV(lv->major, lv->minor);
	} else {
		lvd->lv_dev = MKDEV(LVM_BLK_MAJOR, lvnum_from_lvid(&lv->lvid));
	}

	if (lv->read_ahead == DM_READ_AHEAD_AUTO ||
	    lv->read_ahead == DM_READ_AHEAD_NONE)
		lvd->lv_read_ahead = 0;
	else
		lvd->lv_read_ahead = lv->read_ahead;

	lvd->lv_stripes =
	    dm_list_item(lv->segments.n, struct lv_segment)->area_count;
	lvd->lv_stripesize =
	    dm_list_item(lv->segments.n, struct lv_segment)->stripe_size;

	lvd->lv_size = lv->size;
	lvd->lv_allocated_le = lv->le_count;

	if (lv->status & BADBLOCK_ON)
		lvd->lv_badblock = LV_BADBLOCK_ON;

	if (lv->alloc == ALLOC_CONTIGUOUS)
		lvd->lv_allocation |= LV_CONTIGUOUS;
}

int export_extents(struct disk_list *dl, uint32_t lv_num,
		   struct logical_volume *lv, struct physical_volume *pv)
{
	struct pe_disk *ped;
	struct lv_segment *seg;
	uint32_t pe, s;

	dm_list_iterate_items(seg, &lv->segments) {
		for (s = 0; s < seg->area_count; s++) {
			if (!(seg->segtype->flags & SEG_FORMAT1_SUPPORT)) {
				log_error("Segment type %s in LV %s: "
					  "unsupported by format1",
					  seg->segtype->name, lv->name);
				return 0;
			}
			if (seg_type(seg, s) != AREA_PV) {
				log_error("Non-PV stripe found in LV %s: "
					  "unsupported by format1", lv->name);
				return 0;
			}
			if (seg_pv(seg, s) != pv)
				continue;	/* not our pv */

			for (pe = 0; pe < (seg->len / seg->area_count); pe++) {
				ped = &dl->extents[pe + seg_pe(seg, s)];
				ped->lv_num = lv_num;
				ped->le_num = (seg->le / seg->area_count) + pe +
				    s * (lv->le_count / seg->area_count);
			}
		}
	}

	return 1;
}

int import_pvs(const struct format_type *fmt, struct dm_pool *mem,
	       struct volume_group *vg,
	       struct dm_list *pvds, struct dm_list *results, uint32_t *count)
{
	struct disk_list *dl;
	struct pv_list *pvl;

	*count = 0;
	dm_list_iterate_items(dl, pvds) {
		if (!(pvl = dm_pool_zalloc(mem, sizeof(*pvl))) ||
		    !(pvl->pv = dm_pool_alloc(mem, sizeof(*pvl->pv))))
			return_0;

		if (!import_pv(fmt, mem, dl->dev, vg, pvl->pv, &dl->pvd, &dl->vgd))
			return_0;

		pvl->pv->fmt = fmt;
		dm_list_add(results, &pvl->list);
		(*count)++;
	}

	return 1;
}

static struct logical_volume *_add_lv(struct dm_pool *mem,
				      struct volume_group *vg,
				      struct lv_disk *lvd)
{
	struct logical_volume *lv;

	if (!(lv = alloc_lv(mem)))
		return_NULL;

	lvid_from_lvnum(&lv->lvid, &vg->id, lvd->lv_number);

	if (!import_lv(vg->cmd, mem, lv, lvd)) 
		goto_bad;

	if (!link_lv_to_vg(vg, lv))
		goto_bad;

	return lv;
bad:
	dm_pool_free(mem, lv);
	return NULL;
}

int import_lvs(struct dm_pool *mem, struct volume_group *vg, struct dm_list *pvds)
{
	struct disk_list *dl;
	struct lvd_list *ll;
	struct lv_disk *lvd;

	dm_list_iterate_items(dl, pvds) {
		dm_list_iterate_items(ll, &dl->lvds) {
			lvd = &ll->lvd;

			if (!find_lv(vg, (char *)lvd->lv_name) &&
			    !_add_lv(mem, vg, lvd))
				return_0;
		}
	}

	return 1;
}

/* FIXME: tidy */
int export_lvs(struct disk_list *dl, struct volume_group *vg,
	       struct physical_volume *pv, const char *dev_dir)
{
	int r = 0;
	struct lv_list *ll;
	struct lvd_list *lvdl;
	size_t len;
	uint32_t lv_num;
	struct dm_hash_table *lvd_hash;

	if (!_check_vg_name(vg->name))
		return_0;

	if (!(lvd_hash = dm_hash_create(32)))
		return_0;

	/*
	 * setup the pv's extents array
	 */
	len = sizeof(struct pe_disk) * dl->pvd.pe_total;
	if (!(dl->extents = dm_pool_alloc(dl->mem, len)))
		goto_out;
	memset(dl->extents, 0, len);

	dm_list_iterate_items(ll, &vg->lvs) {
		if (ll->lv->status & SNAPSHOT)
			continue;

		if (!(lvdl = dm_pool_alloc(dl->mem, sizeof(*lvdl))))
			goto_out;

		_export_lv(&lvdl->lvd, vg, ll->lv, dev_dir);

		lv_num = lvnum_from_lvid(&ll->lv->lvid);
		lvdl->lvd.lv_number = lv_num;

		if (!dm_hash_insert(lvd_hash, ll->lv->name, &lvdl->lvd))
			goto_out;

		if (!export_extents(dl, lv_num + 1, ll->lv, pv))
			goto_out;

		if (lv_is_origin(ll->lv))
			lvdl->lvd.lv_access |= LV_SNAPSHOT_ORG;

		if (lv_is_cow(ll->lv)) {
			lvdl->lvd.lv_access |= LV_SNAPSHOT;
			lvdl->lvd.lv_chunk_size = ll->lv->snapshot->chunk_size;
			lvdl->lvd.lv_snapshot_minor =
			    lvnum_from_lvid(&ll->lv->snapshot->origin->lvid);
		}

		dm_list_add(&dl->lvds, &lvdl->list);
		dl->pvd.lv_cur++;
	}

	r = 1;

      out:
	dm_hash_destroy(lvd_hash);
	return r;
}

/*
 * FIXME: More inefficient code.
 */
int import_snapshots(struct dm_pool *mem __attribute((unused)), struct volume_group *vg,
		     struct dm_list *pvds)
{
	struct logical_volume *lvs[MAX_LV];
	struct disk_list *dl;
	struct lvd_list *ll;
	struct lv_disk *lvd;
	int lvnum;
	struct logical_volume *org, *cow;

	/* build an index of lv numbers */
	memset(lvs, 0, sizeof(lvs));
	dm_list_iterate_items(dl, pvds) {
		dm_list_iterate_items(ll, &dl->lvds) {
			lvd = &ll->lvd;

			lvnum = lvd->lv_number;

			if (lvnum >= MAX_LV) {
				log_error("Logical volume number "
					  "out of bounds.");
				return 0;
			}

			if (!lvs[lvnum] &&
			    !(lvs[lvnum] = find_lv(vg, (char *)lvd->lv_name))) {
				log_error("Couldn't find logical volume '%s'.",
					  lvd->lv_name);
				return 0;
			}
		}
	}

	/*
	 * Now iterate through yet again adding the snapshots.
	 */
	dm_list_iterate_items(dl, pvds) {
		dm_list_iterate_items(ll, &dl->lvds) {
			lvd = &ll->lvd;

			if (!(lvd->lv_access & LV_SNAPSHOT))
				continue;

			lvnum = lvd->lv_number;
			cow = lvs[lvnum];
			if (!(org = lvs[lvd->lv_snapshot_minor])) {
				log_error("Couldn't find origin logical volume "
					  "for snapshot '%s'.", lvd->lv_name);
				return 0;
			}

			/* we may have already added this snapshot */
			if (lv_is_cow(cow))
				continue;

			/* insert the snapshot */
			if (!vg_add_snapshot(org, cow, NULL,
					     org->le_count,
					     lvd->lv_chunk_size)) {
				log_error("Couldn't add snapshot.");
				return 0;
			}
		}
	}

	return 1;
}

int export_uuids(struct disk_list *dl, struct volume_group *vg)
{
	struct uuid_list *ul;
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (!(ul = dm_pool_alloc(dl->mem, sizeof(*ul))))
			return_0;

		memset(ul->uuid, 0, sizeof(ul->uuid));
		memcpy(ul->uuid, pvl->pv->id.uuid, ID_LEN);

		dm_list_add(&dl->uuids, &ul->list);
	}
	return 1;
}

/*
 * This calculates the nasty pv_number field
 * used by LVM1.
 */
void export_numbers(struct dm_list *pvds, struct volume_group *vg __attribute((unused)))
{
	struct disk_list *dl;
	int pv_num = 1;

	dm_list_iterate_items(dl, pvds)
		dl->pvd.pv_number = pv_num++;
}

/*
 * Calculate vg_disk->pv_act.
 */
void export_pv_act(struct dm_list *pvds)
{
	struct disk_list *dl;
	int act = 0;

	dm_list_iterate_items(dl, pvds)
		if (dl->pvd.pv_status & PV_ACTIVE)
			act++;

	dm_list_iterate_items(dl, pvds)
		dl->vgd.pv_act = act;
}

int export_vg_number(struct format_instance *fid, struct dm_list *pvds,
		     const char *vg_name, struct dev_filter *filter)
{
	struct disk_list *dl;
	int vg_num;

	if (!get_free_vg_number(fid, filter, vg_name, &vg_num))
		return_0;

	dm_list_iterate_items(dl, pvds)
		dl->vgd.vg_number = vg_num;

	return 1;
}
