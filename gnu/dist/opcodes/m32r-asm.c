/* Assembler interface for targets using CGEN. -*- C -*-
   CGEN: Cpu tools GENerator

This file is used to generate m32r-asm.c.

Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.

This file is part of the GNU Binutils and GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "sysdep.h"
#include <ctype.h>
#include <stdio.h>
#include "ansidecl.h"
#include "bfd.h"
#include "symcat.h"
#include "m32r-opc.h"

/* ??? The layout of this stuff is still work in progress.
   For speed in assembly/disassembly, we use inline functions.  That of course
   will only work for GCC.  When this stuff is finished, we can decide whether
   to keep the inline functions (and only get the performance increase when
   compiled with GCC), or switch to macros, or use something else.
*/

static const char * insert_normal
     PARAMS ((long, unsigned int, int, int, int, char *));
static const char * parse_insn_normal
     PARAMS ((const CGEN_INSN *, const char **, CGEN_FIELDS *));
static const char * insert_insn_normal
     PARAMS ((const CGEN_INSN *, CGEN_FIELDS *, cgen_insn_t *));

/* -- assembler routines inserted here */
/* -- asm.c */

/* Handle '#' prefixes (i.e. skip over them).  */

static const char *
parse_hash (strp, opindex, valuep)
     const char **strp;
     int opindex;
     unsigned long *valuep;
{
  if (**strp == '#')
    ++*strp;
  return NULL;
}

/* Handle shigh(), high().  */

static const char *
parse_hi16 (strp, opindex, valuep)
     const char **strp;
     int opindex;
     unsigned long *valuep;
{
  const char *errmsg;
  enum cgen_parse_operand_result result_type;

  if (**strp == '#')
    ++*strp;

  if (strncmp (*strp, "high(", 5) == 0)
    {
      *strp += 5;
      errmsg = cgen_parse_address (strp, opindex, BFD_RELOC_M32R_HI16_ULO,
				   &result_type, valuep);
      if (**strp != ')')
	return "missing `)'";
      ++*strp;
      if (errmsg == NULL
  	  && result_type == CGEN_PARSE_OPERAND_RESULT_NUMBER)
	*valuep >>= 16;
      return errmsg;
    }
  else if (strncmp (*strp, "shigh(", 6) == 0)
    {
      *strp += 6;
      errmsg = cgen_parse_address (strp, opindex, BFD_RELOC_M32R_HI16_SLO,
 				   &result_type, valuep);
      if (**strp != ')')
	return "missing `)'";
      ++*strp;
      if (errmsg == NULL
	  && result_type == CGEN_PARSE_OPERAND_RESULT_NUMBER)
	*valuep = (*valuep >> 16) + ((*valuep) & 0x8000 ? 1 : 0);
      return errmsg;
    }

  return cgen_parse_unsigned_integer (strp, opindex, valuep);
}

/* Handle low() in a signed context.  Also handle sda().
   The signedness of the value doesn't matter to low(), but this also
   handles the case where low() isn't present.  */

static const char *
parse_slo16 (strp, opindex, valuep)
     const char **strp;
     int opindex;
     long *valuep;
{
  const char *errmsg;
  enum cgen_parse_operand_result result_type;

  if (**strp == '#')
    ++*strp;

  if (strncmp (*strp, "low(", 4) == 0)
    {
      *strp += 4;
      errmsg = cgen_parse_address (strp, opindex, BFD_RELOC_M32R_LO16,
				   &result_type, valuep);
      if (**strp != ')')
	return "missing `)'";
      ++*strp;
      if (errmsg == NULL
	  && result_type == CGEN_PARSE_OPERAND_RESULT_NUMBER)
	*valuep &= 0xffff;
      return errmsg;
    }

  if (strncmp (*strp, "sda(", 4) == 0)
    {
      *strp += 4;
      errmsg = cgen_parse_address (strp, opindex, BFD_RELOC_M32R_SDA16, NULL, valuep);
      if (**strp != ')')
	return "missing `)'";
      ++*strp;
      return errmsg;
    }

  return cgen_parse_signed_integer (strp, opindex, valuep);
}

/* Handle low() in an unsigned context.
   The signedness of the value doesn't matter to low(), but this also
   handles the case where low() isn't present.  */

static const char *
parse_ulo16 (strp, opindex, valuep)
     const char **strp;
     int opindex;
     unsigned long *valuep;
{
  const char *errmsg;
  enum cgen_parse_operand_result result_type;

  if (**strp == '#')
    ++*strp;

  if (strncmp (*strp, "low(", 4) == 0)
    {
      *strp += 4;
      errmsg = cgen_parse_address (strp, opindex, BFD_RELOC_M32R_LO16,
				   &result_type, valuep);
      if (**strp != ')')
	return "missing `)'";
      ++*strp;
      if (errmsg == NULL
	  && result_type == CGEN_PARSE_OPERAND_RESULT_NUMBER)
	*valuep &= 0xffff;
      return errmsg;
    }

  return cgen_parse_unsigned_integer (strp, opindex, valuep);
}

/* -- */

/* Main entry point for operand parsing.

   This function is basically just a big switch statement.  Earlier versions
   used tables to look up the function to use, but
   - if the table contains both assembler and disassembler functions then
     the disassembler contains much of the assembler and vice-versa,
   - there's a lot of inlining possibilities as things grow,
   - using a switch statement avoids the function call overhead.

   This function could be moved into `parse_insn_normal', but keeping it
   separate makes clear the interface between `parse_insn_normal' and each of
   the handlers.
*/

const char *
m32r_cgen_parse_operand (opindex, strp, fields)
     int opindex;
     const char ** strp;
     CGEN_FIELDS * fields;
{
  const char * errmsg;

  switch (opindex)
    {
    case M32R_OPERAND_SR :
      errmsg = cgen_parse_keyword (strp, & m32r_cgen_opval_h_gr, & fields->f_r2);
      break;
    case M32R_OPERAND_DR :
      errmsg = cgen_parse_keyword (strp, & m32r_cgen_opval_h_gr, & fields->f_r1);
      break;
    case M32R_OPERAND_SRC1 :
      errmsg = cgen_parse_keyword (strp, & m32r_cgen_opval_h_gr, & fields->f_r1);
      break;
    case M32R_OPERAND_SRC2 :
      errmsg = cgen_parse_keyword (strp, & m32r_cgen_opval_h_gr, & fields->f_r2);
      break;
    case M32R_OPERAND_SCR :
      errmsg = cgen_parse_keyword (strp, & m32r_cgen_opval_h_cr, & fields->f_r2);
      break;
    case M32R_OPERAND_DCR :
      errmsg = cgen_parse_keyword (strp, & m32r_cgen_opval_h_cr, & fields->f_r1);
      break;
    case M32R_OPERAND_SIMM8 :
      errmsg = cgen_parse_signed_integer (strp, M32R_OPERAND_SIMM8, &fields->f_simm8);
      break;
    case M32R_OPERAND_SIMM16 :
      errmsg = cgen_parse_signed_integer (strp, M32R_OPERAND_SIMM16, &fields->f_simm16);
      break;
    case M32R_OPERAND_UIMM4 :
      errmsg = cgen_parse_unsigned_integer (strp, M32R_OPERAND_UIMM4, &fields->f_uimm4);
      break;
    case M32R_OPERAND_UIMM5 :
      errmsg = cgen_parse_unsigned_integer (strp, M32R_OPERAND_UIMM5, &fields->f_uimm5);
      break;
    case M32R_OPERAND_UIMM16 :
      errmsg = cgen_parse_unsigned_integer (strp, M32R_OPERAND_UIMM16, &fields->f_uimm16);
      break;
    case M32R_OPERAND_HASH :
      errmsg = parse_hash (strp, M32R_OPERAND_HASH, &fields->f_nil);
      break;
    case M32R_OPERAND_HI16 :
      errmsg = parse_hi16 (strp, M32R_OPERAND_HI16, &fields->f_hi16);
      break;
    case M32R_OPERAND_SLO16 :
      errmsg = parse_slo16 (strp, M32R_OPERAND_SLO16, &fields->f_simm16);
      break;
    case M32R_OPERAND_ULO16 :
      errmsg = parse_ulo16 (strp, M32R_OPERAND_ULO16, &fields->f_uimm16);
      break;
    case M32R_OPERAND_UIMM24 :
      errmsg = cgen_parse_address (strp, M32R_OPERAND_UIMM24, 0, NULL, & fields->f_uimm24);
      break;
    case M32R_OPERAND_DISP8 :
      errmsg = cgen_parse_address (strp, M32R_OPERAND_DISP8, 0, NULL, & fields->f_disp8);
      break;
    case M32R_OPERAND_DISP16 :
      errmsg = cgen_parse_address (strp, M32R_OPERAND_DISP16, 0, NULL, & fields->f_disp16);
      break;
    case M32R_OPERAND_DISP24 :
      errmsg = cgen_parse_address (strp, M32R_OPERAND_DISP24, 0, NULL, & fields->f_disp24);
      break;

    default :
      fprintf (stderr, "Unrecognized field %d while parsing.\n", opindex);
      abort ();
  }

  return errmsg;
}

/* Main entry point for operand insertion.

   This function is basically just a big switch statement.  Earlier versions
   used tables to look up the function to use, but
   - if the table contains both assembler and disassembler functions then
     the disassembler contains much of the assembler and vice-versa,
   - there's a lot of inlining possibilities as things grow,
   - using a switch statement avoids the function call overhead.

   This function could be moved into `parse_insn_normal', but keeping it
   separate makes clear the interface between `parse_insn_normal' and each of
   the handlers.  It's also needed by GAS to insert operands that couldn't be
   resolved during parsing.
*/

const char *
m32r_cgen_insert_operand (opindex, fields, buffer)
     int opindex;
     CGEN_FIELDS * fields;
     char * buffer;
{
  const char * errmsg;

  switch (opindex)
    {
    case M32R_OPERAND_SR :
      errmsg = insert_normal (fields->f_r2, 0|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_DR :
      errmsg = insert_normal (fields->f_r1, 0|(1<<CGEN_OPERAND_UNSIGNED), 4, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_SRC1 :
      errmsg = insert_normal (fields->f_r1, 0|(1<<CGEN_OPERAND_UNSIGNED), 4, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_SRC2 :
      errmsg = insert_normal (fields->f_r2, 0|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_SCR :
      errmsg = insert_normal (fields->f_r2, 0|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_DCR :
      errmsg = insert_normal (fields->f_r1, 0|(1<<CGEN_OPERAND_UNSIGNED), 4, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_SIMM8 :
      errmsg = insert_normal (fields->f_simm8, 0|(1<<CGEN_OPERAND_HASH_PREFIX), 8, 8, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_SIMM16 :
      errmsg = insert_normal (fields->f_simm16, 0|(1<<CGEN_OPERAND_HASH_PREFIX), 16, 16, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_UIMM4 :
      errmsg = insert_normal (fields->f_uimm4, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_UIMM5 :
      errmsg = insert_normal (fields->f_uimm5, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), 11, 5, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_UIMM16 :
      errmsg = insert_normal (fields->f_uimm16, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), 16, 16, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_HASH :
      errmsg = insert_normal (fields->f_nil, 0, 0, 0, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_HI16 :
      errmsg = insert_normal (fields->f_hi16, 0|(1<<CGEN_OPERAND_SIGN_OPT)|(1<<CGEN_OPERAND_UNSIGNED), 16, 16, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_SLO16 :
      errmsg = insert_normal (fields->f_simm16, 0, 16, 16, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_ULO16 :
      errmsg = insert_normal (fields->f_uimm16, 0|(1<<CGEN_OPERAND_UNSIGNED), 16, 16, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_UIMM24 :
      errmsg = insert_normal (fields->f_uimm24, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_ABS_ADDR)|(1<<CGEN_OPERAND_UNSIGNED), 8, 24, CGEN_FIELDS_BITSIZE (fields), buffer);
      break;
    case M32R_OPERAND_DISP8 :
      {
        long value = ((fields->f_disp8) >> (2));
        errmsg = insert_normal (value, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), 8, 8, CGEN_FIELDS_BITSIZE (fields), buffer);
      }
      break;
    case M32R_OPERAND_DISP16 :
      {
        long value = ((fields->f_disp16) >> (2));
        errmsg = insert_normal (value, 0|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), 16, 16, CGEN_FIELDS_BITSIZE (fields), buffer);
      }
      break;
    case M32R_OPERAND_DISP24 :
      {
        long value = ((fields->f_disp24) >> (2));
        errmsg = insert_normal (value, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), 8, 24, CGEN_FIELDS_BITSIZE (fields), buffer);
      }
      break;

    default :
      fprintf (stderr, "Unrecognized field %d while building insn.\n",
	       opindex);
      abort ();
  }

  return errmsg;
}

cgen_parse_fn * m32r_cgen_parse_handlers[] = 
{
  0, /* default */
  parse_insn_normal,
};

cgen_insert_fn * m32r_cgen_insert_handlers[] = 
{
  0, /* default */
  insert_insn_normal,
};

void
m32r_cgen_init_asm (mach, endian)
     int mach;
     enum cgen_endian endian;
{
  m32r_cgen_init_tables (mach);
  cgen_set_cpu (& m32r_cgen_opcode_data, mach, endian);
  cgen_asm_init ();
}


/* Default insertion routine.

   ATTRS is a mask of the boolean attributes.
   LENGTH is the length of VALUE in bits.
   TOTAL_LENGTH is the total length of the insn (currently 8,16,32).

   The result is an error message or NULL if success.  */

/* ??? This duplicates functionality with bfd's howto table and
   bfd_install_relocation.  */
/* ??? For architectures where insns can be representable as ints,
   store insn in `field' struct and add registers, etc. while parsing?  */

static const char *
insert_normal (value, attrs, start, length, total_length, buffer)
     long value;
     unsigned int attrs;
     int start;
     int length;
     int total_length;
     char * buffer;
{
  bfd_vma x;
  static char buf[100];

  /* Ensure VALUE will fit.  */
  if ((attrs & (1 << CGEN_OPERAND_UNSIGNED)) != 0)
    {
      unsigned long max = (1 << length) - 1;
      if ((unsigned long) value > max)
	{
	  const char *err = "operand out of range (%lu not between 0 and %lu)";

	  sprintf (buf, err, value, max);
	  return buf;
	}
    }
  else
    {
      long min = - (1 << (length - 1));
      long max = (1 << (length - 1)) - 1;
      if (value < min || value > max)
	{
	  const char *err = "operand out of range (%ld not between %ld and %ld)";

	  sprintf (buf, err, value, min, max);
	  return buf;
	}
    }

#if 0 /*def CGEN_INT_INSN*/
  *buffer |= ((value & ((1 << length) - 1))
	      << (total_length - (start + length)));
#else
  switch (total_length)
    {
    case 8:
      x = * (unsigned char *) buffer;
      break;
    case 16:
      if (CGEN_CURRENT_ENDIAN == CGEN_ENDIAN_BIG)
	x = bfd_getb16 (buffer);
      else
	x = bfd_getl16 (buffer);
      break;
    case 32:
      if (CGEN_CURRENT_ENDIAN == CGEN_ENDIAN_BIG)
	x = bfd_getb32 (buffer);
      else
	x = bfd_getl32 (buffer);
      break;
    default :
      abort ();
    }

  x |= ((value & ((1 << length) - 1))
	<< (total_length - (start + length)));

  switch (total_length)
    {
    case 8:
      * buffer = value;
      break;
    case 16:
      if (CGEN_CURRENT_ENDIAN == CGEN_ENDIAN_BIG)
	bfd_putb16 (x, buffer);
      else
	bfd_putl16 (x, buffer);
      break;
    case 32:
      if (CGEN_CURRENT_ENDIAN == CGEN_ENDIAN_BIG)
	bfd_putb32 (x, buffer);
      else
	bfd_putl32 (x, buffer);
      break;
    default :
      abort ();
    }
#endif

  return NULL;
}

/* Default insn parser.

   The syntax string is scanned and operands are parsed and stored in FIELDS.
   Relocs are queued as we go via other callbacks.

   ??? Note that this is currently an all-or-nothing parser.  If we fail to
   parse the instruction, we return 0 and the caller will start over from
   the beginning.  Backtracking will be necessary in parsing subexpressions,
   but that can be handled there.  Not handling backtracking here may get
   expensive in the case of the m68k.  Deal with later.

   Returns NULL for success, an error message for failure.
*/

static const char *
parse_insn_normal (insn, strp, fields)
     const CGEN_INSN * insn;
     const char ** strp;
     CGEN_FIELDS * fields;
{
  const CGEN_SYNTAX * syntax = CGEN_INSN_SYNTAX (insn);
  const char * str = *strp;
  const char * errmsg;
  const char * p;
  const unsigned char * syn;
#ifdef CGEN_MNEMONIC_OPERANDS
  int past_opcode_p;
#endif

  /* For now we assume the mnemonic is first (there are no leading operands).
     We can parse it without needing to set up operand parsing.  */
  p = CGEN_INSN_MNEMONIC (insn);
  while (* p && * p == * str)
    ++ p, ++ str;
  if (* p || (* str && !isspace (* str)))
    return "unrecognized instruction";

  CGEN_INIT_PARSE ();
  cgen_init_parse_operand ();
#ifdef CGEN_MNEMONIC_OPERANDS
  past_opcode_p = 0;
#endif

  /* We don't check for (*str != '\0') here because we want to parse
     any trailing fake arguments in the syntax string.  */
  syn = CGEN_SYNTAX_STRING (CGEN_INSN_SYNTAX (insn));

  /* Mnemonics come first for now, ensure valid string.  */
  if (! CGEN_SYNTAX_MNEMONIC_P (* syn))
    abort ();

  ++syn;

  while (* syn != 0)
    {
      /* Non operand chars must match exactly.  */
      /* FIXME: Need to better handle whitespace.  */
      if (CGEN_SYNTAX_CHAR_P (* syn))
	{
	  if (*str == CGEN_SYNTAX_CHAR (* syn))
	    {
#ifdef CGEN_MNEMONIC_OPERANDS
	      if (* syn == ' ')
		past_opcode_p = 1;
#endif
	      ++ syn;
	      ++ str;
	    }
	  else
	    {
	      /* Syntax char didn't match.  Can't be this insn.  */
	      /* FIXME: would like to return something like
		 "expected char `c'" */
	      return "syntax error";
	    }
	  continue;
	}

      /* We have an operand of some sort.  */
      errmsg = m32r_cgen_parse_operand (CGEN_SYNTAX_FIELD (*syn),
					 &str, fields);
      if (errmsg)
	return errmsg;

      /* Done with this operand, continue with next one.  */
      ++ syn;
    }

  /* If we're at the end of the syntax string, we're done.  */
  if (* syn == '\0')
    {
      /* FIXME: For the moment we assume a valid `str' can only contain
	 blanks now.  IE: We needn't try again with a longer version of
	 the insn and it is assumed that longer versions of insns appear
	 before shorter ones (eg: lsr r2,r3,1 vs lsr r2,r3).  */
      while (isspace (* str))
	++ str;

      if (* str != '\0')
	return "junk at end of line"; /* FIXME: would like to include `str' */

      return NULL;
    }

  /* We couldn't parse it.  */
  return "unrecognized instruction";
}

/* Default insn builder (insert handler).
   The instruction is recorded in target byte order.
   The result is an error message or NULL if success.  */
/* FIXME: change buffer to char *?  */

static const char *
insert_insn_normal (insn, fields, buffer)
     const CGEN_INSN * insn;
     CGEN_FIELDS * fields;
     cgen_insn_t * buffer;
{
  const CGEN_SYNTAX * syntax = CGEN_INSN_SYNTAX (insn);
  bfd_vma value;
  const unsigned char * syn;

  CGEN_INIT_INSERT ();
  value = CGEN_INSN_VALUE (insn);

  /* If we're recording insns as numbers (rather than a string of bytes),
     target byte order handling is deferred until later.  */
#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#if 0 /*def CGEN_INT_INSN*/
  *buffer = value;
#else
  switch (min (CGEN_BASE_INSN_BITSIZE, CGEN_FIELDS_BITSIZE (fields)))
    {
    case 8:
      * buffer = value;
      break;
    case 16:
      if (CGEN_CURRENT_ENDIAN == CGEN_ENDIAN_BIG)
	bfd_putb16 (value, (char *) buffer);
      else
	bfd_putl16 (value, (char *) buffer);
      break;
    case 32:
      if (CGEN_CURRENT_ENDIAN == CGEN_ENDIAN_BIG)
	bfd_putb32 (value, (char *) buffer);
      else
	bfd_putl32 (value, (char *) buffer);
      break;
    default:
      abort ();
    }
#endif

  /* ??? Rather than scanning the syntax string again, we could store
     in `fields' a null terminated list of the fields that are present.  */

  for (syn = CGEN_SYNTAX_STRING (syntax); * syn != '\0'; ++ syn)
    {
      const char *errmsg;

      if (CGEN_SYNTAX_CHAR_P (* syn))
	continue;

      errmsg = m32r_cgen_insert_operand (CGEN_SYNTAX_FIELD (*syn), fields,
					   (char *) buffer);
      if (errmsg)
	return errmsg;
    }

  return NULL;
}

/* Main entry point.
   This routine is called for each instruction to be assembled.
   STR points to the insn to be assembled.
   We assume all necessary tables have been initialized.
   The result is a pointer to the insn's entry in the opcode table,
   or NULL if an error occured (an error message will have already been
   printed).  */

const CGEN_INSN *
m32r_cgen_assemble_insn (str, fields, buf, errmsg)
     const char * str;
     CGEN_FIELDS * fields;
     cgen_insn_t * buf;
     char ** errmsg;
{
  const char * start;
  CGEN_INSN_LIST * ilist;

  /* Skip leading white space.  */
  while (isspace (* str))
    ++ str;

  /* The instructions are stored in hashed lists.
     Get the first in the list.  */
  ilist = CGEN_ASM_LOOKUP_INSN (str);

  /* Keep looking until we find a match.  */

  start = str;
  for ( ; ilist != NULL ; ilist = CGEN_ASM_NEXT_INSN (ilist))
    {
      const CGEN_INSN *insn = ilist->insn;

#if 0 /* not needed as unsupported opcodes shouldn't be in the hash lists */
      /* Is this insn supported by the selected cpu?  */
      if (! m32r_cgen_insn_supported (insn))
	continue;
#endif

#if 1 /* FIXME: wip */
      /* If the RELAX attribute is set, this is an insn that shouldn't be
	 chosen immediately.  Instead, it is used during assembler/linker
	 relaxation if possible.  */
      if (CGEN_INSN_ATTR (insn, CGEN_INSN_RELAX) != 0)
	continue;
#endif

      str = start;

      /* Record a default length for the insn.  This will get set to the
	 correct value while parsing.  */
      /* FIXME: wip */
      CGEN_FIELDS_BITSIZE (fields) = CGEN_INSN_BITSIZE (insn);

      if (! CGEN_PARSE_FN (insn) (insn, & str, fields))
	{
	  if (CGEN_INSERT_FN (insn) (insn, fields, buf) != NULL)
	    continue;
	  /* It is up to the caller to actually output the insn and any
	     queued relocs.  */
	  return insn;
	}

      /* Try the next entry.  */
    }

  /* FIXME: We can return a better error message than this.
     Need to track why it failed and pick the right one.  */
  {
    static char errbuf[100];
    sprintf (errbuf, "bad instruction `%.50s%s'",
	     start, strlen (start) > 50 ? "..." : "");
    *errmsg = errbuf;
    return NULL;
  }
}

#if 0 /* This calls back to GAS which we can't do without care.  */

/* Record each member of OPVALS in the assembler's symbol table.
   This lets GAS parse registers for us.
   ??? Interesting idea but not currently used.  */

/* Record each member of OPVALS in the assembler's symbol table.
   FIXME: Not currently used.  */

void
m32r_cgen_asm_hash_keywords (opvals)
     CGEN_KEYWORD * opvals;
{
  CGEN_KEYWORD_SEARCH search = cgen_keyword_search_init (opvals, NULL);
  const CGEN_KEYWORD_ENTRY * ke;

  while ((ke = cgen_keyword_search_next (& search)) != NULL)
    {
#if 0 /* Unnecessary, should be done in the search routine.  */
      if (! m32r_cgen_opval_supported (ke))
	continue;
#endif
      cgen_asm_record_register (ke->name, ke->value);
    }
}

#endif /* 0 */
