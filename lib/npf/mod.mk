#	$NetBSD: mod.mk,v 1.6.26.1 2018/09/30 01:45:33 pgoyette Exp $

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
