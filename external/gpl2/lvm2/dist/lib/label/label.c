/*	$NetBSD: label.c,v 1.1.1.2 2009/12/02 00:26:32 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
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
#include "label.h"
#include "crc.h"
#include "xlate.h"
#include "lvmcache.h"
#include "metadata.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* FIXME Allow for larger labels?  Restricted to single sector currently */

/*
 * Internal labeller struct.
 */
struct labeller_i {
	struct dm_list list;

	struct labeller *l;
	char name[0];
};

static struct dm_list _labellers;

static struct labeller_i *_alloc_li(const char *name, struct labeller *l)
{
	struct labeller_i *li;
	size_t len;

	len = sizeof(*li) + strlen(name) + 1;

	if (!(li = dm_malloc(len))) {
		log_error("Couldn't allocate memory for labeller list object.");
		return NULL;
	}

	li->l = l;
	strcpy(li->name, name);

	return li;
}

static void _free_li(struct labeller_i *li)
{
	dm_free(li);
}

int label_init(void)
{
	dm_list_init(&_labellers);
	return 1;
}

void label_exit(void)
{
	struct dm_list *c, *n;
	struct labeller_i *li;

	for (c = _labellers.n; c && c != &_labellers; c = n) {
		n = c->n;
		li = dm_list_item(c, struct labeller_i);
		li->l->ops->destroy(li->l);
		_free_li(li);
	}

	dm_list_init(&_labellers);
}

int label_register_handler(const char *name, struct labeller *handler)
{
	struct labeller_i *li;

	if (!(li = _alloc_li(name, handler)))
		return_0;

	dm_list_add(&_labellers, &li->list);
	return 1;
}

struct labeller *label_get_handler(const char *name)
{
	struct labeller_i *li;

	dm_list_iterate_items(li, &_labellers)
		if (!strcmp(li->name, name))
			return li->l;

	return NULL;
}

static struct labeller *_find_labeller(struct device *dev, char *buf,
				       uint64_t *label_sector,
				       uint64_t scan_sector)
{
	struct labeller_i *li;
	struct labeller *r = NULL;
	struct label_header *lh;
	struct lvmcache_info *info;
	uint64_t sector;
	int found = 0;
	char readbuf[LABEL_SCAN_SIZE] __attribute((aligned(8)));

	if (!dev_read(dev, scan_sector << SECTOR_SHIFT,
		      LABEL_SCAN_SIZE, readbuf)) {
		log_debug("%s: Failed to read label area", dev_name(dev));
		goto out;
	}

	/* Scan a few sectors for a valid label */
	for (sector = 0; sector < LABEL_SCAN_SECTORS;
	     sector += LABEL_SIZE >> SECTOR_SHIFT) {
		lh = (struct label_header *) (readbuf +
					      (sector << SECTOR_SHIFT));

		if (!strncmp((char *)lh->id, LABEL_ID, sizeof(lh->id))) {
			if (found) {
				log_error("Ignoring additional label on %s at "
					  "sector %" PRIu64, dev_name(dev),
					  sector + scan_sector);
			}
			if (xlate64(lh->sector_xl) != sector + scan_sector) {
				log_info("%s: Label for sector %" PRIu64
					 " found at sector %" PRIu64
					 " - ignoring", dev_name(dev),
					 (uint64_t)xlate64(lh->sector_xl),
					 sector + scan_sector);
				continue;
			}
			if (calc_crc(INITIAL_CRC, &lh->offset_xl, LABEL_SIZE -
				     ((uintptr_t) &lh->offset_xl - (uintptr_t) lh)) !=
			    xlate32(lh->crc_xl)) {
				log_info("Label checksum incorrect on %s - "
					 "ignoring", dev_name(dev));
				continue;
			}
			if (found)
				continue;
		}

		dm_list_iterate_items(li, &_labellers) {
			if (li->l->ops->can_handle(li->l, (char *) lh,
						   sector + scan_sector)) {
				log_very_verbose("%s: %s label detected",
						 dev_name(dev), li->name);
				if (found) {
					log_error("Ignoring additional label "
						  "on %s at sector %" PRIu64,
						  dev_name(dev),
						  sector + scan_sector);
					continue;
				}
				r = li->l;
				memcpy(buf, lh, LABEL_SIZE);
				if (label_sector)
					*label_sector = sector + scan_sector;
				found = 1;
				break;
			}
		}
	}

      out:
	if (!found) {
		if ((info = info_from_pvid(dev->pvid, 0)))
			lvmcache_update_vgname_and_id(info, info->fmt->orphan_vg_name,
						      info->fmt->orphan_vg_name,
						      0, NULL);
		log_very_verbose("%s: No label detected", dev_name(dev));
	}

	return r;
}

/* FIXME Also wipe associated metadata area headers? */
int label_remove(struct device *dev)
{
	char buf[LABEL_SIZE] __attribute((aligned(8)));
	char readbuf[LABEL_SCAN_SIZE] __attribute((aligned(8)));
	int r = 1;
	uint64_t sector;
	int wipe;
	struct labeller_i *li;
	struct label_header *lh;

	memset(buf, 0, LABEL_SIZE);

	log_very_verbose("Scanning for labels to wipe from %s", dev_name(dev));

	if (!dev_open(dev))
		return_0;

	/*
	 * We flush the device just in case someone is stupid
	 * enough to be trying to import an open pv into lvm.
	 */
	dev_flush(dev);

	if (!dev_read(dev, UINT64_C(0), LABEL_SCAN_SIZE, readbuf)) {
		log_debug("%s: Failed to read label area", dev_name(dev));
		goto out;
	}

	/* Scan first few sectors for anything looking like a label */
	for (sector = 0; sector < LABEL_SCAN_SECTORS;
	     sector += LABEL_SIZE >> SECTOR_SHIFT) {
		lh = (struct label_header *) (readbuf +
					      (sector << SECTOR_SHIFT));

		wipe = 0;

		if (!strncmp((char *)lh->id, LABEL_ID, sizeof(lh->id))) {
			if (xlate64(lh->sector_xl) == sector)
				wipe = 1;
		} else {
			dm_list_iterate_items(li, &_labellers) {
				if (li->l->ops->can_handle(li->l, (char *) lh,
							   sector)) {
					wipe = 1;
					break;
				}
			}
		}

		if (wipe) {
			log_info("%s: Wiping label at sector %" PRIu64,
				 dev_name(dev), sector);
			if (!dev_write(dev, sector << SECTOR_SHIFT, LABEL_SIZE,
				       buf)) {
				log_error("Failed to remove label from %s at "
					  "sector %" PRIu64, dev_name(dev),
					  sector);
				r = 0;
			}
		}
	}

      out:
	if (!dev_close(dev))
		stack;

	return r;
}

int label_read(struct device *dev, struct label **result,
		uint64_t scan_sector)
{
	char buf[LABEL_SIZE] __attribute((aligned(8)));
	struct labeller *l;
	uint64_t sector;
	struct lvmcache_info *info;
	int r = 0;

	if ((info = info_from_pvid(dev->pvid, 1))) {
		log_debug("Using cached label for %s", dev_name(dev));
		*result = info->label;
		return 1;
	}

	if (!dev_open(dev)) {
		stack;

		if ((info = info_from_pvid(dev->pvid, 0)))
			lvmcache_update_vgname_and_id(info, info->fmt->orphan_vg_name,
						      info->fmt->orphan_vg_name,
						      0, NULL);

		return r;
	}

	if (!(l = _find_labeller(dev, buf, &sector, scan_sector)))
		goto out;

	if ((r = (l->ops->read)(l, dev, buf, result)) && result && *result)
		(*result)->sector = sector;

      out:
	if (!dev_close(dev))
		stack;

	return r;
}

/* Caller may need to use label_get_handler to create label struct! */
int label_write(struct device *dev, struct label *label)
{
	char buf[LABEL_SIZE] __attribute((aligned(8)));
	struct label_header *lh = (struct label_header *) buf;
	int r = 1;

	if (!label->labeller->ops->write) {
		log_error("Label handler does not support label writes");
		return 0;
	}

	if ((LABEL_SIZE + (label->sector << SECTOR_SHIFT)) > LABEL_SCAN_SIZE) {
		log_error("Label sector %" PRIu64 " beyond range (%ld)",
			  label->sector, LABEL_SCAN_SECTORS);
		return 0;
	}

	memset(buf, 0, LABEL_SIZE);

	strncpy((char *)lh->id, LABEL_ID, sizeof(lh->id));
	lh->sector_xl = xlate64(label->sector);
	lh->offset_xl = xlate32(sizeof(*lh));

	if (!(label->labeller->ops->write)(label, buf))
		return_0;

	lh->crc_xl = xlate32(calc_crc(INITIAL_CRC, &lh->offset_xl, LABEL_SIZE -
				      ((uintptr_t) &lh->offset_xl - (uintptr_t) lh)));

	if (!dev_open(dev))
		return_0;

	log_info("%s: Writing label to sector %" PRIu64 " with stored offset %"
		 PRIu32 ".", dev_name(dev), label->sector,
		 xlate32(lh->offset_xl));
	if (!dev_write(dev, label->sector << SECTOR_SHIFT, LABEL_SIZE, buf)) {
		log_debug("Failed to write label to %s", dev_name(dev));
		r = 0;
	}

	if (!dev_close(dev))
		stack;

	return r;
}

/* Unused */
int label_verify(struct device *dev)
{
	struct labeller *l;
	char buf[LABEL_SIZE] __attribute((aligned(8)));
	uint64_t sector;
	struct lvmcache_info *info;
	int r = 0;

	if (!dev_open(dev)) {
		if ((info = info_from_pvid(dev->pvid, 0)))
			lvmcache_update_vgname_and_id(info, info->fmt->orphan_vg_name,
						      info->fmt->orphan_vg_name,
						      0, NULL);

		return_0;
	}

	if (!(l = _find_labeller(dev, buf, &sector, UINT64_C(0))))
		goto out;

	r = l->ops->verify ? l->ops->verify(l, buf, sector) : 1;

      out:
	if (!dev_close(dev))
		stack;

	return r;
}

void label_destroy(struct label *label)
{
	label->labeller->ops->destroy_label(label->labeller, label);
	dm_free(label);
}

struct label *label_create(struct labeller *labeller)
{
	struct label *label;

	if (!(label = dm_malloc(sizeof(*label)))) {
		log_error("label allocaction failed");
		return NULL;
	}
	memset(label, 0, sizeof(*label));

	label->labeller = labeller;

	labeller->ops->initialise_label(labeller, label);

	return label;
}
