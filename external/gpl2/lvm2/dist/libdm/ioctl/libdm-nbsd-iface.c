/*      $NetBSD: libdm-nbsd-iface.c,v 1.5 2009/12/05 11:42:24 haad Exp $        */

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 * Copyright (C) 2008 Adam Hamsik. All rights reserved.
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
#include "libdm-netbsd.h"

#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <fcntl.h>
#include <dirent.h>
#include <limits.h>

#include <dev/dm/netbsd-dm.h>

#include <dm-ioctl.h>

/*
 * Ensure build compatibility.  
 * The hard-coded versions here are the highest present 
 * in the _cmd_data arrays.
 */

#if !((DM_VERSION_MAJOR == 1 && DM_VERSION_MINOR >= 0) || \
      (DM_VERSION_MAJOR == 4 && DM_VERSION_MINOR >= 0))
#error The version of dm-ioctl.h included is incompatible.
#endif

/* dm major version no for running kernel */
static unsigned _dm_version_minor = 0;
static unsigned _dm_version_patchlevel = 0;

static int _control_fd = -1;
static int _version_checked = 0;
static int _version_ok = 1;
static unsigned _ioctl_buffer_double_factor = 0;

/* *INDENT-OFF* */

/*
 * XXX Remove this structure and write another own one
 * I don't understand why ioctl calls has different
 * names then dm task type
 */
static struct cmd_data _cmd_data_v4[] = {
	{"create",	DM_DEV_CREATE,		{4, 0, 0}},
	{"reload",	DM_TABLE_LOAD,		{4, 0, 0}}, /* DM_DEVICE_RELOAD */
	{"remove",	DM_DEV_REMOVE,		{4, 0, 0}},
	{"remove_all",	DM_REMOVE_ALL,		{4, 0, 0}},
	{"suspend",	DM_DEV_SUSPEND,		{4, 0, 0}},
	{"resume",	DM_DEV_SUSPEND,		{4, 0, 0}},
	{"info",	DM_DEV_STATUS,		{4, 0, 0}},
	{"deps",	DM_TABLE_DEPS,		{4, 0, 0}}, /* DM_DEVICE_DEPS */
	{"rename",	DM_DEV_RENAME,		{4, 0, 0}},
	{"version",	DM_VERSION,		{4, 0, 0}},
	{"status",	DM_TABLE_STATUS,	{4, 0, 0}},
	{"table",	DM_TABLE_STATUS,	{4, 0, 0}}, /* DM_DEVICE_TABLE */
	{"waitevent",	DM_DEV_WAIT,		{4, 0, 0}},
	{"names",	DM_LIST_DEVICES,	{4, 0, 0}},
	{"clear",	DM_TABLE_CLEAR,		{4, 0, 0}},
	{"mknodes",	DM_DEV_STATUS,		{4, 0, 0}},
#ifdef DM_LIST_VERSIONS
	{"targets",	DM_LIST_VERSIONS,	{4, 1, 0}},
#endif
#ifdef DM_TARGET_MSG
	{"message",	DM_TARGET_MSG,		{4, 2, 0}},
#endif
#ifdef DM_DEV_SET_GEOMETRY
	{"setgeometry",	DM_DEV_SET_GEOMETRY,	{4, 6, 0}},
#endif
};
/* *INDENT-ON* */

/*
 * In NetBSD we use sysctl to get kernel drivers info. control device
 * has predefined minor number 0 and major number = char major number
 * of dm driver. First slot is therefore ocupied with control device
 * and minor device starts from 1;
 */

static int _control_device_number(uint32_t *major, uint32_t *minor)
{

	nbsd_get_dm_major(major, DM_CHAR_MAJOR);
	
	*minor = 0;
	
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


	return 1;
}

/* Check if major is device-mapper block device major number */
int dm_is_dm_major(uint32_t major)
{
	uint32_t dm_major;

	nbsd_get_dm_major(&dm_major, DM_BLOCK_MAJOR);
	
	if (major == dm_major)
		return 1;

	return 0;
}

/* Open control device if doesn't exist create it. */
static int _open_control(void)
{
	char control[PATH_MAX];
	uint32_t major = 0, minor = 0;

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

	return 1;

error:
	log_error("Failure to communicate with kernel device-mapper driver.");
	return 0;
}

/*
 * Destroy dm task structure there are some dynamically alocated values there.
 * name, uuid, head, tail list.
 */
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

/* Get kernel driver version from dm_ioctl structure. */
int dm_task_get_driver_version(struct dm_task *dmt, char *version, size_t size)
{
	unsigned *v;

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

/* Get kernel driver protocol version and comapre it with library version. */
static int _check_version(char *version, size_t size)
{
	struct dm_task *task;
	int r;

	if (!(task = dm_task_create(DM_DEVICE_VERSION))) {
		log_error("Failed to get device-mapper version");
		version[0] = '\0';
		return 0;
	}

	r = dm_task_run(task);
	dm_task_get_driver_version(task, version, size);
	dm_task_destroy(task);

	return r;
}

/*
 * Find out device-mapper's major version number the first time 
 * this is called and whether or not we support it.
 */
int dm_check_version(void)
{
	char dmversion[64];

	if (_version_checked)
		return _version_ok;

	_version_checked = 1;

	if (_check_version(dmversion, sizeof(dmversion)))
		return 1;


	return 0;
}

int dm_cookie_supported(void)
{
	return (0);
}

/* Get next target(table description) from list pointed by dmt->head. */
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

/*
 * @dev_major is major number of char device
 *
 * I have to find it's block device number and lookup dev in
 * device database to find device path.
 *
 */

int dm_format_dev(char *buf, int bufsize, uint32_t dev_major,
		  uint32_t dev_minor)
{
	int r;
	uint32_t major, dm_major;
	char *name;
	mode_t mode;
	dev_t dev;
	size_t val_len,i;
	struct kinfo_drivers *kd;
	
	mode = 0;
	
	nbsd_get_dm_major(&dm_major, DM_BLOCK_MAJOR);

	if (bufsize < 8)
		return 0;
	
	if (sysctlbyname("kern.drivers",NULL,&val_len,NULL,0) < 0) {
		printf("sysctlbyname failed");
		return 0;
	}

	if ((kd = malloc (val_len)) == NULL){
		printf("malloc kd info error\n");
		return 0;
	}

	if (sysctlbyname("kern.drivers", kd, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed kd");
		return 0;
	}

	for (i = 0, val_len /= sizeof(*kd); i < val_len; i++){
		if (kd[i].d_cmajor == dev_major) {
			major = kd[i].d_bmajor;
			break;
		}
	}
	
	dev = MKDEV(major,dev_minor);

	mode |= S_IFBLK;
	
	name = devname(dev,mode);

	r = snprintf(buf, (size_t) bufsize, "/dev/%s",name);

	free(kd);
	
	if (r < 0 || r > bufsize - 1 || name == NULL)
		return 0;
	
	return 1;
}

/* Fill info from dm_ioctl structure. Look at DM_EXISTS_FLAG*/
int dm_task_get_info(struct dm_task *dmt, struct dm_info *info)
{
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
	
	nbsd_get_dm_major(&info->major, DM_BLOCK_MAJOR); /* get netbsd dm device major number */
	info->minor = MINOR(dmt->dmi.v4->dev);
	
	return 1;
}

/* Unsupported on NetBSD */
uint32_t dm_task_get_read_ahead(const struct dm_task *dmt, uint32_t *read_ahead)
{
	*read_ahead = DM_READ_AHEAD_NONE;
	return 1;
}

const char *dm_task_get_name(const struct dm_task *dmt)
{

	return (dmt->dmi.v4->name);
}

const char *dm_task_get_uuid(const struct dm_task *dmt)
{

	return (dmt->dmi.v4->uuid);
}

struct dm_deps *dm_task_get_deps(struct dm_task *dmt)
{
	return (struct dm_deps *) (((void *) dmt->dmi.v4) +
				   dmt->dmi.v4->data_start);
}

struct dm_names *dm_task_get_names(struct dm_task *dmt)
{
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

/* Unsupported on NetBSD */
int dm_task_set_read_ahead(struct dm_task *dmt, uint32_t read_ahead,
			   uint32_t read_ahead_flags)
{
	return 1;
}

int dm_task_suppress_identical_reload(struct dm_task *dmt)
{
	dmt->suppress_identical_reload = 1;
	return 1;
}

int dm_task_set_newname(struct dm_task *dmt, const char *newname)
{
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

/* Unsupported in NetBSD */
int dm_task_set_geometry(struct dm_task *dmt, const char *cylinders,
    const char *heads, const char *sectors, const char *start)
{
	return 0;
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

int dm_task_query_inactive_table(struct dm_task *dmt)
{
	dmt->query_inactive_table = 1;

	return 1;
}

int dm_task_set_event_nr(struct dm_task *dmt, uint32_t event_nr)
{
	dmt->event_nr = event_nr;

	return 1;
}

/* Allocate one target(table description) entry. */
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

/* Parse given dm task structure to proplib dictionary.  */
static int _flatten(struct dm_task *dmt, prop_dictionary_t dm_dict)
{
	prop_array_t cmd_array;
	prop_dictionary_t target_spec;
	
	struct target *t;
	
	size_t len;
	char type[DM_MAX_TYPE_NAME];
	
	uint32_t major, flags;
	int count = 0;
	const int (*version)[3];
	
	flags = 0;
	version = &_cmd_data_v4[dmt->type].version;

	cmd_array = prop_array_create();

	for (t = dmt->head; t; t = t->next) {
		target_spec = prop_dictionary_create();

		prop_dictionary_set_uint64(target_spec,DM_TABLE_START,t->start);
		prop_dictionary_set_uint64(target_spec,DM_TABLE_LENGTH,t->length);

		strlcpy(type,t->type,DM_MAX_TYPE_NAME);

		prop_dictionary_set_cstring(target_spec,DM_TABLE_TYPE,type);
		prop_dictionary_set_cstring(target_spec,DM_TABLE_PARAMS,t->params);

		prop_array_set(cmd_array,count,target_spec);

		prop_object_release(target_spec);
		
		count++;
	}

	
	if (count && (dmt->sector || dmt->message)) {
		log_error("targets and message are incompatible");
		return -1;
	}

	if (count && dmt->newname) {
		log_error("targets and newname are incompatible");
		return -1;
	}

	if (count && dmt->geometry) {
		log_error("targets and geometry are incompatible");
		return -1;
	}

	if (dmt->newname && (dmt->sector || dmt->message)) {
		log_error("message and newname are incompatible");
		return -1;
	}

	if (dmt->newname && dmt->geometry) {
		log_error("geometry and newname are incompatible");
		return -1;
	}

	if (dmt->geometry && (dmt->sector || dmt->message)) {
		log_error("geometry and message are incompatible");
		return -1;
	}

	if (dmt->sector && !dmt->message) {
		log_error("message is required with sector");
		return -1;
	}

	if (dmt->newname)
		len += strlen(dmt->newname) + 1;

	if (dmt->message)
		len += sizeof(struct dm_target_msg) + strlen(dmt->message) + 1;

	if (dmt->geometry)
		len += strlen(dmt->geometry) + 1;

	nbsd_dmi_add_version((*version), dm_dict);
	    
	nbsd_get_dm_major(&major, DM_BLOCK_MAJOR);
	/* 
	 * Only devices with major which is equal to netbsd dm major 
	 * dm devices in NetBSD can't have more majors then one assigned to dm.
	 */
	if (dmt->major != major && dmt->major != -1)
		return -1;
		
	if (dmt->minor >= 0) {
		flags |= DM_PERSISTENT_DEV_FLAG;
		
		prop_dictionary_set_uint32(dm_dict, DM_IOCTL_MINOR, dmt->minor);
	}

	/* Set values to dictionary. */
	if (dmt->dev_name)
		prop_dictionary_set_cstring(dm_dict, DM_IOCTL_NAME, dmt->dev_name);

	if (dmt->uuid)
		prop_dictionary_set_cstring(dm_dict, DM_IOCTL_UUID, dmt->uuid);
	
	if (dmt->type == DM_DEVICE_SUSPEND)
		flags |= DM_SUSPEND_FLAG;
	if (dmt->no_flush)
		flags |= DM_NOFLUSH_FLAG;
	if (dmt->read_only)
		flags |= DM_READONLY_FLAG;
	if (dmt->skip_lockfs)
		flags |= DM_SKIP_LOCKFS_FLAG;

	if (dmt->query_inactive_table) {
		if (_dm_version_minor < 16)
			log_warn("WARNING: Inactive table query unsupported "
				 "by kernel.  It will use live table.");
		flags |= DM_QUERY_INACTIVE_TABLE_FLAG;
	}
	
	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_FLAGS, flags);

	prop_dictionary_set_uint32(dm_dict, DM_IOCTL_EVENT, dmt->event_nr);

	if (dmt->newname)
		prop_array_set_cstring(cmd_array, 0, dmt->newname);
	
	/* Add array for all COMMAND specific data. */
	prop_dictionary_set(dm_dict, DM_IOCTL_CMD_DATA, cmd_array);
	prop_object_release(cmd_array);
	
	return 0;
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

/* Get list of all devices. */
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

/* Create new device and load table to it. */
static int _create_and_load_v4(struct dm_task *dmt)
{
	struct dm_task *task;
	int r;

	printf("create and load called \n");
	
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

/*
 * This function is heart of NetBSD libdevmapper-> device-mapper kernel protocol
 * It creates proplib_dictionary from dm task structure and sends it to NetBSD
 * kernel driver. After succesfull ioctl it create dmi structure from returned
 * proplib dictionary. This way I keep number of changes in NetBSD version of
 * libdevmapper as small as posible.
 */
static struct dm_ioctl *_do_dm_ioctl(struct dm_task *dmt, unsigned command)
{
	struct dm_ioctl *dmi;
	prop_dictionary_t dm_dict_in, dm_dict_out;
	
	uint32_t flags;

	dm_dict_in = NULL;
	
	dm_dict_in = prop_dictionary_create(); /* Dictionary send to kernel */
	dm_dict_out = prop_dictionary_create(); /* Dictionary received from kernel */

	/* Set command name to dictionary */
	prop_dictionary_set_cstring(dm_dict_in, DM_IOCTL_COMMAND,
	    _cmd_data_v4[dmt->type].name);

	/* Parse dmi from libdevmapper to dictionary */
	if (_flatten(dmt, dm_dict_in) < 0)
		goto bad;

	prop_dictionary_get_uint32(dm_dict_in, DM_IOCTL_FLAGS, &flags);
		
	if (dmt->type == DM_DEVICE_TABLE)
		flags |= DM_STATUS_TABLE_FLAG;

	if (dmt->no_open_count)
		flags |= DM_SKIP_BDGET_FLAG;

	flags |= DM_EXISTS_FLAG;
	
	/* Set flags to dictionary. */
	prop_dictionary_set_uint32(dm_dict_in,DM_IOCTL_FLAGS,flags);
	
	prop_dictionary_externalize_to_file(dm_dict_in,"/tmp/test_in");
	
	log_very_verbose("Ioctl type  %s --- flags %d",_cmd_data_v4[dmt->type].name,flags);
	//printf("name %s, major %d minor %d\n uuid %s\n", 
        //dm_task_get_name(dmt), dmt->minor, dmt->major, dm_task_get_uuid(dmt));
	/* Send dictionary to kernel and wait for reply. */
	if (prop_dictionary_sendrecv_ioctl(dm_dict_in,_control_fd,
		NETBSD_DM_IOCTL,&dm_dict_out) != 0) {

		if (errno == ENOENT &&
		    ((dmt->type == DM_DEVICE_INFO) ||
			(dmt->type == DM_DEVICE_MKNODES) ||
			(dmt->type == DM_DEVICE_STATUS))) {

			/*
			 * Linux version doesn't fail when ENOENT is returned
			 * for nonexisting device after info, deps, mknodes call.
			 * It returns dmi sent to kernel with DM_EXISTS_FLAG = 0;
			 */
			
			dmi = nbsd_dm_dict_to_dmi(dm_dict_in,_cmd_data_v4[dmt->type].cmd);

			dmi->flags &= ~DM_EXISTS_FLAG; 

			prop_object_release(dm_dict_in);
			prop_object_release(dm_dict_out);

			goto out;
		} else {
			log_error("ioctl %s call failed with errno %d\n", 
					  _cmd_data_v4[dmt->type].name, errno);

			prop_object_release(dm_dict_in);
			prop_object_release(dm_dict_out);

			goto bad;
		}
	}

	prop_dictionary_externalize_to_file(dm_dict_out,"/tmp/test_out");

	/* Parse kernel dictionary to dmi structure and return it to libdevmapper. */
	dmi = nbsd_dm_dict_to_dmi(dm_dict_out,_cmd_data_v4[dmt->type].cmd);

	prop_object_release(dm_dict_in);
	prop_object_release(dm_dict_out);
out:	
	return dmi;
bad:
	return NULL;
}

/* Create new edvice nodes in mapper/ dir. */
void dm_task_update_nodes(void)
{
	update_devs();
}

/* Run dm command which is descirbed in dm_task structure. */
int dm_task_run(struct dm_task *dmt)
{
	struct dm_ioctl *dmi;
	unsigned command;

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

	if (!(dmi = _do_dm_ioctl(dmt, command)))
		return 0;

	switch (dmt->type) {
	case DM_DEVICE_CREATE:
		add_dev_node(dmt->dev_name, MAJOR(dmi->dev), MINOR(dmi->dev),
		    dmt->uid, dmt->gid, dmt->mode, 0);
		break;

	case DM_DEVICE_REMOVE:
		/* FIXME Kernel needs to fill in dmi->name */
		if (dmt->dev_name)
			rm_dev_node(dmt->dev_name, 0);
		break;

	case DM_DEVICE_RENAME:
		/* FIXME Kernel needs to fill in dmi->name */
		if (dmt->dev_name)
			rename_dev_node(dmt->dev_name, dmt->newname, 0);
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
			    dmt->uid, dmt->gid, dmt->mode, 0);
		else if (dmt->dev_name)
			rm_dev_node(dmt->dev_name, 0);
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
	dm_dump_memory();
	_version_ok = 1;
	_version_checked = 0;
}
