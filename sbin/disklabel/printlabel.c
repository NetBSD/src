/*	$NetBSD: printlabel.c,v 1.16 2011/01/06 21:39:01 apb Exp $	*/

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: printlabel.c,v 1.16 2011/01/06 21:39:01 apb Exp $");
#endif	/* not lint */

#include <sys/param.h>

#define DKTYPENAMES
#define FSTYPENAMES
#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/disklabel.h>
#else
#include <sys/disklabel.h>
#endif /* HAVE_NBTOOL_CONFIG_H */

#include <stdio.h>

#include "extern.h"


void
showinfo(FILE *f, struct disklabel *lp, const char *specialname)
{
	int	i, j;

	(void)fprintf(f, "# %s:\n", specialname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		(void)fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		(void)fprintf(f, "type: %" PRIu16 "\n", lp->d_type);
	(void)fprintf(f, "disk: %.*s\n", (int) sizeof(lp->d_typename),
	    lp->d_typename);
	(void)fprintf(f, "label: %.*s\n", (int) sizeof(lp->d_packname),
	    lp->d_packname);
	(void)fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		(void)fprintf(f, " removable");
	if (lp->d_flags & D_ECC)
		(void)fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		(void)fprintf(f, " badsect");
	(void)fprintf(f, "\n");
	(void)fprintf(f, "bytes/sector: %" PRIu32 "\n", lp->d_secsize);
	(void)fprintf(f, "sectors/track: %" PRIu32 "\n", lp->d_nsectors);
	(void)fprintf(f, "tracks/cylinder: %" PRIu32 "\n", lp->d_ntracks);
	(void)fprintf(f, "sectors/cylinder: %" PRIu32 "\n", lp->d_secpercyl);
	(void)fprintf(f, "cylinders: %" PRIu32 "\n", lp->d_ncylinders);
	(void)fprintf(f, "total sectors: %" PRIu32 "\n", lp->d_secperunit);
	(void)fprintf(f, "rpm: %" PRIu32 "\n", lp->d_rpm);
	(void)fprintf(f, "interleave: %" PRIu32 "\n", lp->d_interleave);
	(void)fprintf(f, "trackskew: %" PRIu32 "\n", lp->d_trackskew);
	(void)fprintf(f, "cylinderskew: %" PRIu32 "\n", lp->d_cylskew);
	(void)fprintf(f, "headswitch: %" PRIu32 "\t\t# microseconds\n",
	    lp->d_headswitch);
	(void)fprintf(f, "track-to-track seek: %" PRIu32 "\t# microseconds\n",
	    lp->d_trkseek);
	(void)fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		(void)fprintf(f, "%" PRIu32 " ", lp->d_drivedata[j]);
	(void)fprintf(f, "\n\n");
	(void)fflush(f);
}

void
showpartition(FILE *f, struct disklabel *lp, int i, int ctsformat)
{
	struct partition *pp = lp->d_partitions + i;
	if (pp->p_size == 0)
		return;

	if (ctsformat && lp->d_secpercyl && lp->d_nsectors) {
		char sbuf[64], obuf[64];

		(void)snprintf(sbuf, sizeof(sbuf),
                    "%" PRIu32 "/%" PRIu32 "/%" PRIu32,
		    pp->p_size/lp->d_secpercyl,
		    (pp->p_size%lp->d_secpercyl) / lp->d_nsectors,
		    pp->p_size%lp->d_nsectors);

		(void)snprintf(obuf, sizeof(obuf),
                    "%" PRIu32 "/%" PRIu32 "/%" PRIu32,
		    pp->p_offset/lp->d_secpercyl,
		    (pp->p_offset%lp->d_secpercyl) / lp->d_nsectors,
		    pp->p_offset%lp->d_nsectors);

		(void)fprintf(f, " %c: %9s %9s ",
		    'a' + i, sbuf, obuf);
	} else {
		(void)fprintf(f, " %c: %9" PRIu32 " %9" PRIu32 " ",
		    'a' + i, pp->p_size, pp->p_offset);
	}

	if ((unsigned) pp->p_fstype < FSMAXTYPES)
		(void)fprintf(f, "%10.10s", fstypenames[pp->p_fstype]);
	else
		(void)fprintf(f, "%10" PRIu8, pp->p_fstype);

	switch (pp->p_fstype) {
	case FS_UNUSED:				/* XXX */
		(void)fprintf(f, "  %5" PRIu32 " %5" PRIu64 " %5.5s ",
		    pp->p_fsize, (uint64_t)pp->p_fsize * pp->p_frag, "");
		break;

	case FS_BSDFFS:
	case FS_ADOS:
	case FS_APPLEUFS:
		(void)fprintf(f, "  %5" PRIu32 " %5" PRIu64 " %5" PRIu16 " ",
		    pp->p_fsize, (uint64_t)pp->p_fsize * pp->p_frag, pp->p_cpg);
		break;

	case FS_BSDLFS:
		(void)fprintf(f, "  %5" PRIu32 " %5" PRIu64 " %5" PRIu16 " ",
		    pp->p_fsize, (uint64_t)pp->p_fsize * pp->p_frag, pp->p_sgs);
		break;

	case FS_EX2FS:
		(void)fprintf(f, "  %5" PRIu32 " %5" PRIu64 "       ",
		    pp->p_fsize, (uint64_t)pp->p_fsize * pp->p_frag);
		break;

	case FS_ISO9660:
		(void)fprintf(f, "  %6" PRIu32 "            ",
		    pp->p_cdsession);
		break;

	default:
		(void)fprintf(f, "%20.20s", "");
		break;
	}
	if (lp->d_secpercyl != 0) {
		(void)fprintf(f, " # (Cyl. %6" PRIu32,
		    pp->p_offset / lp->d_secpercyl);

		if (pp->p_offset > lp->d_secperunit)
		    putc('+', f);
		else if (pp->p_offset % lp->d_secpercyl)
		    putc('*', f);
		else
		    putc(' ', f);

		(void)fprintf(f, "- %6" PRIu32,
		    (pp->p_offset + 
		    pp->p_size + lp->d_secpercyl - 1) /
		    lp->d_secpercyl - 1);

		if ((pp->p_offset + pp->p_size) > lp->d_secperunit)
		    putc('+', f);
		else if ((pp->p_offset + pp->p_size) % lp->d_secpercyl)
		    putc('*', f);

		(void)fprintf(f, ")\n");
	} else
		(void)fprintf(f, "\n");
}

void
showpartitions(FILE *f, struct disklabel *lp, int ctsformat)
{
	int	i;

	(void)fprintf(f, "%" PRIu16 " partitions:\n", lp->d_npartitions);
	(void)fprintf(f,
	    "#        size    offset     fstype [fsize bsize cpg/sgs]\n");

	for (i = 0; i < lp->d_npartitions; i++)
		showpartition(f, lp, i, ctsformat);
	(void)fflush(f);
}
