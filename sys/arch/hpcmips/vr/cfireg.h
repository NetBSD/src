/* $NetBSD: cfireg.h,v 1.1.104.1 2008/06/02 13:22:11 mjf Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Common Flash Interface
 */

#define CFI_TOTAL_SIZE		0x50

#define CFI_READ_CFI_QUERY	0x98
#define CFI_QUERY_OFFSET	0xaa	/* AMD/Fujitu needs this */

#define CFI_MANUFACT_REG	0x00
#define CFI_DEVCODE_REG		0x01
#define CFI_BLOCK_STAT_REG	0x02
#define CFI_QUERY_ID_STR_REG	0x10
#define CFI_QUERY_ID_STR	"QRY"
#define CFI_QUERY_ID_STR0	'Q'
#define CFI_QUERY_ID_STR1	'R'
#define CFI_QUERY_ID_STR2	'Y'
#define CFI_PRIM_COMM_REG0	0x13
#define CFI_PRIM_COMM_REG1	0x14
#define CFI_PRIM_EXT_TBL_REG0	0x15
#define CFI_PRIM_EXT_TBL_REG1	0x16
#define CFI_ALT_COMM_REG0	0x17
#define CFI_ALT_COMM_REG1	0x18
#define CFI_ALT_EXT_TBL_REG0	0x19
#define CFI_ALT_EXT_TBL_REG1	0x1a
#define CFI_VCC_MIN_REG		0x1b
#define CFI_VCC_MAX_REG		0x1c
#define CFI_VPP_MIN_REG		0x1d
#define CFI_VPP_MAX_REG		0x1e
#define CFI_TYP_WORD_PROG_REG	0x1f
#define CFI_TYP_BUF_WRITE_REG	0x20
#define CFI_TYP_BLOCK_ERASE_REG	0x21
#define CFI_TYP_CHIP_ERASE_REG	0x22
#define CFI_MAX_WORD_PROG_REG	0x23
#define CFI_MAX_BUF_WRITE_REG	0x24
#define CFI_MAX_BLOCK_ERASE_REG	0x25
#define CFI_MAX_CHIP_ERASE_REG	0x26
#define CFI_DEVICE_SIZE_REG	0x27
#define CFI_DEVICE_IF_REG0	0x28
#define CFI_DEVICE_IF_REG1	0x29
#define CFI_MAX_WBUF_SIZE_REG0	0x2a
#define CFI_MAX_WBUF_SIZE_REG1	0x2b
#define CFI_NUM_ERASE_BLK_REG	0x2c

#define CFI_EBLK1_INFO_REG	0x2d
#define CFI_EBLK_INFO_SIZE	4
#define CFI_EBLK_INFO_NSECT0	0x00
#define CFI_EBLK_INFO_NSECT1	0x01
#define CFI_EBLK_INFO_SECSIZE0	0x02
#define CFI_EBLK_INFO_SECSIZE1	0x03

#define CFI_EBLK1_INFO_REG0	0x2d
#define CFI_EBLK1_INFO_REG1	0x2e
#define CFI_EBLK1_INFO_REG2	0x2f
#define CFI_EBLK1_INFO_REG3	0x30
#define CFI_EBLK2_INFO_REG0	0x31
#define CFI_EBLK2_INFO_REG1	0x32
#define CFI_EBLK2_INFO_REG2	0x33
#define CFI_EBLK2_INFO_REG3	0x34
#define CFI_EBLK3_INFO_REG0	0x35
#define CFI_EBLK3_INFO_REG1	0x36
#define CFI_EBLK3_INFO_REG2	0x37
#define CFI_EBLK3_INFO_REG3	0x38
#define CFI_EBLK4_INFO_REG0	0x39
#define CFI_EBLK4_INFO_REG1	0x3a
#define CFI_EBLK4_INFO_REG2	0x3b
#define CFI_EBLK4_INFO_REG3	0x3c


#define CFI_COMMSET_INTEL0	0x01
#define CFI_COMMSET_INTEL1	0x00
#define CFI_COMMSET_AMDFJITU0	0x02
#define CFI_COMMSET_AMDFJITU1	0x00
