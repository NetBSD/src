/*	$NetBSD: dtfs.c,v 1.2 2006/10/25 18:18:16 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
 * Delectable Test File System: a simple in-memory file system which
 * demonstrates the use of puffs.
 * (a.k.a. Detrempe FS ...)
 */

#include <sys/types.h>

#include <err.h>
#include <puffs.h>
#include <stdlib.h>
#include <string.h>

#include "dtfs.h"

#ifdef DEEP_ROOTED_CLUE
#define FSNAME "detrempe"
#else
#define FSNAME "dt"
#endif

int
main(int argc, char *argv[])
{
	struct puffs_usermount *pu;
	struct puffs_vfsops pvfs;
	struct puffs_vnops pvn;
	uint32_t flags = 0;

	setprogname(argv[0]);

	if (argc < 2)
		errx(1, "usage: %s mountpath\n", getprogname());

	if (argc == 3 && *argv[2] == 'd') /* nice */
		flags |= PUFFSFLAG_OPDUMP;

	memset(&pvfs, 0, sizeof(struct puffs_vfsops));
	memset(&pvn, 0, sizeof(struct puffs_vnops));

	pvfs.puffs_start = dtfs_start;
	pvfs.puffs_unmount = dtfs_unmount;
	pvfs.puffs_sync = dtfs_sync;
	pvfs.puffs_statvfs = dtfs_statvfs;

	pvn.puffs_lookup = dtfs_lookup;
	pvn.puffs_getattr = dtfs_getattr;
	pvn.puffs_setattr = dtfs_setattr;
	pvn.puffs_create = dtfs_create;
	pvn.puffs_remove = dtfs_remove;
	pvn.puffs_readdir = dtfs_readdir;
	pvn.puffs_mkdir = dtfs_mkdir;
	pvn.puffs_rmdir = dtfs_rmdir;
	pvn.puffs_rename = dtfs_rename;
	pvn.puffs_read = dtfs_read;
	pvn.puffs_write = dtfs_write;
	pvn.puffs_link = dtfs_link;
	pvn.puffs_symlink = dtfs_symlink;
	pvn.puffs_readlink = dtfs_readlink;
	pvn.puffs_reclaim = dtfs_reclaim;
	pvn.puffs_inactive = dtfs_inactive;

	if ((pu = puffs_mount(&pvfs, &pvn, argv[1], 0, FSNAME, flags, 0))
	    == NULL)
		err(1, "mount");

	if (puffs_mainloop(pu) == -1)
		err(1, "mainloop");

	return 0;
}
