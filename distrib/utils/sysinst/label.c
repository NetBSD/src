/*	$NetBSD: label.c,v 1.8 1999/01/21 08:02:18 garbled Exp $	*/

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
__RCSID("$NetBSD: label.c,v 1.8 1999/01/21 08:02:18 garbled Exp $");
#endif

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <util.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local prototypes
 */
static int boringpart __P((partinfo *lp, int i, int rawpart, int bsdpart));

int	checklabel __P((partinfo *lp, int nparts, int rawpart, int bsdpart,
		    int *bad1, int *bad2));
void	translate_partinfo __P((partinfo *lp, struct partition *pp));


/*
 * Return 1 iff partition i in lp should be ignored when checking
 * for  overlapping partitions.
 */
static int
boringpart(lp, i, rawpart, bsdpart)
	partinfo *lp;
	int i;
	int rawpart;
	int bsdpart;
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
checklabel(lp, nparts, rawpart, bsdpart, ovly1, ovly2)
	partinfo *lp;
	int nparts;
	int rawpart;
	int bsdpart;
	int *ovly1;
	int *ovly2;
{
	int i;
	int j;

	*ovly1 = -1;
	*ovly2 = -1;

	for (i = 0; i < nparts - 1; i ++ ) {
		int *ip = lp[i];
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
			if (boringpart(lp, j, rawpart, bsdpart))
				continue;

			jstart = jp[D_OFFSET];
			jstop = jstart + jp[D_SIZE];

			/* overlap? */
			if ((istart <= jstart && jstart < istop) ||
			    (jstart <= istart && istart < jstop)) {
				*ovly1 = i;
				*ovly2 = j;
				return (1);
			}
		}
	}

	return (0);
}


/*
 * Check a disklabel.
 * If there are overlapping active parititons,
 * Ask the user if they want to edit the parittion or give up.
 */
int
edit_and_check_label(lp, nparts, rawpart, bsdpart)
	partinfo *lp;
	int nparts;
	int rawpart;
	int bsdpart;
{
	 while (1) {
		 int i, j;

		 /* first give the user the option to edit the label... */
		 process_menu(MENU_fspartok);

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

	
void
emptylabel(lp)
	partinfo *lp;
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

int
savenewlabel(lp, nparts)
	partinfo *lp;
	int nparts;
{
	FILE *f;
	int i;

#ifdef DEBUG
	f = fopen("/tmp/disktab", "a");
#else
	/* Create the disktab.preinstall */
	run_prog(0, 0, "cp /etc/disktab.preinstall /etc/disktab");
	f = fopen("/etc/disktab", "a");
#endif
	if (logging)
		(void)fprintf(log, "Creating disklabel %s\n", bsddiskname);
	if (scripting)
		(void)fprintf(script, "cat <<EOF >>/etc/disktab\n");
	if (f == NULL) {
		endwin();
		(void)fprintf(stderr, "Could not open /etc/disktab");
		if (logging)
			(void)fprintf(log, "Failed to open /etc/diskabel for appending.\n");
		exit (1);
	}
	(void)fprintf(f, "%s|NetBSD installation generated:\\\n", bsddiskname);
	(void)fprintf(f, "\t:dt=%s:ty=winchester:\\\n", disktype);
	(void)fprintf(f, "\t:nc#%d:nt#%d:ns#%d:\\\n", dlcyl, dlhead, dlsec);
	(void)fprintf(f, "\t:sc#%d:su#%d:\\\n", dlhead*dlsec, dlsize);
	(void)fprintf(f, "\t:se#%d:%s\\\n", sectorsize, doessf);
	if (scripting) {
		(void)fprintf(script, "%s|NetBSD installation generated:\\\n", bsddiskname);
		(void)fprintf(script, "\t:dt=%s:ty=winchester:\\\n", disktype);
		(void)fprintf(script, "\t:nc#%d:nt#%d:ns#%d:\\\n", dlcyl, dlhead, dlsec);
		(void)fprintf(script, "\t:sc#%d:su#%d:\\\n", dlhead*dlsec, dlsize);
		(void)fprintf(script, "\t:se#%d:%s\\\n", sectorsize, doessf);
	}
	for (i = 0; i < nparts; i++) {
		(void)fprintf(f, "\t:p%c#%d:o%c#%d:t%c=%s:",
		    'a'+i, bsdlabel[i][D_SIZE],
		    'a'+i, bsdlabel[i][D_OFFSET],
		    'a'+i, fstype[bsdlabel[i][D_FSTYPE]]);
		if (scripting)
			(void)fprintf(script, "\t:p%c#%d:o%c#%d:t%c=%s:",
			    'a'+i, bsdlabel[i][D_SIZE],
			    'a'+i, bsdlabel[i][D_OFFSET],
			    'a'+i, fstype[bsdlabel[i][D_FSTYPE]]);
		if (bsdlabel[i][D_FSTYPE] == T_42BSD) {
			(void)fprintf (f, "b%c#%d:f%c#%d:ta=4.2BSD:",
				       'a'+i, bsdlabel[i][D_BSIZE],
				       'a'+i, bsdlabel[i][D_FSIZE]);
			if (scripting)
				(void)fprintf (script, "b%c#%d:f%c#%d:ta=4.2BSD:",
					       'a'+i, bsdlabel[i][D_BSIZE],
					       'a'+i, bsdlabel[i][D_FSIZE]);
		}
		if (i < 7)
			(void)fprintf(f, "\\\n");
		else
			(void)fprintf(f, "\n");
		if (scripting) {
			if (i < 7)
				(void)fprintf(script, "\\\n");
			else
				(void)fprintf(script, "\n");
		}
	}
	fclose (f);
	if (scripting)
		(void)fprintf(script, "EOF\n");
	fflush(NULL);
	return(0);
}


void
translate_partinfo(lp, pp)
	partinfo *lp;
	struct partition *pp;
{

	switch (pp->p_fstype) {

	case FS_UNUSED:				/* XXX */
		(*lp)[D_FSTYPE] = T_UNUSED;
		break;

	case FS_SWAP:
		(*lp)[D_FSTYPE] = T_SWAP;
		break;

	case FS_BSDFFS:
		(*lp)[D_FSTYPE] = T_42BSD;
		(*lp)[D_OFFSET] = 0;
		(*lp)[D_SIZE] = 0;
		(*lp)[D_BSIZE] = pp->p_fsize * pp->p_frag;
		(*lp)[D_FSIZE] = pp->p_fsize;
		break;

	case FS_EX2FS:
		(*lp)[D_FSTYPE] = T_UNUSED;	/* XXX */
		(*lp)[D_BSIZE] = pp->p_fsize * pp->p_frag;
		(*lp)[D_FSIZE] = pp->p_fsize;
		break;

	default:
		(*lp)[D_FSTYPE] = T_UNUSED;
		break;
	}
}

/*
 * Read a label from disk into a sysist label structure.
 */
int
incorelabel(dkname, lp)
	const char *dkname;
	partinfo *lp;
{
	struct disklabel lab;
	int fd;
	int i, maxpart;
	struct partition *pp;
	char nambuf[STRSIZE];

	fd = opendisk(dkname, O_RDONLY,  nambuf, STRSIZE, 0);

	if (ioctl(fd, DIOCGDINFO, &lab) < 0) {
		/*XXX err(4, "ioctl DIOCGDINFO");*/
		return(errno);
	}
	close(fd);
	touchwin(stdscr);
	maxpart = getmaxpartitions();
	if (maxpart > 16)
		maxpart = 16;


	/* XXX set globals used by MD code to compute disk size? */
	
	pp = &lab.d_partitions[0];
	emptylabel(lp);
	for (i = 0; i < maxpart; i++) {
		translate_partinfo(lp+i, pp+i);
	}

	return (0);
}
