/* Disassembler interface for targets using CGEN. -*- C -*-
   CGEN: Cpu tools GENerator

This file is used to generate m32r-dis.c.

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
#include <stdio.h>
#include "ansidecl.h"
#include "dis-asm.h"
#include "bfd.h"
#include "symcat.h"
#include "m32r-opc.h"

/* ??? The layout of this stuff is still work in progress.
   For speed in assembly/disassembly, we use inline functions.  That of course
   will only work for GCC.  When this stuff is finished, we can decide whether
   to keep the inline functions (and only get the performance increase when
   compiled with GCC), or switch to macros, or use something else.
*/

/* Default text to print if an instruction isn't recognized.  */
#define UNKNOWN_INSN_MSG "*unknown*"

/* FIXME: Machine generate.  */
#ifndef CGEN_PCREL_OFFSET
#define CGEN_PCREL_OFFSET 0
#endif

static int print_insn PARAMS ((bfd_vma, disassemble_info *, char *, int));

static int extract_normal
     PARAMS ((PTR, cgen_insn_t, unsigned int, int, int, int, long *));
static void print_normal
     PARAMS ((PTR, long, unsigned int, unsigned long, int));
static void print_keyword
     PARAMS ((PTR, CGEN_KEYWORD *, long, unsigned int));
static int extract_insn_normal
     PARAMS ((const CGEN_INSN *, void *, cgen_insn_t, CGEN_FIELDS *));
static void print_insn_normal
     PARAMS ((void *, const CGEN_INSN *, CGEN_FIELDS *, bfd_vma, int));

/* -- disassembler routines inserted here */
/* -- dis.c */

/* Immediate values are prefixed with '#'.  */

#define CGEN_PRINT_NORMAL(info, value, attrs, pc, length) \
do { \
  if ((attrs) & (1 << CGEN_OPERAND_HASH_PREFIX)) \
    (*info->fprintf_func) (info->stream, "#"); \
} while (0)

/* Handle '#' prefixes as operands.  */

static void
print_hash (dis_info, value, attrs, pc, length)
     PTR dis_info;
     long value;
     unsigned int attrs;
     unsigned long pc; /* FIXME: should be bfd_vma */
     int length;
{
  disassemble_info *info = dis_info;
  (*info->fprintf_func) (info->stream, "#");
}

#undef CGEN_PRINT_INSN
#define CGEN_PRINT_INSN my_print_insn

static int
my_print_insn (pc, info, buf, buflen)
     bfd_vma pc;
     disassemble_info *info;
     char *buf;
     int buflen;
{
  /* 32 bit insn?  */
  if ((pc & 3) == 0 && (buf[0] & 0x80) != 0)
    return print_insn (pc, info, buf, buflen);

  /* Print the first insn.  */
  if ((pc & 3) == 0)
    {
      if (print_insn (pc, info, buf, 16) == 0)
	(*info->fprintf_func) (info->stream, UNKNOWN_INSN_MSG);
      buf += 2;
    }

  if (buf[0] & 0x80)
    {
      /* Parallel.  */
      (*info->fprintf_func) (info->stream, " || ");
      buf[0] &= 0x7f;
    }
  else
    (*info->fprintf_func) (info->stream, " -> ");

  /* The "& 3" is to ensure the branch address is computed correctly
     [if it is a branch].  */
  if (print_insn (pc & ~ (bfd_vma) 3, info, buf, 16) == 0)
    (*info->fprintf_func) (info->stream, UNKNOWN_INSN_MSG);

  return (pc & 3) ? 2 : 4;
}

/* -- */

/* Main entry point for operand extraction.

   This function is basically just a big switch statement.  Earlier versions
   used tables to look up the function to use, but
   - if the table contains both assembler and disassembler functions then
     the disassembler contains much of the assembler and vice-versa,
   - there's a lot of inlining possibilities as things grow,
   - using a switch statement avoids the function call overhead.

   This function could be moved into `print_insn_normal', but keeping it
   separate makes clear the interface between `print_insn_normal' and each of
   the handlers.
*/

int
m32r_cgen_extract_operand (opindex, buf_ctrl, insn_value, fields)
     int opindex;
     PTR buf_ctrl;
     cgen_insn_t insn_value;
     CGEN_FIELDS * fields;
{
  int length;

  switch (opindex)
    {
    case M32R_OPERAND_SR :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_r2);
      break;
    case M32R_OPERAND_DR :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 4, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_r1);
      break;
    case M32R_OPERAND_SRC1 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 4, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_r1);
      break;
    case M32R_OPERAND_SRC2 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_r2);
      break;
    case M32R_OPERAND_SCR :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_r2);
      break;
    case M32R_OPERAND_DCR :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 4, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_r1);
      break;
    case M32R_OPERAND_SIMM8 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_HASH_PREFIX), 8, 8, CGEN_FIELDS_BITSIZE (fields), & fields->f_simm8);
      break;
    case M32R_OPERAND_SIMM16 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_HASH_PREFIX), 16, 16, CGEN_FIELDS_BITSIZE (fields), & fields->f_simm16);
      break;
    case M32R_OPERAND_UIMM4 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), 12, 4, CGEN_FIELDS_BITSIZE (fields), & fields->f_uimm4);
      break;
    case M32R_OPERAND_UIMM5 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), 11, 5, CGEN_FIELDS_BITSIZE (fields), & fields->f_uimm5);
      break;
    case M32R_OPERAND_UIMM16 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), 16, 16, CGEN_FIELDS_BITSIZE (fields), & fields->f_uimm16);
      break;
    case M32R_OPERAND_HASH :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0, 0, 0, CGEN_FIELDS_BITSIZE (fields), & fields->f_nil);
      break;
    case M32R_OPERAND_HI16 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_SIGN_OPT)|(1<<CGEN_OPERAND_UNSIGNED), 16, 16, CGEN_FIELDS_BITSIZE (fields), & fields->f_hi16);
      break;
    case M32R_OPERAND_SLO16 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0, 16, 16, CGEN_FIELDS_BITSIZE (fields), & fields->f_simm16);
      break;
    case M32R_OPERAND_ULO16 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_UNSIGNED), 16, 16, CGEN_FIELDS_BITSIZE (fields), & fields->f_uimm16);
      break;
    case M32R_OPERAND_UIMM24 :
      length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_ABS_ADDR)|(1<<CGEN_OPERAND_UNSIGNED), 8, 24, CGEN_FIELDS_BITSIZE (fields), & fields->f_uimm24);
      break;
    case M32R_OPERAND_DISP8 :
      {
        long value;
        length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), 8, 8, CGEN_FIELDS_BITSIZE (fields), & value);
        fields->f_disp8 = ((value) << (2));
      }
      break;
    case M32R_OPERAND_DISP16 :
      {
        long value;
        length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), 16, 16, CGEN_FIELDS_BITSIZE (fields), & value);
        fields->f_disp16 = ((value) << (2));
      }
      break;
    case M32R_OPERAND_DISP24 :
      {
        long value;
        length = extract_normal (NULL /*FIXME*/, insn_value, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), 8, 24, CGEN_FIELDS_BITSIZE (fields), & value);
        fields->f_disp24 = ((value) << (2));
      }
      break;

    default :
      fprintf (stderr, "Unrecognized field %d while decoding insn.\n",
	       opindex);
      abort ();
    }

  return length;
}

/* Main entry point for printing operands.

   This function is basically just a big switch statement.  Earlier versions
   used tables to look up the function to use, but
   - if the table contains both assembler and disassembler functions then
     the disassembler contains much of the assembler and vice-versa,
   - there's a lot of inlining possibilities as things grow,
   - using a switch statement avoids the function call overhead.

   This function could be moved into `print_insn_normal', but keeping it
   separate makes clear the interface between `print_insn_normal' and each of
   the handlers.
*/

void
m32r_cgen_print_operand (opindex, info, fields, attrs, pc, length)
     int opindex;
     disassemble_info * info;
     CGEN_FIELDS * fields;
     void const * attrs;
     bfd_vma pc;
     int length;
{
  switch (opindex)
    {
    case M32R_OPERAND_SR :
      print_keyword (info, & m32r_cgen_opval_h_gr, fields->f_r2, 0|(1<<CGEN_OPERAND_UNSIGNED));
      break;
    case M32R_OPERAND_DR :
      print_keyword (info, & m32r_cgen_opval_h_gr, fields->f_r1, 0|(1<<CGEN_OPERAND_UNSIGNED));
      break;
    case M32R_OPERAND_SRC1 :
      print_keyword (info, & m32r_cgen_opval_h_gr, fields->f_r1, 0|(1<<CGEN_OPERAND_UNSIGNED));
      break;
    case M32R_OPERAND_SRC2 :
      print_keyword (info, & m32r_cgen_opval_h_gr, fields->f_r2, 0|(1<<CGEN_OPERAND_UNSIGNED));
      break;
    case M32R_OPERAND_SCR :
      print_keyword (info, & m32r_cgen_opval_h_cr, fields->f_r2, 0|(1<<CGEN_OPERAND_UNSIGNED));
      break;
    case M32R_OPERAND_DCR :
      print_keyword (info, & m32r_cgen_opval_h_cr, fields->f_r1, 0|(1<<CGEN_OPERAND_UNSIGNED));
      break;
    case M32R_OPERAND_SIMM8 :
      print_normal (info, fields->f_simm8, 0|(1<<CGEN_OPERAND_HASH_PREFIX), pc, length);
      break;
    case M32R_OPERAND_SIMM16 :
      print_normal (info, fields->f_simm16, 0|(1<<CGEN_OPERAND_HASH_PREFIX), pc, length);
      break;
    case M32R_OPERAND_UIMM4 :
      print_normal (info, fields->f_uimm4, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), pc, length);
      break;
    case M32R_OPERAND_UIMM5 :
      print_normal (info, fields->f_uimm5, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), pc, length);
      break;
    case M32R_OPERAND_UIMM16 :
      print_normal (info, fields->f_uimm16, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_UNSIGNED), pc, length);
      break;
    case M32R_OPERAND_HASH :
      print_hash (info, fields->f_nil, 0, pc, length);
      break;
    case M32R_OPERAND_HI16 :
      print_normal (info, fields->f_hi16, 0|(1<<CGEN_OPERAND_SIGN_OPT)|(1<<CGEN_OPERAND_UNSIGNED), pc, length);
      break;
    case M32R_OPERAND_SLO16 :
      print_normal (info, fields->f_simm16, 0, pc, length);
      break;
    case M32R_OPERAND_ULO16 :
      print_normal (info, fields->f_uimm16, 0|(1<<CGEN_OPERAND_UNSIGNED), pc, length);
      break;
    case M32R_OPERAND_UIMM24 :
      print_normal (info, fields->f_uimm24, 0|(1<<CGEN_OPERAND_HASH_PREFIX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_ABS_ADDR)|(1<<CGEN_OPERAND_UNSIGNED), pc, length);
      break;
    case M32R_OPERAND_DISP8 :
      print_normal (info, fields->f_disp8, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), pc, length);
      break;
    case M32R_OPERAND_DISP16 :
      print_normal (info, fields->f_disp16, 0|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), pc, length);
      break;
    case M32R_OPERAND_DISP24 :
      print_normal (info, fields->f_disp24, 0|(1<<CGEN_OPERAND_RELAX)|(1<<CGEN_OPERAND_RELOC)|(1<<CGEN_OPERAND_PCREL_ADDR), pc, length);
      break;

    default :
      fprintf (stderr, "Unrecognized field %d while printing insn.\n",
	       opindex);
    abort ();
  }
}

cgen_extract_fn * m32r_cgen_extract_handlers[] = 
{
  0, /* default */
  extract_insn_normal,
};

cgen_print_fn * m32r_cgen_print_handlers[] = 
{
  0, /* default */
  print_insn_normal,
};


void
m32r_cgen_init_dis (mach, endian)
     int mach;
     enum cgen_endian endian;
{
  m32r_cgen_init_tables (mach);
  cgen_set_cpu (& m32r_cgen_opcode_data, mach, endian);
  cgen_dis_init ();
}


/* Default extraction routine.

   ATTRS is a mask of the boolean attributes.  We only need `unsigned',
   but for generality we take a bitmask of all of them.  */

static int
extract_normal (buf_ctrl, insn_value, attrs, start, length, total_length, valuep)
     PTR buf_ctrl;
     cgen_insn_t insn_value;
     unsigned int attrs;
     int start, length, total_length;
     long *valuep;
{
  long value;

#ifdef CGEN_INT_INSN
#if 0
  value = ((insn_value >> (CGEN_BASE_INSN_BITSIZE - (start + length)))
	   & ((1 << length) - 1));
#else
  value = ((insn_value >> (total_length - (start + length)))
	   & ((1 << length) - 1));
#endif
  if (! (attrs & CGEN_ATTR_MASK (CGEN_OPERAND_UNSIGNED))
      && (value & (1 << (length - 1))))
    value -= 1 << length;
#else
  /* FIXME: unfinished */
#endif

  *valuep = value;

  /* FIXME: for now */
  return 1;
}

/* Default print handler.  */

static void
print_normal (dis_info, value, attrs, pc, length)
     PTR dis_info;
     long value;
     unsigned int attrs;
     unsigned long pc; /* FIXME: should be bfd_vma */
     int length;
{
  disassemble_info *info = dis_info;

#ifdef CGEN_PRINT_NORMAL
  CGEN_PRINT_NORMAL (info, value, attrs, pc, length);
#endif

  /* Print the operand as directed by the attributes.  */
  if (CGEN_BOOL_ATTR (attrs, CGEN_OPERAND_FAKE))
    ; /* nothing to do (??? at least not yet) */
  else if (CGEN_BOOL_ATTR (attrs, CGEN_OPERAND_PCREL_ADDR))
    (*info->print_address_func) (pc + CGEN_PCREL_OFFSET + value, info);
  /* ??? Not all cases of this are currently caught.  */
  else if (CGEN_BOOL_ATTR (attrs, CGEN_OPERAND_ABS_ADDR))
    /* FIXME: Why & 0xffffffff?  */
    (*info->print_address_func) ((bfd_vma) value & 0xffffffff, info);
  else if (CGEN_BOOL_ATTR (attrs, CGEN_OPERAND_UNSIGNED))
    (*info->fprintf_func) (info->stream, "0x%lx", value);
  else
    (*info->fprintf_func) (info->stream, "%ld", value);
}

/* Keyword print handler.  */

static void
print_keyword (dis_info, keyword_table, value, attrs)
     PTR dis_info;
     CGEN_KEYWORD *keyword_table;
     long value;
     unsigned int attrs;
{
  disassemble_info *info = dis_info;
  const CGEN_KEYWORD_ENTRY *ke;

  ke = cgen_keyword_lookup_value (keyword_table, value);
  if (ke != NULL)
    (*info->fprintf_func) (info->stream, "%s", ke->name);
  else
    (*info->fprintf_func) (info->stream, "???");
}

/* Default insn extractor.

   The extracted fields are stored in DIS_FLDS.
   BUF_CTRL is used to handle reading variable length insns (FIXME: not done).
   Return the length of the insn in bits, or 0 if no match.  */

static int
extract_insn_normal (insn, buf_ctrl, insn_value, fields)
     const CGEN_INSN *insn;
     PTR buf_ctrl;
     cgen_insn_t insn_value;
     CGEN_FIELDS *fields;
{
  const CGEN_SYNTAX *syntax = CGEN_INSN_SYNTAX (insn);
  const unsigned char *syn;

  CGEN_FIELDS_BITSIZE (fields) = CGEN_INSN_BITSIZE (insn);

  CGEN_INIT_EXTRACT ();

  for (syn = CGEN_SYNTAX_STRING (syntax); *syn; ++syn)
    {
      int length;

      if (CGEN_SYNTAX_CHAR_P (*syn))
	continue;

      length = m32r_cgen_extract_operand (CGEN_SYNTAX_FIELD (*syn),
					    buf_ctrl, insn_value, fields);
      if (length == 0)
	return 0;
    }

  /* We recognized and successfully extracted this insn.  */
  return CGEN_INSN_BITSIZE (insn);
}

/* Default insn printer.

   DIS_INFO is defined as `PTR' so the disassembler needn't know anything
   about disassemble_info.
*/

static void
print_insn_normal (dis_info, insn, fields, pc, length)
     PTR dis_info;
     const CGEN_INSN *insn;
     CGEN_FIELDS *fields;
     bfd_vma pc;
     int length;
{
  const CGEN_SYNTAX *syntax = CGEN_INSN_SYNTAX (insn);
  disassemble_info *info = dis_info;
  const unsigned char *syn;

  CGEN_INIT_PRINT ();

  for (syn = CGEN_SYNTAX_STRING (syntax); *syn; ++syn)
    {
      if (CGEN_SYNTAX_MNEMONIC_P (*syn))
	{
	  (*info->fprintf_func) (info->stream, "%s", CGEN_INSN_MNEMONIC (insn));
	  continue;
	}
      if (CGEN_SYNTAX_CHAR_P (*syn))
	{
	  (*info->fprintf_func) (info->stream, "%c", CGEN_SYNTAX_CHAR (*syn));
	  continue;
	}

      /* We have an operand.  */
      m32r_cgen_print_operand (CGEN_SYNTAX_FIELD (*syn), info,
				fields, CGEN_INSN_ATTRS (insn), pc, length);
    }
}

/* Default value for CGEN_PRINT_INSN.
   Given BUFLEN bits (target byte order) read into BUF, look up the
   insn in the instruction table and disassemble it.

   The result is the size of the insn in bytes.  */

#ifndef CGEN_PRINT_INSN
#define CGEN_PRINT_INSN print_insn
#endif

static int
print_insn (pc, info, buf, buflen)
     bfd_vma pc;
     disassemble_info *info;
     char *buf;
     int buflen;
{
  unsigned long insn_value;
  const CGEN_INSN_LIST *insn_list;

  switch (buflen)
    {
    case 8:
      insn_value = buf[0];
      break;
    case 16:
      insn_value = info->endian == BFD_ENDIAN_BIG ? bfd_getb16 (buf) : bfd_getl16 (buf);
      break;
    case 32:
      insn_value = info->endian == BFD_ENDIAN_BIG ? bfd_getb32 (buf) : bfd_getl32 (buf);
      break;
    default:
      abort ();
    }

  /* The instructions are stored in hash lists.
     Pick the first one and keep trying until we find the right one.  */

  insn_list = CGEN_DIS_LOOKUP_INSN (buf, insn_value);
  while (insn_list != NULL)
    {
      const CGEN_INSN *insn = insn_list->insn;
      CGEN_FIELDS fields;
      int length;

#if 0 /* not needed as insn shouldn't be in hash lists if not supported */
      /* Supported by this cpu?  */
      if (! m32r_cgen_insn_supported (insn))
	continue;
#endif

      /* Basic bit mask must be correct.  */
      /* ??? May wish to allow target to defer this check until the extract
	 handler.  */
      if ((insn_value & CGEN_INSN_MASK (insn)) == CGEN_INSN_VALUE (insn))
	{
	  /* Printing is handled in two passes.  The first pass parses the
	     machine insn and extracts the fields.  The second pass prints
	     them.  */

	  length = (*CGEN_EXTRACT_FN (insn)) (insn, NULL, insn_value, &fields);
	  if (length > 0)
	    {
	      (*CGEN_PRINT_FN (insn)) (info, insn, &fields, pc, length);
	      /* length is in bits, result is in bytes */
	      return length / 8;
	    }
	}

      insn_list = CGEN_DIS_NEXT_INSN (insn_list);
    }

  return 0;
}

/* Main entry point.
   Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction (in bytes).  */

int
print_insn_m32r (pc, info)
     bfd_vma pc;
     disassemble_info *info;
{
  char buffer[CGEN_MAX_INSN_SIZE];
  int status, length;
  static int initialized = 0;
  static int current_mach = 0;
  static int current_big_p = 0;
  int mach = info->mach;
  int big_p = info->endian == BFD_ENDIAN_BIG;

  /* If we haven't initialized yet, or if we've switched cpu's, initialize.  */
  if (!initialized || mach != current_mach || big_p != current_big_p)
    {
      initialized = 1;
      current_mach = mach;
      current_big_p = big_p;
      m32r_cgen_init_dis (mach, big_p ? CGEN_ENDIAN_BIG : CGEN_ENDIAN_LITTLE);
    }

  /* Read enough of the insn so we can look it up in the hash lists.  */

  status = (*info->read_memory_func) (pc, buffer, CGEN_BASE_INSN_SIZE, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, pc, info);
      return -1;
    }

  /* We try to have as much common code as possible.
     But at this point some targets need to take over.  */
  /* ??? Some targets may need a hook elsewhere.  Try to avoid this,
     but if not possible try to move this hook elsewhere rather than
     have two hooks.  */
  length = CGEN_PRINT_INSN (pc, info, buffer, CGEN_BASE_INSN_BITSIZE);
  if (length)
    return length;

  (*info->fprintf_func) (info->stream, UNKNOWN_INSN_MSG);
  return CGEN_DEFAULT_INSN_SIZE;
}
