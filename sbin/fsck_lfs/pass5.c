/*	$NetBSD: pass5.c,v 1.3 2000/05/16 04:55:59 perseant Exp $	*/

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>

#include <string.h>
#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern SEGUSE *seg_table;

void
pass5()
{
	SEGUSE *su;
	struct bufarea *bp;
	int i;

	/*
	 * Check segment holdings against actual holdings.  Check for
	 * "clean" segments that contain live data.
	 */
	for(i=0; i < sblock.lfs_nseg; i++) {
		su = lfs_gseguse(i, &bp);
		if (!(su->su_flags & SEGUSE_DIRTY) &&
		    seg_table[i].su_nbytes>0)
		{
			pwarn("%d bytes contained in 'clean' segment %d\n",
			      seg_table[i].su_nbytes, i);
			if(preen || reply("fix")) {
				su->su_flags |= SEGUSE_DIRTY;
				dirty(bp);
			}
		}
		if ((su->su_flags & SEGUSE_DIRTY) &&
		    su->su_nbytes != seg_table[i].su_nbytes)
		{
			pwarn("segment %d claims %d bytes but has %d\n",
			      i, su->su_nbytes, seg_table[i].su_nbytes);
			if(preen || reply("fix")) {
				su->su_nbytes = seg_table[i].su_nbytes;
				dirty(bp);
			}
		}
		bp->b_flags &= ~B_INUSE;
	}
}
