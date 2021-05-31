/*	$NetBSD: dkcksum.c,v 1.1.2.2 2021/05/31 22:15:20 cjep Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 *
 * from: NetBSD: dkcksum.c,v 1.14 2013/05/03 16:05:12 matt Exp
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)dkcksum.c	8.1 (Berkeley) 6/5/93";
#else
__RCSID("$NetBSD: dkcksum.c,v 1.1.2.2 2021/05/31 22:15:20 cjep Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <lib/libkern/libkern.h>

uint16_t
dkcksum(const struct disklabel *lp)
{

	return dkcksum_sized(lp, lp->d_npartitions);
}

uint16_t
dkcksum_sized(const struct disklabel *lp, size_t npartitions)
{
	const uint16_t *start, *end;
	uint16_t sum;

	sum = 0;
	start = (const uint16_t *)lp;
	end = (const uint16_t *)&lp->d_partitions[npartitions];
	while (start < end) {
		sum ^= *start++;
	}
	return sum;
}
