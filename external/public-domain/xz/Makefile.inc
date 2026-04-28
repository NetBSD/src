# $NetBSD: Makefile.inc,v 1.7 2026/04/28 15:24:45 tsutsui Exp $

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
