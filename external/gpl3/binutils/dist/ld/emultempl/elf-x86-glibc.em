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

# This file is sourced from elf.em, and defines x86 glibc specific
# routines.
#

fragment <<EOF
static void
elf_x86_glibc_before_parse (void)
{
  params.gnu2_tls_version_tag = DEFAULT_LD_GNU2_TLS_TAG;
}
EOF

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#

PARSE_AND_LIST_LONGOPTS_X86='
  { "gnu2-tls-tag", no_argument, NULL, OPTION_GNU2_TLS_VERSION_TAG },
  { "no-gnu2-tls-tag", no_argument, NULL, OPTION_NO_GNU2_TLS_VERSION_TAG },
'

PARSE_AND_LIST_OPTIONS_X86='
  if (DEFAULT_LD_GNU2_TLS_TAG == 0)
    fprintf (file, _("\
  --gnu2-tls-tag              Add GNU2_ABI_GNU2_TLS dependency\n\
  --no-gnu2-tls-tag           Do not add GNU2_ABI_GNU2_TLS dependency (default)\n"));
  else if (DEFAULT_LD_GNU2_TLS_TAG == 1)
    fprintf (file, _("\
  --gnu2-tls-tag              Add GNU2_ABI_GNU2_TLS dependency (default)\n\
  --no-gnu2-tls-tag           Do not add GNU2_ABI_GNU2_TLS dependency\n"));
  else
    fprintf (file, _("\
  --gnu2-tls-tag              Add GNU2_ABI_GNU2_TLS dependency (auto)\n\
                                when no options are specified (default)\n\
  --no-gnu2-tls-tag           Do not add GNU2_ABI_GNU2_TLS dependency\n"));
'

PARSE_AND_LIST_ARGS_CASES_X86='
    case OPTION_GNU2_TLS_VERSION_TAG:
      params.gnu2_tls_version_tag = 1;
      break;

    case OPTION_NO_GNU2_TLS_VERSION_TAG:
      params.gnu2_tls_version_tag = 0;
      break;
'

PARSE_AND_LIST_LONGOPTS="$PARSE_AND_LIST_LONGOPTS $PARSE_AND_LIST_LONGOPTS_X86"
PARSE_AND_LIST_OPTIONS="$PARSE_AND_LIST_OPTIONS $PARSE_AND_LIST_OPTIONS_X86"
PARSE_AND_LIST_ARGS_CASES="$PARSE_AND_LIST_ARGS_CASES $PARSE_AND_LIST_ARGS_CASES_X86"
