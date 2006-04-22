/*       $NetBSD: bootinfo.h,v 1.2.4.1 2006/04/22 11:38:01 simonb Exp $        */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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

#ifndef _BOOTINFO_H_
#define _BOOTINFO_H_

#include <machine/param.h>
#include <sparc/bootinfo.h>

/*
 * The bootloader v1.9 and higher makes a call into a kernel with the following
 * arguments:
 *
 * %o0		OpenFirmware entry point, to keep Sun's updaters happy
 * %o1		Address of boot information vector (see below)
 * %o2		Length of the vector, in bytes
 * %o3		OpenFirmware entry point, to mimic Sun bootloader behavior
 * %o4		OpenFirmware entry point, to meet earlier NetBSD kernels
 *              expectations
 *
 * All parameters are of void* type except for vector length which is of
 * integer type.
 *
 * The boot information vector format was dictated by earlier kernels and
 * starting from ofwboot v1.9 has four entries:
 *
 *      +-------------------------------------+
 * #0   | NetBSD boot magic number ('DDB2')   |
 *      +-------------------------------------+
 * #1   | ksyms' table end address            |
 *      +-------------------------------------+
 * #2   | ksyms' table start address          |
 *      +-------------------------------------+
 * #3   | Pointer to a bootinfo binary object |
 *      +-------------------------------------+
 *
 * Kernels written for pre-v1.8 ofwboot require first three cells to present
 * in this particular order, those that rely on v1.9 and higher won't work if
 * the last pointer is not in the table; therefore, the vector cannot be
 * shorter than 4 * sizeof(uint32_t) or 4 * sizeof(uint64_t) for 32-bit and
 * 64-bit boot loader, respectively.
 *
 * As of ofwboot v1.9,  bootinfo binary object is placed right after the kernel
 * data segment end, i.e. in the permanently locked 4MB page. There is no order
 * for the data located in bootinfo binary object. The kernel however expects
 * at least following parameters to exist in there:
 *
 * - Symbol information (size, start, end; see struct btinfo_symtab)
 * - Kernel end address, calculated as true kernel end address plus size of
 *   space reserved for bootinfo binary object (struct btinfo_kernend)
 * - Number of text segment pages (struct btinfo_count)
 * - Number of data segment pages (struct btinfo_count)
 * - Array of text pages VA/PA (struct btinfo_tlb)
 * - Array of data pages VA/PA (struct btinfo_tlb)
 *
 * The last four data structures fully describe kernel mappings. ofwboot v1.9
 * and higher map the kernel with 4MB permanent pages and this is the only 
 * way to let kernel know how exactly it was mapped.
 */

/* Boot magic number */
#define SPARC_MACHINE_OPENFIRMWARE	0x44444230

#ifdef BOOTINFO_SIZE
#undef BOOTINFO_SIZE
#endif

#define BOOTINFO_SIZE			NBPG

#define BTINFO_DTLB_SLOTS		100
#define BTINFO_ITLB_SLOTS		101
#define BTINFO_DTLB			102
#define BTINFO_ITLB			103
#define BTINFO_KERNEND			104

#define LOOKUP_BOOTINFO(btp, info) \
do { \
	if ( ((btp) = lookup_bootinfo(info)) == NULL) { \
		panic("lookup_bootinfo: No " #info " found.\n"); \
	} \
} while (0)

struct tlb_entry {
	uint64_t te_pa;
	uint64_t te_va;
};

struct btinfo_count {
	struct btinfo_common common;
	int count;
};

struct btinfo_tlb {
	struct btinfo_common common;
	struct tlb_entry tlb[1];
};

struct btinfo_kernend {
	struct btinfo_common common;
	uint64_t addr;
};

#endif /* _BOOTINFO_H_ */
