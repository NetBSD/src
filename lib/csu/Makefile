#	$NetBSD: Makefile,v 1.8 1997/02/23 20:42:26 cgd Exp $

.include <bsd.own.mk>			# for ECOFF_TOOLCHAIN defn on alpha

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha" ||
     (${MACHINE_ARCH} == "alpha" && defined(ECOFF_TOOLCHAIN)))
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
