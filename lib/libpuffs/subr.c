/*	$NetBSD: subr.c,v 1.1 2006/10/22 22:52:21 pooka Exp $	*/

/*
 * Copyright (c) 2006 Antti Kantee.  All Rights Reserved.
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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: subr.c,v 1.1 2006/10/22 22:52:21 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/dirent.h>

#include <assert.h>
#include <puffs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Well, you're probably wondering why this isn't optimized.
 * The reason is simple: my available time is not optimized for
 * size ... so please be patient ;)
 */
struct puffs_node *
puffs_newpnode(struct puffs_usermount *puser, void *privdata, enum vtype type)
{
	struct puffs_node *pn;

	pn = calloc(1, sizeof(struct puffs_node));
	if (pn == NULL)
		return NULL;

	pn->pn_mnt = puser;
	pn->pn_data = privdata;
	pn->pn_type = type;

	/* technically not needed */
	memset(&pn->pn_va, 0, sizeof(struct vattr));
	pn->pn_va.va_type = type;

	LIST_INSERT_HEAD(&puser->pu_pnodelst, pn, pn_entries);

	return pn;
}

void
puffs_putpnode(struct puffs_node *pn)
{

	if (pn == NULL)
		return;

	LIST_REMOVE(pn, pn_entries);
	free(pn);
}

int
puffs_gendotdent(struct dirent **dent, ino_t id, int dotdot, size_t *reslen)
{
	const char *name;

	assert(dotdot == 0 || dotdot == 1);
	name = dotdot == 0 ? "." : "..";

	return puffs_nextdent(dent, name, id, DT_DIR, reslen);
}

int
puffs_nextdent(struct dirent **dent, const char *name, ino_t id, uint8_t dtype,
	size_t *reslen)
{
	struct dirent *d = *dent;

	/* check if we have enough room for our dent-aligned dirent */
	if (_DIRENT_RECLEN(d, strlen(name)) > *reslen)
		return 0;

	d->d_fileno = id;
	d->d_type = dtype;
	d->d_namlen = strlen(name);
	(void)memcpy(&d->d_name, name, d->d_namlen);
	d->d_name[d->d_namlen] = '\0';
	d->d_reclen = _DIRENT_SIZE(d);

	*reslen -= d->d_reclen;
	*dent = _DIRENT_NEXT(d);

	return 1;
}


/*
 * Set vattr values for those applicable (i.e. not PUFFS_VNOVAL).
 */
void
puffs_setvattr(struct vattr *vap, const struct vattr *sva)
{

#define SETIFVAL(a) if (sva->a != PUFFS_VNOVAL) vap->a = sva->a
	if (sva->va_type != VNON)
		vap->va_type = sva->va_type;
	SETIFVAL(va_mode);
	SETIFVAL(va_nlink);
	SETIFVAL(va_uid);
	SETIFVAL(va_gid);
	SETIFVAL(va_fsid);
	SETIFVAL(va_size);
	SETIFVAL(va_blocksize);
	SETIFVAL(va_atime.tv_sec);
	SETIFVAL(va_ctime.tv_sec);
	SETIFVAL(va_mtime.tv_sec);
	SETIFVAL(va_birthtime.tv_sec);
	SETIFVAL(va_atime.tv_nsec);
	SETIFVAL(va_ctime.tv_nsec);
	SETIFVAL(va_mtime.tv_nsec);
	SETIFVAL(va_birthtime.tv_nsec);
	SETIFVAL(va_gen);
	SETIFVAL(va_flags);
	SETIFVAL(va_rdev);
	SETIFVAL(va_bytes);
#undef SETIFVAL
	/* ignore va->va_vaflags */
}


static int vdmap[] = {
	DT_UNKNOWN,	/* VNON */
	DT_REG,		/* VREG */
	DT_DIR,		/* VDIR */
	DT_BLK,		/* VBLK */
	DT_CHR,		/* VCHR */
	DT_LNK,		/* VLNK */
	DT_SOCK,	/* VSUCK*/
	DT_FIFO,	/* VFIFO*/
	DT_UNKNOWN	/* VBAD */
};
/* XXX: DT_WHT ? */
int
puffs_vtype2dt(enum vtype vt)
{

	if (vt < 0 || vt > (sizeof(vdmap)/sizeof(vdmap[0])))
		return DT_UNKNOWN;

	return vdmap[vt];
}
