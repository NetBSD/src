#	$NetBSD: Makefile,v 1.5 2021/11/16 09:25:51 nia Exp $

NOPIC=	1

USE_FORT?=	yes # data-driven bugs?

.include <bsd.own.mk>

.include "../Makefile.libx11"
.include "../Makefile.ximcp"
.include "../Makefile.xlcDef"
.include "../Makefile.xlcUTF8Load"
.include "../Makefile.xlibi18n"
.include "../Makefile.xomGeneric"

SRCS+=	\
	${SRCS.ximcp} \
	${SRCS.xlcDef} \
	${SRCS.xlcUTF8Load} \
	${SRCS.xlibi18n} \
	${SRCS.xomGeneric}

CWARNFLAGS.clang+=	-Wno-parentheses

.include <bsd.x11.mk>
.include <bsd.lib.mk>
