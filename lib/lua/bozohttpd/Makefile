#	$NetBSD: Makefile,v 1.1 2018/05/03 22:51:18 sevan Exp $

LUA_MODULES=		bozohttpd
LUA_SRCS.bozohttpd=	glue.c
LUA_DPLIBS+=		bozohttpd \
			${.CURDIR}/../../../libexec/httpd/libbozohttpd

EXTDIST= 		${.CURDIR}/../../../libexec/httpd

CPPFLAGS+= 		-I${EXTDIST}

.PATH: ${EXTDIST}/lua

.include <bsd.lua.mk>
