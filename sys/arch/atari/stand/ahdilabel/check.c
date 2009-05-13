/*	$NetBSD: check.c,v 1.5.14.1 2009/05/13 17:16:31 jym Exp $	*/

/*
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman, Waldi Ravens and Leo Weppelman.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "privahdi.h"

/* Partitions which have errors */
int	ahdi_errp1;
int	ahdi_errp2;

/*
 * Check partitions for consistency :
 *	GEM vs BGM 
 *	Start sector > 2
 *	End sector < secperunit
 *	other partition overlap
 *	auxiliary root overlap
 *	Number of partitions in root/auxiliary root
 */
int
ahdi_checklabel (struct ahdi_ptable *ptable)
{
	int		i, j, rcount, acount;
	u_int32_t	i_end, j_end;

	ahdi_errp1 = -1;
	ahdi_errp2 = -1;

	if (ptable->nparts < 1 || ptable->nparts > MAXPARTITIONS)
		return (-2);

	rcount = 0;
	acount = 0;

	for (i = 0; i < ptable->nparts; i++) {

		/* GEM vs BGM */
		if (ptable->parts[i].size > 32768) {
			if (AHDI_MKPID (ptable->parts[i].id[0],
			    ptable->parts[i].id[1], ptable->parts[i].id[2])
			    == AHDI_PID_GEM) {
				ahdi_errp1 = i;
				return (-3);
			}
		} else {
			if (AHDI_MKPID (ptable->parts[i].id[0],
			    ptable->parts[i].id[1], ptable->parts[i].id[2])
			    == AHDI_PID_BGM) {
				ahdi_errp1 = i;
				return (-3);
			}
		}

		/* Need 2 free sectors at start for root and bad sector list */
		if (ptable->parts[i].start < 2) {
			ahdi_errp1 = i;
			return (-4);
		}

		i_end = ptable->parts[i].start + ptable->parts[i].size - 1;

		/* Check partition does not extend past end of disk */
		if (i_end >= ptable->secperunit) {
			ahdi_errp1 = i;
			return (-5);
		}

		for (j = i + 1; j < ptable->nparts; j++) {
			/* Check for overlap with other partitions */
			j_end = ptable->parts[j].start + ptable->parts[j].size
			    - 1;
			if ((ptable->parts[j].start >= ptable->parts[i].start
			    && ptable->parts[j].start <= i_end) ||
			    (j_end >= ptable->parts[i].start &&
			     j_end <= i_end)) {
				ahdi_errp1 = i;
				ahdi_errp2 = j;
				return (-6);
			}
			/* Check number of partitions in auxiliary root */
			if (ptable->parts[i].root &&
			    ptable->parts[i].root == ptable->parts[j].root) {
				ahdi_errp1 = i;
				ahdi_errp2 = j;
				return (-9);
			}
		}

		for (j = i; j < ptable->nparts; j++)
			/* Check for overlap with auxiliary root(s) */
			if (ptable->parts[j].root >= ptable->parts[i].start &&
			    ptable->parts[j].root <= i_end) {
				ahdi_errp1 = i;
				return (-7);
			}

		/* Count partitions in root/auxiliary roots */
		if (ptable->parts[i].root)
			acount++;
		else
			rcount++;

	}
	/* Check number of partitions in root sector */
	if (acount)
		rcount++;
	if (rcount > 4)
		return (-8);
	else
		return (1);
}
