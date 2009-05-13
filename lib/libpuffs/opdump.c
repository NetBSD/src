/*	$NetBSD: opdump.c,v 1.25.2.1 2009/05/13 19:18:35 jym Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Pretty-printing helper routines for VFS/VOP request contents */

/* yes, this is pretty much a mess */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: opdump.c,v 1.25.2.1 2009/05/13 19:18:35 jym Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/time.h>

#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>

#include "puffs_priv.h"

/* XXX! */
const char *vfsop_revmap[] = {
	"PUFFS_VFS_MOUNT",
	"PUFFS_VFS_START",
	"PUFFS_VFS_UNMOUNT",
	"PUFFS_VFS_ROOT",
	"PUFFS_VFS_STATVFS",
	"PUFFS_VFS_SYNC",
	"PUFFS_VFS_VGET",
	"PUFFS_VFS_FHTOVP",
	"PUFFS_VFS_VPTOFH",
	"PUFFS_VFS_INIT",
	"PUFFS_VFS_DONE",
	"PUFFS_VFS_SNAPSHOT",
	"PUFFS_VFS_EXTATTCTL",
	"PUFFS_VFS_SUSPEND"
};
/* XXX! */
const char *vnop_revmap[] = {
	"PUFFS_VN_LOOKUP",
	"PUFFS_VN_CREATE",
	"PUFFS_VN_MKNOD",
	"PUFFS_VN_OPEN",
	"PUFFS_VN_CLOSE",
	"PUFFS_VN_ACCESS",
	"PUFFS_VN_GETATTR",
	"PUFFS_VN_SETATTR",
	"PUFFS_VN_READ",
	"PUFFS_VN_WRITE",
	"PUFFS_VN_IOCTL",
	"PUFFS_VN_FCNTL",
	"PUFFS_VN_POLL",
	"PUFFS_VN_KQFILTER",
	"PUFFS_VN_REVOKE",
	"PUFFS_VN_MMAP",
	"PUFFS_VN_FSYNC",
	"PUFFS_VN_SEEK",
	"PUFFS_VN_REMOVE",
	"PUFFS_VN_LINK",
	"PUFFS_VN_RENAME",
	"PUFFS_VN_MKDIR",
	"PUFFS_VN_RMDIR",
	"PUFFS_VN_SYMLINK",
	"PUFFS_VN_READDIR",
	"PUFFS_VN_READLINK",
	"PUFFS_VN_ABORTOP",
	"PUFFS_VN_INACTIVE",
	"PUFFS_VN_RECLAIM",
	"PUFFS_VN_LOCK",
	"PUFFS_VN_UNLOCK",
	"PUFFS_VN_BMAP",
	"PUFFS_VN_STRATEGY",
	"PUFFS_VN_PRINT",
	"PUFFS_VN_ISLOCKED",
	"PUFFS_VN_PATHCONF",
	"PUFFS_VN_ADVLOCK",
	"PUFFS_VN_LEASE",
	"PUFFS_VN_WHITEOUT",
	"PUFFS_VN_GETPAGES",
	"PUFFS_VN_PUTPAGES",
	"PUFFS_VN_BWRITE",
	"PUFFS_VN_GETEXTATTR",
	"PUFFS_VN_LISTEXTATTR",
	"PUFFS_VN_OPENEXTATTR",
	"PUFFS_VN_DELETEEXTATTR",
	"PUFFS_VN_SETEXTATTR",
};
/* XXX! */
const char *cacheop_revmap[] = {
	"PUFFS_CACHE_WRITE"
};
/* XXX! */
const char *errnot_revmap[] = {
	"PUFFS_ERR_MAKENODE",
	"PUFFS_ERR_LOOKUP",
	"PUFFS_ERR_READDIR",
	"PUFFS_ERR_READLINK",
	"PUFFS_ERR_READ",
	"PUFFS_ERR_WRITE",
	"PUFFS_ERR_VPTOFH"
};
/* XXX! */
const char *flush_revmap[] = {
	"PUFFS_INVAL_NAMECACHE_NODE",
	"PUFFS_INVAL_NAMECACHE_DIR",
	"PUFFS_INVAL_NAMECACHE_ALL",
	"PUFFS_INVAL_PAGECACHE_NODE_RANGE",
	"PUFFS_FLUSH_PAGECACHE_NODE_RANGE",
};

void
puffsdump_req(struct puffs_req *preq)
{
	static struct timeval tv_prev;
	struct timeval tv_now, tv;
	const char **map;
	int isvn = 0;

	map = NULL; /* yes, we are all interested in your opinion, gcc */
	switch (PUFFSOP_OPCLASS(preq->preq_opclass)) {
	case PUFFSOP_VFS:
		map = vfsop_revmap;
		break;
	case PUFFSOP_VN:
		map = vnop_revmap;
		isvn = 1;
		break;
	case PUFFSOP_CACHE:
		map = cacheop_revmap;
		break;
	case PUFFSOP_ERROR:
		map = errnot_revmap;
		break;
	case PUFFSOP_FLUSH:
		map = flush_revmap;
		break;
	}

	printf("\treqid: %" PRIu64 ", opclass %d%s, optype: %s, "
	    "cookie: %p,\n\t\taux: %p, auxlen: %zu, pid: %d, lwpid: %d\n",
	    preq->preq_id, PUFFSOP_OPCLASS(preq->preq_opclass),
	    PUFFSOP_WANTREPLY(preq->preq_opclass) ? "" : " (FAF)",
	    map[preq->preq_optype], preq->preq_cookie,
	    preq->preq_buf, preq->preq_buflen,
	    preq->preq_pid, preq->preq_lid);

	if (isvn) {
		switch (preq->preq_optype) {
		case PUFFS_VN_LOOKUP:
			puffsdump_lookup(preq);
			break;
		case PUFFS_VN_READ:
		case PUFFS_VN_WRITE:
			puffsdump_readwrite(preq);
			break;
		case PUFFS_VN_OPEN:
			puffsdump_open(preq);
			break;
		case PUFFS_VN_REMOVE:
		case PUFFS_VN_RMDIR:
		case PUFFS_VN_LINK:
			puffsdump_targ(preq);
			break;
		case PUFFS_VN_READDIR:
			puffsdump_readdir(preq);
			break;
		default:
			break;
		}
	}
	
	PU_LOCK();
	gettimeofday(&tv_now, NULL);
	timersub(&tv_now, &tv_prev, &tv);
	printf("\t\tsince previous call: %lld.%06ld\n",
	    (long long)tv.tv_sec, (long)tv.tv_usec);
	gettimeofday(&tv_prev, NULL);
	PU_UNLOCK();
}

void
puffsdump_rv(struct puffs_req *preq)
{

	if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VN) {
		switch (preq->preq_optype) {
		case PUFFS_VN_LOOKUP:
			puffsdump_lookup_rv(preq);
			break;
		case PUFFS_VN_CREATE:
		case PUFFS_VN_MKDIR:
		case PUFFS_VN_MKNOD:
		case PUFFS_VN_SYMLINK:
			puffsdump_create_rv(preq);
			break;
		case PUFFS_VN_READ:
		case PUFFS_VN_WRITE:
			puffsdump_readwrite_rv(preq);
			break;
		case PUFFS_VN_READDIR:
			puffsdump_readdir_rv(preq);
			break;
		default:
			break;
		}
	}

	printf("\tRV reqid: %" PRIu64 ", result: %d %s\n",
	    preq->preq_id, preq->preq_rv,
	    preq->preq_rv ? strerror(preq->preq_rv) : "");
}

void
puffsdump_cookie(puffs_cookie_t c, const char *cookiename)
{
	
	printf("\t%scookie: at %p\n", cookiename, c);
}

static const char *cn_opnames[] = {
	"LOOKUP",
	"CREATE",
	"DELETE",
	"RENAME"
};

void
puffsdump_cn(struct puffs_kcn *pkcn)
{

	printf("\t\tpuffs_cn: \"%s\", len %zu op %s (flags 0x%x)\n",
	    pkcn->pkcn_name, pkcn->pkcn_namelen,
	    cn_opnames[pkcn->pkcn_nameiop & NAMEI_OPMASK],
	    pkcn->pkcn_flags);
}

void
puffsdump_lookup(struct puffs_req *preq)
{
	struct puffs_vnmsg_lookup *lookup_msg = (void *)preq;

	puffsdump_cn(&lookup_msg->pvnr_cn);
}

void
puffsdump_lookup_rv(struct puffs_req *preq)
{
	struct puffs_vnmsg_lookup *lookup_msg = (void *)preq;

	printf("\t\tnew node %p, type 0x%x,\n\t\tsize 0x%"PRIu64", dev 0x%llx\n",
	    lookup_msg->pvnr_newnode, lookup_msg->pvnr_vtype,
	    lookup_msg->pvnr_size, (unsigned long long)lookup_msg->pvnr_rdev);
}

void
puffsdump_create_rv(struct puffs_req *preq)
{
	/* XXX: wrong type, but we know it fits the slot */
	struct puffs_vnmsg_create *create_msg = (void *)preq;

	printf("\t\tnew node %p\n", create_msg->pvnr_newnode);
}

void
puffsdump_readwrite(struct puffs_req *preq)
{
	struct puffs_vnmsg_rw *rw_msg = (void *)preq;

	printf("\t\toffset: %" PRId64 ", resid %zu, ioflag 0x%x\n",
	    rw_msg->pvnr_offset, rw_msg->pvnr_resid, rw_msg->pvnr_ioflag);
}

void
puffsdump_readwrite_rv(struct puffs_req *preq)
{
	struct puffs_vnmsg_rw *rw_msg = (void *)preq;

	printf("\t\tresid after op: %zu\n", rw_msg->pvnr_resid);
}

void
puffsdump_readdir_rv(struct puffs_req *preq)
{
	struct puffs_vnmsg_readdir *readdir_msg = (void *)preq;

	printf("\t\tresid after op: %zu, eofflag %d\n",
	    readdir_msg->pvnr_resid, readdir_msg->pvnr_eofflag);
}

void
puffsdump_open(struct puffs_req *preq)
{
	struct puffs_vnmsg_open *open_msg = (void *)preq;

	printf("\t\tmode: 0x%x\n", open_msg->pvnr_mode);
}

void
puffsdump_targ(struct puffs_req *preq)
{
	struct puffs_vnmsg_remove *remove_msg = (void *)preq; /* XXX! */

	printf("\t\ttarget cookie: %p\n", remove_msg->pvnr_cookie_targ);
}

void
puffsdump_readdir(struct puffs_req *preq)
{
	struct puffs_vnmsg_readdir *readdir_msg = (void *)preq;

	printf("\t\tread offset: %" PRId64 "\n", readdir_msg->pvnr_offset);
}

void
/*ARGSUSED*/
puffsdump_creds(struct puffs_cred *pcr)
{

}

void
puffsdump_int(int value, const char *name)
{

	printf("\tint (%s): %d\n", name, value);
}
