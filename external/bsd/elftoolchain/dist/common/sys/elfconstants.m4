dnl	$NetBSD: elfconstants.m4,v 1.3 2022/05/02 09:43:23 jkoshy Exp $
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
	`Id: elfconstants.m4 3946 2021-04-10 21:10:42Z jkoshy')

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
_(`DF_ORIGIN',           0x1,
	`object being loaded may refer to `$ORIGIN'')
_(`DF_SYMBOLIC',         0x2,
	`search library for references before executable')
_(`DF_TEXTREL',          0x4,
	`relocation entries may modify text segment')
_(`DF_BIND_NOW',         0x8,
	`process relocation entries at load time')
_(`DF_STATIC_TLS',       0x10,
	`uses static thread-local storage')
_(`DF_1_BIND_NOW',       0x1,
	`process relocation entries at load time')
_(`DF_1_GLOBAL',         0x2,
	`unused')
_(`DF_1_GROUP',          0x4,
	`object is a member of a group')
_(`DF_1_NODELETE',       0x8,
	`object cannot be deleted from a process')
_(`DF_1_LOADFLTR',       0x10,
	`immediate load filtees')
_(`DF_1_INITFIRST',      0x20,
	`initialize object first')
_(`DF_1_NOOPEN',         0x40,
	`disallow dlopen()')
_(`DF_1_ORIGIN',         0x80,
	`object being loaded may refer to $ORIGIN')
_(`DF_1_DIRECT',         0x100,
	`direct bindings enabled')
_(`DF_1_INTERPOSE',      0x400,
	`object is interposer')
_(`DF_1_NODEFLIB',       0x800,
	`ignore default library search path')
_(`DF_1_NODUMP',         0x1000,
	`disallow dldump()')
_(`DF_1_CONFALT',        0x2000,
	`object is a configuration alternative')
_(`DF_1_ENDFILTEE',      0x4000,
	`filtee terminates filter search')
_(`DF_1_DISPRELDNE',     0x8000,
	`displacement relocation done')
_(`DF_1_DISPRELPND',     0x10000,
	`displacement relocation pending')')

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
_(`DT_MAXPOSTAGS',       34,
	`the number of positive tags')
_(`DT_LOOS',             0x6000000DUL,
	`start of OS-specific types')
_(`DT_SUNW_AUXILIARY',   0x6000000DUL,
	`offset of string naming auxiliary filtees')
_(`DT_SUNW_RTLDINF',     0x6000000EUL,
	`rtld internal use')
_(`DT_SUNW_FILTER',      0x6000000FUL,
	`offset of string naming standard filtees')
_(`DT_SUNW_CAP',         0x60000010UL,
	`address of hardware capabilities section')
_(`DT_SUNW_ASLR',        0x60000023UL,
	`Address Space Layout Randomization flag')
_(`DT_HIOS',             0x6FFFF000UL,
	`end of OS-specific types')
_(`DT_VALRNGLO',         0x6FFFFD00UL,
	`start of range using the d_val field')
_(`DT_GNU_PRELINKED',    0x6FFFFDF5UL,
	`prelinking timestamp')
_(`DT_GNU_CONFLICTSZ',   0x6FFFFDF6UL,
	`size of conflict section')
_(`DT_GNU_LIBLISTSZ',    0x6FFFFDF7UL,
	`size of library list')
_(`DT_CHECKSUM',         0x6FFFFDF8UL,
	`checksum for the object')
_(`DT_PLTPADSZ',         0x6FFFFDF9UL,
	`size of PLT padding')
_(`DT_MOVEENT',          0x6FFFFDFAUL,
	`size of DT_MOVETAB entries')
_(`DT_MOVESZ',           0x6FFFFDFBUL,
	`total size of the MOVETAB table')
_(`DT_FEATURE',          0x6FFFFDFCUL,
	`feature values')
_(`DT_POSFLAG_1',        0x6FFFFDFDUL,
	`dynamic position flags')
_(`DT_SYMINSZ',          0x6FFFFDFEUL,
	`size of the DT_SYMINFO table')
_(`DT_SYMINENT',         0x6FFFFDFFUL,
	`size of a DT_SYMINFO entry')
_(`DT_VALRNGHI',         0x6FFFFDFFUL,
	`end of range using the d_val field')
_(`DT_ADDRRNGLO',        0x6FFFFE00UL,
	`start of range using the d_ptr field')
_(`DT_GNU_HASH',	       0x6FFFFEF5UL,
	`GNU style hash tables')
_(`DT_TLSDESC_PLT',      0x6FFFFEF6UL,
	`location of PLT entry for TLS descriptor resolver calls')
_(`DT_TLSDESC_GOT',      0x6FFFFEF7UL,
	`location of GOT entry used by TLS descriptor resolver PLT entry')
_(`DT_GNU_CONFLICT',     0x6FFFFEF8UL,
	`address of conflict section')
_(`DT_GNU_LIBLIST',      0x6FFFFEF9UL,
	`address of conflict section')
_(`DT_CONFIG',           0x6FFFFEFAUL,
	`configuration file')
_(`DT_DEPAUDIT',         0x6FFFFEFBUL,
	`string defining audit libraries')
_(`DT_AUDIT',            0x6FFFFEFCUL,
	`string defining audit libraries')
_(`DT_PLTPAD',           0x6FFFFEFDUL,
	`PLT padding')
_(`DT_MOVETAB',          0x6FFFFEFEUL,
	`address of a move table')
_(`DT_SYMINFO',          0x6FFFFEFFUL,
	`address of the symbol information table')
_(`DT_ADDRRNGHI',        0x6FFFFEFFUL,
	`end of range using the d_ptr field')
_(`DT_VERSYM',	       0x6FFFFFF0UL,
	`address of the version section')
_(`DT_RELACOUNT',        0x6FFFFFF9UL,
	`count of RELA relocations')
_(`DT_RELCOUNT',         0x6FFFFFFAUL,
	`count of REL relocations')
_(`DT_FLAGS_1',          0x6FFFFFFBUL,
	`flag values')
_(`DT_VERDEF',	       0x6FFFFFFCUL,
	`address of the version definition segment')
_(`DT_VERDEFNUM',	       0x6FFFFFFDUL,
	`the number of version definition entries')
_(`DT_VERNEED',	       0x6FFFFFFEUL,
	`address of section with needed versions')
_(`DT_VERNEEDNUM',       0x6FFFFFFFUL,
	`the number of version needed entries')
_(`DT_LOPROC',           0x70000000UL,
	`start of processor-specific types')
_(`DT_ARM_SYMTABSZ',     0x70000001UL,
	`number of entries in the dynamic symbol table')
_(`DT_SPARC_REGISTER',   0x70000001UL,
	`index of an STT_SPARC_REGISTER symbol')
_(`DT_ARM_PREEMPTMAP',   0x70000002UL,
	`address of the preemption map')
_(`DT_MIPS_RLD_VERSION', 0x70000001UL,
	`version ID for runtime linker interface')
_(`DT_MIPS_TIME_STAMP',  0x70000002UL,
	`timestamp')
_(`DT_MIPS_ICHECKSUM',   0x70000003UL,
	`checksum of all external strings and common sizes')
_(`DT_MIPS_IVERSION',    0x70000004UL,
	`string table index of a version string')
_(`DT_MIPS_FLAGS',       0x70000005UL,
	`MIPS-specific flags')
_(`DT_MIPS_BASE_ADDRESS', 0x70000006UL,
	`base address for the executable/DSO')
_(`DT_MIPS_CONFLICT',    0x70000008UL,
	`address of .conflict section')
_(`DT_MIPS_LIBLIST',     0x70000009UL,
	`address of .liblist section')
_(`DT_MIPS_LOCAL_GOTNO', 0x7000000AUL,
	`number of local GOT entries')
_(`DT_MIPS_CONFLICTNO',  0x7000000BUL,
	`number of entries in the .conflict section')
_(`DT_MIPS_LIBLISTNO',   0x70000010UL,
	`number of entries in the .liblist section')
_(`DT_MIPS_SYMTABNO',    0x70000011UL,
	`number of entries in the .dynsym section')
_(`DT_MIPS_UNREFEXTNO',  0x70000012UL,
	`index of first external dynamic symbol not referenced locally')
_(`DT_MIPS_GOTSYM',      0x70000013UL,
	`index of first dynamic symbol corresponds to a GOT entry')
_(`DT_MIPS_HIPAGENO',    0x70000014UL,
	`number of page table entries in GOT')
_(`DT_MIPS_RLD_MAP',     0x70000016UL,
	`address of runtime linker map')
_(`DT_MIPS_DELTA_CLASS', 0x70000017UL,
	`Delta C++ class definition')
_(`DT_MIPS_DELTA_CLASS_NO', 0x70000018UL,
	`number of entries in DT_MIPS_DELTA_CLASS')
_(`DT_MIPS_DELTA_INSTANCE', 0x70000019UL,
	`Delta C++ class instances')
_(`DT_MIPS_DELTA_INSTANCE_NO', 0x7000001AUL,
	`number of entries in DT_MIPS_DELTA_INSTANCE')
_(`DT_MIPS_DELTA_RELOC', 0x7000001BUL,
	`Delta relocations')
_(`DT_MIPS_DELTA_RELOC_NO', 0x7000001CUL,
	`number of entries in DT_MIPS_DELTA_RELOC')
_(`DT_MIPS_DELTA_SYM',   0x7000001DUL,
	`Delta symbols referred by Delta relocations')
_(`DT_MIPS_DELTA_SYM_NO', 0x7000001EUL,
	`number of entries in DT_MIPS_DELTA_SYM')
_(`DT_MIPS_DELTA_CLASSSYM', 0x70000020UL,
	`Delta symbols for class declarations')
_(`DT_MIPS_DELTA_CLASSSYM_NO', 0x70000021UL,
	`number of entries in DT_MIPS_DELTA_CLASSSYM')
_(`DT_MIPS_CXX_FLAGS',   0x70000022UL,
	`C++ flavor flags')
_(`DT_MIPS_PIXIE_INIT',  0x70000023UL,
	`address of an initialization routine created by pixie')
_(`DT_MIPS_SYMBOL_LIB',  0x70000024UL,
	`address of .MIPS.symlib section')
_(`DT_MIPS_LOCALPAGE_GOTIDX', 0x70000025UL,
	`GOT index of first page table entry for a segment')
_(`DT_MIPS_LOCAL_GOTIDX', 0x70000026UL,
	`GOT index of first page table entry for a local symbol')
_(`DT_MIPS_HIDDEN_GOTIDX', 0x70000027UL,
	`GOT index of first page table entry for a hidden symbol')
_(`DT_MIPS_PROTECTED_GOTIDX', 0x70000028UL,
	`GOT index of first page table entry for a protected symbol')
_(`DT_MIPS_OPTIONS',     0x70000029UL,
	`address of .MIPS.options section')
_(`DT_MIPS_INTERFACE',   0x7000002AUL,
	`address of .MIPS.interface section')
_(`DT_MIPS_DYNSTR_ALIGN', 0x7000002BUL,
	`???')
_(`DT_MIPS_INTERFACE_SIZE', 0x7000002CUL,
	`size of .MIPS.interface section')
_(`DT_MIPS_RLD_TEXT_RESOLVE_ADDR', 0x7000002DUL,
	`address of _rld_text_resolve in GOT')
_(`DT_MIPS_PERF_SUFFIX', 0x7000002EUL,
	`default suffix of DSO to be appended by dlopen')
_(`DT_MIPS_COMPACT_SIZE', 0x7000002FUL,
	`size of a ucode compact relocation record (o32)')
_(`DT_MIPS_GP_VALUE',    0x70000030UL,
	`GP value of a specified GP relative range')
_(`DT_MIPS_AUX_DYNAMIC', 0x70000031UL,
	`address of an auxiliary dynamic table')
_(`DT_MIPS_PLTGOT',      0x70000032UL,
	`address of the PLTGOT')
_(`DT_MIPS_RLD_OBJ_UPDATE', 0x70000033UL,
	`object list update callback')
_(`DT_MIPS_RWPLT',       0x70000034UL,
	`address of a writable PLT')
_(`DT_PPC_GOT',          0x70000000UL,
	`value of _GLOBAL_OFFSET_TABLE_')
_(`DT_PPC_TLSOPT',       0x70000001UL,
	`TLS descriptor should be optimized')
_(`DT_PPC64_GLINK',      0x70000000UL,
	`address of .glink section')
_(`DT_PPC64_OPD',        0x70000001UL,
	`address of .opd section')
_(`DT_PPC64_OPDSZ',      0x70000002UL,
	`size of .opd section')
_(`DT_PPC64_TLSOPT',     0x70000003UL,
	`TLS descriptor should be optimized')
_(`DT_AUXILIARY',        0x7FFFFFFDUL,
	`offset of string naming auxiliary filtees')
_(`DT_USED',             0x7FFFFFFEUL,
	`ignored')
_(`DT_FILTER',           0x7FFFFFFFUL,
	`index of string naming filtees')
_(`DT_HIPROC',           0x7FFFFFFFUL,
	`end of processor-specific types')
')

define(`DEFINE_DYN_TYPE_ALIASES',`
_(`DT_DEPRECATED_SPARC_REGISTER', `DT_SPARC_REGISTER')
')

#
# Flags used in the executable header (field: e_flags).
#
define(`DEFINE_EHDR_FLAGS',`
_(EF_ARM_RELEXEC,      0x00000001UL,
	`dynamic segment describes only how to relocate segments')
_(EF_ARM_HASENTRY,     0x00000002UL,
	`e_entry contains a program entry point')
_(EF_ARM_SYMSARESORTED, 0x00000004UL,
	`subsection of symbol table is sorted by symbol value')
_(EF_ARM_DYNSYMSUSESEGIDX, 0x00000008UL,
	`dynamic symbol st_shndx = containing segment index + 1')
_(EF_ARM_MAPSYMSFIRST, 0x00000010UL,
	`mapping symbols precede other local symbols in symtab')
_(EF_ARM_BE8,          0x00800000UL,
	`file contains BE-8 code')
_(EF_ARM_LE8,          0x00400000UL,
	`file contains LE-8 code')
_(EF_ARM_EABIMASK,     0xFF000000UL,
	`mask for ARM EABI version number (0 denotes GNU or unknown)')
_(EF_ARM_EABI_UNKNOWN, 0x00000000UL,
	`Unknown or GNU ARM EABI version number')
_(EF_ARM_EABI_VER1,    0x01000000UL,
	`ARM EABI version 1')
_(EF_ARM_EABI_VER2,    0x02000000UL,
	`ARM EABI version 2')
_(EF_ARM_EABI_VER3,    0x03000000UL,
	`ARM EABI version 3')
_(EF_ARM_EABI_VER4,    0x04000000UL,
	`ARM EABI version 4')
_(EF_ARM_EABI_VER5,    0x05000000UL,
	`ARM EABI version 5')
_(EF_ARM_INTERWORK,    0x00000004UL,
	`GNU EABI extension')
_(EF_ARM_APCS_26,      0x00000008UL,
	`GNU EABI extension')
_(EF_ARM_APCS_FLOAT,   0x00000010UL,
	`GNU EABI extension')
_(EF_ARM_PIC,          0x00000020UL,
	`GNU EABI extension')
_(EF_ARM_ALIGN8,       0x00000040UL,
	`GNU EABI extension')
_(EF_ARM_NEW_ABI,      0x00000080UL,
	`GNU EABI extension')
_(EF_ARM_OLD_ABI,      0x00000100UL,
	`GNU EABI extension')
_(EF_ARM_SOFT_FLOAT,   0x00000200UL,
	`GNU EABI extension')
_(EF_ARM_VFP_FLOAT,    0x00000400UL,
	`GNU EABI extension')
_(EF_ARM_MAVERICK_FLOAT, 0x00000800UL,
	`GNU EABI extension')
_(EF_MIPS_NOREORDER,   0x00000001UL,
	`at least one .noreorder directive appeared in the source')
_(EF_MIPS_PIC,         0x00000002UL,
	`file contains position independent code')
_(EF_MIPS_CPIC,        0x00000004UL,
	`file code uses standard conventions for calling PIC')
_(EF_MIPS_UCODE,       0x00000010UL,
	`file contains UCODE (obsolete)')
_(EF_MIPS_ABI,	      0x00007000UL,
	`Application binary interface, see E_MIPS_* values')
_(EF_MIPS_ABI2,        0x00000020UL,
	`file follows MIPS III 32-bit ABI')
_(EF_MIPS_OPTIONS_FIRST, 0x00000080UL,
	`ld(1) should process .MIPS.options section first')
_(EF_MIPS_ARCH_ASE,    0x0F000000UL,
	`file uses application-specific architectural extensions')
_(EF_MIPS_ARCH_ASE_MDMX, 0x08000000UL,
	`file uses MDMX multimedia extensions')
_(EF_MIPS_ARCH_ASE_M16, 0x04000000UL,
	`file uses MIPS-16 ISA extensions')
_(EF_MIPS_ARCH_ASE_MICROMIPS, 0x02000000UL,
	`MicroMIPS architecture')
_(EF_MIPS_ARCH,         0xF0000000UL,
	`4-bit MIPS architecture field')
_(EF_MIPS_ARCH_1,	0x00000000UL,
	`MIPS I instruction set')
_(EF_MIPS_ARCH_2,	0x10000000UL,
	`MIPS II instruction set')
_(EF_MIPS_ARCH_3,	0x20000000UL,
	`MIPS III instruction set')
_(EF_MIPS_ARCH_4,	0x30000000UL,
	`MIPS IV instruction set')
_(EF_MIPS_ARCH_5,	0x40000000UL,
	`Never introduced')
_(EF_MIPS_ARCH_32,	0x50000000UL,
	`Mips32 Revision 1')
_(EF_MIPS_ARCH_64,	0x60000000UL,
	`Mips64 Revision 1')
_(EF_MIPS_ARCH_32R2,	0x70000000UL,
	`Mips32 Revision 2')
_(EF_MIPS_ARCH_64R2,	0x80000000UL,
	`Mips64 Revision 2')
_(EF_PPC_EMB,          0x80000000UL,
	`Embedded PowerPC flag')
_(EF_PPC_RELOCATABLE,  0x00010000UL,
	`-mrelocatable flag')
_(EF_PPC_RELOCATABLE_LIB, 0x00008000UL,
	`-mrelocatable-lib flag')
_(EF_RISCV_RVC,	    0x00000001UL,
	`Compressed instruction extension')
_(EF_RISCV_FLOAT_ABI_MASK, 0x00000006UL,
	`Floating point ABI')
_(EF_RISCV_FLOAT_ABI_SOFT, 0x00000000UL,
	`Software emulated floating point')
_(EF_RISCV_FLOAT_ABI_SINGLE, 0x00000002UL,
	`Single precision floating point')
_(EF_RISCV_FLOAT_ABI_DOUBLE, 0x00000004UL,
	`Double precision floating point')
_(EF_RISCV_FLOAT_ABI_QUAD, 0x00000006UL,
	`Quad precision floating point')
_(EF_RISCV_RVE,	    0x00000008UL,
	`Compressed instruction ABI')
_(EF_RISCV_TSO,	    0x00000010UL,
	`RVTSO memory consistency model')
_(EF_SPARC_EXT_MASK,   0x00ffff00UL,
	`Vendor Extension mask')
_(EF_SPARC_32PLUS,     0x00000100UL,
	`Generic V8+ features')
_(EF_SPARC_SUN_US1,    0x00000200UL,
	`Sun UltraSPARCTM 1 Extensions')
_(EF_SPARC_HAL_R1,     0x00000400UL,
	`HAL R1 Extensions')
_(EF_SPARC_SUN_US3,    0x00000800UL,
	`Sun UltraSPARC 3 Extensions')
_(EF_SPARCV9_MM,       0x00000003UL,
	`Mask for Memory Model')
_(EF_SPARCV9_TSO,      0x00000000UL,
	`Total Store Ordering')
_(EF_SPARCV9_PSO,      0x00000001UL,
	`Partial Store Ordering')
_(EF_SPARCV9_RMO,      0x00000002UL,
	`Relaxed Memory Ordering')
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
_(ELFCLASSNONE, 0,
	`Unknown ELF class')
_(ELFCLASS32,   1,
	`32 bit objects')
_(ELFCLASS64,   2,
	`64 bit objects')
')

#
# Endianness of data in an ELF object.
#
define(`DEFINE_ELF_DATA_ENDIANNESSES',`
_(ELFDATANONE, 0,
	`Unknown data endianness')
_(ELFDATA2LSB, 1,
	`little endian')
_(ELFDATA2MSB, 2,
	`big endian')
')


#
# The magic numbers used in the initial four bytes of an ELF object.
#
# These numbers are: 0x7F, 'E', 'L' and 'F'.
define(`DEFINE_ELF_MAGIC_VALUES',`
_(ELFMAG0, 0x7FU)
_(ELFMAG1, 0x45U)
_(ELFMAG2, 0x4CU)
_(ELFMAG3, 0x46U)
')

#
# ELF OS ABI field.
#
define(`DEFINE_ELF_OSABIS',`
_(ELFOSABI_NONE,       0,
	`No extensions or unspecified')
_(ELFOSABI_SYSV,       0,
	`SYSV')
_(ELFOSABI_HPUX,       1,
	`Hewlett-Packard HP-UX')
_(ELFOSABI_NETBSD,     2,
	`NetBSD')
_(ELFOSABI_GNU,        3,
	`GNU')
_(ELFOSABI_HURD,       4,
	`GNU/HURD')
_(ELFOSABI_86OPEN,     5,
	`86Open Common ABI')
_(ELFOSABI_SOLARIS,    6,
	`Sun Solaris')
_(ELFOSABI_AIX,        7,
	`AIX')
_(ELFOSABI_IRIX,       8,
	`IRIX')
_(ELFOSABI_FREEBSD,    9,
	`FreeBSD')
_(ELFOSABI_TRU64,      10,
	`Compaq TRU64 UNIX')
_(ELFOSABI_MODESTO,    11,
	`Novell Modesto')
_(ELFOSABI_OPENBSD,    12,
	`Open BSD')
_(ELFOSABI_OPENVMS,    13,
	`Open VMS')
_(ELFOSABI_NSK,        14,
	`Hewlett-Packard Non-Stop Kernel')
_(ELFOSABI_AROS,       15,
	`Amiga Research OS')
_(ELFOSABI_FENIXOS,    16,
	`The FenixOS highly scalable multi-core OS')
_(ELFOSABI_CLOUDABI,   17,
	`Nuxi CloudABI')
_(ELFOSABI_OPENVOS,    18,
	`Stratus Technologies OpenVOS')
_(ELFOSABI_ARM_AEABI,  64,
	`ARM specific symbol versioning extensions')
_(ELFOSABI_ARM,        97,
	`ARM ABI')
_(ELFOSABI_STANDALONE, 255,
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
_(EM_NONE,             0,
	`No machine')
_(EM_M32,              1,
	`AT&T WE 32100')
_(EM_SPARC,            2,
	`SPARC')
_(EM_386,              3,
	`Intel 80386')
_(EM_68K,              4,
	`Motorola 68000')
_(EM_88K,              5,
	`Motorola 88000')
_(EM_IAMCU,            6,
	`Intel MCU')
_(EM_860,              7,
	`Intel 80860')
_(EM_MIPS,             8,
	`MIPS I Architecture')
_(EM_S370,             9,
	`IBM System/370 Processor')
_(EM_MIPS_RS3_LE,      10,
	`MIPS RS3000 Little-endian')
_(EM_PARISC,           15,
	`Hewlett-Packard PA-RISC')
_(EM_VPP500,           17,
	`Fujitsu VPP500')
_(EM_SPARC32PLUS,      18,
	`Enhanced instruction set SPARC')
_(EM_960,              19,
	`Intel 80960')
_(EM_PPC,              20,
	`PowerPC')
_(EM_PPC64,            21,
	`64-bit PowerPC')
_(EM_S390,             22,
	`IBM System/390 Processor')
_(EM_SPU,              23,
	`IBM SPU/SPC')
_(EM_V800,             36,
	`NEC V800')
_(EM_FR20,             37,
	`Fujitsu FR20')
_(EM_RH32,             38,
	`TRW RH-32')
_(EM_RCE,              39,
	`Motorola RCE')
_(EM_ARM,              40,
	`Advanced RISC Machines ARM')
_(EM_ALPHA,            41,
	`Digital Alpha')
_(EM_SH,               42,
	`Hitachi SH')
_(EM_SPARCV9,          43,
	`SPARC Version 9')
_(EM_TRICORE,          44,
	`Siemens TriCore embedded processor')
_(EM_ARC,              45,
	`Argonaut RISC Core, Argonaut Technologies Inc.')
_(EM_H8_300,           46,
	`Hitachi H8/300')
_(EM_H8_300H,          47,
	`Hitachi H8/300H')
_(EM_H8S,              48,
	`Hitachi H8S')
_(EM_H8_500,           49,
	`Hitachi H8/500')
_(EM_IA_64,            50,
	`Intel IA-64 processor architecture')
_(EM_MIPS_X,           51,
	`Stanford MIPS-X')
_(EM_COLDFIRE,         52,
	`Motorola ColdFire')
_(EM_68HC12,           53,
	`Motorola M68HC12')
_(EM_MMA,              54,
	`Fujitsu MMA Multimedia Accelerator')
_(EM_PCP,              55,
	`Siemens PCP')
_(EM_NCPU,             56,
	`Sony nCPU embedded RISC processor')
_(EM_NDR1,             57,
	`Denso NDR1 microprocessor')
_(EM_STARCORE,         58,
	`Motorola Star*Core processor')
_(EM_ME16,             59,
	`Toyota ME16 processor')
_(EM_ST100,            60,
	`STMicroelectronics ST100 processor')
_(EM_TINYJ,            61,
	`Advanced Logic Corp. TinyJ embedded processor family')
_(EM_X86_64,           62,
	`AMD x86-64 architecture')
_(EM_PDSP,             63,
	`Sony DSP Processor')
_(EM_PDP10,            64,
	`Digital Equipment Corp. PDP-10')
_(EM_PDP11,            65,
	`Digital Equipment Corp. PDP-11')
_(EM_FX66,             66,
	`Siemens FX66 microcontroller')
_(EM_ST9PLUS,          67,
	`STMicroelectronics ST9+ 8/16 bit microcontroller')
_(EM_ST7,              68,
	`STMicroelectronics ST7 8-bit microcontroller')
_(EM_68HC16,           69,
	`Motorola MC68HC16 Microcontroller')
_(EM_68HC11,           70,
	`Motorola MC68HC11 Microcontroller')
_(EM_68HC08,           71,
	`Motorola MC68HC08 Microcontroller')
_(EM_68HC05,           72,
	`Motorola MC68HC05 Microcontroller')
_(EM_SVX,              73,
	`Silicon Graphics SVx')
_(EM_ST19,             74,
	`STMicroelectronics ST19 8-bit microcontroller')
_(EM_VAX,              75,
	`Digital VAX')
_(EM_CRIS,             76,
	`Axis Communications 32-bit embedded processor')
_(EM_JAVELIN,          77,
	`Infineon Technologies 32-bit embedded processor')
_(EM_FIREPATH,         78,
	`Element 14 64-bit DSP Processor')
_(EM_ZSP,              79,
	`LSI Logic 16-bit DSP Processor')
_(EM_MMIX,             80,
	`Educational 64-bit processor by Donald Knuth')
_(EM_HUANY,            81,
	`Harvard University machine-independent object files')
_(EM_PRISM,            82,
	`SiTera Prism')
_(EM_AVR,              83,
	`Atmel AVR 8-bit microcontroller')
_(EM_FR30,             84,
	`Fujitsu FR30')
_(EM_D10V,             85,
	`Mitsubishi D10V')
_(EM_D30V,             86,
	`Mitsubishi D30V')
_(EM_V850,             87,
	`NEC v850')
_(EM_M32R,             88,
	`Mitsubishi M32R')
_(EM_MN10300,          89,
	`Matsushita MN10300')
_(EM_MN10200,          90,
	`Matsushita MN10200')
_(EM_PJ,               91,
	`picoJava')
_(EM_OPENRISC,         92,
	`OpenRISC 32-bit embedded processor')
_(EM_ARC_COMPACT,      93,
	`ARC International ARCompact processor')
_(EM_XTENSA,           94,
	`Tensilica Xtensa Architecture')
_(EM_VIDEOCORE,        95,
	`Alphamosaic VideoCore processor')
_(EM_TMM_GPP,          96,
	`Thompson Multimedia General Purpose Processor')
_(EM_NS32K,            97,
	`National Semiconductor 32000 series')
_(EM_TPC,              98,
	`Tenor Network TPC processor')
_(EM_SNP1K,            99,
	`Trebia SNP 1000 processor')
_(EM_ST200,            100,
	`STMicroelectronics (www.st.com) ST200 microcontroller')
_(EM_IP2K,             101,
	`Ubicom IP2xxx microcontroller family')
_(EM_MAX,              102,
	`MAX Processor')
_(EM_CR,               103,
	`National Semiconductor CompactRISC microprocessor')
_(EM_F2MC16,           104,
	`Fujitsu F2MC16')
_(EM_MSP430,           105,
	`Texas Instruments embedded microcontroller msp430')
_(EM_BLACKFIN,         106,
	`Analog Devices Blackfin (DSP) processor')
_(EM_SE_C33,           107,
	`S1C33 Family of Seiko Epson processors')
_(EM_SEP,              108,
	`Sharp embedded microprocessor')
_(EM_ARCA,             109,
	`Arca RISC Microprocessor')
_(EM_UNICORE,          110,
	`Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University')
_(EM_EXCESS,           111,
	`eXcess: 16/32/64-bit configurable embedded CPU')
_(EM_DXP,              112,
	`Icera Semiconductor Inc. Deep Execution Processor')
_(EM_ALTERA_NIOS2,     113,
	`Altera Nios II soft-core processor')
_(EM_CRX,              114,
	`National Semiconductor CompactRISC CRX microprocessor')
_(EM_XGATE,            115,
	`Motorola XGATE embedded processor')
_(EM_C166,             116,
	`Infineon C16x/XC16x processor')
_(EM_M16C,             117,
	`Renesas M16C series microprocessors')
_(EM_DSPIC30F,         118,
	`Microchip Technology dsPIC30F Digital Signal Controller')
_(EM_CE,               119,
	`Freescale Communication Engine RISC core')
_(EM_M32C,             120,
	`Renesas M32C series microprocessors')
_(EM_TSK3000,          131,
	`Altium TSK3000 core')
_(EM_RS08,             132,
	`Freescale RS08 embedded processor')
_(EM_SHARC,            133,
	`Analog Devices SHARC family of 32-bit DSP processors')
_(EM_ECOG2,            134,
	`Cyan Technology eCOG2 microprocessor')
_(EM_SCORE7,           135,
	`Sunplus S+core7 RISC processor')
_(EM_DSP24,            136,
	`New Japan Radio (NJR) 24-bit DSP Processor')
_(EM_VIDEOCORE3,       137,
	`Broadcom VideoCore III processor')
_(EM_LATTICEMICO32,    138,
	`RISC processor for Lattice FPGA architecture')
_(EM_SE_C17,           139,
	`Seiko Epson C17 family')
_(EM_TI_C6000,         140,
	`The Texas Instruments TMS320C6000 DSP family')
_(EM_TI_C2000,         141,
	`The Texas Instruments TMS320C2000 DSP family')
_(EM_TI_C5500,         142,
	`The Texas Instruments TMS320C55x DSP family')
_(EM_MMDSP_PLUS,       160,
	`STMicroelectronics 64bit VLIW Data Signal Processor')
_(EM_CYPRESS_M8C,      161,
	`Cypress M8C microprocessor')
_(EM_R32C,             162,
	`Renesas R32C series microprocessors')
_(EM_TRIMEDIA,         163,
	`NXP Semiconductors TriMedia architecture family')
_(EM_QDSP6,            164,
	`QUALCOMM DSP6 Processor')
_(EM_8051,             165,
	`Intel 8051 and variants')
_(EM_STXP7X,           166,
	`STMicroelectronics STxP7x family of configurable and extensible RISC processors')
_(EM_NDS32,            167,
	`Andes Technology compact code size embedded RISC processor family')
_(EM_ECOG1,            168,
	`Cyan Technology eCOG1X family')
_(EM_ECOG1X,           168,
	`Cyan Technology eCOG1X family')
_(EM_MAXQ30,           169,
	`Dallas Semiconductor MAXQ30 Core Micro-controllers')
_(EM_XIMO16,           170,
	`New Japan Radio (NJR) 16-bit DSP Processor')
_(EM_MANIK,            171,
	`M2000 Reconfigurable RISC Microprocessor')
_(EM_CRAYNV2,          172,
	`Cray Inc. NV2 vector architecture')
_(EM_RX,               173,
	`Renesas RX family')
_(EM_METAG,            174,
	`Imagination Technologies META processor architecture')
_(EM_MCST_ELBRUS,      175,
	`MCST Elbrus general purpose hardware architecture')
_(EM_ECOG16,           176,
	`Cyan Technology eCOG16 family')
_(EM_CR16,             177,
	`National Semiconductor CompactRISC CR16 16-bit microprocessor')
_(EM_ETPU,             178,
	`Freescale Extended Time Processing Unit')
_(EM_SLE9X,            179,
	`Infineon Technologies SLE9X core')
_(EM_AARCH64,          183,
	`AArch64 (64-bit ARM)')
_(EM_AVR32,            185,
	`Atmel Corporation 32-bit microprocessor family')
_(EM_STM8,             186,
	`STMicroeletronics STM8 8-bit microcontroller')
_(EM_TILE64,           187,
	`Tilera TILE64 multicore architecture family')
_(EM_TILEPRO,          188,
	`Tilera TILEPro multicore architecture family')
_(EM_MICROBLAZE,       189,
	`Xilinx MicroBlaze 32-bit RISC soft processor core')
_(EM_CUDA,             190,
	`NVIDIA CUDA architecture')
_(EM_TILEGX,           191,
	`Tilera TILE-Gx multicore architecture family')
_(EM_CLOUDSHIELD,      192,
	`CloudShield architecture family')
_(EM_COREA_1ST,        193,
	`KIPO-KAIST Core-A 1st generation processor family')
_(EM_COREA_2ND,        194,
	`KIPO-KAIST Core-A 2nd generation processor family')
_(EM_ARC_COMPACT2,     195,
	`Synopsys ARCompact V2')
_(EM_OPEN8,            196,
	`Open8 8-bit RISC soft processor core')
_(EM_RL78,             197,
	`Renesas RL78 family')
_(EM_VIDEOCORE5,       198,
	`Broadcom VideoCore V processor')
_(EM_78KOR,            199,
	`Renesas 78KOR family')
_(EM_56800EX,          200,
	`Freescale 56800EX Digital Signal Controller')
_(EM_BA1,              201,
	`Beyond BA1 CPU architecture')
_(EM_BA2,              202,
	`Beyond BA2 CPU architecture')
_(EM_XCORE,            203,
	`XMOS xCORE processor family')
_(EM_MCHP_PIC,         204,
	`Microchip 8-bit PIC(r) family')
_(EM_INTELGT,          205,
	`Intel Graphics Technology')
_(EM_INTEL206,         206,
	`Reserved by Intel')
_(EM_INTEL207,         207,
	`Reserved by Intel')
_(EM_INTEL208,         208,
	`Reserved by Intel')
_(EM_INTEL209,         209,
	`Reserved by Intel')
_(EM_KM32,             210,
	`KM211 KM32 32-bit processor')
_(EM_KMX32,            211,
	`KM211 KMX32 32-bit processor')
_(EM_KMX16,            212,
	`KM211 KMX16 16-bit processor')
_(EM_KMX8,             213,
	`KM211 KMX8 8-bit processor')
_(EM_KVARC,            214,
	`KM211 KMX32 KVARC processor')
_(EM_CDP,              215,
	`Paneve CDP architecture family')
_(EM_COGE,             216,
	`Cognitive Smart Memory Processor')
_(EM_COOL,             217,
	`Bluechip Systems CoolEngine')
_(EM_NORC,             218,
	`Nanoradio Optimized RISC')
_(EM_CSR_KALIMBA,      219,
	`CSR Kalimba architecture family')
_(EM_Z80,              220,
	`Zilog Z80')
_(EM_VISIUM,           221,
	`Controls and Data Services VISIUMcore processor')
_(EM_FT32,             222,
	`FTDI Chip FT32 high performance 32-bit RISC architecture')
_(EM_MOXIE,            223,
	`Moxie processor family')
_(EM_AMDGPU,           224,
	`AMD GPU architecture')
_(EM_RISCV,            243,
	`RISC-V')
_(EM_LANAI,            244,
	`Lanai processor')
_(EM_CEVA,             245,
	`CEVA Processor Architecture Family')
_(EM_CEVA_X2,          246,
	`CEVA X2 Processor Family')
_(EM_BPF,              247,
	`Linux BPF – in-kernel virtual machine')
_(EM_GRAPHCORE_IPU,    248,
	`Graphcore Intelligent Processing Unit')
_(EM_IMG1,             249,
	`Imagination Technologies')
_(EM_NFP,              250,
	`Netronome Flow Processor (NFP)')
_(EM_CSKY,             252,
	`C-SKY processor family')
_(EM_65816,            257,
	`WDC 65816/65C816')
_(EM_KF32,             259,
	`ChipON KungFu 32')
')

define(`DEFINE_ELF_MACHINE_TYPE_SYNONYMS',`
_(EM_AMD64, EM_X86_64)
_(EM_ARC_A5, EM_ARC_COMPACT)
')

#
# ELF file types: (ET_*).
#
define(`DEFINE_ELF_TYPES',`
_(ET_NONE,   0,
	`No file type')
_(ET_REL,    1,
	`Relocatable object')
_(ET_EXEC,   2,
	`Executable')
_(ET_DYN,    3,
	`Shared object')
_(ET_CORE,   4,
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
_(EV_NONE, 0)
_(EV_CURRENT, 1)
')

#
# Flags for section groups.
#
define(`DEFINE_GRP_FLAGS',`
_(GRP_COMDAT, 	0x1,
	`COMDAT semantics')
_(GRP_MASKOS,	0x0ff00000,
	`OS-specific flags')
_(GRP_MASKPROC, 	0xf0000000,
	`processor-specific flags')
')

#
# Flags / mask for .gnu.versym sections.
#
define(`DEFINE_VERSYMS',`
_(VERSYM_VERSION,	0x7fff)
_(VERSYM_HIDDEN,	0x8000)
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
_(PF_MASKOS,           0x0ff00000,
	`OS-specific flags')
_(PF_MASKPROC,         0xf0000000,
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
_(PT_NULL,             0UL,
	`ignored entry')
_(PT_LOAD,             1UL,
	`loadable segment')
_(PT_DYNAMIC,          2UL,
	`contains dynamic linking information')
_(PT_INTERP,           3UL,
	`names an interpreter')
_(PT_NOTE,             4UL,
	`auxiliary information')
_(PT_SHLIB,            5UL,
	`reserved')
_(PT_PHDR,             6UL,
	`describes the program header itself')
_(PT_TLS,              7UL,
	`thread local storage')
_(PT_LOOS,             0x60000000UL,
	`start of OS-specific range')
_(PT_SUNW_UNWIND,      0x6464E550UL,
	`Solaris/amd64 stack unwind tables')
_(PT_GNU_EH_FRAME,     0x6474E550UL,
	`GCC generated .eh_frame_hdr segment')
_(PT_GNU_STACK,	    0x6474E551UL,
	`Stack flags')
_(PT_GNU_RELRO,	    0x6474E552UL,
	`Segment becomes read-only after relocation')
_(PT_OPENBSD_RANDOMIZE,0x65A3DBE6UL,
	`Segment filled with random data')
_(PT_OPENBSD_WXNEEDED, 0x65A3DBE7UL,
	`Program violates W^X')
_(PT_OPENBSD_BOOTDATA, 0x65A41BE6UL,
	`Boot data')
_(PT_SUNWBSS,          0x6FFFFFFAUL,
	`A Solaris .SUNW_bss section')
_(PT_SUNWSTACK,        0x6FFFFFFBUL,
	`A Solaris process stack')
_(PT_SUNWDTRACE,       0x6FFFFFFCUL,
	`Used by dtrace(1)')
_(PT_SUNWCAP,          0x6FFFFFFDUL,
	`Special hardware capability requirements')
_(PT_HIOS,             0x6FFFFFFFUL,
	`end of OS-specific range')
_(PT_LOPROC,           0x70000000UL,
	`start of processor-specific range')
_(PT_ARM_ARCHEXT,      0x70000000UL,
	`platform architecture compatibility information')
_(PT_ARM_EXIDX,        0x70000001UL,
	`exception unwind tables')
_(PT_MIPS_REGINFO,     0x70000000UL,
	`register usage information')
_(PT_MIPS_RTPROC,      0x70000001UL,
	`runtime procedure table')
_(PT_MIPS_OPTIONS,     0x70000002UL,
	`options segment')
_(PT_HIPROC,           0x7FFFFFFFUL,
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
_(SHF_WRITE,           0x1,
	`writable during program execution')
_(SHF_ALLOC,           0x2,
	`occupies memory during program execution')
_(SHF_EXECINSTR,       0x4,
	`executable instructions')
_(SHF_MERGE,           0x10,
	`may be merged to prevent duplication')
_(SHF_STRINGS,         0x20,
	`NUL-terminated character strings')
_(SHF_INFO_LINK,       0x40,
	`the sh_info field holds a link')
_(SHF_LINK_ORDER,      0x80,
	`special ordering requirements during linking')
_(SHF_OS_NONCONFORMING, 0x100,
	`requires OS-specific processing during linking')
_(SHF_GROUP,           0x200,
	`member of a section group')
_(SHF_TLS,             0x400,
	`holds thread-local storage')
_(SHF_COMPRESSED,      0x800,
	`holds compressed data')
_(SHF_MASKOS,          0x0FF00000UL,
	`bits reserved for OS-specific semantics')
_(SHF_AMD64_LARGE,     0x10000000UL,
	`section uses large code model')
_(SHF_ENTRYSECT,       0x10000000UL,
	`section contains an entry point (ARM)')
_(SHF_COMDEF,          0x80000000UL,
	`section may be multiply defined in input to link step (ARM)')
_(SHF_MIPS_GPREL,      0x10000000UL,
	`section must be part of global data area')
_(SHF_MIPS_MERGE,      0x20000000UL,
	`section data should be merged to eliminate duplication')
_(SHF_MIPS_ADDR,       0x40000000UL,
	`section data is addressed by default')
_(SHF_MIPS_STRING,     0x80000000UL,
	`section data is string data by default')
_(SHF_MIPS_NOSTRIP,    0x08000000UL,
	`section data may not be stripped')
_(SHF_MIPS_LOCAL,      0x04000000UL,
	`section data local to process')
_(SHF_MIPS_NAMES,      0x02000000UL,
	`linker must generate implicit hidden weak names')
_(SHF_MIPS_NODUPE,     0x01000000UL,
	`linker must retain only one copy')
_(SHF_ORDERED,         0x40000000UL,
	`section is ordered with respect to other sections')
_(SHF_EXCLUDE,	     0x80000000UL,
	`section is excluded from executables and shared objects')
_(SHF_MASKPROC,        0xF0000000UL,
	`bits reserved for processor-specific semantics')
')

#
# Special section indices.
#
define(`DEFINE_SECTION_INDICES',`
_(SHN_UNDEF, 	0,
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
_(SHT_NULL,            0, `inactive header')
_(SHT_PROGBITS,        1, `program defined information')
_(SHT_SYMTAB,          2, `symbol table')
_(SHT_STRTAB,          3, `string table')
_(SHT_RELA,            4,
	`relocation entries with addends')
_(SHT_HASH,            5, `symbol hash table')
_(SHT_DYNAMIC,         6,
	`information for dynamic linking')
_(SHT_NOTE,            7, `additional notes')
_(SHT_NOBITS,          8, `section occupying no space')
_(SHT_REL,             9,
	`relocation entries without addends')
_(SHT_SHLIB,           10, `reserved')
_(SHT_DYNSYM,          11, `symbol table')
_(SHT_INIT_ARRAY,      14,
	`pointers to initialization functions')
_(SHT_FINI_ARRAY,      15,
	`pointers to termination functions')
_(SHT_PREINIT_ARRAY,   16,
	`pointers to functions called before initialization')
_(SHT_GROUP,           17, `defines a section group')
_(SHT_SYMTAB_SHNDX,    18,
	`used for extended section numbering')
_(SHT_LOOS,            0x60000000UL,
	`start of OS-specific range')
_(SHT_SUNW_dof,	     0x6FFFFFF4UL,
	`used by dtrace')
_(SHT_SUNW_cap,	     0x6FFFFFF5UL,
	`capability requirements')
_(SHT_GNU_ATTRIBUTES,  0x6FFFFFF5UL,
	`object attributes')
_(SHT_SUNW_SIGNATURE,  0x6FFFFFF6UL,
	`module verification signature')
_(SHT_GNU_HASH,	     0x6FFFFFF6UL,
	`GNU Hash sections')
_(SHT_GNU_LIBLIST,     0x6FFFFFF7UL,
	`List of libraries to be prelinked')
_(SHT_SUNW_ANNOTATE,   0x6FFFFFF7UL,
	`special section where unresolved references are allowed')
_(SHT_SUNW_DEBUGSTR,   0x6FFFFFF8UL,
	`debugging information')
_(SHT_CHECKSUM, 	     0x6FFFFFF8UL,
	`checksum for dynamic shared objects')
_(SHT_SUNW_DEBUG,      0x6FFFFFF9UL,
	`debugging information')
_(SHT_SUNW_move,       0x6FFFFFFAUL,
	`information to handle partially initialized symbols')
_(SHT_SUNW_COMDAT,     0x6FFFFFFBUL,
	`section supporting merging of multiple copies of data')
_(SHT_SUNW_syminfo,    0x6FFFFFFCUL,
	`additional symbol information')
_(SHT_SUNW_verdef,     0x6FFFFFFDUL,
	`symbol versioning information')
_(SHT_SUNW_verneed,    0x6FFFFFFEUL,
	`symbol versioning requirements')
_(SHT_SUNW_versym,     0x6FFFFFFFUL,
	`symbol versioning table')
_(SHT_HIOS,            0x6FFFFFFFUL,
	`end of OS-specific range')
_(SHT_LOPROC,          0x70000000UL,
	`start of processor-specific range')
_(SHT_ARM_EXIDX,       0x70000001UL,
	`exception index table')
_(SHT_ARM_PREEMPTMAP,  0x70000002UL,
	`BPABI DLL dynamic linking preemption map')
_(SHT_ARM_ATTRIBUTES,  0x70000003UL,
	`object file compatibility attributes')
_(SHT_ARM_DEBUGOVERLAY, 0x70000004UL,
	`overlay debug information')
_(SHT_ARM_OVERLAYSECTION, 0x70000005UL,
	`overlay debug information')
_(SHT_MIPS_LIBLIST,    0x70000000UL,
	`DSO library information used in link')
_(SHT_MIPS_MSYM,       0x70000001UL,
	`MIPS symbol table extension')
_(SHT_MIPS_CONFLICT,   0x70000002UL,
	`symbol conflicting with DSO-defined symbols ')
_(SHT_MIPS_GPTAB,      0x70000003UL,
	`global pointer table')
_(SHT_MIPS_UCODE,      0x70000004UL,
	`reserved')
_(SHT_MIPS_DEBUG,      0x70000005UL,
	`reserved (obsolete debug information)')
_(SHT_MIPS_REGINFO,    0x70000006UL,
	`register usage information')
_(SHT_MIPS_PACKAGE,    0x70000007UL,
	`OSF reserved')
_(SHT_MIPS_PACKSYM,    0x70000008UL,
	`OSF reserved')
_(SHT_MIPS_RELD,       0x70000009UL,
	`dynamic relocation')
_(SHT_MIPS_IFACE,      0x7000000BUL,
	`subprogram interface information')
_(SHT_MIPS_CONTENT,    0x7000000CUL,
	`section content classification')
_(SHT_MIPS_OPTIONS,     0x7000000DUL,
	`general options')
_(SHT_MIPS_DELTASYM,   0x7000001BUL,
	`Delta C++: symbol table')
_(SHT_MIPS_DELTAINST,  0x7000001CUL,
	`Delta C++: instance table')
_(SHT_MIPS_DELTACLASS, 0x7000001DUL,
	`Delta C++: class table')
_(SHT_MIPS_DWARF,      0x7000001EUL,
	`DWARF debug information')
_(SHT_MIPS_DELTADECL,  0x7000001FUL,
	`Delta C++: declarations')
_(SHT_MIPS_SYMBOL_LIB, 0x70000020UL,
	`symbol-to-library mapping')
_(SHT_MIPS_EVENTS,     0x70000021UL,
	`event locations')
_(SHT_MIPS_TRANSLATE,  0x70000022UL,
	`???')
_(SHT_MIPS_PIXIE,      0x70000023UL,
	`special pixie sections')
_(SHT_MIPS_XLATE,      0x70000024UL,
	`address translation table')
_(SHT_MIPS_XLATE_DEBUG, 0x70000025UL,
	`SGI internal address translation table')
_(SHT_MIPS_WHIRL,      0x70000026UL,
	`intermediate code')
_(SHT_MIPS_EH_REGION,  0x70000027UL,
	`C++ exception handling region info')
_(SHT_MIPS_XLATE_OLD,  0x70000028UL,
	`obsolete')
_(SHT_MIPS_PDR_EXCEPTION, 0x70000029UL,
	`runtime procedure descriptor table exception information')
_(SHT_MIPS_ABIFLAGS,   0x7000002AUL,
	`ABI flags')
_(SHT_SPARC_GOTDATA,   0x70000000UL,
	`SPARC-specific data')
_(SHT_X86_64_UNWIND,   0x70000001UL,
	`unwind tables for the AMD64')
_(SHT_ORDERED,         0x7FFFFFFFUL,
	`sort entries in the section')
_(SHT_HIPROC,          0x7FFFFFFFUL,
	`end of processor-specific range')
_(SHT_LOUSER,          0x80000000UL,
	`start of application-specific range')
_(SHT_HIUSER,          0xFFFFFFFFUL,
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

#
# Symbol binding.
#
define(`DEFINE_SYMBOL_BINDING_KINDS',`
_(SYMINFO_BT_SELF,	0xFFFFU,
	`bound to self')
_(SYMINFO_BT_PARENT,	0xFFFEU,
	`bound to parent')
_(SYMINFO_BT_NONE,	0xFFFDU,
	`no special binding')
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

#
# Symbol flags.
#
define(`DEFINE_SYMBOL_FLAGS',`
_(SYMINFO_FLG_DIRECT,	0x01,
	`directly assocated reference')
_(SYMINFO_FLG_COPY,	0x04,
	`definition by copy-relocation')
_(SYMINFO_FLG_LAZYLOAD,	0x08,
	`object should be lazily loaded')
_(SYMINFO_FLG_DIRECTBIND,	0x10,
	`reference should be directly bound')
_(SYMINFO_FLG_NOEXTDIRECT, 0x20,
	`external references not allowed to bind to definition')
')

#
# Version dependencies.
#
define(`DEFINE_VERSIONING_DEPENDENCIES',`
_(VER_NDX_LOCAL,	0,
	`local scope')
_(VER_NDX_GLOBAL,	1,
	`global scope')
')

#
# Version flags.
#
define(`DEFINE_VERSIONING_FLAGS',`
_(VER_FLG_BASE,		0x1,
	`file version')
_(VER_FLG_WEAK,		0x2,
	`weak version')
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
define(`DEFINE_386_RELOCATIONS',`
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

define(`DEFINE_AARCH64_RELOCATIONS',`
_(R_AARCH64_NONE,				0)
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
_(R_AARCH64_TLSLD_ADD_DTPREL_HI12,		529)
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
_(R_AARCH64_COPY,				1024)
_(R_AARCH64_GLOB_DAT,				1025)
_(R_AARCH64_JUMP_SLOT,				1026)
_(R_AARCH64_RELATIVE,				1027)
_(R_AARCH64_TLS_DTPREL64,			1028)
_(R_AARCH64_TLS_DTPMOD64,			1029)
_(R_AARCH64_TLS_TPREL64,			1030)
_(R_AARCH64_TLSDESC,				1031)
_(R_AARCH64_IRELATIVE,				1032)
')

#
# These are the symbols used in the Sun ``Linkers and Loaders
# Guide'', Document No: 817-1984-17.  See the X86_64 relocations list
# below for the spellings used in the ELF specification.
#
define(`DEFINE_AMD64_RELOCATIONS',`
_(R_AMD64_NONE,		0)
_(R_AMD64_64,		1)
_(R_AMD64_PC32,		2)
_(R_AMD64_GOT32,	3)
_(R_AMD64_PLT32,	4)
_(R_AMD64_COPY,		5)
_(R_AMD64_GLOB_DAT,	6)
_(R_AMD64_JUMP_SLOT,	7)
_(R_AMD64_RELATIVE,	8)
_(R_AMD64_GOTPCREL,	9)
_(R_AMD64_32,		10)
_(R_AMD64_32S,		11)
_(R_AMD64_16,		12)
_(R_AMD64_PC16,		13)
_(R_AMD64_8,		14)
_(R_AMD64_PC8,		15)
_(R_AMD64_PC64,		24)
_(R_AMD64_GOTOFF64,	25)
_(R_AMD64_GOTPC32,	26)
')

#
# Relocation definitions from the ARM ELF ABI, version "ARM IHI
# 0044E" released on 30th November 2012.
#
define(`DEFINE_ARM_RELOCATIONS',`
_(R_ARM_NONE,			0)
_(R_ARM_PC24,			1)
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
_(R_ARM_SWI24,			13)
_(R_ARM_TLS_DESC,		13)
_(R_ARM_THM_SWI8,		14)
_(R_ARM_XPC25,			15)
_(R_ARM_THM_XPC22,		16)
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
_(R_ARM_PLT32,			27)
_(R_ARM_CALL,			28)
_(R_ARM_JUMP24,			29)
_(R_ARM_THM_JUMP24,		30)
_(R_ARM_BASE_ABS,		31)
_(R_ARM_ALU_PCREL_7_0,		32)
_(R_ARM_ALU_PCREL_15_8,		33)
_(R_ARM_ALU_PCREL_23_15,	34)
_(R_ARM_LDR_SBREL_11_0_NC,	35)
_(R_ARM_ALU_SBREL_19_12_NC,	36)
_(R_ARM_ALU_SBREL_27_20_CK,	37)
_(R_ARM_TARGET1,		38)
_(R_ARM_SBREL31,		39)
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
_(R_ARM_GNU_VTENTRY,		100)
_(R_ARM_GNU_VTINHERIT,		101)
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
_(R_ARM_ME_TOO,			128)
_(R_ARM_THM_TLS_DESCSEQ16,	129)
_(R_ARM_THM_TLS_DESCSEQ32,	130)
_(R_ARM_THM_GOT_BREL12,		131)
_(R_ARM_IRELATIVE,		140)
')

define(`DEFINE_IA64_RELOCATIONS',`
_(R_IA_64_NONE,			0)
_(R_IA_64_IMM14,		0x21)
_(R_IA_64_IMM22,		0x22)
_(R_IA_64_IMM64,		0x23)
_(R_IA_64_DIR32MSB,		0x24)
_(R_IA_64_DIR32LSB,		0x25)
_(R_IA_64_DIR64MSB,		0x26)
_(R_IA_64_DIR64LSB,		0x27)
_(R_IA_64_GPREL22,		0x2a)
_(R_IA_64_GPREL64I,		0x2b)
_(R_IA_64_GPREL32MSB,		0x2c)
_(R_IA_64_GPREL32LSB,		0x2d)
_(R_IA_64_GPREL64MSB,		0x2e)
_(R_IA_64_GPREL64LSB,		0x2f)
_(R_IA_64_LTOFF22,		0x32)
_(R_IA_64_LTOFF64I,		0x33)
_(R_IA_64_PLTOFF22,		0x3a)
_(R_IA_64_PLTOFF64I,		0x3b)
_(R_IA_64_PLTOFF64MSB,		0x3e)
_(R_IA_64_PLTOFF64LSB,		0x3f)
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
_(R_IA_64_LTOFF_FPTR22,		0x52)
_(R_IA_64_LTOFF_FPTR64I,	0x53)
_(R_IA_64_LTOFF_FPTR32MSB,	0x54)
_(R_IA_64_LTOFF_FPTR32LSB,	0x55)
_(R_IA_64_LTOFF_FPTR64MSB,	0x56)
_(R_IA_64_LTOFF_FPTR64LSB,	0x57)
_(R_IA_64_SEGREL32MSB,		0x5c)
_(R_IA_64_SEGREL32LSB,		0x5d)
_(R_IA_64_SEGREL64MSB,		0x5e)
_(R_IA_64_SEGREL64LSB,		0x5f)
_(R_IA_64_SECREL32MSB,		0x64)
_(R_IA_64_SECREL32LSB,		0x65)
_(R_IA_64_SECREL64MSB,		0x66)
_(R_IA_64_SECREL64LSB,		0x67)
_(R_IA_64_REL32MSB,		0x6c)
_(R_IA_64_REL32LSB,		0x6d)
_(R_IA_64_REL64MSB,		0x6e)
_(R_IA_64_REL64LSB,		0x6f)
_(R_IA_64_LTV32MSB,		0x74)
_(R_IA_64_LTV32LSB,		0x75)
_(R_IA_64_LTV64MSB,		0x76)
_(R_IA_64_LTV64LSB,		0x77)
_(R_IA_64_PCREL21BI,		0x79)
_(R_IA_64_PCREL22,		0x7A)
_(R_IA_64_PCREL64I,		0x7B)
_(R_IA_64_IPLTMSB,		0x80)
_(R_IA_64_IPLTLSB,		0x81)
_(R_IA_64_SUB,			0x85)
_(R_IA_64_LTOFF22X,		0x86)
_(R_IA_64_LDXMOV,		0x87)
_(R_IA_64_TPREL14,		0x91)
_(R_IA_64_TPREL22,		0x92)
_(R_IA_64_TPREL64I,		0x93)
_(R_IA_64_TPREL64MSB,		0x96)
_(R_IA_64_TPREL64LSB,		0x97)
_(R_IA_64_LTOFF_TPREL22,	0x9A)
_(R_IA_64_DTPMOD64MSB,		0xA6)
_(R_IA_64_DTPMOD64LSB,		0xA7)
_(R_IA_64_LTOFF_DTPMOD22,	0xAA)
_(R_IA_64_DTPREL14,		0xB1)
_(R_IA_64_DTPREL22,		0xB2)
_(R_IA_64_DTPREL64I,		0xB3)
_(R_IA_64_DTPREL32MSB,		0xB4)
_(R_IA_64_DTPREL32LSB,		0xB5)
_(R_IA_64_DTPREL64MSB,		0xB6)
_(R_IA_64_DTPREL64LSB,		0xB7)
_(R_IA_64_LTOFF_DTPREL22,	0xBA)
')

define(`DEFINE_MIPS_RELOCATIONS',`
_(R_MIPS_NONE,			0)
_(R_MIPS_16,			1)
_(R_MIPS_32,			2)
_(R_MIPS_REL32,			3)
_(R_MIPS_26,			4)
_(R_MIPS_HI16,			5)
_(R_MIPS_LO16,			6)
_(R_MIPS_GPREL16,		7)
_(R_MIPS_LITERAL, 		8)
_(R_MIPS_GOT16,			9)
_(R_MIPS_PC16,			10)
_(R_MIPS_CALL16,		11)
_(R_MIPS_GPREL32,		12)
_(R_MIPS_SHIFT5,		16)
_(R_MIPS_SHIFT6,		17)
_(R_MIPS_64,			18)
_(R_MIPS_GOT_DISP,		19)
_(R_MIPS_GOT_PAGE,		20)
_(R_MIPS_GOT_OFST,		21)
_(R_MIPS_GOT_HI16,		22)
_(R_MIPS_GOT_LO16,		23)
_(R_MIPS_SUB,			24)
_(R_MIPS_CALLHI16,		30)
_(R_MIPS_CALLLO16,		31)
_(R_MIPS_JALR,			37)
_(R_MIPS_TLS_DTPMOD32,		38)
_(R_MIPS_TLS_DTPREL32,		39)
_(R_MIPS_TLS_DTPMOD64,		40)
_(R_MIPS_TLS_DTPREL64,		41)
_(R_MIPS_TLS_GD,		42)
_(R_MIPS_TLS_LDM,		43)
_(R_MIPS_TLS_DTPREL_HI16,	44)
_(R_MIPS_TLS_DTPREL_LO16,	45)
_(R_MIPS_TLS_GOTTPREL,		46)
_(R_MIPS_TLS_TPREL32,		47)
_(R_MIPS_TLS_TPREL64,		48)
_(R_MIPS_TLS_TPREL_HI16,	49)
_(R_MIPS_TLS_TPREL_LO16,	50)
')

define(`DEFINE_PPC32_RELOCATIONS',`
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
_(R_PPC_SDAREL16,	32)
_(R_PPC_SECTOFF,	33)
_(R_PPC_SECTOFF_LO,	34)
_(R_PPC_SECTOFF_HI,	35)
_(R_PPC_SECTOFF_HA,	36)
_(R_PPC_ADDR30,		37)
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
_(R_PPC_GOT_DTPREL16,	91)
_(R_PPC_GOT_DTPREL16_LO, 92)
_(R_PPC_GOT_DTPREL16_HI, 93)
_(R_PPC_GOT_DTPREL16_HA, 94)
_(R_PPC_TLSGD,		95)
_(R_PPC_TLSLD,		96)
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
')

define(`DEFINE_PPC64_RELOCATIONS',`
_(R_PPC64_NONE,			0)
_(R_PPC64_ADDR32,		1)
_(R_PPC64_ADDR24,		2)
_(R_PPC64_ADDR16,		3)
_(R_PPC64_ADDR16_LO,		4)
_(R_PPC64_ADDR16_HI,		5)
_(R_PPC64_ADDR16_HA,		6)
_(R_PPC64_ADDR14,		7)
_(R_PPC64_ADDR14_BRTAKEN,	8)
_(R_PPC64_ADDR14_BRNTAKEN,	9)
_(R_PPC64_REL24,		10)
_(R_PPC64_REL14,		11)
_(R_PPC64_REL14_BRTAKEN,	12)
_(R_PPC64_REL14_BRNTAKEN,	13)
_(R_PPC64_GOT16,		14)
_(R_PPC64_GOT16_LO,		15)
_(R_PPC64_GOT16_HI,		16)
_(R_PPC64_GOT16_HA,		17)
_(R_PPC64_COPY,			19)
_(R_PPC64_GLOB_DAT,		20)
_(R_PPC64_JMP_SLOT,		21)
_(R_PPC64_RELATIVE,		22)
_(R_PPC64_UADDR32,		24)
_(R_PPC64_UADDR16,		25)
_(R_PPC64_REL32,		26)
_(R_PPC64_PLT32,		27)
_(R_PPC64_PLTREL32,		28)
_(R_PPC64_PLT16_LO,		29)
_(R_PPC64_PLT16_HI,		30)
_(R_PPC64_PLT16_HA,		31)
_(R_PPC64_SECTOFF,		33)
_(R_PPC64_SECTOFF_LO,		34)
_(R_PPC64_SECTOFF_HI,		35)
_(R_PPC64_SECTOFF_HA,		36)
_(R_PPC64_ADDR30,		37)
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
_(R_PPC64_TPREL16_LO,		60)
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
')

define(`DEFINE_RISCV_RELOCATIONS',`
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
_(R_RISCV_GNU_VTINHERIT,	41)
_(R_RISCV_GNU_VTENTRY,		42)
_(R_RISCV_ALIGN,		43)
_(R_RISCV_RVC_BRANCH,		44)
_(R_RISCV_RVC_JUMP,		45)
_(R_RISCV_RVC_LUI,		46)
_(R_RISCV_GPREL_I,		47)
_(R_RISCV_GPREL_S,		48)
_(R_RISCV_TPREL_I,		49)
_(R_RISCV_TPREL_S,		50)
_(R_RISCV_RELAX,		51)
_(R_RISCV_SUB6,			52)
_(R_RISCV_SET6,			53)
_(R_RISCV_SET8,			54)
_(R_RISCV_SET16,		55)
_(R_RISCV_SET32,		56)
_(R_RISCV_32_PCREL,		57)
_(R_RISCV_IRELATIVE,		58)
')

define(`DEFINE_SPARC_RELOCATIONS',`
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
_(R_SPARC_GLOB_JMP,	42)
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
')

define(`DEFINE_X86_64_RELOCATIONS',`
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
_(R_X86_64_GOTPLT64,	30)
_(R_X86_64_PLTOFF64,	31)
_(R_X86_64_SIZE32,	32)
_(R_X86_64_SIZE64,	33)
_(R_X86_64_GOTPC32_TLSDESC, 34)
_(R_X86_64_TLSDESC_CALL, 35)
_(R_X86_64_TLSDESC,	36)
_(R_X86_64_IRELATIVE,	37)
_(R_X86_64_RELATIVE64,	38)
_(R_X86_64_GOTPCRELX,	41)
_(R_X86_64_REX_GOTPCRELX, 42)
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

#
# Note tags
#
define(`DEFINE_NOTE_ENTRY_TYPES',`
_(NT_ABI_TAG,			1,
	`Tag indicating the ABI')
_(NT_GNU_HWCAP,			2,
	`Hardware capabilities')
_(NT_GNU_BUILD_ID,		3,
	`Build id, set by ld(1)')
_(NT_GNU_GOLD_VERSION,		4,
	`Version number of the GNU gold linker')
_(NT_PRSTATUS,			1,
	`Process status')
_(NT_FPREGSET,			2,
	`Floating point information')
_(NT_PRPSINFO,			3,
	`Process information')
_(NT_AUXV,			6,
	`Auxiliary vector')
_(NT_PRXFPREG,		0x46E62B7FUL,
	`Linux user_xfpregs structure')
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
_(NT_FREEBSD_NOINIT_TAG,	2,
	`FreeBSD no .init tag')
_(NT_FREEBSD_ARCH_TAG,		3,
	`FreeBSD arch tag')
_(NT_FREEBSD_FEATURE_CTL,	4,
	`FreeBSD feature control')
')

# Aliases for the ABI tag.
define(`DEFINE_NOTE_ENTRY_ALIASES',`
_(NT_FREEBSD_ABI_TAG,	NT_ABI_TAG)
_(NT_GNU_ABI_TAG,		NT_ABI_TAG)
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
_(OEX_FPU_MIN,    0x0000001FUL,
	`minimum FPU exception which must be enabled')
_(OEX_FPU_MAX,    0x00001F00UL,
	`maximum FPU exception which can be enabled')
_(OEX_PAGE0,      0x00010000UL,
	`page zero must be mapped')
_(OEX_SMM,        0x00020000UL,
	`run in sequential memory mode')
_(OEX_PRECISEFP,  0x00040000UL,
	`run in precise FP exception mode')
_(OEX_DISMISS,    0x00080000UL,
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
_(OHW_R4KEOP,     0x00000001UL,
	`patch for R4000 branch at end-of-page bug')
_(OHW_R8KPFETCH,  0x00000002UL,
	`R8000 prefetch bug may occur')
_(OHW_R5KEOP,     0x00000004UL,
	`patch for R5000 branch at end-of-page bug')
_(OHW_R5KCVTL,    0x00000008UL,
	`R5000 cvt.[ds].l bug: clean == 1')
_(OHW_R10KLDL,    0x00000010UL,
	`need patch for R10000 misaligned load')
_(OHWA0_R4KEOP_CHECKED, 0x00000001UL,
	`object checked for R4000 end-of-page bug')
_(OHWA0_R4KEOP_CLEAN, 0x00000002UL,
	`object verified clean for R4000 end-of-page bug')
_(OHWO0_FIXADE,   0x00000001UL,
	`object requires call to fixade')
')

#
# ODK_IDENT/ODK_GP_GROUP info field masks.
#
define(`DEFINE_ODK_GP_MASKS',`
_(OGP_GROUP,      0x0000FFFFUL,
	`GP group number')
_(OGP_SELF,       0x00010000UL,
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
