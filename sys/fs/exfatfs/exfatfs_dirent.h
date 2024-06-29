/* $NetBSD: exfatfs_dirent.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
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
#define ISPRIMARY(dp) (((dp)->xd_entryType & XD_ENTRYTYPE_TYPECATEGORY_MASK) == 0)
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
	uint64_t xd_dataLength;		/* Bytes 24..32 */
};

struct exfatfs_dirent_plus {
	struct exfatfs_dirent de;
	unsigned long serial;
};

struct exfatfs_dirent_primary {
	uint8_t  xd_entryType;          /* Byte 0 */
	uint8_t  xd_secondaryCount;     /* Byte 1 */ /* # of secondary entries after this one */
	uint16_t xd_setChecksum;        /* Bytes 2-3 */ /* Checksum of all dirents, excluding this field */
	uint16_t xd_generalPrimaryFlags; /* Bytes 4-5 */
#define XD_GENERALPRIMARYFLAGS_ALLOCATIONPOSSIBLE_MASK 0x0001
#define XD_GENERALPRIMARYFLAGS_NOFATCHAIN              0x0002
	uint8_t  xd_customDefined[14];  /* Bytes 6..19 */
	uint32_t xd_firstCluster;	/* Bytes 20..23 */
	uint64_t xd_dataLength;		/* Bytes 24..32 */
};

struct exfatfs_dirent_secondary {
	uint8_t  xd_entryType;          /* Byte 0 */
	uint16_t xd_generalSecondaryFlags; /* Byte 2 */
#define XD_GENERALSECONDARYFLAGS_ALLOCATIONPOSSIBLE_MASK 0x0001
#define XD_GENERALSECONDARYFLAGS_NOFATCHAIN              0x0002
	uint8_t  xd_customDefined[18];  /* Bytes 3..19 */
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
#define XD_FILEATTR_READONLY  0x0001
#define XD_FILEATTR_HIDDEN    0x0002
#define XD_FILEATTR_SYSTEM    0x0004
#define XD_FILEATTR_RESERVED1 0x0008
#define XD_FILEATTR_DIRECTORY 0x0010
#define XD_FILEATTR_ARCHIVE   0x0020
#define XD_FILEATTR_SYMLINK   0x0040 /* Non-standard, from dorimanx/exfat-nofuse */
	uint8_t xd_reserved1[2];
	uint32_t xd_createTimestamp;
	uint32_t xd_lastModifiedTimestamp;
	uint32_t xd_lastAccessedTimestamp;
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
	uint8_t  xd_reserved2[2];          /* 6..7 */
	uint64_t xd_validDataLength;       /* 8..15 */
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
	uint32_t xd_firstCluster;
	uint64_t xd_dataLength;
};

/* Undefined is the TexFAT directory entry, type code 1 and importance 1 */

#endif /* EXFATFS_DIRENT_H_ */
