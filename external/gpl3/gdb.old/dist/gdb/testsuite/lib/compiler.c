/* This test file is part of GDB, the GNU debugger.

   Copyright 1995-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   */

/* Sometimes the behavior of a test depends upon the compiler used to
   compile the test program.  A test script can call get_compiler_info
   to figure out the compiler version and test_compiler_info to test it.

   get_compiler_info runs the preprocessor on this file and then eval's
   the result.  This sets various symbols for use by test_compiler_info.

   TODO: make compiler_info a local variable for get_compiler_info and
   test_compiler_info.

   TODO: all clients should use test_compiler_info and should not
   use gcc_compiled.

   */

set compiler_info "unknown"

#if defined (__GNUC__)
#if defined (__GNUC_PATCHLEVEL__)
/* Only GCC versions >= 3.0 define the __GNUC_PATCHLEVEL__ macro.  */
set compiler_info [join {gcc __GNUC__ __GNUC_MINOR__ __GNUC_PATCHLEVEL__} -]
#else
set compiler_info [join {gcc __GNUC__ __GNUC_MINOR__ "unknown"} -]
#endif
#endif

#if defined (__xlc__)
/* IBM'x xlc compiler. NOTE:  __xlc__ expands to a double quoted string of four
   numbers seperated by '.'s: currently "7.0.0.0" */
set need_a_set [regsub -all {\.} [join {xlc __xlc__} -] - compiler_info]
#endif

#if defined (__ARMCC_VERSION)
set compiler_info [join {armcc __ARMCC_VERSION} -]
#endif

#if defined (__clang__)
set compiler_info [join {clang __clang_major__ __clang_minor__ __clang_patchlevel__} -]
#endif

#if defined (__ICC)
set icc_major [string range __ICC 0 1]
set icc_minor [format "%d" [string range __ICC 2 [expr {[string length __ICC] -1}]]]
set icc_update __INTEL_COMPILER_UPDATE
set compiler_info [join "icc $icc_major $icc_minor $icc_update" -]
#elif defined (__ICL)
set icc_major [string range __ICL 0 1]
set icc_minor [format "%d" [string range __ICL 2 [expr {[string length __ICL] -1}]]]
set icc_update __INTEL_COMPILER_UPDATE
set compiler_info [join "icc $icc_major $icc_minor $icc_update" -]
#endif
