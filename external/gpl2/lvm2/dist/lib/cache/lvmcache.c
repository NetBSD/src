/*	$NetBSD: lvmcache.c,v 1.1.1.1.2.1 2009/05/13 18:52:42 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
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
#include "lvmcache.h"
#include "toolcontext.h"
#include "dev-cache.h"
#include "locking.h"
#include "metadata.h"
#include "filter.h"
#include "memlock.h"
#include "str_list.h"
#include "format-text.h"
#include "format_pool.h"
#include "format1.h"

static struct dm_hash_table *_pvid_hash = NULL;
static struct dm_hash_table *_vgid_hash = NULL;
static struct dm_hash_table *_vgname_hash = NULL;
static struct dm_hash_table *_lock_hash = NULL;
static struct dm_list _vginfos;
static int _scanning_in_progress = 0;
static int _has_scanned = 0;
static int _vgs_locked = 0;
static int _vg_global_lock_held = 0;	/* Global lock held when cache wiped? */

int lvmcache_init(void)
{
	dm_list_init(&_vginfos);

	if (!(_vgname_hash = dm_hash_create(128)))
		return 0;

	if (!(_vgid_hash = dm_hash_create(128)))
		return 0;

	if (!(_pvid_hash = dm_hash_create(128)))
		return 0;

	if (!(_lock_hash = dm_hash_create(128)))
		return 0;

	if (_vg_global_lock_held)
		lvmcache_lock_vgname(VG_GLOBAL, 0);

	return 1;
}

/* Volume Group metadata cache functions */
static void _free_cached_vgmetadata(struct lvmcache_vginfo *vginfo)
{
	if (!vginfo || !vginfo->vgmetadata)
		return;

	dm_free(vginfo->vgmetadata);

	vginfo->vgmetadata = NULL;

	log_debug("Metadata cache: VG %s wiped.", vginfo->vgname);
}

static void _store_metadata(struct lvmcache_vginfo *vginfo,
			    struct volume_group *vg, unsigned precommitted)
{
	int size;

	if (vginfo->vgmetadata)
		_free_cached_vgmetadata(vginfo);

	if (!(size = export_vg_to_buffer(vg, &vginfo->vgmetadata))) {
		stack;
		return;
	}

	vginfo->precommitted = precommitted;

	log_debug("Metadata cache: VG %s stored (%d bytes%s).", vginfo->vgname,
		  size, precommitted ? ", precommitted" : "");
}

static void _update_cache_info_lock_state(struct lvmcache_info *info,
					  int locked,
					  int *cached_vgmetadata_valid)
{
	int was_locked = (info->status & CACHE_LOCKED) ? 1 : 0;

	/*
	 * Cache becomes invalid whenever lock state changes unless
	 * exclusive VG_GLOBAL is held (i.e. while scanning).
	 */
	if (!vgname_is_locked(VG_GLOBAL) && (was_locked != locked)) {
		info->status |= CACHE_INVALID;
		*cached_vgmetadata_valid = 0;
	}

	if (locked)
		info->status |= CACHE_LOCKED;
	else
		info->status &= ~CACHE_LOCKED;
}

static void _update_cache_vginfo_lock_state(struct lvmcache_vginfo *vginfo,
					    int locked)
{
	struct lvmcache_info *info;
	int cached_vgmetadata_valid = 1;

	dm_list_iterate_items(info, &vginfo->infos)
		_update_cache_info_lock_state(info, locked,
					      &cached_vgmetadata_valid);

	if (!cached_vgmetadata_valid)
		_free_cached_vgmetadata(vginfo);
}

static void _update_cache_lock_state(const char *vgname, int locked)
{
	struct lvmcache_vginfo *vginfo;

	if (!(vginfo = vginfo_from_vgname(vgname, NULL)))
		return;

	_update_cache_vginfo_lock_state(vginfo, locked);
}

static void _drop_metadata(const char *vgname)
{
	struct lvmcache_vginfo *vginfo;
	struct lvmcache_info *info;

	if (!(vginfo = vginfo_from_vgname(vgname, NULL)))
		return;

	/*
	 * Invalidate cached PV labels.
	 * If cached precommitted metadata exists that means we
	 * already invalidated the PV labels (before caching it)
	 * and we must not do it again.
	 */

	if (!vginfo->precommitted)
		dm_list_iterate_items(info, &vginfo->infos)
			info->status |= CACHE_INVALID;

	_free_cached_vgmetadata(vginfo);
}

void lvmcache_drop_metadata(const char *vgname)
{
	/* For VG_ORPHANS, we need to invalidate all labels on orphan PVs. */
	if (!strcmp(vgname, VG_ORPHANS)) {
		_drop_metadata(FMT_TEXT_ORPHAN_VG_NAME);
		_drop_metadata(FMT_LVM1_ORPHAN_VG_NAME);
		_drop_metadata(FMT_POOL_ORPHAN_VG_NAME);

		/* Indicate that PVs could now be missing from the cache */
		init_full_scan_done(0);
	} else if (!vgname_is_locked(VG_GLOBAL))
		_drop_metadata(vgname);
}

void lvmcache_lock_vgname(const char *vgname, int read_only __attribute((unused)))
{
	if (!_lock_hash && !lvmcache_init()) {
		log_error("Internal cache initialisation failed");
		return;
	}

	if (dm_hash_lookup(_lock_hash, vgname))
		log_error("Internal error: Nested locking attempted on VG %s.",
			  vgname);
		
	if (!dm_hash_insert(_lock_hash, vgname, (void *) 1))
		log_error("Cache locking failure for %s", vgname);

	_update_cache_lock_state(vgname, 1);

	if (strcmp(vgname, VG_GLOBAL))
		_vgs_locked++;
}

int vgname_is_locked(const char *vgname)
{
	if (!_lock_hash)
		return 0;

	return dm_hash_lookup(_lock_hash, vgname) ? 1 : 0;
}

void lvmcache_unlock_vgname(const char *vgname)
{
	if (!dm_hash_lookup(_lock_hash, vgname))
		log_error("Internal error: Attempt to unlock unlocked VG %s.",
			  vgname);

	_update_cache_lock_state(vgname, 0);

	dm_hash_remove(_lock_hash, vgname);

	/* FIXME Do this per-VG */
	if (strcmp(vgname, VG_GLOBAL) && !--_vgs_locked)
		dev_close_all();
}

int vgs_locked(void)
{
	return _vgs_locked;
}

static void _vginfo_attach_info(struct lvmcache_vginfo *vginfo,
				struct lvmcache_info *info)
{
	if (!vginfo)
		return;

	info->vginfo = vginfo;
	dm_list_add(&vginfo->infos, &info->list);
}

static void _vginfo_detach_info(struct lvmcache_info *info)
{
	if (!dm_list_empty(&info->list)) {
		dm_list_del(&info->list);
		dm_list_init(&info->list);
	}

	info->vginfo = NULL;
}

/* If vgid supplied, require a match. */
struct lvmcache_vginfo *vginfo_from_vgname(const char *vgname, const char *vgid)
{
	struct lvmcache_vginfo *vginfo;

	if (!vgname)
		return vginfo_from_vgid(vgid);

	if (!_vgname_hash)
		return NULL;

	if (!(vginfo = dm_hash_lookup(_vgname_hash, vgname)))
		return NULL;

	if (vgid)
		do
			if (!strncmp(vgid, vginfo->vgid, ID_LEN))
				return vginfo;
		while ((vginfo = vginfo->next));

	return vginfo;
}

const struct format_type *fmt_from_vgname(const char *vgname, const char *vgid)
{
	struct lvmcache_vginfo *vginfo;
	struct lvmcache_info *info;
	struct label *label;
	struct dm_list *devh, *tmp;
	struct dm_list devs;
	struct device_list *devl;
	char vgid_found[ID_LEN + 1] __attribute((aligned(8)));

	if (!(vginfo = vginfo_from_vgname(vgname, vgid)))
		return NULL;

	/* This function is normally called before reading metadata so
 	 * we check cached labels here. Unfortunately vginfo is volatile. */
	dm_list_init(&devs);
	dm_list_iterate_items(info, &vginfo->infos) {
		if (!(devl = dm_malloc(sizeof(*devl)))) {
			log_error("device_list element allocation failed");
			return NULL;
		}
		devl->dev = info->dev;
		dm_list_add(&devs, &devl->list);
	}

	memcpy(vgid_found, vginfo->vgid, sizeof(vgid_found));

	dm_list_iterate_safe(devh, tmp, &devs) {
		devl = dm_list_item(devh, struct device_list);
		label_read(devl->dev, &label, UINT64_C(0));
		dm_list_del(&devl->list);
		dm_free(devl);
	}

	/* If vginfo changed, caller needs to rescan */
	if (!(vginfo = vginfo_from_vgname(vgname, vgid_found)) ||
	    strncmp(vginfo->vgid, vgid_found, ID_LEN))
		return NULL;

	return vginfo->fmt;
}

struct lvmcache_vginfo *vginfo_from_vgid(const char *vgid)
{
	struct lvmcache_vginfo *vginfo;
	char id[ID_LEN + 1] __attribute((aligned(8)));

	if (!_vgid_hash || !vgid)
		return NULL;

	/* vgid not necessarily NULL-terminated */
	strncpy(&id[0], vgid, ID_LEN);
	id[ID_LEN] = '\0';

	if (!(vginfo = dm_hash_lookup(_vgid_hash, id)))
		return NULL;

	return vginfo;
}

const char *vgname_from_vgid(struct dm_pool *mem, const char *vgid)
{
	struct lvmcache_vginfo *vginfo;
	const char *vgname = NULL;

	if ((vginfo = vginfo_from_vgid(vgid)))
		vgname = vginfo->vgname;

	if (mem && vgname)
		return dm_pool_strdup(mem, vgname);

	return vgname;
}

static int _info_is_valid(struct lvmcache_info *info)
{
	if (info->status & CACHE_INVALID)
		return 0;

	/*
	 * The caller must hold the VG lock to manipulate metadata.
	 * In a cluster, remote nodes sometimes read metadata in the
	 * knowledge that the controlling node is holding the lock.
	 * So if the VG appears to be unlocked here, it should be safe
	 * to use the cached value.
	 */
	if (info->vginfo && !vgname_is_locked(info->vginfo->vgname))
		return 1;

	if (!(info->status & CACHE_LOCKED))
		return 0;

	return 1;
}

static int _vginfo_is_valid(struct lvmcache_vginfo *vginfo)
{
	struct lvmcache_info *info;

	/* Invalid if any info is invalid */
	dm_list_iterate_items(info, &vginfo->infos)
		if (!_info_is_valid(info))
			return 0;

	return 1;
}

/* vginfo is invalid if it does not contain at least one valid info */
static int _vginfo_is_invalid(struct lvmcache_vginfo *vginfo)
{
	struct lvmcache_info *info;

	dm_list_iterate_items(info, &vginfo->infos)
		if (_info_is_valid(info))
			return 0;

	return 1;
}

/*
 * If valid_only is set, data will only be returned if the cached data is
 * known still to be valid.
 */
struct lvmcache_info *info_from_pvid(const char *pvid, int valid_only)
{
	struct lvmcache_info *info;
	char id[ID_LEN + 1] __attribute((aligned(8)));

	if (!_pvid_hash || !pvid)
		return NULL;

	strncpy(&id[0], pvid, ID_LEN);
	id[ID_LEN] = '\0';

	if (!(info = dm_hash_lookup(_pvid_hash, id)))
		return NULL;

	if (valid_only && !_info_is_valid(info))
		return NULL;

	return info;
}

static void _rescan_entry(struct lvmcache_info *info)
{
	struct label *label;

	if (info->status & CACHE_INVALID)
		label_read(info->dev, &label, UINT64_C(0));
}

static int _scan_invalid(void)
{
	dm_hash_iter(_pvid_hash, (dm_hash_iterate_fn) _rescan_entry);

	return 1;
}

int lvmcache_label_scan(struct cmd_context *cmd, int full_scan)
{
	struct label *label;
	struct dev_iter *iter;
	struct device *dev;
	struct format_type *fmt;

	int r = 0;

	/* Avoid recursion when a PVID can't be found! */
	if (_scanning_in_progress)
		return 0;

	_scanning_in_progress = 1;

	if (!_vgname_hash && !lvmcache_init()) {
		log_error("Internal cache initialisation failed");
		goto out;
	}

	if (_has_scanned && !full_scan) {
		r = _scan_invalid();
		goto out;
	}

	if (!(iter = dev_iter_create(cmd->filter, (full_scan == 2) ? 1 : 0))) {
		log_error("dev_iter creation failed");
		goto out;
	}

	while ((dev = dev_iter_get(iter)))
		label_read(dev, &label, UINT64_C(0));

	dev_iter_destroy(iter);

	_has_scanned = 1;

	/* Perform any format-specific scanning e.g. text files */
	dm_list_iterate_items(fmt, &cmd->formats) {
		if (fmt->ops->scan && !fmt->ops->scan(fmt))
			goto out;
	}

	r = 1;

      out:
	_scanning_in_progress = 0;

	return r;
}

struct volume_group *lvmcache_get_vg(const char *vgid, unsigned precommitted)
{
	struct lvmcache_vginfo *vginfo;
	struct volume_group *vg;
	struct format_instance *fid;

	if (!vgid || !(vginfo = vginfo_from_vgid(vgid)) || !vginfo->vgmetadata)
		return NULL;

	if (!_vginfo_is_valid(vginfo))
		return NULL;

	/*
	 * Don't return cached data if either:
	 * (i)  precommitted metadata is requested but we don't have it cached
	 *      - caller should read it off disk;
	 * (ii) live metadata is requested but we have precommitted metadata cached
	 *      and no devices are suspended so caller may read it off disk.
	 *
	 * If live metadata is requested but we have precommitted metadata cached
	 * and devices are suspended, we assume this precommitted metadata has
	 * already been preloaded and committed so it's OK to return it as live.
	 * Note that we do not clear the PRECOMMITTED flag.
	 */
	if ((precommitted && !vginfo->precommitted) ||
	    (!precommitted && vginfo->precommitted && !memlock()))
		return NULL;

	if (!(fid =  vginfo->fmt->ops->create_instance(vginfo->fmt,
						       vginfo->vgname,
						       vgid, NULL)))
		return_NULL;

	if (!(vg = import_vg_from_buffer(vginfo->vgmetadata, fid)) ||
	    !vg_validate(vg)) {
		_free_cached_vgmetadata(vginfo);
		return_NULL;
	}

	log_debug("Using cached %smetadata for VG %s.",
		  vginfo->precommitted ? "pre-committed" : "", vginfo->vgname);

	return vg;
}

struct dm_list *lvmcache_get_vgids(struct cmd_context *cmd, int full_scan)
{
	struct dm_list *vgids;
	struct lvmcache_vginfo *vginfo;

	lvmcache_label_scan(cmd, full_scan);

	if (!(vgids = str_list_create(cmd->mem))) {
		log_error("vgids list allocation failed");
		return NULL;
	}

	dm_list_iterate_items(vginfo, &_vginfos) {
		if (!str_list_add(cmd->mem, vgids,
				  dm_pool_strdup(cmd->mem, vginfo->vgid))) {
			log_error("strlist allocation failed");
			return NULL;
		}
	}

	return vgids;
}

struct dm_list *lvmcache_get_vgnames(struct cmd_context *cmd, int full_scan)
{
	struct dm_list *vgnames;
	struct lvmcache_vginfo *vginfo;

	lvmcache_label_scan(cmd, full_scan);

	if (!(vgnames = str_list_create(cmd->mem))) {
		log_error("vgnames list allocation failed");
		return NULL;
	}

	dm_list_iterate_items(vginfo, &_vginfos) {
		if (!str_list_add(cmd->mem, vgnames,
				  dm_pool_strdup(cmd->mem, vginfo->vgname))) {
			log_error("strlist allocation failed");
			return NULL;
		}
	}

	return vgnames;
}

struct dm_list *lvmcache_get_pvids(struct cmd_context *cmd, const char *vgname,
				const char *vgid)
{
	struct dm_list *pvids;
	struct lvmcache_vginfo *vginfo;
	struct lvmcache_info *info;

	if (!(pvids = str_list_create(cmd->mem))) {
		log_error("pvids list allocation failed");
		return NULL;
	}

	if (!(vginfo = vginfo_from_vgname(vgname, vgid)))
		return pvids;

	dm_list_iterate_items(info, &vginfo->infos) {
		if (!str_list_add(cmd->mem, pvids,
				  dm_pool_strdup(cmd->mem, info->dev->pvid))) {
			log_error("strlist allocation failed");
			return NULL;
		}
	}

	return pvids;
}

struct device *device_from_pvid(struct cmd_context *cmd, struct id *pvid)
{
	struct label *label;
	struct lvmcache_info *info;

	/* Already cached ? */
	if ((info = info_from_pvid((char *) pvid, 0))) {
		if (label_read(info->dev, &label, UINT64_C(0))) {
			info = (struct lvmcache_info *) label->info;
			if (id_equal(pvid, (struct id *) &info->dev->pvid))
				return info->dev;
		}
	}

	lvmcache_label_scan(cmd, 0);

	/* Try again */
	if ((info = info_from_pvid((char *) pvid, 0))) {
		if (label_read(info->dev, &label, UINT64_C(0))) {
			info = (struct lvmcache_info *) label->info;
			if (id_equal(pvid, (struct id *) &info->dev->pvid))
				return info->dev;
		}
	}

	if (memlock())
		return NULL;

	lvmcache_label_scan(cmd, 2);

	/* Try again */
	if ((info = info_from_pvid((char *) pvid, 0))) {
		if (label_read(info->dev, &label, UINT64_C(0))) {
			info = (struct lvmcache_info *) label->info;
			if (id_equal(pvid, (struct id *) &info->dev->pvid))
				return info->dev;
		}
	}

	return NULL;
}

static int _free_vginfo(struct lvmcache_vginfo *vginfo)
{
	struct lvmcache_vginfo *primary_vginfo, *vginfo2;
	int r = 1;

	_free_cached_vgmetadata(vginfo);

	vginfo2 = primary_vginfo = vginfo_from_vgname(vginfo->vgname, NULL);

	if (vginfo == primary_vginfo) {
		dm_hash_remove(_vgname_hash, vginfo->vgname);
		if (vginfo->next && !dm_hash_insert(_vgname_hash, vginfo->vgname,
						    vginfo->next)) {
			log_error("_vgname_hash re-insertion for %s failed",
				  vginfo->vgname);
			r = 0;
		}
	} else do
		if (vginfo2->next == vginfo) {
			vginfo2->next = vginfo->next;
			break;
		}
 	while ((vginfo2 = primary_vginfo->next));

	if (vginfo->vgname)
		dm_free(vginfo->vgname);

	if (vginfo->creation_host)
		dm_free(vginfo->creation_host);

	if (*vginfo->vgid && _vgid_hash &&
	    vginfo_from_vgid(vginfo->vgid) == vginfo)
		dm_hash_remove(_vgid_hash, vginfo->vgid);

	dm_list_del(&vginfo->list);

	dm_free(vginfo);

	return r;
}

/*
 * vginfo must be info->vginfo unless info is NULL
 */
static int _drop_vginfo(struct lvmcache_info *info, struct lvmcache_vginfo *vginfo)
{
	if (info)
		_vginfo_detach_info(info);

	/* vginfo still referenced? */
	if (!vginfo || is_orphan_vg(vginfo->vgname) ||
	    !dm_list_empty(&vginfo->infos))
		return 1;

	if (!_free_vginfo(vginfo))
		return_0;

	return 1;
}

/* Unused
void lvmcache_del(struct lvmcache_info *info)
{
	if (info->dev->pvid[0] && _pvid_hash)
		dm_hash_remove(_pvid_hash, info->dev->pvid);

	_drop_vginfo(info, info->vginfo);

	info->label->labeller->ops->destroy_label(info->label->labeller,
						info->label);
	dm_free(info);

	return;
} */

static int _lvmcache_update_pvid(struct lvmcache_info *info, const char *pvid)
{
	/*
	 * Nothing to do if already stored with same pvid.
	 */
	if (((dm_hash_lookup(_pvid_hash, pvid)) == info) &&
	    !strcmp(info->dev->pvid, pvid))
		return 1;
	if (*info->dev->pvid)
		dm_hash_remove(_pvid_hash, info->dev->pvid);
	strncpy(info->dev->pvid, pvid, sizeof(info->dev->pvid));
	if (!dm_hash_insert(_pvid_hash, pvid, info)) {
		log_error("_lvmcache_update: pvid insertion failed: %s", pvid);
		return 0;
	}

	return 1;
}

/*
 * vginfo must be info->vginfo unless info is NULL (orphans)
 */
static int _lvmcache_update_vgid(struct lvmcache_info *info,
				 struct lvmcache_vginfo *vginfo,
				 const char *vgid)
{
	if (!vgid || !vginfo ||
	    !strncmp(vginfo->vgid, vgid, ID_LEN))
		return 1;

	if (vginfo && *vginfo->vgid)
		dm_hash_remove(_vgid_hash, vginfo->vgid);
	if (!vgid) {
		log_debug("lvmcache: %s: clearing VGID", info ? dev_name(info->dev) : vginfo->vgname);
		return 1;
	}

	strncpy(vginfo->vgid, vgid, ID_LEN);
	vginfo->vgid[ID_LEN] = '\0';
	if (!dm_hash_insert(_vgid_hash, vginfo->vgid, vginfo)) {
		log_error("_lvmcache_update: vgid hash insertion failed: %s",
			  vginfo->vgid);
		return 0;
	}

	if (!is_orphan_vg(vginfo->vgname))
		log_debug("lvmcache: %s: setting %s VGID to %s",
			  dev_name(info->dev), vginfo->vgname,
			  vginfo->vgid);

	return 1;
}

static int _insert_vginfo(struct lvmcache_vginfo *new_vginfo, const char *vgid,
			  uint32_t vgstatus, const char *creation_host,
			  struct lvmcache_vginfo *primary_vginfo)
{
	struct lvmcache_vginfo *last_vginfo = primary_vginfo;
	char uuid_primary[64] __attribute((aligned(8)));
	char uuid_new[64] __attribute((aligned(8)));
	int use_new = 0;
	
	/* Pre-existing VG takes precedence. Unexported VG takes precedence. */
	if (primary_vginfo) {
		if (!id_write_format((const struct id *)vgid, uuid_new, sizeof(uuid_new)))
			return_0;

		if (!id_write_format((const struct id *)&primary_vginfo->vgid, uuid_primary,
				     sizeof(uuid_primary)))
			return_0;

		/*
		 * If   Primary not exported, new exported => keep
		 * Else Primary exported, new not exported => change
		 * Else Primary has hostname for this machine => keep
		 * Else Primary has no hostname, new has one => change
		 * Else New has hostname for this machine => change
		 * Else Keep primary.
		 */
		if (!(primary_vginfo->status & EXPORTED_VG) &&
		    (vgstatus & EXPORTED_VG))
			log_error("WARNING: Duplicate VG name %s: "
				  "Existing %s takes precedence over "
				  "exported %s", new_vginfo->vgname,
				  uuid_primary, uuid_new);
		else if ((primary_vginfo->status & EXPORTED_VG) &&
			   !(vgstatus & EXPORTED_VG)) {
			log_error("WARNING: Duplicate VG name %s: "
				  "%s takes precedence over exported %s",
				  new_vginfo->vgname, uuid_new,
				  uuid_primary);
			use_new = 1;
		} else if (primary_vginfo->creation_host &&
			   !strcmp(primary_vginfo->creation_host,
				   primary_vginfo->fmt->cmd->hostname))
			log_error("WARNING: Duplicate VG name %s: "
				  "Existing %s (created here) takes precedence "
				  "over %s", new_vginfo->vgname, uuid_primary,
				  uuid_new);
		else if (!primary_vginfo->creation_host && creation_host) {
			log_error("WARNING: Duplicate VG name %s: "
				  "%s (with creation_host) takes precedence over %s",
				  new_vginfo->vgname, uuid_new,
				  uuid_primary);
			use_new = 1;
		} else if (creation_host &&
			   !strcmp(creation_host,
				   primary_vginfo->fmt->cmd->hostname)) {
			log_error("WARNING: Duplicate VG name %s: "
				  "%s (created here) takes precedence over %s",
				  new_vginfo->vgname, uuid_new,
				  uuid_primary);
			use_new = 1;
		}

		if (!use_new) {
			while (last_vginfo->next)
				last_vginfo = last_vginfo->next;
			last_vginfo->next = new_vginfo;
			return 1;
		}

		dm_hash_remove(_vgname_hash, primary_vginfo->vgname);
	}

	if (!dm_hash_insert(_vgname_hash, new_vginfo->vgname, new_vginfo)) {
		log_error("cache_update: vg hash insertion failed: %s",
		  	new_vginfo->vgname);
		return 0;
	}

	if (primary_vginfo)
		new_vginfo->next = primary_vginfo;

	return 1;
}

static int _lvmcache_update_vgname(struct lvmcache_info *info,
				   const char *vgname, const char *vgid,
				   uint32_t vgstatus, const char *creation_host,
				   const struct format_type *fmt)
{
	struct lvmcache_vginfo *vginfo, *primary_vginfo, *orphan_vginfo;
	struct lvmcache_info *info2, *info3;
	char mdabuf[32];
	// struct lvmcache_vginfo  *old_vginfo, *next;

	if (!vgname || (info && info->vginfo && !strcmp(info->vginfo->vgname, vgname)))
		return 1;

	/* Remove existing vginfo entry */
	if (info)
		_drop_vginfo(info, info->vginfo);

	/* Get existing vginfo or create new one */
	if (!(vginfo = vginfo_from_vgname(vgname, vgid))) {
/*** FIXME - vginfo ends up duplicated instead of renamed.
		// Renaming?  This lookup fails.
		if ((vginfo = vginfo_from_vgid(vgid))) {
			next = vginfo->next;
			old_vginfo = vginfo_from_vgname(vginfo->vgname, NULL);
			if (old_vginfo == vginfo) {
				dm_hash_remove(_vgname_hash, old_vginfo->vgname);
				if (old_vginfo->next) {
					if (!dm_hash_insert(_vgname_hash, old_vginfo->vgname, old_vginfo->next)) {
						log_error("vg hash re-insertion failed: %s",
							  old_vginfo->vgname);
						return 0;
					}
				}
			} else do {
				if (old_vginfo->next == vginfo) {
					old_vginfo->next = vginfo->next;
					break;
				}
			} while ((old_vginfo = old_vginfo->next));
			vginfo->next = NULL;

			dm_free(vginfo->vgname);
			if (!(vginfo->vgname = dm_strdup(vgname))) {
				log_error("cache vgname alloc failed for %s", vgname);
				return 0;
			}

			// Rename so can assume new name does not already exist
			if (!dm_hash_insert(_vgname_hash, vginfo->vgname, vginfo->next)) {
				log_error("vg hash re-insertion failed: %s",
					  vginfo->vgname);
		      		return 0;
			}
		} else {
***/
		if (!(vginfo = dm_malloc(sizeof(*vginfo)))) {
			log_error("lvmcache_update_vgname: list alloc failed");
			return 0;
		}
		memset(vginfo, 0, sizeof(*vginfo));
		if (!(vginfo->vgname = dm_strdup(vgname))) {
			dm_free(vginfo);
			log_error("cache vgname alloc failed for %s", vgname);
			return 0;
		}
		dm_list_init(&vginfo->infos);

		/*
		 * If we're scanning and there's an invalidated entry, remove it.
		 * Otherwise we risk bogus warnings of duplicate VGs.
		 */
		while ((primary_vginfo = vginfo_from_vgname(vgname, NULL)) &&
		       _scanning_in_progress && _vginfo_is_invalid(primary_vginfo))
			dm_list_iterate_items_safe(info2, info3, &primary_vginfo->infos) {
				orphan_vginfo = vginfo_from_vgname(primary_vginfo->fmt->orphan_vg_name, NULL);
				_drop_vginfo(info2, primary_vginfo);	
				_vginfo_attach_info(orphan_vginfo, info2);
				if (info2->mdas.n)
					sprintf(mdabuf, " with %u mdas",
						dm_list_size(&info2->mdas));
				else
					mdabuf[0] = '\0';
				log_debug("lvmcache: %s: now in VG %s%s%s%s%s",
					  dev_name(info2->dev),
					  vgname, orphan_vginfo->vgid[0] ? " (" : "",
					  orphan_vginfo->vgid[0] ? orphan_vginfo->vgid : "",
					  orphan_vginfo->vgid[0] ? ")" : "", mdabuf);
		}

		if (!_insert_vginfo(vginfo, vgid, vgstatus, creation_host,
				    primary_vginfo)) {
			dm_free(vginfo->vgname);
			dm_free(vginfo);
			return 0;
		}
		/* Ensure orphans appear last on list_iterate */
		if (is_orphan_vg(vgname))
			dm_list_add(&_vginfos, &vginfo->list);
		else
			dm_list_add_h(&_vginfos, &vginfo->list);
/***
		}
***/
	}

	if (info)
		_vginfo_attach_info(vginfo, info);
	else if (!_lvmcache_update_vgid(NULL, vginfo, vgid)) /* Orphans */
		return_0;

	_update_cache_vginfo_lock_state(vginfo, vgname_is_locked(vgname));

	/* FIXME Check consistency of list! */
	vginfo->fmt = fmt;

	if (info) {
		if (info->mdas.n)
			sprintf(mdabuf, " with %u mdas", dm_list_size(&info->mdas));
		else
			mdabuf[0] = '\0';
		log_debug("lvmcache: %s: now in VG %s%s%s%s%s",
			  dev_name(info->dev),
			  vgname, vginfo->vgid[0] ? " (" : "",
			  vginfo->vgid[0] ? vginfo->vgid : "",
			  vginfo->vgid[0] ? ")" : "", mdabuf);
	} else
		log_debug("lvmcache: initialised VG %s", vgname);

	return 1;
}

static int _lvmcache_update_vgstatus(struct lvmcache_info *info, uint32_t vgstatus,
				     const char *creation_host)
{
	if (!info || !info->vginfo)
		return 1;

	if ((info->vginfo->status & EXPORTED_VG) != (vgstatus & EXPORTED_VG))
		log_debug("lvmcache: %s: VG %s %s exported",
			  dev_name(info->dev), info->vginfo->vgname,
			  vgstatus & EXPORTED_VG ? "now" : "no longer");

	info->vginfo->status = vgstatus;

	if (!creation_host)
		return 1;

	if (info->vginfo->creation_host && !strcmp(creation_host,
						   info->vginfo->creation_host))
		return 1;

	if (info->vginfo->creation_host)
		dm_free(info->vginfo->creation_host);

	if (!(info->vginfo->creation_host = dm_strdup(creation_host))) {
		log_error("cache creation host alloc failed for %s",
			  creation_host);
		return 0;
	}

	log_debug("lvmcache: %s: VG %s: Set creation host to %s.",
		  dev_name(info->dev), info->vginfo->vgname, creation_host);

	return 1;
}

int lvmcache_add_orphan_vginfo(const char *vgname, struct format_type *fmt)
{
	if (!_lock_hash && !lvmcache_init()) {
		log_error("Internal cache initialisation failed");
		return 0;
	}

	return _lvmcache_update_vgname(NULL, vgname, vgname, 0, "", fmt);
}

int lvmcache_update_vgname_and_id(struct lvmcache_info *info,
				  const char *vgname, const char *vgid,
				  uint32_t vgstatus, const char *creation_host)
{
	if (!vgname && !info->vginfo) {
		log_error("Internal error: NULL vgname handed to cache");
		/* FIXME Remove this */
		vgname = info->fmt->orphan_vg_name;
		vgid = vgname;
	}

	/* If PV without mdas is already in a real VG, don't make it orphan */
	if (is_orphan_vg(vgname) && info->vginfo && !dm_list_size(&info->mdas) &&
	    !is_orphan_vg(info->vginfo->vgname) && memlock())
		return 1;

	/* If moving PV from orphan to real VG, always mark it valid */
	if (!is_orphan_vg(vgname))
		info->status &= ~CACHE_INVALID;

	if (!_lvmcache_update_vgname(info, vgname, vgid, vgstatus,
				     creation_host, info->fmt) ||
	    !_lvmcache_update_vgid(info, info->vginfo, vgid) ||
	    !_lvmcache_update_vgstatus(info, vgstatus, creation_host))
		return_0;

	return 1;
}

int lvmcache_update_vg(struct volume_group *vg, unsigned precommitted)
{
	struct pv_list *pvl;
	struct lvmcache_info *info;
	struct lvmcache_vginfo *vginfo;
	char pvid_s[ID_LEN + 1] __attribute((aligned(8)));

	pvid_s[sizeof(pvid_s) - 1] = '\0';

	dm_list_iterate_items(pvl, &vg->pvs) {
		strncpy(pvid_s, (char *) &pvl->pv->id, sizeof(pvid_s) - 1);
		/* FIXME Could pvl->pv->dev->pvid ever be different? */
		if ((info = info_from_pvid(pvid_s, 0)) &&
		    !lvmcache_update_vgname_and_id(info, vg->name,
						   (char *) &vg->id,
						   vg->status, NULL))
			return_0;
	}

	/* store text representation of vg to cache */
	if (vg->cmd->current_settings.cache_vgmetadata &&
	    (vginfo = vginfo_from_vgname(vg->name, NULL)))
		_store_metadata(vginfo, vg, precommitted);

	return 1;
}

struct lvmcache_info *lvmcache_add(struct labeller *labeller, const char *pvid,
				   struct device *dev,
				   const char *vgname, const char *vgid,
				   uint32_t vgstatus)
{
	struct label *label;
	struct lvmcache_info *existing, *info;
	char pvid_s[ID_LEN + 1] __attribute((aligned(8)));

	if (!_vgname_hash && !lvmcache_init()) {
		log_error("Internal cache initialisation failed");
		return NULL;
	}

	strncpy(pvid_s, pvid, sizeof(pvid_s));
	pvid_s[sizeof(pvid_s) - 1] = '\0';

	if (!(existing = info_from_pvid(pvid_s, 0)) &&
	    !(existing = info_from_pvid(dev->pvid, 0))) {
		if (!(label = label_create(labeller)))
			return_NULL;
		if (!(info = dm_malloc(sizeof(*info)))) {
			log_error("lvmcache_info allocation failed");
			label_destroy(label);
			return NULL;
		}
		memset(info, 0, sizeof(*info));

		label->info = info;
		info->label = label;
		dm_list_init(&info->list);
		info->dev = dev;
	} else {
		if (existing->dev != dev) {
			/* Is the existing entry a duplicate pvid e.g. md ? */
			if (MAJOR(existing->dev->dev) == md_major() &&
			    MAJOR(dev->dev) != md_major()) {
				log_very_verbose("Ignoring duplicate PV %s on "
						 "%s - using md %s",
						 pvid, dev_name(dev),
						 dev_name(existing->dev));
				return NULL;
			} else if (dm_is_dm_major(MAJOR(existing->dev->dev)) &&
				   !dm_is_dm_major(MAJOR(dev->dev))) {
				log_very_verbose("Ignoring duplicate PV %s on "
						 "%s - using dm %s",
						 pvid, dev_name(dev),
						 dev_name(existing->dev));
				return NULL;
			} else if (MAJOR(existing->dev->dev) != md_major() &&
				   MAJOR(dev->dev) == md_major())
				log_very_verbose("Duplicate PV %s on %s - "
						 "using md %s", pvid,
						 dev_name(existing->dev),
						 dev_name(dev));
			else if (!dm_is_dm_major(MAJOR(existing->dev->dev)) &&
				 dm_is_dm_major(MAJOR(dev->dev)))
				log_very_verbose("Duplicate PV %s on %s - "
						 "using dm %s", pvid,
						 dev_name(existing->dev),
						 dev_name(dev));
			/* FIXME If both dm, check dependencies */
			//else if (dm_is_dm_major(MAJOR(existing->dev->dev)) &&
				 //dm_is_dm_major(MAJOR(dev->dev)))
				 //
			else if (!strcmp(pvid_s, existing->dev->pvid)) 
				log_error("Found duplicate PV %s: using %s not "
					  "%s", pvid, dev_name(dev),
					  dev_name(existing->dev));
		}
		if (strcmp(pvid_s, existing->dev->pvid)) 
			log_debug("Updating pvid cache to %s (%s) from %s (%s)",
				  pvid_s, dev_name(dev),
				  existing->dev->pvid, dev_name(existing->dev));
		/* Switch over to new preferred device */
		existing->dev = dev;
		info = existing;
		/* Has labeller changed? */
		if (info->label->labeller != labeller) {
			label_destroy(info->label);
			if (!(info->label = label_create(labeller)))
				/* FIXME leaves info without label! */
				return_NULL;
			info->label->info = info;
		}
		label = info->label;
	}

	info->fmt = (const struct format_type *) labeller->private;
	info->status |= CACHE_INVALID;

	if (!_lvmcache_update_pvid(info, pvid_s)) {
		if (!existing) {
			dm_free(info);
			label_destroy(label);
		}
		return NULL;
	}

	if (!lvmcache_update_vgname_and_id(info, vgname, vgid, vgstatus, NULL)) {
		if (!existing) {
			dm_hash_remove(_pvid_hash, pvid_s);
			strcpy(info->dev->pvid, "");
			dm_free(info);
			label_destroy(label);
		}
		return NULL;
	}

	return info;
}

static void _lvmcache_destroy_entry(struct lvmcache_info *info)
{
	_vginfo_detach_info(info);
	strcpy(info->dev->pvid, "");
	label_destroy(info->label);
	dm_free(info);
}

static void _lvmcache_destroy_vgnamelist(struct lvmcache_vginfo *vginfo)
{
	struct lvmcache_vginfo *next;

	do {
		next = vginfo->next;
		if (!_free_vginfo(vginfo))
			stack;
	} while ((vginfo = next));
}

static void _lvmcache_destroy_lockname(struct dm_hash_node *n)
{
	char *vgname;

	if (!dm_hash_get_data(_lock_hash, n))
		return;

	vgname = dm_hash_get_key(_lock_hash, n);

	if (!strcmp(vgname, VG_GLOBAL))
		_vg_global_lock_held = 1;
	else
		log_error("Internal error: Volume Group %s was not unlocked",
			  dm_hash_get_key(_lock_hash, n));
}

void lvmcache_destroy(struct cmd_context *cmd, int retain_orphans)
{
	struct dm_hash_node *n;
	log_verbose("Wiping internal VG cache");

	_has_scanned = 0;

	if (_vgid_hash) {
		dm_hash_destroy(_vgid_hash);
		_vgid_hash = NULL;
	}

	if (_pvid_hash) {
		dm_hash_iter(_pvid_hash, (dm_hash_iterate_fn) _lvmcache_destroy_entry);
		dm_hash_destroy(_pvid_hash);
		_pvid_hash = NULL;
	}

	if (_vgname_hash) {
		dm_hash_iter(_vgname_hash,
			  (dm_hash_iterate_fn) _lvmcache_destroy_vgnamelist);
		dm_hash_destroy(_vgname_hash);
		_vgname_hash = NULL;
	}

	if (_lock_hash) {
		dm_hash_iterate(n, _lock_hash)
			_lvmcache_destroy_lockname(n);
		dm_hash_destroy(_lock_hash);
		_lock_hash = NULL;
	}

	if (!dm_list_empty(&_vginfos))
		log_error("Internal error: _vginfos list should be empty");
	dm_list_init(&_vginfos);

	if (retain_orphans)
		init_lvmcache_orphans(cmd);
}
