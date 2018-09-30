# $NetBSD: Makefile.inc,v 1.3.40.1 2018/09/30 01:45:30 pgoyette Exp $

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
