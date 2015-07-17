/*	$NetBSD: bswap.c,v 1.3 2015/07/17 20:30:21 tsutsui Exp $	*/

/*-
 * Copyright (c) 2009 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <string.h>

#include <sys/types.h>
#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/disklabel.h>
#else
#include <sys/disklabel.h>
#endif /* HAVE_NBTOOL_CONFIG_H */

#include "bswap.h"
#include "dkcksum.h"

static void
bswaplabel(struct disklabel *nlp, const struct disklabel *olp)
{
	int i;

	nlp->d_magic          = bswap32(olp->d_magic);
	nlp->d_type           = bswap16(olp->d_type);
	nlp->d_subtype        = bswap16(olp->d_subtype);

	/* no need to swap char strings */
	memcpy(nlp->d_typename, olp->d_typename, sizeof(nlp->d_typename));

	/* XXX What should we do for d_un (an union of char and pointers) ? */
	memcpy(nlp->d_packname, olp->d_packname, sizeof(nlp->d_packname));

	nlp->d_secsize        = bswap32(olp->d_secsize);
	nlp->d_nsectors       = bswap32(olp->d_nsectors);
	nlp->d_ntracks        = bswap32(olp->d_ntracks);
	nlp->d_ncylinders     = bswap32(olp->d_ncylinders);
	nlp->d_secpercyl      = bswap32(olp->d_secpercyl);
	nlp->d_secperunit     = bswap32(olp->d_secperunit);

	nlp->d_sparespertrack = bswap16(olp->d_sparespertrack);
	nlp->d_sparespercyl   = bswap16(olp->d_sparespercyl);

	nlp->d_acylinders     = bswap32(olp->d_acylinders);

	nlp->d_rpm            = bswap16(olp->d_rpm);
	nlp->d_interleave     = bswap16(olp->d_interleave);
	nlp->d_trackskew      = bswap16(olp->d_trackskew);
	nlp->d_cylskew        = bswap16(olp->d_cylskew);
	nlp->d_headswitch     = bswap32(olp->d_headswitch);
	nlp->d_trkseek        = bswap32(olp->d_trkseek);
	nlp->d_flags          = bswap32(olp->d_flags);

	for (i = 0; i < NDDATA; i++)
		nlp->d_drivedata[i] = bswap32(olp->d_drivedata[i]);

	for (i = 0; i < NSPARE; i++)
		nlp->d_spare[i]     = bswap32(olp->d_spare[i]);

	nlp->d_magic2         = bswap32(olp->d_magic2);
	nlp->d_checksum       = bswap16(olp->d_checksum);

	/* filesystem and partition information: */
	nlp->d_npartitions    = bswap16(olp->d_npartitions);
	nlp->d_bbsize         = bswap32(olp->d_bbsize);
	nlp->d_sbsize         = bswap32(olp->d_sbsize);

	for (i = 0; i < maxpartitions; i++) {
		nlp->d_partitions[i].p_size =
		    bswap32(olp->d_partitions[i].p_size);
		nlp->d_partitions[i].p_offset =
		    bswap32(olp->d_partitions[i].p_offset);
		nlp->d_partitions[i].p_fsize =
		    bswap32(olp->d_partitions[i].p_fsize);
		/* p_fstype and p_frag is uint8_t, so no need to swap */
		nlp->d_partitions[i].p_fstype = olp->d_partitions[i].p_fstype;
		nlp->d_partitions[i].p_frag = olp->d_partitions[i].p_frag;
		nlp->d_partitions[i].p_cpg =
		    bswap16(olp->d_partitions[i].p_cpg);
	}
}

void
targettohlabel(struct disklabel *hlp, const struct disklabel *tlp)
{

	if (bswap32(tlp->d_magic) == DISKMAGIC)
		bswaplabel(hlp, tlp);
	else
		*hlp = *tlp;
	/* update checksum in host endian */
	hlp->d_checksum = 0;
	hlp->d_checksum = dkcksum(hlp);
}

void
htotargetlabel(struct disklabel *tlp, const struct disklabel *hlp)
{

	if (bswap_p)
		bswaplabel(tlp, hlp);
	else
		*tlp = *hlp;

	/* update checksum in target endian */
	tlp->d_checksum = 0;
	tlp->d_checksum = dkcksum_target(tlp);
}

uint16_t
dkcksum_target(struct disklabel *lp)
{
	uint16_t npartitions;

	if (lp->d_magic == DISKMAGIC)
		npartitions = lp->d_npartitions;
	else if (bswap32(lp->d_magic) == DISKMAGIC)
		npartitions = bswap16(lp->d_npartitions);
	else
		npartitions = 0;

	if (npartitions > maxpartitions)
		npartitions = 0;

	return dkcksum_sized(lp, npartitions);
}
