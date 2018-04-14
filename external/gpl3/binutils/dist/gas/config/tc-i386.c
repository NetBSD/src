/* tc-i386.c -- Assemble code for the Intel 80386
   Copyright (C) 1989-2018 Free Software Foundation, Inc.

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

/* Intel 80386 machine specific gas.
   Written by Eliot Dresselhaus (eliot@mgm.mit.edu).
   x86_64 support by Jan Hubicka (jh@suse.cz)
   VIA PadLock support by Michal Ludvig (mludvig@suse.cz)
   Bugs & suggestions are completely welcome.  This is free software.
   Please help us make it better.  */

#include "as.h"
#include "safe-ctype.h"
#include "subsegs.h"
#include "dwarf2dbg.h"
#include "dw2gencfi.h"
#include "elf/x86-64.h"
#include "opcodes/i386-init.h"

#ifndef REGISTER_WARNINGS
#define REGISTER_WARNINGS 1
#endif

#ifndef INFER_ADDR_PREFIX
#define INFER_ADDR_PREFIX 1
#endif

#ifndef DEFAULT_ARCH
#define DEFAULT_ARCH "i386"
#endif

#ifndef INLINE
#if __GNUC__ >= 2
#define INLINE __inline__
#else
#define INLINE
#endif
#endif

/* Prefixes will be emitted in the order defined below.
   WAIT_PREFIX must be the first prefix since FWAIT is really is an
   instruction, and so must come before any prefixes.
   The preferred prefix order is SEG_PREFIX, ADDR_PREFIX, DATA_PREFIX,
   REP_PREFIX/HLE_PREFIX, LOCK_PREFIX.  */
#define WAIT_PREFIX	0
#define SEG_PREFIX	1
#define ADDR_PREFIX	2
#define DATA_PREFIX	3
#define REP_PREFIX	4
#define HLE_PREFIX	REP_PREFIX
#define BND_PREFIX	REP_PREFIX
#define LOCK_PREFIX	5
#define REX_PREFIX	6       /* must come last.  */
#define MAX_PREFIXES	7	/* max prefixes per opcode */

/* we define the syntax here (modulo base,index,scale syntax) */
#define REGISTER_PREFIX '%'
#define IMMEDIATE_PREFIX '$'
#define ABSOLUTE_PREFIX '*'

/* these are the instruction mnemonic suffixes in AT&T syntax or
   memory operand size in Intel syntax.  */
#define WORD_MNEM_SUFFIX  'w'
#define BYTE_MNEM_SUFFIX  'b'
#define SHORT_MNEM_SUFFIX 's'
#define LONG_MNEM_SUFFIX  'l'
#define QWORD_MNEM_SUFFIX  'q'
#define XMMWORD_MNEM_SUFFIX  'x'
#define YMMWORD_MNEM_SUFFIX 'y'
#define ZMMWORD_MNEM_SUFFIX 'z'
/* Intel Syntax.  Use a non-ascii letter since since it never appears
   in instructions.  */
#define LONG_DOUBLE_MNEM_SUFFIX '\1'

#define END_OF_INSN '\0'

/*
  'templates' is for grouping together 'template' structures for opcodes
  of the same name.  This is only used for storing the insns in the grand
  ole hash table of insns.
  The templates themselves start at START and range up to (but not including)
  END.
  */
typedef struct
{
  const insn_template *start;
  const insn_template *end;
}
templates;

/* 386 operand encoding bytes:  see 386 book for details of this.  */
typedef struct
{
  unsigned int regmem;	/* codes register or memory operand */
  unsigned int reg;	/* codes register operand (or extended opcode) */
  unsigned int mode;	/* how to interpret regmem & reg */
}
modrm_byte;

/* x86-64 extension prefix.  */
typedef int rex_byte;

/* 386 opcode byte to code indirect addressing.  */
typedef struct
{
  unsigned base;
  unsigned index;
  unsigned scale;
}
sib_byte;

/* x86 arch names, types and features */
typedef struct
{
  const char *name;		/* arch name */
  unsigned int len;		/* arch string length */
  enum processor_type type;	/* arch type */
  i386_cpu_flags flags;		/* cpu feature flags */
  unsigned int skip;		/* show_arch should skip this. */
}
arch_entry;

/* Used to turn off indicated flags.  */
typedef struct
{
  const char *name;		/* arch name */
  unsigned int len;		/* arch string length */
  i386_cpu_flags flags;		/* cpu feature flags */
}
noarch_entry;

static void update_code_flag (int, int);
static void set_code_flag (int);
static void set_16bit_gcc_code_flag (int);
static void set_intel_syntax (int);
static void set_intel_mnemonic (int);
static void set_allow_index_reg (int);
static void set_check (int);
static void set_cpu_arch (int);
#ifdef TE_PE
static void pe_directive_secrel (int);
#endif
static void signed_cons (int);
static char *output_invalid (int c);
static int i386_finalize_immediate (segT, expressionS *, i386_operand_type,
				    const char *);
static int i386_finalize_displacement (segT, expressionS *, i386_operand_type,
				       const char *);
static int i386_att_operand (char *);
static int i386_intel_operand (char *, int);
static int i386_intel_simplify (expressionS *);
static int i386_intel_parse_name (const char *, expressionS *);
static const reg_entry *parse_register (char *, char **);
static char *parse_insn (char *, char *);
static char *parse_operands (char *, const char *);
static void swap_operands (void);
static void swap_2_operands (int, int);
static void optimize_imm (void);
static void optimize_disp (void);
static const insn_template *match_template (char);
static int check_string (void);
static int process_suffix (void);
static int check_byte_reg (void);
static int check_long_reg (void);
static int check_qword_reg (void);
static int check_word_reg (void);
static int finalize_imm (void);
static int process_operands (void);
static const seg_entry *build_modrm_byte (void);
static void output_insn (void);
static void output_imm (fragS *, offsetT);
static void output_disp (fragS *, offsetT);
#ifndef I386COFF
static void s_bss (int);
#endif
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
static void handle_large_common (int small ATTRIBUTE_UNUSED);
#endif

static const char *default_arch = DEFAULT_ARCH;

/* This struct describes rounding control and SAE in the instruction.  */
struct RC_Operation
{
  enum rc_type
    {
      rne = 0,
      rd,
      ru,
      rz,
      saeonly
    } type;
  int operand;
};

static struct RC_Operation rc_op;

/* The struct describes masking, applied to OPERAND in the instruction.
   MASK is a pointer to the corresponding mask register.  ZEROING tells
   whether merging or zeroing mask is used.  */
struct Mask_Operation
{
  const reg_entry *mask;
  unsigned int zeroing;
  /* The operand where this operation is associated.  */
  int operand;
};

static struct Mask_Operation mask_op;

/* The struct describes broadcasting, applied to OPERAND.  FACTOR is
   broadcast factor.  */
struct Broadcast_Operation
{
  /* Type of broadcast: no broadcast, {1to8}, or {1to16}.  */
  int type;

  /* Index of broadcasted operand.  */
  int operand;
};

static struct Broadcast_Operation broadcast_op;

/* VEX prefix.  */
typedef struct
{
  /* VEX prefix is either 2 byte or 3 byte.  EVEX is 4 byte.  */
  unsigned char bytes[4];
  unsigned int length;
  /* Destination or source register specifier.  */
  const reg_entry *register_specifier;
} vex_prefix;

/* 'md_assemble ()' gathers together information and puts it into a
   i386_insn.  */

union i386_op
  {
    expressionS *disps;
    expressionS *imms;
    const reg_entry *regs;
  };

enum i386_error
  {
    operand_size_mismatch,
    operand_type_mismatch,
    register_type_mismatch,
    number_of_operands_mismatch,
    invalid_instruction_suffix,
    bad_imm4,
    old_gcc_only,
    unsupported_with_intel_mnemonic,
    unsupported_syntax,
    unsupported,
    invalid_vsib_address,
    invalid_vector_register_set,
    unsupported_vector_index_register,
    unsupported_broadcast,
    broadcast_not_on_src_operand,
    broadcast_needed,
    unsupported_masking,
    mask_not_on_destination,
    no_default_mask,
    unsupported_rc_sae,
    rc_sae_operand_not_last_imm,
    invalid_register_operand,
  };

struct _i386_insn
  {
    /* TM holds the template for the insn were currently assembling.  */
    insn_template tm;

    /* SUFFIX holds the instruction size suffix for byte, word, dword
       or qword, if given.  */
    char suffix;

    /* OPERANDS gives the number of given operands.  */
    unsigned int operands;

    /* REG_OPERANDS, DISP_OPERANDS, MEM_OPERANDS, IMM_OPERANDS give the number
       of given register, displacement, memory operands and immediate
       operands.  */
    unsigned int reg_operands, disp_operands, mem_operands, imm_operands;

    /* TYPES [i] is the type (see above #defines) which tells us how to
       use OP[i] for the corresponding operand.  */
    i386_operand_type types[MAX_OPERANDS];

    /* Displacement expression, immediate expression, or register for each
       operand.  */
    union i386_op op[MAX_OPERANDS];

    /* Flags for operands.  */
    unsigned int flags[MAX_OPERANDS];
#define Operand_PCrel 1

    /* Relocation type for operand */
    enum bfd_reloc_code_real reloc[MAX_OPERANDS];

    /* BASE_REG, INDEX_REG, and LOG2_SCALE_FACTOR are used to encode
       the base index byte below.  */
    const reg_entry *base_reg;
    const reg_entry *index_reg;
    unsigned int log2_scale_factor;

    /* SEG gives the seg_entries of this insn.  They are zero unless
       explicit segment overrides are given.  */
    const seg_entry *seg[2];

    /* Copied first memory operand string, for re-checking.  */
    char *memop1_string;

    /* PREFIX holds all the given prefix opcodes (usually null).
       PREFIXES is the number of prefix opcodes.  */
    unsigned int prefixes;
    unsigned char prefix[MAX_PREFIXES];

    /* RM and SIB are the modrm byte and the sib byte where the
       addressing modes of this insn are encoded.  */
    modrm_byte rm;
    rex_byte rex;
    rex_byte vrex;
    sib_byte sib;
    vex_prefix vex;

    /* Masking attributes.  */
    struct Mask_Operation *mask;

    /* Rounding control and SAE attributes.  */
    struct RC_Operation *rounding;

    /* Broadcasting attributes.  */
    struct Broadcast_Operation *broadcast;

    /* Compressed disp8*N attribute.  */
    unsigned int memshift;

    /* Prefer load or store in encoding.  */
    enum
      {
	dir_encoding_default = 0,
	dir_encoding_load,
	dir_encoding_store
      } dir_encoding;

    /* Prefer 8bit or 32bit displacement in encoding.  */
    enum
      {
	disp_encoding_default = 0,
	disp_encoding_8bit,
	disp_encoding_32bit
      } disp_encoding;

    /* How to encode vector instructions.  */
    enum
      {
	vex_encoding_default = 0,
	vex_encoding_vex2,
	vex_encoding_vex3,
	vex_encoding_evex
      } vec_encoding;

    /* REP prefix.  */
    const char *rep_prefix;

    /* HLE prefix.  */
    const char *hle_prefix;

    /* Have BND prefix.  */
    const char *bnd_prefix;

    /* Have NOTRACK prefix.  */
    const char *notrack_prefix;

    /* Error message.  */
    enum i386_error error;
  };

typedef struct _i386_insn i386_insn;

/* Link RC type with corresponding string, that'll be looked for in
   asm.  */
struct RC_name
{
  enum rc_type type;
  const char *name;
  unsigned int len;
};

static const struct RC_name RC_NamesTable[] =
{
  {  rne, STRING_COMMA_LEN ("rn-sae") },
  {  rd,  STRING_COMMA_LEN ("rd-sae") },
  {  ru,  STRING_COMMA_LEN ("ru-sae") },
  {  rz,  STRING_COMMA_LEN ("rz-sae") },
  {  saeonly,  STRING_COMMA_LEN ("sae") },
};

/* List of chars besides those in app.c:symbol_chars that can start an
   operand.  Used to prevent the scrubber eating vital white-space.  */
const char extra_symbol_chars[] = "*%-([{}"
#ifdef LEX_AT
	"@"
#endif
#ifdef LEX_QM
	"?"
#endif
	;

#if (defined (TE_I386AIX)				\
     || ((defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF))	\
	 && !defined (TE_GNU)				\
	 && !defined (TE_LINUX)				\
	 && !defined (TE_NACL)				\
	 && !defined (TE_NETWARE)			\
	 && !defined (TE_FreeBSD)			\
	 && !defined (TE_DragonFly)			\
	 && !defined (TE_NetBSD)))
/* This array holds the chars that always start a comment.  If the
   pre-processor is disabled, these aren't very useful.  The option
   --divide will remove '/' from this list.  */
const char *i386_comment_chars = "#/";
#define SVR4_COMMENT_CHARS 1
#define PREFIX_SEPARATOR '\\'

#else
const char *i386_comment_chars = "#";
#define PREFIX_SEPARATOR '/'
#endif

/* This array holds the chars that only start a comment at the beginning of
   a line.  If the line seems to have the form '# 123 filename'
   .line and .file directives will appear in the pre-processed output.
   Note that input_file.c hand checks for '#' at the beginning of the
   first line of the input file.  This is because the compiler outputs
   #NO_APP at the beginning of its output.
   Also note that comments started like this one will always work if
   '/' isn't otherwise defined.  */
const char line_comment_chars[] = "#/";

const char line_separator_chars[] = ";";

/* Chars that can be used to separate mant from exp in floating point
   nums.  */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant
   As in 0f12.456
   or    0d1.2345e12.  */
const char FLT_CHARS[] = "fFdDxX";

/* Tables for lexical analysis.  */
static char mnemonic_chars[256];
static char register_chars[256];
static char operand_chars[256];
static char identifier_chars[256];
static char digit_chars[256];

/* Lexical macros.  */
#define is_mnemonic_char(x) (mnemonic_chars[(unsigned char) x])
#define is_operand_char(x) (operand_chars[(unsigned char) x])
#define is_register_char(x) (register_chars[(unsigned char) x])
#define is_space_char(x) ((x) == ' ')
#define is_identifier_char(x) (identifier_chars[(unsigned char) x])
#define is_digit_char(x) (digit_chars[(unsigned char) x])

/* All non-digit non-letter characters that may occur in an operand.  */
static char operand_special_chars[] = "%$-+(,)*._~/<>|&^!:[@]";

/* md_assemble() always leaves the strings it's passed unaltered.  To
   effect this we maintain a stack of saved characters that we've smashed
   with '\0's (indicating end of strings for various sub-fields of the
   assembler instruction).  */
static char save_stack[32];
static char *save_stack_p;
#define END_STRING_AND_SAVE(s) \
	do { *save_stack_p++ = *(s); *(s) = '\0'; } while (0)
#define RESTORE_END_STRING(s) \
	do { *(s) = *--save_stack_p; } while (0)

/* The instruction we're assembling.  */
static i386_insn i;

/* Possible templates for current insn.  */
static const templates *current_templates;

/* Per instruction expressionS buffers: max displacements & immediates.  */
static expressionS disp_expressions[MAX_MEMORY_OPERANDS];
static expressionS im_expressions[MAX_IMMEDIATE_OPERANDS];

/* Current operand we are working on.  */
static int this_operand = -1;

/* We support four different modes.  FLAG_CODE variable is used to distinguish
   these.  */

enum flag_code {
	CODE_32BIT,
	CODE_16BIT,
	CODE_64BIT };

static enum flag_code flag_code;
static unsigned int object_64bit;
static unsigned int disallow_64bit_reloc;
static int use_rela_relocations = 0;

#if ((defined (OBJ_MAYBE_COFF) && defined (OBJ_MAYBE_AOUT)) \
     || defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF) \
     || defined (TE_PE) || defined (TE_PEP) || defined (OBJ_MACH_O))

/* The ELF ABI to use.  */
enum x86_elf_abi
{
  I386_ABI,
  X86_64_ABI,
  X86_64_X32_ABI
};

static enum x86_elf_abi x86_elf_abi = I386_ABI;
#endif

#if defined (TE_PE) || defined (TE_PEP)
/* Use big object file format.  */
static int use_big_obj = 0;
#endif

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
/* 1 if generating code for a shared library.  */
static int shared = 0;
#endif

/* 1 for intel syntax,
   0 if att syntax.  */
static int intel_syntax = 0;

/* 1 for Intel64 ISA,
   0 if AMD64 ISA.  */
static int intel64;

/* 1 for intel mnemonic,
   0 if att mnemonic.  */
static int intel_mnemonic = !SYSV386_COMPAT;

/* 1 if support old (<= 2.8.1) versions of gcc.  */
static int old_gcc = OLDGCC_COMPAT;

/* 1 if pseudo registers are permitted.  */
static int allow_pseudo_reg = 0;

/* 1 if register prefix % not required.  */
static int allow_naked_reg = 0;

/* 1 if the assembler should add BND prefix for all control-transferring
   instructions supporting it, even if this prefix wasn't specified
   explicitly.  */
static int add_bnd_prefix = 0;

/* 1 if pseudo index register, eiz/riz, is allowed .  */
static int allow_index_reg = 0;

/* 1 if the assembler should ignore LOCK prefix, even if it was
   specified explicitly.  */
static int omit_lock_prefix = 0;

/* 1 if the assembler should encode lfence, mfence, and sfence as
   "lock addl $0, (%{re}sp)".  */
static int avoid_fence = 0;

/* 1 if the assembler should generate relax relocations.  */

static int generate_relax_relocations
  = DEFAULT_GENERATE_X86_RELAX_RELOCATIONS;

static enum check_kind
  {
    check_none = 0,
    check_warning,
    check_error
  }
sse_check, operand_check = check_warning;

/* Register prefix used for error message.  */
static const char *register_prefix = "%";

/* Used in 16 bit gcc mode to add an l suffix to call, ret, enter,
   leave, push, and pop instructions so that gcc has the same stack
   frame as in 32 bit mode.  */
static char stackop_size = '\0';

/* Non-zero to optimize code alignment.  */
int optimize_align_code = 1;

/* Non-zero to quieten some warnings.  */
static int quiet_warnings = 0;

/* CPU name.  */
static const char *cpu_arch_name = NULL;
static char *cpu_sub_arch_name = NULL;

/* CPU feature flags.  */
static i386_cpu_flags cpu_arch_flags = CPU_UNKNOWN_FLAGS;

/* If we have selected a cpu we are generating instructions for.  */
static int cpu_arch_tune_set = 0;

/* Cpu we are generating instructions for.  */
enum processor_type cpu_arch_tune = PROCESSOR_UNKNOWN;

/* CPU feature flags of cpu we are generating instructions for.  */
static i386_cpu_flags cpu_arch_tune_flags;

/* CPU instruction set architecture used.  */
enum processor_type cpu_arch_isa = PROCESSOR_UNKNOWN;

/* CPU feature flags of instruction set architecture used.  */
i386_cpu_flags cpu_arch_isa_flags;

/* If set, conditional jumps are not automatically promoted to handle
   larger than a byte offset.  */
static unsigned int no_cond_jump_promotion = 0;

/* Encode SSE instructions with VEX prefix.  */
static unsigned int sse2avx;

/* Encode scalar AVX instructions with specific vector length.  */
static enum
  {
    vex128 = 0,
    vex256
  } avxscalar;

/* Encode scalar EVEX LIG instructions with specific vector length.  */
static enum
  {
    evexl128 = 0,
    evexl256,
    evexl512
  } evexlig;

/* Encode EVEX WIG instructions with specific evex.w.  */
static enum
  {
    evexw0 = 0,
    evexw1
  } evexwig;

/* Value to encode in EVEX RC bits, for SAE-only instructions.  */
static enum rc_type evexrcig = rne;

/* Pre-defined "_GLOBAL_OFFSET_TABLE_".  */
static symbolS *GOT_symbol;

/* The dwarf2 return column, adjusted for 32 or 64 bit.  */
unsigned int x86_dwarf2_return_column;

/* The dwarf2 data alignment, adjusted for 32 or 64 bit.  */
int x86_cie_data_alignment;

/* Interface to relax_segment.
   There are 3 major relax states for 386 jump insns because the
   different types of jumps add different sizes to frags when we're
   figuring out what sort of jump to choose to reach a given label.  */

/* Types.  */
#define UNCOND_JUMP 0
#define COND_JUMP 1
#define COND_JUMP86 2

/* Sizes.  */
#define CODE16	1
#define SMALL	0
#define SMALL16 (SMALL | CODE16)
#define BIG	2
#define BIG16	(BIG | CODE16)

#ifndef INLINE
#ifdef __GNUC__
#define INLINE __inline__
#else
#define INLINE
#endif
#endif

#define ENCODE_RELAX_STATE(type, size) \
  ((relax_substateT) (((type) << 2) | (size)))
#define TYPE_FROM_RELAX_STATE(s) \
  ((s) >> 2)
#define DISP_SIZE_FROM_RELAX_STATE(s) \
    ((((s) & 3) == BIG ? 4 : (((s) & 3) == BIG16 ? 2 : 1)))

/* This table is used by relax_frag to promote short jumps to long
   ones where necessary.  SMALL (short) jumps may be promoted to BIG
   (32 bit long) ones, and SMALL16 jumps to BIG16 (16 bit long).  We
   don't allow a short jump in a 32 bit code segment to be promoted to
   a 16 bit offset jump because it's slower (requires data size
   prefix), and doesn't work, unless the destination is in the bottom
   64k of the code segment (The top 16 bits of eip are zeroed).  */

const relax_typeS md_relax_table[] =
{
  /* The fields are:
     1) most positive reach of this state,
     2) most negative reach of this state,
     3) how many bytes this mode will have in the variable part of the frag
     4) which index into the table to try if we can't fit into this one.  */

  /* UNCOND_JUMP states.  */
  {127 + 1, -128 + 1, 1, ENCODE_RELAX_STATE (UNCOND_JUMP, BIG)},
  {127 + 1, -128 + 1, 1, ENCODE_RELAX_STATE (UNCOND_JUMP, BIG16)},
  /* dword jmp adds 4 bytes to frag:
     0 extra opcode bytes, 4 displacement bytes.  */
  {0, 0, 4, 0},
  /* word jmp adds 2 byte2 to frag:
     0 extra opcode bytes, 2 displacement bytes.  */
  {0, 0, 2, 0},

  /* COND_JUMP states.  */
  {127 + 1, -128 + 1, 1, ENCODE_RELAX_STATE (COND_JUMP, BIG)},
  {127 + 1, -128 + 1, 1, ENCODE_RELAX_STATE (COND_JUMP, BIG16)},
  /* dword conditionals adds 5 bytes to frag:
     1 extra opcode byte, 4 displacement bytes.  */
  {0, 0, 5, 0},
  /* word conditionals add 3 bytes to frag:
     1 extra opcode byte, 2 displacement bytes.  */
  {0, 0, 3, 0},

  /* COND_JUMP86 states.  */
  {127 + 1, -128 + 1, 1, ENCODE_RELAX_STATE (COND_JUMP86, BIG)},
  {127 + 1, -128 + 1, 1, ENCODE_RELAX_STATE (COND_JUMP86, BIG16)},
  /* dword conditionals adds 5 bytes to frag:
     1 extra opcode byte, 4 displacement bytes.  */
  {0, 0, 5, 0},
  /* word conditionals add 4 bytes to frag:
     1 displacement byte and a 3 byte long branch insn.  */
  {0, 0, 4, 0}
};

static const arch_entry cpu_arch[] =
{
  /* Do not replace the first two entries - i386_target_format()
     relies on them being there in this order.  */
  { STRING_COMMA_LEN ("generic32"), PROCESSOR_GENERIC32,
    CPU_GENERIC32_FLAGS, 0 },
  { STRING_COMMA_LEN ("generic64"), PROCESSOR_GENERIC64,
    CPU_GENERIC64_FLAGS, 0 },
  { STRING_COMMA_LEN ("i8086"), PROCESSOR_UNKNOWN,
    CPU_NONE_FLAGS, 0 },
  { STRING_COMMA_LEN ("i186"), PROCESSOR_UNKNOWN,
    CPU_I186_FLAGS, 0 },
  { STRING_COMMA_LEN ("i286"), PROCESSOR_UNKNOWN,
    CPU_I286_FLAGS, 0 },
  { STRING_COMMA_LEN ("i386"), PROCESSOR_I386,
    CPU_I386_FLAGS, 0 },
  { STRING_COMMA_LEN ("i486"), PROCESSOR_I486,
    CPU_I486_FLAGS, 0 },
  { STRING_COMMA_LEN ("i586"), PROCESSOR_PENTIUM,
    CPU_I586_FLAGS, 0 },
  { STRING_COMMA_LEN ("i686"), PROCESSOR_PENTIUMPRO,
    CPU_I686_FLAGS, 0 },
  { STRING_COMMA_LEN ("pentium"), PROCESSOR_PENTIUM,
    CPU_I586_FLAGS, 0 },
  { STRING_COMMA_LEN ("pentiumpro"), PROCESSOR_PENTIUMPRO,
    CPU_PENTIUMPRO_FLAGS, 0 },
  { STRING_COMMA_LEN ("pentiumii"), PROCESSOR_PENTIUMPRO,
    CPU_P2_FLAGS, 0 },
  { STRING_COMMA_LEN ("pentiumiii"),PROCESSOR_PENTIUMPRO,
    CPU_P3_FLAGS, 0 },
  { STRING_COMMA_LEN ("pentium4"), PROCESSOR_PENTIUM4,
    CPU_P4_FLAGS, 0 },
  { STRING_COMMA_LEN ("prescott"), PROCESSOR_NOCONA,
    CPU_CORE_FLAGS, 0 },
  { STRING_COMMA_LEN ("nocona"), PROCESSOR_NOCONA,
    CPU_NOCONA_FLAGS, 0 },
  { STRING_COMMA_LEN ("yonah"), PROCESSOR_CORE,
    CPU_CORE_FLAGS, 1 },
  { STRING_COMMA_LEN ("core"), PROCESSOR_CORE,
    CPU_CORE_FLAGS, 0 },
  { STRING_COMMA_LEN ("merom"), PROCESSOR_CORE2,
    CPU_CORE2_FLAGS, 1 },
  { STRING_COMMA_LEN ("core2"), PROCESSOR_CORE2,
    CPU_CORE2_FLAGS, 0 },
  { STRING_COMMA_LEN ("corei7"), PROCESSOR_COREI7,
    CPU_COREI7_FLAGS, 0 },
  { STRING_COMMA_LEN ("l1om"), PROCESSOR_L1OM,
    CPU_L1OM_FLAGS, 0 },
  { STRING_COMMA_LEN ("k1om"), PROCESSOR_K1OM,
    CPU_K1OM_FLAGS, 0 },
  { STRING_COMMA_LEN ("iamcu"), PROCESSOR_IAMCU,
    CPU_IAMCU_FLAGS, 0 },
  { STRING_COMMA_LEN ("k6"), PROCESSOR_K6,
    CPU_K6_FLAGS, 0 },
  { STRING_COMMA_LEN ("k6_2"), PROCESSOR_K6,
    CPU_K6_2_FLAGS, 0 },
  { STRING_COMMA_LEN ("athlon"), PROCESSOR_ATHLON,
    CPU_ATHLON_FLAGS, 0 },
  { STRING_COMMA_LEN ("sledgehammer"), PROCESSOR_K8,
    CPU_K8_FLAGS, 1 },
  { STRING_COMMA_LEN ("opteron"), PROCESSOR_K8,
    CPU_K8_FLAGS, 0 },
  { STRING_COMMA_LEN ("k8"), PROCESSOR_K8,
    CPU_K8_FLAGS, 0 },
  { STRING_COMMA_LEN ("amdfam10"), PROCESSOR_AMDFAM10,
    CPU_AMDFAM10_FLAGS, 0 },
  { STRING_COMMA_LEN ("bdver1"), PROCESSOR_BD,
    CPU_BDVER1_FLAGS, 0 },
  { STRING_COMMA_LEN ("bdver2"), PROCESSOR_BD,
    CPU_BDVER2_FLAGS, 0 },
  { STRING_COMMA_LEN ("bdver3"), PROCESSOR_BD,
    CPU_BDVER3_FLAGS, 0 },
  { STRING_COMMA_LEN ("bdver4"), PROCESSOR_BD,
    CPU_BDVER4_FLAGS, 0 },
  { STRING_COMMA_LEN ("znver1"), PROCESSOR_ZNVER,
    CPU_ZNVER1_FLAGS, 0 },
  { STRING_COMMA_LEN ("btver1"), PROCESSOR_BT,
    CPU_BTVER1_FLAGS, 0 },
  { STRING_COMMA_LEN ("btver2"), PROCESSOR_BT,
    CPU_BTVER2_FLAGS, 0 },
  { STRING_COMMA_LEN (".8087"), PROCESSOR_UNKNOWN,
    CPU_8087_FLAGS, 0 },
  { STRING_COMMA_LEN (".287"), PROCESSOR_UNKNOWN,
    CPU_287_FLAGS, 0 },
  { STRING_COMMA_LEN (".387"), PROCESSOR_UNKNOWN,
    CPU_387_FLAGS, 0 },
  { STRING_COMMA_LEN (".687"), PROCESSOR_UNKNOWN,
    CPU_687_FLAGS, 0 },
  { STRING_COMMA_LEN (".mmx"), PROCESSOR_UNKNOWN,
    CPU_MMX_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse"), PROCESSOR_UNKNOWN,
    CPU_SSE_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse2"), PROCESSOR_UNKNOWN,
    CPU_SSE2_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse3"), PROCESSOR_UNKNOWN,
    CPU_SSE3_FLAGS, 0 },
  { STRING_COMMA_LEN (".ssse3"), PROCESSOR_UNKNOWN,
    CPU_SSSE3_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse4.1"), PROCESSOR_UNKNOWN,
    CPU_SSE4_1_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse4.2"), PROCESSOR_UNKNOWN,
    CPU_SSE4_2_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse4"), PROCESSOR_UNKNOWN,
    CPU_SSE4_2_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx"), PROCESSOR_UNKNOWN,
    CPU_AVX_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx2"), PROCESSOR_UNKNOWN,
    CPU_AVX2_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512f"), PROCESSOR_UNKNOWN,
    CPU_AVX512F_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512cd"), PROCESSOR_UNKNOWN,
    CPU_AVX512CD_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512er"), PROCESSOR_UNKNOWN,
    CPU_AVX512ER_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512pf"), PROCESSOR_UNKNOWN,
    CPU_AVX512PF_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512dq"), PROCESSOR_UNKNOWN,
    CPU_AVX512DQ_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512bw"), PROCESSOR_UNKNOWN,
    CPU_AVX512BW_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512vl"), PROCESSOR_UNKNOWN,
    CPU_AVX512VL_FLAGS, 0 },
  { STRING_COMMA_LEN (".vmx"), PROCESSOR_UNKNOWN,
    CPU_VMX_FLAGS, 0 },
  { STRING_COMMA_LEN (".vmfunc"), PROCESSOR_UNKNOWN,
    CPU_VMFUNC_FLAGS, 0 },
  { STRING_COMMA_LEN (".smx"), PROCESSOR_UNKNOWN,
    CPU_SMX_FLAGS, 0 },
  { STRING_COMMA_LEN (".xsave"), PROCESSOR_UNKNOWN,
    CPU_XSAVE_FLAGS, 0 },
  { STRING_COMMA_LEN (".xsaveopt"), PROCESSOR_UNKNOWN,
    CPU_XSAVEOPT_FLAGS, 0 },
  { STRING_COMMA_LEN (".xsavec"), PROCESSOR_UNKNOWN,
    CPU_XSAVEC_FLAGS, 0 },
  { STRING_COMMA_LEN (".xsaves"), PROCESSOR_UNKNOWN,
    CPU_XSAVES_FLAGS, 0 },
  { STRING_COMMA_LEN (".aes"), PROCESSOR_UNKNOWN,
    CPU_AES_FLAGS, 0 },
  { STRING_COMMA_LEN (".pclmul"), PROCESSOR_UNKNOWN,
    CPU_PCLMUL_FLAGS, 0 },
  { STRING_COMMA_LEN (".clmul"), PROCESSOR_UNKNOWN,
    CPU_PCLMUL_FLAGS, 1 },
  { STRING_COMMA_LEN (".fsgsbase"), PROCESSOR_UNKNOWN,
    CPU_FSGSBASE_FLAGS, 0 },
  { STRING_COMMA_LEN (".rdrnd"), PROCESSOR_UNKNOWN,
    CPU_RDRND_FLAGS, 0 },
  { STRING_COMMA_LEN (".f16c"), PROCESSOR_UNKNOWN,
    CPU_F16C_FLAGS, 0 },
  { STRING_COMMA_LEN (".bmi2"), PROCESSOR_UNKNOWN,
    CPU_BMI2_FLAGS, 0 },
  { STRING_COMMA_LEN (".fma"), PROCESSOR_UNKNOWN,
    CPU_FMA_FLAGS, 0 },
  { STRING_COMMA_LEN (".fma4"), PROCESSOR_UNKNOWN,
    CPU_FMA4_FLAGS, 0 },
  { STRING_COMMA_LEN (".xop"), PROCESSOR_UNKNOWN,
    CPU_XOP_FLAGS, 0 },
  { STRING_COMMA_LEN (".lwp"), PROCESSOR_UNKNOWN,
    CPU_LWP_FLAGS, 0 },
  { STRING_COMMA_LEN (".movbe"), PROCESSOR_UNKNOWN,
    CPU_MOVBE_FLAGS, 0 },
  { STRING_COMMA_LEN (".cx16"), PROCESSOR_UNKNOWN,
    CPU_CX16_FLAGS, 0 },
  { STRING_COMMA_LEN (".ept"), PROCESSOR_UNKNOWN,
    CPU_EPT_FLAGS, 0 },
  { STRING_COMMA_LEN (".lzcnt"), PROCESSOR_UNKNOWN,
    CPU_LZCNT_FLAGS, 0 },
  { STRING_COMMA_LEN (".hle"), PROCESSOR_UNKNOWN,
    CPU_HLE_FLAGS, 0 },
  { STRING_COMMA_LEN (".rtm"), PROCESSOR_UNKNOWN,
    CPU_RTM_FLAGS, 0 },
  { STRING_COMMA_LEN (".invpcid"), PROCESSOR_UNKNOWN,
    CPU_INVPCID_FLAGS, 0 },
  { STRING_COMMA_LEN (".clflush"), PROCESSOR_UNKNOWN,
    CPU_CLFLUSH_FLAGS, 0 },
  { STRING_COMMA_LEN (".nop"), PROCESSOR_UNKNOWN,
    CPU_NOP_FLAGS, 0 },
  { STRING_COMMA_LEN (".syscall"), PROCESSOR_UNKNOWN,
    CPU_SYSCALL_FLAGS, 0 },
  { STRING_COMMA_LEN (".rdtscp"), PROCESSOR_UNKNOWN,
    CPU_RDTSCP_FLAGS, 0 },
  { STRING_COMMA_LEN (".3dnow"), PROCESSOR_UNKNOWN,
    CPU_3DNOW_FLAGS, 0 },
  { STRING_COMMA_LEN (".3dnowa"), PROCESSOR_UNKNOWN,
    CPU_3DNOWA_FLAGS, 0 },
  { STRING_COMMA_LEN (".padlock"), PROCESSOR_UNKNOWN,
    CPU_PADLOCK_FLAGS, 0 },
  { STRING_COMMA_LEN (".pacifica"), PROCESSOR_UNKNOWN,
    CPU_SVME_FLAGS, 1 },
  { STRING_COMMA_LEN (".svme"), PROCESSOR_UNKNOWN,
    CPU_SVME_FLAGS, 0 },
  { STRING_COMMA_LEN (".sse4a"), PROCESSOR_UNKNOWN,
    CPU_SSE4A_FLAGS, 0 },
  { STRING_COMMA_LEN (".abm"), PROCESSOR_UNKNOWN,
    CPU_ABM_FLAGS, 0 },
  { STRING_COMMA_LEN (".bmi"), PROCESSOR_UNKNOWN,
    CPU_BMI_FLAGS, 0 },
  { STRING_COMMA_LEN (".tbm"), PROCESSOR_UNKNOWN,
    CPU_TBM_FLAGS, 0 },
  { STRING_COMMA_LEN (".adx"), PROCESSOR_UNKNOWN,
    CPU_ADX_FLAGS, 0 },
  { STRING_COMMA_LEN (".rdseed"), PROCESSOR_UNKNOWN,
    CPU_RDSEED_FLAGS, 0 },
  { STRING_COMMA_LEN (".prfchw"), PROCESSOR_UNKNOWN,
    CPU_PRFCHW_FLAGS, 0 },
  { STRING_COMMA_LEN (".smap"), PROCESSOR_UNKNOWN,
    CPU_SMAP_FLAGS, 0 },
  { STRING_COMMA_LEN (".mpx"), PROCESSOR_UNKNOWN,
    CPU_MPX_FLAGS, 0 },
  { STRING_COMMA_LEN (".sha"), PROCESSOR_UNKNOWN,
    CPU_SHA_FLAGS, 0 },
  { STRING_COMMA_LEN (".clflushopt"), PROCESSOR_UNKNOWN,
    CPU_CLFLUSHOPT_FLAGS, 0 },
  { STRING_COMMA_LEN (".prefetchwt1"), PROCESSOR_UNKNOWN,
    CPU_PREFETCHWT1_FLAGS, 0 },
  { STRING_COMMA_LEN (".se1"), PROCESSOR_UNKNOWN,
    CPU_SE1_FLAGS, 0 },
  { STRING_COMMA_LEN (".clwb"), PROCESSOR_UNKNOWN,
    CPU_CLWB_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512ifma"), PROCESSOR_UNKNOWN,
    CPU_AVX512IFMA_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512vbmi"), PROCESSOR_UNKNOWN,
    CPU_AVX512VBMI_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512_4fmaps"), PROCESSOR_UNKNOWN,
    CPU_AVX512_4FMAPS_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512_4vnniw"), PROCESSOR_UNKNOWN,
    CPU_AVX512_4VNNIW_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512_vpopcntdq"), PROCESSOR_UNKNOWN,
    CPU_AVX512_VPOPCNTDQ_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512_vbmi2"), PROCESSOR_UNKNOWN,
    CPU_AVX512_VBMI2_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512_vnni"), PROCESSOR_UNKNOWN,
    CPU_AVX512_VNNI_FLAGS, 0 },
  { STRING_COMMA_LEN (".avx512_bitalg"), PROCESSOR_UNKNOWN,
    CPU_AVX512_BITALG_FLAGS, 0 },
  { STRING_COMMA_LEN (".clzero"), PROCESSOR_UNKNOWN,
    CPU_CLZERO_FLAGS, 0 },
  { STRING_COMMA_LEN (".mwaitx"), PROCESSOR_UNKNOWN,
    CPU_MWAITX_FLAGS, 0 },
  { STRING_COMMA_LEN (".ospke"), PROCESSOR_UNKNOWN,
    CPU_OSPKE_FLAGS, 0 },
  { STRING_COMMA_LEN (".rdpid"), PROCESSOR_UNKNOWN,
    CPU_RDPID_FLAGS, 0 },
  { STRING_COMMA_LEN (".ptwrite"), PROCESSOR_UNKNOWN,
    CPU_PTWRITE_FLAGS, 0 },
  { STRING_COMMA_LEN (".cet"), PROCESSOR_UNKNOWN,
    CPU_CET_FLAGS, 0 },
  { STRING_COMMA_LEN (".gfni"), PROCESSOR_UNKNOWN,
    CPU_GFNI_FLAGS, 0 },
  { STRING_COMMA_LEN (".vaes"), PROCESSOR_UNKNOWN,
    CPU_VAES_FLAGS, 0 },
  { STRING_COMMA_LEN (".vpclmulqdq"), PROCESSOR_UNKNOWN,
    CPU_VPCLMULQDQ_FLAGS, 0 },
};

static const noarch_entry cpu_noarch[] =
{
  { STRING_COMMA_LEN ("no87"),  CPU_ANY_X87_FLAGS },
  { STRING_COMMA_LEN ("no287"),  CPU_ANY_287_FLAGS },
  { STRING_COMMA_LEN ("no387"),  CPU_ANY_387_FLAGS },
  { STRING_COMMA_LEN ("no687"),  CPU_ANY_687_FLAGS },
  { STRING_COMMA_LEN ("nommx"),  CPU_ANY_MMX_FLAGS },
  { STRING_COMMA_LEN ("nosse"),  CPU_ANY_SSE_FLAGS },
  { STRING_COMMA_LEN ("nosse2"),  CPU_ANY_SSE2_FLAGS },
  { STRING_COMMA_LEN ("nosse3"),  CPU_ANY_SSE3_FLAGS },
  { STRING_COMMA_LEN ("nossse3"),  CPU_ANY_SSSE3_FLAGS },
  { STRING_COMMA_LEN ("nosse4.1"),  CPU_ANY_SSE4_1_FLAGS },
  { STRING_COMMA_LEN ("nosse4.2"),  CPU_ANY_SSE4_2_FLAGS },
  { STRING_COMMA_LEN ("nosse4"),  CPU_ANY_SSE4_1_FLAGS },
  { STRING_COMMA_LEN ("noavx"),  CPU_ANY_AVX_FLAGS },
  { STRING_COMMA_LEN ("noavx2"),  CPU_ANY_AVX2_FLAGS },
  { STRING_COMMA_LEN ("noavx512f"), CPU_ANY_AVX512F_FLAGS },
  { STRING_COMMA_LEN ("noavx512cd"), CPU_ANY_AVX512CD_FLAGS },
  { STRING_COMMA_LEN ("noavx512er"), CPU_ANY_AVX512ER_FLAGS },
  { STRING_COMMA_LEN ("noavx512pf"), CPU_ANY_AVX512PF_FLAGS },
  { STRING_COMMA_LEN ("noavx512dq"), CPU_ANY_AVX512DQ_FLAGS },
  { STRING_COMMA_LEN ("noavx512bw"), CPU_ANY_AVX512BW_FLAGS },
  { STRING_COMMA_LEN ("noavx512vl"), CPU_ANY_AVX512VL_FLAGS },
  { STRING_COMMA_LEN ("noavx512ifma"), CPU_ANY_AVX512IFMA_FLAGS },
  { STRING_COMMA_LEN ("noavx512vbmi"), CPU_ANY_AVX512VBMI_FLAGS },
  { STRING_COMMA_LEN ("noavx512_4fmaps"), CPU_ANY_AVX512_4FMAPS_FLAGS },
  { STRING_COMMA_LEN ("noavx512_4vnniw"), CPU_ANY_AVX512_4VNNIW_FLAGS },
  { STRING_COMMA_LEN ("noavx512_vpopcntdq"), CPU_ANY_AVX512_VPOPCNTDQ_FLAGS },
  { STRING_COMMA_LEN ("noavx512_vbmi2"), CPU_ANY_AVX512_VBMI2_FLAGS },
  { STRING_COMMA_LEN ("noavx512_vnni"), CPU_ANY_AVX512_VNNI_FLAGS },
  { STRING_COMMA_LEN ("noavx512_bitalg"), CPU_ANY_AVX512_BITALG_FLAGS },
};

#ifdef I386COFF
/* Like s_lcomm_internal in gas/read.c but the alignment string
   is allowed to be optional.  */

static symbolS *
pe_lcomm_internal (int needs_align, symbolS *symbolP, addressT size)
{
  addressT align = 0;

  SKIP_WHITESPACE ();

  if (needs_align
      && *input_line_pointer == ',')
    {
      align = parse_align (needs_align - 1);

      if (align == (addressT) -1)
	return NULL;
    }
  else
    {
      if (size >= 8)
	align = 3;
      else if (size >= 4)
	align = 2;
      else if (size >= 2)
	align = 1;
      else
	align = 0;
    }

  bss_alloc (symbolP, size, align);
  return symbolP;
}

static void
pe_lcomm (int needs_align)
{
  s_comm_internal (needs_align * 2, pe_lcomm_internal);
}
#endif

const pseudo_typeS md_pseudo_table[] =
{
#if !defined(OBJ_AOUT) && !defined(USE_ALIGN_PTWO)
  {"align", s_align_bytes, 0},
#else
  {"align", s_align_ptwo, 0},
#endif
  {"arch", set_cpu_arch, 0},
#ifndef I386COFF
  {"bss", s_bss, 0},
#else
  {"lcomm", pe_lcomm, 1},
#endif
  {"ffloat", float_cons, 'f'},
  {"dfloat", float_cons, 'd'},
  {"tfloat", float_cons, 'x'},
  {"value", cons, 2},
  {"slong", signed_cons, 4},
  {"noopt", s_ignore, 0},
  {"optim", s_ignore, 0},
  {"code16gcc", set_16bit_gcc_code_flag, CODE_16BIT},
  {"code16", set_code_flag, CODE_16BIT},
  {"code32", set_code_flag, CODE_32BIT},
#ifdef BFD64
  {"code64", set_code_flag, CODE_64BIT},
#endif
  {"intel_syntax", set_intel_syntax, 1},
  {"att_syntax", set_intel_syntax, 0},
  {"intel_mnemonic", set_intel_mnemonic, 1},
  {"att_mnemonic", set_intel_mnemonic, 0},
  {"allow_index_reg", set_allow_index_reg, 1},
  {"disallow_index_reg", set_allow_index_reg, 0},
  {"sse_check", set_check, 0},
  {"operand_check", set_check, 1},
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  {"largecomm", handle_large_common, 0},
#else
  {"file", (void (*) (int)) dwarf2_directive_file, 0},
  {"loc", dwarf2_directive_loc, 0},
  {"loc_mark_labels", dwarf2_directive_loc_mark_labels, 0},
#endif
#ifdef TE_PE
  {"secrel32", pe_directive_secrel, 0},
#endif
  {0, 0, 0}
};

/* For interface with expression ().  */
extern char *input_line_pointer;

/* Hash table for instruction mnemonic lookup.  */
static struct hash_control *op_hash;

/* Hash table for register lookup.  */
static struct hash_control *reg_hash;

void
i386_align_code (fragS *fragP, int count)
{
  /* Various efficient no-op patterns for aligning code labels.
     Note: Don't try to assemble the instructions in the comments.
     0L and 0w are not legal.  */
  static const unsigned char f32_1[] =
    {0x90};					/* nop			*/
  static const unsigned char f32_2[] =
    {0x66,0x90};				/* xchg %ax,%ax */
  static const unsigned char f32_3[] =
    {0x8d,0x76,0x00};				/* leal 0(%esi),%esi	*/
  static const unsigned char f32_4[] =
    {0x8d,0x74,0x26,0x00};			/* leal 0(%esi,1),%esi	*/
  static const unsigned char f32_5[] =
    {0x90,					/* nop			*/
     0x8d,0x74,0x26,0x00};			/* leal 0(%esi,1),%esi	*/
  static const unsigned char f32_6[] =
    {0x8d,0xb6,0x00,0x00,0x00,0x00};		/* leal 0L(%esi),%esi	*/
  static const unsigned char f32_7[] =
    {0x8d,0xb4,0x26,0x00,0x00,0x00,0x00};	/* leal 0L(%esi,1),%esi */
  static const unsigned char f32_8[] =
    {0x90,					/* nop			*/
     0x8d,0xb4,0x26,0x00,0x00,0x00,0x00};	/* leal 0L(%esi,1),%esi */
  static const unsigned char f32_9[] =
    {0x89,0xf6,					/* movl %esi,%esi	*/
     0x8d,0xbc,0x27,0x00,0x00,0x00,0x00};	/* leal 0L(%edi,1),%edi */
  static const unsigned char f32_10[] =
    {0x8d,0x76,0x00,				/* leal 0(%esi),%esi	*/
     0x8d,0xbc,0x27,0x00,0x00,0x00,0x00};	/* leal 0L(%edi,1),%edi */
  static const unsigned char f32_11[] =
    {0x8d,0x74,0x26,0x00,			/* leal 0(%esi,1),%esi	*/
     0x8d,0xbc,0x27,0x00,0x00,0x00,0x00};	/* leal 0L(%edi,1),%edi */
  static const unsigned char f32_12[] =
    {0x8d,0xb6,0x00,0x00,0x00,0x00,		/* leal 0L(%esi),%esi	*/
     0x8d,0xbf,0x00,0x00,0x00,0x00};		/* leal 0L(%edi),%edi	*/
  static const unsigned char f32_13[] =
    {0x8d,0xb6,0x00,0x00,0x00,0x00,		/* leal 0L(%esi),%esi	*/
     0x8d,0xbc,0x27,0x00,0x00,0x00,0x00};	/* leal 0L(%edi,1),%edi */
  static const unsigned char f32_14[] =
    {0x8d,0xb4,0x26,0x00,0x00,0x00,0x00,	/* leal 0L(%esi,1),%esi */
     0x8d,0xbc,0x27,0x00,0x00,0x00,0x00};	/* leal 0L(%edi,1),%edi */
  static const unsigned char f16_3[] =
    {0x8d,0x74,0x00};				/* lea 0(%esi),%esi	*/
  static const unsigned char f16_4[] =
    {0x8d,0xb4,0x00,0x00};			/* lea 0w(%si),%si	*/
  static const unsigned char f16_5[] =
    {0x90,					/* nop			*/
     0x8d,0xb4,0x00,0x00};			/* lea 0w(%si),%si	*/
  static const unsigned char f16_6[] =
    {0x89,0xf6,					/* mov %si,%si		*/
     0x8d,0xbd,0x00,0x00};			/* lea 0w(%di),%di	*/
  static const unsigned char f16_7[] =
    {0x8d,0x74,0x00,				/* lea 0(%si),%si	*/
     0x8d,0xbd,0x00,0x00};			/* lea 0w(%di),%di	*/
  static const unsigned char f16_8[] =
    {0x8d,0xb4,0x00,0x00,			/* lea 0w(%si),%si	*/
     0x8d,0xbd,0x00,0x00};			/* lea 0w(%di),%di	*/
  static const unsigned char jump_31[] =
    {0xeb,0x1d,0x90,0x90,0x90,0x90,0x90,	/* jmp .+31; lotsa nops	*/
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
     0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
  static const unsigned char *const f32_patt[] = {
    f32_1, f32_2, f32_3, f32_4, f32_5, f32_6, f32_7, f32_8,
    f32_9, f32_10, f32_11, f32_12, f32_13, f32_14
  };
  static const unsigned char *const f16_patt[] = {
    f32_1, f32_2, f16_3, f16_4, f16_5, f16_6, f16_7, f16_8
  };
  /* nopl (%[re]ax) */
  static const unsigned char alt_3[] =
    {0x0f,0x1f,0x00};
  /* nopl 0(%[re]ax) */
  static const unsigned char alt_4[] =
    {0x0f,0x1f,0x40,0x00};
  /* nopl 0(%[re]ax,%[re]ax,1) */
  static const unsigned char alt_5[] =
    {0x0f,0x1f,0x44,0x00,0x00};
  /* nopw 0(%[re]ax,%[re]ax,1) */
  static const unsigned char alt_6[] =
    {0x66,0x0f,0x1f,0x44,0x00,0x00};
  /* nopl 0L(%[re]ax) */
  static const unsigned char alt_7[] =
    {0x0f,0x1f,0x80,0x00,0x00,0x00,0x00};
  /* nopl 0L(%[re]ax,%[re]ax,1) */
  static const unsigned char alt_8[] =
    {0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00};
  /* nopw 0L(%[re]ax,%[re]ax,1) */
  static const unsigned char alt_9[] =
    {0x66,0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00};
  /* nopw %cs:0L(%[re]ax,%[re]ax,1) */
  static const unsigned char alt_10[] =
    {0x66,0x2e,0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00};
  static const unsigned char *const alt_patt[] = {
    f32_1, f32_2, alt_3, alt_4, alt_5, alt_6, alt_7, alt_8,
    alt_9, alt_10
  };

  /* Only align for at least a positive non-zero boundary. */
  if (count <= 0 || count > MAX_MEM_FOR_RS_ALIGN_CODE)
    return;

  /* We need to decide which NOP sequence to use for 32bit and
     64bit. When -mtune= is used:

     1. For PROCESSOR_I386, PROCESSOR_I486, PROCESSOR_PENTIUM and
     PROCESSOR_GENERIC32, f32_patt will be used.
     2. For the rest, alt_patt will be used.

     When -mtune= isn't used, alt_patt will be used if
     cpu_arch_isa_flags has CpuNop.  Otherwise, f32_patt will
     be used.

     When -march= or .arch is used, we can't use anything beyond
     cpu_arch_isa_flags.   */

  if (flag_code == CODE_16BIT)
    {
      if (count > 8)
	{
	  memcpy (fragP->fr_literal + fragP->fr_fix,
		  jump_31, count);
	  /* Adjust jump offset.  */
	  fragP->fr_literal[fragP->fr_fix + 1] = count - 2;
	}
      else
	memcpy (fragP->fr_literal + fragP->fr_fix,
		f16_patt[count - 1], count);
    }
  else
    {
      const unsigned char *const *patt = NULL;

      if (fragP->tc_frag_data.isa == PROCESSOR_UNKNOWN)
	{
	  /* PROCESSOR_UNKNOWN means that all ISAs may be used.  */
	  switch (cpu_arch_tune)
	    {
	    case PROCESSOR_UNKNOWN:
	      /* We use cpu_arch_isa_flags to check if we SHOULD
		 optimize with nops.  */
	      if (fragP->tc_frag_data.isa_flags.bitfield.cpunop)
		patt = alt_patt;
	      else
		patt = f32_patt;
	      break;
	    case PROCESSOR_PENTIUM4:
	    case PROCESSOR_NOCONA:
	    case PROCESSOR_CORE:
	    case PROCESSOR_CORE2:
	    case PROCESSOR_COREI7:
	    case PROCESSOR_L1OM:
	    case PROCESSOR_K1OM:
	    case PROCESSOR_GENERIC64:
	    case PROCESSOR_K6:
	    case PROCESSOR_ATHLON:
	    case PROCESSOR_K8:
	    case PROCESSOR_AMDFAM10:
	    case PROCESSOR_BD:
	    case PROCESSOR_ZNVER:
	    case PROCESSOR_BT:
	      patt = alt_patt;
	      break;
	    case PROCESSOR_I386:
	    case PROCESSOR_I486:
	    case PROCESSOR_PENTIUM:
	    case PROCESSOR_PENTIUMPRO:
	    case PROCESSOR_IAMCU:
	    case PROCESSOR_GENERIC32:
	      patt = f32_patt;
	      break;
	    }
	}
      else
	{
	  switch (fragP->tc_frag_data.tune)
	    {
	    case PROCESSOR_UNKNOWN:
	      /* When cpu_arch_isa is set, cpu_arch_tune shouldn't be
		 PROCESSOR_UNKNOWN.  */
	      abort ();
	      break;

	    case PROCESSOR_I386:
	    case PROCESSOR_I486:
	    case PROCESSOR_PENTIUM:
	    case PROCESSOR_IAMCU:
	    case PROCESSOR_K6:
	    case PROCESSOR_ATHLON:
	    case PROCESSOR_K8:
	    case PROCESSOR_AMDFAM10:
	    case PROCESSOR_BD:
	    case PROCESSOR_ZNVER:
	    case PROCESSOR_BT:
	    case PROCESSOR_GENERIC32:
	      /* We use cpu_arch_isa_flags to check if we CAN optimize
		 with nops.  */
	      if (fragP->tc_frag_data.isa_flags.bitfield.cpunop)
		patt = alt_patt;
	      else
		patt = f32_patt;
	      break;
	    case PROCESSOR_PENTIUMPRO:
	    case PROCESSOR_PENTIUM4:
	    case PROCESSOR_NOCONA:
	    case PROCESSOR_CORE:
	    case PROCESSOR_CORE2:
	    case PROCESSOR_COREI7:
	    case PROCESSOR_L1OM:
	    case PROCESSOR_K1OM:
	      if (fragP->tc_frag_data.isa_flags.bitfield.cpunop)
		patt = alt_patt;
	      else
		patt = f32_patt;
	      break;
	    case PROCESSOR_GENERIC64:
	      patt = alt_patt;
	      break;
	    }
	}

      if (patt == f32_patt)
	{
	  /* If the padding is less than 15 bytes, we use the normal
	     ones.  Otherwise, we use a jump instruction and adjust
	     its offset.   */
	  int limit;

	  /* For 64bit, the limit is 3 bytes.  */
	  if (flag_code == CODE_64BIT
	      && fragP->tc_frag_data.isa_flags.bitfield.cpulm)
	    limit = 3;
	  else
	    limit = 15;
	  if (count < limit)
	    memcpy (fragP->fr_literal + fragP->fr_fix,
		    patt[count - 1], count);
	  else
	    {
	      memcpy (fragP->fr_literal + fragP->fr_fix,
		      jump_31, count);
	      /* Adjust jump offset.  */
	      fragP->fr_literal[fragP->fr_fix + 1] = count - 2;
	    }
	}
      else
	{
	  /* Maximum length of an instruction is 10 byte.  If the
	     padding is greater than 10 bytes and we don't use jump,
	     we have to break it into smaller pieces.  */
	  int padding = count;
	  while (padding > 10)
	    {
	      padding -= 10;
	      memcpy (fragP->fr_literal + fragP->fr_fix + padding,
		      patt [9], 10);
	    }

	  if (padding)
	    memcpy (fragP->fr_literal + fragP->fr_fix,
		    patt [padding - 1], padding);
	}
    }
  fragP->fr_var = count;
}

static INLINE int
operand_type_all_zero (const union i386_operand_type *x)
{
  switch (ARRAY_SIZE(x->array))
    {
    case 3:
      if (x->array[2])
	return 0;
      /* Fall through.  */
    case 2:
      if (x->array[1])
	return 0;
      /* Fall through.  */
    case 1:
      return !x->array[0];
    default:
      abort ();
    }
}

static INLINE void
operand_type_set (union i386_operand_type *x, unsigned int v)
{
  switch (ARRAY_SIZE(x->array))
    {
    case 3:
      x->array[2] = v;
      /* Fall through.  */
    case 2:
      x->array[1] = v;
      /* Fall through.  */
    case 1:
      x->array[0] = v;
      /* Fall through.  */
      break;
    default:
      abort ();
    }
}

static INLINE int
operand_type_equal (const union i386_operand_type *x,
		    const union i386_operand_type *y)
{
  switch (ARRAY_SIZE(x->array))
    {
    case 3:
      if (x->array[2] != y->array[2])
	return 0;
      /* Fall through.  */
    case 2:
      if (x->array[1] != y->array[1])
	return 0;
      /* Fall through.  */
    case 1:
      return x->array[0] == y->array[0];
      break;
    default:
      abort ();
    }
}

static INLINE int
cpu_flags_all_zero (const union i386_cpu_flags *x)
{
  switch (ARRAY_SIZE(x->array))
    {
    case 4:
      if (x->array[3])
	return 0;
      /* Fall through.  */
    case 3:
      if (x->array[2])
	return 0;
      /* Fall through.  */
    case 2:
      if (x->array[1])
	return 0;
      /* Fall through.  */
    case 1:
      return !x->array[0];
    default:
      abort ();
    }
}

static INLINE int
cpu_flags_equal (const union i386_cpu_flags *x,
		 const union i386_cpu_flags *y)
{
  switch (ARRAY_SIZE(x->array))
    {
    case 4:
      if (x->array[3] != y->array[3])
	return 0;
      /* Fall through.  */
    case 3:
      if (x->array[2] != y->array[2])
	return 0;
      /* Fall through.  */
    case 2:
      if (x->array[1] != y->array[1])
	return 0;
      /* Fall through.  */
    case 1:
      return x->array[0] == y->array[0];
      break;
    default:
      abort ();
    }
}

static INLINE int
cpu_flags_check_cpu64 (i386_cpu_flags f)
{
  return !((flag_code == CODE_64BIT && f.bitfield.cpuno64)
	   || (flag_code != CODE_64BIT && f.bitfield.cpu64));
}

static INLINE i386_cpu_flags
cpu_flags_and (i386_cpu_flags x, i386_cpu_flags y)
{
  switch (ARRAY_SIZE (x.array))
    {
    case 4:
      x.array [3] &= y.array [3];
      /* Fall through.  */
    case 3:
      x.array [2] &= y.array [2];
      /* Fall through.  */
    case 2:
      x.array [1] &= y.array [1];
      /* Fall through.  */
    case 1:
      x.array [0] &= y.array [0];
      break;
    default:
      abort ();
    }
  return x;
}

static INLINE i386_cpu_flags
cpu_flags_or (i386_cpu_flags x, i386_cpu_flags y)
{
  switch (ARRAY_SIZE (x.array))
    {
    case 4:
      x.array [3] |= y.array [3];
      /* Fall through.  */
    case 3:
      x.array [2] |= y.array [2];
      /* Fall through.  */
    case 2:
      x.array [1] |= y.array [1];
      /* Fall through.  */
    case 1:
      x.array [0] |= y.array [0];
      break;
    default:
      abort ();
    }
  return x;
}

static INLINE i386_cpu_flags
cpu_flags_and_not (i386_cpu_flags x, i386_cpu_flags y)
{
  switch (ARRAY_SIZE (x.array))
    {
    case 4:
      x.array [3] &= ~y.array [3];
      /* Fall through.  */
    case 3:
      x.array [2] &= ~y.array [2];
      /* Fall through.  */
    case 2:
      x.array [1] &= ~y.array [1];
      /* Fall through.  */
    case 1:
      x.array [0] &= ~y.array [0];
      break;
    default:
      abort ();
    }
  return x;
}

#define CPU_FLAGS_ARCH_MATCH		0x1
#define CPU_FLAGS_64BIT_MATCH		0x2
#define CPU_FLAGS_AES_MATCH		0x4
#define CPU_FLAGS_PCLMUL_MATCH		0x8
#define CPU_FLAGS_AVX_MATCH	       0x10

#define CPU_FLAGS_32BIT_MATCH \
  (CPU_FLAGS_ARCH_MATCH | CPU_FLAGS_AES_MATCH \
   | CPU_FLAGS_PCLMUL_MATCH | CPU_FLAGS_AVX_MATCH)
#define CPU_FLAGS_PERFECT_MATCH \
  (CPU_FLAGS_32BIT_MATCH | CPU_FLAGS_64BIT_MATCH)

/* Return CPU flags match bits. */

static int
cpu_flags_match (const insn_template *t)
{
  i386_cpu_flags x = t->cpu_flags;
  int match = cpu_flags_check_cpu64 (x) ? CPU_FLAGS_64BIT_MATCH : 0;

  x.bitfield.cpu64 = 0;
  x.bitfield.cpuno64 = 0;

  if (cpu_flags_all_zero (&x))
    {
      /* This instruction is available on all archs.  */
      match |= CPU_FLAGS_32BIT_MATCH;
    }
  else
    {
      /* This instruction is available only on some archs.  */
      i386_cpu_flags cpu = cpu_arch_flags;

      cpu = cpu_flags_and (x, cpu);
      if (!cpu_flags_all_zero (&cpu))
	{
	  if (x.bitfield.cpuavx)
	    {
	      /* We only need to check AES/PCLMUL/SSE2AVX with AVX.  */
	      if (cpu.bitfield.cpuavx)
		{
		  /* Check SSE2AVX.  */
		  if (!t->opcode_modifier.sse2avx|| sse2avx)
		    {
		      match |= (CPU_FLAGS_ARCH_MATCH
				| CPU_FLAGS_AVX_MATCH);
		      /* Check AES.  */
		      if (!x.bitfield.cpuaes || cpu.bitfield.cpuaes)
			match |= CPU_FLAGS_AES_MATCH;
		      /* Check PCLMUL.  */
		      if (!x.bitfield.cpupclmul
			  || cpu.bitfield.cpupclmul)
			match |= CPU_FLAGS_PCLMUL_MATCH;
		    }
		}
	      else
		match |= CPU_FLAGS_ARCH_MATCH;
	    }
	  else if (x.bitfield.cpuavx512vl)
	    {
	      /* Match AVX512VL.  */
	      if (cpu.bitfield.cpuavx512vl)
		{
		  /* Need another match.  */
		  cpu.bitfield.cpuavx512vl = 0;
		  if (!cpu_flags_all_zero (&cpu))
		    match |= CPU_FLAGS_32BIT_MATCH;
		  else
		    match |= CPU_FLAGS_ARCH_MATCH;
		}
	      else
		match |= CPU_FLAGS_ARCH_MATCH;
	    }
	  else
	    match |= CPU_FLAGS_32BIT_MATCH;
	}
    }
  return match;
}

static INLINE i386_operand_type
operand_type_and (i386_operand_type x, i386_operand_type y)
{
  switch (ARRAY_SIZE (x.array))
    {
    case 3:
      x.array [2] &= y.array [2];
      /* Fall through.  */
    case 2:
      x.array [1] &= y.array [1];
      /* Fall through.  */
    case 1:
      x.array [0] &= y.array [0];
      break;
    default:
      abort ();
    }
  return x;
}

static INLINE i386_operand_type
operand_type_or (i386_operand_type x, i386_operand_type y)
{
  switch (ARRAY_SIZE (x.array))
    {
    case 3:
      x.array [2] |= y.array [2];
      /* Fall through.  */
    case 2:
      x.array [1] |= y.array [1];
      /* Fall through.  */
    case 1:
      x.array [0] |= y.array [0];
      break;
    default:
      abort ();
    }
  return x;
}

static INLINE i386_operand_type
operand_type_xor (i386_operand_type x, i386_operand_type y)
{
  switch (ARRAY_SIZE (x.array))
    {
    case 3:
      x.array [2] ^= y.array [2];
      /* Fall through.  */
    case 2:
      x.array [1] ^= y.array [1];
      /* Fall through.  */
    case 1:
      x.array [0] ^= y.array [0];
      break;
    default:
      abort ();
    }
  return x;
}

static const i386_operand_type acc32 = OPERAND_TYPE_ACC32;
static const i386_operand_type acc64 = OPERAND_TYPE_ACC64;
static const i386_operand_type control = OPERAND_TYPE_CONTROL;
static const i386_operand_type inoutportreg
  = OPERAND_TYPE_INOUTPORTREG;
static const i386_operand_type reg16_inoutportreg
  = OPERAND_TYPE_REG16_INOUTPORTREG;
static const i386_operand_type disp16 = OPERAND_TYPE_DISP16;
static const i386_operand_type disp32 = OPERAND_TYPE_DISP32;
static const i386_operand_type disp32s = OPERAND_TYPE_DISP32S;
static const i386_operand_type disp16_32 = OPERAND_TYPE_DISP16_32;
static const i386_operand_type anydisp
  = OPERAND_TYPE_ANYDISP;
static const i386_operand_type regxmm = OPERAND_TYPE_REGXMM;
static const i386_operand_type regmask = OPERAND_TYPE_REGMASK;
static const i386_operand_type imm8 = OPERAND_TYPE_IMM8;
static const i386_operand_type imm8s = OPERAND_TYPE_IMM8S;
static const i386_operand_type imm16 = OPERAND_TYPE_IMM16;
static const i386_operand_type imm32 = OPERAND_TYPE_IMM32;
static const i386_operand_type imm32s = OPERAND_TYPE_IMM32S;
static const i386_operand_type imm64 = OPERAND_TYPE_IMM64;
static const i386_operand_type imm16_32 = OPERAND_TYPE_IMM16_32;
static const i386_operand_type imm16_32s = OPERAND_TYPE_IMM16_32S;
static const i386_operand_type imm16_32_32s = OPERAND_TYPE_IMM16_32_32S;
static const i386_operand_type vec_imm4 = OPERAND_TYPE_VEC_IMM4;

enum operand_type
{
  reg,
  imm,
  disp,
  anymem
};

static INLINE int
operand_type_check (i386_operand_type t, enum operand_type c)
{
  switch (c)
    {
    case reg:
      return t.bitfield.reg;

    case imm:
      return (t.bitfield.imm8
	      || t.bitfield.imm8s
	      || t.bitfield.imm16
	      || t.bitfield.imm32
	      || t.bitfield.imm32s
	      || t.bitfield.imm64);

    case disp:
      return (t.bitfield.disp8
	      || t.bitfield.disp16
	      || t.bitfield.disp32
	      || t.bitfield.disp32s
	      || t.bitfield.disp64);

    case anymem:
      return (t.bitfield.disp8
	      || t.bitfield.disp16
	      || t.bitfield.disp32
	      || t.bitfield.disp32s
	      || t.bitfield.disp64
	      || t.bitfield.baseindex);

    default:
      abort ();
    }

  return 0;
}

/* Return 1 if there is no conflict in 8bit/16bit/32bit/64bit/80bit on
   operand J for instruction template T.  */

static INLINE int
match_reg_size (const insn_template *t, unsigned int j)
{
  return !((i.types[j].bitfield.byte
	    && !t->operand_types[j].bitfield.byte)
	   || (i.types[j].bitfield.word
	       && !t->operand_types[j].bitfield.word)
	   || (i.types[j].bitfield.dword
	       && !t->operand_types[j].bitfield.dword)
	   || (i.types[j].bitfield.qword
	       && !t->operand_types[j].bitfield.qword)
	   || (i.types[j].bitfield.tbyte
	       && !t->operand_types[j].bitfield.tbyte));
}

/* Return 1 if there is no conflict in SIMD register on
   operand J for instruction template T.  */

static INLINE int
match_simd_size (const insn_template *t, unsigned int j)
{
  return !((i.types[j].bitfield.xmmword
	    && !t->operand_types[j].bitfield.xmmword)
	   || (i.types[j].bitfield.ymmword
	       && !t->operand_types[j].bitfield.ymmword)
	   || (i.types[j].bitfield.zmmword
	       && !t->operand_types[j].bitfield.zmmword));
}

/* Return 1 if there is no conflict in any size on operand J for
   instruction template T.  */

static INLINE int
match_mem_size (const insn_template *t, unsigned int j)
{
  return (match_reg_size (t, j)
	  && !((i.types[j].bitfield.unspecified
		&& !i.broadcast
		&& !t->operand_types[j].bitfield.unspecified)
	       || (i.types[j].bitfield.fword
		   && !t->operand_types[j].bitfield.fword)
	       /* For scalar opcode templates to allow register and memory
		  operands at the same time, some special casing is needed
		  here.  */
	       || ((t->operand_types[j].bitfield.regsimd
		    && !t->opcode_modifier.broadcast
		    && (t->operand_types[j].bitfield.dword
			|| t->operand_types[j].bitfield.qword))
		   ? (i.types[j].bitfield.xmmword
		      || i.types[j].bitfield.ymmword
		      || i.types[j].bitfield.zmmword)
		   : !match_simd_size(t, j))));
}

/* Return 1 if there is no size conflict on any operands for
   instruction template T.  */

static INLINE int
operand_size_match (const insn_template *t)
{
  unsigned int j;
  int match = 1;

  /* Don't check jump instructions.  */
  if (t->opcode_modifier.jump
      || t->opcode_modifier.jumpbyte
      || t->opcode_modifier.jumpdword
      || t->opcode_modifier.jumpintersegment)
    return match;

  /* Check memory and accumulator operand size.  */
  for (j = 0; j < i.operands; j++)
    {
      if (!i.types[j].bitfield.reg && !i.types[j].bitfield.regsimd
	  && t->operand_types[j].bitfield.anysize)
	continue;

      if (t->operand_types[j].bitfield.reg
	  && !match_reg_size (t, j))
	{
	  match = 0;
	  break;
	}

      if (t->operand_types[j].bitfield.regsimd
	  && !match_simd_size (t, j))
	{
	  match = 0;
	  break;
	}

      if (t->operand_types[j].bitfield.acc
	  && (!match_reg_size (t, j) || !match_simd_size (t, j)))
	{
	  match = 0;
	  break;
	}

      if (i.types[j].bitfield.mem && !match_mem_size (t, j))
	{
	  match = 0;
	  break;
	}
    }

  if (match)
    return match;
  else if (!t->opcode_modifier.d && !t->opcode_modifier.floatd)
    {
mismatch:
      i.error = operand_size_mismatch;
      return 0;
    }

  /* Check reverse.  */
  gas_assert (i.operands == 2);

  match = 1;
  for (j = 0; j < 2; j++)
    {
      if ((t->operand_types[j].bitfield.reg
	   || t->operand_types[j].bitfield.acc)
	  && !match_reg_size (t, j ? 0 : 1))
	goto mismatch;

      if (i.types[j].bitfield.mem
	  && !match_mem_size (t, j ? 0 : 1))
	goto mismatch;
    }

  return match;
}

static INLINE int
operand_type_match (i386_operand_type overlap,
		    i386_operand_type given)
{
  i386_operand_type temp = overlap;

  temp.bitfield.jumpabsolute = 0;
  temp.bitfield.unspecified = 0;
  temp.bitfield.byte = 0;
  temp.bitfield.word = 0;
  temp.bitfield.dword = 0;
  temp.bitfield.fword = 0;
  temp.bitfield.qword = 0;
  temp.bitfield.tbyte = 0;
  temp.bitfield.xmmword = 0;
  temp.bitfield.ymmword = 0;
  temp.bitfield.zmmword = 0;
  if (operand_type_all_zero (&temp))
    goto mismatch;

  if (given.bitfield.baseindex == overlap.bitfield.baseindex
      && given.bitfield.jumpabsolute == overlap.bitfield.jumpabsolute)
    return 1;

mismatch:
  i.error = operand_type_mismatch;
  return 0;
}

/* If given types g0 and g1 are registers they must be of the same type
   unless the expected operand type register overlap is null.
   Memory operand size of certain SIMD instructions is also being checked
   here.  */

static INLINE int
operand_type_register_match (i386_operand_type g0,
			     i386_operand_type t0,
			     i386_operand_type g1,
			     i386_operand_type t1)
{
  if (!g0.bitfield.reg
      && !g0.bitfield.regsimd
      && (!operand_type_check (g0, anymem)
	  || g0.bitfield.unspecified
	  || !t0.bitfield.regsimd))
    return 1;

  if (!g1.bitfield.reg
      && !g1.bitfield.regsimd
      && (!operand_type_check (g1, anymem)
	  || g1.bitfield.unspecified
	  || !t1.bitfield.regsimd))
    return 1;

  if (g0.bitfield.byte == g1.bitfield.byte
      && g0.bitfield.word == g1.bitfield.word
      && g0.bitfield.dword == g1.bitfield.dword
      && g0.bitfield.qword == g1.bitfield.qword
      && g0.bitfield.xmmword == g1.bitfield.xmmword
      && g0.bitfield.ymmword == g1.bitfield.ymmword
      && g0.bitfield.zmmword == g1.bitfield.zmmword)
    return 1;

  if (!(t0.bitfield.byte & t1.bitfield.byte)
      && !(t0.bitfield.word & t1.bitfield.word)
      && !(t0.bitfield.dword & t1.bitfield.dword)
      && !(t0.bitfield.qword & t1.bitfield.qword)
      && !(t0.bitfield.xmmword & t1.bitfield.xmmword)
      && !(t0.bitfield.ymmword & t1.bitfield.ymmword)
      && !(t0.bitfield.zmmword & t1.bitfield.zmmword))
    return 1;

  i.error = register_type_mismatch;

  return 0;
}

static INLINE unsigned int
register_number (const reg_entry *r)
{
  unsigned int nr = r->reg_num;

  if (r->reg_flags & RegRex)
    nr += 8;

  if (r->reg_flags & RegVRex)
    nr += 16;

  return nr;
}

static INLINE unsigned int
mode_from_disp_size (i386_operand_type t)
{
  if (t.bitfield.disp8)
    return 1;
  else if (t.bitfield.disp16
	   || t.bitfield.disp32
	   || t.bitfield.disp32s)
    return 2;
  else
    return 0;
}

static INLINE int
fits_in_signed_byte (addressT num)
{
  return num + 0x80 <= 0xff;
}

static INLINE int
fits_in_unsigned_byte (addressT num)
{
  return num <= 0xff;
}

static INLINE int
fits_in_unsigned_word (addressT num)
{
  return num <= 0xffff;
}

static INLINE int
fits_in_signed_word (addressT num)
{
  return num + 0x8000 <= 0xffff;
}

static INLINE int
fits_in_signed_long (addressT num ATTRIBUTE_UNUSED)
{
#ifndef BFD64
  return 1;
#else
  return num + 0x80000000 <= 0xffffffff;
#endif
}				/* fits_in_signed_long() */

static INLINE int
fits_in_unsigned_long (addressT num ATTRIBUTE_UNUSED)
{
#ifndef BFD64
  return 1;
#else
  return num <= 0xffffffff;
#endif
}				/* fits_in_unsigned_long() */

static INLINE int
fits_in_disp8 (offsetT num)
{
  int shift = i.memshift;
  unsigned int mask;

  if (shift == -1)
    abort ();

  mask = (1 << shift) - 1;

  /* Return 0 if NUM isn't properly aligned.  */
  if ((num & mask))
    return 0;

  /* Check if NUM will fit in 8bit after shift.  */
  return fits_in_signed_byte (num >> shift);
}

static INLINE int
fits_in_imm4 (offsetT num)
{
  return (num & 0xf) == num;
}

static i386_operand_type
smallest_imm_type (offsetT num)
{
  i386_operand_type t;

  operand_type_set (&t, 0);
  t.bitfield.imm64 = 1;

  if (cpu_arch_tune != PROCESSOR_I486 && num == 1)
    {
      /* This code is disabled on the 486 because all the Imm1 forms
	 in the opcode table are slower on the i486.  They're the
	 versions with the implicitly specified single-position
	 displacement, which has another syntax if you really want to
	 use that form.  */
      t.bitfield.imm1 = 1;
      t.bitfield.imm8 = 1;
      t.bitfield.imm8s = 1;
      t.bitfield.imm16 = 1;
      t.bitfield.imm32 = 1;
      t.bitfield.imm32s = 1;
    }
  else if (fits_in_signed_byte (num))
    {
      t.bitfield.imm8 = 1;
      t.bitfield.imm8s = 1;
      t.bitfield.imm16 = 1;
      t.bitfield.imm32 = 1;
      t.bitfield.imm32s = 1;
    }
  else if (fits_in_unsigned_byte (num))
    {
      t.bitfield.imm8 = 1;
      t.bitfield.imm16 = 1;
      t.bitfield.imm32 = 1;
      t.bitfield.imm32s = 1;
    }
  else if (fits_in_signed_word (num) || fits_in_unsigned_word (num))
    {
      t.bitfield.imm16 = 1;
      t.bitfield.imm32 = 1;
      t.bitfield.imm32s = 1;
    }
  else if (fits_in_signed_long (num))
    {
      t.bitfield.imm32 = 1;
      t.bitfield.imm32s = 1;
    }
  else if (fits_in_unsigned_long (num))
    t.bitfield.imm32 = 1;

  return t;
}

static offsetT
offset_in_range (offsetT val, int size)
{
  addressT mask;

  switch (size)
    {
    case 1: mask = ((addressT) 1 <<  8) - 1; break;
    case 2: mask = ((addressT) 1 << 16) - 1; break;
    case 4: mask = ((addressT) 2 << 31) - 1; break;
#ifdef BFD64
    case 8: mask = ((addressT) 2 << 63) - 1; break;
#endif
    default: abort ();
    }

#ifdef BFD64
  /* If BFD64, sign extend val for 32bit address mode.  */
  if (flag_code != CODE_64BIT
      || i.prefix[ADDR_PREFIX])
    if ((val & ~(((addressT) 2 << 31) - 1)) == 0)
      val = (val ^ ((addressT) 1 << 31)) - ((addressT) 1 << 31);
#endif

  if ((val & ~mask) != 0 && (val & ~mask) != ~mask)
    {
      char buf1[40], buf2[40];

      sprint_value (buf1, val);
      sprint_value (buf2, val & mask);
      as_warn (_("%s shortened to %s"), buf1, buf2);
    }
  return val & mask;
}

enum PREFIX_GROUP
{
  PREFIX_EXIST = 0,
  PREFIX_LOCK,
  PREFIX_REP,
  PREFIX_DS,
  PREFIX_OTHER
};

/* Returns
   a. PREFIX_EXIST if attempting to add a prefix where one from the
   same class already exists.
   b. PREFIX_LOCK if lock prefix is added.
   c. PREFIX_REP if rep/repne prefix is added.
   d. PREFIX_DS if ds prefix is added.
   e. PREFIX_OTHER if other prefix is added.
 */

static enum PREFIX_GROUP
add_prefix (unsigned int prefix)
{
  enum PREFIX_GROUP ret = PREFIX_OTHER;
  unsigned int q;

  if (prefix >= REX_OPCODE && prefix < REX_OPCODE + 16
      && flag_code == CODE_64BIT)
    {
      if ((i.prefix[REX_PREFIX] & prefix & REX_W)
	  || ((i.prefix[REX_PREFIX] & (REX_R | REX_X | REX_B))
	      && (prefix & (REX_R | REX_X | REX_B))))
	ret = PREFIX_EXIST;
      q = REX_PREFIX;
    }
  else
    {
      switch (prefix)
	{
	default:
	  abort ();

	case DS_PREFIX_OPCODE:
	  ret = PREFIX_DS;
	  /* Fall through.  */
	case CS_PREFIX_OPCODE:
	case ES_PREFIX_OPCODE:
	case FS_PREFIX_OPCODE:
	case GS_PREFIX_OPCODE:
	case SS_PREFIX_OPCODE:
	  q = SEG_PREFIX;
	  break;

	case REPNE_PREFIX_OPCODE:
	case REPE_PREFIX_OPCODE:
	  q = REP_PREFIX;
	  ret = PREFIX_REP;
	  break;

	case LOCK_PREFIX_OPCODE:
	  q = LOCK_PREFIX;
	  ret = PREFIX_LOCK;
	  break;

	case FWAIT_OPCODE:
	  q = WAIT_PREFIX;
	  break;

	case ADDR_PREFIX_OPCODE:
	  q = ADDR_PREFIX;
	  break;

	case DATA_PREFIX_OPCODE:
	  q = DATA_PREFIX;
	  break;
	}
      if (i.prefix[q] != 0)
	ret = PREFIX_EXIST;
    }

  if (ret)
    {
      if (!i.prefix[q])
	++i.prefixes;
      i.prefix[q] |= prefix;
    }
  else
    as_bad (_("same type of prefix used twice"));

  return ret;
}

static void
update_code_flag (int value, int check)
{
  PRINTF_LIKE ((*as_error));

  flag_code = (enum flag_code) value;
  if (flag_code == CODE_64BIT)
    {
      cpu_arch_flags.bitfield.cpu64 = 1;
      cpu_arch_flags.bitfield.cpuno64 = 0;
    }
  else
    {
      cpu_arch_flags.bitfield.cpu64 = 0;
      cpu_arch_flags.bitfield.cpuno64 = 1;
    }
  if (value == CODE_64BIT && !cpu_arch_flags.bitfield.cpulm )
    {
      if (check)
	as_error = as_fatal;
      else
	as_error = as_bad;
      (*as_error) (_("64bit mode not supported on `%s'."),
		   cpu_arch_name ? cpu_arch_name : default_arch);
    }
  if (value == CODE_32BIT && !cpu_arch_flags.bitfield.cpui386)
    {
      if (check)
	as_error = as_fatal;
      else
	as_error = as_bad;
      (*as_error) (_("32bit mode not supported on `%s'."),
		   cpu_arch_name ? cpu_arch_name : default_arch);
    }
  stackop_size = '\0';
}

static void
set_code_flag (int value)
{
  update_code_flag (value, 0);
}

static void
set_16bit_gcc_code_flag (int new_code_flag)
{
  flag_code = (enum flag_code) new_code_flag;
  if (flag_code != CODE_16BIT)
    abort ();
  cpu_arch_flags.bitfield.cpu64 = 0;
  cpu_arch_flags.bitfield.cpuno64 = 1;
  stackop_size = LONG_MNEM_SUFFIX;
}

static void
set_intel_syntax (int syntax_flag)
{
  /* Find out if register prefixing is specified.  */
  int ask_naked_reg = 0;

  SKIP_WHITESPACE ();
  if (!is_end_of_line[(unsigned char) *input_line_pointer])
    {
      char *string;
      int e = get_symbol_name (&string);

      if (strcmp (string, "prefix") == 0)
	ask_naked_reg = 1;
      else if (strcmp (string, "noprefix") == 0)
	ask_naked_reg = -1;
      else
	as_bad (_("bad argument to syntax directive."));
      (void) restore_line_pointer (e);
    }
  demand_empty_rest_of_line ();

  intel_syntax = syntax_flag;

  if (ask_naked_reg == 0)
    allow_naked_reg = (intel_syntax
		       && (bfd_get_symbol_leading_char (stdoutput) != '\0'));
  else
    allow_naked_reg = (ask_naked_reg < 0);

  expr_set_rank (O_full_ptr, syntax_flag ? 10 : 0);

  identifier_chars['%'] = intel_syntax && allow_naked_reg ? '%' : 0;
  identifier_chars['$'] = intel_syntax ? '$' : 0;
  register_prefix = allow_naked_reg ? "" : "%";
}

static void
set_intel_mnemonic (int mnemonic_flag)
{
  intel_mnemonic = mnemonic_flag;
}

static void
set_allow_index_reg (int flag)
{
  allow_index_reg = flag;
}

static void
set_check (int what)
{
  enum check_kind *kind;
  const char *str;

  if (what)
    {
      kind = &operand_check;
      str = "operand";
    }
  else
    {
      kind = &sse_check;
      str = "sse";
    }

  SKIP_WHITESPACE ();

  if (!is_end_of_line[(unsigned char) *input_line_pointer])
    {
      char *string;
      int e = get_symbol_name (&string);

      if (strcmp (string, "none") == 0)
	*kind = check_none;
      else if (strcmp (string, "warning") == 0)
	*kind = check_warning;
      else if (strcmp (string, "error") == 0)
	*kind = check_error;
      else
	as_bad (_("bad argument to %s_check directive."), str);
      (void) restore_line_pointer (e);
    }
  else
    as_bad (_("missing argument for %s_check directive"), str);

  demand_empty_rest_of_line ();
}

static void
check_cpu_arch_compatible (const char *name ATTRIBUTE_UNUSED,
			   i386_cpu_flags new_flag ATTRIBUTE_UNUSED)
{
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  static const char *arch;

  /* Intel LIOM is only supported on ELF.  */
  if (!IS_ELF)
    return;

  if (!arch)
    {
      /* Use cpu_arch_name if it is set in md_parse_option.  Otherwise
	 use default_arch.  */
      arch = cpu_arch_name;
      if (!arch)
	arch = default_arch;
    }

  /* If we are targeting Intel MCU, we must enable it.  */
  if (get_elf_backend_data (stdoutput)->elf_machine_code != EM_IAMCU
      || new_flag.bitfield.cpuiamcu)
    return;

  /* If we are targeting Intel L1OM, we must enable it.  */
  if (get_elf_backend_data (stdoutput)->elf_machine_code != EM_L1OM
      || new_flag.bitfield.cpul1om)
    return;

  /* If we are targeting Intel K1OM, we must enable it.  */
  if (get_elf_backend_data (stdoutput)->elf_machine_code != EM_K1OM
      || new_flag.bitfield.cpuk1om)
    return;

  as_bad (_("`%s' is not supported on `%s'"), name, arch);
#endif
}

static void
set_cpu_arch (int dummy ATTRIBUTE_UNUSED)
{
  SKIP_WHITESPACE ();

  if (!is_end_of_line[(unsigned char) *input_line_pointer])
    {
      char *string;
      int e = get_symbol_name (&string);
      unsigned int j;
      i386_cpu_flags flags;

      for (j = 0; j < ARRAY_SIZE (cpu_arch); j++)
	{
	  if (strcmp (string, cpu_arch[j].name) == 0)
	    {
	      check_cpu_arch_compatible (string, cpu_arch[j].flags);

	      if (*string != '.')
		{
		  cpu_arch_name = cpu_arch[j].name;
		  cpu_sub_arch_name = NULL;
		  cpu_arch_flags = cpu_arch[j].flags;
		  if (flag_code == CODE_64BIT)
		    {
		      cpu_arch_flags.bitfield.cpu64 = 1;
		      cpu_arch_flags.bitfield.cpuno64 = 0;
		    }
		  else
		    {
		      cpu_arch_flags.bitfield.cpu64 = 0;
		      cpu_arch_flags.bitfield.cpuno64 = 1;
		    }
		  cpu_arch_isa = cpu_arch[j].type;
		  cpu_arch_isa_flags = cpu_arch[j].flags;
		  if (!cpu_arch_tune_set)
		    {
		      cpu_arch_tune = cpu_arch_isa;
		      cpu_arch_tune_flags = cpu_arch_isa_flags;
		    }
		  break;
		}

	      flags = cpu_flags_or (cpu_arch_flags,
				    cpu_arch[j].flags);

	      if (!cpu_flags_equal (&flags, &cpu_arch_flags))
		{
		  if (cpu_sub_arch_name)
		    {
		      char *name = cpu_sub_arch_name;
		      cpu_sub_arch_name = concat (name,
						  cpu_arch[j].name,
						  (const char *) NULL);
		      free (name);
		    }
		  else
		    cpu_sub_arch_name = xstrdup (cpu_arch[j].name);
		  cpu_arch_flags = flags;
		  cpu_arch_isa_flags = flags;
		}
	      (void) restore_line_pointer (e);
	      demand_empty_rest_of_line ();
	      return;
	    }
	}

      if (*string == '.' && j >= ARRAY_SIZE (cpu_arch))
	{
	  /* Disable an ISA extension.  */
	  for (j = 0; j < ARRAY_SIZE (cpu_noarch); j++)
	    if (strcmp (string + 1, cpu_noarch [j].name) == 0)
	      {
		flags = cpu_flags_and_not (cpu_arch_flags,
					   cpu_noarch[j].flags);
		if (!cpu_flags_equal (&flags, &cpu_arch_flags))
		  {
		    if (cpu_sub_arch_name)
		      {
			char *name = cpu_sub_arch_name;
			cpu_sub_arch_name = concat (name, string,
						    (const char *) NULL);
			free (name);
		      }
		    else
		      cpu_sub_arch_name = xstrdup (string);
		    cpu_arch_flags = flags;
		    cpu_arch_isa_flags = flags;
		  }
		(void) restore_line_pointer (e);
		demand_empty_rest_of_line ();
		return;
	      }

	  j = ARRAY_SIZE (cpu_arch);
	}

      if (j >= ARRAY_SIZE (cpu_arch))
	as_bad (_("no such architecture: `%s'"), string);

      *input_line_pointer = e;
    }
  else
    as_bad (_("missing cpu architecture"));

  no_cond_jump_promotion = 0;
  if (*input_line_pointer == ','
      && !is_end_of_line[(unsigned char) input_line_pointer[1]])
    {
      char *string;
      char e;

      ++input_line_pointer;
      e = get_symbol_name (&string);

      if (strcmp (string, "nojumps") == 0)
	no_cond_jump_promotion = 1;
      else if (strcmp (string, "jumps") == 0)
	;
      else
	as_bad (_("no such architecture modifier: `%s'"), string);

      (void) restore_line_pointer (e);
    }

  demand_empty_rest_of_line ();
}

enum bfd_architecture
i386_arch (void)
{
  if (cpu_arch_isa == PROCESSOR_L1OM)
    {
      if (OUTPUT_FLAVOR != bfd_target_elf_flavour
	  || flag_code != CODE_64BIT)
	as_fatal (_("Intel L1OM is 64bit ELF only"));
      return bfd_arch_l1om;
    }
  else if (cpu_arch_isa == PROCESSOR_K1OM)
    {
      if (OUTPUT_FLAVOR != bfd_target_elf_flavour
	  || flag_code != CODE_64BIT)
	as_fatal (_("Intel K1OM is 64bit ELF only"));
      return bfd_arch_k1om;
    }
  else if (cpu_arch_isa == PROCESSOR_IAMCU)
    {
      if (OUTPUT_FLAVOR != bfd_target_elf_flavour
	  || flag_code == CODE_64BIT)
	as_fatal (_("Intel MCU is 32bit ELF only"));
      return bfd_arch_iamcu;
    }
  else
    return bfd_arch_i386;
}

unsigned long
i386_mach (void)
{
  if (!strncmp (default_arch, "x86_64", 6))
    {
      if (cpu_arch_isa == PROCESSOR_L1OM)
	{
	  if (OUTPUT_FLAVOR != bfd_target_elf_flavour
	      || default_arch[6] != '\0')
	    as_fatal (_("Intel L1OM is 64bit ELF only"));
	  return bfd_mach_l1om;
	}
      else if (cpu_arch_isa == PROCESSOR_K1OM)
	{
	  if (OUTPUT_FLAVOR != bfd_target_elf_flavour
	      || default_arch[6] != '\0')
	    as_fatal (_("Intel K1OM is 64bit ELF only"));
	  return bfd_mach_k1om;
	}
      else if (default_arch[6] == '\0')
	return bfd_mach_x86_64;
      else
	return bfd_mach_x64_32;
    }
  else if (!strcmp (default_arch, "i386")
	   || !strcmp (default_arch, "iamcu"))
    {
      if (cpu_arch_isa == PROCESSOR_IAMCU)
	{
	  if (OUTPUT_FLAVOR != bfd_target_elf_flavour)
	    as_fatal (_("Intel MCU is 32bit ELF only"));
	  return bfd_mach_i386_iamcu;
	}
      else
	return bfd_mach_i386_i386;
    }
  else
    as_fatal (_("unknown architecture"));
}

void
md_begin (void)
{
  const char *hash_err;

  /* Support pseudo prefixes like {disp32}.  */
  lex_type ['{'] = LEX_BEGIN_NAME;

  /* Initialize op_hash hash table.  */
  op_hash = hash_new ();

  {
    const insn_template *optab;
    templates *core_optab;

    /* Setup for loop.  */
    optab = i386_optab;
    core_optab = XNEW (templates);
    core_optab->start = optab;

    while (1)
      {
	++optab;
	if (optab->name == NULL
	    || strcmp (optab->name, (optab - 1)->name) != 0)
	  {
	    /* different name --> ship out current template list;
	       add to hash table; & begin anew.  */
	    core_optab->end = optab;
	    hash_err = hash_insert (op_hash,
				    (optab - 1)->name,
				    (void *) core_optab);
	    if (hash_err)
	      {
		as_fatal (_("can't hash %s: %s"),
			  (optab - 1)->name,
			  hash_err);
	      }
	    if (optab->name == NULL)
	      break;
	    core_optab = XNEW (templates);
	    core_optab->start = optab;
	  }
      }
  }

  /* Initialize reg_hash hash table.  */
  reg_hash = hash_new ();
  {
    const reg_entry *regtab;
    unsigned int regtab_size = i386_regtab_size;

    for (regtab = i386_regtab; regtab_size--; regtab++)
      {
	hash_err = hash_insert (reg_hash, regtab->reg_name, (void *) regtab);
	if (hash_err)
	  as_fatal (_("can't hash %s: %s"),
		    regtab->reg_name,
		    hash_err);
      }
  }

  /* Fill in lexical tables:  mnemonic_chars, operand_chars.  */
  {
    int c;
    char *p;

    for (c = 0; c < 256; c++)
      {
	if (ISDIGIT (c))
	  {
	    digit_chars[c] = c;
	    mnemonic_chars[c] = c;
	    register_chars[c] = c;
	    operand_chars[c] = c;
	  }
	else if (ISLOWER (c))
	  {
	    mnemonic_chars[c] = c;
	    register_chars[c] = c;
	    operand_chars[c] = c;
	  }
	else if (ISUPPER (c))
	  {
	    mnemonic_chars[c] = TOLOWER (c);
	    register_chars[c] = mnemonic_chars[c];
	    operand_chars[c] = c;
	  }
	else if (c == '{' || c == '}')
	  {
	    mnemonic_chars[c] = c;
	    operand_chars[c] = c;
	  }

	if (ISALPHA (c) || ISDIGIT (c))
	  identifier_chars[c] = c;
	else if (c >= 128)
	  {
	    identifier_chars[c] = c;
	    operand_chars[c] = c;
	  }
      }

#ifdef LEX_AT
    identifier_chars['@'] = '@';
#endif
#ifdef LEX_QM
    identifier_chars['?'] = '?';
    operand_chars['?'] = '?';
#endif
    digit_chars['-'] = '-';
    mnemonic_chars['_'] = '_';
    mnemonic_chars['-'] = '-';
    mnemonic_chars['.'] = '.';
    identifier_chars['_'] = '_';
    identifier_chars['.'] = '.';

    for (p = operand_special_chars; *p != '\0'; p++)
      operand_chars[(unsigned char) *p] = *p;
  }

  if (flag_code == CODE_64BIT)
    {
#if defined (OBJ_COFF) && defined (TE_PE)
      x86_dwarf2_return_column = (OUTPUT_FLAVOR == bfd_target_coff_flavour
				  ? 32 : 16);
#else
      x86_dwarf2_return_column = 16;
#endif
      x86_cie_data_alignment = -8;
    }
  else
    {
      x86_dwarf2_return_column = 8;
      x86_cie_data_alignment = -4;
    }
}

void
i386_print_statistics (FILE *file)
{
  hash_print_statistics (file, "i386 opcode", op_hash);
  hash_print_statistics (file, "i386 register", reg_hash);
}

#ifdef DEBUG386

/* Debugging routines for md_assemble.  */
static void pte (insn_template *);
static void pt (i386_operand_type);
static void pe (expressionS *);
static void ps (symbolS *);

static void
pi (char *line, i386_insn *x)
{
  unsigned int j;

  fprintf (stdout, "%s: template ", line);
  pte (&x->tm);
  fprintf (stdout, "  address: base %s  index %s  scale %x\n",
	   x->base_reg ? x->base_reg->reg_name : "none",
	   x->index_reg ? x->index_reg->reg_name : "none",
	   x->log2_scale_factor);
  fprintf (stdout, "  modrm:  mode %x  reg %x  reg/mem %x\n",
	   x->rm.mode, x->rm.reg, x->rm.regmem);
  fprintf (stdout, "  sib:  base %x  index %x  scale %x\n",
	   x->sib.base, x->sib.index, x->sib.scale);
  fprintf (stdout, "  rex: 64bit %x  extX %x  extY %x  extZ %x\n",
	   (x->rex & REX_W) != 0,
	   (x->rex & REX_R) != 0,
	   (x->rex & REX_X) != 0,
	   (x->rex & REX_B) != 0);
  for (j = 0; j < x->operands; j++)
    {
      fprintf (stdout, "    #%d:  ", j + 1);
      pt (x->types[j]);
      fprintf (stdout, "\n");
      if (x->types[j].bitfield.reg
	  || x->types[j].bitfield.regmmx
	  || x->types[j].bitfield.regsimd
	  || x->types[j].bitfield.sreg2
	  || x->types[j].bitfield.sreg3
	  || x->types[j].bitfield.control
	  || x->types[j].bitfield.debug
	  || x->types[j].bitfield.test)
	fprintf (stdout, "%s\n", x->op[j].regs->reg_name);
      if (operand_type_check (x->types[j], imm))
	pe (x->op[j].imms);
      if (operand_type_check (x->types[j], disp))
	pe (x->op[j].disps);
    }
}

static void
pte (insn_template *t)
{
  unsigned int j;
  fprintf (stdout, " %d operands ", t->operands);
  fprintf (stdout, "opcode %x ", t->base_opcode);
  if (t->extension_opcode != None)
    fprintf (stdout, "ext %x ", t->extension_opcode);
  if (t->opcode_modifier.d)
    fprintf (stdout, "D");
  if (t->opcode_modifier.w)
    fprintf (stdout, "W");
  fprintf (stdout, "\n");
  for (j = 0; j < t->operands; j++)
    {
      fprintf (stdout, "    #%d type ", j + 1);
      pt (t->operand_types[j]);
      fprintf (stdout, "\n");
    }
}

static void
pe (expressionS *e)
{
  fprintf (stdout, "    operation     %d\n", e->X_op);
  fprintf (stdout, "    add_number    %ld (%lx)\n",
	   (long) e->X_add_number, (long) e->X_add_number);
  if (e->X_add_symbol)
    {
      fprintf (stdout, "    add_symbol    ");
      ps (e->X_add_symbol);
      fprintf (stdout, "\n");
    }
  if (e->X_op_symbol)
    {
      fprintf (stdout, "    op_symbol    ");
      ps (e->X_op_symbol);
      fprintf (stdout, "\n");
    }
}

static void
ps (symbolS *s)
{
  fprintf (stdout, "%s type %s%s",
	   S_GET_NAME (s),
	   S_IS_EXTERNAL (s) ? "EXTERNAL " : "",
	   segment_name (S_GET_SEGMENT (s)));
}

static struct type_name
  {
    i386_operand_type mask;
    const char *name;
  }
const type_names[] =
{
  { OPERAND_TYPE_REG8, "r8" },
  { OPERAND_TYPE_REG16, "r16" },
  { OPERAND_TYPE_REG32, "r32" },
  { OPERAND_TYPE_REG64, "r64" },
  { OPERAND_TYPE_IMM8, "i8" },
  { OPERAND_TYPE_IMM8, "i8s" },
  { OPERAND_TYPE_IMM16, "i16" },
  { OPERAND_TYPE_IMM32, "i32" },
  { OPERAND_TYPE_IMM32S, "i32s" },
  { OPERAND_TYPE_IMM64, "i64" },
  { OPERAND_TYPE_IMM1, "i1" },
  { OPERAND_TYPE_BASEINDEX, "BaseIndex" },
  { OPERAND_TYPE_DISP8, "d8" },
  { OPERAND_TYPE_DISP16, "d16" },
  { OPERAND_TYPE_DISP32, "d32" },
  { OPERAND_TYPE_DISP32S, "d32s" },
  { OPERAND_TYPE_DISP64, "d64" },
  { OPERAND_TYPE_INOUTPORTREG, "InOutPortReg" },
  { OPERAND_TYPE_SHIFTCOUNT, "ShiftCount" },
  { OPERAND_TYPE_CONTROL, "control reg" },
  { OPERAND_TYPE_TEST, "test reg" },
  { OPERAND_TYPE_DEBUG, "debug reg" },
  { OPERAND_TYPE_FLOATREG, "FReg" },
  { OPERAND_TYPE_FLOATACC, "FAcc" },
  { OPERAND_TYPE_SREG2, "SReg2" },
  { OPERAND_TYPE_SREG3, "SReg3" },
  { OPERAND_TYPE_ACC, "Acc" },
  { OPERAND_TYPE_JUMPABSOLUTE, "Jump Absolute" },
  { OPERAND_TYPE_REGMMX, "rMMX" },
  { OPERAND_TYPE_REGXMM, "rXMM" },
  { OPERAND_TYPE_REGYMM, "rYMM" },
  { OPERAND_TYPE_REGZMM, "rZMM" },
  { OPERAND_TYPE_REGMASK, "Mask reg" },
  { OPERAND_TYPE_ESSEG, "es" },
};

static void
pt (i386_operand_type t)
{
  unsigned int j;
  i386_operand_type a;

  for (j = 0; j < ARRAY_SIZE (type_names); j++)
    {
      a = operand_type_and (t, type_names[j].mask);
      if (!operand_type_all_zero (&a))
	fprintf (stdout, "%s, ",  type_names[j].name);
    }
  fflush (stdout);
}

#endif /* DEBUG386 */

static bfd_reloc_code_real_type
reloc (unsigned int size,
       int pcrel,
       int sign,
       bfd_reloc_code_real_type other)
{
  if (other != NO_RELOC)
    {
      reloc_howto_type *rel;

      if (size == 8)
	switch (other)
	  {
	  case BFD_RELOC_X86_64_GOT32:
	    return BFD_RELOC_X86_64_GOT64;
	    break;
	  case BFD_RELOC_X86_64_GOTPLT64:
	    return BFD_RELOC_X86_64_GOTPLT64;
	    break;
	  case BFD_RELOC_X86_64_PLTOFF64:
	    return BFD_RELOC_X86_64_PLTOFF64;
	    break;
	  case BFD_RELOC_X86_64_GOTPC32:
	    other = BFD_RELOC_X86_64_GOTPC64;
	    break;
	  case BFD_RELOC_X86_64_GOTPCREL:
	    other = BFD_RELOC_X86_64_GOTPCREL64;
	    break;
	  case BFD_RELOC_X86_64_TPOFF32:
	    other = BFD_RELOC_X86_64_TPOFF64;
	    break;
	  case BFD_RELOC_X86_64_DTPOFF32:
	    other = BFD_RELOC_X86_64_DTPOFF64;
	    break;
	  default:
	    break;
	  }

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
      if (other == BFD_RELOC_SIZE32)
	{
	  if (size == 8)
	    other = BFD_RELOC_SIZE64;
	  if (pcrel)
	    {
	      as_bad (_("there are no pc-relative size relocations"));
	      return NO_RELOC;
	    }
	}
#endif

      /* Sign-checking 4-byte relocations in 16-/32-bit code is pointless.  */
      if (size == 4 && (flag_code != CODE_64BIT || disallow_64bit_reloc))
	sign = -1;

      rel = bfd_reloc_type_lookup (stdoutput, other);
      if (!rel)
	as_bad (_("unknown relocation (%u)"), other);
      else if (size != bfd_get_reloc_size (rel))
	as_bad (_("%u-byte relocation cannot be applied to %u-byte field"),
		bfd_get_reloc_size (rel),
		size);
      else if (pcrel && !rel->pc_relative)
	as_bad (_("non-pc-relative relocation for pc-relative field"));
      else if ((rel->complain_on_overflow == complain_overflow_signed
		&& !sign)
	       || (rel->complain_on_overflow == complain_overflow_unsigned
		   && sign > 0))
	as_bad (_("relocated field and relocation type differ in signedness"));
      else
	return other;
      return NO_RELOC;
    }

  if (pcrel)
    {
      if (!sign)
	as_bad (_("there are no unsigned pc-relative relocations"));
      switch (size)
	{
	case 1: return BFD_RELOC_8_PCREL;
	case 2: return BFD_RELOC_16_PCREL;
	case 4: return BFD_RELOC_32_PCREL;
	case 8: return BFD_RELOC_64_PCREL;
	}
      as_bad (_("cannot do %u byte pc-relative relocation"), size);
    }
  else
    {
      if (sign > 0)
	switch (size)
	  {
	  case 4: return BFD_RELOC_X86_64_32S;
	  }
      else
	switch (size)
	  {
	  case 1: return BFD_RELOC_8;
	  case 2: return BFD_RELOC_16;
	  case 4: return BFD_RELOC_32;
	  case 8: return BFD_RELOC_64;
	  }
      as_bad (_("cannot do %s %u byte relocation"),
	      sign > 0 ? "signed" : "unsigned", size);
    }

  return NO_RELOC;
}

/* Here we decide which fixups can be adjusted to make them relative to
   the beginning of the section instead of the symbol.  Basically we need
   to make sure that the dynamic relocations are done correctly, so in
   some cases we force the original symbol to be used.  */

int
tc_i386_fix_adjustable (fixS *fixP ATTRIBUTE_UNUSED)
{
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  if (!IS_ELF)
    return 1;

  /* Don't adjust pc-relative references to merge sections in 64-bit
     mode.  */
  if (use_rela_relocations
      && (S_GET_SEGMENT (fixP->fx_addsy)->flags & SEC_MERGE) != 0
      && fixP->fx_pcrel)
    return 0;

  /* The x86_64 GOTPCREL are represented as 32bit PCrel relocations
     and changed later by validate_fix.  */
  if (GOT_symbol && fixP->fx_subsy == GOT_symbol
      && fixP->fx_r_type == BFD_RELOC_32_PCREL)
    return 0;

  /* Adjust_reloc_syms doesn't know about the GOT.  Need to keep symbol
     for size relocations.  */
  if (fixP->fx_r_type == BFD_RELOC_SIZE32
      || fixP->fx_r_type == BFD_RELOC_SIZE64
      || fixP->fx_r_type == BFD_RELOC_386_GOTOFF
      || fixP->fx_r_type == BFD_RELOC_386_PLT32
      || fixP->fx_r_type == BFD_RELOC_386_GOT32
      || fixP->fx_r_type == BFD_RELOC_386_GOT32X
      || fixP->fx_r_type == BFD_RELOC_386_TLS_GD
      || fixP->fx_r_type == BFD_RELOC_386_TLS_LDM
      || fixP->fx_r_type == BFD_RELOC_386_TLS_LDO_32
      || fixP->fx_r_type == BFD_RELOC_386_TLS_IE_32
      || fixP->fx_r_type == BFD_RELOC_386_TLS_IE
      || fixP->fx_r_type == BFD_RELOC_386_TLS_GOTIE
      || fixP->fx_r_type == BFD_RELOC_386_TLS_LE_32
      || fixP->fx_r_type == BFD_RELOC_386_TLS_LE
      || fixP->fx_r_type == BFD_RELOC_386_TLS_GOTDESC
      || fixP->fx_r_type == BFD_RELOC_386_TLS_DESC_CALL
      || fixP->fx_r_type == BFD_RELOC_X86_64_PLT32
      || fixP->fx_r_type == BFD_RELOC_X86_64_GOT32
      || fixP->fx_r_type == BFD_RELOC_X86_64_GOTPCREL
      || fixP->fx_r_type == BFD_RELOC_X86_64_GOTPCRELX
      || fixP->fx_r_type == BFD_RELOC_X86_64_REX_GOTPCRELX
      || fixP->fx_r_type == BFD_RELOC_X86_64_TLSGD
      || fixP->fx_r_type == BFD_RELOC_X86_64_TLSLD
      || fixP->fx_r_type == BFD_RELOC_X86_64_DTPOFF32
      || fixP->fx_r_type == BFD_RELOC_X86_64_DTPOFF64
      || fixP->fx_r_type == BFD_RELOC_X86_64_GOTTPOFF
      || fixP->fx_r_type == BFD_RELOC_X86_64_TPOFF32
      || fixP->fx_r_type == BFD_RELOC_X86_64_TPOFF64
      || fixP->fx_r_type == BFD_RELOC_X86_64_GOTOFF64
      || fixP->fx_r_type == BFD_RELOC_X86_64_GOTPC32_TLSDESC
      || fixP->fx_r_type == BFD_RELOC_X86_64_TLSDESC_CALL
      || fixP->fx_r_type == BFD_RELOC_VTABLE_INHERIT
      || fixP->fx_r_type == BFD_RELOC_VTABLE_ENTRY)
    return 0;
#endif
  return 1;
}

static int
intel_float_operand (const char *mnemonic)
{
  /* Note that the value returned is meaningful only for opcodes with (memory)
     operands, hence the code here is free to improperly handle opcodes that
     have no operands (for better performance and smaller code). */

  if (mnemonic[0] != 'f')
    return 0; /* non-math */

  switch (mnemonic[1])
    {
    /* fclex, fdecstp, fdisi, femms, feni, fincstp, finit, fsetpm, and
       the fs segment override prefix not currently handled because no
       call path can make opcodes without operands get here */
    case 'i':
      return 2 /* integer op */;
    case 'l':
      if (mnemonic[2] == 'd' && (mnemonic[3] == 'c' || mnemonic[3] == 'e'))
	return 3; /* fldcw/fldenv */
      break;
    case 'n':
      if (mnemonic[2] != 'o' /* fnop */)
	return 3; /* non-waiting control op */
      break;
    case 'r':
      if (mnemonic[2] == 's')
	return 3; /* frstor/frstpm */
      break;
    case 's':
      if (mnemonic[2] == 'a')
	return 3; /* fsave */
      if (mnemonic[2] == 't')
	{
	  switch (mnemonic[3])
	    {
	    case 'c': /* fstcw */
	    case 'd': /* fstdw */
	    case 'e': /* fstenv */
	    case 's': /* fsts[gw] */
	      return 3;
	    }
	}
      break;
    case 'x':
      if (mnemonic[2] == 'r' || mnemonic[2] == 's')
	return 0; /* fxsave/fxrstor are not really math ops */
      break;
    }

  return 1;
}

/* Build the VEX prefix.  */

static void
build_vex_prefix (const insn_template *t)
{
  unsigned int register_specifier;
  unsigned int implied_prefix;
  unsigned int vector_length;

  /* Check register specifier.  */
  if (i.vex.register_specifier)
    {
      register_specifier =
	~register_number (i.vex.register_specifier) & 0xf;
      gas_assert ((i.vex.register_specifier->reg_flags & RegVRex) == 0);
    }
  else
    register_specifier = 0xf;

  /* Use 2-byte VEX prefix by swapping destination and source
     operand.  */
  if (i.vec_encoding != vex_encoding_vex3
      && i.dir_encoding == dir_encoding_default
      && i.operands == i.reg_operands
      && i.tm.opcode_modifier.vexopcode == VEX0F
      && i.tm.opcode_modifier.load
      && i.rex == REX_B)
    {
      unsigned int xchg = i.operands - 1;
      union i386_op temp_op;
      i386_operand_type temp_type;

      temp_type = i.types[xchg];
      i.types[xchg] = i.types[0];
      i.types[0] = temp_type;
      temp_op = i.op[xchg];
      i.op[xchg] = i.op[0];
      i.op[0] = temp_op;

      gas_assert (i.rm.mode == 3);

      i.rex = REX_R;
      xchg = i.rm.regmem;
      i.rm.regmem = i.rm.reg;
      i.rm.reg = xchg;

      /* Use the next insn.  */
      i.tm = t[1];
    }

  if (i.tm.opcode_modifier.vex == VEXScalar)
    vector_length = avxscalar;
  else if (i.tm.opcode_modifier.vex == VEX256)
    vector_length = 1;
  else
    {
      unsigned int op;

      vector_length = 0;
      for (op = 0; op < t->operands; ++op)
	if (t->operand_types[op].bitfield.xmmword
	    && t->operand_types[op].bitfield.ymmword
	    && i.types[op].bitfield.ymmword)
	  {
	    vector_length = 1;
	    break;
	  }
    }

  switch ((i.tm.base_opcode >> 8) & 0xff)
    {
    case 0:
      implied_prefix = 0;
      break;
    case DATA_PREFIX_OPCODE:
      implied_prefix = 1;
      break;
    case REPE_PREFIX_OPCODE:
      implied_prefix = 2;
      break;
    case REPNE_PREFIX_OPCODE:
      implied_prefix = 3;
      break;
    default:
      abort ();
    }

  /* Use 2-byte VEX prefix if possible.  */
  if (i.vec_encoding != vex_encoding_vex3
      && i.tm.opcode_modifier.vexopcode == VEX0F
      && i.tm.opcode_modifier.vexw != VEXW1
      && (i.rex & (REX_W | REX_X | REX_B)) == 0)
    {
      /* 2-byte VEX prefix.  */
      unsigned int r;

      i.vex.length = 2;
      i.vex.bytes[0] = 0xc5;

      /* Check the REX.R bit.  */
      r = (i.rex & REX_R) ? 0 : 1;
      i.vex.bytes[1] = (r << 7
			| register_specifier << 3
			| vector_length << 2
			| implied_prefix);
    }
  else
    {
      /* 3-byte VEX prefix.  */
      unsigned int m, w;

      i.vex.length = 3;

      switch (i.tm.opcode_modifier.vexopcode)
	{
	case VEX0F:
	  m = 0x1;
	  i.vex.bytes[0] = 0xc4;
	  break;
	case VEX0F38:
	  m = 0x2;
	  i.vex.bytes[0] = 0xc4;
	  break;
	case VEX0F3A:
	  m = 0x3;
	  i.vex.bytes[0] = 0xc4;
	  break;
	case XOP08:
	  m = 0x8;
	  i.vex.bytes[0] = 0x8f;
	  break;
	case XOP09:
	  m = 0x9;
	  i.vex.bytes[0] = 0x8f;
	  break;
	case XOP0A:
	  m = 0xa;
	  i.vex.bytes[0] = 0x8f;
	  break;
	default:
	  abort ();
	}

      /* The high 3 bits of the second VEX byte are 1's compliment
	 of RXB bits from REX.  */
      i.vex.bytes[1] = (~i.rex & 0x7) << 5 | m;

      /* Check the REX.W bit.  */
      w = (i.rex & REX_W) ? 1 : 0;
      if (i.tm.opcode_modifier.vexw == VEXW1)
	w = 1;

      i.vex.bytes[2] = (w << 7
			| register_specifier << 3
			| vector_length << 2
			| implied_prefix);
    }
}

/* Build the EVEX prefix.  */

static void
build_evex_prefix (void)
{
  unsigned int register_specifier;
  unsigned int implied_prefix;
  unsigned int m, w;
  rex_byte vrex_used = 0;

  /* Check register specifier.  */
  if (i.vex.register_specifier)
    {
      gas_assert ((i.vrex & REX_X) == 0);

      register_specifier = i.vex.register_specifier->reg_num;
      if ((i.vex.register_specifier->reg_flags & RegRex))
	register_specifier += 8;
      /* The upper 16 registers are encoded in the fourth byte of the
	 EVEX prefix.  */
      if (!(i.vex.register_specifier->reg_flags & RegVRex))
	i.vex.bytes[3] = 0x8;
      register_specifier = ~register_specifier & 0xf;
    }
  else
    {
      register_specifier = 0xf;

      /* Encode upper 16 vector index register in the fourth byte of
	 the EVEX prefix.  */
      if (!(i.vrex & REX_X))
	i.vex.bytes[3] = 0x8;
      else
	vrex_used |= REX_X;
    }

  switch ((i.tm.base_opcode >> 8) & 0xff)
    {
    case 0:
      implied_prefix = 0;
      break;
    case DATA_PREFIX_OPCODE:
      implied_prefix = 1;
      break;
    case REPE_PREFIX_OPCODE:
      implied_prefix = 2;
      break;
    case REPNE_PREFIX_OPCODE:
      implied_prefix = 3;
      break;
    default:
      abort ();
    }

  /* 4 byte EVEX prefix.  */
  i.vex.length = 4;
  i.vex.bytes[0] = 0x62;

  /* mmmm bits.  */
  switch (i.tm.opcode_modifier.vexopcode)
    {
    case VEX0F:
      m = 1;
      break;
    case VEX0F38:
      m = 2;
      break;
    case VEX0F3A:
      m = 3;
      break;
    default:
      abort ();
      break;
    }

  /* The high 3 bits of the second EVEX byte are 1's compliment of RXB
     bits from REX.  */
  i.vex.bytes[1] = (~i.rex & 0x7) << 5 | m;

  /* The fifth bit of the second EVEX byte is 1's compliment of the
     REX_R bit in VREX.  */
  if (!(i.vrex & REX_R))
    i.vex.bytes[1] |= 0x10;
  else
    vrex_used |= REX_R;

  if ((i.reg_operands + i.imm_operands) == i.operands)
    {
      /* When all operands are registers, the REX_X bit in REX is not
	 used.  We reuse it to encode the upper 16 registers, which is
	 indicated by the REX_B bit in VREX.  The REX_X bit is encoded
	 as 1's compliment.  */
      if ((i.vrex & REX_B))
	{
	  vrex_used |= REX_B;
	  i.vex.bytes[1] &= ~0x40;
	}
    }

  /* EVEX instructions shouldn't need the REX prefix.  */
  i.vrex &= ~vrex_used;
  gas_assert (i.vrex == 0);

  /* Check the REX.W bit.  */
  w = (i.rex & REX_W) ? 1 : 0;
  if (i.tm.opcode_modifier.vexw)
    {
      if (i.tm.opcode_modifier.vexw == VEXW1)
	w = 1;
    }
  /* If w is not set it means we are dealing with WIG instruction.  */
  else if (!w)
    {
      if (evexwig == evexw1)
        w = 1;
    }

  /* Encode the U bit.  */
  implied_prefix |= 0x4;

  /* The third byte of the EVEX prefix.  */
  i.vex.bytes[2] = (w << 7 | register_specifier << 3 | implied_prefix);

  /* The fourth byte of the EVEX prefix.  */
  /* The zeroing-masking bit.  */
  if (i.mask && i.mask->zeroing)
    i.vex.bytes[3] |= 0x80;

  /* Don't always set the broadcast bit if there is no RC.  */
  if (!i.rounding)
    {
      /* Encode the vector length.  */
      unsigned int vec_length;

      switch (i.tm.opcode_modifier.evex)
	{
	case EVEXLIG: /* LL' is ignored */
	  vec_length = evexlig << 5;
	  break;
	case EVEX128:
	  vec_length = 0 << 5;
	  break;
	case EVEX256:
	  vec_length = 1 << 5;
	  break;
	case EVEX512:
	  vec_length = 2 << 5;
	  break;
	default:
	  abort ();
	  break;
	}
      i.vex.bytes[3] |= vec_length;
      /* Encode the broadcast bit.  */
      if (i.broadcast)
	i.vex.bytes[3] |= 0x10;
    }
  else
    {
      if (i.rounding->type != saeonly)
	i.vex.bytes[3] |= 0x10 | (i.rounding->type << 5);
      else
	i.vex.bytes[3] |= 0x10 | (evexrcig << 5);
    }

  if (i.mask && i.mask->mask)
    i.vex.bytes[3] |= i.mask->mask->reg_num;
}

static void
process_immext (void)
{
  expressionS *exp;

  if ((i.tm.cpu_flags.bitfield.cpusse3 || i.tm.cpu_flags.bitfield.cpusvme)
      && i.operands > 0)
    {
      /* MONITOR/MWAIT as well as SVME instructions have fixed operands
	 with an opcode suffix which is coded in the same place as an
	 8-bit immediate field would be.
	 Here we check those operands and remove them afterwards.  */
      unsigned int x;

      for (x = 0; x < i.operands; x++)
	if (register_number (i.op[x].regs) != x)
	  as_bad (_("can't use register '%s%s' as operand %d in '%s'."),
		  register_prefix, i.op[x].regs->reg_name, x + 1,
		  i.tm.name);

      i.operands = 0;
    }

  if (i.tm.cpu_flags.bitfield.cpumwaitx && i.operands > 0)
    {
      /* MONITORX/MWAITX instructions have fixed operands with an opcode
	 suffix which is coded in the same place as an 8-bit immediate
	 field would be.
	 Here we check those operands and remove them afterwards.  */
      unsigned int x;

      if (i.operands != 3)
	abort();

      for (x = 0; x < 2; x++)
	if (register_number (i.op[x].regs) != x)
	  goto bad_register_operand;

      /* Check for third operand for mwaitx/monitorx insn.  */
      if (register_number (i.op[x].regs)
	  != (x + (i.tm.extension_opcode == 0xfb)))
	{
bad_register_operand:
	  as_bad (_("can't use register '%s%s' as operand %d in '%s'."),
		  register_prefix, i.op[x].regs->reg_name, x+1,
		  i.tm.name);
	}

      i.operands = 0;
    }

  /* These AMD 3DNow! and SSE2 instructions have an opcode suffix
     which is coded in the same place as an 8-bit immediate field
     would be.  Here we fake an 8-bit immediate operand from the
     opcode suffix stored in tm.extension_opcode.

     AVX instructions also use this encoding, for some of
     3 argument instructions.  */

  gas_assert (i.imm_operands <= 1
	      && (i.operands <= 2
		  || ((i.tm.opcode_modifier.vex
		       || i.tm.opcode_modifier.evex)
		      && i.operands <= 4)));

  exp = &im_expressions[i.imm_operands++];
  i.op[i.operands].imms = exp;
  i.types[i.operands] = imm8;
  i.operands++;
  exp->X_op = O_constant;
  exp->X_add_number = i.tm.extension_opcode;
  i.tm.extension_opcode = None;
}


static int
check_hle (void)
{
  switch (i.tm.opcode_modifier.hleprefixok)
    {
    default:
      abort ();
    case HLEPrefixNone:
      as_bad (_("invalid instruction `%s' after `%s'"),
	      i.tm.name, i.hle_prefix);
      return 0;
    case HLEPrefixLock:
      if (i.prefix[LOCK_PREFIX])
	return 1;
      as_bad (_("missing `lock' with `%s'"), i.hle_prefix);
      return 0;
    case HLEPrefixAny:
      return 1;
    case HLEPrefixRelease:
      if (i.prefix[HLE_PREFIX] != XRELEASE_PREFIX_OPCODE)
	{
	  as_bad (_("instruction `%s' after `xacquire' not allowed"),
		  i.tm.name);
	  return 0;
	}
      if (i.mem_operands == 0
	  || !operand_type_check (i.types[i.operands - 1], anymem))
	{
	  as_bad (_("memory destination needed for instruction `%s'"
		    " after `xrelease'"), i.tm.name);
	  return 0;
	}
      return 1;
    }
}

/* This is the guts of the machine-dependent assembler.  LINE points to a
   machine dependent instruction.  This function is supposed to emit
   the frags/bytes it assembles to.  */

void
md_assemble (char *line)
{
  unsigned int j;
  char mnemonic[MAX_MNEM_SIZE], mnem_suffix;
  const insn_template *t;

  /* Initialize globals.  */
  memset (&i, '\0', sizeof (i));
  for (j = 0; j < MAX_OPERANDS; j++)
    i.reloc[j] = NO_RELOC;
  memset (disp_expressions, '\0', sizeof (disp_expressions));
  memset (im_expressions, '\0', sizeof (im_expressions));
  save_stack_p = save_stack;

  /* First parse an instruction mnemonic & call i386_operand for the operands.
     We assume that the scrubber has arranged it so that line[0] is the valid
     start of a (possibly prefixed) mnemonic.  */

  line = parse_insn (line, mnemonic);
  if (line == NULL)
    return;
  mnem_suffix = i.suffix;

  line = parse_operands (line, mnemonic);
  this_operand = -1;
  xfree (i.memop1_string);
  i.memop1_string = NULL;
  if (line == NULL)
    return;

  /* Now we've parsed the mnemonic into a set of templates, and have the
     operands at hand.  */

  /* All intel opcodes have reversed operands except for "bound" and
     "enter".  We also don't reverse intersegment "jmp" and "call"
     instructions with 2 immediate operands so that the immediate segment
     precedes the offset, as it does when in AT&T mode. */
  if (intel_syntax
      && i.operands > 1
      && (strcmp (mnemonic, "bound") != 0)
      && (strcmp (mnemonic, "invlpga") != 0)
      && !(operand_type_check (i.types[0], imm)
	   && operand_type_check (i.types[1], imm)))
    swap_operands ();

  /* The order of the immediates should be reversed
     for 2 immediates extrq and insertq instructions */
  if (i.imm_operands == 2
      && (strcmp (mnemonic, "extrq") == 0
	  || strcmp (mnemonic, "insertq") == 0))
      swap_2_operands (0, 1);

  if (i.imm_operands)
    optimize_imm ();

  /* Don't optimize displacement for movabs since it only takes 64bit
     displacement.  */
  if (i.disp_operands
      && i.disp_encoding != disp_encoding_32bit
      && (flag_code != CODE_64BIT
	  || strcmp (mnemonic, "movabs") != 0))
    optimize_disp ();

  /* Next, we find a template that matches the given insn,
     making sure the overlap of the given operands types is consistent
     with the template operand types.  */

  if (!(t = match_template (mnem_suffix)))
    return;

  if (sse_check != check_none
      && !i.tm.opcode_modifier.noavx
      && (i.tm.cpu_flags.bitfield.cpusse
	  || i.tm.cpu_flags.bitfield.cpusse2
	  || i.tm.cpu_flags.bitfield.cpusse3
	  || i.tm.cpu_flags.bitfield.cpussse3
	  || i.tm.cpu_flags.bitfield.cpusse4_1
	  || i.tm.cpu_flags.bitfield.cpusse4_2))
    {
      (sse_check == check_warning
       ? as_warn
       : as_bad) (_("SSE instruction `%s' is used"), i.tm.name);
    }

  /* Zap movzx and movsx suffix.  The suffix has been set from
     "word ptr" or "byte ptr" on the source operand in Intel syntax
     or extracted from mnemonic in AT&T syntax.  But we'll use
     the destination register to choose the suffix for encoding.  */
  if ((i.tm.base_opcode & ~9) == 0x0fb6)
    {
      /* In Intel syntax, there must be a suffix.  In AT&T syntax, if
	 there is no suffix, the default will be byte extension.  */
      if (i.reg_operands != 2
	  && !i.suffix
	  && intel_syntax)
	as_bad (_("ambiguous operand size for `%s'"), i.tm.name);

      i.suffix = 0;
    }

  if (i.tm.opcode_modifier.fwait)
    if (!add_prefix (FWAIT_OPCODE))
      return;

  /* Check if REP prefix is OK.  */
  if (i.rep_prefix && !i.tm.opcode_modifier.repprefixok)
    {
      as_bad (_("invalid instruction `%s' after `%s'"),
		i.tm.name, i.rep_prefix);
      return;
    }

  /* Check for lock without a lockable instruction.  Destination operand
     must be memory unless it is xchg (0x86).  */
  if (i.prefix[LOCK_PREFIX]
      && (!i.tm.opcode_modifier.islockable
	  || i.mem_operands == 0
	  || (i.tm.base_opcode != 0x86
	      && !operand_type_check (i.types[i.operands - 1], anymem))))
    {
      as_bad (_("expecting lockable instruction after `lock'"));
      return;
    }

  /* Check if HLE prefix is OK.  */
  if (i.hle_prefix && !check_hle ())
    return;

  /* Check BND prefix.  */
  if (i.bnd_prefix && !i.tm.opcode_modifier.bndprefixok)
    as_bad (_("expecting valid branch instruction after `bnd'"));

  /* Check NOTRACK prefix.  */
  if (i.notrack_prefix && !i.tm.opcode_modifier.notrackprefixok)
    as_bad (_("expecting indirect branch instruction after `notrack'"));

  if (i.tm.cpu_flags.bitfield.cpumpx)
    {
      if (flag_code == CODE_64BIT && i.prefix[ADDR_PREFIX])
	as_bad (_("32-bit address isn't allowed in 64-bit MPX instructions."));
      else if (flag_code != CODE_16BIT
	       ? i.prefix[ADDR_PREFIX]
	       : i.mem_operands && !i.prefix[ADDR_PREFIX])
	as_bad (_("16-bit address isn't allowed in MPX instructions"));
    }

  /* Insert BND prefix.  */
  if (add_bnd_prefix
      && i.tm.opcode_modifier.bndprefixok
      && !i.prefix[BND_PREFIX])
    add_prefix (BND_PREFIX_OPCODE);

  /* Check string instruction segment overrides.  */
  if (i.tm.opcode_modifier.isstring && i.mem_operands != 0)
    {
      if (!check_string ())
	return;
      i.disp_operands = 0;
    }

  if (!process_suffix ())
    return;

  /* Update operand types.  */
  for (j = 0; j < i.operands; j++)
    i.types[j] = operand_type_and (i.types[j], i.tm.operand_types[j]);

  /* Make still unresolved immediate matches conform to size of immediate
     given in i.suffix.  */
  if (!finalize_imm ())
    return;

  if (i.types[0].bitfield.imm1)
    i.imm_operands = 0;	/* kludge for shift insns.  */

  /* We only need to check those implicit registers for instructions
     with 3 operands or less.  */
  if (i.operands <= 3)
    for (j = 0; j < i.operands; j++)
      if (i.types[j].bitfield.inoutportreg
	  || i.types[j].bitfield.shiftcount
	  || (i.types[j].bitfield.acc && !i.types[j].bitfield.xmmword))
	i.reg_operands--;

  /* ImmExt should be processed after SSE2AVX.  */
  if (!i.tm.opcode_modifier.sse2avx
      && i.tm.opcode_modifier.immext)
    process_immext ();

  /* For insns with operands there are more diddles to do to the opcode.  */
  if (i.operands)
    {
      if (!process_operands ())
	return;
    }
  else if (!quiet_warnings && i.tm.opcode_modifier.ugh)
    {
      /* UnixWare fsub no args is alias for fsubp, fadd -> faddp, etc.  */
      as_warn (_("translating to `%sp'"), i.tm.name);
    }

  if (i.tm.opcode_modifier.vex || i.tm.opcode_modifier.evex)
    {
      if (flag_code == CODE_16BIT)
	{
	  as_bad (_("instruction `%s' isn't supported in 16-bit mode."),
		  i.tm.name);
	  return;
	}

      if (i.tm.opcode_modifier.vex)
	build_vex_prefix (t);
      else
	build_evex_prefix ();
    }

  /* Handle conversion of 'int $3' --> special int3 insn.  XOP or FMA4
     instructions may define INT_OPCODE as well, so avoid this corner
     case for those instructions that use MODRM.  */
  if (i.tm.base_opcode == INT_OPCODE
      && !i.tm.opcode_modifier.modrm
      && i.op[0].imms->X_add_number == 3)
    {
      i.tm.base_opcode = INT3_OPCODE;
      i.imm_operands = 0;
    }

  if ((i.tm.opcode_modifier.jump
       || i.tm.opcode_modifier.jumpbyte
       || i.tm.opcode_modifier.jumpdword)
      && i.op[0].disps->X_op == O_constant)
    {
      /* Convert "jmp constant" (and "call constant") to a jump (call) to
	 the absolute address given by the constant.  Since ix86 jumps and
	 calls are pc relative, we need to generate a reloc.  */
      i.op[0].disps->X_add_symbol = &abs_symbol;
      i.op[0].disps->X_op = O_symbol;
    }

  if (i.tm.opcode_modifier.rex64)
    i.rex |= REX_W;

  /* For 8 bit registers we need an empty rex prefix.  Also if the
     instruction already has a prefix, we need to convert old
     registers to new ones.  */

  if ((i.types[0].bitfield.reg && i.types[0].bitfield.byte
       && (i.op[0].regs->reg_flags & RegRex64) != 0)
      || (i.types[1].bitfield.reg && i.types[1].bitfield.byte
	  && (i.op[1].regs->reg_flags & RegRex64) != 0)
      || (((i.types[0].bitfield.reg && i.types[0].bitfield.byte)
	   || (i.types[1].bitfield.reg && i.types[1].bitfield.byte))
	  && i.rex != 0))
    {
      int x;

      i.rex |= REX_OPCODE;
      for (x = 0; x < 2; x++)
	{
	  /* Look for 8 bit operand that uses old registers.  */
	  if (i.types[x].bitfield.reg && i.types[x].bitfield.byte
	      && (i.op[x].regs->reg_flags & RegRex64) == 0)
	    {
	      /* In case it is "hi" register, give up.  */
	      if (i.op[x].regs->reg_num > 3)
		as_bad (_("can't encode register '%s%s' in an "
			  "instruction requiring REX prefix."),
			register_prefix, i.op[x].regs->reg_name);

	      /* Otherwise it is equivalent to the extended register.
		 Since the encoding doesn't change this is merely
		 cosmetic cleanup for debug output.  */

	      i.op[x].regs = i.op[x].regs + 8;
	    }
	}
    }

  if (i.rex != 0)
    add_prefix (REX_OPCODE | i.rex);

  /* We are ready to output the insn.  */
  output_insn ();
}

static char *
parse_insn (char *line, char *mnemonic)
{
  char *l = line;
  char *token_start = l;
  char *mnem_p;
  int supported;
  const insn_template *t;
  char *dot_p = NULL;

  while (1)
    {
      mnem_p = mnemonic;
      while ((*mnem_p = mnemonic_chars[(unsigned char) *l]) != 0)
	{
	  if (*mnem_p == '.')
	    dot_p = mnem_p;
	  mnem_p++;
	  if (mnem_p >= mnemonic + MAX_MNEM_SIZE)
	    {
	      as_bad (_("no such instruction: `%s'"), token_start);
	      return NULL;
	    }
	  l++;
	}
      if (!is_space_char (*l)
	  && *l != END_OF_INSN
	  && (intel_syntax
	      || (*l != PREFIX_SEPARATOR
		  && *l != ',')))
	{
	  as_bad (_("invalid character %s in mnemonic"),
		  output_invalid (*l));
	  return NULL;
	}
      if (token_start == l)
	{
	  if (!intel_syntax && *l == PREFIX_SEPARATOR)
	    as_bad (_("expecting prefix; got nothing"));
	  else
	    as_bad (_("expecting mnemonic; got nothing"));
	  return NULL;
	}

      /* Look up instruction (or prefix) via hash table.  */
      current_templates = (const templates *) hash_find (op_hash, mnemonic);

      if (*l != END_OF_INSN
	  && (!is_space_char (*l) || l[1] != END_OF_INSN)
	  && current_templates
	  && current_templates->start->opcode_modifier.isprefix)
	{
	  if (!cpu_flags_check_cpu64 (current_templates->start->cpu_flags))
	    {
	      as_bad ((flag_code != CODE_64BIT
		       ? _("`%s' is only supported in 64-bit mode")
		       : _("`%s' is not supported in 64-bit mode")),
		      current_templates->start->name);
	      return NULL;
	    }
	  /* If we are in 16-bit mode, do not allow addr16 or data16.
	     Similarly, in 32-bit mode, do not allow addr32 or data32.  */
	  if ((current_templates->start->opcode_modifier.size16
	       || current_templates->start->opcode_modifier.size32)
	      && flag_code != CODE_64BIT
	      && (current_templates->start->opcode_modifier.size32
		  ^ (flag_code == CODE_16BIT)))
	    {
	      as_bad (_("redundant %s prefix"),
		      current_templates->start->name);
	      return NULL;
	    }
	  if (current_templates->start->opcode_length == 0)
	    {
	      /* Handle pseudo prefixes.  */
	      switch (current_templates->start->base_opcode)
		{
		case 0x0:
		  /* {disp8} */
		  i.disp_encoding = disp_encoding_8bit;
		  break;
		case 0x1:
		  /* {disp32} */
		  i.disp_encoding = disp_encoding_32bit;
		  break;
		case 0x2:
		  /* {load} */
		  i.dir_encoding = dir_encoding_load;
		  break;
		case 0x3:
		  /* {store} */
		  i.dir_encoding = dir_encoding_store;
		  break;
		case 0x4:
		  /* {vex2} */
		  i.vec_encoding = vex_encoding_vex2;
		  break;
		case 0x5:
		  /* {vex3} */
		  i.vec_encoding = vex_encoding_vex3;
		  break;
		case 0x6:
		  /* {evex} */
		  i.vec_encoding = vex_encoding_evex;
		  break;
		default:
		  abort ();
		}
	    }
	  else
	    {
	      /* Add prefix, checking for repeated prefixes.  */
	      switch (add_prefix (current_templates->start->base_opcode))
		{
		case PREFIX_EXIST:
		  return NULL;
		case PREFIX_DS:
		  if (current_templates->start->cpu_flags.bitfield.cpucet)
		    i.notrack_prefix = current_templates->start->name;
		  break;
		case PREFIX_REP:
		  if (current_templates->start->cpu_flags.bitfield.cpuhle)
		    i.hle_prefix = current_templates->start->name;
		  else if (current_templates->start->cpu_flags.bitfield.cpumpx)
		    i.bnd_prefix = current_templates->start->name;
		  else
		    i.rep_prefix = current_templates->start->name;
		  break;
		default:
		  break;
		}
	    }
	  /* Skip past PREFIX_SEPARATOR and reset token_start.  */
	  token_start = ++l;
	}
      else
	break;
    }

  if (!current_templates)
    {
      /* Check if we should swap operand or force 32bit displacement in
	 encoding.  */
      if (mnem_p - 2 == dot_p && dot_p[1] == 's')
	i.dir_encoding = dir_encoding_store;
      else if (mnem_p - 3 == dot_p
	       && dot_p[1] == 'd'
	       && dot_p[2] == '8')
	i.disp_encoding = disp_encoding_8bit;
      else if (mnem_p - 4 == dot_p
	       && dot_p[1] == 'd'
	       && dot_p[2] == '3'
	       && dot_p[3] == '2')
	i.disp_encoding = disp_encoding_32bit;
      else
	goto check_suffix;
      mnem_p = dot_p;
      *dot_p = '\0';
      current_templates = (const templates *) hash_find (op_hash, mnemonic);
    }

  if (!current_templates)
    {
check_suffix:
      /* See if we can get a match by trimming off a suffix.  */
      switch (mnem_p[-1])
	{
	case WORD_MNEM_SUFFIX:
	  if (intel_syntax && (intel_float_operand (mnemonic) & 2))
	    i.suffix = SHORT_MNEM_SUFFIX;
	  else
	    /* Fall through.  */
	case BYTE_MNEM_SUFFIX:
	case QWORD_MNEM_SUFFIX:
	  i.suffix = mnem_p[-1];
	  mnem_p[-1] = '\0';
	  current_templates = (const templates *) hash_find (op_hash,
                                                             mnemonic);
	  break;
	case SHORT_MNEM_SUFFIX:
	case LONG_MNEM_SUFFIX:
	  if (!intel_syntax)
	    {
	      i.suffix = mnem_p[-1];
	      mnem_p[-1] = '\0';
	      current_templates = (const templates *) hash_find (op_hash,
                                                                 mnemonic);
	    }
	  break;

	  /* Intel Syntax.  */
	case 'd':
	  if (intel_syntax)
	    {
	      if (intel_float_operand (mnemonic) == 1)
		i.suffix = SHORT_MNEM_SUFFIX;
	      else
		i.suffix = LONG_MNEM_SUFFIX;
	      mnem_p[-1] = '\0';
	      current_templates = (const templates *) hash_find (op_hash,
                                                                 mnemonic);
	    }
	  break;
	}
      if (!current_templates)
	{
	  as_bad (_("no such instruction: `%s'"), token_start);
	  return NULL;
	}
    }

  if (current_templates->start->opcode_modifier.jump
      || current_templates->start->opcode_modifier.jumpbyte)
    {
      /* Check for a branch hint.  We allow ",pt" and ",pn" for
	 predict taken and predict not taken respectively.
	 I'm not sure that branch hints actually do anything on loop
	 and jcxz insns (JumpByte) for current Pentium4 chips.  They
	 may work in the future and it doesn't hurt to accept them
	 now.  */
      if (l[0] == ',' && l[1] == 'p')
	{
	  if (l[2] == 't')
	    {
	      if (!add_prefix (DS_PREFIX_OPCODE))
		return NULL;
	      l += 3;
	    }
	  else if (l[2] == 'n')
	    {
	      if (!add_prefix (CS_PREFIX_OPCODE))
		return NULL;
	      l += 3;
	    }
	}
    }
  /* Any other comma loses.  */
  if (*l == ',')
    {
      as_bad (_("invalid character %s in mnemonic"),
	      output_invalid (*l));
      return NULL;
    }

  /* Check if instruction is supported on specified architecture.  */
  supported = 0;
  for (t = current_templates->start; t < current_templates->end; ++t)
    {
      supported |= cpu_flags_match (t);
      if (supported == CPU_FLAGS_PERFECT_MATCH)
	goto skip;
    }

  if (!(supported & CPU_FLAGS_64BIT_MATCH))
    {
      as_bad (flag_code == CODE_64BIT
	      ? _("`%s' is not supported in 64-bit mode")
	      : _("`%s' is only supported in 64-bit mode"),
	      current_templates->start->name);
      return NULL;
    }
  if (supported != CPU_FLAGS_PERFECT_MATCH)
    {
      as_bad (_("`%s' is not supported on `%s%s'"),
	      current_templates->start->name,
	      cpu_arch_name ? cpu_arch_name : default_arch,
	      cpu_sub_arch_name ? cpu_sub_arch_name : "");
      return NULL;
    }

skip:
  if (!cpu_arch_flags.bitfield.cpui386
	   && (flag_code != CODE_16BIT))
    {
      as_warn (_("use .code16 to ensure correct addressing mode"));
    }

  return l;
}

static char *
parse_operands (char *l, const char *mnemonic)
{
  char *token_start;

  /* 1 if operand is pending after ','.  */
  unsigned int expecting_operand = 0;

  /* Non-zero if operand parens not balanced.  */
  unsigned int paren_not_balanced;

  while (*l != END_OF_INSN)
    {
      /* Skip optional white space before operand.  */
      if (is_space_char (*l))
	++l;
      if (!is_operand_char (*l) && *l != END_OF_INSN && *l != '"')
	{
	  as_bad (_("invalid character %s before operand %d"),
		  output_invalid (*l),
		  i.operands + 1);
	  return NULL;
	}
      token_start = l;	/* After white space.  */
      paren_not_balanced = 0;
      while (paren_not_balanced || *l != ',')
	{
	  if (*l == END_OF_INSN)
	    {
	      if (paren_not_balanced)
		{
		  if (!intel_syntax)
		    as_bad (_("unbalanced parenthesis in operand %d."),
			    i.operands + 1);
		  else
		    as_bad (_("unbalanced brackets in operand %d."),
			    i.operands + 1);
		  return NULL;
		}
	      else
		break;	/* we are done */
	    }
	  else if (!is_operand_char (*l) && !is_space_char (*l) && *l != '"')
	    {
	      as_bad (_("invalid character %s in operand %d"),
		      output_invalid (*l),
		      i.operands + 1);
	      return NULL;
	    }
	  if (!intel_syntax)
	    {
	      if (*l == '(')
		++paren_not_balanced;
	      if (*l == ')')
		--paren_not_balanced;
	    }
	  else
	    {
	      if (*l == '[')
		++paren_not_balanced;
	      if (*l == ']')
		--paren_not_balanced;
	    }
	  l++;
	}
      if (l != token_start)
	{			/* Yes, we've read in another operand.  */
	  unsigned int operand_ok;
	  this_operand = i.operands++;
	  if (i.operands > MAX_OPERANDS)
	    {
	      as_bad (_("spurious operands; (%d operands/instruction max)"),
		      MAX_OPERANDS);
	      return NULL;
	    }
	  i.types[this_operand].bitfield.unspecified = 1;
	  /* Now parse operand adding info to 'i' as we go along.  */
	  END_STRING_AND_SAVE (l);

	  if (intel_syntax)
	    operand_ok =
	      i386_intel_operand (token_start,
				  intel_float_operand (mnemonic));
	  else
	    operand_ok = i386_att_operand (token_start);

	  RESTORE_END_STRING (l);
	  if (!operand_ok)
	    return NULL;
	}
      else
	{
	  if (expecting_operand)
	    {
	    expecting_operand_after_comma:
	      as_bad (_("expecting operand after ','; got nothing"));
	      return NULL;
	    }
	  if (*l == ',')
	    {
	      as_bad (_("expecting operand before ','; got nothing"));
	      return NULL;
	    }
	}

      /* Now *l must be either ',' or END_OF_INSN.  */
      if (*l == ',')
	{
	  if (*++l == END_OF_INSN)
	    {
	      /* Just skip it, if it's \n complain.  */
	      goto expecting_operand_after_comma;
	    }
	  expecting_operand = 1;
	}
    }
  return l;
}

static void
swap_2_operands (int xchg1, int xchg2)
{
  union i386_op temp_op;
  i386_operand_type temp_type;
  enum bfd_reloc_code_real temp_reloc;

  temp_type = i.types[xchg2];
  i.types[xchg2] = i.types[xchg1];
  i.types[xchg1] = temp_type;
  temp_op = i.op[xchg2];
  i.op[xchg2] = i.op[xchg1];
  i.op[xchg1] = temp_op;
  temp_reloc = i.reloc[xchg2];
  i.reloc[xchg2] = i.reloc[xchg1];
  i.reloc[xchg1] = temp_reloc;

  if (i.mask)
    {
      if (i.mask->operand == xchg1)
	i.mask->operand = xchg2;
      else if (i.mask->operand == xchg2)
	i.mask->operand = xchg1;
    }
  if (i.broadcast)
    {
      if (i.broadcast->operand == xchg1)
	i.broadcast->operand = xchg2;
      else if (i.broadcast->operand == xchg2)
	i.broadcast->operand = xchg1;
    }
  if (i.rounding)
    {
      if (i.rounding->operand == xchg1)
	i.rounding->operand = xchg2;
      else if (i.rounding->operand == xchg2)
	i.rounding->operand = xchg1;
    }
}

static void
swap_operands (void)
{
  switch (i.operands)
    {
    case 5:
    case 4:
      swap_2_operands (1, i.operands - 2);
      /* Fall through.  */
    case 3:
    case 2:
      swap_2_operands (0, i.operands - 1);
      break;
    default:
      abort ();
    }

  if (i.mem_operands == 2)
    {
      const seg_entry *temp_seg;
      temp_seg = i.seg[0];
      i.seg[0] = i.seg[1];
      i.seg[1] = temp_seg;
    }
}

/* Try to ensure constant immediates are represented in the smallest
   opcode possible.  */
static void
optimize_imm (void)
{
  char guess_suffix = 0;
  int op;

  if (i.suffix)
    guess_suffix = i.suffix;
  else if (i.reg_operands)
    {
      /* Figure out a suffix from the last register operand specified.
	 We can't do this properly yet, ie. excluding InOutPortReg,
	 but the following works for instructions with immediates.
	 In any case, we can't set i.suffix yet.  */
      for (op = i.operands; --op >= 0;)
	if (i.types[op].bitfield.reg && i.types[op].bitfield.byte)
	  {
	    guess_suffix = BYTE_MNEM_SUFFIX;
	    break;
	  }
	else if (i.types[op].bitfield.reg && i.types[op].bitfield.word)
	  {
	    guess_suffix = WORD_MNEM_SUFFIX;
	    break;
	  }
	else if (i.types[op].bitfield.reg && i.types[op].bitfield.dword)
	  {
	    guess_suffix = LONG_MNEM_SUFFIX;
	    break;
	  }
	else if (i.types[op].bitfield.reg && i.types[op].bitfield.qword)
	  {
	    guess_suffix = QWORD_MNEM_SUFFIX;
	    break;
	  }
    }
  else if ((flag_code == CODE_16BIT) ^ (i.prefix[DATA_PREFIX] != 0))
    guess_suffix = WORD_MNEM_SUFFIX;

  for (op = i.operands; --op >= 0;)
    if (operand_type_check (i.types[op], imm))
      {
	switch (i.op[op].imms->X_op)
	  {
	  case O_constant:
	    /* If a suffix is given, this operand may be shortened.  */
	    switch (guess_suffix)
	      {
	      case LONG_MNEM_SUFFIX:
		i.types[op].bitfield.imm32 = 1;
		i.types[op].bitfield.imm64 = 1;
		break;
	      case WORD_MNEM_SUFFIX:
		i.types[op].bitfield.imm16 = 1;
		i.types[op].bitfield.imm32 = 1;
		i.types[op].bitfield.imm32s = 1;
		i.types[op].bitfield.imm64 = 1;
		break;
	      case BYTE_MNEM_SUFFIX:
		i.types[op].bitfield.imm8 = 1;
		i.types[op].bitfield.imm8s = 1;
		i.types[op].bitfield.imm16 = 1;
		i.types[op].bitfield.imm32 = 1;
		i.types[op].bitfield.imm32s = 1;
		i.types[op].bitfield.imm64 = 1;
		break;
	      }

	    /* If this operand is at most 16 bits, convert it
	       to a signed 16 bit number before trying to see
	       whether it will fit in an even smaller size.
	       This allows a 16-bit operand such as $0xffe0 to
	       be recognised as within Imm8S range.  */
	    if ((i.types[op].bitfield.imm16)
		&& (i.op[op].imms->X_add_number & ~(offsetT) 0xffff) == 0)
	      {
		i.op[op].imms->X_add_number =
		  (((i.op[op].imms->X_add_number & 0xffff) ^ 0x8000) - 0x8000);
	      }
#ifdef BFD64
	    /* Store 32-bit immediate in 64-bit for 64-bit BFD.  */
	    if ((i.types[op].bitfield.imm32)
		&& ((i.op[op].imms->X_add_number & ~(((offsetT) 2 << 31) - 1))
		    == 0))
	      {
		i.op[op].imms->X_add_number = ((i.op[op].imms->X_add_number
						^ ((offsetT) 1 << 31))
					       - ((offsetT) 1 << 31));
	      }
#endif
	    i.types[op]
	      = operand_type_or (i.types[op],
				 smallest_imm_type (i.op[op].imms->X_add_number));

	    /* We must avoid matching of Imm32 templates when 64bit
	       only immediate is available.  */
	    if (guess_suffix == QWORD_MNEM_SUFFIX)
	      i.types[op].bitfield.imm32 = 0;
	    break;

	  case O_absent:
	  case O_register:
	    abort ();

	    /* Symbols and expressions.  */
	  default:
	    /* Convert symbolic operand to proper sizes for matching, but don't
	       prevent matching a set of insns that only supports sizes other
	       than those matching the insn suffix.  */
	    {
	      i386_operand_type mask, allowed;
	      const insn_template *t;

	      operand_type_set (&mask, 0);
	      operand_type_set (&allowed, 0);

	      for (t = current_templates->start;
		   t < current_templates->end;
		   ++t)
		allowed = operand_type_or (allowed,
					   t->operand_types[op]);
	      switch (guess_suffix)
		{
		case QWORD_MNEM_SUFFIX:
		  mask.bitfield.imm64 = 1;
		  mask.bitfield.imm32s = 1;
		  break;
		case LONG_MNEM_SUFFIX:
		  mask.bitfield.imm32 = 1;
		  break;
		case WORD_MNEM_SUFFIX:
		  mask.bitfield.imm16 = 1;
		  break;
		case BYTE_MNEM_SUFFIX:
		  mask.bitfield.imm8 = 1;
		  break;
		default:
		  break;
		}
	      allowed = operand_type_and (mask, allowed);
	      if (!operand_type_all_zero (&allowed))
		i.types[op] = operand_type_and (i.types[op], mask);
	    }
	    break;
	  }
      }
}

/* Try to use the smallest displacement type too.  */
static void
optimize_disp (void)
{
  int op;

  for (op = i.operands; --op >= 0;)
    if (operand_type_check (i.types[op], disp))
      {
	if (i.op[op].disps->X_op == O_constant)
	  {
	    offsetT op_disp = i.op[op].disps->X_add_number;

	    if (i.types[op].bitfield.disp16
		&& (op_disp & ~(offsetT) 0xffff) == 0)
	      {
		/* If this operand is at most 16 bits, convert
		   to a signed 16 bit number and don't use 64bit
		   displacement.  */
		op_disp = (((op_disp & 0xffff) ^ 0x8000) - 0x8000);
		i.types[op].bitfield.disp64 = 0;
	      }
#ifdef BFD64
	    /* Optimize 64-bit displacement to 32-bit for 64-bit BFD.  */
	    if (i.types[op].bitfield.disp32
		&& (op_disp & ~(((offsetT) 2 << 31) - 1)) == 0)
	      {
		/* If this operand is at most 32 bits, convert
		   to a signed 32 bit number and don't use 64bit
		   displacement.  */
		op_disp &= (((offsetT) 2 << 31) - 1);
		op_disp = (op_disp ^ ((offsetT) 1 << 31)) - ((addressT) 1 << 31);
		i.types[op].bitfield.disp64 = 0;
	      }
#endif
	    if (!op_disp && i.types[op].bitfield.baseindex)
	      {
		i.types[op].bitfield.disp8 = 0;
		i.types[op].bitfield.disp16 = 0;
		i.types[op].bitfield.disp32 = 0;
		i.types[op].bitfield.disp32s = 0;
		i.types[op].bitfield.disp64 = 0;
		i.op[op].disps = 0;
		i.disp_operands--;
	      }
	    else if (flag_code == CODE_64BIT)
	      {
		if (fits_in_signed_long (op_disp))
		  {
		    i.types[op].bitfield.disp64 = 0;
		    i.types[op].bitfield.disp32s = 1;
		  }
		if (i.prefix[ADDR_PREFIX]
		    && fits_in_unsigned_long (op_disp))
		  i.types[op].bitfield.disp32 = 1;
	      }
	    if ((i.types[op].bitfield.disp32
		 || i.types[op].bitfield.disp32s
		 || i.types[op].bitfield.disp16)
		&& fits_in_disp8 (op_disp))
	      i.types[op].bitfield.disp8 = 1;
	  }
	else if (i.reloc[op] == BFD_RELOC_386_TLS_DESC_CALL
		 || i.reloc[op] == BFD_RELOC_X86_64_TLSDESC_CALL)
	  {
	    fix_new_exp (frag_now, frag_more (0) - frag_now->fr_literal, 0,
			 i.op[op].disps, 0, i.reloc[op]);
	    i.types[op].bitfield.disp8 = 0;
	    i.types[op].bitfield.disp16 = 0;
	    i.types[op].bitfield.disp32 = 0;
	    i.types[op].bitfield.disp32s = 0;
	    i.types[op].bitfield.disp64 = 0;
	  }
 	else
	  /* We only support 64bit displacement on constants.  */
	  i.types[op].bitfield.disp64 = 0;
      }
}

/* Check if operands are valid for the instruction.  */

static int
check_VecOperands (const insn_template *t)
{
  unsigned int op;

  /* Without VSIB byte, we can't have a vector register for index.  */
  if (!t->opcode_modifier.vecsib
      && i.index_reg
      && (i.index_reg->reg_type.bitfield.xmmword
	  || i.index_reg->reg_type.bitfield.ymmword
	  || i.index_reg->reg_type.bitfield.zmmword))
    {
      i.error = unsupported_vector_index_register;
      return 1;
    }

  /* Check if default mask is allowed.  */
  if (t->opcode_modifier.nodefmask
      && (!i.mask || i.mask->mask->reg_num == 0))
    {
      i.error = no_default_mask;
      return 1;
    }

  /* For VSIB byte, we need a vector register for index, and all vector
     registers must be distinct.  */
  if (t->opcode_modifier.vecsib)
    {
      if (!i.index_reg
	  || !((t->opcode_modifier.vecsib == VecSIB128
		&& i.index_reg->reg_type.bitfield.xmmword)
	       || (t->opcode_modifier.vecsib == VecSIB256
		   && i.index_reg->reg_type.bitfield.ymmword)
	       || (t->opcode_modifier.vecsib == VecSIB512
		   && i.index_reg->reg_type.bitfield.zmmword)))
      {
	i.error = invalid_vsib_address;
	return 1;
      }

      gas_assert (i.reg_operands == 2 || i.mask);
      if (i.reg_operands == 2 && !i.mask)
	{
	  gas_assert (i.types[0].bitfield.regsimd);
	  gas_assert (i.types[0].bitfield.xmmword
		      || i.types[0].bitfield.ymmword);
	  gas_assert (i.types[2].bitfield.regsimd);
	  gas_assert (i.types[2].bitfield.xmmword
		      || i.types[2].bitfield.ymmword);
	  if (operand_check == check_none)
	    return 0;
	  if (register_number (i.op[0].regs)
	      != register_number (i.index_reg)
	      && register_number (i.op[2].regs)
		 != register_number (i.index_reg)
	      && register_number (i.op[0].regs)
		 != register_number (i.op[2].regs))
	    return 0;
	  if (operand_check == check_error)
	    {
	      i.error = invalid_vector_register_set;
	      return 1;
	    }
	  as_warn (_("mask, index, and destination registers should be distinct"));
	}
      else if (i.reg_operands == 1 && i.mask)
	{
	  if (i.types[1].bitfield.regsimd
	      && (i.types[1].bitfield.xmmword
	          || i.types[1].bitfield.ymmword
	          || i.types[1].bitfield.zmmword)
	      && (register_number (i.op[1].regs)
		  == register_number (i.index_reg)))
	    {
	      if (operand_check == check_error)
		{
		  i.error = invalid_vector_register_set;
		  return 1;
		}
	      if (operand_check != check_none)
		as_warn (_("index and destination registers should be distinct"));
	    }
	}
    }

  /* Check if broadcast is supported by the instruction and is applied
     to the memory operand.  */
  if (i.broadcast)
    {
      int broadcasted_opnd_size;

      /* Check if specified broadcast is supported in this instruction,
	 and it's applied to memory operand of DWORD or QWORD type,
	 depending on VecESize.  */
      if (i.broadcast->type != t->opcode_modifier.broadcast
	  || !i.types[i.broadcast->operand].bitfield.mem
	  || (t->opcode_modifier.vecesize == 0
	      && !i.types[i.broadcast->operand].bitfield.dword
	      && !i.types[i.broadcast->operand].bitfield.unspecified)
	  || (t->opcode_modifier.vecesize == 1
	      && !i.types[i.broadcast->operand].bitfield.qword
	      && !i.types[i.broadcast->operand].bitfield.unspecified))
	goto bad_broadcast;

      broadcasted_opnd_size = t->opcode_modifier.vecesize ? 64 : 32;
      if (i.broadcast->type == BROADCAST_1TO16)
	broadcasted_opnd_size <<= 4; /* Broadcast 1to16.  */
      else if (i.broadcast->type == BROADCAST_1TO8)
	broadcasted_opnd_size <<= 3; /* Broadcast 1to8.  */
      else if (i.broadcast->type == BROADCAST_1TO4)
	broadcasted_opnd_size <<= 2; /* Broadcast 1to4.  */
      else if (i.broadcast->type == BROADCAST_1TO2)
	broadcasted_opnd_size <<= 1; /* Broadcast 1to2.  */
      else
	goto bad_broadcast;

      if ((broadcasted_opnd_size == 256
	   && !t->operand_types[i.broadcast->operand].bitfield.ymmword)
	  || (broadcasted_opnd_size == 512
	      && !t->operand_types[i.broadcast->operand].bitfield.zmmword))
	{
	bad_broadcast:
	  i.error = unsupported_broadcast;
	  return 1;
	}
    }
  /* If broadcast is supported in this instruction, we need to check if
     operand of one-element size isn't specified without broadcast.  */
  else if (t->opcode_modifier.broadcast && i.mem_operands)
    {
      /* Find memory operand.  */
      for (op = 0; op < i.operands; op++)
	if (operand_type_check (i.types[op], anymem))
	  break;
      gas_assert (op < i.operands);
      /* Check size of the memory operand.  */
      if ((t->opcode_modifier.vecesize == 0
	   && i.types[op].bitfield.dword)
	  || (t->opcode_modifier.vecesize == 1
	      && i.types[op].bitfield.qword))
	{
	  i.error = broadcast_needed;
	  return 1;
	}
    }

  /* Check if requested masking is supported.  */
  if (i.mask
      && (!t->opcode_modifier.masking
	  || (i.mask->zeroing
	      && t->opcode_modifier.masking == MERGING_MASKING)))
    {
      i.error = unsupported_masking;
      return 1;
    }

  /* Check if masking is applied to dest operand.  */
  if (i.mask && (i.mask->operand != (int) (i.operands - 1)))
    {
      i.error = mask_not_on_destination;
      return 1;
    }

  /* Check RC/SAE.  */
  if (i.rounding)
    {
      if ((i.rounding->type != saeonly
	   && !t->opcode_modifier.staticrounding)
	  || (i.rounding->type == saeonly
	      && (t->opcode_modifier.staticrounding
		  || !t->opcode_modifier.sae)))
	{
	  i.error = unsupported_rc_sae;
	  return 1;
	}
      /* If the instruction has several immediate operands and one of
	 them is rounding, the rounding operand should be the last
	 immediate operand.  */
      if (i.imm_operands > 1
	  && i.rounding->operand != (int) (i.imm_operands - 1))
	{
	  i.error = rc_sae_operand_not_last_imm;
	  return 1;
	}
    }

  /* Check vector Disp8 operand.  */
  if (t->opcode_modifier.disp8memshift
      && i.disp_encoding != disp_encoding_32bit)
    {
      if (i.broadcast)
	i.memshift = t->opcode_modifier.vecesize ? 3 : 2;
      else
	i.memshift = t->opcode_modifier.disp8memshift;

      for (op = 0; op < i.operands; op++)
	if (operand_type_check (i.types[op], disp)
	    && i.op[op].disps->X_op == O_constant)
	  {
	    if (fits_in_disp8 (i.op[op].disps->X_add_number))
	      {
		i.types[op].bitfield.disp8 = 1;
		return 0;
	      }
	    i.types[op].bitfield.disp8 = 0;
	  }
    }

  i.memshift = 0;

  return 0;
}

/* Check if operands are valid for the instruction.  Update VEX
   operand types.  */

static int
VEX_check_operands (const insn_template *t)
{
  if (i.vec_encoding == vex_encoding_evex)
    {
      /* This instruction must be encoded with EVEX prefix.  */
      if (!t->opcode_modifier.evex)
	{
	  i.error = unsupported;
	  return 1;
	}
      return 0;
    }

  if (!t->opcode_modifier.vex)
    {
      /* This instruction template doesn't have VEX prefix.  */
      if (i.vec_encoding != vex_encoding_default)
	{
	  i.error = unsupported;
	  return 1;
	}
      return 0;
    }

  /* Only check VEX_Imm4, which must be the first operand.  */
  if (t->operand_types[0].bitfield.vec_imm4)
    {
      if (i.op[0].imms->X_op != O_constant
	  || !fits_in_imm4 (i.op[0].imms->X_add_number))
	{
	  i.error = bad_imm4;
	  return 1;
	}

      /* Turn off Imm8 so that update_imm won't complain.  */
      i.types[0] = vec_imm4;
    }

  return 0;
}

static const insn_template *
match_template (char mnem_suffix)
{
  /* Points to template once we've found it.  */
  const insn_template *t;
  i386_operand_type overlap0, overlap1, overlap2, overlap3;
  i386_operand_type overlap4;
  unsigned int found_reverse_match;
  i386_opcode_modifier suffix_check, mnemsuf_check;
  i386_operand_type operand_types [MAX_OPERANDS];
  int addr_prefix_disp;
  unsigned int j;
  unsigned int found_cpu_match;
  unsigned int check_register;
  enum i386_error specific_error = 0;

#if MAX_OPERANDS != 5
# error "MAX_OPERANDS must be 5."
#endif

  found_reverse_match = 0;
  addr_prefix_disp = -1;

  memset (&suffix_check, 0, sizeof (suffix_check));
  if (i.suffix == BYTE_MNEM_SUFFIX)
    suffix_check.no_bsuf = 1;
  else if (i.suffix == WORD_MNEM_SUFFIX)
    suffix_check.no_wsuf = 1;
  else if (i.suffix == SHORT_MNEM_SUFFIX)
    suffix_check.no_ssuf = 1;
  else if (i.suffix == LONG_MNEM_SUFFIX)
    suffix_check.no_lsuf = 1;
  else if (i.suffix == QWORD_MNEM_SUFFIX)
    suffix_check.no_qsuf = 1;
  else if (i.suffix == LONG_DOUBLE_MNEM_SUFFIX)
    suffix_check.no_ldsuf = 1;

  memset (&mnemsuf_check, 0, sizeof (mnemsuf_check));
  if (intel_syntax)
    {
      switch (mnem_suffix)
	{
	case BYTE_MNEM_SUFFIX:  mnemsuf_check.no_bsuf = 1; break;
	case WORD_MNEM_SUFFIX:  mnemsuf_check.no_wsuf = 1; break;
	case SHORT_MNEM_SUFFIX: mnemsuf_check.no_ssuf = 1; break;
	case LONG_MNEM_SUFFIX:  mnemsuf_check.no_lsuf = 1; break;
	case QWORD_MNEM_SUFFIX: mnemsuf_check.no_qsuf = 1; break;
	}
    }

  /* Must have right number of operands.  */
  i.error = number_of_operands_mismatch;

  for (t = current_templates->start; t < current_templates->end; t++)
    {
      addr_prefix_disp = -1;

      if (i.operands != t->operands)
	continue;

      /* Check processor support.  */
      i.error = unsupported;
      found_cpu_match = (cpu_flags_match (t)
			 == CPU_FLAGS_PERFECT_MATCH);
      if (!found_cpu_match)
	continue;

      /* Check old gcc support. */
      i.error = old_gcc_only;
      if (!old_gcc && t->opcode_modifier.oldgcc)
	continue;

      /* Check AT&T mnemonic.   */
      i.error = unsupported_with_intel_mnemonic;
      if (intel_mnemonic && t->opcode_modifier.attmnemonic)
	continue;

      /* Check AT&T/Intel syntax and Intel64/AMD64 ISA.   */
      i.error = unsupported_syntax;
      if ((intel_syntax && t->opcode_modifier.attsyntax)
	  || (!intel_syntax && t->opcode_modifier.intelsyntax)
	  || (intel64 && t->opcode_modifier.amd64)
	  || (!intel64 && t->opcode_modifier.intel64))
	continue;

      /* Check the suffix, except for some instructions in intel mode.  */
      i.error = invalid_instruction_suffix;
      if ((!intel_syntax || !t->opcode_modifier.ignoresize)
	  && ((t->opcode_modifier.no_bsuf && suffix_check.no_bsuf)
	      || (t->opcode_modifier.no_wsuf && suffix_check.no_wsuf)
	      || (t->opcode_modifier.no_lsuf && suffix_check.no_lsuf)
	      || (t->opcode_modifier.no_ssuf && suffix_check.no_ssuf)
	      || (t->opcode_modifier.no_qsuf && suffix_check.no_qsuf)
	      || (t->opcode_modifier.no_ldsuf && suffix_check.no_ldsuf)))
	continue;
      /* In Intel mode all mnemonic suffixes must be explicitly allowed.  */
      if ((t->opcode_modifier.no_bsuf && mnemsuf_check.no_bsuf)
	  || (t->opcode_modifier.no_wsuf && mnemsuf_check.no_wsuf)
	  || (t->opcode_modifier.no_lsuf && mnemsuf_check.no_lsuf)
	  || (t->opcode_modifier.no_ssuf && mnemsuf_check.no_ssuf)
	  || (t->opcode_modifier.no_qsuf && mnemsuf_check.no_qsuf)
	  || (t->opcode_modifier.no_ldsuf && mnemsuf_check.no_ldsuf))
	continue;

      if (!operand_size_match (t))
	continue;

      for (j = 0; j < MAX_OPERANDS; j++)
	operand_types[j] = t->operand_types[j];

      /* In general, don't allow 64-bit operands in 32-bit mode.  */
      if (i.suffix == QWORD_MNEM_SUFFIX
	  && flag_code != CODE_64BIT
	  && (intel_syntax
	      ? (!t->opcode_modifier.ignoresize
		 && !intel_float_operand (t->name))
	      : intel_float_operand (t->name) != 2)
	  && ((!operand_types[0].bitfield.regmmx
	       && !operand_types[0].bitfield.regsimd)
	      || (!operand_types[t->operands > 1].bitfield.regmmx
		  && !operand_types[t->operands > 1].bitfield.regsimd))
	  && (t->base_opcode != 0x0fc7
	      || t->extension_opcode != 1 /* cmpxchg8b */))
	continue;

      /* In general, don't allow 32-bit operands on pre-386.  */
      else if (i.suffix == LONG_MNEM_SUFFIX
	       && !cpu_arch_flags.bitfield.cpui386
	       && (intel_syntax
		   ? (!t->opcode_modifier.ignoresize
		      && !intel_float_operand (t->name))
		   : intel_float_operand (t->name) != 2)
	       && ((!operand_types[0].bitfield.regmmx
		    && !operand_types[0].bitfield.regsimd)
		   || (!operand_types[t->operands > 1].bitfield.regmmx
		       && !operand_types[t->operands > 1].bitfield.regsimd)))
	continue;

      /* Do not verify operands when there are none.  */
      else
	{
	  if (!t->operands)
	    /* We've found a match; break out of loop.  */
	    break;
	}

      /* Address size prefix will turn Disp64/Disp32/Disp16 operand
	 into Disp32/Disp16/Disp32 operand.  */
      if (i.prefix[ADDR_PREFIX] != 0)
	  {
	    /* There should be only one Disp operand.  */
	    switch (flag_code)
	    {
	    case CODE_16BIT:
	      for (j = 0; j < MAX_OPERANDS; j++)
		{
		  if (operand_types[j].bitfield.disp16)
		    {
		      addr_prefix_disp = j;
		      operand_types[j].bitfield.disp32 = 1;
		      operand_types[j].bitfield.disp16 = 0;
		      break;
		    }
		}
	      break;
	    case CODE_32BIT:
	      for (j = 0; j < MAX_OPERANDS; j++)
		{
		  if (operand_types[j].bitfield.disp32)
		    {
		      addr_prefix_disp = j;
		      operand_types[j].bitfield.disp32 = 0;
		      operand_types[j].bitfield.disp16 = 1;
		      break;
		    }
		}
	      break;
	    case CODE_64BIT:
	      for (j = 0; j < MAX_OPERANDS; j++)
		{
		  if (operand_types[j].bitfield.disp64)
		    {
		      addr_prefix_disp = j;
		      operand_types[j].bitfield.disp64 = 0;
		      operand_types[j].bitfield.disp32 = 1;
		      break;
		    }
		}
	      break;
	    }
	  }

      /* Force 0x8b encoding for "mov foo@GOT, %eax".  */
      if (i.reloc[0] == BFD_RELOC_386_GOT32 && t->base_opcode == 0xa0)
	continue;

      /* We check register size if needed.  */
      check_register = t->opcode_modifier.checkregsize;
      overlap0 = operand_type_and (i.types[0], operand_types[0]);
      switch (t->operands)
	{
	case 1:
	  if (!operand_type_match (overlap0, i.types[0]))
	    continue;
	  break;
	case 2:
	  /* xchg %eax, %eax is a special case. It is an alias for nop
	     only in 32bit mode and we can use opcode 0x90.  In 64bit
	     mode, we can't use 0x90 for xchg %eax, %eax since it should
	     zero-extend %eax to %rax.  */
	  if (flag_code == CODE_64BIT
	      && t->base_opcode == 0x90
	      && operand_type_equal (&i.types [0], &acc32)
	      && operand_type_equal (&i.types [1], &acc32))
	    continue;
	  /* If we want store form, we reverse direction of operands.  */
	  if (i.dir_encoding == dir_encoding_store
	      && t->opcode_modifier.d)
	    goto check_reverse;
	  /* Fall through.  */

	case 3:
	  /* If we want store form, we skip the current load.  */
	  if (i.dir_encoding == dir_encoding_store
	      && i.mem_operands == 0
	      && t->opcode_modifier.load)
	    continue;
	  /* Fall through.  */
	case 4:
	case 5:
	  overlap1 = operand_type_and (i.types[1], operand_types[1]);
	  if (!operand_type_match (overlap0, i.types[0])
	      || !operand_type_match (overlap1, i.types[1])
	      || (check_register
		  && !operand_type_register_match (i.types[0],
						   operand_types[0],
						   i.types[1],
						   operand_types[1])))
	    {
	      /* Check if other direction is valid ...  */
	      if (!t->opcode_modifier.d && !t->opcode_modifier.floatd)
		continue;

check_reverse:
	      /* Try reversing direction of operands.  */
	      overlap0 = operand_type_and (i.types[0], operand_types[1]);
	      overlap1 = operand_type_and (i.types[1], operand_types[0]);
	      if (!operand_type_match (overlap0, i.types[0])
		  || !operand_type_match (overlap1, i.types[1])
		  || (check_register
		      && !operand_type_register_match (i.types[0],
						       operand_types[1],
						       i.types[1],
						       operand_types[0])))
		{
		  /* Does not match either direction.  */
		  continue;
		}
	      /* found_reverse_match holds which of D or FloatDR
		 we've found.  */
	      if (t->opcode_modifier.d)
		found_reverse_match = Opcode_D;
	      else if (t->opcode_modifier.floatd)
		found_reverse_match = Opcode_FloatD;
	      else
		found_reverse_match = 0;
	      if (t->opcode_modifier.floatr)
		found_reverse_match |= Opcode_FloatR;
	    }
	  else
	    {
	      /* Found a forward 2 operand match here.  */
	      switch (t->operands)
		{
		case 5:
		  overlap4 = operand_type_and (i.types[4],
					       operand_types[4]);
		  /* Fall through.  */
		case 4:
		  overlap3 = operand_type_and (i.types[3],
					       operand_types[3]);
		  /* Fall through.  */
		case 3:
		  overlap2 = operand_type_and (i.types[2],
					       operand_types[2]);
		  break;
		}

	      switch (t->operands)
		{
		case 5:
		  if (!operand_type_match (overlap4, i.types[4])
		      || !operand_type_register_match (i.types[3],
						       operand_types[3],
						       i.types[4],
						       operand_types[4]))
		    continue;
		  /* Fall through.  */
		case 4:
		  if (!operand_type_match (overlap3, i.types[3])
		      || (check_register
			  && !operand_type_register_match (i.types[2],
							   operand_types[2],
							   i.types[3],
							   operand_types[3])))
		    continue;
		  /* Fall through.  */
		case 3:
		  /* Here we make use of the fact that there are no
		     reverse match 3 operand instructions, and all 3
		     operand instructions only need to be checked for
		     register consistency between operands 2 and 3.  */
		  if (!operand_type_match (overlap2, i.types[2])
		      || (check_register
			  && !operand_type_register_match (i.types[1],
							   operand_types[1],
							   i.types[2],
							   operand_types[2])))
		    continue;
		  break;
		}
	    }
	  /* Found either forward/reverse 2, 3 or 4 operand match here:
	     slip through to break.  */
	}
      if (!found_cpu_match)
	{
	  found_reverse_match = 0;
	  continue;
	}

      /* Check if vector and VEX operands are valid.  */
      if (check_VecOperands (t) || VEX_check_operands (t))
	{
	  specific_error = i.error;
	  continue;
	}

      /* We've found a match; break out of loop.  */
      break;
    }

  if (t == current_templates->end)
    {
      /* We found no match.  */
      const char *err_msg;
      switch (specific_error ? specific_error : i.error)
	{
	default:
	  abort ();
	case operand_size_mismatch:
	  err_msg = _("operand size mismatch");
	  break;
	case operand_type_mismatch:
	  err_msg = _("operand type mismatch");
	  break;
	case register_type_mismatch:
	  err_msg = _("register type mismatch");
	  break;
	case number_of_operands_mismatch:
	  err_msg = _("number of operands mismatch");
	  break;
	case invalid_instruction_suffix:
	  err_msg = _("invalid instruction suffix");
	  break;
	case bad_imm4:
	  err_msg = _("constant doesn't fit in 4 bits");
	  break;
	case old_gcc_only:
	  err_msg = _("only supported with old gcc");
	  break;
	case unsupported_with_intel_mnemonic:
	  err_msg = _("unsupported with Intel mnemonic");
	  break;
	case unsupported_syntax:
	  err_msg = _("unsupported syntax");
	  break;
	case unsupported:
	  as_bad (_("unsupported instruction `%s'"),
		  current_templates->start->name);
	  return NULL;
	case invalid_vsib_address:
	  err_msg = _("invalid VSIB address");
	  break;
	case invalid_vector_register_set:
	  err_msg = _("mask, index, and destination registers must be distinct");
	  break;
	case unsupported_vector_index_register:
	  err_msg = _("unsupported vector index register");
	  break;
	case unsupported_broadcast:
	  err_msg = _("unsupported broadcast");
	  break;
	case broadcast_not_on_src_operand:
	  err_msg = _("broadcast not on source memory operand");
	  break;
	case broadcast_needed:
	  err_msg = _("broadcast is needed for operand of such type");
	  break;
	case unsupported_masking:
	  err_msg = _("unsupported masking");
	  break;
	case mask_not_on_destination:
	  err_msg = _("mask not on destination operand");
	  break;
	case no_default_mask:
	  err_msg = _("default mask isn't allowed");
	  break;
	case unsupported_rc_sae:
	  err_msg = _("unsupported static rounding/sae");
	  break;
	case rc_sae_operand_not_last_imm:
	  if (intel_syntax)
	    err_msg = _("RC/SAE operand must precede immediate operands");
	  else
	    err_msg = _("RC/SAE operand must follow immediate operands");
	  break;
	case invalid_register_operand:
	  err_msg = _("invalid register operand");
	  break;
	}
      as_bad (_("%s for `%s'"), err_msg,
	      current_templates->start->name);
      return NULL;
    }

  if (!quiet_warnings)
    {
      if (!intel_syntax
	  && (i.types[0].bitfield.jumpabsolute
	      != operand_types[0].bitfield.jumpabsolute))
	{
	  as_warn (_("indirect %s without `*'"), t->name);
	}

      if (t->opcode_modifier.isprefix
	  && t->opcode_modifier.ignoresize)
	{
	  /* Warn them that a data or address size prefix doesn't
	     affect assembly of the next line of code.  */
	  as_warn (_("stand-alone `%s' prefix"), t->name);
	}
    }

  /* Copy the template we found.  */
  i.tm = *t;

  if (addr_prefix_disp != -1)
    i.tm.operand_types[addr_prefix_disp]
      = operand_types[addr_prefix_disp];

  if (found_reverse_match)
    {
      /* If we found a reverse match we must alter the opcode
	 direction bit.  found_reverse_match holds bits to change
	 (different for int & float insns).  */

      i.tm.base_opcode ^= found_reverse_match;

      i.tm.operand_types[0] = operand_types[1];
      i.tm.operand_types[1] = operand_types[0];
    }

  return t;
}

static int
check_string (void)
{
  int mem_op = operand_type_check (i.types[0], anymem) ? 0 : 1;
  if (i.tm.operand_types[mem_op].bitfield.esseg)
    {
      if (i.seg[0] != NULL && i.seg[0] != &es)
	{
	  as_bad (_("`%s' operand %d must use `%ses' segment"),
		  i.tm.name,
		  mem_op + 1,
		  register_prefix);
	  return 0;
	}
      /* There's only ever one segment override allowed per instruction.
	 This instruction possibly has a legal segment override on the
	 second operand, so copy the segment to where non-string
	 instructions store it, allowing common code.  */
      i.seg[0] = i.seg[1];
    }
  else if (i.tm.operand_types[mem_op + 1].bitfield.esseg)
    {
      if (i.seg[1] != NULL && i.seg[1] != &es)
	{
	  as_bad (_("`%s' operand %d must use `%ses' segment"),
		  i.tm.name,
		  mem_op + 2,
		  register_prefix);
	  return 0;
	}
    }
  return 1;
}

static int
process_suffix (void)
{
  /* If matched instruction specifies an explicit instruction mnemonic
     suffix, use it.  */
  if (i.tm.opcode_modifier.size16)
    i.suffix = WORD_MNEM_SUFFIX;
  else if (i.tm.opcode_modifier.size32)
    i.suffix = LONG_MNEM_SUFFIX;
  else if (i.tm.opcode_modifier.size64)
    i.suffix = QWORD_MNEM_SUFFIX;
  else if (i.reg_operands)
    {
      /* If there's no instruction mnemonic suffix we try to invent one
	 based on register operands.  */
      if (!i.suffix)
	{
	  /* We take i.suffix from the last register operand specified,
	     Destination register type is more significant than source
	     register type.  crc32 in SSE4.2 prefers source register
	     type. */
	  if (i.tm.base_opcode == 0xf20f38f1)
	    {
	      if (i.types[0].bitfield.reg && i.types[0].bitfield.word)
		i.suffix = WORD_MNEM_SUFFIX;
	      else if (i.types[0].bitfield.reg && i.types[0].bitfield.dword)
		i.suffix = LONG_MNEM_SUFFIX;
	      else if (i.types[0].bitfield.reg && i.types[0].bitfield.qword)
		i.suffix = QWORD_MNEM_SUFFIX;
	    }
	  else if (i.tm.base_opcode == 0xf20f38f0)
	    {
	      if (i.types[0].bitfield.reg && i.types[0].bitfield.byte)
		i.suffix = BYTE_MNEM_SUFFIX;
	    }

	  if (!i.suffix)
	    {
	      int op;

	      if (i.tm.base_opcode == 0xf20f38f1
		  || i.tm.base_opcode == 0xf20f38f0)
		{
		  /* We have to know the operand size for crc32.  */
		  as_bad (_("ambiguous memory operand size for `%s`"),
			  i.tm.name);
		  return 0;
		}

	      for (op = i.operands; --op >= 0;)
		if (!i.tm.operand_types[op].bitfield.inoutportreg
		    && !i.tm.operand_types[op].bitfield.shiftcount)
		  {
		    if (i.types[op].bitfield.reg && i.types[op].bitfield.byte)
		      {
			i.suffix = BYTE_MNEM_SUFFIX;
			break;
		      }
		    if (i.types[op].bitfield.reg && i.types[op].bitfield.word)
		      {
			i.suffix = WORD_MNEM_SUFFIX;
			break;
		      }
		    if (i.types[op].bitfield.reg && i.types[op].bitfield.dword)
		      {
			i.suffix = LONG_MNEM_SUFFIX;
			break;
		      }
		    if (i.types[op].bitfield.reg && i.types[op].bitfield.qword)
		      {
			i.suffix = QWORD_MNEM_SUFFIX;
			break;
		      }
		  }
	    }
	}
      else if (i.suffix == BYTE_MNEM_SUFFIX)
	{
	  if (intel_syntax
	      && i.tm.opcode_modifier.ignoresize
	      && i.tm.opcode_modifier.no_bsuf)
	    i.suffix = 0;
	  else if (!check_byte_reg ())
	    return 0;
	}
      else if (i.suffix == LONG_MNEM_SUFFIX)
	{
	  if (intel_syntax
	      && i.tm.opcode_modifier.ignoresize
	      && i.tm.opcode_modifier.no_lsuf)
	    i.suffix = 0;
	  else if (!check_long_reg ())
	    return 0;
	}
      else if (i.suffix == QWORD_MNEM_SUFFIX)
	{
	  if (intel_syntax
	      && i.tm.opcode_modifier.ignoresize
	      && i.tm.opcode_modifier.no_qsuf)
	    i.suffix = 0;
	  else if (!check_qword_reg ())
	    return 0;
	}
      else if (i.suffix == WORD_MNEM_SUFFIX)
	{
	  if (intel_syntax
	      && i.tm.opcode_modifier.ignoresize
	      && i.tm.opcode_modifier.no_wsuf)
	    i.suffix = 0;
	  else if (!check_word_reg ())
	    return 0;
	}
      else if (i.suffix == XMMWORD_MNEM_SUFFIX
	       || i.suffix == YMMWORD_MNEM_SUFFIX
	       || i.suffix == ZMMWORD_MNEM_SUFFIX)
	{
	  /* Skip if the instruction has x/y/z suffix.  match_template
	     should check if it is a valid suffix.  */
	}
      else if (intel_syntax && i.tm.opcode_modifier.ignoresize)
	/* Do nothing if the instruction is going to ignore the prefix.  */
	;
      else
	abort ();
    }
  else if (i.tm.opcode_modifier.defaultsize
	   && !i.suffix
	   /* exclude fldenv/frstor/fsave/fstenv */
	   && i.tm.opcode_modifier.no_ssuf)
    {
      i.suffix = stackop_size;
    }
  else if (intel_syntax
	   && !i.suffix
	   && (i.tm.operand_types[0].bitfield.jumpabsolute
	       || i.tm.opcode_modifier.jumpbyte
	       || i.tm.opcode_modifier.jumpintersegment
	       || (i.tm.base_opcode == 0x0f01 /* [ls][gi]dt */
		   && i.tm.extension_opcode <= 3)))
    {
      switch (flag_code)
	{
	case CODE_64BIT:
	  if (!i.tm.opcode_modifier.no_qsuf)
	    {
	      i.suffix = QWORD_MNEM_SUFFIX;
	      break;
	    }
	  /* Fall through.  */
	case CODE_32BIT:
	  if (!i.tm.opcode_modifier.no_lsuf)
	    i.suffix = LONG_MNEM_SUFFIX;
	  break;
	case CODE_16BIT:
	  if (!i.tm.opcode_modifier.no_wsuf)
	    i.suffix = WORD_MNEM_SUFFIX;
	  break;
	}
    }

  if (!i.suffix)
    {
      if (!intel_syntax)
	{
	  if (i.tm.opcode_modifier.w)
	    {
	      as_bad (_("no instruction mnemonic suffix given and "
			"no register operands; can't size instruction"));
	      return 0;
	    }
	}
      else
	{
	  unsigned int suffixes;

	  suffixes = !i.tm.opcode_modifier.no_bsuf;
	  if (!i.tm.opcode_modifier.no_wsuf)
	    suffixes |= 1 << 1;
	  if (!i.tm.opcode_modifier.no_lsuf)
	    suffixes |= 1 << 2;
	  if (!i.tm.opcode_modifier.no_ldsuf)
	    suffixes |= 1 << 3;
	  if (!i.tm.opcode_modifier.no_ssuf)
	    suffixes |= 1 << 4;
	  if (flag_code == CODE_64BIT && !i.tm.opcode_modifier.no_qsuf)
	    suffixes |= 1 << 5;

	  /* There are more than suffix matches.  */
	  if (i.tm.opcode_modifier.w
	      || ((suffixes & (suffixes - 1))
		  && !i.tm.opcode_modifier.defaultsize
		  && !i.tm.opcode_modifier.ignoresize))
	    {
	      as_bad (_("ambiguous operand size for `%s'"), i.tm.name);
	      return 0;
	    }
	}
    }

  /* Change the opcode based on the operand size given by i.suffix;
     We don't need to change things for byte insns.  */

  if (i.suffix
      && i.suffix != BYTE_MNEM_SUFFIX
      && i.suffix != XMMWORD_MNEM_SUFFIX
      && i.suffix != YMMWORD_MNEM_SUFFIX
      && i.suffix != ZMMWORD_MNEM_SUFFIX)
    {
      /* It's not a byte, select word/dword operation.  */
      if (i.tm.opcode_modifier.w)
	{
	  if (i.tm.opcode_modifier.shortform)
	    i.tm.base_opcode |= 8;
	  else
	    i.tm.base_opcode |= 1;
	}

      /* Now select between word & dword operations via the operand
	 size prefix, except for instructions that will ignore this
	 prefix anyway.  */
      if (i.tm.opcode_modifier.addrprefixop0)
	{
	  /* The address size override prefix changes the size of the
	     first operand.  */
	  if ((flag_code == CODE_32BIT
	       && i.op->regs[0].reg_type.bitfield.word)
	      || (flag_code != CODE_32BIT
		  && i.op->regs[0].reg_type.bitfield.dword))
	    if (!add_prefix (ADDR_PREFIX_OPCODE))
	      return 0;
	}
      else if (i.suffix != QWORD_MNEM_SUFFIX
	       && i.suffix != LONG_DOUBLE_MNEM_SUFFIX
	       && !i.tm.opcode_modifier.ignoresize
	       && !i.tm.opcode_modifier.floatmf
	       && ((i.suffix == LONG_MNEM_SUFFIX) == (flag_code == CODE_16BIT)
		   || (flag_code == CODE_64BIT
		       && i.tm.opcode_modifier.jumpbyte)))
	{
	  unsigned int prefix = DATA_PREFIX_OPCODE;

	  if (i.tm.opcode_modifier.jumpbyte) /* jcxz, loop */
	    prefix = ADDR_PREFIX_OPCODE;

	  if (!add_prefix (prefix))
	    return 0;
	}

      /* Set mode64 for an operand.  */
      if (i.suffix == QWORD_MNEM_SUFFIX
	  && flag_code == CODE_64BIT
	  && !i.tm.opcode_modifier.norex64)
	{
	  /* Special case for xchg %rax,%rax.  It is NOP and doesn't
	     need rex64.  cmpxchg8b is also a special case. */
	  if (! (i.operands == 2
		 && i.tm.base_opcode == 0x90
		 && i.tm.extension_opcode == None
		 && operand_type_equal (&i.types [0], &acc64)
		 && operand_type_equal (&i.types [1], &acc64))
	      && ! (i.operands == 1
		    && i.tm.base_opcode == 0xfc7
		    && i.tm.extension_opcode == 1
		    && !operand_type_check (i.types [0], reg)
		    && operand_type_check (i.types [0], anymem)))
	    i.rex |= REX_W;
	}

      /* Size floating point instruction.  */
      if (i.suffix == LONG_MNEM_SUFFIX)
	if (i.tm.opcode_modifier.floatmf)
	  i.tm.base_opcode ^= 4;
    }

  return 1;
}

static int
check_byte_reg (void)
{
  int op;

  for (op = i.operands; --op >= 0;)
    {
      /* Skip non-register operands. */
      if (!i.types[op].bitfield.reg)
	continue;

      /* If this is an eight bit register, it's OK.  If it's the 16 or
	 32 bit version of an eight bit register, we will just use the
	 low portion, and that's OK too.  */
      if (i.types[op].bitfield.byte)
	continue;

      /* I/O port address operands are OK too.  */
      if (i.tm.operand_types[op].bitfield.inoutportreg)
	continue;

      /* crc32 doesn't generate this warning.  */
      if (i.tm.base_opcode == 0xf20f38f0)
	continue;

      if ((i.types[op].bitfield.word
	   || i.types[op].bitfield.dword
	   || i.types[op].bitfield.qword)
	  && i.op[op].regs->reg_num < 4
	  /* Prohibit these changes in 64bit mode, since the lowering
	     would be more complicated.  */
	  && flag_code != CODE_64BIT)
	{
#if REGISTER_WARNINGS
	  if (!quiet_warnings)
	    as_warn (_("using `%s%s' instead of `%s%s' due to `%c' suffix"),
		     register_prefix,
		     (i.op[op].regs + (i.types[op].bitfield.word
				       ? REGNAM_AL - REGNAM_AX
				       : REGNAM_AL - REGNAM_EAX))->reg_name,
		     register_prefix,
		     i.op[op].regs->reg_name,
		     i.suffix);
#endif
	  continue;
	}
      /* Any other register is bad.  */
      if (i.types[op].bitfield.reg
	  || i.types[op].bitfield.regmmx
	  || i.types[op].bitfield.regsimd
	  || i.types[op].bitfield.sreg2
	  || i.types[op].bitfield.sreg3
	  || i.types[op].bitfield.control
	  || i.types[op].bitfield.debug
	  || i.types[op].bitfield.test)
	{
	  as_bad (_("`%s%s' not allowed with `%s%c'"),
		  register_prefix,
		  i.op[op].regs->reg_name,
		  i.tm.name,
		  i.suffix);
	  return 0;
	}
    }
  return 1;
}

static int
check_long_reg (void)
{
  int op;

  for (op = i.operands; --op >= 0;)
    /* Skip non-register operands. */
    if (!i.types[op].bitfield.reg)
      continue;
    /* Reject eight bit registers, except where the template requires
       them. (eg. movzb)  */
    else if (i.types[op].bitfield.byte
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && (i.tm.operand_types[op].bitfield.word
		 || i.tm.operand_types[op].bitfield.dword))
      {
	as_bad (_("`%s%s' not allowed with `%s%c'"),
		register_prefix,
		i.op[op].regs->reg_name,
		i.tm.name,
		i.suffix);
	return 0;
      }
    /* Warn if the e prefix on a general reg is missing.  */
    else if ((!quiet_warnings || flag_code == CODE_64BIT)
	     && i.types[op].bitfield.word
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && i.tm.operand_types[op].bitfield.dword)
      {
	/* Prohibit these changes in the 64bit mode, since the
	   lowering is more complicated.  */
	if (flag_code == CODE_64BIT)
	  {
	    as_bad (_("incorrect register `%s%s' used with `%c' suffix"),
		    register_prefix, i.op[op].regs->reg_name,
		    i.suffix);
	    return 0;
	  }
#if REGISTER_WARNINGS
	as_warn (_("using `%s%s' instead of `%s%s' due to `%c' suffix"),
		 register_prefix,
		 (i.op[op].regs + REGNAM_EAX - REGNAM_AX)->reg_name,
		 register_prefix, i.op[op].regs->reg_name, i.suffix);
#endif
      }
    /* Warn if the r prefix on a general reg is present.  */
    else if (i.types[op].bitfield.qword
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && i.tm.operand_types[op].bitfield.dword)
      {
	if (intel_syntax
	    && i.tm.opcode_modifier.toqword
	    && !i.types[0].bitfield.regsimd)
	  {
	    /* Convert to QWORD.  We want REX byte. */
	    i.suffix = QWORD_MNEM_SUFFIX;
	  }
	else
	  {
	    as_bad (_("incorrect register `%s%s' used with `%c' suffix"),
		    register_prefix, i.op[op].regs->reg_name,
		    i.suffix);
	    return 0;
	  }
      }
  return 1;
}

static int
check_qword_reg (void)
{
  int op;

  for (op = i.operands; --op >= 0; )
    /* Skip non-register operands. */
    if (!i.types[op].bitfield.reg)
      continue;
    /* Reject eight bit registers, except where the template requires
       them. (eg. movzb)  */
    else if (i.types[op].bitfield.byte
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && (i.tm.operand_types[op].bitfield.word
		 || i.tm.operand_types[op].bitfield.dword))
      {
	as_bad (_("`%s%s' not allowed with `%s%c'"),
		register_prefix,
		i.op[op].regs->reg_name,
		i.tm.name,
		i.suffix);
	return 0;
      }
    /* Warn if the r prefix on a general reg is missing.  */
    else if ((i.types[op].bitfield.word
	      || i.types[op].bitfield.dword)
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && i.tm.operand_types[op].bitfield.qword)
      {
	/* Prohibit these changes in the 64bit mode, since the
	   lowering is more complicated.  */
	if (intel_syntax
	    && i.tm.opcode_modifier.todword
	    && !i.types[0].bitfield.regsimd)
	  {
	    /* Convert to DWORD.  We don't want REX byte. */
	    i.suffix = LONG_MNEM_SUFFIX;
	  }
	else
	  {
	    as_bad (_("incorrect register `%s%s' used with `%c' suffix"),
		    register_prefix, i.op[op].regs->reg_name,
		    i.suffix);
	    return 0;
	  }
      }
  return 1;
}

static int
check_word_reg (void)
{
  int op;
  for (op = i.operands; --op >= 0;)
    /* Skip non-register operands. */
    if (!i.types[op].bitfield.reg)
      continue;
    /* Reject eight bit registers, except where the template requires
       them. (eg. movzb)  */
    else if (i.types[op].bitfield.byte
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && (i.tm.operand_types[op].bitfield.word
		 || i.tm.operand_types[op].bitfield.dword))
      {
	as_bad (_("`%s%s' not allowed with `%s%c'"),
		register_prefix,
		i.op[op].regs->reg_name,
		i.tm.name,
		i.suffix);
	return 0;
      }
    /* Warn if the e or r prefix on a general reg is present.  */
    else if ((!quiet_warnings || flag_code == CODE_64BIT)
	     && (i.types[op].bitfield.dword
		 || i.types[op].bitfield.qword)
	     && (i.tm.operand_types[op].bitfield.reg
		 || i.tm.operand_types[op].bitfield.acc)
	     && i.tm.operand_types[op].bitfield.word)
      {
	/* Prohibit these changes in the 64bit mode, since the
	   lowering is more complicated.  */
	if (flag_code == CODE_64BIT)
	  {
	    as_bad (_("incorrect register `%s%s' used with `%c' suffix"),
		    register_prefix, i.op[op].regs->reg_name,
		    i.suffix);
	    return 0;
	  }
#if REGISTER_WARNINGS
	as_warn (_("using `%s%s' instead of `%s%s' due to `%c' suffix"),
		 register_prefix,
		 (i.op[op].regs + REGNAM_AX - REGNAM_EAX)->reg_name,
		 register_prefix, i.op[op].regs->reg_name, i.suffix);
#endif
      }
  return 1;
}

static int
update_imm (unsigned int j)
{
  i386_operand_type overlap = i.types[j];
  if ((overlap.bitfield.imm8
       || overlap.bitfield.imm8s
       || overlap.bitfield.imm16
       || overlap.bitfield.imm32
       || overlap.bitfield.imm32s
       || overlap.bitfield.imm64)
      && !operand_type_equal (&overlap, &imm8)
      && !operand_type_equal (&overlap, &imm8s)
      && !operand_type_equal (&overlap, &imm16)
      && !operand_type_equal (&overlap, &imm32)
      && !operand_type_equal (&overlap, &imm32s)
      && !operand_type_equal (&overlap, &imm64))
    {
      if (i.suffix)
	{
	  i386_operand_type temp;

	  operand_type_set (&temp, 0);
	  if (i.suffix == BYTE_MNEM_SUFFIX)
	    {
	      temp.bitfield.imm8 = overlap.bitfield.imm8;
	      temp.bitfield.imm8s = overlap.bitfield.imm8s;
	    }
	  else if (i.suffix == WORD_MNEM_SUFFIX)
	    temp.bitfield.imm16 = overlap.bitfield.imm16;
	  else if (i.suffix == QWORD_MNEM_SUFFIX)
	    {
	      temp.bitfield.imm64 = overlap.bitfield.imm64;
	      temp.bitfield.imm32s = overlap.bitfield.imm32s;
	    }
	  else
	    temp.bitfield.imm32 = overlap.bitfield.imm32;
	  overlap = temp;
	}
      else if (operand_type_equal (&overlap, &imm16_32_32s)
	       || operand_type_equal (&overlap, &imm16_32)
	       || operand_type_equal (&overlap, &imm16_32s))
	{
	  if ((flag_code == CODE_16BIT) ^ (i.prefix[DATA_PREFIX] != 0))
	    overlap = imm16;
	  else
	    overlap = imm32s;
	}
      if (!operand_type_equal (&overlap, &imm8)
	  && !operand_type_equal (&overlap, &imm8s)
	  && !operand_type_equal (&overlap, &imm16)
	  && !operand_type_equal (&overlap, &imm32)
	  && !operand_type_equal (&overlap, &imm32s)
	  && !operand_type_equal (&overlap, &imm64))
	{
	  as_bad (_("no instruction mnemonic suffix given; "
		    "can't determine immediate size"));
	  return 0;
	}
    }
  i.types[j] = overlap;

  return 1;
}

static int
finalize_imm (void)
{
  unsigned int j, n;

  /* Update the first 2 immediate operands.  */
  n = i.operands > 2 ? 2 : i.operands;
  if (n)
    {
      for (j = 0; j < n; j++)
	if (update_imm (j) == 0)
	  return 0;

      /* The 3rd operand can't be immediate operand.  */
      gas_assert (operand_type_check (i.types[2], imm) == 0);
    }

  return 1;
}

static int
process_operands (void)
{
  /* Default segment register this instruction will use for memory
     accesses.  0 means unknown.  This is only for optimizing out
     unnecessary segment overrides.  */
  const seg_entry *default_seg = 0;

  if (i.tm.opcode_modifier.sse2avx && i.tm.opcode_modifier.vexvvvv)
    {
      unsigned int dupl = i.operands;
      unsigned int dest = dupl - 1;
      unsigned int j;

      /* The destination must be an xmm register.  */
      gas_assert (i.reg_operands
		  && MAX_OPERANDS > dupl
		  && operand_type_equal (&i.types[dest], &regxmm));

      if (i.tm.operand_types[0].bitfield.acc
	  && i.tm.operand_types[0].bitfield.xmmword)
	{
	  if (i.tm.opcode_modifier.vexsources == VEX3SOURCES)
	    {
	      /* Keep xmm0 for instructions with VEX prefix and 3
		 sources.  */
	      i.tm.operand_types[0].bitfield.acc = 0;
	      i.tm.operand_types[0].bitfield.regsimd = 1;
	      goto duplicate;
	    }
	  else
	    {
	      /* We remove the first xmm0 and keep the number of
		 operands unchanged, which in fact duplicates the
		 destination.  */
	      for (j = 1; j < i.operands; j++)
		{
		  i.op[j - 1] = i.op[j];
		  i.types[j - 1] = i.types[j];
		  i.tm.operand_types[j - 1] = i.tm.operand_types[j];
		}
	    }
	}
      else if (i.tm.opcode_modifier.implicit1stxmm0)
	{
	  gas_assert ((MAX_OPERANDS - 1) > dupl
		      && (i.tm.opcode_modifier.vexsources
			  == VEX3SOURCES));

	  /* Add the implicit xmm0 for instructions with VEX prefix
	     and 3 sources.  */
	  for (j = i.operands; j > 0; j--)
	    {
	      i.op[j] = i.op[j - 1];
	      i.types[j] = i.types[j - 1];
	      i.tm.operand_types[j] = i.tm.operand_types[j - 1];
	    }
	  i.op[0].regs
	    = (const reg_entry *) hash_find (reg_hash, "xmm0");
	  i.types[0] = regxmm;
	  i.tm.operand_types[0] = regxmm;

	  i.operands += 2;
	  i.reg_operands += 2;
	  i.tm.operands += 2;

	  dupl++;
	  dest++;
	  i.op[dupl] = i.op[dest];
	  i.types[dupl] = i.types[dest];
	  i.tm.operand_types[dupl] = i.tm.operand_types[dest];
	}
      else
	{
duplicate:
	  i.operands++;
	  i.reg_operands++;
	  i.tm.operands++;

	  i.op[dupl] = i.op[dest];
	  i.types[dupl] = i.types[dest];
	  i.tm.operand_types[dupl] = i.tm.operand_types[dest];
	}

       if (i.tm.opcode_modifier.immext)
	 process_immext ();
    }
  else if (i.tm.operand_types[0].bitfield.acc
	   && i.tm.operand_types[0].bitfield.xmmword)
    {
      unsigned int j;

      for (j = 1; j < i.operands; j++)
	{
	  i.op[j - 1] = i.op[j];
	  i.types[j - 1] = i.types[j];

	  /* We need to adjust fields in i.tm since they are used by
	     build_modrm_byte.  */
	  i.tm.operand_types [j - 1] = i.tm.operand_types [j];
	}

      i.operands--;
      i.reg_operands--;
      i.tm.operands--;
    }
  else if (i.tm.opcode_modifier.implicitquadgroup)
    {
      /* The second operand must be {x,y,z}mmN, where N is a multiple of 4. */
      gas_assert (i.operands >= 2 && i.types[1].bitfield.regsimd);
      unsigned int regnum = register_number (i.op[1].regs);
      unsigned int first_reg_in_group = regnum & ~3;
      unsigned int last_reg_in_group = first_reg_in_group + 3;
      if (regnum != first_reg_in_group) {
        as_warn (_("the second source register `%s%s' implicitly denotes"
            " `%s%.3s%d' to `%s%.3s%d' source group in `%s'"),
            register_prefix, i.op[1].regs->reg_name,
            register_prefix, i.op[1].regs->reg_name, first_reg_in_group,
            register_prefix, i.op[1].regs->reg_name, last_reg_in_group,
            i.tm.name);
      }
	}
  else if (i.tm.opcode_modifier.regkludge)
    {
      /* The imul $imm, %reg instruction is converted into
	 imul $imm, %reg, %reg, and the clr %reg instruction
	 is converted into xor %reg, %reg.  */

      unsigned int first_reg_op;

      if (operand_type_check (i.types[0], reg))
	first_reg_op = 0;
      else
	first_reg_op = 1;
      /* Pretend we saw the extra register operand.  */
      gas_assert (i.reg_operands == 1
		  && i.op[first_reg_op + 1].regs == 0);
      i.op[first_reg_op + 1].regs = i.op[first_reg_op].regs;
      i.types[first_reg_op + 1] = i.types[first_reg_op];
      i.operands++;
      i.reg_operands++;
    }

  if (i.tm.opcode_modifier.shortform)
    {
      if (i.types[0].bitfield.sreg2
	  || i.types[0].bitfield.sreg3)
	{
	  if (i.tm.base_opcode == POP_SEG_SHORT
	      && i.op[0].regs->reg_num == 1)
	    {
	      as_bad (_("you can't `pop %scs'"), register_prefix);
	      return 0;
	    }
	  i.tm.base_opcode |= (i.op[0].regs->reg_num << 3);
	  if ((i.op[0].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_B;
	}
      else
	{
	  /* The register or float register operand is in operand
	     0 or 1.  */
	  unsigned int op;

	  if ((i.types[0].bitfield.reg && i.types[0].bitfield.tbyte)
	      || operand_type_check (i.types[0], reg))
	    op = 0;
	  else
	    op = 1;
	  /* Register goes in low 3 bits of opcode.  */
	  i.tm.base_opcode |= i.op[op].regs->reg_num;
	  if ((i.op[op].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_B;
	  if (!quiet_warnings && i.tm.opcode_modifier.ugh)
	    {
	      /* Warn about some common errors, but press on regardless.
		 The first case can be generated by gcc (<= 2.8.1).  */
	      if (i.operands == 2)
		{
		  /* Reversed arguments on faddp, fsubp, etc.  */
		  as_warn (_("translating to `%s %s%s,%s%s'"), i.tm.name,
			   register_prefix, i.op[!intel_syntax].regs->reg_name,
			   register_prefix, i.op[intel_syntax].regs->reg_name);
		}
	      else
		{
		  /* Extraneous `l' suffix on fp insn.  */
		  as_warn (_("translating to `%s %s%s'"), i.tm.name,
			   register_prefix, i.op[0].regs->reg_name);
		}
	    }
	}
    }
  else if (i.tm.opcode_modifier.modrm)
    {
      /* The opcode is completed (modulo i.tm.extension_opcode which
	 must be put into the modrm byte).  Now, we make the modrm and
	 index base bytes based on all the info we've collected.  */

      default_seg = build_modrm_byte ();
    }
  else if ((i.tm.base_opcode & ~0x3) == MOV_AX_DISP32)
    {
      default_seg = &ds;
    }
  else if (i.tm.opcode_modifier.isstring)
    {
      /* For the string instructions that allow a segment override
	 on one of their operands, the default segment is ds.  */
      default_seg = &ds;
    }

  if (i.tm.base_opcode == 0x8d /* lea */
      && i.seg[0]
      && !quiet_warnings)
    as_warn (_("segment override on `%s' is ineffectual"), i.tm.name);

  /* If a segment was explicitly specified, and the specified segment
     is not the default, use an opcode prefix to select it.  If we
     never figured out what the default segment is, then default_seg
     will be zero at this point, and the specified segment prefix will
     always be used.  */
  if ((i.seg[0]) && (i.seg[0] != default_seg))
    {
      if (!add_prefix (i.seg[0]->seg_prefix))
	return 0;
    }
  return 1;
}

static const seg_entry *
build_modrm_byte (void)
{
  const seg_entry *default_seg = 0;
  unsigned int source, dest;
  int vex_3_sources;

  /* The first operand of instructions with VEX prefix and 3 sources
     must be VEX_Imm4.  */
  vex_3_sources = i.tm.opcode_modifier.vexsources == VEX3SOURCES;
  if (vex_3_sources)
    {
      unsigned int nds, reg_slot;
      expressionS *exp;

      if (i.tm.opcode_modifier.veximmext
          && i.tm.opcode_modifier.immext)
        {
          dest = i.operands - 2;
          gas_assert (dest == 3);
        }
      else
        dest = i.operands - 1;
      nds = dest - 1;

      /* There are 2 kinds of instructions:
         1. 5 operands: 4 register operands or 3 register operands
         plus 1 memory operand plus one Vec_Imm4 operand, VexXDS, and
         VexW0 or VexW1.  The destination must be either XMM, YMM or
	 ZMM register.
         2. 4 operands: 4 register operands or 3 register operands
         plus 1 memory operand, VexXDS, and VexImmExt  */
      gas_assert ((i.reg_operands == 4
                   || (i.reg_operands == 3 && i.mem_operands == 1))
                  && i.tm.opcode_modifier.vexvvvv == VEXXDS
                  && (i.tm.opcode_modifier.veximmext
                      || (i.imm_operands == 1
                          && i.types[0].bitfield.vec_imm4
                          && (i.tm.opcode_modifier.vexw == VEXW0
                              || i.tm.opcode_modifier.vexw == VEXW1)
                          && i.tm.operand_types[dest].bitfield.regsimd)));

      if (i.imm_operands == 0)
        {
          /* When there is no immediate operand, generate an 8bit
             immediate operand to encode the first operand.  */
          exp = &im_expressions[i.imm_operands++];
          i.op[i.operands].imms = exp;
          i.types[i.operands] = imm8;
          i.operands++;
          /* If VexW1 is set, the first operand is the source and
             the second operand is encoded in the immediate operand.  */
          if (i.tm.opcode_modifier.vexw == VEXW1)
            {
              source = 0;
              reg_slot = 1;
            }
          else
            {
              source = 1;
              reg_slot = 0;
            }

          /* FMA swaps REG and NDS.  */
          if (i.tm.cpu_flags.bitfield.cpufma)
            {
              unsigned int tmp;
              tmp = reg_slot;
              reg_slot = nds;
              nds = tmp;
            }

          gas_assert (i.tm.operand_types[reg_slot].bitfield.regsimd);
          exp->X_op = O_constant;
          exp->X_add_number = register_number (i.op[reg_slot].regs) << 4;
	  gas_assert ((i.op[reg_slot].regs->reg_flags & RegVRex) == 0);
	}
      else
        {
          unsigned int imm_slot;

          if (i.tm.opcode_modifier.vexw == VEXW0)
            {
              /* If VexW0 is set, the third operand is the source and
                 the second operand is encoded in the immediate
                 operand.  */
              source = 2;
              reg_slot = 1;
            }
          else
            {
              /* VexW1 is set, the second operand is the source and
                 the third operand is encoded in the immediate
                 operand.  */
              source = 1;
              reg_slot = 2;
            }

          if (i.tm.opcode_modifier.immext)
            {
              /* When ImmExt is set, the immediate byte is the last
                 operand.  */
              imm_slot = i.operands - 1;
              source--;
              reg_slot--;
            }
          else
            {
              imm_slot = 0;

              /* Turn on Imm8 so that output_imm will generate it.  */
              i.types[imm_slot].bitfield.imm8 = 1;
            }

          gas_assert (i.tm.operand_types[reg_slot].bitfield.regsimd);
          i.op[imm_slot].imms->X_add_number
              |= register_number (i.op[reg_slot].regs) << 4;
	  gas_assert ((i.op[reg_slot].regs->reg_flags & RegVRex) == 0);
        }

      gas_assert (i.tm.operand_types[nds].bitfield.regsimd);
      i.vex.register_specifier = i.op[nds].regs;
    }
  else
    source = dest = 0;

  /* i.reg_operands MUST be the number of real register operands;
     implicit registers do not count.  If there are 3 register
     operands, it must be a instruction with VexNDS.  For a
     instruction with VexNDD, the destination register is encoded
     in VEX prefix.  If there are 4 register operands, it must be
     a instruction with VEX prefix and 3 sources.  */
  if (i.mem_operands == 0
      && ((i.reg_operands == 2
	   && i.tm.opcode_modifier.vexvvvv <= VEXXDS)
	  || (i.reg_operands == 3
	      && i.tm.opcode_modifier.vexvvvv == VEXXDS)
	  || (i.reg_operands == 4 && vex_3_sources)))
    {
      switch (i.operands)
	{
	case 2:
	  source = 0;
	  break;
	case 3:
	  /* When there are 3 operands, one of them may be immediate,
	     which may be the first or the last operand.  Otherwise,
	     the first operand must be shift count register (cl) or it
	     is an instruction with VexNDS. */
	  gas_assert (i.imm_operands == 1
		      || (i.imm_operands == 0
			  && (i.tm.opcode_modifier.vexvvvv == VEXXDS
			      || i.types[0].bitfield.shiftcount)));
	  if (operand_type_check (i.types[0], imm)
	      || i.types[0].bitfield.shiftcount)
	    source = 1;
	  else
	    source = 0;
	  break;
	case 4:
	  /* When there are 4 operands, the first two must be 8bit
	     immediate operands. The source operand will be the 3rd
	     one.

	     For instructions with VexNDS, if the first operand
	     an imm8, the source operand is the 2nd one.  If the last
	     operand is imm8, the source operand is the first one.  */
	  gas_assert ((i.imm_operands == 2
		       && i.types[0].bitfield.imm8
		       && i.types[1].bitfield.imm8)
		      || (i.tm.opcode_modifier.vexvvvv == VEXXDS
			  && i.imm_operands == 1
			  && (i.types[0].bitfield.imm8
			      || i.types[i.operands - 1].bitfield.imm8
			      || i.rounding)));
	  if (i.imm_operands == 2)
	    source = 2;
	  else
	    {
	      if (i.types[0].bitfield.imm8)
		source = 1;
	      else
		source = 0;
	    }
	  break;
	case 5:
	  if (i.tm.opcode_modifier.evex)
	    {
	      /* For EVEX instructions, when there are 5 operands, the
		 first one must be immediate operand.  If the second one
		 is immediate operand, the source operand is the 3th
		 one.  If the last one is immediate operand, the source
		 operand is the 2nd one.  */
	      gas_assert (i.imm_operands == 2
			  && i.tm.opcode_modifier.sae
			  && operand_type_check (i.types[0], imm));
	      if (operand_type_check (i.types[1], imm))
		source = 2;
	      else if (operand_type_check (i.types[4], imm))
		source = 1;
	      else
		abort ();
	    }
	  break;
	default:
	  abort ();
	}

      if (!vex_3_sources)
	{
	  dest = source + 1;

	  /* RC/SAE operand could be between DEST and SRC.  That happens
	     when one operand is GPR and the other one is XMM/YMM/ZMM
	     register.  */
	  if (i.rounding && i.rounding->operand == (int) dest)
	    dest++;

	  if (i.tm.opcode_modifier.vexvvvv == VEXXDS)
	    {
	      /* For instructions with VexNDS, the register-only source
		 operand must be a 32/64bit integer, XMM, YMM, ZMM, or mask
		 register.  It is encoded in VEX prefix.  We need to
		 clear RegMem bit before calling operand_type_equal.  */

	      i386_operand_type op;
	      unsigned int vvvv;

	      /* Check register-only source operand when two source
		 operands are swapped.  */
	      if (!i.tm.operand_types[source].bitfield.baseindex
		  && i.tm.operand_types[dest].bitfield.baseindex)
		{
		  vvvv = source;
		  source = dest;
		}
	      else
		vvvv = dest;

	      op = i.tm.operand_types[vvvv];
	      op.bitfield.regmem = 0;
	      if ((dest + 1) >= i.operands
		  || ((!op.bitfield.reg
		       || (!op.bitfield.dword && !op.bitfield.qword))
		      && !op.bitfield.regsimd
		      && !operand_type_equal (&op, &regmask)))
		abort ();
	      i.vex.register_specifier = i.op[vvvv].regs;
	      dest++;
	    }
	}

      i.rm.mode = 3;
      /* One of the register operands will be encoded in the i.tm.reg
	 field, the other in the combined i.tm.mode and i.tm.regmem
	 fields.  If no form of this instruction supports a memory
	 destination operand, then we assume the source operand may
	 sometimes be a memory operand and so we need to store the
	 destination in the i.rm.reg field.  */
      if (!i.tm.operand_types[dest].bitfield.regmem
	  && operand_type_check (i.tm.operand_types[dest], anymem) == 0)
	{
	  i.rm.reg = i.op[dest].regs->reg_num;
	  i.rm.regmem = i.op[source].regs->reg_num;
	  if ((i.op[dest].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_R;
	  if ((i.op[dest].regs->reg_flags & RegVRex) != 0)
	    i.vrex |= REX_R;
	  if ((i.op[source].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_B;
	  if ((i.op[source].regs->reg_flags & RegVRex) != 0)
	    i.vrex |= REX_B;
	}
      else
	{
	  i.rm.reg = i.op[source].regs->reg_num;
	  i.rm.regmem = i.op[dest].regs->reg_num;
	  if ((i.op[dest].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_B;
	  if ((i.op[dest].regs->reg_flags & RegVRex) != 0)
	    i.vrex |= REX_B;
	  if ((i.op[source].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_R;
	  if ((i.op[source].regs->reg_flags & RegVRex) != 0)
	    i.vrex |= REX_R;
	}
      if (flag_code != CODE_64BIT && (i.rex & (REX_R | REX_B)))
	{
	  if (!i.types[0].bitfield.control
	      && !i.types[1].bitfield.control)
	    abort ();
	  i.rex &= ~(REX_R | REX_B);
	  add_prefix (LOCK_PREFIX_OPCODE);
	}
    }
  else
    {			/* If it's not 2 reg operands...  */
      unsigned int mem;

      if (i.mem_operands)
	{
	  unsigned int fake_zero_displacement = 0;
	  unsigned int op;

	  for (op = 0; op < i.operands; op++)
	    if (operand_type_check (i.types[op], anymem))
	      break;
	  gas_assert (op < i.operands);

	  if (i.tm.opcode_modifier.vecsib)
	    {
	      if (i.index_reg->reg_num == RegEiz
		  || i.index_reg->reg_num == RegRiz)
		abort ();

	      i.rm.regmem = ESCAPE_TO_TWO_BYTE_ADDRESSING;
	      if (!i.base_reg)
		{
		  i.sib.base = NO_BASE_REGISTER;
		  i.sib.scale = i.log2_scale_factor;
		  i.types[op].bitfield.disp8 = 0;
		  i.types[op].bitfield.disp16 = 0;
		  i.types[op].bitfield.disp64 = 0;
		  if (flag_code != CODE_64BIT || i.prefix[ADDR_PREFIX])
		    {
		      /* Must be 32 bit */
		      i.types[op].bitfield.disp32 = 1;
		      i.types[op].bitfield.disp32s = 0;
		    }
		  else
		    {
		      i.types[op].bitfield.disp32 = 0;
		      i.types[op].bitfield.disp32s = 1;
		    }
		}
	      i.sib.index = i.index_reg->reg_num;
	      if ((i.index_reg->reg_flags & RegRex) != 0)
		i.rex |= REX_X;
	      if ((i.index_reg->reg_flags & RegVRex) != 0)
		i.vrex |= REX_X;
	    }

	  default_seg = &ds;

	  if (i.base_reg == 0)
	    {
	      i.rm.mode = 0;
	      if (!i.disp_operands)
		fake_zero_displacement = 1;
	      if (i.index_reg == 0)
		{
		  gas_assert (!i.tm.opcode_modifier.vecsib);
		  /* Operand is just <disp>  */
		  if (flag_code == CODE_64BIT)
		    {
		      /* 64bit mode overwrites the 32bit absolute
			 addressing by RIP relative addressing and
			 absolute addressing is encoded by one of the
			 redundant SIB forms.  */
		      i.rm.regmem = ESCAPE_TO_TWO_BYTE_ADDRESSING;
		      i.sib.base = NO_BASE_REGISTER;
		      i.sib.index = NO_INDEX_REGISTER;
		      i.types[op] = ((i.prefix[ADDR_PREFIX] == 0)
				     ? disp32s : disp32);
		    }
		  else if ((flag_code == CODE_16BIT)
			   ^ (i.prefix[ADDR_PREFIX] != 0))
		    {
		      i.rm.regmem = NO_BASE_REGISTER_16;
		      i.types[op] = disp16;
		    }
		  else
		    {
		      i.rm.regmem = NO_BASE_REGISTER;
		      i.types[op] = disp32;
		    }
		}
	      else if (!i.tm.opcode_modifier.vecsib)
		{
		  /* !i.base_reg && i.index_reg  */
		  if (i.index_reg->reg_num == RegEiz
		      || i.index_reg->reg_num == RegRiz)
		    i.sib.index = NO_INDEX_REGISTER;
		  else
		    i.sib.index = i.index_reg->reg_num;
		  i.sib.base = NO_BASE_REGISTER;
		  i.sib.scale = i.log2_scale_factor;
		  i.rm.regmem = ESCAPE_TO_TWO_BYTE_ADDRESSING;
		  i.types[op].bitfield.disp8 = 0;
		  i.types[op].bitfield.disp16 = 0;
		  i.types[op].bitfield.disp64 = 0;
		  if (flag_code != CODE_64BIT || i.prefix[ADDR_PREFIX])
		    {
		      /* Must be 32 bit */
		      i.types[op].bitfield.disp32 = 1;
		      i.types[op].bitfield.disp32s = 0;
		    }
		  else
		    {
		      i.types[op].bitfield.disp32 = 0;
		      i.types[op].bitfield.disp32s = 1;
		    }
		  if ((i.index_reg->reg_flags & RegRex) != 0)
		    i.rex |= REX_X;
		}
	    }
	  /* RIP addressing for 64bit mode.  */
	  else if (i.base_reg->reg_num == RegRip ||
		   i.base_reg->reg_num == RegEip)
	    {
	      gas_assert (!i.tm.opcode_modifier.vecsib);
	      i.rm.regmem = NO_BASE_REGISTER;
	      i.types[op].bitfield.disp8 = 0;
	      i.types[op].bitfield.disp16 = 0;
	      i.types[op].bitfield.disp32 = 0;
	      i.types[op].bitfield.disp32s = 1;
	      i.types[op].bitfield.disp64 = 0;
	      i.flags[op] |= Operand_PCrel;
	      if (! i.disp_operands)
		fake_zero_displacement = 1;
	    }
	  else if (i.base_reg->reg_type.bitfield.word)
	    {
	      gas_assert (!i.tm.opcode_modifier.vecsib);
	      switch (i.base_reg->reg_num)
		{
		case 3: /* (%bx)  */
		  if (i.index_reg == 0)
		    i.rm.regmem = 7;
		  else /* (%bx,%si) -> 0, or (%bx,%di) -> 1  */
		    i.rm.regmem = i.index_reg->reg_num - 6;
		  break;
		case 5: /* (%bp)  */
		  default_seg = &ss;
		  if (i.index_reg == 0)
		    {
		      i.rm.regmem = 6;
		      if (operand_type_check (i.types[op], disp) == 0)
			{
			  /* fake (%bp) into 0(%bp)  */
			  i.types[op].bitfield.disp8 = 1;
			  fake_zero_displacement = 1;
			}
		    }
		  else /* (%bp,%si) -> 2, or (%bp,%di) -> 3  */
		    i.rm.regmem = i.index_reg->reg_num - 6 + 2;
		  break;
		default: /* (%si) -> 4 or (%di) -> 5  */
		  i.rm.regmem = i.base_reg->reg_num - 6 + 4;
		}
	      i.rm.mode = mode_from_disp_size (i.types[op]);
	    }
	  else /* i.base_reg and 32/64 bit mode  */
	    {
	      if (flag_code == CODE_64BIT
		  && operand_type_check (i.types[op], disp))
		{
		  i386_operand_type temp;
		  operand_type_set (&temp, 0);
		  temp.bitfield.disp8 = i.types[op].bitfield.disp8;
		  i.types[op] = temp;
		  if (i.prefix[ADDR_PREFIX] == 0)
		    i.types[op].bitfield.disp32s = 1;
		  else
		    i.types[op].bitfield.disp32 = 1;
		}

	      if (!i.tm.opcode_modifier.vecsib)
		i.rm.regmem = i.base_reg->reg_num;
	      if ((i.base_reg->reg_flags & RegRex) != 0)
		i.rex |= REX_B;
	      i.sib.base = i.base_reg->reg_num;
	      /* x86-64 ignores REX prefix bit here to avoid decoder
		 complications.  */
	      if (!(i.base_reg->reg_flags & RegRex)
		  && (i.base_reg->reg_num == EBP_REG_NUM
		   || i.base_reg->reg_num == ESP_REG_NUM))
		  default_seg = &ss;
	      if (i.base_reg->reg_num == 5 && i.disp_operands == 0)
		{
		  fake_zero_displacement = 1;
		  i.types[op].bitfield.disp8 = 1;
		}
	      i.sib.scale = i.log2_scale_factor;
	      if (i.index_reg == 0)
		{
		  gas_assert (!i.tm.opcode_modifier.vecsib);
		  /* <disp>(%esp) becomes two byte modrm with no index
		     register.  We've already stored the code for esp
		     in i.rm.regmem ie. ESCAPE_TO_TWO_BYTE_ADDRESSING.
		     Any base register besides %esp will not use the
		     extra modrm byte.  */
		  i.sib.index = NO_INDEX_REGISTER;
		}
	      else if (!i.tm.opcode_modifier.vecsib)
		{
		  if (i.index_reg->reg_num == RegEiz
		      || i.index_reg->reg_num == RegRiz)
		    i.sib.index = NO_INDEX_REGISTER;
		  else
		    i.sib.index = i.index_reg->reg_num;
		  i.rm.regmem = ESCAPE_TO_TWO_BYTE_ADDRESSING;
		  if ((i.index_reg->reg_flags & RegRex) != 0)
		    i.rex |= REX_X;
		}

	      if (i.disp_operands
		  && (i.reloc[op] == BFD_RELOC_386_TLS_DESC_CALL
		      || i.reloc[op] == BFD_RELOC_X86_64_TLSDESC_CALL))
		i.rm.mode = 0;
	      else
		{
		  if (!fake_zero_displacement
		      && !i.disp_operands
		      && i.disp_encoding)
		    {
		      fake_zero_displacement = 1;
		      if (i.disp_encoding == disp_encoding_8bit)
			i.types[op].bitfield.disp8 = 1;
		      else
			i.types[op].bitfield.disp32 = 1;
		    }
		  i.rm.mode = mode_from_disp_size (i.types[op]);
		}
	    }

	  if (fake_zero_displacement)
	    {
	      /* Fakes a zero displacement assuming that i.types[op]
		 holds the correct displacement size.  */
	      expressionS *exp;

	      gas_assert (i.op[op].disps == 0);
	      exp = &disp_expressions[i.disp_operands++];
	      i.op[op].disps = exp;
	      exp->X_op = O_constant;
	      exp->X_add_number = 0;
	      exp->X_add_symbol = (symbolS *) 0;
	      exp->X_op_symbol = (symbolS *) 0;
	    }

	  mem = op;
	}
      else
	mem = ~0;

      if (i.tm.opcode_modifier.vexsources == XOP2SOURCES)
	{
	  if (operand_type_check (i.types[0], imm))
	    i.vex.register_specifier = NULL;
	  else
	    {
	      /* VEX.vvvv encodes one of the sources when the first
		 operand is not an immediate.  */
	      if (i.tm.opcode_modifier.vexw == VEXW0)
		i.vex.register_specifier = i.op[0].regs;
	      else
		i.vex.register_specifier = i.op[1].regs;
	    }

	  /* Destination is a XMM register encoded in the ModRM.reg
	     and VEX.R bit.  */
	  i.rm.reg = i.op[2].regs->reg_num;
	  if ((i.op[2].regs->reg_flags & RegRex) != 0)
	    i.rex |= REX_R;

	  /* ModRM.rm and VEX.B encodes the other source.  */
	  if (!i.mem_operands)
	    {
	      i.rm.mode = 3;

	      if (i.tm.opcode_modifier.vexw == VEXW0)
		i.rm.regmem = i.op[1].regs->reg_num;
	      else
		i.rm.regmem = i.op[0].regs->reg_num;

	      if ((i.op[1].regs->reg_flags & RegRex) != 0)
		i.rex |= REX_B;
	    }
	}
      else if (i.tm.opcode_modifier.vexvvvv == VEXLWP)
	{
	  i.vex.register_specifier = i.op[2].regs;
	  if (!i.mem_operands)
	    {
	      i.rm.mode = 3;
	      i.rm.regmem = i.op[1].regs->reg_num;
	      if ((i.op[1].regs->reg_flags & RegRex) != 0)
		i.rex |= REX_B;
	    }
	}
      /* Fill in i.rm.reg or i.rm.regmem field with register operand
	 (if any) based on i.tm.extension_opcode.  Again, we must be
	 careful to make sure that segment/control/debug/test/MMX
	 registers are coded into the i.rm.reg field.  */
      else if (i.reg_operands)
	{
	  unsigned int op;
	  unsigned int vex_reg = ~0;

	  for (op = 0; op < i.operands; op++)
	    if (i.types[op].bitfield.reg
		|| i.types[op].bitfield.regmmx
		|| i.types[op].bitfield.regsimd
		|| i.types[op].bitfield.regbnd
		|| i.types[op].bitfield.regmask
		|| i.types[op].bitfield.sreg2
		|| i.types[op].bitfield.sreg3
		|| i.types[op].bitfield.control
		|| i.types[op].bitfield.debug
		|| i.types[op].bitfield.test)
	      break;

	  if (vex_3_sources)
	    op = dest;
	  else if (i.tm.opcode_modifier.vexvvvv == VEXXDS)
	    {
	      /* For instructions with VexNDS, the register-only
		 source operand is encoded in VEX prefix. */
	      gas_assert (mem != (unsigned int) ~0);

	      if (op > mem)
		{
		  vex_reg = op++;
		  gas_assert (op < i.operands);
		}
	      else
		{
		  /* Check register-only source operand when two source
		     operands are swapped.  */
		  if (!i.tm.operand_types[op].bitfield.baseindex
		      && i.tm.operand_types[op + 1].bitfield.baseindex)
		    {
		      vex_reg = op;
		      op += 2;
		      gas_assert (mem == (vex_reg + 1)
				  && op < i.operands);
		    }
		  else
		    {
		      vex_reg = op + 1;
		      gas_assert (vex_reg < i.operands);
		    }
		}
	    }
	  else if (i.tm.opcode_modifier.vexvvvv == VEXNDD)
	    {
	      /* For instructions with VexNDD, the register destination
		 is encoded in VEX prefix.  */
	      if (i.mem_operands == 0)
		{
		  /* There is no memory operand.  */
		  gas_assert ((op + 2) == i.operands);
		  vex_reg = op + 1;
		}
	      else
		{
		  /* There are only 2 operands.  */
		  gas_assert (op < 2 && i.operands == 2);
		  vex_reg = 1;
		}
	    }
	  else
	    gas_assert (op < i.operands);

	  if (vex_reg != (unsigned int) ~0)
	    {
	      i386_operand_type *type = &i.tm.operand_types[vex_reg];

	      if ((!type->bitfield.reg
		   || (!type->bitfield.dword && !type->bitfield.qword))
		  && !type->bitfield.regsimd
		  && !operand_type_equal (type, &regmask))
		abort ();

	      i.vex.register_specifier = i.op[vex_reg].regs;
	    }

	  /* Don't set OP operand twice.  */
	  if (vex_reg != op)
	    {
	      /* If there is an extension opcode to put here, the
		 register number must be put into the regmem field.  */
	      if (i.tm.extension_opcode != None)
		{
		  i.rm.regmem = i.op[op].regs->reg_num;
		  if ((i.op[op].regs->reg_flags & RegRex) != 0)
		    i.rex |= REX_B;
		  if ((i.op[op].regs->reg_flags & RegVRex) != 0)
		    i.vrex |= REX_B;
		}
	      else
		{
		  i.rm.reg = i.op[op].regs->reg_num;
		  if ((i.op[op].regs->reg_flags & RegRex) != 0)
		    i.rex |= REX_R;
		  if ((i.op[op].regs->reg_flags & RegVRex) != 0)
		    i.vrex |= REX_R;
		}
	    }

	  /* Now, if no memory operand has set i.rm.mode = 0, 1, 2 we
	     must set it to 3 to indicate this is a register operand
	     in the regmem field.  */
	  if (!i.mem_operands)
	    i.rm.mode = 3;
	}

      /* Fill in i.rm.reg field with extension opcode (if any).  */
      if (i.tm.extension_opcode != None)
	i.rm.reg = i.tm.extension_opcode;
    }
  return default_seg;
}

static void
output_branch (void)
{
  char *p;
  int size;
  int code16;
  int prefix;
  relax_substateT subtype;
  symbolS *sym;
  offsetT off;

  code16 = flag_code == CODE_16BIT ? CODE16 : 0;
  size = i.disp_encoding == disp_encoding_32bit ? BIG : SMALL;

  prefix = 0;
  if (i.prefix[DATA_PREFIX] != 0)
    {
      prefix = 1;
      i.prefixes -= 1;
      code16 ^= CODE16;
    }
  /* Pentium4 branch hints.  */
  if (i.prefix[SEG_PREFIX] == CS_PREFIX_OPCODE /* not taken */
      || i.prefix[SEG_PREFIX] == DS_PREFIX_OPCODE /* taken */)
    {
      prefix++;
      i.prefixes--;
    }
  if (i.prefix[REX_PREFIX] != 0)
    {
      prefix++;
      i.prefixes--;
    }

  /* BND prefixed jump.  */
  if (i.prefix[BND_PREFIX] != 0)
    {
      FRAG_APPEND_1_CHAR (i.prefix[BND_PREFIX]);
      i.prefixes -= 1;
    }

  if (i.prefixes != 0 && !intel_syntax)
    as_warn (_("skipping prefixes on this instruction"));

  /* It's always a symbol;  End frag & setup for relax.
     Make sure there is enough room in this frag for the largest
     instruction we may generate in md_convert_frag.  This is 2
     bytes for the opcode and room for the prefix and largest
     displacement.  */
  frag_grow (prefix + 2 + 4);
  /* Prefix and 1 opcode byte go in fr_fix.  */
  p = frag_more (prefix + 1);
  if (i.prefix[DATA_PREFIX] != 0)
    *p++ = DATA_PREFIX_OPCODE;
  if (i.prefix[SEG_PREFIX] == CS_PREFIX_OPCODE
      || i.prefix[SEG_PREFIX] == DS_PREFIX_OPCODE)
    *p++ = i.prefix[SEG_PREFIX];
  if (i.prefix[REX_PREFIX] != 0)
    *p++ = i.prefix[REX_PREFIX];
  *p = i.tm.base_opcode;

  if ((unsigned char) *p == JUMP_PC_RELATIVE)
    subtype = ENCODE_RELAX_STATE (UNCOND_JUMP, size);
  else if (cpu_arch_flags.bitfield.cpui386)
    subtype = ENCODE_RELAX_STATE (COND_JUMP, size);
  else
    subtype = ENCODE_RELAX_STATE (COND_JUMP86, size);
  subtype |= code16;

  sym = i.op[0].disps->X_add_symbol;
  off = i.op[0].disps->X_add_number;

  if (i.op[0].disps->X_op != O_constant
      && i.op[0].disps->X_op != O_symbol)
    {
      /* Handle complex expressions.  */
      sym = make_expr_symbol (i.op[0].disps);
      off = 0;
    }

  /* 1 possible extra opcode + 4 byte displacement go in var part.
     Pass reloc in fr_var.  */
  frag_var (rs_machine_dependent, 5, i.reloc[0], subtype, sym, off, p);
}

static void
output_jump (void)
{
  char *p;
  int size;
  fixS *fixP;

  if (i.tm.opcode_modifier.jumpbyte)
    {
      /* This is a loop or jecxz type instruction.  */
      size = 1;
      if (i.prefix[ADDR_PREFIX] != 0)
	{
	  FRAG_APPEND_1_CHAR (ADDR_PREFIX_OPCODE);
	  i.prefixes -= 1;
	}
      /* Pentium4 branch hints.  */
      if (i.prefix[SEG_PREFIX] == CS_PREFIX_OPCODE /* not taken */
	  || i.prefix[SEG_PREFIX] == DS_PREFIX_OPCODE /* taken */)
	{
	  FRAG_APPEND_1_CHAR (i.prefix[SEG_PREFIX]);
	  i.prefixes--;
	}
    }
  else
    {
      int code16;

      code16 = 0;
      if (flag_code == CODE_16BIT)
	code16 = CODE16;

      if (i.prefix[DATA_PREFIX] != 0)
	{
	  FRAG_APPEND_1_CHAR (DATA_PREFIX_OPCODE);
	  i.prefixes -= 1;
	  code16 ^= CODE16;
	}

      size = 4;
      if (code16)
	size = 2;
    }

  if (i.prefix[REX_PREFIX] != 0)
    {
      FRAG_APPEND_1_CHAR (i.prefix[REX_PREFIX]);
      i.prefixes -= 1;
    }

  /* BND prefixed jump.  */
  if (i.prefix[BND_PREFIX] != 0)
    {
      FRAG_APPEND_1_CHAR (i.prefix[BND_PREFIX]);
      i.prefixes -= 1;
    }

  if (i.prefixes != 0 && !intel_syntax)
    as_warn (_("skipping prefixes on this instruction"));

  p = frag_more (i.tm.opcode_length + size);
  switch (i.tm.opcode_length)
    {
    case 2:
      *p++ = i.tm.base_opcode >> 8;
      /* Fall through.  */
    case 1:
      *p++ = i.tm.base_opcode;
      break;
    default:
      abort ();
    }

  fixP = fix_new_exp (frag_now, p - frag_now->fr_literal, size,
		      i.op[0].disps, 1, reloc (size, 1, 1, i.reloc[0]));

  /* All jumps handled here are signed, but don't use a signed limit
     check for 32 and 16 bit jumps as we want to allow wrap around at
     4G and 64k respectively.  */
  if (size == 1)
    fixP->fx_signed = 1;
}

static void
output_interseg_jump (void)
{
  char *p;
  int size;
  int prefix;
  int code16;

  code16 = 0;
  if (flag_code == CODE_16BIT)
    code16 = CODE16;

  prefix = 0;
  if (i.prefix[DATA_PREFIX] != 0)
    {
      prefix = 1;
      i.prefixes -= 1;
      code16 ^= CODE16;
    }
  if (i.prefix[REX_PREFIX] != 0)
    {
      prefix++;
      i.prefixes -= 1;
    }

  size = 4;
  if (code16)
    size = 2;

  if (i.prefixes != 0 && !intel_syntax)
    as_warn (_("skipping prefixes on this instruction"));

  /* 1 opcode; 2 segment; offset  */
  p = frag_more (prefix + 1 + 2 + size);

  if (i.prefix[DATA_PREFIX] != 0)
    *p++ = DATA_PREFIX_OPCODE;

  if (i.prefix[REX_PREFIX] != 0)
    *p++ = i.prefix[REX_PREFIX];

  *p++ = i.tm.base_opcode;
  if (i.op[1].imms->X_op == O_constant)
    {
      offsetT n = i.op[1].imms->X_add_number;

      if (size == 2
	  && !fits_in_unsigned_word (n)
	  && !fits_in_signed_word (n))
	{
	  as_bad (_("16-bit jump out of range"));
	  return;
	}
      md_number_to_chars (p, n, size);
    }
  else
    fix_new_exp (frag_now, p - frag_now->fr_literal, size,
		 i.op[1].imms, 0, reloc (size, 0, 0, i.reloc[1]));
  if (i.op[0].imms->X_op != O_constant)
    as_bad (_("can't handle non absolute segment in `%s'"),
	    i.tm.name);
  md_number_to_chars (p + size, (valueT) i.op[0].imms->X_add_number, 2);
}

static void
output_insn (void)
{
  fragS *insn_start_frag;
  offsetT insn_start_off;

  /* Tie dwarf2 debug info to the address at the start of the insn.
     We can't do this after the insn has been output as the current
     frag may have been closed off.  eg. by frag_var.  */
  dwarf2_emit_insn (0);

  insn_start_frag = frag_now;
  insn_start_off = frag_now_fix ();

  /* Output jumps.  */
  if (i.tm.opcode_modifier.jump)
    output_branch ();
  else if (i.tm.opcode_modifier.jumpbyte
	   || i.tm.opcode_modifier.jumpdword)
    output_jump ();
  else if (i.tm.opcode_modifier.jumpintersegment)
    output_interseg_jump ();
  else
    {
      /* Output normal instructions here.  */
      char *p;
      unsigned char *q;
      unsigned int j;
      unsigned int prefix;

      if (avoid_fence
         && i.tm.base_opcode == 0xfae
         && i.operands == 1
         && i.imm_operands == 1
         && (i.op[0].imms->X_add_number == 0xe8
             || i.op[0].imms->X_add_number == 0xf0
             || i.op[0].imms->X_add_number == 0xf8))
        {
          /* Encode lfence, mfence, and sfence as
             f0 83 04 24 00   lock addl $0x0, (%{re}sp).  */
          offsetT val = 0x240483f0ULL;
          p = frag_more (5);
          md_number_to_chars (p, val, 5);
          return;
        }

      /* Some processors fail on LOCK prefix. This options makes
	 assembler ignore LOCK prefix and serves as a workaround.  */
      if (omit_lock_prefix)
	{
	  if (i.tm.base_opcode == LOCK_PREFIX_OPCODE)
	    return;
	  i.prefix[LOCK_PREFIX] = 0;
	}

      /* Since the VEX/EVEX prefix contains the implicit prefix, we
	 don't need the explicit prefix.  */
      if (!i.tm.opcode_modifier.vex && !i.tm.opcode_modifier.evex)
	{
	  switch (i.tm.opcode_length)
	    {
	    case 3:
	      if (i.tm.base_opcode & 0xff000000)
		{
		  prefix = (i.tm.base_opcode >> 24) & 0xff;
		  goto check_prefix;
		}
	      break;
	    case 2:
	      if ((i.tm.base_opcode & 0xff0000) != 0)
		{
		  prefix = (i.tm.base_opcode >> 16) & 0xff;
		  if (i.tm.cpu_flags.bitfield.cpupadlock)
		    {
check_prefix:
		      if (prefix != REPE_PREFIX_OPCODE
			  || (i.prefix[REP_PREFIX]
			      != REPE_PREFIX_OPCODE))
			add_prefix (prefix);
		    }
		  else
		    add_prefix (prefix);
		}
	      break;
	    case 1:
	      break;
	    case 0:
	      /* Check for pseudo prefixes.  */
	      as_bad_where (insn_start_frag->fr_file,
			    insn_start_frag->fr_line,
			     _("pseudo prefix without instruction"));
	      return;
	    default:
	      abort ();
	    }

#if defined (OBJ_MAYBE_ELF) || defined (OBJ_ELF)
	  /* For x32, add a dummy REX_OPCODE prefix for mov/add with
	     R_X86_64_GOTTPOFF relocation so that linker can safely
	     perform IE->LE optimization.  */
	  if (x86_elf_abi == X86_64_X32_ABI
	      && i.operands == 2
	      && i.reloc[0] == BFD_RELOC_X86_64_GOTTPOFF
	      && i.prefix[REX_PREFIX] == 0)
	    add_prefix (REX_OPCODE);
#endif

	  /* The prefix bytes.  */
	  for (j = ARRAY_SIZE (i.prefix), q = i.prefix; j > 0; j--, q++)
	    if (*q)
	      FRAG_APPEND_1_CHAR (*q);
	}
      else
	{
	  for (j = 0, q = i.prefix; j < ARRAY_SIZE (i.prefix); j++, q++)
	    if (*q)
	      switch (j)
		{
		case REX_PREFIX:
		  /* REX byte is encoded in VEX prefix.  */
		  break;
		case SEG_PREFIX:
		case ADDR_PREFIX:
		  FRAG_APPEND_1_CHAR (*q);
		  break;
		default:
		  /* There should be no other prefixes for instructions
		     with VEX prefix.  */
		  abort ();
		}

	  /* For EVEX instructions i.vrex should become 0 after
	     build_evex_prefix.  For VEX instructions upper 16 registers
	     aren't available, so VREX should be 0.  */
	  if (i.vrex)
	    abort ();
	  /* Now the VEX prefix.  */
	  p = frag_more (i.vex.length);
	  for (j = 0; j < i.vex.length; j++)
	    p[j] = i.vex.bytes[j];
	}

      /* Now the opcode; be careful about word order here!  */
      if (i.tm.opcode_length == 1)
	{
	  FRAG_APPEND_1_CHAR (i.tm.base_opcode);
	}
      else
	{
	  switch (i.tm.opcode_length)
	    {
	    case 4:
	      p = frag_more (4);
	      *p++ = (i.tm.base_opcode >> 24) & 0xff;
	      *p++ = (i.tm.base_opcode >> 16) & 0xff;
	      break;
	    case 3:
	      p = frag_more (3);
	      *p++ = (i.tm.base_opcode >> 16) & 0xff;
	      break;
	    case 2:
	      p = frag_more (2);
	      break;
	    default:
	      abort ();
	      break;
	    }

	  /* Put out high byte first: can't use md_number_to_chars!  */
	  *p++ = (i.tm.base_opcode >> 8) & 0xff;
	  *p = i.tm.base_opcode & 0xff;
	}

      /* Now the modrm byte and sib byte (if present).  */
      if (i.tm.opcode_modifier.modrm)
	{
	  FRAG_APPEND_1_CHAR ((i.rm.regmem << 0
			       | i.rm.reg << 3
			       | i.rm.mode << 6));
	  /* If i.rm.regmem == ESP (4)
	     && i.rm.mode != (Register mode)
	     && not 16 bit
	     ==> need second modrm byte.  */
	  if (i.rm.regmem == ESCAPE_TO_TWO_BYTE_ADDRESSING
	      && i.rm.mode != 3
	      && !(i.base_reg && i.base_reg->reg_type.bitfield.word))
	    FRAG_APPEND_1_CHAR ((i.sib.base << 0
				 | i.sib.index << 3
				 | i.sib.scale << 6));
	}

      if (i.disp_operands)
	output_disp (insn_start_frag, insn_start_off);

      if (i.imm_operands)
	output_imm (insn_start_frag, insn_start_off);
    }

#ifdef DEBUG386
  if (flag_debug)
    {
      pi ("" /*line*/, &i);
    }
#endif /* DEBUG386  */
}

/* Return the size of the displacement operand N.  */

static int
disp_size (unsigned int n)
{
  int size = 4;

  if (i.types[n].bitfield.disp64)
    size = 8;
  else if (i.types[n].bitfield.disp8)
    size = 1;
  else if (i.types[n].bitfield.disp16)
    size = 2;
  return size;
}

/* Return the size of the immediate operand N.  */

static int
imm_size (unsigned int n)
{
  int size = 4;
  if (i.types[n].bitfield.imm64)
    size = 8;
  else if (i.types[n].bitfield.imm8 || i.types[n].bitfield.imm8s)
    size = 1;
  else if (i.types[n].bitfield.imm16)
    size = 2;
  return size;
}

static void
output_disp (fragS *insn_start_frag, offsetT insn_start_off)
{
  char *p;
  unsigned int n;

  for (n = 0; n < i.operands; n++)
    {
      if (operand_type_check (i.types[n], disp))
	{
	  if (i.op[n].disps->X_op == O_constant)
	    {
	      int size = disp_size (n);
	      offsetT val = i.op[n].disps->X_add_number;

	      val = offset_in_range (val >> i.memshift, size);
	      p = frag_more (size);
	      md_number_to_chars (p, val, size);
	    }
	  else
	    {
	      enum bfd_reloc_code_real reloc_type;
	      int size = disp_size (n);
	      int sign = i.types[n].bitfield.disp32s;
	      int pcrel = (i.flags[n] & Operand_PCrel) != 0;
	      fixS *fixP;

	      /* We can't have 8 bit displacement here.  */
	      gas_assert (!i.types[n].bitfield.disp8);

	      /* The PC relative address is computed relative
		 to the instruction boundary, so in case immediate
		 fields follows, we need to adjust the value.  */
	      if (pcrel && i.imm_operands)
		{
		  unsigned int n1;
		  int sz = 0;

		  for (n1 = 0; n1 < i.operands; n1++)
		    if (operand_type_check (i.types[n1], imm))
		      {
			/* Only one immediate is allowed for PC
			   relative address.  */
			gas_assert (sz == 0);
			sz = imm_size (n1);
			i.op[n].disps->X_add_number -= sz;
		      }
		  /* We should find the immediate.  */
		  gas_assert (sz != 0);
		}

	      p = frag_more (size);
	      reloc_type = reloc (size, pcrel, sign, i.reloc[n]);
	      if (GOT_symbol
		  && GOT_symbol == i.op[n].disps->X_add_symbol
		  && (((reloc_type == BFD_RELOC_32
			|| reloc_type == BFD_RELOC_X86_64_32S
			|| (reloc_type == BFD_RELOC_64
			    && object_64bit))
		       && (i.op[n].disps->X_op == O_symbol
			   || (i.op[n].disps->X_op == O_add
			       && ((symbol_get_value_expression
				    (i.op[n].disps->X_op_symbol)->X_op)
				   == O_subtract))))
		      || reloc_type == BFD_RELOC_32_PCREL))
		{
		  offsetT add;

		  if (insn_start_frag == frag_now)
		    add = (p - frag_now->fr_literal) - insn_start_off;
		  else
		    {
		      fragS *fr;

		      add = insn_start_frag->fr_fix - insn_start_off;
		      for (fr = insn_start_frag->fr_next;
			   fr && fr != frag_now; fr = fr->fr_next)
			add += fr->fr_fix;
		      add += p - frag_now->fr_literal;
		    }

		  if (!object_64bit)
		    {
		      reloc_type = BFD_RELOC_386_GOTPC;
		      i.op[n].imms->X_add_number += add;
		    }
		  else if (reloc_type == BFD_RELOC_64)
		    reloc_type = BFD_RELOC_X86_64_GOTPC64;
		  else
		    /* Don't do the adjustment for x86-64, as there
		       the pcrel addressing is relative to the _next_
		       insn, and that is taken care of in other code.  */
		    reloc_type = BFD_RELOC_X86_64_GOTPC32;
		}
	      fixP = fix_new_exp (frag_now, p - frag_now->fr_literal,
				  size, i.op[n].disps, pcrel,
				  reloc_type);
	      /* Check for "call/jmp *mem", "mov mem, %reg",
		 "test %reg, mem" and "binop mem, %reg" where binop
		 is one of adc, add, and, cmp, or, sbb, sub, xor
		 instructions.  Always generate R_386_GOT32X for
		 "sym*GOT" operand in 32-bit mode.  */
	      if ((generate_relax_relocations
		   || (!object_64bit
		       && i.rm.mode == 0
		       && i.rm.regmem == 5))
		  && (i.rm.mode == 2
		      || (i.rm.mode == 0 && i.rm.regmem == 5))
		  && ((i.operands == 1
		       && i.tm.base_opcode == 0xff
		       && (i.rm.reg == 2 || i.rm.reg == 4))
		      || (i.operands == 2
			  && (i.tm.base_opcode == 0x8b
			      || i.tm.base_opcode == 0x85
			      || (i.tm.base_opcode & 0xc7) == 0x03))))
		{
		  if (object_64bit)
		    {
		      fixP->fx_tcbit = i.rex != 0;
		      if (i.base_reg
			  && (i.base_reg->reg_num == RegRip
			      || i.base_reg->reg_num == RegEip))
		      fixP->fx_tcbit2 = 1;
		    }
		  else
		    fixP->fx_tcbit2 = 1;
		}
	    }
	}
    }
}

static void
output_imm (fragS *insn_start_frag, offsetT insn_start_off)
{
  char *p;
  unsigned int n;

  for (n = 0; n < i.operands; n++)
    {
      /* Skip SAE/RC Imm operand in EVEX.  They are already handled.  */
      if (i.rounding && (int) n == i.rounding->operand)
	continue;

      if (operand_type_check (i.types[n], imm))
	{
	  if (i.op[n].imms->X_op == O_constant)
	    {
	      int size = imm_size (n);
	      offsetT val;

	      val = offset_in_range (i.op[n].imms->X_add_number,
				     size);
	      p = frag_more (size);
	      md_number_to_chars (p, val, size);
	    }
	  else
	    {
	      /* Not absolute_section.
		 Need a 32-bit fixup (don't support 8bit
		 non-absolute imms).  Try to support other
		 sizes ...  */
	      enum bfd_reloc_code_real reloc_type;
	      int size = imm_size (n);
	      int sign;

	      if (i.types[n].bitfield.imm32s
		  && (i.suffix == QWORD_MNEM_SUFFIX
		      || (!i.suffix && i.tm.opcode_modifier.no_lsuf)))
		sign = 1;
	      else
		sign = 0;

	      p = frag_more (size);
	      reloc_type = reloc (size, 0, sign, i.reloc[n]);

	      /*   This is tough to explain.  We end up with this one if we
	       * have operands that look like
	       * "_GLOBAL_OFFSET_TABLE_+[.-.L284]".  The goal here is to
	       * obtain the absolute address of the GOT, and it is strongly
	       * preferable from a performance point of view to avoid using
	       * a runtime relocation for this.  The actual sequence of
	       * instructions often look something like:
	       *
	       *	call	.L66
	       * .L66:
	       *	popl	%ebx
	       *	addl	$_GLOBAL_OFFSET_TABLE_+[.-.L66],%ebx
	       *
	       *   The call and pop essentially return the absolute address
	       * of the label .L66 and store it in %ebx.  The linker itself
	       * will ultimately change the first operand of the addl so
	       * that %ebx points to the GOT, but to keep things simple, the
	       * .o file must have this operand set so that it generates not
	       * the absolute address of .L66, but the absolute address of
	       * itself.  This allows the linker itself simply treat a GOTPC
	       * relocation as asking for a pcrel offset to the GOT to be
	       * added in, and the addend of the relocation is stored in the
	       * operand field for the instruction itself.
	       *
	       *   Our job here is to fix the operand so that it would add
	       * the correct offset so that %ebx would point to itself.  The
	       * thing that is tricky is that .-.L66 will point to the
	       * beginning of the instruction, so we need to further modify
	       * the operand so that it will point to itself.  There are
	       * other cases where you have something like:
	       *
	       *	.long	$_GLOBAL_OFFSET_TABLE_+[.-.L66]
	       *
	       * and here no correction would be required.  Internally in
	       * the assembler we treat operands of this form as not being
	       * pcrel since the '.' is explicitly mentioned, and I wonder
	       * whether it would simplify matters to do it this way.  Who
	       * knows.  In earlier versions of the PIC patches, the
	       * pcrel_adjust field was used to store the correction, but
	       * since the expression is not pcrel, I felt it would be
	       * confusing to do it this way.  */

	      if ((reloc_type == BFD_RELOC_32
		   || reloc_type == BFD_RELOC_X86_64_32S
		   || reloc_type == BFD_RELOC_64)
		  && GOT_symbol
		  && GOT_symbol == i.op[n].imms->X_add_symbol
		  && (i.op[n].imms->X_op == O_symbol
		      || (i.op[n].imms->X_op == O_add
			  && ((symbol_get_value_expression
			       (i.op[n].imms->X_op_symbol)->X_op)
			      == O_subtract))))
		{
		  offsetT add;

		  if (insn_start_frag == frag_now)
		    add = (p - frag_now->fr_literal) - insn_start_off;
		  else
		    {
		      fragS *fr;

		      add = insn_start_frag->fr_fix - insn_start_off;
		      for (fr = insn_start_frag->fr_next;
			   fr && fr != frag_now; fr = fr->fr_next)
			add += fr->fr_fix;
		      add += p - frag_now->fr_literal;
		    }

		  if (!object_64bit)
		    reloc_type = BFD_RELOC_386_GOTPC;
		  else if (size == 4)
		    reloc_type = BFD_RELOC_X86_64_GOTPC32;
		  else if (size == 8)
		    reloc_type = BFD_RELOC_X86_64_GOTPC64;
		  i.op[n].imms->X_add_number += add;
		}
	      fix_new_exp (frag_now, p - frag_now->fr_literal, size,
			   i.op[n].imms, 0, reloc_type);
	    }
	}
    }
}

/* x86_cons_fix_new is called via the expression parsing code when a
   reloc is needed.  We use this hook to get the correct .got reloc.  */
static int cons_sign = -1;

void
x86_cons_fix_new (fragS *frag, unsigned int off, unsigned int len,
		  expressionS *exp, bfd_reloc_code_real_type r)
{
  r = reloc (len, 0, cons_sign, r);

#ifdef TE_PE
  if (exp->X_op == O_secrel)
    {
      exp->X_op = O_symbol;
      r = BFD_RELOC_32_SECREL;
    }
#endif

  fix_new_exp (frag, off, len, exp, 0, r);
}

/* Export the ABI address size for use by TC_ADDRESS_BYTES for the
   purpose of the `.dc.a' internal pseudo-op.  */

int
x86_address_bytes (void)
{
  if ((stdoutput->arch_info->mach & bfd_mach_x64_32))
    return 4;
  return stdoutput->arch_info->bits_per_address / 8;
}

#if !(defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF) || defined (OBJ_MACH_O)) \
    || defined (LEX_AT)
# define lex_got(reloc, adjust, types) NULL
#else
/* Parse operands of the form
   <symbol>@GOTOFF+<nnn>
   and similar .plt or .got references.

   If we find one, set up the correct relocation in RELOC and copy the
   input string, minus the `@GOTOFF' into a malloc'd buffer for
   parsing by the calling routine.  Return this buffer, and if ADJUST
   is non-null set it to the length of the string we removed from the
   input line.  Otherwise return NULL.  */
static char *
lex_got (enum bfd_reloc_code_real *rel,
	 int *adjust,
	 i386_operand_type *types)
{
  /* Some of the relocations depend on the size of what field is to
     be relocated.  But in our callers i386_immediate and i386_displacement
     we don't yet know the operand size (this will be set by insn
     matching).  Hence we record the word32 relocation here,
     and adjust the reloc according to the real size in reloc().  */
  static const struct {
    const char *str;
    int len;
    const enum bfd_reloc_code_real rel[2];
    const i386_operand_type types64;
  } gotrel[] = {
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
    { STRING_COMMA_LEN ("SIZE"),      { BFD_RELOC_SIZE32,
					BFD_RELOC_SIZE32 },
      OPERAND_TYPE_IMM32_64 },
#endif
    { STRING_COMMA_LEN ("PLTOFF"),   { _dummy_first_bfd_reloc_code_real,
				       BFD_RELOC_X86_64_PLTOFF64 },
      OPERAND_TYPE_IMM64 },
    { STRING_COMMA_LEN ("PLT"),      { BFD_RELOC_386_PLT32,
				       BFD_RELOC_X86_64_PLT32    },
      OPERAND_TYPE_IMM32_32S_DISP32 },
    { STRING_COMMA_LEN ("GOTPLT"),   { _dummy_first_bfd_reloc_code_real,
				       BFD_RELOC_X86_64_GOTPLT64 },
      OPERAND_TYPE_IMM64_DISP64 },
    { STRING_COMMA_LEN ("GOTOFF"),   { BFD_RELOC_386_GOTOFF,
				       BFD_RELOC_X86_64_GOTOFF64 },
      OPERAND_TYPE_IMM64_DISP64 },
    { STRING_COMMA_LEN ("GOTPCREL"), { _dummy_first_bfd_reloc_code_real,
				       BFD_RELOC_X86_64_GOTPCREL },
      OPERAND_TYPE_IMM32_32S_DISP32 },
    { STRING_COMMA_LEN ("TLSGD"),    { BFD_RELOC_386_TLS_GD,
				       BFD_RELOC_X86_64_TLSGD    },
      OPERAND_TYPE_IMM32_32S_DISP32 },
    { STRING_COMMA_LEN ("TLSLDM"),   { BFD_RELOC_386_TLS_LDM,
				       _dummy_first_bfd_reloc_code_real },
      OPERAND_TYPE_NONE },
    { STRING_COMMA_LEN ("TLSLD"),    { _dummy_first_bfd_reloc_code_real,
				       BFD_RELOC_X86_64_TLSLD    },
      OPERAND_TYPE_IMM32_32S_DISP32 },
    { STRING_COMMA_LEN ("GOTTPOFF"), { BFD_RELOC_386_TLS_IE_32,
				       BFD_RELOC_X86_64_GOTTPOFF },
      OPERAND_TYPE_IMM32_32S_DISP32 },
    { STRING_COMMA_LEN ("TPOFF"),    { BFD_RELOC_386_TLS_LE_32,
				       BFD_RELOC_X86_64_TPOFF32  },
      OPERAND_TYPE_IMM32_32S_64_DISP32_64 },
    { STRING_COMMA_LEN ("NTPOFF"),   { BFD_RELOC_386_TLS_LE,
				       _dummy_first_bfd_reloc_code_real },
      OPERAND_TYPE_NONE },
    { STRING_COMMA_LEN ("DTPOFF"),   { BFD_RELOC_386_TLS_LDO_32,
				       BFD_RELOC_X86_64_DTPOFF32 },
      OPERAND_TYPE_IMM32_32S_64_DISP32_64 },
    { STRING_COMMA_LEN ("GOTNTPOFF"),{ BFD_RELOC_386_TLS_GOTIE,
				       _dummy_first_bfd_reloc_code_real },
      OPERAND_TYPE_NONE },
    { STRING_COMMA_LEN ("INDNTPOFF"),{ BFD_RELOC_386_TLS_IE,
				       _dummy_first_bfd_reloc_code_real },
      OPERAND_TYPE_NONE },
    { STRING_COMMA_LEN ("GOT"),      { BFD_RELOC_386_GOT32,
				       BFD_RELOC_X86_64_GOT32    },
      OPERAND_TYPE_IMM32_32S_64_DISP32 },
    { STRING_COMMA_LEN ("TLSDESC"),  { BFD_RELOC_386_TLS_GOTDESC,
				       BFD_RELOC_X86_64_GOTPC32_TLSDESC },
      OPERAND_TYPE_IMM32_32S_DISP32 },
    { STRING_COMMA_LEN ("TLSCALL"),  { BFD_RELOC_386_TLS_DESC_CALL,
				       BFD_RELOC_X86_64_TLSDESC_CALL },
      OPERAND_TYPE_IMM32_32S_DISP32 },
  };
  char *cp;
  unsigned int j;

#if defined (OBJ_MAYBE_ELF)
  if (!IS_ELF)
    return NULL;
#endif

  for (cp = input_line_pointer; *cp != '@'; cp++)
    if (is_end_of_line[(unsigned char) *cp] || *cp == ',')
      return NULL;

  for (j = 0; j < ARRAY_SIZE (gotrel); j++)
    {
      int len = gotrel[j].len;
      if (strncasecmp (cp + 1, gotrel[j].str, len) == 0)
	{
	  if (gotrel[j].rel[object_64bit] != 0)
	    {
	      int first, second;
	      char *tmpbuf, *past_reloc;

	      *rel = gotrel[j].rel[object_64bit];

	      if (types)
		{
		  if (flag_code != CODE_64BIT)
		    {
		      types->bitfield.imm32 = 1;
		      types->bitfield.disp32 = 1;
		    }
		  else
		    *types = gotrel[j].types64;
		}

	      if (j != 0 && GOT_symbol == NULL)
		GOT_symbol = symbol_find_or_make (GLOBAL_OFFSET_TABLE_NAME);

	      /* The length of the first part of our input line.  */
	      first = cp - input_line_pointer;

	      /* The second part goes from after the reloc token until
		 (and including) an end_of_line char or comma.  */
	      past_reloc = cp + 1 + len;
	      cp = past_reloc;
	      while (!is_end_of_line[(unsigned char) *cp] && *cp != ',')
		++cp;
	      second = cp + 1 - past_reloc;

	      /* Allocate and copy string.  The trailing NUL shouldn't
		 be necessary, but be safe.  */
	      tmpbuf = XNEWVEC (char, first + second + 2);
	      memcpy (tmpbuf, input_line_pointer, first);
	      if (second != 0 && *past_reloc != ' ')
		/* Replace the relocation token with ' ', so that
		   errors like foo@GOTOFF1 will be detected.  */
		tmpbuf[first++] = ' ';
	      else
		/* Increment length by 1 if the relocation token is
		   removed.  */
		len++;
	      if (adjust)
		*adjust = len;
	      memcpy (tmpbuf + first, past_reloc, second);
	      tmpbuf[first + second] = '\0';
	      return tmpbuf;
	    }

	  as_bad (_("@%s reloc is not supported with %d-bit output format"),
		  gotrel[j].str, 1 << (5 + object_64bit));
	  return NULL;
	}
    }

  /* Might be a symbol version string.  Don't as_bad here.  */
  return NULL;
}
#endif

#ifdef TE_PE
#ifdef lex_got
#undef lex_got
#endif
/* Parse operands of the form
   <symbol>@SECREL32+<nnn>

   If we find one, set up the correct relocation in RELOC and copy the
   input string, minus the `@SECREL32' into a malloc'd buffer for
   parsing by the calling routine.  Return this buffer, and if ADJUST
   is non-null set it to the length of the string we removed from the
   input line.  Otherwise return NULL.

   This function is copied from the ELF version above adjusted for PE targets.  */

static char *
lex_got (enum bfd_reloc_code_real *rel ATTRIBUTE_UNUSED,
	 int *adjust ATTRIBUTE_UNUSED,
	 i386_operand_type *types)
{
  static const struct
  {
    const char *str;
    int len;
    const enum bfd_reloc_code_real rel[2];
    const i386_operand_type types64;
  }
  gotrel[] =
  {
    { STRING_COMMA_LEN ("SECREL32"),    { BFD_RELOC_32_SECREL,
					  BFD_RELOC_32_SECREL },
      OPERAND_TYPE_IMM32_32S_64_DISP32_64 },
  };

  char *cp;
  unsigned j;

  for (cp = input_line_pointer; *cp != '@'; cp++)
    if (is_end_of_line[(unsigned char) *cp] || *cp == ',')
      return NULL;

  for (j = 0; j < ARRAY_SIZE (gotrel); j++)
    {
      int len = gotrel[j].len;

      if (strncasecmp (cp + 1, gotrel[j].str, len) == 0)
	{
	  if (gotrel[j].rel[object_64bit] != 0)
	    {
	      int first, second;
	      char *tmpbuf, *past_reloc;

	      *rel = gotrel[j].rel[object_64bit];
	      if (adjust)
		*adjust = len;

	      if (types)
		{
		  if (flag_code != CODE_64BIT)
		    {
		      types->bitfield.imm32 = 1;
		      types->bitfield.disp32 = 1;
		    }
		  else
		    *types = gotrel[j].types64;
		}

	      /* The length of the first part of our input line.  */
	      first = cp - input_line_pointer;

	      /* The second part goes from after the reloc token until
		 (and including) an end_of_line char or comma.  */
	      past_reloc = cp + 1 + len;
	      cp = past_reloc;
	      while (!is_end_of_line[(unsigned char) *cp] && *cp != ',')
		++cp;
	      second = cp + 1 - past_reloc;

	      /* Allocate and copy string.  The trailing NUL shouldn't
		 be necessary, but be safe.  */
	      tmpbuf = XNEWVEC (char, first + second + 2);
	      memcpy (tmpbuf, input_line_pointer, first);
	      if (second != 0 && *past_reloc != ' ')
		/* Replace the relocation token with ' ', so that
		   errors like foo@SECLREL321 will be detected.  */
		tmpbuf[first++] = ' ';
	      memcpy (tmpbuf + first, past_reloc, second);
	      tmpbuf[first + second] = '\0';
	      return tmpbuf;
	    }

	  as_bad (_("@%s reloc is not supported with %d-bit output format"),
		  gotrel[j].str, 1 << (5 + object_64bit));
	  return NULL;
	}
    }

  /* Might be a symbol version string.  Don't as_bad here.  */
  return NULL;
}

#endif /* TE_PE */

bfd_reloc_code_real_type
x86_cons (expressionS *exp, int size)
{
  bfd_reloc_code_real_type got_reloc = NO_RELOC;

  intel_syntax = -intel_syntax;

  exp->X_md = 0;
  if (size == 4 || (object_64bit && size == 8))
    {
      /* Handle @GOTOFF and the like in an expression.  */
      char *save;
      char *gotfree_input_line;
      int adjust = 0;

      save = input_line_pointer;
      gotfree_input_line = lex_got (&got_reloc, &adjust, NULL);
      if (gotfree_input_line)
	input_line_pointer = gotfree_input_line;

      expression (exp);

      if (gotfree_input_line)
	{
	  /* expression () has merrily parsed up to the end of line,
	     or a comma - in the wrong buffer.  Transfer how far
	     input_line_pointer has moved to the right buffer.  */
	  input_line_pointer = (save
				+ (input_line_pointer - gotfree_input_line)
				+ adjust);
	  free (gotfree_input_line);
	  if (exp->X_op == O_constant
	      || exp->X_op == O_absent
	      || exp->X_op == O_illegal
	      || exp->X_op == O_register
	      || exp->X_op == O_big)
	    {
	      char c = *input_line_pointer;
	      *input_line_pointer = 0;
	      as_bad (_("missing or invalid expression `%s'"), save);
	      *input_line_pointer = c;
	    }
	}
    }
  else
    expression (exp);

  intel_syntax = -intel_syntax;

  if (intel_syntax)
    i386_intel_simplify (exp);

  return got_reloc;
}

static void
signed_cons (int size)
{
  if (flag_code == CODE_64BIT)
    cons_sign = 1;
  cons (size);
  cons_sign = -1;
}

#ifdef TE_PE
static void
pe_directive_secrel (int dummy ATTRIBUTE_UNUSED)
{
  expressionS exp;

  do
    {
      expression (&exp);
      if (exp.X_op == O_symbol)
	exp.X_op = O_secrel;

      emit_expr (&exp, 4);
    }
  while (*input_line_pointer++ == ',');

  input_line_pointer--;
  demand_empty_rest_of_line ();
}
#endif

/* Handle Vector operations.  */

static char *
check_VecOperations (char *op_string, char *op_end)
{
  const reg_entry *mask;
  const char *saved;
  char *end_op;

  while (*op_string
	 && (op_end == NULL || op_string < op_end))
    {
      saved = op_string;
      if (*op_string == '{')
	{
	  op_string++;

	  /* Check broadcasts.  */
	  if (strncmp (op_string, "1to", 3) == 0)
	    {
	      int bcst_type;

	      if (i.broadcast)
		goto duplicated_vec_op;

	      op_string += 3;
	      if (*op_string == '8')
		bcst_type = BROADCAST_1TO8;
	      else if (*op_string == '4')
		bcst_type = BROADCAST_1TO4;
	      else if (*op_string == '2')
		bcst_type = BROADCAST_1TO2;
	      else if (*op_string == '1'
		       && *(op_string+1) == '6')
		{
		  bcst_type = BROADCAST_1TO16;
		  op_string++;
		}
	      else
		{
		  as_bad (_("Unsupported broadcast: `%s'"), saved);
		  return NULL;
		}
	      op_string++;

	      broadcast_op.type = bcst_type;
	      broadcast_op.operand = this_operand;
	      i.broadcast = &broadcast_op;
	    }
	  /* Check masking operation.  */
	  else if ((mask = parse_register (op_string, &end_op)) != NULL)
	    {
	      /* k0 can't be used for write mask.  */
	      if (!mask->reg_type.bitfield.regmask || mask->reg_num == 0)
		{
		  as_bad (_("`%s%s' can't be used for write mask"),
			  register_prefix, mask->reg_name);
		  return NULL;
		}

	      if (!i.mask)
		{
		  mask_op.mask = mask;
		  mask_op.zeroing = 0;
		  mask_op.operand = this_operand;
		  i.mask = &mask_op;
		}
	      else
		{
		  if (i.mask->mask)
		    goto duplicated_vec_op;

		  i.mask->mask = mask;

		  /* Only "{z}" is allowed here.  No need to check
		     zeroing mask explicitly.  */
		  if (i.mask->operand != this_operand)
		    {
		      as_bad (_("invalid write mask `%s'"), saved);
		      return NULL;
		    }
		}

	      op_string = end_op;
	    }
	  /* Check zeroing-flag for masking operation.  */
	  else if (*op_string == 'z')
	    {
	      if (!i.mask)
		{
		  mask_op.mask = NULL;
		  mask_op.zeroing = 1;
		  mask_op.operand = this_operand;
		  i.mask = &mask_op;
		}
	      else
		{
		  if (i.mask->zeroing)
		    {
		    duplicated_vec_op:
		      as_bad (_("duplicated `%s'"), saved);
		      return NULL;
		    }

		  i.mask->zeroing = 1;

		  /* Only "{%k}" is allowed here.  No need to check mask
		     register explicitly.  */
		  if (i.mask->operand != this_operand)
		    {
		      as_bad (_("invalid zeroing-masking `%s'"),
			      saved);
		      return NULL;
		    }
		}

	      op_string++;
	    }
	  else
	    goto unknown_vec_op;

	  if (*op_string != '}')
	    {
	      as_bad (_("missing `}' in `%s'"), saved);
	      return NULL;
	    }
	  op_string++;
	  continue;
	}
    unknown_vec_op:
      /* We don't know this one.  */
      as_bad (_("unknown vector operation: `%s'"), saved);
      return NULL;
    }

  if (i.mask && i.mask->zeroing && !i.mask->mask)
    {
      as_bad (_("zeroing-masking only allowed with write mask"));
      return NULL;
    }

  return op_string;
}

static int
i386_immediate (char *imm_start)
{
  char *save_input_line_pointer;
  char *gotfree_input_line;
  segT exp_seg = 0;
  expressionS *exp;
  i386_operand_type types;

  operand_type_set (&types, ~0);

  if (i.imm_operands == MAX_IMMEDIATE_OPERANDS)
    {
      as_bad (_("at most %d immediate operands are allowed"),
	      MAX_IMMEDIATE_OPERANDS);
      return 0;
    }

  exp = &im_expressions[i.imm_operands++];
  i.op[this_operand].imms = exp;

  if (is_space_char (*imm_start))
    ++imm_start;

  save_input_line_pointer = input_line_pointer;
  input_line_pointer = imm_start;

  gotfree_input_line = lex_got (&i.reloc[this_operand], NULL, &types);
  if (gotfree_input_line)
    input_line_pointer = gotfree_input_line;

  exp_seg = expression (exp);

  SKIP_WHITESPACE ();

  /* Handle vector operations.  */
  if (*input_line_pointer == '{')
    {
      input_line_pointer = check_VecOperations (input_line_pointer,
						NULL);
      if (input_line_pointer == NULL)
	return 0;
    }

  if (*input_line_pointer)
    as_bad (_("junk `%s' after expression"), input_line_pointer);

  input_line_pointer = save_input_line_pointer;
  if (gotfree_input_line)
    {
      free (gotfree_input_line);

      if (exp->X_op == O_constant || exp->X_op == O_register)
	exp->X_op = O_illegal;
    }

  return i386_finalize_immediate (exp_seg, exp, types, imm_start);
}

static int
i386_finalize_immediate (segT exp_seg ATTRIBUTE_UNUSED, expressionS *exp,
			 i386_operand_type types, const char *imm_start)
{
  if (exp->X_op == O_absent || exp->X_op == O_illegal || exp->X_op == O_big)
    {
      if (imm_start)
	as_bad (_("missing or invalid immediate expression `%s'"),
		imm_start);
      return 0;
    }
  else if (exp->X_op == O_constant)
    {
      /* Size it properly later.  */
      i.types[this_operand].bitfield.imm64 = 1;
      /* If not 64bit, sign extend val.  */
      if (flag_code != CODE_64BIT
	  && (exp->X_add_number & ~(((addressT) 2 << 31) - 1)) == 0)
	exp->X_add_number
	  = (exp->X_add_number ^ ((addressT) 1 << 31)) - ((addressT) 1 << 31);
    }
#if (defined (OBJ_AOUT) || defined (OBJ_MAYBE_AOUT))
  else if (OUTPUT_FLAVOR == bfd_target_aout_flavour
	   && exp_seg != absolute_section
	   && exp_seg != text_section
	   && exp_seg != data_section
	   && exp_seg != bss_section
	   && exp_seg != undefined_section
	   && !bfd_is_com_section (exp_seg))
    {
      as_bad (_("unimplemented segment %s in operand"), exp_seg->name);
      return 0;
    }
#endif
  else if (!intel_syntax && exp_seg == reg_section)
    {
      if (imm_start)
	as_bad (_("illegal immediate register operand %s"), imm_start);
      return 0;
    }
  else
    {
      /* This is an address.  The size of the address will be
	 determined later, depending on destination register,
	 suffix, or the default for the section.  */
      i.types[this_operand].bitfield.imm8 = 1;
      i.types[this_operand].bitfield.imm16 = 1;
      i.types[this_operand].bitfield.imm32 = 1;
      i.types[this_operand].bitfield.imm32s = 1;
      i.types[this_operand].bitfield.imm64 = 1;
      i.types[this_operand] = operand_type_and (i.types[this_operand],
						types);
    }

  return 1;
}

static char *
i386_scale (char *scale)
{
  offsetT val;
  char *save = input_line_pointer;

  input_line_pointer = scale;
  val = get_absolute_expression ();

  switch (val)
    {
    case 1:
      i.log2_scale_factor = 0;
      break;
    case 2:
      i.log2_scale_factor = 1;
      break;
    case 4:
      i.log2_scale_factor = 2;
      break;
    case 8:
      i.log2_scale_factor = 3;
      break;
    default:
      {
	char sep = *input_line_pointer;

	*input_line_pointer = '\0';
	as_bad (_("expecting scale factor of 1, 2, 4, or 8: got `%s'"),
		scale);
	*input_line_pointer = sep;
	input_line_pointer = save;
	return NULL;
      }
    }
  if (i.log2_scale_factor != 0 && i.index_reg == 0)
    {
      as_warn (_("scale factor of %d without an index register"),
	       1 << i.log2_scale_factor);
      i.log2_scale_factor = 0;
    }
  scale = input_line_pointer;
  input_line_pointer = save;
  return scale;
}

static int
i386_displacement (char *disp_start, char *disp_end)
{
  expressionS *exp;
  segT exp_seg = 0;
  char *save_input_line_pointer;
  char *gotfree_input_line;
  int override;
  i386_operand_type bigdisp, types = anydisp;
  int ret;

  if (i.disp_operands == MAX_MEMORY_OPERANDS)
    {
      as_bad (_("at most %d displacement operands are allowed"),
	      MAX_MEMORY_OPERANDS);
      return 0;
    }

  operand_type_set (&bigdisp, 0);
  if ((i.types[this_operand].bitfield.jumpabsolute)
      || (!current_templates->start->opcode_modifier.jump
	  && !current_templates->start->opcode_modifier.jumpdword))
    {
      bigdisp.bitfield.disp32 = 1;
      override = (i.prefix[ADDR_PREFIX] != 0);
      if (flag_code == CODE_64BIT)
	{
	  if (!override)
	    {
	      bigdisp.bitfield.disp32s = 1;
	      bigdisp.bitfield.disp64 = 1;
	    }
	}
      else if ((flag_code == CODE_16BIT) ^ override)
	{
	  bigdisp.bitfield.disp32 = 0;
	  bigdisp.bitfield.disp16 = 1;
	}
    }
  else
    {
      /* For PC-relative branches, the width of the displacement
	 is dependent upon data size, not address size.  */
      override = (i.prefix[DATA_PREFIX] != 0);
      if (flag_code == CODE_64BIT)
	{
	  if (override || i.suffix == WORD_MNEM_SUFFIX)
	    bigdisp.bitfield.disp16 = 1;
	  else
	    {
	      bigdisp.bitfield.disp32 = 1;
	      bigdisp.bitfield.disp32s = 1;
	    }
	}
      else
	{
	  if (!override)
	    override = (i.suffix == (flag_code != CODE_16BIT
				     ? WORD_MNEM_SUFFIX
				     : LONG_MNEM_SUFFIX));
	  bigdisp.bitfield.disp32 = 1;
	  if ((flag_code == CODE_16BIT) ^ override)
	    {
	      bigdisp.bitfield.disp32 = 0;
	      bigdisp.bitfield.disp16 = 1;
	    }
	}
    }
  i.types[this_operand] = operand_type_or (i.types[this_operand],
					   bigdisp);

  exp = &disp_expressions[i.disp_operands];
  i.op[this_operand].disps = exp;
  i.disp_operands++;
  save_input_line_pointer = input_line_pointer;
  input_line_pointer = disp_start;
  END_STRING_AND_SAVE (disp_end);

#ifndef GCC_ASM_O_HACK
#define GCC_ASM_O_HACK 0
#endif
#if GCC_ASM_O_HACK
  END_STRING_AND_SAVE (disp_end + 1);
  if (i.types[this_operand].bitfield.baseIndex
      && displacement_string_end[-1] == '+')
    {
      /* This hack is to avoid a warning when using the "o"
	 constraint within gcc asm statements.
	 For instance:

	 #define _set_tssldt_desc(n,addr,limit,type) \
	 __asm__ __volatile__ ( \
	 "movw %w2,%0\n\t" \
	 "movw %w1,2+%0\n\t" \
	 "rorl $16,%1\n\t" \
	 "movb %b1,4+%0\n\t" \
	 "movb %4,5+%0\n\t" \
	 "movb $0,6+%0\n\t" \
	 "movb %h1,7+%0\n\t" \
	 "rorl $16,%1" \
	 : "=o"(*(n)) : "q" (addr), "ri"(limit), "i"(type))

	 This works great except that the output assembler ends
	 up looking a bit weird if it turns out that there is
	 no offset.  You end up producing code that looks like:

	 #APP
	 movw $235,(%eax)
	 movw %dx,2+(%eax)
	 rorl $16,%edx
	 movb %dl,4+(%eax)
	 movb $137,5+(%eax)
	 movb $0,6+(%eax)
	 movb %dh,7+(%eax)
	 rorl $16,%edx
	 #NO_APP

	 So here we provide the missing zero.  */

      *displacement_string_end = '0';
    }
#endif
  gotfree_input_line = lex_got (&i.reloc[this_operand], NULL, &types);
  if (gotfree_input_line)
    input_line_pointer = gotfree_input_line;

  exp_seg = expression (exp);

  SKIP_WHITESPACE ();
  if (*input_line_pointer)
    as_bad (_("junk `%s' after expression"), input_line_pointer);
#if GCC_ASM_O_HACK
  RESTORE_END_STRING (disp_end + 1);
#endif
  input_line_pointer = save_input_line_pointer;
  if (gotfree_input_line)
    {
      free (gotfree_input_line);

      if (exp->X_op == O_constant || exp->X_op == O_register)
	exp->X_op = O_illegal;
    }

  ret = i386_finalize_displacement (exp_seg, exp, types, disp_start);

  RESTORE_END_STRING (disp_end);

  return ret;
}

static int
i386_finalize_displacement (segT exp_seg ATTRIBUTE_UNUSED, expressionS *exp,
			    i386_operand_type types, const char *disp_start)
{
  i386_operand_type bigdisp;
  int ret = 1;

  /* We do this to make sure that the section symbol is in
     the symbol table.  We will ultimately change the relocation
     to be relative to the beginning of the section.  */
  if (i.reloc[this_operand] == BFD_RELOC_386_GOTOFF
      || i.reloc[this_operand] == BFD_RELOC_X86_64_GOTPCREL
      || i.reloc[this_operand] == BFD_RELOC_X86_64_GOTOFF64)
    {
      if (exp->X_op != O_symbol)
	goto inv_disp;

      if (S_IS_LOCAL (exp->X_add_symbol)
	  && S_GET_SEGMENT (exp->X_add_symbol) != undefined_section
	  && S_GET_SEGMENT (exp->X_add_symbol) != expr_section)
	section_symbol (S_GET_SEGMENT (exp->X_add_symbol));
      exp->X_op = O_subtract;
      exp->X_op_symbol = GOT_symbol;
      if (i.reloc[this_operand] == BFD_RELOC_X86_64_GOTPCREL)
	i.reloc[this_operand] = BFD_RELOC_32_PCREL;
      else if (i.reloc[this_operand] == BFD_RELOC_X86_64_GOTOFF64)
	i.reloc[this_operand] = BFD_RELOC_64;
      else
	i.reloc[this_operand] = BFD_RELOC_32;
    }

  else if (exp->X_op == O_absent
	   || exp->X_op == O_illegal
	   || exp->X_op == O_big)
    {
    inv_disp:
      as_bad (_("missing or invalid displacement expression `%s'"),
	      disp_start);
      ret = 0;
    }

  else if (flag_code == CODE_64BIT
	   && !i.prefix[ADDR_PREFIX]
	   && exp->X_op == O_constant)
    {
      /* Since displacement is signed extended to 64bit, don't allow
	 disp32 and turn off disp32s if they are out of range.  */
      i.types[this_operand].bitfield.disp32 = 0;
      if (!fits_in_signed_long (exp->X_add_number))
	{
	  i.types[this_operand].bitfield.disp32s = 0;
	  if (i.types[this_operand].bitfield.baseindex)
	    {
	      as_bad (_("0x%lx out range of signed 32bit displacement"),
		      (long) exp->X_add_number);
	      ret = 0;
	    }
	}
    }

#if (defined (OBJ_AOUT) || defined (OBJ_MAYBE_AOUT))
  else if (exp->X_op != O_constant
	   && OUTPUT_FLAVOR == bfd_target_aout_flavour
	   && exp_seg != absolute_section
	   && exp_seg != text_section
	   && exp_seg != data_section
	   && exp_seg != bss_section
	   && exp_seg != undefined_section
	   && !bfd_is_com_section (exp_seg))
    {
      as_bad (_("unimplemented segment %s in operand"), exp_seg->name);
      ret = 0;
    }
#endif

  /* Check if this is a displacement only operand.  */
  bigdisp = i.types[this_operand];
  bigdisp.bitfield.disp8 = 0;
  bigdisp.bitfield.disp16 = 0;
  bigdisp.bitfield.disp32 = 0;
  bigdisp.bitfield.disp32s = 0;
  bigdisp.bitfield.disp64 = 0;
  if (operand_type_all_zero (&bigdisp))
    i.types[this_operand] = operand_type_and (i.types[this_operand],
					      types);

  return ret;
}

/* Return the active addressing mode, taking address override and
   registers forming the address into consideration.  Update the
   address override prefix if necessary.  */

static enum flag_code
i386_addressing_mode (void)
{
  enum flag_code addr_mode;

  if (i.prefix[ADDR_PREFIX])
    addr_mode = flag_code == CODE_32BIT ? CODE_16BIT : CODE_32BIT;
  else
    {
      addr_mode = flag_code;

#if INFER_ADDR_PREFIX
      if (i.mem_operands == 0)
	{
	  /* Infer address prefix from the first memory operand.  */
	  const reg_entry *addr_reg = i.base_reg;

	  if (addr_reg == NULL)
	    addr_reg = i.index_reg;

	  if (addr_reg)
	    {
	      if (addr_reg->reg_num == RegEip
		  || addr_reg->reg_num == RegEiz
		  || addr_reg->reg_type.bitfield.dword)
		addr_mode = CODE_32BIT;
	      else if (flag_code != CODE_64BIT
		       && addr_reg->reg_type.bitfield.word)
		addr_mode = CODE_16BIT;

	      if (addr_mode != flag_code)
		{
		  i.prefix[ADDR_PREFIX] = ADDR_PREFIX_OPCODE;
		  i.prefixes += 1;
		  /* Change the size of any displacement too.  At most one
		     of Disp16 or Disp32 is set.
		     FIXME.  There doesn't seem to be any real need for
		     separate Disp16 and Disp32 flags.  The same goes for
		     Imm16 and Imm32.  Removing them would probably clean
		     up the code quite a lot.  */
		  if (flag_code != CODE_64BIT
		      && (i.types[this_operand].bitfield.disp16
			  || i.types[this_operand].bitfield.disp32))
		    i.types[this_operand]
		      = operand_type_xor (i.types[this_operand], disp16_32);
		}
	    }
	}
#endif
    }

  return addr_mode;
}

/* Make sure the memory operand we've been dealt is valid.
   Return 1 on success, 0 on a failure.  */

static int
i386_index_check (const char *operand_string)
{
  const char *kind = "base/index";
  enum flag_code addr_mode = i386_addressing_mode ();

  if (current_templates->start->opcode_modifier.isstring
      && !current_templates->start->opcode_modifier.immext
      && (current_templates->end[-1].opcode_modifier.isstring
	  || i.mem_operands))
    {
      /* Memory operands of string insns are special in that they only allow
	 a single register (rDI, rSI, or rBX) as their memory address.  */
      const reg_entry *expected_reg;
      static const char *di_si[][2] =
	{
	  { "esi", "edi" },
	  { "si", "di" },
	  { "rsi", "rdi" }
	};
      static const char *bx[] = { "ebx", "bx", "rbx" };

      kind = "string address";

      if (current_templates->start->opcode_modifier.repprefixok)
	{
	  i386_operand_type type = current_templates->end[-1].operand_types[0];

	  if (!type.bitfield.baseindex
	      || ((!i.mem_operands != !intel_syntax)
		  && current_templates->end[-1].operand_types[1]
		     .bitfield.baseindex))
	    type = current_templates->end[-1].operand_types[1];
	  expected_reg = hash_find (reg_hash,
				    di_si[addr_mode][type.bitfield.esseg]);

	}
      else
	expected_reg = hash_find (reg_hash, bx[addr_mode]);

      if (i.base_reg != expected_reg
	  || i.index_reg
	  || operand_type_check (i.types[this_operand], disp))
	{
	  /* The second memory operand must have the same size as
	     the first one.  */
	  if (i.mem_operands
	      && i.base_reg
	      && !((addr_mode == CODE_64BIT
		    && i.base_reg->reg_type.bitfield.qword)
		   || (addr_mode == CODE_32BIT
		       ? i.base_reg->reg_type.bitfield.dword
		       : i.base_reg->reg_type.bitfield.word)))
	    goto bad_address;

	  as_warn (_("`%s' is not valid here (expected `%c%s%s%c')"),
		   operand_string,
		   intel_syntax ? '[' : '(',
		   register_prefix,
		   expected_reg->reg_name,
		   intel_syntax ? ']' : ')');
	  return 1;
	}
      else
	return 1;

bad_address:
      as_bad (_("`%s' is not a valid %s expression"),
	      operand_string, kind);
      return 0;
    }
  else
    {
      if (addr_mode != CODE_16BIT)
	{
	  /* 32-bit/64-bit checks.  */
	  if ((i.base_reg
	       && (addr_mode == CODE_64BIT
		   ? !i.base_reg->reg_type.bitfield.qword
		   : !i.base_reg->reg_type.bitfield.dword)
	       && (i.index_reg
		   || (i.base_reg->reg_num
		       != (addr_mode == CODE_64BIT ? RegRip : RegEip))))
	      || (i.index_reg
		  && !i.index_reg->reg_type.bitfield.xmmword
		  && !i.index_reg->reg_type.bitfield.ymmword
		  && !i.index_reg->reg_type.bitfield.zmmword
		  && ((addr_mode == CODE_64BIT
		       ? !(i.index_reg->reg_type.bitfield.qword
			   || i.index_reg->reg_num == RegRiz)
		       : !(i.index_reg->reg_type.bitfield.dword
			   || i.index_reg->reg_num == RegEiz))
		      || !i.index_reg->reg_type.bitfield.baseindex)))
	    goto bad_address;

	  /* bndmk, bndldx, and bndstx have special restrictions. */
	  if (current_templates->start->base_opcode == 0xf30f1b
	      || (current_templates->start->base_opcode & ~1) == 0x0f1a)
	    {
	      /* They cannot use RIP-relative addressing. */
	      if (i.base_reg && i.base_reg->reg_num == RegRip)
		{
		  as_bad (_("`%s' cannot be used here"), operand_string);
		  return 0;
		}

	      /* bndldx and bndstx ignore their scale factor. */
	      if (current_templates->start->base_opcode != 0xf30f1b
		  && i.log2_scale_factor)
		as_warn (_("register scaling is being ignored here"));
	    }
	}
      else
	{
	  /* 16-bit checks.  */
	  if ((i.base_reg
	       && (!i.base_reg->reg_type.bitfield.word
		   || !i.base_reg->reg_type.bitfield.baseindex))
	      || (i.index_reg
		  && (!i.index_reg->reg_type.bitfield.word
		      || !i.index_reg->reg_type.bitfield.baseindex
		      || !(i.base_reg
			   && i.base_reg->reg_num < 6
			   && i.index_reg->reg_num >= 6
			   && i.log2_scale_factor == 0))))
	    goto bad_address;
	}
    }
  return 1;
}

/* Handle vector immediates.  */

static int
RC_SAE_immediate (const char *imm_start)
{
  unsigned int match_found, j;
  const char *pstr = imm_start;
  expressionS *exp;

  if (*pstr != '{')
    return 0;

  pstr++;
  match_found = 0;
  for (j = 0; j < ARRAY_SIZE (RC_NamesTable); j++)
    {
      if (!strncmp (pstr, RC_NamesTable[j].name, RC_NamesTable[j].len))
	{
	  if (!i.rounding)
	    {
	      rc_op.type = RC_NamesTable[j].type;
	      rc_op.operand = this_operand;
	      i.rounding = &rc_op;
	    }
	  else
	    {
	      as_bad (_("duplicated `%s'"), imm_start);
	      return 0;
	    }
	  pstr += RC_NamesTable[j].len;
	  match_found = 1;
	  break;
	}
    }
  if (!match_found)
    return 0;

  if (*pstr++ != '}')
    {
      as_bad (_("Missing '}': '%s'"), imm_start);
      return 0;
    }
  /* RC/SAE immediate string should contain nothing more.  */;
  if (*pstr != 0)
    {
      as_bad (_("Junk after '}': '%s'"), imm_start);
      return 0;
    }

  exp = &im_expressions[i.imm_operands++];
  i.op[this_operand].imms = exp;

  exp->X_op = O_constant;
  exp->X_add_number = 0;
  exp->X_add_symbol = (symbolS *) 0;
  exp->X_op_symbol = (symbolS *) 0;

  i.types[this_operand].bitfield.imm8 = 1;
  return 1;
}

/* Only string instructions can have a second memory operand, so
   reduce current_templates to just those if it contains any.  */
static int
maybe_adjust_templates (void)
{
  const insn_template *t;

  gas_assert (i.mem_operands == 1);

  for (t = current_templates->start; t < current_templates->end; ++t)
    if (t->opcode_modifier.isstring)
      break;

  if (t < current_templates->end)
    {
      static templates aux_templates;
      bfd_boolean recheck;

      aux_templates.start = t;
      for (; t < current_templates->end; ++t)
	if (!t->opcode_modifier.isstring)
	  break;
      aux_templates.end = t;

      /* Determine whether to re-check the first memory operand.  */
      recheck = (aux_templates.start != current_templates->start
		 || t != current_templates->end);

      current_templates = &aux_templates;

      if (recheck)
	{
	  i.mem_operands = 0;
	  if (i.memop1_string != NULL
	      && i386_index_check (i.memop1_string) == 0)
	    return 0;
	  i.mem_operands = 1;
	}
    }

  return 1;
}

/* Parse OPERAND_STRING into the i386_insn structure I.  Returns zero
   on error.  */

static int
i386_att_operand (char *operand_string)
{
  const reg_entry *r;
  char *end_op;
  char *op_string = operand_string;

  if (is_space_char (*op_string))
    ++op_string;

  /* We check for an absolute prefix (differentiating,
     for example, 'jmp pc_relative_label' from 'jmp *absolute_label'.  */
  if (*op_string == ABSOLUTE_PREFIX)
    {
      ++op_string;
      if (is_space_char (*op_string))
	++op_string;
      i.types[this_operand].bitfield.jumpabsolute = 1;
    }

  /* Check if operand is a register.  */
  if ((r = parse_register (op_string, &end_op)) != NULL)
    {
      i386_operand_type temp;

      /* Check for a segment override by searching for ':' after a
	 segment register.  */
      op_string = end_op;
      if (is_space_char (*op_string))
	++op_string;
      if (*op_string == ':'
	  && (r->reg_type.bitfield.sreg2
	      || r->reg_type.bitfield.sreg3))
	{
	  switch (r->reg_num)
	    {
	    case 0:
	      i.seg[i.mem_operands] = &es;
	      break;
	    case 1:
	      i.seg[i.mem_operands] = &cs;
	      break;
	    case 2:
	      i.seg[i.mem_operands] = &ss;
	      break;
	    case 3:
	      i.seg[i.mem_operands] = &ds;
	      break;
	    case 4:
	      i.seg[i.mem_operands] = &fs;
	      break;
	    case 5:
	      i.seg[i.mem_operands] = &gs;
	      break;
	    }

	  /* Skip the ':' and whitespace.  */
	  ++op_string;
	  if (is_space_char (*op_string))
	    ++op_string;

	  if (!is_digit_char (*op_string)
	      && !is_identifier_char (*op_string)
	      && *op_string != '('
	      && *op_string != ABSOLUTE_PREFIX)
	    {
	      as_bad (_("bad memory operand `%s'"), op_string);
	      return 0;
	    }
	  /* Handle case of %es:*foo.  */
	  if (*op_string == ABSOLUTE_PREFIX)
	    {
	      ++op_string;
	      if (is_space_char (*op_string))
		++op_string;
	      i.types[this_operand].bitfield.jumpabsolute = 1;
	    }
	  goto do_memory_reference;
	}

      /* Handle vector operations.  */
      if (*op_string == '{')
	{
	  op_string = check_VecOperations (op_string, NULL);
	  if (op_string == NULL)
	    return 0;
	}

      if (*op_string)
	{
	  as_bad (_("junk `%s' after register"), op_string);
	  return 0;
	}
      temp = r->reg_type;
      temp.bitfield.baseindex = 0;
      i.types[this_operand] = operand_type_or (i.types[this_operand],
					       temp);
      i.types[this_operand].bitfield.unspecified = 0;
      i.op[this_operand].regs = r;
      i.reg_operands++;
    }
  else if (*op_string == REGISTER_PREFIX)
    {
      as_bad (_("bad register name `%s'"), op_string);
      return 0;
    }
  else if (*op_string == IMMEDIATE_PREFIX)
    {
      ++op_string;
      if (i.types[this_operand].bitfield.jumpabsolute)
	{
	  as_bad (_("immediate operand illegal with absolute jump"));
	  return 0;
	}
      if (!i386_immediate (op_string))
	return 0;
    }
  else if (RC_SAE_immediate (operand_string))
    {
      /* If it is a RC or SAE immediate, do nothing.  */
      ;
    }
  else if (is_digit_char (*op_string)
	   || is_identifier_char (*op_string)
	   || *op_string == '"'
	   || *op_string == '(')
    {
      /* This is a memory reference of some sort.  */
      char *base_string;

      /* Start and end of displacement string expression (if found).  */
      char *displacement_string_start;
      char *displacement_string_end;
      char *vop_start;

    do_memory_reference:
      if (i.mem_operands == 1 && !maybe_adjust_templates ())
	return 0;
      if ((i.mem_operands == 1
	   && !current_templates->start->opcode_modifier.isstring)
	  || i.mem_operands == 2)
	{
	  as_bad (_("too many memory references for `%s'"),
		  current_templates->start->name);
	  return 0;
	}

      /* Check for base index form.  We detect the base index form by
	 looking for an ')' at the end of the operand, searching
	 for the '(' matching it, and finding a REGISTER_PREFIX or ','
	 after the '('.  */
      base_string = op_string + strlen (op_string);

      /* Handle vector operations.  */
      vop_start = strchr (op_string, '{');
      if (vop_start && vop_start < base_string)
	{
	  if (check_VecOperations (vop_start, base_string) == NULL)
	    return 0;
	  base_string = vop_start;
	}

      --base_string;
      if (is_space_char (*base_string))
	--base_string;

      /* If we only have a displacement, set-up for it to be parsed later.  */
      displacement_string_start = op_string;
      displacement_string_end = base_string + 1;

      if (*base_string == ')')
	{
	  char *temp_string;
	  unsigned int parens_balanced = 1;
	  /* We've already checked that the number of left & right ()'s are
	     equal, so this loop will not be infinite.  */
	  do
	    {
	      base_string--;
	      if (*base_string == ')')
		parens_balanced++;
	      if (*base_string == '(')
		parens_balanced--;
	    }
	  while (parens_balanced);

	  temp_string = base_string;

	  /* Skip past '(' and whitespace.  */
	  ++base_string;
	  if (is_space_char (*base_string))
	    ++base_string;

	  if (*base_string == ','
	      || ((i.base_reg = parse_register (base_string, &end_op))
		  != NULL))
	    {
	      displacement_string_end = temp_string;

	      i.types[this_operand].bitfield.baseindex = 1;

	      if (i.base_reg)
		{
		  base_string = end_op;
		  if (is_space_char (*base_string))
		    ++base_string;
		}

	      /* There may be an index reg or scale factor here.  */
	      if (*base_string == ',')
		{
		  ++base_string;
		  if (is_space_char (*base_string))
		    ++base_string;

		  if ((i.index_reg = parse_register (base_string, &end_op))
		      != NULL)
		    {
		      base_string = end_op;
		      if (is_space_char (*base_string))
			++base_string;
		      if (*base_string == ',')
			{
			  ++base_string;
			  if (is_space_char (*base_string))
			    ++base_string;
			}
		      else if (*base_string != ')')
			{
			  as_bad (_("expecting `,' or `)' "
				    "after index register in `%s'"),
				  operand_string);
			  return 0;
			}
		    }
		  else if (*base_string == REGISTER_PREFIX)
		    {
		      end_op = strchr (base_string, ',');
		      if (end_op)
			*end_op = '\0';
		      as_bad (_("bad register name `%s'"), base_string);
		      return 0;
		    }

		  /* Check for scale factor.  */
		  if (*base_string != ')')
		    {
		      char *end_scale = i386_scale (base_string);

		      if (!end_scale)
			return 0;

		      base_string = end_scale;
		      if (is_space_char (*base_string))
			++base_string;
		      if (*base_string != ')')
			{
			  as_bad (_("expecting `)' "
				    "after scale factor in `%s'"),
				  operand_string);
			  return 0;
			}
		    }
		  else if (!i.index_reg)
		    {
		      as_bad (_("expecting index register or scale factor "
				"after `,'; got '%c'"),
			      *base_string);
		      return 0;
		    }
		}
	      else if (*base_string != ')')
		{
		  as_bad (_("expecting `,' or `)' "
			    "after base register in `%s'"),
			  operand_string);
		  return 0;
		}
	    }
	  else if (*base_string == REGISTER_PREFIX)
	    {
	      end_op = strchr (base_string, ',');
	      if (end_op)
		*end_op = '\0';
	      as_bad (_("bad register name `%s'"), base_string);
	      return 0;
	    }
	}

      /* If there's an expression beginning the operand, parse it,
	 assuming displacement_string_start and
	 displacement_string_end are meaningful.  */
      if (displacement_string_start != displacement_string_end)
	{
	  if (!i386_displacement (displacement_string_start,
				  displacement_string_end))
	    return 0;
	}

      /* Special case for (%dx) while doing input/output op.  */
      if (i.base_reg
	  && operand_type_equal (&i.base_reg->reg_type,
				 &reg16_inoutportreg)
	  && i.index_reg == 0
	  && i.log2_scale_factor == 0
	  && i.seg[i.mem_operands] == 0
	  && !operand_type_check (i.types[this_operand], disp))
	{
	  i.types[this_operand] = inoutportreg;
	  return 1;
	}

      if (i386_index_check (operand_string) == 0)
	return 0;
      i.types[this_operand].bitfield.mem = 1;
      if (i.mem_operands == 0)
	i.memop1_string = xstrdup (operand_string);
      i.mem_operands++;
    }
  else
    {
      /* It's not a memory operand; argh!  */
      as_bad (_("invalid char %s beginning operand %d `%s'"),
	      output_invalid (*op_string),
	      this_operand + 1,
	      op_string);
      return 0;
    }
  return 1;			/* Normal return.  */
}

/* Calculate the maximum variable size (i.e., excluding fr_fix)
   that an rs_machine_dependent frag may reach.  */

unsigned int
i386_frag_max_var (fragS *frag)
{
  /* The only relaxable frags are for jumps.
     Unconditional jumps can grow by 4 bytes and others by 5 bytes.  */
  gas_assert (frag->fr_type == rs_machine_dependent);
  return TYPE_FROM_RELAX_STATE (frag->fr_subtype) == UNCOND_JUMP ? 4 : 5;
}

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
static int
elf_symbol_resolved_in_segment_p (symbolS *fr_symbol, offsetT fr_var)
{
  /* STT_GNU_IFUNC symbol must go through PLT.  */
  if ((symbol_get_bfdsym (fr_symbol)->flags
       & BSF_GNU_INDIRECT_FUNCTION) != 0)
    return 0;

  if (!S_IS_EXTERNAL (fr_symbol))
    /* Symbol may be weak or local.  */
    return !S_IS_WEAK (fr_symbol);

  /* Global symbols with non-default visibility can't be preempted. */
  if (ELF_ST_VISIBILITY (S_GET_OTHER (fr_symbol)) != STV_DEFAULT)
    return 1;

  if (fr_var != NO_RELOC)
    switch ((enum bfd_reloc_code_real) fr_var)
      {
      case BFD_RELOC_386_PLT32:
      case BFD_RELOC_X86_64_PLT32:
	/* Symbol with PLT relocation may be preempted. */
	return 0;
      default:
	abort ();
      }

  /* Global symbols with default visibility in a shared library may be
     preempted by another definition.  */
  return !shared;
}
#endif

/* md_estimate_size_before_relax()

   Called just before relax() for rs_machine_dependent frags.  The x86
   assembler uses these frags to handle variable size jump
   instructions.

   Any symbol that is now undefined will not become defined.
   Return the correct fr_subtype in the frag.
   Return the initial "guess for variable size of frag" to caller.
   The guess is actually the growth beyond the fixed part.  Whatever
   we do to grow the fixed or variable part contributes to our
   returned value.  */

int
md_estimate_size_before_relax (fragS *fragP, segT segment)
{
  /* We've already got fragP->fr_subtype right;  all we have to do is
     check for un-relaxable symbols.  On an ELF system, we can't relax
     an externally visible symbol, because it may be overridden by a
     shared library.  */
  if (S_GET_SEGMENT (fragP->fr_symbol) != segment
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
      || (IS_ELF
	  && !elf_symbol_resolved_in_segment_p (fragP->fr_symbol,
						fragP->fr_var))
#endif
#if defined (OBJ_COFF) && defined (TE_PE)
      || (OUTPUT_FLAVOR == bfd_target_coff_flavour
	  && S_IS_WEAK (fragP->fr_symbol))
#endif
      )
    {
      /* Symbol is undefined in this segment, or we need to keep a
	 reloc so that weak symbols can be overridden.  */
      int size = (fragP->fr_subtype & CODE16) ? 2 : 4;
      enum bfd_reloc_code_real reloc_type;
      unsigned char *opcode;
      int old_fr_fix;

      if (fragP->fr_var != NO_RELOC)
	reloc_type = (enum bfd_reloc_code_real) fragP->fr_var;
      else if (size == 2)
	reloc_type = BFD_RELOC_16_PCREL;
      else
	reloc_type = BFD_RELOC_32_PCREL;

      old_fr_fix = fragP->fr_fix;
      opcode = (unsigned char *) fragP->fr_opcode;

      switch (TYPE_FROM_RELAX_STATE (fragP->fr_subtype))
	{
	case UNCOND_JUMP:
	  /* Make jmp (0xeb) a (d)word displacement jump.  */
	  opcode[0] = 0xe9;
	  fragP->fr_fix += size;
	  fix_new (fragP, old_fr_fix, size,
		   fragP->fr_symbol,
		   fragP->fr_offset, 1,
		   reloc_type);
	  break;

	case COND_JUMP86:
	  if (size == 2
	      && (!no_cond_jump_promotion || fragP->fr_var != NO_RELOC))
	    {
	      /* Negate the condition, and branch past an
		 unconditional jump.  */
	      opcode[0] ^= 1;
	      opcode[1] = 3;
	      /* Insert an unconditional jump.  */
	      opcode[2] = 0xe9;
	      /* We added two extra opcode bytes, and have a two byte
		 offset.  */
	      fragP->fr_fix += 2 + 2;
	      fix_new (fragP, old_fr_fix + 2, 2,
		       fragP->fr_symbol,
		       fragP->fr_offset, 1,
		       reloc_type);
	      break;
	    }
	  /* Fall through.  */

	case COND_JUMP:
	  if (no_cond_jump_promotion && fragP->fr_var == NO_RELOC)
	    {
	      fixS *fixP;

	      fragP->fr_fix += 1;
	      fixP = fix_new (fragP, old_fr_fix, 1,
			      fragP->fr_symbol,
			      fragP->fr_offset, 1,
			      BFD_RELOC_8_PCREL);
	      fixP->fx_signed = 1;
	      break;
	    }

	  /* This changes the byte-displacement jump 0x7N
	     to the (d)word-displacement jump 0x0f,0x8N.  */
	  opcode[1] = opcode[0] + 0x10;
	  opcode[0] = TWO_BYTE_OPCODE_ESCAPE;
	  /* We've added an opcode byte.  */
	  fragP->fr_fix += 1 + size;
	  fix_new (fragP, old_fr_fix + 1, size,
		   fragP->fr_symbol,
		   fragP->fr_offset, 1,
		   reloc_type);
	  break;

	default:
	  BAD_CASE (fragP->fr_subtype);
	  break;
	}
      frag_wane (fragP);
      return fragP->fr_fix - old_fr_fix;
    }

  /* Guess size depending on current relax state.  Initially the relax
     state will correspond to a short jump and we return 1, because
     the variable part of the frag (the branch offset) is one byte
     long.  However, we can relax a section more than once and in that
     case we must either set fr_subtype back to the unrelaxed state,
     or return the value for the appropriate branch.  */
  return md_relax_table[fragP->fr_subtype].rlx_length;
}

/* Called after relax() is finished.

   In:	Address of frag.
	fr_type == rs_machine_dependent.
	fr_subtype is what the address relaxed to.

   Out:	Any fixSs and constants are set up.
	Caller will turn frag into a ".space 0".  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, segT sec ATTRIBUTE_UNUSED,
                 fragS *fragP)
{
  unsigned char *opcode;
  unsigned char *where_to_put_displacement = NULL;
  offsetT target_address;
  offsetT opcode_address;
  unsigned int extension = 0;
  offsetT displacement_from_opcode_start;

  opcode = (unsigned char *) fragP->fr_opcode;

  /* Address we want to reach in file space.  */
  target_address = S_GET_VALUE (fragP->fr_symbol) + fragP->fr_offset;

  /* Address opcode resides at in file space.  */
  opcode_address = fragP->fr_address + fragP->fr_fix;

  /* Displacement from opcode start to fill into instruction.  */
  displacement_from_opcode_start = target_address - opcode_address;

  if ((fragP->fr_subtype & BIG) == 0)
    {
      /* Don't have to change opcode.  */
      extension = 1;		/* 1 opcode + 1 displacement  */
      where_to_put_displacement = &opcode[1];
    }
  else
    {
      if (no_cond_jump_promotion
	  && TYPE_FROM_RELAX_STATE (fragP->fr_subtype) != UNCOND_JUMP)
	as_warn_where (fragP->fr_file, fragP->fr_line,
		       _("long jump required"));

      switch (fragP->fr_subtype)
	{
	case ENCODE_RELAX_STATE (UNCOND_JUMP, BIG):
	  extension = 4;		/* 1 opcode + 4 displacement  */
	  opcode[0] = 0xe9;
	  where_to_put_displacement = &opcode[1];
	  break;

	case ENCODE_RELAX_STATE (UNCOND_JUMP, BIG16):
	  extension = 2;		/* 1 opcode + 2 displacement  */
	  opcode[0] = 0xe9;
	  where_to_put_displacement = &opcode[1];
	  break;

	case ENCODE_RELAX_STATE (COND_JUMP, BIG):
	case ENCODE_RELAX_STATE (COND_JUMP86, BIG):
	  extension = 5;		/* 2 opcode + 4 displacement  */
	  opcode[1] = opcode[0] + 0x10;
	  opcode[0] = TWO_BYTE_OPCODE_ESCAPE;
	  where_to_put_displacement = &opcode[2];
	  break;

	case ENCODE_RELAX_STATE (COND_JUMP, BIG16):
	  extension = 3;		/* 2 opcode + 2 displacement  */
	  opcode[1] = opcode[0] + 0x10;
	  opcode[0] = TWO_BYTE_OPCODE_ESCAPE;
	  where_to_put_displacement = &opcode[2];
	  break;

	case ENCODE_RELAX_STATE (COND_JUMP86, BIG16):
	  extension = 4;
	  opcode[0] ^= 1;
	  opcode[1] = 3;
	  opcode[2] = 0xe9;
	  where_to_put_displacement = &opcode[3];
	  break;

	default:
	  BAD_CASE (fragP->fr_subtype);
	  break;
	}
    }

  /* If size if less then four we are sure that the operand fits,
     but if it's 4, then it could be that the displacement is larger
     then -/+ 2GB.  */
  if (DISP_SIZE_FROM_RELAX_STATE (fragP->fr_subtype) == 4
      && object_64bit
      && ((addressT) (displacement_from_opcode_start - extension
		      + ((addressT) 1 << 31))
	  > (((addressT) 2 << 31) - 1)))
    {
      as_bad_where (fragP->fr_file, fragP->fr_line,
		    _("jump target out of range"));
      /* Make us emit 0.  */
      displacement_from_opcode_start = extension;
    }
  /* Now put displacement after opcode.  */
  md_number_to_chars ((char *) where_to_put_displacement,
		      (valueT) (displacement_from_opcode_start - extension),
		      DISP_SIZE_FROM_RELAX_STATE (fragP->fr_subtype));
  fragP->fr_fix += extension;
}

/* Apply a fixup (fixP) to segment data, once it has been determined
   by our caller that we have all the info we need to fix it up.

   Parameter valP is the pointer to the value of the bits.

   On the 386, immediates, displacements, and data pointers are all in
   the same (little-endian) format, so we don't need to care about which
   we are handling.  */

void
md_apply_fix (fixS *fixP, valueT *valP, segT seg ATTRIBUTE_UNUSED)
{
  char *p = fixP->fx_where + fixP->fx_frag->fr_literal;
  valueT value = *valP;

#if !defined (TE_Mach)
  if (fixP->fx_pcrel)
    {
      switch (fixP->fx_r_type)
	{
	default:
	  break;

	case BFD_RELOC_64:
	  fixP->fx_r_type = BFD_RELOC_64_PCREL;
	  break;
	case BFD_RELOC_32:
	case BFD_RELOC_X86_64_32S:
	  fixP->fx_r_type = BFD_RELOC_32_PCREL;
	  break;
	case BFD_RELOC_16:
	  fixP->fx_r_type = BFD_RELOC_16_PCREL;
	  break;
	case BFD_RELOC_8:
	  fixP->fx_r_type = BFD_RELOC_8_PCREL;
	  break;
	}
    }

  if (fixP->fx_addsy != NULL
      && (fixP->fx_r_type == BFD_RELOC_32_PCREL
	  || fixP->fx_r_type == BFD_RELOC_64_PCREL
	  || fixP->fx_r_type == BFD_RELOC_16_PCREL
	  || fixP->fx_r_type == BFD_RELOC_8_PCREL)
      && !use_rela_relocations)
    {
      /* This is a hack.  There should be a better way to handle this.
	 This covers for the fact that bfd_install_relocation will
	 subtract the current location (for partial_inplace, PC relative
	 relocations); see more below.  */
#ifndef OBJ_AOUT
      if (IS_ELF
#ifdef TE_PE
	  || OUTPUT_FLAVOR == bfd_target_coff_flavour
#endif
	  )
	value += fixP->fx_where + fixP->fx_frag->fr_address;
#endif
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
      if (IS_ELF)
	{
	  segT sym_seg = S_GET_SEGMENT (fixP->fx_addsy);

	  if ((sym_seg == seg
	       || (symbol_section_p (fixP->fx_addsy)
		   && sym_seg != absolute_section))
	      && !generic_force_reloc (fixP))
	    {
	      /* Yes, we add the values in twice.  This is because
		 bfd_install_relocation subtracts them out again.  I think
		 bfd_install_relocation is broken, but I don't dare change
		 it.  FIXME.  */
	      value += fixP->fx_where + fixP->fx_frag->fr_address;
	    }
	}
#endif
#if defined (OBJ_COFF) && defined (TE_PE)
      /* For some reason, the PE format does not store a
	 section address offset for a PC relative symbol.  */
      if (S_GET_SEGMENT (fixP->fx_addsy) != seg
	  || S_IS_WEAK (fixP->fx_addsy))
	value += md_pcrel_from (fixP);
#endif
    }
#if defined (OBJ_COFF) && defined (TE_PE)
  if (fixP->fx_addsy != NULL
      && S_IS_WEAK (fixP->fx_addsy)
      /* PR 16858: Do not modify weak function references.  */
      && ! fixP->fx_pcrel)
    {
#if !defined (TE_PEP)
      /* For x86 PE weak function symbols are neither PC-relative
	 nor do they set S_IS_FUNCTION.  So the only reliable way
	 to detect them is to check the flags of their containing
	 section.  */
      if (S_GET_SEGMENT (fixP->fx_addsy) != NULL
	  && S_GET_SEGMENT (fixP->fx_addsy)->flags & SEC_CODE)
	;
      else
#endif
      value -= S_GET_VALUE (fixP->fx_addsy);
    }
#endif

  /* Fix a few things - the dynamic linker expects certain values here,
     and we must not disappoint it.  */
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  if (IS_ELF && fixP->fx_addsy)
    switch (fixP->fx_r_type)
      {
      case BFD_RELOC_386_PLT32:
      case BFD_RELOC_X86_64_PLT32:
	/* Make the jump instruction point to the address of the operand.  At
	   runtime we merely add the offset to the actual PLT entry.  */
	value = -4;
	break;

      case BFD_RELOC_386_TLS_GD:
      case BFD_RELOC_386_TLS_LDM:
      case BFD_RELOC_386_TLS_IE_32:
      case BFD_RELOC_386_TLS_IE:
      case BFD_RELOC_386_TLS_GOTIE:
      case BFD_RELOC_386_TLS_GOTDESC:
      case BFD_RELOC_X86_64_TLSGD:
      case BFD_RELOC_X86_64_TLSLD:
      case BFD_RELOC_X86_64_GOTTPOFF:
      case BFD_RELOC_X86_64_GOTPC32_TLSDESC:
	value = 0; /* Fully resolved at runtime.  No addend.  */
	/* Fallthrough */
      case BFD_RELOC_386_TLS_LE:
      case BFD_RELOC_386_TLS_LDO_32:
      case BFD_RELOC_386_TLS_LE_32:
      case BFD_RELOC_X86_64_DTPOFF32:
      case BFD_RELOC_X86_64_DTPOFF64:
      case BFD_RELOC_X86_64_TPOFF32:
      case BFD_RELOC_X86_64_TPOFF64:
	S_SET_THREAD_LOCAL (fixP->fx_addsy);
	break;

      case BFD_RELOC_386_TLS_DESC_CALL:
      case BFD_RELOC_X86_64_TLSDESC_CALL:
	value = 0; /* Fully resolved at runtime.  No addend.  */
	S_SET_THREAD_LOCAL (fixP->fx_addsy);
	fixP->fx_done = 0;
	return;

      case BFD_RELOC_VTABLE_INHERIT:
      case BFD_RELOC_VTABLE_ENTRY:
	fixP->fx_done = 0;
	return;

      default:
	break;
      }
#endif /* defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)  */
  *valP = value;
#endif /* !defined (TE_Mach)  */

  /* Are we finished with this relocation now?  */
  if (fixP->fx_addsy == NULL)
    fixP->fx_done = 1;
#if defined (OBJ_COFF) && defined (TE_PE)
  else if (fixP->fx_addsy != NULL && S_IS_WEAK (fixP->fx_addsy))
    {
      fixP->fx_done = 0;
      /* Remember value for tc_gen_reloc.  */
      fixP->fx_addnumber = value;
      /* Clear out the frag for now.  */
      value = 0;
    }
#endif
  else if (use_rela_relocations)
    {
      fixP->fx_no_overflow = 1;
      /* Remember value for tc_gen_reloc.  */
      fixP->fx_addnumber = value;
      value = 0;
    }

  md_number_to_chars (p, value, fixP->fx_size);
}

const char *
md_atof (int type, char *litP, int *sizeP)
{
  /* This outputs the LITTLENUMs in REVERSE order;
     in accord with the bigendian 386.  */
  return ieee_md_atof (type, litP, sizeP, FALSE);
}

static char output_invalid_buf[sizeof (unsigned char) * 2 + 6];

static char *
output_invalid (int c)
{
  if (ISPRINT (c))
    snprintf (output_invalid_buf, sizeof (output_invalid_buf),
	      "'%c'", c);
  else
    snprintf (output_invalid_buf, sizeof (output_invalid_buf),
	      "(0x%x)", (unsigned char) c);
  return output_invalid_buf;
}

/* REG_STRING starts *before* REGISTER_PREFIX.  */

static const reg_entry *
parse_real_register (char *reg_string, char **end_op)
{
  char *s = reg_string;
  char *p;
  char reg_name_given[MAX_REG_NAME_SIZE + 1];
  const reg_entry *r;

  /* Skip possible REGISTER_PREFIX and possible whitespace.  */
  if (*s == REGISTER_PREFIX)
    ++s;

  if (is_space_char (*s))
    ++s;

  p = reg_name_given;
  while ((*p++ = register_chars[(unsigned char) *s]) != '\0')
    {
      if (p >= reg_name_given + MAX_REG_NAME_SIZE)
	return (const reg_entry *) NULL;
      s++;
    }

  /* For naked regs, make sure that we are not dealing with an identifier.
     This prevents confusing an identifier like `eax_var' with register
     `eax'.  */
  if (allow_naked_reg && identifier_chars[(unsigned char) *s])
    return (const reg_entry *) NULL;

  *end_op = s;

  r = (const reg_entry *) hash_find (reg_hash, reg_name_given);

  /* Handle floating point regs, allowing spaces in the (i) part.  */
  if (r == i386_regtab /* %st is first entry of table  */)
    {
      if (is_space_char (*s))
	++s;
      if (*s == '(')
	{
	  ++s;
	  if (is_space_char (*s))
	    ++s;
	  if (*s >= '0' && *s <= '7')
	    {
	      int fpr = *s - '0';
	      ++s;
	      if (is_space_char (*s))
		++s;
	      if (*s == ')')
		{
		  *end_op = s + 1;
		  r = (const reg_entry *) hash_find (reg_hash, "st(0)");
		  know (r);
		  return r + fpr;
		}
	    }
	  /* We have "%st(" then garbage.  */
	  return (const reg_entry *) NULL;
	}
    }

  if (r == NULL || allow_pseudo_reg)
    return r;

  if (operand_type_all_zero (&r->reg_type))
    return (const reg_entry *) NULL;

  if ((r->reg_type.bitfield.dword
       || r->reg_type.bitfield.sreg3
       || r->reg_type.bitfield.control
       || r->reg_type.bitfield.debug
       || r->reg_type.bitfield.test)
      && !cpu_arch_flags.bitfield.cpui386)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.tbyte
      && !cpu_arch_flags.bitfield.cpu8087
      && !cpu_arch_flags.bitfield.cpu287
      && !cpu_arch_flags.bitfield.cpu387)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.regmmx && !cpu_arch_flags.bitfield.cpuregmmx)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.xmmword && !cpu_arch_flags.bitfield.cpuregxmm)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.ymmword && !cpu_arch_flags.bitfield.cpuregymm)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.zmmword && !cpu_arch_flags.bitfield.cpuregzmm)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.regmask
      && !cpu_arch_flags.bitfield.cpuregmask)
    return (const reg_entry *) NULL;

  /* Don't allow fake index register unless allow_index_reg isn't 0. */
  if (!allow_index_reg
      && (r->reg_num == RegEiz || r->reg_num == RegRiz))
    return (const reg_entry *) NULL;

  /* Upper 16 vector register is only available with VREX in 64bit
     mode.  */
  if ((r->reg_flags & RegVRex))
    {
      if (i.vec_encoding == vex_encoding_default)
	i.vec_encoding = vex_encoding_evex;

      if (!cpu_arch_flags.bitfield.cpuvrex
	  || i.vec_encoding != vex_encoding_evex
	  || flag_code != CODE_64BIT)
	return (const reg_entry *) NULL;
    }

  if (((r->reg_flags & (RegRex64 | RegRex))
       || r->reg_type.bitfield.qword)
      && (!cpu_arch_flags.bitfield.cpulm
	  || !operand_type_equal (&r->reg_type, &control))
      && flag_code != CODE_64BIT)
    return (const reg_entry *) NULL;

  if (r->reg_type.bitfield.sreg3 && r->reg_num == RegFlat && !intel_syntax)
    return (const reg_entry *) NULL;

  return r;
}

/* REG_STRING starts *before* REGISTER_PREFIX.  */

static const reg_entry *
parse_register (char *reg_string, char **end_op)
{
  const reg_entry *r;

  if (*reg_string == REGISTER_PREFIX || allow_naked_reg)
    r = parse_real_register (reg_string, end_op);
  else
    r = NULL;
  if (!r)
    {
      char *save = input_line_pointer;
      char c;
      symbolS *symbolP;

      input_line_pointer = reg_string;
      c = get_symbol_name (&reg_string);
      symbolP = symbol_find (reg_string);
      if (symbolP && S_GET_SEGMENT (symbolP) == reg_section)
	{
	  const expressionS *e = symbol_get_value_expression (symbolP);

	  know (e->X_op == O_register);
	  know (e->X_add_number >= 0
		&& (valueT) e->X_add_number < i386_regtab_size);
	  r = i386_regtab + e->X_add_number;
	  if ((r->reg_flags & RegVRex))
	    i.vec_encoding = vex_encoding_evex;
	  *end_op = input_line_pointer;
	}
      *input_line_pointer = c;
      input_line_pointer = save;
    }
  return r;
}

int
i386_parse_name (char *name, expressionS *e, char *nextcharP)
{
  const reg_entry *r;
  char *end = input_line_pointer;

  *end = *nextcharP;
  r = parse_register (name, &input_line_pointer);
  if (r && end <= input_line_pointer)
    {
      *nextcharP = *input_line_pointer;
      *input_line_pointer = 0;
      e->X_op = O_register;
      e->X_add_number = r - i386_regtab;
      return 1;
    }
  input_line_pointer = end;
  *end = 0;
  return intel_syntax ? i386_intel_parse_name (name, e) : 0;
}

void
md_operand (expressionS *e)
{
  char *end;
  const reg_entry *r;

  switch (*input_line_pointer)
    {
    case REGISTER_PREFIX:
      r = parse_real_register (input_line_pointer, &end);
      if (r)
	{
	  e->X_op = O_register;
	  e->X_add_number = r - i386_regtab;
	  input_line_pointer = end;
	}
      break;

    case '[':
      gas_assert (intel_syntax);
      end = input_line_pointer++;
      expression (e);
      if (*input_line_pointer == ']')
	{
	  ++input_line_pointer;
	  e->X_op_symbol = make_expr_symbol (e);
	  e->X_add_symbol = NULL;
	  e->X_add_number = 0;
	  e->X_op = O_index;
	}
      else
	{
	  e->X_op = O_absent;
	  input_line_pointer = end;
	}
      break;
    }
}


#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
const char *md_shortopts = "kVQ:sqn";
#else
const char *md_shortopts = "qn";
#endif

#define OPTION_32 (OPTION_MD_BASE + 0)
#define OPTION_64 (OPTION_MD_BASE + 1)
#define OPTION_DIVIDE (OPTION_MD_BASE + 2)
#define OPTION_MARCH (OPTION_MD_BASE + 3)
#define OPTION_MTUNE (OPTION_MD_BASE + 4)
#define OPTION_MMNEMONIC (OPTION_MD_BASE + 5)
#define OPTION_MSYNTAX (OPTION_MD_BASE + 6)
#define OPTION_MINDEX_REG (OPTION_MD_BASE + 7)
#define OPTION_MNAKED_REG (OPTION_MD_BASE + 8)
#define OPTION_MOLD_GCC (OPTION_MD_BASE + 9)
#define OPTION_MSSE2AVX (OPTION_MD_BASE + 10)
#define OPTION_MSSE_CHECK (OPTION_MD_BASE + 11)
#define OPTION_MOPERAND_CHECK (OPTION_MD_BASE + 12)
#define OPTION_MAVXSCALAR (OPTION_MD_BASE + 13)
#define OPTION_X32 (OPTION_MD_BASE + 14)
#define OPTION_MADD_BND_PREFIX (OPTION_MD_BASE + 15)
#define OPTION_MEVEXLIG (OPTION_MD_BASE + 16)
#define OPTION_MEVEXWIG (OPTION_MD_BASE + 17)
#define OPTION_MBIG_OBJ (OPTION_MD_BASE + 18)
#define OPTION_MOMIT_LOCK_PREFIX (OPTION_MD_BASE + 19)
#define OPTION_MEVEXRCIG (OPTION_MD_BASE + 20)
#define OPTION_MSHARED (OPTION_MD_BASE + 21)
#define OPTION_MAMD64 (OPTION_MD_BASE + 22)
#define OPTION_MINTEL64 (OPTION_MD_BASE + 23)
#define OPTION_MFENCE_AS_LOCK_ADD (OPTION_MD_BASE + 24)
#define OPTION_MRELAX_RELOCATIONS (OPTION_MD_BASE + 25)

struct option md_longopts[] =
{
  {"32", no_argument, NULL, OPTION_32},
#if (defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF) \
     || defined (TE_PE) || defined (TE_PEP) || defined (OBJ_MACH_O))
  {"64", no_argument, NULL, OPTION_64},
#endif
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  {"x32", no_argument, NULL, OPTION_X32},
  {"mshared", no_argument, NULL, OPTION_MSHARED},
#endif
  {"divide", no_argument, NULL, OPTION_DIVIDE},
  {"march", required_argument, NULL, OPTION_MARCH},
  {"mtune", required_argument, NULL, OPTION_MTUNE},
  {"mmnemonic", required_argument, NULL, OPTION_MMNEMONIC},
  {"msyntax", required_argument, NULL, OPTION_MSYNTAX},
  {"mindex-reg", no_argument, NULL, OPTION_MINDEX_REG},
  {"mnaked-reg", no_argument, NULL, OPTION_MNAKED_REG},
  {"mold-gcc", no_argument, NULL, OPTION_MOLD_GCC},
  {"msse2avx", no_argument, NULL, OPTION_MSSE2AVX},
  {"msse-check", required_argument, NULL, OPTION_MSSE_CHECK},
  {"moperand-check", required_argument, NULL, OPTION_MOPERAND_CHECK},
  {"mavxscalar", required_argument, NULL, OPTION_MAVXSCALAR},
  {"madd-bnd-prefix", no_argument, NULL, OPTION_MADD_BND_PREFIX},
  {"mevexlig", required_argument, NULL, OPTION_MEVEXLIG},
  {"mevexwig", required_argument, NULL, OPTION_MEVEXWIG},
# if defined (TE_PE) || defined (TE_PEP)
  {"mbig-obj", no_argument, NULL, OPTION_MBIG_OBJ},
#endif
  {"momit-lock-prefix", required_argument, NULL, OPTION_MOMIT_LOCK_PREFIX},
  {"mfence-as-lock-add", required_argument, NULL, OPTION_MFENCE_AS_LOCK_ADD},
  {"mrelax-relocations", required_argument, NULL, OPTION_MRELAX_RELOCATIONS},
  {"mevexrcig", required_argument, NULL, OPTION_MEVEXRCIG},
  {"mamd64", no_argument, NULL, OPTION_MAMD64},
  {"mintel64", no_argument, NULL, OPTION_MINTEL64},
  {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c, const char *arg)
{
  unsigned int j;
  char *arch, *next, *saved;

  switch (c)
    {
    case 'n':
      optimize_align_code = 0;
      break;

    case 'q':
      quiet_warnings = 1;
      break;

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
      /* -Qy, -Qn: SVR4 arguments controlling whether a .comment section
	 should be emitted or not.  FIXME: Not implemented.  */
    case 'Q':
      break;

      /* -V: SVR4 argument to print version ID.  */
    case 'V':
      print_version_id ();
      break;

      /* -k: Ignore for FreeBSD compatibility.  */
    case 'k':
      break;

    case 's':
      /* -s: On i386 Solaris, this tells the native assembler to use
	 .stab instead of .stab.excl.  We always use .stab anyhow.  */
      break;

    case OPTION_MSHARED:
      shared = 1;
      break;
#endif
#if (defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF) \
     || defined (TE_PE) || defined (TE_PEP) || defined (OBJ_MACH_O))
    case OPTION_64:
      {
	const char **list, **l;

	list = bfd_target_list ();
	for (l = list; *l != NULL; l++)
	  if (CONST_STRNEQ (*l, "elf64-x86-64")
	      || strcmp (*l, "coff-x86-64") == 0
	      || strcmp (*l, "pe-x86-64") == 0
	      || strcmp (*l, "pei-x86-64") == 0
	      || strcmp (*l, "mach-o-x86-64") == 0)
	    {
	      default_arch = "x86_64";
	      break;
	    }
	if (*l == NULL)
	  as_fatal (_("no compiled in support for x86_64"));
	free (list);
      }
      break;
#endif

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
    case OPTION_X32:
      if (IS_ELF)
	{
	  const char **list, **l;

	  list = bfd_target_list ();
	  for (l = list; *l != NULL; l++)
	    if (CONST_STRNEQ (*l, "elf32-x86-64"))
	      {
		default_arch = "x86_64:32";
		break;
	      }
	  if (*l == NULL)
	    as_fatal (_("no compiled in support for 32bit x86_64"));
	  free (list);
	}
      else
	as_fatal (_("32bit x86_64 is only supported for ELF"));
      break;
#endif

    case OPTION_32:
      default_arch = "i386";
      break;

    case OPTION_DIVIDE:
#ifdef SVR4_COMMENT_CHARS
      {
	char *n, *t;
	const char *s;

	n = XNEWVEC (char, strlen (i386_comment_chars) + 1);
	t = n;
	for (s = i386_comment_chars; *s != '\0'; s++)
	  if (*s != '/')
	    *t++ = *s;
	*t = '\0';
	i386_comment_chars = n;
      }
#endif
      break;

    case OPTION_MARCH:
      saved = xstrdup (arg);
      arch = saved;
      /* Allow -march=+nosse.  */
      if (*arch == '+')
	arch++;
      do
	{
	  if (*arch == '.')
	    as_fatal (_("invalid -march= option: `%s'"), arg);
	  next = strchr (arch, '+');
	  if (next)
	    *next++ = '\0';
	  for (j = 0; j < ARRAY_SIZE (cpu_arch); j++)
	    {
	      if (strcmp (arch, cpu_arch [j].name) == 0)
		{
		  /* Processor.  */
		  if (! cpu_arch[j].flags.bitfield.cpui386)
		    continue;

		  cpu_arch_name = cpu_arch[j].name;
		  cpu_sub_arch_name = NULL;
		  cpu_arch_flags = cpu_arch[j].flags;
		  cpu_arch_isa = cpu_arch[j].type;
		  cpu_arch_isa_flags = cpu_arch[j].flags;
		  if (!cpu_arch_tune_set)
		    {
		      cpu_arch_tune = cpu_arch_isa;
		      cpu_arch_tune_flags = cpu_arch_isa_flags;
		    }
		  break;
		}
	      else if (*cpu_arch [j].name == '.'
		       && strcmp (arch, cpu_arch [j].name + 1) == 0)
		{
		  /* ISA extension.  */
		  i386_cpu_flags flags;

		  flags = cpu_flags_or (cpu_arch_flags,
					cpu_arch[j].flags);

		  if (!cpu_flags_equal (&flags, &cpu_arch_flags))
		    {
		      if (cpu_sub_arch_name)
			{
			  char *name = cpu_sub_arch_name;
			  cpu_sub_arch_name = concat (name,
						      cpu_arch[j].name,
						      (const char *) NULL);
			  free (name);
			}
		      else
			cpu_sub_arch_name = xstrdup (cpu_arch[j].name);
		      cpu_arch_flags = flags;
		      cpu_arch_isa_flags = flags;
		    }
		  break;
		}
	    }

	  if (j >= ARRAY_SIZE (cpu_arch))
	    {
	      /* Disable an ISA extension.  */
	      for (j = 0; j < ARRAY_SIZE (cpu_noarch); j++)
		if (strcmp (arch, cpu_noarch [j].name) == 0)
		  {
		    i386_cpu_flags flags;

		    flags = cpu_flags_and_not (cpu_arch_flags,
					       cpu_noarch[j].flags);
		    if (!cpu_flags_equal (&flags, &cpu_arch_flags))
		      {
			if (cpu_sub_arch_name)
			  {
			    char *name = cpu_sub_arch_name;
			    cpu_sub_arch_name = concat (arch,
							(const char *) NULL);
			    free (name);
			  }
			else
			  cpu_sub_arch_name = xstrdup (arch);
			cpu_arch_flags = flags;
			cpu_arch_isa_flags = flags;
		      }
		    break;
		  }

	      if (j >= ARRAY_SIZE (cpu_noarch))
		j = ARRAY_SIZE (cpu_arch);
	    }

	  if (j >= ARRAY_SIZE (cpu_arch))
	    as_fatal (_("invalid -march= option: `%s'"), arg);

	  arch = next;
	}
      while (next != NULL);
      free (saved);
      break;

    case OPTION_MTUNE:
      if (*arg == '.')
	as_fatal (_("invalid -mtune= option: `%s'"), arg);
      for (j = 0; j < ARRAY_SIZE (cpu_arch); j++)
	{
	  if (strcmp (arg, cpu_arch [j].name) == 0)
	    {
	      cpu_arch_tune_set = 1;
	      cpu_arch_tune = cpu_arch [j].type;
	      cpu_arch_tune_flags = cpu_arch[j].flags;
	      break;
	    }
	}
      if (j >= ARRAY_SIZE (cpu_arch))
	as_fatal (_("invalid -mtune= option: `%s'"), arg);
      break;

    case OPTION_MMNEMONIC:
      if (strcasecmp (arg, "att") == 0)
	intel_mnemonic = 0;
      else if (strcasecmp (arg, "intel") == 0)
	intel_mnemonic = 1;
      else
	as_fatal (_("invalid -mmnemonic= option: `%s'"), arg);
      break;

    case OPTION_MSYNTAX:
      if (strcasecmp (arg, "att") == 0)
	intel_syntax = 0;
      else if (strcasecmp (arg, "intel") == 0)
	intel_syntax = 1;
      else
	as_fatal (_("invalid -msyntax= option: `%s'"), arg);
      break;

    case OPTION_MINDEX_REG:
      allow_index_reg = 1;
      break;

    case OPTION_MNAKED_REG:
      allow_naked_reg = 1;
      break;

    case OPTION_MOLD_GCC:
      old_gcc = 1;
      break;

    case OPTION_MSSE2AVX:
      sse2avx = 1;
      break;

    case OPTION_MSSE_CHECK:
      if (strcasecmp (arg, "error") == 0)
	sse_check = check_error;
      else if (strcasecmp (arg, "warning") == 0)
	sse_check = check_warning;
      else if (strcasecmp (arg, "none") == 0)
	sse_check = check_none;
      else
	as_fatal (_("invalid -msse-check= option: `%s'"), arg);
      break;

    case OPTION_MOPERAND_CHECK:
      if (strcasecmp (arg, "error") == 0)
	operand_check = check_error;
      else if (strcasecmp (arg, "warning") == 0)
	operand_check = check_warning;
      else if (strcasecmp (arg, "none") == 0)
	operand_check = check_none;
      else
	as_fatal (_("invalid -moperand-check= option: `%s'"), arg);
      break;

    case OPTION_MAVXSCALAR:
      if (strcasecmp (arg, "128") == 0)
	avxscalar = vex128;
      else if (strcasecmp (arg, "256") == 0)
	avxscalar = vex256;
      else
	as_fatal (_("invalid -mavxscalar= option: `%s'"), arg);
      break;

    case OPTION_MADD_BND_PREFIX:
      add_bnd_prefix = 1;
      break;

    case OPTION_MEVEXLIG:
      if (strcmp (arg, "128") == 0)
	evexlig = evexl128;
      else if (strcmp (arg, "256") == 0)
	evexlig = evexl256;
      else  if (strcmp (arg, "512") == 0)
	evexlig = evexl512;
      else
	as_fatal (_("invalid -mevexlig= option: `%s'"), arg);
      break;

    case OPTION_MEVEXRCIG:
      if (strcmp (arg, "rne") == 0)
	evexrcig = rne;
      else if (strcmp (arg, "rd") == 0)
	evexrcig = rd;
      else if (strcmp (arg, "ru") == 0)
	evexrcig = ru;
      else if (strcmp (arg, "rz") == 0)
	evexrcig = rz;
      else
	as_fatal (_("invalid -mevexrcig= option: `%s'"), arg);
      break;

    case OPTION_MEVEXWIG:
      if (strcmp (arg, "0") == 0)
	evexwig = evexw0;
      else if (strcmp (arg, "1") == 0)
	evexwig = evexw1;
      else
	as_fatal (_("invalid -mevexwig= option: `%s'"), arg);
      break;

# if defined (TE_PE) || defined (TE_PEP)
    case OPTION_MBIG_OBJ:
      use_big_obj = 1;
      break;
#endif

    case OPTION_MOMIT_LOCK_PREFIX:
      if (strcasecmp (arg, "yes") == 0)
        omit_lock_prefix = 1;
      else if (strcasecmp (arg, "no") == 0)
        omit_lock_prefix = 0;
      else
        as_fatal (_("invalid -momit-lock-prefix= option: `%s'"), arg);
      break;

    case OPTION_MFENCE_AS_LOCK_ADD:
      if (strcasecmp (arg, "yes") == 0)
        avoid_fence = 1;
      else if (strcasecmp (arg, "no") == 0)
        avoid_fence = 0;
      else
        as_fatal (_("invalid -mfence-as-lock-add= option: `%s'"), arg);
      break;

    case OPTION_MRELAX_RELOCATIONS:
      if (strcasecmp (arg, "yes") == 0)
        generate_relax_relocations = 1;
      else if (strcasecmp (arg, "no") == 0)
        generate_relax_relocations = 0;
      else
        as_fatal (_("invalid -mrelax-relocations= option: `%s'"), arg);
      break;

    case OPTION_MAMD64:
      intel64 = 0;
      break;

    case OPTION_MINTEL64:
      intel64 = 1;
      break;

    default:
      return 0;
    }
  return 1;
}

#define MESSAGE_TEMPLATE \
"                                                                                "

static char *
output_message (FILE *stream, char *p, char *message, char *start,
		int *left_p, const char *name, int len)
{
  int size = sizeof (MESSAGE_TEMPLATE);
  int left = *left_p;

  /* Reserve 2 spaces for ", " or ",\0" */
  left -= len + 2;

  /* Check if there is any room.  */
  if (left >= 0)
    {
      if (p != start)
	{
	  *p++ = ',';
	  *p++ = ' ';
	}
      p = mempcpy (p, name, len);
    }
  else
    {
      /* Output the current message now and start a new one.  */
      *p++ = ',';
      *p = '\0';
      fprintf (stream, "%s\n", message);
      p = start;
      left = size - (start - message) - len - 2;

      gas_assert (left >= 0);

      p = mempcpy (p, name, len);
    }

  *left_p = left;
  return p;
}

static void
show_arch (FILE *stream, int ext, int check)
{
  static char message[] = MESSAGE_TEMPLATE;
  char *start = message + 27;
  char *p;
  int size = sizeof (MESSAGE_TEMPLATE);
  int left;
  const char *name;
  int len;
  unsigned int j;

  p = start;
  left = size - (start - message);
  for (j = 0; j < ARRAY_SIZE (cpu_arch); j++)
    {
      /* Should it be skipped?  */
      if (cpu_arch [j].skip)
	continue;

      name = cpu_arch [j].name;
      len = cpu_arch [j].len;
      if (*name == '.')
	{
	  /* It is an extension.  Skip if we aren't asked to show it.  */
	  if (ext)
	    {
	      name++;
	      len--;
	    }
	  else
	    continue;
	}
      else if (ext)
	{
	  /* It is an processor.  Skip if we show only extension.  */
	  continue;
	}
      else if (check && ! cpu_arch[j].flags.bitfield.cpui386)
	{
	  /* It is an impossible processor - skip.  */
	  continue;
	}

      p = output_message (stream, p, message, start, &left, name, len);
    }

  /* Display disabled extensions.  */
  if (ext)
    for (j = 0; j < ARRAY_SIZE (cpu_noarch); j++)
      {
	name = cpu_noarch [j].name;
	len = cpu_noarch [j].len;
	p = output_message (stream, p, message, start, &left, name,
			    len);
      }

  *p = '\0';
  fprintf (stream, "%s\n", message);
}

void
md_show_usage (FILE *stream)
{
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  fprintf (stream, _("\
  -Q                      ignored\n\
  -V                      print assembler version number\n\
  -k                      ignored\n"));
#endif
  fprintf (stream, _("\
  -n                      Do not optimize code alignment\n\
  -q                      quieten some warnings\n"));
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  fprintf (stream, _("\
  -s                      ignored\n"));
#endif
#if (defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF) \
     || defined (TE_PE) || defined (TE_PEP))
  fprintf (stream, _("\
  --32/--64/--x32         generate 32bit/64bit/x32 code\n"));
#endif
#ifdef SVR4_COMMENT_CHARS
  fprintf (stream, _("\
  --divide                do not treat `/' as a comment character\n"));
#else
  fprintf (stream, _("\
  --divide                ignored\n"));
#endif
  fprintf (stream, _("\
  -march=CPU[,+EXTENSION...]\n\
                          generate code for CPU and EXTENSION, CPU is one of:\n"));
  show_arch (stream, 0, 1);
  fprintf (stream, _("\
                          EXTENSION is combination of:\n"));
  show_arch (stream, 1, 0);
  fprintf (stream, _("\
  -mtune=CPU              optimize for CPU, CPU is one of:\n"));
  show_arch (stream, 0, 0);
  fprintf (stream, _("\
  -msse2avx               encode SSE instructions with VEX prefix\n"));
  fprintf (stream, _("\
  -msse-check=[none|error|warning]\n\
                          check SSE instructions\n"));
  fprintf (stream, _("\
  -moperand-check=[none|error|warning]\n\
                          check operand combinations for validity\n"));
  fprintf (stream, _("\
  -mavxscalar=[128|256]   encode scalar AVX instructions with specific vector\n\
                           length\n"));
  fprintf (stream, _("\
  -mevexlig=[128|256|512] encode scalar EVEX instructions with specific vector\n\
                           length\n"));
  fprintf (stream, _("\
  -mevexwig=[0|1]         encode EVEX instructions with specific EVEX.W value\n\
                           for EVEX.W bit ignored instructions\n"));
  fprintf (stream, _("\
  -mevexrcig=[rne|rd|ru|rz]\n\
                          encode EVEX instructions with specific EVEX.RC value\n\
                           for SAE-only ignored instructions\n"));
  fprintf (stream, _("\
  -mmnemonic=[att|intel]  use AT&T/Intel mnemonic\n"));
  fprintf (stream, _("\
  -msyntax=[att|intel]    use AT&T/Intel syntax\n"));
  fprintf (stream, _("\
  -mindex-reg             support pseudo index registers\n"));
  fprintf (stream, _("\
  -mnaked-reg             don't require `%%' prefix for registers\n"));
  fprintf (stream, _("\
  -mold-gcc               support old (<= 2.8.1) versions of gcc\n"));
  fprintf (stream, _("\
  -madd-bnd-prefix        add BND prefix for all valid branches\n"));
  fprintf (stream, _("\
  -mshared                disable branch optimization for shared code\n"));
# if defined (TE_PE) || defined (TE_PEP)
  fprintf (stream, _("\
  -mbig-obj               generate big object files\n"));
#endif
  fprintf (stream, _("\
  -momit-lock-prefix=[no|yes]\n\
                          strip all lock prefixes\n"));
  fprintf (stream, _("\
  -mfence-as-lock-add=[no|yes]\n\
                          encode lfence, mfence and sfence as\n\
                           lock addl $0x0, (%%{re}sp)\n"));
  fprintf (stream, _("\
  -mrelax-relocations=[no|yes]\n\
                          generate relax relocations\n"));
  fprintf (stream, _("\
  -mamd64                 accept only AMD64 ISA\n"));
  fprintf (stream, _("\
  -mintel64               accept only Intel64 ISA\n"));
}

#if ((defined (OBJ_MAYBE_COFF) && defined (OBJ_MAYBE_AOUT)) \
     || defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF) \
     || defined (TE_PE) || defined (TE_PEP) || defined (OBJ_MACH_O))

/* Pick the target format to use.  */

const char *
i386_target_format (void)
{
  if (!strncmp (default_arch, "x86_64", 6))
    {
      update_code_flag (CODE_64BIT, 1);
      if (default_arch[6] == '\0')
	x86_elf_abi = X86_64_ABI;
      else
	x86_elf_abi = X86_64_X32_ABI;
    }
  else if (!strcmp (default_arch, "i386"))
    update_code_flag (CODE_32BIT, 1);
  else if (!strcmp (default_arch, "iamcu"))
    {
      update_code_flag (CODE_32BIT, 1);
      if (cpu_arch_isa == PROCESSOR_UNKNOWN)
	{
	  static const i386_cpu_flags iamcu_flags = CPU_IAMCU_FLAGS;
	  cpu_arch_name = "iamcu";
	  cpu_sub_arch_name = NULL;
	  cpu_arch_flags = iamcu_flags;
	  cpu_arch_isa = PROCESSOR_IAMCU;
	  cpu_arch_isa_flags = iamcu_flags;
	  if (!cpu_arch_tune_set)
	    {
	      cpu_arch_tune = cpu_arch_isa;
	      cpu_arch_tune_flags = cpu_arch_isa_flags;
	    }
	}
      else if (cpu_arch_isa != PROCESSOR_IAMCU)
	as_fatal (_("Intel MCU doesn't support `%s' architecture"),
		  cpu_arch_name);
    }
  else
    as_fatal (_("unknown architecture"));

  if (cpu_flags_all_zero (&cpu_arch_isa_flags))
    cpu_arch_isa_flags = cpu_arch[flag_code == CODE_64BIT].flags;
  if (cpu_flags_all_zero (&cpu_arch_tune_flags))
    cpu_arch_tune_flags = cpu_arch[flag_code == CODE_64BIT].flags;

  switch (OUTPUT_FLAVOR)
    {
#if defined (OBJ_MAYBE_AOUT) || defined (OBJ_AOUT)
    case bfd_target_aout_flavour:
      return AOUT_TARGET_FORMAT;
#endif
#if defined (OBJ_MAYBE_COFF) || defined (OBJ_COFF)
# if defined (TE_PE) || defined (TE_PEP)
    case bfd_target_coff_flavour:
      if (flag_code == CODE_64BIT)
	return use_big_obj ? "pe-bigobj-x86-64" : "pe-x86-64";
      else
	return "pe-i386";
# elif defined (TE_GO32)
    case bfd_target_coff_flavour:
      return "coff-go32";
# else
    case bfd_target_coff_flavour:
      return "coff-i386";
# endif
#endif
#if defined (OBJ_MAYBE_ELF) || defined (OBJ_ELF)
    case bfd_target_elf_flavour:
      {
	const char *format;

	switch (x86_elf_abi)
	  {
	  default:
	    format = ELF_TARGET_FORMAT;
	    break;
	  case X86_64_ABI:
	    use_rela_relocations = 1;
	    object_64bit = 1;
	    format = ELF_TARGET_FORMAT64;
	    break;
	  case X86_64_X32_ABI:
	    use_rela_relocations = 1;
	    object_64bit = 1;
	    disallow_64bit_reloc = 1;
	    format = ELF_TARGET_FORMAT32;
	    break;
	  }
	if (cpu_arch_isa == PROCESSOR_L1OM)
	  {
	    if (x86_elf_abi != X86_64_ABI)
	      as_fatal (_("Intel L1OM is 64bit only"));
	    return ELF_TARGET_L1OM_FORMAT;
	  }
	else if (cpu_arch_isa == PROCESSOR_K1OM)
	  {
	    if (x86_elf_abi != X86_64_ABI)
	      as_fatal (_("Intel K1OM is 64bit only"));
	    return ELF_TARGET_K1OM_FORMAT;
	  }
	else if (cpu_arch_isa == PROCESSOR_IAMCU)
	  {
	    if (x86_elf_abi != I386_ABI)
	      as_fatal (_("Intel MCU is 32bit only"));
	    return ELF_TARGET_IAMCU_FORMAT;
	  }
	else
	  return format;
      }
#endif
#if defined (OBJ_MACH_O)
    case bfd_target_mach_o_flavour:
      if (flag_code == CODE_64BIT)
	{
	  use_rela_relocations = 1;
	  object_64bit = 1;
	  return "mach-o-x86-64";
	}
      else
	return "mach-o-i386";
#endif
    default:
      abort ();
      return NULL;
    }
}

#endif /* OBJ_MAYBE_ more than one  */

symbolS *
md_undefined_symbol (char *name)
{
  if (name[0] == GLOBAL_OFFSET_TABLE_NAME[0]
      && name[1] == GLOBAL_OFFSET_TABLE_NAME[1]
      && name[2] == GLOBAL_OFFSET_TABLE_NAME[2]
      && strcmp (name, GLOBAL_OFFSET_TABLE_NAME) == 0)
    {
      if (!GOT_symbol)
	{
	  if (symbol_find (name))
	    as_bad (_("GOT already in symbol table"));
	  GOT_symbol = symbol_new (name, undefined_section,
				   (valueT) 0, &zero_address_frag);
	};
      return GOT_symbol;
    }
  return 0;
}

/* Round up a section size to the appropriate boundary.  */

valueT
md_section_align (segT segment ATTRIBUTE_UNUSED, valueT size)
{
#if (defined (OBJ_AOUT) || defined (OBJ_MAYBE_AOUT))
  if (OUTPUT_FLAVOR == bfd_target_aout_flavour)
    {
      /* For a.out, force the section size to be aligned.  If we don't do
	 this, BFD will align it for us, but it will not write out the
	 final bytes of the section.  This may be a bug in BFD, but it is
	 easier to fix it here since that is how the other a.out targets
	 work.  */
      int align;

      align = bfd_get_section_alignment (stdoutput, segment);
      size = ((size + (1 << align) - 1) & (-((valueT) 1 << align)));
    }
#endif

  return size;
}

/* On the i386, PC-relative offsets are relative to the start of the
   next instruction.  That is, the address of the offset, plus its
   size, since the offset is always the last part of the insn.  */

long
md_pcrel_from (fixS *fixP)
{
  return fixP->fx_size + fixP->fx_where + fixP->fx_frag->fr_address;
}

#ifndef I386COFF

static void
s_bss (int ignore ATTRIBUTE_UNUSED)
{
  int temp;

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  if (IS_ELF)
    obj_elf_section_change_hook ();
#endif
  temp = get_absolute_expression ();
  subseg_set (bss_section, (subsegT) temp);
  demand_empty_rest_of_line ();
}

#endif

void
i386_validate_fix (fixS *fixp)
{
  if (fixp->fx_subsy)
    {
      if (fixp->fx_subsy == GOT_symbol)
	{
	  if (fixp->fx_r_type == BFD_RELOC_32_PCREL)
	    {
	      if (!object_64bit)
		abort ();
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
	      if (fixp->fx_tcbit2)
		fixp->fx_r_type = (fixp->fx_tcbit
				   ? BFD_RELOC_X86_64_REX_GOTPCRELX
				   : BFD_RELOC_X86_64_GOTPCRELX);
	      else
#endif
		fixp->fx_r_type = BFD_RELOC_X86_64_GOTPCREL;
	    }
	  else
	    {
	      if (!object_64bit)
		fixp->fx_r_type = BFD_RELOC_386_GOTOFF;
	      else
		fixp->fx_r_type = BFD_RELOC_X86_64_GOTOFF64;
	    }
	  fixp->fx_subsy = 0;
	}
    }
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
  else if (!object_64bit)
    {
      if (fixp->fx_r_type == BFD_RELOC_386_GOT32
	  && fixp->fx_tcbit2)
	fixp->fx_r_type = BFD_RELOC_386_GOT32X;
    }
#endif
}

arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixp)
{
  arelent *rel;
  bfd_reloc_code_real_type code;

  switch (fixp->fx_r_type)
    {
#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
    case BFD_RELOC_SIZE32:
    case BFD_RELOC_SIZE64:
      if (S_IS_DEFINED (fixp->fx_addsy)
	  && !S_IS_EXTERNAL (fixp->fx_addsy))
	{
	  /* Resolve size relocation against local symbol to size of
	     the symbol plus addend.  */
	  valueT value = S_GET_SIZE (fixp->fx_addsy) + fixp->fx_offset;
	  if (fixp->fx_r_type == BFD_RELOC_SIZE32
	      && !fits_in_unsigned_long (value))
	    as_bad_where (fixp->fx_file, fixp->fx_line,
			  _("symbol size computation overflow"));
	  fixp->fx_addsy = NULL;
	  fixp->fx_subsy = NULL;
	  md_apply_fix (fixp, (valueT *) &value, NULL);
	  return NULL;
	}
#endif
      /* Fall through.  */

    case BFD_RELOC_X86_64_PLT32:
    case BFD_RELOC_X86_64_GOT32:
    case BFD_RELOC_X86_64_GOTPCREL:
    case BFD_RELOC_X86_64_GOTPCRELX:
    case BFD_RELOC_X86_64_REX_GOTPCRELX:
    case BFD_RELOC_386_PLT32:
    case BFD_RELOC_386_GOT32:
    case BFD_RELOC_386_GOT32X:
    case BFD_RELOC_386_GOTOFF:
    case BFD_RELOC_386_GOTPC:
    case BFD_RELOC_386_TLS_GD:
    case BFD_RELOC_386_TLS_LDM:
    case BFD_RELOC_386_TLS_LDO_32:
    case BFD_RELOC_386_TLS_IE_32:
    case BFD_RELOC_386_TLS_IE:
    case BFD_RELOC_386_TLS_GOTIE:
    case BFD_RELOC_386_TLS_LE_32:
    case BFD_RELOC_386_TLS_LE:
    case BFD_RELOC_386_TLS_GOTDESC:
    case BFD_RELOC_386_TLS_DESC_CALL:
    case BFD_RELOC_X86_64_TLSGD:
    case BFD_RELOC_X86_64_TLSLD:
    case BFD_RELOC_X86_64_DTPOFF32:
    case BFD_RELOC_X86_64_DTPOFF64:
    case BFD_RELOC_X86_64_GOTTPOFF:
    case BFD_RELOC_X86_64_TPOFF32:
    case BFD_RELOC_X86_64_TPOFF64:
    case BFD_RELOC_X86_64_GOTOFF64:
    case BFD_RELOC_X86_64_GOTPC32:
    case BFD_RELOC_X86_64_GOT64:
    case BFD_RELOC_X86_64_GOTPCREL64:
    case BFD_RELOC_X86_64_GOTPC64:
    case BFD_RELOC_X86_64_GOTPLT64:
    case BFD_RELOC_X86_64_PLTOFF64:
    case BFD_RELOC_X86_64_GOTPC32_TLSDESC:
    case BFD_RELOC_X86_64_TLSDESC_CALL:
    case BFD_RELOC_RVA:
    case BFD_RELOC_VTABLE_ENTRY:
    case BFD_RELOC_VTABLE_INHERIT:
#ifdef TE_PE
    case BFD_RELOC_32_SECREL:
#endif
      code = fixp->fx_r_type;
      break;
    case BFD_RELOC_X86_64_32S:
      if (!fixp->fx_pcrel)
	{
	  /* Don't turn BFD_RELOC_X86_64_32S into BFD_RELOC_32.  */
	  code = fixp->fx_r_type;
	  break;
	}
      /* Fall through.  */
    default:
      if (fixp->fx_pcrel)
	{
	  switch (fixp->fx_size)
	    {
	    default:
	      as_bad_where (fixp->fx_file, fixp->fx_line,
			    _("can not do %d byte pc-relative relocation"),
			    fixp->fx_size);
	      code = BFD_RELOC_32_PCREL;
	      break;
	    case 1: code = BFD_RELOC_8_PCREL;  break;
	    case 2: code = BFD_RELOC_16_PCREL; break;
	    case 4: code = BFD_RELOC_32_PCREL; break;
#ifdef BFD64
	    case 8: code = BFD_RELOC_64_PCREL; break;
#endif
	    }
	}
      else
	{
	  switch (fixp->fx_size)
	    {
	    default:
	      as_bad_where (fixp->fx_file, fixp->fx_line,
			    _("can not do %d byte relocation"),
			    fixp->fx_size);
	      code = BFD_RELOC_32;
	      break;
	    case 1: code = BFD_RELOC_8;  break;
	    case 2: code = BFD_RELOC_16; break;
	    case 4: code = BFD_RELOC_32; break;
#ifdef BFD64
	    case 8: code = BFD_RELOC_64; break;
#endif
	    }
	}
      break;
    }

  if ((code == BFD_RELOC_32
       || code == BFD_RELOC_32_PCREL
       || code == BFD_RELOC_X86_64_32S)
      && GOT_symbol
      && fixp->fx_addsy == GOT_symbol)
    {
      if (!object_64bit)
	code = BFD_RELOC_386_GOTPC;
      else
	code = BFD_RELOC_X86_64_GOTPC32;
    }
  if ((code == BFD_RELOC_64 || code == BFD_RELOC_64_PCREL)
      && GOT_symbol
      && fixp->fx_addsy == GOT_symbol)
    {
      code = BFD_RELOC_X86_64_GOTPC64;
    }

  rel = XNEW (arelent);
  rel->sym_ptr_ptr = XNEW (asymbol *);
  *rel->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);

  rel->address = fixp->fx_frag->fr_address + fixp->fx_where;

  if (!use_rela_relocations)
    {
      /* HACK: Since i386 ELF uses Rel instead of Rela, encode the
	 vtable entry to be used in the relocation's section offset.  */
      if (fixp->fx_r_type == BFD_RELOC_VTABLE_ENTRY)
	rel->address = fixp->fx_offset;
#if defined (OBJ_COFF) && defined (TE_PE)
      else if (fixp->fx_addsy && S_IS_WEAK (fixp->fx_addsy))
	rel->addend = fixp->fx_addnumber - (S_GET_VALUE (fixp->fx_addsy) * 2);
      else
#endif
      rel->addend = 0;
    }
  /* Use the rela in 64bit mode.  */
  else
    {
      if (disallow_64bit_reloc)
	switch (code)
	  {
	  case BFD_RELOC_X86_64_DTPOFF64:
	  case BFD_RELOC_X86_64_TPOFF64:
	  case BFD_RELOC_64_PCREL:
	  case BFD_RELOC_X86_64_GOTOFF64:
	  case BFD_RELOC_X86_64_GOT64:
	  case BFD_RELOC_X86_64_GOTPCREL64:
	  case BFD_RELOC_X86_64_GOTPC64:
	  case BFD_RELOC_X86_64_GOTPLT64:
	  case BFD_RELOC_X86_64_PLTOFF64:
	    as_bad_where (fixp->fx_file, fixp->fx_line,
			  _("cannot represent relocation type %s in x32 mode"),
			  bfd_get_reloc_code_name (code));
	    break;
	  default:
	    break;
	  }

      if (!fixp->fx_pcrel)
	rel->addend = fixp->fx_offset;
      else
	switch (code)
	  {
	  case BFD_RELOC_X86_64_PLT32:
	  case BFD_RELOC_X86_64_GOT32:
	  case BFD_RELOC_X86_64_GOTPCREL:
	  case BFD_RELOC_X86_64_GOTPCRELX:
	  case BFD_RELOC_X86_64_REX_GOTPCRELX:
	  case BFD_RELOC_X86_64_TLSGD:
	  case BFD_RELOC_X86_64_TLSLD:
	  case BFD_RELOC_X86_64_GOTTPOFF:
	  case BFD_RELOC_X86_64_GOTPC32_TLSDESC:
	  case BFD_RELOC_X86_64_TLSDESC_CALL:
	    rel->addend = fixp->fx_offset - fixp->fx_size;
	    break;
	  default:
	    rel->addend = (section->vma
			   - fixp->fx_size
			   + fixp->fx_addnumber
			   + md_pcrel_from (fixp));
	    break;
	  }
    }

  rel->howto = bfd_reloc_type_lookup (stdoutput, code);
  if (rel->howto == NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("cannot represent relocation type %s"),
		    bfd_get_reloc_code_name (code));
      /* Set howto to a garbage value so that we can keep going.  */
      rel->howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_32);
      gas_assert (rel->howto != NULL);
    }

  return rel;
}

#include "tc-i386-intel.c"

void
tc_x86_parse_to_dw2regnum (expressionS *exp)
{
  int saved_naked_reg;
  char saved_register_dot;

  saved_naked_reg = allow_naked_reg;
  allow_naked_reg = 1;
  saved_register_dot = register_chars['.'];
  register_chars['.'] = '.';
  allow_pseudo_reg = 1;
  expression_and_evaluate (exp);
  allow_pseudo_reg = 0;
  register_chars['.'] = saved_register_dot;
  allow_naked_reg = saved_naked_reg;

  if (exp->X_op == O_register && exp->X_add_number >= 0)
    {
      if ((addressT) exp->X_add_number < i386_regtab_size)
	{
	  exp->X_op = O_constant;
	  exp->X_add_number = i386_regtab[exp->X_add_number]
			      .dw2_regnum[flag_code >> 1];
	}
      else
	exp->X_op = O_illegal;
    }
}

void
tc_x86_frame_initial_instructions (void)
{
  static unsigned int sp_regno[2];

  if (!sp_regno[flag_code >> 1])
    {
      char *saved_input = input_line_pointer;
      char sp[][4] = {"esp", "rsp"};
      expressionS exp;

      input_line_pointer = sp[flag_code >> 1];
      tc_x86_parse_to_dw2regnum (&exp);
      gas_assert (exp.X_op == O_constant);
      sp_regno[flag_code >> 1] = exp.X_add_number;
      input_line_pointer = saved_input;
    }

  cfi_add_CFA_def_cfa (sp_regno[flag_code >> 1], -x86_cie_data_alignment);
  cfi_add_CFA_offset (x86_dwarf2_return_column, x86_cie_data_alignment);
}

int
x86_dwarf2_addr_size (void)
{
#if defined (OBJ_MAYBE_ELF) || defined (OBJ_ELF)
  if (x86_elf_abi == X86_64_X32_ABI)
    return 4;
#endif
  return bfd_arch_bits_per_address (stdoutput) / 8;
}

int
i386_elf_section_type (const char *str, size_t len)
{
  if (flag_code == CODE_64BIT
      && len == sizeof ("unwind") - 1
      && strncmp (str, "unwind", 6) == 0)
    return SHT_X86_64_UNWIND;

  return -1;
}

#ifdef TE_SOLARIS
void
i386_solaris_fix_up_eh_frame (segT sec)
{
  if (flag_code == CODE_64BIT)
    elf_section_type (sec) = SHT_X86_64_UNWIND;
}
#endif

#ifdef TE_PE
void
tc_pe_dwarf2_emit_offset (symbolS *symbol, unsigned int size)
{
  expressionS exp;

  exp.X_op = O_secrel;
  exp.X_add_symbol = symbol;
  exp.X_add_number = 0;
  emit_expr (&exp, size);
}
#endif

#if defined (OBJ_ELF) || defined (OBJ_MAYBE_ELF)
/* For ELF on x86-64, add support for SHF_X86_64_LARGE.  */

bfd_vma
x86_64_section_letter (int letter, const char **ptr_msg)
{
  if (flag_code == CODE_64BIT)
    {
      if (letter == 'l')
	return SHF_X86_64_LARGE;

      *ptr_msg = _("bad .section directive: want a,l,w,x,M,S,G,T in string");
    }
  else
    *ptr_msg = _("bad .section directive: want a,w,x,M,S,G,T in string");
  return -1;
}

bfd_vma
x86_64_section_word (char *str, size_t len)
{
  if (len == 5 && flag_code == CODE_64BIT && CONST_STRNEQ (str, "large"))
    return SHF_X86_64_LARGE;

  return -1;
}

static void
handle_large_common (int small ATTRIBUTE_UNUSED)
{
  if (flag_code != CODE_64BIT)
    {
      s_comm_internal (0, elf_common_parse);
      as_warn (_(".largecomm supported only in 64bit mode, producing .comm"));
    }
  else
    {
      static segT lbss_section;
      asection *saved_com_section_ptr = elf_com_section_ptr;
      asection *saved_bss_section = bss_section;

      if (lbss_section == NULL)
	{
	  flagword applicable;
	  segT seg = now_seg;
	  subsegT subseg = now_subseg;

	  /* The .lbss section is for local .largecomm symbols.  */
	  lbss_section = subseg_new (".lbss", 0);
	  applicable = bfd_applicable_section_flags (stdoutput);
	  bfd_set_section_flags (stdoutput, lbss_section,
				 applicable & SEC_ALLOC);
	  seg_info (lbss_section)->bss = 1;

	  subseg_set (seg, subseg);
	}

      elf_com_section_ptr = &_bfd_elf_large_com_section;
      bss_section = lbss_section;

      s_comm_internal (0, elf_common_parse);

      elf_com_section_ptr = saved_com_section_ptr;
      bss_section = saved_bss_section;
    }
}
#endif /* OBJ_ELF || OBJ_MAYBE_ELF */
