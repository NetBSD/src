#	$NetBSD: Makefile.inc,v 1.2 1998/05/22 02:23:42 tv Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

TOP=	${.CURDIR}/../../..
dist=	${TOP}/dist
srcdir=	${dist}/binutils

.PATH:	${srcdir}

# Distribution version
VERSION=2.8.1
# Distribution name
DIST_NAME=binutils-${VERSION}

# Override this to make cross tools. (Not yet guaranteed.)
.if (${MACHINE_ARCH} == "arm32")
# XXX. We need a better system for mapping NetBSD arch -> GNU arch.
TARGET_ARCH?=arm
.else
TARGET_ARCH?=${MACHINE_ARCH}
.endif

# XXX. Need to pull proper target definition from <bsd.own.mk>.
.include <bsd.own.mk>
.if (${OBJECT_FMT} == "a.out")
TARGET=a.out-${TARGET_ARCH}-netbsd
.else
TARGET=${TARGET_ARCH}-unknown-netbsd
.endif

INCLS+=	-I${.OBJDIR} -I${.CURDIR} -I${.CURDIR}/../common

INCLS+=	-I${TOP}/lib/bfd
INCLS+=	-I${dist}/bfd
INCLS+=	-I${dist}/binutils
INCLS+=	-I${dist}/include

CPPFLAGS+=${INCLS}

LIBCOMMONDIR != cd ${.CURDIR}/../common; \
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
LIBCOMMON= ${LIBCOMMONDIR}/libcommon.a
COMMONLIB= -L${LIBCOMMONDIR} -lcommon

DPADD=	${LIBCOMMON} ${LIBBFD}
LDADD=	${COMMONLIB} -lbfd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
