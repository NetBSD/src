/* Ubicom IP2xxx specific support for 32-bit ELF
   Copyright 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/ip2k.h"

/* Struct used to pass miscellaneous paramaters which
   helps to avoid overly long parameter lists.  */
struct misc
{
  Elf_Internal_Shdr *  symtab_hdr;
  Elf_Internal_Rela *  irelbase;
  bfd_byte *           contents;
  Elf_Internal_Sym *   isymbuf;
};

/* Prototypes.  */
static reloc_howto_type *    ip2k_reloc_type_lookup               PARAMS ((bfd *, bfd_reloc_code_real_type));
static void                  ip2k_info_to_howto_rela              PARAMS ((bfd *, arelent *, Elf32_Internal_Rela *));
static asection *            ip2k_elf_gc_mark_hook                PARAMS ((asection *, struct bfd_link_info *, Elf_Internal_Rela *, struct elf_link_hash_entry *, Elf_Internal_Sym *));
static boolean               ip2k_elf_gc_sweep_hook               PARAMS ((bfd *, struct bfd_link_info *, asection *, const Elf_Internal_Rela *));
static bfd_vma               symbol_value                         PARAMS ((bfd *, Elf_Internal_Shdr *, Elf32_Internal_Sym *, Elf_Internal_Rela *));
static void                  adjust_all_relocations               PARAMS ((bfd *, asection *, bfd_vma, bfd_vma, int, int));
static boolean               ip2k_elf_relax_delete_bytes          PARAMS ((bfd *, asection *, bfd_vma, int));
static boolean               ip2k_elf_relax_add_bytes             PARAMS ((bfd *, asection *, bfd_vma, const bfd_byte *, int, int));
static boolean               add_page_insn                        PARAMS ((bfd *, asection *, Elf_Internal_Rela *, struct misc *));
static boolean               ip2k_elf_relax_section               PARAMS ((bfd *, asection *, struct bfd_link_info *, boolean *));
static boolean               relax_switch_dispatch_tables_pass1   PARAMS ((bfd *, asection *, bfd_vma, struct misc *));
static boolean               unrelax_dispatch_table_entries       PARAMS ((bfd *, asection *, bfd_vma, bfd_vma, boolean *, struct misc *));
static boolean               unrelax_switch_dispatch_tables_passN PARAMS ((bfd *, asection *, bfd_vma, boolean *, struct misc *));
static boolean               is_switch_128_dispatch_table_p       PARAMS ((bfd *, bfd_vma, boolean, struct misc *));
static boolean               is_switch_256_dispatch_table_p       PARAMS ((bfd *, bfd_vma, boolean, struct misc *));
static boolean               ip2k_elf_relax_section_pass1         PARAMS ((bfd *, asection *, boolean *, struct misc *));
static boolean               ip2k_elf_relax_section_passN         PARAMS ((bfd *, asection *, boolean *, boolean *, struct misc *));
static bfd_reloc_status_type ip2k_final_link_relocate             PARAMS ((reloc_howto_type *, bfd *, asection *, bfd_byte *, Elf_Internal_Rela *, bfd_vma));
static boolean               ip2k_elf_relocate_section            PARAMS ((bfd *, struct bfd_link_info *, bfd *, asection *, bfd_byte *, Elf_Internal_Rela *, Elf_Internal_Sym *, asection **));

#define IS_OPCODE(CODE0,CODE1,OPCODE) \
  ((CODE0) == (OPCODE)[0] && (CODE1) == (OPCODE)[1])

#define PAGE_INSN_0		0x00
#define PAGE_INSN_1		0x10

static const bfd_byte page_opcode[] =
{
   PAGE_INSN_0, PAGE_INSN_1
};

#define IS_PAGE_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, page_opcode)

#define JMP_INSN_0		0xE0
#define JMP_INSN_1		0x00

static const bfd_byte jmp_opcode[] =
{
   JMP_INSN_0, JMP_INSN_1
};

#define IS_JMP_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, jmp_opcode)

#define CALL_INSN_0		0xC0
#define CALL_INSN_1		0x00

static const bfd_byte call_opcode[] =
{
  CALL_INSN_0, CALL_INSN_1
};

#define IS_CALL_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, call_opcode)

#define ADD_PCL_W_INSN_0	0x1E
#define ADD_PCL_W_INSN_1	0x09

static const bfd_byte add_pcl_w_opcode[] =
{
  ADD_PCL_W_INSN_0, ADD_PCL_W_INSN_1
};

#define IS_ADD_PCL_W_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, add_pcl_w_opcode)

#define ADD_W_WREG_INSN_0	0x1C
#define ADD_W_WREG_INSN_1	0x0A

static const bfd_byte add_w_wreg_opcode[] =
{
  ADD_W_WREG_INSN_0, ADD_W_WREG_INSN_1
};

#define IS_ADD_W_WREG_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, add_w_wreg_opcode)

#define SNC_INSN_0		0xA0
#define SNC_INSN_1		0x0B

static const bfd_byte snc_opcode[] =
{
   SNC_INSN_0, SNC_INSN_1
};

#define IS_SNC_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, snc_opcode)

#define INC_1_SP_INSN_0		0x2B
#define INC_1_SP_INSN_1		0x81

static const bfd_byte inc_1_sp_opcode[] =
{
   INC_1_SP_INSN_0, INC_1_SP_INSN_1
};

#define IS_INC_1_SP_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, inc_1_sp_opcode)

#define ADD_2_SP_W_INSN_0	0x1F
#define ADD_2_SP_W_INSN_1	0x82

static const bfd_byte add_2_sp_w_opcode[] =
{
   ADD_2_SP_W_INSN_0, ADD_2_SP_W_INSN_1
};

#define IS_ADD_2_SP_W_OPCODE(CODE0,CODE1) \
  IS_OPCODE (CODE0, CODE1, add_2_sp_w_opcode)

/* Relocation tables. */
static reloc_howto_type ip2k_elf_howto_table [] =
{
#define IP2K_HOWTO(t,rs,s,bs,pr,bp,name,sm,dm) \
    HOWTO(t,                    /* type */ \
          rs,                   /* rightshift */ \
          s,                    /* size (0 = byte, 1 = short, 2 = long) */ \
          bs,                   /* bitsize */ \
          pr,                   /* pc_relative */ \
          bp,                   /* bitpos */ \
          complain_overflow_dont,/* complain_on_overflow */ \
          bfd_elf_generic_reloc,/* special_function */ \
          name,                 /* name */ \
          false,                /* partial_inplace */ \
          sm,                   /* src_mask */ \
          dm,                   /* dst_mask */ \
          pr)                   /* pcrel_offset */

  /* This reloc does nothing. */
  IP2K_HOWTO (R_IP2K_NONE, 0,2,32, false, 0, "R_IP2K_NONE", 0, 0), 
  /* A 16 bit absolute relocation.  */
  IP2K_HOWTO (R_IP2K_16, 0,1,16, false, 0, "R_IP2K_16", 0, 0xffff),
  /* A 32 bit absolute relocation.  */
  IP2K_HOWTO (R_IP2K_32, 0,2,32, false, 0, "R_IP2K_32", 0, 0xffffffff),
  /* A 8-bit data relocation for the FR9 field.  Ninth bit is computed specially.  */
  IP2K_HOWTO (R_IP2K_FR9, 0,1,9, false, 0, "R_IP2K_FR9", 0, 0x00ff),
  /* A 4-bit data relocation.  */
  IP2K_HOWTO (R_IP2K_BANK, 8,1,4, false, 0, "R_IP2K_BANK", 0, 0x000f),
  /* A 13-bit insn relocation - word address => right-shift 1 bit extra.  */
  IP2K_HOWTO (R_IP2K_ADDR16CJP, 1,1,13, false, 0, "R_IP2K_ADDR16CJP", 0, 0x1fff),
  /* A 3-bit insn relocation - word address => right-shift 1 bit extra.  */
  IP2K_HOWTO (R_IP2K_PAGE3, 14,1,3, false, 0, "R_IP2K_PAGE3", 0, 0x0007),
  /* Two 8-bit data relocations.  */
  IP2K_HOWTO (R_IP2K_LO8DATA, 0,1,8, false, 0, "R_IP2K_LO8DATA", 0, 0x00ff),
  IP2K_HOWTO (R_IP2K_HI8DATA, 8,1,8, false, 0, "R_IP2K_HI8DATA", 0, 0x00ff),
  /* Two 8-bit insn relocations.  word address => right-shift 1 bit extra.  */
  IP2K_HOWTO (R_IP2K_LO8INSN, 1,1,8, false, 0, "R_IP2K_LO8INSN", 0, 0x00ff),
  IP2K_HOWTO (R_IP2K_HI8INSN, 9,1,8, false, 0, "R_IP2K_HI8INSN", 0, 0x00ff),

  /* Special 1 bit relocation for SKIP instructions.  */
  IP2K_HOWTO (R_IP2K_PC_SKIP, 1,1,1, false, 12, "R_IP2K_PC_SKIP", 0xfffe, 0x1000),
  /* 16 bit word address.  */
  IP2K_HOWTO (R_IP2K_TEXT, 1,1,16, false, 0, "R_IP2K_TEXT", 0, 0xffff),
  /* A 7-bit offset relocation for the FR9 field.  Eigth and ninth bit comes from insn.  */
  IP2K_HOWTO (R_IP2K_FR_OFFSET, 0,1,9, false, 0, "R_IP2K_FR_OFFSET", 0x180, 0x007f),
  /* Bits 23:16 of an address.  */
  IP2K_HOWTO (R_IP2K_EX8DATA, 16,1,8, false, 0, "R_IP2K_EX8DATA", 0, 0x00ff),
};


/* Map BFD reloc types to IP2K ELF reloc types. */
static reloc_howto_type *
ip2k_reloc_type_lookup (abfd, code)
     bfd * abfd ATTRIBUTE_UNUSED;
     bfd_reloc_code_real_type code;
{
  /* Note that the ip2k_elf_howto_table is indxed by the R_
     constants.  Thus, the order that the howto records appear in the
     table *must* match the order of the relocation types defined in
     include/elf/ip2k.h. */

  switch (code)
    {
    case BFD_RELOC_NONE:
      return &ip2k_elf_howto_table[ (int) R_IP2K_NONE];
    case BFD_RELOC_16:
      return &ip2k_elf_howto_table[ (int) R_IP2K_16];
    case BFD_RELOC_32:
      return &ip2k_elf_howto_table[ (int) R_IP2K_32];
    case BFD_RELOC_IP2K_FR9:
      return &ip2k_elf_howto_table[ (int) R_IP2K_FR9];
    case BFD_RELOC_IP2K_BANK:
      return &ip2k_elf_howto_table[ (int) R_IP2K_BANK];
    case BFD_RELOC_IP2K_ADDR16CJP:
      return &ip2k_elf_howto_table[ (int) R_IP2K_ADDR16CJP];
    case BFD_RELOC_IP2K_PAGE3:
      return &ip2k_elf_howto_table[ (int) R_IP2K_PAGE3];
    case BFD_RELOC_IP2K_LO8DATA:
      return &ip2k_elf_howto_table[ (int) R_IP2K_LO8DATA];
    case BFD_RELOC_IP2K_HI8DATA:
      return &ip2k_elf_howto_table[ (int) R_IP2K_HI8DATA];
    case BFD_RELOC_IP2K_LO8INSN:
      return &ip2k_elf_howto_table[ (int) R_IP2K_LO8INSN];
    case BFD_RELOC_IP2K_HI8INSN:
      return &ip2k_elf_howto_table[ (int) R_IP2K_HI8INSN];
    case BFD_RELOC_IP2K_PC_SKIP:
      return &ip2k_elf_howto_table[ (int) R_IP2K_PC_SKIP];
    case BFD_RELOC_IP2K_TEXT:
      return &ip2k_elf_howto_table[ (int) R_IP2K_TEXT];
    case BFD_RELOC_IP2K_FR_OFFSET:
      return &ip2k_elf_howto_table[ (int) R_IP2K_FR_OFFSET];
    case BFD_RELOC_IP2K_EX8DATA:
      return &ip2k_elf_howto_table[ (int) R_IP2K_EX8DATA];
    default:
      /* Pacify gcc -Wall. */
      return NULL;
    }
  return NULL;
}

#define PAGENO(ABSADDR) ((ABSADDR) & 0x1C000)
#define BASEADDR(SEC)	((SEC)->output_section->vma + (SEC)->output_offset)

#define UNDEFINED_SYMBOL (~(bfd_vma)0)

/* Return the value of the symbol associated with the relocation IREL.  */

static bfd_vma
symbol_value (abfd, symtab_hdr, isymbuf, irel)
     bfd *abfd;
     Elf_Internal_Shdr *symtab_hdr;
     Elf32_Internal_Sym *isymbuf;
     Elf_Internal_Rela *irel;   
{
  if (ELF32_R_SYM (irel->r_info) < symtab_hdr->sh_info)
    {
      Elf_Internal_Sym *isym;
      asection *sym_sec;

      isym = isymbuf + ELF32_R_SYM (irel->r_info);
      if (isym->st_shndx == SHN_UNDEF)
	sym_sec = bfd_und_section_ptr;
      else if (isym->st_shndx == SHN_ABS)
	sym_sec = bfd_abs_section_ptr;
      else if (isym->st_shndx == SHN_COMMON)
	sym_sec = bfd_com_section_ptr;
      else
	sym_sec = bfd_section_from_elf_index (abfd, isym->st_shndx);

      return isym->st_value + BASEADDR (sym_sec);
    }
  else
    {
      unsigned long indx;
      struct elf_link_hash_entry *h;

      indx = ELF32_R_SYM (irel->r_info) - symtab_hdr->sh_info;
      h = elf_sym_hashes (abfd)[indx];
      BFD_ASSERT (h != NULL);

      if (h->root.type != bfd_link_hash_defined
	  && h->root.type != bfd_link_hash_defweak)
	return UNDEFINED_SYMBOL;

      return (h->root.u.def.value + BASEADDR (h->root.u.def.section));
    }
}

/* Determine if the instruction sequence matches that for
   the prologue of a switch dispatch table with fewer than
   128 entries.
 
          sc
          page    $nnn0
          jmp     $nnn0
          add     w,wreg
          add     pcl,w
  addr=>
          page    $nnn1
          jmp     $nnn1
 	   page    $nnn2
 	   jmp     $nnn2
 	   ...
 	   page    $nnnN
 	   jmp     $nnnN
 
  After relaxation.
  	   sc
 	   page    $nnn0
  	   jmp     $nnn0
 	   add     pcl,w
  addr=>
  	   jmp     $nnn1
 	   jmp     $nnn2
 	   ...
          jmp     $nnnN  */

static boolean 
is_switch_128_dispatch_table_p (abfd, addr, relaxed, misc)
     bfd *abfd ATTRIBUTE_UNUSED;                
     bfd_vma addr;
     boolean relaxed;
     struct misc *misc;
{
  bfd_byte code0, code1;

  if (addr < (3 * 2))
    return false;

  code0 = bfd_get_8 (abfd, misc->contents + addr - 2);
  code1 = bfd_get_8 (abfd, misc->contents + addr - 1);

  /* Is it ADD PCL,W */
  if (! IS_ADD_PCL_W_OPCODE (code0, code1))
    return false;

  code0 = bfd_get_8 (abfd, misc->contents + addr - 4);
  code1 = bfd_get_8 (abfd, misc->contents + addr - 3);

  if (relaxed)
    /* Is it ADD W,WREG  */
    return ! IS_ADD_W_WREG_OPCODE (code0, code1);

  else
    {
      /* Is it ADD W,WREG  */
      if (! IS_ADD_W_WREG_OPCODE (code0, code1))
	return false;

      code0 = bfd_get_8 (abfd, misc->contents + addr - 6);
      code1 = bfd_get_8 (abfd, misc->contents + addr - 5);

      /* Is it JMP $nnnn  */
      if (! IS_JMP_OPCODE (code0, code1))
        return false;
    }

  /* It looks like we've found the prologue for
     a 1-127 entry switch dispatch table.  */
  return true;
}

/* Determine if the instruction sequence matches that for
   the prologue switch dispatch table with fewer than
   256 entries but more than 127.
 
   Before relaxation.
          push    %lo8insn(label) ; Push address of table
          push    %hi8insn(label)
          add     w,wreg          ; index*2 => offset
          snc                     ; CARRY SET?
          inc     1(sp)           ; Propagate MSB into table address
          add     2(sp),w         ; Add low bits of offset to table address
          snc                     ; and handle any carry-out
          inc     1(sp)
   addr=>
          page    __indjmp        ; Do an indirect jump to that location
          jmp     __indjmp
   label:                         ; case dispatch table starts here
 	   page    $nnn1
 	   jmp	   $nnn1
 	   page	   $nnn2
 	   jmp     $nnn2
 	   ...
 	   page    $nnnN
 	   jmp	   $nnnN
 
  After relaxation.
          push    %lo8insn(label) ; Push address of table
          push    %hi8insn(label)
          add     2(sp),w         ; Add low bits of offset to table address
          snc                     ; and handle any carry-out
          inc     1(sp)
  addr=>
          page    __indjmp        ; Do an indirect jump to that location
          jmp     __indjmp
   label:                         ; case dispatch table starts here
          jmp     $nnn1
          jmp     $nnn2
          ...
          jmp     $nnnN  */

static boolean 
is_switch_256_dispatch_table_p (abfd, addr, relaxed,  misc)
     bfd *abfd ATTRIBUTE_UNUSED;
     bfd_vma addr;
     boolean relaxed;
     struct misc *misc;
{
  bfd_byte code0, code1;

  if (addr < (8 * 2))
    return false;

  code0 = bfd_get_8 (abfd, misc->contents + addr - 2);
  code1 = bfd_get_8 (abfd, misc->contents + addr - 1);

  /* Is it INC 1(SP).  */
  if (! IS_INC_1_SP_OPCODE (code0, code1))
    return false;

  code0 = bfd_get_8 (abfd, misc->contents + addr - 4);
  code1 = bfd_get_8 (abfd, misc->contents + addr - 3);

  /* Is it SNC.  */
  if (! IS_SNC_OPCODE (code0, code1))
    return false;

  code0 = bfd_get_8 (abfd, misc->contents + addr - 6);
  code1 = bfd_get_8 (abfd, misc->contents + addr - 5);

  /* Is it ADD 2(SP),W.  */
  if (! IS_ADD_2_SP_W_OPCODE (code0, code1))
    return false;

  code0 = bfd_get_8 (abfd, misc->contents + addr - 8);
  code1 = bfd_get_8 (abfd, misc->contents + addr - 7);

  if (relaxed)
    /* Is it INC 1(SP).  */
    return ! IS_INC_1_SP_OPCODE (code0, code1);

  else
    {
      /* Is it INC 1(SP).  */
      if (! IS_INC_1_SP_OPCODE (code0, code1))
	return false;

      code0 = bfd_get_8 (abfd, misc->contents + addr - 10);
      code1 = bfd_get_8 (abfd, misc->contents + addr - 9);
 
      /* Is it SNC.  */
      if (! IS_SNC_OPCODE (code0, code1))
        return false;

      code0 = bfd_get_8 (abfd, misc->contents + addr - 12);
      code1 = bfd_get_8 (abfd, misc->contents + addr - 11);

      /* Is it ADD W,WREG.  */
      if (! IS_ADD_W_WREG_OPCODE (code0, code1))
	return false;
    }

  /* It looks like we've found the prologue for
     a 128-255 entry switch dispatch table.  */
  return true;
}

static boolean
relax_switch_dispatch_tables_pass1 (abfd, sec, addr, misc)
     bfd *abfd;
     asection *sec;
     bfd_vma addr;
     struct misc *misc;
{
  if (addr + 3 < sec->_cooked_size)
    {
      bfd_byte code0 = bfd_get_8 (abfd, misc->contents + addr + 2);
      bfd_byte code1 = bfd_get_8 (abfd, misc->contents + addr + 3);

      if (IS_JMP_OPCODE (code0, code1)
	  && is_switch_128_dispatch_table_p (abfd, addr, false, misc))
	{
	  /* Delete ADD W,WREG from prologue.  */
	  ip2k_elf_relax_delete_bytes (abfd, sec, addr - (2 * 2), (1 * 2));
	  return true;
	}

      if (IS_JMP_OPCODE (code0, code1)
	  && is_switch_256_dispatch_table_p (abfd, addr, false, misc))
	{
	  /* Delete ADD W,WREG; SNC ; INC 1(SP) from prologue.  */
	  ip2k_elf_relax_delete_bytes (abfd, sec, addr - 6 * 2, 3 * 2);
	  return true;
	}
    }
 
  return true;
}

static boolean
unrelax_dispatch_table_entries (abfd, sec, first, last, changed, misc)
     bfd *abfd;
     asection *sec;
     bfd_vma first;
     bfd_vma last;
     boolean *changed;
     struct misc *misc;
{
  bfd_vma addr = first;

  while (addr < last)
    {
      bfd_byte code0 = bfd_get_8 (abfd, misc->contents + addr);
      bfd_byte code1 = bfd_get_8 (abfd, misc->contents + addr + 1);

      /* We are only expecting to find PAGE or JMP insns
         in the dispatch table. If we find anything else
         something has gone wrong failed the relaxation
         which will cause the link to be aborted.  */

      if (IS_PAGE_OPCODE (code0, code1))
	/* Skip the PAGE and JMP insns.  */
        addr += 4;
      else if (IS_JMP_OPCODE (code0, code1))
         {
            Elf_Internal_Rela * irelend = misc->irelbase
					  + sec->reloc_count;
            Elf_Internal_Rela * irel;

            /* Find the relocation entry.  */
            for (irel = misc->irelbase; irel < irelend; irel++)
               {
                  if (irel->r_offset == addr
                      && ELF32_R_TYPE (irel->r_info) == R_IP2K_ADDR16CJP)
                    {
                      if (! add_page_insn (abfd, sec, irel, misc))
			/* Something has gone wrong.  */
                        return false;

		      *changed = true;
		      break;
                    }
               }

	    /* If we fell off the end something has gone wrong.  */
	    if (irel >= irelend)
	      /* Something has gone wrong.  */
	      return false;

	    /* Skip the PAGE and JMP isns.  */
	    addr += 4;
	    /* Acount for the new PAGE insn.  */
            last += 2;
          }
       else
	 /* Something has gone wrong.  */
	 return false;
    }

  return true;
}

static boolean 
unrelax_switch_dispatch_tables_passN (abfd, sec, addr, changed, misc)
     bfd *abfd;
     asection *sec;
     bfd_vma addr;
     boolean *changed;
     struct misc *misc;
{
  if (2 <= addr && (addr + 3) < sec->_cooked_size)
    {
      bfd_byte code0 = bfd_get_8 (abfd, misc->contents + addr - 2);
      bfd_byte code1 = bfd_get_8 (abfd, misc->contents + addr - 1);

      if (IS_PAGE_OPCODE (code0, code1))
	{
	  addr -= 2;
	  code0 = bfd_get_8 (abfd, misc->contents + addr + 2);
          code1 = bfd_get_8 (abfd, misc->contents + addr + 3);
	}
      else
	{
	  code0 = bfd_get_8 (abfd, misc->contents + addr);
	  code1 = bfd_get_8 (abfd, misc->contents + addr + 1);
	}

      if (IS_JMP_OPCODE (code0, code1)
          && is_switch_128_dispatch_table_p (abfd, addr, true, misc))
        {
	  bfd_vma first = addr;
	  bfd_vma last  = first;
	  boolean relaxed = true;

	  /* On the final pass we must check if *all* entries in the
	     dispatch table are relaxed. If *any* are not relaxed
	     then we must unrelax *all* the entries in the dispach
	     table and also unrelax the dispatch table prologue.  */

	  /* Find the last entry in the dispach table.  */
	  while (last < sec->_cooked_size)
	     {
	        code0 = bfd_get_8 (abfd, misc->contents + last);
	        code1 = bfd_get_8 (abfd, misc->contents + last + 1);

		if (IS_PAGE_OPCODE (code0, code1))
		  relaxed = false;
		else if (! IS_JMP_OPCODE (code0, code1))
		    break;

	        last += 2;
	     }

	  /* We should have found the end of the dispatch table
	     before reaching the end of the section. If we've have
	     reached the end then fail the relaxation which will
	     cause the link to be aborted.  */
	  if (last >= sec->_cooked_size)
	    /* Something has gone wrong.  */
	    return false;

	  /* If we found an unrelaxed entry then
	     unlrelax all the switch table entries.  */
	  if (! relaxed )
	    {
	      if (! unrelax_dispatch_table_entries (abfd, sec, first,
						    last, changed, misc))
		/* Something has gone wrong.  */
	        return false;

	      if (! is_switch_128_dispatch_table_p (abfd, addr, true, misc))
		/* Something has gone wrong.  */
		return false;
		
              /* Unrelax the prologue.  */

              /* Insert an ADD W,WREG insnstruction.  */
              if (! ip2k_elf_relax_add_bytes (abfd, sec,
					      addr - 2,
					      add_w_wreg_opcode,
					      sizeof (add_w_wreg_opcode),
					      0))
		/* Something has gone wrong.  */
                return false;
	    }

          return true;
        }

      if (IS_JMP_OPCODE (code0, code1)
          && is_switch_256_dispatch_table_p (abfd, addr, true, misc))
        {
          bfd_vma first = addr;
          bfd_vma last;
          boolean relaxed = true;

          /* On the final pass we must check if *all* entries in the
             dispatch table are relaxed. If *any* are not relaxed
             then we must unrelax *all* the entries in the dispach
             table and also unrelax the dispatch table prologue.  */

	  /* Note the 1st PAGE/JMP instructions are part of the
	     prologue and can safely be relaxed.  */

          code0 = bfd_get_8 (abfd, misc->contents + first);
          code1 = bfd_get_8 (abfd, misc->contents + first + 1);

	  if (IS_PAGE_OPCODE (code0, code1))
	    {
	      first += 2;
              code0 = bfd_get_8 (abfd, misc->contents + first);
              code1 = bfd_get_8 (abfd, misc->contents + first + 1);
	    }

          if (! IS_JMP_OPCODE (code0, code1))
	    /* Something has gone wrong.  */
	    return false;

          first += 2;
	  last = first; 

          /* Find the last entry in the dispach table.  */
          while (last < sec->_cooked_size)
             {
                code0 = bfd_get_8 (abfd, misc->contents + last);
                code1 = bfd_get_8 (abfd, misc->contents + last + 1);

                if (IS_PAGE_OPCODE (code0, code1))
                  relaxed = false;
                else if (! IS_JMP_OPCODE (code0, code1))
                    break;

                last += 2;
             }

          /* We should have found the end of the dispatch table
             before reaching the end of the section. If we have
             reached the end of the section then fail the
	     relaxation.  */
          if (last >= sec->_cooked_size)
            return false;

          /* If we found an unrelaxed entry then
              unrelax all the switch table entries.  */
          if (! relaxed)
	    {
	      if (! unrelax_dispatch_table_entries (abfd, sec, first,
						    last, changed, misc))
		return false;

              if (! is_switch_256_dispatch_table_p (abfd, addr, true, misc))
		return false;

              /* Unrelax the prologue.  */

              /* Insert an INC 1(SP) insnstruction.  */
              if (! ip2k_elf_relax_add_bytes (abfd, sec,
                                              addr - 6,
                                              inc_1_sp_opcode,
                                              sizeof (inc_1_sp_opcode),
					      0))
		return false;

              /* Insert an SNC insnstruction.  */
              if (! ip2k_elf_relax_add_bytes (abfd, sec,
					      addr - 6,
					      snc_opcode,
					      sizeof (snc_opcode),
					      0))
		return false;

	      /* Insert an ADD W,WREG insnstruction.  */
              if (! ip2k_elf_relax_add_bytes (abfd, sec,
					     addr - 6,
				 	     add_w_wreg_opcode,
					     sizeof (add_w_wreg_opcode),
					     0))
		return false;
	    }

          return true;
        }
    }

  return true;
}

/* This function handles relaxing for the ip2k.  */

static boolean
ip2k_elf_relax_section (abfd, sec, link_info, again)
     bfd *abfd;
     asection *sec;
     struct bfd_link_info *link_info;
     boolean *again;
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Rela *internal_relocs;
  bfd_byte *contents = NULL;
  Elf_Internal_Sym *isymbuf = NULL;
  static asection * first_section = NULL;
  static asection * last_section = NULL;
  static boolean changed = false;
  static boolean final_pass = false;
  static unsigned int pass = 0;
  struct misc misc;
  asection *stab;

  /* Assume nothing changes.  */
  *again = false;

  if (first_section == NULL)
    first_section = sec;

  if (first_section == sec)
    {
      changed = false;
      pass++;
    }

  /* If we make too many passes then it's a sign that
     something is wrong and we fail the relaxation.
     Note if everything is working correctly then the
     relaxation should converge reasonably quickly.  */
  if (pass == 4096)
    return false;

  /* We don't have to do anything for a relocatable link,
     if this section does not have relocs, or if this is
     not a code section.  */
  if (link_info->relocateable
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0
      || (sec->flags & SEC_CODE) == 0)
    return true;

  if (pass == 1)
    last_section = sec;

  /* If this is the first time we have been called
      for this section, initialise the cooked size.  */
  if (sec->_cooked_size == 0)
    sec->_cooked_size = sec->_raw_size;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;

  internal_relocs = _bfd_elf32_link_read_relocs (abfd, sec, NULL,
						 (Elf_Internal_Rela *)NULL,
						 link_info->keep_memory);
  if (internal_relocs == NULL)
    goto error_return;

  /* Make sure the stac.rela stuff gets read in.  */
  stab = bfd_get_section_by_name (abfd, ".stab");

  if (stab)
    {
      /* So stab does exits.  */
      Elf_Internal_Rela * irelbase;

      irelbase = _bfd_elf32_link_read_relocs (abfd, stab, NULL,
					      (Elf_Internal_Rela *)NULL,
					      link_info->keep_memory);
    }

  /* Get section contents cached copy if it exists.  */
  if (contents == NULL)
    {
      /* Get cached copy if it exists.  */
      if (elf_section_data (sec)->this_hdr.contents != NULL)
	contents = elf_section_data (sec)->this_hdr.contents;
      else
	{
	  /* Go get them off disk.  */
	  contents = (bfd_byte *) bfd_malloc (sec->_raw_size);
	  if (contents == NULL)
	    goto error_return;

	  if (! bfd_get_section_contents (abfd, sec, contents,
					  (file_ptr) 0, sec->_raw_size))
	    goto error_return;
	}
    }
      
  /* Read this BFD's symbols cached copy if it exists.  */
  if (isymbuf == NULL && symtab_hdr->sh_info != 0)
    {
      isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
      if (isymbuf == NULL)
	isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
					symtab_hdr->sh_info, 0,
					NULL, NULL, NULL);
      if (isymbuf == NULL)
	goto error_return;
    }

  misc.symtab_hdr = symtab_hdr;
  misc.isymbuf = isymbuf;
  misc.irelbase = internal_relocs;
  misc.contents = contents;
  
  /* This is where all the relaxation actually get done.  */

  if (pass == 1)
    {
      /* On the first pass we remove *all* page instructions and
         relax the prolog for switch dispatch tables. This gets
	 us to the starting point for subsequent passes where
	 we add page instructions back in as needed.  */

      if (! ip2k_elf_relax_section_pass1 (abfd, sec, again, &misc))
	goto error_return;

      changed |= *again;
    }
  else
    {
      /* Add page instructions back in as needed but we ignore 
	 the issue with sections (functions) crossing a page
	 boundary until we have converged to an approximate
	 solution (i.e. nothing has changed on this relaxation
	 pass) and we then know roughly where the page boundaries
	 will end up.

	 After we have have converged to an approximate solution
	 we set the final pass flag and continue relaxing. On these
	 final passes if a section (function) cross page boundary
	 we will add *all* the page instructions back into such
	 sections.

	 After adding *all* page instructions back into a section
	 which crosses a page bounbdary we reset the final pass flag
	 so the we will again interate until we find a new approximate
	 solution which is closer to the final solution.  */

      if (! ip2k_elf_relax_section_passN (abfd, sec, again, &final_pass,
					  &misc))
	goto error_return;

      changed |= *again;

      /* If nothing has changed on this relaxation
	  pass restart the final relaxaton pass.  */
      if (! changed && last_section == sec)
	{
	  /* If this was the final pass and we didn't reset 
	     the final pass flag then we are done, otherwise
	     do another final pass.  */
	  if (! final_pass)
	    {
	      final_pass = true;
	      *again = true;
	    }
	}
    }

  /* Perform some house keeping after relaxing the section.  */  

  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    {
      if (! link_info->keep_memory)
	free (isymbuf);
      else
	symtab_hdr->contents = (unsigned char *) isymbuf;
    }

  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    {
      if (! link_info->keep_memory)
	free (contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
    }

  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return true;

 error_return:
  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    free (contents);
  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);
  return false;
}

/* This function handles relaxation during the first pass.  */

static boolean
ip2k_elf_relax_section_pass1 (abfd, sec, again, misc)
     bfd *abfd;
     asection *sec;
     boolean *again;
     struct misc * misc;
{
  Elf_Internal_Rela *irelend = misc->irelbase + sec->reloc_count;
  Elf_Internal_Rela *irel;

  /* Walk thru the section looking for relaxation opertunities.  */
  for (irel = misc->irelbase; irel < irelend; irel++)
    {
      if (ELF32_R_TYPE (irel->r_info) == (int) R_IP2K_PAGE3)
      {
	bfd_byte code0 = bfd_get_8 (abfd,
				    misc->contents + irel->r_offset);
	bfd_byte code1 = bfd_get_8 (abfd,
				    misc->contents + irel->r_offset + 1);

        /* Verify that this is the PAGE opcode.  */
        if (IS_PAGE_OPCODE (code0, code1))
	  {
	    /* Note that we've changed the relocs, section contents, etc.  */
	    elf_section_data (sec)->relocs = misc->irelbase;
	    elf_section_data (sec)->this_hdr.contents = misc->contents;
	    misc->symtab_hdr->contents = (bfd_byte *) misc->isymbuf;

	    /* Handle switch dispatch tables/prologues.  */
	    if (!  relax_switch_dispatch_tables_pass1 (abfd, sec,
						       irel->r_offset, misc))
	      return false;
	    
	    /* Fix the relocation's type.  */
	    irel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
				         R_IP2K_NONE);

	    /* Delete the PAGE insn.  */
	    if (! ip2k_elf_relax_delete_bytes (abfd, sec,
					       irel->r_offset,
					       sizeof (page_opcode)))
	      return false;

	    /* That will change things, so, we should relax again.
	       Note that this is not required, and it may be slow.  */
	    *again = true;
	  }
      }
    }

  return true;
}

/* This function handles relaxation for 2nd and subsequent passes.  */

static boolean
ip2k_elf_relax_section_passN (abfd, sec, again, final_pass, misc)
     bfd *abfd;
     asection *sec;
     boolean *again;
     boolean *final_pass;
     struct misc * misc;
{
  Elf_Internal_Rela *irelend = misc->irelbase + sec->reloc_count;
  Elf_Internal_Rela *irel;
  boolean add_all;

  /* If we are on the final relaxation pass and the section crosses
     then set a flag to indicate that *all* page instructions need
     to be added back into this section.  */
  if (*final_pass)
    {
      add_all = (PAGENO (BASEADDR (sec))
	         != PAGENO (BASEADDR (sec) + sec->_cooked_size));

      /* If this section crosses a page boundary set the crossed
	 page boundary flag.  */
      if (add_all)
	sec->userdata = sec;
      else
	{
	  /* If the section had previously crossed a page boundary
	     but on this pass does not then reset crossed page
	     boundary flag and rerun the 1st relaxation pass on
	     this section.  */
	  if (sec->userdata)
	    {
	      sec->userdata = NULL;
	      if (! ip2k_elf_relax_section_pass1 (abfd, sec, again, misc))
		return false;
	    }
	}
    }
  else
    add_all = false;

  /* Walk thru the section looking for call/jmp
      instructions which need a page instruction.  */
  for (irel = misc->irelbase; irel < irelend; irel++)
    {
      if (ELF32_R_TYPE (irel->r_info) == (int) R_IP2K_ADDR16CJP)
      {
        /* Get the value of the symbol referred to by the reloc.  */
        bfd_vma symval = symbol_value (abfd, misc->symtab_hdr, misc->isymbuf,
				       irel);
	bfd_byte code0, code1;

        if (symval == UNDEFINED_SYMBOL)
	  {
	    /* This appears to be a reference to an undefined
	       symbol.  Just ignore it--it will be caught by the
	       regular reloc processing.  */
	    continue;
	  }

        /* For simplicity of coding, we are going to modify the section
	   contents, the section relocs, and the BFD symbol table.  We
	   must tell the rest of the code not to free up this
	   information.  It would be possible to instead create a table
	   of changes which have to be made, as is done in coff-mips.c;
	   that would be more work, but would require less memory when
	   the linker is run.  */

	/* Get the opcode.  */
	code0 = bfd_get_8 (abfd, misc->contents + irel->r_offset);
	code1 = bfd_get_8 (abfd, misc->contents + irel->r_offset + 1);

	if (IS_JMP_OPCODE (code0, code1) || IS_CALL_OPCODE (code0, code1))
	  {
	    if (*final_pass)
	      {
		if (! unrelax_switch_dispatch_tables_passN (abfd, sec,
						            irel->r_offset,
                                                            again, misc))
		  return false;

                if (*again)
		  add_all = false;
	      }

	    code0 = bfd_get_8 (abfd, misc->contents + irel->r_offset - 2);
	    code1 = bfd_get_8 (abfd, misc->contents + irel->r_offset - 1);

	    if (! IS_PAGE_OPCODE (code0, code1))
	      {
		bfd_vma value = symval + irel->r_addend;
		bfd_vma addr  = BASEADDR (sec) + irel->r_offset;

		if (add_all || PAGENO (addr) != PAGENO (value))
		  {
		    if (! add_page_insn (abfd, sec, irel, misc))
		      return false;

		    /* That will have changed things, so,  we must relax again.  */
		    *again = true;
		  }
	       }
	   }
        }
    }
      
  /* If anything changed reset the final pass flag.  */
  if (*again)
    *final_pass = false;

  return true;
}

/* Parts of a Stabs entry.  */

#define STRDXOFF  (0)
#define TYPEOFF   (4)
#define OTHEROFF  (5)
#define DESCOFF   (6)
#define VALOFF    (8)
#define STABSIZE  (12)

/* Adjust all the relocations entries after adding or inserting instructions.  */

static void
adjust_all_relocations (abfd, sec, addr, endaddr, count, noadj)
     bfd *abfd;
     asection *sec;
     bfd_vma addr;
     bfd_vma endaddr;
     int count;
     int noadj;
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Sym *isymbuf, *isym, *isymend;
  unsigned int shndx;
  bfd_byte *contents;
  Elf_Internal_Rela *irel, *irelend, *irelbase;
  struct elf_link_hash_entry **sym_hashes;
  struct elf_link_hash_entry **end_hashes;
  unsigned int symcount;
    
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  isymbuf = (Elf32_Internal_Sym *) symtab_hdr->contents;

  shndx = _bfd_elf_section_from_bfd_section (abfd, sec);

  contents = elf_section_data (sec)->this_hdr.contents;

  irelbase = elf_section_data (sec)->relocs;
  irelend = irelbase + sec->reloc_count;

  for (irel = irelbase; irel < irelend; irel++)
    {
      if (ELF32_R_TYPE (irel->r_info) != R_IP2K_NONE)
        {
          /* Get the value of the symbol referred to by the reloc.  */
          if (ELF32_R_SYM (irel->r_info) < symtab_hdr->sh_info)
            {
              asection *sym_sec;

              /* A local symbol.  */
	      isym = isymbuf + ELF32_R_SYM (irel->r_info);
              sym_sec = bfd_section_from_elf_index (abfd, isym->st_shndx);

              if (isym->st_shndx == shndx)
                {
                  bfd_vma baseaddr = BASEADDR (sec);
                  bfd_vma symval = BASEADDR (sym_sec) + isym->st_value
                                   + irel->r_addend;

                  if ((baseaddr + addr + noadj) <= symval
                      && symval < (baseaddr + endaddr))
                    irel->r_addend += count;
                }
            }
        }

      /* Do this only for PC space relocations.  */
      if (addr <= irel->r_offset && irel->r_offset < endaddr)
        irel->r_offset += count;
    }

  /* When adding an instruction back it is sometimes necessary to move any
     global or local symbol that was referencing the first instruction of
     the moved block to refer to the first instruction of the inserted block.

     For example adding a PAGE instruction before a CALL or JMP requires
     that any label on the CALL or JMP is moved to the PAGE insn.  */
  addr += noadj;

  /* Adjust the local symbols defined in this section.  */
  isymend = isymbuf + symtab_hdr->sh_info;
  for (isym = isymbuf; isym < isymend; isym++)
    {
      if (isym->st_shndx == shndx
	  && addr <= isym->st_value
	  && isym->st_value < endaddr)
	isym->st_value += count;
    }

    /* Now adjust the global symbols defined in this section.  */
  symcount = (symtab_hdr->sh_size / sizeof (Elf32_External_Sym)
	      - symtab_hdr->sh_info);
  sym_hashes = elf_sym_hashes (abfd);
  end_hashes = sym_hashes + symcount;
  for (; sym_hashes < end_hashes; sym_hashes++)
    {
      struct elf_link_hash_entry *sym_hash = *sym_hashes;
      if ((sym_hash->root.type == bfd_link_hash_defined
	   || sym_hash->root.type == bfd_link_hash_defweak)
	  && sym_hash->root.u.def.section == sec)
	{
          if (addr <= sym_hash->root.u.def.value
              && sym_hash->root.u.def.value < endaddr)
            {
	      sym_hash->root.u.def.value += count;
            }
	}
    }

  return;
}

static boolean
add_page_insn (abfd, sec, irel, misc)
      bfd *abfd;
      asection *sec;
      Elf_Internal_Rela *irel;
      struct misc *misc;
{
  /* Note that we've changed the relocs, section contents, etc.  */
  elf_section_data (sec)->relocs = misc->irelbase;
  elf_section_data (sec)->this_hdr.contents = misc->contents;
  misc->symtab_hdr->contents = (bfd_byte *) misc->isymbuf;

  /* Add the PAGE insn.  */
  if (! ip2k_elf_relax_add_bytes (abfd, sec, irel->r_offset,
                                  page_opcode,
                                  sizeof (page_opcode),
				  sizeof (page_opcode)))
    return false;
  else
    {
       Elf32_Internal_Rela * jrel = irel - 1;

       /* Add relocation for PAGE insn added.  */
       if (ELF32_R_TYPE (jrel->r_info) != R_IP2K_NONE)
	 {
	   bfd_byte code0, code1;
	   char *msg = NULL;
	   
	   /* Get the opcode.  */
	   code0 = bfd_get_8 (abfd, misc->contents + irel->r_offset);
	   code1 = bfd_get_8 (abfd, misc->contents + irel->r_offset + 1);

	   if (IS_JMP_OPCODE (code0, code1))
	     msg = "\tJMP instruction missing a preceeding PAGE instruction in %s\n\n";

	   else if (IS_CALL_OPCODE (code0, code1))
	     msg = "\tCALL instruction missing a preceeding PAGE instruction in %s\n\n";

	   if (msg)
	     {
	       fprintf (stderr, "\n\t *** LINKER RELAXATION failure ***\n");
	       fprintf (stderr, msg, sec->owner->filename);
	     }

	   return false;
	 }

       jrel->r_addend = irel->r_addend;
       jrel->r_offset = irel->r_offset - sizeof (page_opcode);
       jrel->r_info = ELF32_R_INFO (ELF32_R_SYM (irel->r_info),
                                    R_IP2K_PAGE3);
     }

   return true;
}

/* Insert bytes into a section while relaxing.  */

static boolean
ip2k_elf_relax_add_bytes (abfd, sec, addr, bytes, count, noadj)
     bfd *abfd;
     asection *sec;
     bfd_vma addr;
     const bfd_byte *bytes;
     int count;
     int noadj;
{
  bfd_byte *contents = elf_section_data (sec)->this_hdr.contents;
  bfd_vma endaddr = sec->_cooked_size;

  /* Make room to insert the bytes.  */
  memmove (contents + addr + count, contents + addr, endaddr - addr);

  /* Insert the bytes into the section.  */
  memcpy  (contents + addr, bytes, count);
  
  sec->_cooked_size += count;

  adjust_all_relocations (abfd, sec, addr, endaddr, count, noadj);
  return true;
}

/* Delete some bytes from a section while relaxing.  */

static boolean
ip2k_elf_relax_delete_bytes (abfd, sec, addr, count)
     bfd *abfd;
     asection *sec;
     bfd_vma addr;
     int count;
{
  bfd_byte *contents = elf_section_data (sec)->this_hdr.contents;
  bfd_vma endaddr = sec->_cooked_size;

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count,
	   endaddr - addr - count);

  sec->_cooked_size -= count;

  adjust_all_relocations (abfd, sec, addr + count, endaddr, -count, 0);
  return true;
}

/* -------------------------------------------------------------------- */

/* XXX: The following code is the result of a cut&paste.  This unfortunate
   practice is very widespread in the various target back-end files.  */

/* Set the howto pointer for a IP2K ELF reloc.  */

static void
ip2k_info_to_howto_rela (abfd, cache_ptr, dst)
     bfd * abfd ATTRIBUTE_UNUSED;
     arelent * cache_ptr;
     Elf32_Internal_Rela * dst;
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (dst->r_info);
  switch (r_type)
    {
    default:
      cache_ptr->howto = & ip2k_elf_howto_table [r_type];
      break;
    }
}

/* Perform a single relocation.
   By default we use the standard BFD routines.  */

static bfd_reloc_status_type
ip2k_final_link_relocate (howto, input_bfd, input_section, contents, rel,
			  relocation)
     reloc_howto_type *  howto;
     bfd *               input_bfd;
     asection *          input_section;
     bfd_byte *          contents;
     Elf_Internal_Rela * rel;
     bfd_vma             relocation;
{
  bfd_reloc_status_type r = bfd_reloc_ok;

  switch (howto->type)
    {
      /* Handle data space relocations.  */
    case R_IP2K_FR9:
    case R_IP2K_BANK:
      if ((relocation & IP2K_DATA_MASK) == IP2K_DATA_VALUE)
	relocation &= ~IP2K_DATA_MASK;
      else
	r = bfd_reloc_notsupported;
      break;

    case R_IP2K_LO8DATA:
    case R_IP2K_HI8DATA:
    case R_IP2K_EX8DATA:
      break;

      /* Handle insn space relocations.  */
    case R_IP2K_ADDR16CJP:
    case R_IP2K_PAGE3:
    case R_IP2K_LO8INSN:
    case R_IP2K_HI8INSN:
    case R_IP2K_PC_SKIP:
      if ((relocation & IP2K_INSN_MASK) == IP2K_INSN_VALUE)
	relocation &= ~IP2K_INSN_MASK;
      else
	r = bfd_reloc_notsupported;
      break;

    case R_IP2K_16:
      /* If this is a relocation involving a TEXT
	 symbol, reduce it to a word address.  */
      if ((relocation & IP2K_INSN_MASK) == IP2K_INSN_VALUE)
	howto = &ip2k_elf_howto_table[ (int) R_IP2K_TEXT];
      break;

      /* Pass others through.  */
    default:
      break;
    }

  /* Only install relocation if above tests did not disqualify it.  */
  if (r == bfd_reloc_ok)
    r = _bfd_final_link_relocate (howto, input_bfd, input_section,
				  contents, rel->r_offset,
				  relocation, rel->r_addend);

  return r;
}

/* Relocate a IP2K ELF section.

   The RELOCATE_SECTION function is called by the new ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures; if the section
   actually uses Rel structures, the r_addend field will always be
   zero.

   This function is responsible for adjusting the section contents as
   necessary, and (if using Rela relocs and generating a relocateable
   output file) adjusting the reloc addend as necessary.

   This function does not have to worry about setting the reloc
   address or the reloc symbol index.

   LOCAL_SYMS is a pointer to the swapped in local symbols.

   LOCAL_SECTIONS is an array giving the section in the input file
   corresponding to the st_shndx field of each local symbol.

   The global hash table entry for the global symbols can be found
   via elf_sym_hashes (input_bfd).

   When generating relocateable output, this function must handle
   STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
   going to be the section symbol corresponding to the output
   section, which means that the addend must be adjusted
   accordingly.  */

static boolean
ip2k_elf_relocate_section (output_bfd, info, input_bfd, input_section,
			   contents, relocs, local_syms, local_sections)
     bfd *                   output_bfd ATTRIBUTE_UNUSED;
     struct bfd_link_info *  info;
     bfd *                   input_bfd;
     asection *              input_section;
     bfd_byte *              contents;
     Elf_Internal_Rela *     relocs;
     Elf_Internal_Sym *      local_syms;
     asection **             local_sections;
{
  Elf_Internal_Shdr *           symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes;
  Elf_Internal_Rela *           rel;
  Elf_Internal_Rela *           relend;

  if (info->relocateable)
    return true;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *           howto;
      unsigned long                r_symndx;
      Elf_Internal_Sym *           sym;
      asection *                   sec;
      struct elf_link_hash_entry * h;
      bfd_vma                      relocation;
      bfd_reloc_status_type        r;
      const char *                 name = NULL;
      int                          r_type;
      
      /* This is a final link.  */
      r_type = ELF32_R_TYPE (rel->r_info);
      r_symndx = ELF32_R_SYM (rel->r_info);
      howto  = ip2k_elf_howto_table + ELF32_R_TYPE (rel->r_info);
      h      = NULL;
      sym    = NULL;
      sec    = NULL;
      
      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = BASEADDR (sec) + sym->st_value;
	  
	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = (name == NULL) ? bfd_section_name (input_bfd, sec) : name;
	}
      else
	{
	  h = sym_hashes [r_symndx - symtab_hdr->sh_info];
	  
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  name = h->root.root.string;
	  
	  if (h->root.type == bfd_link_hash_defined
	      || h->root.type == bfd_link_hash_defweak)
	    {
	      sec = h->root.u.def.section;
	      relocation = h->root.u.def.value + BASEADDR (sec);
	    }
	  else if (h->root.type == bfd_link_hash_undefweak)
	    {
	      relocation = 0;
	    }
	  else
	    {
	      if (! ((*info->callbacks->undefined_symbol)
		     (info, h->root.root.string, input_bfd,
		      input_section, rel->r_offset,
		     (! info->shared || info->no_undefined))))
		return false;
	      relocation = 0;
	    }
	}

      /* Finally, the sole IP2K-specific part.  */
      r = ip2k_final_link_relocate (howto, input_bfd, input_section,
				     contents, rel, relocation);

      if (r != bfd_reloc_ok)
	{
	  const char * msg = (const char *) NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      r = info->callbacks->reloc_overflow
		(info, name, howto->name, (bfd_vma) 0,
		 input_bfd, input_section, rel->r_offset);
	      break;
	      
	    case bfd_reloc_undefined:
	      r = info->callbacks->undefined_symbol
		(info, name, input_bfd, input_section, rel->r_offset, true);
	      break;
	      
	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      break;

	      /* This is how ip2k_final_link_relocate tells us of a non-kosher
                 reference between insn & data address spaces.  */
	    case bfd_reloc_notsupported:
              if (sym != NULL) /* Only if it's not an unresolved symbol.  */
	         msg = _("unsupported relocation between data/insn address spaces");
	      break;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous relocation");
	      break;

	    default:
	      msg = _("internal error: unknown error");
	      break;
	    }

	  if (msg)
	    r = info->callbacks->warning
	      (info, msg, name, input_bfd, input_section, rel->r_offset);

	  if (! r)
	    return false;
	}
    }

  return true;
}

static asection *
ip2k_elf_gc_mark_hook (sec, info, rel, h, sym)
     asection *sec;
     struct bfd_link_info *info ATTRIBUTE_UNUSED;
     Elf_Internal_Rela *rel;
     struct elf_link_hash_entry *h;
     Elf_Internal_Sym *sym;
{
  if (h != NULL)
    {
      switch (ELF32_R_TYPE (rel->r_info))
      {
#if 0 
      case R_IP2K_GNU_VTINHERIT:
      case R_IP2K_GNU_VTENTRY:
        break;
#endif

      default:
        switch (h->root.type)
          {
          case bfd_link_hash_defined:
          case bfd_link_hash_defweak:
            return h->root.u.def.section;

          case bfd_link_hash_common:
            return h->root.u.c.p->section;

          default:
            break;
          }
       }
     }
   else
     {
       if (!(elf_bad_symtab (sec->owner)
	     && ELF_ST_BIND (sym->st_info) != STB_LOCAL)
	   && ! ((sym->st_shndx <= 0 || sym->st_shndx >= SHN_LORESERVE)
		 && sym->st_shndx != SHN_COMMON))
          {
            return bfd_section_from_elf_index (sec->owner, sym->st_shndx);
          }
      }
  return NULL;
}

static boolean
ip2k_elf_gc_sweep_hook (abfd, info, sec, relocs)
     bfd *abfd ATTRIBUTE_UNUSED;
     struct bfd_link_info *info ATTRIBUTE_UNUSED;
     asection *sec ATTRIBUTE_UNUSED;
     const Elf_Internal_Rela *relocs ATTRIBUTE_UNUSED;
{
  /* we don't use got and plt entries for ip2k */
  return true;
}


/* -------------------------------------------------------------------- */


#define TARGET_BIG_SYM	 bfd_elf32_ip2k_vec
#define TARGET_BIG_NAME  "elf32-ip2k"

#define ELF_ARCH	 bfd_arch_ip2k
#define ELF_MACHINE_CODE EM_IP2K
#define ELF_MAXPAGESIZE  1 /* No pages on the IP2K */

#define elf_info_to_howto_rel			NULL
#define elf_info_to_howto			ip2k_info_to_howto_rela

#define elf_backend_can_gc_sections     	1
#define elf_backend_rela_normal			1
#define elf_backend_gc_mark_hook                ip2k_elf_gc_mark_hook
#define elf_backend_gc_sweep_hook               ip2k_elf_gc_sweep_hook

#define elf_backend_relocate_section		ip2k_elf_relocate_section

#define elf_symbol_leading_char			'_'
#define bfd_elf32_bfd_reloc_type_lookup		ip2k_reloc_type_lookup
#define bfd_elf32_bfd_relax_section		ip2k_elf_relax_section


#include "elf32-target.h"

