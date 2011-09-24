# This shell script emits a C file. -*- C -*-
#   Copyright 2009  Free Software Foundation, Inc.
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

# This file is sourced from elf32.em, and defines extra rx-elf
# specific routines.
#
test -z "$TARGET2_TYPE" && TARGET2_TYPE="rel"
fragment <<EOF

static bfd_boolean no_flag_mismatch_warnings = FALSE;

/* This is a convenient point to tell BFD about target specific flags.
   After the output has been created, but before inputs are read.  */
static void
rx_elf_create_output_section_statements (void)
{
  extern void bfd_elf32_rx_set_target_flags (bfd_boolean);

  bfd_elf32_rx_set_target_flags (no_flag_mismatch_warnings);
}

EOF

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#
PARSE_AND_LIST_PROLOGUE='
#define OPTION_NO_FLAG_MISMATCH_WARNINGS	301
'

PARSE_AND_LIST_LONGOPTS='
  { "no-flag-mismatch-warnings", no_argument, NULL, OPTION_NO_FLAG_MISMATCH_WARNINGS},
'

PARSE_AND_LIST_OPTIONS='
  fprintf (file, _("  --no-flag-mismatch-warnings Don'\''t warn about objects with incompatible"
		   "                                endian or dsp settings\n"));
'

PARSE_AND_LIST_ARGS_CASES='
    case OPTION_NO_FLAG_MISMATCH_WARNINGS:
      no_flag_mismatch_warnings = TRUE;
      break;
'

LDEMUL_CREATE_OUTPUT_SECTION_STATEMENTS=rx_elf_create_output_section_statements
