/*	$NetBSD: cop0.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "cmd.h"
#include "local.h"

#define	NTLB	48	/* R4000 */
struct tlb {
	uint32_t PageMask;
	uint32_t EntryHi;
	uint32_t EntryLo0;
	uint32_t EntryLo1;
	uint32_t page_mask;
	char *page_size;
	uint32_t vpn, pfn0, pfn1;
} entry[NTLB];
static void __tlb_pagemask(struct tlb *);


/*
EWS4800/350. IPL don't handle TLB refill exception.

16M-page
paddr  0x00000000-0x18000000	M0, M1, M2
vaddr  0x00000000-0x18000000

paddr  0x38000000-0x40000000	M3
vaddr  0x18000000-0x20000000

paddr  0xe0000000-0xf8000000	FrameBuffer 0xf0000000 + reg 0xf5000000
vaddr  0xe0000000-0xf8000000

4K-page
paddr  0x20000000-0x20028000	160K
vaddr  0x20000000-0x20028000
 */
int
cmd_tlb(int argc, char *argp[], int interactive)
{
	extern void tlb_read(int, void *);
	struct tlb  *e;
	int i;

	if (argc < 2) {
		printf("option: r, d, p entry#\n");
		return 1;
	}

	switch (*argp[1]) {
	case 'r':	/* Read TLB entry all. */
		for (i = 0; i < NTLB; i++)
			tlb_read(i, &entry[i]);
		printf("done.\n");
		break;
	case 'd':	/* Dump TLB summary */
		for (i = 0, e = entry; i < NTLB; i++, e++) {
			__tlb_pagemask(e);
			e->vpn = (e->EntryHi >> 13) << 1;
			e->pfn0 = (e->EntryLo0 >> 6);
			e->pfn1 = (e->EntryLo1 >> 6);
			printf("%d %s %x %x+%x", i, e->page_size, e->vpn,
			    e->pfn0, e->pfn1);
			putchar((i + 1) & 3 ? '|' : '\n');
		}
		break;
	case 'p':	/* Print TLB entry */
		if (argc < 3) {
			printf("tlb p entry#.\n");
			return 1;
		}
		i = strtoul(argp[2], 0, 0);
		if (i < 0 || i >= NTLB)
			return 1;
		e = &entry[i];
		printf("[%d] size:%s vaddr:%p paddr:%p+%p mask:%p\n",
		    i, e->page_size, e->vpn << 12,
		    e->pfn0 << 12, e->pfn1 << 12, e->page_mask);
		printf("[%x, %x, %x, %x]\n",
		    e->PageMask, e->EntryHi, e->EntryLo0, e->EntryLo1);

		break;
	default:
		printf("unknown option \"%c\"\n", *argp[1]);
		break;
	}

	return 0;
}

int
cmd_cop0(int argc, char *argp[], int interactive)
{
	int v;

	__asm volatile("mfc0 %0, $%1" : "=r"(v) : "i"(15));
	printf("PRId: %x\n", v);
	__asm volatile("mfc0 %0, $%1" : "=r"(v) : "i"(16));
	printf("Config: %x\n", v);
	__asm volatile("mfc0 %0, $%1" : "=r"(v) : "i"(12));
	printf("Status: %x\n", v);
	__asm volatile("mfc0 %0, $%1" : "=r"(v) : "i"(13));
	printf("Cause: %x\n", v);
	__asm volatile("mfc0 %0, $%1" : "=r"(v) : "i"(8));
	printf("BadVAddr: %x\n", v);
	__asm volatile("mfc0 %0, $%1" : "=r"(v) : "i"(14));
	printf("EPC: %x\n", v);

	return 0;
}

void
__tlb_pagemask(struct tlb *e)
{

	switch (e->PageMask >> 13) {
	default:
		e->page_size = " ERR";
		e->page_mask = 0;
		break;
	case 0:
		e->page_size = "  4K";
		e->page_mask = 0xfff;
		break;
	case 3:
		e->page_size = " 16K";
		e->page_mask = 0x3fff;
		break;
	case 0xf:
		e->page_size = " 64K";
		e->page_mask = 0xffff;
		break;
	case 0x3f:
		e->page_size = "256K";
		e->page_mask = 0x3ffff;
		break;
	case 0xff:
		e->page_size = "  1M";
		e->page_mask = 0xfffff;
		break;
	case 0x3ff:
		e->page_size = "  4M";
		e->page_mask = 0x3fffff;
		break;
	case 0xfff:
		e->page_size = " 16M";
		e->page_mask = 0xffffff;
		break;
	}
}
