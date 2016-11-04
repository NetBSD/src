dnl Process this file with autoconf to produce a configure script
dnl
dnl   Copyright (C) 2012-2016 Free Software Foundation, Inc.
dnl
dnl This file is free software; you can redistribute it and/or modify
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
dnl along with this program; see the file COPYING3.  If not see
dnl <http://www.gnu.org/licenses/>.
dnl

AC_PREREQ(2.59)
m4_include([../bfd/version.m4])
AC_INIT([ld], BFD_VERSION)
AC_CONFIG_SRCDIR(ldmain.c)

AC_CANONICAL_TARGET
AC_CANONICAL_BUILD
AC_ISC_POSIX

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CXX
AC_PROG_GREP
AC_GNU_SOURCE
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_INSTALL

LT_INIT
ACX_LARGEFILE

AC_ARG_WITH(lib-path, [  --with-lib-path=dir1:dir2...  set default LIB_PATH],LIB_PATH=$withval)
AC_ARG_ENABLE(targets,
[  --enable-targets        alternative target configurations],
[case "${enableval}" in
  yes | "") AC_MSG_ERROR(enable-targets option must specify target names or 'all')
            ;;
  no)       enable_targets= ;;
  *)        enable_targets=$enableval ;;
esac])dnl
AC_ARG_ENABLE(64-bit-bfd,
[  --enable-64-bit-bfd     64-bit support (on hosts with narrower word sizes)],
[case "${enableval}" in
  yes)  want64=true  ;;
  no)   want64=false ;;
  *)    AC_MSG_ERROR(bad value ${enableval} for 64-bit-bfd option) ;;
esac],[want64=false])dnl

AC_ARG_WITH(sysroot,
[  --with-sysroot[=DIR] Search for usr/lib et al within DIR.],
[
 case ${with_sysroot} in
 yes) TARGET_SYSTEM_ROOT='${exec_prefix}/${target_alias}/sys-root' ;;
 *) TARGET_SYSTEM_ROOT=$with_sysroot ;;
 esac

 TARGET_SYSTEM_ROOT_DEFINE='-DTARGET_SYSTEM_ROOT=\"$(TARGET_SYSTEM_ROOT)\"'
 use_sysroot=yes

 if test "x$prefix" = xNONE; then
  test_prefix=/usr/local
 else
  test_prefix=$prefix
 fi
 if test "x$exec_prefix" = xNONE; then
  test_exec_prefix=$test_prefix
 else
  test_exec_prefix=$exec_prefix
 fi
 case ${TARGET_SYSTEM_ROOT} in
 "${test_prefix}"|"${test_prefix}/"*|\
 "${test_exec_prefix}"|"${test_exec_prefix}/"*|\
 '${prefix}'|'${prefix}/'*|\
 '${exec_prefix}'|'${exec_prefix}/'*)
   t="$TARGET_SYSTEM_ROOT_DEFINE -DTARGET_SYSTEM_ROOT_RELOCATABLE"
   TARGET_SYSTEM_ROOT_DEFINE="$t"
   ;;
 esac
], [
 use_sysroot=no
 TARGET_SYSTEM_ROOT=
 TARGET_SYSTEM_ROOT_DEFINE='-DTARGET_SYSTEM_ROOT=\"\"'
])
AC_SUBST(use_sysroot)
AC_SUBST(TARGET_SYSTEM_ROOT)
AC_SUBST(TARGET_SYSTEM_ROOT_DEFINE)

dnl Use --enable-gold to decide if this linker should be the default.
dnl "install_as_default" is set to false if gold is the default linker.
dnl "installed_linker" is the installed BFD linker name.
AC_ARG_ENABLE(gold,
[[  --enable-gold[=ARG]     build gold [ARG={default,yes,no}]]],
[case "${enableval}" in
 default)
   install_as_default=no
   installed_linker=ld.bfd
   ;;
 yes|no)
   install_as_default=yes
   installed_linker=ld.bfd
   ;;
 *)
   AC_MSG_ERROR([invalid --enable-gold argument])
   ;;
 esac],
[install_as_default=yes
 installed_linker=ld.bfd])
AC_SUBST(install_as_default)
AC_SUBST(installed_linker)

AC_ARG_ENABLE([got],
AS_HELP_STRING([--enable-got=<type>],
               [GOT handling scheme (target, single, negative, multigot)]),
[case "${enableval}" in
  target | single | negative | multigot)  got_handling=$enableval ;;
  *)  AC_MSG_ERROR(bad value ${enableval} for --enable-got option) ;;
esac],
[got_handling=target])

case "${got_handling}" in
  target)
    AC_DEFINE([GOT_HANDLING_DEFAULT], [GOT_HANDLING_TARGET_DEFAULT],
              [Define to choose default GOT handling scheme]) ;;
  single)
    AC_DEFINE([GOT_HANDLING_DEFAULT], [GOT_HANDLING_SINGLE],
              [Define to choose default GOT handling scheme]) ;;
  negative)
    AC_DEFINE([GOT_HANDLING_DEFAULT], [GOT_HANDLING_NEGATIVE],
              [Define to choose default GOT handling scheme]) ;;
  multigot)
    AC_DEFINE([GOT_HANDLING_DEFAULT], [GOT_HANDLING_MULTIGOT],
              [Define to choose default GOT handling scheme]) ;;
  *)  AC_MSG_ERROR(bad value ${got_handling} for --enable-got option) ;;
esac

# PR gas/19109
# Decide the default method for compressing debug sections.
ac_default_compressed_debug_sections=unset
# Provide a configure time option to override our default.
AC_ARG_ENABLE(compressed_debug_sections,
	      AS_HELP_STRING([--enable-compressed-debug-sections={all,ld,none}],
	      [compress debug sections by default])],
[case ,"${enableval}", in
  ,yes, | ,all, | *,ld,*) ac_default_compressed_debug_sections=yes ;;
  ,no, | ,none,)  ac_default_compressed_debug_sections=no ;;
esac])dnl

# Decide if -z relro should be enabled in ELF linker by default.
ac_default_ld_z_relro=unset
# Provide a configure time option to override our default.
AC_ARG_ENABLE(relro,
	      AS_HELP_STRING([--enable-relro],
	      [enable -z relro in ELF linker by default]),
[case "${enableval}" in
  yes)  ac_default_ld_z_relro=1 ;;
  no)  ac_default_ld_z_relro=0 ;;
esac])dnl

AM_BINUTILS_WARNINGS

AM_LC_MESSAGES

AC_CONFIG_HEADERS([config.h:config.in])

# PR 14072
AH_VERBATIM([00_CONFIG_H_CHECK],
[/* Check that config.h is #included before system headers
   (this works only for glibc, but that should be enough).  */
#if defined(__GLIBC__) && !defined(__FreeBSD_kernel__) && !defined(__CONFIG_H__)
#  error config.h must be #included before system headers
#endif
#define __CONFIG_H__ 1])

if test -z "$target" ; then
    AC_MSG_ERROR(Unrecognized target system type; please check config.sub.)
fi
if test -z "$host" ; then
    AC_MSG_ERROR(Unrecognized host system type; please check config.sub.)
fi

# host-specific stuff:

ALL_LINGUAS="fr sv tr es da vi zh_CN zh_TW ga fi id bg it uk"
ZW_GNU_GETTEXT_SISTER_DIR
AM_PO_SUBDIRS

AC_EXEEXT

AC_PROG_YACC
AM_PROG_LEX

AM_MAINTAINER_MODE
AM_CONDITIONAL(GENINSRC_NEVER, false)
ACX_PROG_CMP_IGNORE_INITIAL

. ${srcdir}/configure.host

AC_SUBST(HDEFINES)
AC_SUBST(HOSTING_CRT0)
AC_SUBST(HOSTING_SCRT0)
AC_SUBST(HOSTING_LIBS)
AC_SUBST(HOSTING_SLIBS)
AC_SUBST(NATIVE_LIB_DIRS)

AC_CHECK_HEADERS(string.h strings.h stdlib.h unistd.h elf-hints.h limits.h locale.h sys/param.h)
AC_CHECK_HEADERS(fcntl.h sys/file.h sys/time.h sys/stat.h)
ACX_HEADER_STRING
AC_CHECK_FUNCS(glob mkstemp realpath sbrk setlocale waitpid)
AC_CHECK_FUNCS(open lseek close)
AC_HEADER_DIRENT

dnl AC_CHECK_HEADERS(sys/mman.h)
AC_FUNC_MMAP

AC_SEARCH_LIBS([dlopen], [dl])
AM_CONDITIONAL([ENABLE_PLUGINS], [test x$plugins = xyes])

AC_ARG_ENABLE(initfini-array,
	[  --enable-initfini-array	use .init_array/.fini_array sections],
	[], [
AC_CACHE_CHECK(for .preinit_array/.init_array/.fini_array support,
		 gcc_cv_initfini_array, [dnl
  if test "x${build}" = "x${target}" ; then
    AC_RUN_IFELSE([AC_LANG_SOURCE([
static int x = -1;
int main (void) { return x; }
int foo (void) { x = 0; }
int (*fp) (void) __attribute__ ((section (".init_array"))) = foo;])],
	     [gcc_cv_initfini_array=yes], [gcc_cv_initfini_array=no],
	     [gcc_cv_initfini_array=no])
   else
     gcc_cv_initfini_array=no
   fi])
  enable_initfini_array=$gcc_cv_initfini_array
])
AC_SUBST(enable_initfini_array)
if test $enable_initfini_array = yes; then
  AC_DEFINE(HAVE_INITFINI_ARRAY, 1,
    [Define .init_array/.fini_array sections are available and working.])
fi

AC_MSG_CHECKING(for a known getopt prototype in unistd.h)
AC_CACHE_VAL(ld_cv_decl_getopt_unistd_h,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <unistd.h>], [extern int getopt (int, char *const*, const char *);])],
ld_cv_decl_getopt_unistd_h=yes, ld_cv_decl_getopt_unistd_h=no)])
AC_MSG_RESULT($ld_cv_decl_getopt_unistd_h)
if test $ld_cv_decl_getopt_unistd_h = yes; then
  AC_DEFINE([HAVE_DECL_GETOPT], 1,
	    [Is the prototype for getopt in <unistd.h> in the expected format?])
fi

BFD_BINARY_FOPEN

AC_CHECK_DECLS([strstr, free, sbrk, getenv, environ])

# When converting linker scripts into strings for use in emulation
# files, use astring.sed if the compiler supports ANSI string
# concatenation, or ostring.sed otherwise.  This is to support the
# broken Microsoft MSVC compiler, which limits the length of string
# constants, while still supporting pre-ANSI compilers which do not
# support string concatenation.
AC_MSG_CHECKING([whether ANSI C string concatenation works])
AC_CACHE_VAL(ld_cv_string_concatenation,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [char *a = "a" "a";])],
  ld_cv_string_concatenation=yes,
  ld_cv_string_concatenation=no)])
AC_MSG_RESULT($ld_cv_string_concatenation)
if test "$ld_cv_string_concatenation" = "yes"; then
  STRINGIFY=astring.sed
else
  STRINGIFY=ostring.sed
fi
AC_SUBST(STRINGIFY)

# target-specific stuff:

all_targets=
EMUL=
all_emuls=
all_emul_extras=
all_libpath=

dnl We need to get an arbitrary number of tdir definitions into
dnl Makefile.  We can't do it using AC_SUBST, because autoconf does
dnl not permit literal newlines in an AC_SUBST variables.  So we use a
dnl file.
rm -f tdirs

# If the host is 64-bit, then we enable 64-bit targets by default.
# This is consistent with what ../bfd/configure.ac does.
if test x${want64} = xfalse; then
  AC_CHECK_SIZEOF(void *)
  if test "x${ac_cv_sizeof_void_p}" = "x8"; then
    want64=true
  fi
fi

elf_list_options=FALSE
elf_shlib_list_options=FALSE
elf_plt_unwind_list_options=FALSE
for targ_alias in `echo $target_alias $enable_targets | sed 's/,/ /g'`
do
  if test "$targ_alias" = "all"; then
    all_targets=true
    elf_list_options=TRUE
    elf_shlib_list_options=TRUE
    elf_plt_unwind_list_options=TRUE
  else
    # Canonicalize the secondary target names.
    result=`$ac_config_sub $targ_alias 2>/dev/null`
    if test -n "$result"; then
	targ=$result
    else
	targ=$targ_alias
    fi

    . ${srcdir}/configure.tgt

    if test "$targ" = "$target"; then
      EMUL=$targ_emul
    fi

    if test x${want64} = xfalse; then
      . ${srcdir}/../bfd/config.bfd
    fi

    if test x${want64} = xtrue; then
      targ_extra_emuls="$targ_extra_emuls $targ64_extra_emuls"
      targ_extra_libpath="$targ_extra_libpath $targ64_extra_libpath"
    fi

    for i in $targ_emul $targ_extra_emuls $targ_extra_libpath; do
	case " $all_emuls " in
	*" e${i}.o "*) ;;
	*)
	  all_emuls="$all_emuls e${i}.o"
	  eval result=\$tdir_$i
	  test -z "$result" && result=$targ_alias
	  echo tdir_$i=$result >> tdirs
	  case "${i}" in
	  *elf*)
	    elf_list_options=TRUE
	    ;;
	  *)
	    if $GREP "TEMPLATE_NAME=elf32" ${srcdir}/emulparams/${i}.sh >/dev/null 2>/dev/null; then
	      elf_list_options=TRUE
	    fi
	    ;;
	  esac
	  if test "$elf_list_options" = "TRUE"; then
	    . ${srcdir}/emulparams/${i}.sh
	    if test x${GENERATE_SHLIB_SCRIPT} = xyes; then
	      elf_shlib_list_options=TRUE
	    fi
	    if test x${PLT_UNWIND} = xyes; then
	      elf_plt_unwind_list_options=TRUE
	    fi
	  fi
	  ;;
	esac
    done

    for i in $targ_emul $targ_extra_libpath; do
	case " $all_libpath " in
	*" ${i} "*) ;;
	*)
	  if test -z "$all_libpath"; then
	    all_libpath=${i}
	  else
	    all_libpath="$all_libpath ${i}"
	  fi
	  ;;
	esac
    done

    for i in $targ_extra_ofiles; do
	case " $all_emul_extras " in
	*" ${i} "*) ;;
	*)
	  all_emul_extras="$all_emul_extras ${i}"
	  ;;
	esac
    done

  fi
done

if test x$ac_default_compressed_debug_sections = xyes ; then
  AC_DEFINE(DEFAULT_FLAG_COMPRESS_DEBUG, 1, [Define if you want compressed debug sections by default.])
fi

if test "${ac_default_ld_z_relro}" = unset; then
  ac_default_ld_z_relro=0
fi
AC_DEFINE_UNQUOTED(DEFAULT_LD_Z_RELRO,
  $ac_default_ld_z_relro,
  [Define to 1 if you want to enable -z relro in ELF linker by default.])

AC_SUBST(elf_list_options)
AC_SUBST(elf_shlib_list_options)
AC_SUBST(elf_plt_unwind_list_options)
AC_SUBST(EMUL)

TDIRS=tdirs
AC_SUBST_FILE(TDIRS)

if test x${all_targets} = xtrue; then
  if test x${want64} = xtrue; then
    EMULATION_OFILES='$(ALL_EMULATIONS) $(ALL_64_EMULATIONS)'
    EMUL_EXTRA_OFILES='$(ALL_EMUL_EXTRA_OFILES) $(ALL_64_EMUL_EXTRA_OFILES)'
  else
    EMULATION_OFILES='$(ALL_EMULATIONS)'
    EMUL_EXTRA_OFILES='$(ALL_EMUL_EXTRA_OFILES)'
  fi
else
  EMULATION_OFILES=$all_emuls
  EMUL_EXTRA_OFILES=$all_emul_extras
fi
AC_SUBST(EMULATION_OFILES)
AC_SUBST(EMUL_EXTRA_OFILES)
AC_SUBST(LIB_PATH)

EMULATION_LIBPATH=$all_libpath
AC_SUBST(EMULATION_LIBPATH)

if test x${enable_static} = xno; then
  TESTBFDLIB="--rpath ../bfd/.libs ../bfd/.libs/libbfd.so"
else
  TESTBFDLIB="../bfd/.libs/libbfd.a"
fi
AC_SUBST(TESTBFDLIB)

target_vendor=${target_vendor=$host_vendor}
case "$target_vendor" in
  hp) EXTRA_SHLIB_EXTENSION=".sl" ;;
  *)  EXTRA_SHLIB_EXTENSION= ;;
esac

case "$target_os" in
  lynxos) EXTRA_SHLIB_EXTENSION=".a" ;;
esac

if test x${EXTRA_SHLIB_EXTENSION} != x ; then
  AC_DEFINE_UNQUOTED(EXTRA_SHLIB_EXTENSION, "$EXTRA_SHLIB_EXTENSION",
   [Additional extension a shared object might have.])
fi

dnl Required by html, pdf, install-pdf and install-html
AC_SUBST(datarootdir)
AC_SUBST(docdir)
AC_SUBST(htmldir)
AC_SUBST(pdfdir)

AC_CONFIG_FILES(Makefile po/Makefile.in:po/Make-in)
AC_OUTPUT
