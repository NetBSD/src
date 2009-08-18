# This shell script emits a C file. -*- C -*-
#   Copyright 2006, 2007 Free Software Foundation, Inc.
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

# This file is sourced from generic.em
#
fragment <<EOF
#include "elf-bfd.h"

EOF
source_em ${srcdir}/emultempl/elf-generic.em
fragment <<EOF

static void
gld${EMULATION_NAME}_finish (void)
{
  gld${EMULATION_NAME}_map_segments (FALSE);
  finish_default ();
}
EOF
# Put these extra routines in ld_${EMULATION_NAME}_emulation
#
LDEMUL_FINISH=gld${EMULATION_NAME}_finish
