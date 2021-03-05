#	$NetBSD: Makefile,v 1.1.6.1 2021/03/05 13:34:19 martin Exp $

LUA_MODULES=		bozohttpd
LUA_SRCS.bozohttpd=	glue.c
LUA_DPLIBS+=		bozohttpd \
			${.CURDIR}/../../../libexec/httpd/libbozohttpd

EXTDIST= 		${.CURDIR}/../../../libexec/httpd

CPPFLAGS+= 		-I${EXTDIST} -DNO_BLOCKLIST_SUPPORT

.PATH: ${EXTDIST}/lua

.include <bsd.lua.mk>
