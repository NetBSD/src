#	$NetBSD: Makefile.inc,v 1.3 1998/07/25 18:58:42 tv Exp $

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
TARGET_ARCH?=${MACHINE_GNU_ARCH}

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
