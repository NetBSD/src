/*	$NetBSD: dev-cache.c,v 1.1.1.2 2009/12/02 00:26:34 haad Exp $	*/

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
#include "dev-cache.h"
#include "lvm-types.h"
#include "btree.h"
#include "filter.h"
#include "filter-persistent.h"
#include "toolcontext.h"

#include <unistd.h>
#include <sys/param.h>
#include <dirent.h>

struct dev_iter {
	struct btree_iter *current;
	struct dev_filter *filter;
};

struct dir_list {
	struct dm_list list;
	char dir[0];
};

static struct {
	struct dm_pool *mem;
	struct dm_hash_table *names;
	struct btree *devices;
	struct dm_regex *preferred_names_matcher;

	int has_scanned;
	struct dm_list dirs;
	struct dm_list files;

} _cache;

#define _alloc(x) dm_pool_zalloc(_cache.mem, (x))
#define _free(x) dm_pool_free(_cache.mem, (x))
#define _strdup(x) dm_pool_strdup(_cache.mem, (x))

static int _insert(const char *path, int rec);

struct device *dev_create_file(const char *filename, struct device *dev,
			       struct str_list *alias, int use_malloc)
{
	int allocate = !dev;

	if (allocate) {
		if (use_malloc) {
			if (!(dev = dm_malloc(sizeof(*dev)))) {
				log_error("struct device allocation failed");
				return NULL;
			}
			if (!(alias = dm_malloc(sizeof(*alias)))) {
				log_error("struct str_list allocation failed");
				dm_free(dev);
				return NULL;
			}
			if (!(alias->str = dm_strdup(filename))) {
				log_error("filename strdup failed");
				dm_free(dev);
				dm_free(alias);
				return NULL;
			}
			dev->flags = DEV_ALLOCED;
		} else {
			if (!(dev = _alloc(sizeof(*dev)))) {
				log_error("struct device allocation failed");
				return NULL;
			}
			if (!(alias = _alloc(sizeof(*alias)))) {
				log_error("struct str_list allocation failed");
				_free(dev);
				return NULL;
			}
			if (!(alias->str = _strdup(filename))) {
				log_error("filename strdup failed");
				return NULL;
			}
		}
	} else if (!(alias->str = dm_strdup(filename))) {
		log_error("filename strdup failed");
		return NULL;
	}

	dev->flags |= DEV_REGULAR;
	dm_list_init(&dev->aliases);
	dm_list_add(&dev->aliases, &alias->list);
	dev->end = UINT64_C(0);
	dev->dev = 0;
	dev->fd = -1;
	dev->open_count = 0;
	dev->block_size = -1;
	dev->read_ahead = -1;
	memset(dev->pvid, 0, sizeof(dev->pvid));
	dm_list_init(&dev->open_list);

	return dev;
}

static struct device *_dev_create(dev_t d)
{
	struct device *dev;

	if (!(dev = _alloc(sizeof(*dev)))) {
		log_error("struct device allocation failed");
		return NULL;
	}
	dev->flags = 0;
	dm_list_init(&dev->aliases);
	dev->dev = d;
	dev->fd = -1;
	dev->open_count = 0;
	dev->block_size = -1;
	dev->read_ahead = -1;
	dev->end = UINT64_C(0);
	memset(dev->pvid, 0, sizeof(dev->pvid));
	dm_list_init(&dev->open_list);

	return dev;
}

void dev_set_preferred_name(struct str_list *sl, struct device *dev)
{
	/*
	 * Don't interfere with ordering specified in config file.
	 */
	if (_cache.preferred_names_matcher)
		return;

	log_debug("%s: New preferred name", sl->str);
	dm_list_del(&sl->list);
	dm_list_add_h(&dev->aliases, &sl->list);
}

/* Return 1 if we prefer path1 else return 0 */
static int _compare_paths(const char *path0, const char *path1)
{
	int slash0 = 0, slash1 = 0;
	int m0, m1;
	const char *p;
	char p0[PATH_MAX], p1[PATH_MAX];
	char *s0, *s1;
	struct stat stat0, stat1;

	/*
	 * FIXME Better to compare patterns one-at-a-time against all names.
	 */
	if (_cache.preferred_names_matcher) {
		m0 = dm_regex_match(_cache.preferred_names_matcher, path0);
		m1 = dm_regex_match(_cache.preferred_names_matcher, path1);

		if (m0 != m1) {
			if (m0 < 0)
				return 1;
			if (m1 < 0)
				return 0;
			if (m0 < m1)
				return 1;
			if (m1 < m0)
				return 0;
		}
	}

	/*
	 * Built-in rules.
	 */

	/* Return the path with fewer slashes */
	for (p = path0; p++; p = (const char *) strchr(p, '/'))
		slash0++;

	for (p = path1; p++; p = (const char *) strchr(p, '/'))
		slash1++;

	if (slash0 < slash1)
		return 0;
	if (slash1 < slash0)
		return 1;

	strncpy(p0, path0, PATH_MAX);
	strncpy(p1, path1, PATH_MAX);
	s0 = &p0[0] + 1;
	s1 = &p1[0] + 1;

	/* We prefer symlinks - they exist for a reason!
	 * So we prefer a shorter path before the first symlink in the name.
	 * FIXME Configuration option to invert this? */
	while (s0) {
		s0 = strchr(s0, '/');
		s1 = strchr(s1, '/');
		if (s0) {
			*s0 = '\0';
			*s1 = '\0';
		}
		if (lstat(p0, &stat0)) {
			log_sys_very_verbose("lstat", p0);
			return 1;
		}
		if (lstat(p1, &stat1)) {
			log_sys_very_verbose("lstat", p1);
			return 0;
		}
		if (S_ISLNK(stat0.st_mode) && !S_ISLNK(stat1.st_mode))
			return 0;
		if (!S_ISLNK(stat0.st_mode) && S_ISLNK(stat1.st_mode))
			return 1;
		if (s0) {
			*s0++ = '/';
			*s1++ = '/';
		}
	}

	/* ASCII comparison */
	if (strcmp(path0, path1) < 0)
		return 0;
	else
		return 1;
}

static int _add_alias(struct device *dev, const char *path)
{
	struct str_list *sl = _alloc(sizeof(*sl));
	struct str_list *strl;
	const char *oldpath;
	int prefer_old = 1;

	if (!sl)
		return_0;

	/* Is name already there? */
	dm_list_iterate_items(strl, &dev->aliases) {
		if (!strcmp(strl->str, path)) {
			log_debug("%s: Already in device cache", path);
			return 1;
		}
	}

	if (!(sl->str = dm_pool_strdup(_cache.mem, path)))
		return_0;

	if (!dm_list_empty(&dev->aliases)) {
		oldpath = dm_list_item(dev->aliases.n, struct str_list)->str;
		prefer_old = _compare_paths(path, oldpath);
		log_debug("%s: Aliased to %s in device cache%s",
			  path, oldpath, prefer_old ? "" : " (preferred name)");

	} else
		log_debug("%s: Added to device cache", path);

	if (prefer_old)
		dm_list_add(&dev->aliases, &sl->list);
	else
		dm_list_add_h(&dev->aliases, &sl->list);

	return 1;
}

/*
 * Either creates a new dev, or adds an alias to
 * an existing dev.
 */
static int _insert_dev(const char *path, dev_t d)
{
	struct device *dev;
	static dev_t loopfile_count = 0;
	int loopfile = 0;

	/* Generate pretend device numbers for loopfiles */
	if (!d) {
		if (dm_hash_lookup(_cache.names, path))
			return 1;
		d = ++loopfile_count;
		loopfile = 1;
	}

	/* is this device already registered ? */
	if (!(dev = (struct device *) btree_lookup(_cache.devices,
						   (uint32_t) d))) {
		/* create new device */
		if (loopfile) {
			if (!(dev = dev_create_file(path, NULL, NULL, 0)))
				return_0;
		} else if (!(dev = _dev_create(d)))
			return_0;

		if (!(btree_insert(_cache.devices, (uint32_t) d, dev))) {
			log_error("Couldn't insert device into binary tree.");
			_free(dev);
			return 0;
		}
	}

	if (!loopfile && !_add_alias(dev, path)) {
		log_error("Couldn't add alias to dev cache.");
		return 0;
	}

	if (!dm_hash_insert(_cache.names, path, dev)) {
		log_error("Couldn't add name to hash in dev cache.");
		return 0;
	}

	return 1;
}

static char *_join(const char *dir, const char *name)
{
	size_t len = strlen(dir) + strlen(name) + 2;
	char *r = dm_malloc(len);
	if (r)
		snprintf(r, len, "%s/%s", dir, name);

	return r;
}

/*
 * Get rid of extra slashes in the path string.
 */
static void _collapse_slashes(char *str)
{
	char *ptr;
	int was_slash = 0;

	for (ptr = str; *ptr; ptr++) {
		if (*ptr == '/') {
			if (was_slash)
				continue;

			was_slash = 1;
		} else
			was_slash = 0;
		*str++ = *ptr;
	}

	*str = *ptr;
}

static int _insert_dir(const char *dir)
{
	int n, dirent_count, r = 1;
	struct dirent **dirent;
	char *path;

	dirent_count = scandir(dir, &dirent, NULL, alphasort);
	if (dirent_count > 0) {
		for (n = 0; n < dirent_count; n++) {
			if (dirent[n]->d_name[0] == '.') {
				free(dirent[n]);
				continue;
			}

			if (!(path = _join(dir, dirent[n]->d_name)))
				return_0;

			_collapse_slashes(path);
			r &= _insert(path, 1);
			dm_free(path);

			free(dirent[n]);
		}
		free(dirent);
	}

	return r;
}

static int _insert_file(const char *path)
{
	struct stat info;

	if (stat(path, &info) < 0) {
		log_sys_very_verbose("stat", path);
		return 0;
	}

	if (!S_ISREG(info.st_mode)) {
		log_debug("%s: Not a regular file", path);
		return 0;
	}

	if (!_insert_dev(path, 0))
		return_0;

	return 1;
}

static int _insert(const char *path, int rec)
{
	struct stat info;
	int r = 0;

	if (stat(path, &info) < 0) {
		log_sys_very_verbose("stat", path);
		return 0;
	}

	if (S_ISDIR(info.st_mode)) {	/* add a directory */
		/* check it's not a symbolic link */
		if (lstat(path, &info) < 0) {
			log_sys_very_verbose("lstat", path);
			return 0;
		}

		if (S_ISLNK(info.st_mode)) {
			log_debug("%s: Symbolic link to directory", path);
			return 0;
		}

		if (rec)
			r = _insert_dir(path);

	} else {		/* add a device */
		if (!S_ISBLK(info.st_mode)) {
			log_debug("%s: Not a block device", path);
			return 0;
		}

		if (!_insert_dev(path, info.st_rdev))
			return_0;

		r = 1;
	}

	return r;
}

static void _full_scan(int dev_scan)
{
	struct dir_list *dl;

	if (_cache.has_scanned && !dev_scan)
		return;

	dm_list_iterate_items(dl, &_cache.dirs)
		_insert_dir(dl->dir);

	dm_list_iterate_items(dl, &_cache.files)
		_insert_file(dl->dir);

	_cache.has_scanned = 1;
	init_full_scan_done(1);
}

int dev_cache_has_scanned(void)
{
	return _cache.has_scanned;
}

void dev_cache_scan(int do_scan)
{
	if (!do_scan)
		_cache.has_scanned = 1;
	else
		_full_scan(1);
}

static int _init_preferred_names(struct cmd_context *cmd)
{
	const struct config_node *cn;
	struct config_value *v;
	struct dm_pool *scratch = NULL;
	char **regex;
	unsigned count = 0;
	int i, r = 0;

	_cache.preferred_names_matcher = NULL;

	if (!(cn = find_config_tree_node(cmd, "devices/preferred_names")) ||
	    cn->v->type == CFG_EMPTY_ARRAY) {
		log_very_verbose("devices/preferred_names not found in config file: "
				 "using built-in preferences");
		return 1;
	}

	for (v = cn->v; v; v = v->next) {
		if (v->type != CFG_STRING) {
			log_error("preferred_names patterns must be enclosed in quotes");
			return 0;
		}

		count++;
	}

	if (!(scratch = dm_pool_create("preferred device name matcher", 1024)))
		return_0;

	if (!(regex = dm_pool_alloc(scratch, sizeof(*regex) * count))) {
		log_error("Failed to allocate preferred device name "
			  "pattern list.");
		goto out;
	}

	for (v = cn->v, i = count - 1; v; v = v->next, i--) {
		if (!(regex[i] = dm_pool_strdup(scratch, v->v.str))) {
			log_error("Failed to allocate a preferred device name "
				  "pattern.");
			goto out;
		}
	}

	if (!(_cache.preferred_names_matcher =
		dm_regex_create(_cache.mem,(const char **) regex, count))) {
		log_error("Preferred device name pattern matcher creation failed.");
		goto out;
	}

	r = 1;

out:
	dm_pool_destroy(scratch);

	return r;
}

int dev_cache_init(struct cmd_context *cmd)
{
	_cache.names = NULL;
	_cache.has_scanned = 0;

	if (!(_cache.mem = dm_pool_create("dev_cache", 10 * 1024)))
		return_0;

	if (!(_cache.names = dm_hash_create(128))) {
		dm_pool_destroy(_cache.mem);
		_cache.mem = 0;
		return_0;
	}

	if (!(_cache.devices = btree_create(_cache.mem))) {
		log_error("Couldn't create binary tree for dev-cache.");
		goto bad;
	}

	dm_list_init(&_cache.dirs);
	dm_list_init(&_cache.files);

	if (!_init_preferred_names(cmd))
		goto_bad;

	return 1;

      bad:
	dev_cache_exit();
	return 0;
}

static void _check_closed(struct device *dev)
{
	if (dev->fd >= 0)
		log_error("Device '%s' has been left open.", dev_name(dev));
}

static void _check_for_open_devices(void)
{
	dm_hash_iter(_cache.names, (dm_hash_iterate_fn) _check_closed);
}

void dev_cache_exit(void)
{
	if (_cache.names)
		_check_for_open_devices();

	if (_cache.preferred_names_matcher)
		_cache.preferred_names_matcher = NULL;

	if (_cache.mem) {
		dm_pool_destroy(_cache.mem);
		_cache.mem = NULL;
	}

	if (_cache.names) {
		dm_hash_destroy(_cache.names);
		_cache.names = NULL;
	}

	_cache.devices = NULL;
	_cache.has_scanned = 0;
	dm_list_init(&_cache.dirs);
	dm_list_init(&_cache.files);
}

int dev_cache_add_dir(const char *path)
{
	struct dir_list *dl;
	struct stat st;

	if (stat(path, &st)) {
		log_error("Ignoring %s: %s", path, strerror(errno));
		/* But don't fail */
		return 1;
	}

	if (!S_ISDIR(st.st_mode)) {
		log_error("Ignoring %s: Not a directory", path);
		return 1;
	}

	if (!(dl = _alloc(sizeof(*dl) + strlen(path) + 1))) {
		log_error("dir_list allocation failed");
		return 0;
	}

	strcpy(dl->dir, path);
	dm_list_add(&_cache.dirs, &dl->list);
	return 1;
}

int dev_cache_add_loopfile(const char *path)
{
	struct dir_list *dl;
	struct stat st;

	if (stat(path, &st)) {
		log_error("Ignoring %s: %s", path, strerror(errno));
		/* But don't fail */
		return 1;
	}

	if (!S_ISREG(st.st_mode)) {
		log_error("Ignoring %s: Not a regular file", path);
		return 1;
	}

	if (!(dl = _alloc(sizeof(*dl) + strlen(path) + 1))) {
		log_error("dir_list allocation failed for file");
		return 0;
	}

	strcpy(dl->dir, path);
	dm_list_add(&_cache.files, &dl->list);
	return 1;
}

/* Check cached device name is still valid before returning it */
/* This should be a rare occurrence */
/* set quiet if the cache is expected to be out-of-date */
/* FIXME Make rest of code pass/cache struct device instead of dev_name */
const char *dev_name_confirmed(struct device *dev, int quiet)
{
	struct stat buf;
	const char *name;
	int r;

	if ((dev->flags & DEV_REGULAR))
		return dev_name(dev);

	while ((r = stat(name = dm_list_item(dev->aliases.n,
					  struct str_list)->str, &buf)) ||
	       (buf.st_rdev != dev->dev)) {
		if (r < 0) {
			if (quiet)
				log_sys_debug("stat", name);
			else
				log_sys_error("stat", name);
		}
		if (quiet)
			log_debug("Path %s no longer valid for device(%d,%d)",
				  name, (int) MAJOR(dev->dev),
				  (int) MINOR(dev->dev));
		else
			log_error("Path %s no longer valid for device(%d,%d)",
				  name, (int) MAJOR(dev->dev),
				  (int) MINOR(dev->dev));

		/* Remove the incorrect hash entry */
		dm_hash_remove(_cache.names, name);

		/* Leave list alone if there isn't an alternative name */
		/* so dev_name will always find something to return. */
		/* Otherwise add the name to the correct device. */
		if (dm_list_size(&dev->aliases) > 1) {
			dm_list_del(dev->aliases.n);
			if (!r)
				_insert(name, 0);
			continue;
		}

		/* Scanning issues this inappropriately sometimes. */
		log_debug("Aborting - please provide new pathname for what "
			  "used to be %s", name);
		return NULL;
	}

	return dev_name(dev);
}

struct device *dev_cache_get(const char *name, struct dev_filter *f)
{
	struct stat buf;
	struct device *d = (struct device *) dm_hash_lookup(_cache.names, name);

	if (d && (d->flags & DEV_REGULAR))
		return d;

	/* If the entry's wrong, remove it */
	if (d && (stat(name, &buf) || (buf.st_rdev != d->dev))) {
		dm_hash_remove(_cache.names, name);
		d = NULL;
	}

	if (!d) {
		_insert(name, 0);
		d = (struct device *) dm_hash_lookup(_cache.names, name);
		if (!d) {
			_full_scan(0);
			d = (struct device *) dm_hash_lookup(_cache.names, name);
		}
	}

	return (d && (!f || (d->flags & DEV_REGULAR) ||
		      f->passes_filter(f, d))) ? d : NULL;
}

struct dev_iter *dev_iter_create(struct dev_filter *f, int dev_scan)
{
	struct dev_iter *di = dm_malloc(sizeof(*di));

	if (!di) {
		log_error("dev_iter allocation failed");
		return NULL;
	}

	if (dev_scan && !trust_cache()) {
		/* Flag gets reset between each command */
		if (!full_scan_done())
			persistent_filter_wipe(f); /* Calls _full_scan(1) */
	} else
		_full_scan(0);

	di->current = btree_first(_cache.devices);
	di->filter = f;

	return di;
}

void dev_iter_destroy(struct dev_iter *iter)
{
	dm_free(iter);
}

static struct device *_iter_next(struct dev_iter *iter)
{
	struct device *d = btree_get_data(iter->current);
	iter->current = btree_next(iter->current);
	return d;
}

struct device *dev_iter_get(struct dev_iter *iter)
{
	while (iter->current) {
		struct device *d = _iter_next(iter);
		if (!iter->filter || (d->flags & DEV_REGULAR) ||
		    iter->filter->passes_filter(iter->filter, d))
			return d;
	}

	return NULL;
}

int dev_fd(struct device *dev)
{
	return dev->fd;
}

const char *dev_name(const struct device *dev)
{
	return (dev) ? dm_list_item(dev->aliases.n, struct str_list)->str :
	    "unknown device";
}
