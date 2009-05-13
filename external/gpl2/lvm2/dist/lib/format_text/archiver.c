/*	$NetBSD: archiver.c,v 1.1.1.1.2.1 2009/05/13 18:52:42 jym Exp $	*/

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

#include "lib.h"
#include "archiver.h"
#include "format-text.h"
#include "lvm-file.h"
#include "lvm-string.h"
#include "lvmcache.h"
#include "toolcontext.h"

#include <unistd.h>

struct archive_params {
	int enabled;
	char *dir;
	unsigned int keep_days;
	unsigned int keep_number;
};

struct backup_params {
	int enabled;
	char *dir;
};

int archive_init(struct cmd_context *cmd, const char *dir,
		 unsigned int keep_days, unsigned int keep_min,
		 int enabled)
{
	if (!(cmd->archive_params = dm_pool_zalloc(cmd->libmem,
						sizeof(*cmd->archive_params)))) {
		log_error("archive_params alloc failed");
		return 0;
	}

	cmd->archive_params->dir = NULL;

	if (!*dir)
		return 1;

	if (!(cmd->archive_params->dir = dm_strdup(dir))) {
		log_error("Couldn't copy archive directory name.");
		return 0;
	}

	cmd->archive_params->keep_days = keep_days;
	cmd->archive_params->keep_number = keep_min;
	archive_enable(cmd, enabled);

	return 1;
}

void archive_exit(struct cmd_context *cmd)
{
	if (cmd->archive_params->dir)
		dm_free(cmd->archive_params->dir);
	memset(cmd->archive_params, 0, sizeof(*cmd->archive_params));
}

void archive_enable(struct cmd_context *cmd, int flag)
{
	cmd->archive_params->enabled = flag;
}

static char *_build_desc(struct dm_pool *mem, const char *line, int before)
{
	size_t len = strlen(line) + 32;
	char *buffer;

	if (!(buffer = dm_pool_zalloc(mem, strlen(line) + 32)))
		return_NULL;

	if (snprintf(buffer, len,
		     "Created %s executing '%s'",
		     before ? "*before*" : "*after*", line) < 0)
		return_NULL;

	return buffer;
}

static int __archive(struct volume_group *vg)
{
	char *desc;

	if (!(desc = _build_desc(vg->cmd->mem, vg->cmd->cmd_line, 1)))
		return_0;

	return archive_vg(vg, vg->cmd->archive_params->dir, desc,
			  vg->cmd->archive_params->keep_days,
			  vg->cmd->archive_params->keep_number);
}

int archive(struct volume_group *vg)
{
	if (!vg->cmd->archive_params->enabled || !vg->cmd->archive_params->dir)
		return 1;

	if (test_mode()) {
		log_verbose("Test mode: Skipping archiving of volume group.");
		return 1;
	}

	if (!dm_create_dir(vg->cmd->archive_params->dir))
		return 0;

	/* Trap a read-only file system */
	if ((access(vg->cmd->archive_params->dir, R_OK | W_OK | X_OK) == -1) &&
	     (errno == EROFS))
		return 0;

	log_verbose("Archiving volume group \"%s\" metadata (seqno %u).", vg->name,
		    vg->seqno);
	if (!__archive(vg)) {
		log_error("Volume group \"%s\" metadata archive failed.",
			  vg->name);
		return 0;
	}

	return 1;
}

int archive_display(struct cmd_context *cmd, const char *vg_name)
{
	int r1, r2;

	r1 = archive_list(cmd, cmd->archive_params->dir, vg_name);
	r2 = backup_list(cmd, cmd->backup_params->dir, vg_name);

	return r1 && r2;
}

int archive_display_file(struct cmd_context *cmd, const char *file)
{
	int r;

	r = archive_list_file(cmd, file);

	return r;
}

int backup_init(struct cmd_context *cmd, const char *dir,
		int enabled)
{
	if (!(cmd->backup_params = dm_pool_zalloc(cmd->libmem,
					       sizeof(*cmd->archive_params)))) {
		log_error("archive_params alloc failed");
		return 0;
	}

	cmd->backup_params->dir = NULL;
	if (!*dir)
		return 1;

	if (!(cmd->backup_params->dir = dm_strdup(dir))) {
		log_error("Couldn't copy backup directory name.");
		return 0;
	}
	backup_enable(cmd, enabled);

	return 1;
}

void backup_exit(struct cmd_context *cmd)
{
	if (cmd->backup_params->dir)
		dm_free(cmd->backup_params->dir);
	memset(cmd->backup_params, 0, sizeof(*cmd->backup_params));
}

void backup_enable(struct cmd_context *cmd, int flag)
{
	cmd->backup_params->enabled = flag;
}

static int __backup(struct volume_group *vg)
{
	char name[PATH_MAX];
	char *desc;

	if (!(desc = _build_desc(vg->cmd->mem, vg->cmd->cmd_line, 0)))
		return_0;

	if (dm_snprintf(name, sizeof(name), "%s/%s",
			 vg->cmd->backup_params->dir, vg->name) < 0) {
		log_error("Failed to generate volume group metadata backup "
			  "filename.");
		return 0;
	}

	return backup_to_file(name, desc, vg);
}

int backup(struct volume_group *vg)
{
	if (!vg->cmd->backup_params->enabled || !vg->cmd->backup_params->dir) {
		log_warn("WARNING: This metadata update is NOT backed up");
		return 1;
	}

	if (test_mode()) {
		log_verbose("Test mode: Skipping volume group backup.");
		return 1;
	}

	if (!dm_create_dir(vg->cmd->backup_params->dir))
		return 0;

	/* Trap a read-only file system */
	if ((access(vg->cmd->backup_params->dir, R_OK | W_OK | X_OK) == -1) &&
	    (errno == EROFS))
		return 0;

	if (!__backup(vg)) {
		log_error("Backup of volume group %s metadata failed.",
			  vg->name);
		return 0;
	}

	return 1;
}

int backup_remove(struct cmd_context *cmd, const char *vg_name)
{
	char path[PATH_MAX];

	if (dm_snprintf(path, sizeof(path), "%s/%s",
			 cmd->backup_params->dir, vg_name) < 0) {
		log_err("Failed to generate backup filename (for removal).");
		return 0;
	}

	/*
	 * Let this fail silently.
	 */
	unlink(path);
	return 1;
}

struct volume_group *backup_read_vg(struct cmd_context *cmd,
				    const char *vg_name, const char *file)
{
	struct volume_group *vg = NULL;
	struct format_instance *tf;
	struct metadata_area *mda;
	void *context;

	if (!(context = create_text_context(cmd, file,
					    cmd->cmd_line)) ||
	    !(tf = cmd->fmt_backup->ops->create_instance(cmd->fmt_backup, NULL,
							 NULL, context))) {
		log_error("Couldn't create text format object.");
		return NULL;
	}

	dm_list_iterate_items(mda, &tf->metadata_areas) {
		if (!(vg = mda->ops->vg_read(tf, vg_name, mda)))
			stack;
		break;
	}

	tf->fmt->ops->destroy_instance(tf);
	return vg;
}

/* ORPHAN and VG locks held before calling this */
int backup_restore_vg(struct cmd_context *cmd, struct volume_group *vg)
{
	struct pv_list *pvl;
	struct physical_volume *pv;
	struct lvmcache_info *info;

	/*
	 * FIXME: Check that the PVs referenced in the backup are
	 * not members of other existing VGs.
	 */

	/* Attempt to write out using currently active format */
	if (!(vg->fid = cmd->fmt->ops->create_instance(cmd->fmt, vg->name,
						       NULL, NULL))) {
		log_error("Failed to allocate format instance");
		return 0;
	}

	/* Add any metadata areas on the PVs */
	dm_list_iterate_items(pvl, &vg->pvs) {
		pv = pvl->pv;
		if (!(info = info_from_pvid(pv->dev->pvid, 0))) {
			log_error("PV %s missing from cache",
				  pv_dev_name(pv));
			return 0;
		}
		if (cmd->fmt != info->fmt) {
			log_error("PV %s is a different format (seqno %s)",
				  pv_dev_name(pv), info->fmt->name);
			return 0;
		}
		if (!vg->fid->fmt->ops->
		    pv_setup(vg->fid->fmt, UINT64_C(0), 0, 0, 0,
			     UINT64_C(0), &vg->fid->metadata_areas, pv, vg)) {
			log_error("Format-specific setup for %s failed",
				  pv_dev_name(pv));
			return 0;
		}
	}

	if (!vg_write(vg) || !vg_commit(vg))
		return_0;

	return 1;
}

/* ORPHAN and VG locks held before calling this */
int backup_restore_from_file(struct cmd_context *cmd, const char *vg_name,
			     const char *file)
{
	struct volume_group *vg;

	/*
	 * Read in the volume group from the text file.
	 */
	if (!(vg = backup_read_vg(cmd, vg_name, file)))
		return_0;

	return backup_restore_vg(cmd, vg);
}

int backup_restore(struct cmd_context *cmd, const char *vg_name)
{
	char path[PATH_MAX];

	if (dm_snprintf(path, sizeof(path), "%s/%s",
			 cmd->backup_params->dir, vg_name) < 0) {
		log_err("Failed to generate backup filename (for restore).");
		return 0;
	}

	return backup_restore_from_file(cmd, vg_name, path);
}

int backup_to_file(const char *file, const char *desc, struct volume_group *vg)
{
	int r = 0;
	struct format_instance *tf;
	struct metadata_area *mda;
	void *context;
	struct cmd_context *cmd;

	cmd = vg->cmd;

	log_verbose("Creating volume group backup \"%s\" (seqno %u).", file, vg->seqno);

	if (!(context = create_text_context(cmd, file, desc)) ||
	    !(tf = cmd->fmt_backup->ops->create_instance(cmd->fmt_backup, NULL,
							 NULL, context))) {
		log_error("Couldn't create backup object.");
		return 0;
	}

	/* Write and commit the metadata area */
	dm_list_iterate_items(mda, &tf->metadata_areas) {
		if (!(r = mda->ops->vg_write(tf, vg, mda))) {
			stack;
			continue;
		}
		if (mda->ops->vg_commit &&
		    !(r = mda->ops->vg_commit(tf, vg, mda))) {
			stack;
		}
	}

	tf->fmt->ops->destroy_instance(tf);
	return r;
}

/*
 * Update backup (and archive) if they're out-of-date or don't exist.
 */
void check_current_backup(struct volume_group *vg)
{
	char path[PATH_MAX];
	struct volume_group *vg_backup;

	if (vg->status & EXPORTED_VG)
		return;

	if (dm_snprintf(path, sizeof(path), "%s/%s",
			 vg->cmd->backup_params->dir, vg->name) < 0) {
		log_debug("Failed to generate backup filename.");
		return;
	}

	log_suppress(1);
	/* Up-to-date backup exists? */
	if ((vg_backup = backup_read_vg(vg->cmd, vg->name, path)) &&
	    (vg->seqno == vg_backup->seqno) &&
	    (id_equal(&vg->id, &vg_backup->id)))
		return;
	log_suppress(0);

	if (vg_backup)
		archive(vg_backup);
	archive(vg);
	backup(vg);
}
