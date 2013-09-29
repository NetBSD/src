/* tc-i386.c -- Assemble Intel syntax code for ix86/x86-64
   Copyright 2009, 2010
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

static struct
  {
    operatorT op_modifier;	/* Operand modifier.  */
    int is_mem;			/* 1 if operand is memory reference.  */
    int is_indirect;		/* 1 if operand is indirect reference.  */
    int has_offset;		/* 1 if operand has offset.  */
    unsigned int in_offset;	/* >=1 if processing operand of offset.  */
    unsigned int in_bracket;	/* >=1 if processing operand in brackets.  */
    unsigned int in_scale;	/* >=1 if processing multipication operand
				 * in brackets.  */
    i386_operand_type reloc_types;	/* Value obtained from lex_got().  */
    const reg_entry *base;	/* Base register (if any).  */
    const reg_entry *index;	/* Index register (if any).  */
    offsetT scale_factor;	/* Accumulated scale factor.  */
    symbolS *seg;
  }
intel_state;

/* offset X_add_symbol */
#define O_offset O_md32
/* offset X_add_symbol */
#define O_short O_md31
/* near ptr X_add_symbol */
#define O_near_ptr O_md30
/* far ptr X_add_symbol */
#define O_far_ptr O_md29
/* byte ptr X_add_symbol */
#define O_byte_ptr O_md28
/* word ptr X_add_symbol */
#define O_word_ptr O_md27
/* dword ptr X_add_symbol */
#define O_dword_ptr O_md26
/* qword ptr X_add_symbol */
#define O_qword_ptr O_md25
/* oword ptr X_add_symbol */
#define O_oword_ptr O_md24
/* fword ptr X_add_symbol */
#define O_fword_ptr O_md23
/* tbyte ptr X_add_symbol */
#define O_tbyte_ptr O_md22
/* xmmword ptr X_add_symbol */
#define O_xmmword_ptr O_md21
/* ymmword ptr X_add_symbol */
#define O_ymmword_ptr O_md20

static struct
  {
    const char *name;
    operatorT op;
    unsigned int operands;
  }
const i386_operators[] =
  {
    { "and", O_bit_and, 2 },
    { "eq", O_eq, 2 },
    { "ge", O_ge, 2 },
    { "gt", O_gt, 2 },
    { "le", O_le, 2 },
    { "lt", O_lt, 2 },
    { "mod", O_modulus, 2 },
    { "ne", O_ne, 2 },
    { "not", O_bit_not, 1 },
    { "offset", O_offset, 1 },
    { "or", O_bit_inclusive_or, 2 },
    { "shl", O_left_shift, 2 },
    { "short", O_short, 1 },
    { "shr", O_right_shift, 2 },
    { "xor", O_bit_exclusive_or, 2 },
    { NULL, O_illegal, 0 }
  };

static struct
  {
    const char *name;
    operatorT op;
    unsigned short sz[3];
  }
const i386_types[] =
  {
#define I386_TYPE(t, n) { #t, O_##t##_ptr, { n, n, n } }
    I386_TYPE(byte, 1),
    I386_TYPE(word, 2),
    I386_TYPE(dword, 4),
    I386_TYPE(fword, 6),
    I386_TYPE(qword, 8),
    I386_TYPE(tbyte, 10),
    I386_TYPE(oword, 16),
    I386_TYPE(xmmword, 16),
    I386_TYPE(ymmword, 32),
#undef I386_TYPE
    { "near", O_near_ptr, { 0xff04, 0xff02, 0xff08 } },
    { "far", O_far_ptr, { 0xff06, 0xff05, 0xff06 } },
    { NULL, O_illegal, { 0, 0, 0 } }
  };

operatorT i386_operator (const char *name, unsigned int operands, char *pc)
{
  unsigned int j;

  if (!intel_syntax)
    return O_absent;

  if (!name)
    {
      if (operands != 2)
	return O_illegal;
      switch (*input_line_pointer)
	{
	case ':':
	  ++input_line_pointer;
	  return O_full_ptr;
	case '[':
	  ++input_line_pointer;
	  return O_index;
	case '@':
	  if (this_operand >= 0 && i.reloc[this_operand] == NO_RELOC)
	    {
	      int adjust = 0;
	      char *gotfree_input_line = lex_got (&i.reloc[this_operand],
						  &adjust,
						  &intel_state.reloc_types);

	      if (!gotfree_input_line)
		break;
	      free (gotfree_input_line);
	      *input_line_pointer++ = '+';
	      memset (input_line_pointer, '0', adjust - 1);
	      input_line_pointer[adjust - 1] = ' ';
	      return O_add;
	    }
	  break;
	}
      return O_illegal;
    }

  for (j = 0; i386_operators[j].name; ++j)
    if (strcasecmp (i386_operators[j].name, name) == 0)
      {
	if (i386_operators[j].operands
	    && i386_operators[j].operands != operands)
	  return O_illegal;
	return i386_operators[j].op;
      }

  for (j = 0; i386_types[j].name; ++j)
    if (strcasecmp (i386_types[j].name, name) == 0)
      break;
  if (i386_types[j].name && *pc == ' ')
    {
      char *pname = ++input_line_pointer;
      char c = get_symbol_end ();

      if (strcasecmp (pname, "ptr") == 0)
	{
	  pname[-1] = *pc;
	  *pc = c;
	  if (intel_syntax > 0 || operands != 1)
	    return O_illegal;
	  return i386_types[j].op;
	}

      *input_line_pointer = c;
      input_line_pointer = pname - 1;
    }

  return O_absent;
}

static int i386_intel_parse_name (const char *name, expressionS *e)
{
  unsigned int j;

  if (! strcmp (name, "$"))
    {
      current_location (e);
      return 1;
    }

  for (j = 0; i386_types[j].name; ++j)
    if (strcasecmp(i386_types[j].name, name) == 0)
      {
	e->X_op = O_constant;
	e->X_add_number = i386_types[j].sz[flag_code];
	e->X_add_symbol = NULL;
	e->X_op_symbol = NULL;
	return 1;
      }

  return 0;
}

static INLINE int i386_intel_check (const reg_entry *rreg,
				    const reg_entry *base,
				    const reg_entry *iindex)
{
  if ((this_operand >= 0
       && rreg != i.op[this_operand].regs)
      || base != intel_state.base
      || iindex != intel_state.index)
    {
      as_bad (_("invalid use of register"));
      return 0;
    }
  return 1;
}

static INLINE void i386_intel_fold (expressionS *e, symbolS *sym)
{
  expressionS *exp = symbol_get_value_expression (sym);
  if (S_GET_SEGMENT (sym) == absolute_section)
    {
      offsetT val = e->X_add_number;

      *e = *exp;
      e->X_add_number += val;
    }
  else
    {
      if (exp->X_op == O_symbol
	  && strcmp (S_GET_NAME (exp->X_add_symbol),
		     GLOBAL_OFFSET_TABLE_NAME) == 0)
	sym = exp->X_add_symbol;
      e->X_add_symbol = sym;
      e->X_op_symbol = NULL;
      e->X_op = O_symbol;
    }
}

static int
i386_intel_simplify_register (expressionS *e)
{
  int reg_num;

  if (this_operand < 0 || intel_state.in_offset)
    {
      as_bad (_("invalid use of register"));
      return 0;
    }

  if (e->X_op == O_register)
    reg_num = e->X_add_number;
  else
    reg_num = e->X_md - 1;

  if (!intel_state.in_bracket)
    {
      if (i.op[this_operand].regs)
	{
	  as_bad (_("invalid use of register"));
	  return 0;
	}
      if (i386_regtab[reg_num].reg_type.bitfield.sreg3
	  && i386_regtab[reg_num].reg_num == RegFlat)
	{
	  as_bad (_("invalid use of pseudo-register"));
	  return 0;
	}
      i.op[this_operand].regs = i386_regtab + reg_num;
    }
  else if (!intel_state.index
	   && (i386_regtab[reg_num].reg_type.bitfield.regxmm
	       || i386_regtab[reg_num].reg_type.bitfield.regymm))
    intel_state.index = i386_regtab + reg_num;
  else if (!intel_state.base && !intel_state.in_scale)
    intel_state.base = i386_regtab + reg_num;
  else if (!intel_state.index)
    {
      if (intel_state.in_scale
	  || i386_regtab[reg_num].reg_type.bitfield.baseindex)
	intel_state.index = i386_regtab + reg_num;
      else
	{
	  /* Convert base to index and make ESP/RSP the base.  */
	  intel_state.index = intel_state.base;
	  intel_state.base = i386_regtab + reg_num;
	}
    }
  else
    {
      /* esp is invalid as index */
      intel_state.index = i386_regtab + REGNAM_EAX + ESP_REG_NUM;
    }
  return 2;
}

static int i386_intel_simplify (expressionS *);

static INLINE int i386_intel_simplify_symbol(symbolS *sym)
{
  int ret = i386_intel_simplify (symbol_get_value_expression (sym));

  if (ret == 2)
  {
    S_SET_SEGMENT(sym, absolute_section);
    ret = 1;
  }
  return ret;
}

static int i386_intel_simplify (expressionS *e)
{
  const reg_entry *the_reg = (this_operand >= 0
			      ? i.op[this_operand].regs : NULL);
  const reg_entry *base = intel_state.base;
  const reg_entry *state_index = intel_state.index;
  int ret;

  if (!intel_syntax)
    return 1;

  switch (e->X_op)
    {
    case O_index:
      if (e->X_add_symbol)
	{
	  if (!i386_intel_simplify_symbol (e->X_add_symbol)
	      || !i386_intel_check(the_reg, intel_state.base,
				   intel_state.index))
	    return 0;;
	}
      if (!intel_state.in_offset)
	++intel_state.in_bracket;
      ret = i386_intel_simplify_symbol (e->X_op_symbol);
      if (!intel_state.in_offset)
	--intel_state.in_bracket;
      if (!ret)
	return 0;
      if (e->X_add_symbol)
	e->X_op = O_add;
      else
	i386_intel_fold (e, e->X_op_symbol);
      break;

    case O_offset:
      intel_state.has_offset = 1;
      ++intel_state.in_offset;
      ret = i386_intel_simplify_symbol (e->X_add_symbol);
      --intel_state.in_offset;
      if (!ret || !i386_intel_check(the_reg, base, state_index))
	return 0;
      i386_intel_fold (e, e->X_add_symbol);
      return ret;

    case O_byte_ptr:
    case O_word_ptr:
    case O_dword_ptr:
    case O_fword_ptr:
    case O_qword_ptr:
    case O_tbyte_ptr:
    case O_oword_ptr:
    case O_xmmword_ptr:
    case O_ymmword_ptr:
    case O_near_ptr:
    case O_far_ptr:
      if (intel_state.op_modifier == O_absent)
	intel_state.op_modifier = e->X_op;
      /* FALLTHROUGH */
    case O_short:
      if (symbol_get_value_expression (e->X_add_symbol)->X_op
	  == O_register)
	{
	  as_bad (_("invalid use of register"));
	  return 0;
	}
      if (!i386_intel_simplify_symbol (e->X_add_symbol))
	return 0;
      i386_intel_fold (e, e->X_add_symbol);
      break;

    case O_full_ptr:
      if (symbol_get_value_expression (e->X_op_symbol)->X_op
	  == O_register)
	{
	  as_bad (_("invalid use of register"));
	  return 0;
	}
      if (!i386_intel_simplify_symbol (e->X_op_symbol)
	  || !i386_intel_check(the_reg, intel_state.base,
			       intel_state.index))
	return 0;
      if (!intel_state.in_offset)
	intel_state.seg = e->X_add_symbol;
      i386_intel_fold (e, e->X_op_symbol);
      break;

    case O_multiply:
      if (this_operand >= 0 && intel_state.in_bracket)
	{
	  expressionS *scale = NULL;

	  if (intel_state.index)
	    --scale;

	  if (!intel_state.in_scale++)
	    intel_state.scale_factor = 1;

	  ret = i386_intel_simplify_symbol (e->X_add_symbol);
	  if (ret && !scale && intel_state.index)
	    scale = symbol_get_value_expression (e->X_op_symbol);

	  if (ret)
	    ret = i386_intel_simplify_symbol (e->X_op_symbol);
	  if (ret && !scale && intel_state.index)
	    scale = symbol_get_value_expression (e->X_add_symbol);

	  if (ret && scale && (scale + 1))
	    {
	      resolve_expression (scale);
	      if (scale->X_op != O_constant
		  || intel_state.index->reg_type.bitfield.reg16)
		scale->X_add_number = 0;
	      intel_state.scale_factor *= scale->X_add_number;
	    }

	  --intel_state.in_scale;
	  if (!ret)
	    return 0;

	  if (!intel_state.in_scale)
	    switch (intel_state.scale_factor)
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
		/* esp is invalid as index */
		intel_state.index = i386_regtab + REGNAM_EAX + ESP_REG_NUM;
		break;
	      }

	  break;
	}
      goto fallthrough;

    case O_register:
      ret = i386_intel_simplify_register (e);
      if (ret == 2)
	{
	  gas_assert (e->X_add_number < (unsigned short) -1);
	  e->X_md = (unsigned short) e->X_add_number + 1;
	  e->X_op = O_constant;
	  e->X_add_number = 0;
	}
      return ret;

    case O_constant:
      if (e->X_md)
	return i386_intel_simplify_register (e);

      /* FALLTHROUGH */
    default:
fallthrough:
      if (e->X_add_symbol
	  && !i386_intel_simplify_symbol (e->X_add_symbol))
	return 0;
      if (e->X_op == O_add || e->X_op == O_subtract)
	{
	  base = intel_state.base;
	  state_index = intel_state.index;
	}
      if (!i386_intel_check (the_reg, base, state_index)
	  || (e->X_op_symbol
	      && !i386_intel_simplify_symbol (e->X_op_symbol))
	  || !i386_intel_check (the_reg,
				(e->X_op != O_add
				 ? base : intel_state.base),
				(e->X_op != O_add
				 ? state_index : intel_state.index)))
	return 0;
      break;
    }

  if (this_operand >= 0
      && e->X_op == O_symbol
      && !intel_state.in_offset)
    {
      segT seg = S_GET_SEGMENT (e->X_add_symbol);

      if (seg != absolute_section
	  && seg != reg_section
	  && seg != expr_section)
	intel_state.is_mem |= 2 - !intel_state.in_bracket;
    }

  return 1;
}

int i386_need_index_operator (void)
{
  return intel_syntax < 0;
}

static int
i386_intel_operand (char *operand_string, int got_a_float)
{
  char *saved_input_line_pointer, *buf;
  segT exp_seg;
  expressionS exp, *expP;
  char suffix = 0;
  int ret;

  /* Initialize state structure.  */
  intel_state.op_modifier = O_absent;
  intel_state.is_mem = 0;
  intel_state.is_indirect = 0;
  intel_state.has_offset = 0;
  intel_state.base = NULL;
  intel_state.index = NULL;
  intel_state.seg = NULL;
  operand_type_set (&intel_state.reloc_types, ~0);
  gas_assert (!intel_state.in_offset);
  gas_assert (!intel_state.in_bracket);
  gas_assert (!intel_state.in_scale);

  saved_input_line_pointer = input_line_pointer;
  input_line_pointer = buf = xstrdup (operand_string);

  intel_syntax = -1;
  memset (&exp, 0, sizeof(exp));
  exp_seg = expression (&exp);
  ret = i386_intel_simplify (&exp);
  intel_syntax = 1;

  SKIP_WHITESPACE ();
  if (!is_end_of_line[(unsigned char) *input_line_pointer])
    {
      as_bad (_("junk `%s' after expression"), input_line_pointer);
      ret = 0;
    }
  else if (exp.X_op == O_illegal || exp.X_op == O_absent)
    {
      as_bad (_("invalid expression"));
      ret = 0;
    }
  else if (!intel_state.has_offset
	   && input_line_pointer > buf
	   && *(input_line_pointer - 1) == ']')
    {
      intel_state.is_mem |= 1;
      intel_state.is_indirect = 1;
    }

  input_line_pointer = saved_input_line_pointer;
  free (buf);

  gas_assert (!intel_state.in_offset);
  gas_assert (!intel_state.in_bracket);
  gas_assert (!intel_state.in_scale);

  if (!ret)
    return 0;

  if (intel_state.op_modifier != O_absent
      && current_templates->start->base_opcode != 0x8d /* lea */)
    {
      i.types[this_operand].bitfield.unspecified = 0;

      switch (intel_state.op_modifier)
	{
	case O_byte_ptr:
	  i.types[this_operand].bitfield.byte = 1;
	  suffix = BYTE_MNEM_SUFFIX;
	  break;

	case O_word_ptr:
	  i.types[this_operand].bitfield.word = 1;
	  if ((current_templates->start->name[0] == 'l'
	       && current_templates->start->name[2] == 's'
	       && current_templates->start->name[3] == 0)
	      || current_templates->start->base_opcode == 0x62 /* bound */)
	    suffix = BYTE_MNEM_SUFFIX; /* so it will cause an error */
	  else if (got_a_float == 2)	/* "fi..." */
	    suffix = SHORT_MNEM_SUFFIX;
	  else
	    suffix = WORD_MNEM_SUFFIX;
	  break;

	case O_dword_ptr:
	  i.types[this_operand].bitfield.dword = 1;
	  if ((current_templates->start->name[0] == 'l'
	       && current_templates->start->name[2] == 's'
	       && current_templates->start->name[3] == 0)
	      || current_templates->start->base_opcode == 0x62 /* bound */)
	    suffix = WORD_MNEM_SUFFIX;
	  else if (flag_code == CODE_16BIT
		   && (current_templates->start->opcode_modifier.jump
		       || current_templates->start->opcode_modifier.jumpdword))
	    suffix = LONG_DOUBLE_MNEM_SUFFIX;
	  else if (got_a_float == 1)	/* "f..." */
	    suffix = SHORT_MNEM_SUFFIX;
	  else
	    suffix = LONG_MNEM_SUFFIX;
	  break;

	case O_fword_ptr:
	  i.types[this_operand].bitfield.fword = 1;
	  if (current_templates->start->name[0] == 'l'
	      && current_templates->start->name[2] == 's'
	      && current_templates->start->name[3] == 0)
	    suffix = LONG_MNEM_SUFFIX;
	  else if (!got_a_float)
	    {
	      if (flag_code == CODE_16BIT)
		add_prefix (DATA_PREFIX_OPCODE);
	      suffix = LONG_DOUBLE_MNEM_SUFFIX;
	    }
	  else
	    suffix = BYTE_MNEM_SUFFIX; /* so it will cause an error */
	  break;

	case O_qword_ptr:
	  i.types[this_operand].bitfield.qword = 1;
	  if (current_templates->start->base_opcode == 0x62 /* bound */
	      || got_a_float == 1)	/* "f..." */
	    suffix = LONG_MNEM_SUFFIX;
	  else
	    suffix = QWORD_MNEM_SUFFIX;
	  break;

	case O_tbyte_ptr:
	  i.types[this_operand].bitfield.tbyte = 1;
	  if (got_a_float == 1)
	    suffix = LONG_DOUBLE_MNEM_SUFFIX;
	  else
	    suffix = BYTE_MNEM_SUFFIX; /* so it will cause an error */
	  break;

	case O_oword_ptr:
	case O_xmmword_ptr:
	  i.types[this_operand].bitfield.xmmword = 1;
	  suffix = XMMWORD_MNEM_SUFFIX;
	  break;

	case O_ymmword_ptr:
	  i.types[this_operand].bitfield.ymmword = 1;
	  suffix = YMMWORD_MNEM_SUFFIX;
	  break;

	case O_far_ptr:
	  suffix = LONG_DOUBLE_MNEM_SUFFIX;
	  /* FALLTHROUGH */
	case O_near_ptr:
	  if (!current_templates->start->opcode_modifier.jump
	      && !current_templates->start->opcode_modifier.jumpdword)
	    suffix = got_a_float /* so it will cause an error */
		     ? BYTE_MNEM_SUFFIX
		     : LONG_DOUBLE_MNEM_SUFFIX;
	  break;

	default:
	  BAD_CASE (intel_state.op_modifier);
	  break;
	}

      if (!i.suffix)
	i.suffix = suffix;
      else if (i.suffix != suffix)
	{
	  as_bad (_("conflicting operand size modifiers"));
	  return 0;
	}
    }

  /* Operands for jump/call need special consideration.  */
  if (current_templates->start->opcode_modifier.jump
      || current_templates->start->opcode_modifier.jumpdword
      || current_templates->start->opcode_modifier.jumpintersegment)
    {
      if (i.op[this_operand].regs
	  || intel_state.base
	  || intel_state.index
	  || intel_state.is_mem > 1)
	i.types[this_operand].bitfield.jumpabsolute = 1;
      else
	switch (intel_state.op_modifier)
	  {
	  case O_near_ptr:
	    if (intel_state.seg)
	      i.types[this_operand].bitfield.jumpabsolute = 1;
	    else
	      intel_state.is_mem = 1;
	    break;
	  case O_far_ptr:
	  case O_absent:
	    if (!intel_state.seg)
	      {
		intel_state.is_mem = 1;
		if (intel_state.op_modifier == O_absent)
		  {
		    if (intel_state.is_indirect == 1)
		      i.types[this_operand].bitfield.jumpabsolute = 1;
		    break;
		  }
		as_bad (_("cannot infer the segment part of the operand"));
		return 0;
	      }
	    else if (S_GET_SEGMENT (intel_state.seg) == reg_section)
	      i.types[this_operand].bitfield.jumpabsolute = 1;
	    else
	      {
		i386_operand_type types;

		if (i.imm_operands >= MAX_IMMEDIATE_OPERANDS)
		  {
		    as_bad (_("at most %d immediate operands are allowed"),
			    MAX_IMMEDIATE_OPERANDS);
		    return 0;
		  }
		expP = &im_expressions[i.imm_operands++];
		memset (expP, 0, sizeof(*expP));
		expP->X_op = O_symbol;
		expP->X_add_symbol = intel_state.seg;
		i.op[this_operand].imms = expP;

		resolve_expression (expP);
		operand_type_set (&types, ~0);
		if (!i386_finalize_immediate (S_GET_SEGMENT (intel_state.seg),
					      expP, types, operand_string))
		  return 0;
		if (i.operands < MAX_OPERANDS)
		  {
		    this_operand = i.operands++;
		    i.types[this_operand].bitfield.unspecified = 1;
		  }
		if (suffix == LONG_DOUBLE_MNEM_SUFFIX)
		  i.suffix = 0;
		intel_state.seg = NULL;
		intel_state.is_mem = 0;
	      }
	    break;
	  default:
	    i.types[this_operand].bitfield.jumpabsolute = 1;
	    break;
	  }
      if (i.types[this_operand].bitfield.jumpabsolute)
	intel_state.is_mem |= 1;
    }
  else if (intel_state.seg)
    intel_state.is_mem |= 1;

  if (i.op[this_operand].regs)
    {
      i386_operand_type temp;

      /* Register operand.  */
      if (intel_state.base || intel_state.index || intel_state.seg)
	{
	  as_bad (_("invalid operand"));
	  return 0;
	}

      temp = i.op[this_operand].regs->reg_type;
      temp.bitfield.baseindex = 0;
      i.types[this_operand] = operand_type_or (i.types[this_operand],
					       temp);
      i.types[this_operand].bitfield.unspecified = 0;
      ++i.reg_operands;
    }
  else if (intel_state.base
	   || intel_state.index
	   || intel_state.seg
	   || intel_state.is_mem)
    {
      /* Memory operand.  */
      if (i.mem_operands
	  >= 2 - !current_templates->start->opcode_modifier.isstring)
	{
	  /* Handle

	     call	0x9090,0x90909090
	     lcall	0x9090,0x90909090
	     jmp	0x9090,0x90909090
	     ljmp	0x9090,0x90909090
	   */

	  if ((current_templates->start->opcode_modifier.jumpintersegment
	       || current_templates->start->opcode_modifier.jumpdword
	       || current_templates->start->opcode_modifier.jump)
	      && this_operand == 1
	      && intel_state.seg == NULL
	      && i.mem_operands == 1
	      && i.disp_operands == 1
	      && intel_state.op_modifier == O_absent)
	    {
	      /* Try to process the first operand as immediate,  */
	      this_operand = 0;
	      if (i386_finalize_immediate (exp_seg, i.op[0].imms,
					   intel_state.reloc_types,
					   NULL))
		{
		  this_operand = 1;
		  expP = &im_expressions[0];
		  i.op[this_operand].imms = expP;
		  *expP = exp;

		  /* Try to process the second operand as immediate,  */
		  if (i386_finalize_immediate (exp_seg, expP,
					       intel_state.reloc_types,
					       NULL))
		    {
		      i.mem_operands = 0;
		      i.disp_operands = 0;
		      i.imm_operands = 2;
		      i.types[0].bitfield.mem = 0;
		      i.types[0].bitfield.disp16 = 0;
		      i.types[0].bitfield.disp32 = 0;
		      i.types[0].bitfield.disp32s = 0;
		      return 1;
		    }
		}
	    }

	  as_bad (_("too many memory references for `%s'"),
		  current_templates->start->name);
	  return 0;
	}

      expP = &disp_expressions[i.disp_operands];
      memcpy (expP, &exp, sizeof(exp));
      resolve_expression (expP);

      if (expP->X_op != O_constant
	  || expP->X_add_number
	  || (!intel_state.base
	      && !intel_state.index))
	{
	  i.op[this_operand].disps = expP;
	  i.disp_operands++;

	  if (flag_code == CODE_64BIT)
	    {
	      i.types[this_operand].bitfield.disp32 = 1;
	      if (!i.prefix[ADDR_PREFIX])
		{
		  i.types[this_operand].bitfield.disp64 = 1;
		  i.types[this_operand].bitfield.disp32s = 1;
		}
	    }
	  else if (!i.prefix[ADDR_PREFIX] ^ (flag_code == CODE_16BIT))
	    i.types[this_operand].bitfield.disp32 = 1;
	  else
	    i.types[this_operand].bitfield.disp16 = 1;

#if defined (OBJ_AOUT) || defined (OBJ_MAYBE_AOUT)
	  /*
	   * exp_seg is used only for verification in
	   * i386_finalize_displacement, and we can end up seeing reg_section
	   * here - but we know we removed all registers from the expression
	   * (or error-ed on any remaining ones) in i386_intel_simplify.  I
	   * consider the check in i386_finalize_displacement bogus anyway, in
	   * particular because it doesn't allow for expr_section, so I'd
	   * rather see that check (and the similar one in
	   * i386_finalize_immediate) use SEG_NORMAL(), but not being an a.out
	   * expert I can't really say whether that would have other bad side
	   * effects.
	   */
	  if (OUTPUT_FLAVOR == bfd_target_aout_flavour
	      && exp_seg == reg_section)
	    exp_seg = expP->X_op != O_constant ? undefined_section
					       : absolute_section;
#endif

	  if (!i386_finalize_displacement (exp_seg, expP,
					   intel_state.reloc_types,
					   operand_string))
	    return 0;
	}

      if (intel_state.base || intel_state.index)
	i.types[this_operand].bitfield.baseindex = 1;

      if (intel_state.seg)
	{
	  for (;;)
	    {
	      expP = symbol_get_value_expression (intel_state.seg);
	      if (expP->X_op != O_full_ptr)
		break;
	      intel_state.seg = expP->X_add_symbol;
	    }
	  if (expP->X_op != O_register)
	    {
	      as_bad (_("segment register name expected"));
	      return 0;
	    }
	  if (!i386_regtab[expP->X_add_number].reg_type.bitfield.sreg2
	      && !i386_regtab[expP->X_add_number].reg_type.bitfield.sreg3)
	    {
	      as_bad (_("invalid use of register"));
	      return 0;
	    }
	  switch (i386_regtab[expP->X_add_number].reg_num)
	    {
	    case 0: i.seg[i.mem_operands] = &es; break;
	    case 1: i.seg[i.mem_operands] = &cs; break;
	    case 2: i.seg[i.mem_operands] = &ss; break;
	    case 3: i.seg[i.mem_operands] = &ds; break;
	    case 4: i.seg[i.mem_operands] = &fs; break;
	    case 5: i.seg[i.mem_operands] = &gs; break;
	    case RegFlat: i.seg[i.mem_operands] = NULL; break;
	    }
	}

      /* Swap base and index in 16-bit memory operands like
	 [si+bx]. Since i386_index_check is also used in AT&T
	 mode we have to do that here.  */
      if (intel_state.base
	  && intel_state.index
	  && intel_state.base->reg_type.bitfield.reg16
	  && intel_state.index->reg_type.bitfield.reg16
	  && intel_state.base->reg_num >= 6
	  && intel_state.index->reg_num < 6)
	{
	  i.base_reg = intel_state.index;
	  i.index_reg = intel_state.base;
	}
      else
	{
	  i.base_reg = intel_state.base;
	  i.index_reg = intel_state.index;
	}

      if (!i386_index_check (operand_string))
	return 0;

      i.types[this_operand].bitfield.mem = 1;
      ++i.mem_operands;
    }
  else
    {
      /* Immediate.  */
      if (i.imm_operands >= MAX_IMMEDIATE_OPERANDS)
	{
	  as_bad (_("at most %d immediate operands are allowed"),
		  MAX_IMMEDIATE_OPERANDS);
	  return 0;
	}

      expP = &im_expressions[i.imm_operands++];
      i.op[this_operand].imms = expP;
      *expP = exp;

      return i386_finalize_immediate (exp_seg, expP, intel_state.reloc_types,
				      operand_string);
    }

  return 1;
}
