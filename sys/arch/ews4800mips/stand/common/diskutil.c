/*	$NetBSD: diskutil.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libkern/libkern.h>

#include <machine/pdinfo.h>
#include <machine/vtoc.h>
#include <machine/bfs.h>

#include "cmd.h"
#include "local.h"

struct pdinfo_sector pdinfo;
struct vtoc_sector vtoc;
int vtoc_readed;

void bfs_ls(void);

int
cmd_disklabel(int argc, char *argp[], int interactive)
{
	struct ux_partition *partition;
	int i;

	if (!read_vtoc())
		return 1;

	partition = vtoc.partition;
	printf("Tag\tFlags\tStart\tCount\n");
	for (i = 0; i < VTOC_MAXPARTITIONS; i++, partition++)
		printf("   %d %d   %d\t%d\t%d\n", i, partition->tag,
		    partition->flags, partition->start_sector,
		    partition->nsectors);

	return 0;
}

int
cmd_ls(int argc, char *argp[], int interactive)
{
	int i;

	if (argc < 2) {
		printf("ls partition\n");
		return 1;
	}

	if (!read_vtoc())
		return 1;

	i = strtoul(argp[1], 0, 0);
	if (i < 0 || i >= VTOC_MAXPARTITIONS)
		return 1;

	if (!device_attach(-1, -1, i))
		return 1;
	switch (fstype(i)) {
	case FSTYPE_UFS:
		ufs_ls("/");
		break;
	case FSTYPE_BFS:
		bfs_ls();
		break;
	default:
		return 1;
	}

	return 0;
}

void
bfs_ls(void)
{
	struct bfs *bfs;
	struct bfs_dirent *file;
	struct bfs_inode *inode;
	int i;

	if (!DEVICE_CAPABILITY.disk_enabled)
		return;

	if (bfs_init(&bfs) != 0)
		return;

	for (file = bfs->dirent, i = 0; i < bfs->max_dirent; i++, file++) {
		if (file->inode != 0) {
			inode = &bfs->inode[file->inode - BFS_ROOT_INODE];
			printf("%s\t%d (%d-%d)\n", file->name,
			    bfs_file_size(inode), inode->start_sector,
			    inode->end_sector);
		}
	}

	bfs_fini(bfs);
}

int
fstype(int partition)
{
	struct ux_partition *p;

	if (!read_vtoc())
		return -1;

	if (partition < 0 || partition >= VTOC_MAXPARTITIONS)
		return -1;

	p = &vtoc.partition[partition];
	if (p->tag == VTOC_TAG_STAND)
		return FSTYPE_BFS;

	if ((p->flags & VTOC_FLAG_UNMOUNT) == 0)
		return FSTYPE_UFS; /* possibly */

	return -1;
}

boolean_t
find_partition_start(int partition,  int *sector)
{

	if (!read_vtoc())
		return FALSE;

	*sector = pdinfo.logical_sector +
	    vtoc.partition[partition].start_sector;
	printf("[partition=%d, start sector=%d]", partition, *sector);

	return TRUE;
}

boolean_t
read_vtoc(void)
{

	if (!DEVICE_CAPABILITY.disk_enabled)
		return FALSE;

	if (vtoc_readed)
		return TRUE;

	if (!pdinfo_sector(0, &pdinfo)) {
		printf("no PDINFO\n");
		return FALSE;
	}

	if (!vtoc_sector(0, &vtoc, pdinfo.logical_sector)) {
		printf("no VTOC\n");
		return FALSE;
	}
	vtoc_readed = TRUE;

	return TRUE;
}
