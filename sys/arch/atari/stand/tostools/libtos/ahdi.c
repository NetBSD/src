/*	$NetBSD: ahdi.c,v 1.1.8.2 2002/03/16 15:56:56 jdolecek Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman, Waldi Ravens.
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
 *      This product includes software developed by
 *			Leo Weppelman and Waldi Ravens.
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
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <xhdi.h>
#include "libtos.h"
#include "diskio.h"
#include "ahdilbl.h"

u_int
ahdi_getparts(dd, ptable, rsec, esec)
	disk_t			*dd;
	ptable_t		*ptable;
	u_int			rsec,
				esec;
{
	struct ahdi_part	*part, *end;
	struct ahdi_root	*root;
	u_int			rv;

	root = disk_read(dd, rsec, 1);
	if (!root) {
		rv = rsec + (rsec == 0);
		goto done;
	}

	if (rsec == AHDI_BBLOCK)
		end = &root->ar_parts[AHDI_MAXRPD];
	else end = &root->ar_parts[AHDI_MAXARPD];
	for (part = root->ar_parts; part < end; ++part) {
		u_int	id = *((u_int32_t *)&part->ap_flg);
		if (!(id & 0x01000000))
			continue;
		if ((id &= 0x00ffffff) == AHDI_PID_XGM) {
			u_int	offs = part->ap_st + esec;
			rv = ahdi_getparts(dd, ptable, offs,
					esec == AHDI_BBLOCK ? offs : esec);
			if (rv)
				goto done;
		} else {
			part_t	*p;
			u_int	i = ++ptable->nparts;
			ptable->parts = xrealloc(ptable->parts,
						i * sizeof *ptable->parts);
			p = &ptable->parts[--i];
			*((u_int32_t *)&p->id) = id << 8;
			p->start = part->ap_st + rsec;
			p->end   = p->start + part->ap_size - 1;
			p->rsec  = rsec;
			p->rent  = part - root->ar_parts;
			p->mod   = 0;
		}
	}
	rv = 0;
done:
	free(root);
	return(rv);
}
