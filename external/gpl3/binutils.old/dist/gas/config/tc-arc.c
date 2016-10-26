/* tc-arc.c -- Assembler for the ARC
   Copyright (C) 1994-2015 Free Software Foundation, Inc.

   Contributor: Claudiu Zissulescu <claziss@synopsys.com>

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
#include "subsegs.h"
#include "struc-symbol.h"
#include "dwarf2dbg.h"
#include "safe-ctype.h"

#include "opcode/arc.h"
#include "elf/arc.h"

/* Defines section.  */

#define MAX_FLAG_NAME_LENGHT 3
#define MAX_INSN_FIXUPS      2
#define MAX_CONSTR_STR       20

#ifdef DEBUG
# define pr_debug(fmt, args...) fprintf (stderr, fmt, ##args)
#else
# define pr_debug(fmt, args...)
#endif

#define MAJOR_OPCODE(x)  (((x) & 0xF8000000) >> 27)
#define SUB_OPCODE(x)	 (((x) & 0x003F0000) >> 16)
#define LP_INSN(x)	 ((MAJOR_OPCODE (x) == 0x4) &&	\
			  (SUB_OPCODE (x) == 0x28))

/* Equal to MAX_PRECISION in atof-ieee.c.  */
#define MAX_LITTLENUMS 6

/* Macros section.  */

#define regno(x)		((x) & 0x3F)
#define is_ir_num(x)		(((x) & ~0x3F) == 0)
#define is_code_density_p(op)   (((op)->subclass == CD1 || (op)->subclass == CD2))
#define is_br_jmp_insn_p(op)    (((op)->class == BRANCH || (op)->class == JUMP))
#define is_kernel_insn_p(op)    (((op)->class == KERNEL))

/* Generic assembler global variables which must be defined by all
   targets.  */

/* Characters which always start a comment.  */
const char comment_chars[] = "#;";

/* Characters which start a comment at the beginning of a line.  */
const char line_comment_chars[] = "#";

/* Characters which may be used to separate multiple commands on a
   single line.  */
const char line_separator_chars[] = "`";

/* Characters which are used to indicate an exponent in a floating
   point number.  */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant
   As in 0f12.456 or 0d1.2345e12.  */
const char FLT_CHARS[] = "rRsSfFdD";

/* Byte order.  */
extern int target_big_endian;
const char *arc_target_format = DEFAULT_TARGET_FORMAT;
static int byte_order = DEFAULT_BYTE_ORDER;

extern int arc_get_mach (char *);

/* Forward declaration.  */
static void arc_lcomm (int);
static void arc_option (int);
static void arc_extra_reloc (int);

const pseudo_typeS md_pseudo_table[] =
{
  /* Make sure that .word is 32 bits.  */
  { "word", cons, 4 },

  { "align",   s_align_bytes, 0 }, /* Defaulting is invalid (0).  */
  { "lcomm",   arc_lcomm, 0 },
  { "lcommon", arc_lcomm, 0 },
  { "cpu",     arc_option, 0 },

  { "tls_gd_ld",   arc_extra_reloc, BFD_RELOC_ARC_TLS_GD_LD },
  { "tls_gd_call", arc_extra_reloc, BFD_RELOC_ARC_TLS_GD_CALL },

  { NULL, NULL, 0 }
};

const char *md_shortopts = "";

enum options
{
  OPTION_EB = OPTION_MD_BASE,
  OPTION_EL,

  OPTION_ARC600,
  OPTION_ARC601,
  OPTION_ARC700,
  OPTION_ARCEM,
  OPTION_ARCHS,

  OPTION_MCPU,
  OPTION_CD,

  /* The following options are deprecated and provided here only for
     compatibility reasons.  */
  OPTION_USER_MODE,
  OPTION_LD_EXT_MASK,
  OPTION_SWAP,
  OPTION_NORM,
  OPTION_BARREL_SHIFT,
  OPTION_MIN_MAX,
  OPTION_NO_MPY,
  OPTION_EA,
  OPTION_MUL64,
  OPTION_SIMD,
  OPTION_SPFP,
  OPTION_DPFP,
  OPTION_XMAC_D16,
  OPTION_XMAC_24,
  OPTION_DSP_PACKA,
  OPTION_CRC,
  OPTION_DVBF,
  OPTION_TELEPHONY,
  OPTION_XYMEMORY,
  OPTION_LOCK,
  OPTION_SWAPE,
  OPTION_RTSC,
  OPTION_FPUDA
};

struct option md_longopts[] =
{
  { "EB",		no_argument,	   NULL, OPTION_EB },
  { "EL",		no_argument,	   NULL, OPTION_EL },
  { "mcpu",		required_argument, NULL, OPTION_MCPU },
  { "mA6",		no_argument,	   NULL, OPTION_ARC600 },
  { "mARC600",	no_argument,	   NULL, OPTION_ARC600 },
  { "mARC601",	no_argument,	   NULL, OPTION_ARC601 },
  { "mARC700",	no_argument,	   NULL, OPTION_ARC700 },
  { "mA7",		no_argument,	   NULL, OPTION_ARC700 },
  { "mEM",		no_argument,	   NULL, OPTION_ARCEM },
  { "mHS",		no_argument,	   NULL, OPTION_ARCHS },
  { "mcode-density",	no_argument,	   NULL, OPTION_CD },

  /* The following options are deprecated and provided here only for
     compatibility reasons.  */
  { "mav2em", no_argument, NULL, OPTION_ARCEM },
  { "mav2hs", no_argument, NULL, OPTION_ARCHS },
  { "muser-mode-only", no_argument, NULL, OPTION_USER_MODE },
  { "mld-extension-reg-mask", required_argument, NULL, OPTION_LD_EXT_MASK },
  { "mswap", no_argument, NULL, OPTION_SWAP },
  { "mnorm", no_argument, NULL, OPTION_NORM },
  { "mbarrel-shifter", no_argument, NULL, OPTION_BARREL_SHIFT },
  { "mbarrel_shifter", no_argument, NULL, OPTION_BARREL_SHIFT },
  { "mmin-max", no_argument, NULL, OPTION_MIN_MAX },
  { "mmin_max", no_argument, NULL, OPTION_MIN_MAX },
  { "mno-mpy", no_argument, NULL, OPTION_NO_MPY },
  { "mea", no_argument, NULL, OPTION_EA },
  { "mEA", no_argument, NULL, OPTION_EA },
  { "mmul64", no_argument, NULL, OPTION_MUL64 },
  { "msimd", no_argument, NULL, OPTION_SIMD},
  { "mspfp", no_argument, NULL, OPTION_SPFP},
  { "mspfp-compact", no_argument, NULL, OPTION_SPFP},
  { "mspfp_compact", no_argument, NULL, OPTION_SPFP},
  { "mspfp-fast", no_argument, NULL, OPTION_SPFP},
  { "mspfp_fast", no_argument, NULL, OPTION_SPFP},
  { "mdpfp", no_argument, NULL, OPTION_DPFP},
  { "mdpfp-compact", no_argument, NULL, OPTION_DPFP},
  { "mdpfp_compact", no_argument, NULL, OPTION_DPFP},
  { "mdpfp-fast", no_argument, NULL, OPTION_DPFP},
  { "mdpfp_fast", no_argument, NULL, OPTION_DPFP},
  { "mmac-d16", no_argument, NULL, OPTION_XMAC_D16},
  { "mmac_d16", no_argument, NULL, OPTION_XMAC_D16},
  { "mmac-24", no_argument, NULL, OPTION_XMAC_24},
  { "mmac_24", no_argument, NULL, OPTION_XMAC_24},
  { "mdsp-packa", no_argument, NULL, OPTION_DSP_PACKA},
  { "mdsp_packa", no_argument, NULL, OPTION_DSP_PACKA},
  { "mcrc", no_argument, NULL, OPTION_CRC},
  { "mdvbf", no_argument, NULL, OPTION_DVBF},
  { "mtelephony", no_argument, NULL, OPTION_TELEPHONY},
  { "mxy", no_argument, NULL, OPTION_XYMEMORY},
  { "mlock", no_argument, NULL, OPTION_LOCK},
  { "mswape", no_argument, NULL, OPTION_SWAPE},
  { "mrtsc", no_argument, NULL, OPTION_RTSC},
  { "mfpuda", no_argument, NULL, OPTION_FPUDA},

  { NULL,		no_argument, NULL, 0 }
};

size_t md_longopts_size = sizeof (md_longopts);

/* Local data and data types.  */

/* Used since new relocation types are introduced in this
   file (DUMMY_RELOC_LITUSE_*).  */
typedef int extended_bfd_reloc_code_real_type;

struct arc_fixup
{
  expressionS exp;

  extended_bfd_reloc_code_real_type reloc;

  /* index into arc_operands.  */
  unsigned int opindex;

  /* PC-relative, used by internals fixups.  */
  unsigned char pcrel;

  /* TRUE if this fixup is for LIMM operand.  */
  bfd_boolean islong;
};

struct arc_insn
{
  unsigned int insn;
  int nfixups;
  struct arc_fixup fixups[MAX_INSN_FIXUPS];
  long limm;
  bfd_boolean short_insn; /* Boolean value: TRUE if current insn is
			     short.  */
  bfd_boolean has_limm;   /* Boolean value: TRUE if limm field is
			     valid.  */
};

/* Structure to hold any last two instructions.  */
static struct arc_last_insn
{
  /* Saved instruction opcode.  */
  const struct arc_opcode *opcode;

  /* Boolean value: TRUE if current insn is short.  */
  bfd_boolean has_limm;

  /* Boolean value: TRUE if current insn has delay slot.  */
  bfd_boolean has_delay_slot;
} arc_last_insns[2];

/* The cpu for which we are generating code.  */
static unsigned arc_target = ARC_OPCODE_BASE;
static const char *arc_target_name = "<all>";
static unsigned arc_features = 0x00;

/* The default architecture.  */
static int arc_mach_type = bfd_mach_arc_arcv2;

/* Non-zero if the cpu type has been explicitly specified.  */
static int mach_type_specified_p = 0;

/* The hash table of instruction opcodes.  */
static struct hash_control *arc_opcode_hash;

/* The hash table of register symbols.  */
static struct hash_control *arc_reg_hash;

/* A table of CPU names and opcode sets.  */
static const struct cpu_type
{
  const char *name;
  unsigned flags;
  int mach;
  unsigned eflags;
  unsigned features;
}
  cpu_types[] =
{
  { "arc600", ARC_OPCODE_ARC600,  bfd_mach_arc_arc600,
    E_ARC_MACH_ARC600,  0x00},
  { "arc700", ARC_OPCODE_ARC700,  bfd_mach_arc_arc700,
    E_ARC_MACH_ARC700,  0x00},
  { "arcem",  ARC_OPCODE_ARCv2EM, bfd_mach_arc_arcv2,
    EF_ARC_CPU_ARCV2EM, 0x00},
  { "archs",  ARC_OPCODE_ARCv2HS, bfd_mach_arc_arcv2,
    EF_ARC_CPU_ARCV2HS, ARC_CD},
  { "all",    ARC_OPCODE_BASE,    bfd_mach_arc_arcv2,
    0x00, 0x00 },
  { 0, 0, 0, 0, 0 }
};

struct arc_flags
{
  /* Name of the parsed flag.  */
  char name[MAX_FLAG_NAME_LENGHT+1];

  /* The code of the parsed flag.  Valid when is not zero.  */
  unsigned char code;
};

/* Used by the arc_reloc_op table.  Order is important.  */
#define O_gotoff  O_md1     /* @gotoff relocation.  */
#define O_gotpc   O_md2     /* @gotpc relocation.  */
#define O_plt     O_md3     /* @plt relocation.  */
#define O_sda     O_md4     /* @sda relocation.  */
#define O_pcl     O_md5     /* @pcl relocation.  */
#define O_tlsgd   O_md6     /* @tlsgd relocation.  */
#define O_tlsie   O_md7     /* @tlsie relocation.  */
#define O_tpoff9  O_md8     /* @tpoff9 relocation.  */
#define O_tpoff   O_md9     /* @tpoff relocation.  */
#define O_dtpoff9 O_md10    /* @dtpoff9 relocation.  */
#define O_dtpoff  O_md11    /* @dtpoff relocation.  */
#define O_last    O_dtpoff

/* Used to define a bracket as operand in tokens.  */
#define O_bracket O_md32

/* Dummy relocation, to be sorted out.  */
#define DUMMY_RELOC_ARC_ENTRY     (BFD_RELOC_UNUSED + 1)

#define USER_RELOC_P(R) ((R) >= O_gotoff && (R) <= O_last)

/* A table to map the spelling of a relocation operand into an appropriate
   bfd_reloc_code_real_type type.  The table is assumed to be ordered such
   that op-O_literal indexes into it.  */
#define ARC_RELOC_TABLE(op)				\
  (&arc_reloc_op[ ((!USER_RELOC_P (op))			\
		   ? (abort (), 0)			\
		   : (int) (op) - (int) O_gotoff) ])

#define DEF(NAME, RELOC, REQ)				\
  { #NAME, sizeof (#NAME)-1, O_##NAME, RELOC, REQ}

static const struct arc_reloc_op_tag
{
  /* String to lookup.  */
  const char *name;
  /* Size of the string.  */
  size_t length;
  /* Which operator to use.  */
  operatorT op;
  extended_bfd_reloc_code_real_type reloc;
  /* Allows complex relocation expression like identifier@reloc +
     const.  */
  unsigned int complex_expr : 1;
}
  arc_reloc_op[] =
{
  DEF (gotoff,  BFD_RELOC_ARC_GOTOFF,		1),
  DEF (gotpc,   BFD_RELOC_ARC_GOTPC32,		0),
  DEF (plt,	BFD_RELOC_ARC_PLT32,		0),
  DEF (sda,	DUMMY_RELOC_ARC_ENTRY,		1),
  DEF (pcl,	BFD_RELOC_ARC_PC32,		1),
  DEF (tlsgd,   BFD_RELOC_ARC_TLS_GD_GOT,	0),
  DEF (tlsie,   BFD_RELOC_ARC_TLS_IE_GOT,	0),
  DEF (tpoff9,  BFD_RELOC_ARC_TLS_LE_S9,	0),
  DEF (tpoff,   BFD_RELOC_ARC_TLS_LE_32,	0),
  DEF (dtpoff9, BFD_RELOC_ARC_TLS_DTPOFF_S9,	0),
  DEF (dtpoff,  BFD_RELOC_ARC_TLS_DTPOFF,	0),
};

static const int arc_num_reloc_op
= sizeof (arc_reloc_op) / sizeof (*arc_reloc_op);

/* Flags to set in the elf header.  */
static flagword arc_eflag = 0x00;

/* Pre-defined "_GLOBAL_OFFSET_TABLE_".  */
symbolS * GOT_symbol = 0;

/* Set to TRUE when we assemble instructions.  */
static bfd_boolean assembling_insn = FALSE;

/* Functions declaration.  */

static void assemble_tokens (const char *, expressionS *, int,
			     struct arc_flags *, int);
static const struct arc_opcode *find_opcode_match (const struct arc_opcode *,
						   expressionS *, int *,
						   struct arc_flags *,
						   int, int *);
static void assemble_insn (const struct arc_opcode *, const expressionS *,
			   int, const struct arc_flags *, int,
			   struct arc_insn *);
static void emit_insn (struct arc_insn *);
static unsigned insert_operand (unsigned, const struct arc_operand *,
				offsetT, char *, unsigned);
static const struct arc_opcode *find_special_case_flag (const char *,
							int *,
							struct arc_flags *);
static const struct arc_opcode *find_special_case (const char *,
						   int *,
						   struct arc_flags *,
						   expressionS *, int *);
static const struct arc_opcode *find_special_case_pseudo (const char *,
							  int *,
							  expressionS *,
							  int *,
							  struct arc_flags *);

/* Functions implementation.  */

/* Like md_number_to_chars but used for limms.  The 4-byte limm value,
   is encoded as 'middle-endian' for a little-endian target.  FIXME!
   this function is used for regular 4 byte instructions as well.  */

static void
md_number_to_chars_midend (char *buf, valueT val, int n)
{
  if (n == 4)
    {
      md_number_to_chars (buf,     (val & 0xffff0000) >> 16, 2);
      md_number_to_chars (buf + 2, (val & 0xffff), 2);
    }
  else
    {
      md_number_to_chars (buf, val, n);
    }
}

/* Here ends all the ARCompact extension instruction assembling
   stuff.  */

static void
arc_extra_reloc (int r_type)
{
  char *sym_name, c;
  symbolS *sym, *lab = NULL;

  if (*input_line_pointer == '@')
    input_line_pointer++;
  c = get_symbol_name (&sym_name);
  sym = symbol_find_or_make (sym_name);
  restore_line_pointer (c);
  if (c == ',' && r_type == BFD_RELOC_ARC_TLS_GD_LD)
    {
      ++input_line_pointer;
      char *lab_name;
      c = get_symbol_name (&lab_name);
      lab = symbol_find_or_make (lab_name);
      restore_line_pointer (c);
    }
  fixS *fixP
    = fix_new (frag_now,	/* Which frag?  */
	       frag_now_fix (),	/* Where in that frag?  */
	       2,		/* size: 1, 2, or 4 usually.  */
	       sym,		/* X_add_symbol.  */
	       0,		/* X_add_number.  */
	       FALSE,		/* TRUE if PC-relative relocation.  */
	       r_type		/* Relocation type.  */);
  fixP->fx_subsy = lab;
}

static symbolS *
arc_lcomm_internal (int ignore ATTRIBUTE_UNUSED,
		    symbolS *symbolP, addressT size)
{
  addressT align = 0;
  SKIP_WHITESPACE ();

  if (*input_line_pointer == ',')
    {
      align = parse_align (1);

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
  S_CLEAR_EXTERNAL (symbolP);

  return symbolP;
}

static void
arc_lcomm (int ignore)
{
  symbolS *symbolP = s_comm_internal (ignore, arc_lcomm_internal);

  if (symbolP)
    symbol_get_bfdsym (symbolP)->flags |= BSF_OBJECT;
}

/* Select the cpu we're assembling for.  */

static void
arc_option (int ignore ATTRIBUTE_UNUSED)
{
  int mach = -1;
  char c;
  char *cpu;

  c = get_symbol_name (&cpu);
  mach = arc_get_mach (cpu);
  restore_line_pointer (c);

  if (mach == -1)
    goto bad_cpu;

  if (!mach_type_specified_p)
    {
      arc_mach_type = mach;
      if (!bfd_set_arch_mach (stdoutput, bfd_arch_arc, mach))
	as_fatal ("could not set architecture and machine");

      mach_type_specified_p = 1;
    }
  else
    if (arc_mach_type != mach)
      as_warn ("Command-line value overrides \".cpu\" directive");

  demand_empty_rest_of_line ();

  return;

 bad_cpu:
  as_bad ("invalid identifier for \".cpu\"");
  ignore_rest_of_line ();
}

/* Smartly print an expression.  */

static void
debug_exp (expressionS *t)
{
  const char *name ATTRIBUTE_UNUSED;
  const char *namemd ATTRIBUTE_UNUSED;

  pr_debug ("debug_exp: ");

  switch (t->X_op)
    {
    default:			name = "unknown";		break;
    case O_illegal:		name = "O_illegal";		break;
    case O_absent:		name = "O_absent";		break;
    case O_constant:		name = "O_constant";		break;
    case O_symbol:		name = "O_symbol";		break;
    case O_symbol_rva:		name = "O_symbol_rva";		break;
    case O_register:		name = "O_register";		break;
    case O_big:			name = "O_big";			break;
    case O_uminus:		name = "O_uminus";		break;
    case O_bit_not:		name = "O_bit_not";		break;
    case O_logical_not:		name = "O_logical_not";		break;
    case O_multiply:		name = "O_multiply";		break;
    case O_divide:		name = "O_divide";		break;
    case O_modulus:		name = "O_modulus";		break;
    case O_left_shift:		name = "O_left_shift";		break;
    case O_right_shift:		name = "O_right_shift";		break;
    case O_bit_inclusive_or:	name = "O_bit_inclusive_or";	break;
    case O_bit_or_not:		name = "O_bit_or_not";		break;
    case O_bit_exclusive_or:	name = "O_bit_exclusive_or";	break;
    case O_bit_and:		name = "O_bit_and";		break;
    case O_add:			name = "O_add";			break;
    case O_subtract:		name = "O_subtract";		break;
    case O_eq:			name = "O_eq";			break;
    case O_ne:			name = "O_ne";			break;
    case O_lt:			name = "O_lt";			break;
    case O_le:			name = "O_le";			break;
    case O_ge:			name = "O_ge";			break;
    case O_gt:			name = "O_gt";			break;
    case O_logical_and:		name = "O_logical_and";		break;
    case O_logical_or:		name = "O_logical_or";		break;
    case O_index:		name = "O_index";		break;
    case O_bracket:		name = "O_bracket";		break;
    }

  switch (t->X_md)
    {
    default:			namemd = "unknown";		break;
    case O_gotoff:		namemd = "O_gotoff";		break;
    case O_gotpc:		namemd = "O_gotpc";		break;
    case O_plt:			namemd = "O_plt";		break;
    case O_sda:			namemd = "O_sda";		break;
    case O_pcl:			namemd = "O_pcl";		break;
    case O_tlsgd:		namemd = "O_tlsgd";		break;
    case O_tlsie:		namemd = "O_tlsie";		break;
    case O_tpoff9:		namemd = "O_tpoff9";		break;
    case O_tpoff:		namemd = "O_tpoff";		break;
    case O_dtpoff9:		namemd = "O_dtpoff9";		break;
    case O_dtpoff:		namemd = "O_dtpoff";		break;
    }

  pr_debug ("%s (%s, %s, %d, %s)", name,
	    (t->X_add_symbol) ? S_GET_NAME (t->X_add_symbol) : "--",
	    (t->X_op_symbol) ? S_GET_NAME (t->X_op_symbol) : "--",
	    (int) t->X_add_number,
	    (t->X_md) ? namemd : "--");
  pr_debug ("\n");
  fflush (stderr);
}

/* Parse the arguments to an opcode.  */

static int
tokenize_arguments (char *str,
		    expressionS *tok,
		    int ntok)
{
  char *old_input_line_pointer;
  bfd_boolean saw_comma = FALSE;
  bfd_boolean saw_arg = FALSE;
  int brk_lvl = 0;
  int num_args = 0;
  int i;
  size_t len;
  const struct arc_reloc_op_tag *r;
  expressionS tmpE;
  char *reloc_name, c;

  memset (tok, 0, sizeof (*tok) * ntok);

  /* Save and restore input_line_pointer around this function.  */
  old_input_line_pointer = input_line_pointer;
  input_line_pointer = str;

  while (*input_line_pointer)
    {
      SKIP_WHITESPACE ();
      switch (*input_line_pointer)
	{
	case '\0':
	  goto fini;

	case ',':
	  input_line_pointer++;
	  if (saw_comma || !saw_arg)
	    goto err;
	  saw_comma = TRUE;
	  break;

	case '}':
	case ']':
	  ++input_line_pointer;
	  --brk_lvl;
	  if (!saw_arg)
	    goto err;
	  tok->X_op = O_bracket;
	  ++tok;
	  ++num_args;
	  break;

	case '{':
	case '[':
	  input_line_pointer++;
	  if (brk_lvl)
	    goto err;
	  ++brk_lvl;
	  tok->X_op = O_bracket;
	  ++tok;
	  ++num_args;
	  break;

	case '@':
	  /* We have labels, function names and relocations, all
	     starting with @ symbol.  Sort them out.  */
	  if (saw_arg && !saw_comma)
	    goto err;

	  /* Parse @label.  */
	  tok->X_op = O_symbol;
	  tok->X_md = O_absent;
	  expression (tok);
	  if (*input_line_pointer != '@')
	    goto normalsymbol; /* This is not a relocation.  */

	relocationsym:

	  /* A relocation opernad has the following form
	     @identifier@relocation_type.  The identifier is already
	     in tok!  */
	  if (tok->X_op != O_symbol)
	    {
	      as_bad (_("No valid label relocation operand"));
	      goto err;
	    }

	  /* Parse @relocation_type.  */
	  input_line_pointer++;
	  c = get_symbol_name (&reloc_name);
	  len = input_line_pointer - reloc_name;
	  if (len == 0)
	    {
	      as_bad (_("No relocation operand"));
	      goto err;
	    }

	  /* Go through known relocation and try to find a match.  */
	  r = &arc_reloc_op[0];
	  for (i = arc_num_reloc_op - 1; i >= 0; i--, r++)
	    if (len == r->length
		&& memcmp (reloc_name, r->name, len) == 0)
	      break;
	  if (i < 0)
	    {
	      as_bad (_("Unknown relocation operand: @%s"), reloc_name);
	      goto err;
	    }

	  *input_line_pointer = c;
	  SKIP_WHITESPACE_AFTER_NAME ();
	  /* Extra check for TLS: base.  */
	  if (*input_line_pointer == '@')
	    {
	      symbolS *base;
	      if (tok->X_op_symbol != NULL
		  || tok->X_op != O_symbol)
		{
		  as_bad (_("Unable to parse TLS base: %s"),
			  input_line_pointer);
		  goto err;
		}
	      input_line_pointer++;
	      char *sym_name;
	      c = get_symbol_name (&sym_name);
	      base = symbol_find_or_make (sym_name);
	      tok->X_op = O_subtract;
	      tok->X_op_symbol = base;
	      restore_line_pointer (c);
	      tmpE.X_add_number = 0;
	    }
	  else if ((*input_line_pointer != '+')
		   && (*input_line_pointer != '-'))
	    {
	      tmpE.X_add_number = 0;
	    }
	  else
	    {
	      /* Parse the constant of a complex relocation expression
		 like @identifier@reloc +/- const.  */
	      if (! r->complex_expr)
		{
		  as_bad (_("@%s is not a complex relocation."), r->name);
		  goto err;
		}
	      expression (&tmpE);
	      if (tmpE.X_op != O_constant)
		{
		  as_bad (_("Bad expression: @%s + %s."),
			  r->name, input_line_pointer);
		  goto err;
		}
	    }

	  tok->X_md = r->op;
	  tok->X_add_number = tmpE.X_add_number;

	  debug_exp (tok);

	  saw_comma = FALSE;
	  saw_arg = TRUE;
	  tok++;
	  num_args++;
	  break;

	case '%':
	  /* Can be a register.  */
	  ++input_line_pointer;
	  /* Fall through.  */
	default:

	  if (saw_arg && !saw_comma)
	    goto err;

	  tok->X_op = O_absent;
	  tok->X_md = O_absent;
	  expression (tok);

	  /* Legacy: There are cases when we have
	     identifier@relocation_type, if it is the case parse the
	     relocation type as well.  */
	  if (*input_line_pointer == '@')
	    goto relocationsym;

	normalsymbol:
	  debug_exp (tok);

	  if (tok->X_op == O_illegal || tok->X_op == O_absent)
	    goto err;

	  saw_comma = FALSE;
	  saw_arg = TRUE;
	  tok++;
	  num_args++;
	  break;
	}
    }

 fini:
  if (saw_comma || brk_lvl)
    goto err;
  input_line_pointer = old_input_line_pointer;

  return num_args;

 err:
  if (brk_lvl)
    as_bad (_("Brackets in operand field incorrect"));
  else if (saw_comma)
    as_bad (_("extra comma"));
  else if (!saw_arg)
    as_bad (_("missing argument"));
  else
    as_bad (_("missing comma or colon"));
  input_line_pointer = old_input_line_pointer;
  return -1;
}

/* Parse the flags to a structure.  */

static int
tokenize_flags (const char *str,
		struct arc_flags flags[],
		int nflg)
{
  char *old_input_line_pointer;
  bfd_boolean saw_flg = FALSE;
  bfd_boolean saw_dot = FALSE;
  int num_flags  = 0;
  size_t flgnamelen;

  memset (flags, 0, sizeof (*flags) * nflg);

  /* Save and restore input_line_pointer around this function.  */
  old_input_line_pointer = input_line_pointer;
  input_line_pointer = (char *) str;

  while (*input_line_pointer)
    {
      switch (*input_line_pointer)
	{
	case ' ':
	case '\0':
	  goto fini;

	case '.':
	  input_line_pointer++;
	  if (saw_dot)
	    goto err;
	  saw_dot = TRUE;
	  saw_flg = FALSE;
	  break;

	default:
	  if (saw_flg && !saw_dot)
	    goto err;

	  if (num_flags >= nflg)
	    goto err;

	  flgnamelen = strspn (input_line_pointer, "abcdefghilmnopqrstvwxz");
	  if (flgnamelen > MAX_FLAG_NAME_LENGHT)
	    goto err;

	  memcpy (flags->name, input_line_pointer, flgnamelen);

	  input_line_pointer += flgnamelen;
	  flags++;
	  saw_dot = FALSE;
	  saw_flg = TRUE;
	  num_flags++;
	  break;
	}
    }

 fini:
  input_line_pointer = old_input_line_pointer;
  return num_flags;

 err:
  if (saw_dot)
    as_bad (_("extra dot"));
  else if (!saw_flg)
    as_bad (_("unrecognized flag"));
  else
    as_bad (_("failed to parse flags"));
  input_line_pointer = old_input_line_pointer;
  return -1;
}

/* The public interface to the instruction assembler.  */

void
md_assemble (char *str)
{
  char *opname;
  expressionS tok[MAX_INSN_ARGS];
  int ntok, nflg;
  size_t opnamelen;
  struct arc_flags flags[MAX_INSN_FLGS];

  /* Split off the opcode.  */
  opnamelen = strspn (str, "abcdefghijklmnopqrstuvwxyz_0123468");
  opname = xmalloc (opnamelen + 1);
  memcpy (opname, str, opnamelen);
  opname[opnamelen] = '\0';

  /* Signalize we are assmbling the instructions.  */
  assembling_insn = TRUE;

  /* Tokenize the flags.  */
  if ((nflg = tokenize_flags (str + opnamelen, flags, MAX_INSN_FLGS)) == -1)
    {
      as_bad (_("syntax error"));
      return;
    }

  /* Scan up to the end of the mnemonic which must end in space or end
     of string.  */
  str += opnamelen;
  for (; *str != '\0'; str++)
    if (*str == ' ')
      break;

  /* Tokenize the rest of the line.  */
  if ((ntok = tokenize_arguments (str, tok, MAX_INSN_ARGS)) < 0)
    {
      as_bad (_("syntax error"));
      return;
    }

  /* Finish it off.  */
  assemble_tokens (opname, tok, ntok, flags, nflg);
  assembling_insn = FALSE;
}

/* Callback to insert a register into the hash table.  */

static void
declare_register (char *name, int number)
{
  const char *err;
  symbolS *regS = symbol_create (name, reg_section,
				 number, &zero_address_frag);

  err = hash_insert (arc_reg_hash, S_GET_NAME (regS), (void *) regS);
  if (err)
    as_fatal ("Inserting \"%s\" into register table failed: %s",
	      name, err);
}

/* Construct symbols for each of the general registers.  */

static void
declare_register_set (void)
{
  int i;
  for (i = 0; i < 32; ++i)
    {
      char name[7];

      sprintf (name, "r%d", i);
      declare_register (name, i);
      if ((i & 0x01) == 0)
	{
	  sprintf (name, "r%dr%d", i, i+1);
	  declare_register (name, i);
	}
    }
}

/* Port-specific assembler initialization.  This function is called
   once, at assembler startup time.  */

void
md_begin (void)
{
  unsigned int i;

  /* The endianness can be chosen "at the factory".  */
  target_big_endian = byte_order == BIG_ENDIAN;

  if (!bfd_set_arch_mach (stdoutput, bfd_arch_arc, arc_mach_type))
    as_warn (_("could not set architecture and machine"));

  /* Set elf header flags.  */
  bfd_set_private_flags (stdoutput, arc_eflag);

  /* Set up a hash table for the instructions.  */
  arc_opcode_hash = hash_new ();
  if (arc_opcode_hash == NULL)
    as_fatal (_("Virtual memory exhausted"));

  /* Initialize the hash table with the insns.  */
  for (i = 0; i < arc_num_opcodes;)
    {
      const char *name, *retval;

      name = arc_opcodes[i].name;
      retval = hash_insert (arc_opcode_hash, name, (void *) &arc_opcodes[i]);
      if (retval)
	as_fatal (_("internal error: can't hash opcode '%s': %s"),
		  name, retval);

      while (++i < arc_num_opcodes
	     && (arc_opcodes[i].name == name
		 || !strcmp (arc_opcodes[i].name, name)))
	continue;
    }

  /* Register declaration.  */
  arc_reg_hash = hash_new ();
  if (arc_reg_hash == NULL)
    as_fatal (_("Virtual memory exhausted"));

  declare_register_set ();
  declare_register ("gp", 26);
  declare_register ("fp", 27);
  declare_register ("sp", 28);
  declare_register ("ilink", 29);
  declare_register ("ilink1", 29);
  declare_register ("ilink2", 30);
  declare_register ("blink", 31);

  declare_register ("mlo", 57);
  declare_register ("mmid", 58);
  declare_register ("mhi", 59);

  declare_register ("acc1", 56);
  declare_register ("acc2", 57);

  declare_register ("lp_count", 60);
  declare_register ("pcl", 63);

  /* Initialize the last instructions.  */
  memset (&arc_last_insns[0], 0, sizeof (arc_last_insns));
}

/* Write a value out to the object file, using the appropriate
   endianness.  */

void
md_number_to_chars (char *buf,
		    valueT val,
		    int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

/* Round up a section size to the appropriate boundary.  */

valueT
md_section_align (segT segment,
		  valueT size)
{
  int align = bfd_get_section_alignment (stdoutput, segment);

  return ((size + (1 << align) - 1) & -(1 << align));
}

/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */

long
md_pcrel_from_section (fixS *fixP,
		       segT sec)
{
  offsetT base = fixP->fx_where + fixP->fx_frag->fr_address;

  pr_debug ("pcrel_from_section, fx_offset = %d\n", (int) fixP->fx_offset);

  if (fixP->fx_addsy != (symbolS *) NULL
      && (!S_IS_DEFINED (fixP->fx_addsy)
	  || S_GET_SEGMENT (fixP->fx_addsy) != sec))
    {
      pr_debug ("Unknown pcrel symbol: %s\n", S_GET_NAME (fixP->fx_addsy));

      /* The symbol is undefined (or is defined but not in this section).
	 Let the linker figure it out.  */
      return 0;
    }

  if ((int) fixP->fx_r_type < 0)
    {
      /* These are the "internal" relocations.  Align them to
	 32 bit boundary (PCL), for the moment.  */
      base &= ~3;
    }
  else
    {
      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_ARC_PC32:
	  /* The hardware calculates relative to the start of the
	     insn, but this relocation is relative to location of the
	     LIMM, compensate.  TIP: the base always needs to be
	     substracted by 4 as we do not support this type of PCrel
	     relocation for short instructions.  */
	  base -= fixP->fx_where - fixP->fx_dot_value;
	  gas_assert ((fixP->fx_where - fixP->fx_dot_value) == 4);
	  /* Fall through.  */
	case BFD_RELOC_ARC_PLT32:
	case BFD_RELOC_ARC_S25H_PCREL_PLT:
	case BFD_RELOC_ARC_S21H_PCREL_PLT:
	case BFD_RELOC_ARC_S25W_PCREL_PLT:
	case BFD_RELOC_ARC_S21W_PCREL_PLT:

	case BFD_RELOC_ARC_S21H_PCREL:
	case BFD_RELOC_ARC_S25H_PCREL:
	case BFD_RELOC_ARC_S13_PCREL:
	case BFD_RELOC_ARC_S21W_PCREL:
	case BFD_RELOC_ARC_S25W_PCREL:
	  base &= ~3;
	  break;
	default:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("unhandled reloc %s in md_pcrel_from_section"),
		  bfd_get_reloc_code_name (fixP->fx_r_type));
	  break;
	}
    }

  pr_debug ("pcrel from %x + %lx = %x, symbol: %s (%x)\n",
	    fixP->fx_frag->fr_address, fixP->fx_where, base,
	    fixP->fx_addsy ? S_GET_NAME (fixP->fx_addsy) : "(null)",
	    fixP->fx_addsy ? S_GET_VALUE (fixP->fx_addsy) : 0);

  return base;
}

/* Given a BFD relocation find the coresponding operand.  */

static const struct arc_operand *
find_operand_for_reloc (extended_bfd_reloc_code_real_type reloc)
{
  unsigned i;

  for (i = 0; i < arc_num_operands; i++)
    if (arc_operands[i].default_reloc == reloc)
      return  &arc_operands[i];
  return NULL;
}

/* Apply a fixup to the object code.  At this point all symbol values
   should be fully resolved, and we attempt to completely resolve the
   reloc.  If we can not do that, we determine the correct reloc code
   and put it back in the fixup.  To indicate that a fixup has been
   eliminated, set fixP->fx_done.  */

void
md_apply_fix (fixS *fixP,
	      valueT *valP,
	      segT seg)
{
  char * const fixpos = fixP->fx_frag->fr_literal + fixP->fx_where;
  valueT value = *valP;
  unsigned insn = 0;
  symbolS *fx_addsy, *fx_subsy;
  offsetT fx_offset;
  segT add_symbol_segment = absolute_section;
  segT sub_symbol_segment = absolute_section;
  const struct arc_operand *operand = NULL;
  extended_bfd_reloc_code_real_type reloc;

  pr_debug ("%s:%u: apply_fix: r_type=%d (%s) value=0x%lX offset=0x%lX\n",
	    fixP->fx_file, fixP->fx_line, fixP->fx_r_type,
	    ((int) fixP->fx_r_type < 0) ? "Internal":
	    bfd_get_reloc_code_name (fixP->fx_r_type), value,
	    fixP->fx_offset);

  fx_addsy = fixP->fx_addsy;
  fx_subsy = fixP->fx_subsy;
  fx_offset = 0;

  if (fx_addsy)
    {
      add_symbol_segment = S_GET_SEGMENT (fx_addsy);
    }

  if (fx_subsy
      && fixP->fx_r_type != BFD_RELOC_ARC_TLS_DTPOFF
      && fixP->fx_r_type != BFD_RELOC_ARC_TLS_DTPOFF_S9
      && fixP->fx_r_type != BFD_RELOC_ARC_TLS_GD_LD)
    {
      resolve_symbol_value (fx_subsy);
      sub_symbol_segment = S_GET_SEGMENT (fx_subsy);

      if (sub_symbol_segment == absolute_section)
	{
	  /* The symbol is really a constant.  */
	  fx_offset -= S_GET_VALUE (fx_subsy);
	  fx_subsy = NULL;
	}
      else
	{
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("can't resolve `%s' {%s section} - `%s' {%s section}"),
			fx_addsy ? S_GET_NAME (fx_addsy) : "0",
			segment_name (add_symbol_segment),
			S_GET_NAME (fx_subsy),
			segment_name (sub_symbol_segment));
	  return;
	}
    }

  if (fx_addsy
      && !S_IS_WEAK (fx_addsy))
    {
      if (add_symbol_segment == seg
	  && fixP->fx_pcrel)
	{
	  value += S_GET_VALUE (fx_addsy);
	  value -= md_pcrel_from_section (fixP, seg);
	  fx_addsy = NULL;
	  fixP->fx_pcrel = FALSE;
	}
      else if (add_symbol_segment == absolute_section)
	{
	  value = fixP->fx_offset;
	  fx_offset += S_GET_VALUE (fixP->fx_addsy);
	  fx_addsy = NULL;
	  fixP->fx_pcrel = FALSE;
	}
    }

  if (!fx_addsy)
    fixP->fx_done = TRUE;

  if (fixP->fx_pcrel)
    {
      if (fx_addsy
	  && ((S_IS_DEFINED (fx_addsy)
	       && S_GET_SEGMENT (fx_addsy) != seg)
	      || S_IS_WEAK (fx_addsy)))
	value += md_pcrel_from_section (fixP, seg);

      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_ARC_32_ME:
	  /* This is a pc-relative value in a LIMM.  Adjust it to the
	     address of the instruction not to the address of the
	     LIMM.  Note: it is not anylonger valid this afirmation as
	     the linker consider ARC_PC32 a fixup to entire 64 bit
	     insn.  */
	  fixP->fx_offset += fixP->fx_frag->fr_address;
	  /* Fall through.  */
	case BFD_RELOC_32:
	  fixP->fx_r_type = BFD_RELOC_ARC_PC32;
	  /* Fall through.  */
	case BFD_RELOC_ARC_PC32:
	  /* fixP->fx_offset += fixP->fx_where - fixP->fx_dot_value; */
	  break;
	default:
	  if ((int) fixP->fx_r_type < 0)
	    as_fatal (_("PC relative relocation not allowed for (internal) type %d"),
		      fixP->fx_r_type);
	  break;
	}
    }

  pr_debug ("%s:%u: apply_fix: r_type=%d (%s) value=0x%lX offset=0x%lX\n",
	    fixP->fx_file, fixP->fx_line, fixP->fx_r_type,
	    ((int) fixP->fx_r_type < 0) ? "Internal":
	    bfd_get_reloc_code_name (fixP->fx_r_type), value,
	    fixP->fx_offset);


  /* Now check for TLS relocations.  */
  reloc = fixP->fx_r_type;
  switch (reloc)
    {
    case BFD_RELOC_ARC_TLS_DTPOFF:
    case BFD_RELOC_ARC_TLS_LE_32:
      fixP->fx_offset = 0;
      /* Fall through.  */
    case BFD_RELOC_ARC_TLS_GD_GOT:
    case BFD_RELOC_ARC_TLS_IE_GOT:
      S_SET_THREAD_LOCAL (fixP->fx_addsy);
      break;

    case BFD_RELOC_ARC_TLS_GD_LD:
      gas_assert (!fixP->fx_offset);
      if (fixP->fx_subsy)
	fixP->fx_offset
	  = (S_GET_VALUE (fixP->fx_subsy)
	     - fixP->fx_frag->fr_address- fixP->fx_where);
      fixP->fx_subsy = NULL;
      /* Fall through.  */
    case BFD_RELOC_ARC_TLS_GD_CALL:
      /* These two relocs are there just to allow ld to change the tls
	 model for this symbol, by patching the code.  The offset -
	 and scale, if any - will be installed by the linker.  */
      S_SET_THREAD_LOCAL (fixP->fx_addsy);
      break;

    case BFD_RELOC_ARC_TLS_LE_S9:
    case BFD_RELOC_ARC_TLS_DTPOFF_S9:
      as_bad (_("TLS_*_S9 relocs are not supported yet"));
      break;

    default:
      break;
    }

  if (!fixP->fx_done)
    {
      return;
    }

  /* Addjust the value if we have a constant.  */
  value += fx_offset;

  /* For hosts with longs bigger than 32-bits make sure that the top
     bits of a 32-bit negative value read in by the parser are set,
     so that the correct comparisons are made.  */
  if (value & 0x80000000)
    value |= (-1L << 31);

  reloc = fixP->fx_r_type;
  switch (reloc)
    {
    case BFD_RELOC_8:
    case BFD_RELOC_16:
    case BFD_RELOC_24:
    case BFD_RELOC_32:
    case BFD_RELOC_64:
    case BFD_RELOC_ARC_32_PCREL:
      md_number_to_chars (fixpos, value, fixP->fx_size);
      return;

    case BFD_RELOC_ARC_GOTPC32:
      /* I cannot fix an GOTPC relocation because I need to relax it
	 from ld rx,[pcl,@sym@gotpc] to add rx,pcl,@sym@gotpc.  */
      as_bad (_("Unsupported operation on reloc"));
      return;
    case BFD_RELOC_ARC_GOTOFF:
    case BFD_RELOC_ARC_32_ME:
    case BFD_RELOC_ARC_PC32:
      md_number_to_chars_midend (fixpos, value, fixP->fx_size);
      return;

    case BFD_RELOC_ARC_PLT32:
      md_number_to_chars_midend (fixpos, value, fixP->fx_size);
      return;

    case BFD_RELOC_ARC_S25H_PCREL_PLT:
      reloc = BFD_RELOC_ARC_S25W_PCREL;
      goto solve_plt;

    case BFD_RELOC_ARC_S21H_PCREL_PLT:
      reloc = BFD_RELOC_ARC_S21H_PCREL;
      goto solve_plt;

    case BFD_RELOC_ARC_S25W_PCREL_PLT:
      reloc = BFD_RELOC_ARC_S25W_PCREL;
      goto solve_plt;

    case BFD_RELOC_ARC_S21W_PCREL_PLT:
      reloc = BFD_RELOC_ARC_S21W_PCREL;

    case BFD_RELOC_ARC_S25W_PCREL:
    case BFD_RELOC_ARC_S21W_PCREL:
    case BFD_RELOC_ARC_S21H_PCREL:
    case BFD_RELOC_ARC_S25H_PCREL:
    case BFD_RELOC_ARC_S13_PCREL:
    solve_plt:
      operand = find_operand_for_reloc (reloc);
      gas_assert (operand);
      break;

    default:
      {
	if ((int) fixP->fx_r_type >= 0)
	  as_fatal (_("unhandled relocation type %s"),
		    bfd_get_reloc_code_name (fixP->fx_r_type));

	/* The rest of these fixups needs to be completely resolved as
	   constants.  */
	if (fixP->fx_addsy != 0
	    && S_GET_SEGMENT (fixP->fx_addsy) != absolute_section)
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("non-absolute expression in constant field"));

	gas_assert (-(int) fixP->fx_r_type < (int) arc_num_operands);
	operand = &arc_operands[-(int) fixP->fx_r_type];
	break;
      }
    }

  if (target_big_endian)
    {
      switch (fixP->fx_size)
	{
	case 4:
	  insn = bfd_getb32 (fixpos);
	  break;
	case 2:
	  insn = bfd_getb16 (fixpos);
	  break;
	default:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("unknown fixup size"));
	}
    }
  else
    {
      insn = 0;
      switch (fixP->fx_size)
	{
	case 4:
	  insn = bfd_getl16 (fixpos) << 16 | bfd_getl16 (fixpos + 2);
	  break;
	case 2:
	  insn = bfd_getl16 (fixpos);
	  break;
	default:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("unknown fixup size"));
	}
    }

  insn = insert_operand (insn, operand, (offsetT) value,
			 fixP->fx_file, fixP->fx_line);

  md_number_to_chars_midend (fixpos, insn, fixP->fx_size);
}

/* Prepare machine-dependent frags for relaxation.

   Called just before relaxation starts.  Any symbol that is now undefined
   will not become defined.

   Return the correct fr_subtype in the frag.

   Return the initial "guess for fr_var" to caller.  The guess for fr_var
   is *actually* the growth beyond fr_fix.  Whatever we do to grow fr_fix
   or fr_var contributes to our returned value.

   Although it may not be explicit in the frag, pretend
   fr_var starts with a value.  */

int
md_estimate_size_before_relax (fragS *fragP ATTRIBUTE_UNUSED,
			       segT segment ATTRIBUTE_UNUSED)
{
  int growth = 4;

  fragP->fr_var = 4;
  pr_debug ("%s:%d: md_estimate_size_before_relax: %d\n",
	   fragP->fr_file, fragP->fr_line, growth);

  as_fatal (_("md_estimate_size_before_relax\n"));
  return growth;
}

/* Translate internal representation of relocation info to BFD target
   format.  */

arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED,
	      fixS *fixP)
{
  arelent *reloc;
  bfd_reloc_code_real_type code;

  reloc = (arelent *) xmalloc (sizeof (* reloc));
  reloc->sym_ptr_ptr = (asymbol **) xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);
  reloc->address = fixP->fx_frag->fr_address + fixP->fx_where;

  /* Make sure none of our internal relocations make it this far.
     They'd better have been fully resolved by this point.  */
  gas_assert ((int) fixP->fx_r_type > 0);

  code = fixP->fx_r_type;

  /* if we have something like add gp, pcl,
     _GLOBAL_OFFSET_TABLE_@gotpc.  */
  if (code == BFD_RELOC_ARC_GOTPC32
      && GOT_symbol
      && fixP->fx_addsy == GOT_symbol)
    code = BFD_RELOC_ARC_GOTPC;

  reloc->howto = bfd_reloc_type_lookup (stdoutput, code);
  if (reloc->howto == NULL)
    {
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("cannot represent `%s' relocation in object file"),
		    bfd_get_reloc_code_name (code));
      return NULL;
    }

  if (!fixP->fx_pcrel != !reloc->howto->pc_relative)
    as_fatal (_("internal error? cannot generate `%s' relocation"),
	      bfd_get_reloc_code_name (code));

  gas_assert (!fixP->fx_pcrel == !reloc->howto->pc_relative);

  if (code == BFD_RELOC_ARC_TLS_DTPOFF
      || code ==  BFD_RELOC_ARC_TLS_DTPOFF_S9)
    {
      asymbol *sym
	= fixP->fx_subsy ? symbol_get_bfdsym (fixP->fx_subsy) : NULL;
      /* We just want to store a 24 bit index, but we have to wait
	 till after write_contents has been called via
	 bfd_map_over_sections before we can get the index from
	 _bfd_elf_symbol_from_bfd_symbol.  Thus, the write_relocs
	 function is elf32-arc.c has to pick up the slack.
	 Unfortunately, this leads to problems with hosts that have
	 pointers wider than long (bfd_vma).  There would be various
	 ways to handle this, all error-prone :-(  */
      reloc->addend = (bfd_vma) sym;
      if ((asymbol *) reloc->addend != sym)
	{
	  as_bad ("Can't store pointer\n");
	  return NULL;
	}
    }
  else
    reloc->addend = fixP->fx_offset;

  return reloc;
}

/* Perform post-processing of machine-dependent frags after relaxation.
   Called after relaxation is finished.
   In:	Address of frag.
   fr_type == rs_machine_dependent.
   fr_subtype is what the address relaxed to.

   Out: Any fixS:s and constants are set up.  */

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 segT segment ATTRIBUTE_UNUSED,
		 fragS *fragP ATTRIBUTE_UNUSED)
{
  pr_debug ("%s:%d: md_convert_frag, subtype: %d, fix: %d, var: %d\n",
	    fragP->fr_file, fragP->fr_line,
	    fragP->fr_subtype, fragP->fr_fix, fragP->fr_var);
  abort ();
}

/* We have no need to default values of symbols.  We could catch
   register names here, but that is handled by inserting them all in
   the symbol table to begin with.  */

symbolS *
md_undefined_symbol (char *name)
{
  /* The arc abi demands that a GOT[0] should be referencible as
     [pc+_DYNAMIC@gotpc].  Hence we convert a _DYNAMIC@gotpc to a
     GOTPC reference to _GLOBAL_OFFSET_TABLE_.  */
  if (((*name == '_')
       && (*(name+1) == 'G')
       && (strcmp (name, GLOBAL_OFFSET_TABLE_NAME) == 0))
      || ((*name == '_')
	  && (*(name+1) == 'D')
	  && (strcmp (name, DYNAMIC_STRUCT_NAME) == 0)))
    {
      if (!GOT_symbol)
	{
	  if (symbol_find (name))
	    as_bad ("GOT already in symbol table");

	  GOT_symbol = symbol_new (GLOBAL_OFFSET_TABLE_NAME, undefined_section,
				   (valueT) 0, &zero_address_frag);
	};
      return GOT_symbol;
    }
  return NULL;
}

/* Turn a string in input_line_pointer into a floating point constant
   of type type, and store the appropriate bytes in *litP.  The number
   of LITTLENUMS emitted is stored in *sizeP.  An error message is
   returned, or NULL on OK.  */

char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, target_big_endian);
}

/* Called for any expression that can not be recognized.  When the
   function is called, `input_line_pointer' will point to the start of
   the expression.  */

void
md_operand (expressionS *expressionP ATTRIBUTE_UNUSED)
{
  char *p = input_line_pointer;
  if (*p == '@')
    {
      input_line_pointer++;
      expressionP->X_op = O_symbol;
      expression (expressionP);
    }
}

/* This function is called from the function 'expression', it attempts
   to parse special names (in our case register names).  It fills in
   the expression with the identified register.  It returns TRUE if
   it is a register and FALSE otherwise.  */

bfd_boolean
arc_parse_name (const char *name,
		struct expressionS *e)
{
  struct symbol *sym;

  if (!assembling_insn)
    return FALSE;

  /* Handle only registers.  */
  if (e->X_op != O_absent)
    return FALSE;

  sym = hash_find (arc_reg_hash, name);
  if (sym)
    {
      e->X_op = O_register;
      e->X_add_number = S_GET_VALUE (sym);
      return TRUE;
    }
  return FALSE;
}

/* md_parse_option
   Invocation line includes a switch not recognized by the base assembler.
   See if it's a processor-specific option.

   New options (supported) are:

   -mcpu=<cpu name>		 Assemble for selected processor
   -EB/-mbig-endian		 Big-endian
   -EL/-mlittle-endian		 Little-endian

   The following CPU names are recognized:
   arc700, av2em, av2hs.  */

int
md_parse_option (int c, char *arg ATTRIBUTE_UNUSED)
{
  int cpu_flags = EF_ARC_CPU_GENERIC;

  switch (c)
    {
    case OPTION_ARC600:
    case OPTION_ARC601:
      return md_parse_option (OPTION_MCPU, "arc600");

    case OPTION_ARC700:
      return md_parse_option (OPTION_MCPU, "arc700");

    case OPTION_ARCEM:
      return md_parse_option (OPTION_MCPU, "arcem");

    case OPTION_ARCHS:
      return md_parse_option (OPTION_MCPU, "archs");

    case OPTION_MCPU:
      {
	int i;
	char *s = alloca (strlen (arg) + 1);

	{
	  char *t = s;
	  char *arg1 = arg;

	  do
	    *t = TOLOWER (*arg1++);
	  while (*t++);
	}

	for (i = 0; cpu_types[i].name; ++i)
	  {
	    if (!strcmp (cpu_types[i].name, s))
	      {
		arc_target      = cpu_types[i].flags;
		arc_target_name = cpu_types[i].name;
		arc_features    = cpu_types[i].features;
		arc_mach_type   = cpu_types[i].mach;
		cpu_flags       = cpu_types[i].eflags;

		mach_type_specified_p = 1;
		break;
	      }
	  }

	if (!cpu_types[i].name)
	  {
	    as_fatal (_("unknown architecture: %s\n"), arg);
	  }
	break;
      }

    case OPTION_EB:
      arc_target_format = "elf32-bigarc";
      byte_order = BIG_ENDIAN;
      break;

    case OPTION_EL:
      arc_target_format = "elf32-littlearc";
      byte_order = LITTLE_ENDIAN;
      break;

    case OPTION_CD:
      /* This option has an effect only on ARC EM.  */
      if (arc_target & ARC_OPCODE_ARCv2EM)
	arc_features |= ARC_CD;
      break;

    case OPTION_USER_MODE:
    case OPTION_LD_EXT_MASK:
    case OPTION_SWAP:
    case OPTION_NORM:
    case OPTION_BARREL_SHIFT:
    case OPTION_MIN_MAX:
    case OPTION_NO_MPY:
    case OPTION_EA:
    case OPTION_MUL64:
    case OPTION_SIMD:
    case OPTION_SPFP:
    case OPTION_DPFP:
    case OPTION_XMAC_D16:
    case OPTION_XMAC_24:
    case OPTION_DSP_PACKA:
    case OPTION_CRC:
    case OPTION_DVBF:
    case OPTION_TELEPHONY:
    case OPTION_XYMEMORY:
    case OPTION_LOCK:
    case OPTION_SWAPE:
    case OPTION_RTSC:
    case OPTION_FPUDA:
      /* Dummy options.  */
      break;

    default:
      return 0;
    }

  if (cpu_flags != EF_ARC_CPU_GENERIC)
    arc_eflag = (arc_eflag & ~EF_ARC_MACH_MSK) | cpu_flags;

  return 1;
}

void
md_show_usage (FILE *stream)
{
  fprintf (stream, _("ARC-specific assembler options:\n"));

  fprintf (stream, "  -mcpu=<cpu name>\t  assemble for CPU <cpu name>\n");
  fprintf (stream,
	   "  -mcode-density\t  enable code density option for ARC EM\n");

  fprintf (stream, _("\
  -EB                     assemble code for a big-endian cpu\n"));
  fprintf (stream, _("\
  -EL                     assemble code for a little-endian cpu\n"));
}

static void
preprocess_operands (const struct arc_opcode *opcode,
		     expressionS *tok,
		     int ntok)
{
  int i;
  size_t len;
  const char *p;
  unsigned j;
  const struct arc_aux_reg *auxr;

  for (i = 0; i < ntok; i++)
    {
      switch (tok[i].X_op)
	{
	case O_illegal:
	case O_absent:
	  break; /* Throw and error.  */

	case O_symbol:
	  if (opcode->class != AUXREG)
	    break;
	  /* Convert the symbol to a constant if possible.  */
	  p = S_GET_NAME (tok[i].X_add_symbol);
	  len = strlen (p);

	  auxr = &arc_aux_regs[0];
	  for (j = 0; j < arc_num_aux_regs; j++, auxr++)
	    if (len == auxr->length
		&& strcasecmp (auxr->name, p) == 0)
	      {
		tok[i].X_op = O_constant;
		tok[i].X_add_number = auxr->address;
		break;
	      }
	  break;
	default:
	  break;
	}
    }
}

/* Given an opcode name, pre-tockenized set of argumenst and the
   opcode flags, take it all the way through emission.  */

static void
assemble_tokens (const char *opname,
		 expressionS *tok,
		 int ntok,
		 struct arc_flags *pflags,
		 int nflgs)
{
  bfd_boolean found_something = FALSE;
  const struct arc_opcode *opcode;
  int cpumatch = 1;

  /* Search opcodes.  */
  opcode = (const struct arc_opcode *) hash_find (arc_opcode_hash, opname);

  /* Couldn't find opcode conventional way, try special cases.  */
  if (!opcode)
    opcode = find_special_case (opname, &nflgs, pflags, tok, &ntok);

  if (opcode)
    {
      pr_debug ("%s:%d: assemble_tokens: %s trying opcode 0x%08X\n",
		frag_now->fr_file, frag_now->fr_line, opcode->name,
		opcode->opcode);

      preprocess_operands (opcode, tok, ntok);

      found_something = TRUE;
      opcode = find_opcode_match (opcode, tok, &ntok, pflags, nflgs, &cpumatch);
      if (opcode)
	{
	  struct arc_insn insn;
	  assemble_insn (opcode, tok, ntok, pflags, nflgs, &insn);
	  emit_insn (&insn);
	  return;
	}
    }

  if (found_something)
    {
      if (cpumatch)
	as_bad (_("inappropriate arguments for opcode '%s'"), opname);
      else
	as_bad (_("opcode '%s' not supported for target %s"), opname,
		arc_target_name);
    }
  else
    as_bad (_("unknown opcode '%s'"), opname);
}

/* Used to find special case opcode.  */

static const struct arc_opcode *
find_special_case (const char *opname,
		   int *nflgs,
		   struct arc_flags *pflags,
		   expressionS *tok,
		   int *ntok)
{
  const struct arc_opcode *opcode;

  opcode = find_special_case_pseudo (opname, ntok, tok, nflgs, pflags);

  if (opcode == NULL)
    opcode = find_special_case_flag (opname, nflgs, pflags);

  return opcode;
}

/* Swap operand tokens.  */

static void
swap_operand (expressionS *operand_array,
	      unsigned source,
	      unsigned destination)
{
  expressionS cpy_operand;
  expressionS *src_operand;
  expressionS *dst_operand;
  size_t size;

  if (source == destination)
    return;

  src_operand = &operand_array[source];
  dst_operand = &operand_array[destination];
  size = sizeof (expressionS);

  /* Make copy of operand to swap with and swap.  */
  memcpy (&cpy_operand, dst_operand, size);
  memcpy (dst_operand, src_operand, size);
  memcpy (src_operand, &cpy_operand, size);
}

/* Check if *op matches *tok type.
   Returns FALSE if they don't match, TRUE if they match.  */

static bfd_boolean
pseudo_operand_match (const expressionS *tok,
		      const struct arc_operand_operation *op)
{
  offsetT min, max, val;
  bfd_boolean ret;
  const struct arc_operand *operand_real = &arc_operands[op->operand_idx];

  ret = FALSE;
  switch (tok->X_op)
    {
    case O_constant:
      if (operand_real->bits == 32 && (operand_real->flags & ARC_OPERAND_LIMM))
	ret = 1;
      else if (!(operand_real->flags & ARC_OPERAND_IR))
	{
	  val = tok->X_add_number;
	  if (operand_real->flags & ARC_OPERAND_SIGNED)
	    {
	      max = (1 << (operand_real->bits - 1)) - 1;
	      min = -(1 << (operand_real->bits - 1));
	    }
	  else
	    {
	      max = (1 << operand_real->bits) - 1;
	      min = 0;
	    }
	  if (min <= val && val <= max)
	    ret = TRUE;
	}
      break;

    case O_symbol:
      /* Handle all symbols as long immediates or signed 9.  */
      if (operand_real->flags & ARC_OPERAND_LIMM ||
	  ((operand_real->flags & ARC_OPERAND_SIGNED) && operand_real->bits == 9))
	ret = TRUE;
      break;

    case O_register:
      if (operand_real->flags & ARC_OPERAND_IR)
	ret = TRUE;
      break;

    case O_bracket:
      if (operand_real->flags & ARC_OPERAND_BRAKET)
	ret = TRUE;
      break;

    default:
      /* Unknown.  */
      break;
    }
  return ret;
}

/* Find pseudo instruction in array.  */

static const struct arc_pseudo_insn *
find_pseudo_insn (const char *opname,
		  int ntok,
		  const expressionS *tok)
{
  const struct arc_pseudo_insn *pseudo_insn = NULL;
  const struct arc_operand_operation *op;
  unsigned int i;
  int j;

  for (i = 0; i < arc_num_pseudo_insn; ++i)
    {
      pseudo_insn = &arc_pseudo_insns[i];
      if (strcmp (pseudo_insn->mnemonic_p, opname) == 0)
	{
	  op = pseudo_insn->operand;
	  for (j = 0; j < ntok; ++j)
	    if (!pseudo_operand_match (&tok[j], &op[j]))
	      break;

	  /* Found the right instruction.  */
	  if (j == ntok)
	    return pseudo_insn;
	}
    }
  return NULL;
}

/* Assumes the expressionS *tok is of sufficient size.  */

static const struct arc_opcode *
find_special_case_pseudo (const char *opname,
			  int *ntok,
			  expressionS *tok,
			  int *nflgs,
			  struct arc_flags *pflags)
{
  const struct arc_pseudo_insn *pseudo_insn = NULL;
  const struct arc_operand_operation *operand_pseudo;
  const struct arc_operand *operand_real;
  unsigned i;
  char construct_operand[MAX_CONSTR_STR];

  /* Find whether opname is in pseudo instruction array.  */
  pseudo_insn = find_pseudo_insn (opname, *ntok, tok);

  if (pseudo_insn == NULL)
    return NULL;

  /* Handle flag, Limited to one flag at the moment.  */
  if (pseudo_insn->flag_r != NULL)
    *nflgs += tokenize_flags (pseudo_insn->flag_r, &pflags[*nflgs],
			      MAX_INSN_FLGS - *nflgs);

  /* Handle operand operations.  */
  for (i = 0; i < pseudo_insn->operand_cnt; ++i)
    {
      operand_pseudo = &pseudo_insn->operand[i];
      operand_real = &arc_operands[operand_pseudo->operand_idx];

      if (operand_real->flags & ARC_OPERAND_BRAKET &&
	  !operand_pseudo->needs_insert)
	continue;

      /* Has to be inserted (i.e. this token does not exist yet).  */
      if (operand_pseudo->needs_insert)
	{
	  if (operand_real->flags & ARC_OPERAND_BRAKET)
	    {
	      tok[i].X_op = O_bracket;
	      ++(*ntok);
	      continue;
	    }

	  /* Check if operand is a register or constant and handle it
	     by type.  */
	  if (operand_real->flags & ARC_OPERAND_IR)
	    snprintf (construct_operand, MAX_CONSTR_STR, "r%d",
		      operand_pseudo->count);
	  else
	    snprintf (construct_operand, MAX_CONSTR_STR, "%d",
		      operand_pseudo->count);

	  tokenize_arguments (construct_operand, &tok[i], 1);
	  ++(*ntok);
	}

      else if (operand_pseudo->count)
	{
	  /* Operand number has to be adjusted accordingly (by operand
	     type).  */
	  switch (tok[i].X_op)
	    {
	    case O_constant:
	      tok[i].X_add_number += operand_pseudo->count;
	      break;

	    case O_symbol:
	      break;

	    default:
	      /* Ignored.  */
	      break;
	    }
	}
    }

  /* Swap operands if necessary.  Only supports one swap at the
     moment.  */
  for (i = 0; i < pseudo_insn->operand_cnt; ++i)
    {
      operand_pseudo = &pseudo_insn->operand[i];

      if (operand_pseudo->swap_operand_idx == i)
	continue;

      swap_operand (tok, i, operand_pseudo->swap_operand_idx);

      /* Prevent a swap back later by breaking out.  */
      break;
    }

  return (const struct arc_opcode *)
    hash_find (arc_opcode_hash,	pseudo_insn->mnemonic_r);
}

static const struct arc_opcode *
find_special_case_flag (const char *opname,
			int *nflgs,
			struct arc_flags *pflags)
{
  unsigned int i;
  const char *flagnm;
  unsigned flag_idx, flag_arr_idx;
  size_t flaglen, oplen;
  const struct arc_flag_special *arc_flag_special_opcode;
  const struct arc_opcode *opcode;

  /* Search for special case instruction.  */
  for (i = 0; i < arc_num_flag_special; i++)
    {
      arc_flag_special_opcode = &arc_flag_special_cases[i];
      oplen = strlen (arc_flag_special_opcode->name);

      if (strncmp (opname, arc_flag_special_opcode->name, oplen) != 0)
	continue;

      /* Found a potential special case instruction, now test for
	 flags.  */
      for (flag_arr_idx = 0;; ++flag_arr_idx)
	{
	  flag_idx = arc_flag_special_opcode->flags[flag_arr_idx];
	  if (flag_idx == 0)
	    break;  /* End of array, nothing found.  */

	  flagnm = arc_flag_operands[flag_idx].name;
	  flaglen = strlen (flagnm);
	  if (strcmp (opname + oplen, flagnm) == 0)
	    {
	      opcode = (const struct arc_opcode *)
		hash_find (arc_opcode_hash,
			   arc_flag_special_opcode->name);

	      if (*nflgs + 1 > MAX_INSN_FLGS)
		break;
	      memcpy (pflags[*nflgs].name, flagnm, flaglen);
	      pflags[*nflgs].name[flaglen] = '\0';
	      (*nflgs)++;
	      return opcode;
	    }
	}
    }
  return NULL;
}

/* Check whether a symbol involves a register.  */

static int
contains_register (symbolS *sym)
{
  if (sym)
  {
    expressionS *ex = symbol_get_value_expression (sym);
    return ((O_register == ex->X_op)
	    && !contains_register (ex->X_add_symbol)
	    && !contains_register (ex->X_op_symbol));
  }
  else
    return 0;
}

/* Returns the register number within a symbol.  */

static int
get_register (symbolS *sym)
{
  if (!contains_register (sym))
    return -1;

  expressionS *ex = symbol_get_value_expression (sym);
  return regno (ex->X_add_number);
}

/* Allocates a tok entry.  */

static int
allocate_tok (expressionS *tok, int ntok, int cidx)
{
  if (ntok > MAX_INSN_ARGS - 2)
    return 0; /* No space left.  */

  if (cidx > ntok)
    return 0; /* Incorect args.  */

  memcpy (&tok[ntok+1], &tok[ntok], sizeof (*tok));

  if (cidx == ntok)
    return 1; /* Success.  */
  return allocate_tok (tok, ntok - 1, cidx);
}

/* Return true if a RELOC is generic.  A generic reloc is PC-rel of a
   simple ME relocation (e.g. RELOC_ARC_32_ME, BFD_RELOC_ARC_PC32.  */

static bfd_boolean
generic_reloc_p (extended_bfd_reloc_code_real_type reloc)
{
  if (!reloc)
    return FALSE;

  switch (reloc)
    {
    case BFD_RELOC_ARC_SDA_LDST:
    case BFD_RELOC_ARC_SDA_LDST1:
    case BFD_RELOC_ARC_SDA_LDST2:
    case BFD_RELOC_ARC_SDA16_LD:
    case BFD_RELOC_ARC_SDA16_LD1:
    case BFD_RELOC_ARC_SDA16_LD2:
    case BFD_RELOC_ARC_SDA16_ST2:
    case BFD_RELOC_ARC_SDA32_ME:
      return FALSE;
    default:
      break;
    }
  return TRUE;
}

/* Search forward through all variants of an opcode looking for a
   syntax match.  */

static const struct arc_opcode *
find_opcode_match (const struct arc_opcode *first_opcode,
		   expressionS *tok,
		   int *pntok,
		   struct arc_flags *first_pflag,
		   int nflgs,
		   int *pcpumatch)
{
  const struct arc_opcode *opcode = first_opcode;
  int ntok = *pntok;
  int got_cpu_match = 0;
  expressionS bktok[MAX_INSN_ARGS];
  int bkntok;
  expressionS emptyE;

  memset (&emptyE, 0, sizeof (emptyE));
  memcpy (bktok, tok, MAX_INSN_ARGS * sizeof (*tok));
  bkntok = ntok;

  do
    {
      const unsigned char *opidx;
      const unsigned char *flgidx;
      int tokidx = 0;
      const expressionS *t = &emptyE;

      pr_debug ("%s:%d: find_opcode_match: trying opcode 0x%08X ",
		frag_now->fr_file, frag_now->fr_line, opcode->opcode);

      /* Don't match opcodes that don't exist on this
	 architecture.  */
      if (!(opcode->cpu & arc_target))
	goto match_failed;

      if (is_code_density_p (opcode) && !(arc_features & ARC_CD))
	goto match_failed;

      got_cpu_match = 1;
      pr_debug ("cpu ");

      /* Check the operands.  */
      for (opidx = opcode->operands; *opidx; ++opidx)
	{
	  const struct arc_operand *operand = &arc_operands[*opidx];

	  /* Only take input from real operands.  */
	  if ((operand->flags & ARC_OPERAND_FAKE)
	      && !(operand->flags & ARC_OPERAND_BRAKET))
	    continue;

	  /* When we expect input, make sure we have it.  */
	  if (tokidx >= ntok)
	    goto match_failed;

	  /* Match operand type with expression type.  */
	  switch (operand->flags & ARC_OPERAND_TYPECHECK_MASK)
	    {
	    case ARC_OPERAND_IR:
	      /* Check to be a register.  */
	      if ((tok[tokidx].X_op != O_register
		   || !is_ir_num (tok[tokidx].X_add_number))
		  && !(operand->flags & ARC_OPERAND_IGNORE))
		goto match_failed;

	      /* If expect duplicate, make sure it is duplicate.  */
	      if (operand->flags & ARC_OPERAND_DUPLICATE)
		{
		  /* Check for duplicate.  */
		  if (t->X_op != O_register
		      || !is_ir_num (t->X_add_number)
		      || (regno (t->X_add_number) !=
			  regno (tok[tokidx].X_add_number)))
		    goto match_failed;
		}

	      /* Special handling?  */
	      if (operand->insert)
		{
		  const char *errmsg = NULL;
		  (*operand->insert)(0,
				     regno (tok[tokidx].X_add_number),
				     &errmsg);
		  if (errmsg)
		    {
		      if (operand->flags & ARC_OPERAND_IGNORE)
			{
			  /* Missing argument, create one.  */
			  if (!allocate_tok (tok, ntok - 1, tokidx))
			    goto match_failed;

			  tok[tokidx].X_op = O_absent;
			  ++ntok;
			}
		      else
			goto match_failed;
		    }
		}

	      t = &tok[tokidx];
	      break;

	    case ARC_OPERAND_BRAKET:
	      /* Check if bracket is also in opcode table as
		 operand.  */
	      if (tok[tokidx].X_op != O_bracket)
		goto match_failed;
	      break;

	    case ARC_OPERAND_LIMM:
	    case ARC_OPERAND_SIGNED:
	    case ARC_OPERAND_UNSIGNED:
	      switch (tok[tokidx].X_op)
		{
		case O_illegal:
		case O_absent:
		case O_register:
		  goto match_failed;

		case O_bracket:
		  /* Got an (too) early bracket, check if it is an
		     ignored operand.  N.B. This procedure works only
		     when bracket is the last operand!  */
		  if (!(operand->flags & ARC_OPERAND_IGNORE))
		    goto match_failed;
		  /* Insert the missing operand.  */
		  if (!allocate_tok (tok, ntok - 1, tokidx))
		    goto match_failed;

		  tok[tokidx].X_op = O_absent;
		  ++ntok;
		  break;

		case O_constant:
		  /* Check the range.  */
		  if (operand->bits != 32
		      && !(operand->flags & ARC_OPERAND_NCHK))
		    {
		      offsetT min, max, val;
		      val = tok[tokidx].X_add_number;

		      if (operand->flags & ARC_OPERAND_SIGNED)
			{
			  max = (1 << (operand->bits - 1)) - 1;
			  min = -(1 << (operand->bits - 1));
			}
		      else
			{
			  max = (1 << operand->bits) - 1;
			  min = 0;
			}

		      if (val < min || val > max)
			goto match_failed;

		      /* Check alignmets.  */
		      if ((operand->flags & ARC_OPERAND_ALIGNED32)
			  && (val & 0x03))
			goto match_failed;

		      if ((operand->flags & ARC_OPERAND_ALIGNED16)
			  && (val & 0x01))
			goto match_failed;
		    }
		  else if (operand->flags & ARC_OPERAND_NCHK)
		    {
		      if (operand->insert)
			{
			  const char *errmsg = NULL;
			  (*operand->insert)(0,
					     tok[tokidx].X_add_number,
					     &errmsg);
			  if (errmsg)
			    goto match_failed;
			}
		      else
			goto match_failed;
		    }
		  break;

		case O_subtract:
		  /* Check if it is register range.  */
		  if ((tok[tokidx].X_add_number == 0)
		      && contains_register (tok[tokidx].X_add_symbol)
		      && contains_register (tok[tokidx].X_op_symbol))
		    {
		      int regs;

		      regs = get_register (tok[tokidx].X_add_symbol);
		      regs <<= 16;
		      regs |= get_register (tok[tokidx].X_op_symbol);
		      if (operand->insert)
			{
			  const char *errmsg = NULL;
			  (*operand->insert)(0,
					     regs,
					     &errmsg);
			  if (errmsg)
			    goto match_failed;
			}
		      else
			goto match_failed;
		      break;
		    }
		default:
		  if (operand->default_reloc == 0)
		    goto match_failed; /* The operand needs relocation.  */

		  /* Relocs requiring long immediate.  FIXME! make it
		     generic and move it to a function.  */
		  switch (tok[tokidx].X_md)
		    {
		    case O_gotoff:
		    case O_gotpc:
		    case O_pcl:
		    case O_tpoff:
		    case O_dtpoff:
		    case O_tlsgd:
		    case O_tlsie:
		      if (!(operand->flags & ARC_OPERAND_LIMM))
			goto match_failed;
		    case O_absent:
		      if (!generic_reloc_p (operand->default_reloc))
			goto match_failed;
		    default:
		      break;
		    }
		  break;
		}
	      /* If expect duplicate, make sure it is duplicate.  */
	      if (operand->flags & ARC_OPERAND_DUPLICATE)
		{
		  if (t->X_op == O_illegal
		      || t->X_op == O_absent
		      || t->X_op == O_register
		      || (t->X_add_number != tok[tokidx].X_add_number))
		    goto match_failed;
		}
	      t = &tok[tokidx];
	      break;

	    default:
	      /* Everything else should have been fake.  */
	      abort ();
	    }

	  ++tokidx;
	}
      pr_debug ("opr ");

      /* Check the flags.  Iterate over the valid flag classes.  */
      int lnflg = nflgs;

      for (flgidx = opcode->flags; *flgidx && lnflg; ++flgidx)
	{
	  /* Get a valid flag class.  */
	  const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
	  const unsigned *flgopridx;

	  for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	    {
	      const struct arc_flag_operand *flg_operand;
	      struct arc_flags *pflag = first_pflag;
	      int i;

	      flg_operand = &arc_flag_operands[*flgopridx];
	      for (i = 0; i < nflgs; i++, pflag++)
		{
		  /* Match against the parsed flags.  */
		  if (!strcmp (flg_operand->name, pflag->name))
		    {
		      /*TODO: Check if it is duplicated.  */
		      pflag->code = *flgopridx;
		      lnflg--;
		      break; /* goto next flag class and parsed flag.  */
		    }
		}
	    }
	}
      /* Did I check all the parsed flags?  */
      if (lnflg)
	goto match_failed;

      pr_debug ("flg");
      /* Possible match -- did we use all of our input?  */
      if (tokidx == ntok)
	{
	  *pntok = ntok;
	  pr_debug ("\n");
	  return opcode;
	}

    match_failed:;
      pr_debug ("\n");
      /* Restore the original parameters.  */
      memcpy (tok, bktok, MAX_INSN_ARGS * sizeof (*tok));
      ntok = bkntok;
    }
  while (++opcode - arc_opcodes < (int) arc_num_opcodes
	 && !strcmp (opcode->name, first_opcode->name));

  if (*pcpumatch)
    *pcpumatch = got_cpu_match;

  return NULL;
}

/* Find the proper relocation for the given opcode.  */

static extended_bfd_reloc_code_real_type
find_reloc (const char *name,
	    const char *opcodename,
	    const struct arc_flags *pflags,
	    int nflg,
	    extended_bfd_reloc_code_real_type reloc)
{
  unsigned int i;
  int j;
  bfd_boolean found_flag;
  extended_bfd_reloc_code_real_type ret = BFD_RELOC_UNUSED;

  for (i = 0; i < arc_num_equiv_tab; i++)
    {
      const struct arc_reloc_equiv_tab *r = &arc_reloc_equiv[i];

      /* Find the entry.  */
      if (strcmp (name, r->name))
	continue;
      if (r->mnemonic && (strcmp (r->mnemonic, opcodename)))
	continue;
      if (r->flagcode)
	{
	  if (!nflg)
	    continue;
	  found_flag = FALSE;
	  for (j = 0; j < nflg; j++)
	    if (pflags[i].code == r->flagcode)
	      {
		found_flag = TRUE;
		break;
	      }
	  if (!found_flag)
	    continue;
	}

      if (reloc != r->oldreloc)
	continue;
      /* Found it.  */
      ret = r->newreloc;
      break;
    }

  if (ret == BFD_RELOC_UNUSED)
    as_bad (_("Unable to find %s relocation for instruction %s"),
	    name, opcodename);
  return ret;
}

/* Turn an opcode description and a set of arguments into
   an instruction and a fixup.  */

static void
assemble_insn (const struct arc_opcode *opcode,
	       const expressionS *tok,
	       int ntok,
	       const struct arc_flags *pflags,
	       int nflg,
	       struct arc_insn *insn)
{
  const expressionS *reloc_exp = NULL;
  unsigned image;
  const unsigned char *argidx;
  int i;
  int tokidx = 0;
  unsigned char pcrel = 0;
  bfd_boolean needGOTSymbol;
  bfd_boolean has_delay_slot = FALSE;
  extended_bfd_reloc_code_real_type reloc = BFD_RELOC_UNUSED;

  memset (insn, 0, sizeof (*insn));
  image = opcode->opcode;

  pr_debug ("%s:%d: assemble_insn: %s using opcode %x\n",
	    frag_now->fr_file, frag_now->fr_line, opcode->name,
	    opcode->opcode);

  /* Handle operands.  */
  for (argidx = opcode->operands; *argidx; ++argidx)
    {
      const struct arc_operand *operand = &arc_operands[*argidx];
      const expressionS *t = (const expressionS *) 0;

      if ((operand->flags & ARC_OPERAND_FAKE)
	  && !(operand->flags & ARC_OPERAND_BRAKET))
	continue;

      if (operand->flags & ARC_OPERAND_DUPLICATE)
	{
	  /* Duplicate operand, already inserted.  */
	  tokidx ++;
	  continue;
	}

      if (tokidx >= ntok)
	{
	  abort ();
	}
      else
	t = &tok[tokidx++];

      /* Regardless if we have a reloc or not mark the instruction
	 limm if it is the case.  */
      if (operand->flags & ARC_OPERAND_LIMM)
	insn->has_limm = TRUE;

      switch (t->X_op)
	{
	case O_register:
	  image = insert_operand (image, operand, regno (t->X_add_number),
				  NULL, 0);
	  break;

	case O_constant:
	  image = insert_operand (image, operand, t->X_add_number, NULL, 0);
	  reloc_exp = t;
	  if (operand->flags & ARC_OPERAND_LIMM)
	    insn->limm = t->X_add_number;
	  break;

	case O_bracket:
	  /* Ignore brackets.  */
	  break;

	case O_absent:
	  gas_assert (operand->flags & ARC_OPERAND_IGNORE);
	  break;

	case O_subtract:
	  /* Maybe register range.  */
	  if ((t->X_add_number == 0)
	      && contains_register (t->X_add_symbol)
	      && contains_register (t->X_op_symbol))
	    {
	      int regs;

	      regs = get_register (t->X_add_symbol);
	      regs <<= 16;
	      regs |= get_register (t->X_op_symbol);
	      image = insert_operand (image, operand, regs, NULL, 0);
	      break;
	    }

	default:
	  /* This operand needs a relocation.  */
	  needGOTSymbol = FALSE;

	  switch (t->X_md)
	    {
	    case O_plt:
	      needGOTSymbol = TRUE;
	      reloc = find_reloc ("plt", opcode->name,
				  pflags, nflg,
				  operand->default_reloc);
	      break;

	    case O_gotoff:
	    case O_gotpc:
	      needGOTSymbol = TRUE;
	      reloc = ARC_RELOC_TABLE (t->X_md)->reloc;
	      break;
	    case O_pcl:
	      reloc = ARC_RELOC_TABLE (t->X_md)->reloc;
	      if (ARC_SHORT (opcode->mask))
		as_bad_where (frag_now->fr_file, frag_now->fr_line,
			      _("Unable to use @pcl relocation for insn %s"),
			      opcode->name);
	      break;
	    case O_sda:
	      reloc = find_reloc ("sda", opcode->name,
				  pflags, nflg,
				  operand->default_reloc);
	      break;
	    case O_tlsgd:
	    case O_tlsie:
	      needGOTSymbol = TRUE;
	      /* Fall-through.  */

	    case O_tpoff:
	    case O_dtpoff:
	      reloc = ARC_RELOC_TABLE (t->X_md)->reloc;
	      break;

	    case O_tpoff9: /*FIXME! Check for the conditionality of
			     the insn.  */
	    case O_dtpoff9: /*FIXME! Check for the conditionality of
			      the insn.  */
	      as_bad (_("TLS_*_S9 relocs are not supported yet"));
	      break;

	    default:
	      /* Just consider the default relocation.  */
	      reloc = operand->default_reloc;
	      break;
	    }

	  if (needGOTSymbol && (GOT_symbol == NULL))
	    GOT_symbol = symbol_find_or_make (GLOBAL_OFFSET_TABLE_NAME);

	  reloc_exp = t;

#if 0
	  if (reloc > 0)
	    {
	      /* sanity checks.  */
	      reloc_howto_type *reloc_howto
		= bfd_reloc_type_lookup (stdoutput,
					 (bfd_reloc_code_real_type) reloc);
	      unsigned reloc_bitsize = reloc_howto->bitsize;
	      if (reloc_howto->rightshift)
		reloc_bitsize -= reloc_howto->rightshift;
	      if (reloc_bitsize != operand->bits)
		{
		  as_bad (_("invalid relocation %s for field"),
			  bfd_get_reloc_code_name (reloc));
		  return;
		}
	    }
#endif
	  if (insn->nfixups >= MAX_INSN_FIXUPS)
	    as_fatal (_("too many fixups"));

	  struct arc_fixup *fixup;
	  fixup = &insn->fixups[insn->nfixups++];
	  fixup->exp = *t;
	  fixup->reloc = reloc;
	  pcrel = (operand->flags & ARC_OPERAND_PCREL) ? 1 : 0;
	  fixup->pcrel = pcrel;
	  fixup->islong = (operand->flags & ARC_OPERAND_LIMM) ?
	    TRUE : FALSE;
	  break;
	}
    }

  /* Handle flags.  */
  for (i = 0; i < nflg; i++)
    {
      const struct arc_flag_operand *flg_operand =
	&arc_flag_operands[pflags[i].code];

      /* Check if the instruction has a delay slot.  */
      if (!strcmp (flg_operand->name, "d"))
	has_delay_slot = TRUE;

      /* There is an exceptional case when we cannot insert a flag
	 just as it is.  The .T flag must be handled in relation with
	 the relative address.  */
      if (!strcmp (flg_operand->name, "t")
	  || !strcmp (flg_operand->name, "nt"))
	{
	  unsigned bitYoperand = 0;
	  /* FIXME! move selection bbit/brcc in arc-opc.c.  */
	  if (!strcmp (flg_operand->name, "t"))
	    if (!strcmp (opcode->name, "bbit0")
		|| !strcmp (opcode->name, "bbit1"))
	      bitYoperand = arc_NToperand;
	    else
	      bitYoperand = arc_Toperand;
	  else
	    if (!strcmp (opcode->name, "bbit0")
		|| !strcmp (opcode->name, "bbit1"))
	      bitYoperand = arc_Toperand;
	    else
	      bitYoperand = arc_NToperand;

	  gas_assert (reloc_exp != NULL);
	  if (reloc_exp->X_op == O_constant)
	    {
	      /* Check if we have a constant and solved it
		 immediately.  */
	      offsetT val = reloc_exp->X_add_number;
	      image |= insert_operand (image, &arc_operands[bitYoperand],
				       val, NULL, 0);
	    }
	  else
	    {
	      struct arc_fixup *fixup;

	      if (insn->nfixups >= MAX_INSN_FIXUPS)
		as_fatal (_("too many fixups"));

	      fixup = &insn->fixups[insn->nfixups++];
	      fixup->exp = *reloc_exp;
	      fixup->reloc = -bitYoperand;
	      fixup->pcrel = pcrel;
	      fixup->islong = FALSE;
	    }
	}
      else
	image |= (flg_operand->code & ((1 << flg_operand->bits) - 1))
	  << flg_operand->shift;
    }

  /* Short instruction?  */
  insn->short_insn = ARC_SHORT (opcode->mask) ? TRUE : FALSE;

  insn->insn = image;

  /* Update last insn status.  */
  arc_last_insns[1]		   = arc_last_insns[0];
  arc_last_insns[0].opcode	   = opcode;
  arc_last_insns[0].has_limm	   = insn->has_limm;
  arc_last_insns[0].has_delay_slot = has_delay_slot;

  /* Check if the current instruction is legally used.  */
  if (arc_last_insns[1].has_delay_slot
      && is_br_jmp_insn_p (arc_last_insns[0].opcode))
    as_bad_where (frag_now->fr_file, frag_now->fr_line,
		  _("A jump/branch instruction in delay slot."));
}

/* Actually output an instruction with its fixup.  */

static void
emit_insn (struct arc_insn *insn)
{
  char *f;
  int i;

  pr_debug ("Emit insn : 0x%x\n", insn->insn);
  pr_debug ("\tShort   : 0x%d\n", insn->short_insn);
  pr_debug ("\tLong imm: 0x%lx\n", insn->limm);

  /* Write out the instruction.  */
  if (insn->short_insn)
    {
      if (insn->has_limm)
	{
	  f = frag_more (6);
	  md_number_to_chars (f, insn->insn, 2);
	  md_number_to_chars_midend (f + 2, insn->limm, 4);
	  dwarf2_emit_insn (6);
	}
      else
	{
	  f = frag_more (2);
	  md_number_to_chars (f, insn->insn, 2);
	  dwarf2_emit_insn (2);
	}
    }
  else
    {
      if (insn->has_limm)
	{
	  f = frag_more (8);
	  md_number_to_chars_midend (f, insn->insn, 4);
	  md_number_to_chars_midend (f + 4, insn->limm, 4);
	  dwarf2_emit_insn (8);
	}
      else
	{
	  f = frag_more (4);
	  md_number_to_chars_midend (f, insn->insn, 4);
	  dwarf2_emit_insn (4);
	}
    }

  /* Apply the fixups in order.  */
  for (i = 0; i < insn->nfixups; i++)
    {
      struct arc_fixup *fixup = &insn->fixups[i];
      int size, pcrel, offset = 0;

      /*FIXME! the reloc size is wrong in the BFD file.  When it will
	be fixed please delete me.  */
      size = (insn->short_insn && !fixup->islong) ? 2 : 4;

      if (fixup->islong)
	offset = (insn->short_insn) ? 2 : 4;

      /* Some fixups are only used internally, thus no howto.  */
      if ((int) fixup->reloc < 0)
	{
	  /*FIXME! the reloc size is wrong in the BFD file.  When it
	    will be fixed please enable me.
	    size = (insn->short_insn && !fixup->islong) ? 2 : 4; */
	  pcrel = fixup->pcrel;
	}
      else
	{
	  reloc_howto_type *reloc_howto =
	    bfd_reloc_type_lookup (stdoutput,
				   (bfd_reloc_code_real_type) fixup->reloc);
	  gas_assert (reloc_howto);
	  /*FIXME! the reloc size is wrong in the BFD file.  When it
	    will be fixed please enable me.
	    size = bfd_get_reloc_size (reloc_howto); */
	  pcrel = reloc_howto->pc_relative;
	}

      pr_debug ("%s:%d: emit_insn: new %s fixup (PCrel:%s) of size %d @ offset %d\n",
		frag_now->fr_file, frag_now->fr_line,
		(fixup->reloc < 0) ? "Internal" :
		bfd_get_reloc_code_name (fixup->reloc),
		pcrel ? "Y" : "N",
		size, offset);
      fix_new_exp (frag_now, f - frag_now->fr_literal + offset,
		   size, &fixup->exp, pcrel, fixup->reloc);

      /* Check for ZOLs, and update symbol info if any.  */
      if (LP_INSN (insn->insn))
	{
	  gas_assert (fixup->exp.X_add_symbol);
	  ARC_SET_FLAG (fixup->exp.X_add_symbol, ARC_FLAG_ZOL);
	}
    }
}

/* Insert an operand value into an instruction.  */

static unsigned
insert_operand (unsigned insn,
		const struct arc_operand *operand,
		offsetT val,
		char *file,
		unsigned line)
{
  offsetT min = 0, max = 0;

  if (operand->bits != 32
      && !(operand->flags & ARC_OPERAND_NCHK)
      && !(operand->flags & ARC_OPERAND_FAKE))
    {
      if (operand->flags & ARC_OPERAND_SIGNED)
	{
	  max = (1 << (operand->bits - 1)) - 1;
	  min = -(1 << (operand->bits - 1));
	}
      else
	{
	  max = (1 << operand->bits) - 1;
	  min = 0;
	}

      if (val < min || val > max)
	as_bad_value_out_of_range (_("operand"),
				   val, min, max, file, line);
    }

  pr_debug ("insert field: %ld <= %ld <= %ld in 0x%08x\n",
	    min, val, max, insn);

  if ((operand->flags & ARC_OPERAND_ALIGNED32)
      && (val & 0x03))
    as_bad_where (file, line,
		  _("Unaligned operand. Needs to be 32bit aligned"));

  if ((operand->flags & ARC_OPERAND_ALIGNED16)
      && (val & 0x01))
    as_bad_where (file, line,
		  _("Unaligned operand. Needs to be 16bit aligned"));

  if (operand->insert)
    {
      const char *errmsg = NULL;

      insn = (*operand->insert) (insn, val, &errmsg);
      if (errmsg)
	as_warn_where (file, line, "%s", errmsg);
    }
  else
    {
      if (operand->flags & ARC_OPERAND_TRUNCATE)
	{
	  if (operand->flags & ARC_OPERAND_ALIGNED32)
	    val >>= 2;
	  if (operand->flags & ARC_OPERAND_ALIGNED16)
	    val >>= 1;
	}
      insn |= ((val & ((1 << operand->bits) - 1)) << operand->shift);
    }
  return insn;
}

void
arc_handle_align (fragS* fragP)
{
  if ((fragP)->fr_type == rs_align_code)
    {
      char *dest = (fragP)->fr_literal + (fragP)->fr_fix;
      valueT count = ((fragP)->fr_next->fr_address
		      - (fragP)->fr_address - (fragP)->fr_fix);

      (fragP)->fr_var = 2;

      if (count & 1)/* Padding in the gap till the next 2-byte
		       boundary with 0s.  */
	{
	  (fragP)->fr_fix++;
	  *dest++ = 0;
	}
      /* Writing nop_s.  */
      md_number_to_chars (dest, NOP_OPCODE_S, 2);
    }
}

/* Here we decide which fixups can be adjusted to make them relative
   to the beginning of the section instead of the symbol.  Basically
   we need to make sure that the dynamic relocations are done
   correctly, so in some cases we force the original symbol to be
   used.  */

int
tc_arc_fix_adjustable (fixS *fixP)
{

  /* Prevent all adjustments to global symbols.  */
  if (S_IS_EXTERNAL (fixP->fx_addsy))
    return 0;
  if (S_IS_WEAK (fixP->fx_addsy))
    return 0;

  /* Adjust_reloc_syms doesn't know about the GOT.  */
  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_ARC_GOTPC32:
    case BFD_RELOC_ARC_PLT32:
    case BFD_RELOC_ARC_S25H_PCREL_PLT:
    case BFD_RELOC_ARC_S21H_PCREL_PLT:
    case BFD_RELOC_ARC_S25W_PCREL_PLT:
    case BFD_RELOC_ARC_S21W_PCREL_PLT:
      return 0;

    default:
      break;
    }

  return 0; /* FIXME! return 1, fix it in the linker.  */
}

/* Compute the reloc type of an expression EXP.  */

static void
arc_check_reloc (expressionS *exp,
		 bfd_reloc_code_real_type *r_type_p)
{
  if (*r_type_p == BFD_RELOC_32
      && exp->X_op == O_subtract
      && exp->X_op_symbol != NULL
      && exp->X_op_symbol->bsym->section == now_seg)
    *r_type_p = BFD_RELOC_ARC_32_PCREL;
}


/* Add expression EXP of SIZE bytes to offset OFF of fragment FRAG.  */

void
arc_cons_fix_new (fragS *frag,
		  int off,
		  int size,
		  expressionS *exp,
		  bfd_reloc_code_real_type r_type)
{
  r_type = BFD_RELOC_UNUSED;

  switch (size)
    {
    case 1:
      r_type = BFD_RELOC_8;
      break;

    case 2:
      r_type = BFD_RELOC_16;
      break;

    case 3:
      r_type = BFD_RELOC_24;
      break;

    case 4:
      r_type = BFD_RELOC_32;
      arc_check_reloc (exp, &r_type);
      break;

    case 8:
      r_type = BFD_RELOC_64;
      break;

    default:
      as_bad (_("unsupported BFD relocation size %u"), size);
      r_type = BFD_RELOC_UNUSED;
    }

  fix_new_exp (frag, off, size, exp, 0, r_type);
}

/* The actual routine that checks the ZOL conditions.  */

static void
check_zol (symbolS *s)
{
  switch (arc_mach_type)
    {
    case bfd_mach_arc_arcv2:
      if (arc_target & ARC_OPCODE_ARCv2EM)
	return;

      if (is_br_jmp_insn_p (arc_last_insns[0].opcode)
	  || arc_last_insns[1].has_delay_slot)
	as_bad (_("Jump/Branch instruction detected at the end of the ZOL label @%s"),
		S_GET_NAME (s));

      break;
    case bfd_mach_arc_arc600:

      if (is_kernel_insn_p (arc_last_insns[0].opcode))
	as_bad (_("Kernel instruction detected at the end of the ZOL label @%s"),
		S_GET_NAME (s));

      if (arc_last_insns[0].has_limm
	  && is_br_jmp_insn_p (arc_last_insns[0].opcode))
	as_bad (_("A jump instruction with long immediate detected at the \
end of the ZOL label @%s"), S_GET_NAME (s));

      /* Fall through.  */
    case bfd_mach_arc_arc700:
      if (arc_last_insns[0].has_delay_slot)
	as_bad (_("An illegal use of delay slot detected at the end of the ZOL label @%s"),
		S_GET_NAME (s));

      break;
    default:
      break;
    }
}

/* If ZOL end check the last two instruction for illegals.  */
void
arc_frob_label (symbolS * sym)
{
  if (ARC_GET_FLAG (sym) & ARC_FLAG_ZOL)
    check_zol (sym);

  dwarf2_emit_label (sym);
}
