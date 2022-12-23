/* tc-ppc.c -- Assemble for the PowerPC or POWER (RS/6000)
   Copyright (C) 1994-2020 Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Support.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "as.h"
#include "safe-ctype.h"
#include "subsegs.h"
#include "dw2gencfi.h"
#include "opcode/ppc.h"

#ifdef OBJ_ELF
#include "elf/ppc.h"
#include "elf/ppc64.h"
#include "dwarf2dbg.h"
#endif

#ifdef TE_PE
#include "coff/pe.h"
#endif

#ifdef OBJ_XCOFF
#include "coff/xcoff.h"
#include "libxcoff.h"
#endif

/* This is the assembler for the PowerPC or POWER (RS/6000) chips.  */

/* Tell the main code what the endianness is.  */
extern int target_big_endian;

/* Whether or not, we've set target_big_endian.  */
static int set_target_endian = 0;

/* Whether to use user friendly register names.  */
#ifndef TARGET_REG_NAMES_P
#ifdef TE_PE
#define TARGET_REG_NAMES_P TRUE
#else
#define TARGET_REG_NAMES_P FALSE
#endif
#endif

/* Macros for calculating LO, HI, HA, HIGHER, HIGHERA, HIGHEST,
   HIGHESTA.  */

/* #lo(value) denotes the least significant 16 bits of the indicated.  */
#define PPC_LO(v) ((v) & 0xffff)

/* #hi(value) denotes bits 16 through 31 of the indicated value.  */
#define PPC_HI(v) (((v) >> 16) & 0xffff)

/* #ha(value) denotes the high adjusted value: bits 16 through 31 of
  the indicated value, compensating for #lo() being treated as a
  signed number.  */
#define PPC_HA(v) PPC_HI ((v) + 0x8000)

/* #higher(value) denotes bits 32 through 47 of the indicated value.  */
#define PPC_HIGHER(v) (((v) >> 16 >> 16) & 0xffff)

/* #highera(value) denotes bits 32 through 47 of the indicated value,
   compensating for #lo() being treated as a signed number.  */
#define PPC_HIGHERA(v) PPC_HIGHER ((v) + 0x8000)

/* #highest(value) denotes bits 48 through 63 of the indicated value.  */
#define PPC_HIGHEST(v) (((v) >> 24 >> 24) & 0xffff)

/* #highesta(value) denotes bits 48 through 63 of the indicated value,
   compensating for #lo being treated as a signed number.  */
#define PPC_HIGHESTA(v) PPC_HIGHEST ((v) + 0x8000)

#define SEX16(val) (((val) ^ 0x8000) - 0x8000)

/* For the time being on ppc64, don't report overflow on @h and @ha
   applied to constants.  */
#define REPORT_OVERFLOW_HI 0

static bfd_boolean reg_names_p = TARGET_REG_NAMES_P;

static void ppc_macro (char *, const struct powerpc_macro *);
static void ppc_byte (int);

#if defined (OBJ_XCOFF) || defined (OBJ_ELF)
static void ppc_tc (int);
static void ppc_machine (int);
#endif

#ifdef OBJ_XCOFF
static void ppc_comm (int);
static void ppc_bb (int);
static void ppc_bc (int);
static void ppc_bf (int);
static void ppc_biei (int);
static void ppc_bs (int);
static void ppc_eb (int);
static void ppc_ec (int);
static void ppc_ef (int);
static void ppc_es (int);
static void ppc_csect (int);
static void ppc_dwsect (int);
static void ppc_change_csect (symbolS *, offsetT);
static void ppc_function (int);
static void ppc_extern (int);
static void ppc_lglobl (int);
static void ppc_ref (int);
static void ppc_section (int);
static void ppc_named_section (int);
static void ppc_stabx (int);
static void ppc_rename (int);
static void ppc_toc (int);
static void ppc_xcoff_cons (int);
static void ppc_vbyte (int);
#endif

#ifdef OBJ_ELF
static void ppc_elf_rdata (int);
static void ppc_elf_lcomm (int);
static void ppc_elf_localentry (int);
static void ppc_elf_abiversion (int);
static void ppc_elf_gnu_attribute (int);
#endif

#ifdef TE_PE
static void ppc_previous (int);
static void ppc_pdata (int);
static void ppc_ydata (int);
static void ppc_reldata (int);
static void ppc_rdata (int);
static void ppc_ualong (int);
static void ppc_znop (int);
static void ppc_pe_comm (int);
static void ppc_pe_section (int);
static void ppc_pe_function (int);
static void ppc_pe_tocd (int);
#endif

/* Generic assembler global variables which must be defined by all
   targets.  */

#ifdef OBJ_ELF
/* This string holds the chars that always start a comment.  If the
   pre-processor is disabled, these aren't very useful.  The macro
   tc_comment_chars points to this.  We use this, rather than the
   usual comment_chars, so that we can switch for Solaris conventions.  */
static const char ppc_solaris_comment_chars[] = "#!";
static const char ppc_eabi_comment_chars[] = "#";

#ifdef TARGET_SOLARIS_COMMENT
const char *ppc_comment_chars = ppc_solaris_comment_chars;
#else
const char *ppc_comment_chars = ppc_eabi_comment_chars;
#endif
#else
const char comment_chars[] = "#";
#endif

/* Characters which start a comment at the beginning of a line.  */
const char line_comment_chars[] = "#";

/* Characters which may be used to separate multiple commands on a
   single line.  */
const char line_separator_chars[] = ";";

/* Characters which are used to indicate an exponent in a floating
   point number.  */
const char EXP_CHARS[] = "eE";

/* Characters which mean that a number is a floating point constant,
   as in 0d1.0.  */
const char FLT_CHARS[] = "dD";

/* Anything that can start an operand needs to be mentioned here,
   to stop the input scrubber eating whitespace.  */
const char ppc_symbol_chars[] = "%[";

/* The dwarf2 data alignment, adjusted for 32 or 64 bit.  */
int ppc_cie_data_alignment;

/* The dwarf2 minimum instruction length.  */
int ppc_dwarf2_line_min_insn_length;

/* More than this number of nops in an alignment op gets a branch
   instead.  */
unsigned long nop_limit = 4;

/* The type of processor we are assembling for.  This is one or more
   of the PPC_OPCODE flags defined in opcode/ppc.h.  */
ppc_cpu_t ppc_cpu = 0;
ppc_cpu_t sticky = 0;

/* Value for ELF e_flags EF_PPC64_ABI.  */
unsigned int ppc_abiversion = 0;

#ifdef OBJ_ELF
/* Flags set on encountering toc relocs.  */
static enum {
  has_large_toc_reloc = 1,
  has_small_toc_reloc = 2
} toc_reloc_types;
#endif

/* Warn on emitting data to code sections.  */
int warn_476;
uint64_t last_insn;
segT last_seg;
subsegT last_subseg;

/* The target specific pseudo-ops which we support.  */

const pseudo_typeS md_pseudo_table[] =
{
  /* Pseudo-ops which must be overridden.  */
  { "byte",	ppc_byte,	0 },

#ifdef OBJ_XCOFF
  /* Pseudo-ops specific to the RS/6000 XCOFF format.  Some of these
     legitimately belong in the obj-*.c file.  However, XCOFF is based
     on COFF, and is only implemented for the RS/6000.  We just use
     obj-coff.c, and add what we need here.  */
  { "comm",	ppc_comm,	0 },
  { "lcomm",	ppc_comm,	1 },
  { "bb",	ppc_bb,		0 },
  { "bc",	ppc_bc,		0 },
  { "bf",	ppc_bf,		0 },
  { "bi",	ppc_biei,	0 },
  { "bs",	ppc_bs,		0 },
  { "csect",	ppc_csect,	0 },
  { "dwsect",	ppc_dwsect,	0 },
  { "data",	ppc_section,	'd' },
  { "eb",	ppc_eb,		0 },
  { "ec",	ppc_ec,		0 },
  { "ef",	ppc_ef,		0 },
  { "ei",	ppc_biei,	1 },
  { "es",	ppc_es,		0 },
  { "extern",	ppc_extern,	0 },
  { "function",	ppc_function,	0 },
  { "lglobl",	ppc_lglobl,	0 },
  { "ref",	ppc_ref,	0 },
  { "rename",	ppc_rename,	0 },
  { "section",	ppc_named_section, 0 },
  { "stabx",	ppc_stabx,	0 },
  { "text",	ppc_section,	't' },
  { "toc",	ppc_toc,	0 },
  { "long",	ppc_xcoff_cons,	2 },
  { "llong",	ppc_xcoff_cons,	3 },
  { "word",	ppc_xcoff_cons,	1 },
  { "short",	ppc_xcoff_cons,	1 },
  { "vbyte",    ppc_vbyte,	0 },
#endif

#ifdef OBJ_ELF
  { "llong",	cons,		8 },
  { "rdata",	ppc_elf_rdata,	0 },
  { "rodata",	ppc_elf_rdata,	0 },
  { "lcomm",	ppc_elf_lcomm,	0 },
  { "localentry", ppc_elf_localentry,	0 },
  { "abiversion", ppc_elf_abiversion,	0 },
  { "gnu_attribute", ppc_elf_gnu_attribute, 0},
#endif

#ifdef TE_PE
  /* Pseudo-ops specific to the Windows NT PowerPC PE (coff) format.  */
  { "previous", ppc_previous,   0 },
  { "pdata",    ppc_pdata,      0 },
  { "ydata",    ppc_ydata,      0 },
  { "reldata",  ppc_reldata,    0 },
  { "rdata",    ppc_rdata,      0 },
  { "ualong",   ppc_ualong,     0 },
  { "znop",     ppc_znop,       0 },
  { "comm",	ppc_pe_comm,	0 },
  { "lcomm",	ppc_pe_comm,	1 },
  { "section",  ppc_pe_section, 0 },
  { "function",	ppc_pe_function,0 },
  { "tocd",     ppc_pe_tocd,    0 },
#endif

#if defined (OBJ_XCOFF) || defined (OBJ_ELF)
  { "tc",	ppc_tc,		0 },
  { "machine",  ppc_machine,    0 },
#endif

  { NULL,	NULL,		0 }
};


/* Predefined register names if -mregnames (or default for Windows NT).
   In general, there are lots of them, in an attempt to be compatible
   with a number of other Windows NT assemblers.  */

/* Structure to hold information about predefined registers.  */
struct pd_reg
  {
    const char *name;
    unsigned short value;
    unsigned short flags;
  };

/* List of registers that are pre-defined:

   Each general register has predefined names of the form:
   1. r<reg_num> which has the value <reg_num>.
   2. r.<reg_num> which has the value <reg_num>.

   Each floating point register has predefined names of the form:
   1. f<reg_num> which has the value <reg_num>.
   2. f.<reg_num> which has the value <reg_num>.

   Each vector unit register has predefined names of the form:
   1. v<reg_num> which has the value <reg_num>.
   2. v.<reg_num> which has the value <reg_num>.

   Each condition register has predefined names of the form:
   1. cr<reg_num> which has the value <reg_num>.
   2. cr.<reg_num> which has the value <reg_num>.

   There are individual registers as well:
   sp or r.sp     has the value 1
   rtoc or r.toc  has the value 2
   xer            has the value 1
   lr             has the value 8
   ctr            has the value 9
   dar            has the value 19
   dsisr          has the value 18
   dec            has the value 22
   sdr1           has the value 25
   srr0           has the value 26
   srr1           has the value 27

   The table is sorted. Suitable for searching by a binary search.  */

static const struct pd_reg pre_defined_registers[] =
{
  /* Condition Registers */
  { "cr.0", 0, PPC_OPERAND_CR_REG },
  { "cr.1", 1, PPC_OPERAND_CR_REG },
  { "cr.2", 2, PPC_OPERAND_CR_REG },
  { "cr.3", 3, PPC_OPERAND_CR_REG },
  { "cr.4", 4, PPC_OPERAND_CR_REG },
  { "cr.5", 5, PPC_OPERAND_CR_REG },
  { "cr.6", 6, PPC_OPERAND_CR_REG },
  { "cr.7", 7, PPC_OPERAND_CR_REG },

  { "cr0", 0, PPC_OPERAND_CR_REG },
  { "cr1", 1, PPC_OPERAND_CR_REG },
  { "cr2", 2, PPC_OPERAND_CR_REG },
  { "cr3", 3, PPC_OPERAND_CR_REG },
  { "cr4", 4, PPC_OPERAND_CR_REG },
  { "cr5", 5, PPC_OPERAND_CR_REG },
  { "cr6", 6, PPC_OPERAND_CR_REG },
  { "cr7", 7, PPC_OPERAND_CR_REG },

  { "ctr", 9, PPC_OPERAND_SPR },
  { "dar", 19, PPC_OPERAND_SPR },
  { "dec", 22, PPC_OPERAND_SPR },
  { "dsisr", 18, PPC_OPERAND_SPR },

  /* Floating point registers */
  { "f.0", 0, PPC_OPERAND_FPR },
  { "f.1", 1, PPC_OPERAND_FPR },
  { "f.10", 10, PPC_OPERAND_FPR },
  { "f.11", 11, PPC_OPERAND_FPR },
  { "f.12", 12, PPC_OPERAND_FPR },
  { "f.13", 13, PPC_OPERAND_FPR },
  { "f.14", 14, PPC_OPERAND_FPR },
  { "f.15", 15, PPC_OPERAND_FPR },
  { "f.16", 16, PPC_OPERAND_FPR },
  { "f.17", 17, PPC_OPERAND_FPR },
  { "f.18", 18, PPC_OPERAND_FPR },
  { "f.19", 19, PPC_OPERAND_FPR },
  { "f.2", 2, PPC_OPERAND_FPR },
  { "f.20", 20, PPC_OPERAND_FPR },
  { "f.21", 21, PPC_OPERAND_FPR },
  { "f.22", 22, PPC_OPERAND_FPR },
  { "f.23", 23, PPC_OPERAND_FPR },
  { "f.24", 24, PPC_OPERAND_FPR },
  { "f.25", 25, PPC_OPERAND_FPR },
  { "f.26", 26, PPC_OPERAND_FPR },
  { "f.27", 27, PPC_OPERAND_FPR },
  { "f.28", 28, PPC_OPERAND_FPR },
  { "f.29", 29, PPC_OPERAND_FPR },
  { "f.3", 3, PPC_OPERAND_FPR },
  { "f.30", 30, PPC_OPERAND_FPR },
  { "f.31", 31, PPC_OPERAND_FPR },
  { "f.32", 32, PPC_OPERAND_VSR },
  { "f.33", 33, PPC_OPERAND_VSR },
  { "f.34", 34, PPC_OPERAND_VSR },
  { "f.35", 35, PPC_OPERAND_VSR },
  { "f.36", 36, PPC_OPERAND_VSR },
  { "f.37", 37, PPC_OPERAND_VSR },
  { "f.38", 38, PPC_OPERAND_VSR },
  { "f.39", 39, PPC_OPERAND_VSR },
  { "f.4", 4, PPC_OPERAND_FPR },
  { "f.40", 40, PPC_OPERAND_VSR },
  { "f.41", 41, PPC_OPERAND_VSR },
  { "f.42", 42, PPC_OPERAND_VSR },
  { "f.43", 43, PPC_OPERAND_VSR },
  { "f.44", 44, PPC_OPERAND_VSR },
  { "f.45", 45, PPC_OPERAND_VSR },
  { "f.46", 46, PPC_OPERAND_VSR },
  { "f.47", 47, PPC_OPERAND_VSR },
  { "f.48", 48, PPC_OPERAND_VSR },
  { "f.49", 49, PPC_OPERAND_VSR },
  { "f.5", 5, PPC_OPERAND_FPR },
  { "f.50", 50, PPC_OPERAND_VSR },
  { "f.51", 51, PPC_OPERAND_VSR },
  { "f.52", 52, PPC_OPERAND_VSR },
  { "f.53", 53, PPC_OPERAND_VSR },
  { "f.54", 54, PPC_OPERAND_VSR },
  { "f.55", 55, PPC_OPERAND_VSR },
  { "f.56", 56, PPC_OPERAND_VSR },
  { "f.57", 57, PPC_OPERAND_VSR },
  { "f.58", 58, PPC_OPERAND_VSR },
  { "f.59", 59, PPC_OPERAND_VSR },
  { "f.6", 6, PPC_OPERAND_FPR },
  { "f.60", 60, PPC_OPERAND_VSR },
  { "f.61", 61, PPC_OPERAND_VSR },
  { "f.62", 62, PPC_OPERAND_VSR },
  { "f.63", 63, PPC_OPERAND_VSR },
  { "f.7", 7, PPC_OPERAND_FPR },
  { "f.8", 8, PPC_OPERAND_FPR },
  { "f.9", 9, PPC_OPERAND_FPR },

  { "f0", 0, PPC_OPERAND_FPR },
  { "f1", 1, PPC_OPERAND_FPR },
  { "f10", 10, PPC_OPERAND_FPR },
  { "f11", 11, PPC_OPERAND_FPR },
  { "f12", 12, PPC_OPERAND_FPR },
  { "f13", 13, PPC_OPERAND_FPR },
  { "f14", 14, PPC_OPERAND_FPR },
  { "f15", 15, PPC_OPERAND_FPR },
  { "f16", 16, PPC_OPERAND_FPR },
  { "f17", 17, PPC_OPERAND_FPR },
  { "f18", 18, PPC_OPERAND_FPR },
  { "f19", 19, PPC_OPERAND_FPR },
  { "f2", 2, PPC_OPERAND_FPR },
  { "f20", 20, PPC_OPERAND_FPR },
  { "f21", 21, PPC_OPERAND_FPR },
  { "f22", 22, PPC_OPERAND_FPR },
  { "f23", 23, PPC_OPERAND_FPR },
  { "f24", 24, PPC_OPERAND_FPR },
  { "f25", 25, PPC_OPERAND_FPR },
  { "f26", 26, PPC_OPERAND_FPR },
  { "f27", 27, PPC_OPERAND_FPR },
  { "f28", 28, PPC_OPERAND_FPR },
  { "f29", 29, PPC_OPERAND_FPR },
  { "f3", 3, PPC_OPERAND_FPR },
  { "f30", 30, PPC_OPERAND_FPR },
  { "f31", 31, PPC_OPERAND_FPR },
  { "f32", 32, PPC_OPERAND_VSR },
  { "f33", 33, PPC_OPERAND_VSR },
  { "f34", 34, PPC_OPERAND_VSR },
  { "f35", 35, PPC_OPERAND_VSR },
  { "f36", 36, PPC_OPERAND_VSR },
  { "f37", 37, PPC_OPERAND_VSR },
  { "f38", 38, PPC_OPERAND_VSR },
  { "f39", 39, PPC_OPERAND_VSR },
  { "f4", 4, PPC_OPERAND_FPR },
  { "f40", 40, PPC_OPERAND_VSR },
  { "f41", 41, PPC_OPERAND_VSR },
  { "f42", 42, PPC_OPERAND_VSR },
  { "f43", 43, PPC_OPERAND_VSR },
  { "f44", 44, PPC_OPERAND_VSR },
  { "f45", 45, PPC_OPERAND_VSR },
  { "f46", 46, PPC_OPERAND_VSR },
  { "f47", 47, PPC_OPERAND_VSR },
  { "f48", 48, PPC_OPERAND_VSR },
  { "f49", 49, PPC_OPERAND_VSR },
  { "f5", 5, PPC_OPERAND_FPR },
  { "f50", 50, PPC_OPERAND_VSR },
  { "f51", 51, PPC_OPERAND_VSR },
  { "f52", 52, PPC_OPERAND_VSR },
  { "f53", 53, PPC_OPERAND_VSR },
  { "f54", 54, PPC_OPERAND_VSR },
  { "f55", 55, PPC_OPERAND_VSR },
  { "f56", 56, PPC_OPERAND_VSR },
  { "f57", 57, PPC_OPERAND_VSR },
  { "f58", 58, PPC_OPERAND_VSR },
  { "f59", 59, PPC_OPERAND_VSR },
  { "f6", 6, PPC_OPERAND_FPR },
  { "f60", 60, PPC_OPERAND_VSR },
  { "f61", 61, PPC_OPERAND_VSR },
  { "f62", 62, PPC_OPERAND_VSR },
  { "f63", 63, PPC_OPERAND_VSR },
  { "f7", 7, PPC_OPERAND_FPR },
  { "f8", 8, PPC_OPERAND_FPR },
  { "f9", 9, PPC_OPERAND_FPR },

  /* Quantization registers used with pair single instructions.  */
  { "gqr.0", 0, PPC_OPERAND_GQR },
  { "gqr.1", 1, PPC_OPERAND_GQR },
  { "gqr.2", 2, PPC_OPERAND_GQR },
  { "gqr.3", 3, PPC_OPERAND_GQR },
  { "gqr.4", 4, PPC_OPERAND_GQR },
  { "gqr.5", 5, PPC_OPERAND_GQR },
  { "gqr.6", 6, PPC_OPERAND_GQR },
  { "gqr.7", 7, PPC_OPERAND_GQR },
  { "gqr0", 0, PPC_OPERAND_GQR },
  { "gqr1", 1, PPC_OPERAND_GQR },
  { "gqr2", 2, PPC_OPERAND_GQR },
  { "gqr3", 3, PPC_OPERAND_GQR },
  { "gqr4", 4, PPC_OPERAND_GQR },
  { "gqr5", 5, PPC_OPERAND_GQR },
  { "gqr6", 6, PPC_OPERAND_GQR },
  { "gqr7", 7, PPC_OPERAND_GQR },

  { "lr", 8, PPC_OPERAND_SPR },

  /* General Purpose Registers */
  { "r.0", 0, PPC_OPERAND_GPR },
  { "r.1", 1, PPC_OPERAND_GPR },
  { "r.10", 10, PPC_OPERAND_GPR },
  { "r.11", 11, PPC_OPERAND_GPR },
  { "r.12", 12, PPC_OPERAND_GPR },
  { "r.13", 13, PPC_OPERAND_GPR },
  { "r.14", 14, PPC_OPERAND_GPR },
  { "r.15", 15, PPC_OPERAND_GPR },
  { "r.16", 16, PPC_OPERAND_GPR },
  { "r.17", 17, PPC_OPERAND_GPR },
  { "r.18", 18, PPC_OPERAND_GPR },
  { "r.19", 19, PPC_OPERAND_GPR },
  { "r.2", 2, PPC_OPERAND_GPR },
  { "r.20", 20, PPC_OPERAND_GPR },
  { "r.21", 21, PPC_OPERAND_GPR },
  { "r.22", 22, PPC_OPERAND_GPR },
  { "r.23", 23, PPC_OPERAND_GPR },
  { "r.24", 24, PPC_OPERAND_GPR },
  { "r.25", 25, PPC_OPERAND_GPR },
  { "r.26", 26, PPC_OPERAND_GPR },
  { "r.27", 27, PPC_OPERAND_GPR },
  { "r.28", 28, PPC_OPERAND_GPR },
  { "r.29", 29, PPC_OPERAND_GPR },
  { "r.3", 3, PPC_OPERAND_GPR },
  { "r.30", 30, PPC_OPERAND_GPR },
  { "r.31", 31, PPC_OPERAND_GPR },
  { "r.4", 4, PPC_OPERAND_GPR },
  { "r.5", 5, PPC_OPERAND_GPR },
  { "r.6", 6, PPC_OPERAND_GPR },
  { "r.7", 7, PPC_OPERAND_GPR },
  { "r.8", 8, PPC_OPERAND_GPR },
  { "r.9", 9, PPC_OPERAND_GPR },

  { "r.sp", 1, PPC_OPERAND_GPR },

  { "r.toc", 2, PPC_OPERAND_GPR },

  { "r0", 0, PPC_OPERAND_GPR },
  { "r1", 1, PPC_OPERAND_GPR },
  { "r10", 10, PPC_OPERAND_GPR },
  { "r11", 11, PPC_OPERAND_GPR },
  { "r12", 12, PPC_OPERAND_GPR },
  { "r13", 13, PPC_OPERAND_GPR },
  { "r14", 14, PPC_OPERAND_GPR },
  { "r15", 15, PPC_OPERAND_GPR },
  { "r16", 16, PPC_OPERAND_GPR },
  { "r17", 17, PPC_OPERAND_GPR },
  { "r18", 18, PPC_OPERAND_GPR },
  { "r19", 19, PPC_OPERAND_GPR },
  { "r2", 2, PPC_OPERAND_GPR },
  { "r20", 20, PPC_OPERAND_GPR },
  { "r21", 21, PPC_OPERAND_GPR },
  { "r22", 22, PPC_OPERAND_GPR },
  { "r23", 23, PPC_OPERAND_GPR },
  { "r24", 24, PPC_OPERAND_GPR },
  { "r25", 25, PPC_OPERAND_GPR },
  { "r26", 26, PPC_OPERAND_GPR },
  { "r27", 27, PPC_OPERAND_GPR },
  { "r28", 28, PPC_OPERAND_GPR },
  { "r29", 29, PPC_OPERAND_GPR },
  { "r3", 3, PPC_OPERAND_GPR },
  { "r30", 30, PPC_OPERAND_GPR },
  { "r31", 31, PPC_OPERAND_GPR },
  { "r4", 4, PPC_OPERAND_GPR },
  { "r5", 5, PPC_OPERAND_GPR },
  { "r6", 6, PPC_OPERAND_GPR },
  { "r7", 7, PPC_OPERAND_GPR },
  { "r8", 8, PPC_OPERAND_GPR },
  { "r9", 9, PPC_OPERAND_GPR },

  { "rtoc", 2, PPC_OPERAND_GPR },

  { "sdr1", 25, PPC_OPERAND_SPR },

  { "sp", 1, PPC_OPERAND_GPR },

  { "srr0", 26, PPC_OPERAND_SPR },
  { "srr1", 27, PPC_OPERAND_SPR },

  /* Vector (Altivec/VMX) registers */
  { "v.0", 0, PPC_OPERAND_VR },
  { "v.1", 1, PPC_OPERAND_VR },
  { "v.10", 10, PPC_OPERAND_VR },
  { "v.11", 11, PPC_OPERAND_VR },
  { "v.12", 12, PPC_OPERAND_VR },
  { "v.13", 13, PPC_OPERAND_VR },
  { "v.14", 14, PPC_OPERAND_VR },
  { "v.15", 15, PPC_OPERAND_VR },
  { "v.16", 16, PPC_OPERAND_VR },
  { "v.17", 17, PPC_OPERAND_VR },
  { "v.18", 18, PPC_OPERAND_VR },
  { "v.19", 19, PPC_OPERAND_VR },
  { "v.2", 2, PPC_OPERAND_VR },
  { "v.20", 20, PPC_OPERAND_VR },
  { "v.21", 21, PPC_OPERAND_VR },
  { "v.22", 22, PPC_OPERAND_VR },
  { "v.23", 23, PPC_OPERAND_VR },
  { "v.24", 24, PPC_OPERAND_VR },
  { "v.25", 25, PPC_OPERAND_VR },
  { "v.26", 26, PPC_OPERAND_VR },
  { "v.27", 27, PPC_OPERAND_VR },
  { "v.28", 28, PPC_OPERAND_VR },
  { "v.29", 29, PPC_OPERAND_VR },
  { "v.3", 3, PPC_OPERAND_VR },
  { "v.30", 30, PPC_OPERAND_VR },
  { "v.31", 31, PPC_OPERAND_VR },
  { "v.4", 4, PPC_OPERAND_VR },
  { "v.5", 5, PPC_OPERAND_VR },
  { "v.6", 6, PPC_OPERAND_VR },
  { "v.7", 7, PPC_OPERAND_VR },
  { "v.8", 8, PPC_OPERAND_VR },
  { "v.9", 9, PPC_OPERAND_VR },

  { "v0", 0, PPC_OPERAND_VR },
  { "v1", 1, PPC_OPERAND_VR },
  { "v10", 10, PPC_OPERAND_VR },
  { "v11", 11, PPC_OPERAND_VR },
  { "v12", 12, PPC_OPERAND_VR },
  { "v13", 13, PPC_OPERAND_VR },
  { "v14", 14, PPC_OPERAND_VR },
  { "v15", 15, PPC_OPERAND_VR },
  { "v16", 16, PPC_OPERAND_VR },
  { "v17", 17, PPC_OPERAND_VR },
  { "v18", 18, PPC_OPERAND_VR },
  { "v19", 19, PPC_OPERAND_VR },
  { "v2", 2, PPC_OPERAND_VR },
  { "v20", 20, PPC_OPERAND_VR },
  { "v21", 21, PPC_OPERAND_VR },
  { "v22", 22, PPC_OPERAND_VR },
  { "v23", 23, PPC_OPERAND_VR },
  { "v24", 24, PPC_OPERAND_VR },
  { "v25", 25, PPC_OPERAND_VR },
  { "v26", 26, PPC_OPERAND_VR },
  { "v27", 27, PPC_OPERAND_VR },
  { "v28", 28, PPC_OPERAND_VR },
  { "v29", 29, PPC_OPERAND_VR },
  { "v3", 3, PPC_OPERAND_VR },
  { "v30", 30, PPC_OPERAND_VR },
  { "v31", 31, PPC_OPERAND_VR },
  { "v4", 4, PPC_OPERAND_VR },
  { "v5", 5, PPC_OPERAND_VR },
  { "v6", 6, PPC_OPERAND_VR },
  { "v7", 7, PPC_OPERAND_VR },
  { "v8", 8, PPC_OPERAND_VR },
  { "v9", 9, PPC_OPERAND_VR },

  /* Vector Scalar (VSX) registers (ISA 2.06).  */
  { "vs.0", 0, PPC_OPERAND_VSR },
  { "vs.1", 1, PPC_OPERAND_VSR },
  { "vs.10", 10, PPC_OPERAND_VSR },
  { "vs.11", 11, PPC_OPERAND_VSR },
  { "vs.12", 12, PPC_OPERAND_VSR },
  { "vs.13", 13, PPC_OPERAND_VSR },
  { "vs.14", 14, PPC_OPERAND_VSR },
  { "vs.15", 15, PPC_OPERAND_VSR },
  { "vs.16", 16, PPC_OPERAND_VSR },
  { "vs.17", 17, PPC_OPERAND_VSR },
  { "vs.18", 18, PPC_OPERAND_VSR },
  { "vs.19", 19, PPC_OPERAND_VSR },
  { "vs.2", 2, PPC_OPERAND_VSR },
  { "vs.20", 20, PPC_OPERAND_VSR },
  { "vs.21", 21, PPC_OPERAND_VSR },
  { "vs.22", 22, PPC_OPERAND_VSR },
  { "vs.23", 23, PPC_OPERAND_VSR },
  { "vs.24", 24, PPC_OPERAND_VSR },
  { "vs.25", 25, PPC_OPERAND_VSR },
  { "vs.26", 26, PPC_OPERAND_VSR },
  { "vs.27", 27, PPC_OPERAND_VSR },
  { "vs.28", 28, PPC_OPERAND_VSR },
  { "vs.29", 29, PPC_OPERAND_VSR },
  { "vs.3", 3, PPC_OPERAND_VSR },
  { "vs.30", 30, PPC_OPERAND_VSR },
  { "vs.31", 31, PPC_OPERAND_VSR },
  { "vs.32", 32, PPC_OPERAND_VSR },
  { "vs.33", 33, PPC_OPERAND_VSR },
  { "vs.34", 34, PPC_OPERAND_VSR },
  { "vs.35", 35, PPC_OPERAND_VSR },
  { "vs.36", 36, PPC_OPERAND_VSR },
  { "vs.37", 37, PPC_OPERAND_VSR },
  { "vs.38", 38, PPC_OPERAND_VSR },
  { "vs.39", 39, PPC_OPERAND_VSR },
  { "vs.4", 4, PPC_OPERAND_VSR },
  { "vs.40", 40, PPC_OPERAND_VSR },
  { "vs.41", 41, PPC_OPERAND_VSR },
  { "vs.42", 42, PPC_OPERAND_VSR },
  { "vs.43", 43, PPC_OPERAND_VSR },
  { "vs.44", 44, PPC_OPERAND_VSR },
  { "vs.45", 45, PPC_OPERAND_VSR },
  { "vs.46", 46, PPC_OPERAND_VSR },
  { "vs.47", 47, PPC_OPERAND_VSR },
  { "vs.48", 48, PPC_OPERAND_VSR },
  { "vs.49", 49, PPC_OPERAND_VSR },
  { "vs.5", 5, PPC_OPERAND_VSR },
  { "vs.50", 50, PPC_OPERAND_VSR },
  { "vs.51", 51, PPC_OPERAND_VSR },
  { "vs.52", 52, PPC_OPERAND_VSR },
  { "vs.53", 53, PPC_OPERAND_VSR },
  { "vs.54", 54, PPC_OPERAND_VSR },
  { "vs.55", 55, PPC_OPERAND_VSR },
  { "vs.56", 56, PPC_OPERAND_VSR },
  { "vs.57", 57, PPC_OPERAND_VSR },
  { "vs.58", 58, PPC_OPERAND_VSR },
  { "vs.59", 59, PPC_OPERAND_VSR },
  { "vs.6", 6, PPC_OPERAND_VSR },
  { "vs.60", 60, PPC_OPERAND_VSR },
  { "vs.61", 61, PPC_OPERAND_VSR },
  { "vs.62", 62, PPC_OPERAND_VSR },
  { "vs.63", 63, PPC_OPERAND_VSR },
  { "vs.7", 7, PPC_OPERAND_VSR },
  { "vs.8", 8, PPC_OPERAND_VSR },
  { "vs.9", 9, PPC_OPERAND_VSR },

  { "vs0", 0, PPC_OPERAND_VSR },
  { "vs1", 1, PPC_OPERAND_VSR },
  { "vs10", 10, PPC_OPERAND_VSR },
  { "vs11", 11, PPC_OPERAND_VSR },
  { "vs12", 12, PPC_OPERAND_VSR },
  { "vs13", 13, PPC_OPERAND_VSR },
  { "vs14", 14, PPC_OPERAND_VSR },
  { "vs15", 15, PPC_OPERAND_VSR },
  { "vs16", 16, PPC_OPERAND_VSR },
  { "vs17", 17, PPC_OPERAND_VSR },
  { "vs18", 18, PPC_OPERAND_VSR },
  { "vs19", 19, PPC_OPERAND_VSR },
  { "vs2", 2, PPC_OPERAND_VSR },
  { "vs20", 20, PPC_OPERAND_VSR },
  { "vs21", 21, PPC_OPERAND_VSR },
  { "vs22", 22, PPC_OPERAND_VSR },
  { "vs23", 23, PPC_OPERAND_VSR },
  { "vs24", 24, PPC_OPERAND_VSR },
  { "vs25", 25, PPC_OPERAND_VSR },
  { "vs26", 26, PPC_OPERAND_VSR },
  { "vs27", 27, PPC_OPERAND_VSR },
  { "vs28", 28, PPC_OPERAND_VSR },
  { "vs29", 29, PPC_OPERAND_VSR },
  { "vs3", 3, PPC_OPERAND_VSR },
  { "vs30", 30, PPC_OPERAND_VSR },
  { "vs31", 31, PPC_OPERAND_VSR },
  { "vs32", 32, PPC_OPERAND_VSR },
  { "vs33", 33, PPC_OPERAND_VSR },
  { "vs34", 34, PPC_OPERAND_VSR },
  { "vs35", 35, PPC_OPERAND_VSR },
  { "vs36", 36, PPC_OPERAND_VSR },
  { "vs37", 37, PPC_OPERAND_VSR },
  { "vs38", 38, PPC_OPERAND_VSR },
  { "vs39", 39, PPC_OPERAND_VSR },
  { "vs4", 4, PPC_OPERAND_VSR },
  { "vs40", 40, PPC_OPERAND_VSR },
  { "vs41", 41, PPC_OPERAND_VSR },
  { "vs42", 42, PPC_OPERAND_VSR },
  { "vs43", 43, PPC_OPERAND_VSR },
  { "vs44", 44, PPC_OPERAND_VSR },
  { "vs45", 45, PPC_OPERAND_VSR },
  { "vs46", 46, PPC_OPERAND_VSR },
  { "vs47", 47, PPC_OPERAND_VSR },
  { "vs48", 48, PPC_OPERAND_VSR },
  { "vs49", 49, PPC_OPERAND_VSR },
  { "vs5", 5, PPC_OPERAND_VSR },
  { "vs50", 50, PPC_OPERAND_VSR },
  { "vs51", 51, PPC_OPERAND_VSR },
  { "vs52", 52, PPC_OPERAND_VSR },
  { "vs53", 53, PPC_OPERAND_VSR },
  { "vs54", 54, PPC_OPERAND_VSR },
  { "vs55", 55, PPC_OPERAND_VSR },
  { "vs56", 56, PPC_OPERAND_VSR },
  { "vs57", 57, PPC_OPERAND_VSR },
  { "vs58", 58, PPC_OPERAND_VSR },
  { "vs59", 59, PPC_OPERAND_VSR },
  { "vs6", 6, PPC_OPERAND_VSR },
  { "vs60", 60, PPC_OPERAND_VSR },
  { "vs61", 61, PPC_OPERAND_VSR },
  { "vs62", 62, PPC_OPERAND_VSR },
  { "vs63", 63, PPC_OPERAND_VSR },
  { "vs7", 7, PPC_OPERAND_VSR },
  { "vs8", 8, PPC_OPERAND_VSR },
  { "vs9", 9, PPC_OPERAND_VSR },

  { "xer", 1, PPC_OPERAND_SPR }
};

#define REG_NAME_CNT	(sizeof (pre_defined_registers) / sizeof (struct pd_reg))

/* Given NAME, find the register number associated with that name, return
   the integer value associated with the given name or -1 on failure.  */

static const struct pd_reg *
reg_name_search (const struct pd_reg *regs, int regcount, const char *name)
{
  int middle, low, high;
  int cmp;

  low = 0;
  high = regcount - 1;

  do
    {
      middle = (low + high) / 2;
      cmp = strcasecmp (name, regs[middle].name);
      if (cmp < 0)
	high = middle - 1;
      else if (cmp > 0)
	low = middle + 1;
      else
	return &regs[middle];
    }
  while (low <= high);

  return NULL;
}

/*
 * Summary of register_name.
 *
 * in:	Input_line_pointer points to 1st char of operand.
 *
 * out:	A expressionS.
 *      The operand may have been a register: in this case, X_op == O_register,
 *      X_add_number is set to the register number, and truth is returned.
 *	Input_line_pointer->(next non-blank) char after operand, or is in its
 *      original state.
 */

static bfd_boolean
register_name (expressionS *expressionP)
{
  const struct pd_reg *reg;
  char *name;
  char *start;
  char c;

  /* Find the spelling of the operand.  */
  start = name = input_line_pointer;
  if (name[0] == '%' && ISALPHA (name[1]))
    name = ++input_line_pointer;

  else if (!reg_names_p || !ISALPHA (name[0]))
    return FALSE;

  c = get_symbol_name (&name);
  reg = reg_name_search (pre_defined_registers, REG_NAME_CNT, name);

  /* Put back the delimiting char.  */
  *input_line_pointer = c;

  /* Look to see if it's in the register table.  */
  if (reg != NULL)
    {
      expressionP->X_op = O_register;
      expressionP->X_add_number = reg->value;
      expressionP->X_md = reg->flags;

      /* Make the rest nice.  */
      expressionP->X_add_symbol = NULL;
      expressionP->X_op_symbol = NULL;
      return TRUE;
    }

  /* Reset the line as if we had not done anything.  */
  input_line_pointer = start;
  return FALSE;
}

/* This function is called for each symbol seen in an expression.  It
   handles the special parsing which PowerPC assemblers are supposed
   to use for condition codes.  */

/* Whether to do the special parsing.  */
static bfd_boolean cr_operand;

/* Names to recognize in a condition code.  This table is sorted.  */
static const struct pd_reg cr_names[] =
{
  { "cr0", 0, PPC_OPERAND_CR_REG },
  { "cr1", 1, PPC_OPERAND_CR_REG },
  { "cr2", 2, PPC_OPERAND_CR_REG },
  { "cr3", 3, PPC_OPERAND_CR_REG },
  { "cr4", 4, PPC_OPERAND_CR_REG },
  { "cr5", 5, PPC_OPERAND_CR_REG },
  { "cr6", 6, PPC_OPERAND_CR_REG },
  { "cr7", 7, PPC_OPERAND_CR_REG },
  { "eq", 2, PPC_OPERAND_CR_BIT },
  { "gt", 1, PPC_OPERAND_CR_BIT },
  { "lt", 0, PPC_OPERAND_CR_BIT },
  { "so", 3, PPC_OPERAND_CR_BIT },
  { "un", 3, PPC_OPERAND_CR_BIT }
};

/* Parsing function.  This returns non-zero if it recognized an
   expression.  */

int
ppc_parse_name (const char *name, expressionS *exp)
{
  const struct pd_reg *reg;

  if (! cr_operand)
    return 0;

  if (*name == '%')
    ++name;
  reg = reg_name_search (cr_names, sizeof cr_names / sizeof cr_names[0],
			 name);
  if (reg == NULL)
    return 0;

  exp->X_op = O_register;
  exp->X_add_number = reg->value;
  exp->X_md = reg->flags;

  return 1;
}

/* Propagate X_md and check register expressions.  This is to support
   condition codes like 4*cr5+eq.  */

int
ppc_optimize_expr (expressionS *left, operatorT op, expressionS *right)
{
  /* Accept 4*cr<n> and cr<n>*4.  */
  if (op == O_multiply
      && ((right->X_op == O_register
	   && right->X_md == PPC_OPERAND_CR_REG
	   && left->X_op == O_constant
	   && left->X_add_number == 4)
	  || (left->X_op == O_register
	      && left->X_md == PPC_OPERAND_CR_REG
	      && right->X_op == O_constant
	      && right->X_add_number == 4)))
    {
      left->X_op = O_register;
      left->X_md = PPC_OPERAND_CR_REG | PPC_OPERAND_CR_BIT;
      left->X_add_number *= right->X_add_number;
      return 1;
    }

  /* Accept the above plus <cr bit>, and <cr bit> plus the above.  */
  if (right->X_op == O_register
      && left->X_op == O_register
      && op == O_add
      && ((right->X_md == PPC_OPERAND_CR_BIT
	   && left->X_md == (PPC_OPERAND_CR_REG | PPC_OPERAND_CR_BIT))
	  || (right->X_md == (PPC_OPERAND_CR_REG | PPC_OPERAND_CR_BIT)
	      && left->X_md == PPC_OPERAND_CR_BIT)))
    {
      left->X_md = PPC_OPERAND_CR_BIT;
      right->X_op = O_constant;
      return 0;
    }

  /* Accept reg +/- constant.  */
  if (left->X_op == O_register
      && !((op == O_add || op == O_subtract) && right->X_op == O_constant))
    as_warn (_("invalid register expression"));

  /* Accept constant + reg.  */
  if (right->X_op == O_register)
    {
      if (op == O_add && left->X_op == O_constant)
	left->X_md = right->X_md;
      else
	as_warn (_("invalid register expression"));
    }

  return 0;
}

/* Local variables.  */

/* Whether to target xcoff64/elf64.  */
static unsigned int ppc_obj64 = BFD_DEFAULT_TARGET_SIZE == 64;

/* Opcode hash table.  */
static struct hash_control *ppc_hash;

/* Macro hash table.  */
static struct hash_control *ppc_macro_hash;

#ifdef OBJ_ELF
/* What type of shared library support to use.  */
static enum { SHLIB_NONE, SHLIB_PIC, SHLIB_MRELOCATABLE } shlib = SHLIB_NONE;

/* Flags to set in the elf header.  */
static flagword ppc_flags = 0;

/* Whether this is Solaris or not.  */
#ifdef TARGET_SOLARIS_COMMENT
#define SOLARIS_P TRUE
#else
#define SOLARIS_P FALSE
#endif

static bfd_boolean msolaris = SOLARIS_P;
#endif

#ifdef OBJ_XCOFF

/* The RS/6000 assembler uses the .csect pseudo-op to generate code
   using a bunch of different sections.  These assembler sections,
   however, are all encompassed within the .text or .data sections of
   the final output file.  We handle this by using different
   subsegments within these main segments.  */

/* Next subsegment to allocate within the .text segment.  */
static subsegT ppc_text_subsegment = 2;

/* Linked list of csects in the text section.  */
static symbolS *ppc_text_csects;

/* Next subsegment to allocate within the .data segment.  */
static subsegT ppc_data_subsegment = 2;

/* Linked list of csects in the data section.  */
static symbolS *ppc_data_csects;

/* The current csect.  */
static symbolS *ppc_current_csect;

/* The RS/6000 assembler uses a TOC which holds addresses of functions
   and variables.  Symbols are put in the TOC with the .tc pseudo-op.
   A special relocation is used when accessing TOC entries.  We handle
   the TOC as a subsegment within the .data segment.  We set it up if
   we see a .toc pseudo-op, and save the csect symbol here.  */
static symbolS *ppc_toc_csect;

/* The first frag in the TOC subsegment.  */
static fragS *ppc_toc_frag;

/* The first frag in the first subsegment after the TOC in the .data
   segment.  NULL if there are no subsegments after the TOC.  */
static fragS *ppc_after_toc_frag;

/* The current static block.  */
static symbolS *ppc_current_block;

/* The COFF debugging section; set by md_begin.  This is not the
   .debug section, but is instead the secret BFD section which will
   cause BFD to set the section number of a symbol to N_DEBUG.  */
static asection *ppc_coff_debug_section;

/* Structure to set the length field of the dwarf sections.  */
struct dw_subsection {
  /* Subsections are simply linked.  */
  struct dw_subsection *link;

  /* The subsection number.  */
  subsegT subseg;

  /* Expression to compute the length of the section.  */
  expressionS end_exp;
};

static struct dw_section {
  /* Corresponding section.  */
  segT sect;

  /* Simply linked list of subsections with a label.  */
  struct dw_subsection *list_subseg;

  /* The anonymous subsection.  */
  struct dw_subsection *anon_subseg;
} dw_sections[XCOFF_DWSECT_NBR_NAMES];
#endif /* OBJ_XCOFF */

#ifdef TE_PE

/* Various sections that we need for PE coff support.  */
static segT ydata_section;
static segT pdata_section;
static segT reldata_section;
static segT rdata_section;
static segT tocdata_section;

/* The current section and the previous section. See ppc_previous.  */
static segT ppc_previous_section;
static segT ppc_current_section;

#endif /* TE_PE */

#ifdef OBJ_ELF
symbolS *GOT_symbol;		/* Pre-defined "_GLOBAL_OFFSET_TABLE" */
unsigned long *ppc_apuinfo_list;
unsigned int ppc_apuinfo_num;
unsigned int ppc_apuinfo_num_alloc;
#endif /* OBJ_ELF */

#ifdef OBJ_ELF
const char *const md_shortopts = "b:l:usm:K:VQ:";
#else
const char *const md_shortopts = "um:";
#endif
#define OPTION_NOPS (OPTION_MD_BASE + 0)
const struct option md_longopts[] = {
  {"nops", required_argument, NULL, OPTION_NOPS},
  {"ppc476-workaround", no_argument, &warn_476, 1},
  {"no-ppc476-workaround", no_argument, &warn_476, 0},
  {NULL, no_argument, NULL, 0}
};
const size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c, const char *arg)
{
  ppc_cpu_t new_cpu;

  switch (c)
    {
    case 'u':
      /* -u means that any undefined symbols should be treated as
	 external, which is the default for gas anyhow.  */
      break;

#ifdef OBJ_ELF
    case 'l':
      /* Solaris as takes -le (presumably for little endian).  For completeness
	 sake, recognize -be also.  */
      if (strcmp (arg, "e") == 0)
	{
	  target_big_endian = 0;
	  set_target_endian = 1;
	  if (ppc_cpu & PPC_OPCODE_VLE)
	    as_bad (_("the use of -mvle requires big endian."));
	}
      else
	return 0;

      break;

    case 'b':
      if (strcmp (arg, "e") == 0)
	{
	  target_big_endian = 1;
	  set_target_endian = 1;
	}
      else
	return 0;

      break;

    case 'K':
      /* Recognize -K PIC.  */
      if (strcmp (arg, "PIC") == 0 || strcmp (arg, "pic") == 0)
	{
	  shlib = SHLIB_PIC;
	  ppc_flags |= EF_PPC_RELOCATABLE_LIB;
	}
      else
	return 0;

      break;
#endif

      /* a64 and a32 determine whether to use XCOFF64 or XCOFF32.  */
    case 'a':
      if (strcmp (arg, "64") == 0)
	{
#ifdef BFD64
	  ppc_obj64 = 1;
	  if (ppc_cpu & PPC_OPCODE_VLE)
	    as_bad (_("the use of -mvle requires -a32."));
#else
	  as_fatal (_("%s unsupported"), "-a64");
#endif
	}
      else if (strcmp (arg, "32") == 0)
	ppc_obj64 = 0;
      else
	return 0;
      break;

    case 'm':
      new_cpu = ppc_parse_cpu (ppc_cpu, &sticky, arg);
      /* "raw" is only valid for the disassembler.  */
      if (new_cpu != 0 && (new_cpu & PPC_OPCODE_RAW) == 0)
	{
	  ppc_cpu = new_cpu;
	  if (strcmp (arg, "vle") == 0)
	    {
	      if (set_target_endian && target_big_endian == 0)
		as_bad (_("the use of -mvle requires big endian."));
	      if (ppc_obj64)
		as_bad (_("the use of -mvle requires -a32."));
	    }
	}

      else if (strcmp (arg, "no-vle") == 0)
	{
	  sticky &= ~PPC_OPCODE_VLE;

	  new_cpu = ppc_parse_cpu (ppc_cpu, &sticky, "booke");
	  new_cpu &= ~PPC_OPCODE_VLE;

	  ppc_cpu = new_cpu;
	}

      else if (strcmp (arg, "regnames") == 0)
	reg_names_p = TRUE;

      else if (strcmp (arg, "no-regnames") == 0)
	reg_names_p = FALSE;

#ifdef OBJ_ELF
      /* -mrelocatable/-mrelocatable-lib -- warn about initializations
	 that require relocation.  */
      else if (strcmp (arg, "relocatable") == 0)
	{
	  shlib = SHLIB_MRELOCATABLE;
	  ppc_flags |= EF_PPC_RELOCATABLE;
	}

      else if (strcmp (arg, "relocatable-lib") == 0)
	{
	  shlib = SHLIB_MRELOCATABLE;
	  ppc_flags |= EF_PPC_RELOCATABLE_LIB;
	}

      /* -memb, set embedded bit.  */
      else if (strcmp (arg, "emb") == 0)
	ppc_flags |= EF_PPC_EMB;

      /* -mlittle/-mbig set the endianness.  */
      else if (strcmp (arg, "little") == 0
	       || strcmp (arg, "little-endian") == 0)
	{
	  target_big_endian = 0;
	  set_target_endian = 1;
	  if (ppc_cpu & PPC_OPCODE_VLE)
	    as_bad (_("the use of -mvle requires big endian."));
	}

      else if (strcmp (arg, "big") == 0 || strcmp (arg, "big-endian") == 0)
	{
	  target_big_endian = 1;
	  set_target_endian = 1;
	}

      else if (strcmp (arg, "solaris") == 0)
	{
	  msolaris = TRUE;
	  ppc_comment_chars = ppc_solaris_comment_chars;
	}

      else if (strcmp (arg, "no-solaris") == 0)
	{
	  msolaris = FALSE;
	  ppc_comment_chars = ppc_eabi_comment_chars;
	}
      else if (strcmp (arg, "spe2") == 0)
	{
	  ppc_cpu |= PPC_OPCODE_SPE2;
	}
#endif
      else
	{
	  as_bad (_("invalid switch -m%s"), arg);
	  return 0;
	}
      break;

#ifdef OBJ_ELF
      /* -V: SVR4 argument to print version ID.  */
    case 'V':
      print_version_id ();
      break;

      /* -Qy, -Qn: SVR4 arguments controlling whether a .comment section
	 should be emitted or not.  FIXME: Not implemented.  */
    case 'Q':
      break;

      /* Solaris takes -s to specify that .stabs go in a .stabs section,
	 rather than .stabs.excl, which is ignored by the linker.
	 FIXME: Not implemented.  */
    case 's':
      if (arg)
	return 0;

      break;
#endif

    case OPTION_NOPS:
      {
	char *end;
	nop_limit = strtoul (optarg, &end, 0);
	if (*end)
	  as_bad (_("--nops needs a numeric argument"));
      }
      break;

    case 0:
      break;

    default:
      return 0;
    }

  return 1;
}

static int
is_ppc64_target (const bfd_target *targ, void *data ATTRIBUTE_UNUSED)
{
  switch (targ->flavour)
    {
#ifdef OBJ_ELF
    case bfd_target_elf_flavour:
      return strncmp (targ->name, "elf64-powerpc", 13) == 0;
#endif
#ifdef OBJ_XCOFF
    case bfd_target_xcoff_flavour:
      return (strcmp (targ->name, "aixcoff64-rs6000") == 0
	      || strcmp (targ->name, "aix5coff64-rs6000") == 0);
#endif
    default:
      return 0;
    }
}

void
md_show_usage (FILE *stream)
{
  fprintf (stream, _("\
PowerPC options:\n"));
  fprintf (stream, _("\
-a32                    generate ELF32/XCOFF32\n"));
  if (bfd_iterate_over_targets (is_ppc64_target, NULL))
    fprintf (stream, _("\
-a64                    generate ELF64/XCOFF64\n"));
  fprintf (stream, _("\
-u                      ignored\n"));
  fprintf (stream, _("\
-mpwrx, -mpwr2          generate code for POWER/2 (RIOS2)\n"));
  fprintf (stream, _("\
-mpwr                   generate code for POWER (RIOS1)\n"));
  fprintf (stream, _("\
-m601                   generate code for PowerPC 601\n"));
  fprintf (stream, _("\
-mppc, -mppc32, -m603, -m604\n\
                        generate code for PowerPC 603/604\n"));
  fprintf (stream, _("\
-m403                   generate code for PowerPC 403\n"));
  fprintf (stream, _("\
-m405                   generate code for PowerPC 405\n"));
  fprintf (stream, _("\
-m440                   generate code for PowerPC 440\n"));
  fprintf (stream, _("\
-m464                   generate code for PowerPC 464\n"));
  fprintf (stream, _("\
-m476                   generate code for PowerPC 476\n"));
  fprintf (stream, _("\
-m7400, -m7410, -m7450, -m7455\n\
                        generate code for PowerPC 7400/7410/7450/7455\n"));
  fprintf (stream, _("\
-m750cl, -mgekko, -mbroadway\n\
                        generate code for PowerPC 750cl/Gekko/Broadway\n"));
  fprintf (stream, _("\
-m821, -m850, -m860     generate code for PowerPC 821/850/860\n"));
  fprintf (stream, _("\
-mppc64, -m620          generate code for PowerPC 620/625/630\n"));
  fprintf (stream, _("\
-mppc64bridge           generate code for PowerPC 64, including bridge insns\n"));
  fprintf (stream, _("\
-mbooke                 generate code for 32-bit PowerPC BookE\n"));
  fprintf (stream, _("\
-ma2                    generate code for A2 architecture\n"));
  fprintf (stream, _("\
-mpower4, -mpwr4        generate code for Power4 architecture\n"));
  fprintf (stream, _("\
-mpower5, -mpwr5, -mpwr5x\n\
                        generate code for Power5 architecture\n"));
  fprintf (stream, _("\
-mpower6, -mpwr6        generate code for Power6 architecture\n"));
  fprintf (stream, _("\
-mpower7, -mpwr7        generate code for Power7 architecture\n"));
  fprintf (stream, _("\
-mpower8, -mpwr8        generate code for Power8 architecture\n"));
  fprintf (stream, _("\
-mpower9, -mpwr9        generate code for Power9 architecture\n"));
  fprintf (stream, _("\
-mcell                  generate code for Cell Broadband Engine architecture\n"));
  fprintf (stream, _("\
-mcom                   generate code for Power/PowerPC common instructions\n"));
  fprintf (stream, _("\
-many                   generate code for any architecture (PWR/PWRX/PPC)\n"));
  fprintf (stream, _("\
-maltivec               generate code for AltiVec\n"));
  fprintf (stream, _("\
-mvsx                   generate code for Vector-Scalar (VSX) instructions\n"));
  fprintf (stream, _("\
-me300                  generate code for PowerPC e300 family\n"));
  fprintf (stream, _("\
-me500, -me500x2        generate code for Motorola e500 core complex\n"));
  fprintf (stream, _("\
-me500mc,               generate code for Freescale e500mc core complex\n"));
  fprintf (stream, _("\
-me500mc64,             generate code for Freescale e500mc64 core complex\n"));
  fprintf (stream, _("\
-me5500,                generate code for Freescale e5500 core complex\n"));
  fprintf (stream, _("\
-me6500,                generate code for Freescale e6500 core complex\n"));
  fprintf (stream, _("\
-mspe                   generate code for Motorola SPE instructions\n"));
  fprintf (stream, _("\
-mspe2                  generate code for Freescale SPE2 instructions\n"));
  fprintf (stream, _("\
-mvle                   generate code for Freescale VLE instructions\n"));
  fprintf (stream, _("\
-mtitan                 generate code for AppliedMicro Titan core complex\n"));
  fprintf (stream, _("\
-mregnames              Allow symbolic names for registers\n"));
  fprintf (stream, _("\
-mno-regnames           Do not allow symbolic names for registers\n"));
#ifdef OBJ_ELF
  fprintf (stream, _("\
-mrelocatable           support for GCC's -mrelocatble option\n"));
  fprintf (stream, _("\
-mrelocatable-lib       support for GCC's -mrelocatble-lib option\n"));
  fprintf (stream, _("\
-memb                   set PPC_EMB bit in ELF flags\n"));
  fprintf (stream, _("\
-mlittle, -mlittle-endian, -le\n\
                        generate code for a little endian machine\n"));
  fprintf (stream, _("\
-mbig, -mbig-endian, -be\n\
                        generate code for a big endian machine\n"));
  fprintf (stream, _("\
-msolaris               generate code for Solaris\n"));
  fprintf (stream, _("\
-mno-solaris            do not generate code for Solaris\n"));
  fprintf (stream, _("\
-K PIC                  set EF_PPC_RELOCATABLE_LIB in ELF flags\n"));
  fprintf (stream, _("\
-V                      print assembler version number\n"));
  fprintf (stream, _("\
-Qy, -Qn                ignored\n"));
#endif
  fprintf (stream, _("\
-nops=count             when aligning, more than COUNT nops uses a branch\n"));
  fprintf (stream, _("\
-ppc476-workaround      warn if emitting data to code sections\n"));
}

/* Set ppc_cpu if it is not already set.  */

static void
ppc_set_cpu (void)
{
  const char *default_os  = TARGET_OS;
  const char *default_cpu = TARGET_CPU;

  if ((ppc_cpu & ~(ppc_cpu_t) PPC_OPCODE_ANY) == 0)
    {
      if (ppc_obj64)
	if (target_big_endian)
	  ppc_cpu |= PPC_OPCODE_PPC | PPC_OPCODE_64;
	else
	  /* The minimum supported cpu for 64-bit little-endian is power8.  */
	  ppc_cpu |= ppc_parse_cpu (ppc_cpu, &sticky, "power8");
      else if (strncmp (default_os, "aix", 3) == 0
	       && default_os[3] >= '4' && default_os[3] <= '9')
	ppc_cpu |= PPC_OPCODE_COMMON;
      else if (strncmp (default_os, "aix3", 4) == 0)
	ppc_cpu |= PPC_OPCODE_POWER;
      else if (strcmp (default_cpu, "rs6000") == 0)
	ppc_cpu |= PPC_OPCODE_POWER;
      else if (strncmp (default_cpu, "powerpc", 7) == 0)
	ppc_cpu |= PPC_OPCODE_PPC;
      else
	as_fatal (_("unknown default cpu = %s, os = %s"),
		  default_cpu, default_os);
    }
}

/* Figure out the BFD architecture to use.  This function and ppc_mach
   are called well before md_begin, when the output file is opened.  */

enum bfd_architecture
ppc_arch (void)
{
  const char *default_cpu = TARGET_CPU;
  ppc_set_cpu ();

  if ((ppc_cpu & PPC_OPCODE_PPC) != 0)
    return bfd_arch_powerpc;
  if ((ppc_cpu & PPC_OPCODE_VLE) != 0)
    return bfd_arch_powerpc;
  if ((ppc_cpu & PPC_OPCODE_POWER) != 0)
    return bfd_arch_rs6000;
  if ((ppc_cpu & (PPC_OPCODE_COMMON | PPC_OPCODE_ANY)) != 0)
    {
      if (strcmp (default_cpu, "rs6000") == 0)
	return bfd_arch_rs6000;
      else if (strncmp (default_cpu, "powerpc", 7) == 0)
	return bfd_arch_powerpc;
    }

  as_fatal (_("neither Power nor PowerPC opcodes were selected."));
  return bfd_arch_unknown;
}

unsigned long
ppc_mach (void)
{
  if (ppc_obj64)
    return bfd_mach_ppc64;
  else if (ppc_arch () == bfd_arch_rs6000)
    return bfd_mach_rs6k;
  else if (ppc_cpu & PPC_OPCODE_TITAN)
    return bfd_mach_ppc_titan;
  else if (ppc_cpu & PPC_OPCODE_VLE)
    return bfd_mach_ppc_vle;
  else
    return bfd_mach_ppc;
}

extern const char*
ppc_target_format (void)
{
#ifdef OBJ_COFF
#ifdef TE_PE
  return target_big_endian ? "pe-powerpc" : "pe-powerpcle";
#elif TE_POWERMAC
  return "xcoff-powermac";
#else
#  ifdef TE_AIX5
  return (ppc_obj64 ? "aix5coff64-rs6000" : "aixcoff-rs6000");
#  else
  return (ppc_obj64 ? "aixcoff64-rs6000" : "aixcoff-rs6000");
#  endif
#endif
#endif
#ifdef OBJ_ELF
# ifdef TE_FreeBSD
  return (ppc_obj64 ? "elf64-powerpc-freebsd" : "elf32-powerpc-freebsd");
# elif defined (TE_VXWORKS)
  return "elf32-powerpc-vxworks";
# else
  return (target_big_endian
	  ? (ppc_obj64 ? "elf64-powerpc" : "elf32-powerpc")
	  : (ppc_obj64 ? "elf64-powerpcle" : "elf32-powerpcle"));
# endif
#endif
}

/* Validate one entry in powerpc_opcodes[] or vle_opcodes[].
   Return TRUE if there's a problem, otherwise FALSE.  */

static bfd_boolean
insn_validate (const struct powerpc_opcode *op)
{
  const unsigned char *o;
  uint64_t omask = op->mask;

  /* The mask had better not trim off opcode bits.  */
  if ((op->opcode & omask) != op->opcode)
    {
      as_bad (_("mask trims opcode bits for %s"), op->name);
      return TRUE;
    }

  /* The operands must not overlap the opcode or each other.  */
  for (o = op->operands; *o; ++o)
    {
      bfd_boolean optional = FALSE;
      if (*o >= num_powerpc_operands)
        {
	  as_bad (_("operand index error for %s"), op->name);
	  return TRUE;
        }
      else
        {
	  uint64_t mask;
	  const struct powerpc_operand *operand = &powerpc_operands[*o];
	  if (operand->shift == (int) PPC_OPSHIFT_INV)
	    {
	      const char *errmsg;
	      int64_t val;

	      errmsg = NULL;
	      val = -1;
	      if ((operand->flags & PPC_OPERAND_NEGATIVE) != 0)
		val = -val;
	      else if ((operand->flags & PPC_OPERAND_PLUS1) != 0)
		val += 1;
	      mask = (*operand->insert) (0, val, ppc_cpu, &errmsg);
	    }
	  else if (operand->shift >= 0)
	    mask = operand->bitm << operand->shift;
	  else
	    mask = operand->bitm >> -operand->shift;
	  if (omask & mask)
	    {
	      as_bad (_("operand %d overlap in %s"),
		      (int) (o - op->operands), op->name);
	      return TRUE;
	    }
	  omask |= mask;
	  if ((operand->flags & PPC_OPERAND_OPTIONAL) != 0)
	    optional = TRUE;
	  else if (optional)
	    {
	      as_bad (_("non-optional operand %d follows optional operand in %s"),
		      (int) (o - op->operands), op->name);
	      return TRUE;
	    }
        }
    }
  return FALSE;
}

/* Insert opcodes and macros into hash tables.  Called at startup and
   for .machine pseudo.  */

static void
ppc_setup_opcodes (void)
{
  const struct powerpc_opcode *op;
  const struct powerpc_opcode *op_end;
  const struct powerpc_macro *macro;
  const struct powerpc_macro *macro_end;
  bfd_boolean bad_insn = FALSE;

  if (ppc_hash != NULL)
    hash_die (ppc_hash);
  if (ppc_macro_hash != NULL)
    hash_die (ppc_macro_hash);

  /* Insert the opcodes into a hash table.  */
  ppc_hash = hash_new ();

  if (ENABLE_CHECKING)
    {
      unsigned int i;

      /* An index into powerpc_operands is stored in struct fix
	 fx_pcrel_adjust which is 8 bits wide.  */
      gas_assert (num_powerpc_operands < 256);

      /* Check operand masks.  Code here and in the disassembler assumes
	 all the 1's in the mask are contiguous.  */
      for (i = 0; i < num_powerpc_operands; ++i)
	{
	  uint64_t mask = powerpc_operands[i].bitm;
	  uint64_t right_bit;
	  unsigned int j;

	  right_bit = mask & -mask;
	  mask += right_bit;
	  right_bit = mask & -mask;
	  if (mask != right_bit)
	    {
	      as_bad (_("powerpc_operands[%d].bitm invalid"), i);
	      bad_insn = TRUE;
	    }
	  for (j = i + 1; j < num_powerpc_operands; ++j)
	    if (memcmp (&powerpc_operands[i], &powerpc_operands[j],
			sizeof (powerpc_operands[0])) == 0)
	      {
		as_bad (_("powerpc_operands[%d] duplicates powerpc_operands[%d]"),
			j, i);
		bad_insn = TRUE;
	      }
	}
    }

  op_end = powerpc_opcodes + powerpc_num_opcodes;
  for (op = powerpc_opcodes; op < op_end; op++)
    {
      if (ENABLE_CHECKING)
	{
	  unsigned int new_opcode = PPC_OP (op[0].opcode);

#ifdef PRINT_OPCODE_TABLE
	  printf ("%-14s\t#%04u\tmajor op: 0x%x\top: 0x%llx\tmask: 0x%llx\tflags: 0x%llx\n",
		  op->name, (unsigned int) (op - powerpc_opcodes),
		  new_opcode, (unsigned long long) op->opcode,
		  (unsigned long long) op->mask, (unsigned long long) op->flags);
#endif

	  /* The major opcodes had better be sorted.  Code in the disassembler
	     assumes the insns are sorted according to major opcode.  */
	  if (op != powerpc_opcodes
	      && new_opcode < PPC_OP (op[-1].opcode))
	    {
	      as_bad (_("major opcode is not sorted for %s"), op->name);
	      bad_insn = TRUE;
	    }

	  if ((op->flags & PPC_OPCODE_VLE) != 0)
	    {
	      as_bad (_("%s is enabled by vle flag"), op->name);
	      bad_insn = TRUE;
	    }
	  if (PPC_OP (op->opcode) != 4
	      && PPC_OP (op->opcode) != 31
	      && (op->deprecated & PPC_OPCODE_VLE) == 0)
	    {
	      as_bad (_("%s not disabled by vle flag"), op->name);
	      bad_insn = TRUE;
	    }
	  bad_insn |= insn_validate (op);
	}

      if ((ppc_cpu & op->flags) != 0
	  && !(ppc_cpu & op->deprecated))
	{
	  const char *retval;

	  retval = hash_insert (ppc_hash, op->name, (void *) op);
	  if (retval != NULL)
	    {
	      as_bad (_("duplicate instruction %s"),
		      op->name);
	      bad_insn = TRUE;
	    }
	}
    }

  if ((ppc_cpu & PPC_OPCODE_ANY) != 0)
    for (op = powerpc_opcodes; op < op_end; op++)
      hash_insert (ppc_hash, op->name, (void *) op);

  op_end = prefix_opcodes + prefix_num_opcodes;
  for (op = prefix_opcodes; op < op_end; op++)
    {
      if (ENABLE_CHECKING)
	{
	  unsigned int new_opcode = PPC_PREFIX_SEG (op[0].opcode);

#ifdef PRINT_OPCODE_TABLE
	  printf ("%-14s\t#%04u\tmajor op/2: 0x%x\top: 0x%llx\tmask: 0x%llx\tflags: 0x%llx\n",
		  op->name, (unsigned int) (op - prefix_opcodes),
		  new_opcode, (unsigned long long) op->opcode,
		  (unsigned long long) op->mask, (unsigned long long) op->flags);
#endif

	  /* The major opcodes had better be sorted.  Code in the disassembler
	     assumes the insns are sorted according to major opcode.  */
	  if (op != prefix_opcodes
	      && new_opcode < PPC_PREFIX_SEG (op[-1].opcode))
	    {
	      as_bad (_("major opcode is not sorted for %s"), op->name);
	      bad_insn = TRUE;
	    }
	  bad_insn |= insn_validate (op);
	}

      if ((ppc_cpu & op->flags) != 0
	  && !(ppc_cpu & op->deprecated))
	{
	  const char *retval;

	  retval = hash_insert (ppc_hash, op->name, (void *) op);
	  if (retval != NULL)
	    {
	      as_bad (_("duplicate instruction %s"),
		      op->name);
	      bad_insn = TRUE;
	    }
	}
    }

  if ((ppc_cpu & PPC_OPCODE_ANY) != 0)
    for (op = prefix_opcodes; op < op_end; op++)
      hash_insert (ppc_hash, op->name, (void *) op);

  op_end = vle_opcodes + vle_num_opcodes;
  for (op = vle_opcodes; op < op_end; op++)
    {
      if (ENABLE_CHECKING)
	{
	  unsigned new_seg = VLE_OP_TO_SEG (VLE_OP (op[0].opcode, op[0].mask));

#ifdef PRINT_OPCODE_TABLE
	  printf ("%-14s\t#%04u\tmajor op: 0x%x\top: 0x%llx\tmask: 0x%llx\tflags: 0x%llx\n",
		  op->name, (unsigned int) (op - vle_opcodes),
		  (unsigned int) new_seg, (unsigned long long) op->opcode,
		  (unsigned long long) op->mask, (unsigned long long) op->flags);
#endif

	  /* The major opcodes had better be sorted.  Code in the disassembler
	     assumes the insns are sorted according to major opcode.  */
	  if (op != vle_opcodes
	      && new_seg < VLE_OP_TO_SEG (VLE_OP (op[-1].opcode, op[-1].mask)))
	    {
	      as_bad (_("major opcode is not sorted for %s"), op->name);
	      bad_insn = TRUE;
	    }

	  bad_insn |= insn_validate (op);
	}

      if ((ppc_cpu & op->flags) != 0
	  && !(ppc_cpu & op->deprecated))
	{
	  const char *retval;

	  retval = hash_insert (ppc_hash, op->name, (void *) op);
	  if (retval != NULL)
	    {
	      as_bad (_("duplicate instruction %s"),
		      op->name);
	      bad_insn = TRUE;
	    }
	}
    }

  /* SPE2 instructions */
  if ((ppc_cpu & PPC_OPCODE_SPE2) == PPC_OPCODE_SPE2)
    {
      op_end = spe2_opcodes + spe2_num_opcodes;
      for (op = spe2_opcodes; op < op_end; op++)
	{
	  if (ENABLE_CHECKING)
	    {
	      if (op != spe2_opcodes)
		{
		unsigned old_seg, new_seg;

		old_seg = VLE_OP (op[-1].opcode, op[-1].mask);
		old_seg = VLE_OP_TO_SEG (old_seg);
		new_seg = VLE_OP (op[0].opcode, op[0].mask);
		new_seg = VLE_OP_TO_SEG (new_seg);

		/* The major opcodes had better be sorted.  Code in the
		    disassembler assumes the insns are sorted according to
		    major opcode.  */
		if (new_seg < old_seg)
		  {
		  as_bad (_("major opcode is not sorted for %s"), op->name);
		  bad_insn = TRUE;
		  }
		}

	      bad_insn |= insn_validate (op);
	    }

	  if ((ppc_cpu & op->flags) != 0 && !(ppc_cpu & op->deprecated))
	    {
	      const char *retval;

	      retval = hash_insert (ppc_hash, op->name, (void *) op);
	      if (retval != NULL)
		{
		  as_bad (_("duplicate instruction %s"),
			  op->name);
		  bad_insn = TRUE;
		}
	    }
	}

      for (op = spe2_opcodes; op < op_end; op++)
	hash_insert (ppc_hash, op->name, (void *) op);
    }

  /* Insert the macros into a hash table.  */
  ppc_macro_hash = hash_new ();

  macro_end = powerpc_macros + powerpc_num_macros;
  for (macro = powerpc_macros; macro < macro_end; macro++)
    {
      if ((macro->flags & ppc_cpu) != 0 || (ppc_cpu & PPC_OPCODE_ANY) != 0)
	{
	  const char *retval;

	  retval = hash_insert (ppc_macro_hash, macro->name, (void *) macro);
	  if (retval != (const char *) NULL)
	    {
	      as_bad (_("duplicate macro %s"), macro->name);
	      bad_insn = TRUE;
	    }
	}
    }

  if (bad_insn)
    abort ();
}

/* This function is called when the assembler starts up.  It is called
   after the options have been parsed and the output file has been
   opened.  */

void
md_begin (void)
{
  ppc_set_cpu ();

  ppc_cie_data_alignment = ppc_obj64 ? -8 : -4;
  ppc_dwarf2_line_min_insn_length = (ppc_cpu & PPC_OPCODE_VLE) ? 2 : 4;

#ifdef OBJ_ELF
  /* Set the ELF flags if desired.  */
  if (ppc_flags && !msolaris)
    bfd_set_private_flags (stdoutput, ppc_flags);
#endif

  ppc_setup_opcodes ();

  /* Tell the main code what the endianness is if it is not overridden
     by the user.  */
  if (!set_target_endian)
    {
      set_target_endian = 1;
      target_big_endian = PPC_BIG_ENDIAN;
    }

#ifdef OBJ_XCOFF
  ppc_coff_debug_section = coff_section_from_bfd_index (stdoutput, N_DEBUG);

  /* Create dummy symbols to serve as initial csects.  This forces the
     text csects to precede the data csects.  These symbols will not
     be output.  */
  ppc_text_csects = symbol_make ("dummy\001");
  symbol_get_tc (ppc_text_csects)->within = ppc_text_csects;
  ppc_data_csects = symbol_make ("dummy\001");
  symbol_get_tc (ppc_data_csects)->within = ppc_data_csects;
#endif

#ifdef TE_PE

  ppc_current_section = text_section;
  ppc_previous_section = 0;

#endif
}

void
ppc_cleanup (void)
{
#ifdef OBJ_ELF
  if (ppc_apuinfo_list == NULL)
    return;

  /* Ok, so write the section info out.  We have this layout:

  byte	data		what
  ----	----		----
  0	8		length of "APUinfo\0"
  4	(n*4)		number of APU's (4 bytes each)
  8	2		note type 2
  12	"APUinfo\0"	name
  20	APU#1		first APU's info
  24	APU#2		second APU's info
  ...	...
  */
  {
    char *p;
    asection *seg = now_seg;
    subsegT subseg = now_subseg;
    asection *apuinfo_secp = (asection *) NULL;
    unsigned int i;

    /* Create the .PPC.EMB.apuinfo section.  */
    apuinfo_secp = subseg_new (APUINFO_SECTION_NAME, 0);
    bfd_set_section_flags (apuinfo_secp, SEC_HAS_CONTENTS | SEC_READONLY);

    p = frag_more (4);
    md_number_to_chars (p, (valueT) 8, 4);

    p = frag_more (4);
    md_number_to_chars (p, (valueT) ppc_apuinfo_num * 4, 4);

    p = frag_more (4);
    md_number_to_chars (p, (valueT) 2, 4);

    p = frag_more (8);
    strcpy (p, APUINFO_LABEL);

    for (i = 0; i < ppc_apuinfo_num; i++)
      {
	p = frag_more (4);
	md_number_to_chars (p, (valueT) ppc_apuinfo_list[i], 4);
      }

    frag_align (2, 0, 0);

    /* We probably can't restore the current segment, for there likely
       isn't one yet...  */
    if (seg && subseg)
      subseg_set (seg, subseg);
  }
#endif
}

/* Insert an operand value into an instruction.  */

static uint64_t
ppc_insert_operand (uint64_t insn,
		    const struct powerpc_operand *operand,
		    int64_t val,
		    ppc_cpu_t cpu,
		    const char *file,
		    unsigned int line)
{
  int64_t min, max, right;

  max = operand->bitm;
  right = max & -max;
  min = 0;

  if ((operand->flags & PPC_OPERAND_SIGNOPT) != 0)
    {
      /* Extend the allowed range for addis to [-32768, 65535].
	 Similarly for cmpli and some VLE high part insns.  For 64-bit
	 it would be good to disable this for signed fields since the
	 value is sign extended into the high 32 bits of the register.
	 If the value is, say, an address, then we might care about
	 the high bits.  However, gcc as of 2014-06 uses unsigned
	 values when loading the high part of 64-bit constants using
	 lis.  */
      min = ~(max >> 1) & -right;
    }
  else if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
    {
      max = (max >> 1) & -right;
      min = ~max & -right;
    }

  if ((operand->flags & PPC_OPERAND_PLUS1) != 0)
    max++;

  if ((operand->flags & PPC_OPERAND_NEGATIVE) != 0)
    {
      int64_t tmp = min;
      min = -max;
      max = -tmp;
    }

  if (min <= max)
    {
      /* Some people write constants with the sign extension done by
	 hand but only up to 32 bits.  This shouldn't really be valid,
	 but, to permit this code to assemble on a 64-bit host, we
	 sign extend the 32-bit value to 64 bits if so doing makes the
	 value valid.  We only do this for operands that are 32-bits or
	 smaller.  */
      if (val > max
	  && (operand->bitm & ~0xffffffffULL) == 0
	  && (val - (1LL << 32)) >= min
	  && (val - (1LL << 32)) <= max
	  && ((val - (1LL << 32)) & (right - 1)) == 0)
	val = val - (1LL << 32);

      /* Similarly, people write expressions like ~(1<<15), and expect
	 this to be OK for a 32-bit unsigned value.  */
      else if (val < min
	       && (operand->bitm & ~0xffffffffULL) == 0
	       && (val + (1LL << 32)) >= min
	       && (val + (1LL << 32)) <= max
	       && ((val + (1LL << 32)) & (right - 1)) == 0)
	val = val + (1LL << 32);

      else if (val < min
	       || val > max
	       || (val & (right - 1)) != 0)
	as_bad_value_out_of_range (_("operand"), val, min, max, file, line);
    }

  if (operand->insert)
    {
      const char *errmsg;

      errmsg = NULL;
      insn = (*operand->insert) (insn, val, cpu, &errmsg);
      if (errmsg != (const char *) NULL)
	as_bad_where (file, line, "%s", errmsg);
    }
  else if (operand->shift >= 0)
    insn |= (val & operand->bitm) << operand->shift;
  else
    insn |= (val & operand->bitm) >> -operand->shift;

  return insn;
}


#ifdef OBJ_ELF
/* Parse @got, etc. and return the desired relocation.  */
static bfd_reloc_code_real_type
ppc_elf_suffix (char **str_p, expressionS *exp_p)
{
  struct map_bfd {
    const char *string;
    unsigned int length : 8;
    unsigned int valid32 : 1;
    unsigned int valid64 : 1;
    unsigned int reloc;
  };

  char ident[20];
  char *str = *str_p;
  char *str2;
  int ch;
  int len;
  const struct map_bfd *ptr;

#define MAP(str, reloc)   { str, sizeof (str) - 1, 1, 1, reloc }
#define MAP32(str, reloc) { str, sizeof (str) - 1, 1, 0, reloc }
#define MAP64(str, reloc) { str, sizeof (str) - 1, 0, 1, reloc }

  static const struct map_bfd mapping[] = {
    MAP ("l",			BFD_RELOC_LO16),
    MAP ("h",			BFD_RELOC_HI16),
    MAP ("ha",			BFD_RELOC_HI16_S),
    MAP ("brtaken",		BFD_RELOC_PPC_B16_BRTAKEN),
    MAP ("brntaken",		BFD_RELOC_PPC_B16_BRNTAKEN),
    MAP ("got",			BFD_RELOC_16_GOTOFF),
    MAP ("got@l",		BFD_RELOC_LO16_GOTOFF),
    MAP ("got@h",		BFD_RELOC_HI16_GOTOFF),
    MAP ("got@ha",		BFD_RELOC_HI16_S_GOTOFF),
    MAP ("plt@l",		BFD_RELOC_LO16_PLTOFF),
    MAP ("plt@h",		BFD_RELOC_HI16_PLTOFF),
    MAP ("plt@ha",		BFD_RELOC_HI16_S_PLTOFF),
    MAP ("copy",		BFD_RELOC_PPC_COPY),
    MAP ("globdat",		BFD_RELOC_PPC_GLOB_DAT),
    MAP ("sectoff",		BFD_RELOC_16_BASEREL),
    MAP ("sectoff@l",		BFD_RELOC_LO16_BASEREL),
    MAP ("sectoff@h",		BFD_RELOC_HI16_BASEREL),
    MAP ("sectoff@ha",		BFD_RELOC_HI16_S_BASEREL),
    MAP ("tls",			BFD_RELOC_PPC_TLS),
    MAP ("dtpmod",		BFD_RELOC_PPC_DTPMOD),
    MAP ("dtprel",		BFD_RELOC_PPC_DTPREL),
    MAP ("dtprel@l",		BFD_RELOC_PPC_DTPREL16_LO),
    MAP ("dtprel@h",		BFD_RELOC_PPC_DTPREL16_HI),
    MAP ("dtprel@ha",		BFD_RELOC_PPC_DTPREL16_HA),
    MAP ("tprel",		BFD_RELOC_PPC_TPREL),
    MAP ("tprel@l",		BFD_RELOC_PPC_TPREL16_LO),
    MAP ("tprel@h",		BFD_RELOC_PPC_TPREL16_HI),
    MAP ("tprel@ha",		BFD_RELOC_PPC_TPREL16_HA),
    MAP ("got@tlsgd",		BFD_RELOC_PPC_GOT_TLSGD16),
    MAP ("got@tlsgd@l",		BFD_RELOC_PPC_GOT_TLSGD16_LO),
    MAP ("got@tlsgd@h",		BFD_RELOC_PPC_GOT_TLSGD16_HI),
    MAP ("got@tlsgd@ha",	BFD_RELOC_PPC_GOT_TLSGD16_HA),
    MAP ("got@tlsld",		BFD_RELOC_PPC_GOT_TLSLD16),
    MAP ("got@tlsld@l",		BFD_RELOC_PPC_GOT_TLSLD16_LO),
    MAP ("got@tlsld@h",		BFD_RELOC_PPC_GOT_TLSLD16_HI),
    MAP ("got@tlsld@ha",	BFD_RELOC_PPC_GOT_TLSLD16_HA),
    MAP ("got@dtprel",		BFD_RELOC_PPC_GOT_DTPREL16),
    MAP ("got@dtprel@l",	BFD_RELOC_PPC_GOT_DTPREL16_LO),
    MAP ("got@dtprel@h",	BFD_RELOC_PPC_GOT_DTPREL16_HI),
    MAP ("got@dtprel@ha",	BFD_RELOC_PPC_GOT_DTPREL16_HA),
    MAP ("got@tprel",		BFD_RELOC_PPC_GOT_TPREL16),
    MAP ("got@tprel@l",		BFD_RELOC_PPC_GOT_TPREL16_LO),
    MAP ("got@tprel@h",		BFD_RELOC_PPC_GOT_TPREL16_HI),
    MAP ("got@tprel@ha",	BFD_RELOC_PPC_GOT_TPREL16_HA),
    MAP32 ("fixup",		BFD_RELOC_CTOR),
    MAP32 ("plt",		BFD_RELOC_24_PLT_PCREL),
    MAP32 ("pltrel24",		BFD_RELOC_24_PLT_PCREL),
    MAP32 ("local24pc",		BFD_RELOC_PPC_LOCAL24PC),
    MAP32 ("local",		BFD_RELOC_PPC_LOCAL24PC),
    MAP32 ("pltrel",		BFD_RELOC_32_PLT_PCREL),
    MAP32 ("sdarel",		BFD_RELOC_GPREL16),
    MAP32 ("sdarel@l",		BFD_RELOC_PPC_VLE_SDAREL_LO16A),
    MAP32 ("sdarel@h",		BFD_RELOC_PPC_VLE_SDAREL_HI16A),
    MAP32 ("sdarel@ha",		BFD_RELOC_PPC_VLE_SDAREL_HA16A),
    MAP32 ("naddr",		BFD_RELOC_PPC_EMB_NADDR32),
    MAP32 ("naddr16",		BFD_RELOC_PPC_EMB_NADDR16),
    MAP32 ("naddr@l",		BFD_RELOC_PPC_EMB_NADDR16_LO),
    MAP32 ("naddr@h",		BFD_RELOC_PPC_EMB_NADDR16_HI),
    MAP32 ("naddr@ha",		BFD_RELOC_PPC_EMB_NADDR16_HA),
    MAP32 ("sdai16",		BFD_RELOC_PPC_EMB_SDAI16),
    MAP32 ("sda2rel",		BFD_RELOC_PPC_EMB_SDA2REL),
    MAP32 ("sda2i16",		BFD_RELOC_PPC_EMB_SDA2I16),
    MAP32 ("sda21",		BFD_RELOC_PPC_EMB_SDA21),
    MAP32 ("sda21@l",		BFD_RELOC_PPC_VLE_SDA21_LO),
    MAP32 ("mrkref",		BFD_RELOC_PPC_EMB_MRKREF),
    MAP32 ("relsect",		BFD_RELOC_PPC_EMB_RELSEC16),
    MAP32 ("relsect@l",		BFD_RELOC_PPC_EMB_RELST_LO),
    MAP32 ("relsect@h",		BFD_RELOC_PPC_EMB_RELST_HI),
    MAP32 ("relsect@ha",	BFD_RELOC_PPC_EMB_RELST_HA),
    MAP32 ("bitfld",		BFD_RELOC_PPC_EMB_BIT_FLD),
    MAP32 ("relsda",		BFD_RELOC_PPC_EMB_RELSDA),
    MAP32 ("xgot",		BFD_RELOC_PPC_TOC16),
    MAP64 ("high",		BFD_RELOC_PPC64_ADDR16_HIGH),
    MAP64 ("higha",		BFD_RELOC_PPC64_ADDR16_HIGHA),
    MAP64 ("higher",		BFD_RELOC_PPC64_HIGHER),
    MAP64 ("highera",		BFD_RELOC_PPC64_HIGHER_S),
    MAP64 ("highest",		BFD_RELOC_PPC64_HIGHEST),
    MAP64 ("highesta",		BFD_RELOC_PPC64_HIGHEST_S),
    MAP64 ("tocbase",		BFD_RELOC_PPC64_TOC),
    MAP64 ("toc",		BFD_RELOC_PPC_TOC16),
    MAP64 ("toc@l",		BFD_RELOC_PPC64_TOC16_LO),
    MAP64 ("toc@h",		BFD_RELOC_PPC64_TOC16_HI),
    MAP64 ("toc@ha",		BFD_RELOC_PPC64_TOC16_HA),
    MAP64 ("dtprel@high",	BFD_RELOC_PPC64_DTPREL16_HIGH),
    MAP64 ("dtprel@higha",	BFD_RELOC_PPC64_DTPREL16_HIGHA),
    MAP64 ("dtprel@higher",	BFD_RELOC_PPC64_DTPREL16_HIGHER),
    MAP64 ("dtprel@highera",	BFD_RELOC_PPC64_DTPREL16_HIGHERA),
    MAP64 ("dtprel@highest",	BFD_RELOC_PPC64_DTPREL16_HIGHEST),
    MAP64 ("dtprel@highesta",	BFD_RELOC_PPC64_DTPREL16_HIGHESTA),
    MAP64 ("localentry",	BFD_RELOC_PPC64_ADDR64_LOCAL),
    MAP64 ("tprel@high",	BFD_RELOC_PPC64_TPREL16_HIGH),
    MAP64 ("tprel@higha",	BFD_RELOC_PPC64_TPREL16_HIGHA),
    MAP64 ("tprel@higher",	BFD_RELOC_PPC64_TPREL16_HIGHER),
    MAP64 ("tprel@highera",	BFD_RELOC_PPC64_TPREL16_HIGHERA),
    MAP64 ("tprel@highest",	BFD_RELOC_PPC64_TPREL16_HIGHEST),
    MAP64 ("tprel@highesta",	BFD_RELOC_PPC64_TPREL16_HIGHESTA),
    MAP64 ("notoc",		BFD_RELOC_PPC64_REL24_NOTOC),
    MAP64 ("pcrel",		BFD_RELOC_PPC64_PCREL34),
    MAP64 ("got@pcrel",		BFD_RELOC_PPC64_GOT_PCREL34),
    MAP64 ("plt@pcrel",		BFD_RELOC_PPC64_PLT_PCREL34),
    MAP64 ("tls@pcrel",		BFD_RELOC_PPC64_TLS_PCREL),
    MAP64 ("got@tlsgd@pcrel",	BFD_RELOC_PPC64_GOT_TLSGD34),
    MAP64 ("got@tlsld@pcrel",	BFD_RELOC_PPC64_GOT_TLSLD34),
    MAP64 ("got@tprel@pcrel",	BFD_RELOC_PPC64_GOT_TPREL34),
    MAP64 ("got@dtprel@pcrel",	BFD_RELOC_PPC64_GOT_DTPREL34),
    MAP64 ("higher34",		BFD_RELOC_PPC64_ADDR16_HIGHER34),
    MAP64 ("highera34",		BFD_RELOC_PPC64_ADDR16_HIGHERA34),
    MAP64 ("highest34",		BFD_RELOC_PPC64_ADDR16_HIGHEST34),
    MAP64 ("highesta34",	BFD_RELOC_PPC64_ADDR16_HIGHESTA34),
    { (char *) 0, 0, 0, 0,	BFD_RELOC_NONE }
  };

  if (*str++ != '@')
    return BFD_RELOC_NONE;

  for (ch = *str, str2 = ident;
       (str2 < ident + sizeof (ident) - 1
	&& (ISALNUM (ch) || ch == '@'));
       ch = *++str)
    {
      *str2++ = TOLOWER (ch);
    }

  *str2 = '\0';
  len = str2 - ident;

  ch = ident[0];
  for (ptr = &mapping[0]; ptr->length > 0; ptr++)
    if (ch == ptr->string[0]
	&& len == ptr->length
	&& memcmp (ident, ptr->string, ptr->length) == 0
	&& (ppc_obj64 ? ptr->valid64 : ptr->valid32))
      {
	int reloc = ptr->reloc;

	if (!ppc_obj64 && exp_p->X_add_number != 0)
	  {
	    switch (reloc)
	      {
	      case BFD_RELOC_16_GOTOFF:
	      case BFD_RELOC_LO16_GOTOFF:
	      case BFD_RELOC_HI16_GOTOFF:
	      case BFD_RELOC_HI16_S_GOTOFF:
		as_warn (_("symbol+offset@%s means symbol@%s+offset"),
			 ptr->string, ptr->string);
		break;

	      case BFD_RELOC_PPC_GOT_TLSGD16:
	      case BFD_RELOC_PPC_GOT_TLSGD16_LO:
	      case BFD_RELOC_PPC_GOT_TLSGD16_HI:
	      case BFD_RELOC_PPC_GOT_TLSGD16_HA:
	      case BFD_RELOC_PPC_GOT_TLSLD16:
	      case BFD_RELOC_PPC_GOT_TLSLD16_LO:
	      case BFD_RELOC_PPC_GOT_TLSLD16_HI:
	      case BFD_RELOC_PPC_GOT_TLSLD16_HA:
	      case BFD_RELOC_PPC_GOT_DTPREL16:
	      case BFD_RELOC_PPC_GOT_DTPREL16_LO:
	      case BFD_RELOC_PPC_GOT_DTPREL16_HI:
	      case BFD_RELOC_PPC_GOT_DTPREL16_HA:
	      case BFD_RELOC_PPC_GOT_TPREL16:
	      case BFD_RELOC_PPC_GOT_TPREL16_LO:
	      case BFD_RELOC_PPC_GOT_TPREL16_HI:
	      case BFD_RELOC_PPC_GOT_TPREL16_HA:
		as_bad (_("symbol+offset@%s not supported"), ptr->string);
		break;
	      }
	  }

	/* Now check for identifier@suffix+constant.  */
	if (*str == '-' || *str == '+')
	  {
	    char *orig_line = input_line_pointer;
	    expressionS new_exp;

	    input_line_pointer = str;
	    expression (&new_exp);
	    if (new_exp.X_op == O_constant)
	      {
		exp_p->X_add_number += new_exp.X_add_number;
		str = input_line_pointer;
	      }

	    if (&input_line_pointer != str_p)
	      input_line_pointer = orig_line;
	  }
	*str_p = str;

	if (reloc == (int) BFD_RELOC_PPC64_TOC
	    && exp_p->X_op == O_symbol
	    && strcmp (S_GET_NAME (exp_p->X_add_symbol), ".TOC.") == 0)
	  {
	    /* Change the symbol so that the dummy .TOC. symbol can be
	       omitted from the object file.  */
	    exp_p->X_add_symbol = &abs_symbol;
	  }

	return (bfd_reloc_code_real_type) reloc;
      }

  return BFD_RELOC_NONE;
}

/* Support @got, etc. on constants emitted via .short, .int etc.  */

bfd_reloc_code_real_type
ppc_elf_parse_cons (expressionS *exp, unsigned int nbytes)
{
  expression (exp);
  if (nbytes >= 2 && *input_line_pointer == '@')
    return ppc_elf_suffix (&input_line_pointer, exp);
  return BFD_RELOC_NONE;
}

/* Warn when emitting data to code sections, unless we are emitting
   a relocation that ld --ppc476-workaround uses to recognise data
   *and* there was an unconditional branch prior to the data.  */

void
ppc_elf_cons_fix_check (expressionS *exp ATTRIBUTE_UNUSED,
			unsigned int nbytes, fixS *fix)
{
  if (warn_476
      && (now_seg->flags & SEC_CODE) != 0
      && (nbytes != 4
	  || fix == NULL
	  || !(fix->fx_r_type == BFD_RELOC_32
	       || fix->fx_r_type == BFD_RELOC_CTOR
	       || fix->fx_r_type == BFD_RELOC_32_PCREL)
	  || !(last_seg == now_seg && last_subseg == now_subseg)
	  || !((last_insn & (0x3f << 26)) == (18u << 26)
	       || ((last_insn & (0x3f << 26)) == (16u << 26)
		   && (last_insn & (0x14 << 21)) == (0x14 << 21))
	       || ((last_insn & (0x3f << 26)) == (19u << 26)
		   && (last_insn & (0x3ff << 1)) == (16u << 1)
		   && (last_insn & (0x14 << 21)) == (0x14 << 21)))))
    {
      /* Flag that we've warned.  */
      if (fix != NULL)
	fix->fx_tcbit = 1;

      as_warn (_("data in executable section"));
    }
}

/* Solaris pseduo op to change to the .rodata section.  */
static void
ppc_elf_rdata (int xxx)
{
  char *save_line = input_line_pointer;
  static char section[] = ".rodata\n";

  /* Just pretend this is .section .rodata  */
  input_line_pointer = section;
  obj_elf_section (xxx);

  input_line_pointer = save_line;
}

/* Pseudo op to make file scope bss items.  */
static void
ppc_elf_lcomm (int xxx ATTRIBUTE_UNUSED)
{
  char *name;
  char c;
  char *p;
  offsetT size;
  symbolS *symbolP;
  offsetT align;
  segT old_sec;
  int old_subsec;
  char *pfrag;
  int align2;

  c = get_symbol_name (&name);

  /* Just after name is now '\0'.  */
  p = input_line_pointer;
  *p = c;
  SKIP_WHITESPACE_AFTER_NAME ();
  if (*input_line_pointer != ',')
    {
      as_bad (_("expected comma after symbol-name: rest of line ignored."));
      ignore_rest_of_line ();
      return;
    }

  input_line_pointer++;		/* skip ',' */
  if ((size = get_absolute_expression ()) < 0)
    {
      as_warn (_(".COMMon length (%ld.) <0! Ignored."), (long) size);
      ignore_rest_of_line ();
      return;
    }

  /* The third argument to .lcomm is the alignment.  */
  if (*input_line_pointer != ',')
    align = 8;
  else
    {
      ++input_line_pointer;
      align = get_absolute_expression ();
      if (align <= 0)
	{
	  as_warn (_("ignoring bad alignment"));
	  align = 8;
	}
    }

  *p = 0;
  symbolP = symbol_find_or_make (name);
  *p = c;

  if (S_IS_DEFINED (symbolP) && ! S_IS_COMMON (symbolP))
    {
      as_bad (_("ignoring attempt to re-define symbol `%s'."),
	      S_GET_NAME (symbolP));
      ignore_rest_of_line ();
      return;
    }

  if (S_GET_VALUE (symbolP) && S_GET_VALUE (symbolP) != (valueT) size)
    {
      as_bad (_("length of .lcomm \"%s\" is already %ld. Not changed to %ld."),
	      S_GET_NAME (symbolP),
	      (long) S_GET_VALUE (symbolP),
	      (long) size);

      ignore_rest_of_line ();
      return;
    }

  /* Allocate_bss.  */
  old_sec = now_seg;
  old_subsec = now_subseg;
  if (align)
    {
      /* Convert to a power of 2 alignment.  */
      for (align2 = 0; (align & 1) == 0; align >>= 1, ++align2);
      if (align != 1)
	{
	  as_bad (_("common alignment not a power of 2"));
	  ignore_rest_of_line ();
	  return;
	}
    }
  else
    align2 = 0;

  record_alignment (bss_section, align2);
  subseg_set (bss_section, 1);
  if (align2)
    frag_align (align2, 0, 0);
  if (S_GET_SEGMENT (symbolP) == bss_section)
    symbol_get_frag (symbolP)->fr_symbol = 0;
  symbol_set_frag (symbolP, frag_now);
  pfrag = frag_var (rs_org, 1, 1, (relax_substateT) 0, symbolP, size,
		    (char *) 0);
  *pfrag = 0;
  S_SET_SIZE (symbolP, size);
  S_SET_SEGMENT (symbolP, bss_section);
  subseg_set (old_sec, old_subsec);
  demand_empty_rest_of_line ();
}

/* Pseudo op to set symbol local entry point.  */
static void
ppc_elf_localentry (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char c = get_symbol_name (&name);
  char *p;
  expressionS exp;
  symbolS *sym;
  asymbol *bfdsym;
  elf_symbol_type *elfsym;

  p = input_line_pointer;
  *p = c;
  SKIP_WHITESPACE_AFTER_NAME ();
  if (*input_line_pointer != ',')
    {
      *p = 0;
      as_bad (_("expected comma after name `%s' in .localentry directive"),
	      name);
      *p = c;
      ignore_rest_of_line ();
      return;
    }
  input_line_pointer++;
  expression (&exp);
  if (exp.X_op == O_absent)
    {
      as_bad (_("missing expression in .localentry directive"));
      exp.X_op = O_constant;
      exp.X_add_number = 0;
    }
  *p = 0;
  sym = symbol_find_or_make (name);
  *p = c;

  if (resolve_expression (&exp)
      && exp.X_op == O_constant)
    {
      unsigned int encoded, ok;

      ok = 1;
      if (exp.X_add_number == 1 || exp.X_add_number == 7)
	encoded = exp.X_add_number << STO_PPC64_LOCAL_BIT;
      else
	{
	  encoded = PPC64_SET_LOCAL_ENTRY_OFFSET (exp.X_add_number);
	  if (exp.X_add_number != (offsetT) PPC64_LOCAL_ENTRY_OFFSET (encoded))
	    {
	      as_bad (_(".localentry expression for `%s' "
			"is not a valid power of 2"), S_GET_NAME (sym));
	      ok = 0;
	    }
	}
      if (ok)
	{
	  bfdsym = symbol_get_bfdsym (sym);
	  elfsym = elf_symbol_from (bfd_asymbol_bfd (bfdsym), bfdsym);
	  gas_assert (elfsym);
	  elfsym->internal_elf_sym.st_other &= ~STO_PPC64_LOCAL_MASK;
	  elfsym->internal_elf_sym.st_other |= encoded;
	  if (ppc_abiversion == 0)
	    ppc_abiversion = 2;
	}
    }
  else
    as_bad (_(".localentry expression for `%s' "
	      "does not evaluate to a constant"), S_GET_NAME (sym));

  demand_empty_rest_of_line ();
}

/* Pseudo op to set ABI version.  */
static void
ppc_elf_abiversion (int ignore ATTRIBUTE_UNUSED)
{
  expressionS exp;

  expression (&exp);
  if (exp.X_op == O_absent)
    {
      as_bad (_("missing expression in .abiversion directive"));
      exp.X_op = O_constant;
      exp.X_add_number = 0;
    }

  if (resolve_expression (&exp)
      && exp.X_op == O_constant)
    ppc_abiversion = exp.X_add_number;
  else
    as_bad (_(".abiversion expression does not evaluate to a constant"));
  demand_empty_rest_of_line ();
}

/* Parse a .gnu_attribute directive.  */
static void
ppc_elf_gnu_attribute (int ignored ATTRIBUTE_UNUSED)
{
  int tag = obj_elf_vendor_attribute (OBJ_ATTR_GNU);

  /* Check validity of defined powerpc tags.  */
  if (tag == Tag_GNU_Power_ABI_FP
      || tag == Tag_GNU_Power_ABI_Vector
      || tag == Tag_GNU_Power_ABI_Struct_Return)
    {
      unsigned int val;

      val = bfd_elf_get_obj_attr_int (stdoutput, OBJ_ATTR_GNU, tag);

      if ((tag == Tag_GNU_Power_ABI_FP && val > 15)
	  || (tag == Tag_GNU_Power_ABI_Vector && val > 3)
	  || (tag == Tag_GNU_Power_ABI_Struct_Return && val > 2))
	as_warn (_("unknown .gnu_attribute value"));
    }
}

/* Set ABI version in output file.  */
void
ppc_elf_end (void)
{
  if (ppc_obj64 && ppc_abiversion != 0)
    {
      elf_elfheader (stdoutput)->e_flags &= ~EF_PPC64_ABI;
      elf_elfheader (stdoutput)->e_flags |= ppc_abiversion & EF_PPC64_ABI;
    }
  /* Any selection of opcodes based on ppc_cpu after gas has finished
     parsing the file is invalid.  md_apply_fix and ppc_handle_align
     must select opcodes based on the machine in force at the point
     where the fixup or alignment frag was created, not the machine in
     force at the end of file.  */
  ppc_cpu = 0;
}

/* Validate any relocations emitted for -mrelocatable, possibly adding
   fixups for word relocations in writable segments, so we can adjust
   them at runtime.  */
static void
ppc_elf_validate_fix (fixS *fixp, segT seg)
{
  if (fixp->fx_done || fixp->fx_pcrel)
    return;

  switch (shlib)
    {
    case SHLIB_NONE:
    case SHLIB_PIC:
      return;

    case SHLIB_MRELOCATABLE:
      if (fixp->fx_r_type != BFD_RELOC_16_GOTOFF
	  && fixp->fx_r_type != BFD_RELOC_HI16_GOTOFF
	  && fixp->fx_r_type != BFD_RELOC_LO16_GOTOFF
	  && fixp->fx_r_type != BFD_RELOC_HI16_S_GOTOFF
	  && fixp->fx_r_type != BFD_RELOC_16_BASEREL
	  && fixp->fx_r_type != BFD_RELOC_LO16_BASEREL
	  && fixp->fx_r_type != BFD_RELOC_HI16_BASEREL
	  && fixp->fx_r_type != BFD_RELOC_HI16_S_BASEREL
	  && (seg->flags & SEC_LOAD) != 0
	  && strcmp (segment_name (seg), ".got2") != 0
	  && strcmp (segment_name (seg), ".dtors") != 0
	  && strcmp (segment_name (seg), ".ctors") != 0
	  && strcmp (segment_name (seg), ".fixup") != 0
	  && strcmp (segment_name (seg), ".gcc_except_table") != 0
	  && strcmp (segment_name (seg), ".eh_frame") != 0
	  && strcmp (segment_name (seg), ".ex_shared") != 0)
	{
	  if ((seg->flags & (SEC_READONLY | SEC_CODE)) != 0
	      || fixp->fx_r_type != BFD_RELOC_CTOR)
	    {
	      as_bad_where (fixp->fx_file, fixp->fx_line,
			    _("relocation cannot be done when using -mrelocatable"));
	    }
	}
      return;
    }
}

/* Prevent elf_frob_file_before_adjust removing a weak undefined
   function descriptor sym if the corresponding code sym is used.  */

void
ppc_frob_file_before_adjust (void)
{
  symbolS *symp;
  asection *toc;

  if (!ppc_obj64)
    return;

  for (symp = symbol_rootP; symp; symp = symbol_next (symp))
    {
      const char *name;
      char *dotname;
      symbolS *dotsym;

      name = S_GET_NAME (symp);
      if (name[0] == '.')
	continue;

      if (! S_IS_WEAK (symp)
	  || S_IS_DEFINED (symp))
	continue;

      dotname = concat (".", name, (char *) NULL);
      dotsym = symbol_find_noref (dotname, 1);
      free (dotname);
      if (dotsym != NULL && (symbol_used_p (dotsym)
			     || symbol_used_in_reloc_p (dotsym)))
	symbol_mark_used (symp);

    }

  toc = bfd_get_section_by_name (stdoutput, ".toc");
  if (toc != NULL
      && toc_reloc_types != has_large_toc_reloc
      && bfd_section_size (toc) > 0x10000)
    as_warn (_("TOC section size exceeds 64k"));
}

/* .TOC. used in an opd entry as .TOC.@tocbase doesn't need to be
   emitted.  Other uses of .TOC. will cause the symbol to be marked
   with BSF_KEEP in md_apply_fix.  */

void
ppc_elf_adjust_symtab (void)
{
  if (ppc_obj64)
    {
      symbolS *symp;
      symp = symbol_find (".TOC.");
      if (symp != NULL)
	{
	  asymbol *bsym = symbol_get_bfdsym (symp);
	  if ((bsym->flags & BSF_KEEP) == 0)
	    symbol_remove (symp, &symbol_rootP, &symbol_lastP);
	}
    }
}
#endif /* OBJ_ELF */

#ifdef TE_PE

/*
 * Summary of parse_toc_entry.
 *
 * in:	Input_line_pointer points to the '[' in one of:
 *
 *        [toc] [tocv] [toc32] [toc64]
 *
 *      Anything else is an error of one kind or another.
 *
 * out:
 *   return value: success or failure
 *   toc_kind:     kind of toc reference
 *   input_line_pointer:
 *     success: first char after the ']'
 *     failure: unchanged
 *
 * settings:
 *
 *     [toc]   - rv == success, toc_kind = default_toc
 *     [tocv]  - rv == success, toc_kind = data_in_toc
 *     [toc32] - rv == success, toc_kind = must_be_32
 *     [toc64] - rv == success, toc_kind = must_be_64
 *
 */

enum toc_size_qualifier
{
  default_toc, /* The toc cell constructed should be the system default size */
  data_in_toc, /* This is a direct reference to a toc cell                   */
  must_be_32,  /* The toc cell constructed must be 32 bits wide              */
  must_be_64   /* The toc cell constructed must be 64 bits wide              */
};

static int
parse_toc_entry (enum toc_size_qualifier *toc_kind)
{
  char *start;
  char *toc_spec;
  char c;
  enum toc_size_qualifier t;

  /* Save the input_line_pointer.  */
  start = input_line_pointer;

  /* Skip over the '[' , and whitespace.  */
  ++input_line_pointer;
  SKIP_WHITESPACE ();

  /* Find the spelling of the operand.  */
  c = get_symbol_name (&toc_spec);

  if (strcmp (toc_spec, "toc") == 0)
    {
      t = default_toc;
    }
  else if (strcmp (toc_spec, "tocv") == 0)
    {
      t = data_in_toc;
    }
  else if (strcmp (toc_spec, "toc32") == 0)
    {
      t = must_be_32;
    }
  else if (strcmp (toc_spec, "toc64") == 0)
    {
      t = must_be_64;
    }
  else
    {
      as_bad (_("syntax error: invalid toc specifier `%s'"), toc_spec);
      *input_line_pointer = c;
      input_line_pointer = start;
      return 0;
    }

  /* Now find the ']'.  */
  *input_line_pointer = c;

  SKIP_WHITESPACE_AFTER_NAME ();	/* leading whitespace could be there.  */
  c = *input_line_pointer++; /* input_line_pointer->past char in c.  */

  if (c != ']')
    {
      as_bad (_("syntax error: expected `]', found  `%c'"), c);
      input_line_pointer = start;
      return 0;
    }

  *toc_kind = t;
  return 1;
}
#endif

#if defined (OBJ_XCOFF) || defined (OBJ_ELF)
/* See whether a symbol is in the TOC section.  */

static int
ppc_is_toc_sym (symbolS *sym)
{
#ifdef OBJ_XCOFF
  return (symbol_get_tc (sym)->symbol_class == XMC_TC
	  || symbol_get_tc (sym)->symbol_class == XMC_TC0);
#endif
#ifdef OBJ_ELF
  const char *sname = segment_name (S_GET_SEGMENT (sym));
  if (ppc_obj64)
    return strcmp (sname, ".toc") == 0;
  else
    return strcmp (sname, ".got") == 0;
#endif
}
#endif /* defined (OBJ_XCOFF) || defined (OBJ_ELF) */


#ifdef OBJ_ELF
#define APUID(a,v)	((((a) & 0xffff) << 16) | ((v) & 0xffff))
static void
ppc_apuinfo_section_add (unsigned int apu, unsigned int version)
{
  unsigned int i;

  /* Check we don't already exist.  */
  for (i = 0; i < ppc_apuinfo_num; i++)
    if (ppc_apuinfo_list[i] == APUID (apu, version))
      return;

  if (ppc_apuinfo_num == ppc_apuinfo_num_alloc)
    {
      if (ppc_apuinfo_num_alloc == 0)
	{
	  ppc_apuinfo_num_alloc = 4;
	  ppc_apuinfo_list = XNEWVEC (unsigned long, ppc_apuinfo_num_alloc);
	}
      else
	{
	  ppc_apuinfo_num_alloc += 4;
	  ppc_apuinfo_list = XRESIZEVEC (unsigned long, ppc_apuinfo_list,
					 ppc_apuinfo_num_alloc);
	}
    }
  ppc_apuinfo_list[ppc_apuinfo_num++] = APUID (apu, version);
}
#undef APUID
#endif

/* Various frobbings of labels and their addresses.  */

/* Symbols labelling the current insn.  */
struct insn_label_list
{
  struct insn_label_list *next;
  symbolS *label;
};

static struct insn_label_list *insn_labels;
static struct insn_label_list *free_insn_labels;

static void
ppc_record_label (symbolS *sym)
{
  struct insn_label_list *l;

  if (free_insn_labels == NULL)
    l = XNEW (struct insn_label_list);
  else
    {
      l = free_insn_labels;
      free_insn_labels = l->next;
    }

  l->label = sym;
  l->next = insn_labels;
  insn_labels = l;
}

static void
ppc_clear_labels (void)
{
  while (insn_labels != NULL)
    {
      struct insn_label_list *l = insn_labels;
      insn_labels = l->next;
      l->next = free_insn_labels;
      free_insn_labels = l;
    }
}

void
ppc_start_line_hook (void)
{
  ppc_clear_labels ();
}

void
ppc_new_dot_label (symbolS *sym)
{
  ppc_record_label (sym);
#ifdef OBJ_XCOFF
  /* Anchor this label to the current csect for relocations.  */
  symbol_get_tc (sym)->within = ppc_current_csect;
#endif
}

void
ppc_frob_label (symbolS *sym)
{
  ppc_record_label (sym);

#ifdef OBJ_XCOFF
  /* Set the class of a label based on where it is defined.  This handles
     symbols without suffixes.  Also, move the symbol so that it follows
     the csect symbol.  */
  if (ppc_current_csect != (symbolS *) NULL)
    {
      if (symbol_get_tc (sym)->symbol_class == -1)
	symbol_get_tc (sym)->symbol_class = symbol_get_tc (ppc_current_csect)->symbol_class;

      symbol_remove (sym, &symbol_rootP, &symbol_lastP);
      symbol_append (sym, symbol_get_tc (ppc_current_csect)->within,
		     &symbol_rootP, &symbol_lastP);
      symbol_get_tc (ppc_current_csect)->within = sym;
      symbol_get_tc (sym)->within = ppc_current_csect;
    }
#endif

#ifdef OBJ_ELF
  dwarf2_emit_label (sym);
#endif
}

/* We need to keep a list of fixups.  We can't simply generate them as
   we go, because that would require us to first create the frag, and
   that would screw up references to ``.''.  */

struct ppc_fixup
{
  expressionS exp;
  int opindex;
  bfd_reloc_code_real_type reloc;
};

#define MAX_INSN_FIXUPS (5)

/* Return the field size operated on by RELOC, and whether it is
   pc-relative in PC_RELATIVE.  */

static unsigned int
fixup_size (bfd_reloc_code_real_type reloc, bfd_boolean *pc_relative)
{
  unsigned int size = 0;
  bfd_boolean pcrel = FALSE;

  switch (reloc)
    {
      /* This switch statement must handle all BFD_RELOC values
	 possible in instruction fixups.  As is, it handles all
	 BFD_RELOC values used in bfd/elf64-ppc.c, bfd/elf32-ppc.c,
	 bfd/coff-ppc, bfd/coff-rs6000.c and bfd/coff64-rs6000.c.
	 Overkill since data and marker relocs need not be handled
	 here, but this way we can be sure a needed fixup reloc isn't
	 accidentally omitted.  */
    case BFD_RELOC_PPC_EMB_MRKREF:
    case BFD_RELOC_VTABLE_ENTRY:
    case BFD_RELOC_VTABLE_INHERIT:
      break;

    case BFD_RELOC_8:
      size = 1;
      break;

    case BFD_RELOC_16:
    case BFD_RELOC_16_BASEREL:
    case BFD_RELOC_16_GOTOFF:
    case BFD_RELOC_GPREL16:
    case BFD_RELOC_HI16:
    case BFD_RELOC_HI16_BASEREL:
    case BFD_RELOC_HI16_GOTOFF:
    case BFD_RELOC_HI16_PLTOFF:
    case BFD_RELOC_HI16_S:
    case BFD_RELOC_HI16_S_BASEREL:
    case BFD_RELOC_HI16_S_GOTOFF:
    case BFD_RELOC_HI16_S_PLTOFF:
    case BFD_RELOC_LO16:
    case BFD_RELOC_LO16_BASEREL:
    case BFD_RELOC_LO16_GOTOFF:
    case BFD_RELOC_LO16_PLTOFF:
    case BFD_RELOC_PPC64_ADDR16_DS:
    case BFD_RELOC_PPC64_ADDR16_HIGH:
    case BFD_RELOC_PPC64_ADDR16_HIGHA:
    case BFD_RELOC_PPC64_ADDR16_HIGHER34:
    case BFD_RELOC_PPC64_ADDR16_HIGHERA34:
    case BFD_RELOC_PPC64_ADDR16_HIGHEST34:
    case BFD_RELOC_PPC64_ADDR16_HIGHESTA34:
    case BFD_RELOC_PPC64_ADDR16_LO_DS:
    case BFD_RELOC_PPC64_DTPREL16_DS:
    case BFD_RELOC_PPC64_DTPREL16_HIGH:
    case BFD_RELOC_PPC64_DTPREL16_HIGHA:
    case BFD_RELOC_PPC64_DTPREL16_HIGHER:
    case BFD_RELOC_PPC64_DTPREL16_HIGHERA:
    case BFD_RELOC_PPC64_DTPREL16_HIGHEST:
    case BFD_RELOC_PPC64_DTPREL16_HIGHESTA:
    case BFD_RELOC_PPC64_DTPREL16_LO_DS:
    case BFD_RELOC_PPC64_GOT16_DS:
    case BFD_RELOC_PPC64_GOT16_LO_DS:
    case BFD_RELOC_PPC64_HIGHER:
    case BFD_RELOC_PPC64_HIGHER_S:
    case BFD_RELOC_PPC64_HIGHEST:
    case BFD_RELOC_PPC64_HIGHEST_S:
    case BFD_RELOC_PPC64_PLT16_LO_DS:
    case BFD_RELOC_PPC64_PLTGOT16:
    case BFD_RELOC_PPC64_PLTGOT16_DS:
    case BFD_RELOC_PPC64_PLTGOT16_HA:
    case BFD_RELOC_PPC64_PLTGOT16_HI:
    case BFD_RELOC_PPC64_PLTGOT16_LO:
    case BFD_RELOC_PPC64_PLTGOT16_LO_DS:
    case BFD_RELOC_PPC64_SECTOFF_DS:
    case BFD_RELOC_PPC64_SECTOFF_LO_DS:
    case BFD_RELOC_PPC64_TOC16_DS:
    case BFD_RELOC_PPC64_TOC16_HA:
    case BFD_RELOC_PPC64_TOC16_HI:
    case BFD_RELOC_PPC64_TOC16_LO:
    case BFD_RELOC_PPC64_TOC16_LO_DS:
    case BFD_RELOC_PPC64_TPREL16_DS:
    case BFD_RELOC_PPC64_TPREL16_HIGH:
    case BFD_RELOC_PPC64_TPREL16_HIGHA:
    case BFD_RELOC_PPC64_TPREL16_HIGHER:
    case BFD_RELOC_PPC64_TPREL16_HIGHERA:
    case BFD_RELOC_PPC64_TPREL16_HIGHEST:
    case BFD_RELOC_PPC64_TPREL16_HIGHESTA:
    case BFD_RELOC_PPC64_TPREL16_LO_DS:
#ifdef OBJ_XCOFF
    case BFD_RELOC_PPC_BA16:
#endif
    case BFD_RELOC_PPC_DTPREL16:
    case BFD_RELOC_PPC_DTPREL16_HA:
    case BFD_RELOC_PPC_DTPREL16_HI:
    case BFD_RELOC_PPC_DTPREL16_LO:
    case BFD_RELOC_PPC_EMB_NADDR16:
    case BFD_RELOC_PPC_EMB_NADDR16_HA:
    case BFD_RELOC_PPC_EMB_NADDR16_HI:
    case BFD_RELOC_PPC_EMB_NADDR16_LO:
    case BFD_RELOC_PPC_EMB_RELSDA:
    case BFD_RELOC_PPC_EMB_RELSEC16:
    case BFD_RELOC_PPC_EMB_RELST_LO:
    case BFD_RELOC_PPC_EMB_RELST_HI:
    case BFD_RELOC_PPC_EMB_RELST_HA:
    case BFD_RELOC_PPC_EMB_SDA2I16:
    case BFD_RELOC_PPC_EMB_SDA2REL:
    case BFD_RELOC_PPC_EMB_SDAI16:
    case BFD_RELOC_PPC_GOT_DTPREL16:
    case BFD_RELOC_PPC_GOT_DTPREL16_HA:
    case BFD_RELOC_PPC_GOT_DTPREL16_HI:
    case BFD_RELOC_PPC_GOT_DTPREL16_LO:
    case BFD_RELOC_PPC_GOT_TLSGD16:
    case BFD_RELOC_PPC_GOT_TLSGD16_HA:
    case BFD_RELOC_PPC_GOT_TLSGD16_HI:
    case BFD_RELOC_PPC_GOT_TLSGD16_LO:
    case BFD_RELOC_PPC_GOT_TLSLD16:
    case BFD_RELOC_PPC_GOT_TLSLD16_HA:
    case BFD_RELOC_PPC_GOT_TLSLD16_HI:
    case BFD_RELOC_PPC_GOT_TLSLD16_LO:
    case BFD_RELOC_PPC_GOT_TPREL16:
    case BFD_RELOC_PPC_GOT_TPREL16_HA:
    case BFD_RELOC_PPC_GOT_TPREL16_HI:
    case BFD_RELOC_PPC_GOT_TPREL16_LO:
    case BFD_RELOC_PPC_TOC16:
    case BFD_RELOC_PPC_TPREL16:
    case BFD_RELOC_PPC_TPREL16_HA:
    case BFD_RELOC_PPC_TPREL16_HI:
    case BFD_RELOC_PPC_TPREL16_LO:
      size = 2;
      break;

    case BFD_RELOC_16_PCREL:
    case BFD_RELOC_HI16_PCREL:
    case BFD_RELOC_HI16_S_PCREL:
    case BFD_RELOC_LO16_PCREL:
    case BFD_RELOC_PPC64_REL16_HIGH:
    case BFD_RELOC_PPC64_REL16_HIGHA:
    case BFD_RELOC_PPC64_REL16_HIGHER:
    case BFD_RELOC_PPC64_REL16_HIGHER34:
    case BFD_RELOC_PPC64_REL16_HIGHERA:
    case BFD_RELOC_PPC64_REL16_HIGHERA34:
    case BFD_RELOC_PPC64_REL16_HIGHEST:
    case BFD_RELOC_PPC64_REL16_HIGHEST34:
    case BFD_RELOC_PPC64_REL16_HIGHESTA:
    case BFD_RELOC_PPC64_REL16_HIGHESTA34:
#ifdef OBJ_XCOFF
    case BFD_RELOC_PPC_B16:
#endif
    case BFD_RELOC_PPC_VLE_REL8:
      size = 2;
      pcrel = TRUE;
      break;

    case BFD_RELOC_16_GOT_PCREL: /* coff reloc, bad name re size.  */
    case BFD_RELOC_32:
    case BFD_RELOC_32_GOTOFF:
    case BFD_RELOC_32_PLTOFF:
#ifdef OBJ_XCOFF
    case BFD_RELOC_CTOR:
#endif
    case BFD_RELOC_PPC64_ENTRY:
    case BFD_RELOC_PPC_16DX_HA:
#ifndef OBJ_XCOFF
    case BFD_RELOC_PPC_BA16:
#endif
    case BFD_RELOC_PPC_BA16_BRNTAKEN:
    case BFD_RELOC_PPC_BA16_BRTAKEN:
    case BFD_RELOC_PPC_BA26:
    case BFD_RELOC_PPC_EMB_BIT_FLD:
    case BFD_RELOC_PPC_EMB_NADDR32:
    case BFD_RELOC_PPC_EMB_SDA21:
    case BFD_RELOC_PPC_TLS:
    case BFD_RELOC_PPC_TLSGD:
    case BFD_RELOC_PPC_TLSLD:
    case BFD_RELOC_PPC_VLE_HA16A:
    case BFD_RELOC_PPC_VLE_HA16D:
    case BFD_RELOC_PPC_VLE_HI16A:
    case BFD_RELOC_PPC_VLE_HI16D:
    case BFD_RELOC_PPC_VLE_LO16A:
    case BFD_RELOC_PPC_VLE_LO16D:
    case BFD_RELOC_PPC_VLE_SDA21:
    case BFD_RELOC_PPC_VLE_SDA21_LO:
    case BFD_RELOC_PPC_VLE_SDAREL_HA16A:
    case BFD_RELOC_PPC_VLE_SDAREL_HA16D:
    case BFD_RELOC_PPC_VLE_SDAREL_HI16A:
    case BFD_RELOC_PPC_VLE_SDAREL_HI16D:
    case BFD_RELOC_PPC_VLE_SDAREL_LO16A:
    case BFD_RELOC_PPC_VLE_SDAREL_LO16D:
    case BFD_RELOC_PPC64_TLS_PCREL:
    case BFD_RELOC_RVA:
      size = 4;
      break;

    case BFD_RELOC_24_PLT_PCREL:
    case BFD_RELOC_32_PCREL:
    case BFD_RELOC_32_PLT_PCREL:
    case BFD_RELOC_PPC64_REL24_NOTOC:
#ifndef OBJ_XCOFF
    case BFD_RELOC_PPC_B16:
#endif
    case BFD_RELOC_PPC_B16_BRNTAKEN:
    case BFD_RELOC_PPC_B16_BRTAKEN:
    case BFD_RELOC_PPC_B26:
    case BFD_RELOC_PPC_LOCAL24PC:
    case BFD_RELOC_PPC_REL16DX_HA:
    case BFD_RELOC_PPC_VLE_REL15:
    case BFD_RELOC_PPC_VLE_REL24:
      size = 4;
      pcrel = TRUE;
      break;

#ifndef OBJ_XCOFF
    case BFD_RELOC_CTOR:
#endif
    case BFD_RELOC_PPC_COPY:
    case BFD_RELOC_PPC_DTPMOD:
    case BFD_RELOC_PPC_DTPREL:
    case BFD_RELOC_PPC_GLOB_DAT:
    case BFD_RELOC_PPC_TPREL:
      size = ppc_obj64 ? 8 : 4;
      break;

    case BFD_RELOC_64:
    case BFD_RELOC_64_PLTOFF:
    case BFD_RELOC_PPC64_ADDR64_LOCAL:
    case BFD_RELOC_PPC64_D28:
    case BFD_RELOC_PPC64_D34:
    case BFD_RELOC_PPC64_D34_LO:
    case BFD_RELOC_PPC64_D34_HI30:
    case BFD_RELOC_PPC64_D34_HA30:
    case BFD_RELOC_PPC64_TPREL34:
    case BFD_RELOC_PPC64_DTPREL34:
    case BFD_RELOC_PPC64_TOC:
      size = 8;
      break;

    case BFD_RELOC_64_PCREL:
    case BFD_RELOC_64_PLT_PCREL:
    case BFD_RELOC_PPC64_GOT_PCREL34:
    case BFD_RELOC_PPC64_GOT_TLSGD34:
    case BFD_RELOC_PPC64_GOT_TLSLD34:
    case BFD_RELOC_PPC64_GOT_TPREL34:
    case BFD_RELOC_PPC64_GOT_DTPREL34:
    case BFD_RELOC_PPC64_PCREL28:
    case BFD_RELOC_PPC64_PCREL34:
    case BFD_RELOC_PPC64_PLT_PCREL34:
      size = 8;
      pcrel = TRUE;
      break;

    default:
      abort ();
    }

  if (ENABLE_CHECKING)
    {
      reloc_howto_type *reloc_howto = bfd_reloc_type_lookup (stdoutput, reloc);
      if (reloc_howto != NULL
	  && (size != bfd_get_reloc_size (reloc_howto)
	      || pcrel != reloc_howto->pc_relative))
	{
	  as_bad (_("%s howto doesn't match size/pcrel in gas"),
		  reloc_howto->name);
	  abort ();
	}
    }
  *pc_relative = pcrel;
  return size;
}

#ifdef OBJ_ELF
/* If we have parsed a call to __tls_get_addr, parse an argument like
   (gd0@tlsgd).  *STR is the leading parenthesis on entry.  If an arg
   is successfully parsed, *STR is updated past the trailing
   parenthesis and trailing white space, and *TLS_FIX contains the
   reloc and arg expression.  */

static int
parse_tls_arg (char **str, const expressionS *exp, struct ppc_fixup *tls_fix)
{
  const char *sym_name = S_GET_NAME (exp->X_add_symbol);
  if (sym_name[0] == '.')
    ++sym_name;

  tls_fix->reloc = BFD_RELOC_NONE;
  if (strcasecmp (sym_name, "__tls_get_addr") == 0)
    {
      char *hold = input_line_pointer;
      input_line_pointer = *str + 1;
      expression (&tls_fix->exp);
      if (tls_fix->exp.X_op == O_symbol)
	{
	  if (strncasecmp (input_line_pointer, "@tlsgd)", 7) == 0)
	    tls_fix->reloc = BFD_RELOC_PPC_TLSGD;
	  else if (strncasecmp (input_line_pointer, "@tlsld)", 7) == 0)
	    tls_fix->reloc = BFD_RELOC_PPC_TLSLD;
	  if (tls_fix->reloc != BFD_RELOC_NONE)
	    {
	      input_line_pointer += 7;
	      SKIP_WHITESPACE ();
	      *str = input_line_pointer;
	    }
	}
      input_line_pointer = hold;
    }
  return tls_fix->reloc != BFD_RELOC_NONE;
}
#endif

/* This routine is called for each instruction to be assembled.  */

void
md_assemble (char *str)
{
  char *s;
  const struct powerpc_opcode *opcode;
  uint64_t insn;
  const unsigned char *opindex_ptr;
  int need_paren;
  int next_opindex;
  struct ppc_fixup fixups[MAX_INSN_FIXUPS];
  int fc;
  char *f;
  int addr_mask;
  int i;
  unsigned int insn_length;

  /* Get the opcode.  */
  for (s = str; *s != '\0' && ! ISSPACE (*s); s++)
    ;
  if (*s != '\0')
    *s++ = '\0';

  /* Look up the opcode in the hash table.  */
  opcode = (const struct powerpc_opcode *) hash_find (ppc_hash, str);
  if (opcode == (const struct powerpc_opcode *) NULL)
    {
      const struct powerpc_macro *macro;

      macro = (const struct powerpc_macro *) hash_find (ppc_macro_hash, str);
      if (macro == (const struct powerpc_macro *) NULL)
	as_bad (_("unrecognized opcode: `%s'"), str);
      else
	ppc_macro (s, macro);

      ppc_clear_labels ();
      return;
    }

  insn = opcode->opcode;

  str = s;
  while (ISSPACE (*str))
    ++str;

  /* PowerPC operands are just expressions.  The only real issue is
     that a few operand types are optional.  If an instruction has
     multiple optional operands and one is omitted, then all optional
     operands past the first omitted one must also be omitted.  */
  int num_optional_operands = 0;
  int num_optional_provided = 0;

  /* Gather the operands.  */
  need_paren = 0;
  next_opindex = 0;
  fc = 0;
  for (opindex_ptr = opcode->operands; *opindex_ptr != 0; opindex_ptr++)
    {
      const struct powerpc_operand *operand;
      const char *errmsg;
      char *hold;
      expressionS ex;
      char endc;

      if (next_opindex == 0)
	operand = &powerpc_operands[*opindex_ptr];
      else
	{
	  operand = &powerpc_operands[next_opindex];
	  next_opindex = 0;
	}
      errmsg = NULL;

      /* If this is an optional operand, and we are skipping it, just
	 insert the default value, usually a zero.  */
      if ((operand->flags & PPC_OPERAND_OPTIONAL) != 0
	  && !((operand->flags & PPC_OPERAND_OPTIONAL32) != 0 && ppc_obj64))
	{
	  if (num_optional_operands == 0)
	    {
	      const unsigned char *optr;
	      int total = 0;
	      int provided = 0;
	      int omitted;

	      s = str;
	      for (optr = opindex_ptr; *optr != 0; optr++)
		{
		  const struct powerpc_operand *op;
		  op = &powerpc_operands[*optr];

		  ++total;

		  if ((op->flags & PPC_OPERAND_OPTIONAL) != 0
		      && !((op->flags & PPC_OPERAND_OPTIONAL32) != 0
			   && ppc_obj64))
		    ++num_optional_operands;

		  if (s != NULL && *s != '\0')
		    {
		      ++provided;

		      /* Look for the start of the next operand.  */
		      if ((op->flags & PPC_OPERAND_PARENS) != 0)
			s = strpbrk (s, "(,");
		      else
			s = strchr (s, ',');

		      if (s != NULL)
			++s;
		    }
		}
	      omitted = total - provided;
	      num_optional_provided = num_optional_operands - omitted;
	    }
	  if (--num_optional_provided < 0)
	    {
	      int64_t val = ppc_optional_operand_value (operand, insn, ppc_cpu,
							num_optional_provided);
	      if (operand->insert)
		{
		  insn = (*operand->insert) (insn, val, ppc_cpu, &errmsg);
		  if (errmsg != (const char *) NULL)
		    as_bad ("%s", errmsg);
		}
	      else if (operand->shift >= 0)
		insn |= (val & operand->bitm) << operand->shift;
	      else
		insn |= (val & operand->bitm) >> -operand->shift;

	      if ((operand->flags & PPC_OPERAND_NEXT) != 0)
		next_opindex = *opindex_ptr + 1;
	      continue;
	    }
	}

      /* Gather the operand.  */
      hold = input_line_pointer;
      input_line_pointer = str;

#ifdef TE_PE
      if (*input_line_pointer == '[')
	{
	  /* We are expecting something like the second argument here:
	   *
	   *    lwz r4,[toc].GS.0.static_int(rtoc)
	   *           ^^^^^^^^^^^^^^^^^^^^^^^^^^^
	   * The argument following the `]' must be a symbol name, and the
	   * register must be the toc register: 'rtoc' or '2'
	   *
	   * The effect is to 0 as the displacement field
	   * in the instruction, and issue an IMAGE_REL_PPC_TOCREL16 (or
	   * the appropriate variation) reloc against it based on the symbol.
	   * The linker will build the toc, and insert the resolved toc offset.
	   *
	   * Note:
	   * o The size of the toc entry is currently assumed to be
	   *   32 bits. This should not be assumed to be a hard coded
	   *   number.
	   * o In an effort to cope with a change from 32 to 64 bits,
	   *   there are also toc entries that are specified to be
	   *   either 32 or 64 bits:
	   *     lwz r4,[toc32].GS.0.static_int(rtoc)
	   *     lwz r4,[toc64].GS.0.static_int(rtoc)
	   *   These demand toc entries of the specified size, and the
	   *   instruction probably requires it.
	   */

	  int valid_toc;
	  enum toc_size_qualifier toc_kind;
	  bfd_reloc_code_real_type toc_reloc;

	  /* Go parse off the [tocXX] part.  */
	  valid_toc = parse_toc_entry (&toc_kind);

	  if (!valid_toc)
	    {
	      ignore_rest_of_line ();
	      break;
	    }

	  /* Now get the symbol following the ']'.  */
	  expression (&ex);

	  switch (toc_kind)
	    {
	    case default_toc:
	      /* In this case, we may not have seen the symbol yet,
		 since  it is allowed to appear on a .extern or .globl
		 or just be a label in the .data section.  */
	      toc_reloc = BFD_RELOC_PPC_TOC16;
	      break;
	    case data_in_toc:
	      /* 1. The symbol must be defined and either in the toc
		 section, or a global.
		 2. The reloc generated must have the TOCDEFN flag set
		 in upper bit mess of the reloc type.
		 FIXME: It's a little confusing what the tocv
		 qualifier can be used for.  At the very least, I've
		 seen three uses, only one of which I'm sure I can
		 explain.  */
	      if (ex.X_op == O_symbol)
		{
		  gas_assert (ex.X_add_symbol != NULL);
		  if (symbol_get_bfdsym (ex.X_add_symbol)->section
		      != tocdata_section)
		    {
		      as_bad (_("[tocv] symbol is not a toc symbol"));
		    }
		}

	      toc_reloc = BFD_RELOC_PPC_TOC16;
	      break;
	    case must_be_32:
	      /* FIXME: these next two specifically specify 32/64 bit
		 toc entries.  We don't support them today.  Is this
		 the right way to say that?  */
	      toc_reloc = BFD_RELOC_NONE;
	      as_bad (_("unimplemented toc32 expression modifier"));
	      break;
	    case must_be_64:
	      /* FIXME: see above.  */
	      toc_reloc = BFD_RELOC_NONE;
	      as_bad (_("unimplemented toc64 expression modifier"));
	      break;
	    default:
	      fprintf (stderr,
		       _("Unexpected return value [%d] from parse_toc_entry!\n"),
		       toc_kind);
	      abort ();
	      break;
	    }

	  /* We need to generate a fixup for this expression.  */
	  if (fc >= MAX_INSN_FIXUPS)
	    as_fatal (_("too many fixups"));

	  fixups[fc].reloc = toc_reloc;
	  fixups[fc].exp = ex;
	  fixups[fc].opindex = *opindex_ptr;
	  ++fc;

	  /* Ok. We've set up the fixup for the instruction. Now make it
	     look like the constant 0 was found here.  */
	  ex.X_unsigned = 1;
	  ex.X_op = O_constant;
	  ex.X_add_number = 0;
	  ex.X_add_symbol = NULL;
	  ex.X_op_symbol = NULL;
	}

      else
#endif		/* TE_PE */
	{
	  if ((reg_names_p
               && (((operand->flags & PPC_OPERAND_CR_BIT) != 0)
		   || ((operand->flags & PPC_OPERAND_CR_REG) != 0)))
	      || !register_name (&ex))
	    {
	      char save_lex = lex_type['%'];

	      if (((operand->flags & PPC_OPERAND_CR_REG) != 0)
		  || (operand->flags & PPC_OPERAND_CR_BIT) != 0)
		{
		  cr_operand = TRUE;
		  lex_type['%'] |= LEX_BEGIN_NAME;
		}
	      expression (&ex);
	      cr_operand = FALSE;
	      lex_type['%'] = save_lex;
	    }
	}

      str = input_line_pointer;
      input_line_pointer = hold;

      if (ex.X_op == O_illegal)
	as_bad (_("illegal operand"));
      else if (ex.X_op == O_absent)
	as_bad (_("missing operand"));
      else if (ex.X_op == O_register)
	{
	  if ((ex.X_md
	       & ~operand->flags
	       & (PPC_OPERAND_GPR | PPC_OPERAND_FPR | PPC_OPERAND_VR
		  | PPC_OPERAND_VSR | PPC_OPERAND_CR_BIT | PPC_OPERAND_CR_REG
		  | PPC_OPERAND_SPR | PPC_OPERAND_GQR)) != 0
	      && !((ex.X_md & PPC_OPERAND_GPR) != 0
		   && ex.X_add_number != 0
		   && (operand->flags & PPC_OPERAND_GPR_0) != 0))
	    as_warn (_("invalid register expression"));
	  insn = ppc_insert_operand (insn, operand, ex.X_add_number,
				     ppc_cpu, (char *) NULL, 0);
	}
      else if (ex.X_op == O_constant)
	{
#ifdef OBJ_ELF
	  /* Allow @HA, @L, @H on constants.  */
	  bfd_reloc_code_real_type reloc;
	  char *orig_str = str;

	  if ((reloc = ppc_elf_suffix (&str, &ex)) != BFD_RELOC_NONE)
	    switch (reloc)
	      {
	      default:
		str = orig_str;
		break;

	      case BFD_RELOC_LO16:
		ex.X_add_number &= 0xffff;
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;

	      case BFD_RELOC_HI16:
		if (REPORT_OVERFLOW_HI && ppc_obj64)
		  {
		    /* PowerPC64 @h is tested for overflow.  */
		    ex.X_add_number = (addressT) ex.X_add_number >> 16;
		    if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		      {
			addressT sign = (((addressT) -1 >> 16) + 1) >> 1;
			ex.X_add_number
			  = ((addressT) ex.X_add_number ^ sign) - sign;
		      }
		    break;
		  }
		/* Fallthru */

	      case BFD_RELOC_PPC64_ADDR16_HIGH:
		ex.X_add_number = PPC_HI (ex.X_add_number);
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;

	      case BFD_RELOC_HI16_S:
		if (REPORT_OVERFLOW_HI && ppc_obj64)
		  {
		    /* PowerPC64 @ha is tested for overflow.  */
		    ex.X_add_number
		      = ((addressT) ex.X_add_number + 0x8000) >> 16;
		    if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		      {
			addressT sign = (((addressT) -1 >> 16) + 1) >> 1;
			ex.X_add_number
			  = ((addressT) ex.X_add_number ^ sign) - sign;
		      }
		    break;
		  }
		/* Fallthru */

	      case BFD_RELOC_PPC64_ADDR16_HIGHA:
		ex.X_add_number = PPC_HA (ex.X_add_number);
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;

	      case BFD_RELOC_PPC64_HIGHER:
		ex.X_add_number = PPC_HIGHER (ex.X_add_number);
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;

	      case BFD_RELOC_PPC64_HIGHER_S:
		ex.X_add_number = PPC_HIGHERA (ex.X_add_number);
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;

	      case BFD_RELOC_PPC64_HIGHEST:
		ex.X_add_number = PPC_HIGHEST (ex.X_add_number);
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;

	      case BFD_RELOC_PPC64_HIGHEST_S:
		ex.X_add_number = PPC_HIGHESTA (ex.X_add_number);
		if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
		  ex.X_add_number = SEX16 (ex.X_add_number);
		break;
	      }
#endif /* OBJ_ELF */
	  insn = ppc_insert_operand (insn, operand, ex.X_add_number,
				     ppc_cpu, (char *) NULL, 0);
	}
      else
	{
	  bfd_reloc_code_real_type reloc = BFD_RELOC_NONE;
#ifdef OBJ_ELF
	  /* Look for a __tls_get_addr arg using the insane old syntax.  */
	  if (ex.X_op == O_symbol && *str == '(' && fc < MAX_INSN_FIXUPS
	      && parse_tls_arg (&str, &ex, &fixups[fc]))
	    {
	      fixups[fc].opindex = *opindex_ptr;
	      ++fc;
	    }

	  if ((reloc = ppc_elf_suffix (&str, &ex)) != BFD_RELOC_NONE)
	    {
	      /* If VLE-mode convert LO/HI/HA relocations.  */
      	      if (opcode->flags & PPC_OPCODE_VLE)
		{
		  uint64_t tmp_insn = insn & opcode->mask;

		  int use_a_reloc = (tmp_insn == E_OR2I_INSN
				     || tmp_insn == E_AND2I_DOT_INSN
				     || tmp_insn == E_OR2IS_INSN
				     || tmp_insn == E_LI_INSN
				     || tmp_insn == E_LIS_INSN
				     || tmp_insn == E_AND2IS_DOT_INSN);


		  int use_d_reloc = (tmp_insn == E_ADD2I_DOT_INSN
				     || tmp_insn == E_ADD2IS_INSN
				     || tmp_insn == E_CMP16I_INSN
				     || tmp_insn == E_MULL2I_INSN
				     || tmp_insn == E_CMPL16I_INSN
				     || tmp_insn == E_CMPH16I_INSN
				     || tmp_insn == E_CMPHL16I_INSN);

		  switch (reloc)
		    {
		    default:
		      break;

		    case BFD_RELOC_PPC_EMB_SDA21:
		      reloc = BFD_RELOC_PPC_VLE_SDA21;
		      break;

		    case BFD_RELOC_LO16:
		      if (use_d_reloc)
			reloc = BFD_RELOC_PPC_VLE_LO16D;
		      else if (use_a_reloc)
			reloc = BFD_RELOC_PPC_VLE_LO16A;
		      break;

		    case BFD_RELOC_HI16:
		      if (use_d_reloc)
			reloc = BFD_RELOC_PPC_VLE_HI16D;
		      else if (use_a_reloc)
			reloc = BFD_RELOC_PPC_VLE_HI16A;
		      break;

		    case BFD_RELOC_HI16_S:
		      if (use_d_reloc)
			reloc = BFD_RELOC_PPC_VLE_HA16D;
		      else if (use_a_reloc)
			reloc = BFD_RELOC_PPC_VLE_HA16A;
		      break;

		    case BFD_RELOC_PPC_VLE_SDAREL_LO16A:
		      if (use_d_reloc)
			reloc = BFD_RELOC_PPC_VLE_SDAREL_LO16D;
		      break;

		    case BFD_RELOC_PPC_VLE_SDAREL_HI16A:
		      if (use_d_reloc)
			reloc = BFD_RELOC_PPC_VLE_SDAREL_HI16D;
		      break;

		    case BFD_RELOC_PPC_VLE_SDAREL_HA16A:
		      if (use_d_reloc)
			reloc = BFD_RELOC_PPC_VLE_SDAREL_HA16D;
		      break;
		    }
		}

	      /* TLS and other tweaks.  */
	      switch (reloc)
		{
		default:
		  break;

		case BFD_RELOC_PPC_TLS:
		case BFD_RELOC_PPC64_TLS_PCREL:
		  if (!_bfd_elf_ppc_at_tls_transform (opcode->opcode, 0))
		    as_bad (_("@tls may not be used with \"%s\" operands"),
			    opcode->name);
		  else if (operand->shift != 11)
		    as_bad (_("@tls may only be used in last operand"));
		  else
		    insn = ppc_insert_operand (insn, operand,
					       ppc_obj64 ? 13 : 2,
					       ppc_cpu, (char *) NULL, 0);
		  break;

		  /* We'll only use the 32 (or 64) bit form of these relocations
		     in constants.  Instructions get the 16 or 34 bit form.  */
		case BFD_RELOC_PPC_DTPREL:
		  if (operand->bitm == 0x3ffffffffULL)
		    reloc = BFD_RELOC_PPC64_DTPREL34;
		  else
		    reloc = BFD_RELOC_PPC_DTPREL16;
		  break;

		case BFD_RELOC_PPC_TPREL:
		  if (operand->bitm == 0x3ffffffffULL)
		    reloc = BFD_RELOC_PPC64_TPREL34;
		  else
		    reloc = BFD_RELOC_PPC_TPREL16;
		  break;

		case BFD_RELOC_PPC64_PCREL34:
		  if (operand->bitm == 0xfffffffULL)
		    {
		      reloc = BFD_RELOC_PPC64_PCREL28;
		      break;
		    }
		  /* Fall through.  */
		case BFD_RELOC_PPC64_GOT_PCREL34:
		case BFD_RELOC_PPC64_PLT_PCREL34:
		case BFD_RELOC_PPC64_GOT_TLSGD34:
		case BFD_RELOC_PPC64_GOT_TLSLD34:
		case BFD_RELOC_PPC64_GOT_TPREL34:
		case BFD_RELOC_PPC64_GOT_DTPREL34:
		  if (operand->bitm != 0x3ffffffffULL
		      || (operand->flags & PPC_OPERAND_NEGATIVE) != 0)
		    as_warn (_("%s unsupported on this instruction"), "@pcrel");
		  break;

		case BFD_RELOC_LO16:
		  if (operand->bitm == 0x3ffffffffULL
		      && (operand->flags & PPC_OPERAND_NEGATIVE) == 0)
		    reloc = BFD_RELOC_PPC64_D34_LO;
		  else if ((operand->bitm | 0xf) != 0xffff
			   || operand->shift != 0
			   || (operand->flags & PPC_OPERAND_NEGATIVE) != 0)
		    as_warn (_("%s unsupported on this instruction"), "@l");
		  break;

		case BFD_RELOC_HI16:
		  if (operand->bitm == 0x3ffffffffULL
		      && (operand->flags & PPC_OPERAND_NEGATIVE) == 0)
		    reloc = BFD_RELOC_PPC64_D34_HI30;
		  else if (operand->bitm != 0xffff
			   || operand->shift != 0
			   || (operand->flags & PPC_OPERAND_NEGATIVE) != 0)
		    as_warn (_("%s unsupported on this instruction"), "@h");
		  break;

		case BFD_RELOC_HI16_S:
		  if (operand->bitm == 0x3ffffffffULL
		      && (operand->flags & PPC_OPERAND_NEGATIVE) == 0)
		    reloc = BFD_RELOC_PPC64_D34_HA30;
		  else if (operand->bitm == 0xffff
			   && operand->shift == (int) PPC_OPSHIFT_INV
			   && opcode->opcode == (19 << 26) + (2 << 1))
		    /* addpcis.  */
		    reloc = BFD_RELOC_PPC_16DX_HA;
		  else if (operand->bitm != 0xffff
			   || operand->shift != 0
			   || (operand->flags & PPC_OPERAND_NEGATIVE) != 0)
		    as_warn (_("%s unsupported on this instruction"), "@ha");
		}
	    }
#endif /* OBJ_ELF */

	  if (reloc != BFD_RELOC_NONE)
	    ;
	  /* Determine a BFD reloc value based on the operand information.
	     We are only prepared to turn a few of the operands into
	     relocs.  */
	  else if ((operand->flags & (PPC_OPERAND_RELATIVE
				      | PPC_OPERAND_ABSOLUTE)) != 0
		   && operand->bitm == 0x3fffffc
		   && operand->shift == 0)
	    reloc = BFD_RELOC_PPC_B26;
	  else if ((operand->flags & (PPC_OPERAND_RELATIVE
				      | PPC_OPERAND_ABSOLUTE)) != 0
		   && operand->bitm == 0xfffc
		   && operand->shift == 0)
	    reloc = BFD_RELOC_PPC_B16;
	  else if ((operand->flags & PPC_OPERAND_RELATIVE) != 0
		   && operand->bitm == 0x1fe
		   && operand->shift == -1)
	    reloc = BFD_RELOC_PPC_VLE_REL8;
	  else if ((operand->flags & PPC_OPERAND_RELATIVE) != 0
		   && operand->bitm == 0xfffe
		   && operand->shift == 0)
	    reloc = BFD_RELOC_PPC_VLE_REL15;
	  else if ((operand->flags & PPC_OPERAND_RELATIVE) != 0
		   && operand->bitm == 0x1fffffe
		   && operand->shift == 0)
	    reloc = BFD_RELOC_PPC_VLE_REL24;
	  else if ((operand->flags & PPC_OPERAND_NEGATIVE) == 0
		   && (operand->bitm & 0xfff0) == 0xfff0
		   && operand->shift == 0)
	    {
	      reloc = BFD_RELOC_16;
#if defined OBJ_XCOFF || defined OBJ_ELF
	      /* Note: the symbol may be not yet defined.  */
	      if ((operand->flags & PPC_OPERAND_PARENS) != 0
		  && ppc_is_toc_sym (ex.X_add_symbol))
		{
		  reloc = BFD_RELOC_PPC_TOC16;
#ifdef OBJ_ELF
		  as_warn (_("assuming %s on symbol"),
			   ppc_obj64 ? "@toc" : "@xgot");
#endif
		}
#endif
	    }
	  else if (operand->bitm == 0x3ffffffffULL)
	    reloc = BFD_RELOC_PPC64_D34;
	  else if (operand->bitm == 0xfffffffULL)
	    reloc = BFD_RELOC_PPC64_D28;

	  /* For the absolute forms of branches, convert the PC
	     relative form back into the absolute.  */
	  if ((operand->flags & PPC_OPERAND_ABSOLUTE) != 0)
	    {
	      switch (reloc)
		{
		case BFD_RELOC_PPC_B26:
		  reloc = BFD_RELOC_PPC_BA26;
		  break;
		case BFD_RELOC_PPC_B16:
		  reloc = BFD_RELOC_PPC_BA16;
		  break;
#ifdef OBJ_ELF
		case BFD_RELOC_PPC_B16_BRTAKEN:
		  reloc = BFD_RELOC_PPC_BA16_BRTAKEN;
		  break;
		case BFD_RELOC_PPC_B16_BRNTAKEN:
		  reloc = BFD_RELOC_PPC_BA16_BRNTAKEN;
		  break;
#endif
		default:
		  break;
		}
	    }

#ifdef OBJ_ELF
	  switch (reloc)
	    {
	    case BFD_RELOC_PPC_TOC16:
	      toc_reloc_types |= has_small_toc_reloc;
	      break;
	    case BFD_RELOC_PPC64_TOC16_LO:
	    case BFD_RELOC_PPC64_TOC16_HI:
	    case BFD_RELOC_PPC64_TOC16_HA:
	      toc_reloc_types |= has_large_toc_reloc;
	      break;
	    default:
	      break;
	    }

	  if (ppc_obj64
	      && (operand->flags & (PPC_OPERAND_DS | PPC_OPERAND_DQ)) != 0)
	    {
	      switch (reloc)
		{
		case BFD_RELOC_16:
		  reloc = BFD_RELOC_PPC64_ADDR16_DS;
		  break;

		case BFD_RELOC_LO16:
		  reloc = BFD_RELOC_PPC64_ADDR16_LO_DS;
		  break;

		case BFD_RELOC_16_GOTOFF:
		  reloc = BFD_RELOC_PPC64_GOT16_DS;
		  break;

		case BFD_RELOC_LO16_GOTOFF:
		  reloc = BFD_RELOC_PPC64_GOT16_LO_DS;
		  break;

		case BFD_RELOC_LO16_PLTOFF:
		  reloc = BFD_RELOC_PPC64_PLT16_LO_DS;
		  break;

		case BFD_RELOC_16_BASEREL:
		  reloc = BFD_RELOC_PPC64_SECTOFF_DS;
		  break;

		case BFD_RELOC_LO16_BASEREL:
		  reloc = BFD_RELOC_PPC64_SECTOFF_LO_DS;
		  break;

		case BFD_RELOC_PPC_TOC16:
		  reloc = BFD_RELOC_PPC64_TOC16_DS;
		  break;

		case BFD_RELOC_PPC64_TOC16_LO:
		  reloc = BFD_RELOC_PPC64_TOC16_LO_DS;
		  break;

		case BFD_RELOC_PPC64_PLTGOT16:
		  reloc = BFD_RELOC_PPC64_PLTGOT16_DS;
		  break;

		case BFD_RELOC_PPC64_PLTGOT16_LO:
		  reloc = BFD_RELOC_PPC64_PLTGOT16_LO_DS;
		  break;

		case BFD_RELOC_PPC_DTPREL16:
		  reloc = BFD_RELOC_PPC64_DTPREL16_DS;
		  break;

		case BFD_RELOC_PPC_DTPREL16_LO:
		  reloc = BFD_RELOC_PPC64_DTPREL16_LO_DS;
		  break;

		case BFD_RELOC_PPC_TPREL16:
		  reloc = BFD_RELOC_PPC64_TPREL16_DS;
		  break;

		case BFD_RELOC_PPC_TPREL16_LO:
		  reloc = BFD_RELOC_PPC64_TPREL16_LO_DS;
		  break;

		case BFD_RELOC_PPC_GOT_DTPREL16:
		case BFD_RELOC_PPC_GOT_DTPREL16_LO:
		case BFD_RELOC_PPC_GOT_TPREL16:
		case BFD_RELOC_PPC_GOT_TPREL16_LO:
		  break;

		default:
		  as_bad (_("unsupported relocation for DS offset field"));
		  break;
		}
	    }

	  /* Look for a __tls_get_addr arg after any __tls_get_addr
	     modifiers like @plt.  This fixup must be emitted before
	     the usual call fixup.  */
	  if (ex.X_op == O_symbol && *str == '(' && fc < MAX_INSN_FIXUPS
	      && parse_tls_arg (&str, &ex, &fixups[fc]))
	    {
	      fixups[fc].opindex = *opindex_ptr;
	      ++fc;
	    }
#endif

	  /* We need to generate a fixup for this expression.  */
	  if (fc >= MAX_INSN_FIXUPS)
	    as_fatal (_("too many fixups"));
	  fixups[fc].exp = ex;
	  fixups[fc].opindex = *opindex_ptr;
	  fixups[fc].reloc = reloc;
	  ++fc;
	}

      if (need_paren)
	{
	  endc = ')';
	  need_paren = 0;
	  /* If expecting more operands, then we want to see "),".  */
	  if (*str == endc && opindex_ptr[1] != 0)
	    {
	      do
		++str;
	      while (ISSPACE (*str));
	      endc = ',';
	    }
	}
      else if ((operand->flags & PPC_OPERAND_PARENS) != 0)
	endc = '(';
      else
	endc = ',';

      /* The call to expression should have advanced str past any
	 whitespace.  */
      if (*str == endc)
	{
	  ++str;
	  if (endc == '(')
	    need_paren = 1;
	}
      else if (*str != '\0')
	{
	  as_bad (_("syntax error; found `%c', expected `%c'"), *str, endc);
	  break;
	}
      else if (endc == ')')
	{
	  as_bad (_("syntax error; end of line, expected `%c'"), endc);
	  break;
	}
    }

  while (ISSPACE (*str))
    ++str;

  if (*str != '\0')
    as_bad (_("junk at end of line: `%s'"), str);

#ifdef OBJ_ELF
  /* Do we need/want an APUinfo section? */
  if ((ppc_cpu & (PPC_OPCODE_E500 | PPC_OPCODE_E500MC | PPC_OPCODE_VLE)) != 0
      && !ppc_obj64)
    {
      /* These are all version "1".  */
      if (opcode->flags & PPC_OPCODE_SPE)
	ppc_apuinfo_section_add (PPC_APUINFO_SPE, 1);
      if (opcode->flags & PPC_OPCODE_ISEL)
	ppc_apuinfo_section_add (PPC_APUINFO_ISEL, 1);
      if (opcode->flags & PPC_OPCODE_EFS)
	ppc_apuinfo_section_add (PPC_APUINFO_EFS, 1);
      if (opcode->flags & PPC_OPCODE_BRLOCK)
	ppc_apuinfo_section_add (PPC_APUINFO_BRLOCK, 1);
      if (opcode->flags & PPC_OPCODE_PMR)
	ppc_apuinfo_section_add (PPC_APUINFO_PMR, 1);
      if (opcode->flags & PPC_OPCODE_CACHELCK)
	ppc_apuinfo_section_add (PPC_APUINFO_CACHELCK, 1);
      if (opcode->flags & PPC_OPCODE_RFMCI)
	ppc_apuinfo_section_add (PPC_APUINFO_RFMCI, 1);
      /* Only set the VLE flag if the instruction has been pulled via
         the VLE instruction set.  This way the flag is guaranteed to
         be set for VLE-only instructions or for VLE-only processors,
         however it'll remain clear for dual-mode instructions on
         dual-mode and, more importantly, standard-mode processors.  */
      if ((ppc_cpu & opcode->flags) == PPC_OPCODE_VLE)
	{
	  ppc_apuinfo_section_add (PPC_APUINFO_VLE, 1);
	  if (elf_section_data (now_seg) != NULL)
	    elf_section_data (now_seg)->this_hdr.sh_flags |= SHF_PPC_VLE;
	}
    }
#endif

  /* Write out the instruction.  */

  addr_mask = 3;
  if ((ppc_cpu & PPC_OPCODE_VLE) != 0)
    /* All instructions can start on a 2 byte boundary for VLE.  */
    addr_mask = 1;

  if (frag_now->insn_addr != addr_mask)
    {
      /* Don't emit instructions to a frag started for data, or for a
	 CPU differing in VLE mode.  Data is allowed to be misaligned,
	 and it's possible to start a new frag in the middle of
	 misaligned data.  */
      frag_wane (frag_now);
      frag_new (0);
    }

  /* Check that insns within the frag are aligned.  ppc_frag_check
     will ensure that the frag start address is aligned.  */
  if ((frag_now_fix () & addr_mask) != 0)
    as_bad (_("instruction address is not a multiple of %d"), addr_mask + 1);

  /* Differentiate between two, four, and eight byte insns.  */
  insn_length = 4;
  if ((ppc_cpu & PPC_OPCODE_VLE) != 0 && PPC_OP_SE_VLE (insn))
    insn_length = 2;
  else if ((opcode->flags & PPC_OPCODE_POWERXX) != 0
	   && PPC_PREFIX_P (insn))
    {
      struct insn_label_list *l;

      insn_length = 8;

      /* 8-byte prefix instructions are not allowed to cross 64-byte
	 boundaries.  */
      frag_align_code (6, 4);
      record_alignment (now_seg, 6);

      /* Update "dot" in any expressions used by this instruction, and
	 a label attached to the instruction.  By "attached" we mean
	 on the same source line as the instruction and without any
	 intervening semicolons.  */
      dot_value = frag_now_fix ();
      dot_frag = frag_now;
      for (l = insn_labels; l != NULL; l = l->next)
	{
	  symbol_set_frag (l->label, dot_frag);
	  S_SET_VALUE (l->label, dot_value);
	}
    }

  ppc_clear_labels ();

  f = frag_more (insn_length);
  frag_now->insn_addr = addr_mask;

  /* The prefix part of an 8-byte instruction always occupies the lower
     addressed word in a doubleword, regardless of endianness.  */
  if (!target_big_endian && insn_length == 8)
    {
      md_number_to_chars (f, PPC_GET_PREFIX (insn), 4);
      md_number_to_chars (f + 4, PPC_GET_SUFFIX (insn), 4);
    }
  else
    md_number_to_chars (f, insn, insn_length);

  last_insn = insn;
  last_seg = now_seg;
  last_subseg = now_subseg;

#ifdef OBJ_ELF
  dwarf2_emit_insn (insn_length);
#endif

  /* Create any fixups.  */
  for (i = 0; i < fc; i++)
    {
      fixS *fixP;
      if (fixups[i].reloc != BFD_RELOC_NONE)
	{
	  bfd_boolean pcrel;
	  unsigned int size = fixup_size (fixups[i].reloc, &pcrel);
	  int offset = target_big_endian ? (insn_length - size) : 0;

	  fixP = fix_new_exp (frag_now,
			      f - frag_now->fr_literal + offset,
			      size,
			      &fixups[i].exp,
			      pcrel,
			      fixups[i].reloc);
	}
      else
	{
	  const struct powerpc_operand *operand;

	  operand = &powerpc_operands[fixups[i].opindex];
	  fixP = fix_new_exp (frag_now,
			      f - frag_now->fr_literal,
			      insn_length,
			      &fixups[i].exp,
			      (operand->flags & PPC_OPERAND_RELATIVE) != 0,
			      BFD_RELOC_NONE);
	}
      fixP->fx_pcrel_adjust = fixups[i].opindex;
    }
}

/* Handle a macro.  Gather all the operands, transform them as
   described by the macro, and call md_assemble recursively.  All the
   operands are separated by commas; we don't accept parentheses
   around operands here.  */

static void
ppc_macro (char *str, const struct powerpc_macro *macro)
{
  char *operands[10];
  unsigned int count;
  char *s;
  unsigned int len;
  const char *format;
  unsigned int arg;
  char *send;
  char *complete;

  /* Gather the users operands into the operands array.  */
  count = 0;
  s = str;
  while (1)
    {
      if (count >= sizeof operands / sizeof operands[0])
	break;
      operands[count++] = s;
      s = strchr (s, ',');
      if (s == (char *) NULL)
	break;
      *s++ = '\0';
    }

  if (count != macro->operands)
    {
      as_bad (_("wrong number of operands"));
      return;
    }

  /* Work out how large the string must be (the size is unbounded
     because it includes user input).  */
  len = 0;
  format = macro->format;
  while (*format != '\0')
    {
      if (*format != '%')
	{
	  ++len;
	  ++format;
	}
      else
	{
	  arg = strtol (format + 1, &send, 10);
	  know (send != format && arg < count);
	  len += strlen (operands[arg]);
	  format = send;
	}
    }

  /* Put the string together.  */
  complete = s = XNEWVEC (char, len + 1);
  format = macro->format;
  while (*format != '\0')
    {
      if (*format != '%')
	*s++ = *format++;
      else
	{
	  arg = strtol (format + 1, &send, 10);
	  strcpy (s, operands[arg]);
	  s += strlen (s);
	  format = send;
	}
    }
  *s = '\0';

  /* Assemble the constructed instruction.  */
  md_assemble (complete);
  free (complete);
}

#ifdef OBJ_ELF
/* For ELF, add support for SHT_ORDERED.  */

int
ppc_section_type (char *str, size_t len)
{
  if (len == 7 && strncmp (str, "ordered", 7) == 0)
    return SHT_ORDERED;

  return -1;
}

int
ppc_section_flags (flagword flags, bfd_vma attr ATTRIBUTE_UNUSED, int type)
{
  if (type == SHT_ORDERED)
    flags |= SEC_ALLOC | SEC_LOAD | SEC_SORT_ENTRIES;

  return flags;
}

bfd_vma
ppc_elf_section_letter (int letter, const char **ptrmsg)
{
  if (letter == 'v')
    return SHF_PPC_VLE;

  *ptrmsg = _("bad .section directive: want a,e,v,w,x,M,S,G,T in string");
  return -1;
}
#endif /* OBJ_ELF */


/* Pseudo-op handling.  */

/* The .byte pseudo-op.  This is similar to the normal .byte
   pseudo-op, but it can also take a single ASCII string.  */

static void
ppc_byte (int ignore ATTRIBUTE_UNUSED)
{
  int count = 0;

  if (*input_line_pointer != '\"')
    {
      cons (1);
      return;
    }

  /* Gather characters.  A real double quote is doubled.  Unusual
     characters are not permitted.  */
  ++input_line_pointer;
  while (1)
    {
      char c;

      c = *input_line_pointer++;

      if (c == '\"')
	{
	  if (*input_line_pointer != '\"')
	    break;
	  ++input_line_pointer;
	}

      FRAG_APPEND_1_CHAR (c);
      ++count;
    }

  if (warn_476 && count != 0 && (now_seg->flags & SEC_CODE) != 0)
    as_warn (_("data in executable section"));
  demand_empty_rest_of_line ();
}

#ifdef OBJ_XCOFF

/* XCOFF specific pseudo-op handling.  */

/* This is set if we are creating a .stabx symbol, since we don't want
   to handle symbol suffixes for such symbols.  */
static bfd_boolean ppc_stab_symbol;

/* The .comm and .lcomm pseudo-ops for XCOFF.  XCOFF puts common
   symbols in the .bss segment as though they were local common
   symbols, and uses a different smclas.  The native Aix 4.3.3 assembler
   aligns .comm and .lcomm to 4 bytes.  */

static void
ppc_comm (int lcomm)
{
  asection *current_seg = now_seg;
  subsegT current_subseg = now_subseg;
  char *name;
  char endc;
  char *end_name;
  offsetT size;
  offsetT align;
  symbolS *lcomm_sym = NULL;
  symbolS *sym;
  char *pfrag;

  endc = get_symbol_name (&name);
  end_name = input_line_pointer;
  (void) restore_line_pointer (endc);

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing size"));
      ignore_rest_of_line ();
      return;
    }
  ++input_line_pointer;

  size = get_absolute_expression ();
  if (size < 0)
    {
      as_bad (_("negative size"));
      ignore_rest_of_line ();
      return;
    }

  if (! lcomm)
    {
      /* The third argument to .comm is the alignment.  */
      if (*input_line_pointer != ',')
	align = 2;
      else
	{
	  ++input_line_pointer;
	  align = get_absolute_expression ();
	  if (align <= 0)
	    {
	      as_warn (_("ignoring bad alignment"));
	      align = 2;
	    }
	}
    }
  else
    {
      char *lcomm_name;
      char lcomm_endc;

      /* The third argument to .lcomm appears to be the real local
	 common symbol to create.  References to the symbol named in
	 the first argument are turned into references to the third
	 argument.  */
      if (*input_line_pointer != ',')
	{
	  as_bad (_("missing real symbol name"));
	  ignore_rest_of_line ();
	  return;
	}
      ++input_line_pointer;

      lcomm_endc = get_symbol_name (&lcomm_name);

      lcomm_sym = symbol_find_or_make (lcomm_name);

      (void) restore_line_pointer (lcomm_endc);

      /* The fourth argument to .lcomm is the alignment.  */
      if (*input_line_pointer != ',')
	{
	  if (size <= 4)
	    align = 2;
	  else
	    align = 3;
	}
      else
	{
	  ++input_line_pointer;
	  align = get_absolute_expression ();
	  if (align <= 0)
	    {
	      as_warn (_("ignoring bad alignment"));
	      align = 2;
	    }
	}
    }

  *end_name = '\0';
  sym = symbol_find_or_make (name);
  *end_name = endc;

  if (S_IS_DEFINED (sym)
      || S_GET_VALUE (sym) != 0)
    {
      as_bad (_("attempt to redefine symbol"));
      ignore_rest_of_line ();
      return;
    }

  record_alignment (bss_section, align);

  if (! lcomm
      || ! S_IS_DEFINED (lcomm_sym))
    {
      symbolS *def_sym;
      offsetT def_size;

      if (! lcomm)
	{
	  def_sym = sym;
	  def_size = size;
	  S_SET_EXTERNAL (sym);
	}
      else
	{
	  symbol_get_tc (lcomm_sym)->output = 1;
	  def_sym = lcomm_sym;
	  def_size = 0;
	}

      subseg_set (bss_section, 1);
      frag_align (align, 0, 0);

      symbol_set_frag (def_sym, frag_now);
      pfrag = frag_var (rs_org, 1, 1, (relax_substateT) 0, def_sym,
			def_size, (char *) NULL);
      *pfrag = 0;
      S_SET_SEGMENT (def_sym, bss_section);
      symbol_get_tc (def_sym)->align = align;
    }
  else if (lcomm)
    {
      /* Align the size of lcomm_sym.  */
      symbol_get_frag (lcomm_sym)->fr_offset =
	((symbol_get_frag (lcomm_sym)->fr_offset + (1 << align) - 1)
	 &~ ((1 << align) - 1));
      if (align > symbol_get_tc (lcomm_sym)->align)
	symbol_get_tc (lcomm_sym)->align = align;
    }

  if (lcomm)
    {
      /* Make sym an offset from lcomm_sym.  */
      S_SET_SEGMENT (sym, bss_section);
      symbol_set_frag (sym, symbol_get_frag (lcomm_sym));
      S_SET_VALUE (sym, symbol_get_frag (lcomm_sym)->fr_offset);
      symbol_get_frag (lcomm_sym)->fr_offset += size;
    }

  subseg_set (current_seg, current_subseg);

  demand_empty_rest_of_line ();
}

/* The .csect pseudo-op.  This switches us into a different
   subsegment.  The first argument is a symbol whose value is the
   start of the .csect.  In COFF, csect symbols get special aux
   entries defined by the x_csect field of union internal_auxent.  The
   optional second argument is the alignment (the default is 2).  */

static void
ppc_csect (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;
  symbolS *sym;
  offsetT align;

  endc = get_symbol_name (&name);

  sym = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  if (S_GET_NAME (sym)[0] == '\0')
    {
      /* An unnamed csect is assumed to be [PR].  */
      symbol_get_tc (sym)->symbol_class = XMC_PR;
    }

  align = 2;
  if (*input_line_pointer == ',')
    {
      ++input_line_pointer;
      align = get_absolute_expression ();
    }

  ppc_change_csect (sym, align);

  demand_empty_rest_of_line ();
}

/* Change to a different csect.  */

static void
ppc_change_csect (symbolS *sym, offsetT align)
{
  if (S_IS_DEFINED (sym))
    subseg_set (S_GET_SEGMENT (sym), symbol_get_tc (sym)->subseg);
  else
    {
      symbolS **list_ptr;
      int after_toc;
      int hold_chunksize;
      symbolS *list;
      int is_code;
      segT sec;

      /* This is a new csect.  We need to look at the symbol class to
	 figure out whether it should go in the text section or the
	 data section.  */
      after_toc = 0;
      is_code = 0;
      switch (symbol_get_tc (sym)->symbol_class)
	{
	case XMC_PR:
	case XMC_RO:
	case XMC_DB:
	case XMC_GL:
	case XMC_XO:
	case XMC_SV:
	case XMC_TI:
	case XMC_TB:
	  S_SET_SEGMENT (sym, text_section);
	  symbol_get_tc (sym)->subseg = ppc_text_subsegment;
	  ++ppc_text_subsegment;
	  list_ptr = &ppc_text_csects;
	  is_code = 1;
	  break;
	case XMC_RW:
	case XMC_TC0:
	case XMC_TC:
	case XMC_DS:
	case XMC_UA:
	case XMC_BS:
	case XMC_UC:
	  if (ppc_toc_csect != NULL
	      && (symbol_get_tc (ppc_toc_csect)->subseg + 1
		  == ppc_data_subsegment))
	    after_toc = 1;
	  S_SET_SEGMENT (sym, data_section);
	  symbol_get_tc (sym)->subseg = ppc_data_subsegment;
	  ++ppc_data_subsegment;
	  list_ptr = &ppc_data_csects;
	  break;
	default:
	  abort ();
	}

      /* We set the obstack chunk size to a small value before
	 changing subsegments, so that we don't use a lot of memory
	 space for what may be a small section.  */
      hold_chunksize = chunksize;
      chunksize = 64;

      sec = subseg_new (segment_name (S_GET_SEGMENT (sym)),
			symbol_get_tc (sym)->subseg);

      chunksize = hold_chunksize;

      if (after_toc)
	ppc_after_toc_frag = frag_now;

      record_alignment (sec, align);
      if (is_code)
	frag_align_code (align, 0);
      else
	frag_align (align, 0, 0);

      symbol_set_frag (sym, frag_now);
      S_SET_VALUE (sym, (valueT) frag_now_fix ());

      symbol_get_tc (sym)->align = align;
      symbol_get_tc (sym)->output = 1;
      symbol_get_tc (sym)->within = sym;

      for (list = *list_ptr;
	   symbol_get_tc (list)->next != (symbolS *) NULL;
	   list = symbol_get_tc (list)->next)
	;
      symbol_get_tc (list)->next = sym;

      symbol_remove (sym, &symbol_rootP, &symbol_lastP);
      symbol_append (sym, symbol_get_tc (list)->within, &symbol_rootP,
		     &symbol_lastP);
    }

  ppc_current_csect = sym;
}

static void
ppc_change_debug_section (unsigned int idx, subsegT subseg)
{
  segT sec;
  flagword oldflags;
  const struct xcoff_dwsect_name *dw = &xcoff_dwsect_names[idx];

  sec = subseg_new (dw->name, subseg);
  oldflags = bfd_section_flags (sec);
  if (oldflags == SEC_NO_FLAGS)
    {
      /* Just created section.  */
      gas_assert (dw_sections[idx].sect == NULL);

      bfd_set_section_flags (sec, SEC_DEBUGGING);
      bfd_set_section_alignment (sec, 0);
      dw_sections[idx].sect = sec;
    }

  /* Not anymore in a csect.  */
  ppc_current_csect = NULL;
}

/* The .dwsect pseudo-op.  Defines a DWARF section.  Syntax is:
     .dwsect flag [, opt-label ]
*/

static void
ppc_dwsect (int ignore ATTRIBUTE_UNUSED)
{
  valueT flag;
  symbolS *opt_label;
  const struct xcoff_dwsect_name *dw;
  struct dw_subsection *subseg;
  struct dw_section *dws;
  int i;

  /* Find section.  */
  flag = get_absolute_expression ();
  dw = NULL;
  for (i = 0; i < XCOFF_DWSECT_NBR_NAMES; i++)
    if (xcoff_dwsect_names[i].flag == flag)
      {
        dw = &xcoff_dwsect_names[i];
        break;
      }

  /* Parse opt-label.  */
  if (*input_line_pointer == ',')
    {
      char *label;
      char c;

      ++input_line_pointer;

      c = get_symbol_name (&label);
      opt_label = symbol_find_or_make (label);
      (void) restore_line_pointer (c);
    }
  else
    opt_label = NULL;

  demand_empty_rest_of_line ();

  /* Return now in case of unknown subsection.  */
  if (dw == NULL)
    {
      as_bad (_("no known dwarf XCOFF section for flag 0x%08x\n"),
              (unsigned)flag);
      return;
    }

  /* Find the subsection.  */
  dws = &dw_sections[i];
  subseg = NULL;
  if (opt_label != NULL && S_IS_DEFINED (opt_label))
    {
      /* Sanity check (note that in theory S_GET_SEGMENT mustn't be null).  */
      if (dws->sect == NULL || S_GET_SEGMENT (opt_label) != dws->sect)
        {
          as_bad (_("label %s was not defined in this dwarf section"),
                  S_GET_NAME (opt_label));
          subseg = dws->anon_subseg;
          opt_label = NULL;
        }
      else
        subseg = symbol_get_tc (opt_label)->u.dw;
    }

  if (subseg != NULL)
    {
      /* Switch to the subsection.  */
      ppc_change_debug_section (i, subseg->subseg);
    }
  else
    {
      /* Create a new dw subsection.  */
      subseg = XNEW (struct dw_subsection);

      if (opt_label == NULL)
        {
          /* The anonymous one.  */
          subseg->subseg = 0;
          subseg->link = NULL;
          dws->anon_subseg = subseg;
        }
      else
        {
          /* A named one.  */
          if (dws->list_subseg != NULL)
            subseg->subseg = dws->list_subseg->subseg + 1;
          else
            subseg->subseg = 1;

          subseg->link = dws->list_subseg;
          dws->list_subseg = subseg;
          symbol_get_tc (opt_label)->u.dw = subseg;
        }

      ppc_change_debug_section (i, subseg->subseg);

      if (dw->def_size)
        {
          /* Add the length field.  */
          expressionS *exp = &subseg->end_exp;
          int sz;

          if (opt_label != NULL)
            symbol_set_value_now (opt_label);

          /* Add the length field.  Note that according to the AIX assembler
             manual, the size of the length field is 4 for powerpc32 but
             12 for powerpc64.  */
          if (ppc_obj64)
            {
              /* Write the 64bit marker.  */
              md_number_to_chars (frag_more (4), -1, 4);
            }

          exp->X_op = O_subtract;
          exp->X_op_symbol = symbol_temp_new_now ();
          exp->X_add_symbol = symbol_temp_make ();

          sz = ppc_obj64 ? 8 : 4;
          exp->X_add_number = -sz;
          emit_expr (exp, sz);
        }
    }
}

/* This function handles the .text and .data pseudo-ops.  These
   pseudo-ops aren't really used by XCOFF; we implement them for the
   convenience of people who aren't used to XCOFF.  */

static void
ppc_section (int type)
{
  const char *name;
  symbolS *sym;

  if (type == 't')
    name = ".text[PR]";
  else if (type == 'd')
    name = ".data[RW]";
  else
    abort ();

  sym = symbol_find_or_make (name);

  ppc_change_csect (sym, 2);

  demand_empty_rest_of_line ();
}

/* This function handles the .section pseudo-op.  This is mostly to
   give an error, since XCOFF only supports .text, .data and .bss, but
   we do permit the user to name the text or data section.  */

static void
ppc_named_section (int ignore ATTRIBUTE_UNUSED)
{
  char *user_name;
  const char *real_name;
  char c;
  symbolS *sym;

  c = get_symbol_name (&user_name);

  if (strcmp (user_name, ".text") == 0)
    real_name = ".text[PR]";
  else if (strcmp (user_name, ".data") == 0)
    real_name = ".data[RW]";
  else
    {
      as_bad (_("the XCOFF file format does not support arbitrary sections"));
      (void) restore_line_pointer (c);
      ignore_rest_of_line ();
      return;
    }

  (void) restore_line_pointer (c);

  sym = symbol_find_or_make (real_name);

  ppc_change_csect (sym, 2);

  demand_empty_rest_of_line ();
}

/* The .extern pseudo-op.  We create an undefined symbol.  */

static void
ppc_extern (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;

  endc = get_symbol_name (&name);

  (void) symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  demand_empty_rest_of_line ();
}

/* The .lglobl pseudo-op.  Keep the symbol in the symbol table.  */

static void
ppc_lglobl (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;
  symbolS *sym;

  endc = get_symbol_name (&name);

  sym = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  symbol_get_tc (sym)->output = 1;

  demand_empty_rest_of_line ();
}

/* The .ref pseudo-op.  It takes a list of symbol names and inserts R_REF
   relocations at the beginning of the current csect.

   (In principle, there's no reason why the relocations _have_ to be at
   the beginning.  Anywhere in the csect would do.  However, inserting
   at the beginning is what the native assembler does, and it helps to
   deal with cases where the .ref statements follow the section contents.)

   ??? .refs don't work for empty .csects.  However, the native assembler
   doesn't report an error in this case, and neither yet do we.  */

static void
ppc_ref (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char c;

  if (ppc_current_csect == NULL)
    {
      as_bad (_(".ref outside .csect"));
      ignore_rest_of_line ();
      return;
    }

  do
    {
      c = get_symbol_name (&name);

      fix_at_start (symbol_get_frag (ppc_current_csect), 0,
		    symbol_find_or_make (name), 0, FALSE, BFD_RELOC_NONE);

      *input_line_pointer = c;
      SKIP_WHITESPACE_AFTER_NAME ();
      c = *input_line_pointer;
      if (c == ',')
	{
	  input_line_pointer++;
	  SKIP_WHITESPACE ();
	  if (is_end_of_line[(unsigned char) *input_line_pointer])
	    {
	      as_bad (_("missing symbol name"));
	      ignore_rest_of_line ();
	      return;
	    }
	}
    }
  while (c == ',');

  demand_empty_rest_of_line ();
}

/* The .rename pseudo-op.  The RS/6000 assembler can rename symbols,
   although I don't know why it bothers.  */

static void
ppc_rename (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;
  symbolS *sym;
  int len;

  endc = get_symbol_name (&name);

  sym = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing rename string"));
      ignore_rest_of_line ();
      return;
    }
  ++input_line_pointer;

  symbol_get_tc (sym)->real_name = demand_copy_C_string (&len);

  demand_empty_rest_of_line ();
}

/* The .stabx pseudo-op.  This is similar to a normal .stabs
   pseudo-op, but slightly different.  A sample is
       .stabx "main:F-1",.main,142,0
   The first argument is the symbol name to create.  The second is the
   value, and the third is the storage class.  The fourth seems to be
   always zero, and I am assuming it is the type.  */

static void
ppc_stabx (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  int len;
  symbolS *sym;
  expressionS exp;

  name = demand_copy_C_string (&len);

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing value"));
      return;
    }
  ++input_line_pointer;

  ppc_stab_symbol = TRUE;
  sym = symbol_make (name);
  ppc_stab_symbol = FALSE;

  symbol_get_tc (sym)->real_name = name;

  (void) expression (&exp);

  switch (exp.X_op)
    {
    case O_illegal:
    case O_absent:
    case O_big:
      as_bad (_("illegal .stabx expression; zero assumed"));
      exp.X_add_number = 0;
      /* Fall through.  */
    case O_constant:
      S_SET_VALUE (sym, (valueT) exp.X_add_number);
      symbol_set_frag (sym, &zero_address_frag);
      break;

    case O_symbol:
      if (S_GET_SEGMENT (exp.X_add_symbol) == undefined_section)
	symbol_set_value_expression (sym, &exp);
      else
	{
	  S_SET_VALUE (sym,
		       exp.X_add_number + S_GET_VALUE (exp.X_add_symbol));
	  symbol_set_frag (sym, symbol_get_frag (exp.X_add_symbol));
	}
      break;

    default:
      /* The value is some complex expression.  This will probably
	 fail at some later point, but this is probably the right
	 thing to do here.  */
      symbol_set_value_expression (sym, &exp);
      break;
    }

  S_SET_SEGMENT (sym, ppc_coff_debug_section);
  symbol_get_bfdsym (sym)->flags |= BSF_DEBUGGING;

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing class"));
      return;
    }
  ++input_line_pointer;

  S_SET_STORAGE_CLASS (sym, get_absolute_expression ());

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing type"));
      return;
    }
  ++input_line_pointer;

  S_SET_DATA_TYPE (sym, get_absolute_expression ());

  symbol_get_tc (sym)->output = 1;

  if (S_GET_STORAGE_CLASS (sym) == C_STSYM)
    {
      /* In this case :

         .bs name
         .stabx	"z",arrays_,133,0
         .es

         .comm arrays_,13768,3

         resolve_symbol_value will copy the exp's "within" into sym's when the
         offset is 0.  Since this seems to be corner case problem,
         only do the correction for storage class C_STSYM.  A better solution
         would be to have the tc field updated in ppc_symbol_new_hook.  */

      if (exp.X_op == O_symbol)
        {
          if (ppc_current_block == NULL)
            as_bad (_(".stabx of storage class stsym must be within .bs/.es"));

          symbol_get_tc (sym)->within = ppc_current_block;
          symbol_get_tc (exp.X_add_symbol)->within = ppc_current_block;
        }
    }

  if (exp.X_op != O_symbol
      || ! S_IS_EXTERNAL (exp.X_add_symbol)
      || S_GET_SEGMENT (exp.X_add_symbol) != bss_section)
    ppc_frob_label (sym);
  else
    {
      symbol_remove (sym, &symbol_rootP, &symbol_lastP);
      symbol_append (sym, exp.X_add_symbol, &symbol_rootP, &symbol_lastP);
      if (symbol_get_tc (ppc_current_csect)->within == exp.X_add_symbol)
	symbol_get_tc (ppc_current_csect)->within = sym;
    }

  demand_empty_rest_of_line ();
}

/* The .function pseudo-op.  This takes several arguments.  The first
   argument seems to be the external name of the symbol.  The second
   argument seems to be the label for the start of the function.  gcc
   uses the same name for both.  I have no idea what the third and
   fourth arguments are meant to be.  The optional fifth argument is
   an expression for the size of the function.  In COFF this symbol
   gets an aux entry like that used for a csect.  */

static void
ppc_function (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;
  char *s;
  symbolS *ext_sym;
  symbolS *lab_sym;

  endc = get_symbol_name (&name);

  /* Ignore any [PR] suffix.  */
  name = ppc_canonicalize_symbol_name (name);
  s = strchr (name, '[');
  if (s != (char *) NULL
      && strcmp (s + 1, "PR]") == 0)
    *s = '\0';

  ext_sym = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing symbol name"));
      ignore_rest_of_line ();
      return;
    }
  ++input_line_pointer;

  endc = get_symbol_name (&name);

  lab_sym = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  if (ext_sym != lab_sym)
    {
      expressionS exp;

      exp.X_op = O_symbol;
      exp.X_add_symbol = lab_sym;
      exp.X_op_symbol = NULL;
      exp.X_add_number = 0;
      exp.X_unsigned = 0;
      symbol_set_value_expression (ext_sym, &exp);
    }

  if (symbol_get_tc (ext_sym)->symbol_class == -1)
    symbol_get_tc (ext_sym)->symbol_class = XMC_PR;
  symbol_get_tc (ext_sym)->output = 1;

  if (*input_line_pointer == ',')
    {
      expressionS exp;

      /* Ignore the third argument.  */
      ++input_line_pointer;
      expression (& exp);
      if (*input_line_pointer == ',')
	{
	  /* Ignore the fourth argument.  */
	  ++input_line_pointer;
	  expression (& exp);
	  if (*input_line_pointer == ',')
	    {
	      /* The fifth argument is the function size.  */
	      ++input_line_pointer;
	      symbol_get_tc (ext_sym)->u.size = symbol_new
                ("L0\001", absolute_section,(valueT) 0, &zero_address_frag);
	      pseudo_set (symbol_get_tc (ext_sym)->u.size);
	    }
	}
    }

  S_SET_DATA_TYPE (ext_sym, DT_FCN << N_BTSHFT);
  SF_SET_FUNCTION (ext_sym);
  SF_SET_PROCESS (ext_sym);
  coff_add_linesym (ext_sym);

  demand_empty_rest_of_line ();
}

/* The .bf pseudo-op.  This is just like a COFF C_FCN symbol named
   ".bf".  If the pseudo op .bi was seen before .bf, patch the .bi sym
   with the correct line number */

static symbolS *saved_bi_sym = 0;

static void
ppc_bf (int ignore ATTRIBUTE_UNUSED)
{
  symbolS *sym;

  sym = symbol_make (".bf");
  S_SET_SEGMENT (sym, text_section);
  symbol_set_frag (sym, frag_now);
  S_SET_VALUE (sym, frag_now_fix ());
  S_SET_STORAGE_CLASS (sym, C_FCN);

  coff_line_base = get_absolute_expression ();

  S_SET_NUMBER_AUXILIARY (sym, 1);
  SA_SET_SYM_LNNO (sym, coff_line_base);

  /* Line number for bi.  */
  if (saved_bi_sym)
    {
      S_SET_VALUE (saved_bi_sym, coff_n_line_nos);
      saved_bi_sym = 0;
    }


  symbol_get_tc (sym)->output = 1;

  ppc_frob_label (sym);

  demand_empty_rest_of_line ();
}

/* The .ef pseudo-op.  This is just like a COFF C_FCN symbol named
   ".ef", except that the line number is absolute, not relative to the
   most recent ".bf" symbol.  */

static void
ppc_ef (int ignore ATTRIBUTE_UNUSED)
{
  symbolS *sym;

  sym = symbol_make (".ef");
  S_SET_SEGMENT (sym, text_section);
  symbol_set_frag (sym, frag_now);
  S_SET_VALUE (sym, frag_now_fix ());
  S_SET_STORAGE_CLASS (sym, C_FCN);
  S_SET_NUMBER_AUXILIARY (sym, 1);
  SA_SET_SYM_LNNO (sym, get_absolute_expression ());
  symbol_get_tc (sym)->output = 1;

  ppc_frob_label (sym);

  demand_empty_rest_of_line ();
}

/* The .bi and .ei pseudo-ops.  These take a string argument and
   generates a C_BINCL or C_EINCL symbol, which goes at the start of
   the symbol list.  The value of .bi will be know when the next .bf
   is encountered.  */

static void
ppc_biei (int ei)
{
  static symbolS *last_biei;

  char *name;
  int len;
  symbolS *sym;
  symbolS *look;

  name = demand_copy_C_string (&len);

  /* The value of these symbols is actually file offset.  Here we set
     the value to the index into the line number entries.  In
     ppc_frob_symbols we set the fix_line field, which will cause BFD
     to do the right thing.  */

  sym = symbol_make (name);
  /* obj-coff.c currently only handles line numbers correctly in the
     .text section.  */
  S_SET_SEGMENT (sym, text_section);
  S_SET_VALUE (sym, coff_n_line_nos);
  symbol_get_bfdsym (sym)->flags |= BSF_DEBUGGING;

  S_SET_STORAGE_CLASS (sym, ei ? C_EINCL : C_BINCL);
  symbol_get_tc (sym)->output = 1;

  /* Save bi.  */
  if (ei)
    saved_bi_sym = 0;
  else
    saved_bi_sym = sym;

  for (look = last_biei ? last_biei : symbol_rootP;
       (look != (symbolS *) NULL
	&& (S_GET_STORAGE_CLASS (look) == C_FILE
	    || S_GET_STORAGE_CLASS (look) == C_BINCL
	    || S_GET_STORAGE_CLASS (look) == C_EINCL));
       look = symbol_next (look))
    ;
  if (look != (symbolS *) NULL)
    {
      symbol_remove (sym, &symbol_rootP, &symbol_lastP);
      symbol_insert (sym, look, &symbol_rootP, &symbol_lastP);
      last_biei = sym;
    }

  demand_empty_rest_of_line ();
}

/* The .bs pseudo-op.  This generates a C_BSTAT symbol named ".bs".
   There is one argument, which is a csect symbol.  The value of the
   .bs symbol is the index of this csect symbol.  */

static void
ppc_bs (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;
  symbolS *csect;
  symbolS *sym;

  if (ppc_current_block != NULL)
    as_bad (_("nested .bs blocks"));

  endc = get_symbol_name (&name);

  csect = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  sym = symbol_make (".bs");
  S_SET_SEGMENT (sym, now_seg);
  S_SET_STORAGE_CLASS (sym, C_BSTAT);
  symbol_get_bfdsym (sym)->flags |= BSF_DEBUGGING;
  symbol_get_tc (sym)->output = 1;

  symbol_get_tc (sym)->within = csect;

  ppc_frob_label (sym);

  ppc_current_block = sym;

  demand_empty_rest_of_line ();
}

/* The .es pseudo-op.  Generate a C_ESTART symbol named .es.  */

static void
ppc_es (int ignore ATTRIBUTE_UNUSED)
{
  symbolS *sym;

  if (ppc_current_block == NULL)
    as_bad (_(".es without preceding .bs"));

  sym = symbol_make (".es");
  S_SET_SEGMENT (sym, now_seg);
  S_SET_STORAGE_CLASS (sym, C_ESTAT);
  symbol_get_bfdsym (sym)->flags |= BSF_DEBUGGING;
  symbol_get_tc (sym)->output = 1;

  ppc_frob_label (sym);

  ppc_current_block = NULL;

  demand_empty_rest_of_line ();
}

/* The .bb pseudo-op.  Generate a C_BLOCK symbol named .bb, with a
   line number.  */

static void
ppc_bb (int ignore ATTRIBUTE_UNUSED)
{
  symbolS *sym;

  sym = symbol_make (".bb");
  S_SET_SEGMENT (sym, text_section);
  symbol_set_frag (sym, frag_now);
  S_SET_VALUE (sym, frag_now_fix ());
  S_SET_STORAGE_CLASS (sym, C_BLOCK);

  S_SET_NUMBER_AUXILIARY (sym, 1);
  SA_SET_SYM_LNNO (sym, get_absolute_expression ());

  symbol_get_tc (sym)->output = 1;

  SF_SET_PROCESS (sym);

  ppc_frob_label (sym);

  demand_empty_rest_of_line ();
}

/* The .eb pseudo-op.  Generate a C_BLOCK symbol named .eb, with a
   line number.  */

static void
ppc_eb (int ignore ATTRIBUTE_UNUSED)
{
  symbolS *sym;

  sym = symbol_make (".eb");
  S_SET_SEGMENT (sym, text_section);
  symbol_set_frag (sym, frag_now);
  S_SET_VALUE (sym, frag_now_fix ());
  S_SET_STORAGE_CLASS (sym, C_BLOCK);
  S_SET_NUMBER_AUXILIARY (sym, 1);
  SA_SET_SYM_LNNO (sym, get_absolute_expression ());
  symbol_get_tc (sym)->output = 1;

  SF_SET_PROCESS (sym);

  ppc_frob_label (sym);

  demand_empty_rest_of_line ();
}

/* The .bc pseudo-op.  This just creates a C_BCOMM symbol with a
   specified name.  */

static void
ppc_bc (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  int len;
  symbolS *sym;

  name = demand_copy_C_string (&len);
  sym = symbol_make (name);
  S_SET_SEGMENT (sym, ppc_coff_debug_section);
  symbol_get_bfdsym (sym)->flags |= BSF_DEBUGGING;
  S_SET_STORAGE_CLASS (sym, C_BCOMM);
  S_SET_VALUE (sym, 0);
  symbol_get_tc (sym)->output = 1;

  ppc_frob_label (sym);

  demand_empty_rest_of_line ();
}

/* The .ec pseudo-op.  This just creates a C_ECOMM symbol.  */

static void
ppc_ec (int ignore ATTRIBUTE_UNUSED)
{
  symbolS *sym;

  sym = symbol_make (".ec");
  S_SET_SEGMENT (sym, ppc_coff_debug_section);
  symbol_get_bfdsym (sym)->flags |= BSF_DEBUGGING;
  S_SET_STORAGE_CLASS (sym, C_ECOMM);
  S_SET_VALUE (sym, 0);
  symbol_get_tc (sym)->output = 1;

  ppc_frob_label (sym);

  demand_empty_rest_of_line ();
}

/* The .toc pseudo-op.  Switch to the .toc subsegment.  */

static void
ppc_toc (int ignore ATTRIBUTE_UNUSED)
{
  if (ppc_toc_csect != (symbolS *) NULL)
    subseg_set (data_section, symbol_get_tc (ppc_toc_csect)->subseg);
  else
    {
      subsegT subseg;
      symbolS *sym;
      symbolS *list;

      subseg = ppc_data_subsegment;
      ++ppc_data_subsegment;

      subseg_new (segment_name (data_section), subseg);
      ppc_toc_frag = frag_now;

      sym = symbol_find_or_make ("TOC[TC0]");
      symbol_set_frag (sym, frag_now);
      S_SET_SEGMENT (sym, data_section);
      S_SET_VALUE (sym, (valueT) frag_now_fix ());
      symbol_get_tc (sym)->subseg = subseg;
      symbol_get_tc (sym)->output = 1;
      symbol_get_tc (sym)->within = sym;

      ppc_toc_csect = sym;

      for (list = ppc_data_csects;
	   symbol_get_tc (list)->next != (symbolS *) NULL;
	   list = symbol_get_tc (list)->next)
	;
      symbol_get_tc (list)->next = sym;

      symbol_remove (sym, &symbol_rootP, &symbol_lastP);
      symbol_append (sym, symbol_get_tc (list)->within, &symbol_rootP,
		     &symbol_lastP);
    }

  ppc_current_csect = ppc_toc_csect;

  demand_empty_rest_of_line ();
}

/* The AIX assembler automatically aligns the operands of a .long or
   .short pseudo-op, and we want to be compatible.  */

static void
ppc_xcoff_cons (int log_size)
{
  frag_align (log_size, 0, 0);
  record_alignment (now_seg, log_size);
  cons (1 << log_size);
}

static void
ppc_vbyte (int dummy ATTRIBUTE_UNUSED)
{
  expressionS exp;
  int byte_count;

  (void) expression (&exp);

  if (exp.X_op != O_constant)
    {
      as_bad (_("non-constant byte count"));
      return;
    }

  byte_count = exp.X_add_number;

  if (*input_line_pointer != ',')
    {
      as_bad (_("missing value"));
      return;
    }

  ++input_line_pointer;
  cons (byte_count);
}

void
ppc_xcoff_end (void)
{
  int i;

  for (i = 0; i < XCOFF_DWSECT_NBR_NAMES; i++)
    {
      struct dw_section *dws = &dw_sections[i];
      struct dw_subsection *dwss;

      if (dws->anon_subseg)
        {
          dwss = dws->anon_subseg;
          dwss->link = dws->list_subseg;
        }
      else
        dwss = dws->list_subseg;

      for (; dwss != NULL; dwss = dwss->link)
        if (dwss->end_exp.X_add_symbol != NULL)
          {
            subseg_set (dws->sect, dwss->subseg);
            symbol_set_value_now (dwss->end_exp.X_add_symbol);
          }
    }
  ppc_cpu = 0;
}

#endif /* OBJ_XCOFF */
#if defined (OBJ_XCOFF) || defined (OBJ_ELF)

/* The .tc pseudo-op.  This is used when generating either XCOFF or
   ELF.  This takes two or more arguments.

   When generating XCOFF output, the first argument is the name to
   give to this location in the toc; this will be a symbol with class
   TC.  The rest of the arguments are N-byte values to actually put at
   this location in the TOC; often there is just one more argument, a
   relocatable symbol reference.  The size of the value to store
   depends on target word size.  A 32-bit target uses 4-byte values, a
   64-bit target uses 8-byte values.

   When not generating XCOFF output, the arguments are the same, but
   the first argument is simply ignored.  */

static void
ppc_tc (int ignore ATTRIBUTE_UNUSED)
{
#ifdef OBJ_XCOFF

  /* Define the TOC symbol name.  */
  {
    char *name;
    char endc;
    symbolS *sym;

    if (ppc_toc_csect == (symbolS *) NULL
	|| ppc_toc_csect != ppc_current_csect)
      {
	as_bad (_(".tc not in .toc section"));
	ignore_rest_of_line ();
	return;
      }

    endc = get_symbol_name (&name);

    sym = symbol_find_or_make (name);

    (void) restore_line_pointer (endc);

    if (S_IS_DEFINED (sym))
      {
	symbolS *label;

	label = symbol_get_tc (ppc_current_csect)->within;
	if (symbol_get_tc (label)->symbol_class != XMC_TC0)
	  {
	    as_bad (_(".tc with no label"));
	    ignore_rest_of_line ();
	    return;
	  }

	S_SET_SEGMENT (label, S_GET_SEGMENT (sym));
	symbol_set_frag (label, symbol_get_frag (sym));
	S_SET_VALUE (label, S_GET_VALUE (sym));

	while (! is_end_of_line[(unsigned char) *input_line_pointer])
	  ++input_line_pointer;

	return;
      }

    S_SET_SEGMENT (sym, now_seg);
    symbol_set_frag (sym, frag_now);
    S_SET_VALUE (sym, (valueT) frag_now_fix ());
    symbol_get_tc (sym)->symbol_class = XMC_TC;
    symbol_get_tc (sym)->output = 1;

    ppc_frob_label (sym);
  }

#endif /* OBJ_XCOFF */
#ifdef OBJ_ELF
  int align;

  /* Skip the TOC symbol name.  */
  while (is_part_of_name (*input_line_pointer)
	 || *input_line_pointer == ' '
	 || *input_line_pointer == '['
	 || *input_line_pointer == ']'
	 || *input_line_pointer == '{'
	 || *input_line_pointer == '}')
    ++input_line_pointer;

  /* Align to a four/eight byte boundary.  */
  align = ppc_obj64 ? 3 : 2;
  frag_align (align, 0, 0);
  record_alignment (now_seg, align);
#endif /* OBJ_ELF */

  if (*input_line_pointer != ',')
    demand_empty_rest_of_line ();
  else
    {
      ++input_line_pointer;
      cons (ppc_obj64 ? 8 : 4);
    }
}

/* Pseudo-op .machine.  */

static void
ppc_machine (int ignore ATTRIBUTE_UNUSED)
{
  char c;
  char *cpu_string;
#define MAX_HISTORY 100
  static ppc_cpu_t *cpu_history;
  static int curr_hist;

  SKIP_WHITESPACE ();

  c = get_symbol_name (&cpu_string);
  cpu_string = xstrdup (cpu_string);
  (void) restore_line_pointer (c);

  if (cpu_string != NULL)
    {
      ppc_cpu_t old_cpu = ppc_cpu;
      ppc_cpu_t new_cpu;
      char *p;

      for (p = cpu_string; *p != 0; p++)
	*p = TOLOWER (*p);

      if (strcmp (cpu_string, "push") == 0)
	{
	  if (cpu_history == NULL)
	    cpu_history = XNEWVEC (ppc_cpu_t, MAX_HISTORY);

	  if (curr_hist >= MAX_HISTORY)
	    as_bad (_(".machine stack overflow"));
	  else
	    cpu_history[curr_hist++] = ppc_cpu;
	}
      else if (strcmp (cpu_string, "pop") == 0)
	{
	  if (curr_hist <= 0)
	    as_bad (_(".machine stack underflow"));
	  else
	    ppc_cpu = cpu_history[--curr_hist];
	}
      else if ((new_cpu = ppc_parse_cpu (ppc_cpu, &sticky, cpu_string)) != 0)
	ppc_cpu = new_cpu;
      else
	as_bad (_("invalid machine `%s'"), cpu_string);

      if (ppc_cpu != old_cpu)
	ppc_setup_opcodes ();
    }

  demand_empty_rest_of_line ();
}
#endif /* defined (OBJ_XCOFF) || defined (OBJ_ELF) */

#ifdef TE_PE

/* Pseudo-ops specific to the Windows NT PowerPC PE (coff) format.  */

/* Set the current section.  */
static void
ppc_set_current_section (segT new)
{
  ppc_previous_section = ppc_current_section;
  ppc_current_section = new;
}

/* pseudo-op: .previous
   behaviour: toggles the current section with the previous section.
   errors:    None
   warnings:  "No previous section"  */

static void
ppc_previous (int ignore ATTRIBUTE_UNUSED)
{
  if (ppc_previous_section == NULL)
    {
      as_warn (_("no previous section to return to, ignored."));
      return;
    }

  subseg_set (ppc_previous_section, 0);

  ppc_set_current_section (ppc_previous_section);
}

/* pseudo-op: .pdata
   behaviour: predefined read only data section
	      double word aligned
   errors:    None
   warnings:  None
   initial:   .section .pdata "adr3"
	      a - don't know -- maybe a misprint
	      d - initialized data
	      r - readable
	      3 - double word aligned (that would be 4 byte boundary)

   commentary:
   Tag index tables (also known as the function table) for exception
   handling, debugging, etc.  */

static void
ppc_pdata (int ignore ATTRIBUTE_UNUSED)
{
  if (pdata_section == 0)
    {
      pdata_section = subseg_new (".pdata", 0);

      bfd_set_section_flags (pdata_section, (SEC_ALLOC | SEC_LOAD | SEC_RELOC
					     | SEC_READONLY | SEC_DATA));

      bfd_set_section_alignment (pdata_section, 2);
    }
  else
    {
      pdata_section = subseg_new (".pdata", 0);
    }
  ppc_set_current_section (pdata_section);
}

/* pseudo-op: .ydata
   behaviour: predefined read only data section
	      double word aligned
   errors:    None
   warnings:  None
   initial:   .section .ydata "drw3"
	      a - don't know -- maybe a misprint
	      d - initialized data
	      r - readable
	      3 - double word aligned (that would be 4 byte boundary)
   commentary:
   Tag tables (also known as the scope table) for exception handling,
   debugging, etc.  */

static void
ppc_ydata (int ignore ATTRIBUTE_UNUSED)
{
  if (ydata_section == 0)
    {
      ydata_section = subseg_new (".ydata", 0);
      bfd_set_section_flags (ydata_section, (SEC_ALLOC | SEC_LOAD | SEC_RELOC
					     | SEC_READONLY | SEC_DATA ));

      bfd_set_section_alignment (ydata_section, 3);
    }
  else
    {
      ydata_section = subseg_new (".ydata", 0);
    }
  ppc_set_current_section (ydata_section);
}

/* pseudo-op: .reldata
   behaviour: predefined read write data section
	      double word aligned (4-byte)
	      FIXME: relocation is applied to it
	      FIXME: what's the difference between this and .data?
   errors:    None
   warnings:  None
   initial:   .section .reldata "drw3"
	      d - initialized data
	      r - readable
	      w - writable
	      3 - double word aligned (that would be 8 byte boundary)

   commentary:
   Like .data, but intended to hold data subject to relocation, such as
   function descriptors, etc.  */

static void
ppc_reldata (int ignore ATTRIBUTE_UNUSED)
{
  if (reldata_section == 0)
    {
      reldata_section = subseg_new (".reldata", 0);

      bfd_set_section_flags (reldata_section, (SEC_ALLOC | SEC_LOAD | SEC_RELOC
					       | SEC_DATA));

      bfd_set_section_alignment (reldata_section, 2);
    }
  else
    {
      reldata_section = subseg_new (".reldata", 0);
    }
  ppc_set_current_section (reldata_section);
}

/* pseudo-op: .rdata
   behaviour: predefined read only data section
	      double word aligned
   errors:    None
   warnings:  None
   initial:   .section .rdata "dr3"
	      d - initialized data
	      r - readable
	      3 - double word aligned (that would be 4 byte boundary)  */

static void
ppc_rdata (int ignore ATTRIBUTE_UNUSED)
{
  if (rdata_section == 0)
    {
      rdata_section = subseg_new (".rdata", 0);
      bfd_set_section_flags (rdata_section, (SEC_ALLOC | SEC_LOAD | SEC_RELOC
					     | SEC_READONLY | SEC_DATA ));

      bfd_set_section_alignment (rdata_section, 2);
    }
  else
    {
      rdata_section = subseg_new (".rdata", 0);
    }
  ppc_set_current_section (rdata_section);
}

/* pseudo-op: .ualong
   behaviour: much like .int, with the exception that no alignment is
	      performed.
	      FIXME: test the alignment statement
   errors:    None
   warnings:  None  */

static void
ppc_ualong (int ignore ATTRIBUTE_UNUSED)
{
  /* Try for long.  */
  cons (4);
}

/* pseudo-op: .znop  <symbol name>
   behaviour: Issue a nop instruction
	      Issue a IMAGE_REL_PPC_IFGLUE relocation against it, using
	      the supplied symbol name.
   errors:    None
   warnings:  Missing symbol name  */

static void
ppc_znop (int ignore ATTRIBUTE_UNUSED)
{
  unsigned long insn;
  const struct powerpc_opcode *opcode;
  char *f;
  symbolS *sym;
  char *symbol_name;
  char c;
  char *name;

  /* Strip out the symbol name.  */
  c = get_symbol_name (&symbol_name);

  name = xstrdup (symbol_name);

  sym = symbol_find_or_make (name);

  *input_line_pointer = c;

  SKIP_WHITESPACE_AFTER_NAME ();

  /* Look up the opcode in the hash table.  */
  opcode = (const struct powerpc_opcode *) hash_find (ppc_hash, "nop");

  /* Stick in the nop.  */
  insn = opcode->opcode;

  /* Write out the instruction.  */
  f = frag_more (4);
  md_number_to_chars (f, insn, 4);
  fix_new (frag_now,
	   f - frag_now->fr_literal,
	   4,
	   sym,
	   0,
	   0,
	   BFD_RELOC_16_GOT_PCREL);

}

/* pseudo-op:
   behaviour:
   errors:
   warnings:  */

static void
ppc_pe_comm (int lcomm)
{
  char *name;
  char c;
  char *p;
  offsetT temp;
  symbolS *symbolP;
  offsetT align;

  c = get_symbol_name (&name);

  /* just after name is now '\0'.  */
  p = input_line_pointer;
  *p = c;
  SKIP_WHITESPACE_AFTER_NAME ();
  if (*input_line_pointer != ',')
    {
      as_bad (_("expected comma after symbol-name: rest of line ignored."));
      ignore_rest_of_line ();
      return;
    }

  input_line_pointer++;		/* skip ',' */
  if ((temp = get_absolute_expression ()) < 0)
    {
      as_warn (_(".COMMon length (%ld.) <0! Ignored."), (long) temp);
      ignore_rest_of_line ();
      return;
    }

  if (! lcomm)
    {
      /* The third argument to .comm is the alignment.  */
      if (*input_line_pointer != ',')
	align = 3;
      else
	{
	  ++input_line_pointer;
	  align = get_absolute_expression ();
	  if (align <= 0)
	    {
	      as_warn (_("ignoring bad alignment"));
	      align = 3;
	    }
	}
    }

  *p = 0;
  symbolP = symbol_find_or_make (name);

  *p = c;
  if (S_IS_DEFINED (symbolP) && ! S_IS_COMMON (symbolP))
    {
      as_bad (_("ignoring attempt to re-define symbol `%s'."),
	      S_GET_NAME (symbolP));
      ignore_rest_of_line ();
      return;
    }

  if (S_GET_VALUE (symbolP))
    {
      if (S_GET_VALUE (symbolP) != (valueT) temp)
	as_bad (_("length of .comm \"%s\" is already %ld. Not changed to %ld."),
		S_GET_NAME (symbolP),
		(long) S_GET_VALUE (symbolP),
		(long) temp);
    }
  else
    {
      S_SET_VALUE (symbolP, (valueT) temp);
      S_SET_EXTERNAL (symbolP);
      S_SET_SEGMENT (symbolP, bfd_com_section_ptr);
    }

  demand_empty_rest_of_line ();
}

/*
 * implement the .section pseudo op:
 *	.section name {, "flags"}
 *                ^         ^
 *                |         +--- optional flags: 'b' for bss
 *                |                              'i' for info
 *                +-- section name               'l' for lib
 *                                               'n' for noload
 *                                               'o' for over
 *                                               'w' for data
 *						 'd' (apparently m88k for data)
 *                                               'x' for text
 * But if the argument is not a quoted string, treat it as a
 * subsegment number.
 *
 * FIXME: this is a copy of the section processing from obj-coff.c, with
 * additions/changes for the moto-pas assembler support. There are three
 * categories:
 *
 * FIXME: I just noticed this. This doesn't work at all really. It it
 *        setting bits that bfd probably neither understands or uses. The
 *        correct approach (?) will have to incorporate extra fields attached
 *        to the section to hold the system specific stuff. (krk)
 *
 * Section Contents:
 * 'a' - unknown - referred to in documentation, but no definition supplied
 * 'c' - section has code
 * 'd' - section has initialized data
 * 'u' - section has uninitialized data
 * 'i' - section contains directives (info)
 * 'n' - section can be discarded
 * 'R' - remove section at link time
 *
 * Section Protection:
 * 'r' - section is readable
 * 'w' - section is writable
 * 'x' - section is executable
 * 's' - section is sharable
 *
 * Section Alignment:
 * '0' - align to byte boundary
 * '1' - align to halfword boundary
 * '2' - align to word boundary
 * '3' - align to doubleword boundary
 * '4' - align to quadword boundary
 * '5' - align to 32 byte boundary
 * '6' - align to 64 byte boundary
 *
 */

void
ppc_pe_section (int ignore ATTRIBUTE_UNUSED)
{
  /* Strip out the section name.  */
  char *section_name;
  char c;
  char *name;
  unsigned int exp;
  flagword flags;
  segT sec;
  int align;

  c = get_symbol_name (&section_name);

  name = xstrdup (section_name);

  *input_line_pointer = c;

  SKIP_WHITESPACE_AFTER_NAME ();

  exp = 0;
  flags = SEC_NO_FLAGS;

  if (strcmp (name, ".idata$2") == 0)
    {
      align = 0;
    }
  else if (strcmp (name, ".idata$3") == 0)
    {
      align = 0;
    }
  else if (strcmp (name, ".idata$4") == 0)
    {
      align = 2;
    }
  else if (strcmp (name, ".idata$5") == 0)
    {
      align = 2;
    }
  else if (strcmp (name, ".idata$6") == 0)
    {
      align = 1;
    }
  else
    /* Default alignment to 16 byte boundary.  */
    align = 4;

  if (*input_line_pointer == ',')
    {
      ++input_line_pointer;
      SKIP_WHITESPACE ();
      if (*input_line_pointer != '"')
	exp = get_absolute_expression ();
      else
	{
	  ++input_line_pointer;
	  while (*input_line_pointer != '"'
		 && ! is_end_of_line[(unsigned char) *input_line_pointer])
	    {
	      switch (*input_line_pointer)
		{
		  /* Section Contents */
		case 'a': /* unknown */
		  as_bad (_("unsupported section attribute -- 'a'"));
		  break;
		case 'c': /* code section */
		  flags |= SEC_CODE;
		  break;
		case 'd': /* section has initialized data */
		  flags |= SEC_DATA;
		  break;
		case 'u': /* section has uninitialized data */
		  /* FIXME: This is IMAGE_SCN_CNT_UNINITIALIZED_DATA
		     in winnt.h */
		  flags |= SEC_ROM;
		  break;
		case 'i': /* section contains directives (info) */
		  /* FIXME: This is IMAGE_SCN_LNK_INFO
		     in winnt.h */
		  flags |= SEC_HAS_CONTENTS;
		  break;
		case 'n': /* section can be discarded */
		  flags &=~ SEC_LOAD;
		  break;
		case 'R': /* Remove section at link time */
		  flags |= SEC_NEVER_LOAD;
		  break;
#if IFLICT_BRAIN_DAMAGE
		  /* Section Protection */
		case 'r': /* section is readable */
		  flags |= IMAGE_SCN_MEM_READ;
		  break;
		case 'w': /* section is writable */
		  flags |= IMAGE_SCN_MEM_WRITE;
		  break;
		case 'x': /* section is executable */
		  flags |= IMAGE_SCN_MEM_EXECUTE;
		  break;
		case 's': /* section is sharable */
		  flags |= IMAGE_SCN_MEM_SHARED;
		  break;

		  /* Section Alignment */
		case '0': /* align to byte boundary */
		  flags |= IMAGE_SCN_ALIGN_1BYTES;
		  align = 0;
		  break;
		case '1':  /* align to halfword boundary */
		  flags |= IMAGE_SCN_ALIGN_2BYTES;
		  align = 1;
		  break;
		case '2':  /* align to word boundary */
		  flags |= IMAGE_SCN_ALIGN_4BYTES;
		  align = 2;
		  break;
		case '3':  /* align to doubleword boundary */
		  flags |= IMAGE_SCN_ALIGN_8BYTES;
		  align = 3;
		  break;
		case '4':  /* align to quadword boundary */
		  flags |= IMAGE_SCN_ALIGN_16BYTES;
		  align = 4;
		  break;
		case '5':  /* align to 32 byte boundary */
		  flags |= IMAGE_SCN_ALIGN_32BYTES;
		  align = 5;
		  break;
		case '6':  /* align to 64 byte boundary */
		  flags |= IMAGE_SCN_ALIGN_64BYTES;
		  align = 6;
		  break;
#endif
		default:
		  as_bad (_("unknown section attribute '%c'"),
			  *input_line_pointer);
		  break;
		}
	      ++input_line_pointer;
	    }
	  if (*input_line_pointer == '"')
	    ++input_line_pointer;
	}
    }

  sec = subseg_new (name, (subsegT) exp);

  ppc_set_current_section (sec);

  if (flags != SEC_NO_FLAGS)
    {
      if (!bfd_set_section_flags (sec, flags))
	as_bad (_("error setting flags for \"%s\": %s"),
		bfd_section_name (sec),
		bfd_errmsg (bfd_get_error ()));
    }

  bfd_set_section_alignment (sec, align);
}

static void
ppc_pe_function (int ignore ATTRIBUTE_UNUSED)
{
  char *name;
  char endc;
  symbolS *ext_sym;

  endc = get_symbol_name (&name);

  ext_sym = symbol_find_or_make (name);

  (void) restore_line_pointer (endc);

  S_SET_DATA_TYPE (ext_sym, DT_FCN << N_BTSHFT);
  SF_SET_FUNCTION (ext_sym);
  SF_SET_PROCESS (ext_sym);
  coff_add_linesym (ext_sym);

  demand_empty_rest_of_line ();
}

static void
ppc_pe_tocd (int ignore ATTRIBUTE_UNUSED)
{
  if (tocdata_section == 0)
    {
      tocdata_section = subseg_new (".tocd", 0);
      /* FIXME: section flags won't work.  */
      bfd_set_section_flags (tocdata_section, (SEC_ALLOC | SEC_LOAD | SEC_RELOC
					       | SEC_READONLY | SEC_DATA));

      bfd_set_section_alignment (tocdata_section, 2);
    }
  else
    {
      rdata_section = subseg_new (".tocd", 0);
    }

  ppc_set_current_section (tocdata_section);

  demand_empty_rest_of_line ();
}

/* Don't adjust TOC relocs to use the section symbol.  */

int
ppc_pe_fix_adjustable (fixS *fix)
{
  return fix->fx_r_type != BFD_RELOC_PPC_TOC16;
}

#endif

#ifdef OBJ_XCOFF

/* XCOFF specific symbol and file handling.  */

/* Canonicalize the symbol name.  We use the to force the suffix, if
   any, to use square brackets, and to be in upper case.  */

char *
ppc_canonicalize_symbol_name (char *name)
{
  char *s;

  if (ppc_stab_symbol)
    return name;

  for (s = name; *s != '\0' && *s != '{' && *s != '['; s++)
    ;
  if (*s != '\0')
    {
      char brac;

      if (*s == '[')
	brac = ']';
      else
	{
	  *s = '[';
	  brac = '}';
	}

      for (s++; *s != '\0' && *s != brac; s++)
	*s = TOUPPER (*s);

      if (*s == '\0' || s[1] != '\0')
	as_bad (_("bad symbol suffix"));

      *s = ']';
    }

  return name;
}

/* Set the class of a symbol based on the suffix, if any.  This is
   called whenever a new symbol is created.  */

void
ppc_symbol_new_hook (symbolS *sym)
{
  struct ppc_tc_sy *tc;
  const char *s;

  tc = symbol_get_tc (sym);
  tc->next = NULL;
  tc->output = 0;
  tc->symbol_class = -1;
  tc->real_name = NULL;
  tc->subseg = 0;
  tc->align = 0;
  tc->u.size = NULL;
  tc->u.dw = NULL;
  tc->within = NULL;

  if (ppc_stab_symbol)
    return;

  s = strchr (S_GET_NAME (sym), '[');
  if (s == (const char *) NULL)
    {
      /* There is no suffix.  */
      return;
    }

  ++s;

  switch (s[0])
    {
    case 'B':
      if (strcmp (s, "BS]") == 0)
	tc->symbol_class = XMC_BS;
      break;
    case 'D':
      if (strcmp (s, "DB]") == 0)
	tc->symbol_class = XMC_DB;
      else if (strcmp (s, "DS]") == 0)
	tc->symbol_class = XMC_DS;
      break;
    case 'G':
      if (strcmp (s, "GL]") == 0)
	tc->symbol_class = XMC_GL;
      break;
    case 'P':
      if (strcmp (s, "PR]") == 0)
	tc->symbol_class = XMC_PR;
      break;
    case 'R':
      if (strcmp (s, "RO]") == 0)
	tc->symbol_class = XMC_RO;
      else if (strcmp (s, "RW]") == 0)
	tc->symbol_class = XMC_RW;
      break;
    case 'S':
      if (strcmp (s, "SV]") == 0)
	tc->symbol_class = XMC_SV;
      break;
    case 'T':
      if (strcmp (s, "TC]") == 0)
	tc->symbol_class = XMC_TC;
      else if (strcmp (s, "TI]") == 0)
	tc->symbol_class = XMC_TI;
      else if (strcmp (s, "TB]") == 0)
	tc->symbol_class = XMC_TB;
      else if (strcmp (s, "TC0]") == 0 || strcmp (s, "T0]") == 0)
	tc->symbol_class = XMC_TC0;
      break;
    case 'U':
      if (strcmp (s, "UA]") == 0)
	tc->symbol_class = XMC_UA;
      else if (strcmp (s, "UC]") == 0)
	tc->symbol_class = XMC_UC;
      break;
    case 'X':
      if (strcmp (s, "XO]") == 0)
	tc->symbol_class = XMC_XO;
      break;
    }

  if (tc->symbol_class == -1)
    as_bad (_("unrecognized symbol suffix"));
}

/* This variable is set by ppc_frob_symbol if any absolute symbols are
   seen.  It tells ppc_adjust_symtab whether it needs to look through
   the symbols.  */

static bfd_boolean ppc_saw_abs;

/* Change the name of a symbol just before writing it out.  Set the
   real name if the .rename pseudo-op was used.  Otherwise, remove any
   class suffix.  Return 1 if the symbol should not be included in the
   symbol table.  */

int
ppc_frob_symbol (symbolS *sym)
{
  static symbolS *ppc_last_function;
  static symbolS *set_end;

  /* Discard symbols that should not be included in the output symbol
     table.  */
  if (! symbol_used_in_reloc_p (sym)
      && ((symbol_get_bfdsym (sym)->flags & BSF_SECTION_SYM) != 0
	  || (! (S_IS_EXTERNAL (sym) || S_IS_WEAK (sym))
	      && ! symbol_get_tc (sym)->output
	      && S_GET_STORAGE_CLASS (sym) != C_FILE)))
    return 1;

  /* This one will disappear anyway.  Don't make a csect sym for it.  */
  if (sym == abs_section_sym)
    return 1;

  if (symbol_get_tc (sym)->real_name != (char *) NULL)
    S_SET_NAME (sym, symbol_get_tc (sym)->real_name);
  else
    {
      const char *name;
      const char *s;

      name = S_GET_NAME (sym);
      s = strchr (name, '[');
      if (s != (char *) NULL)
	{
	  unsigned int len;
	  char *snew;

	  len = s - name;
	  snew = xstrndup (name, len);

	  S_SET_NAME (sym, snew);
	}
    }

  if (set_end != (symbolS *) NULL)
    {
      SA_SET_SYM_ENDNDX (set_end, sym);
      set_end = NULL;
    }

  if (SF_GET_FUNCTION (sym))
    {
      if (ppc_last_function != (symbolS *) NULL)
	as_bad (_("two .function pseudo-ops with no intervening .ef"));
      ppc_last_function = sym;
      if (symbol_get_tc (sym)->u.size != (symbolS *) NULL)
	{
	  resolve_symbol_value (symbol_get_tc (sym)->u.size);
	  SA_SET_SYM_FSIZE (sym,
			    (long) S_GET_VALUE (symbol_get_tc (sym)->u.size));
	}
    }
  else if (S_GET_STORAGE_CLASS (sym) == C_FCN
	   && strcmp (S_GET_NAME (sym), ".ef") == 0)
    {
      if (ppc_last_function == (symbolS *) NULL)
	as_bad (_(".ef with no preceding .function"));
      else
	{
	  set_end = ppc_last_function;
	  ppc_last_function = NULL;

	  /* We don't have a C_EFCN symbol, but we need to force the
	     COFF backend to believe that it has seen one.  */
	  coff_last_function = NULL;
	}
    }

  if (! (S_IS_EXTERNAL (sym) || S_IS_WEAK (sym))
      && (symbol_get_bfdsym (sym)->flags & BSF_SECTION_SYM) == 0
      && S_GET_STORAGE_CLASS (sym) != C_FILE
      && S_GET_STORAGE_CLASS (sym) != C_FCN
      && S_GET_STORAGE_CLASS (sym) != C_BLOCK
      && S_GET_STORAGE_CLASS (sym) != C_BSTAT
      && S_GET_STORAGE_CLASS (sym) != C_ESTAT
      && S_GET_STORAGE_CLASS (sym) != C_BINCL
      && S_GET_STORAGE_CLASS (sym) != C_EINCL
      && S_GET_SEGMENT (sym) != ppc_coff_debug_section)
    S_SET_STORAGE_CLASS (sym, C_HIDEXT);

  if (S_GET_STORAGE_CLASS (sym) == C_EXT
      || S_GET_STORAGE_CLASS (sym) == C_AIX_WEAKEXT
      || S_GET_STORAGE_CLASS (sym) == C_HIDEXT)
    {
      int i;
      union internal_auxent *a;

      /* Create a csect aux.  */
      i = S_GET_NUMBER_AUXILIARY (sym);
      S_SET_NUMBER_AUXILIARY (sym, i + 1);
      a = &coffsymbol (symbol_get_bfdsym (sym))->native[i + 1].u.auxent;
      if (symbol_get_tc (sym)->symbol_class == XMC_TC0)
	{
	  /* This is the TOC table.  */
	  know (strcmp (S_GET_NAME (sym), "TOC") == 0);
	  a->x_csect.x_scnlen.l = 0;
	  a->x_csect.x_smtyp = (2 << 3) | XTY_SD;
	}
      else if (symbol_get_tc (sym)->subseg != 0)
	{
	  /* This is a csect symbol.  x_scnlen is the size of the
	     csect.  */
	  if (symbol_get_tc (sym)->next == (symbolS *) NULL)
	    a->x_csect.x_scnlen.l = (bfd_section_size (S_GET_SEGMENT (sym))
				     - S_GET_VALUE (sym));
	  else
	    {
	      resolve_symbol_value (symbol_get_tc (sym)->next);
	      a->x_csect.x_scnlen.l = (S_GET_VALUE (symbol_get_tc (sym)->next)
				       - S_GET_VALUE (sym));
	    }
	  a->x_csect.x_smtyp = (symbol_get_tc (sym)->align << 3) | XTY_SD;
	}
      else if (S_GET_SEGMENT (sym) == bss_section)
	{
	  /* This is a common symbol.  */
	  a->x_csect.x_scnlen.l = symbol_get_frag (sym)->fr_offset;
	  a->x_csect.x_smtyp = (symbol_get_tc (sym)->align << 3) | XTY_CM;
	  if (S_IS_EXTERNAL (sym))
	    symbol_get_tc (sym)->symbol_class = XMC_RW;
	  else
	    symbol_get_tc (sym)->symbol_class = XMC_BS;
	}
      else if (S_GET_SEGMENT (sym) == absolute_section)
	{
	  /* This is an absolute symbol.  The csect will be created by
	     ppc_adjust_symtab.  */
	  ppc_saw_abs = TRUE;
	  a->x_csect.x_smtyp = XTY_LD;
	  if (symbol_get_tc (sym)->symbol_class == -1)
	    symbol_get_tc (sym)->symbol_class = XMC_XO;
	}
      else if (! S_IS_DEFINED (sym))
	{
	  /* This is an external symbol.  */
	  a->x_csect.x_scnlen.l = 0;
	  a->x_csect.x_smtyp = XTY_ER;
	}
      else if (symbol_get_tc (sym)->symbol_class == XMC_TC)
	{
	  symbolS *next;

	  /* This is a TOC definition.  x_scnlen is the size of the
	     TOC entry.  */
	  next = symbol_next (sym);
	  while (symbol_get_tc (next)->symbol_class == XMC_TC0)
	    next = symbol_next (next);
	  if (next == (symbolS *) NULL
	      || symbol_get_tc (next)->symbol_class != XMC_TC)
	    {
	      if (ppc_after_toc_frag == (fragS *) NULL)
		a->x_csect.x_scnlen.l = (bfd_section_size (data_section)
					 - S_GET_VALUE (sym));
	      else
		a->x_csect.x_scnlen.l = (ppc_after_toc_frag->fr_address
					 - S_GET_VALUE (sym));
	    }
	  else
	    {
	      resolve_symbol_value (next);
	      a->x_csect.x_scnlen.l = (S_GET_VALUE (next)
				       - S_GET_VALUE (sym));
	    }
	  a->x_csect.x_smtyp = (2 << 3) | XTY_SD;
	}
      else
	{
	  symbolS *csect;

	  /* This is a normal symbol definition.  x_scnlen is the
	     symbol index of the containing csect.  */
	  if (S_GET_SEGMENT (sym) == text_section)
	    csect = ppc_text_csects;
	  else if (S_GET_SEGMENT (sym) == data_section)
	    csect = ppc_data_csects;
	  else
	    abort ();

	  /* Skip the initial dummy symbol.  */
	  csect = symbol_get_tc (csect)->next;

	  if (csect == (symbolS *) NULL)
	    {
	      as_warn (_("warning: symbol %s has no csect"), S_GET_NAME (sym));
	      a->x_csect.x_scnlen.l = 0;
	    }
	  else
	    {
	      while (symbol_get_tc (csect)->next != (symbolS *) NULL)
		{
		  resolve_symbol_value (symbol_get_tc (csect)->next);
		  if (S_GET_VALUE (symbol_get_tc (csect)->next)
		      > S_GET_VALUE (sym))
		    break;
		  csect = symbol_get_tc (csect)->next;
		}

	      a->x_csect.x_scnlen.p =
		coffsymbol (symbol_get_bfdsym (csect))->native;
	      coffsymbol (symbol_get_bfdsym (sym))->native[i + 1].fix_scnlen =
		1;
	    }
	  a->x_csect.x_smtyp = XTY_LD;
	}

      a->x_csect.x_parmhash = 0;
      a->x_csect.x_snhash = 0;
      if (symbol_get_tc (sym)->symbol_class == -1)
	a->x_csect.x_smclas = XMC_PR;
      else
	a->x_csect.x_smclas = symbol_get_tc (sym)->symbol_class;
      a->x_csect.x_stab = 0;
      a->x_csect.x_snstab = 0;

      /* Don't let the COFF backend resort these symbols.  */
      symbol_get_bfdsym (sym)->flags |= BSF_NOT_AT_END;
    }
  else if (S_GET_STORAGE_CLASS (sym) == C_BSTAT)
    {
      /* We want the value to be the symbol index of the referenced
	 csect symbol.  BFD will do that for us if we set the right
	 flags.  */
      asymbol *bsym = symbol_get_bfdsym (symbol_get_tc (sym)->within);
      combined_entry_type *c = coffsymbol (bsym)->native;

      S_SET_VALUE (sym, (valueT) (size_t) c);
      coffsymbol (symbol_get_bfdsym (sym))->native->fix_value = 1;
    }
  else if (S_GET_STORAGE_CLASS (sym) == C_STSYM)
    {
      symbolS *block;
      valueT base;

      block = symbol_get_tc (sym)->within;
      if (block)
        {
          /* The value is the offset from the enclosing csect.  */
          symbolS *csect;

          csect = symbol_get_tc (block)->within;
          resolve_symbol_value (csect);
          base = S_GET_VALUE (csect);
        }
      else
        base = 0;

      S_SET_VALUE (sym, S_GET_VALUE (sym) - base);
    }
  else if (S_GET_STORAGE_CLASS (sym) == C_BINCL
	   || S_GET_STORAGE_CLASS (sym) == C_EINCL)
    {
      /* We want the value to be a file offset into the line numbers.
	 BFD will do that for us if we set the right flags.  We have
	 already set the value correctly.  */
      coffsymbol (symbol_get_bfdsym (sym))->native->fix_line = 1;
    }

  return 0;
}

/* Adjust the symbol table.  This creates csect symbols for all
   absolute symbols.  */

void
ppc_adjust_symtab (void)
{
  symbolS *sym;

  if (! ppc_saw_abs)
    return;

  for (sym = symbol_rootP; sym != NULL; sym = symbol_next (sym))
    {
      symbolS *csect;
      int i;
      union internal_auxent *a;

      if (S_GET_SEGMENT (sym) != absolute_section)
	continue;

      csect = symbol_create (".abs[XO]", absolute_section,
			     S_GET_VALUE (sym), &zero_address_frag);
      symbol_get_bfdsym (csect)->value = S_GET_VALUE (sym);
      S_SET_STORAGE_CLASS (csect, C_HIDEXT);
      i = S_GET_NUMBER_AUXILIARY (csect);
      S_SET_NUMBER_AUXILIARY (csect, i + 1);
      a = &coffsymbol (symbol_get_bfdsym (csect))->native[i + 1].u.auxent;
      a->x_csect.x_scnlen.l = 0;
      a->x_csect.x_smtyp = XTY_SD;
      a->x_csect.x_parmhash = 0;
      a->x_csect.x_snhash = 0;
      a->x_csect.x_smclas = XMC_XO;
      a->x_csect.x_stab = 0;
      a->x_csect.x_snstab = 0;

      symbol_insert (csect, sym, &symbol_rootP, &symbol_lastP);

      i = S_GET_NUMBER_AUXILIARY (sym);
      a = &coffsymbol (symbol_get_bfdsym (sym))->native[i].u.auxent;
      a->x_csect.x_scnlen.p = coffsymbol (symbol_get_bfdsym (csect))->native;
      coffsymbol (symbol_get_bfdsym (sym))->native[i].fix_scnlen = 1;
    }

  ppc_saw_abs = FALSE;
}

/* Set the VMA for a section.  This is called on all the sections in
   turn.  */

void
ppc_frob_section (asection *sec)
{
  static bfd_vma vma = 0;

  /* Dwarf sections start at 0.  */
  if (bfd_section_flags (sec) & SEC_DEBUGGING)
    return;

  vma = md_section_align (sec, vma);
  bfd_set_section_vma (sec, vma);
  vma += bfd_section_size (sec);
}

#endif /* OBJ_XCOFF */

const char *
md_atof (int type, char *litp, int *sizep)
{
  return ieee_md_atof (type, litp, sizep, target_big_endian);
}

/* Write a value out to the object file, using the appropriate
   endianness.  */

void
md_number_to_chars (char *buf, valueT val, int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

/* Align a section (I don't know why this is machine dependent).  */

valueT
md_section_align (asection *seg ATTRIBUTE_UNUSED, valueT addr)
{
#ifdef OBJ_ELF
  return addr;
#else
  int align = bfd_section_alignment (seg);

  return ((addr + (1 << align) - 1) & -(1 << align));
#endif
}

/* We don't have any form of relaxing.  */

int
md_estimate_size_before_relax (fragS *fragp ATTRIBUTE_UNUSED,
			       asection *seg ATTRIBUTE_UNUSED)
{
  abort ();
  return 0;
}

/* Convert a machine dependent frag.  We never generate these.  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 asection *sec ATTRIBUTE_UNUSED,
		 fragS *fragp ATTRIBUTE_UNUSED)
{
  abort ();
}

/* We have no need to default values of symbols.  */

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return 0;
}

/* Functions concerning relocs.  */

/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */

long
md_pcrel_from_section (fixS *fixp, segT sec ATTRIBUTE_UNUSED)
{
  return fixp->fx_frag->fr_address + fixp->fx_where;
}

#ifdef OBJ_XCOFF

/* This is called to see whether a fixup should be adjusted to use a
   section symbol.  We take the opportunity to change a fixup against
   a symbol in the TOC subsegment into a reloc against the
   corresponding .tc symbol.  */

int
ppc_fix_adjustable (fixS *fix)
{
  valueT val = resolve_symbol_value (fix->fx_addsy);
  segT symseg = S_GET_SEGMENT (fix->fx_addsy);
  TC_SYMFIELD_TYPE *tc;

  if (symseg == absolute_section)
    return 0;

  /* Always adjust symbols in debugging sections.  */
  if (bfd_section_flags (symseg) & SEC_DEBUGGING)
    return 1;

  if (ppc_toc_csect != (symbolS *) NULL
      && fix->fx_addsy != ppc_toc_csect
      && symseg == data_section
      && val >= ppc_toc_frag->fr_address
      && (ppc_after_toc_frag == (fragS *) NULL
	  || val < ppc_after_toc_frag->fr_address))
    {
      symbolS *sy;

      for (sy = symbol_next (ppc_toc_csect);
	   sy != (symbolS *) NULL;
	   sy = symbol_next (sy))
	{
	  TC_SYMFIELD_TYPE *sy_tc = symbol_get_tc (sy);

	  if (sy_tc->symbol_class == XMC_TC0)
	    continue;
	  if (sy_tc->symbol_class != XMC_TC)
	    break;
	  if (val == resolve_symbol_value (sy))
	    {
	      fix->fx_addsy = sy;
	      fix->fx_addnumber = val - ppc_toc_frag->fr_address;
	      return 0;
	    }
	}

      as_bad_where (fix->fx_file, fix->fx_line,
		    _("symbol in .toc does not match any .tc"));
    }

  /* Possibly adjust the reloc to be against the csect.  */
  tc = symbol_get_tc (fix->fx_addsy);
  if (tc->subseg == 0
      && tc->symbol_class != XMC_TC0
      && tc->symbol_class != XMC_TC
      && symseg != bss_section
      /* Don't adjust if this is a reloc in the toc section.  */
      && (symseg != data_section
	  || ppc_toc_csect == NULL
	  || val < ppc_toc_frag->fr_address
	  || (ppc_after_toc_frag != NULL
	      && val >= ppc_after_toc_frag->fr_address)))
    {
      symbolS *csect = tc->within;

      /* If the symbol was not declared by a label (eg: a section symbol),
         use the section instead of the csect.  This doesn't happen in
         normal AIX assembly code.  */
      if (csect == NULL)
        csect = seg_info (symseg)->sym;

      fix->fx_offset += val - symbol_get_frag (csect)->fr_address;
      fix->fx_addsy = csect;

      return 0;
    }

  /* Adjust a reloc against a .lcomm symbol to be against the base
     .lcomm.  */
  if (symseg == bss_section
      && ! S_IS_EXTERNAL (fix->fx_addsy))
    {
      symbolS *sy = symbol_get_frag (fix->fx_addsy)->fr_symbol;

      fix->fx_offset += val - resolve_symbol_value (sy);
      fix->fx_addsy = sy;
    }

  return 0;
}

/* A reloc from one csect to another must be kept.  The assembler
   will, of course, keep relocs between sections, and it will keep
   absolute relocs, but we need to force it to keep PC relative relocs
   between two csects in the same section.  */

int
ppc_force_relocation (fixS *fix)
{
  /* At this point fix->fx_addsy should already have been converted to
     a csect symbol.  If the csect does not include the fragment, then
     we need to force the relocation.  */
  if (fix->fx_pcrel
      && fix->fx_addsy != NULL
      && symbol_get_tc (fix->fx_addsy)->subseg != 0
      && ((symbol_get_frag (fix->fx_addsy)->fr_address
	   > fix->fx_frag->fr_address)
	  || (symbol_get_tc (fix->fx_addsy)->next != NULL
	      && (symbol_get_frag (symbol_get_tc (fix->fx_addsy)->next)->fr_address
		  <= fix->fx_frag->fr_address))))
    return 1;

  return generic_force_reloc (fix);
}
#endif /* OBJ_XCOFF */

#ifdef OBJ_ELF
/* If this function returns non-zero, it guarantees that a relocation
   will be emitted for a fixup.  */

int
ppc_force_relocation (fixS *fix)
{
  /* Branch prediction relocations must force a relocation, as must
     the vtable description relocs.  */
  switch (fix->fx_r_type)
    {
    case BFD_RELOC_PPC_B16_BRTAKEN:
    case BFD_RELOC_PPC_B16_BRNTAKEN:
    case BFD_RELOC_PPC_BA16_BRTAKEN:
    case BFD_RELOC_PPC_BA16_BRNTAKEN:
    case BFD_RELOC_24_PLT_PCREL:
    case BFD_RELOC_PPC64_TOC:
      return 1;
    case BFD_RELOC_PPC_B26:
    case BFD_RELOC_PPC_BA26:
    case BFD_RELOC_PPC_B16:
    case BFD_RELOC_PPC_BA16:
    case BFD_RELOC_PPC64_REL24_NOTOC:
      /* All branch fixups targeting a localentry symbol must
         force a relocation.  */
      if (fix->fx_addsy)
	{
	  asymbol *bfdsym = symbol_get_bfdsym (fix->fx_addsy);
	  elf_symbol_type *elfsym
	    = elf_symbol_from (bfd_asymbol_bfd (bfdsym), bfdsym);
	  gas_assert (elfsym);
	  if ((STO_PPC64_LOCAL_MASK & elfsym->internal_elf_sym.st_other) != 0)
	    return 1;
	}
      break;
    default:
      break;
    }

  if (fix->fx_r_type >= BFD_RELOC_PPC_TLS
      && fix->fx_r_type <= BFD_RELOC_PPC64_TLS_PCREL)
    return 1;

  return generic_force_reloc (fix);
}

int
ppc_fix_adjustable (fixS *fix)
{
  switch (fix->fx_r_type)
    {
      /* All branch fixups targeting a localentry symbol must
         continue using the symbol.  */
    case BFD_RELOC_PPC_B26:
    case BFD_RELOC_PPC_BA26:
    case BFD_RELOC_PPC_B16:
    case BFD_RELOC_PPC_BA16:
    case BFD_RELOC_PPC_B16_BRTAKEN:
    case BFD_RELOC_PPC_B16_BRNTAKEN:
    case BFD_RELOC_PPC_BA16_BRTAKEN:
    case BFD_RELOC_PPC_BA16_BRNTAKEN:
    case BFD_RELOC_PPC64_REL24_NOTOC:
      if (fix->fx_addsy)
	{
	  asymbol *bfdsym = symbol_get_bfdsym (fix->fx_addsy);
	  elf_symbol_type *elfsym
	    = elf_symbol_from (bfd_asymbol_bfd (bfdsym), bfdsym);
	  gas_assert (elfsym);
	  if ((STO_PPC64_LOCAL_MASK & elfsym->internal_elf_sym.st_other) != 0)
	    return 0;
	}
      break;
    default:
      break;
    }

  return (fix->fx_r_type != BFD_RELOC_16_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_LO16_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_HI16_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_HI16_S_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_PPC64_GOT16_DS
	  && fix->fx_r_type != BFD_RELOC_PPC64_GOT16_LO_DS
	  && fix->fx_r_type != BFD_RELOC_16_GOT_PCREL
	  && fix->fx_r_type != BFD_RELOC_32_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_PPC64_GOT_PCREL34
	  && fix->fx_r_type != BFD_RELOC_24_PLT_PCREL
	  && fix->fx_r_type != BFD_RELOC_32_PLTOFF
	  && fix->fx_r_type != BFD_RELOC_32_PLT_PCREL
	  && fix->fx_r_type != BFD_RELOC_LO16_PLTOFF
	  && fix->fx_r_type != BFD_RELOC_HI16_PLTOFF
	  && fix->fx_r_type != BFD_RELOC_HI16_S_PLTOFF
	  && fix->fx_r_type != BFD_RELOC_64_PLTOFF
	  && fix->fx_r_type != BFD_RELOC_64_PLT_PCREL
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLT16_LO_DS
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLT_PCREL34
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLTGOT16
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLTGOT16_LO
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLTGOT16_HI
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLTGOT16_HA
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLTGOT16_DS
	  && fix->fx_r_type != BFD_RELOC_PPC64_PLTGOT16_LO_DS
	  && fix->fx_r_type != BFD_RELOC_GPREL16
	  && fix->fx_r_type != BFD_RELOC_PPC_VLE_SDAREL_LO16A
	  && fix->fx_r_type != BFD_RELOC_PPC_VLE_SDAREL_HI16A
	  && fix->fx_r_type != BFD_RELOC_PPC_VLE_SDAREL_HA16A
	  && fix->fx_r_type != BFD_RELOC_VTABLE_INHERIT
	  && fix->fx_r_type != BFD_RELOC_VTABLE_ENTRY
	  && !(fix->fx_r_type >= BFD_RELOC_PPC_TLS
	       && fix->fx_r_type <= BFD_RELOC_PPC64_TLS_PCREL));
}
#endif

void
ppc_frag_check (struct frag *fragP)
{
  if ((fragP->fr_address & fragP->insn_addr) != 0)
    as_bad_where (fragP->fr_file, fragP->fr_line,
		  _("instruction address is not a multiple of %d"),
		  fragP->insn_addr + 1);
}

/* rs_align_code frag handling.  */

enum ppc_nop_encoding_for_rs_align_code
{
  PPC_NOP_VANILLA,
  PPC_NOP_VLE,
  PPC_NOP_GROUP_P6,
  PPC_NOP_GROUP_P7
};

unsigned int
ppc_nop_select (void)
{
  if ((ppc_cpu & PPC_OPCODE_VLE) != 0)
    return PPC_NOP_VLE;
  if ((ppc_cpu & (PPC_OPCODE_POWER9 | PPC_OPCODE_E500MC)) == 0)
    {
      if ((ppc_cpu & PPC_OPCODE_POWER7) != 0)
	return PPC_NOP_GROUP_P7;
      if ((ppc_cpu & PPC_OPCODE_POWER6) != 0)
	return PPC_NOP_GROUP_P6;
    }
  return PPC_NOP_VANILLA;
}

void
ppc_handle_align (struct frag *fragP)
{
  valueT count = (fragP->fr_next->fr_address
		  - (fragP->fr_address + fragP->fr_fix));
  char *dest = fragP->fr_literal + fragP->fr_fix;
  enum ppc_nop_encoding_for_rs_align_code nop_select = *dest & 0xff;

  /* Pad with zeros if not inserting a whole number of instructions.
     We could pad with zeros up to an instruction boundary then follow
     with nops but odd counts indicate data in an executable section
     so padding with zeros is most appropriate.  */
  if (count == 0
      || (nop_select == PPC_NOP_VLE ? (count & 1) != 0 : (count & 3) != 0))
    {
      *dest = 0;
      return;
    }

  if (nop_select == PPC_NOP_VLE)
    {

      fragP->fr_var = 2;
      md_number_to_chars (dest, 0x4400, 2);
    }
  else
    {
      fragP->fr_var = 4;

      if (count > 4 * nop_limit && count < 0x2000000)
	{
	  struct frag *rest;

	  /* Make a branch, then follow with nops.  Insert another
	     frag to handle the nops.  */
	  md_number_to_chars (dest, 0x48000000 + count, 4);
	  count -= 4;
	  if (count == 0)
	    return;

	  rest = xmalloc (SIZEOF_STRUCT_FRAG + 4);
	  memcpy (rest, fragP, SIZEOF_STRUCT_FRAG);
	  fragP->fr_next = rest;
	  fragP = rest;
	  rest->fr_address += rest->fr_fix + 4;
	  rest->fr_fix = 0;
	  /* If we leave the next frag as rs_align_code we'll come here
	     again, resulting in a bunch of branches rather than a
	     branch followed by nops.  */
	  rest->fr_type = rs_align;
	  dest = rest->fr_literal;
	}

      md_number_to_chars (dest, 0x60000000, 4);

      if (nop_select >= PPC_NOP_GROUP_P6)
	{
	  /* For power6, power7, and power8, we want the last nop to
	     be a group terminating one.  Do this by inserting an
	     rs_fill frag immediately after this one, with its address
	     set to the last nop location.  This will automatically
	     reduce the number of nops in the current frag by one.  */
	  if (count > 4)
	    {
	      struct frag *group_nop = xmalloc (SIZEOF_STRUCT_FRAG + 4);

	      memcpy (group_nop, fragP, SIZEOF_STRUCT_FRAG);
	      group_nop->fr_address = group_nop->fr_next->fr_address - 4;
	      group_nop->fr_fix = 0;
	      group_nop->fr_offset = 1;
	      group_nop->fr_type = rs_fill;
	      fragP->fr_next = group_nop;
	      dest = group_nop->fr_literal;
	    }

	  if (nop_select == PPC_NOP_GROUP_P6)
	    /* power6 group terminating nop: "ori 1,1,0".  */
	    md_number_to_chars (dest, 0x60210000, 4);
	  else
	    /* power7/power8 group terminating nop: "ori 2,2,0".  */
	    md_number_to_chars (dest, 0x60420000, 4);
	}
    }
}

/* Apply a fixup to the object code.  This is called for all the
   fixups we generated by the calls to fix_new_exp, above.  */

void
md_apply_fix (fixS *fixP, valueT *valP, segT seg)
{
  valueT value = * valP;
  offsetT fieldval;
  const struct powerpc_operand *operand;

#ifdef OBJ_ELF
  if (fixP->fx_addsy != NULL)
    {
      /* Hack around bfd_install_relocation brain damage.  */
      if (fixP->fx_pcrel)
	value += fixP->fx_frag->fr_address + fixP->fx_where;

      if (fixP->fx_addsy == abs_section_sym)
	fixP->fx_done = 1;
    }
  else
    fixP->fx_done = 1;
#else
  /* FIXME FIXME FIXME: The value we are passed in *valP includes
     the symbol values.  If we are doing this relocation the code in
     write.c is going to call bfd_install_relocation, which is also
     going to use the symbol value.  That means that if the reloc is
     fully resolved we want to use *valP since bfd_install_relocation is
     not being used.
     However, if the reloc is not fully resolved we do not want to
     use *valP, and must use fx_offset instead.  If the relocation
     is PC-relative, we then need to re-apply md_pcrel_from_section
     to this new relocation value.  */
  if (fixP->fx_addsy == (symbolS *) NULL)
    fixP->fx_done = 1;

  else
    {
      value = fixP->fx_offset;
      if (fixP->fx_pcrel)
	value -= md_pcrel_from_section (fixP, seg);
    }
#endif

  /* We are only able to convert some relocs to pc-relative.  */
  if (fixP->fx_pcrel)
    {
      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_64:
	  fixP->fx_r_type = BFD_RELOC_64_PCREL;
	  break;

	case BFD_RELOC_32:
	  fixP->fx_r_type = BFD_RELOC_32_PCREL;
	  break;

	case BFD_RELOC_16:
	  fixP->fx_r_type = BFD_RELOC_16_PCREL;
	  break;

	case BFD_RELOC_LO16:
	  fixP->fx_r_type = BFD_RELOC_LO16_PCREL;
	  break;

	case BFD_RELOC_HI16:
	  fixP->fx_r_type = BFD_RELOC_HI16_PCREL;
	  break;

	case BFD_RELOC_HI16_S:
	  fixP->fx_r_type = BFD_RELOC_HI16_S_PCREL;
	  break;

	case BFD_RELOC_PPC64_ADDR16_HIGH:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGH;
	  break;

	case BFD_RELOC_PPC64_ADDR16_HIGHA:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHA;
	  break;

	case BFD_RELOC_PPC64_HIGHER:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHER;
	  break;

	case BFD_RELOC_PPC64_HIGHER_S:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHERA;
	  break;

	case BFD_RELOC_PPC64_HIGHEST:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHEST;
	  break;

	case BFD_RELOC_PPC64_HIGHEST_S:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHESTA;
	  break;

	case BFD_RELOC_PPC64_ADDR16_HIGHER34:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHER34;
	  break;

	case BFD_RELOC_PPC64_ADDR16_HIGHERA34:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHERA34;
	  break;

	case BFD_RELOC_PPC64_ADDR16_HIGHEST34:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHEST34;
	  break;

	case BFD_RELOC_PPC64_ADDR16_HIGHESTA34:
	  fixP->fx_r_type = BFD_RELOC_PPC64_REL16_HIGHESTA34;
	  break;

	case BFD_RELOC_PPC_16DX_HA:
	  fixP->fx_r_type = BFD_RELOC_PPC_REL16DX_HA;
	  break;

	case BFD_RELOC_PPC64_D34:
	  fixP->fx_r_type = BFD_RELOC_PPC64_PCREL34;
	  break;

	case BFD_RELOC_PPC64_D28:
	  fixP->fx_r_type = BFD_RELOC_PPC64_PCREL28;
	  break;

	default:
	  break;
	}
    }
  else if (!fixP->fx_done
	   && fixP->fx_r_type == BFD_RELOC_PPC_16DX_HA)
    {
      /* addpcis is relative to next insn address.  */
      value -= 4;
      fixP->fx_r_type = BFD_RELOC_PPC_REL16DX_HA;
      fixP->fx_pcrel = 1;
    }

  operand = NULL;
  if (fixP->fx_pcrel_adjust != 0)
    {
      /* This is a fixup on an instruction.  */
      int opindex = fixP->fx_pcrel_adjust & 0xff;

      operand = &powerpc_operands[opindex];
#ifdef OBJ_XCOFF
      /* An instruction like `lwz 9,sym(30)' when `sym' is not a TOC symbol
	 does not generate a reloc.  It uses the offset of `sym' within its
	 csect.  Other usages, such as `.long sym', generate relocs.  This
	 is the documented behaviour of non-TOC symbols.  */
      if ((operand->flags & PPC_OPERAND_PARENS) != 0
	  && (operand->bitm & 0xfff0) == 0xfff0
	  && operand->shift == 0
	  && (operand->insert == NULL || ppc_obj64)
	  && fixP->fx_addsy != NULL
	  && symbol_get_tc (fixP->fx_addsy)->subseg != 0
	  && symbol_get_tc (fixP->fx_addsy)->symbol_class != XMC_TC
	  && symbol_get_tc (fixP->fx_addsy)->symbol_class != XMC_TC0
	  && S_GET_SEGMENT (fixP->fx_addsy) != bss_section)
	{
	  value = fixP->fx_offset;
	  fixP->fx_done = 1;
	}

       /* During parsing of instructions, a TOC16 reloc is generated for
          instructions such as 'lwz RT,SYM(RB)' if SYM is a symbol defined
          in the toc.  But at parse time, SYM may be not yet defined, so
          check again here.  */
       if (fixP->fx_r_type == BFD_RELOC_16
           && fixP->fx_addsy != NULL
           && ppc_is_toc_sym (fixP->fx_addsy))
         fixP->fx_r_type = BFD_RELOC_PPC_TOC16;
#endif
    }

  /* Calculate value to be stored in field.  */
  fieldval = value;
  switch (fixP->fx_r_type)
    {
#ifdef OBJ_ELF
    case BFD_RELOC_PPC64_ADDR16_LO_DS:
    case BFD_RELOC_PPC_VLE_LO16A:
    case BFD_RELOC_PPC_VLE_LO16D:
#endif
    case BFD_RELOC_LO16:
    case BFD_RELOC_LO16_PCREL:
      fieldval = value & 0xffff;
    sign_extend_16:
      if (operand != NULL && (operand->flags & PPC_OPERAND_SIGNED) != 0)
	fieldval = SEX16 (fieldval);
      fixP->fx_no_overflow = 1;
      break;

    case BFD_RELOC_HI16:
    case BFD_RELOC_HI16_PCREL:
#ifdef OBJ_ELF
      if (REPORT_OVERFLOW_HI && ppc_obj64)
	{
	  fieldval = value >> 16;
	  if (operand != NULL && (operand->flags & PPC_OPERAND_SIGNED) != 0)
	    {
	      valueT sign = (((valueT) -1 >> 16) + 1) >> 1;
	      fieldval = ((valueT) fieldval ^ sign) - sign;
	    }
	  break;
	}
      /* Fallthru */

    case BFD_RELOC_PPC_VLE_HI16A:
    case BFD_RELOC_PPC_VLE_HI16D:
    case BFD_RELOC_PPC64_ADDR16_HIGH:
#endif
      fieldval = PPC_HI (value);
      goto sign_extend_16;

    case BFD_RELOC_HI16_S:
    case BFD_RELOC_HI16_S_PCREL:
    case BFD_RELOC_PPC_16DX_HA:
    case BFD_RELOC_PPC_REL16DX_HA:
#ifdef OBJ_ELF
      if (REPORT_OVERFLOW_HI && ppc_obj64)
	{
	  fieldval = (value + 0x8000) >> 16;
	  if (operand != NULL && (operand->flags & PPC_OPERAND_SIGNED) != 0)
	    {
	      valueT sign = (((valueT) -1 >> 16) + 1) >> 1;
	      fieldval = ((valueT) fieldval ^ sign) - sign;
	    }
	  break;
	}
      /* Fallthru */

    case BFD_RELOC_PPC_VLE_HA16A:
    case BFD_RELOC_PPC_VLE_HA16D:
    case BFD_RELOC_PPC64_ADDR16_HIGHA:
#endif
      fieldval = PPC_HA (value);
      goto sign_extend_16;

#ifdef OBJ_ELF
    case BFD_RELOC_PPC64_HIGHER:
      fieldval = PPC_HIGHER (value);
      goto sign_extend_16;

    case BFD_RELOC_PPC64_HIGHER_S:
      fieldval = PPC_HIGHERA (value);
      goto sign_extend_16;

    case BFD_RELOC_PPC64_HIGHEST:
      fieldval = PPC_HIGHEST (value);
      goto sign_extend_16;

    case BFD_RELOC_PPC64_HIGHEST_S:
      fieldval = PPC_HIGHESTA (value);
      goto sign_extend_16;
#endif

    default:
      break;
    }

  if (operand != NULL)
    {
      /* Handle relocs in an insn.  */
      switch (fixP->fx_r_type)
	{
#ifdef OBJ_ELF
	  /* The following relocs can't be calculated by the assembler.
	     Leave the field zero.  */
	case BFD_RELOC_PPC_TPREL16:
	case BFD_RELOC_PPC_TPREL16_LO:
	case BFD_RELOC_PPC_TPREL16_HI:
	case BFD_RELOC_PPC_TPREL16_HA:
	case BFD_RELOC_PPC_DTPREL16:
	case BFD_RELOC_PPC_DTPREL16_LO:
	case BFD_RELOC_PPC_DTPREL16_HI:
	case BFD_RELOC_PPC_DTPREL16_HA:
	case BFD_RELOC_PPC_GOT_TLSGD16:
	case BFD_RELOC_PPC_GOT_TLSGD16_LO:
	case BFD_RELOC_PPC_GOT_TLSGD16_HI:
	case BFD_RELOC_PPC_GOT_TLSGD16_HA:
	case BFD_RELOC_PPC_GOT_TLSLD16:
	case BFD_RELOC_PPC_GOT_TLSLD16_LO:
	case BFD_RELOC_PPC_GOT_TLSLD16_HI:
	case BFD_RELOC_PPC_GOT_TLSLD16_HA:
	case BFD_RELOC_PPC_GOT_TPREL16:
	case BFD_RELOC_PPC_GOT_TPREL16_LO:
	case BFD_RELOC_PPC_GOT_TPREL16_HI:
	case BFD_RELOC_PPC_GOT_TPREL16_HA:
	case BFD_RELOC_PPC_GOT_DTPREL16:
	case BFD_RELOC_PPC_GOT_DTPREL16_LO:
	case BFD_RELOC_PPC_GOT_DTPREL16_HI:
	case BFD_RELOC_PPC_GOT_DTPREL16_HA:
	case BFD_RELOC_PPC64_TPREL16_DS:
	case BFD_RELOC_PPC64_TPREL16_LO_DS:
	case BFD_RELOC_PPC64_TPREL16_HIGH:
	case BFD_RELOC_PPC64_TPREL16_HIGHA:
	case BFD_RELOC_PPC64_TPREL16_HIGHER:
	case BFD_RELOC_PPC64_TPREL16_HIGHERA:
	case BFD_RELOC_PPC64_TPREL16_HIGHEST:
	case BFD_RELOC_PPC64_TPREL16_HIGHESTA:
	case BFD_RELOC_PPC64_DTPREL16_HIGH:
	case BFD_RELOC_PPC64_DTPREL16_HIGHA:
	case BFD_RELOC_PPC64_DTPREL16_DS:
	case BFD_RELOC_PPC64_DTPREL16_LO_DS:
	case BFD_RELOC_PPC64_DTPREL16_HIGHER:
	case BFD_RELOC_PPC64_DTPREL16_HIGHERA:
	case BFD_RELOC_PPC64_DTPREL16_HIGHEST:
	case BFD_RELOC_PPC64_DTPREL16_HIGHESTA:
	case BFD_RELOC_PPC64_TPREL34:
	case BFD_RELOC_PPC64_DTPREL34:
	case BFD_RELOC_PPC64_GOT_TLSGD34:
	case BFD_RELOC_PPC64_GOT_TLSLD34:
	case BFD_RELOC_PPC64_GOT_TPREL34:
	case BFD_RELOC_PPC64_GOT_DTPREL34:
	  gas_assert (fixP->fx_addsy != NULL);
	  S_SET_THREAD_LOCAL (fixP->fx_addsy);
	  fieldval = 0;
	  break;

	  /* These also should leave the field zero for the same
	     reason.  Note that older versions of gas wrote values
	     here.  If we want to go back to the old behaviour, then
	     all _LO and _LO_DS cases will need to be treated like
	     BFD_RELOC_LO16_PCREL above.  Similarly for _HI etc.  */
	case BFD_RELOC_16_GOTOFF:
	case BFD_RELOC_LO16_GOTOFF:
	case BFD_RELOC_HI16_GOTOFF:
	case BFD_RELOC_HI16_S_GOTOFF:
	case BFD_RELOC_LO16_PLTOFF:
	case BFD_RELOC_HI16_PLTOFF:
	case BFD_RELOC_HI16_S_PLTOFF:
	case BFD_RELOC_GPREL16:
	case BFD_RELOC_16_BASEREL:
	case BFD_RELOC_LO16_BASEREL:
	case BFD_RELOC_HI16_BASEREL:
	case BFD_RELOC_HI16_S_BASEREL:
	case BFD_RELOC_PPC_TOC16:
	case BFD_RELOC_PPC64_TOC16_LO:
	case BFD_RELOC_PPC64_TOC16_HI:
	case BFD_RELOC_PPC64_TOC16_HA:
	case BFD_RELOC_PPC64_PLTGOT16:
	case BFD_RELOC_PPC64_PLTGOT16_LO:
	case BFD_RELOC_PPC64_PLTGOT16_HI:
	case BFD_RELOC_PPC64_PLTGOT16_HA:
	case BFD_RELOC_PPC64_GOT16_DS:
	case BFD_RELOC_PPC64_GOT16_LO_DS:
	case BFD_RELOC_PPC64_PLT16_LO_DS:
	case BFD_RELOC_PPC64_SECTOFF_DS:
	case BFD_RELOC_PPC64_SECTOFF_LO_DS:
	case BFD_RELOC_PPC64_TOC16_DS:
	case BFD_RELOC_PPC64_TOC16_LO_DS:
	case BFD_RELOC_PPC64_PLTGOT16_DS:
	case BFD_RELOC_PPC64_PLTGOT16_LO_DS:
	case BFD_RELOC_PPC_EMB_NADDR16:
	case BFD_RELOC_PPC_EMB_NADDR16_LO:
	case BFD_RELOC_PPC_EMB_NADDR16_HI:
	case BFD_RELOC_PPC_EMB_NADDR16_HA:
	case BFD_RELOC_PPC_EMB_SDAI16:
	case BFD_RELOC_PPC_EMB_SDA2I16:
	case BFD_RELOC_PPC_EMB_SDA2REL:
	case BFD_RELOC_PPC_EMB_SDA21:
	case BFD_RELOC_PPC_EMB_MRKREF:
	case BFD_RELOC_PPC_EMB_RELSEC16:
	case BFD_RELOC_PPC_EMB_RELST_LO:
	case BFD_RELOC_PPC_EMB_RELST_HI:
	case BFD_RELOC_PPC_EMB_RELST_HA:
	case BFD_RELOC_PPC_EMB_BIT_FLD:
	case BFD_RELOC_PPC_EMB_RELSDA:
	case BFD_RELOC_PPC_VLE_SDA21:
	case BFD_RELOC_PPC_VLE_SDA21_LO:
	case BFD_RELOC_PPC_VLE_SDAREL_LO16A:
	case BFD_RELOC_PPC_VLE_SDAREL_LO16D:
	case BFD_RELOC_PPC_VLE_SDAREL_HI16A:
	case BFD_RELOC_PPC_VLE_SDAREL_HI16D:
	case BFD_RELOC_PPC_VLE_SDAREL_HA16A:
	case BFD_RELOC_PPC_VLE_SDAREL_HA16D:
	case BFD_RELOC_PPC64_GOT_PCREL34:
	case BFD_RELOC_PPC64_PLT_PCREL34:
	  gas_assert (fixP->fx_addsy != NULL);
	  /* Fallthru */

	case BFD_RELOC_PPC_TLS:
	case BFD_RELOC_PPC_TLSGD:
	case BFD_RELOC_PPC_TLSLD:
	case BFD_RELOC_PPC64_TLS_PCREL:
	  fieldval = 0;
	  break;
#endif

#ifdef OBJ_XCOFF
	case BFD_RELOC_PPC_B16:
	  /* Adjust the offset to the instruction boundary.  */
	  fieldval += 2;
	  break;
#endif

	case BFD_RELOC_VTABLE_INHERIT:
	case BFD_RELOC_VTABLE_ENTRY:
	case BFD_RELOC_PPC_DTPMOD:
	case BFD_RELOC_PPC_TPREL:
	case BFD_RELOC_PPC_DTPREL:
	case BFD_RELOC_PPC_COPY:
	case BFD_RELOC_PPC_GLOB_DAT:
	case BFD_RELOC_32_PLT_PCREL:
	case BFD_RELOC_PPC_EMB_NADDR32:
	case BFD_RELOC_PPC64_TOC:
	case BFD_RELOC_CTOR:
	case BFD_RELOC_32:
	case BFD_RELOC_32_PCREL:
	case BFD_RELOC_RVA:
	case BFD_RELOC_64:
	case BFD_RELOC_64_PCREL:
	case BFD_RELOC_PPC64_ADDR64_LOCAL:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("%s unsupported as instruction fixup"),
			bfd_get_reloc_code_name (fixP->fx_r_type));
	  fixP->fx_done = 1;
	  return;

	default:
	  break;
	}

#ifdef OBJ_ELF
/* powerpc uses RELA style relocs, so if emitting a reloc the field
   contents can stay at zero.  */
#define APPLY_RELOC fixP->fx_done
#else
#define APPLY_RELOC 1
#endif
      /* We need to call the insert function even when fieldval is
	 zero if the insert function would translate that zero to a
	 bit pattern other than all zeros.  */
      if ((fieldval != 0 && APPLY_RELOC) || operand->insert != NULL)
	{
	  uint64_t insn;
	  unsigned char *where;

	  /* Fetch the instruction, insert the fully resolved operand
	     value, and stuff the instruction back again.  */
	  where = (unsigned char *) fixP->fx_frag->fr_literal + fixP->fx_where;
	  if (target_big_endian)
	    {
	      if (fixP->fx_size < 4)
		insn = bfd_getb16 (where);
	      else
		{
		  insn = bfd_getb32 (where);
		  if (fixP->fx_size > 4)
		    insn = insn << 32 | bfd_getb32 (where + 4);
		}
	    }
	  else
	    {
	      if (fixP->fx_size < 4)
		insn = bfd_getl16 (where);
	      else
		{
		  insn = bfd_getl32 (where);
		  if (fixP->fx_size > 4)
		    insn = insn << 32 | bfd_getl32 (where + 4);
		}
	    }
	  insn = ppc_insert_operand (insn, operand, fieldval,
				     fixP->tc_fix_data.ppc_cpu,
				     fixP->fx_file, fixP->fx_line);
	  if (target_big_endian)
	    {
	      if (fixP->fx_size < 4)
		bfd_putb16 (insn, where);
	      else
		{
		  if (fixP->fx_size > 4)
		    {
		      bfd_putb32 (insn, where + 4);
		      insn >>= 32;
		    }
		  bfd_putb32 (insn, where);
		}
	    }
	  else
	    {
	      if (fixP->fx_size < 4)
		bfd_putl16 (insn, where);
	      else
		{
		  if (fixP->fx_size > 4)
		    {
		      bfd_putl32 (insn, where + 4);
		      insn >>= 32;
		    }
		  bfd_putl32 (insn, where);
		}
	    }
	}

      if (fixP->fx_done)
	/* Nothing else to do here.  */
	return;

      gas_assert (fixP->fx_addsy != NULL);
      if (fixP->fx_r_type == BFD_RELOC_NONE)
	{
	  const char *sfile;
	  unsigned int sline;

	  /* Use expr_symbol_where to see if this is an expression
	     symbol.  */
	  if (expr_symbol_where (fixP->fx_addsy, &sfile, &sline))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("unresolved expression that must be resolved"));
	  else
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("unsupported relocation against %s"),
			  S_GET_NAME (fixP->fx_addsy));
	  fixP->fx_done = 1;
	  return;
	}
    }
  else
    {
      /* Handle relocs in data.  */
      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_VTABLE_INHERIT:
	  if (fixP->fx_addsy
	      && !S_IS_DEFINED (fixP->fx_addsy)
	      && !S_IS_WEAK (fixP->fx_addsy))
	    S_SET_WEAK (fixP->fx_addsy);
	  /* Fallthru */

	case BFD_RELOC_VTABLE_ENTRY:
	  fixP->fx_done = 0;
	  break;

#ifdef OBJ_ELF
	  /* These can appear with @l etc. in data.  */
	case BFD_RELOC_LO16:
	case BFD_RELOC_LO16_PCREL:
	case BFD_RELOC_HI16:
	case BFD_RELOC_HI16_PCREL:
	case BFD_RELOC_HI16_S:
	case BFD_RELOC_HI16_S_PCREL:
	case BFD_RELOC_PPC64_HIGHER:
	case BFD_RELOC_PPC64_HIGHER_S:
	case BFD_RELOC_PPC64_HIGHEST:
	case BFD_RELOC_PPC64_HIGHEST_S:
	case BFD_RELOC_PPC64_ADDR16_HIGH:
	case BFD_RELOC_PPC64_ADDR16_HIGHA:
	case BFD_RELOC_PPC64_ADDR64_LOCAL:
	  break;

	case BFD_RELOC_PPC_DTPMOD:
	case BFD_RELOC_PPC_TPREL:
	case BFD_RELOC_PPC_DTPREL:
	  S_SET_THREAD_LOCAL (fixP->fx_addsy);
	  break;

	  /* Just punt all of these to the linker.  */
	case BFD_RELOC_PPC_B16_BRTAKEN:
	case BFD_RELOC_PPC_B16_BRNTAKEN:
	case BFD_RELOC_16_GOTOFF:
	case BFD_RELOC_LO16_GOTOFF:
	case BFD_RELOC_HI16_GOTOFF:
	case BFD_RELOC_HI16_S_GOTOFF:
	case BFD_RELOC_LO16_PLTOFF:
	case BFD_RELOC_HI16_PLTOFF:
	case BFD_RELOC_HI16_S_PLTOFF:
	case BFD_RELOC_PPC_COPY:
	case BFD_RELOC_PPC_GLOB_DAT:
	case BFD_RELOC_16_BASEREL:
	case BFD_RELOC_LO16_BASEREL:
	case BFD_RELOC_HI16_BASEREL:
	case BFD_RELOC_HI16_S_BASEREL:
	case BFD_RELOC_PPC_TLS:
	case BFD_RELOC_PPC_DTPREL16_LO:
	case BFD_RELOC_PPC_DTPREL16_HI:
	case BFD_RELOC_PPC_DTPREL16_HA:
	case BFD_RELOC_PPC_TPREL16_LO:
	case BFD_RELOC_PPC_TPREL16_HI:
	case BFD_RELOC_PPC_TPREL16_HA:
	case BFD_RELOC_PPC_GOT_TLSGD16:
	case BFD_RELOC_PPC_GOT_TLSGD16_LO:
	case BFD_RELOC_PPC_GOT_TLSGD16_HI:
	case BFD_RELOC_PPC_GOT_TLSGD16_HA:
	case BFD_RELOC_PPC_GOT_TLSLD16:
	case BFD_RELOC_PPC_GOT_TLSLD16_LO:
	case BFD_RELOC_PPC_GOT_TLSLD16_HI:
	case BFD_RELOC_PPC_GOT_TLSLD16_HA:
	case BFD_RELOC_PPC_GOT_DTPREL16:
	case BFD_RELOC_PPC_GOT_DTPREL16_LO:
	case BFD_RELOC_PPC_GOT_DTPREL16_HI:
	case BFD_RELOC_PPC_GOT_DTPREL16_HA:
	case BFD_RELOC_PPC_GOT_TPREL16:
	case BFD_RELOC_PPC_GOT_TPREL16_LO:
	case BFD_RELOC_PPC_GOT_TPREL16_HI:
	case BFD_RELOC_PPC_GOT_TPREL16_HA:
	case BFD_RELOC_24_PLT_PCREL:
	case BFD_RELOC_PPC_LOCAL24PC:
	case BFD_RELOC_32_PLT_PCREL:
	case BFD_RELOC_GPREL16:
	case BFD_RELOC_PPC_VLE_SDAREL_LO16A:
	case BFD_RELOC_PPC_VLE_SDAREL_HI16A:
	case BFD_RELOC_PPC_VLE_SDAREL_HA16A:
	case BFD_RELOC_PPC_EMB_NADDR32:
	case BFD_RELOC_PPC_EMB_NADDR16:
	case BFD_RELOC_PPC_EMB_NADDR16_LO:
	case BFD_RELOC_PPC_EMB_NADDR16_HI:
	case BFD_RELOC_PPC_EMB_NADDR16_HA:
	case BFD_RELOC_PPC_EMB_SDAI16:
	case BFD_RELOC_PPC_EMB_SDA2REL:
	case BFD_RELOC_PPC_EMB_SDA2I16:
	case BFD_RELOC_PPC_EMB_SDA21:
	case BFD_RELOC_PPC_VLE_SDA21_LO:
	case BFD_RELOC_PPC_EMB_MRKREF:
	case BFD_RELOC_PPC_EMB_RELSEC16:
	case BFD_RELOC_PPC_EMB_RELST_LO:
	case BFD_RELOC_PPC_EMB_RELST_HI:
	case BFD_RELOC_PPC_EMB_RELST_HA:
	case BFD_RELOC_PPC_EMB_BIT_FLD:
	case BFD_RELOC_PPC_EMB_RELSDA:
	case BFD_RELOC_PPC64_TOC:
	case BFD_RELOC_PPC_TOC16:
	case BFD_RELOC_PPC64_TOC16_LO:
	case BFD_RELOC_PPC64_TOC16_HI:
	case BFD_RELOC_PPC64_TOC16_HA:
	case BFD_RELOC_PPC64_DTPREL16_HIGH:
	case BFD_RELOC_PPC64_DTPREL16_HIGHA:
	case BFD_RELOC_PPC64_DTPREL16_HIGHER:
	case BFD_RELOC_PPC64_DTPREL16_HIGHERA:
	case BFD_RELOC_PPC64_DTPREL16_HIGHEST:
	case BFD_RELOC_PPC64_DTPREL16_HIGHESTA:
	case BFD_RELOC_PPC64_TPREL16_HIGH:
	case BFD_RELOC_PPC64_TPREL16_HIGHA:
	case BFD_RELOC_PPC64_TPREL16_HIGHER:
	case BFD_RELOC_PPC64_TPREL16_HIGHERA:
	case BFD_RELOC_PPC64_TPREL16_HIGHEST:
	case BFD_RELOC_PPC64_TPREL16_HIGHESTA:
	case BFD_RELOC_PPC64_TLS_PCREL:
	  fixP->fx_done = 0;
	  break;
#endif

#ifdef OBJ_XCOFF
	case BFD_RELOC_NONE:
#endif
	case BFD_RELOC_CTOR:
	case BFD_RELOC_32:
	case BFD_RELOC_32_PCREL:
	case BFD_RELOC_RVA:
	case BFD_RELOC_64:
	case BFD_RELOC_64_PCREL:
	case BFD_RELOC_16:
	case BFD_RELOC_16_PCREL:
	case BFD_RELOC_8:
	  break;

	default:
	  fprintf (stderr,
		   _("Gas failure, reloc value %d\n"), fixP->fx_r_type);
	  fflush (stderr);
	  abort ();
	}

      if (fixP->fx_size && APPLY_RELOC)
	md_number_to_chars (fixP->fx_frag->fr_literal + fixP->fx_where,
			    fieldval, fixP->fx_size);
      if (warn_476
	  && (seg->flags & SEC_CODE) != 0
	  && fixP->fx_size == 4
	  && fixP->fx_done
	  && !fixP->fx_tcbit
	  && (fixP->fx_r_type == BFD_RELOC_32
	      || fixP->fx_r_type == BFD_RELOC_CTOR
	      || fixP->fx_r_type == BFD_RELOC_32_PCREL))
	as_warn_where (fixP->fx_file, fixP->fx_line,
		       _("data in executable section"));
    }

#ifdef OBJ_ELF
  ppc_elf_validate_fix (fixP, seg);
  fixP->fx_addnumber = value;

  /* PowerPC uses RELA relocs, ie. the reloc addend is stored separately
     from the section contents.  If we are going to be emitting a reloc
     then the section contents are immaterial, so don't warn if they
     happen to overflow.  Leave such warnings to ld.  */
  if (!fixP->fx_done)
    {
      fixP->fx_no_overflow = 1;

      /* Arrange to emit .TOC. as a normal symbol if used in anything
	 but .TOC.@tocbase.  */
      if (ppc_obj64
	  && fixP->fx_r_type != BFD_RELOC_PPC64_TOC
	  && fixP->fx_addsy != NULL
	  && strcmp (S_GET_NAME (fixP->fx_addsy), ".TOC.") == 0)
	symbol_get_bfdsym (fixP->fx_addsy)->flags |= BSF_KEEP;
    }
#else
  if (fixP->fx_r_type != BFD_RELOC_PPC_TOC16)
    fixP->fx_addnumber = 0;
  else
    {
#ifdef TE_PE
      fixP->fx_addnumber = 0;
#else
      /* We want to use the offset within the toc, not the actual VMA
	 of the symbol.  */
      fixP->fx_addnumber = (- bfd_section_vma (S_GET_SEGMENT (fixP->fx_addsy))
			    - S_GET_VALUE (ppc_toc_csect));
      /* Set *valP to avoid errors.  */
      *valP = value;
#endif
    }
#endif
}

/* Generate a reloc for a fixup.  */

arelent *
tc_gen_reloc (asection *seg ATTRIBUTE_UNUSED, fixS *fixp)
{
  arelent *reloc;

  reloc = XNEW (arelent);

  reloc->sym_ptr_ptr = XNEW (asymbol *);
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;
  /* BFD_RELOC_PPC64_TLS_PCREL generates R_PPC64_TLS with an odd r_offset.  */
  if (fixp->fx_r_type == BFD_RELOC_PPC64_TLS_PCREL)
    reloc->address++;
  reloc->howto = bfd_reloc_type_lookup (stdoutput, fixp->fx_r_type);
  if (reloc->howto == (reloc_howto_type *) NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("reloc %d not supported by object file format"),
		    (int) fixp->fx_r_type);
      return NULL;
    }
  reloc->addend = fixp->fx_addnumber;

  return reloc;
}

void
ppc_cfi_frame_initial_instructions (void)
{
  cfi_add_CFA_def_cfa (1, 0);
}

int
tc_ppc_regname_to_dw2regnum (char *regname)
{
  unsigned int regnum = -1;
  unsigned int i;
  const char *p;
  char *q;
  static struct { const char *name; int dw2regnum; } regnames[] =
    {
      { "sp", 1 }, { "r.sp", 1 }, { "rtoc", 2 }, { "r.toc", 2 },
      { "mq", 64 }, { "lr", 65 }, { "ctr", 66 }, { "ap", 67 },
      { "cr", 70 }, { "xer", 76 }, { "vrsave", 109 }, { "vscr", 110 },
      { "spe_acc", 111 }, { "spefscr", 112 }
    };

  for (i = 0; i < ARRAY_SIZE (regnames); ++i)
    if (strcmp (regnames[i].name, regname) == 0)
      return regnames[i].dw2regnum;

  if (regname[0] == 'r' || regname[0] == 'f' || regname[0] == 'v')
    {
      p = regname + 1 + (regname[1] == '.');
      regnum = strtoul (p, &q, 10);
      if (p == q || *q || regnum >= 32)
	return -1;
      if (regname[0] == 'f')
	regnum += 32;
      else if (regname[0] == 'v')
	regnum += 77;
    }
  else if (regname[0] == 'c' && regname[1] == 'r')
    {
      p = regname + 2 + (regname[2] == '.');
      if (p[0] < '0' || p[0] > '7' || p[1])
	return -1;
      regnum = p[0] - '0' + 68;
    }
  return regnum;
}
