/*	$NetBSD: libdm-iface.c,v 1.1.1.1.2.1 2009/05/13 18:52:44 jym Exp $	*/

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

#ifdef DM_COMPAT
#  include "libdm-compat.h"
#endif

#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <limits.h>

#ifdef linux
#  include "kdev_t.h"
#  include <linux/limits.h>
#else
#  define MAJOR(x) major((x))
#  define MINOR(x) minor((x))
#  define MKDEV(x,y) makedev((x),(y))
#endif

#include "dm-ioctl.h"

/*
 * Ensure build compatibility.  
 * The hard-coded versions here are the highest present 
 * in the _cmd_data arrays.
 */

#if !((DM_VERSION_MAJOR == 1 && DM_VERSION_MINOR >= 0) || \
      (DM_VERSION_MAJOR == 4 && DM_VERSION_MINOR >= 0))
#error The version of dm-ioctl.h included is incompatible.
#endif

/* FIXME This should be exported in device-mapper.h */
#define DM_NAME "device-mapper"

#define PROC_MISC "/proc/misc"
#define PROC_DEVICES "/proc/devices"
#define MISC_NAME "misc"

#define NUMBER_OF_MAJORS 4096

/* dm major version no for running kernel */
static unsigned _dm_version = DM_VERSION_MAJOR;
static unsigned _dm_version_minor = 0;
static unsigned _dm_version_patchlevel = 0;
static int _log_suppress = 0;

static dm_bitset_t _dm_bitset = NULL;
static int _control_fd = -1;
static int _version_checked = 0;
static int _version_ok = 1;
static unsigned _ioctl_buffer_double_factor = 0;

/*
 * Support both old and new major numbers to ease the transition.
 * Clumsy, but only temporary.
 */
#if DM_VERSION_MAJOR == 4 && defined(DM_COMPAT)
const int _dm_compat = 1;
#else
const int _dm_compat = 0;
#endif


/* *INDENT-OFF* */
static struct cmd_data _cmd_data_v4[] = {
	{"create",	DM_DEV_CREATE,		{4, 0, 0}},
	{"reload",	DM_TABLE_LOAD,		{4, 0, 0}},
	{"remove",	DM_DEV_REMOVE,		{4, 0, 0}},
	{"remove_all",	DM_REMOVE_ALL,		{4, 0, 0}},
	{"suspend",	DM_DEV_SUSPEND,		{4, 0, 0}},
	{"resume",	DM_DEV_SUSPEND,		{4, 0, 0}},
	{"info",	DM_DEV_STATUS,		{4, 0, 0}},
	{"deps",	DM_TABLE_DEPS,		{4, 0, 0}},
	{"rename",	DM_DEV_RENAME,		{4, 0, 0}},
	{"version",	DM_VERSION,		{4, 0, 0}},
	{"status",	DM_TABLE_STATUS,	{4, 0, 0}},
	{"table",	DM_TABLE_STATUS,	{4, 0, 0}},
	{"waitevent",	DM_DEV_WAIT,		{4, 0, 0}},
	{"names",	DM_LIST_DEVICES,	{4, 0, 0}},
	{"clear",	DM_TABLE_CLEAR,		{4, 0, 0}},
	{"mknodes",	DM_DEV_STATUS,		{4, 0, 0}},
#ifdef DM_LIST_VERSIONS
	{"versions",	DM_LIST_VERSIONS,	{4, 1, 0}},
#endif
#ifdef DM_TARGET_MSG
	{"message",	DM_TARGET_MSG,		{4, 2, 0}},
#endif
#ifdef DM_DEV_SET_GEOMETRY
	{"setgeometry",	DM_DEV_SET_GEOMETRY,	{4, 6, 0}},
#endif
};
/* *INDENT-ON* */

#define ALIGNMENT_V1 sizeof(int)
#define ALIGNMENT 8

/* FIXME Rejig library to record & use errno instead */
#ifndef DM_EXISTS_FLAG
#  define DM_EXISTS_FLAG 0x00000004
#endif

static void *_align(void *ptr, unsigned int a)
{
	register unsigned long agn = --a;

	return (void *) (((unsigned long) ptr + agn) & ~agn);
}

#ifdef DM_IOCTLS
/*
 * Set number to NULL to populate _dm_bitset - otherwise first
 * match is returned.
 */
static int _get_proc_number(const char *file, const char *name,
			    uint32_t *number)
{
	FILE *fl;
	char nm[256];
	int c;
	uint32_t num;

	if (!(fl = fopen(file, "r"))) {
		log_sys_error("fopen", file);
		return 0;
	}

	while (!feof(fl)) {
		if (fscanf(fl, "%d %255s\n", &num, &nm[0]) == 2) {
			if (!strcmp(name, nm)) {
				if (number) {
					*number = num;
					if (fclose(fl))
						log_sys_error("fclose", file);
					return 1;
				}
				dm_bit_set(_dm_bitset, num);
			}
		} else do {
			c = fgetc(fl);
		} while (c != EOF && c != '\n');
	}
	if (fclose(fl))
		log_sys_error("fclose", file);

	if (number) {
		log_error("%s: No entry for %s found", file, name);
		return 0;
	}

	return 1;
}

static int _control_device_number(uint32_t *major, uint32_t *minor)
{
	if (!_get_proc_number(PROC_DEVICES, MISC_NAME, major) ||
	    !_get_proc_number(PROC_MISC, DM_NAME, minor)) {
		*major = 0;
		return 0;
	}

	return 1;
}

/*
 * Returns 1 if exists; 0 if it doesn't; -1 if it's wrong
 */
static int _control_exists(const char *control, uint32_t major, uint32_t minor)
{
	struct stat buf;

	if (stat(control, &buf) < 0) {
		if (errno != ENOENT)
			log_sys_error("stat", control);
		return 0;
	}

	if (!S_ISCHR(buf.st_mode)) {
		log_verbose("%s: Wrong inode type", control);
		if (!unlink(control))
			return 0;
		log_sys_error("unlink", control);
		return -1;
	}

	if (major && buf.st_rdev != MKDEV(major, minor)) {
		log_verbose("%s: Wrong device number: (%u, %u) instead of "
			    "(%u, %u)", control,
			    MAJOR(buf.st_mode), MINOR(buf.st_mode),
			    major, minor);
		if (!unlink(control))
			return 0;
		log_sys_error("unlink", control);
		return -1;
	}

	return 1;
}

static int _create_control(const char *control, uint32_t major, uint32_t minor)
{
	int ret;
	mode_t old_umask;

	if (!major)
		return 0;

	old_umask = umask(0022);
	ret = dm_create_dir(dm_dir());
	umask(old_umask);

	if (!ret)
		return 0;

	log_verbose("Creating device %s (%u, %u)", control, major, minor);

	if (mknod(control, S_IFCHR | S_IRUSR | S_IWUSR,
		  MKDEV(major, minor)) < 0)  {
		log_sys_error("mknod", control);
		return 0;
	}

#ifdef HAVE_SELINUX
	if (!dm_set_selinux_context(control, S_IFCHR)) {
		stack;
		return 0;
	}
#endif

	return 1;
}
#endif

static int _create_dm_bitset(void)
{
#ifdef DM_IOCTLS
	if (_dm_bitset)
		return 1;

	if (!(_dm_bitset = dm_bitset_create(NULL, NUMBER_OF_MAJORS)))
		return 0;

	if (!_get_proc_number(PROC_DEVICES, DM_NAME, NULL)) {
		dm_bitset_destroy(_dm_bitset);
		_dm_bitset = NULL;
		return 0;
	}

	return 1;
#else
	return 0;
#endif
}

int dm_is_dm_major(uint32_t major)
{
	if (!_create_dm_bitset())
		return 0;

	return dm_bit(_dm_bitset, major) ? 1 : 0;
}

static int _open_control(void)
{
#ifdef DM_IOCTLS
	char control[PATH_MAX];
	uint32_t major = 0, minor;

	if (_control_fd != -1)
		return 1;

	snprintf(control, sizeof(control), "%s/control", dm_dir());

	if (!_control_device_number(&major, &minor))
		log_error("Is device-mapper driver missing from kernel?");

	if (!_control_exists(control, major, minor) &&
	    !_create_control(control, major, minor))
		goto error;

	if ((_control_fd = open(control, O_RDWR)) < 0) {
		log_sys_error("open", control);
		goto error;
	}

	if (!_create_dm_bitset()) {
		log_error("Failed to set up list of device-mapper major numbers");
		return 0;
	}

	return 1;

error:
	log_error("Failure to communicate with kernel device-mapper driver.");
	return 0;
#else
	return 1;
#endif
}

void dm_task_destroy(struct dm_task *dmt)
{
	struct target *t, *n;

	for (t = dmt->head; t; t = n) {
		n = t->next;
		dm_free(t->params);
		dm_free(t->type);
		dm_free(t);
	}

	if (dmt->dev_name)
		dm_free(dmt->dev_name);

	if (dmt->newname)
		dm_free(dmt->newname);

	if (dmt->message)
		dm_free(dmt->message);

	if (dmt->dmi.v4)
		dm_free(dmt->dmi.v4);

	if (dmt->uuid)
		dm_free(dmt->uuid);

	dm_free(dmt);
}

/*
 * Protocol Version 1 compatibility functions.
 */

#ifdef DM_COMPAT

static int _dm_task_get_driver_version_v1(struct dm_task *dmt, char *version,
					  size_t size)
{
	unsigned int *v;

	if (!dmt->dmi.v1) {
		version[0] = '\0';
		return 0;
	}

	v = dmt->dmi.v1->version;
	snprintf(version, size, "%u.%u.%u", v[0], v[1], v[2]);
	return 1;
}

/* Unmarshall the target info returned from a status call */
static int _unmarshal_status_v1(struct dm_task *dmt, struct dm_ioctl_v1 *dmi)
{
	char *outbuf = (char *) dmi + dmi->data_start;
	char *outptr = outbuf;
	int32_t i;
	struct dm_target_spec_v1 *spec;

	for (i = 0; i < dmi->target_count; i++) {
		spec = (struct dm_target_spec_v1 *) outptr;

		if (!dm_task_add_target(dmt, spec->sector_start,
					(uint64_t) spec->length,
					spec->target_type,
					outptr + sizeof(*spec))) {
			return 0;
		}

		outptr = outbuf + spec->next;
	}

	return 1;
}

static int _dm_format_dev_v1(char *buf, int bufsize, uint32_t dev_major,
			     uint32_t dev_minor)
{
	int r;

	if (bufsize < 8)
		return 0;

	r = snprintf(buf, bufsize, "%03x:%03x", dev_major, dev_minor);
	if (r < 0 || r > bufsize - 1)
		return 0;

	return 1;
}

static int _dm_task_get_info_v1(struct dm_task *dmt, struct dm_info *info)
{
	if (!dmt->dmi.v1)
		return 0;

	memset(info, 0, sizeof(*info));

	info->exists = dmt->dmi.v1->flags & DM_EXISTS_FLAG ? 1 : 0;
	if (!info->exists)
		return 1;

	info->suspended = dmt->dmi.v1->flags & DM_SUSPEND_FLAG ? 1 : 0;
	info->read_only = dmt->dmi.v1->flags & DM_READONLY_FLAG ? 1 : 0;
	info->target_count = dmt->dmi.v1->target_count;
	info->open_count = dmt->dmi.v1->open_count;
	info->event_nr = 0;
	info->major = MAJOR(dmt->dmi.v1->dev);
	info->minor = MINOR(dmt->dmi.v1->dev);
	info->live_table = 1;
	info->inactive_table = 0;

	return 1;
}

static const char *_dm_task_get_name_v1(const struct dm_task *dmt)
{
	return (dmt->dmi.v1->name);
}

static const char *_dm_task_get_uuid_v1(const struct dm_task *dmt)
{
	return (dmt->dmi.v1->uuid);
}

static struct dm_deps *_dm_task_get_deps_v1(struct dm_task *dmt)
{
	log_error("deps version 1 no longer supported by libdevmapper");
	return NULL;
}

static struct dm_names *_dm_task_get_names_v1(struct dm_task *dmt)
{
	return (struct dm_names *) (((void *) dmt->dmi.v1) +
				    dmt->dmi.v1->data_start);
}

static void *_add_target_v1(struct target *t, void *out, void *end)
{
	void *out_sp = out;
	struct dm_target_spec_v1 sp;
	size_t sp_size = sizeof(struct dm_target_spec_v1);
	int len;
	const char no_space[] = "Ran out of memory building ioctl parameter";

	out += sp_size;
	if (out >= end) {
		log_error(no_space);
		return NULL;
	}

	sp.status = 0;
	sp.sector_start = t->start;
	sp.length = t->length;
	strncpy(sp.target_type, t->type, sizeof(sp.target_type));

	len = strlen(t->params);

	if ((out + len + 1) >= end) {
		log_error(no_space);

		log_error("t->params= '%s'", t->params);
		return NULL;
	}
	strcpy((char *) out, t->params);
	out += len + 1;

	/* align next block */
	out = _align(out, ALIGNMENT_V1);

	sp.next = out - out_sp;

	memcpy(out_sp, &sp, sp_size);

	return out;
}

static struct dm_ioctl_v1 *_flatten_v1(struct dm_task *dmt)
{
	const size_t min_size = 16 * 1024;
	const int (*version)[3];

	struct dm_ioctl_v1 *dmi;
	struct target *t;
	size_t len = sizeof(struct dm_ioctl_v1);
	void *b, *e;
	int count = 0;

	for (t = dmt->head; t; t = t->next) {
		len += sizeof(struct dm_target_spec_v1);
		len += strlen(t->params) + 1 + ALIGNMENT_V1;
		count++;
	}

	if (count && dmt->newname) {
		log_error("targets and newname are incompatible");
		return NULL;
	}

	if (dmt->newname)
		len += strlen(dmt->newname) + 1;

	/*
	 * Give len a minimum size so that we have space to store
	 * dependencies or status information.
	 */
	if (len < min_size)
		len = min_size;

	if (!(dmi = dm_malloc(len)))
		return NULL;

	memset(dmi, 0, len);

	version = &_cmd_data_v1[dmt->type].version;

	dmi->version[0] = (*version)[0];
	dmi->version[1] = (*version)[1];
	dmi->version[2] = (*version)[2];

	dmi->data_size = len;
	dmi->data_start = sizeof(struct dm_ioctl_v1);

	if (dmt->dev_name)
		strncpy(dmi->name, dmt->dev_name, sizeof(dmi->name));

	if (dmt->type == DM_DEVICE_SUSPEND)
		dmi->flags |= DM_SUSPEND_FLAG;
	if (dmt->read_only)
		dmi->flags |= DM_READONLY_FLAG;

	if (dmt->minor >= 0) {
		if (dmt->major <= 0) {
			log_error("Missing major number for persistent device");
			return NULL;
		}
		dmi->flags |= DM_PERSISTENT_DEV_FLAG;
		dmi->dev = MKDEV(dmt->major, dmt->minor);
	}

	if (dmt->uuid)
		strncpy(dmi->uuid, dmt->uuid, sizeof(dmi->uuid));

	dmi->target_count = count;

	b = (void *) (dmi + 1);
	e = (void *) ((char *) dmi + len);

	for (t = dmt->head; t; t = t->next)
		if (!(b = _add_target_v1(t, b, e)))
			goto bad;

	if (dmt->newname)
		strcpy(b, dmt->newname);

	return dmi;

      bad:
	dm_free(dmi);
	return NULL;
}

static int _dm_names_v1(struct dm_ioctl_v1 *dmi)
{
	const char *dev_dir = dm_dir();
	int r = 1, len;
	const char *name;
	struct dirent *dirent;
	DIR *d;
	struct dm_names *names, *old_names = NULL;
	void *end = (void *) dmi + dmi->data_size;
	struct stat buf;
	char path[PATH_MAX];

	log_warn("WARNING: Device list may be incomplete with interface "
		  "version 1.");
	log_warn("Please upgrade your kernel device-mapper driver.");

	if (!(d = opendir(dev_dir))) {
		log_sys_error("opendir", dev_dir);
		return 0;
	}

	names = (struct dm_names *) ((void *) dmi + dmi->data_start);

	names->dev = 0;		/* Flags no data */

	while ((dirent = readdir(d))) {
		name = dirent->d_name;

		if (name[0] == '.' || !strcmp(name, "control"))
			continue;

		if (old_names)
			old_names->next = (uint32_t) ((void *) names -
						      (void *) old_names);
		snprintf(path, sizeof(path), "%s/%s", dev_dir, name);
		if (stat(path, &buf)) {
			log_sys_error("stat", path);
			continue;
		}
		if (!S_ISBLK(buf.st_mode))
			continue;
		names->dev = (uint64_t) buf.st_rdev;
		names->next = 0;
		len = strlen(name);
		if (((void *) (names + 1) + len + 1) >= end) {
			log_error("Insufficient buffer space for device list");
			r = 0;
			break;
		}

		strcpy(names->name, name);

		old_names = names;
		names = _align((void *) ++names + len + 1, ALIGNMENT);
	}

	if (closedir(d))
		log_sys_error("closedir", dev_dir);

	return r;
}

static int _dm_task_run_v1(struct dm_task *dmt)
{
	struct dm_ioctl_v1 *dmi;
	unsigned int command;

	dmi = _flatten_v1(dmt);
	if (!dmi) {
		log_error("Couldn't create ioctl argument.");
		return 0;
	}

	if (!_open_control())
		return 0;

	if ((unsigned) dmt->type >=
	    (sizeof(_cmd_data_v1) / sizeof(*_cmd_data_v1))) {
		log_error("Internal error: unknown device-mapper task %d",
			  dmt->type);
		goto bad;
	}

	command = _cmd_data_v1[dmt->type].cmd;

	if (dmt->type == DM_DEVICE_TABLE)
		dmi->flags |= DM_STATUS_TABLE_FLAG;

	log_debug("dm %s %s %s%s%s [%u]", _cmd_data_v1[dmt->type].name,
		  dmi->name, dmi->uuid, dmt->newname ? " " : "",
		  dmt->newname ? dmt->newname : "",
		  dmi->data_size);
	if (dmt->type == DM_DEVICE_LIST) {
		if (!_dm_names_v1(dmi))
			goto bad;
	} 
#ifdef DM_IOCTLS
	else if (ioctl(_control_fd, command, dmi) < 0) {
		if (_log_suppress)
			log_verbose("device-mapper: %s ioctl failed: %s", 
				    _cmd_data_v1[dmt->type].name,
				    strerror(errno));
		else
			log_error("device-mapper: %s ioctl failed: %s",
				  _cmd_data_v1[dmt->type].name,
				  strerror(errno));
		goto bad;
	}
#else /* Userspace alternative for testing */
#endif

	if (dmi->flags & DM_BUFFER_FULL_FLAG)
		/* FIXME Increase buffer size and retry operation (if query) */
		log_error("WARNING: libdevmapper buffer too small for data");

	switch (dmt->type) {
	case DM_DEVICE_CREATE:
		add_dev_node(dmt->dev_name, MAJOR(dmi->dev), MINOR(dmi->dev),
			     dmt->uid, dmt->gid, dmt->mode);
		break;

	case DM_DEVICE_REMOVE:
		rm_dev_node(dmt->dev_name);
		break;

	case DM_DEVICE_RENAME:
		rename_dev_node(dmt->dev_name, dmt->newname);
		break;

	case DM_DEVICE_MKNODES:
		if (dmi->flags & DM_EXISTS_FLAG)
			add_dev_node(dmt->dev_name, MAJOR(dmi->dev),
				     MINOR(dmi->dev),
				     dmt->uid, dmt->gid, dmt->mode);
		else
			rm_dev_node(dmt->dev_name);
		break;

	case DM_DEVICE_STATUS:
	case DM_DEVICE_TABLE:
		if (!_unmarshal_status_v1(dmt, dmi))
			goto bad;
		break;

	case DM_DEVICE_SUSPEND:
	case DM_DEVICE_RESUME:
		dmt->type = DM_DEVICE_INFO;
		if (!dm_task_run(dmt))
			goto bad;
		dm_free(dmi);	/* We'll use what info returned */
		return 1;
	}

	dmt->dmi.v1 = dmi;
	return 1;

      bad:
	dm_free(dmi);
	return 0;
}

#endif

/*
 * Protocol Version 4 functions.
 */

int dm_task_get_driver_version(struct dm_task *dmt, char *version, size_t size)
{
	unsigned *v;

#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_get_driver_version_v1(dmt, version, size);
#endif

	if (!dmt->dmi.v4) {
		version[0] = '\0';
		return 0;
	}

	v = dmt->dmi.v4->version;
	snprintf(version, size, "%u.%u.%u", v[0], v[1], v[2]);
	_dm_version_minor = v[1];
	_dm_version_patchlevel = v[2];

	return 1;
}

static int _check_version(char *version, size_t size, int log_suppress)
{
	struct dm_task *task;
	int r;

	if (!(task = dm_task_create(DM_DEVICE_VERSION))) {
		log_error("Failed to get device-mapper version");
		version[0] = '\0';
		return 0;
	}

	if (log_suppress)
		_log_suppress = 1;

	r = dm_task_run(task);
	dm_task_get_driver_version(task, version, size);
	dm_task_destroy(task);
	_log_suppress = 0;

	return r;
}

/*
 * Find out device-mapper's major version number the first time 
 * this is called and whether or not we support it.
 */
int dm_check_version(void)
{
	char libversion[64], dmversion[64];
	const char *compat = "";

	if (_version_checked)
		return _version_ok;

	_version_checked = 1;

	if (_check_version(dmversion, sizeof(dmversion), _dm_compat))
		return 1;

	if (!_dm_compat)
		goto bad;

	log_verbose("device-mapper ioctl protocol version %u failed. "
		    "Trying protocol version 1.", _dm_version);
	_dm_version = 1;
	if (_check_version(dmversion, sizeof(dmversion), 0)) {
		log_verbose("Using device-mapper ioctl protocol version 1");
		return 1;
	}

	compat = "(compat)";

	dm_get_library_version(libversion, sizeof(libversion));

	log_error("Incompatible libdevmapper %s%s and kernel driver %s",
		  libversion, compat, dmversion);

      bad:
	_version_ok = 0;
	return 0;
}

void *dm_get_next_target(struct dm_task *dmt, void *next,
			 uint64_t *start, uint64_t *length,
			 char **target_type, char **params)
{
	struct target *t = (struct target *) next;

	if (!t)
		t = dmt->head;

	if (!t)
		return NULL;

	*start = t->start;
	*length = t->length;
	*target_type = t->type;
	*params = t->params;

	return t->next;
}

/* Unmarshall the target info returned from a status call */
static int _unmarshal_status(struct dm_task *dmt, struct dm_ioctl *dmi)
{
	char *outbuf = (char *) dmi + dmi->data_start;
	char *outptr = outbuf;
	uint32_t i;
	struct dm_target_spec *spec;

	for (i = 0; i < dmi->target_count; i++) {
		spec = (struct dm_target_spec *) outptr;
		if (!dm_task_add_target(dmt, spec->sector_start,
					spec->length,
					spec->target_type,
					outptr + sizeof(*spec))) {
			return 0;
		}

		outptr = outbuf + spec->next;
	}

	return 1;
}

int dm_format_dev(char *buf, int bufsize, uint32_t dev_major,
		  uint32_t dev_minor)
{
	int r;

#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_format_dev_v1(buf, bufsize, dev_major, dev_minor);
#endif

	if (bufsize < 8)
		return 0;

	r = snprintf(buf, (size_t) bufsize, "%u:%u", dev_major, dev_minor);
	if (r < 0 || r > bufsize - 1)
		return 0;

	return 1;
}

int dm_task_get_info(struct dm_task *dmt, struct dm_info *info)
{
#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_get_info_v1(dmt, info);
#endif

	if (!dmt->dmi.v4)
		return 0;

	memset(info, 0, sizeof(*info));

	info->exists = dmt->dmi.v4->flags & DM_EXISTS_FLAG ? 1 : 0;
	if (!info->exists)
		return 1;

	info->suspended = dmt->dmi.v4->flags & DM_SUSPEND_FLAG ? 1 : 0;
	info->read_only = dmt->dmi.v4->flags & DM_READONLY_FLAG ? 1 : 0;
	info->live_table = dmt->dmi.v4->flags & DM_ACTIVE_PRESENT_FLAG ? 1 : 0;
	info->inactive_table = dmt->dmi.v4->flags & DM_INACTIVE_PRESENT_FLAG ?
	    1 : 0;
	info->target_count = dmt->dmi.v4->target_count;
	info->open_count = dmt->dmi.v4->open_count;
	info->event_nr = dmt->dmi.v4->event_nr;
	info->major = MAJOR(dmt->dmi.v4->dev);
	info->minor = MINOR(dmt->dmi.v4->dev);

	return 1;
}

uint32_t dm_task_get_read_ahead(const struct dm_task *dmt, uint32_t *read_ahead)
{              
	const char *dev_name;

	*read_ahead = 0;

#ifdef DM_COMPAT
	/* Not supporting this */
        if (_dm_version == 1)
                return 1;
#endif  

        if (!dmt->dmi.v4 || !(dmt->dmi.v4->flags & DM_EXISTS_FLAG))
		return 0;

	if (*dmt->dmi.v4->name)
		dev_name = dmt->dmi.v4->name;
	else if (dmt->dev_name)
		dev_name = dmt->dev_name;
	else {
		log_error("Get read ahead request failed: device name unrecorded.");
		return 0;
	}

	return get_dev_node_read_ahead(dev_name, read_ahead);
}

const char *dm_task_get_name(const struct dm_task *dmt)
{
#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_get_name_v1(dmt);
#endif

	return (dmt->dmi.v4->name);
}

const char *dm_task_get_uuid(const struct dm_task *dmt)
{
#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_get_uuid_v1(dmt);
#endif

	return (dmt->dmi.v4->uuid);
}

struct dm_deps *dm_task_get_deps(struct dm_task *dmt)
{
#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_get_deps_v1(dmt);
#endif

	return (struct dm_deps *) (((void *) dmt->dmi.v4) +
				   dmt->dmi.v4->data_start);
}

struct dm_names *dm_task_get_names(struct dm_task *dmt)
{
#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_get_names_v1(dmt);
#endif

	return (struct dm_names *) (((void *) dmt->dmi.v4) +
				    dmt->dmi.v4->data_start);
}

struct dm_versions *dm_task_get_versions(struct dm_task *dmt)
{
	return (struct dm_versions *) (((void *) dmt->dmi.v4) +
				       dmt->dmi.v4->data_start);
}

int dm_task_set_ro(struct dm_task *dmt)
{
	dmt->read_only = 1;
	return 1;
}

int dm_task_set_read_ahead(struct dm_task *dmt, uint32_t read_ahead,
			   uint32_t read_ahead_flags)
{
	dmt->read_ahead = read_ahead;
	dmt->read_ahead_flags = read_ahead_flags;

	return 1;
}

int dm_task_suppress_identical_reload(struct dm_task *dmt)
{
	dmt->suppress_identical_reload = 1;
	return 1;
}

int dm_task_set_newname(struct dm_task *dmt, const char *newname)
{
	if (strchr(newname, '/')) {
		log_error("Name \"%s\" invalid. It contains \"/\".", newname);
		return 0;
	}

	if (strlen(newname) >= DM_NAME_LEN) {
		log_error("Name \"%s\" too long", newname);
		return 0;
	}

	if (!(dmt->newname = dm_strdup(newname))) {
		log_error("dm_task_set_newname: strdup(%s) failed", newname);
		return 0;
	}

	return 1;
}

int dm_task_set_message(struct dm_task *dmt, const char *message)
{
	if (!(dmt->message = dm_strdup(message))) {
		log_error("dm_task_set_message: strdup(%s) failed", message);
		return 0;
	}

	return 1;
}

int dm_task_set_sector(struct dm_task *dmt, uint64_t sector)
{
	dmt->sector = sector;

	return 1;
}

int dm_task_set_geometry(struct dm_task *dmt, const char *cylinders, const char *heads, const char *sectors, const char *start)
{
	size_t len = strlen(cylinders) + 1 + strlen(heads) + 1 + strlen(sectors) + 1 + strlen(start) + 1;

	if (!(dmt->geometry = dm_malloc(len))) {
		log_error("dm_task_set_geometry: dm_malloc failed");
		return 0;
	}

	if (sprintf(dmt->geometry, "%s %s %s %s", cylinders, heads, sectors, start) < 0) {
		log_error("dm_task_set_geometry: sprintf failed");
		return 0;
	}

	return 1;
}

int dm_task_no_flush(struct dm_task *dmt)
{
	dmt->no_flush = 1;

	return 1;
}

int dm_task_no_open_count(struct dm_task *dmt)
{
	dmt->no_open_count = 1;

	return 1;
}

int dm_task_skip_lockfs(struct dm_task *dmt)
{
	dmt->skip_lockfs = 1;

	return 1;
}

int dm_task_set_event_nr(struct dm_task *dmt, uint32_t event_nr)
{
	dmt->event_nr = event_nr;

	return 1;
}

struct target *create_target(uint64_t start, uint64_t len, const char *type,
			     const char *params)
{
	struct target *t = dm_malloc(sizeof(*t));

	if (!t) {
		log_error("create_target: malloc(%" PRIsize_t ") failed",
			  sizeof(*t));
		return NULL;
	}

	memset(t, 0, sizeof(*t));

	if (!(t->params = dm_strdup(params))) {
		log_error("create_target: strdup(params) failed");
		goto bad;
	}

	if (!(t->type = dm_strdup(type))) {
		log_error("create_target: strdup(type) failed");
		goto bad;
	}

	t->start = start;
	t->length = len;
	return t;

      bad:
	dm_free(t->params);
	dm_free(t->type);
	dm_free(t);
	return NULL;
}

static void *_add_target(struct target *t, void *out, void *end)
{
	void *out_sp = out;
	struct dm_target_spec sp;
	size_t sp_size = sizeof(struct dm_target_spec);
	int len;
	const char no_space[] = "Ran out of memory building ioctl parameter";

	out += sp_size;
	if (out >= end) {
		log_error(no_space);
		return NULL;
	}

	sp.status = 0;
	sp.sector_start = t->start;
	sp.length = t->length;
	strncpy(sp.target_type, t->type, sizeof(sp.target_type));

	len = strlen(t->params);

	if ((out + len + 1) >= end) {
		log_error(no_space);

		log_error("t->params= '%s'", t->params);
		return NULL;
	}
	strcpy((char *) out, t->params);
	out += len + 1;

	/* align next block */
	out = _align(out, ALIGNMENT);

	sp.next = out - out_sp;
	memcpy(out_sp, &sp, sp_size);

	return out;
}

static int _lookup_dev_name(uint64_t dev, char *buf, size_t len)
{
	struct dm_names *names;
	unsigned next = 0;
	struct dm_task *dmt;
	int r = 0;
 
	if (!(dmt = dm_task_create(DM_DEVICE_LIST)))
		return 0;
 
	if (!dm_task_run(dmt))
		goto out;

	if (!(names = dm_task_get_names(dmt)))
		goto out;
 
	if (!names->dev)
		goto out;
 
	do {
		names = (void *) names + next;
		if (names->dev == dev) {
			strncpy(buf, names->name, len);
			r = 1;
			break;
		}
		next = names->next;
	} while (next);

      out:
	dm_task_destroy(dmt);
	return r;
}

static struct dm_ioctl *_flatten(struct dm_task *dmt, unsigned repeat_count)
{
	const size_t min_size = 16 * 1024;
	const int (*version)[3];

	struct dm_ioctl *dmi;
	struct target *t;
	struct dm_target_msg *tmsg;
	size_t len = sizeof(struct dm_ioctl);
	void *b, *e;
	int count = 0;

	for (t = dmt->head; t; t = t->next) {
		len += sizeof(struct dm_target_spec);
		len += strlen(t->params) + 1 + ALIGNMENT;
		count++;
	}

	if (count && (dmt->sector || dmt->message)) {
		log_error("targets and message are incompatible");
		return NULL;
	}

	if (count && dmt->newname) {
		log_error("targets and newname are incompatible");
		return NULL;
	}

	if (count && dmt->geometry) {
		log_error("targets and geometry are incompatible");
		return NULL;
	}

	if (dmt->newname && (dmt->sector || dmt->message)) {
		log_error("message and newname are incompatible");
		return NULL;
	}

	if (dmt->newname && dmt->geometry) {
		log_error("geometry and newname are incompatible");
		return NULL;
	}

	if (dmt->geometry && (dmt->sector || dmt->message)) {
		log_error("geometry and message are incompatible");
		return NULL;
	}

	if (dmt->sector && !dmt->message) {
		log_error("message is required with sector");
		return NULL;
	}

	if (dmt->newname)
		len += strlen(dmt->newname) + 1;

	if (dmt->message)
		len += sizeof(struct dm_target_msg) + strlen(dmt->message) + 1;

	if (dmt->geometry)
		len += strlen(dmt->geometry) + 1;

	/*
	 * Give len a minimum size so that we have space to store
	 * dependencies or status information.
	 */
	if (len < min_size)
		len = min_size;

	/* Increase buffer size if repeating because buffer was too small */
	while (repeat_count--)
		len *= 2;

	if (!(dmi = dm_malloc(len)))
		return NULL;

	memset(dmi, 0, len);

	version = &_cmd_data_v4[dmt->type].version;

	dmi->version[0] = (*version)[0];
	dmi->version[1] = (*version)[1];
	dmi->version[2] = (*version)[2];

	dmi->data_size = len;
	dmi->data_start = sizeof(struct dm_ioctl);

	if (dmt->minor >= 0) {
		if (dmt->major <= 0) {
			log_error("Missing major number for persistent device.");
			goto bad;
		}
		dmi->flags |= DM_PERSISTENT_DEV_FLAG;
		dmi->dev = MKDEV(dmt->major, dmt->minor);
	}

	/* Does driver support device number referencing? */
	if (_dm_version_minor < 3 && !dmt->dev_name && !dmt->uuid && dmi->dev) {
		if (!_lookup_dev_name(dmi->dev, dmi->name, sizeof(dmi->name))) {
			log_error("Unable to find name for device (%" PRIu32
				  ":%" PRIu32 ")", dmt->major, dmt->minor);
			goto bad;
		}
		log_verbose("device (%" PRIu32 ":%" PRIu32 ") is %s "
			    "for compatibility with old kernel",
			    dmt->major, dmt->minor, dmi->name);
	}

	/* FIXME Until resume ioctl supplies name, use dev_name for readahead */
	if (dmt->dev_name && (dmt->type != DM_DEVICE_RESUME || dmt->minor < 0 ||
			      dmt->major < 0))
		strncpy(dmi->name, dmt->dev_name, sizeof(dmi->name));

	if (dmt->uuid)
		strncpy(dmi->uuid, dmt->uuid, sizeof(dmi->uuid));

	if (dmt->type == DM_DEVICE_SUSPEND)
		dmi->flags |= DM_SUSPEND_FLAG;
	if (dmt->no_flush)
		dmi->flags |= DM_NOFLUSH_FLAG;
	if (dmt->read_only)
		dmi->flags |= DM_READONLY_FLAG;
	if (dmt->skip_lockfs)
		dmi->flags |= DM_SKIP_LOCKFS_FLAG;

	dmi->target_count = count;
	dmi->event_nr = dmt->event_nr;

	b = (void *) (dmi + 1);
	e = (void *) ((char *) dmi + len);

	for (t = dmt->head; t; t = t->next)
		if (!(b = _add_target(t, b, e)))
			goto bad;

	if (dmt->newname)
		strcpy(b, dmt->newname);

	if (dmt->message) {
		tmsg = (struct dm_target_msg *) b;
		tmsg->sector = dmt->sector;
		strcpy(tmsg->message, dmt->message);
	}

	if (dmt->geometry)
		strcpy(b, dmt->geometry);

	return dmi;

      bad:
	dm_free(dmi);
	return NULL;
}

static int _process_mapper_dir(struct dm_task *dmt)
{
	struct dirent *dirent;
	DIR *d;
	const char *dir;
	int r = 1;

	dir = dm_dir();
	if (!(d = opendir(dir))) {
		log_sys_error("opendir", dir);
		return 0;
	}

	while ((dirent = readdir(d))) {
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, "..") ||
		    !strcmp(dirent->d_name, "control"))
			continue;
		dm_task_set_name(dmt, dirent->d_name);
		dm_task_run(dmt);
	}

	if (closedir(d))
		log_sys_error("closedir", dir);

	return r;
}

static int _process_all_v4(struct dm_task *dmt)
{
	struct dm_task *task;
	struct dm_names *names;
	unsigned next = 0;
	int r = 1;

	if (!(task = dm_task_create(DM_DEVICE_LIST)))
		return 0;

	if (!dm_task_run(task)) {
		r = 0;
		goto out;
	}

	if (!(names = dm_task_get_names(task))) {
		r = 0;
		goto out;
	}

	if (!names->dev)
		goto out;

	do {
		names = (void *) names + next;
		if (!dm_task_set_name(dmt, names->name)) {
			r = 0;
			goto out;
		}
		if (!dm_task_run(dmt))
			r = 0;
		next = names->next;
	} while (next);

      out:
	dm_task_destroy(task);
	return r;
}

static int _mknodes_v4(struct dm_task *dmt)
{
	(void) _process_mapper_dir(dmt);

	return _process_all_v4(dmt);
}

static int _create_and_load_v4(struct dm_task *dmt)
{
	struct dm_task *task;
	int r;

	/* Use new task struct to create the device */
	if (!(task = dm_task_create(DM_DEVICE_CREATE))) {
		log_error("Failed to create device-mapper task struct");
		return 0;
	}

	/* Copy across relevant fields */
	if (dmt->dev_name && !dm_task_set_name(task, dmt->dev_name)) {
		dm_task_destroy(task);
		return 0;
	}

	if (dmt->uuid && !dm_task_set_uuid(task, dmt->uuid)) {
		dm_task_destroy(task);
		return 0;
	}

	task->major = dmt->major;
	task->minor = dmt->minor;
	task->uid = dmt->uid;
	task->gid = dmt->gid;
	task->mode = dmt->mode;

	r = dm_task_run(task);
	dm_task_destroy(task);
	if (!r)
		return r;

	/* Next load the table */
	if (!(task = dm_task_create(DM_DEVICE_RELOAD))) {
		log_error("Failed to create device-mapper task struct");
		return 0;
	}

	/* Copy across relevant fields */
	if (dmt->dev_name && !dm_task_set_name(task, dmt->dev_name)) {
		dm_task_destroy(task);
		return 0;
	}

	task->read_only = dmt->read_only;
	task->head = dmt->head;
	task->tail = dmt->tail;

	r = dm_task_run(task);

	task->head = NULL;
	task->tail = NULL;
	dm_task_destroy(task);
	if (!r)
		goto revert;

	/* Use the original structure last so the info will be correct */
	dmt->type = DM_DEVICE_RESUME;
	dm_free(dmt->uuid);
	dmt->uuid = NULL;

	r = dm_task_run(dmt);

	if (r)
		return r;

      revert:
 	dmt->type = DM_DEVICE_REMOVE;
	dm_free(dmt->uuid);
	dmt->uuid = NULL;

	if (!dm_task_run(dmt))
		log_error("Failed to revert device creation.");

	return r;
}

uint64_t dm_task_get_existing_table_size(struct dm_task *dmt)
{
	return dmt->existing_table_size;
}

static int _reload_with_suppression_v4(struct dm_task *dmt)
{
	struct dm_task *task;
	struct target *t1, *t2;
	int r;

	/* New task to get existing table information */
	if (!(task = dm_task_create(DM_DEVICE_TABLE))) {
		log_error("Failed to create device-mapper task struct");
		return 0;
	}

	/* Copy across relevant fields */
	if (dmt->dev_name && !dm_task_set_name(task, dmt->dev_name)) {
		dm_task_destroy(task);
		return 0;
	}

	if (dmt->uuid && !dm_task_set_uuid(task, dmt->uuid)) {
		dm_task_destroy(task);
		return 0;
	}

	task->major = dmt->major;
	task->minor = dmt->minor;

	r = dm_task_run(task);

	if (!r) {
		dm_task_destroy(task);
		return r;
	}

	/* Store existing table size */
	t2 = task->head;
	while (t2 && t2->next)
		t2 = t2->next;
	dmt->existing_table_size = t2 ? t2->start + t2->length : 0;

	if ((task->dmi.v4->flags & DM_READONLY_FLAG) ? 1 : 0 != dmt->read_only)
		goto no_match;

	t1 = dmt->head;
	t2 = task->head;

	while (t1 && t2) {
		while (t2->params[strlen(t2->params) - 1] == ' ')
			t2->params[strlen(t2->params) - 1] = '\0';
		if ((t1->start != t2->start) ||
		    (t1->length != t2->length) ||
		    (strcmp(t1->type, t2->type)) ||
		    (strcmp(t1->params, t2->params)))
			goto no_match;
		t1 = t1->next;
		t2 = t2->next;
	}
	
	if (!t1 && !t2) {
		dmt->dmi.v4 = task->dmi.v4;
		task->dmi.v4 = NULL;
		dm_task_destroy(task);
		return 1;
	}

no_match:
	dm_task_destroy(task);

	/* Now do the original reload */
	dmt->suppress_identical_reload = 0;
	r = dm_task_run(dmt);

	return r;
}

static struct dm_ioctl *_do_dm_ioctl(struct dm_task *dmt, unsigned command,
				     unsigned repeat_count)
{
	struct dm_ioctl *dmi;

	dmi = _flatten(dmt, repeat_count);
	if (!dmi) {
		log_error("Couldn't create ioctl argument.");
		return NULL;
	}

	if (dmt->type == DM_DEVICE_TABLE)
		dmi->flags |= DM_STATUS_TABLE_FLAG;

	dmi->flags |= DM_EXISTS_FLAG;	/* FIXME */

	if (dmt->no_open_count)
		dmi->flags |= DM_SKIP_BDGET_FLAG;

	log_debug("dm %s %s %s%s%s %s%.0d%s%.0d%s"
		  "%s%c%c%s %.0" PRIu64 " %s [%u]",
		  _cmd_data_v4[dmt->type].name,
		  dmi->name, dmi->uuid, dmt->newname ? " " : "",
		  dmt->newname ? dmt->newname : "",
		  dmt->major > 0 ? "(" : "",
		  dmt->major > 0 ? dmt->major : 0,
		  dmt->major > 0 ? ":" : "",
		  dmt->minor > 0 ? dmt->minor : 0,
		  dmt->major > 0 && dmt->minor == 0 ? "0" : "",
		  dmt->major > 0 ? ") " : "",
		  dmt->no_open_count ? 'N' : 'O',
		  dmt->no_flush ? 'N' : 'F',
		  dmt->skip_lockfs ? "S " : "",
		  dmt->sector, dmt->message ? dmt->message : "",
		  dmi->data_size);
#ifdef DM_IOCTLS
	if (ioctl(_control_fd, command, dmi) < 0) {
		if (errno == ENXIO && ((dmt->type == DM_DEVICE_INFO) ||
				       (dmt->type == DM_DEVICE_MKNODES) ||
				       (dmt->type == DM_DEVICE_STATUS)))
			dmi->flags &= ~DM_EXISTS_FLAG;	/* FIXME */
		else {
			if (_log_suppress)
				log_verbose("device-mapper: %s ioctl "
					    "failed: %s",
				    	    _cmd_data_v4[dmt->type].name,
					    strerror(errno));
			else
				log_error("device-mapper: %s ioctl "
					  "failed: %s",
				    	   _cmd_data_v4[dmt->type].name,
					  strerror(errno));
			dm_free(dmi);
			return NULL;
		}
	}
#else /* Userspace alternative for testing */
#endif
	return dmi;
}

void dm_task_update_nodes(void)
{
	update_devs();
}

int dm_task_run(struct dm_task *dmt)
{
	struct dm_ioctl *dmi;
	unsigned command;

#ifdef DM_COMPAT
	if (_dm_version == 1)
		return _dm_task_run_v1(dmt);
#endif

	if ((unsigned) dmt->type >=
	    (sizeof(_cmd_data_v4) / sizeof(*_cmd_data_v4))) {
		log_error("Internal error: unknown device-mapper task %d",
			  dmt->type);
		return 0;
	}

	command = _cmd_data_v4[dmt->type].cmd;

	/* Old-style creation had a table supplied */
	if (dmt->type == DM_DEVICE_CREATE && dmt->head)
		return _create_and_load_v4(dmt);

	if (dmt->type == DM_DEVICE_MKNODES && !dmt->dev_name &&
	    !dmt->uuid && dmt->major <= 0)
		return _mknodes_v4(dmt);

	if ((dmt->type == DM_DEVICE_RELOAD) && dmt->suppress_identical_reload)
		return _reload_with_suppression_v4(dmt);

	if (!_open_control())
		return 0;

repeat_ioctl:
	if (!(dmi = _do_dm_ioctl(dmt, command, _ioctl_buffer_double_factor)))
		return 0;

	if (dmi->flags & DM_BUFFER_FULL_FLAG) {
		switch (dmt->type) {
		case DM_DEVICE_LIST_VERSIONS:
		case DM_DEVICE_LIST:
		case DM_DEVICE_DEPS:
		case DM_DEVICE_STATUS:
		case DM_DEVICE_TABLE:
		case DM_DEVICE_WAITEVENT:
			_ioctl_buffer_double_factor++;
			dm_free(dmi);
			goto repeat_ioctl;
		default:
			log_error("WARNING: libdevmapper buffer too small for data");
		}
	}

	switch (dmt->type) {
	case DM_DEVICE_CREATE:
		if (dmt->dev_name && *dmt->dev_name)
			add_dev_node(dmt->dev_name, MAJOR(dmi->dev),
				     MINOR(dmi->dev), dmt->uid, dmt->gid,
				     dmt->mode);
		break;

	case DM_DEVICE_REMOVE:
		/* FIXME Kernel needs to fill in dmi->name */
		if (dmt->dev_name)
			rm_dev_node(dmt->dev_name);
		break;

	case DM_DEVICE_RENAME:
		/* FIXME Kernel needs to fill in dmi->name */
		if (dmt->dev_name)
			rename_dev_node(dmt->dev_name, dmt->newname);
		break;

	case DM_DEVICE_RESUME:
		/* FIXME Kernel needs to fill in dmi->name */
		set_dev_node_read_ahead(dmt->dev_name, dmt->read_ahead,
					dmt->read_ahead_flags);
		break;
	
	case DM_DEVICE_MKNODES:
		if (dmi->flags & DM_EXISTS_FLAG)
			add_dev_node(dmi->name, MAJOR(dmi->dev),
				     MINOR(dmi->dev),
				     dmt->uid, dmt->gid, dmt->mode);
		else if (dmt->dev_name)
			rm_dev_node(dmt->dev_name);
		break;

	case DM_DEVICE_STATUS:
	case DM_DEVICE_TABLE:
	case DM_DEVICE_WAITEVENT:
		if (!_unmarshal_status(dmt, dmi))
			goto bad;
		break;
	}

	/* Was structure reused? */
	if (dmt->dmi.v4)
		dm_free(dmt->dmi.v4);
	dmt->dmi.v4 = dmi;
	return 1;

      bad:
	dm_free(dmi);
	return 0;
}

void dm_lib_release(void)
{
	if (_control_fd != -1) {
		close(_control_fd);
		_control_fd = -1;
	}
	update_devs();
}

void dm_lib_exit(void)
{
	dm_lib_release();
	if (_dm_bitset)
		dm_bitset_destroy(_dm_bitset);
	_dm_bitset = NULL;
	dm_dump_memory();
	_version_ok = 1;
	_version_checked = 0;
}
