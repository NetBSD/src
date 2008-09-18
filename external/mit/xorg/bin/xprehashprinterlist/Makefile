#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:31 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xprehashprinterlist

LDADD+=	-lXprintUtil -lXp -lXext -lX11
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
