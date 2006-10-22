/*	$NetBSD: opdump.c,v 1.1 2006/10/22 22:52:21 pooka Exp $	*/

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
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__RCSID("$NetBSD: opdump.c,v 1.1 2006/10/22 22:52:21 pooka Exp $");
#endif /* !lint */

#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>

/* XXX! */
const char *vnop_revmap[] = {
	"PUFFS_LOOKUP",
	"PUFFS_CREATE",
	"PUFFS_MKNOD",
	"PUFFS_OPEN",
	"PUFFS_CLOSE",
	"PUFFS_ACCESS",
	"PUFFS_GETATTR",
	"PUFFS_SETATTR",
	"PUFFS_READ",
	"PUFFS_WRITE",
	"PUFFS_IOCTL",
	"PUFFS_FCNTL",
	"PUFFS_POLL",
	"PUFFS_KQFILTER",
	"PUFFS_REVOKE",
	"PUFFS_MMAP",
	"PUFFS_FSYNC",
	"PUFFS_SEEK",
	"PUFFS_REMOVE",
	"PUFFS_LINK",
	"PUFFS_RENAME",
	"PUFFS_MKDIR",
	"PUFFS_RMDIR",
	"PUFFS_SYMLINK",
	"PUFFS_READDIR",
	"PUFFS_READLINK",
	"PUFFS_ABORTOP",
	"PUFFS_INACTIVE",
	"PUFFS_RECLAIM",
	"PUFFS_LOCK",
	"PUFFS_UNLOCK",
	"PUFFS_BMAP",
	"PUFFS_STRATEGY",
	"PUFFS_PRINT",
	"PUFFS_ISLOCKED",
	"PUFFS_PATHCONF",
	"PUFFS_ADVLOCK",
	"PUFFS_LEASE",
	"PUFFS_WHITEOUT",
	"PUFFS_GETPAGES",
	"PUFFS_PUTPAGES",
	"PUFFS_BWRITE",
	"PUFFS_GETEXTATTR",
	"PUFFS_LISTEXTATTR",
	"PUFFS_OPENEXTATTR",
	"PUFFS_DELETEEXTATTR",
	"PUFFS_SETEXTATTR",
};

void
puffsdump_req(struct puffs_req *preq)
{

	printf("\treqid: %llu, opclass %d, optype: %s, cookie: %p,\n"
	    "\t\taux: %p, auxlen: %d\n",
	    preq->preq_id, preq->preq_opclass, vnop_revmap[preq->preq_optype],
	    preq->preq_cookie, preq->preq_aux, preq->preq_auxlen);
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
puffsdump_creds(struct puffs_cred *pcr)
{

}

void
puffsdump_int(int value, const char *name)
{

	printf("\tint (%s): %d\n", name, value);
}
