#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:03 haad Exp $

.include <bsd.own.mk>

PROG=	xplsprinters

LDADD+=	-lXprintUtil -lXp -lXext -lX11
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
