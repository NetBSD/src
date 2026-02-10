/* .sframe section processing.
   Copyright (C) 2022-2025 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "sframe-api.h"
#include "sframe-internal.h"

/* Return TRUE if the function has been marked for deletion during the linking
   process.  */

static bool
sframe_decoder_func_deleted_p (struct sframe_dec_info *sfd_info,
			       unsigned int func_idx)
{
  if (func_idx < sfd_info->sfd_fde_count)
    return sfd_info->sfd_func_bfdinfo[func_idx].func_deleted_p;

  return false;
}

/* Mark the function in the decoder info for deletion.  */

static void
sframe_decoder_mark_func_deleted (struct sframe_dec_info *sfd_info,
				  unsigned int func_idx)
{
  if (func_idx < sfd_info->sfd_fde_count)
    sfd_info->sfd_func_bfdinfo[func_idx].func_deleted_p = true;
}

/* Get the relocation offset from the decoder info for the given function.  */

static unsigned int
sframe_decoder_get_func_r_offset (struct sframe_dec_info *sfd_info,
				  unsigned int func_idx)
{
  BFD_ASSERT (func_idx < sfd_info->sfd_fde_count);
  unsigned int func_r_offset
    = sfd_info->sfd_func_bfdinfo[func_idx].func_r_offset;
  /* There must have been a reloc.  */
  BFD_ASSERT (func_r_offset);
  return func_r_offset;
}

/* Bookkeep the function relocation offset in the decoder info.  */

static void
sframe_decoder_set_func_r_offset (struct sframe_dec_info *sfd_info,
				  unsigned int func_idx,
				  unsigned int r_offset)
{
  if (func_idx < sfd_info->sfd_fde_count)
    sfd_info->sfd_func_bfdinfo[func_idx].func_r_offset = r_offset;
}

/* Get the relocation index in the elf_reloc_cookie for the function.  */

static unsigned int
sframe_decoder_get_func_reloc_index (struct sframe_dec_info *sfd_info,
				     unsigned int func_idx)
{
  BFD_ASSERT (func_idx < sfd_info->sfd_fde_count);
  return sfd_info->sfd_func_bfdinfo[func_idx].func_reloc_index;
}

/* Bookkeep the relocation index in the elf_reloc_cookie for the function.  */

static void
sframe_decoder_set_func_reloc_index (struct sframe_dec_info *sfd_info,
				     unsigned int func_idx,
				     unsigned int reloc_index)
{
  if (func_idx < sfd_info->sfd_fde_count)
    sfd_info->sfd_func_bfdinfo[func_idx].func_reloc_index = reloc_index;
}

/* Initialize the set of additional information in CFD_INFO,
   needed for linking SEC.  Returns TRUE if setup is done successfully.  */

static bool
sframe_decoder_init_func_bfdinfo (bfd *abfd,
				  const asection *sec,
				  struct sframe_dec_info *sfd_info,
				  const struct elf_reloc_cookie *cookie)
{
  unsigned int fde_count;
  unsigned int func_bfdinfo_size, i;
  const Elf_Internal_Rela *rel;

  fde_count = sframe_decoder_get_num_fidx (sfd_info->sfd_ctx);
  sfd_info->sfd_fde_count = fde_count;

  /* Allocate and clear the memory.  */
  func_bfdinfo_size = (sizeof (struct sframe_func_bfdinfo)) * fde_count;
  sfd_info->sfd_func_bfdinfo = bfd_zalloc (abfd, func_bfdinfo_size);
  if (sfd_info->sfd_func_bfdinfo == NULL)
    return false;

  /* For linker generated .sframe sections, we have no relocs.  Skip.  */
  if ((sec->flags & SEC_LINKER_CREATED) && cookie->rels == NULL)
    return true;

  rel = cookie->rels;
  for (i = 0; i < fde_count; i++)
    {
      /* Bookkeep the relocation offset and relocation index of each function
	 for later use.  */
      sframe_decoder_set_func_r_offset (sfd_info, i, rel->r_offset);
      sframe_decoder_set_func_reloc_index (sfd_info, i, i);

      rel++;
    }

  /* If there are more relocation entries, they must be R_*_NONE which
     may be generated from relocations against discarded sections by
     ld -r.  */
  for (; rel < cookie->relend; rel++)
   if (rel->r_info != 0)
     break;
  BFD_ASSERT (rel == cookie->relend);

  return true;
}

/* Read the value from CONTENTS at the specified OFFSET for the given ABFD.  */

static bfd_vma
sframe_read_value (bfd *abfd, bfd_byte *contents, unsigned int offset,
		   unsigned int width)
{
  BFD_ASSERT (contents && offset);
  /* Supporting the usecase of reading only the 4-byte relocated
     value (signed offset for func start addr) for now.  */
  BFD_ASSERT (width == 4);
  /* FIXME endianness ?? */
  unsigned char *buf = contents + offset;
  bfd_vma value = bfd_get_signed_32 (abfd, buf);
  return value;
}

/* Return true if there is at least one non-empty .sframe section in
   input files.  Can only be called after ld has mapped input to
   output sections, and before sections are stripped.  */

bool
_bfd_elf_sframe_present (struct bfd_link_info *info)
{
  asection *sframe = bfd_get_section_by_name (info->output_bfd, ".sframe");

  if (sframe == NULL)
    return false;

  /* Count only sections which have at least a single FDE.  */
  for (sframe = sframe->map_head.s; sframe != NULL; sframe = sframe->map_head.s)
    /* Note that this may become an approximate check in the future when
       some ABI/arch begin to use the sfh_auxhdr_len.  When sfh_auxhdr_len has
       non-zero value, it will need to be accounted for in the calculation of
       the SFrame header size.  */
    if (sframe->size > sizeof (sframe_header))
      return true;
  return false;
}

/* Try to parse .sframe section SEC, which belongs to ABFD.  Store the
   information in the section's sec_info field on success.  COOKIE
   describes the relocations in SEC.

   Returns TRUE if success, FALSE if any error or failure.  */

bool
_bfd_elf_parse_sframe (bfd *abfd,
		       struct bfd_link_info *info ATTRIBUTE_UNUSED,
		       asection *sec, struct elf_reloc_cookie *cookie)
{
  bfd_byte *sfbuf = NULL;
  struct sframe_dec_info *sfd_info;
  sframe_decoder_ctx *sfd_ctx;
  bfd_size_type sf_size;
  int decerr = 0;

  /* Prior versions of assembler and ld were generating SFrame sections with
     section type SHT_PROGBITS.  Issue an error for lack of support for such
     objects now.  Even if section size is zero, a valid section type is
     expected.  */
  if (elf_section_type (sec) != SHT_GNU_SFRAME)
    {
      _bfd_error_handler
	(_("error in %pB(%pA); unexpected SFrame section type"),
	 abfd, sec);
      return false;
    }

  if (sec->size == 0
      || (sec->flags & SEC_HAS_CONTENTS) == 0
      || sec->sec_info_type != SEC_INFO_TYPE_NONE)
    {
      /* This file does not contain .sframe information.  */
      return false;
    }

  if (bfd_is_abs_section (sec->output_section))
    {
      /* At least one of the sections is being discarded from the
	 link, so we should just ignore them.  */
      return false;
    }

  /* Read the SFrame stack trace information from abfd.  */
  if (!_bfd_elf_mmap_section_contents (abfd, sec, &sfbuf))
    goto fail_no_free;

  /* Decode the buffer and keep decoded contents for later use.
     Relocations are performed later, but are such that the section's
     size is unaffected.  */
  sfd_info = bfd_zalloc (abfd, sizeof (*sfd_info));
  sf_size = sec->size;

  sfd_info->sfd_ctx = sframe_decode ((const char*)sfbuf, sf_size, &decerr);
  sfd_info->sfd_state = SFRAME_SEC_DECODED;
  sfd_ctx = sfd_info->sfd_ctx;
  if (!sfd_ctx)
    /* Free'ing up any memory held by decoder context is done by
       sframe_decode in case of error.  */
    goto fail_no_free;

  if (!sframe_decoder_init_func_bfdinfo (abfd, sec, sfd_info, cookie))
    {
      sframe_decoder_free (&sfd_ctx);
      goto fail_no_free;
    }

  elf_section_data (sec)->sec_info = sfd_info;
  sec->sec_info_type = SEC_INFO_TYPE_SFRAME;

  goto success;

fail_no_free:
  _bfd_error_handler
   (_("error in %pB(%pA); no .sframe will be created"),
    abfd, sec);
  return false;
success:
  _bfd_elf_munmap_section_contents (sec, sfbuf);
  return true;
}

/* This function is called for each input file before the .sframe section
   is relocated.  It marks the SFrame FDE for the discarded functions for
   deletion.

   The function returns TRUE iff any entries have been deleted.  */

bool
_bfd_elf_discard_section_sframe
   (asection *sec,
    bool (*reloc_symbol_deleted_p) (bfd_vma, void *),
    struct elf_reloc_cookie *cookie)
{
  bool changed;
  bool keep;
  unsigned int i;
  unsigned int func_desc_offset;
  unsigned int num_fidx;
  struct sframe_dec_info *sfd_info;

  changed = false;
  /* FIXME - if relocatable link and changed = true, how does the final
     .rela.sframe get updated ?.  */
  keep = false;

  sfd_info = (struct sframe_dec_info *) elf_section_data (sec)->sec_info;

  /* Skip checking for the linker created .sframe sections
     (for PLT sections).  */
  if ((sec->flags & SEC_LINKER_CREATED) == 0 || cookie->rels != NULL)
    {
      num_fidx = sframe_decoder_get_num_fidx (sfd_info->sfd_ctx);
      for (i = 0; i < num_fidx; i++)
	{
	  func_desc_offset = sframe_decoder_get_func_r_offset (sfd_info, i);

	  cookie->rel = cookie->rels
	    + sframe_decoder_get_func_reloc_index (sfd_info, i);
	  keep = !(*reloc_symbol_deleted_p) (func_desc_offset, cookie);

	  if (!keep)
	    {
	      sframe_decoder_mark_func_deleted (sfd_info, i);
	      changed = true;
	    }
	}
    }
  return changed;
}

/* Update the reference to the output .sframe section in the output ELF
   BFD ABFD.  Returns true if no error.  */

bool
_bfd_elf_set_section_sframe (bfd *abfd, struct bfd_link_info *info)
{
  asection *cfsec;

  cfsec = bfd_get_section_by_name (info->output_bfd, ".sframe");
  if (!cfsec)
    return false;

  elf_section_type (cfsec) = SHT_GNU_SFRAME;
  elf_sframe (abfd) = cfsec;

  return true;
}

/* Merge .sframe section SEC.  This is called with the relocated
   CONTENTS.  */

bool
_bfd_elf_merge_section_sframe (bfd *abfd,
			       struct bfd_link_info *info,
			       asection *sec,
			       bfd_byte *contents)
{
  struct sframe_dec_info *sfd_info;
  struct sframe_enc_info *sfe_info;
  sframe_decoder_ctx *sfd_ctx;
  sframe_encoder_ctx *sfe_ctx;
  uint8_t sfd_ctx_abi_arch;
  int8_t sfd_ctx_fixed_fp_offset;
  int8_t sfd_ctx_fixed_ra_offset;
  uint8_t dctx_version;
  uint8_t ectx_version;
  uint8_t dctx_flags;
  uint8_t ectx_flags;
  int encerr = 0;

  struct elf_link_hash_table *htab;
  asection *cfsec;

  /* Sanity check - handle SFrame sections only.  */
  if (sec->sec_info_type != SEC_INFO_TYPE_SFRAME)
    return false;

  sfd_info = (struct sframe_dec_info *) elf_section_data (sec)->sec_info;
  sfd_ctx = sfd_info->sfd_ctx;

  htab = elf_hash_table (info);
  sfe_info = &(htab->sfe_info);
  sfe_ctx = sfe_info->sfe_ctx;

  /* All input bfds are expected to have a valid SFrame section.  Even if
     the SFrame section is empty with only a header, there must be a valid
     SFrame decoder context by now.  The SFrame encoder context, however,
     will get set later here, if this is the first call to the function.  */
  if (sfd_ctx == NULL || sfe_info == NULL)
    return false;

  dctx_flags = sframe_decoder_get_flags (sfd_ctx);

  if (htab->sfe_info.sfe_ctx == NULL)
    {
      sfd_ctx_abi_arch = sframe_decoder_get_abi_arch (sfd_ctx);
      sfd_ctx_fixed_fp_offset = sframe_decoder_get_fixed_fp_offset (sfd_ctx);
      sfd_ctx_fixed_ra_offset = sframe_decoder_get_fixed_ra_offset (sfd_ctx);

      /* Valid values are non-zero.  */
      if (!sfd_ctx_abi_arch)
	return false;

      /* In-memory FDEs in the encoder object are unsorted during linking and
	 will be sorted before emission.  Reset SFRAME_F_FDE_SORTED to aptly
	 reflect that (doing so has no other functional value at this time
	 though).  */
      uint8_t tflags = dctx_flags & ~SFRAME_F_FDE_SORTED;
      /* ld always generates an output section with
	 SFRAME_F_FDE_FUNC_START_PCREL flag set.  Later using
	 SFRAME_V2_GNU_AS_LD_ENCODING_FLAGS, it is enforced that the provided
	 input sections also have this flag set.  */
      tflags |= SFRAME_F_FDE_FUNC_START_PCREL;
      htab->sfe_info.sfe_ctx = sframe_encode (SFRAME_VERSION_2,
					      tflags, /* SFrame flags.  */
					      sfd_ctx_abi_arch,
					      sfd_ctx_fixed_fp_offset,
					      sfd_ctx_fixed_ra_offset,
					      &encerr);
      /* Handle errors from sframe_encode.  */
      if (htab->sfe_info.sfe_ctx == NULL)
	return false;
    }
  sfe_ctx = sfe_info->sfe_ctx;

  if (sfe_info->sframe_section == NULL)
    {
      /* Make sure things are set for an eventual write.
	 Size of the output section is not known until
	 _bfd_elf_write_section_sframe is ready with the buffer
	 to write out.  */
      cfsec = bfd_get_section_by_name (info->output_bfd, ".sframe");
      if (cfsec)
	{
	  sfe_info->sframe_section = cfsec;
	  // elf_sframe (abfd) = cfsec;
	}
      else
	return false;
    }

  /* Check that all .sframe sections being linked have the same
     ABI/arch.  */
  if (sframe_decoder_get_abi_arch (sfd_ctx)
      != sframe_encoder_get_abi_arch (sfe_ctx))
    {
      _bfd_error_handler
	(_("input SFrame sections with different abi prevent .sframe"
	  " generation"));
      return false;
    }

  /* Check that all .sframe sections being linked have the same version.  */
  dctx_version = sframe_decoder_get_version (sfd_ctx);
  ectx_version = sframe_encoder_get_version (sfe_ctx);
  if (dctx_version != SFRAME_VERSION_2 || dctx_version != ectx_version)
    {
      _bfd_error_handler
	(_("input SFrame sections with different format versions prevent"
	  " .sframe generation"));
      return false;
    }

  /* Check that all SFrame sections being linked have the 'data encoding'
     related flags set.  The implementation does not support updating these
     data encodings on the fly; confirm by checking the ectx_flags.  */
  ectx_flags = sframe_encoder_get_flags (sfe_ctx);
  if ((dctx_flags & ectx_flags & SFRAME_V2_GNU_AS_LD_ENCODING_FLAGS)
      != SFRAME_V2_GNU_AS_LD_ENCODING_FLAGS)
    {
      _bfd_error_handler
	(_("SFrame sections with unexpected data encoding prevent"
	  " .sframe generation"));
      return false;
    }

  /* Iterate over the function descriptor entries and the FREs of the
     function from the decoder context.  Add each of them to the encoder
     context, if suitable.  */
  uint32_t i = 0, j = 0, cur_fidx = 0;

  uint32_t num_fidx = sframe_decoder_get_num_fidx (sfd_ctx);
  uint32_t num_enc_fidx = sframe_encoder_get_num_fidx (sfe_ctx);

  for (i = 0; i < num_fidx; i++)
    {
      unsigned int num_fres = 0;
      int32_t func_start_addr;
      bfd_vma address;
      uint32_t func_size = 0;
      unsigned char func_info = 0;
      unsigned int r_offset = 0;
      bool pltn_reloc_by_hand = false;
      unsigned int pltn_r_offset = 0;
      uint8_t rep_block_size = 0;

      if (!sframe_decoder_get_funcdesc_v2 (sfd_ctx, i, &num_fres, &func_size,
					   &func_start_addr, &func_info,
					   &rep_block_size))
	{
	  /* If function belongs to a deleted section, skip editing the
	     function descriptor entry.  */
	  if (sframe_decoder_func_deleted_p(sfd_info, i))
	    continue;

	  /* Don't edit function descriptor entries for relocatable link.  */
	  if (!bfd_link_relocatable (info))
	    {
	      if (!(sec->flags & SEC_LINKER_CREATED))
		{
		  /* Get relocated contents by reading the value of the
		     relocated function start address at the beginning of the
		     function descriptor entry.  */
		  r_offset = sframe_decoder_get_func_r_offset (sfd_info, i);
		}
	      else
		{
		  /* Expected to land here when SFrame stack trace info is
		     created dynamically for the .plt* sections.  These
		     sections are expected to have upto two SFrame FDE entries.
		     Although the code should work for > 2,  leaving this
		     assert here for safety.  */
		  BFD_ASSERT (num_fidx <= 2);
		  /* For the first entry, we know the offset of the SFrame FDE's
		     sfde_func_start_address.  Side note: see how the value
		     of PLT_SFRAME_FDE_START_OFFSET is also set to the
		     same.  */
		  r_offset = sframe_decoder_get_hdr_size (sfd_ctx);
		  /* For any further SFrame FDEs, the generator has already put
		     in an offset in place of sfde_func_start_address of the
		     corresponding FDE.  We will use it by hand to relocate.  */
		  if (i > 0)
		    {
		      pltn_r_offset
			= r_offset + (i * sizeof (sframe_func_desc_entry));
		      pltn_reloc_by_hand = true;
		    }
		}

	      /* Get the SFrame FDE function start address after relocation.  */
	      address = sframe_read_value (abfd, contents, r_offset, 4);
	      if (pltn_reloc_by_hand)
		address += sframe_read_value (abfd, contents,
					      pltn_r_offset, 4);
	      address += (sec->output_offset + r_offset);
	      /* SFrame FDE function start address is an offset from the
		 sfde_func_start_address field to the start PC.  The
		 calculation below is the distance of sfde_func_start_address
		 field from the start of the output SFrame section.  */
	      uint32_t offsetof_fde_in_sec
		= sframe_encoder_get_offsetof_fde_start_addr (sfe_ctx,
							      cur_fidx + num_enc_fidx,
							      NULL);
	      address -= offsetof_fde_in_sec;

	      /* FIXME For testing only. Cleanup later.  */
	      // address += (sec->output_section->vma);

	      func_start_addr = address;
	    }

	  /* Update the encoder context with updated content.  */
	  int err = sframe_encoder_add_funcdesc_v2 (sfe_ctx, func_start_addr,
						    func_size, func_info,
						    rep_block_size, num_fres);
	  cur_fidx++;
	  BFD_ASSERT (!err);
	}

      for (j = 0; j < num_fres; j++)
	{
	  sframe_frame_row_entry fre;
	  if (!sframe_decoder_get_fre (sfd_ctx, i, j, &fre))
	    {
	      int err = sframe_encoder_add_fre (sfe_ctx,
						cur_fidx-1+num_enc_fidx,
						&fre);
	      BFD_ASSERT (!err);
	    }
	}
    }
  sfd_info->sfd_state = SFRAME_SEC_MERGED;
  /* Free the SFrame decoder context.  */
  sframe_decoder_free (&sfd_ctx);

  return true;
}

/* Adjust an address in the .sframe section.  Given OFFSET within
   SEC, this returns the new offset in the merged .sframe section,
   or -1 if the address refers to an FDE which has been removed.

   PS: This function assumes that _bfd_elf_merge_section_sframe has
   not been called on the input section SEC yet.  Note how it uses
   sframe_encoder_get_num_fidx () to figure out the offset of FDE
   in the output section.  */

bfd_vma
_bfd_elf_sframe_section_offset (bfd *output_bfd ATTRIBUTE_UNUSED,
				struct bfd_link_info *info,
				asection *sec,
				bfd_vma offset)
{
  struct sframe_dec_info *sfd_info;
  struct sframe_enc_info *sfe_info;
  sframe_decoder_ctx *sfd_ctx;
  sframe_encoder_ctx *sfe_ctx;
  struct elf_link_hash_table *htab;

  unsigned int sec_fde_idx, out_num_fdes;
  unsigned int sfd_num_fdes, sfe_num_fdes;
  uint32_t sfd_fde_offset;
  bfd_vma new_offset;

  if (sec->sec_info_type != SEC_INFO_TYPE_SFRAME)
    return offset;

  sfd_info = elf_section_data (sec)->sec_info;
  sfd_ctx = sfd_info->sfd_ctx;
  sfd_num_fdes = sframe_decoder_get_num_fidx (sfd_ctx);

  BFD_ASSERT (sfd_info->sfd_state == SFRAME_SEC_DECODED);

  htab = elf_hash_table (info);
  sfe_info = &(htab->sfe_info);
  sfe_ctx = sfe_info->sfe_ctx;
  sfe_num_fdes = sframe_encoder_get_num_fidx (sfe_ctx);

  /* The index of this FDE in the output section depends on number of deleted
     functions (between index 0 and sec_fde_idx), if any.  */
  out_num_fdes = 0;
  sec_fde_idx = 0;
  for (unsigned int i = 0; i < sfd_num_fdes; i++)
    {
      sfd_fde_offset = sframe_decoder_get_offsetof_fde_start_addr (sfd_ctx,
								   i, NULL);
      if (!sframe_decoder_func_deleted_p (sfd_info, i))
	out_num_fdes++;

      if (sfd_fde_offset == offset)
	{
	  /* Found the index of the FDE (at OFFSET) in the input section.  */
	  sec_fde_idx = i;
	  break;
	}
    }

  if (sframe_decoder_func_deleted_p (sfd_info, sec_fde_idx))
    return (bfd_vma) -1;

  /* The number of FDEs in the output SFrame section.  Note that the output
     index of the FDE of interest will be (out_num_fdes - 1).  */
  out_num_fdes += sfe_num_fdes;

  new_offset = sframe_decoder_get_offsetof_fde_start_addr (sfd_ctx,
							   out_num_fdes - 1,
							   NULL);
  /* Recall that SFrame section merging has distinct requirements: All SFrame
     FDEs from input sections are clubbed together in the beginning of the
     output section.  So, at this point in the current function, the new_offset
     is the correct offset in the merged output SFrame section.  Note, however,
     that the default mechanism in the _elf_link_input_bfd will do the
     following adjustment:
       irela->r_offset += o->output_offset;
     for all section types.  However, such an adjustment in the RELA offset is
     _not_ needed for SFrame sections.  Perform the reverse adjustment here so
     that the default mechanism does not need additional SFrame specific
     checks.  */
  new_offset -= sec->output_offset;

  return new_offset;
}

/* Write out the .sframe section.  This must be called after
   _bfd_elf_merge_section_sframe has been called on all input
   .sframe sections.  */

bool
_bfd_elf_write_section_sframe (bfd *abfd, struct bfd_link_info *info)
{
  bool retval = true;

  struct elf_link_hash_table *htab;
  struct sframe_enc_info *sfe_info;
  sframe_encoder_ctx *sfe_ctx;
  asection *sec;
  void *contents;
  size_t sec_size;
  int err = 0;

  htab = elf_hash_table (info);
  sfe_info = &htab->sfe_info;
  sec = sfe_info->sframe_section;
  sfe_ctx = sfe_info->sfe_ctx;

  if (sec == NULL)
    return true;

  contents = sframe_encoder_write (sfe_ctx, &sec_size, &err);
  sec->size = (bfd_size_type) sec_size;

  if (!bfd_set_section_contents (abfd, sec->output_section, contents,
				 (file_ptr) sec->output_offset,
				 sec->size))
    retval = false;
  else
    {
      Elf_Internal_Shdr *hdr = &elf_section_data (sec)->this_hdr;
      hdr->sh_size = sec->size;
    }

  sframe_encoder_free (&sfe_ctx);

  return retval;
}
