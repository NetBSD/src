/* Target-dependent code for Hitachi Super-H, for GDB.
   Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/*
   Contributed by Steve Chamberlain
   sac@cygnus.com
 */

#include "defs.h"
#include "frame.h"
#include "symtab.h"
#include "symfile.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "value.h"
#include "dis-asm.h"
#include "inferior.h"		/* for BEFORE_TEXT_END etc. */
#include "gdb_string.h"
#include "arch-utils.h"
#include "floatformat.h"
#include "regcache.h"
#include "doublest.h"

#include "sh-tdep.h"

#include "elf-bfd.h"
#include "solib-svr4.h"

/* sh64 flags */
#include "elf/sh.h"
/* registers numbers shared with the simulator */
#include "gdb/sim-sh.h"

void (*sh_show_regs) (void);
CORE_ADDR (*skip_prologue_hard_way) (CORE_ADDR);
void (*do_pseudo_register) (int);

#define SH_DEFAULT_NUM_REGS 59

/* Define other aspects of the stack frame.
   we keep a copy of the worked out return pc lying around, since it
   is a useful bit of info */
  
struct frame_extra_info
{
  CORE_ADDR return_pc;
  int leaf_function;
  int f_offset;
};

static const char *
sh_generic_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    "fpul", "fpscr",
    "fr0",  "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9",  "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    "ssr",  "spc",
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
    "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh3_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
    "ssr",  "spc",
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1"
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh3e_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    "fpul", "fpscr",
    "fr0",  "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9",  "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    "ssr",  "spc",
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh_dsp_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    "",     "dsr",
    "a0g",  "a0",   "a1g",  "a1",   "m0",   "m1",   "x0",   "x1",
    "y0",   "y1",   "",     "",     "",     "",     "",     "mod",
    "",     "",
    "rs",   "re",   "",     "",     "",     "",     "",     "",
    "",     "",     "",     "",     "",     "",     "",     "",
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh3_dsp_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    "",     "dsr",
    "a0g",  "a0",   "a1g",  "a1",   "m0",   "m1",   "x0",   "x1",
    "y0",   "y1",   "",     "",     "",     "",     "",     "mod",
    "ssr",  "spc",
    "rs",   "re",   "",     "",     "",     "",     "",     "",
    "r0b",  "r1b",  "r2b",  "r3b",  "r4b",  "r5b",  "r6b",  "r7b"
    "",     "",     "",     "",     "",     "",     "",     "",
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh4_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    /* general registers 0-15 */
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    /* 16 - 22 */
    "pc",   "pr",   "gbr",  "vbr",  "mach", "macl", "sr",
    /* 23, 24 */
    "fpul", "fpscr",
    /* floating point registers 25 - 40 */
    "fr0",  "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9",  "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    /* 41, 42 */
    "ssr",  "spc",
    /* bank 0 43 - 50 */
    "r0b0", "r1b0", "r2b0", "r3b0", "r4b0", "r5b0", "r6b0", "r7b0",
    /* bank 1 51 - 58 */
    "r0b1", "r1b1", "r2b1", "r3b1", "r4b1", "r5b1", "r6b1", "r7b1",
    /* double precision (pseudo) 59 - 66 */
    "dr0",  "dr2",  "dr4",  "dr6",  "dr8",  "dr10", "dr12", "dr14",
    /* vectors (pseudo) 67 - 70 */
    "fv0",  "fv4",  "fv8",  "fv12",
    /* FIXME: missing XF 71 - 86 */
    /* FIXME: missing XD 87 - 94 */
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

static const char *
sh_sh64_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    /* SH MEDIA MODE (ISA 32) */
    /* general registers (64-bit) 0-63 */
    "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
    "r16",  "r17",  "r18",  "r19",  "r20",  "r21",  "r22",  "r23",
    "r24",  "r25",  "r26",  "r27",  "r28",  "r29",  "r30",  "r31",
    "r32",  "r33",  "r34",  "r35",  "r36",  "r37",  "r38",  "r39",
    "r40",  "r41",  "r42",  "r43",  "r44",  "r45",  "r46",  "r47",
    "r48",  "r49",  "r50",  "r51",  "r52",  "r53",  "r54",  "r55",
    "r56",  "r57",  "r58",  "r59",  "r60",  "r61",  "r62",  "r63",

    /* pc (64-bit) 64 */
    "pc",   

    /* status reg., saved status reg., saved pc reg. (64-bit) 65-67 */
    "sr",  "ssr",  "spc", 

    /* target registers (64-bit) 68-75*/
    "tr0",  "tr1",  "tr2",  "tr3",  "tr4",  "tr5",  "tr6",  "tr7",

    /* floating point state control register (32-bit) 76 */
    "fpscr",

    /* single precision floating point registers (32-bit) 77-140*/
    "fr0",  "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9",  "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    "fr16", "fr17", "fr18", "fr19", "fr20", "fr21", "fr22", "fr23",
    "fr24", "fr25", "fr26", "fr27", "fr28", "fr29", "fr30", "fr31",
    "fr32", "fr33", "fr34", "fr35", "fr36", "fr37", "fr38", "fr39",
    "fr40", "fr41", "fr42", "fr43", "fr44", "fr45", "fr46", "fr47",
    "fr48", "fr49", "fr50", "fr51", "fr52", "fr53", "fr54", "fr55",
    "fr56", "fr57", "fr58", "fr59", "fr60", "fr61", "fr62", "fr63",

    /* double precision registers (pseudo) 141-172 */
    "dr0",  "dr2",  "dr4",  "dr6",  "dr8",  "dr10", "dr12", "dr14",
    "dr16", "dr18", "dr20", "dr22", "dr24", "dr26", "dr28", "dr30",
    "dr32", "dr34", "dr36", "dr38", "dr40", "dr42", "dr44", "dr46",
    "dr48", "dr50", "dr52", "dr54", "dr56", "dr58", "dr60", "dr62",

    /* floating point pairs (pseudo) 173-204*/
    "fp0",  "fp2",  "fp4",  "fp6",  "fp8",  "fp10", "fp12", "fp14",
    "fp16", "fp18", "fp20", "fp22", "fp24", "fp26", "fp28", "fp30",
    "fp32", "fp34", "fp36", "fp38", "fp40", "fp42", "fp44", "fp46",
    "fp48", "fp50", "fp52", "fp54", "fp56", "fp58", "fp60", "fp62",

    /* floating point vectors (4 floating point regs) (pseudo) 205-220*/
    "fv0",  "fv4",  "fv8",  "fv12", "fv16", "fv20", "fv24", "fv28",
    "fv32", "fv36", "fv40", "fv44", "fv48", "fv52", "fv56", "fv60",

    /* SH COMPACT MODE (ISA 16) (all pseudo) 221-272*/
    "r0_c", "r1_c", "r2_c",  "r3_c",  "r4_c",  "r5_c",  "r6_c",  "r7_c",
    "r8_c", "r9_c", "r10_c", "r11_c", "r12_c", "r13_c", "r14_c", "r15_c",
    "pc_c",
    "gbr_c", "mach_c", "macl_c", "pr_c", "t_c",
    "fpscr_c", "fpul_c",
    "fr0_c", "fr1_c", "fr2_c",  "fr3_c",  "fr4_c",  "fr5_c",  "fr6_c",  "fr7_c",
    "fr8_c", "fr9_c", "fr10_c", "fr11_c", "fr12_c", "fr13_c", "fr14_c", "fr15_c",
    "dr0_c", "dr2_c", "dr4_c",  "dr6_c",  "dr8_c",  "dr10_c", "dr12_c", "dr14_c",
    "fv0_c", "fv4_c", "fv8_c",  "fv12_c",
    /* FIXME!!!! XF0 XF15, XD0 XD14 ?????*/
  };

  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

#define NUM_PSEUDO_REGS_SH_MEDIA 80
#define NUM_PSEUDO_REGS_SH_COMPACT 51

static const unsigned char *
sh_breakpoint_from_pc (CORE_ADDR *pcptr, int *lenptr)
{
  /* 0xc3c3 is trapa #c3, and it works in big and little endian modes */
  static unsigned char breakpoint[] =  {0xc3, 0xc3};
  
  *lenptr = sizeof (breakpoint);
  return breakpoint;
}

/* Macros and functions for setting and testing a bit in a minimal
   symbol that marks it as 32-bit function.  The MSB of the minimal
   symbol's "info" field is used for this purpose. This field is
   already being used to store the symbol size, so the assumption is
   that the symbol size cannot exceed 2^31.

   ELF_MAKE_MSYMBOL_SPECIAL
   tests whether an ELF symbol is "special", i.e. refers
   to a 32-bit function, and sets a "special" bit in a 
   minimal symbol to mark it as a 32-bit function
   MSYMBOL_IS_SPECIAL   tests the "special" bit in a minimal symbol
   MSYMBOL_SIZE         returns the size of the minimal symbol, i.e.
   the "info" field with the "special" bit masked out */

#define MSYMBOL_IS_SPECIAL(msym) \
  (((long) MSYMBOL_INFO (msym) & 0x80000000) != 0)

void
sh64_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  if (msym == NULL)
    return;

  if (((elf_symbol_type *)(sym))->internal_elf_sym.st_other == STO_SH5_ISA32)
    {
      MSYMBOL_INFO (msym) = (char *) (((long) MSYMBOL_INFO (msym)) | 0x80000000);
      SYMBOL_VALUE_ADDRESS (msym) |= 1;
    }
}

/* ISA32 (shmedia) function addresses are odd (bit 0 is set).  Here
   are some macros to test, set, or clear bit 0 of addresses.  */
#define IS_ISA32_ADDR(addr)	 ((addr) & 1)
#define MAKE_ISA32_ADDR(addr)	 ((addr) | 1)
#define UNMAKE_ISA32_ADDR(addr)  ((addr) & ~1)

static int
pc_is_isa32 (bfd_vma memaddr)
{
  struct minimal_symbol *sym;

  /* If bit 0 of the address is set, assume this is a
     ISA32 (shmedia) address. */
  if (IS_ISA32_ADDR (memaddr))
    return 1;

  /* A flag indicating that this is a ISA32 function is stored by elfread.c in
     the high bit of the info field.  Use this to decide if the function is
     ISA16 or ISA32.  */
  sym = lookup_minimal_symbol_by_pc (memaddr);
  if (sym)
    return MSYMBOL_IS_SPECIAL (sym);
  else
    return 0;
}

static const unsigned char *
sh_sh64_breakpoint_from_pc (CORE_ADDR *pcptr, int *lenptr)
{
  /* The BRK instruction for shmedia is 
     01101111 11110101 11111111 11110000
     which translates in big endian mode to 0x6f, 0xf5, 0xff, 0xf0
     and in little endian mode to 0xf0, 0xff, 0xf5, 0x6f */

  /* The BRK instruction for shcompact is
     00000000 00111011
     which translates in big endian mode to 0x0, 0x3b
     and in little endian mode to 0x3b, 0x0*/

  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    {
      if (pc_is_isa32 (*pcptr))
	{
	  static unsigned char big_breakpoint_media[] = {0x6f, 0xf5, 0xff, 0xf0};
	  *pcptr = UNMAKE_ISA32_ADDR (*pcptr);
	  *lenptr = sizeof (big_breakpoint_media);
	  return big_breakpoint_media;
	}
      else
	{
	  static unsigned char big_breakpoint_compact[] = {0x0, 0x3b};
	  *lenptr = sizeof (big_breakpoint_compact);
	  return big_breakpoint_compact;
	}
    }
  else
    {
      if (pc_is_isa32 (*pcptr))
	{
	  static unsigned char little_breakpoint_media[] = {0xf0, 0xff, 0xf5, 0x6f};
	  *pcptr = UNMAKE_ISA32_ADDR (*pcptr);
	  *lenptr = sizeof (little_breakpoint_media);
	  return little_breakpoint_media;
	}
      else
	{
	  static unsigned char little_breakpoint_compact[] = {0x3b, 0x0};
	  *lenptr = sizeof (little_breakpoint_compact);
	  return little_breakpoint_compact;
	}
    }
}

/* Prologue looks like
   [mov.l       <regs>,@-r15]...
   [sts.l       pr,@-r15]
   [mov.l       r14,@-r15]
   [mov         r15,r14]

   Actually it can be more complicated than this.  For instance, with
   newer gcc's:

   mov.l   r14,@-r15
   add     #-12,r15
   mov     r15,r14
   mov     r4,r1
   mov     r5,r2
   mov.l   r6,@(4,r14)
   mov.l   r7,@(8,r14)
   mov.b   r1,@r14
   mov     r14,r1
   mov     r14,r1
   add     #2,r1
   mov.w   r2,@r1

 */

/* PTABS/L Rn, TRa       0110101111110001nnnnnnl00aaa0000 
   with l=1 and n = 18   0110101111110001010010100aaa0000 */
#define IS_PTABSL_R18(x)  (((x) & 0xffffff8f) == 0x6bf14a00)

/* STS.L PR,@-r0   0100000000100010
   r0-4-->r0, PR-->(r0) */
#define IS_STS_R0(x)  		((x) == 0x4022)

/* STS PR, Rm      0000mmmm00101010
   PR-->Rm */
#define IS_STS_PR(x)            (((x) & 0xf0ff) == 0x2a)

/* MOV.L Rm,@(disp,r15)  00011111mmmmdddd
   Rm-->(dispx4+r15) */
#define IS_MOV_TO_R15(x)              (((x) & 0xff00) == 0x1f00)

/* MOV.L R14,@(disp,r15)  000111111110dddd
   R14-->(dispx4+r15) */
#define IS_MOV_R14(x)              (((x) & 0xfff0) == 0x1fe0)

/* ST.Q R14, disp, R18    101011001110dddddddddd0100100000
   R18-->(dispx8+R14) */
#define IS_STQ_R18_R14(x)          (((x) & 0xfff003ff) == 0xace00120)

/* ST.Q R15, disp, R18    101011001111dddddddddd0100100000
   R18-->(dispx8+R15) */
#define IS_STQ_R18_R15(x)          (((x) & 0xfff003ff) == 0xacf00120)

/* ST.L R15, disp, R18    101010001111dddddddddd0100100000
   R18-->(dispx4+R15) */
#define IS_STL_R18_R15(x)          (((x) & 0xfff003ff) == 0xa8f00120)

/* ST.Q R15, disp, R14    1010 1100 1111 dddd dddd dd00 1110 0000
   R14-->(dispx8+R15) */
#define IS_STQ_R14_R15(x)          (((x) & 0xfff003ff) == 0xacf000e0)

/* ST.L R15, disp, R14    1010 1000 1111 dddd dddd dd00 1110 0000
   R14-->(dispx4+R15) */
#define IS_STL_R14_R15(x)          (((x) & 0xfff003ff) == 0xa8f000e0)

/* ADDI.L R15,imm,R15     1101 0100 1111 ssss ssss ss00 1111 0000
   R15 + imm --> R15 */
#define IS_ADDIL_SP_MEDIA(x)         (((x) & 0xfff003ff) == 0xd4f000f0)

/* ADDI R15,imm,R15     1101 0000 1111 ssss ssss ss00 1111 0000
   R15 + imm --> R15 */
#define IS_ADDI_SP_MEDIA(x)         (((x) & 0xfff003ff) == 0xd0f000f0)

/* ADD.L R15,R63,R14    0000 0000 1111 1000 1111 1100 1110 0000 
   R15 + R63 --> R14 */
#define IS_ADDL_SP_FP_MEDIA(x)  	((x) == 0x00f8fce0)

/* ADD R15,R63,R14    0000 0000 1111 1001 1111 1100 1110 0000 
   R15 + R63 --> R14 */
#define IS_ADD_SP_FP_MEDIA(x)  	((x) == 0x00f9fce0)

#define IS_MOV_SP_FP_MEDIA(x)  	(IS_ADDL_SP_FP_MEDIA(x) || IS_ADD_SP_FP_MEDIA(x))

/* MOV #imm, R0    1110 0000 ssss ssss 
   #imm-->R0 */
#define IS_MOV_R0(x) 		(((x) & 0xff00) == 0xe000)

/* MOV.L @(disp,PC), R0    1101 0000 iiii iiii  */
#define IS_MOVL_R0(x) 		(((x) & 0xff00) == 0xd000)

/* ADD r15,r0      0011 0000 1111 1100
   r15+r0-->r0 */
#define IS_ADD_SP_R0(x)	        ((x) == 0x30fc)

/* MOV.L R14 @-R0  0010 0000 1110 0110
   R14-->(R0-4), R0-4-->R0 */
#define IS_MOV_R14_R0(x)        ((x) == 0x20e6)

/* ADD Rm,R63,Rn  Rm+R63-->Rn  0000 00mm mmmm 1001 1111 11nn nnnn 0000
   where Rm is one of r2-r9 which are the argument registers. */
/* FIXME: Recognize the float and double register moves too! */
#define IS_MEDIA_IND_ARG_MOV(x) \
((((x) & 0xfc0ffc0f) == 0x0009fc00) && (((x) & 0x03f00000) >= 0x00200000 && ((x) & 0x03f00000) <= 0x00900000))

/* ST.Q Rn,0,Rm  Rm-->Rn+0  1010 11nn nnnn 0000 0000 00mm mmmm 0000
   or ST.L Rn,0,Rm  Rm-->Rn+0  1010 10nn nnnn 0000 0000 00mm mmmm 0000
   where Rm is one of r2-r9 which are the argument registers. */
#define IS_MEDIA_ARG_MOV(x) \
(((((x) & 0xfc0ffc0f) == 0xac000000) || (((x) & 0xfc0ffc0f) == 0xa8000000)) \
   && (((x) & 0x000003f0) >= 0x00000020 && ((x) & 0x000003f0) <= 0x00000090))

/* ST.B R14,0,Rn     Rn-->(R14+0) 1010 0000 1110 0000 0000 00nn nnnn 0000*/
/* ST.W R14,0,Rn     Rn-->(R14+0) 1010 0100 1110 0000 0000 00nn nnnn 0000*/
/* ST.L R14,0,Rn     Rn-->(R14+0) 1010 1000 1110 0000 0000 00nn nnnn 0000*/
/* FST.S R14,0,FRn   Rn-->(R14+0) 1011 0100 1110 0000 0000 00nn nnnn 0000*/
/* FST.D R14,0,DRn   Rn-->(R14+0) 1011 1100 1110 0000 0000 00nn nnnn 0000*/
#define IS_MEDIA_MOV_TO_R14(x)  \
((((x) & 0xfffffc0f) == 0xa0e00000) \
|| (((x) & 0xfffffc0f) == 0xa4e00000) \
|| (((x) & 0xfffffc0f) == 0xa8e00000) \
|| (((x) & 0xfffffc0f) == 0xb4e00000) \
|| (((x) & 0xfffffc0f) == 0xbce00000))

/* MOV Rm, Rn  Rm-->Rn 0110 nnnn mmmm 0011
   where Rm is r2-r9 */
#define IS_COMPACT_IND_ARG_MOV(x) \
((((x) & 0xf00f) == 0x6003) && (((x) & 0x00f0) >= 0x0020) && (((x) & 0x00f0) <= 0x0090))

/* compact direct arg move! 
   MOV.L Rn, @r14     0010 1110 mmmm 0010 */
#define IS_COMPACT_ARG_MOV(x) \
(((((x) & 0xff0f) == 0x2e02) && (((x) & 0x00f0) >= 0x0020) && ((x) & 0x00f0) <= 0x0090))

/* MOV.B Rm, @R14     0010 1110 mmmm 0000 
   MOV.W Rm, @R14     0010 1110 mmmm 0001 */
#define IS_COMPACT_MOV_TO_R14(x) \
((((x) & 0xff0f) == 0x2e00) || (((x) & 0xff0f) == 0x2e01))

#define IS_JSR_R0(x)           ((x) == 0x400b)
#define IS_NOP(x)              ((x) == 0x0009)


/* STS.L PR,@-r15  0100111100100010
   r15-4-->r15, PR-->(r15) */
#define IS_STS(x)  		((x) == 0x4f22)

/* MOV.L Rm,@-r15  00101111mmmm0110
   r15-4-->r15, Rm-->(R15) */
#define IS_PUSH(x) 		(((x) & 0xff0f) == 0x2f06)

#define GET_PUSHED_REG(x)  	(((x) >> 4) & 0xf)

/* MOV r15,r14     0110111011110011
   r15-->r14  */
#define IS_MOV_SP_FP(x)  	((x) == 0x6ef3)

/* ADD #imm,r15    01111111iiiiiiii
   r15+imm-->r15 */
#define IS_ADD_SP(x) 		(((x) & 0xff00) == 0x7f00)

#define IS_MOV_R3(x) 		(((x) & 0xff00) == 0x1a00)
#define IS_SHLL_R3(x)		((x) == 0x4300)

/* ADD r3,r15      0011111100111100
   r15+r3-->r15 */
#define IS_ADD_R3SP(x)		((x) == 0x3f3c)

/* FMOV.S FRm,@-Rn  Rn-4-->Rn, FRm-->(Rn)     1111nnnnmmmm1011
   FMOV DRm,@-Rn    Rn-8-->Rn, DRm-->(Rn)     1111nnnnmmm01011
   FMOV XDm,@-Rn    Rn-8-->Rn, XDm-->(Rn)     1111nnnnmmm11011 */
#define IS_FMOV(x)		(((x) & 0xf00f) == 0xf00b)

/* MOV Rm,Rn            Rm-->Rn          0110nnnnmmmm0011 
   MOV.L Rm,@(disp,Rn)  Rm-->(dispx4+Rn) 0001nnnnmmmmdddd
   MOV.L Rm,@Rn         Rm-->(Rn)        0010nnnnmmmm0010
   where Rm is one of r4,r5,r6,r7 which are the argument registers. */
#define IS_ARG_MOV(x) \
(((((x) & 0xf00f) == 0x6003) && (((x) & 0x00f0) >= 0x0040 && ((x) & 0x00f0) <= 0x0070)) \
 || ((((x) & 0xf000) == 0x1000) && (((x) & 0x00f0) >= 0x0040 && ((x) & 0x00f0) <= 0x0070)) \
 || ((((x) & 0xf00f) == 0x2002) && (((x) & 0x00f0) >= 0x0040 && ((x) & 0x00f0) <= 0x0070)))

/* MOV.L Rm,@(disp,r14)  00011110mmmmdddd
   Rm-->(dispx4+r14) where Rm is one of r4,r5,r6,r7 */
#define IS_MOV_TO_R14(x) \
     ((((x) & 0xff00) == 0x1e) && (((x) & 0x00f0) >= 0x0040 && ((x) & 0x00f0) <= 0x0070))
                        
#define FPSCR_SZ		(1 << 20)

/* Skip any prologue before the guts of a function */

/* Skip the prologue using the debug information. If this fails we'll
   fall back on the 'guess' method below. */
static CORE_ADDR
after_prologue (CORE_ADDR pc)
{
  struct symtab_and_line sal;
  CORE_ADDR func_addr, func_end;

  /* If we can not find the symbol in the partial symbol table, then
     there is no hope we can determine the function's start address
     with this code.  */
  if (!find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    return 0;

  /* Get the line associated with FUNC_ADDR.  */
  sal = find_pc_line (func_addr, 0);

  /* There are only two cases to consider.  First, the end of the source line
     is within the function bounds.  In that case we return the end of the
     source line.  Second is the end of the source line extends beyond the
     bounds of the current function.  We need to use the slow code to
     examine instructions in that case.  */
  if (sal.end < func_end)
    return sal.end;
  else
    return 0;
}

/* Here we look at each instruction in the function, and try to guess
   where the prologue ends. Unfortunately this is not always 
   accurate. */
static CORE_ADDR
sh_skip_prologue_hard_way (CORE_ADDR start_pc)
{
  CORE_ADDR here, end;
  int updated_fp = 0;

  if (!start_pc)
    return 0;

  for (here = start_pc, end = start_pc + (2 * 28); here < end;)
    {
      int w = read_memory_integer (here, 2);
      here += 2;
      if (IS_FMOV (w) || IS_PUSH (w) || IS_STS (w) || IS_MOV_R3 (w)
	  || IS_ADD_R3SP (w) || IS_ADD_SP (w) || IS_SHLL_R3 (w) 
	  || IS_ARG_MOV (w) || IS_MOV_TO_R14 (w))
	{
	  start_pc = here;
	}
      else if (IS_MOV_SP_FP (w))
	{
	  start_pc = here;
	  updated_fp = 1;
	}
      else
	/* Don't bail out yet, if we are before the copy of sp. */
	if (updated_fp)
	  break;
    }

  return start_pc;
}

static CORE_ADDR 
look_for_args_moves (CORE_ADDR start_pc, int media_mode)
{
  CORE_ADDR here, end;
  int w;
  int insn_size = (media_mode ? 4 : 2);

  for (here = start_pc, end = start_pc + (insn_size * 28); here < end;)
    {
      if (media_mode)
	{
	  w = read_memory_integer (UNMAKE_ISA32_ADDR (here), insn_size);
	  here += insn_size;
	  if (IS_MEDIA_IND_ARG_MOV (w))
	    {
	      /* This must be followed by a store to r14, so the argument
		 is where the debug info says it is. This can happen after
		 the SP has been saved, unfortunately. */
	 
	      int next_insn = read_memory_integer (UNMAKE_ISA32_ADDR (here),
						   insn_size);
	      here += insn_size;
	      if (IS_MEDIA_MOV_TO_R14 (next_insn))
		start_pc = here;	  
	    }
	  else if (IS_MEDIA_ARG_MOV (w))
	    {
	      /* These instructions store directly the argument in r14. */
	      start_pc = here;
	    }
	  else
	    break;
	}
      else
	{
	  w = read_memory_integer (here, insn_size);
	  w = w & 0xffff;
	  here += insn_size;
	  if (IS_COMPACT_IND_ARG_MOV (w))
	    {
	      /* This must be followed by a store to r14, so the argument
		 is where the debug info says it is. This can happen after
		 the SP has been saved, unfortunately. */
	 
	      int next_insn = 0xffff & read_memory_integer (here, insn_size);
	      here += insn_size;
	      if (IS_COMPACT_MOV_TO_R14 (next_insn))
		start_pc = here;
	    }
	  else if (IS_COMPACT_ARG_MOV (w))
	    {
	      /* These instructions store directly the argument in r14. */
	      start_pc = here;
	    }
	  else if (IS_MOVL_R0 (w))
	    {
	      /* There is a function that gcc calls to get the arguments
		 passed correctly to the function. Only after this
		 function call the arguments will be found at the place
		 where they are supposed to be. This happens in case the
		 argument has to be stored into a 64-bit register (for
		 instance doubles, long longs).  SHcompact doesn't have
		 access to the full 64-bits, so we store the register in
		 stack slot and store the address of the stack slot in
		 the register, then do a call through a wrapper that
		 loads the memory value into the register.  A SHcompact
		 callee calls an argument decoder
		 (GCC_shcompact_incoming_args) that stores the 64-bit
		 value in a stack slot and stores the address of the
		 stack slot in the register.  GCC thinks the argument is
		 just passed by transparent reference, but this is only
		 true after the argument decoder is called. Such a call
		 needs to be considered part of the prologue. */

	      /* This must be followed by a JSR @r0 instruction and by
                 a NOP instruction. After these, the prologue is over!  */
	 
	      int next_insn = 0xffff & read_memory_integer (here, insn_size);
	      here += insn_size;
	      if (IS_JSR_R0 (next_insn))
		{
		  next_insn = 0xffff & read_memory_integer (here, insn_size);
		  here += insn_size;

		  if (IS_NOP (next_insn))
		    start_pc = here;
		}
	    }
	  else
	    break;
	}
    }

  return start_pc;
}

static CORE_ADDR
sh64_skip_prologue_hard_way (CORE_ADDR start_pc)
{
  CORE_ADDR here, end;
  int updated_fp = 0;
  int insn_size = 4;
  int media_mode = 1;

  if (!start_pc)
    return 0;

  if (pc_is_isa32 (start_pc) == 0)
    {
      insn_size = 2;
      media_mode = 0;
    }

  for (here = start_pc, end = start_pc + (insn_size * 28); here < end;)
    {

      if (media_mode)
	{
	  int w = read_memory_integer (UNMAKE_ISA32_ADDR (here), insn_size);
	  here += insn_size;
	  if (IS_STQ_R18_R14 (w) || IS_STQ_R18_R15 (w) || IS_STQ_R14_R15 (w)
	      || IS_STL_R14_R15 (w) || IS_STL_R18_R15 (w)
	      || IS_ADDIL_SP_MEDIA (w) || IS_ADDI_SP_MEDIA (w) || IS_PTABSL_R18 (w))
	    {
	      start_pc = here;
	    }
	  else if (IS_MOV_SP_FP (w) || IS_MOV_SP_FP_MEDIA(w))
	    {
	      start_pc = here;
	      updated_fp = 1;
	    }
	  else
	    if (updated_fp)
	      {
		/* Don't bail out yet, we may have arguments stored in
		   registers here, according to the debug info, so that
		   gdb can print the frames correctly. */
		start_pc = look_for_args_moves (here - insn_size, media_mode);
		break;
	      }
	}
      else
	{
	  int w = 0xffff & read_memory_integer (here, insn_size);
	  here += insn_size;

	  if (IS_STS_R0 (w) || IS_STS_PR (w)
	      || IS_MOV_TO_R15 (w) || IS_MOV_R14 (w) 
	      || IS_MOV_R0 (w) || IS_ADD_SP_R0 (w) || IS_MOV_R14_R0 (w))
	    {
	      start_pc = here;
	    }
	  else if (IS_MOV_SP_FP (w))
	    {
	      start_pc = here;
	      updated_fp = 1;
	    }
	  else
	    if (updated_fp)
	      {
		/* Don't bail out yet, we may have arguments stored in
		   registers here, according to the debug info, so that
		   gdb can print the frames correctly. */
		start_pc = look_for_args_moves (here - insn_size, media_mode);
		break;
	      }
	}
    }

  return start_pc;
}

static CORE_ADDR
sh_skip_prologue (CORE_ADDR pc)
{
  CORE_ADDR post_prologue_pc;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  post_prologue_pc = after_prologue (pc);

  /* If after_prologue returned a useful address, then use it.  Else
     fall back on the instruction skipping code. */
  if (post_prologue_pc != 0)
    return max (pc, post_prologue_pc);
  else
    return (skip_prologue_hard_way (pc));
}

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.

   The return address is the value saved in the PR register + 4  */
static CORE_ADDR
sh_saved_pc_after_call (struct frame_info *frame)
{
  return (ADDR_BITS_REMOVE (read_register (gdbarch_tdep (current_gdbarch)->PR_REGNUM)));
}

/* Should call_function allocate stack space for a struct return?  */
static int
sh_use_struct_convention (int gcc_p, struct type *type)
{
  return (TYPE_LENGTH (type) > 1);
}

static int
sh64_use_struct_convention (int gcc_p, struct type *type)
{
  return (TYPE_LENGTH (type) > 8);
}

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function.

   We store structs through a pointer passed in R2 */
static void
sh_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  write_register (STRUCT_RETURN_REGNUM, (addr));
}

/* Disassemble an instruction.  */
static int
gdb_print_insn_sh (bfd_vma memaddr, disassemble_info *info)
{
  info->endian = TARGET_BYTE_ORDER;
  return print_insn_sh (memaddr, info);
}

/* Given a GDB frame, determine the address of the calling function's frame.
   This will be used to create a new GDB frame struct, and then
   INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame.

   For us, the frame address is its stack pointer value, so we look up
   the function prologue to determine the caller's sp value, and return it.  */
static CORE_ADDR
sh_frame_chain (struct frame_info *frame)
{
  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    return frame->frame;	/* dummy frame same as caller's frame */
  if (frame->pc && !inside_entry_file (frame->pc))
    return read_memory_integer (FRAME_FP (frame) + frame->extra_info->f_offset, 4);
  else
    return 0;
}

/* Given a register number RN as it appears in an assembly
   instruction, find the corresponding register number in the GDB
   scheme. */
static int 
translate_insn_rn (int rn, int media_mode)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  /* FIXME: this assumes that the number rn is for a not pseudo
     register only. */
  if (media_mode)
    return rn;
  else
    {
      /* These registers don't have a corresponding compact one. */
      /* FIXME: This is probably not enough. */
#if 0
      if ((rn >= 16 && rn <= 63) || (rn >= 93 && rn <= 140))
	return rn;
#endif
      if (rn >= 0 && rn <= tdep->R0_C_REGNUM)
	return tdep->R0_C_REGNUM + rn;
      else
	return rn;
    }
}

static CORE_ADDR
sh64_frame_chain (struct frame_info *frame)
{
  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    return frame->frame;	/* dummy frame same as caller's frame */
  if (frame->pc && !inside_entry_file (frame->pc))
    {
      int media_mode = pc_is_isa32 (frame->pc);
      int size;
      if (gdbarch_tdep (current_gdbarch)->sh_abi == SH_ABI_32)
	size = 4;
      else
	size = REGISTER_RAW_SIZE (translate_insn_rn (FP_REGNUM, media_mode));
      return read_memory_integer (FRAME_FP (frame) + frame->extra_info->f_offset, size);
    }
  else
    return 0;
}

/* Find REGNUM on the stack.  Otherwise, it's in an active register.  One thing
   we might want to do here is to check REGNUM against the clobber mask, and
   somehow flag it as invalid if it isn't saved on the stack somewhere.  This
   would provide a graceful failure mode when trying to get the value of
   caller-saves registers for an inner frame.  */
static CORE_ADDR
sh_find_callers_reg (struct frame_info *fi, int regnum)
{
  for (; fi; fi = fi->next)
    if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
      /* When the caller requests PR from the dummy frame, we return PC because
         that's where the previous routine appears to have done a call from. */
      return generic_read_register_dummy (fi->pc, fi->frame, regnum);
    else
      {
	FRAME_INIT_SAVED_REGS (fi);
	if (!fi->pc)
	  return 0;
	if (fi->saved_regs[regnum] != 0)
	  return read_memory_integer (fi->saved_regs[regnum],
				      REGISTER_RAW_SIZE (regnum));
      }
  return read_register (regnum);
}

static CORE_ADDR
sh64_get_saved_pr (struct frame_info *fi, int pr_regnum)
{
  int media_mode = 0;

  for (; fi; fi = fi->next)
    if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
      /* When the caller requests PR from the dummy frame, we return PC because
         that's where the previous routine appears to have done a call from. */
      return generic_read_register_dummy (fi->pc, fi->frame, pr_regnum);
    else
      {
	FRAME_INIT_SAVED_REGS (fi);
	if (!fi->pc)
	  return 0;

	media_mode = pc_is_isa32 (fi->pc);

	if (fi->saved_regs[pr_regnum] != 0)
	  {
	    int gdb_reg_num = translate_insn_rn (pr_regnum, media_mode);
	    int size = ((gdbarch_tdep (current_gdbarch)->sh_abi == SH_ABI_32)
			? 4
			: REGISTER_RAW_SIZE (gdb_reg_num));
	    return read_memory_integer (fi->saved_regs[pr_regnum], size);
	  }
      }
  return read_register (pr_regnum);
}

/* Put here the code to store, into a struct frame_saved_regs, the
   addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special: the address we
   return for it IS the sp for the next frame. */
static void
sh_nofp_frame_init_saved_regs (struct frame_info *fi)
{
  int *where = (int *) alloca ((NUM_REGS + NUM_PSEUDO_REGS) * sizeof(int));
  int rn;
  int have_fp = 0;
  int depth;
  int pc;
  int opc;
  int insn;
  int r3_val = 0;
  char *dummy_regs = deprecated_generic_find_dummy_frame (fi->pc, fi->frame);
  
  if (fi->saved_regs == NULL)
    frame_saved_regs_zalloc (fi);
  else
    memset (fi->saved_regs, 0, SIZEOF_FRAME_SAVED_REGS);
  
  if (dummy_regs)
    {
      /* DANGER!  This is ONLY going to work if the char buffer format of
         the saved registers is byte-for-byte identical to the 
         CORE_ADDR regs[NUM_REGS] format used by struct frame_saved_regs! */
      memcpy (fi->saved_regs, dummy_regs, sizeof (fi->saved_regs));
      return;
    }

  fi->extra_info->leaf_function = 1;
  fi->extra_info->f_offset = 0;

  for (rn = 0; rn < NUM_REGS + NUM_PSEUDO_REGS; rn++)
    where[rn] = -1;

  depth = 0;

  /* Loop around examining the prologue insns until we find something
     that does not appear to be part of the prologue.  But give up
     after 20 of them, since we're getting silly then. */

  pc = get_pc_function_start (fi->pc);
  if (!pc)
    {
      fi->pc = 0;
      return;
    }

  for (opc = pc + (2 * 28); pc < opc; pc += 2)
    {
      insn = read_memory_integer (pc, 2);
      /* See where the registers will be saved to */
      if (IS_PUSH (insn))
	{
	  rn = GET_PUSHED_REG (insn);
	  where[rn] = depth;
	  depth += 4;
	}
      else if (IS_STS (insn))
	{
	  where[gdbarch_tdep (current_gdbarch)->PR_REGNUM] = depth;
	  /* If we're storing the pr then this isn't a leaf */
	  fi->extra_info->leaf_function = 0;
	  depth += 4;
	}
      else if (IS_MOV_R3 (insn))
	{
	  r3_val = ((insn & 0xff) ^ 0x80) - 0x80;
	}
      else if (IS_SHLL_R3 (insn))
	{
	  r3_val <<= 1;
	}
      else if (IS_ADD_R3SP (insn))
	{
	  depth += -r3_val;
	}
      else if (IS_ADD_SP (insn))
	{
	  depth -= ((insn & 0xff) ^ 0x80) - 0x80;
	}
      else if (IS_MOV_SP_FP (insn))
	break;
#if 0 /* This used to just stop when it found an instruction that
	 was not considered part of the prologue.  Now, we just
	 keep going looking for likely instructions. */
      else
	break;
#endif
    }

  /* Now we know how deep things are, we can work out their addresses */

  for (rn = 0; rn < NUM_REGS + NUM_PSEUDO_REGS; rn++)
    {
      if (where[rn] >= 0)
	{
	  if (rn == FP_REGNUM)
	    have_fp = 1;

	  fi->saved_regs[rn] = fi->frame - where[rn] + depth - 4;
	}
      else
	{
	  fi->saved_regs[rn] = 0;
	}
    }

  if (have_fp)
    {
      fi->saved_regs[SP_REGNUM] = read_memory_integer (fi->saved_regs[FP_REGNUM], 4);
    }
  else
    {
      fi->saved_regs[SP_REGNUM] = fi->frame - 4;
    }

  fi->extra_info->f_offset = depth - where[FP_REGNUM] - 4;
  /* Work out the return pc - either from the saved pr or the pr
     value */
}

/* For vectors of 4 floating point registers. */
static int
fv_reg_base_num (int fv_regnum)
{
  int fp_regnum;

  fp_regnum = FP0_REGNUM + 
    (fv_regnum - gdbarch_tdep (current_gdbarch)->FV0_REGNUM) * 4;
  return fp_regnum;
}

/* For double precision floating point registers, i.e 2 fp regs.*/
static int
dr_reg_base_num (int dr_regnum)
{
  int fp_regnum;

  fp_regnum = FP0_REGNUM + 
    (dr_regnum - gdbarch_tdep (current_gdbarch)->DR0_REGNUM) * 2;
  return fp_regnum;
}

/* For pairs of floating point registers */
static int
fpp_reg_base_num (int fpp_regnum)
{
  int fp_regnum;

  fp_regnum = FP0_REGNUM + 
    (fpp_regnum - gdbarch_tdep (current_gdbarch)->FPP0_REGNUM) * 2;
  return fp_regnum;
}

static int
is_media_pseudo (int rn)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  return (rn >= tdep->DR0_REGNUM 
	  && rn <= tdep->FV_LAST_REGNUM);
}

int
sh64_get_gdb_regnum (int gcc_regnum, CORE_ADDR pc)
{
  return translate_insn_rn (gcc_regnum, pc_is_isa32 (pc));
}

static int
sh64_media_reg_base_num (int reg_nr)
{
  int base_regnum = -1;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (reg_nr >= tdep->DR0_REGNUM
      && reg_nr <= tdep->DR_LAST_REGNUM)
    base_regnum = dr_reg_base_num (reg_nr);

  else if (reg_nr >= tdep->FPP0_REGNUM 
	   && reg_nr <= tdep->FPP_LAST_REGNUM)
    base_regnum = fpp_reg_base_num (reg_nr);

  else if (reg_nr >= tdep->FV0_REGNUM
	   && reg_nr <= tdep->FV_LAST_REGNUM)
    base_regnum = fv_reg_base_num (reg_nr);

  return base_regnum;
}

/* *INDENT-OFF* */
/*
    SH COMPACT MODE (ISA 16) (all pseudo) 221-272
       GDB_REGNUM  BASE_REGNUM
 r0_c       221      0
 r1_c       222      1
 r2_c       223      2
 r3_c       224      3
 r4_c       225      4
 r5_c       226      5
 r6_c       227      6
 r7_c       228      7
 r8_c       229      8
 r9_c       230      9
 r10_c      231      10
 r11_c      232      11
 r12_c      233      12
 r13_c      234      13
 r14_c      235      14
 r15_c      236      15

 pc_c       237      64
 gbr_c      238      16
 mach_c     239      17
 macl_c     240      17
 pr_c       241      18
 t_c        242      19
 fpscr_c    243      76
 fpul_c     244      109

 fr0_c      245      77
 fr1_c      246      78
 fr2_c      247      79
 fr3_c      248      80
 fr4_c      249      81
 fr5_c      250      82
 fr6_c      251      83
 fr7_c      252      84
 fr8_c      253      85
 fr9_c      254      86
 fr10_c     255      87
 fr11_c     256      88
 fr12_c     257      89
 fr13_c     258      90
 fr14_c     259      91
 fr15_c     260      92

 dr0_c      261      77
 dr2_c      262      79
 dr4_c      263      81
 dr6_c      264      83
 dr8_c      265      85
 dr10_c     266      87
 dr12_c     267      89
 dr14_c     268      91

 fv0_c      269      77
 fv4_c      270      81
 fv8_c      271      85
 fv12_c     272      91
*/
/* *INDENT-ON* */
static int
sh64_compact_reg_base_num (int reg_nr)
{
  int base_regnum = -1;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  /* general register N maps to general register N */
  if (reg_nr >= tdep->R0_C_REGNUM 
      && reg_nr <= tdep->R_LAST_C_REGNUM)
    base_regnum = reg_nr - tdep->R0_C_REGNUM;

  /* floating point register N maps to floating point register N */
  else if (reg_nr >= tdep->FP0_C_REGNUM 
	    && reg_nr <= tdep->FP_LAST_C_REGNUM)
    base_regnum = reg_nr - tdep->FP0_C_REGNUM + FP0_REGNUM;

  /* double prec register N maps to base regnum for double prec register N */
  else if (reg_nr >= tdep->DR0_C_REGNUM 
	    && reg_nr <= tdep->DR_LAST_C_REGNUM)
    base_regnum = dr_reg_base_num (tdep->DR0_REGNUM
				   + reg_nr - tdep->DR0_C_REGNUM);

  /* vector N maps to base regnum for vector register N */
  else if (reg_nr >= tdep->FV0_C_REGNUM 
	    && reg_nr <= tdep->FV_LAST_C_REGNUM)
    base_regnum = fv_reg_base_num (tdep->FV0_REGNUM
				   + reg_nr - tdep->FV0_C_REGNUM);

  else if (reg_nr == tdep->PC_C_REGNUM)
    base_regnum = PC_REGNUM;

  else if (reg_nr == tdep->GBR_C_REGNUM) 
    base_regnum = 16;

  else if (reg_nr == tdep->MACH_C_REGNUM
	   || reg_nr == tdep->MACL_C_REGNUM)
    base_regnum = 17;

  else if (reg_nr == tdep->PR_C_REGNUM) 
    base_regnum = 18;

  else if (reg_nr == tdep->T_C_REGNUM) 
    base_regnum = 19;

  else if (reg_nr == tdep->FPSCR_C_REGNUM) 
    base_regnum = tdep->FPSCR_REGNUM; /*???? this register is a mess. */

  else if (reg_nr == tdep->FPUL_C_REGNUM) 
    base_regnum = FP0_REGNUM + 32;
  
  return base_regnum;
}

/* Given a register number RN (according to the gdb scheme) , return
   its corresponding architectural register.  In media mode, only a
   subset of the registers is pseudo registers. For compact mode, all
   the registers are pseudo. */
static int 
translate_rn_to_arch_reg_num (int rn, int media_mode)
{

  if (media_mode)
    {
      if (!is_media_pseudo (rn))
	return rn;
      else
	return sh64_media_reg_base_num (rn);
    }
  else
    /* All compact registers are pseudo. */
    return sh64_compact_reg_base_num (rn);
}

static int
sign_extend (int value, int bits)
{
  value = value & ((1 << bits) - 1);
  return (value & (1 << (bits - 1))
	  ? value | (~((1 << bits) - 1))
	  : value);
}

static void
sh64_nofp_frame_init_saved_regs (struct frame_info *fi)
{
  int *where = (int *) alloca ((NUM_REGS + NUM_PSEUDO_REGS) * sizeof (int));
  int rn;
  int have_fp = 0;
  int fp_regnum;
  int sp_regnum;
  int depth;
  int pc;
  int opc;
  int insn;
  int r0_val = 0;
  int media_mode = 0;
  int insn_size;
  int gdb_register_number;
  int register_number;
  char *dummy_regs = deprecated_generic_find_dummy_frame (fi->pc, fi->frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 
  
  if (fi->saved_regs == NULL)
    frame_saved_regs_zalloc (fi);
  else
    memset (fi->saved_regs, 0, SIZEOF_FRAME_SAVED_REGS);
  
  if (dummy_regs)
    {
      /* DANGER!  This is ONLY going to work if the char buffer format of
         the saved registers is byte-for-byte identical to the 
         CORE_ADDR regs[NUM_REGS] format used by struct frame_saved_regs! */
      memcpy (fi->saved_regs, dummy_regs, sizeof (fi->saved_regs));
      return;
    }

  fi->extra_info->leaf_function = 1;
  fi->extra_info->f_offset = 0;

  for (rn = 0; rn < NUM_REGS + NUM_PSEUDO_REGS; rn++)
    where[rn] = -1;

  depth = 0;

  /* Loop around examining the prologue insns until we find something
     that does not appear to be part of the prologue.  But give up
     after 20 of them, since we're getting silly then. */

  pc = get_pc_function_start (fi->pc);
  if (!pc)
    {
      fi->pc = 0;
      return;
    }

  if (pc_is_isa32 (pc))
    {
      media_mode = 1;
      insn_size = 4;
    }
  else
    {
      media_mode = 0;
      insn_size = 2;
    }

 /* The frame pointer register is general register 14 in shmedia and
    shcompact modes. In sh compact it is a pseudo register.  Same goes
    for the stack pointer register, which is register 15. */
  fp_regnum = translate_insn_rn (FP_REGNUM, media_mode);
  sp_regnum = translate_insn_rn (SP_REGNUM, media_mode);

  for (opc = pc + (insn_size * 28); pc < opc; pc += insn_size)
    {
      insn = read_memory_integer (media_mode ? UNMAKE_ISA32_ADDR (pc) : pc,
				  insn_size);

      if (media_mode == 0)
	{
	  if (IS_STS_PR (insn))
	    {
	      int next_insn = read_memory_integer (pc + insn_size, insn_size);
	      if (IS_MOV_TO_R15 (next_insn))
		{
		  int reg_nr = tdep->PR_C_REGNUM;

		  where[reg_nr] = depth - ((((next_insn & 0xf) ^ 0x8) - 0x8) << 2);
		  fi->extra_info->leaf_function = 0;
		  pc += insn_size;
		}
	    }
	  else if (IS_MOV_R14 (insn))
	    {
	      where[fp_regnum] = depth - ((((insn & 0xf) ^ 0x8) - 0x8) << 2);
	    }

	  else if (IS_MOV_R0 (insn))
	    {
	      /* Put in R0 the offset from SP at which to store some
		 registers. We are interested in this value, because it
		 will tell us where the given registers are stored within
		 the frame.  */
	      r0_val = ((insn & 0xff) ^ 0x80) - 0x80;
	    }
	  else if (IS_ADD_SP_R0 (insn))
	    {
	      /* This instruction still prepares r0, but we don't care.
		 We already have the offset in r0_val. */
	    }
	  else if (IS_STS_R0 (insn))
	    {
	      /* Store PR at r0_val-4 from SP. Decrement r0 by 4*/
	      int reg_nr = tdep->PR_C_REGNUM;
	      where[reg_nr] = depth - (r0_val - 4);
	      r0_val -= 4;
	      fi->extra_info->leaf_function = 0;
	    }
	  else if (IS_MOV_R14_R0 (insn))
	    {
	      /* Store R14 at r0_val-4 from SP. Decrement r0 by 4 */
	      where[fp_regnum] = depth - (r0_val - 4);
	      r0_val -= 4;
	    }

	  else if (IS_ADD_SP (insn))
	    {
	      depth -= ((insn & 0xff) ^ 0x80) - 0x80;
	    }
	  else if (IS_MOV_SP_FP (insn))
	    break;
	}
      else
	{
	  if (IS_ADDIL_SP_MEDIA (insn) 
	      || IS_ADDI_SP_MEDIA (insn))
	    {
	      depth -= sign_extend ((((insn & 0xffc00) ^ 0x80000) - 0x80000) >> 10, 9);
	    }

	  else if (IS_STQ_R18_R15 (insn))
	    {
	      where[tdep->PR_REGNUM] = 
		depth - (sign_extend ((insn & 0xffc00) >> 10, 9) << 3);
	      fi->extra_info->leaf_function = 0;
	    }

	  else if (IS_STL_R18_R15 (insn))
	    {
	      where[tdep->PR_REGNUM] = 
		depth - (sign_extend ((insn & 0xffc00) >> 10, 9) << 2);
	      fi->extra_info->leaf_function = 0;
	    }

	  else if (IS_STQ_R14_R15 (insn))
	    {
	      where[fp_regnum] = depth - (sign_extend ((insn & 0xffc00) >> 10, 9) << 3);
	    }

	  else if (IS_STL_R14_R15 (insn))
	    {
	      where[fp_regnum] = depth - (sign_extend ((insn & 0xffc00) >> 10, 9) << 2);
	    }

	  else if (IS_MOV_SP_FP_MEDIA (insn))
	    break;
	}
    }

  /* Now we know how deep things are, we can work out their addresses. */
  for (rn = 0; rn < NUM_REGS + NUM_PSEUDO_REGS; rn++)
    {
      register_number = translate_rn_to_arch_reg_num (rn, media_mode);

      if (where[rn] >= 0)
	{
	  if (rn == fp_regnum)
	    have_fp = 1;

	  /* Watch out! saved_regs is only for the real registers, and
	     doesn't include space for the pseudo registers. */
	  fi->saved_regs[register_number]= fi->frame - where[rn] + depth; 
	    
	} 
      else 
	fi->saved_regs[register_number] = 0; 
    }

  if (have_fp)
    {
      /* SP_REGNUM is 15. For shmedia 15 is the real register. For
	 shcompact 15 is the arch register corresponding to the pseudo
	 register r15 which still is the SP register. */
      /* The place on the stack where fp is stored contains the sp of
         the caller. */
      /* Again, saved_registers contains only space for the real registers,
	 so we store in FP_REGNUM position. */
      int size;
      if (tdep->sh_abi == SH_ABI_32)
	size = 4;
      else
	size = REGISTER_RAW_SIZE (fp_regnum);
      fi->saved_regs[sp_regnum] = read_memory_integer (fi->saved_regs[fp_regnum], size);
    }
  else
    fi->saved_regs[sp_regnum] = fi->frame;

  fi->extra_info->f_offset = depth - where[fp_regnum]; 
}

static void
sh_fp_frame_init_saved_regs (struct frame_info *fi)
{
  int *where = (int *) alloca ((NUM_REGS + NUM_PSEUDO_REGS) * sizeof (int));
  int rn;
  int have_fp = 0;
  int depth;
  int pc;
  int opc;
  int insn;
  int r3_val = 0;
  char *dummy_regs = deprecated_generic_find_dummy_frame (fi->pc, fi->frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 
  
  if (fi->saved_regs == NULL)
    frame_saved_regs_zalloc (fi);
  else
    memset (fi->saved_regs, 0, SIZEOF_FRAME_SAVED_REGS);
  
  if (dummy_regs)
    {
      /* DANGER!  This is ONLY going to work if the char buffer format of
         the saved registers is byte-for-byte identical to the 
         CORE_ADDR regs[NUM_REGS] format used by struct frame_saved_regs! */
      memcpy (fi->saved_regs, dummy_regs, sizeof (fi->saved_regs));
      return;
    }

  fi->extra_info->leaf_function = 1;
  fi->extra_info->f_offset = 0;

  for (rn = 0; rn < NUM_REGS + NUM_PSEUDO_REGS; rn++)
    where[rn] = -1;

  depth = 0;

  /* Loop around examining the prologue insns until we find something
     that does not appear to be part of the prologue.  But give up
     after 20 of them, since we're getting silly then. */

  pc = get_pc_function_start (fi->pc);
  if (!pc)
    {
      fi->pc = 0;
      return;
    }

  for (opc = pc + (2 * 28); pc < opc; pc += 2)
    {
      insn = read_memory_integer (pc, 2);
      /* See where the registers will be saved to */
      if (IS_PUSH (insn))
	{
	  rn = GET_PUSHED_REG (insn);
	  where[rn] = depth;
	  depth += 4;
	}
      else if (IS_STS (insn))
	{
	  where[tdep->PR_REGNUM] = depth;
	  /* If we're storing the pr then this isn't a leaf */
	  fi->extra_info->leaf_function = 0;
	  depth += 4;
	}
      else if (IS_MOV_R3 (insn))
	{
	  r3_val = ((insn & 0xff) ^ 0x80) - 0x80;
	}
      else if (IS_SHLL_R3 (insn))
	{
	  r3_val <<= 1;
	}
      else if (IS_ADD_R3SP (insn))
	{
	  depth += -r3_val;
	}
      else if (IS_ADD_SP (insn))
	{
	  depth -= ((insn & 0xff) ^ 0x80) - 0x80;
	}
      else if (IS_FMOV (insn))
	{
	  if (read_register (tdep->FPSCR_REGNUM) & FPSCR_SZ)
	    {
	      depth += 8;
	    }
	  else
	    {
	      depth += 4;
	    }
	}
      else if (IS_MOV_SP_FP (insn))
	break;
#if 0 /* This used to just stop when it found an instruction that
	 was not considered part of the prologue.  Now, we just
	 keep going looking for likely instructions. */
      else
	break;
#endif
    }

  /* Now we know how deep things are, we can work out their addresses */

  for (rn = 0; rn < NUM_REGS + NUM_PSEUDO_REGS; rn++)
    {
      if (where[rn] >= 0)
	{
	  if (rn == FP_REGNUM)
	    have_fp = 1;

	  fi->saved_regs[rn] = fi->frame - where[rn] + depth - 4;
	}
      else
	{
	  fi->saved_regs[rn] = 0;
	}
    }

  if (have_fp)
    {
      fi->saved_regs[SP_REGNUM] =
	read_memory_integer (fi->saved_regs[FP_REGNUM], 4);
    }
  else
    {
      fi->saved_regs[SP_REGNUM] = fi->frame - 4;
    }

  fi->extra_info->f_offset = depth - where[FP_REGNUM] - 4;
  /* Work out the return pc - either from the saved pr or the pr
     value */
}

/* Initialize the extra info saved in a FRAME */
static void
sh_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{

  fi->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));

  if (fi->next)
    fi->pc = FRAME_SAVED_PC (fi->next);

  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    {
      /* We need to setup fi->frame here because run_stack_dummy gets it wrong
         by assuming it's always FP.  */
      fi->frame = generic_read_register_dummy (fi->pc, fi->frame,
					       SP_REGNUM);
      fi->extra_info->return_pc = generic_read_register_dummy (fi->pc,
							       fi->frame,
							       PC_REGNUM);
      fi->extra_info->f_offset = -(CALL_DUMMY_LENGTH + 4);
      fi->extra_info->leaf_function = 0;
      return;
    }
  else
    {
      FRAME_INIT_SAVED_REGS (fi);
      fi->extra_info->return_pc = 
	sh_find_callers_reg (fi, gdbarch_tdep (current_gdbarch)->PR_REGNUM);
    }
}

static void
sh64_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{
  int media_mode = pc_is_isa32 (fi->pc);

  fi->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));

  if (fi->next)
    fi->pc = FRAME_SAVED_PC (fi->next);

  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    {
      /* We need to setup fi->frame here because run_stack_dummy gets it wrong
         by assuming it's always FP.  */
      fi->frame = generic_read_register_dummy (fi->pc, fi->frame,
					       SP_REGNUM);
      fi->extra_info->return_pc = 
	generic_read_register_dummy (fi->pc, fi->frame, PC_REGNUM);
      fi->extra_info->f_offset = -(CALL_DUMMY_LENGTH + 4);
      fi->extra_info->leaf_function = 0;
      return;
    }
  else
    {
      FRAME_INIT_SAVED_REGS (fi);
      fi->extra_info->return_pc =
	sh64_get_saved_pr (fi, gdbarch_tdep (current_gdbarch)->PR_REGNUM);
    }
}

void
sh64_get_saved_register (char *raw_buffer, int *optimized, CORE_ADDR *addrp,
			 struct frame_info *frame, int regnum,
			 enum lval_type *lval)
{
  int media_mode;
  int live_regnum = regnum;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (!target_has_registers)
    error ("No registers.");

  /* Normal systems don't optimize out things with register numbers.  */
  if (optimized != NULL)
    *optimized = 0;

  if (addrp)			/* default assumption: not found in memory */
    *addrp = 0;

  if (raw_buffer)
    memset (raw_buffer, 0, sizeof (raw_buffer));

  /* We must do this here, before the following while loop changes
     frame, and makes it NULL. If this is a media register number,
     but we are in compact mode, it will become the corresponding 
     compact pseudo register. If there is no corresponding compact 
     pseudo-register what do we do?*/
  media_mode = pc_is_isa32 (frame->pc);
  live_regnum = translate_insn_rn (regnum, media_mode);

  /* Note: since the current frame's registers could only have been
     saved by frames INTERIOR TO the current frame, we skip examining
     the current frame itself: otherwise, we would be getting the
     previous frame's registers which were saved by the current frame.  */

  while (frame && ((frame = frame->next) != NULL))
    {
      if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
	{
	  if (lval)		/* found it in a CALL_DUMMY frame */
	    *lval = not_lval;
	  if (raw_buffer)
	    memcpy (raw_buffer,
		    (deprecated_generic_find_dummy_frame (frame->pc, frame->frame)
		     + REGISTER_BYTE (regnum)),
		    REGISTER_RAW_SIZE (regnum));
	  return;
	}

      FRAME_INIT_SAVED_REGS (frame);
      if (frame->saved_regs != NULL
	  && frame->saved_regs[regnum] != 0)
	{
	  if (lval)		/* found it saved on the stack */
	    *lval = lval_memory;
	  if (regnum == SP_REGNUM)
	    {
	      if (raw_buffer)	/* SP register treated specially */
		store_address (raw_buffer, REGISTER_RAW_SIZE (regnum),
			       frame->saved_regs[regnum]);
	    }
	  else
	    { /* any other register */
	      
	      if (addrp)
		*addrp = frame->saved_regs[regnum];
	      if (raw_buffer)
		{
		  int size;
		  if (tdep->sh_abi == SH_ABI_32
		      && (live_regnum == FP_REGNUM
			  || live_regnum == tdep->PR_REGNUM))
		    size = 4;
		  else
		    size = REGISTER_RAW_SIZE (live_regnum);
		  if (TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
		    read_memory (frame->saved_regs[regnum], raw_buffer, size);
		  else
		    read_memory (frame->saved_regs[regnum],
				 raw_buffer
				 + REGISTER_RAW_SIZE (live_regnum)
				 - size,
				 size);
		}
	    }
	  return;
	}
    }

  /* If we get thru the loop to this point, it means the register was
     not saved in any frame.  Return the actual live-register value.  */

  if (lval)			/* found it in a live register */
    *lval = lval_register;
  if (addrp)
    *addrp = REGISTER_BYTE (live_regnum);
  if (raw_buffer)
    read_register_gen (live_regnum, raw_buffer);
}

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */
static CORE_ADDR
sh_extract_struct_value_address (char *regbuf)
{
  return (extract_address ((regbuf), REGISTER_RAW_SIZE (0)));
}

static CORE_ADDR
sh64_extract_struct_value_address (char *regbuf)
{
  return (extract_address ((regbuf + REGISTER_BYTE (STRUCT_RETURN_REGNUM)), 
			   REGISTER_RAW_SIZE (STRUCT_RETURN_REGNUM)));
}

static CORE_ADDR
sh_frame_saved_pc (struct frame_info *frame)
{
  return ((frame)->extra_info->return_pc);
}

/* Discard from the stack the innermost frame,
   restoring all saved registers.  */
static void
sh_pop_frame (void)
{
  register struct frame_info *frame = get_current_frame ();
  register CORE_ADDR fp;
  register int regnum;

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    generic_pop_dummy_frame ();
  else
    {
      fp = FRAME_FP (frame);
      FRAME_INIT_SAVED_REGS (frame);

      /* Copy regs from where they were saved in the frame */
      for (regnum = 0; regnum < NUM_REGS + NUM_PSEUDO_REGS; regnum++)
	if (frame->saved_regs[regnum])
	  write_register (regnum,
			  read_memory_integer (frame->saved_regs[regnum], 4));

      write_register (PC_REGNUM, frame->extra_info->return_pc);
      write_register (SP_REGNUM, fp + 4);
    }
  flush_cached_frames ();
}

/* Used in the 'return' command. */
static void
sh64_pop_frame (void)
{
  register struct frame_info *frame = get_current_frame ();
  register CORE_ADDR fp;
  register int regnum;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  int media_mode = pc_is_isa32 (frame->pc);

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    generic_pop_dummy_frame ();
  else
    {
      fp = FRAME_FP (frame);
      FRAME_INIT_SAVED_REGS (frame);

      /* Copy regs from where they were saved in the frame */
      for (regnum = 0; regnum < NUM_REGS + NUM_PSEUDO_REGS; regnum++)
	if (frame->saved_regs[regnum])
	  {
	    int size;
	    if (tdep->sh_abi == SH_ABI_32
		&& (regnum == FP_REGNUM
		    || regnum ==  tdep->PR_REGNUM))
	      size = 4;
	    else
	      size = REGISTER_RAW_SIZE (translate_insn_rn (regnum,
							   media_mode));
	    write_register (regnum,
			    read_memory_integer (frame->saved_regs[regnum],
						 size));
	  }

      write_register (PC_REGNUM, frame->extra_info->return_pc);
      write_register (SP_REGNUM, fp + 8);
    }
  flush_cached_frames ();
}

/* Function: push_arguments
   Setup the function arguments for calling a function in the inferior.

   On the Hitachi SH architecture, there are four registers (R4 to R7)
   which are dedicated for passing function arguments.  Up to the first
   four arguments (depending on size) may go into these registers.
   The rest go on the stack.

   Arguments that are smaller than 4 bytes will still take up a whole
   register or a whole 32-bit word on the stack, and will be 
   right-justified in the register or the stack word.  This includes
   chars, shorts, and small aggregate types.

   Arguments that are larger than 4 bytes may be split between two or 
   more registers.  If there are not enough registers free, an argument
   may be passed partly in a register (or registers), and partly on the
   stack.  This includes doubles, long longs, and larger aggregates. 
   As far as I know, there is no upper limit to the size of aggregates 
   that will be passed in this way; in other words, the convention of 
   passing a pointer to a large aggregate instead of a copy is not used.

   An exceptional case exists for struct arguments (and possibly other
   aggregates such as arrays) if the size is larger than 4 bytes but 
   not a multiple of 4 bytes.  In this case the argument is never split 
   between the registers and the stack, but instead is copied in its
   entirety onto the stack, AND also copied into as many registers as 
   there is room for.  In other words, space in registers permitting, 
   two copies of the same argument are passed in.  As far as I can tell,
   only the one on the stack is used, although that may be a function 
   of the level of compiler optimization.  I suspect this is a compiler
   bug.  Arguments of these odd sizes are left-justified within the 
   word (as opposed to arguments smaller than 4 bytes, which are 
   right-justified).

   If the function is to return an aggregate type such as a struct, it 
   is either returned in the normal return value register R0 (if its 
   size is no greater than one byte), or else the caller must allocate
   space into which the callee will copy the return value (if the size
   is greater than one byte).  In this case, a pointer to the return 
   value location is passed into the callee in register R2, which does 
   not displace any of the other arguments passed in via registers R4
   to R7.   */

static CORE_ADDR
sh_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
	  	   int struct_return, CORE_ADDR struct_addr)
{
  int stack_offset, stack_alloc;
  int argreg;
  int argnum;
  struct type *type;
  CORE_ADDR regval;
  char *val;
  char valbuf[4];
  int len;
  int odd_sized_struct;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  /* first force sp to a 4-byte alignment */
  sp = sp & ~3;

  /* The "struct return pointer" pseudo-argument has its own dedicated 
     register */
  if (struct_return)
    write_register (STRUCT_RETURN_REGNUM, struct_addr);

  /* Now make sure there's space on the stack */
  for (argnum = 0, stack_alloc = 0; argnum < nargs; argnum++)
    stack_alloc += ((TYPE_LENGTH (VALUE_TYPE (args[argnum])) + 3) & ~3);
  sp -= stack_alloc;		/* make room on stack for args */

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  There are 16 bytes
     in four registers available.  Loop thru args from first to last.  */

  argreg = tdep->ARG0_REGNUM;
  for (argnum = 0, stack_offset = 0; argnum < nargs; argnum++)
    {
      type = VALUE_TYPE (args[argnum]);
      len = TYPE_LENGTH (type);
      memset (valbuf, 0, sizeof (valbuf));
      if (len < 4)
	{
	  /* value gets right-justified in the register or stack word */
	  memcpy (valbuf + (4 - len),
		  (char *) VALUE_CONTENTS (args[argnum]), len);
	  val = valbuf;
	}
      else
	val = (char *) VALUE_CONTENTS (args[argnum]);

      if (len > 4 && (len & 3) != 0)
	odd_sized_struct = 1;	/* such structs go entirely on stack */
      else
	odd_sized_struct = 0;
      while (len > 0)
	{
	  if (argreg > tdep->ARGLAST_REGNUM
	      || odd_sized_struct)
	    {			
	      /* must go on the stack */
	      write_memory (sp + stack_offset, val, 4);
	      stack_offset += 4;
	    }
	  /* NOTE WELL!!!!!  This is not an "else if" clause!!!
	     That's because some *&^%$ things get passed on the stack
	     AND in the registers!   */
	  if (argreg <= tdep->ARGLAST_REGNUM)
	    {			
	      /* there's room in a register */
	      regval = extract_address (val, REGISTER_RAW_SIZE (argreg));
	      write_register (argreg++, regval);
	    }
	  /* Store the value 4 bytes at a time.  This means that things
	     larger than 4 bytes may go partly in registers and partly
	     on the stack.  */
	  len -= REGISTER_RAW_SIZE (argreg);
	  val += REGISTER_RAW_SIZE (argreg);
	}
    }
  return sp;
}

/* R2-R9 for integer types and integer equivalent (char, pointers) and
   non-scalar (struct, union) elements (even if the elements are
   floats).  
   FR0-FR11 for single precision floating point (float)
   DR0-DR10 for double precision floating point (double) 
   
   If a float is argument number 3 (for instance) and arguments number
   1,2, and 4 are integer, the mapping will be:
   arg1 -->R2, arg2 --> R3, arg3 -->FR0, arg4 --> R5. I.e. R4 is not used.
   
   If a float is argument number 10 (for instance) and arguments number
   1 through 10 are integer, the mapping will be:
   arg1->R2, arg2->R3, arg3->R4, arg4->R5, arg5->R6, arg6->R7, arg7->R8,
   arg8->R9, arg9->(0,SP)stack(8-byte aligned), arg10->FR0, arg11->stack(16,SP).
   I.e. there is hole in the stack.

   Different rules apply for variable arguments functions, and for functions
   for which the prototype is not known. */

static CORE_ADDR
sh64_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
		     int struct_return, CORE_ADDR struct_addr)
{
  int stack_offset, stack_alloc;
  int int_argreg;
  int float_argreg;
  int double_argreg;
  int float_arg_index = 0;
  int double_arg_index = 0;
  int argnum;
  struct type *type;
  CORE_ADDR regval;
  char *val;
  char valbuf[8];
  char valbuf_tmp[8];
  int len;
  int argreg_size;
  int fp_args[12];
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  memset (fp_args, 0, sizeof (fp_args));

  /* first force sp to a 8-byte alignment */
  sp = sp & ~7;

  /* The "struct return pointer" pseudo-argument has its own dedicated 
     register */

  if (struct_return)
    write_register (STRUCT_RETURN_REGNUM, struct_addr);

  /* Now make sure there's space on the stack */
  for (argnum = 0, stack_alloc = 0; argnum < nargs; argnum++)
    stack_alloc += ((TYPE_LENGTH (VALUE_TYPE (args[argnum])) + 7) & ~7);
  sp -= stack_alloc;		/* make room on stack for args */

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  There are 64 bytes
     in eight registers available.  Loop thru args from first to last.  */

  int_argreg = tdep->ARG0_REGNUM;
  float_argreg = FP0_REGNUM;
  double_argreg = tdep->DR0_REGNUM;

  for (argnum = 0, stack_offset = 0; argnum < nargs; argnum++)
    {
      type = VALUE_TYPE (args[argnum]);
      len = TYPE_LENGTH (type);
      memset (valbuf, 0, sizeof (valbuf));
      
      if (TYPE_CODE (type) != TYPE_CODE_FLT)
	{
	  argreg_size = REGISTER_RAW_SIZE (int_argreg);

	  if (len < argreg_size)
	    {
	      /* value gets right-justified in the register or stack word */
	      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
		memcpy (valbuf + argreg_size - len,
			(char *) VALUE_CONTENTS (args[argnum]), len);
	      else
		memcpy (valbuf, (char *) VALUE_CONTENTS (args[argnum]), len);

	      val = valbuf;
	    }
	  else
	    val = (char *) VALUE_CONTENTS (args[argnum]);

	  while (len > 0)
	    {
	      if (int_argreg > tdep->ARGLAST_REGNUM)
		{			
		  /* must go on the stack */
		  write_memory (sp + stack_offset, val, argreg_size);
		  stack_offset += 8;/*argreg_size;*/
		}
	      /* NOTE WELL!!!!!  This is not an "else if" clause!!!
		 That's because some *&^%$ things get passed on the stack
		 AND in the registers!   */
	      if (int_argreg <= tdep->ARGLAST_REGNUM)
		{			
		  /* there's room in a register */
		  regval = extract_address (val, argreg_size);
		  write_register (int_argreg, regval);
		}
	      /* Store the value 8 bytes at a time.  This means that
		 things larger than 8 bytes may go partly in registers
		 and partly on the stack. FIXME: argreg is incremented
		 before we use its size. */
	      len -= argreg_size;
	      val += argreg_size;
	      int_argreg++;
	    }
	}
      else
	{
	  val = (char *) VALUE_CONTENTS (args[argnum]);
	  if (len == 4)
	    {
	      /* Where is it going to be stored? */
	      while (fp_args[float_arg_index])
		float_arg_index ++;

	      /* Now float_argreg points to the register where it
		 should be stored.  Are we still within the allowed
		 register set? */
	      if (float_arg_index <= tdep->FLOAT_ARGLAST_REGNUM)
		{
		  /* Goes in FR0...FR11 */
		  write_register_gen (FP0_REGNUM + float_arg_index, val);
		  fp_args[float_arg_index] = 1;
		  /* Skip the corresponding general argument register. */
		  int_argreg ++;
		}
	      else 
		;
		/* Store it as the integers, 8 bytes at the time, if
		   necessary spilling on the stack. */
	      
	    }
	    else if (len == 8)
	      {
		/* Where is it going to be stored? */
		while (fp_args[double_arg_index])
		  double_arg_index += 2;
		/* Now double_argreg points to the register
		   where it should be stored.
		   Are we still within the allowed register set? */
		if (double_arg_index < tdep->FLOAT_ARGLAST_REGNUM)
		  {
		    /* Goes in DR0...DR10 */
		    /* The numbering of the DRi registers is consecutive,
		       i.e. includes odd numbers. */
		    int double_register_offset = double_arg_index / 2;
		    int regnum = tdep->DR0_REGNUM +
		                 double_register_offset;
#if 0
		    if (TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
		      {
			memset (valbuf_tmp, 0, sizeof (valbuf_tmp));
			REGISTER_CONVERT_TO_VIRTUAL (regnum,
						     type, val, valbuf_tmp);
			val = valbuf_tmp;
		      }
#endif
		    /* Note: must use write_register_gen here instead
		       of regcache_raw_write, because
		       regcache_raw_write works only for real
		       registers, not pseudo.  write_register_gen will
		       call the gdbarch function to do register
		       writes, and that will properly know how to deal
		       with pseudoregs. */
		    write_register_gen (regnum, val);
		    fp_args[double_arg_index] = 1;
		    fp_args[double_arg_index + 1] = 1;
		    /* Skip the corresponding general argument register. */
		    int_argreg ++;
		  }
		else
		  ;
		  /* Store it as the integers, 8 bytes at the time, if
                     necessary spilling on the stack. */
	      }
	}
    }
  return sp;
}

/* Function: push_return_address (pc)
   Set up the return address for the inferior function call.
   Needed for targets where we don't actually execute a JSR/BSR instruction */

static CORE_ADDR
sh_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  write_register (gdbarch_tdep (current_gdbarch)->PR_REGNUM,
		  CALL_DUMMY_ADDRESS ());
  return sp;
}

/* Function: fix_call_dummy
   Poke the callee function's address into the destination part of 
   the CALL_DUMMY.  The address is actually stored in a data word 
   following the actualy CALL_DUMMY instructions, which will load
   it into a register using PC-relative addressing.  This function
   expects the CALL_DUMMY to look like this:

   mov.w @(2,PC), R8
   jsr   @R8
   nop
   trap
   <destination>
 */

#if 0
void
sh_fix_call_dummy (char *dummy, CORE_ADDR pc, CORE_ADDR fun, int nargs,
		   struct value **args, struct type *type, int gcc_p)
{
  *(unsigned long *) (dummy + 8) = fun;
}
#endif

static int
sh_coerce_float_to_double (struct type *formal, struct type *actual)
{
  return 1;
}

/* Find a function's return value in the appropriate registers (in
   regbuf), and copy it into valbuf.  Extract from an array REGBUF
   containing the (raw) register state a function return value of type
   TYPE, and copy that, in virtual format, into VALBUF.  */
static void
sh_extract_return_value (struct type *type, char *regbuf, char *valbuf)
{
  int len = TYPE_LENGTH (type);
  int return_register = R0_REGNUM;
  int offset;
  
  if (len <= 4)
    {
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	offset = REGISTER_BYTE (return_register) + 4 - len;
      else
	offset = REGISTER_BYTE (return_register);
      memcpy (valbuf, regbuf + offset, len);
    }
  else if (len <= 8)
    {
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	offset = REGISTER_BYTE (return_register) + 8 - len;
      else
	offset = REGISTER_BYTE (return_register);
      memcpy (valbuf, regbuf + offset, len);
    }
  else
    error ("bad size for return value");
}

static void
sh3e_sh4_extract_return_value (struct type *type, char *regbuf, char *valbuf)
{
  int return_register;
  int offset;
  int len = TYPE_LENGTH (type);

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    return_register = FP0_REGNUM;
  else
    return_register = R0_REGNUM;
  
  if (len == 8 && TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      DOUBLEST val;
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
	floatformat_to_doublest (&floatformat_ieee_double_littlebyte_bigword,
				 (char *) regbuf + REGISTER_BYTE (return_register),
				 &val);
      else
	floatformat_to_doublest (&floatformat_ieee_double_big,
				 (char *) regbuf + REGISTER_BYTE (return_register),
				 &val);
      store_floating (valbuf, len, val);
    }
  else if (len <= 4)
    {
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	offset = REGISTER_BYTE (return_register) + 4 - len;
      else
	offset = REGISTER_BYTE (return_register);
      memcpy (valbuf, regbuf + offset, len);
    }
  else if (len <= 8)
    {
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	offset = REGISTER_BYTE (return_register) + 8 - len;
      else
	offset = REGISTER_BYTE (return_register);
      memcpy (valbuf, regbuf + offset, len);
    }
  else
    error ("bad size for return value");
}

static void
sh64_extract_return_value (struct type *type, char *regbuf, char *valbuf)
{
  int offset;
  int return_register;
  int len = TYPE_LENGTH (type);
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 
  
  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      if (len == 4)
	{
	  /* Return value stored in FP0_REGNUM */
	  return_register = FP0_REGNUM;
	  offset = REGISTER_BYTE (return_register);
	  memcpy (valbuf, (char *) regbuf + offset, len); 
	}
      else if (len == 8)
	{
	  /* return value stored in DR0_REGNUM */
	  DOUBLEST val;

	  return_register = tdep->DR0_REGNUM;
	  offset = REGISTER_BYTE (return_register);
	  
	  if (TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
	    floatformat_to_doublest (&floatformat_ieee_double_littlebyte_bigword,
				     (char *) regbuf + offset, &val);
	  else
	    floatformat_to_doublest (&floatformat_ieee_double_big,
				     (char *) regbuf + offset, &val);
	  store_floating (valbuf, len, val);
	}
    }
  else
    { 
      if (len <= 8)
	{
	  /* Result is in register 2. If smaller than 8 bytes, it is padded 
	     at the most significant end. */
	  return_register = tdep->RETURN_REGNUM;
	  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	    offset = REGISTER_BYTE (return_register) +
	      REGISTER_RAW_SIZE (return_register) - len;
	  else
	    offset = REGISTER_BYTE (return_register);
	  memcpy (valbuf, (char *) regbuf + offset, len);
	}
      else
	error ("bad size for return value");
    }
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.
   If the architecture is sh4 or sh3e, store a function's return value
   in the R0 general register or in the FP0 floating point register,
   depending on the type of the return value. In all the other cases
   the result is stored in r0, left-justified. */
static void
sh_default_store_return_value (struct type *type, char *valbuf)
{
  char buf[32];	/* more than enough... */

  if (TYPE_LENGTH (type) < REGISTER_RAW_SIZE (R0_REGNUM))
    {
      /* Add leading zeros to the value. */
      memset (buf, 0, REGISTER_RAW_SIZE (R0_REGNUM));
      memcpy (buf + REGISTER_RAW_SIZE (R0_REGNUM) - TYPE_LENGTH (type),
	      valbuf, TYPE_LENGTH (type));
      write_register_bytes (REGISTER_BYTE (R0_REGNUM), buf, 
			    REGISTER_RAW_SIZE (R0_REGNUM));
    }
  else
    write_register_bytes (REGISTER_BYTE (R0_REGNUM), valbuf, 
			  TYPE_LENGTH (type));
}

static void
sh3e_sh4_store_return_value (struct type *type, char *valbuf)
{
  if (TYPE_CODE (type) == TYPE_CODE_FLT) 
    write_register_bytes (REGISTER_BYTE (FP0_REGNUM), 
			  valbuf, TYPE_LENGTH (type));
  else
    sh_default_store_return_value (type, valbuf);
}

static void
sh64_store_return_value (struct type *type, char *valbuf)
{
  char buf[64];	/* more than enough... */
  int len = TYPE_LENGTH (type);

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      if (len == 4)
	{
	  /* Return value stored in FP0_REGNUM */
	  write_register_gen (FP0_REGNUM, valbuf);
	}
      if (len == 8)
	{
	  /* return value stored in DR0_REGNUM */
	  /* FIXME: Implement */
	}
    }
  else
    {
      int return_register = gdbarch_tdep (current_gdbarch)->RETURN_REGNUM;
      int offset = 0;

      if (len <= REGISTER_RAW_SIZE (return_register))
	{
	  /* Pad with zeros. */
	  memset (buf, 0, REGISTER_RAW_SIZE (return_register));
	  if (TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
	    offset = 0; /*REGISTER_RAW_SIZE (return_register) - len;*/
	  else
	    offset = REGISTER_RAW_SIZE (return_register) - len;

	  memcpy (buf + offset, valbuf, len);
	  write_register_gen (return_register, buf);
	}
      else
	write_register_gen (return_register, valbuf);
    }
}

/* Print the registers in a form similar to the E7000 */

static void
sh_generic_show_regs (void)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s SR=%08lx PR=%08lx MACH=%08lx MACHL=%08lx\n",
		   paddr (read_register (PC_REGNUM)),
		   (long) read_register (tdep->SR_REGNUM),
		   (long) read_register (tdep->PR_REGNUM),
		   (long) read_register (MACH_REGNUM),
		   (long) read_register (MACL_REGNUM));

  printf_filtered ("GBR=%08lx VBR=%08lx",
		   (long) read_register (GBR_REGNUM),
		   (long) read_register (VBR_REGNUM));

  printf_filtered ("\nR0-R7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));
}

static void
sh3_show_regs (void)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s SR=%08lx PR=%08lx MACH=%08lx MACHL=%08lx\n",
		   paddr (read_register (PC_REGNUM)),
		   (long) read_register (tdep->SR_REGNUM),
		   (long) read_register (tdep->PR_REGNUM),
		   (long) read_register (MACH_REGNUM),
		   (long) read_register (MACL_REGNUM));

  printf_filtered ("GBR=%08lx VBR=%08lx",
		   (long) read_register (GBR_REGNUM),
		   (long) read_register (VBR_REGNUM));
  printf_filtered (" SSR=%08lx SPC=%08lx",
	           (long) read_register (tdep->SSR_REGNUM),
		   (long) read_register (tdep->SPC_REGNUM));

  printf_filtered ("\nR0-R7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));
}


static void
sh3e_show_regs (void)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s SR=%08lx PR=%08lx MACH=%08lx MACHL=%08lx\n",
		   paddr (read_register (PC_REGNUM)),
		   (long) read_register (tdep->SR_REGNUM),
		   (long) read_register (tdep->PR_REGNUM),
		   (long) read_register (MACH_REGNUM),
		   (long) read_register (MACL_REGNUM));

  printf_filtered ("GBR=%08lx VBR=%08lx",
		   (long) read_register (GBR_REGNUM),
		   (long) read_register (VBR_REGNUM));
  printf_filtered (" SSR=%08lx SPC=%08lx",
		   (long) read_register (tdep->SSR_REGNUM),
		   (long) read_register (tdep->SPC_REGNUM));
  printf_filtered (" FPUL=%08lx FPSCR=%08lx",
		   (long) read_register (tdep->FPUL_REGNUM),
		   (long) read_register (tdep->FPSCR_REGNUM));

  printf_filtered ("\nR0-R7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));

  printf_filtered (("FP0-FP7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n"),
		   (long) read_register (FP0_REGNUM + 0),
		   (long) read_register (FP0_REGNUM + 1),
		   (long) read_register (FP0_REGNUM + 2),
		   (long) read_register (FP0_REGNUM + 3),
		   (long) read_register (FP0_REGNUM + 4),
		   (long) read_register (FP0_REGNUM + 5),
		   (long) read_register (FP0_REGNUM + 6),
		   (long) read_register (FP0_REGNUM + 7));
  printf_filtered (("FP8-FP15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n"),
		   (long) read_register (FP0_REGNUM + 8),
		   (long) read_register (FP0_REGNUM + 9),
		   (long) read_register (FP0_REGNUM + 10),
		   (long) read_register (FP0_REGNUM + 11),
		   (long) read_register (FP0_REGNUM + 12),
		   (long) read_register (FP0_REGNUM + 13),
		   (long) read_register (FP0_REGNUM + 14),
		   (long) read_register (FP0_REGNUM + 15));
}

static void
sh3_dsp_show_regs (void)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s SR=%08lx PR=%08lx MACH=%08lx MACHL=%08lx\n",
		   paddr (read_register (PC_REGNUM)),
		   (long) read_register (tdep->SR_REGNUM),
		   (long) read_register (tdep->PR_REGNUM),
		   (long) read_register (MACH_REGNUM),
		   (long) read_register (MACL_REGNUM));

  printf_filtered ("GBR=%08lx VBR=%08lx",
		   (long) read_register (GBR_REGNUM),
		   (long) read_register (VBR_REGNUM));

  printf_filtered (" SSR=%08lx SPC=%08lx",
		   (long) read_register (tdep->SSR_REGNUM),
		   (long) read_register (tdep->SPC_REGNUM));

  printf_filtered (" DSR=%08lx", 
		   (long) read_register (tdep->DSR_REGNUM));

  printf_filtered ("\nR0-R7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));

  printf_filtered ("A0G=%02lx A0=%08lx M0=%08lx X0=%08lx Y0=%08lx RS=%08lx MOD=%08lx\n",
		   (long) read_register (tdep->A0G_REGNUM) & 0xff,
		   (long) read_register (tdep->A0_REGNUM),
		   (long) read_register (tdep->M0_REGNUM),
		   (long) read_register (tdep->X0_REGNUM),
		   (long) read_register (tdep->Y0_REGNUM),
		   (long) read_register (tdep->RS_REGNUM),
		   (long) read_register (tdep->MOD_REGNUM));
  printf_filtered ("A1G=%02lx A1=%08lx M1=%08lx X1=%08lx Y1=%08lx RE=%08lx\n",
		   (long) read_register (tdep->A1G_REGNUM) & 0xff,
		   (long) read_register (tdep->A1_REGNUM),
		   (long) read_register (tdep->M1_REGNUM),
		   (long) read_register (tdep->X1_REGNUM),
		   (long) read_register (tdep->Y1_REGNUM),
		   (long) read_register (tdep->RE_REGNUM));
}

static void
sh4_show_regs (void)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  int pr = read_register (tdep->FPSCR_REGNUM) & 0x80000;
  printf_filtered ("PC=%s SR=%08lx PR=%08lx MACH=%08lx MACHL=%08lx\n",
		   paddr (read_register (PC_REGNUM)),
		   (long) read_register (tdep->SR_REGNUM),
		   (long) read_register (tdep->PR_REGNUM),
		   (long) read_register (MACH_REGNUM),
		   (long) read_register (MACL_REGNUM));

  printf_filtered ("GBR=%08lx VBR=%08lx",
		   (long) read_register (GBR_REGNUM),
		   (long) read_register (VBR_REGNUM));
  printf_filtered (" SSR=%08lx SPC=%08lx",
		   (long) read_register (tdep->SSR_REGNUM),
		   (long) read_register (tdep->SPC_REGNUM));
  printf_filtered (" FPUL=%08lx FPSCR=%08lx",
		   (long) read_register (tdep->FPUL_REGNUM),
		   (long) read_register (tdep->FPSCR_REGNUM));

  printf_filtered ("\nR0-R7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));

  printf_filtered ((pr
		    ? "DR0-DR6  %08lx%08lx %08lx%08lx %08lx%08lx %08lx%08lx\n"
		    : "FP0-FP7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n"),
		   (long) read_register (FP0_REGNUM + 0),
		   (long) read_register (FP0_REGNUM + 1),
		   (long) read_register (FP0_REGNUM + 2),
		   (long) read_register (FP0_REGNUM + 3),
		   (long) read_register (FP0_REGNUM + 4),
		   (long) read_register (FP0_REGNUM + 5),
		   (long) read_register (FP0_REGNUM + 6),
		   (long) read_register (FP0_REGNUM + 7));
  printf_filtered ((pr
		    ? "DR8-DR14 %08lx%08lx %08lx%08lx %08lx%08lx %08lx%08lx\n"
		    : "FP8-FP15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n"),
		   (long) read_register (FP0_REGNUM + 8),
		   (long) read_register (FP0_REGNUM + 9),
		   (long) read_register (FP0_REGNUM + 10),
		   (long) read_register (FP0_REGNUM + 11),
		   (long) read_register (FP0_REGNUM + 12),
		   (long) read_register (FP0_REGNUM + 13),
		   (long) read_register (FP0_REGNUM + 14),
		   (long) read_register (FP0_REGNUM + 15));
}

static void
sh_dsp_show_regs (void)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s SR=%08lx PR=%08lx MACH=%08lx MACHL=%08lx\n",
		   paddr (read_register (PC_REGNUM)),
		   (long) read_register (tdep->SR_REGNUM),
		   (long) read_register (tdep->PR_REGNUM),
		   (long) read_register (MACH_REGNUM),
		   (long) read_register (MACL_REGNUM));

  printf_filtered ("GBR=%08lx VBR=%08lx",
		   (long) read_register (GBR_REGNUM),
		   (long) read_register (VBR_REGNUM));

  printf_filtered (" DSR=%08lx", 
		   (long) read_register (tdep->DSR_REGNUM));

  printf_filtered ("\nR0-R7  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));

  printf_filtered ("A0G=%02lx A0=%08lx M0=%08lx X0=%08lx Y0=%08lx RS=%08lx MOD=%08lx\n",
		   (long) read_register (tdep->A0G_REGNUM) & 0xff,
		   (long) read_register (tdep->A0_REGNUM),
		   (long) read_register (tdep->M0_REGNUM),
		   (long) read_register (tdep->X0_REGNUM),
		   (long) read_register (tdep->Y0_REGNUM),
		   (long) read_register (tdep->RS_REGNUM),
		   (long) read_register (tdep->MOD_REGNUM));
  printf_filtered ("A1G=%02lx A1=%08lx M1=%08lx X1=%08lx Y1=%08lx RE=%08lx\n",
		   (long) read_register (tdep->A1G_REGNUM) & 0xff,
		   (long) read_register (tdep->A1_REGNUM),
		   (long) read_register (tdep->M1_REGNUM),
		   (long) read_register (tdep->X1_REGNUM),
		   (long) read_register (tdep->Y1_REGNUM),
		   (long) read_register (tdep->RE_REGNUM));
}

static void
sh64_show_media_regs (void)
{
  int i;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s SR=%016llx \n",
		   paddr (read_register (PC_REGNUM)),
		   (long long) read_register (tdep->SR_REGNUM));

  printf_filtered ("SSR=%016llx SPC=%016llx \n",
		   (long long) read_register (tdep->SSR_REGNUM),
		   (long long) read_register (tdep->SPC_REGNUM));
  printf_filtered ("FPSCR=%016lx\n ",
		   (long) read_register (tdep->FPSCR_REGNUM));

  for (i = 0; i < 64; i = i + 4)
    printf_filtered ("\nR%d-R%d  %016llx %016llx %016llx %016llx\n",
		     i, i + 3,
		     (long long) read_register (i + 0),
		     (long long) read_register (i + 1),
		     (long long) read_register (i + 2),
		     (long long) read_register (i + 3));

  printf_filtered ("\n");
  
  for (i = 0; i < 64; i = i + 8)
    printf_filtered ("FR%d-FR%d  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		     i, i + 7,
		     (long) read_register (FP0_REGNUM + i + 0),
		     (long) read_register (FP0_REGNUM + i + 1),
		     (long) read_register (FP0_REGNUM + i + 2),
		     (long) read_register (FP0_REGNUM + i + 3),
		     (long) read_register (FP0_REGNUM + i + 4),
		     (long) read_register (FP0_REGNUM + i + 5),
		     (long) read_register (FP0_REGNUM + i + 6),
		     (long) read_register (FP0_REGNUM + i + 7));
}

static void
sh64_show_compact_regs (void)
{
  int i;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  printf_filtered ("PC=%s \n",
		   paddr (read_register (tdep->PC_C_REGNUM)));

  printf_filtered ("GBR=%08lx MACH=%08lx MACL=%08lx PR=%08lx T=%08lx\n",
		   (long) read_register (tdep->GBR_C_REGNUM),
		   (long) read_register (tdep->MACH_C_REGNUM),
		   (long) read_register (tdep->MACL_C_REGNUM),
		   (long) read_register (tdep->PR_C_REGNUM),
		   (long) read_register (tdep->T_C_REGNUM));
  printf_filtered ("FPSCR=%08lx FPUL=%08lx\n",
		   (long) read_register (tdep->FPSCR_REGNUM),
		   (long) read_register (tdep->FPUL_REGNUM));

  for (i = 0; i < 16; i = i + 4)
    printf_filtered ("\nR%d-R%d  %08lx %08lx %08lx %08lx\n",
		     i, i + 3,
		     (long) read_register (i + 0),
		     (long) read_register (i + 1),
		     (long) read_register (i + 2),
		     (long) read_register (i + 3));

  printf_filtered ("\n");
  
  for (i = 0; i < 16; i = i + 8)
    printf_filtered ("FR%d-FR%d  %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		     i, i + 7,
		     (long) read_register (FP0_REGNUM + i + 0),
		     (long) read_register (FP0_REGNUM + i + 1),
		     (long) read_register (FP0_REGNUM + i + 2),
		     (long) read_register (FP0_REGNUM + i + 3),
		     (long) read_register (FP0_REGNUM + i + 4),
		     (long) read_register (FP0_REGNUM + i + 5),
		     (long) read_register (FP0_REGNUM + i + 6),
		     (long) read_register (FP0_REGNUM + i + 7));
}

/*FIXME!!! This only shows the registers for shmedia, excluding the
  pseudo registers. */
static void
sh64_show_regs (void)
{
  if (pc_is_isa32 (selected_frame->pc))
    sh64_show_media_regs ();
  else
    sh64_show_compact_regs ();
}

void sh_show_regs_command (char *args, int from_tty)
{
  if (sh_show_regs)
    (*sh_show_regs)();
}

/* Index within `registers' of the first byte of the space for
   register N.  */
static int
sh_default_register_byte (int reg_nr)
{
  return (reg_nr * 4);
}

static int
sh_sh4_register_byte (int reg_nr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (reg_nr >= tdep->DR0_REGNUM 
      && reg_nr <= tdep->DR_LAST_REGNUM)
    return (dr_reg_base_num (reg_nr) * 4);
  else if  (reg_nr >= tdep->FV0_REGNUM 
	    && reg_nr <= tdep->FV_LAST_REGNUM)
    return (fv_reg_base_num (reg_nr) * 4);
  else
    return (reg_nr * 4);
}

/* *INDENT-OFF* */
/*
    SH MEDIA MODE (ISA 32)
    general registers (64-bit) 0-63
0    r0,   r1,   r2,   r3,   r4,   r5,   r6,   r7,
64   r8,   r9,   r10,  r11,  r12,  r13,  r14,  r15,
128  r16,  r17,  r18,  r19,  r20,  r21,  r22,  r23,
192  r24,  r25,  r26,  r27,  r28,  r29,  r30,  r31,
256  r32,  r33,  r34,  r35,  r36,  r37,  r38,  r39,
320  r40,  r41,  r42,  r43,  r44,  r45,  r46,  r47,
384  r48,  r49,  r50,  r51,  r52,  r53,  r54,  r55,
448  r56,  r57,  r58,  r59,  r60,  r61,  r62,  r63,

    pc (64-bit) 64
512  pc,

    status reg., saved status reg., saved pc reg. (64-bit) 65-67
520  sr,  ssr,  spc,

    target registers (64-bit) 68-75
544  tr0,  tr1,  tr2,  tr3,  tr4,  tr5,  tr6,  tr7,

    floating point state control register (32-bit) 76
608  fpscr,

    single precision floating point registers (32-bit) 77-140
612  fr0,  fr1,  fr2,  fr3,  fr4,  fr5,  fr6,  fr7,
644  fr8,  fr9,  fr10, fr11, fr12, fr13, fr14, fr15,
676  fr16, fr17, fr18, fr19, fr20, fr21, fr22, fr23,
708  fr24, fr25, fr26, fr27, fr28, fr29, fr30, fr31,
740  fr32, fr33, fr34, fr35, fr36, fr37, fr38, fr39,
772  fr40, fr41, fr42, fr43, fr44, fr45, fr46, fr47,
804  fr48, fr49, fr50, fr51, fr52, fr53, fr54, fr55,
836  fr56, fr57, fr58, fr59, fr60, fr61, fr62, fr63,

TOTAL SPACE FOR REGISTERS: 868 bytes

From here on they are all pseudo registers: no memory allocated.
REGISTER_BYTE returns the register byte for the base register.

    double precision registers (pseudo) 141-172
     dr0,  dr2,  dr4,  dr6,  dr8,  dr10, dr12, dr14,
     dr16, dr18, dr20, dr22, dr24, dr26, dr28, dr30,
     dr32, dr34, dr36, dr38, dr40, dr42, dr44, dr46,
     dr48, dr50, dr52, dr54, dr56, dr58, dr60, dr62,
 
    floating point pairs (pseudo) 173-204
     fp0,  fp2,  fp4,  fp6,  fp8,  fp10, fp12, fp14,
     fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30,
     fp32, fp34, fp36, fp38, fp40, fp42, fp44, fp46,
     fp48, fp50, fp52, fp54, fp56, fp58, fp60, fp62,
 
    floating point vectors (4 floating point regs) (pseudo) 205-220
     fv0,  fv4,  fv8,  fv12, fv16, fv20, fv24, fv28,
     fv32, fv36, fv40, fv44, fv48, fv52, fv56, fv60,
 
    SH COMPACT MODE (ISA 16) (all pseudo) 221-272
     r0_c, r1_c, r2_c,  r3_c,  r4_c,  r5_c,  r6_c,  r7_c,
     r8_c, r9_c, r10_c, r11_c, r12_c, r13_c, r14_c, r15_c,
     pc_c,
     gbr_c, mach_c, macl_c, pr_c, t_c,
     fpscr_c, fpul_c,
     fr0_c, fr1_c, fr2_c,  fr3_c,  fr4_c,  fr5_c,  fr6_c,  fr7_c,
     fr8_c, fr9_c, fr10_c, fr11_c, fr12_c, fr13_c, fr14_c, fr15_c
     dr0_c, dr2_c, dr4_c,  dr6_c,  dr8_c,  dr10_c, dr12_c, dr14_c
     fv0_c, fv4_c, fv8_c,  fv12_c
*/
/* *INDENT-ON* */
static int
sh_sh64_register_byte (int reg_nr)
{
  int base_regnum = -1;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  /* If it is a pseudo register, get the number of the first floating
     point register that is part of it. */
  if (reg_nr >= tdep->DR0_REGNUM 
      && reg_nr <= tdep->DR_LAST_REGNUM)
    base_regnum = dr_reg_base_num (reg_nr);

  else if (reg_nr >= tdep->FPP0_REGNUM 
	    && reg_nr <= tdep->FPP_LAST_REGNUM)
    base_regnum = fpp_reg_base_num (reg_nr);

  else if (reg_nr >= tdep->FV0_REGNUM 
	    && reg_nr <= tdep->FV_LAST_REGNUM)
    base_regnum = fv_reg_base_num (reg_nr);

  /* sh compact pseudo register. FPSCR is a pathological case, need to
     treat it as special. */
  else if ((reg_nr >= tdep->R0_C_REGNUM 
	    && reg_nr <= tdep->FV_LAST_C_REGNUM) 
	   && reg_nr != tdep->FPSCR_C_REGNUM)
    base_regnum = sh64_compact_reg_base_num (reg_nr);

  /* Now return the offset in bytes within the register cache. */
  /* sh media pseudo register, i.e. any of DR, FFP, FV registers. */
  if (reg_nr >= tdep->DR0_REGNUM 
      && reg_nr <= tdep->FV_LAST_REGNUM)
    return (base_regnum - FP0_REGNUM + 1) * 4 
      + (tdep->TR7_REGNUM + 1) * 8;

  /* sh compact pseudo register: general register */
  if ((reg_nr >= tdep->R0_C_REGNUM 
       && reg_nr <= tdep->R_LAST_C_REGNUM))
    return (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
	    ? base_regnum * 8 + 4
	    : base_regnum * 8);

  /* sh compact pseudo register: */
  if (reg_nr == tdep->PC_C_REGNUM 
       || reg_nr == tdep->GBR_C_REGNUM
       || reg_nr == tdep->MACL_C_REGNUM
       || reg_nr == tdep->PR_C_REGNUM)
    return (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
	    ? base_regnum * 8 + 4
	    : base_regnum * 8);

  if (reg_nr == tdep->MACH_C_REGNUM) 
    return base_regnum * 8;

  if (reg_nr == tdep->T_C_REGNUM) 
    return base_regnum * 8; /* FIXME??? how do we get bit 0? Do we have to? */

  /* sh compact pseudo register: floating point register */
  else if (reg_nr >=tdep->FP0_C_REGNUM
	   && reg_nr <= tdep->FV_LAST_C_REGNUM)
    return (base_regnum  - FP0_REGNUM) * 4
      + (tdep->TR7_REGNUM + 1) * 8 + 4;

  else if (reg_nr == tdep->FPSCR_C_REGNUM)
    /* This is complicated, for now return the beginning of the
       architectural FPSCR register. */
    return (tdep->TR7_REGNUM + 1) * 8;

  else if (reg_nr == tdep->FPUL_C_REGNUM)
    return ((base_regnum - FP0_REGNUM) * 4 + 
	    (tdep->TR7_REGNUM + 1) * 8 + 4);

  /* It is not a pseudo register. */
  /* It is a 64 bit register. */
  else if (reg_nr <= tdep->TR7_REGNUM)
    return reg_nr * 8;

  /* It is a 32 bit register. */
  else
    if (reg_nr == tdep->FPSCR_REGNUM)
      return (tdep->FPSCR_REGNUM * 8);

  /* It is floating point 32-bit register */
  else
    return ((tdep->TR7_REGNUM + 1) * 8 
      + (reg_nr - FP0_REGNUM + 1) * 4);
}

/* Number of bytes of storage in the actual machine representation for
   register REG_NR.  */
static int
sh_default_register_raw_size (int reg_nr)
{
  return 4;
}

static int
sh_sh4_register_raw_size (int reg_nr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (reg_nr >= tdep->DR0_REGNUM 
      && reg_nr <= tdep->DR_LAST_REGNUM)
    return 8;
  else if  (reg_nr >= tdep->FV0_REGNUM 
	    && reg_nr <= tdep->FV_LAST_REGNUM)
    return 16;
  else
    return 4;
}

static int
sh_sh64_register_raw_size (int reg_nr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if ((reg_nr >= tdep->DR0_REGNUM 
       && reg_nr <= tdep->DR_LAST_REGNUM)
      || (reg_nr >= tdep->FPP0_REGNUM 
	  && reg_nr <= tdep->FPP_LAST_REGNUM)
      || (reg_nr >= tdep->DR0_C_REGNUM 
	  && reg_nr <= tdep->DR_LAST_C_REGNUM)
      || (reg_nr <= tdep->TR7_REGNUM))
    return 8;

  else if ((reg_nr >= tdep->FV0_REGNUM 
	    && reg_nr <= tdep->FV_LAST_REGNUM)
	   || (reg_nr >= tdep->FV0_C_REGNUM 
	       && reg_nr <= tdep->FV_LAST_C_REGNUM))
    return 16;

  else /* this covers also the 32-bit SH compact registers. */
    return 4;
}

/* Number of bytes of storage in the program's representation
   for register N.  */
static int
sh_register_virtual_size (int reg_nr)
{
  return 4;
}

/* ??????? FIXME */
static int
sh_sh64_register_virtual_size (int reg_nr)
{
  if (reg_nr >= FP0_REGNUM
      && reg_nr <= gdbarch_tdep (current_gdbarch)->FP_LAST_REGNUM)
    return 4;
  else
    return 8;
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */
static struct type *
sh_sh3e_register_virtual_type (int reg_nr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if ((reg_nr >= FP0_REGNUM
       && (reg_nr <= tdep->FP_LAST_REGNUM)) 
      || (reg_nr == tdep->FPUL_REGNUM))
    return builtin_type_float;
  else
    return builtin_type_int;
}

static struct type *
sh_sh4_build_float_register_type (int high)
{
  struct type *temp;

  temp = create_range_type (NULL, builtin_type_int, 0, high);
  return create_array_type (NULL, builtin_type_float, temp);
}

static struct type *
sh_sh4_register_virtual_type (int reg_nr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if ((reg_nr >= FP0_REGNUM
       && (reg_nr <= tdep->FP_LAST_REGNUM)) 
      || (reg_nr == tdep->FPUL_REGNUM))
    return builtin_type_float;
  else if (reg_nr >= tdep->DR0_REGNUM 
	   && reg_nr <= tdep->DR_LAST_REGNUM)
    return builtin_type_double;
  else if  (reg_nr >= tdep->FV0_REGNUM 
	   && reg_nr <= tdep->FV_LAST_REGNUM)
    return sh_sh4_build_float_register_type (3);
  else
    return builtin_type_int;
}

static struct type *
sh_sh64_register_virtual_type (int reg_nr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if ((reg_nr >= FP0_REGNUM
       && reg_nr <= tdep->FP_LAST_REGNUM)
      || (reg_nr >= tdep->FP0_C_REGNUM
	  && reg_nr <= tdep->FP_LAST_C_REGNUM))
    return builtin_type_float;
  else if ((reg_nr >= tdep->DR0_REGNUM 
	    && reg_nr <= tdep->DR_LAST_REGNUM)
	   || (reg_nr >= tdep->DR0_C_REGNUM 
	       && reg_nr <= tdep->DR_LAST_C_REGNUM))
    return builtin_type_double;
  else if  (reg_nr >= tdep->FPP0_REGNUM 
	    && reg_nr <= tdep->FPP_LAST_REGNUM)
    return sh_sh4_build_float_register_type (1);
  else if ((reg_nr >= tdep->FV0_REGNUM
	    && reg_nr <= tdep->FV_LAST_REGNUM)
	   ||(reg_nr >= tdep->FV0_C_REGNUM 
	      && reg_nr <= tdep->FV_LAST_C_REGNUM))
    return sh_sh4_build_float_register_type (3);
  else if (reg_nr == tdep->FPSCR_REGNUM)
    return builtin_type_int;
  else if (reg_nr >= tdep->R0_C_REGNUM
	   && reg_nr < tdep->FP0_C_REGNUM)
    return builtin_type_int;
  else
    return builtin_type_long_long;
}

static struct type *
sh_default_register_virtual_type (int reg_nr)
{
  return builtin_type_int;
}

/* On the sh4, the DRi pseudo registers are problematic if the target
   is little endian. When the user writes one of those registers, for
   instance with 'ser var $dr0=1', we want the double to be stored
   like this: 
   fr0 = 0x00 0x00 0x00 0x00 0x00 0xf0 0x3f 
   fr1 = 0x00 0x00 0x00 0x00 0x00 0x00 0x00 

   This corresponds to little endian byte order & big endian word
   order.  However if we let gdb write the register w/o conversion, it
   will write fr0 and fr1 this way:
   fr0 = 0x00 0x00 0x00 0x00 0x00 0x00 0x00
   fr1 = 0x00 0x00 0x00 0x00 0x00 0xf0 0x3f
   because it will consider fr0 and fr1 as a single LE stretch of memory.
   
   To achieve what we want we must force gdb to store things in
   floatformat_ieee_double_littlebyte_bigword (which is defined in
   include/floatformat.h and libiberty/floatformat.c.

   In case the target is big endian, there is no problem, the
   raw bytes will look like:
   fr0 = 0x3f 0xf0 0x00 0x00 0x00 0x00 0x00
   fr1 = 0x00 0x00 0x00 0x00 0x00 0x00 0x00 

   The other pseudo registers (the FVs) also don't pose a problem
   because they are stored as 4 individual FP elements. */

static void
sh_sh4_register_convert_to_virtual (int regnum, struct type *type,
                                  char *from, char *to)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (regnum >= tdep->DR0_REGNUM 
      && regnum <= tdep->DR_LAST_REGNUM)
    {
      DOUBLEST val;
      floatformat_to_doublest (&floatformat_ieee_double_littlebyte_bigword, from, &val);
      store_floating (to, TYPE_LENGTH (type), val);
    }
  else
    error ("sh_register_convert_to_virtual called with non DR register number");
}

void
sh_sh64_register_convert_to_virtual (int regnum, struct type *type,
				     char *from, char *to)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (TARGET_BYTE_ORDER != BFD_ENDIAN_LITTLE)
    {
      /* It is a no-op. */
      memcpy (to, from, REGISTER_RAW_SIZE (regnum));
      return;
    }

  if ((regnum >= tdep->DR0_REGNUM 
       && regnum <= tdep->DR_LAST_REGNUM)
      || (regnum >= tdep->DR0_C_REGNUM 
	  && regnum <= tdep->DR_LAST_C_REGNUM))
    {
      DOUBLEST val;
      floatformat_to_doublest (&floatformat_ieee_double_littlebyte_bigword, from, &val);
      store_floating(to, TYPE_LENGTH(type), val);
    }
  else
    error("sh_register_convert_to_virtual called with non DR register number");
}

static void
sh_sh4_register_convert_to_raw (struct type *type, int regnum,
				const void *from, void *to)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (regnum >= tdep->DR0_REGNUM 
      && regnum <= tdep->DR_LAST_REGNUM)
    {
      DOUBLEST val = extract_floating (from, TYPE_LENGTH(type));
      floatformat_from_doublest (&floatformat_ieee_double_littlebyte_bigword, &val, to);
    }
  else
    error("sh_register_convert_to_raw called with non DR register number");
}

void
sh_sh64_register_convert_to_raw (struct type *type, int regnum,
				 const void *from, void *to)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (TARGET_BYTE_ORDER != BFD_ENDIAN_LITTLE)
    {
      /* It is a no-op. */
      memcpy (to, from, REGISTER_RAW_SIZE (regnum));
      return;
    }

  if ((regnum >= tdep->DR0_REGNUM 
       && regnum <= tdep->DR_LAST_REGNUM)
      || (regnum >= tdep->DR0_C_REGNUM 
	  && regnum <= tdep->DR_LAST_C_REGNUM))
    {
      DOUBLEST val = extract_floating (from, TYPE_LENGTH(type));
      floatformat_from_doublest (&floatformat_ieee_double_littlebyte_bigword, &val, to);
    }
  else
    error("sh_register_convert_to_raw called with non DR register number");
}

void
sh_pseudo_register_read (struct gdbarch *gdbarch, struct regcache *regcache,
			 int reg_nr, void *buffer)
{
  int base_regnum, portion;
  char *temp_buffer = (char*) alloca (MAX_REGISTER_RAW_SIZE);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch); 

  if (reg_nr >= tdep->DR0_REGNUM 
      && reg_nr <= tdep->DR_LAST_REGNUM)
    {
      base_regnum = dr_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      /* Read the real regs for which this one is an alias.  */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   (temp_buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));
      /* We must pay attention to the endiannes. */
      sh_sh4_register_convert_to_virtual (reg_nr,
					  REGISTER_VIRTUAL_TYPE (reg_nr),
					  temp_buffer, buffer);
    }
  else if (reg_nr >= tdep->FV0_REGNUM 
	   && reg_nr <= tdep->FV_LAST_REGNUM)
    {
      base_regnum = fv_reg_base_num (reg_nr);

      /* Read the real regs for which this one is an alias.  */
      for (portion = 0; portion < 4; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   ((char *) buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));
    }
}

static void
sh64_pseudo_register_read (struct gdbarch *gdbarch, struct regcache *regcache,
			   int reg_nr, void *buffer)
{
  int base_regnum;
  int portion;
  int offset = 0;
  char *temp_buffer = (char*) alloca (MAX_REGISTER_RAW_SIZE);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch); 

  if (reg_nr >= tdep->DR0_REGNUM 
      && reg_nr <= tdep->DR_LAST_REGNUM)
    {
      base_regnum = dr_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      /* DR regs are double precision registers obtained by
	 concatenating 2 single precision floating point registers. */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   (temp_buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));

      /* We must pay attention to the endiannes. */
      sh_sh64_register_convert_to_virtual (reg_nr, REGISTER_VIRTUAL_TYPE (reg_nr),
					   temp_buffer, buffer);

    }

  else if (reg_nr >= tdep->FPP0_REGNUM 
	   && reg_nr <= tdep->FPP_LAST_REGNUM)
    {
      base_regnum = fpp_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      /* FPP regs are pairs of single precision registers obtained by
	 concatenating 2 single precision floating point registers. */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   ((char *) buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));
    }

  else if (reg_nr >= tdep->FV0_REGNUM 
	   && reg_nr <= tdep->FV_LAST_REGNUM)
    {
      base_regnum = fv_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      /* FV regs are vectors of single precision registers obtained by
	 concatenating 4 single precision floating point registers. */
      for (portion = 0; portion < 4; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   ((char *) buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));
    }

  /* sh compact pseudo registers. 1-to-1 with a shmedia register */
  else if (reg_nr >= tdep->R0_C_REGNUM 
	   && reg_nr <= tdep->T_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      regcache_raw_read (regcache, base_regnum, temp_buffer);
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	offset = 4;
      memcpy (buffer, temp_buffer + offset, 4); /* get LOWER 32 bits only????*/
    }

  else if (reg_nr >= tdep->FP0_C_REGNUM
	   && reg_nr <= tdep->FP_LAST_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      /* Floating point registers map 1-1 to the media fp regs,
	 they have the same size and endienness. */
      regcache_raw_read (regcache, base_regnum, buffer);
    }

  else if (reg_nr >= tdep->DR0_C_REGNUM 
	   && reg_nr <= tdep->DR_LAST_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);

      /* DR_C regs are double precision registers obtained by
	 concatenating 2 single precision floating point registers. */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   (temp_buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));

      /* We must pay attention to the endiannes. */
      sh_sh64_register_convert_to_virtual (reg_nr, REGISTER_VIRTUAL_TYPE (reg_nr),
					   temp_buffer, buffer);
    }

  else if (reg_nr >= tdep->FV0_C_REGNUM 
	   && reg_nr <= tdep->FV_LAST_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);

      /* Build the value in the provided buffer. */ 
      /* FV_C regs are vectors of single precision registers obtained by
	 concatenating 4 single precision floating point registers. */
      for (portion = 0; portion < 4; portion++)
	regcache_raw_read (regcache, base_regnum + portion, 
			   ((char *) buffer
			    + REGISTER_RAW_SIZE (base_regnum) * portion));
    }

  else if (reg_nr == tdep->FPSCR_C_REGNUM)
    {
      int fpscr_base_regnum;
      int sr_base_regnum;
      unsigned int fpscr_value;
      unsigned int sr_value;
      unsigned int fpscr_c_value;
      unsigned int fpscr_c_part1_value;
      unsigned int fpscr_c_part2_value;

      fpscr_base_regnum = tdep->FPSCR_REGNUM;
      sr_base_regnum = tdep->SR_REGNUM;

      /* Build the value in the provided buffer. */ 
      /* FPSCR_C is a very weird register that contains sparse bits
	 from the FPSCR and the SR architectural registers.
	 Specifically: */
      /* *INDENT-OFF* */
      /*
	 FPSRC_C bit
            0         Bit 0 of FPSCR
            1         reserved
            2-17      Bit 2-18 of FPSCR
            18-20     Bits 12,13,14 of SR
            21-31     reserved
       */
      /* *INDENT-ON* */
      /* Get FPSCR into a local buffer */
      regcache_raw_read (regcache, fpscr_base_regnum, temp_buffer);
      /* Get value as an int. */
      fpscr_value = extract_unsigned_integer (temp_buffer, 4);
      /* Get SR into a local buffer */
      regcache_raw_read (regcache, sr_base_regnum, temp_buffer);
      /* Get value as an int. */
      sr_value = extract_unsigned_integer (temp_buffer, 4);
      /* Build the new value. */
      fpscr_c_part1_value = fpscr_value & 0x3fffd;
      fpscr_c_part2_value = (sr_value & 0x7000) << 6;
      fpscr_c_value = fpscr_c_part1_value | fpscr_c_part2_value;
      /* Store that in out buffer!!! */
      store_unsigned_integer (buffer, 4, fpscr_c_value);
      /* FIXME There is surely an endianness gotcha here. */
    }

  else if (reg_nr == tdep->FPUL_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);

      /* FPUL_C register is floating point register 32,
	 same size, same endianness. */
      regcache_raw_read (regcache, base_regnum, buffer);
    }
}

void
sh_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			  int reg_nr, const void *buffer)
{
  int base_regnum, portion;
  char *temp_buffer = (char*) alloca (MAX_REGISTER_RAW_SIZE);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch); 

  if (reg_nr >= tdep->DR0_REGNUM
      && reg_nr <= tdep->DR_LAST_REGNUM)
    {
      base_regnum = dr_reg_base_num (reg_nr);

      /* We must pay attention to the endiannes. */
      sh_sh4_register_convert_to_raw (REGISTER_VIRTUAL_TYPE (reg_nr), reg_nr,
				      buffer, temp_buffer);

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_write (regcache, base_regnum + portion, 
			    (temp_buffer
			     + REGISTER_RAW_SIZE (base_regnum) * portion));
    }
  else if (reg_nr >= tdep->FV0_REGNUM
	   && reg_nr <= tdep->FV_LAST_REGNUM)
    {
      base_regnum = fv_reg_base_num (reg_nr);

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 4; portion++)
	regcache_raw_write (regcache, base_regnum + portion,
			    ((char *) buffer
			     + REGISTER_RAW_SIZE (base_regnum) * portion));
    }
}

void
sh64_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int reg_nr, const void *buffer)
{
  int base_regnum, portion;
  int offset;
  char *temp_buffer = (char*) alloca (MAX_REGISTER_RAW_SIZE);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (reg_nr >= tdep->DR0_REGNUM
      && reg_nr <= tdep->DR_LAST_REGNUM)
    {
      base_regnum = dr_reg_base_num (reg_nr);
      /* We must pay attention to the endiannes. */
      sh_sh64_register_convert_to_raw (REGISTER_VIRTUAL_TYPE (reg_nr), reg_nr,
				       buffer, temp_buffer);
	  

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_write (regcache, base_regnum + portion, 
			    (temp_buffer
			     + REGISTER_RAW_SIZE (base_regnum) * portion));
    }

  else if (reg_nr >= tdep->FPP0_REGNUM 
	   && reg_nr <= tdep->FPP_LAST_REGNUM)
    {
      base_regnum = fpp_reg_base_num (reg_nr);

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 2; portion++)
	regcache_raw_write (regcache, base_regnum + portion,
			    ((char *) buffer
			     + REGISTER_RAW_SIZE (base_regnum) * portion));
    }

  else if (reg_nr >= tdep->FV0_REGNUM
	   && reg_nr <= tdep->FV_LAST_REGNUM)
    {
      base_regnum = fv_reg_base_num (reg_nr);

      /* Write the real regs for which this one is an alias.  */
      for (portion = 0; portion < 4; portion++)
	regcache_raw_write (regcache, base_regnum + portion,
			    ((char *) buffer
			     + REGISTER_RAW_SIZE (base_regnum) * portion));
    }

  /* sh compact general pseudo registers. 1-to-1 with a shmedia
     register but only 4 bytes of it.  */
  else if (reg_nr >= tdep->R0_C_REGNUM 
	   && reg_nr <= tdep->T_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);
      /* reg_nr is 32 bit here, and base_regnum is 64 bits. */
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	offset = 4;
      else 
	offset = 0;
      /* Let's read the value of the base register into a temporary
	 buffer, so that overwriting the last four bytes with the new
	 value of the pseudo will leave the upper 4 bytes unchanged. */
      regcache_raw_read (regcache, base_regnum, temp_buffer);
      /* Write as an 8 byte quantity */
      memcpy (temp_buffer + offset, buffer, 4);
      regcache_raw_write (regcache, base_regnum, temp_buffer);
    }

  /* sh floating point compact pseudo registers. 1-to-1 with a shmedia
     registers. Both are 4 bytes. */
  else if (reg_nr >= tdep->FP0_C_REGNUM
	       && reg_nr <= tdep->FP_LAST_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);
      regcache_raw_write (regcache, base_regnum, buffer);
    }

  else if (reg_nr >= tdep->DR0_C_REGNUM 
	   && reg_nr <= tdep->DR_LAST_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);
      for (portion = 0; portion < 2; portion++)
	{
	  /* We must pay attention to the endiannes. */
	  sh_sh64_register_convert_to_raw (REGISTER_VIRTUAL_TYPE (reg_nr), reg_nr,
					   buffer, temp_buffer);

	  regcache_raw_write (regcache, base_regnum + portion,
			      (temp_buffer
			       + REGISTER_RAW_SIZE (base_regnum) * portion));
	}
    }

  else if (reg_nr >= tdep->FV0_C_REGNUM 
	   && reg_nr <= tdep->FV_LAST_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);
     
      for (portion = 0; portion < 4; portion++)
	{
	  regcache_raw_write (regcache, base_regnum + portion,
			      ((char *) buffer
			       + REGISTER_RAW_SIZE (base_regnum) * portion));
	}
    }

  else if (reg_nr == tdep->FPSCR_C_REGNUM)
    {      
      int fpscr_base_regnum;
      int sr_base_regnum;
      unsigned int fpscr_value;
      unsigned int sr_value;
      unsigned int old_fpscr_value;
      unsigned int old_sr_value;
      unsigned int fpscr_c_value;
      unsigned int fpscr_mask;
      unsigned int sr_mask;

      fpscr_base_regnum = tdep->FPSCR_REGNUM;
      sr_base_regnum = tdep->SR_REGNUM;

      /* FPSCR_C is a very weird register that contains sparse bits
	 from the FPSCR and the SR architectural registers.
	 Specifically: */
      /* *INDENT-OFF* */
      /*
	 FPSRC_C bit
            0         Bit 0 of FPSCR
            1         reserved
            2-17      Bit 2-18 of FPSCR
            18-20     Bits 12,13,14 of SR
            21-31     reserved
       */
      /* *INDENT-ON* */
      /* Get value as an int. */
      fpscr_c_value = extract_unsigned_integer (buffer, 4);

      /* Build the new values. */
      fpscr_mask = 0x0003fffd;
      sr_mask = 0x001c0000;
       
      fpscr_value = fpscr_c_value & fpscr_mask;
      sr_value = (fpscr_value & sr_mask) >> 6;
      
      regcache_raw_read (regcache, fpscr_base_regnum, temp_buffer);
      old_fpscr_value = extract_unsigned_integer (temp_buffer, 4);
      old_fpscr_value &= 0xfffc0002;
      fpscr_value |= old_fpscr_value;
      store_unsigned_integer (temp_buffer, 4, fpscr_value);
      regcache_raw_write (regcache, fpscr_base_regnum, temp_buffer);
      
      regcache_raw_read (regcache, sr_base_regnum, temp_buffer);
      old_sr_value = extract_unsigned_integer (temp_buffer, 4);
      old_sr_value &= 0xffff8fff;
      sr_value |= old_sr_value;
      store_unsigned_integer (temp_buffer, 4, sr_value);
      regcache_raw_write (regcache, sr_base_regnum, temp_buffer);
    }

  else if (reg_nr == tdep->FPUL_C_REGNUM)
    {
      base_regnum = sh64_compact_reg_base_num (reg_nr);
      regcache_raw_write (regcache, base_regnum, buffer);
    }
}

/* Floating point vector of 4 float registers. */
static void
do_fv_register_info (int fv_regnum)
{
  int first_fp_reg_num = fv_reg_base_num (fv_regnum);
  printf_filtered ("fv%d\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n", 
		     fv_regnum - gdbarch_tdep (current_gdbarch)->FV0_REGNUM, 
		     (int) read_register (first_fp_reg_num),
		     (int) read_register (first_fp_reg_num + 1),
		     (int) read_register (first_fp_reg_num + 2),
		     (int) read_register (first_fp_reg_num + 3));
}

/* Floating point vector of 4 float registers, compact mode. */
static void
do_fv_c_register_info (int fv_regnum)
{
  int first_fp_reg_num = sh64_compact_reg_base_num (fv_regnum);
  printf_filtered ("fv%d_c\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n", 
		     fv_regnum - gdbarch_tdep (current_gdbarch)->FV0_C_REGNUM, 
		     (int) read_register (first_fp_reg_num),
		     (int) read_register (first_fp_reg_num + 1),
		     (int) read_register (first_fp_reg_num + 2),
		     (int) read_register (first_fp_reg_num + 3));
}

/* Pairs of single regs. The DR are instead double precision
   registers. */
static void
do_fpp_register_info (int fpp_regnum)
{
  int first_fp_reg_num = fpp_reg_base_num (fpp_regnum);

  printf_filtered ("fpp%d\t0x%08x\t0x%08x\n", 
		    fpp_regnum - gdbarch_tdep (current_gdbarch)->FPP0_REGNUM, 
		    (int) read_register (first_fp_reg_num),
		    (int) read_register (first_fp_reg_num + 1));
}

/* Double precision registers. */
static void
do_dr_register_info (int dr_regnum)
{
  int first_fp_reg_num = dr_reg_base_num (dr_regnum);

  printf_filtered ("dr%d\t0x%08x%08x\n", 
		    dr_regnum - gdbarch_tdep (current_gdbarch)->DR0_REGNUM, 
		    (int) read_register (first_fp_reg_num),
		    (int) read_register (first_fp_reg_num + 1));
}

/* Double precision registers, compact mode. */
static void
do_dr_c_register_info (int dr_regnum)
{
 int first_fp_reg_num = sh64_compact_reg_base_num (dr_regnum);

 printf_filtered ("dr%d_c\t0x%08x%08x\n",
		  dr_regnum - gdbarch_tdep (current_gdbarch)->DR0_C_REGNUM,
		  (int) read_register (first_fp_reg_num),
		  (int) read_register (first_fp_reg_num +1));
}

/* General register in compact mode. */
static void
do_r_c_register_info (int r_c_regnum)
{
  int regnum =  sh64_compact_reg_base_num (r_c_regnum);

  printf_filtered ("r%d_c\t0x%08x\n", 
		    r_c_regnum - gdbarch_tdep (current_gdbarch)->R0_C_REGNUM, 
		   /*FIXME!!!*/  (int) read_register (regnum));
}

/* FIXME:!! THIS SHOULD TAKE CARE OF GETTING THE RIGHT PORTION OF THE
   shmedia REGISTERS. */
/* Control registers, compact mode. */
static void
do_cr_c_register_info (int cr_c_regnum)
{
  switch (cr_c_regnum)
    {
    case 237: printf_filtered ("pc_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 238: printf_filtered ("gbr_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 239: printf_filtered ("mach_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 240: printf_filtered ("macl_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 241: printf_filtered ("pr_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 242: printf_filtered ("t_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 243: printf_filtered ("fpscr_c\t0x%08x\n", (int) read_register (cr_c_regnum));
      break;
    case 244: printf_filtered ("fpul_c\t0x%08x\n", (int)read_register (cr_c_regnum));
      break;
    }
}

static void
sh_do_pseudo_register (int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (regnum < NUM_REGS || regnum >= NUM_REGS + NUM_PSEUDO_REGS)
    internal_error (__FILE__, __LINE__,
		    "Invalid pseudo register number %d\n", regnum);
  else if (regnum >= tdep->DR0_REGNUM
	   && regnum < tdep->DR_LAST_REGNUM)
    do_dr_register_info (regnum);
  else if (regnum >= tdep->FV0_REGNUM
	   && regnum <= tdep->FV_LAST_REGNUM)
    do_fv_register_info (regnum);
}

static void
sh_do_fp_register (int regnum)
{				/* do values for FP (float) regs */
  char *raw_buffer;
  double flt;	/* double extracted from raw hex data */
  int inv;
  int j;

  /* Allocate space for the float. */
  raw_buffer = (char *) alloca (REGISTER_RAW_SIZE (FP0_REGNUM));

  /* Get the data in raw format.  */
  if (!frame_register_read (selected_frame, regnum, raw_buffer))
    error ("can't read register %d (%s)", regnum, REGISTER_NAME (regnum));

  /* Get the register as a number */ 
  flt = unpack_double (builtin_type_float, raw_buffer, &inv);

  /* Print the name and some spaces. */
  fputs_filtered (REGISTER_NAME (regnum), gdb_stdout);
  print_spaces_filtered (15 - strlen (REGISTER_NAME (regnum)), gdb_stdout);

  /* Print the value. */
  if (inv)
    printf_filtered ("<invalid float>");
  else
    printf_filtered ("%-10.9g", flt);

  /* Print the fp register as hex. */
  printf_filtered ("\t(raw 0x");
  for (j = 0; j < REGISTER_RAW_SIZE (regnum); j++)
    {
      register int idx = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? j
	: REGISTER_RAW_SIZE (regnum) - 1 - j;
      printf_filtered ("%02x", (unsigned char) raw_buffer[idx]);
    }
  printf_filtered (")");
  printf_filtered ("\n");
}

static void
sh64_do_pseudo_register (int regnum)
{
  /* All the sh64-compact mode registers are pseudo registers. */
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  if (regnum < NUM_REGS 
      || regnum >= NUM_REGS + NUM_PSEUDO_REGS_SH_MEDIA + NUM_PSEUDO_REGS_SH_COMPACT)
    internal_error (__FILE__, __LINE__,
		    "Invalid pseudo register number %d\n", regnum);

  else if ((regnum >= tdep->DR0_REGNUM
	    && regnum <= tdep->DR_LAST_REGNUM))
    do_dr_register_info (regnum);

  else if ((regnum >= tdep->DR0_C_REGNUM
	    && regnum <= tdep->DR_LAST_C_REGNUM))
    do_dr_c_register_info (regnum);

  else if ((regnum >= tdep->FV0_REGNUM
	    && regnum <= tdep->FV_LAST_REGNUM))
    do_fv_register_info (regnum);
	   
  else if ((regnum >= tdep->FV0_C_REGNUM
	    && regnum <= tdep->FV_LAST_C_REGNUM))
    do_fv_c_register_info (regnum);

  else if (regnum >= tdep->FPP0_REGNUM
	   && regnum <= tdep->FPP_LAST_REGNUM)
    do_fpp_register_info (regnum);

  else if (regnum >= tdep->R0_C_REGNUM
	   && regnum <= tdep->R_LAST_C_REGNUM)
    do_r_c_register_info (regnum); /* FIXME, this function will not print the right format */

  else if (regnum >= tdep->FP0_C_REGNUM
	   && regnum <= tdep->FP_LAST_C_REGNUM)
    sh_do_fp_register (regnum); /* this should work also for pseudoregs */

  else if (regnum >= tdep->PC_C_REGNUM
	   && regnum <= tdep->FPUL_C_REGNUM)
    do_cr_c_register_info (regnum);

}

static void
sh_do_register (int regnum)
{
  char raw_buffer[MAX_REGISTER_RAW_SIZE];

  fputs_filtered (REGISTER_NAME (regnum), gdb_stdout);
  print_spaces_filtered (15 - strlen (REGISTER_NAME (regnum)), gdb_stdout);

  /* Get the data in raw format.  */
  if (!frame_register_read (selected_frame, regnum, raw_buffer))
    printf_filtered ("*value not available*\n");
      
  val_print (REGISTER_VIRTUAL_TYPE (regnum), raw_buffer, 0, 0,
	     gdb_stdout, 'x', 1, 0, Val_pretty_default);
  printf_filtered ("\t");
  val_print (REGISTER_VIRTUAL_TYPE (regnum), raw_buffer, 0, 0,
	     gdb_stdout, 0, 1, 0, Val_pretty_default);
  printf_filtered ("\n");
}

static void
sh_print_register (int regnum)
{
  if (regnum < 0 || regnum >= NUM_REGS + NUM_PSEUDO_REGS)
    internal_error (__FILE__, __LINE__,
		    "Invalid register number %d\n", regnum);

  else if (regnum >= 0 && regnum < NUM_REGS)
    {
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	sh_do_fp_register (regnum);	/* FP regs */
      else
	sh_do_register (regnum);	/* All other regs */
    }

  else if (regnum < NUM_REGS + NUM_PSEUDO_REGS)
    do_pseudo_register (regnum);
}

void
sh_do_registers_info (int regnum, int fpregs)
{
  if (regnum != -1)		/* do one specified register */
    {
      if (*(REGISTER_NAME (regnum)) == '\0')
	error ("Not a valid register for the current processor type");

      sh_print_register (regnum);
    }
  else
    /* do all (or most) registers */
    {
      regnum = 0;
      while (regnum < NUM_REGS)
	{
	  /* If the register name is empty, it is undefined for this
	     processor, so don't display anything.  */
	  if (REGISTER_NAME (regnum) == NULL
	      || *(REGISTER_NAME (regnum)) == '\0')
	    { 
	      regnum++;
	      continue;
	    }

	  if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	    {
	      if (fpregs)
		{
		  /* true for "INFO ALL-REGISTERS" command */
		  sh_do_fp_register (regnum);	/* FP regs */
		  regnum ++;
		}
	      else
		regnum += (gdbarch_tdep (current_gdbarch)->FP_LAST_REGNUM - FP0_REGNUM);	/* skip FP regs */
	    }
	  else
	    {
	      sh_do_register (regnum);	/* All other regs */
	      regnum++;
	    }
	}

      if (fpregs)
	while (regnum < NUM_REGS + NUM_PSEUDO_REGS)
	  {
	    do_pseudo_register (regnum);
	    regnum++;
	  }
    }
}

void
sh_compact_do_registers_info (int regnum, int fpregs)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 
  if (regnum != -1)		/* do one specified register */
    {
      if (*(REGISTER_NAME (regnum)) == '\0')
	error ("Not a valid register for the current processor type");

      if (regnum >= 0 && regnum < tdep->R0_C_REGNUM)
        error ("Not a valid register for the current processor mode.");

      sh_print_register (regnum);
    }
  else
    /* do all compact registers */
    {
      regnum = tdep->R0_C_REGNUM;
      while (regnum < NUM_REGS + NUM_PSEUDO_REGS)
        {
          do_pseudo_register (regnum);
          regnum++;
        }
    }
}

void
sh64_do_registers_info (int regnum, int fpregs)
{
  if (pc_is_isa32 (selected_frame->pc))
   sh_do_registers_info (regnum, fpregs);
  else
   sh_compact_do_registers_info (regnum, fpregs); 
}

#ifdef SVR4_SHARED_LIBS

/* Fetch (and possibly build) an appropriate link_map_offsets structure
   for native i386 linux targets using the struct offsets defined in
   link.h (but without actual reference to that file).

   This makes it possible to access i386-linux shared libraries from
   a gdb that was not built on an i386-linux host (for cross debugging).
   */

struct link_map_offsets *
sh_linux_svr4_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = 0;

  if (lmp == 0)
    {
      lmp = &lmo;

      lmo.r_debug_size = 8;	/* 20 not actual size but all we need */

      lmo.r_map_offset = 4;
      lmo.r_map_size   = 4;

      lmo.link_map_size = 20;	/* 552 not actual size but all we need */

      lmo.l_addr_offset = 0;
      lmo.l_addr_size   = 4;

      lmo.l_name_offset = 4;
      lmo.l_name_size   = 4;

      lmo.l_next_offset = 12;
      lmo.l_next_size   = 4;

      lmo.l_prev_offset = 16;
      lmo.l_prev_size   = 4;
    }

    return lmp;
}
#endif /* SVR4_SHARED_LIBS */


enum
{
   DSP_DSR_REGNUM = 24,
   DSP_A0G_REGNUM,
   DSP_A0_REGNUM,
   DSP_A1G_REGNUM,
   DSP_A1_REGNUM,
   DSP_M0_REGNUM,
   DSP_M1_REGNUM,
   DSP_X0_REGNUM,
   DSP_X1_REGNUM,
   DSP_Y0_REGNUM,
   DSP_Y1_REGNUM,
 
   DSP_MOD_REGNUM = 40,
 
   DSP_RS_REGNUM = 43,
   DSP_RE_REGNUM,
 
   DSP_R0_BANK_REGNUM = 51,
   DSP_R7_BANK_REGNUM = DSP_R0_BANK_REGNUM + 7
};

static int
sh_dsp_register_sim_regno (int nr)
{
  if (legacy_register_sim_regno (nr) < 0)
    return legacy_register_sim_regno (nr);
  if (nr >= DSP_DSR_REGNUM && nr < DSP_Y1_REGNUM)
    return nr - DSP_DSR_REGNUM + SIM_SH_DSR_REGNUM;
  if (nr == DSP_MOD_REGNUM)
    return SIM_SH_MOD_REGNUM;
  if (nr == DSP_RS_REGNUM)
    return SIM_SH_RS_REGNUM;
  if (nr == DSP_RE_REGNUM)
    return SIM_SH_RE_REGNUM;
  if (nr >= DSP_R0_BANK_REGNUM && nr <= DSP_R7_BANK_REGNUM)
    return nr - DSP_R0_BANK_REGNUM + SIM_SH_R0_BANK_REGNUM;
  return nr;
}

static gdbarch_init_ftype sh_gdbarch_init;

static struct gdbarch *
sh_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  static LONGEST sh_call_dummy_words[] = {0};
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  gdbarch_register_name_ftype *sh_register_name;
  gdbarch_deprecated_store_return_value_ftype *sh_store_return_value;
  gdbarch_register_virtual_type_ftype *sh_register_virtual_type;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  /* Try to determine the ABI of the object we are loading.  */

  if (info.abfd != NULL)
    {
      osabi = gdbarch_lookup_osabi (info.abfd);
      /* If we get "unknown" back, just leave it that way.  */
    }

  /* Find a candidate among the list of pre-declared architectures. */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* Make sure the ABI selection matches.  */
      tdep = gdbarch_tdep (arches->gdbarch);
      if (tdep && tdep->osabi == osabi)
	return arches->gdbarch;
    }

  /* None found, create a new architecture from the information
     provided. */
  tdep = XMALLOC (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  tdep->osabi = osabi;

  /* Initialize the register numbers that are not common to all the
     variants to -1, if necessary thse will be overwritten in the case
     statement below. */
  tdep->FPUL_REGNUM = -1;
  tdep->FPSCR_REGNUM = -1;
  tdep->PR_REGNUM = 17;
  tdep->SR_REGNUM = 22;
  tdep->DSR_REGNUM = -1;
  tdep->FP_LAST_REGNUM = -1;
  tdep->A0G_REGNUM = -1;
  tdep->A0_REGNUM = -1;
  tdep->A1G_REGNUM = -1;
  tdep->A1_REGNUM = -1;
  tdep->M0_REGNUM = -1;
  tdep->M1_REGNUM = -1;
  tdep->X0_REGNUM = -1;
  tdep->X1_REGNUM = -1;
  tdep->Y0_REGNUM = -1;
  tdep->Y1_REGNUM = -1;
  tdep->MOD_REGNUM = -1;
  tdep->RS_REGNUM = -1;
  tdep->RE_REGNUM = -1;
  tdep->SSR_REGNUM = -1;
  tdep->SPC_REGNUM = -1;
  tdep->DR0_REGNUM = -1;
  tdep->DR_LAST_REGNUM = -1;
  tdep->FV0_REGNUM = -1;
  tdep->FV_LAST_REGNUM = -1;
  tdep->ARG0_REGNUM = 4;
  tdep->ARGLAST_REGNUM = 7;
  tdep->RETURN_REGNUM = 0;
  tdep->FLOAT_ARGLAST_REGNUM = -1;

  tdep->sh_abi = SH_ABI_UNKNOWN;

  set_gdbarch_fp0_regnum (gdbarch, -1);
  set_gdbarch_num_pseudo_regs (gdbarch, 0);
  set_gdbarch_max_register_raw_size (gdbarch, 4);
  set_gdbarch_max_register_virtual_size (gdbarch, 4);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_num_regs (gdbarch, SH_DEFAULT_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, 15);
  set_gdbarch_fp_regnum (gdbarch, 14);
  set_gdbarch_pc_regnum (gdbarch, 16);
  set_gdbarch_register_size (gdbarch, 4);
  set_gdbarch_register_bytes (gdbarch, SH_DEFAULT_NUM_REGS * 4);
  set_gdbarch_do_registers_info (gdbarch, sh_do_registers_info);
  set_gdbarch_breakpoint_from_pc (gdbarch, sh_breakpoint_from_pc);
  set_gdbarch_frame_chain (gdbarch, sh_frame_chain);
  set_gdbarch_get_saved_register (gdbarch, generic_get_saved_register);
  set_gdbarch_init_extra_frame_info (gdbarch, sh_init_extra_frame_info);
  set_gdbarch_deprecated_extract_return_value (gdbarch, sh_extract_return_value);
  set_gdbarch_push_arguments (gdbarch, sh_push_arguments);
  set_gdbarch_store_struct_return (gdbarch, sh_store_struct_return);
  set_gdbarch_use_struct_convention (gdbarch, sh_use_struct_convention);
  set_gdbarch_deprecated_extract_struct_value_address (gdbarch, sh_extract_struct_value_address);
  set_gdbarch_pop_frame (gdbarch, sh_pop_frame);
  set_gdbarch_print_insn (gdbarch, gdb_print_insn_sh);
  set_gdbarch_register_sim_regno (gdbarch, legacy_register_sim_regno);
  skip_prologue_hard_way = sh_skip_prologue_hard_way;
  do_pseudo_register = sh_do_pseudo_register;

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_sh:
      sh_register_name = sh_sh_register_name;
      sh_show_regs = sh_generic_show_regs;
      sh_store_return_value = sh_default_store_return_value;
      sh_register_virtual_type = sh_default_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_nofp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      break;
    case bfd_mach_sh2:
      sh_register_name = sh_sh_register_name;
      sh_show_regs = sh_generic_show_regs;
      sh_store_return_value = sh_default_store_return_value;
      sh_register_virtual_type = sh_default_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_nofp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      break;      
    case bfd_mach_sh_dsp:
      sh_register_name = sh_sh_dsp_register_name;
      sh_show_regs = sh_dsp_show_regs;
      sh_store_return_value = sh_default_store_return_value;
      sh_register_virtual_type = sh_default_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_nofp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      set_gdbarch_register_sim_regno (gdbarch, sh_dsp_register_sim_regno);
      tdep->DSR_REGNUM = 24;
      tdep->A0G_REGNUM = 25;
      tdep->A0_REGNUM = 26;
      tdep->A1G_REGNUM = 27;
      tdep->A1_REGNUM = 28;
      tdep->M0_REGNUM = 29;
      tdep->M1_REGNUM = 30;
      tdep->X0_REGNUM = 31;
      tdep->X1_REGNUM = 32;
      tdep->Y0_REGNUM = 33;
      tdep->Y1_REGNUM = 34;
      tdep->MOD_REGNUM = 40;
      tdep->RS_REGNUM = 43;
      tdep->RE_REGNUM = 44;
      break;
    case bfd_mach_sh3:
      sh_register_name = sh_sh3_register_name;
      sh_show_regs = sh3_show_regs;
      sh_store_return_value = sh_default_store_return_value;
      sh_register_virtual_type = sh_default_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_nofp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      tdep->SSR_REGNUM = 41;
      tdep->SPC_REGNUM = 42;
      break;
    case bfd_mach_sh3e:
      sh_register_name = sh_sh3e_register_name;
      sh_show_regs = sh3e_show_regs;
      sh_store_return_value = sh3e_sh4_store_return_value;
      sh_register_virtual_type = sh_sh3e_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_fp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      set_gdbarch_deprecated_extract_return_value (gdbarch, sh3e_sh4_extract_return_value);
      set_gdbarch_fp0_regnum (gdbarch, 25);
      tdep->FPUL_REGNUM = 23;
      tdep->FPSCR_REGNUM = 24;
      tdep->FP_LAST_REGNUM = 40;
      tdep->SSR_REGNUM = 41;
      tdep->SPC_REGNUM = 42;
      break;
    case bfd_mach_sh3_dsp:
      sh_register_name = sh_sh3_dsp_register_name;
      sh_show_regs = sh3_dsp_show_regs;
      sh_store_return_value = sh_default_store_return_value;
      sh_register_virtual_type = sh_default_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_nofp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      tdep->DSR_REGNUM = 24;
      tdep->A0G_REGNUM = 25;
      tdep->A0_REGNUM = 26;
      tdep->A1G_REGNUM = 27;
      tdep->A1_REGNUM = 28;
      tdep->M0_REGNUM = 29;
      tdep->M1_REGNUM = 30;
      tdep->X0_REGNUM = 31;
      tdep->X1_REGNUM = 32;
      tdep->Y0_REGNUM = 33;
      tdep->Y1_REGNUM = 34;
      tdep->MOD_REGNUM = 40;
      tdep->RS_REGNUM = 43;
      tdep->RE_REGNUM = 44;
      tdep->SSR_REGNUM = 41;
      tdep->SPC_REGNUM = 42;
      break;
    case bfd_mach_sh4:
      sh_register_name = sh_sh4_register_name;
      sh_show_regs = sh4_show_regs;
      sh_store_return_value = sh3e_sh4_store_return_value;
      sh_register_virtual_type = sh_sh4_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_fp_frame_init_saved_regs);
      set_gdbarch_deprecated_extract_return_value (gdbarch, sh3e_sh4_extract_return_value);
      set_gdbarch_fp0_regnum (gdbarch, 25);
      set_gdbarch_register_raw_size (gdbarch, sh_sh4_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_sh4_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_sh4_register_byte);
      set_gdbarch_num_pseudo_regs (gdbarch, 12);
      set_gdbarch_max_register_raw_size (gdbarch, 4 * 4);
      set_gdbarch_max_register_virtual_size (gdbarch, 4 * 4);
      set_gdbarch_pseudo_register_read (gdbarch, sh_pseudo_register_read);
      set_gdbarch_pseudo_register_write (gdbarch, sh_pseudo_register_write);
      tdep->FPUL_REGNUM = 23;
      tdep->FPSCR_REGNUM = 24;
      tdep->FP_LAST_REGNUM = 40;
      tdep->SSR_REGNUM = 41;
      tdep->SPC_REGNUM = 42;
      tdep->DR0_REGNUM = 59;
      tdep->DR_LAST_REGNUM = 66;
      tdep->FV0_REGNUM = 67;
      tdep->FV_LAST_REGNUM = 70;
      break;
    case bfd_mach_sh5:
      tdep->PR_REGNUM = 18;
      tdep->SR_REGNUM = 65;
      tdep->FPSCR_REGNUM = SIM_SH64_FPCSR_REGNUM;
      tdep->FP_LAST_REGNUM = SIM_SH64_FR0_REGNUM + SIM_SH64_NR_FP_REGS - 1;
      tdep->SSR_REGNUM = SIM_SH64_SSR_REGNUM;
      tdep->SPC_REGNUM = SIM_SH64_SPC_REGNUM;
      tdep->TR7_REGNUM = SIM_SH64_TR0_REGNUM + 7;
      tdep->FPP0_REGNUM = 173;
      tdep->FPP_LAST_REGNUM = 204;
      tdep->DR0_REGNUM = 141;
      tdep->DR_LAST_REGNUM = 172;
      tdep->FV0_REGNUM = 205;
      tdep->FV_LAST_REGNUM = 220;
      tdep->R0_C_REGNUM = 221;
      tdep->R_LAST_C_REGNUM = 236;
      tdep->PC_C_REGNUM = 237; 
      tdep->GBR_C_REGNUM = 238;
      tdep->MACH_C_REGNUM = 239;
      tdep->MACL_C_REGNUM = 240;
      tdep->PR_C_REGNUM = 241;
      tdep->T_C_REGNUM = 242;
      tdep->FPSCR_C_REGNUM = 243;
      tdep->FPUL_C_REGNUM = 244;
      tdep->FP0_C_REGNUM = 245;
      tdep->FP_LAST_C_REGNUM = 260;
      tdep->DR0_C_REGNUM = 261;
      tdep->DR_LAST_C_REGNUM = 268;
      tdep->FV0_C_REGNUM = 269;
      tdep->FV_LAST_C_REGNUM = 272;
      tdep->ARG0_REGNUM = 2;
      tdep->ARGLAST_REGNUM = 9;
      tdep->RETURN_REGNUM = 2;
      tdep->FLOAT_ARGLAST_REGNUM = 11;

      set_gdbarch_num_pseudo_regs (gdbarch, NUM_PSEUDO_REGS_SH_MEDIA + NUM_PSEUDO_REGS_SH_COMPACT);
      set_gdbarch_fp0_regnum (gdbarch, SIM_SH64_FR0_REGNUM);
      set_gdbarch_pc_regnum (gdbarch, 64);

      /* Determine the ABI */
      if (bfd_get_arch_size (info.abfd) == 64)
	{
	  /* If the ABI is the 64-bit one, it can only be sh-media. */
	  tdep->sh_abi = SH_ABI_64;
	  set_gdbarch_ptr_bit (gdbarch, 8 * TARGET_CHAR_BIT);
	  set_gdbarch_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);
	}
      else
	{
	  /* If the ABI is the 32-bit one it could be either media or
             compact. */
	  tdep->sh_abi = SH_ABI_32;
	  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
	}

      /* the number of real registers is the same whether we are in 
	 ISA16(compact) or ISA32(media). */
      set_gdbarch_num_regs (gdbarch, SIM_SH64_NR_REGS);
      set_gdbarch_register_size (gdbarch, 8); /*????*/
      set_gdbarch_register_bytes (gdbarch, 
				  ((SIM_SH64_NR_FP_REGS + 1) * 4)
				  + (SIM_SH64_NR_REGS - SIM_SH64_NR_FP_REGS -1) * 8);

      sh_register_name = sh_sh64_register_name;
      sh_show_regs = sh64_show_regs;
      sh_register_virtual_type = sh_sh64_register_virtual_type;
      sh_store_return_value = sh64_store_return_value;
      skip_prologue_hard_way = sh64_skip_prologue_hard_way;
      do_pseudo_register = sh64_do_pseudo_register;
      set_gdbarch_register_raw_size (gdbarch, sh_sh64_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_sh64_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_sh64_register_byte);
      /* This seems awfully wrong!*/
      /*set_gdbarch_max_register_raw_size (gdbarch, 8);*/
      /* should include the size of the pseudo regs. */
      set_gdbarch_max_register_raw_size (gdbarch, 4 * 4);
      /* Or should that go in the virtual_size? */
      /*set_gdbarch_max_register_virtual_size (gdbarch, 8);*/
      set_gdbarch_max_register_virtual_size (gdbarch, 4 * 4);
      set_gdbarch_pseudo_register_read (gdbarch, sh64_pseudo_register_read);
      set_gdbarch_pseudo_register_write (gdbarch, sh64_pseudo_register_write);

      set_gdbarch_do_registers_info (gdbarch, sh64_do_registers_info);
      set_gdbarch_frame_init_saved_regs (gdbarch, sh64_nofp_frame_init_saved_regs);
      set_gdbarch_breakpoint_from_pc (gdbarch, sh_sh64_breakpoint_from_pc);
      set_gdbarch_init_extra_frame_info (gdbarch, sh64_init_extra_frame_info);
      set_gdbarch_frame_chain (gdbarch, sh64_frame_chain);
      set_gdbarch_get_saved_register (gdbarch, sh64_get_saved_register);
      set_gdbarch_deprecated_extract_return_value (gdbarch, sh64_extract_return_value);
      set_gdbarch_push_arguments (gdbarch, sh64_push_arguments);
      /*set_gdbarch_store_struct_return (gdbarch, sh64_store_struct_return);*/
      set_gdbarch_deprecated_extract_struct_value_address (gdbarch, sh64_extract_struct_value_address);
      set_gdbarch_use_struct_convention (gdbarch, sh64_use_struct_convention);
      set_gdbarch_pop_frame (gdbarch, sh64_pop_frame);
      set_gdbarch_elf_make_msymbol_special (gdbarch,
                                            sh64_elf_make_msymbol_special);
      break;
    default:
      sh_register_name = sh_generic_register_name;
      sh_show_regs = sh_generic_show_regs;
      sh_store_return_value = sh_default_store_return_value;
      sh_register_virtual_type = sh_default_register_virtual_type;
      set_gdbarch_frame_init_saved_regs (gdbarch, sh_nofp_frame_init_saved_regs);
      set_gdbarch_register_raw_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, sh_default_register_raw_size);
      set_gdbarch_register_byte (gdbarch, sh_default_register_byte);
      break;
    }

  set_gdbarch_read_pc (gdbarch, generic_target_read_pc);
  set_gdbarch_write_pc (gdbarch, generic_target_write_pc);
  set_gdbarch_read_fp (gdbarch, generic_target_read_fp);
  set_gdbarch_read_sp (gdbarch, generic_target_read_sp);
  set_gdbarch_write_sp (gdbarch, generic_target_write_sp);

  set_gdbarch_register_name (gdbarch, sh_register_name);
  set_gdbarch_register_virtual_type (gdbarch, sh_register_virtual_type);

  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 16 * TARGET_CHAR_BIT);/*??should be 8?*/

  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, entry_point_address);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1); /*???*/
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_pc_in_call_dummy (gdbarch, generic_pc_in_call_dummy);
  set_gdbarch_call_dummy_words (gdbarch, sh_call_dummy_words);
  set_gdbarch_sizeof_call_dummy_words (gdbarch, sizeof (sh_call_dummy_words));
  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);
  set_gdbarch_coerce_float_to_double (gdbarch, 
				      sh_coerce_float_to_double);

  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_push_return_address (gdbarch, sh_push_return_address);

  set_gdbarch_deprecated_store_return_value (gdbarch, sh_store_return_value);
  set_gdbarch_skip_prologue (gdbarch, sh_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);
  set_gdbarch_function_start_offset (gdbarch, 0);

  set_gdbarch_frame_args_skip (gdbarch, 0);
  set_gdbarch_frameless_function_invocation (gdbarch, frameless_look_for_prologue);
  set_gdbarch_frame_chain_valid (gdbarch, generic_file_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, sh_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, default_frame_address);
  set_gdbarch_frame_locals_address (gdbarch, default_frame_address);
  set_gdbarch_saved_pc_after_call (gdbarch, sh_saved_pc_after_call);
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);
  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  /* Hook in ABI-specific overrides, if they have been registered.

     FIXME: if the ABI is unknown, this is probably an embedded target,
     so we should not warn about this situation.  */
  gdbarch_init_osabi (info, gdbarch, osabi);

  return gdbarch;
}

static void
sh_dump_tdep (struct gdbarch *current_gdbarch, struct ui_file *file)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);

  if (tdep == NULL)
    return;

  fprintf_unfiltered (file, "sh_dump_tdep: OS ABI = %s\n",
                      gdbarch_osabi_name (tdep->osabi));
}

void
_initialize_sh_tdep (void)
{
  struct cmd_list_element *c;
  
  gdbarch_register (bfd_arch_sh, sh_gdbarch_init, sh_dump_tdep);

  add_com ("regs", class_vars, sh_show_regs_command, "Print all registers");
}
