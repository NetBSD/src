/*	$NetBSD: printlabel.c,v 1.1 2000/12/24 07:08:03 lukem Exp $	*/

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: printlabel.c,v 1.1 2000/12/24 07:08:03 lukem Exp $");
#endif	/* not lint */

#include <sys/param.h>

#define DKTYPENAMES
#define FSTYPENAMES
#include <sys/disklabel.h>

#include <stdio.h>

#include "extern.h"


void
showinfo(FILE *f, struct disklabel *lp, const char *specname)
{
	int	i, j;

	(void) fprintf(f, "# %s:\n", specname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		(void) fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		(void) fprintf(f, "type: %d\n", lp->d_type);
	(void) fprintf(f, "disk: %.*s\n", (int) sizeof(lp->d_typename),
	    lp->d_typename);
	(void) fprintf(f, "label: %.*s\n", (int) sizeof(lp->d_packname),
	    lp->d_packname);
	(void) fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		(void) fprintf(f, " removable");
	if (lp->d_flags & D_ECC)
		(void) fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		(void) fprintf(f, " badsect");
	(void) fprintf(f, "\n");
	(void) fprintf(f, "bytes/sector: %ld\n", (long) lp->d_secsize);
	(void) fprintf(f, "sectors/track: %ld\n", (long) lp->d_nsectors);
	(void) fprintf(f, "tracks/cylinder: %ld\n", (long) lp->d_ntracks);
	(void) fprintf(f, "sectors/cylinder: %ld\n", (long) lp->d_secpercyl);
	(void) fprintf(f, "cylinders: %ld\n", (long) lp->d_ncylinders);
	(void) fprintf(f, "total sectors: %ld\n", (long) lp->d_secperunit);
	(void) fprintf(f, "rpm: %ld\n", (long) lp->d_rpm);
	(void) fprintf(f, "interleave: %ld\n", (long) lp->d_interleave);
	(void) fprintf(f, "trackskew: %ld\n", (long) lp->d_trackskew);
	(void) fprintf(f, "cylinderskew: %ld\n", (long) lp->d_cylskew);
	(void) fprintf(f, "headswitch: %ld\t\t# microseconds\n",
		(long) lp->d_headswitch);
	(void) fprintf(f, "track-to-track seek: %ld\t# microseconds\n",
		(long) lp->d_trkseek);
	(void) fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		(void) fprintf(f, "%d ", lp->d_drivedata[j]);
	(void) fprintf(f, "\n\n");
	(void) fflush(f);
}

void
showpartitions(FILE *f, struct disklabel *lp, int ctsformat)
{
	int	i;
	struct partition *pp;

	(void) fprintf(f, "%d partitions:\n", lp->d_npartitions);
	(void) fprintf(f,
	    "#        size   offset     fstype   [fsize bsize cpg/sgs]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			if (ctsformat && lp->d_secpercyl && lp->d_nsectors) {
				char sbuf[32], obuf[32];
				sprintf(sbuf, "%d/%d/%d",
				   pp->p_size/lp->d_secpercyl,
				   (pp->p_size%lp->d_secpercyl) /
					   lp->d_nsectors,
				   pp->p_size%lp->d_nsectors);

				sprintf(obuf, "%d/%d/%d",
				   pp->p_offset/lp->d_secpercyl,
				   (pp->p_offset%lp->d_secpercyl) /
					   lp->d_nsectors,
				   pp->p_offset%lp->d_nsectors);
				(void) fprintf(f, "  %c: %8s %8s ",
				   'a' + i, sbuf, obuf);
			} else
				(void) fprintf(f, "  %c: %8d %8d ", 'a' + i,
				   pp->p_size, pp->p_offset);
			if ((unsigned) pp->p_fstype < FSMAXTYPES)
				(void) fprintf(f, "%10.10s",
					       fstypenames[pp->p_fstype]);
			else
				(void) fprintf(f, "%10d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_UNUSED:				/* XXX */
				(void) fprintf(f, "    %5d %5d %5.5s ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag, "");
				break;

			case FS_BSDFFS:
			case FS_ADOS:
				(void) fprintf(f, "    %5d %5d %5d ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag,
				    pp->p_cpg);
				break;

			case FS_BSDLFS:
				(void) fprintf(f, "    %5d %5d %5d ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag,
				    pp->p_sgs);
				break;

			case FS_EX2FS:
				(void) fprintf(f, "    %5d %5d       ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag);
				break;

			default:
				(void) fprintf(f, "%22.22s", "");
				break;
			}
			if (lp->d_secpercyl != 0) {
				(void) fprintf(f, "  # (Cyl. %4d",
				    pp->p_offset / lp->d_secpercyl);
				if (pp->p_offset % lp->d_secpercyl)
				    putc('*', f);
				else
				    putc(' ', f);
				(void) fprintf(f, "- %d",
				    (pp->p_offset + 
				    pp->p_size + lp->d_secpercyl - 1) /
				    lp->d_secpercyl - 1);
				if ((pp->p_offset + pp->p_size)
				    % lp->d_secpercyl)
				    putc('*', f);
				(void) fprintf(f, ")\n");
			} else
				(void) fprintf(f, "\n");
		}
	}
	(void) fflush(f);
}
