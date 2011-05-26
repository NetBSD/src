# $NetBSD: Makefile.inc,v 1.2 2011/05/26 12:56:27 joerg Exp $

XZHOMEPAGE=	http://tukaani.org/xz/
XZVERSION=	5.0.0

XZSRCDIR:=	${.PARSEDIR}/dist
XZINCDIR:=	${.PARSEDIR}/include

CPPFLAGS+=	-I${XZINCDIR}
CPPFLAGS+=	-I${XZSRCDIR}/src/common
CPPFLAGS+=	-I${XZSRCDIR}/src/liblzma/api
CPPFLAGS+=	-DHAVE_CONFIG_H

CFLAGS+=	-std=c99
WARNS=		2

CWARNFLAGS.clang+=	-Wno-format-security