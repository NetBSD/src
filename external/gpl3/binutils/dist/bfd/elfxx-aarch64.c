/* AArch64-specific support for ELF.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.
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
#include "bfd.h"
#include "elf-bfd.h"
#include "elf/aarch64.h"
#include "elfxx-aarch64.h"
#include "libbfd.h"
#include "libiberty.h"
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
    case BFD_RELOC_AARCH64_TLSDESC_ADD_LO12:
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
    case BFD_RELOC_AARCH64_TLSDESC_LD64_LO12:
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
    case BFD_RELOC_AARCH64_TLSLE_LDST16_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST16_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST32_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST32_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST64_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST64_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST8_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST8_TPREL_LO12_NC:
      if (old_addend & ((1 << howto->rightshift) - 1))
	return bfd_reloc_overflow;
      /* Used for ldr*|str* rt, [rn, #uimm12] to provide the low order
	 12 bits address offset.  */
      contents = reencode_ldst_pos_imm (contents, addend);
      break;

      /* Group relocations to create high bits of a 16, 32, 48 or 64
	 bit signed data or abs address inline. Will change
	 instruction to MOVN or MOVZ depending on sign of calculated
	 value.  */

    case BFD_RELOC_AARCH64_MOVW_G0_S:
    case BFD_RELOC_AARCH64_MOVW_G1_S:
    case BFD_RELOC_AARCH64_MOVW_G2_S:
    case BFD_RELOC_AARCH64_MOVW_PREL_G0:
    case BFD_RELOC_AARCH64_MOVW_PREL_G1:
    case BFD_RELOC_AARCH64_MOVW_PREL_G2:
    case BFD_RELOC_AARCH64_MOVW_PREL_G3:
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
      /* Fall through.  */

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
    case BFD_RELOC_AARCH64_MOVW_PREL_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_PREL_G1_NC:
    case BFD_RELOC_AARCH64_MOVW_PREL_G2_NC:
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
_bfd_aarch64_elf_resolve_relocation (bfd *input_bfd,
				     bfd_reloc_code_real_type r_type,
				     bfd_vma place, bfd_vma value,
				     bfd_vma addend, bool weak_undef_p)
{
  bool tls_reloc = true;
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
    case BFD_RELOC_AARCH64_MOVW_PREL_G0:
    case BFD_RELOC_AARCH64_MOVW_PREL_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_PREL_G1:
    case BFD_RELOC_AARCH64_MOVW_PREL_G1_NC:
    case BFD_RELOC_AARCH64_MOVW_PREL_G2:
    case BFD_RELOC_AARCH64_MOVW_PREL_G2_NC:
    case BFD_RELOC_AARCH64_MOVW_PREL_G3:
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
      tls_reloc = false;
      /* fall-through.  */
    case BFD_RELOC_AARCH64_TLSDESC_OFF_G0_NC:
    case BFD_RELOC_AARCH64_TLSDESC_OFF_G1:
    case BFD_RELOC_AARCH64_TLSGD_MOVW_G0_NC:
    case BFD_RELOC_AARCH64_TLSGD_MOVW_G1:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_HI12:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_ADD_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST16_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST32_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST64_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_LDST8_DTPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G0:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G0_NC:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G1:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G1_NC:
    case BFD_RELOC_AARCH64_TLSLD_MOVW_DTPREL_G2:
    case BFD_RELOC_AARCH64_TLSLE_LDST16_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST32_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST64_TPREL_LO12:
    case BFD_RELOC_AARCH64_TLSLE_LDST8_TPREL_LO12:
      /* Weak Symbols and TLS relocations are implementation defined.  For this
	 case we choose to emit 0.  */
      if (weak_undef_p && tls_reloc)
	{
	  _bfd_error_handler (_("%pB: warning: Weak TLS is implementation "
				"defined and may not work as expected"),
				input_bfd);
	  value = place;
	}
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

    /* Caller must make sure addend is the base address of .got section.  */
    case BFD_RELOC_AARCH64_LD32_GOTPAGE_LO14:
    case BFD_RELOC_AARCH64_LD64_GOTPAGE_LO15:
      addend = PG (addend);
      /* Fall through.  */
    case BFD_RELOC_AARCH64_LD64_GOTOFF_LO15:
    case BFD_RELOC_AARCH64_MOVW_GOTOFF_G0_NC:
    case BFD_RELOC_AARCH64_MOVW_GOTOFF_G1:
      value = value - addend;
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
    case BFD_RELOC_AARCH64_TLSDESC_ADD_LO12:
    case BFD_RELOC_AARCH64_TLSDESC_LD32_LO12_NC:
    case BFD_RELOC_AARCH64_TLSDESC_LD64_LO12:
    case BFD_RELOC_AARCH64_TLSDESC_LDR:
    case BFD_RELOC_AARCH64_TLSGD_ADD_LO12_NC:
    case BFD_RELOC_AARCH64_TLSIE_LD32_GOTTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_ADD_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST16_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST32_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST64_TPREL_LO12_NC:
    case BFD_RELOC_AARCH64_TLSLE_LDST8_TPREL_LO12_NC:
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

/* Support for core dump NOTE sections.  */

bool
_bfd_aarch64_elf_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  int offset;
  size_t size;

  switch (note->descsz)
    {
      default:
	return false;

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

bool
_bfd_aarch64_elf_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  switch (note->descsz)
    {
    default:
      return false;

    case 136:	     /* This is sizeof(struct elf_prpsinfo) on Linux/aarch64.  */
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

  return true;
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
	char data[136] ATTRIBUTE_NONSTRING;
	va_list ap;

	va_start (ap, note_type);
	memset (data, 0, sizeof (data));
	strncpy (data + 40, va_arg (ap, const char *), 16);
#if GCC_VERSION == 8000 || GCC_VERSION == 8001
	DIAGNOSTIC_PUSH;
	/* GCC 8.0 and 8.1 warn about 80 equals destination size with
	   -Wstringop-truncation:
	   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85643
	 */
	DIAGNOSTIC_IGNORE_STRINGOP_TRUNCATION;
#endif
	strncpy (data + 56, va_arg (ap, const char *), 80);
#if GCC_VERSION == 8000 || GCC_VERSION == 8001
	DIAGNOSTIC_POP;
#endif
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

typedef struct
{
  bfd *pbfd;
  asection* sec;
} bfd_search_result_t;

static inline bool
bfd_is_non_dynamic_elf_object (bfd *abfd, elf_backend_data *out_be)
{
  elf_backend_data *in_be = get_elf_backend_data (abfd);

  return bfd_get_flavour (abfd) == bfd_target_elf_flavour
    && bfd_count_sections (abfd) != 0
    && (abfd->flags & (DYNAMIC | BFD_PLUGIN | BFD_LINKER_CREATED)) == 0
    && out_be->elf_machine_code == in_be->elf_machine_code
    && out_be->s->elfclass == in_be->s->elfclass;
}

/* Find the first input bfd with GNU properties.
   If such an input is found, set found to true and return the relevant input.
   Otherwise, return the last input of bfd inputs.  */
static bfd_search_result_t
bfd_linear_search_one_with_gnu_property (struct bfd_link_info *info)
{
  elf_backend_data *be = get_elf_backend_data (info->output_bfd);

  bfd_search_result_t res = {
    .pbfd = NULL,
    .sec = NULL,
  };

  for (bfd *pbfd = info->input_bfds; pbfd != NULL; pbfd = pbfd->link.next)
    if (bfd_is_non_dynamic_elf_object (pbfd, be))
      {
	res.pbfd = pbfd;

	/* Does the input have a list of GNU properties ? */
	if (elf_properties (pbfd) != NULL)
	  break;
      }

  if (res.pbfd != NULL)
    res.sec = bfd_get_section_by_name (res.pbfd, NOTE_GNU_PROPERTY_SECTION_NAME);

  return res;
}

/* Create a GNU property section for the given bfd input.  */
static void
_bfd_aarch64_elf_create_gnu_property_section (struct bfd_link_info *info,
					      bfd *ebfd)
{
  asection *sec;
  sec = bfd_make_section_with_flags (ebfd,
				     NOTE_GNU_PROPERTY_SECTION_NAME,
				     (SEC_ALLOC
				      | SEC_LOAD
				      | SEC_IN_MEMORY
				      | SEC_READONLY
				      | SEC_HAS_CONTENTS
				      | SEC_DATA));
  unsigned align = (bfd_get_mach (ebfd) & bfd_mach_aarch64_ilp32) ? 2 : 3;
  if (sec == NULL
      || !bfd_set_section_alignment (sec, align))
    info->callbacks->fatal (_("%P: failed to create %s\n"),
			    NOTE_GNU_PROPERTY_SECTION_NAME);

  elf_section_type (sec) = SHT_NOTE;
}

static const int GNU_PROPERTY_ISSUES_MAX = 20;

/* Report a summary of the issues met during the merge of the GNU properties, if
   the number of issues goes above GNU_PROPERTY_ISSUES_MAX.  */
static void
_bfd_aarch64_report_summary_merge_issues (struct bfd_link_info *info)
{
  const struct elf_aarch64_obj_tdata * tdata
    = elf_aarch64_tdata (info->output_bfd);

  if (tdata->n_bti_issues > GNU_PROPERTY_ISSUES_MAX
      && tdata->sw_protections.bti_report != MARKING_NONE)
    {
      const char *msg
	= (tdata->sw_protections.bti_report == MARKING_ERROR)
	? _("%Xerror: found a total of %d inputs incompatible with "
	    "BTI requirements.\n")
	: _("warning: found a total of %d inputs incompatible with "
	    "BTI requirements.\n");
      info->callbacks->einfo (msg, tdata->n_bti_issues);
    }

  if (tdata->n_gcs_issues > GNU_PROPERTY_ISSUES_MAX
      && tdata->sw_protections.gcs_report != MARKING_NONE)
    {
      const char *msg
	= (tdata->sw_protections.gcs_report == MARKING_ERROR)
	? _("%Xerror: found a total of %d inputs incompatible with "
	    "GCS requirements.\n")
	: _("warning: found a total of %d inputs incompatible with "
	    "GCS requirements.\n");
      info->callbacks->einfo (msg, tdata->n_gcs_issues);
    }

  if (tdata->n_gcs_dynamic_issues > GNU_PROPERTY_ISSUES_MAX
      && tdata->sw_protections.gcs_report_dynamic != MARKING_NONE)
    {
      const char *msg
	= (tdata->sw_protections.gcs_report_dynamic == MARKING_ERROR)
	? _("%Xerror: found a total of %d dynamically-linked objects "
	    "incompatible with GCS requirements.\n")
	: _("warning: found a total of %d dynamically-linked objects "
	    "incompatible with GCS requirements.\n");
      info->callbacks->einfo (msg, tdata->n_gcs_dynamic_issues);
    }
}

/* Perform a look-up of a property in an unsorted list of properties.  This is
   useful when the list of properties of an object has not been sorted yet.  */
static uint32_t
_bfd_aarch64_prop_linear_lookup (elf_property_list *properties,
				 unsigned int pr_type)
{
  for (elf_property_list *p = properties; p != NULL; p = p->next)
    if (p->property.pr_type == pr_type)
      return p->property.u.number;
  return 0;
}

/* Compare the GNU properties of the current dynamic object, with the ones of
   the output BFD.  Today, we only care about GCS feature stored in
   GNU_PROPERTY_AARCH64_FEATURE_1.  */
static void
_bfd_aarch64_compare_dynamic_obj_prop_against_outprop(
  struct bfd_link_info *info,
  const uint32_t outprop,
  bfd *pbfd)
{
  if (!(outprop & GNU_PROPERTY_AARCH64_FEATURE_1_GCS))
    return;

  const uint32_t dyn_obj_aarch64_feature_prop =
    _bfd_aarch64_prop_linear_lookup (elf_properties (pbfd),
				     GNU_PROPERTY_AARCH64_FEATURE_1_AND);
  if (!(dyn_obj_aarch64_feature_prop & GNU_PROPERTY_AARCH64_FEATURE_1_GCS))
    _bfd_aarch64_elf_check_gcs_report (info, pbfd);
}

/* Check compatibility between the GNU properties of the ouput BFD and the
   linked dynamic objects.  */
static void
_bfd_aarch64_elf_check_gnu_properties_linked_dynamic_objects (
  struct bfd_link_info * info,
  const uint32_t outprop)
{
  elf_backend_data *bed = get_elf_backend_data (info->output_bfd);
  const int elf_machine_code = bed->elf_machine_code;
  const unsigned int elfclass = bed->s->elfclass;

  for (bfd *pbfd = info->input_bfds; pbfd != NULL; pbfd = pbfd->link.next)
    /* Ignore GNU properties from non-ELF objects or ELF objects with different
       machine code.  */
    if ((pbfd->flags & DYNAMIC) != 0
	&& (bfd_get_flavour (pbfd) == bfd_target_elf_flavour)
	&& (get_elf_backend_data (pbfd)->elf_machine_code == elf_machine_code)
	&& (get_elf_backend_data (pbfd)->s->elfclass == elfclass))
      _bfd_aarch64_compare_dynamic_obj_prop_against_outprop(info, outprop,
	pbfd);
}

/* Decode the encoded version number corresponding to the Object Attribute
   version.  Return the version on success, UNSUPPORTED on failure.  */
obj_attr_version_t
_bfd_aarch64_obj_attrs_version_dec (uint8_t encoded_version)
{
  if (encoded_version == 'A')
    return OBJ_ATTR_V2;
  return OBJ_ATTR_VERSION_UNSUPPORTED;
}

/* Encode the Object Attribute version into a byte.  */
uint8_t
_bfd_aarch64_obj_attrs_version_enc (obj_attr_version_t version)
{
  if (version == OBJ_ATTR_V2)
    return 'A';
  abort ();
}

/* List of known attributes in the subsection "aeabi_feature_and_bits".
   Note: the array below has to be sorted by the tag's integer value that can
   be found in the document "Build Attributes for the Arm® 64-bit Architecture
   (AArch64)".  */
static const obj_attr_info_t known_attrs_aeabi_feature_and_bits[] =
{
  { .tag = {"Tag_Feature_BTI", Tag_Feature_BTI} },
  { .tag = {"Tag_Feature_PAC", Tag_Feature_PAC} },
  { .tag = {"Tag_Feature_GCS", Tag_Feature_GCS} },
};

/* List of known attributes in the subsection "aeabi_pauthabi".
   Notes:
   - "aeabi_pauthabi" is a required subsection to use PAuthABI (which is
     today only supported by LLVM, unsupported by GCC 15 and lower. There is no
     plan to add support for it in the future). A value of 0 for any the tags
     below means that the user did not permit this entity to use the PAuthABI.
   - the array below has to be sorted by the tag's integer value that can be
     found in the document "Build Attributes for the Arm® 64-bit Architecture
     (AArch64)".  */
static const obj_attr_info_t known_attrs_aeabi_pauthabi[] =
{
  { .tag = {"Tag_PAuth_Platform", Tag_PAuth_Platform} },
  { .tag = {"Tag_PAuth_Schema", Tag_PAuth_Schema} },
};

/* List of known subsections.
   Note: this array is exported by the backend, and has to be sorted using
   the same criteria as in _bfd_elf_obj_attr_subsection_v2_cmp().  */
const known_subsection_v2_t aarch64_obj_attr_v2_known_subsections[2] =
{
  {
    .subsec_name = "aeabi_feature_and_bits",
    .known_attrs = known_attrs_aeabi_feature_and_bits,
    .optional = true,
    .encoding = OA_ENC_ULEB128,
    .len = ARRAY_SIZE (known_attrs_aeabi_feature_and_bits),
  },
  {
    .subsec_name = "aeabi_pauthabi",
    .known_attrs = known_attrs_aeabi_pauthabi,
    .optional = false,
    .encoding = OA_ENC_ULEB128,
    .len = ARRAY_SIZE (known_attrs_aeabi_pauthabi),
  },
};

/* Record the pair (TAG, VALUE) into SUBSEC.  */
void
_bfd_aarch64_oav2_record (obj_attr_subsection_v2_t *subsec,
			  Tag_Feature_Set feature_tag,
			  uint32_t value)
{
  union obj_attr_value_v2 data;
  data.uint = value;
  obj_attr_v2_t *attr = bfd_elf_obj_attr_v2_init (feature_tag, data);
  LINKED_LIST_APPEND (obj_attr_v2_t) (subsec, attr);
}

/* Wrapper around the recording of the pair (TAG, VALUE) into SUBSEC called from
   a context of translation from GNU properties.  */
static void
obj_attr_v2_record_tag_value (obj_attr_subsection_v2_t *subsec,
			      Tag_Feature_Set tag,
			      bool value)
{
  obj_attr_v2_t *attr;
  attr = bfd_obj_attr_v2_find_by_tag (subsec, tag, false);
  if (attr != NULL)
    {
      if (attr->val.uint != value)
	{
	  /* If we find an existing value for the given object attributes and
	     this value is different from the new one, it can mean two things:
	       - either the values are conflicting, and we need to raise an
	       error.
	       - either there are several GNU properties AARCH64_FEATURE_1_AND
	       which were recorded, but its final value is the result of the
	       merge of those separate values.
	     For now, only the second case occurs.  */
	  uint32_t merged_val = attr->val.uint | value;
	  _bfd_aarch64_oav2_record (subsec, tag, merged_val);
	}
      /* else: nothing to do.  */
    }
  else
    _bfd_aarch64_oav2_record (subsec, tag, value);
}

/* Translate the relevant GNU properties in P to their Object Attributes v2
   equivalents.  */
void
_bfd_aarch64_translate_gnu_props_to_obj_attrs
  (const bfd *abfd, const elf_property_list *p)
{
  if (p->property.pr_type == GNU_PROPERTY_AARCH64_FEATURE_1_AND)
    {
      const elf_property *prop = &p->property;
      BFD_ASSERT (prop->pr_kind == property_number);

      obj_attr_subsection_v2_t *subsec = bfd_obj_attr_subsection_v2_find_by_name
	(elf_obj_attr_subsections (abfd).first, "aeabi_feature_and_bits", false);

      bool new_subsec = false;
      if (subsec == NULL)
	{
	  subsec = bfd_elf_obj_attr_subsection_v2_init
	    (xstrdup ("aeabi_feature_and_bits"), OA_SUBSEC_PUBLIC, true,
	     OA_ENC_ULEB128);
	  new_subsec = true;
	}

      bool bti_bit = prop->u.number & GNU_PROPERTY_AARCH64_FEATURE_1_BTI;
      bool pac_bit = prop->u.number & GNU_PROPERTY_AARCH64_FEATURE_1_PAC;
      bool gcs_bit = prop->u.number & GNU_PROPERTY_AARCH64_FEATURE_1_GCS;

      obj_attr_v2_record_tag_value (subsec, Tag_Feature_BTI, bti_bit);
      obj_attr_v2_record_tag_value (subsec, Tag_Feature_PAC, pac_bit);
      obj_attr_v2_record_tag_value (subsec, Tag_Feature_GCS, gcs_bit);

      if (new_subsec)
	LINKED_LIST_APPEND (obj_attr_subsection_v2_t)
	  (&elf_obj_attr_subsections (abfd), subsec);
    }
}

/* Translate relevant Object Attributes v2 in SUBSEC to GNU properties.  */
void
_bfd_aarch64_translate_obj_attrs_to_gnu_props
  (bfd *abfd, const obj_attr_subsection_v2_t *subsec)
{
  /* Note: there is no need to create the GNU properties section here.  It will
     be handled later by setup_gnu_properties.  */

  if (strcmp (subsec->name, "aeabi_feature_and_bits") == 0)
    {
      uint32_t gnu_property_aarch64_features = 0;

      for (const obj_attr_v2_t *attr = subsec->first;
	   attr != NULL;
	   attr = attr->next)
	{
	  if (attr->tag == Tag_Feature_BTI && attr->val.uint == 1)
	    gnu_property_aarch64_features |= GNU_PROPERTY_AARCH64_FEATURE_1_BTI;
	  else if (attr->tag == Tag_Feature_PAC && attr->val.uint == 1)
	    gnu_property_aarch64_features |= GNU_PROPERTY_AARCH64_FEATURE_1_PAC;
	  else if (attr->tag == Tag_Feature_GCS && attr->val.uint == 1)
	    gnu_property_aarch64_features |= GNU_PROPERTY_AARCH64_FEATURE_1_GCS;
	}

      /* Note: _bfd_elf_get_property find the existing property, or create one.
	 The insertion is already done by it.  */
      elf_property *prop
	= _bfd_elf_get_property (abfd, GNU_PROPERTY_AARCH64_FEATURE_1_AND, 4);
      prop->u.number |= gnu_property_aarch64_features;
      prop->pr_kind = property_number;
    }
}

/* Check whether the given ATTR is managed by the backend, and if that is the
   case, initialize ATTR with its default value coming from the known tag
   registry.
   True if the default value for the tag is managed by the backend, and was
   initialized.  False otherwise.  */
bool
_bfd_aarch64_oav2_default_value
  (const struct bfd_link_info *info ATTRIBUTE_UNUSED,
   const obj_attr_info_t *tag_info ATTRIBUTE_UNUSED,
   const obj_attr_subsection_v2_t *subsec ATTRIBUTE_UNUSED,
   obj_attr_v2_t *attr ATTRIBUTE_UNUSED)
{
  /* For now, there is no default value set by the backend.  The default BTI and
     GCS values are set by the respective command-line options '-z force-bti'
     and '-z gcs'.  */

  return false;
}

/* Merge the values from LHS, RHS, FROZEN, and return the merge result.  LHS,
   RHS and FROZEN correspond to an attribute from SUBSEC with the same tag, but
   from three different contexts:
    - LHS corresponds to the global merge result.
    - RHS corresponds to the new value that is merged into the global merge
      result.
    - FROZEN corresponds to the value coming from some configuration context
      (usually a command-line option) and which is immutable for the whole
      merge process.  */
obj_attr_v2_merge_result_t
_bfd_aarch64_oav2_attr_merge (const struct bfd_link_info *info,
			      const bfd *abfd,
			      const obj_attr_subsection_v2_t *subsec,
			      const obj_attr_v2_t *lhs, const obj_attr_v2_t *rhs,
			      const obj_attr_v2_t *frozen)
{
  obj_attr_v2_merge_result_t res = {
    .merge = false,
    .val.uint = 0,
    .reason = OAv2_MERGE_OK,
  };

  /* No need to list required sections here, they are handled separately as
     they require a perfect one-to-one match for all the tag values.  */
  if (strcmp (subsec->name, "aeabi_feature_and_bits") == 0)
    {
      BFD_ASSERT (subsec->encoding == OA_ENC_ULEB128 && subsec->optional);
      const obj_attr_info_t *attr_info
	= _bfd_obj_attr_v2_find_known_by_tag (get_elf_backend_data (abfd),
					      subsec->name, lhs->tag);
      if (attr_info == NULL)
	{
	  info->callbacks->einfo
	    (_("%pB: warning: cannot merge unknown tag 'Tag_unknown_%" PRIu64 "'"
	       " (=0x%" PRIx32 ") in subsection '%s'\n"),
	     abfd, rhs->tag, rhs->val.uint, subsec->name);
	  res.reason = OAv2_MERGE_UNSUPPORTED;
	  return res;
	}

      /* For now, there is no different between the tags of this section, all
	 will be merged in the same way.  */
      res = _bfd_obj_attr_v2_merge_AND (info, abfd, subsec, lhs, rhs, frozen);

      const aarch64_protection_opts *sw_protections
	= &elf_aarch64_tdata (info->output_bfd)->sw_protections;
      aarch64_feature_marking_report bti_report = sw_protections->bti_report;
      aarch64_feature_marking_report gcs_report = sw_protections->gcs_report;

      if (rhs->tag == Tag_Feature_BTI
	  && bti_report != MARKING_NONE
	  && (sw_protections->plt_type & PLT_BTI)
	  && rhs->val.uint == 0)
	_bfd_aarch64_elf_check_bti_report (info, abfd);

      if ((rhs->tag == Tag_Feature_GCS) && (gcs_report != MARKING_NONE)
       && (sw_protections->gcs_type == GCS_ALWAYS) && (rhs->val.uint == 0))
	_bfd_aarch64_elf_check_gcs_report (info, abfd);

      /* Make sure that frozen bits don't disappear from REF when it will be
	 compared to the next file.  */
      if (frozen != NULL)
	res.val.uint |= frozen->val.uint;
    }
  else
    res.reason = OAv2_MERGE_UNSUPPORTED;

  return res;
}

/* Check for incompatibilities with PAuthABI attributes.  */
static bool
aarch64_check_pauthabi_attributes (const struct bfd_link_info *info)
{
  /* The subsection "aeabi_pauthabi" contains information about the Pointer
     Authentication Signing schema when the object uses an extension to ELF,
     PAUTHABI64, which is today only supported by LLVM, and not supported by
     GCC 15 toolchain or ealier ones.  There is no plan to add support for it
     in the future.  The pointers that are signed as well as the modifiers and
     key used for each type of pointer are known as the signing schema.
     The AEABI Build attributes specification defines the following tuple values
     of (Tag_Pauth_Platform, Tag_Pauth_Schema):
       - The tuple (0, 0) is obtained when both attributes are explicitly set to
	 0 or are implicitly set to 0 due to the rules for setting default values
	 for public tags.  This represents an ELF file which makes no use of the
	 PAuthABI extension.
       - The tuple (0, 1) is reserved for the "Invalid" platform.  ELF files with
	 an "Invalid" platform are incompatible with the PAuth ABI Extension.
       - The tuples (0, N) where N > 1 are reserved.
       - The tuples (M, N) where M is the id of one of the registered platforms
	 defined in PAuthABI64, represents a valid signing schema.  (M, 0)
	 represents a schema version of 0 for platform M.
     Given that the GNU linker does not support PAuthABI, it cannot do anything
     with values others than (0, 0) or (0, 1).
     The check below enforces either that the output object has either no
     subsection "aeabi_pauthabi", or the tuple is set to (0, 0) and (0, 1).  */

  obj_attr_subsection_v2_t *subsecs
    = elf_obj_attr_subsections (info->output_bfd).first;
  const obj_attr_subsection_v2_t *subsec
    = bfd_obj_attr_subsection_v2_find_by_name (subsecs, "aeabi_pauthabi", true);
  if (subsec == NULL)
    return true;

  int platform_id = 0;
  int version_id = 0;

  const obj_attr_v2_t *attr
    = bfd_obj_attr_v2_find_by_tag (subsec, Tag_PAuth_Platform, true);
  if (attr != NULL)
    platform_id = attr->val.uint;

  attr = bfd_obj_attr_v2_find_by_tag (subsec, Tag_PAuth_Schema, true);
  if (attr != NULL)
    version_id = attr->val.uint;

  if (! ((platform_id == 0 && version_id == 0)
      || (platform_id == 0 && version_id == 1)))
    {
      info->callbacks->einfo
	(_("%Xerror: the GNU linker does not support PAuthABI.  Any value "
	   "different from (platform = 0, schema = 0) or (platform = 0, schema "
	   "= 1) is not supported.\n"));
      return false;
    }

  return true;
}

/* Merge the AEABI Build Attributes present in the input BFDs, raise any
   compatibility issue, and write the merge result to OBFD.

   AArch64 backend declares two vendor subsections, and their associated tags:
    - aeabi_feature_and_bits: contains tags that describe the same optional
      bits as the GNU_PROPERTY_AARCH64_FEATURE_1_AND.  For now, the following
      attributes are recognized:
	- Tag_Feature_BTI: means that all the executable sections are
	  compatible with Branch Target Identification (BTI) mechanism.
	- Tag_Feature_PAC: means that all the executable sections have been
	  protected with Return Address Signing.
	- Tag_Feature_GCS: means that all the executable sections are
	  compatible with the Guarded Control Stack (GCS) extension.
    - aeabi_pauthabi: contains information about the Pointer Authentication
      Signing schema when the object uses an extension to ELF, PAUTHABI64,
      which is currently not supported by GCC toolchain.  The pointers that
      are signed as well as the modifiers and key used for each type of pointer
      are known as the signing schema.  The support of this subsection is there
      for completeness with the AEABI Build Attributes document, and allows
      readelf to dump the data nicely, and the linker to detect a use of a
      signing schema, and error.
	- Tag_PAuth_Paltform: the platform vendor id.
	- Tag_PAuth_Schema: the version numner of the schema.

    For backward-compatibilty purpose, AArch64 backend translates
    GNU_PROPERTY_AARCH64_FEATURE_1_AND in input files to its OAv2 equivalents.
    The frozen set of OAv2 is populated with values derived from command-line
    options for BTI (-z force-bti) and GCS (-z gcs=*).
    It also reports incompatibilities for BTI and GCS, and set BTI PLT type
    depending on the OAv2 merge result.
    Regarding incompatibilities, only the ones detected in objects constituting
    the output link unit will be reported.  Supports for detecting incompatibi-
    -lities in shared objects might be a future work to bring it in pair with
    thenGNU properties merge.  However, since OAv2 are translated to GNU
    properties, detection will still happen so this feature seems redundant and
    of little value given the backward compatibility support for GNU properties
    is required (see next paragraph).
    Finally, it translates OAv2s in subsection "aeabi_feature_and_bits" to
    GNU_PROPERTY_AARCH64_FEATURE_1_AND as GNU properties are required for
    the dynamic linker (it does not understand OAv2s yet).  */
bfd *
_bfd_aarch64_elf_link_setup_object_attributes (struct bfd_link_info *info)
{
  bfd *pbfd = _bfd_elf_link_setup_object_attributes (info);

  /* Check PAuthABI compatibility.  */
  if (! aarch64_check_pauthabi_attributes (info))
    return NULL;

  /* Set the flag marking whether the merge of object attributes was done so
     that setup_gnu_properties does not raise the same errors/warning again.  */
  elf_aarch64_tdata (info->output_bfd)->oa_merge_done = true;

  return pbfd;
}

/* Find the first input bfd with GNU property and merge it with GPROP.  If no
   such input is found, add it to a new section at the last input.  Update
   GPROP accordingly.  */
bfd *
_bfd_aarch64_elf_link_setup_gnu_properties (struct bfd_link_info *info)
{
  struct elf_aarch64_obj_tdata *tdata = elf_aarch64_tdata (info->output_bfd);
  uint32_t outprop = tdata->gnu_property_aarch64_feature_1_and;

  bfd_search_result_t res = bfd_linear_search_one_with_gnu_property (info);

  /* If ebfd != NULL it is either an input with property note or the last input.
     Either way if we have an output GNU property that was provided, we should
     add it (by creating a section if needed).  */
  if (res.pbfd != NULL)
    {
      /* If no GNU property note section was found, create one.

	 Note: If there is no .gnu.note.property section, we might think that
	 elf_properties (res.pbfd) is always NULL.  However, this is not always
	 true for the following reasons:
	 - PR23900: old linkers were treating .note.gnu.property as a generic
	   note section, so old objects might contain properties inside .note
	   instead of .note.gnu.property.  In this case, the section won't be
	   detected but the properties are still parsed.  Consequently,
	   elf_properties (res.pbfd) is populated and different from NULL (see
	   https://sourceware.org/bugzilla/show_bug.cgi?id=23900 for more
	   details).
	 - since the introduction of the object attributes, once the merge
	   of the OAs is done, some of the OAs can be translated to GNU
	   properties like GNU_PROPERTY_AARCH64_FEATURE_1_AND.  In this case,
	   we need to check explicitly for the presence of the GNU properties
	   that might be added by the BAs merge.  */
      if (res.sec == NULL
	  && (elf_properties (res.pbfd) == NULL
	      || _bfd_elf_find_property (elf_properties (res.pbfd),
					 GNU_PROPERTY_AARCH64_FEATURE_1_AND,
					 NULL)))
	_bfd_aarch64_elf_create_gnu_property_section (info, res.pbfd);

      /* Merge the found input property with output properties. Note: if no
	 property was found, _bfd_elf_get_property will create one.  */
      elf_property *prop
	= _bfd_elf_get_property (res.pbfd,
				 GNU_PROPERTY_AARCH64_FEATURE_1_AND,
				 4);

      /* Check for a feature mismatch and report issue (if any) before this
	 information get lost as the value of ebfd will be overriden with
	 outprop.  */
      if ((outprop & GNU_PROPERTY_AARCH64_FEATURE_1_BTI)
	   && !(prop->u.number & GNU_PROPERTY_AARCH64_FEATURE_1_BTI))
	_bfd_aarch64_elf_check_bti_report (info, res.pbfd);

      if (tdata->sw_protections.gcs_type == GCS_NEVER)
	prop->u.number &= ~GNU_PROPERTY_AARCH64_FEATURE_1_GCS;
      else if ((outprop & GNU_PROPERTY_AARCH64_FEATURE_1_GCS)
	       && !(prop->u.number & GNU_PROPERTY_AARCH64_FEATURE_1_GCS))
	_bfd_aarch64_elf_check_gcs_report (info, res.pbfd);

      prop->u.number |= outprop;
      if (prop->u.number == 0)
	prop->pr_kind = property_remove;
      else
	prop->pr_kind = property_number;
    }

  /* Set up generic GNU properties, and merge them with the backend-specific
     ones (if any). pbfd points to the first relocatable ELF input with
     GNU properties (if found).  */
  bfd *pbfd = _bfd_elf_link_setup_gnu_properties (info);

  if (pbfd != NULL)
    {
      elf_property_list *p;
      elf_property_list *plist = elf_properties (pbfd);

      /* If pbfd has any GNU_PROPERTY_AARCH64_FEATURE_1_AND properties, update
	 outprop accordingly.  */
      if ((p = _bfd_elf_find_property (plist,
				       GNU_PROPERTY_AARCH64_FEATURE_1_AND, NULL))
				       != NULL)
        outprop = p->property.u.number
		  & (GNU_PROPERTY_AARCH64_FEATURE_1_BTI
		     | GNU_PROPERTY_AARCH64_FEATURE_1_PAC
		     | GNU_PROPERTY_AARCH64_FEATURE_1_GCS);
    }

  tdata->gnu_property_aarch64_feature_1_and = outprop;

  _bfd_aarch64_elf_check_gnu_properties_linked_dynamic_objects (info, outprop);

  _bfd_aarch64_report_summary_merge_issues (info);

  return pbfd;
}

/* Define elf_backend_parse_gnu_properties for AArch64.  */
enum elf_property_kind
_bfd_aarch64_elf_parse_gnu_properties (bfd *abfd, unsigned int type,
				       bfd_byte *ptr, unsigned int datasz)
{
  elf_property *prop;

  switch (type)
    {
    case GNU_PROPERTY_AARCH64_FEATURE_1_AND:
      if (datasz != 4)
	{
	  _bfd_error_handler
	    ( _("error: %pB: <corrupt AArch64 used size: 0x%x>"),
	     abfd, datasz);
	  return property_corrupt;
	}
      prop = _bfd_elf_get_property (abfd, type, datasz);
      /* Merge AArch64 feature properties together if they are declared in
	 different AARCH64_FEATURE_1_AND properties.  */
      prop->u.number |= bfd_h_get_32 (abfd, ptr);
      prop->pr_kind = property_number;
      break;

    default:
      return property_ignored;
    }

  return property_number;
}

/* Merge AArch64 GNU property BPROP with APROP also accounting for OUTPROP.
   If APROP isn't NULL, merge it with BPROP and/or OUTPROP.  Vice-versa if BROP
   isn't NULL.  Return TRUE if there is any update to APROP or if BPROP should
   be merge with ABFD.  */
bool
_bfd_aarch64_elf_merge_gnu_properties (struct bfd_link_info *info
				       ATTRIBUTE_UNUSED,
				       bfd *abfd ATTRIBUTE_UNUSED,
				       elf_property *aprop,
				       elf_property *bprop,
				       uint32_t outprop)
{
  unsigned int orig_number;
  bool updated = false;
  unsigned int pr_type = aprop != NULL ? aprop->pr_type : bprop->pr_type;

  switch (pr_type)
    {
    case GNU_PROPERTY_AARCH64_FEATURE_1_AND:
      {
	aarch64_gcs_type gcs_type
	  = elf_aarch64_tdata (info->output_bfd)->sw_protections.gcs_type;
	/* OUTPROP does not contain GCS for GCS_NEVER. We only need to make sure
	 that APROP does not contain GCS as well.
	 Notes:
	  - if BPROP contains GCS and APROP is not null, it is zeroed by the
	    AND with APROP.
	  - if BPROP contains GCS and APROP is null, it is overwritten with
	    OUTPROP as the AND with APROP would have been equivalent to zeroing
	    BPROP.  */
	if (gcs_type == GCS_NEVER && aprop != NULL)
	  aprop->u.number &= ~GNU_PROPERTY_AARCH64_FEATURE_1_GCS;

	if (aprop != NULL && bprop != NULL)
	  {
	    orig_number = aprop->u.number;
	    aprop->u.number = (orig_number & bprop->u.number) | outprop;
	    updated = orig_number != aprop->u.number;
	    /* Remove the property if all feature bits are cleared.  */
	    if (aprop->u.number == 0)
	      aprop->pr_kind = property_remove;
	    break;
	  }
	/* If either is NULL, the AND would be 0 so, if there is
	   any OUTPROP, assign it to the input that is not NULL.  */
	if (outprop)
	  {
	    if (aprop != NULL)
	      {
		orig_number = aprop->u.number;
		aprop->u.number = outprop;
		updated = orig_number != aprop->u.number;
	      }
	    else
	      {
		bprop->u.number = outprop;
		updated = true;
	      }
	  }
	/* No OUTPROP and BPROP is NULL, so remove APROP.  */
	else if (aprop != NULL)
	  {
	    aprop->pr_kind = property_remove;
	    updated = true;
	  }
      }
      break;

    default:
      abort ();
    }

  return updated;
}

/* Fix up AArch64 GNU properties.  */
void
_bfd_aarch64_elf_link_fixup_gnu_properties
  (struct bfd_link_info *info ATTRIBUTE_UNUSED,
   elf_property_list **listp)
{
  elf_property_list *p, *prev;

  for (p = *listp, prev = *listp; p; p = p->next)
    {
      unsigned int type = p->property.pr_type;
      if (type == GNU_PROPERTY_AARCH64_FEATURE_1_AND)
	{
	  if (p->property.pr_kind == property_remove)
	    {
	      /* Remove empty property.  */
	      if (prev == p)
		{
		  *listp = p->next;
		  prev = *listp;
		}
	      else
		  prev->next = p->next;
	      continue;
	    }
	  prev = p;
	}
      else if (type > GNU_PROPERTY_HIPROC)
	{
	  /* The property list is sorted in order of type.  */
	  break;
	}
    }
}

/* Check AArch64 BTI report.  */
void
_bfd_aarch64_elf_check_bti_report (const struct bfd_link_info *info,
				   const bfd *abfd)
{
  struct elf_aarch64_obj_tdata *tdata = elf_aarch64_tdata (info->output_bfd);

  if (elf_aarch64_tdata (info->output_bfd)->oa_merge_done
      || tdata->sw_protections.bti_report == MARKING_NONE)
    return;

  ++tdata->n_bti_issues;

  if (tdata->n_bti_issues > GNU_PROPERTY_ISSUES_MAX)
    return;

  const char *msg
    = (tdata->sw_protections.bti_report == MARKING_WARN)
    ? _("%pB: warning: BTI is required by -z force-bti, but this input object "
	"file lacks the necessary property note.\n")
    : _("%X%pB: error: BTI is required by -z force-bti, but this input object "
	"file lacks the necessary property note.\n");

  info->callbacks->einfo (msg, abfd);
}

/* Check AArch64 GCS report.  */
void
_bfd_aarch64_elf_check_gcs_report (const struct bfd_link_info *info,
				   const bfd *abfd)
{
  struct elf_aarch64_obj_tdata *tdata = elf_aarch64_tdata (info->output_bfd);
  bool dynamic_obj = (abfd->flags & DYNAMIC) != 0;

  if (dynamic_obj)
    {
      if (tdata->sw_protections.gcs_report_dynamic == MARKING_NONE)
	return;
      ++tdata->n_gcs_dynamic_issues;
      if (tdata->n_gcs_dynamic_issues > GNU_PROPERTY_ISSUES_MAX)
	return;
    }
  else
    {
      if (elf_aarch64_tdata (info->output_bfd)->oa_merge_done
	  || tdata->sw_protections.gcs_report == MARKING_NONE)
	return;
      ++tdata->n_gcs_issues;
      if (tdata->n_gcs_issues > GNU_PROPERTY_ISSUES_MAX)
	return;
    }

  const char *msg;
  if (dynamic_obj)
    msg = (tdata->sw_protections.gcs_report_dynamic == MARKING_WARN)
      ? _("%pB: warning: GCS is required by -z gcs, but this shared library "
	  "lacks the necessary property note. The dynamic loader might not "
	  "enable GCS or refuse to load the program unless all the shared "
	  "library dependencies have the GCS marking.\n")
      : _("%X%pB: error: GCS is required by -z gcs, but this shared library "
	  "lacks the necessary property note. The dynamic loader might not "
	  "enable GCS or refuse to load the program unless all the shared "
	  "library dependencies have the GCS marking.\n");
  else
    msg = (tdata->sw_protections.gcs_report == MARKING_WARN)
      ? _("%pB: warning: GCS is required by -z gcs, but this input object file "
	  "lacks the necessary property note.\n")
      : _("%X%pB: error: GCS is required by -z gcs, but this input object file "
	  "lacks the necessary property note.\n");

  info->callbacks->einfo (msg, abfd);
}
