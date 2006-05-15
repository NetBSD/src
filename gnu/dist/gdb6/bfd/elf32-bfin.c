/* ADI Blackfin BFD support for 32-bit ELF. 
   Copyright 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301,
   USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/bfin.h"

/* Handling expression relocations for blackfin.  Blackfin
   will generate relocations in an expression form with a stack.
   A relocation such as P1.H  = _typenames-4000000;
   will generate the following relocs at offset 4:
00000004 R_expst_push      _typenames
00000004 R_expst_const     .__constant
00000004 R_expst_sub       .__operator
00000006 R_huimm16         .__operator

   The .__constant and .__operator symbol names are fake.
   Special case is a single relocation
     P1.L  = _typenames; generates
00000002 R_luimm16         _typenames

   Thus, if you get a R_luimm16, R_huimm16, R_imm16,
   if the stack is not empty, pop the stack and
   put the value, else do the normal thing
   We will currently assume that the max the stack
   would grow to is 100. .  */

#define RELOC_STACK_SIZE 100
static bfd_vma reloc_stack[RELOC_STACK_SIZE];
static unsigned int reloc_stack_tos = 0;

#define is_reloc_stack_empty() ((reloc_stack_tos > 0) ? 0 : 1)

static void
reloc_stack_push (bfd_vma value)
{
  reloc_stack[reloc_stack_tos++] = value;
}

static bfd_vma
reloc_stack_pop (void)
{
  return reloc_stack[--reloc_stack_tos];
}

static bfd_vma
reloc_stack_operate (unsigned int oper)
{
  bfd_vma value;
  switch (oper)
    {
    case R_add:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] + reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_sub:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] - reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_mult:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] * reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_div:
      {
	if (reloc_stack[reloc_stack_tos - 1] == 0)
	  {
	    _bfd_abort (__FILE__, __LINE__, _("Division by zero. "));
	  }
	else
	  {
	    value =
	      reloc_stack[reloc_stack_tos - 2] / reloc_stack[reloc_stack_tos - 1];
	    reloc_stack_tos -= 2;
	  }
	break;
      }
    case R_mod:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] % reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_lshift:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] << reloc_stack[reloc_stack_tos -
							  1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_rshift:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] >> reloc_stack[reloc_stack_tos -
							  1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_and:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] & reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_or:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] | reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_xor:
      {
	value =
	  reloc_stack[reloc_stack_tos - 2] ^ reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_land:
      {
	value = reloc_stack[reloc_stack_tos - 2]
	  && reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_lor:
      {
	value = reloc_stack[reloc_stack_tos - 2]
	  || reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 2;
	break;
      }
    case R_neg:
      {
	value = -reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos--;
	break;
      }
    case R_comp:
      {
	value = ~reloc_stack[reloc_stack_tos - 1];
	reloc_stack_tos -= 1;
	break;
      }
    default:
      {
	fprintf (stderr, "bfin relocation : Internal bug\n");
	return 0;
      }
    }

  reloc_stack_push (value);

  return value;
}

/* FUNCTION : bfin_pltpc_reloc
   ABSTRACT : TODO : figure out how to handle pltpc relocs.  */
static bfd_reloc_status_type
bfin_pltpc_reloc (
     bfd *abfd ATTRIBUTE_UNUSED,
     arelent *reloc_entry ATTRIBUTE_UNUSED,
     asymbol *symbol ATTRIBUTE_UNUSED,
     PTR data ATTRIBUTE_UNUSED,
     asection *input_section ATTRIBUTE_UNUSED,
     bfd *output_bfd ATTRIBUTE_UNUSED,
     char **error_message ATTRIBUTE_UNUSED) 
{
  bfd_reloc_status_type flag = bfd_reloc_ok;
  return flag; 
}


static bfd_reloc_status_type
bfin_pcrel24_reloc (bfd *abfd,
                    arelent *reloc_entry,
                    asymbol *symbol,
                    PTR data,
                    asection *input_section,
                    bfd *output_bfd,
                    char **error_message ATTRIBUTE_UNUSED)
{
  bfd_vma relocation;
  bfd_size_type addr = reloc_entry->address;
  bfd_vma output_base = 0;
  reloc_howto_type *howto = reloc_entry->howto;
  asection *output_section;
  bfd_boolean relocatable = (output_bfd != NULL);

  if (reloc_entry->address > bfd_get_section_limit (abfd, input_section))
    return bfd_reloc_outofrange;

  if (!is_reloc_stack_empty ())
    relocation = reloc_stack_pop();
  else
    {
      if (bfd_is_und_section (symbol->section)
          && (symbol->flags & BSF_WEAK) == 0
          && !relocatable)
        return bfd_reloc_undefined;

      if (bfd_is_com_section (symbol->section))
	relocation = 0;
      else
	relocation = symbol->value;       

      output_section = symbol->section->output_section;

      if (relocatable)
	output_base = 0;
      else
	output_base = output_section->vma;
      
      if (!relocatable || !strcmp (symbol->name, symbol->section->name))
	relocation += output_base + symbol->section->output_offset;
        
      if (!relocatable && !strcmp (symbol->name, symbol->section->name))
        relocation += reloc_entry->addend;
    }
      
  relocation -= input_section->output_section->vma + input_section->output_offset;
  relocation -= reloc_entry->address;

  if (howto->complain_on_overflow != complain_overflow_dont)
    {
      bfd_reloc_status_type status;
      status= bfd_check_overflow (howto->complain_on_overflow, 
                                  howto->bitsize,
                                  howto->rightshift, 
                                  bfd_arch_bits_per_address(abfd),
                                  relocation);      
      if (status != bfd_reloc_ok)
	return status;
    }
      
  /* if rightshift is 1 and the number odd, return error.  */
  if (howto->rightshift && (relocation & 0x01))
    {
      fprintf(stderr, "relocation should be even number\n");
      return bfd_reloc_overflow;
    }

  relocation >>= (bfd_vma) howto->rightshift;
  /* Shift everything up to where it's going to be used.  */

  relocation <<= (bfd_vma) howto->bitpos;

  if (relocatable)
    {
      reloc_entry->address += input_section->output_offset;
      reloc_entry->addend += symbol->section->output_offset;
    }

  {
    short x;

    /* We are getting reloc_entry->address 2 byte off from
    the start of instruction. Assuming absolute postion
    of the reloc data. But, following code had been written assuming 
    reloc address is starting at begining of instruction.
    To compensate that I have increased the value of 
    relocation by 1 (effectively 2) and used the addr -2 instead of addr.  */ 

    relocation += 1;
    x = bfd_get_16 (abfd, (bfd_byte *) data + addr - 2);
    x = (x & 0xff00) | ((relocation >> 16) & 0xff);
    bfd_put_16 (abfd, x, (unsigned char *) data + addr - 2);

    x = bfd_get_16 (abfd, (bfd_byte *) data + addr);
    x = relocation & 0xFFFF;
    bfd_put_16 (abfd, x, (unsigned char *) data + addr );
  }
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
bfin_push_reloc (bfd *abfd ATTRIBUTE_UNUSED,
     		 arelent *reloc_entry,
     		 asymbol *symbol,
     		 PTR data ATTRIBUTE_UNUSED,
     		 asection *input_section,
     		 bfd *output_bfd,
     		 char **error_message ATTRIBUTE_UNUSED) 
{
  bfd_vma relocation;
  bfd_vma output_base = 0;
  asection *output_section;
  bfd_boolean relocatable = (output_bfd != NULL);

  if (bfd_is_und_section (symbol->section)
      && (symbol->flags & BSF_WEAK) == 0
      && !relocatable)
    return bfd_reloc_undefined;

  /* Is the address of the relocation really within the section?  */
  if (reloc_entry->address > bfd_get_section_limit(abfd, input_section))
     return bfd_reloc_outofrange;
      
  output_section = symbol->section->output_section;
  relocation = symbol->value;      

  /* Convert input-section-relative symbol value to absolute.  */
  if (relocatable)
    output_base = 0;
  else
    output_base = output_section->vma;

  if (!relocatable || !strcmp(symbol->name, symbol->section->name))
    relocation += output_base + symbol->section->output_offset;

  /* Add in supplied addend.  */
  relocation += reloc_entry->addend;

  if (relocatable)
    {
      reloc_entry->address += input_section->output_offset;
      reloc_entry->addend += symbol->section->output_offset;
    }

  /* Now that we have the value, push it. */
  reloc_stack_push (relocation);
  
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
bfin_oper_reloc (bfd *abfd ATTRIBUTE_UNUSED,
     		 arelent *reloc_entry,
     		 asymbol *symbol ATTRIBUTE_UNUSED,
     		 PTR data ATTRIBUTE_UNUSED,
     		 asection *input_section,
     		 bfd *output_bfd,
     		 char **error_message ATTRIBUTE_UNUSED) 
{
  bfd_boolean relocatable = (output_bfd != NULL);

  /* Just call the operation based on the reloc_type.  */
  reloc_stack_operate (reloc_entry->howto->type);
  
  if (relocatable)
    reloc_entry->address += input_section->output_offset;

  return bfd_reloc_ok;
}

static bfd_reloc_status_type
bfin_const_reloc (bfd *abfd ATTRIBUTE_UNUSED,
     		  arelent *reloc_entry,
     		  asymbol *symbol ATTRIBUTE_UNUSED,
     		  PTR data ATTRIBUTE_UNUSED,
     		  asection *input_section,
     		  bfd *output_bfd,
     		  char **error_message ATTRIBUTE_UNUSED) 
{
  bfd_boolean relocatable = (output_bfd != NULL);

  /* Push the addend portion of the relocation.  */
  reloc_stack_push (reloc_entry->addend);

  if (relocatable)
    reloc_entry->address += input_section->output_offset;
  
  return bfd_reloc_ok;
}

static bfd_reloc_status_type
bfin_imm16_reloc (bfd *abfd,
     		  arelent *reloc_entry,
     		  asymbol *symbol,
     		  PTR data,
     		  asection *input_section,
     		  bfd *output_bfd,
     		  char **error_message ATTRIBUTE_UNUSED) 
{
  bfd_vma relocation, x;
  bfd_size_type reloc_addr = reloc_entry->address;
  bfd_vma output_base = 0;
  reloc_howto_type *howto = reloc_entry->howto;
  asection *output_section;
  bfd_boolean relocatable = (output_bfd != NULL);

  /* Is the address of the relocation really within the section?  */
  if (reloc_entry->address > bfd_get_section_limit (abfd, input_section))
    return bfd_reloc_outofrange;

  if (is_reloc_stack_empty ())
    {
      if (bfd_is_und_section (symbol->section)
          && (symbol->flags & BSF_WEAK) == 0
          && !relocatable)
        return bfd_reloc_undefined;

      output_section = symbol->section->output_section;
      relocation = symbol->value;      

      /* Convert input-section-relative symbol value to absolute.  */
      if (relocatable)
        output_base = 0;
      else
	output_base = output_section->vma;
  
      if (!relocatable || !strcmp (symbol->name, symbol->section->name))
	relocation += output_base + symbol->section->output_offset;

      /* Add in supplied addend.  */
      relocation += reloc_entry->addend;
    }
  else
    {
      relocation = reloc_stack_pop ();
    }

  if (relocatable)
    {	              
      reloc_entry->address += input_section->output_offset;
      reloc_entry->addend += symbol->section->output_offset;
    }
  else
    {
      reloc_entry->addend = 0;
    }

  if (howto->complain_on_overflow != complain_overflow_dont)
    {
      bfd_reloc_status_type flag;
      flag = bfd_check_overflow (howto->complain_on_overflow,
                                 howto->bitsize,
                                 howto->rightshift,
                                 bfd_arch_bits_per_address(abfd),
                                 relocation);
      if (flag != bfd_reloc_ok)
        return flag;
    }


  /* Here the variable relocation holds the final address of the
     symbol we are relocating against, plus any addend.  */

  relocation >>= (bfd_vma) howto->rightshift;
  x = relocation;
  bfd_put_16 (abfd, x, (unsigned char *) data + reloc_addr);
  return bfd_reloc_ok;
}


static bfd_reloc_status_type
bfin_byte4_reloc (bfd *abfd,
                  arelent *reloc_entry,
                  asymbol *symbol,
                  PTR data,
                  asection *input_section,
                  bfd *output_bfd,
                  char **error_message ATTRIBUTE_UNUSED) 
{
  bfd_vma relocation, x;
  bfd_size_type addr = reloc_entry->address;
  bfd_vma output_base = 0;
  asection *output_section;
  bfd_boolean relocatable = (output_bfd != NULL);

  /* Is the address of the relocation really within the section?  */
  if (reloc_entry->address > bfd_get_section_limit (abfd, input_section))
    return bfd_reloc_outofrange;

  if (is_reloc_stack_empty ())
    {
      if (bfd_is_und_section (symbol->section)
          && (symbol->flags & BSF_WEAK) == 0
          && !relocatable)
        return bfd_reloc_undefined;

      output_section = symbol->section->output_section;
      relocation = symbol->value;      
      /* Convert input-section-relative symbol value to absolute.  */
      if (relocatable)
	output_base = 0;
      else
	output_base = output_section->vma;
  
      if ((symbol->name 
	  && symbol->section->name
          && !strcmp (symbol->name, symbol->section->name))
          || !relocatable)
        {
	  relocation += output_base + symbol->section->output_offset;
	}

      relocation += reloc_entry->addend;
    }
  else
    {
      relocation = reloc_stack_pop();
      relocation += reloc_entry->addend;
    }

  if (relocatable)
    { 
      /* This output will be relocatable ... like ld -r. */
      reloc_entry->address += input_section->output_offset;
      reloc_entry->addend += symbol->section->output_offset;
    }
  else
    {
      reloc_entry->addend = 0;
    }

  /* Here the variable relocation holds the final address of the
     symbol we are relocating against, plus any addend.  */
  x = relocation & 0xFFFF0000;
  x >>=16;
  bfd_put_16 (abfd, x, (unsigned char *) data + addr + 2);
            
  x = relocation & 0x0000FFFF;
  bfd_put_16 (abfd, x, (unsigned char *) data + addr);
  return bfd_reloc_ok;
}

/* bfin_bfd_reloc handles the blackfin arithmetic relocations.
   Use this instead of bfd_perform_relocation.  */
static bfd_reloc_status_type
bfin_bfd_reloc (bfd *abfd,
		arelent *reloc_entry,
     		asymbol *symbol,
     		PTR data,
     		asection *input_section,
     		bfd *output_bfd,
     		char **error_message ATTRIBUTE_UNUSED)
{
  bfd_vma relocation;
  bfd_size_type addr = reloc_entry->address;
  bfd_vma output_base = 0;
  reloc_howto_type *howto = reloc_entry->howto;
  asection *output_section;
  bfd_boolean relocatable = (output_bfd != NULL);

  /* Is the address of the relocation really within the section?  */
  if (reloc_entry->address > bfd_get_section_limit (abfd, input_section))
    return bfd_reloc_outofrange;

  if (is_reloc_stack_empty())
    {
      if (bfd_is_und_section (symbol->section)
          && (symbol->flags & BSF_WEAK) == 0
          && !relocatable)
        return bfd_reloc_undefined;

      /* Get symbol value.  (Common symbols are special.)  */
      if (bfd_is_com_section (symbol->section))
        relocation = 0;
      else
        relocation = symbol->value;       
  
      output_section = symbol->section->output_section;
        
      /* Convert input-section-relative symbol value to absolute.  */
      if (relocatable)
	output_base = 0;
      else
	output_base = output_section->vma;
        
      if (!relocatable || !strcmp (symbol->name, symbol->section->name))
        relocation += output_base + symbol->section->output_offset;

     if (!relocatable && !strcmp (symbol->name, symbol->section->name))
       {
         /* Add in supplied addend.  */
         relocation += reloc_entry->addend;
       }
        
    }
  else
    {
      relocation = reloc_stack_pop();
    }
      
  /* Here the variable relocation holds the final address of the
     symbol we are relocating against, plus any addend.  */

  if (howto->pc_relative == TRUE)
    {
      relocation -= input_section->output_section->vma + input_section->output_offset;

      if (howto->pcrel_offset == TRUE)
        relocation -= reloc_entry->address;
    }

  if (relocatable)
    {
      reloc_entry->address += input_section->output_offset;
      reloc_entry->addend += symbol->section->output_offset;
    }

  if (howto->complain_on_overflow != complain_overflow_dont)
    {
      bfd_reloc_status_type status;

      status = bfd_check_overflow (howto->complain_on_overflow, 
                                  howto->bitsize,
                                  howto->rightshift, 
                                  bfd_arch_bits_per_address(abfd),
                                  relocation);
      if (status != bfd_reloc_ok)
	return status;
    }
      
  /* If rightshift is 1 and the number odd, return error.  */
  if (howto->rightshift && (relocation & 0x01))
    {
      fprintf(stderr, "relocation should be even number\n");
      return bfd_reloc_overflow;
    }

  relocation >>= (bfd_vma) howto->rightshift;

  /* Shift everything up to where it's going to be used.  */

  relocation <<= (bfd_vma) howto->bitpos;

#define DOIT(x) \
  x = ( (x & ~howto->dst_mask) | (relocation & howto->dst_mask))

  /* handle 8 and 16 bit relocations here. */
  switch (howto->size)
    {
    case 0:
      {
        char x = bfd_get_8 (abfd, (char *) data + addr);
        DOIT (x);
        bfd_put_8 (abfd, x, (unsigned char *) data + addr);
      }
      break;

    case 1:
      {
        unsigned short x = bfd_get_16 (abfd, (bfd_byte *) data + addr);
        DOIT (x);
        bfd_put_16 (abfd, (bfd_vma) x, (unsigned char *) data + addr);
      }
      break;

    default:
      return bfd_reloc_other;
    }

   return bfd_reloc_ok;
}

#if 0
static bfd_reloc_status_type bfin_bfd_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_imm16_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_pcrel24_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_pltpc_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_const_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_oper_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_byte4_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_reloc_status_type bfin_push_reloc
  PARAMS ((bfd *, arelent *, asymbol *, PTR, asection *, bfd *, char **));

static bfd_boolean bfin_is_local_label_name
  PARAMS ((bfd *, const char *));
#endif
bfd_boolean bfd_bfin_elf32_create_embedded_relocs
  PARAMS ((bfd *, struct bfd_link_info *, asection *, asection *, char **));


/* HOWTO Table for blackfin.
   Blackfin relocations are fairly complicated.
   Some of the salient features are
   a. Even numbered offsets. A number of (not all) relocations are
      even numbered. This means that the rightmost bit is not stored.
      Needs to right shift by 1 and check to see if value is not odd
   b. A relocation can be an expression. An expression takes on
      a variety of relocations arranged in a stack.
   As a result, we cannot use the standard generic function as special
   function. We will have our own, which is very similar to the standard
   generic function except that it understands how to get the value from
   the relocation stack. .  */

#define BFIN_RELOC_MIN 0
#define BFIN_RELOC_MAX 0x13
#define BFIN_GNUEXT_RELOC_MIN 0x40
#define BFIN_GNUEXT_RELOC_MAX 0x43
#define BFIN_ARELOC_MIN 0xE0
#define BFIN_ARELOC_MAX 0xF3

static reloc_howto_type bfin_howto_table [] =
{
  /* This reloc does nothing. .  */
  HOWTO (R_unused0,		/* type.  */
	 0,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 32,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_bitfield, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_unused0",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_pcrel5m2,		/* type.  */
	 1,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long)..  */
	 4,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_unsigned, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_pcrel5m2",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x0000000F,		/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_unused1,		/* type.  */
	 0,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 32,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_bitfield, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_unused1",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_pcrel10,		/* type.  */
	 1,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 10,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_pcrel10",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x000003FF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */
 
  HOWTO (R_pcrel12_jump,	/* type.  */
	 1,			/* rightshift.  */
				/* the offset is actually 13 bit
				   aligned on a word boundary so
				   only 12 bits have to be used.
				   Right shift the rightmost bit..  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 12,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_pcrel12_jump",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x0FFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_rimm16,		/* type.  */
	 0,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 16,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_imm16_reloc,	/* special_function.  */
	 "R_rimm16",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x0000FFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_luimm16,		/* type.  */
	 0,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 16,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_imm16_reloc,	/* special_function.  */
	 "R_luimm16",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x0000FFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */
 
  HOWTO (R_huimm16,		/* type.  */
	 16,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 16,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_unsigned, /* complain_on_overflow.  */
	 bfin_imm16_reloc,	/* special_function.  */
	 "R_huimm16",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x0000FFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_pcrel12_jump_s,	/* type.  */
	 1,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 12,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_pcrel12_jump_s",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x00000FFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_pcrel24_jump_x,	/* type.  */
         1,			/* rightshift.  */
         2,			/* size (0 = byte, 1 = short, 2 = long).  */
         24,			/* bitsize.  */
         TRUE,			/* pc_relative.  */
         0,			/* bitpos.  */
         complain_overflow_signed, /* complain_on_overflow.  */
         bfin_pcrel24_reloc,	/* special_function.  */
         "R_pcrel24_jump_x",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x00FFFFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_pcrel24,		/* type.  */
	 1,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 24,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_pcrel24_reloc,	/* special_function.  */
	 "R_pcrel24",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x00FFFFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_unusedb,		/* type.  */
	 0,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 32,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_unusedb",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_unusedc,		/* type.  */
	 0,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 32,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_unusedc",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_pcrel24_jump_l,	/* type.  */
	 1,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 24,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_pcrel24_reloc,	/* special_function.  */
	 "R_pcrel24_jump_l",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x00FFFFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_pcrel24_call_x,	/* type.  */
	 1,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 24,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_pcrel24_reloc,	/* special_function.  */
	 "R_pcrel24_call_x",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x00FFFFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_var_eq_symb,		/* type.  */
	 0,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 32,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_bitfield, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_var_eq_symb",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_byte_data,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 8,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_unsigned, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_byte_data",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xFF,			/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_byte2_data,		/* type.  */
	 0,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 16,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_signed, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_byte2_data",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xFFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_byte4_data,		/* type.  */
	 0,			/* rightshift.  */
	 2,			/* size (0 = byte, 1 = short, 2 = long).  */
	 32,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_unsigned, /* complain_on_overflow.  */
	 bfin_byte4_reloc,	/* special_function.  */
	 "R_byte4_data",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0xFFFFFFFF,		/* dst_mask.  */
	 TRUE),			/* pcrel_offset.  */

  HOWTO (R_pcrel11,		/* type.  */
	 1,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 10,			/* bitsize.  */
	 TRUE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_unsigned, /* complain_on_overflow.  */
	 bfin_bfd_reloc,	/* special_function.  */
	 "R_pcrel11",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0x000003FF,		/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */
};

static reloc_howto_type bfin_areloc_howto_table [] =
{
  HOWTO (R_push,
	 0,
	 2,
	 0,
	 FALSE,
	 0,
	 complain_overflow_dont,
	 bfin_push_reloc,
	 "R_expst_push",
	 FALSE,
	 0,
	 0,
	 FALSE),

  HOWTO (R_const,
	 0,
	 2,
	 0,
	 FALSE,
	 0,
	 complain_overflow_dont,
	 bfin_const_reloc,
	 "R_expst_const",
	 FALSE,
	 0,
	 0,
	 FALSE),

  HOWTO (R_add,
	 0,
	 0,
	 0,
	 FALSE,
	 0,
	 complain_overflow_dont,
	 bfin_oper_reloc,
	 "R_expst_add",
	 FALSE,
	 0,
	 0,
	 FALSE),

  HOWTO (R_sub,
	 0,
	 0,
	 0,
	 FALSE,
	 0,
	 complain_overflow_dont,
	 bfin_oper_reloc,
	 "R_expst_sub",
	 FALSE,
	 0,
	 0,
	 FALSE),

  HOWTO (R_mult,
	 0,
	 0,
	 0,
	 FALSE,
	 0,
	 complain_overflow_dont,
	 bfin_oper_reloc,
	 "R_expst_mult",
	 FALSE,
	 0,
	 0,
	 FALSE),

  HOWTO (R_div,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_div",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_mod,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_mod",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_lshift,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_lshift",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_rshift,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_rshift",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_and,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_and",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_or,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_or",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_xor,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_xor",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_land,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_land",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_lor,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_lor",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_len,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_len",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_neg,			/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_neg",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_comp,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_comp",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */
  
  HOWTO (R_page,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_page",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */
  
  HOWTO (R_hwpage,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_hwpage",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */
  
  HOWTO (R_addr,		/* type.  */
	 0,			/* rightshift.  */
	 0,			/* size (0 = byte, 1 = short, 2 = long).  */
	 0,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_dont, /* complain_on_overflow.  */
	 bfin_oper_reloc,	/* special_function.  */
	 "R_expst_addr",	/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0,			/* src_mask.  */
	 0,			/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */
};

static reloc_howto_type bfin_gnuext_howto_table [] =
{
  HOWTO (R_pltpc,		/* type.  */
	 0,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 16,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_bitfield, /* complain_on_overflow.  */
	 bfin_pltpc_reloc,	/* special_function.  */
	 "R_pltpc",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0xffff,		/* src_mask.  */
	 0xffff,		/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

  HOWTO (R_got,			/* type.  */
	 0,			/* rightshift.  */
	 1,			/* size (0 = byte, 1 = short, 2 = long).  */
	 16,			/* bitsize.  */
	 FALSE,			/* pc_relative.  */
	 0,			/* bitpos.  */
	 complain_overflow_bitfield, /* complain_on_overflow.  */
	 bfd_elf_generic_reloc,	/* special_function.  */
	 "R_got",		/* name.  */
	 FALSE,			/* partial_inplace.  */
	 0x7fff,		/* src_mask.  */
	 0x7fff,		/* dst_mask.  */
	 FALSE),		/* pcrel_offset.  */

/* GNU extension to record C++ vtable hierarchy.  */
  HOWTO (R_BFIN_GNU_VTINHERIT, /* type.  */
         0,                     /* rightshift.  */
         2,                     /* size (0 = byte, 1 = short, 2 = long).  */
         0,                     /* bitsize.  */
         FALSE,                 /* pc_relative.  */
         0,                     /* bitpos.  */
         complain_overflow_dont, /* complain_on_overflow.  */
         NULL,                  /* special_function.  */
         "R_BFIN_GNU_VTINHERIT", /* name.  */
         FALSE,                 /* partial_inplace.  */
         0,                     /* src_mask.  */
         0,                     /* dst_mask.  */
         FALSE),                /* pcrel_offset.  */

/* GNU extension to record C++ vtable member usage.  */
  HOWTO (R_BFIN_GNU_VTENTRY,	/* type.  */
         0,                     /* rightshift.  */
         2,                     /* size (0 = byte, 1 = short, 2 = long).  */
         0,                     /* bitsize.  */
         FALSE,                 /* pc_relative.  */
         0,			/* bitpos.  */
         complain_overflow_dont, /* complain_on_overflow.  */
         _bfd_elf_rel_vtable_reloc_fn, /* special_function.  */
         "R_BFIN_GNU_VTENTRY",	/* name.  */
         FALSE,                 /* partial_inplace.  */
         0,                     /* src_mask.  */
         0,                     /* dst_mask.  */
         FALSE)                 /* pcrel_offset.  */
};

struct bfin_reloc_map
{
  bfd_reloc_code_real_type 	bfd_reloc_val;
  unsigned int			bfin_reloc_val;
};

static const struct bfin_reloc_map bfin_reloc_map [] =
{
  { BFD_RELOC_NONE,			R_unused0 },
  { BFD_RELOC_BFIN_5_PCREL,		R_pcrel5m2 },
  { BFD_RELOC_NONE,			R_unused1 },
  { BFD_RELOC_BFIN_10_PCREL,		R_pcrel10 },
  { BFD_RELOC_BFIN_12_PCREL_JUMP,	R_pcrel12_jump },
  { BFD_RELOC_BFIN_16_IMM,		R_rimm16 },
  { BFD_RELOC_BFIN_16_LOW,		R_luimm16 },
  { BFD_RELOC_BFIN_16_HIGH,		R_huimm16 },
  { BFD_RELOC_BFIN_12_PCREL_JUMP_S,	R_pcrel12_jump_s },
  { BFD_RELOC_24_PCREL,			R_pcrel24 },
  { BFD_RELOC_24_PCREL,			R_pcrel24 },
  { BFD_RELOC_BFIN_24_PCREL_JUMP_L,	R_pcrel24_jump_l },
  { BFD_RELOC_NONE,			R_unusedb },
  { BFD_RELOC_NONE,			R_unusedc },
  { BFD_RELOC_BFIN_24_PCREL_CALL_X,	R_pcrel24_call_x },
  { BFD_RELOC_8,			R_byte_data },
  { BFD_RELOC_16,			R_byte2_data },
  { BFD_RELOC_32,			R_byte4_data },
  { BFD_RELOC_BFIN_11_PCREL,		R_pcrel11 },
  { BFD_RELOC_BFIN_GOT,			R_got },
  { BFD_RELOC_BFIN_PLTPC,		R_pltpc },
  { BFD_RELOC_VTABLE_INHERIT,		R_BFIN_GNU_VTINHERIT },
  { BFD_RELOC_VTABLE_ENTRY,		R_BFIN_GNU_VTENTRY },
  { BFD_ARELOC_BFIN_PUSH,		R_push },
  { BFD_ARELOC_BFIN_CONST,		R_const },
  { BFD_ARELOC_BFIN_ADD,		R_add },
  { BFD_ARELOC_BFIN_SUB,		R_sub },
  { BFD_ARELOC_BFIN_MULT,		R_mult },
  { BFD_ARELOC_BFIN_DIV,		R_div },
  { BFD_ARELOC_BFIN_MOD,		R_mod },
  { BFD_ARELOC_BFIN_LSHIFT,		R_lshift },
  { BFD_ARELOC_BFIN_RSHIFT,		R_rshift },
  { BFD_ARELOC_BFIN_AND,		R_and },
  { BFD_ARELOC_BFIN_OR,			R_or },
  { BFD_ARELOC_BFIN_XOR,		R_xor },
  { BFD_ARELOC_BFIN_LAND,		R_land },
  { BFD_ARELOC_BFIN_LOR,		R_lor },
  { BFD_ARELOC_BFIN_LEN,		R_len },
  { BFD_ARELOC_BFIN_NEG,		R_neg },
  { BFD_ARELOC_BFIN_COMP,		R_comp },
  { BFD_ARELOC_BFIN_PAGE,		R_page },
  { BFD_ARELOC_BFIN_HWPAGE,		R_hwpage },
  { BFD_ARELOC_BFIN_ADDR,		R_addr }

};


static void
bfin_info_to_howto (bfd *abfd ATTRIBUTE_UNUSED,
                    arelent *cache_ptr,
                    Elf_Internal_Rela *dst)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);

  if (r_type <= BFIN_RELOC_MAX)
    cache_ptr->howto = &bfin_howto_table [r_type];

  else if (r_type >= BFIN_ARELOC_MIN && r_type <= BFIN_ARELOC_MAX)
    cache_ptr->howto = &bfin_areloc_howto_table [r_type - BFIN_ARELOC_MIN];

  else if (r_type >= BFIN_GNUEXT_RELOC_MIN && r_type <= BFIN_GNUEXT_RELOC_MAX)
    cache_ptr->howto = &bfin_gnuext_howto_table [r_type - BFIN_GNUEXT_RELOC_MIN];

  else
    cache_ptr->howto = (reloc_howto_type *) NULL;

}
/* Given a BFD reloc type, return the howto.  */
static reloc_howto_type *
bfin_bfd_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED,
			    bfd_reloc_code_real_type code)
{
  unsigned int i;
  unsigned int r_type = BFIN_RELOC_MIN;

  for (i = sizeof (bfin_reloc_map) / sizeof (bfin_reloc_map[0]); --i;)
    if (bfin_reloc_map[i].bfd_reloc_val == code)
      r_type = bfin_reloc_map[i].bfin_reloc_val;

  if (r_type <= BFIN_RELOC_MAX && r_type > BFIN_RELOC_MIN)
    return &bfin_howto_table [r_type];

  else if (r_type >= BFIN_ARELOC_MIN && r_type <= BFIN_ARELOC_MAX)
   return &bfin_areloc_howto_table [r_type - BFIN_ARELOC_MIN];

  else if (r_type >= BFIN_GNUEXT_RELOC_MIN && r_type <= BFIN_GNUEXT_RELOC_MAX)
   return &bfin_gnuext_howto_table [r_type - BFIN_GNUEXT_RELOC_MIN];

  return (reloc_howto_type *) NULL;

}
/* Given a bfin relocation type, return the howto.  */
static reloc_howto_type *
bfin_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED,
			    unsigned int r_type)
{
  if (r_type <= BFIN_RELOC_MAX)
    return &bfin_howto_table [r_type];

  else if (r_type >= BFIN_ARELOC_MIN && r_type <= BFIN_ARELOC_MAX)
   return &bfin_areloc_howto_table [r_type - BFIN_ARELOC_MIN];

  else if (r_type >= BFIN_GNUEXT_RELOC_MIN && r_type <= BFIN_GNUEXT_RELOC_MAX)
   return &bfin_gnuext_howto_table [r_type - BFIN_GNUEXT_RELOC_MIN];

  return (reloc_howto_type *) NULL;

}

/* Return TRUE if the name is a local label.
   bfin local labels begin with L$.  */
static bfd_boolean
bfin_is_local_label_name (
     bfd *abfd ATTRIBUTE_UNUSED,
     const char *label)
{
  if (label[0] == 'L' && label[1] == '$' )
    return TRUE;

  return _bfd_elf_is_local_label_name (abfd, label);
}


/* Look through the relocs for a section during the first phase, and
   allocate space in the global offset table or procedure linkage
   table.  */

static bfd_boolean
bfin_check_relocs (bfd * abfd,
		   struct bfd_link_info *info,
		   asection *sec,
                   const Elf_Internal_Rela *relocs)
{
  bfd *dynobj;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_signed_vma *local_got_refcounts;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;
  asection *sgot;
  asection *srelgot;
  asection *sreloc;
  if (info->relocatable)
    return TRUE;

  dynobj = elf_hash_table (info)->dynobj;
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  local_got_refcounts = elf_local_got_refcounts (abfd);

  sgot = NULL;
  srelgot = NULL;
  sreloc = NULL;

  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;

      r_symndx = ELF32_R_SYM (rel->r_info);
      if (r_symndx < symtab_hdr->sh_info)
	h = NULL;
      else
	h = sym_hashes[r_symndx - symtab_hdr->sh_info];

      switch (ELF32_R_TYPE (rel->r_info))
	{
       /* This relocation describes the C++ object vtable hierarchy.
           Reconstruct it for later use during GC.  */
        case R_BFIN_GNU_VTINHERIT:
          if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
            return FALSE;
          break;

        /* This relocation describes which C++ vtable entries
           are actually used.  Record for later use during GC.  */
        case R_BFIN_GNU_VTENTRY:
          if (!bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
            return FALSE;
          break;

	case R_got:
	  if (h != NULL
	      && strcmp (h->root.root.string, "_GLOBAL_OFFSET_TABLE_") == 0)
	    break;
	  /* Fall through.  */

	  if (dynobj == NULL)
	    {
	      /* Create the .got section.  */
	      elf_hash_table (info)->dynobj = dynobj = abfd;
	      if (!_bfd_elf_create_got_section (dynobj, info))
		return FALSE;
	    }

	  if (sgot == NULL)
	    {
	      sgot = bfd_get_section_by_name (dynobj, ".got");
	      BFD_ASSERT (sgot != NULL);
	    }

	  if (srelgot == NULL && (h != NULL || info->shared))
	    {
	      srelgot = bfd_get_section_by_name (dynobj, ".rela.got");
	      if (srelgot == NULL)
		{
		  srelgot = bfd_make_section (dynobj, ".rela.got");
		  if (srelgot == NULL
		      || !bfd_set_section_flags (dynobj, srelgot,
						 (SEC_ALLOC
						  | SEC_LOAD
						  | SEC_HAS_CONTENTS
						  | SEC_IN_MEMORY
						  | SEC_LINKER_CREATED
						  | SEC_READONLY))
		      || !bfd_set_section_alignment (dynobj, srelgot, 2))
		    return FALSE;
		}
	    }

	  if (h != NULL)
	    {
	      if (h->got.refcount == 0)
		{
		  /* Make sure this symbol is output as a dynamic symbol.  */
		  if (h->dynindx == -1 && !h->forced_local)
		    {
		      if (!bfd_elf_link_record_dynamic_symbol (info, h))
			return FALSE;
		    }

		  /* Allocate space in the .got section.  */
		  sgot->size += 4;
		  /* Allocate relocation space.  */
		  srelgot->size += sizeof (Elf32_External_Rela);
		}
	      h->got.refcount++;
	    }
	  else
	    {
	      /* This is a global offset table entry for a local symbol.  */
	      if (local_got_refcounts == NULL)
		{
		  bfd_size_type size;

		  size = symtab_hdr->sh_info;
		  size *= sizeof (bfd_signed_vma);
		  local_got_refcounts = ((bfd_signed_vma *)
					 bfd_zalloc (abfd, size));
		  if (local_got_refcounts == NULL)
		    return FALSE;
		  elf_local_got_refcounts (abfd) = local_got_refcounts;
		}
	      if (local_got_refcounts[r_symndx] == 0)
		{
		  sgot->size += 4;
		  if (info->shared)
		    {
		      /* If we are generating a shared object, we need to
		         output a R_68K_RELATIVE reloc so that the dynamic
		         linker can adjust this GOT entry.  */
		      srelgot->size += sizeof (Elf32_External_Rela);
		    }
		}
	      local_got_refcounts[r_symndx]++;
	    }
	  break;

	default:
	  break;
	}
    }

  return TRUE;
}

static enum elf_reloc_type_class
elf32_bfin_reloc_type_class (const Elf_Internal_Rela * rela)
{
  switch ((int) ELF32_R_TYPE (rela->r_info))
    {
    default:
      return reloc_class_normal;
    }
}

static bfd_boolean
bfin_relocate_section (bfd * output_bfd,
		       struct bfd_link_info *info,
		       bfd * input_bfd,
		       asection * input_section,
		       bfd_byte * contents,
		       Elf_Internal_Rela * relocs,
		       Elf_Internal_Sym * local_syms,
		       asection ** local_sections)
{
  bfd *dynobj;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_vma *local_got_offsets;
  asection *sgot;
  asection *sreloc;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;
  int i = 0;

  if (info->relocatable)
    return TRUE;

  dynobj = elf_hash_table (info)->dynobj;
  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  local_got_offsets = elf_local_got_offsets (input_bfd);

  sgot = NULL;
  sreloc = NULL;

  rel = relocs;
  relend = relocs + input_section->reloc_count;
  for (; rel < relend; rel++, i++)
    {
      int r_type;
      reloc_howto_type *howto;
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;
      Elf_Internal_Sym *sym;
      asection *sec;
      bfd_vma relocation = 0;
      bfd_boolean unresolved_reloc;
      bfd_reloc_status_type r;
      bfd_vma address;

      r_type = ELF32_R_TYPE (rel->r_info);
      if (r_type < 0 || r_type >= 243)
	{
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}

      if (r_type == R_BFIN_GNU_VTENTRY
          || r_type == R_BFIN_GNU_VTINHERIT)
	continue;

      howto = bfin_reloc_type_lookup (input_bfd, r_type);
      if (howto == NULL)
	{
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}
      r_symndx = ELF32_R_SYM (rel->r_info);

      h = NULL;
      sym = NULL;
      sec = NULL;
      unresolved_reloc = FALSE;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);
	}
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];

	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  if (!
	      (!strcmp (h->root.root.string, ".__constant")
	       || !strcmp (h->root.root.string, ".__operator")))
	    {
	      bfd_boolean warned;
	      h = NULL;
	      RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				       r_symndx, symtab_hdr, sym_hashes,
				       h, sec, relocation,
				       unresolved_reloc, warned);

	    }
	}

      address = rel->r_offset;
      /* First, get stack relocs out of the way.  */
      switch (r_type)
	{
	case R_push:
	  reloc_stack_push (relocation + rel->r_addend);
	  r = bfd_reloc_ok;
	  goto done_reloc;
	case R_const:
	  reloc_stack_push (rel->r_addend);
	  r = bfd_reloc_ok;
	  goto done_reloc;
	case R_add:
	case R_sub:
	case R_mult:
	case R_div:
	case R_mod:
	case R_lshift:
	case R_rshift:
	case R_neg:
	case R_and:
	case R_or:
	case R_xor:
	case R_land:
	case R_lor:
	case R_comp:
	case R_page:
	case R_hwpage:
	  reloc_stack_operate (r_type);
	  r = bfd_reloc_ok;
	  goto done_reloc;

	default:
	  if (!is_reloc_stack_empty())
	    relocation = reloc_stack_pop ();
	  break;
	}

      /* Then, process normally.  */
      switch (r_type)
	{
	case R_BFIN_GNU_VTINHERIT:
	case R_BFIN_GNU_VTENTRY:
	  return bfd_reloc_ok;

	case R_got:
	  /* Relocation is to the address of the entry for this symbol
	     in the global offset table.  */
	  if (h != NULL
	      && strcmp (h->root.root.string, "_GLOBAL_OFFSET_TABLE_") == 0)
	    goto do_default;
	  /* Fall through.  */
	  /* Relocation is the offset of the entry for this symbol in
	     the global offset table.  */

	  {
	    bfd_vma off;

	    if (sgot == NULL)
	      {
		sgot = bfd_get_section_by_name (dynobj, ".got");
		BFD_ASSERT (sgot != NULL);
	      }

	    if (h != NULL)
	      {
		bfd_boolean dyn;

		off = h->got.offset;
		BFD_ASSERT (off != (bfd_vma) - 1);
		dyn = elf_hash_table (info)->dynamic_sections_created;

		if (!WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, h)
		    || (info->shared
			&& (info->symbolic
			    || h->dynindx == -1
			    || h->forced_local)
			&& h->def_regular))
		  {
		    /* This is actually a static link, or it is a
		       -Bsymbolic link and the symbol is defined
		       locally, or the symbol was forced to be local
		       because of a version file..  We must initialize
		       this entry in the global offset table.  Since
		       the offset must always be a multiple of 4, we
		       use the least significant bit to record whether
		       we have initialized it already.

		       When doing a dynamic link, we create a .rela.got
		       relocation entry to initialize the value.  This
		       is done in the finish_dynamic_symbol routine.  */
		    if ((off & 1) != 0)
		      off &= ~1;
		    else
		      {
			bfd_put_32 (output_bfd, relocation,
				    sgot->contents + off);
			h->got.offset |= 1;
		      }
		  }
		else
		  unresolved_reloc = FALSE;
	      }
	    else
	      {
		BFD_ASSERT (local_got_offsets != NULL);
		off = local_got_offsets[r_symndx];
		BFD_ASSERT (off != (bfd_vma) - 1);

		/* The offset must always be a multiple of 4.  We use
		   the least significant bit to record whether we have
		   already generated the necessary reloc.  */
		if ((off & 1) != 0)
		  off &= ~1;
		else
		  {
		    bfd_put_32 (output_bfd, relocation, sgot->contents + off);

		    if (info->shared)
		      {
			asection *s;
			Elf_Internal_Rela outrel;
			bfd_byte *loc;

			s = bfd_get_section_by_name (dynobj, ".rela.got");
			BFD_ASSERT (s != NULL);

			outrel.r_offset = (sgot->output_section->vma
					   + sgot->output_offset + off);
			outrel.r_info =
			  ELF32_R_INFO (0, R_pcrel24);
			outrel.r_addend = relocation;
			loc = s->contents;
			loc +=
			  s->reloc_count++ * sizeof (Elf32_External_Rela);
			bfd_elf32_swap_reloca_out (output_bfd, &outrel, loc);
		      }

		    local_got_offsets[r_symndx] |= 1;
		  }
	      }

	    relocation = sgot->output_offset + off;
	    rel->r_addend = 0;
            /* bfin : preg = [preg + 17bitdiv4offset] relocation is div by 4.  */
            relocation /= 4;
	  }
	  goto do_default;

	case R_pcrel24:
	case R_pcrel24_jump_l:
	  {
	    bfd_vma x;

	    relocation += rel->r_addend;

	    /* Perform usual pc-relative correction.  */
	    relocation -= input_section->output_section->vma + input_section->output_offset;
	    relocation -= address;

	    /* We are getting reloc_entry->address 2 byte off from
	       the start of instruction. Assuming absolute postion
	       of the reloc data. But, following code had been written assuming 
	       reloc address is starting at begining of instruction.
	       To compensate that I have increased the value of 
	       relocation by 1 (effectively 2) and used the addr -2 instead of addr.  */ 

	    relocation += 2;
	    address -= 2;

	    relocation >>= 1;

	    x = bfd_get_16 (input_bfd, contents + address);
	    x = (x & 0xff00) | ((relocation >> 16) & 0xff);
	    bfd_put_16 (input_bfd, x, contents + address);

	    x = bfd_get_16 (input_bfd, contents + address + 2);
	    x = relocation & 0xFFFF;
	    bfd_put_16 (input_bfd, x, contents + address + 2);
	    r = bfd_reloc_ok;
	  }
	  break;

	default:
	do_default:
	  r = _bfd_final_link_relocate (howto, input_bfd, input_section,
					contents, address,
					relocation, rel->r_addend);

	  break;
	}

    done_reloc:
      /* Dynamic relocs are not propagated for SEC_DEBUGGING sections
         because such sections are not SEC_ALLOC and thus ld.so will
         not process them.  */
      if (unresolved_reloc
	  && !((input_section->flags & SEC_DEBUGGING) != 0 && h->def_dynamic))
	{
	  (*_bfd_error_handler)
	    (_("%B(%A+0x%lx): unresolvable relocation against symbol `%s'"),
	     input_bfd,
	     input_section, (long) rel->r_offset, h->root.root.string);
	  return FALSE;
	}

      if (r != bfd_reloc_ok)
	{
	  const char *name;

	  if (h != NULL)
	    name = h->root.root.string;
	  else
	    {
	      name = bfd_elf_string_from_elf_section (input_bfd,
						      symtab_hdr->sh_link,
						      sym->st_name);
	      if (name == NULL)
		return FALSE;
	      if (*name == '\0')
		name = bfd_section_name (input_bfd, sec);
	    }

	  if (r == bfd_reloc_overflow)
	    {
	      if (!(info->callbacks->reloc_overflow
		    (info, (h ? &h->root : NULL), name, howto->name,
		     (bfd_vma) 0, input_bfd, input_section, rel->r_offset)))
		return FALSE;
	    }
	  else
	    {
	      (*_bfd_error_handler)
		(_("%B(%A+0x%lx): reloc against `%s': error %d"),
		 input_bfd, input_section,
		 (long) rel->r_offset, name, (int) r);
	      return FALSE;
	    }
	}
    }

  return TRUE;
}

static asection *
bfin_gc_mark_hook (asection * sec,
		   struct bfd_link_info *info ATTRIBUTE_UNUSED,
		   Elf_Internal_Rela * rel,
		   struct elf_link_hash_entry *h,
                   Elf_Internal_Sym * sym)
{
  if (h != NULL)
    {
      switch (ELF32_R_TYPE (rel->r_info))
	{

	case R_BFIN_GNU_VTINHERIT:
	case R_BFIN_GNU_VTENTRY:
	  break;

	default:
	  switch (h->root.type)
	    {
	    default:
	      break;

	    case bfd_link_hash_defined:
	    case bfd_link_hash_defweak:
	      return h->root.u.def.section;

	    case bfd_link_hash_common:
	      return h->root.u.c.p->section;
	    }
	}
    }
  else
    return bfd_section_from_elf_index (sec->owner, sym->st_shndx);

  return NULL;
}


/* Update the got entry reference counts for the section being removed.  */

static bfd_boolean
bfin_gc_sweep_hook (bfd * abfd,
		    struct bfd_link_info *info,
		    asection * sec,
                    const Elf_Internal_Rela * relocs)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_signed_vma *local_got_refcounts;
  const Elf_Internal_Rela *rel, *relend;
  bfd *dynobj;
  asection *sgot;
  asection *srelgot;

  dynobj = elf_hash_table (info)->dynobj;
  if (dynobj == NULL)
    return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  local_got_refcounts = elf_local_got_refcounts (abfd);

  sgot = bfd_get_section_by_name (dynobj, ".got");
  srelgot = bfd_get_section_by_name (dynobj, ".rela.got");

  relend = relocs + sec->reloc_count;
  for (rel = relocs; rel < relend; rel++)
    {
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;

      switch (ELF32_R_TYPE (rel->r_info))
	{
	case R_got:
	  r_symndx = ELF32_R_SYM (rel->r_info);
	  if (r_symndx >= symtab_hdr->sh_info)
	    {
	      h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	      if (h->got.refcount > 0)
		{
		  --h->got.refcount;
		  if (h->got.refcount == 0)
		    {
		      /* We don't need the .got entry any more.  */
		      sgot->size -= 4;
		      srelgot->size -= sizeof (Elf32_External_Rela);
		    }
		}
	    }
	  else if (local_got_refcounts != NULL)
	    {
	      if (local_got_refcounts[r_symndx] > 0)
		{
		  --local_got_refcounts[r_symndx];
		  if (local_got_refcounts[r_symndx] == 0)
		    {
		      /* We don't need the .got entry any more.  */
		      sgot->size -= 4;
		      if (info->shared)
			srelgot->size -= sizeof (Elf32_External_Rela);
		    }
		}
	    }
	  break;
	default:
	  break;
	}
    }

  return TRUE;
}


/* Merge backend specific data from an object file to the output
   object file when linking.  */
static bfd_boolean
elf32_bfin_merge_private_bfd_data (bfd * ibfd, bfd * obfd)
{
  flagword out_flags;
  flagword in_flags;

  if (bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  in_flags = elf_elfheader (ibfd)->e_flags;
  out_flags = elf_elfheader (obfd)->e_flags;

  if (!elf_flags_init (obfd))
    {
      elf_flags_init (obfd) = TRUE;
      elf_elfheader (obfd)->e_flags = in_flags;
    }

  return TRUE;
}


static bfd_boolean
elf32_bfin_set_private_flags (bfd * abfd, flagword flags)
{
  elf_elfheader (abfd)->e_flags = flags;
  elf_flags_init (abfd) = TRUE;
  return TRUE;
}


/* Display the flags field.  */
static bfd_boolean
elf32_bfin_print_private_bfd_data (bfd * abfd, PTR ptr)
{
  FILE *file = (FILE *) ptr;

  BFD_ASSERT (abfd != NULL && ptr != NULL);

  /* Print normal ELF private data.  */
  _bfd_elf_print_private_bfd_data (abfd, ptr);

  /* Ignore init flag - it may not be set, despite the flags field
     containing valid data.  */

  /* xgettext:c-format */
  fprintf (file, _("private flags = %lx:"), elf_elfheader (abfd)->e_flags);

  fputc ('\n', file);

  return TRUE;
}

/* bfin ELF linker hash entry.  */

struct bfin_link_hash_entry
{
  struct elf_link_hash_entry root;

  /* Number of PC relative relocs copied for this symbol.  */
  struct bfin_pcrel_relocs_copied *pcrel_relocs_copied;
};

/* bfin ELF linker hash table.  */

struct bfin_link_hash_table
{
  struct elf_link_hash_table root;

  /* Small local sym to section mapping cache.  */
  struct sym_sec_cache sym_sec;
};

#define bfin_hash_entry(ent) ((struct bfin_link_hash_entry *) (ent))

static struct bfd_hash_entry *
bfin_link_hash_newfunc (struct bfd_hash_entry *entry,
			    struct bfd_hash_table *table, const char *string)
{
  struct bfd_hash_entry *ret = entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == NULL)
    ret = bfd_hash_allocate (table, sizeof (struct bfin_link_hash_entry));
  if (ret == NULL)
    return ret;

  /* Call the allocation method of the superclass.  */
  ret = _bfd_elf_link_hash_newfunc (ret, table, string);
  if (ret != NULL)
    bfin_hash_entry (ret)->pcrel_relocs_copied = NULL;

  return ret;
}

/* Create an bfin ELF linker hash table.  */

static struct bfd_link_hash_table *
bfin_link_hash_table_create (bfd * abfd)
{
  struct bfin_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct bfin_link_hash_table);

  ret = (struct bfin_link_hash_table *) bfd_malloc (amt);
  if (ret == (struct bfin_link_hash_table *) NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (&ret->root, abfd,
				      bfin_link_hash_newfunc))
    {
      free (ret);
      return NULL;
    }

  ret->sym_sec.abfd = NULL;

  return &ret->root.root;
}

/* The size in bytes of an entry in the procedure linkage table.  */

/* Finish up the dynamic sections.  */

static bfd_boolean
bfin_finish_dynamic_sections (bfd * output_bfd ATTRIBUTE_UNUSED,
				  struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *sdyn;

  dynobj = elf_hash_table (info)->dynobj;

  sdyn = bfd_get_section_by_name (dynobj, ".dynamic");

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      Elf32_External_Dyn *dyncon, *dynconend;

      BFD_ASSERT (sdyn != NULL);

      dyncon = (Elf32_External_Dyn *) sdyn->contents;
      dynconend = (Elf32_External_Dyn *) (sdyn->contents + sdyn->size);
      for (; dyncon < dynconend; dyncon++)
	{
	  Elf_Internal_Dyn dyn;

	  bfd_elf32_swap_dyn_in (dynobj, dyncon, &dyn);

	}

    }
  return TRUE;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

static bfd_boolean
bfin_finish_dynamic_symbol (bfd * output_bfd,
				struct bfd_link_info *info,
				struct elf_link_hash_entry *h,
				Elf_Internal_Sym * sym)
{
  bfd *dynobj;

  dynobj = elf_hash_table (info)->dynobj;

  if (h->got.offset != (bfd_vma) - 1)
    {
      asection *sgot;
      asection *srela;
      Elf_Internal_Rela rela;
      bfd_byte *loc;

      /* This symbol has an entry in the global offset table.
         Set it up.  */

      sgot = bfd_get_section_by_name (dynobj, ".got");
      srela = bfd_get_section_by_name (dynobj, ".rela.got");
      BFD_ASSERT (sgot != NULL && srela != NULL);

      rela.r_offset = (sgot->output_section->vma
		       + sgot->output_offset
		       + (h->got.offset & ~(bfd_vma) 1));

      /* If this is a -Bsymbolic link, and the symbol is defined
         locally, we just want to emit a RELATIVE reloc.  Likewise if
         the symbol was forced to be local because of a version file.
         The entry in the global offset table will already have been
         initialized in the relocate_section function.  */
      if (info->shared
	  && (info->symbolic
	      || h->dynindx == -1 || h->forced_local) && h->def_regular)
	{
fprintf(stderr, "*** check this relocation %s\n", __FUNCTION__);
	  rela.r_info = ELF32_R_INFO (0, R_pcrel24);
	  rela.r_addend = bfd_get_signed_32 (output_bfd,
					     (sgot->contents
					      +
					      (h->got.
					       offset & ~(bfd_vma) 1)));
	}
      else
	{
	  bfd_put_32 (output_bfd, (bfd_vma) 0,
		      sgot->contents + (h->got.offset & ~(bfd_vma) 1));
	  rela.r_info = ELF32_R_INFO (h->dynindx, R_got);
	  rela.r_addend = 0;
	}

      loc = srela->contents;
      loc += srela->reloc_count++ * sizeof (Elf32_External_Rela);
      bfd_elf32_swap_reloca_out (output_bfd, &rela, loc);
    }

  if (h->needs_copy)
    {
      BFD_ASSERT (0);
    }
  /* Mark _DYNAMIC and _GLOBAL_OFFSET_TABLE_ as absolute.  */
  if (strcmp (h->root.root.string, "_DYNAMIC") == 0
      || strcmp (h->root.root.string, "_GLOBAL_OFFSET_TABLE_") == 0)
    sym->st_shndx = SHN_ABS;

  return TRUE;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

static bfd_boolean
bfin_adjust_dynamic_symbol (struct bfd_link_info *info,
				struct elf_link_hash_entry *h)
{
  bfd *dynobj;
  asection *s;
  unsigned int power_of_two;

  dynobj = elf_hash_table (info)->dynobj;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt
		  || h->u.weakdef != NULL
		  || (h->def_dynamic && h->ref_regular && !h->def_regular)));

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later,
     when we know the address of the .got section.  */
  if (h->type == STT_FUNC || h->needs_plt)
    {
      BFD_ASSERT(0);
    }

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->u.weakdef != NULL)
    {
      BFD_ASSERT (h->u.weakdef->root.type == bfd_link_hash_defined
		  || h->u.weakdef->root.type == bfd_link_hash_defweak);
      h->root.u.def.section = h->u.weakdef->root.u.def.section;
      h->root.u.def.value = h->u.weakdef->root.u.def.value;
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (info->shared)
    return TRUE;

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */

  s = bfd_get_section_by_name (dynobj, ".dynbss");
  BFD_ASSERT (s != NULL);

  /* We must generate a R_68K_COPY reloc to tell the dynamic linker to
     copy the initial value out of the dynamic object and into the
     runtime process image.  We need to remember the offset into the
     .rela.bss section we are going to use.  */
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0)
    {
      asection *srel;

      srel = bfd_get_section_by_name (dynobj, ".rela.bss");
      BFD_ASSERT (srel != NULL);
      srel->size += sizeof (Elf32_External_Rela);
      h->needs_copy = 1;
    }

  /* We need to figure out the alignment required for this symbol.  I
     have no idea how ELF linkers handle this.  */
  power_of_two = bfd_log2 (h->size);
  if (power_of_two > 3)
    power_of_two = 3;

  /* Apply the required alignment.  */
  s->size = BFD_ALIGN (s->size, (bfd_size_type) (1 << power_of_two));
  if (power_of_two > bfd_get_section_alignment (dynobj, s))
    {
      if (!bfd_set_section_alignment (dynobj, s, power_of_two))
	return FALSE;
    }

  /* Define the symbol as being at this point in the section.  */
  h->root.u.def.section = s;
  h->root.u.def.value = s->size;

  /* Increment the section size to make room for the symbol.  */
  s->size += h->size;

  return TRUE;
}

/* The bfin linker needs to keep track of the number of relocs that it
   decides to copy in check_relocs for each symbol.  This is so that it
   can discard PC relative relocs if it doesn't need them when linking
   with -Bsymbolic.  We store the information in a field extending the
   regular ELF linker hash table.  */

/* This structure keeps track of the number of PC relative relocs we have
   copied for a given symbol.  */

struct bfin_pcrel_relocs_copied
{
  /* Next section.  */
  struct bfin_pcrel_relocs_copied *next;
  /* A section in dynobj.  */
  asection *section;
  /* Number of relocs copied in this section.  */
  bfd_size_type count;
};

/* This function is called via elf_link_hash_traverse if we are
   creating a shared object.  In the -Bsymbolic case it discards the
   space allocated to copy PC relative relocs against symbols which
   are defined in regular objects.  For the normal shared case, it
   discards space for pc-relative relocs that have become local due to
   symbol visibility changes.  We allocated space for them in the
   check_relocs routine, but we won't fill them in in the
   relocate_section routine.

   We also check whether any of the remaining relocations apply
   against a readonly section, and set the DF_TEXTREL flag in this
   case.  */

static bfd_boolean
bfin_discard_copies (struct elf_link_hash_entry *h, PTR inf)
{
  struct bfd_link_info *info = (struct bfd_link_info *) inf;
  struct bfin_pcrel_relocs_copied *s;

  if (h->root.type == bfd_link_hash_warning)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  if (!h->def_regular || (!info->symbolic && !h->forced_local))
    {
      if ((info->flags & DF_TEXTREL) == 0)
	{
	  /* Look for relocations against read-only sections.  */
	  for (s = bfin_hash_entry (h)->pcrel_relocs_copied;
	       s != NULL; s = s->next)
	    if ((s->section->flags & SEC_READONLY) != 0)
	      {
		info->flags |= DF_TEXTREL;
		break;
	      }
	}

      return TRUE;
    }

  for (s = bfin_hash_entry (h)->pcrel_relocs_copied;
       s != NULL; s = s->next)
    s->section->size -= s->count * sizeof (Elf32_External_Rela);

  return TRUE;
}

/* Set the sizes of the dynamic sections.  */
#define ELF_DYNAMIC_INTERPRETER "/usr/lib/libc.so.1"

static bfd_boolean
bfin_size_dynamic_sections (bfd * output_bfd ATTRIBUTE_UNUSED,
				struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *s;
  bfd_boolean relocs;

  dynobj = elf_hash_table (info)->dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Set the contents of the .interp section to the interpreter.  */
      if (info->executable)
	{
	  s = bfd_get_section_by_name (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  s->size = sizeof ELF_DYNAMIC_INTERPRETER;
	  s->contents = (unsigned char *) ELF_DYNAMIC_INTERPRETER;
	}
    }
  else
    {
      /* We may have created entries in the .rela.got section.
         However, if we are not creating the dynamic sections, we will
         not actually use these entries.  Reset the size of .rela.got,
         which will cause it to get stripped from the output file
         below.  */
      s = bfd_get_section_by_name (dynobj, ".rela.got");
      if (s != NULL)
	s->size = 0;
    }

  /* If this is a -Bsymbolic shared link, then we need to discard all
     PC relative relocs against symbols defined in a regular object.
     For the normal shared case we discard the PC relative relocs
     against symbols that have become local due to visibility changes.
     We allocated space for them in the check_relocs routine, but we
     will not fill them in in the relocate_section routine.  */
  if (info->shared)
    elf_link_hash_traverse (elf_hash_table (info),
			    bfin_discard_copies, (PTR) info);

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  relocs = FALSE;
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      const char *name;
      bfd_boolean strip;

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      /* It's OK to base decisions on the section name, because none
         of the dynobj section names depend upon the input files.  */
      name = bfd_get_section_name (dynobj, s);

      strip = FALSE;

       if (strncmp (name, ".rela", 5) == 0)
	{
	  if (s->size == 0)
	    {
	      /* If we don't need this section, strip it from the
	         output file.  This is mostly to handle .rela.bss and
	         .rela.plt.  We must create both sections in
	         create_dynamic_sections, because they must be created
	         before the linker maps input sections to output
	         sections.  The linker does that before
	         adjust_dynamic_symbol is called, and it is that
	         function which decides whether anything needs to go
	         into these sections.  */
	      strip = TRUE;
	    }
	  else
	    {
	      relocs = TRUE;

	      /* We use the reloc_count field as a counter if we need
	         to copy relocs into the output file.  */
	      s->reloc_count = 0;
	    }
	}
      else if (strncmp (name, ".got", 4) != 0)
	{
	  /* It's not one of our sections, so don't allocate space.  */
	  continue;
	}

      if (strip)
	{
	  s->flags |= SEC_EXCLUDE;
	  continue;
	}

      /* Allocate memory for the section contents.  */
      /* FIXME: This should be a call to bfd_alloc not bfd_zalloc.
         Unused entries should be reclaimed before the section's contents
         are written out, but at the moment this does not happen.  Thus in
         order to prevent writing out garbage, we initialise the section's
         contents to zero.  */
      s->contents = (bfd_byte *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL && s->size != 0)
	return FALSE;
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
         values later, in bfin_finish_dynamic_sections, but we
         must add the entries now so that we get the correct size for
         the .dynamic section.  The DT_DEBUG entry is filled in by the
         dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) \
  _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (!info->shared)
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return FALSE;
	}


      if (relocs)
	{
	  if (!add_dynamic_entry (DT_RELA, 0)
	      || !add_dynamic_entry (DT_RELASZ, 0)
	      || !add_dynamic_entry (DT_RELAENT,
				     sizeof (Elf32_External_Rela)))
	    return FALSE;
	}

      if ((info->flags & DF_TEXTREL) != 0)
	{
	  if (!add_dynamic_entry (DT_TEXTREL, 0))
	    return FALSE;
	}
    }
#undef add_dynamic_entry

  return TRUE;
}

/* Given a .data section and a .emreloc in-memory section, store
   relocation information into the .emreloc section which can be
   used at runtime to relocate the section.  This is called by the
   linker when the --embedded-relocs switch is used.  This is called
   after the add_symbols entry point has been called for all the
   objects, and before the final_link entry point is called.  */

bfd_boolean
bfd_bfin_elf32_create_embedded_relocs (
     bfd *abfd,
     struct bfd_link_info *info,
     asection *datasec,
     asection *relsec,
     char **errmsg)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Sym *isymbuf = NULL;
  Elf_Internal_Rela *internal_relocs = NULL;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *p;
  bfd_size_type amt;

  BFD_ASSERT (! info->relocatable);

  *errmsg = NULL;

  if (datasec->reloc_count == 0)
    return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;

  /* Get a copy of the native relocations.  */
  internal_relocs = (_bfd_elf_link_read_relocs
		     (abfd, datasec, (PTR) NULL, (Elf_Internal_Rela *) NULL,
		      info->keep_memory));
  if (internal_relocs == NULL)
    goto error_return;

  amt = (bfd_size_type) datasec->reloc_count * 12;
  relsec->contents = (bfd_byte *) bfd_alloc (abfd, amt);
  if (relsec->contents == NULL)
    goto error_return;

  p = relsec->contents;

  irelend = internal_relocs + datasec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++, p += 12)
    {
      asection *targetsec;

      /* We are going to write a four byte longword into the runtime
       reloc section.  The longword will be the address in the data
       section which must be relocated.  It is followed by the name
       of the target section NUL-padded or truncated to 8
       characters.  */

      /* We can only relocate absolute longword relocs at run time.  */
      if (ELF32_R_TYPE (irel->r_info) != (int) R_byte4_data)
	{
	  *errmsg = _("unsupported reloc type");
	  bfd_set_error (bfd_error_bad_value);
	  goto error_return;
	}

      /* Get the target section referred to by the reloc.  */
      if (ELF32_R_SYM (irel->r_info) < symtab_hdr->sh_info)
	{
	  /* A local symbol.  */
	  Elf_Internal_Sym *isym;

	  /* Read this BFD's local symbols if we haven't done so already.  */
	  if (isymbuf == NULL)
	    {
	      isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	      if (isymbuf == NULL)
		isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
						symtab_hdr->sh_info, 0,
						NULL, NULL, NULL);
	      if (isymbuf == NULL)
		goto error_return;
	    }

	  isym = isymbuf + ELF32_R_SYM (irel->r_info);
	  targetsec = bfd_section_from_elf_index (abfd, isym->st_shndx);
	}
      else
	{
	  unsigned long indx;
	  struct elf_link_hash_entry *h;

	  /* An external symbol.  */
	  indx = ELF32_R_SYM (irel->r_info) - symtab_hdr->sh_info;
	  h = elf_sym_hashes (abfd)[indx];
	  BFD_ASSERT (h != NULL);
	  if (h->root.type == bfd_link_hash_defined
	      || h->root.type == bfd_link_hash_defweak)
	    targetsec = h->root.u.def.section;
	  else
	    targetsec = NULL;
	}

      bfd_put_32 (abfd, irel->r_offset + datasec->output_offset, p);
      memset (p + 4, 0, 8);
      if (targetsec != NULL)
	strncpy ((char *) p + 4, targetsec->output_section->name, 8);
    }

  if (isymbuf != NULL && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (internal_relocs != NULL
      && elf_section_data (datasec)->relocs != internal_relocs)
    free (internal_relocs);
  return TRUE;

error_return:
  if (isymbuf != NULL && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (internal_relocs != NULL
      && elf_section_data (datasec)->relocs != internal_relocs)
    free (internal_relocs);
  return FALSE;
}

#define TARGET_LITTLE_SYM		bfd_elf32_bfin_vec
#define TARGET_LITTLE_NAME		"elf32-bfin"
#define ELF_ARCH			bfd_arch_bfin
#define ELF_MACHINE_CODE		EM_BLACKFIN	
#define ELF_MAXPAGESIZE			0x1000
#define elf_symbol_leading_char		'_'

#define bfd_elf32_bfd_reloc_type_lookup	bfin_bfd_reloc_type_lookup
#define elf_info_to_howto		bfin_info_to_howto
#define elf_info_to_howto_rel		0

#define bfd_elf32_bfd_is_local_label_name \
                                        bfin_is_local_label_name
#define bfin_hash_table(p) \
  ((struct bfin_link_hash_table *) (p)->hash)



#define elf_backend_create_dynamic_sections \
                                        _bfd_elf_create_dynamic_sections
#define bfd_elf32_bfd_link_hash_table_create \
                                        bfin_link_hash_table_create
#define bfd_elf32_bfd_final_link        bfd_elf_gc_common_final_link

#define elf_backend_check_relocs   bfin_check_relocs
#define elf_backend_adjust_dynamic_symbol \
                                        bfin_adjust_dynamic_symbol
#define elf_backend_size_dynamic_sections \
                                        bfin_size_dynamic_sections
#define elf_backend_relocate_section    bfin_relocate_section
#define elf_backend_finish_dynamic_symbol \
                                        bfin_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections \
                                        bfin_finish_dynamic_sections
#define elf_backend_gc_mark_hook        bfin_gc_mark_hook
#define elf_backend_gc_sweep_hook       bfin_gc_sweep_hook
#define bfd_elf32_bfd_merge_private_bfd_data \
                                        elf32_bfin_merge_private_bfd_data
#define bfd_elf32_bfd_set_private_flags \
                                        elf32_bfin_set_private_flags
#define bfd_elf32_bfd_print_private_bfd_data \
                                        elf32_bfin_print_private_bfd_data
#define elf_backend_reloc_type_class    elf32_bfin_reloc_type_class
#define elf_backend_can_gc_sections 1
#define elf_backend_can_refcount 1
#define elf_backend_want_got_plt 0
#define elf_backend_plt_readonly 1
#define elf_backend_want_plt_sym 0
#define elf_backend_got_header_size     12
#define elf_backend_rela_normal         1


#include "elf32-target.h"
