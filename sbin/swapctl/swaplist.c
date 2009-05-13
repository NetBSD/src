/*	$NetBSD: swaplist.c,v 1.15.4.1 2009/05/13 19:19:06 jym Exp $	*/

/*
 * Copyright (c) 1997 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: swaplist.c,v 1.15.4.1 2009/05/13 19:19:06 jym Exp $");
#endif


#include <sys/param.h>
#include <sys/stat.h>
#include <sys/swap.h>

#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <util.h>

#define	dbtoqb(b) dbtob((int64_t)(b))

/*
 * NOTE:  This file is separate from swapctl.c so that pstat can grab it.
 */

#include "swapctl.h"

int
list_swap(int pri, int kflag, int pflag, int tflag, int dolong, int hflag)
{
	struct	swapent *sep, *fsep;
	long	blocksize;
	char	szbuf[5], usbuf[5], avbuf[5]; /* size, used, avail */
	const	char *header, *suff;
	size_t	l;
	int	hlen, totalsize, size, totalinuse, inuse, ncounted, pathmax;
	int	rnswap, nswap = swapctl(SWAP_NSWAP, 0, 0), i;

	if (nswap < 1) {
		puts("no swap devices configured");
		return 0;
	}

	fsep = sep = (struct swapent *)malloc(nswap * sizeof(*sep));
	if (sep == NULL)
		err(1, "malloc");
	rnswap = swapctl(SWAP_STATS, (void *)sep, nswap);
	if (rnswap < 0)
		err(1, "SWAP_STATS");
	if (nswap != rnswap)
		warnx("SWAP_STATS different to SWAP_NSWAP (%d != %d)",
		    rnswap, nswap);

	pathmax = 11;
	switch(kflag) {
		case 1:
			header = "1K-blocks";
			blocksize = 1024;
			suff = "KBytes";
			break;
		case 2:
			suff = "MBytes";
			header = "1M-blocks";
			blocksize = 1024 * 1024;
			break;
		case 3:
			header = "1G-blocks";
			blocksize = 1024 * 1024 * 1024;
			suff = "GBytes";
			break;
		default:
			suff = "blocks";
			if (!hflag) {
				header = getbsize(&hlen, &blocksize);
			} else {
				header = "Size";
				blocksize = 1; suff = ""; /* unused */
			}
			break;
	}
	if (hflag || kflag)
		hlen = strlen(header);

	if (dolong && tflag == 0) {
		for (i = rnswap; i-- > 0; sep++)
			if ((size_t)pathmax < (l = strlen(sep->se_path)))
				pathmax = l;
		sep = fsep;
		(void)printf("%-*s %*s %8s %8s %8s  %s\n",
		    pathmax, "Device", hlen, header,
		    "Used", "Avail", "Capacity", "Priority");
	}
	totalsize = totalinuse = ncounted = 0;
	for (; rnswap-- > 0; sep++) {
		if (pflag && sep->se_priority != pri)
			continue;
		ncounted++;
		size = sep->se_nblks;
		inuse = sep->se_inuse;
		totalsize += size;
		totalinuse += inuse;

		if (dolong && tflag == 0) {
			if (hflag == 0) {
				(void)printf("%-*s %*ld ", pathmax, sep->se_path, hlen,
				    (long)(dbtoqb(size) / blocksize));

				(void)printf("%8ld %8ld %5.0f%%    %d\n",
				    (long)(dbtoqb(inuse) / blocksize),
				    (long)(dbtoqb(size - inuse) / blocksize),
				    (double)inuse / (double)size * 100.0,
				    sep->se_priority);
			} else {
				if ((humanize_number(szbuf, sizeof(szbuf), (dbtoqb(size)),
					"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
					err(1, "humanize_number");
				if ((humanize_number(usbuf, sizeof(usbuf), (dbtoqb(inuse)),
					"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
					err(1, "humanize_number");
				if ((humanize_number(avbuf, sizeof(avbuf), (dbtoqb(size-inuse)),
					"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
					err(1, "humanize_number");
				(void)printf("%-*s %*s ", pathmax, sep->se_path, hlen, szbuf);

				(void)printf("%8s %8s %5.0f%%    %d\n",
				    usbuf, avbuf, 
				    (double)inuse / (double)size * 100.0,
				    sep->se_priority);
			}
		}
	}
	if (tflag) {
		if (hflag) {
			if ((humanize_number(usbuf, sizeof(usbuf), (dbtoqb(totalinuse)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			if ((humanize_number(szbuf, sizeof(szbuf), (dbtoqb(totalsize)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			(void)printf("%s/%s swap space\n", usbuf, szbuf);
		} else {
			(void)printf("%ld/%ld %s swap space\n",
				(long)(dbtoqb(totalinuse) / blocksize),
				(long)(dbtoqb(totalsize) / blocksize),
				suff);
		}
	}
	else if (dolong == 0) {
		if (hflag) {
			if ((humanize_number(szbuf, sizeof(szbuf), (dbtoqb(totalsize)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			if ((humanize_number(usbuf, sizeof(usbuf), (dbtoqb(totalinuse)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			if ((humanize_number(avbuf, sizeof(avbuf), (dbtoqb(totalsize-totalinuse)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			(void)printf("total: %s allocated = %s used, %s available.\n",
				szbuf, usbuf, avbuf);
		} else {
		    printf("total: %ld %s allocated = %ld %s used, "
			   "%ld %s available\n",
		    (long)(dbtoqb(totalsize) / blocksize), suff,
		    (long)(dbtoqb(totalinuse) / blocksize), suff,
		    (long)(dbtoqb(totalsize - totalinuse) / blocksize), suff);
		}
	} else if (ncounted > 1) {
		if (hflag) {
			if ((humanize_number(szbuf, sizeof(szbuf), (dbtoqb(totalsize)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			if ((humanize_number(usbuf, sizeof(usbuf), (dbtoqb(totalinuse)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			if ((humanize_number(avbuf, sizeof(avbuf), (dbtoqb(totalsize-totalinuse)),
				"", HN_AUTOSCALE, (HN_DECIMAL | HN_B | HN_NOSPACE))) == -1)
				err(1, "humanize_number");
			(void)printf("%-*s %*s %8s %8s %5.0f%%\n", pathmax, "Total",
				hlen, szbuf, usbuf, avbuf, 
			        (double)(totalinuse) / (double)totalsize * 100.0);
		} else {
			(void)printf("%-*s %*ld %8ld %8ld %5.0f%%\n", pathmax, "Total",
			    hlen,
			    (long)(dbtoqb(totalsize) / blocksize),
			    (long)(dbtoqb(totalinuse) / blocksize),
			    (long)(dbtoqb(totalsize - totalinuse) / blocksize),
			    (double)(totalinuse) / (double)totalsize * 100.0);
		}
	}
	if (fsep)
		(void)free(fsep);

	return 1;
}
