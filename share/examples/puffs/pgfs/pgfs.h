/*	$NetBSD: pgfs.h,v 1.1 2011/10/12 01:05:00 yamt Exp $	*/

/*-
 * Copyright (c)2010,2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <puffs.h>
#include <stdbool.h>

puffs_cookie_t pgfs_root_cookie(void);

int pgfs_fs_statvfs(struct puffs_usermount *, struct statvfs *);

int pgfs_node_getattr(struct puffs_usermount *, puffs_cookie_t,
    struct vattr *, const struct puffs_cred *);
int pgfs_node_readdir(struct puffs_usermount *, puffs_cookie_t,
    struct dirent *, off_t *, size_t *, const struct puffs_cred *,
    int *, off_t *, size_t *);
int pgfs_node_lookup(struct puffs_usermount *, puffs_cookie_t,
    struct puffs_newinfo *, const struct puffs_cn *);
int pgfs_node_mkdir(struct puffs_usermount *, puffs_cookie_t,
    struct puffs_newinfo *, const struct puffs_cn *,
    const struct vattr *);
int pgfs_node_create(struct puffs_usermount *, puffs_cookie_t,
    struct puffs_newinfo *, const struct puffs_cn *,
    const struct vattr *);
int pgfs_node_write(struct puffs_usermount *, puffs_cookie_t,
    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
int pgfs_node_read(struct puffs_usermount *, puffs_cookie_t,
    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
int pgfs_node_link(struct puffs_usermount *, puffs_cookie_t,
    puffs_cookie_t, const struct puffs_cn *);
int pgfs_node_remove(struct puffs_usermount *, puffs_cookie_t,
    puffs_cookie_t, const struct puffs_cn *);
int pgfs_node_rmdir(struct puffs_usermount *, puffs_cookie_t,
    puffs_cookie_t, const struct puffs_cn *);
int pgfs_node_inactive(struct puffs_usermount *, puffs_cookie_t);
int pgfs_node_setattr(struct puffs_usermount *, puffs_cookie_t,
    const struct vattr *, const struct puffs_cred *);
int pgfs_node_rename(struct puffs_usermount *, puffs_cookie_t,
    puffs_cookie_t, const struct puffs_cn *, puffs_cookie_t, puffs_cookie_t,
    const struct puffs_cn *);
int pgfs_node_symlink(struct puffs_usermount *, puffs_cookie_t,
    struct puffs_newinfo *, const struct puffs_cn *, const struct vattr *,
    const char *);
int pgfs_node_readlink(struct puffs_usermount *, puffs_cookie_t,
    const struct puffs_cred *, char *, size_t *);
int pgfs_node_access(struct puffs_usermount *, puffs_cookie_t, int,
    const struct puffs_cred *);
int pgfs_node_fsync(struct puffs_usermount *, puffs_cookie_t,
    const struct puffs_cred *, int, off_t, off_t);

typedef uint64_t fileid_t;

#define	PGFS_ROOT_FILEID	1
