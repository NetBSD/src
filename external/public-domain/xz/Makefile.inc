# $NetBSD: Makefile.inc,v 1.6.12.1 2026/11/04 23:18:47 perseant Exp $

.include <bsd.own.mk>

XZHOMEPAGE=	http://tukaani.org/xz/
XZVERSION=	5.8.3

XZSRCDIR:=	${.PARSEDIR}/dist
.if defined(HOSTLIB) || defined(HOSTPROG)
XZINCDIR!=	cd ${NETBSDSRCDIR}/tools/xz-include && ${PRINTOBJDIR}
.else
XZINCDIR:=	${.PARSEDIR}/include
.endif

CPPFLAGS+=	-I${XZINCDIR}
CPPFLAGS+=	-I${XZSRCDIR}/src/common
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/api
CPPFLAGS+=	-DHAVE_CONFIG_H

CFLAGS+=	-std=c99
WARNS=		2
