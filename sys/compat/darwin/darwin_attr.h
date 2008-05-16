/*	$NetBSD: darwin_attr.h,v 1.3.76.1 2008/05/16 02:23:35 yamt Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_ATTR_H_
#define	_DARWIN_ATTR_H_

#define DARWIN_US_ASCII	0x0600

#define DARWIN_ATTR_BIT_MAP_COUNT	5
#define DARWIN_FSOPT_NOFOLLOW		1

typedef u_int32_t darwin_attrgroup_t;
typedef u_int32_t darwin_text_encoding_t;
typedef u_int32_t darwin_fsobj_type_t;
typedef u_int32_t darwin_fsobj_tag_t;
typedef u_int32_t darwin_fsfile_type_t;
typedef u_int32_t darwin_fsvolid_t;

typedef struct darwin_fsobj_id {
	u_int32_t fid_objno;
	u_int32_t fid_generation;
} darwin_fsobj_id_t;

struct darwin_attrlist {
	u_int16_t bitmapcount;
	u_int16_t reserved;
	darwin_attrgroup_t commonattr;
	darwin_attrgroup_t volattr;
	darwin_attrgroup_t dirattr;
	darwin_attrgroup_t fileattr;
	darwin_attrgroup_t forkattr;
};

typedef struct darwin_attribute_set {
	darwin_attrgroup_t commonattr;
	darwin_attrgroup_t volattr;
	darwin_attrgroup_t dirattr;
	darwin_attrgroup_t fileattr;
	darwin_attrgroup_t forkattr;
} darwin_attribute_set_t;

typedef struct darwin_attrreference {
	long	attr_dataoffset;
	size_t	attr_length;
} darwin_attrreference_t;

struct darwin_diskextent {
	u_int32_t startblock;
	u_int32_t blockcount;
};

typedef struct darwin_diskextent darwin_extentrecord[8];

typedef u_int32_t darwin_vol_capabilities_set_t[4];

#define DARWIN_VOL_CAPABILITIES_FORMAT		0
#define DARWIN_VOL_CAPABILITIES_INTERFACES	1
#define DARWIN_VOL_CAPABILITIES_RESERVED1	2
#define DARWIN_VOL_CAPABILITIES_RESERVED2	3

typedef struct darwin_vol_capabilities_attr {
	darwin_vol_capabilities_set_t capabilities;
	darwin_vol_capabilities_set_t valid;
} darwin_vol_capabilities_attr_t;

#define DARWIN_VOL_CAP_FMT_PERSISTENTOBJECTIDS	0x00000001
#define DARWIN_VOL_CAP_FMT_SYMBOLICLINKS	0x00000002
#define DARWIN_VOL_CAP_FMT_HARDLINKS		0x00000004

#define DARWIN_VOL_CAP_INT_SEARCHFS 		0x00000001
#define DARWIN_VOL_CAP_INT_ATTRLIST		0x00000002
#define DARWIN_VOL_CAP_INT_NFSEXPORT		0x00000004
#define DARWIN_VOL_CAP_INT_READDIRATTR		0x00000008

typedef struct darwin_vol_attributes_attr {
	darwin_attribute_set_t validattr;
	darwin_attribute_set_t nativeattr;
} darwin_vol_attributes_attr_t;

#define DARWIN_DIR_MNTSTATUS_MNTPOINT		0x00000001

#define DARWIN_ATTR_CMN_NAME			0x00000001
#define DARWIN_ATTR_CMN_DEVID			0x00000002
#define DARWIN_ATTR_CMN_FSID			0x00000004
#define DARWIN_ATTR_CMN_OBJTYPE			0x00000008
#define DARWIN_ATTR_CMN_OBJTAG			0x00000010
#define DARWIN_ATTR_CMN_OBJID			0x00000020
#define DARWIN_ATTR_CMN_OBJPERMANENTID		0x00000040
#define DARWIN_ATTR_CMN_PAROBJID		0x00000080
#define DARWIN_ATTR_CMN_SCRIPT			0x00000100
#define DARWIN_ATTR_CMN_CRTIME			0x00000200
#define DARWIN_ATTR_CMN_MODTIME			0x00000400
#define DARWIN_ATTR_CMN_CHGTIME			0x00000800
#define DARWIN_ATTR_CMN_ACCTIME			0x00001000
#define DARWIN_ATTR_CMN_BKUPTIME		0x00002000
#define DARWIN_ATTR_CMN_FNDRINFO		0x00004000
#define DARWIN_ATTR_CMN_OWNERID			0x00008000
#define DARWIN_ATTR_CMN_GRPID			0x00010000
#define DARWIN_ATTR_CMN_ACCESSMASK		0x00020000
#define DARWIN_ATTR_CMN_FLAGS			0x00040000
#define DARWIN_ATTR_CMN_NAMEDATTRCOUNT		0x00080000
#define DARWIN_ATTR_CMN_NAMEDATTRLIST		0x00100000
#define DARWIN_ATTR_CMN_USERACCESS		0x00200000

#define DARWIN_ATTR_CMN_VALIDMASK		0x003FFFFF
#define DARWIN_ATTR_CMN_SETMASK			0x0007FF00
#define DARWIN_ATTR_CMN_VOLSETMASK		0x00006700

#define DARWIN_ATTR_VOL_FSTYPE			0x00000001
#define DARWIN_ATTR_VOL_SIGNATURE		0x00000002
#define DARWIN_ATTR_VOL_SIZE			0x00000004
#define DARWIN_ATTR_VOL_SPACEFREE		0x00000008
#define DARWIN_ATTR_VOL_SPACEAVAIL		0x00000010
#define DARWIN_ATTR_VOL_MINALLOCATION		0x00000020
#define DARWIN_ATTR_VOL_ALLOCATIONCLUMP		0x00000040
#define DARWIN_ATTR_VOL_IOBLOCKSIZE		0x00000080
#define DARWIN_ATTR_VOL_OBJCOUNT		0x00000100
#define DARWIN_ATTR_VOL_FILECOUNT		0x00000200
#define DARWIN_ATTR_VOL_DIRCOUNT		0x00000400
#define DARWIN_ATTR_VOL_MAXOBJCOUNT		0x00000800
#define DARWIN_ATTR_VOL_MOUNTPOINT		0x00001000
#define DARWIN_ATTR_VOL_NAME			0x00002000
#define DARWIN_ATTR_VOL_MOUNTFLAGS		0x00004000
#define DARWIN_ATTR_VOL_MOUNTEDDEVICE		0x00008000
#define DARWIN_ATTR_VOL_ENCODINGSUSED		0x00010000
#define DARWIN_ATTR_VOL_CAPABILITIES		0x00020000
#define DARWIN_ATTR_VOL_ATTRIBUTES		0x40000000
#define DARWIN_ATTR_VOL_INFO			0x80000000

#define DARWIN_ATTR_VOL_VALIDMASK		0xC003FFFF
#define DARWIN_ATTR_VOL_SETMASK			0x80002000

#define DARWIN_ATTR_DIR_LINKCOUNT		0x00000001
#define DARWIN_ATTR_DIR_ENTRYCOUNT		0x00000002
#define DARWIN_ATTR_DIR_MOUNTSTATUS		0x00000004

#define DARWIN_ATTR_DIR_VALIDMASK		0x00000007
#define DARWIN_ATTR_DIR_SETMASK			0x00000000

#define DARWIN_ATTR_FILE_LINKCOUNT		0x00000001
#define DARWIN_ATTR_FILE_TOTALSIZE		0x00000002
#define DARWIN_ATTR_FILE_ALLOCSIZE		0x00000004
#define DARWIN_ATTR_FILE_IOBLOCKSIZE		0x00000008
#define DARWIN_ATTR_FILE_CLUMPSIZE		0x00000010
#define DARWIN_ATTR_FILE_DEVTYPE		0x00000020
#define DARWIN_ATTR_FILE_FILETYPE		0x00000040
#define DARWIN_ATTR_FILE_FORKCOUNT		0x00000080
#define DARWIN_ATTR_FILE_FORKLIST		0x00000100
#define DARWIN_ATTR_FILE_DATALENGTH		0x00000200
#define DARWIN_ATTR_FILE_DATAALLOCSIZE		0x00000400
#define DARWIN_ATTR_FILE_DATAEXTENTS		0x00000800
#define DARWIN_ATTR_FILE_RSRCLENGTH		0x00001000
#define DARWIN_ATTR_FILE_RSRCALLOCSIZE		0x00002000
#define DARWIN_ATTR_FILE_RSRCEXTENTS		0x00004000

#define DARWIN_ATTR_FILE_VALIDMASK		0x00007FFF
#define DARWIN_ATTR_FILE_SETMASK		0x00000020

#define DARWIN_ATTR_FORK_TOTALSIZE		0x00000001
#define DARWIN_ATTR_FORK_ALLOCSIZE		0x00000002

#define DARWIN_ATTR_FORK_VALIDMASK		0x00000003
#define DARWIN_ATTR_FORK_SETMASK		0x00000000

#define DARWIN_SRCHFS_START 			0x00000001
#define DARWIN_SRCHFS_MATCHPARTIALNAMES 	0x00000002
#define DARWIN_SRCHFS_MATCHDIRS 		0x00000004
#define DARWIN_SRCHFS_MATCHFILES 		0x00000008
#define DARWIN_SRCHFS_NEGATEPARAMS 		0x80000000
#define DARWIN_SRCHFS_VALIDOPTIONSMASK		0x8000000F

struct darwin_fssearchblock {
	struct darwin_attrlist *returnattrs;
	void *returnbuffer;
	size_t returnbuffersize;
	u_long maxmatches;
	struct timeval timelimit;
	void *searchparams1;
	size_t sizeofsearchparams1;
	void *searchparams2;
	size_t sizeofsearchparams2;
	struct darwin_attrlist searchattrs;
};


struct darwin_searchstate {
	u_char reserved[556];
};

#define DARWIN_FST_EOF (-1)

#endif /* _DARWIN_ATTR_H_ */
