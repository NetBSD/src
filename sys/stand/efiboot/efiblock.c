/* $NetBSD: efiblock.c,v 1.1 2018/08/26 21:28:18 jmcneill Exp $ */

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#define FSTYPENAMES

#include <sys/param.h>

#include "efiboot.h"
#include "efiblock.h"

static EFI_HANDLE *efi_block;
static UINTN efi_nblock;

static TAILQ_HEAD(, efi_block_dev) efi_block_devs = TAILQ_HEAD_INITIALIZER(efi_block_devs);

static int
efi_block_parse(const char *fname, struct efi_block_part **pbpart, char **pfile)
{
	struct efi_block_dev *bdev;
	struct efi_block_part *bpart;
	char pathbuf[PATH_MAX], *default_device, *ep = NULL;
	const char *full_path;
	intmax_t dev;
	int part;

	default_device = get_default_device();
	if (strchr(fname, ':') == NULL) {
		if (strlen(default_device) > 0) {
			snprintf(pathbuf, sizeof(pathbuf), "%s:%s", default_device, fname);
			full_path = pathbuf;
			*pfile = __UNCONST(fname);
		} else {
			return EINVAL;
		}
	} else {
		full_path = fname;
		*pfile = strchr(fname, ':') + 1;
	}

	if (strncasecmp(full_path, "hd", 2) != 0)
		return EINVAL;
	dev = strtoimax(full_path + 2, &ep, 10);
	if (dev < 0 || dev >= efi_nblock)
		return ENXIO;
	if (ep[0] < 'a' || ep[0] >= 'a' + MAXPARTITIONS || ep[1] != ':')
		return EINVAL;
	part = ep[0] - 'a';
	TAILQ_FOREACH(bdev, &efi_block_devs, entries) {
		if (bdev->index == dev) {
			TAILQ_FOREACH(bpart, &bdev->partitions, entries) {
				if (bpart->index == part) {
					*pbpart = bpart;
					return 0;
				}
			}
		}
	}

	return ENOENT;
}

static int
efi_block_find_partitions_disklabel(struct efi_block_dev *bdev, uint32_t start, uint32_t size)
{
	struct efi_block_part *bpart;
	struct disklabel d;
	struct partition *p;
	EFI_STATUS status;
	EFI_LBA lba;
	uint8_t *buf;
	UINT32 sz;
	int n;

	sz = __MAX(sizeof(d), bdev->bio->Media->BlockSize);
	sz = roundup(sz, bdev->bio->Media->BlockSize);
	buf = AllocatePool(sz);
	if (!buf)
		return ENOMEM;

	lba = ((start + LABELSECTOR) * DEV_BSIZE) / bdev->bio->Media->BlockSize;
	status = uefi_call_wrapper(bdev->bio->ReadBlocks, 5, bdev->bio, bdev->media_id, lba, sz, buf);
	if (EFI_ERROR(status) || getdisklabel(buf, &d) != NULL) {
		FreePool(buf);
		return EIO;
	}
	FreePool(buf);

	if (le32toh(d.d_magic) != DISKMAGIC || le32toh(d.d_magic2) != DISKMAGIC)
		return EINVAL;
	if (le16toh(d.d_npartitions) > MAXPARTITIONS)
		return EINVAL;

	for (n = 0; n < le16toh(d.d_npartitions); n++) {
		p = &d.d_partitions[n];
		switch (p->p_fstype) {
		case FS_BSDFFS:
		case FS_MSDOS:
		case FS_BSDLFS:
			break;
		default:
			continue;
		}

		bpart = alloc(sizeof(*bpart));
		bpart->index = n;
		bpart->bdev = bdev;
		bpart->type = EFI_BLOCK_PART_DISKLABEL;
		bpart->disklabel.secsize = le32toh(d.d_secsize);
		bpart->disklabel.part = *p;
		TAILQ_INSERT_TAIL(&bdev->partitions, bpart, entries);
	}

	return 0;
}

static int
efi_block_find_partitions_mbr(struct efi_block_dev *bdev)
{
	struct mbr_sector mbr;
	struct mbr_partition *mbr_part;
	EFI_STATUS status;
	uint8_t *buf;
	UINT32 sz;
	int n;

	sz = __MAX(sizeof(mbr), bdev->bio->Media->BlockSize);
	sz = roundup(sz, bdev->bio->Media->BlockSize);
	buf = AllocatePool(sz);
	if (!buf)
		return ENOMEM;

	status = uefi_call_wrapper(bdev->bio->ReadBlocks, 5, bdev->bio, bdev->media_id, 0, sz, buf);
	if (EFI_ERROR(status)) {
		FreePool(buf);
		return EIO;
	}
	memcpy(&mbr, buf, sizeof(mbr));
	FreePool(buf);

	if (le32toh(mbr.mbr_magic) != MBR_MAGIC)
		return ENOENT;

	for (n = 0; n < MBR_PART_COUNT; n++) {
		mbr_part = &mbr.mbr_parts[n];
		if (le32toh(mbr_part->mbrp_size) == 0)
			continue;
		if (mbr_part->mbrp_type == MBR_PTYPE_NETBSD) {
			efi_block_find_partitions_disklabel(bdev, le32toh(mbr_part->mbrp_start), le32toh(mbr_part->mbrp_size));
			break;
		}
	}

	return 0;
}

static int
efi_block_find_partitions(struct efi_block_dev *bdev)
{
	return efi_block_find_partitions_mbr(bdev);
}

void
efi_block_probe(void)
{
	struct efi_block_dev *bdev;
	EFI_BLOCK_IO *bio;
	EFI_STATUS status;
	uint16_t devindex = 0;
	int depth = -1;
	int n;

	status = LibLocateHandle(ByProtocol, &BlockIoProtocol, NULL, &efi_nblock, &efi_block);
	if (EFI_ERROR(status))
		return;

	if (efi_bootdp) {
		depth = efi_device_path_depth(efi_bootdp, MEDIA_DEVICE_PATH);
		if (depth == 0)
			depth = 1;
	}

	for (n = 0; n < efi_nblock; n++) {
		status = uefi_call_wrapper(BS->HandleProtocol, 3, efi_block[n], &BlockIoProtocol, (void **)&bio);
		if (EFI_ERROR(status) || !bio->Media->MediaPresent)
			continue;

		if (bio->Media->LogicalPartition)
			continue;

		bdev = alloc(sizeof(*bdev));
		bdev->index = devindex++;
		bdev->bio = bio;
		bdev->media_id = bio->Media->MediaId;
		bdev->path = DevicePathFromHandle(efi_block[n]);
		TAILQ_INIT(&bdev->partitions);
		TAILQ_INSERT_TAIL(&efi_block_devs, bdev, entries);

		if (depth > 0 && efi_device_path_ncmp(efi_bootdp, DevicePathFromHandle(efi_block[n]), depth) == 0) {
			char devname[9];
			snprintf(devname, sizeof(devname), "hd%ua", bdev->index);
			set_default_device(devname);
		}

		efi_block_find_partitions(bdev);
	}
}

void
efi_block_show(void)
{
	struct efi_block_dev *bdev;
	struct efi_block_part *bpart;
	uint64_t size;
	CHAR16 *path;

	TAILQ_FOREACH(bdev, &efi_block_devs, entries) {
		printf("hd%u (", bdev->index);

		/* Size in MB */
		size = ((bdev->bio->Media->LastBlock + 1) * bdev->bio->Media->BlockSize) / (1024 * 1024);
		if (size >= 10000)
			printf("%"PRIu64" GB", size / 1024);
		else
			printf("%"PRIu64" MB", size);
		printf("): ");

		path = DevicePathToStr(bdev->path);
		Print(L"%s", path);
		FreePool(path);

		printf("\n");

		TAILQ_FOREACH(bpart, &bdev->partitions, entries) {
			switch (bpart->type) {
			case EFI_BLOCK_PART_DISKLABEL:
				printf("  hd%u%c (", bdev->index, bpart->index + 'a');

				/* Size in MB */
				size = ((uint64_t)bpart->disklabel.secsize * bpart->disklabel.part.p_size) / (1024 * 1024);
				if (size >= 10000)
					printf("%"PRIu64" GB", size / 1024);
				else
					printf("%"PRIu64" MB", size);
				printf("): ");

				printf("%s\n", fstypenames[bpart->disklabel.part.p_fstype]);
				break;
			default:
				break;
			}
		}
	}
}

int
efi_block_open(struct open_file *f, ...)
{
	struct efi_block_part *bpart;
	const char *fname;
	char **file;
	char *path;
	va_list ap;
	int rv, n;
	
	va_start(ap, f);
	fname = va_arg(ap, const char *);
	file = va_arg(ap, char **);
	va_end(ap);

	rv = efi_block_parse(fname, &bpart, &path);
	if (rv != 0)
		return rv;

	for (n = 0; n < ndevs; n++)
		if (strcmp(DEV_NAME(&devsw[n]), "efiblock") == 0) {
			f->f_dev = &devsw[n];
			break;
		}
	if (n == ndevs)
		return ENXIO;

	f->f_devdata = bpart;

	*file = path;

	return 0;
}

int
efi_block_close(struct open_file *f)
{
	return 0;
}

int
efi_block_strategy(void *devdata, int rw, daddr_t dblk, size_t size, void *buf, size_t *rsize)
{
	struct efi_block_part *bpart = devdata;
	EFI_STATUS status;

	if (rw != F_READ)
		return EROFS;

	switch (bpart->type) {
	case EFI_BLOCK_PART_DISKLABEL:
		if (bpart->bdev->bio->Media->BlockSize != bpart->disklabel.secsize) {
			printf("%s: unsupported block size %d (expected %d)\n", __func__,
			    bpart->bdev->bio->Media->BlockSize, bpart->disklabel.secsize);
			return EIO;
		}
		dblk += bpart->disklabel.part.p_offset;
		break;
	default:
		return EINVAL;
	}

	status = uefi_call_wrapper(bpart->bdev->bio->ReadBlocks, 5, bpart->bdev->bio, bpart->bdev->media_id, dblk, size, buf);
	if (EFI_ERROR(status))
		return EIO;

	*rsize = size;

	return 0;
}
