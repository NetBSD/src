#serial 27

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN([jm_PREREQ],
[
  gl_BACKUPFILE
  jm_PREREQ_C_STACK
  gl_CANON_HOST
  gl_DIRNAME
  jm_PREREQ_ERROR
  gl_EXCLUDE
  gl_GETPAGESIZE
  gl_HARD_LOCALE
  gl_HASH
  gl_HUMAN
  gl_MBSWIDTH
  gl_FUNC_MEMCHR
  gl_PHYSMEM
  gl_POSIXVER
  gl_QUOTEARG
  gl_READUTMP
  gl_REGEX
  jm_PREREQ_STAT
  gl_FUNC_STRNLEN
  gl_XGETCWD
  gl_XREADLINK
])

AC_DEFUN([jm_PREREQ_STAT],
[
  AC_CHECK_HEADERS(sys/sysmacros.h sys/statvfs.h sys/vfs.h inttypes.h)
  AC_CHECK_HEADERS(sys/param.h sys/mount.h)
  AC_CHECK_FUNCS(statvfs)
  jm_AC_TYPE_LONG_LONG

  statxfs_includes="\
$ac_includes_default
#if HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#if ( ! HAVE_SYS_STATVFS_H && ! HAVE_SYS_VFS_H && HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H )
/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
# include <sys/param.h>
# include <sys/mount.h>
#endif
"
  AC_CHECK_MEMBERS([struct statfs.f_basetype],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_basetype],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_fstypename],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_type],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_type],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_fsid.__val],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_fsid.__val],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_namemax],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_namemax],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_namelen],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_namelen],,,[$statxfs_includes])
])
