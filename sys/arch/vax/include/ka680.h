/*	$NetBSD: ka680.h,v 1.2.156.1 2017/12/03 11:36:48 jdolecek Exp $	*/
/*
 * Copyright (c) 1998 Ludd, University of Lule}, Sweden.
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

/* Done by Michael Kukat (michael@unixiron.org) */

/*
 * KA675/680 (VAX4000/[45]00) specific definitions.
 */

#ifndef _VAX_KA680_H_
#define _VAX_KA680_H_

/* IPR bits definitions */
#define        PCSTS_FLUSH              4
#define        PCSTS_ENABLE             2
#define        PCTAG_PARITY    0x80000000
#define        PCTAG_VALID              1

/* memory addresses of interest */
#define        KA680_INVFLT    0x20200000
#define        KA680_INVFLTSZ  32768
#define        KA680_CCR       0x23000000
#define        KA680_TAGST     0x2d000000
#define        KA680_TAGSZ     32768

#define        CCR_CENA        0x00000001
#define        CCR_SPECIO      0x00000010

#define        KA680_BWF0      0x20080014
#define        BWF0_FEN        0x01000000

#endif /* _VAX_KA680_H_ */
