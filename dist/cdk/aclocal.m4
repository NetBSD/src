dnl macros used for CDK configure script
dnl -- T.Dickey <dickey@clark.net>
dnl ---------------------------------------------------------------------------
dnl ---------------------------------------------------------------------------
dnl Conditionally generate script according to whether we're using the release
dnl version of autoconf, or a patched version (using the ternary component as
dnl the patch-version).
define(CF_AC_PREREQ,
[CF_PREREQ_COMPARE(
AC_PREREQ_CANON(AC_PREREQ_SPLIT(AC_ACVERSION)),
AC_PREREQ_CANON(AC_PREREQ_SPLIT([$1])), [$1], [$2], [$3])])dnl
dnl ---------------------------------------------------------------------------
dnl Add an include-directory to $CPPFLAGS.  Don't add /usr/include, since it's
dnl redundant.  We don't normally need to add -I/usr/local/include for gcc,
dnl but old versions (and some misinstalled ones) need that.
AC_DEFUN([CF_ADD_INCDIR],
[
for cf_add_incdir in $1
do
	while true
	do
		case $cf_add_incdir in
		/usr/include) # (vi
			;;
		*) # (vi
			CPPFLAGS="$CPPFLAGS -I$cf_add_incdir"
			;;
		esac
		cf_top_incdir=`echo $cf_add_incdir | sed -e 's:/include/.*$:/include:'`
		test "$cf_top_incdir" = "$cf_add_incdir" && break
		cf_add_incdir="$cf_top_incdir"
	done
done
])dnl
dnl ---------------------------------------------------------------------------
dnl Allow user to disable a normally-on option.
AC_DEFUN([CF_ARG_DISABLE],
[CF_ARG_OPTION($1,[$2],[$3],[$4],yes)])dnl
dnl ---------------------------------------------------------------------------
dnl Allow user to enable a normally-off option.
AC_DEFUN([CF_ARG_ENABLE],
[CF_ARG_OPTION($1,[$2],[$3],[$4],no)])dnl
dnl ---------------------------------------------------------------------------
dnl Restricted form of AC_ARG_ENABLE that ensures user doesn't give bogus
dnl values.
dnl
dnl Parameters:
dnl $1 = option name
dnl $2 = help-string
dnl $3 = action to perform if option is not default
dnl $4 = action if perform if option is default
dnl $5 = default option value (either 'yes' or 'no')
AC_DEFUN([CF_ARG_OPTION],
[AC_ARG_ENABLE($1,[$2],[test "$enableval" != ifelse($5,no,yes,no) && enableval=ifelse($5,no,no,yes)
  if test "$enableval" != "$5" ; then
ifelse($3,,[    :]dnl
,[    $3]) ifelse($4,,,[
  else
    $4])
  fi],[enableval=$5 ifelse($4,,,[
  $4
])dnl
  ])])dnl
dnl ---------------------------------------------------------------------------
dnl Check if we're accidentally using a cache from a different machine.
dnl Derive the system name, as a check for reusing the autoconf cache.
dnl
dnl If we've packaged config.guess and config.sub, run that (since it does a
dnl better job than uname).
AC_DEFUN([CF_CHECK_CACHE],
[
if test -f $srcdir/config.guess ; then
	AC_CANONICAL_HOST
	system_name="$host_os"
else
	system_name="`(uname -s -r) 2>/dev/null`"
	if test -z "$system_name" ; then
		system_name="`(hostname) 2>/dev/null`"
	fi
fi

test -z "$system_name" && system_name="$cf_cv_system_name"
test -n "$cf_cv_system_name" && AC_MSG_RESULT("Configuring for $cf_cv_system_name")

if test ".$system_name" != ".$cf_cv_system_name" ; then
	AC_MSG_RESULT(Cached system name ($system_name) does not agree with actual ($cf_cv_system_name))
	AC_ERROR("Please remove config.cache and try again.")
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Test if curses defines 'chtype' (usually a 'long' type for SysV curses).
AC_DEFUN([CF_CURSES_CHTYPE],
[
AC_CACHE_CHECK(for chtype typedef,cf_cv_chtype_decl,[
	AC_TRY_COMPILE([#include <curses.h>],
		[chtype foo],
		[cf_cv_chtype_decl=yes],
		[cf_cv_chtype_decl=no])])
if test $cf_cv_chtype_decl = yes ; then
	AC_DEFINE(HAVE_TYPE_CHTYPE)
	AC_CACHE_CHECK(if chtype is scalar or struct,cf_cv_chtype_type,[
		AC_TRY_COMPILE([#include <curses.h>],
			[chtype foo; long x = foo],
			[cf_cv_chtype_type=scalar],
			[cf_cv_chtype_type=struct])])
	if test $cf_cv_chtype_type = scalar ; then
		AC_DEFINE(TYPE_CHTYPE_IS_SCALAR)
	fi
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Curses-functions are a little complicated, since a lot of them are macros.
AC_DEFUN([CF_CURSES_FUNCS],
[
AC_REQUIRE([CF_XOPEN_CURSES])
for cf_func in $1
do
	CF_UPPER(cf_tr_func,$cf_func)
	AC_MSG_CHECKING(for ${cf_func})
	CF_MSG_LOG(${cf_func})
	AC_CACHE_VAL(cf_cv_func_$cf_func,[
		eval cf_result='$ac_cv_func_'$cf_func
		if test ".$cf_result" != ".no"; then
			AC_TRY_LINK([
#ifdef HAVE_XCURSES
#include <xcurses.h>
char * XCursesProgramName = "test";
#else
#include <curses.h>
#ifdef HAVE_TERM_H
#include <term.h>
#endif
#endif],
			[
#ifndef ${cf_func}
long foo = (long)(&${cf_func});
#endif
			],
			[cf_result=yes],
			[cf_result=no])
		fi
		eval 'cf_cv_func_'$cf_func'=$cf_result'
	])
	# use the computed/retrieved cache-value:
	eval 'cf_result=$cf_cv_func_'$cf_func
	AC_MSG_RESULT($cf_result)
	if test $cf_result != no; then
		AC_DEFINE_UNQUOTED(HAVE_${cf_tr_func})
	fi
done
])dnl
dnl ---------------------------------------------------------------------------
dnl Look for the curses libraries.  Older curses implementations may require
dnl termcap/termlib to be linked as well.
AC_DEFUN([CF_CURSES_LIBS],[
AC_CHECK_FUNC(initscr,,[
case $host_os in #(vi
freebsd*) #(vi
	AC_CHECK_LIB(mytinfo,tgoto,[LIBS="-lmytinfo $LIBS"])
	;;
hpux10.*|hpux11.*)
	AC_CHECK_LIB(cur_colr,initscr,[
		LIBS="-lcur_colr $LIBS"
		CFLAGS="-I/usr/include/curses_colr $CFLAGS"
		ac_cv_func_initscr=yes
		],[
	AC_CHECK_LIB(Hcurses,initscr,[
		# HP's header uses __HP_CURSES, but user claims _HP_CURSES.
		LIBS="-lHcurses $LIBS"
		CFLAGS="-D__HP_CURSES -D_HP_CURSES $CFLAGS"
		ac_cv_func_initscr=yes
		])])
	;;
linux*) # Suse Linux does not follow /usr/lib convention
	LIBS="$LIBS -L/lib"
	;;
esac

if test ".$With5lib" != ".no" ; then
if test -d /usr/5lib ; then
	# SunOS 3.x or 4.x
	CPPFLAGS="$CPPFLAGS -I/usr/5include"
	LIBS="$LIBS -L/usr/5lib"
fi
fi

if test ".$ac_cv_func_initscr" != .yes ; then
	cf_save_LIBS="$LIBS"
	cf_term_lib=""
	cf_curs_lib=""

	# Check for library containing tgoto.  Do this before curses library
	# because it may be needed to link the test-case for initscr.
	AC_CHECK_FUNC(tgoto,[cf_term_lib=predefined],[
		for cf_term_lib in termcap termlib unknown
		do
			AC_CHECK_LIB($cf_term_lib,tgoto,[break])
		done
	])

	# Check for library containing initscr
	test "$cf_term_lib" != predefined && test "$cf_term_lib" != unknown && LIBS="-l$cf_term_lib $cf_save_LIBS"
	for cf_curs_lib in cursesX curses ncurses xcurses jcurses unknown
	do
		AC_CHECK_LIB($cf_curs_lib,initscr,[break])
	done
	test $cf_curs_lib = unknown && AC_ERROR(no curses library found)

	LIBS="-l$cf_curs_lib $cf_save_LIBS"
	if test "$cf_term_lib" = unknown ; then
		AC_MSG_CHECKING(if we can link with $cf_curs_lib library)
		AC_TRY_LINK([#include <${cf_cv_ncurses_header-curses.h}>],
			[initscr()],
			[cf_result=yes],
			[cf_result=no])
		AC_MSG_RESULT($cf_result)
		test $cf_result = no && AC_ERROR(Cannot link curses library)
	elif test "$cf_term_lib" != predefined ; then
		AC_MSG_CHECKING(if we need both $cf_curs_lib and $cf_term_lib libraries)
		AC_TRY_LINK([#include <${cf_cv_ncurses_header-curses.h}>],
			[initscr(); tgoto((char *)0, 0, 0);],
			[cf_result=no],
			[
			LIBS="-l$cf_curs_lib -l$cf_term_lib $cf_save_LIBS"
			AC_TRY_LINK([#include <${cf_cv_ncurses_header-curses.h}>],
				[initscr()],
				[cf_result=yes],
				[cf_result=error])
			])
		AC_MSG_RESULT($cf_result)
	fi
fi

])])
dnl ---------------------------------------------------------------------------
dnl You can always use "make -n" to see the actual options, but it's hard to
dnl pick out/analyze warning messages when the compile-line is long.
dnl
dnl Sets:
dnl	ECHO_LD - symbol to prefix "cc -o" lines
dnl	RULE_CC - symbol to put before implicit "cc -c" lines (e.g., .c.o)
dnl	SHOW_CC - symbol to put before explicit "cc -c" lines
dnl	ECHO_CC - symbol to put before any "cc" line
dnl
AC_DEFUN([CF_DISABLE_ECHO],[
AC_MSG_CHECKING(if you want to see long compiling messages)
CF_ARG_DISABLE(echo,
	[  --disable-echo          display "compiling" commands],
	[
    ECHO_LD='@echo linking [$]@;'
    RULE_CC='	@echo compiling [$]<'
    SHOW_CC='	@echo compiling [$]@'
    ECHO_CC='@'
],[
    ECHO_LD=''
    RULE_CC='# compiling'
    SHOW_CC='# compiling'
    ECHO_CC=''
])
AC_MSG_RESULT($enableval)
AC_SUBST(ECHO_LD)
AC_SUBST(RULE_CC)
AC_SUBST(SHOW_CC)
AC_SUBST(ECHO_CC)
])dnl
dnl ---------------------------------------------------------------------------
dnl Look for a non-standard library, given parameters for AC_TRY_LINK.  We
dnl prefer a standard location, and use -L options only if we do not find the
dnl library in the standard library location(s).
dnl	$1 = library name
dnl	$2 = includes
dnl	$3 = code fragment to compile/link
dnl	$4 = corresponding function-name
dnl
dnl Sets the variable "$cf_libdir" as a side-effect, so we can see if we had
dnl to use a -L option.
AC_DEFUN([CF_FIND_LIBRARY],
[
	cf_cv_have_lib_$1=no
	cf_libdir=""
	AC_CHECK_FUNC($4,cf_cv_have_lib_$1=yes,[
		cf_save_LIBS="$LIBS"
		AC_MSG_CHECKING(for $4 in -l$1)
		LIBS="-l$1 $LIBS"
		AC_TRY_LINK([$2],[$3],
			[AC_MSG_RESULT(yes)
			 cf_cv_have_lib_$1=yes
			],
			[AC_MSG_RESULT(no)
			CF_LIBRARY_PATH(cf_search,$1)
			for cf_libdir in $cf_search
			do
				AC_MSG_CHECKING(for -l$1 in $cf_libdir)
				LIBS="-L$cf_libdir -l$1 $cf_save_LIBS"
				AC_TRY_LINK([$2],[$3],
					[AC_MSG_RESULT(yes)
			 		 cf_cv_have_lib_$1=yes
					 break],
					[AC_MSG_RESULT(no)
					 LIBS="$cf_save_LIBS"])
			done
			])
		])
if test $cf_cv_have_lib_$1 = no ; then
	AC_ERROR(Cannot link $1 library)
fi
case $host_os in #(vi
linux*) # Suse Linux does not follow /usr/lib convention
	LIBS="$LIBS -L/lib"
	;;
esac
])dnl
dnl ---------------------------------------------------------------------------
dnl A conventional existence-check for 'lstat' won't work with the Linux
dnl version of gcc 2.7.0, since the symbol is defined only within <sys/stat.h>
dnl as an inline function.
dnl
dnl So much for portability.
AC_DEFUN([CF_FUNC_LSTAT],
[
AC_MSG_CHECKING(for lstat)
AC_CACHE_VAL(ac_cv_func_lstat,[
AC_TRY_LINK([
#include <sys/types.h>
#include <sys/stat.h>],
	[lstat(".", (struct stat *)0)],
	[ac_cv_func_lstat=yes],
	[ac_cv_func_lstat=no])
	])
AC_MSG_RESULT($ac_cv_func_lstat )
if test $ac_cv_func_lstat = yes; then
	AC_DEFINE(HAVE_LSTAT)
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Test for availability of useful gcc __attribute__ directives to quiet
dnl compiler warnings.  Though useful, not all are supported -- and contrary
dnl to documentation, unrecognized directives cause older compilers to barf.
AC_DEFUN([CF_GCC_ATTRIBUTES],
[
if test -n "$GCC"
then
cat > conftest.i <<EOF
#ifndef GCC_PRINTF
#define GCC_PRINTF 0
#endif
#ifndef GCC_SCANF
#define GCC_SCANF 0
#endif
#ifndef GCC_NORETURN
#define GCC_NORETURN /* nothing */
#endif
#ifndef GCC_UNUSED
#define GCC_UNUSED /* nothing */
#endif
EOF
if test -n "$GCC"
then
	AC_CHECKING([for $CC __attribute__ directives])
	changequote(,)dnl
cat > conftest.$ac_ext <<EOF
#line __oline__ "configure"
#include "confdefs.h"
#include "conftest.h"
#include "conftest.i"
#if	GCC_PRINTF
#define GCC_PRINTFLIKE(fmt,var) __attribute__((format(printf,fmt,var)))
#else
#define GCC_PRINTFLIKE(fmt,var) /*nothing*/
#endif
#if	GCC_SCANF
#define GCC_SCANFLIKE(fmt,var)  __attribute__((format(scanf,fmt,var)))
#else
#define GCC_SCANFLIKE(fmt,var)  /*nothing*/
#endif
extern void wow(char *,...) GCC_SCANFLIKE(1,2);
extern void oops(char *,...) GCC_PRINTFLIKE(1,2) GCC_NORETURN;
extern void foo(void) GCC_NORETURN;
int main(int argc GCC_UNUSED, char *argv[] GCC_UNUSED) { return 0; }
EOF
	changequote([,])dnl
	for cf_attribute in scanf printf unused noreturn
	do
		CF_UPPER(CF_ATTRIBUTE,$cf_attribute)
		cf_directive="__attribute__(($cf_attribute))"
		echo "checking for $CC $cf_directive" 1>&AC_FD_CC
		case $cf_attribute in
		scanf|printf)
		cat >conftest.h <<EOF
#define GCC_$CF_ATTRIBUTE 1
EOF
			;;
		*)
		cat >conftest.h <<EOF
#define GCC_$CF_ATTRIBUTE $cf_directive
EOF
			;;
		esac
		if AC_TRY_EVAL(ac_compile); then
			test -n "$verbose" && AC_MSG_RESULT(... $cf_attribute)
			cat conftest.h >>confdefs.h
#		else
#			sed -e 's/__attr.*/\/*nothing*\//' conftest.h >>confdefs.h
		fi
	done
else
	fgrep define conftest.i >>confdefs.h
fi
rm -rf conftest*
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Check if the compiler supports useful warning options.  There's a few that
dnl we don't use, simply because they're too noisy:
dnl
dnl	-Wconversion (useful in older versions of gcc, but not in gcc 2.7.x)
dnl	-Wredundant-decls (system headers make this too noisy)
dnl	-Wtraditional (combines too many unrelated messages, only a few useful)
dnl	-Wwrite-strings (too noisy, but should review occasionally)
dnl	-pedantic
dnl
AC_DEFUN([CF_GCC_WARNINGS],
[
if test -n "$GCC"
then
	changequote(,)dnl
	cat > conftest.$ac_ext <<EOF
#line __oline__ "configure"
int main(int argc, char *argv[]) { return (argv[argc-1] == 0) ; }
EOF
	changequote([,])dnl
	AC_CHECKING([for $CC warning options])
	cf_save_CFLAGS="$CFLAGS"
	EXTRA_CFLAGS="-W -Wall"
	cf_warn_CONST=""
	test "$with_ext_const" = yes && cf_warn_CONST="Wwrite-strings"
	for cf_opt in \
		Wbad-function-cast \
		Wcast-align \
		Wcast-qual \
		Winline \
		Wmissing-declarations \
		Wmissing-prototypes \
		Wnested-externs \
		Wpointer-arith \
		Wshadow \
		Wstrict-prototypes $cf_warn_CONST
	do
		CFLAGS="$cf_save_CFLAGS $EXTRA_CFLAGS -$cf_opt"
		if AC_TRY_EVAL(ac_compile); then
			test -n "$verbose" && AC_MSG_RESULT(... -$cf_opt)
			EXTRA_CFLAGS="$EXTRA_CFLAGS -$cf_opt"
			test "$cf_opt" = Wcast-qual && EXTRA_CFLAGS="$EXTRA_CFLAGS -DXTSTRINGDEFINES"
		fi
	done
	rm -f conftest*
	CFLAGS="$cf_save_CFLAGS"
fi
AC_SUBST(EXTRA_CFLAGS)
])dnl
dnl ---------------------------------------------------------------------------
dnl Check for getopt's variables which are commonly defined in stdlib.h or
dnl (nonstandard) in getopt.h
AC_DEFUN([CF_GETOPT_HEADER],
[
AC_CACHE_CHECK(for header declaring getopt variables,cf_cv_getopt_header,[
cf_cv_getopt_header=none
for cf_header in stdio.h stdlib.h getopt.h
do
AC_TRY_COMPILE([
#include <$cf_header>],
[int x = optind; char *y = optarg],
[cf_cv_getopt_header=$cf_header
 break])
done
])
case $cf_cv_getopt_header in #(vi
getopt.h) #(vi
	AC_DEFINE(HAVE_GETOPT_H)
	AC_DEFINE(HAVE_GETOPT_HEADER)
	;;
stdlib.h) #(vi
	AC_DEFINE(HAVE_STDLIB_H)
	AC_DEFINE(HAVE_GETOPT_HEADER)
	;;
esac
])dnl
dnl ---------------------------------------------------------------------------
dnl Construct a search-list for a nonstandard header-file
AC_DEFUN([CF_HEADER_PATH],
[$1=""
if test -d "$includedir"  ; then
test "$includedir" != NONE       && $1="[$]$1 $includedir $includedir/$2"
fi
if test -d "$oldincludedir"  ; then
test "$oldincludedir" != NONE    && $1="[$]$1 $oldincludedir $oldincludedir/$2"
fi
if test -d "$prefix"; then
test "$prefix" != NONE           && $1="[$]$1 $prefix/include $prefix/include/$2"
fi
test "$prefix" != /usr/local     && $1="[$]$1 /usr/local/include /usr/local/include/$2"
test "$prefix" != /usr           && $1="[$]$1 /usr/include /usr/include/$2"
test "$prefix" != /opt           && $1="[$]$1 /opt/include /opt/include/$2"
])dnl
dnl ---------------------------------------------------------------------------
dnl Insert text into the help-message, for readability, from AC_ARG_WITH.
AC_DEFUN([CF_HELP_MESSAGE],
[AC_DIVERT_HELP([$1])dnl
])dnl
dnl ---------------------------------------------------------------------------
dnl Construct the list of include-options according to whether we're building
dnl in the source directory or using '--srcdir=DIR' option.  If we're building
dnl with gcc, don't append the includedir if it happens to be /usr/include,
dnl since that usually breaks gcc's shadow-includes.
AC_DEFUN([CF_INCLUDE_DIRS],
[
CPPFLAGS="$CPPFLAGS -I. -I../include"
if test "$srcdir" != "."; then
	CPPFLAGS="$CPPFLAGS -I\$(srcdir)/../include"
fi
if test -z "$GCC"; then
	CPPFLAGS="$CPPFLAGS -I\$(includedir)"
elif test "$includedir" != "/usr/include"; then
	if test "$includedir" = '${prefix}/include' ; then
		if test $prefix != /usr ; then
			CPPFLAGS="$CPPFLAGS -I\$(includedir)"
		fi
	else
		CPPFLAGS="$CPPFLAGS -I\$(includedir)"
	fi
fi
AC_SUBST(CPPFLAGS)
])dnl
dnl ---------------------------------------------------------------------------
dnl	Adds to the include-path
dnl
dnl	Autoconf 1.11 should have provided a way to add include path options to
dnl	the cpp-tests.
dnl
AC_DEFUN([CF_INCLUDE_PATH],
[
for cf_path in $1
do
	cf_result="no"
	AC_MSG_CHECKING(for directory $cf_path)
	if test -d $cf_path
	then
		INCLUDES="$INCLUDES -I$cf_path"
		ac_cpp="${ac_cpp} -I$cf_path"
		CFLAGS="$CFLAGS -I$cf_path"
		cf_result="yes"
		case $cf_path in
		/usr/include|/usr/include/*)
			;;
		*)
changequote(,)dnl
			cf_temp=`echo $cf_path | sed -e s'%/[^/]*$%%'`
changequote([,])dnl
			case $cf_temp in
			*/include)
				INCLUDES="$INCLUDES -I$cf_temp"
				ac_cpp="${ac_cpp} -I$cf_temp"
				CFLAGS="$CFLAGS -I$cf_temp"
				;;
			esac
		esac
	fi
	AC_MSG_RESULT($cf_result)
done
])dnl
dnl ---------------------------------------------------------------------------
dnl Construct a search-list for a nonstandard library-file
AC_DEFUN([CF_LIBRARY_PATH],
[$1=""
if test -d "$libdir"  ; then
test "$libdir" != NONE           && $1="[$]$1 $libdir $libdir/$2"
fi
if test -d "$exec_prefix"; then
test "$exec_prefix" != NONE      && $1="[$]$1 $exec_prefix/lib $exec_prefix/lib/$2"
fi
if test -d "$prefix"; then
test "$prefix" != NONE           && \
test "$prefix" != "$exec_prefix" && $1="[$]$1 $prefix/lib $prefix/lib/$2"
fi
test "$prefix" != /usr/local     && $1="[$]$1 /usr/local/lib /usr/local/lib/$2"
test "$prefix" != /usr           && $1="[$]$1 /usr/lib /usr/lib/$2"
test "$prefix" != /opt           && $1="[$]$1 /opt/lib /opt/lib/$2"
])dnl
dnl ---------------------------------------------------------------------------
dnl Compute the library-prefix for the given host system
dnl $1 = variable to set
AC_DEFUN([CF_LIB_PREFIX],
[
	case $cf_cv_system_name in
	os2)	LIB_PREFIX=''     ;;
	*)	LIB_PREFIX='lib'  ;;
	esac
ifelse($1,,,[$1=$LIB_PREFIX])
	AC_SUBST(LIB_PREFIX)
])dnl
dnl ---------------------------------------------------------------------------
dnl Some 'make' programs support $(MAKEFLAGS), some $(MFLAGS), to pass 'make'
dnl options to lower-levels.  It's very useful for "make -n" -- if we have it.
dnl (GNU 'make' does both, something POSIX 'make', which happens to make the
dnl $(MAKEFLAGS) variable incompatible because it adds the assignments :-)
AC_DEFUN([CF_MAKEFLAGS],
[
AC_MSG_CHECKING([for makeflags variable])
AC_CACHE_VAL(cf_cv_makeflags,[
	cf_cv_makeflags=''
	for cf_option in '-$(MAKEFLAGS)' '$(MFLAGS)'
	do
		cat >cf_makeflags.tmp <<CF_EOF
all :
	@ echo '.$cf_option'
CF_EOF
		cf_result=`${MAKE-make} -k -f cf_makeflags.tmp 2>/dev/null`
		case "$cf_result" in
		.*k)
			cf_result=`${MAKE-make} -k -f cf_makeflags.tmp CC=cc 2>/dev/null`
			case "$cf_result" in
			.*CC=*)	cf_cv_makeflags=
				;;
			*)	cf_cv_makeflags=$cf_option
				;;
			esac
			break
			;;
		*)	echo no match "$cf_result"
			;;
		esac
	done
	rm -f cf_makeflags.tmp])
AC_MSG_RESULT($cf_cv_makeflags)
AC_SUBST(cf_cv_makeflags)
])dnl
dnl ---------------------------------------------------------------------------
dnl Write a debug message to config.log, along with the line number in the
dnl configure script.
AC_DEFUN([CF_MSG_LOG],[
echo "(line __oline__) testing $* ..." 1>&AC_FD_CC
])dnl
dnl ---------------------------------------------------------------------------
dnl Look for the SVr4 curses clone 'ncurses' in the standard places, adjusting
dnl the CPPFLAGS variable.
dnl
dnl The header files may be installed as either curses.h, or ncurses.h
dnl (obsolete).  If not installed for overwrite, the curses.h file would be
dnl in an ncurses subdirectory (e.g., /usr/include/ncurses), but someone may
dnl have installed overwriting the vendor's curses.  Only very old versions
dnl (pre-1.9.2d, the first autoconf'd version) of ncurses don't define
dnl either __NCURSES_H or NCURSES_VERSION in the header.
dnl
dnl If the installer has set $CFLAGS or $CPPFLAGS so that the ncurses header
dnl is already in the include-path, don't even bother with this, since we cannot
dnl easily determine which file it is.  In this case, it has to be <curses.h>.
dnl
AC_DEFUN([CF_NCURSES_CPPFLAGS],
[
AC_MSG_CHECKING(for ncurses header file)
AC_CACHE_VAL(cf_cv_ncurses_header,[
	AC_TRY_COMPILE([#include <curses.h>],[
#ifdef NCURSES_VERSION
printf("%s\n", NCURSES_VERSION);
#else
#ifdef __NCURSES_H
printf("old\n");
#else
make an error
#endif
#endif
	],
	[cf_cv_ncurses_header=predefined],[
	CF_HEADER_PATH(cf_search,ncurses)
	test -n "$verbose" && echo
	for cf_incdir in $cf_search
	do
		for cf_header in \
			curses.h \
			ncurses.h
		do
changequote(,)dnl
			if egrep "NCURSES_[VH]" $cf_incdir/$cf_header 1>&AC_FD_CC 2>&1; then
changequote([,])dnl
				cf_cv_ncurses_header=$cf_incdir/$cf_header
				test -n "$verbose" && echo $ac_n "	... found $ac_c" 1>&AC_FD_MSG
				break
			fi
			test -n "$verbose" && echo "	... tested $cf_incdir/$cf_header" 1>&AC_FD_MSG
		done
		test -n "$cf_cv_ncurses_header" && break
	done
	test -z "$cf_cv_ncurses_header" && AC_ERROR(not found)
	])])
AC_MSG_RESULT($cf_cv_ncurses_header)
AC_DEFINE(NCURSES)

changequote(,)dnl
cf_incdir=`echo $cf_cv_ncurses_header | sed -e 's:/[^/]*$::'`
changequote([,])dnl

case $cf_cv_ncurses_header in # (vi
*/ncurses.h)
	AC_DEFINE(HAVE_NCURSES_H)
	;;
esac

case $cf_cv_ncurses_header in # (vi
predefined) # (vi
	cf_cv_ncurses_header=curses.h
	;;
*)
	CF_ADD_INCDIR($cf_incdir)
	;;
esac
CF_NCURSES_VERSION
])dnl
dnl ---------------------------------------------------------------------------
dnl Look for the ncurses library.  This is a little complicated on Linux,
dnl because it may be linked with the gpm (general purpose mouse) library.
dnl Some distributions have gpm linked with (bsd) curses, which makes it
dnl unusable with ncurses.  However, we don't want to link with gpm unless
dnl ncurses has a dependency, since gpm is normally set up as a shared library,
dnl and the linker will record a dependency.
AC_DEFUN([CF_NCURSES_LIBS],
[AC_REQUIRE([CF_NCURSES_CPPFLAGS])

	# This works, except for the special case where we find gpm, but
	# ncurses is in a nonstandard location via $LIBS, and we really want
	# to link gpm.
cf_ncurses_LIBS=""
cf_ncurses_SAVE="$LIBS"
AC_CHECK_LIB(gpm,Gpm_Open,
	[AC_CHECK_LIB(gpm,initscr,
		[LIBS="$cf_ncurses_SAVE"],
		[cf_ncurses_LIBS="-lgpm"])])

case $host_os in #(vi
freebsd*)
	# This is only necessary if you are linking against an obsolete
	# version of ncurses (but it should do no harm, since it's static).
	AC_CHECK_LIB(mytinfo,tgoto,[cf_ncurses_LIBS="-lmytinfo $cf_ncurses_LIBS"])
	;;
esac

LIBS="$cf_ncurses_LIBS $LIBS"
CF_FIND_LIBRARY(ncurses,
	[#include <${cf_cv_ncurses_header-curses.h}>],
	[initscr()],
	initscr)

if test -n "$cf_ncurses_LIBS" ; then
	AC_MSG_CHECKING(if we can link ncurses without $cf_ncurses_LIBS)
	cf_ncurses_SAVE="$LIBS"
	for p in $cf_ncurses_LIBS ; do
		q=`echo $LIBS | sed -e 's/'$p' //' -e 's/'$p'$//'`
		if test "$q" != "$LIBS" ; then
			LIBS="$q"
		fi
	done
	AC_TRY_LINK([#include <${cf_cv_ncurses_header-curses.h}>],
		[initscr(); mousemask(0,0); tgoto((char *)0, 0, 0);],
		[AC_MSG_RESULT(yes)],
		[AC_MSG_RESULT(no)
		 LIBS="$cf_ncurses_SAVE"])
fi
])dnl
dnl ---------------------------------------------------------------------------
dnl Check for the version of ncurses, to aid in reporting bugs, etc.
AC_DEFUN([CF_NCURSES_VERSION],
[AC_MSG_CHECKING(for ncurses version)
AC_CACHE_VAL(cf_cv_ncurses_version,[
	cf_cv_ncurses_version=no
	cf_tempfile=out$$
	AC_TRY_RUN([
#include <${cf_cv_ncurses_header-curses.h}>
int main()
{
	FILE *fp = fopen("$cf_tempfile", "w");
#ifdef NCURSES_VERSION
# ifdef NCURSES_VERSION_PATCH
	fprintf(fp, "%s.%d\n", NCURSES_VERSION, NCURSES_VERSION_PATCH);
# else
	fprintf(fp, "%s\n", NCURSES_VERSION);
# endif
#else
# ifdef __NCURSES_H
	fprintf(fp, "old\n");
# else
	make an error
# endif
#endif
	exit(0);
}],[
	cf_cv_ncurses_version=`cat $cf_tempfile`
	rm -f $cf_tempfile],,[

	# This will not work if the preprocessor splits the line after the
	# Autoconf token.  The 'unproto' program does that.
	cat > conftest.$ac_ext <<EOF
#include <${cf_cv_ncurses_header-curses.h}>
#undef Autoconf
#ifdef NCURSES_VERSION
Autoconf NCURSES_VERSION
#else
#ifdef __NCURSES_H
Autoconf "old"
#endif
;
#endif
EOF
	cf_try="$ac_cpp conftest.$ac_ext 2>&AC_FD_CC | grep '^Autoconf ' >conftest.out"
	AC_TRY_EVAL(cf_try)
	if test -f conftest.out ; then
changequote(,)dnl
		cf_out=`cat conftest.out | sed -e 's@^Autoconf @@' -e 's@^[^"]*"@@' -e 's@".*@@'`
changequote([,])dnl
		test -n "$cf_out" && cf_cv_ncurses_version="$cf_out"
		rm -f conftest.out
	fi
])])
AC_MSG_RESULT($cf_cv_ncurses_version)
])
dnl ---------------------------------------------------------------------------
dnl Configure for PDCurses' X11 library
AC_DEFUN([CF_PDCURSES_X11],[
AC_REQUIRE([CF_X_ATHENA])
LDFLAGS="$LDFLAGS $X_LIBS"
CFLAGS="$CFLAGS $X_CFLAGS"
AC_CHECK_LIB(X11,XOpenDisplay,
	[LIBS="-lX11 $LIBS"],,
	[$X_PRE_LIBS $LIBS $X_EXTRA_LIBS])
AC_CACHE_CHECK(for XCurses library,cf_cv_lib_XCurses,[
LIBS="-lXCurses $LIBS"
AC_TRY_LINK([
#include <xcurses.h>
char *XCursesProgramName = "test";
],[XCursesExit();],
[cf_cv_lib_XCurses=yes],
[cf_cv_lib_XCurses=no])
])
if test $cf_cv_lib_XCurses = yes ; then
	AC_DEFINE(UNIX)
	AC_DEFINE(XCURSES)
	AC_DEFINE(HAVE_XCURSES)
else
	AC_ERROR(Cannot link with XCurses)
fi
])
dnl ---------------------------------------------------------------------------
dnl Compute $PROG_EXT, used for non-Unix ports, such as OS/2 EMX.
AC_DEFUN([CF_PROG_EXT],
[
AC_REQUIRE([CF_CHECK_CACHE])
PROG_EXT=
case $cf_cv_system_name in
os2*)
    # We make sure -Zexe is not used -- it would interfere with @PROG_EXT@
    CFLAGS="$CFLAGS -Zmt -D__ST_MT_ERRNO__"
    CXXFLAGS="$CXXFLAGS -Zmt -D__ST_MT_ERRNO__"
    LDFLAGS=`echo "$LDFLAGS -Zmt -Zcrtdll" | sed "s/-Zexe//g"`
    PROG_EXT=".exe"
    ;;
cygwin*)
    PROG_EXT=".exe"
    ;;
esac
AC_SUBST(PROG_EXT)
])dnl
dnl ---------------------------------------------------------------------------
dnl	Shorthand macro for substituting things that the user may override
dnl	with an environment variable.
dnl
dnl	$1 = long/descriptive name
dnl	$2 = environment variable
dnl	$3 = default value
AC_DEFUN([CF_SUBST],
[AC_CACHE_VAL(cf_cv_subst_$2,[
AC_MSG_CHECKING(for $1 (symbol $2))
test -z "[$]$2" && $2=$3
AC_MSG_RESULT([$]$2)
AC_SUBST($2)
cf_cv_subst_$2=[$]$2])
$2=${cf_cv_subst_$2}
])dnl
dnl ---------------------------------------------------------------------------
dnl Derive the system-type (our main clue to the method of building shared
dnl libraries).
AC_DEFUN([CF_SYSTYPE],
[
AC_MSG_CHECKING(for system type)
AC_CACHE_VAL(cf_cv_systype,[
AC_ARG_WITH(system-type,
[  --with-system-type=XXX  test: override derived host system-type],
[cf_cv_systype=$withval],
[
changequote(,)dnl
cf_cv_systype="`(uname -s || hostname || echo unknown) 2>/dev/null |sed -e s'/[:\/.-]/_/'g  | sed 1q`"
changequote([,])dnl
if test -z "$cf_cv_systype"; then cf_cv_systype=unknown;fi
])])
AC_MSG_RESULT($cf_cv_systype)
])dnl
dnl ---------------------------------------------------------------------------
dnl Derive the system name, as a check for reusing the autoconf cache
AC_DEFUN([CF_SYS_NAME],
[
SYS_NAME=`(uname -s -r || uname -a || hostname) 2>/dev/null | sed 1q`
test -z "$SYS_NAME" && SYS_NAME=unknown
AC_DEFINE_UNQUOTED(SYS_NAME,"$SYS_NAME")

AC_CACHE_VAL(cf_cv_system_name,[cf_cv_system_name="$SYS_NAME"])

if test ".$SYS_NAME" != ".$cf_cv_system_name" ; then
	AC_MSG_RESULT("Cached system name does not agree with actual")
	AC_ERROR("Please remove config.cache and try again.")
fi])
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
dnl Make an uppercase version of a given name
AC_DEFUN([CF_UPPERCASE],
[
changequote(,)dnl
$2=`echo $1 |tr '[a-z]' '[A-Z]'`
changequote([,])dnl
])dnl
dnl ---------------------------------------------------------------------------
dnl Use AC_VERBOSE w/o the warnings
AC_DEFUN([CF_VERBOSE],
[test -n "$verbose" && echo "	$1" 1>&AC_FD_MSG
])dnl
dnl ---------------------------------------------------------------------------
dnl Test if we should define X/Open source for curses, needed on Digital Unix
dnl 4.x, to see the extended functions, but breaks on IRIX 6.x.
AC_DEFUN([CF_XOPEN_CURSES],
[
AC_CACHE_CHECK(if we must define _XOPEN_SOURCE_EXTENDED,cf_cv_need_xopen_extension,[
AC_TRY_LINK([
#include <stdlib.h>
#include <curses.h>],[
	long x = winnstr(stdscr, "", 0)],
	[cf_cv_need_xopen_extension=no],
	[AC_TRY_LINK([
#define _XOPEN_SOURCE_EXTENDED
#include <stdlib.h>
#include <curses.h>],[
	long x = winnstr(stdscr, "", 0)],
	[cf_cv_need_xopen_extension=yes],
	[cf_cv_need_xopen_extension=no])])])
test $cf_cv_need_xopen_extension = yes && CFLAGS="$CFLAGS -D_XOPEN_SOURCE_EXTENDED"
])dnl
dnl ---------------------------------------------------------------------------
dnl Check for Xaw (Athena) libraries
dnl
AC_DEFUN([CF_X_ATHENA],
[AC_REQUIRE([CF_X_TOOLKIT])
cf_x_athena=${cf_x_athena-Xaw}

AC_ARG_WITH(Xaw3d,
	[  --with-Xaw3d            link with Xaw 3d library],
	[cf_x_athena=Xaw3d])

AC_ARG_WITH(neXtaw,
	[  --with-neXtaw           link with neXT Athena library],
	[cf_x_athena=neXtaw])


AC_CHECK_LIB(Xext,XextCreateExtension,
	[LIBS="-lXext $LIBS"])

cf_x_athena_include=""
cf_x_athena_lib=""

for cf_path in default \
	/usr/contrib/X11R6 \
	/usr/contrib/X11R5 \
	/usr/lib/X11R5
do

	if test -z "$cf_x_athena_include" ; then
		cf_save="$CFLAGS"
		cf_test=X11/$cf_x_athena/SimpleMenu.h
		if test $cf_path != default ; then
			CFLAGS="-I$cf_path/include $cf_save"
			AC_MSG_CHECKING(for $cf_test in $cf_path)
		else
			AC_MSG_CHECKING(for $cf_test)
		fi
		AC_TRY_COMPILE([
#include <X11/Intrinsic.h>
#include <$cf_test>],[],
			[cf_result=yes],
			[cf_result=no])
		AC_MSG_RESULT($cf_result)
		if test "$cf_result" = yes ; then
			cf_x_athena_include=$cf_path
		else
			CFLAGS="$cf_save"
		fi
	fi

	for cf_lib in "-l$cf_x_athena -lXmu" "-l${cf_x_athena}_s -lXmu_s"
	do
		if test -z "$cf_x_athena_lib" ; then
			cf_save="$LIBS"
			cf_test=XawSimpleMenuAddGlobalActions
			if test $cf_path != default ; then
				LIBS="-L$cf_path/lib $cf_lib $LIBS"
				AC_MSG_CHECKING(for $cf_lib in $cf_path)
			else
				LIBS="$cf_lib $LIBS"
				AC_MSG_CHECKING(for $cf_test in $cf_lib)
			fi
			AC_TRY_LINK([],[$cf_test()],
				[cf_result=yes],
				[cf_result=no],
				[$X_PRE_LIBS $LIBS $X_EXTRA_LIBS])
			AC_MSG_RESULT($cf_result)
			if test "$cf_result" = yes ; then
				cf_x_athena_lib="$cf_lib"
			else
				LIBS="$cf_save"
			fi
		fi
	done
done

if test -z "$cf_x_athena_include" ; then
	AC_MSG_WARN(
[Unable to successfully find Athena header files with test program])
fi

if test -z "$cf_x_athena_lib" ; then
	AC_ERROR(
[Unable to successfully link Athena library (-l$cf_x_athena) with test program])
fi

CF_UPPER(CF_X_ATHENA_LIBS,HAVE_LIB_$cf_x_athena)
AC_DEFINE_UNQUOTED($CF_X_ATHENA_LIBS)
])dnl
dnl ---------------------------------------------------------------------------
dnl Check for X Toolkit libraries
dnl
AC_DEFUN([CF_X_TOOLKIT],
[
AC_REQUIRE([CF_CHECK_CACHE])
# We need to check for -lsocket and -lnsl here in order to work around an
# autoconf bug.  autoconf-2.12 is not checking for these prior to checking for
# the X11R6 -lSM and -lICE libraries.  The resultant failures cascade...
# 	(tested on Solaris 2.5 w/ X11R6)
SYSTEM_NAME=`echo "$cf_cv_system_name"|tr ' ' -`
cf_have_X_LIBS=no
case $SYSTEM_NAME in
changequote(,)dnl
irix[56]*) ;;
changequote([,])dnl
clix*)
	# FIXME: modify the library lookup in autoconf to
	# allow _s.a suffix ahead of .a
	AC_CHECK_LIB(c_s,open,
		[LIBS="-lc_s $LIBS"
	AC_CHECK_LIB(bsd,gethostname,
		[LIBS="-lbsd $LIBS"
	AC_CHECK_LIB(nsl_s,gethostname,
		[LIBS="-lnsl_s $LIBS"
	AC_CHECK_LIB(X11_s,XOpenDisplay,
		[LIBS="-lX11_s $LIBS"
	AC_CHECK_LIB(Xt_s,XtAppInitialize,
		[LIBS="-lXt_s $LIBS"
		 cf_have_X_LIBS=Xt
		]) ]) ]) ]) ])
	;;
*)
	AC_CHECK_LIB(socket,socket)
	AC_CHECK_LIB(nsl,gethostname)
	;;
esac

if test $cf_have_X_LIBS = no ; then
	AC_PATH_XTRA
	LDFLAGS="$LDFLAGS $X_LIBS"
	CFLAGS="$CFLAGS $X_CFLAGS"
	AC_CHECK_LIB(X11,XOpenDisplay,
		[LIBS="-lX11 $LIBS"],,
		[$X_PRE_LIBS $LIBS $X_EXTRA_LIBS])
	AC_CHECK_LIB(Xt, XtAppInitialize,
		[AC_DEFINE(HAVE_LIBXT)
		 cf_have_X_LIBS=Xt
		 LIBS="-lXt $X_PRE_LIBS $LIBS"],,
		[$X_PRE_LIBS $LIBS $X_EXTRA_LIBS])
else
	LDFLAGS="$LDFLAGS $X_LIBS"
	CFLAGS="$CFLAGS $X_CFLAGS"
fi

if test $cf_have_X_LIBS = no ; then
	AC_WARN(
[Unable to successfully link X Toolkit library (-lXt) with
test program.  You will have to check and add the proper libraries by hand
to makefile.])
fi
])dnl
