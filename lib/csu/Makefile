#	$NetBSD: Makefile,v 1.7 1996/10/18 05:36:42 thorpej Exp $

.include <bsd.own.mk>			# for ELF_TOOLCHAIN defn on alpha

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha" || !defined(ELF_TOOLCHAIN))
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
