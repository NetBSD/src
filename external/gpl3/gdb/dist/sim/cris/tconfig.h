/* CRIS target configuration file.  -*- C -*-
   Copyright (C) 2004-2015 Free Software Foundation, Inc.
   Contributed by Axis Communications.

This file is part of the GNU simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef CRIS_TCONFIG_H
#define CRIS_TCONFIG_H

/* There's basically a a big ??? FIXME: CHECK THIS on everything in this
   file.  I just copied it from m32r, pruned some stuff and added
   HAVE_MODEL because it seemed useful.  */

/* See sim-hload.c.  We properly handle LMA.  */
#define SIM_HANDLES_LMA 1

/* For MSPR support.  FIXME: revisit.  */
#define WITH_DEVICES 1

#include "sim-module.h"
extern MODULE_INSTALL_FN cris_option_install;
#define MODULE_LIST cris_option_install,

#define SIM_HAVE_MODEL

/* This is a global setting.  Different cpu families can't mix-n-match -scache
   and -pbb.  However some cpu families may use -simple while others use
   one of -scache/-pbb.  */
#define WITH_SCACHE_PBB 1

#endif /* CRIS_TCONFIG_H */
