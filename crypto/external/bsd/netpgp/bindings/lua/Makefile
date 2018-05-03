# $NetBSD: Makefile,v 1.2 2018/05/03 01:14:54 christos Exp $

.include <bsd.init.mk>
.if ${MKPIC} != no
LUA_MODULES=netpgp
LUA_SRCS.netpgp=glue.c
LUA_DPLIBS+=netpgp ${.CURDIR}/../../lib/netpgp

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/bindings/lua
.endif

.include <bsd.lua.mk>
