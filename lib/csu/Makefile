#	$NetBSD: Makefile,v 1.12 1999/01/21 09:45:28 mycroft Exp $

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "powerpc") && \
    (${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
