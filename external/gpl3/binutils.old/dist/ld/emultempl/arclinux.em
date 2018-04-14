# This shell script emits a C file. -*- C -*-
# Copyright (C) 2007-2016 Free Software Foundation, Inc.
#
# Copyright 2008-2012 Synopsys Inc.
#
# This file is part of GLD, the Gnu Linker.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
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

# This file is sourced from elf32.em, and defines extra arc-linux
# specific routines.
#
cat >>e${EMULATION_NAME}.c <<EOF
extern char * init_str;
extern char * fini_str;
EOF

PARSE_AND_LIST_PROLOGUE='
#define OPTION_INIT 300+1
#define OPTION_FINI (OPTION_INIT+1)
'
PARSE_AND_LIST_LONGOPTS='
  /* PE options */
  { "init", required_argument, NULL, OPTION_INIT },
  { "fini", required_argument, NULL, OPTION_FINI },
'

# FIXME: Should set PARSE_AND_LIST_OPTIONS to provide a short description
# of the options.

PARSE_AND_LIST_ARGS_CASES='
    case OPTION_FINI:
      fini_str = optarg;
      break;

    case OPTION_INIT:
      init_str = optarg;
      break;
'
