#	$NetBSD: mod.mk,v 1.7.2.1 2019/09/01 13:18:40 martin Exp $

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
LIBDPLIBS+=	npf ${NETBSDSRCDIR}/lib/libnpf

.include <bsd.lib.mk>
