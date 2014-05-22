// mips.h -- ELF definitions specific to EM_MIPS  -*- C++ -*-

// Copyright 2012 Free Software Foundation, Inc.
// Written by Aleksandar Simeonov <aleksandar.simeonov@rt-rk.com>.

// This file is part of elfcpp.

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public License
// as published by the Free Software Foundation; either version 2, or
// (at your option) any later version.

// In addition to the permissions in the GNU Library General Public
// License, the Free Software Foundation gives you unlimited
// permission to link the compiled version of this file into
// combinations with other programs, and to distribute those
// combinations without any restriction coming from the use of this
// file.  (The Library Public License restrictions do apply in other
// respects; for example, they cover modification of the file, and
/// distribution when not linked into a combined executable.)

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.

// You should have received a copy of the GNU Library General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
// 02110-1301, USA.

#ifndef ELFCPP_MIPS_H
#define ELFCPP_MIPS_H

// Documentation for the MIPS relocs is taken from
//   http://math-atlas.sourceforge.net/devel/assembly/mipsabi32.pdf

namespace elfcpp
{

//
// MIPS Relocation Codes
//

enum
{
  R_MIPS_NONE = 0,
  R_MIPS_16 = 1,
  R_MIPS_32 = 2,
  R_MIPS_REL32 = 3,
  R_MIPS_26 = 4,
  R_MIPS_HI16 = 5,
  R_MIPS_LO16 = 6,
  R_MIPS_GPREL16 = 7,
  R_MIPS_LITERAL = 8,
  R_MIPS_GOT16 = 9,
  R_MIPS_PC16 = 10,
  R_MIPS_CALL16 = 11,
  R_MIPS_GPREL32 = 12,
  R_MIPS_UNUSED1 = 13,
  R_MIPS_UNUSED2 = 14,
  R_MIPS_UNUSED3 = 15,
  R_MIPS_SHIFT5 = 16,
  R_MIPS_SHIFT6 = 17,
  R_MIPS_64 = 18,
  R_MIPS_GOT_DISP = 19,
  R_MIPS_GOT_PAGE = 20,
  R_MIPS_GOT_OFST = 21,
  R_MIPS_GOT_HI16 = 22,
  R_MIPS_GOT_LO16 = 23,
  R_MIPS_SUB = 24,
  R_MIPS_INSERT_A = 25,		// Empty relocation
  R_MIPS_INSERT_B = 26,		// Empty relocation
  R_MIPS_DELETE = 27,		// Empty relocation
  R_MIPS_HIGHER = 28,
  R_MIPS_HIGHEST = 29,
  R_MIPS_CALL_HI16 = 30,
  R_MIPS_CALL_LO16 = 31,
  R_MIPS_SCN_DISP = 32,
  R_MIPS_REL16 = 33,		// Empty relocation
  R_MIPS_ADD_IMMEDIATE = 34,	// Empty relocation
  R_MIPS_PJUMP = 35,		// Empty relocation
  R_MIPS_RELGOT = 36,		// Empty relocation
  R_MIPS_JALR = 37,
  R_MIPS_TLS_DTPMOD32 = 38,
  R_MIPS_TLS_DTPREL32 = 39,
  R_MIPS_TLS_DTPMOD64 = 40,	// Empty relocation
  R_MIPS_TLS_DTPREL64 = 41,	// Empty relocation
  R_MIPS_TLS_GD = 42,
  R_MIPS_TLS_LDM = 43,
  R_MIPS_TLS_DTPREL_HI16 = 44,
  R_MIPS_TLS_DTPREL_LO16 = 45,
  R_MIPS_TLS_GOTTPREL = 46,
  R_MIPS_TLS_TPREL32 = 47,
  R_MIPS_TLS_TPREL64 = 48,	// Empty relocation
  R_MIPS_TLS_TPREL_HI16 = 49,
  R_MIPS_TLS_TPREL_LO16 = 50,
  R_MIPS_GLOB_DAT = 51,
  R_MIPS16_26 = 100,
  R_MIPS16_GPREL = 101,
  R_MIPS16_GOT16 = 102,
  R_MIPS16_CALL16 = 103,
  R_MIPS16_HI16 = 104,
  R_MIPS16_LO16 = 105,
  R_MIPS_COPY = 126,
  R_MIPS_JUMP_SLOT = 127,
  R_MIPS_PC32 = 248,
  R_MIPS_GNU_REL16_S2 = 250,
  R_MIPS_GNU_VTINHERIT = 253,
  R_MIPS_GNU_VTENTRY = 254
};

// Processor specific flags for the ELF header e_flags field.  */
enum
{
  // At least one .noreorder directive appears in the source.
  EF_MIPS_NOREORDER = 0x00000001,
  // File contains position independent code.
  EF_MIPS_PIC = 0x00000002,
  // Code in file uses the standard calling sequence for calling
  // position independent code.
  EF_MIPS_CPIC = 0x00000004,
  // ???  Unknown flag, set in IRIX 6's BSDdup2.o in libbsd.a.
  EF_MIPS_XGOT = 0x00000008,
  // Code in file uses UCODE (obsolete)
  EF_MIPS_UCODE = 0x00000010,
  // Code in file uses new ABI (-n32 on Irix 6).
  EF_MIPS_ABI2 = 0x00000020,
  // Process the .MIPS.options section first by ld
  EF_MIPS_OPTIONS_FIRST = 0x00000080,
  // Architectural Extensions used by this file
  EF_MIPS_ARCH_ASE = 0x0f000000,
  // Use MDMX multimedia extensions
  EF_MIPS_ARCH_ASE_MDMX = 0x08000000,
  // Use MIPS-16 ISA extensions
  EF_MIPS_ARCH_ASE_M16 = 0x04000000,
  // Use MICROMIPS ISA extensions.
  EF_MIPS_ARCH_ASE_MICROMIPS = 0x02000000,
  // Indicates code compiled for a 64-bit machine in 32-bit mode.
  // (regs are 32-bits wide.)
  EF_MIPS_32BITMODE = 0x00000100,
  // MIPS dynamic
  EF_MIPS_DYNAMIC = 0x40
};

// Machine variant if we know it.  This field was invented at Cygnus,
// but it is hoped that other vendors will adopt it.  If some standard
// is developed, this code should be changed to follow it.
enum
{
  EF_MIPS_MACH = 0x00FF0000,

// Cygnus is choosing values between 80 and 9F;
// 00 - 7F should be left for a future standard;
// the rest are open.

  E_MIPS_MACH_3900 = 0x00810000,
  E_MIPS_MACH_4010 = 0x00820000,
  E_MIPS_MACH_4100 = 0x00830000,
  E_MIPS_MACH_4650 = 0x00850000,
  E_MIPS_MACH_4120 = 0x00870000,
  E_MIPS_MACH_4111 = 0x00880000,
  E_MIPS_MACH_SB1 = 0x008a0000,
  E_MIPS_MACH_OCTEON = 0x008b0000,
  E_MIPS_MACH_XLR = 0x008c0000,
  E_MIPS_MACH_OCTEON2 = 0x008d0000,
  E_MIPS_MACH_5400 = 0x00910000,
  E_MIPS_MACH_5500 = 0x00980000,
  E_MIPS_MACH_9000 = 0x00990000,
  E_MIPS_MACH_LS2E = 0x00A00000,
  E_MIPS_MACH_LS2F = 0x00A10000,
  E_MIPS_MACH_LS3A = 0x00A20000,
};

// MIPS architecture
enum
{
  // Four bit MIPS architecture field.
  EF_MIPS_ARCH = 0xf0000000,
  // -mips1 code.
  E_MIPS_ARCH_1 = 0x00000000,
  // -mips2 code.
  E_MIPS_ARCH_2 = 0x10000000,
  // -mips3 code.
  E_MIPS_ARCH_3 = 0x20000000,
  // -mips4 code.
  E_MIPS_ARCH_4 = 0x30000000,
  // -mips5 code.
  E_MIPS_ARCH_5 = 0x40000000,
  // -mips32 code.
  E_MIPS_ARCH_32 = 0x50000000,
  // -mips64 code.
  E_MIPS_ARCH_64 = 0x60000000,
  // -mips32r2 code.
  E_MIPS_ARCH_32R2 = 0x70000000,
  // -mips64r2 code.
  E_MIPS_ARCH_64R2 = 0x80000000,
};

enum
{
  // Mask to extract ABI version, not really a flag value.
  EF_MIPS_ABI = 0x0000F000,

  // The original o32 abi.
  E_MIPS_ABI_O32 = 0x00001000,
  // O32 extended to work on 64 bit architectures
  E_MIPS_ABI_O64 = 0x00002000,
  // EABI in 32 bit mode
  E_MIPS_ABI_EABI32 = 0x00003000,
  // EABI in 64 bit mode
  E_MIPS_ABI_EABI64 = 0x00004000,
};

// Dynamic section MIPS flags
enum
{
  // None
  RHF_NONE = 0x00000000,
  // Use shortcut pointers
  RHF_QUICKSTART = 0x00000001,
  // Hash size not power of two
  RHF_NOTPOT = 0x00000002,
  // Ignore LD_LIBRARY_PATH
  RHF_NO_LIBRARY_REPLACEMENT = 0x00000004
};

// Special values for the st_other field in the symbol table.
enum
{
  // Two topmost bits denote the MIPS ISA for .text symbols:
  // + 00 -- standard MIPS code,
  // + 10 -- microMIPS code,
  // + 11 -- MIPS16 code; requires the following two bits to be set too.
  // Note that one of the MIPS16 bits overlaps with STO_MIPS_PIC.
  STO_MIPS_ISA = 0xc0,

  // The MIPS psABI was updated in 2008 with support for PLTs and copy
  // relocs.  There are therefore two types of nonzero SHN_UNDEF functions:
  // PLT entries and traditional MIPS lazy binding stubs.  We mark the former
  // with STO_MIPS_PLT to distinguish them from the latter.
  STO_MIPS_PLT = 0x8,

  // This value is used to mark PIC functions in an object that mixes
  // PIC and non-PIC.  Note that this bit overlaps with STO_MIPS16,
  // although MIPS16 symbols are never considered to be MIPS_PIC.
  STO_MIPS_PIC = 0x20,

  // This value is used for a mips16 .text symbol.
  STO_MIPS16 = 0xf0,

  // This value is used for a microMIPS .text symbol.  To distinguish from
  // STO_MIPS16, we set top two bits to be 10 to denote STO_MICROMIPS.  The
  // mask is STO_MIPS_ISA.
  STO_MICROMIPS  = 0x80
};

// Values for base offsets for thread-local storage
enum
{
  TP_OFFSET = 0x7000,
  DTP_OFFSET = 0x8000
};

} // End namespace elfcpp.

#endif // !defined(ELFCPP_MIPS_H)
