#	$NetBSD: mod.mk,v 1.5 2013/03/13 13:16:38 christos Exp $

.include <bsd.own.mk>

WARNS=		5
MKLINT=		no

USE_SHLIBDIR=	yes
LIBISMODULE=	yes
LIBROOTDIR=	/lib

#.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
#.endif

.if defined(MLIBDIR)
LIBDIR=		${LIBROOTDIR}/${MLIBDIR}/npf
SHLIBDIR=	${LIBROOTDIR}/${MLIBDIR}
SHLIBINSTALLDIR=${LIBROOTDIR}/${MLIBDIR}/npf
.else
LIBDIR=		${LIBROOTDIR}/npf
SHLIBDIR=	${LIBROOTDIR}
SHLIBINSTALLDIR=${LIBROOTDIR}/npf
.endif

LIB=		${MOD}
SRCS=		npf${MOD}.c

.include <bsd.lib.mk>
