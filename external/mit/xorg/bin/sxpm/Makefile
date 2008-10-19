#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:59 haad Exp $

.include <bsd.own.mk>

PROG=	sxpm

CPPFLAGS+=	-I${X11SRCDIR.Xpm}/src
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LDADD+=	-lXpm -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.Xpm}/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
