/* plt-findfre-2.c -- Test for sframe_find_fre for SFrame FDE of type
   PCMASK with with one SFrame FRE only.

   Copyright (C) 2025 Free Software Foundation, Inc.

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

/* s390x-specific size in bytes of plt0 and pltN.  */
#define PLT_SIZE 32

/* Magic values added to CFA offsets to make them distingishable.  Must
   be multiple of 8 due to s390x-specific CFA alignment factor.  */
#define PLT0_CFA_OFFSET_MAGIC 0
#define PLTN_CFA_OFFSET_MAGIC 8

static int
add_plt0_fde (sframe_encoder_ctx *ectx, uint32_t plt_vaddr,
	      uint32_t sframe_vaddr, int idx)
{
  /* 1 single FRE.  */
  sframe_frame_row_entry fre
    = { 0x0,
	{ SFRAME_V2_S390X_CFA_OFFSET_ENCODE (160 + PLT0_CFA_OFFSET_MAGIC) },
	SFRAME_V1_FRE_INFO (SFRAME_BASE_REG_SP, 1, SFRAME_FRE_OFFSET_1B) };

  unsigned char finfo = sframe_fde_create_func_info (SFRAME_FRE_TYPE_ADDR1,
						     SFRAME_FDE_TYPE_PCINC);
  uint32_t offsetof_fde_in_sec
    = sframe_encoder_get_offsetof_fde_start_addr (ectx, idx, NULL);

  int32_t func_start_addr = (plt_vaddr
			     - (sframe_vaddr + offsetof_fde_in_sec));

  /* 1 PCINC-type FDE for 1 plt0 entry of 32 bytes.  */
  int err = sframe_encoder_add_funcdesc_v2 (ectx, func_start_addr,
					    PLT_SIZE /* func size.  */,
					    finfo,
					    0 /* rep block size.  */,
					    1 /* num FREs.  */);
  if (err == -1)
    return err;

  if (sframe_encoder_add_fre (ectx, idx, &fre) == SFRAME_ERR)
    return -1;

  return 0;
}

static int
add_pltn_fde (sframe_encoder_ctx *ectx, uint32_t plt_vaddr,
	      uint32_t sframe_vaddr, int idx)
{
  /* 1 single FRE.  */
  sframe_frame_row_entry fre
    = { 0x0,
	{ SFRAME_V2_S390X_CFA_OFFSET_ENCODE (160 + PLTN_CFA_OFFSET_MAGIC) },
	SFRAME_V1_FRE_INFO (SFRAME_BASE_REG_SP, 1, SFRAME_FRE_OFFSET_1B) };

  unsigned char finfo = sframe_fde_create_func_info (SFRAME_FRE_TYPE_ADDR1,
						     SFRAME_FDE_TYPE_PCMASK);
  uint32_t offsetof_fde_in_sec
    = sframe_encoder_get_offsetof_fde_start_addr (ectx, idx, NULL);

  int32_t func_start_addr = (plt_vaddr
			     - (sframe_vaddr + offsetof_fde_in_sec));

  /* 1 PCMASK-type FDE for 5 pltN entries of 32 bytes each.  */
  int err = sframe_encoder_add_funcdesc_v2 (ectx, func_start_addr,
					    5 * PLT_SIZE /* func size.  */,
					    finfo,
					    PLT_SIZE /* rep block size.  */,
					    1 /* num FREs.  */);
  if (err == -1)
    return err;

  if (sframe_encoder_add_fre (ectx, idx, &fre) == SFRAME_ERR)
    return -1;

  return 0;
}

static
void test_plt_findfre (const char suffix, const uint32_t plt_vaddr,
		       const uint32_t sframe_vaddr)
{
  sframe_encoder_ctx *ectx;
  sframe_decoder_ctx *dctx;
  sframe_frame_row_entry frep;
  char *sframe_buf;
  size_t sf_size;
  int err = 0;
  unsigned int fde_cnt = 0;
  int i;

#define TEST(cond, ...)							\
  do									\
    {									\
      if (cond)								\
	pass (__VA_ARGS__);						\
      else								\
	fail (__VA_ARGS__);						\
    }									\
    while (0)

  ectx = sframe_encode (SFRAME_VERSION, SFRAME_F_FDE_FUNC_START_PCREL,
			SFRAME_ABI_S390X_ENDIAN_BIG,
			SFRAME_CFA_FIXED_FP_INVALID,
			SFRAME_CFA_FIXED_RA_INVALID,
			&err);
  TEST (ectx != NULL && err == 0, "plt-findfre-2%c: Creating SFrame encoder", suffix);

  err = add_plt0_fde (ectx, plt_vaddr, sframe_vaddr, 0);
  TEST (err == 0, "plt-findfre-2%c: Adding FDE for plt0", suffix);

  fde_cnt = sframe_encoder_get_num_fidx (ectx);
  TEST (fde_cnt == 1, "plt-findfre-2%c: Test FDE count after adding FDE for plt0", suffix);

  err = add_pltn_fde (ectx, plt_vaddr + PLT_SIZE, sframe_vaddr, 1);
  TEST (err == 0, "plt-findfre-2%c: Adding FDE for pltN", suffix);

  fde_cnt = sframe_encoder_get_num_fidx (ectx);
  TEST (fde_cnt == 2, "plt-findfre-2%c: Test FDE count after adding FDE for pltN", suffix);

  sframe_buf = sframe_encoder_write (ectx, &sf_size, &err);
  TEST (err == 0, "plt-findfre-2%c: Encoder write", suffix);

  dctx = sframe_decode (sframe_buf, sf_size, &err);
  TEST (dctx != NULL, "plt-findfre-2%c: Decoder setup", suffix);

  /* Find the only FRE in PLT0 at offset 0.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 0 - sframe_vaddr), &frep);
  TEST (err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 160 + PLT0_CFA_OFFSET_MAGIC,
	"plt-findfre-2%c: Find only FRE in PLT0 at offset 0", suffix);

  /* Find the only FRE in PLT0 at offset PLT_SIZE-1.  */
  err = sframe_find_fre (dctx, (plt_vaddr + (PLT_SIZE-1) - sframe_vaddr), &frep);
  TEST (err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 160 + PLT0_CFA_OFFSET_MAGIC,
	"plt-findfre-2%c: Find only FRE in PLT0 at offset PLT_SIZE-1", suffix);

  /* Find the only FRE in PLT1-5 at offset 0 and PLT_SIZE-1.  */
  for (i = 1; i < 5; i++)
    {
      /* Find the only FRE in PLTN at offset 0.  */
      err = sframe_find_fre (dctx, (plt_vaddr + i * PLT_SIZE + 0 - sframe_vaddr), &frep);
      TEST (err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 160 + PLTN_CFA_OFFSET_MAGIC,
	    "plt-findfre-2%c: Find only FRE in PLT%d at offset 0", suffix, i);

      /* Find the only FRE in PLTN at offset 31.  */
      err = sframe_find_fre (dctx, (plt_vaddr + i * PLT_SIZE + (PLT_SIZE-1) - sframe_vaddr), &frep);
      TEST (err == 0 && sframe_fre_get_cfa_offset (dctx, &frep, &err) == 160 + PLTN_CFA_OFFSET_MAGIC,
	    "plt-findfre-2%c: Find only FRE in PLT%d at offset PLT_SIZE-1", suffix, i);
    }

  /* Find no FRE in non-existing PLT6 at offset 0.  */
  err = sframe_find_fre (dctx, (plt_vaddr + 6 * PLT_SIZE + 0 - sframe_vaddr), &frep);
  TEST (err != 0, "plt-findfre-2%c: Find no FRE in out of range PLT6 at offset 0", suffix);

  sframe_encoder_free (&ectx);
  sframe_decoder_free (&dctx);
}

int
main (void)
{
  uint32_t sframe_vaddr = 0x402220;
  uint32_t plt_vaddr = 0x401020;
  printf ("plt-findfre-2a: Testing with plt_vaddr = %#x; sframe_vaddr = %#x\n",
	  plt_vaddr, sframe_vaddr);
  test_plt_findfre ('a', plt_vaddr, sframe_vaddr);

  sframe_vaddr = 0x401020;
  plt_vaddr = 0x402220;
  printf ("plt-findfre-2b: Testing with plt_vaddr = %#x; sframe_vaddr = %#x\n",
	  plt_vaddr, sframe_vaddr);
  test_plt_findfre ('b', plt_vaddr, sframe_vaddr);

  return 0;
}
