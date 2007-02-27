/*	$NetBSD: pdinfo.c,v 1.1.28.1 2007/02/27 16:50:19 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pdinfo.c,v 1.1.28.1 2007/02/27 16:50:19 yamt Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#ifndef _KERNEL
#include "local.h"
#endif

#ifdef PDINFO_DEBUG
#define	DPRINTF(fmt, args...)	printf(fmt, ##args)
#else
#define	DPRINTF(arg...)		((void)0)
#endif

#include <machine/sector.h>
#include <machine/pdinfo.h>

bool
pdinfo_sector(void *rwops, struct pdinfo_sector *pdinfo)
{

	if (!sector_read(rwops, (void *)pdinfo, PDINFO_SECTOR))
		return false;

	if (!pdinfo_sanity(pdinfo))
		return false;

	return true;
}

bool
pdinfo_valid(const struct pdinfo_sector *disk)
{

	return disk->magic == PDINFO_MAGIC;
}

bool
pdinfo_sanity(const struct pdinfo_sector *disk)
{
	const struct disk_geometory *geom;
	const struct disk_ux *ux;

	if (!pdinfo_valid(disk)) {
		DPRINTF("no physical disk info.\n");
		return false;
	}

	geom = &disk->geometory;
	ux = &disk->ux;
	DPRINTF("physical disk sector size %dbyte\n", sizeof *disk);
	DPRINTF("[disk]\n");
	DPRINTF("drive_id = %#x\n", disk->drive_id);
	DPRINTF("magic = %#x\n", disk->magic);
	DPRINTF("version = %d\n", disk->version);
	DPRINTF("serial # %s\n", disk->device_serial_number);
#define	_(x)	DPRINTF(#x " = %d\n", geom->x);
	_(cylinders_per_drive);
	_(tracks_per_cylinder);
	_(sectors_per_track);
	_(bytes_per_sector);
#undef _
	DPRINTF("logical_sector = %d\n", disk->logical_sector);
#define	_(x)	DPRINTF(#x " = %d\n", ux->x);
	_(errorlog_sector);
	_(errorlog_size_byte);
	_(mfg_sector);
	_(mfg_size_byte);
	_(defect_sector);
	_(defect_size_byte);
	_(n_relocation_area);
	_(relocation_area_sector);
	_(relocation_area_size_byte);
	_(next_relocation_area);
	_(diag_sector);
	_(diag_size);
	_(gap_size);
#undef _
	/* undocumented region */
	{
		int i, j;
		DPRINTF("--reserved--\n");
		for (i = 0, j = 1; i < 104; i++, j++) {
			DPRINTF("[%3d]%8x ", i, disk->device_depend[i]);
			if (j == 8) {
				DPRINTF("\n");
				j = 0;
			}
		}
	}

	return true;
}
