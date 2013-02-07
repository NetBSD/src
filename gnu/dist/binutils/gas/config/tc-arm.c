/* tc-arm.c -- Assemble for the ARM
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005
   Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe@pegasus.esprit.ec.org)
	Modified by David Taylor (dtaylor@armltd.co.uk)
	Cirrus coprocessor mods by Aldy Hernandez (aldyh@redhat.com)
	Cirrus coprocessor fixes by Petko Manolov (petkan@nucleusys.com)
	Cirrus coprocessor fixes by Vladimir Ivanov (vladitx@nucleusys.com)

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include <string.h>
#define  NO_RELOC 0
#include "as.h"
#include "safe-ctype.h"

/* Need TARGET_CPU.  */
#include "config.h"
#include "subsegs.h"
#include "obstack.h"
#include "symbols.h"
#include "listing.h"

#include "opcode/arm.h"

#ifdef OBJ_ELF
#include "elf/arm.h"
#include "dwarf2dbg.h"
#include "dw2gencfi.h"
#endif

/* XXX Set this to 1 after the next binutils release.  */
#define WARN_DEPRECATED 0

#ifdef OBJ_ELF
/* Must be at least the size of the largest unwind opcode (currently two).  */
#define ARM_OPCODE_CHUNK_SIZE 8

/* This structure holds the unwinding state.  */

static struct
{
  symbolS *       proc_start;
  symbolS *       table_entry;
  symbolS *       personality_routine;
  int             personality_index;
  /* The segment containing the function.  */
  segT            saved_seg;
  subsegT         saved_subseg;
  /* Opcodes generated from this function.  */
  unsigned char * opcodes;
  int             opcode_count;
  int             opcode_alloc;
  /* The number of bytes pushed to the stack.  */
  offsetT         frame_size;
  /* We don't add stack adjustment opcodes immediately so that we can merge
     multiple adjustments.  We can also omit the final adjustment
     when using a frame pointer.  */
  offsetT         pending_offset;
  /* These two fields are set by both unwind_movsp and unwind_setfp.  They
     hold the reg+offset to use when restoring sp from a frame pointer.  */
  offsetT         fp_offset;
  int             fp_reg;
  /* Nonzero if an unwind_setfp directive has been seen.  */
  unsigned        fp_used:1;
  /* Nonzero if the last opcode restores sp from fp_reg.  */
  unsigned        sp_restored:1;
} unwind;

#endif /* OBJ_ELF */

enum arm_float_abi
{
  ARM_FLOAT_ABI_HARD,
  ARM_FLOAT_ABI_SOFTFP,
  ARM_FLOAT_ABI_SOFT
};

/* Types of processor to assemble for.  */
#define ARM_1		ARM_ARCH_V1
#define ARM_2		ARM_ARCH_V2
#define ARM_3		ARM_ARCH_V2S
#define ARM_250		ARM_ARCH_V2S
#define ARM_6		ARM_ARCH_V3
#define ARM_7		ARM_ARCH_V3
#define ARM_8		ARM_ARCH_V4
#define ARM_9		ARM_ARCH_V4T
#define ARM_STRONG	ARM_ARCH_V4
#define ARM_CPU_MASK	0x0000000f              /* XXX? */

#ifndef CPU_DEFAULT
#if defined __XSCALE__
#define CPU_DEFAULT	(ARM_ARCH_XSCALE)
#else
#if defined __thumb__
#define CPU_DEFAULT 	(ARM_ARCH_V5T)
#else
#define CPU_DEFAULT 	ARM_ANY
#endif
#endif
#endif

#ifndef FPU_DEFAULT
# ifdef TE_LINUX
#  define FPU_DEFAULT FPU_ARCH_FPA
# elif defined (TE_NetBSD)
#  ifdef OBJ_ELF
#   define FPU_DEFAULT FPU_ARCH_VFP	/* Soft-float, but VFP order.  */
#  else
    /* Legacy a.out format.  */
#   define FPU_DEFAULT FPU_ARCH_FPA	/* Soft-float, but FPA order.  */
#  endif
# elif defined (TE_VXWORKS)
#  define FPU_DEFAULT FPU_ARCH_VFP	/* Soft-float, VFP order.  */
# else
   /* For backwards compatibility, default to FPA.  */
#  define FPU_DEFAULT FPU_ARCH_FPA
# endif
#endif /* ifndef FPU_DEFAULT */

#define streq(a, b)           (strcmp (a, b) == 0)
#define skip_whitespace(str)  while (*(str) == ' ') ++(str)

static unsigned long cpu_variant;

/* Flags stored in private area of BFD structure.  */
static int uses_apcs_26      = FALSE;
static int atpcs             = FALSE;
static int support_interwork = FALSE;
static int uses_apcs_float   = FALSE;
static int pic_code          = FALSE;

/* Variables that we set while parsing command-line options.  Once all
   options have been read we re-process these values to set the real
   assembly flags.  */
static int legacy_cpu = -1;
static int legacy_fpu = -1;

static int mcpu_cpu_opt = -1;
static int mcpu_fpu_opt = -1;
static int march_cpu_opt = -1;
static int march_fpu_opt = -1;
static int mfpu_opt = -1;
static int mfloat_abi_opt = -1;
#ifdef OBJ_ELF
static int meabi_flags = EF_ARM_EABI_UNKNOWN;
#endif

/* This array holds the chars that always start a comment.  If the
   pre-processor is disabled, these aren't very useful.  */
const char comment_chars[] = "@";

/* This array holds the chars that only start a comment at the beginning of
   a line.  If the line seems to have the form '# 123 filename'
   .line and .file directives will appear in the pre-processed output.  */
/* Note that input_file.c hand checks for '#' at the beginning of the
   first line of the input file.  This is because the compiler outputs
   #NO_APP at the beginning of its output.  */
/* Also note that comments like this one will always work.  */
const char line_comment_chars[] = "#";

const char line_separator_chars[] = ";";

/* Chars that can be used to separate mant
   from exp in floating point numbers.  */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant.  */
/* As in 0f12.456  */
/* or    0d1.2345e12  */

const char FLT_CHARS[] = "rRsSfFdDxXeEpP";

/* Prefix characters that indicate the start of an immediate
   value.  */
#define is_immediate_prefix(C) ((C) == '#' || (C) == '$')

#ifdef OBJ_ELF
/* Pre-defined "_GLOBAL_OFFSET_TABLE_"  */
symbolS * GOT_symbol;
#endif

/* Size of relocation record.  */
const int md_reloc_size = 8;

/* 0: assemble for ARM,
   1: assemble for Thumb,
   2: assemble for Thumb even though target CPU does not support thumb
      instructions.  */
static int thumb_mode = 0;

typedef struct arm_fix
{
  int thumb_mode;
} arm_fix_data;

struct arm_it
{
  const char *  error;
  unsigned long instruction;
  int           size;
  struct
  {
    bfd_reloc_code_real_type type;
    expressionS              exp;
    int                      pc_rel;
  } reloc;
};

struct arm_it inst;

enum asm_shift_index
{
  SHIFT_LSL = 0,
  SHIFT_LSR,
  SHIFT_ASR,
  SHIFT_ROR,
  SHIFT_RRX
};

struct asm_shift_properties
{
  enum asm_shift_index index;
  unsigned long        bit_field;
  unsigned int         allows_0  : 1;
  unsigned int         allows_32 : 1;
};

static const struct asm_shift_properties shift_properties [] =
{
  { SHIFT_LSL, 0,    1, 0},
  { SHIFT_LSR, 0x20, 0, 1},
  { SHIFT_ASR, 0x40, 0, 1},
  { SHIFT_ROR, 0x60, 0, 0},
  { SHIFT_RRX, 0x60, 0, 0}
};

struct asm_shift_name
{
  const char *                        name;
  const struct asm_shift_properties * properties;
};

static const struct asm_shift_name shift_names [] =
{
  { "asl", shift_properties + SHIFT_LSL },
  { "lsl", shift_properties + SHIFT_LSL },
  { "lsr", shift_properties + SHIFT_LSR },
  { "asr", shift_properties + SHIFT_ASR },
  { "ror", shift_properties + SHIFT_ROR },
  { "rrx", shift_properties + SHIFT_RRX },
  { "ASL", shift_properties + SHIFT_LSL },
  { "LSL", shift_properties + SHIFT_LSL },
  { "LSR", shift_properties + SHIFT_LSR },
  { "ASR", shift_properties + SHIFT_ASR },
  { "ROR", shift_properties + SHIFT_ROR },
  { "RRX", shift_properties + SHIFT_RRX }
};

/* Any kind of shift is accepted.  */
#define NO_SHIFT_RESTRICT 1
/* The shift operand must be an immediate value, not a register.  */
#define SHIFT_IMMEDIATE	  0
/* The shift must be LSL or ASR and the operand must be an immediate.  */
#define SHIFT_LSL_OR_ASR_IMMEDIATE 2
/* The shift must be ASR and the operand must be an immediate.  */
#define SHIFT_ASR_IMMEDIATE 3
/* The shift must be LSL and the operand must be an immediate.  */
#define SHIFT_LSL_IMMEDIATE 4

#define NUM_FLOAT_VALS 8

const char * fp_const[] =
{
  "0.0", "1.0", "2.0", "3.0", "4.0", "5.0", "0.5", "10.0", 0
};

/* Number of littlenums required to hold an extended precision number.  */
#define MAX_LITTLENUMS 6

LITTLENUM_TYPE fp_values[NUM_FLOAT_VALS][MAX_LITTLENUMS];

#define FAIL	(-1)
#define SUCCESS (0)

/* Whether a Co-processor load/store operation accepts write-back forms.  */
#define CP_WB_OK 1
#define CP_NO_WB 0

#define SUFF_S 1
#define SUFF_D 2
#define SUFF_E 3
#define SUFF_P 4

#define CP_T_X   0x00008000
#define CP_T_Y   0x00400000
#define CP_T_Pre 0x01000000
#define CP_T_UD  0x00800000
#define CP_T_WB  0x00200000

#define CONDS_BIT        0x00100000
#define LOAD_BIT         0x00100000

#define DOUBLE_LOAD_FLAG 0x00000001

struct asm_cond
{
  const char *  template;
  unsigned long value;
};

#define COND_ALWAYS 0xe0000000
#define COND_MASK   0xf0000000

static const struct asm_cond conds[] =
{
  {"eq", 0x00000000},
  {"ne", 0x10000000},
  {"cs", 0x20000000}, {"hs", 0x20000000},
  {"cc", 0x30000000}, {"ul", 0x30000000}, {"lo", 0x30000000},
  {"mi", 0x40000000},
  {"pl", 0x50000000},
  {"vs", 0x60000000},
  {"vc", 0x70000000},
  {"hi", 0x80000000},
  {"ls", 0x90000000},
  {"ge", 0xa0000000},
  {"lt", 0xb0000000},
  {"gt", 0xc0000000},
  {"le", 0xd0000000},
  {"al", 0xe0000000},
  {"nv", 0xf0000000}
};

struct asm_psr
{
  const char *template;
  bfd_boolean cpsr;
  unsigned long field;
};

/* The bit that distinguishes CPSR and SPSR.  */
#define SPSR_BIT   (1 << 22)

/* How many bits to shift the PSR_xxx bits up by.  */
#define PSR_SHIFT  16

#define PSR_c   (1 << 0)
#define PSR_x   (1 << 1)
#define PSR_s   (1 << 2)
#define PSR_f   (1 << 3)

static const struct asm_psr psrs[] =
{
  {"CPSR",	TRUE,  PSR_c | PSR_f},
  {"CPSR_all",	TRUE,  PSR_c | PSR_f},
  {"SPSR",	FALSE, PSR_c | PSR_f},
  {"SPSR_all",	FALSE, PSR_c | PSR_f},
  {"CPSR_flg",	TRUE,  PSR_f},
  {"CPSR_f",    TRUE,  PSR_f},
  {"SPSR_flg",	FALSE, PSR_f},
  {"SPSR_f",    FALSE, PSR_f},
  {"CPSR_c",	TRUE,  PSR_c},
  {"CPSR_ctl",	TRUE,  PSR_c},
  {"SPSR_c",	FALSE, PSR_c},
  {"SPSR_ctl",	FALSE, PSR_c},
  {"CPSR_x",    TRUE,  PSR_x},
  {"CPSR_s",    TRUE,  PSR_s},
  {"SPSR_x",    FALSE, PSR_x},
  {"SPSR_s",    FALSE, PSR_s},
  /* Combinations of flags.  */
  {"CPSR_fs",	TRUE, PSR_f | PSR_s},
  {"CPSR_fx",	TRUE, PSR_f | PSR_x},
  {"CPSR_fc",	TRUE, PSR_f | PSR_c},
  {"CPSR_sf",	TRUE, PSR_s | PSR_f},
  {"CPSR_sx",	TRUE, PSR_s | PSR_x},
  {"CPSR_sc",	TRUE, PSR_s | PSR_c},
  {"CPSR_xf",	TRUE, PSR_x | PSR_f},
  {"CPSR_xs",	TRUE, PSR_x | PSR_s},
  {"CPSR_xc",	TRUE, PSR_x | PSR_c},
  {"CPSR_cf",	TRUE, PSR_c | PSR_f},
  {"CPSR_cs",	TRUE, PSR_c | PSR_s},
  {"CPSR_cx",	TRUE, PSR_c | PSR_x},
  {"CPSR_fsx",	TRUE, PSR_f | PSR_s | PSR_x},
  {"CPSR_fsc",	TRUE, PSR_f | PSR_s | PSR_c},
  {"CPSR_fxs",	TRUE, PSR_f | PSR_x | PSR_s},
  {"CPSR_fxc",	TRUE, PSR_f | PSR_x | PSR_c},
  {"CPSR_fcs",	TRUE, PSR_f | PSR_c | PSR_s},
  {"CPSR_fcx",	TRUE, PSR_f | PSR_c | PSR_x},
  {"CPSR_sfx",	TRUE, PSR_s | PSR_f | PSR_x},
  {"CPSR_sfc",	TRUE, PSR_s | PSR_f | PSR_c},
  {"CPSR_sxf",	TRUE, PSR_s | PSR_x | PSR_f},
  {"CPSR_sxc",	TRUE, PSR_s | PSR_x | PSR_c},
  {"CPSR_scf",	TRUE, PSR_s | PSR_c | PSR_f},
  {"CPSR_scx",	TRUE, PSR_s | PSR_c | PSR_x},
  {"CPSR_xfs",	TRUE, PSR_x | PSR_f | PSR_s},
  {"CPSR_xfc",	TRUE, PSR_x | PSR_f | PSR_c},
  {"CPSR_xsf",	TRUE, PSR_x | PSR_s | PSR_f},
  {"CPSR_xsc",	TRUE, PSR_x | PSR_s | PSR_c},
  {"CPSR_xcf",	TRUE, PSR_x | PSR_c | PSR_f},
  {"CPSR_xcs",	TRUE, PSR_x | PSR_c | PSR_s},
  {"CPSR_cfs",	TRUE, PSR_c | PSR_f | PSR_s},
  {"CPSR_cfx",	TRUE, PSR_c | PSR_f | PSR_x},
  {"CPSR_csf",	TRUE, PSR_c | PSR_s | PSR_f},
  {"CPSR_csx",	TRUE, PSR_c | PSR_s | PSR_x},
  {"CPSR_cxf",	TRUE, PSR_c | PSR_x | PSR_f},
  {"CPSR_cxs",	TRUE, PSR_c | PSR_x | PSR_s},
  {"CPSR_fsxc",	TRUE, PSR_f | PSR_s | PSR_x | PSR_c},
  {"CPSR_fscx",	TRUE, PSR_f | PSR_s | PSR_c | PSR_x},
  {"CPSR_fxsc",	TRUE, PSR_f | PSR_x | PSR_s | PSR_c},
  {"CPSR_fxcs",	TRUE, PSR_f | PSR_x | PSR_c | PSR_s},
  {"CPSR_fcsx",	TRUE, PSR_f | PSR_c | PSR_s | PSR_x},
  {"CPSR_fcxs",	TRUE, PSR_f | PSR_c | PSR_x | PSR_s},
  {"CPSR_sfxc",	TRUE, PSR_s | PSR_f | PSR_x | PSR_c},
  {"CPSR_sfcx",	TRUE, PSR_s | PSR_f | PSR_c | PSR_x},
  {"CPSR_sxfc",	TRUE, PSR_s | PSR_x | PSR_f | PSR_c},
  {"CPSR_sxcf",	TRUE, PSR_s | PSR_x | PSR_c | PSR_f},
  {"CPSR_scfx",	TRUE, PSR_s | PSR_c | PSR_f | PSR_x},
  {"CPSR_scxf",	TRUE, PSR_s | PSR_c | PSR_x | PSR_f},
  {"CPSR_xfsc",	TRUE, PSR_x | PSR_f | PSR_s | PSR_c},
  {"CPSR_xfcs",	TRUE, PSR_x | PSR_f | PSR_c | PSR_s},
  {"CPSR_xsfc",	TRUE, PSR_x | PSR_s | PSR_f | PSR_c},
  {"CPSR_xscf",	TRUE, PSR_x | PSR_s | PSR_c | PSR_f},
  {"CPSR_xcfs",	TRUE, PSR_x | PSR_c | PSR_f | PSR_s},
  {"CPSR_xcsf",	TRUE, PSR_x | PSR_c | PSR_s | PSR_f},
  {"CPSR_cfsx",	TRUE, PSR_c | PSR_f | PSR_s | PSR_x},
  {"CPSR_cfxs",	TRUE, PSR_c | PSR_f | PSR_x | PSR_s},
  {"CPSR_csfx",	TRUE, PSR_c | PSR_s | PSR_f | PSR_x},
  {"CPSR_csxf",	TRUE, PSR_c | PSR_s | PSR_x | PSR_f},
  {"CPSR_cxfs",	TRUE, PSR_c | PSR_x | PSR_f | PSR_s},
  {"CPSR_cxsf",	TRUE, PSR_c | PSR_x | PSR_s | PSR_f},
  {"SPSR_fs",	FALSE, PSR_f | PSR_s},
  {"SPSR_fx",	FALSE, PSR_f | PSR_x},
  {"SPSR_fc",	FALSE, PSR_f | PSR_c},
  {"SPSR_sf",	FALSE, PSR_s | PSR_f},
  {"SPSR_sx",	FALSE, PSR_s | PSR_x},
  {"SPSR_sc",	FALSE, PSR_s | PSR_c},
  {"SPSR_xf",	FALSE, PSR_x | PSR_f},
  {"SPSR_xs",	FALSE, PSR_x | PSR_s},
  {"SPSR_xc",	FALSE, PSR_x | PSR_c},
  {"SPSR_cf",	FALSE, PSR_c | PSR_f},
  {"SPSR_cs",	FALSE, PSR_c | PSR_s},
  {"SPSR_cx",	FALSE, PSR_c | PSR_x},
  {"SPSR_fsx",	FALSE, PSR_f | PSR_s | PSR_x},
  {"SPSR_fsc",	FALSE, PSR_f | PSR_s | PSR_c},
  {"SPSR_fxs",	FALSE, PSR_f | PSR_x | PSR_s},
  {"SPSR_fxc",	FALSE, PSR_f | PSR_x | PSR_c},
  {"SPSR_fcs",	FALSE, PSR_f | PSR_c | PSR_s},
  {"SPSR_fcx",	FALSE, PSR_f | PSR_c | PSR_x},
  {"SPSR_sfx",	FALSE, PSR_s | PSR_f | PSR_x},
  {"SPSR_sfc",	FALSE, PSR_s | PSR_f | PSR_c},
  {"SPSR_sxf",	FALSE, PSR_s | PSR_x | PSR_f},
  {"SPSR_sxc",	FALSE, PSR_s | PSR_x | PSR_c},
  {"SPSR_scf",	FALSE, PSR_s | PSR_c | PSR_f},
  {"SPSR_scx",	FALSE, PSR_s | PSR_c | PSR_x},
  {"SPSR_xfs",	FALSE, PSR_x | PSR_f | PSR_s},
  {"SPSR_xfc",	FALSE, PSR_x | PSR_f | PSR_c},
  {"SPSR_xsf",	FALSE, PSR_x | PSR_s | PSR_f},
  {"SPSR_xsc",	FALSE, PSR_x | PSR_s | PSR_c},
  {"SPSR_xcf",	FALSE, PSR_x | PSR_c | PSR_f},
  {"SPSR_xcs",	FALSE, PSR_x | PSR_c | PSR_s},
  {"SPSR_cfs",	FALSE, PSR_c | PSR_f | PSR_s},
  {"SPSR_cfx",	FALSE, PSR_c | PSR_f | PSR_x},
  {"SPSR_csf",	FALSE, PSR_c | PSR_s | PSR_f},
  {"SPSR_csx",	FALSE, PSR_c | PSR_s | PSR_x},
  {"SPSR_cxf",	FALSE, PSR_c | PSR_x | PSR_f},
  {"SPSR_cxs",	FALSE, PSR_c | PSR_x | PSR_s},
  {"SPSR_fsxc",	FALSE, PSR_f | PSR_s | PSR_x | PSR_c},
  {"SPSR_fscx",	FALSE, PSR_f | PSR_s | PSR_c | PSR_x},
  {"SPSR_fxsc",	FALSE, PSR_f | PSR_x | PSR_s | PSR_c},
  {"SPSR_fxcs",	FALSE, PSR_f | PSR_x | PSR_c | PSR_s},
  {"SPSR_fcsx",	FALSE, PSR_f | PSR_c | PSR_s | PSR_x},
  {"SPSR_fcxs",	FALSE, PSR_f | PSR_c | PSR_x | PSR_s},
  {"SPSR_sfxc",	FALSE, PSR_s | PSR_f | PSR_x | PSR_c},
  {"SPSR_sfcx",	FALSE, PSR_s | PSR_f | PSR_c | PSR_x},
  {"SPSR_sxfc",	FALSE, PSR_s | PSR_x | PSR_f | PSR_c},
  {"SPSR_sxcf",	FALSE, PSR_s | PSR_x | PSR_c | PSR_f},
  {"SPSR_scfx",	FALSE, PSR_s | PSR_c | PSR_f | PSR_x},
  {"SPSR_scxf",	FALSE, PSR_s | PSR_c | PSR_x | PSR_f},
  {"SPSR_xfsc",	FALSE, PSR_x | PSR_f | PSR_s | PSR_c},
  {"SPSR_xfcs",	FALSE, PSR_x | PSR_f | PSR_c | PSR_s},
  {"SPSR_xsfc",	FALSE, PSR_x | PSR_s | PSR_f | PSR_c},
  {"SPSR_xscf",	FALSE, PSR_x | PSR_s | PSR_c | PSR_f},
  {"SPSR_xcfs",	FALSE, PSR_x | PSR_c | PSR_f | PSR_s},
  {"SPSR_xcsf",	FALSE, PSR_x | PSR_c | PSR_s | PSR_f},
  {"SPSR_cfsx",	FALSE, PSR_c | PSR_f | PSR_s | PSR_x},
  {"SPSR_cfxs",	FALSE, PSR_c | PSR_f | PSR_x | PSR_s},
  {"SPSR_csfx",	FALSE, PSR_c | PSR_s | PSR_f | PSR_x},
  {"SPSR_csxf",	FALSE, PSR_c | PSR_s | PSR_x | PSR_f},
  {"SPSR_cxfs",	FALSE, PSR_c | PSR_x | PSR_f | PSR_s},
  {"SPSR_cxsf",	FALSE, PSR_c | PSR_x | PSR_s | PSR_f},
};

enum wreg_type
  {
    IWMMXT_REG_WR = 0,
    IWMMXT_REG_WC = 1,
    IWMMXT_REG_WR_OR_WC = 2,
    IWMMXT_REG_WCG
  };

enum iwmmxt_insn_type
{
  check_rd,
  check_wr,
  check_wrwr,
  check_wrwrwr,
  check_wrwrwcg,
  check_tbcst,
  check_tmovmsk,
  check_tmia,
  check_tmcrr,
  check_tmrrc,
  check_tmcr,
  check_tmrc,
  check_tinsr,
  check_textrc,
  check_waligni,
  check_textrm,
  check_wshufh
};

enum vfp_dp_reg_pos
{
  VFP_REG_Dd, VFP_REG_Dm, VFP_REG_Dn
};

enum vfp_sp_reg_pos
{
  VFP_REG_Sd, VFP_REG_Sm, VFP_REG_Sn
};

enum vfp_ldstm_type
{
  VFP_LDSTMIA, VFP_LDSTMDB, VFP_LDSTMIAX, VFP_LDSTMDBX
};

/* VFP system registers.  */
struct vfp_reg
{
  const char *name;
  unsigned long regno;
};

static const struct vfp_reg vfp_regs[] =
{
  {"fpsid", 0x00000000},
  {"FPSID", 0x00000000},
  {"fpscr", 0x00010000},
  {"FPSCR", 0x00010000},
  {"fpexc", 0x00080000},
  {"FPEXC", 0x00080000}
};

/* Structure for a hash table entry for a register.  */
struct reg_entry
{
  const char * name;
  int          number;
  bfd_boolean  builtin;
};

/* Some well known registers that we refer to directly elsewhere.  */
#define REG_SP  13
#define REG_LR  14
#define REG_PC	15

#define wr_register(reg)  ((reg ^ WR_PREFIX) >= 0 && (reg ^ WR_PREFIX) <= 15)
#define wc_register(reg)  ((reg ^ WC_PREFIX) >= 0 && (reg ^ WC_PREFIX) <= 15)
#define wcg_register(reg) ((reg ^ WC_PREFIX) >= 8 && (reg ^ WC_PREFIX) <= 11)

/* These are the standard names.  Users can add aliases with .req.
   and delete them with .unreq.  */

/* Integer Register Numbers.  */
static const struct reg_entry rn_table[] =
{
  {"r0",  0, TRUE},  {"r1",  1, TRUE},      {"r2",  2, TRUE},      {"r3",  3, TRUE},
  {"r4",  4, TRUE},  {"r5",  5, TRUE},      {"r6",  6, TRUE},      {"r7",  7, TRUE},
  {"r8",  8, TRUE},  {"r9",  9, TRUE},      {"r10", 10, TRUE},     {"r11", 11, TRUE},
  {"r12", 12, TRUE}, {"r13", REG_SP, TRUE}, {"r14", REG_LR, TRUE}, {"r15", REG_PC, TRUE},
  /* ATPCS Synonyms.  */
  {"a1",  0, TRUE},  {"a2",  1, TRUE},      {"a3",  2, TRUE},      {"a4",  3, TRUE},
  {"v1",  4, TRUE},  {"v2",  5, TRUE},      {"v3",  6, TRUE},      {"v4",  7, TRUE},
  {"v5",  8, TRUE},  {"v6",  9, TRUE},      {"v7",  10, TRUE},     {"v8",  11, TRUE},
  /* Well-known aliases.  */
  {"wr",  7, TRUE},  {"sb",  9, TRUE},      {"sl",  10, TRUE},     {"fp",  11, TRUE},
  {"ip",  12, TRUE}, {"sp",  REG_SP, TRUE}, {"lr",  REG_LR, TRUE}, {"pc",  REG_PC, TRUE},
  {NULL, 0, TRUE}
};

#define WR_PREFIX 0x200
#define WC_PREFIX 0x400

static const struct reg_entry iwmmxt_table[] =
{
  /* Intel Wireless MMX technology register names.  */
  {  "wr0", 0x0 | WR_PREFIX, TRUE},   {"wr1", 0x1 | WR_PREFIX, TRUE},
  {  "wr2", 0x2 | WR_PREFIX, TRUE},   {"wr3", 0x3 | WR_PREFIX, TRUE},
  {  "wr4", 0x4 | WR_PREFIX, TRUE},   {"wr5", 0x5 | WR_PREFIX, TRUE},
  {  "wr6", 0x6 | WR_PREFIX, TRUE},   {"wr7", 0x7 | WR_PREFIX, TRUE},
  {  "wr8", 0x8 | WR_PREFIX, TRUE},   {"wr9", 0x9 | WR_PREFIX, TRUE},
  { "wr10", 0xa | WR_PREFIX, TRUE},  {"wr11", 0xb | WR_PREFIX, TRUE},
  { "wr12", 0xc | WR_PREFIX, TRUE},  {"wr13", 0xd | WR_PREFIX, TRUE},
  { "wr14", 0xe | WR_PREFIX, TRUE},  {"wr15", 0xf | WR_PREFIX, TRUE},
  { "wcid", 0x0 | WC_PREFIX, TRUE},  {"wcon", 0x1 | WC_PREFIX, TRUE},
  {"wcssf", 0x2 | WC_PREFIX, TRUE}, {"wcasf", 0x3 | WC_PREFIX, TRUE},
  {"wcgr0", 0x8 | WC_PREFIX, TRUE}, {"wcgr1", 0x9 | WC_PREFIX, TRUE},
  {"wcgr2", 0xa | WC_PREFIX, TRUE}, {"wcgr3", 0xb | WC_PREFIX, TRUE},

  {  "wR0", 0x0 | WR_PREFIX, TRUE},   {"wR1", 0x1 | WR_PREFIX, TRUE},
  {  "wR2", 0x2 | WR_PREFIX, TRUE},   {"wR3", 0x3 | WR_PREFIX, TRUE},
  {  "wR4", 0x4 | WR_PREFIX, TRUE},   {"wR5", 0x5 | WR_PREFIX, TRUE},
  {  "wR6", 0x6 | WR_PREFIX, TRUE},   {"wR7", 0x7 | WR_PREFIX, TRUE},
  {  "wR8", 0x8 | WR_PREFIX, TRUE},   {"wR9", 0x9 | WR_PREFIX, TRUE},
  { "wR10", 0xa | WR_PREFIX, TRUE},  {"wR11", 0xb | WR_PREFIX, TRUE},
  { "wR12", 0xc | WR_PREFIX, TRUE},  {"wR13", 0xd | WR_PREFIX, TRUE},
  { "wR14", 0xe | WR_PREFIX, TRUE},  {"wR15", 0xf | WR_PREFIX, TRUE},
  { "wCID", 0x0 | WC_PREFIX, TRUE},  {"wCon", 0x1 | WC_PREFIX, TRUE},
  {"wCSSF", 0x2 | WC_PREFIX, TRUE}, {"wCASF", 0x3 | WC_PREFIX, TRUE},
  {"wCGR0", 0x8 | WC_PREFIX, TRUE}, {"wCGR1", 0x9 | WC_PREFIX, TRUE},
  {"wCGR2", 0xa | WC_PREFIX, TRUE}, {"wCGR3", 0xb | WC_PREFIX, TRUE},
  {NULL, 0, TRUE}
};

/* Co-processor Numbers.  */
static const struct reg_entry cp_table[] =
{
  {"p0",  0, TRUE},  {"p1",  1, TRUE},  {"p2",  2, TRUE},  {"p3", 3, TRUE},
  {"p4",  4, TRUE},  {"p5",  5, TRUE},  {"p6",  6, TRUE},  {"p7", 7, TRUE},
  {"p8",  8, TRUE},  {"p9",  9, TRUE},  {"p10", 10, TRUE}, {"p11", 11, TRUE},
  {"p12", 12, TRUE}, {"p13", 13, TRUE}, {"p14", 14, TRUE}, {"p15", 15, TRUE},
  {NULL, 0, TRUE}
};

/* Co-processor Register Numbers.  */
static const struct reg_entry cn_table[] =
{
  {"c0",   0, TRUE},  {"c1",   1, TRUE},  {"c2",   2, TRUE},  {"c3",   3, TRUE},
  {"c4",   4, TRUE},  {"c5",   5, TRUE},  {"c6",   6, TRUE},  {"c7",   7, TRUE},
  {"c8",   8, TRUE},  {"c9",   9, TRUE},  {"c10",  10, TRUE}, {"c11",  11, TRUE},
  {"c12",  12, TRUE}, {"c13",  13, TRUE}, {"c14",  14, TRUE}, {"c15",  15, TRUE},
  /* Not really valid, but kept for back-wards compatibility.  */
  {"cr0",  0, TRUE},  {"cr1",  1, TRUE},  {"cr2",  2, TRUE},  {"cr3",  3, TRUE},
  {"cr4",  4, TRUE},  {"cr5",  5, TRUE},  {"cr6",  6, TRUE},  {"cr7",  7, TRUE},
  {"cr8",  8, TRUE},  {"cr9",  9, TRUE},  {"cr10", 10, TRUE}, {"cr11", 11, TRUE},
  {"cr12", 12, TRUE}, {"cr13", 13, TRUE}, {"cr14", 14, TRUE}, {"cr15", 15, TRUE},
  {NULL, 0, TRUE}
};

/* FPA Registers.  */
static const struct reg_entry fn_table[] =
{
  {"f0", 0, TRUE},   {"f1", 1, TRUE},   {"f2", 2, TRUE},   {"f3", 3, TRUE},
  {"f4", 4, TRUE},   {"f5", 5, TRUE},   {"f6", 6, TRUE},   {"f7", 7, TRUE},
  {NULL, 0, TRUE}
};

/* VFP SP Registers.  */
static const struct reg_entry sn_table[] =
{
  {"s0",  0, TRUE},  {"s1",  1, TRUE},  {"s2",  2, TRUE},  {"s3", 3, TRUE},
  {"s4",  4, TRUE},  {"s5",  5, TRUE},  {"s6",  6, TRUE},  {"s7", 7, TRUE},
  {"s8",  8, TRUE},  {"s9",  9, TRUE},  {"s10", 10, TRUE}, {"s11", 11, TRUE},
  {"s12", 12, TRUE}, {"s13", 13, TRUE}, {"s14", 14, TRUE}, {"s15", 15, TRUE},
  {"s16", 16, TRUE}, {"s17", 17, TRUE}, {"s18", 18, TRUE}, {"s19", 19, TRUE},
  {"s20", 20, TRUE}, {"s21", 21, TRUE}, {"s22", 22, TRUE}, {"s23", 23, TRUE},
  {"s24", 24, TRUE}, {"s25", 25, TRUE}, {"s26", 26, TRUE}, {"s27", 27, TRUE},
  {"s28", 28, TRUE}, {"s29", 29, TRUE}, {"s30", 30, TRUE}, {"s31", 31, TRUE},
  {NULL, 0, TRUE}
};

/* VFP DP Registers.  */
static const struct reg_entry dn_table[] =
{
  {"d0",  0, TRUE},  {"d1",  1, TRUE},  {"d2",  2, TRUE},  {"d3", 3, TRUE},
  {"d4",  4, TRUE},  {"d5",  5, TRUE},  {"d6",  6, TRUE},  {"d7", 7, TRUE},
  {"d8",  8, TRUE},  {"d9",  9, TRUE},  {"d10", 10, TRUE}, {"d11", 11, TRUE},
  {"d12", 12, TRUE}, {"d13", 13, TRUE}, {"d14", 14, TRUE}, {"d15", 15, TRUE},
  {NULL, 0, TRUE}
};

/* Maverick DSP coprocessor registers.  */
static const struct reg_entry mav_mvf_table[] =
{
  {"mvf0",  0, TRUE},  {"mvf1",  1, TRUE},  {"mvf2",  2, TRUE},  {"mvf3",  3, TRUE},
  {"mvf4",  4, TRUE},  {"mvf5",  5, TRUE},  {"mvf6",  6, TRUE},  {"mvf7",  7, TRUE},
  {"mvf8",  8, TRUE},  {"mvf9",  9, TRUE},  {"mvf10", 10, TRUE}, {"mvf11", 11, TRUE},
  {"mvf12", 12, TRUE}, {"mvf13", 13, TRUE}, {"mvf14", 14, TRUE}, {"mvf15", 15, TRUE},
  {NULL, 0, TRUE}
};

static const struct reg_entry mav_mvd_table[] =
{
  {"mvd0",  0, TRUE},  {"mvd1",  1, TRUE},  {"mvd2",  2, TRUE},  {"mvd3",  3, TRUE},
  {"mvd4",  4, TRUE},  {"mvd5",  5, TRUE},  {"mvd6",  6, TRUE},  {"mvd7",  7, TRUE},
  {"mvd8",  8, TRUE},  {"mvd9",  9, TRUE},  {"mvd10", 10, TRUE}, {"mvd11", 11, TRUE},
  {"mvd12", 12, TRUE}, {"mvd13", 13, TRUE}, {"mvd14", 14, TRUE}, {"mvd15", 15, TRUE},
  {NULL, 0, TRUE}
};

static const struct reg_entry mav_mvfx_table[] =
{
  {"mvfx0",  0, TRUE},  {"mvfx1",  1, TRUE},  {"mvfx2",  2, TRUE},  {"mvfx3",  3, TRUE},
  {"mvfx4",  4, TRUE},  {"mvfx5",  5, TRUE},  {"mvfx6",  6, TRUE},  {"mvfx7",  7, TRUE},
  {"mvfx8",  8, TRUE},  {"mvfx9",  9, TRUE},  {"mvfx10", 10, TRUE}, {"mvfx11", 11, TRUE},
  {"mvfx12", 12, TRUE}, {"mvfx13", 13, TRUE}, {"mvfx14", 14, TRUE}, {"mvfx15", 15, TRUE},
  {NULL, 0, TRUE}
};

static const struct reg_entry mav_mvdx_table[] =
{
  {"mvdx0",  0, TRUE},  {"mvdx1",  1, TRUE},  {"mvdx2",  2, TRUE},  {"mvdx3",  3, TRUE},
  {"mvdx4",  4, TRUE},  {"mvdx5",  5, TRUE},  {"mvdx6",  6, TRUE},  {"mvdx7",  7, TRUE},
  {"mvdx8",  8, TRUE},  {"mvdx9",  9, TRUE},  {"mvdx10", 10, TRUE}, {"mvdx11", 11, TRUE},
  {"mvdx12", 12, TRUE}, {"mvdx13", 13, TRUE}, {"mvdx14", 14, TRUE}, {"mvdx15", 15, TRUE},
  {NULL, 0, TRUE}
};

static const struct reg_entry mav_mvax_table[] =
{
  {"mvax0", 0, TRUE}, {"mvax1", 1, TRUE}, {"mvax2", 2, TRUE}, {"mvax3", 3, TRUE},
  {NULL, 0, TRUE}
};

static const struct reg_entry mav_dspsc_table[] =
{
  {"dspsc", 0, TRUE},
  {NULL, 0, TRUE}
};

struct reg_map
{
  const struct reg_entry * names;
  int                      max_regno;
  struct hash_control *    htab;
  const char *             expected;
};

struct reg_map all_reg_maps[] =
{
  {rn_table,        15, NULL, N_("ARM register expected")},
  {cp_table,        15, NULL, N_("bad or missing co-processor number")},
  {cn_table,        15, NULL, N_("co-processor register expected")},
  {fn_table,         7, NULL, N_("FPA register expected")},
  {sn_table,	    31, NULL, N_("VFP single precision register expected")},
  {dn_table,	    15, NULL, N_("VFP double precision register expected")},
  {mav_mvf_table,   15, NULL, N_("Maverick MVF register expected")},
  {mav_mvd_table,   15, NULL, N_("Maverick MVD register expected")},
  {mav_mvfx_table,  15, NULL, N_("Maverick MVFX register expected")},
  {mav_mvdx_table,  15, NULL, N_("Maverick MVDX register expected")},
  {mav_mvax_table,   3, NULL, N_("Maverick MVAX register expected")},
  {mav_dspsc_table,  0, NULL, N_("Maverick DSPSC register expected")},
  {iwmmxt_table,    23, NULL, N_("Intel Wireless MMX technology register expected")},
};

/* Enumeration matching entries in table above.  */
enum arm_reg_type
{
  REG_TYPE_RN = 0,
#define REG_TYPE_FIRST REG_TYPE_RN
  REG_TYPE_CP = 1,
  REG_TYPE_CN = 2,
  REG_TYPE_FN = 3,
  REG_TYPE_SN = 4,
  REG_TYPE_DN = 5,
  REG_TYPE_MVF = 6,
  REG_TYPE_MVD = 7,
  REG_TYPE_MVFX = 8,
  REG_TYPE_MVDX = 9,
  REG_TYPE_MVAX = 10,
  REG_TYPE_DSPSC = 11,
  REG_TYPE_IWMMXT = 12,

  REG_TYPE_MAX = 13
};

/* ARM instructions take 4bytes in the object file, Thumb instructions
   take 2:  */
#define INSN_SIZE       4

/* "INSN<cond> X,Y" where X:bit12, Y:bit16.  */
#define MAV_MODE1	0x100c

/* "INSN<cond> X,Y" where X:bit16, Y:bit12.  */
#define MAV_MODE2	0x0c10

/* "INSN<cond> X,Y" where X:bit12, Y:bit16.  */
#define MAV_MODE3	0x100c

/* "INSN<cond> X,Y,Z" where X:16, Y:0, Z:12.  */
#define MAV_MODE4	0x0c0010

/* "INSN<cond> X,Y,Z" where X:12, Y:16, Z:0.  */
#define MAV_MODE5	0x00100c

/* "INSN<cond> W,X,Y,Z" where W:5, X:12, Y:16, Z:0.  */
#define MAV_MODE6	0x00100c05

struct asm_opcode
{
  /* Basic string to match.  */
  const char * template;

  /* Basic instruction code.  */
  unsigned long value;

  /* Offset into the template where the condition code (if any) will be.
     If zero, then the instruction is never conditional.  */
  unsigned cond_offset;

  /* Which architecture variant provides this instruction.  */
  unsigned long variant;

  /* Function to call to parse args.  */
  void (* parms) (char *);
};

/* Defines for various bits that we will want to toggle.  */
#define INST_IMMEDIATE	0x02000000
#define OFFSET_REG	0x02000000
#define HWOFFSET_IMM    0x00400000
#define SHIFT_BY_REG	0x00000010
#define PRE_INDEX	0x01000000
#define INDEX_UP	0x00800000
#define WRITE_BACK	0x00200000
#define LDM_TYPE_2_OR_3	0x00400000

#define LITERAL_MASK	0xf000f000
#define OPCODE_MASK	0xfe1fffff
#define V4_STR_BIT	0x00000020

#define DATA_OP_SHIFT	21

/* Codes to distinguish the arithmetic instructions.  */
#define OPCODE_AND	0
#define OPCODE_EOR	1
#define OPCODE_SUB	2
#define OPCODE_RSB	3
#define OPCODE_ADD	4
#define OPCODE_ADC	5
#define OPCODE_SBC	6
#define OPCODE_RSC	7
#define OPCODE_TST	8
#define OPCODE_TEQ	9
#define OPCODE_CMP	10
#define OPCODE_CMN	11
#define OPCODE_ORR	12
#define OPCODE_MOV	13
#define OPCODE_BIC	14
#define OPCODE_MVN	15

#define T_OPCODE_MUL 0x4340
#define T_OPCODE_TST 0x4200
#define T_OPCODE_CMN 0x42c0
#define T_OPCODE_NEG 0x4240
#define T_OPCODE_MVN 0x43c0

#define T_OPCODE_ADD_R3	0x1800
#define T_OPCODE_SUB_R3 0x1a00
#define T_OPCODE_ADD_HI 0x4400
#define T_OPCODE_ADD_ST 0xb000
#define T_OPCODE_SUB_ST 0xb080
#define T_OPCODE_ADD_SP 0xa800
#define T_OPCODE_ADD_PC 0xa000
#define T_OPCODE_ADD_I8 0x3000
#define T_OPCODE_SUB_I8 0x3800
#define T_OPCODE_ADD_I3 0x1c00
#define T_OPCODE_SUB_I3 0x1e00

#define T_OPCODE_ASR_R	0x4100
#define T_OPCODE_LSL_R	0x4080
#define T_OPCODE_LSR_R  0x40c0
#define T_OPCODE_ASR_I	0x1000
#define T_OPCODE_LSL_I	0x0000
#define T_OPCODE_LSR_I	0x0800

#define T_OPCODE_MOV_I8	0x2000
#define T_OPCODE_CMP_I8 0x2800
#define T_OPCODE_CMP_LR 0x4280
#define T_OPCODE_MOV_HR 0x4600
#define T_OPCODE_CMP_HR 0x4500

#define T_OPCODE_LDR_PC 0x4800
#define T_OPCODE_LDR_SP 0x9800
#define T_OPCODE_STR_SP 0x9000
#define T_OPCODE_LDR_IW 0x6800
#define T_OPCODE_STR_IW 0x6000
#define T_OPCODE_LDR_IH 0x8800
#define T_OPCODE_STR_IH 0x8000
#define T_OPCODE_LDR_IB 0x7800
#define T_OPCODE_STR_IB 0x7000
#define T_OPCODE_LDR_RW 0x5800
#define T_OPCODE_STR_RW 0x5000
#define T_OPCODE_LDR_RH 0x5a00
#define T_OPCODE_STR_RH 0x5200
#define T_OPCODE_LDR_RB 0x5c00
#define T_OPCODE_STR_RB 0x5400

#define T_OPCODE_PUSH	0xb400
#define T_OPCODE_POP	0xbc00

#define T_OPCODE_BRANCH 0xe7fe

#define THUMB_SIZE	2	/* Size of thumb instruction.  */
#define THUMB_REG_LO	0x1
#define THUMB_REG_HI	0x2
#define THUMB_REG_ANY	0x3

#define THUMB_H1	0x0080
#define THUMB_H2	0x0040

#define THUMB_ASR 0
#define THUMB_LSL 1
#define THUMB_LSR 2

#define THUMB_MOVE 0
#define THUMB_COMPARE 1
#define THUMB_CPY 2

#define THUMB_LOAD 0
#define THUMB_STORE 1

#define THUMB_PP_PC_LR 0x0100

/* These three are used for immediate shifts, do not alter.  */
#define THUMB_WORD 2
#define THUMB_HALFWORD 1
#define THUMB_BYTE 0

struct thumb_opcode
{
  /* Basic string to match.  */
  const char * template;

  /* Basic instruction code.  */
  unsigned long value;

  int size;

  /* Which CPU variants this exists for.  */
  unsigned long variant;

  /* Function to call to parse args.  */
  void (* parms) (char *);
};

#define BAD_ARGS 	_("bad arguments to instruction")
#define BAD_PC 		_("r15 not allowed here")
#define BAD_COND 	_("instruction is not conditional")
#define ERR_NO_ACCUM	_("acc0 expected")

static struct hash_control * arm_ops_hsh   = NULL;
static struct hash_control * arm_tops_hsh  = NULL;
static struct hash_control * arm_cond_hsh  = NULL;
static struct hash_control * arm_shift_hsh = NULL;
static struct hash_control * arm_psr_hsh   = NULL;

/* Stuff needed to resolve the label ambiguity
   As:
     ...
     label:   <insn>
   may differ from:
     ...
     label:
              <insn>
*/

symbolS *  last_label_seen;
static int label_is_thumb_function_name = FALSE;

/* Literal Pool stuff.  */

#define MAX_LITERAL_POOL_SIZE 1024

/* Literal pool structure.  Held on a per-section
   and per-sub-section basis.  */

typedef struct literal_pool
{
  expressionS    literals [MAX_LITERAL_POOL_SIZE];
  unsigned int   next_free_entry;
  unsigned int   id;
  symbolS *      symbol;
  segT           section;
  subsegT        sub_section;
  struct literal_pool * next;
} literal_pool;

/* Pointer to a linked list of literal pools.  */
literal_pool * list_of_pools = NULL;

static literal_pool *
find_literal_pool (void)
{
  literal_pool * pool;

  for (pool = list_of_pools; pool != NULL; pool = pool->next)
    {
      if (pool->section == now_seg
	  && pool->sub_section == now_subseg)
	break;
    }

  return pool;
}

static literal_pool *
find_or_make_literal_pool (void)
{
  /* Next literal pool ID number.  */
  static unsigned int latest_pool_num = 1;
  literal_pool *      pool;

  pool = find_literal_pool ();

  if (pool == NULL)
    {
      /* Create a new pool.  */
      pool = xmalloc (sizeof (* pool));
      if (! pool)
	return NULL;

      pool->next_free_entry = 0;
      pool->section         = now_seg;
      pool->sub_section     = now_subseg;
      pool->next            = list_of_pools;
      pool->symbol          = NULL;

      /* Add it to the list.  */
      list_of_pools = pool;
    }

  /* New pools, and emptied pools, will have a NULL symbol.  */
  if (pool->symbol == NULL)
    {
      pool->symbol = symbol_create (FAKE_LABEL_NAME, undefined_section,
				    (valueT) 0, &zero_address_frag);
      pool->id = latest_pool_num ++;
    }

  /* Done.  */
  return pool;
}

/* Add the literal in the global 'inst'
   structure to the relevent literal pool.  */

static int
add_to_lit_pool (void)
{
  literal_pool * pool;
  unsigned int entry;

  pool = find_or_make_literal_pool ();

  /* Check if this literal value is already in the pool.  */
  for (entry = 0; entry < pool->next_free_entry; entry ++)
    {
      if ((pool->literals[entry].X_op == inst.reloc.exp.X_op)
	  && (inst.reloc.exp.X_op == O_constant)
	  && (pool->literals[entry].X_add_number
	      == inst.reloc.exp.X_add_number)
	  && (pool->literals[entry].X_unsigned
	      == inst.reloc.exp.X_unsigned))
	break;

      if ((pool->literals[entry].X_op == inst.reloc.exp.X_op)
          && (inst.reloc.exp.X_op == O_symbol)
          && (pool->literals[entry].X_add_number
	      == inst.reloc.exp.X_add_number)
          && (pool->literals[entry].X_add_symbol
	      == inst.reloc.exp.X_add_symbol)
          && (pool->literals[entry].X_op_symbol
	      == inst.reloc.exp.X_op_symbol))
        break;
    }

  /* Do we need to create a new entry?  */
  if (entry == pool->next_free_entry)
    {
      if (entry >= MAX_LITERAL_POOL_SIZE)
	{
	  inst.error = _("literal pool overflow");
	  return FAIL;
	}

      pool->literals[entry] = inst.reloc.exp;
      pool->next_free_entry += 1;
    }

  inst.reloc.exp.X_op         = O_symbol;
  inst.reloc.exp.X_add_number = ((int) entry) * 4 - 8;
  inst.reloc.exp.X_add_symbol = pool->symbol;

  return SUCCESS;
}

/* Can't use symbol_new here, so have to create a symbol and then at
   a later date assign it a value. Thats what these functions do.  */

static void
symbol_locate (symbolS *    symbolP,
	       const char * name,	/* It is copied, the caller can modify.  */
	       segT         segment,	/* Segment identifier (SEG_<something>).  */
	       valueT       valu,	/* Symbol value.  */
	       fragS *      frag)	/* Associated fragment.  */
{
  unsigned int name_length;
  char * preserved_copy_of_name;

  name_length = strlen (name) + 1;   /* +1 for \0.  */
  obstack_grow (&notes, name, name_length);
  preserved_copy_of_name = obstack_finish (&notes);
#ifdef STRIP_UNDERSCORE
  if (preserved_copy_of_name[0] == '_')
    preserved_copy_of_name++;
#endif

#ifdef tc_canonicalize_symbol_name
  preserved_copy_of_name =
    tc_canonicalize_symbol_name (preserved_copy_of_name);
#endif

  S_SET_NAME (symbolP, preserved_copy_of_name);

  S_SET_SEGMENT (symbolP, segment);
  S_SET_VALUE (symbolP, valu);
  symbol_clear_list_pointers (symbolP);

  symbol_set_frag (symbolP, frag);

  /* Link to end of symbol chain.  */
  {
    extern int symbol_table_frozen;

    if (symbol_table_frozen)
      abort ();
  }

  symbol_append (symbolP, symbol_lastP, & symbol_rootP, & symbol_lastP);

  obj_symbol_new_hook (symbolP);

#ifdef tc_symbol_new_hook
  tc_symbol_new_hook (symbolP);
#endif

#ifdef DEBUG_SYMS
  verify_symbol_chain (symbol_rootP, symbol_lastP);
#endif /* DEBUG_SYMS  */
}

/* Check that an immediate is valid.
   If so, convert it to the right format.  */

static unsigned int
validate_immediate (unsigned int val)
{
  unsigned int a;
  unsigned int i;

#define rotate_left(v, n) (v << n | v >> (32 - n))

  for (i = 0; i < 32; i += 2)
    if ((a = rotate_left (val, i)) <= 0xff)
      return a | (i << 7); /* 12-bit pack: [shift-cnt,const].  */

  return FAIL;
}

/* Check to see if an immediate can be computed as two separate immediate
   values, added together.  We already know that this value cannot be
   computed by just one ARM instruction.  */

static unsigned int
validate_immediate_twopart (unsigned int   val,
			    unsigned int * highpart)
{
  unsigned int a;
  unsigned int i;

  for (i = 0; i < 32; i += 2)
    if (((a = rotate_left (val, i)) & 0xff) != 0)
      {
	if (a & 0xff00)
	  {
	    if (a & ~ 0xffff)
	      continue;
	    * highpart = (a  >> 8) | ((i + 24) << 7);
	  }
	else if (a & 0xff0000)
	  {
	    if (a & 0xff000000)
	      continue;
	    * highpart = (a >> 16) | ((i + 16) << 7);
	  }
	else
	  {
	    assert (a & 0xff000000);
	    * highpart = (a >> 24) | ((i + 8) << 7);
	  }

	return (a & 0xff) | (i << 7);
      }

  return FAIL;
}

static int
validate_offset_imm (unsigned int val, int hwse)
{
  if ((hwse && val > 255) || val > 4095)
    return FAIL;
  return val;
}


#ifdef OBJ_ELF
/* This code is to handle mapping symbols as defined in the ARM ELF spec.
   (This text is taken from version B-02 of the spec):

      4.4.7 Mapping and tagging symbols

      A section of an ARM ELF file can contain a mixture of ARM code,
      Thumb code, and data.  There are inline transitions between code
      and data at literal pool boundaries. There can also be inline
      transitions between ARM code and Thumb code, for example in
      ARM-Thumb inter-working veneers.  Linkers, machine-level
      debuggers, profiling tools, and disassembly tools need to map
      images accurately. For example, setting an ARM breakpoint on a
      Thumb location, or in a literal pool, can crash the program
      being debugged, ruining the debugging session.

      ARM ELF entities are mapped (see section 4.4.7.1 below) and
      tagged (see section 4.4.7.2 below) using local symbols (with
      binding STB_LOCAL).  To assist consumers, mapping and tagging
      symbols should be collated first in the symbol table, before
      other symbols with binding STB_LOCAL.

      To allow properly collated mapping and tagging symbols to be
      skipped by consumers that have no interest in them, the first
      such symbol should have the name $m and its st_value field equal
      to the total number of mapping and tagging symbols (including
      the $m) in the symbol table.

      4.4.7.1 Mapping symbols

      $a    Labels the first byte of a sequence of ARM instructions.
            Its type is STT_FUNC.

      $d    Labels the first byte of a sequence of data items.
            Its type is STT_OBJECT.

      $t    Labels the first byte of a sequence of Thumb instructions.
            Its type is STT_FUNC.

      This list of mapping symbols may be extended in the future.

      Section-relative mapping symbols

      Mapping symbols defined in a section define a sequence of
      half-open address intervals that cover the address range of the
      section. Each interval starts at the address defined by a
      mapping symbol, and continues up to, but not including, the
      address defined by the next (in address order) mapping symbol or
      the end of the section. A corollary is that there must be a
      mapping symbol defined at the beginning of each section.
      Consumers can ignore the size of a section-relative mapping
      symbol. Producers can set it to 0.

      Absolute mapping symbols

      Because of the need to crystallize a Thumb address with the
      Thumb-bit set, absolute symbol of type STT_FUNC (symbols of type
      STT_FUNC defined in section SHN_ABS) need to be mapped with $a
      or $t.

      The extent of a mapping symbol defined in SHN_ABS is [st_value,
      st_value + st_size), or [st_value, st_value + 1) if st_size = 0,
      where [x, y) denotes the half-open address range from x,
      inclusive, to y, exclusive.

      In the absence of a mapping symbol, a consumer can interpret a
      function symbol with an odd value as the Thumb code address
      obtained by clearing the least significant bit of the
      value. This interpretation is deprecated, and it may not work in
      the future.

   Note - the Tagging symbols ($b, $f, $p $m) have been dropped from
   the EABI (which is still under development), so they are not
   implemented here.  */

static enum mstate mapstate = MAP_UNDEFINED;

static void
mapping_state (enum mstate state)
{
  symbolS * symbolP;
  const char * symname;
  int type;

  if (mapstate == state)
    /* The mapping symbol has already been emitted.
       There is nothing else to do.  */
    return;

  mapstate = state;

  switch (state)
    {
    case MAP_DATA:
      symname = "$d";
      type = BSF_OBJECT;
      break;
    case MAP_ARM:
      symname = "$a";
      type = BSF_FUNCTION;
      break;
    case MAP_THUMB:
      symname = "$t";
      type = BSF_FUNCTION;
      break;
    case MAP_UNDEFINED:
      return;
    default:
      abort ();
    }

  seg_info (now_seg)->tc_segment_info_data = state;

  symbolP = symbol_new (symname, now_seg, (valueT) frag_now_fix (), frag_now);
  symbol_table_insert (symbolP);
  symbol_get_bfdsym (symbolP)->flags |= type | BSF_LOCAL;

  switch (state)
    {
    case MAP_ARM:
      THUMB_SET_FUNC (symbolP, 0);
      ARM_SET_THUMB (symbolP, 0);
      ARM_SET_INTERWORK (symbolP, support_interwork);
      break;

    case MAP_THUMB:
      THUMB_SET_FUNC (symbolP, 1);
      ARM_SET_THUMB (symbolP, 1);
      ARM_SET_INTERWORK (symbolP, support_interwork);
      break;

    case MAP_DATA:
    default:
      return;
    }
}

/* When we change sections we need to issue a new mapping symbol.  */

void
arm_elf_change_section (void)
{
  flagword flags;

  /* Link an unlinked unwind index table section to the .text section.  */
  if (elf_section_type (now_seg) == SHT_ARM_EXIDX
      && elf_linked_to_section (now_seg) == NULL)
    elf_linked_to_section (now_seg) = text_section;

  if (!SEG_NORMAL (now_seg))
    return;

  flags = bfd_get_section_flags (stdoutput, now_seg);

  /* We can ignore sections that only contain debug info.  */
  if ((flags & SEC_ALLOC) == 0)
    return;

  mapstate = seg_info (now_seg)->tc_segment_info_data;
}

int
arm_elf_section_type (const char * str, size_t len)
{
  if (len == 5 && strncmp (str, "exidx", 5) == 0)
    return SHT_ARM_EXIDX;

  return -1;
}
#else
#define mapping_state(a)
#endif /* OBJ_ELF */

/* arm_reg_parse () := if it looks like a register, return its token and
   advance the pointer.  */

static int
arm_reg_parse (char ** ccp, struct hash_control * htab)
{
  char * start = * ccp;
  char   c;
  char * p;
  struct reg_entry * reg;

#ifdef REGISTER_PREFIX
  if (*start != REGISTER_PREFIX)
    return FAIL;
  p = start + 1;
#else
  p = start;
#ifdef OPTIONAL_REGISTER_PREFIX
  if (*p == OPTIONAL_REGISTER_PREFIX)
    p++, start++;
#endif
#endif
  if (!ISALPHA (*p) || !is_name_beginner (*p))
    return FAIL;

  c = *p++;
  while (ISALPHA (c) || ISDIGIT (c) || c == '_')
    c = *p++;

  *--p = 0;
  reg = (struct reg_entry *) hash_find (htab, start);
  *p = c;

  if (reg)
    {
      *ccp = p;
      return reg->number;
    }

  return FAIL;
}

/* Search for the following register name in each of the possible reg name
   tables.  Return the classification if found, or REG_TYPE_MAX if not
   present.  */

static enum arm_reg_type
arm_reg_parse_any (char *cp)
{
  int i;

  for (i = (int) REG_TYPE_FIRST; i < (int) REG_TYPE_MAX; i++)
    if (arm_reg_parse (&cp, all_reg_maps[i].htab) != FAIL)
      return (enum arm_reg_type) i;

  return REG_TYPE_MAX;
}

static void
opcode_select (int width)
{
  switch (width)
    {
    case 16:
      if (! thumb_mode)
	{
	  if (! (cpu_variant & ARM_EXT_V4T))
	    as_bad (_("selected processor does not support THUMB opcodes"));

	  thumb_mode = 1;
	  /* No need to force the alignment, since we will have been
             coming from ARM mode, which is word-aligned.  */
	  record_alignment (now_seg, 1);
	}
      mapping_state (MAP_THUMB);
      break;

    case 32:
      if (thumb_mode)
	{
	  if ((cpu_variant & ARM_ALL) == ARM_EXT_V4T)
	    as_bad (_("selected processor does not support ARM opcodes"));

	  thumb_mode = 0;

	  if (!need_pass_2)
	    frag_align (2, 0, 0);

	  record_alignment (now_seg, 1);
	}
      mapping_state (MAP_ARM);
      break;

    default:
      as_bad (_("invalid instruction size selected (%d)"), width);
    }
}

static void
s_req (int a ATTRIBUTE_UNUSED)
{
  as_bad (_("invalid syntax for .req directive"));
}

/* The .unreq directive deletes an alias which was previously defined
   by .req.  For example:

       my_alias .req r11
       .unreq my_alias    */

static void
s_unreq (int a ATTRIBUTE_UNUSED)
{
  char * name;
  char saved_char;

  skip_whitespace (input_line_pointer);
  name = input_line_pointer;

  while (*input_line_pointer != 0
	 && *input_line_pointer != ' '
	 && *input_line_pointer != '\n')
    ++input_line_pointer;

  saved_char = *input_line_pointer;
  *input_line_pointer = 0;

  if (*name)
    {
      enum arm_reg_type req_type = arm_reg_parse_any (name);

      if (req_type != REG_TYPE_MAX)
	{
	  char *temp_name = name;
	  int req_no = arm_reg_parse (&temp_name, all_reg_maps[req_type].htab);

	  if (req_no != FAIL)
	    {
	      struct reg_entry *req_entry;

	      /* Check to see if this alias is a builtin one.  */
	      req_entry = hash_delete (all_reg_maps[req_type].htab, name);

	      if (!req_entry)
		as_bad (_("unreq: missing hash entry for \"%s\""), name);
	      else if (req_entry->builtin)
		/* FIXME: We are deleting a built in register alias which
		   points to a const data structure, so we only need to
		   free up the memory used by the key in the hash table.
		   Unfortunately we have not recorded this value, so this
		   is a memory leak.  */
		  /* FIXME: Should we issue a warning message ?  */
		;
	      else
		{
		  /* Deleting a user defined alias.  We need to free the
		     key and the value, but fortunately the key is the same
		     as the value->name field.  */
		  free ((char *) req_entry->name);
		  free (req_entry);
		}
	    }
          else
            as_bad (_(".unreq: unrecognized symbol \"%s\""), name);
	}
      else
        as_bad (_(".unreq: unrecognized symbol \"%s\""), name);
    }
  else
    as_bad (_("invalid syntax for .unreq directive"));

  *input_line_pointer = saved_char;
  demand_empty_rest_of_line ();
}

static void
s_bss (int ignore ATTRIBUTE_UNUSED)
{
  /* We don't support putting frags in the BSS segment, we fake it by
     marking in_bss, then looking at s_skip for clues.  */
  subseg_set (bss_section, 0);
  demand_empty_rest_of_line ();
  mapping_state (MAP_DATA);
}

static void
s_even (int ignore ATTRIBUTE_UNUSED)
{
  /* Never make frag if expect extra pass.  */
  if (!need_pass_2)
    frag_align (1, 0, 0);

  record_alignment (now_seg, 1);

  demand_empty_rest_of_line ();
}

static void
s_ltorg (int ignored ATTRIBUTE_UNUSED)
{
  unsigned int entry;
  literal_pool * pool;
  char sym_name[20];

  pool = find_literal_pool ();
  if (pool == NULL
      || pool->symbol == NULL
      || pool->next_free_entry == 0)
    return;

  mapping_state (MAP_DATA);

  /* Align pool as you have word accesses.
     Only make a frag if we have to.  */
  if (!need_pass_2)
    frag_align (2, 0, 0);

  record_alignment (now_seg, 2);

  sprintf (sym_name, "$$lit_\002%x", pool->id);

  symbol_locate (pool->symbol, sym_name, now_seg,
		 (valueT) frag_now_fix (), frag_now);
  symbol_table_insert (pool->symbol);

  ARM_SET_THUMB (pool->symbol, thumb_mode);

#if defined OBJ_COFF || defined OBJ_ELF
  ARM_SET_INTERWORK (pool->symbol, support_interwork);
#endif

  for (entry = 0; entry < pool->next_free_entry; entry ++)
    /* First output the expression in the instruction to the pool.  */
    emit_expr (&(pool->literals[entry]), 4); /* .word  */

  /* Mark the pool as empty.  */
  pool->next_free_entry = 0;
  pool->symbol = NULL;
}

/* Same as s_align_ptwo but align 0 => align 2.  */

static void
s_align (int unused ATTRIBUTE_UNUSED)
{
  int temp;
  long temp_fill;
  long max_alignment = 15;

  temp = get_absolute_expression ();
  if (temp > max_alignment)
    as_bad (_("alignment too large: %d assumed"), temp = max_alignment);
  else if (temp < 0)
    {
      as_bad (_("alignment negative. 0 assumed."));
      temp = 0;
    }

  if (*input_line_pointer == ',')
    {
      input_line_pointer++;
      temp_fill = get_absolute_expression ();
    }
  else
    temp_fill = 0;

  if (!temp)
    temp = 2;

  /* Only make a frag if we HAVE to.  */
  if (temp && !need_pass_2)
    frag_align (temp, (int) temp_fill, 0);
  demand_empty_rest_of_line ();

  record_alignment (now_seg, temp);
}

static void
s_force_thumb (int ignore ATTRIBUTE_UNUSED)
{
  /* If we are not already in thumb mode go into it, EVEN if
     the target processor does not support thumb instructions.
     This is used by gcc/config/arm/lib1funcs.asm for example
     to compile interworking support functions even if the
     target processor should not support interworking.  */
  if (! thumb_mode)
    {
      thumb_mode = 2;

      record_alignment (now_seg, 1);
    }

  demand_empty_rest_of_line ();
}

static void
s_thumb_func (int ignore ATTRIBUTE_UNUSED)
{
  if (! thumb_mode)
    opcode_select (16);

  /* The following label is the name/address of the start of a Thumb function.
     We need to know this for the interworking support.  */
  label_is_thumb_function_name = TRUE;

  demand_empty_rest_of_line ();
}

/* Perform a .set directive, but also mark the alias as
   being a thumb function.  */

static void
s_thumb_set (int equiv)
{
  /* XXX the following is a duplicate of the code for s_set() in read.c
     We cannot just call that code as we need to get at the symbol that
     is created.  */
  char *    name;
  char      delim;
  char *    end_name;
  symbolS * symbolP;

  /* Especial apologies for the random logic:
     This just grew, and could be parsed much more simply!
     Dean - in haste.  */
  name      = input_line_pointer;
  delim     = get_symbol_end ();
  end_name  = input_line_pointer;
  *end_name = delim;

  SKIP_WHITESPACE ();

  if (*input_line_pointer != ',')
    {
      *end_name = 0;
      as_bad (_("expected comma after name \"%s\""), name);
      *end_name = delim;
      ignore_rest_of_line ();
      return;
    }

  input_line_pointer++;
  *end_name = 0;

  if (name[0] == '.' && name[1] == '\0')
    {
      /* XXX - this should not happen to .thumb_set.  */
      abort ();
    }

  if ((symbolP = symbol_find (name)) == NULL
      && (symbolP = md_undefined_symbol (name)) == NULL)
    {
#ifndef NO_LISTING
      /* When doing symbol listings, play games with dummy fragments living
	 outside the normal fragment chain to record the file and line info
         for this symbol.  */
      if (listing & LISTING_SYMBOLS)
	{
	  extern struct list_info_struct * listing_tail;
	  fragS * dummy_frag = xmalloc (sizeof (fragS));

	  memset (dummy_frag, 0, sizeof (fragS));
	  dummy_frag->fr_type = rs_fill;
	  dummy_frag->line = listing_tail;
	  symbolP = symbol_new (name, undefined_section, 0, dummy_frag);
	  dummy_frag->fr_symbol = symbolP;
	}
      else
#endif
	symbolP = symbol_new (name, undefined_section, 0, &zero_address_frag);

#ifdef OBJ_COFF
      /* "set" symbols are local unless otherwise specified.  */
      SF_SET_LOCAL (symbolP);
#endif /* OBJ_COFF  */
    }				/* Make a new symbol.  */

  symbol_table_insert (symbolP);

  * end_name = delim;

  if (equiv
      && S_IS_DEFINED (symbolP)
      && S_GET_SEGMENT (symbolP) != reg_section)
    as_bad (_("symbol `%s' already defined"), S_GET_NAME (symbolP));

  pseudo_set (symbolP);

  demand_empty_rest_of_line ();

  /* XXX Now we come to the Thumb specific bit of code.  */

  THUMB_SET_FUNC (symbolP, 1);
  ARM_SET_THUMB (symbolP, 1);
#if defined OBJ_ELF || defined OBJ_COFF
  ARM_SET_INTERWORK (symbolP, support_interwork);
#endif
}

static void
s_arm (int ignore ATTRIBUTE_UNUSED)
{
  opcode_select (32);
  demand_empty_rest_of_line ();
}

static void
s_thumb (int ignore ATTRIBUTE_UNUSED)
{
  opcode_select (16);
  demand_empty_rest_of_line ();
}

static void
s_code (int unused ATTRIBUTE_UNUSED)
{
  int temp;

  temp = get_absolute_expression ();
  switch (temp)
    {
    case 16:
    case 32:
      opcode_select (temp);
      break;

    default:
      as_bad (_("invalid operand to .code directive (%d) (expecting 16 or 32)"), temp);
    }
}

static void
end_of_line (char * str)
{
  skip_whitespace (str);

  if (*str != '\0' && !inst.error)
    inst.error = _("garbage following instruction");
}

static int
skip_past_comma (char ** str)
{
  char * p = * str, c;
  int comma = 0;

  while ((c = *p) == ' ' || c == ',')
    {
      p++;
      if (c == ',' && comma++)
	return FAIL;
    }

  if (c == '\0')
    return FAIL;

  *str = p;
  return comma ? SUCCESS : FAIL;
}

/* Return TRUE if anything in the expression is a bignum.  */

static int
walk_no_bignums (symbolS * sp)
{
  if (symbol_get_value_expression (sp)->X_op == O_big)
    return 1;

  if (symbol_get_value_expression (sp)->X_add_symbol)
    {
      return (walk_no_bignums (symbol_get_value_expression (sp)->X_add_symbol)
	      || (symbol_get_value_expression (sp)->X_op_symbol
		  && walk_no_bignums (symbol_get_value_expression (sp)->X_op_symbol)));
    }

  return 0;
}

static int in_my_get_expression = 0;

static int
my_get_expression (expressionS * ep, char ** str)
{
  char * save_in;
  segT   seg;

  save_in = input_line_pointer;
  input_line_pointer = *str;
  in_my_get_expression = 1;
  seg = expression (ep);
  in_my_get_expression = 0;

  if (ep->X_op == O_illegal)
    {
      /* We found a bad expression in md_operand().  */
      *str = input_line_pointer;
      input_line_pointer = save_in;
      return 1;
    }

#ifdef OBJ_AOUT
  if (seg != absolute_section
      && seg != text_section
      && seg != data_section
      && seg != bss_section
      && seg != undefined_section)
    {
      inst.error = _("bad_segment");
      *str = input_line_pointer;
      input_line_pointer = save_in;
      return 1;
    }
#endif

  /* Get rid of any bignums now, so that we don't generate an error for which
     we can't establish a line number later on.  Big numbers are never valid
     in instructions, which is where this routine is always called.  */
  if (ep->X_op == O_big
      || (ep->X_add_symbol
	  && (walk_no_bignums (ep->X_add_symbol)
	      || (ep->X_op_symbol
		  && walk_no_bignums (ep->X_op_symbol)))))
    {
      inst.error = _("invalid constant");
      *str = input_line_pointer;
      input_line_pointer = save_in;
      return 1;
    }

  *str = input_line_pointer;
  input_line_pointer = save_in;
  return 0;
}

/* A standard register must be given at this point.
   SHIFT is the place to put it in inst.instruction.
   Restores input start point on error.
   Returns the reg#, or FAIL.  */

static int
reg_required_here (char ** str, int shift)
{
  static char buff [128]; /* XXX  */
  int         reg;
  char *      start = * str;

  if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_RN].htab)) != FAIL)
    {
      if (shift >= 0)
	inst.instruction |= reg << shift;
      return reg;
    }

  /* Restore the start point, we may have got a reg of the wrong class.  */
  *str = start;

  /* In the few cases where we might be able to accept something else
     this error can be overridden.  */
  sprintf (buff, _("register expected, not '%.100s'"), start);
  inst.error = buff;

  return FAIL;
}

/* A Intel Wireless MMX technology register
   must be given at this point.
   Shift is the place to put it in inst.instruction.
   Restores input start point on err.
   Returns the reg#, or FAIL.  */

static int
wreg_required_here (char ** str,
		    int shift,
		    enum wreg_type reg_type)
{
  static char buff [128];
  int    reg;
  char * start = *str;

  if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_IWMMXT].htab)) != FAIL)
    {
      if (wr_register (reg)
	  && (reg_type == IWMMXT_REG_WR || reg_type == IWMMXT_REG_WR_OR_WC))
        {
          if (shift >= 0)
            inst.instruction |= (reg ^ WR_PREFIX) << shift;
          return reg;
        }
      else if (wc_register (reg)
	       && (reg_type == IWMMXT_REG_WC || reg_type == IWMMXT_REG_WR_OR_WC))
        {
          if (shift >= 0)
            inst.instruction |= (reg ^ WC_PREFIX) << shift;
          return reg;
        }
      else if ((wcg_register (reg) && reg_type == IWMMXT_REG_WCG))
        {
          if (shift >= 0)
            inst.instruction |= ((reg ^ WC_PREFIX) - 8) << shift;
          return reg;
        }
    }

  /* Restore the start point, we may have got a reg of the wrong class.  */
  *str = start;

  /* In the few cases where we might be able to accept
     something else this error can be overridden.  */
  sprintf (buff, _("Intel Wireless MMX technology register expected, not '%.100s'"), start);
  inst.error = buff;

  return FAIL;
}

static const struct asm_psr *
arm_psr_parse (char ** ccp)
{
  char * start = * ccp;
  char   c;
  char * p;
  const struct asm_psr * psr;

  p = start;

  /* Skip to the end of the next word in the input stream.  */
  do
    {
      c = *p++;
    }
  while (ISALPHA (c) || c == '_');

  /* Terminate the word.  */
  *--p = 0;

  /* CPSR's and SPSR's can now be lowercase.  This is just a convenience
     feature for ease of use and backwards compatibility.  */
  if (!strncmp (start, "cpsr", 4))
    strncpy (start, "CPSR", 4);
  else if (!strncmp (start, "spsr", 4))
    strncpy (start, "SPSR", 4);

  /* Now locate the word in the psr hash table.  */
  psr = (const struct asm_psr *) hash_find (arm_psr_hsh, start);

  /* Restore the input stream.  */
  *p = c;

  /* If we found a valid match, advance the
     stream pointer past the end of the word.  */
  *ccp = p;

  return psr;
}

/* Parse the input looking for a PSR flag.  */

static int
psr_required_here (char ** str)
{
  char * start = * str;
  const struct asm_psr * psr;

  psr = arm_psr_parse (str);

  if (psr)
    {
      /* If this is the SPSR that is being modified, set the R bit.  */
      if (! psr->cpsr)
	inst.instruction |= SPSR_BIT;

      /* Set the psr flags in the MSR instruction.  */
      inst.instruction |= psr->field << PSR_SHIFT;

      return SUCCESS;
    }

  /* In the few cases where we might be able to accept
     something else this error can be overridden.  */
  inst.error = _("flag for {c}psr instruction expected");

  /* Restore the start point.  */
  *str = start;
  return FAIL;
}

static int
co_proc_number (char ** str)
{
  int processor, pchar;
  char *start;

  skip_whitespace (*str);
  start = *str;

  /* The data sheet seems to imply that just a number on its own is valid
     here, but the RISC iX assembler seems to accept a prefix 'p'.  We will
     accept either.  */
  if ((processor = arm_reg_parse (str, all_reg_maps[REG_TYPE_CP].htab))
      == FAIL)
    {
      *str = start;

      pchar = *(*str)++;
      if (pchar >= '0' && pchar <= '9')
	{
	  processor = pchar - '0';
	  if (**str >= '0' && **str <= '9')
	    {
	      processor = processor * 10 + *(*str)++ - '0';
	      if (processor > 15)
		{
		  inst.error = _("illegal co-processor number");
		  return FAIL;
		}
	    }
	}
      else
	{
	  inst.error = all_reg_maps[REG_TYPE_CP].expected;
	  return FAIL;
	}
    }

  inst.instruction |= processor << 8;
  return SUCCESS;
}

static int
cp_opc_expr (char ** str, int where, int length)
{
  expressionS expr;

  skip_whitespace (* str);

  memset (&expr, '\0', sizeof (expr));

  if (my_get_expression (&expr, str))
    return FAIL;
  if (expr.X_op != O_constant)
    {
      inst.error = _("bad or missing expression");
      return FAIL;
    }

  if ((expr.X_add_number & ((1 << length) - 1)) != expr.X_add_number)
    {
      inst.error = _("immediate co-processor expression too large");
      return FAIL;
    }

  inst.instruction |= expr.X_add_number << where;
  return SUCCESS;
}

static int
cp_reg_required_here (char ** str, int where)
{
  int    reg;
  char * start = *str;

  if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_CN].htab)) != FAIL)
    {
      inst.instruction |= reg << where;
      return reg;
    }

  /* In the few cases where we might be able to accept something else
     this error can be overridden.  */
  inst.error = all_reg_maps[REG_TYPE_CN].expected;

  /* Restore the start point.  */
  *str = start;
  return FAIL;
}

static int
fp_reg_required_here (char ** str, int where)
{
  int    reg;
  char * start = * str;

  if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_FN].htab)) != FAIL)
    {
      inst.instruction |= reg << where;
      return reg;
    }

  /* In the few cases where we might be able to accept something else
     this error can be overridden.  */
  inst.error = all_reg_maps[REG_TYPE_FN].expected;

  /* Restore the start point.  */
  *str = start;
  return FAIL;
}

static int
cp_address_offset (char ** str)
{
  int offset;

  skip_whitespace (* str);

  if (! is_immediate_prefix (**str))
    {
      inst.error = _("immediate expression expected");
      return FAIL;
    }

  (*str)++;

  if (my_get_expression (& inst.reloc.exp, str))
    return FAIL;

  if (inst.reloc.exp.X_op == O_constant)
    {
      offset = inst.reloc.exp.X_add_number;

      if (offset & 3)
	{
	  inst.error = _("co-processor address must be word aligned");
	  return FAIL;
	}

      if (offset > 1023 || offset < -1023)
	{
	  inst.error = _("offset too large");
	  return FAIL;
	}

      if (offset >= 0)
	inst.instruction |= INDEX_UP;
      else
	offset = -offset;

      inst.instruction |= offset >> 2;
    }
  else
    inst.reloc.type = BFD_RELOC_ARM_CP_OFF_IMM;

  return SUCCESS;
}

static int
cp_address_required_here (char ** str, int wb_ok)
{
  char * p = * str;
  int    pre_inc = 0;
  int    write_back = 0;

  if (*p == '[')
    {
      int reg;

      p++;
      skip_whitespace (p);

      if ((reg = reg_required_here (& p, 16)) == FAIL)
	return FAIL;

      skip_whitespace (p);

      if (*p == ']')
	{
	  p++;

	  skip_whitespace (p);

	  if (*p == '\0')
	    {
	      /* As an extension to the official ARM syntax we allow:
		   [Rn]
	         as a short hand for:
		   [Rn,#0]  */
	      inst.instruction |= PRE_INDEX | INDEX_UP;
	      *str = p;
	      return SUCCESS;
	    }

	  if (skip_past_comma (& p) == FAIL)
	    {
	      inst.error = _("comma expected after closing square bracket");
	      return FAIL;
	    }

	  skip_whitespace (p);

	  if (*p == '#')
	    {
	      if (wb_ok)
		{
		  /* [Rn], #expr  */
		  write_back = WRITE_BACK;

		  if (reg == REG_PC)
		    {
		      inst.error = _("pc may not be used in post-increment");
		      return FAIL;
		    }

		  if (cp_address_offset (& p) == FAIL)
		    return FAIL;
		}
	      else
		pre_inc = PRE_INDEX | INDEX_UP;
	    }
	  else if (*p == '{')
	    {
	      int option;

	      /* [Rn], {<expr>}  */
	      p++;

	      skip_whitespace (p);

	      if (my_get_expression (& inst.reloc.exp, & p))
		return FAIL;

	      if (inst.reloc.exp.X_op == O_constant)
		{
		  option = inst.reloc.exp.X_add_number;

		  if (option > 255 || option < 0)
		    {
		      inst.error = _("'option' field too large");
		      return FAIL;
		    }

		  skip_whitespace (p);

		  if (*p != '}')
		    {
		      inst.error = _("'}' expected at end of 'option' field");
		      return FAIL;
		    }
		  else
		    {
		      p++;
		      inst.instruction |= option;
		      inst.instruction |= INDEX_UP;
		    }
		}
	      else
		{
		  inst.error = _("non-constant expressions for 'option' field not supported");
		  return FAIL;
		}
	    }
	  else
	    {
	      inst.error = _("# or { expected after comma");
	      return FAIL;
	    }
	}
      else
	{
	  /* '['Rn, #expr']'[!]  */

	  if (skip_past_comma (& p) == FAIL)
	    {
	      inst.error = _("pre-indexed expression expected");
	      return FAIL;
	    }

	  pre_inc = PRE_INDEX;

	  if (cp_address_offset (& p) == FAIL)
	    return FAIL;

	  skip_whitespace (p);

	  if (*p++ != ']')
	    {
	      inst.error = _("missing ]");
	      return FAIL;
	    }

	  skip_whitespace (p);

	  if (wb_ok && *p == '!')
	    {
	      if (reg == REG_PC)
		{
		  inst.error = _("pc may not be used with write-back");
		  return FAIL;
		}

	      p++;
	      write_back = WRITE_BACK;
	    }
	}
    }
  else
    {
      if (my_get_expression (&inst.reloc.exp, &p))
	return FAIL;

      inst.reloc.type = BFD_RELOC_ARM_CP_OFF_IMM;
      inst.reloc.exp.X_add_number -= 8;  /* PC rel adjust.  */
      inst.reloc.pc_rel = 1;
      inst.instruction |= (REG_PC << 16);
      pre_inc = PRE_INDEX;
    }

  inst.instruction |= write_back | pre_inc;
  *str = p;
  return SUCCESS;
}

static int
cp_byte_address_offset (char ** str)
{
  int offset;

  skip_whitespace (* str);

  if (! is_immediate_prefix (**str))
    {
      inst.error = _("immediate expression expected");
      return FAIL;
    }

  (*str)++;

  if (my_get_expression (& inst.reloc.exp, str))
    return FAIL;

  if (inst.reloc.exp.X_op == O_constant)
    {
      offset = inst.reloc.exp.X_add_number;

      if (offset > 255 || offset < -255)
        {
          inst.error = _("offset too large");
          return FAIL;
        }

      if (offset >= 0)
        inst.instruction |= INDEX_UP;
      else
        offset = -offset;

      inst.instruction |= offset;
    }
  else
    inst.reloc.type = BFD_RELOC_ARM_CP_OFF_IMM_S2;

  return SUCCESS;
}

static int
cp_byte_address_required_here (char ** str)
{
  char * p = * str;
  int    pre_inc = 0;
  int    write_back = 0;

  if (*p == '[')
    {
      int reg;

      p++;
      skip_whitespace (p);

      if ((reg = reg_required_here (& p, 16)) == FAIL)
        return FAIL;

      skip_whitespace (p);

      if (*p == ']')
        {
          p++;

          if (skip_past_comma (& p) == SUCCESS)
            {
              /* [Rn], #expr */
              write_back = WRITE_BACK;

              if (reg == REG_PC)
                {
                  inst.error = _("pc may not be used in post-increment");
                  return FAIL;
                }

              if (cp_byte_address_offset (& p) == FAIL)
                return FAIL;
            }
          else
            pre_inc = PRE_INDEX | INDEX_UP;
        }
      else
        {
          /* '['Rn, #expr']'[!] */

          if (skip_past_comma (& p) == FAIL)
            {
              inst.error = _("pre-indexed expression expected");
              return FAIL;
            }

          pre_inc = PRE_INDEX;

          if (cp_byte_address_offset (& p) == FAIL)
            return FAIL;

          skip_whitespace (p);

          if (*p++ != ']')
            {
              inst.error = _("missing ]");
              return FAIL;
            }

          skip_whitespace (p);

          if (*p == '!')
            {
              if (reg == REG_PC)
                {
                  inst.error = _("pc may not be used with write-back");
                  return FAIL;
                }

              p++;
              write_back = WRITE_BACK;
            }
        }
    }
  else
    {
      if (my_get_expression (&inst.reloc.exp, &p))
        return FAIL;

      inst.reloc.type = BFD_RELOC_ARM_CP_OFF_IMM_S2;
      inst.reloc.exp.X_add_number -= 8;  /* PC rel adjust.  */
      inst.reloc.pc_rel = 1;
      inst.instruction |= (REG_PC << 16);
      pre_inc = PRE_INDEX;
    }

  inst.instruction |= write_back | pre_inc;
  *str = p;
  return SUCCESS;
}

static void
do_nop (char * str)
{
  skip_whitespace (str);
  if (*str == '{')
    {
      str++;

      if (my_get_expression (&inst.reloc.exp, &str))
	inst.reloc.exp.X_op = O_illegal;
      else
	{
	  skip_whitespace (str);
	  if (*str == '}')
	    str++;
	  else
	    inst.reloc.exp.X_op = O_illegal;
	}

      if (inst.reloc.exp.X_op != O_constant
	  || inst.reloc.exp.X_add_number > 255
	  || inst.reloc.exp.X_add_number < 0)
	{
	  inst.error = _("Invalid NOP hint");
	  return;
	}

      /* Arcitectural NOP hints are CPSR sets with no bits selected.  */
      inst.instruction &= 0xf0000000;
      inst.instruction |= 0x0320f000 + inst.reloc.exp.X_add_number;
    }

  end_of_line (str);
}

static void
do_empty (char * str)
{
  /* Do nothing really.  */
  end_of_line (str);
}

static void
do_mrs (char * str)
{
  int skip = 0;

  /* Only one syntax.  */
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL)
    {
      inst.error = _("comma expected after register name");
      return;
    }

  skip_whitespace (str);

  if (   streq (str, "CPSR")
      || streq (str, "SPSR")
	 /* Lower case versions for backwards compatibility.  */
      || streq (str, "cpsr")
      || streq (str, "spsr"))
    skip = 4;

  /* This is for backwards compatibility with older toolchains.  */
  else if (   streq (str, "cpsr_all")
	   || streq (str, "spsr_all"))
    skip = 8;
  else
    {
      inst.error = _("CPSR or SPSR expected");
      return;
    }

  if (* str == 's' || * str == 'S')
    inst.instruction |= SPSR_BIT;
  str += skip;

  end_of_line (str);
}

/* Two possible forms:
      "{C|S}PSR_<field>, Rm",
      "{C|S}PSR_f, #expression".  */

static void
do_msr (char * str)
{
  skip_whitespace (str);

  if (psr_required_here (& str) == FAIL)
    return;

  if (skip_past_comma (& str) == FAIL)
    {
      inst.error = _("comma missing after psr flags");
      return;
    }

  skip_whitespace (str);

  if (reg_required_here (& str, 0) != FAIL)
    {
      inst.error = NULL;
      end_of_line (str);
      return;
    }

  if (! is_immediate_prefix (* str))
    {
      inst.error =
	_("only a register or immediate value can follow a psr flag");
      return;
    }

  str ++;
  inst.error = NULL;

  if (my_get_expression (& inst.reloc.exp, & str))
    {
      inst.error =
	_("only a register or immediate value can follow a psr flag");
      return;
    }

  inst.instruction |= INST_IMMEDIATE;

  if (inst.reloc.exp.X_add_symbol)
    {
      inst.reloc.type = BFD_RELOC_ARM_IMMEDIATE;
      inst.reloc.pc_rel = 0;
    }
  else
    {
      unsigned value = validate_immediate (inst.reloc.exp.X_add_number);

      if (value == (unsigned) FAIL)
	{
	  inst.error = _("invalid constant");
	  return;
	}

      inst.instruction |= value;
    }

  inst.error = NULL;
  end_of_line (str);
}

/* Long Multiply Parser
   UMULL RdLo, RdHi, Rm, Rs
   SMULL RdLo, RdHi, Rm, Rs
   UMLAL RdLo, RdHi, Rm, Rs
   SMLAL RdLo, RdHi, Rm, Rs.  */

static void
do_mull (char * str)
{
  int rdlo, rdhi, rm, rs;

  /* Only one format "rdlo, rdhi, rm, rs".  */
  skip_whitespace (str);

  if ((rdlo = reg_required_here (&str, 12)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (rdhi = reg_required_here (&str, 16)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  /* rdhi, rdlo and rm must all be different.  */
  if (rdlo == rdhi || rdlo == rm || rdhi == rm)
    as_tsktsk (_("rdhi, rdlo and rm must all be different"));

  if (skip_past_comma (&str) == FAIL
      || (rs = reg_required_here (&str, 8)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rdhi == REG_PC || rdhi == REG_PC || rdhi == REG_PC || rdhi == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

static void
do_mul (char * str)
{
  int rd, rm;

  /* Only one format "rd, rm, rs".  */
  skip_whitespace (str);

  if ((rd = reg_required_here (&str, 16)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rd == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (rm == rd)
    as_tsktsk (_("rd and rm should be different in mul"));

  if (skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 8)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

static void
do_mla (char * str)
{
  int rd, rm;

  /* Only one format "rd, rm, rs, rn".  */
  skip_whitespace (str);

  if ((rd = reg_required_here (&str, 16)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rd == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (rm == rd)
    as_tsktsk (_("rd and rm should be different in mla"));

  if (skip_past_comma (&str) == FAIL
      || (rd = reg_required_here (&str, 8)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 12)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rd == REG_PC || rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

/* Expects *str -> the characters "acc0", possibly with leading blanks.
   Advances *str to the next non-alphanumeric.
   Returns 0, or else FAIL (in which case sets inst.error).

  (In a future XScale, there may be accumulators other than zero.
  At that time this routine and its callers can be upgraded to suit.)  */

static int
accum0_required_here (char ** str)
{
  static char buff [128];	/* Note the address is taken.  Hence, static.  */
  char * p = * str;
  char   c;
  int result = 0;		/* The accum number.  */

  skip_whitespace (p);

  *str = p;			/* Advance caller's string pointer too.  */
  c = *p++;
  while (ISALNUM (c))
    c = *p++;

  *--p = 0;			/* Aap nul into input buffer at non-alnum.  */

  if (! ( streq (*str, "acc0") || streq (*str, "ACC0")))
    {
      sprintf (buff, _("acc0 expected, not '%.100s'"), *str);
      inst.error = buff;
      result = FAIL;
    }

  *p = c;			/* Unzap.  */
  *str = p;			/* Caller's string pointer to after match.  */
  return result;
}

static int
ldst_extend_v4 (char ** str)
{
  int add = INDEX_UP;

  switch (**str)
    {
    case '#':
    case '$':
      (*str)++;
      if (my_get_expression (& inst.reloc.exp, str))
	return FAIL;

      if (inst.reloc.exp.X_op == O_constant)
	{
	  int value = inst.reloc.exp.X_add_number;

	  if (value < -255 || value > 255)
	    {
	      inst.error = _("address offset too large");
	      return FAIL;
	    }

	  if (value < 0)
	    {
	      value = -value;
	      add = 0;
	    }

	  /* Halfword and signextension instructions have the
             immediate value split across bits 11..8 and bits 3..0.  */
	  inst.instruction |= (add | HWOFFSET_IMM
			       | ((value >> 4) << 8) | (value & 0xF));
	}
      else
	{
	  inst.instruction |= HWOFFSET_IMM;
	  inst.reloc.type = BFD_RELOC_ARM_OFFSET_IMM8;
	  inst.reloc.pc_rel = 0;
	}
      return SUCCESS;

    case '-':
      add = 0;
      /* Fall through.  */

    case '+':
      (*str)++;
      /* Fall through.  */

    default:
      if (reg_required_here (str, 0) == FAIL)
	return FAIL;

      inst.instruction |= add;
      return SUCCESS;
    }
}

/* Expects **str -> after a comma. May be leading blanks.
   Advances *str, recognizing a load  mode, and setting inst.instruction.
   Returns rn, or else FAIL (in which case may set inst.error
   and not advance str)

   Note: doesn't know Rd, so no err checks that require such knowledge.  */

static int
ld_mode_required_here (char ** string)
{
  char * str = * string;
  int    rn;
  int    pre_inc = 0;

  skip_whitespace (str);

  if (* str == '[')
    {
      str++;

      skip_whitespace (str);

      if ((rn = reg_required_here (& str, 16)) == FAIL)
	return FAIL;

      skip_whitespace (str);

      if (* str == ']')
	{
	  str ++;

	  if (skip_past_comma (& str) == SUCCESS)
	    {
	      /* [Rn],... (post inc) */
	      if (ldst_extend_v4 (&str) == FAIL)
		return FAIL;
	    }
	  else 	      /* [Rn] */
	    {
	      skip_whitespace (str);

	      if (* str == '!')
		{
		  str ++;
		  inst.instruction |= WRITE_BACK;
		}

	      inst.instruction |= INDEX_UP | HWOFFSET_IMM;
	      pre_inc = 1;
	    }
	}
      else	  /* [Rn,...] */
	{
	  if (skip_past_comma (& str) == FAIL)
	    {
	      inst.error = _("pre-indexed expression expected");
	      return FAIL;
	    }

	  pre_inc = 1;

	  if (ldst_extend_v4 (&str) == FAIL)
	    return FAIL;

	  skip_whitespace (str);

	  if (* str ++ != ']')
	    {
	      inst.error = _("missing ]");
	      return FAIL;
	    }

	  skip_whitespace (str);

	  if (* str == '!')
	    {
	      str ++;
	      inst.instruction |= WRITE_BACK;
	    }
	}
    }
  else if (* str == '=')	/* ldr's "r,=label" syntax */
    /* We should never reach here, because <text> = <expression> is
       caught gas/read.c read_a_source_file() as a .set operation.  */
    return FAIL;
  else				/* PC +- 8 bit immediate offset.  */
    {
      if (my_get_expression (& inst.reloc.exp, & str))
	return FAIL;

      inst.instruction            |= HWOFFSET_IMM;	/* The I bit.  */
      inst.reloc.type              = BFD_RELOC_ARM_OFFSET_IMM8;
      inst.reloc.exp.X_add_number -= 8;  		/* PC rel adjust.  */
      inst.reloc.pc_rel            = 1;
      inst.instruction            |= (REG_PC << 16);

      rn = REG_PC;
      pre_inc = 1;
    }

  inst.instruction |= (pre_inc ? PRE_INDEX : 0);
  * string = str;

  return rn;
}

/* ARM V5E (El Segundo) signed-multiply-accumulate (argument parse)
   SMLAxy{cond} Rd,Rm,Rs,Rn
   SMLAWy{cond} Rd,Rm,Rs,Rn
   Error if any register is R15.  */

static void
do_smla (char * str)
{
  int rd, rm, rs, rn;

  skip_whitespace (str);

  if ((rd = reg_required_here (& str, 16)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rm = reg_required_here (& str, 0)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rs = reg_required_here (& str, 8)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rn = reg_required_here (& str, 12)) == FAIL)
    inst.error = BAD_ARGS;

  else if (rd == REG_PC || rm == REG_PC || rs == REG_PC || rn == REG_PC)
    inst.error = BAD_PC;

  else
    end_of_line (str);
}

/* ARM V5E (El Segundo) signed-multiply-accumulate-long (argument parse)
   SMLALxy{cond} Rdlo,Rdhi,Rm,Rs
   Error if any register is R15.
   Warning if Rdlo == Rdhi.  */

static void
do_smlal (char * str)
{
  int rdlo, rdhi, rm, rs;

  skip_whitespace (str);

  if ((rdlo = reg_required_here (& str, 12)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rdhi = reg_required_here (& str, 16)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rm = reg_required_here (& str, 0)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rs = reg_required_here (& str, 8)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rdlo == REG_PC || rdhi == REG_PC || rm == REG_PC || rs == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (rdlo == rdhi)
    as_tsktsk (_("rdhi and rdlo must be different"));

  end_of_line (str);
}

/* ARM V5E (El Segundo) signed-multiply (argument parse)
   SMULxy{cond} Rd,Rm,Rs
   Error if any register is R15.  */

static void
do_smul (char * str)
{
  int rd, rm, rs;

  skip_whitespace (str);

  if ((rd = reg_required_here (& str, 16)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rm = reg_required_here (& str, 0)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rs = reg_required_here (& str, 8)) == FAIL)
    inst.error = BAD_ARGS;

  else if (rd == REG_PC || rm == REG_PC || rs == REG_PC)
    inst.error = BAD_PC;

  else
    end_of_line (str);
}

/* ARM V5E (El Segundo) saturating-add/subtract (argument parse)
   Q[D]{ADD,SUB}{cond} Rd,Rm,Rn
   Error if any register is R15.  */

static void
do_qadd (char * str)
{
  int rd, rm, rn;

  skip_whitespace (str);

  if ((rd = reg_required_here (& str, 12)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rm = reg_required_here (& str, 0)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rn = reg_required_here (& str, 16)) == FAIL)
    inst.error = BAD_ARGS;

  else if (rd == REG_PC || rm == REG_PC || rn == REG_PC)
    inst.error = BAD_PC;

  else
    end_of_line (str);
}

/* ARM V5E (el Segundo)
   MCRRcc <coproc>, <opcode>, <Rd>, <Rn>, <CRm>.
   MRRCcc <coproc>, <opcode>, <Rd>, <Rn>, <CRm>.

   These are equivalent to the XScale instructions MAR and MRA,
   respectively, when coproc == 0, opcode == 0, and CRm == 0.

   Result unpredicatable if Rd or Rn is R15.  */

static void
do_co_reg2c (char * str)
{
  int rd, rn;

  skip_whitespace (str);

  if (co_proc_number (& str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_opc_expr (& str, 4, 4) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || (rd = reg_required_here (& str, 12)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || (rn = reg_required_here (& str, 16)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  /* Unpredictable result if rd or rn is R15.  */
  if (rd == REG_PC || rn == REG_PC)
    as_tsktsk
      (_("Warning: instruction unpredictable when using r15"));

  if (skip_past_comma (& str) == FAIL
      || cp_reg_required_here (& str, 0) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

/* ARM V5 count-leading-zeroes instruction (argument parse)
     CLZ{<cond>} <Rd>, <Rm>
     Condition defaults to COND_ALWAYS.
     Error if Rd or Rm are R15.  */

static void
do_clz (char * str)
{
  int rd, rm;

  skip_whitespace (str);

  if (((rd = reg_required_here (& str, 12)) == FAIL)
      || (skip_past_comma (& str) == FAIL)
      || ((rm = reg_required_here (& str, 0)) == FAIL))
    inst.error = BAD_ARGS;

  else if (rd == REG_PC || rm == REG_PC )
    inst.error = BAD_PC;

  else
    end_of_line (str);
}

/* ARM V5 (argument parse)
     LDC2{L} <coproc>, <CRd>, <addressing mode>
     STC2{L} <coproc>, <CRd>, <addressing mode>
     Instruction is not conditional, and has 0xf in the condition field.
     Otherwise, it's the same as LDC/STC.  */

static void
do_lstc2 (char * str)
{
  skip_whitespace (str);

  if (co_proc_number (& str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
    }
  else if (skip_past_comma (& str) == FAIL
	   || cp_reg_required_here (& str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
    }
  else if (skip_past_comma (& str) == FAIL
	   || cp_address_required_here (&str, CP_WB_OK) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
    }
  else
    end_of_line (str);
}

/* ARM V5 (argument parse)
     CDP2 <coproc>, <opcode_1>, <CRd>, <CRn>, <CRm>, <opcode_2>
     Instruction is not conditional, and has 0xf in the condition field.
     Otherwise, it's the same as CDP.  */

static void
do_cdp2 (char * str)
{
  skip_whitespace (str);

  if (co_proc_number (& str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_opc_expr (& str, 20,4) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_reg_required_here (& str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_reg_required_here (& str, 16) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_reg_required_here (& str, 0) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == SUCCESS)
    {
      if (cp_opc_expr (& str, 5, 3) == FAIL)
	{
	  if (!inst.error)
	    inst.error = BAD_ARGS;
	  return;
	}
    }

  end_of_line (str);
}

/* ARM V5 (argument parse)
     MCR2 <coproc>, <opcode_1>, <Rd>, <CRn>, <CRm>, <opcode_2>
     MRC2 <coproc>, <opcode_1>, <Rd>, <CRn>, <CRm>, <opcode_2>
     Instruction is not conditional, and has 0xf in the condition field.
     Otherwise, it's the same as MCR/MRC.  */

static void
do_co_reg2 (char * str)
{
  skip_whitespace (str);

  if (co_proc_number (& str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_opc_expr (& str, 21, 3) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || reg_required_here (& str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_reg_required_here (& str, 16) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || cp_reg_required_here (& str, 0) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == SUCCESS)
    {
      if (cp_opc_expr (& str, 5, 3) == FAIL)
	{
	  if (!inst.error)
	    inst.error = BAD_ARGS;
	  return;
	}
    }

  end_of_line (str);
}

static void
do_bx (char * str)
{
  int reg;

  skip_whitespace (str);

  if ((reg = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  /* Note - it is not illegal to do a "bx pc".  Useless, but not illegal.  */
  if (reg == REG_PC)
    as_tsktsk (_("use of r15 in bx in ARM mode is not really useful"));

  end_of_line (str);
}

/* ARM v5TEJ.  Jump to Jazelle code.  */

static void
do_bxj (char * str)
{
  int reg;

  skip_whitespace (str);

  if ((reg = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  /* Note - it is not illegal to do a "bxj pc".  Useless, but not illegal.  */
  if (reg == REG_PC)
    as_tsktsk (_("use of r15 in bxj is not really useful"));

  end_of_line (str);
}

/* ARM V6 umaal (argument parse).  */

static void
do_umaal (char * str)
{
  int rdlo, rdhi, rm, rs;

  skip_whitespace (str);
  if ((rdlo = reg_required_here (& str, 12)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rdhi = reg_required_here (& str, 16)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rm = reg_required_here (& str, 0)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rs = reg_required_here (& str, 8)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (rdlo == REG_PC || rdhi == REG_PC || rm == REG_PC || rs == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

/* ARM V6 strex (argument parse).  */

static void
do_strex (char * str)
{
  int rd, rm, rn;

  /* Parse Rd, Rm,.  */
  skip_whitespace (str);
  if ((rd = reg_required_here (& str, 12)) == FAIL
      || skip_past_comma (& str) == FAIL
      || (rm = reg_required_here (& str, 0)) == FAIL
      || skip_past_comma (& str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  if (rd == REG_PC || rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }
  if (rd == rm)
    {
      inst.error = _("Rd equal to Rm or Rn yields unpredictable results");
      return;
    }

  /* Skip past '['.  */
  if ((strlen (str) >= 1)
      && strncmp (str, "[", 1) == 0)
    str += 1;

  skip_whitespace (str);

  /* Parse Rn.  */
  if ((rn = reg_required_here (& str, 16)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  else if (rn == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }
  if (rd == rn)
    {
      inst.error = _("Rd equal to Rm or Rn yields unpredictable results");
      return;
    }
  skip_whitespace (str);

  /* Skip past ']'.  */
  if ((strlen (str) >= 1)
      && strncmp (str, "]", 1) == 0)
    str += 1;

  end_of_line (str);
}

/* KIND indicates what kind of shifts are accepted.  */

static int
decode_shift (char ** str, int kind)
{
  const struct asm_shift_name * shift;
  char * p;
  char   c;

  skip_whitespace (* str);

  for (p = * str; ISALPHA (* p); p ++)
    ;

  if (p == * str)
    {
      inst.error = _("shift expression expected");
      return FAIL;
    }

  c = * p;
  * p = '\0';
  shift = (const struct asm_shift_name *) hash_find (arm_shift_hsh, * str);
  * p = c;

  if (shift == NULL)
    {
      inst.error = _("shift expression expected");
      return FAIL;
    }

  assert (shift->properties->index == shift_properties[shift->properties->index].index);

  if (kind == SHIFT_LSL_OR_ASR_IMMEDIATE
      && shift->properties->index != SHIFT_LSL
      && shift->properties->index != SHIFT_ASR)
    {
      inst.error = _("'LSL' or 'ASR' required");
      return FAIL;
    }
  else if (kind == SHIFT_LSL_IMMEDIATE
	   && shift->properties->index != SHIFT_LSL)
    {
      inst.error = _("'LSL' required");
      return FAIL;
    }
  else if (kind == SHIFT_ASR_IMMEDIATE
	   && shift->properties->index != SHIFT_ASR)
    {
      inst.error = _("'ASR' required");
      return FAIL;
    }

  if (shift->properties->index == SHIFT_RRX)
    {
      * str = p;
      inst.instruction |= shift->properties->bit_field;
      return SUCCESS;
    }

  skip_whitespace (p);

  if (kind == NO_SHIFT_RESTRICT && reg_required_here (& p, 8) != FAIL)
    {
      inst.instruction |= shift->properties->bit_field | SHIFT_BY_REG;
      * str = p;
      return SUCCESS;
    }
  else if (! is_immediate_prefix (* p))
    {
      inst.error = (NO_SHIFT_RESTRICT
		    ? _("shift requires register or #expression")
		    : _("shift requires #expression"));
      * str = p;
      return FAIL;
    }

  inst.error = NULL;
  p ++;

  if (my_get_expression (& inst.reloc.exp, & p))
    return FAIL;

  /* Validate some simple #expressions.  */
  if (inst.reloc.exp.X_op == O_constant)
    {
      unsigned num = inst.reloc.exp.X_add_number;

      /* Reject operations greater than 32.  */
      if (num > 32
	  /* Reject a shift of 0 unless the mode allows it.  */
	  || (num == 0 && shift->properties->allows_0 == 0)
	  /* Reject a shift of 32 unless the mode allows it.  */
	  || (num == 32 && shift->properties->allows_32 == 0)
	  )
	{
	  /* As a special case we allow a shift of zero for
	     modes that do not support it to be recoded as an
	     logical shift left of zero (ie nothing).  We warn
	     about this though.  */
	  if (num == 0)
	    {
	      as_warn (_("shift of 0 ignored."));
	      shift = & shift_names[0];
	      assert (shift->properties->index == SHIFT_LSL);
	    }
	  else
	    {
	      inst.error = _("invalid immediate shift");
	      return FAIL;
	    }
	}

      /* Shifts of 32 are encoded as 0, for those shifts that
	 support it.  */
      if (num == 32)
	num = 0;

      inst.instruction |= (num << 7) | shift->properties->bit_field;
    }
  else
    {
      inst.reloc.type   = BFD_RELOC_ARM_SHIFT_IMM;
      inst.reloc.pc_rel = 0;
      inst.instruction |= shift->properties->bit_field;
    }

  * str = p;
  return SUCCESS;
}

static void
do_sat (char ** str, int bias)
{
  int rd, rm;
  expressionS expr;

  skip_whitespace (*str);

  /* Parse <Rd>, field.  */
  if ((rd = reg_required_here (str, 12)) == FAIL
      || skip_past_comma (str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  if (rd == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  /* Parse #<immed>,  field.  */
  if (is_immediate_prefix (**str))
    (*str)++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }
  if (my_get_expression (&expr, str))
    {
      inst.error = _("bad expression");
      return;
    }
  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }
  if (expr.X_add_number + bias < 0
      || expr.X_add_number + bias > 31)
    {
      inst.error = _("immediate value out of range");
      return;
    }
  inst.instruction |= (expr.X_add_number + bias) << 16;
  if (skip_past_comma (str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  /* Parse <Rm> field.  */
  if ((rm = reg_required_here (str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  if (rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (skip_past_comma (str) == SUCCESS)
    decode_shift (str, SHIFT_LSL_OR_ASR_IMMEDIATE);
}

/* ARM V6 ssat (argument parse).  */

static void
do_ssat (char * str)
{
  do_sat (&str, /*bias=*/-1);
  end_of_line (str);
}

/* ARM V6 usat (argument parse).  */

static void
do_usat (char * str)
{
  do_sat (&str, /*bias=*/0);
  end_of_line (str);
}

static void
do_sat16 (char ** str, int bias)
{
  int rd, rm;
  expressionS expr;

  skip_whitespace (*str);

  /* Parse the <Rd> field.  */
  if ((rd = reg_required_here (str, 12)) == FAIL
      || skip_past_comma (str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  if (rd == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  /* Parse #<immed>, field.  */
  if (is_immediate_prefix (**str))
    (*str)++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }
  if (my_get_expression (&expr, str))
    {
      inst.error = _("bad expression");
      return;
    }
  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }
  if (expr.X_add_number + bias < 0
      || expr.X_add_number + bias > 15)
    {
      inst.error = _("immediate value out of range");
      return;
    }
  inst.instruction |= (expr.X_add_number + bias) << 16;
  if (skip_past_comma (str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  /* Parse <Rm> field.  */
  if ((rm = reg_required_here (str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  if (rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }
}

/* ARM V6 ssat16 (argument parse).  */

static void
do_ssat16 (char * str)
{
  do_sat16 (&str, /*bias=*/-1);
  end_of_line (str);
}

static void
do_usat16 (char * str)
{
  do_sat16 (&str, /*bias=*/0);
  end_of_line (str);
}

static void
do_cps_mode (char ** str)
{
  expressionS expr;

  skip_whitespace (*str);

  if (! is_immediate_prefix (**str))
    {
      inst.error = _("immediate expression expected");
      return;
    }

  (*str)++; /* Strip off the immediate signifier.  */
  if (my_get_expression (&expr, str))
    {
      inst.error = _("bad expression");
      return;
    }

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  /* The mode is a 5 bit field.  Valid values are 0-31.  */
  if (((unsigned) expr.X_add_number) > 31
      || (inst.reloc.exp.X_add_number) < 0)
    {
      inst.error = _("invalid constant");
      return;
    }

  inst.instruction |= expr.X_add_number;
}

/* ARM V6 srs (argument parse).  */

static void
do_srs (char * str)
{
  char *exclam;
  skip_whitespace (str);
  exclam = strchr (str, '!');
  if (exclam)
    *exclam = '\0';
  do_cps_mode (&str);
  if (exclam)
    *exclam = '!';
  if (*str == '!')
    {
      inst.instruction |= WRITE_BACK;
      str++;
    }
  end_of_line (str);
}

/* ARM V6 SMMUL (argument parse).  */

static void
do_smmul (char * str)
{
  int rd, rm, rs;

  skip_whitespace (str);
  if ((rd = reg_required_here (&str, 16)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rs = reg_required_here (&str, 8)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (   rd == REG_PC
      || rm == REG_PC
      || rs == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

/* ARM V6 SMLALD (argument parse).  */

static void
do_smlald (char * str)
{
  int rdlo, rdhi, rm, rs;

  skip_whitespace (str);
  if ((rdlo = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rdhi = reg_required_here (&str, 16)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rs = reg_required_here (&str, 8)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (   rdlo == REG_PC
      || rdhi == REG_PC
      || rm == REG_PC
      || rs == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

/* ARM V6 SMLAD (argument parse).  Signed multiply accumulate dual.
   smlad{x}{<cond>} Rd, Rm, Rs, Rn */

static void
do_smlad (char * str)
{
  int rd, rm, rs, rn;

  skip_whitespace (str);
  if ((rd = reg_required_here (&str, 16)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rs = reg_required_here (&str, 8)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rn = reg_required_here (&str, 12)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (   rd == REG_PC
      || rn == REG_PC
      || rs == REG_PC
      || rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  end_of_line (str);
}

/* Returns true if the endian-specifier indicates big-endianness.  */

static int
do_endian_specifier (char * str)
{
  int big_endian = 0;

  skip_whitespace (str);
  if (strlen (str) < 2)
    inst.error = _("missing endian specifier");
  else if (strncasecmp (str, "BE", 2) == 0)
    {
      str += 2;
      big_endian = 1;
    }
  else if (strncasecmp (str, "LE", 2) == 0)
    str += 2;
  else
    inst.error = _("valid endian specifiers are be or le");

  end_of_line (str);

  return big_endian;
}

/* ARM V6 SETEND (argument parse).  Sets the E bit in the CPSR while
   preserving the other bits.

   setend <endian_specifier>, where <endian_specifier> is either
   BE or LE.  */

static void
do_setend (char * str)
{
  if (do_endian_specifier (str))
    inst.instruction |= 0x200;
}

/* ARM V6 SXTH.

   SXTH {<cond>} <Rd>, <Rm>{, <rotation>}
   Condition defaults to COND_ALWAYS.
   Error if any register uses R15.  */

static void
do_sxth (char * str)
{
  int rd, rm;
  expressionS expr;
  int rotation_clear_mask = 0xfffff3ff;
  int rotation_eight_mask = 0x00000400;
  int rotation_sixteen_mask = 0x00000800;
  int rotation_twenty_four_mask = 0x00000c00;

  skip_whitespace (str);
  if ((rd = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  else if (rd == REG_PC || rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  /* Zero out the rotation field.  */
  inst.instruction &= rotation_clear_mask;

  /* Check for lack of optional rotation field.  */
  if (skip_past_comma (&str) == FAIL)
    {
      end_of_line (str);
      return;
    }

  /* Move past 'ROR'.  */
  skip_whitespace (str);
  if (strncasecmp (str, "ROR", 3) == 0)
    str += 3;
  else
    {
      inst.error = _("missing rotation field after comma");
      return;
    }

  /* Get the immediate constant.  */
  skip_whitespace (str);
  if (is_immediate_prefix (* str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    {
      inst.error = _("bad expression");
      return;
    }

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  switch (expr.X_add_number)
    {
    case 0:
      /* Rotation field has already been zeroed.  */
      break;
    case 8:
      inst.instruction |= rotation_eight_mask;
      break;

    case 16:
      inst.instruction |= rotation_sixteen_mask;
      break;

    case 24:
      inst.instruction |= rotation_twenty_four_mask;
      break;

    default:
      inst.error = _("rotation can be 8, 16, 24 or 0 when field is ommited");
      break;
    }

  end_of_line (str);
}

/* ARM V6 SXTAH extracts a 16-bit value from a register, sign
   extends it to 32-bits, and adds the result to a value in another
   register.  You can specify a rotation by 0, 8, 16, or 24 bits
   before extracting the 16-bit value.
   SXTAH{<cond>} <Rd>, <Rn>, <Rm>{, <rotation>}
   Condition defaults to COND_ALWAYS.
   Error if any register uses R15.  */

static void
do_sxtah (char * str)
{
  int rd, rn, rm;
  expressionS expr;
  int rotation_clear_mask = 0xfffff3ff;
  int rotation_eight_mask = 0x00000400;
  int rotation_sixteen_mask = 0x00000800;
  int rotation_twenty_four_mask = 0x00000c00;

  skip_whitespace (str);
  if ((rd = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rn = reg_required_here (&str, 16)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  else if (rd == REG_PC || rn == REG_PC || rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  /* Zero out the rotation field.  */
  inst.instruction &= rotation_clear_mask;

  /* Check for lack of optional rotation field.  */
  if (skip_past_comma (&str) == FAIL)
    {
      end_of_line (str);
      return;
    }

  /* Move past 'ROR'.  */
  skip_whitespace (str);
  if (strncasecmp (str, "ROR", 3) == 0)
    str += 3;
  else
    {
      inst.error = _("missing rotation field after comma");
      return;
    }

  /* Get the immediate constant.  */
  skip_whitespace (str);
  if (is_immediate_prefix (* str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    {
      inst.error = _("bad expression");
      return;
    }

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  switch (expr.X_add_number)
    {
    case 0:
      /* Rotation field has already been zeroed.  */
      break;

    case 8:
      inst.instruction |= rotation_eight_mask;
      break;

    case 16:
      inst.instruction |= rotation_sixteen_mask;
      break;

    case 24:
      inst.instruction |= rotation_twenty_four_mask;
      break;

    default:
      inst.error = _("rotation can be 8, 16, 24 or 0 when field is ommited");
      break;
    }

  end_of_line (str);
}


/* ARM V6 RFE (Return from Exception) loads the PC and CPSR from the
   word at the specified address and the following word
   respectively.
   Unconditionally executed.
   Error if Rn is R15.  */

static void
do_rfe (char * str)
{
  int rn;

  skip_whitespace (str);

  if ((rn = reg_required_here (&str, 16)) == FAIL)
    return;

  if (rn == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  skip_whitespace (str);

  if (*str == '!')
    {
      inst.instruction |= WRITE_BACK;
      str++;
    }
  end_of_line (str);
}

/* ARM V6 REV (Byte Reverse Word) reverses the byte order in a 32-bit
   register (argument parse).
   REV{<cond>} Rd, Rm.
   Condition defaults to COND_ALWAYS.
   Error if Rd or Rm are R15.  */

static void
do_rev (char * str)
{
  int rd, rm;

  skip_whitespace (str);

  if ((rd = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    inst.error = BAD_ARGS;

  else if (rd == REG_PC || rm == REG_PC)
    inst.error = BAD_PC;

  else
    end_of_line (str);
}

/* ARM V6 Perform Two Sixteen Bit Integer Additions. (argument parse).
   QADD16{<cond>} <Rd>, <Rn>, <Rm>
   Condition defaults to COND_ALWAYS.
   Error if Rd, Rn or Rm are R15.  */

static void
do_qadd16 (char * str)
{
  int rd, rm, rn;

  skip_whitespace (str);

  if ((rd = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rn = reg_required_here (&str, 16)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rm = reg_required_here (&str, 0)) == FAIL)
    inst.error = BAD_ARGS;

  else if (rd == REG_PC || rm == REG_PC || rn == REG_PC)
    inst.error = BAD_PC;

  else
    end_of_line (str);
}

static void
do_pkh_core (char * str, int shift)
{
  int rd, rn, rm;

  skip_whitespace (str);
  if (((rd = reg_required_here (&str, 12)) == FAIL)
      || (skip_past_comma (&str) == FAIL)
      || ((rn = reg_required_here (&str, 16)) == FAIL)
      || (skip_past_comma (&str) == FAIL)
      || ((rm = reg_required_here (&str, 0)) == FAIL))
    {
      inst.error = BAD_ARGS;
      return;
    }

  else if (rd == REG_PC || rn == REG_PC || rm == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  /* Check for optional shift immediate constant.  */
  if (skip_past_comma (&str) == FAIL)
    {
      if (shift == SHIFT_ASR_IMMEDIATE)
	{
	  /* If the shift specifier is ommited, turn the instruction
	     into pkhbt rd, rm, rn.  First, switch the instruction
	     code, and clear the rn and rm fields.  */
	  inst.instruction &= 0xfff0f010;
	  /* Now, re-encode the registers.  */
	  inst.instruction |= (rm << 16) | rn;
	}
      return;
    }

  decode_shift (&str, shift);
}

/* ARM V6 Pack Halfword Bottom Top instruction (argument parse).
   PKHBT {<cond>} <Rd>, <Rn>, <Rm> {, LSL #<shift_imm>}
   Condition defaults to COND_ALWAYS.
   Error if Rd, Rn or Rm are R15.  */

static void
do_pkhbt (char * str)
{
  do_pkh_core (str, SHIFT_LSL_IMMEDIATE);
}

/* ARM V6 PKHTB (Argument Parse).  */

static void
do_pkhtb (char * str)
{
  do_pkh_core (str, SHIFT_ASR_IMMEDIATE);
}

/* ARM V6 Load Register Exclusive instruction (argument parse).
   LDREX{,B,D,H}{<cond>} <Rd, [<Rn>]
   Condition defaults to COND_ALWAYS.
   Error if Rd or Rn are R15.
   See ARMARMv6 A4.1.27: LDREX.  */

static void
do_ldrex (char * str)
{
  int rd, rn;

  skip_whitespace (str);

  /* Parse Rd.  */
  if (((rd = reg_required_here (&str, 12)) == FAIL)
      || (skip_past_comma (&str) == FAIL))
    {
      inst.error = BAD_ARGS;
      return;
    }
  else if (rd == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }
  skip_whitespace (str);

  /* Skip past '['.  */
  if ((strlen (str) >= 1)
      &&strncmp (str, "[", 1) == 0)
    str += 1;
  skip_whitespace (str);

  /* Parse Rn.  */
  if ((rn = reg_required_here (&str, 16)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  else if (rn == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }
  skip_whitespace (str);

  /* Skip past ']'.  */
  if ((strlen (str) >= 1)
      && strncmp (str, "]", 1) == 0)
    str += 1;

  end_of_line (str);
}

/* ARM V6 change processor state instruction (argument parse)
      CPS, CPSIE, CSPID .  */

static void
do_cps (char * str)
{
  do_cps_mode (&str);
  end_of_line (str);
}

static void
do_cps_flags (char ** str, int thumb_p)
{
  struct cps_flag
  {
    char character;
    unsigned long arm_value;
    unsigned long thumb_value;
  };
  static struct cps_flag flag_table[] =
  {
    {'a', 0x100, 0x4 },
    {'i', 0x080, 0x2 },
    {'f', 0x040, 0x1 }
  };

  int saw_a_flag = 0;

  skip_whitespace (*str);

  /* Get the a, f and i flags.  */
  while (**str && **str != ',')
    {
      struct cps_flag *p;
      struct cps_flag *q = flag_table + sizeof (flag_table)/sizeof (*p);

      for (p = flag_table; p < q; ++p)
	if (strncasecmp (*str, &p->character, 1) == 0)
	  {
	    inst.instruction |= (thumb_p ? p->thumb_value : p->arm_value);
	    saw_a_flag = 1;
	    break;
	  }
      if (p == q)
	{
	  inst.error = _("unrecognized flag");
	  return;
	}
      (*str)++;
    }

  if (!saw_a_flag)
    inst.error = _("no 'a', 'i', or 'f' flags for 'cps'");
}

static void
do_cpsi (char * str)
{
  do_cps_flags (&str, /*thumb_p=*/0);

  if (skip_past_comma (&str) == SUCCESS)
    {
      skip_whitespace (str);
      do_cps_mode (&str);
    }
  end_of_line (str);
}

/* THUMB V5 breakpoint instruction (argument parse)
	BKPT <immed_8>.  */

static void
do_t_bkpt (char * str)
{
  expressionS expr;
  unsigned long number;

  skip_whitespace (str);

  /* Allow optional leading '#'.  */
  if (is_immediate_prefix (*str))
    str ++;

  memset (& expr, '\0', sizeof (expr));
  if (my_get_expression (& expr, & str)
      || (expr.X_op != O_constant
	  /* As a convenience we allow 'bkpt' without an operand.  */
	  && expr.X_op != O_absent))
    {
      inst.error = _("bad expression");
      return;
    }

  number = expr.X_add_number;

  /* Check it fits an 8 bit unsigned.  */
  if (number != (number & 0xff))
    {
      inst.error = _("immediate value out of range");
      return;
    }

  inst.instruction |= number;

  end_of_line (str);
}

#ifdef OBJ_ELF
static bfd_reloc_code_real_type
arm_parse_reloc (void)
{
  char         id [16];
  char *       ip;
  unsigned int i;
  static struct
  {
    char * str;
    int    len;
    bfd_reloc_code_real_type reloc;
  }
  reloc_map[] =
  {
#define MAP(str,reloc) { str, sizeof (str) - 1, reloc }
    MAP ("(got)",    BFD_RELOC_ARM_GOT32),
    MAP ("(gotoff)", BFD_RELOC_ARM_GOTOFF),
    /* ScottB: Jan 30, 1998 - Added support for parsing "var(PLT)"
       branch instructions generated by GCC for PLT relocs.  */
    MAP ("(plt)",    BFD_RELOC_ARM_PLT32),
    MAP ("(target1)", BFD_RELOC_ARM_TARGET1),
    MAP ("(sbrel)", BFD_RELOC_ARM_SBREL32),
    MAP ("(target2)", BFD_RELOC_ARM_TARGET2),
    { NULL, 0,         BFD_RELOC_UNUSED }
#undef MAP
  };

  for (i = 0, ip = input_line_pointer;
       i < sizeof (id) && (ISALNUM (*ip) || ISPUNCT (*ip));
       i++, ip++)
    id[i] = TOLOWER (*ip);

  for (i = 0; reloc_map[i].str; i++)
    if (strncmp (id, reloc_map[i].str, reloc_map[i].len) == 0)
      break;

  input_line_pointer += reloc_map[i].len;

  return reloc_map[i].reloc;
}
#endif

/* ARM V5 branch-link-exchange (argument parse) for BLX(1) only.
   Expects inst.instruction is set for BLX(1).
   Note: this is cloned from do_branch, and the reloc changed to be a
	new one that can cope with setting one extra bit (the H bit).  */

static void
do_branch25 (char * str)
{
  if (my_get_expression (& inst.reloc.exp, & str))
    return;

#ifdef OBJ_ELF
  {
    char * save_in;

    /* ScottB: February 5, 1998 */
    /* Check to see of PLT32 reloc required for the instruction.  */

    /* arm_parse_reloc() works on input_line_pointer.
       We actually want to parse the operands to the branch instruction
       passed in 'str'.  Save the input pointer and restore it later.  */
    save_in = input_line_pointer;
    input_line_pointer = str;

    if (inst.reloc.exp.X_op == O_symbol
	&& *str == '('
	&& arm_parse_reloc () == BFD_RELOC_ARM_PLT32)
      {
	inst.reloc.type   = BFD_RELOC_ARM_PLT32;
	inst.reloc.pc_rel = 0;
	/* Modify str to point to after parsed operands, otherwise
	   end_of_line() will complain about the (PLT) left in str.  */
	str = input_line_pointer;
      }
    else
      {
	inst.reloc.type   = BFD_RELOC_ARM_PCREL_BLX;
	inst.reloc.pc_rel = 1;
      }

    input_line_pointer = save_in;
  }
#else
  inst.reloc.type   = BFD_RELOC_ARM_PCREL_BLX;
  inst.reloc.pc_rel = 1;
#endif /* OBJ_ELF */

  end_of_line (str);
}

/* ARM V5 branch-link-exchange instruction (argument parse)
     BLX <target_addr>		ie BLX(1)
     BLX{<condition>} <Rm>	ie BLX(2)
   Unfortunately, there are two different opcodes for this mnemonic.
   So, the insns[].value is not used, and the code here zaps values
	into inst.instruction.
   Also, the <target_addr> can be 25 bits, hence has its own reloc.  */

static void
do_blx (char * str)
{
  char * mystr = str;
  int rm;

  skip_whitespace (mystr);
  rm = reg_required_here (& mystr, 0);

  /* The above may set inst.error.  Ignore his opinion.  */
  inst.error = 0;

  if (rm != FAIL)
    {
      /* Arg is a register.
	 Use the condition code our caller put in inst.instruction.
	 Pass ourselves off as a BX with a funny opcode.  */
      inst.instruction |= 0x012fff30;
      do_bx (str);
    }
  else
    {
      /* This must be is BLX <target address>, no condition allowed.  */
      if (inst.instruction != COND_ALWAYS)
	{
	  inst.error = BAD_COND;
	  return;
	}

      inst.instruction = 0xfafffffe;

      /* Process like a B/BL, but with a different reloc.
	 Note that B/BL expecte fffffe, not 0, offset in the opcode table.  */
      do_branch25 (str);
    }
}

/* ARM V5 Thumb BLX (argument parse)
	BLX <target_addr>	which is BLX(1)
	BLX <Rm>		which is BLX(2)
   Unfortunately, there are two different opcodes for this mnemonic.
   So, the tinsns[].value is not used, and the code here zaps values
	into inst.instruction.	*/

static void
do_t_blx (char * str)
{
  char * mystr = str;
  int rm;

  skip_whitespace (mystr);
  inst.instruction = 0x4780;

  /* Note that this call is to the ARM register recognizer.  BLX(2)
     uses the ARM register space, not the Thumb one, so a call to
     thumb_reg() would be wrong.  */
  rm = reg_required_here (& mystr, 3);
  inst.error = 0;

  if (rm != FAIL)
    {
      /* It's BLX(2).  The .instruction was zapped with rm & is final.  */
      inst.size = 2;
    }
  else
    {
      /* No ARM register.  This must be BLX(1).  Change the .instruction.  */
      inst.instruction = 0xf7ffeffe;
      inst.size = 4;

      if (my_get_expression (& inst.reloc.exp, & mystr))
	return;

      inst.reloc.type   = BFD_RELOC_THUMB_PCREL_BLX;
      inst.reloc.pc_rel = 1;
    }

  end_of_line (mystr);
}

/* ARM V5 breakpoint instruction (argument parse)
     BKPT <16 bit unsigned immediate>
     Instruction is not conditional.
	The bit pattern given in insns[] has the COND_ALWAYS condition,
	and it is an error if the caller tried to override that.  */

static void
do_bkpt (char * str)
{
  expressionS expr;
  unsigned long number;

  skip_whitespace (str);

  /* Allow optional leading '#'.  */
  if (is_immediate_prefix (* str))
    str++;

  memset (& expr, '\0', sizeof (expr));

  if (my_get_expression (& expr, & str)
      || (expr.X_op != O_constant
	  /* As a convenience we allow 'bkpt' without an operand.  */
	  && expr.X_op != O_absent))
    {
      inst.error = _("bad expression");
      return;
    }

  number = expr.X_add_number;

  /* Check it fits a 16 bit unsigned.  */
  if (number != (number & 0xffff))
    {
      inst.error = _("immediate value out of range");
      return;
    }

  /* Top 12 of 16 bits to bits 19:8.  */
  inst.instruction |= (number & 0xfff0) << 4;

  /* Bottom 4 of 16 bits to bits 3:0.  */
  inst.instruction |= number & 0xf;

  end_of_line (str);
}

/* THUMB CPS instruction (argument parse).  */

static void
do_t_cps (char * str)
{
  do_cps_flags (&str, /*thumb_p=*/1);
  end_of_line (str);
}

/* Parse and validate that a register is of the right form, this saves
   repeated checking of this information in many similar cases.
   Unlike the 32-bit case we do not insert the register into the opcode
   here, since the position is often unknown until the full instruction
   has been parsed.  */

static int
thumb_reg (char ** strp, int hi_lo)
{
  int reg;

  if ((reg = reg_required_here (strp, -1)) == FAIL)
    return FAIL;

  switch (hi_lo)
    {
    case THUMB_REG_LO:
      if (reg > 7)
	{
	  inst.error = _("lo register required");
	  return FAIL;
	}
      break;

    case THUMB_REG_HI:
      if (reg < 8)
	{
	  inst.error = _("hi register required");
	  return FAIL;
	}
      break;

    default:
      break;
    }

  return reg;
}

static void
thumb_mov_compare (char * str, int move)
{
  int Rd, Rs = FAIL;

  skip_whitespace (str);

  if ((Rd = thumb_reg (&str, THUMB_REG_ANY)) == FAIL
      || skip_past_comma (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (move != THUMB_CPY && is_immediate_prefix (*str))
    {
      str++;
      if (my_get_expression (&inst.reloc.exp, &str))
	return;
    }
  else if ((Rs = thumb_reg (&str, THUMB_REG_ANY)) == FAIL)
    return;

  if (Rs != FAIL)
    {
      if (move != THUMB_CPY && Rs < 8 && Rd < 8)
	{
	  if (move == THUMB_MOVE)
	    /* A move of two lowregs is encoded as ADD Rd, Rs, #0
	       since a MOV instruction produces unpredictable results.  */
	    inst.instruction = T_OPCODE_ADD_I3;
	  else
	    inst.instruction = T_OPCODE_CMP_LR;
	  inst.instruction |= Rd | (Rs << 3);
	}
      else
	{
	  if (move == THUMB_MOVE)
	    inst.instruction = T_OPCODE_MOV_HR;
	  else if (move != THUMB_CPY)
	    inst.instruction = T_OPCODE_CMP_HR;

	  if (Rd > 7)
	    inst.instruction |= THUMB_H1;

	  if (Rs > 7)
	    inst.instruction |= THUMB_H2;

	  inst.instruction |= (Rd & 7) | ((Rs & 7) << 3);
	}
    }
  else
    {
      if (Rd > 7)
	{
	  inst.error = _("only lo regs allowed with immediate");
	  return;
	}

      if (move == THUMB_MOVE)
	inst.instruction = T_OPCODE_MOV_I8;
      else
	inst.instruction = T_OPCODE_CMP_I8;

      inst.instruction |= Rd << 8;

      if (inst.reloc.exp.X_op != O_constant)
	inst.reloc.type = BFD_RELOC_ARM_THUMB_IMM;
      else
	{
	  unsigned value = inst.reloc.exp.X_add_number;

	  if (value > 255)
	    {
	      inst.error = _("invalid immediate");
	      return;
	    }

	  inst.instruction |= value;
	}
    }

  end_of_line (str);
}

/* THUMB CPY instruction (argument parse).  */

static void
do_t_cpy (char * str)
{
  thumb_mov_compare (str, THUMB_CPY);
}

/* THUMB SETEND instruction (argument parse).  */

static void
do_t_setend (char * str)
{
  if (do_endian_specifier (str))
    inst.instruction |= 0x8;
}

/* Parse INSN_TYPE insn STR having a possible IMMEDIATE_SIZE immediate.  */

static unsigned long
check_iwmmxt_insn (char * str,
		   enum iwmmxt_insn_type insn_type,
		   int immediate_size)
{
  int reg = 0;
  const char *  inst_error;
  expressionS expr;
  unsigned long number;

  inst_error = inst.error;
  if (!inst.error)
    inst.error = BAD_ARGS;
  skip_whitespace (str);

  switch (insn_type)
    {
    case check_rd:
      if ((reg = reg_required_here (&str, 12)) == FAIL)
	return FAIL;
      break;

    case check_wr:
       if ((wreg_required_here (&str, 0, IWMMXT_REG_WR)) == FAIL)
	 return FAIL;
       break;

    case check_wrwr:
      if ((wreg_required_here (&str, 12, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL))
	return FAIL;
      break;

    case check_wrwrwr:
      if ((wreg_required_here (&str, 12, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 0, IWMMXT_REG_WR) == FAIL))
	return FAIL;
      break;

    case check_wrwrwcg:
      if ((wreg_required_here (&str, 12, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 0, IWMMXT_REG_WCG) == FAIL))
	return FAIL;
      break;

    case check_tbcst:
      if ((wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 12) == FAIL))
	return FAIL;
      break;

    case check_tmovmsk:
      if ((reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL))
	return FAIL;
      break;

    case check_tmia:
      if ((wreg_required_here (&str, 5, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 0) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 12) == FAIL))
	return FAIL;
      break;

    case check_tmcrr:
      if ((wreg_required_here (&str, 0, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 16) == FAIL))
	return FAIL;
      break;

    case check_tmrrc:
      if ((reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 16) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 0, IWMMXT_REG_WR) == FAIL))
	return FAIL;
      break;

    case check_tmcr:
      if ((wreg_required_here (&str, 16, IWMMXT_REG_WC) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 12) == FAIL))
	return FAIL;
      break;

    case check_tmrc:
      if ((reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WC) == FAIL))
	return FAIL;
      break;

    case check_tinsr:
      if ((wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL))
	return FAIL;
      break;

    case check_textrc:
      if ((reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL))
	return FAIL;
      break;

    case check_waligni:
      if ((wreg_required_here (&str, 12, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 0, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL))
	return FAIL;
      break;

    case check_textrm:
      if ((reg_required_here (&str, 12) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL))
	return FAIL;
      break;

    case check_wshufh:
      if ((wreg_required_here (&str, 12, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL
	   || wreg_required_here (&str, 16, IWMMXT_REG_WR) == FAIL
	   || skip_past_comma (&str) == FAIL))
	return FAIL;
      break;
    }

  if (immediate_size == 0)
    {
      end_of_line (str);
      inst.error = inst_error;
      return reg;
    }
  else
    {
      skip_whitespace (str);

      /* Allow optional leading '#'.  */
      if (is_immediate_prefix (* str))
        str++;

      memset (& expr, '\0', sizeof (expr));

      if (my_get_expression (& expr, & str) || (expr.X_op != O_constant))
        {
          inst.error = _("bad or missing expression");
          return FAIL;
        }

      number = expr.X_add_number;

      if (number != (number & immediate_size))
        {
          inst.error = _("immediate value out of range");
          return FAIL;
        }
      end_of_line (str);
      inst.error = inst_error;
      return number;
    }
}

static void
do_iwmmxt_byte_addr (char * str)
{
  int op = (inst.instruction & 0x300) >> 8;
  int reg;

  inst.instruction &= ~0x300;
  inst.instruction |= (op & 1) << 22 | (op & 2) << 7;

  skip_whitespace (str);

  if ((reg = wreg_required_here (&str, 12, IWMMXT_REG_WR_OR_WC)) == FAIL
      || skip_past_comma (& str) == FAIL
      || cp_byte_address_required_here (&str) == FAIL)
    {
      if (! inst.error)
        inst.error = BAD_ARGS;
    }
  else
    end_of_line (str);

  if (wc_register (reg))
    {
      as_bad (_("non-word size not supported with control register"));
      inst.instruction |=  0xf0000100;
      inst.instruction &= ~0x00400000;
    }
}

static void
do_iwmmxt_tandc (char * str)
{
  int reg;

  reg = check_iwmmxt_insn (str, check_rd, 0);

  if (reg != REG_PC && !inst.error)
    inst.error = _("only r15 allowed here");
}

static void
do_iwmmxt_tbcst (char * str)
{
  check_iwmmxt_insn (str, check_tbcst, 0);
}

static void
do_iwmmxt_textrc (char * str)
{
  unsigned long number;

  if ((number = check_iwmmxt_insn (str, check_textrc, 7)) == (unsigned long) FAIL)
    return;

  inst.instruction |= number & 0x7;
}

static void
do_iwmmxt_textrm (char * str)
{
  unsigned long number;

  if ((number = check_iwmmxt_insn (str, check_textrm, 7)) == (unsigned long) FAIL)
    return;

  inst.instruction |= number & 0x7;
}

static void
do_iwmmxt_tinsr (char * str)
{
  unsigned long number;

  if ((number = check_iwmmxt_insn (str, check_tinsr, 7)) == (unsigned long) FAIL)
    return;

  inst.instruction |= number & 0x7;
}

static void
do_iwmmxt_tmcr (char * str)
{
  check_iwmmxt_insn (str, check_tmcr, 0);
}

static void
do_iwmmxt_tmcrr (char * str)
{
  check_iwmmxt_insn (str, check_tmcrr, 0);
}

static void
do_iwmmxt_tmia (char * str)
{
  check_iwmmxt_insn (str, check_tmia, 0);
}

static void
do_iwmmxt_tmovmsk (char * str)
{
  check_iwmmxt_insn (str, check_tmovmsk, 0);
}

static void
do_iwmmxt_tmrc (char * str)
{
  check_iwmmxt_insn (str, check_tmrc, 0);
}

static void
do_iwmmxt_tmrrc (char * str)
{
  check_iwmmxt_insn (str, check_tmrrc, 0);
}

static void
do_iwmmxt_torc (char * str)
{
  check_iwmmxt_insn (str, check_rd, 0);
}

static void
do_iwmmxt_waligni (char * str)
{
  unsigned long number;

  if ((number = check_iwmmxt_insn (str, check_waligni, 7)) == (unsigned long) FAIL)
    return;

  inst.instruction |= ((number & 0x7) << 20);
}

static void
do_iwmmxt_wmov (char * str)
{
  if (check_iwmmxt_insn (str, check_wrwr, 0) == (unsigned long) FAIL)
    return;

  inst.instruction |= ((inst.instruction >> 16) & 0xf);
}

static void
do_iwmmxt_word_addr (char * str)
{
  int op = (inst.instruction & 0x300) >> 8;
  int reg;

  inst.instruction &= ~0x300;
  inst.instruction |= (op & 1) << 22 | (op & 2) << 7;

  skip_whitespace (str);

  if ((reg = wreg_required_here (&str, 12, IWMMXT_REG_WR_OR_WC)) == FAIL
      || skip_past_comma (& str) == FAIL
      || cp_address_required_here (& str, CP_WB_OK) == FAIL)
    {
      if (! inst.error)
        inst.error = BAD_ARGS;
    }
  else
    end_of_line (str);

  if (wc_register (reg))
    {
      if ((inst.instruction & COND_MASK) != COND_ALWAYS)
	as_bad (_("conditional execution not supported with control register"));
      if (op != 2)
	as_bad (_("non-word size not supported with control register"));
      inst.instruction |=  0xf0000100;
      inst.instruction &= ~0x00400000;
    }
}

static void
do_iwmmxt_wrwr (char * str)
{
  check_iwmmxt_insn (str, check_wrwr, 0);
}

static void
do_iwmmxt_wrwrwcg (char * str)
{
  check_iwmmxt_insn (str, check_wrwrwcg, 0);
}

static void
do_iwmmxt_wrwrwr (char * str)
{
  check_iwmmxt_insn (str, check_wrwrwr, 0);
}

static void
do_iwmmxt_wshufh (char * str)
{
  unsigned long number;

  if ((number = check_iwmmxt_insn (str, check_wshufh, 0xff)) == (unsigned long) FAIL)
    return;

  inst.instruction |= ((number & 0xf0) << 16) | (number & 0xf);
}

static void
do_iwmmxt_wzero (char * str)
{
  if (check_iwmmxt_insn (str, check_wr, 0) == (unsigned long) FAIL)
    return;

  inst.instruction |= ((inst.instruction & 0xf) << 12) | ((inst.instruction & 0xf) << 16);
}

/* Xscale multiply-accumulate (argument parse)
     MIAcc   acc0,Rm,Rs
     MIAPHcc acc0,Rm,Rs
     MIAxycc acc0,Rm,Rs.  */

static void
do_xsc_mia (char * str)
{
  int rs;
  int rm;

  if (accum0_required_here (& str) == FAIL)
    inst.error = ERR_NO_ACCUM;

  else if (skip_past_comma (& str) == FAIL
	   || (rm = reg_required_here (& str, 0)) == FAIL)
    inst.error = BAD_ARGS;

  else if (skip_past_comma (& str) == FAIL
	   || (rs = reg_required_here (& str, 12)) == FAIL)
    inst.error = BAD_ARGS;

  /* inst.instruction has now been zapped with both rm and rs.  */
  else if (rm == REG_PC || rs == REG_PC)
    inst.error = BAD_PC;	/* Undefined result if rm or rs is R15.  */

  else
    end_of_line (str);
}

/* Xscale move-accumulator-register (argument parse)

     MARcc   acc0,RdLo,RdHi.  */

static void
do_xsc_mar (char * str)
{
  int rdlo, rdhi;

  if (accum0_required_here (& str) == FAIL)
    inst.error = ERR_NO_ACCUM;

  else if (skip_past_comma (& str) == FAIL
	   || (rdlo = reg_required_here (& str, 12)) == FAIL)
    inst.error = BAD_ARGS;

  else if (skip_past_comma (& str) == FAIL
	   || (rdhi = reg_required_here (& str, 16)) == FAIL)
    inst.error = BAD_ARGS;

  /* inst.instruction has now been zapped with both rdlo and rdhi.  */
  else if (rdlo == REG_PC || rdhi == REG_PC)
    inst.error = BAD_PC;	/* Undefined result if rdlo or rdhi is R15.  */

  else
    end_of_line (str);
}

/* Xscale move-register-accumulator (argument parse)

     MRAcc   RdLo,RdHi,acc0.  */

static void
do_xsc_mra (char * str)
{
  int rdlo;
  int rdhi;

  skip_whitespace (str);

  if ((rdlo = reg_required_here (& str, 12)) == FAIL)
    inst.error = BAD_ARGS;

  else if (skip_past_comma (& str) == FAIL
	   || (rdhi = reg_required_here (& str, 16)) == FAIL)
    inst.error = BAD_ARGS;

  else if  (skip_past_comma (& str) == FAIL
	    || accum0_required_here (& str) == FAIL)
    inst.error = ERR_NO_ACCUM;

  /* inst.instruction has now been zapped with both rdlo and rdhi.  */
  else if (rdlo == rdhi)
    inst.error = BAD_ARGS;	/* Undefined result if 2 writes to same reg.  */

  else if (rdlo == REG_PC || rdhi == REG_PC)
    inst.error = BAD_PC;	/* Undefined result if rdlo or rdhi is R15.  */
  else
    end_of_line (str);
}

static int
ldst_extend (char ** str)
{
  int add = INDEX_UP;

  switch (**str)
    {
    case '#':
    case '$':
      (*str)++;
      if (my_get_expression (& inst.reloc.exp, str))
	return FAIL;

      if (inst.reloc.exp.X_op == O_constant)
	{
	  int value = inst.reloc.exp.X_add_number;

	  if (value < -4095 || value > 4095)
	    {
	      inst.error = _("address offset too large");
	      return FAIL;
	    }

	  if (value < 0)
	    {
	      value = -value;
	      add = 0;
	    }

	  inst.instruction |= add | value;
	}
      else
	{
	  inst.reloc.type = BFD_RELOC_ARM_OFFSET_IMM;
	  inst.reloc.pc_rel = 0;
	}
      return SUCCESS;

    case '-':
      add = 0;
      /* Fall through.  */

    case '+':
      (*str)++;
      /* Fall through.  */

    default:
      if (reg_required_here (str, 0) == FAIL)
	return FAIL;

      inst.instruction |= add | OFFSET_REG;
      if (skip_past_comma (str) == SUCCESS)
	return decode_shift (str, SHIFT_IMMEDIATE);

      return SUCCESS;
    }
}

/* ARMv5TE: Preload-Cache

    PLD <addr_mode>

  Syntactically, like LDR with B=1, W=0, L=1.  */

static void
do_pld (char * str)
{
  int rd;

  skip_whitespace (str);

  if (* str != '[')
    {
      inst.error = _("'[' expected after PLD mnemonic");
      return;
    }

  ++str;
  skip_whitespace (str);

  if ((rd = reg_required_here (& str, 16)) == FAIL)
    return;

  skip_whitespace (str);

  if (*str == ']')
    {
      /* [Rn], ... ?  */
      ++str;
      skip_whitespace (str);

      /* Post-indexed addressing is not allowed with PLD.  */
      if (skip_past_comma (&str) == SUCCESS)
	{
	  inst.error
	    = _("post-indexed expression used in preload instruction");
	  return;
	}
      else if (*str == '!') /* [Rn]! */
	{
	  inst.error = _("writeback used in preload instruction");
	  ++str;
	}
      else /* [Rn] */
	inst.instruction |= INDEX_UP | PRE_INDEX;
    }
  else /* [Rn, ...] */
    {
      if (skip_past_comma (& str) == FAIL)
	{
	  inst.error = _("pre-indexed expression expected");
	  return;
	}

      if (ldst_extend (&str) == FAIL)
	return;

      skip_whitespace (str);

      if (* str != ']')
	{
	  inst.error = _("missing ]");
	  return;
	}

      ++ str;
      skip_whitespace (str);

      if (* str == '!') /* [Rn]! */
	{
	  inst.error = _("writeback used in preload instruction");
	  ++ str;
	}

      inst.instruction |= PRE_INDEX;
    }

  end_of_line (str);
}

/* ARMv5TE load-consecutive (argument parse)
   Mode is like LDRH.

     LDRccD R, mode
     STRccD R, mode.  */

static void
do_ldrd (char * str)
{
  int rd;
  int rn;

  skip_whitespace (str);

  if ((rd = reg_required_here (& str, 12)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL
      || (rn = ld_mode_required_here (& str)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  /* inst.instruction has now been zapped with Rd and the addressing mode.  */
  if (rd & 1)		/* Unpredictable result if Rd is odd.  */
    {
      inst.error = _("destination register must be even");
      return;
    }

  if (rd == REG_LR)
    {
      inst.error = _("r14 not allowed here");
      return;
    }

  if (((rd == rn) || (rd + 1 == rn))
      && ((inst.instruction & WRITE_BACK)
	  || (!(inst.instruction & PRE_INDEX))))
    as_warn (_("pre/post-indexing used when modified address register is destination"));

  /* For an index-register load, the index register must not overlap the
     destination (even if not write-back).  */
  if ((inst.instruction & V4_STR_BIT) == 0
      && (inst.instruction & HWOFFSET_IMM) == 0)
    {
      int rm = inst.instruction & 0x0000000f;

      if (rm == rd || (rm == rd + 1))
	as_warn (_("ldrd destination registers must not overlap index register"));
    }

  end_of_line (str);
}

/* Returns the index into fp_values of a floating point number,
   or -1 if not in the table.  */

static int
my_get_float_expression (char ** str)
{
  LITTLENUM_TYPE words[MAX_LITTLENUMS];
  char *         save_in;
  expressionS    exp;
  int            i;
  int            j;

  memset (words, 0, MAX_LITTLENUMS * sizeof (LITTLENUM_TYPE));

  /* Look for a raw floating point number.  */
  if ((save_in = atof_ieee (*str, 'x', words)) != NULL
      && is_end_of_line[(unsigned char) *save_in])
    {
      for (i = 0; i < NUM_FLOAT_VALS; i++)
	{
	  for (j = 0; j < MAX_LITTLENUMS; j++)
	    {
	      if (words[j] != fp_values[i][j])
		break;
	    }

	  if (j == MAX_LITTLENUMS)
	    {
	      *str = save_in;
	      return i;
	    }
	}
    }

  /* Try and parse a more complex expression, this will probably fail
     unless the code uses a floating point prefix (eg "0f").  */
  save_in = input_line_pointer;
  input_line_pointer = *str;
  if (expression (&exp) == absolute_section
      && exp.X_op == O_big
      && exp.X_add_number < 0)
    {
      /* FIXME: 5 = X_PRECISION, should be #define'd where we can use it.
	 Ditto for 15.  */
      if (gen_to_words (words, 5, (long) 15) == 0)
	{
	  for (i = 0; i < NUM_FLOAT_VALS; i++)
	    {
	      for (j = 0; j < MAX_LITTLENUMS; j++)
		{
		  if (words[j] != fp_values[i][j])
		    break;
		}

	      if (j == MAX_LITTLENUMS)
		{
		  *str = input_line_pointer;
		  input_line_pointer = save_in;
		  return i;
		}
	    }
	}
    }

  *str = input_line_pointer;
  input_line_pointer = save_in;
  return -1;
}

/* We handle all bad expressions here, so that we can report the faulty
   instruction in the error message.  */
void
md_operand (expressionS * expr)
{
  if (in_my_get_expression)
    {
      expr->X_op = O_illegal;
      if (inst.error == NULL)
	inst.error = _("bad expression");
    }
}

/* Do those data_ops which can take a negative immediate constant
   by altering the instruction.  A bit of a hack really.
        MOV <-> MVN
        AND <-> BIC
        ADC <-> SBC
        by inverting the second operand, and
        ADD <-> SUB
        CMP <-> CMN
        by negating the second operand.  */

static int
negate_data_op (unsigned long * instruction,
		unsigned long   value)
{
  int op, new_inst;
  unsigned long negated, inverted;

  negated = validate_immediate (-value);
  inverted = validate_immediate (~value);

  op = (*instruction >> DATA_OP_SHIFT) & 0xf;
  switch (op)
    {
      /* First negates.  */
    case OPCODE_SUB:             /* ADD <-> SUB  */
      new_inst = OPCODE_ADD;
      value = negated;
      break;

    case OPCODE_ADD:
      new_inst = OPCODE_SUB;
      value = negated;
      break;

    case OPCODE_CMP:             /* CMP <-> CMN  */
      new_inst = OPCODE_CMN;
      value = negated;
      break;

    case OPCODE_CMN:
      new_inst = OPCODE_CMP;
      value = negated;
      break;

      /* Now Inverted ops.  */
    case OPCODE_MOV:             /* MOV <-> MVN  */
      new_inst = OPCODE_MVN;
      value = inverted;
      break;

    case OPCODE_MVN:
      new_inst = OPCODE_MOV;
      value = inverted;
      break;

    case OPCODE_AND:             /* AND <-> BIC  */
      new_inst = OPCODE_BIC;
      value = inverted;
      break;

    case OPCODE_BIC:
      new_inst = OPCODE_AND;
      value = inverted;
      break;

    case OPCODE_ADC:              /* ADC <-> SBC  */
      new_inst = OPCODE_SBC;
      value = inverted;
      break;

    case OPCODE_SBC:
      new_inst = OPCODE_ADC;
      value = inverted;
      break;

      /* We cannot do anything.  */
    default:
      return FAIL;
    }

  if (value == (unsigned) FAIL)
    return FAIL;

  *instruction &= OPCODE_MASK;
  *instruction |= new_inst << DATA_OP_SHIFT;
  return value;
}

static int
data_op2 (char ** str)
{
  int value;
  expressionS expr;

  skip_whitespace (* str);

  if (reg_required_here (str, 0) != FAIL)
    {
      if (skip_past_comma (str) == SUCCESS)
	/* Shift operation on register.  */
	return decode_shift (str, NO_SHIFT_RESTRICT);

      return SUCCESS;
    }
  else
    {
      /* Immediate expression.  */
      if (is_immediate_prefix (**str))
	{
	  (*str)++;
	  inst.error = NULL;

	  if (my_get_expression (&inst.reloc.exp, str))
	    return FAIL;

	  if (inst.reloc.exp.X_add_symbol)
	    {
	      inst.reloc.type = BFD_RELOC_ARM_IMMEDIATE;
	      inst.reloc.pc_rel = 0;
	    }
	  else
	    {
	      if (skip_past_comma (str) == SUCCESS)
		{
		  /* #x, y -- ie explicit rotation by Y.  */
		  if (my_get_expression (&expr, str))
		    return FAIL;

		  if (expr.X_op != O_constant)
		    {
		      inst.error = _("constant expression expected");
		      return FAIL;
		    }

		  /* Rotate must be a multiple of 2.  */
		  if (((unsigned) expr.X_add_number) > 30
		      || (expr.X_add_number & 1) != 0
		      || ((unsigned) inst.reloc.exp.X_add_number) > 255)
		    {
		      inst.error = _("invalid constant");
		      return FAIL;
		    }
		  inst.instruction |= INST_IMMEDIATE;
		  inst.instruction |= inst.reloc.exp.X_add_number;
		  inst.instruction |= expr.X_add_number << 7;
		  return SUCCESS;
		}

	      /* Implicit rotation, select a suitable one.  */
	      value = validate_immediate (inst.reloc.exp.X_add_number);

	      if (value == FAIL)
		{
		  /* Can't be done.  Perhaps the code reads something like
		     "add Rd, Rn, #-n", where "sub Rd, Rn, #n" would be OK.  */
		  if ((value = negate_data_op (&inst.instruction,
					       inst.reloc.exp.X_add_number))
		      == FAIL)
		    {
		      inst.error = _("invalid constant");
		      return FAIL;
		    }
		}

	      inst.instruction |= value;
	    }

	  inst.instruction |= INST_IMMEDIATE;
	  return SUCCESS;
	}

      (*str)++;
      inst.error = _("register or shift expression expected");
      return FAIL;
    }
}

static int
fp_op2 (char ** str)
{
  skip_whitespace (* str);

  if (fp_reg_required_here (str, 0) != FAIL)
    return SUCCESS;
  else
    {
      /* Immediate expression.  */
      if (*((*str)++) == '#')
	{
	  int i;

	  inst.error = NULL;

	  skip_whitespace (* str);

	  /* First try and match exact strings, this is to guarantee
	     that some formats will work even for cross assembly.  */

	  for (i = 0; fp_const[i]; i++)
	    {
	      if (strncmp (*str, fp_const[i], strlen (fp_const[i])) == 0)
		{
		  char *start = *str;

		  *str += strlen (fp_const[i]);
		  if (is_end_of_line[(unsigned char) **str])
		    {
		      inst.instruction |= i + 8;
		      return SUCCESS;
		    }
		  *str = start;
		}
	    }

	  /* Just because we didn't get a match doesn't mean that the
	     constant isn't valid, just that it is in a format that we
	     don't automatically recognize.  Try parsing it with
	     the standard expression routines.  */
	  if ((i = my_get_float_expression (str)) >= 0)
	    {
	      inst.instruction |= i + 8;
	      return SUCCESS;
	    }

	  inst.error = _("invalid floating point immediate expression");
	  return FAIL;
	}
      inst.error =
	_("floating point register or immediate expression expected");
      return FAIL;
    }
}

static void
do_arit (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL
      || skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 16) == FAIL
      || skip_past_comma (&str) == FAIL
      || data_op2 (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_adr (char * str)
{
  /* This is a pseudo-op of the form "adr rd, label" to be converted
     into a relative address of the form "add rd, pc, #label-.-8".  */
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL
      || skip_past_comma (&str) == FAIL
      || my_get_expression (&inst.reloc.exp, &str))
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  /* Frag hacking will turn this into a sub instruction if the offset turns
     out to be negative.  */
  inst.reloc.type = BFD_RELOC_ARM_IMMEDIATE;
#ifndef TE_WINCE
  inst.reloc.exp.X_add_number -= 8; /* PC relative adjust.  */
#endif
  inst.reloc.pc_rel = 1;

  end_of_line (str);
}

static void
do_adrl (char * str)
{
  /* This is a pseudo-op of the form "adrl rd, label" to be converted
     into a relative address of the form:
     add rd, pc, #low(label-.-8)"
     add rd, rd, #high(label-.-8)"  */

  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL
      || skip_past_comma (&str) == FAIL
      || my_get_expression (&inst.reloc.exp, &str))
    {
      if (!inst.error)
	inst.error = BAD_ARGS;

      return;
    }

  end_of_line (str);
  /* Frag hacking will turn this into a sub instruction if the offset turns
     out to be negative.  */
  inst.reloc.type              = BFD_RELOC_ARM_ADRL_IMMEDIATE;
#ifndef TE_WINCE
  inst.reloc.exp.X_add_number -= 8; /* PC relative adjust  */
#endif
  inst.reloc.pc_rel            = 1;
  inst.size                    = INSN_SIZE * 2;
}

static void
do_cmp (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 16) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || data_op2 (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_mov (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || data_op2 (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_ldst (char * str)
{
  int pre_inc = 0;
  int conflict_reg;
  int value;

  skip_whitespace (str);

  if ((conflict_reg = reg_required_here (&str, 12)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL)
    {
      inst.error = _("address expected");
      return;
    }

  if (*str == '[')
    {
      int reg;

      str++;

      skip_whitespace (str);

      if ((reg = reg_required_here (&str, 16)) == FAIL)
	return;

      /* Conflicts can occur on stores as well as loads.  */
      conflict_reg = (conflict_reg == reg);

      skip_whitespace (str);

      if (*str == ']')
	{
	  str ++;

	  if (skip_past_comma (&str) == SUCCESS)
	    {
	      /* [Rn],... (post inc)  */
	      if (ldst_extend (&str) == FAIL)
		return;
	      if (conflict_reg)
		as_warn (_("%s register same as write-back base"),
			 ((inst.instruction & LOAD_BIT)
			  ? _("destination") : _("source")));
	    }
	  else
	    {
	      /* [Rn]  */
	      skip_whitespace (str);

	      if (*str == '!')
		{
		  if (conflict_reg)
		    as_warn (_("%s register same as write-back base"),
			     ((inst.instruction & LOAD_BIT)
			      ? _("destination") : _("source")));
		  str++;
		  inst.instruction |= WRITE_BACK;
		}

	      inst.instruction |= INDEX_UP;
	      pre_inc = 1;
	    }
	}
      else
	{
	  /* [Rn,...]  */
	  if (skip_past_comma (&str) == FAIL)
	    {
	      inst.error = _("pre-indexed expression expected");
	      return;
	    }

	  pre_inc = 1;
	  if (ldst_extend (&str) == FAIL)
	    return;

	  skip_whitespace (str);

	  if (*str++ != ']')
	    {
	      inst.error = _("missing ]");
	      return;
	    }

	  skip_whitespace (str);

	  if (*str == '!')
	    {
	      if (conflict_reg)
		as_warn (_("%s register same as write-back base"),
			 ((inst.instruction & LOAD_BIT)
			  ? _("destination") : _("source")));
	      str++;
	      inst.instruction |= WRITE_BACK;
	    }
	}
    }
  else if (*str == '=')
    {
      if ((inst.instruction & LOAD_BIT) == 0)
	{
	  inst.error = _("invalid pseudo operation");
	  return;
	}

      /* Parse an "ldr Rd, =expr" instruction; this is another pseudo op.  */
      str++;

      skip_whitespace (str);

      if (my_get_expression (&inst.reloc.exp, &str))
	return;

      if (inst.reloc.exp.X_op != O_constant
	  && inst.reloc.exp.X_op != O_symbol)
	{
	  inst.error = _("constant expression expected");
	  return;
	}

      if (inst.reloc.exp.X_op == O_constant)
	{
	  value = validate_immediate (inst.reloc.exp.X_add_number);

	  if (value != FAIL)
	    {
	      /* This can be done with a mov instruction.  */
	      inst.instruction &= LITERAL_MASK;
	      inst.instruction |= (INST_IMMEDIATE
				   | (OPCODE_MOV << DATA_OP_SHIFT));
	      inst.instruction |= value & 0xfff;
	      end_of_line (str);
	      return;
	    }

	  value = validate_immediate (~inst.reloc.exp.X_add_number);

	  if (value != FAIL)
	    {
	      /* This can be done with a mvn instruction.  */
	      inst.instruction &= LITERAL_MASK;
	      inst.instruction |= (INST_IMMEDIATE
				   | (OPCODE_MVN << DATA_OP_SHIFT));
	      inst.instruction |= value & 0xfff;
	      end_of_line (str);
	      return;
	    }
	}

      /* Insert into literal pool.  */
      if (add_to_lit_pool () == FAIL)
	{
	  if (!inst.error)
	    inst.error = _("literal pool insertion failed");
	  return;
	}

      /* Change the instruction exp to point to the pool.  */
      inst.reloc.type = BFD_RELOC_ARM_LITERAL;
      inst.reloc.pc_rel = 1;
      inst.instruction |= (REG_PC << 16);
      pre_inc = 1;
    }
  else
    {
      if (my_get_expression (&inst.reloc.exp, &str))
	return;

      inst.reloc.type = BFD_RELOC_ARM_OFFSET_IMM;
#ifndef TE_WINCE
      /* PC rel adjust.  */
      inst.reloc.exp.X_add_number -= 8;
#endif
      inst.reloc.pc_rel = 1;
      inst.instruction |= (REG_PC << 16);
      pre_inc = 1;
    }

  inst.instruction |= (pre_inc ? PRE_INDEX : 0);
  end_of_line (str);
}

static void
do_ldstt (char * str)
{
  int conflict_reg;

  skip_whitespace (str);

  if ((conflict_reg = reg_required_here (& str, 12)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL)
    {
      inst.error = _("address expected");
      return;
    }

  if (*str == '[')
    {
      int reg;

      str++;

      skip_whitespace (str);

      if ((reg = reg_required_here (&str, 16)) == FAIL)
	return;

      /* ldrt/strt always use post-indexed addressing, so if the base is
	 the same as Rd, we warn.  */
      if (conflict_reg == reg)
	as_warn (_("%s register same as write-back base"),
		 ((inst.instruction & LOAD_BIT)
		  ? _("destination") : _("source")));

      skip_whitespace (str);

      if (*str == ']')
	{
	  str ++;

	  if (skip_past_comma (&str) == SUCCESS)
	    {
	      /* [Rn],... (post inc)  */
	      if (ldst_extend (&str) == FAIL)
		return;
	    }
	  else
	    {
	      /* [Rn]  */
	      skip_whitespace (str);

	      /* Skip a write-back '!'.  */
	      if (*str == '!')
		str++;

	      inst.instruction |= INDEX_UP;
	    }
	}
      else
	{
	  inst.error = _("post-indexed expression expected");
	  return;
	}
    }
  else
    {
      inst.error = _("post-indexed expression expected");
      return;
    }

  end_of_line (str);
}

/* Halfword and signed-byte load/store operations.  */

static void
do_ldstv4 (char * str)
{
  int pre_inc = 0;
  int conflict_reg;
  int value;

  skip_whitespace (str);

  if ((conflict_reg = reg_required_here (& str, 12)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (& str) == FAIL)
    {
      inst.error = _("address expected");
      return;
    }

  if (*str == '[')
    {
      int reg;

      str++;

      skip_whitespace (str);

      if ((reg = reg_required_here (&str, 16)) == FAIL)
	return;

      /* Conflicts can occur on stores as well as loads.  */
      conflict_reg = (conflict_reg == reg);

      skip_whitespace (str);

      if (*str == ']')
	{
	  str ++;

	  if (skip_past_comma (&str) == SUCCESS)
	    {
	      /* [Rn],... (post inc)  */
	      if (ldst_extend_v4 (&str) == FAIL)
		return;
	      if (conflict_reg)
		as_warn (_("%s register same as write-back base"),
			 ((inst.instruction & LOAD_BIT)
			  ? _("destination") : _("source")));
	    }
	  else
	    {
	      /* [Rn]  */
	      inst.instruction |= HWOFFSET_IMM;

	      skip_whitespace (str);

	      if (*str == '!')
		{
		  if (conflict_reg)
		    as_warn (_("%s register same as write-back base"),
			     ((inst.instruction & LOAD_BIT)
			      ? _("destination") : _("source")));
		  str++;
		  inst.instruction |= WRITE_BACK;
		}

	      inst.instruction |= INDEX_UP;
	      pre_inc = 1;
	    }
	}
      else
	{
	  /* [Rn,...]  */
	  if (skip_past_comma (&str) == FAIL)
	    {
	      inst.error = _("pre-indexed expression expected");
	      return;
	    }

	  pre_inc = 1;
	  if (ldst_extend_v4 (&str) == FAIL)
	    return;

	  skip_whitespace (str);

	  if (*str++ != ']')
	    {
	      inst.error = _("missing ]");
	      return;
	    }

	  skip_whitespace (str);

	  if (*str == '!')
	    {
	      if (conflict_reg)
		as_warn (_("%s register same as write-back base"),
			 ((inst.instruction & LOAD_BIT)
			  ? _("destination") : _("source")));
	      str++;
	      inst.instruction |= WRITE_BACK;
	    }
	}
    }
  else if (*str == '=')
    {
      if ((inst.instruction & LOAD_BIT) == 0)
	{
	  inst.error = _("invalid pseudo operation");
	  return;
	}

      /* XXX Does this work correctly for half-word/byte ops?  */
      /* Parse an "ldr Rd, =expr" instruction; this is another pseudo op.  */
      str++;

      skip_whitespace (str);

      if (my_get_expression (&inst.reloc.exp, &str))
	return;

      if (inst.reloc.exp.X_op != O_constant
	  && inst.reloc.exp.X_op != O_symbol)
	{
	  inst.error = _("constant expression expected");
	  return;
	}

      if (inst.reloc.exp.X_op == O_constant)
	{
	  value = validate_immediate (inst.reloc.exp.X_add_number);

	  if (value != FAIL)
	    {
	      /* This can be done with a mov instruction.  */
	      inst.instruction &= LITERAL_MASK;
	      inst.instruction |= INST_IMMEDIATE | (OPCODE_MOV << DATA_OP_SHIFT);
	      inst.instruction |= value & 0xfff;
	      end_of_line (str);
	      return;
	    }

	  value = validate_immediate (~ inst.reloc.exp.X_add_number);

	  if (value != FAIL)
	    {
	      /* This can be done with a mvn instruction.  */
	      inst.instruction &= LITERAL_MASK;
	      inst.instruction |= INST_IMMEDIATE | (OPCODE_MVN << DATA_OP_SHIFT);
	      inst.instruction |= value & 0xfff;
	      end_of_line (str);
	      return;
	    }
	}

      /* Insert into literal pool.  */
      if (add_to_lit_pool () == FAIL)
	{
	  if (!inst.error)
	    inst.error = _("literal pool insertion failed");
	  return;
	}

      /* Change the instruction exp to point to the pool.  */
      inst.instruction |= HWOFFSET_IMM;
      inst.reloc.type = BFD_RELOC_ARM_HWLITERAL;
      inst.reloc.pc_rel = 1;
      inst.instruction |= (REG_PC << 16);
      pre_inc = 1;
    }
  else
    {
      if (my_get_expression (&inst.reloc.exp, &str))
	return;

      inst.instruction |= HWOFFSET_IMM;
      inst.reloc.type = BFD_RELOC_ARM_OFFSET_IMM8;
#ifndef TE_WINCE
      /* PC rel adjust.  */
      inst.reloc.exp.X_add_number -= 8;
#endif
      inst.reloc.pc_rel = 1;
      inst.instruction |= (REG_PC << 16);
      pre_inc = 1;
    }

  inst.instruction |= (pre_inc ? PRE_INDEX : 0);
  end_of_line (str);
}

static long
reg_list (char ** strp)
{
  char * str = * strp;
  long   range = 0;
  int    another_range;

  /* We come back here if we get ranges concatenated by '+' or '|'.  */
  do
    {
      another_range = 0;

      if (*str == '{')
	{
	  int in_range = 0;
	  int cur_reg = -1;

	  str++;
	  do
	    {
	      int reg;

	      skip_whitespace (str);

	      if ((reg = reg_required_here (& str, -1)) == FAIL)
		return FAIL;

	      if (in_range)
		{
		  int i;

		  if (reg <= cur_reg)
		    {
		      inst.error = _("bad range in register list");
		      return FAIL;
		    }

		  for (i = cur_reg + 1; i < reg; i++)
		    {
		      if (range & (1 << i))
			as_tsktsk
			  (_("Warning: duplicated register (r%d) in register list"),
			   i);
		      else
			range |= 1 << i;
		    }
		  in_range = 0;
		}

	      if (range & (1 << reg))
		as_tsktsk (_("Warning: duplicated register (r%d) in register list"),
			   reg);
	      else if (reg <= cur_reg)
		as_tsktsk (_("Warning: register range not in ascending order"));

	      range |= 1 << reg;
	      cur_reg = reg;
	    }
	  while (skip_past_comma (&str) != FAIL
		 || (in_range = 1, *str++ == '-'));
	  str--;
	  skip_whitespace (str);

	  if (*str++ != '}')
	    {
	      inst.error = _("missing `}'");
	      return FAIL;
	    }
	}
      else
	{
	  expressionS expr;

	  if (my_get_expression (&expr, &str))
	    return FAIL;

	  if (expr.X_op == O_constant)
	    {
	      if (expr.X_add_number
		  != (expr.X_add_number & 0x0000ffff))
		{
		  inst.error = _("invalid register mask");
		  return FAIL;
		}

	      if ((range & expr.X_add_number) != 0)
		{
		  int regno = range & expr.X_add_number;

		  regno &= -regno;
		  regno = (1 << regno) - 1;
		  as_tsktsk
		    (_("Warning: duplicated register (r%d) in register list"),
		     regno);
		}

	      range |= expr.X_add_number;
	    }
	  else
	    {
	      if (inst.reloc.type != 0)
		{
		  inst.error = _("expression too complex");
		  return FAIL;
		}

	      memcpy (&inst.reloc.exp, &expr, sizeof (expressionS));
	      inst.reloc.type = BFD_RELOC_ARM_MULTI;
	      inst.reloc.pc_rel = 0;
	    }
	}

      skip_whitespace (str);

      if (*str == '|' || *str == '+')
	{
	  str++;
	  another_range = 1;
	}
    }
  while (another_range);

  *strp = str;
  return range;
}

static void
do_ldmstm (char * str)
{
  int base_reg;
  long range;

  skip_whitespace (str);

  base_reg = (inst.instruction >> 16) & 0xf;
  if (base_reg == 0)
    {
      if ((base_reg = reg_required_here (&str, 16)) == FAIL)
	return;

      if (base_reg == REG_PC)
	{
	  inst.error = _("r15 not allowed as base register");
	  return;
	}

      skip_whitespace (str);

      if (*str == '!')
	{
	  inst.instruction |= WRITE_BACK;
	  str++;
	}

      if (skip_past_comma (&str) == FAIL)
	{
	  if (! inst.error)
	    inst.error = BAD_ARGS;
	  return;
	}
    }

  if ((range = reg_list (&str)) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (*str == '^')
    {
      str++;
      inst.instruction |= LDM_TYPE_2_OR_3;
    }

  if (inst.instruction & WRITE_BACK)
    {
      /* Check for unpredictable uses of writeback.  */
      if (inst.instruction & LOAD_BIT)
	{
	  /* Not allowed in LDM type 2.  */
	  if ((inst.instruction & LDM_TYPE_2_OR_3)
	      && ((range & (1 << REG_PC)) == 0))
	    as_warn (_("writeback of base register is UNPREDICTABLE"));
	  /* Only allowed if base reg not in list for other types.  */
	  else if (range & (1 << base_reg))
	    as_warn (_("writeback of base register when in register list is UNPREDICTABLE"));
	}
      else /* STM.  */
	{
	  /* Not allowed for type 2.  */
	  if (inst.instruction & LDM_TYPE_2_OR_3)
	    as_warn (_("writeback of base register is UNPREDICTABLE"));
	  /* Only allowed if base reg not in list, or first in list.  */
	  else if ((range & (1 << base_reg))
		   && (range & ((1 << base_reg) - 1)))
	    as_warn (_("if writeback register is in list, it must be the lowest reg in the list"));
	}
    }

  inst.instruction |= range;
  end_of_line (str);
}

static void
do_smi (char * str)
{
  skip_whitespace (str);

  /* Allow optional leading '#'.  */
  if (is_immediate_prefix (*str))
    str++;

  if (my_get_expression (& inst.reloc.exp, & str))
    return;

  inst.reloc.type = BFD_RELOC_ARM_SMI;
  inst.reloc.pc_rel = 0;
  end_of_line (str);
}

static void
do_swi (char * str)
{
  skip_whitespace (str);

  /* Allow optional leading '#'.  */
  if (is_immediate_prefix (*str))
    str++;

  if (my_get_expression (& inst.reloc.exp, & str))
    return;

  inst.reloc.type = BFD_RELOC_ARM_SWI;
  inst.reloc.pc_rel = 0;
  end_of_line (str);
}

static void
do_dsb (char * str)
{
  skip_whitespace (str);
  /* Allow optional leading '#'.  */
  if (is_immediate_prefix (*str))
    str++;

  end_of_line (str);
}

static void
do_movwt (char * str)
{
  expressionS expr;
  int reg;

  skip_whitespace (str);

  if ((reg = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL)
    return;

  if (reg == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (is_immediate_prefix (*str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    return;

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  if ((expr.X_add_number & 0xffff0000) != 0)
    {
      inst.error = _("invalid unsigned 16-bit value");
      return;
    }

  inst.instruction |= (expr.X_add_number & 0xf000) << 4;
  inst.instruction |= (expr.X_add_number & 0x0fff);
}

static void
do_bfci (char * str)
{
  expressionS expr;
  unsigned long lsb;
  unsigned long width;
  int reg;

  skip_whitespace (str);

  if ((reg = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL)
    return;

  if (reg == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if ((inst.instruction & 0xf) == 0)
    {
      if ((reg == reg_required_here (&str, 0)) == FAIL
	  || skip_past_comma (&str) == FAIL)
	return;

      if (reg == REG_PC)
	{
	  inst.error = BAD_PC;
	  return;
	}
    }

  if (is_immediate_prefix (*str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    return;

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  lsb = expr.X_add_number;
  if (lsb > 31)
    {
      inst.error = _("invalid lsb");
      return;
    }

  if (skip_past_comma (&str) == FAIL)
    return;

  if (is_immediate_prefix (*str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    return;

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  width = expr.X_add_number;
  if (lsb + width > 32 || width > 32 || width == 0)
    {
      if (!inst.error)
        inst.error = _("invalid width");
      return;
    }

  inst.instruction |= (lsb << 7);
  inst.instruction |= (lsb + width - 1) << 16;

  end_of_line (str);
}

static void
do_bfx (char * str)
{
  expressionS expr;
  unsigned long lsb;
  unsigned long width;
  int rd, rn;

  skip_whitespace (str);

  if ((rd = reg_required_here (&str, 12)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (rn = reg_required_here (&str, 0)) == FAIL
      || skip_past_comma (&str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }
  else if (rd == REG_PC || rn == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  if (is_immediate_prefix (*str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    return;

  if (expr.X_op != O_constant)
    {
      inst.error = _("constant expression expected");
      return;
    }

  lsb = expr.X_add_number;
  if (lsb > 31)
    {
      inst.error = _("invalid lsb");
      return;
    }

  if (skip_past_comma (&str) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (is_immediate_prefix (*str))
    str++;
  else
    {
      inst.error = _("immediate expression expected");
      return;
    }

  if (my_get_expression (&expr, &str))
    return;

  width = expr.X_add_number;
  if (lsb + width > 32 || width > 32 || width == 0)
    {
      inst.error = _("invalid width");
      return;
    }

  inst.instruction |= (lsb << 7);
  inst.instruction |= (width - 1) << 16;

  end_of_line (str);
}

static void
do_swap (char * str)
{
  int reg;

  skip_whitespace (str);

  if ((reg = reg_required_here (&str, 12)) == FAIL)
    return;

  if (reg == REG_PC)
    {
      inst.error = _("r15 not allowed in swap");
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (reg = reg_required_here (&str, 0)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (reg == REG_PC)
    {
      inst.error = _("r15 not allowed in swap");
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || *str++ != '[')
    {
      inst.error = BAD_ARGS;
      return;
    }

  skip_whitespace (str);

  if ((reg = reg_required_here (&str, 16)) == FAIL)
    return;

  if (reg == REG_PC)
    {
      inst.error = BAD_PC;
      return;
    }

  skip_whitespace (str);

  if (*str++ != ']')
    {
      inst.error = _("missing ]");
      return;
    }

  end_of_line (str);
}

static void
do_branch (char * str)
{
  if (my_get_expression (&inst.reloc.exp, &str))
    return;

#ifdef OBJ_ELF
  {
    char * save_in;

    /* ScottB: February 5, 1998 - Check to see of PLT32 reloc
       required for the instruction.  */

    /* arm_parse_reloc () works on input_line_pointer.
       We actually want to parse the operands to the branch instruction
       passed in 'str'.  Save the input pointer and restore it later.  */
    save_in = input_line_pointer;
    input_line_pointer = str;
    if (inst.reloc.exp.X_op == O_symbol
	&& *str == '('
	&& arm_parse_reloc () == BFD_RELOC_ARM_PLT32)
      {
	inst.reloc.type   = BFD_RELOC_ARM_PLT32;
	inst.reloc.pc_rel = 0;
	/* Modify str to point to after parsed operands, otherwise
	   end_of_line() will complain about the (PLT) left in str.  */
	str = input_line_pointer;
      }
    else
      {
	inst.reloc.type   = BFD_RELOC_ARM_PCREL_BRANCH;
	inst.reloc.pc_rel = 1;
      }
    input_line_pointer = save_in;
  }
#else
  inst.reloc.type   = BFD_RELOC_ARM_PCREL_BRANCH;
  inst.reloc.pc_rel = 1;
#endif /* OBJ_ELF  */

  end_of_line (str);
}

static void
do_cdp (char * str)
{
  /* Co-processor data operation.
     Format: CDP{cond} CP#,<expr>,CRd,CRn,CRm{,<expr>}  */
  skip_whitespace (str);

  if (co_proc_number (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_opc_expr (&str, 20,4) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_reg_required_here (&str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_reg_required_here (&str, 16) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_reg_required_here (&str, 0) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == SUCCESS)
    {
      if (cp_opc_expr (&str, 5, 3) == FAIL)
	{
	  if (!inst.error)
	    inst.error = BAD_ARGS;
	  return;
	}
    }

  end_of_line (str);
}

static void
do_lstc (char * str)
{
  /* Co-processor register load/store.
     Format: <LDC|STC{cond}[L] CP#,CRd,<address>  */

  skip_whitespace (str);

  if (co_proc_number (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_reg_required_here (&str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_address_required_here (&str, CP_WB_OK) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_co_reg (char * str)
{
  /* Co-processor register transfer.
     Format: <MCR|MRC>{cond} CP#,<expr1>,Rd,CRn,CRm{,<expr2>}  */

  skip_whitespace (str);

  if (co_proc_number (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_opc_expr (&str, 21, 3) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_reg_required_here (&str, 16) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_reg_required_here (&str, 0) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == SUCCESS)
    {
      if (cp_opc_expr (&str, 5, 3) == FAIL)
	{
	  if (!inst.error)
	    inst.error = BAD_ARGS;
	  return;
	}
    }

  end_of_line (str);
}

static void
do_fpa_ctrl (char * str)
{
  /* FP control registers.
     Format: <WFS|RFS|WFC|RFC>{cond} Rn  */

  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_ldst (char * str)
{
  skip_whitespace (str);

  if (fp_reg_required_here (&str, 12) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_address_required_here (&str, CP_WB_OK) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_ldmstm (char * str)
{
  int num_regs;

  skip_whitespace (str);

  if (fp_reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  /* Get Number of registers to transfer.  */
  if (skip_past_comma (&str) == FAIL
      || my_get_expression (&inst.reloc.exp, &str))
    {
      if (! inst.error)
	inst.error = _("constant expression expected");
      return;
    }

  if (inst.reloc.exp.X_op != O_constant)
    {
      inst.error = _("constant value required for number of registers");
      return;
    }

  num_regs = inst.reloc.exp.X_add_number;

  if (num_regs < 1 || num_regs > 4)
    {
      inst.error = _("number of registers must be in the range [1:4]");
      return;
    }

  switch (num_regs)
    {
    case 1:
      inst.instruction |= CP_T_X;
      break;
    case 2:
      inst.instruction |= CP_T_Y;
      break;
    case 3:
      inst.instruction |= CP_T_Y | CP_T_X;
      break;
    case 4:
      break;
    default:
      abort ();
    }

  if (inst.instruction & (CP_T_Pre | CP_T_UD)) /* ea/fd format.  */
    {
      int reg;
      int write_back;
      int offset;

      /* The instruction specified "ea" or "fd", so we can only accept
	 [Rn]{!}.  The instruction does not really support stacking or
	 unstacking, so we have to emulate these by setting appropriate
	 bits and offsets.  */
      if (skip_past_comma (&str) == FAIL
	  || *str != '[')
	{
	  if (! inst.error)
	    inst.error = BAD_ARGS;
	  return;
	}

      str++;
      skip_whitespace (str);

      if ((reg = reg_required_here (&str, 16)) == FAIL)
	return;

      skip_whitespace (str);

      if (*str != ']')
	{
	  inst.error = BAD_ARGS;
	  return;
	}

      str++;
      if (*str == '!')
	{
	  write_back = 1;
	  str++;
	  if (reg == REG_PC)
	    {
	      inst.error =
		_("r15 not allowed as base register with write-back");
	      return;
	    }
	}
      else
	write_back = 0;

      if (inst.instruction & CP_T_Pre)
	{
	  /* Pre-decrement.  */
	  offset = 3 * num_regs;
	  if (write_back)
	    inst.instruction |= CP_T_WB;
	}
      else
	{
	  /* Post-increment.  */
	  if (write_back)
	    {
	      inst.instruction |= CP_T_WB;
	      offset = 3 * num_regs;
	    }
	  else
	    {
	      /* No write-back, so convert this into a standard pre-increment
		 instruction -- aesthetically more pleasing.  */
	      inst.instruction |= CP_T_Pre | CP_T_UD;
	      offset = 0;
	    }
	}

      inst.instruction |= offset;
    }
  else if (skip_past_comma (&str) == FAIL
	   || cp_address_required_here (&str, CP_WB_OK) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_dyadic (char * str)
{
  skip_whitespace (str);

  if (fp_reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || fp_reg_required_here (&str, 16) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || fp_op2 (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_monadic (char * str)
{
  skip_whitespace (str);

  if (fp_reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || fp_op2 (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_cmp (char * str)
{
  skip_whitespace (str);

  if (fp_reg_required_here (&str, 16) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || fp_op2 (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_from_reg (char * str)
{
  skip_whitespace (str);

  if (fp_reg_required_here (&str, 16) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_fpa_to_reg (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || fp_reg_required_here (&str, 0) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

/* Encode a VFP SP register number.  */

static void
vfp_sp_encode_reg (int reg, enum vfp_sp_reg_pos pos)
{
  switch (pos)
    {
    case VFP_REG_Sd:
      inst.instruction |= ((reg >> 1) << 12) | ((reg & 1) << 22);
      break;

    case VFP_REG_Sn:
      inst.instruction |= ((reg >> 1) << 16) | ((reg & 1) << 7);
      break;

    case VFP_REG_Sm:
      inst.instruction |= ((reg >> 1) << 0) | ((reg & 1) << 5);
      break;

    default:
      abort ();
    }
}

static int
vfp_sp_reg_required_here (char ** str,
			  enum vfp_sp_reg_pos pos)
{
  int    reg;
  char * start = *str;

  if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_SN].htab)) != FAIL)
    {
      vfp_sp_encode_reg (reg, pos);
      return reg;
    }

  /* In the few cases where we might be able to accept something else
     this error can be overridden.  */
  inst.error = _(all_reg_maps[REG_TYPE_SN].expected);

  /* Restore the start point.  */
  *str = start;
  return FAIL;
}

static int
vfp_dp_reg_required_here (char ** str,
			  enum vfp_dp_reg_pos pos)
{
  int    reg;
  char * start = *str;

  if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_DN].htab)) != FAIL)
    {
      switch (pos)
	{
	case VFP_REG_Dd:
	  inst.instruction |= reg << 12;
	  break;

	case VFP_REG_Dn:
	  inst.instruction |= reg << 16;
	  break;

	case VFP_REG_Dm:
	  inst.instruction |= reg << 0;
	  break;

	default:
	  abort ();
	}
      return reg;
    }

  /* In the few cases where we might be able to accept something else
     this error can be overridden.  */
  inst.error = _(all_reg_maps[REG_TYPE_DN].expected);

  /* Restore the start point.  */
  *str = start;
  return FAIL;
}

static void
do_vfp_sp_monadic (char * str)
{
  skip_whitespace (str);

  if (vfp_sp_reg_required_here (&str, VFP_REG_Sd) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_sp_reg_required_here (&str, VFP_REG_Sm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_monadic (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dd) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_dp_reg_required_here (&str, VFP_REG_Dm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_sp_dyadic (char * str)
{
  skip_whitespace (str);

  if (vfp_sp_reg_required_here (&str, VFP_REG_Sd) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_sp_reg_required_here (&str, VFP_REG_Sn) == FAIL
      || skip_past_comma (&str) == FAIL
      || vfp_sp_reg_required_here (&str, VFP_REG_Sm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_dyadic (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dd) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_dp_reg_required_here (&str, VFP_REG_Dn) == FAIL
      || skip_past_comma (&str) == FAIL
      || vfp_dp_reg_required_here (&str, VFP_REG_Dm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_reg_from_sp (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_sp_reg_required_here (&str, VFP_REG_Sn) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

/* Parse a VFP register list.  If the string is invalid return FAIL.
   Otherwise return the number of registers, and set PBASE to the first
   register.  Double precision registers are matched if DP is nonzero.  */

static int
vfp_parse_reg_list (char **str, int *pbase, int dp)
{
  int base_reg;
  int new_base;
  int regtype;
  int max_regs;
  int count = 0;
  int warned = 0;
  unsigned long mask = 0;
  int i;

  if (**str != '{')
    return FAIL;

  (*str)++;
  skip_whitespace (*str);

  if (dp)
    {
      regtype = REG_TYPE_DN;
      max_regs = 16;
    }
  else
    {
      regtype = REG_TYPE_SN;
      max_regs = 32;
    }

  base_reg = max_regs;

  do
    {
      new_base = arm_reg_parse (str, all_reg_maps[regtype].htab);
      if (new_base == FAIL)
	{
	  inst.error = _(all_reg_maps[regtype].expected);
	  return FAIL;
	}

      if (new_base < base_reg)
	base_reg = new_base;

      if (mask & (1 << new_base))
	{
	  inst.error = _("invalid register list");
	  return FAIL;
	}

      if ((mask >> new_base) != 0 && ! warned)
	{
	  as_tsktsk (_("register list not in ascending order"));
	  warned = 1;
	}

      mask |= 1 << new_base;
      count++;

      skip_whitespace (*str);

      if (**str == '-') /* We have the start of a range expression */
	{
	  int high_range;

	  (*str)++;

	  if ((high_range
	       = arm_reg_parse (str, all_reg_maps[regtype].htab))
	      == FAIL)
	    {
	      inst.error = _(all_reg_maps[regtype].expected);
	      return FAIL;
	    }

	  if (high_range <= new_base)
	    {
	      inst.error = _("register range not in ascending order");
	      return FAIL;
	    }

	  for (new_base++; new_base <= high_range; new_base++)
	    {
	      if (mask & (1 << new_base))
		{
		  inst.error = _("invalid register list");
		  return FAIL;
		}

	      mask |= 1 << new_base;
	      count++;
	    }
	}
    }
  while (skip_past_comma (str) != FAIL);

  (*str)++;

  /* Sanity check -- should have raised a parse error above.  */
  if (count == 0 || count > max_regs)
    abort ();

  *pbase = base_reg;

  /* Final test -- the registers must be consecutive.  */
  mask >>= base_reg;
  for (i = 0; i < count; i++)
    {
      if ((mask & (1u << i)) == 0)
	{
	  inst.error = _("non-contiguous register range");
	  return FAIL;
	}
    }

  return count;
}

static void
do_vfp_reg2_from_sp2 (char * str)
{
  int reg;

  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL
      || skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 16) == FAIL
      || skip_past_comma (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  /* We require exactly two consecutive SP registers.  */
  if (vfp_parse_reg_list (&str, &reg, 0) != 2)
    {
      if (! inst.error)
	inst.error = _("only two consecutive VFP SP registers allowed here");
    }
  vfp_sp_encode_reg (reg, VFP_REG_Sm);

  end_of_line (str);
}

static void
do_vfp_sp_from_reg (char * str)
{
  skip_whitespace (str);

  if (vfp_sp_reg_required_here (&str, VFP_REG_Sn) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_sp2_from_reg2 (char * str)
{
  int reg;

  skip_whitespace (str);

  /* We require exactly two consecutive SP registers.  */
  if (vfp_parse_reg_list (&str, &reg, 0) != 2)
    {
      if (! inst.error)
	inst.error = _("only two consecutive VFP SP registers allowed here");
    }
  vfp_sp_encode_reg (reg, VFP_REG_Sm);

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL
      || skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 16) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_reg_from_dp (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_dp_reg_required_here (&str, VFP_REG_Dn) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_reg2_from_dp (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 16) == FAIL
      || skip_past_comma (&str) == FAIL
      || vfp_dp_reg_required_here (&str, VFP_REG_Dm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_from_reg (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dn) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_from_reg2 (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dm) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL
      || skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 16) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static const struct vfp_reg *
vfp_psr_parse (char ** str)
{
  char *start = *str;
  char  c;
  char *p;
  const struct vfp_reg *vreg;

  p = start;

  /* Find the end of the current token.  */
  do
    {
      c = *p++;
    }
  while (ISALPHA (c));

  /* Mark it.  */
  *--p = 0;

  for (vreg = vfp_regs + 0;
       vreg < vfp_regs + sizeof (vfp_regs) / sizeof (struct vfp_reg);
       vreg++)
    {
      if (streq (start, vreg->name))
	{
	  *p = c;
	  *str = p;
	  return vreg;
	}
    }

  *p = c;
  return NULL;
}

static int
vfp_psr_required_here (char ** str)
{
  char *start = *str;
  const struct vfp_reg *vreg;

  vreg = vfp_psr_parse (str);

  if (vreg)
    {
      inst.instruction |= vreg->regno;
      return SUCCESS;
    }

  inst.error = _("VFP system register expected");

  *str = start;
  return FAIL;
}

static void
do_vfp_reg_from_ctrl (char * str)
{
  skip_whitespace (str);

  if (reg_required_here (&str, 12) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_psr_required_here (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_ctrl_from_reg (char * str)
{
  skip_whitespace (str);

  if (vfp_psr_required_here (&str) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || reg_required_here (&str, 12) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_sp_ldst (char * str)
{
  skip_whitespace (str);

  if (vfp_sp_reg_required_here (&str, VFP_REG_Sd) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_address_required_here (&str, CP_NO_WB) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_ldst (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dd) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || cp_address_required_here (&str, CP_NO_WB) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}


static void
vfp_sp_ldstm (char * str, enum vfp_ldstm_type ldstm_type)
{
  int count;
  int reg;

  skip_whitespace (str);

  if (reg_required_here (&str, 16) == FAIL)
    return;

  skip_whitespace (str);

  if (*str == '!')
    {
      inst.instruction |= WRITE_BACK;
      str++;
    }
  else if (ldstm_type != VFP_LDSTMIA)
    {
      inst.error = _("this addressing mode requires base-register writeback");
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (count = vfp_parse_reg_list (&str, &reg, 0)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }
  vfp_sp_encode_reg (reg, VFP_REG_Sd);

  inst.instruction |= count;
  end_of_line (str);
}

static void
vfp_dp_ldstm (char * str, enum vfp_ldstm_type ldstm_type)
{
  int count;
  int reg;

  skip_whitespace (str);

  if (reg_required_here (&str, 16) == FAIL)
    return;

  skip_whitespace (str);

  if (*str == '!')
    {
      inst.instruction |= WRITE_BACK;
      str++;
    }
  else if (ldstm_type != VFP_LDSTMIA && ldstm_type != VFP_LDSTMIAX)
    {
      inst.error = _("this addressing mode requires base-register writeback");
      return;
    }

  if (skip_past_comma (&str) == FAIL
      || (count = vfp_parse_reg_list (&str, &reg, 1)) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  count <<= 1;
  if (ldstm_type == VFP_LDSTMIAX || ldstm_type == VFP_LDSTMDBX)
    count += 1;

  inst.instruction |= (reg << 12) | count;
  end_of_line (str);
}

static void
do_vfp_sp_ldstmia (char * str)
{
  vfp_sp_ldstm (str, VFP_LDSTMIA);
}

static void
do_vfp_sp_ldstmdb (char * str)
{
  vfp_sp_ldstm (str, VFP_LDSTMDB);
}

static void
do_vfp_dp_ldstmia (char * str)
{
  vfp_dp_ldstm (str, VFP_LDSTMIA);
}

static void
do_vfp_dp_ldstmdb (char * str)
{
  vfp_dp_ldstm (str, VFP_LDSTMDB);
}

static void
do_vfp_xp_ldstmia (char *str)
{
  vfp_dp_ldstm (str, VFP_LDSTMIAX);
}

static void
do_vfp_xp_ldstmdb (char * str)
{
  vfp_dp_ldstm (str, VFP_LDSTMDBX);
}

static void
do_vfp_sp_compare_z (char * str)
{
  skip_whitespace (str);

  if (vfp_sp_reg_required_here (&str, VFP_REG_Sd) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_compare_z (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dd) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_dp_sp_cvt (char * str)
{
  skip_whitespace (str);

  if (vfp_dp_reg_required_here (&str, VFP_REG_Dd) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_sp_reg_required_here (&str, VFP_REG_Sm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

static void
do_vfp_sp_dp_cvt (char * str)
{
  skip_whitespace (str);

  if (vfp_sp_reg_required_here (&str, VFP_REG_Sd) == FAIL)
    return;

  if (skip_past_comma (&str) == FAIL
      || vfp_dp_reg_required_here (&str, VFP_REG_Dm) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  end_of_line (str);
}

/* Thumb specific routines.  */

/* Parse an add or subtract instruction, SUBTRACT is non-zero if the opcode
   was SUB.  */

static void
thumb_add_sub (char * str, int subtract)
{
  int Rd, Rs, Rn = FAIL;

  skip_whitespace (str);

  if ((Rd = thumb_reg (&str, THUMB_REG_ANY)) == FAIL
      || skip_past_comma (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (is_immediate_prefix (*str))
    {
      Rs = Rd;
      str++;
      if (my_get_expression (&inst.reloc.exp, &str))
	return;
    }
  else
    {
      if ((Rs = thumb_reg (&str, THUMB_REG_ANY)) == FAIL)
	return;

      if (skip_past_comma (&str) == FAIL)
	{
	  /* Two operand format, shuffle the registers
	     and pretend there are 3.  */
	  Rn = Rs;
	  Rs = Rd;
	}
      else if (is_immediate_prefix (*str))
	{
	  str++;
	  if (my_get_expression (&inst.reloc.exp, &str))
	    return;
	}
      else if ((Rn = thumb_reg (&str, THUMB_REG_ANY)) == FAIL)
	return;
    }

  /* We now have Rd and Rs set to registers, and Rn set to a register or FAIL;
     for the latter case, EXPR contains the immediate that was found.  */
  if (Rn != FAIL)
    {
      /* All register format.  */
      if (Rd > 7 || Rs > 7 || Rn > 7)
	{
	  if (Rs != Rd)
	    {
	      inst.error = _("dest and source1 must be the same register");
	      return;
	    }

	  /* Can't do this for SUB.  */
	  if (subtract)
	    {
	      inst.error = _("subtract valid only on lo regs");
	      return;
	    }

	  inst.instruction = (T_OPCODE_ADD_HI
			      | (Rd > 7 ? THUMB_H1 : 0)
			      | (Rn > 7 ? THUMB_H2 : 0));
	  inst.instruction |= (Rd & 7) | ((Rn & 7) << 3);
	}
      else
	{
	  inst.instruction = subtract ? T_OPCODE_SUB_R3 : T_OPCODE_ADD_R3;
	  inst.instruction |= Rd | (Rs << 3) | (Rn << 6);
	}
    }
  else
    {
      /* Immediate expression, now things start to get nasty.  */

      /* First deal with HI regs, only very restricted cases allowed:
	 Adjusting SP, and using PC or SP to get an address.  */
      if ((Rd > 7 && (Rd != REG_SP || Rs != REG_SP))
	  || (Rs > 7 && Rs != REG_SP && Rs != REG_PC))
	{
	  inst.error = _("invalid Hi register with immediate");
	  return;
	}

      if (inst.reloc.exp.X_op != O_constant)
	{
	  /* Value isn't known yet, all we can do is store all the fragments
	     we know about in the instruction and let the reloc hacking
	     work it all out.  */
	  inst.instruction = (subtract ? 0x8000 : 0) | (Rd << 4) | Rs;
	  inst.reloc.type = BFD_RELOC_ARM_THUMB_ADD;
	}
      else
	{
	  int offset = inst.reloc.exp.X_add_number;

	  if (subtract)
	    offset = - offset;

	  if (offset < 0)
	    {
	      offset = - offset;
	      subtract = 1;

	      /* Quick check, in case offset is MIN_INT.  */
	      if (offset < 0)
		{
		  inst.error = _("immediate value out of range");
		  return;
		}
	    }
	  /* Note - you cannot convert a subtract of 0 into an
	     add of 0 because the carry flag is set differently.  */
	  else if (offset > 0)
	    subtract = 0;

	  if (Rd == REG_SP)
	    {
	      if (offset & ~0x1fc)
		{
		  inst.error = _("invalid immediate value for stack adjust");
		  return;
		}
	      inst.instruction = subtract ? T_OPCODE_SUB_ST : T_OPCODE_ADD_ST;
	      inst.instruction |= offset >> 2;
	    }
	  else if (Rs == REG_PC || Rs == REG_SP)
	    {
	      if (subtract
		  || (offset & ~0x3fc))
		{
		  inst.error = _("invalid immediate for address calculation");
		  return;
		}
	      inst.instruction = (Rs == REG_PC ? T_OPCODE_ADD_PC
				  : T_OPCODE_ADD_SP);
	      inst.instruction |= (Rd << 8) | (offset >> 2);
	    }
	  else if (Rs == Rd)
	    {
	      if (offset & ~0xff)
		{
		  inst.error = _("immediate value out of range");
		  return;
		}
	      inst.instruction = subtract ? T_OPCODE_SUB_I8 : T_OPCODE_ADD_I8;
	      inst.instruction |= (Rd << 8) | offset;
	    }
	  else
	    {
	      if (offset & ~0x7)
		{
		  inst.error = _("immediate value out of range");
		  return;
		}
	      inst.instruction = subtract ? T_OPCODE_SUB_I3 : T_OPCODE_ADD_I3;
	      inst.instruction |= Rd | (Rs << 3) | (offset << 6);
	    }
	}
    }

  end_of_line (str);
}

static void
thumb_shift (char * str, int shift)
{
  int Rd, Rs, Rn = FAIL;

  skip_whitespace (str);

  if ((Rd = thumb_reg (&str, THUMB_REG_LO)) == FAIL
      || skip_past_comma (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (is_immediate_prefix (*str))
    {
      /* Two operand immediate format, set Rs to Rd.  */
      Rs = Rd;
      str ++;
      if (my_get_expression (&inst.reloc.exp, &str))
	return;
    }
  else
    {
      if ((Rs = thumb_reg (&str, THUMB_REG_LO)) == FAIL)
	return;

      if (skip_past_comma (&str) == FAIL)
	{
	  /* Two operand format, shuffle the registers
	     and pretend there are 3.  */
	  Rn = Rs;
	  Rs = Rd;
	}
      else if (is_immediate_prefix (*str))
	{
	  str++;
	  if (my_get_expression (&inst.reloc.exp, &str))
	    return;
	}
      else if ((Rn = thumb_reg (&str, THUMB_REG_LO)) == FAIL)
	return;
    }

  /* We now have Rd and Rs set to registers, and Rn set to a register or FAIL;
     for the latter case, EXPR contains the immediate that was found.  */

  if (Rn != FAIL)
    {
      if (Rs != Rd)
	{
	  inst.error = _("source1 and dest must be same register");
	  return;
	}

      switch (shift)
	{
	case THUMB_ASR: inst.instruction = T_OPCODE_ASR_R; break;
	case THUMB_LSL: inst.instruction = T_OPCODE_LSL_R; break;
	case THUMB_LSR: inst.instruction = T_OPCODE_LSR_R; break;
	}

      inst.instruction |= Rd | (Rn << 3);
    }
  else
    {
      switch (shift)
	{
	case THUMB_ASR: inst.instruction = T_OPCODE_ASR_I; break;
	case THUMB_LSL: inst.instruction = T_OPCODE_LSL_I; break;
	case THUMB_LSR: inst.instruction = T_OPCODE_LSR_I; break;
	}

      if (inst.reloc.exp.X_op != O_constant)
	{
	  /* Value isn't known yet, create a dummy reloc and let reloc
	     hacking fix it up.  */
	  inst.reloc.type = BFD_RELOC_ARM_THUMB_SHIFT;
	}
      else
	{
	  unsigned shift_value = inst.reloc.exp.X_add_number;

	  if (shift_value > 32 || (shift_value == 32 && shift == THUMB_LSL))
	    {
	      inst.error = _("invalid immediate for shift");
	      return;
	    }

	  /* Shifts of zero are handled by converting to LSL.  */
	  if (shift_value == 0)
	    inst.instruction = T_OPCODE_LSL_I;

	  /* Shifts of 32 are encoded as a shift of zero.  */
	  if (shift_value == 32)
	    shift_value = 0;

	  inst.instruction |= shift_value << 6;
	}

      inst.instruction |= Rd | (Rs << 3);
    }

  end_of_line (str);
}

static void
thumb_load_store (char * str, int load_store, int size)
{
  int Rd, Rb, Ro = FAIL;

  skip_whitespace (str);

  if ((Rd = thumb_reg (&str, THUMB_REG_LO)) == FAIL
      || skip_past_comma (&str) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (*str == '[')
    {
      str++;
      if ((Rb = thumb_reg (&str, THUMB_REG_ANY)) == FAIL)
	return;

      if (skip_past_comma (&str) != FAIL)
	{
	  if (is_immediate_prefix (*str))
	    {
	      str++;
	      if (my_get_expression (&inst.reloc.exp, &str))
		return;
	    }
	  else if ((Ro = thumb_reg (&str, THUMB_REG_LO)) == FAIL)
	    return;
	}
      else
	{
	  inst.reloc.exp.X_op = O_constant;
	  inst.reloc.exp.X_add_number = 0;
	}

      if (*str != ']')
	{
	  inst.error = _("expected ']'");
	  return;
	}
      str++;
    }
  else if (*str == '=')
    {
      if (load_store != THUMB_LOAD)
	{
	  inst.error = _("invalid pseudo operation");
	  return;
	}

      /* Parse an "ldr Rd, =expr" instruction; this is another pseudo op.  */
      str++;

      skip_whitespace (str);

      if (my_get_expression (& inst.reloc.exp, & str))
	return;

      end_of_line (str);

      if (   inst.reloc.exp.X_op != O_constant
	  && inst.reloc.exp.X_op != O_symbol)
	{
	  inst.error = "Constant expression expected";
	  return;
	}

      if (inst.reloc.exp.X_op == O_constant
	  && ((inst.reloc.exp.X_add_number & ~0xFF) == 0))
	{
	  /* This can be done with a mov instruction.  */

	  inst.instruction  = T_OPCODE_MOV_I8 | (Rd << 8);
	  inst.instruction |= inst.reloc.exp.X_add_number;
	  return;
	}

      /* Insert into literal pool.  */
      if (add_to_lit_pool () == FAIL)
	{
	  if (!inst.error)
	    inst.error = "literal pool insertion failed";
	  return;
	}

      inst.reloc.type   = BFD_RELOC_ARM_THUMB_OFFSET;
      inst.reloc.pc_rel = 1;
      inst.instruction  = T_OPCODE_LDR_PC | (Rd << 8);
      /* Adjust ARM pipeline offset to Thumb.  */
      inst.reloc.exp.X_add_number += 4;

      return;
    }
  else
    {
      if (my_get_expression (&inst.reloc.exp, &str))
	return;

      inst.instruction = T_OPCODE_LDR_PC | (Rd << 8);
      inst.reloc.pc_rel = 1;
      inst.reloc.exp.X_add_number -= 4; /* Pipeline offset.  */
      inst.reloc.type = BFD_RELOC_ARM_THUMB_OFFSET;
      end_of_line (str);
      return;
    }

  if (Rb == REG_PC || Rb == REG_SP)
    {
      if (size != THUMB_WORD)
	{
	  inst.error = _("byte or halfword not valid for base register");
	  return;
	}
      else if (Rb == REG_PC && load_store != THUMB_LOAD)
	{
	  inst.error = _("r15 based store not allowed");
	  return;
	}
      else if (Ro != FAIL)
	{
	  inst.error = _("invalid base register for register offset");
	  return;
	}

      if (Rb == REG_PC)
	inst.instruction = T_OPCODE_LDR_PC;
      else if (load_store == THUMB_LOAD)
	inst.instruction = T_OPCODE_LDR_SP;
      else
	inst.instruction = T_OPCODE_STR_SP;

      inst.instruction |= Rd << 8;
      if (inst.reloc.exp.X_op == O_constant)
	{
	  unsigned offset = inst.reloc.exp.X_add_number;

	  if (offset & ~0x3fc)
	    {
	      inst.error = _("invalid offset");
	      return;
	    }

	  inst.instruction |= offset >> 2;
	}
      else
	inst.reloc.type = BFD_RELOC_ARM_THUMB_OFFSET;
    }
  else if (Rb > 7)
    {
      inst.error = _("invalid base register in load/store");
      return;
    }
  else if (Ro == FAIL)
    {
      /* Immediate offset.  */
      if (size == THUMB_WORD)
	inst.instruction = (load_store == THUMB_LOAD
			    ? T_OPCODE_LDR_IW : T_OPCODE_STR_IW);
      else if (size == THUMB_HALFWORD)
	inst.instruction = (load_store == THUMB_LOAD
			    ? T_OPCODE_LDR_IH : T_OPCODE_STR_IH);
      else
	inst.instruction = (load_store == THUMB_LOAD
			    ? T_OPCODE_LDR_IB : T_OPCODE_STR_IB);

      inst.instruction |= Rd | (Rb << 3);

      if (inst.reloc.exp.X_op == O_constant)
	{
	  unsigned offset = inst.reloc.exp.X_add_number;

	  if (offset & ~(0x1f << size))
	    {
	      inst.error = _("invalid offset");
	      return;
	    }
	  inst.instruction |= (offset >> size) << 6;
	}
      else
	inst.reloc.type = BFD_RELOC_ARM_THUMB_OFFSET;
    }
  else
    {
      /* Register offset.  */
      if (size == THUMB_WORD)
	inst.instruction = (load_store == THUMB_LOAD
			    ? T_OPCODE_LDR_RW : T_OPCODE_STR_RW);
      else if (size == THUMB_HALFWORD)
	inst.instruction = (load_store == THUMB_LOAD
			    ? T_OPCODE_LDR_RH : T_OPCODE_STR_RH);
      else
	inst.instruction = (load_store == THUMB_LOAD
			    ? T_OPCODE_LDR_RB : T_OPCODE_STR_RB);

      inst.instruction |= Rd | (Rb << 3) | (Ro << 6);
    }

  end_of_line (str);
}

/* A register must be given at this point.

   Shift is the place to put it in inst.instruction.

   Restores input start point on err.
   Returns the reg#, or FAIL.  */

static int
mav_reg_required_here (char ** str, int shift, enum arm_reg_type regtype)
{
  int   reg;
  char *start = *str;

  if ((reg = arm_reg_parse (str, all_reg_maps[regtype].htab)) != FAIL)
    {
      if (shift >= 0)
	inst.instruction |= reg << shift;

      return reg;
    }

  /* Restore the start point.  */
  *str = start;

  /* Try generic coprocessor name if applicable.  */
  if (regtype == REG_TYPE_MVF ||
      regtype == REG_TYPE_MVD ||
      regtype == REG_TYPE_MVFX ||
      regtype == REG_TYPE_MVDX)
    {
      if ((reg = arm_reg_parse (str, all_reg_maps[REG_TYPE_CN].htab)) != FAIL)
	{
	  if (shift >= 0)
	    inst.instruction |= reg << shift;

	  return reg;
	}

      /* Restore the start point.  */
      *str = start;
    }

  /* In the few cases where we might be able to accept something else
     this error can be overridden.  */
  inst.error = _(all_reg_maps[regtype].expected);

  return FAIL;
}

/* Cirrus Maverick Instructions.  */

/* Isnsn like "foo X,Y".  */

static void
do_mav_binops (char * str,
	       int mode,
	       enum arm_reg_type reg0,
	       enum arm_reg_type reg1)
{
  int shift0, shift1;

  shift0 = mode & 0xff;
  shift1 = (mode >> 8) & 0xff;

  skip_whitespace (str);

  if (mav_reg_required_here (&str, shift0, reg0) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, shift1, reg1) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
    }
  else
    end_of_line (str);
}

/* Isnsn like "foo X,Y,Z".  */

static void
do_mav_triple (char * str,
	       int mode,
	       enum arm_reg_type reg0,
	       enum arm_reg_type reg1,
	       enum arm_reg_type reg2)
{
  int shift0, shift1, shift2;

  shift0 = mode & 0xff;
  shift1 = (mode >> 8) & 0xff;
  shift2 = (mode >> 16) & 0xff;

  skip_whitespace (str);

  if (mav_reg_required_here (&str, shift0, reg0) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, shift1, reg1) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, shift2, reg2) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
    }
  else
    end_of_line (str);
}

/* Wrapper functions.  */

static void
do_mav_binops_1a (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_RN, REG_TYPE_MVF);
}

static void
do_mav_binops_1b (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_RN, REG_TYPE_MVD);
}

static void
do_mav_binops_1c (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_RN, REG_TYPE_MVDX);
}

static void
do_mav_binops_1d (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVF, REG_TYPE_MVF);
}

static void
do_mav_binops_1e (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVD, REG_TYPE_MVD);
}

static void
do_mav_binops_1f (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVD, REG_TYPE_MVF);
}

static void
do_mav_binops_1g (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVF, REG_TYPE_MVD);
}

static void
do_mav_binops_1h (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVF, REG_TYPE_MVFX);
}

static void
do_mav_binops_1i (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVD, REG_TYPE_MVFX);
}

static void
do_mav_binops_1j (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVF, REG_TYPE_MVDX);
}

static void
do_mav_binops_1k (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVD, REG_TYPE_MVDX);
}

static void
do_mav_binops_1l (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVFX, REG_TYPE_MVF);
}

static void
do_mav_binops_1m (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVFX, REG_TYPE_MVD);
}

static void
do_mav_binops_1n (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVFX, REG_TYPE_MVFX);
}

static void
do_mav_binops_1o (char * str)
{
  do_mav_binops (str, MAV_MODE1, REG_TYPE_MVDX, REG_TYPE_MVDX);
}

static void
do_mav_binops_2a (char * str)
{
  do_mav_binops (str, MAV_MODE2, REG_TYPE_MVF, REG_TYPE_RN);
}

static void
do_mav_binops_2b (char * str)
{
  do_mav_binops (str, MAV_MODE2, REG_TYPE_MVD, REG_TYPE_RN);
}

static void
do_mav_binops_2c (char * str)
{
  do_mav_binops (str, MAV_MODE2, REG_TYPE_MVDX, REG_TYPE_RN);
}

static void
do_mav_binops_3a (char * str)
{
  do_mav_binops (str, MAV_MODE3, REG_TYPE_MVAX, REG_TYPE_MVFX);
}

static void
do_mav_binops_3b (char * str)
{
  do_mav_binops (str, MAV_MODE3, REG_TYPE_MVFX, REG_TYPE_MVAX);
}

static void
do_mav_binops_3c (char * str)
{
  do_mav_binops (str, MAV_MODE3, REG_TYPE_MVAX, REG_TYPE_MVDX);
}

static void
do_mav_binops_3d (char * str)
{
  do_mav_binops (str, MAV_MODE3, REG_TYPE_MVDX, REG_TYPE_MVAX);
}

static void
do_mav_triple_4a (char * str)
{
  do_mav_triple (str, MAV_MODE4, REG_TYPE_MVFX, REG_TYPE_MVFX, REG_TYPE_RN);
}

static void
do_mav_triple_4b (char * str)
{
  do_mav_triple (str, MAV_MODE4, REG_TYPE_MVDX, REG_TYPE_MVDX, REG_TYPE_RN);
}

static void
do_mav_triple_5a (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_RN, REG_TYPE_MVF, REG_TYPE_MVF);
}

static void
do_mav_triple_5b (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_RN, REG_TYPE_MVD, REG_TYPE_MVD);
}

static void
do_mav_triple_5c (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_RN, REG_TYPE_MVFX, REG_TYPE_MVFX);
}

static void
do_mav_triple_5d (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_RN, REG_TYPE_MVDX, REG_TYPE_MVDX);
}

static void
do_mav_triple_5e (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_MVF, REG_TYPE_MVF, REG_TYPE_MVF);
}

static void
do_mav_triple_5f (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_MVD, REG_TYPE_MVD, REG_TYPE_MVD);
}

static void
do_mav_triple_5g (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_MVFX, REG_TYPE_MVFX, REG_TYPE_MVFX);
}

static void
do_mav_triple_5h (char * str)
{
  do_mav_triple (str, MAV_MODE5, REG_TYPE_MVDX, REG_TYPE_MVDX, REG_TYPE_MVDX);
}

/* Isnsn like "foo W,X,Y,Z".
    where W=MVAX[0:3] and X,Y,Z=MVFX[0:15].  */

static void
do_mav_quad (char * str,
	     int mode,
	     enum arm_reg_type reg0,
	     enum arm_reg_type reg1,
	     enum arm_reg_type reg2,
	     enum arm_reg_type reg3)
{
  int shift0, shift1, shift2, shift3;

  shift0= mode & 0xff;
  shift1 = (mode >> 8) & 0xff;
  shift2 = (mode >> 16) & 0xff;
  shift3 = (mode >> 24) & 0xff;

  skip_whitespace (str);

  if (mav_reg_required_here (&str, shift0, reg0) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, shift1, reg1) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, shift2, reg2) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, shift3, reg3) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
    }
  else
    end_of_line (str);
}

static void
do_mav_quad_6a (char * str)
{
  do_mav_quad (str, MAV_MODE6, REG_TYPE_MVAX, REG_TYPE_MVFX, REG_TYPE_MVFX,
	       REG_TYPE_MVFX);
}

static void
do_mav_quad_6b (char * str)
{
  do_mav_quad (str, MAV_MODE6, REG_TYPE_MVAX, REG_TYPE_MVAX, REG_TYPE_MVFX,
	       REG_TYPE_MVFX);
}

/* cfmvsc32<cond> DSPSC,MVDX[15:0].  */
static void
do_mav_dspsc_1 (char * str)
{
  skip_whitespace (str);

  /* cfmvsc32.  */
  if (mav_reg_required_here (&str, -1, REG_TYPE_DSPSC) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, 12, REG_TYPE_MVDX) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;

      return;
    }

  end_of_line (str);
}

/* cfmv32sc<cond> MVDX[15:0],DSPSC.  */
static void
do_mav_dspsc_2 (char * str)
{
  skip_whitespace (str);

  /* cfmv32sc.  */
  if (mav_reg_required_here (&str, 12, REG_TYPE_MVDX) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, -1, REG_TYPE_DSPSC) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;

      return;
    }

  end_of_line (str);
}

/* Maverick shift immediate instructions.
   cfsh32<cond> MVFX[15:0],MVFX[15:0],Shift[6:0].
   cfsh64<cond> MVDX[15:0],MVDX[15:0],Shift[6:0].  */

static void
do_mav_shift (char * str,
	      enum arm_reg_type reg0,
	      enum arm_reg_type reg1)
{
  int error;
  int imm, neg = 0;

  skip_whitespace (str);

  error = 0;

  if (mav_reg_required_here (&str, 12, reg0) == FAIL
      || skip_past_comma (&str) == FAIL
      || mav_reg_required_here (&str, 16, reg1) == FAIL
      || skip_past_comma  (&str) == FAIL)
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  /* Calculate the immediate operand.
     The operand is a 7bit signed number.  */
  skip_whitespace (str);

  if (*str == '#')
    ++str;

  if (!ISDIGIT (*str) && *str != '-')
    {
      inst.error = _("expecting immediate, 7bit operand");
      return;
    }

  if (*str == '-')
    {
      neg = 1;
      ++str;
    }

  for (imm = 0; *str && ISDIGIT (*str); ++str)
    imm = imm * 10 + *str - '0';

  if (imm > 64)
    {
      inst.error = _("immediate out of range");
      return;
    }

  /* Make negative imm's into 7bit signed numbers.  */
  if (neg)
    {
      imm = -imm;
      imm &= 0x0000007f;
    }

  /* Bits 0-3 of the insn should have bits 0-3 of the immediate.
     Bits 5-7 of the insn should have bits 4-6 of the immediate.
     Bit 4 should be 0.  */
  imm = (imm & 0xf) | ((imm & 0x70) << 1);

  inst.instruction |= imm;
  end_of_line (str);
}

static void
do_mav_shift_1 (char * str)
{
  do_mav_shift (str, REG_TYPE_MVFX, REG_TYPE_MVFX);
}

static void
do_mav_shift_2 (char * str)
{
  do_mav_shift (str, REG_TYPE_MVDX, REG_TYPE_MVDX);
}

static int
mav_parse_offset (char ** str, int * negative)
{
  char * p = *str;
  int offset;

  *negative = 0;

  skip_whitespace (p);

  if (*p == '#')
    ++p;

  if (*p == '-')
    {
      *negative = 1;
      ++p;
    }

  if (!ISDIGIT (*p))
    {
      inst.error = _("offset expected");
      return 0;
    }

  for (offset = 0; *p && ISDIGIT (*p); ++p)
    offset = offset * 10 + *p - '0';

  if (offset > 0x3fc)
    {
      inst.error = _("offset out of range");
      return 0;
    }
  if (offset & 0x3)
    {
      inst.error = _("offset not a multiple of 4");
      return 0;
    }

  *str = p;

  return *negative ? -offset : offset;
}

/* Maverick load/store instructions.
  <insn><cond> CRd,[Rn,<offset>]{!}.
  <insn><cond> CRd,[Rn],<offset>.  */

static void
do_mav_ldst (char * str, enum arm_reg_type reg0)
{
  int offset, negative;

  skip_whitespace (str);

  if (mav_reg_required_here (&str, 12, reg0) == FAIL
      || skip_past_comma (&str) == FAIL
      || *str++ != '['
      || reg_required_here (&str, 16) == FAIL)
    goto fail_ldst;

  if (skip_past_comma (&str) == SUCCESS)
    {
      /* You are here: "<offset>]{!}".  */
      inst.instruction |= PRE_INDEX;

      offset = mav_parse_offset (&str, &negative);

      if (inst.error)
	return;

      if (*str++ != ']')
	{
	  inst.error = _("missing ]");
	  return;
	}

      if (*str == '!')
	{
	  inst.instruction |= WRITE_BACK;
	  ++str;
	}
    }
  else
    {
      /* You are here: "], <offset>".  */
      if (*str++ != ']')
	{
	  inst.error = _("missing ]");
	  return;
	}

      if (skip_past_comma (&str) == FAIL
	  || (offset = mav_parse_offset (&str, &negative), inst.error))
	goto fail_ldst;

      inst.instruction |= CP_T_WB; /* Post indexed, set bit W.  */
    }

  if (negative)
    offset = -offset;
  else
    inst.instruction |= CP_T_UD; /* Positive, so set bit U.  */

  inst.instruction |= offset >> 2;
  end_of_line (str);
  return;

fail_ldst:
  if (!inst.error)
     inst.error = BAD_ARGS;
}

static void
do_mav_ldst_1 (char * str)
{
  do_mav_ldst (str, REG_TYPE_MVF);
}

static void
do_mav_ldst_2 (char * str)
{
  do_mav_ldst (str, REG_TYPE_MVD);
}

static void
do_mav_ldst_3 (char * str)
{
  do_mav_ldst (str, REG_TYPE_MVFX);
}

static void
do_mav_ldst_4 (char * str)
{
  do_mav_ldst (str, REG_TYPE_MVDX);
}

static void
do_t_nop (char * str)
{
  /* Do nothing.  */
  end_of_line (str);
}

/* Handle the Format 4 instructions that do not have equivalents in other
   formats.  That is, ADC, AND, EOR, SBC, ROR, TST, NEG, CMN, ORR, MUL,
   BIC and MVN.  */

static void
do_t_arit (char * str)
{
  int Rd, Rs, Rn;

  skip_whitespace (str);

  if ((Rd = thumb_reg (&str, THUMB_REG_LO)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (Rs = thumb_reg (&str, THUMB_REG_LO)) == FAIL)
    {
      inst.error = BAD_ARGS;
      return;
    }

  if (skip_past_comma (&str) != FAIL)
    {
      /* Three operand format not allowed for TST, CMN, NEG and MVN.
	 (It isn't allowed for CMP either, but that isn't handled by this
	 function.)  */
      if (inst.instruction == T_OPCODE_TST
	  || inst.instruction == T_OPCODE_CMN
	  || inst.instruction == T_OPCODE_NEG
	  || inst.instruction == T_OPCODE_MVN)
	{
	  inst.error = BAD_ARGS;
	  return;
	}

      if ((Rn = thumb_reg (&str, THUMB_REG_LO)) == FAIL)
	return;

      if (Rs != Rd)
	{
	  inst.error = _("dest and source1 must be the same register");
	  return;
	}
      Rs = Rn;
    }

  if (inst.instruction == T_OPCODE_MUL
      && Rs == Rd)
    as_tsktsk (_("Rs and Rd must be different in MUL"));

  inst.instruction |= Rd | (Rs << 3);
  end_of_line (str);
}

static void
do_t_add (char * str)
{
  thumb_add_sub (str, 0);
}

static void
do_t_asr (char * str)
{
  thumb_shift (str, THUMB_ASR);
}

static void
do_t_branch9 (char * str)
{
  if (my_get_expression (&inst.reloc.exp, &str))
    return;
  inst.reloc.type = BFD_RELOC_THUMB_PCREL_BRANCH9;
  inst.reloc.pc_rel = 1;
  end_of_line (str);
}

static void
do_t_branch12 (char * str)
{
  if (my_get_expression (&inst.reloc.exp, &str))
    return;
  inst.reloc.type = BFD_RELOC_THUMB_PCREL_BRANCH12;
  inst.reloc.pc_rel = 1;
  end_of_line (str);
}

/* Find the real, Thumb encoded start of a Thumb function.  */

static symbolS *
find_real_start (symbolS * symbolP)
{
  char *       real_start;
  const char * name = S_GET_NAME (symbolP);
  symbolS *    new_target;

  /* This definition must agree with the one in gcc/config/arm/thumb.c.  */
#define STUB_NAME ".real_start_of"

  if (name == NULL)
    abort ();

  /* Names that start with '.' are local labels, not function entry points.
     The compiler may generate BL instructions to these labels because it
     needs to perform a branch to a far away location.  */
  if (name[0] == '.')
    return symbolP;

  real_start = malloc (strlen (name) + strlen (STUB_NAME) + 1);
  sprintf (real_start, "%s%s", STUB_NAME, name);

  new_target = symbol_find (real_start);

  if (new_target == NULL)
    {
      as_warn ("Failed to find real start of function: %s\n", name);
      new_target = symbolP;
    }

  free (real_start);

  return new_target;
}

static void
do_t_branch23 (char * str)
{
  if (my_get_expression (& inst.reloc.exp, & str))
    return;

  inst.reloc.type   = BFD_RELOC_THUMB_PCREL_BRANCH23;
  inst.reloc.pc_rel = 1;
  end_of_line (str);

  /* If the destination of the branch is a defined symbol which does not have
     the THUMB_FUNC attribute, then we must be calling a function which has
     the (interfacearm) attribute.  We look for the Thumb entry point to that
     function and change the branch to refer to that function instead.  */
  if (   inst.reloc.exp.X_op == O_symbol
      && inst.reloc.exp.X_add_symbol != NULL
      && S_IS_DEFINED (inst.reloc.exp.X_add_symbol)
      && ! THUMB_IS_FUNC (inst.reloc.exp.X_add_symbol))
    inst.reloc.exp.X_add_symbol =
      find_real_start (inst.reloc.exp.X_add_symbol);
}

static void
do_t_bx (char * str)
{
  int reg;

  skip_whitespace (str);

  if ((reg = thumb_reg (&str, THUMB_REG_ANY)) == FAIL)
    return;

  /* This sets THUMB_H2 from the top bit of reg.  */
  inst.instruction |= reg << 3;

  /* ??? FIXME: Should add a hacky reloc here if reg is REG_PC.  The reloc
     should cause the alignment to be checked once it is known.  This is
     because BX PC only works if the instruction is word aligned.  */

  end_of_line (str);
}

static void
do_t_compare (char * str)
{
  thumb_mov_compare (str, THUMB_COMPARE);
}

static void
do_t_ldmstm (char * str)
{
  int Rb;
  long range;

  skip_whitespace (str);

  if ((Rb = thumb_reg (&str, THUMB_REG_LO)) == FAIL)
    return;

  if (*str != '!')
    as_warn (_("inserted missing '!': load/store multiple always writes back base register"));
  else
    str++;

  if (skip_past_comma (&str) == FAIL
      || (range = reg_list (&str)) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (inst.reloc.type != BFD_RELOC_UNUSED)
    {
      /* This really doesn't seem worth it.  */
      inst.reloc.type = BFD_RELOC_UNUSED;
      inst.error = _("expression too complex");
      return;
    }

  if (range & ~0xff)
    {
      inst.error = _("only lo-regs valid in load/store multiple");
      return;
    }

  inst.instruction |= (Rb << 8) | range;
  end_of_line (str);
}

static void
do_t_ldr (char * str)
{
  thumb_load_store (str, THUMB_LOAD, THUMB_WORD);
}

static void
do_t_ldrb (char * str)
{
  thumb_load_store (str, THUMB_LOAD, THUMB_BYTE);
}

static void
do_t_ldrh (char * str)
{
  thumb_load_store (str, THUMB_LOAD, THUMB_HALFWORD);
}

static void
do_t_lds (char * str)
{
  int Rd, Rb, Ro;

  skip_whitespace (str);

  if ((Rd = thumb_reg (&str, THUMB_REG_LO)) == FAIL
      || skip_past_comma (&str) == FAIL
      || *str++ != '['
      || (Rb = thumb_reg (&str, THUMB_REG_LO)) == FAIL
      || skip_past_comma (&str) == FAIL
      || (Ro = thumb_reg (&str, THUMB_REG_LO)) == FAIL
      || *str++ != ']')
    {
      if (! inst.error)
	inst.error = _("syntax: ldrs[b] Rd, [Rb, Ro]");
      return;
    }

  inst.instruction |= Rd | (Rb << 3) | (Ro << 6);
  end_of_line (str);
}

static void
do_t_lsl (char * str)
{
  thumb_shift (str, THUMB_LSL);
}

static void
do_t_lsr (char * str)
{
  thumb_shift (str, THUMB_LSR);
}

static void
do_t_mov (char * str)
{
  thumb_mov_compare (str, THUMB_MOVE);
}

static void
do_t_push_pop (char * str)
{
  long range;

  skip_whitespace (str);

  if ((range = reg_list (&str)) == FAIL)
    {
      if (! inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  if (inst.reloc.type != BFD_RELOC_UNUSED)
    {
      /* This really doesn't seem worth it.  */
      inst.reloc.type = BFD_RELOC_UNUSED;
      inst.error = _("expression too complex");
      return;
    }

  if (range & ~0xff)
    {
      if ((inst.instruction == T_OPCODE_PUSH
	   && (range & ~0xff) == 1 << REG_LR)
	  || (inst.instruction == T_OPCODE_POP
	      && (range & ~0xff) == 1 << REG_PC))
	{
	  inst.instruction |= THUMB_PP_PC_LR;
	  range &= 0xff;
	}
      else
	{
	  inst.error = _("invalid register list to push/pop instruction");
	  return;
	}
    }

  inst.instruction |= range;
  end_of_line (str);
}

static void
do_t_str (char * str)
{
  thumb_load_store (str, THUMB_STORE, THUMB_WORD);
}

static void
do_t_strb (char * str)
{
  thumb_load_store (str, THUMB_STORE, THUMB_BYTE);
}

static void
do_t_strh (char * str)
{
  thumb_load_store (str, THUMB_STORE, THUMB_HALFWORD);
}

static void
do_t_sub (char * str)
{
  thumb_add_sub (str, 1);
}

static void
do_t_swi (char * str)
{
  skip_whitespace (str);

  if (my_get_expression (&inst.reloc.exp, &str))
    return;

  inst.reloc.type = BFD_RELOC_ARM_SWI;
  end_of_line (str);
}

static void
do_t_adr (char * str)
{
  int reg;

  /* This is a pseudo-op of the form "adr rd, label" to be converted
     into a relative address of the form "add rd, pc, #label-.-4".  */
  skip_whitespace (str);

  /* Store Rd in temporary location inside instruction.  */
  if ((reg = reg_required_here (&str, 4)) == FAIL
      || (reg > 7)  /* For Thumb reg must be r0..r7.  */
      || skip_past_comma (&str) == FAIL
      || my_get_expression (&inst.reloc.exp, &str))
    {
      if (!inst.error)
	inst.error = BAD_ARGS;
      return;
    }

  inst.reloc.type = BFD_RELOC_ARM_THUMB_ADD;
  inst.reloc.exp.X_add_number -= 4; /* PC relative adjust.  */
  inst.reloc.pc_rel = 1;
  inst.instruction |= REG_PC; /* Rd is already placed into the instruction.  */

  end_of_line (str);
}

static void
insert_reg (const struct reg_entry * r,
	    struct hash_control * htab)
{
  int    len  = strlen (r->name) + 2;
  char * buf  = xmalloc (len);
  char * buf2 = xmalloc (len);
  int    i    = 0;

#ifdef REGISTER_PREFIX
  buf[i++] = REGISTER_PREFIX;
#endif

  strcpy (buf + i, r->name);

  for (i = 0; buf[i]; i++)
    buf2[i] = TOUPPER (buf[i]);

  buf2[i] = '\0';

  hash_insert (htab, buf,  (PTR) r);
  hash_insert (htab, buf2, (PTR) r);
}

static void
build_reg_hsh (struct reg_map * map)
{
  const struct reg_entry *r;

  if ((map->htab = hash_new ()) == NULL)
    as_fatal (_("virtual memory exhausted"));

  for (r = map->names; r->name != NULL; r++)
    insert_reg (r, map->htab);
}

static void
insert_reg_alias (char * str,
		  int regnum,
		  struct hash_control *htab)
{
  const char * error;
  struct reg_entry * new = xmalloc (sizeof (struct reg_entry));
  const char * name = xmalloc (strlen (str) + 1);

  strcpy ((char *) name, str);

  new->name = name;
  new->number = regnum;
  new->builtin = FALSE;

  error = hash_insert (htab, name, (PTR) new);
  if (error)
    {
      as_bad (_("failed to create an alias for %s, reason: %s"),
	    str, error);
      free ((char *) name);
      free (new);
    }
}

/* Look for the .req directive.  This is of the form:

   	new_register_name .req existing_register_name

   If we find one, or if it looks sufficiently like one that we want to
   handle any error here, return non-zero.  Otherwise return zero.  */

static int
create_register_alias (char * newname, char * p)
{
  char * q;
  char c;

  q = p;
  skip_whitespace (q);

  c = *p;
  *p = '\0';

  if (*q && !strncmp (q, ".req ", 5))
    {
      char *copy_of_str;
      char *r;

#ifndef IGNORE_OPCODE_CASE
      newname = original_case_string;
#endif
      copy_of_str = newname;

      q += 4;
      skip_whitespace (q);

      for (r = q; *r != '\0'; r++)
	if (*r == ' ')
	  break;

      if (r != q)
	{
	  enum arm_reg_type new_type, old_type;
	  int old_regno;
	  char d = *r;

	  *r = '\0';
	  old_type = arm_reg_parse_any (q);
	  *r = d;

	  new_type = arm_reg_parse_any (newname);

	  if (new_type == REG_TYPE_MAX)
	    {
	      if (old_type != REG_TYPE_MAX)
		{
		  old_regno = arm_reg_parse (&q, all_reg_maps[old_type].htab);
		  insert_reg_alias (newname, old_regno,
				    all_reg_maps[old_type].htab);
		}
	      else
		as_warn (_("register '%s' does not exist\n"), q);
	    }
	  else if (old_type == REG_TYPE_MAX)
	    {
	      as_warn (_("ignoring redefinition of register alias '%s' to non-existant register '%s'"),
		       copy_of_str, q);
	    }
	  else
	    {
	      /* Do not warn about redefinitions to the same alias.  */
	      if (new_type != old_type
		  || (arm_reg_parse (&q, all_reg_maps[old_type].htab)
		      != arm_reg_parse (&q, all_reg_maps[new_type].htab)))
		as_warn (_("ignoring redefinition of register alias '%s'"),
			 copy_of_str);

	    }
	}
      else
	as_warn (_("ignoring incomplete .req pseuso op"));

      *p = c;
      return 1;
    }

  *p = c;
  return 0;
}

static void
set_constant_flonums (void)
{
  int i;

  for (i = 0; i < NUM_FLOAT_VALS; i++)
    if (atof_ieee ((char *) fp_const[i], 'x', fp_values[i]) == NULL)
      abort ();
}


static const struct asm_opcode insns[] =
{
  /* Core ARM Instructions.  */
  {"and",        0xe0000000, 3,  ARM_EXT_V1,       do_arit},
  {"ands",       0xe0100000, 3,  ARM_EXT_V1,       do_arit},
  {"eor",        0xe0200000, 3,  ARM_EXT_V1,       do_arit},
  {"eors",       0xe0300000, 3,  ARM_EXT_V1,       do_arit},
  {"sub",        0xe0400000, 3,  ARM_EXT_V1,       do_arit},
  {"subs",       0xe0500000, 3,  ARM_EXT_V1,       do_arit},
  {"rsb",        0xe0600000, 3,  ARM_EXT_V1,       do_arit},
  {"rsbs",       0xe0700000, 3,  ARM_EXT_V1,       do_arit},
  {"add",        0xe0800000, 3,  ARM_EXT_V1,       do_arit},
  {"adds",       0xe0900000, 3,  ARM_EXT_V1,       do_arit},
  {"adc",        0xe0a00000, 3,  ARM_EXT_V1,       do_arit},
  {"adcs",       0xe0b00000, 3,  ARM_EXT_V1,       do_arit},
  {"sbc",        0xe0c00000, 3,  ARM_EXT_V1,       do_arit},
  {"sbcs",       0xe0d00000, 3,  ARM_EXT_V1,       do_arit},
  {"rsc",        0xe0e00000, 3,  ARM_EXT_V1,       do_arit},
  {"rscs",       0xe0f00000, 3,  ARM_EXT_V1,       do_arit},
  {"orr",        0xe1800000, 3,  ARM_EXT_V1,       do_arit},
  {"orrs",       0xe1900000, 3,  ARM_EXT_V1,       do_arit},
  {"bic",        0xe1c00000, 3,  ARM_EXT_V1,       do_arit},
  {"bics",       0xe1d00000, 3,  ARM_EXT_V1,       do_arit},

  {"tst",        0xe1100000, 3,  ARM_EXT_V1,       do_cmp},
  {"tsts",       0xe1100000, 3,  ARM_EXT_V1,       do_cmp},
  {"tstp",       0xe110f000, 3,  ARM_EXT_V1,       do_cmp},
  {"teq",        0xe1300000, 3,  ARM_EXT_V1,       do_cmp},
  {"teqs",       0xe1300000, 3,  ARM_EXT_V1,       do_cmp},
  {"teqp",       0xe130f000, 3,  ARM_EXT_V1,       do_cmp},
  {"cmp",        0xe1500000, 3,  ARM_EXT_V1,       do_cmp},
  {"cmps",       0xe1500000, 3,  ARM_EXT_V1,       do_cmp},
  {"cmpp",       0xe150f000, 3,  ARM_EXT_V1,       do_cmp},
  {"cmn",        0xe1700000, 3,  ARM_EXT_V1,       do_cmp},
  {"cmns",       0xe1700000, 3,  ARM_EXT_V1,       do_cmp},
  {"cmnp",       0xe170f000, 3,  ARM_EXT_V1,       do_cmp},

  {"mov",        0xe1a00000, 3,  ARM_EXT_V1,       do_mov},
  {"movs",       0xe1b00000, 3,  ARM_EXT_V1,       do_mov},
  {"mvn",        0xe1e00000, 3,  ARM_EXT_V1,       do_mov},
  {"mvns",       0xe1f00000, 3,  ARM_EXT_V1,       do_mov},

  {"ldr",        0xe4100000, 3,  ARM_EXT_V1,       do_ldst},
  {"ldrb",       0xe4500000, 3,  ARM_EXT_V1,       do_ldst},
  {"ldrt",       0xe4300000, 3,  ARM_EXT_V1,       do_ldstt},
  {"ldrbt",      0xe4700000, 3,  ARM_EXT_V1,       do_ldstt},
  {"str",        0xe4000000, 3,  ARM_EXT_V1,       do_ldst},
  {"strb",       0xe4400000, 3,  ARM_EXT_V1,       do_ldst},
  {"strt",       0xe4200000, 3,  ARM_EXT_V1,       do_ldstt},
  {"strbt",      0xe4600000, 3,  ARM_EXT_V1,       do_ldstt},

  {"stmia",      0xe8800000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"stmib",      0xe9800000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"stmda",      0xe8000000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"stmdb",      0xe9000000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"stmfd",      0xe9000000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"push",	 0xe92d0000, 4,  ARM_EXT_V1,       do_ldmstm},
  {"stmfa",      0xe9800000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"stmea",      0xe8800000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"stmed",      0xe8000000, 3,  ARM_EXT_V1,       do_ldmstm},

  {"ldmia",      0xe8900000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"pop",	 0xe8bd0000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmib",      0xe9900000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmda",      0xe8100000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmdb",      0xe9100000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmfd",      0xe8900000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmfa",      0xe8100000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmea",      0xe9100000, 3,  ARM_EXT_V1,       do_ldmstm},
  {"ldmed",      0xe9900000, 3,  ARM_EXT_V1,       do_ldmstm},

  {"swi",        0xef000000, 3,  ARM_EXT_V1,       do_swi},
#ifdef TE_WINCE
  /* XXX This is the wrong place to do this.  Think multi-arch.  */
  {"bl",         0xeb000000, 2,  ARM_EXT_V1,       do_branch},
  {"b",          0xea000000, 1,  ARM_EXT_V1,       do_branch},
#else
  {"bl",         0xebfffffe, 2,  ARM_EXT_V1,       do_branch},
  {"b",          0xeafffffe, 1,  ARM_EXT_V1,       do_branch},
#endif

  /* Pseudo ops.  */
  {"adr",        0xe28f0000, 3,  ARM_EXT_V1,       do_adr},
  {"adrl",       0xe28f0000, 3,  ARM_EXT_V1,       do_adrl},
  {"nop",        0xe1a00000, 3,  ARM_EXT_V1,       do_nop},

  /* ARM 2 multiplies.  */
  {"mul",        0xe0000090, 3,  ARM_EXT_V2,       do_mul},
  {"muls",       0xe0100090, 3,  ARM_EXT_V2,       do_mul},
  {"mla",        0xe0200090, 3,  ARM_EXT_V2,       do_mla},
  {"mlas",       0xe0300090, 3,  ARM_EXT_V2,       do_mla},

  /* Generic coprocessor instructions.  */
  {"cdp",        0xee000000, 3,  ARM_EXT_V2,       do_cdp},
  {"ldc",        0xec100000, 3,  ARM_EXT_V2,       do_lstc},
  {"ldcl",       0xec500000, 3,  ARM_EXT_V2,       do_lstc},
  {"stc",        0xec000000, 3,  ARM_EXT_V2,       do_lstc},
  {"stcl",       0xec400000, 3,  ARM_EXT_V2,       do_lstc},
  {"mcr",        0xee000010, 3,  ARM_EXT_V2,       do_co_reg},
  {"mrc",        0xee100010, 3,  ARM_EXT_V2,       do_co_reg},

  /* ARM 3 - swp instructions.  */
  {"swp",        0xe1000090, 3,  ARM_EXT_V2S,      do_swap},
  {"swpb",       0xe1400090, 3,  ARM_EXT_V2S,      do_swap},

  /* ARM 6 Status register instructions.  */
  {"mrs",        0xe10f0000, 3,  ARM_EXT_V3,       do_mrs},
  {"msr",        0xe120f000, 3,  ARM_EXT_V3,       do_msr},
  /* ScottB: our code uses     0xe128f000 for msr.
     NickC:  but this is wrong because the bits 16 through 19 are
             handled by the PSR_xxx defines above.  */

  /* ARM 7M long multiplies.  */
  {"smull",      0xe0c00090, 5,  ARM_EXT_V3M,      do_mull},
  {"smulls",     0xe0d00090, 5,  ARM_EXT_V3M,      do_mull},
  {"umull",      0xe0800090, 5,  ARM_EXT_V3M,      do_mull},
  {"umulls",     0xe0900090, 5,  ARM_EXT_V3M,      do_mull},
  {"smlal",      0xe0e00090, 5,  ARM_EXT_V3M,      do_mull},
  {"smlals",     0xe0f00090, 5,  ARM_EXT_V3M,      do_mull},
  {"umlal",      0xe0a00090, 5,  ARM_EXT_V3M,      do_mull},
  {"umlals",     0xe0b00090, 5,  ARM_EXT_V3M,      do_mull},

  /* ARM Architecture 4.  */
  {"ldrh",       0xe01000b0, 3,  ARM_EXT_V4,       do_ldstv4},
  {"ldrsh",      0xe01000f0, 3,  ARM_EXT_V4,       do_ldstv4},
  {"ldrsb",      0xe01000d0, 3,  ARM_EXT_V4,       do_ldstv4},
  {"strh",       0xe00000b0, 3,  ARM_EXT_V4,       do_ldstv4},

  /* ARM Architecture 4T.  */
  /* Note: bx (and blx) are required on V5, even if the processor does
     not support Thumb.  */
  {"bx",         0xe12fff10, 2,  ARM_EXT_V4T | ARM_EXT_V5, do_bx},

  /*  ARM Architecture 5T.  */
  /* Note: blx has 2 variants, so the .value is set dynamically.
     Only one of the variants has conditional execution.  */
  {"blx",        0xe0000000, 3,  ARM_EXT_V5,       do_blx},
  {"clz",        0xe16f0f10, 3,  ARM_EXT_V5,       do_clz},
  {"bkpt",       0xe1200070, 0,  ARM_EXT_V5,       do_bkpt},
  {"ldc2",       0xfc100000, 0,  ARM_EXT_V5,       do_lstc2},
  {"ldc2l",      0xfc500000, 0,  ARM_EXT_V5,       do_lstc2},
  {"stc2",       0xfc000000, 0,  ARM_EXT_V5,       do_lstc2},
  {"stc2l",      0xfc400000, 0,  ARM_EXT_V5,       do_lstc2},
  {"cdp2",       0xfe000000, 0,  ARM_EXT_V5,       do_cdp2},
  {"mcr2",       0xfe000010, 0,  ARM_EXT_V5,       do_co_reg2},
  {"mrc2",       0xfe100010, 0,  ARM_EXT_V5,       do_co_reg2},

  /*  ARM Architecture 5TExP.  */
  {"smlabb",     0xe1000080, 6,  ARM_EXT_V5ExP,    do_smla},
  {"smlatb",     0xe10000a0, 6,  ARM_EXT_V5ExP,    do_smla},
  {"smlabt",     0xe10000c0, 6,  ARM_EXT_V5ExP,    do_smla},
  {"smlatt",     0xe10000e0, 6,  ARM_EXT_V5ExP,    do_smla},

  {"smlawb",     0xe1200080, 6,  ARM_EXT_V5ExP,    do_smla},
  {"smlawt",     0xe12000c0, 6,  ARM_EXT_V5ExP,    do_smla},

  {"smlalbb",    0xe1400080, 7,  ARM_EXT_V5ExP,    do_smlal},
  {"smlaltb",    0xe14000a0, 7,  ARM_EXT_V5ExP,    do_smlal},
  {"smlalbt",    0xe14000c0, 7,  ARM_EXT_V5ExP,    do_smlal},
  {"smlaltt",    0xe14000e0, 7,  ARM_EXT_V5ExP,    do_smlal},

  {"smulbb",     0xe1600080, 6,  ARM_EXT_V5ExP,    do_smul},
  {"smultb",     0xe16000a0, 6,  ARM_EXT_V5ExP,    do_smul},
  {"smulbt",     0xe16000c0, 6,  ARM_EXT_V5ExP,    do_smul},
  {"smultt",     0xe16000e0, 6,  ARM_EXT_V5ExP,    do_smul},

  {"smulwb",     0xe12000a0, 6,  ARM_EXT_V5ExP,    do_smul},
  {"smulwt",     0xe12000e0, 6,  ARM_EXT_V5ExP,    do_smul},

  {"qadd",       0xe1000050, 4,  ARM_EXT_V5ExP,    do_qadd},
  {"qdadd",      0xe1400050, 5,  ARM_EXT_V5ExP,    do_qadd},
  {"qsub",       0xe1200050, 4,  ARM_EXT_V5ExP,    do_qadd},
  {"qdsub",      0xe1600050, 5,  ARM_EXT_V5ExP,    do_qadd},

  /*  ARM Architecture 5TE.  */
  {"pld",        0xf450f000, 0,  ARM_EXT_V5E,      do_pld},
  {"ldrd",       0xe00000d0, 3,  ARM_EXT_V5E,      do_ldrd},
  {"strd",       0xe00000f0, 3,  ARM_EXT_V5E,      do_ldrd},

  {"mcrr",       0xec400000, 4,  ARM_EXT_V5E,      do_co_reg2c},
  {"mrrc",       0xec500000, 4,  ARM_EXT_V5E,      do_co_reg2c},

  /*  ARM Architecture 5TEJ.  */
  {"bxj",	 0xe12fff20, 3,  ARM_EXT_V5J,	   do_bxj},

  /*  ARM V6.  */
  { "cps",       0xf1020000, 0,  ARM_EXT_V6,       do_cps},
  { "cpsie",     0xf1080000, 0,  ARM_EXT_V6,       do_cpsi},
  { "cpsid",     0xf10C0000, 0,  ARM_EXT_V6,       do_cpsi},
  { "ldrex",     0xe1900f9f, 5,  ARM_EXT_V6,       do_ldrex},
  { "mcrr2",     0xfc400000, 0,  ARM_EXT_V6,       do_co_reg2c},
  { "mrrc2",     0xfc500000, 0,  ARM_EXT_V6,       do_co_reg2c},
  { "pkhbt",     0xe6800010, 5,  ARM_EXT_V6,       do_pkhbt},
  { "pkhtb",     0xe6800050, 5,  ARM_EXT_V6,       do_pkhtb},
  { "qadd16",    0xe6200f10, 6,  ARM_EXT_V6,       do_qadd16},
  { "qadd8",     0xe6200f90, 5,  ARM_EXT_V6,       do_qadd16},
  { "qaddsubx",  0xe6200f30, 8,  ARM_EXT_V6,       do_qadd16},
  { "qsub16",    0xe6200f70, 6,  ARM_EXT_V6,       do_qadd16},
  { "qsub8",     0xe6200ff0, 5,  ARM_EXT_V6,       do_qadd16},
  { "qsubaddx",  0xe6200f50, 8,  ARM_EXT_V6,       do_qadd16},
  { "sadd16",    0xe6100f10, 6,  ARM_EXT_V6,       do_qadd16},
  { "sadd8",     0xe6100f90, 5,  ARM_EXT_V6,       do_qadd16},
  { "saddsubx",  0xe6100f30, 8,  ARM_EXT_V6,       do_qadd16},
  { "shadd16",   0xe6300f10, 7,  ARM_EXT_V6,       do_qadd16},
  { "shadd8",    0xe6300f90, 6,  ARM_EXT_V6,       do_qadd16},
  { "shaddsubx", 0xe6300f30, 9,  ARM_EXT_V6,       do_qadd16},
  { "shsub16",   0xe6300f70, 7,  ARM_EXT_V6,       do_qadd16},
  { "shsub8",    0xe6300ff0, 6,  ARM_EXT_V6,       do_qadd16},
  { "shsubaddx", 0xe6300f50, 9,  ARM_EXT_V6,       do_qadd16},
  { "ssub16",    0xe6100f70, 6,  ARM_EXT_V6,       do_qadd16},
  { "ssub8",     0xe6100ff0, 5,  ARM_EXT_V6,       do_qadd16},
  { "ssubaddx",  0xe6100f50, 8,  ARM_EXT_V6,       do_qadd16},
  { "uadd16",    0xe6500f10, 6,  ARM_EXT_V6,       do_qadd16},
  { "uadd8",     0xe6500f90, 5,  ARM_EXT_V6,       do_qadd16},
  { "uaddsubx",  0xe6500f30, 8,  ARM_EXT_V6,       do_qadd16},
  { "uhadd16",   0xe6700f10, 7,  ARM_EXT_V6,       do_qadd16},
  { "uhadd8",    0xe6700f90, 6,  ARM_EXT_V6,       do_qadd16},
  { "uhaddsubx", 0xe6700f30, 9,  ARM_EXT_V6,       do_qadd16},
  { "uhsub16",   0xe6700f70, 7,  ARM_EXT_V6,       do_qadd16},
  { "uhsub8",    0xe6700ff0, 6,  ARM_EXT_V6,       do_qadd16},
  { "uhsubaddx", 0xe6700f50, 9,  ARM_EXT_V6,       do_qadd16},
  { "uqadd16",   0xe6600f10, 7,  ARM_EXT_V6,       do_qadd16},
  { "uqadd8",    0xe6600f90, 6,  ARM_EXT_V6,       do_qadd16},
  { "uqaddsubx", 0xe6600f30, 9,  ARM_EXT_V6,       do_qadd16},
  { "uqsub16",   0xe6600f70, 7,  ARM_EXT_V6,       do_qadd16},
  { "uqsub8",    0xe6600ff0, 6,  ARM_EXT_V6,       do_qadd16},
  { "uqsubaddx", 0xe6600f50, 9,  ARM_EXT_V6,       do_qadd16},
  { "usub16",    0xe6500f70, 6,  ARM_EXT_V6,       do_qadd16},
  { "usub8",     0xe6500ff0, 5,  ARM_EXT_V6,       do_qadd16},
  { "usubaddx",  0xe6500f50, 8,  ARM_EXT_V6,       do_qadd16},
  { "rev",       0xe6bf0f30, 3,  ARM_EXT_V6,       do_rev},
  { "rev16",     0xe6bf0fb0, 5,  ARM_EXT_V6,       do_rev},
  { "revsh",     0xe6ff0fb0, 5,  ARM_EXT_V6,       do_rev},
  { "rfeia",     0xf8900a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfeib",     0xf9900a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfeda",     0xf8100a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfedb",     0xf9100a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfefd",     0xf8900a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfefa",     0xf9900a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfeea",     0xf8100a00, 0,  ARM_EXT_V6,       do_rfe},
  { "rfeed",     0xf9100a00, 0,  ARM_EXT_V6,       do_rfe},
  { "sxtah",     0xe6b00070, 5,  ARM_EXT_V6,       do_sxtah},
  { "sxtab16",   0xe6800070, 7,  ARM_EXT_V6,       do_sxtah},
  { "sxtab",     0xe6a00070, 5,  ARM_EXT_V6,       do_sxtah},
  { "sxth",      0xe6bf0070, 4,  ARM_EXT_V6,       do_sxth},
  { "sxtb16",    0xe68f0070, 6,  ARM_EXT_V6,       do_sxth},
  { "sxtb",      0xe6af0070, 4,  ARM_EXT_V6,       do_sxth},
  { "uxtah",     0xe6f00070, 5,  ARM_EXT_V6,       do_sxtah},
  { "uxtab16",   0xe6c00070, 7,  ARM_EXT_V6,       do_sxtah},
  { "uxtab",     0xe6e00070, 5,  ARM_EXT_V6,       do_sxtah},
  { "uxth",      0xe6ff0070, 4,  ARM_EXT_V6,       do_sxth},
  { "uxtb16",    0xe6cf0070, 6,  ARM_EXT_V6,       do_sxth},
  { "uxtb",      0xe6ef0070, 4,  ARM_EXT_V6,       do_sxth},
  { "sel",       0xe68000b0, 3,  ARM_EXT_V6,       do_qadd16},
  { "setend",    0xf1010000, 0,  ARM_EXT_V6,       do_setend},
  { "smlad",     0xe7000010, 5,  ARM_EXT_V6,       do_smlad},
  { "smladx",    0xe7000030, 6,  ARM_EXT_V6,       do_smlad},
  { "smlald",    0xe7400010, 6,  ARM_EXT_V6,       do_smlald},
  { "smlaldx",   0xe7400030, 7,  ARM_EXT_V6,       do_smlald},
  { "smlsd",     0xe7000050, 5,  ARM_EXT_V6,       do_smlad},
  { "smlsdx",    0xe7000070, 6,  ARM_EXT_V6,       do_smlad},
  { "smlsld",    0xe7400050, 6,  ARM_EXT_V6,       do_smlald},
  { "smlsldx",   0xe7400070, 7,  ARM_EXT_V6,       do_smlald},
  { "smmla",     0xe7500010, 5,  ARM_EXT_V6,       do_smlad},
  { "smmlar",    0xe7500030, 6,  ARM_EXT_V6,       do_smlad},
  { "smmls",     0xe75000d0, 5,  ARM_EXT_V6,       do_smlad},
  { "smmlsr",    0xe75000f0, 6,  ARM_EXT_V6,       do_smlad},
  { "smmul",     0xe750f010, 5,  ARM_EXT_V6,       do_smmul},
  { "smmulr",    0xe750f030, 6,  ARM_EXT_V6,       do_smmul},
  { "smuad",     0xe700f010, 5,  ARM_EXT_V6,       do_smmul},
  { "smuadx",    0xe700f030, 6,  ARM_EXT_V6,       do_smmul},
  { "smusd",     0xe700f050, 5,  ARM_EXT_V6,       do_smmul},
  { "smusdx",    0xe700f070, 6,  ARM_EXT_V6,       do_smmul},
  { "srsia",     0xf8cd0500, 0,  ARM_EXT_V6,       do_srs},
  { "srsib",     0xf9cd0500, 0,  ARM_EXT_V6,       do_srs},
  { "srsda",     0xf84d0500, 0,  ARM_EXT_V6,       do_srs},
  { "srsdb",     0xf94d0500, 0,  ARM_EXT_V6,       do_srs},
  { "ssat",      0xe6a00010, 4,  ARM_EXT_V6,       do_ssat},
  { "ssat16",    0xe6a00f30, 6,  ARM_EXT_V6,       do_ssat16},
  { "strex",     0xe1800f90, 5,  ARM_EXT_V6,       do_strex},
  { "umaal",     0xe0400090, 5,  ARM_EXT_V6,       do_umaal},
  { "usad8",     0xe780f010, 5,  ARM_EXT_V6,       do_smmul},
  { "usada8",    0xe7800010, 6,  ARM_EXT_V6,       do_smlad},
  { "usat",      0xe6e00010, 4,  ARM_EXT_V6,       do_usat},
  { "usat16",    0xe6e00f30, 6,  ARM_EXT_V6,       do_usat16},

  /*  ARM V6K.  */
  { "clrex",     0xf57ff01f, 0,  ARM_EXT_V6K,      do_empty},
  { "ldrexb",    0xe1d00f9f, 6,  ARM_EXT_V6K,      do_ldrex},
  { "ldrexd",    0xe1b00f9f, 6,  ARM_EXT_V6K,      do_ldrex},
  { "ldrexh",    0xe1f00f9f, 6,  ARM_EXT_V6K,      do_ldrex},
  { "sev",       0xe320f004, 3,  ARM_EXT_V6K,      do_empty},
  { "strexb",    0xe1c00f90, 6,  ARM_EXT_V6K,      do_strex},
  { "strexd",    0xe1a00f90, 6,  ARM_EXT_V6K,      do_strex},
  { "strexh",    0xe1e00f90, 6,  ARM_EXT_V6K,      do_strex},
  { "wfe",       0xe320f002, 3,  ARM_EXT_V6K,      do_empty},
  { "wfi",       0xe320f003, 3,  ARM_EXT_V6K,      do_empty},
  { "yield",     0xe320f001, 5,  ARM_EXT_V6K,      do_empty},

  /*  ARM V6Z.  */
  { "smi",       0xe1600070, 3,  ARM_EXT_V6Z,      do_smi},

  /*  ARM V7A.  */
  { "bfi",       0xe7c00010, 3,  ARM_EXT_V7A,      do_bfci},
  { "bfc",       0xe7c0001f, 3,  ARM_EXT_V7A,      do_bfci},
  { "dmb",       0xf57ff05f, 0,  ARM_EXT_V7A,      do_dsb},
  { "dsb",       0xf57ff04f, 0,  ARM_EXT_V7A,      do_dsb},
  { "isb",       0xf57ff06f, 0,  ARM_EXT_V7A,      do_dsb},
  { "movw",	 0xe3000000, 2,  ARM_EXT_V7A,      do_movwt},
  { "movt",	 0xe3400000, 2,  ARM_EXT_V7A,      do_movwt},
  { "sbfx",      0xe7a00050, 4,  ARM_EXT_V7A,      do_bfx},
  { "ubfx",      0xe7e00050, 4,  ARM_EXT_V7A,      do_bfx},

  /* Core FPA instruction set (V1).  */
  {"wfs",        0xee200110, 3,  FPU_FPA_EXT_V1,   do_fpa_ctrl},
  {"rfs",        0xee300110, 3,  FPU_FPA_EXT_V1,   do_fpa_ctrl},
  {"wfc",        0xee400110, 3,  FPU_FPA_EXT_V1,   do_fpa_ctrl},
  {"rfc",        0xee500110, 3,  FPU_FPA_EXT_V1,   do_fpa_ctrl},

  {"ldfs",       0xec100100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},
  {"ldfd",       0xec108100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},
  {"ldfe",       0xec500100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},
  {"ldfp",       0xec508100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},

  {"stfs",       0xec000100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},
  {"stfd",       0xec008100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},
  {"stfe",       0xec400100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},
  {"stfp",       0xec408100, 3,  FPU_FPA_EXT_V1,   do_fpa_ldst},

  {"mvfs",       0xee008100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfsp",      0xee008120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfsm",      0xee008140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfsz",      0xee008160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfd",       0xee008180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfdp",      0xee0081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfdm",      0xee0081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfdz",      0xee0081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfe",       0xee088100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfep",      0xee088120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfem",      0xee088140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mvfez",      0xee088160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"mnfs",       0xee108100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfsp",      0xee108120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfsm",      0xee108140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfsz",      0xee108160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfd",       0xee108180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfdp",      0xee1081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfdm",      0xee1081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfdz",      0xee1081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfe",       0xee188100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfep",      0xee188120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfem",      0xee188140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"mnfez",      0xee188160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"abss",       0xee208100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"abssp",      0xee208120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"abssm",      0xee208140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"abssz",      0xee208160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absd",       0xee208180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absdp",      0xee2081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absdm",      0xee2081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absdz",      0xee2081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"abse",       0xee288100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absep",      0xee288120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absem",      0xee288140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"absez",      0xee288160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"rnds",       0xee308100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndsp",      0xee308120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndsm",      0xee308140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndsz",      0xee308160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndd",       0xee308180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rnddp",      0xee3081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rnddm",      0xee3081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rnddz",      0xee3081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rnde",       0xee388100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndep",      0xee388120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndem",      0xee388140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"rndez",      0xee388160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"sqts",       0xee408100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtsp",      0xee408120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtsm",      0xee408140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtsz",      0xee408160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtd",       0xee408180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtdp",      0xee4081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtdm",      0xee4081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtdz",      0xee4081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqte",       0xee488100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtep",      0xee488120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtem",      0xee488140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sqtez",      0xee488160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"logs",       0xee508100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logsp",      0xee508120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logsm",      0xee508140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logsz",      0xee508160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logd",       0xee508180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logdp",      0xee5081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logdm",      0xee5081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logdz",      0xee5081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"loge",       0xee588100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logep",      0xee588120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logem",      0xee588140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"logez",      0xee588160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"lgns",       0xee608100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnsp",      0xee608120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnsm",      0xee608140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnsz",      0xee608160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnd",       0xee608180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgndp",      0xee6081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgndm",      0xee6081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgndz",      0xee6081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgne",       0xee688100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnep",      0xee688120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnem",      0xee688140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"lgnez",      0xee688160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"exps",       0xee708100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expsp",      0xee708120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expsm",      0xee708140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expsz",      0xee708160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expd",       0xee708180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expdp",      0xee7081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expdm",      0xee7081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expdz",      0xee7081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expe",       0xee788100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expep",      0xee788120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expem",      0xee788140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"expdz",      0xee788160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"sins",       0xee808100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sinsp",      0xee808120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sinsm",      0xee808140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sinsz",      0xee808160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sind",       0xee808180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sindp",      0xee8081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sindm",      0xee8081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sindz",      0xee8081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sine",       0xee888100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sinep",      0xee888120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sinem",      0xee888140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"sinez",      0xee888160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"coss",       0xee908100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cossp",      0xee908120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cossm",      0xee908140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cossz",      0xee908160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosd",       0xee908180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosdp",      0xee9081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosdm",      0xee9081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosdz",      0xee9081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cose",       0xee988100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosep",      0xee988120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosem",      0xee988140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"cosez",      0xee988160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"tans",       0xeea08100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tansp",      0xeea08120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tansm",      0xeea08140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tansz",      0xeea08160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tand",       0xeea08180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tandp",      0xeea081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tandm",      0xeea081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tandz",      0xeea081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tane",       0xeea88100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tanep",      0xeea88120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tanem",      0xeea88140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"tanez",      0xeea88160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"asns",       0xeeb08100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnsp",      0xeeb08120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnsm",      0xeeb08140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnsz",      0xeeb08160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnd",       0xeeb08180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asndp",      0xeeb081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asndm",      0xeeb081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asndz",      0xeeb081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asne",       0xeeb88100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnep",      0xeeb88120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnem",      0xeeb88140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"asnez",      0xeeb88160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"acss",       0xeec08100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acssp",      0xeec08120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acssm",      0xeec08140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acssz",      0xeec08160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsd",       0xeec08180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsdp",      0xeec081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsdm",      0xeec081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsdz",      0xeec081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acse",       0xeec88100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsep",      0xeec88120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsem",      0xeec88140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"acsez",      0xeec88160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"atns",       0xeed08100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnsp",      0xeed08120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnsm",      0xeed08140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnsz",      0xeed08160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnd",       0xeed08180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atndp",      0xeed081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atndm",      0xeed081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atndz",      0xeed081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atne",       0xeed88100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnep",      0xeed88120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnem",      0xeed88140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"atnez",      0xeed88160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"urds",       0xeee08100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdsp",      0xeee08120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdsm",      0xeee08140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdsz",      0xeee08160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdd",       0xeee08180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urddp",      0xeee081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urddm",      0xeee081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urddz",      0xeee081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urde",       0xeee88100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdep",      0xeee88120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdem",      0xeee88140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"urdez",      0xeee88160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"nrms",       0xeef08100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmsp",      0xeef08120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmsm",      0xeef08140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmsz",      0xeef08160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmd",       0xeef08180, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmdp",      0xeef081a0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmdm",      0xeef081c0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmdz",      0xeef081e0, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrme",       0xeef88100, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmep",      0xeef88120, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmem",      0xeef88140, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},
  {"nrmez",      0xeef88160, 3,  FPU_FPA_EXT_V1,   do_fpa_monadic},

  {"adfs",       0xee000100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfsp",      0xee000120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfsm",      0xee000140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfsz",      0xee000160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfd",       0xee000180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfdp",      0xee0001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfdm",      0xee0001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfdz",      0xee0001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfe",       0xee080100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfep",      0xee080120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfem",      0xee080140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"adfez",      0xee080160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"sufs",       0xee200100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufsp",      0xee200120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufsm",      0xee200140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufsz",      0xee200160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufd",       0xee200180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufdp",      0xee2001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufdm",      0xee2001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufdz",      0xee2001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufe",       0xee280100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufep",      0xee280120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufem",      0xee280140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"sufez",      0xee280160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"rsfs",       0xee300100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfsp",      0xee300120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfsm",      0xee300140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfsz",      0xee300160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfd",       0xee300180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfdp",      0xee3001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfdm",      0xee3001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfdz",      0xee3001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfe",       0xee380100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfep",      0xee380120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfem",      0xee380140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rsfez",      0xee380160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"mufs",       0xee100100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufsp",      0xee100120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufsm",      0xee100140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufsz",      0xee100160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufd",       0xee100180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufdp",      0xee1001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufdm",      0xee1001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufdz",      0xee1001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufe",       0xee180100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufep",      0xee180120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufem",      0xee180140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"mufez",      0xee180160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"dvfs",       0xee400100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfsp",      0xee400120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfsm",      0xee400140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfsz",      0xee400160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfd",       0xee400180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfdp",      0xee4001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfdm",      0xee4001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfdz",      0xee4001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfe",       0xee480100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfep",      0xee480120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfem",      0xee480140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"dvfez",      0xee480160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"rdfs",       0xee500100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfsp",      0xee500120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfsm",      0xee500140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfsz",      0xee500160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfd",       0xee500180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfdp",      0xee5001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfdm",      0xee5001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfdz",      0xee5001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfe",       0xee580100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfep",      0xee580120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfem",      0xee580140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rdfez",      0xee580160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"pows",       0xee600100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powsp",      0xee600120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powsm",      0xee600140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powsz",      0xee600160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powd",       0xee600180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powdp",      0xee6001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powdm",      0xee6001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powdz",      0xee6001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powe",       0xee680100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powep",      0xee680120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powem",      0xee680140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"powez",      0xee680160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"rpws",       0xee700100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwsp",      0xee700120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwsm",      0xee700140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwsz",      0xee700160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwd",       0xee700180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwdp",      0xee7001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwdm",      0xee7001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwdz",      0xee7001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwe",       0xee780100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwep",      0xee780120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwem",      0xee780140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rpwez",      0xee780160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"rmfs",       0xee800100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfsp",      0xee800120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfsm",      0xee800140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfsz",      0xee800160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfd",       0xee800180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfdp",      0xee8001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfdm",      0xee8001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfdz",      0xee8001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfe",       0xee880100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfep",      0xee880120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfem",      0xee880140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"rmfez",      0xee880160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"fmls",       0xee900100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmlsp",      0xee900120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmlsm",      0xee900140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmlsz",      0xee900160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmld",       0xee900180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmldp",      0xee9001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmldm",      0xee9001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmldz",      0xee9001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmle",       0xee980100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmlep",      0xee980120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmlem",      0xee980140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fmlez",      0xee980160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"fdvs",       0xeea00100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvsp",      0xeea00120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvsm",      0xeea00140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvsz",      0xeea00160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvd",       0xeea00180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvdp",      0xeea001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvdm",      0xeea001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvdz",      0xeea001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdve",       0xeea80100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvep",      0xeea80120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvem",      0xeea80140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"fdvez",      0xeea80160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"frds",       0xeeb00100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdsp",      0xeeb00120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdsm",      0xeeb00140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdsz",      0xeeb00160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdd",       0xeeb00180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frddp",      0xeeb001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frddm",      0xeeb001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frddz",      0xeeb001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frde",       0xeeb80100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdep",      0xeeb80120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdem",      0xeeb80140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"frdez",      0xeeb80160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"pols",       0xeec00100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"polsp",      0xeec00120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"polsm",      0xeec00140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"polsz",      0xeec00160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"pold",       0xeec00180, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"poldp",      0xeec001a0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"poldm",      0xeec001c0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"poldz",      0xeec001e0, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"pole",       0xeec80100, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"polep",      0xeec80120, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"polem",      0xeec80140, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},
  {"polez",      0xeec80160, 3,  FPU_FPA_EXT_V1,   do_fpa_dyadic},

  {"cmf",        0xee90f110, 3,  FPU_FPA_EXT_V1,   do_fpa_cmp},
  {"cmfe",       0xeed0f110, 3,  FPU_FPA_EXT_V1,   do_fpa_cmp},
  {"cnf",        0xeeb0f110, 3,  FPU_FPA_EXT_V1,   do_fpa_cmp},
  {"cnfe",       0xeef0f110, 3,  FPU_FPA_EXT_V1,   do_fpa_cmp},
  /* The FPA10 data sheet suggests that the 'E' of cmfe/cnfe should
     not be an optional suffix, but part of the instruction.  To be
     compatible, we accept either.  */
  {"cmfe",       0xeed0f110, 4,  FPU_FPA_EXT_V1,   do_fpa_cmp},
  {"cnfe",       0xeef0f110, 4,  FPU_FPA_EXT_V1,   do_fpa_cmp},

  {"flts",       0xee000110, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltsp",      0xee000130, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltsm",      0xee000150, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltsz",      0xee000170, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltd",       0xee000190, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltdp",      0xee0001b0, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltdm",      0xee0001d0, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltdz",      0xee0001f0, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"flte",       0xee080110, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltep",      0xee080130, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltem",      0xee080150, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},
  {"fltez",      0xee080170, 3,  FPU_FPA_EXT_V1,   do_fpa_from_reg},

  /* The implementation of the FIX instruction is broken on some
     assemblers, in that it accepts a precision specifier as well as a
     rounding specifier, despite the fact that this is meaningless.
     To be more compatible, we accept it as well, though of course it
     does not set any bits.  */
  {"fix",        0xee100110, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixp",       0xee100130, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixm",       0xee100150, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixz",       0xee100170, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixsp",      0xee100130, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixsm",      0xee100150, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixsz",      0xee100170, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixdp",      0xee100130, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixdm",      0xee100150, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixdz",      0xee100170, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixep",      0xee100130, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixem",      0xee100150, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},
  {"fixez",      0xee100170, 3,  FPU_FPA_EXT_V1,   do_fpa_to_reg},

  /* Instructions that were new with the real FPA, call them V2.  */
  {"lfm",        0xec100200, 3,  FPU_FPA_EXT_V2,   do_fpa_ldmstm},
  {"lfmfd",      0xec900200, 3,  FPU_FPA_EXT_V2,   do_fpa_ldmstm},
  {"lfmea",      0xed100200, 3,  FPU_FPA_EXT_V2,   do_fpa_ldmstm},
  {"sfm",        0xec000200, 3,  FPU_FPA_EXT_V2,   do_fpa_ldmstm},
  {"sfmfd",      0xed000200, 3,  FPU_FPA_EXT_V2,   do_fpa_ldmstm},
  {"sfmea",      0xec800200, 3,  FPU_FPA_EXT_V2,   do_fpa_ldmstm},

  /* VFP V1xD (single precision).  */
  /* Moves and type conversions.  */
  {"fcpys",   0xeeb00a40, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fmrs",    0xee100a10, 4, FPU_VFP_EXT_V1xD, do_vfp_reg_from_sp},
  {"fmsr",    0xee000a10, 4, FPU_VFP_EXT_V1xD, do_vfp_sp_from_reg},
  {"fmstat",  0xeef1fa10, 6, FPU_VFP_EXT_V1xD, do_empty},
  {"fsitos",  0xeeb80ac0, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fuitos",  0xeeb80a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"ftosis",  0xeebd0a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"ftosizs", 0xeebd0ac0, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"ftouis",  0xeebc0a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"ftouizs", 0xeebc0ac0, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fmrx",    0xeef00a10, 4, FPU_VFP_EXT_V1xD, do_vfp_reg_from_ctrl},
  {"fmxr",    0xeee00a10, 4, FPU_VFP_EXT_V1xD, do_vfp_ctrl_from_reg},

  /* Memory operations.  */
  {"flds",    0xed100a00, 4, FPU_VFP_EXT_V1xD, do_vfp_sp_ldst},
  {"fsts",    0xed000a00, 4, FPU_VFP_EXT_V1xD, do_vfp_sp_ldst},
  {"fldmias", 0xec900a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmia},
  {"fldmfds", 0xec900a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmia},
  {"fldmdbs", 0xed300a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmdb},
  {"fldmeas", 0xed300a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmdb},
  {"fldmiax", 0xec900b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmia},
  {"fldmfdx", 0xec900b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmia},
  {"fldmdbx", 0xed300b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmdb},
  {"fldmeax", 0xed300b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmdb},
  {"fstmias", 0xec800a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmia},
  {"fstmeas", 0xec800a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmia},
  {"fstmdbs", 0xed200a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmdb},
  {"fstmfds", 0xed200a00, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_ldstmdb},
  {"fstmiax", 0xec800b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmia},
  {"fstmeax", 0xec800b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmia},
  {"fstmdbx", 0xed200b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmdb},
  {"fstmfdx", 0xed200b00, 7, FPU_VFP_EXT_V1xD, do_vfp_xp_ldstmdb},

  /* Monadic operations.  */
  {"fabss",   0xeeb00ac0, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fnegs",   0xeeb10a40, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fsqrts",  0xeeb10ac0, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},

  /* Dyadic operations.  */
  {"fadds",   0xee300a00, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fsubs",   0xee300a40, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fmuls",   0xee200a00, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fdivs",   0xee800a00, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fmacs",   0xee000a00, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fmscs",   0xee100a00, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fnmuls",  0xee200a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fnmacs",  0xee000a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},
  {"fnmscs",  0xee100a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_dyadic},

  /* Comparisons.  */
  {"fcmps",   0xeeb40a40, 5, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fcmpzs",  0xeeb50a40, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_compare_z},
  {"fcmpes",  0xeeb40ac0, 6, FPU_VFP_EXT_V1xD, do_vfp_sp_monadic},
  {"fcmpezs", 0xeeb50ac0, 7, FPU_VFP_EXT_V1xD, do_vfp_sp_compare_z},

  /* VFP V1 (Double precision).  */
  /* Moves and type conversions.  */
  {"fcpyd",   0xeeb00b40, 5, FPU_VFP_EXT_V1,   do_vfp_dp_monadic},
  {"fcvtds",  0xeeb70ac0, 6, FPU_VFP_EXT_V1,   do_vfp_dp_sp_cvt},
  {"fcvtsd",  0xeeb70bc0, 6, FPU_VFP_EXT_V1,   do_vfp_sp_dp_cvt},
  {"fmdhr",   0xee200b10, 5, FPU_VFP_EXT_V1,   do_vfp_dp_from_reg},
  {"fmdlr",   0xee000b10, 5, FPU_VFP_EXT_V1,   do_vfp_dp_from_reg},
  {"fmrdh",   0xee300b10, 5, FPU_VFP_EXT_V1,   do_vfp_reg_from_dp},
  {"fmrdl",   0xee100b10, 5, FPU_VFP_EXT_V1,   do_vfp_reg_from_dp},
  {"fsitod",  0xeeb80bc0, 6, FPU_VFP_EXT_V1,   do_vfp_dp_sp_cvt},
  {"fuitod",  0xeeb80b40, 6, FPU_VFP_EXT_V1,   do_vfp_dp_sp_cvt},
  {"ftosid",  0xeebd0b40, 6, FPU_VFP_EXT_V1,   do_vfp_sp_dp_cvt},
  {"ftosizd", 0xeebd0bc0, 7, FPU_VFP_EXT_V1,   do_vfp_sp_dp_cvt},
  {"ftouid",  0xeebc0b40, 6, FPU_VFP_EXT_V1,   do_vfp_sp_dp_cvt},
  {"ftouizd", 0xeebc0bc0, 7, FPU_VFP_EXT_V1,   do_vfp_sp_dp_cvt},

  /* Memory operations.  */
  {"fldd",    0xed100b00, 4, FPU_VFP_EXT_V1,   do_vfp_dp_ldst},
  {"fstd",    0xed000b00, 4, FPU_VFP_EXT_V1,   do_vfp_dp_ldst},
  {"fldmiad", 0xec900b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmia},
  {"fldmfdd", 0xec900b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmia},
  {"fldmdbd", 0xed300b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmdb},
  {"fldmead", 0xed300b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmdb},
  {"fstmiad", 0xec800b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmia},
  {"fstmead", 0xec800b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmia},
  {"fstmdbd", 0xed200b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmdb},
  {"fstmfdd", 0xed200b00, 7, FPU_VFP_EXT_V1,   do_vfp_dp_ldstmdb},

  /* Monadic operations.  */
  {"fabsd",   0xeeb00bc0, 5, FPU_VFP_EXT_V1,   do_vfp_dp_monadic},
  {"fnegd",   0xeeb10b40, 5, FPU_VFP_EXT_V1,   do_vfp_dp_monadic},
  {"fsqrtd",  0xeeb10bc0, 6, FPU_VFP_EXT_V1,   do_vfp_dp_monadic},

  /* Dyadic operations.  */
  {"faddd",   0xee300b00, 5, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fsubd",   0xee300b40, 5, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fmuld",   0xee200b00, 5, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fdivd",   0xee800b00, 5, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fmacd",   0xee000b00, 5, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fmscd",   0xee100b00, 5, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fnmuld",  0xee200b40, 6, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fnmacd",  0xee000b40, 6, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},
  {"fnmscd",  0xee100b40, 6, FPU_VFP_EXT_V1,   do_vfp_dp_dyadic},

  /* Comparisons.  */
  {"fcmpd",   0xeeb40b40, 5, FPU_VFP_EXT_V1,   do_vfp_dp_monadic},
  {"fcmpzd",  0xeeb50b40, 6, FPU_VFP_EXT_V1,   do_vfp_dp_compare_z},
  {"fcmped",  0xeeb40bc0, 6, FPU_VFP_EXT_V1,   do_vfp_dp_monadic},
  {"fcmpezd", 0xeeb50bc0, 7, FPU_VFP_EXT_V1,   do_vfp_dp_compare_z},

  /* VFP V2.  */
  {"fmsrr",   0xec400a10, 5, FPU_VFP_EXT_V2,   do_vfp_sp2_from_reg2},
  {"fmrrs",   0xec500a10, 5, FPU_VFP_EXT_V2,   do_vfp_reg2_from_sp2},
  {"fmdrr",   0xec400b10, 5, FPU_VFP_EXT_V2,   do_vfp_dp_from_reg2},
  {"fmrrd",   0xec500b10, 5, FPU_VFP_EXT_V2,   do_vfp_reg2_from_dp},

  /* Intel XScale extensions to ARM V5 ISA.  (All use CP0).  */
  {"mia",        0xee200010, 3,  ARM_CEXT_XSCALE,   do_xsc_mia},
  {"miaph",      0xee280010, 5,  ARM_CEXT_XSCALE,   do_xsc_mia},
  {"miabb",      0xee2c0010, 5,  ARM_CEXT_XSCALE,   do_xsc_mia},
  {"miabt",      0xee2d0010, 5,  ARM_CEXT_XSCALE,   do_xsc_mia},
  {"miatb",      0xee2e0010, 5,  ARM_CEXT_XSCALE,   do_xsc_mia},
  {"miatt",      0xee2f0010, 5,  ARM_CEXT_XSCALE,   do_xsc_mia},
  {"mar",        0xec400000, 3,  ARM_CEXT_XSCALE,   do_xsc_mar},
  {"mra",        0xec500000, 3,  ARM_CEXT_XSCALE,   do_xsc_mra},

  /* Intel Wireless MMX technology instructions.  */
  {"tandcb",     0xee130130, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tandc},
  {"tandch",     0xee530130, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tandc},
  {"tandcw",     0xee930130, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tandc},
  {"tbcstb",     0xee400010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tbcst},
  {"tbcsth",     0xee400050, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tbcst},
  {"tbcstw",     0xee400090, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tbcst},
  {"textrcb",    0xee130170, 7, ARM_CEXT_IWMMXT, do_iwmmxt_textrc},
  {"textrch",    0xee530170, 7, ARM_CEXT_IWMMXT, do_iwmmxt_textrc},
  {"textrcw",    0xee930170, 7, ARM_CEXT_IWMMXT, do_iwmmxt_textrc},
  {"textrmub",   0xee100070, 8, ARM_CEXT_IWMMXT, do_iwmmxt_textrm},
  {"textrmuh",   0xee500070, 8, ARM_CEXT_IWMMXT, do_iwmmxt_textrm},
  {"textrmuw",   0xee900070, 8, ARM_CEXT_IWMMXT, do_iwmmxt_textrm},
  {"textrmsb",   0xee100078, 8, ARM_CEXT_IWMMXT, do_iwmmxt_textrm},
  {"textrmsh",   0xee500078, 8, ARM_CEXT_IWMMXT, do_iwmmxt_textrm},
  {"textrmsw",   0xee900078, 8, ARM_CEXT_IWMMXT, do_iwmmxt_textrm},
  {"tinsrb",     0xee600010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tinsr},
  {"tinsrh",     0xee600050, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tinsr},
  {"tinsrw",     0xee600090, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tinsr},
  {"tmcr",       0xee000110, 4, ARM_CEXT_IWMMXT, do_iwmmxt_tmcr},
  {"tmcrr",      0xec400000, 5, ARM_CEXT_IWMMXT, do_iwmmxt_tmcrr},
  {"tmia",       0xee200010, 4, ARM_CEXT_IWMMXT, do_iwmmxt_tmia},
  {"tmiaph",     0xee280010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tmia},
  {"tmiabb",     0xee2c0010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tmia},
  {"tmiabt",     0xee2d0010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tmia},
  {"tmiatb",     0xee2e0010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tmia},
  {"tmiatt",     0xee2f0010, 6, ARM_CEXT_IWMMXT, do_iwmmxt_tmia},
  {"tmovmskb",   0xee100030, 8, ARM_CEXT_IWMMXT, do_iwmmxt_tmovmsk},
  {"tmovmskh",   0xee500030, 8, ARM_CEXT_IWMMXT, do_iwmmxt_tmovmsk},
  {"tmovmskw",   0xee900030, 8, ARM_CEXT_IWMMXT, do_iwmmxt_tmovmsk},
  {"tmrc",       0xee100110, 4, ARM_CEXT_IWMMXT, do_iwmmxt_tmrc},
  {"tmrrc",      0xec500000, 5, ARM_CEXT_IWMMXT, do_iwmmxt_tmrrc},
  {"torcb",      0xee130150, 5, ARM_CEXT_IWMMXT, do_iwmmxt_torc},
  {"torch",      0xee530150, 5, ARM_CEXT_IWMMXT, do_iwmmxt_torc},
  {"torcw",      0xee930150, 5, ARM_CEXT_IWMMXT, do_iwmmxt_torc},
  {"waccb",      0xee0001c0, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wacch",      0xee4001c0, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"waccw",      0xee8001c0, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"waddbss",    0xee300180, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddb",      0xee000180, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddbus",    0xee100180, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddhss",    0xee700180, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddh",      0xee400180, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddhus",    0xee500180, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddwss",    0xeeb00180, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddw",      0xee800180, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waddwus",    0xee900180, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"waligni",    0xee000020, 7, ARM_CEXT_IWMMXT, do_iwmmxt_waligni},
  {"walignr0",   0xee800020, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"walignr1",   0xee900020, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"walignr2",   0xeea00020, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"walignr3",   0xeeb00020, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wand",       0xee200000, 4, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wandn",      0xee300000, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wavg2b",     0xee800000, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wavg2br",    0xee900000, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wavg2h",     0xeec00000, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wavg2hr",    0xeed00000, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpeqb",    0xee000060, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpeqh",    0xee400060, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpeqw",    0xee800060, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpgtub",   0xee100060, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpgtuh",   0xee500060, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpgtuw",   0xee900060, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpgtsb",   0xee300060, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpgtsh",   0xee700060, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wcmpgtsw",   0xeeb00060, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wldrb",      0xec100000, 5, ARM_CEXT_IWMMXT, do_iwmmxt_byte_addr},
  {"wldrh",      0xec100100, 5, ARM_CEXT_IWMMXT, do_iwmmxt_byte_addr},
  {"wldrw",      0xec100200, 5, ARM_CEXT_IWMMXT, do_iwmmxt_word_addr},
  {"wldrd",      0xec100300, 5, ARM_CEXT_IWMMXT, do_iwmmxt_word_addr},
  {"wmacs",      0xee600100, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmacsz",     0xee700100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmacu",      0xee400100, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmacuz",     0xee500100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmadds",     0xeea00100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaddu",     0xee800100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaxsb",     0xee200160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaxsh",     0xee600160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaxsw",     0xeea00160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaxub",     0xee000160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaxuh",     0xee400160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmaxuw",     0xee800160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wminsb",     0xee300160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wminsh",     0xee700160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wminsw",     0xeeb00160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wminub",     0xee100160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wminuh",     0xee500160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wminuw",     0xee900160, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmov",       0xee000000, 4, ARM_CEXT_IWMMXT, do_iwmmxt_wmov},
  {"wmulsm",     0xee300100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmulsl",     0xee200100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmulum",     0xee100100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wmulul",     0xee000100, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wor",        0xee000000, 3, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wpackhss",   0xee700080, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wpackhus",   0xee500080, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wpackwss",   0xeeb00080, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wpackwus",   0xee900080, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wpackdss",   0xeef00080, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wpackdus",   0xeed00080, 8, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wrorh",      0xee700040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wrorhg",     0xee700148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wrorw",      0xeeb00040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wrorwg",     0xeeb00148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wrord",      0xeef00040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wrordg",     0xeef00148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsadb",      0xee000120, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsadbz",     0xee100120, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsadh",      0xee400120, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsadhz",     0xee500120, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wshufh",     0xee0001e0, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wshufh},
  {"wsllh",      0xee500040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsllhg",     0xee500148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsllw",      0xee900040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsllwg",     0xee900148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wslld",      0xeed00040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wslldg",     0xeed00148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsrah",      0xee400040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsrahg",     0xee400148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsraw",      0xee800040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsrawg",     0xee800148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsrad",      0xeec00040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsradg",     0xeec00148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsrlh",      0xee600040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsrlhg",     0xee600148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsrlw",      0xeea00040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsrlwg",     0xeea00148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wsrld",      0xeee00040, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsrldg",     0xeee00148, 6, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwcg},
  {"wstrb",      0xec000000, 5, ARM_CEXT_IWMMXT, do_iwmmxt_byte_addr},
  {"wstrh",      0xec000100, 5, ARM_CEXT_IWMMXT, do_iwmmxt_byte_addr},
  {"wstrw",      0xec000200, 5, ARM_CEXT_IWMMXT, do_iwmmxt_word_addr},
  {"wstrd",      0xec000300, 5, ARM_CEXT_IWMMXT, do_iwmmxt_word_addr},
  {"wsubbss",    0xee3001a0, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubb",      0xee0001a0, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubbus",    0xee1001a0, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubhss",    0xee7001a0, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubh",      0xee4001a0, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubhus",    0xee5001a0, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubwss",    0xeeb001a0, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubw",      0xee8001a0, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wsubwus",    0xee9001a0, 7, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wunpckehub", 0xee0000c0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckehuh", 0xee4000c0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckehuw", 0xee8000c0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckehsb", 0xee2000c0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckehsh", 0xee6000c0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckehsw", 0xeea000c0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckihb",  0xee1000c0, 9, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wunpckihh",  0xee5000c0, 9, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wunpckihw",  0xee9000c0, 9, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wunpckelub", 0xee0000e0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckeluh", 0xee4000e0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckeluw", 0xee8000e0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckelsb", 0xee2000e0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckelsh", 0xee6000e0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckelsw", 0xeea000e0, 10, ARM_CEXT_IWMMXT, do_iwmmxt_wrwr},
  {"wunpckilb",  0xee1000e0, 9, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wunpckilh",  0xee5000e0, 9, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wunpckilw",  0xee9000e0, 9, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wxor",       0xee100000, 4, ARM_CEXT_IWMMXT, do_iwmmxt_wrwrwr},
  {"wzero",      0xee300000, 5, ARM_CEXT_IWMMXT, do_iwmmxt_wzero},

  /* Cirrus Maverick instructions.  */
  {"cfldrs",     0xec100400, 6,  ARM_CEXT_MAVERICK, do_mav_ldst_1},
  {"cfldrd",     0xec500400, 6,  ARM_CEXT_MAVERICK, do_mav_ldst_2},
  {"cfldr32",    0xec100500, 7,  ARM_CEXT_MAVERICK, do_mav_ldst_3},
  {"cfldr64",    0xec500500, 7,  ARM_CEXT_MAVERICK, do_mav_ldst_4},
  {"cfstrs",     0xec000400, 6,  ARM_CEXT_MAVERICK, do_mav_ldst_1},
  {"cfstrd",     0xec400400, 6,  ARM_CEXT_MAVERICK, do_mav_ldst_2},
  {"cfstr32",    0xec000500, 7,  ARM_CEXT_MAVERICK, do_mav_ldst_3},
  {"cfstr64",    0xec400500, 7,  ARM_CEXT_MAVERICK, do_mav_ldst_4},
  {"cfmvsr",     0xee000450, 6,  ARM_CEXT_MAVERICK, do_mav_binops_2a},
  {"cfmvrs",     0xee100450, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1a},
  {"cfmvdlr",    0xee000410, 7,  ARM_CEXT_MAVERICK, do_mav_binops_2b},
  {"cfmvrdl",    0xee100410, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1b},
  {"cfmvdhr",    0xee000430, 7,  ARM_CEXT_MAVERICK, do_mav_binops_2b},
  {"cfmvrdh",    0xee100430, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1b},
  {"cfmv64lr",   0xee000510, 8,  ARM_CEXT_MAVERICK, do_mav_binops_2c},
  {"cfmvr64l",   0xee100510, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1c},
  {"cfmv64hr",   0xee000530, 8,  ARM_CEXT_MAVERICK, do_mav_binops_2c},
  {"cfmvr64h",   0xee100530, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1c},
  {"cfmval32",   0xee200440, 8,  ARM_CEXT_MAVERICK, do_mav_binops_3a},
  {"cfmv32al",   0xee100440, 8,  ARM_CEXT_MAVERICK, do_mav_binops_3b},
  {"cfmvam32",   0xee200460, 8,  ARM_CEXT_MAVERICK, do_mav_binops_3a},
  {"cfmv32am",   0xee100460, 8,  ARM_CEXT_MAVERICK, do_mav_binops_3b},
  {"cfmvah32",   0xee200480, 8,  ARM_CEXT_MAVERICK, do_mav_binops_3a},
  {"cfmv32ah",   0xee100480, 8,  ARM_CEXT_MAVERICK, do_mav_binops_3b},
  {"cfmva32",    0xee2004a0, 7,  ARM_CEXT_MAVERICK, do_mav_binops_3a},
  {"cfmv32a",    0xee1004a0, 7,  ARM_CEXT_MAVERICK, do_mav_binops_3b},
  {"cfmva64",    0xee2004c0, 7,  ARM_CEXT_MAVERICK, do_mav_binops_3c},
  {"cfmv64a",    0xee1004c0, 7,  ARM_CEXT_MAVERICK, do_mav_binops_3d},
  {"cfmvsc32",   0xee2004e0, 8,  ARM_CEXT_MAVERICK, do_mav_dspsc_1},
  {"cfmv32sc",   0xee1004e0, 8,  ARM_CEXT_MAVERICK, do_mav_dspsc_2},
  {"cfcpys",     0xee000400, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1d},
  {"cfcpyd",     0xee000420, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1e},
  {"cfcvtsd",    0xee000460, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1f},
  {"cfcvtds",    0xee000440, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1g},
  {"cfcvt32s",   0xee000480, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1h},
  {"cfcvt32d",   0xee0004a0, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1i},
  {"cfcvt64s",   0xee0004c0, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1j},
  {"cfcvt64d",   0xee0004e0, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1k},
  {"cfcvts32",   0xee100580, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1l},
  {"cfcvtd32",   0xee1005a0, 8,  ARM_CEXT_MAVERICK, do_mav_binops_1m},
  {"cftruncs32", 0xee1005c0, 10, ARM_CEXT_MAVERICK, do_mav_binops_1l},
  {"cftruncd32", 0xee1005e0, 10, ARM_CEXT_MAVERICK, do_mav_binops_1m},
  {"cfrshl32",   0xee000550, 8,  ARM_CEXT_MAVERICK, do_mav_triple_4a},
  {"cfrshl64",   0xee000570, 8,  ARM_CEXT_MAVERICK, do_mav_triple_4b},
  {"cfsh32",     0xee000500, 6,  ARM_CEXT_MAVERICK, do_mav_shift_1},
  {"cfsh64",     0xee200500, 6,  ARM_CEXT_MAVERICK, do_mav_shift_2},
  {"cfcmps",     0xee100490, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5a},
  {"cfcmpd",     0xee1004b0, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5b},
  {"cfcmp32",    0xee100590, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5c},
  {"cfcmp64",    0xee1005b0, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5d},
  {"cfabss",     0xee300400, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1d},
  {"cfabsd",     0xee300420, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1e},
  {"cfnegs",     0xee300440, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1d},
  {"cfnegd",     0xee300460, 6,  ARM_CEXT_MAVERICK, do_mav_binops_1e},
  {"cfadds",     0xee300480, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5e},
  {"cfaddd",     0xee3004a0, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5f},
  {"cfsubs",     0xee3004c0, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5e},
  {"cfsubd",     0xee3004e0, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5f},
  {"cfmuls",     0xee100400, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5e},
  {"cfmuld",     0xee100420, 6,  ARM_CEXT_MAVERICK, do_mav_triple_5f},
  {"cfabs32",    0xee300500, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1n},
  {"cfabs64",    0xee300520, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1o},
  {"cfneg32",    0xee300540, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1n},
  {"cfneg64",    0xee300560, 7,  ARM_CEXT_MAVERICK, do_mav_binops_1o},
  {"cfadd32",    0xee300580, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5g},
  {"cfadd64",    0xee3005a0, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5h},
  {"cfsub32",    0xee3005c0, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5g},
  {"cfsub64",    0xee3005e0, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5h},
  {"cfmul32",    0xee100500, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5g},
  {"cfmul64",    0xee100520, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5h},
  {"cfmac32",    0xee100540, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5g},
  {"cfmsc32",    0xee100560, 7,  ARM_CEXT_MAVERICK, do_mav_triple_5g},
  {"cfmadd32",   0xee000600, 8,  ARM_CEXT_MAVERICK, do_mav_quad_6a},
  {"cfmsub32",   0xee100600, 8,  ARM_CEXT_MAVERICK, do_mav_quad_6a},
  {"cfmadda32",  0xee200600, 9,  ARM_CEXT_MAVERICK, do_mav_quad_6b},
  {"cfmsuba32",  0xee300600, 9,  ARM_CEXT_MAVERICK, do_mav_quad_6b},
};

/* Iterate over the base tables to create the instruction patterns.  */

static void
build_arm_ops_hsh (void)
{
  unsigned int i;
  unsigned int j;
  static struct obstack insn_obstack;

  obstack_begin (&insn_obstack, 4000);

  for (i = 0; i < sizeof (insns) / sizeof (struct asm_opcode); i++)
    {
      const struct asm_opcode *insn = insns + i;

      if (insn->cond_offset != 0)
	{
	  /* Insn supports conditional execution.  Build the varaints
	     and insert them in the hash table.  */
	  for (j = 0; j < sizeof (conds) / sizeof (struct asm_cond); j++)
	    {
	      unsigned len = strlen (insn->template);
	      struct asm_opcode *new;
	      char *template;

	      new = obstack_alloc (&insn_obstack, sizeof (struct asm_opcode));
	      /* All condition codes are two characters.  */
	      template = obstack_alloc (&insn_obstack, len + 3);

	      strncpy (template, insn->template, insn->cond_offset);
	      strcpy (template + insn->cond_offset, conds[j].template);
	      if (len > insn->cond_offset)
		strcpy (template + insn->cond_offset + 2,
			insn->template + insn->cond_offset);
	      new->template = template;
	      new->cond_offset = 0;
	      new->variant = insn->variant;
	      new->parms = insn->parms;
	      new->value = (insn->value & ~COND_MASK) | conds[j].value;

	      hash_insert (arm_ops_hsh, new->template, (PTR) new);
	    }
	}
      /* Finally, insert the unconditional insn in the table directly;
	 no need to build a copy.  */
      hash_insert (arm_ops_hsh, insn->template, (PTR) insn);
    }
}


static const struct thumb_opcode tinsns[] =
{
  /* Thumb v1 (ARMv4T).  */
  {"adc",	0x4140,		2,	ARM_EXT_V4T, do_t_arit},
  {"add",	0x0000,		2,	ARM_EXT_V4T, do_t_add},
  {"and",	0x4000,		2,	ARM_EXT_V4T, do_t_arit},
  {"asr",	0x0000,		2,	ARM_EXT_V4T, do_t_asr},
  {"b",		T_OPCODE_BRANCH, 2,	ARM_EXT_V4T, do_t_branch12},
  {"beq",	0xd0fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bne",	0xd1fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bcs",	0xd2fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bhs",	0xd2fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bcc",	0xd3fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bul",	0xd3fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"blo",	0xd3fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bmi",	0xd4fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bpl",	0xd5fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bvs",	0xd6fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bvc",	0xd7fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bhi",	0xd8fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bls",	0xd9fe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bge",	0xdafe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"blt",	0xdbfe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bgt",	0xdcfe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"ble",	0xddfe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bal",	0xdefe,		2,	ARM_EXT_V4T, do_t_branch9},
  {"bic",	0x4380,		2,	ARM_EXT_V4T, do_t_arit},
  {"bl",	0xf7fffffe,	4,	ARM_EXT_V4T, do_t_branch23},
  {"bx",	0x4700,		2,	ARM_EXT_V4T, do_t_bx},
  {"cmn",	T_OPCODE_CMN,	2,	ARM_EXT_V4T, do_t_arit},
  {"cmp",	0x0000,		2,	ARM_EXT_V4T, do_t_compare},
  {"eor",	0x4040,		2,	ARM_EXT_V4T, do_t_arit},
  {"ldmia",	0xc800,		2,	ARM_EXT_V4T, do_t_ldmstm},
  {"ldr",	0x0000,		2,	ARM_EXT_V4T, do_t_ldr},
  {"ldrb",	0x0000,		2,	ARM_EXT_V4T, do_t_ldrb},
  {"ldrh",	0x0000,		2,	ARM_EXT_V4T, do_t_ldrh},
  {"ldrsb",	0x5600,		2,	ARM_EXT_V4T, do_t_lds},
  {"ldrsh",	0x5e00,		2,	ARM_EXT_V4T, do_t_lds},
  {"ldsb",	0x5600,		2,	ARM_EXT_V4T, do_t_lds},
  {"ldsh",	0x5e00,		2,	ARM_EXT_V4T, do_t_lds},
  {"lsl",	0x0000,		2,	ARM_EXT_V4T, do_t_lsl},
  {"lsr",	0x0000,		2,	ARM_EXT_V4T, do_t_lsr},
  {"mov",	0x0000,		2,	ARM_EXT_V4T, do_t_mov},
  {"mul",	T_OPCODE_MUL,	2,	ARM_EXT_V4T, do_t_arit},
  {"mvn",	T_OPCODE_MVN,	2,	ARM_EXT_V4T, do_t_arit},
  {"neg",	T_OPCODE_NEG,	2,	ARM_EXT_V4T, do_t_arit},
  {"orr",	0x4300,		2,	ARM_EXT_V4T, do_t_arit},
  {"pop",	0xbc00,		2,	ARM_EXT_V4T, do_t_push_pop},
  {"push",	0xb400,		2,	ARM_EXT_V4T, do_t_push_pop},
  {"ror",	0x41c0,		2,	ARM_EXT_V4T, do_t_arit},
  {"sbc",	0x4180,		2,	ARM_EXT_V4T, do_t_arit},
  {"stmia",	0xc000,		2,	ARM_EXT_V4T, do_t_ldmstm},
  {"str",	0x0000,		2,	ARM_EXT_V4T, do_t_str},
  {"strb",	0x0000,		2,	ARM_EXT_V4T, do_t_strb},
  {"strh",	0x0000,		2,	ARM_EXT_V4T, do_t_strh},
  {"swi",	0xdf00,		2,	ARM_EXT_V4T, do_t_swi},
  {"sub",	0x0000,		2,	ARM_EXT_V4T, do_t_sub},
  {"tst",	T_OPCODE_TST,	2,	ARM_EXT_V4T, do_t_arit},
  /* Pseudo ops:  */
  {"adr",       0x0000,         2,      ARM_EXT_V4T, do_t_adr},
  {"nop",       0x46C0,         2,      ARM_EXT_V4T, do_t_nop},      /* mov r8,r8  */
  /* Thumb v2 (ARMv5T).  */
  {"blx",	0,		0,	ARM_EXT_V5T, do_t_blx},
  {"bkpt",	0xbe00,		2,	ARM_EXT_V5T, do_t_bkpt},

  /* ARM V6.  */
  {"cpsie",	0xb660,		2,	ARM_EXT_V6,  do_t_cps},
  {"cpsid",     0xb670,		2,	ARM_EXT_V6,  do_t_cps},
  {"cpy",	0x4600,		2,	ARM_EXT_V6,  do_t_cpy},
  {"rev",	0xba00,		2,	ARM_EXT_V6,  do_t_arit},
  {"rev16",	0xba40,		2,	ARM_EXT_V6,  do_t_arit},
  {"revsh",	0xbac0,		2,	ARM_EXT_V6,  do_t_arit},
  {"setend",	0xb650,		2,	ARM_EXT_V6,  do_t_setend},
  {"sxth",	0xb200,		2,	ARM_EXT_V6,  do_t_arit},
  {"sxtb",	0xb240,		2,	ARM_EXT_V6,  do_t_arit},
  {"uxth",	0xb280,		2,	ARM_EXT_V6,  do_t_arit},
  {"uxtb",	0xb2c0,		2,	ARM_EXT_V6,  do_t_arit},
};

void
md_begin (void)
{
  unsigned mach;
  unsigned int i;

  if (   (arm_ops_hsh = hash_new ()) == NULL
      || (arm_tops_hsh = hash_new ()) == NULL
      || (arm_cond_hsh = hash_new ()) == NULL
      || (arm_shift_hsh = hash_new ()) == NULL
      || (arm_psr_hsh = hash_new ()) == NULL)
    as_fatal (_("virtual memory exhausted"));

  build_arm_ops_hsh ();
  for (i = 0; i < sizeof (tinsns) / sizeof (struct thumb_opcode); i++)
    hash_insert (arm_tops_hsh, tinsns[i].template, (PTR) (tinsns + i));
  for (i = 0; i < sizeof (conds) / sizeof (struct asm_cond); i++)
    hash_insert (arm_cond_hsh, conds[i].template, (PTR) (conds + i));
  for (i = 0; i < sizeof (shift_names) / sizeof (struct asm_shift_name); i++)
    hash_insert (arm_shift_hsh, shift_names[i].name, (PTR) (shift_names + i));
  for (i = 0; i < sizeof (psrs) / sizeof (struct asm_psr); i++)
    hash_insert (arm_psr_hsh, psrs[i].template, (PTR) (psrs + i));

  for (i = (int) REG_TYPE_FIRST; i < (int) REG_TYPE_MAX; i++)
    build_reg_hsh (all_reg_maps + i);

  set_constant_flonums ();

  /* Set the cpu variant based on the command-line options.  We prefer
     -mcpu= over -march= if both are set (as for GCC); and we prefer
     -mfpu= over any other way of setting the floating point unit.
     Use of legacy options with new options are faulted.  */
  if (legacy_cpu != -1)
    {
      if (mcpu_cpu_opt != -1 || march_cpu_opt != -1)
	as_bad (_("use of old and new-style options to set CPU type"));

      mcpu_cpu_opt = legacy_cpu;
    }
  else if (mcpu_cpu_opt == -1)
    mcpu_cpu_opt = march_cpu_opt;

  if (legacy_fpu != -1)
    {
      if (mfpu_opt != -1)
	as_bad (_("use of old and new-style options to set FPU type"));

      mfpu_opt = legacy_fpu;
    }
  else if (mfpu_opt == -1)
    {
#if !(defined (TE_LINUX) || defined (TE_NetBSD) || defined (TE_VXWORKS)) 
      /* Some environments specify a default FPU.  If they don't, infer it
	 from the processor.  */
      if (mcpu_fpu_opt != -1)
	mfpu_opt = mcpu_fpu_opt;
      else
	mfpu_opt = march_fpu_opt;
#else
      mfpu_opt = FPU_DEFAULT;
#endif
    }

  if (mfpu_opt == -1)
    {
      if (mcpu_cpu_opt == -1)
	mfpu_opt = FPU_DEFAULT;
      else if (mcpu_cpu_opt & ARM_EXT_V5)
	mfpu_opt = FPU_ARCH_VFP_V2;
      else
	mfpu_opt = FPU_ARCH_FPA;
    }

  if (mcpu_cpu_opt == -1)
    mcpu_cpu_opt = CPU_DEFAULT;

  cpu_variant = mcpu_cpu_opt | mfpu_opt;

#if defined OBJ_COFF || defined OBJ_ELF
  {
    unsigned int flags = 0;

#if defined OBJ_ELF
    flags = meabi_flags;

    switch (meabi_flags)
      {
      case EF_ARM_EABI_UNKNOWN:
#endif
	/* Set the flags in the private structure.  */
	if (uses_apcs_26)      flags |= F_APCS26;
	if (support_interwork) flags |= F_INTERWORK;
	if (uses_apcs_float)   flags |= F_APCS_FLOAT;
	if (pic_code)          flags |= F_PIC;
	if ((cpu_variant & FPU_ANY) == FPU_NONE
	     || (cpu_variant & FPU_ANY) == FPU_ARCH_VFP) /* VFP layout only.  */
	  flags |= F_SOFT_FLOAT;

	switch (mfloat_abi_opt)
	  {
	  case ARM_FLOAT_ABI_SOFT:
	  case ARM_FLOAT_ABI_SOFTFP:
	    flags |= F_SOFT_FLOAT;
	    break;

	  case ARM_FLOAT_ABI_HARD:
	    if (flags & F_SOFT_FLOAT)
	      as_bad (_("hard-float conflicts with specified fpu"));
	    break;
	  }

	/* Using VFP conventions (even if soft-float).  */
	if (cpu_variant & FPU_VFP_EXT_NONE)
	  flags |= F_VFP_FLOAT;

#if defined OBJ_ELF
	if (cpu_variant & FPU_ARCH_MAVERICK)
	    flags |= EF_ARM_MAVERICK_FLOAT;
	break;

      case EF_ARM_EABI_VER4:
	/* No additional flags to set.  */
	break;

      default:
	abort ();
      }
#endif
    bfd_set_private_flags (stdoutput, flags);

    /* We have run out flags in the COFF header to encode the
       status of ATPCS support, so instead we create a dummy,
       empty, debug section called .arm.atpcs.  */
    if (atpcs)
      {
	asection * sec;

	sec = bfd_make_section (stdoutput, ".arm.atpcs");

	if (sec != NULL)
	  {
	    bfd_set_section_flags
	      (stdoutput, sec, SEC_READONLY | SEC_DEBUGGING /* | SEC_HAS_CONTENTS */);
	    bfd_set_section_size (stdoutput, sec, 0);
	    bfd_set_section_contents (stdoutput, sec, NULL, 0, 0);
	  }
      }
  }
#endif

  /* Record the CPU type as well.  */
  switch (cpu_variant & ARM_CPU_MASK)
    {
    case ARM_2:
      mach = bfd_mach_arm_2;
      break;

    case ARM_3: 		/* Also ARM_250.  */
      mach = bfd_mach_arm_2a;
      break;

    case ARM_6:			/* Also ARM_7.  */
      mach = bfd_mach_arm_3;
      break;

    default:
      mach = bfd_mach_arm_unknown;
      break;
    }

  /* Catch special cases.  */
  if (cpu_variant & ARM_CEXT_IWMMXT)
    mach = bfd_mach_arm_iWMMXt;
  else if (cpu_variant & ARM_CEXT_XSCALE)
    mach = bfd_mach_arm_XScale;
  else if (cpu_variant & ARM_CEXT_MAVERICK)
    mach = bfd_mach_arm_ep9312;
  else if (cpu_variant & ARM_EXT_V5E)
    mach = bfd_mach_arm_5TE;
  else if (cpu_variant & ARM_EXT_V5)
    {
      if (cpu_variant & ARM_EXT_V4T)
	mach = bfd_mach_arm_5T;
      else
	mach = bfd_mach_arm_5;
    }
  else if (cpu_variant & ARM_EXT_V4)
    {
      if (cpu_variant & ARM_EXT_V4T)
	mach = bfd_mach_arm_4T;
      else
	mach = bfd_mach_arm_4;
    }
  else if (cpu_variant & ARM_EXT_V3M)
    mach = bfd_mach_arm_3M;

  bfd_set_arch_mach (stdoutput, TARGET_ARCH, mach);
}

/* Turn an integer of n bytes (in val) into a stream of bytes appropriate
   for use in the a.out file, and stores them in the array pointed to by buf.
   This knows about the endian-ness of the target machine and does
   THE RIGHT THING, whatever it is.  Possible values for n are 1 (byte)
   2 (short) and 4 (long)  Floating numbers are put out as a series of
   LITTLENUMS (shorts, here at least).  */

void
md_number_to_chars (char * buf, valueT val, int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

static valueT
md_chars_to_number (char * buf, int n)
{
  valueT result = 0;
  unsigned char * where = (unsigned char *) buf;

  if (target_big_endian)
    {
      while (n--)
	{
	  result <<= 8;
	  result |= (*where++ & 255);
	}
    }
  else
    {
      while (n--)
	{
	  result <<= 8;
	  result |= (where[n] & 255);
	}
    }

  return result;
}

/* Turn a string in input_line_pointer into a floating point constant
   of type TYPE, and store the appropriate bytes in *LITP.  The number
   of LITTLENUMS emitted is stored in *SIZEP.  An error message is
   returned, or NULL on OK.

   Note that fp constants aren't represent in the normal way on the ARM.
   In big endian mode, things are as expected.  However, in little endian
   mode fp constants are big-endian word-wise, and little-endian byte-wise
   within the words.  For example, (double) 1.1 in big endian mode is
   the byte sequence 3f f1 99 99 99 99 99 9a, and in little endian mode is
   the byte sequence 99 99 f1 3f 9a 99 99 99.

   ??? The format of 12 byte floats is uncertain according to gcc's arm.h.  */

char *
md_atof (int type, char * litP, int * sizeP)
{
  int prec;
  LITTLENUM_TYPE words[MAX_LITTLENUMS];
  char *t;
  int i;

  switch (type)
    {
    case 'f':
    case 'F':
    case 's':
    case 'S':
      prec = 2;
      break;

    case 'd':
    case 'D':
    case 'r':
    case 'R':
      prec = 4;
      break;

    case 'x':
    case 'X':
      prec = 6;
      break;

    case 'p':
    case 'P':
      prec = 6;
      break;

    default:
      *sizeP = 0;
      return _("bad call to MD_ATOF()");
    }

  t = atof_ieee (input_line_pointer, type, words);
  if (t)
    input_line_pointer = t;
  *sizeP = prec * 2;

  if (target_big_endian)
    {
      for (i = 0; i < prec; i++)
	{
	  md_number_to_chars (litP, (valueT) words[i], 2);
	  litP += 2;
	}
    }
  else
    {
      if (cpu_variant & FPU_ARCH_VFP)
	for (i = prec - 1; i >= 0; i--)
	  {
	    md_number_to_chars (litP, (valueT) words[i], 2);
	    litP += 2;
	  }
      else
	/* For a 4 byte float the order of elements in `words' is 1 0.
	   For an 8 byte float the order is 1 0 3 2.  */
	for (i = 0; i < prec; i += 2)
	  {
	    md_number_to_chars (litP, (valueT) words[i + 1], 2);
	    md_number_to_chars (litP + 2, (valueT) words[i], 2);
	    litP += 4;
	  }
    }

  return 0;
}

/* The knowledge of the PC's pipeline offset is built into the insns
   themselves.  */

long
md_pcrel_from (fixS * fixP)
{
  if (fixP->fx_addsy
      && S_GET_SEGMENT (fixP->fx_addsy) == undefined_section
      && fixP->fx_subsy == NULL)
    return 0;

  if (fixP->fx_pcrel && (fixP->fx_r_type == BFD_RELOC_ARM_THUMB_ADD))
    {
      /* PC relative addressing on the Thumb is slightly odd
	 as the bottom two bits of the PC are forced to zero
	 for the calculation.  */
      return (fixP->fx_where + fixP->fx_frag->fr_address) & ~3;
    }

#ifdef TE_WINCE
  /* The pattern was adjusted to accommodate CE's off-by-one fixups,
     so we un-adjust here to compensate for the accommodation.  */
  return fixP->fx_where + fixP->fx_frag->fr_address + 8;
#else
  return fixP->fx_where + fixP->fx_frag->fr_address;
#endif
}

/* Round up a section size to the appropriate boundary.  */

valueT
md_section_align (segT   segment ATTRIBUTE_UNUSED,
		  valueT size)
{
#ifdef OBJ_ELF
  return size;
#else
  /* Round all sects to multiple of 4.  */
  return (size + 3) & ~3;
#endif
}

/* Under ELF we need to default _GLOBAL_OFFSET_TABLE.
   Otherwise we have no need to default values of symbols.  */

symbolS *
md_undefined_symbol (char * name ATTRIBUTE_UNUSED)
{
#ifdef OBJ_ELF
  if (name[0] == '_' && name[1] == 'G'
      && streq (name, GLOBAL_OFFSET_TABLE_NAME))
    {
      if (!GOT_symbol)
	{
	  if (symbol_find (name))
	    as_bad ("GOT already in the symbol table");

	  GOT_symbol = symbol_new (name, undefined_section,
				   (valueT) 0, & zero_address_frag);
	}

      return GOT_symbol;
    }
#endif

  return 0;
}

void
md_apply_fix3 (fixS *   fixP,
	       valueT * valP,
	       segT     seg)
{
  offsetT        value = * valP;
  offsetT        newval;
  unsigned int   newimm;
  unsigned long  temp;
  int            sign;
  char *         buf = fixP->fx_where + fixP->fx_frag->fr_literal;
  arm_fix_data * arm_data = (arm_fix_data *) fixP->tc_fix_data;

  assert (fixP->fx_r_type <= BFD_RELOC_UNUSED);

  /* Note whether this will delete the relocation.  */
  if (fixP->fx_addsy == 0 && !fixP->fx_pcrel)
    fixP->fx_done = 1;

  /* If this symbol is in a different section then we need to leave it for
     the linker to deal with.  Unfortunately, md_pcrel_from can't tell,
     so we have to undo it's effects here.  */
  if (fixP->fx_pcrel)
    {
      if (fixP->fx_addsy != NULL
	  && S_IS_DEFINED (fixP->fx_addsy)
	  && S_GET_SEGMENT (fixP->fx_addsy) != seg)
	value += md_pcrel_from (fixP);
    }

  /* Remember value for emit_reloc.  */
  fixP->fx_addnumber = value;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_NONE:
      /* This will need to go in the object file.  */
      fixP->fx_done = 0;
      break;
  
    case BFD_RELOC_ARM_IMMEDIATE:
      /* We claim that this fixup has been processed here,
	 even if in fact we generate an error because we do
	 not have a reloc for it, so tc_gen_reloc will reject it.  */
      fixP->fx_done = 1;

      if (fixP->fx_addsy
	  && ! S_IS_DEFINED (fixP->fx_addsy))
	{
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("undefined symbol %s used as an immediate value"),
			S_GET_NAME (fixP->fx_addsy));
	  break;
	}

      newimm = validate_immediate (value);
      temp = md_chars_to_number (buf, INSN_SIZE);

      /* If the instruction will fail, see if we can fix things up by
	 changing the opcode.  */
      if (newimm == (unsigned int) FAIL
	  && (newimm = negate_data_op (&temp, value)) == (unsigned int) FAIL)
	{
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("invalid constant (%lx) after fixup"),
			(unsigned long) value);
	  break;
	}

      newimm |= (temp & 0xfffff000);
      md_number_to_chars (buf, (valueT) newimm, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_ADRL_IMMEDIATE:
      {
	unsigned int highpart = 0;
	unsigned int newinsn  = 0xe1a00000; /* nop.  */

	newimm = validate_immediate (value);
	temp = md_chars_to_number (buf, INSN_SIZE);

	/* If the instruction will fail, see if we can fix things up by
	   changing the opcode.  */
	if (newimm == (unsigned int) FAIL
	    && (newimm = negate_data_op (& temp, value)) == (unsigned int) FAIL)
	  {
	    /* No ?  OK - try using two ADD instructions to generate
               the value.  */
	    newimm = validate_immediate_twopart (value, & highpart);

	    /* Yes - then make sure that the second instruction is
               also an add.  */
	    if (newimm != (unsigned int) FAIL)
	      newinsn = temp;
	    /* Still No ?  Try using a negated value.  */
	    else if ((newimm = validate_immediate_twopart (- value, & highpart)) != (unsigned int) FAIL)
	      temp = newinsn = (temp & OPCODE_MASK) | OPCODE_SUB << DATA_OP_SHIFT;
	    /* Otherwise - give up.  */
	    else
	      {
		as_bad_where (fixP->fx_file, fixP->fx_line,
			      _("unable to compute ADRL instructions for PC offset of 0x%lx"),
			      (long) value);
		break;
	      }

	    /* Replace the first operand in the 2nd instruction (which
	       is the PC) with the destination register.  We have
	       already added in the PC in the first instruction and we
	       do not want to do it again.  */
	    newinsn &= ~ 0xf0000;
	    newinsn |= ((newinsn & 0x0f000) << 4);
	  }

	newimm |= (temp & 0xfffff000);
	md_number_to_chars (buf, (valueT) newimm, INSN_SIZE);

	highpart |= (newinsn & 0xfffff000);
	md_number_to_chars (buf + INSN_SIZE, (valueT) highpart, INSN_SIZE);
      }
      break;

    case BFD_RELOC_ARM_OFFSET_IMM:
      sign = value >= 0;

      if (value < 0)
	value = - value;

      if (validate_offset_imm (value, 0) == FAIL)
	{
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("bad immediate value for offset (%ld)"),
			(long) value);
	  break;
	}

      newval = md_chars_to_number (buf, INSN_SIZE);
      newval &= 0xff7ff000;
      newval |= value | (sign ? INDEX_UP : 0);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_OFFSET_IMM8:
    case BFD_RELOC_ARM_HWLITERAL:
      sign = value >= 0;

      if (value < 0)
	value = - value;

      if (validate_offset_imm (value, 1) == FAIL)
	{
	  if (fixP->fx_r_type == BFD_RELOC_ARM_HWLITERAL)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid literal constant: pool needs to be closer"));
	  else
	    as_bad (_("bad immediate value for half-word offset (%ld)"),
		    (long) value);
	  break;
	}

      newval = md_chars_to_number (buf, INSN_SIZE);
      newval &= 0xff7ff0f0;
      newval |= ((value >> 4) << 8) | (value & 0xf) | (sign ? INDEX_UP : 0);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_LITERAL:
      sign = value >= 0;

      if (value < 0)
	value = - value;

      if (validate_offset_imm (value, 0) == FAIL)
	{
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("invalid literal constant: pool needs to be closer"));
	  break;
	}

      newval = md_chars_to_number (buf, INSN_SIZE);
      newval &= 0xff7ff000;
      newval |= value | (sign ? INDEX_UP : 0);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_SHIFT_IMM:
      newval = md_chars_to_number (buf, INSN_SIZE);
      if (((unsigned long) value) > 32
	  || (value == 32
	      && (((newval & 0x60) == 0) || (newval & 0x60) == 0x60)))
	{
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("shift expression is too large"));
	  break;
	}

      if (value == 0)
	/* Shifts of zero must be done as lsl.  */
	newval &= ~0x60;
      else if (value == 32)
	value = 0;
      newval &= 0xfffff07f;
      newval |= (value & 0x1f) << 7;
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_SMI:
      if (((unsigned long) value) > 0xffff)
	as_bad_where (fixP->fx_file, fixP->fx_line,
		      _("invalid smi expression"));
      newval = md_chars_to_number (buf, INSN_SIZE) & 0xfff000f0;
      newval |= (value & 0xf) | ((value & 0xfff0) << 4);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_SWI:
      if (arm_data->thumb_mode)
	{
	  if (((unsigned long) value) > 0xff)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid swi expression"));
	  newval = md_chars_to_number (buf, THUMB_SIZE) & 0xff00;
	  newval |= value;
	  md_number_to_chars (buf, newval, THUMB_SIZE);
	}
      else
	{
	  if (((unsigned long) value) > 0x00ffffff)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid swi expression"));
	  newval = md_chars_to_number (buf, INSN_SIZE) & 0xff000000;
	  newval |= value;
	  md_number_to_chars (buf, newval, INSN_SIZE);
	}
      break;

    case BFD_RELOC_ARM_MULTI:
      if (((unsigned long) value) > 0xffff)
	as_bad_where (fixP->fx_file, fixP->fx_line,
		      _("invalid expression in load/store multiple"));
      newval = value | md_chars_to_number (buf, INSN_SIZE);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_PCREL_BRANCH:
      newval = md_chars_to_number (buf, INSN_SIZE);

      /* Sign-extend a 24-bit number.  */
#define SEXT24(x)	((((x) & 0xffffff) ^ (~ 0x7fffff)) + 0x800000)

#ifdef OBJ_ELF
      value = fixP->fx_offset;
#endif

      /* We are going to store value (shifted right by two) in the
	 instruction, in a 24 bit, signed field.  Thus we need to check
	 that none of the top 8 bits of the shifted value (top 7 bits of
         the unshifted, unsigned value) are set, or that they are all set.  */
      if ((value & ~ ((offsetT) 0x1ffffff)) != 0
	  && ((value & ~ ((offsetT) 0x1ffffff)) != ~ ((offsetT) 0x1ffffff)))
	{
#ifdef OBJ_ELF
	  /* Normally we would be stuck at this point, since we cannot store
	     the absolute address that is the destination of the branch in the
	     24 bits of the branch instruction.  If however, we happen to know
	     that the destination of the branch is in the same section as the
	     branch instruction itself, then we can compute the relocation for
	     ourselves and not have to bother the linker with it.

	     FIXME: The test for OBJ_ELF is only here because I have not
	     worked out how to do this for OBJ_COFF.  */
	  if (fixP->fx_addsy != NULL
	      && S_IS_DEFINED (fixP->fx_addsy)
	      && S_GET_SEGMENT (fixP->fx_addsy) == seg)
	    {
	      /* Get pc relative value to go into the branch.  */
	      value = * valP;

	      /* Permit a backward branch provided that enough bits
		 are set.  Allow a forwards branch, provided that
		 enough bits are clear.  */
	      if (   (value & ~ ((offsetT) 0x1ffffff)) == ~ ((offsetT) 0x1ffffff)
		  || (value & ~ ((offsetT) 0x1ffffff)) == 0)
		fixP->fx_done = 1;
	    }

	  if (! fixP->fx_done)
#endif
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("GAS can't handle same-section branch dest >= 0x04000000"));
	}

      value >>= 2;
      value += SEXT24 (newval);

      if (    (value & ~ ((offsetT) 0xffffff)) != 0
	  && ((value & ~ ((offsetT) 0xffffff)) != ~ ((offsetT) 0xffffff)))
	as_bad_where (fixP->fx_file, fixP->fx_line,
		      _("out of range branch"));

      if (seg->use_rela_p && !fixP->fx_done)
	{
	  /* Must unshift the value before storing it in the addend.  */
	  value <<= 2;
#ifdef OBJ_ELF
	  fixP->fx_offset = value;
#endif
	  fixP->fx_addnumber = value;
	  newval = newval & 0xff000000;
	}
      else
	  newval = (value & 0x00ffffff) | (newval & 0xff000000);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_PCREL_BLX:
      {
	offsetT hbit;
	newval = md_chars_to_number (buf, INSN_SIZE);

#ifdef OBJ_ELF
	value = fixP->fx_offset;
#endif
	hbit   = (value >> 1) & 1;
	value  = (value >> 2) & 0x00ffffff;
	value  = (value + (newval & 0x00ffffff)) & 0x00ffffff;

	if (seg->use_rela_p && !fixP->fx_done)
	  {
	    /* Must sign-extend and unshift the value before storing
	       it in the addend.  */
	    value = SEXT24 (value);
	    value = (value << 2) | hbit;
#ifdef OBJ_ELF
	    fixP->fx_offset = value;
#endif
	    fixP->fx_addnumber = value;
	    newval = newval & 0xfe000000;
	  }
	else
	  newval = value | (newval & 0xfe000000) | (hbit << 24);
	md_number_to_chars (buf, newval, INSN_SIZE);
      }
      break;

    case BFD_RELOC_THUMB_PCREL_BRANCH9: /* Conditional branch.  */
      newval = md_chars_to_number (buf, THUMB_SIZE);
      {
	addressT diff = (newval & 0xff) << 1;
	if (diff & 0x100)
	  diff |= ~0xff;

	value += diff;
	if ((value & ~0xff) && ((value & ~0xff) != ~0xff))
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("branch out of range"));
	if (seg->use_rela_p && !fixP->fx_done)
	  {
#ifdef OBJ_ELF
	    fixP->fx_offset = value;
#endif
	    fixP->fx_addnumber = value;
	    newval = newval & 0xff00;
	  }
	else
	  newval = (newval & 0xff00) | ((value & 0x1ff) >> 1);
      }
      md_number_to_chars (buf, newval, THUMB_SIZE);
      break;

    case BFD_RELOC_THUMB_PCREL_BRANCH12: /* Unconditional branch.  */
      newval = md_chars_to_number (buf, THUMB_SIZE);
      {
	addressT diff = (newval & 0x7ff) << 1;
	if (diff & 0x800)
	  diff |= ~0x7ff;

	value += diff;
	if ((value & ~0x7ff) && ((value & ~0x7ff) != ~0x7ff))
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("branch out of range"));
	if (seg->use_rela_p && !fixP->fx_done)
	  {
#ifdef OBJ_ELF
	    fixP->fx_offset = value;
#endif
	    fixP->fx_addnumber = value;
	    newval = newval & 0xf800;
	  }
	else
	  newval = (newval & 0xf800) | ((value & 0xfff) >> 1);
      }
      md_number_to_chars (buf, newval, THUMB_SIZE);
      break;

    case BFD_RELOC_THUMB_PCREL_BLX:
    case BFD_RELOC_THUMB_PCREL_BRANCH23:
      {
	offsetT newval2;
	addressT diff;

	newval  = md_chars_to_number (buf, THUMB_SIZE);
	newval2 = md_chars_to_number (buf + THUMB_SIZE, THUMB_SIZE);
	diff = ((newval & 0x7ff) << 12) | ((newval2 & 0x7ff) << 1);
	if (diff & 0x400000)
	  diff |= ~0x3fffff;
#ifdef OBJ_ELF
	value = fixP->fx_offset;
#endif
	value += diff;

	if ((value & ~0x3fffff) && ((value & ~0x3fffff) != ~0x3fffff))
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("branch with link out of range"));

	if (fixP->fx_r_type == BFD_RELOC_THUMB_PCREL_BLX)
	  /* For a BLX instruction, make sure that the relocation is rounded up
	     to a word boundary.  This follows the semantics of the instruction
	     which specifies that bit 1 of the target address will come from bit
	     1 of the base address.  */
	  value = (value + 1) & ~ 1;

	if (seg->use_rela_p && !fixP->fx_done)
	  {
#ifdef OBJ_ELF
	    fixP->fx_offset = value;
#endif
	    fixP->fx_addnumber = value;
	    newval = newval & 0xf800;
	    newval2 = newval2 & 0xf800;
	  }
	else
	  {
	    newval  = (newval  & 0xf800) | ((value & 0x7fffff) >> 12);
	    newval2 = (newval2 & 0xf800) | ((value & 0xfff) >> 1);
	  }
	md_number_to_chars (buf, newval, THUMB_SIZE);
	md_number_to_chars (buf + THUMB_SIZE, newval2, THUMB_SIZE);
      }
      break;

    case BFD_RELOC_8:
      if (seg->use_rela_p && !fixP->fx_done)
	break;
      if (fixP->fx_done || fixP->fx_pcrel)
	md_number_to_chars (buf, value, 1);
#ifdef OBJ_ELF
      else
	{
	  value = fixP->fx_offset;
	  md_number_to_chars (buf, value, 1);
	}
#endif
      break;

    case BFD_RELOC_16:
      if (seg->use_rela_p && !fixP->fx_done)
	break;
      if (fixP->fx_done || fixP->fx_pcrel)
	md_number_to_chars (buf, value, 2);
#ifdef OBJ_ELF
      else
	{
	  value = fixP->fx_offset;
	  md_number_to_chars (buf, value, 2);
	}
#endif
      break;

#ifdef OBJ_ELF
    case BFD_RELOC_ARM_GOT32:
    case BFD_RELOC_ARM_GOTOFF:
    case BFD_RELOC_ARM_TARGET2:
      if (seg->use_rela_p && !fixP->fx_done)
	break;
      md_number_to_chars (buf, 0, 4);
      break;
#endif

    case BFD_RELOC_RVA:
    case BFD_RELOC_32:
    case BFD_RELOC_ARM_TARGET1:
    case BFD_RELOC_ARM_ROSEGREL32:
    case BFD_RELOC_ARM_SBREL32:
    case BFD_RELOC_32_PCREL:
      if (seg->use_rela_p && !fixP->fx_done)
	break;
      if (fixP->fx_done || fixP->fx_pcrel)
	md_number_to_chars (buf, value, 4);
#ifdef OBJ_ELF
      else
	{
	  value = fixP->fx_offset;
	  md_number_to_chars (buf, value, 4);
	}
#endif
      break;

#ifdef OBJ_ELF
    case BFD_RELOC_ARM_PREL31:
      if (fixP->fx_done || fixP->fx_pcrel)
	{
	  newval = md_chars_to_number (buf, 4) & 0x80000000;
	  if ((value ^ (value >> 1)) & 0x40000000)
	    {
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("rel31 relocation overflow"));
	    }
	  newval |= value & 0x7fffffff;
	  md_number_to_chars (buf, newval, 4);
	}
      break;

    case BFD_RELOC_ARM_PLT32:
      /* It appears the instruction is fully prepared at this point.  */
      break;
#endif

    case BFD_RELOC_ARM_CP_OFF_IMM:
      sign = value >= 0;
      if (value < -1023 || value > 1023 || (value & 3))
	as_bad_where (fixP->fx_file, fixP->fx_line,
		      _("illegal value for co-processor offset"));
      if (value < 0)
	value = -value;
      newval = md_chars_to_number (buf, INSN_SIZE) & 0xff7fff00;
      newval |= (value >> 2) | (sign ? INDEX_UP : 0);
      md_number_to_chars (buf, newval, INSN_SIZE);
      break;

    case BFD_RELOC_ARM_CP_OFF_IMM_S2:
      sign = value >= 0;
      if (value < -255 || value > 255)
        as_bad_where (fixP->fx_file, fixP->fx_line,
                      _("Illegal value for co-processor offset"));
      if (value < 0)
        value = -value;
      newval = md_chars_to_number (buf, INSN_SIZE) & 0xff7fff00;
      newval |= value | (sign ?  INDEX_UP : 0);
      md_number_to_chars (buf, newval , INSN_SIZE);
      break;

    case BFD_RELOC_ARM_THUMB_OFFSET:
      newval = md_chars_to_number (buf, THUMB_SIZE);
      /* Exactly what ranges, and where the offset is inserted depends
	 on the type of instruction, we can establish this from the
	 top 4 bits.  */
      switch (newval >> 12)
	{
	case 4: /* PC load.  */
	  /* Thumb PC loads are somewhat odd, bit 1 of the PC is
	     forced to zero for these loads, so we will need to round
	     up the offset if the instruction address is not word
	     aligned (since the final address produced must be, and
	     we can only describe word-aligned immediate offsets).  */

	  if ((fixP->fx_frag->fr_address + fixP->fx_where + value) & 3)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid offset, target not word aligned (0x%08X)"),
			  (unsigned int) (fixP->fx_frag->fr_address
					  + fixP->fx_where + value));

	  if ((value + 2) & ~0x3fe)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid offset, value too big (0x%08lX)"),
			  (long) value);

	  /* Round up, since pc will be rounded down.  */
	  newval |= (value + 2) >> 2;
	  break;

	case 9: /* SP load/store.  */
	  if (value & ~0x3fc)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid offset, value too big (0x%08lX)"),
			  (long) value);
	  newval |= value >> 2;
	  break;

	case 6: /* Word load/store.  */
	  if (value & ~0x7c)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid offset, value too big (0x%08lX)"),
			  (long) value);
	  newval |= value << 4; /* 6 - 2.  */
	  break;

	case 7: /* Byte load/store.  */
	  if (value & ~0x1f)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid offset, value too big (0x%08lX)"),
			  (long) value);
	  newval |= value << 6;
	  break;

	case 8: /* Halfword load/store.  */
	  if (value & ~0x3e)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid offset, value too big (0x%08lX)"),
			  (long) value);
	  newval |= value << 5; /* 6 - 1.  */
	  break;

	default:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			"Unable to process relocation for thumb opcode: %lx",
			(unsigned long) newval);
	  break;
	}
      md_number_to_chars (buf, newval, THUMB_SIZE);
      break;

    case BFD_RELOC_ARM_THUMB_ADD:
      /* This is a complicated relocation, since we use it for all of
         the following immediate relocations:

	    3bit ADD/SUB
	    8bit ADD/SUB
	    9bit ADD/SUB SP word-aligned
	   10bit ADD PC/SP word-aligned

         The type of instruction being processed is encoded in the
         instruction field:

	   0x8000  SUB
	   0x00F0  Rd
	   0x000F  Rs
      */
      newval = md_chars_to_number (buf, THUMB_SIZE);
      {
	int rd = (newval >> 4) & 0xf;
	int rs = newval & 0xf;
	int subtract = newval & 0x8000;

	if (rd == REG_SP)
	  {
	    if (value & ~0x1fc)
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("invalid immediate for stack address calculation"));
	    newval = subtract ? T_OPCODE_SUB_ST : T_OPCODE_ADD_ST;
	    newval |= value >> 2;
	  }
	else if (rs == REG_PC || rs == REG_SP)
	  {
	    if (subtract ||
		value & ~0x3fc)
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("invalid immediate for address calculation (value = 0x%08lX)"),
			    (unsigned long) value);
	    newval = (rs == REG_PC ? T_OPCODE_ADD_PC : T_OPCODE_ADD_SP);
	    newval |= rd << 8;
	    newval |= value >> 2;
	  }
	else if (rs == rd)
	  {
	    if (value & ~0xff)
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("invalid 8bit immediate"));
	    newval = subtract ? T_OPCODE_SUB_I8 : T_OPCODE_ADD_I8;
	    newval |= (rd << 8) | value;
	  }
	else
	  {
	    if (value & ~0x7)
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("invalid 3bit immediate"));
	    newval = subtract ? T_OPCODE_SUB_I3 : T_OPCODE_ADD_I3;
	    newval |= rd | (rs << 3) | (value << 6);
	  }
      }
      md_number_to_chars (buf, newval, THUMB_SIZE);
      break;

    case BFD_RELOC_ARM_THUMB_IMM:
      newval = md_chars_to_number (buf, THUMB_SIZE);
      switch (newval >> 11)
	{
	case 0x04: /* 8bit immediate MOV.  */
	case 0x05: /* 8bit immediate CMP.  */
	  if (value < 0 || value > 255)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("invalid immediate: %ld is too large"),
			  (long) value);
	  newval |= value;
	  break;

	default:
	  abort ();
	}
      md_number_to_chars (buf, newval, THUMB_SIZE);
      break;

    case BFD_RELOC_ARM_THUMB_SHIFT:
      /* 5bit shift value (0..31).  */
      if (value < 0 || value > 31)
	as_bad_where (fixP->fx_file, fixP->fx_line,
		      _("illegal Thumb shift value: %ld"), (long) value);
      newval = md_chars_to_number (buf, THUMB_SIZE) & 0xf03f;
      newval |= value << 6;
      md_number_to_chars (buf, newval, THUMB_SIZE);
      break;

    case BFD_RELOC_VTABLE_INHERIT:
    case BFD_RELOC_VTABLE_ENTRY:
      fixP->fx_done = 0;
      return;

    case BFD_RELOC_UNUSED:
    default:
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("bad relocation fixup type (%d)"), fixP->fx_r_type);
    }
}

/* Translate internal representation of relocation info to BFD target
   format.  */

arelent *
tc_gen_reloc (asection * section ATTRIBUTE_UNUSED,
	      fixS *     fixp)
{
  arelent * reloc;
  bfd_reloc_code_real_type code;

  reloc = xmalloc (sizeof (arelent));

  reloc->sym_ptr_ptr = xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;

  /* @@ Why fx_addnumber sometimes and fx_offset other times?  */
#ifndef OBJ_ELF
  if (fixp->fx_pcrel == 0)
    reloc->addend = fixp->fx_offset;
  else
    reloc->addend = fixp->fx_offset = reloc->address;
#else  /* OBJ_ELF */
  reloc->addend = fixp->fx_offset;
#endif

  switch (fixp->fx_r_type)
    {
    case BFD_RELOC_8:
      if (fixp->fx_pcrel)
	{
	  code = BFD_RELOC_8_PCREL;
	  break;
	}

    case BFD_RELOC_16:
      if (fixp->fx_pcrel)
	{
	  code = BFD_RELOC_16_PCREL;
	  break;
	}

    case BFD_RELOC_32:
      if (fixp->fx_pcrel)
	{
	  code = BFD_RELOC_32_PCREL;
	  break;
	}

    case BFD_RELOC_NONE:
    case BFD_RELOC_ARM_PCREL_BRANCH:
    case BFD_RELOC_ARM_PCREL_BLX:
    case BFD_RELOC_RVA:
    case BFD_RELOC_THUMB_PCREL_BRANCH9:
    case BFD_RELOC_THUMB_PCREL_BRANCH12:
    case BFD_RELOC_THUMB_PCREL_BRANCH23:
    case BFD_RELOC_THUMB_PCREL_BLX:
    case BFD_RELOC_VTABLE_ENTRY:
    case BFD_RELOC_VTABLE_INHERIT:
      code = fixp->fx_r_type;
      break;

    case BFD_RELOC_ARM_LITERAL:
    case BFD_RELOC_ARM_HWLITERAL:
      /* If this is called then the a literal has
	 been referenced across a section boundary.  */
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("literal referenced across section boundary"));
      return NULL;

#ifdef OBJ_ELF
    case BFD_RELOC_ARM_GOT32:
    case BFD_RELOC_ARM_GOTOFF:
    case BFD_RELOC_ARM_PLT32:
    case BFD_RELOC_ARM_TARGET1:
    case BFD_RELOC_ARM_ROSEGREL32:
    case BFD_RELOC_ARM_SBREL32:
    case BFD_RELOC_ARM_PREL31:
    case BFD_RELOC_ARM_TARGET2:
      code = fixp->fx_r_type;
      break;
#endif

    case BFD_RELOC_ARM_IMMEDIATE:
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("internal relocation (type: IMMEDIATE) not fixed up"));
      return NULL;

    case BFD_RELOC_ARM_ADRL_IMMEDIATE:
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("ADRL used for a symbol not defined in the same file"));
      return NULL;

    case BFD_RELOC_ARM_OFFSET_IMM:
      if (fixp->fx_addsy != NULL
	  && !S_IS_DEFINED (fixp->fx_addsy)
	  && S_IS_LOCAL (fixp->fx_addsy))
	{
	  as_bad_where (fixp->fx_file, fixp->fx_line,
			_("undefined local label `%s'"),
			S_GET_NAME (fixp->fx_addsy));
	  return NULL;
	}

      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("internal_relocation (type: OFFSET_IMM) not fixed up"));
      return NULL;

    default:
      {
	char * type;

	switch (fixp->fx_r_type)
	  {
	  case BFD_RELOC_NONE:		   type = "NONE";         break;
	  case BFD_RELOC_ARM_OFFSET_IMM8:  type = "OFFSET_IMM8";  break;
	  case BFD_RELOC_ARM_SHIFT_IMM:    type = "SHIFT_IMM";    break;
	  case BFD_RELOC_ARM_SMI:          type = "SMI";          break;
	  case BFD_RELOC_ARM_SWI:          type = "SWI";          break;
	  case BFD_RELOC_ARM_MULTI:        type = "MULTI";        break;
	  case BFD_RELOC_ARM_CP_OFF_IMM:   type = "CP_OFF_IMM";   break;
	  case BFD_RELOC_ARM_THUMB_ADD:    type = "THUMB_ADD";    break;
	  case BFD_RELOC_ARM_THUMB_SHIFT:  type = "THUMB_SHIFT";  break;
	  case BFD_RELOC_ARM_THUMB_IMM:    type = "THUMB_IMM";    break;
	  case BFD_RELOC_ARM_THUMB_OFFSET: type = "THUMB_OFFSET"; break;
	  default:                         type = _("<unknown>"); break;
	  }
	as_bad_where (fixp->fx_file, fixp->fx_line,
		      _("cannot represent %s relocation in this object file format"),
		      type);
	return NULL;
      }
    }

#ifdef OBJ_ELF
  if ((code == BFD_RELOC_32_PCREL || code == BFD_RELOC_32)
      && GOT_symbol
      && fixp->fx_addsy == GOT_symbol)
    {
      code = BFD_RELOC_ARM_GOTPC;
      reloc->addend = fixp->fx_offset = reloc->address;
    }
#endif

  reloc->howto = bfd_reloc_type_lookup (stdoutput, code);

  if (reloc->howto == NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("cannot represent %s relocation in this object file format"),
		    bfd_get_reloc_code_name (code));
      return NULL;
    }

  /* HACK: Since arm ELF uses Rel instead of Rela, encode the
     vtable entry to be used in the relocation's section offset.  */
  if (fixp->fx_r_type == BFD_RELOC_VTABLE_ENTRY)
    reloc->address = fixp->fx_offset;

  return reloc;
}

int
md_estimate_size_before_relax (fragS * fragP ATTRIBUTE_UNUSED,
			       segT    segtype ATTRIBUTE_UNUSED)
{
  as_fatal (_("md_estimate_size_before_relax\n"));
  return 1;
}

/* We need to be able to fix up arbitrary expressions in some statements.
   This is so that we can handle symbols that are an arbitrary distance from
   the pc.  The most common cases are of the form ((+/-sym -/+ . - 8) & mask),
   which returns part of an address in a form which will be valid for
   a data instruction.  We do this by pushing the expression into a symbol
   in the expr_section, and creating a fix for that.  */

static void
fix_new_arm (fragS *       frag,
	     int           where,
	     short int     size,
	     expressionS * exp,
	     int           pc_rel,
	     int           reloc)
{
  fixS *           new_fix;
  arm_fix_data *   arm_data;

  switch (exp->X_op)
    {
    case O_constant:
    case O_symbol:
    case O_add:
    case O_subtract:
      new_fix = fix_new_exp (frag, where, size, exp, pc_rel, reloc);
      break;

    default:
      new_fix = fix_new (frag, where, size, make_expr_symbol (exp), 0,
			 pc_rel, reloc);
      break;
    }

  /* Mark whether the fix is to a THUMB instruction, or an ARM
     instruction.  */
  arm_data = obstack_alloc (& notes, sizeof (arm_fix_data));
  new_fix->tc_fix_data = (PTR) arm_data;
  arm_data->thumb_mode = thumb_mode;
}

static void
output_inst (const char * str)
{
  char * to = NULL;

  if (inst.error)
    {
      as_bad ("%s -- `%s'", inst.error, str);
      return;
    }

  to = frag_more (inst.size);

  if (thumb_mode && (inst.size > THUMB_SIZE))
    {
      assert (inst.size == (2 * THUMB_SIZE));
      md_number_to_chars (to, inst.instruction >> 16, THUMB_SIZE);
      md_number_to_chars (to + THUMB_SIZE, inst.instruction, THUMB_SIZE);
    }
  else if (inst.size > INSN_SIZE)
    {
      assert (inst.size == (2 * INSN_SIZE));
      md_number_to_chars (to, inst.instruction, INSN_SIZE);
      md_number_to_chars (to + INSN_SIZE, inst.instruction, INSN_SIZE);
    }
  else
    md_number_to_chars (to, inst.instruction, inst.size);

  if (inst.reloc.type != BFD_RELOC_UNUSED)
    fix_new_arm (frag_now, to - frag_now->fr_literal,
		 inst.size, & inst.reloc.exp, inst.reloc.pc_rel,
		 inst.reloc.type);

#ifdef OBJ_ELF
  dwarf2_emit_insn (inst.size);
#endif
}

void
md_assemble (char * str)
{
  char  c;
  char *p;
  char *start;

  /* Align the previous label if needed.  */
  if (last_label_seen != NULL)
    {
      symbol_set_frag (last_label_seen, frag_now);
      S_SET_VALUE (last_label_seen, (valueT) frag_now_fix ());
      S_SET_SEGMENT (last_label_seen, now_seg);
    }

  memset (&inst, '\0', sizeof (inst));
  inst.reloc.type = BFD_RELOC_UNUSED;

  skip_whitespace (str);

  /* Scan up to the end of the op-code, which must end in white space or
     end of string.  */
  for (start = p = str; *p != '\0'; p++)
    if (*p == ' ')
      break;

  if (p == str)
    {
      as_bad (_("no operator -- statement `%s'\n"), str);
      return;
    }

  if (thumb_mode)
    {
      const struct thumb_opcode * opcode;

      c = *p;
      *p = '\0';
      opcode = (const struct thumb_opcode *) hash_find (arm_tops_hsh, str);
      *p = c;

      if (opcode)
	{
	  /* Check that this instruction is supported for this CPU.  */
	  if (thumb_mode == 1 && (opcode->variant & cpu_variant) == 0)
	    {
	      as_bad (_("selected processor does not support `%s'"), str);
	      return;
	    }

	  mapping_state (MAP_THUMB);
	  inst.instruction = opcode->value;
	  inst.size = opcode->size;
	  opcode->parms (p);
	  output_inst (str);
	  return;
	}
    }
  else
    {
      const struct asm_opcode * opcode;

      c = *p;
      *p = '\0';
      opcode = (const struct asm_opcode *) hash_find (arm_ops_hsh, str);
      *p = c;

      if (opcode)
	{
	  /* Check that this instruction is supported for this CPU.  */
	  if ((opcode->variant & cpu_variant) == 0)
	    {
	      as_bad (_("selected processor does not support `%s'"), str);
	      return;
	    }

          mapping_state (MAP_ARM);
	  inst.instruction = opcode->value;
	  inst.size = INSN_SIZE;
	  opcode->parms (p);
	  output_inst (str);
	  return;
	}
    }

  /* It wasn't an instruction, but it might be a register alias of the form
     alias .req reg.  */
  if (create_register_alias (str, p))
    return;

  as_bad (_("bad instruction `%s'"), start);
}

/* md_parse_option
      Invocation line includes a switch not recognized by the base assembler.
      See if it's a processor-specific option.

      This routine is somewhat complicated by the need for backwards
      compatibility (since older releases of gcc can't be changed).
      The new options try to make the interface as compatible as
      possible with GCC.

      New options (supported) are:

	      -mcpu=<cpu name>		 Assemble for selected processor
	      -march=<architecture name> Assemble for selected architecture
	      -mfpu=<fpu architecture>	 Assemble for selected FPU.
	      -EB/-mbig-endian		 Big-endian
	      -EL/-mlittle-endian	 Little-endian
	      -k			 Generate PIC code
	      -mthumb			 Start in Thumb mode
	      -mthumb-interwork		 Code supports ARM/Thumb interworking

      For now we will also provide support for:

	      -mapcs-32			 32-bit Program counter
	      -mapcs-26			 26-bit Program counter
	      -macps-float		 Floats passed in FP registers
	      -mapcs-reentrant		 Reentrant code
	      -matpcs
      (sometime these will probably be replaced with -mapcs=<list of options>
      and -matpcs=<list of options>)

      The remaining options are only supported for back-wards compatibility.
      Cpu variants, the arm part is optional:
              -m[arm]1                Currently not supported.
              -m[arm]2, -m[arm]250    Arm 2 and Arm 250 processor
              -m[arm]3                Arm 3 processor
              -m[arm]6[xx],           Arm 6 processors
              -m[arm]7[xx][t][[d]m]   Arm 7 processors
              -m[arm]8[10]            Arm 8 processors
              -m[arm]9[20][tdmi]      Arm 9 processors
              -mstrongarm[110[0]]     StrongARM processors
              -mxscale                XScale processors
              -m[arm]v[2345[t[e]]]    Arm architectures
              -mall                   All (except the ARM1)
      FP variants:
              -mfpa10, -mfpa11        FPA10 and 11 co-processor instructions
              -mfpe-old               (No float load/store multiples)
	      -mvfpxd		      VFP Single precision
	      -mvfp		      All VFP
              -mno-fpu                Disable all floating point instructions

      The following CPU names are recognized:
	      arm1, arm2, arm250, arm3, arm6, arm600, arm610, arm620,
	      arm7, arm7m, arm7d, arm7dm, arm7di, arm7dmi, arm70, arm700,
	      arm700i, arm710 arm710t, arm720, arm720t, arm740t, arm710c,
	      arm7100, arm7500, arm7500fe, arm7tdmi, arm8, arm810, arm9,
	      arm920, arm920t, arm940t, arm946, arm966, arm9tdmi, arm9e,
	      arm10t arm10e, arm1020t, arm1020e, arm10200e,
	      strongarm, strongarm110, strongarm1100, strongarm1110, xscale.

      */

const char * md_shortopts = "m:k";

#ifdef ARM_BI_ENDIAN
#define OPTION_EB (OPTION_MD_BASE + 0)
#define OPTION_EL (OPTION_MD_BASE + 1)
#else
#if TARGET_BYTES_BIG_ENDIAN
#define OPTION_EB (OPTION_MD_BASE + 0)
#else
#define OPTION_EL (OPTION_MD_BASE + 1)
#endif
#endif

struct option md_longopts[] =
{
#ifdef OPTION_EB
  {"EB", no_argument, NULL, OPTION_EB},
#endif
#ifdef OPTION_EL
  {"EL", no_argument, NULL, OPTION_EL},
#endif
  {NULL, no_argument, NULL, 0}
};

size_t md_longopts_size = sizeof (md_longopts);

struct arm_option_table
{
  char *option;		/* Option name to match.  */
  char *help;		/* Help information.  */
  int  *var;		/* Variable to change.  */
  int   value;		/* What to change it to.  */
  char *deprecated;	/* If non-null, print this message.  */
};

struct arm_option_table arm_opts[] =
{
  {"k",      N_("generate PIC code"),      &pic_code,    1, NULL},
  {"mthumb", N_("assemble Thumb code"),    &thumb_mode,  1, NULL},
  {"mthumb-interwork", N_("support ARM/Thumb interworking"),
   &support_interwork, 1, NULL},
  {"mapcs-32", N_("code uses 32-bit program counter"), &uses_apcs_26, 0, NULL},
  {"mapcs-26", N_("code uses 26-bit program counter"), &uses_apcs_26, 1, NULL},
  {"mapcs-float", N_("floating point args are in fp regs"), &uses_apcs_float,
   1, NULL},
  {"mapcs-reentrant", N_("re-entrant code"), &pic_code, 1, NULL},
  {"matpcs", N_("code is ATPCS conformant"), &atpcs, 1, NULL},
  {"mbig-endian", N_("assemble for big-endian"), &target_big_endian, 1, NULL},
  {"mlittle-endian", N_("assemble for little-endian"), &target_big_endian, 0,
   NULL},

  /* These are recognized by the assembler, but have no affect on code.  */
  {"mapcs-frame", N_("use frame pointer"), NULL, 0, NULL},
  {"mapcs-stack-check", N_("use stack size checking"), NULL, 0, NULL},

  /* DON'T add any new processors to this list -- we want the whole list
     to go away...  Add them to the processors table instead.  */
  {"marm1",	 NULL, &legacy_cpu, ARM_ARCH_V1,  N_("use -mcpu=arm1")},
  {"m1",	 NULL, &legacy_cpu, ARM_ARCH_V1,  N_("use -mcpu=arm1")},
  {"marm2",	 NULL, &legacy_cpu, ARM_ARCH_V2,  N_("use -mcpu=arm2")},
  {"m2",	 NULL, &legacy_cpu, ARM_ARCH_V2,  N_("use -mcpu=arm2")},
  {"marm250",	 NULL, &legacy_cpu, ARM_ARCH_V2S, N_("use -mcpu=arm250")},
  {"m250",	 NULL, &legacy_cpu, ARM_ARCH_V2S, N_("use -mcpu=arm250")},
  {"marm3",	 NULL, &legacy_cpu, ARM_ARCH_V2S, N_("use -mcpu=arm3")},
  {"m3",	 NULL, &legacy_cpu, ARM_ARCH_V2S, N_("use -mcpu=arm3")},
  {"marm6",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm6")},
  {"m6",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm6")},
  {"marm600",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm600")},
  {"m600",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm600")},
  {"marm610",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm610")},
  {"m610",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm610")},
  {"marm620",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm620")},
  {"m620",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm620")},
  {"marm7",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7")},
  {"m7",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7")},
  {"marm70",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm70")},
  {"m70",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm70")},
  {"marm700",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm700")},
  {"m700",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm700")},
  {"marm700i",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm700i")},
  {"m700i",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm700i")},
  {"marm710",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm710")},
  {"m710",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm710")},
  {"marm710c",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm710c")},
  {"m710c",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm710c")},
  {"marm720",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm720")},
  {"m720",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm720")},
  {"marm7d",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7d")},
  {"m7d",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7d")},
  {"marm7di",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7di")},
  {"m7di",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7di")},
  {"marm7m",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -mcpu=arm7m")},
  {"m7m",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -mcpu=arm7m")},
  {"marm7dm",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -mcpu=arm7dm")},
  {"m7dm",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -mcpu=arm7dm")},
  {"marm7dmi",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -mcpu=arm7dmi")},
  {"m7dmi",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -mcpu=arm7dmi")},
  {"marm7100",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7100")},
  {"m7100",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7100")},
  {"marm7500",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7500")},
  {"m7500",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7500")},
  {"marm7500fe", NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7500fe")},
  {"m7500fe",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -mcpu=arm7500fe")},
  {"marm7t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm7tdmi")},
  {"m7t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm7tdmi")},
  {"marm7tdmi",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm7tdmi")},
  {"m7tdmi",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm7tdmi")},
  {"marm710t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm710t")},
  {"m710t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm710t")},
  {"marm720t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm720t")},
  {"m720t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm720t")},
  {"marm740t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm740t")},
  {"m740t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm740t")},
  {"marm8",	 NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -mcpu=arm8")},
  {"m8",	 NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -mcpu=arm8")},
  {"marm810",	 NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -mcpu=arm810")},
  {"m810",	 NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -mcpu=arm810")},
  {"marm9",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm9")},
  {"m9",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm9")},
  {"marm9tdmi",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm9tdmi")},
  {"m9tdmi",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm9tdmi")},
  {"marm920",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm920")},
  {"m920",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm920")},
  {"marm940",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm940")},
  {"m940",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -mcpu=arm940")},
  {"mstrongarm", NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -mcpu=strongarm")},
  {"mstrongarm110", NULL, &legacy_cpu, ARM_ARCH_V4,
   N_("use -mcpu=strongarm110")},
  {"mstrongarm1100", NULL, &legacy_cpu, ARM_ARCH_V4,
   N_("use -mcpu=strongarm1100")},
  {"mstrongarm1110", NULL, &legacy_cpu, ARM_ARCH_V4,
   N_("use -mcpu=strongarm1110")},
  {"mxscale",	 NULL, &legacy_cpu, ARM_ARCH_XSCALE, N_("use -mcpu=xscale")},
  {"miwmmxt",	 NULL, &legacy_cpu, ARM_ARCH_IWMMXT, N_("use -mcpu=iwmmxt")},
  {"mall",	 NULL, &legacy_cpu, ARM_ANY,      N_("use -mcpu=all")},

  /* Architecture variants -- don't add any more to this list either.  */
  {"mv2",	 NULL, &legacy_cpu, ARM_ARCH_V2,  N_("use -march=armv2")},
  {"marmv2",	 NULL, &legacy_cpu, ARM_ARCH_V2,  N_("use -march=armv2")},
  {"mv2a",	 NULL, &legacy_cpu, ARM_ARCH_V2S, N_("use -march=armv2a")},
  {"marmv2a",	 NULL, &legacy_cpu, ARM_ARCH_V2S, N_("use -march=armv2a")},
  {"mv3",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -march=armv3")},
  {"marmv3",	 NULL, &legacy_cpu, ARM_ARCH_V3,  N_("use -march=armv3")},
  {"mv3m",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -march=armv3m")},
  {"marmv3m",	 NULL, &legacy_cpu, ARM_ARCH_V3M, N_("use -march=armv3m")},
  {"mv4",	 NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -march=armv4")},
  {"marmv4",	 NULL, &legacy_cpu, ARM_ARCH_V4,  N_("use -march=armv4")},
  {"mv4t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -march=armv4t")},
  {"marmv4t",	 NULL, &legacy_cpu, ARM_ARCH_V4T, N_("use -march=armv4t")},
  {"mv5",	 NULL, &legacy_cpu, ARM_ARCH_V5,  N_("use -march=armv5")},
  {"marmv5",	 NULL, &legacy_cpu, ARM_ARCH_V5,  N_("use -march=armv5")},
  {"mv5t",	 NULL, &legacy_cpu, ARM_ARCH_V5T, N_("use -march=armv5t")},
  {"marmv5t",	 NULL, &legacy_cpu, ARM_ARCH_V5T, N_("use -march=armv5t")},
  {"mv5e",	 NULL, &legacy_cpu, ARM_ARCH_V5TE, N_("use -march=armv5te")},
  {"marmv5e",	 NULL, &legacy_cpu, ARM_ARCH_V5TE, N_("use -march=armv5te")},

  /* Floating point variants -- don't add any more to this list either.  */
  {"mfpe-old", NULL, &legacy_fpu, FPU_ARCH_FPE, N_("use -mfpu=fpe")},
  {"mfpa10",   NULL, &legacy_fpu, FPU_ARCH_FPA, N_("use -mfpu=fpa10")},
  {"mfpa11",   NULL, &legacy_fpu, FPU_ARCH_FPA, N_("use -mfpu=fpa11")},
  {"mno-fpu",  NULL, &legacy_fpu, 0,
   N_("use either -mfpu=softfpa or -mfpu=softvfp")},

  {NULL, NULL, NULL, 0, NULL}
};

struct arm_cpu_option_table
{
  char *name;
  int   value;
  /* For some CPUs we assume an FPU unless the user explicitly sets
     -mfpu=...  */
  int   default_fpu;
};

/* This list should, at a minimum, contain all the cpu names
   recognized by GCC.  */
static struct arm_cpu_option_table arm_cpus[] =
{
  {"all",		ARM_ANY,	 FPU_ARCH_FPA},
  {"arm1",		ARM_ARCH_V1,	 FPU_ARCH_FPA},
  {"arm2",		ARM_ARCH_V2,	 FPU_ARCH_FPA},
  {"arm250",		ARM_ARCH_V2S,	 FPU_ARCH_FPA},
  {"arm3",		ARM_ARCH_V2S,	 FPU_ARCH_FPA},
  {"arm6",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm60",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm600",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm610",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm620",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7m",		ARM_ARCH_V3M,	 FPU_ARCH_FPA},
  {"arm7d",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7dm",		ARM_ARCH_V3M,	 FPU_ARCH_FPA},
  {"arm7di",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7dmi",		ARM_ARCH_V3M,	 FPU_ARCH_FPA},
  {"arm70",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm700",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm700i",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm710",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm710t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm720",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm720t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm740t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm710c",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7100",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7500",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7500fe",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"arm7t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm7tdmi",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm7tdmi-s",	ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm8",		ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"arm810",		ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"strongarm",		ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"strongarm1",	ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"strongarm110",	ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"strongarm1100",	ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"strongarm1110",	ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"arm9",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm920",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm920t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm922t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm940t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"arm9tdmi",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  /* For V5 or later processors we default to using VFP; but the user
     should really set the FPU type explicitly.  */
  {"arm9e-r0",		ARM_ARCH_V5TExP, FPU_ARCH_VFP_V2},
  {"arm9e",		ARM_ARCH_V5TE,   FPU_ARCH_VFP_V2},
  {"arm926ej",		ARM_ARCH_V5TEJ,	 FPU_ARCH_VFP_V2},
  {"arm926ejs",		ARM_ARCH_V5TEJ,	 FPU_ARCH_VFP_V2},
  {"arm926ej-s",	ARM_ARCH_V5TEJ,	 FPU_ARCH_VFP_V2},
  {"arm946e-r0",	ARM_ARCH_V5TExP, FPU_ARCH_VFP_V2},
  {"arm946e",		ARM_ARCH_V5TE,   FPU_ARCH_VFP_V2},
  {"arm966e-r0",	ARM_ARCH_V5TExP, FPU_ARCH_VFP_V2},
  {"arm966e",		ARM_ARCH_V5TE,	 FPU_ARCH_VFP_V2},
  {"arm10t",		ARM_ARCH_V5T,	 FPU_ARCH_VFP_V1},
  {"arm10e",		ARM_ARCH_V5TE,	 FPU_ARCH_VFP_V2},
  {"arm1020",		ARM_ARCH_V5TE,	 FPU_ARCH_VFP_V2},
  {"arm1020t",		ARM_ARCH_V5T,	 FPU_ARCH_VFP_V1},
  {"arm1020e",		ARM_ARCH_V5TE,	 FPU_ARCH_VFP_V2},
  {"arm1026ejs",	ARM_ARCH_V5TEJ,	 FPU_ARCH_VFP_V2},
  {"arm1026ej-s",	ARM_ARCH_V5TEJ,	 FPU_ARCH_VFP_V2},
  {"arm1136js",		ARM_ARCH_V6,     FPU_NONE},
  {"arm1136j-s",	ARM_ARCH_V6,     FPU_NONE},
  {"arm1136jfs",	ARM_ARCH_V6,     FPU_ARCH_VFP_V2},
  {"arm1136jf-s",	ARM_ARCH_V6,     FPU_ARCH_VFP_V2},
  {"mpcore",		ARM_ARCH_V6K,    FPU_ARCH_VFP_V2},
  {"mpcorenovfp",	ARM_ARCH_V6K,    FPU_NONE},
  {"arm1176jz-s",	ARM_ARCH_V6ZK,   FPU_NONE},
  {"arm1176jzf-s",	ARM_ARCH_V6ZK,   FPU_ARCH_VFP_V2},
  {"cortex-a8",		ARM_ARCH_V7A,    FPU_ARCH_VFP_V2},
  {"cortex-a9",		ARM_ARCH_V7A,    FPU_ARCH_VFP_V2},
  /* ??? XSCALE is really an architecture.  */
  {"xscale",		ARM_ARCH_XSCALE, FPU_ARCH_VFP_V2},
  /* ??? iwmmxt is not a processor.  */
  {"iwmmxt",		ARM_ARCH_IWMMXT, FPU_ARCH_VFP_V2},
  {"i80200",		ARM_ARCH_XSCALE, FPU_ARCH_VFP_V2},
  /* Maverick */
  {"ep9312",		ARM_ARCH_V4T | ARM_CEXT_MAVERICK, FPU_ARCH_MAVERICK},
  {NULL, 0, 0}
};

struct arm_arch_option_table
{
  char *name;
  int   value;
  int   default_fpu;
};

/* This list should, at a minimum, contain all the architecture names
   recognized by GCC.  */
static struct arm_arch_option_table arm_archs[] =
{
  {"all",		ARM_ANY,	 FPU_ARCH_FPA},
  {"armv1",		ARM_ARCH_V1,	 FPU_ARCH_FPA},
  {"armv2",		ARM_ARCH_V2,	 FPU_ARCH_FPA},
  {"armv2a",		ARM_ARCH_V2S,	 FPU_ARCH_FPA},
  {"armv2s",		ARM_ARCH_V2S,	 FPU_ARCH_FPA},
  {"armv3",		ARM_ARCH_V3,	 FPU_ARCH_FPA},
  {"armv3m",		ARM_ARCH_V3M,	 FPU_ARCH_FPA},
  {"armv4",		ARM_ARCH_V4,	 FPU_ARCH_FPA},
  {"armv4xm",		ARM_ARCH_V4xM,	 FPU_ARCH_FPA},
  {"armv4t",		ARM_ARCH_V4T,	 FPU_ARCH_FPA},
  {"armv4txm",		ARM_ARCH_V4TxM,	 FPU_ARCH_FPA},
  {"armv5",		ARM_ARCH_V5,	 FPU_ARCH_VFP},
  {"armv5t",		ARM_ARCH_V5T,	 FPU_ARCH_VFP},
  {"armv5txm",		ARM_ARCH_V5TxM,	 FPU_ARCH_VFP},
  {"armv5te",		ARM_ARCH_V5TE,	 FPU_ARCH_VFP},
  {"armv5texp",		ARM_ARCH_V5TExP, FPU_ARCH_VFP},
  {"armv5tej",		ARM_ARCH_V5TEJ,  FPU_ARCH_VFP},
  {"armv6",             ARM_ARCH_V6,     FPU_ARCH_VFP},
  {"armv6j",            ARM_ARCH_V6,     FPU_ARCH_VFP},
  {"armv6k",            ARM_ARCH_V6K,    FPU_ARCH_VFP},
  {"armv6z",            ARM_ARCH_V6Z,    FPU_ARCH_VFP},
  {"armv6zk",           ARM_ARCH_V6ZK,   FPU_ARCH_VFP},
  {"armv7",             ARM_ARCH_V7A,    FPU_ARCH_VFP},
  {"armv7a",            ARM_ARCH_V7A,    FPU_ARCH_VFP},
  {"xscale",		ARM_ARCH_XSCALE, FPU_ARCH_VFP},
  {"iwmmxt",		ARM_ARCH_IWMMXT, FPU_ARCH_VFP},
  {NULL, 0, 0}
};

/* ISA extensions in the co-processor space.  */
struct arm_arch_extension_table
{
  char *name;
  int value;
};

static struct arm_arch_extension_table arm_extensions[] =
{
  {"maverick",		ARM_CEXT_MAVERICK},
  {"xscale",		ARM_CEXT_XSCALE},
  {"iwmmxt",		ARM_CEXT_IWMMXT},
  {NULL,		0}
};

struct arm_fpu_option_table
{
  char *name;
  int   value;
};

/* This list should, at a minimum, contain all the fpu names
   recognized by GCC.  */
static struct arm_fpu_option_table arm_fpus[] =
{
  {"softfpa",		FPU_NONE},
  {"fpe",		FPU_ARCH_FPE},
  {"fpe2",		FPU_ARCH_FPE},
  {"fpe3",		FPU_ARCH_FPA},	/* Third release supports LFM/SFM.  */
  {"fpa",		FPU_ARCH_FPA},
  {"fpa10",		FPU_ARCH_FPA},
  {"fpa11",		FPU_ARCH_FPA},
  {"arm7500fe",		FPU_ARCH_FPA},
  {"softvfp",		FPU_ARCH_VFP},
  {"softvfp+vfp",	FPU_ARCH_VFP_V2},
  {"vfp",		FPU_ARCH_VFP_V2},
  {"vfp9",		FPU_ARCH_VFP_V2},
  {"vfp10",		FPU_ARCH_VFP_V2},
  {"vfp10-r0",		FPU_ARCH_VFP_V1},
  {"vfpxd",		FPU_ARCH_VFP_V1xD},
  {"arm1020t",		FPU_ARCH_VFP_V1},
  {"arm1020e",		FPU_ARCH_VFP_V2},
  {"arm1136jfs",	FPU_ARCH_VFP_V2},
  {"arm1136jf-s",	FPU_ARCH_VFP_V2},
  {"maverick",		FPU_ARCH_MAVERICK},
  {NULL, 0}
};

struct arm_float_abi_option_table
{
  char *name;
  int value;
};

static struct arm_float_abi_option_table arm_float_abis[] =
{
  {"hard",	ARM_FLOAT_ABI_HARD},
  {"softfp",	ARM_FLOAT_ABI_SOFTFP},
  {"soft",	ARM_FLOAT_ABI_SOFT},
  {NULL, 0}
};

struct arm_eabi_option_table
{
  char *name;
  unsigned int value;
};

#ifdef OBJ_ELF
/* We only know how to output GNU and ver 4 (AAELF) formats.  */
static struct arm_eabi_option_table arm_eabis[] =
{
  {"gnu",	EF_ARM_EABI_UNKNOWN},
  {"4",		EF_ARM_EABI_VER4},
  {NULL, 0}
};
#endif

struct arm_long_option_table
{
  char * option;		/* Substring to match.  */
  char * help;			/* Help information.  */
  int (* func) (char * subopt);	/* Function to decode sub-option.  */
  char * deprecated;		/* If non-null, print this message.  */
};

static int
arm_parse_extension (char * str, int * opt_p)
{
  while (str != NULL && *str != 0)
    {
      struct arm_arch_extension_table * opt;
      char * ext;
      int optlen;

      if (*str != '+')
	{
	  as_bad (_("invalid architectural extension"));
	  return 0;
	}

      str++;
      ext = strchr (str, '+');

      if (ext != NULL)
	optlen = ext - str;
      else
	optlen = strlen (str);

      if (optlen == 0)
	{
	  as_bad (_("missing architectural extension"));
	  return 0;
	}

      for (opt = arm_extensions; opt->name != NULL; opt++)
	if (strncmp (opt->name, str, optlen) == 0)
	  {
	    *opt_p |= opt->value;
	    break;
	  }

      if (opt->name == NULL)
	{
	  as_bad (_("unknown architectural extnsion `%s'"), str);
	  return 0;
	}

      str = ext;
    };

  return 1;
}

static int
arm_parse_cpu (char * str)
{
  struct arm_cpu_option_table * opt;
  char * ext = strchr (str, '+');
  int optlen;

  if (ext != NULL)
    optlen = ext - str;
  else
    optlen = strlen (str);

  if (optlen == 0)
    {
      as_bad (_("missing cpu name `%s'"), str);
      return 0;
    }

  for (opt = arm_cpus; opt->name != NULL; opt++)
    if (strncmp (opt->name, str, optlen) == 0)
      {
	mcpu_cpu_opt = opt->value;
	mcpu_fpu_opt = opt->default_fpu;

	if (ext != NULL)
	  return arm_parse_extension (ext, &mcpu_cpu_opt);

	return 1;
      }

  as_bad (_("unknown cpu `%s'"), str);
  return 0;
}

static int
arm_parse_arch (char * str)
{
  struct arm_arch_option_table *opt;
  char *ext = strchr (str, '+');
  int optlen;

  if (ext != NULL)
    optlen = ext - str;
  else
    optlen = strlen (str);

  if (optlen == 0)
    {
      as_bad (_("missing architecture name `%s'"), str);
      return 0;
    }


  for (opt = arm_archs; opt->name != NULL; opt++)
    if (streq (opt->name, str))
      {
	march_cpu_opt = opt->value;
	march_fpu_opt = opt->default_fpu;

	if (ext != NULL)
	  return arm_parse_extension (ext, &march_cpu_opt);

	return 1;
      }

  as_bad (_("unknown architecture `%s'\n"), str);
  return 0;
}

static void
s_arch (int ignore ATTRIBUTE_UNUSED)
{
  char *name, *p, c;
  SKIP_WHITESPACE ();
  name = input_line_pointer;
  c = get_symbol_end ();
  p = input_line_pointer;
  arm_parse_arch (name);
  *p = c;
  SKIP_WHITESPACE ();
  demand_empty_rest_of_line ();
}

static int
arm_parse_fpu (char * str)
{
  struct arm_fpu_option_table * opt;

  for (opt = arm_fpus; opt->name != NULL; opt++)
    if (streq (opt->name, str))
      {
	mfpu_opt = opt->value;
	return 1;
      }

  as_bad (_("unknown floating point format `%s'\n"), str);
  return 0;
}

static int
arm_parse_float_abi (char * str)
{
  struct arm_float_abi_option_table * opt;

  for (opt = arm_float_abis; opt->name != NULL; opt++)
    if (streq (opt->name, str))
      {
	mfloat_abi_opt = opt->value;
	return 1;
      }

  as_bad (_("unknown floating point abi `%s'\n"), str);
  return 0;
}

#ifdef OBJ_ELF
static int
arm_parse_eabi (char * str)
{
  struct arm_eabi_option_table *opt;

  for (opt = arm_eabis; opt->name != NULL; opt++)
    if (streq (opt->name, str))
      {
	meabi_flags = opt->value;
	return 1;
      }
  as_bad (_("unknown EABI `%s'\n"), str);
  return 0;
}
#endif

struct arm_long_option_table arm_long_opts[] =
{
  {"mcpu=", N_("<cpu name>\t  assemble for CPU <cpu name>"),
   arm_parse_cpu, NULL},
  {"march=", N_("<arch name>\t  assemble for architecture <arch name>"),
   arm_parse_arch, NULL},
  {"mfpu=", N_("<fpu name>\t  assemble for FPU architecture <fpu name>"),
   arm_parse_fpu, NULL},
  {"mfloat-abi=", N_("<abi>\t  assemble for floating point ABI <abi>"),
   arm_parse_float_abi, NULL},
#ifdef OBJ_ELF
  {"meabi=", N_("<ver>\t  assemble for eabi version <ver>"),
   arm_parse_eabi, NULL},
#endif
  {NULL, NULL, 0, NULL}
};

int
md_parse_option (int c, char * arg)
{
  struct arm_option_table *opt;
  struct arm_long_option_table *lopt;

  switch (c)
    {
#ifdef OPTION_EB
    case OPTION_EB:
      target_big_endian = 1;
      break;
#endif

#ifdef OPTION_EL
    case OPTION_EL:
      target_big_endian = 0;
      break;
#endif

    case 'a':
      /* Listing option.  Just ignore these, we don't support additional
	 ones.  */
      return 0;

    default:
      for (opt = arm_opts; opt->option != NULL; opt++)
	{
	  if (c == opt->option[0]
	      && ((arg == NULL && opt->option[1] == 0)
		  || streq (arg, opt->option + 1)))
	    {
#if WARN_DEPRECATED
	      /* If the option is deprecated, tell the user.  */
	      if (opt->deprecated != NULL)
		as_tsktsk (_("option `-%c%s' is deprecated: %s"), c,
			   arg ? arg : "", _(opt->deprecated));
#endif

	      if (opt->var != NULL)
		*opt->var = opt->value;

	      return 1;
	    }
	}

      for (lopt = arm_long_opts; lopt->option != NULL; lopt++)
	{
	  /* These options are expected to have an argument.  */
	  if (c == lopt->option[0]
	      && arg != NULL
	      && strncmp (arg, lopt->option + 1,
			  strlen (lopt->option + 1)) == 0)
	    {
#if WARN_DEPRECATED
	      /* If the option is deprecated, tell the user.  */
	      if (lopt->deprecated != NULL)
		as_tsktsk (_("option `-%c%s' is deprecated: %s"), c, arg,
			   _(lopt->deprecated));
#endif

	      /* Call the sup-option parser.  */
	      return lopt->func (arg + strlen (lopt->option) - 1);
	    }
	}

      return 0;
    }

  return 1;
}

void
md_show_usage (FILE * fp)
{
  struct arm_option_table *opt;
  struct arm_long_option_table *lopt;

  fprintf (fp, _(" ARM-specific assembler options:\n"));

  for (opt = arm_opts; opt->option != NULL; opt++)
    if (opt->help != NULL)
      fprintf (fp, "  -%-23s%s\n", opt->option, _(opt->help));

  for (lopt = arm_long_opts; lopt->option != NULL; lopt++)
    if (lopt->help != NULL)
      fprintf (fp, "  -%s%s\n", lopt->option, _(lopt->help));

#ifdef OPTION_EB
  fprintf (fp, _("\
  -EB                     assemble code for a big-endian cpu\n"));
#endif

#ifdef OPTION_EL
  fprintf (fp, _("\
  -EL                     assemble code for a little-endian cpu\n"));
#endif
}

/* This fix_new is called by cons via TC_CONS_FIX_NEW.  */

void
cons_fix_new_arm (fragS *       frag,
		  int           where,
		  int           size,
		  expressionS * exp)
{
  bfd_reloc_code_real_type type;
  int pcrel = 0;

  /* Pick a reloc.
     FIXME: @@ Should look at CPU word size.  */
  switch (size)
    {
    case 1:
      type = BFD_RELOC_8;
      break;
    case 2:
      type = BFD_RELOC_16;
      break;
    case 4:
    default:
      type = BFD_RELOC_32;
      break;
    case 8:
      type = BFD_RELOC_64;
      break;
    }

  fix_new_exp (frag, where, (int) size, exp, pcrel, type);
}

/* A good place to do this, although this was probably not intended
   for this kind of use.  We need to dump the literal pool before
   references are made to a null symbol pointer.  */

void
arm_cleanup (void)
{
  literal_pool * pool;

  for (pool = list_of_pools; pool; pool = pool->next)
    {
      /* Put it at the end of the relevent section.  */
      subseg_set (pool->section, pool->sub_section);
#ifdef OBJ_ELF
      arm_elf_change_section ();
#endif
      s_ltorg (0);
    }
}

void
arm_start_line_hook (void)
{
  last_label_seen = NULL;
}

void
arm_frob_label (symbolS * sym)
{
  last_label_seen = sym;

  ARM_SET_THUMB (sym, thumb_mode);

#if defined OBJ_COFF || defined OBJ_ELF
  ARM_SET_INTERWORK (sym, support_interwork);
#endif

  /* Note - do not allow local symbols (.Lxxx) to be labeled
     as Thumb functions.  This is because these labels, whilst
     they exist inside Thumb code, are not the entry points for
     possible ARM->Thumb calls.  Also, these labels can be used
     as part of a computed goto or switch statement.  eg gcc
     can generate code that looks like this:

                ldr  r2, [pc, .Laaa]
                lsl  r3, r3, #2
                ldr  r2, [r3, r2]
                mov  pc, r2

       .Lbbb:  .word .Lxxx
       .Lccc:  .word .Lyyy
       ..etc...
       .Laaa:   .word Lbbb

     The first instruction loads the address of the jump table.
     The second instruction converts a table index into a byte offset.
     The third instruction gets the jump address out of the table.
     The fourth instruction performs the jump.

     If the address stored at .Laaa is that of a symbol which has the
     Thumb_Func bit set, then the linker will arrange for this address
     to have the bottom bit set, which in turn would mean that the
     address computation performed by the third instruction would end
     up with the bottom bit set.  Since the ARM is capable of unaligned
     word loads, the instruction would then load the incorrect address
     out of the jump table, and chaos would ensue.  */
  if (label_is_thumb_function_name
      && (S_GET_NAME (sym)[0] != '.' || S_GET_NAME (sym)[1] != 'L')
      && (bfd_get_section_flags (stdoutput, now_seg) & SEC_CODE) != 0)
    {
      /* When the address of a Thumb function is taken the bottom
	 bit of that address should be set.  This will allow
	 interworking between Arm and Thumb functions to work
	 correctly.  */

      THUMB_SET_FUNC (sym, 1);

      label_is_thumb_function_name = FALSE;
    }
}

/* Adjust the symbol table.  This marks Thumb symbols as distinct from
   ARM ones.  */

void
arm_adjust_symtab (void)
{
#ifdef OBJ_COFF
  symbolS * sym;

  for (sym = symbol_rootP; sym != NULL; sym = symbol_next (sym))
    {
      if (ARM_IS_THUMB (sym))
	{
	  if (THUMB_IS_FUNC (sym))
	    {
	      /* Mark the symbol as a Thumb function.  */
	      if (   S_GET_STORAGE_CLASS (sym) == C_STAT
		  || S_GET_STORAGE_CLASS (sym) == C_LABEL)  /* This can happen!  */
		S_SET_STORAGE_CLASS (sym, C_THUMBSTATFUNC);

	      else if (S_GET_STORAGE_CLASS (sym) == C_EXT)
		S_SET_STORAGE_CLASS (sym, C_THUMBEXTFUNC);
	      else
		as_bad (_("%s: unexpected function type: %d"),
			S_GET_NAME (sym), S_GET_STORAGE_CLASS (sym));
	    }
	  else switch (S_GET_STORAGE_CLASS (sym))
	    {
	    case C_EXT:
	      S_SET_STORAGE_CLASS (sym, C_THUMBEXT);
	      break;
	    case C_STAT:
	      S_SET_STORAGE_CLASS (sym, C_THUMBSTAT);
	      break;
	    case C_LABEL:
	      S_SET_STORAGE_CLASS (sym, C_THUMBLABEL);
	      break;
	    default:
	      /* Do nothing.  */
	      break;
	    }
	}

      if (ARM_IS_INTERWORK (sym))
	coffsymbol (symbol_get_bfdsym (sym))->native->u.syment.n_flags = 0xFF;
    }
#endif
#ifdef OBJ_ELF
  symbolS * sym;
  char      bind;

  for (sym = symbol_rootP; sym != NULL; sym = symbol_next (sym))
    {
      if (ARM_IS_THUMB (sym))
	{
	  elf_symbol_type * elf_sym;

	  elf_sym = elf_symbol (symbol_get_bfdsym (sym));
	  bind = ELF_ST_BIND (elf_sym);

	  /* If it's a .thumb_func, declare it as so,
	     otherwise tag label as .code 16.  */
	  if (THUMB_IS_FUNC (sym))
	    elf_sym->internal_elf_sym.st_info =
	      ELF_ST_INFO (bind, STT_ARM_TFUNC);
	  else
	    elf_sym->internal_elf_sym.st_info =
	      ELF_ST_INFO (bind, STT_ARM_16BIT);
	}
    }
#endif
}

int
arm_data_in_code (void)
{
  if (thumb_mode && ! strncmp (input_line_pointer + 1, "data:", 5))
    {
      *input_line_pointer = '/';
      input_line_pointer += 5;
      *input_line_pointer = 0;
      return 1;
    }

  return 0;
}

char *
arm_canonicalize_symbol_name (char * name)
{
  int len;

  if (thumb_mode && (len = strlen (name)) > 5
      && streq (name + len - 5, "/data"))
    *(name + len - 5) = 0;

  return name;
}

#if defined OBJ_COFF || defined OBJ_ELF
void
arm_validate_fix (fixS * fixP)
{
  /* If the destination of the branch is a defined symbol which does not have
     the THUMB_FUNC attribute, then we must be calling a function which has
     the (interfacearm) attribute.  We look for the Thumb entry point to that
     function and change the branch to refer to that function instead.  */
  if (fixP->fx_r_type == BFD_RELOC_THUMB_PCREL_BRANCH23
      && fixP->fx_addsy != NULL
      && S_IS_DEFINED (fixP->fx_addsy)
      && ! THUMB_IS_FUNC (fixP->fx_addsy))
    {
      fixP->fx_addsy = find_real_start (fixP->fx_addsy);
    }
}
#endif

int
arm_force_relocation (struct fix * fixp)
{
#if defined (OBJ_COFF) && defined (TE_PE)
  if (fixp->fx_r_type == BFD_RELOC_RVA)
    return 1;
#endif
#ifdef OBJ_ELF
  if (fixp->fx_r_type == BFD_RELOC_ARM_PCREL_BRANCH
      || fixp->fx_r_type == BFD_RELOC_ARM_PCREL_BLX
      || fixp->fx_r_type == BFD_RELOC_THUMB_PCREL_BLX
      || fixp->fx_r_type == BFD_RELOC_THUMB_PCREL_BRANCH23)
    return 1;
#endif

  /* Resolve these relocations even if the symbol is extern or weak.  */
  if (fixp->fx_r_type == BFD_RELOC_ARM_IMMEDIATE
      || fixp->fx_r_type == BFD_RELOC_ARM_OFFSET_IMM
      || fixp->fx_r_type == BFD_RELOC_ARM_ADRL_IMMEDIATE)
    return 0;

  return generic_force_reloc (fixp);
}

#ifdef OBJ_COFF
/* This is a little hack to help the gas/arm/adrl.s test.  It prevents
   local labels from being added to the output symbol table when they
   are used with the ADRL pseudo op.  The ADRL relocation should always
   be resolved before the binbary is emitted, so it is safe to say that
   it is adjustable.  */

bfd_boolean
arm_fix_adjustable (fixS * fixP)
{
  if (fixP->fx_r_type == BFD_RELOC_ARM_ADRL_IMMEDIATE)
    return 1;
  return 0;
}
#endif

#ifdef OBJ_ELF
/* Relocations against Thumb function names must be left unadjusted,
   so that the linker can use this information to correctly set the
   bottom bit of their addresses.  The MIPS version of this function
   also prevents relocations that are mips-16 specific, but I do not
   know why it does this.

   FIXME:
   There is one other problem that ought to be addressed here, but
   which currently is not:  Taking the address of a label (rather
   than a function) and then later jumping to that address.  Such
   addresses also ought to have their bottom bit set (assuming that
   they reside in Thumb code), but at the moment they will not.  */

bfd_boolean
arm_fix_adjustable (fixS * fixP)
{
  if (fixP->fx_addsy == NULL)
    return 1;

  if (THUMB_IS_FUNC (fixP->fx_addsy)
      && fixP->fx_subsy == NULL)
    return 0;

  /* We need the symbol name for the VTABLE entries.  */
  if (   fixP->fx_r_type == BFD_RELOC_VTABLE_INHERIT
      || fixP->fx_r_type == BFD_RELOC_VTABLE_ENTRY)
    return 0;

  /* Don't allow symbols to be discarded on GOT related relocs.  */
  if (fixP->fx_r_type == BFD_RELOC_ARM_PLT32
      || fixP->fx_r_type == BFD_RELOC_ARM_GOT32
      || fixP->fx_r_type == BFD_RELOC_ARM_GOTOFF
      || fixP->fx_r_type == BFD_RELOC_ARM_TARGET2)
    return 0;

  return 1;
}

const char *
elf32_arm_target_format (void)
{
#ifdef TE_SYMBIAN
  return (target_big_endian
	  ? "elf32-bigarm-symbian"
	  : "elf32-littlearm-symbian");
#elif defined (TE_VXWORKS)
  return (target_big_endian
	  ? "elf32-bigarm-vxworks"
	  : "elf32-littlearm-vxworks");
#else
  if (target_big_endian)
    return "elf32-bigarm";
  else
    return "elf32-littlearm";
#endif
}

void
armelf_frob_symbol (symbolS * symp,
		    int *     puntp)
{
  elf_frob_symbol (symp, puntp);
}

static void
s_arm_elf_cons (int nbytes)
{
  expressionS exp;

#ifdef md_flush_pending_output
  md_flush_pending_output ();
#endif

  if (is_it_end_of_statement ())
    {
      demand_empty_rest_of_line ();
      return;
    }

#ifdef md_cons_align
  md_cons_align (nbytes);
#endif

  mapping_state (MAP_DATA);
  do
    {
      bfd_reloc_code_real_type reloc;

      expression (& exp);

      if (exp.X_op == O_symbol
	  && * input_line_pointer == '('
	  && (reloc = arm_parse_reloc ()) != BFD_RELOC_UNUSED)
	{
	  reloc_howto_type *howto = bfd_reloc_type_lookup (stdoutput, reloc);
	  int size = bfd_get_reloc_size (howto);

	  if (size > nbytes)
	    as_bad ("%s relocations do not fit in %d bytes",
		    howto->name, nbytes);
	  else
	    {
	      char *p = frag_more ((int) nbytes);
	      int offset = nbytes - size;

	      fix_new_exp (frag_now, p - frag_now->fr_literal + offset, size,
			   &exp, 0, reloc);
	    }
	}
      else
	emit_expr (&exp, (unsigned int) nbytes);
    }
  while (*input_line_pointer++ == ',');

  /* Put terminator back into stream.  */
  input_line_pointer --;
  demand_empty_rest_of_line ();
}


/* Parse a .rel31 directive.  */

static void
s_arm_rel31 (int ignored ATTRIBUTE_UNUSED)
{
  expressionS exp;
  char *p;
  valueT highbit;

  SKIP_WHITESPACE ();

  highbit = 0;
  if (*input_line_pointer == '1')
    highbit = 0x80000000;
  else if (*input_line_pointer != '0')
    as_bad (_("expected 0 or 1"));

  input_line_pointer++;
  SKIP_WHITESPACE ();
  if (*input_line_pointer != ',')
    as_bad (_("missing comma"));
  input_line_pointer++;

#ifdef md_flush_pending_output
  md_flush_pending_output ();
#endif

#ifdef md_cons_align
  md_cons_align (4);
#endif

  mapping_state (MAP_DATA);

  expression (&exp);

  p = frag_more (4);
  md_number_to_chars (p, highbit, 4);
  fix_new_arm (frag_now, p - frag_now->fr_literal, 4, &exp, 1,
	       BFD_RELOC_ARM_PREL31);

  demand_empty_rest_of_line ();
}

/* Code to deal with unwinding tables.  */

static void add_unwind_adjustsp (offsetT);

/* Switch to section NAME and create section if necessary.  It's
   rather ugly that we have to manipulate input_line_pointer but I
   don't see any other way to accomplish the same thing without
   changing obj-elf.c (which may be the Right Thing, in the end).
   Copied from tc-ia64.c.  */

static void
set_section (char *name)
{
  char *saved_input_line_pointer;

  saved_input_line_pointer = input_line_pointer;
  input_line_pointer = name;
  obj_elf_section (0);
  input_line_pointer = saved_input_line_pointer;
}

/* Cenerate and deferred unwind frame offset.  */

static void
flush_pending_unwind (void)
{
  offsetT offset;

  offset = unwind.pending_offset;
  unwind.pending_offset = 0;
  if (offset != 0)
    add_unwind_adjustsp (offset);
}

/* Add an opcode to this list for this function.  Two-byte opcodes should
   be passed as op[0] << 8 | op[1].  The list of opcodes is built in reverse
   order.  */

static void
add_unwind_opcode (valueT op, int length)
{
  /* Add any deferred stack adjustment.  */
  if (unwind.pending_offset)
    flush_pending_unwind ();

  unwind.sp_restored = 0;

  if (unwind.opcode_count + length > unwind.opcode_alloc)
    {
      unwind.opcode_alloc += ARM_OPCODE_CHUNK_SIZE;
      if (unwind.opcodes)
	unwind.opcodes = xrealloc (unwind.opcodes,
				   unwind.opcode_alloc);
      else
	unwind.opcodes = xmalloc (unwind.opcode_alloc);
    }
  while (length > 0)
    {
      length--;
      unwind.opcodes[unwind.opcode_count] = op & 0xff;
      op >>= 8;
      unwind.opcode_count++;
    }
}

/* Add unwind opcodes to adjust the stack pointer.  */

static void
add_unwind_adjustsp (offsetT offset)
{
  valueT op;

  if (offset > 0x200)
    {
      /* We need at most 5 bytes to hold a 32-bit value in a uleb128.  */
      char bytes[5];
      int n;
      valueT o;

      /* Long form: 0xb2, uleb128.  */
      /* This might not fit in a word so add the individual bytes,
	 remembering the list is built in reverse order.  */
      o = (valueT) ((offset - 0x204) >> 2);
      if (o == 0)
	add_unwind_opcode (0, 1);

      /* Calculate the uleb128 encoding of the offset.  */
      n = 0;
      while (o)
	{
	  bytes[n] = o & 0x7f;
	  o >>= 7;
	  if (o)
	    bytes[n] |= 0x80;
	  n++;
	}
      /* Add the insn.  */
      for (; n; n--)
	add_unwind_opcode (bytes[n - 1], 1);
      add_unwind_opcode (0xb2, 1);
    }
  else if (offset > 0x100)
    {
      /* Two short opcodes.  */
      add_unwind_opcode (0x3f, 1);
      op = (offset - 0x104) >> 2;
      add_unwind_opcode (op, 1);
    }
  else if (offset > 0)
    {
      /* Short opcode.  */
      op = (offset - 4) >> 2;
      add_unwind_opcode (op, 1);
    }
  else if (offset < 0)
    {
      offset = -offset;
      while (offset > 0x100)
	{
	  add_unwind_opcode (0x7f, 1);
	  offset -= 0x100;
	}
      op = ((offset - 4) >> 2) | 0x40;
      add_unwind_opcode (op, 1);
    }
}

/* Finish the list of unwind opcodes for this function.  */
static void
finish_unwind_opcodes (void)
{
  valueT op;

  if (unwind.fp_used)
    {
      /* Adjust sp as neccessary.  */
      unwind.pending_offset += unwind.fp_offset - unwind.frame_size;
      flush_pending_unwind ();

      /* After restoring sp from the frame pointer.  */
      op = 0x90 | unwind.fp_reg;
      add_unwind_opcode (op, 1);
    }
  else
    flush_pending_unwind ();
}


/* Start an exception table entry.  If idx is nonzero this is an index table
   entry.  */

static void
start_unwind_section (const segT text_seg, int idx)
{
  const char * text_name;
  const char * prefix;
  const char * prefix_once;
  size_t prefix_len;
  size_t text_len;
  char * sec_name;
  size_t sec_name_len;

  if (idx)
    {
      prefix = ELF_STRING_ARM_unwind;
      prefix_once = ELF_STRING_ARM_unwind_once;
    }
  else
    {
      prefix = ELF_STRING_ARM_unwind_info;
      prefix_once = ELF_STRING_ARM_unwind_info_once;
    }

  text_name = segment_name (text_seg);
  if (streq (text_name, ".text"))
    text_name = "";

  if (strncmp (text_name, ".gnu.linkonce.t.",
	       strlen (".gnu.linkonce.t.")) == 0)
    {
      prefix = prefix_once;
      text_name += strlen (".gnu.linkonce.t.");
    }

  prefix_len = strlen (prefix);
  text_len = strlen (text_name);
  sec_name_len = prefix_len + text_len;
  sec_name = alloca (sec_name_len + 1);
  memcpy (sec_name, prefix, prefix_len);
  memcpy (sec_name + prefix_len, text_name, text_len);
  sec_name[prefix_len + text_len] = '\0';

  /* Handle COMDAT group.  */
  if (prefix != prefix_once && (text_seg->flags & SEC_LINK_ONCE) != 0)
    {
      char *section;
      size_t len, group_name_len;
      const char *group_name = elf_group_name (text_seg);

      if (group_name == NULL)
        {
          as_bad ("Group section `%s' has no group signature",
                  segment_name (text_seg));
          ignore_rest_of_line ();
          return;
        }
      /* We have to construct a fake section directive.  */
      group_name_len = strlen (group_name);
      if (idx)
	prefix_len = 13;
      else
	prefix_len = 16;

      len = (sec_name_len
             + prefix_len             /* ,"aG",%sectiontype,  */
             + group_name_len         /* ,group_name  */
             + 7);                    /* ,comdat  */

      section = alloca (len + 1);
      memcpy (section, sec_name, sec_name_len);
      if (idx)
	  memcpy (section + sec_name_len, ",\"aG\",%exidx,", 13);
      else
	  memcpy (section + sec_name_len, ",\"aG\",%progbits,", 16);
      memcpy (section + sec_name_len + prefix_len, group_name, group_name_len);
      memcpy (section + len - 7, ",comdat", 7);
      section [len] = '\0';
      set_section (section);
    }
  else
    {
      set_section (sec_name);
      bfd_set_section_flags (stdoutput, now_seg,
			     SEC_LOAD | SEC_ALLOC | SEC_READONLY);
    }

  /* Set the setion link for index tables.  */
  if (idx)
    elf_linked_to_section (now_seg) = text_seg;
}


/* Start an unwind table entry.  HAVE_DATA is nonzero if we have additional
   personality routine data.  Returns zero, or the index table value for
   and inline entry.  */

static valueT
create_unwind_entry (int have_data)
{
  int size;
  addressT where;
  char *ptr;
  /* The current word of data.  */
  valueT data;
  /* The number of bytes left in this word.  */
  int n;

  finish_unwind_opcodes ();

  /* Remember the current text section.  */
  unwind.saved_seg = now_seg;
  unwind.saved_subseg = now_subseg;

  start_unwind_section (now_seg, 0);

  if (unwind.personality_routine == NULL)
    {
      if (unwind.personality_index == -2)
	{
	  if (have_data)
	    as_bad (_("handerdata in cantunwind frame"));
	  return 1; /* EXIDX_CANTUNWIND.  */
	}

      /* Use a default personality routine if none is specified.  */
      if (unwind.personality_index == -1)
	{
	  if (unwind.opcode_count > 3)
	    unwind.personality_index = 1;
	  else
	    unwind.personality_index = 0;
	}

      /* Space for the personality routine entry.  */
      if (unwind.personality_index == 0)
	{
	  if (unwind.opcode_count > 3)
	    as_bad (_("too many unwind opcodes for personality routine 0"));

	  if (!have_data)
	    {
	      /* All the data is inline in the index table.  */
	      data = 0x80;
	      n = 3;
	      while (unwind.opcode_count > 0)
		{
		  unwind.opcode_count--;
		  data = (data << 8) | unwind.opcodes[unwind.opcode_count];
		  n--;
		}

	      /* Pad with "finish" opcodes.  */
	      while (n--)
		data = (data << 8) | 0xb0;

	      return data;
	    }
	  size = 0;
	}
      else
	/* We get two opcodes "free" in the first word.  */
	size = unwind.opcode_count - 2;
    }
  else
    /* An extra byte is required for the opcode count.  */
    size = unwind.opcode_count + 1;

  size = (size + 3) >> 2;
  if (size > 0xff)
    as_bad (_("too many unwind opcodes"));

  frag_align (2, 0, 0);
  record_alignment (now_seg, 2);
  unwind.table_entry = expr_build_dot ();

  /* Allocate the table entry.  */
  ptr = frag_more ((size << 2) + 4);
  where = frag_now_fix () - ((size << 2) + 4);

  switch (unwind.personality_index)
    {
    case -1:
      /* ??? Should this be a PLT generating relocation?  */
      /* Custom personality routine.  */
      fix_new (frag_now, where, 4, unwind.personality_routine, 0, 1,
	       BFD_RELOC_ARM_PREL31);

      /* Indicate dependency to linker.  */
        {
          char *name = "__aeabi_unwind_cpp_pr0";
	  symbolS *pr = symbol_find_or_make (name);
	  fix_new (frag_now, where, 4, pr, 0, 1, BFD_RELOC_NONE);
	}

      where += 4;
      ptr += 4;

      /* Set the first byte to the number of additional words.  */
      data = size - 1;
      n = 3;
      break;

    /* ABI defined personality routines.  */
    case 0:
      /* Three opcodes bytes are packed into the first word.  */
      data = 0x80;
      n = 3;
      goto emit_reloc;

    case 1:
    case 2:
      /* The size and first two opcode bytes go in the first word.  */
      data = ((0x80 + unwind.personality_index) << 8) | size;
      n = 2;
      goto emit_reloc;

    emit_reloc:
      {
	/* Indicate dependency to linker.  */
	char *name[] = { "__aeabi_unwind_cpp_pr0",
	                 "__aeabi_unwind_cpp_pr1",
			 "__aeabi_unwind_cpp_pr2" };
	symbolS *pr = symbol_find_or_make (name[unwind.personality_index]);
	fix_new (frag_now, where, 4, pr, 0, 1, BFD_RELOC_NONE);
      }
      break;

    default:
      /* Should never happen.  */
      abort ();
    }

  /* Pack the opcodes into words (MSB first), reversing the list at the same
     time.  */
  while (unwind.opcode_count > 0)
    {
      if (n == 0)
	{
	  md_number_to_chars (ptr, data, 4);
	  ptr += 4;
	  n = 4;
	  data = 0;
	}
      unwind.opcode_count--;
      n--;
      data = (data << 8) | unwind.opcodes[unwind.opcode_count];
    }

  /* Finish off the last word.  */
  if (n < 4)
    {
      /* Pad with "finish" opcodes.  */
      while (n--)
	data = (data << 8) | 0xb0;

      md_number_to_chars (ptr, data, 4);
    }

  if (!have_data)
    {
      /* Add an empty descriptor if there is no user-specified data.   */
      ptr = frag_more (4);
      md_number_to_chars (ptr, 0, 4);
    }

  return 0;
}


/* Parse an unwind_fnstart directive.  Simply records the current location.  */

static void
s_arm_unwind_fnstart (int ignored ATTRIBUTE_UNUSED)
{
  demand_empty_rest_of_line ();
  /* Mark the start of the function.  */
  unwind.proc_start = expr_build_dot ();

  /* Reset the rest of the unwind info.  */
  unwind.opcode_count = 0;
  unwind.table_entry = NULL;
  unwind.personality_routine = NULL;
  unwind.personality_index = -1;
  unwind.frame_size = 0;
  unwind.fp_offset = 0;
  unwind.fp_reg = 13;
  unwind.fp_used = 0;
  unwind.sp_restored = 0;
}


/* Parse a handlerdata directive.  Creates the exception handling table entry
   for the function.  */

static void
s_arm_unwind_handlerdata (int ignored ATTRIBUTE_UNUSED)
{
  demand_empty_rest_of_line ();
  if (unwind.table_entry)
    as_bad (_("dupicate .handlerdata directive"));

  create_unwind_entry (1);
}

/* Parse an unwind_fnend directive.  Generates the index table entry.  */

static void
s_arm_unwind_fnend (int ignored ATTRIBUTE_UNUSED)
{
  long where;
  char *ptr;
  valueT val;

  demand_empty_rest_of_line ();

  /* Add eh table entry.  */
  if (unwind.table_entry == NULL)
    val = create_unwind_entry (0);
  else
    val = 0;

  /* Add index table entry.  This is two words.  */
  start_unwind_section (unwind.saved_seg, 1);
  frag_align (2, 0, 0);
  record_alignment (now_seg, 2);

  ptr = frag_more (8);
  where = frag_now_fix () - 8;

  /* Self relative offset of the function start.  */
  fix_new (frag_now, where, 4, unwind.proc_start, 0, 1,
	   BFD_RELOC_ARM_PREL31);

  if (val)
    /* Inline exception table entry.  */
    md_number_to_chars (ptr + 4, val, 4);
  else
    /* Self relative offset of the table entry.  */
    fix_new (frag_now, where + 4, 4, unwind.table_entry, 0, 1,
	     BFD_RELOC_ARM_PREL31);

  /* Restore the original section.  */
  subseg_set (unwind.saved_seg, unwind.saved_subseg);
}


/* Parse an unwind_cantunwind directive.  */

static void
s_arm_unwind_cantunwind (int ignored ATTRIBUTE_UNUSED)
{
  demand_empty_rest_of_line ();
  if (unwind.personality_routine || unwind.personality_index != -1)
    as_bad (_("personality routine specified for cantunwind frame"));

  unwind.personality_index = -2;
}


/* Parse a personalityindex directive.  */

static void
s_arm_unwind_personalityindex (int ignored ATTRIBUTE_UNUSED)
{
  expressionS exp;

  if (unwind.personality_routine || unwind.personality_index != -1)
    as_bad (_("duplicate .personalityindex directive"));

  SKIP_WHITESPACE ();

  expression (&exp);

  if (exp.X_op != O_constant
      || exp.X_add_number < 0 || exp.X_add_number > 15)
    {
      as_bad (_("bad personality routine number"));
      ignore_rest_of_line ();
      return;
    }

  unwind.personality_index = exp.X_add_number;

  demand_empty_rest_of_line ();
}


/* Parse a personality directive.  */

static void
s_arm_unwind_personality (int ignored ATTRIBUTE_UNUSED)
{
  char *name, *p, c;

  if (unwind.personality_routine || unwind.personality_index != -1)
    as_bad (_("duplicate .personality directive"));

  SKIP_WHITESPACE ();
  name = input_line_pointer;
  c = get_symbol_end ();
  p = input_line_pointer;
  unwind.personality_routine = symbol_find_or_make (name);
  *p = c;
  SKIP_WHITESPACE ();
  demand_empty_rest_of_line ();
}


/* Parse a directive saving core registers.  */

static void
s_arm_unwind_save_core (void)
{
  valueT op;
  long range;
  int n;

  SKIP_WHITESPACE ();
  range = reg_list (&input_line_pointer);
  if (range == FAIL)
    {
      as_bad (_("expected register list"));
      ignore_rest_of_line ();
      return;
    }

  demand_empty_rest_of_line ();

  /* Turn .unwind_movsp ip followed by .unwind_save {..., ip, ...}
     into .unwind_save {..., sp...}.  We aren't bothered about the value of
     ip because it is clobbered by calls.  */
  if (unwind.sp_restored && unwind.fp_reg == 12
      && (range & 0x3000) == 0x1000)
    {
      unwind.opcode_count--;
      unwind.sp_restored = 0;
      range = (range | 0x2000) & ~0x1000;
      unwind.pending_offset = 0;
    }

  /* See if we can use the short opcodes.  These pop a block of upto 8
     registers starting with r4, plus maybe r14.  */
  for (n = 0; n < 8; n++)
    {
      /* Break at the first non-saved register.  */
      if ((range & (1 << (n + 4))) == 0)
	break;
    }
  /* See if there are any other bits set.  */
  if (n == 0 || (range & (0xfff0 << n) & 0xbff0) != 0)
    {
      /* Use the long form.  */
      op = 0x8000 | ((range >> 4) & 0xfff);
      add_unwind_opcode (op, 2);
    }
  else
    {
      /* Use the short form.  */
      if (range & 0x4000)
	op = 0xa8; /* Pop r14.  */
      else
	op = 0xa0; /* Do not pop r14.  */
      op |= (n - 1);
      add_unwind_opcode (op, 1);
    }

  /* Pop r0-r3.  */
  if (range & 0xf)
    {
      op = 0xb100 | (range & 0xf);
      add_unwind_opcode (op, 2);
    }

  /* Record the number of bytes pushed.  */
  for (n = 0; n < 16; n++)
    {
      if (range & (1 << n))
	unwind.frame_size += 4;
    }
}


/* Parse a directive saving FPA registers.  */

static void
s_arm_unwind_save_fpa (int reg)
{
  expressionS exp;
  int num_regs;
  valueT op;

  /* Get Number of registers to transfer.  */
  if (skip_past_comma (&input_line_pointer) != FAIL)
    expression (&exp);
  else
    exp.X_op = O_illegal;

  if (exp.X_op != O_constant)
    {
      as_bad (_("expected , <constant>"));
      ignore_rest_of_line ();
      return;
    }

  num_regs = exp.X_add_number;

  if (num_regs < 1 || num_regs > 4)
    {
      as_bad (_("number of registers must be in the range [1:4]"));
      ignore_rest_of_line ();
      return;
    }

  demand_empty_rest_of_line ();

  if (reg == 4)
    {
      /* Short form.  */
      op = 0xb4 | (num_regs - 1);
      add_unwind_opcode (op, 1);
    }
  else
    {
      /* Long form.  */
      op = 0xc800 | (reg << 4) | (num_regs - 1);
      add_unwind_opcode (op, 2);
    }
  unwind.frame_size += num_regs * 12;
}


/* Parse a directive saving VFP registers.  */

static void
s_arm_unwind_save_vfp (void)
{
  int count;
  int reg;
  valueT op;

  count = vfp_parse_reg_list (&input_line_pointer, &reg, 1);
  if (count == FAIL)
    {
      as_bad (_("expected register list"));
      ignore_rest_of_line ();
      return;
    }

  demand_empty_rest_of_line ();

  if (reg == 8)
    {
      /* Short form.  */
      op = 0xb8 | (count - 1);
      add_unwind_opcode (op, 1);
    }
  else
    {
      /* Long form.  */
      op = 0xb300 | (reg << 4) | (count - 1);
      add_unwind_opcode (op, 2);
    }
  unwind.frame_size += count * 8 + 4;
}


/* Parse a directive saving iWMMXt registers.  */

static void
s_arm_unwind_save_wmmx (void)
{
  int reg;
  int hi_reg;
  int i;
  unsigned wcg_mask;
  unsigned wr_mask;
  valueT op;

  if (*input_line_pointer == '{')
    input_line_pointer++;

  wcg_mask = 0;
  wr_mask = 0;
  do
    {
      reg = arm_reg_parse (&input_line_pointer,
			   all_reg_maps[REG_TYPE_IWMMXT].htab);

      if (wr_register (reg))
	{
	  i = reg & ~WR_PREFIX;
	  if (wr_mask >> i)
	    as_tsktsk (_("register list not in ascending order"));
	  wr_mask |= 1 << i;
	}
      else if (wcg_register (reg))
	{
	  i = (reg & ~WC_PREFIX) - 8;
	  if (wcg_mask >> i)
	    as_tsktsk (_("register list not in ascending order"));
	  wcg_mask |= 1 << i;
	}
      else
	{
	  as_bad (_("expected wr or wcgr"));
	  goto error;
	}

      SKIP_WHITESPACE ();
      if (*input_line_pointer == '-')
	{
	  hi_reg = arm_reg_parse (&input_line_pointer,
				  all_reg_maps[REG_TYPE_IWMMXT].htab);
	  if (wr_register (reg) && wr_register (hi_reg))
	    {
	      for (; reg < hi_reg; reg++)
		wr_mask |= 1 << (reg & ~WR_PREFIX);
	    }
	  else if (wcg_register (reg) && wcg_register (hi_reg))
	    {
	      for (; reg < hi_reg; reg++)
		wcg_mask |= 1 << ((reg & ~WC_PREFIX) - 8);
	    }
	  else
	    {
	      as_bad (_("bad register range"));
	      goto error;
	    }
	}
    }
  while (skip_past_comma (&input_line_pointer) != FAIL);

  SKIP_WHITESPACE ();
  if (*input_line_pointer == '}')
    input_line_pointer++;

  demand_empty_rest_of_line ();

  if (wr_mask && wcg_mask)
    {
      as_bad (_("inconsistent register types"));
      goto error;
    }

  /* Generate any deferred opcodes becuuse we're going to be looking at
     the list.  */
  flush_pending_unwind ();

  if (wcg_mask)
    {
      for (i = 0; i < 16; i++)
	{
	  if (wcg_mask & (1 << i))
	    unwind.frame_size += 4;
	}
      op = 0xc700 | wcg_mask;
      add_unwind_opcode (op, 2);
    }
  else
    {
      for (i = 0; i < 16; i++)
	{
	  if (wr_mask & (1 << i))
	    unwind.frame_size += 8;
	}
      /* Attempt to combine with a previous opcode.  We do this because gcc
	 likes to output separate unwind directives for a single block of
	 registers.  */
      if (unwind.opcode_count > 0)
	{
	  i = unwind.opcodes[unwind.opcode_count - 1];
	  if ((i & 0xf8) == 0xc0)
	    {
	      i &= 7;
	      /* Only merge if the blocks are contiguous.  */
	      if (i < 6)
		{
		  if ((wr_mask & 0xfe00) == (1 << 9))
		    {
		      wr_mask |= ((1 << (i + 11)) - 1) & 0xfc00;
		      unwind.opcode_count--;
		    }
		}
	      else if (i == 6 && unwind.opcode_count >= 2)
		{
		  i = unwind.opcodes[unwind.opcode_count - 2];
		  reg = i >> 4;
		  i &= 0xf;

		  op = 0xffff << (reg - 1);
		  if (reg > 0
		      || ((wr_mask & op) == (1u << (reg - 1))))
		    {
		      op = (1 << (reg + i + 1)) - 1;
		      op &= ~((1 << reg) - 1);
		      wr_mask |= op;
		      unwind.opcode_count -= 2;
		    }
		}
	    }
	}

      hi_reg = 15;
      /* We want to generate opcodes in the order the registers have been
	 saved, ie. descending order.  */
      for (reg = 15; reg >= -1; reg--)
	{
	  /* Save registers in blocks.  */
	  if (reg < 0
	      || !(wr_mask & (1 << reg)))
	    {
	      /* We found an unsaved reg.  Generate opcodes to save the
		 preceeding block.  */
	      if (reg != hi_reg)
		{
		  if (reg == 9)
		    {
		      /* Short form.  */
		      op = 0xc0 | (hi_reg - 10);
		      add_unwind_opcode (op, 1);
		    }
		  else
		    {
		      /* Long form.  */
		      op = 0xc600 | ((reg + 1) << 4) | ((hi_reg - reg) - 1);
		      add_unwind_opcode (op, 2);
		    }
		}
	      hi_reg = reg - 1;
	    }
	}
    }
  return;
error:
  ignore_rest_of_line ();
}


/* Parse an unwind_save directive.  */

static void
s_arm_unwind_save (int ignored ATTRIBUTE_UNUSED)
{
  char *saved_ptr;
  int reg;

  /* Figure out what sort of save we have.  */
  SKIP_WHITESPACE ();
  saved_ptr = input_line_pointer;

  reg = arm_reg_parse (&input_line_pointer, all_reg_maps[REG_TYPE_FN].htab);
  if (reg != FAIL)
    {
      s_arm_unwind_save_fpa (reg);
      return;
    }

  if (*input_line_pointer == '{')
    input_line_pointer++;

  SKIP_WHITESPACE ();

  reg = arm_reg_parse (&input_line_pointer, all_reg_maps[REG_TYPE_RN].htab);
  if (reg != FAIL)
    {
      input_line_pointer = saved_ptr;
      s_arm_unwind_save_core ();
      return;
    }

  reg = arm_reg_parse (&input_line_pointer, all_reg_maps[REG_TYPE_DN].htab);
  if (reg != FAIL)
    {
      input_line_pointer = saved_ptr;
      s_arm_unwind_save_vfp ();
      return;
    }

  reg = arm_reg_parse (&input_line_pointer,
		       all_reg_maps[REG_TYPE_IWMMXT].htab);
  if (reg != FAIL)
    {
      input_line_pointer = saved_ptr;
      s_arm_unwind_save_wmmx ();
      return;
    }

  /* TODO: Maverick registers.  */
  as_bad (_("unrecognised register"));
}


/* Parse an unwind_movsp directive.  */

static void
s_arm_unwind_movsp (int ignored ATTRIBUTE_UNUSED)
{
  int reg;
  valueT op;

  SKIP_WHITESPACE ();
  reg = reg_required_here (&input_line_pointer, -1);
  if (reg == FAIL)
    {
      as_bad (_("ARM register expected"));
      ignore_rest_of_line ();
      return;
    }

  if (reg == 13 || reg == 15)
    {
      as_bad (_("r%d not permitted in .unwind_movsp directive"), reg);
      ignore_rest_of_line ();
      return;
    }

  if (unwind.fp_reg != 13)
    as_bad (_("unexpected .unwind_movsp directive"));

  /* Generate opcode to restore the value.  */
  op = 0x90 | reg;
  add_unwind_opcode (op, 1);

  /* Record the information for later.  */
  unwind.fp_reg = reg;
  unwind.fp_offset = unwind.frame_size;
  unwind.sp_restored = 1;
  demand_empty_rest_of_line ();
}


/* Parse #<number>.  */

static int
require_hashconst (int * val)
{
  expressionS exp;

  SKIP_WHITESPACE ();
  if (*input_line_pointer == '#')
    {
      input_line_pointer++;
      expression (&exp);
    }
  else
    exp.X_op = O_illegal;

  if (exp.X_op != O_constant)
    {
      as_bad (_("expected #constant"));
      ignore_rest_of_line ();
      return FAIL;
    }
  *val = exp.X_add_number;
  return SUCCESS;
}

/* Parse an unwind_pad directive.  */

static void
s_arm_unwind_pad (int ignored ATTRIBUTE_UNUSED)
{
  int offset;

  if (require_hashconst (&offset) == FAIL)
    return;

  if (offset & 3)
    {
      as_bad (_("stack increment must be multiple of 4"));
      ignore_rest_of_line ();
      return;
    }

  /* Don't generate any opcodes, just record the details for later.  */
  unwind.frame_size += offset;
  unwind.pending_offset += offset;

  demand_empty_rest_of_line ();
}

/* Parse an unwind_setfp directive.  */

static void
s_arm_unwind_setfp (int ignored ATTRIBUTE_UNUSED)
{
  int sp_reg;
  int fp_reg;
  int offset;

  fp_reg = reg_required_here (&input_line_pointer, -1);
  if (skip_past_comma (&input_line_pointer) == FAIL)
    sp_reg = FAIL;
  else
    sp_reg = reg_required_here (&input_line_pointer, -1);

  if (fp_reg == FAIL || sp_reg == FAIL)
    {
      as_bad (_("expected <reg>, <reg>"));
      ignore_rest_of_line ();
      return;
    }

  /* Optonal constant.  */
  if (skip_past_comma (&input_line_pointer) != FAIL)
    {
      if (require_hashconst (&offset) == FAIL)
	return;
    }
  else
    offset = 0;

  demand_empty_rest_of_line ();

  if (sp_reg != 13 && sp_reg != unwind.fp_reg)
    {
      as_bad (_("register must be either sp or set by a previous"
		"unwind_movsp directive"));
      return;
    }

  /* Don't generate any opcodes, just record the information for later.  */
  unwind.fp_reg = fp_reg;
  unwind.fp_used = 1;
  if (sp_reg == 13)
    unwind.fp_offset = unwind.frame_size - offset;
  else
    unwind.fp_offset -= offset;
}

/* Parse an unwind_raw directive.  */

static void
s_arm_unwind_raw (int ignored ATTRIBUTE_UNUSED)
{
  expressionS exp;
  /* This is an arbitary limit.  */
  unsigned char op[16];
  int count;

  SKIP_WHITESPACE ();
  expression (&exp);
  if (exp.X_op == O_constant
      && skip_past_comma (&input_line_pointer) != FAIL)
    {
      unwind.frame_size += exp.X_add_number;
      expression (&exp);
    }
  else
    exp.X_op = O_illegal;

  if (exp.X_op != O_constant)
    {
      as_bad (_("expected <offset>, <opcode>"));
      ignore_rest_of_line ();
      return;
    }

  count = 0;

  /* Parse the opcode.  */
  for (;;)
    {
      if (count >= 16)
	{
	  as_bad (_("unwind opcode too long"));
	  ignore_rest_of_line ();
	}
      if (exp.X_op != O_constant || exp.X_add_number & ~0xff)
	{
	  as_bad (_("invalid unwind opcode"));
	  ignore_rest_of_line ();
	  return;
	}
      op[count++] = exp.X_add_number;

      /* Parse the next byte.  */
      if (skip_past_comma (&input_line_pointer) == FAIL)
	break;

      expression (&exp);
    }

  /* Add the opcode bytes in reverse order.  */
  while (count--)
    add_unwind_opcode (op[count], 1);

  demand_empty_rest_of_line ();
}

#endif /* OBJ_ELF */

/* This is called from HANDLE_ALIGN in write.c.  Fill in the contents
   of an rs_align_code fragment.  */

void
arm_handle_align (fragS * fragP)
{
  static char const arm_noop[4] = { 0x00, 0x00, 0xa0, 0xe1 };
  static char const thumb_noop[2] = { 0xc0, 0x46 };
  static char const arm_bigend_noop[4] = { 0xe1, 0xa0, 0x00, 0x00 };
  static char const thumb_bigend_noop[2] = { 0x46, 0xc0 };

  int bytes, fix, noop_size;
  char * p;
  const char * noop;

  if (fragP->fr_type != rs_align_code)
    return;

  bytes = fragP->fr_next->fr_address - fragP->fr_address - fragP->fr_fix;
  p = fragP->fr_literal + fragP->fr_fix;
  fix = 0;

  if (bytes > MAX_MEM_FOR_RS_ALIGN_CODE)
    bytes &= MAX_MEM_FOR_RS_ALIGN_CODE;

  if (fragP->tc_frag_data)
    {
      if (target_big_endian)
	noop = thumb_bigend_noop;
      else
	noop = thumb_noop;
      noop_size = sizeof (thumb_noop);
    }
  else
    {
      if (target_big_endian)
	noop = arm_bigend_noop;
      else
	noop = arm_noop;
      noop_size = sizeof (arm_noop);
    }

  if (bytes & (noop_size - 1))
    {
      fix = bytes & (noop_size - 1);
      memset (p, 0, fix);
      p += fix;
      bytes -= fix;
    }

  while (bytes >= noop_size)
    {
      memcpy (p, noop, noop_size);
      p += noop_size;
      bytes -= noop_size;
      fix += noop_size;
    }

  fragP->fr_fix += fix;
  fragP->fr_var = noop_size;
}

/* Called from md_do_align.  Used to create an alignment
   frag in a code section.  */

void
arm_frag_align_code (int n, int max)
{
  char * p;

  /* We assume that there will never be a requirement
     to support alignments greater than 32 bytes.  */
  if (max > MAX_MEM_FOR_RS_ALIGN_CODE)
    as_fatal (_("alignments greater than 32 bytes not supported in .text sections."));

  p = frag_var (rs_align_code,
		MAX_MEM_FOR_RS_ALIGN_CODE,
		1,
		(relax_substateT) max,
		(symbolS *) NULL,
		(offsetT) n,
		(char *) NULL);
  *p = 0;
}

/* Perform target specific initialisation of a frag.  */

void
arm_init_frag (fragS * fragP)
{
  /* Record whether this frag is in an ARM or a THUMB area.  */
  fragP->tc_frag_data = thumb_mode;
}

#ifdef OBJ_ELF

/* Convert REGNAME to a DWARF-2 register number.  */

int
tc_arm_regname_to_dw2regnum (const char *regname)
{
  unsigned int i;

  for (i = 0; rn_table[i].name; i++)
    if (streq (regname, rn_table[i].name))
      return rn_table[i].number;

  return -1;
}

/* Initialize the DWARF-2 unwind information for this procedure.  */

void
tc_arm_frame_initial_instructions (void)
{
  cfi_add_CFA_def_cfa (REG_SP, 0);
}
#endif

/* This table describes all the machine specific pseudo-ops the assembler
   has to support.  The fields are:
     pseudo-op name without dot
     function to call to execute this pseudo-op
     Integer arg to pass to the function.  */

const pseudo_typeS md_pseudo_table[] =
{
  /* Never called because '.req' does not start a line.  */
  { "req",         s_req,         0 },
  { "unreq",       s_unreq,       0 },
  { "bss",         s_bss,         0 },
  { "align",       s_align,       0 },
  { "arch",        s_arch,        0 },
  { "arm",         s_arm,         0 },
  { "thumb",       s_thumb,       0 },
  { "code",        s_code,        0 },
  { "force_thumb", s_force_thumb, 0 },
  { "thumb_func",  s_thumb_func,  0 },
  { "thumb_set",   s_thumb_set,   0 },
  { "even",        s_even,        0 },
  { "ltorg",       s_ltorg,       0 },
  { "pool",        s_ltorg,       0 },
#ifdef OBJ_ELF
  { "word",        s_arm_elf_cons, 4 },
  { "long",        s_arm_elf_cons, 4 },
  { "rel31",       s_arm_rel31,   0 },
  { "fnstart",		s_arm_unwind_fnstart,	0 },
  { "fnend",		s_arm_unwind_fnend,	0 },
  { "cantunwind",	s_arm_unwind_cantunwind, 0 },
  { "personality",	s_arm_unwind_personality, 0 },
  { "personalityindex",	s_arm_unwind_personalityindex, 0 },
  { "handlerdata",	s_arm_unwind_handlerdata, 0 },
  { "save",		s_arm_unwind_save,	0 },
  { "movsp",		s_arm_unwind_movsp,	0 },
  { "pad",		s_arm_unwind_pad,	0 },
  { "setfp",		s_arm_unwind_setfp,	0 },
  { "unwind_raw",	s_arm_unwind_raw,	0 },
#else
  { "word",        cons, 4},
#endif
  { "extend",      float_cons, 'x' },
  { "ldouble",     float_cons, 'x' },
  { "packed",      float_cons, 'p' },
  { 0, 0, 0 }
};
