#	$NetBSD: Makefile,v 1.12 2001/01/09 03:13:40 lukem Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
SUBDIR=		c89 cc cpp collect2 g++ g77 gcov \
		common .WAIT cc1 cc1plus cc1obj f771

.else
SUBDIR=		cc cpp g++ g77
.endif

TEXINFO=	cpp.texi g77.texi gcc.texi g++FAQ.texi
INFOFLAGS=	-I${DIST}/gcc/cp -I${DIST}/gcc/f -I${DIST}/gcc

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/f

.include <bsd.info.mk>
.include <bsd.subdir.mk>
