/*	$NetBSD: vgcfgbackup.c,v 1.1.1.2 2009/12/02 00:25:56 haad Exp $	*/

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

static char *_expand_filename(const char *template, const char *vg_name,
			      char **last_filename)
{
	char *filename;

	if (security_level())
		return dm_strdup(template);

	if (!(filename = dm_malloc(PATH_MAX))) {
		log_error("Failed to allocate filename.");
		return NULL;
	}

	if (snprintf(filename, PATH_MAX, template, vg_name) < 0) {
		log_error("Error processing filename template %s",
			   template);
		dm_free(filename);	
		return NULL;
	}
	if (*last_filename && !strncmp(*last_filename, filename, PATH_MAX)) {
		log_error("VGs must be backed up into different files. "
			  "Use %%s in filename for VG name.");
		dm_free(filename);
		return NULL;
	}

	dm_free(*last_filename);
	*last_filename = filename;

	return filename;
}

static int vg_backup_single(struct cmd_context *cmd, const char *vg_name,
			    struct volume_group *vg,
			    void *handle)
{
	char **last_filename = (char **)handle;
	char *filename;

	if (arg_count(cmd, file_ARG)) {
		if (!(filename = _expand_filename(arg_value(cmd, file_ARG),
						  vg->name, last_filename))) {
			stack;
			return ECMD_FAILED;
		}

		if (!backup_to_file(filename, vg->cmd->cmd_line, vg)) {
			stack;
			return ECMD_FAILED;
		}
	} else {
		if (vg_read_error(vg) == FAILED_INCONSISTENT) {
			log_error("No backup taken: specify filename with -f "
				  "to backup an inconsistent VG");
			stack;
			return ECMD_FAILED;
		}

		/* just use the normal backup code */
		backup_enable(cmd, 1);	/* force a backup */
		if (!backup(vg)) {
			stack;
			return ECMD_FAILED;
		}
	}

	log_print("Volume group \"%s\" successfully backed up.", vg_name);
	return ECMD_PROCESSED;
}

int vgcfgbackup(struct cmd_context *cmd, int argc, char **argv)
{
	int ret;
	char *last_filename = NULL;

	init_pvmove(1);

	ret = process_each_vg(cmd, argc, argv, READ_ALLOW_INCONSISTENT,
			      &last_filename, &vg_backup_single);

	dm_free(last_filename);

	init_pvmove(0);

	return ret;
}
