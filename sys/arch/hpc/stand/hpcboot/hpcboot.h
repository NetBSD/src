/*	$NetBSD: hpcboot.h,v 1.1.10.1 2002/02/28 04:09:45 nathanw Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _HPCBOOT_H_
#define _HPCBOOT_H_

#include <hpcdefs.h>
#include <res/resource.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/platid_generated.h>

enum { KERNEL_PAGE_SIZE = 0x1000 };

enum ArchitectureOps {
#ifdef ARM
	ARCHITECTURE_ARM_SA1100	= PLATID_CPU_ARM_STRONGARM_SA1100,
#endif
#ifdef SHx
	ARCHITECTURE_SH3_7709	= PLATID_CPU_SH_3_7709,
	ARCHITECTURE_SH3_7709A	= PLATID_CPU_SH_3_7709A,
	ARCHITECTURE_SH4_7750	= PLATID_CPU_SH_4_7750,
#endif
#ifdef MIPS
	ARCHITECTURE_MIPS_TX3900= PLATID_CPU_MIPS_TX_3900,
	ARCHITECTURE_MIPS_TX3920= PLATID_CPU_MIPS_TX_3920,
	ARCHITECTURE_MIPS_VR41	= PLATID_CPU_MIPS_VR_41XX
#endif
};

enum ConsoleOps {
	CONSOLE_LCD,
	CONSOLE_SERIAL
};

enum MemoryManagerOps {
	MEMORY_MANAGER_VIRTUALCOPY,
	MEMORY_MANAGER_LOCKPAGES,
	MEMORY_MANAGER_SOFTMMU,
	MEMORY_MANAGER_HARDMMU
};

enum FileOps {
	FILE_FAT,
	FILE_UFS,
	FILE_HTTP
};

enum LoaderOps {
	LOADER_UNKNOWN,
	LOADER_ELF,
	LOADER_COFF,
	LOADER_AOUT
};

struct BootSetupArgs {
	enum ArchitectureOps architecture;
	BOOL architectureDebug;
	enum ConsoleOps console;
	BOOL consoleEnable;
	enum MemoryManagerOps memory;
	BOOL memorymanagerDebug;
	enum FileOps file;
	BOOL fileDebug;
	TCHAR fileRoot[MAX_PATH];
	TCHAR fileName[MAX_PATH];
	BOOL loadmfs;
	TCHAR mfsName[MAX_PATH];
	enum LoaderOps loader;
	BOOL loaderDebug;
};

struct PageTag {
	u_int32_t next;	/* next tagged page kernel virtual address; */
	u_int32_t src;	/* kernel virtual or physical address */
	u_int32_t dst;	/* kernel virtual or physical address */
	u_int32_t sz;	/* copy size or zero-clear size; */
};

struct BootArgs {
	kaddr_t kernel_entry;
	kaddr_t argc;
	kaddr_t argv;
	kaddr_t bootinfo;
	struct bootinfo bi;
};

#define VOLATILE_REF(x)			(*(volatile u_int32_t *)(x))
#define VOLATILE_REF16(x)		(*(volatile u_int16_t *)(x))
#define VOLATILE_REF8(x)		(*(volatile u_int8_t *)(x))
#define	_reg_read_1(a)		(*(volatile u_int8_t *)(a))
#define	_reg_read_2(a)		(*(volatile u_int16_t *)(a))
#define	_reg_read_4(a)		(*(volatile u_int32_t *)(a))
#define	_reg_write_1(a, v)	(*(volatile u_int8_t *)(a) = (v))
#define	_reg_write_2(a, v)	(*(volatile u_int16_t *)(a) = (v))
#define	_reg_write_4(a, v)	(*(volatile u_int32_t *)(a) = (v))

#ifdef ARM
#define ptokv(x)	(x)			/* UNCACHED FLAT */
#elif defined SHx
#define ptokv(x)	((x) | 0x80000000)	/* CACHED P1 */
#elif defined MIPS
#define ptokv(x)	((x) | 0x80000000)	/* CACHED kseg0 */
#else
#error "physical address to kernel virtual macro not defined."
#endif

__BEGIN_DECLS
/* Windows CE API */
BOOL VirtualCopy(LPVOID, LPVOID, DWORD, DWORD);
BOOL SetKMode(BOOL);
BOOL LockPages(LPVOID, DWORD, PDWORD, int);
BOOL UnlockPages(LPVOID, DWORD);
void CacheSync(int);
#define CACHE_D_WBINV	1
#define CACHE_I_INV	2
/* ExtEscape */
#define GETVFRAMEPHYSICAL	6144
#define GETVFRAMELEN		6145

/* debug utility */
void _bitdisp(u_int32_t, int, int, int, int);
void _dbg_bit_print(u_int32_t, u_int32_t, const char *);
#define bitdisp(a) _bitdisp((a), 0, 0, 0, 1)
__END_DECLS

#endif /* _HPCBOOT_H_ */
