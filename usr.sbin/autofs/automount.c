/*	$NetBSD: automount.c,v 1.3 2018/01/15 14:38:06 christos Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: automount.c,v 1.3 2018/01/15 14:38:06 christos Exp $");

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fs/autofs/autofs_mount.h>

#include "common.h"

static int
unmount_by_statfs(const struct statvfs *sb, bool force)
{
	int error, flags;

	log_debugx("unmounting %s", sb->f_mntonname);

	flags = 0;
	if (force)
		flags |= MNT_FORCE;
	error = unmount(sb->f_mntonname, flags);
	if (error != 0)
		log_warn("cannot unmount %s", sb->f_mntonname);

	return error;
}

static const struct statvfs *
find_statfs(const struct statvfs *mntbuf, int nitems, const char *mountpoint)
{
	int i;

	for (i = 0; i < nitems; i++) {
		if (strcmp(mntbuf[i].f_mntonname, mountpoint) == 0)
			return mntbuf + i;
	}

	return NULL;
}

static void
mount_autofs(const char *from, const char *fspath, const char *options,
    const char *prefix)
{
	struct autofs_args args;
	int error;
	char *cwd;

	/*
	 * There is no guarantee we are at /, so chdir to /.
	 */
	cwd = getcwd(NULL, 0);
	if (chdir("/") != 0)
		log_warn("failed to chdir to /");
	create_directory(fspath);
	if (chdir(cwd) != 0)
		log_warn("failed to restore cwd");
	free(cwd);

	log_debugx("mounting %s on %s, prefix \"%s\", options \"%s\"",
	    from, fspath, prefix, options);

	memset(&args, 0, sizeof(args));
	args.from = __UNCONST(from);
	args.master_options = __UNCONST(options);
	args.master_prefix = __UNCONST(prefix);

	error = mount("autofs", fspath, 0, &args, sizeof(args));
	if (error != 0)
		log_err(1, "cannot mount %s on %s", from, fspath);
}

static void
mount_if_not_already(const struct node *n, const char *map, const char *options,
    const char *prefix, const struct statvfs *mntbuf, int nitems)
{
	const struct statvfs *sb;
	char *mountpoint;
	char *from;
	int ret;

	ret = asprintf(&from, "map %s", map);
	if (ret < 0)
		log_err(1, "asprintf");

	mountpoint = node_path(n);
	sb = find_statfs(mntbuf, nitems, mountpoint);
	if (sb != NULL) {
		if (strcmp(sb->f_fstypename, "autofs") != 0) {
			log_debugx("unknown filesystem mounted "
			    "on %s; mounting", mountpoint);
			/*
			 * XXX: Compare options and 'from',
			 *	and update the mount if necessary.
			 */
		} else {
			log_debugx("autofs already mounted "
			    "on %s", mountpoint);
			free(from);
			free(mountpoint);
			return;
		}
	} else {
		log_debugx("nothing mounted on %s; mounting",
		    mountpoint);
	}

	mount_autofs(from, mountpoint, options, prefix);
	free(from);
	free(mountpoint);
}

static void
mount_unmount(struct node *root)
{
	struct statvfs *mntbuf;
	struct node *n, *n2;
	int i, nitems;
	static char rootdir[] = "/";

	nitems = getmntinfo(&mntbuf, MNT_WAIT);
	if (nitems <= 0)
		log_err(1, "getmntinfo");

	log_debugx("unmounting stale autofs mounts");

	for (i = 0; i < nitems; i++) {
		if (strcmp(mntbuf[i].f_fstypename, "autofs") != 0) {
			log_debugx("skipping %s, filesystem type is not autofs",
			    mntbuf[i].f_mntonname);
			continue;
		}

		n = node_find(root, mntbuf[i].f_mntonname);
		if (n != NULL) {
			log_debugx("leaving autofs mounted on %s",
			    mntbuf[i].f_mntonname);
			continue;
		}

		log_debugx("autofs mounted on %s not found "
		    "in new configuration; unmounting", mntbuf[i].f_mntonname);
		unmount_by_statfs(&(mntbuf[i]), false);
	}

	log_debugx("mounting new autofs mounts");

	TAILQ_FOREACH(n, &root->n_children, n_next) {
		if (!node_is_direct_map(n)) {
			mount_if_not_already(n, n->n_map, n->n_options,
			    n->n_key, mntbuf, nitems);
			continue;
		}

		TAILQ_FOREACH(n2, &n->n_children, n_next) {
			mount_if_not_already(n2, n->n_map, n->n_options,
			    rootdir, mntbuf, nitems);
		}
	}
}

static void
flush_autofs(const char *fspath)
{
	int error;

	log_debugx("flushing %s", fspath);

	error = mount("autofs", fspath, MNT_UPDATE, NULL, 0);
	if (error != 0)
		log_err(1, "cannot flush %s", fspath);
}

static void
flush_caches(void)
{
	struct statvfs *mntbuf;
	int i, nitems;

	nitems = getmntinfo(&mntbuf, MNT_WAIT);
	if (nitems <= 0)
		log_err(1, "getmntinfo");

	log_debugx("flushing autofs caches");

	for (i = 0; i < nitems; i++) {
		if (strcmp(mntbuf[i].f_fstypename, "autofs") != 0) {
			log_debugx("skipping %s, filesystem type is not autofs",
			    mntbuf[i].f_mntonname);
			continue;
		}

		flush_autofs(mntbuf[i].f_mntonname);
	}
}

static void
unmount_automounted(bool force)
{
	struct statvfs *mntbuf;
	int i, nitems;

	nitems = getmntinfo(&mntbuf, MNT_WAIT);
	if (nitems <= 0)
		log_err(1, "getmntinfo");

	log_debugx("unmounting automounted filesystems");

	for (i = 0; i < nitems; i++) {
		if (strcmp(mntbuf[i].f_fstypename, "autofs") == 0) {
			log_debugx("skipping %s, filesystem type is autofs",
			    mntbuf[i].f_mntonname);
			continue;
		}

		if ((mntbuf[i].f_flag & MNT_AUTOMOUNTED) == 0) {
			log_debugx("skipping %s, not automounted",
			    mntbuf[i].f_mntonname);
			continue;
		}

		unmount_by_statfs(&(mntbuf[i]), force);
	}
}

__dead static void
usage_automount(void)
{

	fprintf(stderr, "Usage: %s [-D name=value][-o opts][-Lcfuv]\n",
	    getprogname());
	exit(1);
}

int
main_automount(int argc, char **argv)
{
	struct node *root;
	int ch, debug = 0, show_maps = 0;
	char *options = NULL;
	bool do_unmount = false, force_unmount = false, flush = false;

	/*
	 * Note that in automount(8), the only purpose of variable
	 * handling is to aid in debugging maps (automount -L).
	 */
	defined_init();

	while ((ch = getopt(argc, argv, "D:Lfco:uv")) != -1) {
		switch (ch) {
		case 'D':
			defined_parse_and_add(optarg);
			break;
		case 'L':
			show_maps++;
			break;
		case 'c':
			flush = true;
			break;
		case 'f':
			force_unmount = true;
			break;
		case 'o':
			options = concat(options, ',', optarg);
			break;
		case 'u':
			do_unmount = true;
			break;
		case 'v':
			debug++;
			break;
		case '?':
		default:
			usage_automount();
		}
	}
	argc -= optind;
	if (argc != 0)
		usage_automount();

	if (force_unmount && !do_unmount)
		usage_automount();

	log_init(debug);

	if (flush) {
		flush_caches();
		return 0;
	}

	if (do_unmount) {
		unmount_automounted(force_unmount);
		return 0;
	}

	root = node_new_root();
	parse_master(root, AUTO_MASTER_PATH);

	if (show_maps) {
		if (show_maps > 1) {
			node_expand_indirect_maps(root);
			node_expand_ampersand(root, NULL);
		}
		node_expand_defined(root);
		node_print(root, options);
		return 0;
	}

	mount_unmount(root);

	return 0;
}
