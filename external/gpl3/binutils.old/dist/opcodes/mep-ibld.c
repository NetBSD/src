/* DO NOT EDIT!  -*- buffer-read-only: t -*- vi:set ro:  */
/* Instruction building/extraction support for mep. -*- C -*-

   THIS FILE IS MACHINE GENERATED WITH CGEN: Cpu tools GENerator.
   - the resultant file is machine generated, cgen-ibld.in isn't

   Copyright (C) 1996-2022 Free Software Foundation, Inc.

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

/* ??? Eventually more and more of this stuff can go to cpu-independent files.
   Keep that in mind.  */

#include "sysdep.h"
#include <stdio.h>
#include "ansidecl.h"
#include "dis-asm.h"
#include "bfd.h"
#include "symcat.h"
#include "mep-desc.h"
#include "mep-opc.h"
#include "cgen/basic-modes.h"
#include "opintl.h"
#include "safe-ctype.h"

#undef  min
#define min(a,b) ((a) < (b) ? (a) : (b))
#undef  max
#define max(a,b) ((a) > (b) ? (a) : (b))

/* Used by the ifield rtx function.  */
#define FLD(f) (fields->f)

static const char * insert_normal
  (CGEN_CPU_DESC, long, unsigned int, unsigned int, unsigned int,
   unsigned int, unsigned int, unsigned int, CGEN_INSN_BYTES_PTR);
static const char * insert_insn_normal
  (CGEN_CPU_DESC, const CGEN_INSN *,
   CGEN_FIELDS *, CGEN_INSN_BYTES_PTR, bfd_vma);
static int extract_normal
  (CGEN_CPU_DESC, CGEN_EXTRACT_INFO *, CGEN_INSN_INT,
   unsigned int, unsigned int, unsigned int, unsigned int,
   unsigned int, unsigned int, bfd_vma, long *);
static int extract_insn_normal
  (CGEN_CPU_DESC, const CGEN_INSN *, CGEN_EXTRACT_INFO *,
   CGEN_INSN_INT, CGEN_FIELDS *, bfd_vma);
#if CGEN_INT_INSN_P
static void put_insn_int_value
  (CGEN_CPU_DESC, CGEN_INSN_BYTES_PTR, int, int, CGEN_INSN_INT);
#endif
#if ! CGEN_INT_INSN_P
static CGEN_INLINE void insert_1
  (CGEN_CPU_DESC, unsigned long, int, int, int, unsigned char *);
static CGEN_INLINE int fill_cache
  (CGEN_CPU_DESC, CGEN_EXTRACT_INFO *,  int, int, bfd_vma);
static CGEN_INLINE long extract_1
  (CGEN_CPU_DESC, CGEN_EXTRACT_INFO *, int, int, int, unsigned char *, bfd_vma);
#endif

/* Operand insertion.  */

#if ! CGEN_INT_INSN_P

/* Subroutine of insert_normal.  */

static CGEN_INLINE void
insert_1 (CGEN_CPU_DESC cd,
	  unsigned long value,
	  int start,
	  int length,
	  int word_length,
	  unsigned char *bufp)
{
  unsigned long x, mask;
  int shift;

  x = cgen_get_insn_value (cd, bufp, word_length, cd->endian);

  /* Written this way to avoid undefined behaviour.  */
  mask = (1UL << (length - 1) << 1) - 1;
  if (CGEN_INSN_LSB0_P)
    shift = (start + 1) - length;
  else
    shift = (word_length - (start + length));
  x = (x & ~(mask << shift)) | ((value & mask) << shift);

  cgen_put_insn_value (cd, bufp, word_length, (bfd_vma) x, cd->endian);
}

#endif /* ! CGEN_INT_INSN_P */

/* Default insertion routine.

   ATTRS is a mask of the boolean attributes.
   WORD_OFFSET is the offset in bits from the start of the insn of the value.
   WORD_LENGTH is the length of the word in bits in which the value resides.
   START is the starting bit number in the word, architecture origin.
   LENGTH is the length of VALUE in bits.
   TOTAL_LENGTH is the total length of the insn in bits.

   The result is an error message or NULL if success.  */

/* ??? This duplicates functionality with bfd's howto table and
   bfd_install_relocation.  */
/* ??? This doesn't handle bfd_vma's.  Create another function when
   necessary.  */

static const char *
insert_normal (CGEN_CPU_DESC cd,
	       long value,
	       unsigned int attrs,
	       unsigned int word_offset,
	       unsigned int start,
	       unsigned int length,
	       unsigned int word_length,
	       unsigned int total_length,
	       CGEN_INSN_BYTES_PTR buffer)
{
  static char errbuf[100];
  unsigned long mask;

  /* If LENGTH is zero, this operand doesn't contribute to the value.  */
  if (length == 0)
    return NULL;

  /* Written this way to avoid undefined behaviour.  */
  mask = (1UL << (length - 1) << 1) - 1;

  if (word_length > 8 * sizeof (CGEN_INSN_INT))
    abort ();

  /* For architectures with insns smaller than the base-insn-bitsize,
     word_length may be too big.  */
  if (cd->min_insn_bitsize < cd->base_insn_bitsize)
    {
      if (word_offset == 0
	  && word_length > total_length)
	word_length = total_length;
    }

  /* Ensure VALUE will fit.  */
  if (CGEN_BOOL_ATTR (attrs, CGEN_IFLD_SIGN_OPT))
    {
      long minval = - (1UL << (length - 1));
      unsigned long maxval = mask;

      if ((value > 0 && (unsigned long) value > maxval)
	  || value < minval)
	{
	  /* xgettext:c-format */
	  sprintf (errbuf,
		   _("operand out of range (%ld not between %ld and %lu)"),
		   value, minval, maxval);
	  return errbuf;
	}
    }
  else if (! CGEN_BOOL_ATTR (attrs, CGEN_IFLD_SIGNED))
    {
      unsigned long maxval = mask;
      unsigned long val = (unsigned long) value;

      /* For hosts with a word size > 32 check to see if value has been sign
	 extended beyond 32 bits.  If so then ignore these higher sign bits
	 as the user is attempting to store a 32-bit signed value into an
	 unsigned 32-bit field which is allowed.  */
      if (sizeof (unsigned long) > 4 && ((value >> 32) == -1))
	val &= 0xFFFFFFFF;

      if (val > maxval)
	{
	  /* xgettext:c-format */
	  sprintf (errbuf,
		   _("operand out of range (0x%lx not between 0 and 0x%lx)"),
		   val, maxval);
	  return errbuf;
	}
    }
  else
    {
      if (! cgen_signed_overflow_ok_p (cd))
	{
	  long minval = - (1UL << (length - 1));
	  long maxval =   (1UL << (length - 1)) - 1;

	  if (value < minval || value > maxval)
	    {
	      sprintf
		/* xgettext:c-format */
		(errbuf, _("operand out of range (%ld not between %ld and %ld)"),
		 value, minval, maxval);
	      return errbuf;
	    }
	}
    }

#if CGEN_INT_INSN_P

  {
    int shift_within_word, shift_to_word, shift;

    /* How to shift the value to BIT0 of the word.  */
    shift_to_word = total_length - (word_offset + word_length);

    /* How to shift the value to the field within the word.  */
    if (CGEN_INSN_LSB0_P)
      shift_within_word = start + 1 - length;
    else
      shift_within_word = word_length - start - length;

    /* The total SHIFT, then mask in the value.  */
    shift = shift_to_word + shift_within_word;
    *buffer = (*buffer & ~(mask << shift)) | ((value & mask) << shift);
  }

#else /* ! CGEN_INT_INSN_P */

  {
    unsigned char *bufp = (unsigned char *) buffer + word_offset / 8;

    insert_1 (cd, value, start, length, word_length, bufp);
  }

#endif /* ! CGEN_INT_INSN_P */

  return NULL;
}

/* Default insn builder (insert handler).
   The instruction is recorded in CGEN_INT_INSN_P byte order (meaning
   that if CGEN_INSN_BYTES_PTR is an int * and thus, the value is
   recorded in host byte order, otherwise BUFFER is an array of bytes
   and the value is recorded in target byte order).
   The result is an error message or NULL if success.  */

static const char *
insert_insn_normal (CGEN_CPU_DESC cd,
		    const CGEN_INSN * insn,
		    CGEN_FIELDS * fields,
		    CGEN_INSN_BYTES_PTR buffer,
		    bfd_vma pc)
{
  const CGEN_SYNTAX *syntax = CGEN_INSN_SYNTAX (insn);
  unsigned long value;
  const CGEN_SYNTAX_CHAR_TYPE * syn;

  CGEN_INIT_INSERT (cd);
  value = CGEN_INSN_BASE_VALUE (insn);

  /* If we're recording insns as numbers (rather than a string of bytes),
     target byte order handling is deferred until later.  */

#if CGEN_INT_INSN_P

  put_insn_int_value (cd, buffer, cd->base_insn_bitsize,
		      CGEN_FIELDS_BITSIZE (fields), value);

#else

  cgen_put_insn_value (cd, buffer, min ((unsigned) cd->base_insn_bitsize,
                                        (unsigned) CGEN_FIELDS_BITSIZE (fields)),
		       value, cd->insn_endian);

#endif /* ! CGEN_INT_INSN_P */

  /* ??? It would be better to scan the format's fields.
     Still need to be able to insert a value based on the operand though;
     e.g. storing a branch displacement that got resolved later.
     Needs more thought first.  */

  for (syn = CGEN_SYNTAX_STRING (syntax); * syn; ++ syn)
    {
      const char *errmsg;

      if (CGEN_SYNTAX_CHAR_P (* syn))
	continue;

      errmsg = (* cd->insert_operand) (cd, CGEN_SYNTAX_FIELD (*syn),
				       fields, buffer, pc);
      if (errmsg)
	return errmsg;
    }

  return NULL;
}

#if CGEN_INT_INSN_P
/* Cover function to store an insn value into an integral insn.  Must go here
   because it needs <prefix>-desc.h for CGEN_INT_INSN_P.  */

static void
put_insn_int_value (CGEN_CPU_DESC cd ATTRIBUTE_UNUSED,
		    CGEN_INSN_BYTES_PTR buf,
		    int length,
		    int insn_length,
		    CGEN_INSN_INT value)
{
  /* For architectures with insns smaller than the base-insn-bitsize,
     length may be too big.  */
  if (length > insn_length)
    *buf = value;
  else
    {
      int shift = insn_length - length;
      /* Written this way to avoid undefined behaviour.  */
      CGEN_INSN_INT mask = length == 0 ? 0 : (1UL << (length - 1) << 1) - 1;

      *buf = (*buf & ~(mask << shift)) | ((value & mask) << shift);
    }
}
#endif

/* Operand extraction.  */

#if ! CGEN_INT_INSN_P

/* Subroutine of extract_normal.
   Ensure sufficient bytes are cached in EX_INFO.
   OFFSET is the offset in bytes from the start of the insn of the value.
   BYTES is the length of the needed value.
   Returns 1 for success, 0 for failure.  */

static CGEN_INLINE int
fill_cache (CGEN_CPU_DESC cd ATTRIBUTE_UNUSED,
	    CGEN_EXTRACT_INFO *ex_info,
	    int offset,
	    int bytes,
	    bfd_vma pc)
{
  /* It's doubtful that the middle part has already been fetched so
     we don't optimize that case.  kiss.  */
  unsigned int mask;
  disassemble_info *info = (disassemble_info *) ex_info->dis_info;

  /* First do a quick check.  */
  mask = (1 << bytes) - 1;
  if (((ex_info->valid >> offset) & mask) == mask)
    return 1;

  /* Search for the first byte we need to read.  */
  for (mask = 1 << offset; bytes > 0; --bytes, ++offset, mask <<= 1)
    if (! (mask & ex_info->valid))
      break;

  if (bytes)
    {
      int status;

      pc += offset;
      status = (*info->read_memory_func)
	(pc, ex_info->insn_bytes + offset, bytes, info);

      if (status != 0)
	{
	  (*info->memory_error_func) (status, pc, info);
	  return 0;
	}

      ex_info->valid |= ((1 << bytes) - 1) << offset;
    }

  return 1;
}

/* Subroutine of extract_normal.  */

static CGEN_INLINE long
extract_1 (CGEN_CPU_DESC cd,
	   CGEN_EXTRACT_INFO *ex_info ATTRIBUTE_UNUSED,
	   int start,
	   int length,
	   int word_length,
	   unsigned char *bufp,
	   bfd_vma pc ATTRIBUTE_UNUSED)
{
  unsigned long x;
  int shift;

  x = cgen_get_insn_value (cd, bufp, word_length, cd->endian);

  if (CGEN_INSN_LSB0_P)
    shift = (start + 1) - length;
  else
    shift = (word_length - (start + length));
  return x >> shift;
}

#endif /* ! CGEN_INT_INSN_P */

/* Default extraction routine.

   INSN_VALUE is the first base_insn_bitsize bits of the insn in host order,
   or sometimes less for cases like the m32r where the base insn size is 32
   but some insns are 16 bits.
   ATTRS is a mask of the boolean attributes.  We only need `SIGNED',
   but for generality we take a bitmask of all of them.
   WORD_OFFSET is the offset in bits from the start of the insn of the value.
   WORD_LENGTH is the length of the word in bits in which the value resides.
   START is the starting bit number in the word, architecture origin.
   LENGTH is the length of VALUE in bits.
   TOTAL_LENGTH is the total length of the insn in bits.

   Returns 1 for success, 0 for failure.  */

/* ??? The return code isn't properly used.  wip.  */

/* ??? This doesn't handle bfd_vma's.  Create another function when
   necessary.  */

static int
extract_normal (CGEN_CPU_DESC cd,
#if ! CGEN_INT_INSN_P
		CGEN_EXTRACT_INFO *ex_info,
#else
		CGEN_EXTRACT_INFO *ex_info ATTRIBUTE_UNUSED,
#endif
		CGEN_INSN_INT insn_value,
		unsigned int attrs,
		unsigned int word_offset,
		unsigned int start,
		unsigned int length,
		unsigned int word_length,
		unsigned int total_length,
#if ! CGEN_INT_INSN_P
		bfd_vma pc,
#else
		bfd_vma pc ATTRIBUTE_UNUSED,
#endif
		long *valuep)
{
  long value, mask;

  /* If LENGTH is zero, this operand doesn't contribute to the value
     so give it a standard value of zero.  */
  if (length == 0)
    {
      *valuep = 0;
      return 1;
    }

  if (word_length > 8 * sizeof (CGEN_INSN_INT))
    abort ();

  /* For architectures with insns smaller than the insn-base-bitsize,
     word_length may be too big.  */
  if (cd->min_insn_bitsize < cd->base_insn_bitsize)
    {
      if (word_offset + word_length > total_length)
	word_length = total_length - word_offset;
    }

  /* Does the value reside in INSN_VALUE, and at the right alignment?  */

  if (CGEN_INT_INSN_P || (word_offset == 0 && word_length == total_length))
    {
      if (CGEN_INSN_LSB0_P)
	value = insn_value >> ((word_offset + start + 1) - length);
      else
	value = insn_value >> (total_length - ( word_offset + start + length));
    }

#if ! CGEN_INT_INSN_P

  else
    {
      unsigned char *bufp = ex_info->insn_bytes + word_offset / 8;

      if (word_length > 8 * sizeof (CGEN_INSN_INT))
	abort ();

      if (fill_cache (cd, ex_info, word_offset / 8, word_length / 8, pc) == 0)
	{
	  *valuep = 0;
	  return 0;
	}

      value = extract_1 (cd, ex_info, start, length, word_length, bufp, pc);
    }

#endif /* ! CGEN_INT_INSN_P */

  /* Written this way to avoid undefined behaviour.  */
  mask = (1UL << (length - 1) << 1) - 1;

  value &= mask;
  /* sign extend? */
  if (CGEN_BOOL_ATTR (attrs, CGEN_IFLD_SIGNED)
      && (value & (1UL << (length - 1))))
    value |= ~mask;

  *valuep = value;

  return 1;
}

/* Default insn extractor.

   INSN_VALUE is the first base_insn_bitsize bits, translated to host order.
   The extracted fields are stored in FIELDS.
   EX_INFO is used to handle reading variable length insns.
   Return the length of the insn in bits, or 0 if no match,
   or -1 if an error occurs fetching data (memory_error_func will have
   been called).  */

static int
extract_insn_normal (CGEN_CPU_DESC cd,
		     const CGEN_INSN *insn,
		     CGEN_EXTRACT_INFO *ex_info,
		     CGEN_INSN_INT insn_value,
		     CGEN_FIELDS *fields,
		     bfd_vma pc)
{
  const CGEN_SYNTAX *syntax = CGEN_INSN_SYNTAX (insn);
  const CGEN_SYNTAX_CHAR_TYPE *syn;

  CGEN_FIELDS_BITSIZE (fields) = CGEN_INSN_BITSIZE (insn);

  CGEN_INIT_EXTRACT (cd);

  for (syn = CGEN_SYNTAX_STRING (syntax); *syn; ++syn)
    {
      int length;

      if (CGEN_SYNTAX_CHAR_P (*syn))
	continue;

      length = (* cd->extract_operand) (cd, CGEN_SYNTAX_FIELD (*syn),
					ex_info, insn_value, fields, pc);
      if (length <= 0)
	return length;
    }

  /* We recognized and successfully extracted this insn.  */
  return CGEN_INSN_BITSIZE (insn);
}

/* Machine generated code added here.  */

const char * mep_cgen_insert_operand
  (CGEN_CPU_DESC, int, CGEN_FIELDS *, CGEN_INSN_BYTES_PTR, bfd_vma);

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
   resolved during parsing.  */

const char *
mep_cgen_insert_operand (CGEN_CPU_DESC cd,
			     int opindex,
			     CGEN_FIELDS * fields,
			     CGEN_INSN_BYTES_PTR buffer,
			     bfd_vma pc ATTRIBUTE_UNUSED)
{
  const char * errmsg = NULL;
  unsigned int total_length = CGEN_FIELDS_BITSIZE (fields);

  switch (opindex)
    {
    case MEP_OPERAND_ADDR24A4 :
      {
{
  FLD (f_24u8a4n_hi) = ((UINT) (FLD (f_24u8a4n)) >> (8));
  FLD (f_24u8a4n_lo) = ((UINT) (((FLD (f_24u8a4n)) & (252))) >> (2));
}
        errmsg = insert_normal (cd, fields->f_24u8a4n_hi, 0, 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_24u8a4n_lo, 0, 0, 8, 6, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_C5RMUIMM20 :
      {
{
  FLD (f_c5_rm) = ((UINT) (FLD (f_c5_rmuimm20)) >> (16));
  FLD (f_c5_16u16) = ((FLD (f_c5_rmuimm20)) & (65535));
}
        errmsg = insert_normal (cd, fields->f_c5_rm, 0, 0, 8, 4, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_c5_16u16, 0, 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_C5RNMUIMM24 :
      {
{
  FLD (f_c5_rnm) = ((UINT) (FLD (f_c5_rnmuimm24)) >> (16));
  FLD (f_c5_16u16) = ((FLD (f_c5_rnmuimm24)) & (65535));
}
        errmsg = insert_normal (cd, fields->f_c5_rnm, 0, 0, 4, 8, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_c5_16u16, 0, 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CALLNUM :
      {
{
  FLD (f_5) = ((((UINT) (FLD (f_callnum)) >> (3))) & (1));
  FLD (f_6) = ((((UINT) (FLD (f_callnum)) >> (2))) & (1));
  FLD (f_7) = ((((UINT) (FLD (f_callnum)) >> (1))) & (1));
  FLD (f_11) = ((FLD (f_callnum)) & (1));
}
        errmsg = insert_normal (cd, fields->f_5, 0, 0, 5, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_6, 0, 0, 6, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_7, 0, 0, 7, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_11, 0, 0, 11, 1, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CCCC :
      errmsg = insert_normal (cd, fields->f_rm, 0, 0, 8, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CCRN :
      {
{
  FLD (f_ccrn_hi) = ((((UINT) (FLD (f_ccrn)) >> (4))) & (3));
  FLD (f_ccrn_lo) = ((FLD (f_ccrn)) & (15));
}
        errmsg = insert_normal (cd, fields->f_ccrn_hi, 0, 0, 28, 2, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_ccrn_lo, 0, 0, 4, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CDISP10 :
      {
        long value = fields->f_cdisp10;
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_CDISP10A2 :
      {
        long value = fields->f_cdisp10;
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_CDISP10A4 :
      {
        long value = fields->f_cdisp10;
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_CDISP10A8 :
      {
        long value = fields->f_cdisp10;
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_CDISP12 :
      errmsg = insert_normal (cd, fields->f_12s20, 0|(1<<CGEN_IFLD_SIGNED), 0, 20, 12, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CIMM4 :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CIMM5 :
      errmsg = insert_normal (cd, fields->f_5u24, 0, 0, 24, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CODE16 :
      errmsg = insert_normal (cd, fields->f_16u16, 0, 0, 16, 16, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CODE24 :
      {
{
  FLD (f_24u4n_hi) = ((UINT) (FLD (f_24u4n)) >> (16));
  FLD (f_24u4n_lo) = ((FLD (f_24u4n)) & (65535));
}
        errmsg = insert_normal (cd, fields->f_24u4n_hi, 0, 0, 4, 8, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_24u4n_lo, 0, 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CP_FLAG :
      break;
    case MEP_OPERAND_CRN :
      errmsg = insert_normal (cd, fields->f_crn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CRN64 :
      errmsg = insert_normal (cd, fields->f_crn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CRNX :
      {
{
  FLD (f_crnx_lo) = ((FLD (f_crnx)) & (15));
  FLD (f_crnx_hi) = ((UINT) (FLD (f_crnx)) >> (4));
}
        errmsg = insert_normal (cd, fields->f_crnx_hi, 0, 0, 28, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_crnx_lo, 0, 0, 4, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CRNX64 :
      {
{
  FLD (f_crnx_lo) = ((FLD (f_crnx)) & (15));
  FLD (f_crnx_hi) = ((UINT) (FLD (f_crnx)) >> (4));
}
        errmsg = insert_normal (cd, fields->f_crnx_hi, 0, 0, 28, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_crnx_lo, 0, 0, 4, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CROC :
      errmsg = insert_normal (cd, fields->f_ivc2_5u7, 0, 0, 7, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CROP :
      errmsg = insert_normal (cd, fields->f_ivc2_5u23, 0, 0, 23, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CRPC :
      errmsg = insert_normal (cd, fields->f_ivc2_5u26, 0, 0, 26, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CRPP :
      errmsg = insert_normal (cd, fields->f_ivc2_5u18, 0, 0, 18, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CRQC :
      errmsg = insert_normal (cd, fields->f_ivc2_5u21, 0, 0, 21, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CRQP :
      errmsg = insert_normal (cd, fields->f_ivc2_5u13, 0, 0, 13, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_CSRN :
      {
{
  FLD (f_csrn_lo) = ((FLD (f_csrn)) & (15));
  FLD (f_csrn_hi) = ((UINT) (FLD (f_csrn)) >> (4));
}
        errmsg = insert_normal (cd, fields->f_csrn_hi, 0, 0, 15, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_csrn_lo, 0, 0, 8, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_CSRN_IDX :
      {
{
  FLD (f_csrn_lo) = ((FLD (f_csrn)) & (15));
  FLD (f_csrn_hi) = ((UINT) (FLD (f_csrn)) >> (4));
}
        errmsg = insert_normal (cd, fields->f_csrn_hi, 0, 0, 15, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_csrn_lo, 0, 0, 8, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_DBG :
      break;
    case MEP_OPERAND_DEPC :
      break;
    case MEP_OPERAND_EPC :
      break;
    case MEP_OPERAND_EXC :
      break;
    case MEP_OPERAND_HI :
      break;
    case MEP_OPERAND_IMM16P0 :
      {
{
  FLD (f_ivc2_8u0) = ((((UINT) (FLD (f_ivc2_imm16p0)) >> (8))) & (255));
  FLD (f_ivc2_8u20) = ((FLD (f_ivc2_imm16p0)) & (255));
}
        errmsg = insert_normal (cd, fields->f_ivc2_8u0, 0, 0, 0, 8, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_ivc2_8u20, 0, 0, 20, 8, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_IMM3P12 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u12, 0, 0, 12, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM3P25 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u25, 0, 0, 25, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM3P4 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u4, 0, 0, 4, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM3P5 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u5, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM3P9 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u9, 0, 0, 9, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM4P10 :
      errmsg = insert_normal (cd, fields->f_ivc2_4u10, 0, 0, 10, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM4P4 :
      errmsg = insert_normal (cd, fields->f_ivc2_4u4, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM4P8 :
      errmsg = insert_normal (cd, fields->f_ivc2_4u8, 0, 0, 8, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM5P23 :
      errmsg = insert_normal (cd, fields->f_ivc2_5u23, 0, 0, 23, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM5P3 :
      errmsg = insert_normal (cd, fields->f_ivc2_5u3, 0, 0, 3, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM5P7 :
      errmsg = insert_normal (cd, fields->f_ivc2_5u7, 0, 0, 7, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM5P8 :
      errmsg = insert_normal (cd, fields->f_ivc2_5u8, 0, 0, 8, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM6P2 :
      errmsg = insert_normal (cd, fields->f_ivc2_6u2, 0, 0, 2, 6, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM6P6 :
      errmsg = insert_normal (cd, fields->f_ivc2_6u6, 0, 0, 6, 6, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM8P0 :
      errmsg = insert_normal (cd, fields->f_ivc2_8u0, 0, 0, 0, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM8P20 :
      errmsg = insert_normal (cd, fields->f_ivc2_8u20, 0, 0, 20, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IMM8P4 :
      errmsg = insert_normal (cd, fields->f_ivc2_8u4, 0, 0, 4, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_0_2 :
      errmsg = insert_normal (cd, fields->f_ivc2_2u0, 0, 0, 0, 2, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_0_3 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u0, 0, 0, 0, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_0_4 :
      errmsg = insert_normal (cd, fields->f_ivc2_4u0, 0, 0, 0, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_0_5 :
      errmsg = insert_normal (cd, fields->f_ivc2_5u0, 0, 0, 0, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_6_1 :
      errmsg = insert_normal (cd, fields->f_ivc2_1u6, 0, 0, 6, 1, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_6_2 :
      errmsg = insert_normal (cd, fields->f_ivc2_2u6, 0, 0, 6, 2, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC_X_6_3 :
      errmsg = insert_normal (cd, fields->f_ivc2_3u6, 0, 0, 6, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_IVC2_ACC0_0 :
      break;
    case MEP_OPERAND_IVC2_ACC0_1 :
      break;
    case MEP_OPERAND_IVC2_ACC0_2 :
      break;
    case MEP_OPERAND_IVC2_ACC0_3 :
      break;
    case MEP_OPERAND_IVC2_ACC0_4 :
      break;
    case MEP_OPERAND_IVC2_ACC0_5 :
      break;
    case MEP_OPERAND_IVC2_ACC0_6 :
      break;
    case MEP_OPERAND_IVC2_ACC0_7 :
      break;
    case MEP_OPERAND_IVC2_ACC1_0 :
      break;
    case MEP_OPERAND_IVC2_ACC1_1 :
      break;
    case MEP_OPERAND_IVC2_ACC1_2 :
      break;
    case MEP_OPERAND_IVC2_ACC1_3 :
      break;
    case MEP_OPERAND_IVC2_ACC1_4 :
      break;
    case MEP_OPERAND_IVC2_ACC1_5 :
      break;
    case MEP_OPERAND_IVC2_ACC1_6 :
      break;
    case MEP_OPERAND_IVC2_ACC1_7 :
      break;
    case MEP_OPERAND_IVC2_CC :
      break;
    case MEP_OPERAND_IVC2_COFA0 :
      break;
    case MEP_OPERAND_IVC2_COFA1 :
      break;
    case MEP_OPERAND_IVC2_COFR0 :
      break;
    case MEP_OPERAND_IVC2_COFR1 :
      break;
    case MEP_OPERAND_IVC2_CSAR0 :
      break;
    case MEP_OPERAND_IVC2_CSAR1 :
      break;
    case MEP_OPERAND_IVC2C3CCRN :
      {
{
  FLD (f_ivc2_ccrn_c3hi) = ((((UINT) (FLD (f_ivc2_ccrn_c3)) >> (4))) & (3));
  FLD (f_ivc2_ccrn_c3lo) = ((FLD (f_ivc2_ccrn_c3)) & (15));
}
        errmsg = insert_normal (cd, fields->f_ivc2_ccrn_c3hi, 0, 0, 28, 2, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_ivc2_ccrn_c3lo, 0, 0, 4, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_IVC2CCRN :
      {
{
  FLD (f_ivc2_ccrn_h2) = ((((UINT) (FLD (f_ivc2_ccrn)) >> (4))) & (3));
  FLD (f_ivc2_ccrn_lo) = ((FLD (f_ivc2_ccrn)) & (15));
}
        errmsg = insert_normal (cd, fields->f_ivc2_ccrn_h2, 0, 0, 20, 2, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_ivc2_ccrn_lo, 0, 0, 0, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_IVC2CRN :
      {
{
  FLD (f_ivc2_ccrn_h1) = ((((UINT) (FLD (f_ivc2_crnx)) >> (4))) & (1));
  FLD (f_ivc2_ccrn_lo) = ((FLD (f_ivc2_crnx)) & (15));
}
        errmsg = insert_normal (cd, fields->f_ivc2_ccrn_h1, 0, 0, 20, 1, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_ivc2_ccrn_lo, 0, 0, 0, 4, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_IVC2RM :
      errmsg = insert_normal (cd, fields->f_ivc2_crm, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_LO :
      break;
    case MEP_OPERAND_LP :
      break;
    case MEP_OPERAND_MB0 :
      break;
    case MEP_OPERAND_MB1 :
      break;
    case MEP_OPERAND_ME0 :
      break;
    case MEP_OPERAND_ME1 :
      break;
    case MEP_OPERAND_NPC :
      break;
    case MEP_OPERAND_OPT :
      break;
    case MEP_OPERAND_PCABS24A2 :
      {
{
  FLD (f_24u5a2n_lo) = ((UINT) (((FLD (f_24u5a2n)) & (255))) >> (1));
  FLD (f_24u5a2n_hi) = ((UINT) (FLD (f_24u5a2n)) >> (8));
}
        errmsg = insert_normal (cd, fields->f_24u5a2n_hi, 0, 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_24u5a2n_lo, 0, 0, 5, 7, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_PCREL12A2 :
      {
        long value = fields->f_12s4a2;
        value = ((SI) (((value) - (pc))) >> (1));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 4, 11, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_PCREL17A2 :
      {
        long value = fields->f_17s16a2;
        value = ((SI) (((value) - (pc))) >> (1));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 16, 16, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_PCREL24A2 :
      {
{
  FLD (f_24s5a2n) = ((FLD (f_24s5a2n)) - (pc));
  FLD (f_24s5a2n_lo) = ((UINT) (((FLD (f_24s5a2n)) & (254))) >> (1));
  FLD (f_24s5a2n_hi) = ((INT) (FLD (f_24s5a2n)) >> (8));
}
        errmsg = insert_normal (cd, fields->f_24s5a2n_hi, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_24s5a2n_lo, 0|(1<<CGEN_IFLD_PCREL_ADDR), 0, 5, 7, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_PCREL8A2 :
      {
        long value = fields->f_8s8a2;
        value = ((SI) (((value) - (pc))) >> (1));
        errmsg = insert_normal (cd, value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 8, 7, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_PSW :
      break;
    case MEP_OPERAND_R0 :
      break;
    case MEP_OPERAND_R1 :
      break;
    case MEP_OPERAND_RL :
      errmsg = insert_normal (cd, fields->f_rl, 0, 0, 12, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RL5 :
      errmsg = insert_normal (cd, fields->f_rl5, 0, 0, 20, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RM :
      errmsg = insert_normal (cd, fields->f_rm, 0, 0, 8, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RMA :
      errmsg = insert_normal (cd, fields->f_rm, 0, 0, 8, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3 :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3C :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3L :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3S :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3UC :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3UL :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RN3US :
      errmsg = insert_normal (cd, fields->f_rn3, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RNC :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RNL :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RNS :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RNUC :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RNUL :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_RNUS :
      errmsg = insert_normal (cd, fields->f_rn, 0, 0, 4, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SAR :
      break;
    case MEP_OPERAND_SDISP16 :
      errmsg = insert_normal (cd, fields->f_16s16, 0|(1<<CGEN_IFLD_SIGNED), 0, 16, 16, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SIMM16 :
      errmsg = insert_normal (cd, fields->f_16s16, 0|(1<<CGEN_IFLD_SIGNED), 0, 16, 16, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SIMM16P0 :
      {
{
  FLD (f_ivc2_8u0) = ((((UINT) (FLD (f_ivc2_simm16p0)) >> (8))) & (255));
  FLD (f_ivc2_8u20) = ((FLD (f_ivc2_simm16p0)) & (255));
}
        errmsg = insert_normal (cd, fields->f_ivc2_8u0, 0, 0, 0, 8, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_ivc2_8u20, 0, 0, 20, 8, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_SIMM6 :
      errmsg = insert_normal (cd, fields->f_6s8, 0|(1<<CGEN_IFLD_SIGNED), 0, 8, 6, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SIMM8 :
      errmsg = insert_normal (cd, fields->f_8s8, 0|(1<<CGEN_IFLD_SIGNED), 0, 8, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SIMM8P0 :
      errmsg = insert_normal (cd, fields->f_ivc2_8s0, 0|(1<<CGEN_IFLD_SIGNED), 0, 0, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SIMM8P20 :
      errmsg = insert_normal (cd, fields->f_ivc2_8s20, 0|(1<<CGEN_IFLD_SIGNED), 0, 20, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SIMM8P4 :
      errmsg = insert_normal (cd, fields->f_ivc2_8s4, 0|(1<<CGEN_IFLD_SIGNED), 0, 4, 8, 32, total_length, buffer);
      break;
    case MEP_OPERAND_SP :
      break;
    case MEP_OPERAND_SPR :
      break;
    case MEP_OPERAND_TP :
      break;
    case MEP_OPERAND_TPR :
      break;
    case MEP_OPERAND_UDISP2 :
      errmsg = insert_normal (cd, fields->f_2u6, 0, 0, 6, 2, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UDISP7 :
      errmsg = insert_normal (cd, fields->f_7u9, 0, 0, 9, 7, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UDISP7A2 :
      {
        long value = fields->f_7u9a2;
        value = ((USI) (value) >> (1));
        errmsg = insert_normal (cd, value, 0, 0, 9, 6, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_UDISP7A4 :
      {
        long value = fields->f_7u9a4;
        value = ((USI) (value) >> (2));
        errmsg = insert_normal (cd, value, 0, 0, 9, 5, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_UIMM16 :
      errmsg = insert_normal (cd, fields->f_16u16, 0, 0, 16, 16, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UIMM2 :
      errmsg = insert_normal (cd, fields->f_2u10, 0, 0, 10, 2, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UIMM24 :
      {
{
  FLD (f_24u8n_hi) = ((UINT) (FLD (f_24u8n)) >> (8));
  FLD (f_24u8n_lo) = ((FLD (f_24u8n)) & (255));
}
        errmsg = insert_normal (cd, fields->f_24u8n_hi, 0, 0, 16, 16, 32, total_length, buffer);
        if (errmsg)
          break;
        errmsg = insert_normal (cd, fields->f_24u8n_lo, 0, 0, 8, 8, 32, total_length, buffer);
        if (errmsg)
          break;
      }
      break;
    case MEP_OPERAND_UIMM3 :
      errmsg = insert_normal (cd, fields->f_3u5, 0, 0, 5, 3, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UIMM4 :
      errmsg = insert_normal (cd, fields->f_4u8, 0, 0, 8, 4, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UIMM5 :
      errmsg = insert_normal (cd, fields->f_5u8, 0, 0, 8, 5, 32, total_length, buffer);
      break;
    case MEP_OPERAND_UIMM7A4 :
      {
        long value = fields->f_7u9a4;
        value = ((USI) (value) >> (2));
        errmsg = insert_normal (cd, value, 0, 0, 9, 5, 32, total_length, buffer);
      }
      break;
    case MEP_OPERAND_ZERO :
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while building insn"),
	 opindex);
      abort ();
  }

  return errmsg;
}

int mep_cgen_extract_operand
  (CGEN_CPU_DESC, int, CGEN_EXTRACT_INFO *, CGEN_INSN_INT, CGEN_FIELDS *, bfd_vma);

/* Main entry point for operand extraction.
   The result is <= 0 for error, >0 for success.
   ??? Actual values aren't well defined right now.

   This function is basically just a big switch statement.  Earlier versions
   used tables to look up the function to use, but
   - if the table contains both assembler and disassembler functions then
     the disassembler contains much of the assembler and vice-versa,
   - there's a lot of inlining possibilities as things grow,
   - using a switch statement avoids the function call overhead.

   This function could be moved into `print_insn_normal', but keeping it
   separate makes clear the interface between `print_insn_normal' and each of
   the handlers.  */

int
mep_cgen_extract_operand (CGEN_CPU_DESC cd,
			     int opindex,
			     CGEN_EXTRACT_INFO *ex_info,
			     CGEN_INSN_INT insn_value,
			     CGEN_FIELDS * fields,
			     bfd_vma pc)
{
  /* Assume success (for those operands that are nops).  */
  int length = 1;
  unsigned int total_length = CGEN_FIELDS_BITSIZE (fields);

  switch (opindex)
    {
    case MEP_OPERAND_ADDR24A4 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_24u8a4n_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 6, 32, total_length, pc, & fields->f_24u8a4n_lo);
        if (length <= 0) break;
  FLD (f_24u8a4n) = ((((FLD (f_24u8a4n_hi)) << (8))) | (((FLD (f_24u8a4n_lo)) << (2))));
      }
      break;
    case MEP_OPERAND_C5RMUIMM20 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_c5_rm);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_c5_16u16);
        if (length <= 0) break;
{
  FLD (f_c5_rmuimm20) = ((FLD (f_c5_16u16)) | (((FLD (f_c5_rm)) << (16))));
}
      }
      break;
    case MEP_OPERAND_C5RNMUIMM24 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 8, 32, total_length, pc, & fields->f_c5_rnm);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_c5_16u16);
        if (length <= 0) break;
{
  FLD (f_c5_rnmuimm24) = ((FLD (f_c5_16u16)) | (((FLD (f_c5_rnm)) << (16))));
}
      }
      break;
    case MEP_OPERAND_CALLNUM :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 1, 32, total_length, pc, & fields->f_5);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 6, 1, 32, total_length, pc, & fields->f_6);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 7, 1, 32, total_length, pc, & fields->f_7);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 11, 1, 32, total_length, pc, & fields->f_11);
        if (length <= 0) break;
  FLD (f_callnum) = ((((FLD (f_5)) << (3))) | (((((FLD (f_6)) << (2))) | (((((FLD (f_7)) << (1))) | (FLD (f_11)))))));
      }
      break;
    case MEP_OPERAND_CCCC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_rm);
      break;
    case MEP_OPERAND_CCRN :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 28, 2, 32, total_length, pc, & fields->f_ccrn_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_ccrn_lo);
        if (length <= 0) break;
  FLD (f_ccrn) = ((((FLD (f_ccrn_hi)) << (4))) | (FLD (f_ccrn_lo)));
      }
      break;
    case MEP_OPERAND_CDISP10 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, pc, & value);
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        fields->f_cdisp10 = value;
      }
      break;
    case MEP_OPERAND_CDISP10A2 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, pc, & value);
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        fields->f_cdisp10 = value;
      }
      break;
    case MEP_OPERAND_CDISP10A4 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, pc, & value);
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        fields->f_cdisp10 = value;
      }
      break;
    case MEP_OPERAND_CDISP10A8 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 22, 10, 32, total_length, pc, & value);
        value = (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (512))) ? (((((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023))) - (1024))) : (((((((((value) & (128))) ? (((value) ^ (768))) : (value)) & (512))) ? ((((((value) & (128))) ? (((value) ^ (768))) : (value)) - (1024))) : ((((value) & (128))) ? (((value) ^ (768))) : (value))) & (1023)));
        fields->f_cdisp10 = value;
      }
      break;
    case MEP_OPERAND_CDISP12 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 20, 12, 32, total_length, pc, & fields->f_12s20);
      break;
    case MEP_OPERAND_CIMM4 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_CIMM5 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 24, 5, 32, total_length, pc, & fields->f_5u24);
      break;
    case MEP_OPERAND_CODE16 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_16u16);
      break;
    case MEP_OPERAND_CODE24 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 8, 32, total_length, pc, & fields->f_24u4n_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_24u4n_lo);
        if (length <= 0) break;
  FLD (f_24u4n) = ((((FLD (f_24u4n_hi)) << (16))) | (FLD (f_24u4n_lo)));
      }
      break;
    case MEP_OPERAND_CP_FLAG :
      break;
    case MEP_OPERAND_CRN :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_crn);
      break;
    case MEP_OPERAND_CRN64 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_crn);
      break;
    case MEP_OPERAND_CRNX :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 28, 1, 32, total_length, pc, & fields->f_crnx_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_crnx_lo);
        if (length <= 0) break;
  FLD (f_crnx) = ((((FLD (f_crnx_hi)) << (4))) | (FLD (f_crnx_lo)));
      }
      break;
    case MEP_OPERAND_CRNX64 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 28, 1, 32, total_length, pc, & fields->f_crnx_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_crnx_lo);
        if (length <= 0) break;
  FLD (f_crnx) = ((((FLD (f_crnx_hi)) << (4))) | (FLD (f_crnx_lo)));
      }
      break;
    case MEP_OPERAND_CROC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 7, 5, 32, total_length, pc, & fields->f_ivc2_5u7);
      break;
    case MEP_OPERAND_CROP :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 23, 5, 32, total_length, pc, & fields->f_ivc2_5u23);
      break;
    case MEP_OPERAND_CRPC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 26, 5, 32, total_length, pc, & fields->f_ivc2_5u26);
      break;
    case MEP_OPERAND_CRPP :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 18, 5, 32, total_length, pc, & fields->f_ivc2_5u18);
      break;
    case MEP_OPERAND_CRQC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 21, 5, 32, total_length, pc, & fields->f_ivc2_5u21);
      break;
    case MEP_OPERAND_CRQP :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 13, 5, 32, total_length, pc, & fields->f_ivc2_5u13);
      break;
    case MEP_OPERAND_CSRN :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 15, 1, 32, total_length, pc, & fields->f_csrn_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_csrn_lo);
        if (length <= 0) break;
  FLD (f_csrn) = ((((FLD (f_csrn_hi)) << (4))) | (FLD (f_csrn_lo)));
      }
      break;
    case MEP_OPERAND_CSRN_IDX :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 15, 1, 32, total_length, pc, & fields->f_csrn_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_csrn_lo);
        if (length <= 0) break;
  FLD (f_csrn) = ((((FLD (f_csrn_hi)) << (4))) | (FLD (f_csrn_lo)));
      }
      break;
    case MEP_OPERAND_DBG :
      break;
    case MEP_OPERAND_DEPC :
      break;
    case MEP_OPERAND_EPC :
      break;
    case MEP_OPERAND_EXC :
      break;
    case MEP_OPERAND_HI :
      break;
    case MEP_OPERAND_IMM16P0 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 8, 32, total_length, pc, & fields->f_ivc2_8u0);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 20, 8, 32, total_length, pc, & fields->f_ivc2_8u20);
        if (length <= 0) break;
{
  FLD (f_ivc2_imm16p0) = ((FLD (f_ivc2_8u20)) | (((FLD (f_ivc2_8u0)) << (8))));
}
      }
      break;
    case MEP_OPERAND_IMM3P12 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 12, 3, 32, total_length, pc, & fields->f_ivc2_3u12);
      break;
    case MEP_OPERAND_IMM3P25 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 25, 3, 32, total_length, pc, & fields->f_ivc2_3u25);
      break;
    case MEP_OPERAND_IMM3P4 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 3, 32, total_length, pc, & fields->f_ivc2_3u4);
      break;
    case MEP_OPERAND_IMM3P5 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_ivc2_3u5);
      break;
    case MEP_OPERAND_IMM3P9 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 9, 3, 32, total_length, pc, & fields->f_ivc2_3u9);
      break;
    case MEP_OPERAND_IMM4P10 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 10, 4, 32, total_length, pc, & fields->f_ivc2_4u10);
      break;
    case MEP_OPERAND_IMM4P4 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_ivc2_4u4);
      break;
    case MEP_OPERAND_IMM4P8 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_ivc2_4u8);
      break;
    case MEP_OPERAND_IMM5P23 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 23, 5, 32, total_length, pc, & fields->f_ivc2_5u23);
      break;
    case MEP_OPERAND_IMM5P3 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 3, 5, 32, total_length, pc, & fields->f_ivc2_5u3);
      break;
    case MEP_OPERAND_IMM5P7 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 7, 5, 32, total_length, pc, & fields->f_ivc2_5u7);
      break;
    case MEP_OPERAND_IMM5P8 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 5, 32, total_length, pc, & fields->f_ivc2_5u8);
      break;
    case MEP_OPERAND_IMM6P2 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 2, 6, 32, total_length, pc, & fields->f_ivc2_6u2);
      break;
    case MEP_OPERAND_IMM6P6 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 6, 6, 32, total_length, pc, & fields->f_ivc2_6u6);
      break;
    case MEP_OPERAND_IMM8P0 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 8, 32, total_length, pc, & fields->f_ivc2_8u0);
      break;
    case MEP_OPERAND_IMM8P20 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 20, 8, 32, total_length, pc, & fields->f_ivc2_8u20);
      break;
    case MEP_OPERAND_IMM8P4 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 8, 32, total_length, pc, & fields->f_ivc2_8u4);
      break;
    case MEP_OPERAND_IVC_X_0_2 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 2, 32, total_length, pc, & fields->f_ivc2_2u0);
      break;
    case MEP_OPERAND_IVC_X_0_3 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 3, 32, total_length, pc, & fields->f_ivc2_3u0);
      break;
    case MEP_OPERAND_IVC_X_0_4 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 4, 32, total_length, pc, & fields->f_ivc2_4u0);
      break;
    case MEP_OPERAND_IVC_X_0_5 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 5, 32, total_length, pc, & fields->f_ivc2_5u0);
      break;
    case MEP_OPERAND_IVC_X_6_1 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 6, 1, 32, total_length, pc, & fields->f_ivc2_1u6);
      break;
    case MEP_OPERAND_IVC_X_6_2 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 6, 2, 32, total_length, pc, & fields->f_ivc2_2u6);
      break;
    case MEP_OPERAND_IVC_X_6_3 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 6, 3, 32, total_length, pc, & fields->f_ivc2_3u6);
      break;
    case MEP_OPERAND_IVC2_ACC0_0 :
      break;
    case MEP_OPERAND_IVC2_ACC0_1 :
      break;
    case MEP_OPERAND_IVC2_ACC0_2 :
      break;
    case MEP_OPERAND_IVC2_ACC0_3 :
      break;
    case MEP_OPERAND_IVC2_ACC0_4 :
      break;
    case MEP_OPERAND_IVC2_ACC0_5 :
      break;
    case MEP_OPERAND_IVC2_ACC0_6 :
      break;
    case MEP_OPERAND_IVC2_ACC0_7 :
      break;
    case MEP_OPERAND_IVC2_ACC1_0 :
      break;
    case MEP_OPERAND_IVC2_ACC1_1 :
      break;
    case MEP_OPERAND_IVC2_ACC1_2 :
      break;
    case MEP_OPERAND_IVC2_ACC1_3 :
      break;
    case MEP_OPERAND_IVC2_ACC1_4 :
      break;
    case MEP_OPERAND_IVC2_ACC1_5 :
      break;
    case MEP_OPERAND_IVC2_ACC1_6 :
      break;
    case MEP_OPERAND_IVC2_ACC1_7 :
      break;
    case MEP_OPERAND_IVC2_CC :
      break;
    case MEP_OPERAND_IVC2_COFA0 :
      break;
    case MEP_OPERAND_IVC2_COFA1 :
      break;
    case MEP_OPERAND_IVC2_COFR0 :
      break;
    case MEP_OPERAND_IVC2_COFR1 :
      break;
    case MEP_OPERAND_IVC2_CSAR0 :
      break;
    case MEP_OPERAND_IVC2_CSAR1 :
      break;
    case MEP_OPERAND_IVC2C3CCRN :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 28, 2, 32, total_length, pc, & fields->f_ivc2_ccrn_c3hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_ivc2_ccrn_c3lo);
        if (length <= 0) break;
  FLD (f_ivc2_ccrn_c3) = ((((FLD (f_ivc2_ccrn_c3hi)) << (4))) | (FLD (f_ivc2_ccrn_c3lo)));
      }
      break;
    case MEP_OPERAND_IVC2CCRN :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 20, 2, 32, total_length, pc, & fields->f_ivc2_ccrn_h2);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 4, 32, total_length, pc, & fields->f_ivc2_ccrn_lo);
        if (length <= 0) break;
  FLD (f_ivc2_ccrn) = ((((FLD (f_ivc2_ccrn_h2)) << (4))) | (FLD (f_ivc2_ccrn_lo)));
      }
      break;
    case MEP_OPERAND_IVC2CRN :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 20, 1, 32, total_length, pc, & fields->f_ivc2_ccrn_h1);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 4, 32, total_length, pc, & fields->f_ivc2_ccrn_lo);
        if (length <= 0) break;
  FLD (f_ivc2_crnx) = ((((FLD (f_ivc2_ccrn_h1)) << (4))) | (FLD (f_ivc2_ccrn_lo)));
      }
      break;
    case MEP_OPERAND_IVC2RM :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_ivc2_crm);
      break;
    case MEP_OPERAND_LO :
      break;
    case MEP_OPERAND_LP :
      break;
    case MEP_OPERAND_MB0 :
      break;
    case MEP_OPERAND_MB1 :
      break;
    case MEP_OPERAND_ME0 :
      break;
    case MEP_OPERAND_ME1 :
      break;
    case MEP_OPERAND_NPC :
      break;
    case MEP_OPERAND_OPT :
      break;
    case MEP_OPERAND_PCABS24A2 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_24u5a2n_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 7, 32, total_length, pc, & fields->f_24u5a2n_lo);
        if (length <= 0) break;
  FLD (f_24u5a2n) = ((((FLD (f_24u5a2n_hi)) << (8))) | (((FLD (f_24u5a2n_lo)) << (1))));
      }
      break;
    case MEP_OPERAND_PCREL12A2 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 4, 11, 32, total_length, pc, & value);
        value = ((((value) * (2))) + (pc));
        fields->f_12s4a2 = value;
      }
      break;
    case MEP_OPERAND_PCREL17A2 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 16, 16, 32, total_length, pc, & value);
        value = ((((value) * (2))) + (pc));
        fields->f_17s16a2 = value;
      }
      break;
    case MEP_OPERAND_PCREL24A2 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 16, 16, 32, total_length, pc, & fields->f_24s5a2n_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_PCREL_ADDR), 0, 5, 7, 32, total_length, pc, & fields->f_24s5a2n_lo);
        if (length <= 0) break;
  FLD (f_24s5a2n) = ((((((FLD (f_24s5a2n_hi)) * (256))) | (((FLD (f_24s5a2n_lo)) << (1))))) + (pc));
      }
      break;
    case MEP_OPERAND_PCREL8A2 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED)|(1<<CGEN_IFLD_PCREL_ADDR), 0, 8, 7, 32, total_length, pc, & value);
        value = ((((value) * (2))) + (pc));
        fields->f_8s8a2 = value;
      }
      break;
    case MEP_OPERAND_PSW :
      break;
    case MEP_OPERAND_R0 :
      break;
    case MEP_OPERAND_R1 :
      break;
    case MEP_OPERAND_RL :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 12, 4, 32, total_length, pc, & fields->f_rl);
      break;
    case MEP_OPERAND_RL5 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 20, 4, 32, total_length, pc, & fields->f_rl5);
      break;
    case MEP_OPERAND_RM :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_rm);
      break;
    case MEP_OPERAND_RMA :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_rm);
      break;
    case MEP_OPERAND_RN :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_RN3 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RN3C :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RN3L :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RN3S :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RN3UC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RN3UL :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RN3US :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_rn3);
      break;
    case MEP_OPERAND_RNC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_RNL :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_RNS :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_RNUC :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_RNUL :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_RNUS :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 4, 4, 32, total_length, pc, & fields->f_rn);
      break;
    case MEP_OPERAND_SAR :
      break;
    case MEP_OPERAND_SDISP16 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 16, 16, 32, total_length, pc, & fields->f_16s16);
      break;
    case MEP_OPERAND_SIMM16 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 16, 16, 32, total_length, pc, & fields->f_16s16);
      break;
    case MEP_OPERAND_SIMM16P0 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 0, 8, 32, total_length, pc, & fields->f_ivc2_8u0);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 20, 8, 32, total_length, pc, & fields->f_ivc2_8u20);
        if (length <= 0) break;
{
  FLD (f_ivc2_simm16p0) = ((FLD (f_ivc2_8u20)) | (((FLD (f_ivc2_8u0)) << (8))));
}
      }
      break;
    case MEP_OPERAND_SIMM6 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 8, 6, 32, total_length, pc, & fields->f_6s8);
      break;
    case MEP_OPERAND_SIMM8 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 8, 8, 32, total_length, pc, & fields->f_8s8);
      break;
    case MEP_OPERAND_SIMM8P0 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 0, 8, 32, total_length, pc, & fields->f_ivc2_8s0);
      break;
    case MEP_OPERAND_SIMM8P20 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 20, 8, 32, total_length, pc, & fields->f_ivc2_8s20);
      break;
    case MEP_OPERAND_SIMM8P4 :
      length = extract_normal (cd, ex_info, insn_value, 0|(1<<CGEN_IFLD_SIGNED), 0, 4, 8, 32, total_length, pc, & fields->f_ivc2_8s4);
      break;
    case MEP_OPERAND_SP :
      break;
    case MEP_OPERAND_SPR :
      break;
    case MEP_OPERAND_TP :
      break;
    case MEP_OPERAND_TPR :
      break;
    case MEP_OPERAND_UDISP2 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 6, 2, 32, total_length, pc, & fields->f_2u6);
      break;
    case MEP_OPERAND_UDISP7 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 9, 7, 32, total_length, pc, & fields->f_7u9);
      break;
    case MEP_OPERAND_UDISP7A2 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 9, 6, 32, total_length, pc, & value);
        value = ((value) * (2));
        fields->f_7u9a2 = value;
      }
      break;
    case MEP_OPERAND_UDISP7A4 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 9, 5, 32, total_length, pc, & value);
        value = ((value) << (2));
        fields->f_7u9a4 = value;
      }
      break;
    case MEP_OPERAND_UIMM16 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_16u16);
      break;
    case MEP_OPERAND_UIMM2 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 10, 2, 32, total_length, pc, & fields->f_2u10);
      break;
    case MEP_OPERAND_UIMM24 :
      {
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 16, 16, 32, total_length, pc, & fields->f_24u8n_hi);
        if (length <= 0) break;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 8, 32, total_length, pc, & fields->f_24u8n_lo);
        if (length <= 0) break;
  FLD (f_24u8n) = ((((FLD (f_24u8n_hi)) << (8))) | (FLD (f_24u8n_lo)));
      }
      break;
    case MEP_OPERAND_UIMM3 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 5, 3, 32, total_length, pc, & fields->f_3u5);
      break;
    case MEP_OPERAND_UIMM4 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 4, 32, total_length, pc, & fields->f_4u8);
      break;
    case MEP_OPERAND_UIMM5 :
      length = extract_normal (cd, ex_info, insn_value, 0, 0, 8, 5, 32, total_length, pc, & fields->f_5u8);
      break;
    case MEP_OPERAND_UIMM7A4 :
      {
        long value;
        length = extract_normal (cd, ex_info, insn_value, 0, 0, 9, 5, 32, total_length, pc, & value);
        value = ((value) << (2));
        fields->f_7u9a4 = value;
      }
      break;
    case MEP_OPERAND_ZERO :
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while decoding insn"),
	 opindex);
      abort ();
    }

  return length;
}

cgen_insert_fn * const mep_cgen_insert_handlers[] =
{
  insert_insn_normal,
};

cgen_extract_fn * const mep_cgen_extract_handlers[] =
{
  extract_insn_normal,
};

int mep_cgen_get_int_operand     (CGEN_CPU_DESC, int, const CGEN_FIELDS *);
bfd_vma mep_cgen_get_vma_operand (CGEN_CPU_DESC, int, const CGEN_FIELDS *);

/* Getting values from cgen_fields is handled by a collection of functions.
   They are distinguished by the type of the VALUE argument they return.
   TODO: floating point, inlining support, remove cases where result type
   not appropriate.  */

int
mep_cgen_get_int_operand (CGEN_CPU_DESC cd ATTRIBUTE_UNUSED,
			     int opindex,
			     const CGEN_FIELDS * fields)
{
  int value;

  switch (opindex)
    {
    case MEP_OPERAND_ADDR24A4 :
      value = fields->f_24u8a4n;
      break;
    case MEP_OPERAND_C5RMUIMM20 :
      value = fields->f_c5_rmuimm20;
      break;
    case MEP_OPERAND_C5RNMUIMM24 :
      value = fields->f_c5_rnmuimm24;
      break;
    case MEP_OPERAND_CALLNUM :
      value = fields->f_callnum;
      break;
    case MEP_OPERAND_CCCC :
      value = fields->f_rm;
      break;
    case MEP_OPERAND_CCRN :
      value = fields->f_ccrn;
      break;
    case MEP_OPERAND_CDISP10 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP10A2 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP10A4 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP10A8 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP12 :
      value = fields->f_12s20;
      break;
    case MEP_OPERAND_CIMM4 :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_CIMM5 :
      value = fields->f_5u24;
      break;
    case MEP_OPERAND_CODE16 :
      value = fields->f_16u16;
      break;
    case MEP_OPERAND_CODE24 :
      value = fields->f_24u4n;
      break;
    case MEP_OPERAND_CP_FLAG :
      value = 0;
      break;
    case MEP_OPERAND_CRN :
      value = fields->f_crn;
      break;
    case MEP_OPERAND_CRN64 :
      value = fields->f_crn;
      break;
    case MEP_OPERAND_CRNX :
      value = fields->f_crnx;
      break;
    case MEP_OPERAND_CRNX64 :
      value = fields->f_crnx;
      break;
    case MEP_OPERAND_CROC :
      value = fields->f_ivc2_5u7;
      break;
    case MEP_OPERAND_CROP :
      value = fields->f_ivc2_5u23;
      break;
    case MEP_OPERAND_CRPC :
      value = fields->f_ivc2_5u26;
      break;
    case MEP_OPERAND_CRPP :
      value = fields->f_ivc2_5u18;
      break;
    case MEP_OPERAND_CRQC :
      value = fields->f_ivc2_5u21;
      break;
    case MEP_OPERAND_CRQP :
      value = fields->f_ivc2_5u13;
      break;
    case MEP_OPERAND_CSRN :
      value = fields->f_csrn;
      break;
    case MEP_OPERAND_CSRN_IDX :
      value = fields->f_csrn;
      break;
    case MEP_OPERAND_DBG :
      value = 0;
      break;
    case MEP_OPERAND_DEPC :
      value = 0;
      break;
    case MEP_OPERAND_EPC :
      value = 0;
      break;
    case MEP_OPERAND_EXC :
      value = 0;
      break;
    case MEP_OPERAND_HI :
      value = 0;
      break;
    case MEP_OPERAND_IMM16P0 :
      value = fields->f_ivc2_imm16p0;
      break;
    case MEP_OPERAND_IMM3P12 :
      value = fields->f_ivc2_3u12;
      break;
    case MEP_OPERAND_IMM3P25 :
      value = fields->f_ivc2_3u25;
      break;
    case MEP_OPERAND_IMM3P4 :
      value = fields->f_ivc2_3u4;
      break;
    case MEP_OPERAND_IMM3P5 :
      value = fields->f_ivc2_3u5;
      break;
    case MEP_OPERAND_IMM3P9 :
      value = fields->f_ivc2_3u9;
      break;
    case MEP_OPERAND_IMM4P10 :
      value = fields->f_ivc2_4u10;
      break;
    case MEP_OPERAND_IMM4P4 :
      value = fields->f_ivc2_4u4;
      break;
    case MEP_OPERAND_IMM4P8 :
      value = fields->f_ivc2_4u8;
      break;
    case MEP_OPERAND_IMM5P23 :
      value = fields->f_ivc2_5u23;
      break;
    case MEP_OPERAND_IMM5P3 :
      value = fields->f_ivc2_5u3;
      break;
    case MEP_OPERAND_IMM5P7 :
      value = fields->f_ivc2_5u7;
      break;
    case MEP_OPERAND_IMM5P8 :
      value = fields->f_ivc2_5u8;
      break;
    case MEP_OPERAND_IMM6P2 :
      value = fields->f_ivc2_6u2;
      break;
    case MEP_OPERAND_IMM6P6 :
      value = fields->f_ivc2_6u6;
      break;
    case MEP_OPERAND_IMM8P0 :
      value = fields->f_ivc2_8u0;
      break;
    case MEP_OPERAND_IMM8P20 :
      value = fields->f_ivc2_8u20;
      break;
    case MEP_OPERAND_IMM8P4 :
      value = fields->f_ivc2_8u4;
      break;
    case MEP_OPERAND_IVC_X_0_2 :
      value = fields->f_ivc2_2u0;
      break;
    case MEP_OPERAND_IVC_X_0_3 :
      value = fields->f_ivc2_3u0;
      break;
    case MEP_OPERAND_IVC_X_0_4 :
      value = fields->f_ivc2_4u0;
      break;
    case MEP_OPERAND_IVC_X_0_5 :
      value = fields->f_ivc2_5u0;
      break;
    case MEP_OPERAND_IVC_X_6_1 :
      value = fields->f_ivc2_1u6;
      break;
    case MEP_OPERAND_IVC_X_6_2 :
      value = fields->f_ivc2_2u6;
      break;
    case MEP_OPERAND_IVC_X_6_3 :
      value = fields->f_ivc2_3u6;
      break;
    case MEP_OPERAND_IVC2_ACC0_0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_2 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_3 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_4 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_5 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_6 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_7 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_2 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_3 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_4 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_5 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_6 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_7 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_CC :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFA0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFA1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFR0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFR1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_CSAR0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_CSAR1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2C3CCRN :
      value = fields->f_ivc2_ccrn_c3;
      break;
    case MEP_OPERAND_IVC2CCRN :
      value = fields->f_ivc2_ccrn;
      break;
    case MEP_OPERAND_IVC2CRN :
      value = fields->f_ivc2_crnx;
      break;
    case MEP_OPERAND_IVC2RM :
      value = fields->f_ivc2_crm;
      break;
    case MEP_OPERAND_LO :
      value = 0;
      break;
    case MEP_OPERAND_LP :
      value = 0;
      break;
    case MEP_OPERAND_MB0 :
      value = 0;
      break;
    case MEP_OPERAND_MB1 :
      value = 0;
      break;
    case MEP_OPERAND_ME0 :
      value = 0;
      break;
    case MEP_OPERAND_ME1 :
      value = 0;
      break;
    case MEP_OPERAND_NPC :
      value = 0;
      break;
    case MEP_OPERAND_OPT :
      value = 0;
      break;
    case MEP_OPERAND_PCABS24A2 :
      value = fields->f_24u5a2n;
      break;
    case MEP_OPERAND_PCREL12A2 :
      value = fields->f_12s4a2;
      break;
    case MEP_OPERAND_PCREL17A2 :
      value = fields->f_17s16a2;
      break;
    case MEP_OPERAND_PCREL24A2 :
      value = fields->f_24s5a2n;
      break;
    case MEP_OPERAND_PCREL8A2 :
      value = fields->f_8s8a2;
      break;
    case MEP_OPERAND_PSW :
      value = 0;
      break;
    case MEP_OPERAND_R0 :
      value = 0;
      break;
    case MEP_OPERAND_R1 :
      value = 0;
      break;
    case MEP_OPERAND_RL :
      value = fields->f_rl;
      break;
    case MEP_OPERAND_RL5 :
      value = fields->f_rl5;
      break;
    case MEP_OPERAND_RM :
      value = fields->f_rm;
      break;
    case MEP_OPERAND_RMA :
      value = fields->f_rm;
      break;
    case MEP_OPERAND_RN :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RN3 :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3C :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3L :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3S :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3UC :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3UL :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3US :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RNC :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNL :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNS :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNUC :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNUL :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNUS :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_SAR :
      value = 0;
      break;
    case MEP_OPERAND_SDISP16 :
      value = fields->f_16s16;
      break;
    case MEP_OPERAND_SIMM16 :
      value = fields->f_16s16;
      break;
    case MEP_OPERAND_SIMM16P0 :
      value = fields->f_ivc2_simm16p0;
      break;
    case MEP_OPERAND_SIMM6 :
      value = fields->f_6s8;
      break;
    case MEP_OPERAND_SIMM8 :
      value = fields->f_8s8;
      break;
    case MEP_OPERAND_SIMM8P0 :
      value = fields->f_ivc2_8s0;
      break;
    case MEP_OPERAND_SIMM8P20 :
      value = fields->f_ivc2_8s20;
      break;
    case MEP_OPERAND_SIMM8P4 :
      value = fields->f_ivc2_8s4;
      break;
    case MEP_OPERAND_SP :
      value = 0;
      break;
    case MEP_OPERAND_SPR :
      value = 0;
      break;
    case MEP_OPERAND_TP :
      value = 0;
      break;
    case MEP_OPERAND_TPR :
      value = 0;
      break;
    case MEP_OPERAND_UDISP2 :
      value = fields->f_2u6;
      break;
    case MEP_OPERAND_UDISP7 :
      value = fields->f_7u9;
      break;
    case MEP_OPERAND_UDISP7A2 :
      value = fields->f_7u9a2;
      break;
    case MEP_OPERAND_UDISP7A4 :
      value = fields->f_7u9a4;
      break;
    case MEP_OPERAND_UIMM16 :
      value = fields->f_16u16;
      break;
    case MEP_OPERAND_UIMM2 :
      value = fields->f_2u10;
      break;
    case MEP_OPERAND_UIMM24 :
      value = fields->f_24u8n;
      break;
    case MEP_OPERAND_UIMM3 :
      value = fields->f_3u5;
      break;
    case MEP_OPERAND_UIMM4 :
      value = fields->f_4u8;
      break;
    case MEP_OPERAND_UIMM5 :
      value = fields->f_5u8;
      break;
    case MEP_OPERAND_UIMM7A4 :
      value = fields->f_7u9a4;
      break;
    case MEP_OPERAND_ZERO :
      value = 0;
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while getting int operand"),
	 opindex);
      abort ();
  }

  return value;
}

bfd_vma
mep_cgen_get_vma_operand (CGEN_CPU_DESC cd ATTRIBUTE_UNUSED,
			     int opindex,
			     const CGEN_FIELDS * fields)
{
  bfd_vma value;

  switch (opindex)
    {
    case MEP_OPERAND_ADDR24A4 :
      value = fields->f_24u8a4n;
      break;
    case MEP_OPERAND_C5RMUIMM20 :
      value = fields->f_c5_rmuimm20;
      break;
    case MEP_OPERAND_C5RNMUIMM24 :
      value = fields->f_c5_rnmuimm24;
      break;
    case MEP_OPERAND_CALLNUM :
      value = fields->f_callnum;
      break;
    case MEP_OPERAND_CCCC :
      value = fields->f_rm;
      break;
    case MEP_OPERAND_CCRN :
      value = fields->f_ccrn;
      break;
    case MEP_OPERAND_CDISP10 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP10A2 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP10A4 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP10A8 :
      value = fields->f_cdisp10;
      break;
    case MEP_OPERAND_CDISP12 :
      value = fields->f_12s20;
      break;
    case MEP_OPERAND_CIMM4 :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_CIMM5 :
      value = fields->f_5u24;
      break;
    case MEP_OPERAND_CODE16 :
      value = fields->f_16u16;
      break;
    case MEP_OPERAND_CODE24 :
      value = fields->f_24u4n;
      break;
    case MEP_OPERAND_CP_FLAG :
      value = 0;
      break;
    case MEP_OPERAND_CRN :
      value = fields->f_crn;
      break;
    case MEP_OPERAND_CRN64 :
      value = fields->f_crn;
      break;
    case MEP_OPERAND_CRNX :
      value = fields->f_crnx;
      break;
    case MEP_OPERAND_CRNX64 :
      value = fields->f_crnx;
      break;
    case MEP_OPERAND_CROC :
      value = fields->f_ivc2_5u7;
      break;
    case MEP_OPERAND_CROP :
      value = fields->f_ivc2_5u23;
      break;
    case MEP_OPERAND_CRPC :
      value = fields->f_ivc2_5u26;
      break;
    case MEP_OPERAND_CRPP :
      value = fields->f_ivc2_5u18;
      break;
    case MEP_OPERAND_CRQC :
      value = fields->f_ivc2_5u21;
      break;
    case MEP_OPERAND_CRQP :
      value = fields->f_ivc2_5u13;
      break;
    case MEP_OPERAND_CSRN :
      value = fields->f_csrn;
      break;
    case MEP_OPERAND_CSRN_IDX :
      value = fields->f_csrn;
      break;
    case MEP_OPERAND_DBG :
      value = 0;
      break;
    case MEP_OPERAND_DEPC :
      value = 0;
      break;
    case MEP_OPERAND_EPC :
      value = 0;
      break;
    case MEP_OPERAND_EXC :
      value = 0;
      break;
    case MEP_OPERAND_HI :
      value = 0;
      break;
    case MEP_OPERAND_IMM16P0 :
      value = fields->f_ivc2_imm16p0;
      break;
    case MEP_OPERAND_IMM3P12 :
      value = fields->f_ivc2_3u12;
      break;
    case MEP_OPERAND_IMM3P25 :
      value = fields->f_ivc2_3u25;
      break;
    case MEP_OPERAND_IMM3P4 :
      value = fields->f_ivc2_3u4;
      break;
    case MEP_OPERAND_IMM3P5 :
      value = fields->f_ivc2_3u5;
      break;
    case MEP_OPERAND_IMM3P9 :
      value = fields->f_ivc2_3u9;
      break;
    case MEP_OPERAND_IMM4P10 :
      value = fields->f_ivc2_4u10;
      break;
    case MEP_OPERAND_IMM4P4 :
      value = fields->f_ivc2_4u4;
      break;
    case MEP_OPERAND_IMM4P8 :
      value = fields->f_ivc2_4u8;
      break;
    case MEP_OPERAND_IMM5P23 :
      value = fields->f_ivc2_5u23;
      break;
    case MEP_OPERAND_IMM5P3 :
      value = fields->f_ivc2_5u3;
      break;
    case MEP_OPERAND_IMM5P7 :
      value = fields->f_ivc2_5u7;
      break;
    case MEP_OPERAND_IMM5P8 :
      value = fields->f_ivc2_5u8;
      break;
    case MEP_OPERAND_IMM6P2 :
      value = fields->f_ivc2_6u2;
      break;
    case MEP_OPERAND_IMM6P6 :
      value = fields->f_ivc2_6u6;
      break;
    case MEP_OPERAND_IMM8P0 :
      value = fields->f_ivc2_8u0;
      break;
    case MEP_OPERAND_IMM8P20 :
      value = fields->f_ivc2_8u20;
      break;
    case MEP_OPERAND_IMM8P4 :
      value = fields->f_ivc2_8u4;
      break;
    case MEP_OPERAND_IVC_X_0_2 :
      value = fields->f_ivc2_2u0;
      break;
    case MEP_OPERAND_IVC_X_0_3 :
      value = fields->f_ivc2_3u0;
      break;
    case MEP_OPERAND_IVC_X_0_4 :
      value = fields->f_ivc2_4u0;
      break;
    case MEP_OPERAND_IVC_X_0_5 :
      value = fields->f_ivc2_5u0;
      break;
    case MEP_OPERAND_IVC_X_6_1 :
      value = fields->f_ivc2_1u6;
      break;
    case MEP_OPERAND_IVC_X_6_2 :
      value = fields->f_ivc2_2u6;
      break;
    case MEP_OPERAND_IVC_X_6_3 :
      value = fields->f_ivc2_3u6;
      break;
    case MEP_OPERAND_IVC2_ACC0_0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_2 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_3 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_4 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_5 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_6 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC0_7 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_2 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_3 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_4 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_5 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_6 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_ACC1_7 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_CC :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFA0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFA1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFR0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_COFR1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_CSAR0 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2_CSAR1 :
      value = 0;
      break;
    case MEP_OPERAND_IVC2C3CCRN :
      value = fields->f_ivc2_ccrn_c3;
      break;
    case MEP_OPERAND_IVC2CCRN :
      value = fields->f_ivc2_ccrn;
      break;
    case MEP_OPERAND_IVC2CRN :
      value = fields->f_ivc2_crnx;
      break;
    case MEP_OPERAND_IVC2RM :
      value = fields->f_ivc2_crm;
      break;
    case MEP_OPERAND_LO :
      value = 0;
      break;
    case MEP_OPERAND_LP :
      value = 0;
      break;
    case MEP_OPERAND_MB0 :
      value = 0;
      break;
    case MEP_OPERAND_MB1 :
      value = 0;
      break;
    case MEP_OPERAND_ME0 :
      value = 0;
      break;
    case MEP_OPERAND_ME1 :
      value = 0;
      break;
    case MEP_OPERAND_NPC :
      value = 0;
      break;
    case MEP_OPERAND_OPT :
      value = 0;
      break;
    case MEP_OPERAND_PCABS24A2 :
      value = fields->f_24u5a2n;
      break;
    case MEP_OPERAND_PCREL12A2 :
      value = fields->f_12s4a2;
      break;
    case MEP_OPERAND_PCREL17A2 :
      value = fields->f_17s16a2;
      break;
    case MEP_OPERAND_PCREL24A2 :
      value = fields->f_24s5a2n;
      break;
    case MEP_OPERAND_PCREL8A2 :
      value = fields->f_8s8a2;
      break;
    case MEP_OPERAND_PSW :
      value = 0;
      break;
    case MEP_OPERAND_R0 :
      value = 0;
      break;
    case MEP_OPERAND_R1 :
      value = 0;
      break;
    case MEP_OPERAND_RL :
      value = fields->f_rl;
      break;
    case MEP_OPERAND_RL5 :
      value = fields->f_rl5;
      break;
    case MEP_OPERAND_RM :
      value = fields->f_rm;
      break;
    case MEP_OPERAND_RMA :
      value = fields->f_rm;
      break;
    case MEP_OPERAND_RN :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RN3 :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3C :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3L :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3S :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3UC :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3UL :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RN3US :
      value = fields->f_rn3;
      break;
    case MEP_OPERAND_RNC :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNL :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNS :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNUC :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNUL :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_RNUS :
      value = fields->f_rn;
      break;
    case MEP_OPERAND_SAR :
      value = 0;
      break;
    case MEP_OPERAND_SDISP16 :
      value = fields->f_16s16;
      break;
    case MEP_OPERAND_SIMM16 :
      value = fields->f_16s16;
      break;
    case MEP_OPERAND_SIMM16P0 :
      value = fields->f_ivc2_simm16p0;
      break;
    case MEP_OPERAND_SIMM6 :
      value = fields->f_6s8;
      break;
    case MEP_OPERAND_SIMM8 :
      value = fields->f_8s8;
      break;
    case MEP_OPERAND_SIMM8P0 :
      value = fields->f_ivc2_8s0;
      break;
    case MEP_OPERAND_SIMM8P20 :
      value = fields->f_ivc2_8s20;
      break;
    case MEP_OPERAND_SIMM8P4 :
      value = fields->f_ivc2_8s4;
      break;
    case MEP_OPERAND_SP :
      value = 0;
      break;
    case MEP_OPERAND_SPR :
      value = 0;
      break;
    case MEP_OPERAND_TP :
      value = 0;
      break;
    case MEP_OPERAND_TPR :
      value = 0;
      break;
    case MEP_OPERAND_UDISP2 :
      value = fields->f_2u6;
      break;
    case MEP_OPERAND_UDISP7 :
      value = fields->f_7u9;
      break;
    case MEP_OPERAND_UDISP7A2 :
      value = fields->f_7u9a2;
      break;
    case MEP_OPERAND_UDISP7A4 :
      value = fields->f_7u9a4;
      break;
    case MEP_OPERAND_UIMM16 :
      value = fields->f_16u16;
      break;
    case MEP_OPERAND_UIMM2 :
      value = fields->f_2u10;
      break;
    case MEP_OPERAND_UIMM24 :
      value = fields->f_24u8n;
      break;
    case MEP_OPERAND_UIMM3 :
      value = fields->f_3u5;
      break;
    case MEP_OPERAND_UIMM4 :
      value = fields->f_4u8;
      break;
    case MEP_OPERAND_UIMM5 :
      value = fields->f_5u8;
      break;
    case MEP_OPERAND_UIMM7A4 :
      value = fields->f_7u9a4;
      break;
    case MEP_OPERAND_ZERO :
      value = 0;
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while getting vma operand"),
	 opindex);
      abort ();
  }

  return value;
}

void mep_cgen_set_int_operand  (CGEN_CPU_DESC, int, CGEN_FIELDS *, int);
void mep_cgen_set_vma_operand  (CGEN_CPU_DESC, int, CGEN_FIELDS *, bfd_vma);

/* Stuffing values in cgen_fields is handled by a collection of functions.
   They are distinguished by the type of the VALUE argument they accept.
   TODO: floating point, inlining support, remove cases where argument type
   not appropriate.  */

void
mep_cgen_set_int_operand (CGEN_CPU_DESC cd ATTRIBUTE_UNUSED,
			     int opindex,
			     CGEN_FIELDS * fields,
			     int value)
{
  switch (opindex)
    {
    case MEP_OPERAND_ADDR24A4 :
      fields->f_24u8a4n = value;
      break;
    case MEP_OPERAND_C5RMUIMM20 :
      fields->f_c5_rmuimm20 = value;
      break;
    case MEP_OPERAND_C5RNMUIMM24 :
      fields->f_c5_rnmuimm24 = value;
      break;
    case MEP_OPERAND_CALLNUM :
      fields->f_callnum = value;
      break;
    case MEP_OPERAND_CCCC :
      fields->f_rm = value;
      break;
    case MEP_OPERAND_CCRN :
      fields->f_ccrn = value;
      break;
    case MEP_OPERAND_CDISP10 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP10A2 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP10A4 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP10A8 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP12 :
      fields->f_12s20 = value;
      break;
    case MEP_OPERAND_CIMM4 :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_CIMM5 :
      fields->f_5u24 = value;
      break;
    case MEP_OPERAND_CODE16 :
      fields->f_16u16 = value;
      break;
    case MEP_OPERAND_CODE24 :
      fields->f_24u4n = value;
      break;
    case MEP_OPERAND_CP_FLAG :
      break;
    case MEP_OPERAND_CRN :
      fields->f_crn = value;
      break;
    case MEP_OPERAND_CRN64 :
      fields->f_crn = value;
      break;
    case MEP_OPERAND_CRNX :
      fields->f_crnx = value;
      break;
    case MEP_OPERAND_CRNX64 :
      fields->f_crnx = value;
      break;
    case MEP_OPERAND_CROC :
      fields->f_ivc2_5u7 = value;
      break;
    case MEP_OPERAND_CROP :
      fields->f_ivc2_5u23 = value;
      break;
    case MEP_OPERAND_CRPC :
      fields->f_ivc2_5u26 = value;
      break;
    case MEP_OPERAND_CRPP :
      fields->f_ivc2_5u18 = value;
      break;
    case MEP_OPERAND_CRQC :
      fields->f_ivc2_5u21 = value;
      break;
    case MEP_OPERAND_CRQP :
      fields->f_ivc2_5u13 = value;
      break;
    case MEP_OPERAND_CSRN :
      fields->f_csrn = value;
      break;
    case MEP_OPERAND_CSRN_IDX :
      fields->f_csrn = value;
      break;
    case MEP_OPERAND_DBG :
      break;
    case MEP_OPERAND_DEPC :
      break;
    case MEP_OPERAND_EPC :
      break;
    case MEP_OPERAND_EXC :
      break;
    case MEP_OPERAND_HI :
      break;
    case MEP_OPERAND_IMM16P0 :
      fields->f_ivc2_imm16p0 = value;
      break;
    case MEP_OPERAND_IMM3P12 :
      fields->f_ivc2_3u12 = value;
      break;
    case MEP_OPERAND_IMM3P25 :
      fields->f_ivc2_3u25 = value;
      break;
    case MEP_OPERAND_IMM3P4 :
      fields->f_ivc2_3u4 = value;
      break;
    case MEP_OPERAND_IMM3P5 :
      fields->f_ivc2_3u5 = value;
      break;
    case MEP_OPERAND_IMM3P9 :
      fields->f_ivc2_3u9 = value;
      break;
    case MEP_OPERAND_IMM4P10 :
      fields->f_ivc2_4u10 = value;
      break;
    case MEP_OPERAND_IMM4P4 :
      fields->f_ivc2_4u4 = value;
      break;
    case MEP_OPERAND_IMM4P8 :
      fields->f_ivc2_4u8 = value;
      break;
    case MEP_OPERAND_IMM5P23 :
      fields->f_ivc2_5u23 = value;
      break;
    case MEP_OPERAND_IMM5P3 :
      fields->f_ivc2_5u3 = value;
      break;
    case MEP_OPERAND_IMM5P7 :
      fields->f_ivc2_5u7 = value;
      break;
    case MEP_OPERAND_IMM5P8 :
      fields->f_ivc2_5u8 = value;
      break;
    case MEP_OPERAND_IMM6P2 :
      fields->f_ivc2_6u2 = value;
      break;
    case MEP_OPERAND_IMM6P6 :
      fields->f_ivc2_6u6 = value;
      break;
    case MEP_OPERAND_IMM8P0 :
      fields->f_ivc2_8u0 = value;
      break;
    case MEP_OPERAND_IMM8P20 :
      fields->f_ivc2_8u20 = value;
      break;
    case MEP_OPERAND_IMM8P4 :
      fields->f_ivc2_8u4 = value;
      break;
    case MEP_OPERAND_IVC_X_0_2 :
      fields->f_ivc2_2u0 = value;
      break;
    case MEP_OPERAND_IVC_X_0_3 :
      fields->f_ivc2_3u0 = value;
      break;
    case MEP_OPERAND_IVC_X_0_4 :
      fields->f_ivc2_4u0 = value;
      break;
    case MEP_OPERAND_IVC_X_0_5 :
      fields->f_ivc2_5u0 = value;
      break;
    case MEP_OPERAND_IVC_X_6_1 :
      fields->f_ivc2_1u6 = value;
      break;
    case MEP_OPERAND_IVC_X_6_2 :
      fields->f_ivc2_2u6 = value;
      break;
    case MEP_OPERAND_IVC_X_6_3 :
      fields->f_ivc2_3u6 = value;
      break;
    case MEP_OPERAND_IVC2_ACC0_0 :
      break;
    case MEP_OPERAND_IVC2_ACC0_1 :
      break;
    case MEP_OPERAND_IVC2_ACC0_2 :
      break;
    case MEP_OPERAND_IVC2_ACC0_3 :
      break;
    case MEP_OPERAND_IVC2_ACC0_4 :
      break;
    case MEP_OPERAND_IVC2_ACC0_5 :
      break;
    case MEP_OPERAND_IVC2_ACC0_6 :
      break;
    case MEP_OPERAND_IVC2_ACC0_7 :
      break;
    case MEP_OPERAND_IVC2_ACC1_0 :
      break;
    case MEP_OPERAND_IVC2_ACC1_1 :
      break;
    case MEP_OPERAND_IVC2_ACC1_2 :
      break;
    case MEP_OPERAND_IVC2_ACC1_3 :
      break;
    case MEP_OPERAND_IVC2_ACC1_4 :
      break;
    case MEP_OPERAND_IVC2_ACC1_5 :
      break;
    case MEP_OPERAND_IVC2_ACC1_6 :
      break;
    case MEP_OPERAND_IVC2_ACC1_7 :
      break;
    case MEP_OPERAND_IVC2_CC :
      break;
    case MEP_OPERAND_IVC2_COFA0 :
      break;
    case MEP_OPERAND_IVC2_COFA1 :
      break;
    case MEP_OPERAND_IVC2_COFR0 :
      break;
    case MEP_OPERAND_IVC2_COFR1 :
      break;
    case MEP_OPERAND_IVC2_CSAR0 :
      break;
    case MEP_OPERAND_IVC2_CSAR1 :
      break;
    case MEP_OPERAND_IVC2C3CCRN :
      fields->f_ivc2_ccrn_c3 = value;
      break;
    case MEP_OPERAND_IVC2CCRN :
      fields->f_ivc2_ccrn = value;
      break;
    case MEP_OPERAND_IVC2CRN :
      fields->f_ivc2_crnx = value;
      break;
    case MEP_OPERAND_IVC2RM :
      fields->f_ivc2_crm = value;
      break;
    case MEP_OPERAND_LO :
      break;
    case MEP_OPERAND_LP :
      break;
    case MEP_OPERAND_MB0 :
      break;
    case MEP_OPERAND_MB1 :
      break;
    case MEP_OPERAND_ME0 :
      break;
    case MEP_OPERAND_ME1 :
      break;
    case MEP_OPERAND_NPC :
      break;
    case MEP_OPERAND_OPT :
      break;
    case MEP_OPERAND_PCABS24A2 :
      fields->f_24u5a2n = value;
      break;
    case MEP_OPERAND_PCREL12A2 :
      fields->f_12s4a2 = value;
      break;
    case MEP_OPERAND_PCREL17A2 :
      fields->f_17s16a2 = value;
      break;
    case MEP_OPERAND_PCREL24A2 :
      fields->f_24s5a2n = value;
      break;
    case MEP_OPERAND_PCREL8A2 :
      fields->f_8s8a2 = value;
      break;
    case MEP_OPERAND_PSW :
      break;
    case MEP_OPERAND_R0 :
      break;
    case MEP_OPERAND_R1 :
      break;
    case MEP_OPERAND_RL :
      fields->f_rl = value;
      break;
    case MEP_OPERAND_RL5 :
      fields->f_rl5 = value;
      break;
    case MEP_OPERAND_RM :
      fields->f_rm = value;
      break;
    case MEP_OPERAND_RMA :
      fields->f_rm = value;
      break;
    case MEP_OPERAND_RN :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RN3 :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3C :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3L :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3S :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3UC :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3UL :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3US :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RNC :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNL :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNS :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNUC :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNUL :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNUS :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_SAR :
      break;
    case MEP_OPERAND_SDISP16 :
      fields->f_16s16 = value;
      break;
    case MEP_OPERAND_SIMM16 :
      fields->f_16s16 = value;
      break;
    case MEP_OPERAND_SIMM16P0 :
      fields->f_ivc2_simm16p0 = value;
      break;
    case MEP_OPERAND_SIMM6 :
      fields->f_6s8 = value;
      break;
    case MEP_OPERAND_SIMM8 :
      fields->f_8s8 = value;
      break;
    case MEP_OPERAND_SIMM8P0 :
      fields->f_ivc2_8s0 = value;
      break;
    case MEP_OPERAND_SIMM8P20 :
      fields->f_ivc2_8s20 = value;
      break;
    case MEP_OPERAND_SIMM8P4 :
      fields->f_ivc2_8s4 = value;
      break;
    case MEP_OPERAND_SP :
      break;
    case MEP_OPERAND_SPR :
      break;
    case MEP_OPERAND_TP :
      break;
    case MEP_OPERAND_TPR :
      break;
    case MEP_OPERAND_UDISP2 :
      fields->f_2u6 = value;
      break;
    case MEP_OPERAND_UDISP7 :
      fields->f_7u9 = value;
      break;
    case MEP_OPERAND_UDISP7A2 :
      fields->f_7u9a2 = value;
      break;
    case MEP_OPERAND_UDISP7A4 :
      fields->f_7u9a4 = value;
      break;
    case MEP_OPERAND_UIMM16 :
      fields->f_16u16 = value;
      break;
    case MEP_OPERAND_UIMM2 :
      fields->f_2u10 = value;
      break;
    case MEP_OPERAND_UIMM24 :
      fields->f_24u8n = value;
      break;
    case MEP_OPERAND_UIMM3 :
      fields->f_3u5 = value;
      break;
    case MEP_OPERAND_UIMM4 :
      fields->f_4u8 = value;
      break;
    case MEP_OPERAND_UIMM5 :
      fields->f_5u8 = value;
      break;
    case MEP_OPERAND_UIMM7A4 :
      fields->f_7u9a4 = value;
      break;
    case MEP_OPERAND_ZERO :
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while setting int operand"),
	 opindex);
      abort ();
  }
}

void
mep_cgen_set_vma_operand (CGEN_CPU_DESC cd ATTRIBUTE_UNUSED,
			     int opindex,
			     CGEN_FIELDS * fields,
			     bfd_vma value)
{
  switch (opindex)
    {
    case MEP_OPERAND_ADDR24A4 :
      fields->f_24u8a4n = value;
      break;
    case MEP_OPERAND_C5RMUIMM20 :
      fields->f_c5_rmuimm20 = value;
      break;
    case MEP_OPERAND_C5RNMUIMM24 :
      fields->f_c5_rnmuimm24 = value;
      break;
    case MEP_OPERAND_CALLNUM :
      fields->f_callnum = value;
      break;
    case MEP_OPERAND_CCCC :
      fields->f_rm = value;
      break;
    case MEP_OPERAND_CCRN :
      fields->f_ccrn = value;
      break;
    case MEP_OPERAND_CDISP10 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP10A2 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP10A4 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP10A8 :
      fields->f_cdisp10 = value;
      break;
    case MEP_OPERAND_CDISP12 :
      fields->f_12s20 = value;
      break;
    case MEP_OPERAND_CIMM4 :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_CIMM5 :
      fields->f_5u24 = value;
      break;
    case MEP_OPERAND_CODE16 :
      fields->f_16u16 = value;
      break;
    case MEP_OPERAND_CODE24 :
      fields->f_24u4n = value;
      break;
    case MEP_OPERAND_CP_FLAG :
      break;
    case MEP_OPERAND_CRN :
      fields->f_crn = value;
      break;
    case MEP_OPERAND_CRN64 :
      fields->f_crn = value;
      break;
    case MEP_OPERAND_CRNX :
      fields->f_crnx = value;
      break;
    case MEP_OPERAND_CRNX64 :
      fields->f_crnx = value;
      break;
    case MEP_OPERAND_CROC :
      fields->f_ivc2_5u7 = value;
      break;
    case MEP_OPERAND_CROP :
      fields->f_ivc2_5u23 = value;
      break;
    case MEP_OPERAND_CRPC :
      fields->f_ivc2_5u26 = value;
      break;
    case MEP_OPERAND_CRPP :
      fields->f_ivc2_5u18 = value;
      break;
    case MEP_OPERAND_CRQC :
      fields->f_ivc2_5u21 = value;
      break;
    case MEP_OPERAND_CRQP :
      fields->f_ivc2_5u13 = value;
      break;
    case MEP_OPERAND_CSRN :
      fields->f_csrn = value;
      break;
    case MEP_OPERAND_CSRN_IDX :
      fields->f_csrn = value;
      break;
    case MEP_OPERAND_DBG :
      break;
    case MEP_OPERAND_DEPC :
      break;
    case MEP_OPERAND_EPC :
      break;
    case MEP_OPERAND_EXC :
      break;
    case MEP_OPERAND_HI :
      break;
    case MEP_OPERAND_IMM16P0 :
      fields->f_ivc2_imm16p0 = value;
      break;
    case MEP_OPERAND_IMM3P12 :
      fields->f_ivc2_3u12 = value;
      break;
    case MEP_OPERAND_IMM3P25 :
      fields->f_ivc2_3u25 = value;
      break;
    case MEP_OPERAND_IMM3P4 :
      fields->f_ivc2_3u4 = value;
      break;
    case MEP_OPERAND_IMM3P5 :
      fields->f_ivc2_3u5 = value;
      break;
    case MEP_OPERAND_IMM3P9 :
      fields->f_ivc2_3u9 = value;
      break;
    case MEP_OPERAND_IMM4P10 :
      fields->f_ivc2_4u10 = value;
      break;
    case MEP_OPERAND_IMM4P4 :
      fields->f_ivc2_4u4 = value;
      break;
    case MEP_OPERAND_IMM4P8 :
      fields->f_ivc2_4u8 = value;
      break;
    case MEP_OPERAND_IMM5P23 :
      fields->f_ivc2_5u23 = value;
      break;
    case MEP_OPERAND_IMM5P3 :
      fields->f_ivc2_5u3 = value;
      break;
    case MEP_OPERAND_IMM5P7 :
      fields->f_ivc2_5u7 = value;
      break;
    case MEP_OPERAND_IMM5P8 :
      fields->f_ivc2_5u8 = value;
      break;
    case MEP_OPERAND_IMM6P2 :
      fields->f_ivc2_6u2 = value;
      break;
    case MEP_OPERAND_IMM6P6 :
      fields->f_ivc2_6u6 = value;
      break;
    case MEP_OPERAND_IMM8P0 :
      fields->f_ivc2_8u0 = value;
      break;
    case MEP_OPERAND_IMM8P20 :
      fields->f_ivc2_8u20 = value;
      break;
    case MEP_OPERAND_IMM8P4 :
      fields->f_ivc2_8u4 = value;
      break;
    case MEP_OPERAND_IVC_X_0_2 :
      fields->f_ivc2_2u0 = value;
      break;
    case MEP_OPERAND_IVC_X_0_3 :
      fields->f_ivc2_3u0 = value;
      break;
    case MEP_OPERAND_IVC_X_0_4 :
      fields->f_ivc2_4u0 = value;
      break;
    case MEP_OPERAND_IVC_X_0_5 :
      fields->f_ivc2_5u0 = value;
      break;
    case MEP_OPERAND_IVC_X_6_1 :
      fields->f_ivc2_1u6 = value;
      break;
    case MEP_OPERAND_IVC_X_6_2 :
      fields->f_ivc2_2u6 = value;
      break;
    case MEP_OPERAND_IVC_X_6_3 :
      fields->f_ivc2_3u6 = value;
      break;
    case MEP_OPERAND_IVC2_ACC0_0 :
      break;
    case MEP_OPERAND_IVC2_ACC0_1 :
      break;
    case MEP_OPERAND_IVC2_ACC0_2 :
      break;
    case MEP_OPERAND_IVC2_ACC0_3 :
      break;
    case MEP_OPERAND_IVC2_ACC0_4 :
      break;
    case MEP_OPERAND_IVC2_ACC0_5 :
      break;
    case MEP_OPERAND_IVC2_ACC0_6 :
      break;
    case MEP_OPERAND_IVC2_ACC0_7 :
      break;
    case MEP_OPERAND_IVC2_ACC1_0 :
      break;
    case MEP_OPERAND_IVC2_ACC1_1 :
      break;
    case MEP_OPERAND_IVC2_ACC1_2 :
      break;
    case MEP_OPERAND_IVC2_ACC1_3 :
      break;
    case MEP_OPERAND_IVC2_ACC1_4 :
      break;
    case MEP_OPERAND_IVC2_ACC1_5 :
      break;
    case MEP_OPERAND_IVC2_ACC1_6 :
      break;
    case MEP_OPERAND_IVC2_ACC1_7 :
      break;
    case MEP_OPERAND_IVC2_CC :
      break;
    case MEP_OPERAND_IVC2_COFA0 :
      break;
    case MEP_OPERAND_IVC2_COFA1 :
      break;
    case MEP_OPERAND_IVC2_COFR0 :
      break;
    case MEP_OPERAND_IVC2_COFR1 :
      break;
    case MEP_OPERAND_IVC2_CSAR0 :
      break;
    case MEP_OPERAND_IVC2_CSAR1 :
      break;
    case MEP_OPERAND_IVC2C3CCRN :
      fields->f_ivc2_ccrn_c3 = value;
      break;
    case MEP_OPERAND_IVC2CCRN :
      fields->f_ivc2_ccrn = value;
      break;
    case MEP_OPERAND_IVC2CRN :
      fields->f_ivc2_crnx = value;
      break;
    case MEP_OPERAND_IVC2RM :
      fields->f_ivc2_crm = value;
      break;
    case MEP_OPERAND_LO :
      break;
    case MEP_OPERAND_LP :
      break;
    case MEP_OPERAND_MB0 :
      break;
    case MEP_OPERAND_MB1 :
      break;
    case MEP_OPERAND_ME0 :
      break;
    case MEP_OPERAND_ME1 :
      break;
    case MEP_OPERAND_NPC :
      break;
    case MEP_OPERAND_OPT :
      break;
    case MEP_OPERAND_PCABS24A2 :
      fields->f_24u5a2n = value;
      break;
    case MEP_OPERAND_PCREL12A2 :
      fields->f_12s4a2 = value;
      break;
    case MEP_OPERAND_PCREL17A2 :
      fields->f_17s16a2 = value;
      break;
    case MEP_OPERAND_PCREL24A2 :
      fields->f_24s5a2n = value;
      break;
    case MEP_OPERAND_PCREL8A2 :
      fields->f_8s8a2 = value;
      break;
    case MEP_OPERAND_PSW :
      break;
    case MEP_OPERAND_R0 :
      break;
    case MEP_OPERAND_R1 :
      break;
    case MEP_OPERAND_RL :
      fields->f_rl = value;
      break;
    case MEP_OPERAND_RL5 :
      fields->f_rl5 = value;
      break;
    case MEP_OPERAND_RM :
      fields->f_rm = value;
      break;
    case MEP_OPERAND_RMA :
      fields->f_rm = value;
      break;
    case MEP_OPERAND_RN :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RN3 :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3C :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3L :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3S :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3UC :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3UL :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RN3US :
      fields->f_rn3 = value;
      break;
    case MEP_OPERAND_RNC :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNL :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNS :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNUC :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNUL :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_RNUS :
      fields->f_rn = value;
      break;
    case MEP_OPERAND_SAR :
      break;
    case MEP_OPERAND_SDISP16 :
      fields->f_16s16 = value;
      break;
    case MEP_OPERAND_SIMM16 :
      fields->f_16s16 = value;
      break;
    case MEP_OPERAND_SIMM16P0 :
      fields->f_ivc2_simm16p0 = value;
      break;
    case MEP_OPERAND_SIMM6 :
      fields->f_6s8 = value;
      break;
    case MEP_OPERAND_SIMM8 :
      fields->f_8s8 = value;
      break;
    case MEP_OPERAND_SIMM8P0 :
      fields->f_ivc2_8s0 = value;
      break;
    case MEP_OPERAND_SIMM8P20 :
      fields->f_ivc2_8s20 = value;
      break;
    case MEP_OPERAND_SIMM8P4 :
      fields->f_ivc2_8s4 = value;
      break;
    case MEP_OPERAND_SP :
      break;
    case MEP_OPERAND_SPR :
      break;
    case MEP_OPERAND_TP :
      break;
    case MEP_OPERAND_TPR :
      break;
    case MEP_OPERAND_UDISP2 :
      fields->f_2u6 = value;
      break;
    case MEP_OPERAND_UDISP7 :
      fields->f_7u9 = value;
      break;
    case MEP_OPERAND_UDISP7A2 :
      fields->f_7u9a2 = value;
      break;
    case MEP_OPERAND_UDISP7A4 :
      fields->f_7u9a4 = value;
      break;
    case MEP_OPERAND_UIMM16 :
      fields->f_16u16 = value;
      break;
    case MEP_OPERAND_UIMM2 :
      fields->f_2u10 = value;
      break;
    case MEP_OPERAND_UIMM24 :
      fields->f_24u8n = value;
      break;
    case MEP_OPERAND_UIMM3 :
      fields->f_3u5 = value;
      break;
    case MEP_OPERAND_UIMM4 :
      fields->f_4u8 = value;
      break;
    case MEP_OPERAND_UIMM5 :
      fields->f_5u8 = value;
      break;
    case MEP_OPERAND_UIMM7A4 :
      fields->f_7u9a4 = value;
      break;
    case MEP_OPERAND_ZERO :
      break;

    default :
      /* xgettext:c-format */
      opcodes_error_handler
	(_("internal error: unrecognized field %d while setting vma operand"),
	 opindex);
      abort ();
  }
}

/* Function to call before using the instruction builder tables.  */

void
mep_cgen_init_ibld_table (CGEN_CPU_DESC cd)
{
  cd->insert_handlers = & mep_cgen_insert_handlers[0];
  cd->extract_handlers = & mep_cgen_extract_handlers[0];

  cd->insert_operand = mep_cgen_insert_operand;
  cd->extract_operand = mep_cgen_extract_operand;

  cd->get_int_operand = mep_cgen_get_int_operand;
  cd->set_int_operand = mep_cgen_set_int_operand;
  cd->get_vma_operand = mep_cgen_get_vma_operand;
  cd->set_vma_operand = mep_cgen_set_vma_operand;
}
