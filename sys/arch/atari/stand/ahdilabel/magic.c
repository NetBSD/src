/*	$NetBSD: magic.c,v 1.1.1.1 2000/08/07 09:23:40 leo Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <stdlib.h>

/*
 * Check label magic numbers for AHDI partitions
 */
int
check_magic(fd, offset, flags)
	int			 fd;
	u_int			 offset;
	int			 flags;
{
	u_char	*bblk;
	u_int	 nsec;
	int	 rv = 1;

	nsec = (BBMINSIZE + (DEV_BSIZE - 1)) / DEV_BSIZE;
	bblk = disk_read(fd, offset, nsec);
	if (bblk != NULL) {
		u_int	*end, *p;
		
		end = (u_int *)&bblk[BBMINSIZE - sizeof(struct disklabel)];
		for (p = (u_int *)bblk; p < end; ++p) {
			struct disklabel *dl = (struct disklabel *)&p[1];
			if (((p[0] == NBDAMAGIC && offset == 0 &&
			    !(flags & FORCE_AHDI)) ||
			    (p[0] == AHDIMAGIC && offset != 0) ||
			    (u_char *)dl - bblk == 7168) &&
			    dl->d_npartitions <= MAXPARTITIONS &&
			    dl->d_magic2 == DISKMAGIC &&
			    dl->d_magic  == DISKMAGIC &&
			    dkcksum(dl)  == 0) {
				rv = -3;
				break;
			}
		}
		free(bblk);
	}
	else
		rv = -1;

	return (rv);
}
