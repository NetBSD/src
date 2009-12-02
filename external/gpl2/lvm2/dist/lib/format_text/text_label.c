/*	$NetBSD: text_label.c,v 1.1.1.2 2009/12/02 00:26:28 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
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
#include "format-text.h"
#include "layout.h"
#include "label.h"
#include "xlate.h"
#include "lvmcache.h"

#include <sys/stat.h>
#include <fcntl.h>

static int _text_can_handle(struct labeller *l __attribute((unused)),
			    void *buf,
			    uint64_t sector __attribute((unused)))
{
	struct label_header *lh = (struct label_header *) buf;

	if (!strncmp((char *)lh->type, LVM2_LABEL, sizeof(lh->type)))
		return 1;

	return 0;
}

static int _text_write(struct label *label, void *buf)
{
	struct label_header *lh = (struct label_header *) buf;
	struct pv_header *pvhdr;
	struct lvmcache_info *info;
	struct disk_locn *pvh_dlocn_xl;
	struct metadata_area *mda;
	struct mda_context *mdac;
	struct data_area_list *da;
	char buffer[64] __attribute((aligned(8)));
	int da1, mda1, mda2;

	/* FIXME Move to where label is created */
	strncpy(label->type, LVM2_LABEL, sizeof(label->type));

	strncpy((char *)lh->type, label->type, sizeof(label->type));

	pvhdr = (struct pv_header *) ((void *) buf + xlate32(lh->offset_xl));
	info = (struct lvmcache_info *) label->info;
	pvhdr->device_size_xl = xlate64(info->device_size);
	memcpy(pvhdr->pv_uuid, &info->dev->pvid, sizeof(struct id));
	if (!id_write_format((const struct id *)pvhdr->pv_uuid, buffer,
			     sizeof(buffer))) {
		stack;
		buffer[0] = '\0';
	}

	pvh_dlocn_xl = &pvhdr->disk_areas_xl[0];

	/* List of data areas (holding PEs) */
	dm_list_iterate_items(da, &info->das) {
		pvh_dlocn_xl->offset = xlate64(da->disk_locn.offset);
		pvh_dlocn_xl->size = xlate64(da->disk_locn.size);
		pvh_dlocn_xl++;
	}

	/* NULL-termination */
	pvh_dlocn_xl->offset = xlate64(UINT64_C(0));
	pvh_dlocn_xl->size = xlate64(UINT64_C(0));
	pvh_dlocn_xl++;

	/* List of metadata area header locations */
	dm_list_iterate_items(mda, &info->mdas) {
		mdac = (struct mda_context *) mda->metadata_locn;

		if (mdac->area.dev != info->dev)
			continue;

		pvh_dlocn_xl->offset = xlate64(mdac->area.start);
		pvh_dlocn_xl->size = xlate64(mdac->area.size);
		pvh_dlocn_xl++;
	}

	/* NULL-termination */
	pvh_dlocn_xl->offset = xlate64(UINT64_C(0));
	pvh_dlocn_xl->size = xlate64(UINT64_C(0));

	/* Create debug message with da and mda locations */
	if (xlate64(pvhdr->disk_areas_xl[0].offset) ||
	    xlate64(pvhdr->disk_areas_xl[0].size))
		da1 = 0;
	else
		da1 = -1;

	mda1 = da1 + 2;
	mda2 = mda1 + 1;
	
	if (!xlate64(pvhdr->disk_areas_xl[mda1].offset) &&
	    !xlate64(pvhdr->disk_areas_xl[mda1].size))
		mda1 = mda2 = 0;
	else if (!xlate64(pvhdr->disk_areas_xl[mda2].offset) &&
		 !xlate64(pvhdr->disk_areas_xl[mda2].size))
		mda2 = 0;

	log_debug("%s: Preparing PV label header %s size %" PRIu64 " with"
		  "%s%.*" PRIu64 "%s%.*" PRIu64 "%s"
		  "%s%.*" PRIu64 "%s%.*" PRIu64 "%s"
		  "%s%.*" PRIu64 "%s%.*" PRIu64 "%s",
		  dev_name(info->dev), buffer, info->device_size, 
		  (da1 > -1) ? " da1 (" : "",
		  (da1 > -1) ? 1 : 0,
		  (da1 > -1) ? xlate64(pvhdr->disk_areas_xl[da1].offset) >> SECTOR_SHIFT : 0,
		  (da1 > -1) ? "s, " : "",
		  (da1 > -1) ? 1 : 0,
		  (da1 > -1) ? xlate64(pvhdr->disk_areas_xl[da1].size) >> SECTOR_SHIFT : 0,
		  (da1 > -1) ? "s)" : "",
		  mda1 ? " mda1 (" : "",
		  mda1 ? 1 : 0,
		  mda1 ? xlate64(pvhdr->disk_areas_xl[mda1].offset) >> SECTOR_SHIFT : 0,
		  mda1 ? "s, " : "",
		  mda1 ? 1 : 0,
		  mda1 ? xlate64(pvhdr->disk_areas_xl[mda1].size) >> SECTOR_SHIFT : 0,
		  mda1 ? "s)" : "",
		  mda2 ? " mda2 (" : "",
		  mda2 ? 1 : 0,
		  mda2 ? xlate64(pvhdr->disk_areas_xl[mda2].offset) >> SECTOR_SHIFT : 0,
		  mda2 ? "s, " : "",
		  mda2 ? 1 : 0,
		  mda2 ? xlate64(pvhdr->disk_areas_xl[mda2].size) >> SECTOR_SHIFT : 0,
		  mda2 ? "s)" : "");

	if (da1 < 0) {
		log_error("Internal error: %s label header currently requires "
			  "a data area.", dev_name(info->dev));
		return 0;
	}

	return 1;
}

int add_da(struct dm_pool *mem, struct dm_list *das,
	   uint64_t start, uint64_t size)
{
	struct data_area_list *dal;

	if (!mem) {
		if (!(dal = dm_malloc(sizeof(*dal)))) {
			log_error("struct data_area_list allocation failed");
			return 0;
		}
	} else {
		if (!(dal = dm_pool_alloc(mem, sizeof(*dal)))) {
			log_error("struct data_area_list allocation failed");
			return 0;
		}
	}

	dal->disk_locn.offset = start;
	dal->disk_locn.size = size;

	dm_list_add(das, &dal->list);

	return 1;
}

void del_das(struct dm_list *das)
{
	struct dm_list *dah, *tmp;
	struct data_area_list *da;

	dm_list_iterate_safe(dah, tmp, das) {
		da = dm_list_item(dah, struct data_area_list);
		dm_list_del(&da->list);
		dm_free(da);
	}
}

int add_mda(const struct format_type *fmt, struct dm_pool *mem, struct dm_list *mdas,
	    struct device *dev, uint64_t start, uint64_t size)
{
/* FIXME List size restricted by pv_header SECTOR_SIZE */
	struct metadata_area *mdal;
	struct mda_lists *mda_lists = (struct mda_lists *) fmt->private;
	struct mda_context *mdac;

	if (!mem) {
		if (!(mdal = dm_malloc(sizeof(struct metadata_area)))) {
			log_error("struct mda_list allocation failed");
			return 0;
		}

		if (!(mdac = dm_malloc(sizeof(struct mda_context)))) {
			log_error("struct mda_context allocation failed");
			dm_free(mdal);
			return 0;
		}
	} else {
		if (!(mdal = dm_pool_alloc(mem, sizeof(struct metadata_area)))) {
			log_error("struct mda_list allocation failed");
			return 0;
		}

		if (!(mdac = dm_pool_alloc(mem, sizeof(struct mda_context)))) {
			log_error("struct mda_context allocation failed");
			return 0;
		}
	}

	mdal->ops = mda_lists->raw_ops;
	mdal->metadata_locn = mdac;

	mdac->area.dev = dev;
	mdac->area.start = start;
	mdac->area.size = size;
	mdac->free_sectors = UINT64_C(0);
	memset(&mdac->rlocn, 0, sizeof(mdac->rlocn));

	dm_list_add(mdas, &mdal->list);
	return 1;
}

void del_mdas(struct dm_list *mdas)
{
	struct dm_list *mdah, *tmp;
	struct metadata_area *mda;

	dm_list_iterate_safe(mdah, tmp, mdas) {
		mda = dm_list_item(mdah, struct metadata_area);
		dm_free(mda->metadata_locn);
		dm_list_del(&mda->list);
		dm_free(mda);
	}
}

static int _text_initialise_label(struct labeller *l __attribute((unused)),
				  struct label *label)
{
	strncpy(label->type, LVM2_LABEL, sizeof(label->type));

	return 1;
}

static int _text_read(struct labeller *l, struct device *dev, void *buf,
		 struct label **label)
{
	struct label_header *lh = (struct label_header *) buf;
	struct pv_header *pvhdr;
	struct lvmcache_info *info;
	struct disk_locn *dlocn_xl;
	uint64_t offset;
	struct metadata_area *mda;
	struct id vgid;
	struct mda_context *mdac;
	const char *vgname;
	uint32_t vgstatus;
	char *creation_host;

	pvhdr = (struct pv_header *) ((void *) buf + xlate32(lh->offset_xl));

	if (!(info = lvmcache_add(l, (char *)pvhdr->pv_uuid, dev,
				  FMT_TEXT_ORPHAN_VG_NAME,
				  FMT_TEXT_ORPHAN_VG_NAME, 0)))
		return_0;
	*label = info->label;

	info->device_size = xlate64(pvhdr->device_size_xl);

	if (info->das.n)
		del_das(&info->das);
	dm_list_init(&info->das);

	if (info->mdas.n)
		del_mdas(&info->mdas);
	dm_list_init(&info->mdas);

	/* Data areas holding the PEs */
	dlocn_xl = pvhdr->disk_areas_xl;
	while ((offset = xlate64(dlocn_xl->offset))) {
		add_da(NULL, &info->das, offset,
		       xlate64(dlocn_xl->size));
		dlocn_xl++;
	}

	/* Metadata area headers */
	dlocn_xl++;
	while ((offset = xlate64(dlocn_xl->offset))) {
		add_mda(info->fmt, NULL, &info->mdas, dev, offset,
			xlate64(dlocn_xl->size));
		dlocn_xl++;
	}

	dm_list_iterate_items(mda, &info->mdas) {
		mdac = (struct mda_context *) mda->metadata_locn;
		if ((vgname = vgname_from_mda(info->fmt, &mdac->area,
					      &vgid, &vgstatus, &creation_host,
					      &mdac->free_sectors)) &&
		    !lvmcache_update_vgname_and_id(info, vgname,
						   (char *) &vgid, vgstatus,
						   creation_host))
			return_0;
	}

	info->status &= ~CACHE_INVALID;

	return 1;
}

static void _text_destroy_label(struct labeller *l __attribute((unused)),
				struct label *label)
{
	struct lvmcache_info *info = (struct lvmcache_info *) label->info;

	if (info->mdas.n)
		del_mdas(&info->mdas);
	if (info->das.n)
		del_das(&info->das);
}

static void _fmt_text_destroy(struct labeller *l)
{
	dm_free(l);
}

struct label_ops _text_ops = {
	.can_handle = _text_can_handle,
	.write = _text_write,
	.read = _text_read,
	.verify = _text_can_handle,
	.initialise_label = _text_initialise_label,
	.destroy_label = _text_destroy_label,
	.destroy = _fmt_text_destroy,
};

struct labeller *text_labeller_create(const struct format_type *fmt)
{
	struct labeller *l;

	if (!(l = dm_malloc(sizeof(*l)))) {
		log_error("Couldn't allocate labeller object.");
		return NULL;
	}

	l->ops = &_text_ops;
	l->private = (const void *) fmt;

	return l;
}
