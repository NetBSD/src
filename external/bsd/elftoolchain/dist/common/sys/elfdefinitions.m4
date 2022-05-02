dnl 	$NetBSD: elfdefinitions.m4,v 1.4 2022/05/02 20:27:43 jkoshy Exp $
/*-
 * Copyright (c) 2010,2021 Joseph Koshy
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
divert(-1)
define(`VCSID_ELFDEFINITIONS_M4',
	`Id: elfdefinitions.m4 3980 2022-05-02 19:50:00Z jkoshy')
include(`elfconstants.m4')dnl

define(`_',`ifelse(eval(len($1) <= 7),1,
    `#define	$1		$2',
    `#define	$1	$2')')
divert(0)dnl

/*
 * WARNING: GENERATED FILE.  DO NOT MODIFY.
 *
 *  GENERATED FROM: VCSID_ELFDEFINITIONS_M4
 *  GENERATED FROM: VCSID_ELFCONSTANTS_M4
 */

/*
 * These definitions are based on:
 * - The public specification of the ELF format as defined in the
 *   October 2009 draft of System V ABI.
 *   See: http://www.sco.com/developers/gabi/latest/ch4.intro.html
 * - The May 1998 (version 1.5) draft of "The ELF-64 object format".
 * - Processor-specific ELF ABI definitions for sparc, i386, amd64, mips,
 *   ia64, powerpc, and RISC-V processors.
 * - The "Linkers and Libraries Guide", from Sun Microsystems.
 */

#ifndef _SYS_ELFDEFINITIONS_H_
#define _SYS_ELFDEFINITIONS_H_

#include <stdint.h>

/*
 * Types of capabilities.
 */
DEFINE_CAPABILITIES()

/*
 * Flags used with dynamic linking entries.
 */
DEFINE_DYN_FLAGS()

/*
 * Dynamic linking entry types.
 */
DEFINE_DYN_TYPES()

/* Aliases for dynamic linking entry symbols. */
DEFINE_DYN_TYPE_ALIASES()

/*
 * Flags used in the executable header (field: e_flags).
 */
DEFINE_EHDR_FLAGS()

/*
 * Offsets in the `ei_ident[]' field of an ELF executable header.
 */
DEFINE_EI_OFFSETS()

/*
 * The ELF class of an object.
 */
DEFINE_ELF_CLASSES()

/*
 * Endianness of data in an ELF object.
 */
DEFINE_ELF_DATA_ENDIANNESSES()

/*
 * The magic numbers used in the initial four bytes of an ELF object.
 *
 * These numbers are: 0x7F, 'E', 'L' and 'F'.
 */
DEFINE_ELF_MAGIC_VALUES()
/* Additional magic-related constants. */
DEFINE_ELF_MAGIC_ADDITIONAL_CONSTANTS()

/*
 * ELF OS ABI field.
 */
DEFINE_ELF_OSABIS()

/* OS ABI Aliases. */
DEFINE_ELF_OSABI_ALIASES()

/*
 * ELF Machine types: (EM_*).
 */
DEFINE_ELF_MACHINE_TYPES()
/* Other synonyms. */
DEFINE_ELF_MACHINE_TYPE_SYNONYMS()

/*
 * ELF file types: (ET_*).
 */
DEFINE_ELF_TYPES()

/* ELF file format version numbers. */
DEFINE_ELF_FILE_VERSIONS()

/*
 * Flags for section groups.
 */
DEFINE_GRP_FLAGS()

/*
 * Flags / mask for .gnu.versym sections.
 */
DEFINE_VERSYMS()

/*
 * Flags used by program header table entries.
 */
DEFINE_PHDR_FLAGS()

/*
 * Types of program header table entries.
 */
DEFINE_PHDR_TYPES()
/* synonyms. */
DEFINE_PHDR_TYPE_SYNONYMS()

/*
 * Section flags.
 */
DEFINE_SECTION_FLAGS()

/*
 * Special section indices.
 */
DEFINE_SECTION_INDICES()

/*
 * Section types.
 */
DEFINE_SECTION_TYPES()
/* Aliases for section types. */
DEFINE_SECTION_TYPE_ALIASES()

#define	PN_XNUM			0xFFFFU /* Use extended section numbering. */

/*
 * Symbol binding information.
 */
DEFINE_SYMBOL_BINDINGS()

/*
 * Symbol types
 */
DEFINE_SYMBOL_TYPES()
/* Additional constants related to symbol types. */
DEFINE_SYMBOL_TYPES_ADDITIONAL_CONSTANTS()

/*
 * Symbol binding.
 */
DEFINE_SYMBOL_BINDING_KINDS()

/*
 * Symbol visibility.
 */
DEFINE_SYMBOL_VISIBILITIES()

/*
 * Symbol flags.
 */
DEFINE_SYMBOL_FLAGS()

/*
 * Versioning dependencies.
 */
DEFINE_VERSIONING_DEPENDENCIES()

/*
 * Versioning flags.
 */
DEFINE_VERSIONING_FLAGS()

/*
 * Versioning needs
 */
DEFINE_VERSIONING_NEEDS()

/*
 * Versioning numbers.
 */
DEFINE_VERSIONING_NUMBERS()

/**
 ** Relocation types.
 **/
DEFINE_RELOCATIONS()

/*
 * MIPS ABI related.
 */
DEFINE_MIPS_ABIS()

/**
 ** ELF Types.
 **/

typedef uint32_t	Elf32_Addr;	/* Program address. */
typedef uint8_t		Elf32_Byte;	/* Unsigned tiny integer. */
typedef uint16_t	Elf32_Half;	/* Unsigned medium integer. */
typedef uint32_t	Elf32_Off;	/* File offset. */
typedef uint16_t	Elf32_Section;	/* Section index. */
typedef int32_t		Elf32_Sword;	/* Signed integer. */
typedef uint32_t	Elf32_Word;	/* Unsigned integer. */
typedef uint64_t	Elf32_Lword;	/* Unsigned long integer. */

typedef uint64_t	Elf64_Addr;	/* Program address. */
typedef uint8_t		Elf64_Byte;	/* Unsigned tiny integer. */
typedef uint16_t	Elf64_Half;	/* Unsigned medium integer. */
typedef uint64_t	Elf64_Off;	/* File offset. */
typedef uint16_t	Elf64_Section;	/* Section index. */
typedef int32_t		Elf64_Sword;	/* Signed integer. */
typedef uint32_t	Elf64_Word;	/* Unsigned integer. */
typedef uint64_t	Elf64_Lword;	/* Unsigned long integer. */
typedef uint64_t	Elf64_Xword;	/* Unsigned long integer. */
typedef int64_t		Elf64_Sxword;	/* Signed long integer. */


/*
 * Capability descriptors.
 */

/* 32-bit capability descriptor. */
typedef struct {
	Elf32_Word	c_tag;	     /* Type of entry. */
	union {
		Elf32_Word	c_val; /* Integer value. */
		Elf32_Addr	c_ptr; /* Pointer value. */
	} c_un;
} Elf32_Cap;

/* 64-bit capability descriptor. */
typedef struct {
	Elf64_Xword	c_tag;	     /* Type of entry. */
	union {
		Elf64_Xword	c_val; /* Integer value. */
		Elf64_Addr	c_ptr; /* Pointer value. */
	} c_un;
} Elf64_Cap;

/*
 * MIPS .conflict section entries.
 */

/* 32-bit entry. */
typedef struct {
	Elf32_Addr	c_index;
} Elf32_Conflict;

/* 64-bit entry. */
typedef struct {
	Elf64_Addr	c_index;
} Elf64_Conflict;

/*
 * Dynamic section entries.
 */

/* 32-bit entry. */
typedef struct {
	Elf32_Sword	d_tag;	     /* Type of entry. */
	union {
		Elf32_Word	d_val; /* Integer value. */
		Elf32_Addr	d_ptr; /* Pointer value. */
	} d_un;
} Elf32_Dyn;

/* 64-bit entry. */
typedef struct {
	Elf64_Sxword	d_tag;	     /* Type of entry. */
	union {
		Elf64_Xword	d_val; /* Integer value. */
		Elf64_Addr	d_ptr; /* Pointer value; */
	} d_un;
} Elf64_Dyn;


/*
 * The executable header (EHDR).
 */

/* 32 bit EHDR. */
typedef struct {
	unsigned char   e_ident[EI_NIDENT]; /* ELF identification. */
	Elf32_Half      e_type;	     /* Object file type (ET_*). */
	Elf32_Half      e_machine;   /* Machine type (EM_*). */
	Elf32_Word      e_version;   /* File format version (EV_*). */
	Elf32_Addr      e_entry;     /* Start address. */
	Elf32_Off       e_phoff;     /* File offset to the PHDR table. */
	Elf32_Off       e_shoff;     /* File offset to the SHDRheader. */
	Elf32_Word      e_flags;     /* Flags (EF_*). */
	Elf32_Half      e_ehsize;    /* Elf header size in bytes. */
	Elf32_Half      e_phentsize; /* PHDR table entry size in bytes. */
	Elf32_Half      e_phnum;     /* Number of PHDR entries. */
	Elf32_Half      e_shentsize; /* SHDR table entry size in bytes. */
	Elf32_Half      e_shnum;     /* Number of SHDR entries. */
	Elf32_Half      e_shstrndx;  /* Index of section name string table. */
} Elf32_Ehdr;


/* 64 bit EHDR. */
typedef struct {
	unsigned char   e_ident[EI_NIDENT]; /* ELF identification. */
	Elf64_Half      e_type;	     /* Object file type (ET_*). */
	Elf64_Half      e_machine;   /* Machine type (EM_*). */
	Elf64_Word      e_version;   /* File format version (EV_*). */
	Elf64_Addr      e_entry;     /* Start address. */
	Elf64_Off       e_phoff;     /* File offset to the PHDR table. */
	Elf64_Off       e_shoff;     /* File offset to the SHDRheader. */
	Elf64_Word      e_flags;     /* Flags (EF_*). */
	Elf64_Half      e_ehsize;    /* Elf header size in bytes. */
	Elf64_Half      e_phentsize; /* PHDR table entry size in bytes. */
	Elf64_Half      e_phnum;     /* Number of PHDR entries. */
	Elf64_Half      e_shentsize; /* SHDR table entry size in bytes. */
	Elf64_Half      e_shnum;     /* Number of SHDR entries. */
	Elf64_Half      e_shstrndx;  /* Index of section name string table. */
} Elf64_Ehdr;


/*
 * Shared object information.
 */

/* 32-bit entry. */
typedef struct {
	Elf32_Word l_name;	     /* The name of a shared object. */
	Elf32_Word l_time_stamp;     /* 32-bit timestamp. */
	Elf32_Word l_checksum;	     /* Checksum of visible symbols, sizes. */
	Elf32_Word l_version;	     /* Interface version string index. */
	Elf32_Word l_flags;	     /* Flags (LL_*). */
} Elf32_Lib;

/* 64-bit entry. */
typedef struct {
	Elf64_Word l_name;	     /* The name of a shared object. */
	Elf64_Word l_time_stamp;     /* 32-bit timestamp. */
	Elf64_Word l_checksum;	     /* Checksum of visible symbols, sizes. */
	Elf64_Word l_version;	     /* Interface version string index. */
	Elf64_Word l_flags;	     /* Flags (LL_*). */
} Elf64_Lib;

DEFINE_LL_FLAGS()

/*
 * Note tags
 */
DEFINE_NOTE_ENTRY_TYPES()
/* Aliases for the ABI tag. */
DEFINE_NOTE_ENTRY_ALIASES()

/*
 * Note descriptors.
 */

typedef	struct {
	uint32_t	n_namesz;    /* Length of note's name. */
	uint32_t	n_descsz;    /* Length of note's value. */
	uint32_t	n_type;	     /* Type of note. */
} Elf_Note;

typedef Elf_Note Elf32_Nhdr;	     /* 32-bit note header. */
typedef Elf_Note Elf64_Nhdr;	     /* 64-bit note header. */

/*
 * MIPS ELF options descriptor header.
 */

typedef struct {
	Elf64_Byte	kind;        /* Type of options. */
	Elf64_Byte     	size;	     /* Size of option descriptor. */
	Elf64_Half	section;     /* Index of section affected. */
	Elf64_Word	info;        /* Kind-specific information. */
} Elf_Options;

/*
 * Option kinds.
 */
DEFINE_OPTION_KINDS()

/*
 * ODK_EXCEPTIONS info field masks.
 */
DEFINE_OPTION_EXCEPTIONS()

/*
 * ODK_PAD info field masks.
 */
DEFINE_OPTION_PADS()

/*
 * ODK_HWPATCH info field masks and ODK_HWAND/ODK_HWOR info field
 * and hwp_flags[12] masks.
 */
DEFINE_ODK_HWPATCH_MASKS()

/*
 * ODK_IDENT/ODK_GP_GROUP info field masks.
 */
DEFINE_ODK_GP_MASKS()

/*
 * MIPS ELF register info descriptor.
 */

/* 32 bit RegInfo entry. */
typedef struct {
	Elf32_Word	ri_gprmask;  /* Mask of general register used. */
	Elf32_Word	ri_cprmask[4]; /* Mask of coprocessor register used. */
	Elf32_Addr	ri_gp_value; /* GP register value. */
} Elf32_RegInfo;

/* 64 bit RegInfo entry. */
typedef struct {
	Elf64_Word	ri_gprmask;  /* Mask of general register used. */
	Elf64_Word	ri_pad;	     /* Padding. */
	Elf64_Word	ri_cprmask[4]; /* Mask of coprocessor register used. */
	Elf64_Addr	ri_gp_value; /* GP register value. */
} Elf64_RegInfo;

/*
 * Program Header Table (PHDR) entries.
 */

/* 32 bit PHDR entry. */
typedef struct {
	Elf32_Word	p_type;	     /* Type of segment. */
	Elf32_Off	p_offset;    /* File offset to segment. */
	Elf32_Addr	p_vaddr;     /* Virtual address in memory. */
	Elf32_Addr	p_paddr;     /* Physical address (if relevant). */
	Elf32_Word	p_filesz;    /* Size of segment in file. */
	Elf32_Word	p_memsz;     /* Size of segment in memory. */
	Elf32_Word	p_flags;     /* Segment flags. */
	Elf32_Word	p_align;     /* Alignment constraints. */
} Elf32_Phdr;

/* 64 bit PHDR entry. */
typedef struct {
	Elf64_Word	p_type;	     /* Type of segment. */
	Elf64_Word	p_flags;     /* Segment flags. */
	Elf64_Off	p_offset;    /* File offset to segment. */
	Elf64_Addr	p_vaddr;     /* Virtual address in memory. */
	Elf64_Addr	p_paddr;     /* Physical address (if relevant). */
	Elf64_Xword	p_filesz;    /* Size of segment in file. */
	Elf64_Xword	p_memsz;     /* Size of segment in memory. */
	Elf64_Xword	p_align;     /* Alignment constraints. */
} Elf64_Phdr;


/*
 * Move entries, for describing data in COMMON blocks in a compact
 * manner.
 */

/* 32-bit move entry. */
typedef struct {
	Elf32_Lword	m_value;     /* Initialization value. */
	Elf32_Word 	m_info;	     /* Encoded size and index. */
	Elf32_Word	m_poffset;   /* Offset relative to symbol. */
	Elf32_Half	m_repeat;    /* Repeat count. */
	Elf32_Half	m_stride;    /* Number of units to skip. */
} Elf32_Move;

/* 64-bit move entry. */
typedef struct {
	Elf64_Lword	m_value;     /* Initialization value. */
	Elf64_Xword 	m_info;	     /* Encoded size and index. */
	Elf64_Xword	m_poffset;   /* Offset relative to symbol. */
	Elf64_Half	m_repeat;    /* Repeat count. */
	Elf64_Half	m_stride;    /* Number of units to skip. */
} Elf64_Move;

#define ELF32_M_SYM(I)		((I) >> 8)
#define ELF32_M_SIZE(I)		((unsigned char) (I))
#define ELF32_M_INFO(M, S)	(((M) << 8) + (unsigned char) (S))

#define ELF64_M_SYM(I)		((I) >> 8)
#define ELF64_M_SIZE(I)		((unsigned char) (I))
#define ELF64_M_INFO(M, S)	(((M) << 8) + (unsigned char) (S))

/*
 * Section Header Table (SHDR) entries.
 */

/* 32 bit SHDR */
typedef struct {
	Elf32_Word	sh_name;     /* index of section name */
	Elf32_Word	sh_type;     /* section type */
	Elf32_Word	sh_flags;    /* section flags */
	Elf32_Addr	sh_addr;     /* in-memory address of section */
	Elf32_Off	sh_offset;   /* file offset of section */
	Elf32_Word	sh_size;     /* section size in bytes */
	Elf32_Word	sh_link;     /* section header table link */
	Elf32_Word	sh_info;     /* extra information */
	Elf32_Word	sh_addralign; /* alignment constraint */
	Elf32_Word	sh_entsize;   /* size for fixed-size entries */
} Elf32_Shdr;

/* 64 bit SHDR */
typedef struct {
	Elf64_Word	sh_name;     /* index of section name */
	Elf64_Word	sh_type;     /* section type */
	Elf64_Xword	sh_flags;    /* section flags */
	Elf64_Addr	sh_addr;     /* in-memory address of section */
	Elf64_Off	sh_offset;   /* file offset of section */
	Elf64_Xword	sh_size;     /* section size in bytes */
	Elf64_Word	sh_link;     /* section header table link */
	Elf64_Word	sh_info;     /* extra information */
	Elf64_Xword	sh_addralign; /* alignment constraint */
	Elf64_Xword	sh_entsize;  /* size for fixed-size entries */
} Elf64_Shdr;


/*
 * Symbol table entries.
 */

typedef struct {
	Elf32_Word	st_name;     /* index of symbol's name */
	Elf32_Addr	st_value;    /* value for the symbol */
	Elf32_Word	st_size;     /* size of associated data */
	unsigned char	st_info;     /* type and binding attributes */
	unsigned char	st_other;    /* visibility */
	Elf32_Half	st_shndx;    /* index of related section */
} Elf32_Sym;

typedef struct {
	Elf64_Word	st_name;     /* index of symbol's name */
	unsigned char	st_info;     /* type and binding attributes */
	unsigned char	st_other;    /* visibility */
	Elf64_Half	st_shndx;    /* index of related section */
	Elf64_Addr	st_value;    /* value for the symbol */
	Elf64_Xword	st_size;     /* size of associated data */
} Elf64_Sym;

#define ELF32_ST_BIND(I)	((I) >> 4)
#define ELF32_ST_TYPE(I)	((I) & 0xFU)
#define ELF32_ST_INFO(B,T)	(((B) << 4) + ((T) & 0xF))

#define ELF64_ST_BIND(I)	((I) >> 4)
#define ELF64_ST_TYPE(I)	((I) & 0xFU)
#define ELF64_ST_INFO(B,T)	(((B) << 4) + ((T) & 0xF))

#define ELF32_ST_VISIBILITY(O)	((O) & 0x3)
#define ELF64_ST_VISIBILITY(O)	((O) & 0x3)

/*
 * Syminfo descriptors, containing additional symbol information.
 */

/* 32-bit entry. */
typedef struct {
	Elf32_Half	si_boundto;  /* Entry index with additional flags. */
	Elf32_Half	si_flags;    /* Flags. */
} Elf32_Syminfo;

/* 64-bit entry. */
typedef struct {
	Elf64_Half	si_boundto;  /* Entry index with additional flags. */
	Elf64_Half	si_flags;    /* Flags. */
} Elf64_Syminfo;

/*
 * Relocation descriptors.
 */

typedef struct {
	Elf32_Addr	r_offset;    /* location to apply relocation to */
	Elf32_Word	r_info;	     /* type+section for relocation */
} Elf32_Rel;

typedef struct {
	Elf32_Addr	r_offset;    /* location to apply relocation to */
	Elf32_Word	r_info;      /* type+section for relocation */
	Elf32_Sword	r_addend;    /* constant addend */
} Elf32_Rela;

typedef struct {
	Elf64_Addr	r_offset;    /* location to apply relocation to */
	Elf64_Xword	r_info;      /* type+section for relocation */
} Elf64_Rel;

typedef struct {
	Elf64_Addr	r_offset;    /* location to apply relocation to */
	Elf64_Xword	r_info;      /* type+section for relocation */
	Elf64_Sxword	r_addend;    /* constant addend */
} Elf64_Rela;


#define ELF32_R_SYM(I)		((I) >> 8)
#define ELF32_R_TYPE(I)		((unsigned char) (I))
#define ELF32_R_INFO(S,T)	(((S) << 8) + (unsigned char) (T))

#define ELF64_R_SYM(I)		((I) >> 32)
#define ELF64_R_TYPE(I)		((I) & 0xFFFFFFFFUL)
#define ELF64_R_INFO(S,T)	\
	(((Elf64_Xword) (S) << 32) + ((T) & 0xFFFFFFFFUL))

/*
 * Symbol versioning structures.
 */

/* 32-bit structures. */
typedef struct
{
	Elf32_Word	vda_name;    /* Index to name. */
	Elf32_Word	vda_next;    /* Offset to next entry. */
} Elf32_Verdaux;

typedef struct
{
	Elf32_Word	vna_hash;    /* Hash value of dependency name. */
	Elf32_Half	vna_flags;   /* Flags. */
	Elf32_Half	vna_other;   /* Unused. */
	Elf32_Word	vna_name;    /* Offset to dependency name. */
	Elf32_Word	vna_next;    /* Offset to next vernaux entry. */
} Elf32_Vernaux;

typedef struct
{
	Elf32_Half	vd_version;  /* Version information. */
	Elf32_Half	vd_flags;    /* Flags. */
	Elf32_Half	vd_ndx;	     /* Index into the versym section. */
	Elf32_Half	vd_cnt;	     /* Number of aux entries. */
	Elf32_Word	vd_hash;     /* Hash value of name. */
	Elf32_Word	vd_aux;	     /* Offset to aux entries. */
	Elf32_Word	vd_next;     /* Offset to next version definition. */
} Elf32_Verdef;

typedef struct
{
	Elf32_Half	vn_version;  /* Version number. */
	Elf32_Half	vn_cnt;	     /* Number of aux entries. */
	Elf32_Word	vn_file;     /* Offset of associated file name. */
	Elf32_Word	vn_aux;	     /* Offset of vernaux array. */
	Elf32_Word	vn_next;     /* Offset of next verneed entry. */
} Elf32_Verneed;

typedef Elf32_Half	Elf32_Versym;

/* 64-bit structures. */

typedef struct {
	Elf64_Word	vda_name;    /* Index to name. */
	Elf64_Word	vda_next;    /* Offset to next entry. */
} Elf64_Verdaux;

typedef struct {
	Elf64_Word	vna_hash;    /* Hash value of dependency name. */
	Elf64_Half	vna_flags;   /* Flags. */
	Elf64_Half	vna_other;   /* Unused. */
	Elf64_Word	vna_name;    /* Offset to dependency name. */
	Elf64_Word	vna_next;    /* Offset to next vernaux entry. */
} Elf64_Vernaux;

typedef struct {
	Elf64_Half	vd_version;  /* Version information. */
	Elf64_Half	vd_flags;    /* Flags. */
	Elf64_Half	vd_ndx;	     /* Index into the versym section. */
	Elf64_Half	vd_cnt;	     /* Number of aux entries. */
	Elf64_Word	vd_hash;     /* Hash value of name. */
	Elf64_Word	vd_aux;	     /* Offset to aux entries. */
	Elf64_Word	vd_next;     /* Offset to next version definition. */
} Elf64_Verdef;

typedef struct {
	Elf64_Half	vn_version;  /* Version number. */
	Elf64_Half	vn_cnt;	     /* Number of aux entries. */
	Elf64_Word	vn_file;     /* Offset of associated file name. */
	Elf64_Word	vn_aux;	     /* Offset of vernaux array. */
	Elf64_Word	vn_next;     /* Offset of next verneed entry. */
} Elf64_Verneed;

typedef Elf64_Half	Elf64_Versym;


/*
 * The header for GNU-style hash sections.
 */

typedef struct {
	uint32_t	gh_nbuckets;	/* Number of hash buckets. */
	uint32_t	gh_symndx;	/* First visible symbol in .dynsym. */
	uint32_t	gh_maskwords;	/* #maskwords used in bloom filter. */
	uint32_t	gh_shift2;	/* Bloom filter shift count. */
} Elf_GNU_Hash_Header;

#endif	/* _SYS_ELFDEFINITIONS_H_ */
