/* sframe.c - SFrame decoder/encoder.

   Copyright (C) 2022 Free Software Foundation, Inc.

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
#include "sframe-impl.h"
#include "swap.h"

typedef struct sf_funidx_tbl
{
  unsigned int count;
  unsigned int alloced;
  sframe_func_desc_entry entry[1];
} sf_funidx_tbl;

typedef struct sf_fre_tbl
{
  unsigned int count;
  unsigned int alloced;
  sframe_frame_row_entry entry[1];
} sf_fre_tbl;

#define _sf_printflike_(string_index,first_to_check) \
    __attribute__ ((__format__ (__printf__, (string_index), (first_to_check))))

static void debug_printf (const char *, ...);

static int _sframe_debug;	/* Control for printing out debug info.  */
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

/* Get the SFrame header size.  */

static uint32_t
sframe_get_hdr_size (sframe_header *sfh)
{
  return SFRAME_V1_HDR_SIZE (*sfh);
}

/* Access functions for frame row entry data.  */

static unsigned int
sframe_fre_get_offset_count (unsigned char fre_info)
{
  return SFRAME_V1_FRE_OFFSET_COUNT (fre_info);
}

static unsigned int
sframe_fre_get_offset_size (unsigned char fre_info)
{
  return SFRAME_V1_FRE_OFFSET_SIZE (fre_info);
}

static bool
sframe_get_fre_ra_mangled_p (unsigned char fre_info)
{
  return SFRAME_V1_FRE_MANGLED_RA_P (fre_info);
}

/* Access functions for info from function descriptor entry.  */

static unsigned int
sframe_get_fre_type (sframe_func_desc_entry *fdep)
{
  unsigned int fre_type = 0;
  if (fdep)
    fre_type = SFRAME_V1_FUNC_FRE_TYPE (fdep->sfde_func_info);
  return fre_type;
}

static unsigned int
sframe_get_fde_type (sframe_func_desc_entry *fdep)
{
  unsigned int fde_type = 0;
  if (fdep)
    fde_type = SFRAME_V1_FUNC_FDE_TYPE (fdep->sfde_func_info);
  return fde_type;
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
	return is_little;
      default:
	break;
    }

  return 0;
}

/* Flip the endianness of the SFrame header.  */

static void
flip_header (sframe_header *sfheader)
{
  swap_thing (sfheader->sfh_preamble.sfp_magic);
  swap_thing (sfheader->sfh_preamble.sfp_version);
  swap_thing (sfheader->sfh_preamble.sfp_flags);
  swap_thing (sfheader->sfh_cfa_fixed_fp_offset);
  swap_thing (sfheader->sfh_cfa_fixed_ra_offset);
  swap_thing (sfheader->sfh_num_fdes);
  swap_thing (sfheader->sfh_num_fres);
  swap_thing (sfheader->sfh_fre_len);
  swap_thing (sfheader->sfh_fdeoff);
  swap_thing (sfheader->sfh_freoff);
}

static void
flip_fde (sframe_func_desc_entry *fdep)
{
  swap_thing (fdep->sfde_func_start_address);
  swap_thing (fdep->sfde_func_size);
  swap_thing (fdep->sfde_func_start_fre_off);
  swap_thing (fdep->sfde_func_num_fres);
}

/* Check if SFrame header has valid data.  */

static int
sframe_header_sanity_check_p (sframe_header *hp)
{
  unsigned char all_flags = SFRAME_F_FDE_SORTED | SFRAME_F_FRAME_POINTER;
  /* Check preamble is valid.  */
  if ((hp->sfh_preamble.sfp_magic != SFRAME_MAGIC)
      || (hp->sfh_preamble.sfp_version != SFRAME_VERSION)
      || ((hp->sfh_preamble.sfp_flags | all_flags) != all_flags))
    return 0;

  /* Check offsets are valid.  */
  if (hp->sfh_fdeoff > hp->sfh_freoff)
    return 0;

  return 1;
}

/* Flip the start address pointed to by FP.  */

static void
flip_fre_start_address (char *fp, unsigned int fre_type)
{
  void *start = (void*)fp;
  if (fre_type == SFRAME_FRE_TYPE_ADDR2)
    {
      unsigned short *start_addr = (unsigned short *)(start);
      swap_thing (*start_addr);
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR4)
    {
      uint32_t *start_addr = (uint32_t *)(start);
      swap_thing (*start_addr);
    }
}

static void
flip_fre_stack_offsets (char *fp, unsigned char offset_size,
			unsigned char offset_cnt)
{
  int j;
  void *offsets = (void *)fp;

  if (offset_size == SFRAME_FRE_OFFSET_2B)
    {
      unsigned short *ust = (unsigned short *)offsets;
      for (j = offset_cnt; j > 0; ust++, j--)
	swap_thing (*ust);
    }
  else if (offset_size == SFRAME_FRE_OFFSET_4B)
    {
      uint32_t *uit = (uint32_t *)offsets;
      for (j = offset_cnt; j > 0; uit++, j--)
	swap_thing (*uit);
    }
}

/* Get the FRE start address size, given the FRE_TYPE.  */

static size_t
sframe_fre_start_addr_size (unsigned int fre_type)
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

static int
sframe_fre_sanity_check_p (sframe_frame_row_entry *frep)
{
  unsigned int offset_size, offset_cnt;
  unsigned int fre_info;

  if (frep == NULL)
    return 0;

  fre_info = frep->fre_info;
  offset_size = sframe_fre_get_offset_size (fre_info);

  if (offset_size != SFRAME_FRE_OFFSET_1B
      && offset_size != SFRAME_FRE_OFFSET_2B
      && offset_size != SFRAME_FRE_OFFSET_4B)
    return 0;

  offset_cnt = sframe_fre_get_offset_count (fre_info);
  if (offset_cnt > 3)
    return 0;

  return 1;
}

/* Get FRE_INFO's offset size in bytes.  */

static size_t
sframe_fre_offset_bytes_size (unsigned char fre_info)
{
  unsigned int offset_size, offset_cnt;

  offset_size = sframe_fre_get_offset_size (fre_info);

  debug_printf ("offset_size =  %u\n", offset_size);

  offset_cnt = sframe_fre_get_offset_count (fre_info);

  if (offset_size == SFRAME_FRE_OFFSET_2B
      || offset_size == SFRAME_FRE_OFFSET_4B)	/* 2 or 4 bytes.  */
    return (offset_cnt * (offset_size * 2));

  return (offset_cnt);
}

/* Get total size in bytes to represent FREP in the binary format.  This
   includes the starting address, FRE info, and all the offsets.  */

static size_t
sframe_fre_entry_size (sframe_frame_row_entry *frep, unsigned int fre_type)
{
  if (frep == NULL)
    return 0;

  unsigned char fre_info = frep->fre_info;
  size_t addr_size = sframe_fre_start_addr_size (fre_type);

  return (addr_size + sizeof (frep->fre_info)
	  + sframe_fre_offset_bytes_size (fre_info));
}

static int
flip_fre (char *fp, unsigned int fre_type, size_t *fre_size)
{
  unsigned char fre_info;
  unsigned int offset_size, offset_cnt;
  size_t addr_size, fre_info_size = 0;
  int err = 0;

  if (fre_size == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  flip_fre_start_address (fp, fre_type);

  /* Advance the buffer pointer to where the FRE info is.  */
  addr_size = sframe_fre_start_addr_size (fre_type);
  fp += addr_size;

  /* FRE info is unsigned char.  No need to flip.  */
  fre_info = *(unsigned char*)fp;
  offset_size = sframe_fre_get_offset_size (fre_info);
  offset_cnt = sframe_fre_get_offset_count (fre_info);

  /* Advance the buffer pointer to where the stack offsets are.  */
  fre_info_size = sizeof (unsigned char);
  fp += fre_info_size;
  flip_fre_stack_offsets (fp, offset_size, offset_cnt);

  *fre_size
    = addr_size + fre_info_size + sframe_fre_offset_bytes_size (fre_info);

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
flip_sframe (char *frame_buf, size_t buf_size, uint32_t to_foreign)
{
  unsigned int i, j, prev_frep_index;
  sframe_header *ihp;
  char *fdes;
  char *fp = NULL;
  sframe_func_desc_entry *fdep;
  unsigned int num_fdes = 0;
  unsigned int num_fres = 0;
  unsigned int fre_type = 0;
  uint32_t fre_offset = 0;
  size_t esz = 0;
  int err = 0;

  /* Header must be in host endianness at this time.  */
  ihp = (sframe_header *)frame_buf;

  if (!sframe_header_sanity_check_p (ihp))
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);

  /* The contents of the SFrame header are safe to read.  Get the number of
     FDEs and the first FDE in the buffer.  */
  num_fdes = ihp->sfh_num_fdes;
  fdes = frame_buf + sframe_get_hdr_size (ihp) + ihp->sfh_fdeoff;
  fdep = (sframe_func_desc_entry *)fdes;

  j = 0;
  prev_frep_index = 0;
  for (i = 0; i < num_fdes; fdep++, i++)
    {
      if (to_foreign)
	{
	  num_fres = fdep->sfde_func_num_fres;
	  fre_type = sframe_get_fre_type (fdep);
	  fre_offset = fdep->sfde_func_start_fre_off;
	}

      flip_fde (fdep);

      if (!to_foreign)
	{
	  num_fres = fdep->sfde_func_num_fres;
	  fre_type = sframe_get_fre_type (fdep);
	  fre_offset = fdep->sfde_func_start_fre_off;
	}

      fp = frame_buf + sframe_get_hdr_size (ihp) + ihp->sfh_freoff;
      fp += fre_offset;
      for (; j < prev_frep_index + num_fres; j++)
	{
	  if (flip_fre (fp, fre_type, &esz))
	    goto bad;

	  if (esz == 0)
	    goto bad;
	  fp += esz;
	}
      prev_frep_index = j;
    }
  /* All FREs must have been endian flipped by now.  */
  if (j != ihp->sfh_num_fres)
    goto bad;
  /* Contents, if any, must have been processed by now.
     Recall that .sframe section with just a SFrame header may be generated by
     GAS if no SFrame FDEs were found for the input file.  */
  if (ihp->sfh_num_fres && ((frame_buf + buf_size) != (void*)fp))
    goto bad;

  /* Success.  */
  return 0;
bad:
  return SFRAME_ERR;
}

/* The SFrame Decoder.  */

/* Get DECODER's SFrame header.  */

static sframe_header *
sframe_decoder_get_header (sframe_decoder_ctx *decoder)
{
  sframe_header *hp = NULL;
  if (decoder != NULL)
    hp = &decoder->sfd_header;
  return hp;
}

/* Compare function for qsort'ing the FDE table.  */

static int
fde_func (const void *p1, const void *p2)
{
  const sframe_func_desc_entry *aa = p1;
  const sframe_func_desc_entry *bb = p2;

  if (aa->sfde_func_start_address < bb->sfde_func_start_address)
    return -1;
  else if (aa->sfde_func_start_address > bb->sfde_func_start_address)
    return 1;
  return 0;
}

/* Get IDX'th offset from FRE.  Set errp as applicable.  */

static int32_t
sframe_get_fre_offset (sframe_frame_row_entry *fre, int idx, int *errp)
{
  int offset_cnt, offset_size;

  if (fre == NULL || !sframe_fre_sanity_check_p (fre))
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  offset_cnt = sframe_fre_get_offset_count (fre->fre_info);
  offset_size = sframe_fre_get_offset_size (fre->fre_info);

  if (offset_cnt < idx + 1)
    return sframe_set_errno (errp, SFRAME_ERR_FREOFFSET_NOPRESENT);

  if (errp)
    *errp = 0; /* Offset Valid.  */

  if (offset_size == SFRAME_FRE_OFFSET_1B)
    {
      int8_t *sp = (int8_t *)fre->fre_offsets;
      return sp[idx];
    }
  else if (offset_size == SFRAME_FRE_OFFSET_2B)
    {
      int16_t *sp = (int16_t *)fre->fre_offsets;
      return sp[idx];
    }
  else
    {
      int32_t *ip = (int32_t *)fre->fre_offsets;
      return ip[idx];
    }
}

/* Free the decoder context.  */

void
sframe_decoder_free (sframe_decoder_ctx **decoder)
{
  if (decoder != NULL)
    {
      sframe_decoder_ctx *dctx = *decoder;
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

      free (*decoder);
      *decoder = NULL;
    }
}

/* Create an FDE function info byte given an FRE_TYPE and an FDE_TYPE.  */
/* FIXME API for linker.  Revisit if its better placed somewhere else?  */

unsigned char
sframe_fde_create_func_info (unsigned int fre_type,
			     unsigned int fde_type)
{
  unsigned char func_info;
  sframe_assert (fre_type == SFRAME_FRE_TYPE_ADDR1
		   || fre_type == SFRAME_FRE_TYPE_ADDR2
		   || fre_type == SFRAME_FRE_TYPE_ADDR4);
  sframe_assert (fde_type == SFRAME_FDE_TYPE_PCINC
		    || fde_type == SFRAME_FDE_TYPE_PCMASK);
  func_info = SFRAME_V1_FUNC_INFO (fde_type, fre_type);
  return func_info;
}

/* Get the FRE type given the function size.  */
/* FIXME API for linker.  Revisit if its better placed somewhere else?  */

unsigned int
sframe_calc_fre_type (unsigned int func_size)
{
  unsigned int fre_type = 0;
  if (func_size < SFRAME_FRE_TYPE_ADDR1_LIMIT)
    fre_type = SFRAME_FRE_TYPE_ADDR1;
  else if (func_size < SFRAME_FRE_TYPE_ADDR2_LIMIT)
    fre_type = SFRAME_FRE_TYPE_ADDR2;
  else if (func_size < SFRAME_FRE_TYPE_ADDR4_LIMIT)
    fre_type = SFRAME_FRE_TYPE_ADDR4;
  return fre_type;
}

/* Get the base reg id from the FRE info.  Set errp if failure.  */

unsigned int
sframe_fre_get_base_reg_id (sframe_frame_row_entry *fre, int *errp)
{
  if (fre == NULL)
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  unsigned int fre_info = fre->fre_info;
  return SFRAME_V1_FRE_CFA_BASE_REG_ID (fre_info);
}

/* Get the CFA offset from the FRE.  If the offset is invalid, sets errp.  */

int32_t
sframe_fre_get_cfa_offset (sframe_decoder_ctx *dctx ATTRIBUTE_UNUSED,
			   sframe_frame_row_entry *fre, int *errp)
{
  return sframe_get_fre_offset (fre, SFRAME_FRE_CFA_OFFSET_IDX, errp);
}

/* Get the FP offset from the FRE.  If the offset is invalid, sets errp.  */

int32_t
sframe_fre_get_fp_offset (sframe_decoder_ctx *dctx,
			  sframe_frame_row_entry *fre, int *errp)
{
  uint32_t fp_offset_idx = 0;
  sframe_header *dhp = sframe_decoder_get_header (dctx);
  /* If the FP offset is not being tracked, return an error code so the caller
     can gather the fixed FP offset from the SFrame header.  */
  if (dhp->sfh_cfa_fixed_fp_offset != SFRAME_CFA_FIXED_FP_INVALID)
    return sframe_set_errno (errp, SFRAME_ERR_FREOFFSET_NOPRESENT);

  /* In some ABIs, the stack offset to recover RA (using the CFA) from is
     fixed (like AMD64).  In such cases, the stack offset to recover FP will
     appear at the second index.  */
  fp_offset_idx = ((dhp->sfh_cfa_fixed_ra_offset != SFRAME_CFA_FIXED_RA_INVALID)
		   ? SFRAME_FRE_RA_OFFSET_IDX
		   : SFRAME_FRE_FP_OFFSET_IDX);
  return sframe_get_fre_offset (fre, fp_offset_idx, errp);
}

/* Get the RA offset from the FRE.  If the offset is invalid, sets errp.  */

int32_t
sframe_fre_get_ra_offset (sframe_decoder_ctx *dctx,
			  sframe_frame_row_entry *fre, int *errp)
{
  sframe_header *dhp = sframe_decoder_get_header (dctx);
  /* If the RA offset was not being tracked, return an error code so the caller
     can gather the fixed RA offset from the SFrame header.  */
  if (dhp->sfh_cfa_fixed_ra_offset != SFRAME_CFA_FIXED_RA_INVALID)
    return sframe_set_errno (errp, SFRAME_ERR_FREOFFSET_NOPRESENT);

  /* Otherwise, get the RA offset from the FRE.  */
  return sframe_get_fre_offset (fre, SFRAME_FRE_RA_OFFSET_IDX, errp);
}

/* Get whether the RA is mangled.  */

bool
sframe_fre_get_ra_mangled_p (sframe_decoder_ctx *dctx ATTRIBUTE_UNUSED,
			     sframe_frame_row_entry *fre, int *errp)
{
  if (fre == NULL || !sframe_fre_sanity_check_p (fre))
    return sframe_set_errno (errp, SFRAME_ERR_FRE_INVAL);

  return sframe_get_fre_ra_mangled_p (fre->fre_info);
}

static int
sframe_frame_row_entry_copy (sframe_frame_row_entry *dst, sframe_frame_row_entry *src)
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
sframe_decode_fre_start_address (const char *fre_buf,
				 uint32_t *fre_start_addr,
				 unsigned int fre_type)
{
  uint32_t saddr = 0;
  int err = 0;
  size_t addr_size = 0;

  addr_size = sframe_fre_start_addr_size (fre_type);

  if (fre_type == SFRAME_FRE_TYPE_ADDR1)
    {
      uint8_t *uc = (uint8_t *)fre_buf;
      saddr = (uint32_t)*uc;
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR2)
    {
      uint16_t *ust = (uint16_t *)fre_buf;
      /* SFrame is an unaligned on-disk format.  Using memcpy helps avoid the
	 use of undesirable unaligned loads.  See PR libsframe/29856.  */
      uint16_t tmp = 0;
      memcpy (&tmp, ust, addr_size);
      saddr = (uint32_t)tmp;
    }
  else if (fre_type == SFRAME_FRE_TYPE_ADDR4)
    {
      uint32_t *uit = (uint32_t *)fre_buf;
      int32_t tmp = 0;
      memcpy (&tmp, uit, addr_size);
      saddr = (uint32_t)tmp;
    }
  else
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  *fre_start_addr = saddr;
  return 0;
}

/* Decode a frame row entry FRE which starts at location FRE_BUF.  The function
   updates ESZ to the size of the FRE as stored in the binary format.

   This function works closely with the SFrame binary format.

   Returns SFRAME_ERR if failure.  */

static int
sframe_decode_fre (const char *fre_buf, sframe_frame_row_entry *fre,
		   unsigned int fre_type,
		   size_t *esz)
{
  int err = 0;
  void *stack_offsets = NULL;
  size_t stack_offsets_sz;
  size_t addr_size;
  size_t fre_size;

  if (fre_buf == NULL || fre == NULL || esz == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  /* Copy over the FRE start address.  */
  sframe_decode_fre_start_address (fre_buf, &fre->fre_start_addr, fre_type);

  addr_size = sframe_fre_start_addr_size (fre_type);
  fre->fre_info = *(unsigned char *)(fre_buf + addr_size);
  /* Sanity check as the API works closely with the binary format.  */
  sframe_assert (sizeof (fre->fre_info) == sizeof (unsigned char));

  /* Cleanup the space for fre_offsets first, then copy over the valid
     bytes.  */
  memset (fre->fre_offsets, 0, MAX_OFFSET_BYTES);
  /* Get offsets size.  */
  stack_offsets_sz = sframe_fre_offset_bytes_size (fre->fre_info);
  stack_offsets = (unsigned char *)fre_buf + addr_size + sizeof (fre->fre_info);
  memcpy (fre->fre_offsets, stack_offsets, stack_offsets_sz);

  /* The FRE has been decoded.  Use it to perform one last sanity check.  */
  fre_size = sframe_fre_entry_size (fre, fre_type);
  sframe_assert (fre_size == (addr_size + sizeof (fre->fre_info)
			      + stack_offsets_sz));
  *esz = fre_size;

  return 0;
}

/* Decode the specified SFrame buffer CF_BUF of size CF_SIZE and return the
   new SFrame decoder context.

   Sets ERRP for the caller if any error.  Frees up the allocated memory in
   case of error.  */

sframe_decoder_ctx *
sframe_decode (const char *sf_buf, size_t sf_size, int *errp)
{
  const sframe_preamble *sfp;
  size_t hdrsz;
  sframe_header *sfheaderp;
  sframe_decoder_ctx *dctx;
  char *frame_buf;
  char *tempbuf = NULL;

  int fidx_size;
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

      /* Flip the header.  */
      sframe_header *ihp = (sframe_header *) tempbuf;
      flip_header (ihp);
      /* Flip the rest of the SFrame section data buffer.  */
      if (flip_sframe (tempbuf, sf_size, 0))
	{
	  free (tempbuf);
	  return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
	}
      frame_buf = tempbuf;
    }
  else
    frame_buf = (char *)sf_buf;

  /* Handle the SFrame header.  */
  dctx->sfd_header = *(sframe_header *) frame_buf;
  /* Validate the contents of SFrame header.  */
  sfheaderp = &dctx->sfd_header;
  if (!sframe_header_sanity_check_p (sfheaderp))
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
      goto decode_fail_free;
    }
  hdrsz = sframe_get_hdr_size (sfheaderp);
  frame_buf += hdrsz;

  /* Handle the SFrame Function Descriptor Entry section.  */
  fidx_size
    = sfheaderp->sfh_num_fdes * sizeof (sframe_func_desc_entry);
  dctx->sfd_funcdesc = malloc (fidx_size);
  if (dctx->sfd_funcdesc == NULL)
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
      goto decode_fail_free;
    }
  memcpy (dctx->sfd_funcdesc, frame_buf, fidx_size);

  debug_printf ("%u total fidx size\n", fidx_size);

  frame_buf += (fidx_size);

  /* Handle the SFrame Frame Row Entry section.  */
  dctx->sfd_fres = malloc (sfheaderp->sfh_fre_len);
  if (dctx->sfd_fres == NULL)
    {
      sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
      goto decode_fail_free;
    }
  memcpy (dctx->sfd_fres, frame_buf, sfheaderp->sfh_fre_len);

  fre_bytes = sfheaderp->sfh_fre_len;
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
sframe_decoder_get_hdr_size (sframe_decoder_ctx *ctx)
{
  sframe_header *dhp;
  dhp = sframe_decoder_get_header (ctx);
  return sframe_get_hdr_size (dhp);
}

/* Get the SFrame's abi/arch info given the decoder context CTX.  */

unsigned char
sframe_decoder_get_abi_arch (sframe_decoder_ctx *ctx)
{
  sframe_header *sframe_header;
  sframe_header = sframe_decoder_get_header (ctx);
  return sframe_header->sfh_abi_arch;
}

/* Get the SFrame's fixed FP offset given the decoder context CTX.  */
int8_t
sframe_decoder_get_fixed_fp_offset (sframe_decoder_ctx *ctx)
{
  sframe_header *dhp;
  dhp = sframe_decoder_get_header (ctx);
  return dhp->sfh_cfa_fixed_fp_offset;
}

/* Get the SFrame's fixed RA offset given the decoder context CTX.  */
int8_t
sframe_decoder_get_fixed_ra_offset (sframe_decoder_ctx *ctx)
{
  sframe_header *dhp;
  dhp = sframe_decoder_get_header (ctx);
  return dhp->sfh_cfa_fixed_ra_offset;
}

/* Find the function descriptor entry starting which contains the specified
   address ADDR.  */

sframe_func_desc_entry *
sframe_get_funcdesc_with_addr (sframe_decoder_ctx *ctx,
			       int32_t addr, int *errp)
{
  sframe_header *dhp;
  sframe_func_desc_entry *fdp;
  int low, high, cnt;

  if (ctx == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_INVAL);

  dhp = sframe_decoder_get_header (ctx);

  if (dhp == NULL || dhp->sfh_num_fdes == 0 || ctx->sfd_funcdesc == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_DCTX_INVAL);
  /* If the FDE sub-section is not sorted on PCs, skip the lookup because
     binary search cannot be used.  */
  if ((dhp->sfh_preamble.sfp_flags & SFRAME_F_FDE_SORTED) == 0)
    return sframe_ret_set_errno (errp, SFRAME_ERR_FDE_NOTSORTED);

  /* Do the binary search.  */
  fdp = (sframe_func_desc_entry *) ctx->sfd_funcdesc;
  low = 0;
  high = dhp->sfh_num_fdes;
  cnt = high;
  while (low <= high)
    {
      int mid = low + (high - low) / 2;

      if (fdp[mid].sfde_func_start_address == addr)
	return fdp + mid;

      if (fdp[mid].sfde_func_start_address < addr)
	{
	  if (mid == (cnt - 1)) 	/* Check if it's the last one.  */
	    return fdp + (cnt - 1) ;
	  else if (fdp[mid+1].sfde_func_start_address > addr)
	    return fdp + mid;
	  low = mid + 1;
	}
      else
	high = mid - 1;
    }

  return sframe_ret_set_errno (errp, SFRAME_ERR_FDE_NOTFOUND);
}

/* Find the SFrame Row Entry which contains the PC.  Returns
   SFRAME_ERR if failure.  */

int
sframe_find_fre (sframe_decoder_ctx *ctx, int32_t pc,
		 sframe_frame_row_entry *frep)
{
  sframe_func_desc_entry *fdep;
  uint32_t start_address, i;
  sframe_frame_row_entry cur_fre, next_fre;
  unsigned char *sp;
  unsigned int fre_type, fde_type;
  size_t esz;
  int err = 0;
  size_t size = 0;
  /* For regular FDEs (i.e. fde_type SFRAME_FDE_TYPE_PCINC),
     where the start address in the FRE is an offset from start pc,
     use a bitmask with all bits set so that none of the address bits are
     ignored.  In this case, we need to return the FRE where
     (PC >= FRE_START_ADDR) */
  uint64_t bitmask = 0xffffffff;

  if ((ctx == NULL) || (frep == NULL))
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  /* Find the FDE which contains the PC, then scan its fre entries.  */
  fdep = sframe_get_funcdesc_with_addr (ctx, pc, &err);
  if (fdep == NULL || ctx->sfd_fres == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_DCTX_INVAL);

  fre_type = sframe_get_fre_type (fdep);
  fde_type = sframe_get_fde_type (fdep);

  /* For FDEs for repetitive pattern of insns, we need to return the FRE
     such that (PC & FRE_START_ADDR_AS_MASK >= FRE_START_ADDR_AS_MASK).
     so, update the bitmask to the start address.  */
  /* FIXME - the bitmask should be picked per ABI or encoded in the format
     somehow. For AMD64, the pltN entry stub is 16 bytes. */
  if (fde_type == SFRAME_FDE_TYPE_PCMASK)
    bitmask = 0xff;

  sp = (unsigned char *) ctx->sfd_fres + fdep->sfde_func_start_fre_off;
  for (i = 0; i < fdep->sfde_func_num_fres; i++)
   {
     err = sframe_decode_fre ((const char *)sp, &next_fre, fre_type, &esz);
     start_address = next_fre.fre_start_addr;

     if (((fdep->sfde_func_start_address
	   + (int32_t) start_address) & bitmask) <= (pc & bitmask))
       {
	 sframe_frame_row_entry_copy (&cur_fre, &next_fre);

	 /* Get the next FRE in sequence.  */
	 if (i < fdep->sfde_func_num_fres - 1)
	   {
	     sp += esz;
	     err = sframe_decode_fre ((const char*)sp, &next_fre,
					 fre_type, &esz);

	     /* Sanity check the next FRE.  */
	     if (!sframe_fre_sanity_check_p (&next_fre))
	       return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

	     size = next_fre.fre_start_addr;
	   }
	 else size = fdep->sfde_func_size;

	 /* If the cur FRE is the one that contains the PC, return it.  */
	 if (((fdep->sfde_func_start_address
	       + (int32_t)size) & bitmask) > (pc & bitmask))
	   {
	     sframe_frame_row_entry_copy (frep, &cur_fre);
	     return 0;
	   }
       }
     else
       return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);
   }
  return sframe_set_errno (&err, SFRAME_ERR_FDE_INVAL);
}

/* Return the number of function descriptor entries in the SFrame decoder
   DCTX.  */

unsigned int
sframe_decoder_get_num_fidx (sframe_decoder_ctx *ctx)
{
  unsigned int num_fdes = 0;
  sframe_header *dhp = NULL;
  dhp = sframe_decoder_get_header (ctx);
  if (dhp)
    num_fdes = dhp->sfh_num_fdes;
  return num_fdes;
}

/* Get the data (NUM_FRES, FUNC_START_ADDRESS) from the function
   descriptor entry at index I'th in the decoder CTX.  If failed,
   return error code.  */
/* FIXME - consolidate the args and return a
   sframe_func_desc_index_elem rather?  */

int
sframe_decoder_get_funcdesc (sframe_decoder_ctx *ctx,
			     unsigned int i,
			     uint32_t *num_fres,
			     uint32_t *func_size,
			     int32_t *func_start_address,
			     unsigned char *func_info)
{
  sframe_func_desc_entry *fdp;
  unsigned int num_fdes;
  int err = 0;

  if (ctx == NULL || func_start_address == NULL || num_fres == NULL
      || func_size == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  num_fdes = sframe_decoder_get_num_fidx (ctx);
  if (num_fdes == 0
      || i >= num_fdes
      || ctx->sfd_funcdesc == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_DCTX_INVAL);

  fdp = (sframe_func_desc_entry *) ctx->sfd_funcdesc + i;
  *num_fres = fdp->sfde_func_num_fres;
  *func_start_address = fdp->sfde_func_start_address;
  *func_size = fdp->sfde_func_size;
  *func_info = fdp->sfde_func_info;

  return 0;
}

/* Get the function descriptor entry at index FUNC_IDX in the decoder
   context CTX.  */

static sframe_func_desc_entry *
sframe_decoder_get_funcdesc_at_index (sframe_decoder_ctx *ctx,
				      uint32_t func_idx)
{
  /* Invalid argument.  No FDE will be found.  */
  if (func_idx >= sframe_decoder_get_num_fidx (ctx))
    return NULL;

  sframe_func_desc_entry *fdep;
  fdep = (sframe_func_desc_entry *) ctx->sfd_funcdesc;
  return fdep + func_idx;
}

/* Get the FRE_IDX'th FRE of the function at FUNC_IDX'th function
   descriptor entry in the SFrame decoder CTX.  Returns error code as
   applicable.  */

int
sframe_decoder_get_fre (sframe_decoder_ctx *ctx,
			unsigned int func_idx,
			unsigned int fre_idx,
			sframe_frame_row_entry *fre)
{
  sframe_func_desc_entry *fdep;
  sframe_frame_row_entry ifre;
  unsigned char *sp;
  uint32_t i;
  unsigned int fre_type;
  size_t esz = 0;
  int err = 0;

  if (ctx == NULL || fre == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  /* Get function descriptor entry at index func_idx.  */
  fdep = sframe_decoder_get_funcdesc_at_index (ctx, func_idx);

  if (fdep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  fre_type = sframe_get_fre_type (fdep);
  /* Now scan the FRE entries.  */
  sp = (unsigned char *) ctx->sfd_fres + fdep->sfde_func_start_fre_off;
  for (i = 0; i < fdep->sfde_func_num_fres; i++)
   {
     /* Decode the FRE at the current position.  Return it if valid.  */
     err = sframe_decode_fre ((const char *)sp, &ifre, fre_type, &esz);
     if (i == fre_idx)
       {
	 if (!sframe_fre_sanity_check_p (&ifre))
	   return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

	 sframe_frame_row_entry_copy (fre, &ifre);

	 if (fdep->sfde_func_size)
	   sframe_assert (fre->fre_start_addr < fdep->sfde_func_size);
	 else
	   /* A SFrame FDE with func size equal to zero is possible.  */
	   sframe_assert (fre->fre_start_addr == fdep->sfde_func_size);

	 return 0;
       }
     /* Next FRE.  */
     sp += esz;
   }

  return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);
}


/* SFrame Encoder.  */

/* Get a reference to the ENCODER's SFrame header.  */

static sframe_header *
sframe_encoder_get_header (sframe_encoder_ctx *encoder)
{
  sframe_header *hp = NULL;
  if (encoder)
    hp = &encoder->sfe_header;
  return hp;
}

static sframe_func_desc_entry *
sframe_encoder_get_funcdesc_at_index (sframe_encoder_ctx *encoder,
				      uint32_t func_idx)
{
  sframe_func_desc_entry *fde = NULL;
  if (func_idx < sframe_encoder_get_num_fidx (encoder))
    {
      sf_funidx_tbl *func_tbl = (sf_funidx_tbl *) encoder->sfe_funcdesc;
      fde = func_tbl->entry + func_idx;
    }
  return fde;
}

/* Create an encoder context with the given SFrame format version VER, FLAGS
   and ABI information.  Sets errp if failure.  */

sframe_encoder_ctx *
sframe_encode (unsigned char ver, unsigned char flags, int abi_arch,
	       int8_t fixed_fp_offset, int8_t fixed_ra_offset, int *errp)
{
  sframe_header *hp;
  sframe_encoder_ctx *fp;

  if (ver != SFRAME_VERSION)
    return sframe_ret_set_errno (errp, SFRAME_ERR_VERSION_INVAL);

  if ((fp = malloc (sizeof (sframe_encoder_ctx))) == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);

  memset (fp, 0, sizeof (sframe_encoder_ctx));

  /* Get the SFrame header and update it.  */
  hp = sframe_encoder_get_header (fp);
  hp->sfh_preamble.sfp_version = ver;
  hp->sfh_preamble.sfp_magic = SFRAME_MAGIC;
  hp->sfh_preamble.sfp_flags = flags;

  hp->sfh_abi_arch = abi_arch;
  hp->sfh_cfa_fixed_fp_offset = fixed_fp_offset;
  hp->sfh_cfa_fixed_ra_offset = fixed_ra_offset;

  return fp;
}

/* Free the encoder context.  */

void
sframe_encoder_free (sframe_encoder_ctx **encoder)
{
  if (encoder != NULL)
    {
      sframe_encoder_ctx *ectx = *encoder;
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

      free (*encoder);
      *encoder = NULL;
    }
}

/* Get the size of the SFrame header from the encoder ctx ENCODER.  */

unsigned int
sframe_encoder_get_hdr_size (sframe_encoder_ctx *encoder)
{
  sframe_header *ehp;
  ehp = sframe_encoder_get_header (encoder);
  return sframe_get_hdr_size (ehp);
}

/* Get the abi/arch info from the SFrame encoder context ENCODER.  */

unsigned char
sframe_encoder_get_abi_arch (sframe_encoder_ctx *encoder)
{
  unsigned char abi_arch = 0;
  sframe_header *ehp;
  ehp = sframe_encoder_get_header (encoder);
  if (ehp)
    abi_arch = ehp->sfh_abi_arch;
  return abi_arch;
}

/* Return the number of function descriptor entries in the SFrame encoder
   ENCODER.  */

unsigned int
sframe_encoder_get_num_fidx (sframe_encoder_ctx *encoder)
{
  unsigned int num_fdes = 0;
  sframe_header *ehp = NULL;
  ehp = sframe_encoder_get_header (encoder);
  if (ehp)
    num_fdes = ehp->sfh_num_fdes;
  return num_fdes;
}

/* Add an FRE to function at FUNC_IDX'th function descriptor entry in
   the encoder context.  */

int
sframe_encoder_add_fre (sframe_encoder_ctx *encoder,
			unsigned int func_idx,
			sframe_frame_row_entry *frep)
{
  sframe_header *ehp;
  sframe_func_desc_entry *fdep;
  sframe_frame_row_entry *ectx_frep;
  size_t offsets_sz, esz;
  unsigned int fre_type;
  size_t fre_tbl_sz;
  int err = 0;

  if (encoder == NULL || frep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);
  if (!sframe_fre_sanity_check_p (frep))
    return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

  /* Use func_idx to gather the function descriptor entry.  */
  fdep = sframe_encoder_get_funcdesc_at_index (encoder, func_idx);

  if (fdep == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_FDE_NOTFOUND);

  fre_type = sframe_get_fre_type (fdep);
  sf_fre_tbl *fre_tbl = (sf_fre_tbl *) encoder->sfe_fres;

  if (fre_tbl == NULL)
    {
      fre_tbl_sz = (sizeof (sf_fre_tbl)
		    + (number_of_entries * sizeof (sframe_frame_row_entry)));
      fre_tbl = malloc (fre_tbl_sz);

      if (fre_tbl == NULL)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;		/* OOM.  */
	}
      memset (fre_tbl, 0, fre_tbl_sz);
      fre_tbl->alloced = number_of_entries;
    }
  else if (fre_tbl->count == fre_tbl->alloced)
    {
      fre_tbl_sz = (sizeof (sf_fre_tbl)
		    + ((fre_tbl->alloced + number_of_entries)
		       * sizeof (sframe_frame_row_entry)));
      fre_tbl = realloc (fre_tbl, fre_tbl_sz);
      if (fre_tbl == NULL)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;		/* OOM.  */
	}

      memset (&fre_tbl->entry[fre_tbl->alloced], 0,
	      number_of_entries * sizeof (sframe_frame_row_entry));
      fre_tbl->alloced += number_of_entries;
    }

  ectx_frep = &fre_tbl->entry[fre_tbl->count];
  ectx_frep->fre_start_addr
    = frep->fre_start_addr;
  ectx_frep->fre_info = frep->fre_info;

  if (fdep->sfde_func_size)
    sframe_assert (frep->fre_start_addr < fdep->sfde_func_size);
  else
    /* A SFrame FDE with func size equal to zero is possible.  */
    sframe_assert (frep->fre_start_addr == fdep->sfde_func_size);

  /* frep has already been sanity check'd.  Get offsets size.  */
  offsets_sz = sframe_fre_offset_bytes_size (frep->fre_info);
  memcpy (&ectx_frep->fre_offsets, &frep->fre_offsets, offsets_sz);

  esz = sframe_fre_entry_size (frep, fre_type);
  fre_tbl->count++;

  encoder->sfe_fres = (void *) fre_tbl;
  encoder->sfe_fre_nbytes += esz;

  ehp = sframe_encoder_get_header (encoder);
  ehp->sfh_num_fres = fre_tbl->count;

  /* Update the value of the number of FREs for the function.  */
  fdep->sfde_func_num_fres++;

  return 0;

bad:
  if (fre_tbl != NULL)
    free (fre_tbl);
  encoder->sfe_fres = NULL;
  encoder->sfe_fre_nbytes = 0;
  return -1;
}

/* Add a new function descriptor entry with START_ADDR, FUNC_SIZE and NUM_FRES
   to the encoder.  */

int
sframe_encoder_add_funcdesc (sframe_encoder_ctx *encoder,
			     int32_t start_addr,
			     uint32_t func_size,
			     unsigned char func_info,
			     uint32_t num_fres __attribute__ ((unused)))
{
  sframe_header *ehp;
  sf_funidx_tbl *fd_info;
  size_t fd_tbl_sz;
  int err = 0;

  /* FIXME book-keep num_fres for error checking.  */
  if (encoder == NULL)
    return sframe_set_errno (&err, SFRAME_ERR_INVAL);

  fd_info = (sf_funidx_tbl *) encoder->sfe_funcdesc;
  ehp = sframe_encoder_get_header (encoder);

  if (fd_info == NULL)
    {
      fd_tbl_sz = (sizeof (sf_funidx_tbl)
		   + (number_of_entries * sizeof (sframe_func_desc_entry)));
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
      fd_tbl_sz = (sizeof (sf_funidx_tbl)
		   + ((fd_info->alloced + number_of_entries)
		      * sizeof (sframe_func_desc_entry)));
      fd_info = realloc (fd_info, fd_tbl_sz);
      if (fd_info == NULL)
	{
	  sframe_set_errno (&err, SFRAME_ERR_NOMEM);
	  goto bad;		/* OOM.  */
	}

      memset (&fd_info->entry[fd_info->alloced], 0,
	      number_of_entries * sizeof (sframe_func_desc_entry));
      fd_info->alloced += number_of_entries;
    }

  fd_info->entry[fd_info->count].sfde_func_start_address = start_addr;
  /* Num FREs is updated as FREs are added for the function later via
     sframe_encoder_add_fre.  */
  fd_info->entry[fd_info->count].sfde_func_size = func_size;
  fd_info->entry[fd_info->count].sfde_func_start_fre_off
    = encoder->sfe_fre_nbytes;
#if 0
  // Linker optimization test code cleanup later ibhagat TODO FIXME
  unsigned int fre_type = sframe_calc_fre_type (func_size);

  fd_info->entry[fd_info->count].sfde_func_info
    = sframe_fde_func_info (fre_type);
#endif
  fd_info->entry[fd_info->count].sfde_func_info = func_info;
  fd_info->count++;
  encoder->sfe_funcdesc = (void *) fd_info;
  ehp->sfh_num_fdes++;
  return 0;

bad:
  if (fd_info != NULL)
    free (fd_info);
  encoder->sfe_funcdesc = NULL;
  ehp->sfh_num_fdes = 0;
  return -1;
}

static int
sframe_sort_funcdesc (sframe_encoder_ctx *encoder)
{
  sframe_header *ehp;

  ehp = sframe_encoder_get_header (encoder);
  /* Sort and write out the FDE table.  */
  sf_funidx_tbl *fd_info = (sf_funidx_tbl *) encoder->sfe_funcdesc;
  if (fd_info)
    {
      qsort (fd_info->entry, fd_info->count,
	     sizeof (sframe_func_desc_entry), fde_func);
      /* Update preamble's flags.  */
      ehp->sfh_preamble.sfp_flags |= SFRAME_F_FDE_SORTED;
    }
  return 0;
}

/* Write a frame row entry pointed to by FREP into the buffer CONTENTS.  The
   size in bytes written out are updated in ESZ.

   This function works closely with the SFrame binary format.

   Returns SFRAME_ERR if failure.  */

static int
sframe_encoder_write_fre (char *contents, sframe_frame_row_entry *frep,
			  unsigned int fre_type, size_t *esz)
{
  size_t fre_size;
  size_t fre_start_addr_sz;
  size_t fre_stack_offsets_sz;
  int err = 0;

  if (!sframe_fre_sanity_check_p (frep))
    return sframe_set_errno (&err, SFRAME_ERR_FRE_INVAL);

  fre_start_addr_sz = sframe_fre_start_addr_size (fre_type);
  fre_stack_offsets_sz = sframe_fre_offset_bytes_size (frep->fre_info);

  /* The FRE start address must be encodable in the available number of
     bytes.  */
  uint64_t bitmask = SFRAME_BITMASK_OF_SIZE (fre_start_addr_sz);
  sframe_assert ((uint64_t)frep->fre_start_addr <= bitmask);

  memcpy (contents,
	  &frep->fre_start_addr,
	  fre_start_addr_sz);
  contents += fre_start_addr_sz;

  memcpy (contents,
	  &frep->fre_info,
	  sizeof (frep->fre_info));
  contents += sizeof (frep->fre_info);

  memcpy (contents,
	  frep->fre_offsets,
	  fre_stack_offsets_sz);
  contents+= fre_stack_offsets_sz;

  fre_size = sframe_fre_entry_size (frep, fre_type);
  /* Sanity checking.  */
  sframe_assert ((fre_start_addr_sz
		     + sizeof (frep->fre_info)
		     + fre_stack_offsets_sz) == fre_size);

  *esz = fre_size;

  return 0;
}

/* Serialize the core contents of the SFrame section and write out to the
   output buffer held in the ENCODER.  Return SFRAME_ERR if failure.  */

static int
sframe_encoder_write_sframe (sframe_encoder_ctx *encoder)
{
  char *contents;
  size_t buf_size;
  size_t hdr_size;
  size_t all_fdes_size;
  size_t fre_size;
  size_t esz = 0;
  sframe_header *ehp;
  unsigned char flags;
  sf_funidx_tbl *fd_info;
  sf_fre_tbl *fr_info;
  uint32_t i, num_fdes;
  uint32_t j, num_fres;
  sframe_func_desc_entry *fdep;
  sframe_frame_row_entry *frep;

  unsigned int fre_type;
  int err = 0;

  contents = encoder->sfe_data;
  buf_size = encoder->sfe_data_size;
  num_fdes = sframe_encoder_get_num_fidx (encoder);
  all_fdes_size = num_fdes * sizeof (sframe_func_desc_entry);
  ehp = sframe_encoder_get_header (encoder);
  hdr_size = sframe_get_hdr_size (ehp);

  fd_info = (sf_funidx_tbl *) encoder->sfe_funcdesc;
  fr_info = (sf_fre_tbl *) encoder->sfe_fres;

  /* Sanity checks:
     - buffers must be malloc'd by the caller.  */
  if ((contents == NULL) || (buf_size < hdr_size))
    return sframe_set_errno (&err, SFRAME_ERR_BUF_INVAL);
  if (fr_info == NULL)
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
      num_fres = fdep->sfde_func_num_fres;

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
  sframe_assert ((size_t)(contents - encoder->sfe_data) == buf_size);

  /* Sort the FDE table */
  sframe_sort_funcdesc (encoder);

  /* Sanity checks:
     - the FDE section must have been sorted by now on the start address
     of each function.  */
  flags = ehp->sfh_preamble.sfp_flags;
  if (!(flags & SFRAME_F_FDE_SORTED)
      || (fd_info == NULL))
    return sframe_set_errno (&err, SFRAME_ERR_FDE_INVAL);

  contents = encoder->sfe_data;
  /* Write out the SFrame header.  The SFrame header in the encoder
     object has already been updated with correct offsets by the caller.  */
  memcpy (contents, ehp, hdr_size);
  contents += hdr_size;

  /* Write out the FDE table sorted on funtion start address.  */
  memcpy (contents, fd_info->entry, all_fdes_size);
  contents += all_fdes_size;

  return 0;
}

/* Serialize the contents of the encoder and return the buffer.  ENCODED_SIZE
   is updated to the size of the buffer.  */

char *
sframe_encoder_write (sframe_encoder_ctx *encoder,
		      size_t *encoded_size, int *errp)
{
  sframe_header *ehp;
  size_t hdrsize, fsz, fresz, bufsize;
  int foreign_endian;

  /* Initialize the encoded_size to zero.  This makes it simpler to just
     return from the function in case of failure.  Free'ing up of
     encoder->sfe_data is the responsibility of the caller.  */
  *encoded_size = 0;

  if (encoder == NULL || encoded_size == NULL || errp == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_INVAL);

  ehp = sframe_encoder_get_header (encoder);
  hdrsize = sframe_get_hdr_size (ehp);
  fsz = sframe_encoder_get_num_fidx (encoder)
    * sizeof (sframe_func_desc_entry);
  fresz = encoder->sfe_fre_nbytes;

  /* The total size of buffer is the sum of header, SFrame Function Descriptor
     Entries section and the FRE section.  */
  bufsize = hdrsize + fsz + fresz;
  encoder->sfe_data = (char *) malloc (bufsize);
  if (encoder->sfe_data == NULL)
    return sframe_ret_set_errno (errp, SFRAME_ERR_NOMEM);
  encoder->sfe_data_size = bufsize;

  /* Update the information in the SFrame header.  */
  /* SFrame FDE section follows immediately after the header.  */
  ehp->sfh_fdeoff = 0;
  /* SFrame FRE section follows immediately after the SFrame FDE section.  */
  ehp->sfh_freoff = fsz;
  ehp->sfh_fre_len = fresz;

  foreign_endian = need_swapping (ehp->sfh_abi_arch);

  /* Write out the FDE Index and the FRE table in the sfe_data. */
  if (sframe_encoder_write_sframe (encoder))
    return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);

  /* Endian flip the contents if necessary.  */
  if (foreign_endian)
    {
      if (flip_sframe (encoder->sfe_data, bufsize, 1))
	return sframe_ret_set_errno (errp, SFRAME_ERR_BUF_INVAL);
      flip_header ((sframe_header*)encoder->sfe_data);
    }

  *encoded_size = bufsize;
  return encoder->sfe_data;
}
