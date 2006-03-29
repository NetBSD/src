dnl acinclude.m4 -- Extra macros needed for GNU make.
dnl
dnl Automake will incorporate this into its generated aclocal.m4.

dnl ---------------------------------------------------------------------------
dnl Got this from the lynx 2.8 distribution.
dnl by T.E.Dickey <dickey@clark.net>
dnl and Jim Spath <jspath@mail.bcpl.lib.md.us>
dnl and Philippe De Muyter <phdm@macqel.be>
dnl
dnl Created: 1997/1/28
dnl Updated: 1997/12/23
dnl ---------------------------------------------------------------------------
dnl After checking for functions in the default $LIBS, make a further check
dnl for the functions that are netlib-related (these aren't always in the
dnl libc, etc., and have to be handled specially because there are conflicting
dnl and broken implementations.
dnl Common library requirements (in order):
dnl	-lresolv -lsocket -lnsl
dnl	-lnsl -lsocket
dnl	-lsocket
dnl	-lbsd
AC_DEFUN([CF_NETLIBS],[
cf_test_netlibs=no
AC_MSG_CHECKING(for network libraries)
AC_CACHE_VAL(cf_cv_netlibs,[
AC_MSG_RESULT(working...)
cf_cv_netlibs=""
cf_test_netlibs=yes
AC_CHECK_FUNCS(gethostname,,[
	CF_RECHECK_FUNC(gethostname,nsl,cf_cv_netlibs,[
		CF_RECHECK_FUNC(gethostname,socket,cf_cv_netlibs)])])
#
# FIXME:  sequent needs this library (i.e., -lsocket -linet -lnsl), but
# I don't know the entrypoints - 97/7/22 TD
AC_CHECK_LIB(inet,main,cf_cv_netlibs="-linet $cf_cv_netlibs")
#
if test "$ac_cv_func_lsocket" != no ; then
AC_CHECK_FUNCS(socket,,[
	CF_RECHECK_FUNC(socket,socket,cf_cv_netlibs,[
		CF_RECHECK_FUNC(socket,bsd,cf_cv_netlibs)])])
fi
#
AC_CHECK_FUNCS(gethostbyname,,[
	CF_RECHECK_FUNC(gethostbyname,nsl,cf_cv_netlibs)])
#
AC_CHECK_FUNCS(strcasecmp,,[
	CF_RECHECK_FUNC(strcasecmp,resolv,cf_cv_netlibs)])
])
LIBS="$LIBS $cf_cv_netlibs"
test $cf_test_netlibs = no && echo "$cf_cv_netlibs" >&AC_FD_MSG
])dnl
dnl ---------------------------------------------------------------------------
dnl Re-check on a function to see if we can pick it up by adding a library.
dnl	$1 = function to check
dnl	$2 = library to check in
dnl	$3 = environment to update (e.g., $LIBS)
dnl	$4 = what to do if this fails
dnl
dnl This uses 'unset' if the shell happens to support it, but leaves the
dnl configuration variable set to 'unknown' if not.  This is a little better
dnl than the normal autoconf test, which gives misleading results if a test
dnl for the function is made (e.g., with AC_CHECK_FUNC) after this macro is
dnl used (autoconf does not distinguish between a null token and one that is
dnl set to 'no').
AC_DEFUN([CF_RECHECK_FUNC],[
AC_CHECK_LIB($2,$1,[
	CF_UPPER(cf_tr_func,$1)
	AC_DEFINE_UNQUOTED(HAVE_$cf_tr_func,1,[Define if you have function $1])
	ac_cv_func_$1=yes
	$3="-l$2 [$]$3"],[
	ac_cv_func_$1=unknown
	unset ac_cv_func_$1 2>/dev/null
	$4],
	[[$]$3])
])dnl
dnl ---------------------------------------------------------------------------
dnl Make an uppercase version of a variable
dnl $1=uppercase($2)
AC_DEFUN([CF_UPPER],
[
changequote(,)dnl
$1=`echo $2 | tr '[a-z]' '[A-Z]'`
changequote([,])dnl
])dnl


dnl ---------------------------------------------------------------------------
dnl From Paul Eggert <eggert@twinsun.com>

AC_DEFUN(AC_STRUCT_ST_MTIM_NSEC,
 [AC_CACHE_CHECK([for nanoseconds field of struct stat.st_mtim],
   ac_cv_struct_st_mtim_nsec,
   [ac_save_CPPFLAGS="$CPPFLAGS"
    ac_cv_struct_st_mtim_nsec=no
    # tv_nsec -- the usual case
    # _tv_nsec -- Solaris 2.6, if
    #	(defined _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED == 1
    #	 && !defined __EXTENSIONS__)
    # st__tim.tv_nsec -- UnixWare 2.1.2
    for ac_val in tv_nsec _tv_nsec st__tim.tv_nsec; do
      CPPFLAGS="$ac_save_CPPFLAGS -DST_MTIM_NSEC=$ac_val"
      AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>], [struct stat s; s.st_mtim.ST_MTIM_NSEC;],
        [ac_cv_struct_st_mtim_nsec=$ac_val; break])
    done
    CPPFLAGS="$ac_save_CPPFLAGS"])

  if test $ac_cv_struct_st_mtim_nsec != no; then
    AC_DEFINE_UNQUOTED(ST_MTIM_NSEC, $ac_cv_struct_st_mtim_nsec, [Define if 'struct stat' contains a nanoseconds field])
  fi
 ]
)


dnl ---------------------------------------------------------------------------
dnl This will be in the next version of autoconf; take this out then!

# make_FUNC_SETVBUF_REVERSED
# ------------------------
AC_DEFUN([make_FUNC_SETVBUF_REVERSED],
[AC_REQUIRE([AC_C_PROTOTYPES])dnl
AC_CACHE_CHECK(whether setvbuf arguments are reversed,
  ac_cv_func_setvbuf_reversed,
  [ac_cv_func_setvbuf_reversed=no
   AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
	[[#include <stdio.h>
#	  if PROTOTYPES
	   int (setvbuf) (FILE *, int, char *, size_t);
#	  endif]],
	[[char buf; return setvbuf (stdout, _IOLBF, &buf, 1);]])],
     [AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
	   [[#include <stdio.h>
#	     if PROTOTYPES
	      int (setvbuf) (FILE *, int, char *, size_t);
#	     endif]],
	   [[char buf; return setvbuf (stdout, &buf, _IOLBF, 1);]])],
	[# It compiles and links either way, so it must not be declared
	 # with a prototype and most likely this is a K&R C compiler.
	 # Try running it.
	 AC_RUN_IFELSE(
	   [AC_LANG_PROGRAM(
	      [[#include <stdio.h>]],
	      [[/* This call has the arguments reversed.
		   A reversed system may check and see that the address of buf
		   is not _IOLBF, _IONBF, or _IOFBF, and return nonzero.  */
		char buf;
		if (setvbuf (stdout, _IOLBF, &buf, 1) != 0)
		  exit (1);
		putchar ('\r');
		exit (0); /* Non-reversed systems SEGV here.  */]])],
	   ac_cv_func_setvbuf_reversed=yes,
	   rm -f core core.* *.core,
	   [[: # Assume setvbuf is not reversed when cross-compiling.]])]
	ac_cv_func_setvbuf_reversed=yes)])])
if test $ac_cv_func_setvbuf_reversed = yes; then
  AC_DEFINE(SETVBUF_REVERSED, 1,
            [Define to 1 if the `setvbuf' function takes the buffering type as
             its second argument and the buffer pointer as the third, as on
             System V before release 3.])
fi
])# make_FUNC_SETVBUF_REVERSED
