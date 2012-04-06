//
// hfs_misc.c - hfs routines
//
// Written by Eryk Vershen
//

/*
 * Copyright 2000 by Eryk Vershen
 */

// for *printf()
#include <stdio.h>

// for malloc(), calloc() & free()
#ifndef __linux__
#include <stdlib.h>
#else
#include <malloc.h>
#endif

// for strncpy() & strcmp()
#include <string.h>
// for O_RDONLY & O_RDWR
#include <fcntl.h>
// for errno
#include <errno.h>

#include <inttypes.h>

#include "hfs_misc.h"
#include "partition_map.h"
#include "convert.h"
#include "errors.h"


//
// Defines
//
#define MDB_OFFSET	2
#define HFS_SIG		0x4244	/* i.e 'BD' */
#define HFS_PLUS_SIG	0x482B	/* i.e 'H+' */

#define get_align_long(x)	(*(uint32_t*)(x))


//
// Types
//
typedef long long u64;

typedef struct ExtDescriptor {		// extent descriptor
    uint16_t	xdrStABN;	// first allocation block
    uint16_t	xdrNumABlks;	// number of allocation blocks
} ext_descriptor;

typedef struct ExtDataRec {
    ext_descriptor	ed[3];	// extent data record
} ext_data_rec;

/*
 * The crazy "uint16_t x[2]" stuff here is to get around the fact
 * that I can't convince the Mac compiler to align on 32 bit
 * quantities on 16 bit boundaries...
 */
struct mdb_record {		// master directory block
    uint16_t	drSigWord;	// volume signature
    uint16_t	drCrDate[2];	// date and time of volume creation
    uint16_t	drLsMod[2];	// date and time of last modification
    uint16_t	drAtrb;		// volume attributes
    uint16_t	drNmFls;	// number of files in root directory
    uint16_t	drVBMSt;	// first block of volume bitmap
    uint16_t	drAllocPtr;	// start of next allocation search
    uint16_t	drNmAlBlks;	// number of allocation blocks in volume
    uint32_t	drAlBlkSiz;	// size (in bytes) of allocation blocks
    uint32_t	drClpSiz;	// default clump size
    uint16_t	drAlBlSt;	// first allocation block in volume
    uint16_t	drNxtCNID[2];	// next unused catalog node ID
    uint16_t	drFreeBks;	// number of unused allocation blocks
    char	drVN[28];	// volume name
    uint16_t	drVolBkUp[2];	// date and time of last backup
    uint16_t	drVSeqNum;	// volume backup sequence number
    uint16_t	drWrCnt[2];	// volume write count
    uint16_t	drXTClpSiz[2];	// clump size for extents overflow file
    uint16_t	drCTClpSiz[2];	// clump size for catalog file
    uint16_t	drNmRtDirs;	// number of directories in root directory
    uint32_t	drFilCnt;	// number of files in volume
    uint32_t	drDirCnt;	// number of directories in volume
    uint32_t	drFndrInfo[8];	// information used by the Finder
#ifdef notdef
    uint16_t	drVCSize;	// size (in blocks) of volume cache
    uint16_t	drVBMCSize;	// size (in blocks) of volume bitmap cache
    uint16_t	drCtlCSize;	// size (in blocks) of common volume cache
#else
    uint16_t	drEmbedSigWord;	// type of embedded volume
    ext_descriptor	drEmbedExtent;	// embedded volume extent
#endif
    uint16_t	drXTFlSize[2];	// size of extents overflow file
    ext_data_rec	drXTExtRec;	// extent record for extents overflow file
    uint16_t	drCTFlSize[2];	// size of catalog file
    ext_data_rec	drCTExtRec;	// extent record for catalog file
};


typedef uint32_t HFSCatalogNodeID;

typedef struct HFSPlusExtentDescriptor {
    uint32_t startBlock;
    uint32_t blockCount;
} HFSPlusExtentDescriptor;

typedef HFSPlusExtentDescriptor HFSPlusExtentRecord[ 8];

typedef struct HFSPlusForkData {
    u64 logicalSize;
    uint32_t clumpSize;
    uint32_t totalBlocks;
    HFSPlusExtentRecord extents;
} HFSPlusForkData;

struct HFSPlusVolumeHeader {
    uint16_t signature;
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t reserved;
    uint32_t createDate;
    uint32_t modifyDate;
    uint32_t backupDate;
    uint32_t checkedDate;
    uint32_t fileCount;
    uint32_t folderCount;
    uint32_t blockSize;
    uint32_t totalBlocks;
    uint32_t freeBlocks;
    uint32_t nextAllocation;
    uint32_t rsrcClumpSize;
    uint32_t dataClumpSize;
    HFSCatalogNodeID nextCatalogID;
    uint32_t writeCount;
    u64 encodingsBitmap;
    uint8_t finderInfo[ 32];
    HFSPlusForkData allocationFile;
    HFSPlusForkData extentsFile;
    HFSPlusForkData catalogFile;
    HFSPlusForkData attributesFile;
    HFSPlusForkData startupFile;
} HFSPlusVolumeHeader;


//
// Global Constants
//


//
// Global Variables
//


//
// Forward declarations
//
uint32_t embeded_offset(struct mdb_record *mdb, uint32_t sector);
int read_partition_block(partition_map *entry, uint32_t num, char *buf);


//
// Routines
//
uint32_t
embeded_offset(struct mdb_record *mdb, uint32_t sector)
{
    uint32_t e_offset;
    
    e_offset = mdb->drAlBlSt + mdb->drEmbedExtent.xdrStABN * (mdb->drAlBlkSiz / 512);
    
    return e_offset + sector;
}


char *
get_HFS_name(partition_map *entry, int *kind)
{
    DPME *data;
    struct mdb_record *mdb;
    //struct HFSPlusVolumeHeader *mdb2;
    char *name = NULL;
    int len;
    
    *kind = kHFS_not;

    mdb = (struct mdb_record *) malloc(PBLOCK_SIZE);
    if (mdb == NULL) {
	error(errno, "can't allocate memory for MDB");
	return NULL;
    }

    data = entry->data;
    if (strcmp(data->dpme_type, kHFSType) == 0) {
	if (read_partition_block(entry, 2, (char *)mdb) == 0) {
	    error(-1, "Can't read block %d from partition %d", 2, entry->disk_address);
	    goto not_hfs;
	}
	if (mdb->drSigWord == HFS_PLUS_SIG) {
	    // pure HFS Plus
	    // printf("%lu HFS Plus\n", entry->disk_address);
	    *kind = kHFS_plus;
	} else if (mdb->drSigWord != HFS_SIG) {
	    // not HFS !!!
	    // printf("%"PRIu32" not HFS\n", entry->disk_address);
	    *kind = kHFS_not;
	} else if (mdb->drEmbedSigWord != HFS_PLUS_SIG) {
	    // HFS
	    // printf("%lu HFS\n", entry->disk_address);
	    *kind = kHFS_std;
	    len = mdb->drVN[0];
	    name = (char *) malloc(len+1);
	    strncpy(name, &mdb->drVN[1], len);
	    name[len] = 0;
	} else {
	    // embedded HFS plus
	    // printf("%lu embedded HFS Plus\n", entry->disk_address);
	    *kind = kHFS_embed;
	    len = mdb->drVN[0];
	    name = (char *) malloc(len+1);
	    strncpy(name, &mdb->drVN[1], len);
	    name[len] = 0;
	}
    }
not_hfs:
    free(mdb);
    return name;
}

// really need a function to read block n from partition m

int
read_partition_block(partition_map *entry, uint32_t num, char *buf)
{
    DPME *data;
    partition_map_header * map;
    uint32_t base;
    u64 offset;
    
    map = entry->the_map;
    data = entry->data;
    base = data->dpme_pblock_start;

    if (num >= data->dpme_pblocks) {
	return 0;
    }
    offset = ((long long) base) * map->logical_block + num * 512;
    
    return read_media(map->m, offset, 512, (void *)buf);
}
