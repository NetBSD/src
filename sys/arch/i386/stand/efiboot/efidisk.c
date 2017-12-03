/*	$NetBSD: efidisk.c,v 1.1.18.2 2017/12/03 11:36:18 jdolecek Exp $	*/

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

#include "efiboot.h"

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
	struct efidiskinfo *edi;
	EFI_DEVICE_PATH *dp, *bdp;
	bool bootdev;
	int dev;

	TAILQ_INIT(&efi_disklist);

	status = LibLocateHandle(ByProtocol, &BlockIoProtocol, NULL,
	    &nhandles, &handles);
	if (EFI_ERROR(status))
		Panic(L"LocateHandle(BlockIoProtocol): %r", status);

	for (i = 0; i < nhandles; i++) {
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &BlockIoProtocol, (void **)&bio);
		if (EFI_ERROR(status))
			Panic(L"HandleProtocol(BlockIoProtocol): %r", status);

		media = bio->Media;
		if (media->LogicalPartition || !media->MediaPresent)
			continue;

		edi = alloc(sizeof(struct efidiskinfo));
		memset(edi, 0, sizeof(*edi));
		edi->bio = bio;
		edi->media_id = media->MediaId;

		bootdev = false;
		if (efi_bootdp == NULL)
			goto next;

		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &DevicePathProtocol, (void **)&dp);
		if (EFI_ERROR(status))
			goto next;

		bdp = efi_bootdp;
		for (;;) {
			if (IsDevicePathEnd(dp)) {
				bootdev = true;
				break;
			}
			if (memcmp(dp, bdp, sizeof(EFI_DEVICE_PATH)) != 0 ||
			    memcmp(dp, bdp, DevicePathNodeLength(dp)) != 0)
				break;
			dp = NextDevicePathNode(dp);
			bdp = NextDevicePathNode(bdp);
		}
next:
		if (bootdev)
			TAILQ_INSERT_HEAD(&efi_disklist, edi, list);
		else
			TAILQ_INSERT_TAIL(&efi_disklist, edi, list);
	}

	FreePool(handles);

	dev = 0x80;
	TAILQ_FOREACH(edi, &efi_disklist, list) {
		edi->dev = dev++;
		nefidisks++;
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
