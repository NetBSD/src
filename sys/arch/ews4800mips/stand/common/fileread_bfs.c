/*	$NetBSD: fileread_bfs.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
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
#include <lib/libkern/libkern.h>

#include <sys/types.h>
#include <sys/param.h>
#include "bootxx.h"

#include <machine/pdinfo.h>
#include <machine/vtoc.h>
#include <machine/bfs.h>
#include <machine/sbd.h>

int
fileread(const char *fname, size_t *size)
{
	struct pdinfo_sector *pdinfo = (void *)SDBOOT_PDINFOADDR;
	struct vtoc_sector *vtoc = (void *)SDBOOT_VTOCADDR;
	struct ux_partition *partition = vtoc->partition;
	struct bfs_inode *inode = (void *)SDBOOT_INODEADDR;
	struct bfs_dirent *dirent = (void *)SDBOOT_DIRENTADDR;
	int i, n, err, block_size, bfs_sector;

	if (pdinfo->magic != PDINFO_MAGIC)
		return BERR_PDINFO;
	block_size = pdinfo->geometory.bytes_per_sector;

#if 0
{
	char msg[] = "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f";
	const char hex[] = "0123456789ABCDEF";
	u_int v;
	uint8_t *p = (void *)vtoc;

	err = dk_read(pdinfo->logical_sector + VTOC_SECTOR, 1, vtoc);
	for (n = 0; n < 16; n++) {
		for (i = 0; i < 16; i++) {
			v = (*p >> 4) & 0x0f;
			msg[i*3] = hex[v];
			v = *p & 0x0f;
			msg[i*3+1] = hex[v];
			p++;
		}
		for (i = 0; msg[i] != 0; i++)
			ROM_PUTC(32 + i * 12, 0 + n * 24, msg[i]);
		ROM_PUTC(0, 0 + n * 24, '\r');
		ROM_PUTC(0, 0 + n * 24, '\n');
	}
}
#endif
	/* Read VTOC */
	err = dk_read(pdinfo->logical_sector + VTOC_SECTOR, 1, vtoc);
	if ((err & 0x7f) != 0)
		return BERR_RDVTOC;

	if (vtoc->magic != VTOC_MAGIC)
		return BERR_VTOC;

	/* Find BFS */
	for (i = 0; i < VTOC_MAXPARTITIONS; i++, partition++)
		if (partition->tag == VTOC_TAG_STAND)
			break;

	if (i == VTOC_MAXPARTITIONS)
		return BERR_NOBFS;
	bfs_sector = pdinfo->logical_sector + partition->start_sector;

	/* Read inode */
	err = dk_read(bfs_sector + 1/* skip super block */, 1, inode);
	if ((err & 0x7f) != 0)
		return BERR_RDINODE;

	if (inode->number != BFS_ROOT_INODE)
		return BERR_NOROOT;

	/* Read root directory */
	n = inode->eof_offset_byte - inode->start_sector * 512 + 1;

	err = dk_read(bfs_sector + inode->start_sector, 1, dirent);
	if ((err & 0x7f) != 0)
		return BERR_RDDIRENT;

	n /= sizeof(struct bfs_dirent);
	DPRINTF("%d files.\n", n);
	for (i = 0; i < n; i++, dirent++)
		if (strcmp(dirent->name, fname) == 0)
			break;

	if (i == n)
		return BERR_NOFILE;

	/* Read file */
	DPRINTF("%s (%d)\n", dirent->name, dirent->inode);
	inode = &inode[dirent->inode - BFS_ROOT_INODE];

	err = dk_read(bfs_sector + inode->start_sector,
	    inode->end_sector - inode->start_sector + 1,
	    (void *)SDBOOT_SCRATCHADDR);

	if ((err & 0x7f) != 0)
		return BERR_RDFILE;

	*size = inode->eof_offset_byte - inode->start_sector * 512 + 1;
	DPRINTF("read %dbyte\n", *size);

	return 0;
}
