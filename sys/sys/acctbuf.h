/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 *	$Id: acctbuf.h,v 1.4 1994/01/28 23:48:32 jtc Exp $
 */

#ifndef _SYS_ACCTBUF_H_
#define _SYS_ACCTBUF_H_

#define ACCT_BSIZE	4096
#define ACCT_NBRECS	((ACCT_BSIZE / sizeof(struct acct)) - 1)

struct acctbufhdr {
	long	_ab_magic;
#define ACCT_MAGIC 0x64696521
	long	_ab_wind;
	long	_ab_rind;
};

struct acctbuf {
	union {
		struct acctbufhdr	_acctbuf_hdr;
		char			pad[sizeof(struct acct)];
	} u;
	struct acct	recs[ACCT_NBRECS];
};

#define ab_magic u._acctbuf_hdr._ab_magic
#define ab_wind	u._acctbuf_hdr._ab_wind
#define ab_rind	u._acctbuf_hdr._ab_rind

#ifdef KERNEL
#ifdef ACCOUNTING
extern struct acctbuf	*acctbufp;

extern void acctbuf_init(void);
#ifdef ACCT_DEBUG
extern void acctbuf_checkbuf(char *);
#endif /* ACCT_DEBUG */
#endif /* ACCOUNTING */
#endif

#endif /* _SYS_ACCTBUF_H_ */
