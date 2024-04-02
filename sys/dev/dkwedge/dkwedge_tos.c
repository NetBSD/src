/* $NetBSD: dkwedge_tos.c,v 1.1 2024/04/02 22:30:03 charlotte Exp $ */

/*
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charlotte Koch.
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

/*
 * dk(4) support for Atari "TOS" partition schemes.
 *
 * Technical details taken from:
 *
 * - "Atari Hard Disk File Systems Reference Guide" v1.2b by Jean
 * Louis-Guerin (DrCoolZic) (September 2014)
 *
 * https://info-coach.fr/atari/documents/_mydoc/Atari_HD_File_Sytem_Reference_Guide.pdf
 */

#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/endian.h>
#include <sys/vnode.h>

#define TOS_PART_TYPE_LEN	3
#define TOS_GEM_PARTITION	"GEM"
#define TOS_BGM_PARTITION	"BGM"
#define TOS_XGM_PARTITION	"XGM"

#define TOS_CXNMETHOD_SASI	0x00
#define TOS_CXNMETHOD_SCSI	0xFF

#define TOS_MAX_PART_COUNT	4
#define TOS_SECTOR_SIZE		512

struct tos_partition {
	uint8_t status;
	char type[TOS_PART_TYPE_LEN];
	uint32_t offset;
	uint32_t size;
} __packed;
__CTASSERT(sizeof(struct tos_partition) == 12);

struct tos_root_sector {
	uint8_t unused1[441];
	uint8_t connection_method;
	uint8_t unused2[8];
	uint32_t size;
	struct tos_partition parts[TOS_MAX_PART_COUNT];
	uint8_t unused3[10];
} __packed;
__CTASSERT(sizeof(struct tos_root_sector) == TOS_SECTOR_SIZE);

static int dkwedge_discover_tos(struct disk *pdk, struct vnode *vp);

static int
dkwedge_discover_tos(struct disk *pdk, struct vnode *vp)
{
	struct dkwedge_info dkw;
	int error = 0;
	size_t i;
	char safe_type[4];

	/* Get ourselves a fistful of memory. */
	buf_t *bp = geteblk(TOS_SECTOR_SIZE);

	error = dkwedge_read(pdk, vp, 0L, bp->b_data, TOS_SECTOR_SIZE);
	if (error) {
		aprint_error("unable to read TOS Root Sector: error = %d",
		    error);
		goto out;
	}

	struct tos_root_sector *trs = bp->b_data;

	/*
	 * If the "connection method" isn't recognized, then this is
	 * probably NOT an Atari-style partition, so get outta here. Note
	 * that there really isn't a magic number we can rely on; this check
	 * is somewhat made up. But at least it's better than nothing.
	 */
	switch (trs->connection_method) {
	case TOS_CXNMETHOD_SASI: /* FALLTHROUGH */
	case TOS_CXNMETHOD_SCSI:
		; /* OK */
		break;
	default:
		error = ESRCH;
		goto out;
	}

	/*
	 * Make a wedge for each partition that exists (i.e., has the "exist"
	 * bit set).
	 */
	for (i = 0; i < TOS_MAX_PART_COUNT; i++) {
		struct tos_partition part = trs->parts[i];

		if (!(part.status & 0x01))
			continue;

		/* Ignore if we see it's an extended "XGM" partition. */
		if (!strncmp(part.type, TOS_XGM_PARTITION, TOS_PART_TYPE_LEN)) {
			aprint_normal(
			    "WARNING: XGM partitions are not yet supported\n");
			continue;
		}

		/*
		 * Otherwise, get outta here if it's not the partition-types
		 * we *do* support.
		 */
		if (strncmp(part.type, TOS_GEM_PARTITION, TOS_PART_TYPE_LEN) &&
		    strncmp(part.type, TOS_BGM_PARTITION, TOS_PART_TYPE_LEN)) {
			error = ESRCH;
			goto out;
		}

		memset(&dkw, 0, sizeof(dkw));
		memset(safe_type, 0, sizeof(safe_type));

		/*
		 * The partition type string is NOT NUL-terminated, so let's
		 * play it safe.
		 */
		memcpy(safe_type, part.type, TOS_PART_TYPE_LEN);
		safe_type[TOS_PART_TYPE_LEN] = '\0';

		/* Finally, make the wedge. */
		snprintf(dkw.dkw_wname, sizeof(dkw.dkw_wname), "ATARI_%s_%02lu",
		    safe_type, i);
		dkw.dkw_offset = be32toh(trs->parts[i].offset);
		dkw.dkw_size = be32toh(trs->parts[i].size);
		strlcpy(dkw.dkw_ptype, DKW_PTYPE_FAT, sizeof(dkw.dkw_ptype));
		strlcpy(dkw.dkw_parent, pdk->dk_name, sizeof(dkw.dkw_parent));
		error = dkwedge_add(&dkw);

		if (error == EEXIST) {
			aprint_error("partition named \"%s\" already exists",
			    dkw.dkw_wname);
			goto out;
		}
	}

	error = 0;

out:
	brelse(bp, 0);
	return error;
}

DKWEDGE_DISCOVERY_METHOD_DECL(TOS, 10, dkwedge_discover_tos);
