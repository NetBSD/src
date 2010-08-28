/*	$NetBSD: omap2_gpmcreg.h,v 1.4 2010/08/28 04:03:51 kiyohara Exp $	*/
/*
 * Copyright (c) 2007 Microsoft
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _OMAP2430GPMCREG_H
#define _OMAP2430GPMCREG_H

/*
 * Header for OMAP2 General Purpose Memory Controller
 */

/*
 * GPMC register base address, offsets, and size
 */
#ifdef OMAP_2430
#define GPMC_BASE			0x6e000000
#endif
#ifdef OMAP_2420
#define GPMC_BASE			0x6800a000
#endif
#ifdef OMAP_3530
#define GPMC_BASE			0x6e000000
#endif

#define GPMC_REVISION			0x000
#define GPMC_SYSCONFIG			0x010
#define GPMC_SYSSTATUS			0x014
#define GPMC_IRQSTATUS			0x018
#define GPMC_IRQENABLE			0x01C
#define GPMC_TIMEOUT_CONTROL		0x040
#define GPMC_ERR_ADDRESS		0x044
#define GPMC_ERR_TYPE			0x048
#define GPMC_CONFIG			0x050
#define GPMC_STATUS			0x054
#define GPMC_CONFIG1_0			0x060
#define GPMC_CONFIG2_0			0x064
#define GPMC_CONFIG3_0			0x068
#define GPMC_CONFIG4_0			0x06C
#define GPMC_CONFIG5_0			0x070
#define GPMC_CONFIG6_0			0x074
#define GPMC_CONFIG7_0			0x078
#define GPMC_NAND_COMMAND_0		0x07C
#define GPMC_NAND_ADDRESS_0		0x080
#define GPMC_NAND_DATA_0		0x084
#define GPMC_CONFIG1_1			0x090
#define GPMC_CONFIG2_1			0x094
#define GPMC_CONFIG3_1			0x098
#define GPMC_CONFIG4_1			0x09C
#define GPMC_CONFIG5_1			0x0A0
#define GPMC_CONFIG6_1			0x0A4
#define GPMC_CONFIG7_1			0x0A8
#define GPMC_NAND_COMMAND_1		0x0AC
#define GPMC_NAND_ADDRESS_1		0x0B0
#define GPMC_NAND_DATA_1		0x0B4
#define GPMC_CONFIG1_2			0x0C0
#define GPMC_CONFIG2_2			0x0C4
#define GPMC_CONFIG3_2			0x0C8
#define GPMC_CONFIG4_2			0x0CC
#define GPMC_CONFIG5_2			0x0D0
#define GPMC_CONFIG6_2			0x0D4
#define GPMC_CONFIG7_2			0x0D8
#define GPMC_NAND_COMMAND_2		0x0DC
#define GPMC_NAND_ADDRESS_2		0x0E0
#define GPMC_NAND_DATA_2		0x0E4
#define GPMC_CONFIG1_3			0x0F0
#define GPMC_CONFIG2_3			0x0F4
#define GPMC_CONFIG3_3			0x0F8
#define GPMC_CONFIG4_3			0x0FC
#define GPMC_CONFIG5_3			0x100
#define GPMC_CONFIG6_3			0x104
#define GPMC_CONFIG7_3			0x108
#define GPMC_NAND_COMMAND_3		0x10C
#define GPMC_NAND_ADDRESS_3		0x110
#define GPMC_NAND_DATA_3		0x114
#define GPMC_CONFIG1_4			0x120
#define GPMC_CONFIG2_4			0x124
#define GPMC_CONFIG3_4			0x128
#define GPMC_CONFIG4_4			0x12C
#define GPMC_CONFIG5_4			0x130
#define GPMC_CONFIG6_4			0x134
#define GPMC_CONFIG7_4			0x138
#define GPMC_NAND_COMMAND_4		0x13C
#define GPMC_NAND_ADDRESS_4		0x140
#define GPMC_NAND_DATA_4		0x144
#define GPMC_CONFIG1_5			0x150
#define GPMC_CONFIG2_5			0x154
#define GPMC_CONFIG3_5			0x158
#define GPMC_CONFIG4_5			0x15C
#define GPMC_CONFIG5_5			0x160
#define GPMC_CONFIG6_5			0x164
#define GPMC_CONFIG7_5			0x168
#define GPMC_NAND_COMMAND_5		0x16C
#define GPMC_NAND_ADDRESS_5		0x170
#define GPMC_NAND_DATA_5		0x174
#define GPMC_CONFIG1_6			0x180
#define GPMC_CONFIG2_6			0x184
#define GPMC_CONFIG3_6			0x188
#define GPMC_CONFIG4_6			0x18C
#define GPMC_CONFIG5_6			0x190
#define GPMC_CONFIG6_6			0x194
#define GPMC_CONFIG7_6			0x198
#define GPMC_NAND_COMMAND_6		0x19C
#define GPMC_NAND_ADDRESS_6		0x1A0
#define GPMC_NAND_DATA_6		0x1A4
#define GPMC_CONFIG1_7			0x1B0
#define GPMC_CONFIG2_7			0x1B4
#define GPMC_CONFIG3_7			0x1B8
#define GPMC_CONFIG4_7			0x1BC
#define GPMC_CONFIG5_7			0x1C0
#define GPMC_CONFIG6_7			0x1C4
#define GPMC_CONFIG7_7			0x1C8
#define GPMC_NAND_COMMAND_7		0x1CC
#define GPMC_NAND_ADDRESS_7		0x1D0
#define GPMC_NAND_DATA_7		0x1D4
#define GPMC_PREFETCH_CONFIG1		0x1E0
#define GPMC_PREFETCH_CONFIG2		0x1E4
#define GPMC_PREFETCH_CONTROL		0x1EC
#define GPMC_CONFIG1_6			0x180
#define GPMC_CONFIG2_6			0x184
#define GPMC_CONFIG3_6			0x188
#define GPMC_CONFIG4_6			0x18C
#define GPMC_CONFIG5_6			0x190
#define GPMC_CONFIG6_6			0x194
#define GPMC_CONFIG7_6			0x198
#define GPMC_NAND_COMMAND_6		0x19C
#define GPMC_NAND_ADDRESS_6		0x1A0
#define GPMC_NAND_DATA_6		0x1A4
#define GPMC_CONFIG1_7			0x1B0
#define GPMC_CONFIG2_7			0x1B4
#define GPMC_CONFIG3_7			0x1B8
#define GPMC_CONFIG4_7			0x1BC
#define GPMC_CONFIG5_7			0x1C0
#define GPMC_CONFIG6_7			0x1C4
#define GPMC_CONFIG7_7			0x1C8
#define GPMC_NAND_COMMAND_7		0x1CC
#define GPMC_NAND_ADDRESS_7		0x1D0
#define GPMC_NAND_DATA_7		0x1D4
#define GPMC_PREFETCH_CONFIG1		0x1E0
#define GPMC_PREFETCH_CONFIG2		0x1E4
#define GPMC_PREFETCH_CONTROL		0x1EC
#define GPMC_PREFETCH_STATUS		0x1F0
#define GPMC_ECC_CONFIG			0x1F4
#define GPMC_ECC_CONTROL		0x1F8
#define GPMC_ECC_SIZE_CONFIG		0x1FC
#define GPMC_ECC1_RESULT		0x200
#define GPMC_ECC2_RESULT		0x204
#define GPMC_ECC3_RESULT		0x208
#define GPMC_ECC4_RESULT		0x20C
#define GPMC_ECC5_RESULT		0x210
#define GPMC_ECC6_RESULT		0x214
#define GPMC_ECC7_RESULT		0x218
#define GPMC_ECC8_RESULT		0x21C
#define GPMC_ECC9_RESULT		0x220
#define GPMC_TESTMODE_CTRL		0x230
#define GPMC_PSA_LSB			0x234
#define GPMC_PSA_MSB			0x238

#define GPMC_SIZE			(GPMC_PSA_MSB + 4)
#define GPMC_NCS			8	/* # Chip Selects */
#define GPMC_CS_SIZE			(GPMC_CONFIG1_1 - GPMC_CONFIG1_0)

/*
 * GPMC OMAP2430_GPMC_REVISION
 */
#define GPMC_REVISION_REV		__BITS(7,0)
#define GPMC_REVISION_REV_MAJ(r)	(((r) >> 4) & 0xf)
#define GPMC_REVISION_REV_MIN(r)	(((r) >> 0) & 0xf)

/*
 * GPMC CONFIG7_[0-7] bits
 */
#define GPMC_CONFIG7_BASEADDRESS	__BITS(5,0)
#define GPMC_CONFIG7_CSVALID		__BIT(6)
#define GPMC_CONFIG7_MASKADDRESS	__BITS(11,8)
#define GPMC_CONFIG7(m, b)		(((m) << 8) | (((b) >> 24) & 0x3f))
#define GPMC_CONFIG7_MASK_256M		0x0
#define GPMC_CONFIG7_MASK_128M		0x8
#define GPMC_CONFIG7_MASK_64M		0xc
#define GPMC_CONFIG7_MASK_32M		0xe
#define GPMC_CONFIG7_MASK_16M		0xf

static __inline ulong
omap_gpmc_config7_addr(uint32_t r)
{
	return ((r) & GPMC_CONFIG7_BASEADDRESS) << 24;
}
static __inline ulong
omap_gpmc_config7_size(uint32_t r)
{
	uint i;
	uint mask;
	const struct {
		uint  mask;
		ulong size;
	} gpmc_config7_size_tab[5] = {
		{ GPMC_CONFIG7_MASK_256M, (256 << 20) },
		{ GPMC_CONFIG7_MASK_128M, (128 << 20) },
		{ GPMC_CONFIG7_MASK_64M,  ( 64 << 20) },
		{ GPMC_CONFIG7_MASK_32M,  ( 32 << 20) },
		{ GPMC_CONFIG7_MASK_16M,  ( 16 << 20) },
	};
	mask = ((r) & GPMC_CONFIG7_MASKADDRESS) >> 8;
	for (i=0; i < 5; i++) {
		if (gpmc_config7_size_tab[i].mask == mask)
			return gpmc_config7_size_tab[i].size;
	}
	return 0;
}

#endif	/* _OMAP2430GPMCREG_H */
