#	$NetBSD: mod.mk,v 1.9 2024/04/05 01:16:00 christos Exp $

.include <bsd.own.mk>

WARNS=		5
MKLINT=		no

USE_SHLIBDIR=	yes
LIBISMODULE=	yes
LIBROOTDIR=	/lib

.include "${.CURDIR}/../../Makefile.inc"

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
