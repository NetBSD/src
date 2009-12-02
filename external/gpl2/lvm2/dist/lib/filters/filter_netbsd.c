/*      $NetBSD: filter_netbsd.c,v 1.3 2009/12/02 01:53:25 haad Exp $        */

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 * Copyright (C) 2008 Adam Hamsik. All rights reserved. 
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
#include "filter.h"
#include "lvm-string.h"
#include "config.h"
#include "metadata.h"
#include "activate.h"

#include <sys/sysctl.h>

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#define NUMBER_OF_MAJORS 4096

#define LVM_SUCCESS 1
#define LVM_FAILURE 0

/* -1 means LVM won't use this major number. */
static int _char_device_major[NUMBER_OF_MAJORS];
static int _block_device_major[NUMBER_OF_MAJORS];

typedef struct {
	const char *name;
	const int max_partitions;
} device_info_t;

static int _md_major = -1;
static int _device_mapper_major = -1;

int md_major(void)
{
	return _md_major;
}

int dev_subsystem_part_major(const struct device *dev)
{
	return 0;
}

const char *dev_subsystem_name(const struct device *dev)
{
	return "";
}

/*
 * Devices are only checked for partition tables if their minor number
 * is a multiple of the number corresponding to their type below
 * i.e. this gives the granularity of whole-device minor numbers.
 * Use 1 if the device is not partitionable.
 *
 * The list can be supplemented with devices/types in the config file.
 */
static const device_info_t device_info[] = {
	{"wd", 64},
	{"sd", 64},	
	{"dk", 1},
	{"wd", 64},	
	{"vnd", 1},
	{"raid", 64},
	{"cgd", 1},
	{"ccd", 1},	
	{"xbd", 64},
	{NULL, -1}
};

/*
 * Test if device passes filter tests and can be inserted in to cache.
 */
static int _passes_lvm_type_device_filter(struct dev_filter *f __attribute((unused)),
					  struct device *dev)
{
	const char *name = dev_name(dev);
	int ret = 0;
	uint64_t size;

	/* Is this a recognised device type? */
	if (_char_device_major[MAJOR(dev->dev)] == -1 ){
		log_debug("%s: Skipping: Unrecognised LVM device type %"
				  PRIu64, name, (uint64_t) MAJOR(dev->dev));
		return LVM_FAILURE;
	}

	/* Skip suspended devices */
	if (MAJOR(dev->dev) == _device_mapper_major &&
	    ignore_suspended_devices() && !device_is_usable(dev->dev)) {
		log_debug("%s: Skipping: Suspended dm device", name);
		return LVM_FAILURE;
	}
	
	/* Check it's accessible */
	if (!dev_open_flags(dev, O_RDONLY, 0, 1)) {
		log_debug("%s: Skipping: open failed", name);
		return LVM_FAILURE;
	}

	/* Check it's not too small */
	if (!dev_get_size(dev, &size)) {
	       	log_debug("%s: Skipping: dev_get_size failed", name);
		goto out;
	}
	
	if (size < PV_MIN_SIZE) {
		log_debug("%s: Skipping: Too small to hold a PV", name);
		goto out;
	}

	if (is_partitioned_dev(dev)) {
		log_debug("%s: Skipping: Partition table signature found",
			  name);
		goto out;
	}

	ret = LVM_SUCCESS;

      out:
	dev_close(dev);

	return ret;
}

static int _scan_dev(const struct config_node *cn)
{
	size_t val_len,i,j;
	char *name;

	struct kinfo_drivers *kd;
	struct config_value *cv;
	
	/* All types unrecognised initially */
	memset(_char_device_major, -1, sizeof(int) * NUMBER_OF_MAJORS);
	memset(_block_device_major, -1, sizeof(int) * NUMBER_OF_MAJORS);

	/* get size kernel drivers array from kernel*/
	if (sysctlbyname("kern.drivers", NULL, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed");
		return LVM_FAILURE;
	}
	
	if ((kd = malloc(val_len)) == NULL){
		printf("malloc kd info error\n");
		return LVM_FAILURE;
	}

	/* get array from kernel */
	if (sysctlbyname("kern.drivers", kd, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed kd");
		return LVM_FAILURE;
	}

	for (i = 0, val_len /= sizeof(*kd); i < val_len; i++) {

		if (!strncmp("device-mapper", kd[i].d_name, 13) ||
		    !strncmp("dm", kd[i].d_name, 2))
			_device_mapper_major = kd[i].d_bmajor;

		/* We select only devices with correct char/block major number. */
		if (kd[i].d_cmajor != -1 && kd[i].d_bmajor != -1) {
			/* Go through the valid device names and if there is a
			   match store max number of partitions */
			for (j = 0; device_info[j].name != NULL; j++){
				if (!strcmp(device_info[j].name, kd[i].d_name)){
					_char_device_major[kd[i].d_cmajor] =
					    device_info[j].max_partitions;
					_block_device_major[kd[i].d_bmajor] =
					    device_info[j].max_partitions;
					break;
				}
			}
		}
		
		if (!cn)
			continue;

		/* Check devices/types for local variations */
		for (cv = cn->v; cv; cv = cv->next) {
			if (cv->type != CFG_STRING) {
				log_error("Expecting string in devices/types "
					  "in config file");
				free(kd);
				return LVM_FAILURE;
			}

			name = cv->v.str;
			cv = cv->next;
			if (!cv || cv->type != CFG_INT) {
				log_error("Max partition count missing for %s "
					  "in devices/types in config file",
					  name);
				free(kd);
				return LVM_FAILURE;
			}
			if (!cv->v.i) {
				log_error("Zero partition count invalid for "
					  "%s in devices/types in config file",
					  name);
				free(kd);
				return LVM_FAILURE;
			}
			
			if (!strncmp(name, kd[i].d_name, strlen(name))){
					_char_device_major[kd[i].d_cmajor] =
					    device_info[j].max_partitions;
					_block_device_major[kd[i].d_bmajor] =
					    device_info[j].max_partitions;
					break;
			}
		}
	}

	free(kd);

	return LVM_SUCCESS;
}

int max_partitions(int major)
{
	return _char_device_major[major];
}

struct dev_filter *lvm_type_filter_create(const char *proc,
					  const struct config_node *cn)
{
	struct dev_filter *f;

	if (!(f = dm_malloc(sizeof(struct dev_filter)))) {
		log_error("LVM type filter allocation failed");
		return NULL;
	}

	f->passes_filter = _passes_lvm_type_device_filter;
	f->destroy = lvm_type_filter_destroy;
	f->private = NULL;

	if (!_scan_dev(cn)) {
		dm_free(f);
		return_NULL;
	}

	return f;
}

void lvm_type_filter_destroy(struct dev_filter *f)
{
	dm_free(f);
	return;
}
