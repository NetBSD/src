#	$NetBSD: Makefile,v 1.3.28.1 2025/08/02 05:50:40 perseant Exp $

.include <bsd.own.mk>

PROG=	xlsfonts
SRCS=	xlsfonts.c dsimple.c

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

CPPFLAGS+=-DHAVE_REALLOCARRAY

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xlsfonts.c+=      -Wno-error

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
