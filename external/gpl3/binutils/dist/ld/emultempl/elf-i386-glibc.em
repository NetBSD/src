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

# This file is sourced from elf.em, and defines i386 glibc specific
# routines.
#

source_em ${srcdir}/emultempl/elf-x86.em
source_em ${srcdir}/emultempl/elf-x86-glibc.em

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#

fragment <<EOF
static void
elf_i386_glibc_before_parse (void)
{
  elf_x86_before_parse ();
  elf_x86_glibc_before_parse ();
  params.gnu_tls_version_tag = DEFAULT_LD_GNU_TLS_TAG;
}
EOF

LDEMUL_BEFORE_PARSE=elf_i386_glibc_before_parse

PARSE_AND_LIST_LONGOPTS_386='
  { "gnu-tls-tag", no_argument, NULL, OPTION_GNU_TLS_VERSION_TAG },
  { "no-gnu-tls-tag", no_argument, NULL, OPTION_NO_GNU_TLS_VERSION_TAG },
'

PARSE_AND_LIST_OPTIONS_386='
  if (DEFAULT_LD_GNU_TLS_TAG == 0)
    fprintf (file, _("\
  --gnu-tls-tag               Add GLIBC_ABI_GNU_TLS dependency\n\
  --no-gnu-tls-tag            Do not add GLIBC_ABI_GNU_TLS dependency (default)\n"));
  else if (DEFAULT_LD_GNU_TLS_TAG == 1)
    fprintf (file, _("\
  --gnu-tls-tag               Add GLIBC_ABI_GNU_TLS dependency (default)\n\
  --no-gnu-tls-tag            Do not add GLIBC_ABI_GNU_TLS dependency\n"));
  else
    fprintf (file, _("\
  --gnu-tls-tag               Add GLIBC_ABI_GNU_TLS dependency (auto)\n\
                                when no options are specified (default)\n\
  --no-gnu-tls-tag            Do not add GLIBC_ABI_GNU_TLS dependency\n"));
'

PARSE_AND_LIST_ARGS_CASES_386='
    case OPTION_GNU_TLS_VERSION_TAG:
      params.gnu_tls_version_tag = 1;
      break;

    case OPTION_NO_GNU_TLS_VERSION_TAG:
      params.gnu_tls_version_tag = 0;
      break;
'

PARSE_AND_LIST_LONGOPTS="$PARSE_AND_LIST_LONGOPTS $PARSE_AND_LIST_LONGOPTS_386"
PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_386"
PARSE_AND_LIST_ARGS_CASES="$PARSE_AND_LIST_ARGS_CASES $PARSE_AND_LIST_ARGS_CASES_386"
