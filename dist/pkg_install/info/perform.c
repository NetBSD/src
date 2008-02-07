/*	$NetBSD: perform.c,v 1.1.1.5 2008/02/07 23:42:16 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.23 1997/10/13 15:03:53 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.1.1.5 2008/02/07 23:42:16 joerg Exp $");
#endif
#endif

/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
 * 23 Aug 1993
 *
 * This is the main body of the info module.
 *
 */

#include "lib.h"
#include "info.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifndef BOOTSTRAP
#include <archive.h>
#include <archive_entry.h>
#endif
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#include <stddef.h>

#define	LOAD_CONTENTS		(1 << 0)
#define	LOAD_COMMENT		(1 << 1)
#define	LOAD_DESC		(1 << 2)
#define	LOAD_INSTALL		(1 << 3)
#define	LOAD_DEINSTALL		(1 << 4)
#define	LOAD_DISPLAY		(1 << 5)
#define	LOAD_MTREE		(1 << 6)
#define	LOAD_BUILD_VERSION	(1 << 7)
#define	LOAD_BUILD_INFO		(1 << 8)
#define	LOAD_SIZE_PKG		(1 << 9)
#define	LOAD_SIZE_ALL		(1 << 10)
#define	LOAD_PRESERVE		(1 << 11)
#define	LOAD_VIEWS		(1 << 12)
#define	LOAD_REQUIRED_BY	(1 << 13)
#define	LOAD_INSTALLED_INFO	(1 << 14)

static const struct pkg_meta_desc {
	size_t entry_offset;
	const char *entry_filename;
	int entry_mask;
} pkg_meta_descriptors[] = {
	{ offsetof(struct pkg_meta, meta_contents), CONTENTS_FNAME ,
	    LOAD_CONTENTS},
	{ offsetof(struct pkg_meta, meta_comment), COMMENT_FNAME,
	    LOAD_COMMENT },
	{ offsetof(struct pkg_meta, meta_desc), DESC_FNAME,
	    LOAD_DESC },
	{ offsetof(struct pkg_meta, meta_install), INSTALL_FNAME,
	    LOAD_INSTALL },
	{ offsetof(struct pkg_meta, meta_deinstall), DEINSTALL_FNAME,
	    LOAD_DEINSTALL },
	{ offsetof(struct pkg_meta, meta_display), DISPLAY_FNAME,
	    LOAD_DISPLAY },
	{ offsetof(struct pkg_meta, meta_mtree), MTREE_FNAME,
	    LOAD_MTREE },
	{ offsetof(struct pkg_meta, meta_build_version), BUILD_VERSION_FNAME,
	    LOAD_BUILD_VERSION },
	{ offsetof(struct pkg_meta, meta_build_info), BUILD_INFO_FNAME,
	    LOAD_BUILD_INFO },
	{ offsetof(struct pkg_meta, meta_size_pkg), SIZE_PKG_FNAME,
	    LOAD_SIZE_PKG },
	{ offsetof(struct pkg_meta, meta_size_all), SIZE_ALL_FNAME,
	    LOAD_SIZE_ALL },
	{ offsetof(struct pkg_meta, meta_preserve), PRESERVE_FNAME,
	    LOAD_PRESERVE },
	{ offsetof(struct pkg_meta, meta_views), VIEWS_FNAME,
	    LOAD_VIEWS },
	{ offsetof(struct pkg_meta, meta_required_by), REQUIRED_BY_FNAME,
	    LOAD_REQUIRED_BY },
	{ offsetof(struct pkg_meta, meta_installed_info), INSTALLED_INFO_FNAME,
	    LOAD_INSTALLED_INFO },
	{ 0, NULL, 0 },
};

static int desired_meta_data;

static void
free_pkg_meta(struct pkg_meta *meta)
{
	const struct pkg_meta_desc *descr;

	for (descr = pkg_meta_descriptors; descr->entry_filename; ++descr)
		free(*(char **)((char *)meta + descr->entry_offset));

	free(meta);
}

static struct pkg_meta *
read_meta_data_from_fd(int fd)
{
#ifdef BOOTSTRAP
	err(2, "binary packages not supported during bootstrap");
	return NULL;
#else
	struct archive *archive;
	struct pkg_meta *meta;
	struct archive_entry *entry;
	const char *fname;
	const struct pkg_meta_desc *descr, *last_descr;
	char **target;
	int64_t size;
	int r;

	archive = archive_read_new();
	archive_read_support_compression_all(archive);
	archive_read_support_format_all(archive);
	if (archive_read_open_fd(archive, fd, 1024))
		err(2, "cannot open archive: %s", archive_error_string(archive));

	if ((meta = malloc(sizeof(*meta))) == NULL)
		err(2, "cannot allocate meta data header");

	memset(meta, 0, sizeof(*meta));

	last_descr = 0;
	while ((r = archive_read_next_header(archive, &entry)) == ARCHIVE_OK) {
		fname = archive_entry_pathname(entry);

		for (descr = pkg_meta_descriptors; descr->entry_filename;
		     ++descr) {
			if (strcmp(descr->entry_filename, fname) == 0)
				break;
		}
		if (descr->entry_filename == NULL)
			break;

		target = (char **)((char *)meta + descr->entry_offset);
		if (*target)
			errx(2, "duplicate entry, package corrupt");
		if (descr < last_descr)
			warnx("misordered package, continuing");
		else
			last_descr = descr;

		if ((descr->entry_mask & desired_meta_data) == 0) {
			if (archive_read_data_skip(archive))
				errx(2, "cannot read package meta data");
			continue;
		}

		size = archive_entry_size(entry);
		if (size > SSIZE_MAX - 1)
			errx(2, "package meta data too large to process");
		if ((*target = malloc(size + 1)) == NULL)
			err(2, "cannot allocate meta data");
		if (archive_read_data(archive, *target, size) != size)
			errx(2, "cannot read package meta data");
		(*target)[size] = '\0';
	}

	return meta;
#endif
}

static struct pkg_meta *
read_meta_data_from_pkgdb(const char *pkg)
{
	struct pkg_meta *meta;
	const struct pkg_meta_desc *descr;
	char **target;
	char *fname;
	int fd;
	struct stat st;

	if ((meta = malloc(sizeof(*meta))) == NULL)
		err(2, "cannot allocate meta data header");

	memset(meta, 0, sizeof(*meta));

	for (descr = pkg_meta_descriptors; descr->entry_filename; ++descr) {
		if ((descr->entry_mask & desired_meta_data) == 0)
			continue;

		fname = pkgdb_pkg_file(pkg, descr->entry_filename);
		fd = open(fname, O_RDONLY, 0);
		free(fname);
		if (fd == -1) {
			if (errno == ENOENT)
				continue;
			err(2, "cannot read meta data file %s",
			    descr->entry_filename);
		}	
		target = (char **)((char *)meta + descr->entry_offset);

		if (fstat(fd, &st) == -1)
			err(2, "cannot stat meta data");
		if ((st.st_mode & S_IFMT) != S_IFREG)
			errx(1, "meta data is not regular file");
		if (st.st_size > SSIZE_MAX - 1)
			err(2, "meta data file too large to process");
		if ((*target = malloc(st.st_size + 1)) == NULL)
			err(2, "cannot allocate meta data");
		if (read(fd, *target, st.st_size) != st.st_size)
			err(2, "cannot read meta data");
		(*target)[st.st_size] = '\0';
		close(fd);
	}

	return meta;
}

static lfile_head_t files;

static void
fetch_child(int fd, const char *url)
{
	close(STDOUT_FILENO);
	if (dup2(fd, STDOUT_FILENO) == -1) {
		static const char err_msg[] =
		    "cannot redirect stdout of FTP process\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
		_exit(255);
	}
	close(fd);
	execlp(FTP_CMD, FTP_CMD, "-V", "-o", "-", url, (char *)NULL);
	_exit(255);
}

static int
pkg_do(const char *pkg)
{
	struct pkg_meta *meta;
	char    log_dir[MaxPathSize];
	int     code = 0;
	const char   *binpkgfile = NULL;

	if (IS_URL(pkg)) {
		pid_t child;
		int fd[2], status;

		if (pipe(fd) == -1)
			err(EXIT_FAILURE, "cannot create input pipes");
		if (Verbose)
			fprintf(stderr, "ftp -V -o - %s\n", pkg);
		child = vfork();
		if (child == -1)
			err(EXIT_FAILURE, "cannot fork FTP process");
		if (child == 0) {
			close(fd[0]);
			fetch_child(fd[1], pkg);
		}
		close(fd[1]);
		meta = read_meta_data_from_fd(fd[0]);
		kill(child, SIGTERM);
		close(fd[0]);
		waitpid(child, &status, 0);
	} else if (fexists(pkg) && isfile(pkg)) {
		int	pkg_fd;

		if ((pkg_fd = open(pkg, O_RDONLY, 0)) == -1)
			err(EXIT_FAILURE, "cannot open package %s", pkg);
		meta = read_meta_data_from_fd(pkg_fd);
		close(pkg_fd);
		binpkgfile = pkg;
	} else {
		/*
	         * It's not an uninstalled package, try and find it among the
	         * installed
	         */
		(void) snprintf(log_dir, sizeof(log_dir), "%s/%s",
		    _pkgdb_getPKGDB_DIR(), pkg);
		if (!fexists(log_dir) || !(isdir(log_dir) || islinktodir(log_dir))) {
			switch (add_installed_pkgs_by_basename(pkg, &pkgs)) {
			case 1:
				return 0;
			case 0:
				/* No match */
				warnx("can't find package `%s'", pkg);
				return 1;
			case -1:
				errx(EXIT_FAILURE, "Error during search in pkgdb for %s", pkg);
			}
		}
		meta = read_meta_data_from_pkgdb(pkg);
	}

	/*
         * Index is special info type that has to override all others to make
         * any sense.
         */
	if (Flags & SHOW_INDEX) {
		char    tmp[MaxPathSize];

		(void) snprintf(tmp, sizeof(tmp), "%-19s ", pkg);
		show_index(meta->meta_comment, tmp);
	} else if (Flags & SHOW_BI_VAR) {
		if (strcspn(BuildInfoVariable, "ABCDEFGHIJKLMNOPQRSTUVWXYZ")
		    == strlen(BuildInfoVariable))
			show_var(meta->meta_installed_info, BuildInfoVariable);
		else
			show_var(meta->meta_build_info, BuildInfoVariable);
	} else {
		package_t plist;
		
		/* Read the contents list */
		plist.head = plist.tail = NULL;
		parse_plist(&plist, meta->meta_contents);

		/* Start showing the package contents */
		if (!Quiet && !(Flags & SHOW_SUMMARY)) {
			printf("%sInformation for %s:\n\n", InfoPrefix, pkg);
			if (meta->meta_preserve) {
				printf("*** PACKAGE MAY NOT BE DELETED ***\n");
			}
		}
		if (Flags & SHOW_SUMMARY) {
			show_summary(meta, &plist, binpkgfile);
		}
		if (Flags & SHOW_COMMENT) {
			show_file(meta->meta_comment, "Comment:\n", TRUE);
		}
		if (Flags & SHOW_DEPENDS) {
			show_depends("Requires:\n", &plist);
		}
		if (Flags & SHOW_BLD_DEPENDS) {
			show_bld_depends("Built using:\n", &plist);
		}
		if ((Flags & SHOW_REQBY) && meta->meta_required_by) {
			show_file(meta->meta_required_by, "Required by:\n", TRUE);
		}
		if (Flags & SHOW_DESC) {
			show_file(meta->meta_desc, "Description:\n", TRUE);
		}
		if ((Flags & SHOW_DISPLAY) && meta->meta_display) {
			show_file(meta->meta_display, "Install notice:\n",
				  TRUE);
		}
		if (Flags & SHOW_PLIST) {
			show_plist("Packing list:\n", &plist, PLIST_SHOW_ALL);
		}
		if ((Flags & SHOW_INSTALL) && meta->meta_install) {
			show_file(meta->meta_install, "Install script:\n",
				  TRUE);
		}
		if ((Flags & SHOW_DEINSTALL) && meta->meta_deinstall) {
			show_file(meta->meta_deinstall, "De-Install script:\n",
				  TRUE);
		}
		if ((Flags & SHOW_MTREE) && meta->meta_mtree) {
			show_file(meta->meta_mtree, "mtree file:\n", TRUE);
		}
		if (Flags & SHOW_PREFIX) {
			show_plist("Prefix(s):\n", &plist, PLIST_CWD);
		}
		if (Flags & SHOW_FILES) {
			show_files("Files:\n", &plist);
		}
		if ((Flags & SHOW_BUILD_VERSION) && meta->meta_build_version) {
			show_file(meta->meta_build_version, "Build version:\n",
				  TRUE);
		}
		if (Flags & SHOW_BUILD_INFO) {
			if (meta->meta_build_info) {
				show_file(meta->meta_build_info, "Build information:\n",
					  TRUE);
			}
			if (meta->meta_installed_info) {
				show_file(meta->meta_installed_info, "Installed information:\n",
					  TRUE);
			}
		}
		if ((Flags & SHOW_PKG_SIZE) && meta->meta_size_pkg) {
			show_file(meta->meta_size_pkg, "Size of this package in bytes: ",
				  TRUE);
		}
		if ((Flags & SHOW_ALL_SIZE) && meta->meta_size_all) {
			show_file(meta->meta_size_all, "Size in bytes including required pkgs: ",
				  TRUE);
		}
		if (!Quiet && !(Flags & SHOW_SUMMARY)) {
			if (meta->meta_preserve) {
				printf("*** PACKAGE MAY NOT BE DELETED ***\n\n");
			}
			puts(InfoPrefix);
		}
		free_plist(&plist);
	}
	free_pkg_meta(meta);
	return code;
}

struct print_matching_arg {
	const char *pattern;
	int got_match;
};

static int
print_matching_pkg(const char *pkgname, void *cookie)
{
	struct print_matching_arg *arg= cookie;

	if (pkg_match(arg->pattern, pkgname)) {
		if (!Quiet)
			puts(pkgname);
		arg->got_match = 1;
	}

	return 0;
}

/*
 * Returns 0 if at least one package matching pkgname.
 * Returns 1 otherwise.
 *
 * If -q was not specified, print all matching packages to stdout.
 */
int
CheckForPkg(const char *pkgname)
{
	struct print_matching_arg arg;

	arg.pattern = pkgname;
	arg.got_match = 0;

	if (iterate_pkg_db(print_matching_pkg, &arg) == -1) {
		warnx("cannot iterate pkgdb");
		return 1;
	}

	if (arg.got_match == 0 && !ispkgpattern(pkgname)) {
		char *pattern;

		if (asprintf(&pattern, "%s-[0-9]*", pkgname) == -1)
			errx(EXIT_FAILURE, "asprintf failed");

		arg.pattern = pattern;
		arg.got_match = 0;

		if (iterate_pkg_db(print_matching_pkg, &arg) == -1) {
			free(pattern);
			warnx("cannot iterate pkgdb");
			return 1;
		}
		free(pattern);
	}

	if (arg.got_match)
		return 0;
	else
		return 1;
}

/*
 * Returns 0 if at least one package matching pkgname.
 * Returns 1 otherwise.
 *
 * If -q was not specified, print best match to stdout.
 */
int
CheckForBestPkg(const char *pkgname)
{
	char *pattern, *best_match;

	best_match = find_best_matching_installed_pkg(pkgname);
	if (best_match == NULL) {
		if (ispkgpattern(pkgname))
			return 1;

		if (asprintf(&pattern, "%s-[0-9]*", pkgname) == -1)
			errx(EXIT_FAILURE, "asprintf failed");

		best_match = find_best_matching_installed_pkg(pattern);
		free(pattern);
	}

	if (best_match == NULL)
		return 1;
	if (!Quiet)
		puts(best_match);
	free(best_match);
	return 0;
}

void
cleanup(int sig)
{
	exit(1);
}

static int
perform_single_pkg(const char *pkg, void *cookie)
{
	int *err_cnt = cookie;

	if (Which == WHICH_ALL || !is_automatic_installed(pkg))
		*err_cnt += pkg_do(pkg);

	return 0;
}

int
pkg_perform(lpkg_head_t *pkghead)
{
	int     err_cnt = 0;

	signal(SIGINT, cleanup);

	TAILQ_INIT(&files);

	desired_meta_data = 0;
	if ((Flags & (SHOW_INDEX | SHOW_BI_VAR)) == 0)
		desired_meta_data |= LOAD_PRESERVE;
	if ((Flags & (SHOW_INDEX | SHOW_BI_VAR)) == 0)
		desired_meta_data |= LOAD_CONTENTS;
	if (Flags & (SHOW_COMMENT | SHOW_INDEX | SHOW_SUMMARY))
		desired_meta_data |= LOAD_COMMENT;
	if (Flags & (SHOW_BI_VAR | SHOW_BUILD_INFO | SHOW_SUMMARY))
		desired_meta_data |= LOAD_BUILD_INFO | LOAD_INSTALLED_INFO;
	if (Flags & (SHOW_SUMMARY | SHOW_PKG_SIZE))
		desired_meta_data |= LOAD_SIZE_PKG;
	if (Flags & SHOW_ALL_SIZE)
		desired_meta_data |= LOAD_SIZE_ALL;
	if (Flags & (SHOW_SUMMARY | SHOW_DESC))
		desired_meta_data |= LOAD_DESC;
	if (Flags & SHOW_REQBY)
		desired_meta_data |= LOAD_REQUIRED_BY;
	if (Flags & SHOW_DISPLAY)
		desired_meta_data |= LOAD_DISPLAY;
	if (Flags & SHOW_INSTALL)
		desired_meta_data |= LOAD_INSTALL;
	if (Flags & SHOW_DEINSTALL)
		desired_meta_data |= LOAD_DEINSTALL;
	if (Flags & SHOW_MTREE)
		desired_meta_data |= LOAD_MTREE;
	if (Flags & SHOW_BUILD_VERSION)
		desired_meta_data |= LOAD_BUILD_VERSION;


	if (Which != WHICH_LIST) {
		if (File2Pkg) {
			/* Show all files with the package they belong to */
			if (pkgdb_dump() == -1)
				err_cnt = 1;
		} else {
			if (iterate_pkg_db(perform_single_pkg, &err_cnt) == -1)
				err_cnt = 1;
		}
	} else {
		/* Show info on individual pkg(s) */
		lpkg_t *lpp;

		while ((lpp = TAILQ_FIRST(pkghead))) {
			TAILQ_REMOVE(pkghead, lpp, lp_link);
			err_cnt += pkg_do(lpp->lp_name);
			free_lpkg(lpp);
		}
	}
	return err_cnt;
}
