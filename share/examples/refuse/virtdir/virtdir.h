/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FUSE_TREE_H_
#define FUSE_TREE_H_	20070405

#include <sys/types.h>
#include <sys/stat.h>

#include <fuse.h>

#include "defs.h"

/* this struct keeps a note of all the info related to a virtual directory entry */
typedef struct virt_dirent_t {
	char		*name;		/* entry name - used as key */
	size_t		 namelen;	/* length of name */
	char		*d_name;	/* component in this directory */
	char		*tgt;		/* any symlink target */
	size_t		 tgtlen;	/* length of symlink target */
	uint8_t		 type;		/* entry type - file, dir, lnk */
	ino_t		 ino;		/* inode number */
} virt_dirent_t;

/* this defines the list of virtual directory entries,
   sorted in name alpha order */
typedef struct virtdir_t {
	uint32_t	 c;		/* count of entries */
	uint32_t	 size;		/* size of allocated list */
	virt_dirent_t	*v;		/* list */
	char		*rootdir;	/* root directory of virtual fs */
	struct stat	 file;		/* stat struct for file entries */
	struct stat	 dir;		/* stat struct for dir entries */
	struct stat	 lnk;		/* stat struct for symlinks */
} virtdir_t;

/* this struct is used to walk through directories */
typedef struct VIRTDIR {
	char		*dirname;	/* directory name */
	int		 dirnamelen;	/* length of directory name */
	virtdir_t	*tp;		/* the directory tree */
	int		 i;		/* current offset in dir tree */
} VIRTDIR;

int virtdir_init(virtdir_t *, const char *, struct stat *, struct stat *, struct stat *);
int virtdir_add(virtdir_t *, const char *, size_t, uint8_t, const char *, size_t);
int virtdir_del(virtdir_t *, const char *, size_t);
virt_dirent_t *virtdir_find(virtdir_t *, const char *, size_t);
virt_dirent_t *virtdir_find_tgt(virtdir_t *, const char *, size_t);
void virtdir_drop(virtdir_t *);
char *virtdir_rootdir(virtdir_t *);

VIRTDIR *openvirtdir(virtdir_t *, const char *);
virt_dirent_t *readvirtdir(VIRTDIR *);
void closevirtdir(VIRTDIR *);

int virtdir_offset(virtdir_t *, virt_dirent_t *);

#endif
