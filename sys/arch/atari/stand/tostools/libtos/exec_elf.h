/*	$NetBSD: exec_elf.h,v 1.1 2001/10/10 14:19:51 leo Exp $	*/

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
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

#ifndef _SYS_EXEC_ELF_H_
#define	_SYS_EXEC_ELF_H_

/*
 * The current ELF ABI specification is available at:
 *	http://www.sco.com/developer/gabi/
 *
 * Current header definitions are in:
 *	http://www.sco.com/developer/gabi/latest/ch4.eheader.html
 */

/*
 * Leo 10/10/2001:
 *   This is a copy of the file in sys/sys, but modified a bit to
 *   be used in a TOS/MiNT environment. I will probably trim it down
 *   in the near future. It is only used for loading kernels.
 */
#ifdef TOSTOOLS
typedef	signed char		 __int8_t;
typedef	unsigned char		__uint8_t;
typedef	short int		__int16_t;
typedef	unsigned short int     __uint16_t;
typedef	int			__int32_t;
typedef	unsigned int	       __uint32_t;
typedef	long long int		__int64_t;
typedef	unsigned long long int __uint64_t;
#else
#include <machine/int_types.h>
#endif /* TOSTOOLS */

typedef	__uint8_t  	Elf_Byte;

typedef	__uint32_t	Elf32_Addr;
#define	ELF32_FSZ_ADDR	4
typedef	__uint32_t Elf32_Off;
#define	ELF32_FSZ_OFF	4
typedef	__int32_t   Elf32_Sword;
#define	ELF32_FSZ_SWORD	4
typedef	__uint32_t Elf32_Word;
#define	ELF32_FSZ_WORD	4
typedef	__uint16_t Elf32_Half;
#define	ELF32_FSZ_HALF	2

typedef	__uint64_t	Elf64_Addr;
#define	ELF64_FSZ_ADDR	8
typedef	__uint64_t	Elf64_Off;
#define	ELF64_FSZ_OFF	8
typedef	__int32_t	Elf64_Shalf;
#define	ELF64_FSZ_SHALF	4

#ifdef __alpha__
typedef	__int64_t	Elf64_Sword;
#define	ELF64_FSZ_SWORD	8
typedef	__uint64_t	Elf64_Word;
#define	ELF64_FSZ_WORD	8
#else
typedef	__int32_t	Elf64_Sword;
#define	ELF64_FSZ_SWORD	4
typedef	__uint32_t	Elf64_Word;
#define	ELF64_FSZ_WORD	4
#endif /* __alpha__ */

typedef	__int64_t	Elf64_Sxword;
#define	ELF64_FSZ_XWORD	8
typedef	__uint64_t	Elf64_Xword;
#define	ELF64_FSZ_XWORD	8
typedef	__uint32_t	Elf64_Half;
#define	ELF64_FSZ_HALF	4
typedef	__uint16_t	Elf64_Quarter;
#define	ELF64_FSZ_QUARTER 2

/*
 * ELF Header
 */
#define	ELF_NIDENT	16

typedef struct {
	unsigned char	e_ident[ELF_NIDENT];	/* Id bytes */
	Elf32_Half	e_type;			/* file type */
	Elf32_Half	e_machine;		/* machine type */
	Elf32_Word	e_version;		/* version number */
	Elf32_Addr	e_entry;		/* entry point */
	Elf32_Off	e_phoff;		/* Program hdr offset */
	Elf32_Off	e_shoff;		/* Section hdr offset */
	Elf32_Word	e_flags;		/* Processor flags */
	Elf32_Half      e_ehsize;		/* sizeof ehdr */
	Elf32_Half      e_phentsize;		/* Program header entry size */
	Elf32_Half      e_phnum;		/* Number of program headers */
	Elf32_Half      e_shentsize;		/* Section header entry size */
	Elf32_Half      e_shnum;		/* Number of section headers */
	Elf32_Half      e_shstrndx;		/* String table index */
} Elf32_Ehdr;

/* e_ident offsets */
#define	EI_MAG0		0	/* '\177' */
#define	EI_MAG1		1	/* 'E'    */
#define	EI_MAG2		2	/* 'L'    */
#define	EI_MAG3		3	/* 'F'    */
#define	EI_CLASS	4	/* File class */
#define	EI_DATA		5	/* Data encoding */
#define	EI_VERSION	6	/* File version */
#define	EI_OSABI	7	/* Operating system/ABI identification */
#define	EI_ABIVERSION	8	/* ABI version */
#define	EI_PAD		9	/* Start of padding bytes up to EI_NIDENT*/

/* e_ident[ELFMAG0,ELFMAG3] */
#define	ELFMAG0		0x7f
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

/* e_ident[EI_CLASS] */
#define	ELFCLASSNONE	0	/* Invalid class */
#define	ELFCLASS32	1	/* 32-bit objects */
#define	ELFCLASS64	2	/* 64-bit objects */
#define	ELFCLASSNUM	3

/* e_ident[EI_DATA] */
#define	ELFDATANONE	0	/* Invalid data encoding */
#define	ELFDATA2LSB	1	/* 2's complement values, LSB first */
#define	ELFDATA2MSB	2	/* 2's complement values, MSB first */

/* e_ident[EI_VERSION] */
#define	EV_NONE		0	/* Invalid version */
#define	EV_CURRENT	1	/* Current version */
#define	EV_NUM		2

/* e_ident[EI_OSABI] */
#define	ELFOSABI_SYSV		0	/* UNIX System V ABI */
#define	ELFOSABI_HPUX		1	/* HP-UX operating system */
#define ELFOSABI_NETBSD		2	/* NetBSD */
#define ELFOSABI_LINUX		3	/* GNU/Linux */
#define ELFOSABI_HURD		4	/* GNU/Hurd */
#define ELFOSABI_86OPEN		5	/* 86Open */
#define ELFOSABI_SOLARIS	6	/* Solaris */
#define ELFOSABI_MONTEREY	7	/* Monterey */
#define ELFOSABI_IRIX		8	/* IRIX */
#define ELFOSABI_FREEBSD	9	/* FreeBSD */
#define ELFOSABI_TRU64		10	/* TRU64 UNIX */
#define ELFOSABI_MODESTO	11	/* Novell Modesto */
#define ELFOSABI_OPENBSD	12	/* OpenBSD */
/* Unofficial OSABIs follow */
#define ELFOSABI_ARM		97	/* ARM */
#define	ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

/* e_type */
#define	ET_NONE		0	/* No file type */
#define	ET_REL		1	/* Relocatable file */
#define	ET_EXEC		2	/* Executable file */
#define	ET_DYN		3	/* Shared object file */
#define	ET_CORE		4	/* Core file */
#define	ET_NUM		5

#define	ET_LOOS		0xfe00	/* Operating system specific range */
#define	ET_HIOS		0xfeff
#define	ET_LOPROC	0xff00	/* Processor-specific range */
#define	ET_HIPROC	0xffff

/* e_machine */
#define	EM_NONE		0	/* No machine */
#define	EM_M32		1	/* AT&T WE 32100 */
#define	EM_SPARC	2	/* SPARC */
#define	EM_386		3	/* Intel 80386 */
#define	EM_68K		4	/* Motorola 68000 */
#define	EM_88K		5	/* Motorola 88000 */
#define	EM_486		6	/* Intel 80486 */
#define	EM_860		7	/* Intel 80860 */
#define	EM_MIPS		8	/* MIPS I Architecture */
#define	EM_S370		9	/* Amdahl UTS on System/370 */
#define	EM_MIPS_RS3_LE	10	/* MIPS RS3000 Little-endian */
			/* 11-14 - Reserved */
#define	EM_RS6000	11	/* IBM RS/6000 XXX reserved */
#define	EM_PARISC	15	/* Hewlett-Packard PA-RISC */
#define	EM_NCUBE	16	/* NCube XXX reserved */
#define	EM_VPP500	17	/* Fujitsu VPP500 */
#define	EM_SPARC32PLUS	18	/* Enhanced instruction set SPARC */
#define	EM_960		19	/* Intel 80960 */
#define	EM_PPC		20	/* PowerPC */
#define	EM_PPC64	21	/* 64-bit PowerPC */
			/* 22-35 - Reserved */
#define	EM_V800		36	/* NEC V800 */
#define	EM_FR20		37	/* Fujitsu FR20 */
#define	EM_RH32		38	/* TRW RH-32 */
#define	EM_RCE		39	/* Motorola RCE */
#define	EM_ARM		40	/* Advanced RISC Machines ARM */
#define	EM_ALPHA	41	/* DIGITAL Alpha */
#define	EM_SH		42	/* Hitachi Super-H */
#define	EM_SPARCV9	43	/* SPARC Version 9 */
#define	EM_TRICORE	44	/* Siemens Tricore */
#define	EM_ARC		45	/* Argonaut RISC Core */
#define	EM_H8_300	46	/* Hitachi H8/300 */
#define	EM_H8_300H	47	/* Hitachi H8/300H */
#define	EM_H8S		48	/* Hitachi H8S */
#define	EM_H8_500	49	/* Hitachi H8/500 */
#define	EM_IA_64	50	/* Intel Merced Processor */
#define	EM_MIPS_X	51	/* Stanford MIPS-X */
#define	EM_COLDFIRE	52	/* Motorola Coldfire */
#define	EM_68HC12	53	/* Motorola MC68HC12 */
#define	EM_MMA		54	/* Fujitsu MMA Multimedia Accelerator */
#define	EM_PCP		55	/* Siemens PCP */
#define	EM_NCPU		56	/* Sony nCPU embedded RISC processor */
#define	EM_NDR1		57	/* Denso NDR1 microprocessor */
#define	EM_STARCORE	58	/* Motorola Star*Core processor */
#define	EM_ME16		59	/* Toyota ME16 processor */
#define	EM_ST100	60	/* STMicroelectronics ST100 processor */
#define	EM_TINYJ	61	/* Advanced Logic Corp. TinyJ embedded family processor */
#define	EM_X86_64	62	/* AMD x86-64 architecture */
#define	EM_PDSP		63	/* Sony DSP Processor */
			/* 64-65 - Reserved */
#define	EM_FX66		66	/* Siemens FX66 microcontroller */
#define	EM_ST9PLUS	67	/* STMicroelectronics ST9+ 8/16 bit microcontroller */
#define	EM_ST7		68	/* STMicroelectronics ST7 8-bit microcontroller */
#define	EM_68HC16	69	/* Motorola MC68HC16 Microcontroller */
#define	EM_68HC11	70	/* Motorola MC68HC11 Microcontroller */
#define	EM_68HC08	71	/* Motorola MC68HC08 Microcontroller */
#define	EM_68HC05	72	/* Motorola MC68HC05 Microcontroller */
#define	EM_SVX		73	/* Silicon Graphics SVx */
#define	EM_ST19		74	/* STMicroelectronics ST19 8-bit cpu */
#define	EM_VAX		75	/* Digital VAX */
#define	EM_CRIS		76	/* Axis Communications 32-bit embedded processor */
#define	EM_JAVELIN	77	/* Infineon Technologies 32-bit embedded cpu */
#define	EM_FIREPATH	78	/* Element 14 64-bit DSP processor */
#define	EM_ZSP		79	/* LSI Logic's 16-bit DSP processor */
#define	EM_MMIX		80	/* Donald Knuth's educational 64-bit processor */
#define	EM_HUANY	81	/* Harvard's machine-independent format */
#define	EM_PRISM	82	/* SiTera Prism */
#define	EM_AVR		83	/* Atmel AVR 8-bit microcontroller */
#define	EM_FR30		84	/* Fujitsu FR30 */
#define	EM_D10V		85	/* Mitsubishi D10V */
#define	EM_D30V		86	/* Mitsubishi D30V */
#define	EM_V850		87	/* NEC v850 */
#define	EM_M32R		88	/* Mitsubishi M32R */
#define	EM_MN10300	89	/* Matsushita MN10300 */
#define	EM_MN10200	90	/* Matsushita MN10200 */
#define	EM_PJ		91	/* picoJava */
#define	EM_OPENRISC	92	/* OpenRISC 32-bit embedded processor */
#define	EM_ARC_A5	93	/* ARC Cores Tangent-A5 */
#define	EM_XTENSA	94	/* Tensilica Xtensa Architecture */
/* Unofficial machine types follow */
#define	EM_ALPHA_EXP	36902	/* used by NetBSD/alpha; obsolete */
#define	EM_NUM		36903

/*
 * Program Header
 */
typedef struct {
	Elf32_Word	p_type;		/* entry type */
	Elf32_Off	p_offset;	/* offset */
	Elf32_Addr	p_vaddr;	/* virtual address */
	Elf32_Addr	p_paddr;	/* physical address */
	Elf32_Word	p_filesz;	/* file size */
	Elf32_Word	p_memsz;	/* memory size */
	Elf32_Word	p_flags;	/* flags */
	Elf32_Word	p_align;	/* memory & file alignment */
} Elf32_Phdr;

/* p_type */
#define	PT_NULL		0		/* Program header table entry unused */
#define	PT_LOAD		1		/* Loadable program segment */
#define	PT_DYNAMIC	2		/* Dynamic linking information */
#define	PT_INTERP	3		/* Program interpreter */
#define	PT_NOTE		4		/* Auxiliary information */
#define	PT_SHLIB	5		/* Reserved, unspecified semantics */
#define	PT_PHDR		6		/* Entry for header table itself */
#define	PT_NUM		7

/* p_flags */
#define	PF_R		0x4	/* Segment is readable */
#define	PF_W		0x2	/* Segment is writable */
#define	PF_X		0x1	/* Segment is executable */

#define	PF_MASKOS	0x0ff00000	/* Opersting system specific values */
#define	PF_MASKPROC	0xf0000000	/* Processor-specific values */

#define	PT_LOPROC	0x70000000	/* Processor-specific range */
#define	PT_HIPROC	0x7fffffff

#define	PT_MIPS_REGINFO	0x70000000

/*
 * Section Headers
 */
typedef struct {
	Elf32_Word	sh_name;	/* section name (.shstrtab index) */
	Elf32_Word	sh_type;	/* section type */
	Elf32_Word	sh_flags;	/* section flags */
	Elf32_Addr	sh_addr;	/* virtual address */
	Elf32_Off	sh_offset;	/* file offset */
	Elf32_Word	sh_size;	/* section size */
	Elf32_Word	sh_link;	/* link to another */
	Elf32_Word	sh_info;	/* misc info */
	Elf32_Word	sh_addralign;	/* memory alignment */
	Elf32_Word	sh_entsize;	/* table entry size */
} Elf32_Shdr;

/* sh_type */
#define	SHT_NULL	0		/* Section header table entry unused */
#define	SHT_PROGBITS	1		/* Program information */
#define	SHT_SYMTAB	2		/* Symbol table */
#define	SHT_STRTAB	3		/* String table */
#define	SHT_RELA	4		/* Relocation information w/ addend */
#define	SHT_HASH	5		/* Symbol hash table */
#define	SHT_DYNAMIC	6		/* Dynamic linking information */
#define	SHT_NOTE	7		/* Auxiliary information */
#define	SHT_NOBITS	8		/* No space allocated in file image */
#define	SHT_REL		9		/* Relocation information w/o addend */
#define	SHT_SHLIB	10		/* Reserved, unspecified semantics */
#define	SHT_DYNSYM	11		/* Symbol table for dynamic linker */
#define	SHT_NUM		12

#define	SHT_LOOS	0x60000000	/* Operating system specific range */
#define	SHT_HIOS	0x6fffffff
#define	SHT_LOPROC	0x70000000	/* Processor-specific range */
#define	SHT_HIPROC	0x7fffffff
#define	SHT_LOUSER	0x80000000	/* Application-specific range */
#define	SHT_HIUSER	0xffffffff

/* sh_flags */
#define	SHF_WRITE	0x1		/* Section contains writable data */
#define	SHF_ALLOC	0x2		/* Section occupies memory */
#define	SHF_EXECINSTR	0x4		/* Section contains executable insns */

#define	SHF_MASKOS	0x0f000000	/* Operating system specific values */
#define	SHF_MASKPROC	0xf0000000	/* Processor-specific values */

/*
 * Symbol Table
 */
typedef struct {
	Elf32_Word	st_name;	/* Symbol name (.symtab index) */
	Elf32_Word	st_value;	/* value of symbol */
	Elf32_Word	st_size;	/* size of symbol */
	Elf_Byte	st_info;	/* type / binding attrs */
	Elf_Byte	st_other;	/* unused */
	Elf32_Half	st_shndx;	/* section index of symbol */
} Elf32_Sym;

/* Symbol Table index of the undefined symbol */
#define	ELF_SYM_UNDEFINED	0

/* st_info: Symbol Bindings */
#define	STB_LOCAL		0	/* local symbol */
#define	STB_GLOBAL		1	/* global symbol */
#define	STB_WEAK		2	/* weakly defined global symbol */
#define	STB_NUM			3

#define	STB_LOOS		10	/* Operating system specific range */
#define	STB_HIOS		12
#define	STB_LOPROC		13	/* Processor-specific range */
#define	STB_HIPROC		15

/* st_info: Symbol Types */
#define	STT_NOTYPE		0	/* Type not specified */
#define	STT_OBJECT		1	/* Associated with a data object */
#define	STT_FUNC		2	/* Associated with a function */
#define	STT_SECTION		3	/* Associated with a section */
#define	STT_FILE		4	/* Associated with a file name */
#define	STT_NUM			5

#define	STT_LOOS		10	/* Operating system specific range */
#define	STT_HIOS		12
#define	STT_LOPROC		13	/* Processor-specific range */
#define	STT_HIPROC		15

/* st_info utility macros */
#define	ELF32_ST_BIND(info)		((Elf32_Word)(info) >> 4)
#define	ELF32_ST_TYPE(info)		((Elf32_Word)(info) & 0xf)
#define	ELF32_ST_INFO(bind,type)	((Elf_Byte)(((bind) << 4) | ((type) & 0xf)))

/*
 * Special section indexes
 */
#define	SHN_UNDEF	0		/* Undefined section */

#define	SHN_LORESERVE	0xff00		/* Reserved range */
#define	SHN_ABS		0xfff1		/*  Absolute symbols */
#define	SHN_COMMON	0xfff2		/*  Common symbols */
#define	SHN_HIRESERVE	0xffff

#define	SHN_LOPROC	0xff00		/* Processor-specific range */
#define	SHN_HIPROC	0xff1f
#define	SHN_LOOS	0xff20		/* Operating system specific range */
#define	SHN_HIOS	0xff3f

#define	SHN_MIPS_ACOMMON 0xff00
#define	SHN_MIPS_TEXT	0xff01
#define	SHN_MIPS_DATA	0xff02
#define	SHN_MIPS_SCOMMON 0xff03

/*
 * Relocation Entries
 */
typedef struct {
	Elf32_Word	r_offset;	/* where to do it */
	Elf32_Word	r_info;		/* index & type of relocation */
} Elf32_Rel;

typedef struct {
	Elf32_Word	r_offset;	/* where to do it */
	Elf32_Word	r_info;		/* index & type of relocation */
	Elf32_Sword	r_addend;	/* adjustment value */
} Elf32_Rela;

/* r_info utility macros */
#define	ELF32_R_SYM(info)	((info) >> 8)
#define	ELF32_R_TYPE(info)	((info) & 0xff)
#define	ELF32_R_INFO(sym, type)	(((sym) << 8) + (unsigned char)(type))

/*
 * Dynamic Section structure array
 */
typedef struct {
	Elf32_Word	d_tag;		/* entry tag value */
	union {
	    Elf32_Addr	d_ptr;
	    Elf32_Word	d_val;
	} d_un;
} Elf32_Dyn;

/* d_tag */
#define	DT_NULL		0	/* Marks end of dynamic array */
#define	DT_NEEDED	1	/* Name of needed library (DT_STRTAB offset) */
#define	DT_PLTRELSZ	2	/* Size, in bytes, of relocations in PLT */
#define	DT_PLTGOT	3	/* Address of PLT and/or GOT */
#define	DT_HASH		4	/* Address of symbol hash table */
#define	DT_STRTAB	5	/* Address of string table */
#define	DT_SYMTAB	6	/* Address of symbol table */
#define	DT_RELA		7	/* Address of Rela relocation table */
#define	DT_RELASZ	8	/* Size, in bytes, of DT_RELA table */
#define	DT_RELAENT	9	/* Size, in bytes, of one DT_RELA entry */
#define	DT_STRSZ	10	/* Size, in bytes, of DT_STRTAB table */
#define	DT_SYMENT	11	/* Size, in bytes, of one DT_SYMTAB entry */
#define	DT_INIT		12	/* Address of initialization function */
#define	DT_FINI		13	/* Address of termination function */
#define	DT_SONAME	14	/* Shared object name (DT_STRTAB offset) */
#define	DT_RPATH	15	/* Library search path (DT_STRTAB offset) */
#define	DT_SYMBOLIC	16	/* Start symbol search within local object */
#define	DT_REL		17	/* Address of Rel relocation table */
#define	DT_RELSZ	18	/* Size, in bytes, of DT_REL table */
#define	DT_RELENT	19	/* Size, in bytes, of one DT_REL entry */
#define	DT_PLTREL	20 	/* Type of PLT relocation entries */
#define	DT_DEBUG	21	/* Used for debugging; unspecified */
#define	DT_TEXTREL	22	/* Relocations might modify non-writable seg */
#define	DT_JMPREL	23	/* Address of relocations associated with PLT */
#define	DT_BIND_NOW	24	/* Process all relocations at load-time */
#define	DT_INIT_ARRAY	25	/* Address of initialization function array */
#define	DT_FINI_ARRAY	26	/* Size, in bytes, of DT_INIT_ARRAY array */
#define	DT_INIT_ARRAYSZ	27	/* Address of termination function array */
#define	DT_FINI_ARRAYSZ	28	/* Size, in bytes, of DT_FINI_ARRAY array*/
#define	DT_NUM		29

#define	DT_LOOS		0x60000000	/* Operating system specific range */
#define	DT_HIOS		0x6fffffff
#define	DT_LOPROC	0x70000000	/* Processor-specific range */
#define	DT_HIPROC	0x7fffffff

/*
 * Auxiliary Vectors
 */
typedef struct {
	Elf32_Word	a_type;				/* 32-bit id */
	Elf32_Word	a_v;				/* 32-bit id */
} Aux32Info;

/* a_type */
#define	AT_NULL		0	/* Marks end of array */
#define	AT_IGNORE	1	/* No meaning, a_un is undefined */
#define	AT_EXECFD	2	/* Open file descriptor of object file */
#define	AT_PHDR		3	/* &phdr[0] */
#define	AT_PHENT	4	/* sizeof(phdr[0]) */
#define	AT_PHNUM	5	/* # phdr entries */
#define	AT_PAGESZ	6	/* PAGESIZE */
#define	AT_BASE		7	/* Interpreter base addr */
#define	AT_FLAGS	8	/* Processor flags */
#define	AT_ENTRY	9	/* Entry address of executable */
#define	AT_DCACHEBSIZE	10	/* Data cache block size */
#define	AT_ICACHEBSIZE	11	/* Instruction cache block size */
#define	AT_UCACHEBSIZE	12	/* Unified cache block size */

	/* Vendor specific */
#define	AT_MIPS_NOTELF	10	/* XXX a_val != 0 -> MIPS XCOFF executable */

#define	AT_SUN_UID	2000	/* euid */
#define	AT_SUN_RUID	2001	/* ruid */
#define	AT_SUN_GID	2002	/* egid */
#define	AT_SUN_RGID	2003	/* rgid */

	/* Solaris kernel specific */
#define	AT_SUN_LDELF	2004	/* dynamic linker's ELF header */
#define	AT_SUN_LDSHDR	2005	/* dynamic linker's section header */
#define	AT_SUN_LDNAME	2006	/* dynamic linker's name */
#define	AT_SUN_LPGSIZE	2007	/* large pagesize */

	/* Other information */
#define	AT_SUN_PLATFORM	2008	/* sysinfo(SI_PLATFORM) */
#define	AT_SUN_HWCAP	2009	/* process hardware capabilities */
#define	AT_SUN_IFLUSH	2010	/* do we need to flush the instruction cache? */
#define	AT_SUN_CPU	2011	/* cpu name */
	/* ibcs2 emulation band aid */
#define	AT_SUN_EMUL_ENTRY 2012	/* coff entry point */
#define	AT_SUN_EMUL_EXECFD 2013	/* coff file descriptor */
	/* Executable's fully resolved name */
#define	AT_SUN_EXECNAME	2014

/*
 * Note Headers
 */
typedef struct {
	Elf32_Word n_namesz;
	Elf32_Word n_descsz;
	Elf32_Word n_type;
} Elf32_Nhdr;

#define	ELF_NOTE_TYPE_ABI_TAG		1

/* GNU-specific note name and description sizes */
#define	ELF_NOTE_ABI_NAMESZ		4
#define	ELF_NOTE_ABI_DESCSZ		16
/* GNU-specific note name */
#define	ELF_NOTE_ABI_NAME		"GNU\0"

/* GNU-specific OS/version value stuff */
#define	ELF_NOTE_ABI_OS_LINUX		0
#define	ELF_NOTE_ABI_OS_HURD		1
#define	ELF_NOTE_ABI_OS_SOLARIS		2

/* NetBSD-specific note type: Emulation name.  desc is emul name string. */
#define	ELF_NOTE_TYPE_NETBSD_TAG	1

/* NetBSD-specific note name and description sizes */
#define	ELF_NOTE_NETBSD_NAMESZ		7
#define	ELF_NOTE_NETBSD_DESCSZ		4
/* NetBSD-specific note name */
#define	ELF_NOTE_NETBSD_NAME		"NetBSD\0\0"

#if defined(ELFSIZE)
#define	CONCAT(x,y)	__CONCAT(x,y)
#define	ELFNAME(x)	CONCAT(elf,CONCAT(ELFSIZE,CONCAT(_,x)))
#define	ELFNAME2(x,y)	CONCAT(x,CONCAT(_elf,CONCAT(ELFSIZE,CONCAT(_,y))))
#define	ELFNAMEEND(x)	CONCAT(x,CONCAT(_elf,ELFSIZE))
#define	ELFDEFNNAME(x)	CONCAT(ELF,CONCAT(ELFSIZE,CONCAT(_,x)))
#endif

/*
 * Leo: This is actually from machine/elf_machdep.h
 */
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_68K:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define ARCH_ELFSIZE		32	/* MD native binary size */

/* m68k relocation types */
#define	R_68K_NONE	0
#define	R_68K_32	1
#define	R_68K_16	2
#define	R_68K_8		3
#define	R_68K_PC32	4
#define	R_68K_PC16	5
#define	R_68K_PC8	6
#define	R_68K_GOT32	7
#define	R_68K_GOT16	8
#define	R_68K_GOT8	9
#define	R_68K_GOT32O	10
#define	R_68K_GOT16O	11
#define	R_68K_GOT8O	12
#define	R_68K_PLT32	13
#define	R_68K_PLT16	14
#define	R_68K_PLT8	15
#define	R_68K_PLT32O	16
#define	R_68K_PLT16O	17
#define	R_68K_PLT8O	18
#define	R_68K_COPY	19
#define	R_68K_GLOB_DAT	20
#define	R_68K_JMP_SLOT	21
#define	R_68K_RELATIVE	22

#define	R_TYPE(name)	__CONCAT(R_68K_,name)

#define	Elf_Ehdr	Elf32_Ehdr
#define	Elf_Phdr	Elf32_Phdr
#define	Elf_Shdr	Elf32_Shdr
#define	Elf_Sym		Elf32_Sym
#define	Elf_Rel		Elf32_Rel
#define	Elf_Rela	Elf32_Rela
#define	Elf_Dyn		Elf32_Dyn
#define	Elf_Word	Elf32_Word
#define	Elf_Sword	Elf32_Sword
#define	Elf_Addr	Elf32_Addr
#define	Elf_Off		Elf32_Off
#define	Elf_Nhdr	Elf32_Nhdr

#define	ELF_R_SYM	ELF32_R_SYM
#define	ELF_R_TYPE	ELF32_R_TYPE
#define	ELFCLASS	ELFCLASS32

#define	ELF_ST_BIND	ELF32_ST_BIND
#define	ELF_ST_TYPE	ELF32_ST_TYPE
#define	ELF_ST_INFO	ELF32_ST_INFO

#define	AuxInfo		Aux32Info

#ifdef _KERNEL

#define ELF_AUX_ENTRIES	8		/* Size of aux array passed to loader */
#define ELF32_NO_ADDR	(~(Elf32_Addr)0) /* Indicates addr. not yet filled in */
#define ELF64_NO_ADDR	(~(Elf64_Addr)0) /* Indicates addr. not yet filled in */

#if defined(ELFSIZE) && (ELFSIZE == 64)
#define ELF_NO_ADDR	ELF64_NO_ADDR
#elif defined(ELFSIZE) && (ELFSIZE == 32)
#define ELF_NO_ADDR	ELF32_NO_ADDR
#endif

#if defined(ELFSIZE)
struct elf_args {
        Elf_Addr  arg_entry;      /* program entry point */
        Elf_Addr  arg_interp;     /* Interpreter load address */
        Elf_Addr  arg_phaddr;     /* program header address */
        Elf_Addr  arg_phentsize;  /* Size of program header */
        Elf_Addr  arg_phnum;      /* Number of program headers */
};
#endif

#endif /* _KERNEL */

#endif /* !_SYS_EXEC_ELF_H_ */
