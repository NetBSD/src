/* $NetBSD: efiblock.c,v 1.4 2018/11/01 00:43:38 jmcneill Exp $ */

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
#include <sys/md5.h>
#include <sys/uuid.h>

#include "efiboot.h"
#include "efiblock.h"

static EFI_HANDLE *efi_block;
static UINTN efi_nblock;
static struct efi_block_part *efi_block_booted = NULL;

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

static void
efi_block_generate_hash_mbr(struct efi_block_part *bpart, struct mbr_sector *mbr)
{
	MD5_CTX md5ctx;

	MD5Init(&md5ctx);
	MD5Update(&md5ctx, (void *)mbr, sizeof(*mbr));
	MD5Final(bpart->hash, &md5ctx);
}

static int
efi_block_find_partitions_disklabel(struct efi_block_dev *bdev, struct mbr_sector *mbr, uint32_t start, uint32_t size)
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

	lba = (((EFI_LBA)start + LABELSECTOR) * DEV_BSIZE) / bdev->bio->Media->BlockSize;
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
		efi_block_generate_hash_mbr(bpart, mbr);
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
			efi_block_find_partitions_disklabel(bdev, &mbr, le32toh(mbr_part->mbrp_start), le32toh(mbr_part->mbrp_size));
			break;
		}
	}

	return 0;
}

static const struct {
	struct uuid guid;
	uint8_t fstype;
} gpt_guid_to_str[] = {
	{ GPT_ENT_TYPE_NETBSD_FFS,		FS_BSDFFS },
	{ GPT_ENT_TYPE_NETBSD_LFS,		FS_BSDLFS },
	{ GPT_ENT_TYPE_NETBSD_RAIDFRAME,	FS_RAID },
	{ GPT_ENT_TYPE_NETBSD_CCD,		FS_CCD },
	{ GPT_ENT_TYPE_NETBSD_CGD,		FS_CGD },
	{ GPT_ENT_TYPE_MS_BASIC_DATA,		FS_MSDOS },	/* or NTFS? ambiguous */
};

static int
efi_block_find_partitions_gpt_entry(struct efi_block_dev *bdev, struct gpt_hdr *hdr, struct gpt_ent *ent, UINT32 index)
{
	struct efi_block_part *bpart;
	uint8_t fstype = FS_UNUSED;
	struct uuid uuid;
	int n;

	memcpy(&uuid, ent->ent_type, sizeof(uuid));
	for (n = 0; n < __arraycount(gpt_guid_to_str); n++)
		if (memcmp(ent->ent_type, &gpt_guid_to_str[n].guid, sizeof(ent->ent_type)) == 0) {
			fstype = gpt_guid_to_str[n].fstype;
			break;
		}
	if (fstype == FS_UNUSED)
		return 0;

	bpart = alloc(sizeof(*bpart));
	bpart->index = index;
	bpart->bdev = bdev;
	bpart->type = EFI_BLOCK_PART_GPT;
	bpart->gpt.fstype = fstype;
	bpart->gpt.ent = *ent;
	memcpy(bpart->hash, ent->ent_guid, sizeof(bpart->hash));
	TAILQ_INSERT_TAIL(&bdev->partitions, bpart, entries);

	return 0;
}

static int
efi_block_find_partitions_gpt(struct efi_block_dev *bdev)
{
	struct gpt_hdr hdr;
	struct gpt_ent ent;
	EFI_STATUS status;
	UINT32 sz, entry;
	uint8_t *buf;

	sz = __MAX(sizeof(hdr), bdev->bio->Media->BlockSize);
	sz = roundup(sz, bdev->bio->Media->BlockSize);
	buf = AllocatePool(sz);
	if (!buf)
		return ENOMEM;

	status = uefi_call_wrapper(bdev->bio->ReadBlocks, 5, bdev->bio, bdev->media_id, GPT_HDR_BLKNO, sz, buf);
	if (EFI_ERROR(status)) {
		FreePool(buf);
		return EIO;
	}
	memcpy(&hdr, buf, sizeof(hdr));
	FreePool(buf);

	if (memcmp(hdr.hdr_sig, GPT_HDR_SIG, sizeof(hdr.hdr_sig)) != 0)
		return ENOENT;
	if (le32toh(hdr.hdr_entsz) < sizeof(ent))
		return EINVAL;

	sz = __MAX(le32toh(hdr.hdr_entsz) * le32toh(hdr.hdr_entries), bdev->bio->Media->BlockSize);
	sz = roundup(sz, bdev->bio->Media->BlockSize);
	buf = AllocatePool(sz);
	if (!buf)
		return ENOMEM;

	status = uefi_call_wrapper(bdev->bio->ReadBlocks, 5, bdev->bio, bdev->media_id, le64toh(hdr.hdr_lba_table), sz, buf);
	if (EFI_ERROR(status)) {
		FreePool(buf);
		return EIO;
	}

	for (entry = 0; entry < le32toh(hdr.hdr_entries); entry++) {
		memcpy(&ent, buf + (entry * le32toh(hdr.hdr_entsz)), sizeof(ent));
		efi_block_find_partitions_gpt_entry(bdev, &hdr, &ent, entry);
	}

	FreePool(buf);

	return 0;
}

static int
efi_block_find_partitions(struct efi_block_dev *bdev)
{
	int error;

	error = efi_block_find_partitions_gpt(bdev);
	if (error)
		error = efi_block_find_partitions_mbr(bdev);

	return error;
}

void
efi_block_probe(void)
{
	struct efi_block_dev *bdev;
	struct efi_block_part *bpart;
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

		efi_block_find_partitions(bdev);

		if (depth > 0 && efi_device_path_ncmp(efi_bootdp, DevicePathFromHandle(efi_block[n]), depth) == 0) {
			TAILQ_FOREACH(bpart, &bdev->partitions, entries) {
				uint8_t fstype = FS_UNUSED;
				switch (bpart->type) {
				case EFI_BLOCK_PART_DISKLABEL:
					fstype = bpart->disklabel.part.p_fstype;
					break;
				case EFI_BLOCK_PART_GPT:
					fstype = bpart->gpt.fstype;
					break;
				}
				if (fstype == FS_BSDFFS) {
					char devname[9];
					snprintf(devname, sizeof(devname), "hd%u%c", bdev->index, bpart->index + 'a');
					set_default_device(devname);
					break;
				}
			}
		}
	}
}

static void
print_guid(const uint8_t *guid)
{
	const int index[] = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };
	int i;

	for (i = 0; i < 16; i++) {
		printf("%02x", guid[index[i]]);
		if (i == 3 || i == 5 || i == 7 || i == 9)
			printf("-");
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
			case EFI_BLOCK_PART_GPT:
				printf("  hd%u%c ", bdev->index, bpart->index + 'a');

				if (bpart->gpt.ent.ent_name[0] == 0x0000) {
					printf("\"");
					print_guid(bpart->gpt.ent.ent_guid);
					printf("\"");
				} else {
					Print(L"\"%s\"", bpart->gpt.ent.ent_name);
				}
			
				/* Size in MB */
				size = (le64toh(bpart->gpt.ent.ent_lba_end) - le64toh(bpart->gpt.ent.ent_lba_start)) * bdev->bio->Media->BlockSize;
				size /= (1024 * 1024);
				if (size >= 10000)
					printf(" (%"PRIu64" GB): ", size / 1024);
				else
					printf(" (%"PRIu64" MB): ", size);

				printf("%s\n", fstypenames[bpart->gpt.fstype]);
				break;
			default:
				break;
			}
		}
	}
}

struct efi_block_part *
efi_block_boot_part(void)
{
	return efi_block_booted;
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

	efi_block_booted = bpart;

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
	case EFI_BLOCK_PART_GPT:
		if (bpart->bdev->bio->Media->BlockSize != DEV_BSIZE) {
			printf("%s: unsupported block size %d (expected %d)\n", __func__,
			    bpart->bdev->bio->Media->BlockSize, DEV_BSIZE);
			return EIO;
		}
		dblk += le64toh(bpart->gpt.ent.ent_lba_start);
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
