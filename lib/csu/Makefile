#	$NetBSD: Makefile,v 1.10 1997/04/16 19:43:53 thorpej Exp $

.include <bsd.own.mk>			# for ECOFF_TOOLCHAIN defn on alpha

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha" || \
     (${MACHINE_ARCH} == "alpha" && defined(ECOFF_TOOLCHAIN))) && \
    (${MACHINE_ARCH} != "powerpc")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
