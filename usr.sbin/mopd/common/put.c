/*	$NetBSD: put.c,v 1.6 2009/11/17 18:58:07 drochner Exp $	*/

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
__RCSID("$NetBSD: put.c,v 1.6 2009/11/17 18:58:07 drochner Exp $");
#endif

#include "os.h"
#include "mopdef.h"
#include "put.h"

void
mopPutChar(u_char *pkt, int *idx, u_char value)
{
	pkt[*idx] = value;
	*idx = *idx + 1;
}

void
mopPutShort(u_char *pkt, int *idx, u_short value)
{
        int i;
	for (i = 0; i < 2; i++) {
	  pkt[*idx+i] = value % 256;
	  value = value / 256;
	}
	*idx = *idx + 2;
}

void
mopPutLong(u_char *pkt, int *idx, u_int32_t value)
{
        int i;
	for (i = 0; i < 4; i++) {
	  pkt[*idx+i] = value % 256;
	  value = value / 256;
	}
	*idx = *idx + 4;
}

void
mopPutMulti(u_char *pkt, int *idx, const u_char *value, int size)
{
	int i;

	for (i = 0; i < size; i++) {
	  pkt[*idx+i] = value[i];
	}  
	*idx = *idx + size;
}

void
mopPutTime(u_char *pkt, int *idx, time_t value)
{
	time_t tnow;
	struct tm *timenow;

	if ((value == 0)) {
	  tnow = time(NULL);
	} else {
	  tnow = value;
	}

	timenow = localtime(&tnow);

	mopPutChar (pkt,idx,10);
	mopPutChar (pkt,idx,(timenow->tm_year / 100) + 19);
	mopPutChar (pkt,idx,(timenow->tm_year % 100));
	mopPutChar (pkt,idx,(timenow->tm_mon + 1));
	mopPutChar (pkt,idx,(timenow->tm_mday));
	mopPutChar (pkt,idx,(timenow->tm_hour));
	mopPutChar (pkt,idx,(timenow->tm_min));
	mopPutChar (pkt,idx,(timenow->tm_sec));
	mopPutChar (pkt,idx,0x00);
	mopPutChar (pkt,idx,0x00);
	mopPutChar (pkt,idx,0x00);
}

void
mopPutHeader(u_char *pkt, int *idx, const u_char *dst, const u_char *src,
	     u_short proto, int trans)
{
	
	mopPutMulti(pkt, idx, dst, 6);
	mopPutMulti(pkt, idx, src, 6);
	if (trans == TRANS_8023) {
		mopPutShort(pkt, idx, 0);
		mopPutChar (pkt, idx, MOP_K_PROTO_802_DSAP);
		mopPutChar (pkt, idx, MOP_K_PROTO_802_SSAP);
		mopPutChar (pkt, idx, MOP_K_PROTO_802_CNTL);
		mopPutChar (pkt, idx, 0x08);
		mopPutChar (pkt, idx, 0x00);
		mopPutChar (pkt, idx, 0x2b);
	}
#if !defined(__FreeBSD__)
	mopPutChar(pkt, idx, (proto / 256));
	mopPutChar(pkt, idx, (proto % 256));
#else
	if (trans == TRANS_8023) {
		mopPutChar(pkt, idx, (proto / 256));
		mopPutChar(pkt, idx, (proto % 256));
	} else {
		mopPutChar(pkt, idx, (proto % 256));
		mopPutChar(pkt, idx, (proto / 256));
	}
#endif
	if (trans == TRANS_ETHER)
		mopPutShort(pkt, idx, 0);

}

void
mopPutLength(u_char *pkt, int trans, u_short len)
{
	int	 idx = 0;
	
	switch(trans) {
	case TRANS_ETHER:
		idx = 14;
		mopPutChar(pkt, &idx, ((len - 16) % 256));
		mopPutChar(pkt, &idx, ((len - 16) / 256));
		break;
	case TRANS_8023:
		idx = 12;
#if !defined(__FreeBSD__)
		mopPutChar(pkt, &idx, ((len - 14) / 256));
		mopPutChar(pkt, &idx, ((len - 14) % 256));
#else
		mopPutChar(pkt, &idx, ((len - 14) % 256));
		mopPutChar(pkt, &idx, ((len - 14) / 256));
#endif
		break;
	}

}
