/*	$NetBSD: reporter.c,v 1.1.1.1.2.1 2009/05/13 18:52:47 jym Exp $	*/

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
#include "report.h"

static int _vgs_single(struct cmd_context *cmd __attribute((unused)),
		       const char *vg_name, struct volume_group *vg,
		       int consistent __attribute((unused)), void *handle)
{
	if (!vg) {
		log_error("Volume group %s not found", vg_name);
		return ECMD_FAILED;
	}

	if (!report_object(handle, vg, NULL, NULL, NULL, NULL))
		return ECMD_FAILED;

	check_current_backup(vg);

	return ECMD_PROCESSED;
}

static int _lvs_single(struct cmd_context *cmd, struct logical_volume *lv,
		       void *handle)
{
	if (!arg_count(cmd, all_ARG) && !lv_is_displayable(lv))
		return ECMD_PROCESSED;

	if (!report_object(handle, lv->vg, lv, NULL, NULL, NULL))
		return ECMD_FAILED;

	return ECMD_PROCESSED;
}

static int _segs_single(struct cmd_context *cmd __attribute((unused)),
			struct lv_segment *seg, void *handle)
{
	if (!report_object(handle, seg->lv->vg, seg->lv, NULL, seg, NULL))
		return ECMD_FAILED;

	return ECMD_PROCESSED;
}

static int _pvsegs_sub_single(struct cmd_context *cmd __attribute((unused)),
			      struct volume_group *vg,
			      struct pv_segment *pvseg, void *handle)
{
	int ret = ECMD_PROCESSED;
	struct lv_segment *seg = pvseg->lvseg;

	struct logical_volume _free_logical_volume = {
		.vg = vg,
		.name = (char *) "",
	        .snapshot = NULL,
		.status = VISIBLE_LV,
		.major = -1,
		.minor = -1,
	};

	struct lv_segment _free_lv_segment = {
        	.lv = &_free_logical_volume,
        	.le = 0,
        	.status = 0,
        	.stripe_size = 0,
        	.area_count = 0,
        	.area_len = 0,
        	.origin = NULL,
        	.cow = NULL,
        	.chunk_size = 0,
        	.region_size = 0,
        	.extents_copied = 0,
        	.log_lv = NULL,
        	.areas = NULL,
	};

        _free_lv_segment.segtype = get_segtype_from_string(cmd, "free");
	_free_lv_segment.len = pvseg->len;
	dm_list_init(&_free_lv_segment.tags);
	dm_list_init(&_free_lv_segment.origin_list);
	dm_list_init(&_free_logical_volume.tags);
	dm_list_init(&_free_logical_volume.segments);
	dm_list_init(&_free_logical_volume.segs_using_this_lv);
	dm_list_init(&_free_logical_volume.snapshot_segs);

	if (!report_object(handle, vg, seg ? seg->lv : &_free_logical_volume, pvseg->pv,
			   seg ? : &_free_lv_segment, pvseg))
		ret = ECMD_FAILED;

	return ret;
}

static int _lvsegs_single(struct cmd_context *cmd, struct logical_volume *lv,
			  void *handle)
{
	if (!arg_count(cmd, all_ARG) && !lv_is_displayable(lv))
		return ECMD_PROCESSED;

	return process_each_segment_in_lv(cmd, lv, handle, _segs_single);
}

static int _pvsegs_single(struct cmd_context *cmd, struct volume_group *vg,
			  struct physical_volume *pv, void *handle)
{
	return process_each_segment_in_pv(cmd, vg, pv, handle,
					  _pvsegs_sub_single);
}

static int _pvs_single(struct cmd_context *cmd, struct volume_group *vg,
		       struct physical_volume *pv, void *handle)
{
	struct pv_list *pvl;
	int ret = ECMD_PROCESSED;
	const char *vg_name = NULL;

	if (is_pv(pv) && !is_orphan(pv) && !vg) {
		vg_name = pv_vg_name(pv);

		if (!(vg = vg_lock_and_read(cmd, vg_name, (char *)&pv->vgid,
					    LCK_VG_READ, CLUSTERED, 0))) {
			log_error("Skipping volume group %s", vg_name);
			return ECMD_FAILED;
		}

		/*
		 * Replace possibly incomplete PV structure with new one
		 * allocated in vg_read() path.
		*/
		if (!(pvl = find_pv_in_vg(vg, pv_dev_name(pv)))) {
			log_error("Unable to find \"%s\" in volume group \"%s\"",
				  pv_dev_name(pv), vg->name);
			ret = ECMD_FAILED;
			goto out;
		}

		 pv = pvl->pv;
	}

	if (!report_object(handle, vg, NULL, pv, NULL, NULL))
		ret = ECMD_FAILED;

out:
	if (vg_name)
		unlock_vg(cmd, vg_name);

	return ret;
}

static int _pvs_in_vg(struct cmd_context *cmd, const char *vg_name,
		      struct volume_group *vg,
		      int consistent __attribute((unused)),
		      void *handle)
{
	if (!vg) {
		log_error("Volume group %s not found", vg_name);
		return ECMD_FAILED;
	}

	return process_each_pv_in_vg(cmd, vg, NULL, handle, &_pvs_single);
}

static int _pvsegs_in_vg(struct cmd_context *cmd, const char *vg_name,
			 struct volume_group *vg,
			 int consistent __attribute((unused)),
			 void *handle)
{
	if (!vg) {
		log_error("Volume group %s not found", vg_name);
		return ECMD_FAILED;
	}

	return process_each_pv_in_vg(cmd, vg, NULL, handle, &_pvsegs_single);
}

static int _report(struct cmd_context *cmd, int argc, char **argv,
		   report_type_t report_type)
{
	void *report_handle;
	const char *opts;
	char *str;
	const char *keys = NULL, *options = NULL, *separator;
	int r = ECMD_PROCESSED;
	int aligned, buffered, headings, field_prefixes, quoted;
	int columns_as_rows;
	unsigned args_are_pvs;

	aligned = find_config_tree_int(cmd, "report/aligned",
				  DEFAULT_REP_ALIGNED);
	buffered = find_config_tree_int(cmd, "report/buffered",
				   DEFAULT_REP_BUFFERED);
	headings = find_config_tree_int(cmd, "report/headings",
				   DEFAULT_REP_HEADINGS);
	separator = find_config_tree_str(cmd, "report/separator",
				    DEFAULT_REP_SEPARATOR);
	field_prefixes = find_config_tree_int(cmd, "report/prefixes",
					      DEFAULT_REP_PREFIXES);
	quoted = find_config_tree_int(cmd, "report/quoted",
				     DEFAULT_REP_QUOTED);
	columns_as_rows = find_config_tree_int(cmd, "report/columns_as_rows",
					       DEFAULT_REP_COLUMNS_AS_ROWS);

	args_are_pvs = (report_type == PVS || report_type == PVSEGS) ? 1 : 0;

	switch (report_type) {
	case LVS:
		keys = find_config_tree_str(cmd, "report/lvs_sort",
				       DEFAULT_LVS_SORT);
		if (!arg_count(cmd, verbose_ARG))
			options = find_config_tree_str(cmd,
						  "report/lvs_cols",
						  DEFAULT_LVS_COLS);
		else
			options = find_config_tree_str(cmd,
						  "report/lvs_cols_verbose",
						  DEFAULT_LVS_COLS_VERB);
		break;
	case VGS:
		keys = find_config_tree_str(cmd, "report/vgs_sort",
				       DEFAULT_VGS_SORT);
		if (!arg_count(cmd, verbose_ARG))
			options = find_config_tree_str(cmd,
						  "report/vgs_cols",
						  DEFAULT_VGS_COLS);
		else
			options = find_config_tree_str(cmd,
						  "report/vgs_cols_verbose",
						  DEFAULT_VGS_COLS_VERB);
		break;
	case PVS:
		keys = find_config_tree_str(cmd, "report/pvs_sort",
				       DEFAULT_PVS_SORT);
		if (!arg_count(cmd, verbose_ARG))
			options = find_config_tree_str(cmd,
						  "report/pvs_cols",
						  DEFAULT_PVS_COLS);
		else
			options = find_config_tree_str(cmd,
						  "report/pvs_cols_verbose",
						  DEFAULT_PVS_COLS_VERB);
		break;
	case SEGS:
		keys = find_config_tree_str(cmd, "report/segs_sort",
				       DEFAULT_SEGS_SORT);
		if (!arg_count(cmd, verbose_ARG))
			options = find_config_tree_str(cmd,
						  "report/segs_cols",
						  DEFAULT_SEGS_COLS);
		else
			options = find_config_tree_str(cmd,
						  "report/segs_cols_verbose",
						  DEFAULT_SEGS_COLS_VERB);
		break;
	case PVSEGS:
		keys = find_config_tree_str(cmd, "report/pvsegs_sort",
				       DEFAULT_PVSEGS_SORT);
		if (!arg_count(cmd, verbose_ARG))
			options = find_config_tree_str(cmd,
						  "report/pvsegs_cols",
						  DEFAULT_PVSEGS_COLS);
		else
			options = find_config_tree_str(cmd,
						  "report/pvsegs_cols_verbose",
						  DEFAULT_PVSEGS_COLS_VERB);
		break;
	}

	/* If -o supplied use it, else use default for report_type */
	if (arg_count(cmd, options_ARG)) {
		opts = arg_str_value(cmd, options_ARG, "");
		if (!opts || !*opts) {
			log_error("Invalid options string: %s", opts);
			return EINVALID_CMD_LINE;
		}
		if (*opts == '+') {
			if (!(str = dm_pool_alloc(cmd->mem,
					 strlen(options) + strlen(opts) + 1))) {
				log_error("options string allocation failed");
				return ECMD_FAILED;
			}
			strcpy(str, options);
			strcat(str, ",");
			strcat(str, opts + 1);
			options = str;
		} else
			options = opts;
	}

	/* -O overrides default sort settings */
	if (arg_count(cmd, sort_ARG))
		keys = arg_str_value(cmd, sort_ARG, "");

	if (arg_count(cmd, separator_ARG))
		separator = arg_str_value(cmd, separator_ARG, " ");
	if (arg_count(cmd, separator_ARG))
		aligned = 0;
	if (arg_count(cmd, aligned_ARG))
		aligned = 1;
	if (arg_count(cmd, unbuffered_ARG) && !arg_count(cmd, sort_ARG))
		buffered = 0;
	if (arg_count(cmd, noheadings_ARG))
		headings = 0;
	if (arg_count(cmd, nameprefixes_ARG)) {
		aligned = 0;
		field_prefixes = 1;
	}
	if (arg_count(cmd, unquoted_ARG))
		quoted = 0;
	if (arg_count(cmd, rows_ARG))
		columns_as_rows = 1;

	if (!(report_handle = report_init(cmd, options, keys, &report_type,
					  separator, aligned, buffered,
					  headings, field_prefixes, quoted,
					  columns_as_rows))) {
		stack;
		return ECMD_FAILED;
	}

	/* Ensure options selected are compatible */
	if (report_type & SEGS)
		report_type |= LVS;
	if (report_type & PVSEGS)
		report_type |= PVS;
	if ((report_type & LVS) && (report_type & PVS) && !args_are_pvs) {
		log_error("Can't report LV and PV fields at the same time");
		dm_report_free(report_handle);
		return ECMD_FAILED;
	}

	/* Change report type if fields specified makes this necessary */
	if ((report_type & PVSEGS) ||
	    ((report_type & PVS) && (report_type & LVS)))
		report_type = PVSEGS;
	else if (report_type & PVS)
		report_type = PVS;
	else if (report_type & SEGS)
		report_type = SEGS;
	else if (report_type & LVS)
		report_type = LVS;

	switch (report_type) {
	case LVS:
		r = process_each_lv(cmd, argc, argv, LCK_VG_READ, report_handle,
				    &_lvs_single);
		break;
	case VGS:
		r = process_each_vg(cmd, argc, argv, LCK_VG_READ, 0,
				    report_handle, &_vgs_single);
		break;
	case PVS:
		if (args_are_pvs)
			r = process_each_pv(cmd, argc, argv, NULL, LCK_VG_READ,
					    report_handle, &_pvs_single);
		else
			r = process_each_vg(cmd, argc, argv, LCK_VG_READ, 0,
					    report_handle, &_pvs_in_vg);
		break;
	case SEGS:
		r = process_each_lv(cmd, argc, argv, LCK_VG_READ, report_handle,
				    &_lvsegs_single);
		break;
	case PVSEGS:
		if (args_are_pvs)
			r = process_each_pv(cmd, argc, argv, NULL, LCK_VG_READ,
					    report_handle, &_pvsegs_single);
		else
			r = process_each_vg(cmd, argc, argv, LCK_VG_READ, 0,
					    report_handle, &_pvsegs_in_vg);
		break;
	}

	dm_report_output(report_handle);

	dm_report_free(report_handle);
	return r;
}

int lvs(struct cmd_context *cmd, int argc, char **argv)
{
	report_type_t type;

	if (arg_count(cmd, segments_ARG))
		type = SEGS;
	else
		type = LVS;

	return _report(cmd, argc, argv, type);
}

int vgs(struct cmd_context *cmd, int argc, char **argv)
{
	return _report(cmd, argc, argv, VGS);
}

int pvs(struct cmd_context *cmd, int argc, char **argv)
{
	report_type_t type;

	if (arg_count(cmd, segments_ARG))
		type = PVSEGS;
	else
		type = PVS;

	return _report(cmd, argc, argv, type);
}
