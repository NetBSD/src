# This shell script emits C code -*- C -*-
# to keep track of the machine type of Z80 object files
# It does some substitutions.
#   Copyright 2005, 2007, 2008 Free Software Foundation, Inc.
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

LDEMUL_BEFORE_PARSE=gldz80_before_parse
LDEMUL_RECOGNIZED_FILE=gldz80_recognized_file
LDEMUL_AFTER_OPEN=gldz80_after_open

fragment <<EOF
/* --- \begin{z80.em} */
/* Codes for machine types, bitwise or gives the code to use for the
   output.  */
#define M_Z80STRICT 1
#define M_Z80 3
#define M_Z80FULL 7
#define M_R800 11
#define M_Z80ANY 15

/* Bitwise or of the machine types seen so far.  */
static int result_mach_type;

static void
${LDEMUL_BEFORE_PARSE} (void)
{
#ifndef TARGET_			/* I.e., if not generic.  */
  ldfile_set_output_arch ("`echo ${ARCH}`", bfd_arch_unknown);
#endif /* not TARGET_ */
  result_mach_type = M_Z80STRICT;
}


/* Update result_mach_type.  */
static bfd_boolean
${LDEMUL_RECOGNIZED_FILE} (lang_input_statement_type *entry)
{
  unsigned long mach_type;

  mach_type = bfd_get_mach (entry->the_bfd);
  switch (mach_type)
    {
    case bfd_mach_z80strict:
      result_mach_type |= M_Z80STRICT;
      break;
    case bfd_mach_z80:
      result_mach_type |= M_Z80;
      break;
    case bfd_mach_z80full:
      result_mach_type |= M_Z80FULL;
      break;
    case bfd_mach_r800:
      result_mach_type |= M_R800;
      break;
    default:
      result_mach_type |= M_Z80ANY;
    }
  return FALSE;
}

/* Set the machine type of the output file based on result_mach_type.  */
static void
gldz80_after_open (void)
{
  unsigned long mach_type;

  after_open_default ();

  switch (result_mach_type)
    {
    case M_Z80STRICT:
      mach_type = bfd_mach_z80strict;
      break;
    case M_Z80:
      mach_type = bfd_mach_z80;
      break;
    case M_Z80FULL:
      mach_type = bfd_mach_z80full;
      break;
    case M_R800:
      mach_type = bfd_mach_r800;
      break;
    default:
      mach_type = 0;
    }
  bfd_set_arch_mach (link_info.output_bfd, bfd_arch_z80, mach_type);
}
/* --- \end{z80.em} */
EOF
