/*	$NetBSD: libdm-file.c,v 1.1.1.1 2008/12/12 11:42:50 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"

#include <sys/file.h>
#include <fcntl.h>
#include <dirent.h>

static int _create_dir_recursive(const char *dir)
{
	char *orig, *s;
	int rc, r = 0;

	log_verbose("Creating directory \"%s\"", dir);
	/* Create parent directories */
	orig = s = dm_strdup(dir);
	while ((s = strchr(s, '/')) != NULL) {
		*s = '\0';
		if (*orig) {
			rc = mkdir(orig, 0777);
			if (rc < 0 && errno != EEXIST) {
				if (errno != EROFS)
					log_sys_error("mkdir", orig);
				goto out;
			}
		}
		*s++ = '/';
	}

	/* Create final directory */
	rc = mkdir(dir, 0777);
	if (rc < 0 && errno != EEXIST) {
		if (errno != EROFS)
			log_sys_error("mkdir", orig);
		goto out;
	}

	r = 1;
out:
	dm_free(orig);
	return r;
}

int dm_create_dir(const char *dir)
{
	struct stat info;

	if (!*dir)
		return 1;

	if (stat(dir, &info) < 0)
		return _create_dir_recursive(dir);

	if (S_ISDIR(info.st_mode))
		return 1;

	log_error("Directory \"%s\" not found", dir);
	return 0;
}

int dm_fclose(FILE *stream)
{
	int prev_fail = ferror(stream);
	int fclose_fail = fclose(stream);

	/* If there was a previous failure, but fclose succeeded,
	   clear errno, since ferror does not set it, and its value
	   may be unrelated to the ferror-reported failure.  */
	if (prev_fail && !fclose_fail)
		errno = 0;

	return prev_fail || fclose_fail ? EOF : 0;
}
