#	$NetBSD: Makefile,v 1.6.24.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	smproxy
SRCS=	smproxy.c save.c

CPPFLAGS+=-DHAVE_MKSTEMP -DHAVE_ASPRINTF

LDADD+=	${XLIBLDADD_XMU}
DPADD+=	${XLIBDPADD_XMU}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
