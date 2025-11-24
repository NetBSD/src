dnl 	$NetBSD: elfconstants.m4,v 1.12 2025/11/24 18:56:54 jkoshy Exp $
# Copyright (c) 2010,2021 Joseph Koshy
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

define(`VCSID_ELFCONSTANTS_M4',
	`Id: elfconstants.m4 4272 2025-11-18 22:10:56Z jkoshy')

define(`COMPATIBILITY_NOTICE',`dnl
# These definitions are believed to be compatible with:
#
# - The ELF object file format specification at: https://gabi.xinuos.com/.
#
# - The May 1998 (version 1.5) draft of "The ELF-64 object format".
#
# - The "Linkers and Libraries Guide", from Sun Microsystems.
#
# - Processor-specific ELF ABI definitions for the aarch64, arm, i386,
#   ia_64, loongarch, mips, ppc, ppc64, riscv, s390, sparc, vax and
#   x86_64 architectures:
#
#   i386 ::
#     System V Application Binary Interface
#     Intel386 Architecture Processor Supplement Version 1.2
#     https://gitlab.com/x86-psABIs/i386-ABI/-/tree/hjl/x86/master
#
#   aarch64 ::
#     ELF for the Arm® 64-bit Architecture (AArch64)
#     https://github.com/ARM-software/abi-aa/blob/main/aaelf64/aaelf64.rst
#
#   arm ::
#     ELF for the Arm® Architecture
#     https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst
#
#   ia_64 ::
#      Intel® Itanium™ Processor-specific Application Binary Interface (ABI)
#      Document Number: 245370-003
#      http://refspecs.linux-foundation.org/elf/IA64-SysV-psABI.pdf
#
#   loongarch ::
#     ELF for the LoongArch™ Architecture
#     https://github.com/loongson/la-abi-specs/blob/release/laelf.adoc.
#
#   mips ::
#     SYSTEM V APPLICATION BINARY INTERFACE, MIPS RISC Processor Supplement,
#     3rd Edition, 1996.
#     https://refspecs.linuxfoundation.org/elf/mipsabi.pdf
#     
#   ppc ::
#     Power Architecture® 32-bit Application Binary Interface
#     Supplement 1.0 - Linux® & Embedded
#     (Archived link) https://web.archive.org/web/20120608002551/\
#       https://www.power.org/resources/downloads/\
#       Power-Arch-32-bit-ABI-supp-1.0-Unified.pdf
#
#   ppc64 ::
#     64-bit ELF ABI Specification for OpenPOWER Architecture
#     https://openpowerfoundation.org/specifications/64bitelfabi/
#
#   riscv ::
#     RISC-V ELF Specification
#     https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc
#
#   s390 ::
#     S/390 ELF Application Binary Interface Supplement
#     https://refspecs.linuxfoundation.org/ELF/zSeries/lzsabi0_zSeries.htm
#
#   sparc ::
#     Oracle Solaris Linkers and Libraries Guide
#     November 2024, Document E36783-04.
#
#   x86_64 ::
#     ELF x86-64-ABI psABI
#     https://gitlab.com/x86-psABIs/x86-64-ABI')

# In the following definitions, `_' is an M4 macro that is meant to be
# expanded later.  Its intended usage is:
#
#   `_(ELF_SYMBOL, VALUE, OPTIONAL-DESCRIPTION)'
#
# These (deferred) macros are then grouped together into named collections.
#
# At the point of use, `_' would be defined to expand to the desired
# replacement text.
#
#   # File: example_expansion.m4
#
#   define(`_',`case $2: return ("$1");')
#   include(`elfconstants.m4')
#
#   const char *name_of_capability(int capability)
#   {
#       switch(capability) {
#       DEFINE_CAPABILITIES();
#       default:
#           return (NULL);
#       }
#   }   


# The `__' macro is used to record comments.
#
# Provide a default definition for the macro that ignores its arguments.
define(`__',`')

#
# Types of capabilities.
# 
define(`DEFINE_CAPABILITIES',`
_(`CA_SUNW_NULL',	0,	`ignored')
_(`CA_SUNW_HW_1',	1,	`hardware capability')
_(`CA_SUNW_SW_1',	2,	`software capability')')

#
# Flags used with dynamic linking entries.
#
define(`DEFINE_DYN_FLAGS',`
_(DF_ORIGIN,           0x00000001U,
	`object being loaded may refer to `$ORIGIN'')
_(DF_SYMBOLIC,         0x00000002U,
	`search library for references before executable')
_(DF_TEXTREL,          0x00000004U,
	`relocation entries may modify text segment')
_(DF_BIND_NOW,         0x00000008U,
	`process relocation entries at load time')
_(DF_STATIC_TLS,       0x00000010U,
	`uses static thread-local storage')
_(DF_1_BIND_NOW,       0x00000001U,
	`process relocation entries at load time')
_(DF_1_GLOBAL,         0x00000002U,
	`unused')
_(DF_1_GROUP,          0x00000004U,
	`object is a member of a group')
_(DF_1_NODELETE,       0x00000008U,
	`object cannot be deleted from a process')
_(DF_1_LOADFLTR,       0x00000010U,
	`immediate load filtees')
_(DF_1_INITFIRST,      0x00000020U,
	`initialize object first')
_(DF_1_NOOPEN,         0x00000040U,
	`disallow dlopen()')
_(DF_1_ORIGIN,         0x00000080U,
	`object being loaded may refer to $ORIGIN')
_(DF_1_DIRECT,         0x00000100U,
	`direct bindings enabled')
_(DF_1_INTERPOSE,      0x00000400U,
	`object is interposer')
_(DF_1_NODEFLIB,       0x00000800U,
	`ignore default library search path')
_(DF_1_NODUMP,         0x00001000U,
	`disallow dldump()')
_(DF_1_CONFALT,        0x00002000U,
	`object is a configuration alternative')
_(DF_1_ENDFILTEE,      0x00004000U,
	`filtee terminates filter search')
_(DF_1_DISPRELDNE,     0x00008000U,
	`displacement relocation done')
_(DF_1_DISPRELPND,     0x00010000U,
	`displacement relocation pending')
_(DF_1_NODIRECT,       0x00020000U,
	`object contains non-direct bindings')
_(DF_1_IGNMULDEF,      0x00040000U,
	`unused')
_(DF_1_NOKSYMS,        0x00080000U,
	`unused')
_(DF_1_NOHDR,          0x00100000U,
	`unused')
_(DF_1_EDITED,         0x00200000U,
	`object has been modified')
_(DF_1_NORELOC,        0x00400000U,
	`unused')
_(DF_1_SYMINTPOSE,     0x00800000U,
	`symbol interposers exist')
_(DF_1_GLOBAUDIT,      0x01000000U,
	`global auditing')
_(DF_1_SINGLETON,      0x02000000U,
	`contains singleton symbols')
_(DF_1_STUB,           0x04000000U,
	`stub object')
_(DF_1_PIE,            0x08000000U,
	`position-independent executable')
_(DF_1_KMOD,           0x10000000U,
	`kernel module')
_(DF_1_WEAKFILTER,     0x20000000U,
	`object is a weak filter')
')

define(`DEFINE_DYN_FLAG_ALIASES',`
_(DF_1_NOW,	DF_1_BIND_NOW)
')

#
# Dynamic linking entry types.
#
define(`DEFINE_DYN_TYPES',`
_(`DT_NULL',             0,
	`end of array')
_(`DT_NEEDED',           1,
	`names a needed library')
_(`DT_PLTRELSZ',         2,
	`size in bytes of associated relocation entries')
_(`DT_PLTGOT',           3,
	`address associated with the procedure linkage table')
_(`DT_HASH',             4,
	`address of the symbol hash table')
_(`DT_STRTAB',           5,
	`address of the string table')
_(`DT_SYMTAB',           6,
	`address of the symbol table')
_(`DT_RELA',             7,
	`address of the relocation table')
_(`DT_RELASZ',           8,
	`size of the DT_RELA table')
_(`DT_RELAENT',          9,
	`size of each DT_RELA entry')
_(`DT_STRSZ',            10,
	`size of the string table')
_(`DT_SYMENT',           11,
	`size of a symbol table entry')
_(`DT_INIT',             12,
	`address of the initialization function')
_(`DT_FINI',             13,
	`address of the finalization function')
_(`DT_SONAME',           14,
	`names the shared object')
_(`DT_RPATH',            15,
	`runtime library search path')
_(`DT_SYMBOLIC',         16,
	`alter symbol resolution algorithm')
_(`DT_REL',              17,
	`address of the DT_REL table')
_(`DT_RELSZ',            18,
	`size of the DT_REL table')
_(`DT_RELENT',           19,
	`size of each DT_REL entry')
_(`DT_PLTREL',           20,
	`type of relocation entry in the procedure linkage table')
_(`DT_DEBUG',            21,
	`used for debugging')
_(`DT_TEXTREL',          22,
	`text segment may be written to during relocation')
_(`DT_JMPREL',           23,
	`address of relocation entries associated with the procedure linkage table')
_(`DT_BIND_NOW',         24,
	`bind symbols at loading time')
_(`DT_INIT_ARRAY',       25,
	`pointers to initialization functions')
_(`DT_FINI_ARRAY',       26,
	`pointers to termination functions')
_(`DT_INIT_ARRAYSZ',     27,
	`size of the DT_INIT_ARRAY')
_(`DT_FINI_ARRAYSZ',     28,
	`size of the DT_FINI_ARRAY')
_(`DT_RUNPATH',          29,
	`index of library search path string')
_(`DT_FLAGS',            30,
	`flags specific to the object being loaded')
_(`DT_ENCODING',         32,
	`standard semantics')
_(`DT_PREINIT_ARRAY',    32,
	`pointers to pre-initialization functions')
_(`DT_PREINIT_ARRAYSZ',  33,
	`size of pre-initialization array')
_(`DT_SYMTAB_SHNDX',     34,
	`the address of the SHT_SYMTAB_SHNDX section for the DT_SYMTAB entry')
_(`DT_RELRSZ',           35,
	`the total size in bytes of the DT_RELR relocation table')
_(`DT_RELR',             36,
	`The address of a table with relative relocation entries')
_(`DT_RELRENT',          37,
	`The size in bytes of a DT_RELR relocation entry')
_(`DT_SYMTABSZ',	 39,
	`The size in bytes of the DT_SYMTAB symbol table')
_(`DT_LOOS',             0x6000000D,
	`start of OS-specific types')
_(`DT_SUNW_AUXILIARY',   0x6000000D,
	`offset of string naming auxiliary filtees')
_(`DT_SUNW_RTLDINF',     0x6000000E,
	`rtld internal use')
_(`DT_SUNW_FILTER',      0x6000000F,
	`offset of string naming standard filtees')
_(`DT_SUNW_CAP',         0x60000010,
	`address of hardware capabilities section')
_(`DT_SUNW_ASLR',        0x60000023,
	`Address Space Layout Randomization flag')
_(`DT_HIOS',             0x6FFFF000,
	`end of OS-specific types')
_(`DT_VALRNGLO',         0x6FFFFD00,
	`start of range using the d_val field')
_(`DT_GNU_PRELINKED',    0x6FFFFDF5,
	`prelinking timestamp')
_(`DT_GNU_CONFLICTSZ',   0x6FFFFDF6,
	`size of conflict section')
_(`DT_GNU_LIBLISTSZ',    0x6FFFFDF7,
	`size of library list')
_(`DT_CHECKSUM',         0x6FFFFDF8,
	`checksum for the object')
_(`DT_PLTPADSZ',         0x6FFFFDF9,
	`size of PLT padding')
_(`DT_MOVEENT',          0x6FFFFDFA,
	`size of DT_MOVETAB entries')
_(`DT_MOVESZ',           0x6FFFFDFB,
	`total size of the MOVETAB table')
_(`DT_FEATURE',          0x6FFFFDFC,
	`feature values')
_(`DT_POSFLAG_1',        0x6FFFFDFD,
	`dynamic position flags')
_(`DT_SYMINSZ',          0x6FFFFDFE,
	`size of the DT_SYMINFO table')
_(`DT_SYMINENT',         0x6FFFFDFF,
	`size of a DT_SYMINFO entry')
_(`DT_VALRNGHI',         0x6FFFFDFF,
	`end of range using the d_val field')
_(`DT_ADDRRNGLO',        0x6FFFFE00,
	`start of range using the d_ptr field')
_(`DT_GNU_HASH',	       0x6FFFFEF5,
	`GNU style hash tables')
_(`DT_TLSDESC_PLT',      0x6FFFFEF6,
	`location of PLT entry for TLS descriptor resolver calls')
_(`DT_TLSDESC_GOT',      0x6FFFFEF7,
	`location of GOT entry used by TLS descriptor resolver PLT entry')
_(`DT_GNU_CONFLICT',     0x6FFFFEF8,
	`address of conflict section')
_(`DT_GNU_LIBLIST',      0x6FFFFEF9,
	`address of conflict section')
_(`DT_CONFIG',           0x6FFFFEFA,
	`configuration file')
_(`DT_DEPAUDIT',         0x6FFFFEFB,
	`string defining audit libraries')
_(`DT_AUDIT',            0x6FFFFEFC,
	`string defining audit libraries')
_(`DT_PLTPAD',           0x6FFFFEFD,
	`PLT padding')
_(`DT_MOVETAB',          0x6FFFFEFE,
	`address of a move table')
_(`DT_SYMINFO',          0x6FFFFEFF,
	`address of the symbol information table')
_(`DT_ADDRRNGHI',        0x6FFFFEFF,
	`end of range using the d_ptr field')
_(`DT_VERSYM',	       0x6FFFFFF0,
	`address of the version section')
_(`DT_RELACOUNT',        0x6FFFFFF9,
	`count of RELA relocations')
_(`DT_RELCOUNT',         0x6FFFFFFA,
	`count of REL relocations')
_(`DT_FLAGS_1',          0x6FFFFFFB,
	`flag values')
_(`DT_VERDEF',	       0x6FFFFFFC,
	`address of the version definition segment')
_(`DT_VERDEFNUM',	       0x6FFFFFFD,
	`the number of version definition entries')
_(`DT_VERNEED',	       0x6FFFFFFE,
	`address of section with needed versions')
_(`DT_VERNEEDNUM',       0x6FFFFFFF,
	`the number of version needed entries')
_(`DT_LOPROC',           0x70000000,
	`start of processor-specific types')
_(`DT_ARM_SYMTABSZ',     0x70000001,
	`number of entries in the dynamic symbol table')
_(`DT_SPARC_REGISTER',   0x70000001,
	`index of an STT_SPARC_REGISTER symbol')
_(`DT_ARM_PREEMPTMAP',   0x70000002,
	`address of the preemption map')
_(`DT_MIPS_RLD_VERSION', 0x70000001,
	`version ID for runtime linker interface')
_(`DT_MIPS_TIME_STAMP',  0x70000002,
	`timestamp')
_(`DT_MIPS_ICHECKSUM',   0x70000003,
	`checksum of all external strings and common sizes')
_(`DT_MIPS_IVERSION',    0x70000004,
	`string table index of a version string')
_(`DT_MIPS_FLAGS',       0x70000005,
	`MIPS-specific flags')
_(`DT_MIPS_BASE_ADDRESS', 0x70000006,
	`base address for the executable/DSO')
_(`DT_MIPS_CONFLICT',    0x70000008,
	`address of .conflict section')
_(`DT_MIPS_LIBLIST',     0x70000009,
	`address of .liblist section')
_(`DT_MIPS_LOCAL_GOTNO', 0x7000000A,
	`number of local GOT entries')
_(`DT_MIPS_CONFLICTNO',  0x7000000B,
	`number of entries in the .conflict section')
_(`DT_MIPS_LIBLISTNO',   0x70000010,
	`number of entries in the .liblist section')
_(`DT_MIPS_SYMTABNO',    0x70000011,
	`number of entries in the .dynsym section')
_(`DT_MIPS_UNREFEXTNO',  0x70000012,
	`index of first external dynamic symbol not referenced locally')
_(`DT_MIPS_GOTSYM',      0x70000013,
	`index of first dynamic symbol corresponds to a GOT entry')
_(`DT_MIPS_HIPAGENO',    0x70000014,
	`number of page table entries in GOT')
_(`DT_MIPS_RLD_MAP',     0x70000016,
	`address of runtime linker map')
_(`DT_MIPS_DELTA_CLASS', 0x70000017,
	`Delta C++ class definition')
_(`DT_MIPS_DELTA_CLASS_NO', 0x70000018,
	`number of entries in DT_MIPS_DELTA_CLASS')
_(`DT_MIPS_DELTA_INSTANCE', 0x70000019,
	`Delta C++ class instances')
_(`DT_MIPS_DELTA_INSTANCE_NO', 0x7000001A,
	`number of entries in DT_MIPS_DELTA_INSTANCE')
_(`DT_MIPS_DELTA_RELOC', 0x7000001B,
	`Delta relocations')
_(`DT_MIPS_DELTA_RELOC_NO', 0x7000001C,
	`number of entries in DT_MIPS_DELTA_RELOC')
_(`DT_MIPS_DELTA_SYM',   0x7000001D,
	`Delta symbols referred by Delta relocations')
_(`DT_MIPS_DELTA_SYM_NO', 0x7000001E,
	`number of entries in DT_MIPS_DELTA_SYM')
_(`DT_MIPS_DELTA_CLASSSYM', 0x70000020,
	`Delta symbols for class declarations')
_(`DT_MIPS_DELTA_CLASSSYM_NO', 0x70000021,
	`number of entries in DT_MIPS_DELTA_CLASSSYM')
_(`DT_MIPS_CXX_FLAGS',   0x70000022,
	`C++ flavor flags')
_(`DT_MIPS_PIXIE_INIT',  0x70000023,
	`address of an initialization routine created by pixie')
_(`DT_MIPS_SYMBOL_LIB',  0x70000024,
	`address of .MIPS.symlib section')
_(`DT_MIPS_LOCALPAGE_GOTIDX', 0x70000025,
	`GOT index of first page table entry for a segment')
_(`DT_MIPS_LOCAL_GOTIDX', 0x70000026,
	`GOT index of first page table entry for a local symbol')
_(`DT_MIPS_HIDDEN_GOTIDX', 0x70000027,
	`GOT index of first page table entry for a hidden symbol')
_(`DT_MIPS_PROTECTED_GOTIDX', 0x70000028,
	`GOT index of first page table entry for a protected symbol')
_(`DT_MIPS_OPTIONS',     0x70000029,
	`address of .MIPS.options section')
_(`DT_MIPS_INTERFACE',   0x7000002A,
	`address of .MIPS.interface section')
_(`DT_MIPS_DYNSTR_ALIGN', 0x7000002B,
	`???')
_(`DT_MIPS_INTERFACE_SIZE', 0x7000002C,
	`size of .MIPS.interface section')
_(`DT_MIPS_RLD_TEXT_RESOLVE_ADDR', 0x7000002D,
	`address of _rld_text_resolve in GOT')
_(`DT_MIPS_PERF_SUFFIX', 0x7000002E,
	`default suffix of DSO to be appended by dlopen')
_(`DT_MIPS_COMPACT_SIZE', 0x7000002F,
	`size of a ucode compact relocation record (o32)')
_(`DT_MIPS_GP_VALUE',    0x70000030,
	`GP value of a specified GP relative range')
_(`DT_MIPS_AUX_DYNAMIC', 0x70000031,
	`address of an auxiliary dynamic table')
_(`DT_MIPS_PLTGOT',      0x70000032,
	`address of the PLTGOT')
_(`DT_MIPS_RLD_OBJ_UPDATE', 0x70000033,
	`object list update callback')
_(`DT_MIPS_RWPLT',       0x70000034,
	`address of a writable PLT')
_(`DT_PPC_GOT',          0x70000000,
	`value of _GLOBAL_OFFSET_TABLE_')
_(`DT_PPC_TLSOPT',       0x70000001,
	`TLS descriptor should be optimized')
_(`DT_PPC64_GLINK',      0x70000000,
	`address of .glink section')
_(`DT_PPC64_OPD',        0x70000001,
	`address of .opd section')
_(`DT_PPC64_OPDSZ',      0x70000002,
	`size of .opd section')
_(`DT_PPC64_TLSOPT',     0x70000003,
	`TLS descriptor should be optimized')
_(`DT_AUXILIARY',        0x7FFFFFFD,
	`offset of string naming auxiliary filtees')
_(`DT_USED',             0x7FFFFFFE,
	`ignored')
_(`DT_FILTER',           0x7FFFFFFF,
	`index of string naming filtees')
_(`DT_HIPROC',           0x7FFFFFFF,
	`end of processor-specific types')
')

define(`DEFINE_DYN_TYPE_ALIASES',`
_(`DT_DEPRECATED_SPARC_REGISTER', `DT_SPARC_REGISTER')
')

#
# Flags used in the executable header (field: e_flags).
#
define(`DEFINE_EHDR_FLAGS_ARM',`dnl
_(EF_ARM_RELEXEC,      0x00000001U,
	`dynamic segment describes only how to relocate segments')
_(EF_ARM_HASENTRY,     0x00000002U,
	`e_entry contains a program entry point')
_(EF_ARM_SYMSARESORTED, 0x00000004U,
	`subsection of symbol table is sorted by symbol value')
_(EF_ARM_DYNSYMSUSESEGIDX, 0x00000008U,
	`dynamic symbol st_shndx = containing segment index + 1')
_(EF_ARM_MAPSYMSFIRST, 0x00000010U,
	`mapping symbols precede other local symbols in symtab')
_(EF_ARM_BE8,          0x00800000U,
	`Executable contains BE-8 code for ARMv6.')
_(EF_ARM_LE8,          0x00400000U,
	`file contains LE-8 code')
_(EF_ARM_EABI_UNKNOWN, 0x00000000U,
	`Unknown or GNU ARM EABI version number')
_(EF_ARM_EABI_VER1,    0x01000000U,
	`ARM EABI version 1')
_(EF_ARM_EABI_VER2,    0x02000000U,
	`ARM EABI version 2')
_(EF_ARM_EABI_VER3,    0x03000000U,
	`ARM EABI version 3')
_(EF_ARM_EABI_VER4,    0x04000000U,
	`ARM EABI version 4')
_(EF_ARM_EABI_VER5,    0x05000000U,
	`ARM EABI version 5')
_(EF_ARM_INTERWORK,    0x00000004U,
	`GNU EABI extension')
_(EF_ARM_APCS_26,      0x00000008U,
	`GNU EABI extension')
_(EF_ARM_APCS_FLOAT,   0x00000010U,
	`GNU EABI extension')
_(EF_ARM_PIC,          0x00000020U,
	`GNU EABI extension')
_(EF_ARM_ALIGN8,       0x00000040U,
	`GNU EABI extension')
_(EF_ARM_NEW_ABI,      0x00000080U,
	`GNU EABI extension')
_(EF_ARM_OLD_ABI,      0x00000100U,
	`GNU EABI extension')
_(EF_ARM_ABI_FLOAT_SOFT,   0x00000200U,
	`Object uses the software floating point procedure call standard.')
_(EF_ARM_ABI_FLOAT_HARD,   0x00000400U,
	`Object uses the hardware floating point procedure call standard.')
_(EF_ARM_MAVERICK_FLOAT, 0x00000800U,
	`GNU EABI extension')
')
define(`DEFINE_EHDR_FLAG_MASKS_ARM',`dnl
_(EF_ARM_EABIMASK,     0xFF000000U,
	`mask for ARM EABI version number (0 denotes GNU or unknown)')
_(EF_ARM_GCCMASK,	0x00400FFFU,
	`Legacy code generated by GCC may use these bits')
')
define(`DEFINE_EHDR_FLAG_SYNONYMS_ARM',`dnl
_(EF_ARM_VFP_FLOAT,	0x00000400U,
	`GNU spelling, see EF_ARM_ABI_FLOAT_HARD.')
_(EF_ARM_SOFT_FLOAT,	0x00000200U,
	`GNU spelling, see EF_ARM_FLOAT_SOFT.')
')

define(`DEFINE_EHDR_FLAGS_IA_64',`dnl
_(EF_IA_64_ABI64,	0x00000010U,
	`Object uses the LP64 programming model.')
_(EF_IA_64_REDUCEDFP,	0x00000020U,
	`Object has been compiled with a reduced floating-point model.')
_(EF_IA_64_CONS_GP,	0x00000040U,
	`The global pointer is constant except for indirect function calls.')
_(EF_IA_64_NOFUNCDESC_CONS_GP,	0x00000080U,
	`The global pointer is a program-wide constant.')
_(EF_IA_64_ABSOLUTE,	0x00000100U,
	`The program headers specify the load address.')
')
define(`DEFINE_EHDR_FLAG_MASKS_IA_64',`dnl
_(EF_IA_64_MASKOS,	0x00FF000FU,
	`Bits reserved for OS-specific flags.')
_(EF_IA_64_ARCH,	0xFF000000U,
	`These bits record the minimum architecture level required.')
')

define(`DEFINE_EHDR_FLAGS_LOONGARCH',`dnl
_(EF_LOONGARCH_ABI_SOFT_FLOAT,     0x00000001U,
	`LoongArch software floating point emulation')
_(EF_LOONGARCH_ABI_SINGLE_FLOAT,   0x00000002U,
	`LoongArch 32-bit floating point registers')
_(EF_LOONGARCH_ABI_DOUBLE_FLOAT,   0x00000003U,
	`LoongArch 64-bit floating point registers')
_(EF_LOONGARCH_OBJABI_V0,          0x00000000U,
	`LoongArch object file ABI version 0')
_(EF_LOONGARCH_OBJABI_V1,          0x00000040U,
	`LoongArch object file ABI version 1')
')
define(`DEFINE_EHDR_FLAG_MASKS_LOONGARCH',`dnl
_(EF_LOONGARCH_ABI_MODIFIER_MASK,  0x00000007U,
	`LoongArch floating point modifier mask')
_(EF_LOONGARCH_OBJABI_MASK,        0x000000C0U,
	`LoongArch object file ABI version mask')
')

define(`DEFINE_EHDR_FLAGS_MIPS',`dnl
_(EF_MIPS_NOREORDER,   0x00000001U,
	`at least one .noreorder directive appeared in the source')
_(EF_MIPS_PIC,         0x00000002U,
	`file contains position independent code')
_(EF_MIPS_CPIC,        0x00000004U,
	`file code uses standard conventions for calling PIC')
_(EF_MIPS_UCODE,       0x00000010U,
	`file contains UCODE (obsolete)')
_(EF_MIPS_ABI,	      0x00007000U,
	`Application binary interface, see E_MIPS_* values')
_(EF_MIPS_ABI2,        0x00000020U,
	`file follows MIPS III 32-bit ABI')
_(EF_MIPS_OPTIONS_FIRST, 0x00000080U,
	`ld(1) should process .MIPS.options section first')
_(EF_MIPS_ARCH_ASE_MDMX, 0x08000000U,
	`file uses MDMX multimedia extensions')
_(EF_MIPS_ARCH_ASE_M16, 0x04000000U,
	`file uses MIPS-16 ISA extensions')
_(EF_MIPS_ARCH_ASE_MICROMIPS, 0x02000000U,
	`MicroMIPS architecture')
_(EF_MIPS_ARCH_1,	0x00000000U,
	`MIPS I instruction set')
_(EF_MIPS_ARCH_2,	0x10000000U,
	`MIPS II instruction set')
_(EF_MIPS_ARCH_3,	0x20000000U,
	`MIPS III instruction set')
_(EF_MIPS_ARCH_4,	0x30000000U,
	`MIPS IV instruction set')
_(EF_MIPS_ARCH_5,	0x40000000U,
	`Never introduced')
_(EF_MIPS_ARCH_32,	0x50000000U,
	`Mips32 Revision 1')
_(EF_MIPS_ARCH_64,	0x60000000U,
	`Mips64 Revision 1')
_(EF_MIPS_ARCH_32R2,	0x70000000U,
	`Mips32 Revision 2')
_(EF_MIPS_ARCH_64R2,	0x80000000U,
	`Mips64 Revision 2')
')
define(`DEFINE_EHDR_FLAG_MASKS_MIPS',`dnl
_(EF_MIPS_ARCH_ASE,	0x0F000000U,
	`file uses application-specific architectural extensions')
_(EF_MIPS_ARCH,		0xF0000000U,
	`4-bit MIPS architecture field')
')

define(`DEFINE_EHDR_FLAGS_PPC',`dnl
_(EF_PPC_EMB,          0x80000000U,
	`Embedded PowerPC flag')
_(EF_PPC_RELOCATABLE,  0x00010000U,
	`-mrelocatable flag')
_(EF_PPC_RELOCATABLE_LIB, 0x00008000U,
	`-mrelocatable-lib flag')
')

define(`DEFINE_EHDR_FLAGS_RISCV',`dnl
_(EF_RISCV_RVC,	    0x00000001U,
	`Binary uses the C ABI.')
_(EF_RISCV_FLOAT_ABI_SOFT, 0x00000000U,
	`Software emulated floating point')
_(EF_RISCV_FLOAT_ABI_SINGLE, 0x00000002U,
	`Single precision floating point')
_(EF_RISCV_FLOAT_ABI_DOUBLE, 0x00000004U,
	`Double precision floating point')
_(EF_RISCV_FLOAT_ABI_QUAD, 0x00000006U,
	`Quad precision floating point')
_(EF_RISCV_RVE,	    0x00000008U,
	`Binary targets the E ABI.')
_(EF_RISCV_TSO,	    0x00000010U,
	`Binary requires the RVTSO memory consistency model.')
_(EF_RISCV_RV64ILP32,	0x00000020U,
	`Binary requires RV64ILP32 ABIs.')
')
define(`DEFINE_EHDR_FLAG_MASKS_RISCV',`dnl
_(EF_RISCV_FLOAT_ABI_MASK, 0x00000006U,
	`Bits determining the floating point ABI.')
')

define(`DEFINE_EHDR_FLAGS_SPARC',`dnl
_(EF_SPARC_32PLUS,     0x00000100U,
	`Generic V8+ features')
_(EF_SPARC_SUN_US1,    0x00000200U,
	`Sun UltraSPARCTM 1 Extensions')
_(EF_SPARC_HAL_R1,     0x00000400U,
	`HAL R1 Extensions')
_(EF_SPARC_SUN_US3,    0x00000800U,
	`Sun UltraSPARC 3 Extensions')
_(EF_SPARCV9_TSO,      0x00000000U,
	`Total Store Ordering')
_(EF_SPARCV9_PSO,      0x00000001U,
	`Partial Store Ordering')
_(EF_SPARCV9_RMO,      0x00000002U,
	`Relaxed Memory Ordering')
')
define(`DEFINE_EHDR_FLAG_MASKS_SPARC',`dnl
_(EF_SPARC_EXT_MASK,   0x00FFFF00U,
	`Vendor Extension mask')
_(EF_SPARCV9_MM,       0x00000003U,
	`Mask for Memory Model')
')

define(`DEFINE_EHDR_FLAGS',`
DEFINE_EHDR_FLAGS_ARM()
DEFINE_EHDR_FLAG_MASKS_ARM()
DEFINE_EHDR_FLAGS_IA_64()
DEFINE_EHDR_FLAG_MASKS_IA_64()
DEFINE_EHDR_FLAGS_LOONGARCH()
DEFINE_EHDR_FLAG_MASKS_LOONGARCH()
DEFINE_EHDR_FLAGS_MIPS()
DEFINE_EHDR_FLAG_MASKS_MIPS()
DEFINE_EHDR_FLAGS_PPC()
DEFINE_EHDR_FLAGS_RISCV()
DEFINE_EHDR_FLAG_MASKS_RISCV()
DEFINE_EHDR_FLAGS_SPARC()
DEFINE_EHDR_FLAG_MASKS_SPARC()
')

define(`DEFINE_EHDR_FLAG_SYNONYMS',`
DEFINE_EHDR_FLAG_SYNONYMS_ARM()
')

#
# Offsets in the `ei_ident[]` field of an ELF executable header.
#
define(`DEFINE_EI_OFFSETS',`
_(EI_MAG0,     0,
	`magic number')
_(EI_MAG1,     1,
	`magic number')
_(EI_MAG2,     2,
	`magic number')
_(EI_MAG3,     3,
	`magic number')
_(EI_CLASS,    4,
	`file class')
_(EI_DATA,     5,
	`data encoding')
_(EI_VERSION,  6,
	`file version')
_(EI_OSABI,    7,
	`OS ABI kind')
_(EI_ABIVERSION, 8,
	`OS ABI version')
_(EI_PAD,	    9,
	`padding start')
_(EI_NIDENT,  16,
	`total size')
')

#
# The ELF class of an object.
#
define(`DEFINE_ELF_CLASSES',`
_(ELFCLASSNONE, 0U,
	`Unknown ELF class')
_(ELFCLASS32,   1U,
	`32 bit objects')
_(ELFCLASS64,   2U,
	`64 bit objects')
')

#
# Endianness of data in an ELF object.
#
define(`DEFINE_ELF_DATA_ENDIANNESSES',`
_(ELFDATANONE, 0U,
	`Unknown data endianness')
_(ELFDATA2LSB, 1U,
	`little endian')
_(ELFDATA2MSB, 2U,
	`big endian')
')


#
# The magic numbers used in the initial four bytes of an ELF object.
#
# These numbers are: 0x7F, and the characters 'E', 'L' and 'F' encoded
# in ASCII.
#
# This definition needs an expansion of `_' that replaces `@' characters
# with single quotes.
define(`DEFINE_ELF_MAGIC_VALUES',`
_(ELFMAG0, 0x7FU)
_(ELFMAG1, 0x45U, @E@)
_(ELFMAG2, 0x4CU, @L@)
_(ELFMAG3, 0x46U, @F@)
')

# Additional ELFMAG related constants.
define(`DEFINE_ELF_MAGIC_ADDITIONAL_CONSTANTS',`
_(ELFMAG,  "\177ELF",	`ELF magic bytes as a string.')
_(SELFMAG, 4,		`The number of ELF magic bytes.')
')

#
# ELF OS ABI field.
#
define(`DEFINE_ELF_OSABIS',`
_(ELFOSABI_NONE,       0U,
	`No extensions or unspecified')
_(ELFOSABI_SYSV,       0U,
	`SYSV')
_(ELFOSABI_HPUX,       1U,
	`Hewlett-Packard HP-UX')
_(ELFOSABI_NETBSD,     2U,
	`NetBSD')
_(ELFOSABI_GNU,        3U,
	`GNU')
_(ELFOSABI_HURD,       4U,
	`GNU/HURD')
_(ELFOSABI_86OPEN,     5U,
	`86Open Common ABI')
_(ELFOSABI_SOLARIS,    6U,
	`Sun Solaris')
_(ELFOSABI_AIX,        7U,
	`AIX')
_(ELFOSABI_IRIX,       8U,
	`IRIX')
_(ELFOSABI_FREEBSD,    9U,
	`FreeBSD')
_(ELFOSABI_TRU64,      10U,
	`Compaq TRU64 UNIX')
_(ELFOSABI_MODESTO,    11U,
	`Novell Modesto')
_(ELFOSABI_OPENBSD,    12U,
	`Open BSD')
_(ELFOSABI_OPENVMS,    13U,
	`Open VMS')
_(ELFOSABI_NSK,        14U,
	`Hewlett-Packard Non-Stop Kernel')
_(ELFOSABI_AROS,       15U,
	`Amiga Research OS')
_(ELFOSABI_FENIXOS,    16U,
	`The FenixOS highly scalable multi-core OS')
_(ELFOSABI_CLOUDABI,   17U,
	`Nuxi CloudABI')
_(ELFOSABI_OPENVOS,    18U,
	`Stratus Technologies OpenVOS')
_(ELFOSABI_ARM_AEABI,  64U,
	`ARM specific symbol versioning extensions')
_(ELFOSABI_ARM,        97U,
	`ARM ABI')
_(ELFOSABI_STANDALONE, 255U,
	`Standalone (embedded) application')
')

# OS ABI aliases.
define(`DEFINE_ELF_OSABI_ALIASES',`
_(ELFOSABI_LINUX,	ELFOSABI_GNU)
')

#
# ELF Machine types: (EM_*).
#
define(`DEFINE_ELF_MACHINE_TYPES',`
_(EM_NONE,             0U,
	`No machine')
_(EM_M32,              1U,
	`AT&T WE 32100')
_(EM_SPARC,            2U,
	`SPARC')
_(EM_386,              3U,
	`Intel 80386')
_(EM_68K,              4U,
	`Motorola 68000')
_(EM_88K,              5U,
	`Motorola 88000')
_(EM_IAMCU,            6U,
	`Intel MCU')
_(EM_860,              7U,
	`Intel 80860')
_(EM_MIPS,             8U,
	`MIPS I Architecture')
_(EM_S370,             9U,
	`IBM System/370 Processor')
_(EM_MIPS_RS3_LE,      10U,
	`MIPS RS3000 Little-endian')
__(`	', `Reserved: 11-14.')
_(EM_PARISC,           15U,
	`Hewlett-Packard PA-RISC')
__(`	', `Reserved: 16.')
_(EM_VPP500,           17U,
	`Fujitsu VPP500')
_(EM_SPARC32PLUS,      18U,
	`Enhanced instruction set SPARC')
_(EM_960,              19U,
	`Intel 80960')
_(EM_PPC,              20U,
	`PowerPC')
_(EM_PPC64,            21U,
	`64-bit PowerPC')
_(EM_S390,             22U,
	`IBM System/390 Processor')
_(EM_SPU,              23U,
	`IBM SPU/SPC')
__(`	', `Reserved: 24-35.')
_(EM_V800,             36U,
	`NEC V800')
_(EM_FR20,             37U,
	`Fujitsu FR20')
_(EM_RH32,             38U,
	`TRW RH-32')
_(EM_RCE,              39U,
	`Motorola RCE')
_(EM_ARM,              40U,
	`Advanced RISC Machines ARM')
_(EM_ALPHA,            41U,
	`Digital Alpha')
_(EM_SH,               42U,
	`Hitachi SH')
_(EM_SPARCV9,          43U,
	`SPARC Version 9')
_(EM_TRICORE,          44U,
	`Siemens TriCore embedded processor')
_(EM_ARC,              45U,
	`Argonaut RISC Core, Argonaut Technologies Inc.')
_(EM_H8_300,           46U,
	`Hitachi H8/300')
_(EM_H8_300H,          47U,
	`Hitachi H8/300H')
_(EM_H8S,              48U,
	`Hitachi H8S')
_(EM_H8_500,           49U,
	`Hitachi H8/500')
_(EM_IA_64,            50U,
	`Intel IA-64 processor architecture')
_(EM_MIPS_X,           51U,
	`Stanford MIPS-X')
_(EM_COLDFIRE,         52U,
	`Motorola ColdFire')
_(EM_68HC12,           53U,
	`Motorola M68HC12')
_(EM_MMA,              54U,
	`Fujitsu MMA Multimedia Accelerator')
_(EM_PCP,              55U,
	`Siemens PCP')
_(EM_NCPU,             56U,
	`Sony nCPU embedded RISC processor')
_(EM_NDR1,             57U,
	`Denso NDR1 microprocessor')
_(EM_STARCORE,         58U,
	`Motorola Star*Core processor')
_(EM_ME16,             59U,
	`Toyota ME16 processor')
_(EM_ST100,            60U,
	`STMicroelectronics ST100 processor')
_(EM_TINYJ,            61U,
	`Advanced Logic Corp. TinyJ embedded processor family')
_(EM_X86_64,           62U,
	`AMD x86-64 architecture')
_(EM_PDSP,             63U,
	`Sony DSP Processor')
_(EM_PDP10,            64U,
	`Digital Equipment Corp. PDP-10')
_(EM_PDP11,            65U,
	`Digital Equipment Corp. PDP-11')
_(EM_FX66,             66U,
	`Siemens FX66 microcontroller')
_(EM_ST9PLUS,          67U,
	`STMicroelectronics ST9+ 8/16 bit microcontroller')
_(EM_ST7,              68U,
	`STMicroelectronics ST7 8-bit microcontroller')
_(EM_68HC16,           69U,
	`Motorola MC68HC16 Microcontroller')
_(EM_68HC11,           70U,
	`Motorola MC68HC11 Microcontroller')
_(EM_68HC08,           71U,
	`Motorola MC68HC08 Microcontroller')
_(EM_68HC05,           72U,
	`Motorola MC68HC05 Microcontroller')
_(EM_SVX,              73U,
	`Silicon Graphics SVx')
_(EM_ST19,             74U,
	`STMicroelectronics ST19 8-bit microcontroller')
_(EM_VAX,              75U,
	`Digital VAX')
_(EM_CRIS,             76U,
	`Axis Communications 32-bit embedded processor')
_(EM_JAVELIN,          77U,
	`Infineon Technologies 32-bit embedded processor')
_(EM_FIREPATH,         78U,
	`Element 14 64-bit DSP Processor')
_(EM_ZSP,              79U,
	`LSI Logic 16-bit DSP Processor')
_(EM_MMIX,             80U,
	`Educational 64-bit processor by Donald Knuth')
_(EM_HUANY,            81U,
	`Harvard University machine-independent object files')
_(EM_PRISM,            82U,
	`SiTera Prism')
_(EM_AVR,              83U,
	`Atmel AVR 8-bit microcontroller')
_(EM_FR30,             84U,
	`Fujitsu FR30')
_(EM_D10V,             85U,
	`Mitsubishi D10V')
_(EM_D30V,             86U,
	`Mitsubishi D30V')
_(EM_V850,             87U,
	`NEC v850')
_(EM_M32R,             88U,
	`Mitsubishi M32R')
_(EM_MN10300,          89U,
	`Matsushita MN10300')
_(EM_MN10200,          90U,
	`Matsushita MN10200')
_(EM_PJ,               91U,
	`picoJava')
_(EM_OPENRISC,         92U,
	`OpenRISC 32-bit embedded processor')
_(EM_ARC_COMPACT,      93U,
	`ARC International ARCompact processor')
_(EM_XTENSA,           94U,
	`Tensilica Xtensa Architecture')
_(EM_VIDEOCORE,        95U,
	`Alphamosaic VideoCore processor')
_(EM_TMM_GPP,          96U,
	`Thompson Multimedia General Purpose Processor')
_(EM_NS32K,            97U,
	`National Semiconductor 32000 series')
_(EM_TPC,              98U,
	`Tenor Network TPC processor')
_(EM_SNP1K,            99U,
	`Trebia SNP 1000 processor')
_(EM_ST200,            100U,
	`STMicroelectronics (www.st.com) ST200 microcontroller')
_(EM_IP2K,             101U,
	`Ubicom IP2xxx microcontroller family')
_(EM_MAX,              102U,
	`MAX Processor')
_(EM_CR,               103U,
	`National Semiconductor CompactRISC microprocessor')
_(EM_F2MC16,           104U,
	`Fujitsu F2MC16')
_(EM_MSP430,           105U,
	`Texas Instruments embedded microcontroller msp430')
_(EM_BLACKFIN,         106U,
	`Analog Devices Blackfin (DSP) processor')
_(EM_SE_C33,           107U,
	`S1C33 Family of Seiko Epson processors')
_(EM_SEP,              108U,
	`Sharp embedded microprocessor')
_(EM_ARCA,             109U,
	`Arca RISC Microprocessor')
_(EM_UNICORE,          110U,
	`Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University')
_(EM_EXCESS,           111U,
	`eXcess: 16/32/64-bit configurable embedded CPU')
_(EM_DXP,              112U,
	`Icera Semiconductor Inc. Deep Execution Processor')
_(EM_ALTERA_NIOS2,     113U,
	`Altera Nios II soft-core processor')
_(EM_CRX,              114U,
	`National Semiconductor CompactRISC CRX microprocessor')
_(EM_XGATE,            115U,
	`Motorola XGATE embedded processor')
_(EM_C166,             116U,
	`Infineon C16x/XC16x processor')
_(EM_M16C,             117U,
	`Renesas M16C series microprocessors')
_(EM_DSPIC30F,         118U,
	`Microchip Technology dsPIC30F Digital Signal Controller')
_(EM_CE,               119U,
	`Freescale Communication Engine RISC core')
_(EM_M32C,             120U,
	`Renesas M32C series microprocessors')
__(`	', `Reserved: 121-130.')
_(EM_TSK3000,          131U,
	`Altium TSK3000 core')
_(EM_RS08,             132U,
	`Freescale RS08 embedded processor')
_(EM_SHARC,            133U,
	`Analog Devices SHARC family of 32-bit DSP processors')
_(EM_ECOG2,            134U,
	`Cyan Technology eCOG2 microprocessor')
_(EM_SCORE7,           135U,
	`Sunplus S+core7 RISC processor')
_(EM_DSP24,            136U,
	`New Japan Radio (NJR) 24-bit DSP Processor')
_(EM_VIDEOCORE3,       137U,
	`Broadcom VideoCore III processor')
_(EM_LATTICEMICO32,    138U,
	`RISC processor for Lattice FPGA architecture')
_(EM_SE_C17,           139U,
	`Seiko Epson C17 family')
_(EM_TI_C6000,         140U,
	`The Texas Instruments TMS320C6000 DSP family')
_(EM_TI_C2000,         141U,
	`The Texas Instruments TMS320C2000 DSP family')
_(EM_TI_C5500,         142U,
	`The Texas Instruments TMS320C55x DSP family')
_(EM_TI_ARP32,         143U,
	`Texas Instruments Application Specific RISC Processor, 32bit fetch')
_(EM_TI_PRU,           144U,
	`Texas Instruments Programmable Realtime Unit')
__(`	', `Reserved: 145-159.')
_(EM_MMDSP_PLUS,       160U,
	`STMicroelectronics 64bit VLIW Data Signal Processor')
_(EM_CYPRESS_M8C,      161U,
	`Cypress M8C microprocessor')
_(EM_R32C,             162U,
	`Renesas R32C series microprocessors')
_(EM_TRIMEDIA,         163U,
	`NXP Semiconductors TriMedia architecture family')
_(EM_QDSP6,            164U,
	`QUALCOMM DSP6 Processor')
_(EM_8051,             165U,
	`Intel 8051 and variants')
_(EM_STXP7X,           166U,
	`STMicroelectronics STxP7x family of configurable and extensible RISC processors')
_(EM_NDS32,            167U,
	`Andes Technology compact code size embedded RISC processor family')
_(EM_ECOG1X,           168U,
	`Cyan Technology eCOG1X family')
_(EM_MAXQ30,           169U,
	`Dallas Semiconductor MAXQ30 Core Micro-controllers')
_(EM_XIMO16,           170U,
	`New Japan Radio (NJR) 16-bit DSP Processor')
_(EM_MANIK,            171U,
	`M2000 Reconfigurable RISC Microprocessor')
_(EM_CRAYNV2,          172U,
	`Cray Inc. NV2 vector architecture')
_(EM_RX,               173U,
	`Renesas RX family')
_(EM_METAG,            174U,
	`Imagination Technologies META processor architecture')
_(EM_MCST_ELBRUS,      175U,
	`MCST Elbrus general purpose hardware architecture')
_(EM_ECOG16,           176U,
	`Cyan Technology eCOG16 family')
_(EM_CR16,             177U,
	`National Semiconductor CompactRISC CR16 16-bit microprocessor')
_(EM_ETPU,             178U,
	`Freescale Extended Time Processing Unit')
_(EM_SLE9X,            179U,
	`Infineon Technologies SLE9X core')
_(EM_L10M,             180U,
	`Intel L10M')
_(EM_K10M,             181U,
	`Intel K10M')
__(`	', `Reserved for future Intel use: 182.')
_(EM_AARCH64,          183U,
	`AArch64 (64-bit ARM)')
__(`	', `Reserved for future ARM use: 184.')
_(EM_AVR32,            185U,
	`Atmel Corporation 32-bit microprocessor family')
_(EM_STM8,             186U,
	`STMicroeletronics STM8 8-bit microcontroller')
_(EM_TILE64,           187U,
	`Tilera TILE64 multicore architecture family')
_(EM_TILEPRO,          188U,
	`Tilera TILEPro multicore architecture family')
_(EM_MICROBLAZE,       189U,
	`Xilinx MicroBlaze 32-bit RISC soft processor core')
_(EM_CUDA,             190U,
	`NVIDIA CUDA architecture')
_(EM_TILEGX,           191U,
	`Tilera TILE-Gx multicore architecture family')
_(EM_CLOUDSHIELD,      192U,
	`CloudShield architecture family')
_(EM_COREA_1ST,        193U,
	`KIPO-KAIST Core-A 1st generation processor family')
_(EM_COREA_2ND,        194U,
	`KIPO-KAIST Core-A 2nd generation processor family')
_(EM_ARC_COMPACT2,     195U,
	`Synopsys ARCompact V2')
_(EM_OPEN8,            196U,
	`Open8 8-bit RISC soft processor core')
_(EM_RL78,             197U,
	`Renesas RL78 family')
_(EM_VIDEOCORE5,       198U,
	`Broadcom VideoCore V processor')
_(EM_78KOR,            199U,
	`Renesas 78KOR family')
_(EM_56800EX,          200U,
	`Freescale 56800EX Digital Signal Controller')
_(EM_BA1,              201U,
	`Beyond BA1 CPU architecture')
_(EM_BA2,              202U,
	`Beyond BA2 CPU architecture')
_(EM_XCORE,            203U,
	`XMOS xCORE processor family')
_(EM_MCHP_PIC,         204U,
	`Microchip 8-bit PIC(r) family')
_(EM_INTEL205,         205U,
	`Intel Graphics Technology')
_(EM_INTEL206,         206U,
	`Reserved by Intel')
_(EM_INTEL207,         207U,
	`Reserved by Intel')
_(EM_INTEL208,         208U,
	`Reserved by Intel')
_(EM_INTEL209,         209U,
	`Reserved by Intel')
_(EM_KM32,             210U,
	`KM211 KM32 32-bit processor')
_(EM_KMX32,            211U,
	`KM211 KMX32 32-bit processor')
_(EM_KMX16,            212U,
	`KM211 KMX16 16-bit processor')
_(EM_KMX8,             213U,
	`KM211 KMX8 8-bit processor')
_(EM_KVARC,            214U,
	`KM211 KMX32 KVARC processor')
_(EM_CDP,              215U,
	`Paneve CDP architecture family')
_(EM_COGE,             216U,
	`Cognitive Smart Memory Processor')
_(EM_COOL,             217U,
	`Bluechip Systems CoolEngine')
_(EM_NORC,             218U,
	`Nanoradio Optimized RISC')
_(EM_CSR_KALIMBA,      219U,
	`CSR Kalimba architecture family')
_(EM_Z80,              220U,
	`Zilog Z80')
_(EM_VISIUM,           221U,
	`Controls and Data Services VISIUMcore processor')
_(EM_FT32,             222U,
	`FTDI Chip FT32 high performance 32-bit RISC architecture')
_(EM_MOXIE,            223U,
	`Moxie processor family')
_(EM_AMDGPU,           224U,
	`AMD GPU architecture')
__(`	', `Reserved for future use: 225-242.')
_(EM_RISCV,            243U,
	`RISC-V')
_(EM_LANAI,            244U,
	`Lanai processor')
_(EM_CEVA,             245U,
	`CEVA Processor Architecture Family')
_(EM_CEVA_X2,          246U,
	`CEVA X2 Processor Family')
_(EM_BPF,              247U,
	`Linux BPF – in-kernel virtual machine')
_(EM_GRAPHCORE_IPU,    248U,
	`Graphcore Intelligent Processing Unit')
_(EM_IMG1,             249U,
	`Imagination Technologies')
_(EM_NFP,              250U,
	`Netronome Flow Processor (NFP)')
_(EM_VE,               251U,
	`NEC Vector Engine')
_(EM_CSKY,             252U,
	`C-SKY processor family')
_(EM_ARC_COMPACT3_64,  253U,
	`Synopsys ARCv2.3 64-bit')
_(EM_MCS6502,          254U,
	`MOS Technology MCS 6502 processor')
_(EM_ARC_COMPACT3,     255U,
	`Synopsys ARCv2.3 32-bit')
_(EM_KVX,              256U,
	`Kalray VLIW core of the MPPA processor family')
_(EM_65816,            257U,
	`WDC 65816/65C816')
_(EM_LOONGARCH,        258U,
	`Loongson LoongArch')
_(EM_KF32,             259U,
	`ChipON KungFu 32')
_(EM_U16_U8CORE,       260U,
	`LAPIS nX-U16/U8')
_(EM_TACHYUM,          261U,
	`Reserved for Tachyum processor')
_(EM_56800EF,          262U,
	`NXP 56800EF Digital Signal Controller (DSC)')
_(EM_SBF,              263U,
	`Solana Bytecode Format')
_(EM_AIENGINE,         264U,
	`AMD/Xilinx AIEngine architecture')
_(EM_SIMA_MLA,         265U,
	`SiMa MLA')
_(EM_BANG,             266U,
	`Cambricon BANG')
_(EM_LOONGGPU,         267U,
	`Loongson LoongArch GPU')
_(EM_SW64,             268U,
	`Wuxi Institute of Advanced Technology SW64')
_(EM_AIECTRLCODE,      269U,
	`AMD/Xilinx AIEngine ctrlcode')
__(`	', ` Historical and experimental values. ')
_(EM_ALPHA_HISTORICAL, 0x9026U,
	`Prior value used by GNU and NetBSD')
')

define(`DEFINE_ELF_MACHINE_TYPE_SYNONYMS',`
_(EM_486, EM_IAMCU)
_(EM_AMD64, EM_X86_64)
_(EM_ARC_A5, EM_ARC_COMPACT)
_(EM_ECOG1, EM_ECOG1X)
_(EM_INTELGT, EM_INTEL205)
')

#
# ELF file types: (ET_*).
#
define(`DEFINE_ELF_TYPES',`
_(ET_NONE,   0U,
	`No file type')
_(ET_REL,    1U,
	`Relocatable object')
_(ET_EXEC,   2U,
	`Executable')
_(ET_DYN,    3U,
	`Shared object')
_(ET_CORE,   4U,
	`Core file')
_(ET_LOOS,   0xFE00U,
	`Begin OS-specific range')
_(ET_HIOS,   0xFEFFU,
	`End OS-specific range')
_(ET_LOPROC, 0xFF00U,
	`Begin processor-specific range')
_(ET_HIPROC, 0xFFFFU,
	`End processor-specific range')
')

# ELF file format version numbers.
define(`DEFINE_ELF_FILE_VERSIONS',`
_(EV_NONE, 0U)
_(EV_CURRENT, 1U)
')

#
# Flags for section groups.
#
define(`DEFINE_GRP_FLAGS',`
_(GRP_COMDAT, 	0x1U,
	`COMDAT semantics')
_(GRP_MASKOS,	0x0FF00000U,
	`OS-specific flags')
_(GRP_MASKPROC, 	0xF0000000U,
	`processor-specific flags')
')

#
# Flags / mask for .gnu.versym sections.
#
define(`DEFINE_VERSYMS',`
_(VERSYM_VERSION,	0x7FFFU)
_(VERSYM_HIDDEN,	0x8000U)
')

#
# Flags used by program header table entries.
#
define(`DEFINE_PHDR_FLAGS',`
_(PF_X,                0x1,
	`Execute')
_(PF_W,                0x2,
	`Write')
_(PF_R,                0x4,
	`Read')
_(PF_MASKOS,           0x0FF00000,
	`OS-specific flags')
_(PF_MASKPROC,         0xF0000000,
	`Processor-specific flags')
_(PF_ARM_SB,           0x10000000,
	`segment contains the location addressed by the static base')
_(PF_ARM_PI,           0x20000000,
	`segment is position-independent')
_(PF_ARM_ABS,          0x40000000,
	`segment must be loaded at its base address')
')

#
# Types of program header table entries.
#
define(`DEFINE_PHDR_TYPES',`
_(PT_NULL,             0U,
	`ignored entry')
_(PT_LOAD,             1U,
	`loadable segment')
_(PT_DYNAMIC,          2U,
	`contains dynamic linking information')
_(PT_INTERP,           3U,
	`names an interpreter')
_(PT_NOTE,             4U,
	`auxiliary information')
_(PT_SHLIB,            5U,
	`reserved')
_(PT_PHDR,             6U,
	`describes the program header itself')
_(PT_TLS,              7U,
	`thread local storage')
_(PT_LOOS,             0x60000000U,
	`start of OS-specific range')
_(PT_SUNW_UNWIND,      0x6464E550U,
	`Solaris/amd64 stack unwind tables')
_(PT_GNU_EH_FRAME,     0x6474E550U,
	`GCC generated .eh_frame_hdr segment')
_(PT_GNU_STACK,	    0x6474E551U,
	`Stack flags')
_(PT_GNU_RELRO,	    0x6474E552U,
	`Segment becomes read-only after relocation')
_(PT_OPENBSD_RANDOMIZE,0x65A3DBE6U,
	`Segment filled with random data')
_(PT_OPENBSD_WXNEEDED, 0x65A3DBE7U,
	`Program violates W^X')
_(PT_OPENBSD_BOOTDATA, 0x65A41BE6U,
	`Boot data')
_(PT_SUNWBSS,          0x6FFFFFFAU,
	`A Solaris .SUNW_bss section')
_(PT_SUNWSTACK,        0x6FFFFFFBU,
	`A Solaris process stack')
_(PT_SUNWDTRACE,       0x6FFFFFFCU,
	`Used by dtrace(1)')
_(PT_SUNWCAP,          0x6FFFFFFDU,
	`Special hardware capability requirements')
_(PT_HIOS,             0x6FFFFFFFU,
	`end of OS-specific range')
_(PT_LOPROC,           0x70000000U,
	`start of processor-specific range')
_(PT_AARCH64_ARCHEXT,  0x70000000U,
	`platform architecture compatibility information')
_(PT_AARCH64_UNWIND,   0x70000001U,
	`exception unwinding tables')
_(PT_AARCH64_MEMTAG_MTE, 0x70000002U,
	`MTE memory tag data dumps in core files')
_(PT_ARM_ARCHEXT,      0x70000000U,
	`platform architecture compatibility information')
_(PT_ARM_EXIDX,        0x70000001U,
	`exception unwind tables')
_(PT_MIPS_REGINFO,     0x70000000U,
	`register usage information')
_(PT_MIPS_RTPROC,      0x70000001U,
	`runtime procedure table')
_(PT_MIPS_OPTIONS,     0x70000002U,
	`options segment')
_(PT_HIPROC,           0x7FFFFFFFU,
	`end of processor-specific range')
')

define(`DEFINE_PHDR_TYPE_SYNONYMS',`
_(PT_ARM_UNWIND,	PT_ARM_EXIDX)
_(PT_HISUNW,	PT_HIOS)
_(PT_LOSUNW,	PT_SUNWBSS)
')

#
# Section flags.
#
define(`DEFINE_SECTION_FLAGS',`
_(SHF_WRITE,           0x00000001U,
	`writable during program execution')
_(SHF_ALLOC,           0x00000002U,
	`occupies memory during program execution')
_(SHF_EXECINSTR,       0x00000004U,
	`executable instructions')
_(SHF_MERGE,           0x00000010U,
	`may be merged to prevent duplication')
_(SHF_STRINGS,         0x00000020U,
	`NUL-terminated character strings')
_(SHF_INFO_LINK,       0x00000040U,
	`the sh_info field holds a link')
_(SHF_LINK_ORDER,      0x00000080U,
	`special ordering requirements during linking')
_(SHF_OS_NONCONFORMING, 0x00000100U,
	`requires OS-specific processing during linking')
_(SHF_GROUP,           0x00000200U,
	`member of a section group')
_(SHF_TLS,             0x00000400U,
	`holds thread-local storage')
_(SHF_COMPRESSED,      0x00000800U,
	`holds compressed data')
_(SHF_MASKOS,          0x0FF00000U,
	`bits reserved for OS-specific semantics')
_(SHF_AMD64_LARGE,     0x10000000U,
	`section uses large code model')
_(SHF_ENTRYSECT,       0x10000000U,
	`section contains an entry point (ARM)')
_(SHF_COMDEF,          0x80000000U,
	`section may be multiply defined in input to link step (ARM)')
_(SHF_MIPS_GPREL,      0x10000000U,
	`section must be part of global data area')
_(SHF_MIPS_MERGE,      0x20000000U,
	`section data should be merged to eliminate duplication')
_(SHF_MIPS_ADDR,       0x40000000U,
	`section data is addressed by default')
_(SHF_MIPS_STRING,     0x80000000U,
	`section data is string data by default')
_(SHF_MIPS_NOSTRIP,    0x08000000U,
	`section data may not be stripped')
_(SHF_MIPS_LOCAL,      0x04000000U,
	`section data local to process')
_(SHF_MIPS_NAMES,      0x02000000U,
	`linker must generate implicit hidden weak names')
_(SHF_MIPS_NODUPE,     0x01000000U,
	`linker must retain only one copy')
_(SHF_ORDERED,         0x40000000U,
	`section is ordered with respect to other sections')
_(SHF_EXCLUDE,         0x80000000U,
	`section is excluded from executables and shared objects')
_(SHF_MASKPROC,        0xF0000000U,
	`bits reserved for processor-specific semantics')
')

#
# Special section indices.
#
define(`DEFINE_SECTION_INDICES',`
_(SHN_UNDEF, 	0U,
	 `undefined section')
_(SHN_LORESERVE, 	0xFF00U,
	`start of reserved area')
_(SHN_LOPROC, 	0xFF00U,
	`start of processor-specific range')
_(SHN_BEFORE,	0xFF00U,
	`used for section ordering')
_(SHN_AFTER,	0xFF01U,
	`used for section ordering')
_(SHN_AMD64_LCOMMON, 0xFF02U,
	`large common block label')
_(SHN_MIPS_ACOMMON, 0xFF00U,
	`allocated common symbols in a DSO')
_(SHN_MIPS_TEXT,	0xFF01U,
	`Reserved (obsolete)')
_(SHN_MIPS_DATA,	0xFF02U,
	`Reserved (obsolete)')
_(SHN_MIPS_SCOMMON, 0xFF03U,
	`gp-addressable common symbols')
_(SHN_MIPS_SUNDEFINED, 0xFF04U,
	`gp-addressable undefined symbols')
_(SHN_MIPS_LCOMMON, 0xFF05U,
	`local common symbols')
_(SHN_MIPS_LUNDEFINED, 0xFF06U,
	`local undefined symbols')
_(SHN_HIPROC, 	0xFF1FU,
	`end of processor-specific range')
_(SHN_LOOS, 	0xFF20U,
	`start of OS-specific range')
_(SHN_SUNW_IGNORE, 0xFF3FU,
	`used by dtrace')
_(SHN_HIOS, 	0xFF3FU,
	`end of OS-specific range')
_(SHN_ABS, 	0xFFF1U,
	`absolute references')
_(SHN_COMMON, 	0xFFF2U,
	`references to COMMON areas')
_(SHN_XINDEX, 	0xFFFFU,
	`extended index')
_(SHN_HIRESERVE, 	0xFFFFU,
	`end of reserved area')
')

#
# Section types.
#
define(`DEFINE_SECTION_TYPES',`
_(SHT_NULL,            0U, `inactive header')
_(SHT_PROGBITS,        1U, `program defined information')
_(SHT_SYMTAB,          2U, `symbol table')
_(SHT_STRTAB,          3U, `string table')
_(SHT_RELA,            4U,
	`relocation entries with addends')
_(SHT_HASH,            5U, `symbol hash table')
_(SHT_DYNAMIC,         6U,
	`information for dynamic linking')
_(SHT_NOTE,            7U, `additional notes')
_(SHT_NOBITS,          8U, `section occupying no space')
_(SHT_REL,             9U,
	`relocation entries without addends')
_(SHT_SHLIB,           10U, `reserved')
_(SHT_DYNSYM,          11U, `symbol table')
_(SHT_INIT_ARRAY,      14U,
	`pointers to initialization functions')
_(SHT_FINI_ARRAY,      15U,
	`pointers to termination functions')
_(SHT_PREINIT_ARRAY,   16U,
	`pointers to functions called before initialization')
_(SHT_GROUP,           17U, `defines a section group')
_(SHT_SYMTAB_SHNDX,    18U,
	`used for extended section numbering')
_(SHT_RELR,            19U,
	`used to encode relative relocations')
_(SHT_LOOS,            0x60000000U,
	`start of OS-specific range')
_(SHT_SUNW_dof,	     0x6FFFFFF4U,
	`used by dtrace')
_(SHT_SUNW_cap,	     0x6FFFFFF5U,
	`capability requirements')
_(SHT_GNU_ATTRIBUTES,  0x6FFFFFF5U,
	`object attributes')
_(SHT_SUNW_SIGNATURE,  0x6FFFFFF6U,
	`module verification signature')
_(SHT_GNU_HASH,	     0x6FFFFFF6U,
	`GNU Hash sections')
_(SHT_GNU_LIBLIST,     0x6FFFFFF7U,
	`List of libraries to be prelinked')
_(SHT_SUNW_ANNOTATE,   0x6FFFFFF7U,
	`special section where unresolved references are allowed')
_(SHT_SUNW_DEBUGSTR,   0x6FFFFFF8U,
	`debugging information')
_(SHT_CHECKSUM, 	     0x6FFFFFF8U,
	`checksum for dynamic shared objects')
_(SHT_SUNW_DEBUG,      0x6FFFFFF9U,
	`debugging information')
_(SHT_SUNW_move,       0x6FFFFFFAU,
	`information to handle partially initialized symbols')
_(SHT_SUNW_COMDAT,     0x6FFFFFFBU,
	`section supporting merging of multiple copies of data')
_(SHT_SUNW_syminfo,    0x6FFFFFFCU,
	`additional symbol information')
_(SHT_SUNW_verdef,     0x6FFFFFFDU,
	`symbol versioning information')
_(SHT_SUNW_verneed,    0x6FFFFFFEU,
	`symbol versioning requirements')
_(SHT_SUNW_versym,     0x6FFFFFFFU,
	`symbol versioning table')
_(SHT_HIOS,            0x6FFFFFFFU,
	`end of OS-specific range')
_(SHT_LOPROC,          0x70000000U,
	`start of processor-specific range')
_(SHT_ARM_EXIDX,       0x70000001U,
	`exception index table')
_(SHT_ARM_PREEMPTMAP,  0x70000002U,
	`BPABI DLL dynamic linking preemption map')
_(SHT_ARM_ATTRIBUTES,  0x70000003U,
	`object file compatibility attributes')
_(SHT_ARM_DEBUGOVERLAY, 0x70000004U,
	`overlay debug information')
_(SHT_ARM_OVERLAYSECTION, 0x70000005U,
	`overlay debug information')
_(SHT_MIPS_LIBLIST,    0x70000000U,
	`DSO library information used in link')
_(SHT_MIPS_MSYM,       0x70000001U,
	`MIPS symbol table extension')
_(SHT_MIPS_CONFLICT,   0x70000002U,
	`symbol conflicting with DSO-defined symbols ')
_(SHT_MIPS_GPTAB,      0x70000003U,
	`global pointer table')
_(SHT_MIPS_UCODE,      0x70000004U,
	`reserved')
_(SHT_MIPS_DEBUG,      0x70000005U,
	`reserved (obsolete debug information)')
_(SHT_MIPS_REGINFO,    0x70000006U,
	`register usage information')
_(SHT_MIPS_PACKAGE,    0x70000007U,
	`OSF reserved')
_(SHT_MIPS_PACKSYM,    0x70000008U,
	`OSF reserved')
_(SHT_MIPS_RELD,       0x70000009U,
	`dynamic relocation')
_(SHT_MIPS_IFACE,      0x7000000BU,
	`subprogram interface information')
_(SHT_MIPS_CONTENT,    0x7000000CU,
	`section content classification')
_(SHT_MIPS_OPTIONS,     0x7000000DU,
	`general options')
_(SHT_MIPS_DELTASYM,   0x7000001BU,
	`Delta C++: symbol table')
_(SHT_MIPS_DELTAINST,  0x7000001CU,
	`Delta C++: instance table')
_(SHT_MIPS_DELTACLASS, 0x7000001DU,
	`Delta C++: class table')
_(SHT_MIPS_DWARF,      0x7000001EU,
	`DWARF debug information')
_(SHT_MIPS_DELTADECL,  0x7000001FU,
	`Delta C++: declarations')
_(SHT_MIPS_SYMBOL_LIB, 0x70000020U,
	`symbol-to-library mapping')
_(SHT_MIPS_EVENTS,     0x70000021U,
	`event locations')
_(SHT_MIPS_TRANSLATE,  0x70000022U,
	`???')
_(SHT_MIPS_PIXIE,      0x70000023U,
	`special pixie sections')
_(SHT_MIPS_XLATE,      0x70000024U,
	`address translation table')
_(SHT_MIPS_XLATE_DEBUG, 0x70000025U,
	`SGI internal address translation table')
_(SHT_MIPS_WHIRL,      0x70000026U,
	`intermediate code')
_(SHT_MIPS_EH_REGION,  0x70000027U,
	`C++ exception handling region info')
_(SHT_MIPS_XLATE_OLD,  0x70000028U,
	`obsolete')
_(SHT_MIPS_PDR_EXCEPTION, 0x70000029U,
	`runtime procedure descriptor table exception information')
_(SHT_MIPS_ABIFLAGS,   0x7000002AU,
	`ABI flags')
_(SHT_SPARC_GOTDATA,   0x70000000U,
	`SPARC-specific data')
_(SHT_X86_64_UNWIND,   0x70000001U,
	`unwind tables for the AMD64')
_(SHT_ORDERED,         0x7FFFFFFFU,
	`sort entries in the section')
_(SHT_HIPROC,          0x7FFFFFFFU,
	`end of processor-specific range')
_(SHT_LOUSER,          0x80000000U,
	`start of application-specific range')
_(SHT_HIUSER,          0xFFFFFFFFU,
	`end of application-specific range')
')

# Aliases for section types.
define(`DEFINE_SECTION_TYPE_ALIASES',`
_(SHT_AMD64_UNWIND,	SHT_X86_64_UNWIND)
_(SHT_GNU_verdef,	SHT_SUNW_verdef)
_(SHT_GNU_verneed,	SHT_SUNW_verneed)
_(SHT_GNU_versym,	SHT_SUNW_versym)
')

#
# Symbol binding information.
#
define(`DEFINE_SYMBOL_BINDINGS',`
_(STB_LOCAL,           0,
	`not visible outside defining object file')
_(STB_GLOBAL,          1,
	`visible across all object files being combined')
_(STB_WEAK,            2,
	`visible across all object files but with low precedence')
_(STB_LOOS,            10,
	`start of OS-specific range')
_(STB_GNU_UNIQUE,      10,
	`unique symbol (GNU)')
_(STB_HIOS,            12,
	`end of OS-specific range')
_(STB_LOPROC,          13,
	`start of processor-specific range')
_(STB_HIPROC,          15,
	`end of processor-specific range')
')

#
# Symbol types
#
define(`DEFINE_SYMBOL_TYPES',`
_(STT_NOTYPE,          0,
	`unspecified type')
_(STT_OBJECT,          1,
	`data object')
_(STT_FUNC,            2,
	`executable code')
_(STT_SECTION,         3,
	`section')
_(STT_FILE,            4,
	`source file')
_(STT_COMMON,          5,
	`uninitialized common block')
_(STT_TLS,             6,
	`thread local storage')
_(STT_LOOS,            10,
	`start of OS-specific types')
_(STT_GNU_IFUNC,       10,
	`indirect function')
_(STT_HIOS,            12,
	`end of OS-specific types')
_(STT_LOPROC,          13,
	`start of processor-specific types')
_(STT_ARM_TFUNC,       13,
	`Thumb function (GNU)')
_(STT_ARM_16BIT,       15,
	`Thumb label (GNU)')
_(STT_SPARC_REGISTER,  13,
	`SPARC register information')
_(STT_HIPROC,          15,
	`end of processor-specific types')
')

# Additional symbol type related constants.
define(`DEFINE_SYMBOL_TYPES_ADDITIONAL_CONSTANTS',`
_(STT_NUM,             7,
	`the number of symbol types')
')

#
# Symbol visibility.
#
define(`DEFINE_SYMBOL_VISIBILITIES',`
_(STV_DEFAULT,         0,
	`as specified by symbol type')
_(STV_INTERNAL,        1,
	`as defined by processor semantics')
_(STV_HIDDEN,          2,
	`hidden from other components')
_(STV_PROTECTED,       3,
	`local references are not preemptable')
')

# Syminfo flags.
define(`DEFINE_SYMINFO_FLAGS',`
_(SYMINFO_FLG_DIRECT,	0x0001U,
	`directly assocated reference')
_(SYMINFO_FLG_FILTER, 0x0002U,
	`associated with a filter')
_(SYMINFO_FLG_COPY,	0x0004U,
	`definition by copy-relocation')
_(SYMINFO_FLG_LAZYLOAD,	0x0008U,
	`object should be lazily loaded')
_(SYMINFO_FLG_DIRECTBIND,	0x0010U,
	`reference should be directly bound')
_(SYMINFO_FLG_NOEXTDIRECT, 0x0020U,
	`external references not allowed to bind to definition')
_(SYMINFO_FLG_AUXILIARY,   0x0040U,
	`auxiliary filter')
_(SYMINFO_FLG_INTERPOSE,   0x0080U,
	`interposer symbol')
_(SYMINFO_FLG_CAP,	   0x0100U,
	`associated with capabilities')
_(SYMINFO_FLG_DEFERRED,	   0x0200U,
	`deferred reference')
_(SYMINFO_FLG_WEAKFILTER,  0x0400U,
	`weak filter')
')

# Syminfo bindings.
define(`DEFINE_SYMINFO_BINDINGS',`
_(SYMINFO_BT_SELF,	0xFFFFU,
	`bound to self')
_(SYMINFO_BT_PARENT,	0xFFFEU,
	`bound to parent')
_(SYMINFO_BT_NONE,	0xFFFDU,
	`no special binding')
_(SYMINFO_BT_EXTERN,	0xFFFCU,
	`defined as external')
')

# The version of the syminfo table.  Stored at index 0 of the table.
define(`DEFINE_SYMINFO_VERSIONS',`
_(SYMINFO_NONE,		0,
	`no version')
_(SYMINFO_CURRENT,	1,
	`current version')
')

#
# Version dependencies.
#
define(`DEFINE_VERSIONING_DEPENDENCIES',`
_(VER_NDX_LOCAL,	0,
	`local scope')
_(VER_NDX_GLOBAL,	1,
	`global scope')
_(VER_NDX_GIVEN,	2,
	`global, with user-specified versioning')
')

#
# Version flags.
#
define(`DEFINE_VERSIONING_FLAGS',`
_(VER_FLG_BASE,		0x1,
	`file version')
_(VER_FLG_WEAK,		0x2,
	`weak version')
_(VER_FLG_INFO,		0x4,
	`informational-only version')
')

#
# Version needs
#
define(`DEFINE_VERSIONING_NEEDS',`
_(VER_NEED_NONE,		0,
	`invalid version')
_(VER_NEED_CURRENT,	1,
	`current version')
')

#
# Versioning numbers.
#
define(`DEFINE_VERSIONING_NUMBERS',`
_(VER_DEF_NONE,		0,
	`invalid version')
_(VER_DEF_CURRENT,	1, 
	`current version')
')

#
# Relocation types.
#
define(`DEFINE_386_RELOCATION_TYPES',`
__(`EM_386')
_(R_386_NONE,		0)
_(R_386_32,		1)
_(R_386_PC32,		2)
_(R_386_GOT32,		3)
_(R_386_PLT32,		4)
_(R_386_COPY,		5)
_(R_386_GLOB_DAT,	6)
_(R_386_JUMP_SLOT,	7)
_(R_386_RELATIVE,	8)
_(R_386_GOTOFF,		9)
_(R_386_GOTPC,		10)
_(R_386_32PLT,		11)
__(`	', `unused: 12-13')
_(R_386_TLS_TPOFF,	14)
_(R_386_TLS_IE,		15)
_(R_386_TLS_GOTIE,	16)
_(R_386_TLS_LE,		17)
_(R_386_TLS_GD,		18)
_(R_386_TLS_LDM,	19)
_(R_386_16,		20)
_(R_386_PC16,		21)
_(R_386_8,		22)
_(R_386_PC8,		23)
_(R_386_TLS_GD_32,	24)
_(R_386_TLS_GD_PUSH,	25)
_(R_386_TLS_GD_CALL,	26)
_(R_386_TLS_GD_POP,	27)
_(R_386_TLS_LDM_32,	28)
_(R_386_TLS_LDM_PUSH,	29)
_(R_386_TLS_LDM_CALL,	30)
_(R_386_TLS_LDM_POP,	31)
_(R_386_TLS_LDO_32,	32)
_(R_386_TLS_IE_32,	33)
_(R_386_TLS_LE_32,	34)
_(R_386_TLS_DTPMOD32,	35)
_(R_386_TLS_DTPOFF32,	36)
_(R_386_TLS_TPOFF32,	37)
_(R_386_SIZE32,		38)
_(R_386_TLS_GOTDESC,	39)
_(R_386_TLS_DESC_CALL,	40)
_(R_386_TLS_DESC,	41)
_(R_386_IRELATIVE,	42)
_(R_386_GOT32X,		43)
')

define(`DEFINE_386_RELOCATION_TYPE_SYNONYMS',`
_(R_386_JMP_SLOT, R_386_JUMP_SLOT)
')

define(`DEFINE_AARCH64_RELOCATION_TYPES',`
__(`EM_AARCH64')
_(R_AARCH64_NONE,				0)
_(R_AARCH64_P32_ABS32,				1)
_(R_AARCH64_P32_ABS16,				2)
_(R_AARCH64_P32_PREL32,				3)
_(R_AARCH64_P32_PREL16,				4)
_(R_AARCH64_P32_MOVW_UABS_G0,			5)
_(R_AARCH64_P32_MOVW_UABS_G0_NC,		6)
_(R_AARCH64_P32_MOVW_UABS_G1,			7)
_(R_AARCH64_P32_MOVW_SABS_G0,			8)
_(R_AARCH64_P32_LD_PREL_LO19,			9)
_(R_AARCH64_P32_ADR_PREL_LO21,			10)
_(R_AARCH64_P32_ADR_PREL_PG_HI21,		11)
_(R_AARCH64_P32_ADD_ABS_LO12_NC,		12)
_(R_AARCH64_P32_LDST8_ABS_LO12_NC,		13)
_(R_AARCH64_P32_LDST16_ABS_LO12_NC,		14)
_(R_AARCH64_P32_LDST32_ABS_LO12_NC,		15)
_(R_AARCH64_P32_LDST64_ABS_LO12_NC,		16)
_(R_AARCH64_P32_LDST128_ABS_LO12_NC,		17)
_(R_AARCH64_P32_TSTBR14,			18)
_(R_AARCH64_P32_CONDBR19,			19)
_(R_AARCH64_P32_JUMP26,				20)
_(R_AARCH64_P32_CALL26,				21)
_(R_AARCH64_P32_MOVW_PREL_G0,			22)
_(R_AARCH64_P32_MOVW_PREL_G0_NC,		23)
_(R_AARCH64_P32_MOVW_PREL_G1,			24)
_(R_AARCH64_P32_GOT_LD_PREL19,			25)
_(R_AARCH64_P32_ADR_GOT_PAGE,			26)
_(R_AARCH64_P32_LD32_GOT_LO12_NC,		27)
_(R_AARCH64_P32_LD32_GOTPAGE_LO14,		28)
_(R_AARCH64_P32_PLT32,				29)
__(`	', `Unused: 30-79.')
_(R_AARCH64_P32_TLSGD_ADR_PREL21,		80)
_(R_AARCH64_P32_TLSGD_ADR_PAGE21,		81)
_(R_AARCH64_P32_TLSGD_ADD_LO12_NC,		82)
_(R_AARCH64_P32_TLSLD_ADR_PREL21,		83)
_(R_AARCH64_P32_TLSLD_ADR_PAGE21,		84)
_(R_AARCH64_P32_TLSLD_ADD_LO12_NC,		85)
_(R_AARCH64_P32_TLSLD_LD_PREL19,		86)
_(R_AARCH64_P32_TLSLD_MOVW_DTPREL_G1,		87)
_(R_AARCH64_P32_TLSLD_MOVW_DTPREL_G0,		88)
_(R_AARCH64_P32_TLSLD_MOVW_DTPREL_G0_NC,	89)
_(R_AARCH64_P32_TLSLD_ADD_DTPREL_HI12,		90)
_(R_AARCH64_P32_TLSLD_ADD_DTPREL_LO12,		91)
_(R_AARCH64_P32_TLSLD_ADD_DTPREL_LO12_NC,	92)
_(R_AARCH64_P32_TLSLD_LDST8_DTPREL_LO12,	93)
_(R_AARCH64_P32_TLSLD_LDST8_DTPREL_LO12_NC,	94)
_(R_AARCH64_P32_TLSLD_LDST16_DTPREL_LO12,	95)
_(R_AARCH64_P32_TLSLD_LDST16_DTPREL_LO12_NC,	96)
_(R_AARCH64_P32_TLSLD_LDST32_DTPREL_LO12,	97)
_(R_AARCH64_P32_TLSLD_LDST32_DTPREL_LO12_NC,	98)
_(R_AARCH64_P32_TLSLD_LDST64_DTPREL_LO12,	99)
_(R_AARCH64_P32_TLSLD_LDST64_DTPREL_LO12_NC,	100)
_(R_AARCH64_P32_TLSLD_LDST128_DTPREL_LO12,	101)
_(R_AARCH64_P32_TLSLD_LDST128_DTPREL_LO12_NC,	102)
_(R_AARCH64_P32_TLSIE_ADR_GOTTPREL_PAGE21,	103)
_(R_AARCH64_P32_TLSIE_LD32_GOTTPREL_LO12_NC,	104)
_(R_AARCH64_P32_TLSIE_LD_GOTTPREL_PREL19,	105)
_(R_AARCH64_P32_TLSLE_MOVW_TPREL_G1,		106)
_(R_AARCH64_P32_TLSLE_MOVW_TPREL_G0,		107)
_(R_AARCH64_P32_TLSLE_MOVW_TPREL_G0_NC,		108)
_(R_AARCH64_P32_TLSLE_ADD_TPREL_HI12,		109)
_(R_AARCH64_P32_TLSLE_ADD_TPREL_LO12,		110)
_(R_AARCH64_P32_TLSLE_ADD_TPREL_LO12_NC,	111)
_(R_AARCH64_P32_TLSLE_LDST8_TPREL_LO12,		112)
_(R_AARCH64_P32_TLSLE_LDST8_TPREL_LO12_NC,	113)
_(R_AARCH64_P32_TLSLE_LDST16_TPREL_LO12,	114)
_(R_AARCH64_P32_TLSLE_LDST16_TPREL_LO12_NC,	115)
_(R_AARCH64_P32_TLSLE_LDST32_TPREL_LO12,	116)
_(R_AARCH64_P32_TLSLE_LDST32_TPREL_LO12_NC,	117)
_(R_AARCH64_P32_TLSLE_LDST64_TPREL_LO12,	118)
_(R_AARCH64_P32_TLSLE_LDST64_TPREL_LO12_NC,	119)
_(R_AARCH64_P32_TLSLE_LDST128_TPREL_LO12,	120)
_(R_AARCH64_P32_TLSLE_LDST128_TPREL_LO12_NC,	121)
_(R_AARCH64_P32_TLSDESC_LD_PREL19,		122)
_(R_AARCH64_P32_TLSDESC_ADR_PREL21,		123)
_(R_AARCH64_P32_TLSDESC_ADR_PAGE21,		124)
_(R_AARCH64_P32_TLSDESC_LD32_LO12,		125)
_(R_AARCH64_P32_TLSDESC_ADD_LO12,		126)
_(R_AARCH64_P32_TLSDESC_CALL,			127)
__(`	', `Unused: 128-179.')
_(R_AARCH64_P32_COPY,				180)
_(R_AARCH64_P32_GLOB_DAT,			181)
_(R_AARCH64_P32_JUMP_SLOT,			182)
_(R_AARCH64_P32_RELATIVE,			183)
_(R_AARCH64_P32_TLS_IMPDEF1,			184,
	`R_AARCH64_P32_TLS_DTPREL or R_AARCH64_P32_TLS_DTPMOD.')
_(R_AARCH64_P32_TLS_IMPDEF2,			185,
	`R_AARCH64_P32_TLS_DTPMOD or R_AARCH64_P32_TLS_DTPREL.')
_(R_AARCH64_P32_TLS_TPREL,			186)
_(R_AARCH64_P32_TLSDESC,			187)
_(R_AARCH64_P32_IRELATIVE,			188)
__(`	', `Unused: 189-256.')
_(R_AARCH64_ABS64,				257)
_(R_AARCH64_ABS32,				258)
_(R_AARCH64_ABS16,				259)
_(R_AARCH64_PREL64,				260)
_(R_AARCH64_PREL32,				261)
_(R_AARCH64_PREL16,				262)
_(R_AARCH64_MOVW_UABS_G0,			263)
_(R_AARCH64_MOVW_UABS_G0_NC,			264)
_(R_AARCH64_MOVW_UABS_G1,			265)
_(R_AARCH64_MOVW_UABS_G1_NC,			266)
_(R_AARCH64_MOVW_UABS_G2,			267)
_(R_AARCH64_MOVW_UABS_G2_NC,			268)
_(R_AARCH64_MOVW_UABS_G3,			269)
_(R_AARCH64_MOVW_SABS_G0,			270)
_(R_AARCH64_MOVW_SABS_G1,			271)
_(R_AARCH64_MOVW_SABS_G2,			272)
_(R_AARCH64_LD_PREL_LO19,			273)
_(R_AARCH64_ADR_PREL_LO21,			274)
_(R_AARCH64_ADR_PREL_PG_HI21,			275)
_(R_AARCH64_ADR_PREL_PG_HI21_NC,		276)
_(R_AARCH64_ADD_ABS_LO12_NC,			277)
_(R_AARCH64_LDST8_ABS_LO12_NC,			278)
_(R_AARCH64_TSTBR14,				279)
_(R_AARCH64_CONDBR19,				280)
__(`	', `unused: 281')
_(R_AARCH64_JUMP26,				282)
_(R_AARCH64_CALL26,				283)
_(R_AARCH64_LDST16_ABS_LO12_NC,			284)
_(R_AARCH64_LDST32_ABS_LO12_NC,			285)
_(R_AARCH64_LDST64_ABS_LO12_NC,			286)
_(R_AARCH64_MOVW_PREL_G0,			287)
_(R_AARCH64_MOVW_PREL_G0_NC,			288)
_(R_AARCH64_MOVW_PREL_G1,			289)
_(R_AARCH64_MOVW_PREL_G1_NC,			290)
_(R_AARCH64_MOVW_PREL_G2,			291)
_(R_AARCH64_MOVW_PREL_G2_NC,			292)
_(R_AARCH64_MOVW_PREL_G3,			293)
__(`	', `unused: 294-298')
_(R_AARCH64_LDST128_ABS_LO12_NC,		299)
_(R_AARCH64_MOVW_GOTOFF_G0,			300)
_(R_AARCH64_MOVW_GOTOFF_G0_NC,			301)
_(R_AARCH64_MOVW_GOTOFF_G1,			302)
_(R_AARCH64_MOVW_GOTOFF_G1_NC,			303)
_(R_AARCH64_MOVW_GOTOFF_G2,			304)
_(R_AARCH64_MOVW_GOTOFF_G2_NC,			305)
_(R_AARCH64_MOVW_GOTOFF_G3,			306)
_(R_AARCH64_GOTREL64,				307)
_(R_AARCH64_GOTREL32,				308)
_(R_AARCH64_GOT_LD_PREL19,			309)
_(R_AARCH64_LD64_GOTOFF_LO15,			310)
_(R_AARCH64_ADR_GOT_PAGE,			311)
_(R_AARCH64_LD64_GOT_LO12_NC,			312)
_(R_AARCH64_LD64_GOTPAGE_LO15,			313)
_(R_AARCH64_PLT32,				314)
_(R_AARCH64_GOTPCREL32,				315)
__(`	', `unused: 316-511')
_(R_AARCH64_TLSGD_ADR_PREL21,			512)
_(R_AARCH64_TLSGD_ADR_PAGE21,			513)
_(R_AARCH64_TLSGD_ADD_LO12_NC,			514)
_(R_AARCH64_TLSGD_MOVW_G1,			515)
_(R_AARCH64_TLSGD_MOVW_G0_NC,			516)
_(R_AARCH64_TLSLD_ADR_PREL21,			517)
_(R_AARCH64_TLSLD_ADR_PAGE21,			518)
_(R_AARCH64_TLSLD_ADD_LO12_NC,			519)
_(R_AARCH64_TLSLD_MOVW_G1,			520)
_(R_AARCH64_TLSLD_MOVW_G0_NC,			521)
_(R_AARCH64_TLSLD_LD_PREL19,			522)
_(R_AARCH64_TLSLD_MOVW_DTPREL_G2,		523)
_(R_AARCH64_TLSLD_MOVW_DTPREL_G1,		524)
_(R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC,		525)
_(R_AARCH64_TLSLD_MOVW_DTPREL_G0,		526)
_(R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC,		527)
_(R_AARCH64_TLSLD_ADD_DTPREL_HI12,		528)
_(R_AARCH64_TLSLD_ADD_DTPREL_LO12,		529)
_(R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC,		530)
_(R_AARCH64_TLSLD_LDST8_DTPREL_LO12,		531)
_(R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC,		532)
_(R_AARCH64_TLSLD_LDST16_DTPREL_LO12,		533)
_(R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC,	534)
_(R_AARCH64_TLSLD_LDST32_DTPREL_LO12,		535)
_(R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC,	536)
_(R_AARCH64_TLSLD_LDST64_DTPREL_LO12,		537)
_(R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC,	538)
_(R_AARCH64_TLSIE_MOVW_GOTTPREL_G1,		539)
_(R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC,		540)
_(R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21,		541)
_(R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC,	542)
_(R_AARCH64_TLSIE_LD_GOTTPREL_PREL19,		543)
_(R_AARCH64_TLSLE_MOVW_TPREL_G2,		544)
_(R_AARCH64_TLSLE_MOVW_TPREL_G1,		545)
_(R_AARCH64_TLSLE_MOVW_TPREL_G1_NC,		546)
_(R_AARCH64_TLSLE_MOVW_TPREL_G0,		547)
_(R_AARCH64_TLSLE_MOVW_TPREL_G0_NC,		548)
_(R_AARCH64_TLSLE_ADD_TPREL_HI12,		549)
_(R_AARCH64_TLSLE_ADD_TPREL_LO12,		550)
_(R_AARCH64_TLSLE_ADD_TPREL_LO12_NC,		551)
_(R_AARCH64_TLSLE_LDST8_TPREL_LO12,		552)
_(R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC,		553)
_(R_AARCH64_TLSLE_LDST16_TPREL_LO12,		554)
_(R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC,		555)
_(R_AARCH64_TLSLE_LDST32_TPREL_LO12,		556)
_(R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC,		557)
_(R_AARCH64_TLSLE_LDST64_TPREL_LO12,		558)
_(R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC,		559)
_(R_AARCH64_TLSDESC_LD_PREL19,			560)
_(R_AARCH64_TLSDESC_ADR_PREL21,			561)
_(R_AARCH64_TLSDESC_ADR_PAGE21,			562)
_(R_AARCH64_TLSDESC_LD64_LO12,			563)
_(R_AARCH64_TLSDESC_ADD_LO12,			564)
_(R_AARCH64_TLSDESC_OFF_G1,			565)
_(R_AARCH64_TLSDESC_OFF_G0_NC,			566)
_(R_AARCH64_TLSDESC_LDR,			567)
_(R_AARCH64_TLSDESC_ADD,			568)
_(R_AARCH64_TLSDESC_CALL,			569)
_(R_AARCH64_TLSLE_LDST128_TPREL_LO12,		570)
_(R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC,	571)
_(R_AARCH64_TLSLD_LDST128_DTPREL_LO12,		572)
_(R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC,	573)
__(`	', `unused: 574-579')
_(R_AARCH64_AUTH_ABS64,				580)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G0,		581)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G0_NC,		582)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G1,		583)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G1_NC,		584)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G2,		585)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G2_NC,		586)
_(R_AARCH64_AUTH_MOVW_GOTOFF_G3,		587)
_(R_AARCH64_AUTH_GOT_LD_PREL19,			588)
_(R_AARCH64_AUTH_LD64_GOTOFF_LO15,		589)
_(R_AARCH64_AUTH_ADR_GOT_PAGE,			590)
_(R_AARCH64_AUTH_LD64_GOT_LO12_NC,		591)
_(R_AARCH64_AUTH_LD64_GOTPAGE_LO15,		592)
_(R_AARCH64_AUTH_GOT_ADD_LO12_NC,		593)
_(R_AARCH64_AUTH_GOT_ADR_PREL_LO21,		594)
_(R_AARCH64_AUTH_TLSDESC_ADR_PAGE21,		595)
_(R_AARCH64_AUTH_TLSDESC_LD64_LO12,		596)
_(R_AARCH64_AUTH_TLSDESC_ADD_LO12,		597)
__(`	', `unused: 598-1023')
_(R_AARCH64_COPY,				1024)
_(R_AARCH64_GLOB_DAT,				1025)
_(R_AARCH64_JUMP_SLOT,				1026)
_(R_AARCH64_RELATIVE,				1027)
_(R_AARCH64_TLS_IMPDEF1,			1028,
	`R_AARCH64_TLS_DTPREL or R_AARCH64_TLS_DTPMOD.')
_(R_AARCH64_TLS_IMPDEF2,			1029,
	`R_AARCH64_TLS_DTPMOD or R_AARCH64_TLS_DTPREL.')
_(R_AARCH64_TLS_TPREL,				1030)
_(R_AARCH64_TLSDESC,				1031)
_(R_AARCH64_IRELATIVE,				1032)
__(`	', `unused: 1033-1040')
_(R_AARCH64_AUTH_RELATIVE,			1041)
_(R_AARCH64_AUTH_GLOB_DAT,			1042)
_(R_AARCH64_AUTH_TLSDESC,			1043)
_(R_AARCH64_AUTH_IRELATIVE,			1044)
')

define(`DEFINE_AARCH64_RELOCATION_TYPE_SYNONYMS',`
_(R_AARCH64_TLS_TPREL64,			R_AARCH64_TLS_TPREL)
')

#
# Relocation definitions from the ARM ELF ABI, version "ARM IHI
# 0044E" released on 30th November 2012.
#
define(`DEFINE_ARM_RELOCATION_TYPES',`
__(`EM_ARM')
_(R_ARM_NONE,			0)
_(R_ARM_PC24,			1, `Deprecated.')
_(R_ARM_ABS32,			2)
_(R_ARM_REL32,			3)
_(R_ARM_LDR_PC_G0,		4)
_(R_ARM_ABS16,			5)
_(R_ARM_ABS12,			6)
_(R_ARM_THM_ABS5,		7)
_(R_ARM_ABS8,			8)
_(R_ARM_SBREL32,		9)
_(R_ARM_THM_CALL,		10)
_(R_ARM_THM_PC8,		11)
_(R_ARM_BREL_ADJ,		12)
_(R_ARM_TLS_DESC,		13)
_(R_ARM_THM_SWI8,		14, `Obsolete.')
_(R_ARM_XPC25,			15, `Obsolete.')
_(R_ARM_THM_XPC22,		16, `Obsolete.')
_(R_ARM_TLS_DTPMOD32,		17)
_(R_ARM_TLS_DTPOFF32,		18)
_(R_ARM_TLS_TPOFF32,		19)
_(R_ARM_COPY,			20)
_(R_ARM_GLOB_DAT,		21)
_(R_ARM_JUMP_SLOT,		22)
_(R_ARM_RELATIVE,		23)
_(R_ARM_GOTOFF32,		24)
_(R_ARM_BASE_PREL,		25)
_(R_ARM_GOT_BREL,		26)
_(R_ARM_PLT32,			27, `Deprecated.')
_(R_ARM_CALL,			28)
_(R_ARM_JUMP24,			29)
_(R_ARM_THM_JUMP24,		30)
_(R_ARM_BASE_ABS,		31)
_(R_ARM_ALU_PCREL_7_0,		32, `Obsolete.')
_(R_ARM_ALU_PCREL_15_8,		33, `Obsolete.')
_(R_ARM_ALU_PCREL_23_15,	34, `Obsolete.')
_(R_ARM_LDR_SBREL_11_0_NC,	35, `Deprecated.')
_(R_ARM_ALU_SBREL_19_12_NC,	36, `Deprecated.')
_(R_ARM_ALU_SBREL_27_20_CK,	37, `Deprecated.')
_(R_ARM_TARGET1,		38)
_(R_ARM_SBREL31,		39, `Deprecated.')
_(R_ARM_V4BX,			40)
_(R_ARM_TARGET2,		41)
_(R_ARM_PREL31,			42)
_(R_ARM_MOVW_ABS_NC,		43)
_(R_ARM_MOVT_ABS,		44)
_(R_ARM_MOVW_PREL_NC,		45)
_(R_ARM_MOVT_PREL,		46)
_(R_ARM_THM_MOVW_ABS_NC,	47)
_(R_ARM_THM_MOVT_ABS,		48)
_(R_ARM_THM_MOVW_PREL_NC,	49)
_(R_ARM_THM_MOVT_PREL,		50)
_(R_ARM_THM_JUMP19,		51)
_(R_ARM_THM_JUMP6,		52)
_(R_ARM_THM_ALU_PREL_11_0,	53)
_(R_ARM_THM_PC12,		54)
_(R_ARM_ABS32_NOI,		55)
_(R_ARM_REL32_NOI,		56)
_(R_ARM_ALU_PC_G0_NC,		57)
_(R_ARM_ALU_PC_G0,		58)
_(R_ARM_ALU_PC_G1_NC,		59)
_(R_ARM_ALU_PC_G1,		60)
_(R_ARM_ALU_PC_G2,		61)
_(R_ARM_LDR_PC_G1,		62)
_(R_ARM_LDR_PC_G2,		63)
_(R_ARM_LDRS_PC_G0,		64)
_(R_ARM_LDRS_PC_G1,		65)
_(R_ARM_LDRS_PC_G2,		66)
_(R_ARM_LDC_PC_G0,		67)
_(R_ARM_LDC_PC_G1,		68)
_(R_ARM_LDC_PC_G2,		69)
_(R_ARM_ALU_SB_G0_NC,		70)
_(R_ARM_ALU_SB_G0,		71)
_(R_ARM_ALU_SB_G1_NC,		72)
_(R_ARM_ALU_SB_G1,		73)
_(R_ARM_ALU_SB_G2,		74)
_(R_ARM_LDR_SB_G0,		75)
_(R_ARM_LDR_SB_G1,		76)
_(R_ARM_LDR_SB_G2,		77)
_(R_ARM_LDRS_SB_G0,		78)
_(R_ARM_LDRS_SB_G1,		79)
_(R_ARM_LDRS_SB_G2,		80)
_(R_ARM_LDC_SB_G0,		81)
_(R_ARM_LDC_SB_G1,		82)
_(R_ARM_LDC_SB_G2,		83)
_(R_ARM_MOVW_BREL_NC,		84)
_(R_ARM_MOVT_BREL,		85)
_(R_ARM_MOVW_BREL,		86)
_(R_ARM_THM_MOVW_BREL_NC,	87)
_(R_ARM_THM_MOVT_BREL,		88)
_(R_ARM_THM_MOVW_BREL,		89)
_(R_ARM_TLS_GOTDESC,		90)
_(R_ARM_TLS_CALL,		91)
_(R_ARM_TLS_DESCSEQ,		92)
_(R_ARM_THM_TLS_CALL,		93)
_(R_ARM_PLT32_ABS,		94)
_(R_ARM_GOT_ABS,		95)
_(R_ARM_GOT_PREL,		96)
_(R_ARM_GOT_BREL12,		97)
_(R_ARM_GOTOFF12,		98)
_(R_ARM_GOTRELAX,		99)
_(R_ARM_GNU_VTENTRY,		100, `Deprecated.')
_(R_ARM_GNU_VTINHERIT,		101, `Deprecated.')
_(R_ARM_THM_JUMP11,		102)
_(R_ARM_THM_JUMP8,		103)
_(R_ARM_TLS_GD32,		104)
_(R_ARM_TLS_LDM32,		105)
_(R_ARM_TLS_LDO32,		106)
_(R_ARM_TLS_IE32,		107)
_(R_ARM_TLS_LE32,		108)
_(R_ARM_TLS_LDO12,		109)
_(R_ARM_TLS_LE12,		110)
_(R_ARM_TLS_IE12GP,		111)
_(R_ARM_PRIVATE_0,		112)
_(R_ARM_PRIVATE_1,		113)
_(R_ARM_PRIVATE_2,		114)
_(R_ARM_PRIVATE_3,		115)
_(R_ARM_PRIVATE_4,		116)
_(R_ARM_PRIVATE_5,		117)
_(R_ARM_PRIVATE_6,		118)
_(R_ARM_PRIVATE_7,		119)
_(R_ARM_PRIVATE_8,		120)
_(R_ARM_PRIVATE_9,		121)
_(R_ARM_PRIVATE_10,		122)
_(R_ARM_PRIVATE_11,		123)
_(R_ARM_PRIVATE_12,		124)
_(R_ARM_PRIVATE_13,		125)
_(R_ARM_PRIVATE_14,		126)
_(R_ARM_PRIVATE_15,		127)
_(R_ARM_ME_TOO,			128, `Obsolete.')
_(R_ARM_THM_TLS_DESCSEQ16,	129)
_(R_ARM_THM_TLS_DESCSEQ32,	130)
_(R_ARM_THM_GOT_BREL12,		131)
_(R_ARM_THM_ALU_ABS_G0_NC,	132)
_(R_ARM_THM_ALU_ABS_G1_NC,	133)
_(R_ARM_THM_ALU_ABS_G2_NC,	134)
_(R_ARM_THM_ALU_ABS_G3,		135)
_(R_ARM_THM_BF16,		136)
_(R_ARM_THM_BF12,		137)
_(R_ARM_THM_BF18,		138)
__(`	', `Reserved: 139-159.')
_(R_ARM_IRELATIVE,		160)
_(R_ARM_PRIVATE_16,		161)
_(R_ARM_PRIVATE_17,		162)
_(R_ARM_PRIVATE_18,		163)
_(R_ARM_PRIVATE_19,		164)
_(R_ARM_PRIVATE_20,		165)
_(R_ARM_PRIVATE_21,		166)
_(R_ARM_PRIVATE_22,		167)
_(R_ARM_PRIVATE_23,		168)
_(R_ARM_PRIVATE_24,		169)
_(R_ARM_PRIVATE_25,		170)
_(R_ARM_PRIVATE_26,		171)
_(R_ARM_PRIVATE_27,		172)
_(R_ARM_PRIVATE_28,		173)
_(R_ARM_PRIVATE_29,		174)
_(R_ARM_PRIVATE_30,		175)
_(R_ARM_PRIVATE_31,		176)
__(`	', `Reserved: 177-255.')
')

define(`DEFINE_ARM_OBSOLETE_RELOCATION_TYPES',`
_(R_ARM_PC13, 	       	     	4)
_(R_ARM_THM_PC22,		10)
_(R_ARM_AMP_VCALL9,		12)
_(R_ARM_SWI24,			13)
_(R_ARM_GOTOFF,			24)
_(R_ARM_GOTPC,			25)
_(R_ARM_GOT32,			26)
_(R_ARM_THM_PC11,		102)
_(R_ARM_THM_PC9,		103)
')

define(`DEFINE_IA_64_RELOCATION_TYPES',`
__(`EM_IA_64')
_(R_IA_64_NONE,			0)
__(`	', `unused: 0x1-0x20')
_(R_IA_64_IMM14,		0x21)
_(R_IA_64_IMM22,		0x22)
_(R_IA_64_IMM64,		0x23)
_(R_IA_64_DIR32MSB,		0x24)
_(R_IA_64_DIR32LSB,		0x25)
_(R_IA_64_DIR64MSB,		0x26)
_(R_IA_64_DIR64LSB,		0x27)
__(`	', `unused: 0x28-0x29')
_(R_IA_64_GPREL22,		0x2a)
_(R_IA_64_GPREL64I,		0x2b)
_(R_IA_64_GPREL32MSB,		0x2c)
_(R_IA_64_GPREL32LSB,		0x2d)
_(R_IA_64_GPREL64MSB,		0x2e)
_(R_IA_64_GPREL64LSB,		0x2f)
__(`	', `unused: 0x30-0x31')
_(R_IA_64_LTOFF22,		0x32)
_(R_IA_64_LTOFF64I,		0x33)
__(`	', `unused: 0x34-0x39')
_(R_IA_64_PLTOFF22,		0x3a)
_(R_IA_64_PLTOFF64I,		0x3b)
__(`	', `unused: 0x3c-0x3d')
_(R_IA_64_PLTOFF64MSB,		0x3e)
_(R_IA_64_PLTOFF64LSB,		0x3f)
__(`	', `unused: 0x40-0x42')
_(R_IA_64_FPTR64I,		0x43)
_(R_IA_64_FPTR32MSB,		0x44)
_(R_IA_64_FPTR32LSB,		0x45)
_(R_IA_64_FPTR64MSB,		0x46)
_(R_IA_64_FPTR64LSB,		0x47)
_(R_IA_64_PCREL60B,		0x48)
_(R_IA_64_PCREL21B,		0x49)
_(R_IA_64_PCREL21M,		0x4a)
_(R_IA_64_PCREL21F,		0x4b)
_(R_IA_64_PCREL32MSB,		0x4c)
_(R_IA_64_PCREL32LSB,		0x4d)
_(R_IA_64_PCREL64MSB,		0x4e)
_(R_IA_64_PCREL64LSB,		0x4f)
__(`	', `unused: 0x50-0x51')
_(R_IA_64_LTOFF_FPTR22,		0x52)
_(R_IA_64_LTOFF_FPTR64I,	0x53)
_(R_IA_64_LTOFF_FPTR32MSB,	0x54)
_(R_IA_64_LTOFF_FPTR32LSB,	0x55)
_(R_IA_64_LTOFF_FPTR64MSB,	0x56)
_(R_IA_64_LTOFF_FPTR64LSB,	0x57)
__(`	', `unused: 0x58-0x5b')
_(R_IA_64_SEGREL32MSB,		0x5c)
_(R_IA_64_SEGREL32LSB,		0x5d)
_(R_IA_64_SEGREL64MSB,		0x5e)
_(R_IA_64_SEGREL64LSB,		0x5f)
__(`	', `unused: 0x60-0x63')
_(R_IA_64_SECREL32MSB,		0x64)
_(R_IA_64_SECREL32LSB,		0x65)
_(R_IA_64_SECREL64MSB,		0x66)
_(R_IA_64_SECREL64LSB,		0x67)
__(`	', `unused: 0x68-0x6b')
_(R_IA_64_REL32MSB,		0x6c)
_(R_IA_64_REL32LSB,		0x6d)
_(R_IA_64_REL64MSB,		0x6e)
_(R_IA_64_REL64LSB,		0x6f)
__(`	', `unused: 0x70-0x73')
_(R_IA_64_LTV32MSB,		0x74)
_(R_IA_64_LTV32LSB,		0x75)
_(R_IA_64_LTV64MSB,		0x76)
_(R_IA_64_LTV64LSB,		0x77)
__(`	', `unused: 0x78')
_(R_IA_64_PCREL21BI,		0x79)
_(R_IA_64_PCREL22,		0x7A)
_(R_IA_64_PCREL64I,		0x7B)
__(`	', `unused: 0x7C-0x7F')
_(R_IA_64_IPLTMSB,		0x80)
_(R_IA_64_IPLTLSB,		0x81)
__(`	', `unused: 0x82-0x84')
_(R_IA_64_SUB,			0x85)
_(R_IA_64_LTOFF22X,		0x86)
_(R_IA_64_LDXMOV,		0x87)
__(`	', `unused: 0x88-0x90')
_(R_IA_64_TPREL14,		0x91)
_(R_IA_64_TPREL22,		0x92)
_(R_IA_64_TPREL64I,		0x93)
__(`	', `unused: 0x94-0x95')
_(R_IA_64_TPREL64MSB,		0x96)
_(R_IA_64_TPREL64LSB,		0x97)
__(`	', `unused: 0x98-0x99')
_(R_IA_64_LTOFF_TPREL22,	0x9A)
__(`	', `unused: 0x9B-0xA5')
_(R_IA_64_DTPMOD64MSB,		0xA6)
_(R_IA_64_DTPMOD64LSB,		0xA7)
__(`	', `unused: 0xA8-0xA9')
_(R_IA_64_LTOFF_DTPMOD22,	0xAA)
__(`	', `unused: 0xAB-0xB0')
_(R_IA_64_DTPREL14,		0xB1)
_(R_IA_64_DTPREL22,		0xB2)
_(R_IA_64_DTPREL64I,		0xB3)
_(R_IA_64_DTPREL32MSB,		0xB4)
_(R_IA_64_DTPREL32LSB,		0xB5)
_(R_IA_64_DTPREL64MSB,		0xB6)
_(R_IA_64_DTPREL64LSB,		0xB7)
__(`	', `unused: 0xB8-0xB9')
_(R_IA_64_LTOFF_DTPREL22,	0xBA)
')

define(`DEFINE_IA_64_RELOCATION_TYPE_SYNONYMS',`
_(R_IA64_NONE,			R_IA_64_NONE)
_(R_IA64_IMM14,			R_IA_64_IMM14)
_(R_IA64_IMM22,			R_IA_64_IMM22)
_(R_IA64_IMM64,			R_IA_64_IMM64)
_(R_IA64_DIR32MSB,		R_IA_64_DIR32MSB)
_(R_IA64_DIR32LSB,		R_IA_64_DIR32LSB)
_(R_IA64_DIR64MSB,		R_IA_64_DIR64MSB)
_(R_IA64_DIR64LSB,		R_IA_64_DIR64LSB)
_(R_IA64_GPREL22,		R_IA_64_GPREL22)
_(R_IA64_GPREL64I,		R_IA_64_GPREL64I)
_(R_IA64_GPREL64MSB,		R_IA_64_GPREL64MSB)
_(R_IA64_GPREL64LSB,		R_IA_64_GPREL64LSB)
_(R_IA64_LTOFF22,		R_IA_64_LTOFF22)
_(R_IA64_LTOFF64I,		R_IA_64_LTOFF64I)
_(R_IA64_PLTOFF22,		R_IA_64_PLTOFF22)
_(R_IA64_PLTOFF64I,		R_IA_64_PLTOFF64I)
_(R_IA64_PLTOFF64MSB,		R_IA_64_PLTOFF64MSB)
_(R_IA64_PLTOFF64LSB,		R_IA_64_PLTOFF64LSB)
_(R_IA64_FPTR64I,		R_IA_64_FPTR64I)
_(R_IA64_FPTR32MSB,		R_IA_64_FPTR32MSB)
_(R_IA64_FPTR32LSB,		R_IA_64_FPTR32LSB)
_(R_IA64_FPTR64MSB,		R_IA_64_FPTR64MSB)
_(R_IA64_FPTR64LSB,		R_IA_64_FPTR64LSB)
_(R_IA64_PCREL21B,		R_IA_64_PCREL21B)
_(R_IA64_PCREL21M,		R_IA_64_PCREL21M)
_(R_IA64_PCREL21F,		R_IA_64_PCREL21F)
_(R_IA64_PCREL32MSB,		R_IA_64_PCREL32MSB)
_(R_IA64_PCREL32LSB,		R_IA_64_PCREL32LSB)
_(R_IA64_PCREL64MSB,		R_IA_64_PCREL64MSB)
_(R_IA64_PCREL64LSB,		R_IA_64_PCREL64LSB)
_(R_IA64_LTOFF_FPTR22,		R_IA_64_LTOFF_FPTR22)
_(R_IA64_LTOFF_FPTR64I,		R_IA_64_LTOFF_FPTR64I)
_(R_IA64_LTOFF_FPTR32MSB,	R_IA_64_LTOFF_FPTR32MSB)
_(R_IA64_LTOFF_FPTR32LSB,	R_IA_64_LTOFF_FPTR32LSB)
_(R_IA64_LTOFF_FPTR64MSB,	R_IA_64_LTOFF_FPTR64MSB)
_(R_IA64_LTOFF_FPTR64LSB,	R_IA_64_LTOFF_FPTR64LSB)
_(R_IA64_SEGREL32MSB,		R_IA_64_SEGREL32MSB)
_(R_IA64_SEGREL32LSB,		R_IA_64_SEGREL32LSB)
_(R_IA64_SEGREL64MSB,		R_IA_64_SEGREL64MSB)
_(R_IA64_SEGREL64LSB,		R_IA_64_SEGREL64LSB)
_(R_IA64_SECREL32MSB,		R_IA_64_SECREL32MSB)
_(R_IA64_SECREL32LSB,		R_IA_64_SECREL32LSB)
_(R_IA64_SECREL64MSB,		R_IA_64_SECREL64MSB)
_(R_IA64_SECREL64LSB,		R_IA_64_SECREL64LSB)
_(R_IA64_REL32MSB,		R_IA_64_REL32MSB)
_(R_IA64_REL32LSB,		R_IA_64_REL32LSB)
_(R_IA64_REL64MSB,		R_IA_64_REL64MSB)
_(R_IA64_REL64LSB,		R_IA_64_REL64LSB)
_(R_IA64_LTV32MSB,		R_IA_64_LTV32MSB)
_(R_IA64_LTV32LSB,		R_IA_64_LTV32LSB)
_(R_IA64_LTV64MSB,		R_IA_64_LTV64MSB)
_(R_IA64_LTV64LSB,		R_IA_64_LTV64LSB)
_(R_IA64_IPLTMSB,		R_IA_64_IPLTMSB)
_(R_IA64_IPLTLSB,		R_IA_64_IPLTLSB)
_(R_IA64_SUB,			R_IA_64_SUB)
_(R_IA64_LTOFF22X,		R_IA_64_LTOFF22X)
_(R_IA64_LDXMOV,		R_IA_64_LDXMOV)
_(R_IA64_TPREL14,		R_IA_64_TPREL14)
_(R_IA64_TPREL22,		R_IA_64_TPREL22)
_(R_IA64_TPREL64I,		R_IA_64_TPREL64I)
_(R_IA64_TPREL64MSB,		R_IA_64_TPREL64MSB)
_(R_IA64_TPREL64LSB,		R_IA_64_TPREL64LSB)
_(R_IA64_LTOFF_TPREL22,		R_IA_64_LTOFF_TPREL22)
_(R_IA64_DTPMOD64MSB,		R_IA_64_DTPMOD64MSB)
_(R_IA64_DTPMOD64LSB,		R_IA_64_DTPMOD64LSB)
_(R_IA64_LTOFF_DTPMOD22,	R_IA_64_LTOFF_DTPMOD22)
_(R_IA64_DTPREL14,		R_IA_64_DTPREL14)
_(R_IA64_DTPREL22,		R_IA_64_DTPREL22)
_(R_IA64_DTPREL64I,		R_IA_64_DTPREL64I)
_(R_IA64_DTPREL32MSB,		R_IA_64_DTPREL32MSB)
_(R_IA64_DTPREL32LSB,		R_IA_64_DTPREL32LSB)
_(R_IA64_DTPREL64MSB,		R_IA_64_DTPREL64MSB)
_(R_IA64_DTPREL64LSB,		R_IA_64_DTPREL64LSB)
_(R_IA64_LTOFF_DTPREL22,	R_IA_64_LTOFF_DTPREL22)
')

define(`DEFINE_LOONGARCH_RELOCATION_TYPES',`
__(`EM_LOONGARCH')
_(R_LARCH_NONE,				0)
_(R_LARCH_32,				1)
_(R_LARCH_64,				2)
_(R_LARCH_RELATIVE,			3)
_(R_LARCH_COPY,				4)
_(R_LARCH_JUMP_SLOT,			5)
_(R_LARCH_TLS_DTPMOD32,			6)
_(R_LARCH_TLS_DTPMOD64,			7)
_(R_LARCH_TLS_DTPREL32,			8)
_(R_LARCH_TLS_DTPREL64,			9)
_(R_LARCH_TLS_TPREL32,			10)
_(R_LARCH_TLS_TPREL64,			11)
_(R_LARCH_IRELATIVE,			12)
_(R_LARCH_TLS_DESC32,			13)
_(R_LARCH_TLS_DESC64,			14)
__(`	', `reserved for the dynamic linker: 15-19')
_(R_LARCH_MARK_LA,			20)
_(R_LARCH_MARK_PCREL,			21)
_(R_LARCH_SOP_PUSH_PCREL,		22)
_(R_LARCH_SOP_PUSH_ABSOLUTE,		23)
_(R_LARCH_SOP_PUSH_DUP,			24)
_(R_LARCH_SOP_PUSH_GPREL,		25)
_(R_LARCH_SOP_PUSH_TLS_TPREL,		26)
_(R_LARCH_SOP_PUSH_TLS_GOT,		27)
_(R_LARCH_SOP_PUSH_TLS_GD,		28)
_(R_LARCH_SOP_PUSH_PLT_PCREL,		29)
_(R_LARCH_SOP_ASSERT,			30)
_(R_LARCH_SOP_NOT,			31)
_(R_LARCH_SOP_SUB,			32)
_(R_LARCH_SOP_SL,			33)
_(R_LARCH_SOP_SR,			34)
_(R_LARCH_SOP_ADD,			35)
_(R_LARCH_SOP_AND,			36)
_(R_LARCH_SOP_IF_ELSE,			37)
_(R_LARCH_SOP_POP_32_S_10_5,		38)
_(R_LARCH_SOP_POP_32_U_10_12,		39)
_(R_LARCH_SOP_POP_32_S_10_12,		40)
_(R_LARCH_SOP_POP_32_S_10_16,		41)
_(R_LARCH_SOP_POP_32_S_10_16_S2,	42)
_(R_LARCH_SOP_POP_32_S_5_20,		43)
_(R_LARCH_SOP_POP_32_S_0_5_10_16_S2,	44)
_(R_LARCH_SOP_POP_32_S_0_10_10_16_S2,	45)
_(R_LARCH_SOP_POP_32_U,			46)
_(R_LARCH_ADD8,				47)
_(R_LARCH_ADD16,			48)
_(R_LARCH_ADD24,			49)
_(R_LARCH_ADD32,			50)
_(R_LARCH_ADD64,			51)
_(R_LARCH_SUB8,				52)
_(R_LARCH_SUB16,			53)
_(R_LARCH_SUB24,			54)
_(R_LARCH_SUB32,			55)
_(R_LARCH_SUB64,			56)
_(R_LARCH_GNU_VTINHERIT,		57)
_(R_LARCH_GNU_VTENTRY,			58)
__(`	', `reserved: 59-63')
_(R_LARCH_B16,				64)
_(R_LARCH_B21,				65)
_(R_LARCH_B26,				66)
_(R_LARCH_ABS_HI20,			67)
_(R_LARCH_ABS_LO12,			68)
_(R_LARCH_ABS64_LO20,			69)
_(R_LARCH_ABS64_HI12,			70)
_(R_LARCH_PCALA_HI20,			71)
_(R_LARCH_PCALA_LO12,			72)
_(R_LARCH_PCALA64_LO20,			73)
_(R_LARCH_PCALA64_HI12,			74)
_(R_LARCH_GOT_PC_HI20,			75)
_(R_LARCH_GOT_PC_LO12,			76)
_(R_LARCH_GOT64_PC_LO20,		77)
_(R_LARCH_GOT64_PC_HI12,		78)
_(R_LARCH_GOT_HI20,			79)
_(R_LARCH_GOT_LO12,			80)
_(R_LARCH_GOT64_LO20,			81)
_(R_LARCH_GOT64_HI12,			82)
_(R_LARCH_TLS_LE_HI20,			83)
_(R_LARCH_TLS_LE_LO12,			84)
_(R_LARCH_TLS_LE64_LO20,		85)
_(R_LARCH_TLS_LE64_HI12,		86)
_(R_LARCH_TLS_IE_PC_HI20,		87)
_(R_LARCH_TLS_IE_PC_LO12,		88)
_(R_LARCH_TLS_IE64_PC_LO20,		89)
_(R_LARCH_TLS_IE64_PC_HI12,		90)
_(R_LARCH_TLS_IE_HI20,			91)
_(R_LARCH_TLS_IE_LO12,			92)
_(R_LARCH_TLS_IE64_LO20,		93)
_(R_LARCH_TLS_IE64_HI12,		94)
_(R_LARCH_TLS_LD_PC_HI20,		95)
_(R_LARCH_TLS_LD_HI20,			96)
_(R_LARCH_TLS_GD_PC_HI20,		97)
_(R_LARCH_TLS_GD_HI20,			98)
_(R_LARCH_32_PCREL,			99)
_(R_LARCH_RELAX,			100)
__(`	', `reserved: 101')
_(R_LARCH_ALIGN,			102)
_(R_LARCH_PCREL20_S2,			103)
__(`	', `reserved: 104')
_(R_LARCH_ADD6,				105)
_(R_LARCH_SUB6,				106)
_(R_LARCH_ADD_ULEB128,			107)
_(R_LARCH_SUB_ULEB128,			108)
_(R_LARCH_64_PCREL,			109)
_(R_LARCH_CALL36,			110)
_(R_LARCH_TLS_DESC_PC_HI20,		111)
_(R_LARCH_TLS_DESC_PC_LO12,		112)
_(R_LARCH_TLS_DESC64_PC_LO20,		113)
_(R_LARCH_TLS_DESC64_PC_HI12,		114)
_(R_LARCH_TLS_DESC_HI20,		115)
_(R_LARCH_TLS_DESC_LO12,		116)
_(R_LARCH_TLS_DESC64_LO20,		117)
_(R_LARCH_TLS_DESC64_HI12,		118)
_(R_LARCH_TLS_DESC_LD,			119)
_(R_LARCH_TLS_DESC_CALL,		120)
_(R_LARCH_TLS_LE_HI20_R,		121)
_(R_LARCH_TLS_LE_ADD_R,			122)
_(R_LARCH_TLS_LE_LO12_R,		123)
_(R_LARCH_TLS_LD_PCREL20_S2,		124)
_(R_LARCH_TLS_GD_PCREL20_S2,		125)
_(R_LARCH_TLS_DESC_PCREL20_S2,		126)
')

define(`DEFINE_MIPS_RELOCATION_TYPES',`
__(`EM_MIPS')
_(R_MIPS_NONE,			0, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_16,			1, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_32,			2, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_REL32,			3, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_26,			4, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_HI16,			5, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_LO16,			6, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_GPREL16,		7, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_LITERAL, 		8, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_GOT16,			9, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_PC16,			10, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_CALL16,		11, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_GPREL32,		12, `GNU binutils, LLVM, MIPS psABI.')
__(`	', `Unused: 13-15.')
_(R_MIPS_SHIFT5,		16, `GNU binutils, LLVM.')
_(R_MIPS_SHIFT6,		17, `GNU binutils, LLVM.')
_(R_MIPS_64,			18, `GNU binutils, LLVM')
_(R_MIPS_GOT_DISP,		19, `GNU binutils, LLVM.')
_(R_MIPS_GOT_PAGE,		20, `GNU binutils, LLVM.')
_(R_MIPS_GOTHI16,		21, `MIPS psABI.')
_(R_MIPS_GOTLO16,		22, `MIPS psABI.')
_(R_MIPS_GOT_LO16,		23, `GNU binutils, LLVM.')
_(R_MIPS_SUB,			24, `GNU binutils, LLVM.')
_(R_MIPS_INSERT_A,		25, `GNU binutils, LLVM.')
_(R_MIPS_INSERT_B,		26, `GNU binutils, LLVM.')
_(R_MIPS_DELETE,		27, `GNU binutils, LLVM.')
_(R_MIPS_HIGHER,		28, `GNU binutils, LLVM.')
_(R_MIPS_HIGHEST,		29, `GNU binutils, LLVM.')
_(R_MIPS_CALLHI16,		30, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_CALLLO16,		31, `GNU binutils, LLVM, MIPS psABI.')
_(R_MIPS_SCN_DISP,		32, `GNU binutils, LLVM.')
_(R_MIPS_REL16,			33, `GNU binutils, LLVM.')
_(R_MIPS_ADD_IMMEDIATE,		34, `GNU binutils, LLVM.')
_(R_MIPS_PJUMP,			35, `GNU binutils, LLVM.')
_(R_MIPS_RELGOT,		36, `GNU binutils, LLVM.')
_(R_MIPS_JALR,			37, `GNU binutils, LLVM.')
_(R_MIPS_TLS_DTPMOD32,		38, `GNU binutils, LLVM.')
_(R_MIPS_TLS_DTPREL32,		39, `GNU binutils, LLVM.')
_(R_MIPS_TLS_DTPMOD64,		40, `GNU binutils, LLVM.')
_(R_MIPS_TLS_DTPREL64,		41, `GNU binutils, LLVM.')
_(R_MIPS_TLS_GD,		42, `GNU binutils, LLVM.')
_(R_MIPS_TLS_LDM,		43, `GNU binutils, LLVM.')
_(R_MIPS_TLS_DTPREL_HI16,	44, `GNU binutils, LLVM.')
_(R_MIPS_TLS_DTPREL_LO16,	45, `GNU binutils, LLVM.')
_(R_MIPS_TLS_GOTTPREL,		46, `GNU binutils, LLVM.')
_(R_MIPS_TLS_TPREL32,		47, `GNU binutils, LLVM.')
_(R_MIPS_TLS_TPREL64,		48, `GNU binutils, LLVM.')
_(R_MIPS_TLS_TPREL_HI16,	49, `GNU binutils, LLVM.')
_(R_MIPS_TLS_TPREL_LO16,	50, `GNU binutils, LLVM.')
_(R_MIPS_GLOB_DAT,		51, `GNU binutils, LLVM.')
__(`	', `Unused: 52-59.')
_(R_MIPS_PC21_S2,		60, `GNU binutils, LLVM.')
_(R_MIPS_PC26_S2,		61, `GNU binutils, LLVM.')
_(R_MIPS_PC18_S3,		62, `GNU binutils, LLVM.')
_(R_MIPS_PC19_S2,		63, `GNU binutils, LLVM.')
_(R_MIPS_PCHI16,		64, `GNU binutils, LLVM.')
_(R_MIPS_PCLO16,		65, `GNU binutils, LLVM.')
__(`	', `Unused: 66-99.')
_(R_MIPS16_26,			100, `GNU binutils, LLVM.')
_(R_MIPS16_GPREL,		101, `GNU binutils, LLVM.')
_(R_MIPS16_GOT16,		102, `GNU binutils, LLVM.')
_(R_MIPS16_CALL16,		103, `GNU binutils, LLVM.')
_(R_MIPS16_HI16,		104, `GNU binutils, LLVM.')
_(R_MIPS16_LO16,		105, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_GD,		106, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_LDM,		107, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_DTPREL_HI16,	108, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_DTPREL_LO16,	109, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_GOTTPREL,	110, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_TPREL_HI16,	111, `GNU binutils, LLVM.')
_(R_MIPS16_TLS_TPREL_LO16,	112, `GNU binutils, LLVM.')
__(`	', `Unused: 113-125.')
_(R_MIPS_COPY,			126, `GNU binutils, LLVM.')
_(R_MIPS_JUMP_SLOT,		127, `GNU binutils, LLVM.')
__(`	', `Unused: 128-132.')
_(R_MICROMIPS_26_S1,		133, `GNU binutils, LLVM.')
_(R_MICROMIPS_HI16,		134, `GNU binutils, LLVM.')
_(R_MICROMIPS_LO16,		135, `GNU binutils, LLVM.')
_(R_MICROMIPS_GPREL16,		136, `GNU binutils, LLVM.')
_(R_MICROMIPS_LITERAL,		137, `GNU binutils, LLVM.')
_(R_MICROMIPS_GOT16,		138, `GNU binutils, LLVM.')
_(R_MICROMIPS_PC7_S1,		139, `GNU binutils, LLVM.')
_(R_MICROMIPS_PC10_S1,		140, `GNU binutils, LLVM.')
_(R_MICROMIPS_PC16_S1,		141, `GNU binutils, LLVM.')
_(R_MICROMIPS_CALL16,		142, `GNU binutils, LLVM.')
__(`	', `Unused: 143-144.')
_(R_MICROMIPS_GOT_DISP,		145, `GNU binutils, LLVM.')
_(R_MICROMIPS_GOT_PAGE,		146, `GNU binutils, LLVM.')
_(R_MICROMIPS_GOT_OFST,		147, `GNU binutils, LLVM.')
_(R_MICROMIPS_GOT_HI16,		148, `GNU binutils, LLVM.')
_(R_MICROMIPS_GOT_LO16,		149, `GNU binutils, LLVM.')
_(R_MICROMIPS_SUB,		150, `GNU binutils, LLVM.')
_(R_MICROMIPS_HIGHER,		151, `GNU binutils, LLVM.')
_(R_MICROMIPS_HIGHEST,		152, `GNU binutils, LLVM.')
_(R_MICROMIPS_CALL_HI16,	153, `GNU binutils, LLVM.')
_(R_MICROMIPS_CALL_LO16,	154, `GNU binutils, LLVM.')
_(R_MICROMIPS_SCN_DISP,		155, `GNU binutils, LLVM.')
_(R_MICROMIPS_JALR,		156, `GNU binutils, LLVM.')
_(R_MICROMIPS_HI0_LO16,		157, `GNU binutils, LLVM.')
__(`	', `Unused: 158-161.')
_(R_MICROMIPS_TLS_GD,		162, `GNU binutils, LLVM.')
_(R_MICROMIPS_TLS_LDM,		163, `GNU binutils, LLVM.')
_(R_MICROMIPS_TLS_DTPREL_HI16,	164, `GNU binutils, LLVM.')
_(R_MICROMIPS_TLS_DTPREL_LO16,	165, `GNU binutils, LLVM.')
_(R_MICROMIPS_TLS_GOTTPREL,	166, `GNU binutils, LLVM.')
__(`	', `Unused: 167-168.')
_(R_MICROMIPS_TLS_TPREL_HI16,	169, `GNU binutils, LLVM.')
_(R_MICROMIPS_TLS_TPREL_LO16,	170, `GNU binutils, LLVM.')
__(`	', `Unused: 171.')
_(R_MICROMIPS_GPREL7_S2,	172, `GNU binutils, LLVM.')
_(R_MICROMIPS_PC23_S2,		173, `GNU binutils, LLVM.')
_(R_MICROMIPS_PC21_S1,		174, `LLVM.')
_(R_MICROMIPS_PC26_S1,		175, `LLVM.')
_(R_MICROMIPS_PC18_S3,		176, `LLVM.')
_(R_MICROMIPS_PC19_S2,		177, `LLVM.')
__(`	', `Unused: 178-247.')
_(R_MIPS_PC32,			248, `GNU binutils, LLVM.')
_(R_MIPS_EH,			249, `GNU binutils, LLVM.')
__(`	', `GNU extensions.')
_(R_MIPS_GNU_REL16_S2,		250, `GNU binutils.')
_(R_MIPS_GNU_VTINHERIT,		251, `GNU binutils.')
_(R_MIPS_GNU_VTENTRY,		252, `GNU binutils.')
')

define(`DEFINE_MIPS_RELOCATION_TYPE_SYNONYMS',`
_(R_MIPS_GOT_OFST,		21, `GNU binutils, LLVM.')
_(R_MIPS_GOT_HI16,		22, `GNU binutils, LLVM.')
')

define(`DEFINE_PPC_RELOCATION_TYPES',`
__(EM_PPC)
_(R_PPC_NONE,		0)
_(R_PPC_ADDR32,		1)
_(R_PPC_ADDR24,		2)
_(R_PPC_ADDR16,		3)
_(R_PPC_ADDR16_LO,	4)
_(R_PPC_ADDR16_HI,	5)
_(R_PPC_ADDR16_HA,	6)
_(R_PPC_ADDR14,		7)
_(R_PPC_ADDR14_BRTAKEN,	8)
_(R_PPC_ADDR14_BRNTAKEN, 9)
_(R_PPC_REL24,		10)
_(R_PPC_REL14,		11)
_(R_PPC_REL14_BRTAKEN,	12)
_(R_PPC_REL14_BRNTAKEN,	13)
_(R_PPC_GOT16,		14)
_(R_PPC_GOT16_LO,	15)
_(R_PPC_GOT16_HI,	16)
_(R_PPC_GOT16_HA,	17)
_(R_PPC_PLTREL24,	18)
_(R_PPC_COPY,		19)
_(R_PPC_GLOB_DAT,	20)
_(R_PPC_JMP_SLOT,	21)
_(R_PPC_RELATIVE,	22)
_(R_PPC_LOCAL24PC,	23)
_(R_PPC_UADDR32,	24)
_(R_PPC_UADDR16,	25)
_(R_PPC_REL32,		26)
_(R_PPC_PLT32,		27)
_(R_PPC_PLTREL32,	28)
_(R_PPC_PLT16_LO,	29)
_(R_PPC_PLT16_HI,	30)
_(R_PPC_PLT16_HA,	31)
__(`	', `Not in the psABI: 32')
_(R_PPC_SDAREL16,	32)
_(R_PPC_SECTOFF,	33)
_(R_PPC_SECTOFF_LO,	34)
_(R_PPC_SECTOFF_HI,	35)
_(R_PPC_SECTOFF_HA,	36)
_(R_PPC_ADDR30,		37)
__(`	', `Used by the PPC64 ABI: 38-66.')
_(R_PPC_TLS,		67)
_(R_PPC_DTPMOD32,	68)
_(R_PPC_TPREL16,	69)
_(R_PPC_TPREL16_LO,	70)
_(R_PPC_TPREL16_HI,	71)
_(R_PPC_TPREL16_HA,	72)
_(R_PPC_TPREL32,	73)
_(R_PPC_DTPREL16,	74)
_(R_PPC_DTPREL16_LO,	75)
_(R_PPC_DTPREL16_HI,	76)
_(R_PPC_DTPREL16_HA,	77)
_(R_PPC_DTPREL32,	78)
_(R_PPC_GOT_TLSGD16,	79)
_(R_PPC_GOT_TLSGD16_LO,	80)
_(R_PPC_GOT_TLSGD16_HI,	81)
_(R_PPC_GOT_TLSGD16_HA,	82)
_(R_PPC_GOT_TLSLD16,	83)
_(R_PPC_GOT_TLSLD16_LO,	84)
_(R_PPC_GOT_TLSLD16_HI,	85)
_(R_PPC_GOT_TLSLD16_HA,	86)
_(R_PPC_GOT_TPREL16,	87)
_(R_PPC_GOT_TPREL16_LO,	88)
_(R_PPC_GOT_TPREL16_HI,	89)
_(R_PPC_GOT_TPREL16_HA,	90)
__(`	', `Not in the psABI: 91-94.')
_(R_PPC_GOT_DTPREL16,	91)
_(R_PPC_GOT_DTPREL16_LO, 92)
_(R_PPC_GOT_DTPREL16_HI, 93)
_(R_PPC_GOT_DTPREL16_HA, 94)
_(R_PPC_TLSGD,		95)
_(R_PPC_TLSLD,		96)
__(`	', `Reserved: 97-100.')
_(R_PPC_EMB_NADDR32,	101)
_(R_PPC_EMB_NADDR16,	102)
_(R_PPC_EMB_NADDR16_LO,	103)
_(R_PPC_EMB_NADDR16_HI,	104)
_(R_PPC_EMB_NADDR16_HA,	105)
_(R_PPC_EMB_SDAI16,	106)
_(R_PPC_EMB_SDA2I16,	107)
_(R_PPC_EMB_SDA2REL,	108)
_(R_PPC_EMB_SDA21,	109)
_(R_PPC_EMB_MRKREF,	110)
_(R_PPC_EMB_RELSEC16,	111)
_(R_PPC_EMB_RELST_LO,	112)
_(R_PPC_EMB_RELST_HI,	113)
_(R_PPC_EMB_RELST_HA,	114)
_(R_PPC_EMB_BIT_FLD,	115)
_(R_PPC_EMB_RELSDA,	116)
__(`	', `Reserved: 117-179.')
_(R_PPC_DIAB_SDA21_LO,	180)
_(R_PPC_DIAB_SDA21_HI,	181)
_(R_PPC_DIAB_SDA21_HA,	182)
_(R_PPC_DIAB_RELSDA_LO,	183)
_(R_PPC_DIAB_RELSDA_HI,	184)
_(R_PPC_DIAB_RELSDA_HA,	185)
__(`	', `Reserved: 201-200.')
_(R_PPC_EMB_SPE_DOUBLE,	201)
_(R_PPC_EMB_SPE_WORD,	202)
_(R_PPC_EMB_SPE_HALF,	203)
_(R_PPC_EMB_SPE_DOUBLE_SDAREL,	204)
_(R_PPC_EMB_SPE_WORD_SDAREL,	205)
_(R_PPC_EMB_SPE_HALF_SDAREL,	206)
_(R_PPC_EMB_SPE_DOUBLE_SDA2REL,	207)
_(R_PPC_EMB_SPE_WORD_SDA2REL,	208)
_(R_PPC_EMB_SPE_HALF_SDA2REL,	209)
_(R_PPC_EMB_SPE_DOUBLE_SDA0REL,	210)
_(R_PPC_EMB_SPE_WORD_SDA0REL,	211)
_(R_PPC_EMB_SPE_HALF_SDA0REL,	212)
_(R_PPC_EMB_SPE_DOUBLE_SDA,	213)
_(R_PPC_EMB_SPE_WORD_SDA,	214)
_(R_PPC_EMB_SPE_HALF_SDA,	215)
_(R_PPC_VLE_REL8,	216)
_(R_PPC_VLE_REL15,	217)
_(R_PPC_VLE_REL24,	218)
_(R_PPC_VLE_LO16A,	219)
_(R_PPC_VLE_LO16D,	220)
_(R_PPC_VLE_HI16A,	221)
_(R_PPC_VLE_HI16D,	222)
_(R_PPC_VLE_HA16A,	223)
_(R_PPC_VLE_HA16D,	224)
_(R_PPC_VLE_SDA21,	225)
_(R_PPC_VLE_SDA21_LO,	226)
_(R_PPC_VLE_SDAREL_LO16A,	227)
_(R_PPC_VLE_SDAREL_LO16D,	228)
_(R_PPC_VLE_SDAREL_HI16A,	229)
_(R_PPC_VLE_SDAREL_HI16D,	230)
_(R_PPC_VLE_SDAREL_HA16A,	231)
_(R_PPC_VLE_SDAREL_HA16D,	232)
_(R_PPC_VLE_ADDR20,	233)
__(`	', `Reserved: 234-248.')
_(R_PPC_REL16,		249)
_(R_PPC_REL16_LO,	250)
_(R_PPC_REL16_HI,	251)
_(R_PPC_REL16_HA,	252)
__(`	', `Reserved: 253-255.')
')

define(`DEFINE_PPC64_RELOCATION_TYPES',`
__(EM_PPC64)
_(R_PPC64_NONE,			0)
_(R_PPC64_ADDR32,		1)
_(R_PPC64_ADDR24,		2)
_(R_PPC64_ADDR16,		3)
_(R_PPC64_ADDR16_LO,		4)
_(R_PPC64_ADDR16_HI,		5)
_(R_PPC64_ADDR16_HA,		6)
_(R_PPC64_ADDR14,		7)
__(`	', `unused: 8-9.')
_(R_PPC64_REL24,		10)
_(R_PPC64_REL14,		11)
__(`	', `unused: 12-13.')
_(R_PPC64_GOT16,		14)
_(R_PPC64_GOT16_LO,		15)
_(R_PPC64_GOT16_HI,		16)
_(R_PPC64_GOT16_HA,		17)
__(`	', `unused: 18.')
_(R_PPC64_COPY,			19)
_(R_PPC64_GLOB_DAT,		20)
_(R_PPC64_JMP_SLOT,		21)
_(R_PPC64_RELATIVE,		22)
__(`	', `unused: 23.')
_(R_PPC64_UADDR32,		24)
_(R_PPC64_UADDR16,		25)
_(R_PPC64_REL32,		26)
_(R_PPC64_PLT32,		27)
_(R_PPC64_PLTREL32,		28)
_(R_PPC64_PLT16_LO,		29)
_(R_PPC64_PLT16_HI,		30)
_(R_PPC64_PLT16_HA,		31)
__(`	', `unused: 32.')
_(R_PPC64_SECTOFF,		33)
_(R_PPC64_SECTOFF_LO,		34)
_(R_PPC64_SECTOFF_HI,		35)
_(R_PPC64_SECTOFF_HA,		36)
_(R_PPC64_REL30,		37)
_(R_PPC64_ADDR64,		38)
_(R_PPC64_ADDR16_HIGHER,	39)
_(R_PPC64_ADDR16_HIGHERA,	40)
_(R_PPC64_ADDR16_HIGHEST,	41)
_(R_PPC64_ADDR16_HIGHESTA,	42)
_(R_PPC64_UADDR64,		43)
_(R_PPC64_REL64,		44)
_(R_PPC64_PLT64,		45)
_(R_PPC64_PLTREL64,		46)
_(R_PPC64_TOC16,		47)
_(R_PPC64_TOC16_LO,		48)
_(R_PPC64_TOC16_HI,		49)
_(R_PPC64_TOC16_HA,		50)
_(R_PPC64_TOC,			51)
_(R_PPC64_PLTGOT16,		52)
_(R_PPC64_PLTGOT16_LO,		53)
_(R_PPC64_PLTGOT16_HI,		54)
_(R_PPC64_PLTGOT16_HA,		55)
_(R_PPC64_ADDR16_DS,		56)
_(R_PPC64_ADDR16_LO_DS,		57)
_(R_PPC64_GOT16_DS,		58)
_(R_PPC64_GOT16_LO_DS,		59)
_(R_PPC64_PLT16_LO_DS,		60)
_(R_PPC64_SECTOFF_DS,		61)
_(R_PPC64_SECTOFF_LO_DS,	62)
_(R_PPC64_TOC16_DS,		63)
_(R_PPC64_TOC16_LO_DS,		64)
_(R_PPC64_PLTGOT16_DS,		65)
_(R_PPC64_PLTGOT16_LO_DS,	66)
_(R_PPC64_TLS,			67)
_(R_PPC64_DTPMOD64,		68)
_(R_PPC64_TPREL16,		69)
_(R_PPC64_TPREL16_LO,		70)
_(R_PPC64_TPREL16_HI,		71)
_(R_PPC64_TPREL16_HA,		72)
_(R_PPC64_TPREL64,		73)
_(R_PPC64_DTPREL16,		74)
_(R_PPC64_DTPREL16_LO,		75)
_(R_PPC64_DTPREL16_HI,		76)
_(R_PPC64_DTPREL16_HA,		77)
_(R_PPC64_DTPREL64,		78)
_(R_PPC64_GOT_TLSGD16,		79)
_(R_PPC64_GOT_TLSGD16_LO,	80)
_(R_PPC64_GOT_TLSGD16_HI,	81)
_(R_PPC64_GOT_TLSGD16_HA,	82)
_(R_PPC64_GOT_TLSLD16,		83)
_(R_PPC64_GOT_TLSLD16_LO,	84)
_(R_PPC64_GOT_TLSLD16_HI,	85)
_(R_PPC64_GOT_TLSLD16_HA,	86)
_(R_PPC64_GOT_TPREL16_DS,	87)
_(R_PPC64_GOT_TPREL16_LO_DS,	88)
_(R_PPC64_GOT_TPREL16_HI,	89)
_(R_PPC64_GOT_TPREL16_HA,	90)
_(R_PPC64_GOT_DTPREL16_DS,	91)
_(R_PPC64_GOT_DTPREL16_LO_DS,	92)
_(R_PPC64_GOT_DTPREL16_HI,	93)
_(R_PPC64_GOT_DTPREL16_HA,	94)
_(R_PPC64_TPREL16_DS,		95)
_(R_PPC64_TPREL16_LO_DS,	96)
_(R_PPC64_TPREL16_HIGHER,	97)
_(R_PPC64_TPREL16_HIGHERA,	98)
_(R_PPC64_TPREL16_HIGHEST,	99)
_(R_PPC64_TPREL16_HIGHESTA,	100)
_(R_PPC64_DTPREL16_DS,		101)
_(R_PPC64_DTPREL16_LO_DS,	102)
_(R_PPC64_DTPREL16_HIGHER,	103)
_(R_PPC64_DTPREL16_HIGHERA,	104)
_(R_PPC64_DTPREL16_HIGHEST,	105)
_(R_PPC64_DTPREL16_HIGHESTA,	106)
_(R_PPC64_TLSGD,		107)
_(R_PPC64_TLSLD,		108)
_(R_PPC64_TOCSAVE,		109)
_(R_PPC64_ADDR16_HIGH,		110)
_(R_PPC64_ADDR16_HIGHA,		111)
_(R_PPC64_TPREL16_HIGH,		112)
_(R_PPC64_TPREL16_HIGHA,	113)
_(R_PPC64_DTPREL16_HIGH,	114)
_(R_PPC64_DTPREL16_HIGHA,	115)
_(R_PPC64_REL24_NOTOC,		116)
_(R_PPC64_ADDR64_LOCAL,		117)
_(R_PPC64_ENTRY,		118)
_(R_PPC64_PLTSEQ,		119)
_(R_PPC64_PLTCALL,		120)
_(R_PPC64_PLTSEQ_NOTOC,		121)
_(R_PPC64_PLTCALL_NOTOC,	122)
_(R_PPC64_PCREL_OPT,		123)
__(`	', `unused: 124-127.')
_(R_PPC64_D34,			128)
_(R_PPC64_D34_LO,		129)
_(R_PPC64_D34_HI30,		130)
_(R_PPC64_D34_HA30,		131)
_(R_PPC64_PCREL34,		132)
_(R_PPC64_GOT_PCREL34,		133)
_(R_PPC64_PLT_PCREL34,		134)
_(R_PPC64_PLT_PCREL34_NOTOC,	135)
_(R_PPC64_ADDR16_HIGHER34,	136)
_(R_PPC64_ADDR16_HIGHERA34,	137)
_(R_PPC64_ADDR16_HIGHEST34,	138)
_(R_PPC64_ADDR16_HIGHESTA34,	139)
_(R_PPC64_REL16_HIGHER34,	140)
_(R_PPC64_REL16_HIGHERA34,	141)
_(R_PPC64_REL16_HIGHEST34,	142)
_(R_PPC64_REL16_HIGHESTA34,	143)
_(R_PPC64_D28,			144)
_(R_PPC64_PCREL28,		145)
_(R_PPC64_TPREL34,		146)
_(R_PPC64_DTPREL34,		147)
_(R_PPC64_GOT_TLSGD_PCREL34,	148)
_(R_PPC64_GOT_TLSLD_PCREL34,	149)
_(R_PPC64_GOT_TPREL_PCREL34,	150)
_(R_PPC64_GOT_DTPREL_PCREL34,	151)
__(`	', `unused: 152-239.')
_(R_PPC64_REL16_HIGH,		240)
_(R_PPC64_REL16_HIGHA,		241)
_(R_PPC64_REL16_HIGHER,		242)
_(R_PPC64_REL16_HIGHERA,	243)
_(R_PPC64_REL16_HIGHEST,	244)
_(R_PPC64_REL16_HIGHESTA,	245)
_(R_PPC64_REL16DX_HA,		246)
__(`	', `unused: 247.')
_(R_PPC64_IRELATIVE,		248)
_(R_PPC64_REL16,		249)
_(R_PPC64_REL16_LO,		250)
_(R_PPC64_REL16_HI,		251)
_(R_PPC64_REL16_HA,		252)
_(R_PPC64_GNU_VTINHERIT,	253)
_(R_PPC64_GNU_VTENTRY,		254)
')

define(`DEFINE_PPC64_OBSOLETE_RELOCATION_TYPES',`
_(R_PPC64_ADDR14_BRTAKEN,	8)
_(R_PPC64_ADDR14_BRNTAKEN,	9)
_(R_PPC64_REL14_BRTAKEN,	12)
_(R_PPC64_REL14_BRNTAKEN,	13)
_(R_PPC64_ADDR30,		37)
')

define(`DEFINE_RISCV_RELOCATION_TYPES',`
__(`EM_RISCV')
_(R_RISCV_NONE,			0)
_(R_RISCV_32,			1)
_(R_RISCV_64,			2)
_(R_RISCV_RELATIVE,		3)
_(R_RISCV_COPY,			4)
_(R_RISCV_JUMP_SLOT,		5)
_(R_RISCV_TLS_DTPMOD32,		6)
_(R_RISCV_TLS_DTPMOD64,		7)
_(R_RISCV_TLS_DTPREL32,		8)
_(R_RISCV_TLS_DTPREL64,		9)
_(R_RISCV_TLS_TPREL32,		10)
_(R_RISCV_TLS_TPREL64,		11)
_(R_RISCV_TLSDESC,		12)
__(`	', `unused: 13-15')
_(R_RISCV_BRANCH,		16)
_(R_RISCV_JAL,			17)
_(R_RISCV_CALL,			18)
_(R_RISCV_CALL_PLT,		19)
_(R_RISCV_GOT_HI20,		20)
_(R_RISCV_TLS_GOT_HI20,		21)
_(R_RISCV_TLS_GD_HI20,		22)
_(R_RISCV_PCREL_HI20,		23)
_(R_RISCV_PCREL_LO12_I,		24)
_(R_RISCV_PCREL_LO12_S,		25)
_(R_RISCV_HI20,			26)
_(R_RISCV_LO12_I,		27)
_(R_RISCV_LO12_S,		28)
_(R_RISCV_TPREL_HI20,		29)
_(R_RISCV_TPREL_LO12_I,		30)
_(R_RISCV_TPREL_LO12_S,		31)
_(R_RISCV_TPREL_ADD,		32)
_(R_RISCV_ADD8,			33)
_(R_RISCV_ADD16,		34)
_(R_RISCV_ADD32,		35)
_(R_RISCV_ADD64,		36)
_(R_RISCV_SUB8,			37)
_(R_RISCV_SUB16,		38)
_(R_RISCV_SUB32,		39)
_(R_RISCV_SUB64,		40)
_(R_RISCV_GOT32_PCREL,		41)
__(`	', `reserved: 42')
_(R_RISCV_ALIGN,		43)
_(R_RISCV_RVC_BRANCH,		44)
_(R_RISCV_RVC_JUMP,		45)
__(`	', `reserved: 46-50')
_(R_RISCV_RELAX,		51)
_(R_RISCV_SUB6,			52)
_(R_RISCV_SET6,			53)
_(R_RISCV_SET8,			54)
_(R_RISCV_SET16,		55)
_(R_RISCV_SET32,		56)
_(R_RISCV_32_PCREL,		57)
_(R_RISCV_IRELATIVE,		58)
_(R_RISCV_PLT32,		59)
_(R_RISCV_SET_ULEB128,		60)
_(R_RISCV_SUB_ULEB128,		61)
_(R_RISCV_TLSDESC_HI20,		62)
_(R_RISCV_TLSDESC_LOAD_LO12,	63)
_(R_RISCV_TLSDESC_ADD_LO12,	64)
_(R_RISCV_TLSDESC_CALL,		65)
__(`	', `reserved: 66-190')
_(R_RISCV_VENDOR,		191)
__(`	', `reserved: 192-255')
')

define(`DEFINE_RISCV_OBSOLETE_RELOCATION_TYPES',`
_(R_RISCV_GNU_VTINHERIT,	41)
_(R_RISCV_GNU_VTENTRY,		42)
_(R_RISCV_RVC_LUI,		46)
_(R_RISCV_GPREL_I,		47)
_(R_RISCV_GPREL_S,		48)
_(R_RISCV_TPREL_I,		49)
_(R_RISCV_TPREL_S,		50)
')

define(`DEFINE_S390_RELOCATION_TYPES',`
__(`EM_S390')
_(R_390_NONE,		0)
_(R_390_8,		1)
_(R_390_12,		2)
_(R_390_16,		3)
_(R_390_32,		4)
_(R_390_PC32,		5)
_(R_390_GOT12,		6)
_(R_390_GOT32,		7)
_(R_390_PLT32,		8)
_(R_390_COPY,		9)
_(R_390_GLOB_DAT,	10)
_(R_390_JMP_SLOT,	11)
_(R_390_RELATIVE,	12)
_(R_390_GOTOFF,		13)
_(R_390_GOTPC,		14)
_(R_390_GOT16,		15)
_(R_390_PC16,		16)
_(R_390_PC16DBL,	17)
_(R_390_PLT16DBL,	18)
_(R_390_PC32DBL,	19)
_(R_390_PLT32DBL,	20)
_(R_390_GOTPCDBL,	21)
_(R_390_64,		22)
_(R_390_PC64,		23)
_(R_390_GOT64,		24)
_(R_390_PLT64,		25)
_(R_390_GOTENT,		26)
')

define(`DEFINE_SPARC_RELOCATION_TYPES',`
__(`EM_SPARC')
_(R_SPARC_NONE,		0)
_(R_SPARC_8,		1)
_(R_SPARC_16,		2)
_(R_SPARC_32, 		3)
_(R_SPARC_DISP8,	4)
_(R_SPARC_DISP16,	5)
_(R_SPARC_DISP32,	6)
_(R_SPARC_WDISP30,	7)
_(R_SPARC_WDISP22,	8)
_(R_SPARC_HI22,		9)
_(R_SPARC_22,		10)
_(R_SPARC_13,		11)
_(R_SPARC_LO10,		12)
_(R_SPARC_GOT10,	13)
_(R_SPARC_GOT13,	14)
_(R_SPARC_GOT22,	15)
_(R_SPARC_PC10,		16)
_(R_SPARC_PC22,		17)
_(R_SPARC_WPLT30,	18)
_(R_SPARC_COPY,		19)
_(R_SPARC_GLOB_DAT,	20)
_(R_SPARC_JMP_SLOT,	21)
_(R_SPARC_RELATIVE,	22)
_(R_SPARC_UA32,		23)
_(R_SPARC_PLT32,	24)
_(R_SPARC_HIPLT22,	25)
_(R_SPARC_LOPLT10,	26)
_(R_SPARC_PCPLT32,	27)
_(R_SPARC_PCPLT22,	28)
_(R_SPARC_PCPLT10,	29)
_(R_SPARC_10,		30)
_(R_SPARC_11,		31)
_(R_SPARC_64,		32)
_(R_SPARC_OLO10,	33)
_(R_SPARC_HH22,		34)
_(R_SPARC_HM10,		35)
_(R_SPARC_LM22,		36)
_(R_SPARC_PC_HH22,	37)
_(R_SPARC_PC_HM10,	38)
_(R_SPARC_PC_LM22,	39)
_(R_SPARC_WDISP16,	40)
_(R_SPARC_WDISP19,	41)
__(`	', `unused: 42')
_(R_SPARC_7,		43)
_(R_SPARC_5,		44)
_(R_SPARC_6,		45)
_(R_SPARC_DISP64,	46)
_(R_SPARC_PLT64,	47)
_(R_SPARC_HIX22,	48)
_(R_SPARC_LOX10,	49)
_(R_SPARC_H44,		50)
_(R_SPARC_M44,		51)
_(R_SPARC_L44,		52)
_(R_SPARC_REGISTER,	53)
_(R_SPARC_UA64,		54)
_(R_SPARC_UA16,		55)
_(R_SPARC_TLS_GD_HI22,	56)
_(R_SPARC_TLS_GD_LO10,	57)
_(R_SPARC_TLS_GD_ADD,	58)
_(R_SPARC_TLS_GD_CALL,	59)
_(R_SPARC_TLS_LDM_HI22,	60)
_(R_SPARC_TLS_LDM_LO10,	61)
_(R_SPARC_TLS_LDM_ADD,	62)
_(R_SPARC_TLS_LDM_CALL,	63)
_(R_SPARC_TLS_LDO_HIX22, 64)
_(R_SPARC_TLS_LDO_LOX10, 65)
_(R_SPARC_TLS_LDO_ADD,	66)
_(R_SPARC_TLS_IE_HI22,	67)
_(R_SPARC_TLS_IE_LO10,	68)
_(R_SPARC_TLS_IE_LD,	69)
_(R_SPARC_TLS_IE_LDX,	70)
_(R_SPARC_TLS_IE_ADD,	71)
_(R_SPARC_TLS_LE_HIX22,	72)
_(R_SPARC_TLS_LE_LOX10,	73)
_(R_SPARC_TLS_DTPMOD32,	74)
_(R_SPARC_TLS_DTPMOD64,	75)
_(R_SPARC_TLS_DTPOFF32,	76)
_(R_SPARC_TLS_DTPOFF64,	77)
_(R_SPARC_TLS_TPOFF32,	78)
_(R_SPARC_TLS_TPOFF64,	79)
_(R_SPARC_GOTDATA_HIX22, 80)
_(R_SPARC_GOTDATA_LOX10, 81)
_(R_SPARC_GOTDATA_OP_HIX22, 82)
_(R_SPARC_GOTDATA_OP_LOX10, 83)
_(R_SPARC_GOTDATA_OP,	84)
_(R_SPARC_H34,		85)
_(R_SPARC_SIZE32,	86)
_(R_SPARC_SIZE64,	87)
_(R_SPARC_WDISP10,	88)
')

define(`DEFINE_SPARC_OBSOLETE_RELOCATION_TYPES',`
_(R_SPARC_GLOB_JMP,	42)
')

define(`DEFINE_VAX_RELOCATION_TYPES',`
__(`EM_VAX')
_(R_VAX_NONE,           0)
_(R_VAX_32,             1)
_(R_VAX_16,             2)
_(R_VAX_8,              3)
_(R_VAX_PC32,           4)
_(R_VAX_PC16,           5)
_(R_VAX_PC8,            6)
_(R_VAX_GOT32,          7)
_(R_VAX_PLT32,         13)
_(R_VAX_COPY,          19)
_(R_VAX_GLOB_DAT,      20)
_(R_VAX_JMP_SLOT,      21)
_(R_VAX_RELATIVE,      22)
')

define(`DEFINE_X86_64_RELOCATION_TYPES',`
__(`EM_X86_64')
_(R_X86_64_NONE,	0)
_(R_X86_64_64,		1)
_(R_X86_64_PC32,	2)
_(R_X86_64_GOT32,	3)
_(R_X86_64_PLT32,	4)
_(R_X86_64_COPY,	5)
_(R_X86_64_GLOB_DAT,	6)
_(R_X86_64_JUMP_SLOT,	7)
_(R_X86_64_RELATIVE,	8)
_(R_X86_64_GOTPCREL,	9)
_(R_X86_64_32,		10)
_(R_X86_64_32S,		11)
_(R_X86_64_16,		12)
_(R_X86_64_PC16,	13)
_(R_X86_64_8,		14)
_(R_X86_64_PC8,		15)
_(R_X86_64_DTPMOD64,	16)
_(R_X86_64_DTPOFF64,	17)
_(R_X86_64_TPOFF64,	18)
_(R_X86_64_TLSGD,	19)
_(R_X86_64_TLSLD,	20)
_(R_X86_64_DTPOFF32,	21)
_(R_X86_64_GOTTPOFF,	22)
_(R_X86_64_TPOFF32,	23)
_(R_X86_64_PC64,	24)
_(R_X86_64_GOTOFF64,	25)
_(R_X86_64_GOTPC32,	26)
_(R_X86_64_GOT64,	27)
_(R_X86_64_GOTPCREL64,	28)
_(R_X86_64_GOTPC64,	29)
__(`	', `deprecated: 30')
_(R_X86_64_PLTOFF64,	31)
_(R_X86_64_SIZE32,	32)
_(R_X86_64_SIZE64,	33)
_(R_X86_64_GOTPC32_TLSDESC,	34)
_(R_X86_64_TLSDESC_CALL,	35)
_(R_X86_64_TLSDESC,	36)
_(R_X86_64_IRELATIVE,	37)
_(R_X86_64_RELATIVE64,	38)
__(`	', `deprecated: 39-40')
_(R_X86_64_GOTPCRELX,	41)
_(R_X86_64_REX_GOTPCRELX,	42)
_(R_X86_64_CODE_4_GOTPCRELX,	43)
_(R_X86_64_CODE_4_GOTTPOFF,	44)
_(R_X86_64_CODE_4_GOTPC32_TLSDESC,	45)
_(R_X86_64_CODE_5_GOTPCRELX,	46)
_(R_X86_64_CODE_5_GOTTPOFF,	47)
_(R_X86_64_CODE_5_GOTPC32_TLSDESC,	48)
_(R_X86_64_CODE_6_GOTPCRELX,	49)
_(R_X86_64_CODE_6_GOTTPOFF,	50)
_(R_X86_64_CODE_6_GOTPC32_TLSDESC,	51)
')

define(`DEFINE_X86_64_OBSOLETE_RELOCATION_TYPES', `
_(R_X86_64_GOTPLT64,	30)
_(R_X86_64_PC32_BND,	39)
_(R_X86_64_PLT32_BND,	40)
')

# These are the symbols used in the Sun ``Linkers and Loaders
# Guide'', Document No: 817-1984-17.  See the X86_64 relocations
# list above for the spellings used in the ELF specification.
define(`DEFINE_X86_64_RELOCATION_TYPE_SYNONYMS',`
_(R_AMD64_NONE,		R_X86_64_NONE)
_(R_AMD64_64,		R_X86_64_64)
_(R_AMD64_PC32,		R_X86_64_PC32)
_(R_AMD64_GOT32,	R_X86_64_GOT32)
_(R_AMD64_PLT32,	R_X86_64_PLT32)
_(R_AMD64_COPY,		R_X86_64_COPY)
_(R_AMD64_GLOB_DAT,	R_X86_64_GLOB_DAT)
_(R_AMD64_JUMP_SLOT,	R_X86_64_JUMP_SLOT)
_(R_AMD64_RELATIVE,	R_X86_64_RELATIVE)
_(R_AMD64_GOTPCREL,	R_X86_64_GOTPCREL)
_(R_AMD64_32,		R_X86_64_32)
_(R_AMD64_32S,		R_X86_64_32S)
_(R_AMD64_16,		R_X86_64_16)
_(R_AMD64_PC16,		R_X86_64_PC16)
_(R_AMD64_8,		R_X86_64_8)
_(R_AMD64_PC8,		R_X86_64_PC8)
_(R_AMD64_PC64,		R_X86_64_PC64)
_(R_AMD64_GOTOFF64,	R_X86_64_GOTOFF64)
_(R_AMD64_GOTPC32,	R_X86_64_PC32)
')

define(`DEFINE_RELOCATION_TYPES',`
DEFINE_386_RELOCATION_TYPES()
DEFINE_AARCH64_RELOCATION_TYPES()
DEFINE_ARM_RELOCATION_TYPES()
DEFINE_IA_64_RELOCATION_TYPES()
DEFINE_LOONGARCH_RELOCATION_TYPES()
DEFINE_MIPS_RELOCATION_TYPES()
DEFINE_PPC64_RELOCATION_TYPES()
DEFINE_PPC_RELOCATION_TYPES()
DEFINE_RISCV_RELOCATION_TYPES()
DEFINE_S390_RELOCATION_TYPES()
DEFINE_SPARC_RELOCATION_TYPES()
DEFINE_VAX_RELOCATION_TYPES()
DEFINE_X86_64_RELOCATION_TYPES()
')

# Obsolete relocation types.
define(`DEFINE_OBSOLETE_RELOCATION_TYPES',`dnl
DEFINE_ARM_OBSOLETE_RELOCATION_TYPES()
DEFINE_PPC64_OBSOLETE_RELOCATION_TYPES()
DEFINE_RISCV_OBSOLETE_RELOCATION_TYPES()
DEFINE_SPARC_OBSOLETE_RELOCATION_TYPES()
DEFINE_X86_64_OBSOLETE_RELOCATION_TYPES()
')

# Alternate spellings for relocation types.
define(`DEFINE_RELOCATION_TYPE_SYNONYMS',`
DEFINE_386_RELOCATION_TYPE_SYNONYMS()
DEFINE_AARCH64_RELOCATION_TYPE_SYNONYMS()
DEFINE_IA_64_RELOCATION_TYPE_SYNONYMS()
DEFINE_MIPS_RELOCATION_TYPE_SYNONYMS()
DEFINE_X86_64_RELOCATION_TYPE_SYNONYMS()
')

define(`DEFINE_LL_FLAGS',`
_(LL_NONE,			0,
	`no flags')
_(LL_EXACT_MATCH,		0x1,
	`require an exact match')
_(LL_IGNORE_INT_VER,	0x2,
	`ignore version incompatibilities')
_(LL_REQUIRE_MINOR,	0x4,
	`')
_(LL_EXPORTS,		0x8,
	`')
_(LL_DELAY_LOAD,		0x10,
	`')
_(LL_DELTA,		0x20,
	`')
')

# ELF Note types.
#
# These values are used in the n_type field of the Elf Note header.
define(`DEFINE_COMMON_NOTE_TYPES',`
_(NT_ABI_TAG,			1,
	`Tag indicating the OS ABI')
')

define(`DEFINE_CORE_FILE_NOTE_TYPES',`
__(`Note types used in core files.')
_(NT_PRSTATUS,			1,
	`Process status')
_(NT_FPREGSET,			2,
	`Floating point information')
_(NT_PRPSINFO,			3,
	`Process information')
_(NT_AUXV,			6,
	`Auxiliary vector')
_(NT_PSTATUS,			10,
	`Linux process status')
_(NT_FPREGS,			12,
	`Linux floating point regset')
_(NT_PSINFO,			13,
	`Linux process information')
_(NT_LWPSTATUS,			16,
	`Linux lwpstatus_t type')
_(NT_LWPSINFO,			17,
	`Linux lwpinfo_t type')
_(NT_PRXFPREG,		0x46E62B7FU,
	`Linux user_xfpregs structure')
')

define(`DEFINE_GNU_NOTE_TYPES',`
__(`GNU note types')
_(NT_GNU_ABI_TAG,		1,
	`GNU ABI version')
_(NT_GNU_HWCAP,			2,
	`Hardware capabilities')
_(NT_GNU_BUILD_ID,		3,
	`Build id, set by ld(1)')
_(NT_GNU_GOLD_VERSION,		4,
	`Version number of the GNU gold linker')
')

define(`DEFINE_FREEBSD_NOTE_TYPES',`
__(`FreeBSD note types.')
_(NT_FREEBSD_ABI_TAG,		1,
	`FreeBSD ABI version')
_(NT_FREEBSD_NOINIT_TAG,	2,
	`FreeBSD no .init tag')
_(NT_FREEBSD_ARCH_TAG,		3,
	`FreeBSD arch tag')
_(NT_FREEBSD_FEATURE_CTL,	4,
	`FreeBSD feature control')
')

define(`DEFINE_NOTE_TYPES',`dnl
DEFINE_COMMON_NOTE_TYPES()dnl
DEFINE_GNU_NOTE_TYPES()dnl
DEFINE_FREEBSD_NOTE_TYPES()dnl
DEFINE_CORE_FILE_NOTE_TYPES()dnl
')

# Aliases for the ABI tag.
define(`DEFINE_NOTE_TYPE_ALIASES',`
_(NT_NETBSD_IDENT,	NT_ABI_TAG)
_(NT_OPENBSD_IDENT,	NT_ABI_TAG)
')

#
# Option kinds.
#
define(`DEFINE_OPTION_KINDS',`
_(ODK_NULL,       0,
	`undefined')
_(ODK_REGINFO,    1,
	`register usage info')
_(ODK_EXCEPTIONS, 2,
	`exception processing info')
_(ODK_PAD,        3,
	`section padding')
_(ODK_HWPATCH,    4,
	`hardware patch applied')
_(ODK_FILL,       5,
	`fill value used by linker')
_(ODK_TAGS,       6,
	`reserved space for tools')
_(ODK_HWAND,      7,
	`hardware AND patch applied')
_(ODK_HWOR,       8,
	`hardware OR patch applied')
_(ODK_GP_GROUP,   9,
	`GP group to use for text/data sections')
_(ODK_IDENT,      10,
	`ID information')
_(ODK_PAGESIZE,   11,
	`page size information')
')

#
# ODK_EXCEPTIONS info field masks.
#
define(`DEFINE_OPTION_EXCEPTIONS',`
_(OEX_FPU_MIN,    0x0000001FU,
	`minimum FPU exception which must be enabled')
_(OEX_FPU_MAX,    0x00001F00U,
	`maximum FPU exception which can be enabled')
_(OEX_PAGE0,      0x00010000U,
	`page zero must be mapped')
_(OEX_SMM,        0x00020000U,
	`run in sequential memory mode')
_(OEX_PRECISEFP,  0x00040000U,
	`run in precise FP exception mode')
_(OEX_DISMISS,    0x00080000U,
	`dismiss invalid address traps')
')

#
# ODK_PAD info field masks.
#
define(`DEFINE_OPTION_PADS',`
_(OPAD_PREFIX,   0x0001)
_(OPAD_POSTFIX,  0x0002)
_(OPAD_SYMBOL,   0x0004)
')

#
# ODK_HWPATCH info field masks and ODK_HWAND/ODK_HWOR
# info field and hwp_flags[12] masks.
#
define(`DEFINE_ODK_HWPATCH_MASKS',`
_(OHW_R4KEOP,     0x00000001U,
	`patch for R4000 branch at end-of-page bug')
_(OHW_R8KPFETCH,  0x00000002U,
	`R8000 prefetch bug may occur')
_(OHW_R5KEOP,     0x00000004U,
	`patch for R5000 branch at end-of-page bug')
_(OHW_R5KCVTL,    0x00000008U,
	`R5000 cvt.[ds].l bug: clean == 1')
_(OHW_R10KLDL,    0x00000010U,
	`need patch for R10000 misaligned load')
_(OHWA0_R4KEOP_CHECKED, 0x00000001U,
	`object checked for R4000 end-of-page bug')
_(OHWA0_R4KEOP_CLEAN, 0x00000002U,
	`object verified clean for R4000 end-of-page bug')
_(OHWO0_FIXADE,   0x00000001U,
	`object requires call to fixade')
')

#
# ODK_IDENT/ODK_GP_GROUP info field masks.
#
define(`DEFINE_ODK_GP_MASKS',`
_(OGP_GROUP,      0x0000FFFFU,
	`GP group number')
_(OGP_SELF,       0x00010000U,
	`GP group is self-contained')
')

# MIPS ABI related constants.
define(`DEFINE_MIPS_ABIS',`
_(E_MIPS_ABI_O32,		0x00001000,
	`MIPS 32 bit ABI (UCODE)')
_(E_MIPS_ABI_O64,		0x00002000,
	`UCODE MIPS 64 bit ABI')
_(E_MIPS_ABI_EABI32,	0x00003000,
	`Embedded ABI for 32-bit')
_(E_MIPS_ABI_EABI64,	0x00004000,
	`Embedded ABI for 64-bit')
')
