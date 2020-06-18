/*	$NetBSD: octeon_faureg.h,v 1.2 2020/06/18 13:52:08 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 */

/*
 * Fetch-and-Add Operations
 */

#ifndef _OCTEON_FAUREG_H_
#define _OCTEON_FAUREG_H_


#define	FAU_MAJOR_DID			0x1e
#define	FAU_SUB_DID			0

/* ---- operations */

/* -- load operations */
#define	POW_LOAD_INCVAL			__BITS(35,14)
#define	POW_LOAD_TAGWAIT		__BIT(13)
/*      reserved			__BiTS(12,11) */
#define	POW_LOAD_REG			__BITS(10,0)

/* -- iobdma store operations */

#define	POW_IOBDMA_LEN			1	/* always 1 for POW */
#define	POW_IOBDMA_INCVAL		POW_LOAD_INCVAL
#define	POW_IOBDMA_TAGWAIT		POW_LOAD_TAGWAIT
#define	POW_IOBDMA_SIZE			__BITS(12,11)
#define	POW_IOBDMA_REG			POW_LOAD_REG

/* -- store operations */
/*      reserved			__BiTS(35,14) */
#define	POW_STORE_NOADD			__BIT(13)
/*      reserved			__BiTS(12,11) */
#define	POW_STORE_REG			__BITS(10,0)

#endif /* _OCTEON_FAUREG_H_ */
