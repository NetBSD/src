/* BFD back-end for 32-bit a.out files.
   Copyright (C) 1990, 91, 92, 93, 94 Free Software Foundation, Inc.
   Written by Cygnus Support.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define FPRINTF(x)

#ifndef FPRINTF
#define FPRINTF(x)	fprintf x
#endif

#define	N_HEADER_IN_TEXT(x)	1

#include <stdio.h>

#define ARCH_SIZE 32

#define BYTES_IN_WORD	4

#include "bfd.h"
#include "aout/aout64.h"

#define MY(op)	CAT(aoutarm_,op)
#define NAME(x, y)	CAT3(aoutarm,_32_,y)

#include "libaout.h"

static bfd_reloc_status_type
MY(fix_pcrel_26_done)	PARAMS ((bfd *, arelent *, asymbol *, PTR,
				 asection *, bfd *, char **));

static bfd_reloc_status_type
MY(fix_pcrel_26)	 PARAMS ((bfd *, arelent *, asymbol *, PTR,
				  asection *, bfd *, char **));

#define MY_swap_std_reloc_in MY(swap_std_reloc_in)
#define MY_swap_std_reloc_out MY(swap_std_reloc_out)

static void
MY_swap_std_reloc_in PARAMS ((bfd *abfd, struct reloc_std_external *bytes,
			      arelent *cache_ptr, asymbol **symbols,
			      bfd_size_type symcount));

static void
MY_swap_std_reloc_out PARAMS ((bfd *abfd, arelent *g,
			       struct reloc_std_external *natptr));

reloc_howto_type MY(howto_table)[] =
{
  /* type rs size bsz pcrel bitpos ovrf sf name part_inpl readmask setmask
     pcdone */
  HOWTO (0, 0, 0, 8, false, 0, complain_overflow_bitfield, 0, "8", true,
	 0x000000ff, 0x000000ff, false),
  HOWTO (1, 0, 1, 16, false, 0, complain_overflow_bitfield, 0, "16", true,
	 0x0000ffff, 0x0000ffff, false),
  HOWTO (2, 0, 2, 32, false, 0, complain_overflow_bitfield, 0, "32", true,
	 0xffffffff, 0xffffffff, false),
  HOWTO (3, 2, 2, 26, true, 0, complain_overflow_signed, MY(fix_pcrel_26),
	 "ARM26", true, 0x00ffffff, 0x00ffffff, true),
  HOWTO (4, 0, 0, 8, true, 0, complain_overflow_signed, 0, "DISP8", true,
	 0x000000ff, 0x000000ff, true),
  HOWTO (5, 0, 1, 16, true, 0, complain_overflow_signed, 0, "DISP16", true,
	 0x0000ffff, 0x0000ffff, true),
  HOWTO (6, 0, 2, 32, true, 0, complain_overflow_signed, 0, "DISP32", true,
	 0xffffffff, 0xffffffff, true),
  HOWTO (7, 2, 2, 26, false, 0, complain_overflow_signed,
	 MY(fix_pcrel_26_done), "ARM26D", true, 0x0, 0x0,
	 false),
  {-1},
  HOWTO (9, 0, -1, 16, false, 0, complain_overflow_bitfield, 0, "NEG16", true,
	 0x0000ffff, 0x0000ffff, false),
  HOWTO (10, 0, -2, 32, false, 0, complain_overflow_bitfield, 0, "NEG32", true,
	 0xffffffff, 0xffffffff, false),
  {-1},
  {-1},
  {-1},
  {-1},
  {-1},

  {-1},
  HOWTO (17, 0, 1, 16, false, 0, complain_overflow_bitfield, 0, "GOT12", true,
	 0x00000fff, 0x00000fff, false),
  HOWTO (18, 0, 2, 32, false, 0, complain_overflow_bitfield, 0, "GOT32", true,
	 0xffffffff, 0xffffffff, false),
  HOWTO (19, 0, 2, 26, true, 0, complain_overflow_bitfield, MY(fix_pcrel_26), "JMPSLOT", true,
	 0x00ffffff, 0x00ffffff, true),

  {-1},
  {-1},
  HOWTO (22, 0, 2, 32, true, 0, complain_overflow_bitfield, 0, "GOTPC", true,
	 0xffffffff, 0xffffffff, true),
  {-1},

  {-1},
  {-1},
  {-1},
  {-1},
  {-1},
  {-1},
  {-1},
  {-1},
};

#define RELOC_ARM_BITS_NEG_BIG      ((unsigned int) 0x08)
#define RELOC_ARM_BITS_NEG_LITTLE   ((unsigned int) 0x10)
#define RELOC_ARM_BITS_PIC_BIG      ((unsigned int) 0x04)
#define RELOC_ARM_BITS_PIC_LITTLE   ((unsigned int) 0x20)

reloc_howto_type *
MY(reloc_howto)(abfd, rel, r_index, r_extern, r_pcrel)
     bfd *abfd;
     struct reloc_std_external *rel;
     int *r_index;
     int *r_extern;
     int *r_pcrel;
{
  unsigned int r_length;
  unsigned int r_pcrel_done;
  unsigned int r_neg;
  unsigned int r_pic;
  int index;

FPRINTF((stderr, "%s:reloc_howto", __FILE__));

  *r_pcrel = 0;
  if (bfd_header_big_endian (abfd))
    {
      *r_index     =  ((rel->r_index[0] << 16)
		       | (rel->r_index[1] << 8)
		       | rel->r_index[2]);
      *r_extern    = (0 != (rel->r_type[0] & RELOC_STD_BITS_EXTERN_BIG));
      r_pcrel_done = (0 != (rel->r_type[0] & RELOC_STD_BITS_PCREL_BIG));
      r_neg 	   = (0 != (rel->r_type[0] & RELOC_ARM_BITS_NEG_BIG));
      r_pic 	   = (0 != (rel->r_type[0] & RELOC_ARM_BITS_PIC_BIG));
      r_length     = ((rel->r_type[0] & RELOC_STD_BITS_LENGTH_BIG)
		      >> RELOC_STD_BITS_LENGTH_SH_BIG);
    }
  else
    {
      *r_index     = ((rel->r_index[2] << 16)
		      | (rel->r_index[1] << 8)
		      | rel->r_index[0]);
      *r_extern    = (0 != (rel->r_type[0] & RELOC_STD_BITS_EXTERN_LITTLE));
      r_pcrel_done = (0 != (rel->r_type[0] & RELOC_STD_BITS_PCREL_LITTLE));
      r_neg 	   = (0 != (rel->r_type[0] & RELOC_ARM_BITS_NEG_LITTLE));
      r_pic 	   = (0 != (rel->r_type[0] & RELOC_ARM_BITS_PIC_LITTLE));
      r_length     = ((rel->r_type[0] & RELOC_STD_BITS_LENGTH_LITTLE)
		      >> RELOC_STD_BITS_LENGTH_SH_LITTLE);
    }
  index = r_length + 4 * r_pcrel_done + 8 * r_neg + 16 * r_pic;
  if (index == 3)
    *r_pcrel = 1;        
  if (index == 19)	/* jmpslot */
    *r_pcrel = 1;        
FPRINTF((stderr, " index=%d name=%s\n", index, (MY(howto_table) + index)->name));
  return MY(howto_table) + index;
}
 
#define MY_reloc_howto(BFD, REL, IN, EX, PC) \
	MY(reloc_howto) (BFD, REL, &IN, &EX, &PC)

void
MY(put_reloc)(abfd, r_extern, r_index, value, howto, reloc)
     bfd *abfd;
     int r_extern;
     int r_index;
     long value;
     reloc_howto_type *howto;
     struct reloc_std_external *reloc;
{
  unsigned int r_length;
  int r_pcrel;
  int r_neg;
  int r_pic;

FPRINTF((stderr, "%s:put_reloc\n", __FILE__));

  PUT_WORD (abfd, value, reloc->r_address);
  r_length = howto->size ;	/* Size as a power of two */

  /* Special case for branch relocations. */
  if (howto->type == 3 || howto->type == 7)
    r_length = 3;
  if (howto->type == 19)
    r_length = 3;

  r_pcrel  = howto->type & 4; 	/* PC Relative done? */
  r_neg = howto->type & 8;	/* Negative relocation */
  r_pic = howto->type & 16;	/* PIC relocation */
  if (bfd_header_big_endian (abfd))
    {
      reloc->r_index[0] = r_index >> 16;
      reloc->r_index[1] = r_index >> 8;
      reloc->r_index[2] = r_index;
      reloc->r_type[0] =
	((r_extern ?     RELOC_STD_BITS_EXTERN_BIG : 0)
	 | (r_pcrel ?    RELOC_STD_BITS_PCREL_BIG : 0)
	 | (r_neg ?	 RELOC_ARM_BITS_NEG_BIG : 0)
         | (r_pic ?	 RELOC_ARM_BITS_PIC_BIG : 0)
	 | (r_length <<  RELOC_STD_BITS_LENGTH_SH_BIG));
    }
  else
    {
      reloc->r_index[2] = r_index >> 16;
      reloc->r_index[1] = r_index >> 8;
      reloc->r_index[0] = r_index;
      reloc->r_type[0] =
	((r_extern ?     RELOC_STD_BITS_EXTERN_LITTLE : 0)
	 | (r_pcrel ?    RELOC_STD_BITS_PCREL_LITTLE : 0)
	 | (r_neg ?	 RELOC_ARM_BITS_NEG_LITTLE : 0)
         | (r_pic ?	 RELOC_ARM_BITS_PIC_LITTLE : 0)
	 | (r_length <<  RELOC_STD_BITS_LENGTH_SH_LITTLE));
    }
}
 
#define MY_put_reloc(BFD, EXT, IDX, VAL, HOWTO, RELOC) \
  MY(put_reloc)(BFD, EXT, IDX, VAL, HOWTO, RELOC)

#include "aoutx.h"

reloc_howto_type *
MY(bfd_reloc_type_lookup)(abfd,code)
     bfd *abfd;
     bfd_reloc_code_real_type code;
{
FPRINTF((stderr, "%s:bfd_reloc_type_lookup code=%d\n", __FILE__, (int)code));

#define ASTD(i,j)       case i: FPRINTF((stderr, "i=%d j=%d\n", i,j)); return &MY(howto_table)[j]
  if (code == BFD_RELOC_CTOR)
    switch (bfd_get_arch_info (abfd)->bits_per_address)
      {
      case 32:
        code = BFD_RELOC_32;
        break;
      default: return (CONST struct reloc_howto_struct *) 0;
      }

  switch (code)
    {
      ASTD (BFD_RELOC_16, 1);
      ASTD (BFD_RELOC_32, 2);
      ASTD (BFD_RELOC_ARM_PCREL_BRANCH, 3);
      ASTD (BFD_RELOC_8_PCREL, 4);
      ASTD (BFD_RELOC_16_PCREL, 5);
      ASTD (BFD_RELOC_32_PCREL, 6);
      ASTD (BFD_RELOC_ARM_GOT12, 17);
      ASTD (BFD_RELOC_ARM_GOT32, 18);
      ASTD (BFD_RELOC_ARM_GOTPC, 22);
      ASTD (BFD_RELOC_ARM_JMPSLOT, 19);
    default: return (CONST struct reloc_howto_struct *) 0;
    }
}

static bfd_reloc_status_type
MY(fix_pcrel_26) (abfd, reloc_entry, symbol, data, input_section,
		       output_bfd, error_message)
     bfd *abfd;
     arelent *reloc_entry;
     asymbol *symbol;
     PTR data;
     asection *input_section;
     bfd *output_bfd;
     char **error_message;
{
  bfd_vma relocation;
  bfd_size_type addr = reloc_entry->address;
  long target = bfd_get_32 (abfd, (bfd_byte *) data + addr);
  bfd_reloc_status_type flag = bfd_reloc_ok;

FPRINTF((stderr, "%s:fix_pcrel_26\n", __FILE__));
FPRINTF((stderr, "reloc: target=%x sym->val=%x sym->vma=%x sym->off=%x\n", target, symbol->value, symbol->section->output_section->vma, symbol->section->output_offset));
FPRINTF((stderr, "reloc: addend=%x ins->vma=%x ins->off=%x addr=%x\n", reloc_entry->addend, input_section->output_section->vma, input_section->output_offset, addr));

  /* If this is an undefined symbol, return error */
  if (symbol->section == &bfd_und_section
      && (symbol->flags & BSF_WEAK) == 0)
    return output_bfd ? bfd_reloc_ok : bfd_reloc_undefined;

  /* If the sections are different, and we are doing a partial relocation,
     just ignore it for now.  */
  if (symbol->section->name != input_section->name
      && output_bfd != (bfd *)NULL)
    return bfd_reloc_ok;

FPRINTF((stderr, "doing relocation\n"));

  relocation = (target & 0x00ffffff) << 2;
  relocation = (relocation ^ 0x02000000) - 0x02000000; /* Sign extend */
  relocation += symbol->value;
  relocation += symbol->section->output_section->vma;
  relocation += symbol->section->output_offset;
  relocation += reloc_entry->addend;
  relocation -= input_section->output_section->vma;
  relocation -= input_section->output_offset;
  relocation -= addr;
  if (relocation & 3)
    return bfd_reloc_overflow;

  /* Check for overflow */
  if (relocation & 0x02000000)
    {
      if ((relocation & ~0x03ffffff) != ~0x03ffffff)
	flag = bfd_reloc_overflow;
    }
  else if (relocation & ~0x03ffffff)
    flag = bfd_reloc_overflow;

  target &= ~0x00ffffff;
  target |= (relocation >> 2) & 0x00ffffff;
  bfd_put_32 (abfd, target, (bfd_byte *) data + addr);

  /* Now the ARM magic... Change the reloc type so that it is marked as done.
     Strictly this is only necessary if we are doing a partial relocation.  */
  reloc_entry->howto = &MY(howto_table)[7];
  
  return flag;
}

static bfd_reloc_status_type
MY(fix_pcrel_26_done) (abfd, reloc_entry, symbol, data, input_section,
		       output_bfd, error_message)
     bfd *abfd;
     arelent *reloc_entry;
     asymbol *symbol;
     PTR data;
     asection *input_section;
     bfd *output_bfd;
     char **error_message;
{
  bfd_size_type addr = reloc_entry->address;
  long target = bfd_get_32 (abfd, (bfd_byte *) data + addr);

FPRINTF((stderr, "%s:fix_pcrel_26_done\n", __FILE__));
FPRINTF((stderr, "reloc: target=%x sym->val=%x sym->vma=%x sym->off=%x\n", target, symbol->value, symbol->section->output_section->vma, symbol->section->output_offset));
FPRINTF((stderr, "reloc: addend=%x ins->vma=%x ins->off=%x addr=%x\n", reloc_entry->addend, input_section->output_section->vma, input_section->output_offset, addr));
  return bfd_reloc_ok;
}

static void
MY_swap_std_reloc_in (abfd, bytes, cache_ptr, symbols, symcount)
     bfd *abfd;
     struct reloc_std_external *bytes;
     arelent *cache_ptr;
     asymbol **symbols;
     bfd_size_type symcount;
{
  int r_index;
  int r_extern;
  unsigned int r_length;
  int r_pcrel;
  struct aoutdata *su = &(abfd->tdata.aout_data->a);

  FPRINTF((stderr, "%s:swap_std_reloc_in\n", __FILE__));

  cache_ptr->address = bfd_h_get_32 (abfd, bytes->r_address);

  cache_ptr->howto = MY_reloc_howto (abfd, bytes, r_index, r_extern, r_pcrel);

  FPRINTF((stderr, "%s:swap_std_reloc_in: type=%d name=%s\n", __FILE__, cache_ptr->howto->type, cache_ptr->howto->name));

  MOVE_ADDRESS (0);
}

void
MY_swap_std_reloc_out (abfd, g, natptr)
     bfd *abfd;
     arelent *g;
     struct reloc_std_external *natptr;
{
  int r_index;
  asymbol *sym = *(g->sym_ptr_ptr);
  int r_extern;
  int r_length;
  int r_pcrel;
  int r_neg = 0;	/* Negative relocs use the BASEREL bit.  */
  int r_pic = 0;
  asection *output_section = sym->section->output_section;

  FPRINTF((stderr, "%s:swap_std_reloc_out type=%d name=%s: ga=%x na=%x", __FILE__, g->howto->type, g->howto->name, g->address, natptr->r_address));

  PUT_WORD(abfd, g->address, natptr->r_address);

  r_length = g->howto->size ;   /* Size as a power of two */
  if (r_length < 0)
    {
      r_length = -r_length;
      r_neg = 1;
    }

  r_pcrel  = (int) g->howto->pc_relative; /* Relative to PC? */

  /* For RISC iX, in pc-relative relocs the r_pcrel bit means that the
     relocation has been done already (Only for the 26-bit one I think)???!!!
     */

  if (g->howto->type == 3)
    {
      r_length = 3;
      r_pcrel = 0;
    }
  else if (g->howto->type == 7)
    { 
      r_length = 3;
      r_pcrel = 1;
    }

  /* Are we base relative i.e. PIC */
  if (g->howto->type >= 16)
    r_pic = 1;

  /* jmpslot (branch with pic set) - treat like a branch */
  if (g->howto->type == 19) {
      r_length = 3;
      r_pcrel = 0;
     }
#if 0
  /* For a standard reloc, the addend is in the object file.  */
  r_addend = g->addend + (*(g->sym_ptr_ptr))->section->output_section->vma;
#endif

  /* name was clobbered by aout_write_syms to be symbol index */

  /* If this relocation is relative to a symbol then set the
     r_index to the symbols index, and the r_extern bit.

     Absolute symbols can come in in two ways, either as an offset
     from the abs section, or as a symbol which has an abs value.
     check for that here
     */

  if (bfd_is_com_section (output_section)
      || output_section == &bfd_abs_section
      || output_section == &bfd_und_section)
    {
      if (bfd_abs_section.symbol == sym)
	{
	  /* Whoops, looked like an abs symbol, but is really an offset
	     from the abs section */
	  r_index = 0;
	  r_extern = 0;
	}
      else
	{
	  /* Fill in symbol */
	  r_extern = 1;
	  r_index = (*(g->sym_ptr_ptr))->KEEPIT;
	}
    }
  else
    {
      /* Just an ordinary section */
      r_extern = 0;
        r_index  = output_section->target_index;
      if (r_pic && g->howto->type == 18) {
	r_index = (*(g->sym_ptr_ptr))->KEEPIT;
/*	r_extern = 1;*/	/*260697*/
	if (sym->flags & BSF_GLOBAL)
		r_extern = 1;
	FPRINTF((stderr, "*"));
      }
    }
FPRINTF((stderr, " index=%d\n", r_index));
  /* now the fun stuff */
  if (bfd_header_big_endian (abfd))
    {
      natptr->r_index[0] = r_index >> 16;
      natptr->r_index[1] = r_index >> 8;
      natptr->r_index[2] = r_index;
      natptr->r_type[0] =
	(  (r_extern ?   RELOC_STD_BITS_EXTERN_BIG: 0)
	 | (r_pcrel  ?   RELOC_STD_BITS_PCREL_BIG: 0)
	 | (r_neg    ?   RELOC_ARM_BITS_NEG_BIG: 0)
	 | (r_pic    ?   RELOC_ARM_BITS_PIC_BIG: 0)
	 | (r_length <<  RELOC_STD_BITS_LENGTH_SH_BIG));
    }
  else
    {
      natptr->r_index[2] = r_index >> 16;
      natptr->r_index[1] = r_index >> 8;
      natptr->r_index[0] = r_index;
      natptr->r_type[0] =
	(  (r_extern ?   RELOC_STD_BITS_EXTERN_LITTLE: 0)
	 | (r_pcrel  ?   RELOC_STD_BITS_PCREL_LITTLE: 0)
	 | (r_neg    ?   RELOC_ARM_BITS_NEG_LITTLE: 0)
	 | (r_pic    ?   RELOC_ARM_BITS_PIC_LITTLE: 0)
	 | (r_length <<  RELOC_STD_BITS_LENGTH_SH_LITTLE));
    }
}

