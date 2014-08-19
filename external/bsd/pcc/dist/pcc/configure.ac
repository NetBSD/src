                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([pcc], [1.1.0.DEVEL], <pcc-list@ludd.ltu.se>)
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_TARGET

abi=unknown
endian=little
targosver=0
tls=no
gcccompat=yes
pccdebug=yes
stripping=yes
native=no
useyasm=no
stabs=no
# allowed: UNSIGNED (4-char u_int), INT (4-char int), SHORT (2-char u_short)
wchar_type=INT

case "$target_os" in

    apple)
	targos=apple
	abi=classic68k
	stabs=yes
	case "$target_cpu" in
	    m68k) targmach=m68k endian=big ;;
	esac
	;;

    bsd)
	targos=bsd
	abi=aout
	case "$target_cpu" in
	    pdp11) targmach=pdp11 ;;
	    nova) targmach=nova ;;
	esac
	wchar_type=USHORT
	;;

    darwin*)
	targos=darwin
	abi=macho
	stabs=yes
	case "$target_os" in
	    *10.*) targosver=10 ;;
	    *9.*) targosver=9 ;;
	    *8.*) targosver=8 ;;
	    *7.*) targosver=7 ;;
	esac
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    powerpc) targmach=powerpc endian=big ;;
	    x86_64) targmach=amd64 ;;
	esac
        ;;

    dragonfly*)
	targos=dragonfly
	abi=elf
	stabs=yes
	tls=yes
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    freebsd*)
	targos=freebsd
	abi=elf
	stabs=yes
	case "$target_os" in
	    *10.*) targosver=10 ;;
	    *9.*) targosver=9 ;;
	    *8.*) targosver=8 ;;
	    *7.*) targosver=7 ;;
	    *6.*) targosver=6 ;;
	    *5.*) targosver=5 ;;
	    *4.*) targosver=4 ;;
	esac
	case "$target_cpu" in
	    i386) targmach=i386 ;;
	    sparc64) targmach=sparc64 endian=big ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    linux*)
	targos=linux
	abi=elf
	stabs=yes
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    powerpc*) targmach=powerpc endian=big ;;
	    x86_64) targmach=amd64 ;;
	    mipseb) targmach=mips endian=big ;;
	    mips*) targmach=mips ;;
	esac
	;;

    midnightbsd*)
	targos=midnightbsd
	abi=elf
	stabs=yes
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    sparc64) targmach=sparc64 endian=big ;;
	esac
	;;

    mingw*)
	targos=win32
	abi=pecoff
	wchar_type=USHORT
	targmach=i386
	altincdir="c:/mingw/include"
	altlibdir="c:/mingw/lib"
	;;

    minix*)
 	targos=minix
	stabs=yes
	case "$target_os" in
	    # explicit setting
	    minixaout|minix2|*aout) abi=aout ;;
	    minixelf|minix3|*elf) abi=elf ;;
	    # default to ELF
	    *) abi=elf ;;
	esac
	case "$target_cpu" in
	    i86) targmach=i86 ;;
	    i?86) targmach=i386 ;;
	esac
	;;

    mirbsd*)
	targos=mirbsd
	abi=elf
	stabs=yes
	wchar_type=USHORT
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	esac
	;;

    netbsd*)
	targos=netbsd
	abi=elf
	stabs=yes
	case "$target_os" in
	    *7.*) targosver=7 ;;
	    *6.*) targosver=6 ;;
	    *5.*) targosver=5 ;;
	    *4.*) targosver=4 ;;
	    *3.*) targosver=3 ;;
	    *2.*) targosver=2 ;;
	    *1.*) targosver=1 ;;
	esac
	case "$target_cpu" in
	    armeb) targmach=arm endian=big ;;
	    arm*) targmach=arm ;;
	    i?86) targmach=i386 ;;
	    m68k*) targmach=m68k endian=big ;;
	    mipseb) targmach=mips endian=big ;;
	    mips*) targmach=mips ;;
	    pdp10) targmach=pdp10 ;;
	    powerpc) targmach=powerpc endian=big ;;
	    sparc64) targmach=sparc64 endian=big ;;
	    vax) targmach=vax ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    nextstep*)
	targos=nextstep
	abi=macho
	stabs=yes
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    sparc) targmach=sparc endian=big ;;
	    hppa) targmach=hppa endian=big ;;
	esac
	;;

    openbsd*)
	targos=openbsd
	abi=elf
	stabs=yes
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    vax) targmach=vax ;;
	    powerpc) targmach=powerpc endian=big ;;
	    sparc64) targmach=sparc64 endian=big ;;
	    m68k) targmach=m68k endian=big ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    sysv4*)
	targos=sysv4
	abi=elf
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	esac
	;;

    sunos*|solaris*)
        targos=sunos
        abi=elf
	stabs=yes
        case "$target_cpu" in
            i?86) targmach=i386 ;;
            sparc*) targmach=sparc64 endian=big ;;
        esac
        ;;

    windows*|pe*)
	target_alias=i386-pe
	targos=win32
	abi=pecoff
	wchar_type=USHORT
	targmach=i386
	;;

    *)
        targos="$target_os"
	case "$target_cpu" in
	    m16c) targmach=m16c ;;
	    nova) targmach=nova ;;
	esac
	;;
esac

if test "X$targos" = X -o "X$targmach" = X ; then
	AC_MSG_ERROR(['$target' is not (yet) supported by pcc.])
fi

case "$host_os" in

    apple)
        hostos=apple
	;;
    bsd)
        hostos=bsd
	;;
    darwin*)
	echo $targosver
	if test "$targosver" -ge 10 ; then
		targmach=amd64
	fi
	hostos=darwin
	;;
    dragonfly*)
        hostos=dragonfly
	;;
    freebsd*)
        hostos=freebsd
	;;
    linux*)
        hostos=linux
	;;
    midnightbsd*)
        hostos=midnightbsd
	;;
    mingw*)
        hostos=win32
	;;
    minix*)
        hostos=minix
	;;
    mirbsd*)
        hostos=mirbsd
	;;
    netbsd*)
        hostos=netbsd
	;;
    nextstep*)
        hostos=nextstep
	;;
    openbsd*)
        hostos=openbsd
	;;
    sunos*|solaris*)
	ADD_CPPFLAGS="$ADD_CPPFLAGS -D_XOPEN_SOURCE=600"
        hostos=sunos
	;;
    pe*|windows*)
	# quick hack for cross-build to win32 host
	hostos=win32
	if "$prefix" = NONE; then
		prefix="c:/pcc"
		assembler="yasm.exe -p gnu -f win32"
		linker="link.exe /nologo"
		ADD_CPPFLAGS="$ADD_CPPFLAGS -DMSLINKER"
	fi
	;;

esac

if test "X$endian" = "Xbig" ; then
	AC_DEFINE(TARGET_BIG_ENDIAN, 1,
		[Define if target defaults to BIG endian])
else
	AC_DEFINE(TARGET_LITTLE_ENDIAN, 1,
		[Define if target defaults to LITTLE endian])
fi

case "$abi" in
	elf*)		AC_DEFINE(ELFABI, 1, [Using ELF ABI]) ;;
	aout)		AC_DEFINE(AOUTABI, 1, [Using a.out ABI]) ;;
	macho)		AC_DEFINE(MACHOABI, 1, [Using Mach-O ABI]) ;;
	coff)		AC_DEFINE(COFFABI, 1, [Using COFF ABI]) ;;
	ecoff)		AC_DEFINE(ECOFFABI, 1, [Using ECOFF ABI]) ;;
	pecoff)		AC_DEFINE(PECOFFABI, 1, [Using PE/COFF ABI]) ;;
	classic68k)	AC_DEFINE(CLASSIC68K, 1, [Using Classic 68k ABI]) ;;
esac

if test "$stabs" = "yes"; then
	AC_DEFINE(STABS, 1, [Enable STABS debugging output])
fi

# Specify alternate assembler, linker, include and lib paths
AC_ARG_WITH(incdir,
	AC_HELP_STRING([--with-incdir=<path>],
		[Specify the default include path.]),
	altincdir=$withval,
	[])
AC_ARG_WITH(libdir,
	AC_HELP_STRING([--with-libdir=<path>],
		[Specify the default library path.]),
	altlibdir=$withval,
	[])
AC_ARG_WITH(assembler,
	AC_HELP_STRING([--with-assembler=<path>],
		[Specify alternate assember.]),
	assembler=$withval,
	[])
AC_ARG_WITH(linker,
	AC_HELP_STRING([--with-linker=<path>],
		[Specify alternate linker.]),
	linker=$withval,
	[])
AC_ARG_ENABLE(tls,
	AC_HELP_STRING([--enable-tls],
		[Enable Thread-local storage (TLS).]),
	[tls=$enableval], [])
if test "$tls" = "yes"; then
	AC_DEFINE(TLS, 1, [Enable thread-local storage (TLS).])
fi
AC_ARG_ENABLE(Werror,
	AC_HELP_STRING([--enable-Werror],
		[Enable use of compiler -Werror flag]),
	[werror=$enableval], [])
if test "$werror" = "yes"; then
	ADD_CFLAGS="$ADD_CFLAGS -Werror"
fi
AC_ARG_ENABLE(gcc-compat,
	AC_HELP_STRING([--disable-gcc-compat],
		[Disable GCC compatibility]),
	[gcccompat=$enableval], [])
if test "$gcccompat" = "yes"; then
	ADD_CPPFLAGS="$ADD_CPPFLAGS -DGCC_COMPAT";
fi
AC_ARG_ENABLE(pcc-debug,
	AC_HELP_STRING([--disable-pcc-debug],
		[Disable PCC debugging]),
	[pccdebug=$enableval], [])
if test "$pccdebug" = "yes"; then
	ADD_CPPFLAGS="$ADD_CPPFLAGS -DPCC_DEBUG";
fi

AC_ARG_ENABLE(stripping,
	AC_HELP_STRING([--disable-stripping],
		[Disable stripping of symbols in installed binaries]),
	[stripping=$enableval], [])
if test "$stripping" = "yes"; then
	if test -z "$INSTALL_PROGRAM"; then
		INSTALL_PROGRAM='${INSTALL} -s'
	else
		AC_MSG_WARN([Installed binaries may be unstripped])
	fi
fi

AC_ARG_WITH(yasm,
	AC_HELP_STRING([--use-yasm], [Use yasm assembler]),
	useyasm=$withval,
	[])
if test "$useyasm" = "yes"; then
	assembler="yasm"
	ADD_CPPFLAGS="$ADD_CPPFLAGS -DUSE_YASM"
fi
AC_ARG_ENABLE(native,
	AC_HELP_STRING([--enable-native],
		[Build the compiler as a native rather than cross-build compiler]),
	[native=$enableval], [])

# setup for building a cross-compiler
if test "X$native" = "Xyes" -o "X$target_alias" = "X$host_alias" -o "X$target_alias" = "X"; then
	BINPREFIX=""
else
	BINPREFIX="${target_alias}-"
	test "X$prefix" = XNONE && prefix="$ac_default_prefix"
	test "X$exec_prefix" = XNONE && exec_prefix="${prefix}"
	if test -z "$altincdir"; then
		altincdir=${exec_prefix}/${target_alias}/include
	fi
	if test -z "$altlibdir"; then
		altlibdir=${exec_prefix}/${target_alias}/lib
	fi
	if test -z "$assembler"; then
		assembler=${BINPREFIX}as
	fi
	if test -z  "$linker"; then
		linker=${BINPREFIX}ld
	fi
	preprocessor="${BINPREFIX}cpp"
	compiler="${BINPREFIX}ccom"
fi
AC_SUBST(BINPREFIX)

if test -n "$altincdir"; then
	AC_DEFINE_UNQUOTED(STDINC, "$altincdir",
		[Define alternate standard include directory])
fi
if test -n "$altlibdir"; then
	AC_DEFINE_UNQUOTED(LIBDIR, "${altlibdir}/",
		[Define alternate standard lib directory])
fi
if test -n "$assembler"; then
	AC_DEFINE_UNQUOTED(ASSEMBLER, "$assembler",
		[Define path to alternate assembler])
fi
if test -n "$linker"; then
	AC_DEFINE_UNQUOTED(LINKER, "$linker",
		[Define path to alternate linker])
fi
if test -n "$preprocessor"; then
	AC_DEFINE_UNQUOTED(PREPROCESSOR, "$preprocessor",
		[Define path to alternate preprocessor])
fi
if test -n "$compiler"; then
	AC_DEFINE_UNQUOTED(COMPILER, "$compiler",
		[Define path to alternate compiler])
fi

case $wchar_type in
USHORT) wchar_size=2 ;;
UNSIGNED|INT) wchar_size=4 ;;
*) AC_MSG_ERROR([Unknown wchar_t '$wchar_type'.]) ;;
esac

AC_DEFINE_UNQUOTED(WCHAR_TYPE, $wchar_type, [Type to use for wide characters])
AC_DEFINE_UNQUOTED(WCHAR_SIZE, $wchar_size, [Size of wide-character type in chars])

# check for additional compiler flags
AC_PROG_CC
DESIRED_FLAGS="-Wall -Wmissing-prototypes -Wstrict-prototypes -Wshadow -Wsign-compare -Wtruncate"
for flag in $DESIRED_FLAGS
do
	AC_MSG_CHECKING([whether $CC accepts $flag])
	cflags="$CFLAGS"
	CFLAGS="$CFLAGS $flag -Werror"
	AC_COMPILE_IFELSE([
	    AC_LANG_PROGRAM([[]], [[]])
	  ], [
	    use_flag=yes
	  ], [
	    use_flag=no
	])
	CFLAGS="$cflags"

	AC_MSG_RESULT([$use_flag])
	if test $use_flag = yes; then
	    ADD_CFLAGS="$ADD_CFLAGS $flag"
	fi
done

# setup for cross-compiling mkext
AC_MSG_CHECKING([for a C compiler for mkext])
if test $cross_compiling = yes; then
        AC_MSG_RESULT([cross compiling])
        AC_CHECK_PROGS(CC_FOR_BUILD, [pcc gcc cc])
else
        AC_MSG_RESULT([not cross compiling])
        CC_FOR_BUILD=${CC-cc}
        AC_SUBST(CC_FOR_BUILD)
fi

AC_CACHE_CHECK([for C99 printf size specifiers], ac_cv_have_c99_format, [
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT], [[
      char buf[64];
      if (sprintf(buf, "%lld%hhd%jd%zd%td", (long long int)1, (char)2, (intmax_t)3, (size_t)4, (ptrdiff_t)5) != 5)
        exit(1);
      else if (strcmp(buf, "12345"))
        exit(2);
      ]])],
    [ ac_cv_have_c99_format=yes ],
    [ ac_cv_have_c99_format=no ],
    [ ac_cv_have_c99_format=yes ])
])
if test $ac_cv_have_c99_format = yes; then
  AC_DEFINE([HAVE_C99_FORMAT], 1,
    [Define to 1 if printf supports C99 size specifiers])
fi

# Byteorder of host
AC_C_BIGENDIAN([AC_DEFINE(HOST_BIG_ENDIAN, 1, [Define if host is BIG endian])],
	[AC_DEFINE(HOST_LITTLE_ENDIAN, 1, [Define if host is LITTLE endian])],
	[],[])

# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_LEX

# Checks for libraries.

# Checks for header files.
# AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([string.h malloc.h libgen.h])
AC_HEADER_SYS_WAIT

# Checks for library functions.
##  AC_FUNC_STRTOD
# AC_FUNC_VPRINTF
# AC_CHECK_FUNCS([memset strchr strdup strrchr strtol])
AC_CHECK_FUNCS([strtold vsnprintf snprintf mkstemp strlcat strlcpy basename getopt ffs vfork])

AC_EXEEXT

AC_SUBST(targos)
AC_SUBST(targosver)
AC_SUBST(targmach)
AC_SUBST(hostos)
AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(libexecdir)
AC_SUBST(includedir)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(ADD_CFLAGS)
AC_SUBST(ADD_CPPFLAGS)

pcc_major=`echo $PACKAGE_VERSION | awk -F. '{print $1}'`
pcc_minor=`echo $PACKAGE_VERSION | awk -F. '{print $2}'`
pcc_minorminor=`echo $PACKAGE_VERSION | awk -F. '{print $3}'`
test -n "$MPVERSION" && MPVERSION=", $MPVERSION"
versstr="\"$PACKAGE_STRING `cat $srcdir/DATESTAMP` for $target$MPVERSION\""

AC_DEFINE_UNQUOTED(PCC_MAJOR, $pcc_major, [Major version no])
AC_DEFINE_UNQUOTED(PCC_MINOR, $pcc_minor, [Minor version no])
AC_DEFINE_UNQUOTED(PCC_MINORMINOR, $pcc_minorminor, [Minor minor version no])
AC_DEFINE_UNQUOTED(VERSSTR, $versstr, [Version string])

AC_CONFIG_FILES([Makefile
		cc/Makefile
		cc/cc/Makefile
		cc/cpp/Makefile
		cc/ccom/Makefile
		cc/cxxcom/Makefile
		cc/driver/Makefile
		f77/Makefile
		f77/f77/Makefile
		f77/fcom/Makefile
])
AC_OUTPUT

eval "exec_prefix=$exec_prefix"
eval "bindir=$bindir"
eval "libexecdir=$libexecdir"

echo
echo "Target CPU is .................... ${targmach}"
echo "Target ABI is .................... ${abi}"
echo "Target OS is ..................... ${targos}"
echo "Compiler is called ............... ${BINPREFIX}pcc${EXEEXT}"
echo "Installing compiler into ......... ${bindir}"
echo "Installing pre-processor into .... ${libexecdir}"
echo "Using assembler .................. ${assembler-<default>}"
echo "Using linker ..................... ${linker-<default>}"
echo "Using include path ............... ${altincdir-<default>}"
echo "Using library path ............... ${altlibdir-<default>}"
echo "Has TLS support .................. $tls"
echo "Has GCC compatibility ............ $gcccompat"
echo "Has PCC debugging ................ $pccdebug"
echo "Type of wchar_t is ............... ${wchar_type} (${wchar_size} chars)"
echo
echo "Configure finished.  Do 'make && make install' to compile and install.
"
