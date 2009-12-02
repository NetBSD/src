/*	$NetBSD: format_pool.c,v 1.1.1.2 2009/12/02 00:26:50 haad Exp $	*/

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
#include "limits.h"
#include "display.h"
#include "toolcontext.h"
#include "lvmcache.h"
#include "disk_rep.h"
#include "format_pool.h"
#include "pool_label.h"

/* Must be called after pvs are imported */
static struct user_subpool *_build_usp(struct dm_list *pls, struct dm_pool *mem,
				       int *sps)
{
	struct pool_list *pl;
	struct user_subpool *usp = NULL, *cur_sp = NULL;
	struct user_device *cur_dev = NULL;

	/*
	 * FIXME: Need to do some checks here - I'm tempted to add a
	 * user_pool structure and build the entire thing to check against.
	 */
	dm_list_iterate_items(pl, pls) {
		*sps = pl->pd.pl_subpools;
		if (!usp && (!(usp = dm_pool_zalloc(mem, sizeof(*usp) * (*sps))))) {
			log_error("Unable to allocate %d subpool structures",
				  *sps);
			return 0;
		}

		if (cur_sp != &usp[pl->pd.pl_sp_id]) {
			cur_sp = &usp[pl->pd.pl_sp_id];

			cur_sp->id = pl->pd.pl_sp_id;
			cur_sp->striping = pl->pd.pl_striping;
			cur_sp->num_devs = pl->pd.pl_sp_devs;
			cur_sp->type = pl->pd.pl_sp_type;
			cur_sp->initialized = 1;
		}

		if (!cur_sp->devs &&
		    (!(cur_sp->devs =
		       dm_pool_zalloc(mem,
				   sizeof(*usp->devs) * pl->pd.pl_sp_devs)))) {

			log_error("Unable to allocate %d pool_device "
				  "structures", pl->pd.pl_sp_devs);
			return 0;
		}

		cur_dev = &cur_sp->devs[pl->pd.pl_sp_devid];
		cur_dev->sp_id = cur_sp->id;
		cur_dev->devid = pl->pd.pl_sp_id;
		cur_dev->blocks = pl->pd.pl_blocks;
		cur_dev->pv = pl->pv;
		cur_dev->initialized = 1;
	}

	return usp;
}

static int _check_usp(char *vgname, struct user_subpool *usp, int sp_count)
{
	int i;
	unsigned j;

	for (i = 0; i < sp_count; i++) {
		if (!usp[i].initialized) {
			log_error("Missing subpool %d in pool %s", i, vgname);
			return 0;
		}
		for (j = 0; j < usp[i].num_devs; j++) {
			if (!usp[i].devs[j].initialized) {
				log_error("Missing device %u for subpool %d"
					  " in pool %s", j, i, vgname);
				return 0;
			}

		}
	}

	return 1;
}

static struct volume_group *_build_vg_from_pds(struct format_instance
					       *fid, struct dm_pool *mem,
					       struct dm_list *pds)
{
	struct dm_pool *smem = fid->fmt->cmd->mem;
	struct volume_group *vg = NULL;
	struct user_subpool *usp = NULL;
	int sp_count;

	if (!(vg = dm_pool_zalloc(smem, sizeof(*vg)))) {
		log_error("Unable to allocate volume group structure");
		return NULL;
	}

	vg->cmd = fid->fmt->cmd;
	vg->vgmem = mem;
	vg->fid = fid;
	vg->name = NULL;
	vg->status = 0;
	vg->extent_count = 0;
	vg->pv_count = 0;
	vg->seqno = 1;
	vg->system_id = NULL;
	dm_list_init(&vg->pvs);
	dm_list_init(&vg->lvs);
	dm_list_init(&vg->tags);
	dm_list_init(&vg->removed_pvs);

	if (!import_pool_vg(vg, smem, pds))
		return_NULL;

	if (!import_pool_pvs(fid->fmt, vg, &vg->pvs, smem, pds))
		return_NULL;

	if (!import_pool_lvs(vg, smem, pds))
		return_NULL;

	/*
	 * I need an intermediate subpool structure that contains all the
	 * relevant info for this.  Then i can iterate through the subpool
	 * structures for checking, and create the segments
	 */
	if (!(usp = _build_usp(pds, mem, &sp_count)))
		return_NULL;

	/*
	 * check the subpool structures - we can't handle partial VGs in
	 * the pool format, so this will error out if we're missing PVs
	 */
	if (!_check_usp(vg->name, usp, sp_count))
		return_NULL;

	if (!import_pool_segments(&vg->lvs, smem, usp, sp_count))
		return_NULL;

	return vg;
}

static struct volume_group *_pool_vg_read(struct format_instance *fid,
				     const char *vg_name,
				     struct metadata_area *mda __attribute((unused)))
{
	struct dm_pool *mem = dm_pool_create("pool vg_read", VG_MEMPOOL_CHUNK);
	struct dm_list pds;
	struct volume_group *vg = NULL;

	dm_list_init(&pds);

	/* We can safely ignore the mda passed in */

	if (!mem)
		return_NULL;

	/* Strip dev_dir if present */
	vg_name = strip_dir(vg_name, fid->fmt->cmd->dev_dir);

	/* Read all the pvs in the vg */
	if (!read_pool_pds(fid->fmt, vg_name, mem, &pds))
		goto_out;

	/* Do the rest of the vg stuff */
	if (!(vg = _build_vg_from_pds(fid, mem, &pds)))
		goto_out;

	return vg;
out:
	dm_pool_destroy(mem);
	return NULL;
}

static int _pool_pv_setup(const struct format_type *fmt __attribute((unused)),
			  uint64_t pe_start __attribute((unused)),
			  uint32_t extent_count __attribute((unused)),
			  uint32_t extent_size __attribute((unused)),
			  unsigned long data_alignment __attribute((unused)),
			  unsigned long data_alignment_offset __attribute((unused)),
			  int pvmetadatacopies __attribute((unused)),
			  uint64_t pvmetadatasize __attribute((unused)),
			  struct dm_list *mdas __attribute((unused)),
			  struct physical_volume *pv __attribute((unused)),
			  struct volume_group *vg __attribute((unused)))
{
	return 1;
}

static int _pool_pv_read(const struct format_type *fmt, const char *pv_name,
			 struct physical_volume *pv,
			 struct dm_list *mdas __attribute((unused)),
			 int scan_label_only __attribute((unused)))
{
	struct dm_pool *mem = dm_pool_create("pool pv_read", 1024);
	struct pool_list *pl;
	struct device *dev;
	int r = 0;

	log_very_verbose("Reading physical volume data %s from disk", pv_name);

	if (!mem)
		return_0;

	if (!(dev = dev_cache_get(pv_name, fmt->cmd->filter)))
		goto_out;

	/*
	 * I need to read the disk and populate a pv structure here
	 * I'll probably need to abstract some of this later for the
	 * vg_read code
	 */
	if (!(pl = read_pool_disk(fmt, dev, mem, NULL)))
		goto_out;

	if (!import_pool_pv(fmt, fmt->cmd->mem, NULL, pv, pl))
		goto_out;

	pv->fmt = fmt;

	r = 1;

      out:
	dm_pool_destroy(mem);
	return r;
}

/* *INDENT-OFF* */
static struct metadata_area_ops _metadata_format_pool_ops = {
	.vg_read = _pool_vg_read,
};
/* *INDENT-ON* */

static struct format_instance *_pool_create_instance(const struct format_type *fmt,
						const char *vgname __attribute((unused)),
						const char *vgid __attribute((unused)),
						void *private __attribute((unused)))
{
	struct format_instance *fid;
	struct metadata_area *mda;

	if (!(fid = dm_pool_zalloc(fmt->cmd->mem, sizeof(*fid)))) {
		log_error("Unable to allocate format instance structure for "
			  "pool format");
		return NULL;
	}

	fid->fmt = fmt;
	dm_list_init(&fid->metadata_areas);

	/* Define a NULL metadata area */
	if (!(mda = dm_pool_zalloc(fmt->cmd->mem, sizeof(*mda)))) {
		log_error("Unable to allocate metadata area structure "
			  "for pool format");
		dm_pool_free(fmt->cmd->mem, fid);
		return NULL;
	}

	mda->ops = &_metadata_format_pool_ops;
	mda->metadata_locn = NULL;
	dm_list_add(&fid->metadata_areas, &mda->list);

	return fid;
}

static void _pool_destroy_instance(struct format_instance *fid __attribute((unused)))
{
	return;
}

static void _pool_destroy(const struct format_type *fmt)
{
	dm_free((void *) fmt);
}

/* *INDENT-OFF* */
static struct format_handler _format_pool_ops = {
	.pv_read = _pool_pv_read,
	.pv_setup = _pool_pv_setup,
	.create_instance = _pool_create_instance,
	.destroy_instance = _pool_destroy_instance,
	.destroy = _pool_destroy,
};
/* *INDENT-ON */

#ifdef POOL_INTERNAL
struct format_type *init_pool_format(struct cmd_context *cmd)
#else				/* Shared */
struct format_type *init_format(struct cmd_context *cmd);
struct format_type *init_format(struct cmd_context *cmd)
#endif
{
	struct format_type *fmt = dm_malloc(sizeof(*fmt));

	if (!fmt) {
		log_error("Unable to allocate format type structure for pool "
			  "format");
		return NULL;
	}

	fmt->cmd = cmd;
	fmt->ops = &_format_pool_ops;
	fmt->name = FMT_POOL_NAME;
	fmt->alias = NULL;
	fmt->orphan_vg_name = FMT_POOL_ORPHAN_VG_NAME;
	fmt->features = 0;
	fmt->private = NULL;

	if (!(fmt->labeller = pool_labeller_create(fmt))) {
		log_error("Couldn't create pool label handler.");
		return NULL;
	}

	if (!(label_register_handler(FMT_POOL_NAME, fmt->labeller))) {
		log_error("Couldn't register pool label handler.");
		return NULL;
	}

	log_very_verbose("Initialised format: %s", fmt->name);

	return fmt;
}
