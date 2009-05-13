/*	$NetBSD: archive.c,v 1.1.1.1.2.1 2009/05/13 18:52:42 jym Exp $	*/

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
#include "format-text.h"

#include "config.h"
#include "import-export.h"
#include "lvm-string.h"
#include "lvm-file.h"
#include "toolcontext.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>

#define SECS_PER_DAY 86400	/* 24*60*60 */

/*
 * The format instance is given a directory path upon creation.
 * Each file in this directory whose name is of the form
 * '(.*)_[0-9]*.vg' is a config file (see lib/config.[hc]), which
 * contains a description of a single volume group.
 *
 * The prefix ($1 from the above regex) of the config file gives
 * the volume group name.
 *
 * Backup files that have expired will be removed.
 */

/*
 * A list of these is built up for our volume group.  Ordered
 * with the least recent at the head.
 */
struct archive_file {
	struct dm_list list;

	char *path;
	uint32_t index;
};

/*
 * Extract vg name and version number from a filename.
 */
static int _split_vg(const char *filename, char *vgname, size_t vg_size,
		     uint32_t *ix)
{
	size_t len, vg_len;
	const char *dot, *underscore;

	len = strlen(filename);
	if (len < 7)
		return 0;

	dot = (filename + len - 3);
	if (strcmp(".vg", dot))
		return 0;

	if (!(underscore = strrchr(filename, '_')))
		return 0;

	if (sscanf(underscore + 1, "%u", ix) != 1)
		return 0;

	vg_len = underscore - filename;
	if (vg_len + 1 > vg_size)
		return 0;

	strncpy(vgname, filename, vg_len);
	vgname[vg_len] = '\0';

	return 1;
}

static void _insert_archive_file(struct dm_list *head, struct archive_file *b)
{
	struct archive_file *bf = NULL;

	if (dm_list_empty(head)) {
		dm_list_add(head, &b->list);
		return;
	}

	/* index reduces through list */
	dm_list_iterate_items(bf, head) {
		if (b->index > bf->index) {
			dm_list_add(&bf->list, &b->list);
			return;
		}
	}

	dm_list_add_h(&bf->list, &b->list);
}

static char *_join_file_to_dir(struct dm_pool *mem, const char *dir, const char *name)
{
	if (!dm_pool_begin_object(mem, 32) ||
	    !dm_pool_grow_object(mem, dir, strlen(dir)) ||
	    !dm_pool_grow_object(mem, "/", 1) ||
	    !dm_pool_grow_object(mem, name, strlen(name)) ||
	    !dm_pool_grow_object(mem, "\0", 1))
		return_NULL;

	return dm_pool_end_object(mem);
}

/*
 * Returns a list of archive_files.
 */
static struct dm_list *_scan_archive(struct dm_pool *mem,
				  const char *vgname, const char *dir)
{
	int i, count;
	uint32_t ix;
	char vgname_found[64], *path;
	struct dirent **dirent;
	struct archive_file *af;
	struct dm_list *results;

	if (!(results = dm_pool_alloc(mem, sizeof(*results))))
		return_NULL;

	dm_list_init(results);

	/* Sort fails beyond 5-digit indexes */
	if ((count = scandir(dir, &dirent, NULL, alphasort)) < 0) {
		log_err("Couldn't scan the archive directory (%s).", dir);
		return 0;
	}

	for (i = 0; i < count; i++) {
		if (!strcmp(dirent[i]->d_name, ".") ||
		    !strcmp(dirent[i]->d_name, ".."))
			continue;

		/* check the name is the correct format */
		if (!_split_vg(dirent[i]->d_name, vgname_found,
			       sizeof(vgname_found), &ix))
			continue;

		/* is it the vg we're interested in ? */
		if (strcmp(vgname, vgname_found))
			continue;

		if (!(path = _join_file_to_dir(mem, dir, dirent[i]->d_name)))
			goto_out;

		/*
		 * Create a new archive_file.
		 */
		if (!(af = dm_pool_alloc(mem, sizeof(*af)))) {
			log_err("Couldn't create new archive file.");
			results = NULL;
			goto out;
		}

		af->index = ix;
		af->path = path;

		/*
		 * Insert it to the correct part of the list.
		 */
		_insert_archive_file(results, af);
	}

      out:
	for (i = 0; i < count; i++)
		free(dirent[i]);
	free(dirent);

	return results;
}

static void _remove_expired(struct dm_list *archives, uint32_t archives_size,
			    uint32_t retain_days, uint32_t min_archive)
{
	struct archive_file *bf;
	struct stat sb;
	time_t retain_time;

	/* Make sure there are enough archives to even bother looking for
	 * expired ones... */
	if (archives_size <= min_archive)
		return;

	/* Convert retain_days into the time after which we must retain */
	retain_time = time(NULL) - (time_t) retain_days *SECS_PER_DAY;

	/* Assume list is ordered newest first (by index) */
	dm_list_iterate_back_items(bf, archives) {
		/* Get the mtime of the file and unlink if too old */
		if (stat(bf->path, &sb)) {
			log_sys_error("stat", bf->path);
			continue;
		}

		if (sb.st_mtime > retain_time)
			return;

		log_very_verbose("Expiring archive %s", bf->path);
		if (unlink(bf->path))
			log_sys_error("unlink", bf->path);

		/* Don't delete any more if we've reached the minimum */
		if (--archives_size <= min_archive)
			return;
	}
}

int archive_vg(struct volume_group *vg,
	       const char *dir, const char *desc,
	       uint32_t retain_days, uint32_t min_archive)
{
	int i, fd, renamed = 0;
	uint32_t ix = 0;
	struct archive_file *last;
	FILE *fp = NULL;
	char temp_file[PATH_MAX], archive_name[PATH_MAX];
	struct dm_list *archives;

	/*
	 * Write the vg out to a temporary file.
	 */
	if (!create_temp_name(dir, temp_file, sizeof(temp_file), &fd,
			      &vg->cmd->rand_seed)) {
		log_err("Couldn't create temporary archive name.");
		return 0;
	}

	if (!(fp = fdopen(fd, "w"))) {
		log_err("Couldn't create FILE object for archive.");
		if (close(fd))
			log_sys_error("close", temp_file);
		return 0;
	}

	if (!text_vg_export_file(vg, desc, fp)) {
		if (fclose(fp))
			log_sys_error("fclose", temp_file);
		return_0;
	}

	if (lvm_fclose(fp, temp_file))
		return_0; /* Leave file behind as evidence of failure */

	/*
	 * Now we want to rename this file to <vg>_index.vg.
	 */
	if (!(archives = _scan_archive(vg->cmd->mem, vg->name, dir)))
		return_0;

	if (dm_list_empty(archives))
		ix = 0;
	else {
		last = dm_list_item(dm_list_first(archives), struct archive_file);
		ix = last->index + 1;
	}

	for (i = 0; i < 10; i++) {
		if (dm_snprintf(archive_name, sizeof(archive_name),
				 "%s/%s_%05u.vg", dir, vg->name, ix) < 0) {
			log_error("Archive file name too long.");
			return 0;
		}

		if ((renamed = lvm_rename(temp_file, archive_name)))
			break;

		ix++;
	}

	if (!renamed)
		log_error("Archive rename failed for %s", temp_file);

	_remove_expired(archives, dm_list_size(archives) + renamed, retain_days,
			min_archive);

	return 1;
}

static void _display_archive(struct cmd_context *cmd, struct archive_file *af)
{
	struct volume_group *vg = NULL;
	struct format_instance *tf;
	time_t when;
	char *desc;
	void *context;

	log_print(" ");
	log_print("File:\t\t%s", af->path);

	if (!(context = create_text_context(cmd, af->path, NULL)) ||
	    !(tf = cmd->fmt_backup->ops->create_instance(cmd->fmt_backup, NULL,
							 NULL, context))) {
		log_error("Couldn't create text instance object.");
		return;
	}

	/*
	 * Read the archive file to ensure that it is valid, and
	 * retrieve the archive time and description.
	 */
	/* FIXME Use variation on _vg_read */
	if (!(vg = text_vg_import_file(tf, af->path, &when, &desc))) {
		log_print("Unable to read archive file.");
		tf->fmt->ops->destroy_instance(tf);
		return;
	}

	log_print("VG name:    \t%s", vg->name);
	log_print("Description:\t%s", desc ? : "<No description>");
	log_print("Backup Time:\t%s", ctime(&when));

	dm_pool_free(cmd->mem, vg);
	tf->fmt->ops->destroy_instance(tf);
}

int archive_list(struct cmd_context *cmd, const char *dir, const char *vgname)
{
	struct dm_list *archives;
	struct archive_file *af;

	if (!(archives = _scan_archive(cmd->mem, vgname, dir)))
		return_0;

	if (dm_list_empty(archives))
		log_print("No archives found in %s.", dir);

	dm_list_iterate_back_items(af, archives)
		_display_archive(cmd, af);

	dm_pool_free(cmd->mem, archives);

	return 1;
}

int archive_list_file(struct cmd_context *cmd, const char *file)
{
	struct archive_file af;

	af.path = (char *)file;

	if (!path_exists(af.path)) {
		log_err("Archive file %s not found.", af.path);
		return 0;
	}

	_display_archive(cmd, &af);

	return 1;
}

int backup_list(struct cmd_context *cmd, const char *dir, const char *vgname)
{
	struct archive_file af;

	if (!(af.path = _join_file_to_dir(cmd->mem, dir, vgname)))
		return_0;

	if (path_exists(af.path))
		_display_archive(cmd, &af);

	return 1;
}
