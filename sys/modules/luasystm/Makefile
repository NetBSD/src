#	$NetBSD: Makefile,v 1.2.2.1 2015/02/04 21:32:46 martin Exp $

.include "../Makefile.inc"

KMOD=		luasystm
SRCS=		luasystm.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
