/* Common definitions.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include "config.h"
#ifdef GDBSERVER
#include "build-gnulib-gdbserver/config.h"
#else
#include "build-gnulib/config.h"
#endif

/* From:
    https://www.gnu.org/software/gnulib/manual/html_node/stdint_002eh.html

   "On some hosts that predate C++11, when using C++ one must define
   __STDC_CONSTANT_MACROS to make visible the definitions of constant
   macros such as INTMAX_C, and one must define __STDC_LIMIT_MACROS to
   make visible the definitions of limit macros such as INTMAX_MAX.".

   And:
    https://www.gnu.org/software/gnulib/manual/html_node/inttypes_002eh.html

   "On some hosts that predate C++11, when using C++ one must define
   __STDC_FORMAT_MACROS to make visible the declarations of format
   macros such as PRIdMAX."

   Must do this before including any system header, since other system
   headers may include stdint.h/inttypes.h.  */
#define __STDC_CONSTANT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#define __STDC_FORMAT_MACROS 1

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>	/* for strcasecmp and strncasecmp */
#endif
#include <errno.h>
#include <alloca.h>

#include "ansidecl.h"
/* This is defined by ansidecl.h, but we prefer gnulib's version.  On
   MinGW, gnulib might enable __USE_MINGW_ANSI_STDIO, which may or not
   require use of attribute gnu_printf instead of printf.  gnulib
   checks that at configure time.  Since _GL_ATTRIBUTE_FORMAT_PRINTF
   is compatible with ATTRIBUTE_PRINTF, simply use it.  */
#undef ATTRIBUTE_PRINTF
#define ATTRIBUTE_PRINTF _GL_ATTRIBUTE_FORMAT_PRINTF

#include "libiberty.h"
#include "pathmax.h"
#include "gdb/signals.h"
#include "gdb_locale.h"
#include "ptid.h"
#include "common-types.h"
#include "common-utils.h"
#include "gdb_assert.h"
#include "errors.h"
#include "print-utils.h"
#include "common-debug.h"
#include "cleanups.h"
#include "common-exceptions.h"

#define EXTERN_C extern "C"
#define EXTERN_C_PUSH extern "C" {
#define EXTERN_C_POP }

/* Pull in gdb::unique_xmalloc_ptr.  */
#include "common/gdb_unique_ptr.h"

#endif /* COMMON_DEFS_H */
