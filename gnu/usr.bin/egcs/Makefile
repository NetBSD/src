#	$NetBSD: Makefile,v 1.14 2001/08/10 12:33:55 mrg Exp $

.include <bsd.own.mk>

.if !defined(USE_NEW_TOOLCHAIN)

.if (${MACHINE_ARCH} != "sparc64")
SUBDIR=		c89 cc cpp collect2 g++ g77 gcov \
		common .WAIT cc1 cc1plus cc1obj f771
.endif

TEXINFO=	cpp.texi g77.texi gcc.texi g++FAQ.texi
INFOFLAGS=	-I${DIST}/gcc/cp -I${DIST}/gcc/f -I${DIST}/gcc

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/gcc ${DIST}/gcc/cp ${DIST}/gcc/f

.include <bsd.info.mk>
.endif
.include <bsd.subdir.mk>
