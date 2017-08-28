#	$NetBSD: Makefile,v 1.1.12.2 2017/08/28 17:53:10 skrll Exp $

#.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	luareadhappy
SRCS=	luareadhappy.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
