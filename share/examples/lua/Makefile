# $NetBSD: Makefile,v 1.1 2011/10/15 12:58:43 mbalmer Exp $

NOOBJ=		# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		README gpio.lua sqlite.lua
FILESDIR=	/usr/share/examples/lua
.endif

.include <bsd.prog.mk>
