#	$Id: Makefile,v 1.6 1996/10/08 00:36:28 cgd Exp $

.include <bsd.own.mk>			# for ELF_TOOLCHAIN defn on alpha

SUBDIR=	${MACHINE_ARCH}

.if (${MACHINE_ARCH} != "alpha" || !defined(ELF_TOOLCHAIN))
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
