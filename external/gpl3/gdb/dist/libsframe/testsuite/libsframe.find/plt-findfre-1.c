/* plt-findfre-1.c -- Test for sframe_find_fre for SFRAME_FDE_TYPE_PCMASK.

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
add_plt_fde1 (sframe_encoder_ctx *ectx, uint32_t plt_vaddr,
	      uint32_t sframe_vaddr, int idx)
{
  /* A contiguous block containing 3 FREs.  The start_ip_offset must remain
     less than 16 bytes.  */
#define PLT1_NUM_FRES    3
  sframe_frame_row_entry fres[PLT1_NUM_FRES]
    = { {0x0, {0x1, 0, 0}, 0x3},
	{0x6, {0x2, 0xf0, 0}, 0x5},
	{0xc, {0x3, 0xf0, 0}, 0x4}
      };

  unsigned char finfo = sframe_fde_create_func_info (SFRAME_FRE_TYPE_ADDR1,
						     SFRAME_FDE_TYPE_PCMASK);
  uint32_t offsetof_fde_in_sec
    = sframe_encoder_get_offsetof_fde_start_addr (ectx, idx, NULL);
  int32_t func_start_addr = (plt_vaddr
			     - (sframe_vaddr + offsetof_fde_in_sec));

  /* 5 pltN entries of 16 bytes each.  */
  int err = sframe_encoder_add_funcdesc_v2 (ectx, func_start_addr,
					    16 * 5 /* func size in bytes.  */,
					    finfo,
					    16 /* rep block size in bytes.  */,
					    PLT1_NUM_FRES);
  if (err == -1)
    return err;

  for (unsigned i = 0; i < PLT1_NUM_FRES; i++)
    if (sframe_encoder_add_fre (ectx, idx, fres + i) == SFRAME_ERR)
      return -1;

  return 0;
}

static
void test_plt_findfre (uint32_t plt_vaddr, uint32_t sframe_vaddr)
{
  sframe_encoder_ctx *ectx;
  sframe_decoder_ctx *dctx;
  sframe_frame_row_entry frep;
  char *sframe_buf;
  size_t sf_size;
  int err = 0;
  uint32_t fde_cnt = 0;

#define TEST(name, cond)                                                      \
  do                                                                          \
    {                                                                         \
      if (cond)                                                               \
	pass (name);                                                          \
      else                                                                    \
	fail (name);                                                          \
    }                                                                         \
    while (0)

  ectx = sframe_encode (SFRAME_VERSION, SFRAME_F_FDE_FUNC_START_PCREL,
			SFRAME_ABI_AMD64_ENDIAN_LITTLE,
			SFRAME_CFA_FIXED_FP_INVALID,
			-8, /* Fixed RA offset for AMD64.  */
			&err);

  err = add_plt_fde1 (ectx, plt_vaddr, sframe_vaddr, 0);
  TEST ("plt-findfre-1: Adding FDE1 for plt", err == 0);

  fde_cnt = sframe_encoder_get_num_fidx (ectx);
  TEST ("plt-findfre-1: Test FDE count", fde_cnt == 1);

  sframe_buf = sframe_encoder_write (ectx, &sf_size, &err);
  TEST ("plt-findfre-1: Encoder write", err == 0);

  dctx = sframe_decode (sframe_buf, sf_size, &err);
  TEST ("plt-findfre-1: Decoder setup", dctx != NULL);

  /* Find the first FRE in PLT1.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 0x0 - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find first FRE in PLT1",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x1));

  /* Find the second FRE.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 0x6 - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find second FRE in PLT1",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x2));

  /* Find the last FRE.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 0xc - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find last FRE in PLT1",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x3));

  /* Find the first FRE in PLT4.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 16*3 + 0x0 - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find first FRE in PLT4",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x1));

  /* Find the second FRE in PLT4.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 16*3 + 0x6 - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find second FRE in PLT4",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x2));

  /* Find the last FRE in PLT4.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 16*3 + 0xc - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find last FRE in PLT4",
	(err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 0x3));

  /* Find no FRE for out of range PLT6.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 16*5 + 0x0 - sframe_vaddr), &frep);
  TEST ("plt-findfre-1: Find no FRE for out of range PLT6", err != 0);

  sframe_encoder_free (&ectx);
  sframe_decoder_free (&dctx);
}

int main (void)
{
  uint32_t sframe_vaddr = 0x402220;
  uint32_t plt_vaddr = 0x401020;
  printf ("Testing with plt_vaddr = %#x; sframe_vaddr = %#x\n", plt_vaddr,
	  sframe_vaddr);
  test_plt_findfre (plt_vaddr, sframe_vaddr);

  sframe_vaddr = 0x401020;
  plt_vaddr = 0x402220;
  printf ("Testing with plt_vaddr = %#x; sframe_vaddr = %#x\n", plt_vaddr,
	  sframe_vaddr);
  test_plt_findfre (plt_vaddr, sframe_vaddr);
}
