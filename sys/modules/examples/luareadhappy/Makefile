#	$NetBSD: Makefile,v 1.1.16.2 2017/12/03 11:38:53 jdolecek Exp $

#.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	luareadhappy
SRCS=	luareadhappy.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
