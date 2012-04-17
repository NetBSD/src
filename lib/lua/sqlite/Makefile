#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:05:35 yamt Exp $

LUA_MODULES=		sqlite
LUA_SRCS.sqlite=	sqlite.c

LUA_DPLIBS+=		sqlite3 \
			${.CURDIR}/../../../external/public-domain/sqlite/lib

.include <bsd.lua.mk>
