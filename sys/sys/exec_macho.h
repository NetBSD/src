/*	$NetBSD: exec_macho.h,v 1.2.2.2 2001/08/03 04:14:05 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _SYS_EXEC_MACHO_H_
#define	_SYS_EXEC_MACHO_H_

#include <machine/int_types.h>
#include <machine/macho_machdep.h>

/*
 * the `fat' binary description
 */
struct exec_macho_fat_header {
	__uint32_t	magic;
	__uint32_t	nfat_arch;
};

#define MACHO_FAT_MAGIC	0xcafebabe

/*
 * The fat architecture header
 */
struct exec_macho_fat_arch {
	__uint32_t	cputype;
	__uint32_t	cpusubtype;
	u_long		offset;
	u_long		size;
	u_long		align;
};

#define	MACHO_CPU_TYPE_ANY	~0
#define	MACHO_CPU_TYPE_VAX	 1
#define	MACHO_CPU_TYPE_MC680x0	 6
#define	MACHO_CPU_TYPE_I386	 7
#define	MACHO_CPU_TYPE_MIPS	 8
#define	MACHO_CPU_TYPE_MC98000	10
#define	MACHO_CPU_TYPE_HPPA	11
#define	MACHO_CPU_TYPE_ARM	12
#define	MACHO_CPU_TYPE_MC88000	13
#define	MACHO_CPU_TYPE_SPARC	14
#define	MACHO_CPU_TYPE_I860	15
#define	MACHO_CPU_TYPE_ALPHA	16
#define	MACHO_CPU_TYPE_POWERPC	18

#define	MACHO_CPU_SUBTYPE_MULTIPLE		~0
#define	MACHO_CPU_SUBTYPE_LITTLE_ENDIAN		 0
#define	MACHO_CPU_SUBTYPE_BIG_ENDIAN		 1

/*
 * m68k
 */
#define	MACHO_CPU_SUBTYPE_MC680x0_ALL		1
#define	MACHO_CPU_SUBTYPE_MC68030		1
#define	MACHO_CPU_SUBTYPE_MC68040		2 
#define	MACHO_CPU_SUBTYPE_MC68030_ONLY		3

/*
 * x86
 */
#define	MACHO_CPU_SUBTYPE_I386_ALL	3
#define	MACHO_CPU_SUBTYPE_386		3
#define	MACHO_CPU_SUBTYPE_486		4
#define	MACHO_CPU_SUBTYPE_486SX		(4 + 128)
#define	MACHO_CPU_SUBTYPE_586		5
#define	MACHO_CPU_SUBTYPE_INTEL(f, m)	(f) + ((m) << 4))
#define	MACHO_CPU_SUBTYPE_PENT		CPU_SUBTYPE_INTEL(5, 0)
#define	MACHO_CPU_SUBTYPE_PENTPRO	CPU_SUBTYPE_INTEL(6, 1)
#define	MACHO_CPU_SUBTYPE_PENTII_M3	CPU_SUBTYPE_INTEL(6, 3)
#define	MACHO_CPU_SUBTYPE_PENTII_M5	CPU_SUBTYPE_INTEL(6, 5)
#define	MACHO_CPU_SUBTYPE_INTEL_FAMILY(x)	((x) & 15)
#define	MACHO_CPU_SUBTYPE_INTEL_FAMILY_MAX	15
#define	MACHO_CPU_SUBTYPE_INTEL_MODEL(x)	((x) >> 4)
#define	MACHO_CPU_SUBTYPE_INTEL_MODEL_ALL	0
/*
 * PowerPC
 */
#define	MACHO_CPU_SUBTYPE_POWERPC_ALL		 0
#define	MACHO_CPU_SUBTYPE_POWERPC_601		 1
#define	MACHO_CPU_SUBTYPE_POWERPC_602		 2
#define	MACHO_CPU_SUBTYPE_POWERPC_603		 3
#define	MACHO_CPU_SUBTYPE_POWERPC_603e		 4
#define	MACHO_CPU_SUBTYPE_POWERPC_603ev		 5
#define	MACHO_CPU_SUBTYPE_POWERPC_604		 6
#define	MACHO_CPU_SUBTYPE_POWERPC_604e		 7
#define	MACHO_CPU_SUBTYPE_POWERPC_620		 8
#define	MACHO_CPU_SUBTYPE_POWERPC_750		 9
#define	MACHO_CPU_SUBTYPE_POWERPC_7400		10
#define	MACHO_CPU_SUBTYPE_POWERPC_7450		11


struct exec_macho_object_header {
	u_long	magic;
	__uint32_t	cputype;
	__uint32_t	cpusubtype;
	u_long	filetype;
	u_long	ncmds;
	u_long	sizeofcmds;
	u_long	flags;
};

#define	MACHO_MOH_MAGIC	0xfeedface

/*
 * Object header filetype 
 */
#define	MACHO_MOH_OBJECT	0x1
#define	MACHO_MOH_EXECUTE	0x2
#define	MACHO_MOH_FVMLIB	0x3
#define	MACHO_MOH_CORE		0x4
#define	MACHO_MOH_PRELOAD	0x5
#define	MACHO_MOH_DYLIB		0x6
#define	MACHO_MOH_DYLINKER	0x7
#define	MACHO_MOH_BUNDLE	0x8

/*
 * Object header flags
 */
#define	MACHO_MOH_NOUNDEFS	0x001
#define	MACHO_MOH_INCRLINK	0x002
#define	MACHO_MOH_DYLDLINK	0x004
#define	MACHO_MOH_BINDATLOAD	0x008
#define	MACHO_MOH_PREBOUND	0x010
#define	MACHO_MOH_SPLIT_SEGS	0x020
#define	MACHO_MOH_LAZY_INIT	0x040
#define	MACHO_MOH_TWOLEVEL	0x080
#define	MACHO_MOH_FORCE_FLAT	0x100

struct exec_macho_load_command {
	u_long cmd;
	u_long cmdsize;
};

#define	MACHO_LC_SEGMENT	0x01
#define	MACHO_LC_SYMTAB		0x02
#define	MACHO_LC_SYMSEG		0x03
#define	MACHO_LC_THREAD		0x04
#define	MACHO_LC_UNIXTHREAD	0x05
#define	MACHO_LC_LOADFVMLIB	0x06
#define	MACHO_LC_IDFVMLIB	0x07
#define	MACHO_LC_IDENT		0x08
#define	MACHO_LC_FVMFILE	0x09
#define	MACHO_LC_PREPAGE	0x0a 
#define	MACHO_LC_DYSYMTAB	0x0b
#define	MACHO_LC_LOAD_DYLIB	0x0c
#define	MACHO_LC_ID_DYLIB	0x0d
#define	MACHO_LC_LOAD_DYLINKER	0x0e
#define	MACHO_LC_ID_DYLINKER	0x0f
#define	MACHO_LC_PREBOUND_DYLIB	0x10
#define	MACHO_LC_ROUTINES	0x11
#define	MACHO_LC_SUB_FRAMEWORK	0x12
#define	MACHO_LC_SUB_UMBRELLA	0x13
#define	MACHO_LC_SUB_CLIENT	0x14

struct exec_macho_segment_command {
	u_long	cmd;
	u_long	cmdsize;
	char	segname[16];
	u_long	vmaddr;
	u_long	vmsize;
	u_long	fileoff;
	u_long	filesize;
	__uint32_t	maxprot;
	__uint32_t	initprot;
	u_long	nsects;
	u_long	flags;
};

union macho_lc_str {
	u_long	offset;
	char		*ptr;
};

#define	MACHO_SG_HIGHVM		0x1
#define	MACHO_SG_FVMLIB		0x2
#define	MACHO_SG_NORELOC	0x4

struct exec_macho_dylinker_command {
	u_long	cmd;
	u_long	cmdsize;
	union macho_lc_str	name;
};

struct exec_macho_dylib {
    union macho_lc_str name;
    u_long timestamp;
    u_long current_version;
    u_long compatibility_version;
};

struct exec_macho_dylib_command {
	u_long	cmd;
	u_long	cmdsize;
	struct exec_macho_dylib	dylib;
};

struct exec_macho_thread_command {
	u_long	cmd;
	u_long	cmdsize;
	u_long	flavor;
	u_long	count;
};

#ifndef _LKM
#include "opt_execfmt.h"
#endif

#ifdef _KERNEL 
struct exec_package;
struct ps_strings;
u_long	exec_macho_thread_entry(struct exec_macho_thread_command *);
int	exec_macho_makecmds __P((struct proc *, struct exec_package *));
int	exec_macho_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));
int	exec_macho_setup_stack __P((struct proc *, struct exec_package *));
#endif /* _KERNEL */

#endif /* !_SYS_EXEC_MACHO_H_ */
