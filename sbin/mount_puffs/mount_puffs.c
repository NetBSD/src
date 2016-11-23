/*	$NetBSD: mount_puffs.c,v 1.5 2016/11/23 14:33:29 pho Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This is to support -o getargs without having to replicate it in
 * every file server. It also allows puffs filesystems to be mounted
 * via "mount -a".
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount_puffs.c,v 1.5 2016/11/23 14:33:29 pho Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <fs/puffs/puffs_msgif.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <unistd.h>
#include <util.h>

static int
usage(void)
{

	fprintf(stderr, "usage: %s [-o options] program[#source] mountpoint\n", getprogname());
	return 1;
}

static int show_puffs_mount_args(const char *mountpoint)
{
	const char *vtypes[] = { VNODE_TYPES };
	struct puffs_kargs kargs;

	if (mount(MOUNT_PUFFS, mountpoint, MNT_GETARGS, &kargs, sizeof(kargs)) == -1)
		err(1, "mount");

	printf("version=%d, ", kargs.pa_vers);
	printf("flags=0x%x, ", kargs.pa_flags);

	printf("root cookie=%p, ", kargs.pa_root_cookie);
	printf("root type=%s", vtypes[kargs.pa_root_vtype]);

	if (kargs.pa_root_vtype != VDIR)
		printf(", root size=%llu",
		    (unsigned long long)kargs.pa_root_vsize);
	if (kargs.pa_root_vtype == VCHR || kargs.pa_root_vtype == VBLK)
		printf(", root rdev=0x%" PRIx64, (uint64_t)kargs.pa_root_rdev);
	printf("\n");

	return 0;
}

static int
mount_puffs_filesystem(const char *program, const char *opts,
					const char *source, const char *mountpoint)
{
	int argc = 0;
	const char **argv;
	int rv = 0;

	/* Construct an argument vector:
	 * program [-o opts] [source] mountpoint */
	argv = ecalloc(1 + 2 + 1 + 1, sizeof(*argv));
	argv[argc++] = program;
	if (opts != NULL) {
		argv[argc++] = "-o";
		argv[argc++] = opts;
	}
	if (source != NULL) {
		argv[argc++] = source;
	}
	argv[argc++] = mountpoint;
	argv[argc] = NULL;

	/* We intentionally use execvp(3) here because the program can
	 * actually be a basename. */
	if (execvp(program, __UNCONST(argv)) == -1) {
		warn("Cannot execute %s", program);
		rv = 1;
	}

	free(argv);
	return rv;
}

static void add_opt(char **opts, const char *opt)
{
	const size_t orig_len = *opts == NULL ? 0 : strlen(*opts);

	*opts = erealloc(*opts, orig_len + 1 + strlen(opt) + 1);

	if (orig_len == 0) {
		strcpy(*opts, opt);
	}
	else {
		strcat(*opts, ",");
		strcat(*opts, opt);
	}
}

int
main(int argc, char *argv[])
{
	int mntflags = 0;
	int ch;
	char *opts = NULL;
	int rv = 0;

	while ((ch = getopt(argc, argv, "o:")) != -1) {
		switch (ch) {
		case 'o':
			for (char *opt = optarg; (opt = strtok(opt, ",")) != NULL; opt = NULL) {
				if (strcmp(opt, "getargs") == 0) {
					mntflags |= MNT_GETARGS;
					break; /* No need to parse it any further. */
				}
				else {
					add_opt(&opts, opt);
				}
			}
			break;
		default:
			rv = usage();
			goto free_opts;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2) {
		rv = usage();
		goto free_opts;
	}

	if (mntflags & MNT_GETARGS) {
		/* Special case for -o getargs: retrieve kernel arguments for
		 * an already mounted filesystem. */
		rv = show_puffs_mount_args(argv[1]);
	}
	else {
		/* Split the program name and source. This is to allow
		 * filesystems to be mounted via "mount -a" i.e. /etc/fstab */
		char *source  = argv[0];
		char *program = strsep(&source, "#");

		rv = mount_puffs_filesystem(program, opts, source, argv[1]);
	}

free_opts:
	free(opts);
	return rv;
}
