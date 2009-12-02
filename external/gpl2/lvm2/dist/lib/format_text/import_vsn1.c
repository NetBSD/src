/*	$NetBSD: import_vsn1.c,v 1.1.1.2 2009/12/02 00:26:30 haad Exp $	*/

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
#include "metadata.h"
#include "import-export.h"
#include "display.h"
#include "toolcontext.h"
#include "lvmcache.h"
#include "lv_alloc.h"
#include "pv_alloc.h"
#include "segtype.h"
#include "text_import.h"

typedef int (*section_fn) (struct format_instance * fid, struct dm_pool * mem,
			   struct volume_group * vg, struct config_node * pvn,
			   struct config_node * vgn,
			   struct dm_hash_table * pv_hash);

#define _read_int32(root, path, result) \
	get_config_uint32(root, path, (uint32_t *) result)

#define _read_uint32(root, path, result) \
	get_config_uint32(root, path, result)

#define _read_int64(root, path, result) \
	get_config_uint64(root, path, result)

/*
 * Logs an attempt to read an invalid format file.
 */
static void _invalid_format(const char *str)
{
	log_error("Can't process text format file - %s.", str);
}

/*
 * Checks that the config file contains vg metadata, and that it
 * we recognise the version number,
 */
static int _check_version(struct config_tree *cft)
{
	struct config_node *cn;
	struct config_value *cv;

	/*
	 * Check the contents field.
	 */
	if (!(cn = find_config_node(cft->root, CONTENTS_FIELD))) {
		_invalid_format("missing contents field");
		return 0;
	}

	cv = cn->v;
	if (!cv || cv->type != CFG_STRING || strcmp(cv->v.str, CONTENTS_VALUE)) {
		_invalid_format("unrecognised contents field");
		return 0;
	}

	/*
	 * Check the version number.
	 */
	if (!(cn = find_config_node(cft->root, FORMAT_VERSION_FIELD))) {
		_invalid_format("missing version number");
		return 0;
	}

	cv = cn->v;
	if (!cv || cv->type != CFG_INT || cv->v.i != FORMAT_VERSION_VALUE) {
		_invalid_format("unrecognised version number");
		return 0;
	}

	return 1;
}

static int _is_converting(struct logical_volume *lv)
{
	struct lv_segment *seg;

	if (lv->status & MIRRORED) {
		seg = first_seg(lv);
		/* Can't use is_temporary_mirror() because the metadata for
		 * seg_lv may not be read in and flags may not be set yet. */
		if (seg_type(seg, 0) == AREA_LV &&
		    strstr(seg_lv(seg, 0)->name, MIRROR_SYNC_LAYER))
			return 1;
	}

	return 0;
}

static int _read_id(struct id *id, struct config_node *cn, const char *path)
{
	struct config_value *cv;

	if (!(cn = find_config_node(cn, path))) {
		log_error("Couldn't find uuid.");
		return 0;
	}

	cv = cn->v;
	if (!cv || !cv->v.str) {
		log_error("uuid must be a string.");
		return 0;
	}

	if (!id_read_format(id, cv->v.str)) {
		log_error("Invalid uuid.");
		return 0;
	}

	return 1;
}

static int _read_flag_config(struct config_node *n, uint32_t *status, int type)
{
	struct config_node *cn;
	*status = 0;

	if (!(cn = find_config_node(n, "status"))) {
		log_error("Could not find status flags.");
		return 0;
	}

	if (!(read_flags(status, type | STATUS_FLAG, cn->v))) {
		log_error("Could not read status flags.");
		return 0;
	}

	if ((cn = find_config_node(n, "flags"))) {
		if (!(read_flags(status, type, cn->v))) {
			log_error("Could not read flags.");
			return 0;
		}
	}

	return 1;
}

static int _read_pv(struct format_instance *fid, struct dm_pool *mem,
		    struct volume_group *vg, struct config_node *pvn,
		    struct config_node *vgn __attribute((unused)),
		    struct dm_hash_table *pv_hash)
{
	struct physical_volume *pv;
	struct pv_list *pvl;
	struct config_node *cn;
	uint64_t size;

	if (!(pvl = dm_pool_zalloc(mem, sizeof(*pvl))) ||
	    !(pvl->pv = dm_pool_zalloc(mem, sizeof(*pvl->pv))))
		return_0;

	pv = pvl->pv;

	/*
	 * Add the pv to the pv hash for quick lookup when we read
	 * the lv segments.
	 */
	if (!dm_hash_insert(pv_hash, pvn->key, pv))
		return_0;

	if (!(pvn = pvn->child)) {
		log_error("Empty pv section.");
		return 0;
	}

	if (!_read_id(&pv->id, pvn, "id")) {
		log_error("Couldn't read uuid for physical volume.");
		return 0;
	}

	/*
	 * Convert the uuid into a device.
	 */
	if (!(pv->dev = device_from_pvid(fid->fmt->cmd, &pv->id))) {
		char buffer[64] __attribute((aligned(8)));

		if (!id_write_format(&pv->id, buffer, sizeof(buffer)))
			log_error("Couldn't find device.");
		else
			log_error("Couldn't find device with uuid '%s'.",
				  buffer);
	}

	if (!(pv->vg_name = dm_pool_strdup(mem, vg->name)))
		return_0;

	memcpy(&pv->vgid, &vg->id, sizeof(vg->id));

	if (!_read_flag_config(pvn, &pv->status, PV_FLAGS)) {
		log_error("Couldn't read status flags for physical volume.");
		return 0;
	}

	if (!pv->dev)
		pv->status |= MISSING_PV;

	/* Late addition */
	_read_int64(pvn, "dev_size", &pv->size);

	if (!_read_int64(pvn, "pe_start", &pv->pe_start)) {
		log_error("Couldn't read extent size for physical volume.");
		return 0;
	}

	if (!_read_int32(pvn, "pe_count", &pv->pe_count)) {
		log_error("Couldn't find extent count (pe_count) for "
			  "physical volume.");
		return 0;
	}

	dm_list_init(&pv->tags);
	dm_list_init(&pv->segments);

	/* Optional tags */
	if ((cn = find_config_node(pvn, "tags")) &&
	    !(read_tags(mem, &pv->tags, cn->v))) {
		log_error("Couldn't read tags for physical volume %s in %s.",
			  pv_dev_name(pv), vg->name);
		return 0;
	}

	/* adjust the volume group. */
	vg->extent_count += pv->pe_count;
	vg->free_count += pv->pe_count;

	pv->pe_size = vg->extent_size;

	pv->pe_alloc_count = 0;
	pv->pe_align = 0;
	pv->fmt = fid->fmt;

	/* Fix up pv size if missing or impossibly large */
	if ((!pv->size || pv->size > (1ULL << 62)) && pv->dev) {
		if (!dev_get_size(pv->dev, &pv->size)) {
			log_error("%s: Couldn't get size.", pv_dev_name(pv));
			return 0;
		}
		log_verbose("Fixing up missing size (%s) "
			    "for PV %s", display_size(fid->fmt->cmd, pv->size),
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

	if (!alloc_pv_segment_whole_pv(mem, pv))
		return_0;

	vg->pv_count++;
	dm_list_add(&vg->pvs, &pvl->list);

	return 1;
}

static void _insert_segment(struct logical_volume *lv, struct lv_segment *seg)
{
	struct lv_segment *comp;

	dm_list_iterate_items(comp, &lv->segments) {
		if (comp->le > seg->le) {
			dm_list_add(&comp->list, &seg->list);
			return;
		}
	}

	lv->le_count += seg->len;
	dm_list_add(&lv->segments, &seg->list);
}

static int _read_segment(struct dm_pool *mem, struct volume_group *vg,
			 struct logical_volume *lv, struct config_node *sn,
			 struct dm_hash_table *pv_hash)
{
	uint32_t area_count = 0u;
	struct lv_segment *seg;
	struct config_node *cn, *sn_child = sn->child;
	struct config_value *cv;
	uint32_t start_extent, extent_count;
	struct segment_type *segtype;
	const char *segtype_str;

	if (!sn_child) {
		log_error("Empty segment section.");
		return 0;
	}

	if (!_read_int32(sn_child, "start_extent", &start_extent)) {
		log_error("Couldn't read 'start_extent' for segment '%s' "
			  "of logical volume %s.", sn->key, lv->name);
		return 0;
	}

	if (!_read_int32(sn_child, "extent_count", &extent_count)) {
		log_error("Couldn't read 'extent_count' for segment '%s' "
			  "of logical volume %s.", sn->key, lv->name);
		return 0;
	}

	segtype_str = "striped";

	if ((cn = find_config_node(sn_child, "type"))) {
		cv = cn->v;
		if (!cv || !cv->v.str) {
			log_error("Segment type must be a string.");
			return 0;
		}
		segtype_str = cv->v.str;
	}

	if (!(segtype = get_segtype_from_string(vg->cmd, segtype_str)))
		return_0;

	if (segtype->ops->text_import_area_count &&
	    !segtype->ops->text_import_area_count(sn_child, &area_count))
		return_0;

	if (!(seg = alloc_lv_segment(mem, segtype, lv, start_extent,
				     extent_count, 0, 0, NULL, area_count,
				     extent_count, 0, 0, 0))) {
		log_error("Segment allocation failed");
		return 0;
	}

	if (seg->segtype->ops->text_import &&
	    !seg->segtype->ops->text_import(seg, sn_child, pv_hash))
		return_0;

	/* Optional tags */
	if ((cn = find_config_node(sn_child, "tags")) &&
	    !(read_tags(mem, &seg->tags, cn->v))) {
		log_error("Couldn't read tags for a segment of %s/%s.",
			  vg->name, lv->name);
		return 0;
	}

	/*
	 * Insert into correct part of segment list.
	 */
	_insert_segment(lv, seg);

	if (seg_is_mirrored(seg))
		lv->status |= MIRRORED;

	if (seg_is_virtual(seg))
		lv->status |= VIRTUAL;

	if (_is_converting(lv))
		lv->status |= CONVERTING;

	return 1;
}

int text_import_areas(struct lv_segment *seg, const struct config_node *sn,
		      const struct config_node *cn, struct dm_hash_table *pv_hash,
		      uint32_t flags)
{
	unsigned int s;
	struct config_value *cv;
	struct logical_volume *lv1;
	struct physical_volume *pv;
	const char *seg_name = config_parent_name(sn);

	if (!seg->area_count) {
		log_error("Zero areas not allowed for segment %s", seg_name);
		return 0;
	}

	for (cv = cn->v, s = 0; cv && s < seg->area_count; s++, cv = cv->next) {

		/* first we read the pv */
		if (cv->type != CFG_STRING) {
			log_error("Bad volume name in areas array for segment %s.", seg_name);
			return 0;
		}

		if (!cv->next) {
			log_error("Missing offset in areas array for segment %s.", seg_name);
			return 0;
		}

		if (cv->next->type != CFG_INT) {
			log_error("Bad offset in areas array for segment %s.", seg_name);
			return 0;
		}

		/* FIXME Cope if LV not yet read in */
		if ((pv = dm_hash_lookup(pv_hash, cv->v.str))) {
			if (!set_lv_segment_area_pv(seg, s, pv, (uint32_t) cv->next->v.i))
				return_0;
		} else if ((lv1 = find_lv(seg->lv->vg, cv->v.str))) {
			if (!set_lv_segment_area_lv(seg, s, lv1,
						    (uint32_t) cv->next->v.i,
						    flags))
				return_0;
		} else {
			log_error("Couldn't find volume '%s' "
				  "for segment '%s'.",
				  cv->v.str ? : "NULL", seg_name);
			return 0;
		}

		cv = cv->next;
	}

	/*
	 * Check we read the correct number of stripes.
	 */
	if (cv || (s < seg->area_count)) {
		log_error("Incorrect number of areas in area array "
			  "for segment '%s'.", seg_name);
		return 0;
	}

	return 1;
}

static int _read_segments(struct dm_pool *mem, struct volume_group *vg,
			  struct logical_volume *lv, struct config_node *lvn,
			  struct dm_hash_table *pv_hash)
{
	struct config_node *sn;
	int count = 0, seg_count;

	for (sn = lvn; sn; sn = sn->sib) {

		/*
		 * All sub-sections are assumed to be segments.
		 */
		if (!sn->v) {
			if (!_read_segment(mem, vg, lv, sn, pv_hash))
				return_0;

			count++;
		}
		/* FIXME Remove this restriction */
		if ((lv->status & SNAPSHOT) && count > 1) {
			log_error("Only one segment permitted for snapshot");
			return 0;
		}
	}

	if (!_read_int32(lvn, "segment_count", &seg_count)) {
		log_error("Couldn't read segment count for logical volume %s.",
			  lv->name);
		return 0;
	}

	if (seg_count != count) {
		log_error("segment_count and actual number of segments "
			  "disagree for logical volume %s.", lv->name);
		return 0;
	}

	/*
	 * Check there are no gaps or overlaps in the lv.
	 */
	if (!check_lv_segments(lv, 0))
		return_0;

	/*
	 * Merge segments in case someones been editing things by hand.
	 */
	if (!lv_merge_segments(lv))
		return_0;

	return 1;
}

static int _read_lvnames(struct format_instance *fid __attribute((unused)),
			 struct dm_pool *mem,
			 struct volume_group *vg, struct config_node *lvn,
			 struct config_node *vgn __attribute((unused)),
			 struct dm_hash_table *pv_hash __attribute((unused)))
{
	struct logical_volume *lv;
	struct config_node *cn;

	if (!(lv = alloc_lv(mem)))
		return_0;

	if (!(lv->name = dm_pool_strdup(mem, lvn->key)))
		return_0;

	if (!(lvn = lvn->child)) {
		log_error("Empty logical volume section.");
		return 0;
	}

	if (!_read_flag_config(lvn, &lv->status, LV_FLAGS)) {
		log_error("Couldn't read status flags for logical volume %s.",
			  lv->name);
		return 0;
	}

	lv->alloc = ALLOC_INHERIT;
	if ((cn = find_config_node(lvn, "allocation_policy"))) {
		struct config_value *cv = cn->v;
		if (!cv || !cv->v.str) {
			log_error("allocation_policy must be a string.");
			return 0;
		}

		lv->alloc = get_alloc_from_string(cv->v.str);
		if (lv->alloc == ALLOC_INVALID)
			return_0;
	}

	if (!_read_int32(lvn, "read_ahead", &lv->read_ahead))
		/* If not present, choice of auto or none is configurable */
		lv->read_ahead = vg->cmd->default_settings.read_ahead;
	else {
		switch (lv->read_ahead) {
		case 0:
			lv->read_ahead = DM_READ_AHEAD_AUTO;
			break;
		case (uint32_t) -1:
			lv->read_ahead = DM_READ_AHEAD_NONE;
			break;
		default:
			;
		}
	}

	/* Optional tags */
	if ((cn = find_config_node(lvn, "tags")) &&
	    !(read_tags(mem, &lv->tags, cn->v))) {
		log_error("Couldn't read tags for logical volume %s/%s.",
			  vg->name, lv->name);
		return 0;
	}

	return link_lv_to_vg(vg, lv);
}

static int _read_lvsegs(struct format_instance *fid __attribute((unused)),
			struct dm_pool *mem,
			struct volume_group *vg, struct config_node *lvn,
			struct config_node *vgn __attribute((unused)),
			struct dm_hash_table *pv_hash)
{
	struct logical_volume *lv;
	struct lv_list *lvl;

	if (!(lvl = find_lv_in_vg(vg, lvn->key))) {
		log_error("Lost logical volume reference %s", lvn->key);
		return 0;
	}

	lv = lvl->lv;

	if (!(lvn = lvn->child)) {
		log_error("Empty logical volume section.");
		return 0;
	}

	/* FIXME: read full lvid */
	if (!_read_id(&lv->lvid.id[1], lvn, "id")) {
		log_error("Couldn't read uuid for logical volume %s.",
			  lv->name);
		return 0;
	}

	memcpy(&lv->lvid.id[0], &lv->vg->id, sizeof(lv->lvid.id[0]));

	if (!_read_segments(mem, vg, lv, lvn, pv_hash))
		return_0;

	lv->size = (uint64_t) lv->le_count * (uint64_t) vg->extent_size;

	lv->minor = -1;
	if ((lv->status & FIXED_MINOR) &&
	    !_read_int32(lvn, "minor", &lv->minor)) {
		log_error("Couldn't read minor number for logical "
			  "volume %s.", lv->name);
		return 0;
	}

	lv->major = -1;
	if ((lv->status & FIXED_MINOR) &&
	    !_read_int32(lvn, "major", &lv->major)) {
		log_error("Couldn't read major number for logical "
			  "volume %s.", lv->name);
	}

	return 1;
}

static int _read_sections(struct format_instance *fid,
			  const char *section, section_fn fn,
			  struct dm_pool *mem,
			  struct volume_group *vg, struct config_node *vgn,
			  struct dm_hash_table *pv_hash, int optional)
{
	struct config_node *n;

	if (!(n = find_config_node(vgn, section))) {
		if (!optional) {
			log_error("Couldn't find section '%s'.", section);
			return 0;
		}

		return 1;
	}

	for (n = n->child; n; n = n->sib) {
		if (!fn(fid, mem, vg, n, vgn, pv_hash))
			return_0;
	}

	return 1;
}

static struct volume_group *_read_vg(struct format_instance *fid,
				     struct config_tree *cft)
{
	struct config_node *vgn, *cn;
	struct volume_group *vg;
	struct dm_hash_table *pv_hash = NULL;
	struct dm_pool *mem = dm_pool_create("lvm2 vg_read", VG_MEMPOOL_CHUNK);

	if (!mem)
		return_NULL;

	/* skip any top-level values */
	for (vgn = cft->root; (vgn && vgn->v); vgn = vgn->sib) ;

	if (!vgn) {
		log_error("Couldn't find volume group in file.");
		goto bad;
	}

	if (!(vg = dm_pool_zalloc(mem, sizeof(*vg))))
		goto_bad;

	vg->vgmem = mem;
	vg->cmd = fid->fmt->cmd;

	/* FIXME Determine format type from file contents */
	/* eg Set to instance of fmt1 here if reading a format1 backup? */
	vg->fid = fid;

	if (!(vg->name = dm_pool_strdup(mem, vgn->key)))
		goto_bad;

	if (!(vg->system_id = dm_pool_zalloc(mem, NAME_LEN)))
		goto_bad;

	vgn = vgn->child;

	if ((cn = find_config_node(vgn, "system_id")) && cn->v) {
		if (!cn->v->v.str) {
			log_error("system_id must be a string");
			goto bad;
		}
		strncpy(vg->system_id, cn->v->v.str, NAME_LEN);
	}

	if (!_read_id(&vg->id, vgn, "id")) {
		log_error("Couldn't read uuid for volume group %s.", vg->name);
		goto bad;
	}

	if (!_read_int32(vgn, "seqno", &vg->seqno)) {
		log_error("Couldn't read 'seqno' for volume group %s.",
			  vg->name);
		goto bad;
	}

	if (!_read_flag_config(vgn, &vg->status, VG_FLAGS)) {
		log_error("Error reading flags of volume group %s.",
			  vg->name);
		goto bad;
	}

	if (!_read_int32(vgn, "extent_size", &vg->extent_size)) {
		log_error("Couldn't read extent size for volume group %s.",
			  vg->name);
		goto bad;
	}

	/*
	 * 'extent_count' and 'free_count' get filled in
	 * implicitly when reading in the pv's and lv's.
	 */

	if (!_read_int32(vgn, "max_lv", &vg->max_lv)) {
		log_error("Couldn't read 'max_lv' for volume group %s.",
			  vg->name);
		goto bad;
	}

	if (!_read_int32(vgn, "max_pv", &vg->max_pv)) {
		log_error("Couldn't read 'max_pv' for volume group %s.",
			  vg->name);
		goto bad;
	}

	vg->alloc = ALLOC_NORMAL;
	if ((cn = find_config_node(vgn, "allocation_policy"))) {
		struct config_value *cv = cn->v;
		if (!cv || !cv->v.str) {
			log_error("allocation_policy must be a string.");
			return 0;
		}

		vg->alloc = get_alloc_from_string(cv->v.str);
		if (vg->alloc == ALLOC_INVALID)
			return_0;
	}

	/*
	 * The pv hash memoises the pv section names -> pv
	 * structures.
	 */
	if (!(pv_hash = dm_hash_create(32))) {
		log_error("Couldn't create hash table.");
		goto bad;
	}

	dm_list_init(&vg->pvs);
	if (!_read_sections(fid, "physical_volumes", _read_pv, mem, vg,
			    vgn, pv_hash, 0)) {
		log_error("Couldn't find all physical volumes for volume "
			  "group %s.", vg->name);
		goto bad;
	}

	dm_list_init(&vg->lvs);
	dm_list_init(&vg->tags);
	dm_list_init(&vg->removed_pvs);

	/* Optional tags */
	if ((cn = find_config_node(vgn, "tags")) &&
	    !(read_tags(mem, &vg->tags, cn->v))) {
		log_error("Couldn't read tags for volume group %s.", vg->name);
		goto bad;
	}

	if (!_read_sections(fid, "logical_volumes", _read_lvnames, mem, vg,
			    vgn, pv_hash, 1)) {
		log_error("Couldn't read all logical volume names for volume "
			  "group %s.", vg->name);
		goto bad;
	}

	if (!_read_sections(fid, "logical_volumes", _read_lvsegs, mem, vg,
			    vgn, pv_hash, 1)) {
		log_error("Couldn't read all logical volumes for "
			  "volume group %s.", vg->name);
		goto bad;
	}

	if (!fixup_imported_mirrors(vg)) {
		log_error("Failed to fixup mirror pointers after import for "
			  "volume group %s.", vg->name);
		goto bad;
	}

	dm_hash_destroy(pv_hash);

	/*
	 * Finished.
	 */
	return vg;

      bad:
	if (pv_hash)
		dm_hash_destroy(pv_hash);

	dm_pool_destroy(mem);
	return NULL;
}

static void _read_desc(struct dm_pool *mem,
		       struct config_tree *cft, time_t *when, char **desc)
{
	const char *d;
	unsigned int u = 0u;
	int old_suppress;

	old_suppress = log_suppress(1);
	d = find_config_str(cft->root, "description", "");
	log_suppress(old_suppress);
	*desc = dm_pool_strdup(mem, d);

	get_config_uint32(cft->root, "creation_time", &u);
	*when = u;
}

static const char *_read_vgname(const struct format_type *fmt,
				struct config_tree *cft, struct id *vgid,
				uint32_t *vgstatus, char **creation_host)
{
	struct config_node *vgn;
	struct dm_pool *mem = fmt->cmd->mem;
	char *vgname;
	int old_suppress;

	old_suppress = log_suppress(2);
	*creation_host = dm_pool_strdup(mem,
					find_config_str(cft->root,
							"creation_host", ""));
	log_suppress(old_suppress);

	/* skip any top-level values */
	for (vgn = cft->root; (vgn && vgn->v); vgn = vgn->sib) ;

	if (!vgn) {
		log_error("Couldn't find volume group in file.");
		return 0;
	}

	if (!(vgname = dm_pool_strdup(mem, vgn->key)))
		return_0;

	vgn = vgn->child;

	if (!_read_id(vgid, vgn, "id")) {
		log_error("Couldn't read uuid for volume group %s.", vgname);
		return 0;
	}

	if (!_read_flag_config(vgn, vgstatus, VG_FLAGS)) {
		log_error("Couldn't find status flags for volume group %s.",
			  vgname);
		return 0;
	}

	return vgname;
}

static struct text_vg_version_ops _vsn1_ops = {
	.check_version = _check_version,
	.read_vg = _read_vg,
	.read_desc = _read_desc,
	.read_vgname = _read_vgname,
};

struct text_vg_version_ops *text_vg_vsn1_init(void)
{
	return &_vsn1_ops;
}
