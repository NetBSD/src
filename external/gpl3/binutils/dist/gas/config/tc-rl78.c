/* tc-rl78.c -- Assembler for the Renesas RL78
   Copyright 2011
   Free Software Foundation, Inc.

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
#include "struc-symbol.h"
#include "obstack.h"
#include "safe-ctype.h"
#include "dwarf2dbg.h"
#include "libbfd.h"
#include "elf/common.h"
#include "elf/rl78.h"
#include "rl78-defs.h"
#include "filenames.h"
#include "listing.h"
#include "sb.h"
#include "macro.h"

const char comment_chars[]        = ";";
/* Note that input_file.c hand checks for '#' at the beginning of the
   first line of the input file.  This is because the compiler outputs
   #NO_APP at the beginning of its output.  */
const char line_comment_chars[]   = "#";
const char line_separator_chars[] = "|";

const char EXP_CHARS[]            = "eE";
const char FLT_CHARS[]            = "dD";

/*------------------------------------------------------------------*/

char * rl78_lex_start;
char * rl78_lex_end;

typedef struct rl78_bytesT
{
  char prefix[1];
  int n_prefix;
  char base[4];
  int n_base;
  char ops[8];
  int n_ops;
  struct
  {
    expressionS  exp;
    char         offset;
    char         nbits;
    char         type; /* RL78REL_*.  */
    int          reloc;
    fixS *       fixP;
  } fixups[2];
  int n_fixups;
  struct
  {
    char type;
    char field_pos;
    char val_ofs;
  } relax[2];
  int n_relax;
  int link_relax;
  fixS *link_relax_fixP;
  char times_grown;
  char times_shrank;
} rl78_bytesT;

static rl78_bytesT rl78_bytes;

void
rl78_linkrelax_addr16 (void)
{
  rl78_bytes.link_relax |= RL78_RELAXA_ADDR16;
}

void
rl78_linkrelax_branch (void)
{
  rl78_bytes.link_relax |= RL78_RELAXA_BRA;
}

static void
rl78_fixup (expressionS exp, int offsetbits, int nbits, int type)
{
  rl78_bytes.fixups[rl78_bytes.n_fixups].exp = exp;
  rl78_bytes.fixups[rl78_bytes.n_fixups].offset = offsetbits;
  rl78_bytes.fixups[rl78_bytes.n_fixups].nbits = nbits;
  rl78_bytes.fixups[rl78_bytes.n_fixups].type = type;
  rl78_bytes.fixups[rl78_bytes.n_fixups].reloc = exp.X_md;
  rl78_bytes.n_fixups ++;
}

#define rl78_field_fixup(exp, offset, nbits, type)	\
  rl78_fixup (exp, offset + 8 * rl78_bytes.n_prefix), nbits, type)

#define rl78_op_fixup(exp, offset, nbits, type)		\
  rl78_fixup (exp, offset + 8 * (rl78_bytes.n_prefix + rl78_bytes.n_base), nbits, type)

void
rl78_prefix (int p)
{
  rl78_bytes.prefix[0] = p;
  rl78_bytes.n_prefix = 1;
}

int
rl78_has_prefix ()
{
  return rl78_bytes.n_prefix;
}

void
rl78_base1 (int b1)
{
  rl78_bytes.base[0] = b1;
  rl78_bytes.n_base = 1;
}

void
rl78_base2 (int b1, int b2)
{
  rl78_bytes.base[0] = b1;
  rl78_bytes.base[1] = b2;
  rl78_bytes.n_base = 2;
}

void
rl78_base3 (int b1, int b2, int b3)
{
  rl78_bytes.base[0] = b1;
  rl78_bytes.base[1] = b2;
  rl78_bytes.base[2] = b3;
  rl78_bytes.n_base = 3;
}

void
rl78_base4 (int b1, int b2, int b3, int b4)
{
  rl78_bytes.base[0] = b1;
  rl78_bytes.base[1] = b2;
  rl78_bytes.base[2] = b3;
  rl78_bytes.base[3] = b4;
  rl78_bytes.n_base = 4;
}

#define F_PRECISION 2

void
rl78_op (expressionS exp, int nbytes, int type)
{
  int v = 0;

  if ((exp.X_op == O_constant || exp.X_op == O_big)
      && type != RL78REL_PCREL)
    {
      if (exp.X_op == O_big && exp.X_add_number <= 0)
	{
	  LITTLENUM_TYPE w[2];
	  char * ip = rl78_bytes.ops + rl78_bytes.n_ops;

	  gen_to_words (w, F_PRECISION, 8);
	  ip[3] = w[0] >> 8;
	  ip[2] = w[0];
	  ip[1] = w[1] >> 8;
	  ip[0] = w[1];
	  rl78_bytes.n_ops += 4;
	}
      else
	{
	  v = exp.X_add_number;
	  while (nbytes)
	    {
	      rl78_bytes.ops[rl78_bytes.n_ops++] =v & 0xff;
	      v >>= 8;
	      nbytes --;
	    }
	}
    }
  else
    {
      rl78_op_fixup (exp, rl78_bytes.n_ops * 8, nbytes * 8, type);
      memset (rl78_bytes.ops + rl78_bytes.n_ops, 0, nbytes);
      rl78_bytes.n_ops += nbytes;
    }
}

/* This gets complicated when the field spans bytes, because fields
   are numbered from the MSB of the first byte as zero, and bits are
   stored LSB towards the LSB of the byte.  Thus, a simple four-bit
   insertion of 12 at position 4 of 0x00 yields: 0x0b.  A three-bit
   insertion of b'MXL at position 7 is like this:

     - - - -  - - - -   - - - -  - - - -
                    M   X L               */

void
rl78_field (int val, int pos, int sz)
{
  int valm;
  int bytep, bitp;

  if (sz > 0)
    {
      if (val < 0 || val >= (1 << sz))
	as_bad (_("Value %d doesn't fit in unsigned %d-bit field"), val, sz);
    }
  else
    {
      sz = - sz;
      if (val < -(1 << (sz - 1)) || val >= (1 << (sz - 1)))
	as_bad (_("Value %d doesn't fit in signed %d-bit field"), val, sz);
    }

  /* This code points at 'M' in the above example.  */
  bytep = pos / 8;
  bitp = pos % 8;

  while (bitp + sz > 8)
    {
      int ssz = 8 - bitp;
      int svalm;

      svalm = val >> (sz - ssz);
      svalm = svalm & ((1 << ssz) - 1);
      svalm = svalm << (8 - bitp - ssz);
      gas_assert (bytep < rl78_bytes.n_base);
      rl78_bytes.base[bytep] |= svalm;

      bitp = 0;
      sz -= ssz;
      bytep ++;
    }
  valm = val & ((1 << sz) - 1);
  valm = valm << (8 - bitp - sz);
  gas_assert (bytep < rl78_bytes.n_base);
  rl78_bytes.base[bytep] |= valm;
}

/*------------------------------------------------------------------*/

enum options
{
  OPTION_RELAX = OPTION_MD_BASE,
};

#define RL78_SHORTOPTS ""
const char * md_shortopts = RL78_SHORTOPTS;

/* Assembler options.  */
struct option md_longopts[] =
{
  {"relax", no_argument, NULL, OPTION_RELAX},
  {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c, char * arg ATTRIBUTE_UNUSED)
{
  switch (c)
    {
    case OPTION_RELAX:
      linkrelax = 1;
      return 1;

    }
  return 0;
}

void
md_show_usage (FILE * stream ATTRIBUTE_UNUSED)
{
}


static void
s_bss (int ignore ATTRIBUTE_UNUSED)
{
  int temp;

  temp = get_absolute_expression ();
  subseg_set (bss_section, (subsegT) temp);
  demand_empty_rest_of_line ();
}

/* The target specific pseudo-ops which we support.  */
const pseudo_typeS md_pseudo_table[] =
{
  /* Our "standard" pseudos. */
  { "double",   float_cons,    'd' },
  { "bss",	s_bss, 		0 },
  { "3byte",	cons,		3 },
  { "int",	cons,		4 },
  { "word",	cons,		4 },

  /* End of list marker.  */
  { NULL, 	NULL, 		0 }
};

void
md_begin (void)
{
}

void
rl78_md_end (void)
{
}

/* Write a value out to the object file, using the appropriate endianness.  */
void
md_number_to_chars (char * buf, valueT val, int n)
{
  number_to_chars_littleendian (buf, val, n);
}

static struct
{
  char * fname;
  int    reloc;
}
reloc_functions[] =
{
  { "lo16", BFD_RELOC_RL78_LO16 },
  { "hi16", BFD_RELOC_RL78_HI16 },
  { "hi8",  BFD_RELOC_RL78_HI8 },
  { 0, 0 }
};

void
md_operand (expressionS * exp ATTRIBUTE_UNUSED)
{
  int reloc = 0;
  int i;

  for (i = 0; reloc_functions[i].fname; i++)
    {
      int flen = strlen (reloc_functions[i].fname);

      if (input_line_pointer[0] == '%'
	  && strncasecmp (input_line_pointer + 1, reloc_functions[i].fname, flen) == 0
	  && input_line_pointer[flen + 1] == '(')
	{
	  reloc = reloc_functions[i].reloc;
	  input_line_pointer += flen + 2;
	  break;
	}
    }
  if (reloc == 0)
    return;

  expression (exp);
  if (* input_line_pointer == ')')
    input_line_pointer ++;

  exp->X_md = reloc;
}

void
rl78_frag_init (fragS * fragP)
{
  if (rl78_bytes.n_relax || rl78_bytes.link_relax)
    {
      fragP->tc_frag_data = malloc (sizeof (rl78_bytesT));
      memcpy (fragP->tc_frag_data, & rl78_bytes, sizeof (rl78_bytesT));
    }
  else
    fragP->tc_frag_data = 0;
}

/* When relaxing, we need to output a reloc for any .align directive
   so that we can retain this alignment as we adjust opcode sizes.  */
void
rl78_handle_align (fragS * frag)
{
  if (linkrelax
      && (frag->fr_type == rs_align
	  || frag->fr_type == rs_align_code)
      && frag->fr_address + frag->fr_fix > 0
      && frag->fr_offset > 0
      && now_seg != bss_section)
    {
      fix_new (frag, frag->fr_fix, 0,
	       &abs_symbol, RL78_RELAXA_ALIGN + frag->fr_offset,
	       0, BFD_RELOC_RL78_RELAX);
      /* For the purposes of relaxation, this relocation is attached
	 to the byte *after* the alignment - i.e. the byte that must
	 remain aligned.  */
      fix_new (frag->fr_next, 0, 0,
	       &abs_symbol, RL78_RELAXA_ELIGN + frag->fr_offset,
	       0, BFD_RELOC_RL78_RELAX);
    }
}

char *
md_atof (int type, char * litP, int * sizeP)
{
  return ieee_md_atof (type, litP, sizeP, target_big_endian);
}

symbolS *
md_undefined_symbol (char * name ATTRIBUTE_UNUSED)
{
  return NULL;
}

#define APPEND(B, N_B)				       \
  if (rl78_bytes.N_B)				       \
    {						       \
      memcpy (bytes + idx, rl78_bytes.B, rl78_bytes.N_B);  \
      idx += rl78_bytes.N_B;			       \
    }


void
md_assemble (char * str)
{
  char * bytes;
  fragS * frag_then = frag_now;
  int idx = 0;
  int i;
  int rel;
  expressionS  *exp;

  /*printf("\033[32mASM: %s\033[0m\n", str);*/

  dwarf2_emit_insn (0);

  memset (& rl78_bytes, 0, sizeof (rl78_bytes));

  rl78_lex_init (str, str + strlen (str));

  rl78_parse ();

  /* This simplifies the relaxation code.  */
  if (rl78_bytes.link_relax)
    {
      int olen = rl78_bytes.n_prefix + rl78_bytes.n_base + rl78_bytes.n_ops;
      /* We do it this way because we want the frag to have the
	 rl78_bytes in it, which we initialize above.  */
      bytes = frag_more (olen);
      frag_then = frag_now;
      frag_variant (rs_machine_dependent,
		    olen /* max_chars */,
		    0 /* var */,
		    olen /* subtype */,
		    0 /* symbol */,
		    0 /* offset */,
		    0 /* opcode */);
      frag_then->fr_opcode = bytes;
      frag_then->fr_fix = olen + (bytes - frag_then->fr_literal);
      frag_then->fr_subtype = olen;
      frag_then->fr_var = 0;
    }
  else
    {
      bytes = frag_more (rl78_bytes.n_prefix + rl78_bytes.n_base + rl78_bytes.n_ops);
      frag_then = frag_now;
    }

  APPEND (prefix, n_prefix);
  APPEND (base, n_base);
  APPEND (ops, n_ops);

  if (rl78_bytes.link_relax)
    {
      fixS * f;

      f = fix_new (frag_then,
		   (char *) bytes - frag_then->fr_literal,
		   0,
		   abs_section_sym,
		   rl78_bytes.link_relax | rl78_bytes.n_fixups,
		   0,
		   BFD_RELOC_RL78_RELAX);
      frag_then->tc_frag_data->link_relax_fixP = f;
    }

  for (i = 0; i < rl78_bytes.n_fixups; i ++)
    {
      /* index: [nbytes][type] */
      static int reloc_map[5][4] =
	{
	  { 0,            0 },
	  { BFD_RELOC_8,  BFD_RELOC_8_PCREL },
	  { BFD_RELOC_16, BFD_RELOC_16_PCREL },
	  { BFD_RELOC_24, BFD_RELOC_24_PCREL },
	  { BFD_RELOC_32, BFD_RELOC_32_PCREL },
	};
      fixS * f;

      idx = rl78_bytes.fixups[i].offset / 8;
      rel = reloc_map [rl78_bytes.fixups[i].nbits / 8][(int) rl78_bytes.fixups[i].type];

      if (rl78_bytes.fixups[i].reloc)
	rel = rl78_bytes.fixups[i].reloc;

      if (frag_then->tc_frag_data)
	exp = & frag_then->tc_frag_data->fixups[i].exp;
      else
	exp = & rl78_bytes.fixups[i].exp;

      f = fix_new_exp (frag_then,
		       (char *) bytes + idx - frag_then->fr_literal,
		       rl78_bytes.fixups[i].nbits / 8,
		       exp,
		       rl78_bytes.fixups[i].type == RL78REL_PCREL ? 1 : 0,
		       rel);
      if (frag_then->tc_frag_data)
	frag_then->tc_frag_data->fixups[i].fixP = f;
    }
}

void
rl78_cons_fix_new (fragS *	frag,
		 int		where,
		 int		size,
		 expressionS *  exp)
{
  bfd_reloc_code_real_type type;

  switch (size)
    {
    case 1:
      type = BFD_RELOC_8;
      break;
    case 2:
      type = BFD_RELOC_16;
      break;
    case 3:
      type = BFD_RELOC_24;
      break;
    case 4:
      type = BFD_RELOC_32;
      break;
    default:
      as_bad (_("unsupported constant size %d\n"), size);
      return;
    }

  if (exp->X_op == O_subtract && exp->X_op_symbol)
    {
      if (size != 4 && size != 2 && size != 1)
	as_bad (_("difference of two symbols only supported with .long, .short, or .byte"));
      else
	type = BFD_RELOC_RL78_DIFF;
    }

  fix_new_exp (frag, where, (int) size, exp, 0, type);
}

/* No relaxation just yet */
int
md_estimate_size_before_relax (fragS * fragP ATTRIBUTE_UNUSED, segT segment ATTRIBUTE_UNUSED)
{
  return 0;
}

arelent **
tc_gen_reloc (asection * seg ATTRIBUTE_UNUSED, fixS * fixp)
{
  static arelent * reloc[8];
  int rp;

  if (fixp->fx_r_type == BFD_RELOC_NONE)
    {
      reloc[0] = NULL;
      return reloc;
    }

  if (fixp->fx_subsy
      && S_GET_SEGMENT (fixp->fx_subsy) == absolute_section)
    {
      fixp->fx_offset -= S_GET_VALUE (fixp->fx_subsy);
      fixp->fx_subsy = NULL;
    }

  reloc[0]		  = (arelent *) xmalloc (sizeof (arelent));
  reloc[0]->sym_ptr_ptr   = (asymbol **) xmalloc (sizeof (asymbol *));
  * reloc[0]->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc[0]->address       = fixp->fx_frag->fr_address + fixp->fx_where;
  reloc[0]->addend        = fixp->fx_offset;

  if (fixp->fx_r_type == BFD_RELOC_RL78_32_OP
      && fixp->fx_subsy)
    {
      fixp->fx_r_type = BFD_RELOC_RL78_DIFF;
    }

#define OPX(REL,SYM,ADD)							\
  reloc[rp]		   = (arelent *) xmalloc (sizeof (arelent));		\
  reloc[rp]->sym_ptr_ptr   = (asymbol **) xmalloc (sizeof (asymbol *));		\
  reloc[rp]->howto         = bfd_reloc_type_lookup (stdoutput, REL);		\
  reloc[rp]->addend        = ADD;						\
  * reloc[rp]->sym_ptr_ptr = SYM;						\
  reloc[rp]->address       = fixp->fx_frag->fr_address + fixp->fx_where;	\
  reloc[++rp] = NULL
#define OPSYM(SYM) OPX(BFD_RELOC_RL78_SYM, SYM, 0)
#define OPIMM(IMM) OPX(BFD_RELOC_RL78_SYM, abs_symbol.bsym, IMM)
#define OP(OP) OPX(BFD_RELOC_RL78_##OP, *reloc[0]->sym_ptr_ptr, 0)
#define SYM0() reloc[0]->howto = bfd_reloc_type_lookup (stdoutput, BFD_RELOC_RL78_SYM)

  rp = 1;

  /* Certain BFD relocations cannot be translated directly into
     a single (non-Red Hat) RL78 relocation, but instead need
     multiple RL78 relocations - handle them here.  */
  switch (fixp->fx_r_type)
    {
    case BFD_RELOC_RL78_DIFF:
      SYM0 ();
      OPSYM (symbol_get_bfdsym (fixp->fx_subsy));
      OP(OP_SUBTRACT);

      switch (fixp->fx_size)
	{
	case 1:
	  OP(ABS8);
	  break;
	case 2:
	  OP (ABS16);
	  break;
	case 4:
	  OP (ABS32);
	  break;
	}
      break;

    case BFD_RELOC_RL78_NEG32:
      SYM0 ();
      OP (OP_NEG);
      OP (ABS32);
      break;

    case BFD_RELOC_RL78_LO16:
      SYM0 ();
      OPIMM (0xffff);
      OP (OP_AND);
      OP (ABS16);
      break;

    case BFD_RELOC_RL78_HI16:
      SYM0 ();
      OPIMM (16);
      OP (OP_SHRA);
      OP (ABS16);
      break;

    case BFD_RELOC_RL78_HI8:
      SYM0 ();
      OPIMM (16);
      OP (OP_SHRA);
      OPIMM (0xff);
      OP (OP_AND);
      OP (ABS8);
      break;

    default:
      reloc[0]->howto = bfd_reloc_type_lookup (stdoutput, fixp->fx_r_type);
      reloc[1] = NULL;
      break;
    }

  return reloc;
}

int
rl78_validate_fix_sub (struct fix * f)
{
  /* We permit the subtraction of two symbols in a few cases.  */
  /* mov #sym1-sym2, R3 */
  if (f->fx_r_type == BFD_RELOC_RL78_32_OP)
    return 1;
  /* .long sym1-sym2 */
  if (f->fx_r_type == BFD_RELOC_RL78_DIFF
      && ! f->fx_pcrel
      && (f->fx_size == 4 || f->fx_size == 2 || f->fx_size == 1))
    return 1;
  return 0;
}

long
md_pcrel_from_section (fixS * fixP, segT sec)
{
  long rv;

  if (fixP->fx_addsy != NULL
      && (! S_IS_DEFINED (fixP->fx_addsy)
	  || S_GET_SEGMENT (fixP->fx_addsy) != sec))
    /* The symbol is undefined (or is defined but not in this section).
       Let the linker figure it out.  */
    return 0;

  rv = fixP->fx_frag->fr_address + fixP->fx_where;
  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_8_PCREL:
      rv += 1;
      break;
    case BFD_RELOC_16_PCREL:
      rv += 2;
      break;
    default:
      break;
    }
  return rv;
}

void
md_apply_fix (struct fix * f ATTRIBUTE_UNUSED,
	      valueT *     t ATTRIBUTE_UNUSED,
	      segT         s ATTRIBUTE_UNUSED)
{
  char * op;
  unsigned long val;

  if (f->fx_addsy && S_FORCE_RELOC (f->fx_addsy, 1))
    return;
  if (f->fx_subsy && S_FORCE_RELOC (f->fx_subsy, 1))
    return;

  op = f->fx_frag->fr_literal + f->fx_where;
  val = (unsigned long) * t;

  switch (f->fx_r_type)
    {
    case BFD_RELOC_NONE:
      break;

    case BFD_RELOC_RL78_RELAX:
      f->fx_done = 1;
      break;

    case BFD_RELOC_8:
    case BFD_RELOC_8_PCREL:
      op[0] = val;
      break;

    case BFD_RELOC_16:
    case BFD_RELOC_16_PCREL:
      op[0] = val;
      op[1] = val >> 8;
      break;

    case BFD_RELOC_24:
      op[0] = val;
      op[1] = val >> 8;
      op[2] = val >> 16;
      break;

    case BFD_RELOC_32:
    case BFD_RELOC_RL78_DIFF:
      op[0] = val;
      op[1] = val >> 8;
      op[2] = val >> 16;
      op[3] = val >> 24;
      break;

    default:
      as_bad (_("Unknown reloc in md_apply_fix: %s"),
	      bfd_get_reloc_code_name (f->fx_r_type));
      break;
    }

  if (f->fx_addsy == NULL)
    f->fx_done = 1;
}

valueT
md_section_align (segT segment, valueT size)
{
  int align = bfd_get_section_alignment (stdoutput, segment);
  return ((size + (1 << align) - 1) & (-1 << align));
}

void
md_convert_frag (bfd *   abfd ATTRIBUTE_UNUSED,
		 segT    segment ATTRIBUTE_UNUSED,
		 fragS * fragP ATTRIBUTE_UNUSED)
{
  /* No relaxation yet */
  fragP->fr_var = 0;
}
