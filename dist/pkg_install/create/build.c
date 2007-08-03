/*	$NetBSD: build.c,v 1.1.1.1.2.2 2007/08/03 13:58:21 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.38 1997/10/13 15:03:51 jkh Exp";
#else
__RCSID("$NetBSD: build.c,v 1.1.1.1.2.2 2007/08/03 13:58:21 joerg Exp $");
#endif
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * This is the main body of the create module.
 *
 */

#include "lib.h"
#include "create.h"

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <pwd.h>
#include <grp.h>

#include <archive.h>
#include <archive_entry.h>

static struct memory_file *contents_file;
static struct memory_file *comment_file;
static struct memory_file *desc_file;
static struct memory_file *install_file;
static struct memory_file *deinstall_file;
static struct memory_file *display_file;
static struct memory_file *build_version_file;
static struct memory_file *build_info_file;
static struct memory_file *size_pkg_file;
static struct memory_file *size_all_file;
static struct memory_file *preserve_file;
static struct memory_file *views_file;

static void
write_meta_file(struct memory_file *file, struct archive *archive)
{
	struct archive_entry *entry;

	entry = archive_entry_new();
	archive_entry_set_pathname(entry, file->name);
	archive_entry_copy_stat(entry, &file->st);

	archive_entry_set_uname(entry, file->owner);
	archive_entry_set_gname(entry, file->group);

	if (archive_write_header(archive, entry))
		errx(2, "cannot write to archive: %s", archive_error_string(archive));

	archive_write_data(archive, file->data, file->len);

	archive_entry_free(entry);
}

LIST_HEAD(hardlink_list, hardlinked_entry);
struct hardlink_list written_hardlinks;

struct hardlinked_entry {
	LIST_ENTRY(hardlinked_entry) link;
	const char *existing_name;
	nlink_t remaining_links;
	dev_t existing_device;
	ino_t existing_ino;
};

static void
write_normal_file(const char *name, struct archive *archive, const char *owner, const char *group)
{
	char buf[16384];
	off_t len;
	ssize_t buf_len;
	struct hardlinked_entry *older_link;
	struct archive_entry *entry;
	struct stat st;
	int fd;

	if (lstat(name, &st) == -1)
		err(2, "lstat failed for file %s", name);

	entry = archive_entry_new();
	archive_entry_set_pathname(entry, name);

	if (!S_ISDIR(st.st_mode) && st.st_nlink > 1) {
		LIST_FOREACH(older_link, &written_hardlinks, link) {
			if (st.st_dev == older_link->existing_device &&
			    st.st_ino == older_link->existing_ino) {
				archive_entry_copy_hardlink(entry,
				    older_link->existing_name);
				if (archive_write_header(archive, entry)) {
					errx(2, "cannot write to archive: %s",
					    archive_error_string(archive));
				}

				if (--older_link->remaining_links > 0)
					return;
				LIST_REMOVE(older_link, link);
				free(older_link);
				return;
			}
		}
		/* Not yet linked */
		if ((older_link = malloc(sizeof(*older_link))) == NULL)
			err(2, "malloc failed");
		older_link->existing_name = name;
		older_link->remaining_links = st.st_nlink - 1;
		older_link->existing_device = st.st_dev;
		older_link->existing_ino = st.st_ino;
		LIST_INSERT_HEAD(&written_hardlinks, older_link, link);
	}

	archive_entry_copy_stat(entry, &st);

	if (owner != NULL) {
		uid_t uid;

		archive_entry_set_uname(entry, owner);
		if (uid_from_user(owner, &uid) == -1)
			errx(2, "user %s unknown", owner);
		archive_entry_set_uid(entry, uid);
	} else {
		archive_entry_set_uname(entry, user_from_uid(st.st_uid, 1));
	}

	if (group != NULL) {
		gid_t gid;

		archive_entry_set_gname(entry, group);
		if (gid_from_group(group, &gid) == -1)
			errx(2, "group %s unknown", group);
		archive_entry_set_gid(entry, gid);
	} else {
		archive_entry_set_gname(entry, group_from_gid(st.st_gid, 1));
	}

	switch (st.st_mode & S_IFMT) {
	case S_IFLNK:
		buf_len = readlink(name, buf, sizeof buf);
		if (buf_len < 0)
			err(2, "cannot read symlink %s", name);
		buf[buf_len] = '\0';
		archive_entry_set_symlink(entry, buf);

		if (archive_write_header(archive, entry))
			errx(2, "cannot write to archive: %s", archive_error_string(archive));

		break;

	case S_IFREG:
		fd = open(name, O_RDONLY);
		if (fd == -1)
			errx(2, "cannot open data file %s: %s", name, archive_error_string(archive));

		len = st.st_size;

		if (archive_write_header(archive, entry))
			errx(2, "cannot write to archive: %s", archive_error_string(archive));

		while (len > 0) {
			if (len > sizeof(buf))
				buf_len = sizeof(buf);
			else
				buf_len = (ssize_t)len;
			if ((buf_len = read(fd, buf, buf_len)) <= 0)
				break;
			archive_write_data(archive, buf, (size_t)buf_len);
			len -= buf_len;
		}

		close(fd);
		break;

	default:
		errx(2, "PLIST entry neither symlink nor directory: %s", name);
	}

	archive_entry_free(entry);
}

static void
make_dist(const char *pkg, const char *suffix, const package_t *plist)
{
	char *archive_name;
	const char *owner, *group;
	const plist_t *p;
	struct archive *archive;
	char *initial_cwd;
	
	archive = archive_write_new();
	archive_write_set_format_pax_restricted(archive);

	if (strcmp(suffix, "tbz") == 0 || strcmp(suffix, "tar.bz2") == 0)
		archive_write_set_compression_bzip2(archive);
	else if (strcmp(suffix, "tgz") == 0 || strcmp(suffix, "tar.gz") == 0)
		archive_write_set_compression_gzip(archive);
	else
		archive_write_set_compression_none(archive);

	if (asprintf(&archive_name, "%s.%s", pkg, suffix) == -1)
		err(2, "cannot compute output name");

	if (archive_write_open_file(archive, archive_name))
		errx(2, "cannot create archive: %s", archive_error_string(archive));

	free(archive_name);

	owner = DefaultOwner;
	group = DefaultGroup;

	write_meta_file(contents_file, archive);
	write_meta_file(comment_file, archive);
	write_meta_file(desc_file, archive);

	if (Install)
		write_meta_file(install_file, archive);
	if (DeInstall)
		write_meta_file(deinstall_file, archive);
	if (Display)
		write_meta_file(display_file, archive);
	if (BuildVersion)
		write_meta_file(build_version_file, archive);
	if (BuildInfo)
		write_meta_file(build_info_file, archive);
	if (SizePkg)
		write_meta_file(size_pkg_file, archive);
	if (SizeAll)
		write_meta_file(size_all_file, archive);
	if (Preserve)
		write_meta_file(preserve_file, archive);
	if (create_views)
		write_meta_file(views_file, archive);

	initial_cwd = getcwd(NULL, 0);

	for (p = plist->head; p; p = p->next) {
		if (p->type == PLIST_FILE) {
			write_normal_file(p->name, archive, owner, group);
		} else if (p->type == PLIST_CWD || p->type == PLIST_SRC) {
			
			/* XXX let PLIST_SRC override PLIST_CWD */
			if (p->type == PLIST_CWD && p->next != NULL &&
			    p->next->type == PLIST_SRC) {
				continue;
			}
			chdir(p->name);
		} else if (p->type == PLIST_IGNORE) {
			p = p->next;
		} else if (p->type == PLIST_CHOWN) {
			if (p->name != NULL)
				owner = p->name;
			else
				owner = DefaultOwner;
		} else if (p->type == PLIST_CHGRP) {
			if (p->name != NULL)
				group = p->name;
			else
				group = DefaultGroup;
		}
	}
	chdir(initial_cwd);
	free(initial_cwd);

	if (archive_write_close(archive))
		errx(2, "cannot finish archive: %s", archive_error_string(archive));
	archive_write_finish(archive);
}

static struct memory_file *
load_and_add(package_t *plist, const char *input_name,
    const char *target_name, mode_t perm)
{
	struct memory_file *file;

	file = load_memory_file(input_name, target_name, DefaultOwner,
	    DefaultGroup, perm);
	add_plist(plist, PLIST_IGNORE, NULL);
	add_plist(plist, PLIST_FILE, target_name);

	return file;
}

static struct memory_file *
make_and_add(package_t *plist, const char *target_name,
    char *content, mode_t perm)
{
	struct memory_file *file;

	file = make_memory_file(target_name, content, strlen(content),
	    DefaultOwner, DefaultGroup, perm);
	add_plist(plist, PLIST_IGNORE, NULL);
	add_plist(plist, PLIST_FILE, target_name);

	return file;
}

int
pkg_build(const char *pkg, const char *full_pkg, const char *suffix,
    package_t *plist)
{
	char *plist_buf;
	size_t plist_len;

	/* Now put the release specific items in */
	add_plist(plist, PLIST_CWD, ".");
	comment_file = make_and_add(plist, COMMENT_FNAME, Comment, 0444);
	desc_file = make_and_add(plist, DESC_FNAME, Desc, 0444);

	if (Install) {
		install_file = load_and_add(plist, Install, INSTALL_FNAME,
		    0555);
	}
	if (DeInstall) {
		deinstall_file = load_and_add(plist, DeInstall,
		    DEINSTALL_FNAME, 0555);
	}
	if (Display) {
		display_file = load_and_add(plist, Display,
		    DISPLAY_FNAME, 0444);
		add_plist(plist, PLIST_DISPLAY, DISPLAY_FNAME);
	}
	if (BuildVersion) {
		build_version_file = load_and_add(plist, BuildVersion,
		    BUILD_VERSION_FNAME, 0444);
	}
	if (BuildInfo) {
		build_info_file = load_and_add(plist, BuildInfo,
		    BUILD_INFO_FNAME, 0444);
	}
	if (SizePkg) {
		size_pkg_file = load_and_add(plist, SizePkg,
		    SIZE_PKG_FNAME, 0444);
	}
	if (SizeAll) {
		size_all_file = load_and_add(plist, SizeAll,
		    SIZE_ALL_FNAME, 0444);
	}
	if (Preserve) {
		preserve_file = load_and_add(plist, Preserve,
		    PRESERVE_FNAME, 0444);
	}
	if (create_views)
		views_file = make_and_add(plist, VIEWS_FNAME, "", 0444);

	/* Finally, write out the packing list */
	stringify_plist(plist, &plist_buf, &plist_len, realprefix);
	contents_file = make_memory_file(CONTENTS_FNAME, plist_buf, plist_len,
	    DefaultOwner, DefaultGroup, 0644);

	/* And stick it into a tar ball */
	make_dist(pkg, suffix, plist);

	return TRUE;		/* Success */
}
