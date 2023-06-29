/*	$NetBSD: boot1.c,v 1.22 2023/06/29 14:18:58 manu Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
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
__RCSID("$NetBSD: boot1.c,v 1.22 2023/06/29 14:18:58 manu Exp $");

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <biosdisk_ll.h>

#include <sys/param.h>
#include <sys/uuid.h>
#include <sys/bootblock.h>
#include <sys/disklabel.h>
#include <sys/disklabel_gpt.h>
#include <dev/raidframe/raidframevar.h>	/* For RF_PROTECTED_SECTORS */

#define XSTR(x) #x
#define STR(x) XSTR(x)

static daddr_t bios_sector;

static struct biosdisk_ll d;

const char *boot1(uint32_t, uint64_t *);
#ifndef NO_GPT
static daddr_t gpt_lookup(daddr_t);
#endif
extern void putstr(const char *);

extern struct disklabel ptn_disklabel;

static int
ob(void)
{
	return open("boot", 0);
}

const char *
boot1(uint32_t biosdev, uint64_t *sector)
{
	struct stat sb;
	int fd;

	bios_sector = *sector;
	d.dev = biosdev;

	putstr("\r\nNetBSD/x86 " STR(FS) " Primary Bootstrap\r\n");

	if (set_geometry(&d, NULL))
		return "set_geometry\r\n";

	/*
	 * We default to the filesystem at the start of the
	 * MBR partition
	 */
	fd = ob();
	if (fd != -1)
		goto done;
	/*
	 * Maybe the filesystem is enclosed in a raid set.
	 * add in size of raidframe header and try again.
	 * (Maybe this should only be done if the filesystem
	 * magic number is absent.)
	 */
	bios_sector += RF_PROTECTED_SECTORS;
	fd = ob();
	if (fd != -1)
		goto done;

#ifndef NO_GPT
	/*
	 * Test for a GPT inside the RAID
	 */
	bios_sector += gpt_lookup(bios_sector);
	fd = ob();
	if (fd != -1)
		goto done;
#endif

	/*
	 * Nothing at the start of the MBR partition, fallback on
	 * partition 'a' from the disklabel in this MBR partition.
	 */
	if (ptn_disklabel.d_magic != DISKMAGIC ||
	    ptn_disklabel.d_magic2 != DISKMAGIC ||
	    ptn_disklabel.d_partitions[0].p_fstype == FS_UNUSED)
		goto done;
	bios_sector = ptn_disklabel.d_partitions[0].p_offset;
	*sector = bios_sector;
	if (ptn_disklabel.d_partitions[0].p_fstype == FS_RAID)
		bios_sector += RF_PROTECTED_SECTORS;

	fd = ob();

done:
	if (fd == -1 || fstat(fd, &sb) == -1)
		return "Can't open /boot\r\n";

	biosdev = (uint32_t)sb.st_size;
#if 0
	if (biosdev > SECONDARY_MAX_LOAD)
		return "/boot too large\r\n";
#endif

	if (read(fd, (void *)SECONDARY_LOAD_ADDRESS, biosdev) != biosdev)
		return "/boot load failed\r\n";

	if (*(uint32_t *)(SECONDARY_LOAD_ADDRESS + 4) != X86_BOOT_MAGIC_2)
		return "Invalid /boot file format\r\n";

	/* We need to jump to the secondary bootstrap in realmode */
	return 0;
}

int
blkdevstrategy(void *devdata, int flag, daddr_t dblk, size_t size, void *buf, size_t *rsize)
{
	if (flag != F_READ)
		return EROFS;

	if (size & (BIOSDISK_DEFAULT_SECSIZE - 1))
		return EINVAL;

	if (rsize)
		*rsize = size;

	if (size != 0 && readsects(&d, bios_sector + dblk,
				   size / BIOSDISK_DEFAULT_SECSIZE,
				   buf, 1) != 0)
		return EIO;

	return 0;
}

#ifndef NO_GPT
static int
is_unused(struct gpt_ent *ent)
{
	const struct uuid unused = GPT_ENT_TYPE_UNUSED;

	return (memcmp(ent->ent_type, &unused, sizeof(unused)) == 0);
}

static int
is_bootable(struct gpt_ent *ent)
{
	/* GPT_ENT_TYPE_NETBSD_RAID omitted as we are already in a RAID */
	const struct uuid bootable[] = {
		GPT_ENT_TYPE_NETBSD_FFS,
		GPT_ENT_TYPE_NETBSD_LFS,
		GPT_ENT_TYPE_NETBSD_CCD,
		GPT_ENT_TYPE_NETBSD_CGD,
	};
	int i;

	for (i = 0; i < sizeof(bootable) / sizeof(*bootable); i++) {
		if (memcmp(ent->ent_type, &bootable[i],
		    sizeof(struct uuid)) == 0)
			return 1;
	}

	return 0;
}

static daddr_t
gpt_lookup(daddr_t sector)
{
	char buf[BIOSDISK_DEFAULT_SECSIZE];
	struct mbr_sector *pmbr;	
	const char gpt_hdr_sig[] = GPT_HDR_SIG;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	uint32_t nents;
	uint32_t entsz;
	uint32_t entries_per_sector;
	uint32_t sectors_per_entry;
	uint64_t firstpart_lba = 0;
	uint64_t bootable_lba = 0;
	uint64_t bootme_lba = 0;
	int i, j;

	/*
	 * Look for a PMBR
	 */
	if (readsects(&d, sector, 1, buf, 1) != 0)
		return 0;

	pmbr = (struct mbr_sector *)buf;

	if (pmbr->mbr_magic != htole16(MBR_MAGIC))
		return 0;

	if (pmbr->mbr_parts[0].mbrp_type != MBR_PTYPE_PMBR)
		return 0;

	sector++; /* skip PMBR */

	/*
	 * Look for a GPT header
	 * Space is scarce, we do not check CRC.
	 */
	if (readsects(&d, sector, 1, buf, 1) != 0)
		return 0;

	hdr = (struct gpt_hdr *)buf;

	if (memcmp(gpt_hdr_sig, hdr->hdr_sig, sizeof(hdr->hdr_sig)) != 0)
		return 0;

	if (hdr->hdr_revision != htole32(GPT_HDR_REVISION))
		return 0;

	if (le32toh(hdr->hdr_size) > BIOSDISK_DEFAULT_SECSIZE)
		return 0;

	nents = le32toh(hdr->hdr_entries);
	entsz = le32toh(hdr->hdr_entsz);

	sector++; /* skip GPT header */

	/*
	 * Read partition table
	 *
	 * According to UEFI specification section 5.3.2, entries
	 * are 128 * (2^n) bytes long. The most common scenario is
	 * 128 bytes (n = 0) where there are 4 entries per sector.
	 * If n > 2, then entries spans multiple sectors, but they
	 * remain sector-aligned.
	 */
	entries_per_sector = BIOSDISK_DEFAULT_SECSIZE / entsz;
	if (entries_per_sector == 0)
		entries_per_sector = 1;

	sectors_per_entry = entsz / BIOSDISK_DEFAULT_SECSIZE;
	if (sectors_per_entry == 0)
		sectors_per_entry = 1;

	for (i = 0; i < nents; i += entries_per_sector) {
		if (readsects(&d, sector, 1, buf, 1) != 0)
			return 0;

		sector += sectors_per_entry;

		for (j = 0; j < entries_per_sector; j++) {
			ent = (struct gpt_ent *)&buf[j * entsz];

			if (is_unused(ent))
				continue;

			/* First bootme wins, we can stop there */
			if (ent->ent_attr & GPT_ENT_ATTR_BOOTME) {
				bootme_lba = le64toh(ent->ent_lba_start);
				goto out;
			}

			if (firstpart_lba == 0)
				firstpart_lba = le64toh(ent->ent_lba_start);

			if (is_bootable(ent) && bootable_lba == 0)
				bootable_lba = le64toh(ent->ent_lba_start);
		}
	}

out:
	if (bootme_lba)
		return bootme_lba;

	if (bootable_lba)
		return bootable_lba;

	if (firstpart_lba)
		return firstpart_lba;

	return 0;
}
#endif /* ! NO_GPT */
