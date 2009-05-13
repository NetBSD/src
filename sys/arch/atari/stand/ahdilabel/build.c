/*	$NetBSD: build.c,v 1.2.14.1 2009/05/13 17:16:31 jym Exp $	*/

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
#include <strings.h>

/*
 * Build AHDI auxillary roots (extended partitions).
 * Always make first 3 partitions real.
 */
int
ahdi_buildlabel (struct ahdi_ptable *ptable)
{
	int			i, j;
	struct ahdi_ptable	old_table;

	/* Copy original ptable */
	bcopy (ptable, &old_table, sizeof (struct ahdi_ptable));

	/*
	 * Adjust partition starts.  Do this first because some
	 * partitioning tools (ICD) change the relative ordering of
	 * auxiliary roots vs. partitions.
 	 */
	for (i = 0; i < ptable->nparts; i++)
		for (j = 0; j < ptable->nparts; j++)
			if (ptable->parts[j].start ==
		    	ptable->parts[i].root + 1) {
				if (ptable->parts[j].start)
					ptable->parts[j].start--;
				if (ptable->parts[j].size)
					ptable->parts[j].size++;
			}
	for (i = 0; i < ptable->nparts; i++) {
		/* Remove auxiliary roots for first 3 (or all) partitions */
		if (i < 3 || (i < 4 && ptable->nparts < 5)) {
				ptable->parts[i].root = 0;
		/* Create auxiliary roots for other partitions */
		} else {
			ptable->parts[i].root = ptable->parts[i].start;
			if (ptable->parts[i].start)
				ptable->parts[i].start++;
			if (ptable->parts[i].size)
				ptable->parts[i].size--;
		}
	}

	/* Compare old and new */
	for (i = 0; i < ptable->nparts; i++)
		if (ptable->parts[i].root != old_table.parts[i].root ||
		    ptable->parts[i].start != old_table.parts[i].start ||
		    ptable->parts[i].size != old_table.parts[i].size)
			return (1);

	return (0);
}
