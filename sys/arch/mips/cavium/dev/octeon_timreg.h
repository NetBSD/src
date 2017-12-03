/*	$NetBSD: octeon_timreg.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
 * Timer Registers
 */

#ifndef _OCTEON_TIMREG_H_
#define _OCTEON_TIMREG_H_

/* ---- register addresses */

#define	TIM_REG_FLAGS				0x0001180058000000ULL
#define	TIM_REG_READ_IDX			0x0001180058000008ULL
#define	TIM_REG_BIST_RESULT			0x0001180058000000ULL
#define	TIM_REG_ERROR				0x0001180058000088ULL
#define	TIM_REG_INT_MASK			0x0001180058000090ULL
#define	TIM_MEM_RING0				0x0001180058001000ULL
#define	TIM_MEM_RING1				0x0001180058001008ULL
#define	TIM_MEM_DEBUG0				0x0001180058001100ULL
#define	TIM_MEM_DEBUG1				0x0001180058001108ULL
#define	TIM_MEM_DEBUG2				0x0001180058001110ULL

/* ---- register bits */

#define	TIM_REG_FLAGS_XXX_63_3			UINT64_C(0xfffffffffffffff8)
#define	TIM_REG_FLAGS_RESET			UINT64_C(0x0000000000000004)
#define	TIM_REG_FLAGS_ENA_DWB			UINT64_C(0x0000000000000002)
#define	TIM_REG_FLAGS_ENA_TIM			UINT64_C(0x0000000000000001)

#define	TIM_REG_READ_IDX_XXX_63_16		UINT64_C(0xffffffffffff0000)
#define	TIM_REG_READ_IDX_INC			UINT64_C(0x000000000000ff00)
#define	TIM_REG_READ_IDX_IDX			UINT64_C(0x00000000000000ff)

#define	TIM_REG_BIST_RESULT_XXX_63_4		UINT64_C(0xfffffffffffffff0)
#define	TIM_REG_BIST_RESULT_STA			UINT64_C(0x000000000000000c)
#define	TIM_REG_BIST_RESULT_NCB			UINT64_C(0x0000000000000002)
#define	TIM_REG_BIST_RESULT_CTL			UINT64_C(0x0000000000000001)

#define	TIM_REG_ERROR_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	TIM_REG_ERROR_MASK			UINT64_C(0x000000000000ffff)

#define	TIM_REG_INT_XXX_63_16			UINT64_C(0xffffffffffff0000)
#define	TIM_REG_INT_MASK			UINT64_C(0x000000000000ffff)

#define	TIM_MEM_RING0_XXX_63_55			UINT64_C(0xff80000000000000)
#define	TIM_MEM_RING0_BASE			UINT64_C(0x007fffffff000000)
#define	TIM_MEM_RING0_BSIZE			UINT64_C(0x0000000000fffff0)
#define	TIM_MEM_RING0_RID			UINT64_C(0x000000000000000f)

#define	TIM_MEM_RING1_XXX_63_43			UINT64_C(0xfffff80000000000)
#define	TIM_MEM_RING1_ENA			UINT64_C(0x0000040000000000)
#define	TIM_MEM_RING1_CPOOL			UINT64_C(0x0000038000000000)
#define	TIM_MEM_RING1_CSIZE			UINT64_C(0x0000007ffc000000)
#define	TIM_MEM_RING1_INTERVAL			UINT64_C(0x0000000003fffff0)
#define	TIM_MEM_RING1_RID			UINT64_C(0x000000000000000f)

#define	TIM_MEM_DEBUG0_XXX_63_48		UINT64_C(0xffff000000000000)
#define	TIM_MEM_DEBUG0_ENA			UINT64_C(0x0000800000000000)
#define	TIM_MEM_DEBUG0_XXX_46			UINT64_C(0x0000400000000000)
#define	TIM_MEM_DEBUG0_COUNT			UINT64_C(0x00003fffff000000)
#define	TIM_MEM_DEBUG0_XXX_23_22		UINT64_C(0x0000000000c00000)
#define	TIM_MEM_DEBUG0_INTERVAL			UINT64_C(0x00000000003fffff)

#define	TIM_MEM_DEBUG1_BUCKET			UINT64_C(0xfff8000000000000)
#define	TIM_MEM_DEBUG1_BASE			UINT64_C(0x0007fffffff00000)
#define	TIM_MEM_DEBUG1_BSIZE			UINT64_C(0x00000000000fffff)

#define	TIM_MEM_DEBUG2_XXX_63_32		UINT64_C(0xffffffff00000000)
#define	TIM_MEM_DEBUG2_XXX_31_24		UINT64_C(0x00000000ff000000)
#define	TIM_MEM_DEBUG2_CPOOL			UINT64_C(0x0000000000e00000)
#define	TIM_MEM_DEBUG2_CSIZE			UINT64_C(0x00000000001fff00)
#define	TIM_MEM_DEBUG2_XXX_7			UINT64_C(0x0000000000000080)
#define	TIM_MEM_DEBUG2_BUCKET			UINT64_C(0x000000000000007f)

/*
 * Timer Bucket Entry Structure
 */

struct tim_tbe {
	int64_t		tim_tbe_first;		/* first chunk (pointer) */
	uint32_t	tim_tbe_nentries;
	uint32_t	tim_tbe_remainder;	/* chunk remainder */
	int64_t		tim_tbe_current;	/* current chunk (pointer) */
	uint64_t	tim_tbe_pad;
} __packed;

/* ---- snprintb */

#define	TIM_REG_FLAGS_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x02"		"RESET\0" \
	"b\x01"		"ENA_DWB\0" \
	"b\x00"		"ENA_TIM\0"

#define	TIM_REG_READ_IDX_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x08\x08"	"INC\0" \
	"f\x00\x08"	"IDX\0"

#define	TIM_REG_BIST_RESULT_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x02\x02"	"STA\0" \
	"b\x01"		"NCB\0" \
	"b\x00"		"CTL\0"

#define	TIM_REG_ERROR_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"MASK\0"

#define	TIM_REG_INT_MASK_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x00\x10"	"TIM_REG_INT_MASK\0"

#define	TIM_MEM_RING0_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x18\x1f"	"BASE\0" \
	"f\x04\x14"	"BSIZE\0" \
	"f\x00\x04"	"RID\0"

#define	TIM_MEM_RING1_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x2a"		"ENA\0" \
	"f\x27\x03"	"CPOOL\0" \
	"f\x1a\x0d"	"CSIZE\0" \
	"f\x04\x16"	"INTERVAL\0" \
	"f\x00\x04"	"RID\0"

#define	TIM_MEM_DEBUG0_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"b\x2f"		"ENA\0" \
	"f\x18\x16"	"COUNT\0" \
	"f\x00\x16"	"INTERVAL\0"

#define	TIM_MEM_DEBUG1_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x33\x0d"	"BUCKET\0" \
	"f\x14\x1f"	"BASE\0" \
	"f\x00\x14"	"BSIZE\0"

#define	TIM_MEM_DEBUG2_BITS \
	"\177"		/* new format */ \
	"\020"		/* hex display */ \
	"\020"		/* %016x format */ \
	"f\x15\x03"	"CPOOL\0" \
	"f\x08\x0d"	"CSIZE\0" \
	"f\x00\x07"	"BUCKET\0"

#endif /* _OCTEON_TIMREG_H_ */
