/*	$NetBSD: toollib.c,v 1.1.1.3 2009/12/02 00:25:56 haad Exp $	*/

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
#include "lv_alloc.h"
#include "xlate.h"

#include <sys/stat.h>
#include <sys/wait.h>

const char *command_name(struct cmd_context *cmd)
{
	return cmd->command->name;
}

/*
 * Strip dev_dir if present
 */
char *skip_dev_dir(struct cmd_context *cmd, const char *vg_name,
		   unsigned *dev_dir_found)
{
	const char *dmdir = dm_dir();
	size_t dmdir_len = strlen(dmdir), vglv_sz;
	char *vgname, *lvname, *layer, *vglv;

	/* FIXME Do this properly */
	if (*vg_name == '/') {
		while (*vg_name == '/')
			vg_name++;
		vg_name--;
	}

	/* Reformat string if /dev/mapper found */
	if (!strncmp(vg_name, dmdir, dmdir_len) && vg_name[dmdir_len] == '/') {
		if (dev_dir_found)
			*dev_dir_found = 1;
		vg_name += dmdir_len;
		while (*vg_name == '/')
			vg_name++;

		if (!dm_split_lvm_name(cmd->mem, vg_name, &vgname, &lvname, &layer) ||
		    *layer) {
			log_error("skip_dev_dir: Couldn't split up device name %s",
				  vg_name);
			return (char *) vg_name;
		}
		vglv_sz = strlen(vgname) + strlen(lvname) + 2;
		if (!(vglv = dm_pool_alloc(cmd->mem, vglv_sz)) ||
		    dm_snprintf(vglv, vglv_sz, "%s%s%s", vgname,
				 *lvname ? "/" : "",
				 lvname) < 0) {
			log_error("vg/lv string alloc failed");
			return (char *) vg_name;
		}
		return vglv;
	}

	if (!strncmp(vg_name, cmd->dev_dir, strlen(cmd->dev_dir))) {
		if (dev_dir_found)
			*dev_dir_found = 1;
		vg_name += strlen(cmd->dev_dir);
		while (*vg_name == '/')
			vg_name++;
	} else if (dev_dir_found)
		*dev_dir_found = 0;

	return (char *) vg_name;
}

/*
 * Metadata iteration functions
 */
int process_each_lv_in_vg(struct cmd_context *cmd,
			  const struct volume_group *vg,
			  const struct dm_list *arg_lvnames,
			  const struct dm_list *tags,
			  void *handle,
			  process_single_lv_fn_t process_single)
{
	int ret_max = ECMD_PROCESSED;
	int ret = 0;
	unsigned process_all = 0;
	unsigned process_lv = 0;
	unsigned tags_supplied = 0;
	unsigned lvargs_supplied = 0;
	unsigned lvargs_matched = 0;

	struct lv_list *lvl;

	if (!vg_check_status(vg, EXPORTED_VG))
		return ECMD_FAILED;

	if (tags && !dm_list_empty(tags))
		tags_supplied = 1;

	if (arg_lvnames && !dm_list_empty(arg_lvnames))
		lvargs_supplied = 1;

	/* Process all LVs in this VG if no restrictions given */
	if (!tags_supplied && !lvargs_supplied)
		process_all = 1;

	/* Or if VG tags match */
	if (!process_lv && tags_supplied &&
	    str_list_match_list(tags, &vg->tags)) {
		process_all = 1;
	}

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (lvl->lv->status & SNAPSHOT)
			continue;

		if (lv_is_virtual_origin(lvl->lv) && !arg_count(cmd, all_ARG))
			continue;

		/* Should we process this LV? */
		if (process_all)
			process_lv = 1;
		else
			process_lv = 0;

		/* LV tag match? */
		if (!process_lv && tags_supplied &&
		    str_list_match_list(tags, &lvl->lv->tags)) {
			process_lv = 1;
		}

		/* LV name match? */
		if (lvargs_supplied &&
		    str_list_match_item(arg_lvnames, lvl->lv->name)) {
			process_lv = 1;
			lvargs_matched++;
		}

		if (!process_lv)
			continue;

		ret = process_single(cmd, lvl->lv, handle);
		if (ret > ret_max)
			ret_max = ret;
		if (sigint_caught())
			return ret_max;
	}

	if (lvargs_supplied && lvargs_matched != dm_list_size(arg_lvnames)) {
		log_error("One or more specified logical volume(s) not found.");
		if (ret_max < ECMD_FAILED)
			ret_max = ECMD_FAILED;
	}

	return ret_max;
}

int process_each_lv(struct cmd_context *cmd, int argc, char **argv,
		    uint32_t flags, void *handle,
		    int (*process_single) (struct cmd_context * cmd,
					   struct logical_volume * lv,
					   void *handle))
{
	int opt = 0;
	int ret_max = ECMD_PROCESSED;
	int ret = 0;

	struct dm_list *tags_arg;
	struct dm_list *vgnames;	/* VGs to process */
	struct str_list *sll, *strl;
	struct volume_group *vg;
	struct dm_list tags, lvnames;
	struct dm_list arg_lvnames;	/* Cmdline vgname or vgname/lvname */
	char *vglv;
	size_t vglv_sz;

	const char *vgname;

	dm_list_init(&tags);
	dm_list_init(&arg_lvnames);

	if (argc) {
		struct dm_list arg_vgnames;

		log_verbose("Using logical volume(s) on command line");
		dm_list_init(&arg_vgnames);

		for (; opt < argc; opt++) {
			const char *lv_name = argv[opt];
			char *vgname_def;
			unsigned dev_dir_found = 0;

			/* Do we have a tag or vgname or lvname? */
			vgname = lv_name;

			if (*vgname == '@') {
				if (!validate_name(vgname + 1)) {
					log_error("Skipping invalid tag %s",
						  vgname);
					continue;
				}
				if (!str_list_add(cmd->mem, &tags,
						  dm_pool_strdup(cmd->mem,
							      vgname + 1))) {
					log_error("strlist allocation failed");
					return ECMD_FAILED;
				}
				continue;
			}

			/* FIXME Jumbled parsing */
			vgname = skip_dev_dir(cmd, vgname, &dev_dir_found);

			if (*vgname == '/') {
				log_error("\"%s\": Invalid path for Logical "
					  "Volume", argv[opt]);
				if (ret_max < ECMD_FAILED)
					ret_max = ECMD_FAILED;
				continue;
			}
			lv_name = vgname;
			if (strchr(vgname, '/')) {
				/* Must be an LV */
				lv_name = strchr(vgname, '/');
				while (*lv_name == '/')
					lv_name++;
				if (!(vgname = extract_vgname(cmd, vgname))) {
					if (ret_max < ECMD_FAILED)
						ret_max = ECMD_FAILED;
					continue;
				}
			} else if (!dev_dir_found &&
				   (vgname_def = default_vgname(cmd))) {
				vgname = vgname_def;
			} else
				lv_name = NULL;

			if (!str_list_add(cmd->mem, &arg_vgnames,
					  dm_pool_strdup(cmd->mem, vgname))) {
				log_error("strlist allocation failed");
				return ECMD_FAILED;
			}

			if (!lv_name) {
				if (!str_list_add(cmd->mem, &arg_lvnames,
						  dm_pool_strdup(cmd->mem,
							      vgname))) {
					log_error("strlist allocation failed");
					return ECMD_FAILED;
				}
			} else {
				vglv_sz = strlen(vgname) + strlen(lv_name) + 2;
				if (!(vglv = dm_pool_alloc(cmd->mem, vglv_sz)) ||
				    dm_snprintf(vglv, vglv_sz, "%s/%s", vgname,
						 lv_name) < 0) {
					log_error("vg/lv string alloc failed");
					return ECMD_FAILED;
				}
				if (!str_list_add(cmd->mem, &arg_lvnames, vglv)) {
					log_error("strlist allocation failed");
					return ECMD_FAILED;
				}
			}
		}
		vgnames = &arg_vgnames;
	}

	if (!argc || !dm_list_empty(&tags)) {
		log_verbose("Finding all logical volumes");
		if (!(vgnames = get_vgnames(cmd, 0)) || dm_list_empty(vgnames)) {
			log_error("No volume groups found");
			return ret_max;
		}
	}

	vg = NULL;
	dm_list_iterate_items(strl, vgnames) {
		vgname = strl->str;
		if (is_orphan_vg(vgname))
			continue;	/* FIXME Unnecessary? */
		vg = vg_read(cmd, vgname, NULL, flags);

		if (vg_read_error(vg)) {
			vg_release(vg);
			if (ret_max < ECMD_FAILED) {
				log_error("Skipping volume group %s", vgname);
				ret_max = ECMD_FAILED;
			} else
				stack;
			continue;
		}

		tags_arg = &tags;
		dm_list_init(&lvnames);	/* LVs to be processed in this VG */
		dm_list_iterate_items(sll, &arg_lvnames) {
			const char *vg_name = sll->str;
			const char *lv_name = strchr(vg_name, '/');

			if ((!lv_name && !strcmp(vg_name, vgname))) {
				/* Process all LVs in this VG */
				tags_arg = NULL;
				dm_list_init(&lvnames);
				break;
			} else if (!strncmp(vg_name, vgname, strlen(vgname)) &&
				   strlen(vgname) == (size_t) (lv_name - vg_name)) {
				if (!str_list_add(cmd->mem, &lvnames,
						  dm_pool_strdup(cmd->mem,
							      lv_name + 1))) {
					log_error("strlist allocation failed");
					vg_release(vg);
					return ECMD_FAILED;
				}
			}
		}

		ret = process_each_lv_in_vg(cmd, vg, &lvnames, tags_arg,
					    handle, process_single);
		unlock_and_release_vg(cmd, vg, vgname);
		if (ret > ret_max)
			ret_max = ret;
		if (sigint_caught())
			break;
	}

	return ret_max;
}

int process_each_segment_in_pv(struct cmd_context *cmd,
			       struct volume_group *vg,
			       struct physical_volume *pv,
			       void *handle,
			       int (*process_single) (struct cmd_context * cmd,
						      struct volume_group * vg,
						      struct pv_segment * pvseg,
						      void *handle))
{
	struct pv_segment *pvseg;
	struct pv_list *pvl;
	const char *vg_name = NULL;
	int ret_max = ECMD_PROCESSED;
	int ret;
	struct volume_group *old_vg = vg;
	struct pv_segment _free_pv_segment = { .pv = pv };

	if (is_pv(pv) && !vg && !is_orphan(pv)) {
		vg_name = pv_vg_name(pv);

		vg = vg_read(cmd, vg_name, NULL, 0);
		if (vg_read_error(vg)) {
			vg_release(vg);
			log_error("Skipping volume group %s", vg_name);
			return ECMD_FAILED;
		}

		/*
		 * Replace possibly incomplete PV structure with new one
		 * allocated in vg_read_internal() path.
		 */
		if (!(pvl = find_pv_in_vg(vg, pv_dev_name(pv)))) {
			 log_error("Unable to find %s in volume group %s",
				   pv_dev_name(pv), vg_name);
			vg_release(vg);
			return ECMD_FAILED;
		}

		pv = pvl->pv;
	}

	if (dm_list_empty(&pv->segments)) {
		ret = process_single(cmd, NULL, &_free_pv_segment, handle);
		if (ret > ret_max)
			ret_max = ret;
	} else
		dm_list_iterate_items(pvseg, &pv->segments) {
			ret = process_single(cmd, vg, pvseg, handle);
			if (ret > ret_max)
				ret_max = ret;
			if (sigint_caught())
				break;
		}

	if (vg_name)
		unlock_vg(cmd, vg_name);
	if (!old_vg)
		vg_release(vg);

	return ret_max;
}

int process_each_segment_in_lv(struct cmd_context *cmd,
			       struct logical_volume *lv,
			       void *handle,
			       int (*process_single) (struct cmd_context * cmd,
						      struct lv_segment * seg,
						      void *handle))
{
	struct lv_segment *seg;
	int ret_max = ECMD_PROCESSED;
	int ret;

	dm_list_iterate_items(seg, &lv->segments) {
		ret = process_single(cmd, seg, handle);
		if (ret > ret_max)
			ret_max = ret;
		if (sigint_caught())
			break;
	}

	return ret_max;
}

static int _process_one_vg(struct cmd_context *cmd, const char *vg_name,
			   const char *vgid,
			   struct dm_list *tags, struct dm_list *arg_vgnames,
			   uint32_t flags, void *handle, int ret_max,
			   int (*process_single) (struct cmd_context * cmd,
						  const char *vg_name,
						  struct volume_group * vg,
						  void *handle))
{
	struct volume_group *vg;
	int ret = 0;

	log_verbose("Finding volume group \"%s\"", vg_name);

	vg = vg_read(cmd, vg_name, vgid, flags);
	/* Allow FAILED_INCONSISTENT through only for vgcfgrestore */
	if (vg_read_error(vg) &&
	    !((vg_read_error(vg) == FAILED_INCONSISTENT) &&
	      (flags & READ_ALLOW_INCONSISTENT))) {
		ret_max = ECMD_FAILED;
		goto_out;
	}

	if (!dm_list_empty(tags)) {
		/* Only process if a tag matches or it's on arg_vgnames */
		if (!str_list_match_item(arg_vgnames, vg_name) &&
		    !str_list_match_list(tags, &vg->tags))
			goto out;
	}

	if ((ret = process_single(cmd, vg_name, vg,
				  handle)) > ret_max)
		ret_max = ret;

out:
	if (vg_read_error(vg))
		vg_release(vg);
	else
		unlock_and_release_vg(cmd, vg, vg_name);
	return ret_max;
}

int process_each_vg(struct cmd_context *cmd, int argc, char **argv,
		    uint32_t flags, void *handle,
		    int (*process_single) (struct cmd_context * cmd,
					   const char *vg_name,
					   struct volume_group * vg,
					   void *handle))
{
	int opt = 0;
	int ret_max = ECMD_PROCESSED;

	struct str_list *sl;
	struct dm_list *vgnames, *vgids;
	struct dm_list arg_vgnames, tags;

	const char *vg_name, *vgid;

	dm_list_init(&tags);
	dm_list_init(&arg_vgnames);

	if (argc) {
		log_verbose("Using volume group(s) on command line");

		for (; opt < argc; opt++) {
			vg_name = argv[opt];
			if (*vg_name == '@') {
				if (!validate_name(vg_name + 1)) {
					log_error("Skipping invalid tag %s",
						  vg_name);
					if (ret_max < EINVALID_CMD_LINE)
						ret_max = EINVALID_CMD_LINE;
					continue;
				}
				if (!str_list_add(cmd->mem, &tags,
						  dm_pool_strdup(cmd->mem,
							      vg_name + 1))) {
					log_error("strlist allocation failed");
					return ECMD_FAILED;
				}
				continue;
			}

			vg_name = skip_dev_dir(cmd, vg_name, NULL);
			if (strchr(vg_name, '/')) {
				log_error("Invalid volume group name: %s",
					  vg_name);
				if (ret_max < EINVALID_CMD_LINE)
					ret_max = EINVALID_CMD_LINE;
				continue;
			}
			if (!str_list_add(cmd->mem, &arg_vgnames,
					  dm_pool_strdup(cmd->mem, vg_name))) {
				log_error("strlist allocation failed");
				return ECMD_FAILED;
			}
		}

		vgnames = &arg_vgnames;
	}

	if (!argc || !dm_list_empty(&tags)) {
		log_verbose("Finding all volume groups");
		if (!(vgids = get_vgids(cmd, 0)) || dm_list_empty(vgids)) {
			log_error("No volume groups found");
			return ret_max;
		}
		dm_list_iterate_items(sl, vgids) {
			vgid = sl->str;
			if (!vgid || !(vg_name = vgname_from_vgid(cmd->mem, vgid)) ||
			    is_orphan_vg(vg_name))
				continue;
			ret_max = _process_one_vg(cmd, vg_name, vgid, &tags,
						  &arg_vgnames,
						  flags, handle,
					  	  ret_max, process_single);
			if (sigint_caught())
				return ret_max;
		}
	} else {
		dm_list_iterate_items(sl, vgnames) {
			vg_name = sl->str;
			if (is_orphan_vg(vg_name))
				continue;	/* FIXME Unnecessary? */
			ret_max = _process_one_vg(cmd, vg_name, NULL, &tags,
						  &arg_vgnames,
						  flags, handle,
					  	  ret_max, process_single);
			if (sigint_caught())
				return ret_max;
		}
	}

	return ret_max;
}

int process_each_pv_in_vg(struct cmd_context *cmd, struct volume_group *vg,
			  const struct dm_list *tags, void *handle,
			  process_single_pv_fn_t process_single)
{
	int ret_max = ECMD_PROCESSED;
	int ret = 0;
	struct pv_list *pvl;

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (tags && !dm_list_empty(tags) &&
		    !str_list_match_list(tags, &pvl->pv->tags)) {
			continue;
		}
		if ((ret = process_single(cmd, vg, pvl->pv, handle)) > ret_max)
			ret_max = ret;
		if (sigint_caught())
			return ret_max;
	}

	return ret_max;
}

static int _process_all_devs(struct cmd_context *cmd, void *handle,
		    int (*process_single) (struct cmd_context * cmd,
					   struct volume_group * vg,
					   struct physical_volume * pv,
					   void *handle))
{
	struct physical_volume *pv;
	struct physical_volume pv_dummy;
	struct dev_iter *iter;
	struct device *dev;

	int ret_max = ECMD_PROCESSED;
	int ret = 0;

	if (!scan_vgs_for_pvs(cmd)) {
		stack;
		return ECMD_FAILED;
	}

	if (!(iter = dev_iter_create(cmd->filter, 1))) {
		log_error("dev_iter creation failed");
		return ECMD_FAILED;
	}

	while ((dev = dev_iter_get(iter))) {
		if (!(pv = pv_read(cmd, dev_name(dev), NULL, NULL, 0, 0))) {
			memset(&pv_dummy, 0, sizeof(pv_dummy));
			dm_list_init(&pv_dummy.tags);
			dm_list_init(&pv_dummy.segments);
			pv_dummy.dev = dev;
			pv_dummy.fmt = NULL;
			pv = &pv_dummy;
		}
		ret = process_single(cmd, NULL, pv, handle);
		if (ret > ret_max)
			ret_max = ret;
		if (sigint_caught())
			break;
	}

	dev_iter_destroy(iter);

	return ret_max;
}

/*
 * If the lock_type is LCK_VG_READ (used only in reporting commands),
 * we lock VG_GLOBAL to enable use of metadata cache.
 * This can pause alongide pvscan or vgscan process for a while.
 */
int process_each_pv(struct cmd_context *cmd, int argc, char **argv,
		    struct volume_group *vg, uint32_t flags,
		    int scan_label_only, void *handle,
		    int (*process_single) (struct cmd_context * cmd,
					   struct volume_group * vg,
					   struct physical_volume * pv,
					   void *handle))
{
	int opt = 0;
	int ret_max = ECMD_PROCESSED;
	int ret = 0;
	int lock_global = !(flags & READ_WITHOUT_LOCK) && !(flags & READ_FOR_UPDATE);

	struct pv_list *pvl;
	struct physical_volume *pv;
	struct dm_list *pvslist, *vgnames;
	struct dm_list tags;
	struct str_list *sll;
	char *tagname;
	int scanned = 0;

	dm_list_init(&tags);

	if (lock_global && !lock_vol(cmd, VG_GLOBAL, LCK_VG_READ)) {
		log_error("Unable to obtain global lock.");
		return ECMD_FAILED;
	}

	if (argc) {
		log_verbose("Using physical volume(s) on command line");
		for (; opt < argc; opt++) {
			if (*argv[opt] == '@') {
				tagname = argv[opt] + 1;

				if (!validate_name(tagname)) {
					log_error("Skipping invalid tag %s",
						  tagname);
					if (ret_max < EINVALID_CMD_LINE)
						ret_max = EINVALID_CMD_LINE;
					continue;
				}
				if (!str_list_add(cmd->mem, &tags,
						  dm_pool_strdup(cmd->mem,
							      tagname))) {
					log_error("strlist allocation failed");
					goto bad;
				}
				continue;
			}
			if (vg) {
				if (!(pvl = find_pv_in_vg(vg, argv[opt]))) {
					log_error("Physical Volume \"%s\" not "
						  "found in Volume Group "
						  "\"%s\"", argv[opt],
						  vg->name);
					ret_max = ECMD_FAILED;
					continue;
				}
				pv = pvl->pv;
			} else {
				if (!(pv = pv_read(cmd, argv[opt], NULL,
						   NULL, 1, scan_label_only))) {
					log_error("Failed to read physical "
						  "volume \"%s\"", argv[opt]);
					ret_max = ECMD_FAILED;
					continue;
				}

				/*
				 * If a PV has no MDAs it may appear to be an
				 * orphan until the metadata is read off
				 * another PV in the same VG.  Detecting this
				 * means checking every VG by scanning every
				 * PV on the system.
				 */
				if (!scanned && is_orphan(pv)) {
					if (!scan_label_only &&
					    !scan_vgs_for_pvs(cmd)) {
						stack;
						ret_max = ECMD_FAILED;
						continue;
					}
					scanned = 1;
					if (!(pv = pv_read(cmd, argv[opt],
							   NULL, NULL, 1,
							   scan_label_only))) {
						log_error("Failed to read "
							  "physical volume "
							  "\"%s\"", argv[opt]);
						ret_max = ECMD_FAILED;
						continue;
					}
				}
			}

			ret = process_single(cmd, vg, pv, handle);
			if (ret > ret_max)
				ret_max = ret;
			if (sigint_caught())
				goto out;
		}
		if (!dm_list_empty(&tags) && (vgnames = get_vgnames(cmd, 0)) &&
			   !dm_list_empty(vgnames)) {
			dm_list_iterate_items(sll, vgnames) {
				vg = vg_read(cmd, sll->str, NULL, flags);
				if (vg_read_error(vg)) {
					ret_max = ECMD_FAILED;
					vg_release(vg);
					stack;
					continue;
				}

				ret = process_each_pv_in_vg(cmd, vg, &tags,
							    handle,
							    process_single);

				unlock_and_release_vg(cmd, vg, sll->str);

				if (ret > ret_max)
					ret_max = ret;
				if (sigint_caught())
					goto out;
			}
		}
	} else {
		if (vg) {
			log_verbose("Using all physical volume(s) in "
				    "volume group");
			ret = process_each_pv_in_vg(cmd, vg, NULL, handle,
						    process_single);
			if (ret > ret_max)
				ret_max = ret;
			if (sigint_caught())
				goto out;
		} else if (arg_count(cmd, all_ARG)) {
			ret = _process_all_devs(cmd, handle, process_single);
			if (ret > ret_max)
				ret_max = ret;
			if (sigint_caught())
				goto out;
		} else {
			log_verbose("Scanning for physical volume names");

			if (!(pvslist = get_pvs(cmd)))
				goto bad;

			dm_list_iterate_items(pvl, pvslist) {
				ret = process_single(cmd, NULL, pvl->pv,
						     handle);
				if (ret > ret_max)
					ret_max = ret;
				if (sigint_caught())
					goto out;
			}
		}
	}
out:
	if (lock_global)
		unlock_vg(cmd, VG_GLOBAL);
	return ret_max;
bad:
	if (lock_global)
		unlock_vg(cmd, VG_GLOBAL);

	return ECMD_FAILED;
}

/*
 * Determine volume group name from a logical volume name
 */
const char *extract_vgname(struct cmd_context *cmd, const char *lv_name)
{
	const char *vg_name = lv_name;
	char *st;
	char *dev_dir = cmd->dev_dir;
	int dev_dir_provided = 0;

	/* Path supplied? */
	if (vg_name && strchr(vg_name, '/')) {
		/* Strip dev_dir (optional) */
		if (*vg_name == '/') {
			while (*vg_name == '/')
				vg_name++;
			vg_name--;
		}
		if (!strncmp(vg_name, dev_dir, strlen(dev_dir))) {
			vg_name += strlen(dev_dir);
			dev_dir_provided = 1;
			while (*vg_name == '/')
				vg_name++;
		}
		if (*vg_name == '/') {
			log_error("\"%s\": Invalid path for Logical "
				  "Volume", lv_name);
			return 0;
		}

		/* Require exactly one set of consecutive slashes */
		if ((st = strchr(vg_name, '/')))
			while (*st == '/')
				st++;

		if (!strchr(vg_name, '/') || strchr(st, '/')) {
			log_error("\"%s\": Invalid path for Logical Volume",
				  lv_name);
			return 0;
		}

		vg_name = dm_pool_strdup(cmd->mem, vg_name);
		if (!vg_name) {
			log_error("Allocation of vg_name failed");
			return 0;
		}

		*strchr(vg_name, '/') = '\0';
		return vg_name;
	}

	if (!(vg_name = default_vgname(cmd))) {
		if (lv_name)
			log_error("Path required for Logical Volume \"%s\"",
				  lv_name);
		return 0;
	}

	return vg_name;
}

/*
 * Extract default volume group name from environment
 */
char *default_vgname(struct cmd_context *cmd)
{
	char *vg_path;

	/* Take default VG from environment? */
	vg_path = getenv("LVM_VG_NAME");
	if (!vg_path)
		return 0;

	vg_path = skip_dev_dir(cmd, vg_path, NULL);

	if (strchr(vg_path, '/')) {
		log_error("Environment Volume Group in LVM_VG_NAME invalid: "
			  "\"%s\"", vg_path);
		return 0;
	}

	return dm_pool_strdup(cmd->mem, vg_path);
}

/*
 * Process physical extent range specifiers
 */
static int _add_pe_range(struct dm_pool *mem, const char *pvname,
			 struct dm_list *pe_ranges, uint32_t start, uint32_t count)
{
	struct pe_range *per;

	log_debug("Adding PE range: start PE %" PRIu32 " length %" PRIu32
		  " on %s", start, count, pvname);

	/* Ensure no overlap with existing areas */
	dm_list_iterate_items(per, pe_ranges) {
		if (((start < per->start) && (start + count - 1 >= per->start))
		    || ((start >= per->start) &&
			(per->start + per->count - 1) >= start)) {
			log_error("Overlapping PE ranges specified (%" PRIu32
				  "-%" PRIu32 ", %" PRIu32 "-%" PRIu32 ")"
				  " on %s",
				  start, start + count - 1, per->start,
				  per->start + per->count - 1, pvname);
			return 0;
		}
	}

	if (!(per = dm_pool_alloc(mem, sizeof(*per)))) {
		log_error("Allocation of list failed");
		return 0;
	}

	per->start = start;
	per->count = count;
	dm_list_add(pe_ranges, &per->list);

	return 1;
}

static int xstrtouint32(const char *s, char **p, int base, uint32_t *result)
{
	unsigned long ul;

	errno = 0;
	ul = strtoul(s, p, base);
	if (errno || *p == s || (uint32_t) ul != ul)
		return -1;
	*result = ul;
	return 0;
}

static int _parse_pes(struct dm_pool *mem, char *c, struct dm_list *pe_ranges,
		      const char *pvname, uint32_t size)
{
	char *endptr;
	uint32_t start, end;

	/* Default to whole PV */
	if (!c) {
		if (!_add_pe_range(mem, pvname, pe_ranges, UINT32_C(0), size))
			return_0;
		return 1;
	}

	while (*c) {
		if (*c != ':')
			goto error;

		c++;

		/* Disallow :: and :\0 */
		if (*c == ':' || !*c)
			goto error;

		/* Default to whole range */
		start = UINT32_C(0);
		end = size - 1;

		/* Start extent given? */
		if (isdigit(*c)) {
			if (xstrtouint32(c, &endptr, 10, &start))
				goto error;
			c = endptr;
			/* Just one number given? */
			if (!*c || *c == ':')
				end = start;
		}
		/* Range? */
		if (*c == '-') {
			c++;
			if (isdigit(*c)) {
				if (xstrtouint32(c, &endptr, 10, &end))
					goto error;
				c = endptr;
			}
		}
		if (*c && *c != ':')
			goto error;

		if ((start > end) || (end > size - 1)) {
			log_error("PE range error: start extent %" PRIu32 " to "
				  "end extent %" PRIu32, start, end);
			return 0;
		}

		if (!_add_pe_range(mem, pvname, pe_ranges, start, end - start + 1))
			return_0;

	}

	return 1;

      error:
	log_error("Physical extent parsing error at %s", c);
	return 0;
}

static int _create_pv_entry(struct dm_pool *mem, struct pv_list *pvl,
			     char *colon, int allocatable_only, struct dm_list *r)
{
	const char *pvname;
	struct pv_list *new_pvl = NULL, *pvl2;
	struct dm_list *pe_ranges;

	pvname = pv_dev_name(pvl->pv);
	if (allocatable_only && !(pvl->pv->status & ALLOCATABLE_PV)) {
		log_error("Physical volume %s not allocatable", pvname);
		return 1;
	}

	if (allocatable_only && (pvl->pv->status & MISSING_PV)) {
		log_error("Physical volume %s is missing", pvname);
		return 1;
	}

	if (allocatable_only &&
	    (pvl->pv->pe_count == pvl->pv->pe_alloc_count)) {
		log_error("No free extents on physical volume \"%s\"", pvname);
		return 1;
	}

	dm_list_iterate_items(pvl2, r)
		if (pvl->pv->dev == pvl2->pv->dev) {
			new_pvl = pvl2;
			break;
		}

	if (!new_pvl) {
		if (!(new_pvl = dm_pool_alloc(mem, sizeof(*new_pvl)))) {
			log_error("Unable to allocate physical volume list.");
			return 0;
		}

		memcpy(new_pvl, pvl, sizeof(*new_pvl));

		if (!(pe_ranges = dm_pool_alloc(mem, sizeof(*pe_ranges)))) {
			log_error("Allocation of pe_ranges list failed");
			return 0;
		}
		dm_list_init(pe_ranges);
		new_pvl->pe_ranges = pe_ranges;
		dm_list_add(r, &new_pvl->list);
	}

	/* Determine selected physical extents */
	if (!_parse_pes(mem, colon, new_pvl->pe_ranges, pv_dev_name(pvl->pv),
			pvl->pv->pe_count))
		return_0;

	return 1;
}

struct dm_list *create_pv_list(struct dm_pool *mem, struct volume_group *vg, int argc,
			    char **argv, int allocatable_only)
{
	struct dm_list *r;
	struct pv_list *pvl;
	struct dm_list tags, arg_pvnames;
	const char *pvname = NULL;
	char *colon, *tagname;
	int i;

	/* Build up list of PVs */
	if (!(r = dm_pool_alloc(mem, sizeof(*r)))) {
		log_error("Allocation of list failed");
		return NULL;
	}
	dm_list_init(r);

	dm_list_init(&tags);
	dm_list_init(&arg_pvnames);

	for (i = 0; i < argc; i++) {
		if (*argv[i] == '@') {
			tagname = argv[i] + 1;
			if (!validate_name(tagname)) {
				log_error("Skipping invalid tag %s", tagname);
				continue;
			}
			dm_list_iterate_items(pvl, &vg->pvs) {
				if (str_list_match_item(&pvl->pv->tags,
							tagname)) {
					if (!_create_pv_entry(mem, pvl, NULL,
							      allocatable_only,
							      r))
						return_NULL;
				}
			}
			continue;
		}

		pvname = argv[i];

		if ((colon = strchr(pvname, ':'))) {
			if (!(pvname = dm_pool_strndup(mem, pvname,
						    (unsigned) (colon -
								pvname)))) {
				log_error("Failed to clone PV name");
				return NULL;
			}
		}

		if (!(pvl = find_pv_in_vg(vg, pvname))) {
			log_error("Physical Volume \"%s\" not found in "
				  "Volume Group \"%s\"", pvname, vg->name);
			return NULL;
		}
		if (!_create_pv_entry(mem, pvl, colon, allocatable_only, r))
			return_NULL;
	}

	if (dm_list_empty(r))
		log_error("No specified PVs have space available");

	return dm_list_empty(r) ? NULL : r;
}

struct dm_list *clone_pv_list(struct dm_pool *mem, struct dm_list *pvsl)
{
	struct dm_list *r;
	struct pv_list *pvl, *new_pvl;

	/* Build up list of PVs */
	if (!(r = dm_pool_alloc(mem, sizeof(*r)))) {
		log_error("Allocation of list failed");
		return NULL;
	}
	dm_list_init(r);

	dm_list_iterate_items(pvl, pvsl) {
		if (!(new_pvl = dm_pool_zalloc(mem, sizeof(*new_pvl)))) {
			log_error("Unable to allocate physical volume list.");
			return NULL;
		}

		memcpy(new_pvl, pvl, sizeof(*new_pvl));
		dm_list_add(r, &new_pvl->list);
	}

	return r;
}

int apply_lvname_restrictions(const char *name)
{
	if (!strncmp(name, "snapshot", 8)) {
		log_error("Names starting \"snapshot\" are reserved. "
			  "Please choose a different LV name.");
		return 0;
	}

	if (!strncmp(name, "pvmove", 6)) {
		log_error("Names starting \"pvmove\" are reserved. "
			  "Please choose a different LV name.");
		return 0;
	}

	if (strstr(name, "_mlog")) {
		log_error("Names including \"_mlog\" are reserved. "
			  "Please choose a different LV name.");
		return 0;
	}

	if (strstr(name, "_mimage")) {
		log_error("Names including \"_mimage\" are reserved. "
			  "Please choose a different LV name.");
		return 0;
	}

	if (strstr(name, "_vorigin")) {
		log_error("Names including \"_vorigin\" are reserved. "
			  "Please choose a different LV name.");
		return 0;
	}

	return 1;
}

int is_reserved_lvname(const char *name)
{
	int rc, old_suppress;

	old_suppress = log_suppress(2);
	rc = !apply_lvname_restrictions(name);
	log_suppress(old_suppress);

	return rc;
}

void vgcreate_params_set_defaults(struct vgcreate_params *vp_def,
				  struct volume_group *vg)
{
	if (vg) {
		vp_def->vg_name = NULL;
		vp_def->extent_size = vg->extent_size;
		vp_def->max_pv = vg->max_pv;
		vp_def->max_lv = vg->max_lv;
		vp_def->alloc = vg->alloc;
		vp_def->clustered = vg_is_clustered(vg);
	} else {
		vp_def->vg_name = NULL;
		vp_def->extent_size = DEFAULT_EXTENT_SIZE * 2;
		vp_def->max_pv = DEFAULT_MAX_PV;
		vp_def->max_lv = DEFAULT_MAX_LV;
		vp_def->alloc = DEFAULT_ALLOC_POLICY;
		vp_def->clustered = DEFAULT_CLUSTERED;
	}
}

/*
 * Set members of struct vgcreate_params from cmdline arguments.
 * Do preliminary validation with arg_*() interface.
 * Further, more generic validation is done in validate_vgcreate_params().
 * This function is to remain in tools directory.
 */
int vgcreate_params_set_from_args(struct cmd_context *cmd,
				  struct vgcreate_params *vp_new,
				  struct vgcreate_params *vp_def)
{
	vp_new->vg_name = skip_dev_dir(cmd, vp_def->vg_name, NULL);
	vp_new->max_lv = arg_uint_value(cmd, maxlogicalvolumes_ARG,
					vp_def->max_lv);
	vp_new->max_pv = arg_uint_value(cmd, maxphysicalvolumes_ARG,
					vp_def->max_pv);
	vp_new->alloc = arg_uint_value(cmd, alloc_ARG, vp_def->alloc);

	/* Units of 512-byte sectors */
	vp_new->extent_size =
	    arg_uint_value(cmd, physicalextentsize_ARG, vp_def->extent_size);

	if (arg_count(cmd, clustered_ARG))
		vp_new->clustered =
			!strcmp(arg_str_value(cmd, clustered_ARG,
					      vp_def->clustered ? "y":"n"), "y");
	else
		/* Default depends on current locking type */
		vp_new->clustered = locking_is_clustered();

	if (arg_sign_value(cmd, physicalextentsize_ARG, 0) == SIGN_MINUS) {
		log_error("Physical extent size may not be negative");
		return 1;
	}

	if (arg_sign_value(cmd, maxlogicalvolumes_ARG, 0) == SIGN_MINUS) {
		log_error("Max Logical Volumes may not be negative");
		return 1;
	}

	if (arg_sign_value(cmd, maxphysicalvolumes_ARG, 0) == SIGN_MINUS) {
		log_error("Max Physical Volumes may not be negative");
		return 1;
	}

	return 0;
}

int lv_refresh(struct cmd_context *cmd, struct logical_volume *lv)
{
	return suspend_lv(cmd, lv) && resume_lv(cmd, lv);
}

int vg_refresh_visible(struct cmd_context *cmd, struct volume_group *vg)
{
	struct lv_list *lvl;
	int r = 1;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (lv_is_visible(lvl->lv))
			if (!lv_refresh(cmd, lvl->lv))
				r = 0;

	return r;
}

void lv_spawn_background_polling(struct cmd_context *cmd,
				 struct logical_volume *lv)
{
	const char *pvname;

	if ((lv->status & PVMOVE) &&
	    (pvname = get_pvmove_pvname_from_lv_mirr(lv))) {
		log_verbose("Spawning background pvmove process for %s",
			    pvname);
		pvmove_poll(cmd, pvname, 1);
	} else if ((lv->status & LOCKED) &&
	    (pvname = get_pvmove_pvname_from_lv(lv))) {
		log_verbose("Spawning background pvmove process for %s",
			    pvname);
		pvmove_poll(cmd, pvname, 1);
	}

	if (lv->status & CONVERTING) {
		log_verbose("Spawning background lvconvert process for %s",
			lv->name);
		lvconvert_poll(cmd, lv, 1);
	}
}

/*
 * Intial sanity checking of non-recovery related command-line arguments.
 *
 * Output arguments:
 * pp: structure allocated by caller, fields written / validated here
 */
int pvcreate_params_validate(struct cmd_context *cmd,
			     int argc, char **argv,
			     struct pvcreate_params *pp)
{
	if (!argc) {
		log_error("Please enter a physical volume path");
		return 0;
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
	    (arg_count(cmd, pvmetadatacopies_ARG) ||
	     arg_count(cmd, metadatasize_ARG)   ||
	     arg_count(cmd, dataalignment_ARG)  ||
	     arg_count(cmd, dataalignmentoffset_ARG))) {
		log_error("Metadata and data alignment parameters only "
			  "apply to text format.");
		return 0;
	}

	if (arg_count(cmd, pvmetadatacopies_ARG) &&
	    arg_int_value(cmd, pvmetadatacopies_ARG, -1) > 2) {
		log_error("Metadatacopies may only be 0, 1 or 2");
		return 0;
	}

	if (arg_count(cmd, zero_ARG))
		pp->zero = strcmp(arg_str_value(cmd, zero_ARG, "y"), "n");

	if (arg_sign_value(cmd, dataalignment_ARG, 0) == SIGN_MINUS) {
		log_error("Physical volume data alignment may not be negative");
		return 0;
	}
	pp->data_alignment = arg_uint64_value(cmd, dataalignment_ARG, UINT64_C(0));

	if (pp->data_alignment > ULONG_MAX) {
		log_error("Physical volume data alignment is too big.");
		return 0;
	}

	if (pp->data_alignment && pp->pe_start) {
		if (pp->pe_start % pp->data_alignment)
			log_warn("WARNING: Ignoring data alignment %" PRIu64
				 " incompatible with --restorefile value (%"
				 PRIu64").", pp->data_alignment, pp->pe_start);
		pp->data_alignment = 0;
	}

	if (arg_sign_value(cmd, dataalignmentoffset_ARG, 0) == SIGN_MINUS) {
		log_error("Physical volume data alignment offset may not be negative");
		return 0;
	}
	pp->data_alignment_offset = arg_uint64_value(cmd, dataalignmentoffset_ARG, UINT64_C(0));

	if (pp->data_alignment_offset > ULONG_MAX) {
		log_error("Physical volume data alignment offset is too big.");
		return 0;
	}

	if (pp->data_alignment_offset && pp->pe_start) {
		log_warn("WARNING: Ignoring data alignment offset %" PRIu64
			 " incompatible with --restorefile value (%"
			 PRIu64").", pp->data_alignment_offset, pp->pe_start);
		pp->data_alignment_offset = 0;
	}

	if (arg_sign_value(cmd, metadatasize_ARG, 0) == SIGN_MINUS) {
		log_error("Metadata size may not be negative");
		return 0;
	}

	pp->pvmetadatasize = arg_uint64_value(cmd, metadatasize_ARG, UINT64_C(0));
	if (!pp->pvmetadatasize)
		pp->pvmetadatasize = find_config_tree_int(cmd,
						 "metadata/pvmetadatasize",
						 DEFAULT_PVMETADATASIZE);

	pp->pvmetadatacopies = arg_int_value(cmd, pvmetadatacopies_ARG, -1);
	if (pp->pvmetadatacopies < 0)
		pp->pvmetadatacopies = find_config_tree_int(cmd,
						   "metadata/pvmetadatacopies",
						   DEFAULT_PVMETADATACOPIES);

	return 1;
}

