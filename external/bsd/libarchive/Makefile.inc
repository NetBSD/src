# $NetBSD: Makefile.inc,v 1.1.14.2 2009/06/05 17:19:29 snj Exp $

.include <bsd.own.mk>

USE_FORT?=	yes # complex string handling

LIBARCHIVEDIR=	${NETBSDSRCDIR}/external/bsd/libarchive/dist

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/bsd/libarchive/include
CPPFLAGS+=	-DPLATFORM_CONFIG_H=\"config_netbsd.h\"

WARNS?=	4
