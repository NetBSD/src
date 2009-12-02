/*	$NetBSD: import_export.c,v 1.1.1.2 2009/12/02 00:26:50 haad Exp $	*/

/*
 * Copyright (C) 1997-2004 Sistina Software, Inc. All rights reserved.
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

#include "lib.h"
#include "label.h"
#include "metadata.h"
#include "lvmcache.h"
#include "disk_rep.h"
#include "sptype_names.h"
#include "lv_alloc.h"
#include "pv_alloc.h"
#include "str_list.h"
#include "display.h"
#include "segtype.h"
#include "toolcontext.h"

/* This file contains only imports at the moment... */

int import_pool_vg(struct volume_group *vg, struct dm_pool *mem, struct dm_list *pls)
{
	struct pool_list *pl;

	dm_list_iterate_items(pl, pls) {
		vg->extent_count +=
		    ((pl->pd.pl_blocks) / POOL_PE_SIZE);

		vg->free_count = vg->extent_count;
		vg->pv_count++;

		if (vg->name)
			continue;

		vg->name = dm_pool_strdup(mem, pl->pd.pl_pool_name);
		get_pool_vg_uuid(&vg->id, &pl->pd);
		vg->extent_size = POOL_PE_SIZE;
		vg->status |= LVM_READ | LVM_WRITE | CLUSTERED | SHARED;
		vg->max_lv = 1;
		vg->max_pv = POOL_MAX_DEVICES;
		vg->alloc = ALLOC_NORMAL;
	}

	return 1;
}

int import_pool_lvs(struct volume_group *vg, struct dm_pool *mem, struct dm_list *pls)
{
	struct pool_list *pl;
	struct logical_volume *lv;

	if (!(lv = alloc_lv(mem)))
		return_0;

	lv->status = 0;
	lv->alloc = ALLOC_NORMAL;
	lv->size = 0;
	lv->name = NULL;
	lv->le_count = 0;
	lv->read_ahead = vg->cmd->default_settings.read_ahead;

	dm_list_iterate_items(pl, pls) {
		lv->size += pl->pd.pl_blocks;

		if (lv->name)
			continue;

		if (!(lv->name = dm_pool_strdup(mem, pl->pd.pl_pool_name)))
			return_0;

		get_pool_lv_uuid(lv->lvid.id, &pl->pd);
		log_debug("Calculated lv uuid for lv %s: %s", lv->name,
			  lv->lvid.s);

		lv->status |= VISIBLE_LV | LVM_READ | LVM_WRITE;
		lv->major = POOL_MAJOR;

		/* for pool a minor of 0 is dynamic */
		if (pl->pd.pl_minor) {
			lv->status |= FIXED_MINOR;
			lv->minor = pl->pd.pl_minor + MINOR_OFFSET;
		} else {
			lv->minor = -1;
		}
	}

	lv->le_count = lv->size / POOL_PE_SIZE;

	return link_lv_to_vg(vg, lv);
}

int import_pool_pvs(const struct format_type *fmt, struct volume_group *vg,
		    struct dm_list *pvs, struct dm_pool *mem, struct dm_list *pls)
{
	struct pv_list *pvl;
	struct pool_list *pl;

	dm_list_iterate_items(pl, pls) {
		if (!(pvl = dm_pool_zalloc(mem, sizeof(*pvl)))) {
			log_error("Unable to allocate pv list structure");
			return 0;
		}
		if (!(pvl->pv = dm_pool_zalloc(mem, sizeof(*pvl->pv)))) {
			log_error("Unable to allocate pv structure");
			return 0;
		}
		if (!import_pool_pv(fmt, mem, vg, pvl->pv, pl)) {
			return 0;
		}
		pl->pv = pvl->pv;
		pvl->mdas = NULL;
		pvl->pe_ranges = NULL;
		dm_list_add(pvs, &pvl->list);
	}

	return 1;
}

int import_pool_pv(const struct format_type *fmt, struct dm_pool *mem,
		   struct volume_group *vg, struct physical_volume *pv,
		   struct pool_list *pl)
{
	struct pool_disk *pd = &pl->pd;

	memset(pv, 0, sizeof(*pv));

	get_pool_pv_uuid(&pv->id, pd);
	pv->fmt = fmt;

	pv->dev = pl->dev;
	if (!(pv->vg_name = dm_pool_strdup(mem, pd->pl_pool_name))) {
		log_error("Unable to duplicate vg_name string");
		return 0;
	}
	if (vg != NULL)
		memcpy(&pv->vgid, &vg->id, sizeof(vg->id));
	pv->status = 0;
	pv->size = pd->pl_blocks;
	pv->pe_size = POOL_PE_SIZE;
	pv->pe_start = POOL_PE_START;
	pv->pe_count = pv->size / POOL_PE_SIZE;
	pv->pe_alloc_count = 0;
	pv->pe_align = 0;

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	if (!alloc_pv_segment_whole_pv(mem, pv))
		return_0;

	return 1;
}

static const char *_cvt_sptype(uint32_t sptype)
{
	int i;
	for (i = 0; sptype_names[i].name[0]; i++) {
		if (sptype == sptype_names[i].label) {
			break;
		}
	}
	log_debug("Found sptype %X and converted it to %s",
		  sptype, sptype_names[i].name);
	return sptype_names[i].name;
}

static int _add_stripe_seg(struct dm_pool *mem,
			   struct user_subpool *usp, struct logical_volume *lv,
			   uint32_t *le_cur)
{
	struct lv_segment *seg;
	struct segment_type *segtype;
	unsigned j;
	uint32_t area_len;

	if (usp->striping & (usp->striping - 1)) {
		log_error("Stripe size must be a power of 2");
		return 0;
	}

	area_len = (usp->devs[0].blocks) / POOL_PE_SIZE;

	if (!(segtype = get_segtype_from_string(lv->vg->cmd,
						     "striped")))
		return_0;

	if (!(seg = alloc_lv_segment(mem, segtype, lv, *le_cur,
				     area_len * usp->num_devs, 0,
				     usp->striping, NULL, usp->num_devs,
				     area_len, 0, 0, 0))) {
		log_error("Unable to allocate striped lv_segment structure");
		return 0;
	}

	for (j = 0; j < usp->num_devs; j++)
		if (!set_lv_segment_area_pv(seg, j, usp->devs[j].pv, 0))
			return_0;

	/* add the subpool type to the segment tag list */
	str_list_add(mem, &seg->tags, _cvt_sptype(usp->type));

	dm_list_add(&lv->segments, &seg->list);

	*le_cur += seg->len;

	return 1;
}

static int _add_linear_seg(struct dm_pool *mem,
			   struct user_subpool *usp, struct logical_volume *lv,
			   uint32_t *le_cur)
{
	struct lv_segment *seg;
	struct segment_type *segtype;
	unsigned j;
	uint32_t area_len;

	if (!(segtype = get_segtype_from_string(lv->vg->cmd, "striped")))
		return_0;

	for (j = 0; j < usp->num_devs; j++) {
		area_len = (usp->devs[j].blocks) / POOL_PE_SIZE;

		if (!(seg = alloc_lv_segment(mem, segtype, lv, *le_cur,
					     area_len, 0, usp->striping,
					     NULL, 1, area_len,
					     POOL_PE_SIZE, 0, 0))) {
			log_error("Unable to allocate linear lv_segment "
				  "structure");
			return 0;
		}

		/* add the subpool type to the segment tag list */
		str_list_add(mem, &seg->tags, _cvt_sptype(usp->type));

		if (!set_lv_segment_area_pv(seg, 0, usp->devs[j].pv, 0))
			return_0;
		dm_list_add(&lv->segments, &seg->list);

		*le_cur += seg->len;
	}

	return 1;
}

int import_pool_segments(struct dm_list *lvs, struct dm_pool *mem,
			 struct user_subpool *usp, int subpools)
{
	struct lv_list *lvl;
	struct logical_volume *lv;
	uint32_t le_cur = 0;
	int i;

	dm_list_iterate_items(lvl, lvs) {
		lv = lvl->lv;

		if (lv->status & SNAPSHOT)
			continue;

		for (i = 0; i < subpools; i++) {
			if (usp[i].striping) {
				if (!_add_stripe_seg(mem, &usp[i], lv, &le_cur))
					return_0;
			} else {
				if (!_add_linear_seg(mem, &usp[i], lv, &le_cur))
					return_0;
			}
		}
	}

	return 1;
}
