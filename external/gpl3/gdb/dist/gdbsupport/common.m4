dnl Autoconf configure snippets for common.
dnl Copyright (C) 1995-2020 Free Software Foundation, Inc.
dnl
dnl This file is part of GDB.
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Invoke configury needed by the files in 'common'.
AC_DEFUN([GDB_AC_COMMON], [
  # Set the 'development' global.
  . $srcdir/../bfd/development.sh

  AC_HEADER_STDC
  AC_FUNC_ALLOCA

  WIN32APILIBS=
  case ${host} in
    *mingw32*)
      AC_DEFINE(USE_WIN32API, 1,
		[Define if we should use the Windows API, instead of the
		 POSIX API.  On Windows, we use the Windows API when
		 building for MinGW, but the POSIX API when building
		 for Cygwin.])
      WIN32APILIBS="-lws2_32"
      ;;
  esac

  dnl Note that this requires codeset.m4, which is included
  dnl by the users of common.m4.
  AM_LANGINFO_CODESET

  AC_CHECK_HEADERS(linux/perf_event.h locale.h memory.h signal.h dnl
		   sys/resource.h sys/socket.h dnl
		   sys/un.h sys/wait.h dnl
		   thread_db.h wait.h dnl
		   termios.h dnl
		   dlfcn.h dnl
		   linux/elf.h proc_service.h dnl
		   poll.h sys/poll.h sys/select.h)

  AC_FUNC_MMAP
  AC_FUNC_VFORK
  AC_CHECK_FUNCS([fdwalk getrlimit pipe pipe2 poll socketpair sigaction \
		  ptrace64 sbrk setns sigaltstack sigprocmask \
		  setpgid setpgrp getrusage getauxval])

  dnl Check if we can disable the virtual address space randomization.
  dnl The functionality of setarch -R.
  AC_CHECK_DECLS([ADDR_NO_RANDOMIZE],,, [#include <sys/personality.h>])
  define([PERSONALITY_TEST], [AC_LANG_PROGRAM([#include <sys/personality.h>], [
  #      if !HAVE_DECL_ADDR_NO_RANDOMIZE
  #       define ADDR_NO_RANDOMIZE 0x0040000
  #      endif
	 /* Test the flag could be set and stays set.  */
	 personality (personality (0xffffffff) | ADDR_NO_RANDOMIZE);
	 if (!(personality (personality (0xffffffff)) & ADDR_NO_RANDOMIZE))
	     return 1])])
  AC_RUN_IFELSE([PERSONALITY_TEST],
		[have_personality=true],
		[have_personality=false],
		[AC_LINK_IFELSE([PERSONALITY_TEST],
				[have_personality=true],
				[have_personality=false])])
  if $have_personality
  then
      AC_DEFINE([HAVE_PERSONALITY], 1,
		[Define if you support the personality syscall.])
  fi

  AC_CHECK_DECLS([strstr])

  # ----------------------- #
  # Checks for structures.  #
  # ----------------------- #

  AC_CHECK_MEMBERS([struct stat.st_blocks, struct stat.st_blksize])

  AC_SEARCH_LIBS(kinfo_getfile, util util-freebsd,
    [AC_DEFINE(HAVE_KINFO_GETFILE, 1,
	      [Define to 1 if your system has the kinfo_getfile function. ])])

  # Check for std::thread.  This does not work on some platforms, like
  # mingw and DJGPP.
  AC_LANG_PUSH([C++])
  AX_PTHREAD([threads=yes], [threads=no])
  if test "$threads" = "yes"; then
    save_LIBS="$LIBS"
    LIBS="$PTHREAD_LIBS $LIBS"
    save_CXXFLAGS="$CXXFLAGS"
    CXXFLAGS="$PTHREAD_CFLAGS $save_CXXFLAGS"
    AC_CACHE_CHECK([for std::thread],
		   gdb_cv_cxx_std_thread,
		   [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <thread>
      void callback() { }]],
    [[std::thread t(callback);]])],
				  gdb_cv_cxx_std_thread=yes,
				  gdb_cv_cxx_std_thread=no)])

    # This check must be here, while LIBS includes any necessary
    # threading library.
    AC_CHECK_FUNCS([pthread_sigmask pthread_setname_np])

    LIBS="$save_LIBS"
    CXXFLAGS="$save_CXXFLAGS"
  fi
  if test "$gdb_cv_cxx_std_thread" = "yes"; then
    AC_DEFINE(CXX_STD_THREAD, 1,
	      [Define to 1 if std::thread works.])
  fi
  AC_LANG_POP

  dnl Check if sigsetjmp is available.  Using AC_CHECK_FUNCS won't
  dnl do since sigsetjmp might only be defined as a macro.
  AC_CACHE_CHECK([for sigsetjmp], gdb_cv_func_sigsetjmp,
  [AC_TRY_COMPILE([
  #include <setjmp.h>
  ], [sigjmp_buf env; while (! sigsetjmp (env, 1)) siglongjmp (env, 1);],
  gdb_cv_func_sigsetjmp=yes, gdb_cv_func_sigsetjmp=no)])
  if test "$gdb_cv_func_sigsetjmp" = "yes"; then
    AC_DEFINE(HAVE_SIGSETJMP, 1, [Define if sigsetjmp is available. ])
  fi

  AC_ARG_WITH(intel_pt,
    AS_HELP_STRING([--with-intel-pt], [include Intel Processor Trace support (auto/yes/no)]),
    [], [with_intel_pt=auto])
  AC_MSG_CHECKING([whether to use intel pt])
  AC_MSG_RESULT([$with_intel_pt])

  if test "${with_intel_pt}" = no; then
    AC_MSG_WARN([Intel Processor Trace support disabled; some features may be unavailable.])
    HAVE_LIBIPT=no
  else
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
  #include <linux/perf_event.h>
  #ifndef PERF_ATTR_SIZE_VER5
  # error
  #endif
    ]])], [perf_event=yes], [perf_event=no])
    if test "$perf_event" != yes; then
      if test "$with_intel_pt" = yes; then
	AC_MSG_ERROR([linux/perf_event.h missing or too old])
      else
	AC_MSG_WARN([linux/perf_event.h missing or too old; some features may be unavailable.])
      fi
    fi

    AC_LIB_HAVE_LINKFLAGS([ipt], [], [#include "intel-pt.h"], [pt_insn_alloc_decoder (0);])
    if test "$HAVE_LIBIPT" != yes; then
      if test "$with_intel_pt" = yes; then
	AC_MSG_ERROR([libipt is missing or unusable])
      else
	AC_MSG_WARN([libipt is missing or unusable; some features may be unavailable.])
      fi
    else
      save_LIBS=$LIBS
      LIBS="$LIBS $LIBIPT"
      AC_CHECK_FUNCS(pt_insn_event)
      AC_CHECK_MEMBERS([struct pt_insn.enabled, struct pt_insn.resynced], [], [],
		       [#include <intel-pt.h>])
      LIBS=$save_LIBS
    fi
  fi

  BFD_SYS_PROCFS_H
  if test "$ac_cv_header_sys_procfs_h" = yes; then
    BFD_HAVE_SYS_PROCFS_TYPE(gregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(fpregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(prgregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(prfpregset_t)
    BFD_HAVE_SYS_PROCFS_TYPE(prgregset32_t)
    BFD_HAVE_SYS_PROCFS_TYPE(lwpid_t)
    BFD_HAVE_SYS_PROCFS_TYPE(psaddr_t)
    BFD_HAVE_SYS_PROCFS_TYPE(elf_fpregset_t)
  fi
])
