/*	$NetBSD: libdm-common.c,v 1.1.1.3 2009/12/02 00:26:04 haad Exp $	*/

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
#include "kdev_t.h"
#include "dm-ioctl.h"

#include <stdarg.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef UDEV_SYNC_SUPPORT
#  include <sys/types.h>
#  include <sys/ipc.h>
#  include <sys/sem.h>
#ifdef HAVE_UDEV_QUEUE_GET_UDEV_IS_ACTIVE
#  define LIBUDEV_I_KNOW_THE_API_IS_SUBJECT_TO_CHANGE
#  include <libudev.h>
#endif
#endif

#ifdef linux
#  include <linux/fs.h>
#endif

#ifdef HAVE_SELINUX
#  include <selinux/selinux.h>
#endif

#define DEV_DIR "/dev/"

static char _dm_dir[PATH_MAX] = DEV_DIR DM_DIR;

static int _verbose = 0;

#ifdef UDEV_SYNC_SUPPORT
static int _udev_running = -1;
static int _sync_with_udev = 1;
#endif

/*
 * Library users can provide their own logging
 * function.
 */

static void _default_log_line(int level,
	    const char *file __attribute((unused)),
	    int line __attribute((unused)), int dm_errno, 
	    const char *f, va_list ap)
{
	int use_stderr = level & _LOG_STDERR;

	level &= ~_LOG_STDERR;

	if (level > _LOG_WARN && !_verbose)
		return;

	if (level < _LOG_WARN)
		vfprintf(stderr, f, ap);
	else
		vfprintf(use_stderr ? stderr : stdout, f, ap);

	if (level < _LOG_WARN)
		fprintf(stderr, "\n");
	else
		fprintf(use_stderr ? stderr : stdout, "\n");
}

static void _default_log_with_errno(int level,
	    const char *file __attribute((unused)),
	    int line __attribute((unused)), int dm_errno, 
	    const char *f, ...)
{
	va_list ap;

	va_start(ap, f);
	_default_log_line(level, file, line, dm_errno, f, ap);
	va_end(ap);
}

static void _default_log(int level, const char *file,
			 int line, const char *f, ...)
{
	va_list ap;

	va_start(ap, f);
	_default_log_line(level, file, line, 0, f, ap);
	va_end(ap);
}

dm_log_fn dm_log = _default_log;
dm_log_with_errno_fn dm_log_with_errno = _default_log_with_errno;

void dm_log_init(dm_log_fn fn)
{
	if (fn)
		dm_log = fn;
	else
		dm_log = _default_log;

	dm_log_with_errno = _default_log_with_errno;
}

int dm_log_is_non_default(void)
{
	return (dm_log == _default_log) ? 0 : 1;
}

void dm_log_with_errno_init(dm_log_with_errno_fn fn)
{
	if (fn)
		dm_log_with_errno = fn;
	else
		dm_log_with_errno = _default_log_with_errno;

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
	dmt->allow_default_major_fallback = 1;
	dmt->uid = DM_DEVICE_UID;
	dmt->gid = DM_DEVICE_GID;
	dmt->mode = DM_DEVICE_MODE;
	dmt->no_open_count = 0;
	dmt->read_ahead = DM_READ_AHEAD_AUTO;
	dmt->read_ahead_flags = 0;
	dmt->event_nr = 0;
	dmt->cookie_set = 0;
	dmt->query_inactive_table = 0;

	return dmt;
}

/*
 * Find the name associated with a given device number by scanning _dm_dir.
 */
static char *_find_dm_name_of_device(dev_t st_rdev)
{
	const char *name;
	char path[PATH_MAX];
	struct dirent *dirent;
	DIR *d;
	struct stat buf;
	char *new_name = NULL;

	if (!(d = opendir(_dm_dir))) {
		log_sys_error("opendir", _dm_dir);
		return NULL;
	}

	while ((dirent = readdir(d))) {
		name = dirent->d_name;

		if (!strcmp(name, ".") || !strcmp(name, ".."))
			continue;

		if (dm_snprintf(path, sizeof(path), "%s/%s", _dm_dir,
				name) == -1) {
			log_error("Couldn't create path for %s", name);
			continue;
		}

		if (stat(path, &buf))
			continue;

		if (buf.st_rdev == st_rdev) {
			if (!(new_name = dm_strdup(name)))
				log_error("dm_task_set_name: strdup(%s) failed",
					  name);
			break;
		}
	}

	if (closedir(d))
		log_sys_error("closedir", _dm_dir);

	return new_name;
}

int dm_task_set_name(struct dm_task *dmt, const char *name)
{
	char *pos;
	char *new_name = NULL;
	char path[PATH_MAX];
	struct stat st1, st2;

	if (dmt->dev_name) {
		dm_free(dmt->dev_name);
		dmt->dev_name = NULL;
	}

	/*
	 * Path supplied for existing device?
	 */
	if ((pos = strrchr(name, '/'))) {
		if (dmt->type == DM_DEVICE_CREATE) {
			log_error("Name \"%s\" invalid. It contains \"/\".", name);
			return 0;
		}

		if (stat(name, &st1)) {
			log_error("Device %s not found", name);
			return 0;
		}

		/*
		 * If supplied path points to same device as last component
		 * under /dev/mapper, use that name directly.  Otherwise call
		 * _find_dm_name_of_device() to scan _dm_dir for a match.
		 */
		if (dm_snprintf(path, sizeof(path), "%s/%s", _dm_dir,
				pos + 1) == -1) {
			log_error("Couldn't create path for %s", pos + 1);
			return 0;
		}

		if (!stat(path, &st2) && (st1.st_rdev == st2.st_rdev))
			name = pos + 1;
		else if ((new_name = _find_dm_name_of_device(st1.st_rdev)))
			name = new_name;
		else {
			log_error("Device %s not found", name);
			return 0;
		}
	}

	if (strlen(name) >= DM_NAME_LEN) {
		log_error("Name \"%s\" too long", name);
		if (new_name)
			dm_free(new_name);
		return 0;
	}

	if (new_name)
		dmt->dev_name = new_name;
	else if (!(dmt->dev_name = dm_strdup(name))) {
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
	dmt->allow_default_major_fallback = 0;

	return 1;
}

int dm_task_set_minor(struct dm_task *dmt, int minor)
{
	dmt->minor = minor;

	return 1;
}

int dm_task_set_major_minor(struct dm_task *dmt, int major, int minor,
			    int allow_default_major_fallback)
{
	dmt->major = major;
	dmt->minor = minor;
	dmt->allow_default_major_fallback = allow_default_major_fallback;

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

int dm_set_selinux_context(const char *path, mode_t mode)
{
#ifdef HAVE_SELINUX
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
#endif
	return 1;
}

static int _add_dev_node(const char *dev_name, uint32_t major, uint32_t minor,
			 uid_t uid, gid_t gid, mode_t mode, int check_udev)
{
	char path[PATH_MAX];
	struct stat info;
	dev_t dev = MKDEV(major, minor);
	mode_t old_mask;

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
	} else if (dm_udev_get_sync_support() && check_udev)
		log_warn("%s not set up by udev: Falling back to direct "
			 "node creation.", path);

	old_mask = umask(0);
	if (mknod(path, S_IFBLK | mode, dev) < 0) {
		umask(old_mask);
		log_error("Unable to make device node for '%s'", dev_name);
		return 0;
	}
	umask(old_mask);

	if (chown(path, uid, gid) < 0) {
		log_sys_error("chown", path);
		return 0;
	}

	log_debug("Created %s", path);

	if (!dm_set_selinux_context(path, S_IFBLK))
		return 0;

	return 1;
}

static int _rm_dev_node(const char *dev_name, int check_udev)
{
	char path[PATH_MAX];
	struct stat info;

	_build_dev_path(path, sizeof(path), dev_name);

	if (stat(path, &info) < 0)
		return 1;
	else if (dm_udev_get_sync_support() && check_udev)
		log_warn("Node %s was not removed by udev. "
			 "Falling back to direct node removal.", path);

	if (unlink(path) < 0) {
		log_error("Unable to unlink device node for '%s'", dev_name);
		return 0;
	}

	log_debug("Removed %s", path);

	return 1;
}

static int _rename_dev_node(const char *old_name, const char *new_name,
			    int check_udev)
{
	char oldpath[PATH_MAX];
	char newpath[PATH_MAX];
	struct stat info;

	_build_dev_path(oldpath, sizeof(oldpath), old_name);
	_build_dev_path(newpath, sizeof(newpath), new_name);

	if (stat(newpath, &info) == 0) {
		if (!S_ISBLK(info.st_mode)) {
			log_error("A non-block device file at '%s' "
				  "is already present", newpath);
			return 0;
		}
		else if (dm_udev_get_sync_support() && check_udev) {
			if (stat(oldpath, &info) < 0 &&
				 errno == ENOENT)
				/* assume udev already deleted this */
				return 1;
			else {
				log_warn("The node %s should have been renamed to %s "
					 "by udev but old node is still present. "
					 "Falling back to direct old node removal.",
					 oldpath, newpath);
				return _rm_dev_node(old_name, 0);
			}
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
	else if (dm_udev_get_sync_support() && check_udev)
		log_warn("The node %s should have been renamed to %s "
			 "by udev but new node is not present. "
			 "Falling back to direct node rename.",
			 oldpath, newpath);

	if (rename(oldpath, newpath) < 0) {
		log_error("Unable to rename device node from '%s' to '%s'",
			  old_name, new_name);
		return 0;
	}

	log_debug("Renamed %s to %s", oldpath, newpath);

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
		       uint32_t read_ahead_flags, int check_udev)
{
	switch (type) {
	case NODE_ADD:
		return _add_dev_node(dev_name, major, minor, uid, gid,
				     mode, check_udev);
	case NODE_DEL:
		return _rm_dev_node(dev_name, check_udev);
	case NODE_RENAME:
		return _rename_dev_node(old_name, dev_name, check_udev);
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
	int check_udev;
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
			  uint32_t read_ahead_flags, int check_udev)
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
	nop->check_udev = check_udev;

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
			    nop->read_ahead, nop->read_ahead_flags,
			    nop->check_udev);
		dm_list_del(&nop->list);
		dm_free(nop);
	}
}

int add_dev_node(const char *dev_name, uint32_t major, uint32_t minor,
		 uid_t uid, gid_t gid, mode_t mode, int check_udev)
{
	log_debug("%s: Stacking NODE_ADD (%" PRIu32 ",%" PRIu32 ") %u:%u 0%o",
		  dev_name, major, minor, uid, gid, mode);

	return _stack_node_op(NODE_ADD, dev_name, major, minor, uid,
			      gid, mode, "", 0, 0, check_udev);
}

int rename_dev_node(const char *old_name, const char *new_name, int check_udev)
{
	log_debug("%s: Stacking NODE_RENAME to %s", old_name, new_name);

	return _stack_node_op(NODE_RENAME, new_name, 0, 0, 0,
			      0, 0, old_name, 0, 0, check_udev);
}

int rm_dev_node(const char *dev_name, int check_udev)
{
	log_debug("%s: Stacking NODE_DEL (replaces other stacked ops)", dev_name);

	return _stack_node_op(NODE_DEL, dev_name, 0, 0, 0,
			      0, 0, "", 0, 0, check_udev);
}

int set_dev_node_read_ahead(const char *dev_name, uint32_t read_ahead,
			    uint32_t read_ahead_flags)
{
	if (read_ahead == DM_READ_AHEAD_AUTO)
		return 1;

	log_debug("%s: Stacking NODE_READ_AHEAD %" PRIu32 " (flags=%" PRIu32
		  ")", dev_name, read_ahead, read_ahead_flags);

	return _stack_node_op(NODE_READ_AHEAD, dev_name, 0, 0, 0, 0,
                              0, "", read_ahead, read_ahead_flags, 0);
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

#ifndef UDEV_SYNC_SUPPORT
void dm_udev_set_sync_support(int sync_with_udev)
{
}

int dm_udev_get_sync_support(void)
{
	return 0;
}

int dm_task_set_cookie(struct dm_task *dmt, uint32_t *cookie, uint16_t flags)
{
	if (dm_cookie_supported())
		dmt->event_nr = flags << DM_UDEV_FLAGS_SHIFT;
	*cookie = 0;

	return 1;
}

int dm_udev_complete(uint32_t cookie)
{
	return 1;
}

int dm_udev_wait(uint32_t cookie)
{
	return 1;
}

#else		/* UDEV_SYNC_SUPPORT */


static int _check_udev_is_running(void)
{

#  ifndef HAVE_UDEV_QUEUE_GET_UDEV_IS_ACTIVE

	log_debug("Could not get udev state because libudev library "
		  "was not found and it was not compiled in. "
		  "Assuming udev is not running.");
	return 0;

#  else	/* HAVE_UDEV_QUEUE_GET_UDEV_IS_ACTIVE */

	struct udev *udev;
	struct udev_queue *udev_queue;
	int r;

	if (!(udev = udev_new()))
		goto_bad;

	if (!(udev_queue = udev_queue_new(udev))) {
		udev_unref(udev);
		goto_bad;
	}

	if (!(r = udev_queue_get_udev_is_active(udev_queue)))
		log_debug("Udev is not running. "
			  "Not using udev synchronisation code.");

	udev_queue_unref(udev_queue);
	udev_unref(udev);

	return r;

bad:
	log_error("Could not get udev state. Assuming udev is not running.");
	return 0;

#  endif	/* HAVE_UDEV_QUEUE_GET_UDEV_IS_ACTIVE */

}

void dm_udev_set_sync_support(int sync_with_udev)
{
	if (_udev_running < 0)
		_udev_running = _check_udev_is_running();

	_sync_with_udev = sync_with_udev;
}

int dm_udev_get_sync_support(void)
{
	if (_udev_running < 0)
		_udev_running = _check_udev_is_running();

	return dm_cookie_supported() && _udev_running && _sync_with_udev;
}

static int _get_cookie_sem(uint32_t cookie, int *semid)
{
	if (cookie >> 16 != DM_COOKIE_MAGIC) {
		log_error("Could not continue to access notification "
			  "semaphore identified by cookie value %"
			  PRIu32 " (0x%x). Incorrect cookie prefix.",
			  cookie, cookie);
		return 0;
	}

	if ((*semid = semget((key_t) cookie, 1, 0)) >= 0)
		return 1;

	switch (errno) {
		case ENOENT:
			log_error("Could not find notification "
				  "semaphore identified by cookie "
				  "value %" PRIu32 " (0x%x)",
				  cookie, cookie);
			break;
		case EACCES:
			log_error("No permission to access "
				  "notificaton semaphore identified "
				  "by cookie value %" PRIu32 " (0x%x)",
				  cookie, cookie);
			break;
		default:
			log_error("Failed to access notification "
				   "semaphore identified by cookie "
				   "value %" PRIu32 " (0x%x): %s",
				  cookie, cookie, strerror(errno));
			break;
	}

	return 0;
}

static int _udev_notify_sem_inc(uint32_t cookie, int semid)
{
	struct sembuf sb = {0, 1, 0};

	if (semop(semid, &sb, 1) < 0) {
		log_error("semid %d: semop failed for cookie 0x%" PRIx32 ": %s",
			  semid, cookie, strerror(errno));
		return 0;
	}

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d) incremented",
		  cookie, semid);

	return 1;
}

static int _udev_notify_sem_dec(uint32_t cookie, int semid)
{
	struct sembuf sb = {0, -1, IPC_NOWAIT};

	if (semop(semid, &sb, 1) < 0) {
		switch (errno) {
			case EAGAIN:
				log_error("semid %d: semop failed for cookie "
					  "0x%" PRIx32 ": "
					  "incorrect semaphore state",
					  semid, cookie);
				break;
			default:
				log_error("semid %d: semop failed for cookie "
					  "0x%" PRIx32 ": %s",
					  semid, cookie, strerror(errno));
				break;
		}
		return 0;
	}

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d) decremented",
		  cookie, semid);

	return 1;
}

static int _udev_notify_sem_destroy(uint32_t cookie, int semid)
{
	if (semctl(semid, 0, IPC_RMID, 0) < 0) {
		log_error("Could not cleanup notification semaphore "
			  "identified by cookie value %" PRIu32 " (0x%x): %s",
			  cookie, cookie, strerror(errno));
		return 0;
	}

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d) destroyed", cookie,
		  semid);

	return 1;
}

static int _udev_notify_sem_create(uint32_t *cookie, int *semid)
{
	int fd;
	int gen_semid;
	uint16_t base_cookie;
	uint32_t gen_cookie;

	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		log_error("Failed to open /dev/urandom "
			  "to create random cookie value");
		*cookie = 0;
		return 0;
	}

	/* Generate random cookie value. Be sure it is unique and non-zero. */
	do {
		/* FIXME Handle non-error returns from read(). Move _io() into libdm? */
		if (read(fd, &base_cookie, sizeof(base_cookie)) != sizeof(base_cookie)) {
			log_error("Failed to initialize notification cookie");
			goto bad;
		}

		gen_cookie = DM_COOKIE_MAGIC << 16 | base_cookie;

		if (base_cookie && (gen_semid = semget((key_t) gen_cookie,
				    1, 0600 | IPC_CREAT | IPC_EXCL)) < 0) {
			switch (errno) {
				case EEXIST:
					/* if the semaphore key exists, we
					 * simply generate another random one */
					base_cookie = 0;
					break;
				case ENOMEM:
					log_error("Not enough memory to create "
						  "notification semaphore");
					goto bad;
				case ENOSPC:
					log_error("Limit for the maximum number "
						  "of semaphores reached. You can "
						  "check and set the limits in "
						  "/proc/sys/kernel/sem.");
					goto bad;
				default:
					log_error("Failed to create notification "
						  "semaphore: %s", strerror(errno));
					goto bad;
			}
		}
	} while (!base_cookie);

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d) created",
		  gen_cookie, gen_semid);

	if (semctl(gen_semid, 0, SETVAL, 1) < 0) {
		log_error("semid %d: semctl failed: %s", gen_semid, strerror(errno));
		/* We have to destroy just created semaphore
		 * so it won't stay in the system. */
		(void) _udev_notify_sem_destroy(gen_cookie, gen_semid);
		goto bad;
	}

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d) incremented",
		  gen_cookie, gen_semid);

	if (close(fd))
		stack;

	*semid = gen_semid;
	*cookie = gen_cookie;

	return 1;

bad:
	if (close(fd))
		stack;

	*cookie = 0;

	return 0;
}

int dm_task_set_cookie(struct dm_task *dmt, uint32_t *cookie, uint16_t flags)
{
	int semid;

	if (dm_cookie_supported())
		dmt->event_nr = flags << DM_UDEV_FLAGS_SHIFT;

	if (!dm_udev_get_sync_support()) {
		*cookie = 0;
		return 1;
	}

	if (*cookie) {
		if (!_get_cookie_sem(*cookie, &semid))
			goto_bad;
	} else if (!_udev_notify_sem_create(cookie, &semid))
		goto_bad;

	if (!_udev_notify_sem_inc(*cookie, semid)) {
		log_error("Could not set notification semaphore "
			  "identified by cookie value %" PRIu32 " (0x%x)",
			  *cookie, *cookie);
		goto bad;
	}

	dmt->event_nr |= ~DM_UDEV_FLAGS_MASK & *cookie;
	dmt->cookie_set = 1;

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d) assigned to dm_task "
		  "with flags 0x%" PRIx16, *cookie, semid, flags);

	return 1;

bad:
	dmt->event_nr = 0;
	return 0;
}

int dm_udev_complete(uint32_t cookie)
{
	int semid;

	if (!cookie || !dm_udev_get_sync_support())
		return 1;

	if (!_get_cookie_sem(cookie, &semid))
		return_0;

	if (!_udev_notify_sem_dec(cookie, semid)) {
		log_error("Could not signal waiting process using notification "
			  "semaphore identified by cookie value %" PRIu32 " (0x%x)",
			  cookie, cookie);
		return 0;
	}

	return 1;
}

int dm_udev_wait(uint32_t cookie)
{
	int semid;
	struct sembuf sb = {0, 0, 0};

	if (!cookie || !dm_udev_get_sync_support())
		return 1;

	if (!_get_cookie_sem(cookie, &semid))
		return_0;

	if (!_udev_notify_sem_dec(cookie, semid)) {
		log_error("Failed to set a proper state for notification "
			  "semaphore identified by cookie value %" PRIu32 " (0x%x) "
			  "to initialize waiting for incoming notifications.",
			  cookie, cookie);
		(void) _udev_notify_sem_destroy(cookie, semid);
		return 0;
	}

	log_debug("Udev cookie 0x%" PRIx32 " (semid %d): Waiting for zero",
		  cookie, semid);

repeat_wait:
	if (semop(semid, &sb, 1) < 0) {
		if (errno == EINTR)
			goto repeat_wait;
		else if (errno == EIDRM)
			return 1;

		log_error("Could not set wait state for notification semaphore "
			  "identified by cookie value %" PRIu32 " (0x%x): %s",
			  cookie, cookie, strerror(errno));
		(void) _udev_notify_sem_destroy(cookie, semid);
		return 0;
	}

	return _udev_notify_sem_destroy(cookie, semid);
}

#endif		/* UDEV_SYNC_SUPPORT */
