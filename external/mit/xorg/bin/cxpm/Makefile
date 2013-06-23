#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:38 tls Exp $

.include <bsd.own.mk>

PROG=	cxpm

CPPFLAGS+=	-I${X11SRCDIR.Xpm}/src
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LDADD+=	-lXpm -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.Xpm}/${PROG}
.PATH:	${X11SRCDIR.Xpm}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
