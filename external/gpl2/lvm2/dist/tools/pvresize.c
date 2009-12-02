/*	$NetBSD: pvresize.c,v 1.1.1.2 2009/12/02 00:25:54 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
 * Copyright (C) 2005 Zak Kipling. All rights reserved.
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

struct pvresize_params {
	uint64_t new_size;

	unsigned done;
	unsigned total;
};

static int _pv_resize_single(struct cmd_context *cmd,
			     struct volume_group *vg,
			     struct physical_volume *pv,
			     const uint64_t new_size)
{
	struct pv_list *pvl;
	uint64_t size = 0;
	uint32_t new_pe_count = 0;
	int r = 0;
	struct dm_list mdas;
	const char *pv_name = pv_dev_name(pv);
	const char *vg_name;
	struct lvmcache_info *info;
	int mda_count = 0;
	struct volume_group *old_vg = vg;

	dm_list_init(&mdas);

	if (is_orphan_vg(pv_vg_name(pv))) {
		vg_name = VG_ORPHANS;
		if (!lock_vol(cmd, vg_name, LCK_VG_WRITE)) {
			log_error("Can't get lock for orphans");
			return 0;
		}

		if (!(pv = pv_read(cmd, pv_name, &mdas, NULL, 1, 0))) {
			unlock_vg(cmd, vg_name);
			log_error("Unable to read PV \"%s\"", pv_name);
			return 0;
		}

		mda_count = dm_list_size(&mdas);
	} else {
		vg_name = pv_vg_name(pv);

		vg = vg_read_for_update(cmd, vg_name, NULL, 0);

		if (vg_read_error(vg))
			goto bad;

		if (!(pvl = find_pv_in_vg(vg, pv_name))) {
			log_error("Unable to find \"%s\" in volume group \"%s\"",
				  pv_name, vg->name);
			goto bad;
		}

		pv = pvl->pv;

		if (!(info = info_from_pvid(pv->dev->pvid, 0))) {
			log_error("Can't get info for PV %s in volume group %s",
				  pv_name, vg->name);
			goto bad;
		}

		mda_count = dm_list_size(&info->mdas);

		if (!archive(vg))
			goto bad;
	}

	/* FIXME Create function to test compatibility properly */
	if (mda_count > 1) {
		log_error("%s: too many metadata areas for pvresize", pv_name);
		goto bad;
	}

	if (!(pv->fmt->features & FMT_RESIZE_PV)) {
		log_error("Physical volume %s format does not support resizing.",
			  pv_name);
		goto bad;
	}

	/* Get new size */
	if (!dev_get_size(pv_dev(pv), &size)) {
		log_error("%s: Couldn't get size.", pv_name);
		goto bad;
	}

	if (new_size) {
		if (new_size > size)
			log_warn("WARNING: %s: Overriding real size. "
				  "You could lose data.", pv_name);
		log_verbose("%s: Pretending size is %" PRIu64 " not %" PRIu64
			    " sectors.", pv_name, new_size, pv_size(pv));
		size = new_size;
	}

	if (size < PV_MIN_SIZE) {
		log_error("%s: Size must exceed minimum of %ld sectors.",
			  pv_name, PV_MIN_SIZE);
		goto bad;
	}

	if (size < pv_pe_start(pv)) {
		log_error("%s: Size must exceed physical extent start of "
			  "%" PRIu64 " sectors.", pv_name, pv_pe_start(pv));
		goto bad;
	}

	pv->size = size;

	if (vg) {
		pv->size -= pv_pe_start(pv);
		new_pe_count = pv_size(pv) / vg->extent_size;

 		if (!new_pe_count) {
			log_error("%s: Size must leave space for at "
				  "least one physical extent of "
				  "%" PRIu32 " sectors.", pv_name,
				  pv_pe_size(pv));
			goto bad;
		}

		if (!pv_resize(pv, vg, new_pe_count))
			goto_bad;
	}

	log_verbose("Resizing volume \"%s\" to %" PRIu64 " sectors.",
		    pv_name, pv_size(pv));

	log_verbose("Updating physical volume \"%s\"", pv_name);
	if (!is_orphan_vg(pv_vg_name(pv))) {
		if (!vg_write(vg) || !vg_commit(vg)) {
			log_error("Failed to store physical volume \"%s\" in "
				  "volume group \"%s\"", pv_name, vg->name);
			goto bad;
		}
		backup(vg);
	} else if (!(pv_write(cmd, pv, NULL, INT64_C(-1)))) {
		log_error("Failed to store physical volume \"%s\"",
			  pv_name);
		goto bad;;
	}

	log_print("Physical volume \"%s\" changed", pv_name);
	r = 1;

bad:
	unlock_vg(cmd, vg_name);
	if (!old_vg)
		vg_release(vg);
	return r;
}

static int _pvresize_single(struct cmd_context *cmd,
			    struct volume_group *vg,
			    struct physical_volume *pv,
			    void *handle)
{
	struct pvresize_params *params = (struct pvresize_params *) handle;

	params->total++;

	if (!_pv_resize_single(cmd, vg, pv, params->new_size)) {
		stack;
		return ECMD_FAILED;
	}
	
	params->done++;

	return ECMD_PROCESSED;
}

int pvresize(struct cmd_context *cmd, int argc, char **argv)
{
	struct pvresize_params params;
	int ret;

	if (!argc) {
		log_error("Please supply physical volume(s)");
		return EINVALID_CMD_LINE;
	}

	if (arg_sign_value(cmd, physicalvolumesize_ARG, 0) == SIGN_MINUS) {
		log_error("Physical volume size may not be negative");
		return 0;
	}

	params.new_size = arg_uint64_value(cmd, physicalvolumesize_ARG,
					   UINT64_C(0));

	params.done = 0;
	params.total = 0;

	ret = process_each_pv(cmd, argc, argv, NULL, READ_FOR_UPDATE, 0, &params,
			      _pvresize_single);

	log_print("%d physical volume(s) resized / %d physical volume(s) "
		  "not resized", params.done, params.total - params.done);

	return ret;
}
