/* Emulation code used by all ELF targets.
   Copyright (C) 1991-2020 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

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
#include "bfdlink.h"
#include "ctf-api.h"
#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldlang.h"
#include "elf-bfd.h"
#include "ldelfgen.h"

void
ldelf_map_segments (bfd_boolean need_layout)
{
  int tries = 10;

  do
    {
      lang_relax_sections (need_layout);
      need_layout = FALSE;

      if (link_info.output_bfd->xvec->flavour == bfd_target_elf_flavour
	  && !bfd_link_relocatable (&link_info))
	{
	  bfd_size_type phdr_size;

	  phdr_size = elf_program_header_size (link_info.output_bfd);
	  /* If we don't have user supplied phdrs, throw away any
	     previous linker generated program headers.  */
	  if (lang_phdr_list == NULL)
	    elf_seg_map (link_info.output_bfd) = NULL;
	  if (!_bfd_elf_map_sections_to_segments (link_info.output_bfd,
						  &link_info))
	    einfo (_("%F%P: map sections to segments failed: %E\n"));

	  if (phdr_size != elf_program_header_size (link_info.output_bfd))
	    {
	      if (tries > 6)
		/* The first few times we allow any change to
		   phdr_size .  */
		need_layout = TRUE;
	      else if (phdr_size
		       < elf_program_header_size (link_info.output_bfd))
		/* After that we only allow the size to grow.  */
		need_layout = TRUE;
	      else
		elf_program_header_size (link_info.output_bfd) = phdr_size;
	    }
	}
    }
  while (need_layout && --tries);

  if (tries == 0)
    einfo (_("%F%P: looping in map_segments"));
}

/* We want to emit CTF early if and only if we are not targetting ELF with this
   invocation.  */

int
ldelf_emit_ctf_early (void)
{
  if (bfd_get_flavour (link_info.output_bfd) == bfd_target_elf_flavour)
    return 0;
  return 1;
}

/* Callbacks used to map from bfd types to libctf types, under libctf's
   control.  */

struct ctf_strsym_iter_cb_arg
{
  struct elf_sym_strtab *syms;
  bfd_size_type symcount;
  struct elf_strtab_hash *symstrtab;
  size_t next_i;
  size_t next_idx;
};

/* Return strings from the strtab to libctf, one by one.  Returns NULL when
   iteration is complete.  */

static const char *
ldelf_ctf_strtab_iter_cb (uint32_t *offset, void *arg_)
{
  bfd_size_type off;
  const char *ret;

  struct ctf_strsym_iter_cb_arg *arg =
    (struct ctf_strsym_iter_cb_arg *) arg_;

  /* There is no zeroth string.  */
  if (arg->next_i == 0)
    arg->next_i = 1;

  if (arg->next_i >= _bfd_elf_strtab_len (arg->symstrtab))
    {
      arg->next_i = 0;
      return NULL;
    }

  ret = _bfd_elf_strtab_str (arg->symstrtab, arg->next_i++, &off);
  *offset = off;

  /* If we've overflowed, we cannot share any further strings: the CTF
     format cannot encode strings with such high offsets.  */
  if (*offset != off)
    return NULL;

  return ret;
}

/* Return symbols from the symbol table to libctf, one by one.  We assume (and
   assert) that the symbols in the elf_link_hash_table are in strictly ascending
   order, and that none will be added in between existing ones.  Returns NULL
   when iteration is complete.  */

static struct ctf_link_sym *
ldelf_ctf_symbols_iter_cb (struct ctf_link_sym *dest,
					   void *arg_)
{
  struct ctf_strsym_iter_cb_arg *arg =
    (struct ctf_strsym_iter_cb_arg *) arg_;

  if (arg->next_i > arg->symcount)
    {
      arg->next_i = 0;
      arg->next_idx = 0;
      return NULL;
    }

  ASSERT (arg->syms[arg->next_i].dest_index == arg->next_idx);
  dest->st_name = _bfd_elf_strtab_str (arg->symstrtab, arg->next_i, NULL);
  dest->st_shndx = arg->syms[arg->next_i].sym.st_shndx;
  dest->st_type = ELF_ST_TYPE (arg->syms[arg->next_i].sym.st_info);
  dest->st_value = arg->syms[arg->next_i].sym.st_value;
  arg->next_i++;
  return dest;
}

void
ldelf_examine_strtab_for_ctf
  (struct ctf_file *ctf_output, struct elf_sym_strtab *syms,
   bfd_size_type symcount, struct elf_strtab_hash *symstrtab)
{
  struct ctf_strsym_iter_cb_arg args = { syms, symcount, symstrtab,
					  0, 0 };
   if (!ctf_output)
     return;

   if (bfd_get_flavour (link_info.output_bfd) == bfd_target_elf_flavour
       && !bfd_link_relocatable (&link_info))
    {
      if (ctf_link_add_strtab (ctf_output, ldelf_ctf_strtab_iter_cb,
			       &args) < 0)
	einfo (_("%F%P: warning: CTF strtab association failed; strings will "
		 "not be shared: %s\n"),
	       ctf_errmsg (ctf_errno (ctf_output)));

      if (ctf_link_shuffle_syms (ctf_output, ldelf_ctf_symbols_iter_cb,
				 &args) < 0)
	einfo (_("%F%P: warning: CTF symbol shuffling failed; slight space "
		 "cost: %s\n"), ctf_errmsg (ctf_errno (ctf_output)));
    }
}
