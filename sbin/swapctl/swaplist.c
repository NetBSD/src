/*	$NetBSD: swaplist.c,v 1.1.1.1 1997/06/12 13:14:11 mrg Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Matthew R. Green for
 *      The NetBSD Foundation.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/stat.h>

#include <vm/vm_swap.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * NOTE:  This file is separate from swapctl.c so that pstat can grab it.
 */

#include "swapctl.h"

void
list_swap(pri, kflag, pflag, tflag, dolong)
	int	pri;
	int	kflag;
	int	pflag;
	int	tflag;
	int	dolong;
{
	struct	swapent *sep;
	long	blocksize;
	char	*header;
	int	hlen, totalsize, size, totalinuse, inuse, ncounted;
	int	rnswap, nswap = swapctl(SWAP_NSWAP, 0, 0);

	if (nswap < 1) {
		puts("no swap devices configured");
		exit(0);
	}

	sep = (struct swapent *)malloc(nswap * sizeof(*sep));
	if (sep == NULL)
		err(1, "malloc");
	rnswap = swapctl(SWAP_STATS, (void *)sep, nswap);
	if (nswap < 0)
		errx(1, "SWAP_STATS");
	if (nswap != rnswap)
		warnx("SWAP_STATS gave different value than SWAP_NSWAP");

	if (dolong && tflag == 0) {
		if (kflag) {
			header = "1K-blocks";
			blocksize = 1024;
			hlen = strlen(header);
		} else
			header = getbsize(&hlen, &blocksize);
		(void)printf("%-11s %*s %8s %8s %8s  %s\n",
		    "Device", hlen, header,
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
			/* XXX handle se_dev == NODEV */
			(void)printf("/dev/%-6s %*d ",
			    devname(sep->se_dev, S_IFBLK),
				hlen, dbtob(size) / blocksize);

			(void)printf("%8d %8d %5.0f%%    %d\n",
			    dbtob(inuse) / blocksize,
			    dbtob(size - inuse) / blocksize,
			    (double)inuse / (double)size * 100.0,
			    sep->se_priority);
		}
	}
	if (tflag)
		(void)printf("%dM/%dM swap space\n",
		    dbtob(totalinuse) / (1024 * 1024),
		    dbtob(totalsize) / (1024 * 1024));
	else if (dolong == 0)
(void)printf("total: %dk bytes allocated = %dk used, %dk available\n",
		    dbtob(totalsize) / 1024,
		    dbtob(totalinuse) / 1024,
		    dbtob(totalsize - totalinuse) / 1024);
	else if (ncounted > 1)
		(void)printf("%-11s %*d %8d %8d %5.0f%%\n", "Total", hlen,
		    dbtob(totalsize) / blocksize,
		    dbtob(totalinuse) / blocksize,
		    dbtob(totalsize - totalinuse) / blocksize,
		    (double)(totalinuse) / (double)totalsize * 100.0);
}
