/* $NetBSD: db_disasm.h,v 1.2 2002/04/28 17:10:34 uch Exp $ */

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define	OP_BF	0x8b00
#define	OP_BFS	0x8f00
#define	OP_BT	0x8900
#define	OP_BTS	0x8d00
#define	OP_BRA	0xa000
#define	OP_BRAF	0x0023
#define	OP_BSR	0xb000
#define	OP_BSRF	0x0003
#define	OP_JMP	0x402b
#define	OP_JSR	0x400b
#define	OP_RTS	0xffff

#define	OP_BF_MASK	0xff00
#define	OP_BFS_MASK	0xff00
#define	OP_BT_MASK	0xff00
#define	OP_BTS_MASK	0xff00
#define	OP_BRA_MASK	0xf000
#define	OP_BSR_MASK	0xf000
#define	OP_BRAF_MASK	0xf0ff
#define	OP_BSRF_MASK	0xf0ff
#define	OP_JMP_MASK	0xf0ff
#define	OP_JSR_MASK	0xf0ff
#define	OP_RTS_MASK	0xffff
