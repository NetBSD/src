/*	$NetBSD: vrc4172pcsreg.h,v 1.2 2022/04/08 10:17:53 andvar Exp $	*/

/*
 * Copyright (c) 2000 SATO Kazumi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *	Vrc4172 PCS (Programmable Chip Select) Unit Registers.
 */
#define VRC2_EXCSREG_MAX		0x30

#define VRC2_EXCS0SELL	0x00
#define VRC2_EXCS0SELH	0x02
#define VRC2_EXCS0MSKL	0x04
#define VRC2_EXCS0MSKH	0x06
#define VRC2_EXCS1SELL	0x08
#define VRC2_EXCS1SELH	0x0a
#define VRC2_EXCS1MSKL	0x0c
#define VRC2_EXCS1MSKH	0x0e
#define VRC2_EXCS2SELL	0x10
#define VRC2_EXCS2SELH	0x12
#define VRC2_EXCS2MSKL	0x14
#define VRC2_EXCS2MSKH	0x16
#define VRC2_EXCS3SELL	0x18
#define VRC2_EXCS3SELH	0x1a
#define VRC2_EXCS3MSKL	0x1c
#define VRC2_EXCS3MSKH	0x1e
#define VRC2_EXCS4SELL	0x20
#define VRC2_EXCS4SELH	0x22
#define VRC2_EXCS4MSKL	0x24
#define VRC2_EXCS4MSKH	0x26
#define VRC2_EXCS5SELL	0x28
#define VRC2_EXCS5SELH	0x2a
#define VRC2_EXCS5MSKL	0x2c
#define VRC2_EXCS5MSKH	0x2e

/* for EXCSnSELL */
#define VRC2_EXCSSELLMASK	0xfffe
/* for EXCSnSELH */
#define VRC2_EXCSSELHMASK	0x01ff

/* for EXCSnMSKL */
#define VRC2_EXCSENMASK	0x1
#define VRC2_EXCSEN		0x1
#define VRC2_EXCSDIS		0x00
#define VRC2_EXCSMSKLMASK	0xfffe
/* for EXCSnMSKH */
#define VRC2_EXCSMSKHMASK	0x01ff
/* end */
