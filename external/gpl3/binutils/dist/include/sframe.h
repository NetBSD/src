/* SFrame format description.
   Copyright (C) 2022-2026 Free Software Foundation, Inc.

   This file is part of libsframe.

   libsframe is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef	_SFRAME_H
#define	_SFRAME_H

#include <sys/types.h>
#include <limits.h>
#include <stdint.h>

#include "ansidecl.h"

#ifdef	__cplusplus
extern "C"
{
#endif

/* SFrame format.

   SFrame format is a simple format to represent the information needed
   for generating vanilla backtraces.  SFrame format keeps track of the
   minimal necessary information needed for stack tracing:
     - Canonical Frame Address (CFA)
     - Frame Pointer (FP)
     - Return Address (RA)

   The SFrame section itself has the following structure:

       +--------+------------+---------+
       |  file  |  function  | frame   |
       | header | descriptor |  row    |
       |        |   entries  | entries |
       +--------+------------+---------+

   The file header stores a magic number and version information, flags, and
   the byte offset of each of the sections relative to the end of the header
   itself.  The file header also specifies the total number of Function
   Descriptor Entries, Frame Row Entries and length of the FRE sub-section.

   Following the header is a list of Function Descriptor Entries (FDEs).
   This list may be sorted if the flags in the file header indicate it to be
   so.  The sort order, if applicable, is the order of functions in the
   .text.* sections in the resulting binary artifact.  Each Function
   Descriptor Entry specifies the start PC of a function, the size in bytes
   of the function and an offset to its first Frame Row Entry (FRE).  Each FDE
   additionally also specifies the type of FRE it uses to encode the stack
   trace information.

   Next, the SFrame Frame Row Entry sub-section is a list of variable size
   records.  Each entry represents stack trace information for a set of PCs
   of the function.  A singular Frame Row Entry is a self-sufficient record
   which contains information on how to generate stack trace from the
   applicable set of PCs.

   */


/* SFrame format versions.  */
#define SFRAME_VERSION_1	1
#define SFRAME_VERSION_2	2
#define SFRAME_VERSION_3        3
/* SFrame magic number.  */
#define SFRAME_MAGIC		0xdee2
/* Current version of SFrame format.  */
#define SFRAME_VERSION	SFRAME_VERSION_3

/* Various flags for SFrame.  */

/* Function Descriptor Entries are sorted on PC.  */
#define SFRAME_F_FDE_SORTED		    0x1
/* Functions preserve frame pointer.  */
#define SFRAME_F_FRAME_POINTER		    0x2
/* Function start address in SFrame FDE is encoded as the distance from the
   location of the sfde_func_start_address to the start PC of the function.
   If absent, the function start address in SFrame FDE is encoded as the
   distance from the start of the SFrame FDE section to the start PC of the
   function.  */
#define SFRAME_F_FDE_FUNC_START_PCREL	    0x4

/* Set of all defined flags in SFrame V2.  */
#define SFRAME_V2_F_ALL_FLAGS \
  (SFRAME_F_FDE_SORTED | SFRAME_F_FRAME_POINTER \
   | SFRAME_F_FDE_FUNC_START_PCREL)

/* Set of all defined flags in SFrame V3.  */
#define SFRAME_V3_F_ALL_FLAGS \
  (SFRAME_F_FDE_SORTED | SFRAME_F_FDE_FUNC_START_PCREL)

#define SFRAME_CFA_FIXED_FP_INVALID 0
#define SFRAME_CFA_FIXED_RA_INVALID 0

/* Supported ABIs/Arch.  */
#define SFRAME_ABI_AARCH64_ENDIAN_BIG      1 /* AARCH64 big endian.  */
#define SFRAME_ABI_AARCH64_ENDIAN_LITTLE   2 /* AARCH64 little endian.  */
#define SFRAME_ABI_AMD64_ENDIAN_LITTLE     3 /* AMD64 little endian.  */
#define SFRAME_ABI_S390X_ENDIAN_BIG        4 /* s390x big endian.  */

/* SFrame FRE types.  */
#define SFRAME_FRE_TYPE_ADDR1	0
#define SFRAME_FRE_TYPE_ADDR2	1
#define SFRAME_FRE_TYPE_ADDR4	2

/* SFrame Function Descriptor Entry PC types.

   The SFrame format has two possible representations for functions' PC Type.
   The choice of which PC type to use is made according to the instruction
   patterns in the relevant program stub.

   For more details, see the (renamed to) entries SFRAME_V3_FDE_PCTYPE_INC and
   SFRAME_V3_FDE_PCTYPE_MASK for SFrame V3.  */

/* Unwinders perform a (PC >= FRE_START_ADDR) to look up a matching FRE.
   NB: Use SFRAME_V3_FDE_PCTYPE_INC in SFrame V3 instead.  */
#define SFRAME_FDE_TYPE_PCINC   0
/* Unwinders perform a (PC % REP_BLOCK_SIZE >= FRE_START_ADDR) to look up a
   matching FRE.  NB: Use SFRAME_V3_FDE_PCTYPE_MASK in SFrame V3 instead.  */
#define SFRAME_FDE_TYPE_PCMASK  1

/* SFrame FDE types.  */

/* Default FDE type.  */
#define SFRAME_FDE_TYPE_DEFAULT	  0
/* Flexible Frame FDE type.
   The recovery rule for CFA, RA and FP allow more flexibility.  Examples of
   patterns supported include:
     - CFA may be non-SP/FP based.
     - CFA, FP may encode dereferencing of register after offset adjustment
     - RA may be in a non-default register.
   Currently used for SFRAME_ABI_AMD64_ENDIAN_LITTLE.  */
#define SFRAME_FDE_TYPE_FLEX	  1

typedef struct sframe_preamble
{
  uint16_t sfp_magic;	/* Magic number (SFRAME_MAGIC).  */
  uint8_t sfp_version;	/* Data format version number (SFRAME_VERSION).  */
  uint8_t sfp_flags;	/* Flags.  */
} ATTRIBUTE_PACKED sframe_preamble;

typedef struct sframe_header
{
  sframe_preamble sfh_preamble;
  /* Information about the arch (endianness) and ABI.  */
  uint8_t sfh_abi_arch;
  /* Offset for the Frame Pointer (FP) from CFA may be fixed for some
     ABIs (e.g, in AMD64 when -fno-omit-frame-pointer is used).  When fixed,
     this field specifies the fixed stack frame offset and the individual
     FREs do not need to track it.  When not fixed, it is set to
     SFRAME_CFA_FIXED_FP_INVALID, and the individual FREs may provide
     the applicable stack frame offset, if any.  */
  int8_t sfh_cfa_fixed_fp_offset;
  /* Offset for the Return Address from CFA is fixed for some ABIs
     (e.g., AMD64 has it as CFA-8).  When fixed, the header specifies the
     fixed stack frame offset and the individual FREs do not track it.  When
     not fixed, it is set to SFRAME_CFA_FIXED_RA_INVALID, and individual
     FREs provide the applicable stack frame offset, if any.  */
  int8_t sfh_cfa_fixed_ra_offset;
  /* Number of bytes making up the auxiliary header, if any.
     Some ABI/arch, in the future, may use this space for extending the
     information in SFrame header.  Auxiliary header is contained in
     bytes sequentially following the sframe_header.  */
  uint8_t sfh_auxhdr_len;
  /* Number of SFrame FDEs in this SFrame section.  */
  uint32_t sfh_num_fdes;
  /* Number of SFrame Frame Row Entries.  */
  uint32_t sfh_num_fres;
  /* Number of bytes in the SFrame Frame Row Entry section. */
  uint32_t sfh_fre_len;
  /* Offset of SFrame Function Descriptor Entry section.  */
  uint32_t sfh_fdeoff;
  /* Offset of SFrame Frame Row Entry section.  */
  uint32_t sfh_freoff;
} ATTRIBUTE_PACKED sframe_header;

#define SFRAME_V1_HDR_SIZE(sframe_hdr)	\
  ((sizeof (sframe_header) + (sframe_hdr).sfh_auxhdr_len))

/* Two possible keys for executable (instruction) pointers signing.  */
#define SFRAME_AARCH64_PAUTH_KEY_A    0 /* Key A.  */
#define SFRAME_AARCH64_PAUTH_KEY_B    1 /* Key B.  */

typedef struct sframe_func_desc_entry_v2
{
  /* Function start address.  Encoded as a signed offset, relative to the
     beginning of the current FDE.  */
  int32_t sfde_func_start_address;
  /* Size of the function in bytes.  */
  uint32_t sfde_func_size;
  /* Offset of the first SFrame Frame Row Entry of the function, relative to the
     beginning of the SFrame Frame Row Entry sub-section.  */
  uint32_t sfde_func_start_fre_off;
  /* Number of frame row entries for the function.  */
  uint32_t sfde_func_num_fres;
  /* Additional information for stack tracing from the function:
     - 4-bits: Identify the FRE type used for the function.
     - 1-bit: Identify the FDE type of the function - mask or inc.
     - 1-bit: PAC authorization A/B key (aarch64).
     - 2-bits: Unused.
     ------------------------------------------------------------------------
     |     Unused    |  PAC auth A/B key (aarch64) |  FDE type |   FRE type   |
     |               |     Unused (amd64, s390x)   |           |              |
     ------------------------------------------------------------------------
     8               6                             5           4              0     */
  uint8_t sfde_func_info;
  /* Size of the block of repeating insns.  Used for SFrame FDEs of type
     SFRAME_FDE_TYPE_PCMASK.  */
  uint8_t sfde_func_rep_size;
  uint16_t sfde_func_padding2;
} ATTRIBUTE_PACKED sframe_func_desc_entry_v2;

/* Macros to compose and decompose function info in FDE.  */

/* Note: Set PAC auth key to SFRAME_AARCH64_PAUTH_KEY_A by default.  */
#define SFRAME_V1_FUNC_INFO(fde_type, fre_enc_type) \
  (((SFRAME_AARCH64_PAUTH_KEY_A & 0x1) << 5) | \
   (((fde_type) & 0x1) << 4) | ((fre_enc_type) & 0xf))

#define SFRAME_V1_FUNC_FRE_TYPE(data)	  ((data) & 0xf)
#define SFRAME_V1_FUNC_FDE_TYPE(data)	  (((data) >> 4) & 0x1)
#define SFRAME_V1_FUNC_PAUTH_KEY(data)	  (((data) >> 5) & 0x1)

/* Set the pauth key as indicated.  */
#define SFRAME_V1_FUNC_INFO_UPDATE_PAUTH_KEY(pauth_key, fde_info) \
  ((((pauth_key) & 0x1) << 5) | ((fde_info) & 0xdf))

/* SFrame V2 has similar SFrame FDE representation as SFrame V1.  */

#define SFRAME_V2_FUNC_INFO(fde_type, fre_enc_type) \
  (SFRAME_V1_FUNC_INFO (fde_type, fre_enc_type))

#define SFRAME_V2_FUNC_FRE_TYPE(data)    (SFRAME_V1_FUNC_FRE_TYPE (data))
#define SFRAME_V2_FUNC_PC_TYPE(data)     (SFRAME_V1_FUNC_FDE_TYPE (data))
#define SFRAME_V2_FUNC_PAUTH_KEY(data)   (SFRAME_V1_FUNC_PAUTH_KEY (data))

#define SFRAME_V2_FUNC_INFO_UPDATE_PAUTH_KEY(pauth_key, fde_info) \
  SFRAME_V1_FUNC_INFO_UPDATE_PAUTH_KEY (pauth_key, fde_info)

/* SFrame Function Descriptor Entry PC types.

   The SFrame format has two possible representations for functions' PC Type.
   The choice of which PC type to use is made according to the instruction
   patterns in the relevant program stub.

   The PC type SFRAME_V3_FDE_PCTYPE_INC is an indication that the PCs in the
   FREs should be treated as increments in bytes.  This is used for a bulk of
   the executable code of a program, which contains instructions with no
   specific pattern.

   The PC type SFRAME_V3_FDE_PCTYPE_MASK is an indication that the PCs in the
   FREs should be treated as masks.  This type is useful for the cases when a
   small pattern of instructions in a program stub is repeatedly to cover a
   specific functionality.  Typical usescases are pltN entries, trampolines
   etc.

   NB: In SFrame version 2 or lower, the names SFRAME_FDE_TYPE_PCINC and
   SFRAME_FDE_TYPE_PCMASK were used.  */

/* Unwinders perform a (PC >= FRE_START_ADDR) to look up a matching FRE.  */
#define SFRAME_V3_FDE_PCTYPE_INC   SFRAME_FDE_TYPE_PCINC
/* Unwinders perform a (PC % REP_BLOCK_SIZE >= FRE_START_ADDR) to look up a
   matching FRE.  */
#define SFRAME_V3_FDE_PCTYPE_MASK  SFRAME_FDE_TYPE_PCMASK

typedef struct sframe_func_desc_idx_v3
{
  /* Offset to the function start address.  Encoded as a signed offset,
     relative to the beginning of the current FDE.  */
  int64_t sfdi_func_start_offset;
  /* Size of the function in bytes.  */
  uint32_t sfdi_func_size;
  /* Offset of the first SFrame Frame Row Entry of the function, relative to the
     beginning of the SFrame Frame Row Entry sub-section.  */
  uint32_t sfdi_func_start_fre_off;
} ATTRIBUTE_PACKED sframe_func_desc_idx_v3;

typedef struct sframe_func_desc_attr_v3
{
  /* Number of frame row entries for the function.  */
  uint16_t sfda_func_num_fres;
  /* Additional information for stack tracing from the function:
     - 4-bits: Identify the FRE type used for the function.
     - 1-bit: Identify the PC type of the function - mask or inc.
     - 1-bit: PAC authorization A/B key (aarch64).
     - 1-bits: Unused.
     - 1-bit: Signal frame.
     -------------------------------------------------------------------------------
     | Signal |   Unused    |  PAC auth A/B key (aarch64) |   FDE   |   FRE Type   |
     | frame  |             |        Unused (amd64)       | PC Type |              |
     -------------------------------------------------------------------------------
     8        7             6                             5         4              0     */
  uint8_t sfda_func_info;
  /* Additional information for stack tracing from the function:
     - 5-bits: FDE type.
     - 3-bits: Unused.
     ------------------------------------------------------------
     |                     Unused                 |  FDE Type   |
     |                                            |             |
     ------------------------------------------------------------
     8                7             6             5             0     */
  uint8_t sfda_func_info2;
  /* Size of the block of repeating insns.  Used for SFrame FDEs of type
     SFRAME_V3_FDE_PCTYPE_MASK.  */
  uint8_t sfda_func_rep_size;
} ATTRIBUTE_PACKED sframe_func_desc_attr_v3;

#define SFRAME_V3_FDE_FUNC_INFO(fde_pc_type, fre_type) \
  (SFRAME_V2_FUNC_INFO (fde_pc_type, fre_type))

/* Mask for the ABI/arch specific FDE type (lower 5 bits).  */
#define SFRAME_V3_FDE_TYPE_MASK		      0x01f
/* Get the FDE type from the info2 byte.  */
#define SFRAME_V3_FDE_TYPE(info2) \
  ((info2) & SFRAME_V3_FDE_TYPE_MASK)
#define SFRAME_V3_FDE_FRE_TYPE(info)          (SFRAME_V2_FUNC_FRE_TYPE (info))
#define SFRAME_V3_FDE_PC_TYPE(info)           (SFRAME_V2_FUNC_PC_TYPE (info))
#define SFRAME_V3_AARCH64_FDE_PAUTH_KEY(info) (SFRAME_V2_FUNC_PAUTH_KEY (info))
#define SFRAME_V3_FDE_SIGNAL_P(info)           (((info) >> 7) & 0x1)

/* Set the FDE type in the info2 byte, preserving upper bits.  */
#define SFRAME_V3_SET_FDE_TYPE(info2, fde_type) \
  (((info2) & ~SFRAME_V3_FDE_TYPE_MASK) \
   | ((fde_type) & SFRAME_V3_FDE_TYPE_MASK))

#define SFRAME_V3_FDE_UPDATE_PAUTH_KEY(pauth_key, info) \
  SFRAME_V2_FUNC_INFO_UPDATE_PAUTH_KEY (pauth_key, info)

#define SFRAME_V3_FDE_UPDATE_SIGNAL_P(signal_p, info)  \
  ((((signal_p) & 0x1) << 7) | ((info) & 0x7f))

#define SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE(reg, deref_p, reg_p) \
  ((((reg) << 0x3) | (0 << 0x2) | (((deref_p) & 0x1) << 0x1) | ((reg_p) & 0x1)))

#define SFRAME_V3_FLEX_FDE_CTRLWORD_REGNUM(data)    ((data) >> 3)
#define SFRAME_V3_FLEX_FDE_CTRLWORD_DEREF_P(data)   (((data) >> 1) & 0x1)
#define SFRAME_V3_FLEX_FDE_CTRLWORD_REG_P(data)     ((data) & 0x1)

#define SFRAME_V3_FRE_RA_UNDEFINED_P(fre_info) \
  (SFRAME_V2_FRE_RA_UNDEFINED_P (fre_info))

/* Size of stack frame offsets in an SFrame Frame Row Entry.  A single
   SFrame FRE has all offsets of the same size.  Offset size may vary
   across frame row entries.  */
#define SFRAME_FRE_OFFSET_1B	  0
#define SFRAME_FRE_OFFSET_2B	  1
#define SFRAME_FRE_OFFSET_4B	  2

/* In SFrame V3, with the addition of flexible FDE, usage of term "offsets"
   (for the varlen data trailing the SFrame FRE) is inappropriate.  Use the
   terminology of "data word" instead.  A single SFrame FRE has all data words
   of the same size.  Size of data words may vary across frame row entries.  */
#define SFRAME_FRE_DATAWORD_1B	    SFRAME_FRE_OFFSET_1B
#define SFRAME_FRE_DATAWORD_2B	    SFRAME_FRE_OFFSET_2B
#define SFRAME_FRE_DATAWORD_4B	    SFRAME_FRE_OFFSET_4B

/* An SFrame Frame Row Entry can be SP or FP based.  */
#define SFRAME_BASE_REG_FP	0
#define SFRAME_BASE_REG_SP	1

/* The index at which a specific offset is presented in the variable length
   bytes of an FRE.  */
#define SFRAME_FRE_CFA_OFFSET_IDX   0
/* The RA stack offset, if present, will always be at index 1 in the variable
   length bytes of the FRE.  */
#define SFRAME_FRE_RA_OFFSET_IDX    1
/* The FP stack offset may appear at offset 1 or 2, depending on the ABI as RA
   may or may not be tracked.  */
#define SFRAME_FRE_FP_OFFSET_IDX    2

/* Invalid RA offset.  Currently used for s390x as padding to represent FP
   without RA saved.  */
#define SFRAME_FRE_RA_OFFSET_INVALID 0

typedef struct sframe_fre_info
{
  /* Information about
     - 1 bit: base reg for CFA
     - 4 bits: Number of data words (N).  Typically for default FDE type, a
     value of upto 3 suffices to track all three of CFA, FP and RA (fixed
     implicit order).
     - 2 bits: information about size of the data words (S) in bytes.
     Valid values are SFRAME_FRE_DATAWORD_1B, SFRAME_FRE_DATAWORD_2B,
     SFRAME_FRE_DATAWORD_4B.
     - 1 bit: Mangled RA state bit (aarch64 only).
     ----------------------------------------------------------------------------------
     | Mangled-RA (aarch64) | Size of Data Words |  Number of Data Words  |   base_reg |
     | Unused (amd64, s390x)|                    |                        |            |
     ----------------------------------------------------------------------------------
     8                     7                    5                        1            0

     */
  uint8_t fre_info;
} sframe_fre_info;

/* Macros to compose and decompose FRE info.  */

/* Note: Set mangled_ra_p to zero by default.  */
#define SFRAME_V1_FRE_INFO(base_reg_id, offset_num, offset_size) \
  (((0 & 0x1) << 7) | (((offset_size) & 0x3) << 5) | \
   (((offset_num) & 0xf) << 1) | ((base_reg_id) & 0x1))

/* Set the mangled_ra_p bit as indicated.  */
#define SFRAME_V1_FRE_INFO_UPDATE_MANGLED_RA_P(mangled_ra_p, fre_info) \
  ((((mangled_ra_p) & 0x1) << 7) | ((fre_info) & 0x7f))

#define SFRAME_V1_FRE_CFA_BASE_REG_ID(data)	  ((data) & 0x1)
#define SFRAME_V1_FRE_OFFSET_COUNT(data)	  (((data) >> 1) & 0xf)
#define SFRAME_V1_FRE_OFFSET_SIZE(data)		  (((data) >> 5) & 0x3)
#define SFRAME_V1_FRE_MANGLED_RA_P(data)	  (((data) >> 7) & 0x1)
#define SFRAME_V2_FRE_RA_UNDEFINED_P(data)	  (SFRAME_V1_FRE_OFFSET_COUNT (data) == 0)

/* In SFrame V3, with the introduction of flexible FDE type
   SFRAME_FDE_TYPE_FLEX, the variable-length data following SFrame FRE header
   may contain unsigned Control Data Words or signed Offset Data Words.  These
   are referred to as 'Data Words'.  Note that the usage of the term 'Word'
   here is colloquial, the size of a data word is determined by applicable
   bits.  */
#define SFRAME_V3_FRE_DATAWORD_COUNT(data)	\
  SFRAME_V1_FRE_OFFSET_COUNT (data)
#define SFRAME_V3_FRE_DATAWORD_SIZE(data) \
  SFRAME_V1_FRE_OFFSET_SIZE (data)

/* SFrame Frame Row Entry definitions.

   Used for Default FDEs in AMD64, AARCH64, and s390x.

   An SFrame Frame Row Entry is a self-sufficient record which contains
   information on how to generate the stack trace for the specified range of
   PCs.  Each SFrame Frame Row Entry is followed by S*N bytes, where:
     S is the size of the stack frame offset for the FRE, and
     N is the number of stack frame offsets in the FRE

   The interpretation of FRE stack offsets for default FDEs is ABI-specific:

   AMD64:
     offset1 (interpreted as CFA = BASE_REG + offset1)
      if FP is being tracked
	offset2 (intrepreted as FP = CFA + offset2)
      fi

    AARCH64:
     offset1 (interpreted as CFA = BASE_REG + offset1)
     if FP is being tracked (in other words, if frame record created)
       offset2 (interpreted as RA = CFA + offset2)
       offset3 (intrepreted as FP = CFA + offset3)
     fi
     Note that in AAPCS64, a frame record, if created, will save both FP and
     LR on stack.

   s390x:
     offset1 (interpreted as CFA = BASE_REG + offset1)
     if RA is being tracked
       offset2 (interpreted as RA = CFA + offset2; an offset value of
	       SFRAME_FRE_RA_OFFSET_INVALID indicates a dummy padding RA offset
	       to represent FP without RA saved on stack)
       if FP is being tracked
	 offset3 (intrepreted as FP = CFA + offset3)
       fi
     else
      if FP is being tracked
	offset2 (intrepreted as FP = CFA + offset2)
      fi
    fi
    Note that in s390x, if a FP/RA is to be restored from a register, flex FDEs
    are used in SFrame V3.  In SFrame V2, default FDEs were used: the
    least-significant bit of the offset was set to indicate that the encoded
    value is a DWARF register number shifted to the left by 1.
*/

/* Used when SFRAME_FRE_TYPE_ADDR1 is specified as FRE type.  */
typedef struct sframe_frame_row_entry_addr1
{
  /* Start address of the frame row entry.  Encoded as an 1-byte unsigned
     offset, relative to the start address of the function.  */
  uint8_t sfre_start_address;
  sframe_fre_info sfre_info;
} ATTRIBUTE_PACKED sframe_frame_row_entry_addr1;

/* Upper limit of start address in sframe_frame_row_entry_addr1
   is 0x100 (not inclusive).  */
#define SFRAME_FRE_TYPE_ADDR1_LIMIT   \
  (1ULL << ((SFRAME_FRE_TYPE_ADDR1 + 1) * 8))

/* Used when SFRAME_FRE_TYPE_ADDR2 is specified as FRE type.  */
typedef struct sframe_frame_row_entry_addr2
{
  /* Start address of the frame row entry.  Encoded as an 2-byte unsigned
     offset, relative to the start address of the function.  */
  uint16_t sfre_start_address;
  sframe_fre_info sfre_info;
} ATTRIBUTE_PACKED sframe_frame_row_entry_addr2;

/* Upper limit of start address in sframe_frame_row_entry_addr2
   is 0x10000 (not inclusive).  */
#define SFRAME_FRE_TYPE_ADDR2_LIMIT   \
  (1ULL << ((SFRAME_FRE_TYPE_ADDR2 * 2) * 8))

/* Used when SFRAME_FRE_TYPE_ADDR4 is specified as FRE type.  */
typedef struct sframe_frame_row_entry_addr4
{
  /* Start address of the frame row entry.  Encoded as a 4-byte unsigned
     offset, relative to the start address of the function.  */
  uint32_t sfre_start_address;
  sframe_fre_info sfre_info;
} ATTRIBUTE_PACKED sframe_frame_row_entry_addr4;

/* Upper limit of start address in sframe_frame_row_entry_addr2
   is 0x100000000 (not inclusive).  */
#define SFRAME_FRE_TYPE_ADDR4_LIMIT   \
  (1ULL << ((SFRAME_FRE_TYPE_ADDR4 * 2) * 8))

/* On s390x, the CFA offset from CFA base register is by definition a minimum
   of 160.  Store it adjusted by -160 to enable use of 8-bit SFrame offsets.
   Additionally scale by an alignment factor of 8, as the SP and thus CFA
   offset on s390x is always 8-byte aligned.  */
#define SFRAME_S390X_CFA_OFFSET_ADJUSTMENT		SFRAME_S390X_SP_VAL_OFFSET
#define SFRAME_S390X_CFA_OFFSET_ALIGNMENT_FACTOR	8
#define SFRAME_V2_S390X_CFA_OFFSET_ENCODE(offset) \
  (((offset) + SFRAME_S390X_CFA_OFFSET_ADJUSTMENT) \
   / SFRAME_S390X_CFA_OFFSET_ALIGNMENT_FACTOR)
#define SFRAME_V2_S390X_CFA_OFFSET_DECODE(offset) \
  (((offset) * SFRAME_S390X_CFA_OFFSET_ALIGNMENT_FACTOR) \
   - SFRAME_S390X_CFA_OFFSET_ADJUSTMENT)

/* On s390x, the CFA is defined as SP at call site + 160.  Therefore the
   SP value offset from CFA is -160.  */
#define SFRAME_S390X_SP_VAL_OFFSET			(-160)

/* On s390x, the FP and RA registers can be saved either on the stack or,
   in case of leaf functions, in registers.  Store DWARF register numbers
   encoded as offset by using the least-significant bit (LSB) as indicator:
   - LSB=0: Stack offset.  The s390x ELF ABI mandates that stack register
     slots must be 8-byte aligned.
   - LSB=1: DWARF register number shifted to the left by one.  */
#define SFRAME_V2_S390X_OFFSET_IS_REGNUM(offset) \
  ((offset) & 1)
#define SFRAME_V2_S390X_OFFSET_ENCODE_REGNUM(regnum) \
  (((regnum) << 1) | 1)
#define SFRAME_V2_S390X_OFFSET_DECODE_REGNUM(offset) \
  ((offset) >> 1)

#ifdef	__cplusplus
}
#endif

#endif				/* _SFRAME_H */
