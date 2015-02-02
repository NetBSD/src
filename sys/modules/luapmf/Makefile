#	$NetBSD: Makefile,v 1.4 2015/02/02 14:03:05 lneto Exp $

.include "../Makefile.inc"

KMOD=		luapmf
SRCS=		luapmf.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
