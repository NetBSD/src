/*	$NetBSD: lvresize.c,v 1.1.1.3 2009/12/02 00:25:53 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#define SIZE_BUF 128

struct lvresize_params {
	const char *vg_name;
	const char *lv_name;

	uint32_t stripes;
	uint32_t stripe_size;
	uint32_t mirrors;

	const struct segment_type *segtype;

	/* size */
	uint32_t extents;
	uint64_t size;
	sign_t sign;
	percent_t percent;

	enum {
		LV_ANY = 0,
		LV_REDUCE = 1,
		LV_EXTEND = 2
	} resize;

	int resizefs;
	int nofsck;

	int argc;
	char **argv;
};

static int _validate_stripesize(struct cmd_context *cmd,
				const struct volume_group *vg,
				struct lvresize_params *lp)
{
	if (arg_sign_value(cmd, stripesize_ARG, 0) == SIGN_MINUS) {
		log_error("Stripesize may not be negative.");
		return 0;
	}

	if (arg_uint_value(cmd, stripesize_ARG, 0) > STRIPE_SIZE_LIMIT * 2) {
		log_error("Stripe size cannot be larger than %s",
			  display_size(cmd, (uint64_t) STRIPE_SIZE_LIMIT));
		return 0;
	}

	if (!(vg->fid->fmt->features & FMT_SEGMENTS))
		log_warn("Varied stripesize not supported. Ignoring.");
	else if (arg_uint_value(cmd, stripesize_ARG, 0) > vg->extent_size * 2) {
		log_error("Reducing stripe size %s to maximum, "
			  "physical extent size %s",
			  display_size(cmd,
				       (uint64_t) arg_uint_value(cmd, stripesize_ARG, 0)),
			  display_size(cmd, (uint64_t) vg->extent_size));
		lp->stripe_size = vg->extent_size;
	} else
		lp->stripe_size = arg_uint_value(cmd, stripesize_ARG, 0);

	if (lp->mirrors) {
		log_error("Mirrors and striping cannot be combined yet.");
		return 0;
	}
	if (lp->stripe_size & (lp->stripe_size - 1)) {
		log_error("Stripe size must be power of 2");
		return 0;
	}

	return 1;
}

static int _request_confirmation(struct cmd_context *cmd,
				 const struct volume_group *vg,
				 const struct logical_volume *lv,
				 const struct lvresize_params *lp)
{
	struct lvinfo info;

	memset(&info, 0, sizeof(info));

	if (!lv_info(cmd, lv, &info, 1, 0) && driver_version(NULL, 0)) {
		log_error("lv_info failed: aborting");
		return 0;
	}

	if (lp->resizefs) {
		if (!info.exists) {
			log_error("Logical volume %s must be activated "
				  "before resizing filesystem", lp->lv_name);
			return 0;
		}
		return 1;
	}

	if (!info.exists)
		return 1;

	log_warn("WARNING: Reducing active%s logical volume to %s",
		 info.open_count ? " and open" : "",
		 display_size(cmd, (uint64_t) lp->extents * vg->extent_size));

	log_warn("THIS MAY DESTROY YOUR DATA (filesystem etc.)");

	if (!arg_count(cmd, force_ARG)) {
		if (yes_no_prompt("Do you really want to reduce %s? [y/n]: ",
				  lp->lv_name) == 'n') {
			log_print("Logical volume %s NOT reduced", lp->lv_name);
			return 0;
		}
		if (sigint_caught())
			return 0;
	}

	return 1;
}

enum fsadm_cmd_e { FSADM_CMD_CHECK, FSADM_CMD_RESIZE };
#define FSADM_CMD "fsadm"
#define FSADM_CMD_MAX_ARGS 6

/*
 * FSADM_CMD --dry-run --verbose --force check lv_path
 * FSADM_CMD --dry-run --verbose --force resize lv_path size
 */
static int _fsadm_cmd(struct cmd_context *cmd,
		      const struct volume_group *vg,
		      const struct lvresize_params *lp,
		      enum fsadm_cmd_e fcmd)
{
	char lv_path[PATH_MAX];
	char size_buf[SIZE_BUF];
	const char *argv[FSADM_CMD_MAX_ARGS + 2];
	unsigned i = 0;

	argv[i++] = FSADM_CMD;

	if (test_mode())
		argv[i++] = "--dry-run";

	if (verbose_level() >= _LOG_NOTICE)
		argv[i++] = "--verbose";

	if (arg_count(cmd, force_ARG))
		argv[i++] = "--force";

	argv[i++] = (fcmd == FSADM_CMD_RESIZE) ? "resize" : "check";

	if (dm_snprintf(lv_path, PATH_MAX, "%s%s/%s", cmd->dev_dir, lp->vg_name,
			lp->lv_name) < 0) {
		log_error("Couldn't create LV path for %s", lp->lv_name);
		return 0;
	}

	argv[i++] = lv_path;

	if (fcmd == FSADM_CMD_RESIZE) {
		if (dm_snprintf(size_buf, SIZE_BUF, "%" PRIu64 "K",
				(uint64_t) lp->extents * vg->extent_size / 2) < 0) {
			log_error("Couldn't generate new LV size string");
			return 0;
		}

		argv[i++] = size_buf;
	}

	argv[i] = NULL;

	return exec_cmd(cmd, argv);
}

static int _lvresize_params(struct cmd_context *cmd, int argc, char **argv,
			    struct lvresize_params *lp)
{
	const char *cmd_name;
	char *st;
	unsigned dev_dir_found = 0;

	lp->sign = SIGN_NONE;
	lp->resize = LV_ANY;

	cmd_name = command_name(cmd);
	if (!strcmp(cmd_name, "lvreduce"))
		lp->resize = LV_REDUCE;
	if (!strcmp(cmd_name, "lvextend"))
		lp->resize = LV_EXTEND;

	/*
	 * Allow omission of extents and size if the user has given us
	 * one or more PVs.  Most likely, the intent was "resize this
	 * LV the best you can with these PVs"
	 */
	if ((arg_count(cmd, extents_ARG) + arg_count(cmd, size_ARG) == 0) &&
	    (argc >= 2)) {
		lp->extents = 100;
		lp->percent = PERCENT_PVS;
		lp->sign = SIGN_PLUS;
	} else if ((arg_count(cmd, extents_ARG) +
		    arg_count(cmd, size_ARG) != 1)) {
		log_error("Please specify either size or extents but not "
			  "both.");
		return 0;
	}

	if (arg_count(cmd, extents_ARG)) {
		lp->extents = arg_uint_value(cmd, extents_ARG, 0);
		lp->sign = arg_sign_value(cmd, extents_ARG, SIGN_NONE);
		lp->percent = arg_percent_value(cmd, extents_ARG, PERCENT_NONE);
	}

	/* Size returned in kilobyte units; held in sectors */
	if (arg_count(cmd, size_ARG)) {
		lp->size = arg_uint64_value(cmd, size_ARG, UINT64_C(0));
		lp->sign = arg_sign_value(cmd, size_ARG, SIGN_NONE);
		lp->percent = PERCENT_NONE;
	}

	if (lp->resize == LV_EXTEND && lp->sign == SIGN_MINUS) {
		log_error("Negative argument not permitted - use lvreduce");
		return 0;
	}

	if (lp->resize == LV_REDUCE && lp->sign == SIGN_PLUS) {
		log_error("Positive sign not permitted - use lvextend");
		return 0;
	}

	lp->resizefs = arg_is_set(cmd, resizefs_ARG);
	lp->nofsck = arg_is_set(cmd, nofsck_ARG);

	if (!argc) {
		log_error("Please provide the logical volume name");
		return 0;
	}

	lp->lv_name = argv[0];
	argv++;
	argc--;

	if (!(lp->lv_name = skip_dev_dir(cmd, lp->lv_name, &dev_dir_found)) ||
	    !(lp->vg_name = extract_vgname(cmd, lp->lv_name))) {
		log_error("Please provide a volume group name");
		return 0;
	}

	if (!validate_name(lp->vg_name)) {
		log_error("Volume group name %s has invalid characters",
			  lp->vg_name);
		return 0;
	}

	if ((st = strrchr(lp->lv_name, '/')))
		lp->lv_name = st + 1;

	lp->argc = argc;
	lp->argv = argv;

	return 1;
}

static int _lvresize(struct cmd_context *cmd, struct volume_group *vg,
		     struct lvresize_params *lp)
{
	struct logical_volume *lv;
	struct lvinfo info;
	uint32_t stripesize_extents = 0;
	uint32_t seg_stripes = 0, seg_stripesize = 0, seg_size = 0;
	uint32_t seg_mirrors = 0;
	uint32_t extents_used = 0;
	uint32_t size_rest;
	uint32_t pv_extent_count = 0;
	alloc_policy_t alloc;
	struct logical_volume *lock_lv;
	struct lv_list *lvl;
	struct lv_segment *seg;
	uint32_t seg_extents;
	uint32_t sz, str;
	struct dm_list *pvh = NULL;

	/* does LV exist? */
	if (!(lvl = find_lv_in_vg(vg, lp->lv_name))) {
		log_error("Logical volume %s not found in volume group %s",
			  lp->lv_name, lp->vg_name);
		return ECMD_FAILED;
	}

	if (arg_count(cmd, stripes_ARG)) {
		if (vg->fid->fmt->features & FMT_SEGMENTS)
			lp->stripes = arg_uint_value(cmd, stripes_ARG, 1);
		else
			log_warn("Varied striping not supported. Ignoring.");
	}

	if (arg_count(cmd, mirrors_ARG)) {
		if (vg->fid->fmt->features & FMT_SEGMENTS)
			lp->mirrors = arg_uint_value(cmd, mirrors_ARG, 1) + 1;
		else
			log_warn("Mirrors not supported. Ignoring.");
		if (arg_sign_value(cmd, mirrors_ARG, 0) == SIGN_MINUS) {
			log_error("Mirrors argument may not be negative");
			return EINVALID_CMD_LINE;
		}
	}

	if (arg_count(cmd, stripesize_ARG) &&
	    !_validate_stripesize(cmd, vg, lp))
		return EINVALID_CMD_LINE;

	lv = lvl->lv;

	if (lv->status & LOCKED) {
		log_error("Can't resize locked LV %s", lv->name);
		return ECMD_FAILED;
	}

	if (lv->status & CONVERTING) {
		log_error("Can't resize %s while lvconvert in progress", lv->name);
		return ECMD_FAILED;
	}

	alloc = arg_uint_value(cmd, alloc_ARG, lv->alloc);

	if (lp->size) {
		if (lp->size % vg->extent_size) {
			if (lp->sign == SIGN_MINUS)
				lp->size -= lp->size % vg->extent_size;
			else
				lp->size += vg->extent_size -
				    (lp->size % vg->extent_size);

			log_print("Rounding up size to full physical extent %s",
				  display_size(cmd, (uint64_t) lp->size));
		}

		lp->extents = lp->size / vg->extent_size;
	}

	if (!(pvh = lp->argc ? create_pv_list(cmd->mem, vg, lp->argc,
						     lp->argv, 1) : &vg->pvs)) {
		stack;
		return ECMD_FAILED;
	}

	switch(lp->percent) {
		case PERCENT_VG:
			lp->extents = lp->extents * vg->extent_count / 100;
			break;
		case PERCENT_FREE:
			lp->extents = lp->extents * vg->free_count / 100;
			break;
		case PERCENT_LV:
			lp->extents = lp->extents * lv->le_count / 100;
			break;
		case PERCENT_PVS:
			if (lp->argc) {
				pv_extent_count = pv_list_extents_free(pvh);
				lp->extents = lp->extents * pv_extent_count / 100;
			} else
				lp->extents = lp->extents * vg->extent_count / 100;
			break;
		case PERCENT_NONE:
			break;
	}

	if (lp->sign == SIGN_PLUS)
		lp->extents += lv->le_count;

	if (lp->sign == SIGN_MINUS) {
		if (lp->extents >= lv->le_count) {
			log_error("Unable to reduce %s below 1 extent",
				  lp->lv_name);
			return EINVALID_CMD_LINE;
		}

		lp->extents = lv->le_count - lp->extents;
	}

	if (!lp->extents) {
		log_error("New size of 0 not permitted");
		return EINVALID_CMD_LINE;
	}

	if (lp->extents == lv->le_count) {
		if (!lp->resizefs) {
			log_error("New size (%d extents) matches existing size "
				  "(%d extents)", lp->extents, lv->le_count);
			return EINVALID_CMD_LINE;
		}
		lp->resize = LV_EXTEND; /* lets pretend zero size extension */
	}

	seg_size = lp->extents - lv->le_count;

	/* Use segment type of last segment */
	dm_list_iterate_items(seg, &lv->segments) {
		lp->segtype = seg->segtype;
	}

	/* FIXME Support LVs with mixed segment types */
	if (lp->segtype != arg_ptr_value(cmd, type_ARG, lp->segtype)) {
		log_error("VolumeType does not match (%s)", lp->segtype->name);
		return EINVALID_CMD_LINE;
	}

	/* If extending, find stripes, stripesize & size of last segment */
	if ((lp->extents > lv->le_count) &&
	    !(lp->stripes == 1 || (lp->stripes > 1 && lp->stripe_size))) {
		dm_list_iterate_items(seg, &lv->segments) {
			if (!seg_is_striped(seg))
				continue;

			sz = seg->stripe_size;
			str = seg->area_count;

			if ((seg_stripesize && seg_stripesize != sz &&
			     !lp->stripe_size) ||
			    (seg_stripes && seg_stripes != str && !lp->stripes)) {
				log_error("Please specify number of "
					  "stripes (-i) and stripesize (-I)");
				return EINVALID_CMD_LINE;
			}

			seg_stripesize = sz;
			seg_stripes = str;
		}

		if (!lp->stripes)
			lp->stripes = seg_stripes;

		if (!lp->stripe_size && lp->stripes > 1) {
			if (seg_stripesize) {
				log_print("Using stripesize of last segment %s",
					  display_size(cmd, (uint64_t) seg_stripesize));
				lp->stripe_size = seg_stripesize;
			} else {
				lp->stripe_size =
					find_config_tree_int(cmd,
							"metadata/stripesize",
							DEFAULT_STRIPESIZE) * 2;
				log_print("Using default stripesize %s",
					  display_size(cmd, (uint64_t) lp->stripe_size));
			}
		}
	}

	/* If extending, find mirrors of last segment */
	if ((lp->extents > lv->le_count)) {
		dm_list_iterate_back_items(seg, &lv->segments) {
			if (seg_is_mirrored(seg))
				seg_mirrors = lv_mirror_count(seg->lv);
			else
				seg_mirrors = 0;
			break;
		}
		if (!arg_count(cmd, mirrors_ARG) && seg_mirrors) {
			log_print("Extending %" PRIu32 " mirror images.",
				  seg_mirrors);
			lp->mirrors = seg_mirrors;
		}
		if ((arg_count(cmd, mirrors_ARG) || seg_mirrors) &&
		    (lp->mirrors != seg_mirrors)) {
			log_error("Cannot vary number of mirrors in LV yet.");
			return EINVALID_CMD_LINE;
		}
	}

	/* If reducing, find stripes, stripesize & size of last segment */
	if (lp->extents < lv->le_count) {
		extents_used = 0;

		if (lp->stripes || lp->stripe_size || lp->mirrors)
			log_error("Ignoring stripes, stripesize and mirrors "
				  "arguments when reducing");

		dm_list_iterate_items(seg, &lv->segments) {
			seg_extents = seg->len;

			if (seg_is_striped(seg)) {
				seg_stripesize = seg->stripe_size;
				seg_stripes = seg->area_count;
			}

			if (seg_is_mirrored(seg))
				seg_mirrors = lv_mirror_count(seg->lv);
			else
				seg_mirrors = 0;

			if (lp->extents <= extents_used + seg_extents)
				break;

			extents_used += seg_extents;
		}

		seg_size = lp->extents - extents_used;
		lp->stripe_size = seg_stripesize;
		lp->stripes = seg_stripes;
		lp->mirrors = seg_mirrors;
	}

	if (lp->stripes > 1 && !lp->stripe_size) {
		log_error("Stripesize for striped segment should not be 0!");
		return EINVALID_CMD_LINE;
	}

	if ((lp->stripes > 1)) {
		if (!(stripesize_extents = lp->stripe_size / vg->extent_size))
			stripesize_extents = 1;

		if ((size_rest = seg_size % (lp->stripes * stripesize_extents))) {
			log_print("Rounding size (%d extents) down to stripe "
				  "boundary size for segment (%d extents)",
				  lp->extents, lp->extents - size_rest);
			lp->extents = lp->extents - size_rest;
		}

		if (lp->stripe_size < STRIPE_SIZE_MIN) {
			log_error("Invalid stripe size %s",
				  display_size(cmd, (uint64_t) lp->stripe_size));
			return EINVALID_CMD_LINE;
		}
	}

	if (lp->extents < lv->le_count) {
		if (lp->resize == LV_EXTEND) {
			log_error("New size given (%d extents) not larger "
				  "than existing size (%d extents)",
				  lp->extents, lv->le_count);
			return EINVALID_CMD_LINE;
		}
		lp->resize = LV_REDUCE;
	} else if (lp->extents > lv->le_count) {
		if (lp->resize == LV_REDUCE) {
			log_error("New size given (%d extents) not less than "
				  "existing size (%d extents)", lp->extents,
				  lv->le_count);
			return EINVALID_CMD_LINE;
		}
		lp->resize = LV_EXTEND;
	}

	if (lv_is_origin(lv)) {
		if (lp->resize == LV_REDUCE) {
			log_error("Snapshot origin volumes cannot be reduced "
				  "in size yet.");
			return ECMD_FAILED;
		}

		memset(&info, 0, sizeof(info));

		if (lv_info(cmd, lv, &info, 0, 0) && info.exists) {
			log_error("Snapshot origin volumes can be resized "
				  "only while inactive: try lvchange -an");
			return ECMD_FAILED;
		}
	}

	if ((lp->resize == LV_REDUCE) && lp->argc)
		log_warn("Ignoring PVs on command line when reducing");

	/* Request confirmation before operations that are often mistakes. */
	if ((lp->resizefs || (lp->resize == LV_REDUCE)) &&
	    !_request_confirmation(cmd, vg, lv, lp)) {
		stack;
		return ECMD_FAILED;
	}

	if (lp->resizefs) {
		if (!lp->nofsck &&
		    !_fsadm_cmd(cmd, vg, lp, FSADM_CMD_CHECK)) {
			stack;
			return ECMD_FAILED;
		}

		if ((lp->resize == LV_REDUCE) &&
		    !_fsadm_cmd(cmd, vg, lp, FSADM_CMD_RESIZE)) {
			stack;
			return ECMD_FAILED;
		}
	}

	if (!archive(vg)) {
		stack;
		return ECMD_FAILED;
	}

	log_print("%sing logical volume %s to %s",
		  (lp->resize == LV_REDUCE) ? "Reduc" : "Extend",
		  lp->lv_name,
		  display_size(cmd, (uint64_t) lp->extents * vg->extent_size));

	if (lp->resize == LV_REDUCE) {
		if (!lv_reduce(lv, lv->le_count - lp->extents)) {
			stack;
			return ECMD_FAILED;
		}
	} else if ((lp->extents > lv->le_count) && /* Ensure we extend */
		   !lv_extend(lv, lp->segtype, lp->stripes,
			      lp->stripe_size, lp->mirrors,
			      lp->extents - lv->le_count,
			      NULL, 0u, 0u, pvh, alloc)) {
		stack;
		return ECMD_FAILED;
	}

	/* store vg on disk(s) */
	if (!vg_write(vg)) {
		stack;
		return ECMD_FAILED;
	}

	/* If snapshot, must suspend all associated devices */
	if (lv_is_cow(lv))
		lock_lv = origin_from_cow(lv);
	else
		lock_lv = lv;

	if (!suspend_lv(cmd, lock_lv)) {
		log_error("Failed to suspend %s", lp->lv_name);
		vg_revert(vg);
		backup(vg);
		return ECMD_FAILED;
	}

	if (!vg_commit(vg)) {
		stack;
		resume_lv(cmd, lock_lv);
		backup(vg);
		return ECMD_FAILED;
	}

	if (!resume_lv(cmd, lock_lv)) {
		log_error("Problem reactivating %s", lp->lv_name);
		backup(vg);
		return ECMD_FAILED;
	}

	backup(vg);

	log_print("Logical volume %s successfully resized", lp->lv_name);

	if (lp->resizefs && (lp->resize == LV_EXTEND) &&
	    !_fsadm_cmd(cmd, vg, lp, FSADM_CMD_RESIZE)) {
		stack;
		return ECMD_FAILED;
	}

	return ECMD_PROCESSED;
}

int lvresize(struct cmd_context *cmd, int argc, char **argv)
{
	struct lvresize_params lp;
	struct volume_group *vg;
	int r;

	memset(&lp, 0, sizeof(lp));

	if (!_lvresize_params(cmd, argc, argv, &lp))
		return EINVALID_CMD_LINE;

	log_verbose("Finding volume group %s", lp.vg_name);
	vg = vg_read_for_update(cmd, lp.vg_name, NULL, 0);
	if (vg_read_error(vg)) {
		vg_release(vg);
		stack;
		return ECMD_FAILED;
	}

	if (!(r = _lvresize(cmd, vg, &lp)))
		stack;

	unlock_and_release_vg(cmd, vg, lp.vg_name);

	return r;
}
