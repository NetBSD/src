#	$NetBSD: Makefile,v 1.1.1.1.4.1 2014/05/22 14:09:36 yamt Exp $

.include <bsd.own.mk>

PROG=	sxpm

CPPFLAGS+=	-I${X11SRCDIR.Xpm}/src
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LDADD+=	-lXpm -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.Xpm}/${PROG}
.PATH:	${X11SRCDIR.Xpm}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
