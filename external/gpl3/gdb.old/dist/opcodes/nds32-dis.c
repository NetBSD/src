/* NDS32-specific support for 32-bit ELF.
   Copyright (C) 2012-2017 Free Software Foundation, Inc.
   Contributed by Andes Technology Corporation.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include "ansidecl.h"
#include "dis-asm.h"
#include "bfd.h"
#include "symcat.h"
#include "libiberty.h"
#include "opintl.h"
#include "bfd_stdint.h"
#include "hashtab.h"
#include "nds32-asm.h"
#include "opcode/nds32.h"

/* Get fields macro define.  */
#define MASK_OP(insn, mask)	((insn) & (0x3f << 25 | (mask)))

/* Default text to print if an instruction isn't recognized.  */
#define UNKNOWN_INSN_MSG _("*unknown*")
#define NDS32_PARSE_INSN16      0x01
#define NDS32_PARSE_INSN32      0x02
#define NDS32_PARSE_EX9IT       0x04
#define NDS32_PARSE_EX9TAB      0x08

extern struct nds32_opcode nds32_opcodes[];
extern const field_t operand_fields[];
extern const keyword_t *keywords[];
extern const keyword_t keyword_gpr[];
static void print_insn16 (bfd_vma pc, disassemble_info *info,
			  uint32_t insn, uint32_t parse_mode);
static void print_insn32 (bfd_vma pc, disassemble_info *info, uint32_t insn,
			  uint32_t parse_mode);
static uint32_t nds32_mask_opcode (uint32_t);
static void nds32_special_opcode (uint32_t, struct nds32_opcode **);

/* define in objdump.c.  */
struct objdump_disasm_info
{
  bfd *              abfd;
  asection *         sec;
  bfd_boolean        require_sec;
  arelent **         dynrelbuf;
  long               dynrelcount;
  disassembler_ftype disassemble_fn;
  arelent *          reloc;
};

/* file_ptr    ex9_filepos=NULL;.  */
bfd_byte *ex9_data = NULL;
int ex9_ready = 0, ex9_base_offset = 0;

/* Hash function for disassemble.  */

static htab_t opcode_htab;

static void
nds32_ex9_info (bfd_vma pc ATTRIBUTE_UNUSED,
		disassemble_info *info, uint32_t ex9_index)
{
  uint32_t insn;
  static asymbol *itb = NULL;
  bfd_byte buffer[4];
  long unsigned int isec_vma;

  /* Lookup itb symbol.  */
  if (!itb)
    {
      int i;

      for (i = 0; i < info->symtab_size; i++)
	if (bfd_asymbol_name (info->symtab[i])
	    && (strcmp (bfd_asymbol_name (info->symtab[i]), "$_ITB_BASE_") == 0
		|| strcmp (bfd_asymbol_name (info->symtab[i]),
			   "_ITB_BASE_") == 0))
	  {
	    itb = info->symtab[i];
	    break;
	  }

      /* Lookup it only once, in case _ITB_BASE_ doesn't exist at all.  */
      if (itb == NULL)
	itb = (void *) -1;
    }

  if (itb == (void *) -1)
    return;

  isec_vma = itb->section->vma;
  isec_vma = itb->section->vma - bfd_asymbol_value (itb);
  if (!itb->section || !itb->section->owner)
    return;
  bfd_get_section_contents (itb->section->owner, itb->section, buffer,
			    ex9_index * 4 - isec_vma, 4);
  insn = bfd_getb32 (buffer);
  /* 16-bit instructions in ex9 table.  */
  if (insn & 0x80000000)
    print_insn16 (pc, info, (insn & 0x0000FFFF),
		  NDS32_PARSE_INSN16 | NDS32_PARSE_EX9IT);
  /* 32-bit instructions in ex9 table.  */
  else
    print_insn32 (pc, info, insn, NDS32_PARSE_INSN32 | NDS32_PARSE_EX9IT);
}

/* Find the value map register name.  */

static keyword_t *
nds32_find_reg_keyword (keyword_t *reg, int value)
{
  if (!reg)
    return NULL;

  while (reg->name != NULL && reg->value != value)
    {
      reg++;
    }
  if (reg->name == NULL)
    return NULL;
  return reg;
}

static void
nds32_parse_audio_ext (const field_t *pfd,
		       disassemble_info *info, uint32_t insn)
{
  fprintf_ftype func = info->fprintf_func;
  void *stream = info->stream;
  keyword_t *psys_reg;
  int int_value, new_value;

  if (pfd->hw_res == HW_INT || pfd->hw_res == HW_UINT)
    {
      if (pfd->hw_res == HW_INT)
	int_value =
	  N32_IMMS ((insn >> pfd->bitpos), pfd->bitsize) << pfd->shift;
      else
	int_value = __GF (insn, pfd->bitpos, pfd->bitsize) << pfd->shift;

      if (int_value < 10)
	func (stream, "#%d", int_value);
      else
	func (stream, "#0x%x", int_value);
      return;
    }
  int_value =
    __GF (insn, pfd->bitpos, pfd->bitsize) << pfd->shift;
  new_value = int_value;
  psys_reg = (keyword_t*) keywords[pfd->hw_res];

  /* p = bit[4].bit[1:0], r = bit[4].bit[3:2].  */
  if (strcmp (pfd->name, "im5_i") == 0)
    {
      new_value = int_value & 0x03;
      new_value |= ((int_value & 0x10) >> 2);
    }
  else if (strcmp (pfd->name, "im5_m") == 0)
    {
      new_value = ((int_value & 0x1C) >> 2);
    }
  /* p = 0.bit[1:0], r = 0.bit[3:2].  */
  /* q = 1.bit[1:0], s = 1.bit[5:4].  */
  else if (strcmp (pfd->name, "im6_iq") == 0)
    {
      new_value |= 0x04;
    }
  else if (strcmp (pfd->name, "im6_ms") == 0)
    {
      new_value |= 0x04;
    }
  /*  Rt CONCAT(c, t21, t0).  */
  else if (strcmp (pfd->name, "a_rt21") == 0)
    {
      new_value = (insn & 0x00000020) >> 5;
      new_value |= (insn & 0x00000C00) >> 9;
      new_value |= (insn & 0x00008000) >> 12;
    }
  else if (strcmp (pfd->name, "a_rte") == 0)
    {
      new_value = (insn & 0x00000C00) >> 9;
      new_value |= (insn & 0x00008000) >> 12;
    }
  else if (strcmp (pfd->name, "a_rte1") == 0)
    {
      new_value = (insn & 0x00000C00) >> 9;
      new_value |= (insn & 0x00008000) >> 12;
      new_value |= 0x01;
    }
  else if (strcmp (pfd->name, "a_rte69") == 0)
    {
      new_value = int_value << 1;
    }
  else if (strcmp (pfd->name, "a_rte69_1") == 0)
    {
      new_value = int_value << 1;
      new_value |= 0x01;
    }

  psys_reg = nds32_find_reg_keyword (psys_reg, new_value);
  if (!psys_reg)
    func (stream, "???");
  else
    func (stream, "$%s", psys_reg->name);
}

/* Dump instruction.  If the opcode is unknown, return FALSE.  */

static void
nds32_parse_opcode (struct nds32_opcode *opc, bfd_vma pc ATTRIBUTE_UNUSED,
		    disassemble_info *info, uint32_t insn,
		    uint32_t parse_mode)
{
  int op = 0;
  fprintf_ftype func = info->fprintf_func;
  void *stream = info->stream;
  const char *pstr_src;
  char *pstr_tmp;
  char tmp_string[16];
  unsigned int push25gpr = 0, lsmwRb, lsmwRe, lsmwEnb4, checkbit, i;
  int int_value, ifthe1st = 1;
  const field_t *pfd;
  keyword_t *psys_reg;

  if (opc == NULL)
    {
      func (stream, UNKNOWN_INSN_MSG);
      return;
    }

  if (parse_mode & NDS32_PARSE_EX9IT)
    func (stream, "		!");

  pstr_src = opc->instruction;
  if (*pstr_src == 0)
    {
      func (stream, "%s", opc->opcode);
      return;
    }
  /* NDS32_PARSE_INSN16.  */
  if (parse_mode & NDS32_PARSE_INSN16)
    {
      func (stream, "%s ", opc->opcode);
    }

  /* NDS32_PARSE_INSN32.  */
  else
    {
      op = N32_OP6 (insn);
      if (op == N32_OP6_LSMW)
	func (stream, "%s.", opc->opcode);
      else if (strstr (opc->instruction, "tito"))
	func (stream, "%s", opc->opcode);
      else
	func (stream, "%s\t", opc->opcode);
    }

  while (*pstr_src)
    {
      switch (*pstr_src)
	{
	case '%':
	case '=':
	case '&':
	  pstr_src++;
	  /* Compare with operand_fields[].name.  */
	  pstr_tmp = &tmp_string[0];
	  while (*pstr_src)
	    {
	      if ((*pstr_src == ',') || (*pstr_src == ' ')
		  || (*pstr_src == '{') || (*pstr_src == '}')
		  || (*pstr_src == '[') || (*pstr_src == ']')
		  || (*pstr_src == '(') || (*pstr_src == ')')
		  || (*pstr_src == '+') || (*pstr_src == '<'))
		break;
	      *pstr_tmp++ = *pstr_src++;
	    }
	  *pstr_tmp = 0;

	  pfd = (const field_t *) &operand_fields[0];
	  while (1)
	    {
	      if (pfd->name == NULL)
		return;
	      else if (strcmp (&tmp_string[0], pfd->name) == 0)
		break;
	      pfd++;
	    }

	  /* For insn-16.  */
	  if (parse_mode & NDS32_PARSE_INSN16)
	    {
	      if (pfd->hw_res == HW_GPR)
		{
		  int_value =
		    __GF (insn, pfd->bitpos, pfd->bitsize) << pfd->shift;
		  /* push25/pop25.  */
		  if ((opc->value == 0xfc00) || (opc->value == 0xfc80))
		    {
		      if (int_value == 0)
			int_value = 6;
		      else
			int_value = (6 + (0x01 << int_value));
		      push25gpr = int_value;
		    }
		  else if (strcmp (pfd->name, "rt4") == 0)
		    {
		      int_value = nds32_r45map[int_value];
		    }
		  func (stream, "$%s", keyword_gpr[int_value].name);
		}
	      else if ((pfd->hw_res == HW_INT) || (pfd->hw_res == HW_UINT))
		{
		  if (pfd->hw_res == HW_INT)
		    int_value =
		      N32_IMMS ((insn >> pfd->bitpos),
			    pfd->bitsize) << pfd->shift;
		  else
		    int_value =
		      __GF (insn, pfd->bitpos, pfd->bitsize) << pfd->shift;

		  /* movpi45.  */
		  if (opc->value == 0xfa00)
		    {
		      int_value += 16;
		      func (stream, "#0x%x", int_value);
		    }
		  /* lwi45.fe.  */
		  else if (opc->value == 0xb200)
		    {
		      int_value = 0 - (128 - int_value);
		      func (stream, "#%d", int_value);
		    }
		  /* beqz38/bnez38/beqs38/bnes38/j8/beqzs8/bnezs8/ifcall9.  */
		  else if ((opc->value == 0xc000) || (opc->value == 0xc800)
			   || (opc->value == 0xd000) || (opc->value == 0xd800)
			   || (opc->value == 0xd500) || (opc->value == 0xe800)
			   || (opc->value == 0xe900)
			   || (opc->value == 0xf800))
		    {
		      info->print_address_func (int_value + pc, info);
		    }
		  /* push25/pop25.  */
		  else if ((opc->value == 0xfc00) || (opc->value == 0xfc80))
		    {
		      func (stream, "#%d    ! {$r6", int_value);
		      if (push25gpr != 6)
			func (stream, "~$%s", keyword_gpr[push25gpr].name);
		      func (stream, ", $fp, $gp, $lp}");
		    }
		  /* ex9.it.  */
		  else if ((opc->value == 0xdd40) || (opc->value == 0xea00))
		    {
		      func (stream, "#%d", int_value);
		      nds32_ex9_info (pc, info, int_value);
		    }
		  else if (pfd->hw_res == HW_INT)
		    {
		      if (int_value < 10)
			func (stream, "#%d", int_value);
		      else
			func (stream, "#0x%x", int_value);
		    }
		  else /* if (pfd->hw_res == HW_UINT).  */
		    {
		      if (int_value < 10)
			func (stream, "#%u", int_value);
		      else
			func (stream, "#0x%x", int_value);
		    }
		}

	    }
	  /* for audio-ext.  */
	  else if (op == N32_OP6_AEXT)
	    {
	      nds32_parse_audio_ext (pfd, info, insn);
	    }
	  /* for insn-32.  */
	  else if (pfd->hw_res < _HW_LAST)
	    {
	      int_value =
		__GF (insn, pfd->bitpos, pfd->bitsize) << pfd->shift;

	      psys_reg = (keyword_t*) keywords[pfd->hw_res];

	      psys_reg = nds32_find_reg_keyword (psys_reg, int_value);
	      /* For HW_SR, dump the index when it can't
		 map the register name.  */
	      if (!psys_reg && pfd->hw_res == HW_SR)
		func (stream, "%d", int_value);
	      else if (!psys_reg)
		func (stream, "???");
	      else
		{
		  if (pfd->hw_res == HW_GPR || pfd->hw_res == HW_CPR
		      || pfd->hw_res == HW_FDR || pfd->hw_res == HW_FSR
		      || pfd->hw_res == HW_DXR || pfd->hw_res == HW_SR
		      || pfd->hw_res == HW_USR)
		    func (stream, "$%s", psys_reg->name);
		  else if (pfd->hw_res == HW_DTITON
			   || pfd->hw_res == HW_DTITOFF)
		    func (stream, ".%s", psys_reg->name);
		  else
		    func (stream, "%s", psys_reg->name);
		}
	    }
	  else if ((pfd->hw_res == HW_INT) || (pfd->hw_res == HW_UINT))
	    {
	      if (pfd->hw_res == HW_INT)
		int_value =
		  N32_IMMS ((insn >> pfd->bitpos), pfd->bitsize) << pfd->shift;
	      else
		int_value =
		  __GF (insn, pfd->bitpos, pfd->bitsize) << pfd->shift;

	      if ((op == N32_OP6_BR1) || (op == N32_OP6_BR2))
		{
		  info->print_address_func (int_value + pc, info);
		}
	      else if ((op == N32_OP6_BR3) && (pfd->bitpos == 0))
		{
		  info->print_address_func (int_value + pc, info);
		}
	      else if (op == N32_OP6_JI)
		{
		  /* FIXME: Handle relocation.  */
		  if (info->flags & INSN_HAS_RELOC)
		    pc = 0;
		  /* Check if insn32 in ex9 table.  */
		  if (parse_mode & NDS32_PARSE_EX9IT)
		    info->print_address_func ((pc & 0xFE000000) | int_value,
					      info);
		  /* Check if decode ex9 table,  PC(31,25)|Inst(23,0)<<1.  */
		  else if (parse_mode & NDS32_PARSE_EX9TAB)
		    func (stream, "PC(31,25)|#0x%x", int_value);
		  else
		    info->print_address_func (int_value + pc, info);
		}
	      else if (op == N32_OP6_LSMW)
		{
		  /* lmw.adm/smw.adm.  */
		  func (stream, "#0x%x    ! {", int_value);
		  lsmwEnb4 = int_value;
		  lsmwRb = ((insn >> 20) & 0x1F);
		  lsmwRe = ((insn >> 10) & 0x1F);

		  /* If [Rb, Re] specifies at least one register,
		     Rb(4,0) <= Re(4,0) and 0 <= Rb(4,0), Re(4,0) < 28.
		     Disassembling does not consider this currently because of
		     the convience comparing with bsp320.  */
		  if (lsmwRb != 31 || lsmwRe != 31)
		    {
		      func (stream, "$%s", keyword_gpr[lsmwRb].name);
		      if (lsmwRb != lsmwRe)
			func (stream, "~$%s", keyword_gpr[lsmwRe].name);
		      ifthe1st = 0;
		    }
		  if (lsmwEnb4 != 0)
		    {
		      /* $fp, $gp, $lp, $sp.  */
		      checkbit = 0x08;
		      for (i = 0; i < 4; i++)
			{
			  if (lsmwEnb4 & checkbit)
			    {
			      if (ifthe1st == 1)
				{
				  ifthe1st = 0;
				  func (stream, "$%s", keyword_gpr[28 + i].name);
				}
			      else
				func (stream, ", $%s", keyword_gpr[28 + i].name);
			    }
			  checkbit >>= 1;
			}
		    }
		  func (stream, "}");
		}
	      else if (pfd->hw_res == HW_INT)
		{
		  if (int_value < 10)
		    func (stream, "#%d", int_value);
		  else
		    func (stream, "#0x%x", int_value);
		}
	      else /* if (pfd->hw_res == HW_UINT).  */
		{
		  if (int_value < 10)
		    func (stream, "#%u", int_value);
		  else
		    func (stream, "#0x%x", int_value);
		}
	    }
	  break;

	case '{':
	case '}':
	  pstr_src++;
	  break;

	case ',':
	  func (stream, ", ");
	  pstr_src++;
	  break;
	  
	case '+':
	  func (stream, " + ");
	  pstr_src++;
	  break;
	  
	case '<':
	  if (pstr_src[1] == '<')
	    {
	      func (stream, " << ");
	      pstr_src += 2;
	    }
	  else
	    {
	      func (stream, " <");
	      pstr_src++;
	    }
	  break;
	  
	default:
	  func (stream, "%c", *pstr_src++);
	  break;
	}
    }
}

/* Filter instructions with some bits must be fixed.  */

static void
nds32_filter_unknown_insn (uint32_t insn, struct nds32_opcode **opc)
{
  if (!(*opc))
    return;

  switch ((*opc)->value)
    {
    case JREG (JR):
    case JREG (JRNEZ):
      /* jr jr.xtoff */
      if (__GF (insn, 6, 2) != 0 || __GF (insn, 15, 10) != 0)
        *opc = NULL;
      break;
    case MISC (STANDBY):
      if (__GF (insn, 7, 18) != 0)
        *opc = NULL;
      break;
    case SIMD (PBSAD):
    case SIMD (PBSADA):
      if (__GF (insn, 5, 5) != 0)
        *opc = NULL;
      break;
    case BR2 (IFCALL):
      if (__GF (insn, 20, 5) != 0)
        *opc = NULL;
      break;
    case JREG (JRAL):
      if (__GF (insn, 5, 3) != 0 || __GF (insn, 15, 5) != 0)
        *opc = NULL;
      break;
    case ALU1 (NOR):
    case ALU1 (SLT):
    case ALU1 (SLTS):
    case ALU1 (SLLI):
    case ALU1 (SRLI):
    case ALU1 (SRAI):
    case ALU1 (ROTRI):
    case ALU1 (SLL):
    case ALU1 (SRL):
    case ALU1 (SRA):
    case ALU1 (ROTR):
    case ALU1 (SEB):
    case ALU1 (SEH):
    case ALU1 (ZEH):
    case ALU1 (WSBH):
    case ALU1 (SVA):
    case ALU1 (SVS):
    case ALU1 (CMOVZ):
    case ALU1 (CMOVN):
      if (__GF (insn, 5, 5) != 0)
        *opc = NULL;
      break;
    case MISC (IRET):
    case MISC (ISB):
    case MISC (DSB):
      if (__GF (insn, 5, 20) != 0)
        *opc = NULL;
      break;
    }
}

static void
print_insn32 (bfd_vma pc, disassemble_info *info, uint32_t insn,
	      uint32_t parse_mode)
{
  /* Get the final correct opcode and parse.  */
  struct nds32_opcode *opc;
  uint32_t opcode = nds32_mask_opcode (insn);
  opc = (struct nds32_opcode *) htab_find (opcode_htab, &opcode);

  nds32_special_opcode (insn, &opc);
  nds32_filter_unknown_insn (insn, &opc);
  nds32_parse_opcode (opc, pc, info, insn, parse_mode);
}

static void
print_insn16 (bfd_vma pc, disassemble_info *info,
	      uint32_t insn, uint32_t parse_mode)
{
  struct nds32_opcode *opc;
  uint32_t opcode;

  /* Get highest 7 bit in default.  */
  unsigned int mask = 0xfe00;

  /* Classify 16-bit instruction to 4 sets by bit 13 and 14.  */
  switch (__GF (insn, 13, 2))
    {
    case 0x0:
      /* mov55 movi55 */
      if (__GF (insn, 11, 2) == 0)
	{
	  mask = 0xfc00;
	  /* ifret16 = mov55 $sp, $sp*/
	  if (__GF (insn, 0, 11) == 0x3ff)
	    mask = 0xffff;
	}
      else if (__GF (insn, 9, 4) == 0xb)
	mask = 0xfe07;
      break;
    case 0x1:
      /* lwi37 swi37 */
      if (__GF (insn, 11, 2) == 0x3)
	mask = 0xf880;
      break;
    case 0x2:
      mask = 0xf800;
      /* Exclude beqz38, bnez38, beqs38, and bnes38.  */
      if (__GF (insn, 12, 1) == 0x1
	  && __GF (insn, 8, 3) == 0x5)
	{
	  if (__GF (insn, 11, 1) == 0x0)
	    mask = 0xff00;
	  else
	    mask = 0xffe0;
	}
      break;
    case 0x3:
      switch (__GF (insn, 11, 2))
	{
	case 0x1:
	  /* beqzs8 bnezs8 */
	  if (__GF (insn, 9, 2) == 0x0)
	    mask = 0xff00;
	  /* addi10s */
	  else if (__GF(insn, 10, 1) == 0x1)
	    mask = 0xfc00;
	  break;
	case 0x2:
	  /* lwi37.sp swi37.sp */
	  mask = 0xf880;
	  break;
	case 0x3:
	  if (__GF (insn, 8, 3) == 0x5)
	    mask = 0xff00;
	  else if (__GF (insn, 8, 3) == 0x4)
	    mask = 0xff80;
	  else if (__GF (insn, 9 , 2) == 0x3)
	    mask = 0xfe07;
	  break;
	}
      break;
    }
  opcode = insn & mask;
  opc = (struct nds32_opcode *) htab_find (opcode_htab, &opcode);

  nds32_special_opcode (insn, &opc);
  /* Get the final correct opcode and parse it.  */
  nds32_parse_opcode (opc, pc, info, insn, parse_mode);
}

static hashval_t
htab_hash_hash (const void *p)
{
  return (*(unsigned int *) p) % 49;
}

static int
htab_hash_eq (const void *p, const void *q)
{
  uint32_t pinsn = ((struct nds32_opcode *) p)->value;
  uint32_t qinsn = *((uint32_t *) q);

  return (pinsn == qinsn);
}

/* Get the format of instruction.  */

static uint32_t
nds32_mask_opcode (uint32_t insn)
{
  uint32_t opcode = N32_OP6 (insn);
  switch (opcode)
    {
    case N32_OP6_LBI:
    case N32_OP6_LHI:
    case N32_OP6_LWI:
    case N32_OP6_LDI:
    case N32_OP6_LBI_BI:
    case N32_OP6_LHI_BI:
    case N32_OP6_LWI_BI:
    case N32_OP6_LDI_BI:
    case N32_OP6_SBI:
    case N32_OP6_SHI:
    case N32_OP6_SWI:
    case N32_OP6_SDI:
    case N32_OP6_SBI_BI:
    case N32_OP6_SHI_BI:
    case N32_OP6_SWI_BI:
    case N32_OP6_SDI_BI:
    case N32_OP6_LBSI:
    case N32_OP6_LHSI:
    case N32_OP6_LWSI:
    case N32_OP6_LBSI_BI:
    case N32_OP6_LHSI_BI:
    case N32_OP6_LWSI_BI:
    case N32_OP6_MOVI:
    case N32_OP6_SETHI:
    case N32_OP6_ADDI:
    case N32_OP6_SUBRI:
    case N32_OP6_ANDI:
    case N32_OP6_XORI:
    case N32_OP6_ORI:
    case N32_OP6_SLTI:
    case N32_OP6_SLTSI:
    case N32_OP6_CEXT:
    case N32_OP6_BITCI:
      return MASK_OP (insn, 0);
    case N32_OP6_ALU2:
      /* FFBI */
      if (__GF (insn, 0, 7) == (N32_ALU2_FFBI | __BIT (6)))
	return MASK_OP (insn, 0x7f);
      else if (__GF (insn, 0, 7) == (N32_ALU2_MFUSR | __BIT (6))
	       || __GF (insn, 0, 7) == (N32_ALU2_MTUSR | __BIT (6)))
	/* RDOV CLROV */
	return MASK_OP (insn, 0xf81ff);
      return MASK_OP (insn, 0x1ff);
    case N32_OP6_ALU1:
    case N32_OP6_SIMD:
      return MASK_OP (insn, 0x1f);
    case N32_OP6_MEM:
      return MASK_OP (insn, 0xff);
    case N32_OP6_JREG:
      return MASK_OP (insn, 0x7f);
    case N32_OP6_LSMW:
      return MASK_OP (insn, 0x23);
    case N32_OP6_SBGP:
    case N32_OP6_LBGP:
      return MASK_OP (insn, 0x1 << 19);
    case N32_OP6_HWGP:
      if (__GF (insn, 18, 2) == 0x3)
	return MASK_OP (insn, 0x7 << 17);
      return MASK_OP (insn, 0x3 << 18);
    case N32_OP6_DPREFI:
      return MASK_OP (insn, 0x1 << 24);
    case N32_OP6_LWC:
    case N32_OP6_SWC:
    case N32_OP6_LDC:
    case N32_OP6_SDC:
      return MASK_OP (insn, 0x1 << 12);
    case N32_OP6_JI:
      return MASK_OP (insn, 0x1 << 24);
    case N32_OP6_BR1:
      return MASK_OP (insn, 0x1 << 14);
    case N32_OP6_BR2:
      return MASK_OP (insn, 0xf << 16);
    case N32_OP6_BR3:
      return MASK_OP (insn, 0x1 << 19);
    case N32_OP6_MISC:
      switch (__GF (insn, 0, 5))
	{
	case N32_MISC_MTSR:
	  /* SETGIE and SETEND  */
	  if (__GF (insn, 5, 5) == 0x1 || __GF (insn, 5, 5) == 0x2)
	    return MASK_OP (insn, 0x1fffff);
	  return MASK_OP (insn, 0x1f);
	case N32_MISC_TLBOP:
	  if (__GF (insn, 5, 5) == 5 || __GF (insn, 5, 5) == 7)
	    /* PB FLUA  */
	    return MASK_OP (insn, 0x3ff);
	  return MASK_OP (insn, 0x1f);
	default:
	  return MASK_OP (insn, 0x1f);
	}
    case N32_OP6_COP:
      if (__GF (insn, 4, 2) == 0)
	{
	  /* FPU */
	  switch (__GF (insn, 0, 4))
	    {
	    case 0x0:
	    case 0x8:
	      /* FS1/F2OP FD1/F2OP */
	      if (__GF (insn, 6, 4) == 0xf)
		return MASK_OP (insn, 0x7fff);
	      /* FS1 FD1 */
	      return MASK_OP (insn, 0x3ff);
	    case 0x4:
	    case 0xc:
	      /* FS2 */
	      return MASK_OP (insn, 0x3ff);
	    case 0x1:
	    case 0x9:
	      /* XR */
	      if (__GF (insn, 6, 4) == 0xc)
		return MASK_OP (insn, 0x7fff);
	      /* MFCP MTCP */
	      return MASK_OP (insn, 0x3ff);
	    default:
	      return MASK_OP (insn, 0xff);
	    }
	}
      else if  (__GF (insn, 0, 2) == 0)
	return MASK_OP (insn, 0xf);
      return MASK_OP (insn, 0xcf);
    case N32_OP6_AEXT:
      /* AUDIO */
      switch (__GF (insn, 23, 2))
	{
	case 0x0:
	  if (__GF (insn, 5, 4) == 0)
	    /* AMxxx AMAyyS AMyyS AMAWzS AMWzS */
	    return MASK_OP (insn, (0x1f << 20) | 0x1ff);
	  else if (__GF (insn, 5, 4) == 1)
	    /* ALR ASR ALA ASA AUPI */
	    return MASK_OP (insn, (0x1f << 20) | (0xf << 5));
	  else if (__GF (insn, 20, 3) == 0 && __GF (insn, 6, 3) == 1)
	    /* ALR2 */
	    return MASK_OP (insn, (0x1f << 20) | (0x7 << 6));
	  else if (__GF (insn, 20 ,3) == 2 && __GF (insn, 6, 3) == 1)
	    /* AWEXT ASATS48 */
	    return MASK_OP (insn, (0x1f << 20) | (0xf << 5));
	  else if (__GF (insn, 20 ,3) == 3 && __GF (insn, 6, 3) == 1)
	    /* AMTAR AMTAR2 AMFAR AMFAR2 */
	    return MASK_OP (insn, (0x1f << 20) | (0x1f << 5));
	  else if (__GF (insn, 7, 2) == 3)
	    /* AMxxxSA */
	    return MASK_OP (insn, (0x1f << 20) | (0x3 << 7));
	  else if (__GF (insn, 6, 3) == 2)
	    /* AMxxxL.S  */
	    return MASK_OP (insn, (0x1f << 20) | (0xf << 5));
	  else
	    /* AmxxxL.l AmxxxL2.S AMxxxL2.L  */
	    return MASK_OP (insn, (0x1f << 20) | (0x7 << 6));
	case 0x1:
	  if (__GF (insn, 20, 3) == 0)
	    /* AADDL ASUBL */
	    return MASK_OP (insn, (0x1f << 20) | (0x1 << 5));
	  else if (__GF (insn, 20, 3) == 1)
	    /* AMTARI Ix AMTARI Mx */
	    return MASK_OP (insn, (0x1f << 20));
	  else if (__GF (insn, 6, 3) == 2)
	    /* AMAWzSl.S AMWzSl.S */
	    return MASK_OP (insn, (0x1f << 20) | (0xf << 5));
	  else if (__GF (insn, 7, 2) == 3)
	    /* AMAWzSSA AMWzSSA */
	    return MASK_OP (insn, (0x1f << 20) | (0x3 << 7));
	  else
	    /* AMAWzSL.L AMAWzSL2.S AMAWzSL2.L AMWzSL.L AMWzSL.L AMWzSL2.S */
	    return MASK_OP (insn, (0x1f << 20) | (0x7 << 6));
	case 0x2:
	  if (__GF (insn, 6, 3) == 2)
	    /* AMAyySl.S AMWyySl.S */
	    return MASK_OP (insn, (0x1f << 20) | (0xf << 5));
	  else if (__GF (insn, 7, 2) == 3)
	    /* AMAWyySSA AMWyySSA */
	    return MASK_OP (insn, (0x1f << 20) | (0x3 << 7));
	  else
	    /* AMAWyySL.L AMAWyySL2.S AMAWyySL2.L AMWyySL.L AMWyySL.L AMWyySL2.S */
	    return MASK_OP (insn, (0x1f << 20) | (0x7 << 6));
	}
      return MASK_OP (insn, 0x1f << 20);
    default:
      return (1 << 31);
    }
}

/* Define cctl subtype.  */
static char *cctl_subtype [] =
{
  /* 0x0 */
  "st0", "st0", "st0", "st2", "st2", "st3", "st3", "st4",
  "st1", "st1", "st1", "st0", "st0", NULL, NULL, "st5",
  /* 0x10 */
  "st0", NULL, NULL, "st2", "st2", "st3", "st3", NULL,
  "st1", NULL, NULL, "st0", "st0", NULL, NULL, NULL
};

/* Check the subset of opcode.  */

static void
nds32_special_opcode (uint32_t insn, struct nds32_opcode **opc)
{
  char *string = NULL;
  uint32_t op;

  if (!(*opc))
    return;

  /* Check if special case.  */
  switch ((*opc)->value)
    {
    case OP6 (LWC):
    case OP6 (SWC):
    case OP6 (LDC):
    case OP6 (SDC):
    case FPU_RA_IMMBI (LWC):
    case FPU_RA_IMMBI (SWC):
    case FPU_RA_IMMBI (LDC):
    case FPU_RA_IMMBI (SDC):
      /* Check if cp0 => FPU.  */
      if (__GF (insn, 13, 2) == 0)
      {
	while (!((*opc)->attr & ATTR (FPU)) && (*opc)->next)
	  *opc = (*opc)->next;
      }
      break;
    case ALU1 (ADD):
    case ALU1 (SUB):
    case ALU1 (AND):
    case ALU1 (XOR):
    case ALU1 (OR):
      /* Check if (add/add_slli) (sub/sub_slli) (and/and_slli).  */
      if (N32_SH5(insn) != 0)
        string = "sh";
      break;
    case ALU1 (SRLI):
      /* Check if nop.  */
      if (__GF (insn, 10, 15) == 0)
        string = "nop";
      break;
    case MISC (CCTL):
      string = cctl_subtype [__GF (insn, 5, 5)];
      break;
    case JREG (JR):
    case JREG (JRAL):
    case JREG (JR) | JREG_RET:
      if (__GF (insn, 8, 2) != 0)
	string = "tit";
    break;
    case N32_OP6_COP:
    break;
    case 0xea00:
      /* break16 ex9 */
      if (__GF (insn, 5, 4) != 0)
	string = "ex9";
      break;
    case 0x9200:
      /* nop16 */
      if (__GF (insn, 0, 9) == 0)
	string = "nop16";
      break;
    }

  if (string)
    {
      while (strstr ((*opc)->opcode, string) == NULL
	     && strstr ((*opc)->instruction, string) == NULL && (*opc)->next)
	*opc = (*opc)->next;
      return;
    }

  /* Classify instruction is COP or FPU.  */
  op = N32_OP6 (insn);
  if (op == N32_OP6_COP && __GF (insn, 4, 2) != 0)
    {
      while (((*opc)->attr & ATTR (FPU)) != 0 && (*opc)->next)
	*opc = (*opc)->next;
    }
}

int
print_insn_nds32 (bfd_vma pc, disassemble_info *info)
{
  int status;
  bfd_byte buf[4];
  uint32_t insn;
  static int init = 1;
  int i = 0;
  struct nds32_opcode *opc;
  struct nds32_opcode **slot;

  if (init)
    {
      /* Build opcode table.  */
      opcode_htab = htab_create_alloc (1024, htab_hash_hash, htab_hash_eq,
				       NULL, xcalloc, free);

      while (nds32_opcodes[i].opcode != NULL)
	{
	  opc = &nds32_opcodes[i];
	  slot =
	    (struct nds32_opcode **) htab_find_slot (opcode_htab, &opc->value,
						     INSERT);
	  if (*slot == NULL)
	    {
	      /* This is the new one.  */
	      *slot = opc;
	    }
	  else
	    {
	      /* Already exists.  Append to the list.  */
	      opc = *slot;
	      while (opc->next)
		opc = opc->next;
	      opc->next = &nds32_opcodes[i];
	    }
	  i++;
	}
      init = 0;
    }

  status = info->read_memory_func (pc, (bfd_byte *) buf, 4, info);
  if (status)
    {
      /* for the last 16-bit instruction.  */
      status = info->read_memory_func (pc, (bfd_byte *) buf, 2, info);
      if (status)
	{
	  (*info->memory_error_func)(status, pc, info);
	  return -1;
	}
    }

  insn = bfd_getb32 (buf);
  /* 16-bit instruction.  */
  if (insn & 0x80000000)
    {
      if (info->section && strstr (info->section->name, ".ex9.itable") != NULL)
	{
	  print_insn16 (pc, info, (insn & 0x0000FFFF),
			NDS32_PARSE_INSN16 | NDS32_PARSE_EX9TAB);
	  return 4;
	}
      print_insn16 (pc, info, (insn >> 16), NDS32_PARSE_INSN16);
      return 2;
    }

  /* 32-bit instructions.  */
  else
    {
      if (info->section
	  && strstr (info->section->name, ".ex9.itable") != NULL)
	print_insn32 (pc, info, insn, NDS32_PARSE_INSN32 | NDS32_PARSE_EX9TAB);
      else
	print_insn32 (pc, info, insn, NDS32_PARSE_INSN32);
      return 4;
    }
}
