#	$NetBSD: Makefile.inc,v 1.5.2.2 1998/11/07 00:56:56 cgd Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

TOP=	${.CURDIR}/../../..
dist=	${TOP}/dist
srcdir=	${dist}/binutils
BFDOBJ!=cd ${TOP}/lib/bfd; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

.PATH:	${srcdir}

# Distribution version
VERSION=2.8.1
# Distribution name
DIST_NAME=binutils-${VERSION}

.include <bsd.own.mk>
TARGET=${MACHINE_GNU_ARCH}--netbsd

INCLS+=	-I${.OBJDIR} -I${.CURDIR} -I${.CURDIR}/../common

INCLS+=	-I${BFDOBJ}
INCLS+=	-I${dist}/bfd
INCLS+=	-I${dist}/binutils
INCLS+=	-I${dist}/include

CPPFLAGS+=${INCLS}

LIBCOMMONDIR != cd ${.CURDIR}/../common; \
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
LIBCOMMON= ${LIBCOMMONDIR}/libcommon.a
COMMONLIB= -L${LIBCOMMONDIR} -lcommon

DPADD=	${LIBCOMMON} ${BFDOBJ}/libbfd.a
LDADD=	${COMMONLIB} -L${BFDOBJ} -lbfd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
