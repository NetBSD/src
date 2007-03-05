/*	$NetBSD: libhfsp.h,v 1.1.1.1 2007/03/05 23:01:08 dillo Exp $	*/

/*-
 * Copyright (c) 2005, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yevgeny Binder and Dieter Baron.
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

#ifndef _FS_HFSP_LIBHFSP_H_
#define _FS_HFSP_LIBHFSP_H_

#include <sys/endian.h>
#include <sys/param.h>
#include <sys/mount.h>	/* needs to go after sys/param.h or compile fails */
#include <sys/types.h>
#if defined(_KERNEL)
#include <sys/kernel.h>
#include <sys/systm.h>
#include <machine/stdarg.h>
#include <sys/fcntl.h>
#endif /* defined(_KERNEL) */

#if !defined(_KERNEL) && !defined(STANDALONE)
#include <fcntl.h>
#include <iconv.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif /* !defined(_KERNEL) && !defined(STANDALONE) */

#ifndef va_list
#define va_list _BSD_VA_LIST_
#endif /* !va_list */

#define max(A,B) ((A) > (B) ? (A):(B))
#define min(A,B) ((A) < (B) ? (A):(B))


/* Macros to handle errors in this library. Not recommended outside libhfsp.c */
#define HFSP_LIBERR(format, ...) \
	do{ hfsplib_error(format, __FILE__, __LINE__ , ##__VA_ARGS__); \
		goto error; } while(/*CONSTCOND*/ 0)

#if 0
#pragma mark Constants (on-disk)
#endif


enum
{
	HFSP_SIG_HFSP	= 0x482B,	/* 'H+' */
	HFSP_SIG_HFSX	= 0x4858,	/* 'HX' */
	HFSP_SIG_HFS	= 0x4244	/* 'BD' */
}; /* volume signatures */

typedef enum
{
							/* bits 0-6 are reserved */
	HFSP_VOL_HWLOCK			= 7,
	HFSP_VOL_UNMOUNTED		= 8,
	HFSP_VOL_BADBLOCKS		= 9,
	HFSP_VOL_NOCACHE		= 10,
	HFSP_VOL_DIRTY			= 11,
	HFSP_VOL_CNIDS_RECYCLED	= 12,
	HFSP_VOL_JOURNALED		= 13,
							/* bit 14 is reserved */
	HFSP_VOL_SWLOCK			= 15
							/* bits 16-31 are reserved */
} hfsp_volume_attribute_bit; /* volume header attribute bits */

typedef enum
{
	HFSP_LEAFNODE	= -1,
	HFSP_INDEXNODE	= 0,
	HFSP_HEADERNODE	= 1,
	HFSP_MAPNODE	= 2
} hfsp_node_kind; /* btree node kinds */

enum
{
	HFSP_BAD_CLOSE_MASK			= 0x00000001,
	HFSP_BIG_KEYS_MASK			= 0x00000002,
	HFSP_VAR_INDEX_KEYS_MASK	= 0x00000004
}; /* btree header attribute masks */

typedef enum
{
	HFSP_CNID_ROOT_PARENT	= 1,
	HFSP_CNID_ROOT_FOLDER	= 2,
	HFSP_CNID_EXTENTS		= 3,
	HFSP_CNID_CATALOG		= 4,
	HFSP_CNID_BADBLOCKS		= 5,
	HFSP_CNID_ALLOCATION	= 6,
	HFSP_CNID_STARTUP		= 7,
	HFSP_CNID_ATTRIBUTES	= 8,
								/* CNIDs 9-13 are reserved */
	HFSP_CNID_REPAIR		= 14,
	HFSP_CNID_TEMP			= 15,
	HFSP_CNID_USER			= 16
} hfsp_special_cnid; /* special CNID values */

typedef enum
{
	HFSP_REC_FLDR			= 0x0001,
	HFSP_REC_FILE			= 0x0002,
	HFSP_REC_FLDR_THREAD	= 0x0003,
	HFSP_REC_FILE_THREAD	= 0x0004
} hfsp_catalog_rec_kind; /* catalog record types */

enum
{
    HFSP_JOURNAL_ON_DISK_MASK		= 0x00000001, /* journal on same volume */
    HFSP_JOURNAL_ON_OTHER_MASK		= 0x00000002, /* journal elsewhere */
    HFSP_JOURNAL_NEEDS_INIT_MASK	= 0x00000004
}; /* journal flag masks */

enum
{
	HFSP_JOURNAL_HEADER_MAGIC	= 0x4a4e4c78,
	HFSP_JOURNAL_ENDIAN_MAGIC	= 0x12345678
}; /* journal magic numbers */

enum
{
	HFSP_DATAFORK	= 0x00,
	HFSP_RSRCFORK	= 0xFF
}; /* common fork types */

enum
{
	HFSP_KEY_CASEFOLD	= 0xCF,
	HFSP_KEY_BINARY		= 0XBC
}; /* catalog key comparison method types */

enum
{
	HFSP_MIN_CAT_KEY_LEN	= 6,
	HFSP_MAX_CAT_KEY_LEN	= 516,
	HFSP_MAX_EXT_KEY_LEN	= 10
};

enum {
    HFSP_HARD_LINK_FILE_TYPE = 0x686C6E6B,  /* 'hlnk' */
    HFSP_HFSPLUS_CREATOR     = 0x6866732B   /* 'hfs+' */
};


#if 0
#pragma mark -
#pragma mark Constants (custom)
#endif


/* number of bytes between start of volume and volume header */
#define HFSP_VOLUME_HEAD_RESERVE_SIZE	1024

typedef enum
{
	HFSP_CATALOG_FILE = 1,
	HFSP_EXTENTS_FILE = 2,
	HFSP_ATTRIBUTES_FILE = 3
} hfsp_btree_file_type; /* btree file kinds */


#if 0
#pragma mark -
#pragma mark On-Disk Types (Mac OS specific)
#endif

typedef uint32_t	hfsp_macos_type_code; /* four 1-byte char field */

typedef struct
{
  int16_t	v;
  int16_t	h;
} hfsp_macos_point_t;

typedef struct
{
  int16_t	t;	/* top */
  int16_t	l;	/* left */
  int16_t	b;	/* bottom */
  int16_t	r;	/* right */
} hfsp_macos_rect_t;

typedef struct
{
  hfsp_macos_type_code	file_type;
  hfsp_macos_type_code	file_creator;
  uint16_t				finder_flags;
  hfsp_macos_point_t	location;
  uint16_t				reserved;
} hfsp_macos_file_info_t;

typedef struct
{
  int16_t	reserved[4];
  uint16_t	extended_finder_flags;
  int16_t	reserved2;
  int32_t	put_away_folder_cnid;
} hfsp_macos_extended_file_info_t;

typedef struct
{
  hfsp_macos_rect_t		window_bounds;
  uint16_t				finder_flags;
  hfsp_macos_point_t	location;
  uint16_t				reserved;
} hfsp_macos_folder_info_t;

typedef struct
{
  hfsp_macos_point_t	scroll_position;
  int32_t				reserved;
  uint16_t				extended_finder_flags;
  int16_t				reserved2;
  int32_t				put_away_folder_cnid;
} hfsp_macos_extended_folder_info_t;


#if 0
#pragma mark -
#pragma mark On-Disk Types
#endif

typedef uint16_t unichar_t;

typedef uint32_t hfsp_cnid_t;


typedef struct
{
	uint16_t	length;
	unichar_t	unicode[255];
} hfsp_unistr255_t;

typedef struct
{
	uint32_t	start_block;
	uint32_t	block_count;
} hfsp_extent_descriptor_t;

typedef hfsp_extent_descriptor_t hfsp_extent_record_t[8];

typedef struct hfsp_fork_t
{
	uint64_t				logical_size;
	uint32_t				clump_size;
	uint32_t				total_blocks;
	hfsp_extent_record_t	extents;
} hfsp_fork_t;
 
typedef struct
{
	uint16_t	signature;
	uint16_t	version;
	uint32_t	attributes;
	uint32_t	last_mounting_version;
	uint32_t	journal_info_block;
 
	uint32_t	date_created;
	uint32_t	date_modified;
	uint32_t	date_backedup;
	uint32_t	date_checked;
 
	uint32_t	file_count;
	uint32_t	folder_count;
 
	uint32_t	block_size;
	uint32_t	total_blocks;
	uint32_t	free_blocks;
 
	uint32_t	next_alloc_block;
	uint32_t	rsrc_clump_size;
	uint32_t	data_clump_size;
	hfsp_cnid_t	next_cnid;
 
	uint32_t	write_count;
	uint64_t	encodings;
 
	uint32_t	finder_info[8];
 
	hfsp_fork_t	allocation_file;
	hfsp_fork_t	extents_file;
	hfsp_fork_t	catalog_file;
	hfsp_fork_t	attributes_file;
	hfsp_fork_t	startup_file;
} hfsp_volume_header_t;

typedef struct
{
	uint32_t	flink;
	uint32_t	blink;
	int8_t		kind;
	uint8_t		height;
	uint16_t	num_recs;
	uint16_t	reserved;
} hfsp_node_descriptor_t;

typedef struct
{
	uint16_t	tree_depth;
	uint32_t	root_node;
	uint32_t	leaf_recs;
	uint32_t	first_leaf;
	uint32_t	last_leaf;
	uint16_t	node_size;
	uint16_t	max_key_len;
	uint32_t	total_nodes;
	uint32_t	free_nodes;
	uint16_t	reserved;
	uint32_t	clump_size;		/* misaligned */
	uint8_t		btree_type;
	uint8_t		keycomp_type;
	uint32_t	attributes;		/* long aligned again */
	uint32_t	reserved2[16];
} hfsp_header_record_t;

typedef struct
{
	uint16_t			key_len;
	hfsp_cnid_t			parent_cnid;
	hfsp_unistr255_t	name;
} hfsp_catalog_key_t;

typedef struct
{
	uint16_t	key_length;
	uint8_t		fork_type;
	uint8_t		padding;
	hfsp_cnid_t	file_cnid;
	uint32_t	start_block;
} hfsp_extent_key_t;

typedef struct
{
	uint32_t	owner_id;
	uint32_t	group_id;
	uint8_t		admin_flags;
	uint8_t		owner_flags;
	uint16_t	file_mode;
	union
	{
		uint32_t	inode_num;
		uint32_t	link_count;
		uint32_t	raw_device;
	} special;
} hfsp_bsd_data_t;

typedef struct
{
	int16_t			rec_type;
	uint16_t		flags;
	uint32_t		valence;
	hfsp_cnid_t		cnid;
	uint32_t		date_created;
	uint32_t		date_content_mod;
	uint32_t		date_attrib_mod;
	uint32_t		date_accessed;
	uint32_t		date_backedup;
	hfsp_bsd_data_t						bsd;
	hfsp_macos_folder_info_t			user_info;
	hfsp_macos_extended_folder_info_t	finder_info;
	uint32_t		text_encoding;
	uint32_t		reserved;
} hfsp_folder_record_t;

typedef struct
{
	int16_t			rec_type;
	uint16_t		flags;
	uint32_t		reserved;
	hfsp_cnid_t		cnid;
	uint32_t		date_created;
	uint32_t		date_content_mod;
	uint32_t		date_attrib_mod;
	uint32_t		date_accessed;
	uint32_t		date_backedup;
	hfsp_bsd_data_t						bsd;
	hfsp_macos_file_info_t				user_info;
	hfsp_macos_extended_file_info_t		finder_info;
	uint32_t		text_encoding;
	uint32_t		reserved2;
	hfsp_fork_t		data_fork;
	hfsp_fork_t		rsrc_fork;
} hfsp_file_record_t;

typedef struct
{
	int16_t				rec_type;
	int16_t				reserved;
	hfsp_cnid_t			parent_cnid;
	hfsp_unistr255_t	name;
} hfsp_thread_record_t;

typedef struct
{
	uint32_t	flags;
	uint32_t	device_signature[8];
	uint64_t	offset;
	uint64_t	size;
	uint64_t	reserved[32];
} hfsp_journal_info_t;

typedef struct
{
	uint32_t	magic;
	uint32_t	endian;
	uint64_t	start;
	uint64_t	end;
	uint64_t	size;
	uint32_t	blocklist_header_size;
	uint32_t	checksum;
	uint32_t	journal_header_size;
} hfsp_journal_header_t;

#if 0
#pragma mark -
#pragma mark Custom Types
#endif

typedef struct
{
	hfsp_volume_header_t	vh;		/* volume header */
	hfsp_header_record_t	chr;	/* catalog file header node record*/
	hfsp_header_record_t	ehr;	/* extent overflow file header node record*/
	uint8_t	catkeysizefieldsize;	/* size of catalog file key_len field in
									 * bytes (1 or 2); always 2 for HFS+ */
	uint8_t	extkeysizefieldsize;	/* size of extent file key_len field in
									 * bytes (1 or 2); always 2 for HFS+ */
	hfsp_unistr255_t		name;	/* volume name */

	/* pointer to catalog file key comparison function */
	int (*keycmp) (const void*, const void*);

	int						journaled;	/* 1 if volume is journaled, else 0 */
	hfsp_journal_info_t		jib;	/* journal info block */
	hfsp_journal_header_t	jh;		/* journal header */

	int		readonly;	/* 0 if mounted r/w, 1 if mounted r/o */
	void*	cbdata;		/* application-specific data; allocated, defined and
						 * used (if desired) by the program, usually within
						 * callback routines */
} hfsp_volume;

typedef union
{
	/* for leaf nodes */
	int16_t					type; /* type of record: folder, file, or thread */
	hfsp_folder_record_t	folder;
	hfsp_file_record_t		file;
	hfsp_thread_record_t	thread;
	
	/* for pointer nodes */
	/* (using this large union for just one tiny field is not memory-efficient,
	 *	 so change this if it becomes problematic) */ 
	uint32_t	child;	/* node number of this node's child node */
} hfsp_catalog_keyed_record_t;

/*
 * These arguments are passed among libhfsp without any inspection. This struct
 * is accepted by all public functions of libhfsp, and passed to each callback.
 * An application dereferences each pointer to its own specific struct of
 * arguments. Callbacks must be prepared to deal with NULL values for any of
 * these fields (by providing default values to be used in lieu of that
 * argument). However, a NULL pointer to this struct is an error.
 *
 * It was decided to make one unified argument structure, rather than many
 * separate, operand-specific structures, because, when this structure is passed
 * to a public function (e.g., hfsplib_open_volume()), the function may make
 * several calls (and subcalls) to various facilities, e.g., read(), malloc(),
 * and free(), all of which require their own particular arguments. The
 * facilities to be used are quite impractical to foreshadow, so the application
 * takes care of all possible calls at once. This also reinforces the idea that
 * a public call is an umbrella to a set of system calls, and all of these calls
 * must be passed arguments which do not change within the context of this
 * umbrella. (E.g., if a public function makes two calls to read(), one call
 * should not be passed a uid of root and the other passed a uid of daemon.)
 */
typedef struct
{
	/* The 'error' function does not take an argument. All others do. */
	
	void*	allocmem;
	void*	reallocmem;
	void*	freemem;
	void*	openvol;
	void*	closevol;
	void*	read;
} hfsp_callback_args;

typedef struct
{
	/* error(in_format, in_file, in_line, in_args) */
	void (*error) (const char*, const char*, int, va_list);
	
	/* allocmem(in_size, cbargs) */
	void* (*allocmem) (size_t, hfsp_callback_args*);
	
	/* reallocmem(in_ptr, in_size, cbargs) */
	void* (*reallocmem) (void*, size_t, hfsp_callback_args*);
	
	/* freemem(in_ptr, cbargs) */
	void (*freemem) (void*, hfsp_callback_args*);
	
	/* openvol(in_volume, in_devicepath, in_offset, cbargs)
	 * returns 0 on success */
	int (*openvol) (hfsp_volume*, const char*, uint64_t, hfsp_callback_args*);
	
	/* closevol(in_volume, cbargs) */
	void (*closevol) (hfsp_volume*, hfsp_callback_args*);
	
	/* read(in_volume, out_buffer, in_length, in_offset, cbargs)
	 * returns 0 on success */
	int (*read) (hfsp_volume*, void*, uint64_t, uint64_t,
		hfsp_callback_args*);
		
} hfsp_callbacks;

hfsp_callbacks	hfsp_gcb;	/* global callbacks */

/*
 * global case folding table
 * (lazily initialized; see comments at bottom of hfsp_open_volume())
 */
unichar_t* hfsp_gcft;

#if 0
#pragma mark -
#pragma mark Functions
#endif

void hfsplib_init(hfsp_callbacks*);
void hfsplib_done(void);
void hfsplib_init_cbargs(hfsp_callback_args*);

int hfsplib_open_volume(const char*, uint64_t, int, hfsp_volume*,
	hfsp_callback_args*);
void hfsplib_close_volume(hfsp_volume*, hfsp_callback_args*);

int hfsplib_path_to_cnid(hfsp_volume*, hfsp_cnid_t, char**, uint16_t*,
	hfsp_callback_args*);
hfsp_cnid_t hfsplib_find_parent_thread(hfsp_volume*, hfsp_cnid_t,
	hfsp_thread_record_t*, hfsp_callback_args*);
int hfsplib_find_catalog_record_with_cnid(hfsp_volume*, hfsp_cnid_t,
	hfsp_catalog_keyed_record_t*, hfsp_catalog_key_t*, hfsp_callback_args*);
int hfsplib_find_catalog_record_with_key(hfsp_volume*, hfsp_catalog_key_t*,
	hfsp_catalog_keyed_record_t*, hfsp_callback_args*);
int hfsplib_find_extent_record_with_key(hfsp_volume*, hfsp_extent_key_t*,
	hfsp_extent_record_t*, hfsp_callback_args*);
int hfsplib_get_directory_contents(hfsp_volume*, hfsp_cnid_t,
	hfsp_catalog_keyed_record_t**, hfsp_unistr255_t**, uint32_t*,
	hfsp_callback_args*);
int hfsplib_is_journal_clean(hfsp_volume*);
int hfsplib_is_private_file(hfsp_catalog_key_t*);

int hfsplib_get_hardlink(hfsp_volume *, uint32_t,
			 hfsp_catalog_keyed_record_t *, hfsp_callback_args *);

size_t hfsplib_read_volume_header(void*, hfsp_volume_header_t*);
size_t hfsplib_reada_node(void*, hfsp_node_descriptor_t*, void***, uint16_t**,
	hfsp_btree_file_type, hfsp_volume*, hfsp_callback_args*);
size_t hfsplib_reada_node_offsets(void*, uint16_t*);
size_t hfsplib_read_header_node(void**, uint16_t*, uint16_t,
	hfsp_header_record_t*, void*, void*);
size_t hfsplib_read_catalog_keyed_record(void*, hfsp_catalog_keyed_record_t*,
	int16_t*, hfsp_catalog_key_t*, hfsp_volume*);
size_t hfsplib_read_extent_record(void*, hfsp_extent_record_t*, hfsp_node_kind,
	hfsp_extent_key_t*, hfsp_volume*);
void hfsplib_free_recs(void***, uint16_t**, uint16_t*, hfsp_callback_args*);

size_t hfsplib_read_fork_descriptor(void*, hfsp_fork_t*);
size_t hfsplib_read_extent_descriptors(void*, hfsp_extent_record_t*);
size_t hfsplib_read_unistr255(void*, hfsp_unistr255_t*);
size_t hfsplib_read_bsd_data(void*, hfsp_bsd_data_t*);
size_t hfsplib_read_file_userinfo(void*, hfsp_macos_file_info_t*);
size_t hfsplib_read_file_finderinfo(void*, hfsp_macos_extended_file_info_t*);
size_t hfsplib_read_folder_userinfo(void*, hfsp_macos_folder_info_t*);
size_t hfsplib_read_folder_finderinfo(void*, hfsp_macos_extended_folder_info_t*);
size_t hfsplib_read_journal_info(void*, hfsp_journal_info_t*);
size_t hfsplib_read_journal_header(void*, hfsp_journal_header_t*);

uint16_t hfsplib_make_catalog_key(hfsp_cnid_t, uint16_t, unichar_t*,
	hfsp_catalog_key_t*);
uint16_t hfsplib_make_extent_key(hfsp_cnid_t, uint8_t, uint32_t,
	hfsp_extent_key_t*);
uint16_t hfsplib_get_file_extents(hfsp_volume*, hfsp_cnid_t, uint8_t,
	hfsp_extent_descriptor_t**, hfsp_callback_args*);
int hfsplib_readd_with_extents(hfsp_volume*, void*, uint64_t*, uint64_t,
	uint64_t, hfsp_extent_descriptor_t*, uint16_t, hfsp_callback_args*);

int hfsplib_compare_catalog_keys_cf(const void*, const void*);
int hfsplib_compare_catalog_keys_bc(const void*, const void*);
int hfsplib_compare_extent_keys(const void*, const void*);


/* callback wrappers */
void hfsplib_error(const char*, const char*, int, ...) __attribute__ ((format (printf, 1, 4)));
void* hfsplib_malloc(size_t, hfsp_callback_args*);
void* hfsplib_realloc(void*, size_t, hfsp_callback_args*);
void hfsplib_free(void*, hfsp_callback_args*);
int hfsplib_openvoldevice(hfsp_volume*, const char*, uint64_t,hfsp_callback_args*);
void hfsplib_closevoldevice(hfsp_volume*, hfsp_callback_args*);
int hfsplib_readd(hfsp_volume*, void*, uint64_t, uint64_t, hfsp_callback_args*);

#endif /* !_FS_HFSP_LIBHFSP_H_ */
