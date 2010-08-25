/*  $NetBSD: fuse.h,v 1.1 2010/08/25 07:16:00 manu Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _FUSE_H
#define _FUSE_H

#define FUSE_KERNEL_VERSION 7
#define FUSE_KERNEL_MINOR_VERSION 12
#define FUSE_ROOT_ID 1
#define FUSE_UNKNOWN_FH (uint64_t)0

#define FUSE_MIN_BUFSIZE 0x21000
#define FUSE_PREF_BUFSIZE (PAGE_SIZE + 0x1000)
#define FUSE_BUFSIZE MAX(FUSE_PREF_BUFSIZE, FUSE_MIN_BUFSIZE)

struct fuse_attr {
	uint64_t	ino;
	uint64_t	size;
	uint64_t	blocks;
	uint64_t	atime;
	uint64_t	mtime;
	uint64_t	ctime;
	uint32_t	atimensec;
	uint32_t	mtimensec;
	uint32_t	ctimensec;
	uint32_t	mode;
	uint32_t	nlink;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	rdev;
	uint32_t	blksize;
	uint32_t	padding;
};

struct fuse_kstatfs {
	uint64_t	blocks;
	uint64_t	bfree;
	uint64_t	bavail;
	uint64_t	files;
	uint64_t	ffree;
	uint32_t	bsize;
	uint32_t	namelen;
	uint32_t	frsize;
	uint32_t	padding;
	uint32_t	spare[6];
};

struct fuse_file_lock {
	uint64_t	start;
	uint64_t	end;
	uint32_t	type;
	uint32_t	pid;
};

/*
 * Various flags
 */
#define FUSE_FATTR_MODE		0x0001
#define FUSE_FATTR_UID		0x0002
#define FUSE_FATTR_GID		0x0004
#define FUSE_FATTR_SIZE		0x0008
#define FUSE_FATTR_ATIME	0x0010
#define FUSE_FATTR_MTIME	0x0020
#define FUSE_FATTR_FH		0x0040
#define FUSE_FATTR_ATIME_NOW	0x0080
#define FUSE_FATTR_MTIME_NOW	0x0100
#define FUSE_FATTR_LOCKOWNER	0x0200

#define FUSE_FOPEN_DIRECT_IO	0x0001
#define FUSE_FOPEN_KEEP_CACHE	0x0002
#define FUSE_FOPEN_NONSEEKABLE	0x0004

#define FUSE_ASYNC_READ		0x0001
#define FUSE_POSIX_LOCKS	0x0002
#define FUSE_FILE_OPS		0x0004
#define FUSE_ATOMIC_O_TRUNC	0x0008
#define FUSE_EXPORT_SUPPORT	0x0010
#define FUSE_BIG_WRITES		0x0020
#define FUSE_DONT_MASK		0x0040

#define FUSE_CUSE_UNRESTRICTED_IOCTL	0x0001

#define FUSE_RELEASE_FLUSH	0x0001

#define FUSE_GETATTR_FH		0x0001

#define FUSE_LK_FLOCK		0x0001

#define FUSE_WRITE_CACHE	0x0001
#define FUSE_WRITE_LOCKOWNER	0x0002

#define FUSE_READ_LOCKOWNER	0x0002

#define FUSE_IOCTL_COMPAT	0x0001
#define FUSE_IOCTL_UNRESTRICTED	0x0002
#define FUSE_IOCTL_RETRY	0x0004

#define FUSE_IOCTL_MAX_IOV	256

#define FUSE_POLL_SCHEDULE_NOTIFY 0x0001

enum fuse_opcode {
	FUSE_LOOKUP	   = 1,
	FUSE_FORGET	   = 2,
	FUSE_GETATTR	   = 3,
	FUSE_SETATTR	   = 4,
	FUSE_READLINK	   = 5,
	FUSE_SYMLINK	   = 6,
	FUSE_MKNOD	   = 8,
	FUSE_MKDIR	   = 9,
	FUSE_UNLINK	   = 10,
	FUSE_RMDIR	   = 11,
	FUSE_RENAME	   = 12,
	FUSE_LINK	   = 13,
	FUSE_OPEN	   = 14,
	FUSE_READ	   = 15,
	FUSE_WRITE	   = 16,
	FUSE_STATFS	   = 17,
	FUSE_RELEASE       = 18,
	FUSE_FSYNC         = 20,
	FUSE_SETXATTR      = 21,
	FUSE_GETXATTR      = 22,
	FUSE_LISTXATTR     = 23,
	FUSE_REMOVEXATTR   = 24,
	FUSE_FLUSH         = 25,
	FUSE_INIT          = 26,
	FUSE_OPENDIR       = 27,
	FUSE_READDIR       = 28,
	FUSE_RELEASEDIR    = 29,
	FUSE_FSYNCDIR      = 30,
	FUSE_GETLK         = 31,
	FUSE_SETLK         = 32,
	FUSE_SETLKW        = 33,
	FUSE_ACCESS        = 34,
	FUSE_CREATE        = 35,
	FUSE_INTERRUPT     = 36,
	FUSE_BMAP          = 37,
	FUSE_DESTROY       = 38,
	FUSE_IOCTL         = 39,
	FUSE_POLL          = 40,

	FUSE_CUSE_INIT     = 4096
};

enum fuse_notify_code {
	FUSE_NOTIFY_POLL   = 1,
	FUSE_NOTIFY_INVAL_INODE = 2,
	FUSE_NOTIFY_INVAL_ENTRY = 3,
	FUSE_NOTIFY_CODE_MAX
};

#define FUSE_MIN_READ_BUFFER 8192

#define FUSE_COMPAT_ENTRY_OUT_SIZE 120

struct fuse_entry_out {
	uint64_t	nodeid;
	uint64_t	generation;
	uint64_t	entry_valid;
	uint64_t	attr_valid;
	uint32_t	entry_valid_nsec;
	uint32_t	attr_valid_nsec;
	struct fuse_attr attr;
};

struct fuse_forget_in {
	uint64_t	nlookup;
};

struct fuse_getattr_in {
	uint32_t	getattr_flags;
	uint32_t	dummy;
	uint64_t	fh;
};

#define FUSE_COMPAT_ATTR_OUT_SIZE 96

struct fuse_attr_out {
	uint64_t	attr_valid;
	uint32_t	attr_valid_nsec;
	uint32_t	dummy;
	struct fuse_attr attr;
};

#define FUSE_COMPAT_MKNOD_IN_SIZE 8

struct fuse_mknod_in {
	uint32_t	mode;
	uint32_t	rdev;
	uint32_t	umask;
	uint32_t	padding;
};

struct fuse_mkdir_in {
	uint32_t	mode;
	uint32_t	umask;
};

struct fuse_rename_in {
	uint64_t	newdir;
};

struct fuse_link_in {
	uint64_t	oldnodeid;
};

struct fuse_setattr_in {
	uint32_t	valid;
	uint32_t	padding;
	uint64_t	fh;
	uint64_t	size;
	uint64_t	lock_owner;
	uint64_t	atime;
	uint64_t	mtime;
	uint64_t	unused2;
	uint32_t	atimensec;
	uint32_t	mtimensec;
	uint32_t	unused3;
	uint32_t	mode;
	uint32_t	unused4;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	unused5;
};

struct fuse_open_in {
	uint32_t	flags;
	uint32_t	unused;
};

struct fuse_create_in {
	uint32_t	flags;
	uint32_t	mode;
	uint32_t	umask;
	uint32_t	padding;
};

struct fuse_open_out {
	uint64_t	fh;
	uint32_t	open_flags; /* FUSE_FOPEN_ */
	uint32_t	padding;
};

struct fuse_release_in {
	uint64_t	fh;
	uint32_t	flags;
	uint32_t	release_flags;
	uint64_t	lock_owner;
};

struct fuse_flush_in {
	uint64_t	fh;
	uint32_t	unused;
	uint32_t	padding;
	uint64_t	lock_owner;
};

struct fuse_read_in {
	uint64_t	fh;
	uint64_t	offset;
	uint32_t	size;
	uint32_t	read_flags;
	uint64_t	lock_owner;
	uint32_t	flags;
	uint32_t	padding;
};

#define FUSE_COMPAT_WRITE_IN_SIZE 24

struct fuse_write_in {
	uint64_t	fh;
	uint64_t	offset;
	uint32_t	size;
	uint32_t	write_flags;
	uint64_t	lock_owner;
	uint32_t	flags;
	uint32_t	padding;
};

struct fuse_write_out {
	uint32_t	size;
	uint32_t	padding;
};

#define FUSE_COMPAT_STATFS_SIZE 48

struct fuse_statfs_out {
	struct fuse_kstatfs st;
};

struct fuse_fsync_in {
	uint64_t	fh;
	uint32_t	fsync_flags;
	uint32_t	padding;
};

struct fuse_setxattr_in {
	uint32_t	size;
	uint32_t	flags;
};

struct fuse_getxattr_in {
	uint32_t	size;
	uint32_t	padding;
};

struct fuse_getxattr_out {
	uint32_t	size;
	uint32_t	padding;
};

struct fuse_lk_in {
	uint64_t	fh;
	uint64_t	owner;
	struct fuse_file_lock lk;
	uint32_t	lk_flags;
	uint32_t	padding;
};

struct fuse_lk_out {
	struct fuse_file_lock lk;
};

struct fuse_access_in {
	uint32_t	mask;
	uint32_t	padding;
};

struct fuse_init_in {
	uint32_t	major;
	uint32_t	minor;
	uint32_t	max_readahead;
	uint32_t	flags;
};

struct fuse_init_out {
	uint32_t	major;
	uint32_t	minor;
	uint32_t	max_readahead;
	uint32_t	flags;
	uint32_t	unused;
	uint32_t	max_write;
};

#define FUSE_CUSE_INIT_INFO_MAX 4096

struct fuse_cuse_init_in {
	uint32_t	major;
	uint32_t	minor;
	uint32_t	unused;
	uint32_t	flags;
};

struct fuse_cuse_init_out {
	uint32_t	major;
	uint32_t	minor;
	uint32_t	unused;
	uint32_t	flags;
	uint32_t	max_read;
	uint32_t	max_write;
	uint32_t	dev_major;		/* chardev major */
	uint32_t	dev_minor;		/* chardev minor */
	uint32_t	spare[10];
};

struct fuse_interrupt_in {
	uint64_t	unique;
};

struct fuse_bmap_in {
	uint64_t	block;
	uint32_t	blocksize;
	uint32_t	padding;
};

struct fuse_bmap_out {
	uint64_t	block;
};

struct fuse_ioctl_in {
	uint64_t	fh;
	uint32_t	flags;
	uint32_t	cmd;
	uint64_t	arg;
	uint32_t	in_size;
	uint32_t	out_size;
};

struct fuse_ioctl_out {
	int32_t	result;
	uint32_t	flags;
	uint32_t	in_iovs;
	uint32_t	out_iovs;
};

struct fuse_poll_in {
	uint64_t	fh;
	uint64_t	kh;
	uint32_t	flags;
	uint32_t   padding;
};

struct fuse_poll_out {
	uint32_t	revents;
	uint32_t	padding;
};

struct fuse_notify_poll_wakeup_out {
	uint64_t	kh;
};

#if 0 /* Duplicated in perfuse.h to avoid making fuse.h public */
/* Send from kernel to proces */
struct fuse_in_header {
	uint32_t	len;
	uint32_t	opcode;
	uint64_t	unique;
	uint64_t	nodeid;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	pid;
	uint32_t	padding;
};

struct fuse_in_arg {
	uint32_t	size;
	const void *value;
};

struct fuse_in {
	struct 		fuse_in_header h;
	uint32_t	argpages:1;	/* Req fits in a page? Always 1 */
	uint32_t	numargs;
	struct fuse_in_arg args[3];	/* args copied to userspace */
};


/* From process to kernel */
struct fuse_out_header {
	uint32_t	len;
	int32_t	error;
	uint64_t	unique;
};
#endif

struct fuse_dirent {
	uint64_t	ino;
	uint64_t	off;	/* offset of next field from after foh */
	uint32_t	namelen;
	uint32_t	type;
	char name[0];
};

#define FUSE_NAME_OFFSET offsetof(struct fuse_dirent, name)
#define FUSE_DIRENT_ALIGN(x) \
	(((x) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1))
#define FUSE_DIRENT_SIZE(d) \
	FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET + (d)->namelen)

struct fuse_notify_inval_inode_out {
	uint64_t	ino;
	int64_t	off;
	int64_t	len;
};

struct fuse_notify_inval_entry_out {
	uint64_t	parent;
	uint32_t	namelen;
	uint32_t	padding;
};

#endif /* _FUSE_H */
