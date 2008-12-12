/*	$NetBSD: pvcreate.c,v 1.1.1.2 2008/12/12 11:43:11 haad Exp $	*/

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

#include "tools.h"
#include "metadata.h"

struct pvcreate_params {
	int zero;
	uint64_t size;
	int pvmetadatacopies;
	uint64_t pvmetadatasize;
	int64_t labelsector;
	struct id id; /* FIXME: redundant */
	struct id *idp; /* 0 if no --uuid option */
	uint64_t pe_start;
	uint32_t extent_count;
	uint32_t extent_size;
	const char *restorefile; /* 0 if no --restorefile option */
	force_t force;
	unsigned yes;
};

const char _really_init[] =
    "Really INITIALIZE physical volume \"%s\" of volume group \"%s\" [y/n]? ";

/*
 * See if we may pvcreate on this device.
 * 0 indicates we may not.
 */
static int pvcreate_check(struct cmd_context *cmd, const char *name,
			  struct pvcreate_params *pp)
{
	struct physical_volume *pv;
	struct device *dev;
	uint64_t md_superblock;

	/* FIXME Check partition type is LVM unless --force is given */

	/* Is there a pv here already? */
	pv = pv_read(cmd, name, NULL, NULL, 0);

	/*
	 * If a PV has no MDAs it may appear to be an orphan until the
	 * metadata is read off another PV in the same VG.  Detecting
	 * this means checking every VG by scanning every PV on the
	 * system.
	 */
	if (pv && is_orphan(pv)) {
		if (!scan_vgs_for_pvs(cmd))
			return_0;
		pv = pv_read(cmd, name, NULL, NULL, 0);
	}

	/* Allow partial & exported VGs to be destroyed. */
	/* We must have -ff to overwrite a non orphan */
	if (pv && !is_orphan(pv) && pp->force != DONT_PROMPT_OVERRIDE) {
		log_error("Can't initialize physical volume \"%s\" of "
			  "volume group \"%s\" without -ff", name, pv_vg_name(pv));
		return 0;
	}

	/* prompt */
	if (pv && !is_orphan(pv) && !pp->yes &&
	    yes_no_prompt(_really_init, name, pv_vg_name(pv)) == 'n') {
		log_print("%s: physical volume not initialized", name);
		return 0;
	}

	if (sigint_caught())
		return 0;

	dev = dev_cache_get(name, cmd->filter);

	/* Is there an md superblock here? */
	if (!dev && md_filtering()) {
		unlock_vg(cmd, VG_ORPHANS);

		persistent_filter_wipe(cmd->filter);
		lvmcache_destroy(cmd, 1);

		init_md_filtering(0);
		if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
			log_error("Can't get lock for orphan PVs");
			init_md_filtering(1);
			return 0;
		}
		dev = dev_cache_get(name, cmd->filter);
		init_md_filtering(1);
	}

	if (!dev) {
		log_error("Device %s not found (or ignored by filtering).", name);
		return 0;
	}

	/*
 	 * This test will fail if the device belongs to an MD array.
	 */
	if (!dev_test_excl(dev)) {
		/* FIXME Detect whether device-mapper itself is still using it */
		log_error("Can't open %s exclusively.  Mounted filesystem?",
			  name);
		return 0;
	}

	/* Wipe superblock? */
	if (dev_is_md(dev, &md_superblock) &&
	    ((!pp->idp && !pp->restorefile) || pp->yes ||
	     (yes_no_prompt("Software RAID md superblock "
			    "detected on %s. Wipe it? [y/n] ", name) == 'y'))) {
		log_print("Wiping software RAID md superblock on %s", name);
		if (!dev_set(dev, md_superblock, 4, 0)) {
			log_error("Failed to wipe RAID md superblock on %s",
				  name);
			return 0;
		}
	}

	if (sigint_caught())
		return 0;

	if (pv && !is_orphan(pv) && pp->force) {
		log_warn("WARNING: Forcing physical volume creation on "
			  "%s%s%s%s", name,
			  !is_orphan(pv) ? " of volume group \"" : "",
			  !is_orphan(pv) ? pv_vg_name(pv) : "",
			  !is_orphan(pv) ? "\"" : "");
	}

	return 1;
}

static int pvcreate_single(struct cmd_context *cmd, const char *pv_name,
			   void *handle)
{
	struct pvcreate_params *pp = (struct pvcreate_params *) handle;
	void *pv;
	struct device *dev;
	struct dm_list mdas;

	if (pp->idp) {
		if ((dev = device_from_pvid(cmd, pp->idp)) &&
		    (dev != dev_cache_get(pv_name, cmd->filter))) {
			log_error("uuid %s already in use on \"%s\"",
				  pp->idp->uuid, dev_name(dev));
			return 0;
		}
	}

	if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		return 0;
	}

	if (!pvcreate_check(cmd, pv_name, pp))
		goto error;

	if (sigint_caught())
		goto error;

	if (!(dev = dev_cache_get(pv_name, cmd->filter))) {
		log_error("%s: Couldn't find device.  Check your filters?",
			  pv_name);
		goto error;
	}

	dm_list_init(&mdas);
	if (!(pv = pv_create(cmd, dev, pp->idp, pp->size, pp->pe_start,
			     pp->extent_count, pp->extent_size,
			     pp->pvmetadatacopies,
			     pp->pvmetadatasize,&mdas))) {
		log_error("Failed to setup physical volume \"%s\"", pv_name);
		goto error;
	}

	log_verbose("Set up physical volume for \"%s\" with %" PRIu64
		    " available sectors", pv_name, pv_size(pv));

	/* Wipe existing label first */
	if (!label_remove(pv_dev(pv))) {
		log_error("Failed to wipe existing label on %s", pv_name);
		goto error;
	}

	if (pp->zero) {
		log_verbose("Zeroing start of device %s", pv_name);
		if (!dev_open_quiet(dev)) {
			log_error("%s not opened: device not zeroed", pv_name);
			goto error;
		}

		if (!dev_set(dev, UINT64_C(0), (size_t) 2048, 0)) {
			log_error("%s not wiped: aborting", pv_name);
			dev_close(dev);
			goto error;
		}
		dev_close(dev);
	}

	log_very_verbose("Writing physical volume data to disk \"%s\"",
			 pv_name);
	if (!(pv_write(cmd, (struct physical_volume *)pv, &mdas,
		       pp->labelsector))) {
		log_error("Failed to write physical volume \"%s\"", pv_name);
		goto error;
	}

	log_print("Physical volume \"%s\" successfully created", pv_name);

	unlock_vg(cmd, VG_ORPHANS);
	return 1;

      error:
	unlock_vg(cmd, VG_ORPHANS);
	return 0;
}

/*
 * Intial sanity checking of command-line arguments and fill in 'pp' fields.
 *
 * Input arguments:
 * cmd, argc, argv
 *
 * Output arguments:
 * pp: structure allocated by caller, fields written / validated here
 */
static int pvcreate_validate_params(struct cmd_context *cmd,
				    int argc, char **argv,
				    struct pvcreate_params *pp)
{
	const char *uuid = NULL;
	void *existing_pv;
	struct volume_group *vg;

	memset(pp, 0, sizeof(*pp));

	if (!argc) {
		log_error("Please enter a physical volume path");
		return 0;
	}

	if (arg_count(cmd, restorefile_ARG) && !arg_count(cmd, uuidstr_ARG)) {
		log_error("--uuid is required with --restorefile");
		return 0;
	}

	if (arg_count(cmd, uuidstr_ARG) && argc != 1) {
		log_error("Can only set uuid on one volume at once");
		return 0;
	}

 	if (arg_count(cmd, uuidstr_ARG)) {
		uuid = arg_str_value(cmd, uuidstr_ARG, "");
		if (!id_read_format(&pp->id, uuid))
			return 0;
		pp->idp = &pp->id;
	}

	if (arg_count(cmd, restorefile_ARG)) {
		pp->restorefile = arg_str_value(cmd, restorefile_ARG, "");
		/* The uuid won't already exist */
		if (!(vg = backup_read_vg(cmd, NULL, pp->restorefile))) {
			log_error("Unable to read volume group from %s",
				  pp->restorefile);
			return 0;
		}
		if (!(existing_pv = find_pv_in_vg_by_uuid(vg, pp->idp))) {
			log_error("Can't find uuid %s in backup file %s",
				  uuid, pp->restorefile);
			return 0;
		}
		pp->pe_start = pv_pe_start(existing_pv);
		pp->extent_size = pv_pe_size(existing_pv);
		pp->extent_count = pv_pe_count(existing_pv);
	}

	if (arg_count(cmd, yes_ARG) && !arg_count(cmd, force_ARG)) {
		log_error("Option y can only be given with option f");
		return 0;
	}

	pp->yes = arg_count(cmd, yes_ARG);
	pp->force = arg_count(cmd, force_ARG);

	if (arg_int_value(cmd, labelsector_ARG, 0) >= LABEL_SCAN_SECTORS) {
		log_error("labelsector must be less than %lu",
			  LABEL_SCAN_SECTORS);
		return 0;
	} else {
		pp->labelsector = arg_int64_value(cmd, labelsector_ARG,
						  DEFAULT_LABELSECTOR);
	}

	if (!(cmd->fmt->features & FMT_MDAS) &&
	    (arg_count(cmd, metadatacopies_ARG) ||
	     arg_count(cmd, metadatasize_ARG))) {
		log_error("Metadata parameters only apply to text format");
		return 0;
	}

	if (arg_count(cmd, metadatacopies_ARG) &&
	    arg_int_value(cmd, metadatacopies_ARG, -1) > 2) {
		log_error("Metadatacopies may only be 0, 1 or 2");
		return 0;
	}

	if (arg_count(cmd, zero_ARG))
		pp->zero = strcmp(arg_str_value(cmd, zero_ARG, "y"), "n");
	else if (arg_count(cmd, restorefile_ARG) || arg_count(cmd, uuidstr_ARG))
		pp->zero = 0;
	else
		pp->zero = 1;

	if (arg_sign_value(cmd, physicalvolumesize_ARG, 0) == SIGN_MINUS) {
		log_error("Physical volume size may not be negative");
		return 0;
	}
	pp->size = arg_uint64_value(cmd, physicalvolumesize_ARG, UINT64_C(0));

	if (arg_sign_value(cmd, metadatasize_ARG, 0) == SIGN_MINUS) {
		log_error("Metadata size may not be negative");
		return 0;
	}

	pp->pvmetadatasize = arg_uint64_value(cmd, metadatasize_ARG, UINT64_C(0));
	if (!pp->pvmetadatasize)
		pp->pvmetadatasize = find_config_tree_int(cmd,
						 "metadata/pvmetadatasize",
						 DEFAULT_PVMETADATASIZE);

	pp->pvmetadatacopies = arg_int_value(cmd, metadatacopies_ARG, -1);
	if (pp->pvmetadatacopies < 0)
		pp->pvmetadatacopies = find_config_tree_int(cmd,
						   "metadata/pvmetadatacopies",
						   DEFAULT_PVMETADATACOPIES);

	return 1;
}


int pvcreate(struct cmd_context *cmd, int argc, char **argv)
{
	int i;
	int ret = ECMD_PROCESSED;
	struct pvcreate_params pp;

	if (!pvcreate_validate_params(cmd, argc, argv, &pp)) {
		return EINVALID_CMD_LINE;
	}

	for (i = 0; i < argc; i++) {
		if (!pvcreate_single(cmd, argv[i], &pp))
			ret = ECMD_FAILED;

		if (sigint_caught())
			return ret;
	}

	return ret;
}
