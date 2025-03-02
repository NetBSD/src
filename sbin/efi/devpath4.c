/* $NetBSD: devpath4.c,v 1.5 2025/03/02 00:23:59 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: devpath4.c,v 1.5 2025/03/02 00:23:59 riastradh Exp $");
#endif /* not lint */

#include <sys/uuid.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "defs.h"
#include "devpath.h"
#include "devpath4.h"
#include "utils.h"

#define easprintf	(size_t)easprintf

#define EFI_VIRTUAL_DISK_GUID \
    {0x77AB535A,0x45FC,0x624B,0x55,0x60,{0xF7,0xB2,0x81,0xD1,0xF9,0x6E}}

#define EFI_VIRTUAL_CD_GUID \
    {0x3D5ABD30,0x4175,0x87CE,0x6D,0x64,{0xD2,0xAD,0xE5,0x23,0xC4,0xBB}}

#define EFI_PERSISTENT_VIRTUAL_DISK_GUID \
    {0x5CEA02C9,0x4D07,0x69D3,0x26,0x9F,{0x44,0x96,0xFB,0xE0,0x96,0xF9}}

#define EFI_PERSISTENT_VIRTUAL_CD_GUID \
    {0x08018188,0x42CD,0xBB48,0x10,0x0F,{0x53,0x87,0xD5,0x3D,0xED,0x3D}}

/************************************************************************
 * Type 4 - Media Device Path
 ************************************************************************/

static void
devpath_media_harddisk(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.1 */
	struct {			/* Sub-Type 1 */
		devpath_t	hdr;	/* Length 42 */
		uint32_t	PartitionNumber;
		uint64_t	PartitionStart;
		uint64_t	PartitionSize;
		uuid_t		PartitionSignature;
		uint8_t		PartitionFormat;
#define PARTITION_FORMAT_MBR	0x01
#define PARTITION_FORMAT_GPT	0x02
		uint8_t		SignatureType;
#define SIGNATURE_TYPE_NONE	0x00
#define SIGNATURE_TYPE_MBR	0x01
#define SIGNATURE_TYPE_GUID	0x02
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 42);
	char uuid_str[UUID_STR_LEN];
	const char *name_PartitionFormat;

	uuid_snprintf(uuid_str, sizeof(uuid_str),
	    &p->PartitionSignature);

	switch (p->PartitionFormat) {
	case PARTITION_FORMAT_MBR:	name_PartitionFormat = "MBR";	break;
	case PARTITION_FORMAT_GPT:	name_PartitionFormat = "GPT";	break;
	default:			name_PartitionFormat = "unknown";	break;
	}

	path->sz = easprintf(&path->cp, "HD(%u,%s,%s,0x%" PRIx64 ",0x%"
	    PRIx64 ")",
	    p->PartitionNumber,
	    name_PartitionFormat,
	    uuid_str,
	    p->PartitionStart,
	    p->PartitionSize);

	if (dbg != NULL) {
		const char *name_SignatureType;

		switch (p->SignatureType) {
		case SIGNATURE_TYPE_NONE:	name_SignatureType = "NONE";	break;
		case SIGNATURE_TYPE_MBR:	name_SignatureType = "MBR";	break;
		case SIGNATURE_TYPE_GUID:	name_SignatureType = "GUID";	break;
		default:			name_SignatureType = "unknown";	break;
		}

		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(PartitionNumber: %u\n)
		    DEVPATH_FMT(PartitionStart:) "  0x%" PRIx64 "\n"
		    DEVPATH_FMT(PartitionSize:) "   0x%" PRIx64 "\n"
		    DEVPATH_FMT(PartitionSignature: %s\n)
		    DEVPATH_FMT(PartitionFormat: %s(%u)\n)
		    DEVPATH_FMT(SignatureType: %s(%u)\n),
		    DEVPATH_DAT_HDR(dp),
		    p->PartitionNumber,
		    p->PartitionStart,
		    p->PartitionSize,
		    uuid_str,
		    name_PartitionFormat,
		    p->PartitionFormat,
		    name_SignatureType,
		    p->SignatureType);
	}
}

static void
devpath_media_cdrom(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.2 */
	struct {			/* Sub-Type 2 */
		devpath_t	hdr;	/* Length 24 */
		uint32_t	BootEntry;
		uint64_t	PartitionStart;
		uint64_t	PartitionSize;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);

	path->sz = easprintf(&path->cp, "CDROM(0x%x,0x%016" PRIx64 ",0x%016"
	    PRIx64 ")", p->BootEntry,
	    p->PartitionStart, p->PartitionSize);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(BootEntry: 0x%04x\n)
		    DEVPATH_FMT(PartitionStart:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(PartitionSize:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->BootEntry,
		    p->PartitionStart,
		    p->PartitionSize);
	}
}

static void
devpath_media_vendor(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.3 */

	struct {			/* Sub-Type 3 */
		devpath_t	hdr;	/* Length = 20 + n */
		uuid_t		VendorGUID;
		uint8_t		vendorData[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->VendorGUID);

	path->sz = easprintf(&path->cp, "VenMedia(%s)", uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(VendorGUID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str);
	}
}

static void
devpath_media_filepath(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.4 */
	struct {			/* Sub-Type 4 */
		devpath_t	hdr;	/* Length = 4 + n */
		uint16_t	PathName[];
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 4);
	char *PathName;
	size_t sz = p->hdr.Length - sizeof(*p);

	PathName = ucs2_to_utf8(p->PathName, sz, NULL, NULL);
	path->sz = easprintf(&path->cp, "File(%s)", PathName);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(PathName: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    PathName);
	}
	free(PathName);
}

static void
devpath_media_protocol(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.5 */
	struct {			/* Sub-Type 5 */
		devpath_t	hdr;	/* Length 20 */
		uuid_t		ProtocolGUID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->ProtocolGUID);
	path->sz = easprintf(&path->cp, "Media(%s)", uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(ProtocolGUID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str);
	}
}

static void
devpath_media_PIWGfwfile(devpath_t *dp, devpath_elm_t *path,
    devpath_elm_t *dbg)
{	/* See 10.3.5.6 */
	/* See UEFI PI Version 1.8 Errata A (March 5, 2024) II-8.3 */
	struct {			/* Sub-Type 6 */
		devpath_t	hdr;	/* Length 20 */
		uuid_t		FWFileName;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->FWFileName);
	path->sz = easprintf(&path->cp, "FvFile(%s)", uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(FirmwareFileName: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str);
	}
}

static void
devpath_media_PIWGfwvol(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.7 */
	/* See UEFI PI Version 1.8 Errata A (March 5, 2024) II-8.2 */
	struct {			/* Sub-Type 7 */
		devpath_t	hdr;	/* Length 20 */
		uuid_t		GUID;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 20);
	char uuid_str[UUID_STR_LEN];

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);
	path->sz = easprintf(&path->cp, "Fv(%s)", uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(GUID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    uuid_str);
	}
}

static void
devpath_media_reloff(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.8 */
	struct {			/* Sub-Type 8 */
		devpath_t	hdr;	/* Length 24 */
		uint32_t	Reserved;
		uint64_t	StartingOffset;
		uint64_t	EndingOffset;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 24);

	path->sz = easprintf(&path->cp, "Offset(0x%016" PRIx64 ",0x%016"
	    PRIx64 ")",
	    p->StartingOffset, p->EndingOffset);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(Reserved: 0x%08x\n)
		    DEVPATH_FMT(StartingOffset:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(EndingOffset:) " 0x%016" PRIx64 "\n",
		    DEVPATH_DAT_HDR(dp),
		    p->Reserved,
		    p->StartingOffset,
		    p->EndingOffset);
	}
}

static const char *
devpath_media_ramdisk_type(uuid_t *uuid)
{
	static struct {
		uuid_t GUID;
		const char *type;
	} tbl[] = {
		{ .GUID = EFI_VIRTUAL_DISK_GUID,
		  .type = "VirtualDisk",
		},
		{ .GUID = EFI_VIRTUAL_CD_GUID,
		  .type = "VirtualCD",
		},
		{ .GUID = EFI_PERSISTENT_VIRTUAL_DISK_GUID,
		  .type = "PersistentVirtualDisk",
		},
		{ .GUID =EFI_PERSISTENT_VIRTUAL_CD_GUID,
		  .type = "PersistentVirtualCD",
		},
	};

	for (size_t i = 0; i < __arraycount(tbl); i++) {
		if (memcmp(uuid, &tbl[i].GUID, sizeof(*uuid)) == 0) {
			return tbl[i].type;
		}
	}
	return NULL;
}

static void
devpath_media_ramdisk(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{	/* See 10.3.5.9 */
	struct {			/* Sub-Type 9 */
		devpath_t	hdr;	/* Length 38 */
		uint64_t	StartingAddress;
		uint64_t	EndingAddress;
		uuid_t		GUID;
		uint16_t	Instance;
	} __packed *p = (void *)dp;
	__CTASSERT(sizeof(*p) == 38);
	char uuid_str[UUID_STR_LEN];
	const char *type;

	uuid_snprintf(uuid_str, sizeof(uuid_str), &p->GUID);

	type = devpath_media_ramdisk_type(&p->GUID);
	if (type != NULL)
		path->sz = easprintf(&path->cp, "%s(0x%016" PRIx64 ",0x%016"
		    PRIx64 ",%u)", type,
		    p->StartingAddress, p->EndingAddress, p->Instance);
	else
		path->sz = easprintf(&path->cp, "RamDisk(0x%016" PRIx64
		    ",0x%016" PRIx64 ",%u,%s)",
		    p->StartingAddress, p->EndingAddress, p->Instance,
		    uuid_str);

	if (dbg != NULL) {
		dbg->sz = easprintf(&dbg->cp,
		    DEVPATH_FMT_HDR
		    DEVPATH_FMT(StartingAddress:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(EndingAddress:) " 0x%016" PRIx64 "\n"
		    DEVPATH_FMT(Instance: 0x%08x\n)
		    DEVPATH_FMT(GUID: %s\n),
		    DEVPATH_DAT_HDR(dp),
		    p->StartingAddress,
		    p->EndingAddress,
		    p->Instance,
		    uuid_str);
	}
}

PUBLIC void
devpath_media(devpath_t *dp, devpath_elm_t *path, devpath_elm_t *dbg)
{

	switch (dp->SubType) {
	case 1:    devpath_media_harddisk(dp, path, dbg);	return;
	case 2:    devpath_media_cdrom(dp, path, dbg);		return;
	case 3:    devpath_media_vendor(dp, path, dbg);		return;
	case 4:    devpath_media_filepath(dp, path, dbg);	return;
	case 5:    devpath_media_protocol(dp, path, dbg);	return;
	case 6:    devpath_media_PIWGfwfile(dp, path, dbg);	return;
	case 7:    devpath_media_PIWGfwvol(dp, path, dbg);	return;
	case 8:    devpath_media_reloff(dp, path, dbg);		return;
	case 9:    devpath_media_ramdisk(dp, path, dbg);	return;
	default:   devpath_unsupported(dp, path, dbg);		return;
	}
}
