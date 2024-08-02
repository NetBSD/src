/*	$NetBSD: pass0.c,v 1.1.2.3 2024/08/02 00:18:59 perseant Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <util.h>

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/queue.h>

#define buf ubuf
#define vnode uvnode
#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>

#include "bufcache.h"
#include "fsutil.h"
#include "fsck_exfatfs.h"
#include "extern.h"
#include "pass0.h"

/*
 * Check the superblock.
 */
void
pass0(struct exfatfs *fs, struct dkwedge_info *dkwp)
{
	int base, i;
	unsigned j;
	struct ubuf *bp;
	uint32_t cksum;
	uint8_t boot_ignore[3] = { 106, 107, 112 };
	int boot_ignore_len = 3;

	/*
	 * Check the checksum of the boot block set, for both boot blocks.
	 */
	for (base = 0; base < 24; base += 12) {
		cksum = 0;
		
		/* Read boot sectors to compute checksum */
		for (i = 0; i < 11; i++) {
			int written = 0;
			bread(fs->xf_devvp, base + i, BSSIZE(fs), 0, &bp);

			cksum = exfatfs_cksum32(cksum, (uint8_t *)bp->b_data,
						BSSIZE(fs),
						boot_ignore,
						(i == 0 ? boot_ignore_len : 0));
			if (i > 0 && i < 9) {
				uint32_t *ebcp = (uint32_t *)((char *)bp->b_data
					      + BSSIZE(fs) - sizeof(uint32_t));
				if (*ebcp && *ebcp != htole32(0xAA550000)) {
					pwarn("Extended boot sector %d magic number wrong\n",
					      base + i);
					if (Pflag || reply("CONTINUE") == 0)
						exit(1);
					if (reply("zero") == 1) {
						*ebcp = 0;
						bwrite(bp);
						written = 1;
					} else if (reply("set to 0xAA550000") == 1) {
						*ebcp = htole32(0xAA550000);
						bwrite(bp);
						written = 1;
					}
				}
			}

			if (!written)
				brelse(bp, 0);
		}
		
		/* Compare boot block against recorded checksum */
		bread(fs->xf_devvp, base + i, BSSIZE(fs), 0, &bp);
		for (j = 0; j < BSSIZE(fs) / sizeof(uint32_t); j++) {
			if (((uint32_t *)bp->b_data)[j] != cksum) {
				pwarn("Boot block %d checksum mismatch:"
				      " word %d expected 0x%x computed 0x%x\n",
				      base / 12, j,
				      ((uint32_t *)bp->b_data)[j], cksum);
				break;
			}
		}
		bwrite(bp);
	}

	/*
	 * Sanity check boot block values
	 */

	/* BootSignature: spec says to verify this field first. */
	if (fs->xf_BootSignature != EXFAT_BOOT_SIGNATURE) {
		pfatal("BootSignature value %hu is not %hu."
		       "  FILE SYSTEM MAY NOT BE exFAT!\n",
		       fs->xf_BootSignature, EXFAT_BOOT_SIGNATURE);
		if (Pflag || reply("fix") == 0)
			exit(1);
		fs->xf_BootSignature = EXFAT_BOOT_SIGNATURE;
		fsdirty = 1;
	}
	
	/*
	 * Check fsname early; if that is wrong the operator ought
	 * to know before making other changes.
	 */
	if (memcmp(fs->xf_FileSystemName, EXFAT_FSNAME,
		   sizeof(EXFAT_FSNAME)) != 0) {
		pfatal("Filesystem type is not '%s'\n", EXFAT_FSNAME);
		if (Pflag || reply("CONTINUE") == 0)
			exit(1);
		if (reply("FIX") == 1) {
			memcpy(fs->xf_FileSystemName, EXFAT_FSNAME, sizeof(EXFAT_FSNAME));
			fsdirty = 1;
		}
	}

	/* JumpBoot */
	if (memcmp(fs->xf_JumpBoot, EXFAT_JUMPBOOT,
		   sizeof(fs->xf_JumpBoot)) != 0) {
		pwarn("JumpBoot value %hhX %hhX %hhX wrong, should be %hhX %hhX %hhX%s\n",
		      fs->xf_JumpBoot[0],
		      fs->xf_JumpBoot[1],
		      fs->xf_JumpBoot[2],
		      EXFAT_JUMPBOOT[0],
		      EXFAT_JUMPBOOT[1],
		      EXFAT_JUMPBOOT[2],
		      Pflag ? "( corrected)" : "");
		if (Pflag || reply("correct") == 1) {
			memcpy(fs->xf_JumpBoot, EXFAT_JUMPBOOT,
			       sizeof(EXFAT_JUMPBOOT));
			fsdirty = 1;
		}
	}

	/* MustBeZero */
	for (i = 0; i < (int)sizeof(fs->xf_MustBeZero); i++) {
		if (fs->xf_MustBeZero[i] != 0) {
			pwarn("MustBeZero is not zero at byte %d\n", i);
			if (Pflag || reply("correct") == 1) {
				memset(fs->xf_MustBeZero, 0,
				       sizeof(fs->xf_MustBeZero));
				fsdirty = 1;
				break;
			}
		}
	}

	/* PartitionOffset: not really used? */
	if (fs->xf_PartitionOffset != 0
	    && fs->xf_PartitionOffset != (uint64_t)dkwp->dkw_offset) {
		pwarn("PartitionOffset does not match disklabel\n");
		if (Pflag || reply("correct") == 1) {
			fs->xf_PartitionOffset = dkwp->dkw_offset;
			fsdirty = 1;
		}
	}

	/* VolumeLength */
	if (fs->xf_VolumeLength > dkwp->dkw_size) {
		pfatal("VolumeLength larger than volume size\n");
		if (Pflag || reply("correct") == 1) {
			fs->xf_VolumeLength = dkwp->dkw_size;
			fsdirty = 1;
		}
	}

	/* FatOffset */
	if (fs->xf_FatOffset < 24) {
		pfatal("FatOffset too small\n");
		if (Pflag || reply("continue") == 0) {
			exit(1);
		}
	}
		
	/* FatLength */
	if (fs->xf_FatLength < howmany((fs->xf_ClusterCount + 2) * 4, FATBSIZE(fs))) {
		pfatal("FatLength too small\n");
		if (Pflag || reply("continue") == 0)
			exit(1);
	}
	if (fs->xf_FatLength * fs->xf_NumberOfFats
	    > (fs->xf_ClusterHeapOffset - fs->xf_FatOffset)) {
		pfatal("FatLength too large\n");
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* ClusterHeapOffset */
	if (fs->xf_ClusterHeapOffset < fs->xf_FatOffset
	    + fs->xf_FatLength * fs->xf_NumberOfFats) {
		pfatal("ClusterHeapOffset too small\n");
		if (Pflag || reply("continue") == 0)
			exit(1);
	}
	if (fs->xf_ClusterHeapOffset > fs->xf_VolumeLength - (fs->xf_ClusterCount << fs->xf_SectorsPerClusterShift)) {
		pfatal("ClusterHeapOffset too large\n");
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/*
	 * CluaterCount:
	 * An error here is fatal because changing the value would require
	 * changing the size of the FAT table as well, and we might not
	 * have space for that.
	 * XXX what if we do?
	 */
	uint32_t computedClusterCount = (fs->xf_VolumeLength - fs->xf_ClusterHeapOffset) >> fs->xf_SectorsPerClusterShift;
	if (fs->xf_ClusterCount > computedClusterCount) {
		pwarn("Given ClusterCount %lu > computed value %lu\n",
			(unsigned long)fs->xf_ClusterCount, (unsigned long)computedClusterCount);
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* FirstClusterOfRootDirectory */
	if (fs->xf_FirstClusterOfRootDirectory < 2 || 
	    fs->xf_FirstClusterOfRootDirectory > fs->xf_ClusterCount + 1) {
		pfatal("FirstClusterOfRootDirectory out of bounds\n");
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* VolumeSerialNumber: all values are valid */

	/* FileSystemRevision */
	uint16_t lo = fs->xf_FileSystemRevision & 0xFF;
	uint16_t hi = fs->xf_FileSystemRevision >> 8;
	if (hi < 1 || hi > 99 || lo > 99) {
		pwarn("FileSystemRevision %hu.%hu out of bounds\n", hi, lo);
		if (Pflag || reply("fix") == 1) {
			fs->xf_FileSystemRevision = 0x100;
			fsdirty = 1;
		}
	} else if (hi > EXFAT_GREATEST_VERSION) {
		pfatal("FileSystemRevision %hu.%hu not understood by this version of fsck_exfatfs\n", hi, lo);
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* VolumeFlags */
	if ((fs->xf_VolumeFlags & EXFATFS_VOLUME_ACTIVE_FAT) > 0
	    && fs->xf_NumberOfFats == 1) {
		pwarn("VolumeFlags indicates ActiveFat==1 but only one FAT\n");
		if (Pflag || reply("fix") == 1) {
			fs->xf_VolumeFlags &= ~EXFATFS_VOLUME_ACTIVE_FAT;
			fsdirty = 1;
		}
	}

	/* BytesPerSectorShift */
	if (fs->xf_BytesPerSectorShift < 9 || fs->xf_BytesPerSectorShift > 12) {
		pfatal("BytesPerSectorShift %hhu out of bounds\n",
		       fs->xf_BytesPerSectorShift);
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* SectorsPerClsterShift */
	if (fs->xf_SectorsPerClusterShift > 25 - fs->xf_BytesPerSectorShift) {
		pfatal("SectorsPerClusterShift %hhu too large\n",
		       fs->xf_SectorsPerClusterShift);
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* NumberOfFats */
	if (fs->xf_NumberOfFats > 1) {
		pfatal("NumberOfFats value %hhu out of bounds\n",
		       fs->xf_NumberOfFats);
		if (Pflag || reply("continue") == 0)
			exit(1);
	}

	/* DriveSelect: all values are valid but 0x80 is suggested */
	if (fs->xf_DriveSelect != 0x80) {
		pwarn("DriveSelect is not 0x80\n");
		if (!Pflag && reply("reset") == 1) {
			fs->xf_DriveSelect = 0x80;
			fsdirty = 1;
		}
	}

	/* PercentInUse */
	if (fs->xf_PercentInUse != 0xFF && fs->xf_PercentInUse > 100) {
		pfatal("PercentInUse value %hhu out of bounds\n",
		       fs->xf_PercentInUse);
		if (Pflag || reply("fix") == 1) {
			fs->xf_PercentInUse = 0xFF;
			fsdirty = 1;
		}
	}

	/* BootCode: we can't analyze bootstrapping instructions */
}
