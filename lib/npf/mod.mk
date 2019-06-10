#	$NetBSD: mod.mk,v 1.6.28.1 2019/06/10 22:05:28 christos Exp $

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

CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/libnv/dist
LIBDPLIBS+=	npf ${NETBSDSRCDIR}/lib/libnpf

.include <bsd.lib.mk>
