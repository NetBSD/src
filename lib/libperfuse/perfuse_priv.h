/*  $NetBSD: perfuse_priv.h,v 1.7 2010/09/06 01:17:05 manu Exp $ */

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

#ifndef _PERFUSE_PRIV_H_
#define _PERFUSE_PRIV_H_

#include <syslog.h>
#include <paths.h>
#include <err.h>
#include <sysexits.h>
#include <puffs.h>

#include "perfuse_if.h"
#include "fuse.h"

struct perfuse_state {
	void *ps_private;	/* Private field for libperfuse user */
	struct puffs_usermount *ps_pu;
	struct puffs_node *ps_root;
	uid_t ps_owner_uid;
	int ps_flags;
#define PS_NO_ACCESS	0x0001	/* access is unimplemented; */
#define PS_NO_FSYNC	0x0002	/* fsync is unimplemented */
#define PS_NO_CREAT	0x0004	/* create is unimplemented */
#define PS_INLOOP	0x0008	/* puffs mainloop started */
	long ps_fsid;
	uint32_t ps_max_readahead;
	uint32_t ps_max_write;
	uint64_t ps_syncreads;
	uint64_t ps_syncwrites;
	uint64_t ps_asyncreads;
	uint64_t ps_asyncwrites;
	char *ps_source;
	char *ps_target;
	char *ps_filesystemtype;
	int ps_mountflags;
	uint64_t ps_unique;
	size_t ps_readahead;
	size_t ps_write;
	perfuse_new_msg_fn ps_new_msg;
	perfuse_xchg_msg_fn ps_xchg_msg;
	perfuse_destroy_msg_fn ps_destroy_msg;
	perfuse_get_inhdr_fn ps_get_inhdr;
	perfuse_get_inpayload_fn ps_get_inpayload;
	perfuse_get_outhdr_fn ps_get_outhdr;
	perfuse_get_outpayload_fn ps_get_outpayload;
};


enum perfuse_qtype { PCQ_READDIR, PCQ_READ, PCQ_WRITE, PCQ_AFTERWRITE };

struct perfuse_cc_queue {
	enum perfuse_qtype pcq_type;
	struct puffs_cc *pcq_cc;
	TAILQ_ENTRY(perfuse_cc_queue) pcq_next;
};


struct perfuse_node_data {
	uint64_t pnd_rfh;
	uint64_t pnd_wfh;
	uint64_t pnd_ino;			/* inode */
	uint64_t pnd_nlookup;			/* vnode refcount */
	uint64_t pnd_offset;			/* seek state */
	uint64_t pnd_lock_owner;
	struct dirent *pnd_dirent;		/* native buffer for readdir */
	off_t pnd_dirent_len;
	struct fuse_dirent *pnd_all_fd;		/* FUSE buffer for readdir */
	size_t pnd_all_fd_len;
	TAILQ_HEAD(,perfuse_cc_queue) pnd_pcq;	/* queued requests */
	int pnd_flags;
#define PND_RECLAIMED		0x01	/* reclaim pending */
#define PND_INREADDIR		0x02	/* readdir in progress */
#define PND_DIRTY		0x04	/* There is some data to sync */
#define PND_RFH			0x08	/* Read FH allocated */
#define PND_WFH			0x10	/* Write FH allocated */
#define PND_REMOVED		0x20	/* Node was removed */
#define PND_INWRITE		0x40	/* write in progress */

#define PND_OPEN		(PND_RFH|PND_WFH)	/* File is open */
	puffs_cookie_t pnd_parent;
	int pnd_childcount;
};

#define PERFUSE_NODE_DATA(opc)	\
	((struct perfuse_node_data *)puffs_pn_getpriv((struct puffs_node *)opc))


#define UNSPEC_REPLY_LEN PERFUSE_UNSPEC_REPLY_LEN /* shorter! */
#define NO_PAYLOAD_REPLY_LEN 0

#define GET_INHDR(ps, pm) ps->ps_get_inhdr(pm)
#define GET_INPAYLOAD(ps, pm, type) \
	(struct type *)(void *)ps->ps_get_inpayload(pm)
#define _GET_INPAYLOAD(ps, pm, type) (type)ps->ps_get_inpayload(pm)
#define GET_OUTHDR(ps, pm) ps->ps_get_outhdr(pm)
#define GET_OUTPAYLOAD(ps, pm, type) \
	(struct type *)(void *)ps->ps_get_outpayload(pm)
#define _GET_OUTPAYLOAD(ps, pm, type) (type)ps->ps_get_outpayload(pm)

#define XCHG_MSG(ps, pu, opc, len) ps->ps_xchg_msg(pu, opc, len, wait_reply)
#define XCHG_MSG_NOREPLY(ps, pu, opc, len) \
    ps->ps_xchg_msg(pu, opc, len, no_reply)

__BEGIN_DECLS

struct puffs_node *perfuse_new_pn(struct puffs_usermount *, 
    struct puffs_node *);
void perfuse_destroy_pn(struct puffs_node *);
void perfuse_new_fh(puffs_cookie_t, uint64_t, int);
void perfuse_destroy_fh(puffs_cookie_t, uint64_t);
uint64_t perfuse_get_fh(puffs_cookie_t, int);
uint64_t perfuse_next_unique(struct puffs_usermount *);

char *perfuse_fs_mount(int, ssize_t);


/*
 * opc.c - filesystem operations
 */
int perfuse_fs_unmount(struct puffs_usermount *, int);
int perfuse_fs_statvfs(struct puffs_usermount *, struct statvfs *);
int perfuse_fs_sync(struct puffs_usermount *, int,
    const struct puffs_cred *);
int perfuse_fs_fhtonode(struct puffs_usermount *, void *, size_t,
    struct puffs_newinfo *);
int perfuse_fs_nodetofh(struct puffs_usermount *, puffs_cookie_t,
    void *, size_t *);
void perfuse_fs_suspend(struct puffs_usermount *, int);
int perfuse_node_lookup(struct puffs_usermount *,
    puffs_cookie_t, struct puffs_newinfo *, const struct puffs_cn *);
int perfuse_node_create(struct puffs_usermount *,
    puffs_cookie_t, struct puffs_newinfo *, const struct puffs_cn *,
    const struct vattr *);
int perfuse_node_mknod(struct puffs_usermount *,
    puffs_cookie_t, struct puffs_newinfo *, const struct puffs_cn *,
    const struct vattr *);
int perfuse_node_open(struct puffs_usermount *,
    puffs_cookie_t, int, const struct puffs_cred *);
int perfuse_node_close(struct puffs_usermount *,
    puffs_cookie_t, int, const struct puffs_cred *);
int perfuse_node_access(struct puffs_usermount *,
    puffs_cookie_t, int, const struct puffs_cred *);
int perfuse_node_getattr(struct puffs_usermount *,
    puffs_cookie_t, struct vattr *, const struct puffs_cred *);
int perfuse_node_setattr(struct puffs_usermount *,
    puffs_cookie_t, const struct vattr *, const struct puffs_cred *);
int perfuse_node_poll(struct puffs_usermount *, puffs_cookie_t, int *);
int perfuse_node_mmap(struct puffs_usermount *,
    puffs_cookie_t, vm_prot_t, const struct puffs_cred *);
int perfuse_node_fsync(struct puffs_usermount *,
    puffs_cookie_t, const struct puffs_cred *, int, off_t, off_t);
int perfuse_node_seek(struct puffs_usermount *,
    puffs_cookie_t, off_t, off_t, const struct puffs_cred *);
int perfuse_node_remove(struct puffs_usermount *,
    puffs_cookie_t, puffs_cookie_t, const struct puffs_cn *);
int perfuse_node_link(struct puffs_usermount *,
    puffs_cookie_t, puffs_cookie_t, const struct puffs_cn *);
int perfuse_node_rename(struct puffs_usermount *,
    puffs_cookie_t, puffs_cookie_t, const struct puffs_cn *,
    puffs_cookie_t, puffs_cookie_t, const struct puffs_cn *);
int perfuse_node_mkdir(struct puffs_usermount *,
    puffs_cookie_t, struct puffs_newinfo *, const struct puffs_cn *,
    const struct vattr *);
int perfuse_node_rmdir(struct puffs_usermount *,
    puffs_cookie_t, puffs_cookie_t, const struct puffs_cn *);
int perfuse_node_symlink(struct puffs_usermount *,
    puffs_cookie_t, struct puffs_newinfo *, const struct puffs_cn *,
    const struct vattr *, const char *);
int perfuse_node_readdir(struct puffs_usermount *,
    puffs_cookie_t, struct dirent *, off_t *, size_t *,
    const struct puffs_cred *, int *, off_t *, size_t *);
int perfuse_node_readlink(struct puffs_usermount *,
    puffs_cookie_t, const struct puffs_cred *, char *, size_t *);
int perfuse_node_reclaim(struct puffs_usermount *, puffs_cookie_t);
int perfuse_node_inactive(struct puffs_usermount *, puffs_cookie_t);
int perfuse_node_print(struct puffs_usermount *, puffs_cookie_t);
int perfuse_node_pathconf(struct puffs_usermount *,
    puffs_cookie_t, int, int *);
int perfuse_node_advlock(struct puffs_usermount *,
    puffs_cookie_t, void *, int, struct flock *, int);
int perfuse_node_read(struct puffs_usermount *, puffs_cookie_t,
    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
int perfuse_node_write(struct puffs_usermount *, puffs_cookie_t,
    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
void perfuse_cache_write(struct puffs_usermount *,
    puffs_cookie_t, size_t, struct puffs_cacherun *);

__END_DECLS

#endif /* _PERFUSE_PRIV_H_ */
