/*	$NetBSD: format-text.c,v 1.1.1.3 2009/12/02 00:26:32 haad Exp $	*/

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
#include "format-text.h"
#include "import-export.h"
#include "device.h"
#include "lvm-file.h"
#include "config.h"
#include "display.h"
#include "toolcontext.h"
#include "lvm-string.h"
#include "uuid.h"
#include "layout.h"
#include "crc.h"
#include "xlate.h"
#include "label.h"
#include "memlock.h"
#include "lvmcache.h"

#include <unistd.h>
#include <sys/file.h>
#include <sys/param.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>

static struct mda_header *_raw_read_mda_header(const struct format_type *fmt,
					       struct device_area *dev_area);

static struct format_instance *_text_create_text_instance(const struct format_type
						     *fmt, const char *vgname,
						     const char *vgid,
						     void *context);

struct text_fid_context {
	char *raw_metadata_buf;
	uint32_t raw_metadata_buf_size;
};

struct dir_list {
	struct dm_list list;
	char dir[0];
};

struct raw_list {
	struct dm_list list;
	struct device_area dev_area;
};

struct text_context {
	char *path_live;	/* Path to file holding live metadata */
	char *path_edit;	/* Path to file holding edited metadata */
	char *desc;		/* Description placed inside file */
};

/*
 * NOTE: Currently there can be only one vg per text file.
 */

static int _text_vg_setup(struct format_instance *fid __attribute((unused)),
			  struct volume_group *vg)
{
	if (vg->extent_size & (vg->extent_size - 1)) {
		log_error("Extent size must be power of 2");
		return 0;
	}

	return 1;
}

static uint64_t _mda_free_sectors_raw(struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;

	return mdac->free_sectors;
}

static uint64_t _mda_total_sectors_raw(struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;

	return mdac->area.size >> SECTOR_SHIFT;
}

/*
 * Check if metadata area belongs to vg
 */
static int _mda_in_vg_raw(struct format_instance *fid __attribute((unused)),
			     struct volume_group *vg, struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs)
		if (pvl->pv->dev == mdac->area.dev)
			return 1;

	return 0;
}

/*
 * For circular region between region_start and region_start + region_size,
 * back up one SECTOR_SIZE from 'region_ptr' and return the value.
 * This allows reverse traversal through text metadata area to find old
 * metadata.
 *
 * Parameters:
 *   region_start: start of the region (bytes)
 *   region_size: size of the region (bytes)
 *   region_ptr: pointer within the region (bytes)
 *   NOTE: region_start <= region_ptr <= region_start + region_size
 */
static uint64_t _get_prev_sector_circular(uint64_t region_start,
					  uint64_t region_size,
					  uint64_t region_ptr)
{
	if (region_ptr >= region_start + SECTOR_SIZE)
		return region_ptr - SECTOR_SIZE;
	else
		return (region_start + region_size - SECTOR_SIZE);
}

/*
 * Analyze a metadata area for old metadata records in the circular buffer.
 * This function just looks through and makes a first pass at the data in
 * the sectors for particular things.
 * FIXME: do something with each metadata area (try to extract vg, write
 * raw data to file, etc)
 */
static int _pv_analyze_mda_raw (const struct format_type * fmt,
				struct metadata_area *mda)
{
	struct mda_header *mdah;
	struct raw_locn *rlocn;
	uint64_t area_start;
	uint64_t area_size;
	uint64_t prev_sector, prev_sector2;
	uint64_t latest_mrec_offset;
	int i;
	uint64_t offset;
	uint64_t offset2;
	size_t size;
	size_t size2;
	char *buf=NULL;
	struct device_area *area;
	struct mda_context *mdac;
	int r=0;

	mdac = (struct mda_context *) mda->metadata_locn;

	log_print("Found text metadata area: offset=%" PRIu64 ", size=%"
		  PRIu64, mdac->area.start, mdac->area.size);
	area = &mdac->area;

	if (!dev_open(area->dev))
		return_0;

	if (!(mdah = _raw_read_mda_header(fmt, area)))
		goto_out;

	rlocn = mdah->raw_locns;

	/*
	 * The device area includes the metadata header as well as the
	 * records, so remove the metadata header from the start and size
	 */
	area_start = area->start + MDA_HEADER_SIZE;
	area_size = area->size - MDA_HEADER_SIZE;
	latest_mrec_offset = rlocn->offset + area->start;

	/*
	 * Start searching at rlocn (point of live metadata) and go
	 * backwards.
	 */
	prev_sector = _get_prev_sector_circular(area_start, area_size,
					       latest_mrec_offset);
	offset = prev_sector;
	size = SECTOR_SIZE;
	offset2 = size2 = 0;
	i = 0;
	while (prev_sector != latest_mrec_offset) {
		prev_sector2 = prev_sector;
		prev_sector = _get_prev_sector_circular(area_start, area_size,
							prev_sector);
		if (prev_sector > prev_sector2)
			goto_out;
		/*
		 * FIXME: for some reason, the whole metadata region from
		 * area->start to area->start+area->size is not used.
		 * Only ~32KB seems to contain valid metadata records
		 * (LVM2 format - format_text).  As a result, I end up with
		 * "maybe_config_section" returning true when there's no valid
		 * metadata in a sector (sectors with all nulls).
		 */
		if (!(buf = dm_pool_alloc(fmt->cmd->mem, size + size2)))
			goto_out;

		if (!dev_read_circular(area->dev, offset, size,
				       offset2, size2, buf))
			goto_out;

		/*
		 * FIXME: We could add more sophisticated metadata detection
		 */
		if (maybe_config_section(buf, size + size2)) {
			/* FIXME: Validate region, pull out timestamp?, etc */
			/* FIXME: Do something with this region */
			log_verbose ("Found LVM2 metadata record at "
				     "offset=%"PRIu64", size=%"PRIsize_t", "
				     "offset2=%"PRIu64" size2=%"PRIsize_t,
				     offset, size, offset2, size2);
			offset = prev_sector;
			size = SECTOR_SIZE;
			offset2 = size2 = 0;
		} else {
			/*
			 * Not a complete metadata record, assume we have
			 * metadata and just increase the size and offset.
			 * Start the second region if the previous sector is
			 * wrapping around towards the end of the disk.
			 */
			if (prev_sector > offset) {
				offset2 = prev_sector;
				size2 += SECTOR_SIZE;
			} else {
				offset = prev_sector;
				size += SECTOR_SIZE;
			}
		}
		dm_pool_free(fmt->cmd->mem, buf);
		buf = NULL;
	}

	r = 1;
 out:
	if (buf)
		dm_pool_free(fmt->cmd->mem, buf);
	if (!dev_close(area->dev))
		stack;
	return r;
}



static int _text_lv_setup(struct format_instance *fid __attribute((unused)),
			  struct logical_volume *lv)
{
/******** FIXME Any LV size restriction?
	uint64_t max_size = UINT_MAX;

	if (lv->size > max_size) {
		char *dummy = display_size(max_size);
		log_error("logical volumes cannot be larger than %s", dummy);
		dm_free(dummy);
		return 0;
	}
*/

	if (!*lv->lvid.s && !lvid_create(&lv->lvid, &lv->vg->id)) {
		log_error("Random lvid creation failed for %s/%s.",
			  lv->vg->name, lv->name);
		return 0;
	}

	return 1;
}

static void _xlate_mdah(struct mda_header *mdah)
{
	struct raw_locn *rl;

	mdah->version = xlate32(mdah->version);
	mdah->start = xlate64(mdah->start);
	mdah->size = xlate64(mdah->size);

	rl = &mdah->raw_locns[0];
	while (rl->offset) {
		rl->checksum = xlate32(rl->checksum);
		rl->offset = xlate64(rl->offset);
		rl->size = xlate64(rl->size);
		rl++;
	}
}

static struct mda_header *_raw_read_mda_header(const struct format_type *fmt,
					       struct device_area *dev_area)
{
	struct mda_header *mdah;

	if (!(mdah = dm_pool_alloc(fmt->cmd->mem, MDA_HEADER_SIZE))) {
		log_error("struct mda_header allocation failed");
		return NULL;
	}

	if (!dev_read(dev_area->dev, dev_area->start, MDA_HEADER_SIZE, mdah))
		goto_bad;

	if (mdah->checksum_xl != xlate32(calc_crc(INITIAL_CRC, mdah->magic,
						  MDA_HEADER_SIZE -
						  sizeof(mdah->checksum_xl)))) {
		log_error("Incorrect metadata area header checksum");
		goto bad;
	}

	_xlate_mdah(mdah);

	if (strncmp((char *)mdah->magic, FMTT_MAGIC, sizeof(mdah->magic))) {
		log_error("Wrong magic number in metadata area header");
		goto bad;
	}

	if (mdah->version != FMTT_VERSION) {
		log_error("Incompatible metadata area header version: %d",
			  mdah->version);
		goto bad;
	}

	if (mdah->start != dev_area->start) {
		log_error("Incorrect start sector in metadata area header: %"
			  PRIu64, mdah->start);
		goto bad;
	}

	return mdah;

bad:
	dm_pool_free(fmt->cmd->mem, mdah);
	return NULL;
}

static int _raw_write_mda_header(const struct format_type *fmt,
				 struct device *dev,
				 uint64_t start_byte, struct mda_header *mdah)
{
	strncpy((char *)mdah->magic, FMTT_MAGIC, sizeof(mdah->magic));
	mdah->version = FMTT_VERSION;
	mdah->start = start_byte;

	_xlate_mdah(mdah);
	mdah->checksum_xl = xlate32(calc_crc(INITIAL_CRC, mdah->magic,
					     MDA_HEADER_SIZE -
					     sizeof(mdah->checksum_xl)));

	if (!dev_write(dev, start_byte, MDA_HEADER_SIZE, mdah))
		return_0;

	return 1;
}

static struct raw_locn *_find_vg_rlocn(struct device_area *dev_area,
				       struct mda_header *mdah,
				       const char *vgname,
				       int *precommitted)
{
	size_t len;
	char vgnamebuf[NAME_LEN + 2] __attribute((aligned(8)));
	struct raw_locn *rlocn, *rlocn_precommitted;
	struct lvmcache_info *info;

	rlocn = mdah->raw_locns;	/* Slot 0 */
	rlocn_precommitted = rlocn + 1;	/* Slot 1 */

	/* Should we use precommitted metadata? */
	if (*precommitted && rlocn_precommitted->size &&
	    (rlocn_precommitted->offset != rlocn->offset)) {
		rlocn = rlocn_precommitted;
	} else
		*precommitted = 0;

	/* FIXME Loop through rlocns two-at-a-time.  List null-terminated. */
	/* FIXME Ignore if checksum incorrect!!! */
	if (!dev_read(dev_area->dev, dev_area->start + rlocn->offset,
		      sizeof(vgnamebuf), vgnamebuf))
		goto_bad;

	if (!strncmp(vgnamebuf, vgname, len = strlen(vgname)) &&
	    (isspace(vgnamebuf[len]) || vgnamebuf[len] == '{')) {
		return rlocn;
	}

      bad:
	if ((info = info_from_pvid(dev_area->dev->pvid, 0)))
		lvmcache_update_vgname_and_id(info, FMT_TEXT_ORPHAN_VG_NAME,
					      FMT_TEXT_ORPHAN_VG_NAME, 0, NULL);

	return NULL;
}

/*
 * Determine offset for uncommitted metadata
 */
static uint64_t _next_rlocn_offset(struct raw_locn *rlocn,
				   struct mda_header *mdah)
{
	if (!rlocn)
		/* Find an empty slot */
		/* FIXME Assume only one VG per mdah for now */
		return MDA_HEADER_SIZE;

	/* Start of free space - round up to next sector; circular */
	return ((rlocn->offset + rlocn->size +
		(SECTOR_SIZE - rlocn->size % SECTOR_SIZE) -
		MDA_HEADER_SIZE) % (mdah->size - MDA_HEADER_SIZE))
	       + MDA_HEADER_SIZE;
}

static int _raw_holds_vgname(struct format_instance *fid,
			     struct device_area *dev_area, const char *vgname)
{
	int r = 0;
	int noprecommit = 0;
	struct mda_header *mdah;

	if (!dev_open(dev_area->dev))
		return_0;

	if (!(mdah = _raw_read_mda_header(fid->fmt, dev_area)))
		return_0;

	if (_find_vg_rlocn(dev_area, mdah, vgname, &noprecommit))
		r = 1;

	if (!dev_close(dev_area->dev))
		stack;

	return r;
}

static struct volume_group *_vg_read_raw_area(struct format_instance *fid,
					      const char *vgname,
					      struct device_area *area,
					      int precommitted)
{
	struct volume_group *vg = NULL;
	struct raw_locn *rlocn;
	struct mda_header *mdah;
	time_t when;
	char *desc;
	uint32_t wrap = 0;

	if (!dev_open(area->dev))
		return_NULL;

	if (!(mdah = _raw_read_mda_header(fid->fmt, area)))
		goto_out;

	if (!(rlocn = _find_vg_rlocn(area, mdah, vgname, &precommitted))) {
		log_debug("VG %s not found on %s", vgname, dev_name(area->dev));
		goto out;
	}

	if (rlocn->offset + rlocn->size > mdah->size)
		wrap = (uint32_t) ((rlocn->offset + rlocn->size) - mdah->size);

	if (wrap > rlocn->offset) {
		log_error("VG %s metadata too large for circular buffer",
			  vg->name);
		goto out;
	}

	/* FIXME 64-bit */
	if (!(vg = text_vg_import_fd(fid, NULL, area->dev,
				     (off_t) (area->start + rlocn->offset),
				     (uint32_t) (rlocn->size - wrap),
				     (off_t) (area->start + MDA_HEADER_SIZE),
				     wrap, calc_crc, rlocn->checksum, &when,
				     &desc)))
		goto_out;
	log_debug("Read %s %smetadata (%u) from %s at %" PRIu64 " size %"
		  PRIu64, vg->name, precommitted ? "pre-commit " : "",
		  vg->seqno, dev_name(area->dev),
		  area->start + rlocn->offset, rlocn->size);

	if (precommitted)
		vg->status |= PRECOMMITTED;

      out:
	if (!dev_close(area->dev))
		stack;

	return vg;
}

static struct volume_group *_vg_read_raw(struct format_instance *fid,
					 const char *vgname,
					 struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;

	return _vg_read_raw_area(fid, vgname, &mdac->area, 0);
}

static struct volume_group *_vg_read_precommit_raw(struct format_instance *fid,
						   const char *vgname,
						   struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;

	return _vg_read_raw_area(fid, vgname, &mdac->area, 1);
}

static int _vg_write_raw(struct format_instance *fid, struct volume_group *vg,
			 struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;
	struct text_fid_context *fidtc = (struct text_fid_context *) fid->private;
	struct raw_locn *rlocn;
	struct mda_header *mdah;
	struct pv_list *pvl;
	int r = 0;
       uint64_t new_wrap = 0, old_wrap = 0, new_end;
	int found = 0;
	int noprecommit = 0;

	/* Ignore any mda on a PV outside the VG. vgsplit relies on this */
	dm_list_iterate_items(pvl, &vg->pvs) {
		if (pvl->pv->dev == mdac->area.dev) {
			found = 1;
			break;
		}
	}

	if (!found)
		return 1;

	if (!dev_open(mdac->area.dev))
		return_0;

	if (!(mdah = _raw_read_mda_header(fid->fmt, &mdac->area)))
		goto_out;

	rlocn = _find_vg_rlocn(&mdac->area, mdah, vg->name, &noprecommit);
	mdac->rlocn.offset = _next_rlocn_offset(rlocn, mdah);

	if (!fidtc->raw_metadata_buf &&
	    !(fidtc->raw_metadata_buf_size =
			text_vg_export_raw(vg, "", &fidtc->raw_metadata_buf))) {
		log_error("VG %s metadata writing failed", vg->name);
		goto out;
	}

	mdac->rlocn.size = fidtc->raw_metadata_buf_size;

	if (mdac->rlocn.offset + mdac->rlocn.size > mdah->size)
		new_wrap = (mdac->rlocn.offset + mdac->rlocn.size) - mdah->size;

	if (rlocn && (rlocn->offset + rlocn->size > mdah->size))
		old_wrap = (rlocn->offset + rlocn->size) - mdah->size;

	new_end = new_wrap ? new_wrap + MDA_HEADER_SIZE :
			    mdac->rlocn.offset + mdac->rlocn.size;

	if ((new_wrap && old_wrap) ||
	    (rlocn && (new_wrap || old_wrap) && (new_end > rlocn->offset)) ||
	    (mdac->rlocn.size >= mdah->size)) {
		log_error("VG %s metadata too large for circular buffer",
			  vg->name);
		goto out;
	}

	log_debug("Writing %s metadata to %s at %" PRIu64 " len %" PRIu64,
		  vg->name, dev_name(mdac->area.dev), mdac->area.start +
		  mdac->rlocn.offset, mdac->rlocn.size - new_wrap);

	/* Write text out, circularly */
	if (!dev_write(mdac->area.dev, mdac->area.start + mdac->rlocn.offset,
		       (size_t) (mdac->rlocn.size - new_wrap),
		       fidtc->raw_metadata_buf))
		goto_out;

	if (new_wrap) {
               log_debug("Writing metadata to %s at %" PRIu64 " len %" PRIu64,
			  dev_name(mdac->area.dev), mdac->area.start +
			  MDA_HEADER_SIZE, new_wrap);

		if (!dev_write(mdac->area.dev,
			       mdac->area.start + MDA_HEADER_SIZE,
			       (size_t) new_wrap,
			       fidtc->raw_metadata_buf +
			       mdac->rlocn.size - new_wrap))
			goto_out;
	}

	mdac->rlocn.checksum = calc_crc(INITIAL_CRC, fidtc->raw_metadata_buf,
					(uint32_t) (mdac->rlocn.size -
						    new_wrap));
	if (new_wrap)
		mdac->rlocn.checksum = calc_crc(mdac->rlocn.checksum,
						fidtc->raw_metadata_buf +
						mdac->rlocn.size -
						new_wrap, (uint32_t) new_wrap);

	r = 1;

      out:
	if (!r) {
		if (!dev_close(mdac->area.dev))
			stack;

		if (fidtc->raw_metadata_buf) {
			dm_free(fidtc->raw_metadata_buf);
			fidtc->raw_metadata_buf = NULL;
		}
	}

	return r;
}

static int _vg_commit_raw_rlocn(struct format_instance *fid,
				struct volume_group *vg,
				struct metadata_area *mda,
				int precommit)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;
	struct text_fid_context *fidtc = (struct text_fid_context *) fid->private;
	struct mda_header *mdah;
	struct raw_locn *rlocn;
	struct pv_list *pvl;
	int r = 0;
	int found = 0;
	int noprecommit = 0;

	/* Ignore any mda on a PV outside the VG. vgsplit relies on this */
	dm_list_iterate_items(pvl, &vg->pvs) {
		if (pvl->pv->dev == mdac->area.dev) {
			found = 1;
			break;
		}
	}

	if (!found)
		return 1;

	if (!(mdah = _raw_read_mda_header(fid->fmt, &mdac->area)))
		goto_out;

	if (!(rlocn = _find_vg_rlocn(&mdac->area, mdah, vg->name, &noprecommit))) {
		mdah->raw_locns[0].offset = 0;
		mdah->raw_locns[0].size = 0;
		mdah->raw_locns[0].checksum = 0;
		mdah->raw_locns[1].offset = 0;
		mdah->raw_locns[1].size = 0;
		mdah->raw_locns[1].checksum = 0;
		mdah->raw_locns[2].offset = 0;
		mdah->raw_locns[2].size = 0;
		mdah->raw_locns[2].checksum = 0;
		rlocn = &mdah->raw_locns[0];
	}

	if (precommit)
		rlocn++;
	else {
		/* If not precommitting, wipe the precommitted rlocn */
		mdah->raw_locns[1].offset = 0;
		mdah->raw_locns[1].size = 0;
		mdah->raw_locns[1].checksum = 0;
	}

	/* Is there new metadata to commit? */
	if (mdac->rlocn.size) {
		rlocn->offset = mdac->rlocn.offset;
		rlocn->size = mdac->rlocn.size;
		rlocn->checksum = mdac->rlocn.checksum;
		log_debug("%sCommitting %s metadata (%u) to %s header at %"
			  PRIu64, precommit ? "Pre-" : "", vg->name, vg->seqno,
			  dev_name(mdac->area.dev), mdac->area.start);
	} else
		log_debug("Wiping pre-committed %s metadata from %s "
			  "header at %" PRIu64, vg->name,
			  dev_name(mdac->area.dev), mdac->area.start);

	if (!_raw_write_mda_header(fid->fmt, mdac->area.dev, mdac->area.start,
				   mdah)) {
		dm_pool_free(fid->fmt->cmd->mem, mdah);
		log_error("Failed to write metadata area header");
		goto out;
	}

	r = 1;

      out:
	if (!precommit) {
		if (!dev_close(mdac->area.dev))
			stack;
		if (fidtc->raw_metadata_buf) {
			dm_free(fidtc->raw_metadata_buf);
			fidtc->raw_metadata_buf = NULL;
		}
	}

	return r;
}

static int _vg_commit_raw(struct format_instance *fid, struct volume_group *vg,
			  struct metadata_area *mda)
{
	return _vg_commit_raw_rlocn(fid, vg, mda, 0);
}

static int _vg_precommit_raw(struct format_instance *fid,
			     struct volume_group *vg,
			     struct metadata_area *mda)
{
	return _vg_commit_raw_rlocn(fid, vg, mda, 1);
}

/* Close metadata area devices */
static int _vg_revert_raw(struct format_instance *fid, struct volume_group *vg,
			  struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;
	struct pv_list *pvl;
	int found = 0;

	/* Ignore any mda on a PV outside the VG. vgsplit relies on this */
	dm_list_iterate_items(pvl, &vg->pvs) {
		if (pvl->pv->dev == mdac->area.dev) {
			found = 1;
			break;
		}
	}

	if (!found)
		return 1;

	/* Wipe pre-committed metadata */
	mdac->rlocn.size = 0;
	return _vg_commit_raw_rlocn(fid, vg, mda, 0);
}

static int _vg_remove_raw(struct format_instance *fid, struct volume_group *vg,
			  struct metadata_area *mda)
{
	struct mda_context *mdac = (struct mda_context *) mda->metadata_locn;
	struct mda_header *mdah;
	struct raw_locn *rlocn;
	int r = 0;
	int noprecommit = 0;

	if (!dev_open(mdac->area.dev))
		return_0;

	if (!(mdah = _raw_read_mda_header(fid->fmt, &mdac->area)))
		goto_out;

	if (!(rlocn = _find_vg_rlocn(&mdac->area, mdah, vg->name, &noprecommit))) {
		rlocn = &mdah->raw_locns[0];
		mdah->raw_locns[1].offset = 0;
	}

	rlocn->offset = 0;
	rlocn->size = 0;
	rlocn->checksum = 0;

	if (!_raw_write_mda_header(fid->fmt, mdac->area.dev, mdac->area.start,
				   mdah)) {
		dm_pool_free(fid->fmt->cmd->mem, mdah);
		log_error("Failed to write metadata area header");
		goto out;
	}

	r = 1;

      out:
	if (!dev_close(mdac->area.dev))
		stack;

	return r;
}

static struct volume_group *_vg_read_file_name(struct format_instance *fid,
					       const char *vgname,
					       const char *read_path)
{
	struct volume_group *vg;
	time_t when;
	char *desc;

	if (!(vg = text_vg_import_file(fid, read_path, &when, &desc)))
		return_NULL;

	/*
	 * Currently you can only have a single volume group per
	 * text file (this restriction may remain).  We need to
	 * check that it contains the correct volume group.
	 */
	if (vgname && strcmp(vgname, vg->name)) {
		dm_pool_free(fid->fmt->cmd->mem, vg);
		log_error("'%s' does not contain volume group '%s'.",
			  read_path, vgname);
		return NULL;
	} else
		log_debug("Read volume group %s from %s", vg->name, read_path);

	return vg;
}

static struct volume_group *_vg_read_file(struct format_instance *fid,
					  const char *vgname,
					  struct metadata_area *mda)
{
	struct text_context *tc = (struct text_context *) mda->metadata_locn;

	return _vg_read_file_name(fid, vgname, tc->path_live);
}

static struct volume_group *_vg_read_precommit_file(struct format_instance *fid,
						    const char *vgname,
						    struct metadata_area *mda)
{
	struct text_context *tc = (struct text_context *) mda->metadata_locn;
	struct volume_group *vg;

	if ((vg = _vg_read_file_name(fid, vgname, tc->path_edit)))
		vg->status |= PRECOMMITTED;
	else
		vg = _vg_read_file_name(fid, vgname, tc->path_live);

	return vg;
}

static int _vg_write_file(struct format_instance *fid __attribute((unused)),
			  struct volume_group *vg, struct metadata_area *mda)
{
	struct text_context *tc = (struct text_context *) mda->metadata_locn;

	FILE *fp;
	int fd;
	char *slash;
	char temp_file[PATH_MAX], temp_dir[PATH_MAX];

	slash = strrchr(tc->path_edit, '/');

	if (slash == 0)
		strcpy(temp_dir, ".");
	else if (slash - tc->path_edit < PATH_MAX) {
		strncpy(temp_dir, tc->path_edit,
			(size_t) (slash - tc->path_edit));
		temp_dir[slash - tc->path_edit] = '\0';

	} else {
		log_error("Text format failed to determine directory.");
		return 0;
	}

	if (!create_temp_name(temp_dir, temp_file, sizeof(temp_file), &fd,
			      &vg->cmd->rand_seed)) {
		log_error("Couldn't create temporary text file name.");
		return 0;
	}

	if (!(fp = fdopen(fd, "w"))) {
		log_sys_error("fdopen", temp_file);
		if (close(fd))
			log_sys_error("fclose", temp_file);
		return 0;
	}

	log_debug("Writing %s metadata to %s", vg->name, temp_file);

	if (!text_vg_export_file(vg, tc->desc, fp)) {
		log_error("Failed to write metadata to %s.", temp_file);
		if (fclose(fp))
			log_sys_error("fclose", temp_file);
		return 0;
	}

	if (fsync(fd) && (errno != EROFS) && (errno != EINVAL)) {
		log_sys_error("fsync", tc->path_edit);
		if (fclose(fp))
			log_sys_error("fclose", tc->path_edit);
		return 0;
	}

	if (lvm_fclose(fp, tc->path_edit))
		return_0;

	if (rename(temp_file, tc->path_edit)) {
		log_debug("Renaming %s to %s", temp_file, tc->path_edit);
		log_error("%s: rename to %s failed: %s", temp_file,
			  tc->path_edit, strerror(errno));
		return 0;
	}

	return 1;
}

static int _vg_commit_file_backup(struct format_instance *fid __attribute((unused)),
				  struct volume_group *vg,
				  struct metadata_area *mda)
{
	struct text_context *tc = (struct text_context *) mda->metadata_locn;

	if (test_mode()) {
		log_verbose("Test mode: Skipping committing %s metadata (%u)",
			    vg->name, vg->seqno);
		if (unlink(tc->path_edit)) {
			log_debug("Unlinking %s", tc->path_edit);
			log_sys_error("unlink", tc->path_edit);
			return 0;
		}
	} else {
		log_debug("Committing %s metadata (%u)", vg->name, vg->seqno);
		log_debug("Renaming %s to %s", tc->path_edit, tc->path_live);
		if (rename(tc->path_edit, tc->path_live)) {
			log_error("%s: rename to %s failed: %s", tc->path_edit,
				  tc->path_live, strerror(errno));
			return 0;
		}
	}

	sync_dir(tc->path_edit);

	return 1;
}

static int _vg_commit_file(struct format_instance *fid, struct volume_group *vg,
			   struct metadata_area *mda)
{
	struct text_context *tc = (struct text_context *) mda->metadata_locn;
	char *slash;
	char new_name[PATH_MAX];
	size_t len;

	if (!_vg_commit_file_backup(fid, vg, mda))
		return 0;

	/* vgrename? */
	if ((slash = strrchr(tc->path_live, '/')))
		slash = slash + 1;
	else
		slash = tc->path_live;

	if (strcmp(slash, vg->name)) {
		len = slash - tc->path_live;
		strncpy(new_name, tc->path_live, len);
		strcpy(new_name + len, vg->name);
		log_debug("Renaming %s to %s", tc->path_live, new_name);
		if (test_mode())
			log_verbose("Test mode: Skipping rename");
		else {
			if (rename(tc->path_live, new_name)) {
				log_error("%s: rename to %s failed: %s",
					  tc->path_live, new_name,
					  strerror(errno));
				sync_dir(new_name);
				return 0;
			}
		}
	}

	return 1;
}

static int _vg_remove_file(struct format_instance *fid __attribute((unused)),
			   struct volume_group *vg __attribute((unused)),
			   struct metadata_area *mda)
{
	struct text_context *tc = (struct text_context *) mda->metadata_locn;

	if (path_exists(tc->path_edit) && unlink(tc->path_edit)) {
		log_sys_error("unlink", tc->path_edit);
		return 0;
	}

	if (path_exists(tc->path_live) && unlink(tc->path_live)) {
		log_sys_error("unlink", tc->path_live);
		return 0;
	}

	sync_dir(tc->path_live);

	return 1;
}

static int _scan_file(const struct format_type *fmt)
{
	struct dirent *dirent;
	struct dir_list *dl;
	struct dm_list *dir_list;
	char *tmp;
	DIR *d;
	struct volume_group *vg;
	struct format_instance *fid;
	char path[PATH_MAX];
	char *vgname;

	dir_list = &((struct mda_lists *) fmt->private)->dirs;

	dm_list_iterate_items(dl, dir_list) {
		if (!(d = opendir(dl->dir))) {
			log_sys_error("opendir", dl->dir);
			continue;
		}
		while ((dirent = readdir(d)))
			if (strcmp(dirent->d_name, ".") &&
			    strcmp(dirent->d_name, "..") &&
			    (!(tmp = strstr(dirent->d_name, ".tmp")) ||
			     tmp != dirent->d_name + strlen(dirent->d_name)
			     - 4)) {
				vgname = dirent->d_name;
				if (dm_snprintf(path, PATH_MAX, "%s/%s",
						 dl->dir, vgname) < 0) {
					log_error("Name too long %s/%s",
						  dl->dir, vgname);
					break;
				}

				/* FIXME stat file to see if it's changed */
				fid = _text_create_text_instance(fmt, NULL, NULL,
							    NULL);
				if ((vg = _vg_read_file_name(fid, vgname,
							     path)))
					/* FIXME Store creation host in vg */
					lvmcache_update_vg(vg, 0);
			}

		if (closedir(d))
			log_sys_error("closedir", dl->dir);
	}

	return 1;
}

const char *vgname_from_mda(const struct format_type *fmt,
			    struct device_area *dev_area, struct id *vgid,
			    uint32_t *vgstatus, char **creation_host,
			    uint64_t *mda_free_sectors)
{
	struct raw_locn *rlocn;
	struct mda_header *mdah;
	uint32_t wrap = 0;
	const char *vgname = NULL;
	unsigned int len = 0;
	char buf[NAME_LEN + 1] __attribute((aligned(8)));
	char uuid[64] __attribute((aligned(8)));
	uint64_t buffer_size, current_usage;

	if (mda_free_sectors)
		*mda_free_sectors = ((dev_area->size - MDA_HEADER_SIZE) / 2) >> SECTOR_SHIFT;

	if (!dev_open(dev_area->dev))
		return_NULL;

	if (!(mdah = _raw_read_mda_header(fmt, dev_area)))
		goto_out;

	/* FIXME Cope with returning a list */
	rlocn = mdah->raw_locns;

	/*
	 * If no valid offset, do not try to search for vgname
	 */
	if (!rlocn->offset)
		goto out;

	/* Do quick check for a vgname */
	if (!dev_read(dev_area->dev, dev_area->start + rlocn->offset,
		      NAME_LEN, buf))
		goto_out;

	while (buf[len] && !isspace(buf[len]) && buf[len] != '{' &&
	       len < (NAME_LEN - 1))
		len++;

	buf[len] = '\0';

	/* Ignore this entry if the characters aren't permissible */
	if (!validate_name(buf))
		goto_out;

	/* We found a VG - now check the metadata */
	if (rlocn->offset + rlocn->size > mdah->size)
		wrap = (uint32_t) ((rlocn->offset + rlocn->size) - mdah->size);

	if (wrap > rlocn->offset) {
		log_error("%s: metadata too large for circular buffer",
			  dev_name(dev_area->dev));
		goto out;
	}

	/* FIXME 64-bit */
	if (!(vgname = text_vgname_import(fmt, dev_area->dev,
					  (off_t) (dev_area->start +
						   rlocn->offset),
					  (uint32_t) (rlocn->size - wrap),
					  (off_t) (dev_area->start +
						   MDA_HEADER_SIZE),
					  wrap, calc_crc, rlocn->checksum,
					  vgid, vgstatus, creation_host)))
		goto_out;

	/* Ignore this entry if the characters aren't permissible */
	if (!validate_name(vgname)) {
		vgname = NULL;
		goto_out;
	}

	if (!id_write_format(vgid, uuid, sizeof(uuid))) {
		vgname = NULL;
		goto_out;
	}

	log_debug("%s: Found metadata at %" PRIu64 " size %" PRIu64
		  " (in area at %" PRIu64 " size %" PRIu64
		  ") for %s (%s)",
		  dev_name(dev_area->dev), dev_area->start + rlocn->offset,
		  rlocn->size, dev_area->start, dev_area->size, vgname, uuid);

	if (mda_free_sectors) {
		current_usage = (rlocn->size + SECTOR_SIZE - UINT64_C(1)) -
				 (rlocn->size + SECTOR_SIZE - UINT64_C(1)) % SECTOR_SIZE;
		buffer_size = mdah->size - MDA_HEADER_SIZE;

		if (current_usage * 2 >= buffer_size)
			*mda_free_sectors = UINT64_C(0);
		else
			*mda_free_sectors = ((buffer_size - 2 * current_usage) / 2) >> SECTOR_SHIFT;
	}

      out:
	if (!dev_close(dev_area->dev))
		stack;

	return vgname;
}

static int _scan_raw(const struct format_type *fmt)
{
	struct raw_list *rl;
	struct dm_list *raw_list;
	const char *vgname;
	struct volume_group *vg;
	struct format_instance fid;
	struct id vgid;
	uint32_t vgstatus;

	raw_list = &((struct mda_lists *) fmt->private)->raws;

	fid.fmt = fmt;
	dm_list_init(&fid.metadata_areas);

	dm_list_iterate_items(rl, raw_list) {
		/* FIXME We're reading mdah twice here... */
		if ((vgname = vgname_from_mda(fmt, &rl->dev_area, &vgid, &vgstatus,
					      NULL, NULL))) {
			if ((vg = _vg_read_raw_area(&fid, vgname,
						    &rl->dev_area, 0)))
				lvmcache_update_vg(vg, 0);
		}
	}

	return 1;
}

static int _text_scan(const struct format_type *fmt)
{
	return (_scan_file(fmt) & _scan_raw(fmt));
}

/* For orphan, creates new mdas according to policy.
   Always have an mda between end-of-label and pe_align() boundary */
static int _mda_setup(const struct format_type *fmt,
		      uint64_t pe_start, uint64_t pe_end,
		      int pvmetadatacopies,
		      uint64_t pvmetadatasize, struct dm_list *mdas,
		      struct physical_volume *pv,
		      struct volume_group *vg __attribute((unused)))
{
	uint64_t mda_adjustment, disk_size, alignment, alignment_offset;
	uint64_t start1, mda_size1;	/* First area - start of disk */
	uint64_t start2, mda_size2;	/* Second area - end of disk */
	uint64_t wipe_size = 8 << SECTOR_SHIFT;
	size_t pagesize = lvm_getpagesize();

	if (!pvmetadatacopies)
		return 1;

	alignment = pv->pe_align << SECTOR_SHIFT;
	alignment_offset = pv->pe_align_offset << SECTOR_SHIFT;
	disk_size = pv->size << SECTOR_SHIFT;
	pe_start <<= SECTOR_SHIFT;
	pe_end <<= SECTOR_SHIFT;

	if (pe_end > disk_size) {
		log_error("Physical extents end beyond end of device %s!",
			  pv_dev_name(pv));
		return 0;
	}

	/* Requested metadatasize */
	mda_size1 = pvmetadatasize << SECTOR_SHIFT;

	/* Place mda straight after label area at start of disk */
	start1 = LABEL_SCAN_SIZE;

	/* Unless the space available is tiny, round to PAGE_SIZE boundary */
	if ((!pe_start && !pe_end) ||
	    ((pe_start > start1) && (pe_start - start1 >= MDA_SIZE_MIN))) {
		mda_adjustment = start1 % pagesize;
		if (mda_adjustment)
			start1 += (pagesize - mda_adjustment);
	}

	/* Round up to pe_align boundary */
	mda_adjustment = (mda_size1 + start1) % alignment;
	if (mda_adjustment) {
		mda_size1 += (alignment - mda_adjustment);
		/* Revert if it's now too large */
		if (start1 + mda_size1 > disk_size)
			mda_size1 -= (alignment - mda_adjustment);
	}

	/* Add pe_align_offset if on pe_align boundary */
	if (alignment_offset &&
	    (((start1 + mda_size1) % alignment) == 0)) {
		mda_size1 += alignment_offset;
		/* Revert if it's now too large */
		if (start1 + mda_size1 > disk_size)
			mda_size1 -= alignment_offset;
	}

	/* Ensure it's not going to be bigger than the disk! */
	if (start1 + mda_size1 > disk_size) {
		log_warn("WARNING: metadata area fills disk leaving no "
			 "space for data on %s.", pv_dev_name(pv));
		/* Leave some free space for rounding */
		/* Avoid empty data area as could cause tools problems */
		mda_size1 = disk_size - start1 - alignment * 2;
		if (start1 + mda_size1 > disk_size) {
			log_error("Insufficient space for first mda on %s",
				  pv_dev_name(pv));
			return 0;
		}
		/* Round up to pe_align boundary */
		mda_adjustment = (mda_size1 + start1) % alignment;
		if (mda_adjustment)
			mda_size1 += (alignment - mda_adjustment);
		/* Only have 1 mda in this case */
		pvmetadatacopies = 1;
	}

	/* If we already have PEs, avoid overlap */
	if (pe_start || pe_end) {
		if (pe_start <= start1)
			mda_size1 = 0;
		else if (start1 + mda_size1 > pe_start)
			mda_size1 = pe_start - start1;
	}

	/* FIXME If creating new mdas, wipe them! */
	if (mda_size1) {
		if (!add_mda(fmt, fmt->cmd->mem, mdas, pv->dev, start1,
			     mda_size1))
			return 0;

		if (!dev_set((struct device *) pv->dev, start1,
			     (size_t) (mda_size1 >
				       wipe_size ? : mda_size1), 0)) {
			log_error("Failed to wipe new metadata area");
			return 0;
		}

		if (pvmetadatacopies == 1)
			return 1;
	} else
		start1 = 0;

	/* A second copy at end of disk */
	mda_size2 = pvmetadatasize << SECTOR_SHIFT;

	/* Ensure it's not going to be bigger than the disk! */
	if (mda_size2 > disk_size)
		mda_size2 = disk_size - start1 - mda_size1;

	mda_adjustment = (disk_size - mda_size2) % alignment;
	if (mda_adjustment)
		mda_size2 += mda_adjustment;

	start2 = disk_size - mda_size2;

	/* If we already have PEs, avoid overlap */
	if (pe_start || pe_end) {
		if (start2 < pe_end) {
			mda_size2 -= (pe_end - start2);
			start2 = pe_end;
		}
	}

	/* If we already have a first mda, avoid overlap */
	if (mda_size1) {
		if (start2 < start1 + mda_size1) {
			mda_size2 -= (start1 + mda_size1 - start2);
			start2 = start1 + mda_size1;
		}
		/* No room for any PEs here now! */
	}

	if (mda_size2) {
		if (!add_mda(fmt, fmt->cmd->mem, mdas, pv->dev, start2,
			     mda_size2)) return 0;
		if (!dev_set(pv->dev, start2,
			     (size_t) (mda_size1 >
				       wipe_size ? : mda_size1), 0)) {
			log_error("Failed to wipe new metadata area");
			return 0;
		}
	} else
		return 0;

	return 1;
}

/* Only for orphans */
/* Set label_sector to -1 if rewriting existing label into same sector */
/* If mdas is supplied it overwrites existing mdas e.g. used with pvcreate */
static int _text_pv_write(const struct format_type *fmt, struct physical_volume *pv,
		     struct dm_list *mdas, int64_t label_sector)
{
	struct label *label;
	struct lvmcache_info *info;
	struct mda_context *mdac;
	struct metadata_area *mda;
	char buf[MDA_HEADER_SIZE] __attribute((aligned(8)));
	struct mda_header *mdah = (struct mda_header *) buf;
	uint64_t adjustment;
	struct data_area_list *da;

	/* FIXME Test mode don't update cache? */

	if (!(info = lvmcache_add(fmt->labeller, (char *) &pv->id, pv->dev,
				  FMT_TEXT_ORPHAN_VG_NAME, NULL, 0)))
		return_0;
	label = info->label;

	if (label_sector != -1)
		label->sector = label_sector;

	info->device_size = pv->size << SECTOR_SHIFT;
	info->fmt = fmt;

	/* If mdas supplied, use them regardless of existing ones, */
	/* otherwise retain existing ones */
	if (mdas) {
		if (info->mdas.n)
			del_mdas(&info->mdas);
		else
			dm_list_init(&info->mdas);
		dm_list_iterate_items(mda, mdas) {
			mdac = mda->metadata_locn;
			log_debug("Creating metadata area on %s at sector %"
				  PRIu64 " size %" PRIu64 " sectors",
				  dev_name(mdac->area.dev),
				  mdac->area.start >> SECTOR_SHIFT,
				  mdac->area.size >> SECTOR_SHIFT);
			add_mda(fmt, NULL, &info->mdas, mdac->area.dev,
				mdac->area.start, mdac->area.size);
		}
		/* FIXME Temporary until mda creation supported by tools */
	} else if (!info->mdas.n) {
		dm_list_init(&info->mdas);
	}

	/*
	 * If no pe_start supplied but PV already exists,
	 * get existing value; use-cases include:
	 * - pvcreate on PV without prior pvremove
	 * - vgremove on VG with PV(s) that have pe_start=0 (hacked cfg)
	 */
	if (info->das.n) {
		if (!pv->pe_start)
			dm_list_iterate_items(da, &info->das)
				pv->pe_start = da->disk_locn.offset >> SECTOR_SHIFT;
		del_das(&info->das);
	} else
		dm_list_init(&info->das);

#if 0
	/*
	 * FIXME: ideally a pre-existing pe_start seen in .pv_write
	 * would always be preserved BUT 'pvcreate on PV without prior pvremove'
	 * could easily cause the pe_start to overlap with the first mda!
	 */
	if (pv->pe_start) {
		log_very_verbose("%s: preserving pe_start=%lu",
				 pv_dev_name(pv), pv->pe_start);
		goto preserve_pe_start;
	}
#endif

	/*
	 * If pe_start is still unset, set it to first aligned
	 * sector after any metadata areas that begin before pe_start.
	 */
	if (!pv->pe_start) {
		pv->pe_start = pv->pe_align;
		if (pv->pe_align_offset)
			pv->pe_start += pv->pe_align_offset;
	}
	dm_list_iterate_items(mda, &info->mdas) {
		mdac = (struct mda_context *) mda->metadata_locn;
		if (pv->dev == mdac->area.dev &&
		    ((mdac->area.start <= (pv->pe_start << SECTOR_SHIFT)) ||
		    (mdac->area.start <= lvm_getpagesize() &&
		     pv->pe_start < (lvm_getpagesize() >> SECTOR_SHIFT))) &&
		    (mdac->area.start + mdac->area.size >
		     (pv->pe_start << SECTOR_SHIFT))) {
			pv->pe_start = (mdac->area.start + mdac->area.size)
			    >> SECTOR_SHIFT;
			/* Adjust pe_start to: (N * pe_align) + pe_align_offset */
			if (pv->pe_align) {
				adjustment =
				(pv->pe_start - pv->pe_align_offset) % pv->pe_align;
				if (adjustment)
					pv->pe_start += pv->pe_align - adjustment;

				log_very_verbose("%s: setting pe_start=%" PRIu64
					 " (orig_pe_start=%" PRIu64 ", "
					 "pe_align=%lu, pe_align_offset=%lu, "
					 "adjustment=%" PRIu64 ")",
					 pv_dev_name(pv), pv->pe_start,
					 (adjustment ?
					  pv->pe_start -= pv->pe_align - adjustment :
					  pv->pe_start),
					 pv->pe_align, pv->pe_align_offset, adjustment);
			}
		}
	}
	if (pv->pe_start >= pv->size) {
		log_error("Data area is beyond end of device %s!",
			  pv_dev_name(pv));
		return 0;
	}

	/* FIXME: preserve_pe_start: */
	if (!add_da
	    (NULL, &info->das, pv->pe_start << SECTOR_SHIFT, UINT64_C(0)))
		return_0;

	if (!dev_open(pv->dev))
		return_0;

	dm_list_iterate_items(mda, &info->mdas) {
		mdac = mda->metadata_locn;
		memset(&buf, 0, sizeof(buf));
		mdah->size = mdac->area.size;
		if (!_raw_write_mda_header(fmt, mdac->area.dev,
					   mdac->area.start, mdah)) {
			if (!dev_close(pv->dev))
				stack;
			return_0;
		}
	}

	if (!label_write(pv->dev, label)) {
		dev_close(pv->dev);
		return_0;
	}

	if (!dev_close(pv->dev))
		return_0;

	return 1;
}

static int _add_raw(struct dm_list *raw_list, struct device_area *dev_area)
{
	struct raw_list *rl;

	/* Already present? */
	dm_list_iterate_items(rl, raw_list) {
		/* FIXME Check size/overlap consistency too */
		if (rl->dev_area.dev == dev_area->dev &&
		    rl->dev_area.start == dev_area->start)
			return 1;
	}

	if (!(rl = dm_malloc(sizeof(struct raw_list)))) {
		log_error("_add_raw allocation failed");
		return 0;
	}
	memcpy(&rl->dev_area, dev_area, sizeof(*dev_area));
	dm_list_add(raw_list, &rl->list);

	return 1;
}

static int _get_pv_if_in_vg(struct lvmcache_info *info,
			    struct physical_volume *pv)
{
	if (info->vginfo && info->vginfo->vgname &&
	    !is_orphan_vg(info->vginfo->vgname) &&
	    get_pv_from_vg_by_id(info->fmt, info->vginfo->vgname,
				 info->vginfo->vgid, info->dev->pvid, pv))
		return 1;

	return 0;
}

static int _populate_pv_fields(struct lvmcache_info *info,
			       struct physical_volume *pv,
			       int scan_label_only)
{
	struct data_area_list *da;

	/* Have we already cached vgname? */
	if (!scan_label_only && _get_pv_if_in_vg(info, pv))
		return 1;

	/* Perform full scan (just the first time) and try again */
	if (!scan_label_only && !memlock() && !full_scan_done()) {
		lvmcache_label_scan(info->fmt->cmd, 2);

		if (_get_pv_if_in_vg(info, pv))
			return 1;
	}

	/* Orphan */
	pv->dev = info->dev;
	pv->fmt = info->fmt;
	pv->size = info->device_size >> SECTOR_SHIFT;
	pv->vg_name = FMT_TEXT_ORPHAN_VG_NAME;
	memcpy(&pv->id, &info->dev->pvid, sizeof(pv->id));

	/* Currently only support exactly one data area */
	if (dm_list_size(&info->das) != 1) {
		log_error("Must be exactly one data area (found %d) on PV %s",
			  dm_list_size(&info->das), dev_name(info->dev));
		return 0;
	}

	dm_list_iterate_items(da, &info->das)
		pv->pe_start = da->disk_locn.offset >> SECTOR_SHIFT;

	return 1;
}

static int _text_pv_read(const struct format_type *fmt, const char *pv_name,
		    struct physical_volume *pv, struct dm_list *mdas,
		    int scan_label_only)
{
	struct label *label;
	struct device *dev;
	struct lvmcache_info *info;
	struct metadata_area *mda, *mda_new;
	struct mda_context *mdac, *mdac_new;

	if (!(dev = dev_cache_get(pv_name, fmt->cmd->filter)))
		return_0;

	if (!(label_read(dev, &label, UINT64_C(0))))
		return_0;
	info = (struct lvmcache_info *) label->info;

	if (!_populate_pv_fields(info, pv, scan_label_only))
		return 0;

	if (!mdas)
		return 1;

	/* Add copy of mdas to supplied list */
	dm_list_iterate_items(mda, &info->mdas) {
		mdac = (struct mda_context *) mda->metadata_locn;
		if (!(mda_new = dm_pool_alloc(fmt->cmd->mem, sizeof(*mda_new)))) {
			log_error("metadata_area allocation failed");
			return 0;
		}
		if (!(mdac_new = dm_pool_alloc(fmt->cmd->mem, sizeof(*mdac_new)))) {
			log_error("metadata_area allocation failed");
			return 0;
		}
		memcpy(mda_new, mda, sizeof(*mda));
		memcpy(mdac_new, mdac, sizeof(*mdac));
		mda_new->metadata_locn = mdac_new;
		dm_list_add(mdas, &mda_new->list);
	}

	return 1;
}

static void _text_destroy_instance(struct format_instance *fid __attribute((unused)))
{
	return;
}

static void _free_dirs(struct dm_list *dir_list)
{
	struct dm_list *dl, *tmp;

	dm_list_iterate_safe(dl, tmp, dir_list) {
		dm_list_del(dl);
		dm_free(dl);
	}
}

static void _free_raws(struct dm_list *raw_list)
{
	struct dm_list *rl, *tmp;

	dm_list_iterate_safe(rl, tmp, raw_list) {
		dm_list_del(rl);
		dm_free(rl);
	}
}

static void _text_destroy(const struct format_type *fmt)
{
	if (fmt->private) {
		_free_dirs(&((struct mda_lists *) fmt->private)->dirs);
		_free_raws(&((struct mda_lists *) fmt->private)->raws);
		dm_free(fmt->private);
	}

	dm_free((void *)fmt);
}

static struct metadata_area_ops _metadata_text_file_ops = {
	.vg_read = _vg_read_file,
	.vg_read_precommit = _vg_read_precommit_file,
	.vg_write = _vg_write_file,
	.vg_remove = _vg_remove_file,
	.vg_commit = _vg_commit_file
};

static struct metadata_area_ops _metadata_text_file_backup_ops = {
	.vg_read = _vg_read_file,
	.vg_write = _vg_write_file,
	.vg_remove = _vg_remove_file,
	.vg_commit = _vg_commit_file_backup
};

static struct metadata_area_ops _metadata_text_raw_ops = {
	.vg_read = _vg_read_raw,
	.vg_read_precommit = _vg_read_precommit_raw,
	.vg_write = _vg_write_raw,
	.vg_remove = _vg_remove_raw,
	.vg_precommit = _vg_precommit_raw,
	.vg_commit = _vg_commit_raw,
	.vg_revert = _vg_revert_raw,
	.mda_free_sectors = _mda_free_sectors_raw,
	.mda_total_sectors = _mda_total_sectors_raw,
	.mda_in_vg = _mda_in_vg_raw,
	.pv_analyze_mda = _pv_analyze_mda_raw,
};

/* pvmetadatasize in sectors */
/*
 * pe_start goal: FIXME -- reality of .pv_write complexity undermines this goal
 * - In cases where a pre-existing pe_start is provided (pvcreate --restorefile
 *   and vgconvert): pe_start must not be changed (so pv->pe_start = pe_start).
 * - In cases where pe_start is 0: leave pv->pe_start as 0 and defer the
 *   setting of pv->pe_start to .pv_write
 */
static int _text_pv_setup(const struct format_type *fmt,
		     uint64_t pe_start, uint32_t extent_count,
		     uint32_t extent_size, unsigned long data_alignment,
		     unsigned long data_alignment_offset,
		     int pvmetadatacopies,
		     uint64_t pvmetadatasize, struct dm_list *mdas,
		     struct physical_volume *pv, struct volume_group *vg)
{
	struct metadata_area *mda, *mda_new, *mda2;
	struct mda_context *mdac, *mdac_new, *mdac2;
	struct dm_list *pvmdas;
	struct lvmcache_info *info;
	int found;
	uint64_t pe_end = 0;
	unsigned mda_count = 0;
	uint64_t mda_size2 = 0;
	uint64_t pe_count;

	/* FIXME Cope with pvchange */
	/* FIXME Merge code with _text_create_text_instance */

	/* If new vg, add any further mdas on this PV to the fid's mda list */
	if (vg) {
		/* Iterate through all mdas on this PV */
		if ((info = info_from_pvid(pv->dev->pvid, 0))) {
			pvmdas = &info->mdas;
			dm_list_iterate_items(mda, pvmdas) {
				mda_count++;
				mdac =
				    (struct mda_context *) mda->metadata_locn;

				/* FIXME Check it isn't already in use */

				/* Reduce usable device size */
				if (mda_count > 1)
					mda_size2 = mdac->area.size >> SECTOR_SHIFT;

				/* Ensure it isn't already on list */
				found = 0;
				dm_list_iterate_items(mda2, mdas) {
					if (mda2->ops !=
					    &_metadata_text_raw_ops) continue;
					mdac2 =
					    (struct mda_context *)
					    mda2->metadata_locn;
					if (!memcmp
					    (&mdac2->area, &mdac->area,
					     sizeof(mdac->area))) {
						found = 1;
						break;
					}
				}
				if (found)
					continue;

				if (!(mda_new = dm_pool_alloc(fmt->cmd->mem,
							   sizeof(*mda_new))))
					return_0;

				if (!(mdac_new = dm_pool_alloc(fmt->cmd->mem,
							    sizeof(*mdac_new))))
					return_0;
				/* FIXME multiple dev_areas inside area */
				memcpy(mda_new, mda, sizeof(*mda));
				memcpy(mdac_new, mdac, sizeof(*mdac));
				mda_new->metadata_locn = mdac_new;
				dm_list_add(mdas, &mda_new->list);
			}
		}

		/* FIXME Cope with genuine pe_count 0 */

		/* If missing, estimate pv->size from file-based metadata */
		if (!pv->size && pv->pe_count)
			pv->size = pv->pe_count * (uint64_t) vg->extent_size +
				   pv->pe_start + mda_size2;

		/* Recalculate number of extents that will fit */
		if (!pv->pe_count) {
			pe_count = (pv->size - pv->pe_start - mda_size2) /
				   vg->extent_size;
			if (pe_count > UINT32_MAX) {
				log_error("PV %s too large for extent size %s.",
					  pv_dev_name(pv),
					  display_size(vg->cmd, (uint64_t) vg->extent_size));
				return 0;
			}
			pv->pe_count = (uint32_t) pe_count;
		}

		/* Unlike LVM1, we don't store this outside a VG */
		/* FIXME Default from config file? vgextend cmdline flag? */
		pv->status |= ALLOCATABLE_PV;
	} else {
		if (pe_start)
			pv->pe_start = pe_start;

		if (!data_alignment)
			data_alignment = find_config_tree_int(pv->fmt->cmd,
						      "devices/data_alignment",
						      0) * 2;

		if (set_pe_align(pv, data_alignment) != data_alignment &&
		    data_alignment)
			log_warn("WARNING: %s: Overriding data alignment to "
				 "%lu sectors (requested %lu sectors)",
				 pv_dev_name(pv), pv->pe_align, data_alignment);

		if (set_pe_align_offset(pv, data_alignment_offset) != data_alignment_offset &&
		    data_alignment_offset)
			log_warn("WARNING: %s: Overriding data alignment offset to "
				 "%lu sectors (requested %lu sectors)",
				 pv_dev_name(pv), pv->pe_align_offset, data_alignment_offset);

		if (pv->pe_align < pv->pe_align_offset) {
			log_error("%s: pe_align (%lu sectors) must not be less "
				  "than pe_align_offset (%lu sectors)",
				  pv_dev_name(pv), pv->pe_align, pv->pe_align_offset);
			return 0;
		}

		/*
		 * This initialization has a side-effect of allowing
		 * orphaned PVs to be created with the proper alignment.
		 * Setting pv->pe_start here circumvents .pv_write's
		 * "pvcreate on PV without prior pvremove" retreival of
		 * the PV's previous pe_start.
		 * - Without this you get actual != expected pe_start
		 *   failures in the testsuite.
		 */
		if (!pe_start && pv->pe_start < pv->pe_align)
			pv->pe_start = pv->pe_align;

		if (extent_count)
			pe_end = pe_start + extent_count * extent_size - 1;
		if (!_mda_setup(fmt, pe_start, pe_end, pvmetadatacopies,
				pvmetadatasize, mdas, pv, vg))
			return_0;
	}

	return 1;
}

/* NULL vgname means use only the supplied context e.g. an archive file */
static struct format_instance *_text_create_text_instance(const struct format_type
						     *fmt, const char *vgname,
						     const char *vgid,
						     void *context)
{
	struct format_instance *fid;
	struct text_fid_context *fidtc;
	struct metadata_area *mda, *mda_new;
	struct mda_context *mdac, *mdac_new;
	struct dir_list *dl;
	struct raw_list *rl;
	struct dm_list *dir_list, *raw_list, *mdas;
	char path[PATH_MAX];
	struct lvmcache_vginfo *vginfo;
	struct lvmcache_info *info;

	if (!(fid = dm_pool_alloc(fmt->cmd->mem, sizeof(*fid)))) {
		log_error("Couldn't allocate format instance object.");
		return NULL;
	}

	if (!(fidtc = (struct text_fid_context *)
			dm_pool_zalloc(fmt->cmd->mem,sizeof(*fidtc)))) {
		log_error("Couldn't allocate text_fid_context.");
		return NULL;
	}

	fidtc->raw_metadata_buf = NULL;
	fid->private = (void *) fidtc;

	fid->fmt = fmt;
	dm_list_init(&fid->metadata_areas);

	if (!vgname) {
		if (!(mda = dm_pool_alloc(fmt->cmd->mem, sizeof(*mda))))
			return_NULL;
		mda->ops = &_metadata_text_file_backup_ops;
		mda->metadata_locn = context;
		dm_list_add(&fid->metadata_areas, &mda->list);
	} else {
		dir_list = &((struct mda_lists *) fmt->private)->dirs;

		dm_list_iterate_items(dl, dir_list) {
			if (dm_snprintf(path, PATH_MAX, "%s/%s",
					 dl->dir, vgname) < 0) {
				log_error("Name too long %s/%s", dl->dir,
					  vgname);
				return NULL;
			}

			context = create_text_context(fmt->cmd, path, NULL);
			if (!(mda = dm_pool_alloc(fmt->cmd->mem, sizeof(*mda))))
				return_NULL;
			mda->ops = &_metadata_text_file_ops;
			mda->metadata_locn = context;
			dm_list_add(&fid->metadata_areas, &mda->list);
		}

		raw_list = &((struct mda_lists *) fmt->private)->raws;

		dm_list_iterate_items(rl, raw_list) {
			/* FIXME Cache this; rescan below if some missing */
			if (!_raw_holds_vgname(fid, &rl->dev_area, vgname))
				continue;

			if (!(mda = dm_pool_alloc(fmt->cmd->mem, sizeof(*mda))))
				return_NULL;

			if (!(mdac = dm_pool_alloc(fmt->cmd->mem, sizeof(*mdac))))
				return_NULL;
			mda->metadata_locn = mdac;
			/* FIXME Allow multiple dev_areas inside area */
			memcpy(&mdac->area, &rl->dev_area, sizeof(mdac->area));
			mda->ops = &_metadata_text_raw_ops;
			/* FIXME MISTAKE? mda->metadata_locn = context; */
			dm_list_add(&fid->metadata_areas, &mda->list);
		}

		/* Scan PVs in VG for any further MDAs */
		lvmcache_label_scan(fmt->cmd, 0);
		if (!(vginfo = vginfo_from_vgname(vgname, vgid)))
			goto_out;
		dm_list_iterate_items(info, &vginfo->infos) {
			mdas = &info->mdas;
			dm_list_iterate_items(mda, mdas) {
				mdac =
				    (struct mda_context *) mda->metadata_locn;

				/* FIXME Check it holds this VG */
				if (!(mda_new = dm_pool_alloc(fmt->cmd->mem,
							   sizeof(*mda_new))))
					return_NULL;

				if (!(mdac_new = dm_pool_alloc(fmt->cmd->mem,
							    sizeof(*mdac_new))))
					return_NULL;
				/* FIXME multiple dev_areas inside area */
				memcpy(mda_new, mda, sizeof(*mda));
				memcpy(mdac_new, mdac, sizeof(*mdac));
				mda_new->metadata_locn = mdac_new;
				dm_list_add(&fid->metadata_areas, &mda_new->list);
			}
		}
		/* FIXME Check raw metadata area count - rescan if required */
	}

      out:
	return fid;
}

void *create_text_context(struct cmd_context *cmd, const char *path,
			  const char *desc)
{
	struct text_context *tc;
	char *tmp;

	if ((tmp = strstr(path, ".tmp")) && (tmp == path + strlen(path) - 4)) {
		log_error("%s: Volume group filename may not end in .tmp",
			  path);
		return NULL;
	}

	if (!(tc = dm_pool_alloc(cmd->mem, sizeof(*tc))))
		return_NULL;

	if (!(tc->path_live = dm_pool_strdup(cmd->mem, path)))
		goto_bad;

	if (!(tc->path_edit = dm_pool_alloc(cmd->mem, strlen(path) + 5)))
		goto_bad;

	sprintf(tc->path_edit, "%s.tmp", path);

	if (!desc)
		desc = "";

	if (!(tc->desc = dm_pool_strdup(cmd->mem, desc)))
		goto_bad;

	return (void *) tc;

      bad:
	dm_pool_free(cmd->mem, tc);

	log_error("Couldn't allocate text format context object.");
	return NULL;
}

static struct format_handler _text_handler = {
	.scan = _text_scan,
	.pv_read = _text_pv_read,
	.pv_setup = _text_pv_setup,
	.pv_write = _text_pv_write,
	.vg_setup = _text_vg_setup,
	.lv_setup = _text_lv_setup,
	.create_instance = _text_create_text_instance,
	.destroy_instance = _text_destroy_instance,
	.destroy = _text_destroy
};

static int _add_dir(const char *dir, struct dm_list *dir_list)
{
	struct dir_list *dl;

	if (dm_create_dir(dir)) {
		if (!(dl = dm_malloc(sizeof(struct dm_list) + strlen(dir) + 1))) {
			log_error("_add_dir allocation failed");
			return 0;
		}
		log_very_verbose("Adding text format metadata dir: %s", dir);
		strcpy(dl->dir, dir);
		dm_list_add(dir_list, &dl->list);
		return 1;
	}

	return 0;
}

static int _get_config_disk_area(struct cmd_context *cmd,
				 struct config_node *cn, struct dm_list *raw_list)
{
	struct device_area dev_area;
	char *id_str;
	struct id id;

	if (!(cn = cn->child)) {
		log_error("Empty metadata disk_area section of config file");
		return 0;
	}

	if (!get_config_uint64(cn, "start_sector", &dev_area.start)) {
		log_error("Missing start_sector in metadata disk_area section "
			  "of config file");
		return 0;
	}
	dev_area.start <<= SECTOR_SHIFT;

	if (!get_config_uint64(cn, "size", &dev_area.size)) {
		log_error("Missing size in metadata disk_area section "
			  "of config file");
		return 0;
	}
	dev_area.size <<= SECTOR_SHIFT;

	if (!get_config_str(cn, "id", &id_str)) {
		log_error("Missing uuid in metadata disk_area section "
			  "of config file");
		return 0;
	}

	if (!id_read_format(&id, id_str)) {
		log_error("Invalid uuid in metadata disk_area section "
			  "of config file: %s", id_str);
		return 0;
	}

	if (!(dev_area.dev = device_from_pvid(cmd, &id))) {
		char buffer[64] __attribute((aligned(8)));

		if (!id_write_format(&id, buffer, sizeof(buffer)))
			log_error("Couldn't find device.");
		else
			log_error("Couldn't find device with uuid '%s'.",
				  buffer);

		return 0;
	}

	return _add_raw(raw_list, &dev_area);
}

struct format_type *create_text_format(struct cmd_context *cmd)
{
	struct format_type *fmt;
	struct config_node *cn;
	struct config_value *cv;
	struct mda_lists *mda_lists;

	if (!(fmt = dm_malloc(sizeof(*fmt))))
		return_NULL;

	fmt->cmd = cmd;
	fmt->ops = &_text_handler;
	fmt->name = FMT_TEXT_NAME;
	fmt->alias = FMT_TEXT_ALIAS;
	fmt->orphan_vg_name = ORPHAN_VG_NAME(FMT_TEXT_NAME);
	fmt->features = FMT_SEGMENTS | FMT_MDAS | FMT_TAGS | FMT_PRECOMMIT |
			FMT_UNLIMITED_VOLS | FMT_RESIZE_PV |
			FMT_UNLIMITED_STRIPESIZE;

	if (!(mda_lists = dm_malloc(sizeof(struct mda_lists)))) {
		log_error("Failed to allocate dir_list");
		dm_free(fmt);
		return NULL;
	}

	dm_list_init(&mda_lists->dirs);
	dm_list_init(&mda_lists->raws);
	mda_lists->file_ops = &_metadata_text_file_ops;
	mda_lists->raw_ops = &_metadata_text_raw_ops;
	fmt->private = (void *) mda_lists;

	if (!(fmt->labeller = text_labeller_create(fmt))) {
		log_error("Couldn't create text label handler.");
		dm_free(fmt);
		return NULL;
	}

	if (!(label_register_handler(FMT_TEXT_NAME, fmt->labeller))) {
		log_error("Couldn't register text label handler.");
		dm_free(fmt);
		return NULL;
	}

	if ((cn = find_config_tree_node(cmd, "metadata/dirs"))) {
		for (cv = cn->v; cv; cv = cv->next) {
			if (cv->type != CFG_STRING) {
				log_error("Invalid string in config file: "
					  "metadata/dirs");
				goto err;
			}

			if (!_add_dir(cv->v.str, &mda_lists->dirs)) {
				log_error("Failed to add %s to text format "
					  "metadata directory list ", cv->v.str);
				goto err;
			}
		}
	}

	if ((cn = find_config_tree_node(cmd, "metadata/disk_areas"))) {
		for (cn = cn->child; cn; cn = cn->sib) {
			if (!_get_config_disk_area(cmd, cn, &mda_lists->raws))
				goto err;
		}
	}

	log_very_verbose("Initialised format: %s", fmt->name);

	return fmt;

      err:
	_free_dirs(&mda_lists->dirs);

	dm_free(fmt);
	return NULL;
}
