#	$NetBSD: Makefile,v 1.11 1998/03/28 00:52:06 cgd Exp $

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "powerpc")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
