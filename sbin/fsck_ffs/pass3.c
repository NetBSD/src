/*	$NetBSD: pass3.c,v 1.21 2013/01/22 09:39:12 dholland Exp $	*/

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
static char sccsid[] = "@(#)pass3.c	8.2 (Berkeley) 4/27/95";
#else
__RCSID("$NetBSD: pass3.c,v 1.21 2013/01/22 09:39:12 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <string.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <dirent.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

void
pass3(void)
{
	struct inoinfo *inp, **inpp;
	int loopcnt, state;
	ino_t orphan;
	struct inodesc idesc;
	char namebuf[MAXNAMLEN+1];
	union dinode *dp;

	for (inpp = &inpsort[inplast - 1]; inpp >= inpsort; inpp--) {
		int inpindex = inpp - inpsort;
		if (got_siginfo) {
			fprintf(stderr,
			    "%s: phase 3: dir %d of %d (%d%%)\n", cdevname(),
			    (int)(inplast - inpindex - 1), (int)inplast,
			    (int)((inplast - inpindex - 1) * 100 / inplast));
			got_siginfo = 0;
		}
#ifdef PROGRESS
		progress_bar(cdevname(), preen ? NULL : "phase 3",
			    inplast - inpindex - 1, inplast);
#endif /* PROGRESS */
		inp = *inpp;
		state = inoinfo(inp->i_number)->ino_state;
		if (inp->i_number == UFS_ROOTINO ||
		    (inp->i_parent != 0 && state != DSTATE))
			continue;
		if (state == DCLEAR)
			continue;
		/*
		 * If we are running with soft updates and we come
		 * across unreferenced directories, we just leave
		 * them in DSTATE which will cause them to be pitched
		 * in pass 4.
		 */
		if (preen &&
		    resolved && usedsoftdep && state == DSTATE) {
			if (inp->i_dotdot >= UFS_ROOTINO)
				inoinfo(inp->i_dotdot)->ino_linkcnt++;
			continue;
		}
		for (loopcnt = 0; ; loopcnt++) {
			orphan = inp->i_number;
			if (inp->i_parent == 0 ||
			    inoinfo(inp->i_parent)->ino_state != DSTATE ||
			    loopcnt > countdirs)
				break;
			inp = getinoinfo(inp->i_parent);
		}
		if (loopcnt <= countdirs) {
			if (linkup(orphan, inp->i_dotdot, NULL))
				inoinfo(lfdir)->ino_linkcnt--;
			continue;
		}
		pfatal("ORPHANED DIRECTORY LOOP DETECTED I=%lu",
		    (u_long)orphan);
		if (reply("RECONNECT") == 0)
			continue;
		dp = ginode(inp->i_parent);
		memset(&idesc, 0, sizeof(struct inodesc));
		idesc.id_type = DATA;
		idesc.id_number = inp->i_parent;
		idesc.id_parent = orphan;
		idesc.id_func = findname;
		idesc.id_name = namebuf;
		idesc.id_uid = iswap32(DIP(dp, uid));
		idesc.id_gid = iswap32(DIP(dp, gid));
		if ((ckinode(dp, &idesc) & FOUND) == 0)
			pfatal("COULD NOT FIND NAME IN PARENT DIRECTORY");
		if (linkup(orphan, inp->i_parent, namebuf)) {
			idesc.id_func = clearentry;
			if (ckinode(ginode(inp->i_parent), &idesc) & FOUND)
				inoinfo(orphan)->ino_linkcnt++;
			inoinfo(lfdir)->ino_linkcnt--;
		}
	}
#ifdef PROGRESS
	if (!preen)
		progress_done();
#endif /* PROGRESS */
}
