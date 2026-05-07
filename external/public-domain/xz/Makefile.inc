# $NetBSD: Makefile.inc,v 1.6.14.1 2026/05/07 17:41:21 martin Exp $

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
