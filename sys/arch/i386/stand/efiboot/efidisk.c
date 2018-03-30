/*	$NetBSD: efidisk.c,v 1.1.20.3 2018/03/30 06:20:11 pgoyette Exp $	*/

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

#include <sys/disklabel.h>

#include "biosdisk.h"
#include "biosdisk_ll.h"
#include "devopen.h"
#include "efidisk.h"

static struct efidiskinfo_lh efi_disklist;
static int nefidisks;

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
		panic("LocateHandle(BlockIoProtocol): %" PRIxMAX,
		    (uintmax_t)status);

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
			panic("HandleProtocol(BlockIoProtocol): %" PRIxMAX,
			    (uintmax_t)status);

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

	if (efi_bootdp_type == BIOSDISK_TYPE_CD) {
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

void
efi_disk_show(void)
{
	const struct efidiskinfo *edi;
	EFI_BLOCK_IO_MEDIA *media;
	struct biosdisk_partition *part;
	uint64_t size;
	int i, nparts;
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

		if (biosdisk_readpartition(edi->dev, &part, &nparts))
			continue;

		for (i = 0; i < nparts; i++) {
			if (part[i].size == 0)
				continue;
			if (part[i].fstype == FS_UNUSED)
				continue;
			if (first) {
				printf(" ");
				first = false;
			}
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
		dealloc(part, sizeof(*part) * nparts);
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
