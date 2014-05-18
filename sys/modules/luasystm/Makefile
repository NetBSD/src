#	$NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:12 rmind Exp $

.include "../Makefile.inc"

KMOD=		luasystm
SRCS=		luasystm.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
