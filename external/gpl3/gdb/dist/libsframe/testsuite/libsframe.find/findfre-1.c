/* findfre-1.c -- Test for sframe_find_fre in libsframe.

   Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sframe-api.h"

/* DejaGnu should not use gnulib's vsnprintf replacement here.  */
#undef vsnprintf
#include <dejagnu.h>

static int
add_fde1 (sframe_encoder_ctx *encode, uint32_t start_pc_vaddr,
	  uint32_t sframe_vaddr, int idx, uint32_t *func_size)
{
  /* A contiguous block containing 4 FREs.  */
#define FDE1_NUM_FRES    4
  sframe_frame_row_entry fres[FDE1_NUM_FRES]
    = { {0x0, {0x1, 0, 0}, 0x3},
	{0x1, {0x2, 0xf0, 0}, 0x5},
	{0x10, {0x3, 0xf0, 0}, 0x4},
	{0x38, {0x8, 0xf0, 0}, 0x5}
      };
 /* Function size in bytes.  P.S. Must be a value greater than the
    fre_start_addr of the last FRE above (0x38).  */
  *func_size = 0x40;

  uint32_t offsetof_fde_in_sec
    = sframe_encoder_get_offsetof_fde_start_addr (encode, idx, NULL);
  int32_t func1_start_addr = (start_pc_vaddr
			      - (sframe_vaddr + offsetof_fde_in_sec));
  unsigned char finfo = sframe_fde_create_func_info (SFRAME_FRE_TYPE_ADDR1,
						     SFRAME_FDE_TYPE_PCINC);
  int err = sframe_encoder_add_funcdesc_v2 (encode, func1_start_addr,
					    *func_size, finfo, 0,
					    FDE1_NUM_FRES);
  if (err == -1)
    return err;

  for (unsigned int i = 0; i < FDE1_NUM_FRES; i++)
    if (sframe_encoder_add_fre (encode, idx, fres + i) == SFRAME_ERR)
      return -1;

  return 0;
}

static int
add_fde2 (sframe_encoder_ctx *encode, uint32_t start_pc_vaddr,
	  uint32_t sframe_vaddr, int idx, uint32_t *func_size)
{
  /* A contiguous block containing 4 FREs.  */
#define FDE2_NUM_FRES    4
  sframe_frame_row_entry fres[FDE2_NUM_FRES]
    = { {0x0, {0x10, 0, 0}, 0x3},
	{0x10, {0x12, 0xf0, 0}, 0x5},
	{0x14, {0x14, 0xf0, 0}, 0x4},
	{0x20, {0x15, 0xf0, 0}, 0x5}
      };
 /* Function size in bytes.  P.S. Must be a value greater than the
    fre_start_addr of the last FRE above (0x20).  */
  *func_size = 0x60;

  uint32_t offsetof_fde_in_sec
    = sframe_encoder_get_offsetof_fde_start_addr (encode, idx, NULL);
  int32_t func2_start_addr = (start_pc_vaddr
			      - (sframe_vaddr + offsetof_fde_in_sec));
  unsigned char finfo = sframe_fde_create_func_info (SFRAME_FRE_TYPE_ADDR1,
						     SFRAME_FDE_TYPE_PCINC);
  int err = sframe_encoder_add_funcdesc_v2 (encode, func2_start_addr,
					    *func_size, finfo, 0,
					    FDE2_NUM_FRES);
  if (err == -1)
    return err;

  for (unsigned int i = 0; i < FDE2_NUM_FRES; i++)
    if (sframe_encoder_add_fre (encode, idx, fres + i) == SFRAME_ERR)
      return -1;

  return 0;
}

static
void test_text_findfre (uint32_t text_vaddr, uint32_t sframe_vaddr)
{
  sframe_encoder_ctx *encode;
  sframe_decoder_ctx *dctx;
  sframe_frame_row_entry frep;
  uint32_t func1_start_vaddr;
  uint32_t func2_start_vaddr;
  uint32_t func1_size = 0;
  uint32_t func2_size = 0;
  uint32_t fde_cnt = 0;
  int32_t lookup_pc;
  char *sframe_buf;
  size_t sf_size;
  int err = 0;

#define TEST(name, cond)                                                      \
  do                                                                          \
    {                                                                         \
      if (cond)                                                               \
	pass (name);                                                          \
      else                                                                    \
	fail (name);                                                          \
    }                                                                         \
    while (0)

  encode = sframe_encode (SFRAME_VERSION,
			  SFRAME_F_FDE_FUNC_START_PCREL,
			  SFRAME_ABI_AMD64_ENDIAN_LITTLE,
			  SFRAME_CFA_FIXED_FP_INVALID,
			  -8, /* Fixed RA offset for AMD64.  */
			  &err);

  func1_start_vaddr = text_vaddr;
  err = add_fde1 (encode, func1_start_vaddr, sframe_vaddr, 0, &func1_size);
  TEST ("findfre-1: Adding FDE1", err == 0);

  /* Function 2 is placed after 0x10 bytes from the end of Function 1.  */
  func2_start_vaddr = func1_start_vaddr + func1_size + 0x10;
  err = add_fde2 (encode, func2_start_vaddr, sframe_vaddr, 1, &func2_size);
  TEST ("findfre-1: Adding FDE2", err == 0);

  fde_cnt = sframe_encoder_get_num_fidx (encode);
  TEST ("findfre-1: Test FDE count", fde_cnt == 2);

  sframe_buf = sframe_encoder_write (encode, &sf_size, &err);
  TEST ("findfre-1: Encoder write", err == 0);

  dctx = sframe_decode (sframe_buf, sf_size, &err);
  TEST ("findfre-1: Decoder setup", dctx != NULL);

  /* Find the third FRE in first FDE.  */
  lookup_pc = func1_start_vaddr + 0x15 - sframe_vaddr;
  err = sframe_find_fre (dctx, lookup_pc, &frep);
  TEST ("findfre-1: Find third FRE",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x3));

  /* Find an FRE for PC at the end of range covered by FRE.  */
  lookup_pc = func1_start_vaddr + 0x9 - sframe_vaddr;
  err = sframe_find_fre (dctx, lookup_pc, &frep);
  TEST ("findfre-1: Find FRE for last PC covered by FRE",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x2));

  /* Find the last FRE in first FDE.  */
  lookup_pc = func1_start_vaddr + 0x39 - sframe_vaddr;
  err = sframe_find_fre (dctx, lookup_pc, &frep);
  TEST ("findfre-1: Find last FRE",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x8));

  /* Find the second FRE in second FDE.  */
  lookup_pc = func2_start_vaddr + 0x11 - sframe_vaddr;
  err = sframe_find_fre (dctx, lookup_pc, &frep);
  TEST ("findfre-1: Find second FRE",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x12));

  /* Find the first FRE in second FDE.  */
  lookup_pc = func2_start_vaddr + 0x0 - sframe_vaddr;
  err = sframe_find_fre (dctx, lookup_pc, &frep);
  TEST ("findfre-1: Find first FRE",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x10));

  /* Find FRE for PC out of range.  Expect error code.  */
  lookup_pc = func1_start_vaddr + func1_size - sframe_vaddr;
  err = sframe_find_fre (dctx, lookup_pc, &frep);
  TEST ("findfre-1: Find FRE for out of range PC", err == SFRAME_ERR);

  sframe_encoder_free (&encode);
  sframe_decoder_free (&dctx);
}

int main (void)
{
  uint32_t sframe_vaddr = 0x402220;
  uint32_t text_vaddr = 0x401020;
  printf ("Testing with text_vaddr = %#x; sframe_vaddr = %#x\n", text_vaddr,
	  sframe_vaddr);
  test_text_findfre (text_vaddr, sframe_vaddr);

  sframe_vaddr = 0x401020;
  text_vaddr = 0x402220;
  printf ("Testing with text_vaddr = %#x; sframe_vaddr = %#x\n", text_vaddr,
	  sframe_vaddr);
  test_text_findfre (text_vaddr, sframe_vaddr);
}
