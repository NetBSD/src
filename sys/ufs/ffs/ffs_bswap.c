/*	$NetBSD: ffs_bswap.c,v 1.10.6.1 2001/08/25 06:17:16 thorpej Exp $	*/

/*
 * Copyright (c) 1998 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#if defined(_KERNEL)
#include <sys/systm.h>
#endif

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#if !defined(_KERNEL)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define panic(x)	printf("%s\n", (x)), abort()
#endif

void
ffs_sb_swap(struct fs *o, struct fs *n)
{
	int i, needswap, len;
	u_int32_t *o32, *n32;
	u_int16_t *o16, *n16;
	u_int32_t postbloff, postblfmt;

	if (o->fs_magic == FS_MAGIC) {
		needswap = 0;
	} else if (o->fs_magic == bswap32(FS_MAGIC)) {
		needswap = 1;
	} else {
		panic("ffs_sb_swap: can't determine magic");
	}
	postbloff = ufs_rw32(o->fs_postbloff, needswap);
	postblfmt = ufs_rw32(o->fs_postblformat, needswap);

	/*
	 * In order to avoid a lot of lines, as the first 52 fields of
	 * the superblock are u_int32_t, we loop here to convert it.
	 */
	o32 = (u_int32_t *)o;
	n32 = (u_int32_t *)n;
	for (i = 0; i < 52; i++)
		n32[i] = bswap32(o32[i]);

	n->fs_cpc = bswap32(o->fs_cpc);
	n->fs_fscktime = bswap32(o->fs_fscktime);
	n->fs_contigsumsize = bswap32(o->fs_contigsumsize);
	n->fs_maxsymlinklen = bswap32(o->fs_maxsymlinklen);
	n->fs_inodefmt = bswap32(o->fs_inodefmt);
	n->fs_maxfilesize = bswap64(o->fs_maxfilesize);
	n->fs_qbmask = bswap64(o->fs_qbmask);
	n->fs_qfmask = bswap64(o->fs_qfmask);
	n->fs_state = bswap32(o->fs_state);
	n->fs_postblformat = bswap32(o->fs_postblformat);
	n->fs_nrpos = bswap32(o->fs_nrpos);
	n->fs_postbloff = bswap32(o->fs_postbloff);
	n->fs_rotbloff = bswap32(o->fs_rotbloff);
	n->fs_magic = bswap32(o->fs_magic);
	/* byteswap the postbl */
	o16 = (postblfmt == FS_42POSTBLFMT) ? o->fs_opostbl[0] :
	    (int16_t *)((u_int8_t *)o + postbloff);
	n16 = (postblfmt == FS_42POSTBLFMT) ? n->fs_opostbl[0] :
	    (int16_t *)((u_int8_t *)n + postbloff);
	len = postblfmt == FS_42POSTBLFMT ?
	    128 /* fs_opostbl[16][8] */ :
	    ufs_rw32(o->fs_cpc, needswap) * ufs_rw32(o->fs_nrpos, needswap);
	for (i = 0; i < len; i++)
		n16[i] = bswap16(o16[i]);
}

void
ffs_dinode_swap(struct dinode *o, struct dinode *n)
{

	n->di_mode = bswap16(o->di_mode);
	n->di_nlink = bswap16(o->di_nlink);
	n->di_u.oldids[0] = bswap16(o->di_u.oldids[0]);
	n->di_u.oldids[1] = bswap16(o->di_u.oldids[1]);
	n->di_size = bswap64(o->di_size);
	n->di_atime = bswap32(o->di_atime);
	n->di_atimensec = bswap32(o->di_atimensec);
	n->di_mtime = bswap32(o->di_mtime);
	n->di_mtimensec = bswap32(o->di_mtimensec);
	n->di_ctime = bswap32(o->di_ctime);
	n->di_ctimensec = bswap32(o->di_ctimensec);
	memcpy(n->di_db, o->di_db, (NDADDR + NIADDR) * sizeof(u_int32_t));
	n->di_flags = bswap32(o->di_flags);
	n->di_blocks = bswap32(o->di_blocks);
	n->di_gen = bswap32(o->di_gen);
	n->di_uid = bswap32(o->di_uid);
	n->di_gid = bswap32(o->di_gid);
}

void
ffs_csum_swap(struct csum *o, struct csum *n, int size)
{
	int i;
	u_int32_t *oint, *nint;
	
	oint = (u_int32_t*)o;
	nint = (u_int32_t*)n;

	for (i = 0; i < size / sizeof(u_int32_t); i++)
		nint[i] = bswap32(oint[i]);
}
