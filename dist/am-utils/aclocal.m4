# aclocal.m4 generated automatically by aclocal 1.6.3 -*- Autoconf -*-

# Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

dnl aclocal.m4 file for am-utils-6.x
dnl Contains definitions for specialized GNU-autoconf macros.
dnl Author: Erez Zadok <ezk@cs.columbia.edu>
dnl
dnl DO NOT EDIT DIRECTLY!  Generated automatically by maintainers from
dnl m4/GNUmakefile!
dnl
dnl ######################################################################
dnl UNCOMMENT THE NEXT FEW LINES FOR DEBUGGING CONFIGURE
dnl define([AC_CACHE_LOAD], )dnl
dnl define([AC_CACHE_SAVE], )dnl
dnl ======================================================================



dnl ######################################################################
dnl check if compiler can handle "void *"
AC_DEFUN(AMU_C_VOID_P,
[
AC_CACHE_CHECK(if compiler can handle void *,
ac_cv_c_void_p,
[
# try to compile a program which uses void *
AC_TRY_COMPILE(
[ ],
[
void *vp;
], ac_cv_c_void_p=yes, ac_cv_c_void_p=no)
])
if test "$ac_cv_c_void_p" = yes
then
  AC_DEFINE(voidp, void *)
else
  AC_DEFINE(voidp, char *)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl New versions of the cache functions which also dynamically evaluate the
dnl cache-id field, so that it may contain shell variables to expand
dnl dynamically for the creation of $ac_cv_* variables on the fly.
dnl In addition, this function allows you to call COMMANDS which generate
dnl output on the command line, because it prints its own AC_MSG_CHECKING
dnl after COMMANDS are run.
dnl
dnl ======================================================================
dnl AMU_CACHE_CHECK_DYNAMIC(MESSAGE, CACHE-ID, COMMANDS)
define(AMU_CACHE_CHECK_DYNAMIC,
[
ac_tmp=`echo $2`
if eval "test \"`echo '$''{'$ac_tmp'+set}'`\" = set"; then
  AC_MSG_CHECKING([$1])
  echo $ECHO_N "(cached) $ECHO_C" 1>&AS_MESSAGE_FD([])
dnl XXX: for older autoconf versions
dnl  echo $ac_n "(cached) $ac_c" 1>&AS_MESSAGE_FD([])
else
  $3
  AC_MSG_CHECKING([$1])
fi
ac_tmp_val=`eval eval "echo '$''{'$ac_tmp'}'"`
AC_MSG_RESULT($ac_tmp_val)
])
dnl ======================================================================


dnl ######################################################################
dnl check if an automounter filesystem exists (it almost always does).
dnl Usage: AC_CHECK_AMU_FS(<fs>, <msg>, [<depfs>])
dnl Print the message in <msg>, and declare HAVE_AMU_FS_<fs> true.
dnl If <depfs> is defined, then define this filesystem as tru only of the
dnl filesystem for <depfs> is true.
AC_DEFUN(AMU_CHECK_AMU_FS,
[
# store variable name of fs
ac_upcase_am_fs_name=`echo $1 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=HAVE_AMU_FS_$ac_upcase_am_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for $2 filesystem ($1),
ac_cv_am_fs_$1,
[
# true by default
eval "ac_cv_am_fs_$1=yes"
# if <depfs> exists but is defined to "no", set this filesystem to no.
if test -n "$3"
then
  # flse by default if arg 3 was supplied
  eval "ac_cv_am_fs_$1=no"
  if test "`eval echo '$''{ac_cv_fs_'$3'}'`" = yes
  then
    eval "ac_cv_am_fs_$1=yes"
  fi
  # some filesystems do not have a mnttab entry, but exist based on headers
  if test "`eval echo '$''{ac_cv_fs_header_'$3'}'`" = yes
  then
    eval "ac_cv_am_fs_$1=yes"
  fi
fi
])
# check if need to define variable
if test "`eval echo '$''{ac_cv_am_fs_'$1'}'`" = yes
then
  AC_DEFINE_UNQUOTED($ac_safe)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check the autofs flavor
AC_DEFUN(AMU_CHECK_AUTOFS_STYLE,
[
AC_CACHE_CHECK(autofs style,
ac_cv_autofs_style,
[
# select the correct style to mount(2) a filesystem
case "${host_os}" in
       solaris1* | solaris2.[[0-4]] )
	       ac_cv_autofs_style=default ;;
       solaris2.5* )
               ac_cv_autofs_style=solaris_v1 ;;
       # Solaris 8+ uses the AutoFS V3 protocol, but it's very similar to V2,
       # so use one style for both.
       solaris* )
               ac_cv_autofs_style=solaris_v2_v3 ;;
#       irix* )
#	       ac_cv_autofs_style=solaris_v1 ;;
       linux* )
               ac_cv_autofs_style=linux ;;
       * )
               ac_cv_autofs_style=default ;;
esac
])
# always make a link and include the file name, otherwise on systems where
# autofs support has not been ported yet check_fs_{headers, mntent}.m4 add
# ops_autofs.o to AMD_FS_OBJS, but there's no way to build it.
am_utils_autofs_style=$srcdir"/conf/autofs/autofs_"$ac_cv_autofs_style".h"
am_utils_link_files=${am_utils_link_files}amd/ops_autofs.c:conf/autofs/autofs_${ac_cv_autofs_style}.c" "
AC_SUBST_FILE(am_utils_autofs_style)
])
dnl ======================================================================


dnl ######################################################################
dnl check style of fixmount check_mount() function
AC_DEFUN(AMU_CHECK_CHECKMOUNT_STYLE,
[
AC_CACHE_CHECK(style of fixmount check_mount(),
ac_cv_style_checkmount,
[
# select the correct style for unmounting filesystems
case "${host_os_name}" in
	svr4* | sysv4* | solaris2* | sunos5* )
			ac_cv_style_checkmount=svr4 ;;
	bsd44* | bsdi* | freebsd* | netbsd* | openbsd* | darwin* | rhapsody* )
			ac_cv_style_checkmount=bsd44 ;;
	aix* )
			ac_cv_style_checkmount=aix ;;
	osf* )
			ac_cv_style_checkmount=osf ;;
	ultrix* )
			ac_cv_style_checkmount=ultrix ;;
	* )
			ac_cv_style_checkmount=default ;;
esac
])
am_utils_checkmount_style_file="check_mount.c"
am_utils_link_files=${am_utils_link_files}fixmount/${am_utils_checkmount_style_file}:conf/checkmount/checkmount_${ac_cv_style_checkmount}.c" "

])
dnl ======================================================================


dnl ######################################################################
dnl check for external definition for a function (not external variables)
dnl Usage AMU_CHECK_EXTERN(extern)
dnl Checks for external definition for "extern" that is delimited on the
dnl left and the right by a character that is not a valid symbol character.
dnl
dnl Note that $pattern below is very carefully crafted to match any system
dnl external definition, with __P posix prototypes, with or without an extern
dnl word, etc.  Think twice before changing this.
AC_DEFUN(AMU_CHECK_EXTERN,
[
# store variable name for external definition
ac_upcase_extern_name=`echo $1 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=HAVE_EXTERN_$ac_upcase_extern_name
# check for cached value and set it if needed
AMU_CACHE_CHECK_DYNAMIC(external function definition for $1,
ac_cv_extern_$1,
[
# the old pattern assumed that the complete external definition is on one
# line but on some systems it is split over several lines, so only match
# beginning of the extern definition including the opening parenthesis.
#pattern="(extern)?.*[^a-zA-Z0-9_]$1[^a-zA-Z0-9_]?.*\(.*\).*;"
pattern="(extern)?.*[[^a-zA-Z0-9_]]$1[[^a-zA-Z0-9_]]?.*\("
AC_EGREP_CPP(${pattern},
[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else /* not TIME_WITH_SYS_TIME */
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else /* not HAVE_SYS_TIME_H */
#  include <time.h>
# endif /* not HAVE_SYS_TIME_H */
#endif /* not TIME_WITH_SYS_TIME */

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif /* HAVE_STDIO_H */
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#if HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_STRING_H
# include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif /* HAVE_STRINGS_H */
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif /* HAVE_NETDB_H */
#ifdef HAVE_CLUSTER_H
# include <cluster.h>
#endif /* HAVE_CLUSTER_H */
#ifdef HAVE_RPC_RPC_H
/*
 * Turn on PORTMAP, so that additional header files would get included
 * and the important definition for UDPMSGSIZE is included too.
 */
# ifndef PORTMAP
#  define PORTMAP
# endif /* not PORTMAP */
# include <rpc/rpc.h>
# ifndef XDRPROC_T_TYPE
typedef bool_t (*xdrproc_t) __P ((XDR *, __ptr_t, ...));
# endif /* not XDRPROC_T_TYPE */
#endif /* HAVE_RPC_RPC_H */

], eval "ac_cv_extern_$1=yes", eval "ac_cv_extern_$1=no")
])
# check if need to define variable
if test "`eval echo '$''{ac_cv_extern_'$1'}'`" = yes
then
  AC_DEFINE_UNQUOTED($ac_safe)
fi
])
dnl ======================================================================

dnl ######################################################################
dnl run AMU_CHECK_EXTERN on each argument given
dnl Usage: AMU_CHECK_EXTERNS(arg arg arg ...)
AC_DEFUN(AMU_CHECK_EXTERNS,
[
for ac_tmp_arg in $1
do
AMU_CHECK_EXTERN($ac_tmp_arg)
done
])
dnl ======================================================================


dnl ######################################################################
dnl Find if type 'fhandle' exists
AC_DEFUN(AMU_CHECK_FHANDLE,
[
AC_CACHE_CHECK(if plain fhandle type exists,
ac_cv_have_fhandle,
[
# try to compile a program which may have a definition for the type
# set to a default value
ac_cv_have_fhandle=no
# look for "struct nfs_fh"
if test "$ac_cv_have_fhandle" = no
then
AC_TRY_COMPILE_NFS(
[ fhandle a;
], ac_cv_have_fhandle=yes, ac_cv_have_fhandle=no)
fi

])
if test "$ac_cv_have_fhandle" != no
then
  AC_DEFINE(HAVE_FHANDLE)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl FIXED VERSION OF AUTOCONF 2.50 AC_CHECK_MEMBER.  g/cc will fail to check
dnl a member if the .member is itself a data structure, because you cannot
dnl compare, in C, a data structure against NULL; you can compare a native
dnl data type (int, char) or a pointer.  Solution: do what I did in my
dnl original member checking macro: try to take the address of the member.
dnl You can always take the address of anything.
dnl -Erez Zadok, Feb 6, 2002.
dnl
# AC_CHECK_MEMBER2(AGGREGATE.MEMBER,
#                 [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                 [INCLUDES])
# ---------------------------------------------------------
# AGGREGATE.MEMBER is for instance `struct passwd.pw_gecos', shell
# variables are not a valid argument.
AC_DEFUN([AC_CHECK_MEMBER2],
[AS_LITERAL_IF([$1], [],
               [AC_FATAL([$0: requires literal arguments])])dnl
m4_if(m4_regexp([$1], [\.]), -1,
      [AC_FATAL([$0: Did not see any dot in `$1'])])dnl
AS_VAR_PUSHDEF([ac_Member], [ac_cv_member_$1])dnl
dnl Extract the aggregate name, and the member name
AC_CACHE_CHECK([for $1], ac_Member,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT([$4])],
[dnl AGGREGATE ac_aggr;
static m4_patsubst([$1], [\..*]) ac_aggr;
dnl ac_aggr.MEMBER;
if (&(ac_aggr.m4_patsubst([$1], [^[^.]*\.])))
return 0;])],
                [AS_VAR_SET(ac_Member, yes)],
                [AS_VAR_SET(ac_Member, no)])])
AS_IF([test AS_VAR_GET(ac_Member) = yes], [$2], [$3])dnl
AS_VAR_POPDEF([ac_Member])dnl
])# AC_CHECK_MEMBER

# AC_CHECK_MEMBERS2([AGGREGATE.MEMBER, ...],
#                  [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND]
#                  [INCLUDES])
# ---------------------------------------------------------
# The first argument is an m4 list.
AC_DEFUN([AC_CHECK_MEMBERS2],
[m4_foreach([AC_Member], [$1],
  [AC_CHECK_MEMBER2(AC_Member,
         [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_[]AC_Member), 1,
                            [Define if `]m4_patsubst(AC_Member,
                                                     [^[^.]*\.])[' is
                             member of `]m4_patsubst(AC_Member, [\..*])['.])
$2],
                 [$3],
                 [$4])])])


dnl ######################################################################
dnl find if structure $1 has field field $2
AC_DEFUN(AMU_CHECK_FIELD,
[
AC_CHECK_MEMBERS2($1, , ,[
AMU_MOUNT_HEADERS(
[
/* now set the typedef */
#ifdef HAVE_STRUCT_MNTENT
typedef struct mntent mntent_t;
#else /* not HAVE_STRUCT_MNTENT */
# ifdef HAVE_STRUCT_MNTTAB
typedef struct mnttab mntent_t;
# endif /*  HAVE_STRUCT_MNTTAB */
#endif /* not HAVE_STRUCT_MNTENT */

/*
 * for various filesystem specific mount arguments
 */

#ifdef HAVE_SYS_FS_PC_FS_H
# include <sys/fs/pc_fs.h>
#endif /* HAVE_SYS_FS_PC_FS_H */
#ifdef HAVE_MSDOSFS_MSDOSFSMOUNT_H
# include <msdosfs/msdosfsmount.h>
#endif /* HAVE_MSDOSFS_MSDOSFSMOUNT_H */

#ifdef HAVE_SYS_FS_EFS_CLNT_H
# include <sys/fs/efs_clnt.h>
#endif /* HAVE_SYS_FS_EFS_CLNT_H */
#ifdef HAVE_SYS_FS_XFS_CLNT_H
# include <sys/fs/xfs_clnt.h>
#endif /* HAVE_SYS_FS_XFS_CLNT_H */
#ifdef HAVE_SYS_FS_UFS_MOUNT_H
# include <sys/fs/ufs_mount.h>
#endif /* HAVE_SYS_FS_UFS_MOUNT_H */
#ifdef HAVE_SYS_FS_AUTOFS_H
# include <sys/fs/autofs.h>
#endif /* HAVE_SYS_FS_AUTOFS_H */
#ifdef HAVE_RPCSVC_AUTOFS_PROT_H
# include <rpcsvc/autofs_prot.h>
#else  /* not HAVE_RPCSVC_AUTOFS_PROT_H */
# ifdef HAVE_SYS_FS_AUTOFS_PROT_H
#  include <sys/fs/autofs_prot.h>
# endif /* HAVE_SYS_FS_AUTOFS_PROT_H */
#endif /* not HAVE_RPCSVC_AUTOFS_PROT_H */
#ifdef HAVE_HSFS_HSFS_H
# include <hsfs/hsfs.h>
#endif /* HAVE_HSFS_HSFS_H */

#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
#endif /* HAVE_IFADDRS_H */

])
])
])
dnl ======================================================================


dnl ######################################################################
dnl check if a filesystem exists (if any of its header files exist).
dnl Usage: AC_CHECK_FS_HEADERS(<headers>..., <fs>, [<fssymbol>])
dnl Check if any of the headers <headers> exist.  If any exist, then
dnl define HAVE_FS_<fs>.  If <fssymbol> exits, then define
dnl HAVE_FS_<fssymbol> instead...
AC_DEFUN(AMU_CHECK_FS_HEADERS,
[
# find what name to give to the fs
if test -n "$3"
then
  ac_fs_name=$3
else
  ac_fs_name=$2
fi
# store variable name of fs
ac_upcase_fs_name=`echo $2 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_fs_headers_safe=HAVE_FS_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for $ac_fs_name filesystem in <$1>,
ac_cv_fs_header_$ac_fs_name,
[
# define to "no" by default
eval "ac_cv_fs_header_$ac_fs_name=no"
# and look to see if it was found
AC_CHECK_HEADERS($1,
[ eval "ac_cv_fs_header_$ac_fs_name=yes"
  break
])])
# check if need to define variable
if test "`eval echo '$''{ac_cv_fs_header_'$ac_fs_name'}'`" = yes
then
  AC_DEFINE_UNQUOTED($ac_fs_headers_safe)
# append ops_<fs>.o object to AMD_FS_OBJS for automatic compilation
# if first time we add something to this list, then also tell autoconf
# to replace instances of it in Makefiles.
  if test -z "$AMD_FS_OBJS"
  then
    AMD_FS_OBJS="ops_${ac_fs_name}.o"
    AC_SUBST(AMD_FS_OBJS)
  else
    # since this object file could have already been added before
    # we need to ensure we do not add it twice.
    case "${AMD_FS_OBJS}" in
      *ops_${ac_fs_name}.o* ) ;;
      * )
        AMD_FS_OBJS="$AMD_FS_OBJS ops_${ac_fs_name}.o"
      ;;
    esac
  fi
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check if a filesystem type exists (if its header files exist)
dnl Usage: AC_CHECK_FS_MNTENT(<filesystem>, [<fssymbol>])
dnl
dnl Check in some headers for MNTTYPE_<filesystem> macro.  If that exist,
dnl then define HAVE_FS_<filesystem>.  If <fssymbol> exits, then define
dnl HAVE_FS_<fssymbol> instead...
AC_DEFUN(AMU_CHECK_FS_MNTENT,
[
# find what name to give to the fs
if test -n "$2"
then
  ac_fs_name=$2
  ac_fs_as_name=" (from: $1)"
else
  ac_fs_name=$1
  ac_fs_as_name=""
fi
# store variable name of filesystem
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=HAVE_FS_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for $ac_fs_name$ac_fs_as_name mntent definition,
ac_cv_fs_$ac_fs_name,
[
# assume not found
eval "ac_cv_fs_$ac_fs_name=no"
for ac_fs_tmp in $1
do
  ac_upcase_fs_symbol=`echo $ac_fs_tmp | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`

  # first look for MNTTYPE_*
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MNTTYPE_$ac_upcase_fs_symbol
    yes
#endif /* MNTTYPE_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_fs_$ac_fs_name=yes"], [eval "ac_cv_fs_$ac_fs_name=no"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" != no
  then
    break
  fi

  # now try to look for MOUNT_ macro
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MOUNT_$ac_upcase_fs_symbol
    yes
#endif /* MOUNT_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_fs_$ac_fs_name=yes"], [eval "ac_cv_fs_$ac_fs_name=no"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" != no
  then
    break
  fi

  # now try to look for MNT_ macro
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MNT_$ac_upcase_fs_symbol
    yes
#endif /* MNT_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_fs_$ac_fs_name=yes"], [eval "ac_cv_fs_$ac_fs_name=no"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" != no
  then
    break
  fi

  # now try to look for GT_ macro (ultrix)
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef GT_$ac_upcase_fs_symbol
    yes
#endif /* GT_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_fs_$ac_fs_name=yes"], [eval "ac_cv_fs_$ac_fs_name=no"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" != no
  then
    break
  fi

  # look for a loadable filesystem module (linux)
  if test -f /lib/modules/$host_os_version/fs/$ac_fs_tmp.o
  then
    eval "ac_cv_fs_$ac_fs_name=yes"
    break
  fi

  # look for a loadable filesystem module (linux 2.4+)
  if test -f /lib/modules/$host_os_version/kernel/fs/$ac_fs_tmp/$ac_fs_tmp.o
  then
    eval "ac_cv_fs_$ac_fs_name=yes"
    break
  fi

  # look for a loadable filesystem module (linux redhat-5.1)
  if test -f /lib/modules/preferred/fs/$ac_fs_tmp.o
  then
    eval "ac_cv_fs_$ac_fs_name=yes"
    break
  fi

  # in addition look for statically compiled filesystem (linux)
  if egrep "[[^a-zA-Z0-9_]]$ac_fs_tmp$" /proc/filesystems >/dev/null 2>&1
  then
    eval "ac_cv_fs_$ac_fs_name=yes"
    break
  fi

  if test "$ac_fs_tmp" = "nfs3" -a "$ac_cv_header_linux_nfs_mount_h" = "yes"
  then
    # hack hack hack
    # in 6.1, which has fallback to v2/udp, we might want
    # to always use version 4.
    # in 6.0 we do not have much choice
    #
    let nfs_mount_version="`grep NFS_MOUNT_VERSION /usr/include/linux/nfs_mount.h | awk '{print $''3;}'`"
    if test $nfs_mount_version -ge 4
    then
      eval "ac_cv_fs_$ac_fs_name=yes"
      break
    fi
  fi

  # run a test program for bsdi3
  AC_TRY_RUN(
  [
#include <sys/param.h>
#include <sys/mount.h>
main()
{
  int i;
  struct vfsconf vf;
  i = getvfsbyname("$ac_fs_tmp", &vf);
  if (i < 0)
    exit(1);
  else
    exit(0);
}
  ], [eval "ac_cv_fs_$ac_fs_name=yes"
      break
     ]
  )

done
])
# check if need to define variable
if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" = yes
then
  AC_DEFINE_UNQUOTED($ac_safe)
# append ops_<fs>.o object to AMD_FS_OBJS for automatic compilation
# if first time we add something to this list, then also tell autoconf
# to replace instances of it in Makefiles.
  if test -z "$AMD_FS_OBJS"
  then
    AMD_FS_OBJS="ops_${ac_fs_name}.o"
    AC_SUBST(AMD_FS_OBJS)
  else
    # since this object file could have already been added before
    # we need to ensure we do not add it twice.
    case "${AMD_FS_OBJS}" in
      *ops_${ac_fs_name}.o* ) ;;
      * )
        AMD_FS_OBJS="$AMD_FS_OBJS ops_${ac_fs_name}.o"
      ;;
    esac
  fi
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Do we have a GNUish getopt
AC_DEFUN(AMU_CHECK_GNU_GETOPT,
[
AC_CACHE_CHECK([for GNU getopt], ac_cv_sys_gnu_getopt, [
AC_TRY_RUN([
#include <stdio.h>
#include <unistd.h>
int main()
{
   int argc = 3;
   char *argv[] = { "actest", "arg", "-x", NULL };
   int c;
   FILE* rf;
   int isGNU = 0;

   rf = fopen("conftestresult", "w");
   if (rf == NULL) exit(1);

   while ( (c = getopt(argc, argv, "x")) != -1 ) {
       switch ( c ) {
          case 'x':
	     isGNU=1;
             break;
          default:
             exit(1);
       }
   }
   fprintf(rf, isGNU ? "yes" : "no");
   exit(0);
}
],[
ac_cv_sys_gnu_getopt="`cat conftestresult`"
],[
AC_MSG_ERROR(could not test for getopt())
])
])
if test "$ac_cv_sys_gnu_getopt" = "yes"
then
    AC_DEFINE(HAVE_GNU_GETOPT)
fi
])


dnl ######################################################################
dnl Define mount type to hide amd mounts from df(1)
dnl
dnl This has to be determined individually per OS.  Depending on whatever
dnl mount options are defined in the system header files such as
dnl MNTTYPE_IGNORE or MNTTYPE_AUTO, or others does not work: some OSs define
dnl some of these then use other stuff; some do not define them at all in
dnl the headers, but still use it; and more.  After a long attempt to get
dnl this automatically configured, I came to the conclusion that the semi-
dnl automatic per-host-os determination here is the best.
dnl
AC_DEFUN(AMU_CHECK_HIDE_MOUNT_TYPE,
[
AC_CACHE_CHECK(for mount type to hide from df,
ac_cv_hide_mount_type,
[
case "${host_os}" in
	irix* | hpux* )
		ac_cv_hide_mount_type="ignore"
		;;
	sunos4* )
		ac_cv_hide_mount_type="auto"
		;;
	* )
		ac_cv_hide_mount_type="nfs"
		;;
esac
])
AC_DEFINE_UNQUOTED(HIDE_MOUNT_TYPE, "$ac_cv_hide_mount_type")
])
dnl ======================================================================


dnl a bug-fixed version of autoconf 2.12.
dnl first try to link library without $5, and only of that failed,
dnl try with $5 if specified.
dnl it adds $5 to $LIBS if it was needed -Erez.
dnl AC_CHECK_LIB2(LIBRARY, FUNCTION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl              [, OTHER-LIBRARIES]]])
AC_DEFUN(AMU_CHECK_LIB2,
[AC_MSG_CHECKING([for $2 in -l$1])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $1 defining function $2, not
dnl just for library $1.  Separate tests with the same $1 and different $2s
dnl may have different results.
ac_lib_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"

# first try with base library, without auxiliary library
LIBS="-l$1 $LIBS"
AC_TRY_LINK(dnl
ifelse([$2], [main], , dnl Avoid conflicting decl of main.
[/* Override any gcc2 internal prototype to avoid an error.  */
]
[/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char $2();
]),
	    [$2()],
	    eval "ac_cv_lib_$ac_lib_var=\"$1\"",
	    eval "ac_cv_lib_$ac_lib_var=no")

# if OK, set to no auxiliary library, else try auxiliary library
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = no"; then
 LIBS="-l$1 $5 $LIBS"
 AC_TRY_LINK(dnl
 ifelse([$2], [main], , dnl Avoid conflicting decl of main.
 [/* Override any gcc2 internal prototype to avoid an error.  */
 ]
 [/* We use char because int might match the return type of a gcc2
     builtin and then its argument prototype would still apply.  */
 char $2();
 ]),
 	    [$2()],
 	    eval "ac_cv_lib_$ac_lib_var=\"$1 $5\"",
 	    eval "ac_cv_lib_$ac_lib_var=no")
fi

LIBS="$ac_save_LIBS"
])dnl
ac_tmp="`eval echo '$''{ac_cv_lib_'$ac_lib_var'}'`"
if test "${ac_tmp}" != no; then
  AC_MSG_RESULT(-l$ac_tmp)
  ifelse([$3], ,
[
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[[^a-zA-Z0-9_]]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`

  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$ac_tmp $LIBS"
], [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi

])


dnl ######################################################################
dnl check if a map exists (if some library function exists).
dnl Usage: AC_CHECK_MAP_FUNCS(<functions>..., <map>, [<mapsymbol>])
dnl Check if any of the functions <functions> exist.  If any exist, then
dnl define HAVE_MAP_<map>.  If <mapsymbol> exits, then defined
dnl HAVE_MAP_<mapsymbol> instead...
AC_DEFUN(AMU_CHECK_MAP_FUNCS,
[
# find what name to give to the map
if test -n "$3"
then
  ac_map_name=$3
else
  ac_map_name=$2
fi
# store variable name of map
ac_upcase_map_name=`echo $2 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=HAVE_MAP_$ac_upcase_map_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for $ac_map_name maps,
ac_cv_map_$ac_map_name,
[
# define to "no" by default
eval "ac_cv_map_$ac_map_name=no"
# and look to see if it was found
AC_CHECK_FUNCS($1,
[
  eval "ac_cv_map_$ac_map_name=yes"
  break
])])
# check if need to define variable
if test "`eval echo '$''{ac_cv_map_'$ac_map_name'}'`" = yes
then
  AC_DEFINE_UNQUOTED($ac_safe)
# append info_<map>.o object to AMD_INFO_OBJS for automatic compilation
# if first time we add something to this list, then also tell autoconf
# to replace instances of it in Makefiles.
  if test -z "$AMD_INFO_OBJS"
  then
    AMD_INFO_OBJS="info_${ac_map_name}.o"
    AC_SUBST(AMD_INFO_OBJS)
  else
    AMD_INFO_OBJS="$AMD_INFO_OBJS info_${ac_map_name}.o"
  fi
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find CDFS-specific mount(2) options (hex numbers)
dnl Usage: AMU_CHECK_MNT2_CDFS_OPT(<fs>)
dnl Check if there is an entry for MS_<fs> or M_<fs> in sys/mntent.h or
dnl mntent.h, then define MNT2_CDFS_OPT_<fs> to the hex number.
AC_DEFUN(AMU_CHECK_MNT2_CDFS_OPT,
[
# what name to give to the fs
ac_fs_name=$1
# store variable name of fs
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=MNT2_CDFS_OPT_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for CDFS-specific mount(2) option $ac_fs_name,
ac_cv_mnt2_cdfs_opt_$ac_fs_name,
[
# undefine by default
eval "ac_cv_mnt2_cdfs_opt_$ac_fs_name=notfound"
value=notfound

# first, try MS_* (most systems).  Must be the first test!
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, MS_$ac_upcase_fs_name)
fi

# if failed, try MNT_* (bsd44 systems)
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, MNT_$ac_upcase_fs_name)
fi

# if failed, try MS_*  as an integer (linux systems)
if test "$value" = notfound
then
AMU_EXPAND_CPP_INT(
AMU_MOUNT_HEADERS
, MS_$ac_upcase_fs_name)
fi

# If failed try M_* (must be last test since svr4 systems define M_DATA etc.
# in <sys/stream.h>
# This test was off for now, because of the conflicts with other systems.
# but I turned it back on by faking the inclusion of <sys/stream.h> already.
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
#ifndef _sys_stream_h
# define _sys_stream_h
#endif /* not _sys_stream_h */
#ifndef _SYS_STREAM_H
# define _SYS_STREAM_H
#endif	/* not _SYS_STREAM_H */
AMU_MOUNT_HEADERS
, M_$ac_upcase_fs_name)
fi

# if failed, try ISOFSMNT_* as a hex (bsdi4 systems)
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, ISOFSMNT_$ac_upcase_fs_name)
fi

# set cache variable to value
eval "ac_cv_mnt2_cdfs_opt_$ac_fs_name=$value"
])
# outside cache check, if ok, define macro
ac_tmp=`eval echo '$''{ac_cv_mnt2_cdfs_opt_'$ac_fs_name'}'`
if test "${ac_tmp}" != notfound
then
  AC_DEFINE_UNQUOTED($ac_safe, $ac_tmp)
fi
])
dnl ======================================================================

dnl ######################################################################
dnl run AMU_CHECK_MNT2_CDFS_OPT on each argument given
dnl Usage: AMU_CHECK_MNT2_CDFS_OPTS(arg arg arg ...)
AC_DEFUN(AMU_CHECK_MNT2_CDFS_OPTS,
[
for ac_tmp_arg in $1
do
AMU_CHECK_MNT2_CDFS_OPT($ac_tmp_arg)
done
])
dnl ======================================================================


dnl ######################################################################
dnl Find generic mount(2) options (hex numbers)
dnl Usage: AMU_CHECK_MNT2_GEN_OPT(<fs>)
dnl Check if there is an entry for MS_<fs>, MNT_<fs>, or M_<fs>
dnl (in that order) in mntent.h, sys/mntent.h, or mount.h...
dnl then define MNT2_GEN_OPT_<fs> to the hex number.
AC_DEFUN(AMU_CHECK_MNT2_GEN_OPT,
[
# what name to give to the fs
ac_fs_name=$1
# store variable name of fs
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=MNT2_GEN_OPT_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for generic mount(2) option $ac_fs_name,
ac_cv_mnt2_gen_opt_$ac_fs_name,
[
# undefine by default
eval "ac_cv_mnt2_gen_opt_$ac_fs_name=notfound"
value=notfound

# first, try MS_* (most systems).  Must be the first test!
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, MS_$ac_upcase_fs_name)
fi

# if failed, try MNT_* (bsd44 systems)
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, MNT_$ac_upcase_fs_name)
fi

# if failed, try MS_*  as an integer (linux systems)
if test "$value" = notfound
then
AMU_EXPAND_CPP_INT(
AMU_MOUNT_HEADERS
, MS_$ac_upcase_fs_name)
fi

# If failed try M_* (must be last test since svr4 systems define M_DATA etc.
# in <sys/stream.h>
# This test was off for now, because of the conflicts with other systems.
# but I turned it back on by faking the inclusion of <sys/stream.h> already.
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
#ifndef _sys_stream_h
# define _sys_stream_h
#endif /* not _sys_stream_h */
#ifndef _SYS_STREAM_H
# define _SYS_STREAM_H
#endif	/* not _SYS_STREAM_H */
AMU_MOUNT_HEADERS
, M_$ac_upcase_fs_name)
fi

# set cache variable to value
eval "ac_cv_mnt2_gen_opt_$ac_fs_name=$value"
])
# outside cache check, if ok, define macro
ac_tmp=`eval echo '$''{ac_cv_mnt2_gen_opt_'$ac_fs_name'}'`
if test "${ac_tmp}" != notfound
then
  AC_DEFINE_UNQUOTED($ac_safe, $ac_tmp)
fi
])
dnl ======================================================================

dnl ######################################################################
dnl run AMU_CHECK_MNT2_GEN_OPT on each argument given
dnl Usage: AMU_CHECK_MNT2_GEN_OPTS(arg arg arg ...)
AC_DEFUN(AMU_CHECK_MNT2_GEN_OPTS,
[
for ac_tmp_arg in $1
do
AMU_CHECK_MNT2_GEN_OPT($ac_tmp_arg)
done
])
dnl ======================================================================


dnl ######################################################################
dnl Find NFS-specific mount(2) options (hex numbers)
dnl Usage: AMU_CHECK_MNT2_NFS_OPT(<fs>)
dnl Check if there is an entry for NFSMNT_<fs> in sys/mntent.h or
dnl mntent.h, then define MNT2_NFS_OPT_<fs> to the hex number.
AC_DEFUN(AMU_CHECK_MNT2_NFS_OPT,
[
# what name to give to the fs
ac_fs_name=$1
# store variable name of fs
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=MNT2_NFS_OPT_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for NFS-specific mount(2) option $ac_fs_name,
ac_cv_mnt2_nfs_opt_$ac_fs_name,
[
# undefine by default
eval "ac_cv_mnt2_nfs_opt_$ac_fs_name=notfound"
value=notfound

# first try NFSMNT_* (most systems)
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, NFSMNT_$ac_upcase_fs_name)
fi

# next try NFS_MOUNT_* (linux)
if test "$value" = notfound
then
AMU_EXPAND_CPP_HEX(
AMU_MOUNT_HEADERS
, NFS_MOUNT_$ac_upcase_fs_name)
fi

# set cache variable to value
eval "ac_cv_mnt2_nfs_opt_$ac_fs_name=$value"
])
# outside cache check, if ok, define macro
ac_tmp=`eval echo '$''{ac_cv_mnt2_nfs_opt_'$ac_fs_name'}'`
if test "${ac_tmp}" != notfound
then
  AC_DEFINE_UNQUOTED($ac_safe, $ac_tmp)
fi
])
dnl ======================================================================

dnl ######################################################################
dnl run AMU_CHECK_MNT2_NFS_OPT on each argument given
dnl Usage: AMU_CHECK_MNT2_NFS_OPTS(arg arg arg ...)
AC_DEFUN(AMU_CHECK_MNT2_NFS_OPTS,
[
for ac_tmp_arg in $1
do
AMU_CHECK_MNT2_NFS_OPT($ac_tmp_arg)
done
])
dnl ======================================================================


dnl ######################################################################
dnl Find name of mount table file, and define it as MNTTAB_FILE_NAME
dnl
dnl Solaris defines MNTTAB as /etc/mnttab, the file where /sbin/mount
dnl stores its cache of mounted filesystems.  But under SunOS, the same
dnl macro MNTTAB, is defined as the _source_ of filesystems to mount, and
dnl is set to /etc/fstab.  That is why I have to first check out
dnl if MOUNTED exists, and if not, check for the MNTTAB macro.
dnl
AC_DEFUN(AMU_CHECK_MNTTAB_FILE_NAME,
[
AC_CACHE_CHECK(for name of mount table file name,
ac_cv_mnttab_file_name,
[
# expand cpp value for MNTTAB
AMU_EXPAND_CPP_STRING(
AMU_MOUNT_HEADERS(
[
/* see M4 comment at the top of the definition of this macro */
#ifdef MOUNTED
# define _MNTTAB_FILE_NAME MOUNTED
# else /* not MOUNTED */
# ifdef MNTTAB
#  define _MNTTAB_FILE_NAME MNTTAB
# endif /* MNTTAB */
#endif /* not MOUNTED */
]),
_MNTTAB_FILE_NAME,
[ ac_cv_mnttab_file_name=$value
],
[
ac_cv_mnttab_file_name=notfound
# check explicitly for /etc/mnttab
if test "$ac_cv_mnttab_file_name" = notfound
then
  if test -f /etc/mnttab
  then
    ac_cv_mnttab_file_name="/etc/mnttab"
  fi
fi
# check explicitly for /etc/mtab
if test "$ac_cv_mnttab_file_name" = notfound
then
  if test -f /etc/mtab
  then
    ac_cv_mnttab_file_name="/etc/mtab"
  fi
fi
])
])
# test value and create macro as needed
if test "$ac_cv_mnttab_file_name" != notfound
then
  AC_DEFINE_UNQUOTED(MNTTAB_FILE_NAME, "$ac_cv_mnttab_file_name")
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check if the mount table is kept in a file or in the kernel.
AC_DEFUN(AMU_CHECK_MNTTAB_LOCATION,
[
AMU_CACHE_CHECK_DYNAMIC(where mount table is kept,
ac_cv_mnttab_location,
[
# assume location is on file
ac_cv_mnttab_location=file
AC_CHECK_FUNCS(mntctl getmntinfo getmountent,
ac_cv_mnttab_location=kernel)
# Solaris 8 Beta Refresh and up use the mntfs pseudo filesystem to store the
# mount table in kernel (cf. mnttab(4): the MS_NOMNTTAB option in
# <sys/mount.h> inhibits that a mount shows up there and thus can be used to
# check for the in-kernel mount table
if test "$ac_cv_mnt2_gen_opt_nomnttab" != notfound
then
  ac_cv_mnttab_location=kernel
fi
])
if test "$ac_cv_mnttab_location" = file
then
 AC_DEFINE(MOUNT_TABLE_ON_FILE)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check the string type of the name of a filesystem mount table entry
dnl option.
dnl Usage: AMU_CHECK_MNTTAB_OPT(<fs>)
dnl Check if there is an entry for MNTOPT_<fs> in sys/mntent.h or mntent.h
dnl define MNTTAB_OPT_<fs> to the string name (e.g., "ro").
AC_DEFUN(AMU_CHECK_MNTTAB_OPT,
[
# what name to give to the fs
ac_fs_name=$1
# store variable name of fs
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=MNTTAB_OPT_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for mount table option $ac_fs_name,
ac_cv_mnttab_opt_$ac_fs_name,
[
# undefine by default
eval "ac_cv_mnttab_opt_$ac_fs_name=notfound"
# and look to see if it was found
AMU_EXPAND_CPP_STRING(
AMU_MOUNT_HEADERS
, MNTOPT_$ac_upcase_fs_name)
# set cache variable to value
if test "${value}" != notfound
then
  eval "ac_cv_mnttab_opt_$ac_fs_name=\\\"$value\\\""
else
  eval "ac_cv_mnttab_opt_$ac_fs_name=$value"
fi
dnl DO NOT CHECK FOR MNT_* b/c bsd44 systems don't use /etc/mnttab,
])
# outside cache check, if ok, define macro
ac_tmp=`eval echo '$''{ac_cv_mnttab_opt_'$ac_fs_name'}'`
if test "${ac_tmp}" != notfound
then
  AC_DEFINE_UNQUOTED($ac_safe, $ac_tmp)
fi
])
dnl ======================================================================

dnl ######################################################################
dnl run AMU_CHECK_MNTTAB_OPT on each argument given
dnl Usage: AMU_CHECK_MNTTAB_OPTS(arg arg arg ...)
AC_DEFUN(AMU_CHECK_MNTTAB_OPTS,
[
for ac_tmp_arg in $1
do
AMU_CHECK_MNTTAB_OPT($ac_tmp_arg)
done
])
dnl ======================================================================


dnl ######################################################################
dnl check style of accessing the mount table file
AC_DEFUN(AMU_CHECK_MNTTAB_STYLE,
[
AC_CACHE_CHECK(mount table style,
ac_cv_style_mnttab,
[
# select the correct style for mount table manipulation functions
case "${host_os_name}" in
	aix* )
			ac_cv_style_mnttab=aix ;;
	bsd* | bsdi* | freebsd* | netbsd* | openbsd* | darwin* | rhapsody* )
			ac_cv_style_mnttab=bsd ;;
	isc3* )
			ac_cv_style_mnttab=isc3 ;;
	mach3* )
			ac_cv_style_mnttab=mach3 ;;
	osf* )
			ac_cv_style_mnttab=osf ;;
	svr4* | sysv4* | solaris2* | sunos5* | aoi* )
			ac_cv_style_mnttab=svr4 ;;
	ultrix* )
			ac_cv_style_mnttab=ultrix ;;
	* )
			ac_cv_style_mnttab=file ;;
esac
])
am_utils_link_files=${am_utils_link_files}libamu/mtabutil.c:conf/mtab/mtab_${ac_cv_style_mnttab}.c" "

# append mtab utilities object to LIBOBJS for automatic compilation
AC_LIBOBJ(mtabutil)
])
dnl ======================================================================


dnl ######################################################################
dnl check the string type of the name of a filesystem mount table entry.
dnl Usage: AC_CHECK_MNTTAB_TYPE(<fs>, [fssymbol])
dnl Check if there is an entry for MNTTYPE_<fs> in sys/mntent.h and mntent.h
dnl define MNTTAB_TYPE_<fs> to the string name (e.g., "nfs").  If <fssymbol>
dnl exist, then define MNTTAB_TYPE_<fssymbol> instead.  If <fssymbol> is
dnl defined, then <fs> can be a list of fs strings to look for.
dnl If no symbols have been defined, but the filesystem has been found
dnl earlier, then set the mount-table type to "<fs>" anyway...
AC_DEFUN(AMU_CHECK_MNTTAB_TYPE,
[
# find what name to give to the fs
if test -n "$2"
then
  ac_fs_name=$2
else
  ac_fs_name=$1
fi
# store variable name of fs
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
ac_safe=MNTTAB_TYPE_$ac_upcase_fs_name
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for mnttab name for $ac_fs_name filesystem,
ac_cv_mnttab_type_$ac_fs_name,
[
# undefine by default
eval "ac_cv_mnttab_type_$ac_fs_name=notfound"
# and look to see if it was found
for ac_fs_tmp in $1
do
  if test "$ac_fs_tmp" = "nfs3" -a "$ac_cv_fs_nfs3" = "yes" -a "$ac_cv_header_linux_nfs_h" = "yes"
  then
    eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_cv_mnttab_type_nfs\\\""
    break
  fi

  ac_upcase_fs_symbol=`echo $ac_fs_tmp | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' | tr -d '.'`

  # first look for MNTTYPE_*
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MNTTYPE_$ac_upcase_fs_symbol
    yes
#endif /* MNTTYPE_$ac_upcase_fs_symbol */
  ]),
  [ eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
  ])
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_mnttab_type_'$ac_fs_name'}'`" != notfound
  then
    break
  fi

  # look for a loadable filesystem module (linux)
  if test -f /lib/modules/$host_os_version/fs/$ac_fs_tmp.o
  then
    eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # look for a loadable filesystem module (linux 2.4+)
  if test -f /lib/modules/$host_os_version/kernel/fs/$ac_fs_tmp/$ac_fs_tmp.o
  then
    eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # look for a loadable filesystem module (linux redhat-5.1)
  if test -f /lib/modules/preferred/fs/$ac_fs_tmp.o
  then
    eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # next look for statically compiled filesystem (linux)
  if egrep "[[^a-zA-Z0-9_]]$ac_fs_tmp$" /proc/filesystems >/dev/null 2>&1
  then
    eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # then try to run a program that derefences a static array (bsd44)
  AMU_EXPAND_RUN_STRING(
  AMU_MOUNT_HEADERS(
  [
#ifndef INITMOUNTNAMES
# error INITMOUNTNAMES not defined
#endif /* not INITMOUNTNAMES */
  ]),
  [
  char const *namelist[] = INITMOUNTNAMES;
  if (argc > 1)
    printf("\"%s\"", namelist[MOUNT_$ac_upcase_fs_symbol]);
  ], [ eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$value\\\""
  ])
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_mnttab_type_'$ac_fs_name'}'`" != notfound
  then
    break
  fi

  # finally run a test program for bsdi3
  AC_TRY_RUN(
  [
#include <sys/param.h>
#include <sys/mount.h>
main()
{
  int i;
  struct vfsconf vf;
  i = getvfsbyname("$ac_fs_tmp", &vf);
  if (i < 0)
    exit(1);
  else
    exit(0);
}
  ], [eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
      break
     ]
  )

done

# check if not defined, yet the filesystem is defined
if test "`eval echo '$''{ac_cv_mnttab_type_'$ac_fs_name'}'`" = notfound
then
# this should test if $ac_cv_fs_<fsname> is "yes"
  if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" = yes ||
    test "`eval echo '$''{ac_cv_fs_header_'$ac_fs_name'}'`" = yes
  then
    eval "ac_cv_mnttab_type_$ac_fs_name=\\\"$ac_fs_name\\\""
  fi
fi
])
# check if need to define variable
ac_tmp=`eval echo '$''{ac_cv_mnttab_type_'$ac_fs_name'}'`
if test "$ac_tmp" != notfound
then
  AC_DEFINE_UNQUOTED($ac_safe, $ac_tmp)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check style of mounting filesystems
AC_DEFUN(AMU_CHECK_MOUNT_STYLE,
[
AC_CACHE_CHECK(style of mounting filesystems,
ac_cv_style_mount,
[
# select the correct style for mounting filesystems
case "${host_os_name}" in
	solaris1* | sunos[[34]]* | bsdi[[12]]* )
			ac_cv_style_mount=default ;;
	hpux[[6-9]]* | hpux10* )
			ac_cv_style_mount=hpux ;;
	svr4* | sysv4* | solaris* | sunos* | aoi* | hpux* )
			ac_cv_style_mount=svr4 ;;
	bsdi* )
			ac_cv_style_mount=bsdi3 ;;
	aix* )
			ac_cv_style_mount=aix ;;
	irix5* )
			ac_cv_style_mount=irix5 ;;
	irix* )
			ac_cv_style_mount=irix6 ;;
	isc3* )
			ac_cv_style_mount=isc3 ;;
	linux* )
			ac_cv_style_mount=linux ;;
	mach3* )
			ac_cv_style_mount=mach3 ;;
	stellix* )
			ac_cv_style_mount=stellix ;;
	* )	# no style needed.  Use default filesystem calls ala BSD
			ac_cv_style_mount=default ;;
esac
])
am_utils_mount_style_file="mountutil.c"
am_utils_link_files=${am_utils_link_files}libamu/${am_utils_mount_style_file}:conf/mount/mount_${ac_cv_style_mount}.c" "

# append mount utilities object to LIBOBJS for automatic compilation
AC_LIBOBJ(mountutil)
])
dnl ======================================================================


dnl ######################################################################
dnl check the mount system call trap needed to mount(2) a filesystem
AC_DEFUN(AMU_CHECK_MOUNT_TRAP,
[
AC_CACHE_CHECK(mount trap system-call style,
ac_cv_mount_trap,
[
# select the correct style to mount(2) a filesystem
case "${host_os_name}" in
	solaris1* | sunos[[34]]* )
		ac_cv_mount_trap=default ;;
	hpux[[6-9]]* | hpux10* )
		ac_cv_mount_trap=hpux ;;
	svr4* | sysv4* | solaris* | sunos* | aoi* | hpux* )
		ac_cv_mount_trap=svr4 ;;
	news4* | riscix* )
		ac_cv_mount_trap=news4 ;;
	linux* )
		ac_cv_mount_trap=linux ;;
	irix* )
		ac_cv_mount_trap=irix ;;
	aux* )
		ac_cv_mount_trap=aux ;;
	hcx* )
		ac_cv_mount_trap=hcx ;;
	rtu6* )
		ac_cv_mount_trap=rtu6 ;;
	dgux* )
		ac_cv_mount_trap=dgux ;;
	aix* )
		ac_cv_mount_trap=aix3 ;;
	mach2* | mach3* )
		ac_cv_mount_trap=mach3 ;;
	ultrix* )
		ac_cv_mount_trap=ultrix ;;
	isc3* )
		ac_cv_mount_trap=isc3 ;;
	stellix* )
		ac_cv_mount_trap=stellix ;;
	* )
		ac_cv_mount_trap=default ;;
esac
])
am_utils_mount_trap=$srcdir"/conf/trap/trap_"$ac_cv_mount_trap".h"
AC_SUBST_FILE(am_utils_mount_trap)
])
dnl ======================================================================


dnl ######################################################################
dnl check the string type of the name of a filesystem mount table entry.
dnl Usage: AC_CHECK_MOUNT_TYPE(<fs>, [fssymbol])
dnl Check if there is an entry for MNTTYPE_<fs> in sys/mntent.h and mntent.h
dnl define MOUNT_TYPE_<fs> to the string name (e.g., "nfs").  If <fssymbol>
dnl exist, then define MOUNT_TYPE_<fssymbol> instead.  If <fssymbol> is
dnl defined, then <fs> can be a list of fs strings to look for.
dnl If no symbols have been defined, but the filesystem has been found
dnl earlier, then set the mount-table type to "<fs>" anyway...
AC_DEFUN(AMU_CHECK_MOUNT_TYPE,
[
# find what name to give to the fs
if test -n "$2"
then
  ac_fs_name=$2
else
  ac_fs_name=$1
fi
# prepare upper-case name of filesystem
ac_upcase_fs_name=`echo $ac_fs_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
# check for cache and set it if needed
AMU_CACHE_CHECK_DYNAMIC(for mount(2) type/name for $ac_fs_name filesystem,
ac_cv_mount_type_$ac_fs_name,
[
# undefine by default
eval "ac_cv_mount_type_$ac_fs_name=notfound"
# and look to see if it was found
for ac_fs_tmp in $1
do

  ac_upcase_fs_symbol=`echo $ac_fs_tmp | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' | tr -d '.'`

  # first look for MNTTYPE_<fs>
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MNTTYPE_$ac_upcase_fs_symbol
    yes
#endif /* MNTTYPE_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_mount_type_$ac_fs_name=MNTTYPE_$ac_upcase_fs_symbol"],
      [eval "ac_cv_mount_type_$ac_fs_name=notfound"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`" != notfound
  then
    break
  fi

  # next look for MOUNT_<fs>
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MOUNT_$ac_upcase_fs_symbol
    yes
#endif /* MOUNT_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_mount_type_$ac_fs_name=MOUNT_$ac_upcase_fs_symbol"],
      [eval "ac_cv_mount_type_$ac_fs_name=notfound"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`" != notfound
  then
    break
  fi

  # next look for MNT_<fs>
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef MNT_$ac_upcase_fs_symbol
    yes
#endif /* MNT_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_mount_type_$ac_fs_name=MNT_$ac_upcase_fs_symbol"],
      [eval "ac_cv_mount_type_$ac_fs_name=notfound"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`" != notfound
  then
    break
  fi

  # next look for GT_<fs> (ultrix)
  AC_EGREP_CPP(yes,
  AMU_MOUNT_HEADERS(
  [
#ifdef GT_$ac_upcase_fs_symbol
    yes
#endif /* GT_$ac_upcase_fs_symbol */
  ]), [eval "ac_cv_mount_type_$ac_fs_name=GT_$ac_upcase_fs_symbol"],
      [eval "ac_cv_mount_type_$ac_fs_name=notfound"] )
  # check if need to terminate "for" loop
  if test "`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`" != notfound
  then
    break
  fi

  # look for a loadable filesystem module (linux)
  if test -f /lib/modules/$host_os_version/fs/$ac_fs_tmp.o
  then
    eval "ac_cv_mount_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # look for a loadable filesystem module (linux 2.4+)
  if test -f /lib/modules/$host_os_version/kernel/fs/$ac_fs_tmp/$ac_fs_tmp.o
  then
    eval "ac_cv_mount_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # look for a loadable filesystem module (linux redhat-5.1)
  if test -f /lib/modules/preferred/fs/$ac_fs_tmp.o
  then
    eval "ac_cv_mount_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # in addition look for statically compiled filesystem (linux)
  if egrep "[[^a-zA-Z0-9_]]$ac_fs_tmp$" /proc/filesystems >/dev/null 2>&1
  then
    eval "ac_cv_mount_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
    break
  fi

  # run a test program for bsdi3
  AC_TRY_RUN(
  [
#include <sys/param.h>
#include <sys/mount.h>
main()
{
  int i;
  struct vfsconf vf;
  i = getvfsbyname("$ac_fs_tmp", &vf);
  if (i < 0)
    exit(1);
  else
    exit(0);
}
  ], [eval "ac_cv_mount_type_$ac_fs_name=\\\"$ac_fs_tmp\\\""
      break
     ]
  )

done
# check if not defined, yet the filesystem is defined
if test "`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`" = notfound
then
# this should test if $ac_cv_fs_<fsname> is "yes"
  if test "`eval echo '$''{ac_cv_fs_'$ac_fs_name'}'`" = yes ||
    test "`eval echo '$''{ac_cv_fs_header_'$ac_fs_name'}'`" = yes
  then
    eval "ac_cv_mount_type_$ac_fs_name=MNTTYPE_$ac_upcase_fs_name"
  fi
fi
])
# end of cache check for ac_cv_mount_type_$ac_fs_name
# check if need to define variable
if test "`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`" != notfound
then
  ac_safe=MOUNT_TYPE_$ac_upcase_fs_name
  ac_tmp=`eval echo '$''{ac_cv_mount_type_'$ac_fs_name'}'`
  AC_DEFINE_UNQUOTED($ac_safe, $ac_tmp)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct printf-style type for the mount type in the mount()
dnl system call.
dnl If you change this one, you must also fix the check_mtype_type.m4.
AC_DEFUN(AMU_CHECK_MTYPE_PRINTF_TYPE,
[
AC_CACHE_CHECK(printf string to print type field of mount() call,
ac_cv_mtype_printf_type,
[
# select the correct printf type
case "${host_os_name}" in
	osf* | freebsd2* | bsdi2* | aix* | ultrix* )
		ac_cv_mtype_printf_type="%d" ;;
	irix3 | isc3 )
		ac_cv_mtype_printf_type="0x%x" ;;
	* )
		ac_cv_mtype_printf_type="%s" ;;
esac
])
AC_DEFINE_UNQUOTED(MTYPE_PRINTF_TYPE, "$ac_cv_mtype_printf_type")
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct type for the mount type in the mount() system call
dnl If you change this one, you must also fix the check_mtype_printf_type.m4.
AC_DEFUN(AMU_CHECK_MTYPE_TYPE,
[
AC_CACHE_CHECK(type of mount type field in mount() call,
ac_cv_mtype_type,
[
# select the correct type
case "${host_os_name}" in
	osf* | freebsd2* | bsdi2* | aix* | ultrix* )
		ac_cv_mtype_type=int ;;
	* )
		ac_cv_mtype_type="char *" ;;
esac
])
AC_DEFINE_UNQUOTED(MTYPE_TYPE, $ac_cv_mtype_type)
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct network transport type to use
AC_DEFUN(AMU_CHECK_NETWORK_TRANSPORT_TYPE,
[
AC_CACHE_CHECK(network transport type,
ac_cv_transport_type,
[
# select the correct type
case "${host_os_name}" in
	solaris1* | sunos[[34]]* | hpux[[6-9]]* | hpux10* )
		ac_cv_transport_type=sockets ;;
	solaris* | sunos* | hpux* )
		ac_cv_transport_type=tli ;;
	* )
		ac_cv_transport_type=sockets ;;
esac
])
am_utils_link_files=${am_utils_link_files}libamu/transputil.c:conf/transp/transp_${ac_cv_transport_type}.c" "

# append transport utilities object to LIBOBJS for automatic compilation
AC_LIBOBJ(transputil)
if test $ac_cv_transport_type = tli
then
  AC_DEFINE(HAVE_TRANSPORT_TYPE_TLI)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct way to dereference the address part of the nfs fhandle
AC_DEFUN(AMU_CHECK_NFS_FH_DREF,
[
AC_CACHE_CHECK(nfs file-handle address dereferencing style,
ac_cv_nfs_fh_dref_style,
[
# select the correct nfs address dereferencing style
case "${host_os}" in
	hpux[[6-9]]* | hpux10* )
		ac_cv_nfs_fh_dref_style=hpux ;;
	sunos3* )
		ac_cv_nfs_fh_dref_style=sunos3 ;;
	sunos4* | solaris1* )
		ac_cv_nfs_fh_dref_style=sunos4 ;;
	svr4* | sysv4* | solaris* | sunos* | hpux* )
		ac_cv_nfs_fh_dref_style=svr4 ;;
	bsd44* | bsdi2* | freebsd2.[[01]]* )
		ac_cv_nfs_fh_dref_style=bsd44 ;;
	# all new BSDs changed the type of the
	# filehandle in nfs_args from nfsv2fh_t to u_char.
	freebsd* | freebsdelf* | bsdi* | netbsd* | openbsd* | darwin* | rhapsody* )
		ac_cv_nfs_fh_dref_style=freebsd22 ;;
	aix[[1-3]]* | aix4.[[01]]* )
		ac_cv_nfs_fh_dref_style=aix3 ;;
	aix* )
		ac_cv_nfs_fh_dref_style=aix42 ;;
	irix* )
		ac_cv_nfs_fh_dref_style=irix ;;
	linux* )
		ac_cv_nfs_fh_dref_style=linux ;;
	isc3 )
		ac_cv_nfs_fh_dref_style=isc3 ;;
	osf[[1-3]]* )
		ac_cv_nfs_fh_dref_style=osf2 ;;
	osf* )
		ac_cv_nfs_fh_dref_style=osf4 ;;
	nextstep* )
		ac_cv_nfs_fh_dref_style=nextstep ;;
	* )
		ac_cv_nfs_fh_dref_style=default ;;
esac
])
am_utils_nfs_fh_dref=$srcdir"/conf/fh_dref/fh_dref_"$ac_cv_nfs_fh_dref_style".h"
AC_SUBST_FILE(am_utils_nfs_fh_dref)
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct way to dereference the hostname part of the nfs fhandle
AC_DEFUN(AMU_CHECK_NFS_HN_DREF,
[
AC_CACHE_CHECK(nfs hostname dereferencing style,
ac_cv_nfs_hn_dref_style,
[
# select the correct nfs address dereferencing style
case "${host_os_name}" in
	linux* )
		ac_cv_nfs_hn_dref_style=linux ;;
	isc3 )
		ac_cv_nfs_hn_dref_style=isc3 ;;
	* )
		ac_cv_nfs_hn_dref_style=default ;;
esac
])
am_utils_nfs_hn_dref=$srcdir"/conf/hn_dref/hn_dref_"$ac_cv_nfs_hn_dref_style".h"
AC_SUBST_FILE(am_utils_nfs_hn_dref)
])
dnl ======================================================================


dnl ######################################################################
dnl check if system has NFS protocol headers
AC_DEFUN(AMU_CHECK_NFS_PROT_HEADERS,
[
AC_CACHE_CHECK(location of NFS protocol header files,
ac_cv_nfs_prot_headers,
[
# select the correct style for mounting filesystems
case "${host_os}" in
	irix5* )
			ac_cv_nfs_prot_headers=irix5 ;;
	irix* )
			ac_cv_nfs_prot_headers=irix6 ;;
	sunos3* )
			ac_cv_nfs_prot_headers=sunos3 ;;
	sunos4* | solaris1* )
			ac_cv_nfs_prot_headers=sunos4 ;;
	sunos5.[[0-3]]* | solaris2.[[0-3]]* )
			ac_cv_nfs_prot_headers=sunos5_3 ;;
	sunos5.4* | solaris2.4* )
			ac_cv_nfs_prot_headers=sunos5_4 ;;
	sunos5.5* | solaris2.5* )
			ac_cv_nfs_prot_headers=sunos5_5 ;;
	sunos5.6* | solaris2.6* )
			ac_cv_nfs_prot_headers=sunos5_6 ;;
	sunos5.7* | solaris2.7* )
			ac_cv_nfs_prot_headers=sunos5_7 ;;
	sunos* | solaris* )
			ac_cv_nfs_prot_headers=sunos5_8 ;;
	bsdi2*)
			ac_cv_nfs_prot_headers=bsdi2 ;;
	bsdi* )
			ac_cv_nfs_prot_headers=bsdi3 ;;
	freebsd2* )
			ac_cv_nfs_prot_headers=freebsd2 ;;
	freebsd* | freebsdelf* )
			ac_cv_nfs_prot_headers=freebsd3 ;;
	netbsd1.[[0-2]]* )
			ac_cv_nfs_prot_headers=netbsd ;;
	netbsd1.3* )
			ac_cv_nfs_prot_headers=netbsd1_3 ;;
	netbsd* | netbsdelf* )
			ac_cv_nfs_prot_headers=netbsd1_4 ;;
	openbsd* )
			ac_cv_nfs_prot_headers=openbsd ;;
	hpux[[6-9]]* | hpux10* )
			ac_cv_nfs_prot_headers=hpux ;;
	hpux* )
			ac_cv_nfs_prot_headers=hpux11 ;;
	aix[[1-3]]* )
			ac_cv_nfs_prot_headers=aix3 ;;
	aix4.[[01]]* )
			ac_cv_nfs_prot_headers=aix4 ;;
	aix4.2* )
			ac_cv_nfs_prot_headers=aix4_2 ;;
	aix4.3* )
			ac_cv_nfs_prot_headers=aix4_3 ;;
	aix5.1* )
			ac_cv_nfs_prot_headers=aix5_1 ;;
	aix* )
			ac_cv_nfs_prot_headers=aix5_2 ;;
	osf[[1-3]]* )
			ac_cv_nfs_prot_headers=osf2 ;;
	osf4* )
			ac_cv_nfs_prot_headers=osf4 ;;
	osf* )
			ac_cv_nfs_prot_headers=osf5 ;;
	svr4* )
			ac_cv_nfs_prot_headers=svr4 ;;
	sysv4* )	# this is for NCR2 machines
			ac_cv_nfs_prot_headers=ncr2 ;;
	linux* )
			ac_cv_nfs_prot_headers=linux ;;
	nextstep* )
			ac_cv_nfs_prot_headers=nextstep ;;
	ultrix* )
			ac_cv_nfs_prot_headers=ultrix ;;
 	darwin* | rhapsody* )
 			ac_cv_nfs_prot_headers=darwin ;;
	* )
			ac_cv_nfs_prot_headers=default ;;
esac
])

# make sure correct header is linked in top build directory
am_utils_nfs_prot_file="amu_nfs_prot.h"
am_utils_link_files=${am_utils_link_files}${am_utils_nfs_prot_file}:conf/nfs_prot/nfs_prot_${ac_cv_nfs_prot_headers}.h" "

# define the name of the header to be included for other M4 macros
AC_DEFINE_UNQUOTED(AMU_NFS_PROTOCOL_HEADER, "${srcdir}/conf/nfs_prot/nfs_prot_${ac_cv_nfs_prot_headers}.h")

# set headers in a macro for Makefile.am files to use (for dependencies)
AMU_NFS_PROT_HEADER='${top_builddir}/'$am_utils_nfs_prot_file
AC_SUBST(AMU_NFS_PROT_HEADER)
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct way to dereference the address part of the nfs fhandle
AC_DEFUN(AMU_CHECK_NFS_SA_DREF,
[
AC_CACHE_CHECK(nfs address dereferencing style,
ac_cv_nfs_sa_dref_style,
[
# select the correct nfs address dereferencing style
case "${host_os}" in
	hpux[[6-9]]* | hpux10* | sunos[[34]]* | solaris1* )
		ac_cv_nfs_sa_dref_style=default ;;
	svr4* | sysv4* | solaris* | sunos* | hpux* )
		ac_cv_nfs_sa_dref_style=svr4 ;;
	386bsd* | bsdi1* )
		ac_cv_nfs_sa_dref_style=386bsd ;;
	bsd44* | bsdi* | freebsd* | netbsd* | openbsd* | darwin* | rhapsody* )
		ac_cv_nfs_sa_dref_style=bsd44 ;;
	linux* )
		ac_cv_nfs_sa_dref_style=linux ;;
	aix* )
		ac_cv_nfs_sa_dref_style=aix3 ;;
	aoi* )
		ac_cv_nfs_sa_dref_style=aoi ;;
	isc3 )
		ac_cv_nfs_sa_dref_style=isc3 ;;
	* )
		ac_cv_nfs_sa_dref_style=default ;;
esac
])
am_utils_nfs_sa_dref=$srcdir"/conf/sa_dref/sa_dref_"$ac_cv_nfs_sa_dref_style".h"
AC_SUBST_FILE(am_utils_nfs_sa_dref)
])
dnl ======================================================================


dnl ######################################################################
dnl check if need to turn on, off, or leave alone the NFS "noconn" option
AC_DEFUN(AMU_CHECK_NFS_SOCKET_CONNECTION,
[
AC_CACHE_CHECK(if to turn on/off noconn option,
ac_cv_nfs_socket_connection,
[
# set default to no-change
ac_cv_nfs_socket_connection=none
# select the correct style
case "${host_os}" in
	openbsd2.[[01]]* )
			ac_cv_nfs_socket_connection=noconn ;;
	openbsd* | freebsd* | freebsdelf* )
			ac_cv_nfs_socket_connection=conn ;;
esac
])
# set correct value
case "$ac_cv_nfs_socket_connection" in
	noconn )
		AC_DEFINE(USE_UNCONNECTED_NFS_SOCKETS)
		;;
	conn )
		AC_DEFINE(USE_CONNECTED_NFS_SOCKETS)
		;;
esac
])
dnl ======================================================================


dnl ######################################################################
dnl set OS libraries specific to an OS:
dnl libnsl/libsocket are needed only on solaris and some svr4 systems.
dnl Using a typical macro has proven unsuccesful, because on some other
dnl systems such as irix, including libnsl and or libsocket actually breaks
dnl lots of code.  So I am forced to use a special purpose macro that sets
dnl the libraries based on the OS.  Sigh.  -Erez.
AC_DEFUN(AMU_CHECK_OS_LIBS,
[
AC_CACHE_CHECK(for additional OS libraries,
ac_cv_os_libs,
[
# select the correct set of libraries to link with
case "${host_os_name}" in
	svr4* | sysv4* | solaris2* | sunos5* | aoi* )
			ac_cv_os_libs="-lsocket -lnsl" ;;
	* )
			ac_cv_os_libs=none ;;
esac
])
# set list of libraries to link with
if test "$ac_cv_os_libs" != none
then
  LIBS="$ac_cv_os_libs $LIBS"
fi

])
dnl ======================================================================


dnl ######################################################################
dnl check if a system needs to restart its signal handlers
AC_DEFUN(AMU_CHECK_RESTARTABLE_SIGNAL_HANDLER,
[
AC_CACHE_CHECK(if system needs to restart signal handlers,
ac_cv_restartable_signal_handler,
[
# select the correct systems to restart signal handlers
case "${host_os_name}" in
	svr3* | svr4* | sysv4* | solaris2* | sunos5* | aoi* | irix* )
			ac_cv_restartable_signal_handler=yes ;;
	* )
			ac_cv_restartable_signal_handler=no ;;
esac
])
# define REINSTALL_SIGNAL_HANDLER if need to
if test "$ac_cv_restartable_signal_handler" = yes
then
  AC_DEFINE(REINSTALL_SIGNAL_HANDLER)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check style of unmounting filesystems
AC_DEFUN(AMU_CHECK_UMOUNT_STYLE,
[
AC_CACHE_CHECK(style of unmounting filesystems,
ac_cv_style_umount,
[
# select the correct style for unmounting filesystems
case "${host_os_name}" in
	bsd44* | bsdi* | freebsd* | netbsd* | openbsd* | darwin* | rhapsody* )
			ac_cv_style_umount=bsd44 ;;
	osf* )
			ac_cv_style_umount=osf ;;
	* )
			ac_cv_style_umount=default ;;
esac
])
am_utils_umount_style_file="umount_fs.c"
am_utils_link_files=${am_utils_link_files}libamu/${am_utils_umount_style_file}:conf/umount/umount_${ac_cv_style_umount}.c" "

# append un-mount utilities object to LIBOBJS for automatic compilation
AC_LIBOBJ(umount_fs)
])
dnl ======================================================================


dnl ######################################################################
dnl check the unmount system call arguments needed for
AC_DEFUN(AMU_CHECK_UNMOUNT_ARGS,
[
AC_CACHE_CHECK(unmount system-call arguments,
ac_cv_unmount_args,
[
# select the correct style to mount(2) a filesystem
case "${host_os_name}" in
	aix* )
		ac_cv_unmount_args="mnt->mnt_passno, 0" ;;
	ultrix* )
		ac_cv_unmount_args="mnt->mnt_passno" ;;
	* )
		ac_cv_unmount_args="mnt->mnt_dir" ;;
esac
])
am_utils_unmount_args=$ac_cv_unmount_args
AC_SUBST(am_utils_unmount_args)
])
dnl ======================================================================


dnl ######################################################################
dnl check for the correct system call to unmount a filesystem.
AC_DEFUN(AMU_CHECK_UNMOUNT_CALL,
[
dnl make sure this one is called before [AC_CHECK_UNMOUNT_ARGS]
AC_BEFORE([$0], [AC_CHECK_UNMOUNT_ARGS])
AC_CACHE_CHECK(the system call to unmount a filesystem,
ac_cv_unmount_call,
[
# check for various unmount a filesystem calls
if test "$ac_cv_func_uvmount" = yes ; then
  ac_cv_unmount_call=uvmount
elif test "$ac_cv_func_unmount" = yes ; then
  ac_cv_unmount_call=unmount
elif test "$ac_cv_func_umount" = yes ; then
  ac_cv_unmount_call=umount
else
  ac_cv_unmount_call=no
fi
])
if test "$ac_cv_unmount_call" != no
then
  am_utils_unmount_call=$ac_cv_unmount_call
  AC_SUBST(am_utils_unmount_call)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Expand the value of a CPP macro into a printable hex number.
dnl Takes: header, macro, [action-if-found, [action-if-not-found]]
dnl It runs the header through CPP looking for a match between the macro
dnl and a string pattern, and if sucessful, it prints the string value out.
AC_DEFUN(AMU_EXPAND_CPP_HEX,
[
# we are looking for a regexp of a string
AC_EGREP_CPP(0x,
[$1]
$2,
value="notfound"
AC_TRY_RUN(
[
[$1]
main(argc)
int argc;
{
#ifdef $2
if (argc > 1)
  printf("0x%x", $2);
exit(0);
#else
# error no such option $2
#endif
exit(1);
}], value=`./conftest dummy 2>>config.log`, value="notfound", value="notfound")
,
value="notfound"
)
if test "$value" = notfound
then
  :
  $4
else
  :
  $3
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Expand the value of a CPP macro into a printable integer number.
dnl Takes: header, macro, [action-if-found, [action-if-not-found]]
dnl It runs the header through CPP looking for a match between the macro
dnl and a string pattern, and if sucessful, it prints the string value out.
AC_DEFUN(AMU_EXPAND_CPP_INT,
[
# we are looking for a regexp of an integer (must not start with 0 --- those
# are octals).
AC_EGREP_CPP(
[[1-9]][[0-9]]*,
[$1]
$2,
value="notfound"
AC_TRY_RUN(
[
[$1]
main(argc)
int argc;
{
#ifdef $2
if (argc > 1)
  printf("%d", $2);
exit(0);
#else
# error no such option $2
#endif
exit(1);
}], value=`./conftest dummy 2>>config.log`, value="notfound", value="notfound")
,
value="notfound"
)
if test "$value" = notfound
then
  :
  $4
else
  :
  $3
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Expand the value of a CPP macro into a printable string.
dnl Takes: header, macro, [action-if-found, [action-if-not-found]]
dnl It runs the header through CPP looking for a match between the macro
dnl and a string pattern, and if sucessful, it prints the string value out.
AC_DEFUN(AMU_EXPAND_CPP_STRING,
[
# we are looking for a regexp of a string
AC_EGREP_CPP(\".*\",
[$1]
$2,
value="notfound"
AC_TRY_RUN(
[
[$1]
main(argc)
int argc;
{
#ifdef $2
if (argc > 1)
  printf("%s", $2);
exit(0);
#else
# error no such option $2
#endif
exit(1);
}], value=`./conftest dummy 2>>config.log`, value="notfound", value="notfound")
,
value="notfound"
)
if test "$value" = notfound
then
  :
  $4
else
  :
  $3
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Run a program and print its output as a string
dnl Takes: (header, code-to-run, [action-if-found, [action-if-not-found]])
AC_DEFUN(AMU_EXPAND_RUN_STRING,
[
value="notfound"
AC_TRY_RUN(
[
$1
main(argc)
int argc;
{
$2
exit(0);
}], value=`./conftest dummy 2>>config.log`, value="notfound", value="notfound")
if test "$value" = notfound
then
  :
  $4
else
  :
  $3
fi
])
dnl ======================================================================


dnl ######################################################################
dnl find if "extern char *optarg" exists in headers
AC_DEFUN(AMU_EXTERN_OPTARG,
[
AC_CACHE_CHECK(if external definition for optarg[] exists,
ac_cv_extern_optarg,
[
# try to compile program that uses the variable
AC_TRY_COMPILE(
[
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif /* HAVE_STDIO_H */
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif /* HAVE_SYS_ERRNO_H */
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif /* HAVE_ERRNO_H */
],
[
char *cp = optarg;
], ac_cv_extern_optarg=yes, ac_cv_extern_optarg=no)
])
if test "$ac_cv_extern_optarg" = yes
then
  AC_DEFINE(HAVE_EXTERN_OPTARG)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl find if "extern char *sys_errlist[]" exist in headers
AC_DEFUN(AMU_EXTERN_SYS_ERRLIST,
[
AC_CACHE_CHECK(if external definition for sys_errlist[] exists,
ac_cv_extern_sys_errlist,
[
# try to locate pattern in header files
#pattern="(extern)?.*char.*sys_errlist.*\[\]"
pattern="(extern)?.*char.*sys_errlist.*"
AC_EGREP_CPP(${pattern},
[
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif /* HAVE_STDIO_H */
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif /* HAVE_SYS_ERRNO_H */
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif /* HAVE_ERRNO_H */
], ac_cv_extern_sys_errlist=yes, ac_cv_extern_sys_errlist=no)
])
# check if need to define variable
if test "$ac_cv_extern_sys_errlist" = yes
then
  AC_DEFINE(HAVE_EXTERN_SYS_ERRLIST)
fi
])
dnl ======================================================================


fdnl ######################################################################
dnl find if mntent_t field mnt_time exists and is of type "char *"
AC_DEFUN(AMU_FIELD_MNTENT_T_MNT_TIME_STRING,
[
AC_CACHE_CHECK(if mntent_t field mnt_time exist as type string,
ac_cv_field_mntent_t_mnt_time_string,
[
# try to compile a program
AC_TRY_COMPILE(
AMU_MOUNT_HEADERS(
[
/* now set the typedef */
#ifdef HAVE_STRUCT_MNTENT
typedef struct mntent mntent_t;
#else /* not HAVE_STRUCT_MNTENT */
# ifdef HAVE_STRUCT_MNTTAB
typedef struct mnttab mntent_t;
# else /* not HAVE_STRUCT_MNTTAB */
#  error XXX: could not find definition for struct mntent or struct mnttab!
# endif /* not HAVE_STRUCT_MNTTAB */
#endif /* not HAVE_STRUCT_MNTENT */
]),
[
mntent_t mtt;
char *cp = "test";
int i;
mtt.mnt_time = cp;
i = mtt.mnt_time[0];
], ac_cv_field_mntent_t_mnt_time_string=yes, ac_cv_field_mntent_t_mnt_time_string=no)
])
if test "$ac_cv_field_mntent_t_mnt_time_string" = yes
then
  AC_DEFINE(HAVE_MNTENT_T_MNT_TIME_STRING)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Check if we have as buggy hasmntopt() libc function
AC_DEFUN([AMU_FUNC_BAD_HASMNTOPT],
[
AC_CACHE_CHECK([for working hasmntopt], ac_cv_func_hasmntopt_working,
[AC_TRY_RUN(
AMU_MOUNT_HEADERS(
[[
#ifdef HAVE_MNTENT_H
/* some systems need <stdio.h> before <mntent.h> is included */
# ifdef HAVE_STDIO_H
#  include <stdio.h>
# endif /* HAVE_STDIO_H */
# include <mntent.h>
#endif /* HAVE_MNTENT_H */
#ifdef HAVE_SYS_MNTENT_H
# include <sys/mntent.h>
#endif /* HAVE_SYS_MNTENT_H */
#ifdef HAVE_SYS_MNTTAB_H
# include <sys/mnttab.h>
#endif /* HAVE_SYS_MNTTAB_H */
#if defined(HAVE_MNTTAB_H) && !defined(MNTTAB)
# include <mnttab.h>
#endif /* defined(HAVE_MNTTAB_H) && !defined(MNTTAB) */
#ifdef HAVE_STRUCT_MNTENT
typedef struct mntent mntent_t;
#else /* not HAVE_STRUCT_MNTENT */
# ifdef HAVE_STRUCT_MNTTAB
typedef struct mnttab mntent_t;
/* map struct mnttab field names to struct mntent field names */
#  define mnt_opts	mnt_mntopts
# endif /* not HAVE_STRUCT_MNTTAB */
#endif /* not HAVE_STRUCT_MNTENT */

int main()
{
  mntent_t mnt;
  char *tmp = NULL;

 /*
  * Test if hasmntopt will incorrectly find the string "soft", which
  * is part of the large "softlookup" function.
  */
  mnt.mnt_opts = "hard,softlookup,ro";

  if ((tmp = hasmntopt(&mnt, "soft")))
    exit(1);
  exit(0);
}
]]),
	[ac_cv_func_hasmntopt_working=yes],
	[ac_cv_func_hasmntopt_working=no]
)])
if test $ac_cv_func_hasmntopt_working = no
then
	AC_LIBOBJ([hasmntopt])
 	AC_DEFINE(HAVE_BAD_HASMNTOPT)
fi
])


dnl My version is similar to the one from Autoconf 2.52, but I also
dnl define HAVE_BAD_MEMCMP so that I can do smarter things to avoid
dnl linkage conflicts with bad memcmp versions that are in libc.
AC_DEFUN(AMU_FUNC_BAD_MEMCMP,
[
AC_FUNC_MEMCMP
if test "$ac_cv_func_memcmp_working" = no
then
AC_DEFINE(HAVE_BAD_MEMCMP)
fi
])


dnl Check for a yp_all() function that does not leak a file descriptor
dnl to the ypserv process.
AC_DEFUN(AMU_FUNC_BAD_YP_ALL,
[
AC_CACHE_CHECK(for a file-descriptor leakage clean yp_all,
ac_cv_func_yp_all_clean,
[
# clean by default
ac_cv_func_yp_all_clean=yes
# select the correct type
case "${host_os_name}" in
	irix* )
		ac_cv_func_yp_all_clean=no ;;
	linux* )
		# RedHat 5.1 systems with glibc glibc-2.0.7-19 or below
		# leak a UDP socket from yp_all()
		case "`cat /etc/redhat-release /dev/null 2>/dev/null`" in
			*5.1* )
				ac_cv_func_yp_all_clean=no ;;
		esac
esac
])
if test $ac_cv_func_yp_all_clean = no
then
  AC_DEFINE(HAVE_BAD_YP_ALL)
fi
])


dnl FILE: m4/macros/header_templates.m4
dnl defines descriptions for various am-utils specific macros

AH_TEMPLATE([HAVE_AMU_FS_AUTO],
[Define if have automount filesystem])

AH_TEMPLATE([HAVE_AMU_FS_DIRECT],
[Define if have direct automount filesystem])

AH_TEMPLATE([HAVE_AMU_FS_TOPLVL],
[Define if have "top-level" filesystem])

AH_TEMPLATE([HAVE_AMU_FS_ERROR],
[Define if have error filesystem])

AH_TEMPLATE([HAVE_AMU_FS_INHERIT],
[Define if have inheritance filesystem])

AH_TEMPLATE([HAVE_AMU_FS_PROGRAM],
[Define if have program filesystem])

AH_TEMPLATE([HAVE_AMU_FS_LINK],
[Define if have symbolic-link filesystem])

AH_TEMPLATE([HAVE_AMU_FS_LINKX],
[Define if have symlink with existence check filesystem])

AH_TEMPLATE([HAVE_AMU_FS_HOST],
[Define if have NFS host-tree filesystem])

AH_TEMPLATE([HAVE_AMU_FS_NFSL],
[Define if have nfsl (NFS with local link check) filesystem])

AH_TEMPLATE([HAVE_AMU_FS_NFSX],
[Define if have multi-NFS filesystem])

AH_TEMPLATE([HAVE_AMU_FS_UNION],
[Define if have union filesystem])

AH_TEMPLATE([HAVE_MAP_FILE],
[Define if have file maps (everyone should have it!)])

AH_TEMPLATE([HAVE_MAP_NIS],
[Define if have NIS maps])

AH_TEMPLATE([HAVE_MAP_NISPLUS],
[Define if have NIS+ maps])

AH_TEMPLATE([HAVE_MAP_DBM],
[Define if have DBM maps])

AH_TEMPLATE([HAVE_MAP_NDBM],
[Define if have NDBM maps])

AH_TEMPLATE([HAVE_MAP_HESIOD],
[Define if have HESIOD maps])

AH_TEMPLATE([HAVE_MAP_LDAP],
[Define if have LDAP maps])

AH_TEMPLATE([HAVE_MAP_PASSWD],
[Define if have PASSWD maps])

AH_TEMPLATE([HAVE_MAP_UNION],
[Define if have UNION maps])

AH_TEMPLATE([HAVE_FS_UFS],
[Define if have UFS filesystem])

AH_TEMPLATE([HAVE_FS_XFS],
[Define if have XFS filesystem (irix)])

AH_TEMPLATE([HAVE_FS_EFS],
[Define if have EFS filesystem (irix)])

AH_TEMPLATE([HAVE_FS_NFS],
[Define if have NFS filesystem])

AH_TEMPLATE([HAVE_FS_NFS3],
[Define if have NFS3 filesystem])

AH_TEMPLATE([HAVE_FS_PCFS],
[Define if have PCFS filesystem])

AH_TEMPLATE([HAVE_FS_LOFS],
[Define if have LOFS filesystem])

AH_TEMPLATE([HAVE_FS_HSFS],
[Define if have HSFS filesystem])

AH_TEMPLATE([HAVE_FS_CDFS],
[Define if have CDFS filesystem])

AH_TEMPLATE([HAVE_FS_TFS],
[Define if have TFS filesystem])

AH_TEMPLATE([HAVE_FS_TMPFS],
[Define if have TMPFS filesystem])

AH_TEMPLATE([HAVE_FS_MFS],
[Define if have MFS filesystem])

AH_TEMPLATE([HAVE_FS_CFS],
[Define if have CFS (crypto) filesystem])

AH_TEMPLATE([HAVE_FS_AUTOFS],
[Define if have AUTOFS filesystem])

AH_TEMPLATE([HAVE_FS_CACHEFS],
[Define if have CACHEFS filesystem])

AH_TEMPLATE([HAVE_FS_NULLFS],
[Define if have NULLFS (loopback on bsd44) filesystem])

AH_TEMPLATE([HAVE_FS_UNIONFS],
[Define if have UNIONFS filesystem])

AH_TEMPLATE([HAVE_FS_UMAPFS],
[Define if have UMAPFS (uid/gid mapping) filesystem])

AH_TEMPLATE([MOUNT_TYPE_UFS],
[Mount(2) type/name for UFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_XFS],
[Mount(2) type/name for XFS filesystem (irix)])

AH_TEMPLATE([MOUNT_TYPE_EFS],
[Mount(2) type/name for EFS filesystem (irix)])

AH_TEMPLATE([MOUNT_TYPE_NFS],
[Mount(2) type/name for NFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_NFS3],
[Mount(2) type/name for NFS3 filesystem])

AH_TEMPLATE([MOUNT_TYPE_PCFS],
[Mount(2) type/name for PCFS filesystem. XXX: conf/trap/trap_hpux.h may override this definition for HPUX 9.0])

AH_TEMPLATE([MOUNT_TYPE_LOFS],
[Mount(2) type/name for LOFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_CDFS],
[Mount(2) type/name for CDFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_TFS],
[Mount(2) type/name for TFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_TMPFS],
[Mount(2) type/name for TMPFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_MFS],
[Mount(2) type/name for MFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_CFS],
[Mount(2) type/name for CFS (crypto) filesystem])

AH_TEMPLATE([MOUNT_TYPE_AUTOFS],
[Mount(2) type/name for AUTOFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_CACHEFS],
[Mount(2) type/name for CACHEFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_IGNORE],
[Mount(2) type/name for IGNORE filesystem (not real just ignore for df)])

AH_TEMPLATE([MOUNT_TYPE_NULLFS],
[Mount(2) type/name for NULLFS (loopback on bsd44) filesystem])

AH_TEMPLATE([MOUNT_TYPE_UNIONFS],
[Mount(2) type/name for UNIONFS filesystem])

AH_TEMPLATE([MOUNT_TYPE_UMAPFS],
[Mount(2) type/name for UMAPFS (uid/gid mapping) filesystem])

AH_TEMPLATE([MNTTAB_TYPE_UFS],
[Mount-table entry name for UFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_XFS],
[Mount-table entry name for XFS filesystem (irix)])

AH_TEMPLATE([MNTTAB_TYPE_EFS],
[Mount-table entry name for EFS filesystem (irix)])

AH_TEMPLATE([MNTTAB_TYPE_NFS],
[Mount-table entry name for NFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_NFS3],
[Mount-table entry name for NFS3 filesystem])

AH_TEMPLATE([MNTTAB_TYPE_PCFS],
[Mount-table entry name for PCFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_LOFS],
[Mount-table entry name for LOFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_CDFS],
[Mount-table entry name for CDFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_TFS],
[Mount-table entry name for TFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_TMPFS],
[Mount-table entry name for TMPFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_MFS],
[Mount-table entry name for MFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_CFS],
[Mount-table entry name for CFS (crypto) filesystem])

AH_TEMPLATE([MNTTAB_TYPE_AUTOFS],
[Mount-table entry name for AUTOFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_CACHEFS],
[Mount-table entry name for CACHEFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_NULLFS],
[Mount-table entry name for NULLFS (loopback on bsd44) filesystem])

AH_TEMPLATE([MNTTAB_TYPE_UNIONFS],
[Mount-table entry name for UNIONFS filesystem])

AH_TEMPLATE([MNTTAB_TYPE_UMAPFS],
[Mount-table entry name for UMAPFS (uid/gid mapping) filesystem])

AH_TEMPLATE([MNTTAB_FILE_NAME],
[Name of mount table file name])

AH_TEMPLATE([HIDE_MOUNT_TYPE],
[Name of mount type to hide amd mount from df(1)])

AH_TEMPLATE([MNTTAB_OPT_RO],
[Mount Table option string: Read only])

AH_TEMPLATE([MNTTAB_OPT_RW],
[Mount Table option string: Read/write])

AH_TEMPLATE([MNTTAB_OPT_RQ],
[Mount Table option string: Read/write with quotas])

AH_TEMPLATE([MNTTAB_OPT_QUOTA],
[Mount Table option string: Check quotas])

AH_TEMPLATE([MNTTAB_OPT_NOQUOTA],
[Mount Table option string: Don't check quotas])

AH_TEMPLATE([MNTTAB_OPT_ONERROR],
[Mount Table option string: action to taken on error])

AH_TEMPLATE([MNTTAB_OPT_TOOSOON],
[Mount Table option string: min. time between inconsistencies])

AH_TEMPLATE([MNTTAB_OPT_SOFT],
[Mount Table option string: Soft mount])

AH_TEMPLATE([MNTTAB_OPT_SPONGY],
[Mount Table option string: spongy mount])

AH_TEMPLATE([MNTTAB_OPT_HARD],
[Mount Table option string: Hard mount])

AH_TEMPLATE([MNTTAB_OPT_SUID],
[Mount Table option string: Set uid allowed])

AH_TEMPLATE([MNTTAB_OPT_NOSUID],
[Mount Table option string: Set uid not allowed])

AH_TEMPLATE([MNTTAB_OPT_GRPID],
[Mount Table option string: SysV-compatible gid on create])

AH_TEMPLATE([MNTTAB_OPT_REMOUNT],
[Mount Table option string: Change mount options])

AH_TEMPLATE([MNTTAB_OPT_NOSUB],
[Mount Table option string: Disallow mounts on subdirs])

AH_TEMPLATE([MNTTAB_OPT_MULTI],
[Mount Table option string: Do multi-component lookup])

AH_TEMPLATE([MNTTAB_OPT_INTR],
[Mount Table option string: Allow NFS ops to be interrupted])

AH_TEMPLATE([MNTTAB_OPT_NOINTR],
[Mount Table option string: Don't allow interrupted ops])

AH_TEMPLATE([MNTTAB_OPT_PORT],
[Mount Table option string: NFS server IP port number])

AH_TEMPLATE([MNTTAB_OPT_SECURE],
[Mount Table option string: Secure (AUTH_DES) mounting])

AH_TEMPLATE([MNTTAB_OPT_KERB],
[Mount Table option string: Secure (AUTH_Kerb) mounting])

AH_TEMPLATE([MNTTAB_OPT_RSIZE],
[Mount Table option string: Max NFS read size (bytes)])

AH_TEMPLATE([MNTTAB_OPT_WSIZE],
[Mount Table option string: Max NFS write size (bytes)])

AH_TEMPLATE([MNTTAB_OPT_TIMEO],
[Mount Table option string: NFS timeout (1/10 sec)])

AH_TEMPLATE([MNTTAB_OPT_RETRANS],
[Mount Table option string: Max retransmissions (soft mnts)])

AH_TEMPLATE([MNTTAB_OPT_ACTIMEO],
[Mount Table option string: Attr cache timeout (sec)])

AH_TEMPLATE([MNTTAB_OPT_ACREGMIN],
[Mount Table option string: Min attr cache timeout (files)])

AH_TEMPLATE([MNTTAB_OPT_ACREGMAX],
[Mount Table option string: Max attr cache timeout (files)])

AH_TEMPLATE([MNTTAB_OPT_ACDIRMIN],
[Mount Table option string: Min attr cache timeout (dirs)])

AH_TEMPLATE([MNTTAB_OPT_ACDIRMAX],
[Mount Table option string: Max attr cache timeout (dirs)])

AH_TEMPLATE([MNTTAB_OPT_NOAC],
[Mount Table option string: Don't cache attributes at all])

AH_TEMPLATE([MNTTAB_OPT_NOCTO],
[Mount Table option string: No close-to-open consistency])

AH_TEMPLATE([MNTTAB_OPT_BG],
[Mount Table option string: Do mount retries in background])

AH_TEMPLATE([MNTTAB_OPT_FG],
[Mount Table option string: Do mount retries in foreground])

AH_TEMPLATE([MNTTAB_OPT_RETRY],
[Mount Table option string: Number of mount retries])

AH_TEMPLATE([MNTTAB_OPT_DEV],
[Mount Table option string: Device id of mounted fs])

AH_TEMPLATE([MNTTAB_OPT_FSID],
[Mount Table option string: Filesystem id of mounted fs])

AH_TEMPLATE([MNTTAB_OPT_POSIX],
[Mount Table option string: Get static pathconf for mount])

AH_TEMPLATE([MNTTAB_OPT_MAP],
[Mount Table option string: Automount map])

AH_TEMPLATE([MNTTAB_OPT_DIRECT],
[Mount Table option string: Automount   direct map mount])

AH_TEMPLATE([MNTTAB_OPT_INDIRECT],
[Mount Table option string: Automount indirect map mount])

AH_TEMPLATE([MNTTAB_OPT_LLOCK],
[Mount Table option string: Local locking (no lock manager)])

AH_TEMPLATE([MNTTAB_OPT_IGNORE],
[Mount Table option string: Ignore this entry])

AH_TEMPLATE([MNTTAB_OPT_NOAUTO],
[Mount Table option string: No auto (what?)])

AH_TEMPLATE([MNTTAB_OPT_NOCONN],
[Mount Table option string: No connection])

AH_TEMPLATE([MNTTAB_OPT_VERS],
[Mount Table option string: protocol version number indicator])

AH_TEMPLATE([MNTTAB_OPT_PROTO],
[Mount Table option string: protocol network_id indicator])

AH_TEMPLATE([MNTTAB_OPT_SYNCDIR],
[Mount Table option string: Synchronous local directory ops])

AH_TEMPLATE([MNTTAB_OPT_NOSETSEC],
[Mount Table option string: Do no allow setting sec attrs])

AH_TEMPLATE([MNTTAB_OPT_SYMTTL],
[Mount Table option string: set symlink cache time-to-live])

AH_TEMPLATE([MNTTAB_OPT_COMPRESS],
[Mount Table option string: compress])

AH_TEMPLATE([MNTTAB_OPT_PGTHRESH],
[Mount Table option string: paging threshold])

AH_TEMPLATE([MNTTAB_OPT_MAXGROUPS],
[Mount Table option string: max groups])

AH_TEMPLATE([MNTTAB_OPT_PROPLIST],
[Mount Table option string: support property lists (ACLs)])

AH_TEMPLATE([MNT2_GEN_OPT_ASYNC],
[asynchronous filesystem access])

AH_TEMPLATE([MNT2_GEN_OPT_AUTOMNTFS],
[automounter filesystem (ignore) flag, used in bsdi-4.1])

AH_TEMPLATE([MNT2_GEN_OPT_BIND],
[directory hardlink])

AH_TEMPLATE([MNT2_GEN_OPT_CACHE],
[cache (what?)])

AH_TEMPLATE([MNT2_GEN_OPT_DATA],
[6-argument mount])

AH_TEMPLATE([MNT2_GEN_OPT_FSS],
[old (4-argument) mount (compatibility)])

AH_TEMPLATE([MNT2_GEN_OPT_IGNORE],
[ignore mount entry in df output])

AH_TEMPLATE([MNT2_GEN_OPT_JFS],
[journaling filesystem (AIX's UFS/FFS)])

AH_TEMPLATE([MNT2_GEN_OPT_GRPID],
[old BSD group-id on create])

AH_TEMPLATE([MNT2_GEN_OPT_MULTI],
[do multi-component lookup on files])

AH_TEMPLATE([MNT2_GEN_OPT_NEWTYPE],
[use type string instead of int])

AH_TEMPLATE([MNT2_GEN_OPT_NFS],
[NFS mount])

AH_TEMPLATE([MNT2_GEN_OPT_NOCACHE],
[nocache (what?)])

AH_TEMPLATE([MNT2_GEN_OPT_NODEV],
[do not interpret special device files])

AH_TEMPLATE([MNT2_GEN_OPT_NOEXEC],
[no exec calls allowed])

AH_TEMPLATE([MNT2_GEN_OPT_NONDEV],
[do not interpret special device files])

AH_TEMPLATE([MNT2_GEN_OPT_NOSUB],
[Disallow mounts beneath this mount])

AH_TEMPLATE([MNT2_GEN_OPT_NOSUID],
[Setuid programs disallowed])

AH_TEMPLATE([MNT2_GEN_OPT_NOTRUNC],
[Return ENAMETOOLONG for long filenames])

AH_TEMPLATE([MNT2_GEN_OPT_OPTIONSTR],
[Pass mount option string to kernel])

AH_TEMPLATE([MNT2_GEN_OPT_OVERLAY],
[allow overlay mounts])

AH_TEMPLATE([MNT2_GEN_OPT_QUOTA],
[check quotas])

AH_TEMPLATE([MNT2_GEN_OPT_RDONLY],
[Read-only])

AH_TEMPLATE([MNT2_GEN_OPT_REMOUNT],
[change options on an existing mount])

AH_TEMPLATE([MNT2_GEN_OPT_RONLY],
[read only])

AH_TEMPLATE([MNT2_GEN_OPT_SYNC],
[synchronize data immediately to filesystem])

AH_TEMPLATE([MNT2_GEN_OPT_SYNCHRONOUS],
[synchronous filesystem access (same as SYNC)])

AH_TEMPLATE([MNT2_GEN_OPT_SYS5],
[Mount with Sys 5-specific semantics])

AH_TEMPLATE([MNT2_GEN_OPT_UNION],
[Union mount])

AH_TEMPLATE([MNT2_NFS_OPT_AUTO],
[hide mount type from df(1)])

AH_TEMPLATE([MNT2_NFS_OPT_ACDIRMAX],
[set max secs for dir attr cache])

AH_TEMPLATE([MNT2_NFS_OPT_ACDIRMIN],
[set min secs for dir attr cache])

AH_TEMPLATE([MNT2_NFS_OPT_ACREGMAX],
[set max secs for file attr cache])

AH_TEMPLATE([MNT2_NFS_OPT_ACREGMIN],
[set min secs for file attr cache])

AH_TEMPLATE([MNT2_NFS_OPT_AUTHERR],
[Authentication error])

AH_TEMPLATE([MNT2_NFS_OPT_DEADTHRESH],
[set dead server retry thresh])

AH_TEMPLATE([MNT2_NFS_OPT_DISMINPROG],
[Dismount in progress])

AH_TEMPLATE([MNT2_NFS_OPT_DISMNT],
[Dismounted])

AH_TEMPLATE([MNT2_NFS_OPT_DUMBTIMR],
[Don't estimate rtt dynamically])

AH_TEMPLATE([MNT2_NFS_OPT_GRPID],
[System V-style gid inheritance])

AH_TEMPLATE([MNT2_NFS_OPT_HASAUTH],
[Has authenticator])

AH_TEMPLATE([MNT2_NFS_OPT_FSNAME],
[provide name of server's fs to system])

AH_TEMPLATE([MNT2_NFS_OPT_HOSTNAME],
[set hostname for error printf])

AH_TEMPLATE([MNT2_NFS_OPT_IGNORE],
[ignore mount point])

AH_TEMPLATE([MNT2_NFS_OPT_INT],
[allow interrupts on hard mount])

AH_TEMPLATE([MNT2_NFS_OPT_INTR],
[allow interrupts on hard mount])

AH_TEMPLATE([MNT2_NFS_OPT_INTERNAL],
[Bits set internally])

AH_TEMPLATE([MNT2_NFS_OPT_KERB],
[Use Kerberos authentication])

AH_TEMPLATE([MNT2_NFS_OPT_KERBEROS],
[use kerberos credentials])

AH_TEMPLATE([MNT2_NFS_OPT_KNCONF],
[transport's knetconfig structure])

AH_TEMPLATE([MNT2_NFS_OPT_LEASETERM],
[set lease term (nqnfs)])

AH_TEMPLATE([MNT2_NFS_OPT_LLOCK],
[Local locking (no lock manager)])

AH_TEMPLATE([MNT2_NFS_OPT_MAXGRPS],
[set maximum grouplist size])

AH_TEMPLATE([MNT2_NFS_OPT_MNTD],
[Mnt server for mnt point])

AH_TEMPLATE([MNT2_NFS_OPT_MYWRITE],
[Assume writes were mine])

AH_TEMPLATE([MNT2_NFS_OPT_NFSV3],
[mount NFS Version 3])

AH_TEMPLATE([MNT2_NFS_OPT_NOAC],
[don't cache attributes])

AH_TEMPLATE([MNT2_NFS_OPT_NOCONN],
[Don't Connect the socket])

AH_TEMPLATE([MNT2_NFS_OPT_NOCTO],
[no close-to-open consistency])

AH_TEMPLATE([MNT2_NFS_OPT_NOINT],
[disallow interrupts on hard mounts])

AH_TEMPLATE([MNT2_NFS_OPT_NQLOOKLEASE],
[Get lease for lookup])

AH_TEMPLATE([MNT2_NFS_OPT_NONLM],
[Don't use locking])

AH_TEMPLATE([MNT2_NFS_OPT_NQNFS],
[Use Nqnfs protocol])

AH_TEMPLATE([MNT2_NFS_OPT_POSIX],
[static pathconf kludge info])

AH_TEMPLATE([MNT2_NFS_OPT_RCVLOCK],
[Rcv socket lock])

AH_TEMPLATE([MNT2_NFS_OPT_RDIRALOOK],
[Do lookup with readdir (nqnfs)])

AH_TEMPLATE([MNT2_NFS_OPT_PROPLIST],
[allow property list operations (ACLs over NFS)])

AH_TEMPLATE([MNT2_NFS_OPTS_RDIRPLUS],
[Use Readdirplus for NFSv3])

AH_TEMPLATE([MNT2_NFS_OPT_READAHEAD],
[set read ahead])

AH_TEMPLATE([MNT2_NFS_OPT_READDIRSIZE],
[Set readdir size])

AH_TEMPLATE([MNT2_NFS_OPT_RESVPORT],
[Allocate a reserved port])

AH_TEMPLATE([MNT2_NFS_OPT_RETRANS],
[set number of request retries])

AH_TEMPLATE([MNT2_NFS_OPT_RONLY],
[read only])

AH_TEMPLATE([MNT2_NFS_OPT_RPCTIMESYNC],
[use RPC to do secure NFS time sync])

AH_TEMPLATE([MNT2_NFS_OPT_RSIZE],
[set read size])

AH_TEMPLATE([MNT2_NFS_OPT_SECURE],
[secure mount])

AH_TEMPLATE([MNT2_NFS_OPT_SNDLOCK],
[Send socket lock])

AH_TEMPLATE([MNT2_NFS_OPT_SOFT],
[soft mount (hard is default)])

AH_TEMPLATE([MNT2_NFS_OPT_SPONGY],
[spongy mount])

AH_TEMPLATE([MNT2_NFS_OPT_TIMEO],
[set initial timeout])

AH_TEMPLATE([MNT2_NFS_OPT_TCP],
[use TCP for mounts])

AH_TEMPLATE([MNT2_NFS_OPT_VER3],
[linux NFSv3])

AH_TEMPLATE([MNT2_NFS_OPT_WAITAUTH],
[Wait for authentication])

AH_TEMPLATE([MNT2_NFS_OPT_WANTAUTH],
[Wants an authenticator])

AH_TEMPLATE([MNT2_NFS_OPT_WANTRCV],
[Want receive socket lock])

AH_TEMPLATE([MNT2_NFS_OPT_WANTSND],
[Want send socket lock])

AH_TEMPLATE([MNT2_NFS_OPT_WSIZE],
[set write size])

AH_TEMPLATE([MNT2_NFS_OPT_SYMTTL],
[set symlink cache time-to-live])

AH_TEMPLATE([MNT2_NFS_OPT_PGTHRESH],
[paging threshold])

AH_TEMPLATE([MNT2_NFS_OPT_XLATECOOKIE],
[32<->64 dir cookie translation])

AH_TEMPLATE([MNT2_CDFS_OPT_DEFPERM],
[Ignore permission bits])

AH_TEMPLATE([MNT2_CDFS_OPT_NODEFPERM],
[Use on-disk permission bits])

AH_TEMPLATE([MNT2_CDFS_OPT_NOVERSION],
[Strip off extension from version string])

AH_TEMPLATE([MNT2_CDFS_OPT_RRIP],
[Use Rock Ridge Interchange Protocol (RRIP) extensions])

AH_TEMPLATE([HAVE_MNTENT_T_MNT_TIME_STRING],
[does mntent_t have mnt_time field and is of type "char *" ?])

AH_TEMPLATE([REINSTALL_SIGNAL_HANDLER],
[should signal handlers be reinstalled?])

AH_TEMPLATE([DEBUG],
[Turn off general debugging by default])

AH_TEMPLATE([DEBUG_MEM],
[Turn off memory debugging by default])

AH_TEMPLATE([PACKAGE_NAME],
[Define package name (must be defined by configure.in)])

AH_TEMPLATE([PACKAGE_VERSION],
[Define version of package (must be defined by configure.in)])

AH_TEMPLATE([PACKAGE_BUGREPORT],
[Define bug-reporting address (must be defined by configure.in)])

AH_TEMPLATE([HOST_CPU],
[Define name of host machine's cpu (eg. sparc)])

AH_TEMPLATE([HOST_ARCH],
[Define name of host machine's architecture (eg. sun4)])

AH_TEMPLATE([HOST_VENDOR],
[Define name of host machine's vendor (eg. sun)])

AH_TEMPLATE([HOST_OS],
[Define name and version of host machine (eg. solaris2.5.1)])

AH_TEMPLATE([HOST_OS_NAME],
[Define only name of host machine OS (eg. solaris2)])

AH_TEMPLATE([HOST_OS_VERSION],
[Define only version of host machine (eg. 2.5.1)])

AH_TEMPLATE([HOST_HEADER_VERSION],
[Define the header version of (linux) hosts (eg. 2.2.10)])

AH_TEMPLATE([HOST_NAME],
[Define name of host])

AH_TEMPLATE([USER_NAME],
[Define user name])

AH_TEMPLATE([CONFIG_DATE],
[Define configuration date])

AH_TEMPLATE([HAVE_TRANSPORT_TYPE_TLI],
[what type of network transport type is in use?  TLI or sockets?])

AH_TEMPLATE([time_t],
[Define to `long' if <sys/types.h> doesn't define time_t])

AH_TEMPLATE([voidp],
[Define to "void *" if compiler can handle, otherwise "char *"])

AH_TEMPLATE([am_nfs_fh],
[Define a type/structure for an NFS V2 filehandle])

AH_TEMPLATE([am_nfs_fh3],
[Define a type/structure for an NFS V3 filehandle])

AH_TEMPLATE([HAVE_NFS_PROT_HEADERS],
[define if the host has NFS protocol headers in system headers])

AH_TEMPLATE([AMU_NFS_PROTOCOL_HEADER],
[define name of am-utils' NFS protocol header])

AH_TEMPLATE([nfs_args_t],
[Define a type for the nfs_args structure])

AH_TEMPLATE([NFS_FH_FIELD],
[Define the field name for the filehandle within nfs_args_t])

AH_TEMPLATE([HAVE_FHANDLE],
[Define if plain fhandle type exists])

AH_TEMPLATE([SVC_IN_ARG_TYPE],
[Define the type of the 3rd argument ('in') to svc_getargs()])

AH_TEMPLATE([XDRPROC_T_TYPE],
[Define to the type of xdr procedure type])

AH_TEMPLATE([MOUNT_TABLE_ON_FILE],
[Define if mount table is on file, undefine if in kernel])

AH_TEMPLATE([HAVE_STRUCT_MNTENT],
[Define if have struct mntent in one of the standard headers])

AH_TEMPLATE([HAVE_STRUCT_MNTTAB],
[Define if have struct mnttab in one of the standard headers])

AH_TEMPLATE([HAVE_STRUCT_NFS_ARGS],
[Define if have struct nfs_args in one of the standard nfs headers])

AH_TEMPLATE([HAVE_STRUCT_NFS_GFS_MOUNT],
[Define if have struct nfs_gfs_mount in one of the standard nfs headers])

AH_TEMPLATE([YP_ORDER_OUTORDER_TYPE],
[Type of the 3rd argument to yp_order()])

AH_TEMPLATE([RECVFROM_FROMLEN_TYPE],
[Type of the 6th argument to recvfrom()])

AH_TEMPLATE([AUTH_CREATE_GIDLIST_TYPE],
[Type of the 5rd argument to authunix_create()])

AH_TEMPLATE([MTYPE_PRINTF_TYPE],
[The string used in printf to print the mount-type field of mount(2)])

AH_TEMPLATE([MTYPE_TYPE],
[Type of the mount-type field in the mount() system call])

AH_TEMPLATE([pcfs_args_t],
[Define a type for the pcfs_args structure])

AH_TEMPLATE([autofs_args_t],
[Define a type for the autofs_args structure])

AH_TEMPLATE([cachefs_args_t],
[Define a type for the cachefs_args structure])

AH_TEMPLATE([tmpfs_args_t],
[Define a type for the tmpfs_args structure])

AH_TEMPLATE([ufs_args_t],
[Define a type for the ufs_args structure])

AH_TEMPLATE([efs_args_t],
[Define a type for the efs_args structure])

AH_TEMPLATE([xfs_args_t],
[Define a type for the xfs_args structure])

AH_TEMPLATE([lofs_args_t],
[Define a type for the lofs_args structure])

AH_TEMPLATE([cdfs_args_t],
[Define a type for the cdfs_args structure])

AH_TEMPLATE([mfs_args_t],
[Define a type for the mfs_args structure])

AH_TEMPLATE([rfs_args_t],
[Define a type for the rfs_args structure])

AH_TEMPLATE([HAVE_BAD_HASMNTOPT],
[define if have a bad version of hasmntopt()])

AH_TEMPLATE([HAVE_BAD_MEMCMP],
[define if have a bad version of memcmp()])

AH_TEMPLATE([HAVE_BAD_YP_ALL],
[define if have a bad version of yp_all()])

AH_TEMPLATE([USE_UNCONNECTED_NFS_SOCKETS],
[define if must use NFS "noconn" option])

AH_TEMPLATE([USE_CONNECTED_NFS_SOCKETS],
[define if must NOT use NFS "noconn" option])

AH_TEMPLATE([HAVE_GNU_GETOPT],
[define if your system's getopt() is GNU getopt() (are you using glibc)])

AH_TEMPLATE([HAVE_EXTERN_SYS_ERRLIST],
[does extern definition for sys_errlist[] exist?])

AH_TEMPLATE([HAVE_EXTERN_OPTARG],
[does extern definition for optarg exist?])

AH_TEMPLATE([HAVE_EXTERN_CLNT_SPCREATEERROR],
[does extern definition for clnt_spcreateerror() exist?])

AH_TEMPLATE([HAVE_EXTERN_CLNT_SPERRNO],
[does extern definition for clnt_sperrno() exist?])

AH_TEMPLATE([HAVE_EXTERN_FREE],
[does extern definition for free() exist?])

AH_TEMPLATE([HAVE_EXTERN_GET_MYADDRESS],
[does extern definition for get_myaddress() exist?])

AH_TEMPLATE([HAVE_EXTERN_GETCCENT],
[does extern definition for getccent() (hpux) exist?])

AH_TEMPLATE([HAVE_EXTERN_GETDOMAINNAME],
[does extern definition for getdomainname() exist?])

AH_TEMPLATE([HAVE_EXTERN_GETHOSTNAME],
[does extern definition for gethostname() exist?])

AH_TEMPLATE([HAVE_EXTERN_GETLOGIN],
[does extern definition for getlogin() exist?])

AH_TEMPLATE([HAVE_EXTERN_GETTABLESIZE],
[does extern definition for gettablesize() exist?])

AH_TEMPLATE([HAVE_EXTERN_GETPAGESIZE],
[does extern definition for getpagesize() exist?])

AH_TEMPLATE([HAVE_EXTERN_INNETGR],
[does extern definition for innetgr() exist?])

AH_TEMPLATE([HAVE_EXTERN_MKSTEMP],
[does extern definition for mkstemp() exist?])

AH_TEMPLATE([HAVE_EXTERN_SBRK],
[does extern definition for sbrk() exist?])

AH_TEMPLATE([HAVE_EXTERN_SETEUID],
[does extern definition for seteuid() exist?])

AH_TEMPLATE([HAVE_EXTERN_SETITIMER],
[does extern definition for setitimer() exist?])

AH_TEMPLATE([HAVE_EXTERN_STRCASECMP],
[does extern definition for strcasecmp() exist?])

AH_TEMPLATE([HAVE_EXTERN_STRDUP],
[does extern definition for strdup() exist?])

AH_TEMPLATE([HAVE_EXTERN_STRSTR],
[does extern definition for strstr() exist?])

AH_TEMPLATE([HAVE_EXTERN_USLEEP],
[does extern definition for usleep() exist?])

AH_TEMPLATE([HAVE_EXTERN_WAIT3],
[does extern definition for wait3() exist?])

AH_TEMPLATE([HAVE_EXTERN_VSNPRINTF],
[does extern definition for vsnprintf() exist?])

AH_TEMPLATE([HAVE_EXTERN_XDR_CALLMSG],
[does extern definition for xdr_callmsg() exist?])

AH_TEMPLATE([HAVE_EXTERN_XDR_OPAQUE_AUTH],
[does extern definition for xdr_opaque_auth() exist?])


dnl ######################################################################
dnl AC_HOST_MACROS: define HOST_CPU, HOST_VENDOR, and HOST_OS
AC_DEFUN(AMU_HOST_MACROS,
[
# these are defined already by the macro 'CANONICAL_HOST'
  AC_MSG_CHECKING([host cpu])
  AC_DEFINE_UNQUOTED(HOST_CPU, "$host_cpu")
  AC_MSG_RESULT($host_cpu)

  AC_MSG_CHECKING([vendor])
  AC_DEFINE_UNQUOTED(HOST_VENDOR, "$host_vendor")
  AC_MSG_RESULT($host_vendor)

  AC_MSG_CHECKING([host full OS name and version])
  # normalize some host OS names
  case ${host_os} in
	# linux is linux is linux, regardless of RMS.
	linux-gnu* | lignux* )	host_os=linux ;;
  esac
  AC_DEFINE_UNQUOTED(HOST_OS, "$host_os")
  AC_MSG_RESULT($host_os)

# break host_os into host_os_name and host_os_version
  AC_MSG_CHECKING([host OS name])
  host_os_name=`echo $host_os | sed 's/\..*//g'`
  # normalize some OS names
  case ${host_os_name} in
	# linux is linux is linux, regardless of RMS.
	linux-gnu* | lignux* )	host_os_name=linux ;;
  esac
  AC_DEFINE_UNQUOTED(HOST_OS_NAME, "$host_os_name")
  AC_MSG_RESULT($host_os_name)

# parse out the OS version of the host
  AC_MSG_CHECKING([host OS version])
  host_os_version=`echo $host_os | sed 's/^[[^0-9]]*//g'`
  if test -z "$host_os_version"
  then
	host_os_version=`(uname -r) 2>/dev/null` || host_os_version=unknown
  fi
  case ${host_os_version} in
	# fixes for some OS versions (solaris used to be here)
	* ) # do nothing for now
	;;
  esac
  AC_DEFINE_UNQUOTED(HOST_OS_VERSION, "$host_os_version")
  AC_MSG_RESULT($host_os_version)

# figure out host architecture (different than CPU)
  AC_MSG_CHECKING([host OS architecture])
  host_arch=`(uname -m) 2>/dev/null` || host_arch=unknown
  # normalize some names
  case ${host_arch} in
	sun4* )	host_arch=sun4 ;;
	sun3x )	host_arch=sun3 ;;
	sun )	host_arch=`(arch) 2>/dev/null` || host_arch=unknown ;;
	i?86 )	host_arch=i386 ;; # all x86 should show up as i386
  esac
  AC_DEFINE_UNQUOTED(HOST_ARCH, "$host_arch")
  AC_MSG_RESULT($host_arch)

# figure out host name
  AC_MSG_CHECKING([host name])
  host_name=`(hostname || uname -n) 2>/dev/null` || host_name=unknown
  AC_DEFINE_UNQUOTED(HOST_NAME, "$host_name")
  AC_MSG_RESULT($host_name)

# figure out user name
  AC_MSG_CHECKING([user name])
  if test -n "$USER"
  then
    user_name="$USER"
  else
    if test -n "$LOGNAME"
    then
      user_name="$LOGNAME"
    else
      user_name=`(whoami) 2>/dev/null` || user_name=unknown
    fi
  fi
  AC_DEFINE_UNQUOTED(USER_NAME, "$user_name")
  AC_MSG_RESULT($user_name)

# figure out configuration date
  AC_MSG_CHECKING([configuration date])
  config_date=`(date) 2>/dev/null` || config_date=unknown_date
  AC_DEFINE_UNQUOTED(CONFIG_DATE, "$config_date")
  AC_MSG_RESULT($config_date)

])
dnl ======================================================================


dnl ######################################################################
dnl ensure that linux kernel headers match running kernel
AC_DEFUN(AMU_LINUX_HEADERS,
[
# test sanity of running kernel vs. kernel headers
  AC_MSG_CHECKING("host headers version")
  case ${host_os} in
    linux )
      host_header_version="bad"
      AMU_EXPAND_RUN_STRING(
[
#include <stdio.h>
#include <linux/version.h>
],
[
if (argc > 1)
  printf("%s", UTS_RELEASE);
],
[ host_header_version=$value ],
[ echo
  AC_MSG_ERROR([cannot find UTS_RELEASE in <linux/version.h>.
  This Linux system may be misconfigured or unconfigured!])
])
	;;
	* ) host_header_version=$host_os_version ;;
  esac
  AC_DEFINE_UNQUOTED(HOST_HEADER_VERSION, "$host_header_version")
  AC_MSG_RESULT($host_header_version)

  case ${host_os} in
    linux )
	if test "$host_os_version" != $host_header_version
	then
		AC_MSG_WARN([Linux kernel $host_os_version mismatch with $host_header_version headers!])
	fi
    ;;
esac
dnl cache these two for debugging purposes
ac_cv_os_version=$host_os_version
ac_cv_header_version=$host_header_version
])
dnl ======================================================================


dnl ######################################################################
dnl check if a local configuration file exists
AC_DEFUN(AMU_LOCALCONFIG,
[AC_MSG_CHECKING(a local configuration file)
if test -f localconfig.h
then
  AC_DEFINE(HAVE_LOCALCONFIG_H)
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl an M4 macro to include a list of common headers being used everywhere
define(AMU_MOUNT_HEADERS,
[
#include "${srcdir}/include/mount_headers1.h"
#include AMU_NFS_PROTOCOL_HEADER
#include "${srcdir}/include/mount_headers2.h"

$1
]
)
dnl ======================================================================


dnl ######################################################################
dnl Which options to add to CFLAGS for compilation?
dnl NOTE: this is only for final compiltions, not for configure tests)
AC_DEFUN(AMU_OPT_AMU_CFLAGS,
[AC_MSG_CHECKING(for additional C option compilation flags)
AC_ARG_ENABLE(am-cflags,
AC_HELP_STRING([--enable-am-cflags=ARG],
		[compile package with ARG additional C flags]),
[
if test "$enableval" = "" || test "$enableval" = "yes" || test "$enableval" = "no"; then
  AC_MSG_ERROR(am-cflags must be supplied if option is used)
fi
# user supplied a cflags option to configure
AMU_CFLAGS="$enableval"
AC_SUBST(AMU_CFLAGS)
AC_MSG_RESULT($enableval)
], [
  # default is to have no additional C flags
  AMU_CFLAGS=""
  AC_SUBST(AMU_CFLAGS)
  AC_MSG_RESULT(none)
])
])
dnl ======================================================================


dnl ######################################################################
dnl Initial settings for CPPFLAGS (-I options)
dnl NOTE: this is for configuration as well as compilations!
AC_DEFUN(AMU_OPT_CPPFLAGS,
[AC_MSG_CHECKING(for configuration/compilation (-I) preprocessor flags)
AC_ARG_ENABLE(cppflags,
AC_HELP_STRING([--enable-cppflags=ARG],
	[configure/compile with ARG (-I) preprocessor flags]),
[
if test "$enableval" = "" || test "$enableval" = "yes" || test "$enableval" = "no"; then
  AC_MSG_ERROR(cppflags must be supplied if option is used)
fi
# use supplied options
CPPFLAGS="$CPPFLAGS $enableval"
export CPPFLAGS
AC_MSG_RESULT($enableval)
], [
  # default is to have no additional flags
  AC_MSG_RESULT(none)
])
])
dnl ======================================================================


dnl ######################################################################
dnl Debugging: "yes" means general, "mem" means general and memory debugging,
dnl and "no" means none.
AC_DEFUN(AMU_OPT_DEBUG,
[AC_MSG_CHECKING(for debugging options)
AC_ARG_ENABLE(debug,
AC_HELP_STRING([--enable-debug=ARG],[enable debugging (yes/mem/no)]),
[
if test "$enableval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(DEBUG)
  ac_cv_opt_debug=yes
elif test "$enableval" = mem; then
  AC_MSG_RESULT(mem)
  AC_DEFINE(DEBUG)
  AC_DEFINE(DEBUG_MEM)
  AC_CHECK_LIB(mapmalloc, malloc_verify)
  AC_CHECK_LIB(malloc, mallinfo)
  ac_cv_opt_debug=mem
else
  AC_MSG_RESULT(no)
  ac_cv_opt_debug=no
fi
],
[
  # default is no debugging
  AC_MSG_RESULT(no)
])
])
dnl ======================================================================


dnl ######################################################################
dnl Initial settings for LDFLAGS (-L options)
dnl NOTE: this is for configuration as well as compilations!
AC_DEFUN(AMU_OPT_LDFLAGS,
[AC_MSG_CHECKING(for configuration/compilation (-L) library flags)
AC_ARG_ENABLE(ldflags,
AC_HELP_STRING([--enable-ldflags=ARG],
		[configure/compile with ARG (-L) library flags]),
[
if test "$enableval" = "" || test "$enableval" = "yes" || test "$enableval" = "no"; then
  AC_MSG_ERROR(ldflags must be supplied if option is used)
fi
# use supplied options
LDFLAGS="$LDFLAGS $enableval"
export LDFLAGS
AC_MSG_RESULT($enableval)
], [
  # default is to have no additional flags
  AC_MSG_RESULT(none)
])
])
dnl ======================================================================


dnl ######################################################################
dnl Initial settings for LIBS (-l options)
dnl NOTE: this is for configuration as well as compilations!
AC_DEFUN(AMU_OPT_LIBS,
[AC_MSG_CHECKING(for configuration/compilation (-l) library flags)
AC_ARG_ENABLE(libs,
AC_HELP_STRING([--enable-libs=ARG],
		[configure/compile with ARG (-l) library flags]),
[
if test "$enableval" = "" || test "$enableval" = "yes" || test "$enableval" = "no"; then
  AC_MSG_ERROR(libs must be supplied if option is used)
fi
# use supplied options
LIBS="$LIBS $enableval"
export LIBS
AC_MSG_RESULT($enableval)
], [
  # default is to have no additional flags
  AC_MSG_RESULT(none)
])
])
dnl ======================================================================


dnl ######################################################################
dnl Specify additional compile options based on the OS and the compiler
AC_DEFUN(AMU_OS_CFLAGS,
[
AC_CACHE_CHECK(additional compiler flags,
ac_cv_os_cflags,
[
case "${host_os}" in
	irix6* )
		case "${CC}" in
			cc )
				# do not use 64-bit compiler
				ac_cv_os_cflags="-n32 -mips3 -Wl,-woff,84"
				;;
		esac
		;;
	osf[[1-3]]* )
		# get the right version of struct sockaddr
		case "${CC}" in
			cc )
				ac_cv_os_cflags="-std -D_SOCKADDR_LEN -D_NO_PROTO"
				;;
			* )
				ac_cv_os_cflags="-D_SOCKADDR_LEN -D_NO_PROTO"
				;;
		esac
		;;
	osf* )
		# get the right version of struct sockaddr
		case "${CC}" in
			cc )
				ac_cv_os_cflags="-std -D_SOCKADDR_LEN"
				;;
			* )
				ac_cv_os_cflags="-D_SOCKADDR_LEN"
				;;
		esac
		;;
	aix[[1-3]]* )
		ac_cv_os_cflags="" ;;
	aix4.[[0-2]]* )
		# turn on additional headers
		ac_cv_os_cflags="-D_XOPEN_EXTENDED_SOURCE"
		;;
	aix* )
		# avoid circular dependencies in yp headers
		ac_cv_os_cflags="-DHAVE_BAD_HEADERS -D_XOPEN_EXTENDED_SOURCE"
		;;
	OFF-sunos4* )
		# make sure passing whole structures is handled in gcc
		case "${CC}" in
			gcc )
				ac_cv_os_cflags="-fpcc-struct-return"
				;;
		esac
		;;
	sunos[[34]]* | solaris1* | solaris2.[[0-5]]* | sunos5.[[0-5]]* )
		ac_cv_os_cflags="" ;;
	solaris* | sunos* )
		# turn on 64-bit file offset interface
		case "${CC}" in
			* )
				ac_cv_os_cflags="-D_LARGEFILE64_SOURCE"
				;;
		esac
		;;
	hpux* )
		# use Ansi compiler on HPUX
		case "${CC}" in
			cc )
				ac_cv_os_cflags="-Ae"
				;;
		esac
		;;
	darwin* )
		ac_cv_os_cflags="-D_P1003_1B_VISIBLE"
		;;
	* )
		ac_cv_os_cflags=""
		;;
esac
])
CFLAGS="$CFLAGS $ac_cv_os_cflags"
])
dnl ======================================================================


dnl ######################################################################
dnl Specify additional cpp options based on the OS and the compiler
AC_DEFUN(AMU_OS_CPPFLAGS,
[
AC_CACHE_CHECK(additional preprocessor flags,
ac_cv_os_cppflags,
[
case "${host_os}" in
# off for now, posix may be a broken thing for nextstep3...
#	nextstep* )
#		ac_cv_os_cppflags="-D_POSIX_SOURCE"
#		;;
	* )	ac_cv_os_cppflags="" ;;
esac
])
CPPFLAGS="$CPPFLAGS $ac_cv_os_cppflags"
])
dnl ======================================================================


dnl ######################################################################
dnl Specify additional linker options based on the OS and the compiler
AC_DEFUN(AMU_OS_LDFLAGS,
[
AC_CACHE_CHECK(additional linker flags,
ac_cv_os_ldflags,
[
case "${host_os}" in
	solaris2.7* | sunos5.7* )
		# find LDAP: off until Sun includes ldap headers.
		case "${CC}" in
			* )
				#ac_cv_os_ldflags="-L/usr/lib/fn"
				;;
		esac
		;;
	* )	ac_cv_os_ldflags="" ;;
esac
])
LDFLAGS="$LDFLAGS $ac_cv_os_ldflags"
])
dnl ======================================================================


dnl ######################################################################
dnl Bugreport name
AC_DEFUN(AMU_PACKAGE_BUGREPORT,
[AC_MSG_CHECKING(bug-reporting address)
AC_DEFINE_UNQUOTED(PACKAGE_BUGREPORT, "$1")
AC_MSG_RESULT(\"$1\")
])
dnl ======================================================================


dnl ######################################################################
dnl Package name
AC_DEFUN(AMU_PACKAGE_NAME,
[AC_MSG_CHECKING(package name)
AC_DEFINE_UNQUOTED(PACKAGE_NAME, "$1")
AC_MSG_RESULT(\"$1\")
])
dnl ======================================================================


dnl ######################################################################
dnl Version of package
AC_DEFUN(AMU_PACKAGE_VERSION,
[AC_MSG_CHECKING(version of package)
AC_DEFINE_UNQUOTED(PACKAGE_VERSION, "$1")
AC_MSG_RESULT(\"$1\")
])
dnl ======================================================================


dnl ######################################################################
dnl AC_SAVE_STATE: save confdefs.h onto dbgcf.h and write $ac_cv_* cache
dnl variables that are known so far.
define(AMU_SAVE_STATE,
AC_MSG_NOTICE(*** SAVING CONFIGURE STATE ***)
if test -f confdefs.h
then
 cp confdefs.h dbgcf.h
fi
[AC_CACHE_SAVE]
)
dnl ======================================================================


dnl ######################################################################
dnl Find the name of the nfs filehandle field in nfs_args_t.
AC_DEFUN(AMU_STRUCT_FIELD_NFS_FH,
[
dnl make sure this is called before macros which depend on it
AC_BEFORE([$0], [AC_TYPE_NFS_FH])
AC_CACHE_CHECK(for the name of the nfs filehandle field in nfs_args_t,
ac_cv_struct_field_nfs_fh,
[
# set to a default value
ac_cv_struct_field_nfs_fh=notfound
# look for name "fh" (most systems)
if test "$ac_cv_struct_field_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfs_args_t nat;
  char *cp = (char *) &(nat.fh);
], ac_cv_struct_field_nfs_fh=fh, ac_cv_struct_field_nfs_fh=notfound)
fi

# look for name "root" (for example Linux)
if test "$ac_cv_struct_field_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfs_args_t nat;
  char *cp = (char *) &(nat.root);
], ac_cv_struct_field_nfs_fh=root, ac_cv_struct_field_nfs_fh=notfound)
fi
])
if test "$ac_cv_struct_field_nfs_fh" != notfound
then
  AC_DEFINE_UNQUOTED(NFS_FH_FIELD, $ac_cv_struct_field_nfs_fh)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find if struct mntent exists anywhere in mount.h or mntent.h headers
AC_DEFUN(AMU_STRUCT_MNTENT,
[
AC_CACHE_CHECK(for struct mntent,
ac_cv_have_struct_mntent,
[
# try to compile a program which may have a definition for the structure
AC_TRY_COMPILE(
AMU_MOUNT_HEADERS
,
[
struct mntent mt;
], ac_cv_have_struct_mntent=yes, ac_cv_have_struct_mntent=no)
])
if test "$ac_cv_have_struct_mntent" = yes
then
  AC_DEFINE(HAVE_STRUCT_MNTENT)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find if struct mnttab exists anywhere in mount.h or mnttab.h headers
AC_DEFUN(AMU_STRUCT_MNTTAB,
[
AC_CACHE_CHECK(for struct mnttab,
ac_cv_have_struct_mnttab,
[
# try to compile a program which may have a definition for the structure
AC_TRY_COMPILE(
AMU_MOUNT_HEADERS
,
[
struct mnttab mt;
], ac_cv_have_struct_mnttab=yes, ac_cv_have_struct_mnttab=no)
])
if test "$ac_cv_have_struct_mnttab" = yes
then
  AC_DEFINE(HAVE_STRUCT_MNTTAB)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find if struct nfs_args exists anywhere in typical headers
AC_DEFUN(AMU_STRUCT_NFS_ARGS,
[
dnl make sure this is called before [AC_TYPE_NFS_FH]
AC_BEFORE([$0], [AC_TYPE_NFS_FH])
AC_BEFORE([$0], [AC_STRUCT_FIELD_NFS_FH])
AC_CACHE_CHECK(for struct nfs_args,
ac_cv_have_struct_nfs_args,
[
# try to compile a program which may have a definition for the structure
# assume not found
ac_cv_have_struct_nfs_args=notfound

# look for "struct irix5_nfs_args" (specially set in conf/nfs_prot/)
if test "$ac_cv_have_struct_nfs_args" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct irix5_nfs_args na;
], ac_cv_have_struct_nfs_args="struct irix5_nfs_args", ac_cv_have_struct_nfs_args=notfound)
fi

# look for "struct aix52_nfs_args" (specially set in conf/nfs_prot/)
if test "$ac_cv_have_struct_nfs_args" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct aix52_nfs_args na;
], ac_cv_have_struct_nfs_args="struct aix52_nfs_args", ac_cv_have_struct_nfs_args=notfound)
fi

# look for "struct aix51_nfs_args" (specially set in conf/nfs_prot/)
if test "$ac_cv_have_struct_nfs_args" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct aix51_nfs_args na;
], ac_cv_have_struct_nfs_args="struct aix51_nfs_args", ac_cv_have_struct_nfs_args=notfound)
fi

# look for "struct aix42_nfs_args" (specially set in conf/nfs_prot/)
if test "$ac_cv_have_struct_nfs_args" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct aix42_nfs_args na;
], ac_cv_have_struct_nfs_args="struct aix42_nfs_args", ac_cv_have_struct_nfs_args=notfound)
fi

# look for "struct nfs_args"
if test "$ac_cv_have_struct_nfs_args" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct nfs_args na;
], ac_cv_have_struct_nfs_args="struct nfs_args", ac_cv_have_struct_nfs_args=notfound)
fi

])

if test "$ac_cv_have_struct_nfs_args" != notfound
then
  AC_DEFINE(HAVE_STRUCT_NFS_ARGS)
  AC_DEFINE_UNQUOTED(nfs_args_t, $ac_cv_have_struct_nfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the structure of an nfs filehandle.
dnl if found, defined am_nfs_fh to it, else leave it undefined.
dnl THE ORDER OF LOOKUPS IN THIS FILE IS VERY IMPORTANT!!!
AC_DEFUN(AMU_STRUCT_NFS_FH,
[
AC_CACHE_CHECK(for type/structure of NFS V2 filehandle,
ac_cv_struct_nfs_fh,
[
# try to compile a program which may have a definition for the type
dnl need a series of compilations, which will test out every possible type
dnl such as struct nfs_fh, fhandle_t, nfsv2fh_t, etc.
# set to a default value
ac_cv_struct_nfs_fh=notfound

# look for "nfs_fh"
if test "$ac_cv_struct_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfs_fh nh;
], ac_cv_struct_nfs_fh="nfs_fh", ac_cv_struct_nfs_fh=notfound)
fi

# look for "struct nfs_fh"
if test "$ac_cv_struct_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct nfs_fh nh;
], ac_cv_struct_nfs_fh="struct nfs_fh", ac_cv_struct_nfs_fh=notfound)
fi

# look for "struct nfssvcfh"
if test "$ac_cv_struct_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct nfssvcfh nh;
], ac_cv_struct_nfs_fh="struct nfssvcfh", ac_cv_struct_nfs_fh=notfound)
fi

# look for "nfsv2fh_t"
if test "$ac_cv_struct_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfsv2fh_t nh;
], ac_cv_struct_nfs_fh="nfsv2fh_t", ac_cv_struct_nfs_fh=notfound)
fi

# look for "fhandle_t"
if test "$ac_cv_struct_nfs_fh" = notfound
then
AC_TRY_COMPILE_NFS(
[ fhandle_t nh;
], ac_cv_struct_nfs_fh="fhandle_t", ac_cv_struct_nfs_fh=notfound)
fi

])

if test "$ac_cv_struct_nfs_fh" != notfound
then
  AC_DEFINE_UNQUOTED(am_nfs_fh, $ac_cv_struct_nfs_fh)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the structure of an NFS V3 filehandle.
dnl if found, defined am_nfs_fh3 to it, else leave it undefined.
AC_DEFUN(AMU_STRUCT_NFS_FH3,
[
AC_CACHE_CHECK(for type/structure of NFS V3 filehandle,
ac_cv_struct_nfs_fh3,
[
# try to compile a program which may have a definition for the type
dnl need a series of compilations, which will test out every possible type
dnl such as struct nfs_fh3, fhandle3_t, nfsv3fh_t, etc.
# set to a default value
ac_cv_struct_nfs_fh3=notfound

# look for "nfs_fh3_freebsd3"
if test "$ac_cv_struct_nfs_fh3" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfs_fh3_freebsd3 nh;
], ac_cv_struct_nfs_fh3="nfs_fh3_freebsd3", ac_cv_struct_nfs_fh3=notfound)
fi

# look for "nfs_fh3"
if test "$ac_cv_struct_nfs_fh3" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfs_fh3 nh;
], ac_cv_struct_nfs_fh3="nfs_fh3", ac_cv_struct_nfs_fh3=notfound)
fi

# look for "struct nfs_fh3"
if test "$ac_cv_struct_nfs_fh3" = notfound
then
AC_TRY_COMPILE_NFS(
[ struct nfs_fh3 nh;
], ac_cv_struct_nfs_fh3="struct nfs_fh3", ac_cv_struct_nfs_fh3=notfound)
fi

# look for "nfsv3fh_t"
if test "$ac_cv_struct_nfs_fh3" = notfound
then
AC_TRY_COMPILE_NFS(
[ nfsv3fh_t nh;
], ac_cv_struct_nfs_fh3="nfsv3fh_t", ac_cv_struct_nfs_fh3=notfound)
fi

# look for "fhandle3_t"
if test "$ac_cv_struct_nfs_fh3" = notfound
then
AC_TRY_COMPILE_NFS(
[ fhandle3_t nh;
], ac_cv_struct_nfs_fh3="fhandle3_t", ac_cv_struct_nfs_fh3=notfound)
fi

])

if test "$ac_cv_struct_nfs_fh3" != notfound
then
  AC_DEFINE_UNQUOTED(am_nfs_fh3, $ac_cv_struct_nfs_fh3)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find if struct nfs_gfs_mount exists anywhere in typical headers
AC_DEFUN(AMU_STRUCT_NFS_GFS_MOUNT,
[
dnl make sure this is called before [AC_TYPE_NFS_FH]
AC_BEFORE([$0], [AC_TYPE_NFS_FH])
AC_BEFORE([$0], [AC_STRUCT_FIELD_NFS_FH])
AC_CACHE_CHECK(for struct nfs_gfs_mount,
ac_cv_have_struct_nfs_gfs_mount,
[
# try to compile a program which may have a definition for the structure
AC_TRY_COMPILE_NFS(
[ struct nfs_gfs_mount ngm;
], ac_cv_have_struct_nfs_gfs_mount=yes, ac_cv_have_struct_nfs_gfs_mount=no)
])
if test "$ac_cv_have_struct_nfs_gfs_mount" = yes
then
  AC_DEFINE(HAVE_STRUCT_NFS_GFS_MOUNT)
  AC_DEFINE(nfs_args_t, struct nfs_gfs_mount)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Compile a program with <any>FS headers to try and find a feature.
dnl The headers part are fixed.  Only three arguments are allowed:
dnl [$1] is the program to compile (2nd arg to AC_TRY_COMPILE)
dnl [$2] action to take if the program compiled (3rd arg to AC_TRY_COMPILE)
dnl [$3] action to take if program did not compile (4rd arg to AC_TRY_COMPILE)
AC_DEFUN(AC_TRY_COMPILE_ANYFS,
[# try to compile a program which may have a definition for a structure
AC_TRY_COMPILE(
[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif /* HAVE_SYS_ERRNO_H */
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else /* not TIME_WITH_SYS_TIME */
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else /* not HAVE_SYS_TIME_H */
#  include <time.h>
# endif /* not HAVE_SYS_TIME_H */
#endif /* not TIME_WITH_SYS_TIME */

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#ifdef HAVE_SYS_TIUSER_H
# include <sys/tiuser.h>
#endif /* HAVE_SYS_TIUSER_H */

#ifdef HAVE_SYS_MOUNT_H
# ifndef NFSCLIENT
#  define NFSCLIENT
# endif /* not NFSCLIENT */
# ifndef PCFS
#  define PCFS
# endif /* not PCFS */
# ifndef LOFS
#  define LOFS
# endif /* not LOFS */
# ifndef RFS
#  define RFS
# endif /* not RFS */
# ifndef MSDOSFS
#  define MSDOSFS
# endif /* not MSDOSFS */
# ifndef MFS
#  define MFS 1
# endif /* not MFS */
# ifndef CD9660
#  define CD9660
# endif /* not CD9660 */
# ifndef NFS
#  define NFS
# endif /* not NFS */
# include <sys/mount.h>
#endif /* HAVE_SYS_MOUNT_H */

#ifdef HAVE_SYS_VMOUNT_H
# include <sys/vmount.h>
#endif /* HAVE_SYS_VMOUNT_H */

/*
 * There is no point in including this on a glibc2 system
 * we're only asking for trouble
 */
#if defined HAVE_LINUX_FS_H && (!defined __GLIBC__ || __GLIBC__ < 2)
/*
 * There's a conflict of definitions on redhat alpha linux between
 * <netinet/in.h> and <linux/fs.h>.
 * Also a conflict in definitions of ntohl/htonl in RH-5.1 sparc64
 * between <netinet/in.h> and <linux/byteorder/generic.h> (2.1 kernels).
 */
# ifdef HAVE_SOCKETBITS_H
#  define _LINUX_SOCKET_H
#  undef BLKFLSBUF
#  undef BLKGETSIZE
#  undef BLKRAGET
#  undef BLKRASET
#  undef BLKROGET
#  undef BLKROSET
#  undef BLKRRPART
#  undef MS_MGC_VAL
#  undef MS_RMT_MASK
# endif /* HAVE_SOCKETBITS_H */
# ifdef HAVE_LINUX_POSIX_TYPES_H
#  include <linux/posix_types.h>
# endif /* HAVE_LINUX_POSIX_TYPES_H */
# ifndef _LINUX_BYTEORDER_GENERIC_H
#  define _LINUX_BYTEORDER_GENERIC_H
# endif /* _LINUX_BYTEORDER_GENERIC_H */
# ifndef _LINUX_STRING_H_
#  define _LINUX_STRING_H_
# endif /* not _LINUX_STRING_H_ */
# ifdef HAVE_LINUX_KDEV_T_H
#  define __KERNEL__
#  include <linux/kdev_t.h>
#  undef __KERNEL__
# endif /* HAVE_LINUX_KDEV_T_H */
# ifdef HAVE_LINUX_LIST_H
#  define __KERNEL__
#  include <linux/list.h>
#  undef __KERNEL__
# endif /* HAVE_LINUX_LIST_H */
# include <linux/fs.h>
#endif /* HAVE_LINUX_FS_H && (!__GLIBC__ || __GLIBC__ < 2) */

#ifdef HAVE_SYS_FS_AUTOFS_H
# include <sys/fs/autofs.h>
#endif /* HAVE_SYS_FS_AUTOFS_H */
#ifdef HAVE_SYS_FS_CACHEFS_FS_H
# include <sys/fs/cachefs_fs.h>
#endif /* HAVE_SYS_FS_CACHEFS_FS_H */

#ifdef HAVE_SYS_FS_PC_FS_H
# include <sys/fs/pc_fs.h>
#endif /* HAVE_SYS_FS_PC_FS_H */
#ifdef HAVE_MSDOSFS_MSDOSFSMOUNT_H
# include <msdosfs/msdosfsmount.h>
#endif /* HAVE_MSDOSFS_MSDOSFSMOUNT_H */

#ifdef HAVE_SYS_FS_TMP_H
# include <sys/fs/tmp.h>
#endif /* HAVE_SYS_FS_TMP_H */

#ifdef HAVE_UFS_UFS_MOUNT_H
# include <ufs/ufs_mount.h>
#endif /* HAVE_UFS_UFS_MOUNT_H */
#ifdef HAVE_UFS_UFS_UFSMOUNT_H
# ifndef MAXQUOTAS
#  define MAXQUOTAS     2
# endif /* not MAXQUOTAS */
struct netexport { int this_is_SO_wrong; }; /* for bsdi-2.1 */
/* netbsd-1.4 does't protect <ufs/ufs/ufsmount.h> */
# ifndef _UFS_UFS_UFSMOUNT_H
#  include <ufs/ufs/ufsmount.h>
#  define _UFS_UFS_UFSMOUNT_H
# endif /* not _UFS_UFS_UFSMOUNT_H */
#endif /* HAVE_UFS_UFS_UFSMOUNT_H */
#ifdef HAVE_SYS_FS_UFS_MOUNT_H
# include <sys/fs/ufs_mount.h>
#endif /* HAVE_SYS_FS_UFS_MOUNT_H */
#ifdef HAVE_SYS_FS_EFS_CLNT_H
# include <sys/fs/efs_clnt.h>
#endif /* HAVE_SYS_FS_EFS_CLNT_H */
#ifdef HAVE_SYS_FS_XFS_CLNT_H
# include <sys/fs/xfs_clnt.h>
#endif /* HAVE_SYS_FS_XFS_CLNT_H */

#ifdef HAVE_CDFS_CDFS_MOUNT_H
# include <cdfs/cdfs_mount.h>
#endif /* HAVE_CDFS_CDFS_MOUNT_H */
#ifdef HAVE_HSFS_HSFS_H
# include <hsfs/hsfs.h>
#endif /* HAVE_HSFS_HSFS_H */
#ifdef HAVE_CDFS_CDFSMOUNT_H
# include <cdfs/cdfsmount.h>
#endif /* HAVE_CDFS_CDFSMOUNT_H */
#ifdef HAVE_ISOFS_CD9660_CD9660_MOUNT_H
# include <isofs/cd9660/cd9660_mount.h>
#endif /* HAVE_ISOFS_CD9660_CD9660_MOUNT_H */
], [$1], [$2], [$3])
])
dnl ======================================================================


dnl ######################################################################
dnl Compile a program with NFS headers to try and find a feature.
dnl The headers part are fixed.  Only three arguments are allowed:
dnl [$1] is the program to compile (2nd arg to AC_TRY_COMPILE)
dnl [$2] action to take if the program compiled (3rd arg to AC_TRY_COMPILE)
dnl [$3] action to take if program did not compile (4rd arg to AC_TRY_COMPILE)
AC_DEFUN(AC_TRY_COMPILE_NFS,
[# try to compile a program which may have a definition for a structure
AC_TRY_COMPILE(
AMU_MOUNT_HEADERS
, [$1], [$2], [$3])
])
dnl ======================================================================


dnl ######################################################################
dnl Compile a program with RPC headers to try and find a feature.
dnl The headers part are fixed.  Only three arguments are allowed:
dnl [$1] is the program to compile (2nd arg to AC_TRY_COMPILE)
dnl [$2] action to take if the program compiled (3rd arg to AC_TRY_COMPILE)
dnl [$3] action to take if program did not compile (4rd arg to AC_TRY_COMPILE)
AC_DEFUN(AC_TRY_COMPILE_RPC,
[# try to compile a program which may have a definition for a structure
AC_TRY_COMPILE(
[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_RPC_RPC_H
# include <rpc/rpc.h>
#endif /* HAVE_RPC_RPC_H */
/* Prevent multiple inclusion on Ultrix 4 */
#if defined(HAVE_RPC_XDR_H) && !defined(__XDR_HEADER__)
# include <rpc/xdr.h>
#endif /* defined(HAVE_RPC_XDR_H) && !defined(__XDR_HEADER__) */
], [$1], [$2], [$3])
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct type for the 5th argument to authunix_create()
AC_DEFUN(AMU_TYPE_AUTH_CREATE_GIDLIST,
[
AC_CACHE_CHECK(argument type of 5rd argument to authunix_create(),
ac_cv_auth_create_gidlist,
[
# select the correct type
case "${host_os_name}" in
	sunos[[34]]* | bsdi2* | sysv4* | hpux10.10 | ultrix* | aix4* )
		ac_cv_auth_create_gidlist="int" ;;
	* )
		ac_cv_auth_create_gidlist="gid_t" ;;
esac
])
AC_DEFINE_UNQUOTED(AUTH_CREATE_GIDLIST_TYPE, $ac_cv_auth_create_gidlist)
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for AUTOFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_AUTOFS_ARGS,
[
AC_CACHE_CHECK(for structure type of autofs mount(2) arguments,
ac_cv_type_autofs_args,
[
# set to a default value
ac_cv_type_autofs_args=notfound

# look for "struct auto_args"
if test "$ac_cv_type_autofs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct auto_args a;
], ac_cv_type_autofs_args="struct auto_args", ac_cv_type_autofs_args=notfound)
fi

# look for "struct autofs_args"
if test "$ac_cv_type_autofs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct autofs_args a;
], ac_cv_type_autofs_args="struct autofs_args", ac_cv_type_autofs_args=notfound)
fi

])
if test "$ac_cv_type_autofs_args" != notfound
then
  AC_DEFINE_UNQUOTED(autofs_args_t, $ac_cv_type_autofs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for CACHEFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_CACHEFS_ARGS,
[
AC_CACHE_CHECK(for structure type of cachefs mount(2) arguments,
ac_cv_type_cachefs_args,
[
# set to a default value
ac_cv_type_cachefs_args=notfound
# look for "struct cachefs_mountargs"
if test "$ac_cv_type_cachefs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct cachefs_mountargs a;
], ac_cv_type_cachefs_args="struct cachefs_mountargs", ac_cv_type_cachefs_args=notfound)
fi
])
if test "$ac_cv_type_cachefs_args" != notfound
then
  AC_DEFINE_UNQUOTED(cachefs_args_t, $ac_cv_type_cachefs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for CDFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_CDFS_ARGS,
[
AC_CACHE_CHECK(for structure type of cdfs mount(2) arguments,
ac_cv_type_cdfs_args,
[
# set to a default value
ac_cv_type_cdfs_args=notfound

# look for "struct iso_args"
if test "$ac_cv_type_cdfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct iso_args a;
], ac_cv_type_cdfs_args="struct iso_args", ac_cv_type_cdfs_args=notfound)
fi

# look for "struct iso9660_args"
if test "$ac_cv_type_cdfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct iso9660_args a;
], ac_cv_type_cdfs_args="struct iso9660_args", ac_cv_type_cdfs_args=notfound)
fi

# look for "struct cdfs_args"
if test "$ac_cv_type_cdfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct cdfs_args a;
], ac_cv_type_cdfs_args="struct cdfs_args", ac_cv_type_cdfs_args=notfound)
fi

# look for "struct hsfs_args"
if test "$ac_cv_type_cdfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct hsfs_args a;
], ac_cv_type_cdfs_args="struct hsfs_args", ac_cv_type_cdfs_args=notfound)
fi

# look for "struct iso_specific" (ultrix)
if test "$ac_cv_type_cdfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct iso_specific a;
], ac_cv_type_cdfs_args="struct iso_specific", ac_cv_type_cdfs_args=notfound)
fi

])
if test "$ac_cv_type_cdfs_args" != notfound
then
  AC_DEFINE_UNQUOTED(cdfs_args_t, $ac_cv_type_cdfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for EFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_EFS_ARGS,
[
AC_CACHE_CHECK(for structure type of efs mount(2) arguments,
ac_cv_type_efs_args,
[
# set to a default value
ac_cv_type_efs_args=notfound

# look for "struct efs_args"
if test "$ac_cv_type_efs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct efs_args a;
], ac_cv_type_efs_args="struct efs_args", ac_cv_type_efs_args=notfound)
fi

])
if test "$ac_cv_type_efs_args" != notfound
then
  AC_DEFINE_UNQUOTED(efs_args_t, $ac_cv_type_efs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for LOFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_LOFS_ARGS,
[
AC_CACHE_CHECK(for structure type of lofs mount(2) arguments,
ac_cv_type_lofs_args,
[
# set to a default value
ac_cv_type_lofs_args=notfound
# look for "struct lofs_args"
if test "$ac_cv_type_lofs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct lofs_args a;
], ac_cv_type_lofs_args="struct lofs_args", ac_cv_type_lofs_args=notfound)
fi
# look for "struct lo_args"
if test "$ac_cv_type_lofs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct lo_args a;
], ac_cv_type_lofs_args="struct lo_args", ac_cv_type_lofs_args=notfound)
fi
])
if test "$ac_cv_type_lofs_args" != notfound
then
  AC_DEFINE_UNQUOTED(lofs_args_t, $ac_cv_type_lofs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for MFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_MFS_ARGS,
[
AC_CACHE_CHECK(for structure type of mfs mount(2) arguments,
ac_cv_type_mfs_args,
[
# set to a default value
ac_cv_type_mfs_args=notfound
# look for "struct mfs_args"
if test "$ac_cv_type_mfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct mfs_args a;
], ac_cv_type_mfs_args="struct mfs_args", ac_cv_type_mfs_args=notfound)
fi
])
if test "$ac_cv_type_mfs_args" != notfound
then
  AC_DEFINE_UNQUOTED(mfs_args_t, $ac_cv_type_mfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for PC/FS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_PCFS_ARGS,
[
AC_CACHE_CHECK(for structure type of pcfs mount(2) arguments,
ac_cv_type_pcfs_args,
[
# set to a default value
ac_cv_type_pcfs_args=notfound

# look for "struct msdos_args"
if test "$ac_cv_type_pcfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct msdos_args a;
], ac_cv_type_pcfs_args="struct msdos_args", ac_cv_type_pcfs_args=notfound)
fi

# look for "struct pc_args"
if test "$ac_cv_type_pcfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct pc_args a;
], ac_cv_type_pcfs_args="struct pc_args", ac_cv_type_pcfs_args=notfound)
fi

# look for "struct pcfs_args"
if test "$ac_cv_type_pcfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct pcfs_args a;
], ac_cv_type_pcfs_args="struct pcfs_args", ac_cv_type_pcfs_args=notfound)
fi

# look for "struct msdosfs_args"
if test "$ac_cv_type_pcfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct msdosfs_args a;
], ac_cv_type_pcfs_args="struct msdosfs_args", ac_cv_type_pcfs_args=notfound)
fi

])

if test "$ac_cv_type_pcfs_args" != notfound
then
  AC_DEFINE_UNQUOTED(pcfs_args_t, $ac_cv_type_pcfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct type for the 6th argument to recvfrom()
AC_DEFUN(AMU_TYPE_RECVFROM_FROMLEN,
[
AC_CACHE_CHECK(non-pointer type of 6th (fromlen) argument to recvfrom(),
ac_cv_recvfrom_fromlen,
[
# select the correct type
case "${host_os}" in
	aix[[1-3]]* )
		ac_cv_recvfrom_fromlen="int" ;;
	aix* )
		ac_cv_recvfrom_fromlen="size_t" ;;
	* )
		ac_cv_recvfrom_fromlen="int" ;;
esac
])
AC_DEFINE_UNQUOTED(RECVFROM_FROMLEN_TYPE, $ac_cv_recvfrom_fromlen)
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for RFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_RFS_ARGS,
[
AC_CACHE_CHECK(for structure type of rfs mount(2) arguments,
ac_cv_type_rfs_args,
[
# set to a default value
ac_cv_type_rfs_args=notfound
# look for "struct rfs_args"
if test "$ac_cv_type_rfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct rfs_args a;
], ac_cv_type_rfs_args="struct rfs_args", ac_cv_type_rfs_args=notfound)
fi
])
if test "$ac_cv_type_rfs_args" != notfound
then
  AC_DEFINE_UNQUOTED(rfs_args_t, $ac_cv_type_rfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the type of the 3rd argument (in) to svc_sendreply() call
AC_DEFUN(AMU_TYPE_SVC_IN_ARG,
[
AC_CACHE_CHECK(for type of 3rd arg ('in') arg to svc_sendreply(),
ac_cv_type_svc_in_arg,
[
# try to compile a program which may have a definition for the type
dnl need a series of compilations, which will test out every possible type
dnl such as caddr_t, char *, etc.
# set to a default value
ac_cv_type_svc_in_arg=notfound
# look for "caddr_t"
if test "$ac_cv_type_svc_in_arg" = notfound
then
AC_TRY_COMPILE_RPC(
[ SVCXPRT *SX;
  xdrproc_t xp;
  caddr_t p;
  svc_sendreply(SX, xp, p);
], ac_cv_type_svc_in_arg="caddr_t", ac_cv_type_svc_in_arg=notfound)
fi
# look for "char *"
if test "$ac_cv_type_svc_in_arg" = notfound
then
AC_TRY_COMPILE_RPC(
[ SVCXPRT *SX;
  xdrproc_t xp;
  char *p;
  svc_sendreply(SX, xp, p);
], ac_cv_type_svc_in_arg="char *", ac_cv_type_svc_in_arg=notfound)
fi
])
if test "$ac_cv_type_svc_in_arg" != notfound
then
  AC_DEFINE_UNQUOTED(SVC_IN_ARG_TYPE, $ac_cv_type_svc_in_arg)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check for type of time_t (usually in <sys/types.h>)
AC_DEFUN(AMU_TYPE_TIME_T,
[AC_CHECK_TYPE(time_t, long)])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for TMPFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_TMPFS_ARGS,
[
AC_CACHE_CHECK(for structure type of tmpfs mount(2) arguments,
ac_cv_type_tmpfs_args,
[
# set to a default value
ac_cv_type_tmpfs_args=notfound
# look for "struct tmpfs_args"
if test "$ac_cv_type_tmpfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct tmpfs_args a;
], ac_cv_type_tmpfs_args="struct tmpfs_args", ac_cv_type_tmpfs_args=notfound)
fi
])
if test "$ac_cv_type_tmpfs_args" != notfound
then
  AC_DEFINE_UNQUOTED(tmpfs_args_t, $ac_cv_type_tmpfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for UFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_UFS_ARGS,
[
AC_CACHE_CHECK(for structure type of ufs mount(2) arguments,
ac_cv_type_ufs_args,
[
# set to a default value
ac_cv_type_ufs_args=notfound

# look for "struct ufs_args"
if test "$ac_cv_type_ufs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct ufs_args a;
], ac_cv_type_ufs_args="struct ufs_args", ac_cv_type_ufs_args=notfound)
fi

# look for "struct efs_args" (irix)
if test "$ac_cv_type_ufs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct efs_args a;
], ac_cv_type_ufs_args="struct efs_args", ac_cv_type_ufs_args=notfound)
fi

# look for "struct ufs_specific" (ultrix)
if test "$ac_cv_type_ufs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct ufs_specific a;
], ac_cv_type_ufs_args="struct ufs_specific", ac_cv_type_ufs_args=notfound)
fi

])
if test "$ac_cv_type_ufs_args" != notfound
then
  AC_DEFINE_UNQUOTED(ufs_args_t, $ac_cv_type_ufs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check for type of xdrproc_t (usually in <rpc/xdr.h>)
AC_DEFUN(AMU_TYPE_XDRPROC_T,
[
AC_CACHE_CHECK(for xdrproc_t,
ac_cv_type_xdrproc_t,
[
# try to compile a program which may have a definition for the type
dnl need a series of compilations, which will test out every possible type
# look for "xdrproc_t"
AC_TRY_COMPILE_RPC(
[ xdrproc_t xdr_int;
], ac_cv_type_xdrproc_t=yes, ac_cv_type_xdrproc_t=no)
])
if test "$ac_cv_type_xdrproc_t" = yes
then
  AC_DEFINE_UNQUOTED(XDRPROC_T_TYPE, xdrproc_t)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl Find the correct type for XFS mount(2) arguments structure
AC_DEFUN(AMU_TYPE_XFS_ARGS,
[
AC_CACHE_CHECK(for structure type of xfs mount(2) arguments,
ac_cv_type_xfs_args,
[
# set to a default value
ac_cv_type_xfs_args=notfound

# look for "struct xfs_args"
if test "$ac_cv_type_xfs_args" = notfound
then
AC_TRY_COMPILE_ANYFS(
[ struct xfs_args a;
], ac_cv_type_xfs_args="struct xfs_args", ac_cv_type_xfs_args=notfound)
fi

])
if test "$ac_cv_type_xfs_args" != notfound
then
  AC_DEFINE_UNQUOTED(xfs_args_t, $ac_cv_type_xfs_args)
fi
])
dnl ======================================================================


dnl ######################################################################
dnl check the correct type for the 3rd argument to yp_order()
AC_DEFUN(AMU_TYPE_YP_ORDER_OUTORDER,
[
AC_CACHE_CHECK(pointer type of 3rd argument to yp_order(),
ac_cv_yp_order_outorder,
[
# select the correct type
case "${host_os}" in
	aix[[1-3]]* | aix4.[[0-2]]* | sunos[[34]]* | solaris1* )
		ac_cv_yp_order_outorder=int ;;
	solaris* | svr4* | sysv4* | sunos* | hpux* | aix* )
		ac_cv_yp_order_outorder="unsigned long" ;;
	osf* )
		# DU4 man page is wrong, headers are right
		ac_cv_yp_order_outorder="unsigned int" ;;
	* )
		ac_cv_yp_order_outorder=int ;;
esac
])
AC_DEFINE_UNQUOTED(YP_ORDER_OUTORDER_TYPE, $ac_cv_yp_order_outorder)
])
dnl ======================================================================


dnl ######################################################################
dnl Do we want to compile with "ADDON" support? (hesiod, ldap, etc.)
AC_DEFUN(AMU_WITH_ADDON,
[AC_MSG_CHECKING([if $1 is wanted])
ac_upcase=`echo $1|tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
AC_ARG_WITH($1,
 AC_HELP_STRING([--with-$1],
		[enable $2 support (default=yes if found)]
),[
if test "$withval" = "yes"; then
  with_$1=yes
elif test "$withval" = "no"; then
  with_$1=no
else
  AC_MSG_ERROR(please use \"yes\" or \"no\" with --with-$1)
fi
],[
with_$1=yes
])
if test "$with_$1" = "yes"
then
  AC_MSG_RESULT([yes, will enable if all libraries are found])
else
  AC_MSG_RESULT([no])
fi
])


dnl ######################################################################
dnl end of aclocal.m4 for am-utils-6.x

# Like AC_CONFIG_HEADER, but automatically create stamp file. -*- Autoconf -*-

# Copyright 1996, 1997, 2000, 2001 Free Software Foundation, Inc.

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

AC_PREREQ([2.52])

# serial 6

# When config.status generates a header, we must update the stamp-h file.
# This file resides in the same directory as the config header
# that is generated.  We must strip everything past the first ":",
# and everything past the last "/".

# _AM_DIRNAME(PATH)
# -----------------
# Like AS_DIRNAME, only do it during macro expansion
AC_DEFUN([_AM_DIRNAME],
       [m4_if(regexp([$1], [^.*[^/]//*[^/][^/]*/*$]), -1,
	      m4_if(regexp([$1], [^//\([^/]\|$\)]), -1,
		    m4_if(regexp([$1], [^/.*]), -1,
			  [.],
			  patsubst([$1], [^\(/\).*], [\1])),
		    patsubst([$1], [^\(//\)\([^/].*\|$\)], [\1])),
	      patsubst([$1], [^\(.*[^/]\)//*[^/][^/]*/*$], [\1]))[]dnl
])# _AM_DIRNAME


# The stamp files are numbered to have different names.
# We could number them on a directory basis, but that's additional
# complications, let's have a unique counter.
m4_define([_AM_STAMP_Count], [0])


# _AM_STAMP(HEADER)
# -----------------
# The name of the stamp file for HEADER.
AC_DEFUN([_AM_STAMP],
[m4_define([_AM_STAMP_Count], m4_incr(_AM_STAMP_Count))dnl
AS_ESCAPE(_AM_DIRNAME(patsubst([$1],
                               [:.*])))/stamp-h[]_AM_STAMP_Count])


# _AM_CONFIG_HEADER(HEADER[:SOURCES], COMMANDS, INIT-COMMANDS)
# ------------------------------------------------------------
# We used to try to get a real timestamp in stamp-h.  But the fear is that
# that will cause unnecessary cvs conflicts.
AC_DEFUN([_AM_CONFIG_HEADER],
[# Add the stamp file to the list of files AC keeps track of,
# along with our hook.
AC_CONFIG_HEADERS([$1],
                  [# update the timestamp
echo 'timestamp for $1' >"_AM_STAMP([$1])"
$2],
                  [$3])
])# _AM_CONFIG_HEADER


# AM_CONFIG_HEADER(HEADER[:SOURCES]..., COMMANDS, INIT-COMMANDS)
# --------------------------------------------------------------
AC_DEFUN([AM_CONFIG_HEADER],
[AC_FOREACH([_AM_File], [$1], [_AM_CONFIG_HEADER(_AM_File, [$2], [$3])])
])# AM_CONFIG_HEADER

# Do all the work for Automake.                            -*- Autoconf -*-

# This macro actually does too much some checks are only needed if
# your package does certain things.  But this isn't really a big deal.

# Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.

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

# serial 8

# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...


AC_PREREQ([2.52])

# Autoconf 2.50 wants to disallow AM_ names.  We explicitly allow
# the ones we care about.
m4_pattern_allow([^AM_[A-Z]+FLAGS$])dnl

# AM_INIT_AUTOMAKE(PACKAGE, VERSION, [NO-DEFINE])
# AM_INIT_AUTOMAKE([OPTIONS])
# -----------------------------------------------
# The call with PACKAGE and VERSION arguments is the old style
# call (pre autoconf-2.50), which is being phased out.  PACKAGE
# and VERSION should now be passed to AC_INIT and removed from
# the call to AM_INIT_AUTOMAKE.
# We support both call styles for the transition.  After
# the next Automake release, Autoconf can make the AC_INIT
# arguments mandatory, and then we can depend on a new Autoconf
# release and drop the old call support.
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AM_SET_CURRENT_AUTOMAKE_VERSION])dnl
 AC_REQUIRE([AC_PROG_INSTALL])dnl
# test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" &&
   test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi

# Define the identity of the package.
dnl Distinguish between old-style and new-style calls.
m4_ifval([$2],
[m4_ifval([$3], [_AM_SET_OPTION([no-define])])dnl
 AC_SUBST([PACKAGE], [$1])dnl
 AC_SUBST([VERSION], [$2])],
[_AM_SET_OPTIONS([$1])dnl
 AC_SUBST([PACKAGE], [AC_PACKAGE_TARNAME])dnl
 AC_SUBST([VERSION], [AC_PACKAGE_VERSION])])dnl

_AM_IF_OPTION([no-define],,
[AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
 AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])])dnl

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG(ACLOCAL, aclocal-${am__api_version})
AM_MISSING_PROG(AUTOCONF, autoconf)
AM_MISSING_PROG(AUTOMAKE, automake-${am__api_version})
AM_MISSING_PROG(AUTOHEADER, autoheader)
AM_MISSING_PROG(MAKEINFO, makeinfo)
AM_MISSING_PROG(AMTAR, tar)
AM_PROG_INSTALL_SH
AM_PROG_INSTALL_STRIP
# We need awk for the "check" target.  The system "awk" is bad on
# some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl

_AM_IF_OPTION([no-dependencies],,
[AC_PROVIDE_IFELSE([AC_PROG_][CC],
                  [_AM_DEPENDENCIES(CC)],
                  [define([AC_PROG_][CC],
                          defn([AC_PROG_][CC])[_AM_DEPENDENCIES(CC)])])dnl
AC_PROVIDE_IFELSE([AC_PROG_][CXX],
                  [_AM_DEPENDENCIES(CXX)],
                  [define([AC_PROG_][CXX],
                          defn([AC_PROG_][CXX])[_AM_DEPENDENCIES(CXX)])])dnl
])
])

# Copyright 2002  Free Software Foundation, Inc.

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

# AM_AUTOMAKE_VERSION(VERSION)
# ----------------------------
# Automake X.Y traces this macro to ensure aclocal.m4 has been
# generated from the m4 files accompanying Automake X.Y.
AC_DEFUN([AM_AUTOMAKE_VERSION],[am__api_version="1.6"])

# AM_SET_CURRENT_AUTOMAKE_VERSION
# -------------------------------
# Call AM_AUTOMAKE_VERSION so it can be traced.
# This function is AC_REQUIREd by AC_INIT_AUTOMAKE.
AC_DEFUN([AM_SET_CURRENT_AUTOMAKE_VERSION],
	 [AM_AUTOMAKE_VERSION([1.6.3])])

# Helper functions for option handling.                    -*- Autoconf -*-

# Copyright 2001, 2002  Free Software Foundation, Inc.

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

# serial 2

# _AM_MANGLE_OPTION(NAME)
# -----------------------
AC_DEFUN([_AM_MANGLE_OPTION],
[[_AM_OPTION_]m4_bpatsubst($1, [[^a-zA-Z0-9_]], [_])])

# _AM_SET_OPTION(NAME)
# ------------------------------
# Set option NAME.  Presently that only means defining a flag for this option.
AC_DEFUN([_AM_SET_OPTION],
[m4_define(_AM_MANGLE_OPTION([$1]), 1)])

# _AM_SET_OPTIONS(OPTIONS)
# ----------------------------------
# OPTIONS is a space-separated list of Automake options.
AC_DEFUN([_AM_SET_OPTIONS],
[AC_FOREACH([_AM_Option], [$1], [_AM_SET_OPTION(_AM_Option)])])

# _AM_IF_OPTION(OPTION, IF-SET, [IF-NOT-SET])
# -------------------------------------------
# Execute IF-SET if OPTION is set, IF-NOT-SET otherwise.
AC_DEFUN([_AM_IF_OPTION],
[m4_ifset(_AM_MANGLE_OPTION([$1]), [$2], [$3])])

#
# Check to make sure that the build environment is sane.
#

# Copyright 1996, 1997, 2000, 2001 Free Software Foundation, Inc.

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

# serial 3

# AM_SANITY_CHECK
# ---------------
AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftest.file
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftest.file 2> /dev/null`
   if test "$[*]" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftest.file`
   fi
   rm -f conftest.file
   if test "$[*]" != "X $srcdir/configure conftest.file" \
      && test "$[*]" != "X conftest.file $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "$[2]" = conftest.file
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
AC_MSG_RESULT(yes)])

#  -*- Autoconf -*-


# Copyright 1997, 1999, 2000, 2001 Free Software Foundation, Inc.

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

# serial 3

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])


# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it supports --run.
# If it does, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
test x"${MISSING+set}" = xset || MISSING="\${SHELL} $am_aux_dir/missing"
# Use eval to expand $SHELL
if eval "$MISSING --run true"; then
  am_missing_run="$MISSING --run "
else
  am_missing_run=
  AC_MSG_WARN([`missing' script is too old or missing])
fi
])

# AM_AUX_DIR_EXPAND

# Copyright 2001 Free Software Foundation, Inc.

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

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to `$srcdir/foo'.  In other projects, it is set to
# `$srcdir', `$srcdir/..', or `$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is `.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

# Rely on autoconf to set up CDPATH properly.
AC_PREREQ([2.50])

AC_DEFUN([AM_AUX_DIR_EXPAND], [
# expand $ac_aux_dir to an absolute path
am_aux_dir=`cd $ac_aux_dir && pwd`
])

# AM_PROG_INSTALL_SH
# ------------------
# Define $install_sh.

# Copyright 2001 Free Software Foundation, Inc.

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

AC_DEFUN([AM_PROG_INSTALL_SH],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
install_sh=${install_sh-"$am_aux_dir/install-sh"}
AC_SUBST(install_sh)])

# AM_PROG_INSTALL_STRIP

# Copyright 2001 Free Software Foundation, Inc.

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

# One issue with vendor `install' (even GNU) is that you can't
# specify the program used to strip binaries.  This is especially
# annoying in cross-compiling environments, where the build's strip
# is unlikely to handle the host's binaries.
# Fortunately install-sh will honor a STRIPPROG variable, so we
# always use install-sh in `make install-strip', and initialize
# STRIPPROG with the value of the STRIP variable (set by the user).
AC_DEFUN([AM_PROG_INSTALL_STRIP],
[AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
# Installed binaries are usually stripped using `strip' when the user
# run `make install-strip'.  However `strip' might not be the right
# tool to use in cross-compilation environments, therefore Automake
# will honor the `STRIP' environment variable to overrule this program.
dnl Don't test for $cross_compiling = yes, because it might be `maybe'.
if test "$cross_compiling" != no; then
  AC_CHECK_TOOL([STRIP], [strip], :)
fi
INSTALL_STRIP_PROGRAM="\${SHELL} \$(install_sh) -c -s"
AC_SUBST([INSTALL_STRIP_PROGRAM])])

# serial 4						-*- Autoconf -*-

# Copyright 1999, 2000, 2001 Free Software Foundation, Inc.

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


# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...



# _AM_DEPENDENCIES(NAME)
# ----------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX", "GCJ", or "OBJC".
# We try a few techniques and use that to set a single cache variable.
#
# We don't AC_REQUIRE the corresponding AC_PROG_CC since the latter was
# modified to invoke _AM_DEPENDENCIES(CC); we would have a circular
# dependency, and given that the user is not expected to run this macro,
# just rely on AC_PROG_CC.
AC_DEFUN([_AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])dnl
AC_REQUIRE([AM_MAKE_INCLUDE])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl

ifelse([$1], CC,   [depcc="$CC"   am_compiler_list=],
       [$1], CXX,  [depcc="$CXX"  am_compiler_list=],
       [$1], OBJC, [depcc="$OBJC" am_compiler_list='gcc3 gcc'],
       [$1], GCJ,  [depcc="$GCJ"  am_compiler_list='gcc3 gcc'],
                   [depcc="$$1"   am_compiler_list=])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP_TRUE" && test -f "$am_depcomp"; then
  # We make a subdir and do the tests there.  Otherwise we can end up
  # making bogus files that we don't know about and never remove.  For
  # instance it was reported that on HP-UX the gcc test will end up
  # making a dummy file named `D' -- because `-MD' means `put the output
  # in D'.
  mkdir conftest.dir
  # Copy depcomp to subdir because otherwise we won't find it if we're
  # using a relative directory.
  cp "$am_depcomp" conftest.dir
  cd conftest.dir

  am_cv_$1_dependencies_compiler_type=none
  if test "$am_compiler_list" = ""; then
     am_compiler_list=`sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < ./depcomp`
  fi
  for depmode in $am_compiler_list; do
    # We need to recreate these files for each test, as the compiler may
    # overwrite some of them when testing with obscure command lines.
    # This happens at least with the AIX C compiler.
    echo '#include "conftest.h"' > conftest.c
    echo 'int i;' > conftest.h
    echo "${am__include} ${am__quote}conftest.Po${am__quote}" > confmf

    case $depmode in
    nosideeffect)
      # after this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    none) break ;;
    esac
    # We check with `-c' and `-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle `-M -o', and we need to detect this.
    if depmode=$depmode \
       source=conftest.c object=conftest.o \
       depfile=conftest.Po tmpdepfile=conftest.TPo \
       $SHELL ./depcomp $depcc -c conftest.c -o conftest.o >/dev/null 2>&1 &&
       grep conftest.h conftest.Po > /dev/null 2>&1 &&
       ${MAKE-make} -s -f confmf > /dev/null 2>&1; then
      am_cv_$1_dependencies_compiler_type=$depmode
      break
    fi
  done

  cd ..
  rm -rf conftest.dir
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
AC_SUBST([$1DEPMODE], [depmode=$am_cv_$1_dependencies_compiler_type])
])


# AM_SET_DEPDIR
# -------------
# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in _AM_DEPENDENCIES
AC_DEFUN([AM_SET_DEPDIR],
[rm -f .deps 2>/dev/null
mkdir .deps 2>/dev/null
if test -d .deps; then
  DEPDIR=.deps
else
  # MS-DOS does not allow filenames that begin with a dot.
  DEPDIR=_deps
fi
rmdir .deps 2>/dev/null
AC_SUBST([DEPDIR])
])


# AM_DEP_TRACK
# ------------
AC_DEFUN([AM_DEP_TRACK],
[AC_ARG_ENABLE(dependency-tracking,
[  --disable-dependency-tracking Speeds up one-time builds
  --enable-dependency-tracking  Do not reject slow dependency extractors])
if test "x$enable_dependency_tracking" != xno; then
  am_depcomp="$ac_aux_dir/depcomp"
  AMDEPBACKSLASH='\'
fi
AM_CONDITIONAL([AMDEP], [test "x$enable_dependency_tracking" != xno])
AC_SUBST([AMDEPBACKSLASH])
])

# Generate code to set up dependency tracking.   -*- Autoconf -*-

# Copyright 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#serial 2

# _AM_OUTPUT_DEPENDENCY_COMMANDS
# ------------------------------
AC_DEFUN([_AM_OUTPUT_DEPENDENCY_COMMANDS],
[for mf in $CONFIG_FILES; do
  # Strip MF so we end up with the name of the file.
  mf=`echo "$mf" | sed -e 's/:.*$//'`
  # Check whether this is an Automake generated Makefile or not.
  # We used to match only the files named `Makefile.in', but
  # some people rename them; so instead we look at the file content.
  # Grep'ing the first line is not enough: some people post-process
  # each Makefile.in and add a new line on top of each file to say so.
  # So let's grep whole file.
  if grep '^#.*generated by automake' $mf > /dev/null 2>&1; then
    dirpart=`AS_DIRNAME("$mf")`
  else
    continue
  fi
  grep '^DEP_FILES *= *[[^ @%:@]]' < "$mf" > /dev/null || continue
  # Extract the definition of DEP_FILES from the Makefile without
  # running `make'.
  DEPDIR=`sed -n -e '/^DEPDIR = / s///p' < "$mf"`
  test -z "$DEPDIR" && continue
  # When using ansi2knr, U may be empty or an underscore; expand it
  U=`sed -n -e '/^U = / s///p' < "$mf"`
  test -d "$dirpart/$DEPDIR" || mkdir "$dirpart/$DEPDIR"
  # We invoke sed twice because it is the simplest approach to
  # changing $(DEPDIR) to its actual value in the expansion.
  for file in `sed -n -e '
    /^DEP_FILES = .*\\\\$/ {
      s/^DEP_FILES = //
      :loop
	s/\\\\$//
	p
	n
	/\\\\$/ b loop
      p
    }
    /^DEP_FILES = / s/^DEP_FILES = //p' < "$mf" | \
       sed -e 's/\$(DEPDIR)/'"$DEPDIR"'/g' -e 's/\$U/'"$U"'/g'`; do
    # Make sure the directory exists.
    test -f "$dirpart/$file" && continue
    fdir=`AS_DIRNAME(["$file"])`
    AS_MKDIR_P([$dirpart/$fdir])
    # echo "creating $dirpart/$file"
    echo '# dummy' > "$dirpart/$file"
  done
done
])# _AM_OUTPUT_DEPENDENCY_COMMANDS


# AM_OUTPUT_DEPENDENCY_COMMANDS
# -----------------------------
# This macro should only be invoked once -- use via AC_REQUIRE.
#
# This code is only required when automatic dependency tracking
# is enabled.  FIXME.  This creates each `.P' file that we will
# need in order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],
[AC_CONFIG_COMMANDS([depfiles],
     [test x"$AMDEP_TRUE" != x"" || _AM_OUTPUT_DEPENDENCY_COMMANDS],
     [AMDEP_TRUE="$AMDEP_TRUE" ac_aux_dir="$ac_aux_dir"])
])

# Copyright 2001 Free Software Foundation, Inc.             -*- Autoconf -*-

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

# serial 2

# AM_MAKE_INCLUDE()
# -----------------
# Check to see how make treats includes.
AC_DEFUN([AM_MAKE_INCLUDE],
[am_make=${MAKE-make}
cat > confinc << 'END'
doit:
	@echo done
END
# If we don't find an include directive, just comment out the code.
AC_MSG_CHECKING([for style of include used by $am_make])
am__include="#"
am__quote=
_am_result=none
# First try GNU make style include.
echo "include confinc" > confmf
# We grep out `Entering directory' and `Leaving directory'
# messages which can occur if `w' ends up in MAKEFLAGS.
# In particular we don't look at `^make:' because GNU make might
# be invoked under some other name (usually "gmake"), in which
# case it prints its new name instead of `make'.
if test "`$am_make -s -f confmf 2> /dev/null | fgrep -v 'ing directory'`" = "done"; then
   am__include=include
   am__quote=
   _am_result=GNU
fi
# Now try BSD make style include.
if test "$am__include" = "#"; then
   echo '.include "confinc"' > confmf
   if test "`$am_make -s -f confmf 2> /dev/null`" = "done"; then
      am__include=.include
      am__quote="\""
      _am_result=BSD
   fi
fi
AC_SUBST(am__include)
AC_SUBST(am__quote)
AC_MSG_RESULT($_am_result)
rm -f confinc confmf
])

# AM_CONDITIONAL                                              -*- Autoconf -*-

# Copyright 1997, 2000, 2001 Free Software Foundation, Inc.

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

# serial 5

AC_PREREQ(2.52)

# AM_CONDITIONAL(NAME, SHELL-CONDITION)
# -------------------------------------
# Define a conditional.
AC_DEFUN([AM_CONDITIONAL],
[ifelse([$1], [TRUE],  [AC_FATAL([$0: invalid condition: $1])],
        [$1], [FALSE], [AC_FATAL([$0: invalid condition: $1])])dnl
AC_SUBST([$1_TRUE])
AC_SUBST([$1_FALSE])
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi
AC_CONFIG_COMMANDS_PRE(
[if test -z "${$1_TRUE}" && test -z "${$1_FALSE}"; then
  AC_MSG_ERROR([conditional \"$1\" was never defined.
Usually this means the macro was only invoked conditionally.])
fi])])

# isc-posix.m4 serial 1 (gettext-0.10.40)
dnl Copyright (C) 1995-2002 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

# libtool.m4 - Configure libtool for the host system. -*-Shell-script-*-

# serial 46 AC_PROG_LIBTOOL

AC_DEFUN([AC_PROG_LIBTOOL],
[AC_REQUIRE([AC_LIBTOOL_SETUP])dnl

# This can be used to rebuild libtool when needed
LIBTOOL_DEPS="$ac_aux_dir/ltmain.sh"

# Always use our own libtool.
LIBTOOL='$(SHELL) $(top_builddir)/libtool'
AC_SUBST(LIBTOOL)dnl

# Prevent multiple expansion
define([AC_PROG_LIBTOOL], [])
])

AC_DEFUN([AC_LIBTOOL_SETUP],
[AC_PREREQ(2.13)dnl
AC_REQUIRE([AC_ENABLE_SHARED])dnl
AC_REQUIRE([AC_ENABLE_STATIC])dnl
AC_REQUIRE([AC_ENABLE_FAST_INSTALL])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_CANONICAL_BUILD])dnl
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_LD])dnl
AC_REQUIRE([AC_PROG_LD_RELOAD_FLAG])dnl
AC_REQUIRE([AC_PROG_NM])dnl
AC_REQUIRE([AC_PROG_LN_S])dnl
AC_REQUIRE([AC_DEPLIBS_CHECK_METHOD])dnl
AC_REQUIRE([AC_OBJEXT])dnl
AC_REQUIRE([AC_EXEEXT])dnl
dnl

_LT_AC_PROG_ECHO_BACKSLASH
# Only perform the check for file, if the check method requires it
case $deplibs_check_method in
file_magic*)
  if test "$file_magic_cmd" = '$MAGIC_CMD'; then
    AC_PATH_MAGIC
  fi
  ;;
esac

AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_CHECK_TOOL(STRIP, strip, :)

ifdef([AC_PROVIDE_AC_LIBTOOL_DLOPEN], enable_dlopen=yes, enable_dlopen=no)
ifdef([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL],
enable_win32_dll=yes, enable_win32_dll=no)

AC_ARG_ENABLE(libtool-lock,
  [  --disable-libtool-lock  avoid locking (might break parallel builds)])
test "x$enable_libtool_lock" != xno && enable_libtool_lock=yes

# Some flags need to be propagated to the compiler or linker for good
# libtool support.
case $host in
*-*-irix6*)
  # Find out which ABI we are using.
  echo '[#]line __oline__ "configure"' > conftest.$ac_ext
  if AC_TRY_EVAL(ac_compile); then
    case `/usr/bin/file conftest.$ac_objext` in
    *32-bit*)
      LD="${LD-ld} -32"
      ;;
    *N32*)
      LD="${LD-ld} -n32"
      ;;
    *64-bit*)
      LD="${LD-ld} -64"
      ;;
    esac
  fi
  rm -rf conftest*
  ;;

*-*-sco3.2v5*)
  # On SCO OpenServer 5, we need -belf to get full-featured binaries.
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -belf"
  AC_CACHE_CHECK([whether the C compiler needs -belf], lt_cv_cc_needs_belf,
    [AC_LANG_SAVE
     AC_LANG_C
     AC_TRY_LINK([],[],[lt_cv_cc_needs_belf=yes],[lt_cv_cc_needs_belf=no])
     AC_LANG_RESTORE])
  if test x"$lt_cv_cc_needs_belf" != x"yes"; then
    # this is probably gcc 2.8.0, egcs 1.0 or newer; no need for -belf
    CFLAGS="$SAVE_CFLAGS"
  fi
  ;;

ifdef([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL],
[*-*-cygwin* | *-*-mingw* | *-*-pw32*)
  AC_CHECK_TOOL(DLLTOOL, dlltool, false)
  AC_CHECK_TOOL(AS, as, false)
  AC_CHECK_TOOL(OBJDUMP, objdump, false)

  # recent cygwin and mingw systems supply a stub DllMain which the user
  # can override, but on older systems we have to supply one
  AC_CACHE_CHECK([if libtool should supply DllMain function], lt_cv_need_dllmain,
    [AC_TRY_LINK([],
      [extern int __attribute__((__stdcall__)) DllMain(void*, int, void*);
      DllMain (0, 0, 0);],
      [lt_cv_need_dllmain=no],[lt_cv_need_dllmain=yes])])

  case $host/$CC in
  *-*-cygwin*/gcc*-mno-cygwin*|*-*-mingw*)
    # old mingw systems require "-dll" to link a DLL, while more recent ones
    # require "-mdll"
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -mdll"
    AC_CACHE_CHECK([how to link DLLs], lt_cv_cc_dll_switch,
      [AC_TRY_LINK([], [], [lt_cv_cc_dll_switch=-mdll],[lt_cv_cc_dll_switch=-dll])])
    CFLAGS="$SAVE_CFLAGS" ;;
  *-*-cygwin* | *-*-pw32*)
    # cygwin systems need to pass --dll to the linker, and not link
    # crt.o which will require a WinMain@16 definition.
    lt_cv_cc_dll_switch="-Wl,--dll -nostartfiles" ;;
  esac
  ;;
  ])
esac

_LT_AC_LTCONFIG_HACK

])

# AC_LIBTOOL_HEADER_ASSERT
# ------------------------
AC_DEFUN([AC_LIBTOOL_HEADER_ASSERT],
[AC_CACHE_CHECK([whether $CC supports assert without backlinking],
    [lt_cv_func_assert_works],
    [case $host in
    *-*-solaris*)
      if test "$GCC" = yes && test "$with_gnu_ld" != yes; then
        case `$CC --version 2>/dev/null` in
        [[12]].*) lt_cv_func_assert_works=no ;;
        *)        lt_cv_func_assert_works=yes ;;
        esac
      fi
      ;;
    esac])

if test "x$lt_cv_func_assert_works" = xyes; then
  AC_CHECK_HEADERS(assert.h)
fi
])# AC_LIBTOOL_HEADER_ASSERT

# _LT_AC_CHECK_DLFCN
# --------------------
AC_DEFUN([_LT_AC_CHECK_DLFCN],
[AC_CHECK_HEADERS(dlfcn.h)
])# _LT_AC_CHECK_DLFCN

# AC_LIBTOOL_SYS_GLOBAL_SYMBOL_PIPE
# ---------------------------------
AC_DEFUN([AC_LIBTOOL_SYS_GLOBAL_SYMBOL_PIPE],
[AC_REQUIRE([AC_CANONICAL_HOST])
AC_REQUIRE([AC_PROG_NM])
AC_REQUIRE([AC_OBJEXT])
# Check for command to grab the raw symbol name followed by C symbol from nm.
AC_MSG_CHECKING([command to parse $NM output])
AC_CACHE_VAL([lt_cv_sys_global_symbol_pipe], [dnl

# These are sane defaults that work on at least a few old systems.
# [They come from Ultrix.  What could be older than Ultrix?!! ;)]

# Character class describing NM global symbol codes.
symcode='[[BCDEGRST]]'

# Regexp to match symbols that can be accessed directly from C.
sympat='\([[_A-Za-z]][[_A-Za-z0-9]]*\)'

# Transform the above into a raw symbol and a C symbol.
symxfrm='\1 \2\3 \3'

# Transform an extracted symbol line into a proper C declaration
lt_cv_global_symbol_to_cdecl="sed -n -e 's/^. .* \(.*\)$/extern char \1;/p'"

# Transform an extracted symbol line into symbol name and symbol address
lt_cv_global_symbol_to_c_name_address="sed -n -e 's/^: \([[^ ]]*\) $/  {\\\"\1\\\", (lt_ptr) 0},/p' -e 's/^$symcode \([[^ ]]*\) \([[^ ]]*\)$/  {\"\2\", (lt_ptr) \&\2},/p'"

# Define system-specific variables.
case $host_os in
aix*)
  symcode='[[BCDT]]'
  ;;
cygwin* | mingw* | pw32*)
  symcode='[[ABCDGISTW]]'
  ;;
hpux*) # Its linker distinguishes data from code symbols
  lt_cv_global_symbol_to_cdecl="sed -n -e 's/^T .* \(.*\)$/extern char \1();/p' -e 's/^$symcode* .* \(.*\)$/extern char \1;/p'"
  lt_cv_global_symbol_to_c_name_address="sed -n -e 's/^: \([[^ ]]*\) $/  {\\\"\1\\\", (lt_ptr) 0},/p' -e 's/^$symcode* \([[^ ]]*\) \([[^ ]]*\)$/  {\"\2\", (lt_ptr) \&\2},/p'"
  ;;
irix*)
  symcode='[[BCDEGRST]]'
  ;;
solaris* | sysv5*)
  symcode='[[BDT]]'
  ;;
sysv4)
  symcode='[[DFNSTU]]'
  ;;
esac

# Handle CRLF in mingw tool chain
opt_cr=
case $host_os in
mingw*)
  opt_cr=`echo 'x\{0,1\}' | tr x '\015'` # option cr in regexp
  ;;
esac

# If we're using GNU nm, then use its standard symbol codes.
if $NM -V 2>&1 | egrep '(GNU|with BFD)' > /dev/null; then
  symcode='[[ABCDGISTW]]'
fi

# Try without a prefix undercore, then with it.
for ac_symprfx in "" "_"; do

  # Write the raw and C identifiers.
lt_cv_sys_global_symbol_pipe="sed -n -e 's/^.*[[ 	]]\($symcode$symcode*\)[[ 	]][[ 	]]*\($ac_symprfx\)$sympat$opt_cr$/$symxfrm/p'"

  # Check to see that the pipe works correctly.
  pipe_works=no
  rm -f conftest*
  cat > conftest.$ac_ext <<EOF
#ifdef __cplusplus
extern "C" {
#endif
char nm_test_var;
void nm_test_func(){}
#ifdef __cplusplus
}
#endif
int main(){nm_test_var='a';nm_test_func();return(0);}
EOF

  if AC_TRY_EVAL(ac_compile); then
    # Now try to grab the symbols.
    nlist=conftest.nm
    if AC_TRY_EVAL(NM conftest.$ac_objext \| $lt_cv_sys_global_symbol_pipe \> $nlist) && test -s "$nlist"; then
      # Try sorting and uniquifying the output.
      if sort "$nlist" | uniq > "$nlist"T; then
	mv -f "$nlist"T "$nlist"
      else
	rm -f "$nlist"T
      fi

      # Make sure that we snagged all the symbols we need.
      if egrep ' nm_test_var$' "$nlist" >/dev/null; then
	if egrep ' nm_test_func$' "$nlist" >/dev/null; then
	  cat <<EOF > conftest.$ac_ext
#ifdef __cplusplus
extern "C" {
#endif

EOF
	  # Now generate the symbol file.
	  eval "$lt_cv_global_symbol_to_cdecl"' < "$nlist" >> conftest.$ac_ext'

	  cat <<EOF >> conftest.$ac_ext
#if defined (__STDC__) && __STDC__
# define lt_ptr void *
#else
# define lt_ptr char *
# define const
#endif

/* The mapping between symbol names and symbols. */
const struct {
  const char *name;
  lt_ptr address;
}
lt_preloaded_symbols[[]] =
{
EOF
	  sed "s/^$symcode$symcode* \(.*\) \(.*\)$/  {\"\2\", (lt_ptr) \&\2},/" < "$nlist" >> conftest.$ac_ext
	  cat <<\EOF >> conftest.$ac_ext
  {0, (lt_ptr) 0}
};

#ifdef __cplusplus
}
#endif
EOF
	  # Now try linking the two files.
	  mv conftest.$ac_objext conftstm.$ac_objext
	  save_LIBS="$LIBS"
	  save_CFLAGS="$CFLAGS"
	  LIBS="conftstm.$ac_objext"
	  CFLAGS="$CFLAGS$no_builtin_flag"
	  if AC_TRY_EVAL(ac_link) && test -s conftest; then
	    pipe_works=yes
	  fi
	  LIBS="$save_LIBS"
	  CFLAGS="$save_CFLAGS"
	else
	  echo "cannot find nm_test_func in $nlist" >&AC_FD_CC
	fi
      else
	echo "cannot find nm_test_var in $nlist" >&AC_FD_CC
      fi
    else
      echo "cannot run $lt_cv_sys_global_symbol_pipe" >&AC_FD_CC
    fi
  else
    echo "$progname: failed program was:" >&AC_FD_CC
    cat conftest.$ac_ext >&5
  fi
  rm -f conftest* conftst*

  # Do not use the global_symbol_pipe unless it works.
  if test "$pipe_works" = yes; then
    break
  else
    lt_cv_sys_global_symbol_pipe=
  fi
done
])
global_symbol_pipe="$lt_cv_sys_global_symbol_pipe"
if test -z "$lt_cv_sys_global_symbol_pipe"; then
  global_symbol_to_cdecl=
  global_symbol_to_c_name_address=
else
  global_symbol_to_cdecl="$lt_cv_global_symbol_to_cdecl"
  global_symbol_to_c_name_address="$lt_cv_global_symbol_to_c_name_address"
fi
if test -z "$global_symbol_pipe$global_symbol_to_cdec$global_symbol_to_c_name_address";
then
  AC_MSG_RESULT(failed)
else
  AC_MSG_RESULT(ok)
fi
]) # AC_LIBTOOL_SYS_GLOBAL_SYMBOL_PIPE

# _LT_AC_LIBTOOL_SYS_PATH_SEPARATOR
# ---------------------------------
AC_DEFUN([_LT_AC_LIBTOOL_SYS_PATH_SEPARATOR],
[# Find the correct PATH separator.  Usually this is `:', but
# DJGPP uses `;' like DOS.
if test "X${PATH_SEPARATOR+set}" != Xset; then
  UNAME=${UNAME-`uname 2>/dev/null`}
  case X$UNAME in
    *-DOS) lt_cv_sys_path_separator=';' ;;
    *)     lt_cv_sys_path_separator=':' ;;
  esac
  PATH_SEPARATOR=$lt_cv_sys_path_separator
fi
])# _LT_AC_LIBTOOL_SYS_PATH_SEPARATOR

# _LT_AC_PROG_ECHO_BACKSLASH
# --------------------------
# Add some code to the start of the generated configure script which
# will find an echo command which doesn't interpret backslashes.
AC_DEFUN([_LT_AC_PROG_ECHO_BACKSLASH],
[ifdef([AC_DIVERSION_NOTICE], [AC_DIVERT_PUSH(AC_DIVERSION_NOTICE)],
			      [AC_DIVERT_PUSH(NOTICE)])
_LT_AC_LIBTOOL_SYS_PATH_SEPARATOR

# Check that we are running under the correct shell.
SHELL=${CONFIG_SHELL-/bin/sh}

case X$ECHO in
X*--fallback-echo)
  # Remove one level of quotation (which was required for Make).
  ECHO=`echo "$ECHO" | sed 's,\\\\\[$]\\[$]0,'[$]0','`
  ;;
esac

echo=${ECHO-echo}
if test "X[$]1" = X--no-reexec; then
  # Discard the --no-reexec flag, and continue.
  shift
elif test "X[$]1" = X--fallback-echo; then
  # Avoid inline document here, it may be left over
  :
elif test "X`($echo '\t') 2>/dev/null`" = 'X\t'; then
  # Yippee, $echo works!
  :
else
  # Restart under the correct shell.
  exec $SHELL "[$]0" --no-reexec ${1+"[$]@"}
fi

if test "X[$]1" = X--fallback-echo; then
  # used as fallback echo
  shift
  cat <<EOF
$*
EOF
  exit 0
fi

# The HP-UX ksh and POSIX shell print the target directory to stdout
# if CDPATH is set.
if test "X${CDPATH+set}" = Xset; then CDPATH=:; export CDPATH; fi

if test -z "$ECHO"; then
if test "X${echo_test_string+set}" != Xset; then
# find a string as large as possible, as long as the shell can cope with it
  for cmd in 'sed 50q "[$]0"' 'sed 20q "[$]0"' 'sed 10q "[$]0"' 'sed 2q "[$]0"' 'echo test'; do
    # expected sizes: less than 2Kb, 1Kb, 512 bytes, 16 bytes, ...
    if (echo_test_string="`eval $cmd`") 2>/dev/null &&
       echo_test_string="`eval $cmd`" &&
       (test "X$echo_test_string" = "X$echo_test_string") 2>/dev/null
    then
      break
    fi
  done
fi

if test "X`($echo '\t') 2>/dev/null`" = 'X\t' &&
   echo_testing_string=`($echo "$echo_test_string") 2>/dev/null` &&
   test "X$echo_testing_string" = "X$echo_test_string"; then
  :
else
  # The Solaris, AIX, and Digital Unix default echo programs unquote
  # backslashes.  This makes it impossible to quote backslashes using
  #   echo "$something" | sed 's/\\/\\\\/g'
  #
  # So, first we look for a working echo in the user's PATH.

  IFS="${IFS= 	}"; save_ifs="$IFS"; IFS=$PATH_SEPARATOR
  for dir in $PATH /usr/ucb; do
    if (test -f $dir/echo || test -f $dir/echo$ac_exeext) &&
       test "X`($dir/echo '\t') 2>/dev/null`" = 'X\t' &&
       echo_testing_string=`($dir/echo "$echo_test_string") 2>/dev/null` &&
       test "X$echo_testing_string" = "X$echo_test_string"; then
      echo="$dir/echo"
      break
    fi
  done
  IFS="$save_ifs"

  if test "X$echo" = Xecho; then
    # We didn't find a better echo, so look for alternatives.
    if test "X`(print -r '\t') 2>/dev/null`" = 'X\t' &&
       echo_testing_string=`(print -r "$echo_test_string") 2>/dev/null` &&
       test "X$echo_testing_string" = "X$echo_test_string"; then
      # This shell has a builtin print -r that does the trick.
      echo='print -r'
    elif (test -f /bin/ksh || test -f /bin/ksh$ac_exeext) &&
	 test "X$CONFIG_SHELL" != X/bin/ksh; then
      # If we have ksh, try running configure again with it.
      ORIGINAL_CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
      export ORIGINAL_CONFIG_SHELL
      CONFIG_SHELL=/bin/ksh
      export CONFIG_SHELL
      exec $CONFIG_SHELL "[$]0" --no-reexec ${1+"[$]@"}
    else
      # Try using printf.
      echo='printf %s\n'
      if test "X`($echo '\t') 2>/dev/null`" = 'X\t' &&
	 echo_testing_string=`($echo "$echo_test_string") 2>/dev/null` &&
	 test "X$echo_testing_string" = "X$echo_test_string"; then
	# Cool, printf works
	:
      elif echo_testing_string=`($ORIGINAL_CONFIG_SHELL "[$]0" --fallback-echo '\t') 2>/dev/null` &&
	   test "X$echo_testing_string" = 'X\t' &&
	   echo_testing_string=`($ORIGINAL_CONFIG_SHELL "[$]0" --fallback-echo "$echo_test_string") 2>/dev/null` &&
	   test "X$echo_testing_string" = "X$echo_test_string"; then
	CONFIG_SHELL=$ORIGINAL_CONFIG_SHELL
	export CONFIG_SHELL
	SHELL="$CONFIG_SHELL"
	export SHELL
	echo="$CONFIG_SHELL [$]0 --fallback-echo"
      elif echo_testing_string=`($CONFIG_SHELL "[$]0" --fallback-echo '\t') 2>/dev/null` &&
	   test "X$echo_testing_string" = 'X\t' &&
	   echo_testing_string=`($CONFIG_SHELL "[$]0" --fallback-echo "$echo_test_string") 2>/dev/null` &&
	   test "X$echo_testing_string" = "X$echo_test_string"; then
	echo="$CONFIG_SHELL [$]0 --fallback-echo"
      else
	# maybe with a smaller string...
	prev=:

	for cmd in 'echo test' 'sed 2q "[$]0"' 'sed 10q "[$]0"' 'sed 20q "[$]0"' 'sed 50q "[$]0"'; do
	  if (test "X$echo_test_string" = "X`eval $cmd`") 2>/dev/null
	  then
	    break
	  fi
	  prev="$cmd"
	done

	if test "$prev" != 'sed 50q "[$]0"'; then
	  echo_test_string=`eval $prev`
	  export echo_test_string
	  exec ${ORIGINAL_CONFIG_SHELL-${CONFIG_SHELL-/bin/sh}} "[$]0" ${1+"[$]@"}
	else
	  # Oops.  We lost completely, so just stick with echo.
	  echo=echo
	fi
      fi
    fi
  fi
fi
fi

# Copy echo and quote the copy suitably for passing to libtool from
# the Makefile, instead of quoting the original, which is used later.
ECHO=$echo
if test "X$ECHO" = "X$CONFIG_SHELL [$]0 --fallback-echo"; then
   ECHO="$CONFIG_SHELL \\\$\[$]0 --fallback-echo"
fi

AC_SUBST(ECHO)
AC_DIVERT_POP
])# _LT_AC_PROG_ECHO_BACKSLASH

# _LT_AC_TRY_DLOPEN_SELF (ACTION-IF-TRUE, ACTION-IF-TRUE-W-USCORE,
#                           ACTION-IF-FALSE, ACTION-IF-CROSS-COMPILING)
# ------------------------------------------------------------------
AC_DEFUN([_LT_AC_TRY_DLOPEN_SELF],
[if test "$cross_compiling" = yes; then :
  [$4]
else
  AC_REQUIRE([_LT_AC_CHECK_DLFCN])dnl
  lt_dlunknown=0; lt_dlno_uscore=1; lt_dlneed_uscore=2
  lt_status=$lt_dlunknown
  cat > conftest.$ac_ext <<EOF
[#line __oline__ "configure"
#include "confdefs.h"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <stdio.h>

#ifdef RTLD_GLOBAL
#  define LT_DLGLOBAL		RTLD_GLOBAL
#else
#  ifdef DL_GLOBAL
#    define LT_DLGLOBAL		DL_GLOBAL
#  else
#    define LT_DLGLOBAL		0
#  endif
#endif

/* We may have to define LT_DLLAZY_OR_NOW in the command line if we
   find out it does not work in some platform. */
#ifndef LT_DLLAZY_OR_NOW
#  ifdef RTLD_LAZY
#    define LT_DLLAZY_OR_NOW		RTLD_LAZY
#  else
#    ifdef DL_LAZY
#      define LT_DLLAZY_OR_NOW		DL_LAZY
#    else
#      ifdef RTLD_NOW
#        define LT_DLLAZY_OR_NOW	RTLD_NOW
#      else
#        ifdef DL_NOW
#          define LT_DLLAZY_OR_NOW	DL_NOW
#        else
#          define LT_DLLAZY_OR_NOW	0
#        endif
#      endif
#    endif
#  endif
#endif

#ifdef __cplusplus
extern "C" void exit (int);
#endif

void fnord() { int i=42;}
int main ()
{
  void *self = dlopen (0, LT_DLGLOBAL|LT_DLLAZY_OR_NOW);
  int status = $lt_dlunknown;

  if (self)
    {
      if (dlsym (self,"fnord"))       status = $lt_dlno_uscore;
      else if (dlsym( self,"_fnord")) status = $lt_dlneed_uscore;
      /* dlclose (self); */
    }

    exit (status);
}]
EOF
  if AC_TRY_EVAL(ac_link) && test -s conftest${ac_exeext} 2>/dev/null; then
    (./conftest; exit; ) 2>/dev/null
    lt_status=$?
    case x$lt_status in
      x$lt_dlno_uscore) $1 ;;
      x$lt_dlneed_uscore) $2 ;;
      x$lt_unknown|x*) $3 ;;
    esac
  else :
    # compilation failed
    $3
  fi
fi
rm -fr conftest*
])# _LT_AC_TRY_DLOPEN_SELF

# AC_LIBTOOL_DLOPEN_SELF
# -------------------
AC_DEFUN([AC_LIBTOOL_DLOPEN_SELF],
[if test "x$enable_dlopen" != xyes; then
  enable_dlopen=unknown
  enable_dlopen_self=unknown
  enable_dlopen_self_static=unknown
else
  lt_cv_dlopen=no
  lt_cv_dlopen_libs=

  case $host_os in
  beos*)
    lt_cv_dlopen="load_add_on"
    lt_cv_dlopen_libs=
    lt_cv_dlopen_self=yes
    ;;

  cygwin* | mingw* | pw32*)
    lt_cv_dlopen="LoadLibrary"
    lt_cv_dlopen_libs=
   ;;

  *)
    AC_CHECK_FUNC([shl_load],
          [lt_cv_dlopen="shl_load"],
      [AC_CHECK_LIB([dld], [shl_load],
            [lt_cv_dlopen="shl_load" lt_cv_dlopen_libs="-dld"],
	[AC_CHECK_FUNC([dlopen],
	      [lt_cv_dlopen="dlopen"],
	  [AC_CHECK_LIB([dl], [dlopen],
	        [lt_cv_dlopen="dlopen" lt_cv_dlopen_libs="-ldl"],
	    [AC_CHECK_LIB([svld], [dlopen],
	          [lt_cv_dlopen="dlopen" lt_cv_dlopen_libs="-lsvld"],
	      [AC_CHECK_LIB([dld], [dld_link],
	            [lt_cv_dlopen="dld_link" lt_cv_dlopen_libs="-dld"])
	      ])
	    ])
	  ])
	])
      ])
    ;;
  esac

  if test "x$lt_cv_dlopen" != xno; then
    enable_dlopen=yes
  else
    enable_dlopen=no
  fi

  case $lt_cv_dlopen in
  dlopen)
    save_CPPFLAGS="$CPPFLAGS"
    AC_REQUIRE([_LT_AC_CHECK_DLFCN])dnl
    test "x$ac_cv_header_dlfcn_h" = xyes && CPPFLAGS="$CPPFLAGS -DHAVE_DLFCN_H"

    save_LDFLAGS="$LDFLAGS"
    eval LDFLAGS=\"\$LDFLAGS $export_dynamic_flag_spec\"

    save_LIBS="$LIBS"
    LIBS="$lt_cv_dlopen_libs $LIBS"

    AC_CACHE_CHECK([whether a program can dlopen itself],
	  lt_cv_dlopen_self, [dnl
	  _LT_AC_TRY_DLOPEN_SELF(
	    lt_cv_dlopen_self=yes, lt_cv_dlopen_self=yes,
	    lt_cv_dlopen_self=no, lt_cv_dlopen_self=cross)
    ])

    if test "x$lt_cv_dlopen_self" = xyes; then
      LDFLAGS="$LDFLAGS $link_static_flag"
      AC_CACHE_CHECK([whether a statically linked program can dlopen itself],
    	  lt_cv_dlopen_self_static, [dnl
	  _LT_AC_TRY_DLOPEN_SELF(
	    lt_cv_dlopen_self_static=yes, lt_cv_dlopen_self_static=yes,
	    lt_cv_dlopen_self_static=no,  lt_cv_dlopen_self_static=cross)
      ])
    fi

    CPPFLAGS="$save_CPPFLAGS"
    LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"
    ;;
  esac

  case $lt_cv_dlopen_self in
  yes|no) enable_dlopen_self=$lt_cv_dlopen_self ;;
  *) enable_dlopen_self=unknown ;;
  esac

  case $lt_cv_dlopen_self_static in
  yes|no) enable_dlopen_self_static=$lt_cv_dlopen_self_static ;;
  *) enable_dlopen_self_static=unknown ;;
  esac
fi
])# AC_LIBTOOL_DLOPEN_SELF

AC_DEFUN([_LT_AC_LTCONFIG_HACK],
[AC_REQUIRE([AC_LIBTOOL_SYS_GLOBAL_SYMBOL_PIPE])dnl
# Sed substitution that helps us do robust quoting.  It backslashifies
# metacharacters that are still active within double-quoted strings.
Xsed='sed -e s/^X//'
sed_quote_subst='s/\([[\\"\\`$\\\\]]\)/\\\1/g'

# Same as above, but do not quote variable references.
double_quote_subst='s/\([[\\"\\`\\\\]]\)/\\\1/g'

# Sed substitution to delay expansion of an escaped shell variable in a
# double_quote_subst'ed string.
delay_variable_subst='s/\\\\\\\\\\\$/\\\\\\$/g'

# Constants:
rm="rm -f"

# Global variables:
default_ofile=libtool
can_build_shared=yes

# All known linkers require a `.a' archive for static linking (except M$VC,
# which needs '.lib').
libext=a
ltmain="$ac_aux_dir/ltmain.sh"
ofile="$default_ofile"
with_gnu_ld="$lt_cv_prog_gnu_ld"
need_locks="$enable_libtool_lock"

old_CC="$CC"
old_CFLAGS="$CFLAGS"

# Set sane defaults for various variables
test -z "$AR" && AR=ar
test -z "$AR_FLAGS" && AR_FLAGS=cru
test -z "$AS" && AS=as
test -z "$CC" && CC=cc
test -z "$DLLTOOL" && DLLTOOL=dlltool
test -z "$LD" && LD=ld
test -z "$LN_S" && LN_S="ln -s"
test -z "$MAGIC_CMD" && MAGIC_CMD=file
test -z "$NM" && NM=nm
test -z "$OBJDUMP" && OBJDUMP=objdump
test -z "$RANLIB" && RANLIB=:
test -z "$STRIP" && STRIP=:
test -z "$ac_objext" && ac_objext=o

if test x"$host" != x"$build"; then
  ac_tool_prefix=${host_alias}-
else
  ac_tool_prefix=
fi

# Transform linux* to *-*-linux-gnu*, to support old configure scripts.
case $host_os in
linux-gnu*) ;;
linux*) host=`echo $host | sed 's/^\(.*-.*-linux\)\(.*\)$/\1-gnu\2/'`
esac

case $host_os in
aix3*)
  # AIX sometimes has problems with the GCC collect2 program.  For some
  # reason, if we set the COLLECT_NAMES environment variable, the problems
  # vanish in a puff of smoke.
  if test "X${COLLECT_NAMES+set}" != Xset; then
    COLLECT_NAMES=
    export COLLECT_NAMES
  fi
  ;;
esac

# Determine commands to create old-style static archives.
old_archive_cmds='$AR $AR_FLAGS $oldlib$oldobjs$old_deplibs'
old_postinstall_cmds='chmod 644 $oldlib'
old_postuninstall_cmds=

if test -n "$RANLIB"; then
  case $host_os in
  openbsd*)
    old_postinstall_cmds="\$RANLIB -t \$oldlib~$old_postinstall_cmds"
    ;;
  *)
    old_postinstall_cmds="\$RANLIB \$oldlib~$old_postinstall_cmds"
    ;;
  esac
  old_archive_cmds="$old_archive_cmds~\$RANLIB \$oldlib"
fi

# Allow CC to be a program name with arguments.
set dummy $CC
compiler="[$]2"

AC_MSG_CHECKING([for objdir])
rm -f .libs 2>/dev/null
mkdir .libs 2>/dev/null
if test -d .libs; then
  objdir=.libs
else
  # MS-DOS does not allow filenames that begin with a dot.
  objdir=_libs
fi
rmdir .libs 2>/dev/null
AC_MSG_RESULT($objdir)


AC_ARG_WITH(pic,
[  --with-pic              try to use only PIC/non-PIC objects [default=use both]],
pic_mode="$withval", pic_mode=default)
test -z "$pic_mode" && pic_mode=default

# We assume here that the value for lt_cv_prog_cc_pic will not be cached
# in isolation, and that seeing it set (from the cache) indicates that
# the associated values are set (in the cache) correctly too.
AC_MSG_CHECKING([for $compiler option to produce PIC])
AC_CACHE_VAL(lt_cv_prog_cc_pic,
[ lt_cv_prog_cc_pic=
  lt_cv_prog_cc_shlib=
  lt_cv_prog_cc_wl=
  lt_cv_prog_cc_static=
  lt_cv_prog_cc_no_builtin=
  lt_cv_prog_cc_can_build_shared=$can_build_shared

  if test "$GCC" = yes; then
    lt_cv_prog_cc_wl='-Wl,'
    lt_cv_prog_cc_static='-static'

    case $host_os in
    aix*)
      # Below there is a dirty hack to force normal static linking with -ldl
      # The problem is because libdl dynamically linked with both libc and
      # libC (AIX C++ library), which obviously doesn't included in libraries
      # list by gcc. This cause undefined symbols with -static flags.
      # This hack allows C programs to be linked with "-static -ldl", but
      # not sure about C++ programs.
      lt_cv_prog_cc_static="$lt_cv_prog_cc_static ${lt_cv_prog_cc_wl}-lC"
      ;;
    amigaos*)
      # FIXME: we need at least 68020 code to build shared libraries, but
      # adding the `-m68020' flag to GCC prevents building anything better,
      # like `-m68040'.
      lt_cv_prog_cc_pic='-m68020 -resident32 -malways-restore-a4'
      ;;
    beos* | irix5* | irix6* | osf3* | osf4* | osf5*)
      # PIC is the default for these OSes.
      ;;
    darwin* | rhapsody*)
      # PIC is the default on this platform
      # Common symbols not allowed in MH_DYLIB files
      lt_cv_prog_cc_pic='-fno-common'
      ;;
    cygwin* | mingw* | pw32* | os2*)
      # This hack is so that the source file can tell whether it is being
      # built for inclusion in a dll (and should export symbols for example).
      lt_cv_prog_cc_pic='-DDLL_EXPORT'
      ;;
    sysv4*MP*)
      if test -d /usr/nec; then
	 lt_cv_prog_cc_pic=-Kconform_pic
      fi
      ;;
    *)
      lt_cv_prog_cc_pic='-fPIC'
      ;;
    esac
  else
    # PORTME Check for PIC flags for the system compiler.
    case $host_os in
    aix3* | aix4* | aix5*)
      lt_cv_prog_cc_wl='-Wl,'
      # All AIX code is PIC.
      if test "$host_cpu" = ia64; then
	# AIX 5 now supports IA64 processor
	lt_cv_prog_cc_static='-Bstatic'
      else
	lt_cv_prog_cc_static='-bnso -bI:/lib/syscalls.exp'
      fi
      ;;

    hpux9* | hpux10* | hpux11*)
      # Is there a better lt_cv_prog_cc_static that works with the bundled CC?
      lt_cv_prog_cc_wl='-Wl,'
      lt_cv_prog_cc_static="${lt_cv_prog_cc_wl}-a ${lt_cv_prog_cc_wl}archive"
      lt_cv_prog_cc_pic='+Z'
      ;;

    irix5* | irix6*)
      lt_cv_prog_cc_wl='-Wl,'
      lt_cv_prog_cc_static='-non_shared'
      # PIC (with -KPIC) is the default.
      ;;

    cygwin* | mingw* | pw32* | os2*)
      # This hack is so that the source file can tell whether it is being
      # built for inclusion in a dll (and should export symbols for example).
      lt_cv_prog_cc_pic='-DDLL_EXPORT'
      ;;

    newsos6)
      lt_cv_prog_cc_pic='-KPIC'
      lt_cv_prog_cc_static='-Bstatic'
      ;;

    osf3* | osf4* | osf5*)
      # All OSF/1 code is PIC.
      lt_cv_prog_cc_wl='-Wl,'
      lt_cv_prog_cc_static='-non_shared'
      ;;

    sco3.2v5*)
      lt_cv_prog_cc_pic='-Kpic'
      lt_cv_prog_cc_static='-dn'
      lt_cv_prog_cc_shlib='-belf'
      ;;

    solaris*)
      lt_cv_prog_cc_pic='-KPIC'
      lt_cv_prog_cc_static='-Bstatic'
      lt_cv_prog_cc_wl='-Wl,'
      ;;

    sunos4*)
      lt_cv_prog_cc_pic='-PIC'
      lt_cv_prog_cc_static='-Bstatic'
      lt_cv_prog_cc_wl='-Qoption ld '
      ;;

    sysv4 | sysv4.2uw2* | sysv4.3* | sysv5*)
      lt_cv_prog_cc_pic='-KPIC'
      lt_cv_prog_cc_static='-Bstatic'
      if test "x$host_vendor" = xsni; then
	lt_cv_prog_cc_wl='-LD'
      else
	lt_cv_prog_cc_wl='-Wl,'
      fi
      ;;

    uts4*)
      lt_cv_prog_cc_pic='-pic'
      lt_cv_prog_cc_static='-Bstatic'
      ;;

    sysv4*MP*)
      if test -d /usr/nec ;then
	lt_cv_prog_cc_pic='-Kconform_pic'
	lt_cv_prog_cc_static='-Bstatic'
      fi
      ;;

    *)
      lt_cv_prog_cc_can_build_shared=no
      ;;
    esac
  fi
])
if test -z "$lt_cv_prog_cc_pic"; then
  AC_MSG_RESULT([none])
else
  AC_MSG_RESULT([$lt_cv_prog_cc_pic])

  # Check to make sure the pic_flag actually works.
  AC_MSG_CHECKING([if $compiler PIC flag $lt_cv_prog_cc_pic works])
  AC_CACHE_VAL(lt_cv_prog_cc_pic_works, [dnl
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $lt_cv_prog_cc_pic -DPIC"
    AC_TRY_COMPILE([], [], [dnl
      case $host_os in
      hpux9* | hpux10* | hpux11*)
	# On HP-UX, both CC and GCC only warn that PIC is supported... then
	# they create non-PIC objects.  So, if there were any warnings, we
	# assume that PIC is not supported.
	if test -s conftest.err; then
	  lt_cv_prog_cc_pic_works=no
	else
	  lt_cv_prog_cc_pic_works=yes
	fi
	;;
      *)
	lt_cv_prog_cc_pic_works=yes
	;;
      esac
    ], [dnl
      lt_cv_prog_cc_pic_works=no
    ])
    CFLAGS="$save_CFLAGS"
  ])

  if test "X$lt_cv_prog_cc_pic_works" = Xno; then
    lt_cv_prog_cc_pic=
    lt_cv_prog_cc_can_build_shared=no
  else
    lt_cv_prog_cc_pic=" $lt_cv_prog_cc_pic"
  fi

  AC_MSG_RESULT([$lt_cv_prog_cc_pic_works])
fi

# Check for any special shared library compilation flags.
if test -n "$lt_cv_prog_cc_shlib"; then
  AC_MSG_WARN([\`$CC' requires \`$lt_cv_prog_cc_shlib' to build shared libraries])
  if echo "$old_CC $old_CFLAGS " | egrep -e "[[ 	]]$lt_cv_prog_cc_shlib[[ 	]]" >/dev/null; then :
  else
   AC_MSG_WARN([add \`$lt_cv_prog_cc_shlib' to the CC or CFLAGS env variable and reconfigure])
    lt_cv_prog_cc_can_build_shared=no
  fi
fi

AC_MSG_CHECKING([if $compiler static flag $lt_cv_prog_cc_static works])
AC_CACHE_VAL([lt_cv_prog_cc_static_works], [dnl
  lt_cv_prog_cc_static_works=no
  save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $lt_cv_prog_cc_static"
  AC_TRY_LINK([], [], [lt_cv_prog_cc_static_works=yes])
  LDFLAGS="$save_LDFLAGS"
])

# Belt *and* braces to stop my trousers falling down:
test "X$lt_cv_prog_cc_static_works" = Xno && lt_cv_prog_cc_static=
AC_MSG_RESULT([$lt_cv_prog_cc_static_works])

pic_flag="$lt_cv_prog_cc_pic"
special_shlib_compile_flags="$lt_cv_prog_cc_shlib"
wl="$lt_cv_prog_cc_wl"
link_static_flag="$lt_cv_prog_cc_static"
no_builtin_flag="$lt_cv_prog_cc_no_builtin"
can_build_shared="$lt_cv_prog_cc_can_build_shared"


# Check to see if options -o and -c are simultaneously supported by compiler
AC_MSG_CHECKING([if $compiler supports -c -o file.$ac_objext])
AC_CACHE_VAL([lt_cv_compiler_c_o], [
$rm -r conftest 2>/dev/null
mkdir conftest
cd conftest
echo "int some_variable = 0;" > conftest.$ac_ext
mkdir out
# According to Tom Tromey, Ian Lance Taylor reported there are C compilers
# that will create temporary files in the current directory regardless of
# the output directory.  Thus, making CWD read-only will cause this test
# to fail, enabling locking or at least warning the user not to do parallel
# builds.
chmod -w .
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -o out/conftest2.$ac_objext"
compiler_c_o=no
if { (eval echo configure:__oline__: \"$ac_compile\") 1>&5; (eval $ac_compile) 2>out/conftest.err; } && test -s out/conftest2.$ac_objext; then
  # The compiler can only warn and ignore the option if not recognized
  # So say no if there are warnings
  if test -s out/conftest.err; then
    lt_cv_compiler_c_o=no
  else
    lt_cv_compiler_c_o=yes
  fi
else
  # Append any errors to the config.log.
  cat out/conftest.err 1>&AC_FD_CC
  lt_cv_compiler_c_o=no
fi
CFLAGS="$save_CFLAGS"
chmod u+w .
$rm conftest* out/*
rmdir out
cd ..
rmdir conftest
$rm -r conftest 2>/dev/null
])
compiler_c_o=$lt_cv_compiler_c_o
AC_MSG_RESULT([$compiler_c_o])

if test x"$compiler_c_o" = x"yes"; then
  # Check to see if we can write to a .lo
  AC_MSG_CHECKING([if $compiler supports -c -o file.lo])
  AC_CACHE_VAL([lt_cv_compiler_o_lo], [
  lt_cv_compiler_o_lo=no
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -c -o conftest.lo"
  save_objext="$ac_objext"
  ac_objext=lo
  AC_TRY_COMPILE([], [int some_variable = 0;], [dnl
    # The compiler can only warn and ignore the option if not recognized
    # So say no if there are warnings
    if test -s conftest.err; then
      lt_cv_compiler_o_lo=no
    else
      lt_cv_compiler_o_lo=yes
    fi
  ])
  ac_objext="$save_objext"
  CFLAGS="$save_CFLAGS"
  ])
  compiler_o_lo=$lt_cv_compiler_o_lo
  AC_MSG_RESULT([$compiler_o_lo])
else
  compiler_o_lo=no
fi

# Check to see if we can do hard links to lock some files if needed
hard_links="nottested"
if test "$compiler_c_o" = no && test "$need_locks" != no; then
  # do not overwrite the value of need_locks provided by the user
  AC_MSG_CHECKING([if we can lock with hard links])
  hard_links=yes
  $rm conftest*
  ln conftest.a conftest.b 2>/dev/null && hard_links=no
  touch conftest.a
  ln conftest.a conftest.b 2>&5 || hard_links=no
  ln conftest.a conftest.b 2>/dev/null && hard_links=no
  AC_MSG_RESULT([$hard_links])
  if test "$hard_links" = no; then
    AC_MSG_WARN([\`$CC' does not support \`-c -o', so \`make -j' may be unsafe])
    need_locks=warn
  fi
else
  need_locks=no
fi

if test "$GCC" = yes; then
  # Check to see if options -fno-rtti -fno-exceptions are supported by compiler
  AC_MSG_CHECKING([if $compiler supports -fno-rtti -fno-exceptions])
  echo "int some_variable = 0;" > conftest.$ac_ext
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -fno-rtti -fno-exceptions -c conftest.$ac_ext"
  compiler_rtti_exceptions=no
  AC_TRY_COMPILE([], [int some_variable = 0;], [dnl
    # The compiler can only warn and ignore the option if not recognized
    # So say no if there are warnings
    if test -s conftest.err; then
      compiler_rtti_exceptions=no
    else
      compiler_rtti_exceptions=yes
    fi
  ])
  CFLAGS="$save_CFLAGS"
  AC_MSG_RESULT([$compiler_rtti_exceptions])

  if test "$compiler_rtti_exceptions" = "yes"; then
    no_builtin_flag=' -fno-builtin -fno-rtti -fno-exceptions'
  else
    no_builtin_flag=' -fno-builtin'
  fi
fi

# See if the linker supports building shared libraries.
AC_MSG_CHECKING([whether the linker ($LD) supports shared libraries])

allow_undefined_flag=
no_undefined_flag=
need_lib_prefix=unknown
need_version=unknown
# when you set need_version to no, make sure it does not cause -set_version
# flags to be left without arguments
archive_cmds=
archive_expsym_cmds=
old_archive_from_new_cmds=
old_archive_from_expsyms_cmds=
export_dynamic_flag_spec=
whole_archive_flag_spec=
thread_safe_flag_spec=
hardcode_into_libs=no
hardcode_libdir_flag_spec=
hardcode_libdir_separator=
hardcode_direct=no
hardcode_minus_L=no
hardcode_shlibpath_var=unsupported
runpath_var=
link_all_deplibs=unknown
always_export_symbols=no
export_symbols_cmds='$NM $libobjs $convenience | $global_symbol_pipe | sed '\''s/.* //'\'' | sort | uniq > $export_symbols'
# include_expsyms should be a list of space-separated symbols to be *always*
# included in the symbol list
include_expsyms=
# exclude_expsyms can be an egrep regular expression of symbols to exclude
# it will be wrapped by ` (' and `)$', so one must not match beginning or
# end of line.  Example: `a|bc|.*d.*' will exclude the symbols `a' and `bc',
# as well as any symbol that contains `d'.
exclude_expsyms="_GLOBAL_OFFSET_TABLE_"
# Although _GLOBAL_OFFSET_TABLE_ is a valid symbol C name, most a.out
# platforms (ab)use it in PIC code, but their linkers get confused if
# the symbol is explicitly referenced.  Since portable code cannot
# rely on this symbol name, it's probably fine to never include it in
# preloaded symbol tables.
extract_expsyms_cmds=

case $host_os in
cygwin* | mingw* | pw32*)
  # FIXME: the MSVC++ port hasn't been tested in a loooong time
  # When not using gcc, we currently assume that we are using
  # Microsoft Visual C++.
  if test "$GCC" != yes; then
    with_gnu_ld=no
  fi
  ;;
openbsd*)
  with_gnu_ld=no
  ;;
esac

ld_shlibs=yes
if test "$with_gnu_ld" = yes; then
  # If archive_cmds runs LD, not CC, wlarc should be empty
  wlarc='${wl}'

  # See if GNU ld supports shared libraries.
  case $host_os in
  aix3* | aix4* | aix5*)
    # On AIX, the GNU linker is very broken
    # Note:Check GNU linker on AIX 5-IA64 when/if it becomes available.
    ld_shlibs=no
    cat <<EOF 1>&2

*** Warning: the GNU linker, at least up to release 2.9.1, is reported
*** to be unable to reliably create shared libraries on AIX.
*** Therefore, libtool is disabling shared libraries support.  If you
*** really care for shared libraries, you may want to modify your PATH
*** so that a non-GNU linker is found, and then restart.

EOF
    ;;

  amigaos*)
    archive_cmds='$rm $output_objdir/a2ixlibrary.data~$echo "#define NAME $libname" > $output_objdir/a2ixlibrary.data~$echo "#define LIBRARY_ID 1" >> $output_objdir/a2ixlibrary.data~$echo "#define VERSION $major" >> $output_objdir/a2ixlibrary.data~$echo "#define REVISION $revision" >> $output_objdir/a2ixlibrary.data~$AR $AR_FLAGS $lib $libobjs~$RANLIB $lib~(cd $output_objdir && a2ixlibrary -32)'
    hardcode_libdir_flag_spec='-L$libdir'
    hardcode_minus_L=yes

    # Samuel A. Falvo II <kc5tja@dolphin.openprojects.net> reports
    # that the semantics of dynamic libraries on AmigaOS, at least up
    # to version 4, is to share data among multiple programs linked
    # with the same dynamic library.  Since this doesn't match the
    # behavior of shared libraries on other platforms, we can use
    # them.
    ld_shlibs=no
    ;;

  beos*)
    if $LD --help 2>&1 | egrep ': supported targets:.* elf' > /dev/null; then
      allow_undefined_flag=unsupported
      # Joseph Beckenbach <jrb3@best.com> says some releases of gcc
      # support --undefined.  This deserves some investigation.  FIXME
      archive_cmds='$CC -nostart $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname -o $lib'
    else
      ld_shlibs=no
    fi
    ;;

  cygwin* | mingw* | pw32*)
    # hardcode_libdir_flag_spec is actually meaningless, as there is
    # no search path for DLLs.
    hardcode_libdir_flag_spec='-L$libdir'
    allow_undefined_flag=unsupported
    always_export_symbols=yes

    extract_expsyms_cmds='test -f $output_objdir/impgen.c || \
      sed -e "/^# \/\* impgen\.c starts here \*\//,/^# \/\* impgen.c ends here \*\// { s/^# //;s/^# *$//; p; }" -e d < $''0 > $output_objdir/impgen.c~
      test -f $output_objdir/impgen.exe || (cd $output_objdir && \
      if test "x$HOST_CC" != "x" ; then $HOST_CC -o impgen impgen.c ; \
      else $CC -o impgen impgen.c ; fi)~
      $output_objdir/impgen $dir/$soroot > $output_objdir/$soname-def'

    old_archive_from_expsyms_cmds='$DLLTOOL --as=$AS --dllname $soname --def $output_objdir/$soname-def --output-lib $output_objdir/$newlib'

    # cygwin and mingw dlls have different entry points and sets of symbols
    # to exclude.
    # FIXME: what about values for MSVC?
    dll_entry=__cygwin_dll_entry@12
    dll_exclude_symbols=DllMain@12,_cygwin_dll_entry@12,_cygwin_noncygwin_dll_entry@12~
    case $host_os in
    mingw*)
      # mingw values
      dll_entry=_DllMainCRTStartup@12
      dll_exclude_symbols=DllMain@12,DllMainCRTStartup@12,DllEntryPoint@12~
      ;;
    esac

    # mingw and cygwin differ, and it's simplest to just exclude the union
    # of the two symbol sets.
    dll_exclude_symbols=DllMain@12,_cygwin_dll_entry@12,_cygwin_noncygwin_dll_entry@12,DllMainCRTStartup@12,DllEntryPoint@12

    # recent cygwin and mingw systems supply a stub DllMain which the user
    # can override, but on older systems we have to supply one (in ltdll.c)
    if test "x$lt_cv_need_dllmain" = "xyes"; then
      ltdll_obj='$output_objdir/$soname-ltdll.'"$ac_objext "
      ltdll_cmds='test -f $output_objdir/$soname-ltdll.c || sed -e "/^# \/\* ltdll\.c starts here \*\//,/^# \/\* ltdll.c ends here \*\// { s/^# //; p; }" -e d < $''0 > $output_objdir/$soname-ltdll.c~
	test -f $output_objdir/$soname-ltdll.$ac_objext || (cd $output_objdir && $CC -c $soname-ltdll.c)~'
    else
      ltdll_obj=
      ltdll_cmds=
    fi

    # Extract the symbol export list from an `--export-all' def file,
    # then regenerate the def file from the symbol export list, so that
    # the compiled dll only exports the symbol export list.
    # Be careful not to strip the DATA tag left be newer dlltools.
    export_symbols_cmds="$ltdll_cmds"'
      $DLLTOOL --export-all --exclude-symbols '$dll_exclude_symbols' --output-def $output_objdir/$soname-def '$ltdll_obj'$libobjs $convenience~
      sed -e "1,/EXPORTS/d" -e "s/ @ [[0-9]]*//" -e "s/ *;.*$//" < $output_objdir/$soname-def > $export_symbols'

    # If the export-symbols file already is a .def file (1st line
    # is EXPORTS), use it as is.
    # If DATA tags from a recent dlltool are present, honour them!
    archive_expsym_cmds='if test "x`head -1 $export_symbols`" = xEXPORTS; then
	cp $export_symbols $output_objdir/$soname-def;
      else
	echo EXPORTS > $output_objdir/$soname-def;
	_lt_hint=1;
	cat $export_symbols | while read symbol; do
	 set dummy \$symbol;
	 case \[$]# in
	   2) echo "   \[$]2 @ \$_lt_hint ; " >> $output_objdir/$soname-def;;
	   *) echo "     \[$]2 @ \$_lt_hint \[$]3 ; " >> $output_objdir/$soname-def;;
	 esac;
	 _lt_hint=`expr 1 + \$_lt_hint`;
	done;
      fi~
      '"$ltdll_cmds"'
      $CC -Wl,--base-file,$output_objdir/$soname-base '$lt_cv_cc_dll_switch' -Wl,-e,'$dll_entry' -o $output_objdir/$soname '$ltdll_obj'$libobjs $deplibs $compiler_flags~
      $DLLTOOL --as=$AS --dllname $soname --exclude-symbols '$dll_exclude_symbols' --def $output_objdir/$soname-def --base-file $output_objdir/$soname-base --output-exp $output_objdir/$soname-exp~
      $CC -Wl,--base-file,$output_objdir/$soname-base $output_objdir/$soname-exp '$lt_cv_cc_dll_switch' -Wl,-e,'$dll_entry' -o $output_objdir/$soname '$ltdll_obj'$libobjs $deplibs $compiler_flags~
      $DLLTOOL --as=$AS --dllname $soname --exclude-symbols '$dll_exclude_symbols' --def $output_objdir/$soname-def --base-file $output_objdir/$soname-base --output-exp $output_objdir/$soname-exp --output-lib $output_objdir/$libname.dll.a~
      $CC $output_objdir/$soname-exp '$lt_cv_cc_dll_switch' -Wl,-e,'$dll_entry' -o $output_objdir/$soname '$ltdll_obj'$libobjs $deplibs $compiler_flags'
    ;;

  netbsd*)
    if echo __ELF__ | $CC -E - | grep __ELF__ >/dev/null; then
      archive_cmds='$LD -Bshareable $libobjs $deplibs $linker_flags -o $lib'
      wlarc=
    else
      archive_cmds='$CC -shared -nodefaultlibs $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname -o $lib'
      archive_expsym_cmds='$CC -shared -nodefaultlibs $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname ${wl}-retain-symbols-file $wl$export_symbols -o $lib'
    fi
    ;;

  solaris* | sysv5*)
    if $LD -v 2>&1 | egrep 'BFD 2\.8' > /dev/null; then
      ld_shlibs=no
      cat <<EOF 1>&2

*** Warning: The releases 2.8.* of the GNU linker cannot reliably
*** create shared libraries on Solaris systems.  Therefore, libtool
*** is disabling shared libraries support.  We urge you to upgrade GNU
*** binutils to release 2.9.1 or newer.  Another option is to modify
*** your PATH or compiler configuration so that the native linker is
*** used, and then restart.

EOF
    elif $LD --help 2>&1 | egrep ': supported targets:.* elf' > /dev/null; then
      archive_cmds='$CC -shared $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname -o $lib'
      archive_expsym_cmds='$CC -shared $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname ${wl}-retain-symbols-file $wl$export_symbols -o $lib'
    else
      ld_shlibs=no
    fi
    ;;

  sunos4*)
    archive_cmds='$LD -assert pure-text -Bshareable -o $lib $libobjs $deplibs $linker_flags'
    wlarc=
    hardcode_direct=yes
    hardcode_shlibpath_var=no
    ;;

  *)
    if $LD --help 2>&1 | egrep ': supported targets:.* elf' > /dev/null; then
      archive_cmds='$CC -shared $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname -o $lib'
      archive_expsym_cmds='$CC -shared $libobjs $deplibs $compiler_flags ${wl}-soname $wl$soname ${wl}-retain-symbols-file $wl$export_symbols -o $lib'
    else
      ld_shlibs=no
    fi
    ;;
  esac

  if test "$ld_shlibs" = yes; then
    runpath_var=LD_RUN_PATH
    hardcode_libdir_flag_spec='${wl}--rpath ${wl}$libdir'
    export_dynamic_flag_spec='${wl}--export-dynamic'
    case $host_os in
    cygwin* | mingw* | pw32*)
      # dlltool doesn't understand --whole-archive et. al.
      whole_archive_flag_spec=
      ;;
    *)
      # ancient GNU ld didn't support --whole-archive et. al.
      if $LD --help 2>&1 | egrep 'no-whole-archive' > /dev/null; then
	whole_archive_flag_spec="$wlarc"'--whole-archive$convenience '"$wlarc"'--no-whole-archive'
      else
	whole_archive_flag_spec=
      fi
      ;;
    esac
  fi
else
  # PORTME fill in a description of your system's linker (not GNU ld)
  case $host_os in
  aix3*)
    allow_undefined_flag=unsupported
    always_export_symbols=yes
    archive_expsym_cmds='$LD -o $output_objdir/$soname $libobjs $deplibs $linker_flags -bE:$export_symbols -T512 -H512 -bM:SRE~$AR $AR_FLAGS $lib $output_objdir/$soname'
    # Note: this linker hardcodes the directories in LIBPATH if there
    # are no directories specified by -L.
    hardcode_minus_L=yes
    if test "$GCC" = yes && test -z "$link_static_flag"; then
      # Neither direct hardcoding nor static linking is supported with a
      # broken collect2.
      hardcode_direct=unsupported
    fi
    ;;

  aix4* | aix5*)
    if test "$host_cpu" = ia64; then
      # On IA64, the linker does run time linking by default, so we don't
      # have to do anything special.
      aix_use_runtimelinking=no
      exp_sym_flag='-Bexport'
      no_entry_flag=""
    else
      aix_use_runtimelinking=no

      # Test if we are trying to use run time linking or normal
      # AIX style linking. If -brtl is somewhere in LDFLAGS, we
      # need to do runtime linking.
      case $host_os in aix4.[[23]]|aix4.[[23]].*|aix5*)
	for ld_flag in $LDFLAGS; do
	  if (test $ld_flag = "-brtl" || test $ld_flag = "-Wl,-brtl"); then
	    aix_use_runtimelinking=yes
	    break
	  fi
	done
      esac

      exp_sym_flag='-bexport'
      no_entry_flag='-bnoentry'
    fi

    # When large executables or shared objects are built, AIX ld can
    # have problems creating the table of contents.  If linking a library
    # or program results in "error TOC overflow" add -mminimal-toc to
    # CXXFLAGS/CFLAGS for g++/gcc.  In the cases where that is not
    # enough to fix the problem, add -Wl,-bbigtoc to LDFLAGS.

    hardcode_direct=yes
    archive_cmds=''
    hardcode_libdir_separator=':'
    if test "$GCC" = yes; then
      case $host_os in aix4.[[012]]|aix4.[[012]].*)
	collect2name=`${CC} -print-prog-name=collect2`
	if test -f "$collect2name" && \
	  strings "$collect2name" | grep resolve_lib_name >/dev/null
	then
	  # We have reworked collect2
	  hardcode_direct=yes
	else
	  # We have old collect2
	  hardcode_direct=unsupported
	  # It fails to find uninstalled libraries when the uninstalled
	  # path is not listed in the libpath.  Setting hardcode_minus_L
	  # to unsupported forces relinking
	  hardcode_minus_L=yes
	  hardcode_libdir_flag_spec='-L$libdir'
	  hardcode_libdir_separator=
	fi
      esac

      shared_flag='-shared'
    else
      # not using gcc
      if test "$host_cpu" = ia64; then
	shared_flag='${wl}-G'
      else
	if test "$aix_use_runtimelinking" = yes; then
	  shared_flag='${wl}-G'
	else
	  shared_flag='${wl}-bM:SRE'
	fi
      fi
    fi

    # It seems that -bexpall can do strange things, so it is better to
    # generate a list of symbols to export.
    always_export_symbols=yes
    if test "$aix_use_runtimelinking" = yes; then
      # Warning - without using the other runtime loading flags (-brtl),
      # -berok will link without error, but may produce a broken library.
      allow_undefined_flag='-berok'
      hardcode_libdir_flag_spec='${wl}-blibpath:$libdir:/usr/lib:/lib'
      archive_expsym_cmds="\$CC"' -o $output_objdir/$soname $libobjs $deplibs $compiler_flags `if test "x${allow_undefined_flag}" != "x"; then echo "${wl}${allow_undefined_flag}"; else :; fi` '"\${wl}$no_entry_flag \${wl}$exp_sym_flag:\$export_symbols $shared_flag"
    else
      if test "$host_cpu" = ia64; then
	hardcode_libdir_flag_spec='${wl}-R $libdir:/usr/lib:/lib'
	allow_undefined_flag="-z nodefs"
	archive_expsym_cmds="\$CC $shared_flag"' -o $output_objdir/$soname ${wl}-h$soname $libobjs $deplibs $compiler_flags ${wl}${allow_undefined_flag} '"\${wl}$no_entry_flag \${wl}$exp_sym_flag:\$export_symbols"
      else
	hardcode_libdir_flag_spec='${wl}-bnolibpath ${wl}-blibpath:$libdir:/usr/lib:/lib'
	# Warning - without using the other run time loading flags,
	# -berok will link without error, but may produce a broken library.
	allow_undefined_flag='${wl}-berok'
	# This is a bit strange, but is similar to how AIX traditionally builds
	# it's shared libraries.
	archive_expsym_cmds="\$CC $shared_flag"' -o $output_objdir/$soname $libobjs $deplibs $compiler_flags ${allow_undefined_flag} '"\${wl}$no_entry_flag \${wl}$exp_sym_flag:\$export_symbols"' ~$AR -crlo $objdir/$libname$release.a $objdir/$soname'
      fi
    fi
    ;;

  amigaos*)
    archive_cmds='$rm $output_objdir/a2ixlibrary.data~$echo "#define NAME $libname" > $output_objdir/a2ixlibrary.data~$echo "#define LIBRARY_ID 1" >> $output_objdir/a2ixlibrary.data~$echo "#define VERSION $major" >> $output_objdir/a2ixlibrary.data~$echo "#define REVISION $revision" >> $output_objdir/a2ixlibrary.data~$AR $AR_FLAGS $lib $libobjs~$RANLIB $lib~(cd $output_objdir && a2ixlibrary -32)'
    hardcode_libdir_flag_spec='-L$libdir'
    hardcode_minus_L=yes
    # see comment about different semantics on the GNU ld section
    ld_shlibs=no
    ;;

  cygwin* | mingw* | pw32*)
    # When not using gcc, we currently assume that we are using
    # Microsoft Visual C++.
    # hardcode_libdir_flag_spec is actually meaningless, as there is
    # no search path for DLLs.
    hardcode_libdir_flag_spec=' '
    allow_undefined_flag=unsupported
    # Tell ltmain to make .lib files, not .a files.
    libext=lib
    # FIXME: Setting linknames here is a bad hack.
    archive_cmds='$CC -o $lib $libobjs $compiler_flags `echo "$deplibs" | sed -e '\''s/ -lc$//'\''` -link -dll~linknames='
    # The linker will automatically build a .lib file if we build a DLL.
    old_archive_from_new_cmds='true'
    # FIXME: Should let the user specify the lib program.
    old_archive_cmds='lib /OUT:$oldlib$oldobjs$old_deplibs'
    fix_srcfile_path='`cygpath -w "$srcfile"`'
    ;;

  darwin* | rhapsody*)
    case "$host_os" in
    rhapsody* | darwin1.[[012]])
      allow_undefined_flag='-undefined suppress'
      ;;
    *) # Darwin 1.3 on
      allow_undefined_flag='-flat_namespace -undefined suppress'
      ;;
    esac
    # FIXME: Relying on posixy $() will cause problems for
    #        cross-compilation, but unfortunately the echo tests do not
    #        yet detect zsh echo's removal of \ escapes.
    archive_cmds='$nonopt $(test "x$module" = xyes && echo -bundle || echo -dynamiclib) $allow_undefined_flag -o $lib $libobjs $deplibs$linker_flags -install_name $rpath/$soname $verstring'
    # We need to add '_' to the symbols in $export_symbols first
    #archive_expsym_cmds="$archive_cmds"' && strip -s $export_symbols'
    hardcode_direct=yes
    hardcode_shlibpath_var=no
    whole_archive_flag_spec='-all_load $convenience'
    ;;

  freebsd1*)
    ld_shlibs=no
    ;;

  # FreeBSD 2.2.[012] allows us to include c++rt0.o to get C++ constructor
  # support.  Future versions do this automatically, but an explicit c++rt0.o
  # does not break anything, and helps significantly (at the cost of a little
  # extra space).
  freebsd2.2*)
    archive_cmds='$LD -Bshareable -o $lib $libobjs $deplibs $linker_flags /usr/lib/c++rt0.o'
    hardcode_libdir_flag_spec='-R$libdir'
    hardcode_direct=yes
    hardcode_shlibpath_var=no
    ;;

  # Unfortunately, older versions of FreeBSD 2 do not have this feature.
  freebsd2*)
    archive_cmds='$LD -Bshareable -o $lib $libobjs $deplibs $linker_flags'
    hardcode_direct=yes
    hardcode_minus_L=yes
    hardcode_shlibpath_var=no
    ;;

  # FreeBSD 3 and greater uses gcc -shared to do shared libraries.
  freebsd*)
    archive_cmds='$CC -shared -o $lib $libobjs $deplibs $compiler_flags'
    hardcode_libdir_flag_spec='-R$libdir'
    hardcode_direct=yes
    hardcode_shlibpath_var=no
    ;;

  hpux9* | hpux10* | hpux11*)
    case $host_os in
    hpux9*) archive_cmds='$rm $output_objdir/$soname~$LD -b +b $install_libdir -o $output_objdir/$soname $libobjs $deplibs $linker_flags~test $output_objdir/$soname = $lib || mv $output_objdir/$soname $lib' ;;
    *) archive_cmds='$LD -b +h $soname +b $install_libdir -o $lib $libobjs $deplibs $linker_flags' ;;
    esac
    hardcode_libdir_flag_spec='${wl}+b ${wl}$libdir'
    hardcode_libdir_separator=:
    hardcode_direct=yes
    hardcode_minus_L=yes # Not in the search PATH, but as the default
			 # location of the library.
    export_dynamic_flag_spec='${wl}-E'
    ;;

  irix5* | irix6*)
    if test "$GCC" = yes; then
      archive_cmds='$CC -shared $libobjs $deplibs $compiler_flags ${wl}-soname ${wl}$soname `test -n "$verstring" && echo ${wl}-set_version ${wl}$verstring` ${wl}-update_registry ${wl}${output_objdir}/so_locations -o $lib'
    else
      archive_cmds='$LD -shared $libobjs $deplibs $linker_flags -soname $soname `test -n "$verstring" && echo -set_version $verstring` -update_registry ${output_objdir}/so_locations -o $lib'
    fi
    hardcode_libdir_flag_spec='${wl}-rpath ${wl}$libdir'
    hardcode_libdir_separator=:
    link_all_deplibs=yes
    ;;

  netbsd*)
    if echo __ELF__ | $CC -E - | grep __ELF__ >/dev/null; then
      archive_cmds='$LD -Bshareable -o $lib $libobjs $deplibs $linker_flags'  # a.out
    else
      archive_cmds='$LD -shared -o $lib $libobjs $deplibs $linker_flags'      # ELF
    fi
    hardcode_libdir_flag_spec='-R$libdir'
    hardcode_direct=yes
    hardcode_shlibpath_var=no
    ;;

  newsos6)
    archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
    hardcode_direct=yes
    hardcode_libdir_flag_spec='${wl}-rpath ${wl}$libdir'
    hardcode_libdir_separator=:
    hardcode_shlibpath_var=no
    ;;

  openbsd*)
    hardcode_direct=yes
    hardcode_shlibpath_var=no
    if test -z "`echo __ELF__ | $CC -E - | grep __ELF__`" || test "$host_os-$host_cpu" = "openbsd2.8-powerpc"; then
      archive_cmds='$CC -shared $pic_flag -o $lib $libobjs $deplibs $linker_flags'
      hardcode_libdir_flag_spec='${wl}-rpath,$libdir'
      export_dynamic_flag_spec='${wl}-E'
    else
      case "$host_os" in
      openbsd[[01]].* | openbsd2.[[0-7]] | openbsd2.[[0-7]].*)
	archive_cmds='$LD -Bshareable -o $lib $libobjs $deplibs $linker_flags'
	hardcode_libdir_flag_spec='-R$libdir'
        ;;
      *)
        archive_cmds='$CC -shared $pic_flag -o $lib $libobjs $deplibs $linker_flags'
        hardcode_libdir_flag_spec='${wl}-rpath,$libdir'
        ;;
      esac
    fi
    ;;

  os2*)
    hardcode_libdir_flag_spec='-L$libdir'
    hardcode_minus_L=yes
    allow_undefined_flag=unsupported
    archive_cmds='$echo "LIBRARY $libname INITINSTANCE" > $output_objdir/$libname.def~$echo "DESCRIPTION \"$libname\"" >> $output_objdir/$libname.def~$echo DATA >> $output_objdir/$libname.def~$echo " SINGLE NONSHARED" >> $output_objdir/$libname.def~$echo EXPORTS >> $output_objdir/$libname.def~emxexp $libobjs >> $output_objdir/$libname.def~$CC -Zdll -Zcrtdll -o $lib $libobjs $deplibs $compiler_flags $output_objdir/$libname.def'
    old_archive_from_new_cmds='emximp -o $output_objdir/$libname.a $output_objdir/$libname.def'
    ;;

  osf3*)
    if test "$GCC" = yes; then
      allow_undefined_flag=' ${wl}-expect_unresolved ${wl}\*'
      archive_cmds='$CC -shared${allow_undefined_flag} $libobjs $deplibs $compiler_flags ${wl}-soname ${wl}$soname `test -n "$verstring" && echo ${wl}-set_version ${wl}$verstring` ${wl}-update_registry ${wl}${output_objdir}/so_locations -o $lib'
    else
      allow_undefined_flag=' -expect_unresolved \*'
      archive_cmds='$LD -shared${allow_undefined_flag} $libobjs $deplibs $linker_flags -soname $soname `test -n "$verstring" && echo -set_version $verstring` -update_registry ${output_objdir}/so_locations -o $lib'
    fi
    hardcode_libdir_flag_spec='${wl}-rpath ${wl}$libdir'
    hardcode_libdir_separator=:
    ;;

  osf4* | osf5*)	# as osf3* with the addition of -msym flag
    if test "$GCC" = yes; then
      allow_undefined_flag=' ${wl}-expect_unresolved ${wl}\*'
      archive_cmds='$CC -shared${allow_undefined_flag} $libobjs $deplibs $compiler_flags ${wl}-msym ${wl}-soname ${wl}$soname `test -n "$verstring" && echo ${wl}-set_version ${wl}$verstring` ${wl}-update_registry ${wl}${output_objdir}/so_locations -o $lib'
      hardcode_libdir_flag_spec='${wl}-rpath ${wl}$libdir'
    else
      allow_undefined_flag=' -expect_unresolved \*'
      archive_cmds='$LD -shared${allow_undefined_flag} $libobjs $deplibs $linker_flags -msym -soname $soname `test -n "$verstring" && echo -set_version $verstring` -update_registry ${output_objdir}/so_locations -o $lib'
      archive_expsym_cmds='for i in `cat $export_symbols`; do printf "-exported_symbol " >> $lib.exp; echo "\$i" >> $lib.exp; done; echo "-hidden">> $lib.exp~
      $LD -shared${allow_undefined_flag} -input $lib.exp $linker_flags $libobjs $deplibs -soname $soname `test -n "$verstring" && echo -set_version $verstring` -update_registry ${objdir}/so_locations -o $lib~$rm $lib.exp'

      #Both c and cxx compiler support -rpath directly
      hardcode_libdir_flag_spec='-rpath $libdir'
    fi
    hardcode_libdir_separator=:
    ;;

  sco3.2v5*)
    archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
    hardcode_shlibpath_var=no
    runpath_var=LD_RUN_PATH
    hardcode_runpath_var=yes
    export_dynamic_flag_spec='${wl}-Bexport'
    ;;

  solaris*)
    # gcc --version < 3.0 without binutils cannot create self contained
    # shared libraries reliably, requiring libgcc.a to resolve some of
    # the object symbols generated in some cases.  Libraries that use
    # assert need libgcc.a to resolve __eprintf, for example.  Linking
    # a copy of libgcc.a into every shared library to guarantee resolving
    # such symbols causes other problems:  According to Tim Van Holder
    # <tim.van.holder@pandora.be>, C++ libraries end up with a separate
    # (to the application) exception stack for one thing.
    no_undefined_flag=' -z defs'
    if test "$GCC" = yes; then
      case `$CC --version 2>/dev/null` in
      [[12]].*)
	cat <<EOF 1>&2

*** Warning: Releases of GCC earlier than version 3.0 cannot reliably
*** create self contained shared libraries on Solaris systems, without
*** introducing a dependency on libgcc.a.  Therefore, libtool is disabling
*** -no-undefined support, which will at least allow you to build shared
*** libraries.  However, you may find that when you link such libraries
*** into an application without using GCC, you have to manually add
*** \`gcc --print-libgcc-file-name\` to the link command.  We urge you to
*** upgrade to a newer version of GCC.  Another option is to rebuild your
*** current GCC to use the GNU linker from GNU binutils 2.9.1 or newer.

EOF
        no_undefined_flag=
	;;
      esac
    fi
    # $CC -shared without GNU ld will not create a library from C++
    # object files and a static libstdc++, better avoid it by now
    archive_cmds='$LD -G${allow_undefined_flag} -h $soname -o $lib $libobjs $deplibs $linker_flags'
    archive_expsym_cmds='$echo "{ global:" > $lib.exp~cat $export_symbols | sed -e "s/\(.*\)/\1;/" >> $lib.exp~$echo "local: *; };" >> $lib.exp~
		$LD -G${allow_undefined_flag} -M $lib.exp -h $soname -o $lib $libobjs $deplibs $linker_flags~$rm $lib.exp'
    hardcode_libdir_flag_spec='-R$libdir'
    hardcode_shlibpath_var=no
    case $host_os in
    solaris2.[[0-5]] | solaris2.[[0-5]].*) ;;
    *) # Supported since Solaris 2.6 (maybe 2.5.1?)
      whole_archive_flag_spec='-z allextract$convenience -z defaultextract' ;;
    esac
    link_all_deplibs=yes
    ;;

  sunos4*)
    if test "x$host_vendor" = xsequent; then
      # Use $CC to link under sequent, because it throws in some extra .o
      # files that make .init and .fini sections work.
      archive_cmds='$CC -G ${wl}-h $soname -o $lib $libobjs $deplibs $compiler_flags'
    else
      archive_cmds='$LD -assert pure-text -Bstatic -o $lib $libobjs $deplibs $linker_flags'
    fi
    hardcode_libdir_flag_spec='-L$libdir'
    hardcode_direct=yes
    hardcode_minus_L=yes
    hardcode_shlibpath_var=no
    ;;

  sysv4)
    if test "x$host_vendor" = xsno; then
      archive_cmds='$LD -G -Bsymbolic -h $soname -o $lib $libobjs $deplibs $linker_flags'
      hardcode_direct=yes # is this really true???
    else
      archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
      hardcode_direct=no #Motorola manual says yes, but my tests say they lie
    fi
    runpath_var='LD_RUN_PATH'
    hardcode_shlibpath_var=no
    ;;

  sysv4.3*)
    archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
    hardcode_shlibpath_var=no
    export_dynamic_flag_spec='-Bexport'
    ;;

  sysv5*)
    no_undefined_flag=' -z text'
    # $CC -shared without GNU ld will not create a library from C++
    # object files and a static libstdc++, better avoid it by now
    archive_cmds='$LD -G${allow_undefined_flag} -h $soname -o $lib $libobjs $deplibs $linker_flags'
    archive_expsym_cmds='$echo "{ global:" > $lib.exp~cat $export_symbols | sed -e "s/\(.*\)/\1;/" >> $lib.exp~$echo "local: *; };" >> $lib.exp~
		$LD -G${allow_undefined_flag} -M $lib.exp -h $soname -o $lib $libobjs $deplibs $linker_flags~$rm $lib.exp'
    hardcode_libdir_flag_spec=
    hardcode_shlibpath_var=no
    runpath_var='LD_RUN_PATH'
    ;;

  uts4*)
    archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
    hardcode_libdir_flag_spec='-L$libdir'
    hardcode_shlibpath_var=no
    ;;

  dgux*)
    archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
    hardcode_libdir_flag_spec='-L$libdir'
    hardcode_shlibpath_var=no
    ;;

  sysv4*MP*)
    if test -d /usr/nec; then
      archive_cmds='$LD -G -h $soname -o $lib $libobjs $deplibs $linker_flags'
      hardcode_shlibpath_var=no
      runpath_var=LD_RUN_PATH
      hardcode_runpath_var=yes
      ld_shlibs=yes
    fi
    ;;

  sysv4.2uw2*)
    archive_cmds='$LD -G -o $lib $libobjs $deplibs $linker_flags'
    hardcode_direct=yes
    hardcode_minus_L=no
    hardcode_shlibpath_var=no
    hardcode_runpath_var=yes
    runpath_var=LD_RUN_PATH
    ;;

  sysv5uw7* | unixware7*)
    no_undefined_flag='${wl}-z ${wl}text'
    if test "$GCC" = yes; then
      archive_cmds='$CC -shared ${wl}-h ${wl}$soname -o $lib $libobjs $deplibs $compiler_flags'
    else
      archive_cmds='$CC -G ${wl}-h ${wl}$soname -o $lib $libobjs $deplibs $compiler_flags'
    fi
    runpath_var='LD_RUN_PATH'
    hardcode_shlibpath_var=no
    ;;

  *)
    ld_shlibs=no
    ;;
  esac
fi
AC_MSG_RESULT([$ld_shlibs])
test "$ld_shlibs" = no && can_build_shared=no

# Check hardcoding attributes.
AC_MSG_CHECKING([how to hardcode library paths into programs])
hardcode_action=
if test -n "$hardcode_libdir_flag_spec" || \
   test -n "$runpath_var"; then

  # We can hardcode non-existant directories.
  if test "$hardcode_direct" != no &&
     # If the only mechanism to avoid hardcoding is shlibpath_var, we
     # have to relink, otherwise we might link with an installed library
     # when we should be linking with a yet-to-be-installed one
     ## test "$hardcode_shlibpath_var" != no &&
     test "$hardcode_minus_L" != no; then
    # Linking always hardcodes the temporary library directory.
    hardcode_action=relink
  else
    # We can link without hardcoding, and we can hardcode nonexisting dirs.
    hardcode_action=immediate
  fi
else
  # We cannot hardcode anything, or else we can only hardcode existing
  # directories.
  hardcode_action=unsupported
fi
AC_MSG_RESULT([$hardcode_action])

striplib=
old_striplib=
AC_MSG_CHECKING([whether stripping libraries is possible])
if test -n "$STRIP" && $STRIP -V 2>&1 | grep "GNU strip" >/dev/null; then
  test -z "$old_striplib" && old_striplib="$STRIP --strip-debug"
  test -z "$striplib" && striplib="$STRIP --strip-unneeded"
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

reload_cmds='$LD$reload_flag -o $output$reload_objs'
test -z "$deplibs_check_method" && deplibs_check_method=unknown

# PORTME Fill in your ld.so characteristics
AC_MSG_CHECKING([dynamic linker characteristics])
library_names_spec=
libname_spec='lib$name'
soname_spec=
postinstall_cmds=
postuninstall_cmds=
finish_cmds=
finish_eval=
shlibpath_var=
shlibpath_overrides_runpath=unknown
version_type=none
dynamic_linker="$host_os ld.so"
sys_lib_dlsearch_path_spec="/lib /usr/lib"
sys_lib_search_path_spec="/lib /usr/lib /usr/local/lib"

case $host_os in
aix3*)
  version_type=linux
  library_names_spec='${libname}${release}.so$versuffix $libname.a'
  shlibpath_var=LIBPATH

  # AIX has no versioning support, so we append a major version to the name.
  soname_spec='${libname}${release}.so$major'
  ;;

aix4* | aix5*)
  version_type=linux
  if test "$host_cpu" = ia64; then
    # AIX 5 supports IA64
    library_names_spec='${libname}${release}.so$major ${libname}${release}.so$versuffix $libname.so'
    shlibpath_var=LD_LIBRARY_PATH
  else
    # With GCC up to 2.95.x, collect2 would create an import file
    # for dependence libraries.  The import file would start with
    # the line `#! .'.  This would cause the generated library to
    # depend on `.', always an invalid library.  This was fixed in
    # development snapshots of GCC prior to 3.0.
    case $host_os in
      aix4 | aix4.[[01]] | aix4.[[01]].*)
	if { echo '#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 97)'
	     echo ' yes '
	     echo '#endif'; } | ${CC} -E - | grep yes > /dev/null; then
	  :
	else
	  can_build_shared=no
	fi
	;;
    esac
    # AIX (on Power*) has no versioning support, so currently we can
    # not hardcode correct soname into executable. Probably we can
    # add versioning support to collect2, so additional links can
    # be useful in future.
    if test "$aix_use_runtimelinking" = yes; then
      # If using run time linking (on AIX 4.2 or later) use lib<name>.so
      # instead of lib<name>.a to let people know that these are not
      # typical AIX shared libraries.
      library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
    else
      # We preserve .a as extension for shared libraries through AIX4.2
      # and later when we are not doing run time linking.
      library_names_spec='${libname}${release}.a $libname.a'
      soname_spec='${libname}${release}.so$major'
    fi
    shlibpath_var=LIBPATH
  fi
  ;;

amigaos*)
  library_names_spec='$libname.ixlibrary $libname.a'
  # Create ${libname}_ixlibrary.a entries in /sys/libs.
  finish_eval='for lib in `ls $libdir/*.ixlibrary 2>/dev/null`; do libname=`$echo "X$lib" | $Xsed -e '\''s%^.*/\([[^/]]*\)\.ixlibrary$%\1%'\''`; test $rm /sys/libs/${libname}_ixlibrary.a; $show "(cd /sys/libs && $LN_S $lib ${libname}_ixlibrary.a)"; (cd /sys/libs && $LN_S $lib ${libname}_ixlibrary.a) || exit 1; done'
  ;;

beos*)
  library_names_spec='${libname}.so'
  dynamic_linker="$host_os ld.so"
  shlibpath_var=LIBRARY_PATH
  ;;

bsdi4*)
  version_type=linux
  need_version=no
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  soname_spec='${libname}${release}.so$major'
  finish_cmds='PATH="\$PATH:/sbin" ldconfig $libdir'
  shlibpath_var=LD_LIBRARY_PATH
  sys_lib_search_path_spec="/shlib /usr/lib /usr/X11/lib /usr/contrib/lib /lib /usr/local/lib"
  sys_lib_dlsearch_path_spec="/shlib /usr/lib /usr/local/lib"
  export_dynamic_flag_spec=-rdynamic
  # the default ld.so.conf also contains /usr/contrib/lib and
  # /usr/X11R6/lib (/usr/X11 is a link to /usr/X11R6), but let us allow
  # libtool to hard-code these into programs
  ;;

cygwin* | mingw* | pw32*)
  version_type=windows
  need_version=no
  need_lib_prefix=no
  case $GCC,$host_os in
  yes,cygwin*)
    library_names_spec='$libname.dll.a'
    soname_spec='`echo ${libname} | sed -e 's/^lib/cyg/'``echo ${release} | sed -e 's/[[.]]/-/g'`${versuffix}.dll'
    postinstall_cmds='dlpath=`bash 2>&1 -c '\''. $dir/${file}i;echo \$dlname'\''`~
      dldir=$destdir/`dirname \$dlpath`~
      test -d \$dldir || mkdir -p \$dldir~
      $install_prog .libs/$dlname \$dldir/$dlname'
    postuninstall_cmds='dldll=`bash 2>&1 -c '\''. $file; echo \$dlname'\''`~
      dlpath=$dir/\$dldll~
       $rm \$dlpath'
    ;;
  yes,mingw*)
    library_names_spec='${libname}`echo ${release} | sed -e 's/[[.]]/-/g'`${versuffix}.dll'
    sys_lib_search_path_spec=`$CC -print-search-dirs | grep "^libraries:" | sed -e "s/^libraries://" -e "s/;/ /g"`
    ;;
  yes,pw32*)
    library_names_spec='`echo ${libname} | sed -e 's/^lib/pw/'``echo ${release} | sed -e 's/[.]/-/g'`${versuffix}.dll'
    ;;
  *)
    library_names_spec='${libname}`echo ${release} | sed -e 's/[[.]]/-/g'`${versuffix}.dll $libname.lib'
    ;;
  esac
  dynamic_linker='Win32 ld.exe'
  # FIXME: first we should search . and the directory the executable is in
  shlibpath_var=PATH
  ;;

darwin* | rhapsody*)
  dynamic_linker="$host_os dyld"
  version_type=darwin
  need_lib_prefix=no
  need_version=no
  # FIXME: Relying on posixy $() will cause problems for
  #        cross-compilation, but unfortunately the echo tests do not
  #        yet detect zsh echo's removal of \ escapes.
  library_names_spec='${libname}${release}${versuffix}.$(test .$module = .yes && echo so || echo dylib) ${libname}${release}${major}.$(test .$module = .yes && echo so || echo dylib) ${libname}.$(test .$module = .yes && echo so || echo dylib)'
  soname_spec='${libname}${release}${major}.$(test .$module = .yes && echo so || echo dylib)'
  shlibpath_overrides_runpath=yes
  shlibpath_var=DYLD_LIBRARY_PATH
  ;;

freebsd1*)
  dynamic_linker=no
  ;;

freebsd*)
  objformat=`test -x /usr/bin/objformat && /usr/bin/objformat || echo aout`
  version_type=freebsd-$objformat
  case $version_type in
    freebsd-elf*)
      library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so $libname.so'
      need_version=no
      need_lib_prefix=no
      ;;
    freebsd-*)
      library_names_spec='${libname}${release}.so$versuffix $libname.so$versuffix'
      need_version=yes
      ;;
  esac
  shlibpath_var=LD_LIBRARY_PATH
  case $host_os in
  freebsd2*)
    shlibpath_overrides_runpath=yes
    ;;
  *)
    shlibpath_overrides_runpath=no
    hardcode_into_libs=yes
    ;;
  esac
  ;;

gnu*)
  version_type=linux
  need_lib_prefix=no
  need_version=no
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so${major} ${libname}.so'
  soname_spec='${libname}${release}.so$major'
  shlibpath_var=LD_LIBRARY_PATH
  hardcode_into_libs=yes
  ;;

hpux9* | hpux10* | hpux11*)
  # Give a soname corresponding to the major version so that dld.sl refuses to
  # link against other versions.
  dynamic_linker="$host_os dld.sl"
  version_type=sunos
  need_lib_prefix=no
  need_version=no
  shlibpath_var=SHLIB_PATH
  shlibpath_overrides_runpath=no # +s is required to enable SHLIB_PATH
  library_names_spec='${libname}${release}.sl$versuffix ${libname}${release}.sl$major $libname.sl'
  soname_spec='${libname}${release}.sl$major'
  # HP-UX runs *really* slowly unless shared libraries are mode 555.
  postinstall_cmds='chmod 555 $lib'
  ;;

irix5* | irix6*)
  version_type=irix
  need_lib_prefix=no
  need_version=no
  soname_spec='${libname}${release}.so$major'
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major ${libname}${release}.so $libname.so'
  case $host_os in
  irix5*)
    libsuff= shlibsuff=
    ;;
  *)
    case $LD in # libtool.m4 will add one of these switches to LD
    *-32|*"-32 ") libsuff= shlibsuff= libmagic=32-bit;;
    *-n32|*"-n32 ") libsuff=32 shlibsuff=N32 libmagic=N32;;
    *-64|*"-64 ") libsuff=64 shlibsuff=64 libmagic=64-bit;;
    *) libsuff= shlibsuff= libmagic=never-match;;
    esac
    ;;
  esac
  shlibpath_var=LD_LIBRARY${shlibsuff}_PATH
  shlibpath_overrides_runpath=no
  sys_lib_search_path_spec="/usr/lib${libsuff} /lib${libsuff} /usr/local/lib${libsuff}"
  sys_lib_dlsearch_path_spec="/usr/lib${libsuff} /lib${libsuff}"
  ;;

# No shared lib support for Linux oldld, aout, or coff.
linux-gnuoldld* | linux-gnuaout* | linux-gnucoff*)
  dynamic_linker=no
  ;;

# This must be Linux ELF.
linux-gnu*)
  version_type=linux
  need_lib_prefix=no
  need_version=no
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  soname_spec='${libname}${release}.so$major'
  finish_cmds='PATH="\$PATH:/sbin" ldconfig -n $libdir'
  shlibpath_var=LD_LIBRARY_PATH
  shlibpath_overrides_runpath=no
  # This implies no fast_install, which is unacceptable.
  # Some rework will be needed to allow for fast_install
  # before this can be enabled.
  hardcode_into_libs=yes

  # We used to test for /lib/ld.so.1 and disable shared libraries on
  # powerpc, because MkLinux only supported shared libraries with the
  # GNU dynamic linker.  Since this was broken with cross compilers,
  # most powerpc-linux boxes support dynamic linking these days and
  # people can always --disable-shared, the test was removed, and we
  # assume the GNU/Linux dynamic linker is in use.
  dynamic_linker='GNU/Linux ld.so'
  ;;

netbsd*)
  version_type=sunos
  need_lib_prefix=no
  need_version=no
  if echo __ELF__ | $CC -E - | grep __ELF__ >/dev/null; then
    library_names_spec='${libname}${release}.so$versuffix ${libname}.so$versuffix'
    finish_cmds='PATH="\$PATH:/sbin" ldconfig -m $libdir'
    dynamic_linker='NetBSD (a.out) ld.so'
  else
    library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major ${libname}${release}.so ${libname}.so'
    soname_spec='${libname}${release}.so$major'
    dynamic_linker='NetBSD ld.elf_so'
  fi
  shlibpath_var=LD_LIBRARY_PATH
  shlibpath_overrides_runpath=yes
  hardcode_into_libs=yes
  ;;

newsos6)
  version_type=linux
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  shlibpath_var=LD_LIBRARY_PATH
  shlibpath_overrides_runpath=yes
  ;;

openbsd*)
  version_type=sunos
  need_lib_prefix=no
  need_version=no
  if test -z "`echo __ELF__ | $CC -E - | grep __ELF__`" || test "$host_os-$host_cpu" = "openbsd2.8-powerpc"; then
    case "$host_os" in
    openbsd2.[[89]] | openbsd2.[[89]].*)
      shlibpath_overrides_runpath=no
      ;;
    *)
      shlibpath_overrides_runpath=yes
      ;;
    esac
  else
    shlibpath_overrides_runpath=yes
  fi
  library_names_spec='${libname}${release}.so$versuffix ${libname}.so$versuffix'
  finish_cmds='PATH="\$PATH:/sbin" ldconfig -m $libdir'
  shlibpath_var=LD_LIBRARY_PATH
  ;;

os2*)
  libname_spec='$name'
  need_lib_prefix=no
  library_names_spec='$libname.dll $libname.a'
  dynamic_linker='OS/2 ld.exe'
  shlibpath_var=LIBPATH
  ;;

osf3* | osf4* | osf5*)
  version_type=osf
  need_version=no
  soname_spec='${libname}${release}.so'
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so $libname.so'
  shlibpath_var=LD_LIBRARY_PATH
  sys_lib_search_path_spec="/usr/shlib /usr/ccs/lib /usr/lib/cmplrs/cc /usr/lib /usr/local/lib /var/shlib"
  sys_lib_dlsearch_path_spec="$sys_lib_search_path_spec"
  ;;

sco3.2v5*)
  version_type=osf
  soname_spec='${libname}${release}.so$major'
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  shlibpath_var=LD_LIBRARY_PATH
  ;;

solaris*)
  version_type=linux
  need_lib_prefix=no
  need_version=no
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  soname_spec='${libname}${release}.so$major'
  shlibpath_var=LD_LIBRARY_PATH
  shlibpath_overrides_runpath=yes
  hardcode_into_libs=yes
  # ldd complains unless libraries are executable
  postinstall_cmds='chmod +x $lib'
  ;;

sunos4*)
  version_type=sunos
  library_names_spec='${libname}${release}.so$versuffix ${libname}.so$versuffix'
  finish_cmds='PATH="\$PATH:/usr/etc" ldconfig $libdir'
  shlibpath_var=LD_LIBRARY_PATH
  shlibpath_overrides_runpath=yes
  if test "$with_gnu_ld" = yes; then
    need_lib_prefix=no
  fi
  need_version=yes
  ;;

sysv4 | sysv4.2uw2* | sysv4.3* | sysv5*)
  version_type=linux
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  soname_spec='${libname}${release}.so$major'
  shlibpath_var=LD_LIBRARY_PATH
  case $host_vendor in
    sni)
      shlibpath_overrides_runpath=no
      ;;
    motorola)
      need_lib_prefix=no
      need_version=no
      shlibpath_overrides_runpath=no
      sys_lib_search_path_spec='/lib /usr/lib /usr/ccs/lib'
      ;;
  esac
  ;;

uts4*)
  version_type=linux
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  soname_spec='${libname}${release}.so$major'
  shlibpath_var=LD_LIBRARY_PATH
  ;;

dgux*)
  version_type=linux
  need_lib_prefix=no
  need_version=no
  library_names_spec='${libname}${release}.so$versuffix ${libname}${release}.so$major $libname.so'
  soname_spec='${libname}${release}.so$major'
  shlibpath_var=LD_LIBRARY_PATH
  ;;

sysv4*MP*)
  if test -d /usr/nec ;then
    version_type=linux
    library_names_spec='$libname.so.$versuffix $libname.so.$major $libname.so'
    soname_spec='$libname.so.$major'
    shlibpath_var=LD_LIBRARY_PATH
  fi
  ;;

*)
  dynamic_linker=no
  ;;
esac
AC_MSG_RESULT([$dynamic_linker])
test "$dynamic_linker" = no && can_build_shared=no

# Report the final consequences.
AC_MSG_CHECKING([if libtool supports shared libraries])
AC_MSG_RESULT([$can_build_shared])

AC_MSG_CHECKING([whether to build shared libraries])
test "$can_build_shared" = "no" && enable_shared=no

# On AIX, shared libraries and static libraries use the same namespace, and
# are all built from PIC.
case "$host_os" in
aix3*)
  test "$enable_shared" = yes && enable_static=no
  if test -n "$RANLIB"; then
    archive_cmds="$archive_cmds~\$RANLIB \$lib"
    postinstall_cmds='$RANLIB $lib'
  fi
  ;;

aix4*)
  if test "$host_cpu" != ia64 && test "$aix_use_runtimelinking" = no ; then
    test "$enable_shared" = yes && enable_static=no
  fi
  ;;
esac
AC_MSG_RESULT([$enable_shared])

AC_MSG_CHECKING([whether to build static libraries])
# Make sure either enable_shared or enable_static is yes.
test "$enable_shared" = yes || enable_static=yes
AC_MSG_RESULT([$enable_static])

if test "$hardcode_action" = relink; then
  # Fast installation is not supported
  enable_fast_install=no
elif test "$shlibpath_overrides_runpath" = yes ||
     test "$enable_shared" = no; then
  # Fast installation is not necessary
  enable_fast_install=needless
fi

variables_saved_for_relink="PATH $shlibpath_var $runpath_var"
if test "$GCC" = yes; then
  variables_saved_for_relink="$variables_saved_for_relink GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH"
fi

AC_LIBTOOL_DLOPEN_SELF

if test "$enable_shared" = yes && test "$GCC" = yes; then
  case $archive_cmds in
  *'~'*)
    # FIXME: we may have to deal with multi-command sequences.
    ;;
  '$CC '*)
    # Test whether the compiler implicitly links with -lc since on some
    # systems, -lgcc has to come before -lc. If gcc already passes -lc
    # to ld, don't add -lc before -lgcc.
    AC_MSG_CHECKING([whether -lc should be explicitly linked in])
    AC_CACHE_VAL([lt_cv_archive_cmds_need_lc],
    [$rm conftest*
    echo 'static int dummy;' > conftest.$ac_ext

    if AC_TRY_EVAL(ac_compile); then
      soname=conftest
      lib=conftest
      libobjs=conftest.$ac_objext
      deplibs=
      wl=$lt_cv_prog_cc_wl
      compiler_flags=-v
      linker_flags=-v
      verstring=
      output_objdir=.
      libname=conftest
      save_allow_undefined_flag=$allow_undefined_flag
      allow_undefined_flag=
      if AC_TRY_EVAL(archive_cmds 2\>\&1 \| grep \" -lc \" \>/dev/null 2\>\&1)
      then
	lt_cv_archive_cmds_need_lc=no
      else
	lt_cv_archive_cmds_need_lc=yes
      fi
      allow_undefined_flag=$save_allow_undefined_flag
    else
      cat conftest.err 1>&5
    fi])
    AC_MSG_RESULT([$lt_cv_archive_cmds_need_lc])
    ;;
  esac
fi
need_lc=${lt_cv_archive_cmds_need_lc-yes}

# The second clause should only fire when bootstrapping the
# libtool distribution, otherwise you forgot to ship ltmain.sh
# with your package, and you will get complaints that there are
# no rules to generate ltmain.sh.
if test -f "$ltmain"; then
  :
else
  # If there is no Makefile yet, we rely on a make rule to execute
  # `config.status --recheck' to rerun these tests and create the
  # libtool script then.
  test -f Makefile && make "$ltmain"
fi

if test -f "$ltmain"; then
  trap "$rm \"${ofile}T\"; exit 1" 1 2 15
  $rm -f "${ofile}T"

  echo creating $ofile

  # Now quote all the things that may contain metacharacters while being
  # careful not to overquote the AC_SUBSTed values.  We take copies of the
  # variables and quote the copies for generation of the libtool script.
  for var in echo old_CC old_CFLAGS \
    AR AR_FLAGS CC LD LN_S NM SHELL \
    reload_flag reload_cmds wl \
    pic_flag link_static_flag no_builtin_flag export_dynamic_flag_spec \
    thread_safe_flag_spec whole_archive_flag_spec libname_spec \
    library_names_spec soname_spec \
    RANLIB old_archive_cmds old_archive_from_new_cmds old_postinstall_cmds \
    old_postuninstall_cmds archive_cmds archive_expsym_cmds postinstall_cmds \
    postuninstall_cmds extract_expsyms_cmds old_archive_from_expsyms_cmds \
    old_striplib striplib file_magic_cmd export_symbols_cmds \
    deplibs_check_method allow_undefined_flag no_undefined_flag \
    finish_cmds finish_eval global_symbol_pipe global_symbol_to_cdecl \
    global_symbol_to_c_name_address \
    hardcode_libdir_flag_spec hardcode_libdir_separator  \
    sys_lib_search_path_spec sys_lib_dlsearch_path_spec \
    compiler_c_o compiler_o_lo need_locks exclude_expsyms include_expsyms; do

    case $var in
    reload_cmds | old_archive_cmds | old_archive_from_new_cmds | \
    old_postinstall_cmds | old_postuninstall_cmds | \
    export_symbols_cmds | archive_cmds | archive_expsym_cmds | \
    extract_expsyms_cmds | old_archive_from_expsyms_cmds | \
    postinstall_cmds | postuninstall_cmds | \
    finish_cmds | sys_lib_search_path_spec | sys_lib_dlsearch_path_spec)
      # Double-quote double-evaled strings.
      eval "lt_$var=\\\"\`\$echo \"X\$$var\" | \$Xsed -e \"\$double_quote_subst\" -e \"\$sed_quote_subst\" -e \"\$delay_variable_subst\"\`\\\""
      ;;
    *)
      eval "lt_$var=\\\"\`\$echo \"X\$$var\" | \$Xsed -e \"\$sed_quote_subst\"\`\\\""
      ;;
    esac
  done

  cat <<__EOF__ > "${ofile}T"
#! $SHELL

# `$echo "$ofile" | sed 's%^.*/%%'` - Provide generalized library-building support services.
# Generated automatically by $PROGRAM (GNU $PACKAGE $VERSION$TIMESTAMP)
# NOTE: Changes made to this file will be lost: look at ltmain.sh.
#
# Copyright (C) 1996-2000 Free Software Foundation, Inc.
# Originally by Gordon Matzigkeit <gord@gnu.ai.mit.edu>, 1996
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# As a special exception to the GNU General Public License, if you
# distribute this file as part of a program that contains a
# configuration script generated by Autoconf, you may include it under
# the same distribution terms that you use for the rest of that program.

# Sed that helps us avoid accidentally triggering echo(1) options like -n.
Xsed="sed -e s/^X//"

# The HP-UX ksh and POSIX shell print the target directory to stdout
# if CDPATH is set.
if test "X\${CDPATH+set}" = Xset; then CDPATH=:; export CDPATH; fi

# ### BEGIN LIBTOOL CONFIG

# Libtool was configured on host `(hostname || uname -n) 2>/dev/null | sed 1q`:

# Shell to use when invoking shell scripts.
SHELL=$lt_SHELL

# Whether or not to build shared libraries.
build_libtool_libs=$enable_shared

# Whether or not to build static libraries.
build_old_libs=$enable_static

# Whether or not to add -lc for building shared libraries.
build_libtool_need_lc=$need_lc

# Whether or not to optimize for fast installation.
fast_install=$enable_fast_install

# The host system.
host_alias=$host_alias
host=$host

# An echo program that does not interpret backslashes.
echo=$lt_echo

# The archiver.
AR=$lt_AR
AR_FLAGS=$lt_AR_FLAGS

# The default C compiler.
CC=$lt_CC

# Is the compiler the GNU C compiler?
with_gcc=$GCC

# The linker used to build libraries.
LD=$lt_LD

# Whether we need hard or soft links.
LN_S=$lt_LN_S

# A BSD-compatible nm program.
NM=$lt_NM

# A symbol stripping program
STRIP=$STRIP

# Used to examine libraries when file_magic_cmd begins "file"
MAGIC_CMD=$MAGIC_CMD

# Used on cygwin: DLL creation program.
DLLTOOL="$DLLTOOL"

# Used on cygwin: object dumper.
OBJDUMP="$OBJDUMP"

# Used on cygwin: assembler.
AS="$AS"

# The name of the directory that contains temporary libtool files.
objdir=$objdir

# How to create reloadable object files.
reload_flag=$lt_reload_flag
reload_cmds=$lt_reload_cmds

# How to pass a linker flag through the compiler.
wl=$lt_wl

# Object file suffix (normally "o").
objext="$ac_objext"

# Old archive suffix (normally "a").
libext="$libext"

# Executable file suffix (normally "").
exeext="$exeext"

# Additional compiler flags for building library objects.
pic_flag=$lt_pic_flag
pic_mode=$pic_mode

# Does compiler simultaneously support -c and -o options?
compiler_c_o=$lt_compiler_c_o

# Can we write directly to a .lo ?
compiler_o_lo=$lt_compiler_o_lo

# Must we lock files when doing compilation ?
need_locks=$lt_need_locks

# Do we need the lib prefix for modules?
need_lib_prefix=$need_lib_prefix

# Do we need a version for libraries?
need_version=$need_version

# Whether dlopen is supported.
dlopen_support=$enable_dlopen

# Whether dlopen of programs is supported.
dlopen_self=$enable_dlopen_self

# Whether dlopen of statically linked programs is supported.
dlopen_self_static=$enable_dlopen_self_static

# Compiler flag to prevent dynamic linking.
link_static_flag=$lt_link_static_flag

# Compiler flag to turn off builtin functions.
no_builtin_flag=$lt_no_builtin_flag

# Compiler flag to allow reflexive dlopens.
export_dynamic_flag_spec=$lt_export_dynamic_flag_spec

# Compiler flag to generate shared objects directly from archives.
whole_archive_flag_spec=$lt_whole_archive_flag_spec

# Compiler flag to generate thread-safe objects.
thread_safe_flag_spec=$lt_thread_safe_flag_spec

# Library versioning type.
version_type=$version_type

# Format of library name prefix.
libname_spec=$lt_libname_spec

# List of archive names.  First name is the real one, the rest are links.
# The last name is the one that the linker finds with -lNAME.
library_names_spec=$lt_library_names_spec

# The coded name of the library, if different from the real name.
soname_spec=$lt_soname_spec

# Commands used to build and install an old-style archive.
RANLIB=$lt_RANLIB
old_archive_cmds=$lt_old_archive_cmds
old_postinstall_cmds=$lt_old_postinstall_cmds
old_postuninstall_cmds=$lt_old_postuninstall_cmds

# Create an old-style archive from a shared archive.
old_archive_from_new_cmds=$lt_old_archive_from_new_cmds

# Create a temporary old-style archive to link instead of a shared archive.
old_archive_from_expsyms_cmds=$lt_old_archive_from_expsyms_cmds

# Commands used to build and install a shared archive.
archive_cmds=$lt_archive_cmds
archive_expsym_cmds=$lt_archive_expsym_cmds
postinstall_cmds=$lt_postinstall_cmds
postuninstall_cmds=$lt_postuninstall_cmds

# Commands to strip libraries.
old_striplib=$lt_old_striplib
striplib=$lt_striplib

# Method to check whether dependent libraries are shared objects.
deplibs_check_method=$lt_deplibs_check_method

# Command to use when deplibs_check_method == file_magic.
file_magic_cmd=$lt_file_magic_cmd

# Flag that allows shared libraries with undefined symbols to be built.
allow_undefined_flag=$lt_allow_undefined_flag

# Flag that forces no undefined symbols.
no_undefined_flag=$lt_no_undefined_flag

# Commands used to finish a libtool library installation in a directory.
finish_cmds=$lt_finish_cmds

# Same as above, but a single script fragment to be evaled but not shown.
finish_eval=$lt_finish_eval

# Take the output of nm and produce a listing of raw symbols and C names.
global_symbol_pipe=$lt_global_symbol_pipe

# Transform the output of nm in a proper C declaration
global_symbol_to_cdecl=$lt_global_symbol_to_cdecl

# Transform the output of nm in a C name address pair
global_symbol_to_c_name_address=$lt_global_symbol_to_c_name_address

# This is the shared library runtime path variable.
runpath_var=$runpath_var

# This is the shared library path variable.
shlibpath_var=$shlibpath_var

# Is shlibpath searched before the hard-coded library search path?
shlibpath_overrides_runpath=$shlibpath_overrides_runpath

# How to hardcode a shared library path into an executable.
hardcode_action=$hardcode_action

# Whether we should hardcode library paths into libraries.
hardcode_into_libs=$hardcode_into_libs

# Flag to hardcode \$libdir into a binary during linking.
# This must work even if \$libdir does not exist.
hardcode_libdir_flag_spec=$lt_hardcode_libdir_flag_spec

# Whether we need a single -rpath flag with a separated argument.
hardcode_libdir_separator=$lt_hardcode_libdir_separator

# Set to yes if using DIR/libNAME.so during linking hardcodes DIR into the
# resulting binary.
hardcode_direct=$hardcode_direct

# Set to yes if using the -LDIR flag during linking hardcodes DIR into the
# resulting binary.
hardcode_minus_L=$hardcode_minus_L

# Set to yes if using SHLIBPATH_VAR=DIR during linking hardcodes DIR into
# the resulting binary.
hardcode_shlibpath_var=$hardcode_shlibpath_var

# Variables whose values should be saved in libtool wrapper scripts and
# restored at relink time.
variables_saved_for_relink="$variables_saved_for_relink"

# Whether libtool must link a program against all its dependency libraries.
link_all_deplibs=$link_all_deplibs

# Compile-time system search path for libraries
sys_lib_search_path_spec=$lt_sys_lib_search_path_spec

# Run-time system search path for libraries
sys_lib_dlsearch_path_spec=$lt_sys_lib_dlsearch_path_spec

# Fix the shell variable \$srcfile for the compiler.
fix_srcfile_path="$fix_srcfile_path"

# Set to yes if exported symbols are required.
always_export_symbols=$always_export_symbols

# The commands to list exported symbols.
export_symbols_cmds=$lt_export_symbols_cmds

# The commands to extract the exported symbol list from a shared archive.
extract_expsyms_cmds=$lt_extract_expsyms_cmds

# Symbols that should not be listed in the preloaded symbols.
exclude_expsyms=$lt_exclude_expsyms

# Symbols that must always be exported.
include_expsyms=$lt_include_expsyms

# ### END LIBTOOL CONFIG

__EOF__

  case $host_os in
  aix3*)
    cat <<\EOF >> "${ofile}T"

# AIX sometimes has problems with the GCC collect2 program.  For some
# reason, if we set the COLLECT_NAMES environment variable, the problems
# vanish in a puff of smoke.
if test "X${COLLECT_NAMES+set}" != Xset; then
  COLLECT_NAMES=
  export COLLECT_NAMES
fi
EOF
    ;;
  esac

  case $host_os in
  cygwin* | mingw* | pw32* | os2*)
    cat <<'EOF' >> "${ofile}T"
      # This is a source program that is used to create dlls on Windows
      # Don't remove nor modify the starting and closing comments
# /* ltdll.c starts here */
# #define WIN32_LEAN_AND_MEAN
# #include <windows.h>
# #undef WIN32_LEAN_AND_MEAN
# #include <stdio.h>
#
# #ifndef __CYGWIN__
# #  ifdef __CYGWIN32__
# #    define __CYGWIN__ __CYGWIN32__
# #  endif
# #endif
#
# #ifdef __cplusplus
# extern "C" {
# #endif
# BOOL APIENTRY DllMain (HINSTANCE hInst, DWORD reason, LPVOID reserved);
# #ifdef __cplusplus
# }
# #endif
#
# #ifdef __CYGWIN__
# #include <cygwin/cygwin_dll.h>
# DECLARE_CYGWIN_DLL( DllMain );
# #endif
# HINSTANCE __hDllInstance_base;
#
# BOOL APIENTRY
# DllMain (HINSTANCE hInst, DWORD reason, LPVOID reserved)
# {
#   __hDllInstance_base = hInst;
#   return TRUE;
# }
# /* ltdll.c ends here */
	# This is a source program that is used to create import libraries
	# on Windows for dlls which lack them. Don't remove nor modify the
	# starting and closing comments
# /* impgen.c starts here */
# /*   Copyright (C) 1999-2000 Free Software Foundation, Inc.
#
#  This file is part of GNU libtool.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#  */
#
# #include <stdio.h>		/* for printf() */
# #include <unistd.h>		/* for open(), lseek(), read() */
# #include <fcntl.h>		/* for O_RDONLY, O_BINARY */
# #include <string.h>		/* for strdup() */
#
# /* O_BINARY isn't required (or even defined sometimes) under Unix */
# #ifndef O_BINARY
# #define O_BINARY 0
# #endif
#
# static unsigned int
# pe_get16 (fd, offset)
#      int fd;
#      int offset;
# {
#   unsigned char b[2];
#   lseek (fd, offset, SEEK_SET);
#   read (fd, b, 2);
#   return b[0] + (b[1]<<8);
# }
#
# static unsigned int
# pe_get32 (fd, offset)
#     int fd;
#     int offset;
# {
#   unsigned char b[4];
#   lseek (fd, offset, SEEK_SET);
#   read (fd, b, 4);
#   return b[0] + (b[1]<<8) + (b[2]<<16) + (b[3]<<24);
# }
#
# static unsigned int
# pe_as32 (ptr)
#      void *ptr;
# {
#   unsigned char *b = ptr;
#   return b[0] + (b[1]<<8) + (b[2]<<16) + (b[3]<<24);
# }
#
# int
# main (argc, argv)
#     int argc;
#     char *argv[];
# {
#     int dll;
#     unsigned long pe_header_offset, opthdr_ofs, num_entries, i;
#     unsigned long export_rva, export_size, nsections, secptr, expptr;
#     unsigned long name_rvas, nexp;
#     unsigned char *expdata, *erva;
#     char *filename, *dll_name;
#
#     filename = argv[1];
#
#     dll = open(filename, O_RDONLY|O_BINARY);
#     if (dll < 1)
# 	return 1;
#
#     dll_name = filename;
#
#     for (i=0; filename[i]; i++)
# 	if (filename[i] == '/' || filename[i] == '\\'  || filename[i] == ':')
# 	    dll_name = filename + i +1;
#
#     pe_header_offset = pe_get32 (dll, 0x3c);
#     opthdr_ofs = pe_header_offset + 4 + 20;
#     num_entries = pe_get32 (dll, opthdr_ofs + 92);
#
#     if (num_entries < 1) /* no exports */
# 	return 1;
#
#     export_rva = pe_get32 (dll, opthdr_ofs + 96);
#     export_size = pe_get32 (dll, opthdr_ofs + 100);
#     nsections = pe_get16 (dll, pe_header_offset + 4 +2);
#     secptr = (pe_header_offset + 4 + 20 +
# 	      pe_get16 (dll, pe_header_offset + 4 + 16));
#
#     expptr = 0;
#     for (i = 0; i < nsections; i++)
#     {
# 	char sname[8];
# 	unsigned long secptr1 = secptr + 40 * i;
# 	unsigned long vaddr = pe_get32 (dll, secptr1 + 12);
# 	unsigned long vsize = pe_get32 (dll, secptr1 + 16);
# 	unsigned long fptr = pe_get32 (dll, secptr1 + 20);
# 	lseek(dll, secptr1, SEEK_SET);
# 	read(dll, sname, 8);
# 	if (vaddr <= export_rva && vaddr+vsize > export_rva)
# 	{
# 	    expptr = fptr + (export_rva - vaddr);
# 	    if (export_rva + export_size > vaddr + vsize)
# 		export_size = vsize - (export_rva - vaddr);
# 	    break;
# 	}
#     }
#
#     expdata = (unsigned char*)malloc(export_size);
#     lseek (dll, expptr, SEEK_SET);
#     read (dll, expdata, export_size);
#     erva = expdata - export_rva;
#
#     nexp = pe_as32 (expdata+24);
#     name_rvas = pe_as32 (expdata+32);
#
#     printf ("EXPORTS\n");
#     for (i = 0; i<nexp; i++)
#     {
# 	unsigned long name_rva = pe_as32 (erva+name_rvas+i*4);
# 	printf ("\t%s @ %ld ;\n", erva+name_rva, 1+ i);
#     }
#
#     return 0;
# }
# /* impgen.c ends here */

EOF
    ;;
  esac

  # We use sed instead of cat because bash on DJGPP gets confused if
  # if finds mixed CR/LF and LF-only lines.  Since sed operates in
  # text mode, it properly converts lines to CR/LF.  This bash problem
  # is reportedly fixed, but why not run on old versions too?
  sed '$q' "$ltmain" >> "${ofile}T" || (rm -f "${ofile}T"; exit 1)

  mv -f "${ofile}T" "$ofile" || \
    (rm -f "$ofile" && cp "${ofile}T" "$ofile" && rm -f "${ofile}T")
  chmod +x "$ofile"
fi

])# _LT_AC_LTCONFIG_HACK

# AC_LIBTOOL_DLOPEN - enable checks for dlopen support
AC_DEFUN([AC_LIBTOOL_DLOPEN], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])])

# AC_LIBTOOL_WIN32_DLL - declare package support for building win32 dll's
AC_DEFUN([AC_LIBTOOL_WIN32_DLL], [AC_BEFORE([$0], [AC_LIBTOOL_SETUP])])

# AC_ENABLE_SHARED - implement the --enable-shared flag
# Usage: AC_ENABLE_SHARED[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AC_ENABLE_SHARED],
[define([AC_ENABLE_SHARED_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(shared,
changequote(<<, >>)dnl
<<  --enable-shared[=PKGS]  build shared libraries [default=>>AC_ENABLE_SHARED_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case $enableval in
yes) enable_shared=yes ;;
no) enable_shared=no ;;
*)
  enable_shared=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_shared=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_shared=AC_ENABLE_SHARED_DEFAULT)dnl
])

# AC_DISABLE_SHARED - set the default shared flag to --disable-shared
AC_DEFUN([AC_DISABLE_SHARED],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_SHARED(no)])

# AC_ENABLE_STATIC - implement the --enable-static flag
# Usage: AC_ENABLE_STATIC[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AC_ENABLE_STATIC],
[define([AC_ENABLE_STATIC_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(static,
changequote(<<, >>)dnl
<<  --enable-static[=PKGS]  build static libraries [default=>>AC_ENABLE_STATIC_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case $enableval in
yes) enable_static=yes ;;
no) enable_static=no ;;
*)
  enable_static=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_static=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_static=AC_ENABLE_STATIC_DEFAULT)dnl
])

# AC_DISABLE_STATIC - set the default static flag to --disable-static
AC_DEFUN([AC_DISABLE_STATIC],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_STATIC(no)])


# AC_ENABLE_FAST_INSTALL - implement the --enable-fast-install flag
# Usage: AC_ENABLE_FAST_INSTALL[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AC_ENABLE_FAST_INSTALL],
[define([AC_ENABLE_FAST_INSTALL_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(fast-install,
changequote(<<, >>)dnl
<<  --enable-fast-install[=PKGS]  optimize for fast installation [default=>>AC_ENABLE_FAST_INSTALL_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case $enableval in
yes) enable_fast_install=yes ;;
no) enable_fast_install=no ;;
*)
  enable_fast_install=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_fast_install=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_fast_install=AC_ENABLE_FAST_INSTALL_DEFAULT)dnl
])

# AC_DISABLE_FAST_INSTALL - set the default to --disable-fast-install
AC_DEFUN([AC_DISABLE_FAST_INSTALL],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_FAST_INSTALL(no)])

# AC_LIBTOOL_PICMODE - implement the --with-pic flag
# Usage: AC_LIBTOOL_PICMODE[(MODE)]
#   Where MODE is either `yes' or `no'.  If omitted, it defaults to
#   `both'.
AC_DEFUN([AC_LIBTOOL_PICMODE],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
pic_mode=ifelse($#,1,$1,default)])


# AC_PATH_TOOL_PREFIX - find a file program which can recognise shared library
AC_DEFUN([AC_PATH_TOOL_PREFIX],
[AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(lt_cv_path_MAGIC_CMD,
[case $MAGIC_CMD in
  /*)
  lt_cv_path_MAGIC_CMD="$MAGIC_CMD" # Let the user override the test with a path.
  ;;
  ?:/*)
  lt_cv_path_MAGIC_CMD="$MAGIC_CMD" # Let the user override the test with a dos path.
  ;;
  *)
  ac_save_MAGIC_CMD="$MAGIC_CMD"
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS=":"
dnl $ac_dummy forces splitting on constant user-supplied paths.
dnl POSIX.2 word splitting is done only on the output of word expansions,
dnl not every word.  This closes a longstanding sh security hole.
  ac_dummy="ifelse([$2], , $PATH, [$2])"
  for ac_dir in $ac_dummy; do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$1; then
      lt_cv_path_MAGIC_CMD="$ac_dir/$1"
      if test -n "$file_magic_test_file"; then
	case $deplibs_check_method in
	"file_magic "*)
	  file_magic_regex="`expr \"$deplibs_check_method\" : \"file_magic \(.*\)\"`"
	  MAGIC_CMD="$lt_cv_path_MAGIC_CMD"
	  if eval $file_magic_cmd \$file_magic_test_file 2> /dev/null |
	    egrep "$file_magic_regex" > /dev/null; then
	    :
	  else
	    cat <<EOF 1>&2

*** Warning: the command libtool uses to detect shared libraries,
*** $file_magic_cmd, produces output that libtool cannot recognize.
*** The result is that libtool may fail to recognize shared libraries
*** as such.  This will affect the creation of libtool libraries that
*** depend on shared libraries, but programs linked with such libtool
*** libraries will work regardless of this problem.  Nevertheless, you
*** may want to report the problem to your system manager and/or to
*** bug-libtool@gnu.org

EOF
	  fi ;;
	esac
      fi
      break
    fi
  done
  IFS="$ac_save_ifs"
  MAGIC_CMD="$ac_save_MAGIC_CMD"
  ;;
esac])
MAGIC_CMD="$lt_cv_path_MAGIC_CMD"
if test -n "$MAGIC_CMD"; then
  AC_MSG_RESULT($MAGIC_CMD)
else
  AC_MSG_RESULT(no)
fi
])


# AC_PATH_MAGIC - find a file program which can recognise a shared library
AC_DEFUN([AC_PATH_MAGIC],
[AC_REQUIRE([AC_CHECK_TOOL_PREFIX])dnl
AC_PATH_TOOL_PREFIX(${ac_tool_prefix}file, /usr/bin:$PATH)
if test -z "$lt_cv_path_MAGIC_CMD"; then
  if test -n "$ac_tool_prefix"; then
    AC_PATH_TOOL_PREFIX(file, /usr/bin:$PATH)
  else
    MAGIC_CMD=:
  fi
fi
])


# AC_PROG_LD - find the path to the GNU or non-GNU linker
AC_DEFUN([AC_PROG_LD],
[AC_ARG_WITH(gnu-ld,
[  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]],
test "$withval" = no || with_gnu_ld=yes, with_gnu_ld=no)
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_CANONICAL_BUILD])dnl
AC_REQUIRE([_LT_AC_LIBTOOL_SYS_PATH_SEPARATOR])dnl
ac_prog=ld
if test "$GCC" = yes; then
  # Check if gcc -print-prog-name=ld gives a path.
  AC_MSG_CHECKING([for ld used by GCC])
  case $host in
  *-*-mingw*)
    # gcc leaves a trailing carriage return which upsets mingw
    ac_prog=`($CC -print-prog-name=ld) 2>&5 | tr -d '\015'` ;;
  *)
    ac_prog=`($CC -print-prog-name=ld) 2>&5` ;;
  esac
  case $ac_prog in
    # Accept absolute paths.
    [[\\/]]* | [[A-Za-z]]:[[\\/]]*)
      re_direlt='/[[^/]][[^/]]*/\.\./'
      # Canonicalize the path of ld
      ac_prog=`echo $ac_prog| sed 's%\\\\%/%g'`
      while echo $ac_prog | grep "$re_direlt" > /dev/null 2>&1; do
	ac_prog=`echo $ac_prog| sed "s%$re_direlt%/%"`
      done
      test -z "$LD" && LD="$ac_prog"
      ;;
  "")
    # If it fails, then pretend we aren't using GCC.
    ac_prog=ld
    ;;
  *)
    # If it is relative, then search for the first ld in PATH.
    with_gnu_ld=unknown
    ;;
  esac
elif test "$with_gnu_ld" = yes; then
  AC_MSG_CHECKING([for GNU ld])
else
  AC_MSG_CHECKING([for non-GNU ld])
fi
AC_CACHE_VAL(lt_cv_path_LD,
[if test -z "$LD"; then
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=$PATH_SEPARATOR
  for ac_dir in $PATH; do
    test -z "$ac_dir" && ac_dir=.
    if test -f "$ac_dir/$ac_prog" || test -f "$ac_dir/$ac_prog$ac_exeext"; then
      lt_cv_path_LD="$ac_dir/$ac_prog"
      # Check to see if the program is GNU ld.  I'd rather use --version,
      # but apparently some GNU ld's only accept -v.
      # Break only if it was the GNU/non-GNU ld that we prefer.
      if "$lt_cv_path_LD" -v 2>&1 < /dev/null | egrep '(GNU|with BFD)' > /dev/null; then
	test "$with_gnu_ld" != no && break
      else
	test "$with_gnu_ld" != yes && break
      fi
    fi
  done
  IFS="$ac_save_ifs"
else
  lt_cv_path_LD="$LD" # Let the user override the test with a path.
fi])
LD="$lt_cv_path_LD"
if test -n "$LD"; then
  AC_MSG_RESULT($LD)
else
  AC_MSG_RESULT(no)
fi
test -z "$LD" && AC_MSG_ERROR([no acceptable ld found in \$PATH])
AC_PROG_LD_GNU
])

# AC_PROG_LD_GNU -
AC_DEFUN([AC_PROG_LD_GNU],
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], lt_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
if $LD -v 2>&1 </dev/null | egrep '(GNU|with BFD)' 1>&5; then
  lt_cv_prog_gnu_ld=yes
else
  lt_cv_prog_gnu_ld=no
fi])
with_gnu_ld=$lt_cv_prog_gnu_ld
])

# AC_PROG_LD_RELOAD_FLAG - find reload flag for linker
#   -- PORTME Some linkers may need a different reload flag.
AC_DEFUN([AC_PROG_LD_RELOAD_FLAG],
[AC_CACHE_CHECK([for $LD option to reload object files], lt_cv_ld_reload_flag,
[lt_cv_ld_reload_flag='-r'])
reload_flag=$lt_cv_ld_reload_flag
test -n "$reload_flag" && reload_flag=" $reload_flag"
])

# AC_DEPLIBS_CHECK_METHOD - how to check for library dependencies
#  -- PORTME fill in with the dynamic library characteristics
AC_DEFUN([AC_DEPLIBS_CHECK_METHOD],
[AC_CACHE_CHECK([how to recognise dependant libraries],
lt_cv_deplibs_check_method,
[lt_cv_file_magic_cmd='$MAGIC_CMD'
lt_cv_file_magic_test_file=
lt_cv_deplibs_check_method='unknown'
# Need to set the preceding variable on all platforms that support
# interlibrary dependencies.
# 'none' -- dependencies not supported.
# `unknown' -- same as none, but documents that we really don't know.
# 'pass_all' -- all dependencies passed with no checks.
# 'test_compile' -- check by making test program.
# 'file_magic [[regex]]' -- check by looking for files in library path
# which responds to the $file_magic_cmd with a given egrep regex.
# If you have `file' or equivalent on your system and you're not sure
# whether `pass_all' will *always* work, you probably want this one.

case $host_os in
aix4* | aix5*)
  lt_cv_deplibs_check_method=pass_all
  ;;

beos*)
  lt_cv_deplibs_check_method=pass_all
  ;;

bsdi4*)
  lt_cv_deplibs_check_method='file_magic ELF [[0-9]][[0-9]]*-bit [[ML]]SB (shared object|dynamic lib)'
  lt_cv_file_magic_cmd='/usr/bin/file -L'
  lt_cv_file_magic_test_file=/shlib/libc.so
  ;;

cygwin* | mingw* | pw32*)
  lt_cv_deplibs_check_method='file_magic file format pei*-i386(.*architecture: i386)?'
  lt_cv_file_magic_cmd='$OBJDUMP -f'
  ;;

darwin* | rhapsody*)
  lt_cv_deplibs_check_method='file_magic Mach-O dynamically linked shared library'
  lt_cv_file_magic_cmd='/usr/bin/file -L'
  case "$host_os" in
  rhapsody* | darwin1.[[012]])
    lt_cv_file_magic_test_file=`echo /System/Library/Frameworks/System.framework/Versions/*/System | head -1`
    ;;
  *) # Darwin 1.3 on
    lt_cv_file_magic_test_file='/usr/lib/libSystem.dylib'
    ;;
  esac
  ;;

freebsd*)
  if echo __ELF__ | $CC -E - | grep __ELF__ > /dev/null; then
    case $host_cpu in
    i*86 )
      # Not sure whether the presence of OpenBSD here was a mistake.
      # Let's accept both of them until this is cleared up.
      lt_cv_deplibs_check_method='file_magic (FreeBSD|OpenBSD)/i[[3-9]]86 (compact )?demand paged shared library'
      lt_cv_file_magic_cmd=/usr/bin/file
      lt_cv_file_magic_test_file=`echo /usr/lib/libc.so.*`
      ;;
    esac
  else
    lt_cv_deplibs_check_method=pass_all
  fi
  ;;

gnu*)
  lt_cv_deplibs_check_method=pass_all
  ;;

hpux10.20*|hpux11*)
  lt_cv_deplibs_check_method='file_magic (s[[0-9]][[0-9]][[0-9]]|PA-RISC[[0-9]].[[0-9]]) shared library'
  lt_cv_file_magic_cmd=/usr/bin/file
  lt_cv_file_magic_test_file=/usr/lib/libc.sl
  ;;

irix5* | irix6*)
  case $host_os in
  irix5*)
    # this will be overridden with pass_all, but let us keep it just in case
    lt_cv_deplibs_check_method="file_magic ELF 32-bit MSB dynamic lib MIPS - version 1"
    ;;
  *)
    case $LD in
    *-32|*"-32 ") libmagic=32-bit;;
    *-n32|*"-n32 ") libmagic=N32;;
    *-64|*"-64 ") libmagic=64-bit;;
    *) libmagic=never-match;;
    esac
    # this will be overridden with pass_all, but let us keep it just in case
    lt_cv_deplibs_check_method="file_magic ELF ${libmagic} MSB mips-[[1234]] dynamic lib MIPS - version 1"
    ;;
  esac
  lt_cv_file_magic_test_file=`echo /lib${libsuff}/libc.so*`
  lt_cv_deplibs_check_method=pass_all
  ;;

# This must be Linux ELF.
linux-gnu*)
  case $host_cpu in
  alpha* | hppa* | i*86 | powerpc* | sparc* | ia64* | s390* )
    lt_cv_deplibs_check_method=pass_all ;;
  *)
    # glibc up to 2.1.1 does not perform some relocations on ARM
    lt_cv_deplibs_check_method='file_magic ELF [[0-9]][[0-9]]*-bit [[LM]]SB (shared object|dynamic lib )' ;;
  esac
  lt_cv_file_magic_test_file=`echo /lib/libc.so* /lib/libc-*.so`
  ;;

netbsd*)
  if echo __ELF__ | $CC -E - | grep __ELF__ > /dev/null; then
    lt_cv_deplibs_check_method='match_pattern /lib[[^/\.]]+\.so\.[[0-9]]+\.[[0-9]]+$'
  else
    lt_cv_deplibs_check_method='match_pattern /lib[[^/\.]]+\.so$'
  fi
  ;;

newos6*)
  lt_cv_deplibs_check_method='file_magic ELF [[0-9]][[0-9]]*-bit [[ML]]SB (executable|dynamic lib)'
  lt_cv_file_magic_cmd=/usr/bin/file
  lt_cv_file_magic_test_file=/usr/lib/libnls.so
  ;;

openbsd*)
  lt_cv_file_magic_cmd=/usr/bin/file
  lt_cv_file_magic_test_file=`echo /usr/lib/libc.so.*`
  if test -z "`echo __ELF__ | $CC -E - | grep __ELF__`" || test "$host_os-$host_cpu" = "openbsd2.8-powerpc"; then
    lt_cv_deplibs_check_method='file_magic ELF [[0-9]][[0-9]]*-bit [[LM]]SB shared object'
  else
    lt_cv_deplibs_check_method='file_magic OpenBSD.* shared library'
  fi
  ;;

osf3* | osf4* | osf5*)
  # this will be overridden with pass_all, but let us keep it just in case
  lt_cv_deplibs_check_method='file_magic COFF format alpha shared library'
  lt_cv_file_magic_test_file=/shlib/libc.so
  lt_cv_deplibs_check_method=pass_all
  ;;

sco3.2v5*)
  lt_cv_deplibs_check_method=pass_all
  ;;

solaris*)
  lt_cv_deplibs_check_method=pass_all
  lt_cv_file_magic_test_file=/lib/libc.so
  ;;

sysv5uw[[78]]* | sysv4*uw2*)
  lt_cv_deplibs_check_method=pass_all
  ;;

sysv4 | sysv4.2uw2* | sysv4.3* | sysv5*)
  case $host_vendor in
  motorola)
    lt_cv_deplibs_check_method='file_magic ELF [[0-9]][[0-9]]*-bit [[ML]]SB (shared object|dynamic lib) M[[0-9]][[0-9]]* Version [[0-9]]'
    lt_cv_file_magic_test_file=`echo /usr/lib/libc.so*`
    ;;
  ncr)
    lt_cv_deplibs_check_method=pass_all
    ;;
  sequent)
    lt_cv_file_magic_cmd='/bin/file'
    lt_cv_deplibs_check_method='file_magic ELF [[0-9]][[0-9]]*-bit [[LM]]SB (shared object|dynamic lib )'
    ;;
  sni)
    lt_cv_file_magic_cmd='/bin/file'
    lt_cv_deplibs_check_method="file_magic ELF [[0-9]][[0-9]]*-bit [[LM]]SB dynamic lib"
    lt_cv_file_magic_test_file=/lib/libc.so
    ;;
  esac
  ;;
esac
])
file_magic_cmd=$lt_cv_file_magic_cmd
deplibs_check_method=$lt_cv_deplibs_check_method
])


# AC_PROG_NM - find the path to a BSD-compatible name lister
AC_DEFUN([AC_PROG_NM],
[AC_REQUIRE([_LT_AC_LIBTOOL_SYS_PATH_SEPARATOR])dnl
AC_MSG_CHECKING([for BSD-compatible nm])
AC_CACHE_VAL(lt_cv_path_NM,
[if test -n "$NM"; then
  # Let the user override the test.
  lt_cv_path_NM="$NM"
else
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=$PATH_SEPARATOR
  for ac_dir in $PATH /usr/ccs/bin /usr/ucb /bin; do
    test -z "$ac_dir" && ac_dir=.
    tmp_nm=$ac_dir/${ac_tool_prefix}nm
    if test -f $tmp_nm || test -f $tmp_nm$ac_exeext ; then
      # Check to see if the nm accepts a BSD-compat flag.
      # Adding the `sed 1q' prevents false positives on HP-UX, which says:
      #   nm: unknown option "B" ignored
      # Tru64's nm complains that /dev/null is an invalid object file
      if ($tmp_nm -B /dev/null 2>&1 | sed '1q'; exit 0) | egrep '(/dev/null|Invalid file or object type)' >/dev/null; then
	lt_cv_path_NM="$tmp_nm -B"
	break
      elif ($tmp_nm -p /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
	lt_cv_path_NM="$tmp_nm -p"
	break
      else
	lt_cv_path_NM=${lt_cv_path_NM="$tmp_nm"} # keep the first match, but
	continue # so that we can try to find one that supports BSD flags
      fi
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$lt_cv_path_NM" && lt_cv_path_NM=nm
fi])
NM="$lt_cv_path_NM"
AC_MSG_RESULT([$NM])
])

# AC_CHECK_LIBM - check for math library
AC_DEFUN([AC_CHECK_LIBM],
[AC_REQUIRE([AC_CANONICAL_HOST])dnl
LIBM=
case $host in
*-*-beos* | *-*-cygwin* | *-*-pw32*)
  # These system don't have libm
  ;;
*-ncr-sysv4.3*)
  AC_CHECK_LIB(mw, _mwvalidcheckl, LIBM="-lmw")
  AC_CHECK_LIB(m, main, LIBM="$LIBM -lm")
  ;;
*)
  AC_CHECK_LIB(m, main, LIBM="-lm")
  ;;
esac
])

# AC_LIBLTDL_CONVENIENCE[(dir)] - sets LIBLTDL to the link flags for
# the libltdl convenience library and INCLTDL to the include flags for
# the libltdl header and adds --enable-ltdl-convenience to the
# configure arguments.  Note that LIBLTDL and INCLTDL are not
# AC_SUBSTed, nor is AC_CONFIG_SUBDIRS called.  If DIR is not
# provided, it is assumed to be `libltdl'.  LIBLTDL will be prefixed
# with '${top_builddir}/' and INCLTDL will be prefixed with
# '${top_srcdir}/' (note the single quotes!).  If your package is not
# flat and you're not using automake, define top_builddir and
# top_srcdir appropriately in the Makefiles.
AC_DEFUN([AC_LIBLTDL_CONVENIENCE],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  case $enable_ltdl_convenience in
  no) AC_MSG_ERROR([this package needs a convenience libltdl]) ;;
  "") enable_ltdl_convenience=yes
      ac_configure_args="$ac_configure_args --enable-ltdl-convenience" ;;
  esac
  LIBLTDL='${top_builddir}/'ifelse($#,1,[$1],['libltdl'])/libltdlc.la
  INCLTDL='-I${top_srcdir}/'ifelse($#,1,[$1],['libltdl'])
])

# AC_LIBLTDL_INSTALLABLE[(dir)] - sets LIBLTDL to the link flags for
# the libltdl installable library and INCLTDL to the include flags for
# the libltdl header and adds --enable-ltdl-install to the configure
# arguments.  Note that LIBLTDL and INCLTDL are not AC_SUBSTed, nor is
# AC_CONFIG_SUBDIRS called.  If DIR is not provided and an installed
# libltdl is not found, it is assumed to be `libltdl'.  LIBLTDL will
# be prefixed with '${top_builddir}/' and INCLTDL will be prefixed
# with '${top_srcdir}/' (note the single quotes!).  If your package is
# not flat and you're not using automake, define top_builddir and
# top_srcdir appropriately in the Makefiles.
# In the future, this macro may have to be called after AC_PROG_LIBTOOL.
AC_DEFUN([AC_LIBLTDL_INSTALLABLE],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  AC_CHECK_LIB(ltdl, main,
  [test x"$enable_ltdl_install" != xyes && enable_ltdl_install=no],
  [if test x"$enable_ltdl_install" = xno; then
     AC_MSG_WARN([libltdl not installed, but installation disabled])
   else
     enable_ltdl_install=yes
   fi
  ])
  if test x"$enable_ltdl_install" = x"yes"; then
    ac_configure_args="$ac_configure_args --enable-ltdl-install"
    LIBLTDL='${top_builddir}/'ifelse($#,1,[$1],['libltdl'])/libltdl.la
    INCLTDL='-I${top_srcdir}/'ifelse($#,1,[$1],['libltdl'])
  else
    ac_configure_args="$ac_configure_args --enable-ltdl-install=no"
    LIBLTDL="-lltdl"
    INCLTDL=
  fi
])

# old names
AC_DEFUN([AM_PROG_LIBTOOL],   [AC_PROG_LIBTOOL])
AC_DEFUN([AM_ENABLE_SHARED],  [AC_ENABLE_SHARED($@)])
AC_DEFUN([AM_ENABLE_STATIC],  [AC_ENABLE_STATIC($@)])
AC_DEFUN([AM_DISABLE_SHARED], [AC_DISABLE_SHARED($@)])
AC_DEFUN([AM_DISABLE_STATIC], [AC_DISABLE_STATIC($@)])
AC_DEFUN([AM_PROG_LD],        [AC_PROG_LD])
AC_DEFUN([AM_PROG_NM],        [AC_PROG_NM])

# This is just to silence aclocal about the macro not being used
ifelse([AC_DISABLE_FAST_INSTALL])


# Copyright 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

# serial 3

AC_PREREQ(2.50)

# AM_PROG_LEX
# -----------
# Autoconf leaves LEX=: if lex or flex can't be found.  Change that to a
# "missing" invocation, for better error output.
AC_DEFUN([AM_PROG_LEX],
[AC_REQUIRE([AM_MISSING_HAS_RUN])dnl
AC_REQUIRE([AC_PROG_LEX])dnl
if test "$LEX" = :; then
  LEX=${am_missing_run}flex
fi])

