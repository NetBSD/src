/*	$NetBSD: get.c,v 1.6 2009/11/17 18:58:07 drochner Exp $	*/

/*
 * Copyright (c) 1993-95 Mats O Jansson.  All rights reserved.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: get.c,v 1.6 2009/11/17 18:58:07 drochner Exp $");
#endif

#include "os.h"
#include "get.h"
#include "mopdef.h"

u_char
mopGetChar(const u_char *pkt, int *idx)
{
        u_char ret;

	ret = pkt[*idx];
	*idx = *idx + 1;
	return(ret);
}

u_short
mopGetShort(const u_char *pkt, int *idx)
{
        u_short ret;
	
	ret = pkt[*idx] + pkt[*idx+1]*256;
	*idx = *idx + 2;
	return(ret);
}

u_int32_t
mopGetLong(const u_char *pkt, int *idx)
{
        u_int32_t ret;
	
	ret = pkt[*idx] +
	      pkt[*idx+1]*0x100 +
	      pkt[*idx+2]*0x10000 +
	      pkt[*idx+3]*0x1000000;
	*idx = *idx + 4;
	return(ret);
}

void
mopGetMulti(const u_char *pkt, int *idx, u_char *dest, int size)
{
	int i;

	for (i = 0; i < size; i++) {
	  dest[i] = pkt[*idx+i];
	}  
	*idx = *idx + size;

}

int
mopGetTrans(const u_char *pkt, int trans)
{
	const u_short	*ptype;
	
	if (trans == 0) {
		ptype = (const u_short *)(pkt+12);
		if (ntohs(*ptype) < 1600) {
			trans = TRANS_8023;
		} else {
			trans = TRANS_ETHER;
		}
	}
	return(trans);
}

void
mopGetHeader(const u_char *pkt, int *idx, const u_char **dst, const u_char **src,
	     u_short *proto, int *len, int trans)
{
	*dst = pkt;
	*src = pkt + 6;
	*idx = *idx + 12;

	switch(trans) {
	case TRANS_ETHER:
		*proto = (u_short)(pkt[*idx]*256 + pkt[*idx+1]);
		*idx = *idx + 2;
		*len   = (int)(pkt[*idx+1]*256 + pkt[*idx]);
		*idx = *idx + 2;
		break;
	case TRANS_8023:
		*len   = (int)(pkt[*idx]*256 + pkt[*idx+1]);
		*idx = *idx + 8;
		*proto = (u_short)(pkt[*idx]*256 + pkt[*idx+1]);
		*idx = *idx + 2;
		break;
	}
}

u_short
mopGetLength(const u_char *pkt, int trans)
{
	switch(trans) {
	case TRANS_ETHER:
		return(pkt[15]*256 + pkt[14]);
		break;
	case TRANS_8023:
		return(pkt[12]*256 + pkt[13]);
		break;
	}
	return(0);
}
