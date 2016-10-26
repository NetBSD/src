/* tc-ft32.c -- Assemble code for ft32
   Copyright (C) 2008-2015 Free Software Foundation, Inc.

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
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* Contributed by Anthony Green <green@spindazzle.org>.  */

#include "as.h"
#include "safe-ctype.h"
#include "opcode/ft32.h"

extern const ft32_opc_info_t ft32_opc_info[128];

const char comment_chars[]        = "#";
const char line_separator_chars[] = ";";
const char line_comment_chars[]   = "#";

static int pending_reloc;
static struct hash_control *opcode_hash_control;

static valueT md_chars_to_number (char * buf, int n);

const pseudo_typeS md_pseudo_table[] =
{
  {0, 0, 0}
};

const char FLT_CHARS[] = "rRsSfFdDxXpP";
const char EXP_CHARS[] = "eE";

/* This function is called once, at assembler startup time.  It sets
   up the hash table with all the opcodes in it, and also initializes
   some aliases for compatibility with other assemblers.  */

void
md_begin (void)
{
  const ft32_opc_info_t *opcode;
  opcode_hash_control = hash_new ();

  /* Insert names into hash table.  */
  for (opcode = ft32_opc_info; opcode->name; opcode++)
    hash_insert (opcode_hash_control, opcode->name, (char *) opcode);

  bfd_set_arch_mach (stdoutput, TARGET_ARCH, 0);
}

/* Parse an expression and then restore the input line pointer.  */

static char *
parse_exp_save_ilp (char *s, expressionS *op)
{
  char *save = input_line_pointer;

  input_line_pointer = s;
  expression (op);
  s = input_line_pointer;
  input_line_pointer = save;
  return s;
}

static int
parse_condition (char **ptr)
{
  char *s = *ptr;
  static const struct {
    char *name;
    int bits;
  } ccs[] = {
    { "gt,"   , (2 << FT32_FLD_CR_BIT) | (5 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "gte,"  , (2 << FT32_FLD_CR_BIT) | (4 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "lt,"   , (2 << FT32_FLD_CR_BIT) | (4 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "lte,"  , (2 << FT32_FLD_CR_BIT) | (5 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "a,"    , (2 << FT32_FLD_CR_BIT) | (6 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "ae,"   , (2 << FT32_FLD_CR_BIT) | (1 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "be,"   , (2 << FT32_FLD_CR_BIT) | (6 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "b,"    , (2 << FT32_FLD_CR_BIT) | (1 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "nz,"   , (2 << FT32_FLD_CR_BIT) | (0 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "z,"    , (2 << FT32_FLD_CR_BIT) | (0 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "nc,"   , (2 << FT32_FLD_CR_BIT) | (1 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "c,"    , (2 << FT32_FLD_CR_BIT) | (1 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "no,"   , (2 << FT32_FLD_CR_BIT) | (2 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "o,"    , (2 << FT32_FLD_CR_BIT) | (2 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { "ns,"   , (2 << FT32_FLD_CR_BIT) | (3 << FT32_FLD_CB_BIT) | (0 << FT32_FLD_CV_BIT)},
    { "s,"    , (2 << FT32_FLD_CR_BIT) | (3 << FT32_FLD_CB_BIT) | (1 << FT32_FLD_CV_BIT)},
    { NULL, 0}
  }, *pc;

  for (pc = ccs; pc->name; pc++)
    {
      if (memcmp(pc->name, s, strlen(pc->name)) == 0)
        {
          *ptr += strlen(pc->name) - 1;
          return pc->bits;
        }
    }
  return -1;
}

static int
parse_decimal (char **ptr)
{
  int r = 0;
  char *s = *ptr;

  while (('0' <= *s) && (*s <= '9'))
    {
      r *= 10;
      r += (*s++ - '0');
    }
  *ptr = s;
  return r;
}

static int
parse_register_operand (char **ptr)
{
  int reg;
  char *s = *ptr;

  if (*s != '$')
    {
      as_bad (_("expecting register"));
      ignore_rest_of_line ();
      return -1;
    }
  if ((s[1] == 's') && (s[2] == 'p'))
    {
      reg = 31;
    }
  else if ((s[1] == 'c') && (s[2] == 'c'))
    {
      reg = 30;
    }
  else if ((s[1] == 'f') && (s[2] == 'p'))
    {
      reg = 29;
    }
  else if (s[1] == 'r')
    {
      reg = s[2] - '0';
      if ((reg < 0) || (reg > 9))
	{
	  as_bad (_("illegal register number"));
	  ignore_rest_of_line ();
	  return -1;
	}
      if ((reg == 1) || (reg == 2) || (reg == 3))
	{
	  int r2 = s[3] - '0';
	  if ((r2 >= 0) && (r2 <= 9))
	    {
	      reg = (reg * 10) + r2;
	      *ptr += 1;
	    }
	}
    }
  else
    {
      as_bad (_("illegal register number"));
      ignore_rest_of_line ();
      return -1;
    }

  *ptr += 3;

  return reg;
}

/* This is the guts of the machine-dependent assembler.  STR points to
   a machine dependent instruction.  This function is supposed to emit
   the frags/bytes it assembles to.  */

void
md_assemble (char *str)
{
  char *op_start;
  char *op_end;

  ft32_opc_info_t *opcode;
  char *output;
  int idx = 0;
  char pend;

  int nlen = 0;

  unsigned int b;
  int f;

  expressionS arg;

  /* Drop leading whitespace.  */
  while (*str == ' ')
    str++;

  /* Find the op code end.  */
  op_start = str;
  for (op_end = str;
       *op_end && !is_end_of_line[*op_end & 0xff] && *op_end != ' ' && *op_end != '.';
       op_end++)
    nlen++;

  pend = *op_end;
  *op_end = 0;

  if (nlen == 0)
    as_bad (_("can't find opcode "));

  opcode = (ft32_opc_info_t *) hash_find (opcode_hash_control, op_start);
  *op_end = pend;

  if (opcode == NULL)
    {
      as_bad (_("unknown opcode %s"), op_start);
      return;
    }

  b = opcode->bits;
  f = opcode->fields;

  if (opcode->dw)
    {
      int dw;
      if (*op_end == '.')
        {
          switch (op_end[1])
            {
              case 'b':
                dw = 0;
                break;
              case 's':
                dw = 1;
                break;
              case 'l':
                dw = 2;
                break;
              default:
                as_bad (_("unknown width specifier '.%c'"), op_end[1]);
                return;
            }
          op_end += 2;
        }
      else
        {
          dw = 2; /* default is ".l" */
        }
      b |= dw << FT32_FLD_DW_BIT;
    }

  while (ISSPACE (*op_end))
    op_end++;

  output = frag_more (4);

  while (f)
    {
      int lobit = f & -f;
      if (f & lobit)
        {
          switch (lobit)
          {
          case  FT32_FLD_CBCRCV:
            b |= parse_condition( &op_end);
            break;
          case  FT32_FLD_CB:
            b |= parse_decimal (&op_end) << FT32_FLD_CB_BIT;
            break;
          case  FT32_FLD_R_D:
            b |= parse_register_operand (&op_end) << FT32_FLD_R_D_BIT;
            break;
          case  FT32_FLD_CR:
            b |= (parse_register_operand (&op_end) - 28) << FT32_FLD_CR_BIT;
            break;
          case  FT32_FLD_CV:
            b |= parse_decimal (&op_end) << FT32_FLD_CV_BIT;
            break;
          case  FT32_FLD_R_1:
            b |= parse_register_operand (&op_end) << FT32_FLD_R_1_BIT;
            break;
          case  FT32_FLD_RIMM:
            if (*op_end == '$')
              {
                b |= parse_register_operand (&op_end) << FT32_FLD_RIMM_BIT;
              }
            else
              {
                b |= 0x400 << FT32_FLD_RIMM_BIT;
                op_end = parse_exp_save_ilp (op_end, &arg);
                fix_new_exp (frag_now,
                             (output - frag_now->fr_literal),
                             2,
                             &arg,
                             0,
                             BFD_RELOC_FT32_10);
              }
            break;
          case  FT32_FLD_R_2:
            b |= parse_register_operand (&op_end) << FT32_FLD_R_2_BIT;
            break;
          case  FT32_FLD_K20:
            op_end = parse_exp_save_ilp (op_end, &arg);
            fix_new_exp (frag_now,
                         (output - frag_now->fr_literal),
                         3,
                         &arg,
                         0,
                         BFD_RELOC_FT32_20);
            break;
          case  FT32_FLD_PA:
            op_end = parse_exp_save_ilp (op_end, &arg);
            fix_new_exp (frag_now,
                         (output - frag_now->fr_literal),
                         3,
                         &arg,
                         0,
                         BFD_RELOC_FT32_18);
            break;
          case  FT32_FLD_AA:
            op_end = parse_exp_save_ilp (op_end, &arg);
            fix_new_exp (frag_now,
                         (output - frag_now->fr_literal),
                         3,
                         &arg,
                         0,
                         BFD_RELOC_FT32_17);
            break;
          case  FT32_FLD_K16:
            op_end = parse_exp_save_ilp (op_end, &arg);
            fix_new_exp (frag_now,
                         (output - frag_now->fr_literal),
                         2,
                         &arg,
                         0,
                         BFD_RELOC_16);
            break;
          case  FT32_FLD_K8:
            op_end = parse_exp_save_ilp (op_end, &arg);
            fix_new_exp (frag_now,
                         (output - frag_now->fr_literal),
                         1,
                         &arg,
                         0,
                         BFD_RELOC_8);
            break;
          case  FT32_FLD_R_D_POST:
            b |= parse_register_operand (&op_end) << FT32_FLD_R_D_BIT;
            break;
          case  FT32_FLD_R_1_POST:
            b |= parse_register_operand (&op_end) << FT32_FLD_R_1_BIT;
            break;
          default:
            as_bad (_("internal error in argument parsing"));
            break;
          }
          f &= ~lobit;
          if (f)
            {
              while (ISSPACE (*op_end))
                op_end++;

              if (*op_end != ',')
                {
                  as_bad (_("expected comma separator"));
                  ignore_rest_of_line ();
                }

              op_end++;
              while (ISSPACE (*op_end))
                op_end++;
            }
        }
    }
  if (*op_end != 0)
    as_warn (_("extra stuff on line ignored"));

  output[idx++] = 0xff & (b >> 0);
  output[idx++] = 0xff & (b >> 8);
  output[idx++] = 0xff & (b >> 16);
  output[idx++] = 0xff & (b >> 24);

  while (ISSPACE (*op_end))
    op_end++;

  if (*op_end != 0)
    as_warn ("extra stuff on line ignored");

  if (pending_reloc)
    as_bad ("Something forgot to clean up\n");
}

/* Turn a string in input_line_pointer into a floating point constant
   of type type, and store the appropriate bytes in *LITP.  The number
   of LITTLENUMS emitted is stored in *SIZEP .  An error message is
   returned, or NULL on OK.  */

char *
md_atof (int type, char *litP, int *sizeP)
{
  int prec;
  LITTLENUM_TYPE words[4];
  char *t;
  int i;

  switch (type)
    {
    case 'f':
      prec = 2;
      break;

    case 'd':
      prec = 4;
      break;

    default:
      *sizeP = 0;
      return _("bad call to md_atof");
    }

  t = atof_ieee (input_line_pointer, type, words);
  if (t)
    input_line_pointer = t;

  *sizeP = prec * 2;

  for (i = prec - 1; i >= 0; i--)
    {
      md_number_to_chars (litP, (valueT) words[i], 2);
      litP += 2;
    }

  return NULL;
}

const char *md_shortopts = "";

struct option md_longopts[] =
{
  {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

/* We have no target specific options yet, so these next
   two functions are empty.  */
int
md_parse_option (int c ATTRIBUTE_UNUSED, char *arg ATTRIBUTE_UNUSED)
{
  return 0;
}

void
md_show_usage (FILE *stream ATTRIBUTE_UNUSED)
{
}

/* Convert from target byte order to host byte order.  */

static valueT
md_chars_to_number (char * buf, int n)
{
  valueT result = 0;
  unsigned char * where = (unsigned char *) buf;

  while (n--)
    {
      result <<= 8;
      result |= (where[n] & 255);
    }

  return result;
}
/* Apply a fixup to the object file.  */

void
md_apply_fix (fixS *fixP ATTRIBUTE_UNUSED,
	      valueT * valP ATTRIBUTE_UNUSED, segT seg ATTRIBUTE_UNUSED)
{
  char *buf = fixP->fx_where + fixP->fx_frag->fr_literal;
  long val = *valP;
  long newval;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_32:
      buf[3] = val >> 24;
      buf[2] = val >> 16;
      buf[1] = val >> 8;
      buf[0] = val >> 0;
      break;

    case BFD_RELOC_16:
      buf[1] = val >> 8;
      buf[0] = val >> 0;
      break;

    case BFD_RELOC_8:
      *buf = val;
      break;

    case BFD_RELOC_FT32_10:
      if (!val)
	break;
      newval = md_chars_to_number (buf, 2);
      newval |= (val & ((1 << 10) - 1)) << FT32_FLD_RIMM_BIT;
      md_number_to_chars (buf, newval, 2);
      break;

    case BFD_RELOC_FT32_20:
      if (!val)
	break;
      newval = md_chars_to_number (buf, 3);
      newval |= val & ((1 << 20) - 1);
      md_number_to_chars (buf, newval, 3);
      break;

    case BFD_RELOC_FT32_17:
      if (!val)
	break;
      newval = md_chars_to_number (buf, 3);
      newval |= val & ((1 << 17) - 1);
      md_number_to_chars (buf, newval, 3);
      break;

    case BFD_RELOC_FT32_18:
      if (!val)
	break;
      newval = md_chars_to_number (buf, 4);
      newval |= (val >> 2) & ((1 << 18) - 1);
      md_number_to_chars (buf, newval, 4);
      break;

    default:
      abort ();
    }

  if (fixP->fx_addsy == NULL && fixP->fx_pcrel == 0)
    fixP->fx_done = 1;
  // printf("fx_addsy=%p fixP->fx_pcrel=%d fx_done=%d\n", fixP->fx_addsy, fixP->fx_pcrel, fixP->fx_done);
}

void
md_number_to_chars (char *ptr, valueT use, int nbytes)
{
  number_to_chars_littleendian (ptr, use, nbytes);
}

/* Generate a machine-dependent relocation.  */
arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixP)
{
  arelent *relP;
  bfd_reloc_code_real_type code;

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_32:
    case BFD_RELOC_16:
    case BFD_RELOC_8:
    case BFD_RELOC_FT32_10:
    case BFD_RELOC_FT32_20:
    case BFD_RELOC_FT32_17:
    case BFD_RELOC_FT32_18:
      code = fixP->fx_r_type;
      break;
    default:
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("Semantics error.  This type of operand can not be relocated, it must be an assembly-time constant"));
      return 0;
    }

  relP = xmalloc (sizeof (arelent));
  gas_assert (relP != 0);
  relP->sym_ptr_ptr = xmalloc (sizeof (asymbol *));
  *relP->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);
  relP->address = fixP->fx_frag->fr_address + fixP->fx_where;

  relP->addend = fixP->fx_offset;

  relP->howto = bfd_reloc_type_lookup (stdoutput, code);
  if (! relP->howto)
    {
      const char *name;

      name = S_GET_NAME (fixP->fx_addsy);
      if (name == NULL)
	name = _("<unknown>");
      as_fatal (_("Cannot generate relocation type for symbol %s, code %s"),
		name, bfd_get_reloc_code_name (code));
    }

  return relP;
}
