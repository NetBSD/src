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

# These definitions are based on:
# - The public specification of the ELF format as defined in the
#   October 2009 draft of System V ABI.
#   See: http://www.sco.com/developers/gabi/latest/ch4.intro.html
# - The May 1998 (version 1.5) draft of "The ELF-64 object format".
# - Processor-specific ELF ABI definitions for sparc, i386, amd64, mips,
#   ia64, powerpc, and RISC-V processors.
# - The "Linkers and Libraries Guide", from Sun Microsystems.

define(`VCSID_ELFCONSTANTS_M4',
	`Id: elfconstants.m4 3939 2021-04-04 20:15:13Z jkoshy')

#
# Types of capabilities.
# 
define(`DEFINE_CAPABILITIES',`
DEFINE_CAPABILITY(`CA_SUNW_NULL',	0,	`ignored')
DEFINE_CAPABILITY(`CA_SUNW_HW_1',	1,	`hardware capability')
DEFINE_CAPABILITY(`CA_SUNW_SW_1',	2,	`software capability')')

#
# Flags used with dynamic linking entries.
#
define(`DEFINE_DYN_FLAGS',`
DEFINE_DYN_FLAG(`DF_ORIGIN',           0x1,
	`object being loaded may refer to `$ORIGIN'')
DEFINE_DYN_FLAG(`DF_SYMBOLIC',         0x2,
	`search library for references before executable')
DEFINE_DYN_FLAG(`DF_TEXTREL',          0x4,
	`relocation entries may modify text segment')
DEFINE_DYN_FLAG(`DF_BIND_NOW',         0x8,
	`process relocation entries at load time')
DEFINE_DYN_FLAG(`DF_STATIC_TLS',       0x10,
	`uses static thread-local storage')
DEFINE_DYN_FLAG(`DF_1_BIND_NOW',       0x1,
	`process relocation entries at load time')
DEFINE_DYN_FLAG(`DF_1_GLOBAL',         0x2,
	`unused')
DEFINE_DYN_FLAG(`DF_1_GROUP',          0x4,
	`object is a member of a group')
DEFINE_DYN_FLAG(`DF_1_NODELETE',       0x8,
	`object cannot be deleted from a process')
DEFINE_DYN_FLAG(`DF_1_LOADFLTR',       0x10,
	`immediate load filtees')
DEFINE_DYN_FLAG(`DF_1_INITFIRST',      0x20,
	`initialize object first')
DEFINE_DYN_FLAG(`DF_1_NOOPEN',         0x40,
	`disallow dlopen()')
DEFINE_DYN_FLAG(`DF_1_ORIGIN',         0x80,
	`object being loaded may refer to $ORIGIN')
DEFINE_DYN_FLAG(`DF_1_DIRECT',         0x100,
	`direct bindings enabled')
DEFINE_DYN_FLAG(`DF_1_INTERPOSE',      0x400,
	`object is interposer')
DEFINE_DYN_FLAG(`DF_1_NODEFLIB',       0x800,
	`ignore default library search path')
DEFINE_DYN_FLAG(`DF_1_NODUMP',         0x1000,
	`disallow dldump()')
DEFINE_DYN_FLAG(`DF_1_CONFALT',        0x2000,
	`object is a configuration alternative')
DEFINE_DYN_FLAG(`DF_1_ENDFILTEE',      0x4000,
	`filtee terminates filter search')
DEFINE_DYN_FLAG(`DF_1_DISPRELDNE',     0x8000,
	`displacement relocation done')
DEFINE_DYN_FLAG(`DF_1_DISPRELPND',     0x10000,
	`displacement relocation pending')')

#
# Dynamic linking entry types.
#
define(`DEFINE_DYN_TYPES',`
DEFINE_DYN_TYPE(`DT_NULL',             0,
	`end of array')
DEFINE_DYN_TYPE(`DT_NEEDED',           1,
	`names a needed library')
DEFINE_DYN_TYPE(`DT_PLTRELSZ',         2,
	`size in bytes of associated relocation entries')
DEFINE_DYN_TYPE(`DT_PLTGOT',           3,
	`address associated with the procedure linkage table')
DEFINE_DYN_TYPE(`DT_HASH',             4,
	`address of the symbol hash table')
DEFINE_DYN_TYPE(`DT_STRTAB',           5,
	`address of the string table')
DEFINE_DYN_TYPE(`DT_SYMTAB',           6,
	`address of the symbol table')
DEFINE_DYN_TYPE(`DT_RELA',             7,
	`address of the relocation table')
DEFINE_DYN_TYPE(`DT_RELASZ',           8,
	`size of the DT_RELA table')
DEFINE_DYN_TYPE(`DT_RELAENT',          9,
	`size of each DT_RELA entry')
DEFINE_DYN_TYPE(`DT_STRSZ',            10,
	`size of the string table')
DEFINE_DYN_TYPE(`DT_SYMENT',           11,
	`size of a symbol table entry')
DEFINE_DYN_TYPE(`DT_INIT',             12,
	`address of the initialization function')
DEFINE_DYN_TYPE(`DT_FINI',             13,
	`address of the finalization function')
DEFINE_DYN_TYPE(`DT_SONAME',           14,
	`names the shared object')
DEFINE_DYN_TYPE(`DT_RPATH',            15,
	`runtime library search path')
DEFINE_DYN_TYPE(`DT_SYMBOLIC',         16,
	`alter symbol resolution algorithm')
DEFINE_DYN_TYPE(`DT_REL',              17,
	`address of the DT_REL table')
DEFINE_DYN_TYPE(`DT_RELSZ',            18,
	`size of the DT_REL table')
DEFINE_DYN_TYPE(`DT_RELENT',           19,
	`size of each DT_REL entry')
DEFINE_DYN_TYPE(`DT_PLTREL',           20,
	`type of relocation entry in the procedure linkage table')
DEFINE_DYN_TYPE(`DT_DEBUG',            21,
	`used for debugging')
DEFINE_DYN_TYPE(`DT_TEXTREL',          22,
	`text segment may be written to during relocation')
DEFINE_DYN_TYPE(`DT_JMPREL',           23,
	`address of relocation entries associated with the procedure linkage table')
DEFINE_DYN_TYPE(`DT_BIND_NOW',         24,
	`bind symbols at loading time')
DEFINE_DYN_TYPE(`DT_INIT_ARRAY',       25,
	`pointers to initialization functions')
DEFINE_DYN_TYPE(`DT_FINI_ARRAY',       26,
	`pointers to termination functions')
DEFINE_DYN_TYPE(`DT_INIT_ARRAYSZ',     27,
	`size of the DT_INIT_ARRAY')
DEFINE_DYN_TYPE(`DT_FINI_ARRAYSZ',     28,
	`size of the DT_FINI_ARRAY')
DEFINE_DYN_TYPE(`DT_RUNPATH',          29,
	`index of library search path string')
DEFINE_DYN_TYPE(`DT_FLAGS',            30,
	`flags specific to the object being loaded')
DEFINE_DYN_TYPE(`DT_ENCODING',         32,
	`standard semantics')
DEFINE_DYN_TYPE(`DT_PREINIT_ARRAY',    32,
	`pointers to pre-initialization functions')
DEFINE_DYN_TYPE(`DT_PREINIT_ARRAYSZ',  33,
	`size of pre-initialization array')
DEFINE_DYN_TYPE(`DT_MAXPOSTAGS',       34,
	`the number of positive tags')
DEFINE_DYN_TYPE(`DT_LOOS',             0x6000000DUL,
	`start of OS-specific types')
DEFINE_DYN_TYPE(`DT_SUNW_AUXILIARY',   0x6000000DUL,
	`offset of string naming auxiliary filtees')
DEFINE_DYN_TYPE(`DT_SUNW_RTLDINF',     0x6000000EUL,
	`rtld internal use')
DEFINE_DYN_TYPE(`DT_SUNW_FILTER',      0x6000000FUL,
	`offset of string naming standard filtees')
DEFINE_DYN_TYPE(`DT_SUNW_CAP',         0x60000010UL,
	`address of hardware capabilities section')
DEFINE_DYN_TYPE(`DT_SUNW_ASLR',        0x60000023UL,
	`Address Space Layout Randomization flag')
DEFINE_DYN_TYPE(`DT_HIOS',             0x6FFFF000UL,
	`end of OS-specific types')
DEFINE_DYN_TYPE(`DT_VALRNGLO',         0x6FFFFD00UL,
	`start of range using the d_val field')
DEFINE_DYN_TYPE(`DT_GNU_PRELINKED',    0x6FFFFDF5UL,
	`prelinking timestamp')
DEFINE_DYN_TYPE(`DT_GNU_CONFLICTSZ',   0x6FFFFDF6UL,
	`size of conflict section')
DEFINE_DYN_TYPE(`DT_GNU_LIBLISTSZ',    0x6FFFFDF7UL,
	`size of library list')
DEFINE_DYN_TYPE(`DT_CHECKSUM',         0x6FFFFDF8UL,
	`checksum for the object')
DEFINE_DYN_TYPE(`DT_PLTPADSZ',         0x6FFFFDF9UL,
	`size of PLT padding')
DEFINE_DYN_TYPE(`DT_MOVEENT',          0x6FFFFDFAUL,
	`size of DT_MOVETAB entries')
DEFINE_DYN_TYPE(`DT_MOVESZ',           0x6FFFFDFBUL,
	`total size of the MOVETAB table')
DEFINE_DYN_TYPE(`DT_FEATURE',          0x6FFFFDFCUL,
	`feature values')
DEFINE_DYN_TYPE(`DT_POSFLAG_1',        0x6FFFFDFDUL,
	`dynamic position flags')
DEFINE_DYN_TYPE(`DT_SYMINSZ',          0x6FFFFDFEUL,
	`size of the DT_SYMINFO table')
DEFINE_DYN_TYPE(`DT_SYMINENT',         0x6FFFFDFFUL,
	`size of a DT_SYMINFO entry')
DEFINE_DYN_TYPE(`DT_VALRNGHI',         0x6FFFFDFFUL,
	`end of range using the d_val field')
DEFINE_DYN_TYPE(`DT_ADDRRNGLO',        0x6FFFFE00UL,
	`start of range using the d_ptr field')
DEFINE_DYN_TYPE(`DT_GNU_HASH',	       0x6FFFFEF5UL,
	`GNU style hash tables')
DEFINE_DYN_TYPE(`DT_TLSDESC_PLT',      0x6FFFFEF6UL,
	`location of PLT entry for TLS descriptor resolver calls')
DEFINE_DYN_TYPE(`DT_TLSDESC_GOT',      0x6FFFFEF7UL,
	`location of GOT entry used by TLS descriptor resolver PLT entry')
DEFINE_DYN_TYPE(`DT_GNU_CONFLICT',     0x6FFFFEF8UL,
	`address of conflict section')
DEFINE_DYN_TYPE(`DT_GNU_LIBLIST',      0x6FFFFEF9UL,
	`address of conflict section')
DEFINE_DYN_TYPE(`DT_CONFIG',           0x6FFFFEFAUL,
	`configuration file')
DEFINE_DYN_TYPE(`DT_DEPAUDIT',         0x6FFFFEFBUL,
	`string defining audit libraries')
DEFINE_DYN_TYPE(`DT_AUDIT',            0x6FFFFEFCUL,
	`string defining audit libraries')
DEFINE_DYN_TYPE(`DT_PLTPAD',           0x6FFFFEFDUL,
	`PLT padding')
DEFINE_DYN_TYPE(`DT_MOVETAB',          0x6FFFFEFEUL,
	`address of a move table')
DEFINE_DYN_TYPE(`DT_SYMINFO',          0x6FFFFEFFUL,
	`address of the symbol information table')
DEFINE_DYN_TYPE(`DT_ADDRRNGHI',        0x6FFFFEFFUL,
	`end of range using the d_ptr field')
DEFINE_DYN_TYPE(`DT_VERSYM',	       0x6FFFFFF0UL,
	`address of the version section')
DEFINE_DYN_TYPE(`DT_RELACOUNT',        0x6FFFFFF9UL,
	`count of RELA relocations')
DEFINE_DYN_TYPE(`DT_RELCOUNT',         0x6FFFFFFAUL,
	`count of REL relocations')
DEFINE_DYN_TYPE(`DT_FLAGS_1',          0x6FFFFFFBUL,
	`flag values')
DEFINE_DYN_TYPE(`DT_VERDEF',	       0x6FFFFFFCUL,
	`address of the version definition segment')
DEFINE_DYN_TYPE(`DT_VERDEFNUM',	       0x6FFFFFFDUL,
	`the number of version definition entries')
DEFINE_DYN_TYPE(`DT_VERNEED',	       0x6FFFFFFEUL,
	`address of section with needed versions')
DEFINE_DYN_TYPE(`DT_VERNEEDNUM',       0x6FFFFFFFUL,
	`the number of version needed entries')
DEFINE_DYN_TYPE(`DT_LOPROC',           0x70000000UL,
	`start of processor-specific types')
DEFINE_DYN_TYPE(`DT_ARM_SYMTABSZ',     0x70000001UL,
	`number of entries in the dynamic symbol table')
DEFINE_DYN_TYPE(`DT_SPARC_REGISTER',   0x70000001UL,
	`index of an STT_SPARC_REGISTER symbol')
DEFINE_DYN_TYPE(`DT_ARM_PREEMPTMAP',   0x70000002UL,
	`address of the preemption map')
DEFINE_DYN_TYPE(`DT_MIPS_RLD_VERSION', 0x70000001UL,
	`version ID for runtime linker interface')
DEFINE_DYN_TYPE(`DT_MIPS_TIME_STAMP',  0x70000002UL,
	`timestamp')
DEFINE_DYN_TYPE(`DT_MIPS_ICHECKSUM',   0x70000003UL,
	`checksum of all external strings and common sizes')
DEFINE_DYN_TYPE(`DT_MIPS_IVERSION',    0x70000004UL,
	`string table index of a version string')
DEFINE_DYN_TYPE(`DT_MIPS_FLAGS',       0x70000005UL,
	`MIPS-specific flags')
DEFINE_DYN_TYPE(`DT_MIPS_BASE_ADDRESS', 0x70000006UL,
	`base address for the executable/DSO')
DEFINE_DYN_TYPE(`DT_MIPS_CONFLICT',    0x70000008UL,
	`address of .conflict section')
DEFINE_DYN_TYPE(`DT_MIPS_LIBLIST',     0x70000009UL,
	`address of .liblist section')
DEFINE_DYN_TYPE(`DT_MIPS_LOCAL_GOTNO', 0x7000000AUL,
	`number of local GOT entries')
DEFINE_DYN_TYPE(`DT_MIPS_CONFLICTNO',  0x7000000BUL,
	`number of entries in the .conflict section')
DEFINE_DYN_TYPE(`DT_MIPS_LIBLISTNO',   0x70000010UL,
	`number of entries in the .liblist section')
DEFINE_DYN_TYPE(`DT_MIPS_SYMTABNO',    0x70000011UL,
	`number of entries in the .dynsym section')
DEFINE_DYN_TYPE(`DT_MIPS_UNREFEXTNO',  0x70000012UL,
	`index of first external dynamic symbol not referenced locally')
DEFINE_DYN_TYPE(`DT_MIPS_GOTSYM',      0x70000013UL,
	`index of first dynamic symbol corresponds to a GOT entry')
DEFINE_DYN_TYPE(`DT_MIPS_HIPAGENO',    0x70000014UL,
	`number of page table entries in GOT')
DEFINE_DYN_TYPE(`DT_MIPS_RLD_MAP',     0x70000016UL,
	`address of runtime linker map')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_CLASS', 0x70000017UL,
	`Delta C++ class definition')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_CLASS_NO', 0x70000018UL,
	`number of entries in DT_MIPS_DELTA_CLASS')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_INSTANCE', 0x70000019UL,
	`Delta C++ class instances')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_INSTANCE_NO', 0x7000001AUL,
	`number of entries in DT_MIPS_DELTA_INSTANCE')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_RELOC', 0x7000001BUL,
	`Delta relocations')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_RELOC_NO', 0x7000001CUL,
	`number of entries in DT_MIPS_DELTA_RELOC')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_SYM',   0x7000001DUL,
	`Delta symbols referred by Delta relocations')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_SYM_NO', 0x7000001EUL,
	`number of entries in DT_MIPS_DELTA_SYM')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_CLASSSYM', 0x70000020UL,
	`Delta symbols for class declarations')
DEFINE_DYN_TYPE(`DT_MIPS_DELTA_CLASSSYM_NO', 0x70000021UL,
	`number of entries in DT_MIPS_DELTA_CLASSSYM')
DEFINE_DYN_TYPE(`DT_MIPS_CXX_FLAGS',   0x70000022UL,
	`C++ flavor flags')
DEFINE_DYN_TYPE(`DT_MIPS_PIXIE_INIT',  0x70000023UL,
	`address of an initialization routine created by pixie')
DEFINE_DYN_TYPE(`DT_MIPS_SYMBOL_LIB',  0x70000024UL,
	`address of .MIPS.symlib section')
DEFINE_DYN_TYPE(`DT_MIPS_LOCALPAGE_GOTIDX', 0x70000025UL,
	`GOT index of first page table entry for a segment')
DEFINE_DYN_TYPE(`DT_MIPS_LOCAL_GOTIDX', 0x70000026UL,
	`GOT index of first page table entry for a local symbol')
DEFINE_DYN_TYPE(`DT_MIPS_HIDDEN_GOTIDX', 0x70000027UL,
	`GOT index of first page table entry for a hidden symbol')
DEFINE_DYN_TYPE(`DT_MIPS_PROTECTED_GOTIDX', 0x70000028UL,
	`GOT index of first page table entry for a protected symbol')
DEFINE_DYN_TYPE(`DT_MIPS_OPTIONS',     0x70000029UL,
	`address of .MIPS.options section')
DEFINE_DYN_TYPE(`DT_MIPS_INTERFACE',   0x7000002AUL,
	`address of .MIPS.interface section')
DEFINE_DYN_TYPE(`DT_MIPS_DYNSTR_ALIGN', 0x7000002BUL,
	`???')
DEFINE_DYN_TYPE(`DT_MIPS_INTERFACE_SIZE', 0x7000002CUL,
	`size of .MIPS.interface section')
DEFINE_DYN_TYPE(`DT_MIPS_RLD_TEXT_RESOLVE_ADDR', 0x7000002DUL,
	`address of _rld_text_resolve in GOT')
DEFINE_DYN_TYPE(`DT_MIPS_PERF_SUFFIX', 0x7000002EUL,
	`default suffix of DSO to be appended by dlopen')
DEFINE_DYN_TYPE(`DT_MIPS_COMPACT_SIZE', 0x7000002FUL,
	`size of a ucode compact relocation record (o32)')
DEFINE_DYN_TYPE(`DT_MIPS_GP_VALUE',    0x70000030UL,
	`GP value of a specified GP relative range')
DEFINE_DYN_TYPE(`DT_MIPS_AUX_DYNAMIC', 0x70000031UL,
	`address of an auxiliary dynamic table')
DEFINE_DYN_TYPE(`DT_MIPS_PLTGOT',      0x70000032UL,
	`address of the PLTGOT')
DEFINE_DYN_TYPE(`DT_MIPS_RLD_OBJ_UPDATE', 0x70000033UL,
	`object list update callback')
DEFINE_DYN_TYPE(`DT_MIPS_RWPLT',       0x70000034UL,
	`address of a writable PLT')
DEFINE_DYN_TYPE(`DT_PPC_GOT',          0x70000000UL,
	`value of _GLOBAL_OFFSET_TABLE_')
DEFINE_DYN_TYPE(`DT_PPC_TLSOPT',       0x70000001UL,
	`TLS descriptor should be optimized')
DEFINE_DYN_TYPE(`DT_PPC64_GLINK',      0x70000000UL,
	`address of .glink section')
DEFINE_DYN_TYPE(`DT_PPC64_OPD',        0x70000001UL,
	`address of .opd section')
DEFINE_DYN_TYPE(`DT_PPC64_OPDSZ',      0x70000002UL,
	`size of .opd section')
DEFINE_DYN_TYPE(`DT_PPC64_TLSOPT',     0x70000003UL,
	`TLS descriptor should be optimized')
DEFINE_DYN_TYPE(`DT_AUXILIARY',        0x7FFFFFFDUL,
	`offset of string naming auxiliary filtees')
DEFINE_DYN_TYPE(`DT_USED',             0x7FFFFFFEUL,
	`ignored')
DEFINE_DYN_TYPE(`DT_FILTER',           0x7FFFFFFFUL,
	`index of string naming filtees')
DEFINE_DYN_TYPE(`DT_HIPROC',           0x7FFFFFFFUL,
	`end of processor-specific types')
')

define(`DEFINE_DYN_TYPE_ALIASES',`
DEFINE_DYN_TYPE_ALIAS(`DT_DEPRECATED_SPARC_REGISTER', `DT_SPARC_REGISTER')
')

#
# Flags used in the executable header (field: e_flags).
#
define(`DEFINE_EHDR_FLAGS',`
DEFINE_EHDR_FLAG(EF_ARM_RELEXEC,      0x00000001UL,
	`dynamic segment describes only how to relocate segments')
DEFINE_EHDR_FLAG(EF_ARM_HASENTRY,     0x00000002UL,
	`e_entry contains a program entry point')
DEFINE_EHDR_FLAG(EF_ARM_SYMSARESORTED, 0x00000004UL,
	`subsection of symbol table is sorted by symbol value')
DEFINE_EHDR_FLAG(EF_ARM_DYNSYMSUSESEGIDX, 0x00000008UL,
	`dynamic symbol st_shndx = containing segment index + 1')
DEFINE_EHDR_FLAG(EF_ARM_MAPSYMSFIRST, 0x00000010UL,
	`mapping symbols precede other local symbols in symtab')
DEFINE_EHDR_FLAG(EF_ARM_BE8,          0x00800000UL,
	`file contains BE-8 code')
DEFINE_EHDR_FLAG(EF_ARM_LE8,          0x00400000UL,
	`file contains LE-8 code')
DEFINE_EHDR_FLAG(EF_ARM_EABIMASK,     0xFF000000UL,
	`mask for ARM EABI version number (0 denotes GNU or unknown)')
DEFINE_EHDR_FLAG(EF_ARM_EABI_UNKNOWN, 0x00000000UL,
	`Unknown or GNU ARM EABI version number')
DEFINE_EHDR_FLAG(EF_ARM_EABI_VER1,    0x01000000UL,
	`ARM EABI version 1')
DEFINE_EHDR_FLAG(EF_ARM_EABI_VER2,    0x02000000UL,
	`ARM EABI version 2')
DEFINE_EHDR_FLAG(EF_ARM_EABI_VER3,    0x03000000UL,
	`ARM EABI version 3')
DEFINE_EHDR_FLAG(EF_ARM_EABI_VER4,    0x04000000UL,
	`ARM EABI version 4')
DEFINE_EHDR_FLAG(EF_ARM_EABI_VER5,    0x05000000UL,
	`ARM EABI version 5')
DEFINE_EHDR_FLAG(EF_ARM_INTERWORK,    0x00000004UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_APCS_26,      0x00000008UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_APCS_FLOAT,   0x00000010UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_PIC,          0x00000020UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_ALIGN8,       0x00000040UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_NEW_ABI,      0x00000080UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_OLD_ABI,      0x00000100UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_SOFT_FLOAT,   0x00000200UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_VFP_FLOAT,    0x00000400UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_ARM_MAVERICK_FLOAT, 0x00000800UL,
	`GNU EABI extension')
DEFINE_EHDR_FLAG(EF_MIPS_NOREORDER,   0x00000001UL,
	`at least one .noreorder directive appeared in the source')
DEFINE_EHDR_FLAG(EF_MIPS_PIC,         0x00000002UL,
	`file contains position independent code')
DEFINE_EHDR_FLAG(EF_MIPS_CPIC,        0x00000004UL,
	`file code uses standard conventions for calling PIC')
DEFINE_EHDR_FLAG(EF_MIPS_UCODE,       0x00000010UL,
	`file contains UCODE (obsolete)')
DEFINE_EHDR_FLAG(EF_MIPS_ABI,	      0x00007000UL,
	`Application binary interface, see E_MIPS_* values')
DEFINE_EHDR_FLAG(EF_MIPS_ABI2,        0x00000020UL,
	`file follows MIPS III 32-bit ABI')
DEFINE_EHDR_FLAG(EF_MIPS_OPTIONS_FIRST, 0x00000080UL,
	`ld(1) should process .MIPS.options section first')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_ASE,    0x0F000000UL,
	`file uses application-specific architectural extensions')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_ASE_MDMX, 0x08000000UL,
	`file uses MDMX multimedia extensions')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_ASE_M16, 0x04000000UL,
	`file uses MIPS-16 ISA extensions')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_ASE_MICROMIPS, 0x02000000UL,
	`MicroMIPS architecture')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH,         0xF0000000UL,
	`4-bit MIPS architecture field')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_1,	0x00000000UL,
	`MIPS I instruction set')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_2,	0x10000000UL,
	`MIPS II instruction set')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_3,	0x20000000UL,
	`MIPS III instruction set')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_4,	0x30000000UL,
	`MIPS IV instruction set')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_5,	0x40000000UL,
	`Never introduced')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_32,	0x50000000UL,
	`Mips32 Revision 1')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_64,	0x60000000UL,
	`Mips64 Revision 1')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_32R2,	0x70000000UL,
	`Mips32 Revision 2')
DEFINE_EHDR_FLAG(EF_MIPS_ARCH_64R2,	0x80000000UL,
	`Mips64 Revision 2')
DEFINE_EHDR_FLAG(EF_PPC_EMB,          0x80000000UL,
	`Embedded PowerPC flag')
DEFINE_EHDR_FLAG(EF_PPC_RELOCATABLE,  0x00010000UL,
	`-mrelocatable flag')
DEFINE_EHDR_FLAG(EF_PPC_RELOCATABLE_LIB, 0x00008000UL,
	`-mrelocatable-lib flag')
DEFINE_EHDR_FLAG(EF_RISCV_RVC,	    0x00000001UL,
	`Compressed instruction extension')
DEFINE_EHDR_FLAG(EF_RISCV_FLOAT_ABI_MASK, 0x00000006UL,
	`Floating point ABI')
DEFINE_EHDR_FLAG(EF_RISCV_FLOAT_ABI_SOFT, 0x00000000UL,
	`Software emulated floating point')
DEFINE_EHDR_FLAG(EF_RISCV_FLOAT_ABI_SINGLE, 0x00000002UL,
	`Single precision floating point')
DEFINE_EHDR_FLAG(EF_RISCV_FLOAT_ABI_DOUBLE, 0x00000004UL,
	`Double precision floating point')
DEFINE_EHDR_FLAG(EF_RISCV_FLOAT_ABI_QUAD, 0x00000006UL,
	`Quad precision floating point')
DEFINE_EHDR_FLAG(EF_RISCV_RVE,	    0x00000008UL,
	`Compressed instruction ABI')
DEFINE_EHDR_FLAG(EF_RISCV_TSO,	    0x00000010UL,
	`RVTSO memory consistency model')
DEFINE_EHDR_FLAG(EF_SPARC_EXT_MASK,   0x00ffff00UL,
	`Vendor Extension mask')
DEFINE_EHDR_FLAG(EF_SPARC_32PLUS,     0x00000100UL,
	`Generic V8+ features')
DEFINE_EHDR_FLAG(EF_SPARC_SUN_US1,    0x00000200UL,
	`Sun UltraSPARCTM 1 Extensions')
DEFINE_EHDR_FLAG(EF_SPARC_HAL_R1,     0x00000400UL,
	`HAL R1 Extensions')
DEFINE_EHDR_FLAG(EF_SPARC_SUN_US3,    0x00000800UL,
	`Sun UltraSPARC 3 Extensions')
DEFINE_EHDR_FLAG(EF_SPARCV9_MM,       0x00000003UL,
	`Mask for Memory Model')
DEFINE_EHDR_FLAG(EF_SPARCV9_TSO,      0x00000000UL,
	`Total Store Ordering')
DEFINE_EHDR_FLAG(EF_SPARCV9_PSO,      0x00000001UL,
	`Partial Store Ordering')
DEFINE_EHDR_FLAG(EF_SPARCV9_RMO,      0x00000002UL,
	`Relaxed Memory Ordering')
')

#
# Offsets in the `ei_ident[]` field of an ELF executable header.
#
define(`DEFINE_EI_OFFSETS',`
DEFINE_EI_OFFSET(EI_MAG0,     0,
	`magic number')
DEFINE_EI_OFFSET(EI_MAG1,     1,
	`magic number')
DEFINE_EI_OFFSET(EI_MAG2,     2,
	`magic number')
DEFINE_EI_OFFSET(EI_MAG3,     3,
	`magic number')
DEFINE_EI_OFFSET(EI_CLASS,    4,
	`file class')
DEFINE_EI_OFFSET(EI_DATA,     5,
	`data encoding')
DEFINE_EI_OFFSET(EI_VERSION,  6,
	`file version')
DEFINE_EI_OFFSET(EI_OSABI,    7,
	`OS ABI kind')
DEFINE_EI_OFFSET(EI_ABIVERSION, 8,
	`OS ABI version')
DEFINE_EI_OFFSET(EI_PAD,	    9,
	`padding start')
DEFINE_EI_OFFSET(EI_NIDENT,  16,
	`total size')
')

#
# The ELF class of an object.
#
define(`DEFINE_ELF_CLASSES',`
DEFINE_ELF_CLASS(ELFCLASSNONE, 0,
	`Unknown ELF class')
DEFINE_ELF_CLASS(ELFCLASS32,   1,
	`32 bit objects')
DEFINE_ELF_CLASS(ELFCLASS64,   2,
	`64 bit objects')
')

#
# Endianness of data in an ELF object.
#
define(`DEFINE_ELF_DATA_ENDIANNESSES',`
DEFINE_ELF_DATA_ENDIANNESS(ELFDATANONE, 0,
	`Unknown data endianness')
DEFINE_ELF_DATA_ENDIANNESS(ELFDATA2LSB, 1,
	`little endian')
DEFINE_ELF_DATA_ENDIANNESS(ELFDATA2MSB, 2,
	`big endian')
')


#
# Values of the magic numbers used in identification array.
#
changequote({,})
define({DEFINE_ELF_MAGIC_VALUES},{
DEFINE_ELF_MAGIC_VALUE(ELFMAG0, 0x7FU)
DEFINE_ELF_MAGIC_VALUE(ELFMAG1, 'E')
DEFINE_ELF_MAGIC_VALUE(ELFMAG2, 'L')
DEFINE_ELF_MAGIC_VALUE(ELFMAG3, 'F')
})
changequote

#
# ELF OS ABI field.
#
define(`DEFINE_ELF_OSABIS',`
DEFINE_ELF_OSABI(ELFOSABI_NONE,       0,
	`No extensions or unspecified')
DEFINE_ELF_OSABI(ELFOSABI_SYSV,       0,
	`SYSV')
DEFINE_ELF_OSABI(ELFOSABI_HPUX,       1,
	`Hewlett-Packard HP-UX')
DEFINE_ELF_OSABI(ELFOSABI_NETBSD,     2,
	`NetBSD')
DEFINE_ELF_OSABI(ELFOSABI_GNU,        3,
	`GNU')
DEFINE_ELF_OSABI(ELFOSABI_HURD,       4,
	`GNU/HURD')
DEFINE_ELF_OSABI(ELFOSABI_86OPEN,     5,
	`86Open Common ABI')
DEFINE_ELF_OSABI(ELFOSABI_SOLARIS,    6,
	`Sun Solaris')
DEFINE_ELF_OSABI(ELFOSABI_AIX,        7,
	`AIX')
DEFINE_ELF_OSABI(ELFOSABI_IRIX,       8,
	`IRIX')
DEFINE_ELF_OSABI(ELFOSABI_FREEBSD,    9,
	`FreeBSD')
DEFINE_ELF_OSABI(ELFOSABI_TRU64,      10,
	`Compaq TRU64 UNIX')
DEFINE_ELF_OSABI(ELFOSABI_MODESTO,    11,
	`Novell Modesto')
DEFINE_ELF_OSABI(ELFOSABI_OPENBSD,    12,
	`Open BSD')
DEFINE_ELF_OSABI(ELFOSABI_OPENVMS,    13,
	`Open VMS')
DEFINE_ELF_OSABI(ELFOSABI_NSK,        14,
	`Hewlett-Packard Non-Stop Kernel')
DEFINE_ELF_OSABI(ELFOSABI_AROS,       15,
	`Amiga Research OS')
DEFINE_ELF_OSABI(ELFOSABI_FENIXOS,    16,
	`The FenixOS highly scalable multi-core OS')
DEFINE_ELF_OSABI(ELFOSABI_CLOUDABI,   17,
	`Nuxi CloudABI')
DEFINE_ELF_OSABI(ELFOSABI_OPENVOS,    18,
	`Stratus Technologies OpenVOS')
DEFINE_ELF_OSABI(ELFOSABI_ARM_AEABI,  64,
	`ARM specific symbol versioning extensions')
DEFINE_ELF_OSABI(ELFOSABI_ARM,        97,
	`ARM ABI')
DEFINE_ELF_OSABI(ELFOSABI_STANDALONE, 255,
	`Standalone (embedded) application')
')

# OS ABI aliases.
define(`DEFINE_ELF_OSABI_ALIASES',`
DEFINE_ELF_OSABI_ALIAS(ELFOSABI_LINUX,	ELFOSABI_GNU)
')

#
# ELF Machine types: (EM_*).
#
define(`DEFINE_ELF_MACHINE_TYPES',`
DEFINE_ELF_MACHINE_TYPE(EM_NONE,             0,
	`No machine')
DEFINE_ELF_MACHINE_TYPE(EM_M32,              1,
	`AT&T WE 32100')
DEFINE_ELF_MACHINE_TYPE(EM_SPARC,            2,
	`SPARC')
DEFINE_ELF_MACHINE_TYPE(EM_386,              3,
	`Intel 80386')
DEFINE_ELF_MACHINE_TYPE(EM_68K,              4,
	`Motorola 68000')
DEFINE_ELF_MACHINE_TYPE(EM_88K,              5,
	`Motorola 88000')
DEFINE_ELF_MACHINE_TYPE(EM_IAMCU,            6,
	`Intel MCU')
DEFINE_ELF_MACHINE_TYPE(EM_860,              7,
	`Intel 80860')
DEFINE_ELF_MACHINE_TYPE(EM_MIPS,             8,
	`MIPS I Architecture')
DEFINE_ELF_MACHINE_TYPE(EM_S370,             9,
	`IBM System/370 Processor')
DEFINE_ELF_MACHINE_TYPE(EM_MIPS_RS3_LE,      10,
	`MIPS RS3000 Little-endian')
DEFINE_ELF_MACHINE_TYPE(EM_PARISC,           15,
	`Hewlett-Packard PA-RISC')
DEFINE_ELF_MACHINE_TYPE(EM_VPP500,           17,
	`Fujitsu VPP500')
DEFINE_ELF_MACHINE_TYPE(EM_SPARC32PLUS,      18,
	`Enhanced instruction set SPARC')
DEFINE_ELF_MACHINE_TYPE(EM_960,              19,
	`Intel 80960')
DEFINE_ELF_MACHINE_TYPE(EM_PPC,              20,
	`PowerPC')
DEFINE_ELF_MACHINE_TYPE(EM_PPC64,            21,
	`64-bit PowerPC')
DEFINE_ELF_MACHINE_TYPE(EM_S390,             22,
	`IBM System/390 Processor')
DEFINE_ELF_MACHINE_TYPE(EM_SPU,              23,
	`IBM SPU/SPC')
DEFINE_ELF_MACHINE_TYPE(EM_V800,             36,
	`NEC V800')
DEFINE_ELF_MACHINE_TYPE(EM_FR20,             37,
	`Fujitsu FR20')
DEFINE_ELF_MACHINE_TYPE(EM_RH32,             38,
	`TRW RH-32')
DEFINE_ELF_MACHINE_TYPE(EM_RCE,              39,
	`Motorola RCE')
DEFINE_ELF_MACHINE_TYPE(EM_ARM,              40,
	`Advanced RISC Machines ARM')
DEFINE_ELF_MACHINE_TYPE(EM_ALPHA,            41,
	`Digital Alpha')
DEFINE_ELF_MACHINE_TYPE(EM_SH,               42,
	`Hitachi SH')
DEFINE_ELF_MACHINE_TYPE(EM_SPARCV9,          43,
	`SPARC Version 9')
DEFINE_ELF_MACHINE_TYPE(EM_TRICORE,          44,
	`Siemens TriCore embedded processor')
DEFINE_ELF_MACHINE_TYPE(EM_ARC,              45,
	`Argonaut RISC Core, Argonaut Technologies Inc.')
DEFINE_ELF_MACHINE_TYPE(EM_H8_300,           46,
	`Hitachi H8/300')
DEFINE_ELF_MACHINE_TYPE(EM_H8_300H,          47,
	`Hitachi H8/300H')
DEFINE_ELF_MACHINE_TYPE(EM_H8S,              48,
	`Hitachi H8S')
DEFINE_ELF_MACHINE_TYPE(EM_H8_500,           49,
	`Hitachi H8/500')
DEFINE_ELF_MACHINE_TYPE(EM_IA_64,            50,
	`Intel IA-64 processor architecture')
DEFINE_ELF_MACHINE_TYPE(EM_MIPS_X,           51,
	`Stanford MIPS-X')
DEFINE_ELF_MACHINE_TYPE(EM_COLDFIRE,         52,
	`Motorola ColdFire')
DEFINE_ELF_MACHINE_TYPE(EM_68HC12,           53,
	`Motorola M68HC12')
DEFINE_ELF_MACHINE_TYPE(EM_MMA,              54,
	`Fujitsu MMA Multimedia Accelerator')
DEFINE_ELF_MACHINE_TYPE(EM_PCP,              55,
	`Siemens PCP')
DEFINE_ELF_MACHINE_TYPE(EM_NCPU,             56,
	`Sony nCPU embedded RISC processor')
DEFINE_ELF_MACHINE_TYPE(EM_NDR1,             57,
	`Denso NDR1 microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_STARCORE,         58,
	`Motorola Star*Core processor')
DEFINE_ELF_MACHINE_TYPE(EM_ME16,             59,
	`Toyota ME16 processor')
DEFINE_ELF_MACHINE_TYPE(EM_ST100,            60,
	`STMicroelectronics ST100 processor')
DEFINE_ELF_MACHINE_TYPE(EM_TINYJ,            61,
	`Advanced Logic Corp. TinyJ embedded processor family')
DEFINE_ELF_MACHINE_TYPE(EM_X86_64,           62,
	`AMD x86-64 architecture')
DEFINE_ELF_MACHINE_TYPE(EM_PDSP,             63,
	`Sony DSP Processor')
DEFINE_ELF_MACHINE_TYPE(EM_PDP10,            64,
	`Digital Equipment Corp. PDP-10')
DEFINE_ELF_MACHINE_TYPE(EM_PDP11,            65,
	`Digital Equipment Corp. PDP-11')
DEFINE_ELF_MACHINE_TYPE(EM_FX66,             66,
	`Siemens FX66 microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_ST9PLUS,          67,
	`STMicroelectronics ST9+ 8/16 bit microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_ST7,              68,
	`STMicroelectronics ST7 8-bit microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_68HC16,           69,
	`Motorola MC68HC16 Microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_68HC11,           70,
	`Motorola MC68HC11 Microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_68HC08,           71,
	`Motorola MC68HC08 Microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_68HC05,           72,
	`Motorola MC68HC05 Microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_SVX,              73,
	`Silicon Graphics SVx')
DEFINE_ELF_MACHINE_TYPE(EM_ST19,             74,
	`STMicroelectronics ST19 8-bit microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_VAX,              75,
	`Digital VAX')
DEFINE_ELF_MACHINE_TYPE(EM_CRIS,             76,
	`Axis Communications 32-bit embedded processor')
DEFINE_ELF_MACHINE_TYPE(EM_JAVELIN,          77,
	`Infineon Technologies 32-bit embedded processor')
DEFINE_ELF_MACHINE_TYPE(EM_FIREPATH,         78,
	`Element 14 64-bit DSP Processor')
DEFINE_ELF_MACHINE_TYPE(EM_ZSP,              79,
	`LSI Logic 16-bit DSP Processor')
DEFINE_ELF_MACHINE_TYPE(EM_MMIX,             80,
	`Educational 64-bit processor by Donald Knuth')
DEFINE_ELF_MACHINE_TYPE(EM_HUANY,            81,
	`Harvard University machine-independent object files')
DEFINE_ELF_MACHINE_TYPE(EM_PRISM,            82,
	`SiTera Prism')
DEFINE_ELF_MACHINE_TYPE(EM_AVR,              83,
	`Atmel AVR 8-bit microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_FR30,             84,
	`Fujitsu FR30')
DEFINE_ELF_MACHINE_TYPE(EM_D10V,             85,
	`Mitsubishi D10V')
DEFINE_ELF_MACHINE_TYPE(EM_D30V,             86,
	`Mitsubishi D30V')
DEFINE_ELF_MACHINE_TYPE(EM_V850,             87,
	`NEC v850')
DEFINE_ELF_MACHINE_TYPE(EM_M32R,             88,
	`Mitsubishi M32R')
DEFINE_ELF_MACHINE_TYPE(EM_MN10300,          89,
	`Matsushita MN10300')
DEFINE_ELF_MACHINE_TYPE(EM_MN10200,          90,
	`Matsushita MN10200')
DEFINE_ELF_MACHINE_TYPE(EM_PJ,               91,
	`picoJava')
DEFINE_ELF_MACHINE_TYPE(EM_OPENRISC,         92,
	`OpenRISC 32-bit embedded processor')
DEFINE_ELF_MACHINE_TYPE(EM_ARC_COMPACT,      93,
	`ARC International ARCompact processor')
DEFINE_ELF_MACHINE_TYPE(EM_XTENSA,           94,
	`Tensilica Xtensa Architecture')
DEFINE_ELF_MACHINE_TYPE(EM_VIDEOCORE,        95,
	`Alphamosaic VideoCore processor')
DEFINE_ELF_MACHINE_TYPE(EM_TMM_GPP,          96,
	`Thompson Multimedia General Purpose Processor')
DEFINE_ELF_MACHINE_TYPE(EM_NS32K,            97,
	`National Semiconductor 32000 series')
DEFINE_ELF_MACHINE_TYPE(EM_TPC,              98,
	`Tenor Network TPC processor')
DEFINE_ELF_MACHINE_TYPE(EM_SNP1K,            99,
	`Trebia SNP 1000 processor')
DEFINE_ELF_MACHINE_TYPE(EM_ST200,            100,
	`STMicroelectronics (www.st.com) ST200 microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_IP2K,             101,
	`Ubicom IP2xxx microcontroller family')
DEFINE_ELF_MACHINE_TYPE(EM_MAX,              102,
	`MAX Processor')
DEFINE_ELF_MACHINE_TYPE(EM_CR,               103,
	`National Semiconductor CompactRISC microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_F2MC16,           104,
	`Fujitsu F2MC16')
DEFINE_ELF_MACHINE_TYPE(EM_MSP430,           105,
	`Texas Instruments embedded microcontroller msp430')
DEFINE_ELF_MACHINE_TYPE(EM_BLACKFIN,         106,
	`Analog Devices Blackfin (DSP) processor')
DEFINE_ELF_MACHINE_TYPE(EM_SE_C33,           107,
	`S1C33 Family of Seiko Epson processors')
DEFINE_ELF_MACHINE_TYPE(EM_SEP,              108,
	`Sharp embedded microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_ARCA,             109,
	`Arca RISC Microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_UNICORE,          110,
	`Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University')
DEFINE_ELF_MACHINE_TYPE(EM_EXCESS,           111,
	`eXcess: 16/32/64-bit configurable embedded CPU')
DEFINE_ELF_MACHINE_TYPE(EM_DXP,              112,
	`Icera Semiconductor Inc. Deep Execution Processor')
DEFINE_ELF_MACHINE_TYPE(EM_ALTERA_NIOS2,     113,
	`Altera Nios II soft-core processor')
DEFINE_ELF_MACHINE_TYPE(EM_CRX,              114,
	`National Semiconductor CompactRISC CRX microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_XGATE,            115,
	`Motorola XGATE embedded processor')
DEFINE_ELF_MACHINE_TYPE(EM_C166,             116,
	`Infineon C16x/XC16x processor')
DEFINE_ELF_MACHINE_TYPE(EM_M16C,             117,
	`Renesas M16C series microprocessors')
DEFINE_ELF_MACHINE_TYPE(EM_DSPIC30F,         118,
	`Microchip Technology dsPIC30F Digital Signal Controller')
DEFINE_ELF_MACHINE_TYPE(EM_CE,               119,
	`Freescale Communication Engine RISC core')
DEFINE_ELF_MACHINE_TYPE(EM_M32C,             120,
	`Renesas M32C series microprocessors')
DEFINE_ELF_MACHINE_TYPE(EM_TSK3000,          131,
	`Altium TSK3000 core')
DEFINE_ELF_MACHINE_TYPE(EM_RS08,             132,
	`Freescale RS08 embedded processor')
DEFINE_ELF_MACHINE_TYPE(EM_SHARC,            133,
	`Analog Devices SHARC family of 32-bit DSP processors')
DEFINE_ELF_MACHINE_TYPE(EM_ECOG2,            134,
	`Cyan Technology eCOG2 microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_SCORE7,           135,
	`Sunplus S+core7 RISC processor')
DEFINE_ELF_MACHINE_TYPE(EM_DSP24,            136,
	`New Japan Radio (NJR) 24-bit DSP Processor')
DEFINE_ELF_MACHINE_TYPE(EM_VIDEOCORE3,       137,
	`Broadcom VideoCore III processor')
DEFINE_ELF_MACHINE_TYPE(EM_LATTICEMICO32,    138,
	`RISC processor for Lattice FPGA architecture')
DEFINE_ELF_MACHINE_TYPE(EM_SE_C17,           139,
	`Seiko Epson C17 family')
DEFINE_ELF_MACHINE_TYPE(EM_TI_C6000,         140,
	`The Texas Instruments TMS320C6000 DSP family')
DEFINE_ELF_MACHINE_TYPE(EM_TI_C2000,         141,
	`The Texas Instruments TMS320C2000 DSP family')
DEFINE_ELF_MACHINE_TYPE(EM_TI_C5500,         142,
	`The Texas Instruments TMS320C55x DSP family')
DEFINE_ELF_MACHINE_TYPE(EM_MMDSP_PLUS,       160,
	`STMicroelectronics 64bit VLIW Data Signal Processor')
DEFINE_ELF_MACHINE_TYPE(EM_CYPRESS_M8C,      161,
	`Cypress M8C microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_R32C,             162,
	`Renesas R32C series microprocessors')
DEFINE_ELF_MACHINE_TYPE(EM_TRIMEDIA,         163,
	`NXP Semiconductors TriMedia architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_QDSP6,            164,
	`QUALCOMM DSP6 Processor')
DEFINE_ELF_MACHINE_TYPE(EM_8051,             165,
	`Intel 8051 and variants')
DEFINE_ELF_MACHINE_TYPE(EM_STXP7X,           166,
	`STMicroelectronics STxP7x family of configurable and extensible RISC processors')
DEFINE_ELF_MACHINE_TYPE(EM_NDS32,            167,
	`Andes Technology compact code size embedded RISC processor family')
DEFINE_ELF_MACHINE_TYPE(EM_ECOG1,            168,
	`Cyan Technology eCOG1X family')
DEFINE_ELF_MACHINE_TYPE(EM_ECOG1X,           168,
	`Cyan Technology eCOG1X family')
DEFINE_ELF_MACHINE_TYPE(EM_MAXQ30,           169,
	`Dallas Semiconductor MAXQ30 Core Micro-controllers')
DEFINE_ELF_MACHINE_TYPE(EM_XIMO16,           170,
	`New Japan Radio (NJR) 16-bit DSP Processor')
DEFINE_ELF_MACHINE_TYPE(EM_MANIK,            171,
	`M2000 Reconfigurable RISC Microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_CRAYNV2,          172,
	`Cray Inc. NV2 vector architecture')
DEFINE_ELF_MACHINE_TYPE(EM_RX,               173,
	`Renesas RX family')
DEFINE_ELF_MACHINE_TYPE(EM_METAG,            174,
	`Imagination Technologies META processor architecture')
DEFINE_ELF_MACHINE_TYPE(EM_MCST_ELBRUS,      175,
	`MCST Elbrus general purpose hardware architecture')
DEFINE_ELF_MACHINE_TYPE(EM_ECOG16,           176,
	`Cyan Technology eCOG16 family')
DEFINE_ELF_MACHINE_TYPE(EM_CR16,             177,
	`National Semiconductor CompactRISC CR16 16-bit microprocessor')
DEFINE_ELF_MACHINE_TYPE(EM_ETPU,             178,
	`Freescale Extended Time Processing Unit')
DEFINE_ELF_MACHINE_TYPE(EM_SLE9X,            179,
	`Infineon Technologies SLE9X core')
DEFINE_ELF_MACHINE_TYPE(EM_AARCH64,          183,
	`AArch64 (64-bit ARM)')
DEFINE_ELF_MACHINE_TYPE(EM_AVR32,            185,
	`Atmel Corporation 32-bit microprocessor family')
DEFINE_ELF_MACHINE_TYPE(EM_STM8,             186,
	`STMicroeletronics STM8 8-bit microcontroller')
DEFINE_ELF_MACHINE_TYPE(EM_TILE64,           187,
	`Tilera TILE64 multicore architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_TILEPRO,          188,
	`Tilera TILEPro multicore architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_MICROBLAZE,       189,
	`Xilinx MicroBlaze 32-bit RISC soft processor core')
DEFINE_ELF_MACHINE_TYPE(EM_CUDA,             190,
	`NVIDIA CUDA architecture')
DEFINE_ELF_MACHINE_TYPE(EM_TILEGX,           191,
	`Tilera TILE-Gx multicore architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_CLOUDSHIELD,      192,
	`CloudShield architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_COREA_1ST,        193,
	`KIPO-KAIST Core-A 1st generation processor family')
DEFINE_ELF_MACHINE_TYPE(EM_COREA_2ND,        194,
	`KIPO-KAIST Core-A 2nd generation processor family')
DEFINE_ELF_MACHINE_TYPE(EM_ARC_COMPACT2,     195,
	`Synopsys ARCompact V2')
DEFINE_ELF_MACHINE_TYPE(EM_OPEN8,            196,
	`Open8 8-bit RISC soft processor core')
DEFINE_ELF_MACHINE_TYPE(EM_RL78,             197,
	`Renesas RL78 family')
DEFINE_ELF_MACHINE_TYPE(EM_VIDEOCORE5,       198,
	`Broadcom VideoCore V processor')
DEFINE_ELF_MACHINE_TYPE(EM_78KOR,            199,
	`Renesas 78KOR family')
DEFINE_ELF_MACHINE_TYPE(EM_56800EX,          200,
	`Freescale 56800EX Digital Signal Controller')
DEFINE_ELF_MACHINE_TYPE(EM_BA1,              201,
	`Beyond BA1 CPU architecture')
DEFINE_ELF_MACHINE_TYPE(EM_BA2,              202,
	`Beyond BA2 CPU architecture')
DEFINE_ELF_MACHINE_TYPE(EM_XCORE,            203,
	`XMOS xCORE processor family')
DEFINE_ELF_MACHINE_TYPE(EM_MCHP_PIC,         204,
	`Microchip 8-bit PIC(r) family')
DEFINE_ELF_MACHINE_TYPE(EM_INTELGT,          205,
	`Intel Graphics Technology')
DEFINE_ELF_MACHINE_TYPE(EM_INTEL206,         206,
	`Reserved by Intel')
DEFINE_ELF_MACHINE_TYPE(EM_INTEL207,         207,
	`Reserved by Intel')
DEFINE_ELF_MACHINE_TYPE(EM_INTEL208,         208,
	`Reserved by Intel')
DEFINE_ELF_MACHINE_TYPE(EM_INTEL209,         209,
	`Reserved by Intel')
DEFINE_ELF_MACHINE_TYPE(EM_KM32,             210,
	`KM211 KM32 32-bit processor')
DEFINE_ELF_MACHINE_TYPE(EM_KMX32,            211,
	`KM211 KMX32 32-bit processor')
DEFINE_ELF_MACHINE_TYPE(EM_KMX16,            212,
	`KM211 KMX16 16-bit processor')
DEFINE_ELF_MACHINE_TYPE(EM_KMX8,             213,
	`KM211 KMX8 8-bit processor')
DEFINE_ELF_MACHINE_TYPE(EM_KVARC,            214,
	`KM211 KMX32 KVARC processor')
DEFINE_ELF_MACHINE_TYPE(EM_CDP,              215,
	`Paneve CDP architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_COGE,             216,
	`Cognitive Smart Memory Processor')
DEFINE_ELF_MACHINE_TYPE(EM_COOL,             217,
	`Bluechip Systems CoolEngine')
DEFINE_ELF_MACHINE_TYPE(EM_NORC,             218,
	`Nanoradio Optimized RISC')
DEFINE_ELF_MACHINE_TYPE(EM_CSR_KALIMBA,      219,
	`CSR Kalimba architecture family')
DEFINE_ELF_MACHINE_TYPE(EM_Z80,              220,
	`Zilog Z80')
DEFINE_ELF_MACHINE_TYPE(EM_VISIUM,           221,
	`Controls and Data Services VISIUMcore processor')
DEFINE_ELF_MACHINE_TYPE(EM_FT32,             222,
	`FTDI Chip FT32 high performance 32-bit RISC architecture')
DEFINE_ELF_MACHINE_TYPE(EM_MOXIE,            223,
	`Moxie processor family')
DEFINE_ELF_MACHINE_TYPE(EM_AMDGPU,           224,
	`AMD GPU architecture')
DEFINE_ELF_MACHINE_TYPE(EM_RISCV,            243,
	`RISC-V')
DEFINE_ELF_MACHINE_TYPE(EM_LANAI,            244,
	`Lanai processor')
DEFINE_ELF_MACHINE_TYPE(EM_CEVA,             245,
	`CEVA Processor Architecture Family')
DEFINE_ELF_MACHINE_TYPE(EM_CEVA_X2,          246,
	`CEVA X2 Processor Family')
DEFINE_ELF_MACHINE_TYPE(EM_BPF,              247,
	`Linux BPF – in-kernel virtual machine')
DEFINE_ELF_MACHINE_TYPE(EM_GRAPHCORE_IPU,    248,
	`Graphcore Intelligent Processing Unit')
DEFINE_ELF_MACHINE_TYPE(EM_IMG1,             249,
	`Imagination Technologies')
DEFINE_ELF_MACHINE_TYPE(EM_NFP,              250,
	`Netronome Flow Processor (NFP)')
DEFINE_ELF_MACHINE_TYPE(EM_CSKY,             252,
	`C-SKY processor family')
DEFINE_ELF_MACHINE_TYPE(EM_65816,            257,
	`WDC 65816/65C816')
DEFINE_ELF_MACHINE_TYPE(EM_KF32,             259,
	`ChipON KungFu 32')
')

define(`DEFINE_ELF_MACHINE_TYPE_SYNONYMS',`
DEFINE_ELF_MACHINE_TYPE_SYNONYM(EM_AMD64, EM_X86_64)
DEFINE_ELF_MACHINE_TYPE_SYNONYM(EM_ARC_A5, EM_ARC_COMPACT)
')

#
# ELF file types: (ET_*).
#
define(`DEFINE_ELF_TYPES',`
DEFINE_ELF_TYPE(ET_NONE,   0,
	`No file type')
DEFINE_ELF_TYPE(ET_REL,    1,
	`Relocatable object')
DEFINE_ELF_TYPE(ET_EXEC,   2,
	`Executable')
DEFINE_ELF_TYPE(ET_DYN,    3,
	`Shared object')
DEFINE_ELF_TYPE(ET_CORE,   4,
	`Core file')
DEFINE_ELF_TYPE(ET_LOOS,   0xFE00U,
	`Begin OS-specific range')
DEFINE_ELF_TYPE(ET_HIOS,   0xFEFFU,
	`End OS-specific range')
DEFINE_ELF_TYPE(ET_LOPROC, 0xFF00U,
	`Begin processor-specific range')
DEFINE_ELF_TYPE(ET_HIPROC, 0xFFFFU,
	`End processor-specific range')
')

# ELF file format version numbers.
define(`DEFINE_ELF_FILE_VERSIONS',`
DEFINE_ELF_FILE_VERSION(EV_NONE, 0)
DEFINE_ELF_FILE_VERSION(EV_CURRENT, 1)
')

#
# Flags for section groups.
#
define(`DEFINE_GRP_FLAGS',`
DEFINE_GRP_FLAG(GRP_COMDAT, 	0x1,
	`COMDAT semantics')
DEFINE_GRP_FLAG(GRP_MASKOS,	0x0ff00000,
	`OS-specific flags')
DEFINE_GRP_FLAG(GRP_MASKPROC, 	0xf0000000,
	`processor-specific flags')
')

#
# Flags / mask for .gnu.versym sections.
#
define(`DEFINE_VERSYMS',`
DEFINE_VERSYM(VERSYM_VERSION,	0x7fff)
DEFINE_VERSYM(VERSYM_HIDDEN,	0x8000)
')

#
# Flags used by program header table entries.
#
define(`DEFINE_PHDR_FLAGS',`
DEFINE_PHDR_FLAG(PF_X,                0x1,
	`Execute')
DEFINE_PHDR_FLAG(PF_W,                0x2,
	`Write')
DEFINE_PHDR_FLAG(PF_R,                0x4,
	`Read')
DEFINE_PHDR_FLAG(PF_MASKOS,           0x0ff00000,
	`OS-specific flags')
DEFINE_PHDR_FLAG(PF_MASKPROC,         0xf0000000,
	`Processor-specific flags')
DEFINE_PHDR_FLAG(PF_ARM_SB,           0x10000000,
	`segment contains the location addressed by the static base')
DEFINE_PHDR_FLAG(PF_ARM_PI,           0x20000000,
	`segment is position-independent')
DEFINE_PHDR_FLAG(PF_ARM_ABS,          0x40000000,
	`segment must be loaded at its base address')
')

#
# Types of program header table entries.
#
define(`DEFINE_PHDR_TYPES',`
DEFINE_PHDR_TYPE(PT_NULL,             0UL,
	`ignored entry')
DEFINE_PHDR_TYPE(PT_LOAD,             1UL,
	`loadable segment')
DEFINE_PHDR_TYPE(PT_DYNAMIC,          2UL,
	`contains dynamic linking information')
DEFINE_PHDR_TYPE(PT_INTERP,           3UL,
	`names an interpreter')
DEFINE_PHDR_TYPE(PT_NOTE,             4UL,
	`auxiliary information')
DEFINE_PHDR_TYPE(PT_SHLIB,            5UL,
	`reserved')
DEFINE_PHDR_TYPE(PT_PHDR,             6UL,
	`describes the program header itself')
DEFINE_PHDR_TYPE(PT_TLS,              7UL,
	`thread local storage')
DEFINE_PHDR_TYPE(PT_LOOS,             0x60000000UL,
	`start of OS-specific range')
DEFINE_PHDR_TYPE(PT_SUNW_UNWIND,      0x6464E550UL,
	`Solaris/amd64 stack unwind tables')
DEFINE_PHDR_TYPE(PT_GNU_EH_FRAME,     0x6474E550UL,
	`GCC generated .eh_frame_hdr segment')
DEFINE_PHDR_TYPE(PT_GNU_STACK,	    0x6474E551UL,
	`Stack flags')
DEFINE_PHDR_TYPE(PT_GNU_RELRO,	    0x6474E552UL,
	`Segment becomes read-only after relocation')
DEFINE_PHDR_TYPE(PT_OPENBSD_RANDOMIZE,0x65A3DBE6UL,
	`Segment filled with random data')
DEFINE_PHDR_TYPE(PT_OPENBSD_WXNEEDED, 0x65A3DBE7UL,
	`Program violates W^X')
DEFINE_PHDR_TYPE(PT_OPENBSD_BOOTDATA, 0x65A41BE6UL,
	`Boot data')
DEFINE_PHDR_TYPE(PT_SUNWBSS,          0x6FFFFFFAUL,
	`A Solaris .SUNW_bss section')
DEFINE_PHDR_TYPE(PT_SUNWSTACK,        0x6FFFFFFBUL,
	`A Solaris process stack')
DEFINE_PHDR_TYPE(PT_SUNWDTRACE,       0x6FFFFFFCUL,
	`Used by dtrace(1)')
DEFINE_PHDR_TYPE(PT_SUNWCAP,          0x6FFFFFFDUL,
	`Special hardware capability requirements')
DEFINE_PHDR_TYPE(PT_HIOS,             0x6FFFFFFFUL,
	`end of OS-specific range')
DEFINE_PHDR_TYPE(PT_LOPROC,           0x70000000UL,
	`start of processor-specific range')
DEFINE_PHDR_TYPE(PT_ARM_ARCHEXT,      0x70000000UL,
	`platform architecture compatibility information')
DEFINE_PHDR_TYPE(PT_ARM_EXIDX,        0x70000001UL,
	`exception unwind tables')
DEFINE_PHDR_TYPE(PT_MIPS_REGINFO,     0x70000000UL,
	`register usage information')
DEFINE_PHDR_TYPE(PT_MIPS_RTPROC,      0x70000001UL,
	`runtime procedure table')
DEFINE_PHDR_TYPE(PT_MIPS_OPTIONS,     0x70000002UL,
	`options segment')
DEFINE_PHDR_TYPE(PT_HIPROC,           0x7FFFFFFFUL,
	`end of processor-specific range')
')

define(`DEFINE_PHDR_TYPE_SYNONYMS',`
DEFINE_PHDR_TYPE_SYNONYM(PT_ARM_UNWIND,	PT_ARM_EXIDX)
DEFINE_PHDR_TYPE_SYNONYM(PT_HISUNW,	PT_HIOS)
DEFINE_PHDR_TYPE_SYNONYM(PT_LOSUNW,	PT_SUNWBSS)
')

#
# Section flags.
#
define(`DEFINE_SECTION_FLAGS',`
DEFINE_SECTION_FLAG(SHF_WRITE,           0x1,
	`writable during program execution')
DEFINE_SECTION_FLAG(SHF_ALLOC,           0x2,
	`occupies memory during program execution')
DEFINE_SECTION_FLAG(SHF_EXECINSTR,       0x4,
	`executable instructions')
DEFINE_SECTION_FLAG(SHF_MERGE,           0x10,
	`may be merged to prevent duplication')
DEFINE_SECTION_FLAG(SHF_STRINGS,         0x20,
	`NUL-terminated character strings')
DEFINE_SECTION_FLAG(SHF_INFO_LINK,       0x40,
	`the sh_info field holds a link')
DEFINE_SECTION_FLAG(SHF_LINK_ORDER,      0x80,
	`special ordering requirements during linking')
DEFINE_SECTION_FLAG(SHF_OS_NONCONFORMING, 0x100,
	`requires OS-specific processing during linking')
DEFINE_SECTION_FLAG(SHF_GROUP,           0x200,
	`member of a section group')
DEFINE_SECTION_FLAG(SHF_TLS,             0x400,
	`holds thread-local storage')
DEFINE_SECTION_FLAG(SHF_COMPRESSED,      0x800,
	`holds compressed data')
DEFINE_SECTION_FLAG(SHF_MASKOS,          0x0FF00000UL,
	`bits reserved for OS-specific semantics')
DEFINE_SECTION_FLAG(SHF_AMD64_LARGE,     0x10000000UL,
	`section uses large code model')
DEFINE_SECTION_FLAG(SHF_ENTRYSECT,       0x10000000UL,
	`section contains an entry point (ARM)')
DEFINE_SECTION_FLAG(SHF_COMDEF,          0x80000000UL,
	`section may be multiply defined in input to link step (ARM)')
DEFINE_SECTION_FLAG(SHF_MIPS_GPREL,      0x10000000UL,
	`section must be part of global data area')
DEFINE_SECTION_FLAG(SHF_MIPS_MERGE,      0x20000000UL,
	`section data should be merged to eliminate duplication')
DEFINE_SECTION_FLAG(SHF_MIPS_ADDR,       0x40000000UL,
	`section data is addressed by default')
DEFINE_SECTION_FLAG(SHF_MIPS_STRING,     0x80000000UL,
	`section data is string data by default')
DEFINE_SECTION_FLAG(SHF_MIPS_NOSTRIP,    0x08000000UL,
	`section data may not be stripped')
DEFINE_SECTION_FLAG(SHF_MIPS_LOCAL,      0x04000000UL,
	`section data local to process')
DEFINE_SECTION_FLAG(SHF_MIPS_NAMES,      0x02000000UL,
	`linker must generate implicit hidden weak names')
DEFINE_SECTION_FLAG(SHF_MIPS_NODUPE,     0x01000000UL,
	`linker must retain only one copy')
DEFINE_SECTION_FLAG(SHF_ORDERED,         0x40000000UL,
	`section is ordered with respect to other sections')
DEFINE_SECTION_FLAG(SHF_EXCLUDE,	     0x80000000UL,
	`section is excluded from executables and shared objects')
DEFINE_SECTION_FLAG(SHF_MASKPROC,        0xF0000000UL,
	`bits reserved for processor-specific semantics')
')

#
# Special section indices.
#
define(`DEFINE_SECTION_INDICES',`
DEFINE_SECTION_INDEX(SHN_UNDEF, 	0,
	 `undefined section')
DEFINE_SECTION_INDEX(SHN_LORESERVE, 	0xFF00U,
	`start of reserved area')
DEFINE_SECTION_INDEX(SHN_LOPROC, 	0xFF00U,
	`start of processor-specific range')
DEFINE_SECTION_INDEX(SHN_BEFORE,	0xFF00U,
	`used for section ordering')
DEFINE_SECTION_INDEX(SHN_AFTER,	0xFF01U,
	`used for section ordering')
DEFINE_SECTION_INDEX(SHN_AMD64_LCOMMON, 0xFF02U,
	`large common block label')
DEFINE_SECTION_INDEX(SHN_MIPS_ACOMMON, 0xFF00U,
	`allocated common symbols in a DSO')
DEFINE_SECTION_INDEX(SHN_MIPS_TEXT,	0xFF01U,
	`Reserved (obsolete)')
DEFINE_SECTION_INDEX(SHN_MIPS_DATA,	0xFF02U,
	`Reserved (obsolete)')
DEFINE_SECTION_INDEX(SHN_MIPS_SCOMMON, 0xFF03U,
	`gp-addressable common symbols')
DEFINE_SECTION_INDEX(SHN_MIPS_SUNDEFINED, 0xFF04U,
	`gp-addressable undefined symbols')
DEFINE_SECTION_INDEX(SHN_MIPS_LCOMMON, 0xFF05U,
	`local common symbols')
DEFINE_SECTION_INDEX(SHN_MIPS_LUNDEFINED, 0xFF06U,
	`local undefined symbols')
DEFINE_SECTION_INDEX(SHN_HIPROC, 	0xFF1FU,
	`end of processor-specific range')
DEFINE_SECTION_INDEX(SHN_LOOS, 	0xFF20U,
	`start of OS-specific range')
DEFINE_SECTION_INDEX(SHN_SUNW_IGNORE, 0xFF3FU,
	`used by dtrace')
DEFINE_SECTION_INDEX(SHN_HIOS, 	0xFF3FU,
	`end of OS-specific range')
DEFINE_SECTION_INDEX(SHN_ABS, 	0xFFF1U,
	`absolute references')
DEFINE_SECTION_INDEX(SHN_COMMON, 	0xFFF2U,
	`references to COMMON areas')
DEFINE_SECTION_INDEX(SHN_XINDEX, 	0xFFFFU,
	`extended index')
DEFINE_SECTION_INDEX(SHN_HIRESERVE, 	0xFFFFU,
	`end of reserved area')
')

#
# Section types.
#
define(`DEFINE_SECTION_TYPES',`
DEFINE_SECTION_TYPE(SHT_NULL,            0, `inactive header')
DEFINE_SECTION_TYPE(SHT_PROGBITS,        1, `program defined information')
DEFINE_SECTION_TYPE(SHT_SYMTAB,          2, `symbol table')
DEFINE_SECTION_TYPE(SHT_STRTAB,          3, `string table')
DEFINE_SECTION_TYPE(SHT_RELA,            4,
	`relocation entries with addends')
DEFINE_SECTION_TYPE(SHT_HASH,            5, `symbol hash table')
DEFINE_SECTION_TYPE(SHT_DYNAMIC,         6,
	`information for dynamic linking')
DEFINE_SECTION_TYPE(SHT_NOTE,            7, `additional notes')
DEFINE_SECTION_TYPE(SHT_NOBITS,          8, `section occupying no space')
DEFINE_SECTION_TYPE(SHT_REL,             9,
	`relocation entries without addends')
DEFINE_SECTION_TYPE(SHT_SHLIB,           10, `reserved')
DEFINE_SECTION_TYPE(SHT_DYNSYM,          11, `symbol table')
DEFINE_SECTION_TYPE(SHT_INIT_ARRAY,      14,
	`pointers to initialization functions')
DEFINE_SECTION_TYPE(SHT_FINI_ARRAY,      15,
	`pointers to termination functions')
DEFINE_SECTION_TYPE(SHT_PREINIT_ARRAY,   16,
	`pointers to functions called before initialization')
DEFINE_SECTION_TYPE(SHT_GROUP,           17, `defines a section group')
DEFINE_SECTION_TYPE(SHT_SYMTAB_SHNDX,    18,
	`used for extended section numbering')
DEFINE_SECTION_TYPE(SHT_LOOS,            0x60000000UL,
	`start of OS-specific range')
DEFINE_SECTION_TYPE(SHT_SUNW_dof,	     0x6FFFFFF4UL,
	`used by dtrace')
DEFINE_SECTION_TYPE(SHT_SUNW_cap,	     0x6FFFFFF5UL,
	`capability requirements')
DEFINE_SECTION_TYPE(SHT_GNU_ATTRIBUTES,  0x6FFFFFF5UL,
	`object attributes')
DEFINE_SECTION_TYPE(SHT_SUNW_SIGNATURE,  0x6FFFFFF6UL,
	`module verification signature')
DEFINE_SECTION_TYPE(SHT_GNU_HASH,	     0x6FFFFFF6UL,
	`GNU Hash sections')
DEFINE_SECTION_TYPE(SHT_GNU_LIBLIST,     0x6FFFFFF7UL,
	`List of libraries to be prelinked')
DEFINE_SECTION_TYPE(SHT_SUNW_ANNOTATE,   0x6FFFFFF7UL,
	`special section where unresolved references are allowed')
DEFINE_SECTION_TYPE(SHT_SUNW_DEBUGSTR,   0x6FFFFFF8UL,
	`debugging information')
DEFINE_SECTION_TYPE(SHT_CHECKSUM, 	     0x6FFFFFF8UL,
	`checksum for dynamic shared objects')
DEFINE_SECTION_TYPE(SHT_SUNW_DEBUG,      0x6FFFFFF9UL,
	`debugging information')
DEFINE_SECTION_TYPE(SHT_SUNW_move,       0x6FFFFFFAUL,
	`information to handle partially initialized symbols')
DEFINE_SECTION_TYPE(SHT_SUNW_COMDAT,     0x6FFFFFFBUL,
	`section supporting merging of multiple copies of data')
DEFINE_SECTION_TYPE(SHT_SUNW_syminfo,    0x6FFFFFFCUL,
	`additional symbol information')
DEFINE_SECTION_TYPE(SHT_SUNW_verdef,     0x6FFFFFFDUL,
	`symbol versioning information')
DEFINE_SECTION_TYPE(SHT_SUNW_verneed,    0x6FFFFFFEUL,
	`symbol versioning requirements')
DEFINE_SECTION_TYPE(SHT_SUNW_versym,     0x6FFFFFFFUL,
	`symbol versioning table')
DEFINE_SECTION_TYPE(SHT_HIOS,            0x6FFFFFFFUL,
	`end of OS-specific range')
DEFINE_SECTION_TYPE(SHT_LOPROC,          0x70000000UL,
	`start of processor-specific range')
DEFINE_SECTION_TYPE(SHT_ARM_EXIDX,       0x70000001UL,
	`exception index table')
DEFINE_SECTION_TYPE(SHT_ARM_PREEMPTMAP,  0x70000002UL,
	`BPABI DLL dynamic linking preemption map')
DEFINE_SECTION_TYPE(SHT_ARM_ATTRIBUTES,  0x70000003UL,
	`object file compatibility attributes')
DEFINE_SECTION_TYPE(SHT_ARM_DEBUGOVERLAY, 0x70000004UL,
	`overlay debug information')
DEFINE_SECTION_TYPE(SHT_ARM_OVERLAYSECTION, 0x70000005UL,
	`overlay debug information')
DEFINE_SECTION_TYPE(SHT_MIPS_LIBLIST,    0x70000000UL,
	`DSO library information used in link')
DEFINE_SECTION_TYPE(SHT_MIPS_MSYM,       0x70000001UL,
	`MIPS symbol table extension')
DEFINE_SECTION_TYPE(SHT_MIPS_CONFLICT,   0x70000002UL,
	`symbol conflicting with DSO-defined symbols ')
DEFINE_SECTION_TYPE(SHT_MIPS_GPTAB,      0x70000003UL,
	`global pointer table')
DEFINE_SECTION_TYPE(SHT_MIPS_UCODE,      0x70000004UL,
	`reserved')
DEFINE_SECTION_TYPE(SHT_MIPS_DEBUG,      0x70000005UL,
	`reserved (obsolete debug information)')
DEFINE_SECTION_TYPE(SHT_MIPS_REGINFO,    0x70000006UL,
	`register usage information')
DEFINE_SECTION_TYPE(SHT_MIPS_PACKAGE,    0x70000007UL,
	`OSF reserved')
DEFINE_SECTION_TYPE(SHT_MIPS_PACKSYM,    0x70000008UL,
	`OSF reserved')
DEFINE_SECTION_TYPE(SHT_MIPS_RELD,       0x70000009UL,
	`dynamic relocation')
DEFINE_SECTION_TYPE(SHT_MIPS_IFACE,      0x7000000BUL,
	`subprogram interface information')
DEFINE_SECTION_TYPE(SHT_MIPS_CONTENT,    0x7000000CUL,
	`section content classification')
DEFINE_SECTION_TYPE(SHT_MIPS_OPTIONS,     0x7000000DUL,
	`general options')
DEFINE_SECTION_TYPE(SHT_MIPS_DELTASYM,   0x7000001BUL,
	`Delta C++: symbol table')
DEFINE_SECTION_TYPE(SHT_MIPS_DELTAINST,  0x7000001CUL,
	`Delta C++: instance table')
DEFINE_SECTION_TYPE(SHT_MIPS_DELTACLASS, 0x7000001DUL,
	`Delta C++: class table')
DEFINE_SECTION_TYPE(SHT_MIPS_DWARF,      0x7000001EUL,
	`DWARF debug information')
DEFINE_SECTION_TYPE(SHT_MIPS_DELTADECL,  0x7000001FUL,
	`Delta C++: declarations')
DEFINE_SECTION_TYPE(SHT_MIPS_SYMBOL_LIB, 0x70000020UL,
	`symbol-to-library mapping')
DEFINE_SECTION_TYPE(SHT_MIPS_EVENTS,     0x70000021UL,
	`event locations')
DEFINE_SECTION_TYPE(SHT_MIPS_TRANSLATE,  0x70000022UL,
	`???')
DEFINE_SECTION_TYPE(SHT_MIPS_PIXIE,      0x70000023UL,
	`special pixie sections')
DEFINE_SECTION_TYPE(SHT_MIPS_XLATE,      0x70000024UL,
	`address translation table')
DEFINE_SECTION_TYPE(SHT_MIPS_XLATE_DEBUG, 0x70000025UL,
	`SGI internal address translation table')
DEFINE_SECTION_TYPE(SHT_MIPS_WHIRL,      0x70000026UL,
	`intermediate code')
DEFINE_SECTION_TYPE(SHT_MIPS_EH_REGION,  0x70000027UL,
	`C++ exception handling region info')
DEFINE_SECTION_TYPE(SHT_MIPS_XLATE_OLD,  0x70000028UL,
	`obsolete')
DEFINE_SECTION_TYPE(SHT_MIPS_PDR_EXCEPTION, 0x70000029UL,
	`runtime procedure descriptor table exception information')
DEFINE_SECTION_TYPE(SHT_MIPS_ABIFLAGS,   0x7000002AUL,
	`ABI flags')
DEFINE_SECTION_TYPE(SHT_SPARC_GOTDATA,   0x70000000UL,
	`SPARC-specific data')
DEFINE_SECTION_TYPE(SHT_X86_64_UNWIND,   0x70000001UL,
	`unwind tables for the AMD64')
DEFINE_SECTION_TYPE(SHT_ORDERED,         0x7FFFFFFFUL,
	`sort entries in the section')
DEFINE_SECTION_TYPE(SHT_HIPROC,          0x7FFFFFFFUL,
	`end of processor-specific range')
DEFINE_SECTION_TYPE(SHT_LOUSER,          0x80000000UL,
	`start of application-specific range')
DEFINE_SECTION_TYPE(SHT_HIUSER,          0xFFFFFFFFUL,
	`end of application-specific range')
')

# Aliases for section types.
define(`DEFINE_SECTION_TYPE_ALIASES',`
DEFINE_SECTION_TYPE_ALIAS(SHT_AMD64_UNWIND,	SHT_X86_64_UNWIND)
DEFINE_SECTION_TYPE_ALIAS(SHT_GNU_verdef,	SHT_SUNW_verdef)
DEFINE_SECTION_TYPE_ALIAS(SHT_GNU_verneed,	SHT_SUNW_verneed)
DEFINE_SECTION_TYPE_ALIAS(SHT_GNU_versym,	SHT_SUNW_versym)
')

#
# Symbol binding information.
#
define(`DEFINE_SYMBOL_BINDINGS',`
DEFINE_SYMBOL_BINDING(STB_LOCAL,           0,
	`not visible outside defining object file')
DEFINE_SYMBOL_BINDING(STB_GLOBAL,          1,
	`visible across all object files being combined')
DEFINE_SYMBOL_BINDING(STB_WEAK,            2,
	`visible across all object files but with low precedence')
DEFINE_SYMBOL_BINDING(STB_LOOS,            10,
	`start of OS-specific range')
DEFINE_SYMBOL_BINDING(STB_GNU_UNIQUE,      10,
	`unique symbol (GNU)')
DEFINE_SYMBOL_BINDING(STB_HIOS,            12,
	`end of OS-specific range')
DEFINE_SYMBOL_BINDING(STB_LOPROC,          13,
	`start of processor-specific range')
DEFINE_SYMBOL_BINDING(STB_HIPROC,          15,
	`end of processor-specific range')
')

#
# Symbol types
#
define(`DEFINE_SYMBOL_TYPES',`
DEFINE_SYMBOL_TYPE(STT_NOTYPE,          0,
	`unspecified type')
DEFINE_SYMBOL_TYPE(STT_OBJECT,          1,
	`data object')
DEFINE_SYMBOL_TYPE(STT_FUNC,            2,
	`executable code')
DEFINE_SYMBOL_TYPE(STT_SECTION,         3,
	`section')
DEFINE_SYMBOL_TYPE(STT_FILE,            4,
	`source file')
DEFINE_SYMBOL_TYPE(STT_COMMON,          5,
	`uninitialized common block')
DEFINE_SYMBOL_TYPE(STT_TLS,             6,
	`thread local storage')
DEFINE_SYMBOL_TYPE(STT_LOOS,            10,
	`start of OS-specific types')
DEFINE_SYMBOL_TYPE(STT_GNU_IFUNC,       10,
	`indirect function')
DEFINE_SYMBOL_TYPE(STT_HIOS,            12,
	`end of OS-specific types')
DEFINE_SYMBOL_TYPE(STT_LOPROC,          13,
	`start of processor-specific types')
DEFINE_SYMBOL_TYPE(STT_ARM_TFUNC,       13,
	`Thumb function (GNU)')
DEFINE_SYMBOL_TYPE(STT_ARM_16BIT,       15,
	`Thumb label (GNU)')
DEFINE_SYMBOL_TYPE(STT_SPARC_REGISTER,  13,
	`SPARC register information')
DEFINE_SYMBOL_TYPE(STT_HIPROC,          15,
	`end of processor-specific types')
')

#
# Symbol binding.
#
define(`DEFINE_SYMBOL_BINDING_KINDS',`
DEFINE_SYMBOL_BINDING_KIND(SYMINFO_BT_SELF,	0xFFFFU,
	`bound to self')
DEFINE_SYMBOL_BINDING_KIND(SYMINFO_BT_PARENT,	0xFFFEU,
	`bound to parent')
DEFINE_SYMBOL_BINDING_KIND(SYMINFO_BT_NONE,	0xFFFDU,
	`no special binding')
')

#
# Symbol visibility.
#
define(`DEFINE_SYMBOL_VISIBILITIES',`
DEFINE_SYMBOL_VISIBILITY(STV_DEFAULT,         0,
	`as specified by symbol type')
DEFINE_SYMBOL_VISIBILITY(STV_INTERNAL,        1,
	`as defined by processor semantics')
DEFINE_SYMBOL_VISIBILITY(STV_HIDDEN,          2,
	`hidden from other components')
DEFINE_SYMBOL_VISIBILITY(STV_PROTECTED,       3,
	`local references are not preemptable')
')

#
# Symbol flags.
#
define(`DEFINE_SYMBOL_FLAGS',`
DEFINE_SYMBOL_FLAG(SYMINFO_FLG_DIRECT,	0x01,
	`directly assocated reference')
DEFINE_SYMBOL_FLAG(SYMINFO_FLG_COPY,	0x04,
	`definition by copy-relocation')
DEFINE_SYMBOL_FLAG(SYMINFO_FLG_LAZYLOAD,	0x08,
	`object should be lazily loaded')
DEFINE_SYMBOL_FLAG(SYMINFO_FLG_DIRECTBIND,	0x10,
	`reference should be directly bound')
DEFINE_SYMBOL_FLAG(SYMINFO_FLG_NOEXTDIRECT, 0x20,
	`external references not allowed to bind to definition')
')

#
# Version dependencies.
#
define(`DEFINE_VERSIONING_DEPENDENCIES',`
DEFINE_VERSIONING_DEPENDENCY(VER_NDX_LOCAL,	0,
	`local scope')
DEFINE_VERSIONING_DEPENDENCY(VER_NDX_GLOBAL,	1,
	`global scope')
')

#
# Version flags.
#
define(`DEFINE_VERSIONING_FLAGS',`
DEFINE_VERSIONING_FLAG(VER_FLG_BASE,		0x1,
	`file version')
DEFINE_VERSIONING_FLAG(VER_FLG_WEAK,		0x2,
	`weak version')
')

#
# Version needs
#
define(`DEFINE_VERSIONING_NEEDS',`
DEFINE_VERSIONING_NEED(VER_NEED_NONE,		0,
	`invalid version')
DEFINE_VERSIONING_NEED(VER_NEED_CURRENT,	1,
	`current version')
')

#
# Versioning numbers.
#
define(`DEFINE_VERSIONING_NUMBERS',`
DEFINE_VERSIONING_NUMBER(VER_DEF_NONE,		0,
	`invalid version')
DEFINE_VERSIONING_NUMBER(VER_DEF_CURRENT,	1, 
	`current version')
')

#
# Relocation types.
#
define(`DEFINE_386_RELOCATIONS',`
DEFINE_RELOCATION(R_386_NONE,		0)
DEFINE_RELOCATION(R_386_32,		1)
DEFINE_RELOCATION(R_386_PC32,		2)
DEFINE_RELOCATION(R_386_GOT32,		3)
DEFINE_RELOCATION(R_386_PLT32,		4)
DEFINE_RELOCATION(R_386_COPY,		5)
DEFINE_RELOCATION(R_386_GLOB_DAT,	6)
DEFINE_RELOCATION(R_386_JUMP_SLOT,	7)
DEFINE_RELOCATION(R_386_RELATIVE,	8)
DEFINE_RELOCATION(R_386_GOTOFF,		9)
DEFINE_RELOCATION(R_386_GOTPC,		10)
DEFINE_RELOCATION(R_386_32PLT,		11)
DEFINE_RELOCATION(R_386_TLS_TPOFF,	14)
DEFINE_RELOCATION(R_386_TLS_IE,		15)
DEFINE_RELOCATION(R_386_TLS_GOTIE,	16)
DEFINE_RELOCATION(R_386_TLS_LE,		17)
DEFINE_RELOCATION(R_386_TLS_GD,		18)
DEFINE_RELOCATION(R_386_TLS_LDM,	19)
DEFINE_RELOCATION(R_386_16,		20)
DEFINE_RELOCATION(R_386_PC16,		21)
DEFINE_RELOCATION(R_386_8,		22)
DEFINE_RELOCATION(R_386_PC8,		23)
DEFINE_RELOCATION(R_386_TLS_GD_32,	24)
DEFINE_RELOCATION(R_386_TLS_GD_PUSH,	25)
DEFINE_RELOCATION(R_386_TLS_GD_CALL,	26)
DEFINE_RELOCATION(R_386_TLS_GD_POP,	27)
DEFINE_RELOCATION(R_386_TLS_LDM_32,	28)
DEFINE_RELOCATION(R_386_TLS_LDM_PUSH,	29)
DEFINE_RELOCATION(R_386_TLS_LDM_CALL,	30)
DEFINE_RELOCATION(R_386_TLS_LDM_POP,	31)
DEFINE_RELOCATION(R_386_TLS_LDO_32,	32)
DEFINE_RELOCATION(R_386_TLS_IE_32,	33)
DEFINE_RELOCATION(R_386_TLS_LE_32,	34)
DEFINE_RELOCATION(R_386_TLS_DTPMOD32,	35)
DEFINE_RELOCATION(R_386_TLS_DTPOFF32,	36)
DEFINE_RELOCATION(R_386_TLS_TPOFF32,	37)
DEFINE_RELOCATION(R_386_SIZE32,		38)
DEFINE_RELOCATION(R_386_TLS_GOTDESC,	39)
DEFINE_RELOCATION(R_386_TLS_DESC_CALL,	40)
DEFINE_RELOCATION(R_386_TLS_DESC,	41)
DEFINE_RELOCATION(R_386_IRELATIVE,	42)
DEFINE_RELOCATION(R_386_GOT32X,		43)
')

define(`DEFINE_AARCH64_RELOCATIONS',`
DEFINE_RELOCATION(R_AARCH64_NONE,				0)
DEFINE_RELOCATION(R_AARCH64_ABS64,				257)
DEFINE_RELOCATION(R_AARCH64_ABS32,				258)
DEFINE_RELOCATION(R_AARCH64_ABS16,				259)
DEFINE_RELOCATION(R_AARCH64_PREL64,				260)
DEFINE_RELOCATION(R_AARCH64_PREL32,				261)
DEFINE_RELOCATION(R_AARCH64_PREL16,				262)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G0,			263)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G0_NC,			264)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G1,			265)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G1_NC,			266)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G2,			267)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G2_NC,			268)
DEFINE_RELOCATION(R_AARCH64_MOVW_UABS_G3,			269)
DEFINE_RELOCATION(R_AARCH64_MOVW_SABS_G0,			270)
DEFINE_RELOCATION(R_AARCH64_MOVW_SABS_G1,			271)
DEFINE_RELOCATION(R_AARCH64_MOVW_SABS_G2,			272)
DEFINE_RELOCATION(R_AARCH64_LD_PREL_LO19,			273)
DEFINE_RELOCATION(R_AARCH64_ADR_PREL_LO21,			274)
DEFINE_RELOCATION(R_AARCH64_ADR_PREL_PG_HI21,			275)
DEFINE_RELOCATION(R_AARCH64_ADR_PREL_PG_HI21_NC,		276)
DEFINE_RELOCATION(R_AARCH64_ADD_ABS_LO12_NC,			277)
DEFINE_RELOCATION(R_AARCH64_LDST8_ABS_LO12_NC,			278)
DEFINE_RELOCATION(R_AARCH64_TSTBR14,				279)
DEFINE_RELOCATION(R_AARCH64_CONDBR19,				280)
DEFINE_RELOCATION(R_AARCH64_JUMP26,				282)
DEFINE_RELOCATION(R_AARCH64_CALL26,				283)
DEFINE_RELOCATION(R_AARCH64_LDST16_ABS_LO12_NC,			284)
DEFINE_RELOCATION(R_AARCH64_LDST32_ABS_LO12_NC,			285)
DEFINE_RELOCATION(R_AARCH64_LDST64_ABS_LO12_NC,			286)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G0,			287)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G0_NC,			288)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G1,			289)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G1_NC,			290)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G2,			291)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G2_NC,			292)
DEFINE_RELOCATION(R_AARCH64_MOVW_PREL_G3,			293)
DEFINE_RELOCATION(R_AARCH64_LDST128_ABS_LO12_NC,		299)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G0,			300)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G0_NC,			301)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G1,			302)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G1_NC,			303)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G2,			304)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G2_NC,			305)
DEFINE_RELOCATION(R_AARCH64_MOVW_GOTOFF_G3,			306)
DEFINE_RELOCATION(R_AARCH64_GOTREL64,				307)
DEFINE_RELOCATION(R_AARCH64_GOTREL32,				308)
DEFINE_RELOCATION(R_AARCH64_GOT_LD_PREL19,			309)
DEFINE_RELOCATION(R_AARCH64_LD64_GOTOFF_LO15,			310)
DEFINE_RELOCATION(R_AARCH64_ADR_GOT_PAGE,			311)
DEFINE_RELOCATION(R_AARCH64_LD64_GOT_LO12_NC,			312)
DEFINE_RELOCATION(R_AARCH64_LD64_GOTPAGE_LO15,			313)
DEFINE_RELOCATION(R_AARCH64_TLSGD_ADR_PREL21,			512)
DEFINE_RELOCATION(R_AARCH64_TLSGD_ADR_PAGE21,			513)
DEFINE_RELOCATION(R_AARCH64_TLSGD_ADD_LO12_NC,			514)
DEFINE_RELOCATION(R_AARCH64_TLSGD_MOVW_G1,			515)
DEFINE_RELOCATION(R_AARCH64_TLSGD_MOVW_G0_NC,			516)
DEFINE_RELOCATION(R_AARCH64_TLSLD_ADR_PREL21,			517)
DEFINE_RELOCATION(R_AARCH64_TLSLD_ADR_PAGE21,			518)
DEFINE_RELOCATION(R_AARCH64_TLSLD_ADD_LO12_NC,			519)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_G1,			520)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_G0_NC,			521)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LD_PREL19,			522)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_DTPREL_G2,		523)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_DTPREL_G1,		524)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC,		525)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_DTPREL_G0,		526)
DEFINE_RELOCATION(R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC,		527)
DEFINE_RELOCATION(R_AARCH64_TLSLD_ADD_DTPREL_HI12,		529)
DEFINE_RELOCATION(R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC,		530)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST8_DTPREL_LO12,		531)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC,		532)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST16_DTPREL_LO12,		533)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC,	534)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST32_DTPREL_LO12,		535)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC,	536)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST64_DTPREL_LO12,		537)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC,	538)
DEFINE_RELOCATION(R_AARCH64_TLSIE_MOVW_GOTTPREL_G1,		539)
DEFINE_RELOCATION(R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC,		540)
DEFINE_RELOCATION(R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21,		541)
DEFINE_RELOCATION(R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC,	542)
DEFINE_RELOCATION(R_AARCH64_TLSIE_LD_GOTTPREL_PREL19,		543)
DEFINE_RELOCATION(R_AARCH64_TLSLE_MOVW_TPREL_G2,		544)
DEFINE_RELOCATION(R_AARCH64_TLSLE_MOVW_TPREL_G1,		545)
DEFINE_RELOCATION(R_AARCH64_TLSLE_MOVW_TPREL_G1_NC,		546)
DEFINE_RELOCATION(R_AARCH64_TLSLE_MOVW_TPREL_G0,		547)
DEFINE_RELOCATION(R_AARCH64_TLSLE_MOVW_TPREL_G0_NC,		548)
DEFINE_RELOCATION(R_AARCH64_TLSLE_ADD_TPREL_HI12,		549)
DEFINE_RELOCATION(R_AARCH64_TLSLE_ADD_TPREL_LO12,		550)
DEFINE_RELOCATION(R_AARCH64_TLSLE_ADD_TPREL_LO12_NC,		551)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST8_TPREL_LO12,		552)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC,		553)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST16_TPREL_LO12,		554)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC,		555)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST32_TPREL_LO12,		556)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC,		557)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST64_TPREL_LO12,		558)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC,		559)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_LD_PREL19,			560)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_ADR_PREL21,			561)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_ADR_PAGE21,			562)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_LD64_LO12,			563)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_ADD_LO12,			564)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_OFF_G1,			565)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_OFF_G0_NC,			566)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_LDR,			567)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_ADD,			568)
DEFINE_RELOCATION(R_AARCH64_TLSDESC_CALL,			569)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST128_TPREL_LO12,		570)
DEFINE_RELOCATION(R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC,	571)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST128_DTPREL_LO12,		572)
DEFINE_RELOCATION(R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC,	573)
DEFINE_RELOCATION(R_AARCH64_COPY,				1024)
DEFINE_RELOCATION(R_AARCH64_GLOB_DAT,				1025)
DEFINE_RELOCATION(R_AARCH64_JUMP_SLOT,				1026)
DEFINE_RELOCATION(R_AARCH64_RELATIVE,				1027)
DEFINE_RELOCATION(R_AARCH64_TLS_DTPREL64,			1028)
DEFINE_RELOCATION(R_AARCH64_TLS_DTPMOD64,			1029)
DEFINE_RELOCATION(R_AARCH64_TLS_TPREL64,			1030)
DEFINE_RELOCATION(R_AARCH64_TLSDESC,				1031)
DEFINE_RELOCATION(R_AARCH64_IRELATIVE,				1032)
')

#
# These are the symbols used in the Sun ``Linkers and Loaders
# Guide'', Document No: 817-1984-17.  See the X86_64 relocations list
# below for the spellings used in the ELF specification.
#
define(`DEFINE_AMD64_RELOCATIONS',`
DEFINE_RELOCATION(R_AMD64_NONE,		0)
DEFINE_RELOCATION(R_AMD64_64,		1)
DEFINE_RELOCATION(R_AMD64_PC32,		2)
DEFINE_RELOCATION(R_AMD64_GOT32,	3)
DEFINE_RELOCATION(R_AMD64_PLT32,	4)
DEFINE_RELOCATION(R_AMD64_COPY,		5)
DEFINE_RELOCATION(R_AMD64_GLOB_DAT,	6)
DEFINE_RELOCATION(R_AMD64_JUMP_SLOT,	7)
DEFINE_RELOCATION(R_AMD64_RELATIVE,	8)
DEFINE_RELOCATION(R_AMD64_GOTPCREL,	9)
DEFINE_RELOCATION(R_AMD64_32,		10)
DEFINE_RELOCATION(R_AMD64_32S,		11)
DEFINE_RELOCATION(R_AMD64_16,		12)
DEFINE_RELOCATION(R_AMD64_PC16,		13)
DEFINE_RELOCATION(R_AMD64_8,		14)
DEFINE_RELOCATION(R_AMD64_PC8,		15)
DEFINE_RELOCATION(R_AMD64_PC64,		24)
DEFINE_RELOCATION(R_AMD64_GOTOFF64,	25)
DEFINE_RELOCATION(R_AMD64_GOTPC32,	26)
')

#
# Relocation definitions from the ARM ELF ABI, version "ARM IHI
# 0044E" released on 30th November 2012.
#
define(`DEFINE_ARM_RELOCATIONS',`
DEFINE_RELOCATION(R_ARM_NONE,			0)
DEFINE_RELOCATION(R_ARM_PC24,			1)
DEFINE_RELOCATION(R_ARM_ABS32,			2)
DEFINE_RELOCATION(R_ARM_REL32,			3)
DEFINE_RELOCATION(R_ARM_LDR_PC_G0,		4)
DEFINE_RELOCATION(R_ARM_ABS16,			5)
DEFINE_RELOCATION(R_ARM_ABS12,			6)
DEFINE_RELOCATION(R_ARM_THM_ABS5,		7)
DEFINE_RELOCATION(R_ARM_ABS8,			8)
DEFINE_RELOCATION(R_ARM_SBREL32,		9)
DEFINE_RELOCATION(R_ARM_THM_CALL,		10)
DEFINE_RELOCATION(R_ARM_THM_PC8,		11)
DEFINE_RELOCATION(R_ARM_BREL_ADJ,		12)
DEFINE_RELOCATION(R_ARM_SWI24,			13)
DEFINE_RELOCATION(R_ARM_TLS_DESC,		13)
DEFINE_RELOCATION(R_ARM_THM_SWI8,		14)
DEFINE_RELOCATION(R_ARM_XPC25,			15)
DEFINE_RELOCATION(R_ARM_THM_XPC22,		16)
DEFINE_RELOCATION(R_ARM_TLS_DTPMOD32,		17)
DEFINE_RELOCATION(R_ARM_TLS_DTPOFF32,		18)
DEFINE_RELOCATION(R_ARM_TLS_TPOFF32,		19)
DEFINE_RELOCATION(R_ARM_COPY,			20)
DEFINE_RELOCATION(R_ARM_GLOB_DAT,		21)
DEFINE_RELOCATION(R_ARM_JUMP_SLOT,		22)
DEFINE_RELOCATION(R_ARM_RELATIVE,		23)
DEFINE_RELOCATION(R_ARM_GOTOFF32,		24)
DEFINE_RELOCATION(R_ARM_BASE_PREL,		25)
DEFINE_RELOCATION(R_ARM_GOT_BREL,		26)
DEFINE_RELOCATION(R_ARM_PLT32,			27)
DEFINE_RELOCATION(R_ARM_CALL,			28)
DEFINE_RELOCATION(R_ARM_JUMP24,			29)
DEFINE_RELOCATION(R_ARM_THM_JUMP24,		30)
DEFINE_RELOCATION(R_ARM_BASE_ABS,		31)
DEFINE_RELOCATION(R_ARM_ALU_PCREL_7_0,		32)
DEFINE_RELOCATION(R_ARM_ALU_PCREL_15_8,		33)
DEFINE_RELOCATION(R_ARM_ALU_PCREL_23_15,	34)
DEFINE_RELOCATION(R_ARM_LDR_SBREL_11_0_NC,	35)
DEFINE_RELOCATION(R_ARM_ALU_SBREL_19_12_NC,	36)
DEFINE_RELOCATION(R_ARM_ALU_SBREL_27_20_CK,	37)
DEFINE_RELOCATION(R_ARM_TARGET1,		38)
DEFINE_RELOCATION(R_ARM_SBREL31,		39)
DEFINE_RELOCATION(R_ARM_V4BX,			40)
DEFINE_RELOCATION(R_ARM_TARGET2,		41)
DEFINE_RELOCATION(R_ARM_PREL31,			42)
DEFINE_RELOCATION(R_ARM_MOVW_ABS_NC,		43)
DEFINE_RELOCATION(R_ARM_MOVT_ABS,		44)
DEFINE_RELOCATION(R_ARM_MOVW_PREL_NC,		45)
DEFINE_RELOCATION(R_ARM_MOVT_PREL,		46)
DEFINE_RELOCATION(R_ARM_THM_MOVW_ABS_NC,	47)
DEFINE_RELOCATION(R_ARM_THM_MOVT_ABS,		48)
DEFINE_RELOCATION(R_ARM_THM_MOVW_PREL_NC,	49)
DEFINE_RELOCATION(R_ARM_THM_MOVT_PREL,		50)
DEFINE_RELOCATION(R_ARM_THM_JUMP19,		51)
DEFINE_RELOCATION(R_ARM_THM_JUMP6,		52)
DEFINE_RELOCATION(R_ARM_THM_ALU_PREL_11_0,	53)
DEFINE_RELOCATION(R_ARM_THM_PC12,		54)
DEFINE_RELOCATION(R_ARM_ABS32_NOI,		55)
DEFINE_RELOCATION(R_ARM_REL32_NOI,		56)
DEFINE_RELOCATION(R_ARM_ALU_PC_G0_NC,		57)
DEFINE_RELOCATION(R_ARM_ALU_PC_G0,		58)
DEFINE_RELOCATION(R_ARM_ALU_PC_G1_NC,		59)
DEFINE_RELOCATION(R_ARM_ALU_PC_G1,		60)
DEFINE_RELOCATION(R_ARM_ALU_PC_G2,		61)
DEFINE_RELOCATION(R_ARM_LDR_PC_G1,		62)
DEFINE_RELOCATION(R_ARM_LDR_PC_G2,		63)
DEFINE_RELOCATION(R_ARM_LDRS_PC_G0,		64)
DEFINE_RELOCATION(R_ARM_LDRS_PC_G1,		65)
DEFINE_RELOCATION(R_ARM_LDRS_PC_G2,		66)
DEFINE_RELOCATION(R_ARM_LDC_PC_G0,		67)
DEFINE_RELOCATION(R_ARM_LDC_PC_G1,		68)
DEFINE_RELOCATION(R_ARM_LDC_PC_G2,		69)
DEFINE_RELOCATION(R_ARM_ALU_SB_G0_NC,		70)
DEFINE_RELOCATION(R_ARM_ALU_SB_G0,		71)
DEFINE_RELOCATION(R_ARM_ALU_SB_G1_NC,		72)
DEFINE_RELOCATION(R_ARM_ALU_SB_G1,		73)
DEFINE_RELOCATION(R_ARM_ALU_SB_G2,		74)
DEFINE_RELOCATION(R_ARM_LDR_SB_G0,		75)
DEFINE_RELOCATION(R_ARM_LDR_SB_G1,		76)
DEFINE_RELOCATION(R_ARM_LDR_SB_G2,		77)
DEFINE_RELOCATION(R_ARM_LDRS_SB_G0,		78)
DEFINE_RELOCATION(R_ARM_LDRS_SB_G1,		79)
DEFINE_RELOCATION(R_ARM_LDRS_SB_G2,		80)
DEFINE_RELOCATION(R_ARM_LDC_SB_G0,		81)
DEFINE_RELOCATION(R_ARM_LDC_SB_G1,		82)
DEFINE_RELOCATION(R_ARM_LDC_SB_G2,		83)
DEFINE_RELOCATION(R_ARM_MOVW_BREL_NC,		84)
DEFINE_RELOCATION(R_ARM_MOVT_BREL,		85)
DEFINE_RELOCATION(R_ARM_MOVW_BREL,		86)
DEFINE_RELOCATION(R_ARM_THM_MOVW_BREL_NC,	87)
DEFINE_RELOCATION(R_ARM_THM_MOVT_BREL,		88)
DEFINE_RELOCATION(R_ARM_THM_MOVW_BREL,		89)
DEFINE_RELOCATION(R_ARM_TLS_GOTDESC,		90)
DEFINE_RELOCATION(R_ARM_TLS_CALL,		91)
DEFINE_RELOCATION(R_ARM_TLS_DESCSEQ,		92)
DEFINE_RELOCATION(R_ARM_THM_TLS_CALL,		93)
DEFINE_RELOCATION(R_ARM_PLT32_ABS,		94)
DEFINE_RELOCATION(R_ARM_GOT_ABS,		95)
DEFINE_RELOCATION(R_ARM_GOT_PREL,		96)
DEFINE_RELOCATION(R_ARM_GOT_BREL12,		97)
DEFINE_RELOCATION(R_ARM_GOTOFF12,		98)
DEFINE_RELOCATION(R_ARM_GOTRELAX,		99)
DEFINE_RELOCATION(R_ARM_GNU_VTENTRY,		100)
DEFINE_RELOCATION(R_ARM_GNU_VTINHERIT,		101)
DEFINE_RELOCATION(R_ARM_THM_JUMP11,		102)
DEFINE_RELOCATION(R_ARM_THM_JUMP8,		103)
DEFINE_RELOCATION(R_ARM_TLS_GD32,		104)
DEFINE_RELOCATION(R_ARM_TLS_LDM32,		105)
DEFINE_RELOCATION(R_ARM_TLS_LDO32,		106)
DEFINE_RELOCATION(R_ARM_TLS_IE32,		107)
DEFINE_RELOCATION(R_ARM_TLS_LE32,		108)
DEFINE_RELOCATION(R_ARM_TLS_LDO12,		109)
DEFINE_RELOCATION(R_ARM_TLS_LE12,		110)
DEFINE_RELOCATION(R_ARM_TLS_IE12GP,		111)
DEFINE_RELOCATION(R_ARM_PRIVATE_0,		112)
DEFINE_RELOCATION(R_ARM_PRIVATE_1,		113)
DEFINE_RELOCATION(R_ARM_PRIVATE_2,		114)
DEFINE_RELOCATION(R_ARM_PRIVATE_3,		115)
DEFINE_RELOCATION(R_ARM_PRIVATE_4,		116)
DEFINE_RELOCATION(R_ARM_PRIVATE_5,		117)
DEFINE_RELOCATION(R_ARM_PRIVATE_6,		118)
DEFINE_RELOCATION(R_ARM_PRIVATE_7,		119)
DEFINE_RELOCATION(R_ARM_PRIVATE_8,		120)
DEFINE_RELOCATION(R_ARM_PRIVATE_9,		121)
DEFINE_RELOCATION(R_ARM_PRIVATE_10,		122)
DEFINE_RELOCATION(R_ARM_PRIVATE_11,		123)
DEFINE_RELOCATION(R_ARM_PRIVATE_12,		124)
DEFINE_RELOCATION(R_ARM_PRIVATE_13,		125)
DEFINE_RELOCATION(R_ARM_PRIVATE_14,		126)
DEFINE_RELOCATION(R_ARM_PRIVATE_15,		127)
DEFINE_RELOCATION(R_ARM_ME_TOO,			128)
DEFINE_RELOCATION(R_ARM_THM_TLS_DESCSEQ16,	129)
DEFINE_RELOCATION(R_ARM_THM_TLS_DESCSEQ32,	130)
DEFINE_RELOCATION(R_ARM_THM_GOT_BREL12,		131)
DEFINE_RELOCATION(R_ARM_IRELATIVE,		140)
')

define(`DEFINE_IA64_RELOCATIONS',`
DEFINE_RELOCATION(R_IA_64_NONE,			0)
DEFINE_RELOCATION(R_IA_64_IMM14,		0x21)
DEFINE_RELOCATION(R_IA_64_IMM22,		0x22)
DEFINE_RELOCATION(R_IA_64_IMM64,		0x23)
DEFINE_RELOCATION(R_IA_64_DIR32MSB,		0x24)
DEFINE_RELOCATION(R_IA_64_DIR32LSB,		0x25)
DEFINE_RELOCATION(R_IA_64_DIR64MSB,		0x26)
DEFINE_RELOCATION(R_IA_64_DIR64LSB,		0x27)
DEFINE_RELOCATION(R_IA_64_GPREL22,		0x2a)
DEFINE_RELOCATION(R_IA_64_GPREL64I,		0x2b)
DEFINE_RELOCATION(R_IA_64_GPREL32MSB,		0x2c)
DEFINE_RELOCATION(R_IA_64_GPREL32LSB,		0x2d)
DEFINE_RELOCATION(R_IA_64_GPREL64MSB,		0x2e)
DEFINE_RELOCATION(R_IA_64_GPREL64LSB,		0x2f)
DEFINE_RELOCATION(R_IA_64_LTOFF22,		0x32)
DEFINE_RELOCATION(R_IA_64_LTOFF64I,		0x33)
DEFINE_RELOCATION(R_IA_64_PLTOFF22,		0x3a)
DEFINE_RELOCATION(R_IA_64_PLTOFF64I,		0x3b)
DEFINE_RELOCATION(R_IA_64_PLTOFF64MSB,		0x3e)
DEFINE_RELOCATION(R_IA_64_PLTOFF64LSB,		0x3f)
DEFINE_RELOCATION(R_IA_64_FPTR64I,		0x43)
DEFINE_RELOCATION(R_IA_64_FPTR32MSB,		0x44)
DEFINE_RELOCATION(R_IA_64_FPTR32LSB,		0x45)
DEFINE_RELOCATION(R_IA_64_FPTR64MSB,		0x46)
DEFINE_RELOCATION(R_IA_64_FPTR64LSB,		0x47)
DEFINE_RELOCATION(R_IA_64_PCREL60B,		0x48)
DEFINE_RELOCATION(R_IA_64_PCREL21B,		0x49)
DEFINE_RELOCATION(R_IA_64_PCREL21M,		0x4a)
DEFINE_RELOCATION(R_IA_64_PCREL21F,		0x4b)
DEFINE_RELOCATION(R_IA_64_PCREL32MSB,		0x4c)
DEFINE_RELOCATION(R_IA_64_PCREL32LSB,		0x4d)
DEFINE_RELOCATION(R_IA_64_PCREL64MSB,		0x4e)
DEFINE_RELOCATION(R_IA_64_PCREL64LSB,		0x4f)
DEFINE_RELOCATION(R_IA_64_LTOFF_FPTR22,		0x52)
DEFINE_RELOCATION(R_IA_64_LTOFF_FPTR64I,	0x53)
DEFINE_RELOCATION(R_IA_64_LTOFF_FPTR32MSB,	0x54)
DEFINE_RELOCATION(R_IA_64_LTOFF_FPTR32LSB,	0x55)
DEFINE_RELOCATION(R_IA_64_LTOFF_FPTR64MSB,	0x56)
DEFINE_RELOCATION(R_IA_64_LTOFF_FPTR64LSB,	0x57)
DEFINE_RELOCATION(R_IA_64_SEGREL32MSB,		0x5c)
DEFINE_RELOCATION(R_IA_64_SEGREL32LSB,		0x5d)
DEFINE_RELOCATION(R_IA_64_SEGREL64MSB,		0x5e)
DEFINE_RELOCATION(R_IA_64_SEGREL64LSB,		0x5f)
DEFINE_RELOCATION(R_IA_64_SECREL32MSB,		0x64)
DEFINE_RELOCATION(R_IA_64_SECREL32LSB,		0x65)
DEFINE_RELOCATION(R_IA_64_SECREL64MSB,		0x66)
DEFINE_RELOCATION(R_IA_64_SECREL64LSB,		0x67)
DEFINE_RELOCATION(R_IA_64_REL32MSB,		0x6c)
DEFINE_RELOCATION(R_IA_64_REL32LSB,		0x6d)
DEFINE_RELOCATION(R_IA_64_REL64MSB,		0x6e)
DEFINE_RELOCATION(R_IA_64_REL64LSB,		0x6f)
DEFINE_RELOCATION(R_IA_64_LTV32MSB,		0x74)
DEFINE_RELOCATION(R_IA_64_LTV32LSB,		0x75)
DEFINE_RELOCATION(R_IA_64_LTV64MSB,		0x76)
DEFINE_RELOCATION(R_IA_64_LTV64LSB,		0x77)
DEFINE_RELOCATION(R_IA_64_PCREL21BI,		0x79)
DEFINE_RELOCATION(R_IA_64_PCREL22,		0x7A)
DEFINE_RELOCATION(R_IA_64_PCREL64I,		0x7B)
DEFINE_RELOCATION(R_IA_64_IPLTMSB,		0x80)
DEFINE_RELOCATION(R_IA_64_IPLTLSB,		0x81)
DEFINE_RELOCATION(R_IA_64_SUB,			0x85)
DEFINE_RELOCATION(R_IA_64_LTOFF22X,		0x86)
DEFINE_RELOCATION(R_IA_64_LDXMOV,		0x87)
DEFINE_RELOCATION(R_IA_64_TPREL14,		0x91)
DEFINE_RELOCATION(R_IA_64_TPREL22,		0x92)
DEFINE_RELOCATION(R_IA_64_TPREL64I,		0x93)
DEFINE_RELOCATION(R_IA_64_TPREL64MSB,		0x96)
DEFINE_RELOCATION(R_IA_64_TPREL64LSB,		0x97)
DEFINE_RELOCATION(R_IA_64_LTOFF_TPREL22,	0x9A)
DEFINE_RELOCATION(R_IA_64_DTPMOD64MSB,		0xA6)
DEFINE_RELOCATION(R_IA_64_DTPMOD64LSB,		0xA7)
DEFINE_RELOCATION(R_IA_64_LTOFF_DTPMOD22,	0xAA)
DEFINE_RELOCATION(R_IA_64_DTPREL14,		0xB1)
DEFINE_RELOCATION(R_IA_64_DTPREL22,		0xB2)
DEFINE_RELOCATION(R_IA_64_DTPREL64I,		0xB3)
DEFINE_RELOCATION(R_IA_64_DTPREL32MSB,		0xB4)
DEFINE_RELOCATION(R_IA_64_DTPREL32LSB,		0xB5)
DEFINE_RELOCATION(R_IA_64_DTPREL64MSB,		0xB6)
DEFINE_RELOCATION(R_IA_64_DTPREL64LSB,		0xB7)
DEFINE_RELOCATION(R_IA_64_LTOFF_DTPREL22,	0xBA)
')

define(`DEFINE_MIPS_RELOCATIONS',`
DEFINE_RELOCATION(R_MIPS_NONE,			0)
DEFINE_RELOCATION(R_MIPS_16,			1)
DEFINE_RELOCATION(R_MIPS_32,			2)
DEFINE_RELOCATION(R_MIPS_REL32,			3)
DEFINE_RELOCATION(R_MIPS_26,			4)
DEFINE_RELOCATION(R_MIPS_HI16,			5)
DEFINE_RELOCATION(R_MIPS_LO16,			6)
DEFINE_RELOCATION(R_MIPS_GPREL16,		7)
DEFINE_RELOCATION(R_MIPS_LITERAL, 		8)
DEFINE_RELOCATION(R_MIPS_GOT16,			9)
DEFINE_RELOCATION(R_MIPS_PC16,			10)
DEFINE_RELOCATION(R_MIPS_CALL16,		11)
DEFINE_RELOCATION(R_MIPS_GPREL32,		12)
DEFINE_RELOCATION(R_MIPS_SHIFT5,		16)
DEFINE_RELOCATION(R_MIPS_SHIFT6,		17)
DEFINE_RELOCATION(R_MIPS_64,			18)
DEFINE_RELOCATION(R_MIPS_GOT_DISP,		19)
DEFINE_RELOCATION(R_MIPS_GOT_PAGE,		20)
DEFINE_RELOCATION(R_MIPS_GOT_OFST,		21)
DEFINE_RELOCATION(R_MIPS_GOT_HI16,		22)
DEFINE_RELOCATION(R_MIPS_GOT_LO16,		23)
DEFINE_RELOCATION(R_MIPS_SUB,			24)
DEFINE_RELOCATION(R_MIPS_CALLHI16,		30)
DEFINE_RELOCATION(R_MIPS_CALLLO16,		31)
DEFINE_RELOCATION(R_MIPS_JALR,			37)
DEFINE_RELOCATION(R_MIPS_TLS_DTPMOD32,		38)
DEFINE_RELOCATION(R_MIPS_TLS_DTPREL32,		39)
DEFINE_RELOCATION(R_MIPS_TLS_DTPMOD64,		40)
DEFINE_RELOCATION(R_MIPS_TLS_DTPREL64,		41)
DEFINE_RELOCATION(R_MIPS_TLS_GD,		42)
DEFINE_RELOCATION(R_MIPS_TLS_LDM,		43)
DEFINE_RELOCATION(R_MIPS_TLS_DTPREL_HI16,	44)
DEFINE_RELOCATION(R_MIPS_TLS_DTPREL_LO16,	45)
DEFINE_RELOCATION(R_MIPS_TLS_GOTTPREL,		46)
DEFINE_RELOCATION(R_MIPS_TLS_TPREL32,		47)
DEFINE_RELOCATION(R_MIPS_TLS_TPREL64,		48)
DEFINE_RELOCATION(R_MIPS_TLS_TPREL_HI16,	49)
DEFINE_RELOCATION(R_MIPS_TLS_TPREL_LO16,	50)
')

define(`DEFINE_PPC32_RELOCATIONS',`
DEFINE_RELOCATION(R_PPC_NONE,		0)
DEFINE_RELOCATION(R_PPC_ADDR32,		1)
DEFINE_RELOCATION(R_PPC_ADDR24,		2)
DEFINE_RELOCATION(R_PPC_ADDR16,		3)
DEFINE_RELOCATION(R_PPC_ADDR16_LO,	4)
DEFINE_RELOCATION(R_PPC_ADDR16_HI,	5)
DEFINE_RELOCATION(R_PPC_ADDR16_HA,	6)
DEFINE_RELOCATION(R_PPC_ADDR14,		7)
DEFINE_RELOCATION(R_PPC_ADDR14_BRTAKEN,	8)
DEFINE_RELOCATION(R_PPC_ADDR14_BRNTAKEN, 9)
DEFINE_RELOCATION(R_PPC_REL24,		10)
DEFINE_RELOCATION(R_PPC_REL14,		11)
DEFINE_RELOCATION(R_PPC_REL14_BRTAKEN,	12)
DEFINE_RELOCATION(R_PPC_REL14_BRNTAKEN,	13)
DEFINE_RELOCATION(R_PPC_GOT16,		14)
DEFINE_RELOCATION(R_PPC_GOT16_LO,	15)
DEFINE_RELOCATION(R_PPC_GOT16_HI,	16)
DEFINE_RELOCATION(R_PPC_GOT16_HA,	17)
DEFINE_RELOCATION(R_PPC_PLTREL24,	18)
DEFINE_RELOCATION(R_PPC_COPY,		19)
DEFINE_RELOCATION(R_PPC_GLOB_DAT,	20)
DEFINE_RELOCATION(R_PPC_JMP_SLOT,	21)
DEFINE_RELOCATION(R_PPC_RELATIVE,	22)
DEFINE_RELOCATION(R_PPC_LOCAL24PC,	23)
DEFINE_RELOCATION(R_PPC_UADDR32,	24)
DEFINE_RELOCATION(R_PPC_UADDR16,	25)
DEFINE_RELOCATION(R_PPC_REL32,		26)
DEFINE_RELOCATION(R_PPC_PLT32,		27)
DEFINE_RELOCATION(R_PPC_PLTREL32,	28)
DEFINE_RELOCATION(R_PPC_PLT16_LO,	29)
DEFINE_RELOCATION(R_PPC_PLT16_HI,	30)
DEFINE_RELOCATION(R_PPC_PLT16_HA,	31)
DEFINE_RELOCATION(R_PPC_SDAREL16,	32)
DEFINE_RELOCATION(R_PPC_SECTOFF,	33)
DEFINE_RELOCATION(R_PPC_SECTOFF_LO,	34)
DEFINE_RELOCATION(R_PPC_SECTOFF_HI,	35)
DEFINE_RELOCATION(R_PPC_SECTOFF_HA,	36)
DEFINE_RELOCATION(R_PPC_ADDR30,		37)
DEFINE_RELOCATION(R_PPC_TLS,		67)
DEFINE_RELOCATION(R_PPC_DTPMOD32,	68)
DEFINE_RELOCATION(R_PPC_TPREL16,	69)
DEFINE_RELOCATION(R_PPC_TPREL16_LO,	70)
DEFINE_RELOCATION(R_PPC_TPREL16_HI,	71)
DEFINE_RELOCATION(R_PPC_TPREL16_HA,	72)
DEFINE_RELOCATION(R_PPC_TPREL32,	73)
DEFINE_RELOCATION(R_PPC_DTPREL16,	74)
DEFINE_RELOCATION(R_PPC_DTPREL16_LO,	75)
DEFINE_RELOCATION(R_PPC_DTPREL16_HI,	76)
DEFINE_RELOCATION(R_PPC_DTPREL16_HA,	77)
DEFINE_RELOCATION(R_PPC_DTPREL32,	78)
DEFINE_RELOCATION(R_PPC_GOT_TLSGD16,	79)
DEFINE_RELOCATION(R_PPC_GOT_TLSGD16_LO,	80)
DEFINE_RELOCATION(R_PPC_GOT_TLSGD16_HI,	81)
DEFINE_RELOCATION(R_PPC_GOT_TLSGD16_HA,	82)
DEFINE_RELOCATION(R_PPC_GOT_TLSLD16,	83)
DEFINE_RELOCATION(R_PPC_GOT_TLSLD16_LO,	84)
DEFINE_RELOCATION(R_PPC_GOT_TLSLD16_HI,	85)
DEFINE_RELOCATION(R_PPC_GOT_TLSLD16_HA,	86)
DEFINE_RELOCATION(R_PPC_GOT_TPREL16,	87)
DEFINE_RELOCATION(R_PPC_GOT_TPREL16_LO,	88)
DEFINE_RELOCATION(R_PPC_GOT_TPREL16_HI,	89)
DEFINE_RELOCATION(R_PPC_GOT_TPREL16_HA,	90)
DEFINE_RELOCATION(R_PPC_GOT_DTPREL16,	91)
DEFINE_RELOCATION(R_PPC_GOT_DTPREL16_LO, 92)
DEFINE_RELOCATION(R_PPC_GOT_DTPREL16_HI, 93)
DEFINE_RELOCATION(R_PPC_GOT_DTPREL16_HA, 94)
DEFINE_RELOCATION(R_PPC_TLSGD,		95)
DEFINE_RELOCATION(R_PPC_TLSLD,		96)
DEFINE_RELOCATION(R_PPC_EMB_NADDR32,	101)
DEFINE_RELOCATION(R_PPC_EMB_NADDR16,	102)
DEFINE_RELOCATION(R_PPC_EMB_NADDR16_LO,	103)
DEFINE_RELOCATION(R_PPC_EMB_NADDR16_HI,	104)
DEFINE_RELOCATION(R_PPC_EMB_NADDR16_HA,	105)
DEFINE_RELOCATION(R_PPC_EMB_SDAI16,	106)
DEFINE_RELOCATION(R_PPC_EMB_SDA2I16,	107)
DEFINE_RELOCATION(R_PPC_EMB_SDA2REL,	108)
DEFINE_RELOCATION(R_PPC_EMB_SDA21,	109)
DEFINE_RELOCATION(R_PPC_EMB_MRKREF,	110)
DEFINE_RELOCATION(R_PPC_EMB_RELSEC16,	111)
DEFINE_RELOCATION(R_PPC_EMB_RELST_LO,	112)
DEFINE_RELOCATION(R_PPC_EMB_RELST_HI,	113)
DEFINE_RELOCATION(R_PPC_EMB_RELST_HA,	114)
DEFINE_RELOCATION(R_PPC_EMB_BIT_FLD,	115)
DEFINE_RELOCATION(R_PPC_EMB_RELSDA,	116)
')

define(`DEFINE_PPC64_RELOCATIONS',`
DEFINE_RELOCATION(R_PPC64_NONE,			0)
DEFINE_RELOCATION(R_PPC64_ADDR32,		1)
DEFINE_RELOCATION(R_PPC64_ADDR24,		2)
DEFINE_RELOCATION(R_PPC64_ADDR16,		3)
DEFINE_RELOCATION(R_PPC64_ADDR16_LO,		4)
DEFINE_RELOCATION(R_PPC64_ADDR16_HI,		5)
DEFINE_RELOCATION(R_PPC64_ADDR16_HA,		6)
DEFINE_RELOCATION(R_PPC64_ADDR14,		7)
DEFINE_RELOCATION(R_PPC64_ADDR14_BRTAKEN,	8)
DEFINE_RELOCATION(R_PPC64_ADDR14_BRNTAKEN,	9)
DEFINE_RELOCATION(R_PPC64_REL24,		10)
DEFINE_RELOCATION(R_PPC64_REL14,		11)
DEFINE_RELOCATION(R_PPC64_REL14_BRTAKEN,	12)
DEFINE_RELOCATION(R_PPC64_REL14_BRNTAKEN,	13)
DEFINE_RELOCATION(R_PPC64_GOT16,		14)
DEFINE_RELOCATION(R_PPC64_GOT16_LO,		15)
DEFINE_RELOCATION(R_PPC64_GOT16_HI,		16)
DEFINE_RELOCATION(R_PPC64_GOT16_HA,		17)
DEFINE_RELOCATION(R_PPC64_COPY,			19)
DEFINE_RELOCATION(R_PPC64_GLOB_DAT,		20)
DEFINE_RELOCATION(R_PPC64_JMP_SLOT,		21)
DEFINE_RELOCATION(R_PPC64_RELATIVE,		22)
DEFINE_RELOCATION(R_PPC64_UADDR32,		24)
DEFINE_RELOCATION(R_PPC64_UADDR16,		25)
DEFINE_RELOCATION(R_PPC64_REL32,		26)
DEFINE_RELOCATION(R_PPC64_PLT32,		27)
DEFINE_RELOCATION(R_PPC64_PLTREL32,		28)
DEFINE_RELOCATION(R_PPC64_PLT16_LO,		29)
DEFINE_RELOCATION(R_PPC64_PLT16_HI,		30)
DEFINE_RELOCATION(R_PPC64_PLT16_HA,		31)
DEFINE_RELOCATION(R_PPC64_SECTOFF,		33)
DEFINE_RELOCATION(R_PPC64_SECTOFF_LO,		34)
DEFINE_RELOCATION(R_PPC64_SECTOFF_HI,		35)
DEFINE_RELOCATION(R_PPC64_SECTOFF_HA,		36)
DEFINE_RELOCATION(R_PPC64_ADDR30,		37)
DEFINE_RELOCATION(R_PPC64_ADDR64,		38)
DEFINE_RELOCATION(R_PPC64_ADDR16_HIGHER,	39)
DEFINE_RELOCATION(R_PPC64_ADDR16_HIGHERA,	40)
DEFINE_RELOCATION(R_PPC64_ADDR16_HIGHEST,	41)
DEFINE_RELOCATION(R_PPC64_ADDR16_HIGHESTA,	42)
DEFINE_RELOCATION(R_PPC64_UADDR64,		43)
DEFINE_RELOCATION(R_PPC64_REL64,		44)
DEFINE_RELOCATION(R_PPC64_PLT64,		45)
DEFINE_RELOCATION(R_PPC64_PLTREL64,		46)
DEFINE_RELOCATION(R_PPC64_TOC16,		47)
DEFINE_RELOCATION(R_PPC64_TOC16_LO,		48)
DEFINE_RELOCATION(R_PPC64_TOC16_HI,		49)
DEFINE_RELOCATION(R_PPC64_TOC16_HA,		50)
DEFINE_RELOCATION(R_PPC64_TOC,			51)
DEFINE_RELOCATION(R_PPC64_PLTGOT16,		52)
DEFINE_RELOCATION(R_PPC64_PLTGOT16_LO,		53)
DEFINE_RELOCATION(R_PPC64_PLTGOT16_HI,		54)
DEFINE_RELOCATION(R_PPC64_PLTGOT16_HA,		55)
DEFINE_RELOCATION(R_PPC64_ADDR16_DS,		56)
DEFINE_RELOCATION(R_PPC64_ADDR16_LO_DS,		57)
DEFINE_RELOCATION(R_PPC64_GOT16_DS,		58)
DEFINE_RELOCATION(R_PPC64_GOT16_LO_DS,		59)
DEFINE_RELOCATION(R_PPC64_PLT16_LO_DS,		60)
DEFINE_RELOCATION(R_PPC64_SECTOFF_DS,		61)
DEFINE_RELOCATION(R_PPC64_SECTOFF_LO_DS,	62)
DEFINE_RELOCATION(R_PPC64_TOC16_DS,		63)
DEFINE_RELOCATION(R_PPC64_TOC16_LO_DS,		64)
DEFINE_RELOCATION(R_PPC64_PLTGOT16_DS,		65)
DEFINE_RELOCATION(R_PPC64_PLTGOT16_LO_DS,	66)
DEFINE_RELOCATION(R_PPC64_TLS,			67)
DEFINE_RELOCATION(R_PPC64_DTPMOD64,		68)
DEFINE_RELOCATION(R_PPC64_TPREL16,		69)
DEFINE_RELOCATION(R_PPC64_TPREL16_LO,		60)
DEFINE_RELOCATION(R_PPC64_TPREL16_HI,		71)
DEFINE_RELOCATION(R_PPC64_TPREL16_HA,		72)
DEFINE_RELOCATION(R_PPC64_TPREL64,		73)
DEFINE_RELOCATION(R_PPC64_DTPREL16,		74)
DEFINE_RELOCATION(R_PPC64_DTPREL16_LO,		75)
DEFINE_RELOCATION(R_PPC64_DTPREL16_HI,		76)
DEFINE_RELOCATION(R_PPC64_DTPREL16_HA,		77)
DEFINE_RELOCATION(R_PPC64_DTPREL64,		78)
DEFINE_RELOCATION(R_PPC64_GOT_TLSGD16,		79)
DEFINE_RELOCATION(R_PPC64_GOT_TLSGD16_LO,	80)
DEFINE_RELOCATION(R_PPC64_GOT_TLSGD16_HI,	81)
DEFINE_RELOCATION(R_PPC64_GOT_TLSGD16_HA,	82)
DEFINE_RELOCATION(R_PPC64_GOT_TLSLD16,		83)
DEFINE_RELOCATION(R_PPC64_GOT_TLSLD16_LO,	84)
DEFINE_RELOCATION(R_PPC64_GOT_TLSLD16_HI,	85)
DEFINE_RELOCATION(R_PPC64_GOT_TLSLD16_HA,	86)
DEFINE_RELOCATION(R_PPC64_GOT_TPREL16_DS,	87)
DEFINE_RELOCATION(R_PPC64_GOT_TPREL16_LO_DS,	88)
DEFINE_RELOCATION(R_PPC64_GOT_TPREL16_HI,	89)
DEFINE_RELOCATION(R_PPC64_GOT_TPREL16_HA,	90)
DEFINE_RELOCATION(R_PPC64_GOT_DTPREL16_DS,	91)
DEFINE_RELOCATION(R_PPC64_GOT_DTPREL16_LO_DS,	92)
DEFINE_RELOCATION(R_PPC64_GOT_DTPREL16_HI,	93)
DEFINE_RELOCATION(R_PPC64_GOT_DTPREL16_HA,	94)
DEFINE_RELOCATION(R_PPC64_TPREL16_DS,		95)
DEFINE_RELOCATION(R_PPC64_TPREL16_LO_DS,	96)
DEFINE_RELOCATION(R_PPC64_TPREL16_HIGHER,	97)
DEFINE_RELOCATION(R_PPC64_TPREL16_HIGHERA,	98)
DEFINE_RELOCATION(R_PPC64_TPREL16_HIGHEST,	99)
DEFINE_RELOCATION(R_PPC64_TPREL16_HIGHESTA,	100)
DEFINE_RELOCATION(R_PPC64_DTPREL16_DS,		101)
DEFINE_RELOCATION(R_PPC64_DTPREL16_LO_DS,	102)
DEFINE_RELOCATION(R_PPC64_DTPREL16_HIGHER,	103)
DEFINE_RELOCATION(R_PPC64_DTPREL16_HIGHERA,	104)
DEFINE_RELOCATION(R_PPC64_DTPREL16_HIGHEST,	105)
DEFINE_RELOCATION(R_PPC64_DTPREL16_HIGHESTA,	106)
DEFINE_RELOCATION(R_PPC64_TLSGD,		107)
DEFINE_RELOCATION(R_PPC64_TLSLD,		108)
')

define(`DEFINE_RISCV_RELOCATIONS',`
DEFINE_RELOCATION(R_RISCV_NONE,			0)
DEFINE_RELOCATION(R_RISCV_32,			1)
DEFINE_RELOCATION(R_RISCV_64,			2)
DEFINE_RELOCATION(R_RISCV_RELATIVE,		3)
DEFINE_RELOCATION(R_RISCV_COPY,			4)
DEFINE_RELOCATION(R_RISCV_JUMP_SLOT,		5)
DEFINE_RELOCATION(R_RISCV_TLS_DTPMOD32,		6)
DEFINE_RELOCATION(R_RISCV_TLS_DTPMOD64,		7)
DEFINE_RELOCATION(R_RISCV_TLS_DTPREL32,		8)
DEFINE_RELOCATION(R_RISCV_TLS_DTPREL64,		9)
DEFINE_RELOCATION(R_RISCV_TLS_TPREL32,		10)
DEFINE_RELOCATION(R_RISCV_TLS_TPREL64,		11)
DEFINE_RELOCATION(R_RISCV_BRANCH,		16)
DEFINE_RELOCATION(R_RISCV_JAL,			17)
DEFINE_RELOCATION(R_RISCV_CALL,			18)
DEFINE_RELOCATION(R_RISCV_CALL_PLT,		19)
DEFINE_RELOCATION(R_RISCV_GOT_HI20,		20)
DEFINE_RELOCATION(R_RISCV_TLS_GOT_HI20,		21)
DEFINE_RELOCATION(R_RISCV_TLS_GD_HI20,		22)
DEFINE_RELOCATION(R_RISCV_PCREL_HI20,		23)
DEFINE_RELOCATION(R_RISCV_PCREL_LO12_I,		24)
DEFINE_RELOCATION(R_RISCV_PCREL_LO12_S,		25)
DEFINE_RELOCATION(R_RISCV_HI20,			26)
DEFINE_RELOCATION(R_RISCV_LO12_I,		27)
DEFINE_RELOCATION(R_RISCV_LO12_S,		28)
DEFINE_RELOCATION(R_RISCV_TPREL_HI20,		29)
DEFINE_RELOCATION(R_RISCV_TPREL_LO12_I,		30)
DEFINE_RELOCATION(R_RISCV_TPREL_LO12_S,		31)
DEFINE_RELOCATION(R_RISCV_TPREL_ADD,		32)
DEFINE_RELOCATION(R_RISCV_ADD8,			33)
DEFINE_RELOCATION(R_RISCV_ADD16,		34)
DEFINE_RELOCATION(R_RISCV_ADD32,		35)
DEFINE_RELOCATION(R_RISCV_ADD64,		36)
DEFINE_RELOCATION(R_RISCV_SUB8,			37)
DEFINE_RELOCATION(R_RISCV_SUB16,		38)
DEFINE_RELOCATION(R_RISCV_SUB32,		39)
DEFINE_RELOCATION(R_RISCV_SUB64,		40)
DEFINE_RELOCATION(R_RISCV_GNU_VTINHERIT,	41)
DEFINE_RELOCATION(R_RISCV_GNU_VTENTRY,		42)
DEFINE_RELOCATION(R_RISCV_ALIGN,		43)
DEFINE_RELOCATION(R_RISCV_RVC_BRANCH,		44)
DEFINE_RELOCATION(R_RISCV_RVC_JUMP,		45)
DEFINE_RELOCATION(R_RISCV_RVC_LUI,		46)
DEFINE_RELOCATION(R_RISCV_GPREL_I,		47)
DEFINE_RELOCATION(R_RISCV_GPREL_S,		48)
DEFINE_RELOCATION(R_RISCV_TPREL_I,		49)
DEFINE_RELOCATION(R_RISCV_TPREL_S,		50)
DEFINE_RELOCATION(R_RISCV_RELAX,		51)
DEFINE_RELOCATION(R_RISCV_SUB6,			52)
DEFINE_RELOCATION(R_RISCV_SET6,			53)
DEFINE_RELOCATION(R_RISCV_SET8,			54)
DEFINE_RELOCATION(R_RISCV_SET16,		55)
DEFINE_RELOCATION(R_RISCV_SET32,		56)
DEFINE_RELOCATION(R_RISCV_32_PCREL,		57)
DEFINE_RELOCATION(R_RISCV_IRELATIVE,		58)
')

define(`DEFINE_SPARC_RELOCATIONS',`
DEFINE_RELOCATION(R_SPARC_NONE,		0)
DEFINE_RELOCATION(R_SPARC_8,		1)
DEFINE_RELOCATION(R_SPARC_16,		2)
DEFINE_RELOCATION(R_SPARC_32, 		3)
DEFINE_RELOCATION(R_SPARC_DISP8,	4)
DEFINE_RELOCATION(R_SPARC_DISP16,	5)
DEFINE_RELOCATION(R_SPARC_DISP32,	6)
DEFINE_RELOCATION(R_SPARC_WDISP30,	7)
DEFINE_RELOCATION(R_SPARC_WDISP22,	8)
DEFINE_RELOCATION(R_SPARC_HI22,		9)
DEFINE_RELOCATION(R_SPARC_22,		10)
DEFINE_RELOCATION(R_SPARC_13,		11)
DEFINE_RELOCATION(R_SPARC_LO10,		12)
DEFINE_RELOCATION(R_SPARC_GOT10,	13)
DEFINE_RELOCATION(R_SPARC_GOT13,	14)
DEFINE_RELOCATION(R_SPARC_GOT22,	15)
DEFINE_RELOCATION(R_SPARC_PC10,		16)
DEFINE_RELOCATION(R_SPARC_PC22,		17)
DEFINE_RELOCATION(R_SPARC_WPLT30,	18)
DEFINE_RELOCATION(R_SPARC_COPY,		19)
DEFINE_RELOCATION(R_SPARC_GLOB_DAT,	20)
DEFINE_RELOCATION(R_SPARC_JMP_SLOT,	21)
DEFINE_RELOCATION(R_SPARC_RELATIVE,	22)
DEFINE_RELOCATION(R_SPARC_UA32,		23)
DEFINE_RELOCATION(R_SPARC_PLT32,	24)
DEFINE_RELOCATION(R_SPARC_HIPLT22,	25)
DEFINE_RELOCATION(R_SPARC_LOPLT10,	26)
DEFINE_RELOCATION(R_SPARC_PCPLT32,	27)
DEFINE_RELOCATION(R_SPARC_PCPLT22,	28)
DEFINE_RELOCATION(R_SPARC_PCPLT10,	29)
DEFINE_RELOCATION(R_SPARC_10,		30)
DEFINE_RELOCATION(R_SPARC_11,		31)
DEFINE_RELOCATION(R_SPARC_64,		32)
DEFINE_RELOCATION(R_SPARC_OLO10,	33)
DEFINE_RELOCATION(R_SPARC_HH22,		34)
DEFINE_RELOCATION(R_SPARC_HM10,		35)
DEFINE_RELOCATION(R_SPARC_LM22,		36)
DEFINE_RELOCATION(R_SPARC_PC_HH22,	37)
DEFINE_RELOCATION(R_SPARC_PC_HM10,	38)
DEFINE_RELOCATION(R_SPARC_PC_LM22,	39)
DEFINE_RELOCATION(R_SPARC_WDISP16,	40)
DEFINE_RELOCATION(R_SPARC_WDISP19,	41)
DEFINE_RELOCATION(R_SPARC_GLOB_JMP,	42)
DEFINE_RELOCATION(R_SPARC_7,		43)
DEFINE_RELOCATION(R_SPARC_5,		44)
DEFINE_RELOCATION(R_SPARC_6,		45)
DEFINE_RELOCATION(R_SPARC_DISP64,	46)
DEFINE_RELOCATION(R_SPARC_PLT64,	47)
DEFINE_RELOCATION(R_SPARC_HIX22,	48)
DEFINE_RELOCATION(R_SPARC_LOX10,	49)
DEFINE_RELOCATION(R_SPARC_H44,		50)
DEFINE_RELOCATION(R_SPARC_M44,		51)
DEFINE_RELOCATION(R_SPARC_L44,		52)
DEFINE_RELOCATION(R_SPARC_REGISTER,	53)
DEFINE_RELOCATION(R_SPARC_UA64,		54)
DEFINE_RELOCATION(R_SPARC_UA16,		55)
DEFINE_RELOCATION(R_SPARC_TLS_GD_HI22,	56)
DEFINE_RELOCATION(R_SPARC_TLS_GD_LO10,	57)
DEFINE_RELOCATION(R_SPARC_TLS_GD_ADD,	58)
DEFINE_RELOCATION(R_SPARC_TLS_GD_CALL,	59)
DEFINE_RELOCATION(R_SPARC_TLS_LDM_HI22,	60)
DEFINE_RELOCATION(R_SPARC_TLS_LDM_LO10,	61)
DEFINE_RELOCATION(R_SPARC_TLS_LDM_ADD,	62)
DEFINE_RELOCATION(R_SPARC_TLS_LDM_CALL,	63)
DEFINE_RELOCATION(R_SPARC_TLS_LDO_HIX22, 64)
DEFINE_RELOCATION(R_SPARC_TLS_LDO_LOX10, 65)
DEFINE_RELOCATION(R_SPARC_TLS_LDO_ADD,	66)
DEFINE_RELOCATION(R_SPARC_TLS_IE_HI22,	67)
DEFINE_RELOCATION(R_SPARC_TLS_IE_LO10,	68)
DEFINE_RELOCATION(R_SPARC_TLS_IE_LD,	69)
DEFINE_RELOCATION(R_SPARC_TLS_IE_LDX,	70)
DEFINE_RELOCATION(R_SPARC_TLS_IE_ADD,	71)
DEFINE_RELOCATION(R_SPARC_TLS_LE_HIX22,	72)
DEFINE_RELOCATION(R_SPARC_TLS_LE_LOX10,	73)
DEFINE_RELOCATION(R_SPARC_TLS_DTPMOD32,	74)
DEFINE_RELOCATION(R_SPARC_TLS_DTPMOD64,	75)
DEFINE_RELOCATION(R_SPARC_TLS_DTPOFF32,	76)
DEFINE_RELOCATION(R_SPARC_TLS_DTPOFF64,	77)
DEFINE_RELOCATION(R_SPARC_TLS_TPOFF32,	78)
DEFINE_RELOCATION(R_SPARC_TLS_TPOFF64,	79)
DEFINE_RELOCATION(R_SPARC_GOTDATA_HIX22, 80)
DEFINE_RELOCATION(R_SPARC_GOTDATA_LOX10, 81)
DEFINE_RELOCATION(R_SPARC_GOTDATA_OP_HIX22, 82)
DEFINE_RELOCATION(R_SPARC_GOTDATA_OP_LOX10, 83)
DEFINE_RELOCATION(R_SPARC_GOTDATA_OP,	84)
DEFINE_RELOCATION(R_SPARC_H34,		85)
')

define(`DEFINE_X86_64_RELOCATIONS',`
DEFINE_RELOCATION(R_X86_64_NONE,	0)
DEFINE_RELOCATION(R_X86_64_64,		1)
DEFINE_RELOCATION(R_X86_64_PC32,	2)
DEFINE_RELOCATION(R_X86_64_GOT32,	3)
DEFINE_RELOCATION(R_X86_64_PLT32,	4)
DEFINE_RELOCATION(R_X86_64_COPY,	5)
DEFINE_RELOCATION(R_X86_64_GLOB_DAT,	6)
DEFINE_RELOCATION(R_X86_64_JUMP_SLOT,	7)
DEFINE_RELOCATION(R_X86_64_RELATIVE,	8)
DEFINE_RELOCATION(R_X86_64_GOTPCREL,	9)
DEFINE_RELOCATION(R_X86_64_32,		10)
DEFINE_RELOCATION(R_X86_64_32S,		11)
DEFINE_RELOCATION(R_X86_64_16,		12)
DEFINE_RELOCATION(R_X86_64_PC16,	13)
DEFINE_RELOCATION(R_X86_64_8,		14)
DEFINE_RELOCATION(R_X86_64_PC8,		15)
DEFINE_RELOCATION(R_X86_64_DTPMOD64,	16)
DEFINE_RELOCATION(R_X86_64_DTPOFF64,	17)
DEFINE_RELOCATION(R_X86_64_TPOFF64,	18)
DEFINE_RELOCATION(R_X86_64_TLSGD,	19)
DEFINE_RELOCATION(R_X86_64_TLSLD,	20)
DEFINE_RELOCATION(R_X86_64_DTPOFF32,	21)
DEFINE_RELOCATION(R_X86_64_GOTTPOFF,	22)
DEFINE_RELOCATION(R_X86_64_TPOFF32,	23)
DEFINE_RELOCATION(R_X86_64_PC64,	24)
DEFINE_RELOCATION(R_X86_64_GOTOFF64,	25)
DEFINE_RELOCATION(R_X86_64_GOTPC32,	26)
DEFINE_RELOCATION(R_X86_64_GOT64,	27)
DEFINE_RELOCATION(R_X86_64_GOTPCREL64,	28)
DEFINE_RELOCATION(R_X86_64_GOTPC64,	29)
DEFINE_RELOCATION(R_X86_64_GOTPLT64,	30)
DEFINE_RELOCATION(R_X86_64_PLTOFF64,	31)
DEFINE_RELOCATION(R_X86_64_SIZE32,	32)
DEFINE_RELOCATION(R_X86_64_SIZE64,	33)
DEFINE_RELOCATION(R_X86_64_GOTPC32_TLSDESC, 34)
DEFINE_RELOCATION(R_X86_64_TLSDESC_CALL, 35)
DEFINE_RELOCATION(R_X86_64_TLSDESC,	36)
DEFINE_RELOCATION(R_X86_64_IRELATIVE,	37)
DEFINE_RELOCATION(R_X86_64_RELATIVE64,	38)
DEFINE_RELOCATION(R_X86_64_GOTPCRELX,	41)
DEFINE_RELOCATION(R_X86_64_REX_GOTPCRELX, 42)
')

define(`DEFINE_RELOCATIONS',`
DEFINE_386_RELOCATIONS()
DEFINE_AARCH64_RELOCATIONS()
DEFINE_AMD64_RELOCATIONS()
DEFINE_ARM_RELOCATIONS()
DEFINE_IA64_RELOCATIONS()
DEFINE_MIPS_RELOCATIONS()
DEFINE_PPC32_RELOCATIONS()
DEFINE_PPC64_RELOCATIONS()
DEFINE_RISCV_RELOCATIONS()
DEFINE_SPARC_RELOCATIONS()
DEFINE_X86_64_RELOCATIONS()
')

define(`DEFINE_LL_FLAGS',`
DEFINE_LL_FLAG(LL_NONE,			0,
	`no flags')
DEFINE_LL_FLAG(LL_EXACT_MATCH,		0x1,
	`require an exact match')
DEFINE_LL_FLAG(LL_IGNORE_INT_VER,	0x2,
	`ignore version incompatibilities')
DEFINE_LL_FLAG(LL_REQUIRE_MINOR,	0x4,
	`')
DEFINE_LL_FLAG(LL_EXPORTS,		0x8,
	`')
DEFINE_LL_FLAG(LL_DELAY_LOAD,		0x10,
	`')
DEFINE_LL_FLAG(LL_DELTA,		0x20,
	`')
')

#
# Note tags
#
define(`DEFINE_NOTE_ENTRY_TYPES',`
DEFINE_NOTE_ENTRY(NT_ABI_TAG,			1,
	`Tag indicating the ABI')
DEFINE_NOTE_ENTRY(NT_GNU_HWCAP,			2,
	`Hardware capabilities')
DEFINE_NOTE_ENTRY(NT_GNU_BUILD_ID,		3,
	`Build id, set by ld(1)')
DEFINE_NOTE_ENTRY(NT_GNU_GOLD_VERSION,		4,
	`Version number of the GNU gold linker')
DEFINE_NOTE_ENTRY(NT_PRSTATUS,			1,
	`Process status')
DEFINE_NOTE_ENTRY(NT_FPREGSET,			2,
	`Floating point information')
DEFINE_NOTE_ENTRY(NT_PRPSINFO,			3,
	`Process information')
DEFINE_NOTE_ENTRY(NT_AUXV,			6,
	`Auxiliary vector')
DEFINE_NOTE_ENTRY(NT_PRXFPREG,		0x46E62B7FUL,
	`Linux user_xfpregs structure')
DEFINE_NOTE_ENTRY(NT_PSTATUS,			10,
	`Linux process status')
DEFINE_NOTE_ENTRY(NT_FPREGS,			12,
	`Linux floating point regset')
DEFINE_NOTE_ENTRY(NT_PSINFO,			13,
	`Linux process information')
DEFINE_NOTE_ENTRY(NT_LWPSTATUS,			16,
	`Linux lwpstatus_t type')
DEFINE_NOTE_ENTRY(NT_LWPSINFO,			17,
	`Linux lwpinfo_t type')
DEFINE_NOTE_ENTRY(NT_FREEBSD_NOINIT_TAG,	2,
	`FreeBSD no .init tag')
DEFINE_NOTE_ENTRY(NT_FREEBSD_ARCH_TAG,		3,
	`FreeBSD arch tag')
DEFINE_NOTE_ENTRY(NT_FREEBSD_FEATURE_CTL,	4,
	`FreeBSD feature control')
')

# Aliases for the ABI tag.
define(`DEFINE_NOTE_ENTRY_ALIASES',`
DEFINE_NOTE_ENTRY_ALIAS(NT_FREEBSD_ABI_TAG,	NT_ABI_TAG)
DEFINE_NOTE_ENTRY_ALIAS(NT_GNU_ABI_TAG,		NT_ABI_TAG)
DEFINE_NOTE_ENTRY_ALIAS(NT_NETBSD_IDENT,	NT_ABI_TAG)
DEFINE_NOTE_ENTRY_ALIAS(NT_OPENBSD_IDENT,	NT_ABI_TAG)
')

#
# Option kinds.
#
define(`DEFINE_OPTION_KINDS',`
DEFINE_OPTION_KIND(ODK_NULL,       0,
	`undefined')
DEFINE_OPTION_KIND(ODK_REGINFO,    1,
	`register usage info')
DEFINE_OPTION_KIND(ODK_EXCEPTIONS, 2,
	`exception processing info')
DEFINE_OPTION_KIND(ODK_PAD,        3,
	`section padding')
DEFINE_OPTION_KIND(ODK_HWPATCH,    4,
	`hardware patch applied')
DEFINE_OPTION_KIND(ODK_FILL,       5,
	`fill value used by linker')
DEFINE_OPTION_KIND(ODK_TAGS,       6,
	`reserved space for tools')
DEFINE_OPTION_KIND(ODK_HWAND,      7,
	`hardware AND patch applied')
DEFINE_OPTION_KIND(ODK_HWOR,       8,
	`hardware OR patch applied')
DEFINE_OPTION_KIND(ODK_GP_GROUP,   9,
	`GP group to use for text/data sections')
DEFINE_OPTION_KIND(ODK_IDENT,      10,
	`ID information')
DEFINE_OPTION_KIND(ODK_PAGESIZE,   11,
	`page size information')
')

#
# ODK_EXCEPTIONS info field masks.
#
define(`DEFINE_OPTION_EXCEPTIONS',`
DEFINE_OPTION_EXCEPTION(OEX_FPU_MIN,    0x0000001FUL,
	`minimum FPU exception which must be enabled')
DEFINE_OPTION_EXCEPTION(OEX_FPU_MAX,    0x00001F00UL,
	`maximum FPU exception which can be enabled')
DEFINE_OPTION_EXCEPTION(OEX_PAGE0,      0x00010000UL,
	`page zero must be mapped')
DEFINE_OPTION_EXCEPTION(OEX_SMM,        0x00020000UL,
	`run in sequential memory mode')
DEFINE_OPTION_EXCEPTION(OEX_PRECISEFP,  0x00040000UL,
	`run in precise FP exception mode')
DEFINE_OPTION_EXCEPTION(OEX_DISMISS,    0x00080000UL,
	`dismiss invalid address traps')
')

#
# ODK_PAD info field masks.
#
define(`DEFINE_OPTION_PADS',`
DEFINE_OPTION_PAD(OPAD_PREFIX,   0x0001)
DEFINE_OPTION_PAD(OPAD_POSTFIX,  0x0002)
DEFINE_OPTION_PAD(OPAD_SYMBOL,   0x0004)
')

#
# ODK_HWPATCH info field masks and ODK_HWAND/ODK_HWOR
# info field and hwp_flags[12] masks.
#
define(`DEFINE_ODK_HWPATCH_MASKS',`
DEFINE_ODK_HWPATCH_MASK(OHW_R4KEOP,     0x00000001UL,
	`patch for R4000 branch at end-of-page bug')
DEFINE_ODK_HWPATCH_MASK(OHW_R8KPFETCH,  0x00000002UL,
	`R8000 prefetch bug may occur')
DEFINE_ODK_HWPATCH_MASK(OHW_R5KEOP,     0x00000004UL,
	`patch for R5000 branch at end-of-page bug')
DEFINE_ODK_HWPATCH_MASK(OHW_R5KCVTL,    0x00000008UL,
	`R5000 cvt.[ds].l bug: clean == 1')
DEFINE_ODK_HWPATCH_MASK(OHW_R10KLDL,    0x00000010UL,
	`need patch for R10000 misaligned load')
DEFINE_ODK_HWPATCH_MASK(OHWA0_R4KEOP_CHECKED, 0x00000001UL,
	`object checked for R4000 end-of-page bug')
DEFINE_ODK_HWPATCH_MASK(OHWA0_R4KEOP_CLEAN, 0x00000002UL,
	`object verified clean for R4000 end-of-page bug')
DEFINE_ODK_HWPATCH_MASK(OHWO0_FIXADE,   0x00000001UL,
	`object requires call to fixade')
')

#
# ODK_IDENT/ODK_GP_GROUP info field masks.
#
define(`DEFINE_ODK_GP_MASKS',`
DEFINE_ODK_GP_MASK(OGP_GROUP,      0x0000FFFFUL,
	`GP group number')
DEFINE_ODK_GP_MASK(OGP_SELF,       0x00010000UL,
	`GP group is self-contained')
')

# MIPS ABI related constants.
define(`DEFINE_MIPS_ABIS',`
DEFINE_MIPS_ABI(E_MIPS_ABI_O32,		0x00001000,
	`MIPS 32 bit ABI (UCODE)')
DEFINE_MIPS_ABI(E_MIPS_ABI_O64,		0x00002000,
	`UCODE MIPS 64 bit ABI')
DEFINE_MIPS_ABI(E_MIPS_ABI_EABI32,	0x00003000,
	`Embedded ABI for 32-bit')
DEFINE_MIPS_ABI(E_MIPS_ABI_EABI64,	0x00004000,
	`Embedded ABI for 64-bit')
')
