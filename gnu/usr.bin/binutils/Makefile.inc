#	$NetBSD: Makefile.inc,v 1.6 1999/02/06 05:27:42 tv Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

TOP=	${.CURDIR}/../../..
dist=	${TOP}/dist
srcdir=	${dist}/binutils
BFDOBJ!=cd ${TOP}/lib/bfd; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

.PATH:	${srcdir}

.include <bsd.own.mk>
TARGET=${MACHINE_GNU_ARCH}--netbsd

INCLS+=	-I${.OBJDIR} -I${.CURDIR} -I${.CURDIR}/../common

INCLS+=	-I${BFDOBJ}
INCLS+=	-I${dist}/bfd
INCLS+=	-I${dist}/binutils
INCLS+=	-I${dist}/include

CPPFLAGS+=${INCLS} -DHAVE_CONFIG_H

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
