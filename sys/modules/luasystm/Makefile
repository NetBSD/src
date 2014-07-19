#	$NetBSD: Makefile,v 1.2 2014/07/19 18:38:35 lneto Exp $

.include "../Makefile.inc"

KMOD=		luasystm
SRCS=		luasystm.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src

.include <bsd.kmodule.mk>
