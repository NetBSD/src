/* AArch64-specific support for ELF.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "elfxx-aarch64.h"
#include <stdarg.h>
#include <string.h>

#define MASK(n) ((1u << (n)) - 1)

/* Sign-extend VALUE, which has the indicated number of BITS.  */

bfd_signed_vma
_bfd_aarch64_sign_extend (bfd_vma value, int bits)
{
  if (value & ((bfd_vma) 1 << (bits - 1)))
    /* VALUE is negative.  */
    value |= ((bfd_vma) - 1) << bits;

  return value;
}

/* Decode the IMM field of ADRP.  */

uint32_t
_bfd_aarch64_decode_adrp_imm (uint32_t insn)
{
  return (((insn >> 5) & MASK (19)) << 2) | ((insn >> 29) & MASK (2));
}

/* Reencode the imm field of add immediate.  */
static inline uint32_t
reencode_add_imm (uint32_t insn, uint32_t imm)
{
  return (insn & ~(MASK (12) << 10)) | ((imm & MASK (12)) << 10);
}

/* Reencode the IMM field of ADR.  */

uint32_t
_bfd_aarch64_reencode_adr_imm (uint32_t insn, uint32_t imm)
{
  return (insn & ~((MASK (2) << 29) | (MASK (19) << 5)))
    | ((imm & MASK (2)) << 29) | ((imm & (MASK (19) << 2)) << 3);
}

/* Reencode the imm field of ld/st pos immediate.  */
static inline uint32_t
reencode_ldst_pos_imm (uint32_t insn, uint32_t imm)
{
  return (insn & ~(MASK (12) << 10)) | ((imm & MASK (12)) << 10);
}

/* Encode the 26-bit offset of unconditional branch.  */
static inline uint32_t
reencode_branch_ofs_26 (uint32_t insn, uint32_t ofs)
{
  return (insn & ~MASK (26)) | (ofs & MASK (26));
}

/* Encode the 19-bit offset of conditional branch and compare & branch.  */
static inline uint32_t
reencode_cond_branch_ofs_19 (uint32_t insn, uint32_t ofs)
{
  return (insn & ~(MASK (19) << 5)) | ((ofs & MASK (19)) << 5);
}

/* Decode the 19-bit offset of load literal.  */
static inline uint32_t
reencode_ld_lit_ofs_19 (uint32_t insn, uint32_t ofs)
{
  return (insn & ~(MASK (19) << 5)) | ((ofs & MASK (19)) << 5);
}

/* Encode the 14-bit offset of test & branch.  */
static inline uint32_t
reencode_tst_branch_ofs_14 (uint32_t insn, uint32_t ofs)
{
  return (insn & ~(MASK (14) << 5)) | ((ofs & MASK (14)) << 5);
}

/* Reencode the imm field of move wide.  */
static inline uint32_t
reencode_movw_imm (uint32_t insn, uint32_t imm)
{
  return (insn & ~(MASK (16) << 5)) | ((imm & MASK (16)) << 5);
}

/* Reencode mov[zn] to movz.  */
static inline uint32_t
reencode_movzn_to_movz (uint32_t opcode)
{
  return opcode | (1 << 30);
}

/* Reencode mov[zn] to movn.  */
static inline uint32_t
reencode_movzn_to_movn (uint32_t opcode)
{
  return opcode & ~(1 << 30);
}

/* Return non-zero if the indicated VALUE has overflowed the maximum
   range expressible by a unsigned number with the indicated number of
   BITS.  */

static bfd_reloc_status_type
aarch64_unsigned_overflow (bfd_vma value, unsigned int bits)
{
  bfd_vma lim;
  if (bits >= sizeof (bfd_vma) * 8)
    return bfd_reloc_ok;
  lim = (bfd_vma) 1 << bits;
  if (value >= lim)
    return bfd_reloc_overflow;
  return bfd_reloc_ok;
}

/* Return non-zero if the indicated VALUE has overflowed the maximum
   range expressible by an signed number with the indicated number of
   BITS.  */

static bfd_reloc_status_type
aarch64_signed_overflow (bfd_vma value, unsigned int bits)
{
  bfd_signed_vma svalue = (bfd_signed_vma) value;
  bfd_signed_vma lim;

  if (bits >= sizeof (bfd_vma) * 8)
    return bfd_reloc_ok;
  lim = (bfd_signed_vma) 1 << (bits - 1);
  if (svalue < -lim || svalue >= lim)
    return bfd_reloc_overflow;
  return bfd_reloc_ok;
}

/* Insert the addend/value into the instruction or data object being
   relocated.  */
bfd_reloc_status_type
_bfd_aarch64_elf_put_addend (bfd *abfd,
			     bfd_byte *address, bfd_reloc_code_real_type r_type,
			     reloc_howto_type *howto, bfd_signed_vma addend)
{
  bfd_reloc_status_type status = bfd_reloc_ok;
  bfd_signed_vma old_addend = addend;
  bfd_vma contents;
  int size;

  size = bfd_get_reloc_size (howto);
  switch (size)
    {
    case 0:
      return status;
    case 2:
      contents = bfd_get_16 (abfd, address);
      break;
    case 4:
      if (howto->src_mask != 0xffffffff)
	/* Must be 32-bit instruction, always little-endian.  */
	contents = bfd_getl32 (address);
      else
	/* Must be 32-bit data (endianness dependent).  */
	contents = bfd_get_32 (abfd, address);
      break;
    case 8:
      contents = bfd_get_64 (abfd, address);
      break;
    default:
      abort ();
    }

  switch (howto->complain_on_overflow)
    {
    case complain_overflow_dont:
      break;
    case complain_overflow_signed:
      status = aarch64_signed_overflow (addend,
					howto->bitsize + howto->rightshift);
      break;
    case complain_overflow_unsigned:
      status = aarch64_unsigned_overflow (addend,
					  howto->bitsize + howto->rightshift);
      break;
    case complain_overflow_bitfield:
    default:
      abort ();
    }

  addend >>= howto->rightshift;

  switch (r_type)
    {
    case BFD_RELOC_AARCH64_CALL26:
    case BFD_RELOC_AARCH64_JUMP26:
      contents = reencode_branch_ofs_26 (contents, addend);
      break;

    case BFD_RELOC_AARCH64_BRANCH19:
      contents = reencode_cond_branch_ofs_19 (contents, addend);
      break;

    case BFD_RELOC_AARCH64_TSTBR14:
      contents = reencode_tst_branch_ofs_14 (contents, addend);
      break;

    case BFD_RELOC_AARCH64_GOT_LD_PREL19:
    case BFD_RELOC_AARCH64_LD_LO19_PCREL:
    case BFD_RELOC_AARCH64_TLSDESC_LD_PREL19:
    case BFD_RELOC_AARCH64_TLSIE_LD_GOTTPREL_PREL19:
      if (old_addend & ((1 << howto->rightshift) - 1))
	return bfd_reloc_overflow;
      contents = reencode_ld_lit_ofs_19 (contents, addend);
      break;

    case BFD_RELOC_AARCH64_TLSDESC_CALL:
      break;

    case BFD_RELOC_AARCH64_ADR_GOT_PAGE:
    case BFD_RELOC_AARCH64_ADR_HI21_NC_PCREL:
    case BFD_RELOC_AARCH64_ADR_HI21_PCREL:
    case BFD_RELOC_AARCH64_ADR_LO21_PCREL:
    case BFD_RELOC_AARCH64_TLSDESC_ADR_PAGE21:
    case BFD_RELOC_AARCH64_TLSDESC_ADR_PREL21:
    case BFD_RELOC_AARCH64_TLSGD_ADR_PAGE21:
    case BFD_RELOC_AARCH64_TLSGD_ADR_PREL21:
    case BFD_RELOC_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21:
    case BFD_RELOC_AARCH64_TLSLD_ADR_PAGE21:
    case BFD_RELOC_AARCH64_TLSLD_ADR_PREL21:
      contents = _bfd_aarch64_reencode_adr_imm (contents, addend);
      break;

    case BFD_RELOC_AARCH64_ADD_LO12:
    case BFD_RELOC_AARCH64_TLSDESC_ADD_LO12_NC:
    case BFD_RELOC_AARCH64_TLSGD_ADD_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_HI12:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_ADD_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_HI12:
    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_LO12_NC:
      /* Corresponds to: add rd, rn, #uimm12 to provide the low order
         12 bits of the page offset following
         BFD_RELOC_AARCH64_ADR_HI21_PCREL which computes the
         (pc-relative) page base.  */
      contents = reencode_add_imm (contents, addend);
      break;

    case BFD_RELOC_AARCH64_LD32_GOTPAGE_LO14:
    case BFD_RELOC_AARCH64_LD32_GOT_LO12_NC:
    case BFD_RELOC_AARCH64_LD64_GOTOFF_LO15:
    case BFD_RELOC_AARCH64_LD64_GOTPAGE_LO15:
    case BFD_RELOC_AARCH64_LD64_GOT_LO12_NC:
    case BFD_RELOC_AARCH64_LDST128_LO12:
    case BFD_RELOC_AARCH64_LDST16_LO12:
    case BFD_RELOC_AARCH64_LDST32_LO12:
    case BFD_RELOC_AARCH64_LDST64_LO12:
    case BFD_RELOC_AARCH64_LDST8_LO12:
    case BFD_RELOC_AARCH64_TLSDESC_LD32_LO12_NC:
    case BFD_RELOC_AARCH64_TLSDESC_LD64_LO12_NC:
    case BFD_RELOC_AARCH64_TLSIE_LD32_GOTTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST16_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST32_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST64_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST8_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC:
      if (old_addend & ((1 << howto->rightshift) - 1))
	return bfd_reloc_overflow;
      /* Used for ldr*|str* rt, [rn, #uimm12] to provide the low order
         12 bits of the page offset following BFD_RELOC_AARCH64_ADR_HI21_PCREL
         which computes the (pc-relative) page base.  */
      contents = reencode_ldst_pos_imm (contents, addend);
      break;

      /* Group relocations to create high bits of a 16, 32, 48 or 64
         bit signed data or abs address inline. Will change
         instruction to MOVN or MOVZ depending on sign of calculated
         value.  */

    case BFD_RELOC_AARCH64_MOVW_G0_S:
    case BFD_RELOC_AARCH64_MOVW_G1_S:
    case BFD_RELOC_AARCH64_MOVW_G2_S:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G0:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G1:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G2:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G0:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G1:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G2:
      /* NOTE: We can only come here with movz or movn.  */
      if (addend < 0)
	{
	  /* Force use of MOVN.  */
	  addend = ~addend;
	  contents = reencode_movzn_to_movn (contents);
	}
      else
	{
	  /* Force use of MOVZ.  */
	  contents = reencode_movzn_to_movz (contents);
	}
      /* fall through */

      /* Group relocations to create a 16, 32, 48 or 64 bit unsigned
         data or abs address inline.  */

    case BFD_RELOC_AARCH64_MOVW_G0:
    case BFD_RELOC_AARCH64_MOVW_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_G1:
    case BFD_RELOC_AARCH64_MOVW_G1_NC:
    case BFD_RELOC_AARCH64_MOVW_G2:
    case BFD_RELOC_AARCH64_MOVW_G2_NC:
    case BFD_RELOC_AARCH64_MOVW_G3:
    case BFD_RELOC_AARCH64_MOVW_GOTOFF_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_GOTOFF_G1:
    case BFD_RELOC_AARCH64_TLSDESC_OFF_G0_NC:
    case BFD_RELOC_AARCH64_TLSDESC_OFF_G1:
    case BFD_RELOC_AARCH64_TLSGD_MOVW_G0_NC:
    case BFD_RELOC_AARCH64_TLSGD_MOVW_G1:
    case BFD_RELOC_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC:
    case BFD_RELOC_AARCH64_TLSIE_MOVW_GOTTPREL_G1:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G0_NC:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G1_NC:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G0_NC:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G1_NC:
      contents = reencode_movw_imm (contents, addend);
      break;

    default:
      /* Repack simple data */
      if (howto->dst_mask & (howto->dst_mask + 1))
	return bfd_reloc_notsupported;

      contents = ((contents & ~howto->dst_mask) | (addend & howto->dst_mask));
      break;
    }

  switch (size)
    {
    case 2:
      bfd_put_16 (abfd, contents, address);
      break;
    case 4:
      if (howto->dst_mask != 0xffffffff)
	/* must be 32-bit instruction, always little-endian */
	bfd_putl32 (contents, address);
      else
	/* must be 32-bit data (endianness dependent) */
	bfd_put_32 (abfd, contents, address);
      break;
    case 8:
      bfd_put_64 (abfd, contents, address);
      break;
    default:
      abort ();
    }

  return status;
}

bfd_vma
_bfd_aarch64_elf_resolve_relocation (bfd_reloc_code_real_type r_type,
				     bfd_vma place, bfd_vma value,
				     bfd_vma addend, bfd_boolean weak_undef_p)
{
  switch (r_type)
    {
    case BFD_RELOC_AARCH64_NONE:
    case BFD_RELOC_AARCH64_TLSDESC_CALL:
      break;

    case BFD_RELOC_AARCH64_16_PCREL:
    case BFD_RELOC_AARCH64_32_PCREL:
    case BFD_RELOC_AARCH64_64_PCREL:
    case BFD_RELOC_AARCH64_ADR_LO21_PCREL:
    case BFD_RELOC_AARCH64_BRANCH19:
    case BFD_RELOC_AARCH64_LD_LO19_PCREL:
    case BFD_RELOC_AARCH64_TLSDESC_ADR_PREL21:
    case BFD_RELOC_AARCH64_TLSDESC_LD_PREL19:
    case BFD_RELOC_AARCH64_TLSGD_ADR_PREL21:
    case BFD_RELOC_AARCH64_TLSIE_LD_GOTTPREL_PREL19:
    case BFD_RELOC_AARCH64_TLSLD_ADR_PREL21:
    case BFD_RELOC_AARCH64_TSTBR14:
      if (weak_undef_p)
	value = place;
      value = value + addend - place;
      break;

    case BFD_RELOC_AARCH64_CALL26:
    case BFD_RELOC_AARCH64_JUMP26:
      value = value + addend - place;
      break;

    case BFD_RELOC_AARCH64_16:
    case BFD_RELOC_AARCH64_32:
    case BFD_RELOC_AARCH64_LD64_GOTOFF_LO15:
    case BFD_RELOC_AARCH64_MOVW_G0:
    case BFD_RELOC_AARCH64_MOVW_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_G0_S:
    case BFD_RELOC_AARCH64_MOVW_G1:
    case BFD_RELOC_AARCH64_MOVW_G1_NC:
    case BFD_RELOC_AARCH64_MOVW_G1_S:
    case BFD_RELOC_AARCH64_MOVW_G2:
    case BFD_RELOC_AARCH64_MOVW_G2_NC:
    case BFD_RELOC_AARCH64_MOVW_G2_S:
    case BFD_RELOC_AARCH64_MOVW_G3:
    case BFD_RELOC_AARCH64_MOVW_GOTOFF_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_GOTOFF_G1:
    case BFD_RELOC_AARCH64_TLSDESC_OFF_G0_NC:
    case BFD_RELOC_AARCH64_TLSDESC_OFF_G1:
    case BFD_RELOC_AARCH64_TLSGD_MOVW_G0_NC:
    case BFD_RELOC_AARCH64_TLSGD_MOVW_G1:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_HI12:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST16_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST32_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST64_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST8_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G0:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G0_NC:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G1:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G1_NC:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G2:
      value = value + addend;
      break;

    case BFD_RELOC_AARCH64_ADR_HI21_NC_PCREL:
    case BFD_RELOC_AARCH64_ADR_HI21_PCREL:
      if (weak_undef_p)
	value = PG (place);
      value = PG (value + addend) - PG (place);
      break;

    case BFD_RELOC_AARCH64_GOT_LD_PREL19:
      value = value + addend - place;
      break;

    case BFD_RELOC_AARCH64_ADR_GOT_PAGE:
    case BFD_RELOC_AARCH64_TLSDESC_ADR_PAGE21:
    case BFD_RELOC_AARCH64_TLSGD_ADR_PAGE21:
    case BFD_RELOC_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21:
    case BFD_RELOC_AARCH64_TLSLD_ADR_PAGE21:
      value = PG (value + addend) - PG (place);
      break;

    case BFD_RELOC_AARCH64_LD32_GOTPAGE_LO14:
    case BFD_RELOC_AARCH64_LD64_GOTPAGE_LO15:
      /* Caller must make sure addend is the base address of .got section.  */
      value = value - PG (addend);
      break;

    case BFD_RELOC_AARCH64_ADD_LO12:
    case BFD_RELOC_AARCH64_LD32_GOT_LO12_NC:
    case BFD_RELOC_AARCH64_LD64_GOT_LO12_NC:
    case BFD_RELOC_AARCH64_LDST128_LO12:
    case BFD_RELOC_AARCH64_LDST16_LO12:
    case BFD_RELOC_AARCH64_LDST32_LO12:
    case BFD_RELOC_AARCH64_LDST64_LO12:
    case BFD_RELOC_AARCH64_LDST8_LO12:
    case BFD_RELOC_AARCH64_TLSDESC_ADD:
    case BFD_RELOC_AARCH64_TLSDESC_ADD_LO12_NC:
    case BFD_RELOC_AARCH64_TLSDESC_LD32_LO12_NC:
    case BFD_RELOC_AARCH64_TLSDESC_LD64_LO12_NC:
    case BFD_RELOC_AARCH64_TLSDESC_LDR:
    case BFD_RELOC_AARCH64_TLSGD_ADD_LO12_NC:
    case BFD_RELOC_AARCH64_TLSIE_LD32_GOTTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_LO12_NC:
      value = PG_OFFSET (value + addend);
      break;

    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_LO12:
      value = value + addend;
      break;

    case BFD_RELOC_AARCH64_TLSIE_MOVW_GOTTPREL_G1:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G1:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G1_NC:
      value = (value + addend) & (bfd_vma) 0xffff0000;
      break;
    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_HI12:
      /* Mask off low 12bits, keep all other high bits, so that the later
	 generic code could check whehter there is overflow.  */
      value = (value + addend) & ~(bfd_vma) 0xfff;
      break;

    case BFD_RELOC_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G0:
    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G0_NC:
      value = (value + addend) & (bfd_vma) 0xffff;
      break;

    case BFD_RELOC_AARCH64_TLSLE_MOVW_TPREL_G2:
      value = (value + addend) & ~(bfd_vma) 0xffffffff;
      value -= place & ~(bfd_vma) 0xffffffff;
      break;

    default:
      break;
    }

  return value;
}

/* Hook called by the linker routine which adds symbols from an object
   file.  */

bfd_boolean
_bfd_aarch64_elf_add_symbol_hook (bfd *abfd, struct bfd_link_info *info,
				  Elf_Internal_Sym *sym,
				  const char **namep ATTRIBUTE_UNUSED,
				  flagword *flagsp ATTRIBUTE_UNUSED,
				  asection **secp ATTRIBUTE_UNUSED,
				  bfd_vma *valp ATTRIBUTE_UNUSED)
{
  if (ELF_ST_TYPE (sym->st_info) == STT_GNU_IFUNC
      && (abfd->flags & DYNAMIC) == 0
      && bfd_get_flavour (info->output_bfd) == bfd_target_elf_flavour)
    elf_tdata (info->output_bfd)->has_gnu_symbols |= elf_gnu_symbol_ifunc;

  return TRUE;
}

/* Support for core dump NOTE sections.  */

bfd_boolean
_bfd_aarch64_elf_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  int offset;
  size_t size;

  switch (note->descsz)
    {
      default:
	return FALSE;

      case 392:		/* sizeof(struct elf_prstatus) on Linux/arm64.  */
	/* pr_cursig */
	elf_tdata (abfd)->core->signal
	  = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core->lwpid
	  = bfd_get_32 (abfd, note->descdata + 32);

	/* pr_reg */
	offset = 112;
	size = 272;

	break;
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg",
					  size, note->descpos + offset);
}

bfd_boolean
_bfd_aarch64_elf_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  switch (note->descsz)
    {
    default:
      return FALSE;

    case 136:        /* This is sizeof(struct elf_prpsinfo) on Linux/aarch64.  */
      elf_tdata (abfd)->core->pid = bfd_get_32 (abfd, note->descdata + 24);
      elf_tdata (abfd)->core->program
	= _bfd_elfcore_strndup (abfd, note->descdata + 40, 16);
      elf_tdata (abfd)->core->command
	= _bfd_elfcore_strndup (abfd, note->descdata + 56, 80);
    }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */

  {
    char *command = elf_tdata (abfd)->core->command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return TRUE;
}

char *
_bfd_aarch64_elf_write_core_note (bfd *abfd, char *buf, int *bufsiz, int note_type,
				  ...)
{
  switch (note_type)
    {
    default:
      return NULL;

    case NT_PRPSINFO:
      {
        char data[136];
        va_list ap;

        va_start (ap, note_type);
        memset (data, 0, sizeof (data));
        strncpy (data + 40, va_arg (ap, const char *), 16);
        strncpy (data + 56, va_arg (ap, const char *), 80);
        va_end (ap);

        return elfcore_write_note (abfd, buf, bufsiz, "CORE",
				   note_type, data, sizeof (data));
      }

    case NT_PRSTATUS:
      {
        char data[392];
        va_list ap;
        long pid;
        int cursig;
        const void *greg;

        va_start (ap, note_type);
        memset (data, 0, sizeof (data));
        pid = va_arg (ap, long);
        bfd_put_32 (abfd, pid, data + 32);
        cursig = va_arg (ap, int);
        bfd_put_16 (abfd, cursig, data + 12);
        greg = va_arg (ap, const void *);
        memcpy (data + 112, greg, 272);
        va_end (ap);

        return elfcore_write_note (abfd, buf, bufsiz, "CORE",
				   note_type, data, sizeof (data));
      }
    }
}
