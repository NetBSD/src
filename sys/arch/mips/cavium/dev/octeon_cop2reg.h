/*	$NetBSD: octeon_cop2reg.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2008 Internet Initiative Japan, Inc.
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
 * Cavium-Specific Instruction Summary
 * Cavium Networks-Specific Instruction Description
 *
 * DMFC2 / DMTC2 instructions
 */

#ifndef _CN30XXCOP2REG_H_
#define _CN30XXCOP2REG_H_

/* 3DES */

#define	CVM_MF_3DES_IV			0x0084	/* Load IV from 3DES Unit */
#define	CVM_MF_3DES_KEY			0x0080	/* Load Key from 3DES Unit */
#define	CVM_MF_3DES_RESULT		0x0088	/* Load Result from 3DES Unit */
#define	CVM_MT_3DES_DEC			0x408e	/* 3DES Decrypt */
#define	CVM_MT_3DES_DEC_CBC		0x408c	/* 3DES CBC Decrypt */
#define	CVM_MT_3DES_ENC			0x408a	/* 3DES Encrypt */
#define	CVM_MT_3DES_ENC_CBC		0x4088	/* 3DES CBC Encrypt */
#define	CVM_MT_3DES_IV			0x0084	/* Load IV into 3DES Unit */
#define	CVM_MT_3DES_KEY			0x0080	/* Load Key into 3DES Unit */
#define	CVM_MT_3DES_RESULT		0x0098	/* Load Result into 3DES Unit */

/* AES */

#define	CVM_MF_AES_INP0			0x0111	/* Load INP0 from AES Unit */
#define	CVM_MF_AES_IV			0x0102	/* Load IV from AES Unit */
#define	CVM_MF_AES_KEY			0x0104	/* Load Key from AES Unit */
#define	CVM_MF_AES_KEYLENGTH		0x0110	/* Load Keylength from AES Unit */
#define	CVM_MF_AES_RESINP		0x0100	/* Load Result/Input from AES Unit */
#define	CVM_MT_AES_DEC_CBC0		0x010c	/* AES CBC Decrypt (part 1) */
#define	CVM_MT_AES_DEC_CBC1		0x310d	/* AES CBC Decrypt (part 2) */
#define	CVM_MT_AES_DEC0			0x010e	/* AES Decrypt (part 1) */
#define	CVM_MT_AES_DEC1			0x310f	/* AES Decrypt (part 2) */
#define	CVM_MT_AES_ENC_CBC0		0x0108	/* AES CBC Encrypt (part 1) */
#define	CVM_MT_AES_ENC_CBC1		0x3109	/* AES CBC Encrypt (part 2) */
#define	CVM_MT_AES_ENC0			0x010a	/* AES Encrypt (part 1) */
#define	CVM_MT_AES_ENC1			0x310b	/* AES Encrypt (part 2) */
#define	CVM_MT_AES_IV			0x0102	/* Load IV into AES Unit */
#define	CVM_MT_AES_KEY			0x0104	/* Load Key into AES Unit */
#define	CVM_MT_AES_KEYLENGTH		0x0110	/* Load Keylength into AES Unit */
#define	CVM_MT_AES_RESINP		0x0100	/* Load Result/Input into AES Unit */

/* CRC */

#define	CVM_MF_CRC_IV			0x0201	/* Load IV from CRC Unit */
#define	CVM_MF_CRC_IV_REFLECT		0x0203	/* Load IV from CRC Unit Reflected */
#define	CVM_MF_CRC_LEN			0x0202	/* Load Length from CRC Unit */
#define	CVM_MF_CRC_POLYNOMIAL		0x0200	/* Load Polynomial from CRC Unit */
#define	CVM_MT_CRC_BYTE			0x0204	/* CRC for a Byte */
#define	CVM_MT_CRC_BYTE_REFLECT		0x0214	/* CRC for a Byte Reflected */
#define	CVM_MT_CRC_DWORD		0x1207	/* CRC for a Double-word */
#define	CVM_MT_CRC_DWORD_REFLECT	0x1217	/* CRC for a Double-word Reflected */
#define	CVM_MT_CRC_HALF			0x0205	/* CRC for a Halfword */
#define	CVM_MT_CRC_HALF_REFLECT		0x0215	/* CRC for a Halfword Reflected */
#define	CVM_MT_CRC_IV			0x0201	/* Load IV into CRC Unit */
#define	CVM_MT_CRC_IV_REFLECT		0x0211	/* Load IV into CRC Unit Reflected */
#define	CVM_MT_CRC_LEN			0x1202	/* Load Length into CRC Unit */
#define	CVM_MT_CRC_POLYNOMIAL		0x4200	/* Load Polynomial into CRC Unit */
#define	CVM_MT_CRC_POLYNOMIAL_REFLECT	0x4210	/* Load Polynomial into CRC Unit Reflected */
#define	CVM_MT_CRC_VAR			0x1208	/* CRC for Variable Length */
#define	CVM_MT_CRC_VAR_REFLECT		0x1218	/* CRC for Variable Length Reflected */
#define	CVM_MT_CRC_WORD			0x0206	/* CRC for a Word */
#define	CVM_MT_CRC_WORD_REFLECT		0x0216	/* CRC for a Word Reflected */

/* GFM */

#define	CVM_MF_GFM_MUL			0x0258	/* Load Multiplier from GFM Unit */
#define	CVM_MF_GFM_RESINP		0x025a	/* Load Result/Input from GFM Unit */
#define	CVM_MF_GFM_POLY			0x025e	/* Load Polynomial from GFM Unit */
#define	CVM_MT_GFM_MUL			0x0258	/* Load Multiplier into GFM Unit */
#define	CVM_MT_GFM_RESINP		0x025a	/* Load Result/Input into GFM Unit */
#define	CVM_MT_GFM_XOR0			0x025c	/* XOR into GFM Unit */
#define	CVM_MT_GFM_XORMUL1		0x425d	/* XOR and GFM Multiply */
#define	CVM_MT_GFM_POLY			0x025e	/* Load Polynomial into GFM Unit */

/* Hash */

#define	CVM_MF_HSH_DAT			0x0040	/* Load Data from HSH Unit (narrow mode) */
#define	CVM_MF_HSH_DATW			0x0240	/* Load Data from HSH Unit (wide mode) */
#define	CVM_MF_HSH_IV			0x0048	/* Load IV from HSH Unit (narrow mode) */
#define	CVM_MF_HSH_IVW			0x0250	/* Load IV from HSH Unit (wide mode) */
#define	CVM_MT_HSH_DAT			0x0040	/* Load Data into HSH Unit (narrow mode) */
#define	CVM_MT_HSH_DATW			0x0240	/* Load Data into HSH Unit (wide mode) */
#define	CVM_MT_HSH_IV			0x0048	/* Load IV into HSH Unit (narrow mode) */
#define	CVM_MT_HSH_IVW			0x0250	/* Load IV into HSH Unit (wide mode) */
#define	CVM_MT_HSH_STANDARD5		0x4047	/* MD5 Hash */
#define	CVM_MT_HSH_STARTSHA		0x4057	/* SHA-1 Hash */
#define	CVM_MT_HSH_STARTSHA256		0x404f	/* SHA-256 Hash */
#define	CVM_MT_HSH_STARTSHA512		0x424f	/* SHA-512 Hash */

/* KASUMI */

#define	CVM_MF_KAS_KEY			0x0080	/* Load Key from KASUMI Unit */
#define	CVM_MT_KAS_ENC_CBC		0x4089	/* KASUMI CBC Encrypt */
#define	CVM_MT_KAS_ENC			0x408b	/* KASUMI Encrypt */
#define	CVM_MT_KAS_KEY			0x0080	/* Load Key into KASUMI Unit */
#define	CVM_MT_KAS_RESULT		0x0098	/* Load Result into KASUMI Unit */

#endif /* _CN30XXCOP2REG_H_ */
