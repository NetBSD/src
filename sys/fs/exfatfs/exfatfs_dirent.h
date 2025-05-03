/* $NetBSD: exfatfs_dirent.h,v 1.1.2.4 2025/05/03 04:31:56 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024, 2025 The NetBSD Foundation, Inc.
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

#ifndef EXFATFS_DIRENT_H_
#define EXFATFS_DIRENT_H_

/*
 * A structure to represent a directory entry.
 * Taken from section 6.2 of the standard.
 */
struct exfatfs_dirent {
	uint8_t  xd_entryType;          /* Byte 0 */
#define XD_ENTRYTYPE_TYPECODE_MASK       0x1f /* All values valid */
#define XD_ENTRYTYPE_TYPEIMPORTANCE_MASK 0x20 /* 0 = critical, 1 = benign */
#define XD_ENTRYTYPE_TYPECATEGORY_MASK   0x40 /* 0 = primary, 1 = secondary */
#define ISPRIMARY(dp) (((dp)->xd_entryType & XD_ENTRYTYPE_TYPECATEGORY_MASK) \
			== 0)
#define XD_ENTRYTYPE_INUSE_MASK          0x80 /* 0 = not in use, 1 = in use */
#define ISINUSE(dp) (((dp)->xd_entryType & XD_ENTRYTYPE_INUSE_MASK) > 0)

#define XD_ENTRYTYPE_EOD               0x00
#define ISEOD(dp) ((dp)->xd_entryType == XD_ENTRYTYPE_EOD)
/* Primary Critical */
#define XD_ENTRYTYPE_ALLOC_BITMAP      0x81
#define XD_ENTRYTYPE_UPCASE_TABLE      0x82
#define XD_ENTRYTYPE_VOLUME_LABEL      0x83
#define XD_ENTRYTYPE_FILE              0x85
/* Primary Benign */
#define XD_ENTRYTYPE_VOLUME_GUID       0xA0
#define XD_ENTRYTYPE_TEXFAT_PADDING    0xA1
/* Secondary Critical */
#define XD_ENTRYTYPE_STREAM_EXTENSION  0xC0
#define XD_ENTRYTYPE_FILE_NAME         0xC1
/* Secondary Benign */
#define XD_ENTRYTYPE_VENDOR_EXTENSION  0xE0
#define XD_ENTRYTYPE_VENDOR_ALLOCATION 0xE1
	uint8_t  xd_customDefined[19];  /* Bytes 1..19 */
	uint32_t xd_firstCluster;	/* Bytes 20..23 */
#define GET_DE_FIRST_CLUSTER(x) le32toh((x)->xd_firstCluster)
#define SET_DE_FIRST_CLUSTER(x, v) do { (x)->xd_firstCluster = htole32(v); } while (0)
	uint64_t xd_dataLength;		/* Bytes 24..32 */
#define GET_DE_DATA_LENGTH(x) le64toh((x)->xd_dataLength)
#define SET_DE_DATA_LENGTH(x, v) do { (x)->xd_dataLength = htole64(v); } while (0)
};

/* Not in the spec: FILE but not in use */
#define XD_ENTRYTYPE_FILLER	       0x05

struct exfatfs_dirent_plus {
	struct exfatfs_dirent de;
	unsigned long serial;
};

struct exfatfs_dirent_primary {
	uint8_t  xd_entryType;          /* Byte 0 */
	/* # of secondary entries after this one */
	uint8_t  xd_secondaryCount;     /* Byte 1 */
	/* Checksum of all dirents, excluding this field */
	uint16_t xd_setChecksum;        /* Bytes 2-3 */
#define GET_DE_SET_CHECKSUM(x) le16toh((x)->xd_setChecksum)
#define SET_DE_SET_CHECKSUM(x) do { (x)->xd_setChecksum = htole16(v); } while (0)
	uint16_t xd_generalPrimaryFlags; /* Bytes 4-5 */
#define GET_DE_GENERAL_PRIMARY_FLAGS(x) le16toh((x)->xd_generalPrimaryFlags)
#define SET_DE_GENERAL_PRIMARY_FLAGS(x) do { (x)->xd_generalPrimaryFlags = htole16(v); } while (0)
#define XD_GENERALPRIMARYFLAGS_ALLOCATIONPOSSIBLE_MASK 0x0001
#define XD_GENERALPRIMARYFLAGS_NOFATCHAIN              0x0002
	uint8_t  xd_customDefined[14];  /* Bytes 6..19 */
	uint32_t xd_firstCluster;	/* Bytes 20..23 */
	uint64_t xd_dataLength;		/* Bytes 24..32 */
};

struct exfatfs_dirent_secondary {
	uint8_t  xd_entryType;          /* Byte 0 */
	uint8_t xd_generalSecondaryFlags; /* Byte 1 */
#define XD_GENERALSECONDARYFLAGS_ALLOCATIONPOSSIBLE_MASK 0x0001
#define XD_GENERALSECONDARYFLAGS_NOFATCHAIN              0x0002
	uint8_t  xd_customDefined[18];  /* Bytes 2..19 */
	uint32_t xd_firstCluster;	/* Bytes 20..23 */
	uint64_t xd_dataLength;		/* Bytes 24..32 */
};

/* 
 * Section 7: Directory Entry Definitions
 */
struct exfatfs_dirent_allocation_bitmap {
        uint8_t  xd_entryType;          /* Byte 0 */
	uint8_t  xd_bitmapFlags;        /* Byte 1 */
        uint8_t  xd_customDefined[18];  /* Bytes 2..19 */
        uint32_t xd_firstCluster;       /* Bytes 20..23 */
        uint64_t xd_dataLength;         /* Bytes 24..32 */
};

/*
 * Macros to convert cluster number to sector offset within the cluster,
 * and to test within a sector whether a given bitmap entry
 * is set.
 */
/* XXX */

/*
 * Up-case directory entry
 */

struct exfatfs_dirent_upcase_table {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_reserved1[3];       /* Bytes 1..3 */
        uint32_t xd_tableChecksum;      /* Bytes 4..7 */
#define GET_DUE_TABLE_CHECKSUM(x) le32toh((x)->xd_tableChecksum)
#define SET_DUE_TABLE_CHECKSUM(x, v) do { (x)->xd_tableChecksum = htole32(x); } while (0)
        uint8_t  xd_reserved2[12];      /* Bytes 8..19 */
        uint32_t xd_firstCluster;       /* Bytes 20..23 */
        uint64_t xd_dataLength;         /* Bytes 24..32 */
};

struct exfatfs_dirent_volume_label {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_characterCount;     /* Byte 2 */
	uint8_t  xd_volumeLabel[22];    /* Bytes 3..23 */
	uint8_t  xd_reserved[8];        /* Bytes 24..32 */
};


struct exfatfs_dfe {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_secondaryCount;     /* Byte 2 */
	uint16_t xd_setChecksum;
	uint16_t xd_fileAttributes;
#define GET_DE_FILE_ATTRIBUTES(x) le16toh((x)->xd_fileAttributes)
#define SET_DE_FILE_ATTRIBUTES(x, v) do { (x)->xd_fileAttributes = htole16(v); } while (0)
#define XD_FILEATTR_READONLY  0x0001
#define XD_FILEATTR_HIDDEN    0x0002
#define XD_FILEATTR_SYSTEM    0x0004
#define XD_FILEATTR_RESERVED1 0x0008
#define XD_FILEATTR_DIRECTORY 0x0010
#define XD_FILEATTR_ARCHIVE   0x0020
/* Non-standard, from dorimanx/exfat-nofuse */
#define XD_FILEATTR_SYMLINK   0x0040
	uint8_t xd_reserved1[2];
	uint32_t xd_createTimestamp;
#define GET_DE_CREATE_TIMESTAMP(x) le32toh((x)->xd_createTimestamp)
#define SET_DE_CREATE_TIMESTAMP(x, v) do { (x)->xd_createTimestamp = htole32(v); } while (0)
	uint32_t xd_lastModifiedTimestamp;
#define GET_DE_LAST_MODIFIED_TIMESTAMP(x) le32toh((x)->xd_lastModifiedTimestamp)
#define SET_DE_LAST_MODIFIED_TIMESTAMP(x, v) do { (x)->xd_lastModifiedTimestamp = htole32(v); } while (0)
	uint32_t xd_lastAccessedTimestamp;
#define GET_DE_LAST_ACCESSED_TIMESTAMP(x) le32toh((x)->xd_lastAccessedTimestamp)
#define SET_DE_LAST_ACCESSED_TIMESTAMP(x, v) do { (x)->xd_lastAccessedTimestamp = htole32(v); } while (0)
	uint8_t xd_create10msIncrement;
	uint8_t xd_lastModified10msIncrement;
	uint8_t xd_createUtcOffset; /* 15-minute increments, signed int7_t */
	/* high bit indicates whether offset is valid or not */
	uint8_t xd_lastModifiedUtcOffset;
	uint8_t xd_lastAccessedUtcOffset;
	uint8_t  xd_reserved[7];
};

/* 0 = 0 seconds, 29 = 58 seconds */
/* The 10ms fields have valid values between 0 and 199 (0..1.99 seconds) */
#define XD_TIMESTAMP_MASK_DOUBLESECONDS 0x0000001f
#define XD_TIMESTAMP_SHIFT_DOUBLESECONDS 0
/* These are interpreted exactly as they are in US date/time notation */
#define XD_TIMESTAMP_MASK_MINUTE        0x000007e0
#define XD_TIMESTAMP_SHIFT_MINUTE 5
#define XD_TIMESTAMP_MASK_HOUR          0x0000f100
#define XD_TIMESTAMP_SHIFT_HOUR 11
#define XD_TIMESTAMP_MASK_DAY           0x001f0000
#define XD_TIMESTAMP_SHIFT_DAY 16
#define XD_TIMESTAMP_MASK_MONTH         0x01e00000
#define XD_TIMESTAMP_SHIFT_MONTH 21
/* 0 = 1980, 127 = 2107.  Years earlier than 1980 are not representable. */
#define XD_TIMESTAMP_MASK_YEAR          0xfe000000
#define XD_TIMESTAMP_SHIFT_YEAR 25

struct exfatfs_dirent_volume_guid {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_secondaryCount;
        uint16_t xd_setChecksum;
        uint16_t xd_generalPrimaryFlags;
	uint8_t  xd_volumeGuid[16];
	uint8_t  xd_reserved[10];
};

struct exfatfs_dse {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_generalSecondaryFlags; /* 1 */
#define XD_FLAG_ALLOCPOSSIBLE 0x01
#define XD_FLAG_NOFATCHAIN    0x02
	uint8_t  xd_reserved1;             /* 2 */
	uint8_t  xd_nameLength;            /* 3 */
	uint16_t xd_nameHash;              /* 4..5 */
#define GET_DE_NAME_HASH(x) le16toh((x)->xd_nameHash)
#define SET_DE_NAME_HASH(x, v) do { (x)->xd_nameHash = htole16(v); } while (0)
	uint8_t  xd_reserved2[2];          /* 6..7 */
	uint64_t xd_validDataLength;       /* 8..15 */
#define GET_DE_VALID_DATA_LENGTH(x) le64toh((x)->xd_validDataLength)
#define SET_DE_VALID_DATA_LENGTH(x, v) do { (x)->xd_validDataLength = htole64(v); } while (0)
	uint8_t  xd_reserved3[4];          /* 16..19 */
	uint32_t xd_firstCluster;          /* 20..23 */
	uint64_t xd_dataLength;            /* 24..31 */
};

struct exfatfs_dfn {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_generalSecondaryFlags;
#define EXFATFS_NAME_CHUNKSIZE 15
	uint16_t xd_fileName[EXFATFS_NAME_CHUNKSIZE];
};
#define EXFATFS_MAX_NAMELEN 255

struct exfatfs_dirent_vendor_extension {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_generalSecondaryFlags;
	uint8_t  xd_vendorGuid[16];
	uint8_t  xd_vendorDefined[14];
};

struct exfatfs_dirent_vendor_allocation {
        uint8_t  xd_entryType;          /* Byte 0 */
        uint8_t  xd_generalSecondaryFlags;
	uint8_t  xd_vendorGuid[16];
	uint16_t xd_vendorDefined;
#define GET_DVAE_VENDOR_DEFINED(x) le16toh((x)->xd_vendorDefined)
#define SET_DVAE_VENDOR_DEFINED(x, v) do { (x)->xd_vendorDefined = htole16(x); } while (0)
	uint32_t xd_firstCluster;
	uint64_t xd_dataLength;
};

/* Undefined is the TexFAT directory entry, type code 1 and importance 1 */

#endif /* EXFATFS_DIRENT_H_ */
