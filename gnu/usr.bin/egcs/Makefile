#	$NetBSD: Makefile,v 1.11 2000/08/06 15:26:55 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
SUBDIR=		common cc1 cc1plus cc1obj f771 \
		c89 cc cpp collect2 g++ g77 gcov

all-cc1 all-cc1obj all-cc1plus all-f771: all-common
depend-cc1 depend-cc1obj depend-cc1plus depend-f771: depend-common
.else
SUBDIR=		cc cpp g++ g77
.endif

TEXINFO=	cpp.texi g77.texi gcc.texi g++FAQ.texi
INFOFLAGS=	-I${DIST}/gcc/cp -I${DIST}/gcc/f -I${DIST}/gcc

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/f

.include <bsd.info.mk>
.include <bsd.subdir.mk>
