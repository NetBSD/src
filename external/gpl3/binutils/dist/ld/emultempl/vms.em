# This shell script emits a C file. -*- C -*-
#   Copyright 2010
#   Free Software Foundation, Inc.
#
# This file is part of the GNU Binutils.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.
#

# This file is sourced from generic.em.

fragment <<EOF
static void
gld${EMULATION_NAME}_before_parse (void)
{
  ldfile_set_output_arch ("${ARCH}", bfd_arch_`echo ${ARCH} | sed -e 's/:.*//'`);
  config.dynamic_link = TRUE;
  config.has_shared = FALSE; /* Not yet.  */
}

/* This is called before the input files are opened.  We add the
   standard library.  */

static void
gld${EMULATION_NAME}_create_output_section_statements (void)
{
  lang_add_input_file ("imagelib", lang_input_file_is_l_enum, NULL);
  lang_add_input_file ("starlet", lang_input_file_is_l_enum, NULL);
  lang_add_input_file ("sys\$public_vectors", lang_input_file_is_l_enum, NULL);
}

/* Try to open a dynamic archive.  This is where we know that VMS
   shared images (dynamic libraries) have an extension of .exe.  */

static bfd_boolean
gld${EMULATION_NAME}_open_dynamic_archive (const char *arch ATTRIBUTE_UNUSED,
                                           search_dirs_type *search,
                                           lang_input_statement_type *entry)
{
  char *string;

  if (! entry->maybe_archive)
    return FALSE;

  string = (char *) xmalloc (strlen (search->name)
			     + strlen (entry->filename)
			     + sizeof "/.exe");

  sprintf (string, "%s/%s.exe", search->name, entry->filename);

  if (! ldfile_try_open_bfd (string, entry))
    {
      free (string);
      return FALSE;
    }

  entry->filename = string;

  return TRUE;
}

static int
gld${EMULATION_NAME}_find_potential_libraries
  (char *name, lang_input_statement_type *entry)
{
  return ldfile_open_file_search (name, entry, "", ".olb");
}

/* Place an orphan section.  We use this to put random OVR sections.
   Much borrowed from elf32.em.  */

static lang_output_section_statement_type *
vms_place_orphan (asection *s,
		  const char *secname ATTRIBUTE_UNUSED,
		  int constraint ATTRIBUTE_UNUSED)
{
  static struct orphan_save hold_data =
    {
      "\$DATA\$",
      SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_DATA,
      0, 0, 0, 0
    };

  /* We have nothing to say for anything other than a final link or an excluded
     section.  */
  if (link_info.relocatable
      || (s->flags & (SEC_EXCLUDE | SEC_LOAD)) != SEC_LOAD)
    return NULL;

  /* FIXME: we should place sections by VMS program section flags.  */

  /* Only handle data sections.  */
  if ((s->flags & SEC_DATA) == 0)
    return NULL;

  if (hold_data.os == NULL)
    hold_data.os = lang_output_section_find (hold_data.name);

  if (hold_data.os != NULL)
    {
      lang_add_section (&hold_data.os->children, s, hold_data.os);
      return hold_data.os;
    }
  else
    return NULL;
}
EOF

LDEMUL_PLACE_ORPHAN=vms_place_orphan
LDEMUL_BEFORE_PARSE=gld"$EMULATION_NAME"_before_parse
LDEMUL_CREATE_OUTPUT_SECTION_STATEMENTS=gld"$EMULATION_NAME"_create_output_section_statements
LDEMUL_FIND_POTENTIAL_LIBRARIES=gld"$EMULATION_NAME"_find_potential_libraries
LDEMUL_OPEN_DYNAMIC_ARCHIVE=gld"$EMULATION_NAME"_open_dynamic_archive
