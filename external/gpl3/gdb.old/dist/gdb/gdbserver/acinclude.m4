dnl gdb/gdbserver/configure.in uses BFD_HAVE_SYS_PROCFS_TYPE.
sinclude(../../bfd/bfd.m4)

sinclude(../acx_configure_dir.m4)

# This gets GDB_AC_LIBMCHECK.
sinclude(../libmcheck.m4)

# This gets AM_GDB_WARNINGS.
sinclude(../warning.m4)

dnl This gets autoconf bugfixes
sinclude(../../config/override.m4)

dnl For ACX_PKGVERSION and ACX_BUGURL.
sinclude(../../config/acx.m4)

m4_include(../../config/depstand.m4)
m4_include(../../config/lead-dot.m4)

dnl codeset.m4 is needed for common.m4, but not for
dnl anything else in gdbserver.
m4_include(../../config/codeset.m4)
m4_include(../common/common.m4)

dnl For libiberty_INIT.
m4_include(../libiberty.m4)

dnl For --enable-build-with-cxx and COMPILER.
m4_include(../build-with-cxx.m4)

dnl For GDB_AC_PTRACE.
m4_include(../ptrace.m4)

dnl Check for existence of a type $1 in libthread_db.h
dnl Based on BFD_HAVE_SYS_PROCFS_TYPE in bfd/bfd.m4.

AC_DEFUN([GDBSERVER_HAVE_THREAD_DB_TYPE],
[AC_MSG_CHECKING([for $1 in thread_db.h])
 AC_CACHE_VAL(gdbserver_cv_have_thread_db_type_$1,
   [AC_TRY_COMPILE([
#include <thread_db.h>],
      [$1 avar],
      gdbserver_cv_have_thread_db_type_$1=yes,
      gdbserver_cv_have_thread_db_type_$1=no
   )])
 if test $gdbserver_cv_have_thread_db_type_$1 = yes; then
   AC_DEFINE([HAVE_]translit($1, [a-z], [A-Z]), 1,
	     [Define if <thread_db.h> has $1.])
 fi
 AC_MSG_RESULT($gdbserver_cv_have_thread_db_type_$1)
])
