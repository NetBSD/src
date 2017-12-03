#	$NetBSD: Makefile,v 1.2.4.3 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

KMOD=		luasystm
SRCS=		luasystm.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
