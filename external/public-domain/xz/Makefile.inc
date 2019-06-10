# $NetBSD: Makefile.inc,v 1.3.42.1 2019/06/10 22:05:05 christos Exp $

.include <bsd.own.mk>

XZHOMEPAGE=	http://tukaani.org/xz/
XZVERSION=	5.2.4

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
