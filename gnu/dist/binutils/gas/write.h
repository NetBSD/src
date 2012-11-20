/* write.h
   Copyright 1987, 1992, 1993, 1994, 1995, 1996, 1997, 1999, 2000, 2001,
   2002, 2003 Free Software Foundation, Inc.

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

#ifndef __write_h__
#define __write_h__

#ifndef TC_I960
#ifdef hpux
#define EXEC_MACHINE_TYPE HP9000S200_ID
#endif
#endif /* TC_I960 */

#ifndef BFD_ASSEMBLER

#ifndef LOCAL_LABEL
#define LOCAL_LABEL(name) (name [0] == 'L' )
#endif

#define S_LOCAL_NAME(s) (LOCAL_LABEL (S_GET_NAME (s)))

#endif /* ! BFD_ASSEMBLER */

/* This is the name of a fake symbol which will never appear in the
   assembler output.  S_IS_LOCAL detects it because of the \001.  */
#ifndef FAKE_LABEL_NAME
#define FAKE_LABEL_NAME "L0\001"
#endif

#include "bit_fix.h"

/*
 * FixSs may be built up in any order.
 */

struct fix
{
  /* These small fields are grouped together for compactness of
     this structure, and efficiency of access on some architectures.  */

  /* pc-relative offset adjust (only used by m68k) */
  char fx_pcrel_adjust;

  /* How many bytes are involved? */
  unsigned char fx_size;

  /* Is this a pc-relative relocation?  */
  unsigned fx_pcrel : 1;

  /* Is this a relocation to a procedure linkage table entry?  If so,
     some of the reductions we try to apply are invalid.  A better way
     might be to represent PLT entries with different kinds of
     symbols, and use normal relocations (with undefined symbols);
     look into it for version 2.6.  */
  unsigned fx_plt : 1;

  /* Is this value an immediate displacement?  */
  /* Only used on i960 and ns32k; merge it into TC_FIX_TYPE sometime.  */
  unsigned fx_im_disp : 2;

  /* A bit for the CPU specific code.
     This probably can be folded into tc_fix_data, below.  */
  unsigned fx_tcbit : 1;

  /* Has this relocation already been applied?  */
  unsigned fx_done : 1;

  /* Suppress overflow complaints on large addends.  This is used
     in the PowerPC ELF config to allow large addends on the
     BFD_RELOC_{LO16,HI16,HI16_S} relocations.

     @@ Can this be determined from BFD?  */
  unsigned fx_no_overflow : 1;

  /* The value is signed when checking for overflow.  */
  unsigned fx_signed : 1;

  /* Which frag does this fix apply to?  */
  fragS *fx_frag;

  /* Where is the first byte to fix up?  */
  long fx_where;

  /* NULL or Symbol whose value we add in.  */
  symbolS *fx_addsy;

  /* NULL or Symbol whose value we subtract.  */
  symbolS *fx_subsy;

  /* Absolute number we add in.  */
  valueT fx_offset;

  /* The value of dot when the fixup expression was parsed.  */
  addressT fx_dot_value;

  /* Next fixS in linked list, or NULL.  */
  struct fix *fx_next;

  /* If NULL, no bitfix's to do.  */
  /* Only i960-coff and ns32k use this, and i960-coff stores an
     integer.  This can probably be folded into tc_fix_data, below.
     @@ Alpha also uses it, but only to disable certain relocation
     processing.  */
  bit_fixS *fx_bit_fixP;

#ifdef BFD_ASSEMBLER
  bfd_reloc_code_real_type fx_r_type;
#else
#ifdef NEED_FX_R_TYPE
  /* Hack for machines where the type of reloc can't be
     worked out by looking at how big it is.  */
  int fx_r_type;
#endif
#endif

  /* This field is sort of misnamed.  It appears to be a sort of random
     scratch field, for use by the back ends.  The main gas code doesn't
     do anything but initialize it to zero.  The use of it does need to
     be coordinated between the cpu and format files, though.  E.g., some
     coff targets pass the `addend' field from the cpu file via this
     field.  I don't know why the `fx_offset' field above can't be used
     for that; investigate later and document. KR  */
  valueT fx_addnumber;

  /* The location of the instruction which created the reloc, used
     in error messages.  */
  char *fx_file;
  unsigned fx_line;

#ifdef USING_CGEN
  struct {
    /* CGEN_INSN entry for this instruction.  */
    const struct cgen_insn *insn;
    /* Target specific data, usually reloc number.  */
    int opinfo;
  } fx_cgen;
#endif

#ifdef TC_FIX_TYPE
  /* Location where a backend can attach additional data
     needed to perform fixups.  */
  TC_FIX_TYPE tc_fix_data;
#endif
};

typedef struct fix fixS;

extern int finalize_syms;
extern symbolS *abs_section_sym;
extern addressT dot_value;

#ifndef BFD_ASSEMBLER
extern char *next_object_file_charP;

#ifndef MANY_SEGMENTS
COMMON fixS *text_fix_root, *text_fix_tail;	/* Chains fixSs.  */
COMMON fixS *data_fix_root, *data_fix_tail;	/* Chains fixSs.  */
COMMON fixS *bss_fix_root, *bss_fix_tail;	/* Chains fixSs.  */
extern struct frag *text_last_frag;		/* Last frag in segment.  */
extern struct frag *data_last_frag;		/* Last frag in segment.  */
#endif
COMMON fixS **seg_fix_rootP, **seg_fix_tailP;	/* -> one of above.  */
#endif

extern long string_byte_count;
extern int section_alignment[];

extern bit_fixS *bit_fix_new
  (int size, int offset, long base_type, long base_adj, long min,
   long max, long add);
extern void append (char **charPP, char *fromP, unsigned long length);
extern void record_alignment (segT seg, int align);
extern int get_recorded_alignment (segT seg);
extern void subsegs_finish (void);
extern void write_object_file (void);
extern long relax_frag (segT, fragS *, long);
extern int relax_segment (struct frag * seg_frag_root, segT seg_type);

extern void number_to_chars_littleendian (char *, valueT, int);
extern void number_to_chars_bigendian (char *, valueT, int);

#ifdef BFD_ASSEMBLER
extern fixS *fix_new
  (fragS * frag, int where, int size, symbolS * add_symbol,
   offsetT offset, int pcrel, bfd_reloc_code_real_type r_type);
extern fixS *fix_new_exp
  (fragS * frag, int where, int size, expressionS *exp, int pcrel,
   bfd_reloc_code_real_type r_type);
#else
extern fixS *fix_new
  (fragS * frag, int where, int size, symbolS * add_symbol,
   offsetT offset, int pcrel, int r_type);
extern fixS *fix_new_exp
  (fragS * frag, int where, int size, expressionS *exp, int pcrel,
   int r_type);
#endif

extern void write_print_statistics (FILE *);

#endif /* __write_h__ */
