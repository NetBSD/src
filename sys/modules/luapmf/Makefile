#	$NetBSD: Makefile,v 1.3.4.3 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

KMOD=		luapmf
SRCS=		luapmf.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
