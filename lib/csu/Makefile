#	$NetBSD: Makefile,v 1.9 1997/02/23 21:40:09 explorer Exp $

.include <bsd.own.mk>			# for ECOFF_TOOLCHAIN defn on alpha

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha" || \
     (${MACHINE_ARCH} == "alpha" && defined(ECOFF_TOOLCHAIN)))
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
