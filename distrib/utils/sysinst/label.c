/*	$NetBSD: label.c,v 1.1 1997/12/04 11:33:44 jonathan Exp $	*/

/*
 * Copyright 1997 Jonathan Stone
 * All rights reserved.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Jonathan Stone.
 * 4. The name of Jonathan Stone may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JONATHAN STONE ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: label.c,v 1.1 1997/12/04 11:33:44 jonathan Exp $");
#endif

#include <stdio.h>
#include <util.h>
#include <sys/types.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local prototypes
 */


static int boringpart __P((partinfo *lp, int i, int rawpart, int bsdpart));

int checklabel __P((partinfo *lp, int nparts, int rawpart, int bsdpart,
		    int *bad1, int *bad2));

/*
 * Return 1 iff partition i in lp should be ignored when checking
 * for  overlapping partitions.
 */
static int
boringpart(partinfo *lp, int i, int rawpart, int bsdpart)
{

	if (i == rawpart || i == bsdpart ||
	    lp[i][D_FSTYPE] == T_UNUSED || lp[i][D_SIZE] == 0)
		return 1;
	return 0;
}



/*
 * Check a sysinst label structure for overlapping partitions.
 * Returns 0 if no overlapping partition found, nonzero otherwise.
 * Sets reference arguments ovly1 and ovly2 to the indices of
 * overlapping partitions if any are found.
 */
int
checklabel(partinfo *lp, int nparts, int rawpart, int bsdpart, 
	   int *ovly1, int *ovly2)
{
	int i;
	int j;

	*ovly1 = -1;
	*ovly2 = -1;

	for (i = 0; i < nparts - 1; i ++ ) {
		int  *ip = lp[i];
		int istart, istop;

		/* skip unused or reserved partitions */
		if (boringpart(lp, i, rawpart, bsdpart))
			continue;

		/*
		 * check succeding partitions for overlap.
		 * O(n^2), but n is small (currently <= 16).
		 */
		istart = ip[D_OFFSET];
		istop = istart + ip[D_SIZE];

		for (j = i+1; j < nparts; j++) {
			int  *jp = lp[j];
			int jstart, jstop;

			/* skip unused or reserved partitions */
			if (boringpart(lp, i, rawpart, bsdpart))
				continue;

			jstart = jp[D_OFFSET];
			jstop = jstart = jp[D_SIZE];

			/* overlap? */
			if ((istart <= jstart && jstart < istop) ||
			    (jstart <= istart && istart < jstop)) {
				*ovly1 = i;
				*ovly2 = j;
				return(1);
			}
		}
	}

	return(0);
}


/*
 * Check a disklabel.
 * If there are overlapping active parititons,
 * Ask the user if they want to edit the parittion or give up.
 */
int
edit_and_check_label(partinfo *lp, int nparts, int rawpart, int bsdpart)
{
	 while (1) {
		 int i, j;

		 /* first give the user the option to edit the label... */
		 process_menu (MENU_fspartok);

		 /* User thinks the label is OK. check for overlaps.*/
		 if (checklabel(lp, nparts, rawpart, bsdpart, &i, &j) == 0) {
			/* partitions are OK. */
			 return (1);
		 }
		 
		 /* partitions overlap. */
		 msg_display(MSG_partitions_overlap, 'a' + i, 'a' + j);
		 /*XXX*/
		 msg_display_add(MSG_edit_partitions_again);
		 process_menu(MENU_yesno);
		 if (!yesno)
			 return(0);
	 }

	/*NOTREACHED*/
}

	

void emptylabel(partinfo *lp)
{
	register int i, maxpart;

	maxpart = getmaxpartitions();

	for (i = 0; i < maxpart; i++) {
		lp[i][D_FSTYPE] = T_UNUSED;
		lp[i][D_OFFSET] = 0;
		lp[i][D_SIZE] = 0;
		lp[i][D_BSIZE] = 0;
		lp[i][D_FSIZE] = 0;
	}
}

