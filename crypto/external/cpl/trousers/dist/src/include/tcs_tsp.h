
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#ifndef _TCS_TSP_H_
#define _TCS_TSP_H_

/* Structures and defines needed to be known by the
 * TSP layer and the TCS layer.
 */

/*
 * disk store format:
 *
 * [type     name               ] cached?
 * --------------------------------------
 * [BYTE     TrouSerS PS version] no
 * [UINT32   num_keys_on_disk   ] no
 * [TSS_UUID uuid0              ] yes
 * [TSS_UUID uuid_parent0       ] yes
 * [UINT16   pub_data_size0     ] yes
 * [UINT16   blob_size0         ] yes
 * [UINT32   vendor_data_size0  ] yes
 * [UINT16   cache_flags0       ] yes
 * [BYTE[]   pub_data0          ] no
 * [BYTE[]   blob0              ] no
 * [BYTE[]   vendor_data0       ] no
 * [...]
 *
 */


/*
 * PS disk cache flags
 */
/* A key may be written to disk, in cache and yet be invalid if it has
 * since been unregistered. */
#define CACHE_FLAG_VALID                0x0001
/* set if the key's parent is stored in system PS */
#define CACHE_FLAG_PARENT_PS_SYSTEM     0x0002

/* the structure that makes up the in-memory PS disk cache */
struct key_disk_cache
{
        unsigned int offset;
        UINT16 pub_data_size;
        UINT16 blob_size;
        UINT16 flags;
	UINT32 vendor_data_size;
        TSS_UUID uuid;
        TSS_UUID parent_uuid;
        struct key_disk_cache *next;
};

/* The current PS version */
#define TSSPS_VERSION	1

/* offsets into each key on disk. These should be passed a (struct key_disk_cache *) */
#define TSSPS_VERSION_OFFSET		(0)
#define TSSPS_NUM_KEYS_OFFSET         (TSSPS_VERSION_OFFSET + sizeof(BYTE))
#define TSSPS_KEYS_OFFSET             (TSSPS_NUM_KEYS_OFFSET + sizeof(UINT32))
#define TSSPS_UUID_OFFSET(c)          ((c)->offset)
#define TSSPS_PARENT_UUID_OFFSET(c)   ((c)->offset + sizeof(TSS_UUID))
#define TSSPS_PUB_DATA_SIZE_OFFSET(c) ((c)->offset + (2 * sizeof(TSS_UUID)))
#define TSSPS_BLOB_SIZE_OFFSET(c)     ((c)->offset + (2 * sizeof(TSS_UUID)) + sizeof(UINT16))
#define TSSPS_VENDOR_SIZE_OFFSET(c)   ((c)->offset + (2 * sizeof(TSS_UUID)) + (2 * sizeof(UINT16)))
#define TSSPS_CACHE_FLAGS_OFFSET(c)   ((c)->offset + (2 * sizeof(TSS_UUID)) + (2 * sizeof(UINT16)) + sizeof(UINT32))
#define TSSPS_PUB_DATA_OFFSET(c)      ((c)->offset + (2 * sizeof(TSS_UUID)) + (3 * sizeof(UINT16)) + sizeof(UINT32))
#define TSSPS_BLOB_DATA_OFFSET(c)     ((c)->offset + (2 * sizeof(TSS_UUID)) + (3 * sizeof(UINT16)) + sizeof(UINT32) + (c)->pub_data_size)
#define TSSPS_VENDOR_DATA_OFFSET(c)   ((c)->offset + (2 * sizeof(TSS_UUID)) + (3 * sizeof(UINT16)) + sizeof(UINT32) + (c)->pub_data_size + (c)->blob_size)

/* XXX Get rid of this, there's no reason to set an arbitrary limit */
#define MAX_KEY_CHILDREN	10

#define STRUCTURE_PACKING_ATTRIBUTE	__attribute__((packed))

#ifdef TSS_DEBUG
#define DBG_ASSERT(x)	assert(x)
#else
#define DBG_ASSERT(x)
#endif

/* needed by execute transport in the TSP */
#define TSS_TPM_TXBLOB_HDR_LEN		(sizeof(UINT16) + (2 * sizeof(UINT32)))

#endif
