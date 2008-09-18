/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
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
#include "metadata.h"
#include "import-export.h"
#include "display.h"
#include "toolcontext.h"
#include "lvmcache.h"

/* FIXME Use tidier inclusion method */
static struct text_vg_version_ops *(_text_vsn_list[2]);

static int _text_import_initialised = 0;

static void _init_text_import()
{
	if (_text_import_initialised)
		return;

	_text_vsn_list[0] = text_vg_vsn1_init();
	_text_vsn_list[1] = NULL;
	_text_import_initialised = 1;
}

const char *text_vgname_import(const struct format_type *fmt,
			       struct device *dev,
			       off_t offset, uint32_t size,
			       off_t offset2, uint32_t size2,
			       checksum_fn_t checksum_fn, uint32_t checksum,
			       struct id *vgid, uint32_t *vgstatus,
			       char **creation_host)
{
	struct config_tree *cft;
	struct text_vg_version_ops **vsn;
	const char *vgname = NULL;

	_init_text_import();

	if (!(cft = create_config_tree(NULL, 0)))
		return_NULL;

	if ((!dev && !read_config_file(cft)) ||
	    (dev && !read_config_fd(cft, dev, offset, size,
				    offset2, size2, checksum_fn, checksum)))
		goto_out;

	/*
	 * Find a set of version functions that can read this file
	 */
	for (vsn = &_text_vsn_list[0]; *vsn; vsn++) {
		if (!(*vsn)->check_version(cft))
			continue;

		if (!(vgname = (*vsn)->read_vgname(fmt, cft, vgid, vgstatus,
						   creation_host)))
			goto_out;

		break;
	}

      out:
	destroy_config_tree(cft);
	return vgname;
}

struct volume_group *text_vg_import_fd(struct format_instance *fid,
				       const char *file,
				       struct device *dev,
				       off_t offset, uint32_t size,
				       off_t offset2, uint32_t size2,
				       checksum_fn_t checksum_fn,
				       uint32_t checksum,
				       time_t *when, char **desc)
{
	struct volume_group *vg = NULL;
	struct config_tree *cft;
	struct text_vg_version_ops **vsn;

	_init_text_import();

	*desc = NULL;
	*when = 0;

	if (!(cft = create_config_tree(file, 0)))
		return_NULL;

	if ((!dev && !read_config_file(cft)) ||
	    (dev && !read_config_fd(cft, dev, offset, size,
				    offset2, size2, checksum_fn, checksum))) {
		log_error("Couldn't read volume group metadata.");
		goto out;
	}

	/*
	 * Find a set of version functions that can read this file
	 */
	for (vsn = &_text_vsn_list[0]; *vsn; vsn++) {
		if (!(*vsn)->check_version(cft))
			continue;

		if (!(vg = (*vsn)->read_vg(fid, cft)))
			goto_out;

		(*vsn)->read_desc(fid->fmt->cmd->mem, cft, when, desc);
		break;
	}

      out:
	destroy_config_tree(cft);
	return vg;
}

struct volume_group *text_vg_import_file(struct format_instance *fid,
					 const char *file,
					 time_t *when, char **desc)
{
	return text_vg_import_fd(fid, file, NULL, (off_t)0, 0, (off_t)0, 0, NULL, 0,
				 when, desc);
}

struct volume_group *import_vg_from_buffer(char *buf,
                                           struct format_instance *fid)
{
	struct volume_group *vg = NULL;
	struct config_tree *cft;
	struct text_vg_version_ops **vsn;

	_init_text_import();

	if (!(cft = create_config_tree_from_string(fid->fmt->cmd, buf)))
		return_NULL;

	for (vsn = &_text_vsn_list[0]; *vsn; vsn++) {
		if (!(*vsn)->check_version(cft))
			continue;
		if (!(vg = (*vsn)->read_vg(fid, cft)))
			stack;
		break;
	}

	destroy_config_tree(cft);
	return vg;
}
