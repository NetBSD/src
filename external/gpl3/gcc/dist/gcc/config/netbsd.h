/* Base configuration file for all NetBSD targets.
   Copyright (C) 1997-2013 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* TARGET_OS_CPP_BUILTINS() common to all NetBSD targets.  */
#define NETBSD_OS_CPP_BUILTINS_COMMON()		\
  do						\
    {						\
      builtin_define ("__NetBSD__");		\
      builtin_define ("__unix__");		\
      builtin_assert ("system=bsd");		\
      builtin_assert ("system=unix");		\
      builtin_assert ("system=NetBSD");		\
    }						\
  while (0)

/* CPP_SPEC parts common to all NetBSD targets.  */
#define NETBSD_CPP_SPEC				\
  "%{posix:-D_POSIX_SOURCE} \
   %{pthread:-D_REENTRANT -D_PTHREADS}"

/* NETBSD_NATIVE is defined when gcc is integrated into the NetBSD
   source tree so it can be configured appropriately without using
   the GNU configure/build mechanism.

   NETBSD_TOOLS is defined when gcc is built as cross-compiler for
   the in-tree toolchain.
 */

#if defined(NETBSD_NATIVE) || defined(NETBSD_TOOLS)

/* Look for the include files in the system-defined places.  */

#undef GPLUSPLUS_INCLUDE_DIR
#define GPLUSPLUS_INCLUDE_DIR "/usr/include/g++"

#undef GPLUSPLUS_INCLUDE_DIR_ADD_SYSROOT
#define GPLUSPLUS_INCLUDE_DIR_ADD_SYSROOT 1

#undef GPLUSPLUS_BACKWARD_INCLUDE_DIR
#define GPLUSPLUS_BACKWARD_INCLUDE_DIR "/usr/include/g++/backward"

#undef GCC_INCLUDE_DIR_ADD_SYSROOT
#define GCC_INCLUDE_DIR_ADD_SYSROOT 1

/*
 * XXX figure out a better way to do this
 */
#undef GCC_INCLUDE_DIR
#define GCC_INCLUDE_DIR "/usr/include/gcc-4.8"

/* Under NetBSD, the normal location of the various *crt*.o files is the
   /usr/lib directory.  */

#undef STANDARD_STARTFILE_PREFIX
#define STANDARD_STARTFILE_PREFIX	"/usr/lib/"
#undef STANDARD_STARTFILE_PREFIX_1
#define STANDARD_STARTFILE_PREFIX_1	"/usr/lib/"

#endif /* NETBSD_NATIVE || NETBSD_TOOLS */

#if defined(NETBSD_NATIVE)
/* Under NetBSD, the normal location of the compiler back ends is the
   /usr/libexec directory.  */

#undef STANDARD_EXEC_PREFIX
#define STANDARD_EXEC_PREFIX		"/usr/libexec/"

#undef TOOLDIR_BASE_PREFIX
#define TOOLDIR_BASE_PREFIX		"../"

#undef STANDARD_BINDIR_PREFIX
#define STANDARD_BINDIR_PREFIX		"/usr/bin"

#undef STANDARD_LIBEXEC_PREFIX
#define STANDARD_LIBEXEC_PREFIX		STANDARD_EXEC_PREFIX

#endif /* NETBSD_NATIVE */


/* Provide a LIB_SPEC appropriate for NetBSD.  Here we:

   1. Select the appropriate set of libs, depending on whether we're
      profiling.

   2. Include the pthread library if -pthread is specified (only
      if threads are enabled).

   3. Include the posix library if -posix is specified.

   FIXME: Could eliminate the duplication here if we were allowed to
   use string concatenation.  */

#ifdef NETBSD_ENABLE_PTHREADS
#define NETBSD_LIB_SPEC		\
  "%{pthread:			\
     %{!p:			\
       %{!pg:-lpthread}}	\
     %{p:-lpthread_p}		\
     %{pg:-lpthread_p}}		\
   %{posix:			\
     %{!p:			\
       %{!pg:-lposix}}		\
     %{p:-lposix_p}		\
     %{pg:-lposix_p}}		\
   %{shared:-lc}		\
   %{!shared:			\
     %{!symbolic:		\
       %{!p:			\
	 %{!pg:-lc}}		\
       %{p:-lc_p}		\
       %{pg:-lc_p}}}"
#else
#define NETBSD_LIB_SPEC		\
  "%{posix:			\
     %{!p:			\
       %{!pg:-lposix}}		\
     %{p:-lposix_p}		\
     %{pg:-lposix_p}}		\
   %{shared:-lc}		\
   %{!shared:			\
     %{!symbolic:		\
       %{!p:			\
	 %{!pg:-lc}}		\
       %{p:-lc_p}		\
       %{pg:-lc_p}}}"
#endif

#undef LIB_SPEC
#define LIB_SPEC NETBSD_LIB_SPEC

/* Pass -cxx-isystem to cc1/cc1plus.  */
#define NETBSD_CC1_AND_CC1PLUS_SPEC		\
  "%{cxx-isystem}"

#undef CC1_SPEC
#define CC1_SPEC NETBSD_CC1_AND_CC1PLUS_SPEC

#undef CC1PLUS_SPEC
#define CC1PLUS_SPEC NETBSD_CC1_AND_CC1PLUS_SPEC

/* When building shared libraries, the initialization and finalization 
   functions for the library are .init and .fini respectively.  */

#define COLLECT_SHARED_INIT_FUNC(STREAM,FUNC)				\
  do {									\
    fprintf ((STREAM), "void __init() __asm__ (\".init\");");		\
    fprintf ((STREAM), "void __init() {\n\t%s();\n}\n", (FUNC));	\
  } while (0)

#define COLLECT_SHARED_FINI_FUNC(STREAM,FUNC)				\
  do {									\
    fprintf ((STREAM), "void __fini() __asm__ (\".fini\");");		\
    fprintf ((STREAM), "void __fini() {\n\t%s();\n}\n", (FUNC));	\
  } while (0)

#undef TARGET_POSIX_IO
#define TARGET_POSIX_IO

/* Don't assume anything about the header files.  */
#undef  NO_IMPLICIT_EXTERN_C
#define NO_IMPLICIT_EXTERN_C    1

/* Define some types that are the same on all NetBSD platforms,
   making them agree with <machine/ansi.h>.  */

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

#undef WINT_TYPE
#define WINT_TYPE "int"

#define LINK_EH_SPEC "%{!static:--eh-frame-hdr} "

/* Use --as-needed -lgcc_s for eh support.  */
#ifdef HAVE_LD_AS_NEEDED
#define USE_LD_AS_NEEDED 1
#endif
