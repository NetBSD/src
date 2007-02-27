/*	$NetBSD: vtoc.c,v 1.1.28.1 2007/02/27 16:50:20 yamt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vtoc.c,v 1.1.28.1 2007/02/27 16:50:20 yamt Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#if !defined(_KERNEL) && defined(_STANDALONE)
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#endif

#ifdef VTOC_DEBUG
#define	DPRINTF(fmt, args...)	printf(fmt, ##args)
#else
#define	DPRINTF(arg...)		((void)0)
#endif

#include <machine/sector.h>
#include <machine/vtoc.h>

#ifdef VTOC_DEBUG
void vtoc_print_partition_table(const struct ux_partition *);
#endif /* VTOC_DEBUG */

bool
vtoc_sector(void *rwops, struct vtoc_sector *vtoc, int start)
{

	if (!sector_read(rwops, (void *)vtoc, start + VTOC_SECTOR))
		return false;

	if (!vtoc_sanity(vtoc))
		return false;

	return true;
}

const struct ux_partition *
vtoc_find_bfs(const struct vtoc_sector *vtoc)
{
	int i;

	if (!vtoc_valid(vtoc)) {
		printf("invalid VTOC\n");
		return 0;
	}

	for (i = 0; i < vtoc->npartitions; i++)
		if (vtoc->partition[i].tag == VTOC_TAG_STAND)
			break;

	if (i == vtoc->npartitions) {
		printf("no bfs partition found.\n");
		return 0;
	}

	return &vtoc->partition[i];
}

bool
vtoc_valid(const struct vtoc_sector *vtoc)
{

	return (vtoc->magic == VTOC_MAGIC) && (vtoc->version == VTOC_VERSION);
}

bool
vtoc_sanity(const struct vtoc_sector *vtoc)
{

	if (!vtoc_valid(vtoc)) {
		DPRINTF("Invalid VTOC.\n");
		return false;
	}

	DPRINTF("[VTOC] (%d byte)\n", sizeof *vtoc);
	DPRINTF("Bootinfo = %08x %08x %08x\n",
	    vtoc->bootinfo[0], vtoc->bootinfo[1], vtoc->bootinfo[2]);
	DPRINTF("Magic = %08x\n", vtoc->magic);
	DPRINTF("Version = %d\n", vtoc->version);
	DPRINTF("Volume = %s\n", vtoc->volume);
	DPRINTF("Sector size = %d\n", vtoc->sector_size_byte);
	DPRINTF("# of partitions = %d\n", vtoc->npartitions);
#ifdef VTOC_DEBUG
	vtoc_print_partition_table(vtoc->partition);
#endif
	return true;
}

#ifdef VTOC_DEBUG
void
vtoc_print_partition_table(const struct ux_partition *p)
{
	int i;

	DPRINTF(" Partition Tag  Flags      Start        Count        Last\n");
	for (i = 0; i < 16; i++, p++) {
		DPRINTF("     %x      %d    %02x     %8d     %8d    %8d\n",
		    i, p->tag, p->flags, p->start_sector, p->nsectors,
		    p->start_sector + p->nsectors - 1);
	}
}
#endif /* VTOC_DEBUG */
