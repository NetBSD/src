/*	$NetBSD: bootinfo.h,v 1.5 2024/01/08 05:09:41 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _VIRT68K_BOOTINFO_H_
#define	_VIRT68K_BOOTINFO_H_

/*
 * Linux/m68k boot information structures.  These records are used to
 * pass information from the Linux boot loader (or Qemu) to the loaded
 * kernel.
 *
 * Each record has a header with the type tag and record size, followed
 * by a data field.  Each record + data is padded to align the record
 * header at a 4-byte boundary.
 */

struct bi_record {
	uint16_t	bi_tag;		/* record type */
	uint16_t	bi_size;	/* record size (including header) */
	uint8_t		bi_data[];
};

/*
 * Record types.  Record types >= 0x8000 are machine-dependent.
 */
#define	BI_MACHDEP(x)	((x) + 0x8000)
#define	BI_LAST		0	/* list terminator */
#define	BI_MACHTYPE	1	/* machine type (uint32_t) */
#define	BI_CPUTYPE	2	/* CPU type (uint32_t) */
#define	BI_FPUTYPE	3	/* FPU type (uint32_t) */
#define	BI_MMUTYPE	4	/* MMU type (uint32_t) */
#define	BI_MEMCHUNK	5	/* memory segment (bi_mem_info) */
#define	BI_RAMDISK	6	/* RAM disk (bi_mem_info) */
#define	BI_COMMAND_LINE	7	/* kernel command line parameters (C string) */
#define	BI_RNG_SEED	8	/* random number generator seed (bi_data) */

#define	BI_VIRT_QEMU_VERSION	BI_MACHDEP(0)	/* uint32_t */
#define	BI_VIRT_GF_PIC_BASE	BI_MACHDEP(1)	/* bi_virt_dev */
#define	BI_VIRT_GF_RTC_BASE	BI_MACHDEP(2)
#define	BI_VIRT_GF_TTY_BASE	BI_MACHDEP(3)
#define	BI_VIRT_VIRTIO_BASE	BI_MACHDEP(4)
#define	BI_VIRT_CTRL_BASE	BI_MACHDEP(5)

struct bi_mem_info {
	uint32_t	mem_addr;	/* PA of memory segment */
	uint32_t	mem_size;	/* size in bytes */
};

struct bi_data {
	uint16_t	data_length;	/* length of data */
	uint8_t		data_bytes[];
};

struct bi_virt_dev {
	uint32_t	vd_mmio_base;
	uint32_t	vd_irq_base;
};

/*
 * Values for BI_MACHTYPE.
 */
#define	BI_MACH_AMIGA		1
#define	BI_MACH_ATARI		2
#define	BI_MACH_MAC		3
#define	BI_MACH_APOLLO		4
#define	BI_MACH_SUN3		5
#define	BI_MACH_MVME147		6
#define	BI_MACH_MVME16x		7
#define	BI_MACH_BVME6000	8
#define	BI_MACH_HP300		9
#define	BI_MACH_Q40		10
#define	BI_MACH_SUN3X		11
#define	BI_MACH_M54XX		12
#define	BI_MACH_M5441X		13
#define	BI_MACH_VIRT		14

/*
 * Values for BI_CPUTYPE.
 */
#define	BI_CPU_68020		__BIT(0)
#define	BI_CPU_68030		__BIT(1)
#define	BI_CPU_68040		__BIT(2)
#define	BI_CPU_68060		__BIT(3)
#define	BI_CPU_COLDFIRE		__BIT(4)

/*
 * Values for BI_FPUTYPE.
 */
#define	BI_FPU_68881		__BIT(0)
#define	BI_FPU_68882		__BIT(1)
#define	BI_FPU_68040		__BIT(2)
#define	BI_FPU_68060		__BIT(3)
#define	BI_FPU_SUNFPA		__BIT(4)
#define	BI_FPU_COLDFIRE		__BIT(5)

/*
 * Values for BI_MMUTYPE.
 */
#define	BI_MMU_68851		__BIT(0)
#define	BI_MMU_68030		__BIT(1)
#define	BI_MMU_68040		__BIT(2)
#define	BI_MMU_68060		__BIT(3)
#define	BI_MMU_APOLLO		__BIT(4)
#define	BI_MMU_SUN3		__BIT(5)
#define	BI_MMU_COLDFIRE		__BIT(6)

#ifdef _KERNEL

#include <sys/bus.h>

extern uint32_t	bootinfo_machtype;
extern int	bootinfo_mem_segments_ignored;
extern size_t	bootinfo_mem_segments_ignored_bytes;
extern struct bi_mem_info bootinfo_mem_segments[];
extern struct bi_mem_info bootinfo_mem_segments_avail[];
extern int	bootinfo_mem_nsegments;
extern int	bootinfo_mem_nsegments_avail;
extern vaddr_t	bootinfo_end;

#define	bootinfo_dataptr(bi)	((void *)&(bi)->bi_data[0])
#define	bootinfo_get_u32(bi)	(*(uint32_t *)bootinfo_dataptr(bi))

void			bootinfo_start(struct bi_record *);
struct bi_record *	bootinfo_find(uint32_t tag);
void			bootinfo_enumerate(bool (*)(struct bi_record *, void *),
					   void *);
bool			bootinfo_addr_is_console(paddr_t);

void			bootinfo_setup_initrd(void);
void			bootinfo_setup_rndseed(void);
bool			bootinfo_getarg(const char *, char *, size_t);

void			bootinfo_md_cnattach(void (*)(bus_space_tag_t,
						      bus_space_handle_t),
					     paddr_t, psize_t);

#endif /* _KERNEL */

#endif /* _VIRT68K_BOOTINFO_H_ */
