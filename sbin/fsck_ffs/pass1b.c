/*	$NetBSD: pass1b.c,v 1.21.48.1 2011/01/20 14:24:54 bouyer Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)pass1b.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: pass1b.c,v 1.21.48.1 2011/01/20 14:24:54 bouyer Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

static  struct dups *duphead;
static int pass1bcheck(struct inodesc *);

void
pass1b(void)
{
	int c, i;
	union dinode *dp;
	struct inodesc idesc;
	ino_t inumber;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1bcheck;
	duphead = duplist;
	inumber = 0;
	for (c = 0; c < sblock->fs_ncg; c++) {
		if (got_siginfo) {
			fprintf(stderr,
			    "%s: phase 1b: cyl group %d of %d (%d%%)\n",
			    cdevname(), c, sblock->fs_ncg,
			    c * 100 / sblock->fs_ncg);
			got_siginfo = 0;
		}
#ifdef PROGRESS
		progress_bar(cdevname(), "phase 1b", c, sblock->fs_ncg);
#endif /* PROGRESS */
		for (i = 0; i < sblock->fs_ipg; i++, inumber++) {
			if (inumber < ROOTINO)
				continue;
			dp = ginode(inumber);
			if (dp == NULL)
				continue;
			idesc.id_number = inumber;
			idesc.id_uid = iswap32(DIP(dp, uid));
			idesc.id_gid = iswap32(DIP(dp, gid));
			if (inoinfo(inumber)->ino_state != USTATE &&
			    (ckinode(dp, &idesc) & STOP))
				return;
		}
	}
#ifdef PROGRESS
	progress_done();
#endif /* PROGRESS */
}

static int
pass1bcheck(struct inodesc *idesc)
{
	struct dups *dlp;
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (chkrange(blkno, 1))
			res = SKIP;
		for (dlp = duphead; dlp; dlp = dlp->next) {
			if (dlp->dup == blkno) {
				blkerror(idesc->id_number, "DUP", blkno);
				dlp->dup = duphead->dup;
				duphead->dup = blkno;
				duphead = duphead->next;
			}
			if (dlp == muldup)
				break;
		}
		if (muldup == 0 || duphead == muldup->next)
			return (STOP);
	}
	return (res);
}
