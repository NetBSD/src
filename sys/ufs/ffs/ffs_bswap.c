/*	$NetBSD: ffs_bswap.c,v 1.6.8.1 1999/12/21 23:20:07 wrstuden Exp $	*/

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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#if !defined(_KERNEL)
#include <string.h>
#endif

void
ffs_sb_swap(o, n, ns)
	struct fs *o, *n;
	int ns;
{
	int i;
	u_int32_t *o32, *n32;
	u_int16_t *o16, *n16;
	
	/* in order to avoid a lot of lines, as the first 52 fields of
	 * the superblock are u_int32_t, we loop here to convert it.
	 */
	o32 = (u_int32_t *)o;
	n32 = (u_int32_t *)n;
	for (i=0; i< 52; i++)
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
	n->fs_fsbtodb =  bswap32(o->fs_fsbtodb);
	/* byteswap the postbl */
	o16 = (ufs_rw32(o->fs_postblformat, ns) == FS_42POSTBLFMT)
		? o->fs_opostbl[0]
		: (int16_t *)((u_int8_t *)o + ufs_rw32(o->fs_postbloff, ns));
	n16 = (ufs_rw32(o->fs_postblformat, ns) == FS_42POSTBLFMT)
		? n->fs_opostbl[0]
		: (int16_t *)((u_int8_t *)n + ufs_rw32(n->fs_postbloff, ns));
	for (i = 0;
		i < ufs_rw32(o->fs_cpc, ns) * ufs_rw32(o->fs_nrpos, ns);
		i++)
		n16[i] = bswap16(o16[i]);
}

void
ffs_dinode_swap(o, n)
	struct dinode *o, *n;
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
ffs_csum_swap(o, n, size)
	struct csum *o, *n;
	int size;
{
	int i;
	u_int32_t *oint, *nint;
	
	oint = (u_int32_t*)o;
	nint = (u_int32_t*)n;

	for (i = 0; i < size / sizeof(u_int32_t); i++)
		nint[i] = bswap32(oint[i]);
}
