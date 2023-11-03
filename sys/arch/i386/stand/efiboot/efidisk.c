/*	$NetBSD: efidisk.c,v 1.9.26.1 2023/11/03 10:01:13 martin Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define FSTYPENAMES	/* for sys/disklabel.h */

#include "efiboot.h"

#include <sys/param.h>	/* for howmany, required by <dev/raidframe/raidframevar.h> */
#include <sys/disklabel.h>
#include <sys/disklabel_gpt.h>

#include "biosdisk.h"
#include "biosdisk_ll.h"
#include "devopen.h"
#include "efidisk.h"

static struct efidiskinfo_lh efi_disklist;
static int nefidisks;

#define MAXDEVNAME 39 /* "NAME=" + 34 char part_name */

#include <dev/raidframe/raidframevar.h>
#define RF_COMPONENT_INFO_OFFSET   16384   /* from sys/dev/raidframe/rf_netbsdkintf.c */
#define RF_COMPONENT_LABEL_VERSION     2   /* from <dev/raidframe/rf_raid.h> */

#define RAIDFRAME_NDEV 16 /* abitrary limit to 15 raidframe devices */
struct efi_raidframe {
       int last_unit;
       int serial;
       const struct efidiskinfo *edi;
       int parent_part;
       char parent_name[MAXDEVNAME + 1];
       daddr_t offset;
       daddr_t size;
};

static void
dealloc_biosdisk_part(struct biosdisk_partition *part, int nparts)
{
	int i;

	for (i = 0; i < nparts; i++) {
		if (part[i].part_name != NULL) {
			dealloc(part[i].part_name, BIOSDISK_PART_NAME_LEN);
			part[i].part_name = NULL;
		}
	}

	dealloc(part, sizeof(*part) * nparts);

	return;
}

void
efi_disk_probe(void)
{
	EFI_STATUS status;
	UINTN i, nhandles;
	EFI_HANDLE *handles;
	EFI_BLOCK_IO *bio;
	EFI_BLOCK_IO_MEDIA *media;
	EFI_DEVICE_PATH *dp;
	struct efidiskinfo *edi;
	int dev, depth = -1;

	TAILQ_INIT(&efi_disklist);

	status = LibLocateHandle(ByProtocol, &BlockIoProtocol, NULL,
	    &nhandles, &handles);
	if (EFI_ERROR(status))
		return;

	if (efi_bootdp != NULL)
		depth = efi_device_path_depth(efi_bootdp, MEDIA_DEVICE_PATH);

	/*
	 * U-Boot incorrectly represents devices with a single
	 * MEDIA_DEVICE_PATH component.  In that case include that
	 * component into the matching, otherwise we'll blindly select
	 * the first device.
	 */
	if (depth == 0)
		depth = 1;

	for (i = 0; i < nhandles; i++) {
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &BlockIoProtocol, (void **)&bio);
		if (EFI_ERROR(status))
			continue;

		media = bio->Media;
		if (media->LogicalPartition || !media->MediaPresent)
			continue;

		edi = alloc(sizeof(struct efidiskinfo));
		memset(edi, 0, sizeof(*edi));
		edi->type = BIOSDISK_TYPE_HD;
		edi->bio = bio;
		edi->media_id = media->MediaId;

		if (efi_bootdp != NULL && depth > 0) {
			status = uefi_call_wrapper(BS->HandleProtocol, 3,
			    handles[i], &DevicePathProtocol, (void **)&dp);
			if (EFI_ERROR(status))
				goto next;
			if (efi_device_path_ncmp(efi_bootdp, dp, depth) == 0) {
				edi->bootdev = true;
				TAILQ_INSERT_HEAD(&efi_disklist, edi,
				    list);
				continue;
			}
		}
next:
		TAILQ_INSERT_TAIL(&efi_disklist, edi, list);
	}

	FreePool(handles);

	if (efi_bootdp_type == BOOT_DEVICE_TYPE_CD) {
		edi = TAILQ_FIRST(&efi_disklist);
		if (edi != NULL && edi->bootdev) {
			edi->type = BIOSDISK_TYPE_CD;
			TAILQ_REMOVE(&efi_disklist, edi, list);
			TAILQ_INSERT_TAIL(&efi_disklist, edi, list);
		}
	}

	dev = 0x80;
	TAILQ_FOREACH(edi, &efi_disklist, list) {
		edi->dev = dev++;
		if (edi->type == BIOSDISK_TYPE_HD)
			nefidisks++;
		if (edi->bootdev)
			boot_biosdev = edi->dev;
	}
}

static void
efi_raidframe_probe(struct efi_raidframe *raidframe, int *raidframe_count,
		    const struct efidiskinfo *edi,
		    struct biosdisk_partition *part, int parent_part)

{
	int i = *raidframe_count;
	struct RF_ComponentLabel_s label;

	if (i + 1 > RAIDFRAME_NDEV)
		return;

	if (biosdisk_read_raidframe(edi->dev, part->offset, &label) != 0)
		return;

	if (label.version != RF_COMPONENT_LABEL_VERSION)
		return;

	raidframe[i].last_unit = label.last_unit;
	raidframe[i].serial = label.serial_number;
	raidframe[i].edi = edi;
	raidframe[i].parent_part = parent_part;
	if (part->part_name)
		strlcpy(raidframe[i].parent_name, part->part_name, MAXDEVNAME);
	else
		raidframe[i].parent_name[0] = '\0';
	raidframe[i].offset = part->offset;
	raidframe[i].size = label.__numBlocks;

	(*raidframe_count)++;

	return;
}

void
efi_disk_show(void)
{
	const struct efidiskinfo *edi;
	struct efi_raidframe raidframe[RAIDFRAME_NDEV];
	int raidframe_count = 0;
	EFI_BLOCK_IO_MEDIA *media;
	struct biosdisk_partition *part;
	uint64_t size;
	int i, j, nparts;
	bool first;

	TAILQ_FOREACH(edi, &efi_disklist, list) {
		media = edi->bio->Media;
		first = true;
		printf("disk ");
		switch (edi->type) {
		case BIOSDISK_TYPE_CD:
			printf("cd0");
			printf(" mediaId %u", media->MediaId);
			if (edi->media_id != media->MediaId)
				printf("(%u)", edi->media_id);
			printf("\n");
			printf("  cd0a\n");
			break;
		case BIOSDISK_TYPE_HD:
			printf("hd%d", edi->dev & 0x7f);
			printf(" mediaId %u", media->MediaId);
			if (edi->media_id != media->MediaId)
				printf("(%u)", edi->media_id);
			printf(" size ");
			size = (media->LastBlock + 1) * media->BlockSize;
			if (size >= (10ULL * 1024 * 1024 * 1024))
				printf("%"PRIu64" GB", size / (1024 * 1024 * 1024));
			else
				printf("%"PRIu64" MB", size / (1024 * 1024));
			printf("\n");
			break;
		}
		if (edi->type != BIOSDISK_TYPE_HD)
			continue;

		if (biosdisk_readpartition(edi->dev, 0, 0, &part, &nparts))
			continue;

		for (i = 0; i < nparts; i++) {
			if (part[i].size == 0)
				continue;
			if (part[i].fstype == FS_UNUSED)
				continue;
			if (part[i].fstype == FS_RAID) {
				efi_raidframe_probe(raidframe, &raidframe_count,
						    edi, &part[i], i);
			}
			if (first) {
				printf(" ");
				first = false;
			}
			if (part[i].part_name && part[i].part_name[0])
				printf(" NAME=%s(", part[i].part_name);
			else
				printf(" hd%d%c(", edi->dev & 0x7f, i + 'a');
			if (part[i].guid != NULL)
				printf("%s", part[i].guid->name);
			else if (part[i].fstype < FSMAXTYPES)
				printf("%s", fstypenames[part[i].fstype]);
			else
				printf("%d", part[i].fstype);
			printf(")");
		}
		if (!first)
			printf("\n");
		dealloc_biosdisk_part(part, nparts);
	}

	for (i = 0; i < raidframe_count; i++) {
		size_t secsize = raidframe[i].edi->bio->Media->BlockSize;
		printf("raidframe raid%d serial %d in ",
		       raidframe[i].last_unit, raidframe[i].serial);
		if (raidframe[i].parent_name[0])
			printf("NAME=%s size ", raidframe[i].parent_name);
		else
			printf("hd%d%c size ",
			       raidframe[i].edi->dev & 0x7f,
			       raidframe[i].parent_part + 'a');
		if (raidframe[i].size >= (10ULL * 1024 * 1024 * 1024 / secsize))
			printf("%"PRIu64" GB",
			    raidframe[i].size / (1024 * 1024 * 1024 / secsize));
		else
			printf("%"PRIu64" MB",
			    raidframe[i].size / (1024 * 1024 / secsize));
		printf("\n");

		if (biosdisk_readpartition(raidframe[i].edi->dev,
		    raidframe[i].offset + RF_PROTECTED_SECTORS,
		    raidframe[i].size,
		    &part, &nparts))
			continue;

		first = 1;
		for (j = 0; j < nparts; j++) {
			bool bootme = part[j].attr & GPT_ENT_ATTR_BOOTME;

			if (part[j].size == 0)
				continue;
			if (part[j].fstype == FS_UNUSED)
				continue;
			if (part[j].fstype == FS_RAID) /* raid in raid? */
				continue;
			if (first) {
				printf(" ");
				first = 0;
			}
			if (part[j].part_name && part[j].part_name[0])
				printf(" NAME=%s(", part[j].part_name);
			else
				printf(" raid%d%c(",
				       raidframe[i].last_unit, j + 'a');
			if (part[j].guid != NULL)
				printf("%s", part[j].guid->name);
			else if (part[j].fstype < FSMAXTYPES)
				printf("%s",
				  fstypenames[part[j].fstype]);
			else
				printf("%d", part[j].fstype);
			printf("%s)", bootme ? ", bootme" : "");
		}

		if (first == 0)
			printf("\n");

		dealloc_biosdisk_part(part, nparts);
	}
}

const struct efidiskinfo *
efidisk_getinfo(int dev)
{
	const struct efidiskinfo *edi;

	TAILQ_FOREACH(edi, &efi_disklist, list) {
		if (dev == edi->dev)
			return edi;
	}
	return NULL;
}

/*
 * Return the number of hard disk drives.
 */
int
get_harddrives(void)
{
	return nefidisks;
}

int
efidisk_get_efi_system_partition(int dev, int *partition)
{
	extern const struct uuid GET_efi;
	const struct efidiskinfo *edi;
	struct biosdisk_partition *part;
	int i, nparts;

	edi = efidisk_getinfo(dev);
	if (edi == NULL)
		return ENXIO;

	if (edi->type != BIOSDISK_TYPE_HD)
		return ENOTSUP;

	if (biosdisk_readpartition(edi->dev, 0, 0, &part, &nparts))
		return EIO;

	for (i = 0; i < nparts; i++) {
		if (part[i].size == 0)
			continue;
		if (part[i].fstype == FS_UNUSED)
			continue;
		if (guid_is_equal(part[i].guid->guid, &GET_efi))
			break;
	}
	dealloc_biosdisk_part(part, nparts);
	if (i == nparts)
		return ENOENT;

	*partition = i;
	return 0;
}
