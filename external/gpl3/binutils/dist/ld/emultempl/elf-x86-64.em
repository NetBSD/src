# This shell script emits a C file. -*- C -*-
#   Copyright (C) 2025-2026 Free Software Foundation, Inc.
#
# This file is part of the GNU Binutils.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the license, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING3. If not,
# see <http://www.gnu.org/licenses/>.
#

# This file is sourced from elf.em, and defines x86-64 specific routines.
#

source_em ${srcdir}/emultempl/elf-x86.em

fragment <<EOF
static void
elf_x86_64_before_parse (void)
{
  params.mark_plt = DEFAULT_LD_Z_MARK_PLT;

  elf_x86_before_parse ();
}

static void
elf_x86_64_before_allocation (void)
{
  if (!bfd_link_relocatable (&link_info)
      && is_elf_hash_table (link_info.hash)
      && expld.phase != lang_mark_phase_enum)
    {
      struct elf_link_hash_table *htab = elf_hash_table (&link_info);
      /* Run one_lang_size_sections_pass to estimate the output section
	 layout before sizing dynamic sections.  */
      expld.dataseg.phase = exp_seg_none;
      expld.phase = lang_mark_phase_enum;
      /* NB: Exclude linker created GOT setions when estimating output
	 section layout as sizing dynamic sections may change linker
	 created GOT sections.  */
      if (htab->sgot != NULL)
	htab->sgot->flags |= SEC_EXCLUDE;
      if (htab->sgotplt != NULL)
	htab->sgotplt->flags |= SEC_EXCLUDE;
      one_lang_size_sections_pass (NULL, false);
      /* Restore linker created GOT setions.  */
      if (htab->sgot != NULL)
	htab->sgot->flags &= ~SEC_EXCLUDE;
      if (htab->sgotplt != NULL)
	htab->sgotplt->flags &= ~SEC_EXCLUDE;
      lang_reset_memory_regions ();
    }

  gld${EMULATION_NAME}_before_allocation ();
}
EOF

LDEMUL_BEFORE_PARSE=elf_x86_64_before_parse
LDEMUL_BEFORE_ALLOCATION=elf_x86_64_before_allocation
