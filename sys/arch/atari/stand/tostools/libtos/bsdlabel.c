/*	$NetBSD: bsdlabel.c,v 1.1 2002/03/22 21:27:59 leo Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libtos.h"
#include "diskio.h"
#include "ahdilbl.h"
#include "disklbl.h"

static int	dkcksum    PROTO((struct disklabel *));

int
bsd_getlabel(dd, dlp, offset)
	disk_t		 *dd;
	struct disklabel *dlp;
	u_int		 offset;
{
	u_char		*bblk;
	u_int		nsec;
	int		rv;

	nsec = (BBMINSIZE + (dd->bsize - 1)) / dd->bsize;
	bblk = disk_read(dd, offset, nsec);
	if (bblk) {
		u_int	*end, *p;
		
		end = (u_int *)&bblk[BBMINSIZE - sizeof(struct disklabel)];
		rv  = 1;
		for (p = (u_int *)bblk; p < end; ++p) {
			struct disklabel *dl = (struct disklabel *)&p[1];
			if (  (  (p[0] == NBDAMAGIC && offset == 0)
			      || (p[0] == AHDIMAGIC && offset != 0)
			      || (u_char *)dl - bblk == 7168
			      )
			   && dl->d_npartitions <= MAXPARTITIONS
			   && dl->d_magic2 == DISKMAGIC
			   && dl->d_magic  == DISKMAGIC
			   && dkcksum(dl)  == 0
			   )	{
				bcopy(dl, dlp, sizeof(*dlp));
				rv = 0;
				break;
			}
		}
		free(bblk);
	}
	else rv = -1;

	return(rv);
}

static int
dkcksum(dl)
	struct disklabel *dl;
{
	u_short	*start, *end, sum = 0;

	start = (u_short *)dl;
	end   = (u_short *)&dl->d_partitions[dl->d_npartitions];
	while (start < end)
		sum ^= *start++;
	return(sum);
}
