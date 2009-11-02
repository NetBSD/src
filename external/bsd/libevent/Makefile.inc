# $NetBSD: Makefile.inc,v 1.1 2009/11/02 10:03:56 plunky Exp $

.include <bsd.own.mk>

LIBEVENT_DIR=	${NETBSDSRCDIR}/external/bsd/libevent

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${LIBEVENT_DIR}/dist -I${LIBEVENT_DIR}/include

WARNS?=		4

.PATH:		${LIBEVENT_DIR}/dist
