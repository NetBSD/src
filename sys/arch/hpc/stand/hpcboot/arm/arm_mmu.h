/* -*-C++-*-	$NetBSD: arm_mmu.h,v 1.3.78.1 2008/05/16 02:22:24 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _HPCBOOT_ARM_MMU_H_
#define	_HPCBOOT_ARM_MMU_H_

#include <arm/arm_arch.h>
#include <memory.h>

#define	ARM_MMU_TABLEBASE_MASK				0xffffc000
#define	ARM_MMU_TABLEINDEX_MASK				0x00003ffc
#define	ARM_MMU_VADDR_TABLE_INDEX_MASK			0xfff00000
#define	ARM_MMU_TABLEINDEX_SHIFT			18
#define	ARM_MMU_TABLEINDEX(x)						\
	((((x) & ARM_MMU_VADDR_TABLE_INDEX_MASK) >>			\
	 ARM_MMU_TABLEINDEX_SHIFT) & ARM_MMU_TABLEINDEX_MASK)

/*
 * 1st level descriptor
 */
#define	ARM_MMU_LEVEL1DESC_TRANSLATE_TYPE_MASK		0x3
#define	ARM_MMU_LEVEL1DESC_TRANSLATE_TYPE(x)				\
	((x) & ARM_MMU_LEVEL1DESC_TRANSLATE_TYPE_MASK)
#define	ARM_MMU_LEVEL1DESC_TRANSLATE_SECTION		0x2
#define	ARM_MMU_LEVEL1DESC_TRANSLATE_PAGE		0x1

/*
 * Section translation
 */
#define	ARM_MMU_SECTION_BASE_MASK			0xfff00000
#define	ARM_MMU_SECTION_BASE(x)						\
	((x) & ARM_MMU_SECTION_BASE_MASK)
#define	ARM_MMU_VADDR_SECTION_INDEX_MASK		0x000fffff
#define	ARM_MMU_VADDR_SECTION_INDEX(x)					\
	((x) & ARM_MMU_VADDR_SECTION_INDEX_MASK)
/*
 * Page translation
 */
#define	ARM_MMU_PTE_BASE_MASK				0xfffffc00
#define	ARM_MMU_PTE_BASE(x)	((x) & ARM_MMU_PTE_BASE_MASK)
#define	ARM_MMU_VADDR_PTE_INDEX_MASK			0x000003fc
#define	ARM_MMU_VADDR_PTE_INDEX_SHIFT			10
#define	ARM_MMU_VADDR_PTE_INDEX(x)					\
	(((x) >> ARM_MMU_VADDR_PTE_INDEX_SHIFT) &			\
	ARM_MMU_VADDR_PTE_INDEX_MASK)

class MemoryManager_ArmMMU : public MemoryManager {
private:
	BOOL _kmode;
	paddr_t _table_base;

public:
	MemoryManager_ArmMMU(Console *&, size_t);
	virtual ~MemoryManager_ArmMMU();
	BOOL init(void);
	paddr_t searchPage(vaddr_t vaddr);
};

#endif // _HPCBOOT_ARM_MMU_H_
