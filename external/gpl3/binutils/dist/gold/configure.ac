dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT(gold, 0.1)
AC_CONFIG_SRCDIR(gold.cc)

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

AM_CONFIG_HEADER(config.h:config.in)

AC_ARG_WITH(sysroot,
[  --with-sysroot[=DIR]    search for usr/lib et al within DIR],
[sysroot=$withval], [sysroot=no])

if test "$sysroot" = "yes"; then
  sysroot='${exec_prefix}/${target_alias}/sys-root'
elif test "$sysroot" = "no"; then
  sysroot=
fi

sysroot_relocatable=0
if test -n "$sysroot"; then
  case "sysroot" in
    "${prefix}" | "${prefix}/"* | \
    "${exec_prefix}" | "${exec_prefix}/"* | \
    '${prefix}' | '${prefix}/'*| \
    '${exec_prefix}' | '${exec_prefix}/'*)
      sysroot_relocatable=1
      ;;
  esac
fi

AC_DEFINE_UNQUOTED(TARGET_SYSTEM_ROOT, "$sysroot",
  [System root for target files])
AC_DEFINE_UNQUOTED(TARGET_SYSTEM_ROOT_RELOCATABLE, $sysroot_relocatable,
  [Whether the system root can be relocated])

dnl For now threads are a configure time option.
AC_ARG_ENABLE([threads],
[  --enable-threads        multi-threaded linking],
[case "${enableval}" in
  yes | "") threads=yes ;;
  no) threads=no ;;
  *) threads=yes ;;
 esac],
[threads=no])
if test "$threads" = "yes"; then
  AC_DEFINE(ENABLE_THREADS, 1,
	    [Define to do multi-threaded linking])
fi
AM_CONDITIONAL(THREADS, test "$threads" = "yes")

AC_ARG_ENABLE([targets],
[  --enable-targets        alternative target configurations],
[case "${enableval}" in
  yes | "")
    AC_MSG_ERROR([--enable-targets option must specify target names or 'all'])
    ;;
  no)
    enable_targets=
    ;;
  *)
    enable_targets=$enableval
    ;;
esac],
[# For now, enable all targets by default
 enable_targets=all
])

# Canonicalize the enabled targets.
if test -n "$enable_targets"; then
  for targ in `echo $enable_targets | sed -e 's/,/ /g'`; do
    result=`$ac_config_sub $targ 2>/dev/null`
    if test -n "$result"; then
      canon_targets="$canon_targets $result"
    else
      # Permit unrecognized target names, like "all".
      canon_targets="$canon_targets $targ"
    fi
  done
fi

# See which specific instantiations we need.
targetobjs=
all_targets=
default_machine=
default_size=
default_big_endian=
targ_32_little=
targ_32_big=
targ_64_little=
targ_64_big=
for targ in $target $canon_targets; do
  if test "$targ" = "all"; then
    targ_32_little=yes
    targ_32_big=yes
    targ_64_little=yes
    targ_64_big=yes
    all_targets=yes
  else
    . ${srcdir}/configure.tgt

    if test "$targ_obj" = "UNKNOWN"; then
      AC_MSG_ERROR("unsupported target $targ")
    else
      targetobjs="$targetobjs ${targ_obj}.\$(OBJEXT)"
      if test "$targ_extra_obj" != ""; then
        targetobjs="$targetobjs ${targ_extra_obj}.\$(OBJEXT)"
      fi
      if test "$targ_size" = "32" -o "$targ_extra_size" = "32"; then
	  if test "$targ_big_endian" = "true" \
	      -o "$targ_extra_big_endian" = "true"; then
	      targ_32_big=yes
	  fi
	  if test "$targ_big_endian" = "false" \
	      -o "$targ_extra_big_endian" = "false"; then
	      targ_32_little=yes
	  fi
      fi
      if test "$targ_size" = "64" -o "$targ_extra_size" = "64"; then
	  if test "$targ_big_endian" = "true" \
	      -o "$targ_extra_big_endian" = "true"; then
	      targ_64_big=yes
	  fi
	  if test "$targ_big_endian" = "false" \
	      -o "$targ_extra_big_endian" = "false"; then
	      targ_64_little=yes
	  fi
      fi

      if test "$target" = "$targ"; then
        default_machine=$targ_machine
	default_size=$targ_size
	default_big_endian=$targ_big_endian
      fi
    fi
  fi
done

if test -n "$targ_32_little"; then
  AC_DEFINE(HAVE_TARGET_32_LITTLE, 1,
	      [Define to support 32-bit little-endian targets])
fi
if test -n "$targ_32_big"; then
  AC_DEFINE(HAVE_TARGET_32_BIG, 1,
	      [Define to support 32-bit big-endian targets])
fi
if test -n "$targ_64_little"; then
  AC_DEFINE(HAVE_TARGET_64_LITTLE, 1,
	      [Define to support 64-bit little-endian targets])
fi
if test -n "$targ_64_big"; then
  AC_DEFINE(HAVE_TARGET_64_BIG, 1,
	      [Define to support 64-bit big-endian targets])
fi

if test -n "$all_targets"; then
  TARGETOBJS='$(ALL_TARGETOBJS)'
else
  TARGETOBJS="$targetobjs"
fi
AC_SUBST(TARGETOBJS)

AC_DEFINE_UNQUOTED(GOLD_DEFAULT_MACHINE, $default_machine,
		   [Default machine code])
AC_DEFINE_UNQUOTED(GOLD_DEFAULT_SIZE, $default_size,
		   [Default size (32 or 64)])
AC_DEFINE_UNQUOTED(GOLD_DEFAULT_BIG_ENDIAN, $default_big_endian,
		   [Default big endian (true or false)])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

AC_GNU_SOURCE

ZW_GNU_GETTEXT_SISTER_DIR
AM_PO_SUBDIRS

AC_C_BIGENDIAN

AC_EXEEXT

AM_CONDITIONAL(NATIVE_LINKER,
  test "x$target_alias" = "x" -o "x$host_alias" = "x$target_alias")
AM_CONDITIONAL(GCC, test "$GCC" = yes)

dnl Some architectures do not support taking pointers of functions
dnl defined in shared libraries except in -fPIC mode.  We need to
dnl tell the unittest framework if we're compiling for one of those
dnl targets, so it doesn't try to run the tests that do that.
AM_CONDITIONAL(FN_PTRS_IN_SO_WITHOUT_PIC, [
  case $target_cpu in
    i?86) true;;
    x86_64) false;;
    sparc64) false;;
    *) true;;
  esac])

dnl Test for __thread support.
AC_CACHE_CHECK([for thread support], [gold_cv_c_thread],
[AC_COMPILE_IFELSE([__thread int i = 1;],
[gold_cv_c_thread=yes], [gold_cv_c_thread=no])])

AM_CONDITIONAL(TLS, test "$gold_cv_c_thread" = "yes")

dnl On GNU/Linux TLS in static programs only works when using glibc
dnl 2.4 or later.
AC_CACHE_CHECK([for glibc >= 2.4], [gold_cv_lib_glibc24],
[AC_COMPILE_IFELSE([
#include <features.h>
#if !defined __GLIBC__
error
#elif __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 4)
error
#endif
], [gold_cv_lib_glibc24=yes], [gold_cv_lib_glibc24=no])])

AM_CONDITIONAL(STATIC_TLS, test "$gold_cv_lib_glibc24" = "yes")

dnl Test for #pragma omp threadprivate
AC_CACHE_CHECK([for omp support], [gold_cv_c_threadprivate],
[save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fopenmp"
AC_COMPILE_IFELSE([
#include <omp.h>
int i;
#pragma omp threadprivate (i)
], [gold_cv_c_threadprivate=yes], [gold_cv_c_threadprivate=no])
CFLAGS="$save_CFLAGS"])
if test "$gold_cv_c_threadprivate" = "yes"; then
  AC_DEFINE(HAVE_OMP_SUPPORT, 1,
	    [Define if compiler supports #pragma omp threadprivate])
fi
AM_CONDITIONAL(OMP_SUPPORT, test "$gold_cv_c_threadprivate" = "yes")

dnl Test for the -ftls-dialect=gnu2 option.
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -mtls-dialect=gnu2"
AC_COMPILE_IFELSE([int i;], [have_tls_gnu2=yes], [have_tls_gnu2=no])
CFLAGS="$save_CFLAGS"
AM_CONDITIONAL(TLS_GNU2_DIALECT, test "$have_tls_gnu2" = "yes")

dnl On GNU/Linux TLS descriptors are supported by the dynamic loader
dnl only with glibc 2.5 or later.
AC_CACHE_CHECK([for glibc >= 2.5], [gold_cv_lib_glibc25],
[AC_COMPILE_IFELSE([
#include <features.h>
#if !defined __GLIBC__
error
#elif __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 5)
error
#endif
], [gold_cv_lib_glibc25=yes], [gold_cv_lib_glibc25=no])])

AM_CONDITIONAL(TLS_DESCRIPTORS, test "$gold_cv_lib_glibc25" = "yes")

dnl Check whether the compiler supports constructor priorities in
dnl attributes, which were added in gcc 4.3.
AC_CACHE_CHECK([for constructor priorities], [gold_cv_c_conprio],
[AC_COMPILE_IFELSE([void f() __attribute__ ((constructor (1)));],
[gold_cv_c_conprio=yes], [gold_cv_c_conprio=no])])

AM_CONDITIONAL(CONSTRUCTOR_PRIORITY, test "$gold_cv_c_conprio" = "yes")

dnl Test for the -frandom-seed option.
AC_CACHE_CHECK([for -frandom-seed support], [gold_cv_c_random_seed],
[save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -frandom-seed=foo"
AC_COMPILE_IFELSE([int i;], [gold_cv_c_random_seed=yes],
[gold_cv_c_random_seed=no])
CFLAGS="$save_CFLAGS"])
if test "$gold_cv_c_random_seed" = "yes"; then
  # In Makefile, '$@' will be expanded to be the name of the file
  # being built, providing a unique seed for each file.
  RANDOM_SEED_CFLAGS=-frandom-seed=\$@
fi
AC_SUBST(RANDOM_SEED_CFLAGS)

AM_BINUTILS_WARNINGS

WARN_CXXFLAGS=`echo ${WARN_CFLAGS} | sed -e 's/-Wstrict-prototypes//' -e 's/-Wmissing-prototypes//'`
AC_SUBST(WARN_CXXFLAGS)

dnl Force support for large files by default.  This may need to be
dnl host dependent.  If build == host, we can check getconf LFS_CFLAGS.
LFS_CFLAGS="-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
AC_SUBST(LFS_CFLAGS)

AC_REPLACE_FUNCS(pread)

# Link in zlib if we can.  This allows us to write compressed sections.
AC_SEARCH_LIBS(zlibVersion, z, [AC_CHECK_HEADERS(zlib.h)])
AM_CONDITIONAL(HAVE_ZLIB, test "$ac_cv_search_zlibVersion" != "no")

AC_LANG_PUSH(C++)

AC_CHECK_HEADERS(tr1/unordered_set tr1/unordered_map)
AC_CHECK_HEADERS(ext/hash_map ext/hash_set)
AC_CHECK_FUNCS(mallinfo)

# gcc 4.3.0 doesn't recognize the printf attribute on a template
# function.  Check for that.  This is gcc bug 35546.  This test can
# probably be removed after the bug has been fixed for a while.
AC_CACHE_CHECK([whether we can use attributes with template functions],
[gold_cv_template_attribute],
[AC_COMPILE_IFELSE([
template<typename T> extern void foo(const char*, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
template<typename T> void foo(const char* format, ...) {}
void bar() { foo<int>("%s\n", "foo"); }
], [gold_cv_template_attribute=yes], [gold_cv_template_attribute=no])])
if test "$gold_cv_template_attribute" = "yes"; then
  AC_DEFINE(HAVE_TEMPLATE_ATTRIBUTES, 1,
	    [Define if attributes work on C++ templates])
fi

AC_LANG_POP(C++)

AM_MAINTAINER_MODE

AC_OUTPUT(Makefile testsuite/Makefile po/Makefile.in:po/Make-in)
