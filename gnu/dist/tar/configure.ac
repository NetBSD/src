# Configure template for GNU tar.

# Copyright (C) 1991, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001 Free
# Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AC_INIT(src/tar.c)
AM_CONFIG_HEADER(config.h)
AC_PREREQ(2.52)
AM_INIT_AUTOMAKE(tar, 1.13.25)
AC_DEFINE(_GNU_SOURCE, 1,
  [Define unconditionally for setting a GNU environment.])
ALL_LINGUAS="cs da de es et fr id it ja ko nl no pl pt pt_BR ru sl sv tr"

AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_AIX
AC_MINIX
AC_EXEEXT
AC_PROG_RANLIB
AC_SYS_LARGEFILE
AC_ISC_POSIX
AM_C_PROTOTYPES
AC_C_CONST
AC_C_INLINE

# getdate.y works with bison only.
YACC=${YACC-'bison -y'}
AC_SUBST(YACC)

AC_CHECK_HEADERS(fcntl.h limits.h linux/fd.h memory.h net/errno.h poll.h \
  sgtty.h stdbool.h string.h stropts.h \
  sys/buf.h sys/device.h sys/gentape.h \
  sys/inet.h sys/io/trioctl.h sys/ioccom.h \
  sys/mtio.h sys/param.h sys/tprintf.h sys/tape.h sys/time.h sys/timeb.h \
  unistd.h wchar.h wctype.h)

AC_HEADER_SYS_WAIT

AC_MBSTATE_T

# It seems that that many machines where <utime.h> seems to be
# broken just require something like -D_XXX_SOURCE, where XXX might
# be POSIX, POSIX_C, ALL, HPUX or whatever, depending on the machine.

AC_CACHE_CHECK(for utime.h, tar_cv_header_utime_h,
  [AC_TRY_COMPILE([
#include <sys/types.h>
#include <utime.h>], [struct utimbuf foo],
  tar_cv_header_utime_h=yes, tar_cv_header_utime_h=no)])
if test $tar_cv_header_utime_h = yes; then
  AC_DEFINE(HAVE_UTIME_H, 1,
    [Define to 1 if utime.h exists and declares struct utimbuf.])
fi

if test $ac_cv_header_sys_mtio_h = yes; then
  AC_CACHE_CHECK(for remote tape header files, tar_cv_header_rmt,
    [AC_TRY_CPP([
#if HAVE_SGTTY_H
# include <sgtty.h>
#endif
#include <sys/socket.h>],
    tar_cv_header_rmt=yes, tar_cv_header_rmt=no)])
  test $tar_cv_header_rmt = yes && RMT='rmt$(EXEEXT)'
  AC_SUBST(RMT)
fi

jm_CHECK_DECLARATION(free, [#include <stdlib.h>])
test $jm_cv_func_decl_free != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_FREE, $?,
  [Define to 1 if free is declared.])

jm_CHECK_DECLARATION(getgrgid, [#include <grp.h>])
test $jm_cv_func_decl_getgrgid != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_GETGRGID, $?,
  [Define to 1 if getgrgid is declared.])

jm_CHECK_DECLARATION(getpwuid, [#include <pwd.h>])
test $jm_cv_func_decl_getpwuid != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_GETPWUID, $?,
  [Define to 1 if getpwuid is declared.])

jm_CHECK_DECLARATION(getenv, [#include <stdlib.h>])
test $jm_cv_func_decl_getenv != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_GETENV, $?,
  [Define to 1 if getenv is declared.])

jm_CHECK_DECLARATION(malloc, [#include <stdlib.h>])
test $jm_cv_func_decl_malloc != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_MALLOC, $?,
  [Define to 1 if malloc is declared.])

jm_CHECK_DECLARATION(strtoul, [#include <stdlib.h>])
test $jm_cv_func_decl_strtoul != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_STRTOUL, $?,
  [Define to 1 if strtoul is declared.])

jm_CHECK_DECLARATION(strtoull, [#include <stdlib.h>])
test $jm_cv_func_decl_strtoull != yes
AC_DEFINE_UNQUOTED(HAVE_DECL_STRTOULL, $?,
  [Define to 1 if strtoull is declared.])

AC_CACHE_CHECK(which ioctl field to test for reversed bytes,
  tar_cv_header_mtio_check_field,
  [AC_EGREP_HEADER(mt_model, sys/mtio.h,
  tar_cv_header_mtio_check_field=mt_model,
  tar_cv_header_mtio_check_field=mt_type)])
AC_DEFINE_UNQUOTED(MTIO_CHECK_FIELD, $tar_cv_header_mtio_check_field,
  [Define to mt_model (v.g., for DG/UX), else to mt_type.])

AC_HEADER_DIRENT
AC_HEADER_MAJOR
AC_HEADER_STAT
AC_HEADER_STDC
AC_HEADER_TIME
AC_STRUCT_ST_BLKSIZE
AC_STRUCT_ST_BLOCKS
AC_STRUCT_TIMEZONE
jm_CHECK_TYPE_STRUCT_DIRENT_D_INO
AC_MSG_CHECKING([for st_fstype string in struct stat])
AC_CACHE_VAL(diff_cv_st_fstype_string,
  [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_fstype[0] = 'x';],
   diff_cv_st_fstype_string=yes,
   diff_cv_st_fstype_string=no)])
AC_MSG_RESULT($diff_cv_st_fstype_string)
if test $diff_cv_st_fstype_string = yes; then
  AC_DEFINE(HAVE_ST_FSTYPE_STRING, 1,
    [Define if struct stat has a char st_fstype[] member.])
fi

AC_TYPE_SIGNAL
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_CHECK_TYPE(major_t, , AC_DEFINE(major_t, int,
				   [Type of major device numbers.]))
AC_CHECK_TYPE(minor_t, , AC_DEFINE(minor_t, int,
				   [Type of minor device numbers.]))
AC_CHECK_TYPE(ssize_t, , AC_DEFINE(ssize_t, int,
				   [Type of sizes or error indications.]))
jm_AC_TYPE_UINTMAX_T
jm_AC_PREREQ_XSTRTOUMAX
jm_PREREQ_ADDEXT
jm_PREREQ_ERROR
jm_PREREQ_HUMAN
jm_PREREQ_QUOTEARG
jm_PREREQ_XGETCWD

# Set tar_LDADD to -lrt or -lposix4 if necessary.
#
# Solaris 2.5.1 needs -lposix4 to get the clock_gettime function.
# Solaris 7 prefers the library name -lrt to the obsolescent name -lposix4.
tar_LDADD=
tar_save_LIBS="$LIBS"
AC_SEARCH_LIBS(clock_gettime, [rt posix4])
AC_CHECK_FUNCS(clock_gettime)
if test " $LIBS" != " $tar_save_LIBS"; then
  tar_LDADD="$ac_cv_search_clock_gettime"
  LIBS="$tar_save_LIBS"
fi
AC_SUBST(tar_LDADD)

AC_CHECK_FUNCS(fchdir fsync ftime getcwd isascii lstat \
  mkfifo nap napms poll \
  readlink select strerror strstr symlink usleep)

AC_CACHE_CHECK(for mknod, tar_cv_func_mknod,
  [AC_TRY_LINK([
#include <sys/types.h>
#include <sys/stat.h>],
  [mknod (0, 0, 0)],
  tar_cv_func_mknod=yes, tar_cv_func_mknod=no)])
if test $tar_cv_func_mknod = yes; then
  AC_DEFINE(HAVE_MKNOD, 1,
    [Define to 1 if the mknod function is available.])
fi

# Set rmt_LDADD to -lnsl -lsocket if necessary.
#
# Whenever both -lsocket and -lnsl are needed, it seems to be always the
# case that gethostbyname requires -lnsl.  So, check -lnsl first, for it
# needs to be in LIBS before the setsockopt checks are performed.  *However*,
# on SINIX-N 5.43, this is false, and gethostent seems to be a better
# candidate.
rmt_LDADD=
tar_save_LIBS="$LIBS"
AC_SEARCH_LIBS(gethostent, [nsl])
if test " $LIBS" != " $tar_save_LIBS"; then
  rmt_LDADD="$rmt_LDADD $ac_cv_search_gethostent"
fi
tar_save_LIBS1="$LIBS"
AC_SEARCH_LIBS(setsockopt, [socket])
if test " $LIBS" != " $tar_save_LIBS1"; then
  rmt_LDADD="$rmt_LDADD $ac_cv_search_setsockopt"
fi
LIBS="$tar_save_LIBS"
AC_SUBST(rmt_LDADD)

AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_STRERROR_R

# fnmatch
AC_FUNC_FNMATCH
if test $ac_cv_func_fnmatch_works = yes; then
  rm -f lib/fnmatch.h
else
  AC_LIBOBJ(fnmatch)
  AC_CONFIG_LINKS(lib/fnmatch.h:lib/fnmatch.hin)
fi

AC_FUNC_VPRINTF
AC_REPLACE_FUNCS(ftruncate lchown memset rename rmdir \
  strcasecmp strncasecmp strtol strtoul waitpid)
AM_FUNC_GETLINE
jm_FUNC_MALLOC
jm_FUNC_MKTIME
jm_FUNC_REALLOC
test "$ac_cv_func_strstr" = yes || LIBOBJS="$LIBOBJS strstr.o"

AC_CACHE_CHECK(for remote shell, tar_cv_path_RSH,
  [if test -n "$RSH"; then
    tar_cv_path_RSH=$RSH
  else
    tar_cv_path_RSH=no
    for ac_file in /usr/ucb/rsh /usr/bin/remsh /usr/bin/rsh /usr/bsd/rsh \
	/usr/bin/nsh /usr/bin/rcmd
    do
      # Prefer a non-symlink rsh to a symlink one, so that binaries built
      # on AIX 4.1.4, where /usr/ucb/rsh is a symlink to /usr/bin/rsh
      # will run on AIX 4.3.0, which has only /usr/bin/rsh.
      if test -f $ac_file; then
	if (test -h $ac_file) 2>/dev/null; then
	  test $tar_cv_path_RSH = no && tar_cv_path_RSH=$ac_file
	else
	  tar_cv_path_RSH=$ac_file
	  break
	fi
      fi
    done
  fi])
if test $tar_cv_path_RSH = no; then
  AC_CHECK_HEADERS(netdb.h)
else
  AC_DEFINE_UNQUOTED(REMOTE_SHELL, "$tar_cv_path_RSH",
    [Define to the full path of your rsh, if any.])
fi

AC_MSG_CHECKING(for default archive)

if test -z "$DEFAULT_ARCHIVE"; then
  DEFAULT_ARCHIVE=-
else
  if test -z "`ls $DEFAULT_ARCHIVE 2>/dev/null`"; then
    AC_MSG_WARN(DEFAULT_ARCHIVE \`$DEFAULT_ARCHIVE' not found on this system)
  fi
  # FIXME: Look for DEFTAPE in <sys/mtio.h>.
  # FIXME: Let DEVICE_PREFIX be configured from the environment.
  # FIXME: Rearrange, here.
  case $DEFAULT_ARCHIVE in
    *[[0-7][lmh]])
      AC_DEFINE(DENSITY_LETTER, 1,
	[[Define to 1 if density may be indicated by [lmh] at end of device.]])
      device_prefix=`echo $DEFAULT_ARCHIVE | sed 's/[0-7][lmh]$//'`
      ;;
    *[[0-7]])
      device_prefix=`echo $DEFAULT_ARCHIVE | sed 's/[0-7]$//'`
      ;;
    *)
      device_prefix=
      ;;
  esac
  case "$device_prefix" in
    ?*)
      AC_DEFINE_UNQUOTED(DEVICE_PREFIX, "$device_prefix",
	[Define to a string giving the prefix of the default device, without the part specifying the unit and density.])
      ;;
  esac
fi
AC_DEFINE_UNQUOTED(DEFAULT_ARCHIVE, "$DEFAULT_ARCHIVE",
  [Define to a string giving the full name of the default archive file.])
AC_MSG_RESULT($DEFAULT_ARCHIVE)

AC_MSG_CHECKING(for default blocking)
DEFAULT_BLOCKING=${DEFAULT_BLOCKING-20}
AC_DEFINE_UNQUOTED(DEFAULT_BLOCKING, $DEFAULT_BLOCKING,
  [Define to a number giving the default blocking size for archives.])
AC_MSG_RESULT($DEFAULT_BLOCKING)

AM_GNU_GETTEXT
if test $USE_INCLUDED_LIBINTL = yes; then
  AC_DEFINE(USE_INCLUDED_LIBINTL, 1,
    [Define to 1 if using the included libintl implementation.])
fi

AC_OUTPUT([Makefile doc/Makefile intl/Makefile lib/Makefile \
m4/Makefile po/Makefile.in \
scripts/Makefile src/Makefile tests/Makefile tests/preset])
