/* sframe.c - SFrame decoder/encoder.

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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "sframe-impl.h"
#include "swap.h"

/* Representation of SFrame FDE internal to libsframe.  */
typedef struct sframe_func_desc_entry_int
{
  int64_t func_start_pc_offset;
  uint32_t func_size;
  uint32_t func_start_fre_off;
  uint32_t func_num_fres;
  uint8_t func_info;
  uint8_t func_info2;
  uint8_t func_rep_size;
} sframe_func_desc_entry_int;

struct sf_fde_tbl
{
  unsigned int count;
  unsigned int alloced;
  sframe_func_desc_entry_int entry[1];
};

struct sf_fre_tbl
{
  unsigned int count;
  unsigned int alloced;
  sframe_frame_row_entry entry[1];
};

#define _sf_printflike_(string_index,first_to_check) \
    __attribute__ ((__format__ (__printf__, (string_index), (first_to_check))))

static void debug_printf (const char *, ...);

static int _sframe_debug;	/* Control for printing out debug info.  */

#define SFRAME_FRE_ALLOC_LEN  64
static int number_of_entries = 64;

static void
sframe_init_debug (void)
{
  static int inited;

  if (!inited)
    {
      _sframe_debug = getenv ("SFRAME_DEBUG") != NULL;
      inited = 1;
    }
}

_sf_printflike_ (1, 2)
static void debug_printf (const char *format, ...)
{
  if (_sframe_debug)
    {
      va_list args;

      va_start (args, format);
      vfprintf (stderr, format, args);
      va_end (args);
    }
}

/* Generate bitmask of given size in bytes.  This is used for
   some checks on the FRE start address.
   SFRAME_FRE_TYPE_ADDR1 => 1 byte => [ bitmask = 0xff ]
   SFRAME_FRE_TYPE_ADDR2 => 2 byte => [ bitmask = 0xffff ]
   SFRAME_FRE_TYPE_ADDR4 => 4 byte => [ bitmask = 0xffffffff ].  */
#define SFRAME_BITMASK_OF_SIZE(size_in_bytes) \
  (((uint64_t)1 << (size_in_bytes*8)) - 1)

/* Store the specified error code into errp if it is non-NULL.
   Return SFRAME_ERR.  */

static int
sframe_set_errno (int *errp, int error)
{
  if (errp != NULL)
    *errp = error;
  return SFRAME_ERR;
}

/* Store the specified error code into errp if it is non-NULL.
   Return NULL.  */

static void *
sframe_ret_set_errno (int *errp, int error)
{
  if (errp != NULL)
    *errp = error;
  return NULL;
}

/* Allocate space for NUM_FDES number of SFrame FDEs of type
   sframe_func_desc_entry_int.  This is version-unaware because this pertains
   to libsframe's internal in-memory representation of SFrame FDE.  */

static int
sframe_fde_tbl_alloc (sf_fde_tbl **fde_tbl, unsigned int num_fdes)
{
  size_t fidx_size = num_fdes * sizeof (sframe_func_desc_entry_int);
  size_t fd_tbl_sz = (sizeof (sf_fde_tbl) + fidx_size);

  *fde_tbl = malloc (fd_tbl_sz);
  if (*fde_tbl == NULL)
    return SFRAME_ERR;

  (*fde_tbl)->alloced = num_fdes;

  return 0;
}

/* Initialize libsframe's internal representation of SFrame FDEs.  */

static int
sframe_fde_tbl_init (sf_fde_tbl *fde_tbl, const char *fde_buf,
		     const char *fre_buf, size_t *fidx_size,
		     unsigned int num_fdes, uint8_t ver)
{
  if (ver == SFRAME_VERSION_3 && SFRAME_VERSION == SFRAME_VERSION_3)
    {
      *fidx_size = num_fdes * sizeof (sframe_func_desc_idx_v3);
      for (unsigned int i = 0; i < num_fdes; i++)
	{
	  const sframe_func_desc_idx_v3 *fdep
	    = (sframe_func_desc_idx_v3 *)fde_buf + i;
	  fde_tbl->entry[i].func_start_pc_offset = fdep->sfdi_func_start_offset;
	  fde_tbl->entry[i].func_size = fdep->sfdi_func_size;
	  fde_tbl->entry[i].func_start_fre_off = fdep->sfdi_func_start_fre_off;
	  /* V3 organizes the following data closer to the SFrame FREs for the
	     function.  Access them via the sfde_func_start_fre_off.  */
	  const sframe_func_desc_attr_v3 *fattr
	    = (sframe_func_desc_attr_v3 *)(fre_buf
					   + fdep->sfdi_func_start_fre_off);
	  fde_tbl->entry[i].func_num_fres = fattr->sfda_func_num_fres;
	  fde_tbl->entry[i].func_info = fattr->sfda_func_info;
	  fde_tbl->entry[i].func_info2 = fattr->sfda_func_info2;
	  fde_tbl->entry[i].func_rep_size = fattr->sfda_func_rep_size;
	}
      fde_tbl->count = num_fdes;
    }
  /* If ver is not the latest, read buffer manually and upgrade from
     sframe_func_desc_entry_v2 to populate the sf_fde_tbl entries.  */
  else if (ver == SFRAME_VERSION_2 && SFRAME_VERSION == SFRAME_VERSION_3)
    {
      *fidx_size = num_fdes * sizeof (sframe_func_desc_entry_v2);
      for (unsigned int i = 0; i < num_fdes; i++)
	{
	  const sframe_func_desc_entry_v2 *fdep
	    = (sframe_func_desc_entry_v2 *)fde_buf + i;
	  fde_tbl->entry[i].func_start_pc_offset
	    = fdep->sfde_func_start_address;
	  fde_tbl->entry[i].func_size = fdep->sfde_func_size;
	  fde_tbl->entry[i].func_start_fre_off = fdep->sfde_func_start_fre_off;
	  fde_tbl->entry[i].func_num_fres = fdep->sfde_func_num_fres;
	  fde_tbl->entry[i].func_info = fdep->sfde_func_info;
	  fde_tbl->entry[i].func_info2 = 0;
	  fde_tbl->entry[i].func_rep_size = fdep->sfde_func_rep_size;
	}
      fde_tbl->count = num_fdes;
    }
  else
    {
      /* Not possible ATM.  */
      *fidx_size = 0;
      return SFRAME_ERR;
    }

  return 0;
}

/* Get the SFrame header size.  */

static uint32_t
sframe_get_hdr_size (const sframe_header *sfh)
{
  return SFRAME_V1_HDR_SIZE (*sfh);
}

/* Access functions for frame row entry data.  */

static uint8_t
sframe_fre_get_dataword_count (uint8_t fre_info)
{
  return SFRAME_V1_FRE_OFFSET_COUNT (fre_info);
}

static uint8_t
sframe_fre_get_dataword_size (uint8_t fre_info)
{
  return SFRAME_V1_FRE_OFFSET_SIZE (fre_info);
}

static bool
sframe_get_fre_ra_mangled_p (uint8_t fre_info)
{
  return SFRAME_V1_FRE_MANGLED_RA_P (fre_info);
}

static bool
sframe_get_fre_ra_undefined_p (uint8_t fre_info)
{
  return SFRAME_V2_FRE_RA_UNDEFINED_P (fre_info);
}

/* Access functions for info from function descriptor entry.  */

static uint32_t
sframe_get_fre_type (sframe_func_desc_entry_int *fdep)
{
  uint32_t fre_type = 0;
  if (fdep)
    fre_type = SFRAME_V2_FUNC_FRE_TYPE (fdep->func_info);
  return fre_type;
}

static uint32_t
sframe_get_fde_pc_type (sframe_func_desc_entry_int *fdep)
{
  uint32_t fde_pc_type = 0;
  if (fdep)
    fde_pc_type = SFRAME_V2_FUNC_PC_TYPE (fdep->func_info);
  return fde_pc_type;
}

/* Check if flipping is needed, based on ENDIAN.  */

static int
need_swapping (int endian)
{
  unsigned int ui = 1;
  char *c = (char *)&ui;
  int is_little = (int)*c;

  switch (endian)
    {
      case SFRAME_ABI_AARCH64_ENDIAN_LITTLE:
      case SFRAME_ABI_AMD64_ENDIAN_LITTLE:
	return !is_little;
      case SFRAME_ABI_AARCH64_ENDIAN_BIG:
      case SFRAME_ABI_S390X_ENDIAN_BIG:
	return is_little;
      default:
	break;
    }

  return 0;
}

/* Flip the endianness of the SFrame header starting at BUF.
   VER is the version of the SFrame data in the buffer.

   Returns SFRAME_ERR if any error.  If error code is returned, the flipped
   header should not be used.  */

static int
flip_header (char *buf, uint8_t ver ATTRIBUTE_UNUSED)
{
  /* SFrame header binary format has remained the same in SFRAME_VERSION_1,
     SFRAME_VERSION_2.  */
  sframe_header *sfh = (sframe_header *) buf;
  swap_thing (sfh->sfh_preamble.sfp_magic);
  swap_thing (sfh->sfh_preamble.sfp_version);
  swap_thing (sfh->sfh_preamble.sfp_flags);
  swap_thing (sfh->sfh_abi_arch);
  swap_thing (sfh->sfh_cfa_fixed_fp_offset);
  swap_thing (sfh->sfh_cfa_fixed_ra_offset);
  swap_thing (sfh->sfh_auxhdr_len);
  swap_thing (sfh->sfh_num_fdes);
  swap_thing (sfh->sfh_num_fres);
  swap_thing (sfh->sfh_fre_len);
  swap_thing (sfh->sfh_fdeoff);
  swap_thing (sfh->sfh_freoff);

  /* Alert for missing functionatlity.  Auxiliary header, if present, needs to
     flipped based on per abi/arch semantics.  */
  if (sfh->sfh_auxhdr_len)
    return SFRAME_ERR;

  return 0;
}

/* Endian flip the SFrame FDE at BUF (buffer size provided in BUF_SIZE), given
   the SFrame version VER.  Update the FDE_SIZE to the size of the SFrame FDE
   flipped.

   Return SFRAME_ERR if any error.  If error code is returned, the flipped FDEP
   should not be used.  */

static int
flip_fde_desc (char *buf, size_t buf_size, uint8_t ver)
{
  if (ver == SFRAME_VERSION_3)
    {
      if (buf_size < sizeof (sframe_func_desc_idx_v3))
	return SFRAME_ERR;

      sframe_func_desc_idx_v3 *fdep = (sframe_func_desc_idx_v3 *) buf;
      swap_thing (fdep->sfdi_func_start_offset);
      swap_thing (fdep->sfdi_func_size);
      swap_thing (fdep->sfdi_func_start_fre_off);
    }
  else if (ver == SFRAME_VERSION_2)
    {
      if (buf_size < sizeof (sframe_func_desc_entry_v2))
	return SFRAME_ERR;

      sframe_func_desc_entry_v2 *fdep = (sframe_func_desc_entry_v2 *) buf;
      swap_thing (fdep->sfde_func_start_address);
      swap_thing (fdep->sfde_func_size);
      swap_thing (fdep->sfde_func_start_fre_off);
      swap_thing (fdep->sfde_func_num_fres);
    }
  else
    return SFRAME_ERR;

  return 0;
}

static int
flip_fde_attr_v3 (char *buf, size_t buf_size)
{
  if (buf_size < sizeof (sframe_func_desc_attr_v3))
    return SFRAME_ERR;

  /* sfda_func_num_fres is the first member of sframe_func_desc_attr_v3.  */
  struct { uint16_t x; } ATTRIBUTE_PACKED *p = (void*)buf;
  swap_thing (p->x);

  return 0;
}
/* Check if SFrame header has valid data.  */

static bool
sframe_header_sanity_check_p (const sframe_header *hp)
{
  /* Check preamble is valid.  */
  if (hp->sfh_preamble.sfp_magic != SFRAME_MAGIC
      || (hp->sfh_preamble.sfp_version != SFRAME_VERSION_1
	  && hp->sfh_preamble.sfp_version != SFRAME_VERSION_2
	  && hp->sfh_preamble.sfp_version != SFRAME_VERSION_3))
    return false;

  /* Check flags (version-aware).
     Do not validate V3 headers against V2 flag definitions, as V3 may
     introduce new flags.  */
  uint8_t valid_flags = SFRAME_V2_F_ALL_FLAGS;
  if (hp->sfh_preamble.sfp_version == SFRAME_VERSION_3)
    /* Replace with SFRAME_V3_F_ALL_FLAGS.  */
    valid_flags = SFRAME_V3_F_ALL_FLAGS;
  if (hp->sfh_preamble.sfp_flags & ~valid_flags)
    return false;

  /* Check offsets are valid.  */
  if (hp->sfh_fdeoff > hp->sfh_freoff)
    return false;

  return true;
}

/* Flip the start address pointed to by FP.  */

static void
flip_fre_start_address (void *addr, uint32_t fre_type)
{
  if (fre_type == SFRAME_FRE_TYPE_ADDR2)
    {
      struct { uint16_t x; } ATTRIBUTE_PACKED *p = addr;
      swap_thing (p->x);
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR4)
    {
      struct { uint32_t x; } ATTRIBUTE_PACKED *p = addr;
      swap_thing (p->x);
    }
}

static void
flip_fre_datawords (void *datawords, uint8_t dataword_size,
		    uint8_t dataword_cnt)
{
  int j;

  if (dataword_size == SFRAME_FRE_DATAWORD_2B)
    {
      struct { uint16_t x; } ATTRIBUTE_PACKED *p = datawords;
      for (j = dataword_cnt; j > 0; p++, j--)
	swap_thing (p->x);
    }
  else if (dataword_size == SFRAME_FRE_DATAWORD_4B)
    {
      struct { uint32_t x; } ATTRIBUTE_PACKED *p = datawords;
      for (j = dataword_cnt; j > 0; p++, j--)
	swap_thing (p->x);
    }
}

/* Get the FRE start address size, given the FRE_TYPE.  */

static size_t
sframe_fre_start_addr_size (uint32_t fre_type)
{
  size_t addr_size = 0;
  switch (fre_type)
    {
    case SFRAME_FRE_TYPE_ADDR1:
      addr_size = 1;
      break;
    case SFRAME_FRE_TYPE_ADDR2:
      addr_size = 2;
      break;
    case SFRAME_FRE_TYPE_ADDR4:
      addr_size = 4;
      break;
    default:
      /* No other value is expected.  */
      sframe_assert (0);
      break;
    }
  return addr_size;
}

/* Check if the FREP has valid data.  */

static bool
sframe_fre_sanity_check_p (const sframe_frame_row_entry *frep)
{
  uint8_t dataword_size, dataword_cnt;
  uint8_t fre_info;

  if (frep == NULL)
    return false;

  fre_info = frep->fre_info;
  dataword_size = sframe_fre_get_dataword_size (fre_info);

  if (dataword_size != SFRAME_FRE_DATAWORD_1B
      && dataword_size != SFRAME_FRE_DATAWORD_2B
      && dataword_size != SFRAME_FRE_DATAWORD_4B)
    return false;

  dataword_cnt = sframe_fre_get_dataword_count (fre_info);
  if (dataword_cnt > MAX_NUM_DATAWORDS)
    return false;

  return true;
}

/* Get FRE_INFO's data words' size in bytes.  */

static size_t
sframe_fre_datawords_bytes_size (uint8_t fre_info)
{
  uint8_t dataword_size, dataword_cnt;

  dataword_size = sframe_fre_get_dataword_size (fre_info);

  debug_printf ("dataword_size =  %u\n", dataword_size);

  dataword_cnt = sframe_fre_get_dataword_count (fre_info);

  if (dataword_size == SFRAME_FRE_DATAWORD_2B
      || dataword_size == SFRAME_FRE_DATAWORD_4B)	/* 2 or 4 bytes.  */
    return (dataword_cnt * (dataword_size * 2));

  return dataword_cnt;
}

/* Get total size in bytes to represent FREP in the binary format.  This
   includes the starting address, FRE info, and all the offsets.  */

static size_t
sframe_fre_entry_size (sframe_frame_row_entry *frep, uint32_t fre_type)
{
  if (frep == NULL)
    return 0;

  uint8_t fre_info = frep->fre_info;
  size_t addr_size = sframe_fre_start_addr_size (fre_type);

  return (addr_size + sizeof (frep->fre_info)
	  + sframe_fre_datawords_bytes_size (fre_info));
}

/* Get total size in bytes in the SFrame FRE at FRE_BUF location, given the
   type of FRE as FRE_TYPE.  */

static size_t
sframe_buf_fre_entry_size (const char *fre_buf, uint32_t fre_type)
{
  if (fre_buf == NULL)
    return 0;

  size_t addr_size = sframe_fre_start_addr_size (fre_type);
  uint8_t fre_info = *(uint8_t *)(fre_buf + addr_size);

  return (addr_size + sizeof (fre_info)
	  + sframe_fre_datawords_bytes_size (fre_info));
}
/* Get the function descriptor entry at index FUNC_IDX in the decoder
   context CTX.  */

static sframe_func_desc_entry_int *
sframe_decoder_get_funcdesc_at_index (const sframe_decoder_ctx *ctx,
				      uint32_t func_idx)
{
  sframe_func_desc_entry_int *fdep;
  uint32_t num_fdes;
  int err;

  num_fdes = sframe_decoder_get_num_fidx (ctx);
  if (num_fdes == 0
      || func_idx >= num_fdes
      || ctx->sfd_funcdesc == NULL
      || ctx->sfd_funcdesc->count <= func_idx)
    return sframe_ret_set_errno (&err, SFRAME_ERR_DCTX_INVAL);

  fdep = &ctx->sfd_funcdesc->entry[func_idx];
  return fdep;
}

/* Get the offset of the start PC of the SFrame FDE at FUNC_IDX from the start
   of the SFrame section.  This section-relative offset is used within
   libsframe for sorting the SFrame FDEs, and also information lookup routines
   like sframe_find_fre.

   If FUNC_IDX is not a valid index in the given decoder object, returns 0.  */

static int64_t
sframe_decoder_get_secrel_func_start_addr (const sframe_decoder_ctx *dctx,
					   uint32_t func_idx)
{
  int err = 0;
  int32_t offsetof_fde_in_sec
    = sframe_decoder_get_offsetof_fde_start_addr (dctx, func_idx, &err);
  /* If func_idx is not a valid index, return 0.  */
  if (err)
    return 0;

  const sframe_func_desc_entry_int *fdep = &dctx->sfd_funcdesc->entry[func_idx];
  int64_t func_start_pc_offset = fdep->func_start_pc_offset;

  return func_start_pc_offset + offsetof_fde_in_sec;
}

/* Check whether for the given FDEP, the SFrame Frame Row Entry identified via
   the START_IP_OFFSET and the END_IP_OFFSET, provides the stack trace
   information for the PC.  */

static bool
sframe_fre_check_range_p (const sframe_decoder_ctx *dctx, uint32_t func_idx,
			  uint32_t start_ip_offset, uint32_t end_ip_offset,
			  int64_t pc)
{
  sframe_func_desc_entry_int *fdep;
  int64_t func_start_pc_offset;
  uint8_t rep_block_size;
  uint32_t pc_type;
  uint32_t pc_offset;
  bool mask_p;

  fdep = &dctx->sfd_funcdesc->entry[func_idx];
  func_start_pc_offset = sframe_decoder_get_secrel_func_start_addr (dctx,
								    func_idx);
  pc_type = sframe_get_fde_pc_type (fdep);
  mask_p = (pc_type == SFRAME_V3_FDE_PCTYPE_MASK);
  rep_block_size = fdep->func_rep_size;

  if (func_start_pc_offset > pc)
    return false;

  /* Given func_start_addr <= pc, pc - func_start_addr must be positive.  */
  pc_offset = pc - func_start_pc_offset;
  /* For SFrame FDEs encoding information for repetitive pattern of insns,
     masking with the rep_block_size is necessary to find the matching FRE.  */
  if (mask_p)
    pc_offset = pc_offset % rep_block_size;

  return (start_ip_offset <= pc_offset) && (end_ip_offset >= pc_offset);
}

/* Read the on-disk SFrame FDE from location BUF of size in bytes equal to
   BUF_SIZE.

   Return SFRAME_ERR if any error.  If error code is returned, the read values
   should not be used.  */

static int
sframe_decode_fde_desc_v2 (const char *buf, size_t buf_size,
			   uint32_t *num_fres, uint32_t *fre_type,
			   uint32_t *fre_offset)
{
  if (buf_size < sizeof (sframe_func_desc_entry_v2))
    return SFRAME_ERR;

  sframe_func_desc_entry_v2 *fdep = (sframe_func_desc_entry_v2 *) buf;
  *num_fres = fdep->sfde_func_num_fres;
  *fre_type = SFRAME_V2_FUNC_FRE_TYPE (fdep->sfde_func_info);
  *fre_offset = fdep->sfde_func_start_fre_off;

  return 0;
}

/* Read the on-disk SFrame FDE from location BUF of size in bytes equal to
   BUF_SIZE.

   Return SFRAME_ERR if any error.  If error code is returned, the read values
   should not be used.  */

static int
sframe_decode_fde_idx_v3 (const char *buf, size_t buf_size,
			  uint32_t *fre_offset)
{
  if (buf_size < sizeof (sframe_func_desc_idx_v3))
    return SFRAME_ERR;

  sframe_func_desc_idx_v3 *fdep = (sframe_func_desc_idx_v3 *) buf;
  *fre_offset = fdep->sfdi_func_start_fre_off;

  return 0;
}

static int
sframe_decode_fde_attr_v3 (const char *buf, size_t buf_size,
			   uint16_t *num_fres, uint32_t *fre_type)
{
  if (buf_size < sizeof (sframe_func_desc_attr_v3))
    return SFRAME_ERR;

  const sframe_func_desc_attr_v3 *fdap = (sframe_func_desc_attr_v3 *) buf;
  *num_fres = fdap->sfda_func_num_fres;
  *fre_type = SFRAME_V3_FDE_FRE_TYPE (fdap->sfda_func_info);
  return 0;
}

static int
flip_fre (char *fp, size_t fp_size, uint32_t fre_type, size_t *fre_size)
{
  uint8_t fre_info;
  uint8_t dataword_size, dataword_cnt;
  size_t addr_size, fre_info_size, datawords_bytes_size;
  int err = 0;

  if (fre_size == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  addr_size = sframe_fre_start_addr_size (fre_type);
  if (addr_size > fp_size)
    return SFRAME_ERR;
  flip_fre_start_address (fp, fre_type);

  /* Advance the buffer pointer to where the FRE info is.  */
  fp += addr_size;
  fp_size -= addr_size;

  /* FRE info is uint8_t.  No need to flip.  */
  fre_info_size = sizeof (uint8_t);
  if (fre_info_size > fp_size)
    return SFRAME_ERR;
  fre_info = *(uint8_t*)fp;
  dataword_size = sframe_fre_get_dataword_size (fre_info);
  dataword_cnt = sframe_fre_get_dataword_count (fre_info);

  /* Advance the buffer pointer to where the stack offsets are.  */
  fp += fre_info_size;
  fp_size -= fre_info_size;
  datawords_bytes_size = sframe_fre_datawords_bytes_size (fre_info);
  if (datawords_bytes_size > fp_size)
    return SFRAME_ERR;
  flip_fre_datawords (fp, dataword_size, dataword_cnt);

  *fre_size = addr_size + fre_info_size + datawords_bytes_size;

  return 0;
}

/* Endian flip the contents of FRAME_BUF of size BUF_SIZE.
   The SFrame header in the FRAME_BUF must be endian flipped prior to
   calling flip_sframe.

   Endian flipping at decode time vs encode time have different needs.  At
   encode time, the frame_buf is in host endianness, and hence, values should
   be read up before the buffer is changed to foreign endianness.  This change
   of behaviour is specified via TO_FOREIGN arg.

   If an error code is returned, the buffer should not be used.  */

static int
flip_sframe_fdes_with_fres_v2 (char *frame_buf, size_t buf_size,
			       uint32_t to_foreign)
{
  char *fp = NULL;
  uint32_t num_fres = 0;
  uint32_t fre_type = 0;
  uint32_t fre_offset = 0;
  size_t esz = 0;
  int err = 0;
  /* For error checking.  */
  size_t fde_bytes_flipped = 0;
  size_t fre_bytes_flipped = 0;

  /* Header must be in host endianness at this time.  */
  const sframe_header *ihp = (sframe_header *)frame_buf;

  if (!sframe_header_sanity_check_p (ihp))
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);

  /* The contents of the SFrame header are safe to read.  Get the number of
     FDEs and the first FDE in the buffer.  */
  size_t hdrsz = sframe_get_hdr_size (ihp);
  uint32_t num_fdes = ihp->sfh_num_fdes;
  uint8_t ver = ihp->sfh_preamble.sfp_version;
  char *fdes = frame_buf + hdrsz + ihp->sfh_fdeoff;
  char *fres = frame_buf + hdrsz + ihp->sfh_freoff;
  const char *buf_end = frame_buf + buf_size;

  unsigned int i = 0, j = 0;
  unsigned int prev_frep_index = 0;
  size_t fsz = sizeof (sframe_func_desc_entry_v2);
  for (i = 0; i < num_fdes; fdes += fsz, i++)
    {
      if (fdes >= buf_end)
	goto bad;

      /* Handle FDE.  */
      if (to_foreign && sframe_decode_fde_desc_v2 (fdes, buf_end - fdes,
						   &num_fres, &fre_type,
						   &fre_offset))
	goto bad;

      if (flip_fde_desc (fdes, buf_end - fdes, ver))
	goto bad;

      if (!to_foreign && sframe_decode_fde_desc_v2 (fdes, buf_end - fdes,
						    &num_fres, &fre_type,
						    &fre_offset))
	goto bad;

      fde_bytes_flipped += fsz;

      /* Handle FREs.  */
      fp = fres + fre_offset;
      for (; j < prev_frep_index + num_fres; j++)
	{
	  if (flip_fre (fp, buf_end - fp, fre_type, &esz))
	    goto bad;
	  fre_bytes_flipped += esz;
	  fp += esz;
	}
      prev_frep_index = j;
    }

  /* All FDEs must have been endian flipped by now.  */
  if (i != num_fdes || fde_bytes_flipped > ihp->sfh_freoff - ihp->sfh_fdeoff)
    goto bad;

  /* All FREs must have been endian flipped by now.  */
  if (j != ihp->sfh_num_fres || fre_bytes_flipped > ihp->sfh_fre_len)
    goto bad;

  /* Optional trailing section padding.  */
  size_t frame_size = hdrsz + ihp->sfh_freoff + fre_bytes_flipped;
  for (fp = frame_buf + frame_size; fp < frame_buf + buf_size; fp++)
    if (*fp != '\0')
      goto bad;

  /* Done.  */
  return 0;
bad:
  return SFRAME_ERR;
}

/* Endian flip the contents of FRAME_BUF of size BUF_SIZE.
   The SFrame header in the FRAME_BUF must be endian flipped prior to
   calling flip_sframe_fdes_with_fres_v3.

   Endian flipping at decode time vs encode time have different needs.  At
   encode time, the frame_buf is in host endianness, and hence, values should
   be read up before the buffer is changed to foreign endianness.  This change
   of behaviour is specified via TO_FOREIGN arg.

   If an error code is returned, the buffer should not be used.  */

static int
flip_sframe_fdes_with_fres_v3 (char *frame_buf, size_t buf_size,
			       uint32_t to_foreign)
{
  char *fp = NULL;
  uint16_t num_fres = 0;
  uint32_t fre_type = 0;
  uint32_t fre_offset = 0;
  size_t esz = 0;
  int err = 0;
  /* For error checking.  */
  size_t fde_bytes_flipped = 0;
  size_t fre_bytes_flipped = 0;

  /* Header must be in host endianness at this time.  */
  const sframe_header *ihp = (sframe_header *)frame_buf;

  if (!sframe_header_sanity_check_p (ihp))
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);

  /* The contents of the SFrame header are safe to read.  Get the number of
     FDEs and the first FDE in the buffer.  */
  size_t hdrsz = sframe_get_hdr_size (ihp);
  uint32_t num_fdes = ihp->sfh_num_fdes;
  uint8_t ver = ihp->sfh_preamble.sfp_version;
  char *fdes = frame_buf + hdrsz + ihp->sfh_fdeoff;
  char *fres = frame_buf + hdrsz + ihp->sfh_freoff;
  const char *buf_end = frame_buf + buf_size;

  unsigned int i = 0, j = 0;
  unsigned int prev_frep_index = 0;
  size_t fsz = sizeof (sframe_func_desc_idx_v3);
  for (i = 0; i < num_fdes; fdes += fsz, i++)
    {
      if (fdes >= buf_end)
	goto bad;

      /* Handle FDE.  */
      if (to_foreign && sframe_decode_fde_idx_v3 (fdes, buf_end - fdes,
						  &fre_offset))
	goto bad;

      if (flip_fde_desc (fdes, buf_end - fdes, ver))
	goto bad;

      if (!to_foreign && sframe_decode_fde_idx_v3 (fdes, buf_end - fdes,
						   &fre_offset))
	goto bad;

      fde_bytes_flipped += fsz;

      /* Handle FDE attr (only in V3).  */
      fp = fres + fre_offset;
      if (to_foreign && sframe_decode_fde_attr_v3 (fp, buf_end - fp,
						   &num_fres, &fre_type))
	goto bad;

      if (flip_fde_attr_v3 (fp, buf_end - fp))
	goto bad;

      fre_bytes_flipped += sizeof (sframe_func_desc_attr_v3);

      if (!to_foreign && sframe_decode_fde_attr_v3 (fp, buf_end - fp,
						    &num_fres, &fre_type))
	goto bad;

      /* Handle FREs.  */
      fp += sizeof (sframe_func_desc_attr_v3);
      for (; j < prev_frep_index + num_fres; j++)
	{
	  if (flip_fre (fp, buf_end - fp, fre_type, &esz))
	    goto bad;
	  fre_bytes_flipped += esz;
	  fp += esz;
	}
      prev_frep_index = j;
    }

  /* All FDEs must have been endian flipped by now.  */
  if (i != num_fdes || fde_bytes_flipped > ihp->sfh_freoff - ihp->sfh_fdeoff)
    goto bad;

  /* All FREs must have been endian flipped by now.  */
  if (j != ihp->sfh_num_fres || fre_bytes_flipped > ihp->sfh_fre_len)
    goto bad;

  /* Optional trailing section padding.  */
  size_t frame_size = hdrsz + ihp->sfh_freoff + fre_bytes_flipped;
  for (fp = frame_buf + frame_size; fp < frame_buf + buf_size; fp++)
    if (*fp != '\0')
      goto bad;

  /* Done.  */
  return 0;
bad:
  return SFRAME_ERR;
}

static int
flip_sframe (char *frame_buf, size_t buf_size, uint32_t to_foreign)
{
  int err = 0;

  /* Header must be in host endianness at this time.  */
  const sframe_header *ihp = (sframe_header *)frame_buf;
  if (!sframe_header_sanity_check_p (ihp))
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);
  uint8_t ver = ihp->sfh_preamble.sfp_version;

  if (ver == SFRAME_VERSION_3)
    err = flip_sframe_fdes_with_fres_v3 (frame_buf, buf_size, to_foreign);
  else if (ver == SFRAME_VERSION_2)
    err = flip_sframe_fdes_with_fres_v2 (frame_buf, buf_size, to_foreign);
  else
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);

  if (err)
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);

  /* Success.  */
  return 0;
}

/* Expands the memory allocated for the SFrame Frame Row Entry (FRE) table
   FRE_TBL.  This function is called when the current FRE buffer is
   insufficient and more stack trace data in the form of COUNT number of SFrame
   FREs need to be added to the SFrame section.

   Updates ERRP with 0 on success, or an SFrame error code on failure (e.g.,
   memory allocation error).  */

static sf_fre_tbl *
sframe_grow_fre_tbl (sf_fre_tbl *fre_tbl, uint32_t count, int *errp)
{
  size_t fre_tbl_sz = 0;
  /* Ensure buffer for at least COUNT number of elements.  */
  uint32_t grow_count = SFRAME_FRE_ALLOC_LEN + count;
  sf_fre_tbl *new_tbl = NULL;

  if (fre_tbl == NULL)
    {
      fre_tbl_sz = (sizeof (sf_fre_tbl)
		    + (grow_count * sizeof (sframe_frame_row_entry)));
      new_tbl = malloc (fre_tbl_sz);
      if (new_tbl == NULL)
	{
	  sframe_set_errno (errp, SFRAME_ERR_NOMEM);
	  goto bad;
	}

      memset (new_tbl, 0, fre_tbl_sz);
      new_tbl->alloced = grow_count;
    }
  else if (fre_tbl->count + count >= fre_tbl->alloced)
    {
      uint32_t new_len = fre_tbl->alloced + grow_count;
      fre_tbl_sz = (sizeof (sf_fre_tbl)
		    + (new_len * sizeof (sframe_frame_row_entry)));
      void *tmp = realloc (fre_tbl, fre_tbl_sz);
      if (tmp == NULL)
	{
	  sframe_set_errno (errp, SFRAME_ERR_NOMEM);
	  goto bad;
	}
      new_tbl = tmp;

      memset (&new_tbl->entry[new_tbl->alloced], 0,
	      grow_count * sizeof (sframe_frame_row_entry));
      new_tbl->alloced += grow_count;
    }

bad:
  return new_tbl;
}

/* The SFrame Decoder.  */

/* Get SFrame header from the given decoder context DCTX.  */

static const sframe_header *
sframe_decoder_get_header (const sframe_decoder_ctx *dctx)
{
  const sframe_header *hp = NULL;
  if (dctx != NULL)
    hp = &dctx->sfd_header;
  return hp;
}

/* Compare function for qsort'ing the FDE table.  */

static int
fde_func (const void *p1, const void *p2)
{
  const sframe_func_desc_entry_int *aa = p1;
  const sframe_func_desc_entry_int *bb = p2;

  if (aa->func_start_pc_offset < bb->func_start_pc_offset)
    return -1;
  else if (aa->func_start_pc_offset > bb->func_start_pc_offset)
    return 1;
  return 0;
}

/* Get IDX'th offset from FRE.  Set errp as applicable.  */

static int32_t
sframe_get_fre_offset (const sframe_frame_row_entry *fre, int idx, int *errp)
{
  uint8_t dataword_cnt, dataword_size;

  if (fre == NULL || !sframe_fre_sanity_check_p (fre))
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  dataword_cnt = sframe_fre_get_dataword_count (fre->fre_info);
  dataword_size = sframe_fre_get_dataword_size (fre->fre_info);

  if (dataword_cnt < idx + 1)
    return sframe_set_errno (errp, SFRAME_ERR_FREOFFSET_NOPRESENT);

  if (errp)
    *errp = 0; /* Offset Valid.  */

  if (dataword_size == SFRAME_FRE_DATAWORD_1B)
    {
      int8_t *offsets = (int8_t *)fre->fre_datawords;
      return offsets[idx];
    }
  else if (dataword_size == SFRAME_FRE_DATAWORD_2B)
    {
      int16_t *offsets = (int16_t *)fre->fre_datawords;
      return offsets[idx];
    }
  else
    {
      int32_t *offsets = (int32_t *)fre->fre_datawords;
      return offsets[idx];
    }
}

/* Get IDX'th data word as unsigned data from FRE.  Set errp as applicable.  */

uint32_t
sframe_get_fre_udata (const sframe_frame_row_entry *fre, int idx, int *errp)
{
  uint8_t dataword_cnt, dataword_size;

  if (fre == NULL || !sframe_fre_sanity_check_p (fre))
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  dataword_cnt = sframe_fre_get_dataword_count (fre->fre_info);
  dataword_size = sframe_fre_get_dataword_size (fre->fre_info);

  if (dataword_cnt < idx + 1)
    return sframe_set_errno (errp, SFRAME_ERR_FREOFFSET_NOPRESENT);

  if (errp)
    *errp = 0; /* Offset Valid.  */

  if (dataword_size == SFRAME_FRE_DATAWORD_1B)
    {
      uint8_t *datawords = (uint8_t *)fre->fre_datawords;
      return datawords[idx];
    }
  else if (dataword_size == SFRAME_FRE_DATAWORD_2B)
    {
      uint16_t *datawords = (uint16_t *)fre->fre_datawords;
      return datawords[idx];
    }
  else
    {
      uint32_t *datawords = (uint32_t *)fre->fre_datawords;
      return datawords[idx];
    }
}

/* Free the decoder context.  */

void
sframe_decoder_free (sframe_decoder_ctx **dctxp)
{
  if (dctxp != NULL)
    {
      sframe_decoder_ctx *dctx = *dctxp;
      if (dctx == NULL)
	return;

      if (dctx->sfd_funcdesc != NULL)
	{
	  free (dctx->sfd_funcdesc);
	  dctx->sfd_funcdesc = NULL;
	}
      if (dctx->sfd_fres != NULL)
	{
	  free (dctx->sfd_fres);
	  dctx->sfd_fres = NULL;
	}
      if (dctx->sfd_buf != NULL)
	{
	  free (dctx->sfd_buf);
	  dctx->sfd_buf = NULL;
	}

      free (*dctxp);
      *dctxp = NULL;
    }
}

/* Create an FDE function info byte given an FRE_TYPE and an FDE_TYPE.  */
/* FIXME API for linker.  Revisit if its better placed somewhere else?  */

unsigned char
sframe_fde_create_func_info (uint32_t fre_type,
			     uint32_t fde_type)
{
  unsigned char func_info;
  sframe_assert (fre_type == SFRAME_FRE_TYPE_ADDR1
		   || fre_type == SFRAME_FRE_TYPE_ADDR2
		   || fre_type == SFRAME_FRE_TYPE_ADDR4);
  sframe_assert (fde_type == SFRAME_V3_FDE_PCTYPE_INC
		 || fde_type == SFRAME_V3_FDE_PCTYPE_MASK);
  func_info = SFRAME_V2_FUNC_INFO (fde_type, fre_type);
  return func_info;
}

/* Get the FRE type given the function size.  */
/* FIXME API for linker.  Revisit if its better placed somewhere else?  */

uint32_t
sframe_calc_fre_type (size_t func_size)
{
  uint32_t fre_type = 0;
  if (func_size < SFRAME_FRE_TYPE_ADDR1_LIMIT)
    fre_type = SFRAME_FRE_TYPE_ADDR1;
  else if (func_size < SFRAME_FRE_TYPE_ADDR2_LIMIT)
    fre_type = SFRAME_FRE_TYPE_ADDR2;
  /* Adjust the check a bit so that it remains warning-free but meaningful
     on 32-bit systems.  */
  else if (func_size <= (size_t) (SFRAME_FRE_TYPE_ADDR4_LIMIT - 1))
    fre_type = SFRAME_FRE_TYPE_ADDR4;
  return fre_type;
}

/* Get the base reg id from the FRE info.  Set errp if failure.  */

uint8_t
sframe_fre_get_base_reg_id (const sframe_frame_row_entry *fre, int *errp)
{
  if (fre == NULL)
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  uint8_t fre_info = fre->fre_info;
  return SFRAME_V1_FRE_CFA_BASE_REG_ID (fre_info);
}

/* Get the CFA offset from the FRE.  If the offset is invalid, sets errp.  */

int32_t
sframe_fre_get_cfa_offset (const sframe_decoder_ctx *dctx,
			   const sframe_frame_row_entry *fre,
			   uint32_t fde_type,
			   int *errp)
{
  int err;
  bool flex_p = (fde_type == SFRAME_FDE_TYPE_FLEX);
  uint32_t idx = flex_p ? 1 : 0;
  int32_t offset = sframe_get_fre_offset (fre, idx, &err);

  /* For s390x undo adjustment of CFA offset (to enable 8-bit offsets).  */
  if (!err
      && sframe_decoder_get_abi_arch (dctx) == SFRAME_ABI_S390X_ENDIAN_BIG)
    offset = SFRAME_V2_S390X_CFA_OFFSET_DECODE (offset);

  if (errp)
    *errp = err;
  return offset;
}

/* Get the FP offset from the FRE.  If the offset is invalid, sets errp.  */

int32_t
sframe_fre_get_fp_offset (const sframe_decoder_ctx *dctx,
			  const sframe_frame_row_entry *fre,
			  uint32_t fde_type,
			  int *errp)
{
  int fp_err = 0;
  int8_t fixed_fp_offset = sframe_decoder_get_fixed_fp_offset (dctx);
  bool flex_p = (fde_type == SFRAME_FDE_TYPE_FLEX);

  /* Initialize fp_offset_idx for default FDEs.  In some ABIs, the stack offset
     to recover RA (using the CFA) from is fixed (like AMD64).  In such cases,
     the stack offset to recover FP will appear at the second index.  */
  uint32_t fp_offset_idx = ((sframe_decoder_get_fixed_ra_offset (dctx)
			     != SFRAME_CFA_FIXED_RA_INVALID)
			    ? SFRAME_FRE_RA_OFFSET_IDX
			    : SFRAME_FRE_FP_OFFSET_IDX);
  if (flex_p)
    {
      uint32_t flex_ra_reg_data
	= sframe_get_fre_udata (fre, SFRAME_FRE_RA_OFFSET_IDX * 2, errp);
      /* In presence of RA padding SFRAME_FRE_RA_OFFSET_INVALID (instead of RA
	 offsets), adjust the expected index of the FP offset.  */
      if (errp && *errp == 0
	  && flex_ra_reg_data == SFRAME_FRE_RA_OFFSET_INVALID)
	fp_offset_idx = SFRAME_FRE_FP_OFFSET_IDX * 2;
      else
	fp_offset_idx = SFRAME_FRE_FP_OFFSET_IDX * 2 + 1;
    }

  /* NB: This errp must be retained if returning fp_offset.  */
  int32_t fp_offset = sframe_get_fre_offset (fre, fp_offset_idx, &fp_err);

  /* If the FP offset is not being tracked, return the fixed FP offset from the
     SFrame header:
       - For default FDEs (!flex_p)
       - For flex FDEs, if there were no FP offsets found.  */
  if ((!flex_p || (flex_p && fp_err))
      && fixed_fp_offset != SFRAME_CFA_FIXED_FP_INVALID
      && !sframe_get_fre_ra_undefined_p (fre->fre_info))
    {
      if (errp)
	*errp = 0;
      return fixed_fp_offset;
    }

  if (errp)
    *errp = fp_err;
  return fp_offset;
}

/* Get the RA offset from the FRE.  If the offset is invalid, sets errp.  */

int32_t
sframe_fre_get_ra_offset (const sframe_decoder_ctx *dctx,
			  const sframe_frame_row_entry *fre,
			  uint32_t fde_type,
			  int *errp)
{
  int ra_err = 0;
  int8_t fixed_ra_offset = sframe_decoder_get_fixed_ra_offset (dctx);
  bool flex_p = (fde_type == SFRAME_FDE_TYPE_FLEX);

  uint32_t ra_offset_idx = (flex_p
			    ? SFRAME_FRE_RA_OFFSET_IDX * 2 + 1
			    : SFRAME_FRE_RA_OFFSET_IDX);
  /* NB: This errp must be retained if returning ra_offset.  */
  int32_t ra_offset = sframe_get_fre_offset (fre, ra_offset_idx, &ra_err);

  /* For ABIs where RA offset is not being tracked, return the fixed RA offset
     specified in the the SFrame header, when:
       - for default FDEs (!flex_p)
       - for flex FDEs, if RA offset is solely padding or not present.  */
  if ((!flex_p || (flex_p && ra_err))
      && fixed_ra_offset != SFRAME_CFA_FIXED_RA_INVALID
      && !sframe_get_fre_ra_undefined_p (fre->fre_info))
    {
      if (errp)
	*errp = 0;
      return fixed_ra_offset;
    }

  /* Otherwise, return the RA offset from the FRE.  The corresponding errp was
     set earlier.  */
  if (errp)
    *errp = ra_err;
  return ra_offset;
}

/* Get whether the RA is mangled.  */

bool
sframe_fre_get_ra_mangled_p (const sframe_decoder_ctx *dctx ATTRIBUTE_UNUSED,
			     const sframe_frame_row_entry *fre, int *errp)
{
  if (fre == NULL || !sframe_fre_sanity_check_p (fre))
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  return sframe_get_fre_ra_mangled_p (fre->fre_info);
}

/* Get whether the RA is undefined (i.e. outermost frame).  */

bool
sframe_fre_get_ra_undefined_p (const sframe_decoder_ctx *dctx ATTRIBUTE_UNUSED,
			       const sframe_frame_row_entry *fre, int *errp)
{
  if (fre == NULL || !sframe_fre_sanity_check_p (fre))
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  return sframe_get_fre_ra_undefined_p (fre->fre_info);
}

static int
sframe_frame_row_entry_copy (sframe_frame_row_entry *dst,
			     sframe_frame_row_entry *src)
{
  int err = 0;

  if (dst == NULL || src == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  memcpy (dst, src, sizeof (sframe_frame_row_entry));
  return 0;
}

/* Decode the SFrame FRE start address offset value from FRE_BUF in on-disk
   binary format, given the FRE_TYPE.  Updates the FRE_START_ADDR.

   Returns 0 on success, SFRAME_ERR otherwise.  */

static int
sframe_decode_fre_start_address (const void *fre_buf,
				 uint32_t *fre_start_addr,
				 uint32_t fre_type)
{
  uint32_t saddr = 0;
  int err = 0;

  if (fre_type == SFRAME_FRE_TYPE_ADDR1)
    {
      const uint8_t *uc = fre_buf;
      saddr = *uc;
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR2)
    {
      /* SFrame is an unaligned on-disk format.  See PR libsframe/29856.  */
      const struct { uint16_t x; } ATTRIBUTE_PACKED *p = fre_buf;
      saddr = p->x;
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR4)
    {
      const struct { uint32_t x; } ATTRIBUTE_PACKED *p = fre_buf;
      saddr = p->x;
    }
  else
    sframe_set_errno (&err, SFRAME_ERR_INVAL);

  *fre_start_addr = saddr;
  return err;
}

/* Decode a frame row entry FRE which starts at location FRE_BUF.  The function
   updates ESZ to the size of the FRE as stored in the binary format.

   This function works closely with the SFrame binary format.

   Returns SFRAME_ERR if failure.  */

static int
sframe_decode_fre (const char *fre_buf, sframe_frame_row_entry *fre,
		   uint32_t fre_type, size_t *esz)
{
  int err = 0;
  const char *datawords = NULL;
  size_t datawords_sz;
  size_t addr_size;
  size_t fre_size;

  if (fre_buf == NULL || fre == NULL || esz == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  /* Copy over the FRE start address.  */
  sframe_decode_fre_start_address (fre_buf, &fre->fre_start_addr, fre_type);

  addr_size = sframe_fre_start_addr_size (fre_type);
  fre->fre_info = *(uint8_t *)(fre_buf + addr_size);
  /* Sanity check as the API works closely with the binary format.  */
  sframe_assert (sizeof (fre->fre_info) == sizeof (uint8_t));

  /* Cleanup the space for fre_datawords first, then copy over the valid
     bytes.  */
  memset (fre->fre_datawords, 0, MAX_DATAWORD_BYTES);
  /* Get offsets size.  */
  datawords_sz = sframe_fre_datawords_bytes_size (fre->fre_info);
  datawords = fre_buf + addr_size + sizeof (fre->fre_info);
  memcpy (fre->fre_datawords, datawords, datawords_sz);

  /* The FRE has been decoded.  Use it to perform one last sanity check.  */
  fre_size = sframe_fre_entry_size (fre, fre_type);
  sframe_assert (fre_size == (addr_size + sizeof (fre->fre_info)
			      + datawords_sz));
  *esz = fre_size;

  return 0;
}

/* Decode the specified SFrame buffer SF_BUF of size SF_SIZE and return the
   new SFrame decoder context.

   Sets ERRP for the caller if any error.  Frees up the allocated memory in
   case of error.  */

sframe_decoder_ctx *
sframe_decode (const char *sf_buf, size_t sf_size, int *errp)
{
  const sframe_preamble *sfp;
  size_t hdrsz;
  const sframe_header *dhp;
  sframe_decoder_ctx *dctx;
  char *frame_buf;
  char *tempbuf = NULL;

  size_t fidx_size;
  uint32_t fre_bytes;
  int foreign_endian = 0;

  sframe_init_debug ();

  if ((sf_buf == NULL) || (!sf_size))
    return sframe_ret_set_errno (errp, SFRAME_ERR_INVAL);
  else if (sf_size < sizeof (sframe_header))
    return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);

  sfp = (const sframe_preamble *) sf_buf;

  debug_printf ("sframe_decode: magic=0x%x version=%u flags=%u\n",
		sfp->sfp_magic, sfp->sfp_version, sfp->sfp_flags);

  /* Check for foreign endianness.  */
  if (sfp->sfp_magic != SFRAME_MAGIC)
    {
      if (sfp->sfp_magic == bswap_16 (SFRAME_MAGIC))
	foreign_endian = 1;
      else
	return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
    }

  /* Initialize a new decoder context.  */
  if ((dctx = malloc (sizeof (sframe_decoder_ctx))) == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
  memset (dctx, 0, sizeof (sframe_decoder_ctx));

  if (foreign_endian)
    {
      /* Allocate a new buffer and initialize it.  */
      tempbuf = (char *) malloc (sf_size * sizeof (char));
      if (tempbuf == NULL)
	return sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
      memcpy (tempbuf, sf_buf, sf_size);

      /* Flip the header first.  */
      if (flip_header (tempbuf, sfp->sfp_version))
	{
	  sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
	  goto decode_fail_free;
	}
      /* Flip the rest of the SFrame section data buffer.  */
      if (flip_sframe (tempbuf, sf_size, 0))
	{
	  sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
	  goto decode_fail_free;
	}

      frame_buf = tempbuf;
      /* This buffer is malloc'd when endian flipping the contents of the input
	 buffer are needed.  Keep a reference to it so it can be free'd up
	 later in sframe_decoder_free ().  */
      dctx->sfd_buf = tempbuf;
    }
  else
    frame_buf = (char *)sf_buf;

  /* Handle the SFrame header.  */
  dctx->sfd_header = *(sframe_header *) frame_buf;
  /* Validate the contents of SFrame header.  */
  dhp = &dctx->sfd_header;
  if (!sframe_header_sanity_check_p (dhp))
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
      goto decode_fail_free;
    }
  hdrsz = sframe_get_hdr_size (dhp);
  frame_buf += hdrsz;

  /* Handle the SFrame Function Descriptor Entry section.  */
  if (sframe_fde_tbl_alloc (&dctx->sfd_funcdesc, dhp->sfh_num_fdes))
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
      goto decode_fail_free;
    }

  /* SFrame FDEs are at an offset of sfh_fdeoff from SFrame header end.  */
  if (sframe_fde_tbl_init (dctx->sfd_funcdesc, frame_buf + dhp->sfh_fdeoff,
			   frame_buf + dhp->sfh_freoff,
			   &fidx_size, dhp->sfh_num_fdes, sfp->sfp_version))
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
      goto decode_fail_free;
    }

  debug_printf ("%zu total fidx size\n", fidx_size);

  /* Handle the SFrame Frame Row Entry section.  */
  dctx->sfd_fres = (char *) malloc (dhp->sfh_fre_len);
  if (dctx->sfd_fres == NULL)
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
      goto decode_fail_free;
    }
  /* SFrame FREs are at an offset of sfh_freoff from SFrame header end.  */
  memcpy (dctx->sfd_fres, frame_buf + dhp->sfh_freoff, dhp->sfh_fre_len);

  fre_bytes = dhp->sfh_fre_len;
  dctx->sfd_fre_nbytes = fre_bytes;

  debug_printf ("%u total fre bytes\n", fre_bytes);

  return dctx;

decode_fail_free:
  if (foreign_endian && tempbuf != NULL)
    free (tempbuf);
  sframe_decoder_free (&dctx);
  dctx = NULL;
  return dctx;
}

/* Get the size of the SFrame header from the decoder context CTX.  */

unsigned int
sframe_decoder_get_hdr_size (const sframe_decoder_ctx *ctx)
{
  const sframe_header *dhp = sframe_decoder_get_header (ctx);
  return sframe_get_hdr_size (dhp);
}

/* Get the SFrame's abi/arch info given the decoder context DCTX.  */

uint8_t
sframe_decoder_get_abi_arch (const sframe_decoder_ctx *dctx)
{
  const sframe_header *dhp = sframe_decoder_get_header (dctx);
  return dhp->sfh_abi_arch;
}

/* Get the format version from the SFrame decoder context DCTX.  */

uint8_t
sframe_decoder_get_version (const sframe_decoder_ctx *dctx)
{
  const sframe_header *dhp = sframe_decoder_get_header (dctx);
  return dhp->sfh_preamble.sfp_version;
}

/* Get the section flags from the SFrame decoder context DCTX.  */

uint8_t
sframe_decoder_get_flags (const sframe_decoder_ctx *dctx)
{
  const sframe_header *dhp = sframe_decoder_get_header (dctx);
  return dhp->sfh_preamble.sfp_flags;
}

/* Get the SFrame's fixed FP offset given the decoder context CTX.  */
int8_t
sframe_decoder_get_fixed_fp_offset (const sframe_decoder_ctx *ctx)
{
  const sframe_header *dhp = sframe_decoder_get_header (ctx);
  return dhp->sfh_cfa_fixed_fp_offset;
}

/* Get the SFrame's fixed RA offset given the decoder context CTX.  */
int8_t
sframe_decoder_get_fixed_ra_offset (const sframe_decoder_ctx *ctx)
{
  const sframe_header *dhp = sframe_decoder_get_header (ctx);
  return dhp->sfh_cfa_fixed_ra_offset;
}

/* Get the offset of the sfde_func_start_address field (from the start of the
   on-disk layout of the SFrame section) of the FDE at FUNC_IDX in the decoder
   context DCTX.

   If FUNC_IDX is more than the number of SFrame FDEs in the section, sets
   error code in ERRP, but returns the (hypothetical) offset.  This is useful
   for the linker when arranging input FDEs into the output section to be
   emitted.  */

uint32_t
sframe_decoder_get_offsetof_fde_start_addr (const sframe_decoder_ctx *dctx,
					    uint32_t func_idx, int *errp)
{
  if (func_idx >= sframe_decoder_get_num_fidx (dctx))
    sframe_ret_set_errno (errp, SFRAME_ERR_FDE_NOTFOUND);
  else if (errp)
    *errp = 0;

  if (sframe_decoder_get_version (dctx) == SFRAME_VERSION_3)
    return (sframe_decoder_get_hdr_size (dctx)
	    + func_idx * sizeof (sframe_func_desc_idx_v3)
	    + offsetof (sframe_func_desc_idx_v3, sfdi_func_start_offset));
  else if (sframe_decoder_get_version (dctx) == SFRAME_VERSION_2)
    return (sframe_decoder_get_hdr_size (dctx)
	    + func_idx * sizeof (sframe_func_desc_entry_v2)
	    + offsetof (sframe_func_desc_entry_v2, sfde_func_start_address));
  else
    sframe_ret_set_errno (errp, SFRAME_ERR_INVAL);

  return 0;
}

/* Find the function descriptor entry starting which contains the specified
   address ADDR.  */

static sframe_func_desc_entry_int *
sframe_get_funcdesc_with_addr_internal (const sframe_decoder_ctx *ctx,
					int64_t addr, int *errp,
					uint32_t *func_idx)
{
  sframe_func_desc_entry_int *fdp;
  int low, high;

  if (ctx == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_INVAL);

  const sframe_header *dhp = sframe_decoder_get_header (ctx);

  if (dhp == NULL || dhp->sfh_num_fdes == 0 || ctx->sfd_funcdesc == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_DCTX_INVAL);
  /* If the FDE sub-section is not sorted on PCs, skip the lookup because
     binary search cannot be used.  */
  if ((sframe_decoder_get_flags (ctx) & SFRAME_F_FDE_SORTED) == 0)
    return sframe_ret_set_errno (errp, SFRAME_ERR_FDE_NOTSORTED);

  /* Do the binary search.  */
  fdp = (sframe_func_desc_entry_int *) ctx->sfd_funcdesc->entry;
  low = 0;
  high = dhp->sfh_num_fdes - 1;
  while (low <= high)
    {
      int mid = low + (high - low) / 2;

      /* Given func_start_addr <= addr,
	 addr - func_start_addr must be positive.  */
      if (sframe_decoder_get_secrel_func_start_addr (ctx, mid) <= addr
	  && ((uint32_t)(addr - sframe_decoder_get_secrel_func_start_addr (ctx,
									   mid))
	      < fdp[mid].func_size))
	{
	  *func_idx = mid;
	  return fdp + mid;
	}

      if (sframe_decoder_get_secrel_func_start_addr (ctx, mid) < addr)
	low = mid + 1;
      else
	high = mid - 1;
    }

  return sframe_ret_set_errno (errp, SFRAME_ERR_FDE_NOTFOUND);
}

/* Get the end IP offset for the FRE at index i in the FDEP.  The buffer FRES
   is the starting location for the FRE.  */

static uint32_t
sframe_fre_get_end_ip_offset (sframe_func_desc_entry_int *fdep, unsigned int i,
			      const char *fres)
{
  uint32_t end_ip_offset;
  uint32_t fre_type;

  fre_type = sframe_get_fre_type (fdep);

  /* Get the start address of the next FRE in sequence.  */
  if (i < fdep->func_num_fres - 1)
    {
      sframe_decode_fre_start_address (fres, &end_ip_offset, fre_type);
      end_ip_offset -= 1;
    }
  else
    /* The end IP offset for the FRE needs to be deduced from the function
       size.  */
    end_ip_offset = fdep->func_size - 1;

  return end_ip_offset;
}

/* Find the SFrame Row Entry which contains the PC.  Returns
   SFRAME_ERR if failure.  */

int
sframe_find_fre (const sframe_decoder_ctx *ctx, int64_t pc,
		 sframe_frame_row_entry *frep)
{
  sframe_frame_row_entry cur_fre;
  sframe_func_desc_entry_int *fdep;
  uint32_t func_idx;
  uint32_t fre_type, i;
  int64_t func_start_pc_offset;
  uint32_t start_ip_offset, end_ip_offset;
  const char *fres;
  size_t size = 0;
  int err = 0;

  if ((ctx == NULL) || (frep == NULL))
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  uint8_t ver = sframe_decoder_get_version (ctx);
  /* Find the FDE which contains the PC, then scan its fre entries.  */
  fdep = sframe_get_funcdesc_with_addr_internal (ctx, pc, &err, &func_idx);
  if (fdep == NULL || ctx->sfd_fres == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_DCTX_INVAL);

  fre_type = sframe_get_fre_type (fdep);

  fres = ctx->sfd_fres + fdep->func_start_fre_off;
  if (ver == SFRAME_VERSION_3)
    fres += sizeof (sframe_func_desc_attr_v3);
  func_start_pc_offset = sframe_decoder_get_secrel_func_start_addr (ctx,
								    func_idx);

  for (i = 0; i < fdep->func_num_fres; i++)
   {
     err = sframe_decode_fre (fres, &cur_fre, fre_type, &size);
     if (err)
       return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

     start_ip_offset = cur_fre.fre_start_addr;
     end_ip_offset = sframe_fre_get_end_ip_offset (fdep, i, fres + size);

     /* Stop search if FRE's start_ip is greater than pc.  Given
	func_start_addr <= pc, pc - func_start_addr must be positive.  */
     if (start_ip_offset > (uint32_t)(pc - func_start_pc_offset))
       return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

     if (sframe_fre_check_range_p (ctx, func_idx, start_ip_offset,
				   end_ip_offset, pc))
       {
	 sframe_frame_row_entry_copy (frep, &cur_fre);
	 return 0;
       }
     fres += size;
   }
  return sframe_set_errno (&err, SFRAME_ERR_FDE_INVAL);
}

/* Return the number of function descriptor entries in the SFrame decoder
   DCTX.  */

uint32_t
sframe_decoder_get_num_fidx (const sframe_decoder_ctx *ctx)
{
  uint32_t num_fdes = 0;
  const sframe_header *dhp = sframe_decoder_get_header (ctx);
  if (dhp)
    num_fdes = dhp->sfh_num_fdes;
  return num_fdes;
}

int
sframe_decoder_get_funcdesc_v2 (const sframe_decoder_ctx *dctx,
				unsigned int i,
				uint32_t *num_fres,
				uint32_t *func_size,
				int32_t *func_start_address,
				unsigned char *func_info,
				uint8_t *rep_block_size)
{
  sframe_func_desc_entry_int *fdp;
  int err = 0;

  if (dctx == NULL || func_start_address == NULL
      || num_fres == NULL || func_size == NULL
      || sframe_decoder_get_version (dctx) == SFRAME_VERSION_1)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  fdp = sframe_decoder_get_funcdesc_at_index (dctx, i);

  if (fdp == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  *num_fres = fdp->func_num_fres;
  *func_start_address = (int32_t) fdp->func_start_pc_offset;
  *func_size = fdp->func_size;
  *func_info = fdp->func_info;
  *rep_block_size = fdp->func_rep_size;

  return 0;
}

/* Get the data (NUM_FRES, FUNC_SIZE, START_PC_OFFSET, FUNC_INFO,
   REP_BLOCK_SIZE) from the SFrame function descriptor entry at the I'th index
   in the decoder object DCTX.  Return SFRAME_ERR on failure.  */

int
sframe_decoder_get_funcdesc_v3 (const sframe_decoder_ctx *dctx,
				unsigned int i,
				uint32_t *num_fres,
				uint32_t *func_size,
				int64_t *start_pc_offset,
				unsigned char *func_info,
				unsigned char *func_info2,
				uint8_t *rep_block_size)
{
  int err = 0;
  if (dctx == NULL || sframe_decoder_get_version (dctx) != SFRAME_VERSION_3)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  sframe_func_desc_entry_int *fdp
    = sframe_decoder_get_funcdesc_at_index (dctx, i);
  if (fdp == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  if (num_fres)
    *num_fres = fdp->func_num_fres;
  if (start_pc_offset)
    *start_pc_offset = fdp->func_start_pc_offset;
  if (func_size)
    *func_size = fdp->func_size;
  if (func_info)
    *func_info = fdp->func_info;
  if (func_info2)
    *func_info2 = fdp->func_info2;
  if (rep_block_size)
    *rep_block_size = fdp->func_rep_size;

  return 0;
}

/* Get the FRE_IDX'th FRE of the function at FUNC_IDX'th function
   descriptor entry in the SFrame decoder CTX.  Returns error code as
   applicable.  */

int
sframe_decoder_get_fre (const sframe_decoder_ctx *ctx,
			unsigned int func_idx,
			unsigned int fre_idx,
			sframe_frame_row_entry *fre)
{
  sframe_func_desc_entry_int *fdep;
  sframe_frame_row_entry ifre;
  const char *fres;
  uint32_t i;
  uint32_t fre_type;
  size_t esz = 0;
  int err = 0;

  if (ctx == NULL || fre == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  uint8_t ver = sframe_decoder_get_version (ctx);

  /* Get function descriptor entry at index func_idx.  */
  fdep = sframe_decoder_get_funcdesc_at_index (ctx, func_idx);
  if (fdep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  fre_type = sframe_get_fre_type (fdep);
  /* Now scan the FRE entries.  */
  fres = ctx->sfd_fres + fdep->func_start_fre_off;
  if (ver == SFRAME_VERSION_3)
    fres += sizeof (sframe_func_desc_attr_v3);

  for (i = 0; i < fdep->func_num_fres; i++)
   {
     /* Decode the FRE at the current position.  Return it if valid.  */
     err = sframe_decode_fre (fres, &ifre, fre_type, &esz);
     if (i == fre_idx)
       {
	 if (!sframe_fre_sanity_check_p (&ifre))
	   return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

	  /* Although a stricter sanity check on fre_start_addr like:
	       if (fdep->func_size)
		 sframe_assert (frep->fre_start_addr < fdep->func_size);
	     is more suitable, some code has been seen to not abide by it.  See
	     PR libsframe/33131.  */
	  sframe_assert (ifre.fre_start_addr <= fdep->func_size);

	 sframe_frame_row_entry_copy (fre, &ifre);

	 return 0;
       }
     /* Next FRE.  */
     fres += esz;
   }

  return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);
}

/* Get the SFrame FRE data of the function at FUNC_IDX'th function index entry
   in the SFrame decoder DCTX.  The reference to the buffer is returned in
   FRES_BUF with FRES_BUF_SIZE indicating the size of the buffer.  The number
   of FREs in the buffer are NUM_FRES.  In SFrame V3, this buffer also contains
   the FDE attr data before the actual SFrame FREs.  Returns SFRAME_ERR in case
   of error.  */

int
sframe_decoder_get_fres_buf (const sframe_decoder_ctx *dctx,
			     const uint32_t func_idx,
			     char **fres_buf,
			     size_t *fres_buf_size,
			     uint32_t *num_fres)
{
  sframe_func_desc_entry_int *fdep;
  uint32_t i = 0;
  uint32_t fre_type;
  size_t esz;
  int err = 0;

  if (dctx == NULL || fres_buf == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  /* Get function descriptor entry at index func_idx.  */
  fdep = sframe_decoder_get_funcdesc_at_index (dctx, func_idx);
  *num_fres = fdep->func_num_fres;

  if (fdep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  fre_type = sframe_get_fre_type (fdep);
  /* Update the pointer to (and total size of) the FRE entries.  */
  *fres_buf = dctx->sfd_fres + fdep->func_start_fre_off;
  const char *tmp_buf = *fres_buf + sizeof (sframe_func_desc_attr_v3);
  *fres_buf_size = sizeof (sframe_func_desc_attr_v3);
  while (i < *num_fres)
    {
      /* Avoid cost of full decoding at this time.  */
      esz = sframe_buf_fre_entry_size (tmp_buf, fre_type);
      tmp_buf += esz;
      *fres_buf_size += esz;
      i++;
    }

  return 0;
}


/* SFrame Encoder.  */

/* Get a reference to the SFrame header, given the encoder context ECTX.  */

static sframe_header *
sframe_encoder_get_header (sframe_encoder_ctx *ectx)
{
  sframe_header *hp = NULL;
  if (ectx)
    hp = &ectx->sfe_header;
  return hp;
}

static sframe_func_desc_entry_int *
sframe_encoder_get_funcdesc_at_index (sframe_encoder_ctx *ectx,
				      uint32_t func_idx)
{
  sframe_func_desc_entry_int *fde = NULL;
  if (func_idx < sframe_encoder_get_num_fidx (ectx))
    {
      sf_fde_tbl *func_tbl = ectx->sfe_funcdesc;
      fde = func_tbl->entry + func_idx;
    }
  return fde;
}

/* Create an encoder context with the given SFrame format version VER, FLAGS
   and ABI information.  Uses the ABI specific FIXED_FP_OFFSET and
   FIXED_RA_OFFSET values as provided.  Sets errp if failure.  */

sframe_encoder_ctx *
sframe_encode (uint8_t ver, uint8_t flags, uint8_t abi_arch,
	       int8_t fixed_fp_offset, int8_t fixed_ra_offset, int *errp)
{
  sframe_header *hp;
  sframe_encoder_ctx *ectx;

  if (ver != SFRAME_VERSION)
    return sframe_ret_set_errno (errp, SFRAME_ERR_VERSION_INVAL);

  if ((ectx = malloc (sizeof (sframe_encoder_ctx))) == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);

  memset (ectx, 0, sizeof (sframe_encoder_ctx));

  /* Get the SFrame header and update it.  */
  hp = sframe_encoder_get_header (ectx);
  hp->sfh_preamble.sfp_version = ver;
  hp->sfh_preamble.sfp_magic = SFRAME_MAGIC;
  hp->sfh_preamble.sfp_flags = flags;

  /* Implementation in the SFrame encoder APIs, e.g.,
     sframe_encoder_write_sframe assume flag SFRAME_F_FDE_FUNC_START_PCREL
     set.  */
  if (!(flags & SFRAME_F_FDE_FUNC_START_PCREL))
   return sframe_ret_set_errno (errp, SFRAME_ERR_ECTX_INVAL);

  hp->sfh_abi_arch = abi_arch;
  hp->sfh_cfa_fixed_fp_offset = fixed_fp_offset;
  hp->sfh_cfa_fixed_ra_offset = fixed_ra_offset;

  return ectx;
}

/* Free the encoder context ECTXP.  */

void
sframe_encoder_free (sframe_encoder_ctx **ectxp)
{
  if (ectxp != NULL)
    {
      sframe_encoder_ctx *ectx = *ectxp;
      if (ectx == NULL)
	return;

      if (ectx->sfe_funcdesc != NULL)
	{
	  free (ectx->sfe_funcdesc);
	  ectx->sfe_funcdesc = NULL;
	}
      if (ectx->sfe_fres != NULL)
	{
	  free (ectx->sfe_fres);
	  ectx->sfe_fres = NULL;
	}
      if (ectx->sfe_data != NULL)
	{
	  free (ectx->sfe_data);
	  ectx->sfe_data = NULL;
	}

      free (*ectxp);
      *ectxp = NULL;
    }
}

/* Get the size of the SFrame header from the encoder context ECTX.  */

unsigned int
sframe_encoder_get_hdr_size (sframe_encoder_ctx *ectx)
{
  const sframe_header *ehp = sframe_encoder_get_header (ectx);
  return sframe_get_hdr_size (ehp);
}

/* Get the SFrame abi/arch info from the encoder context ECTX.  */

uint8_t
sframe_encoder_get_abi_arch (sframe_encoder_ctx *ectx)
{
  uint8_t abi_arch = 0;
  const sframe_header *ehp = sframe_encoder_get_header (ectx);
  if (ehp)
    abi_arch = ehp->sfh_abi_arch;
  return abi_arch;
}

/* Get the SFrame format version from the encoder context ECTX.  */

uint8_t
sframe_encoder_get_version (sframe_encoder_ctx *ectx)
{
  const sframe_header *ehp = sframe_encoder_get_header (ectx);
  return ehp->sfh_preamble.sfp_version;
}

/* Get the SFrame flags from the encoder context ECTX.  */

uint8_t
sframe_encoder_get_flags (sframe_encoder_ctx *ectx)
{
  const sframe_header *ehp = sframe_encoder_get_header (ectx);
  return ehp->sfh_preamble.sfp_flags;
}

/* Return the number of SFrame function descriptor entries in the encoder
   context ECTX.  */

uint32_t
sframe_encoder_get_num_fidx (sframe_encoder_ctx *ectx)
{
  uint32_t num_fdes = 0;
  const sframe_header *ehp = sframe_encoder_get_header (ectx);
  if (ehp)
    num_fdes = ehp->sfh_num_fdes;
  return num_fdes;
}

/* Get the offset of the sfde_func_start_address field (from the start of the
   on-disk layout of the SFrame section) of the FDE at FUNC_IDX in the encoder
   context ECTX.

   If FUNC_IDX is more than the number of SFrame FDEs in the section, sets
   error code in ERRP, but returns the (hypothetical) offset.  This is useful
   for the linker when arranging input FDEs into the output section to be
   emitted.  */

uint32_t
sframe_encoder_get_offsetof_fde_start_addr (sframe_encoder_ctx *ectx,
					    uint32_t func_idx, int *errp)
{
  if (func_idx >= sframe_encoder_get_num_fidx (ectx))
    sframe_ret_set_errno (errp, SFRAME_ERR_FDE_INVAL);
  else if (errp)
    *errp = 0;

  return (sframe_encoder_get_hdr_size (ectx)
	  + func_idx * sizeof (sframe_func_desc_idx_v3)
	  + offsetof (sframe_func_desc_idx_v3, sfdi_func_start_offset));
}

/* Add an SFrame FRE to function at FUNC_IDX'th function descriptor entry in
   the encoder context ECTX.  */

int
sframe_encoder_add_fre (sframe_encoder_ctx *ectx,
			unsigned int func_idx,
			sframe_frame_row_entry *frep)
{
  sframe_header *ehp;
  sframe_func_desc_entry_int *fdep;
  sframe_frame_row_entry *ectx_frep;
  size_t datawords_sz, esz;
  uint32_t fre_type;
  int err = 0;

  if (ectx == NULL || frep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);
  if (!sframe_fre_sanity_check_p (frep))
    return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

  /* Use func_idx to gather the function descriptor entry.  */
  fdep = sframe_encoder_get_funcdesc_at_index (ectx, func_idx);

  if (fdep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  fre_type = sframe_get_fre_type (fdep);
  sf_fre_tbl *fre_tbl = ectx->sfe_fres;

  if (fre_tbl == NULL || fre_tbl->count == fre_tbl->alloced)
    {
      sf_fre_tbl *tmp = sframe_grow_fre_tbl (fre_tbl, 1, &err);
      if (err)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;
	}
      fre_tbl = tmp;
    }

  ectx_frep = &fre_tbl->entry[fre_tbl->count];
  ectx_frep->fre_start_addr
    = frep->fre_start_addr;
  ectx_frep->fre_info = frep->fre_info;

  /* Although a stricter sanity check on fre_start_addr like:
       if (fdep->func_size)
	 sframe_assert (frep->fre_start_addr < fdep->func_size);
     is more suitable, some code has been seen to not abide by it.  See PR
     libsframe/33131.  */
  sframe_assert (frep->fre_start_addr <= fdep->func_size);

  /* frep has already been sanity check'd.  Get offsets size.  */
  datawords_sz = sframe_fre_datawords_bytes_size (frep->fre_info);
  memcpy (&ectx_frep->fre_datawords, &frep->fre_datawords, datawords_sz);

  esz = sframe_fre_entry_size (frep, fre_type);
  fre_tbl->count++;

  ectx->sfe_fres = fre_tbl;
  ectx->sfe_fre_nbytes += esz;

  if (!fdep->func_num_fres)
    ectx->sfe_fre_nbytes += sizeof (sframe_func_desc_attr_v3);

  ehp = sframe_encoder_get_header (ectx);
  ehp->sfh_num_fres = fre_tbl->count;

  /* Update the value of the number of FREs for the function.  */
  fdep->func_num_fres++;

  return 0;

bad:
  if (fre_tbl != NULL)
    free (fre_tbl);
  ectx->sfe_fres = NULL;
  ectx->sfe_fre_nbytes = 0;
  return -1;
}

/* Add SFrame FRE data given in the buffer FRES_BUF of size FRES_BUF_SIZE (for
   function at index FUNC_IDX) to the encoder context ECTX.  The number of FREs
   in the buffer are NUM_FRES.  Returns SFRAME_ERR if failure.  */

int
sframe_encoder_add_fres_buf (sframe_encoder_ctx *ectx,
			     unsigned int func_idx,
			     uint32_t num_fres,
			     const char *fres_buf,
			     size_t fres_buf_size)
{
  sframe_frame_row_entry *ectx_frep;
  size_t esz = 0;

  int err = 0;
  if (ectx == NULL || ((fres_buf == NULL) != (fres_buf_size == 0)))
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  /* Use func_idx to gather the function descriptor entry.  */
  sframe_func_desc_entry_int *fdep
    = sframe_encoder_get_funcdesc_at_index (ectx, func_idx);
  if (fdep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  sf_fre_tbl *fre_tbl = ectx->sfe_fres;
  if (fre_tbl == NULL || fre_tbl->count + num_fres >= fre_tbl->alloced)
    {
      sf_fre_tbl *tmp = sframe_grow_fre_tbl (fre_tbl, num_fres, &err);
      if (err)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;		/* OOM.  */
	}
      fre_tbl = tmp;
    }

  /* Update the SFrame func attr values.  */
  fdep->func_num_fres = num_fres;
  const char *fres = fres_buf + sizeof (uint16_t);
  fdep->func_info = *(uint8_t *)fres;
  fres += sizeof (uint8_t);
  fdep->func_info2 = *(uint8_t *)fres;
  fres += sizeof (uint8_t);
  fdep->func_rep_size = *(uint8_t *)fres;
  fres += sizeof (uint8_t);

  uint32_t fre_type = sframe_get_fre_type (fdep);
  uint32_t remaining = num_fres;
  size_t buf_size = sizeof (sframe_func_desc_attr_v3);
  while (remaining)
    {
      ectx_frep = &fre_tbl->entry[fre_tbl->count];
      /* Copy the SFrame FRE data over to the encoder object's fre_tbl.  */
      sframe_decode_fre (fres, ectx_frep, fre_type, &esz);

      if (!sframe_fre_sanity_check_p (ectx_frep))
	return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

      /* Although a stricter sanity check on fre_start_addr like:
	   if (fdep->func_size)
	     sframe_assert (frep->fre_start_addr < fdep->func_size);
	 is more suitable, some code has been seen to not abide by it.  See PR
	 libsframe/33131.  */
      sframe_assert (ectx_frep->fre_start_addr <= fdep->func_size);

      fre_tbl->count++;
      fres += esz;
      buf_size += esz;
      remaining--;
    }

  sframe_assert (fres_buf_size == buf_size);
  ectx->sfe_fres = fre_tbl;
  ectx->sfe_fre_nbytes += buf_size;

  sframe_header *ehp = sframe_encoder_get_header (ectx);
  ehp->sfh_num_fres = fre_tbl->count;

  return 0;

bad:
  if (fre_tbl != NULL)
    free (fre_tbl);
  ectx->sfe_fres = NULL;
  ectx->sfe_fre_nbytes = 0;
  return -1;
}

/* Add a new SFrame function descriptor entry with START_ADDR, FUNC_SIZE and
   FUNC_INFO to the encoder context ECTX.  Caller must make sure that ECTX
   exists.  */

static int
sframe_encoder_add_funcdesc_internal (sframe_encoder_ctx *ectx,
				      int64_t start_addr,
				      uint32_t func_size)
{
  sframe_header *ehp;
  sf_fde_tbl *fd_info;
  size_t fd_tbl_sz;
  int err = 0;

  fd_info = ectx->sfe_funcdesc;
  ehp = sframe_encoder_get_header (ectx);

  if (fd_info == NULL)
    {
      fd_tbl_sz = (sizeof (sf_fde_tbl)
		   + (number_of_entries * sizeof (sframe_func_desc_entry_int)));
      fd_info = malloc (fd_tbl_sz);
      if (fd_info == NULL)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;		/* OOM.  */
	}
      memset (fd_info, 0, fd_tbl_sz);
      fd_info->alloced = number_of_entries;
    }
  else if (fd_info->count == fd_info->alloced)
    {
      fd_tbl_sz = (sizeof (sf_fde_tbl)
		   + ((fd_info->alloced + number_of_entries)
		      * sizeof (sframe_func_desc_entry_int)));
      sf_fde_tbl *tmp = realloc (fd_info, fd_tbl_sz);
      if (tmp == NULL)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;		/* OOM.  */
	}
      fd_info = tmp;

      memset (&fd_info->entry[fd_info->alloced], 0,
	      number_of_entries * sizeof (sframe_func_desc_entry_int));
      fd_info->alloced += number_of_entries;
    }

  fd_info->entry[fd_info->count].func_start_pc_offset = start_addr;
  /* Num FREs is updated as FREs are added for the function later via
     sframe_encoder_add_fre.  */
  fd_info->entry[fd_info->count].func_size = func_size;
  fd_info->entry[fd_info->count].func_start_fre_off = ectx->sfe_fre_nbytes;
#if 0
  // Linker optimization test code cleanup later ibhagat TODO FIXME
  uint32_t fre_type = sframe_calc_fre_type (func_size);

  fd_info->entry[fd_info->count].sfde_func_info
    = sframe_fde_func_info (fre_type);
#endif
  fd_info->count++;
  ectx->sfe_funcdesc = fd_info;
  ehp->sfh_num_fdes++;
  return 0;

bad:
  if (fd_info != NULL)
    free (fd_info);
  ectx->sfe_funcdesc = NULL;
  ehp->sfh_num_fdes = 0;
  return err;
}

/* Add a new SFrame function descriptor entry with START_PC_OFFSET and
   FUNC_SIZE to the encoder context ECTX.  */

int
sframe_encoder_add_funcdesc (sframe_encoder_ctx *ectx, int64_t start_pc_offset,
			     uint32_t func_size)
{
  int err = 0;
  if (ectx == NULL || sframe_encoder_get_version (ectx) == SFRAME_VERSION_1)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  err = sframe_encoder_add_funcdesc_internal (ectx, start_pc_offset, func_size);
  if (err)
    return err;

  return 0;
}

/* Add a new SFrame function descriptor entry with START_ADDR, FUNC_SIZE,
   FUNC_INFO and REP_BLOCK_SIZE to the encoder context ECTX.  This API is valid
   only for SFrame format version 2.  */

int
sframe_encoder_add_funcdesc_v2 (sframe_encoder_ctx *ectx,
				int32_t start_addr,
				uint32_t func_size,
				unsigned char func_info,
				uint8_t rep_block_size,
				uint32_t num_fres ATTRIBUTE_UNUSED)
{
  int err = 0;
  if (ectx == NULL || sframe_encoder_get_version (ectx) == SFRAME_VERSION_1)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  err = sframe_encoder_add_funcdesc_internal (ectx, start_addr, func_size);
  if (err)
    return err;

  sf_fde_tbl *fd_info = ectx->sfe_funcdesc;
  fd_info->entry[fd_info->count-1].func_info = func_info;
  fd_info->entry[fd_info->count-1].func_rep_size = rep_block_size;

  return 0;
}

/* Add a new SFrame function descriptor entry with START_PC_OFFSET, FUNC_SIZE,
   FUNC_INFO, FUNC_INFO2 and REP_BLOCK_SIZE to the encoder context ECTX.
   Return error code on failure.  */

int
sframe_encoder_add_funcdesc_v3 (sframe_encoder_ctx *ectx,
				int64_t start_pc_offset,
				uint32_t func_size,
				unsigned char func_info,
				unsigned char func_info2,
				uint8_t rep_block_size,
				uint32_t num_fres ATTRIBUTE_UNUSED)
{
  int err = 0;
  if (ectx == NULL || sframe_encoder_get_version (ectx) != SFRAME_VERSION_3)
    {
      sframe_set_errno (&err, SFRAME_ERR_INVAL);
      return err;
    }

  err = sframe_encoder_add_funcdesc_internal (ectx, start_pc_offset,
					      func_size);
  if (err)
    return err;

  sf_fde_tbl *fd_info = ectx->sfe_funcdesc;
  fd_info->entry[fd_info->count-1].func_info = func_info;
  fd_info->entry[fd_info->count-1].func_info2 = func_info2;
  fd_info->entry[fd_info->count-1].func_rep_size = rep_block_size;

  return 0;
}

static int
sframe_sort_funcdesc (sframe_encoder_ctx *ectx)
{
  sframe_header *ehp = sframe_encoder_get_header (ectx);

  /* Sort and write out the FDE table.  */
  sf_fde_tbl *fd_info = ectx->sfe_funcdesc;
  if (fd_info)
    {
      /* The new encoding of sfde_func_start_address means the distances are
	 not from the same anchor, so cannot be sorted directly.  At the moment
	 we adress this by manual value adjustments before and after sorting.
	 FIXME - qsort_r may be more optimal.  */

      for (unsigned int i = 0; i < fd_info->count; i++)
	fd_info->entry[i].func_start_pc_offset
	  += sframe_encoder_get_offsetof_fde_start_addr (ectx, i, NULL);

      qsort (fd_info->entry, fd_info->count,
	     sizeof (sframe_func_desc_entry_int), fde_func);

      for (unsigned int i = 0; i < fd_info->count; i++)
	fd_info->entry[i].func_start_pc_offset
	  -= sframe_encoder_get_offsetof_fde_start_addr (ectx, i, NULL);

      /* Update preamble's flags.  */
      ehp->sfh_preamble.sfp_flags |= SFRAME_F_FDE_SORTED;
    }
  return 0;
}

/* Write the SFrame FRE start address from the in-memory FRE_START_ADDR
   to the buffer CONTENTS (on-disk format), given the FRE_TYPE and
   FRE_START_ADDR_SZ.  */

static int
sframe_encoder_write_fre_start_addr (char *contents,
				     uint32_t fre_start_addr,
				     uint32_t fre_type,
				     size_t fre_start_addr_sz)
{
  int err = 0;

  if (fre_type == SFRAME_FRE_TYPE_ADDR1)
    {
      uint8_t uc = fre_start_addr;
      memcpy (contents, &uc, fre_start_addr_sz);
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR2)
    {
      uint16_t ust = fre_start_addr;
      memcpy (contents, &ust, fre_start_addr_sz);
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR4)
    {
      uint32_t uit = fre_start_addr;
      memcpy (contents, &uit, fre_start_addr_sz);
    }
  else
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  return 0;
}

/* Write a frame row entry pointed to by FREP into the buffer CONTENTS.  The
   size in bytes written out are updated in ESZ.

   This function works closely with the SFrame binary format.

   Returns SFRAME_ERR if failure.  */

static int
sframe_encoder_write_fre (char *contents, sframe_frame_row_entry *frep,
			  uint32_t fre_type, size_t *esz)
{
  size_t fre_sz;
  size_t fre_start_addr_sz;
  size_t fre_datawords_sz;
  int err = 0;

  if (!sframe_fre_sanity_check_p (frep))
    return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

  fre_start_addr_sz = sframe_fre_start_addr_size (fre_type);
  fre_datawords_sz = sframe_fre_datawords_bytes_size (frep->fre_info);

  /* The FRE start address must be encodable in the available number of
     bytes.  */
  uint64_t bitmask = SFRAME_BITMASK_OF_SIZE (fre_start_addr_sz);
  sframe_assert ((uint64_t)frep->fre_start_addr <= bitmask);

  sframe_encoder_write_fre_start_addr (contents, frep->fre_start_addr,
				       fre_type, fre_start_addr_sz);
  contents += fre_start_addr_sz;

  memcpy (contents, &frep->fre_info, sizeof (frep->fre_info));
  contents += sizeof (frep->fre_info);

  memcpy (contents, frep->fre_datawords, fre_datawords_sz);
  contents+= fre_datawords_sz;

  fre_sz = sframe_fre_entry_size (frep, fre_type);
  /* Sanity checking.  */
  sframe_assert ((fre_start_addr_sz
		  + sizeof (frep->fre_info)
		  + fre_datawords_sz) == fre_sz);

  *esz = fre_sz;

  return 0;
}

/* Write an SFrame FDE Index element for SFrame V3 into the provided buffer at
   CONTENTS.  Update FDE_WRITE_SIZE with the number of bytes written to the
   buffer.  Return 0 on success, SFRAME_ERR otherwise.  */

static int
sframe_encoder_write_fde_idx (const sframe_header *sfhp ATTRIBUTE_UNUSED,
			      char *contents,
			      const sframe_func_desc_entry_int *fde,
			      size_t *fde_write_size)
{
  sframe_func_desc_idx_v3 *fdep = (sframe_func_desc_idx_v3 *)contents;

  fdep->sfdi_func_start_offset = fde->func_start_pc_offset;
  fdep->sfdi_func_size = fde->func_size;
  fdep->sfdi_func_start_fre_off = fde->func_start_fre_off;

  *fde_write_size = sizeof (sframe_func_desc_idx_v3);

  return 0;
}

/* Write the SFrame FDE Attribute element for SFrame V3 into the provided
   buffer at CONTENTS.  Return 0 on success, SFRAME_ERR otherwise.  */

static int
sframe_encoder_write_fde_attr (char *contents,
			       const sframe_func_desc_entry_int *fde)
{
  sframe_func_desc_attr_v3 *fattr = (sframe_func_desc_attr_v3 *)contents;

  /* Access to num_fres may be unaligned.  */
  uint16_t num_fres = (uint16_t)fde->func_num_fres;
  memcpy (fattr, &num_fres, sizeof (uint16_t));

  fattr->sfda_func_info = fde->func_info;
  fattr->sfda_func_info2 = fde->func_info2;
  fattr->sfda_func_rep_size = fde->func_rep_size;

  return 0;
}
/* Serialize the core contents of the SFrame section and write out to the
   output buffer held in the encoder context ECTX.  Sort the SFrame FDEs on
   start PC if SORT_FDE_P is true.  Return SFRAME_ERR if failure.  */

static int
sframe_encoder_write_sframe (sframe_encoder_ctx *ectx, bool sort_fde_p)
{
  char *contents;
  size_t buf_size;
  size_t hdr_size;
  size_t fde_write_size, all_fdes_size;
  size_t fre_size;
  size_t esz = 0;
  sframe_header *ehp;
  sf_fde_tbl *fd_info;
  sf_fre_tbl *fr_info;
  uint32_t i, num_fdes;
  uint32_t j, num_fres;
  sframe_func_desc_entry_int *fdep;
  sframe_frame_row_entry *frep;

  uint32_t fre_type;
  int err = 0;

  contents = ectx->sfe_data;
  buf_size = ectx->sfe_data_size;
  num_fdes = sframe_encoder_get_num_fidx (ectx);
  all_fdes_size = num_fdes * sizeof (sframe_func_desc_idx_v3);
  ehp = sframe_encoder_get_header (ectx);
  hdr_size = sframe_get_hdr_size (ehp);

  fd_info = ectx->sfe_funcdesc;
  fr_info = ectx->sfe_fres;

  /* Sanity checks:
     - buffers must be malloc'd by the caller.  */
  if ((contents == NULL) || (buf_size < hdr_size))
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);
  if (ehp->sfh_num_fres > 0 && fr_info == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

  /* Write out the FRE table first.

     Recall that read/write of FREs needs information from the corresponding
     FDE; the latter stores the information about the FRE type record used for
     the function.  Also note that sorting of FDEs does NOT impact the order
     in which FREs are stored in the SFrame's FRE sub-section.  This means
     that writing out FREs after sorting of FDEs will need some additional
     book-keeping.  At this time, we can afford to avoid it by writing out
     the FREs first to the output buffer.  */
  fre_size = 0;
  uint32_t global = 0;
  uint32_t fre_index = 0;

  contents += hdr_size + all_fdes_size;
  for (i = 0; i < num_fdes; i++)
    {
      fdep = &fd_info->entry[i];
      fre_type = sframe_get_fre_type (fdep);
      num_fres = fdep->func_num_fres;

      if (num_fres > 0 && fr_info == NULL)
	return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

      sframe_encoder_write_fde_attr (contents, fdep);
      contents += sizeof (sframe_func_desc_attr_v3);
      fre_size += sizeof (sframe_func_desc_attr_v3);

      for (j = 0; j < num_fres; j++)
	{
	  fre_index = global + j;
	  frep = &fr_info->entry[fre_index];

	  sframe_encoder_write_fre (contents, frep, fre_type, &esz);
	  contents += esz;
	  fre_size += esz; /* For debugging only.  */
	}
      global += j;
    }

  sframe_assert (fre_size == ehp->sfh_fre_len);
  sframe_assert (global == ehp->sfh_num_fres);
  sframe_assert ((size_t)(contents - ectx->sfe_data) == buf_size);

  /* Sort the FDE table */
  if (sort_fde_p)
    sframe_sort_funcdesc (ectx);

  /* Sanity checks:
     - the FDE section must have been sorted by now on the start address
     of each function, if sorting was needed.  */
  if ((sort_fde_p != (sframe_encoder_get_flags (ectx) & SFRAME_F_FDE_SORTED))
      || (fd_info == NULL))
    return sframe_set_errno (&err, SFRAME_ERR_FDE_INVAL);

  contents = ectx->sfe_data;
  /* Write out the SFrame header.  The SFrame header in the encoder
     object has already been updated with correct offsets by the caller.  */
  memcpy (contents, ehp, hdr_size);
  contents += hdr_size;

  /* Write out the FDE table sorted on funtion start address.  */
  for (i = 0; i < num_fdes; i++)
    {
      sframe_encoder_write_fde_idx (ehp, contents, &fd_info->entry[i],
				    &fde_write_size);
      contents += fde_write_size;
    }

  return 0;
}

/* Serialize the contents of the encoder context ECTX and return the buffer.
   Sort the SFrame FDEs on start PC if SORT_FDE_P is true.  ENCODED_SIZE is
   updated to the size of the buffer.  Sets ERRP if failure.  */

char *
sframe_encoder_write (sframe_encoder_ctx *ectx, size_t *encoded_size,
		      bool sort_fde_p, int *errp)
{
  sframe_header *ehp;
  size_t hdrsize, fsz, fresz, bufsize;
  int foreign_endian;

  /* Initialize the encoded_size to zero.  This makes it simpler to just
     return from the function in case of failure.  Free'ing up of
     ectx->sfe_data is the responsibility of the caller.  */
  *encoded_size = 0;

  if (ectx == NULL || encoded_size == NULL || errp == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_INVAL);

  ehp = sframe_encoder_get_header (ectx);
  hdrsize = sframe_get_hdr_size (ehp);
  fsz = sframe_encoder_get_num_fidx (ectx) * sizeof (sframe_func_desc_idx_v3);
  fresz = ectx->sfe_fre_nbytes;

  /* Encoder writes out data in the latest SFrame format version.  */
  if (sframe_encoder_get_version (ectx) != SFRAME_VERSION)
    return sframe_ret_set_errno (errp, SFRAME_ERR_VERSION_INVAL);

  /* The total size of buffer is the sum of header, SFrame Function Descriptor
     Entries section and the FRE section.  */
  bufsize = hdrsize + fsz + fresz;
  ectx->sfe_data = (char *) malloc (bufsize);
  if (ectx->sfe_data == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
  ectx->sfe_data_size = bufsize;

  /* Update the information in the SFrame header.  */
  /* SFrame FDE section follows immediately after the header.  */
  ehp->sfh_fdeoff = 0;
  /* SFrame FRE section follows immediately after the SFrame FDE section.  */
  ehp->sfh_freoff = fsz;
  ehp->sfh_fre_len = fresz;

  foreign_endian = need_swapping (ehp->sfh_abi_arch);

  /* Write out the FDE Index and the FRE table in the sfe_data. */
  if (sframe_encoder_write_sframe (ectx, sort_fde_p))
    return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);

  /* Endian flip the contents if necessary.  */
  if (foreign_endian)
    {
      if (flip_sframe (ectx->sfe_data, bufsize, 1))
	return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
      if (flip_header (ectx->sfe_data, SFRAME_VERSION))
	return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
    }

  *encoded_size = bufsize;
  return ectx->sfe_data;
}
