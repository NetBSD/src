/* Mach-O support for BFD.
   Copyright 1999, 2000, 2001, 2002, 2003, 2005, 2007, 2008, 2009
   Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _BFD_MACH_O_H_
#define _BFD_MACH_O_H_

#include "bfd.h"

/* Symbol n_type values.  */
#define BFD_MACH_O_N_STAB  0xe0	/* If any of these bits set, a symbolic debugging entry.  */
#define BFD_MACH_O_N_PEXT  0x10	/* Private external symbol bit.  */
#define BFD_MACH_O_N_TYPE  0x0e	/* Mask for the type bits.  */
#define BFD_MACH_O_N_EXT   0x01	/* External symbol bit, set for external symbols.  */
#define BFD_MACH_O_N_UNDF  0x00	/* Undefined, n_sect == NO_SECT.  */
#define BFD_MACH_O_N_ABS   0x02	/* Absolute, n_sect == NO_SECT.  */
#define BFD_MACH_O_N_INDR  0x0a	/* Indirect.  */
#define BFD_MACH_O_N_PBUD  0x0c /* Prebound undefined (defined in a dylib).  */
#define BFD_MACH_O_N_SECT  0x0e	/* Defined in section number n_sect.  */

#define BFD_MACH_O_NO_SECT 0	/* Symbol not in any section of the image.  */

/* Symbol n_desc reference flags.  */
#define BFD_MACH_O_REFERENCE_MASK 				0x0f
#define BFD_MACH_O_REFERENCE_FLAG_UNDEFINED_NON_LAZY		0x00
#define BFD_MACH_O_REFERENCE_FLAG_UNDEFINED_LAZY		0x01
#define BFD_MACH_O_REFERENCE_FLAG_DEFINED			0x02
#define BFD_MACH_O_REFERENCE_FLAG_PRIVATE_DEFINED		0x03
#define BFD_MACH_O_REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY	0x04
#define BFD_MACH_O_REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY	0x05

#define BFD_MACH_O_REFERENCED_DYNAMICALLY			0x10
#define BFD_MACH_O_N_DESC_DISCARDED				0x20
#define BFD_MACH_O_N_NO_DEAD_STRIP				0x20
#define BFD_MACH_O_N_WEAK_REF					0x40
#define BFD_MACH_O_N_WEAK_DEF					0x80

typedef enum bfd_mach_o_mach_header_magic
{
  BFD_MACH_O_MH_MAGIC    = 0xfeedface,
  BFD_MACH_O_MH_CIGAM    = 0xcefaedfe,
  BFD_MACH_O_MH_MAGIC_64 = 0xfeedfacf,
  BFD_MACH_O_MH_CIGAM_64 = 0xcffaedfe
}
bfd_mach_o_mach_header_magic;

typedef enum bfd_mach_o_ppc_thread_flavour
{
  BFD_MACH_O_PPC_THREAD_STATE      = 1,
  BFD_MACH_O_PPC_FLOAT_STATE       = 2,
  BFD_MACH_O_PPC_EXCEPTION_STATE   = 3,
  BFD_MACH_O_PPC_VECTOR_STATE      = 4,
  BFD_MACH_O_PPC_THREAD_STATE64    = 5,
  BFD_MACH_O_PPC_EXCEPTION_STATE64 = 6,
  BFD_MACH_O_PPC_THREAD_STATE_NONE = 7
}
bfd_mach_o_ppc_thread_flavour;

/* Defined in <mach/i386/thread_status.h> */
typedef enum bfd_mach_o_i386_thread_flavour
{
  BFD_MACH_O_x86_THREAD_STATE32    = 1,
  BFD_MACH_O_x86_FLOAT_STATE32     = 2,
  BFD_MACH_O_x86_EXCEPTION_STATE32 = 3,
  BFD_MACH_O_x86_THREAD_STATE64    = 4,
  BFD_MACH_O_x86_FLOAT_STATE64     = 5,
  BFD_MACH_O_x86_EXCEPTION_STATE64 = 6,
  BFD_MACH_O_x86_THREAD_STATE      = 7,
  BFD_MACH_O_x86_FLOAT_STATE       = 8,
  BFD_MACH_O_x86_EXCEPTION_STATE   = 9,
  BFD_MACH_O_x86_DEBUG_STATE32     = 10,
  BFD_MACH_O_x86_DEBUG_STATE64     = 11,
  BFD_MACH_O_x86_DEBUG_STATE       = 12,
  BFD_MACH_O_x86_THREAD_STATE_NONE = 13
}
bfd_mach_o_i386_thread_flavour;

#define BFD_MACH_O_LC_REQ_DYLD 0x80000000

typedef enum bfd_mach_o_load_command_type
{
  BFD_MACH_O_LC_SEGMENT = 0x1,		/* File segment to be mapped.  */
  BFD_MACH_O_LC_SYMTAB = 0x2,		/* Link-edit stab symbol table info (obsolete).  */
  BFD_MACH_O_LC_SYMSEG = 0x3,		/* Link-edit gdb symbol table info.  */
  BFD_MACH_O_LC_THREAD = 0x4,		/* Thread.  */
  BFD_MACH_O_LC_UNIXTHREAD = 0x5,	/* UNIX thread (includes a stack).  */
  BFD_MACH_O_LC_LOADFVMLIB = 0x6,	/* Load a fixed VM shared library.  */
  BFD_MACH_O_LC_IDFVMLIB = 0x7,		/* Fixed VM shared library id.  */
  BFD_MACH_O_LC_IDENT = 0x8,		/* Object identification information (obsolete).  */
  BFD_MACH_O_LC_FVMFILE = 0x9,		/* Fixed VM file inclusion.  */
  BFD_MACH_O_LC_PREPAGE = 0xa,		/* Prepage command (internal use).  */
  BFD_MACH_O_LC_DYSYMTAB = 0xb,		/* Dynamic link-edit symbol table info.  */
  BFD_MACH_O_LC_LOAD_DYLIB = 0xc,	/* Load a dynamically linked shared library.  */
  BFD_MACH_O_LC_ID_DYLIB = 0xd,		/* Dynamically linked shared lib identification.  */
  BFD_MACH_O_LC_LOAD_DYLINKER = 0xe,	/* Load a dynamic linker.  */
  BFD_MACH_O_LC_ID_DYLINKER = 0xf,	/* Dynamic linker identification.  */
  BFD_MACH_O_LC_PREBOUND_DYLIB = 0x10,	/* Modules prebound for a dynamically.  */
  BFD_MACH_O_LC_ROUTINES = 0x11,	/* Image routines.  */
  BFD_MACH_O_LC_SUB_FRAMEWORK = 0x12,	/* Sub framework.  */
  BFD_MACH_O_LC_SUB_UMBRELLA = 0x13,	/* Sub umbrella.  */
  BFD_MACH_O_LC_SUB_CLIENT = 0x14,	/* Sub client.  */
  BFD_MACH_O_LC_SUB_LIBRARY = 0x15,   	/* Sub library.  */
  BFD_MACH_O_LC_TWOLEVEL_HINTS = 0x16,	/* Two-level namespace lookup hints.  */
  BFD_MACH_O_LC_PREBIND_CKSUM = 0x17, 	/* Prebind checksum.  */
  /* Load a dynamically linked shared library that is allowed to be
       missing (weak).  */
  BFD_MACH_O_LC_LOAD_WEAK_DYLIB = 0x18,
  BFD_MACH_O_LC_SEGMENT_64 = 0x19,	/* 64-bit segment of this file to be 
                                           mapped.  */
  BFD_MACH_O_LC_ROUTINES_64 = 0x1a,     /* Address of the dyld init routine 
                                           in a dylib.  */
  BFD_MACH_O_LC_UUID = 0x1b,            /* 128-bit UUID of the executable.  */
  BFD_MACH_O_LC_RPATH = 0x1c,		/* Run path addiions.  */
  BFD_MACH_O_LC_CODE_SIGNATURE = 0x1d,	/* Local of code signature.  */
  BFD_MACH_O_LC_SEGMENT_SPLIT_INFO = 0x1e, /* Local of info to split seg.  */
  BFD_MACH_O_LC_REEXPORT_DYLIB = 0x1f,  /* Load and re-export lib.  */
  BFD_MACH_O_LC_LAZY_LOAD_DYLIB = 0x20, /* Delay load of lib until use.  */
  BFD_MACH_O_LC_ENCRYPTION_INFO = 0x21, /* Encrypted segment info.  */
  BFD_MACH_O_LC_DYLD_INFO = 0x22	/* Compressed dyld information.  */
}
bfd_mach_o_load_command_type;

#define BFD_MACH_O_CPU_IS64BIT 0x1000000

typedef enum bfd_mach_o_cpu_type
{
  BFD_MACH_O_CPU_TYPE_VAX = 1,
  BFD_MACH_O_CPU_TYPE_MC680x0 = 6,
  BFD_MACH_O_CPU_TYPE_I386 = 7,
  BFD_MACH_O_CPU_TYPE_MIPS = 8,
  BFD_MACH_O_CPU_TYPE_MC98000 = 10,
  BFD_MACH_O_CPU_TYPE_HPPA = 11,
  BFD_MACH_O_CPU_TYPE_ARM = 12,
  BFD_MACH_O_CPU_TYPE_MC88000 = 13,
  BFD_MACH_O_CPU_TYPE_SPARC = 14,
  BFD_MACH_O_CPU_TYPE_I860 = 15,
  BFD_MACH_O_CPU_TYPE_ALPHA = 16,
  BFD_MACH_O_CPU_TYPE_POWERPC = 18,
  BFD_MACH_O_CPU_TYPE_POWERPC_64 = (BFD_MACH_O_CPU_TYPE_POWERPC | BFD_MACH_O_CPU_IS64BIT),
  BFD_MACH_O_CPU_TYPE_X86_64 = (BFD_MACH_O_CPU_TYPE_I386 | BFD_MACH_O_CPU_IS64BIT)
}
bfd_mach_o_cpu_type;

typedef enum bfd_mach_o_cpu_subtype
{
  BFD_MACH_O_CPU_SUBTYPE_X86_ALL = 3
}
bfd_mach_o_cpu_subtype;

typedef enum bfd_mach_o_filetype
{
  BFD_MACH_O_MH_OBJECT      = 0x01,
  BFD_MACH_O_MH_EXECUTE     = 0x02,
  BFD_MACH_O_MH_FVMLIB      = 0x03,
  BFD_MACH_O_MH_CORE        = 0x04,
  BFD_MACH_O_MH_PRELOAD     = 0x05,
  BFD_MACH_O_MH_DYLIB       = 0x06,
  BFD_MACH_O_MH_DYLINKER    = 0x07,
  BFD_MACH_O_MH_BUNDLE      = 0x08,
  BFD_MACH_O_MH_DYLIB_STUB  = 0x09,
  BFD_MACH_O_MH_DSYM        = 0x0a,
  BFD_MACH_O_MH_KEXT_BUNDLE = 0x0b
}
bfd_mach_o_filetype;

typedef enum bfd_mach_o_header_flags
{
  BFD_MACH_O_MH_NOUNDEFS		= 0x000001,
  BFD_MACH_O_MH_INCRLINK		= 0x000002,
  BFD_MACH_O_MH_DYLDLINK		= 0x000004,
  BFD_MACH_O_MH_BINDATLOAD		= 0x000008,
  BFD_MACH_O_MH_PREBOUND		= 0x000010,
  BFD_MACH_O_MH_SPLIT_SEGS		= 0x000020,
  BFD_MACH_O_MH_LAZY_INIT		= 0x000040,
  BFD_MACH_O_MH_TWOLEVEL		= 0x000080,
  BFD_MACH_O_MH_FORCE_FLAT		= 0x000100,
  BFD_MACH_O_MH_NOMULTIDEFS		= 0x000200,
  BFD_MACH_O_MH_NOFIXPREBINDING		= 0x000400,
  BFD_MACH_O_MH_PREBINDABLE		= 0x000800,
  BFD_MACH_O_MH_ALLMODSBOUND		= 0x001000,
  BFD_MACH_O_MH_SUBSECTIONS_VIA_SYMBOLS = 0x002000,
  BFD_MACH_O_MH_CANONICAL		= 0x004000,
  BFD_MACH_O_MH_WEAK_DEFINES		= 0x008000,
  BFD_MACH_O_MH_BINDS_TO_WEAK		= 0x010000,
  BFD_MACH_O_MH_ALLOW_STACK_EXECUTION	= 0x020000,
  BFD_MACH_O_MH_ROOT_SAFE		= 0x040000,
  BFD_MACH_O_MH_SETUID_SAFE		= 0x080000,
  BFD_MACH_O_MH_NO_REEXPORTED_DYLIBS	= 0x100000,
  BFD_MACH_O_MH_PIE			= 0x200000
}
bfd_mach_o_header_flags;

/* Constants for the type of a section.  */

typedef enum bfd_mach_o_section_type
{
  /* Regular section.  */
  BFD_MACH_O_S_REGULAR = 0x0,

  /* Zero fill on demand section.  */
  BFD_MACH_O_S_ZEROFILL = 0x1,

  /* Section with only literal C strings.  */
  BFD_MACH_O_S_CSTRING_LITERALS = 0x2,

  /* Section with only 4 byte literals.  */
  BFD_MACH_O_S_4BYTE_LITERALS = 0x3,

  /* Section with only 8 byte literals.  */
  BFD_MACH_O_S_8BYTE_LITERALS = 0x4,

  /* Section with only pointers to literals.  */
  BFD_MACH_O_S_LITERAL_POINTERS = 0x5,

  /* For the two types of symbol pointers sections and the symbol stubs
     section they have indirect symbol table entries.  For each of the
     entries in the section the indirect symbol table entries, in
     corresponding order in the indirect symbol table, start at the index
     stored in the reserved1 field of the section structure.  Since the
     indirect symbol table entries correspond to the entries in the
     section the number of indirect symbol table entries is inferred from
     the size of the section divided by the size of the entries in the
     section.  For symbol pointers sections the size of the entries in
     the section is 4 bytes and for symbol stubs sections the byte size
     of the stubs is stored in the reserved2 field of the section
     structure.  */

  /* Section with only non-lazy symbol pointers.  */
  BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS = 0x6,

  /* Section with only lazy symbol pointers.  */
  BFD_MACH_O_S_LAZY_SYMBOL_POINTERS = 0x7,

  /* Section with only symbol stubs, byte size of stub in the reserved2
     field.  */
  BFD_MACH_O_S_SYMBOL_STUBS = 0x8,

  /* Section with only function pointers for initialization.  */
  BFD_MACH_O_S_MOD_INIT_FUNC_POINTERS = 0x9,

  /* Section with only function pointers for termination.  */
  BFD_MACH_O_S_MOD_FINI_FUNC_POINTERS = 0xa,

  /* Section contains symbols that are coalesced by the linkers.  */
  BFD_MACH_O_S_COALESCED = 0xb,

  /* Zero fill on demand section (possibly larger than 4 GB).  */
  BFD_MACH_O_S_GB_ZEROFILL = 0xc,

  /* Section with only pairs of function pointers for interposing.  */
  BFD_MACH_O_S_INTERPOSING = 0xd,

  /* Section with only 16 byte literals.  */
  BFD_MACH_O_S_16BYTE_LITERALS = 0xe,

  /* Section contains DTrace Object Format.  */
  BFD_MACH_O_S_DTRACE_DOF = 0xf,

  /* Section with only lazy symbol pointers to lazy loaded dylibs.  */
  BFD_MACH_O_S_LAZY_DYLIB_SYMBOL_POINTERS = 0x10
}
bfd_mach_o_section_type;

/* The flags field of a section structure is separated into two parts a section
   type and section attributes.  The section types are mutually exclusive (it
   can only have one type) but the section attributes are not (it may have more
   than one attribute).  */

#define BFD_MACH_O_SECTION_TYPE_MASK        0x000000ff

/* Constants for the section attributes part of the flags field of a section
   structure.  */
#define BFD_MACH_O_SECTION_ATTRIBUTES_MASK  0xffffff00
/* System setable attributes.  */
#define BFD_MACH_O_SECTION_ATTRIBUTES_SYS   0x00ffff00
/* User attributes.  */   
#define BFD_MACH_O_SECTION_ATTRIBUTES_USR   0xff000000

typedef enum bfd_mach_o_section_attribute
{
  /* Section has local relocation entries.  */
  BFD_MACH_O_S_ATTR_LOC_RELOC         = 0x00000100,

  /* Section has external relocation entries.  */  
  BFD_MACH_O_S_ATTR_EXT_RELOC         = 0x00000200,

  /* Section contains some machine instructions.  */
  BFD_MACH_O_S_ATTR_SOME_INSTRUCTIONS = 0x00000400,

  /* A debug section.  */
  BFD_MACH_O_S_ATTR_DEBUG             = 0x02000000,

  /* Used with i386 stubs.  */
  BFD_MACH_O_S_SELF_MODIFYING_CODE    = 0x04000000,
  
  /* Blocks are live if they reference live blocks.  */
  BFD_MACH_O_S_ATTR_LIVE_SUPPORT      = 0x08000000,

  /* No dead stripping.  */
  BFD_MACH_O_S_ATTR_NO_DEAD_STRIP     = 0x10000000,

  /* Section symbols can be stripped in files with MH_DYLDLINK flag.  */
  BFD_MACH_O_S_ATTR_STRIP_STATIC_SYMS = 0x20000000,

  /* Section contains coalesced symbols that are not to be in the TOC of an
     archive.  */
  BFD_MACH_O_S_ATTR_NO_TOC            = 0x40000000,

  /* Section contains only true machine instructions.  */
  BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS = 0x80000000
}
bfd_mach_o_section_attribute;

typedef struct bfd_mach_o_header
{
  unsigned long magic;
  unsigned long cputype;
  unsigned long cpusubtype;
  unsigned long filetype;
  unsigned long ncmds;
  unsigned long sizeofcmds;
  unsigned long flags;
  unsigned int reserved;
  /* Version 1: 32 bits, version 2: 64 bits.  */
  unsigned int version;
  enum bfd_endian byteorder;
}
bfd_mach_o_header;

#define BFD_MACH_O_HEADER_SIZE 28
#define BFD_MACH_O_HEADER_64_SIZE 32

typedef struct bfd_mach_o_section
{
  asection *bfdsection;
  char sectname[16 + 1];
  char segname[16 + 1];
  bfd_vma addr;
  bfd_vma size;
  bfd_vma offset;
  unsigned long align;
  bfd_vma reloff;
  unsigned long nreloc;
  unsigned long flags;
  unsigned long reserved1;
  unsigned long reserved2;
  unsigned long reserved3;
}
bfd_mach_o_section;
#define BFD_MACH_O_SECTION_SIZE 68
#define BFD_MACH_O_SECTION_64_SIZE 80

typedef struct bfd_mach_o_segment_command
{
  char segname[16 + 1];
  bfd_vma vmaddr;
  bfd_vma vmsize;
  bfd_vma fileoff;
  unsigned long filesize;
  unsigned long maxprot;	/* Maximum permitted protection.  */
  unsigned long initprot;	/* Initial protection.  */
  unsigned long nsects;
  unsigned long flags;
  bfd_mach_o_section *sections;
}
bfd_mach_o_segment_command;
#define BFD_MACH_O_LC_SEGMENT_SIZE 56
#define BFD_MACH_O_LC_SEGMENT_64_SIZE 72

/* Protection flags.  */
#define BFD_MACH_O_PROT_READ    0x01
#define BFD_MACH_O_PROT_WRITE   0x02
#define BFD_MACH_O_PROT_EXECUTE 0x04

/* Generic relocation types (used by i386).  */
#define BFD_MACH_O_GENERIC_RELOC_VANILLA 	0
#define BFD_MACH_O_GENERIC_RELOC_PAIR	 	1
#define BFD_MACH_O_GENERIC_RELOC_SECTDIFF	2
#define BFD_MACH_O_GENERIC_RELOC_PB_LA_PTR	3
#define BFD_MACH_O_GENERIC_RELOC_LOCAL_SECTDIFF	4

/* X86-64 relocations.  */
#define BFD_MACH_O_X86_64_RELOC_UNSIGNED   0 /* Absolute addresses.  */
#define BFD_MACH_O_X86_64_RELOC_SIGNED     1 /* 32-bit disp.  */
#define BFD_MACH_O_X86_64_RELOC_BRANCH     2 /* 32-bit pcrel disp.  */
#define BFD_MACH_O_X86_64_RELOC_GOT_LOAD   3 /* Movq load of a GOT entry.  */
#define BFD_MACH_O_X86_64_RELOC_GOT        4 /* GOT reference.  */
#define BFD_MACH_O_X86_64_RELOC_SUBTRACTOR 5 /* Symbol difference.  */
#define BFD_MACH_O_X86_64_RELOC_SIGNED_1   6 /* 32-bit signed disp -1.  */
#define BFD_MACH_O_X86_64_RELOC_SIGNED_2   7 /* 32-bit signed disp -2.  */
#define BFD_MACH_O_X86_64_RELOC_SIGNED_4   8 /* 32-bit signed disp -4.  */

/* Size of a relocation entry.  */
#define BFD_MACH_O_RELENT_SIZE 8

/* Fields for a normal (non-scattered) entry.  */
#define BFD_MACH_O_R_PCREL		0x01000000
#define BFD_MACH_O_GET_R_LENGTH(s)	(((s) >> 25) & 0x3)
#define BFD_MACH_O_R_EXTERN		0x08000000
#define BFD_MACH_O_GET_R_TYPE(s)	(((s) >> 28) & 0x0f)
#define BFD_MACH_O_GET_R_SYMBOLNUM(s)	((s) & 0x00ffffff)
#define BFD_MACH_O_SET_R_LENGTH(l)	(((l) & 0x3) << 25)
#define BFD_MACH_O_SET_R_TYPE(t)	(((t) & 0xf) << 28)
#define BFD_MACH_O_SET_R_SYMBOLNUM(s)	((s) & 0x00ffffff)

/* Fields for a scattered entry.  */
#define BFD_MACH_O_SR_SCATTERED		0x80000000
#define BFD_MACH_O_SR_PCREL		0x40000000
#define BFD_MACH_O_GET_SR_LENGTH(s)	(((s) >> 28) & 0x3)
#define BFD_MACH_O_GET_SR_TYPE(s)	(((s) >> 24) & 0x0f)
#define BFD_MACH_O_GET_SR_ADDRESS(s)	((s) & 0x00ffffff)
#define BFD_MACH_O_SET_SR_LENGTH(l)	(((l) & 0x3) << 28)
#define BFD_MACH_O_SET_SR_TYPE(t)	(((t) & 0xf) << 24)
#define BFD_MACH_O_SET_SR_ADDRESS(s)	((s) & 0x00ffffff)

/* Expanded internal representation of a relocation entry.  */
typedef struct bfd_mach_o_reloc_info
{
  bfd_vma r_address;
  bfd_vma r_value;
  unsigned int r_scattered : 1;
  unsigned int r_type : 4;
  unsigned int r_pcrel : 1;
  unsigned int r_length : 2;
  unsigned int r_extern : 1;
}
bfd_mach_o_reloc_info;

typedef struct bfd_mach_o_asymbol
{
  /* The actual symbol which the rest of BFD works with.  */
  asymbol symbol;

  /* Fields from Mach-O symbol.  */
  unsigned char n_type;
  unsigned char n_sect;
  unsigned short n_desc;
}
bfd_mach_o_asymbol;
#define BFD_MACH_O_NLIST_SIZE 12
#define BFD_MACH_O_NLIST_64_SIZE 16

typedef struct bfd_mach_o_symtab_command
{
  unsigned int symoff;
  unsigned int nsyms;
  unsigned int stroff;
  unsigned int strsize;
  bfd_mach_o_asymbol *symbols;
  char *strtab;
}
bfd_mach_o_symtab_command;

/* This is the second set of the symbolic information which is used to support
   the data structures for the dynamically link editor.

   The original set of symbolic information in the symtab_command which contains
   the symbol and string tables must also be present when this load command is
   present.  When this load command is present the symbol table is organized
   into three groups of symbols:
       local symbols (static and debugging symbols) - grouped by module
       defined external symbols - grouped by module (sorted by name if not lib)
       undefined external symbols (sorted by name)
   In this load command there are offsets and counts to each of the three groups
   of symbols.

   This load command contains a the offsets and sizes of the following new
   symbolic information tables:
       table of contents
       module table
       reference symbol table
       indirect symbol table
   The first three tables above (the table of contents, module table and
   reference symbol table) are only present if the file is a dynamically linked
   shared library.  For executable and object modules, which are files
   containing only one module, the information that would be in these three
   tables is determined as follows:
       table of contents - the defined external symbols are sorted by name
       module table - the file contains only one module so everything in the
                      file is part of the module.
       reference symbol table - is the defined and undefined external symbols

   For dynamically linked shared library files this load command also contains
   offsets and sizes to the pool of relocation entries for all sections
   separated into two groups:
       external relocation entries
       local relocation entries
   For executable and object modules the relocation entries continue to hang
   off the section structures.  */

typedef struct bfd_mach_o_dylib_module
{
  /* Index into the string table indicating the name of the module.  */
  unsigned long module_name_idx;
  char *module_name;

  /* Index into the symbol table of the first defined external symbol provided
     by the module.  */
  unsigned long iextdefsym;

  /* Number of external symbols provided by this module.  */
  unsigned long nextdefsym;

  /* Index into the external reference table of the first entry
     provided by this module.  */
  unsigned long irefsym;

  /* Number of external reference entries provided by this module.  */
  unsigned long nrefsym;

  /* Index into the symbol table of the first local symbol provided by this
     module.  */
  unsigned long ilocalsym;

  /* Number of local symbols provided by this module.  */
  unsigned long nlocalsym;

  /* Index into the external relocation table of the first entry provided
     by this module.  */
  unsigned long iextrel;

  /* Number of external relocation entries provided by this module.  */
  unsigned long nextrel;

  /* Index in the module initialization section to the pointers for this
     module.  */
  unsigned short iinit;

  /* Index in the module termination section to the pointers for this
     module.  */
  unsigned short iterm;

  /* Number of pointers in the module initialization for this module.  */
  unsigned short ninit;

  /* Number of pointers in the module termination for this module.  */
  unsigned short nterm;

  /* Number of data byte for this module that are used in the __module_info
     section of the __OBJC segment.  */
  unsigned long objc_module_info_size;

  /* Statically linked address of the start of the data for this module
     in the __module_info section of the __OBJC_segment.  */
  bfd_vma objc_module_info_addr;
}
bfd_mach_o_dylib_module;
#define BFD_MACH_O_DYLIB_MODULE_SIZE 52
#define BFD_MACH_O_DYLIB_MODULE_64_SIZE 56

typedef struct bfd_mach_o_dylib_table_of_content
{
  /* Index into the symbol table to the defined external symbol.  */
  unsigned long symbol_index;

  /* Index into the module table to the module for this entry.  */
  unsigned long module_index;
}
bfd_mach_o_dylib_table_of_content;
#define BFD_MACH_O_TABLE_OF_CONTENT_SIZE 8

typedef struct bfd_mach_o_dylib_reference
{
  /* Index into the symbol table for the symbol being referenced.  */
  unsigned long isym;

  /* Type of the reference being made (use REFERENCE_FLAGS constants).  */
  unsigned long flags;
}
bfd_mach_o_dylib_reference;
#define BFD_MACH_O_REFERENCE_SIZE 4

typedef struct bfd_mach_o_dysymtab_command
{
  /* The symbols indicated by symoff and nsyms of the LC_SYMTAB load command
     are grouped into the following three groups:
       local symbols (further grouped by the module they are from)
       defined external symbols (further grouped by the module they are from)
       undefined symbols

     The local symbols are used only for debugging.  The dynamic binding
     process may have to use them to indicate to the debugger the local
     symbols for a module that is being bound.

     The last two groups are used by the dynamic binding process to do the
     binding (indirectly through the module table and the reference symbol
     table when this is a dynamically linked shared library file).  */

  unsigned long ilocalsym;    /* Index to local symbols.  */
  unsigned long nlocalsym;    /* Number of local symbols.  */
  unsigned long iextdefsym;   /* Index to externally defined symbols.  */
  unsigned long nextdefsym;   /* Number of externally defined symbols.  */
  unsigned long iundefsym;    /* Index to undefined symbols.  */
  unsigned long nundefsym;    /* Number of undefined symbols.  */

  /* For the for the dynamic binding process to find which module a symbol
     is defined in the table of contents is used (analogous to the ranlib
     structure in an archive) which maps defined external symbols to modules
     they are defined in.  This exists only in a dynamically linked shared
     library file.  For executable and object modules the defined external
     symbols are sorted by name and is use as the table of contents.  */

  unsigned long tocoff;       /* File offset to table of contents.  */
  unsigned long ntoc;         /* Number of entries in table of contents.  */

  /* To support dynamic binding of "modules" (whole object files) the symbol
     table must reflect the modules that the file was created from.  This is
     done by having a module table that has indexes and counts into the merged
     tables for each module.  The module structure that these two entries
     refer to is described below.  This exists only in a dynamically linked
     shared library file.  For executable and object modules the file only
     contains one module so everything in the file belongs to the module.  */

  unsigned long modtaboff;    /* File offset to module table.  */
  unsigned long nmodtab;      /* Number of module table entries.  */

  /* To support dynamic module binding the module structure for each module
     indicates the external references (defined and undefined) each module
     makes.  For each module there is an offset and a count into the
     reference symbol table for the symbols that the module references.
     This exists only in a dynamically linked shared library file.  For
     executable and object modules the defined external symbols and the
     undefined external symbols indicates the external references.  */

  unsigned long extrefsymoff;  /* Offset to referenced symbol table.  */
  unsigned long nextrefsyms;   /* Number of referenced symbol table entries.  */

  /* The sections that contain "symbol pointers" and "routine stubs" have
     indexes and (implied counts based on the size of the section and fixed
     size of the entry) into the "indirect symbol" table for each pointer
     and stub.  For every section of these two types the index into the
     indirect symbol table is stored in the section header in the field
     reserved1.  An indirect symbol table entry is simply a 32bit index into
     the symbol table to the symbol that the pointer or stub is referring to.
     The indirect symbol table is ordered to match the entries in the section.  */

  unsigned long indirectsymoff; /* File offset to the indirect symbol table.  */
  unsigned long nindirectsyms;  /* Number of indirect symbol table entries.  */

  /* To support relocating an individual module in a library file quickly the
     external relocation entries for each module in the library need to be
     accessed efficiently.  Since the relocation entries can't be accessed
     through the section headers for a library file they are separated into
     groups of local and external entries further grouped by module.  In this
     case the presents of this load command who's extreloff, nextrel,
     locreloff and nlocrel fields are non-zero indicates that the relocation
     entries of non-merged sections are not referenced through the section
     structures (and the reloff and nreloc fields in the section headers are
     set to zero).

     Since the relocation entries are not accessed through the section headers
     this requires the r_address field to be something other than a section
     offset to identify the item to be relocated.  In this case r_address is
     set to the offset from the vmaddr of the first LC_SEGMENT command.

     The relocation entries are grouped by module and the module table
     entries have indexes and counts into them for the group of external
     relocation entries for that the module.

     For sections that are merged across modules there must not be any
     remaining external relocation entries for them (for merged sections
     remaining relocation entries must be local).  */

  unsigned long extreloff;    /* Offset to external relocation entries.  */
  unsigned long nextrel;      /* Number of external relocation entries.  */

  /* All the local relocation entries are grouped together (they are not
     grouped by their module since they are only used if the object is moved
     from it statically link edited address).  */

  unsigned long locreloff;    /* Offset to local relocation entries.  */
  unsigned long nlocrel;      /* Number of local relocation entries.  */

  bfd_mach_o_dylib_module *dylib_module;
  bfd_mach_o_dylib_table_of_content *dylib_toc;
  unsigned int *indirect_syms;
  bfd_mach_o_dylib_reference *ext_refs;
}
bfd_mach_o_dysymtab_command;

/* An indirect symbol table entry is simply a 32bit index into the symbol table
   to the symbol that the pointer or stub is refering to.  Unless it is for a
   non-lazy symbol pointer section for a defined symbol which strip(1) has
   removed.  In which case it has the value INDIRECT_SYMBOL_LOCAL.  If the
   symbol was also absolute INDIRECT_SYMBOL_ABS is or'ed with that.  */

#define BFD_MACH_O_INDIRECT_SYMBOL_LOCAL 0x80000000
#define BFD_MACH_O_INDIRECT_SYMBOL_ABS   0x40000000
#define BFD_MACH_O_INDIRECT_SYMBOL_SIZE  4

/* For LC_THREAD or LC_UNIXTHREAD.  */

typedef struct bfd_mach_o_thread_flavour
{
  unsigned long flavour;
  unsigned long offset;
  unsigned long size;
}
bfd_mach_o_thread_flavour;

typedef struct bfd_mach_o_thread_command
{
  unsigned long nflavours;
  bfd_mach_o_thread_flavour *flavours;
  asection *section;
}
bfd_mach_o_thread_command;

/* For LC_LOAD_DYLINKER and LC_ID_DYLINKER.  */

typedef struct bfd_mach_o_dylinker_command
{
  unsigned long name_offset;         /* Offset to library's path name.  */
  unsigned long name_len;            /* Offset to library's path name.  */
  char *name_str;
}
bfd_mach_o_dylinker_command;

/* For LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_ID_DYLIB
   or LC_REEXPORT_DYLIB.  */

typedef struct bfd_mach_o_dylib_command
{
  unsigned long name_offset;           /* Offset to library's path name.  */
  unsigned long name_len;              /* Offset to library's path name.  */
  unsigned long timestamp;	       /* Library's build time stamp.  */
  unsigned long current_version;       /* Library's current version number.  */
  unsigned long compatibility_version; /* Library's compatibility vers number.  */
  char *name_str;
}
bfd_mach_o_dylib_command;

/* For LC_PREBOUND_DYLIB.  */

typedef struct bfd_mach_o_prebound_dylib_command
{
  unsigned long name;                /* Library's path name.  */
  unsigned long nmodules;            /* Number of modules in library.  */
  unsigned long linked_modules;      /* Bit vector of linked modules.  */
}
bfd_mach_o_prebound_dylib_command;

/* For LC_UUID.  */

typedef struct bfd_mach_o_uuid_command
{
  unsigned char uuid[16];
}
bfd_mach_o_uuid_command;

/* For LC_CODE_SIGNATURE or LC_SEGMENT_SPLIT_INFO.  */

typedef struct bfd_mach_o_linkedit_command
{
  unsigned long dataoff;
  unsigned long datasize;
}
bfd_mach_o_linkedit_command;

typedef struct bfd_mach_o_str_command
{
  unsigned long stroff;
  unsigned long str_len;
  char *str;
}
bfd_mach_o_str_command;

typedef struct bfd_mach_o_dyld_info_command
{
  /* File offset and size to rebase info.  */
  unsigned int rebase_off; 
  unsigned int rebase_size;

  /* File offset and size of binding info.  */
  unsigned int bind_off;
  unsigned int bind_size;

  /* File offset and size of weak binding info.  */
  unsigned int weak_bind_off;
  unsigned int weak_bind_size;

  /* File offset and size of lazy binding info.  */
  unsigned int lazy_bind_off;
  unsigned int lazy_bind_size;

  /* File offset and size of export info.  */
  unsigned int export_off;
  unsigned int export_size;
}
bfd_mach_o_dyld_info_command;

typedef struct bfd_mach_o_load_command
{
  bfd_mach_o_load_command_type type;
  bfd_boolean type_required;
  unsigned int offset;
  unsigned int len;
  union
  {
    bfd_mach_o_segment_command segment;
    bfd_mach_o_symtab_command symtab;
    bfd_mach_o_dysymtab_command dysymtab;
    bfd_mach_o_thread_command thread;
    bfd_mach_o_dylib_command dylib;
    bfd_mach_o_dylinker_command dylinker;
    bfd_mach_o_prebound_dylib_command prebound_dylib;
    bfd_mach_o_uuid_command uuid;
    bfd_mach_o_linkedit_command linkedit;
    bfd_mach_o_str_command str;
    bfd_mach_o_dyld_info_command dyld_info;
  }
  command;
}
bfd_mach_o_load_command;

typedef struct mach_o_data_struct
{
  /* Mach-O header.  */
  bfd_mach_o_header header;
  /* Array of load commands (length is given by header.ncmds).  */
  bfd_mach_o_load_command *commands;

  /* Flatten array of sections.  The array is 0-based.  */
  unsigned long nsects;
  bfd_mach_o_section **sections;

  /* Used while writting: current length of the output file.  This is used
     to allocate space in the file.  */
  ufile_ptr filelen;

  /* As symtab is referenced by other load command, it is handy to have
     a direct access to it.  Also it is not clearly stated, only one symtab
     is expected.  */
  bfd_mach_o_symtab_command *symtab;
  bfd_mach_o_dysymtab_command *dysymtab;
}
bfd_mach_o_data_struct;

/* Target specific routines.  */
typedef struct bfd_mach_o_backend_data
{
  enum bfd_architecture arch;
  bfd_boolean (*_bfd_mach_o_swap_reloc_in)(arelent *, bfd_mach_o_reloc_info *);
  bfd_boolean (*_bfd_mach_o_swap_reloc_out)(arelent *, bfd_mach_o_reloc_info *);
  bfd_boolean (*_bfd_mach_o_print_thread)(bfd *, bfd_mach_o_thread_flavour *,
                                          void *, char *);
}
bfd_mach_o_backend_data;

#define bfd_mach_o_get_data(abfd) ((abfd)->tdata.mach_o_data)
#define bfd_mach_o_get_backend_data(abfd) \
  ((bfd_mach_o_backend_data*)(abfd)->xvec->backend_data)

bfd_boolean bfd_mach_o_valid (bfd *);
int bfd_mach_o_read_dysymtab_symbol (bfd *, bfd_mach_o_dysymtab_command *, bfd_mach_o_symtab_command *, bfd_mach_o_asymbol *, unsigned long);
int bfd_mach_o_scan_start_address (bfd *);
int bfd_mach_o_scan (bfd *, bfd_mach_o_header *, bfd_mach_o_data_struct *);
bfd_boolean bfd_mach_o_mkobject_init (bfd *);
const bfd_target *bfd_mach_o_object_p (bfd *);
const bfd_target *bfd_mach_o_core_p (bfd *);
const bfd_target *bfd_mach_o_archive_p (bfd *);
bfd *bfd_mach_o_openr_next_archived_file (bfd *, bfd *);
bfd_boolean bfd_mach_o_set_arch_mach (bfd *, enum bfd_architecture,
                                      unsigned long);
int bfd_mach_o_lookup_section (bfd *, asection *, bfd_mach_o_load_command **, bfd_mach_o_section **);
int bfd_mach_o_lookup_command (bfd *, bfd_mach_o_load_command_type, bfd_mach_o_load_command **);
bfd_boolean bfd_mach_o_write_contents (bfd *);
bfd_boolean bfd_mach_o_bfd_copy_private_symbol_data (bfd *, asymbol *,
                                                     bfd *, asymbol *);
bfd_boolean bfd_mach_o_bfd_copy_private_section_data (bfd *, asection *,
                                                      bfd *, asection *);
bfd_boolean bfd_mach_o_bfd_copy_private_bfd_data (bfd *, bfd *);
long bfd_mach_o_get_symtab_upper_bound (bfd *);
long bfd_mach_o_canonicalize_symtab (bfd *, asymbol **);
long bfd_mach_o_get_synthetic_symtab (bfd *, long, asymbol **, long, 
                                      asymbol **, asymbol **ret);
long bfd_mach_o_get_reloc_upper_bound (bfd *, asection *);
long bfd_mach_o_canonicalize_reloc (bfd *, asection *, arelent **, asymbol **);
long bfd_mach_o_get_dynamic_reloc_upper_bound (bfd *);
long bfd_mach_o_canonicalize_dynamic_reloc (bfd *, arelent **, asymbol **);
asymbol *bfd_mach_o_make_empty_symbol (bfd *);
void bfd_mach_o_get_symbol_info (bfd *, asymbol *, symbol_info *);
void bfd_mach_o_print_symbol (bfd *, PTR, asymbol *, bfd_print_symbol_type);
bfd_boolean bfd_mach_o_bfd_print_private_bfd_data (bfd *, PTR);
int bfd_mach_o_sizeof_headers (bfd *, struct bfd_link_info *);
unsigned long bfd_mach_o_stack_addr (enum bfd_mach_o_cpu_type);
int bfd_mach_o_core_fetch_environment (bfd *, unsigned char **, unsigned int *);
char *bfd_mach_o_core_file_failing_command (bfd *);
int bfd_mach_o_core_file_failing_signal (bfd *);
bfd_boolean bfd_mach_o_core_file_matches_executable_p (bfd *, bfd *);
bfd *bfd_mach_o_fat_extract (bfd *, bfd_format , const bfd_arch_info_type *);
const bfd_target *bfd_mach_o_header_p (bfd *, bfd_mach_o_filetype,
                                       bfd_mach_o_cpu_type);
bfd_boolean bfd_mach_o_build_commands (bfd *);
bfd_boolean bfd_mach_o_set_section_contents (bfd *, asection *, const void *,
                                             file_ptr, bfd_size_type);
unsigned int bfd_mach_o_version (bfd *);

extern const bfd_target mach_o_fat_vec;

#endif /* _BFD_MACH_O_H_ */
