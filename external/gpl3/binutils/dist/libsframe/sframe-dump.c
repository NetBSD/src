/* sframe-dump.c - Textual dump of .sframe.

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sframe-impl.h"

typedef struct
{
  unsigned int reg_num;
  const char *reg_name;
} sframe_reg_map_entry;

typedef struct
{
  const sframe_reg_map_entry *reg_map;
  size_t map_size;
} sframe_abi_reg_map;

/* Register number - Register name pair.
   Stack pointer (sp) and Frame pointer (fp) pairs.  */
#define SFRAME_SP(num) { num, "sp" }
#define SFRAME_FP(num) { num, "fp" }

#define SFRAME_ABI_REG_MAP(abi_str, ...) \
  const sframe_reg_map_entry abi_str##_reg_map_entries[] = { __VA_ARGS__ }; \
  const sframe_abi_reg_map abi_str##_sframe_abi_reg_map = { \
      abi_str##_reg_map_entries, \
      (sizeof (abi_str##_reg_map_entries) \
       / sizeof (abi_str##_reg_map_entries[0])) \
  };

/* Create a map for each supported arch specifying DWARF register numbers for
   stack pointer and frame pointer.  */
SFRAME_ABI_REG_MAP (amd64, SFRAME_SP (7), SFRAME_FP (6));
SFRAME_ABI_REG_MAP (aarch64, SFRAME_SP (31), SFRAME_FP (29));
SFRAME_ABI_REG_MAP (s390x, SFRAME_SP (15), SFRAME_FP (11));

static const char *
sframe_get_reg_name (uint8_t abi_arch, unsigned int reg_num, char *buf,
		     size_t buf_size)
{
  const char *reg_name = NULL;
  const sframe_abi_reg_map *abi_reg_map = NULL;

  switch (abi_arch)
    {
      case SFRAME_ABI_AARCH64_ENDIAN_BIG:
      case SFRAME_ABI_AARCH64_ENDIAN_LITTLE:
	abi_reg_map = &aarch64_sframe_abi_reg_map;
	break;
      case SFRAME_ABI_AMD64_ENDIAN_LITTLE:
	abi_reg_map = &amd64_sframe_abi_reg_map;
	break;
      case SFRAME_ABI_S390X_ENDIAN_BIG:
	abi_reg_map = &s390x_sframe_abi_reg_map;
	break;
      default:
	sframe_assert (false);
	break;
    }

  if (abi_reg_map->reg_map[0].reg_num == reg_num)
    reg_name = abi_reg_map->reg_map[0].reg_name;
  else if (abi_reg_map->reg_map[1].reg_num == reg_num)
    reg_name = abi_reg_map->reg_map[1].reg_name;

  /* Handle fallback if name is missing or reg num is non-SP/FP.  */
  if (reg_name == NULL)
    {
      snprintf (buf, buf_size, "r%u", reg_num);
      reg_name = buf;
    }

  return reg_name;
}

/* Return TRUE if the SFrame section is associated with the aarch64 ABIs.  */

static bool
is_sframe_abi_arch_aarch64 (const sframe_decoder_ctx *sfd_ctx)
{
  bool aarch64_p = false;

  uint8_t abi_arch = sframe_decoder_get_abi_arch (sfd_ctx);
  if (abi_arch == SFRAME_ABI_AARCH64_ENDIAN_BIG
      || abi_arch == SFRAME_ABI_AARCH64_ENDIAN_LITTLE)
    aarch64_p = true;

  return aarch64_p;
}

/* Return TRUE if the SFrame section is associated with the s390x ABI.  */

static bool
is_sframe_abi_arch_s390x (const sframe_decoder_ctx *sfd_ctx)
{
  return sframe_decoder_get_abi_arch (sfd_ctx) == SFRAME_ABI_S390X_ENDIAN_BIG;
}

static bool
sframe_s390x_offset_regnum_p (int32_t offset, uint8_t ver)
{
  if (ver == SFRAME_VERSION_2)
    return SFRAME_V2_S390X_OFFSET_IS_REGNUM (offset);
  else if (ver == SFRAME_VERSION_3)
    return false;
  else
    /* No other version is supported yet.  */
    sframe_assert (false);
}

static int
sframe_s390x_offset_decode_regnum (int32_t offset, uint8_t ver)
{
  if (ver == SFRAME_VERSION_2)
    return SFRAME_V2_S390X_OFFSET_DECODE_REGNUM (offset);
  else
    /* No other version is supported yet.  */
    sframe_assert (false);
}

static void
dump_sframe_header_flags (const sframe_decoder_ctx *sfd_ctx)
{
  const char *prefix = "Flags: ";

  uint8_t ver = sframe_decoder_get_version (sfd_ctx);
  uint8_t flags = sframe_decoder_get_flags (sfd_ctx);

  if (!flags)
    {
      printf ("%11sNONE\n", prefix);
      return;
    }

#define PRINT_FLAG(x) \
  if (flags & (x)) \
    { flags = (flags & ~(x)); \
      printf ("%11s%s%s\n", prefix, #x, flags ? "," : ""); \
      prefix = " "; \
    }

  PRINT_FLAG (SFRAME_F_FDE_SORTED);
  /* SFRAME_F_FRAME_POINTER has been removed from SFrame V3 and beyond.  */
  if (ver == SFRAME_VERSION_2)
    PRINT_FLAG (SFRAME_F_FRAME_POINTER);
  PRINT_FLAG (SFRAME_F_FDE_FUNC_START_PCREL);
#undef PRINT_FLAG

  /* Print any residual flags, should this implementation be out of sync when
     new flags are added.  */
  if (flags)
    printf ("%11s%d\n", prefix, flags);
}

static void
dump_sframe_header (const sframe_decoder_ctx *sfd_ctx)
{
  uint8_t ver;
  const char *ver_str = NULL;
  int8_t cfa_fixed_fp_offset;
  int8_t cfa_fixed_ra_offset;
  const sframe_header *header = &(sfd_ctx->sfd_header);

  /* Prepare SFrame section version string.  */
  const char *version_names[]
    = { "NULL",
	"SFRAME_VERSION_1",
	"SFRAME_VERSION_2",
	"SFRAME_VERSION_3" };

  ver = sframe_decoder_get_version (sfd_ctx);
  if (ver <= SFRAME_VERSION)
    ver_str = version_names[ver];

  /* CFA fixed FP and RA offsets.  */
  cfa_fixed_fp_offset = header->sfh_cfa_fixed_fp_offset;
  cfa_fixed_ra_offset = header->sfh_cfa_fixed_ra_offset;

  const char* subsec_name = "Header";
  printf ("\n");
  printf ("  %s :\n", subsec_name);
  printf ("\n");
  printf ("    Version: %s\n", ver_str);

  dump_sframe_header_flags (sfd_ctx);

  if (cfa_fixed_fp_offset != SFRAME_CFA_FIXED_FP_INVALID)
    printf ("    CFA fixed FP offset: %d\n", cfa_fixed_fp_offset);
  if (cfa_fixed_ra_offset != SFRAME_CFA_FIXED_RA_INVALID)
    printf ("    CFA fixed RA offset: %d\n", cfa_fixed_ra_offset);
  printf ("    Num FDEs: %d\n", sframe_decoder_get_num_fidx (sfd_ctx));
  printf ("    Num FREs: %d\n", header->sfh_num_fres);
}

static void
dump_sframe_func_fres_simple (const sframe_decoder_ctx *sfd_ctx,
			      unsigned int funcidx,
			      uint32_t num_fres,
			      int64_t func_start_pc_vma,
			      bool pc_mask_p)
{
  uint32_t j = 0;
  const char *base_reg_str[] = {"fp", "sp"};
  bool ra_undefined_p = false;
  int32_t cfa_offset = 0;
  int32_t fp_offset = 0;
  int32_t ra_offset = 0;
  uint8_t base_reg_id = 0;
  int err[3] = {0, 0, 0};

  sframe_frame_row_entry fre;

  uint8_t ver = sframe_decoder_get_version (sfd_ctx);
  char temp[100] = {0};
  for (j = 0; j < num_fres; j++)
    {
      sframe_decoder_get_fre (sfd_ctx, funcidx, j, &fre);

      int64_t fre_start_pc_vma = (pc_mask_p
				  ? fre.fre_start_addr
				  : func_start_pc_vma + fre.fre_start_addr);

      /* FIXME - fixup the err caching in array.
	 assert no error for base reg id and RA undefined.  */
      base_reg_id = sframe_fre_get_base_reg_id (&fre, &err[0]);
      ra_undefined_p = sframe_fre_get_ra_undefined_p (sfd_ctx, &fre, &err[0]);
      cfa_offset = sframe_fre_get_cfa_offset (sfd_ctx, &fre,
					      SFRAME_FDE_TYPE_DEFAULT,
					      &err[0]);
      fp_offset = sframe_fre_get_fp_offset (sfd_ctx, &fre,
					    SFRAME_FDE_TYPE_DEFAULT, &err[1]);
      ra_offset = sframe_fre_get_ra_offset (sfd_ctx, &fre,
					    SFRAME_FDE_TYPE_DEFAULT, &err[2]);

      /* Dump VMA.  */
      printf ("\n");
      printf ("    %016"PRIx64, fre_start_pc_vma);

      /* Dump RA undefined (FRE without any offsets).  */
      if (ra_undefined_p)
	{
	  printf ("  RA undefined");
	  continue;
	}

      /* Dump CFA info.  */
      sprintf (temp, "%s+%d", base_reg_str[base_reg_id], cfa_offset);
      printf ("  %-10s", temp);

      /* Dump SP/FP info.  */
      if (err[1] == 0)
	{
	  if (is_sframe_abi_arch_s390x (sfd_ctx)
	      && sframe_s390x_offset_regnum_p (fp_offset, ver))
	    sprintf (temp, "r%d",
		     sframe_s390x_offset_decode_regnum (fp_offset, ver));
	  else
	    sprintf (temp, "c%+d", fp_offset);
	}
      else
	strcpy (temp, "u");
      printf ("%-10s", temp);

      /* Dump RA info.
	 If an ABI does not track RA offset, e.g., AMD64, display 'f',
	 else display the offset d as 'c+-d'.  */
      if (sframe_decoder_get_fixed_ra_offset (sfd_ctx)
	  != SFRAME_CFA_FIXED_RA_INVALID)
	strcpy (temp, "f");
      /* If an ABI does track RA offset, e.g. s390x, it can be a padding
	 to represent FP without RA being saved on stack.  */
      else if (err[2] == 0 && ra_offset == SFRAME_FRE_RA_OFFSET_INVALID)
	sprintf (temp, "U");
      else if (err[2] == 0)
	{
	  if (is_sframe_abi_arch_s390x (sfd_ctx)
	      && sframe_s390x_offset_regnum_p (ra_offset, ver))
	    sprintf (temp, "r%d",
		     sframe_s390x_offset_decode_regnum (ra_offset, ver));
	  else
	    sprintf (temp, "c%+d", ra_offset);
	}
      else
	strcpy (temp, "u");

      /* Mark SFrame FRE's RA information with "[s]" if the RA is mangled
	 with signature bits.  */
      const char *ra_mangled_p_str
	= ((sframe_fre_get_ra_mangled_p (sfd_ctx, &fre, &err[2]))
	   ? "[s]" : "   ");
      strcat (temp, ra_mangled_p_str);
      printf ("%-13s", temp);
    }
}

/* Helper to safely format "reg+offset" or "(reg+offset)". */

static void
sframe_format_fre_disp (char *buf, size_t size, uint8_t abi_arch,
			unsigned int reg_num, bool reg_p, int32_t offset,
			bool deref_p)
{
  /* Initialize to string for CFA-based.  */
  const char *reg_name = "c";

  /* Allocate space for the potential fallback name (e.g., "r12") */
  char temp_reg_name[32] = {0};
  if (reg_p)
    reg_name = sframe_get_reg_name (abi_arch, reg_num, temp_reg_name,
				    sizeof (temp_reg_name));

  if (deref_p)
    snprintf (buf, size, "(%s%+d)", reg_name, offset);
  else
    snprintf (buf, size, "%s%+d", reg_name, offset);
}

static void
dump_sframe_func_fres_flex (const sframe_decoder_ctx *sfd_ctx,
			    unsigned int funcidx,
			    uint32_t num_fres,
			    int64_t func_start_pc_vma,
			    bool pc_mask_p)
{
  uint32_t j = 0;
  bool ra_undefined_p = false;
  int64_t fre_start_pc_vma = 0;
  uint32_t fde_type = SFRAME_FDE_TYPE_FLEX;

  sframe_frame_row_entry fre;
  char temp[100] = {0};

  for (j = 0; j < num_fres; j++)
    {
      sframe_decoder_get_fre (sfd_ctx, funcidx, j, &fre);

      fre_start_pc_vma = (pc_mask_p
			  ? fre.fre_start_addr
			  : func_start_pc_vma + fre.fre_start_addr);

      /* Dump VMA.  */
      printf ("\n");
      printf ("    %016"PRIx64, fre_start_pc_vma);

      int err_ra_offset = 0;
      /* Dump RA undefined (FRE without any offsets).  */
      ra_undefined_p = sframe_fre_get_ra_undefined_p (sfd_ctx, &fre,
						      &err_ra_offset);
      sframe_assert (!err_ra_offset);
      if (ra_undefined_p)
	{
	  printf ("  RA undefined");
	  continue;
	}

      unsigned int cfa_reg = 0, ra_reg = 0, fp_reg = 0;
      bool cfa_deref_p = 0, ra_deref_p = 0, fp_deref_p = 0;

      int err_cfa_reg = 0;
      int err_cfa_offset = 0;
      /* Read the Register/Control Data as unsigned.  */
      uint32_t cfa_reg_data
	= sframe_get_fre_udata (&fre, SFRAME_FRE_CFA_OFFSET_IDX,
				&err_cfa_reg);
      int32_t cfa_offset = sframe_fre_get_cfa_offset (sfd_ctx, &fre, fde_type,
						      &err_cfa_offset);
      sframe_assert (!err_cfa_reg && !err_cfa_offset);
      bool cfa_reg_p = SFRAME_V3_FLEX_FDE_CTRLWORD_REG_P (cfa_reg_data);
      if (cfa_reg_p)
	{
	  cfa_reg = SFRAME_V3_FLEX_FDE_CTRLWORD_REGNUM (cfa_reg_data);
	  cfa_deref_p = SFRAME_V3_FLEX_FDE_CTRLWORD_DEREF_P (cfa_reg_data);
	}

      int err_ra_reg = 0;
      /* Read the Register/Control Data as unsigned.  */
      uint32_t ra_reg_data
	= sframe_get_fre_udata (&fre, SFRAME_FRE_RA_OFFSET_IDX * 2,
				&err_ra_reg);
      int32_t ra_offset = sframe_fre_get_ra_offset (sfd_ctx, &fre, fde_type,
						    &err_ra_offset);
      bool ra_reg_p = SFRAME_V3_FLEX_FDE_CTRLWORD_REG_P (ra_reg_data);
      if (ra_reg_p)
	{
	  ra_reg = SFRAME_V3_FLEX_FDE_CTRLWORD_REGNUM (ra_reg_data);
	  ra_deref_p = SFRAME_V3_FLEX_FDE_CTRLWORD_DEREF_P (ra_reg_data);
	}

      int err_fp_reg = 0;
      int err_fp_offset = 0;
      int fp_idx = SFRAME_FRE_FP_OFFSET_IDX * 2;
      if (!err_ra_reg && ra_reg_data == SFRAME_FRE_RA_OFFSET_INVALID)
       fp_idx -= 1;

      /* Read the Register/Control Data as unsigned.  */
      uint32_t fp_reg_data = sframe_get_fre_udata (&fre, fp_idx, &err_fp_reg);
      int32_t fp_offset = sframe_fre_get_fp_offset (sfd_ctx, &fre, fde_type,
						    &err_fp_offset);
      bool fp_reg_p = SFRAME_V3_FLEX_FDE_CTRLWORD_REG_P (fp_reg_data);
      if (fp_reg_p)
	{
	  fp_reg = SFRAME_V3_FLEX_FDE_CTRLWORD_REGNUM (fp_reg_data);
	  fp_deref_p = SFRAME_V3_FLEX_FDE_CTRLWORD_DEREF_P (fp_reg_data);
	}

      /* Dump CFA info.  */
      uint8_t abi_arch = sframe_decoder_get_abi_arch (sfd_ctx);
      sframe_format_fre_disp (temp, sizeof (temp), abi_arch, cfa_reg,
			      cfa_reg_p, cfa_offset, cfa_deref_p);
      printf ("  %-10s", temp);

      /* Dump FP info.  */
      if (!err_fp_reg && !err_fp_offset)
	sframe_format_fre_disp (temp, sizeof (temp), abi_arch, fp_reg,
				fp_reg_p, fp_offset, fp_deref_p);
      else
	strcpy (temp, "u");
      printf ("%-10s", temp);

      /* Dump RA info.
	 Even if an ABI does not track RA offset, e.g., AMD64, for flex
	 frame, it may have RA recovery from register.  Else, display 'f'.  */
      if (err_ra_reg)
	{
	  if (sframe_decoder_get_fixed_ra_offset (sfd_ctx)
	      != SFRAME_CFA_FIXED_RA_INVALID)
	    strcpy (temp, "f");
	  else
	    strcpy (temp, "u");
	}
      else if (ra_reg_data == SFRAME_FRE_RA_OFFSET_INVALID)
	strcpy (temp, "U");
      else
	sframe_format_fre_disp (temp, sizeof (temp), abi_arch, ra_reg,
				ra_reg_p, ra_offset, ra_deref_p);

      /* Mark SFrame FRE's RA information with "[s]" if the RA is mangled
	 with signature bits.  */
      err_ra_offset = 0;
      const char *ra_mangled_p_str
	= ((sframe_fre_get_ra_mangled_p (sfd_ctx, &fre, &err_ra_offset))
	   ? "[s]" : "   ");
      sframe_assert (!err_ra_offset);
      strcat (temp, ra_mangled_p_str);
      printf ("%-13s", temp);
    }
}

static void
dump_sframe_func_with_fres (const sframe_decoder_ctx *sfd_ctx,
			    unsigned int funcidx,
			    uint64_t sec_addr)
{
  uint32_t num_fres = 0;
  uint32_t func_size = 0;
  uint64_t func_start_pc_vma = 0;
  unsigned char func_info = 0;
  unsigned char func_info2 = 0;
  uint8_t rep_block_size = 0;

  uint8_t ver = sframe_decoder_get_version (sfd_ctx);
  sframe_assert (ver == SFRAME_VERSION_3 || ver == SFRAME_VERSION_2);
  /* Get the SFrame function descriptor - all data including the index and
     attributes.  */
  if (ver == SFRAME_VERSION_3)
    {
      int64_t func_start_addr = 0;
      sframe_decoder_get_funcdesc_v3 (sfd_ctx, funcidx, &num_fres, &func_size,
				      &func_start_addr, &func_info,
				      &func_info2, &rep_block_size);
      func_start_pc_vma = func_start_addr + sec_addr;
    }
  else
    {
      int32_t func_start_addr = 0;
      sframe_decoder_get_funcdesc_v2 (sfd_ctx, funcidx, &num_fres, &func_size,
				      &func_start_addr, &func_info,
				      &rep_block_size);
      func_start_pc_vma = func_start_addr + sec_addr;
    }

  /* Calculate the virtual memory address for function start pc.  Some older
     SFrame V2 sections in ET_DYN or ET_EXEC may still have the
     SFRAME_F_FDE_FUNC_START_PCREL flag unset, and hence may be using the old
     encoding.  Continue to support dumping the sections at least.  */
  if (sframe_decoder_get_flags (sfd_ctx) & SFRAME_F_FDE_FUNC_START_PCREL)
    func_start_pc_vma += sframe_decoder_get_offsetof_fde_start_addr (sfd_ctx,
								     funcidx,
								     NULL);

  /* Gather all FDE attributes.  Use a single snprintf to:
     - Include 'S', if fde_signal_p is true
     - Include 'F', if flex_p is true.  */
  char attrs[16] = {0};
  bool fde_signal_p = SFRAME_V3_FDE_SIGNAL_P (func_info);
  bool flex_p = (SFRAME_V3_FDE_TYPE (func_info2) == SFRAME_FDE_TYPE_FLEX);
  snprintf (attrs, sizeof (attrs), "%s%s",
	    fde_signal_p ? "S" : "",
	    flex_p ? "F" : "");

  printf ("\n    func idx [%d]: pc = 0x%"PRIx64 ", size = %d bytes",
	  funcidx,
	  func_start_pc_vma,
	  func_size);
  /* Print attributes if they exist.  */
  if (attrs[0])
    printf (", attr = \"%s\"", attrs);

  if (is_sframe_abi_arch_aarch64 (sfd_ctx)
      && (SFRAME_V2_FUNC_PAUTH_KEY (func_info) == SFRAME_AARCH64_PAUTH_KEY_B))
    printf (", pauth = B key");

  /* Mark FDEs with [m] where the FRE start address is interpreted as a
     mask.  */
  bool pc_mask_p
    = (SFRAME_V2_FUNC_PC_TYPE (func_info) == SFRAME_V3_FDE_PCTYPE_MASK);
  const char *pc_mask_marker = (pc_mask_p ? "[m]" : "   ");
  printf ("\n    %-7s%-8s %-10s%-10s%-13s",
	  "STARTPC", pc_mask_marker, "CFA", "FP", "RA");

  if (!flex_p)
    dump_sframe_func_fres_simple (sfd_ctx, funcidx, num_fres,
				  func_start_pc_vma, pc_mask_p);
  else
    dump_sframe_func_fres_flex (sfd_ctx, funcidx, num_fres, func_start_pc_vma,
				pc_mask_p);
}

static void
dump_sframe_functions (const sframe_decoder_ctx *sfd_ctx, uint64_t sec_addr)
{
  uint32_t i;
  uint32_t num_fdes;

  const char* subsec_name = "Function Index";
  printf ("\n  %s :\n", subsec_name);

  num_fdes = sframe_decoder_get_num_fidx (sfd_ctx);
  for (i = 0; i < num_fdes; i++)
    {
      dump_sframe_func_with_fres (sfd_ctx, i, sec_addr);
      printf ("\n");
    }
}

void
dump_sframe (const sframe_decoder_ctx *sfd_ctx, uint64_t sec_addr)
{
  dump_sframe_header (sfd_ctx);

  uint8_t ver = sframe_decoder_get_version (sfd_ctx);
  /* Although newer gas and ld do not generate SFrame V2, continue to support
     textual dump of SFrame V2 sections ATM.  */
  if (ver == SFRAME_VERSION_3 || ver == SFRAME_VERSION_2)
    dump_sframe_functions (sfd_ctx, sec_addr);
  else
    printf ("\n No further information can be displayed.  %s",
	    "SFrame version not supported\n");
}
