/* TI C6X disassembler.
   Copyright 2010
   Free Software Foundation, Inc.
   Contributed by Joseph Myers <joseph@codesourcery.com>
   		  Bernd Schmidt  <bernds@codesourcery.com>

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "dis-asm.h"
#include "opcode/tic6x.h"
#include "libiberty.h"

/* Define the instruction format table.  */
const tic6x_insn_format tic6x_insn_format_table[tic6x_insn_format_max] =
  {
#define FMT(name, num_bits, cst_bits, mask, fields) \
    { num_bits, cst_bits, mask, fields },
#include "opcode/tic6x-insn-formats.h"
#undef FMT
  };

/* Define the control register table.  */
const tic6x_ctrl tic6x_ctrl_table[tic6x_ctrl_max] =
  {
#define CTRL(name, isa, rw, crlo, crhi_mask)	\
    {						\
      STRINGX(name),				\
      CONCAT2(TIC6X_INSN_,isa),			\
      CONCAT2(tic6x_rw_,rw),			\
      crlo,					\
      crhi_mask					\
    },
#include "opcode/tic6x-control-registers.h"
#undef CTRL
  };

/* Define the opcode table.  */
const tic6x_opcode tic6x_opcode_table[tic6x_opcode_max] =
  {
#define INSN(name, func_unit, format, type, isa, flags, fixed, ops, var) \
    {									\
      STRINGX(name),							\
      CONCAT2(tic6x_func_unit_,func_unit),				\
      CONCAT4(tic6x_insn_format_,func_unit,_,format),			\
      CONCAT2(tic6x_pipeline_,type),					\
      CONCAT2(TIC6X_INSN_,isa),						\
      flags,								\
      fixed,								\
      ops,								\
      var								\
    },
#define INSNE(name, e, func_unit, format, type, isa, flags, fixed, ops, var) \
    {									\
      STRINGX(name),							\
      CONCAT2(tic6x_func_unit_,func_unit),				\
      CONCAT4(tic6x_insn_format_,func_unit,_,format),			\
      CONCAT2(tic6x_pipeline_,type),					\
      CONCAT2(TIC6X_INSN_,isa),						\
      flags,								\
      fixed,								\
      ops,								\
      var								\
    },
#include "opcode/tic6x-opcode-table.h"
#undef INSN
#undef INSNE
  };

/* If instruction format FMT has a field FIELD, return a pointer to
   the description of that field; otherwise return NULL.  */

const tic6x_insn_field *
tic6x_field_from_fmt (const tic6x_insn_format *fmt, tic6x_insn_field_id field)
{
  unsigned int f;

  for (f = 0; f < fmt->num_fields; f++)
    if (fmt->fields[f].field_id == field)
      return &fmt->fields[f];

  return NULL;
}

/* Extract the bits corresponding to FIELD from OPCODE.  */

static unsigned int
tic6x_field_bits (unsigned int opcode, const tic6x_insn_field *field)
{
  return (opcode >> field->low_pos) & ((1u << field->width) - 1);
}

/* Extract a 32-bit value read from the instruction stream.  */

static unsigned int
tic6x_extract_32 (unsigned char *p, struct disassemble_info *info)
{
  if (info->endian == BFD_ENDIAN_LITTLE)
    return (p[0]) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
  else
    return (p[3]) | (p[2] << 8) | (p[1] << 16) | (p[0] << 24);
}

/* Extract a 16-bit value read from the instruction stream.  */

static unsigned int
tic6x_extract_16 (unsigned char *p, struct disassemble_info *info)
{
  if (info->endian == BFD_ENDIAN_LITTLE)
    return (p[0]) | (p[1] << 8);
  else
    return (p[1]) | (p[0] << 8);
}

/* FP points to a fetch packet.  Return whether it is header-based; if
   it is, fill in HEADER.  */

static bfd_boolean
tic6x_check_fetch_packet_header (unsigned char *fp,
				 tic6x_fetch_packet_header *header,
				 struct disassemble_info *info)
{
  int i;

  header->header = tic6x_extract_32 (fp + 28, info);
  if ((header->header & 0xf0000000) != 0xe0000000)
    return FALSE;

  for (i = 0; i < 7; i++)
    header->word_compact[i]
      = (header->header & (1u << (21 + i))) ? TRUE : FALSE;

  header->prot = (header->header & (1u << 20)) ? TRUE : FALSE;
  header->rs = (header->header & (1u << 19)) ? TRUE : FALSE;
  header->dsz = (header->header >> 16) & 0x7;
  header->br = (header->header & (1u << 15)) ? TRUE : FALSE;
  header->sat = (header->header & (1u << 14)) ? TRUE : FALSE;

  for (i = 0; i < 14; i++)
    header->p_bits[i]
      = (header->header & (1u << i)) ? TRUE : FALSE;

  return TRUE;
}

/* Disassemble the instruction at ADDR and print it using
   INFO->FPRINTF_FUNC and INFO->STREAM, returning the number of bytes
   consumed.  */

int
print_insn_tic6x (bfd_vma addr, struct disassemble_info *info)
{
  int status;
  bfd_vma fp_addr;
  bfd_vma fp_offset;
  unsigned char fp[32];
  unsigned int opcode;
  tic6x_opcode_id opcode_id;
  bfd_boolean fetch_packet_header_based;
  tic6x_fetch_packet_header header;
  unsigned int num_bits;
  bfd_boolean bad_offset = FALSE;

  fp_offset = addr & 0x1f;
  fp_addr = addr - fp_offset;
  status = info->read_memory_func (fp_addr, fp, 32, info);
  if (status)
    {
      info->memory_error_func (status, addr, info);
      return -1;
    }

  fetch_packet_header_based
    = tic6x_check_fetch_packet_header (fp, &header, info);
  if (fetch_packet_header_based)
    {
      if (fp_offset & 0x1)
	bad_offset = TRUE;
      if ((fp_offset & 0x3) && (fp_offset >= 28
				|| !header.word_compact[fp_offset >> 2]))
	bad_offset = TRUE;
      if (fp_offset == 28)
	{
	  info->bytes_per_chunk = 4;
	  info->fprintf_func (info->stream, "<fetch packet header 0x%.8x>",
			      header.header);
	  return 4;
	}
      num_bits = (header.word_compact[fp_offset >> 2] ? 16 : 32);
    }
  else
    {
      num_bits = 32;
      if (fp_offset & 0x3)
	bad_offset = TRUE;
    }

  if (bad_offset)
    {
      info->bytes_per_chunk = 1;
      info->fprintf_func (info->stream, ".byte 0x%.2x", fp[fp_offset]);
      return 1;
    }

  if (num_bits == 16)
    {
      /* The least-significant part of a 32-bit word comes logically
	 before the most-significant part.  For big-endian, follow the
	 TI assembler in showing instructions in logical order by
	 pretending that the two halves of the word are in opposite
	 locations to where they actually are.  */
      if (info->endian == BFD_ENDIAN_LITTLE)
	opcode = tic6x_extract_16 (fp + fp_offset, info);
      else
	opcode = tic6x_extract_16 (fp + (fp_offset ^ 2), info);
    }
  else
    opcode = tic6x_extract_32 (fp + fp_offset, info);

  for (opcode_id = 0; opcode_id < tic6x_opcode_max; opcode_id++)
    {
      const tic6x_opcode *const opc = &tic6x_opcode_table[opcode_id];
      const tic6x_insn_format *const fmt
	= &tic6x_insn_format_table[opc->format];
      const tic6x_insn_field *creg_field;
      bfd_boolean p_bit;
      const char *parallel;
      const char *cond = "";
      const char *func_unit;
      char func_unit_buf[7];
      unsigned int func_unit_side = 0;
      unsigned int func_unit_data_side = 0;
      unsigned int func_unit_cross = 0;
      /* The maximum length of the text of a non-PC-relative operand
	 is 24 bytes (SPMASK masking all eight functional units, with
	 separating commas and trailing NUL).  */
      char operands[TIC6X_MAX_OPERANDS][24] = { { 0 } };
      bfd_vma operands_addresses[TIC6X_MAX_OPERANDS] = { 0 };
      bfd_boolean operands_text[TIC6X_MAX_OPERANDS] = { FALSE };
      bfd_boolean operands_pcrel[TIC6X_MAX_OPERANDS] = { FALSE };
      unsigned int fix;
      unsigned int num_operands;
      unsigned int op_num;
      bfd_boolean fixed_ok;
      bfd_boolean operands_ok;

      if (opc->flags & TIC6X_FLAG_MACRO)
	continue;
      if (fmt->num_bits != num_bits)
	continue;
      if ((opcode & fmt->mask) != fmt->cst_bits)
	continue;

      /* If the format has a creg field, it is only a candidate for a
	 match if the creg and z fields have values indicating a valid
	 condition; reserved values indicate either an instruction
	 format without a creg field, or an invalid instruction.  */
      creg_field = tic6x_field_from_fmt (fmt, tic6x_field_creg);
      if (creg_field)
	{
	  const tic6x_insn_field *z_field;
	  unsigned int creg_value, z_value;
	  static const char *const conds[8][2] =
	    {
	      { "", NULL },
	      { "[b0] ", "[!b0] " },
	      { "[b1] ", "[!b1] " },
	      { "[b2] ", "[!b2] " },
	      { "[a1] ", "[!a1] " },
	      { "[a2] ", "[!a2] " },
	      { "[a0] ", "[!a0] " },
	      { NULL, NULL }
	    };

	  /* A creg field is not meaningful without a z field, so if
	     the z field is not present this is an error in the format
	     table.  */
	  z_field = tic6x_field_from_fmt (fmt, tic6x_field_z);
	  if (!z_field)
	    abort ();

	  creg_value = tic6x_field_bits (opcode, creg_field);
	  z_value = tic6x_field_bits (opcode, z_field);
	  cond = conds[creg_value][z_value];
	  if (cond == NULL)
	    continue;
	}

      /* All fixed fields must have matching values; all fields with
	 restricted ranges must have values within those ranges.  */
      fixed_ok = TRUE;
      for (fix = 0; fix < opc->num_fixed_fields; fix++)
	{
	  unsigned int field_bits;
	  const tic6x_insn_field *const field
	    = tic6x_field_from_fmt (fmt, opc->fixed_fields[fix].field_id);

	  if (!field)
	    abort ();
	  field_bits = tic6x_field_bits (opcode, field);
	  if (field_bits < opc->fixed_fields[fix].min_val
	      || field_bits > opc->fixed_fields[fix].max_val)
	    {
	      fixed_ok = FALSE;
	      break;
	    }
	}
      if (!fixed_ok)
	continue;

      /* The instruction matches.  */

      /* The p-bit indicates whether this instruction is in parallel
	 with the *next* instruction, whereas the parallel bars
	 indicate the instruction is in parallel with the *previous*
	 instruction.  Thus, we must find the p-bit for the previous
	 instruction.  */
      if (num_bits == 16 && (fp_offset & 0x2) == 2)
	{
	  /* This is the logically second (most significant; second in
	     fp_offset terms because fp_offset relates to logical not
	     physical addresses) instruction of a compact pair; find
	     the p-bit for the first (least significant).  */
	  p_bit = header.p_bits[(fp_offset >> 2) << 1];
	}
      else if (fp_offset >= 4)
	{
	  /* Find the last instruction of the previous word in this
	     fetch packet.  For compact instructions, this is the most
	     significant 16 bits.  */
	  if (fetch_packet_header_based
	      && header.word_compact[(fp_offset >> 2) - 1])
	    p_bit = header.p_bits[(fp_offset >> 1) - 1];
	  else
	    {
	      unsigned int prev_opcode
		= tic6x_extract_32 (fp + (fp_offset & 0x1c) - 4, info);
	      p_bit = (prev_opcode & 0x1) ? TRUE : FALSE;
	    }
	}
      else
	{
	  /* Find the last instruction of the previous fetch
	     packet.  */
	  unsigned char fp_prev[32];
	  status = info->read_memory_func (fp_addr - 32, fp_prev, 32, info);
	  if (status)
	    /* No previous instruction to be parallel with.  */
	    p_bit = FALSE;
	  else
	    {
	      bfd_boolean prev_header_based;
	      tic6x_fetch_packet_header prev_header;

	      prev_header_based
		= tic6x_check_fetch_packet_header (fp_prev, &prev_header, info);
	      if (prev_header_based && prev_header.word_compact[6])
		p_bit = prev_header.p_bits[13];
	      else
		{
		  unsigned int prev_opcode = tic6x_extract_32 (fp_prev + 28,
							       info);
		  p_bit = (prev_opcode & 0x1) ? TRUE : FALSE;
		}
	    }
	}
      parallel = p_bit ? "|| " : "";

      if (opc->func_unit == tic6x_func_unit_nfu)
	func_unit = "";
      else
	{
	  unsigned int fld_num;
	  char func_unit_char;
	  const char *data_str;
	  bfd_boolean have_areg = FALSE;
	  bfd_boolean have_cross = FALSE;

	  func_unit_side = (opc->flags & TIC6X_FLAG_SIDE_B_ONLY) ? 2 : 0;
	  func_unit_cross = 0;
	  func_unit_data_side = (opc->flags & TIC6X_FLAG_SIDE_T2_ONLY) ? 2 : 0;

	  for (fld_num = 0; fld_num < opc->num_variable_fields; fld_num++)
	    {
	      const tic6x_coding_field *const enc = &opc->variable_fields[fld_num];
	      const tic6x_insn_field *field;
	      unsigned int fld_val;

	      field = tic6x_field_from_fmt (fmt, enc->field_id);
	      if (!field)
		abort ();
	      fld_val = tic6x_field_bits (opcode, field);
	      switch (enc->coding_method)
		{
		case tic6x_coding_fu:
		  /* The side must be specified exactly once.  */
		  if (func_unit_side)
		    abort ();
		  func_unit_side = (fld_val ? 2 : 1);
		  break;

		case tic6x_coding_data_fu:
		  /* The data side must be specified exactly once.  */
		  if (func_unit_data_side)
		    abort ();
		  func_unit_data_side = (fld_val ? 2 : 1);
		  break;

		case tic6x_coding_xpath:
		  /* Cross path use must be specified exactly
		     once.  */
		  if (have_cross)
		    abort ();
		  have_cross = TRUE;
		  func_unit_cross = fld_val;
		  break;

		case tic6x_coding_areg:
		  have_areg = TRUE;
		  break;

		default:
		  /* Don't relate to functional units.  */
		  break;
		}
	    }

	  /* The side of the functional unit used must now have been
	     determined either from the flags or from an instruction
	     field.  */
	  if (func_unit_side != 1 && func_unit_side != 2)
	    abort ();

	  /* Cross paths are not applicable when sides are specified
	     for both address and data paths.  */
	  if (func_unit_data_side && have_cross)
	    abort ();

	  /* Separate address and data paths are only applicable for
	     the D unit.  */
	  if (func_unit_data_side && opc->func_unit != tic6x_func_unit_d)
	    abort ();

	  /* If an address register is being used but in ADDA rather
	     than a load or store, it uses a cross path for side-A
	     instructions, and the cross path use is not specified by
	     an instruction field.  */
	  if (have_areg && !func_unit_data_side)
	    {
	      if (have_cross)
		abort ();
	      func_unit_cross = (func_unit_side == 1 ? TRUE : FALSE);
	    }

	  switch (opc->func_unit)
	    {
	    case tic6x_func_unit_d:
	      func_unit_char = 'D';
	      break;

	    case tic6x_func_unit_l:
	      func_unit_char = 'L';
	      break;

	    case tic6x_func_unit_m:
	      func_unit_char = 'M';
	      break;

	    case tic6x_func_unit_s:
	      func_unit_char = 'S';
	      break;

	    default:
	      abort ();
	    }

	  switch (func_unit_data_side)
	    {
	    case 0:
	      data_str = "";
	      break;

	    case 1:
	      data_str = "T1";
	      break;

	    case 2:
	      data_str = "T2";
	      break;

	    default:
	      abort ();
	    }

	  snprintf (func_unit_buf, 7, " .%c%u%s%s", func_unit_char,
		    func_unit_side, (func_unit_cross ? "X" : ""), data_str);
	  func_unit = func_unit_buf;
	}

      /* For each operand there must be one or more fields set based
	 on that operand, that can together be used to derive the
	 operand value.  */
      operands_ok = TRUE;
      num_operands = opc->num_operands;
      for (op_num = 0; op_num < num_operands; op_num++)
	{
	  unsigned int fld_num;
	  unsigned int mem_base_reg = 0;
	  bfd_boolean mem_base_reg_known = FALSE;
	  bfd_boolean mem_base_reg_known_long = FALSE;
	  unsigned int mem_offset = 0;
	  bfd_boolean mem_offset_known = FALSE;
	  bfd_boolean mem_offset_known_long = FALSE;
	  unsigned int mem_mode = 0;
	  bfd_boolean mem_mode_known = FALSE;
	  unsigned int mem_scaled = 0;
	  bfd_boolean mem_scaled_known = FALSE;
	  unsigned int crlo = 0;
	  bfd_boolean crlo_known = FALSE;
	  unsigned int crhi = 0;
	  bfd_boolean crhi_known = FALSE;
	  bfd_boolean spmask_skip_operand = FALSE;
	  unsigned int fcyc_bits = 0;
	  bfd_boolean prev_sploop_found = FALSE;

	  switch (opc->operand_info[op_num].form)
	    {
	    case tic6x_operand_retreg:
	      /* Fully determined by the functional unit.  */
	      operands_text[op_num] = TRUE;
	      snprintf (operands[op_num], 24, "%c3",
			(func_unit_side == 2 ? 'b' : 'a'));
	      continue;

	    case tic6x_operand_irp:
	      operands_text[op_num] = TRUE;
	      snprintf (operands[op_num], 24, "irp");
	      continue;

	    case tic6x_operand_nrp:
	      operands_text[op_num] = TRUE;
	      snprintf (operands[op_num], 24, "nrp");
	      continue;

	    default:
	      break;
	    }

	  for (fld_num = 0; fld_num < opc->num_variable_fields; fld_num++)
	    {
	      const tic6x_coding_field *const enc
		= &opc->variable_fields[fld_num];
	      const tic6x_insn_field *field;
	      unsigned int fld_val;
	      signed int signed_fld_val;

	      if (enc->operand_num != op_num)
		continue;
	      field = tic6x_field_from_fmt (fmt, enc->field_id);
	      if (!field)
		abort ();
	      fld_val = tic6x_field_bits (opcode, field);
	      switch (enc->coding_method)
		{
		case tic6x_coding_ucst:
		case tic6x_coding_ulcst_dpr_byte:
		case tic6x_coding_ulcst_dpr_half:
		case tic6x_coding_ulcst_dpr_word:
		case tic6x_coding_lcst_low16:
		  switch (opc->operand_info[op_num].form)
		    {
		    case tic6x_operand_asm_const:
		    case tic6x_operand_link_const:
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%u", fld_val);
		      break;

		    case tic6x_operand_mem_long:
		      mem_offset = fld_val;
		      mem_offset_known_long = TRUE;
		      break;

		    default:
		      abort ();
		    }
		  break;

		case tic6x_coding_lcst_high16:
		  operands_text[op_num] = TRUE;
		  snprintf (operands[op_num], 24, "%u", fld_val << 16);
		  break;

		case tic6x_coding_scst:
		  operands_text[op_num] = TRUE;
		  signed_fld_val = (signed int) fld_val;
		  signed_fld_val ^= (1 << (field->width - 1));
		  signed_fld_val -= (1 << (field->width - 1));
		  snprintf (operands[op_num], 24, "%d", signed_fld_val);
		  break;

		case tic6x_coding_ucst_minus_one:
		  operands_text[op_num] = TRUE;
		  snprintf (operands[op_num], 24, "%u", fld_val + 1);
		  break;

		case tic6x_coding_pcrel:
		case tic6x_coding_pcrel_half:
		  signed_fld_val = (signed int) fld_val;
		  signed_fld_val ^= (1 << (field->width - 1));
		  signed_fld_val -= (1 << (field->width - 1));
		  if (fetch_packet_header_based
		      && enc->coding_method == tic6x_coding_pcrel_half)
		    signed_fld_val *= 2;
		  else
		    signed_fld_val *= 4;
		  operands_pcrel[op_num] = TRUE;
		  operands_addresses[op_num] = fp_addr + signed_fld_val;
		  break;

		case tic6x_coding_reg_shift:
		  fld_val <<= 1;
		  /* Fall through.  */
		case tic6x_coding_reg:
		  switch (opc->operand_info[op_num].form)
		    {
		    case tic6x_operand_reg:
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%c%u",
				(func_unit_side == 2 ? 'b' : 'a'), fld_val);
		      break;

		    case tic6x_operand_xreg:
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%c%u",
				(((func_unit_side == 2) ^ func_unit_cross)
				 ? 'b'
				 : 'a'), fld_val);
		      break;

		    case tic6x_operand_dreg:
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%c%u",
				(func_unit_data_side == 2 ? 'b' : 'a'),
				fld_val);
		      break;

		    case tic6x_operand_regpair:
		      operands_text[op_num] = TRUE;
		      if (fld_val & 1)
			operands_ok = FALSE;
		      snprintf (operands[op_num], 24, "%c%u:%c%u",
				(func_unit_side == 2 ? 'b' : 'a'), fld_val + 1,
				(func_unit_side == 2 ? 'b' : 'a'), fld_val);
		      break;

		    case tic6x_operand_xregpair:
		      operands_text[op_num] = TRUE;
		      if (fld_val & 1)
			operands_ok = FALSE;
		      snprintf (operands[op_num], 24, "%c%u:%c%u",
				(((func_unit_side == 2) ^ func_unit_cross)
				 ? 'b'
				 : 'a'), fld_val + 1,
				(((func_unit_side == 2) ^ func_unit_cross)
				 ? 'b'
				 : 'a'), fld_val);
		      break;

		    case tic6x_operand_dregpair:
		      operands_text[op_num] = TRUE;
		      if (fld_val & 1)
			operands_ok = FALSE;
		      snprintf (operands[op_num], 24, "%c%u:%c%u",
				(func_unit_data_side == 2 ? 'b' : 'a'),
				fld_val + 1,
				(func_unit_data_side == 2 ? 'b' : 'a'),
				fld_val);
		      break;

		    case tic6x_operand_mem_deref:
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "*%c%u",
				(func_unit_side == 2 ? 'b' : 'a'), fld_val);
		      break;

		    case tic6x_operand_mem_short:
		    case tic6x_operand_mem_ndw:
		      mem_base_reg = fld_val;
		      mem_base_reg_known = TRUE;
		      break;

		    default:
		      abort ();
		    }
		  break;

		case tic6x_coding_areg:
		  switch (opc->operand_info[op_num].form)
		    {
		    case tic6x_operand_areg:
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "b%u",
				fld_val ? 15u : 14u);
		      break;

		    case tic6x_operand_mem_long:
		      mem_base_reg = fld_val ? 15u : 14u;
		      mem_base_reg_known_long = TRUE;
		      break;

		    default:
		      abort ();
		    }
		  break;

		case tic6x_coding_mem_offset:
		case tic6x_coding_mem_offset_noscale:
		  mem_offset = fld_val;
		  mem_offset_known = TRUE;
		  break;

		case tic6x_coding_mem_mode:
		  mem_mode = fld_val;
		  mem_mode_known = TRUE;
		  break;

		case tic6x_coding_scaled:
		  mem_scaled = fld_val;
		  mem_scaled_known = TRUE;
		  break;

		case tic6x_coding_crlo:
		  crlo = fld_val;
		  crlo_known = TRUE;
		  break;

		case tic6x_coding_crhi:
		  crhi = fld_val;
		  crhi_known = TRUE;
		  break;

		case tic6x_coding_fstg:
		case tic6x_coding_fcyc:
		  if (!prev_sploop_found)
		    {
		      bfd_vma search_fp_addr = fp_addr;
		      bfd_vma search_fp_offset = fp_offset;
		      bfd_boolean search_fp_header_based
			= fetch_packet_header_based;
		      tic6x_fetch_packet_header search_fp_header = header;
		      unsigned char search_fp[32];
		      unsigned int search_num_bits;
		      unsigned int search_opcode;
		      unsigned int sploop_ii = 0;
		      int i;

		      memcpy (search_fp, fp, 32);

		      /* To interpret these bits in an SPKERNEL
			 instruction, we must find the previous
			 SPLOOP-family instruction.  It may come up to
			 48 execute packets earlier.  */
		      for (i = 0; i < 48 * 8; i++)
			{
			  /* Find the previous instruction.  */
			  if (search_fp_offset & 2)
			    search_fp_offset -= 2;
			  else if (search_fp_offset >= 4)
			    {
			      if (search_fp_header_based
				  && (search_fp_header.word_compact
				      [(search_fp_offset >> 2) - 1]))
				search_fp_offset -= 2;
			      else
				search_fp_offset -= 4;
			    }
			  else
			    {
			      search_fp_addr -= 32;
			      status = info->read_memory_func (search_fp_addr,
							       search_fp,
							       32, info);
			      if (status)
				/* No previous SPLOOP instruction.  */
				break;
			      search_fp_header_based
				= (tic6x_check_fetch_packet_header
				   (search_fp, &search_fp_header, info));
			      if (search_fp_header_based)
				search_fp_offset
				  = search_fp_header.word_compact[6] ? 26 : 24;
			      else
				search_fp_offset = 28;
			    }

			  /* Extract the previous instruction.  */
			  if (search_fp_header_based)
			    search_num_bits
			      = (search_fp_header.word_compact[search_fp_offset
							       >> 2]
				 ? 16
				 : 32);
			  else
			    search_num_bits = 32;
			  if (search_num_bits == 16)
			    {
			      if (info->endian == BFD_ENDIAN_LITTLE)
				search_opcode
				  = (tic6x_extract_16
				     (search_fp + search_fp_offset, info));
			      else
				search_opcode
				  = (tic6x_extract_16
				     (search_fp + (search_fp_offset ^ 2),
				      info));
			    }
			  else
			    search_opcode
			      = tic6x_extract_32 (search_fp + search_fp_offset,
						  info);

			  /* Check whether it is an SPLOOP-family
			     instruction.  */
			  if (search_num_bits == 32
			      && ((search_opcode & 0x003ffffe) == 0x00038000
				  || (search_opcode & 0x003ffffe) == 0x0003a000
				  || ((search_opcode & 0x003ffffe)
				      == 0x0003e000)))
			    {
			      prev_sploop_found = TRUE;
			      sploop_ii = ((search_opcode >> 23) & 0x1f) + 1;
			    }
			  else if (search_num_bits == 16
				   && (search_opcode & 0x3c7e) == 0x0c66)
			    {
			      prev_sploop_found = TRUE;
			      sploop_ii
				= (((search_opcode >> 7) & 0x7)
				   | ((search_opcode >> 11) & 0x8)) + 1;
			    }
			  if (prev_sploop_found)
			    {
			      if (sploop_ii <= 0)
				abort ();
			      else if (sploop_ii <= 1)
				fcyc_bits = 0;
			      else if (sploop_ii <= 2)
				fcyc_bits = 1;
			      else if (sploop_ii <= 4)
				fcyc_bits = 2;
			      else if (sploop_ii <= 8)
				fcyc_bits = 3;
			      else if (sploop_ii <= 14)
				fcyc_bits = 4;
			      else
				prev_sploop_found = FALSE;
			    }
			  if (prev_sploop_found)
			    break;
			}
		    }
		  if (!prev_sploop_found)
		    {
		      operands_ok = FALSE;
		      operands_text[op_num] = TRUE;
		      break;
		    }
		  if (fcyc_bits > field->width)
		    abort ();
		  if (enc->coding_method == tic6x_coding_fstg)
		    {
		      int i, t;
		      for (t = 0, i = fcyc_bits; i < 6; i++)
			t = (t << 1) | ((fld_val >> i) & 1);
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%u", t);
		    }
		  else
		    {
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%u",
				fld_val & ((1 << fcyc_bits) - 1));
		    }
		  break;

		case tic6x_coding_spmask:
		  if (fld_val == 0)
		    spmask_skip_operand = TRUE;
		  else
		    {
		      char *p;
		      unsigned int i;

		      operands_text[op_num] = TRUE;
		      p = operands[op_num];
		      for (i = 0; i < 8; i++)
			if (fld_val & (1 << i))
			  {
			    *p++ = "LSDM"[i/2];
			    *p++ = '1' + (i & 1);
			    *p++ = ',';
			  }
		      p[-1] = 0;
		    }
		  break;

		case tic6x_coding_fu:
		case tic6x_coding_data_fu:
		case tic6x_coding_xpath:
		  /* Don't relate to operands, so operand number is
		     meaningless.  */
		  break;

		default:
		  abort ();
		}

	      if (mem_base_reg_known_long && mem_offset_known_long)
		{
		  if (operands_text[op_num] || operands_pcrel[op_num])
		    abort ();
		  operands_text[op_num] = TRUE;
		  snprintf (operands[op_num], 24, "*+b%u(%u)", mem_base_reg,
			    mem_offset * opc->operand_info[op_num].size);
		}

	      if (mem_base_reg_known && mem_offset_known && mem_mode_known
		  && (mem_scaled_known
		      || (opc->operand_info[op_num].form
			  != tic6x_operand_mem_ndw)))
		{
		  char side;
		  char base[4];
		  bfd_boolean offset_is_reg;
		  bfd_boolean offset_scaled;
		  char offset[4];
		  char offsetp[6];

		  if (operands_text[op_num] || operands_pcrel[op_num])
		    abort ();

		  side = func_unit_side == 2 ? 'b' : 'a';
		  snprintf (base, 4, "%c%u", side, mem_base_reg);

		  offset_is_reg = ((mem_mode & 4) ? TRUE : FALSE);
		  if (offset_is_reg)
		    {
		      snprintf (offset, 4, "%c%u", side, mem_offset);
		      if (opc->operand_info[op_num].form
			  == tic6x_operand_mem_ndw)
			offset_scaled = mem_scaled ? TRUE : FALSE;
		      else
			offset_scaled = TRUE;
		    }
		  else
		    {
		      if (opc->operand_info[op_num].form
			  == tic6x_operand_mem_ndw)
			{
			  offset_scaled = mem_scaled ? TRUE : FALSE;
			  snprintf (offset, 4, "%u", mem_offset);
			}
		      else
			{
			  offset_scaled = FALSE;
			  snprintf (offset, 4, "%u",
				    (mem_offset
				     * opc->operand_info[op_num].size));
			}
		    }

		  if (offset_scaled)
		    snprintf (offsetp, 6, "[%s]", offset);
		  else
		    snprintf (offsetp, 6, "(%s)", offset);

		  operands_text[op_num] = TRUE;
		  switch (mem_mode & ~4u)
		    {
		    case 0:
		      snprintf (operands[op_num], 24, "*-%s%s", base, offsetp);
		      break;

		    case 1:
		      snprintf (operands[op_num], 24, "*+%s%s", base, offsetp);
		      break;

		    case 2:
		    case 3:
		      operands_ok = FALSE;
		      break;

		    case 8:
		      snprintf (operands[op_num], 24, "*--%s%s", base,
				offsetp);
		      break;

		    case 9:
		      snprintf (operands[op_num], 24, "*++%s%s", base,
				offsetp);
		      break;

		    case 10:
		      snprintf (operands[op_num], 24, "*%s--%s", base,
				offsetp);
		      break;

		    case 11:
		      snprintf (operands[op_num], 24, "*%s++%s", base,
				offsetp);
		      break;

		    default:
		      abort ();
		    }
		}

	      if (crlo_known && crhi_known)
		{
		  tic6x_rw rw;
		  tic6x_ctrl_id crid;

		  if (operands_text[op_num] || operands_pcrel[op_num])
		    abort ();

		  rw = opc->operand_info[op_num].rw;
		  if (rw != tic6x_rw_read
		      && rw != tic6x_rw_write)
		    abort ();

		  for (crid = 0; crid < tic6x_ctrl_max; crid++)
		    {
		      if (crlo == tic6x_ctrl_table[crid].crlo
			  && (crhi & tic6x_ctrl_table[crid].crhi_mask) == 0
			  && (rw == tic6x_rw_read
			      ? (tic6x_ctrl_table[crid].rw == tic6x_rw_read
				 || (tic6x_ctrl_table[crid].rw
				     == tic6x_rw_read_write))
			      : (tic6x_ctrl_table[crid].rw == tic6x_rw_write
				 || (tic6x_ctrl_table[crid].rw
				     == tic6x_rw_read_write))))
			break;
		    }
		  if (crid == tic6x_ctrl_max)
		    {
		      operands_text[op_num] = TRUE;
		      operands_ok = FALSE;
		    }
		  else
		    {
		      operands_text[op_num] = TRUE;
		      snprintf (operands[op_num], 24, "%s",
				tic6x_ctrl_table[crid].name);
		    }
		}

	      if (operands_text[op_num] || operands_pcrel[op_num]
		  || spmask_skip_operand)
		break;
	    }
	  if (spmask_skip_operand)
	    {
	      /* SPMASK operands are only valid as the single operand
		 in the opcode table.  */
	      if (num_operands != 1)
		abort ();
	      num_operands = 0;
	      break;
	    }
	  /* The operand must by now have been decoded.  */
	  if (!operands_text[op_num] && !operands_pcrel[op_num])
	    abort ();
	}

      if (!operands_ok)
	continue;

      info->bytes_per_chunk = num_bits / 8;
      info->fprintf_func (info->stream, "%s%s%s%s", parallel, cond,
			  opc->name, func_unit);
      for (op_num = 0; op_num < num_operands; op_num++)
	{
	  info->fprintf_func (info->stream, "%c", (op_num == 0 ? ' ' : ','));
	  if (operands_pcrel[op_num])
	    info->print_address_func (operands_addresses[op_num], info);
	  else
	    info->fprintf_func (info->stream, "%s", operands[op_num]);
	}
      if (fetch_packet_header_based && header.prot)
	info->fprintf_func (info->stream, " || nop 5");

      return num_bits / 8;
    }

  info->bytes_per_chunk = num_bits / 8;
  info->fprintf_func (info->stream, "<undefined instruction 0x%.*x>",
		      (int) num_bits / 4, opcode);
  return num_bits / 8;
}
