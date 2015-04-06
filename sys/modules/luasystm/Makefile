#	$NetBSD: Makefile,v 1.2.6.1 2015/04/06 15:18:22 skrll Exp $

.include "../Makefile.inc"

KMOD=		luasystm
SRCS=		luasystm.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
