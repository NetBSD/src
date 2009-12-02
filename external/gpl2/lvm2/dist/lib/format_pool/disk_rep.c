/*	$NetBSD: disk_rep.c,v 1.1.1.2 2009/12/02 00:26:50 haad Exp $	*/

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
#include "filter.h"
#include "xlate.h"
#include "disk_rep.h"

#include <assert.h>

/* FIXME: memcpy might not be portable */
#define CPIN_8(x, y, z) {memcpy((x), (y), (z));}
#define CPOUT_8(x, y, z) {memcpy((y), (x), (z));}
#define CPIN_16(x, y) {(x) = xlate16_be((y));}
#define CPOUT_16(x, y) {(y) = xlate16_be((x));}
#define CPIN_32(x, y) {(x) = xlate32_be((y));}
#define CPOUT_32(x, y) {(y) = xlate32_be((x));}
#define CPIN_64(x, y) {(x) = xlate64_be((y));}
#define CPOUT_64(x, y) {(y) = xlate64_be((x));}

static int __read_pool_disk(const struct format_type *fmt, struct device *dev,
			    struct dm_pool *mem __attribute((unused)), struct pool_list *pl,
			    const char *vg_name __attribute((unused)))
{
	char buf[512] __attribute((aligned(8)));

	/* FIXME: Need to check the cache here first */
	if (!dev_read(dev, UINT64_C(0), 512, buf)) {
		log_very_verbose("Failed to read PV data from %s",
				 dev_name(dev));
		return 0;
	}

	if (!read_pool_label(pl, fmt->labeller, dev, buf, NULL))
		return_0;

	return 1;
}

static void _add_pl_to_list(struct dm_list *head, struct pool_list *data)
{
	struct pool_list *pl;

	dm_list_iterate_items(pl, head) {
		if (id_equal(&data->pv_uuid, &pl->pv_uuid)) {
			char uuid[ID_LEN + 7] __attribute((aligned(8)));

			id_write_format(&pl->pv_uuid, uuid, ID_LEN + 7);

			if (!dev_subsystem_part_major(data->dev)) {
				log_very_verbose("Ignoring duplicate PV %s on "
						 "%s", uuid,
						 dev_name(data->dev));
				return;
			}
			log_very_verbose("Duplicate PV %s - using %s %s",
					 uuid, dev_subsystem_name(data->dev),
					 dev_name(data->dev));
			dm_list_del(&pl->list);
			break;
		}
	}
	dm_list_add(head, &data->list);
}

int read_pool_label(struct pool_list *pl, struct labeller *l,
		    struct device *dev, char *buf, struct label **label)
{
	struct lvmcache_info *info;
	struct id pvid;
	struct id vgid;
	char uuid[ID_LEN + 7] __attribute((aligned(8)));
	struct pool_disk *pd = &pl->pd;

	pool_label_in(pd, buf);

	get_pool_pv_uuid(&pvid, pd);
	id_write_format(&pvid, uuid, ID_LEN + 7);
	log_debug("Calculated uuid %s for %s", uuid, dev_name(dev));

	get_pool_vg_uuid(&vgid, pd);
	id_write_format(&vgid, uuid, ID_LEN + 7);
	log_debug("Calculated uuid %s for %s", uuid, pd->pl_pool_name);

	if (!(info = lvmcache_add(l, (char *) &pvid, dev, pd->pl_pool_name,
				  (char *) &vgid, 0)))
		return_0;
	if (label)
		*label = info->label;

	info->device_size = xlate32_be(pd->pl_blocks) << SECTOR_SHIFT;
	dm_list_init(&info->mdas);

	info->status &= ~CACHE_INVALID;

	pl->dev = dev;
	pl->pv = NULL;
	memcpy(&pl->pv_uuid, &pvid, sizeof(pvid));

	return 1;
}

/**
 * pool_label_out - copies a pool_label_t into a char buffer
 * @pl: ptr to a pool_label_t struct
 * @buf: ptr to raw space where label info will be copied
 *
 * This function is important because it takes care of all of
 * the endian issues when copying to disk.  This way, when
 * machines of different architectures are used, they will
 * be able to interpret ondisk labels correctly.  Always use
 * this function before writing to disk.
 */
void pool_label_out(struct pool_disk *pl, void *buf)
{
	struct pool_disk *bufpl = (struct pool_disk *) buf;

	CPOUT_64(pl->pl_magic, bufpl->pl_magic);
	CPOUT_64(pl->pl_pool_id, bufpl->pl_pool_id);
	CPOUT_8(pl->pl_pool_name, bufpl->pl_pool_name, POOL_NAME_SIZE);
	CPOUT_32(pl->pl_version, bufpl->pl_version);
	CPOUT_32(pl->pl_subpools, bufpl->pl_subpools);
	CPOUT_32(pl->pl_sp_id, bufpl->pl_sp_id);
	CPOUT_32(pl->pl_sp_devs, bufpl->pl_sp_devs);
	CPOUT_32(pl->pl_sp_devid, bufpl->pl_sp_devid);
	CPOUT_32(pl->pl_sp_type, bufpl->pl_sp_type);
	CPOUT_64(pl->pl_blocks, bufpl->pl_blocks);
	CPOUT_32(pl->pl_striping, bufpl->pl_striping);
	CPOUT_32(pl->pl_sp_dmepdevs, bufpl->pl_sp_dmepdevs);
	CPOUT_32(pl->pl_sp_dmepid, bufpl->pl_sp_dmepid);
	CPOUT_32(pl->pl_sp_weight, bufpl->pl_sp_weight);
	CPOUT_32(pl->pl_minor, bufpl->pl_minor);
	CPOUT_32(pl->pl_padding, bufpl->pl_padding);
	CPOUT_8(pl->pl_reserve, bufpl->pl_reserve, 184);
}

/**
 * pool_label_in - copies a char buffer into a pool_label_t
 * @pl: ptr to a pool_label_t struct
 * @buf: ptr to raw space where label info is copied from
 *
 * This function is important because it takes care of all of
 * the endian issues when information from disk is about to be
 * used.  This way, when machines of different architectures
 * are used, they will be able to interpret ondisk labels
 * correctly.  Always use this function before using labels that
 * were read from disk.
 */
void pool_label_in(struct pool_disk *pl, void *buf)
{
	struct pool_disk *bufpl = (struct pool_disk *) buf;

	CPIN_64(pl->pl_magic, bufpl->pl_magic);
	CPIN_64(pl->pl_pool_id, bufpl->pl_pool_id);
	CPIN_8(pl->pl_pool_name, bufpl->pl_pool_name, POOL_NAME_SIZE);
	CPIN_32(pl->pl_version, bufpl->pl_version);
	CPIN_32(pl->pl_subpools, bufpl->pl_subpools);
	CPIN_32(pl->pl_sp_id, bufpl->pl_sp_id);
	CPIN_32(pl->pl_sp_devs, bufpl->pl_sp_devs);
	CPIN_32(pl->pl_sp_devid, bufpl->pl_sp_devid);
	CPIN_32(pl->pl_sp_type, bufpl->pl_sp_type);
	CPIN_64(pl->pl_blocks, bufpl->pl_blocks);
	CPIN_32(pl->pl_striping, bufpl->pl_striping);
	CPIN_32(pl->pl_sp_dmepdevs, bufpl->pl_sp_dmepdevs);
	CPIN_32(pl->pl_sp_dmepid, bufpl->pl_sp_dmepid);
	CPIN_32(pl->pl_sp_weight, bufpl->pl_sp_weight);
	CPIN_32(pl->pl_minor, bufpl->pl_minor);
	CPIN_32(pl->pl_padding, bufpl->pl_padding);
	CPIN_8(pl->pl_reserve, bufpl->pl_reserve, 184);
}

static char _calc_char(unsigned int id)
{
	/*
	 * [0-9A-Za-z!#] - 64 printable chars (6-bits)
	 */

	if (id < 10)
		return id + 48;
	if (id < 36)
		return (id - 10) + 65;
	if (id < 62)
		return (id - 36) + 97;
	if (id == 62)
		return '!';
	if (id == 63)
		return '#';

	return '%';
}

void get_pool_uuid(char *uuid, uint64_t poolid, uint32_t spid, uint32_t devid)
{
	int i;
	unsigned shifter = 0x003F;

	assert(ID_LEN == 32);
	memset(uuid, 0, ID_LEN);
	strcat(uuid, "POOL0000000000");

	/* We grab the entire 64 bits (+2 that get shifted in) */
	for (i = 13; i < 24; i++) {
		uuid[i] = _calc_char(((unsigned) poolid) & shifter);
		poolid = poolid >> 6;
	}

	/* We grab the entire 32 bits (+4 that get shifted in) */
	for (i = 24; i < 30; i++) {
		uuid[i] = _calc_char((unsigned) (spid & shifter));
		spid = spid >> 6;
	}

	/*
	 * Since we can only have 128 devices, we only worry about the
	 * last 12 bits
	 */
	for (i = 30; i < 32; i++) {
		uuid[i] = _calc_char((unsigned) (devid & shifter));
		devid = devid >> 6;
	}

}

static int _read_vg_pds(const struct format_type *fmt, struct dm_pool *mem,
			struct lvmcache_vginfo *vginfo, struct dm_list *head,
			uint32_t *devcount)
{
	struct lvmcache_info *info;
	struct pool_list *pl = NULL;
	struct dm_pool *tmpmem;

	uint32_t sp_count = 0;
	uint32_t *sp_devs = NULL;
	uint32_t i;

	/* FIXME: maybe should return a different error in memory
	 * allocation failure */
	if (!(tmpmem = dm_pool_create("pool read_vg", 512)))
		return_0;

	dm_list_iterate_items(info, &vginfo->infos) {
		if (info->dev &&
		    !(pl = read_pool_disk(fmt, info->dev, mem, vginfo->vgname)))
			    break;
		/*
		 * We need to keep track of the total expected number
		 * of devices per subpool
		 */
		if (!sp_count) {
			/* FIXME pl left uninitialised if !info->dev */
			sp_count = pl->pd.pl_subpools;
			if (!(sp_devs =
			      dm_pool_zalloc(tmpmem,
					  sizeof(uint32_t) * sp_count))) {
				log_error("Unable to allocate %d 32-bit uints",
					  sp_count);
				dm_pool_destroy(tmpmem);
				return 0;
			}
		}
		/*
		 * watch out for a pool label with a different subpool
		 * count than the original - give up if it does
		 */
		if (sp_count != pl->pd.pl_subpools)
			break;

		_add_pl_to_list(head, pl);

		if (sp_count > pl->pd.pl_sp_id && sp_devs[pl->pd.pl_sp_id] == 0)
			sp_devs[pl->pd.pl_sp_id] = pl->pd.pl_sp_devs;
	}

	*devcount = 0;
	for (i = 0; i < sp_count; i++)
		*devcount += sp_devs[i];

	dm_pool_destroy(tmpmem);

	if (pl && *pl->pd.pl_pool_name)
		return 1;

	return 0;

}

int read_pool_pds(const struct format_type *fmt, const char *vg_name,
		  struct dm_pool *mem, struct dm_list *pdhead)
{
	struct lvmcache_vginfo *vginfo;
	uint32_t totaldevs;
	int full_scan = -1;

	do {
		/*
		 * If the cache scanning doesn't work, this will never work
		 */
		if (vg_name && (vginfo = vginfo_from_vgname(vg_name, NULL)) &&
		    vginfo->infos.n) {

			if (_read_vg_pds(fmt, mem, vginfo, pdhead, &totaldevs)) {
				/*
				 * If we found all the devices we were
				 * expecting, return success
				 */
				if (dm_list_size(pdhead) == totaldevs)
					return 1;

				/*
				 * accept partial pool if we've done a full
				 * rescan of the cache
				 */
				if (full_scan > 0)
					return 1;
			}
		}
		/* Failed */
		dm_list_init(pdhead);

		full_scan++;
		if (full_scan > 1) {
			log_debug("No devices for vg %s found in cache",
				  vg_name);
			return 0;
		}
		lvmcache_label_scan(fmt->cmd, full_scan);

	} while (1);

}

struct pool_list *read_pool_disk(const struct format_type *fmt,
				 struct device *dev, struct dm_pool *mem,
				 const char *vg_name)
{
	struct pool_list *pl;

	if (!dev_open(dev))
		return_NULL;

	if (!(pl = dm_pool_zalloc(mem, sizeof(*pl)))) {
		log_error("Unable to allocate pool list structure");
		return 0;
	}

	if (!__read_pool_disk(fmt, dev, mem, pl, vg_name))
		return_NULL;

	if (!dev_close(dev))
		stack;

	return pl;

}
