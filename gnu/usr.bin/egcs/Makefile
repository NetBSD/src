#	$NetBSD: Makefile,v 1.9 1999/02/11 15:58:19 tv Exp $

SUBDIR=		common cc1 cc1plus cc1obj f771 \
		c89 cc cpp collect2 g++ g77

all-cc1 all-cc1obj all-cc1plus all-f771: all-common
depend-cc1 depend-cc1obj depend-cc1plus depend-f771: depend-common

TEXINFO=	cpp.texi g77.texi gcc.texi g++FAQ.texi
INFOFLAGS=	-I${DIST}/gcc/cp -I${DIST}/gcc/f -I${DIST}/gcc

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/f

.include <bsd.info.mk>
.include <bsd.subdir.mk>
