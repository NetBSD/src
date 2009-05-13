/*	$NetBSD: libdm-common.c,v 1.2.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include "libdm-targets.h"
#include "libdm-common.h"
#ifdef linux
#include "kdev_t.h"
#endif
#include "dm-ioctl.h"

#include <stdarg.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#ifdef linux
#  include <linux/fs.h>
#endif

#ifdef HAVE_SELINUX
#  include <selinux/selinux.h>
#endif

#ifdef __NetBSD__
#include <netbsd-dm.h>
#endif

#define DEV_DIR "/dev/"

static char _dm_dir[PATH_MAX] = DEV_DIR DM_DIR;

static int _verbose = 0;

/*
 * Library users can provide their own logging
 * function.
 */
static void _default_log(int level, const char *file __attribute((unused)),
			 int line __attribute((unused)), const char *f, ...)
{
	va_list ap;
	int use_stderr = level & _LOG_STDERR;

	level &= ~_LOG_STDERR;

	if (level > _LOG_WARN && !_verbose)
		return;

	va_start(ap, f);

	if (level < _LOG_WARN)
		vfprintf(stderr, f, ap);
	else
		vfprintf(use_stderr ? stderr : stdout, f, ap);

	va_end(ap);

	if (level < _LOG_WARN)
		fprintf(stderr, "\n");
	else
		fprintf(use_stderr ? stderr : stdout, "\n");
}

dm_log_fn dm_log = _default_log;

void dm_log_init(dm_log_fn fn)
{
	if (fn)
		dm_log = fn;
	else
		dm_log = _default_log;
}

void dm_log_init_verbose(int level)
{
	_verbose = level;
}

static void _build_dev_path(char *buffer, size_t len, const char *dev_name)
{
	/* If there's a /, assume caller knows what they're doing */
	if (strchr(dev_name, '/'))
		snprintf(buffer, len, "%s", dev_name);
	else
		snprintf(buffer, len, "%s/%s", _dm_dir, dev_name);
}

int dm_get_library_version(char *version, size_t size)
{
	strncpy(version, DM_LIB_VERSION, size);
	return 1;
}

struct dm_task *dm_task_create(int type)
{
	struct dm_task *dmt = dm_malloc(sizeof(*dmt));

	if (!dmt) {
		log_error("dm_task_create: malloc(%" PRIsize_t ") failed",
			  sizeof(*dmt));
		return NULL;
	}

	if (!dm_check_version()) {
		dm_free(dmt);
		return NULL;
	}

	memset(dmt, 0, sizeof(*dmt));

	dmt->type = type;
	dmt->minor = -1;
	dmt->major = -1;
	dmt->uid = DM_DEVICE_UID;
	dmt->gid = DM_DEVICE_GID;
	dmt->mode = DM_DEVICE_MODE;
	dmt->no_open_count = 0;
	dmt->read_ahead = DM_READ_AHEAD_AUTO;
	dmt->read_ahead_flags = 0;

	return dmt;
}

int dm_task_set_name(struct dm_task *dmt, const char *name)
{
	char *pos;
	char path[PATH_MAX];
	struct stat st1, st2;

	if (dmt->dev_name) {
		dm_free(dmt->dev_name);
		dmt->dev_name = NULL;
	}

	/* If path was supplied, remove it if it points to the same device
	 * as its last component.
	 */
	if ((pos = strrchr(name, '/'))) {
		if (dmt->type == DM_DEVICE_CREATE) {
			log_error("Name \"%s\" invalid. It contains \"/\".", name);
			return 0;
		}

		snprintf(path, sizeof(path), "%s/%s", _dm_dir, pos + 1);

		if (stat(name, &st1) || stat(path, &st2) ||
		    !(st1.st_dev == st2.st_dev)) {
			log_error("Device %s not found", name);
			return 0;
		}

		name = pos + 1;
	}

	if (strlen(name) >= DM_NAME_LEN) {
		log_error("Name \"%s\" too long", name);
		return 0;
	}

	if (!(dmt->dev_name = dm_strdup(name))) {
		log_error("dm_task_set_name: strdup(%s) failed", name);
		return 0;
	}

	return 1;
}

int dm_task_set_uuid(struct dm_task *dmt, const char *uuid)
{
	if (dmt->uuid) {
		dm_free(dmt->uuid);
		dmt->uuid = NULL;
	}

	if (!(dmt->uuid = dm_strdup(uuid))) {
		log_error("dm_task_set_uuid: strdup(%s) failed", uuid);
		return 0;
	}

	return 1;
}

int dm_task_set_major(struct dm_task *dmt, int major)
{
	dmt->major = major;

	return 1;
}

int dm_task_set_minor(struct dm_task *dmt, int minor)
{
	dmt->minor = minor;

	return 1;
}

int dm_task_set_uid(struct dm_task *dmt, uid_t uid)
{
	dmt->uid = uid;

	return 1;
}

int dm_task_set_gid(struct dm_task *dmt, gid_t gid)
{
	dmt->gid = gid;

	return 1;
}

int dm_task_set_mode(struct dm_task *dmt, mode_t mode)
{
	dmt->mode = mode;

	return 1;
}

int dm_task_add_target(struct dm_task *dmt, uint64_t start, uint64_t size,
		       const char *ttype, const char *params)
{
	struct target *t = create_target(start, size, ttype, params);

	if (!t)
		return 0;

	if (!dmt->head)
		dmt->head = dmt->tail = t;
	else {
		dmt->tail->next = t;
		dmt->tail = t;
	}

	return 1;
}

#ifdef HAVE_SELINUX
int dm_set_selinux_context(const char *path, mode_t mode)
{
	security_context_t scontext;

	if (is_selinux_enabled() <= 0)
		return 1;

	if (matchpathcon(path, mode, &scontext) < 0) {
		log_error("%s: matchpathcon %07o failed: %s", path, mode,
			  strerror(errno));
		return 0;
	}

	log_debug("Setting SELinux context for %s to %s.", path, scontext);

	if ((lsetfilecon(path, scontext) < 0) && (errno != ENOTSUP)) {
		log_sys_error("lsetfilecon", path);
		freecon(scontext);
		return 0;
	}

	freecon(scontext);
	return 1;
}
#endif

static int _add_dev_node(const char *dev_name, uint32_t major, uint32_t minor,
			 uid_t uid, gid_t gid, mode_t mode)
{
	char path[PATH_MAX];
	struct stat info;
	dev_t dev = MKDEV(major, minor);
	mode_t old_mask;

	#ifdef __NetBSD__
	char rpath[PATH_MAX];
	uint32_t raw_major;
	dev_t rdev;
	char raw_devname[DM_NAME_LEN+1]; /* r + other device name */

	nbsd_get_dm_major(&raw_major,DM_CHAR_MAJOR);
	rdev = MKDEV(raw_major,minor);

	snprintf(raw_devname,sizeof(raw_devname),"r%s",dev_name);

	_build_dev_path(rpath, sizeof(rpath), raw_devname);

	if (stat(rpath, &info) >= 0) {
		if (!S_ISCHR(info.st_mode)) {
			log_error("A non-raw device file at '%s' "
			    "is already present", rpath);
			return 0;
		}

		/* If right inode already exists we don't touch uid etc. */
		if (info.st_rdev == rdev)
			return 1;

		if (unlink(rpath) < 0) {
			log_error("Unable to unlink device node for '%s'",
			    raw_devname);
			return 0;
		}
	}

	old_mask = umask(0);

	if (mknod(rpath, S_IFCHR | mode, rdev) < 0) {
		log_error("Unable to make device node for '%s'", raw_devname);
		return 0;
	}
#endif
	
	_build_dev_path(path, sizeof(path), dev_name);

	if (stat(path, &info) >= 0) {
		if (!S_ISBLK(info.st_mode)) {
			log_error("A non-block device file at '%s' "
				  "is already present", path);
			return 0;
		}

		/* If right inode already exists we don't touch uid etc. */
		if (info.st_rdev == dev)
			return 1;

		if (unlink(path) < 0) {
			log_error("Unable to unlink device node for '%s'",
				  dev_name);
			return 0;
		}
	}

	old_mask = umask(0);
	if (mknod(path, S_IFBLK | mode, dev) < 0) {
		log_error("Unable to make device node for '%s'", dev_name);
		return 0;
	}
	umask(old_mask);

	if (chown(path, uid, gid) < 0) {
		log_sys_error("chown", path);
		return 0;
	}

	log_debug("Created %s", path);

#ifdef HAVE_SELINUX
	if (!dm_set_selinux_context(path, S_IFBLK))
		return 0;
#endif

	return 1;
}

static int _rename_dev_node(const char *old_name, const char *new_name)
{
	char oldpath[PATH_MAX];
	char newpath[PATH_MAX];
	struct stat info;

#ifdef __NetBSD__
	char rpath[PATH_MAX];
	char nrpath[PATH_MAX];
	char raw_devname[DM_NAME_LEN+1]; /* r + other device name */
	char nraw_devname[DM_NAME_LEN+1]; /* r + other device name */

	snprintf(nraw_devname,sizeof(raw_devname),"r%s",new_name);
	snprintf(raw_devname,sizeof(raw_devname),"r%s",old_name);

	_build_dev_path(nrpath, sizeof(nrpath), nraw_devname);
	_build_dev_path(rpath, sizeof(rpath), raw_devname);

	if (stat(nrpath, &info) == 0) {
		if (S_ISBLK(info.st_mode)) {
			log_error("A block device file at '%s' "
			    "is present where raw device should be.", newpath);
			return 0;
		}

		if (unlink(nrpath) < 0) {
			log_error("Unable to unlink device node for '%s'",
			    nraw_devname);
			return 0;
		}
	}

	if (rename(rpath, nrpath) < 0) {
		log_error("Unable to rename device node from '%s' to '%s'",
		    raw_devname, nraw_devname);
		return 0;
	}

	log_debug("Renamed %s to %s", rpath, nrpath);

#endif
	
	_build_dev_path(oldpath, sizeof(oldpath), old_name);
	_build_dev_path(newpath, sizeof(newpath), new_name);

	if (stat(newpath, &info) == 0) {
		if (!S_ISBLK(info.st_mode)) {
			log_error("A non-block device file at '%s' "
				  "is already present", newpath);
			return 0;
		}

		if (unlink(newpath) < 0) {
			if (errno == EPERM) {
				/* devfs, entry has already been renamed */
				return 1;
			}
			log_error("Unable to unlink device node for '%s'",
				  new_name);
			return 0;
		}
	}

	if (rename(oldpath, newpath) < 0) {
		log_error("Unable to rename device node from '%s' to '%s'",
			  old_name, new_name);
		return 0;
	}

	log_debug("Renamed %s to %s", oldpath, newpath);

	return 1;
}

static int _rm_dev_node(const char *dev_name)
{
	char path[PATH_MAX];
	struct stat info;

#ifdef __NetBSD__
	char rpath[PATH_MAX];
	char raw_devname[DM_NAME_LEN+1]; /* r + other device name */

	snprintf(raw_devname,sizeof(raw_devname),"r%s",dev_name);

	_build_dev_path(rpath, sizeof(rpath), raw_devname);

	if (stat(rpath, &info) < 0)
		return 1;

	if (unlink(rpath) < 0) {
		log_error("Unable to unlink device node for '%s'", raw_devname);
		return 0;
	}

	log_debug("Removed %s", rpath);
#endif
	
	_build_dev_path(path, sizeof(path), dev_name);

	if (stat(path, &info) < 0)
		return 1;

	if (unlink(path) < 0) {
		log_error("Unable to unlink device node for '%s'", dev_name);
		return 0;
	}

	log_debug("Removed %s", path);

	return 1;
}

#ifdef linux
static int _open_dev_node(const char *dev_name)
{
	int fd = -1;
	char path[PATH_MAX];

	_build_dev_path(path, sizeof(path), dev_name);

	if ((fd = open(path, O_RDONLY, 0)) < 0)
		log_sys_error("open", path);

	return fd;
}

int get_dev_node_read_ahead(const char *dev_name, uint32_t *read_ahead)
{
	int r = 1;
	int fd;
	long read_ahead_long;

	if (!*dev_name) {
		log_error("Empty device name passed to BLKRAGET");
		return 0;
	}

	if ((fd = _open_dev_node(dev_name)) < 0)
		return_0;

	if (ioctl(fd, BLKRAGET, &read_ahead_long)) {
		log_sys_error("BLKRAGET", dev_name);
		*read_ahead = 0;
		r = 0;
	}  else {
		*read_ahead = (uint32_t) read_ahead_long;
		log_debug("%s: read ahead is %" PRIu32, dev_name, *read_ahead);
	}

	if (close(fd))
		stack;

	return r;
}

static int _set_read_ahead(const char *dev_name, uint32_t read_ahead)
{
	int r = 1;
	int fd;
	long read_ahead_long = (long) read_ahead;

	if (!*dev_name) {
		log_error("Empty device name passed to BLKRAGET");
		return 0;
	}

	if ((fd = _open_dev_node(dev_name)) < 0)
		return_0;

	log_debug("%s: Setting read ahead to %" PRIu32, dev_name, read_ahead);

	if (ioctl(fd, BLKRASET, read_ahead_long)) {
		log_sys_error("BLKRASET", dev_name);
		r = 0;
	}

	if (close(fd))
		stack;

	return r;
}

static int _set_dev_node_read_ahead(const char *dev_name, uint32_t read_ahead,
				    uint32_t read_ahead_flags)
{
	uint32_t current_read_ahead;

	if (read_ahead == DM_READ_AHEAD_AUTO)
		return 1;

	if (read_ahead == DM_READ_AHEAD_NONE)
		read_ahead = 0;

	if (read_ahead_flags & DM_READ_AHEAD_MINIMUM_FLAG) {
		if (!get_dev_node_read_ahead(dev_name, &current_read_ahead))
			return_0;

		if (current_read_ahead > read_ahead) {
			log_debug("%s: retaining kernel read ahead of %" PRIu32
				  " (requested %" PRIu32 ")",           
				  dev_name, current_read_ahead, read_ahead);
			return 1;
		}
	}

	return _set_read_ahead(dev_name, read_ahead);
}

#else

int get_dev_node_read_ahead(const char *dev_name, uint32_t *read_ahead)
{
	*read_ahead = 0;

	return 1;
}

static int _set_dev_node_read_ahead(const char *dev_name, uint32_t read_ahead,
				    uint32_t read_ahead_flags)
{
	return 1;
}
#endif

typedef enum {
	NODE_ADD,
	NODE_DEL,
	NODE_RENAME,
	NODE_READ_AHEAD
} node_op_t;

static int _do_node_op(node_op_t type, const char *dev_name, uint32_t major,
		       uint32_t minor, uid_t uid, gid_t gid, mode_t mode,
		       const char *old_name, uint32_t read_ahead,
		       uint32_t read_ahead_flags)
{
	switch (type) {
	case NODE_ADD:
		return _add_dev_node(dev_name, major, minor, uid, gid, mode);
	case NODE_DEL:
		return _rm_dev_node(dev_name);
	case NODE_RENAME:
		return _rename_dev_node(old_name, dev_name);
	case NODE_READ_AHEAD:
		return _set_dev_node_read_ahead(dev_name, read_ahead,
						read_ahead_flags);
	}

	return 1;
}

static DM_LIST_INIT(_node_ops);

struct node_op_parms {
	struct dm_list list;
	node_op_t type;
	char *dev_name;
	uint32_t major;
	uint32_t minor;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	uint32_t read_ahead;
	uint32_t read_ahead_flags;
	char *old_name;
	char names[0];
};

static void _store_str(char **pos, char **ptr, const char *str)
{
	strcpy(*pos, str);
	*ptr = *pos;
	*pos += strlen(*ptr) + 1;
}

static int _stack_node_op(node_op_t type, const char *dev_name, uint32_t major,
			  uint32_t minor, uid_t uid, gid_t gid, mode_t mode,
			  const char *old_name, uint32_t read_ahead,
			  uint32_t read_ahead_flags)
{
	struct node_op_parms *nop;
	struct dm_list *noph, *nopht;
	size_t len = strlen(dev_name) + strlen(old_name) + 2;
	char *pos;

	/*
	 * Ignore any outstanding operations on the node if deleting it
	 */
	if (type == NODE_DEL) {
		dm_list_iterate_safe(noph, nopht, &_node_ops) {
			nop = dm_list_item(noph, struct node_op_parms);
			if (!strcmp(dev_name, nop->dev_name)) {
				dm_list_del(&nop->list);
				dm_free(nop);
			}
		}
	}

	if (!(nop = dm_malloc(sizeof(*nop) + len))) {
		log_error("Insufficient memory to stack mknod operation");
		return 0;
	}

	pos = nop->names;
	nop->type = type;
	nop->major = major;
	nop->minor = minor;
	nop->uid = uid;
	nop->gid = gid;
	nop->mode = mode;
	nop->read_ahead = read_ahead;
	nop->read_ahead_flags = read_ahead_flags;

	_store_str(&pos, &nop->dev_name, dev_name);
	_store_str(&pos, &nop->old_name, old_name);

	dm_list_add(&_node_ops, &nop->list);

	return 1;
}

static void _pop_node_ops(void)
{
	struct dm_list *noph, *nopht;
	struct node_op_parms *nop;

	dm_list_iterate_safe(noph, nopht, &_node_ops) {
		nop = dm_list_item(noph, struct node_op_parms);
		_do_node_op(nop->type, nop->dev_name, nop->major, nop->minor,
			    nop->uid, nop->gid, nop->mode, nop->old_name,
			    nop->read_ahead, nop->read_ahead_flags);
		dm_list_del(&nop->list);
		dm_free(nop);
	}
}

int add_dev_node(const char *dev_name, uint32_t major, uint32_t minor,
		 uid_t uid, gid_t gid, mode_t mode)
{
	log_debug("%s: Stacking NODE_ADD (%" PRIu32 ",%" PRIu32 ") %u:%u 0%o",
		  dev_name, major, minor, uid, gid, mode);

	return _stack_node_op(NODE_ADD, dev_name, major, minor, uid, gid, mode,
			      "", 0, 0);
}

int rename_dev_node(const char *old_name, const char *new_name)
{
	log_debug("%s: Stacking NODE_RENAME to %s", old_name, new_name);

	return _stack_node_op(NODE_RENAME, new_name, 0, 0, 0, 0, 0, old_name,
			      0, 0);
}

int rm_dev_node(const char *dev_name)
{
	log_debug("%s: Stacking NODE_DEL (replaces other stacked ops)", dev_name);

	return _stack_node_op(NODE_DEL, dev_name, 0, 0, 0, 0, 0, "", 0, 0);
}

int set_dev_node_read_ahead(const char *dev_name, uint32_t read_ahead,
			    uint32_t read_ahead_flags)
{
	if (read_ahead == DM_READ_AHEAD_AUTO)
		return 1;

	log_debug("%s: Stacking NODE_READ_AHEAD %" PRIu32 " (flags=%" PRIu32
		  ")", dev_name, read_ahead, read_ahead_flags);

	return _stack_node_op(NODE_READ_AHEAD, dev_name, 0, 0, 0, 0, 0, "",
			      read_ahead, read_ahead_flags);
}

void update_devs(void)
{
	_pop_node_ops();
}

int dm_set_dev_dir(const char *dev_dir)
{
	size_t len;
	const char *slash;
	if (*dev_dir != '/') {
		log_debug("Invalid dev_dir value, %s: "
			  "not an absolute name.", dev_dir);
		return 0;
	}

	len = strlen(dev_dir);
	slash = dev_dir[len-1] == '/' ? "" : "/";

	if (snprintf(_dm_dir, sizeof _dm_dir, "%s%s%s", dev_dir, slash, DM_DIR)
	    >= sizeof _dm_dir) {
		log_debug("Invalid dev_dir value, %s: name too long.", dev_dir);
		return 0;
	}

	return 1;
}

const char *dm_dir(void)
{
	return _dm_dir;
}

int dm_mknodes(const char *name)
{
	struct dm_task *dmt;
	int r = 0;

	if (!(dmt = dm_task_create(DM_DEVICE_MKNODES)))
		return 0;

	if (name && !dm_task_set_name(dmt, name))
		goto out;

	if (!dm_task_no_open_count(dmt))
		goto out;

	r = dm_task_run(dmt);

out:
	dm_task_destroy(dmt);
	return r;
}

int dm_driver_version(char *version, size_t size)
{
	struct dm_task *dmt;
	int r = 0;

	if (!(dmt = dm_task_create(DM_DEVICE_VERSION)))
		return 0;

	if (!dm_task_run(dmt))
		log_error("Failed to get driver version");

	if (!dm_task_get_driver_version(dmt, version, size))
		goto out;

	r = 1;

out:
	dm_task_destroy(dmt);
	return r;
}

