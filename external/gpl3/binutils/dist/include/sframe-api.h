/* Public API to SFrame.

   Copyright (C) 2022-2026 Free Software Foundation, Inc.

   This file is part of libsframe.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef	_SFRAME_API_H
#define	_SFRAME_API_H

#include <sframe.h>
#include <stdbool.h>

#ifdef	__cplusplus
extern "C"
{
#endif

typedef struct sframe_decoder_ctx sframe_decoder_ctx;
typedef struct sframe_encoder_ctx sframe_encoder_ctx;

#define MAX_NUM_DATAWORDS	6

#define MAX_DATAWORD_BYTES  \
  ((SFRAME_FRE_DATAWORD_4B * 2 * MAX_NUM_DATAWORDS))

/* User interfacing SFrame Row Entry.
   An abstraction provided by libsframe so the consumer is decoupled from
   the binary format representation of the same.

   The members are best ordered such that they are aligned at their natural
   boundaries.  This helps avoid usage of undesirable misaligned memory
   accesses.  See PR libsframe/29856.  */

typedef struct sframe_frame_row_entry
{
  uint32_t fre_start_addr;
  unsigned char fre_datawords[MAX_DATAWORD_BYTES];
  unsigned char fre_info;
} sframe_frame_row_entry;

#define SFRAME_ERR ((int) -1)

/* This macro holds information about all the available SFrame
   errors.  It is used to form both an enum holding all the error
   constants, and also the error strings themselves.  To use, define
   _SFRAME_FIRST and _SFRAME_ITEM to expand as you like, then
   mention the macro name.  See the enum after this for an example.  */
#define _SFRAME_ERRORS \
  _SFRAME_FIRST (SFRAME_ERR_VERSION_INVAL, "SFrame version not supported.") \
  _SFRAME_ITEM (SFRAME_ERR_NOMEM, "Out of Memory.") \
  _SFRAME_ITEM (SFRAME_ERR_INVAL, "Corrupt SFrame.") \
  _SFRAME_ITEM (SFRAME_ERR_BUF_INVAL, "Buffer does not contain SFrame data.") \
  _SFRAME_ITEM (SFRAME_ERR_DCTX_INVAL, "Corrupt SFrame decoder.") \
  _SFRAME_ITEM (SFRAME_ERR_ECTX_INVAL, "Corrupt SFrame encoder.") \
  _SFRAME_ITEM (SFRAME_ERR_FDE_INVAL, "Corrput FDE.") \
  _SFRAME_ITEM (SFRAME_ERR_FRE_INVAL, "Corrupt FRE.") \
  _SFRAME_ITEM (SFRAME_ERR_FDE_NOTFOUND,"FDE not found.") \
  _SFRAME_ITEM (SFRAME_ERR_FDE_NOTSORTED, "FDEs not sorted.") \
  _SFRAME_ITEM (SFRAME_ERR_FRE_NOTFOUND,"FRE not found.") \
  _SFRAME_ITEM (SFRAME_ERR_FREOFFSET_NOPRESENT,"FRE offset not present.")

#define	SFRAME_ERR_BASE	2000	/* Base value for libsframe errnos.  */

enum
  {
#define _SFRAME_FIRST(NAME, STR) NAME = SFRAME_ERR_BASE
#define _SFRAME_ITEM(NAME, STR) , NAME
_SFRAME_ERRORS
#undef _SFRAME_ITEM
#undef _SFRAME_FIRST
  };

/* Count of SFrame errors.  */
#define SFRAME_ERR_NERR (SFRAME_ERR_FREOFFSET_NOPRESENT - SFRAME_ERR_BASE + 1)

/* Get the error message string.  */

extern const char *
sframe_errmsg (int error);

/* Create an FDE function info bye given an FRE_TYPE and an FDE_TYPE.  */

extern unsigned char
sframe_fde_create_func_info (uint32_t fre_type, uint32_t fde_type);

/* Gather the FRE type given the function size.  */

extern uint32_t
sframe_calc_fre_type (size_t func_size);

/* The SFrame Decoder.  */

/* Decode the specified SFrame buffer SF_BUF of size SF_SIZE and return the
   new SFrame decoder context.  Sets ERRP for the caller if any error.  */
extern sframe_decoder_ctx *
sframe_decode (const char *sf_buf, size_t sf_size, int *errp);

/* Free the decoder context.  */
extern void
sframe_decoder_free (sframe_decoder_ctx **dctx);

/* Get the size of the SFrame header from the decoder context DCTX.  */
extern unsigned int
sframe_decoder_get_hdr_size (const sframe_decoder_ctx *dctx);

/* Get the SFrame's abi/arch info.  */
extern uint8_t
sframe_decoder_get_abi_arch (const sframe_decoder_ctx *dctx);

/* Get the format version from the SFrame decoder context DCTX.  */
extern uint8_t
sframe_decoder_get_version (const sframe_decoder_ctx *dctx);

/* Get the section flags from the SFrame decoder context DCTX.  */
extern uint8_t
sframe_decoder_get_flags (const sframe_decoder_ctx *dctx);

/* Get the offset of the sfde_func_start_address field (from the start of the
   on-disk layout of the SFrame section) of the FDE at FUNC_IDX in the decoder
   context DCTX.

   If FUNC_IDX is more than the number of SFrame FDEs in the section, sets
   error code in ERRP, but returns the (hypothetical) offset.  This is useful
   for the linker when arranging input FDEs into the output section to be
   emitted.  */
uint32_t
sframe_decoder_get_offsetof_fde_start_addr (const sframe_decoder_ctx *dctx,
					    uint32_t func_idx, int *errp);

/* Return the number of function descriptor entries in the SFrame decoder
   DCTX.  */
extern uint32_t
sframe_decoder_get_num_fidx (const sframe_decoder_ctx *dctx);

/* Get the fixed FP offset from the decoder context DCTX.  */
extern int8_t
sframe_decoder_get_fixed_fp_offset (const sframe_decoder_ctx *dctx);

/* Get the fixed RA offset from the decoder context DCTX.  */
extern int8_t
sframe_decoder_get_fixed_ra_offset (const sframe_decoder_ctx *dctx);

/* Find the SFrame Frame Row Entry which contains the PC.  Returns
   SFRAME_ERR if failure.  */

extern int
sframe_find_fre (const sframe_decoder_ctx *ctx, int64_t pc,
		 sframe_frame_row_entry *frep);

/* Get the FRE_IDX'th FRE of the function at FUNC_IDX'th function
   index entry in the SFrame decoder CTX.  Returns error code as
   applicable.  */
extern int
sframe_decoder_get_fre (const sframe_decoder_ctx *ctx,
			unsigned int func_idx,
			unsigned int fre_idx,
			sframe_frame_row_entry *fre);

/* Get the SFrame FRE data of the function at FUNC_IDX'th function index entry
   in the SFrame decoder DCTX.  The reference to the buffer is returned in
   FRES_BUF with FRES_BUF_SIZE indicating the size of the buffer.  The number
   of FREs in the buffer are NUM_FRES.  In SFrame V3, this buffer also contains
   the FDE attr data before the actual SFrame FREs.  Returns SFRAME_ERR in case
   of error.  */
extern int
sframe_decoder_get_fres_buf (const sframe_decoder_ctx *dctx,
			     uint32_t func_idx,
			     char **fres_buf,
			     size_t *fres_buf_size,
			     uint32_t *num_fres);

/* Get the data (NUM_FRES, FUNC_SIZE, FUNC_START_ADDRESS, FUNC_INFO,
   REP_BLOCK_SIZE) from the function descriptor entry at index I'th
   in the decoder CTX.  If failed, return error code.
   This API is only available from SFRAME_VERSION_2.  */
extern int
sframe_decoder_get_funcdesc_v2 (const sframe_decoder_ctx *ctx,
				unsigned int i,
				uint32_t *num_fres,
				uint32_t *func_size,
				int32_t *func_start_address,
				unsigned char *func_info,
				uint8_t *rep_block_size);

/* Get the data (NUM_FRES, FUNC_SIZE, START_PC_OFFSET, FUNC_INFO, FUNC_INFO2,
   REP_BLOCK_SIZE) from the SFrame function descriptor entry at the I'th index
   in the decoder object DCTX.  Return SFRAME_ERR on failure.  */
extern int
sframe_decoder_get_funcdesc_v3 (const sframe_decoder_ctx *dctx,
				unsigned int i,
				uint32_t *num_fres,
				uint32_t *func_size,
				int64_t *start_pc_offset,
				unsigned char *func_info,
				unsigned char *func_info2,
				uint8_t *rep_block_size);

/* SFrame textual dump.  */
extern void
dump_sframe (const sframe_decoder_ctx *decoder, uint64_t addr);

/* Get IDX'th offset as unsigned data from FRE.  Set errp as applicable.  */
extern uint32_t
sframe_get_fre_udata (const sframe_frame_row_entry *fre, int idx, int *errp);

/* Get the base reg id from the FRE info.  Sets errp if fails.  */
extern uint8_t
sframe_fre_get_base_reg_id (const sframe_frame_row_entry *fre, int *errp);

/* Get the CFA offset from the FRE.  If the offset is invalid, sets errp.  */
extern int32_t
sframe_fre_get_cfa_offset (const sframe_decoder_ctx *dtcx,
			   const sframe_frame_row_entry *fre,
			   uint32_t fde_type,
			   int *errp);

/* Get the FP offset from the FRE.  If the offset is invalid, sets errp.

   For s390x the offset may be an encoded register number, indicated by
   LSB set to one, which is only valid in the topmost frame.  */
extern int32_t
sframe_fre_get_fp_offset (const sframe_decoder_ctx *dctx,
			  const sframe_frame_row_entry *fre,
			  uint32_t fde_type,
			  int *errp);

/* Get the RA offset from the FRE.  If the offset is invalid, sets errp.

   For s390x an RA offset value of SFRAME_FRE_RA_OFFSET_INVALID indicates
   that the RA is not saved, which is only valid in the topmost frame.
   For s390x the offset may be an encoded register number, indicated by
   LSB set to one, which is only valid in the topmost frame.  */
extern int32_t
sframe_fre_get_ra_offset (const sframe_decoder_ctx *dctx,
			  const sframe_frame_row_entry *fre,
			  uint32_t fde_type,
			  int *errp);

/* Get whether the RA is mangled.  */

extern bool
sframe_fre_get_ra_mangled_p (const sframe_decoder_ctx *dctx,
			     const sframe_frame_row_entry *fre, int *errp);

/* Get whether the RA is undefined (i.e. outermost frame).  */

bool
sframe_fre_get_ra_undefined_p (const sframe_decoder_ctx *dctx ATTRIBUTE_UNUSED,
			       const sframe_frame_row_entry *fre, int *errp);

/* The SFrame Encoder.  */

/* Create an encoder context with the given SFrame format version VER, FLAGS
   and ABI information.  Sets errp if failure.  */
extern sframe_encoder_ctx *
sframe_encode (uint8_t ver, uint8_t flags, uint8_t abi_arch,
	       int8_t fixed_fp_offset, int8_t fixed_ra_offset, int *errp);

/* Free the encoder context ECTXP.  */
extern void
sframe_encoder_free (sframe_encoder_ctx **ectxp);

/* Get the size of the SFrame header from the encoder context ECTX.  */
extern unsigned int
sframe_encoder_get_hdr_size (sframe_encoder_ctx *ectx);

/* Get the SFrame abi/arch info from the encoder context ECTX.  */
extern uint8_t
sframe_encoder_get_abi_arch (sframe_encoder_ctx *ectx);

/* Get the SFrame format version from the encoder context ECTX.  */
extern uint8_t
sframe_encoder_get_version (sframe_encoder_ctx *ectx);

/* Get the SFrame flags from the encoder context ECTX.  */
extern uint8_t
sframe_encoder_get_flags (sframe_encoder_ctx *ectx);

/* Get the offset of the sfde_func_start_address field (from the start of the
   on-disk layout of the SFrame section) of the FDE at FUNC_IDX in the encoder
   context ECTX.

   If FUNC_IDX is more than the number of SFrame FDEs in the section, sets
   error code in ERRP, but returns the (hypothetical) offset.  This is useful
   for the linker when arranging input FDEs into the output section to be
   emitted.  */
uint32_t
sframe_encoder_get_offsetof_fde_start_addr (sframe_encoder_ctx *ectx,
					    uint32_t func_idx, int *errp);

/* Return the number of SFrame function descriptor entries in the encoder
   context ECTX.  */
extern uint32_t
sframe_encoder_get_num_fidx (sframe_encoder_ctx *ectx);

/* Add an SFrame FRE to function at FUNC_IDX'th function descriptor entry in
   the encoder context ECTX.  */
extern int
sframe_encoder_add_fre (sframe_encoder_ctx *ectx,
			unsigned int func_idx,
			sframe_frame_row_entry *frep);

/* Add SFrame FRE data given in the buffer FRES_BUF of size FRES_BUF_SIZE (for
   function at index FUNC_IDX) to the encoder context ECTX.  The number of FREs
   in the buffer are NUM_FRES.  */
int
sframe_encoder_add_fres_buf (sframe_encoder_ctx *ectx,
			     unsigned int func_idx,
			     uint32_t num_fres,
			     const char *fres_buf,
			     size_t fres_buf_size);

/* Add a new SFrame function descriptor entry with START_ADDR and FUNC_SIZE to
   the encoder context ECTX.  */
extern int
sframe_encoder_add_funcdesc (sframe_encoder_ctx *ectx,
			     int64_t start_addr,
			     uint32_t func_size);

/* Add a new SFrame function descriptor entry with START_ADDR, FUNC_SIZE,
   FUNC_INFO and REP_BLOCK_SIZE to the encoder context ECTX.  This API is valid
   only for SFrame format version 2.  */
extern int
sframe_encoder_add_funcdesc_v2 (sframe_encoder_ctx *ectx,
				int32_t start_addr,
				uint32_t func_size,
				unsigned char func_info,
				uint8_t rep_block_size,
				uint32_t num_fres);

/* Add a new SFrame function descriptor entry with START_PC_OFFSET, FUNC_SIZE,
   FUNC_INFO, FUNC_INFO2 and REP_BLOCK_SIZE to the encoder context ECTX.
   Return error code on failure.  */
extern int
sframe_encoder_add_funcdesc_v3 (sframe_encoder_ctx *ectx,
				int64_t start_pc_offset,
				uint32_t func_size,
				unsigned char func_info,
				unsigned char func_info2,
				uint8_t rep_block_size,
				uint32_t num_fres);

/* Serialize the contents of the encoder context ECTX and return the buffer.
   Sort the SFrame FDEs on start PC if SORT_FDE_P is true.  ENCODED_SIZE is
   updated to the size of the buffer.  Sets ERRP if failure.  */
extern char  *
sframe_encoder_write (sframe_encoder_ctx *ectx, size_t *encoded_size,
		      bool sort_fde_p, int *errp);

#ifdef	__cplusplus
}
#endif

#endif				/* _SFRAME_API_H */
