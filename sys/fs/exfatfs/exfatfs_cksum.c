/*	$NetBSD: exfatfs_cksum.c,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $	*/

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exfatfs_cksum.c,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $");

#include <sys/types.h>
#ifdef _KERNEL
# include <sys/endian.h>
# include <sys/systm.h>
#else
# include <stdio.h>
# include <string.h>
#endif /* _KERNEL */

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_tables.h>

/* #define EXFATFS_CKSUM_DEBUG */

#ifdef EXFATFS_CKSUM_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x) __nothing
#endif

/*
 * Checksum routines.  The 32-bit version is used for boot block checksum
 * and table checksum.
 */
uint32_t exfatfs_cksum32(uint32_t seed, uint8_t *data, uint64_t datalen,
			uint8_t *ignore, uint8_t ignorelen)
{
	uint32_t cksum = seed;
	uint32_t idx;
	int i, ignoring;

	DPRINTF(("%x ->", (unsigned)cksum));
	for (idx = 0; idx < datalen; idx++)
	{
		ignoring = 0;
		for (i = 0; i < ignorelen; ++i) {
			if (idx == ignore[i]) {
				ignoring = 1;
				break;
			}
		}
		if (ignoring) {
			DPRINTF((" --"));
			continue;
		}
		DPRINTF((" %2.2hhx", data[idx]));
		
		cksum = ((cksum & 1) ? 0x80000000 : 0)
			+ (cksum >> 1) + (uint32_t)data[idx];
	}
	DPRINTF((" -> %x\n", (unsigned)cksum));
	
	return cksum;
}

/*
 * Compute the hash of the upcased version of a filename.
 */
uint16_t exfatfs_namehash(struct exfatfs *fs,
			   uint16_t seed, uint16_t *name, int namelen)
{
	uint16_t upcase[EXFATFS_NAMEMAX];

	memcpy(upcase, name, namelen * sizeof(*upcase));
	exfatfs_upcase_str(fs, upcase, namelen);
	return exfatfs_cksum16(seed, (uint8_t *)upcase,
			       namelen * sizeof(*upcase),
			       NULL, 0);
}

/*
 * The 16-bit version is used for filename hash and EntrySet.
 */
uint16_t exfatfs_cksum16(uint16_t seed, uint8_t *data, uint64_t datalen,
			 uint8_t *ignore, uint8_t ignorelen)
{
	uint16_t cksum = seed;
	uint16_t idx;
	int i, ignoring;

	DPRINTF(("%hx ->", cksum));
	for (idx = 0; idx < datalen; idx++)
	{
		ignoring = 0;
		for (i = 0; i < ignorelen; ++i) {
			if (idx == ignore[i]) {
				ignoring = 1;
				break;
			}
		}
		if (ignoring) {
			DPRINTF((" --"));
			continue;
		}
		DPRINTF((" %2.2hhx", data[idx]));
		
		cksum = ((cksum & 1) ? 0x8000 : 0)
			+ (cksum >> 1) + (uint16_t)data[idx];
	}
	DPRINTF((" -> %hx\n", cksum));
	
	return cksum;
}

void htole_bootblock(struct exfatfs *out, struct exfatfs *in)
{
	memcpy(out, in, sizeof(in->xf_JumpBoot)
	       + sizeof(in->xf_FileSystemName) + sizeof(in->xf_MustBeZero));
	out->xf_PartitionOffset = htole64(in->xf_PartitionOffset);
	out->xf_VolumeLength = htole64(in->xf_VolumeLength);
	out->xf_FatOffset = htole32(in->xf_FatOffset);
	out->xf_FatLength = htole32(in->xf_FatLength);
	out->xf_ClusterHeapOffset = htole32(in->xf_ClusterHeapOffset);
	out->xf_ClusterCount = htole32(in->xf_ClusterCount);
	out->xf_FirstClusterOfRootDirectory =
		htole32(in->xf_FirstClusterOfRootDirectory);
	out->xf_VolumeSerialNumber = htole32(in->xf_VolumeSerialNumber);
	out->xf_FileSystemRevision = htole16(in->xf_FileSystemRevision);
	out->xf_VolumeFlags = htole16(in->xf_VolumeFlags);
	out->xf_BytesPerSectorShift = in->xf_BytesPerSectorShift;
	out->xf_SectorsPerClusterShift = in->xf_SectorsPerClusterShift;
	out->xf_NumberOfFats = in->xf_NumberOfFats;
	out->xf_DriveSelect = in->xf_DriveSelect;
	out->xf_PercentInUse = in->xf_PercentInUse;
	memcpy(out->xf_Reserved, in->xf_Reserved, sizeof(in->xf_Reserved) +
	       sizeof(in->xf_BootCode));
	out->xf_BootSignature = htole16(in->xf_BootSignature);
}

void letoh_bootblock(struct exfatfs *out, struct exfatfs *in)
{
	memcpy(out, in, sizeof(in->xf_JumpBoot)
	       + sizeof(in->xf_FileSystemName) + sizeof(in->xf_MustBeZero));
	out->xf_PartitionOffset = le64toh(in->xf_PartitionOffset);
	out->xf_VolumeLength = le64toh(in->xf_VolumeLength);
	out->xf_FatOffset = le32toh(in->xf_FatOffset);
	out->xf_FatLength = le32toh(in->xf_FatLength);
	out->xf_ClusterHeapOffset = le32toh(in->xf_ClusterHeapOffset);
	out->xf_ClusterCount = le32toh(in->xf_ClusterCount);
	out->xf_FirstClusterOfRootDirectory =
		le32toh(in->xf_FirstClusterOfRootDirectory);
	out->xf_VolumeSerialNumber = le32toh(in->xf_VolumeSerialNumber);
	out->xf_FileSystemRevision = le16toh(in->xf_FileSystemRevision);
	out->xf_VolumeFlags = le16toh(in->xf_VolumeFlags);
	out->xf_BytesPerSectorShift = in->xf_BytesPerSectorShift;
	out->xf_SectorsPerClusterShift = in->xf_SectorsPerClusterShift;
	out->xf_NumberOfFats = in->xf_NumberOfFats;
	out->xf_DriveSelect = in->xf_DriveSelect;
	out->xf_PercentInUse = in->xf_PercentInUse;
	memcpy(out->xf_Reserved, in->xf_Reserved, sizeof(in->xf_Reserved) +
	       sizeof(in->xf_BootCode));
	out->xf_BootSignature = le16toh(in->xf_BootSignature);
}

void htole_dfe(struct exfatfs_dfe *out, struct exfatfs_dfe *in)
{
	memcpy(out, in, sizeof(*in));
	out->xd_setChecksum = htole16(in->xd_setChecksum);
	out->xd_fileAttributes = htole16(in->xd_fileAttributes);
	out->xd_createTimestamp = htole32(in->xd_createTimestamp);
	out->xd_lastModifiedTimestamp = htole32(in->xd_lastModifiedTimestamp);
	out->xd_lastAccessedTimestamp = htole32(in->xd_lastAccessedTimestamp);
}

void letoh_dfe(struct exfatfs_dfe *out, struct exfatfs_dfe *in)
{
	memcpy(out, in, sizeof(*in));
	out->xd_setChecksum = le16toh(in->xd_setChecksum);
	out->xd_fileAttributes = le16toh(in->xd_fileAttributes);
	out->xd_createTimestamp = le32toh(in->xd_createTimestamp);
	out->xd_lastModifiedTimestamp = le32toh(in->xd_lastModifiedTimestamp);
	out->xd_lastAccessedTimestamp = le32toh(in->xd_lastAccessedTimestamp);
}
