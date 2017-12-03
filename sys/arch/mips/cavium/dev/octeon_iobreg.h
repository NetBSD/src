/*	$NetBSD: octeon_iobreg.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
 * IOB Registers
 */

#ifndef _OCTEON_IOBREG_H_
#define _OCTEON_IOBREG_H_

/* ---- register addresses */

#define	IOB_FAU_TIMEOUT				0x00011800f0000000ULL
#define	IOB_CTL_STATUS				0x00011800f0000050ULL
#define	IOB_INT_SUM				0x00011800f0000058ULL
#define	IOB_INT_ENB				0x00011800f0000060ULL
#define	IOB_PKT_ERR				0x00011800f0000068ULL
#define	IOB_INB_DATA_MATCH			0x00011800f0000070ULL
#define	IOB_INB_CONTROL_MATCH			0x00011800f0000078ULL
#define	IOB_INB_DATA_MATCH_ENB			0x00011800f0000080ULL
#define	IOB_INB_CONTROL_MATCH_ENB		0x00011800f0000088ULL
#define	IOB_OUTB_DATA_MATCH			0x00011800f0000090ULL
#define	IOB_OUTB_CONTROL_MATCH			0x00011800f0000098ULL
#define	IOB_OUTB_DATA_MATCH_ENB			0x00011800f00000a0ULL
#define	IOB_OUTB_CONTROL_MATCH_ENB		0x00011800f00000a8ULL
#define	IOB_BIST_STATUS				0x00011800f00007f8ULL

/* ---- register bits */

/* XXX */

#endif /* _OCTEON_IOBREG_H_ */
