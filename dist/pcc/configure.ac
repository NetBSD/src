                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([pcc], [0.9.9], <pcc-list@ludd.ltu.se>)
AC_CONFIG_HEADER([config.h])

AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_CANONICAL_TARGET


case "$target_os" in
    netbsd*)
	targos=netbsd
	case "$target_cpu" in
	    armel) targmach=arm ;;
	    arm*) targmach=arm endian=big ;;
	    i?86) targmach=i386 ;;
	    mipseb) targmach=mips endian=big ;;
	    mips*) targmach=mips ;;
	    powerpc) targmach=powerpc endian=big ;;
	    sparc64) targmach=sparc64 endian=big ;;
	    vax) targmach=vax ;;
	    pdp10) targmach=pdp10 ;;
	esac
	;;

    openbsd*)
	targos=openbsd
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    vax) targmach=vax ;;
	    powerpc) targmach=powerpc endian=big ;;
	    sparc64) targmach=sparc64 endian=big ;;
	esac
	;;

    dragonfly*)
	targos=dragonfly
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	esac
	;;

    darwin*)
      targos=darwin
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    powerpc) targmach=powerpc endian=big ;;
	esac
        ;;

    freebsd*)
      targos=freebsd
	case "$target_cpu" in
	    i386) targmach=i386 ;;
	esac
	;;

    linux*)
	targos=linux
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    powerpc*) targmach=powerpc ;;
	esac
	;;

    mirbsd*)
	targos=mirbsd
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	esac
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

if test "X$endian" = "Xbig" ; then
	AC_DEFINE(TARGET_BIG_ENDIAN, 1, [Define if target is BIG endian])
else
	AC_DEFINE(TARGET_LITTLE_ENDIAN, 1, [Define if target is LITTLE endian])
fi

# Byteorder of host
AC_C_BIGENDIAN([AC_DEFINE(HOST_BIG_ENDIAN,[],[Define if host is BIG endian])],
	[AC_DEFINE(HOST_LITTLE_ENDIAN,[],[Define if host is LITTLE endian])],
	[],[])

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
	$assembler=$widthval,
	[])
AC_ARG_WITH(linker,
	AC_HELP_STRING([--with-linker=<path>],
		[Specify alternate linker.]),
	$linker=$withval,
	[])

# setup for building a cross-compiler
if test "X$target_alias" = "X$host_alias" -o "X$target_alias" = "X"; then
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
	AC_DEFINE_UNQUOTED(PREPROCESSOR, "${BINPREFIX}cpp",
		[Define path to alternate preprocessor])
fi
if test -n "$compiler"; then
	AC_DEFINE_UNQUOTED(COMPILER, "${BINPREFIX}ccom",
		[Define path to alternate preprocessor])
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC

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

AC_CHECK_PROG(strip,strip,yes,no)

# Checks for libraries.

# Checks for header files.
# AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([string.h alloca.h])

# Checks for library functions.
AC_FUNC_ALLOCA
##  AC_FUNC_STRTOD
# AC_FUNC_VPRINTF
# AC_CHECK_FUNCS([memset strchr strdup strrchr strtol])
AC_CHECK_FUNCS([mkstemp strlcat strlcpy])

AC_SUBST(targos)
AC_SUBST(targmach)
AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(libexecdir)
AC_SUBST(includedir)
AC_SUBST(strip)

pcc_major=`echo $PACKAGE_VERSION | awk -F. '{print $1}'`
pcc_minor=`echo $PACKAGE_VERSION | awk -F. '{print $2}'`
pcc_minorminor=`echo $PACKAGE_VERSION | awk -F. '{print $3}'`
versstr="\"$PACKAGE_STRING for $target, $USER@`hostname` `date`\""

AC_DEFINE_UNQUOTED(TARGOS, $targos, [Target OS])
AC_DEFINE_UNQUOTED(PCC_MAJOR, $pcc_major, [Major version no])
AC_DEFINE_UNQUOTED(PCC_MINOR, $pcc_minor, [Minor version no])
AC_DEFINE_UNQUOTED(PCC_MINORMINOR, $pcc_minorminor, [Minor minor version no])
AC_DEFINE_UNQUOTED(VERSSTR, $versstr, [Version string])
if test "$LEX" = flex ; then
	AC_DEFINE_UNQUOTED(ISFLEX, 1, [lex is flex])
fi

AC_CONFIG_FILES([Makefile
		cc/Makefile
		cc/cc/Makefile
		cc/cpp/Makefile
		cc/ccom/Makefile
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
echo "Target OS is ..................... ${targos}"
echo "Compiler is called ............... ${BINPREFIX}pcc"
echo "Installing compiler into ......... ${bindir}"
echo "Installing pre-processor into .... ${libexecdir}"
echo "Using assembler .................. ${assembler-<default>}"
echo "Using linker ..................... ${linker-<default>}"
echo "Using include path ............... ${altincdir-<default>}"
echo "Using library path ............... ${altlibdir-<default>}"
echo
echo "Configure finished.  Do 'make && make install' to compile and install.
"
