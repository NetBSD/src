#	$NetBSD: Makefile,v 1.2 2013/12/16 23:35:48 lneto Exp $

.include "../Makefile.inc"

KMOD=		luapmf
SRCS=		luapmf.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
