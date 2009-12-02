/*	$NetBSD: lvcreate.c,v 1.1.1.2 2009/12/02 00:25:50 haad Exp $	*/

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
#include "lv_alloc.h"

#include <fcntl.h>

struct lvcreate_cmdline_params {
	percent_t percent;
	uint64_t size;
	char **pvs;
	int pv_count;
};

static int _lvcreate_name_params(struct lvcreate_params *lp,
				 struct cmd_context *cmd,
				 int *pargc, char ***pargv)
{
	int argc = *pargc;
	char **argv = *pargv, *ptr;
	char *vg_name;

	lp->lv_name = arg_str_value(cmd, name_ARG, NULL);

	if (lp->snapshot && !arg_count(cmd, virtualsize_ARG)) {
		if (!argc) {
			log_error("Please specify a logical volume to act as "
				  "the snapshot origin.");
			return 0;
		}

		lp->origin = argv[0];
		(*pargv)++, (*pargc)--;
		if (!(lp->vg_name = extract_vgname(cmd, lp->origin))) {
			log_error("The origin name should include the "
				  "volume group.");
			return 0;
		}

		/* Strip the volume group from the origin */
		if ((ptr = strrchr(lp->origin, (int) '/')))
			lp->origin = ptr + 1;

	} else {
		/*
		 * If VG not on command line, try -n arg and then
		 * environment.
		 */
		if (!argc) {
			if (!(lp->vg_name = extract_vgname(cmd, lp->lv_name))) {
				log_error("Please provide a volume group name");
				return 0;
			}

		} else {
			vg_name = skip_dev_dir(cmd, argv[0], NULL);
			if (strrchr(vg_name, '/')) {
				log_error("Volume group name expected "
					  "(no slash)");
				return 0;
			}

			/*
			 * Ensure lv_name doesn't contain a
			 * different VG.
			 */
			if (lp->lv_name && strchr(lp->lv_name, '/')) {
				if (!(lp->vg_name =
				      extract_vgname(cmd, lp->lv_name)))
					return 0;

				if (strcmp(lp->vg_name, vg_name)) {
					log_error("Inconsistent volume group "
						  "names "
						  "given: \"%s\" and \"%s\"",
						  lp->vg_name, vg_name);
					return 0;
				}
			}

			lp->vg_name = vg_name;
			(*pargv)++, (*pargc)--;
		}
	}

	if (!validate_name(lp->vg_name)) {
		log_error("Volume group name %s has invalid characters",
			  lp->vg_name);
		return 0;
	}

	if (lp->lv_name) {
		if ((ptr = strrchr(lp->lv_name, '/')))
			lp->lv_name = ptr + 1;

		if (!apply_lvname_restrictions(lp->lv_name))
			return_0;

		if (!validate_name(lp->lv_name)) {
			log_error("Logical volume name \"%s\" is invalid",
				  lp->lv_name);
			return 0;
		}
	}

	return 1;
}

/*
 * Update extents parameters based on other parameters which affect the size
 * calcuation.
 * NOTE: We must do this here because of the percent_t typedef and because we
 * need the vg.
 */
static int _update_extents_params(struct volume_group *vg,
				  struct lvcreate_params *lp,
				  struct lvcreate_cmdline_params *lcp)
{
	uint32_t pv_extent_count;

	if (lcp->size &&
	    !(lp->extents = extents_from_size(vg->cmd, lcp->size,
					       vg->extent_size)))
		return_0;

	if (lp->voriginsize &&
	    !(lp->voriginextents = extents_from_size(vg->cmd, lp->voriginsize,
						      vg->extent_size)))
		return_0;

	/*
	 * Create the pv list before we parse lcp->percent - might be
	 * PERCENT_PVSs
	 */
	if (lcp->pv_count) {
		if (!(lp->pvh = create_pv_list(vg->cmd->mem, vg,
					   lcp->pv_count, lcp->pvs, 1)))
			return_0;
	} else
		lp->pvh = &vg->pvs;

	switch(lcp->percent) {
		case PERCENT_VG:
			lp->extents = lp->extents * vg->extent_count / 100;
			break;
		case PERCENT_FREE:
			lp->extents = lp->extents * vg->free_count / 100;
			break;
		case PERCENT_PVS:
			if (!lcp->pv_count)
				lp->extents = lp->extents * vg->extent_count / 100;
			else {
				pv_extent_count = pv_list_extents_free(lp->pvh);
				lp->extents = lp->extents * pv_extent_count / 100;
			}
			break;
		case PERCENT_LV:
			log_error("Please express size as %%VG, %%PVS, or "
				  "%%FREE.");
			return 0;
		case PERCENT_NONE:
			break;
	}
	return 1;
}

static int _read_size_params(struct lvcreate_params *lp,
			     struct lvcreate_cmdline_params *lcp,
			     struct cmd_context *cmd)
{
	if (arg_count(cmd, extents_ARG) + arg_count(cmd, size_ARG) != 1) {
		log_error("Please specify either size or extents (not both)");
		return 0;
	}

	if (arg_count(cmd, extents_ARG)) {
		if (arg_sign_value(cmd, extents_ARG, 0) == SIGN_MINUS) {
			log_error("Negative number of extents is invalid");
			return 0;
		}
		lp->extents = arg_uint_value(cmd, extents_ARG, 0);
		lcp->percent = arg_percent_value(cmd, extents_ARG, PERCENT_NONE);
	}

	/* Size returned in kilobyte units; held in sectors */
	if (arg_count(cmd, size_ARG)) {
		if (arg_sign_value(cmd, size_ARG, 0) == SIGN_MINUS) {
			log_error("Negative size is invalid");
			return 0;
		}
		lcp->size = arg_uint64_value(cmd, size_ARG, UINT64_C(0));
		lcp->percent = PERCENT_NONE;
	}

	/* Size returned in kilobyte units; held in sectors */
	if (arg_count(cmd, virtualsize_ARG)) {
		if (arg_sign_value(cmd, virtualsize_ARG, 0) == SIGN_MINUS) {
			log_error("Negative virtual origin size is invalid");
			return 0;
		}
		lp->voriginsize = arg_uint64_value(cmd, virtualsize_ARG,
						   UINT64_C(0));
		if (!lp->voriginsize) {
			log_error("Virtual origin size may not be zero");
			return 0;
		}
	}

	return 1;
}

/*
 * Generic stripe parameter checks.
 * FIXME: Should eventually be moved into lvm library.
 */
static int _validate_stripe_params(struct cmd_context *cmd,
				   struct lvcreate_params *lp)
{
	if (lp->stripes == 1 && lp->stripe_size) {
		log_print("Ignoring stripesize argument with single stripe");
		lp->stripe_size = 0;
	}

	if (lp->stripes > 1 && !lp->stripe_size) {
		lp->stripe_size = find_config_tree_int(cmd,
						  "metadata/stripesize",
						  DEFAULT_STRIPESIZE) * 2;
		log_print("Using default stripesize %s",
			  display_size(cmd, (uint64_t) lp->stripe_size));
	}

	if (lp->stripes < 1 || lp->stripes > MAX_STRIPES) {
		log_error("Number of stripes (%d) must be between %d and %d",
			  lp->stripes, 1, MAX_STRIPES);
		return 0;
	}

	/* MAX size check is in _lvcreate */
	if (lp->stripes > 1 && (lp->stripe_size < STRIPE_SIZE_MIN ||
				lp->stripe_size & (lp->stripe_size - 1))) {
		log_error("Invalid stripe size %s",
			  display_size(cmd, (uint64_t) lp->stripe_size));
		return 0;
	}

	return 1;
}

/* The stripe size is limited by the size of a uint32_t, but since the
 * value given by the user is doubled, and the final result must be a
 * power of 2, we must divide UINT_MAX by four and add 1 (to round it
 * up to the power of 2) */
static int _read_stripe_params(struct lvcreate_params *lp,
			       struct cmd_context *cmd)
{
	if (arg_count(cmd, stripesize_ARG)) {
		if (arg_sign_value(cmd, stripesize_ARG, 0) == SIGN_MINUS) {
			log_error("Negative stripesize is invalid");
			return 0;
		}
		/* Check to make sure we won't overflow lp->stripe_size */
		if(arg_uint_value(cmd, stripesize_ARG, 0) > STRIPE_SIZE_LIMIT * 2) {
			log_error("Stripe size cannot be larger than %s",
				  display_size(cmd, (uint64_t) STRIPE_SIZE_LIMIT));
			return 0;
		}
		lp->stripe_size = arg_uint_value(cmd, stripesize_ARG, 0);
	}


	if (!_validate_stripe_params(cmd, lp))
		return 0;

	return 1;
}

/*
 * Generic mirror parameter checks.
 * FIXME: Should eventually be moved into lvm library.
 */
static int _validate_mirror_params(const struct cmd_context *cmd __attribute((unused)),
				   const struct lvcreate_params *lp)
{
	int pagesize = lvm_getpagesize();

	if (lp->region_size & (lp->region_size - 1)) {
		log_error("Region size (%" PRIu32 ") must be a power of 2",
			  lp->region_size);
		return 0;
	}

	if (lp->region_size % (pagesize >> SECTOR_SHIFT)) {
		log_error("Region size (%" PRIu32 ") must be a multiple of "
			  "machine memory page size (%d)",
			  lp->region_size, pagesize >> SECTOR_SHIFT);
		return 0;
	}

	if (!lp->region_size) {
		log_error("Non-zero region size must be supplied.");
		return 0;
	}

	return 1;
}

static int _read_mirror_params(struct lvcreate_params *lp,
			       struct cmd_context *cmd)
{
	int region_size;
	const char *mirrorlog;

	if (arg_count(cmd, corelog_ARG))
		lp->corelog = 1;

	mirrorlog = arg_str_value(cmd, mirrorlog_ARG,
				  lp->corelog ? "core" : DEFAULT_MIRRORLOG);

	if (!strcmp("disk", mirrorlog)) {
		if (lp->corelog) {
			log_error("--mirrorlog disk and --corelog "
				  "are incompatible");
			return 0;
		}
		lp->corelog = 0;
	} else if (!strcmp("core", mirrorlog))
		lp->corelog = 1;
	else {
		log_error("Unknown mirrorlog type: %s", mirrorlog);
		return 0;
	}

	log_verbose("Setting logging type to %s", mirrorlog);

	lp->nosync = arg_is_set(cmd, nosync_ARG);

	if (arg_count(cmd, regionsize_ARG)) {
		if (arg_sign_value(cmd, regionsize_ARG, 0) == SIGN_MINUS) {
			log_error("Negative regionsize is invalid");
			return 0;
		}
		lp->region_size = arg_uint_value(cmd, regionsize_ARG, 0);
	} else {
		region_size = 2 * find_config_tree_int(cmd,
					"activation/mirror_region_size",
					DEFAULT_MIRROR_REGION_SIZE);
		if (region_size < 0) {
			log_error("Negative regionsize in configuration file "
				  "is invalid");
			return 0;
		}
		lp->region_size = region_size;
	}

	if (!_validate_mirror_params(cmd, lp))
		return 0;

	return 1;
}

static int _lvcreate_params(struct lvcreate_params *lp,
			    struct lvcreate_cmdline_params *lcp,
			    struct cmd_context *cmd,
			    int argc, char **argv)
{
	int contiguous;
	unsigned pagesize;

	memset(lp, 0, sizeof(*lp));
	memset(lcp, 0, sizeof(*lcp));

	/*
	 * Check selected options are compatible and determine segtype
	 */
	lp->segtype = (const struct segment_type *)
	    arg_ptr_value(cmd, type_ARG,
			  get_segtype_from_string(cmd, "striped"));

	lp->stripes = arg_uint_value(cmd, stripes_ARG, 1);
	if (arg_count(cmd, stripes_ARG) && lp->stripes == 1)
		log_print("Redundant stripes argument: default is 1");

	if (arg_count(cmd, snapshot_ARG) || seg_is_snapshot(lp) ||
	    arg_count(cmd, virtualsize_ARG))
		lp->snapshot = 1;

	lp->mirrors = 1;

	/* Default to 2 mirrored areas if --type mirror */
	if (seg_is_mirrored(lp))
		lp->mirrors = 2;

	if (arg_count(cmd, mirrors_ARG)) {
		lp->mirrors = arg_uint_value(cmd, mirrors_ARG, 0) + 1;
		if (lp->mirrors == 1)
			log_print("Redundant mirrors argument: default is 0");
		if (arg_sign_value(cmd, mirrors_ARG, 0) == SIGN_MINUS) {
			log_error("Mirrors argument may not be negative");
			return 0;
		}
	}

	if (lp->snapshot) {
		if (arg_count(cmd, zero_ARG)) {
			log_error("-Z is incompatible with snapshots");
			return 0;
		}
		if (arg_sign_value(cmd, chunksize_ARG, 0) == SIGN_MINUS) {
			log_error("Negative chunk size is invalid");
			return 0;
		}
		lp->chunk_size = arg_uint_value(cmd, chunksize_ARG, 8);
		if (lp->chunk_size < 8 || lp->chunk_size > 1024 ||
		    (lp->chunk_size & (lp->chunk_size - 1))) {
			log_error("Chunk size must be a power of 2 in the "
				  "range 4K to 512K");
			return 0;
		}
		log_verbose("Setting chunksize to %d sectors.", lp->chunk_size);

		if (!(lp->segtype = get_segtype_from_string(cmd, "snapshot")))
			return_0;
	} else {
		if (arg_count(cmd, chunksize_ARG)) {
			log_error("-c is only available with snapshots");
			return 0;
		}
	}

	if (lp->mirrors > 1) {
		if (lp->snapshot) {
			log_error("mirrors and snapshots are currently "
				  "incompatible");
			return 0;
		}

		if (lp->stripes > 1) {
			log_error("mirrors and stripes are currently "
				  "incompatible");
			return 0;
		}

		if (!(lp->segtype = get_segtype_from_string(cmd, "striped")))
			return_0;
	} else {
		if (arg_count(cmd, corelog_ARG)) {
			log_error("--corelog is only available with mirrors");
			return 0;
		}

		if (arg_count(cmd, nosync_ARG)) {
			log_error("--nosync is only available with mirrors");
			return 0;
		}
	}

	if (activation() && lp->segtype->ops->target_present &&
	    !lp->segtype->ops->target_present(cmd, NULL, NULL)) {
		log_error("%s: Required device-mapper target(s) not "
			  "detected in your kernel", lp->segtype->name);
		return 0;
	}

	if (!_lvcreate_name_params(lp, cmd, &argc, &argv) ||
	    !_read_size_params(lp, lcp, cmd) ||
	    !_read_stripe_params(lp, cmd) ||
	    !_read_mirror_params(lp, cmd))
		return_0;

	/*
	 * Should we zero the lv.
	 */
	lp->zero = strcmp(arg_str_value(cmd, zero_ARG,
		(lp->segtype->flags & SEG_CANNOT_BE_ZEROED) ? "n" : "y"), "n");

	/*
	 * Alloc policy
	 */
	contiguous = strcmp(arg_str_value(cmd, contiguous_ARG, "n"), "n");

	lp->alloc = contiguous ? ALLOC_CONTIGUOUS : ALLOC_INHERIT;

	lp->alloc = arg_uint_value(cmd, alloc_ARG, lp->alloc);

	if (contiguous && (lp->alloc != ALLOC_CONTIGUOUS)) {
		log_error("Conflicting contiguous and alloc arguments");
		return 0;
	}

	/*
	 * Read ahead.
	 */
	lp->read_ahead = arg_uint_value(cmd, readahead_ARG, DM_READ_AHEAD_NONE);
	pagesize = lvm_getpagesize() >> SECTOR_SHIFT;
	if (lp->read_ahead != DM_READ_AHEAD_AUTO &&
	    lp->read_ahead != DM_READ_AHEAD_NONE &&
	    lp->read_ahead % pagesize) {
		if (lp->read_ahead < pagesize)
			lp->read_ahead = pagesize;
		else
			lp->read_ahead = (lp->read_ahead / pagesize) * pagesize;
		log_warn("WARNING: Overriding readahead to %u sectors, a multiple "
			    "of %uK page size.", lp->read_ahead, pagesize >> 1);
	}

	/*
	 * Permissions.
	 */
	lp->permission = arg_uint_value(cmd, permission_ARG,
					LVM_READ | LVM_WRITE);

	/* Must not zero read only volume */
	if (!(lp->permission & LVM_WRITE))
		lp->zero = 0;

	lp->minor = arg_int_value(cmd, minor_ARG, -1);
	lp->major = arg_int_value(cmd, major_ARG, -1);

	/* Persistent minor */
	if (arg_count(cmd, persistent_ARG)) {
		if (!strcmp(arg_str_value(cmd, persistent_ARG, "n"), "y")) {
			if (lp->minor == -1) {
				log_error("Please specify minor number with "
					  "--minor when using -My");
				return 0;
			}
			if (lp->major == -1) {
				log_error("Please specify major number with "
					  "--major when using -My");
				return 0;
			}
		} else {
			if ((lp->minor != -1) || (lp->major != -1)) {
				log_error("--major and --minor incompatible "
					  "with -Mn");
				return 0;
			}
		}
	} else if (arg_count(cmd, minor_ARG) || arg_count(cmd, major_ARG)) {
		log_error("--major and --minor require -My");
		return 0;
	}

	lp->tag = arg_str_value(cmd, addtag_ARG, NULL);

	lcp->pv_count = argc;
	lcp->pvs = argv;

	return 1;
}

int lvcreate(struct cmd_context *cmd, int argc, char **argv)
{
	int r = ECMD_PROCESSED;
	struct lvcreate_params lp;
	struct lvcreate_cmdline_params lcp;
	struct volume_group *vg;

	memset(&lp, 0, sizeof(lp));

	if (!_lvcreate_params(&lp, &lcp, cmd, argc, argv))
		return EINVALID_CMD_LINE;

	log_verbose("Finding volume group \"%s\"", lp.vg_name);
	vg = vg_read_for_update(cmd, lp.vg_name, NULL, 0);
	if (vg_read_error(vg)) {
		vg_release(vg);
		stack;
		return ECMD_FAILED;
	}

	if (!_update_extents_params(vg, &lp, &lcp)) {
		r = ECMD_FAILED;
		goto_out;
	}

	if (!lv_create_single(vg, &lp)) {
		stack;
		r = ECMD_FAILED;
	}
out:
	unlock_and_release_vg(cmd, vg, lp.vg_name);
	return r;
}
