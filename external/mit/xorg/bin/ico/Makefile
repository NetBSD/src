#	$NetBSD: Makefile,v 1.2.14.1 2018/03/15 09:11:55 pgoyette Exp $

.include <bsd.own.mk>

PROG=	ico

CPPFLAGS+=-DMULTITHREAD ${X11FLAGS.THREADS} ${X11FLAGS.EXTENSION}

LDADD+=	-lXext -lX11 -lpthread -lm
DPADD+=	${LIBXEXT} ${LIBX11} ${LIBPTHREAD} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
