#	$NetBSD: Makefile,v 1.3 2016/06/02 22:41:26 mrg Exp $

NOPIC=	1

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

SRCS+=	\
	XlcDL.c \
	XlcSL.c

CWARNFLAGS.clang+=	-Wno-parentheses

.include <bsd.x11.mk>
.include <bsd.lib.mk>
