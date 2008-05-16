/*	$NetBSD: pci.c,v 1.1.12.1 2008/05/16 02:22:09 yamt Exp $	*/

/*-
 * Copyright (c) 2008 Izumi Tsutsui.  All rights reserved.
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

#include <sys/param.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <mips/cpuregs.h>
#include <cobalt/dev/gtreg.h>

#include "boot.h"

#define GT_BASE		0x14000000

uint32_t
pcicfgread(uint32_t tag, uint32_t off)
{
	uint32_t reg;
	volatile uint32_t *pcicfg_addr, *pcicfg_data;

	pcicfg_addr = (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_PCICFG_ADDR);
	pcicfg_data = (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_PCICFG_DATA);

	*pcicfg_addr = PCICFG_ENABLE | tag | (off & ~3U);
	reg = *pcicfg_data;
	*pcicfg_addr = 0;

	return reg;
}
