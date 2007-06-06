/*	$NetBSD: opdump.c,v 1.10 2007/06/06 01:55:01 pooka Exp $	*/

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: opdump.c,v 1.10 2007/06/06 01:55:01 pooka Exp $");
#endif /* !lint */

#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>

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
const char *cacheop_revmap[] = {
	"PUFFS_CACHE_WRITE"
};

void
puffsdump_req(struct puffs_req *preq)
{
	const char **map;

	map = NULL; /* yes, we are all interested in your opinion, gcc */
	switch (PUFFSOP_OPCLASS(preq->preq_opclass)) {
	case PUFFSOP_VFS:
		map = vfsop_revmap;
		break;
	case PUFFSOP_VN:
		map = vnop_revmap;
		break;
	case PUFFSOP_CACHE:
		map = cacheop_revmap;
		break;
	}
		
	printf("\treqid: %" PRIu64 ", opclass %d%s, optype: %s, "
	    "cookie: %p,\n\t\taux: %p, auxlen: %zu\n",
	    preq->preq_id, PUFFSOP_OPCLASS(preq->preq_opclass),
	    PUFFSOP_WANTREPLY(preq->preq_opclass) ? "" : " (FAF)",
	    map[preq->preq_optype], preq->preq_cookie,
	    preq->preq_buf, preq->preq_buflen);
}

void
puffsdump_rv(struct puffs_req *preq)
{

	printf("\tRV reqid: %" PRIu64 ", result: %d %s\n",
	    preq->preq_id, preq->preq_rv,
	    preq->preq_rv ? strerror(preq->preq_rv) : "");
}

void
puffsdump_cookie(void *c, const char *cookiename)
{
	
	printf("\t%scookie: at %p\n", cookiename, c);
}

#if 0
static const char *cn_opnames[] = {
	"LOOKUP",
	"CREATE",
	"DELETE",
	"RENAME"
};
void
puffsdump_cn(struct puffs_cn *pcn)
{

	printf("\tpuffs_cn: %s (%sfollow)\n",
	    cn_opnames[pcn->pcn_nameio & PUFFSLOOKUP_OPMASK],
	    pcn->pcn_nameio&PUFFSLOOKUP_OPTIONS==PUFFSLOOKUP_NOFOLLOW?"no":"");
	/*
	TOFINISH
	*/
}
#endif

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
