#	$NetBSD: Makefile.inc,v 1.38 2023/08/03 01:36:55 mrg Exp $

.ifndef _EXTERNAL_GPL3_GCC_USR_BIN_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_USR_BIN_MAKEFILE_INC_=1

NOLINT=yes
NOCTF=	# defined
.include <bsd.init.mk>

.sinclude "../../Makefile.gcc_path"

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}
.include "../../../lgpl3/gmp/Makefile.arch"

TOP=		${NETBSDSRCDIR}
DIST=		${GCCDIST}

GCCARCH=	${.CURDIR}/../gcc/arch/${GCC_MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE
HOST_CPPFLAGS+=	-I${.CURDIR}/..

# Link in the GMP/MPFR/MPC headers since we don't install them
GMPDISTDIR=	${NETBSDSRCDIR}/external/lgpl3/gmp
MPFRDISTDIR=	${NETBSDSRCDIR}/external/lgpl3/mpfr
MPCDISTDIR=	${NETBSDSRCDIR}/external/lgpl3/mpc

BUILDSYMLINKS+=	${GMPDISTDIR}/lib/libgmp/arch/${GMP_MACHINE_ARCH}/gmp.h gmp.h
BUILDSYMLINKS+=	${MPFRDISTDIR}/dist/src/mpfr.h mpfr.h
BUILDSYMLINKS+=	${MPFRDISTDIR}/dist/src/mpf2mpfr.h mpf2mpfr.h
BUILDSYMLINKS+=	${MPCDISTDIR}/dist/src/mpc.h mpc.h
BUILDSYMLINKS+=	${MPCDISTDIR}/dist/src/mpc-log.h mpc-log.h
CPPFLAGS+=	-I.
# XXX
DPSRCS+= gmp.h mpfr.h mpf2mpfr.h mpc.h mpc-log.h

BACKENDOBJ!=		cd ${.CURDIR}/../backend && ${PRINTOBJDIR}
FRONTENDOBJ!=		cd ${.CURDIR}/../frontend && ${PRINTOBJDIR}

.if ${USETOOLS} == "yes"
NBCOMPATLIB=   -L${TOOLDIR}/lib -lnbcompat
.endif

# Reduce debugging for these extremely large objects.
.if ${MKDEBUG:Uno} != "no"
CXXFLAGS+=      -g1
.endif

GNUHOSTDIST=	${DIST}
BUILD_PREFIX=	${C_BUILD_PREFIX}

HOSTLIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}
HOSTLIBIBERTY=		${HOSTLIBIBERTYOBJ}/libiberty/libiberty.a

HOSTLIBCPPOBJ!=		cd ${.CURDIR}/../host-libcpp && ${PRINTOBJDIR}
HOSTLIBCPP=		${HOSTLIBCPPOBJ}/libcpp/libcpp.a

.include "${.PARSEDIR}/../Makefile.version_h"

VERFILES=	cppbuiltin.cc c-cppbuiltin.cc gcc.cc version.cc

.for _f in ${VERFILES}
CPPFLAGS.${_f} +=	${VER_CPPFLAGS}
${_f}: ${GCCARCH}/defs.mk ${TOP}/tools/gcc/gcc-version.mk
.endfor

.include "${GCCARCH}/defs.mk"

.if ${MKPIC} != "no"
.if ${G_ENABLE_SHARED} == "yes" && ${G_SHLIB_LINK} != ""
CPPFLAGS+=	-DENABLE_SHARED_LIBGCC
.endif

.if empty(G_SHLIB_MULTILIB)
CPPFLAGS+=	-DNO_SHARED_LIBGCC_MULTILIB
.endif
.endif

NOCLANGERROR=	# defined

WARNS=1
COPTS+=-Wno-narrowing -Wno-unused

# Lots trigger this.
CFLAGS+=	-Wno-format-diag

# Avoid jump tables that don't fit.
.if ${GCC_MACHINE_ARCH} == "m68000" || ${GCC_MACHINE_ARCH} == "m68k" || \
	${GCC_MACHINE_ARCH} == "coldfire"
COPTS.c-omp.cc+=-fno-jump-tables
.endif

.endif
