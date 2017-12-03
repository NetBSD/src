/*	$NetBSD: epoc32.cpp,v 1.1.4.3 2017/12/03 11:36:02 jdolecek Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <e32base.h>
#include <e32def.h>
#include <e32std.h>

#include "cpu.h"
#include "e32boot.h"
#include "ekern.h"
#include "epoc32.h"

#include "arm/armreg.h"
#include "arm/arm32/pte.h"


static inline void
AllowAllDomains(void)
{
	TUint domains;

#define ALL_DOMAINS(v)	\
    (((v) << 28) | \
     ((v) << 24) | \
     ((v) << 20) | \
     ((v) << 16) | \
     ((v) << 12) | \
     ((v) <<  8) | \
     ((v) <<  4) | \
     ((v) <<  0))

	domains = ALL_DOMAINS(0xf);
	__asm("mcr	p15, 0, %0, c3, c0" : : "r"(domains));
}

EPOC32::EPOC32(const EPOC32& c)
{
	cpu = c.cpu;
}

EPOC32::EPOC32(void)
{
	TUint procid;

	__asm("mrc	p15, 0, %0, c0, c0" : "=r"(procid));
	if (procid == CPU_ID_SA1100) {
		cpu = new SA1100;
	} else if ((procid & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD) {
		if (CPU_ID_IS7(procid)) {
			if ((procid & CPU_ID_7ARCH_MASK) == CPU_ID_7ARCH_V3)
				cpu = new ARM7;
			else
				cpu = new ARM7TDMI;
		}
	}
}

EPOC32::~EPOC32(void)
{
}

TAny *
EPOC32::GetPhysicalAddress(TAny *address)
{
	TUint l1Index, l1, pageOffset, pa, va;
	TUint *l1Tbl;

	AllowAllDomains();

	l1Tbl = GetTTB();

	va = (TUint)address;
	pa = pageOffset = 0;
	l1Index = (va & L1_ADDR_BITS) >> L1_S_SHIFT;
	l1 = *(l1Tbl + l1Index);
	switch (l1 & L1_TYPE_MASK) {
	case L1_TYPE_INV:
	case L1_TYPE_F:
		return NULL;

	case L1_TYPE_S:
		pa = l1 & L1_S_ADDR_MASK;
		pageOffset = va & L1_S_OFFSET;
		break;

	case L1_TYPE_C:
	{
		TUint *l2Tbl, tag;

		l2Tbl = (TUint *)(l1 & L1_C_ADDR_MASK);
		tag = MapPhysicalAddress(l2Tbl, (TAny **)&l2Tbl);
		pa = *(l2Tbl + ((va & L2_ADDR_BITS) >> 12));
		UnmapPhysicalAddress(l2Tbl,  tag);

		switch (pa & L2_TYPE_MASK) {
		case L2_TYPE_L:
			pa &= L2_L_FRAME;
			pageOffset = va & L2_L_OFFSET;
			break;

		case L2_TYPE_S:
			pa &= L2_S_FRAME;
			pageOffset = va & L2_S_OFFSET;
			break;

		default:
			pageOffset = 0xffffffff;	/* XXXX */
		}
	}
	}
	return (TAny *)(pa | pageOffset);
}

TUint
EPOC32::MapPhysicalAddress(TAny *pa, TAny **vap)
{
	TUint *l1Tbl, l1Index, l1, tag;

	AllowAllDomains();

	l1Tbl = GetTTB();

	l1Index = ((TUint)pa & L1_ADDR_BITS) >> L1_S_SHIFT;
	l1 = ((TUint)pa & L1_S_ADDR_MASK) |
					L1_S_AP(AP_KRW) | L1_S_IMP | L1_TYPE_S;
	tag = *(l1Tbl + l1Index);
	*(l1Tbl + l1Index) = l1;
	cpu->cacheFlush();
	cpu->tlbFlush();
	*vap = pa;

	return tag;
}

void
EPOC32::UnmapPhysicalAddress(TAny *address, TUint tag)
{
	TUint *l1Tbl, l1Index, pa;

	AllowAllDomains();

	l1Tbl = GetTTB();

	pa = (TUint)address;
	l1Index = (pa & L1_ADDR_BITS) >> L1_S_SHIFT;
	*(l1Tbl + l1Index) = tag;
	cpu->cacheFlush();
	cpu->tlbFlush();
}
