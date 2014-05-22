#	$NetBSD: Makefile,v 1.2.6.2 2014/05/22 11:41:08 yamt Exp $

.include "../Makefile.inc"

KMOD=		luapmf
SRCS=		luapmf.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
