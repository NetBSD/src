#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=	xprehashprinterlist

.PATH:	${X11SRCDIR.xc}/programs/${PROG}
SRCS=	xprehashprinterlist.c

LDADD+=	-lXprintUtil -lXp -lXext -lX11
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
