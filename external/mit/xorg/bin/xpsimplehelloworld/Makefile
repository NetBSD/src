#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:31 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xpsimplehelloworld

.PATH:	${X11SRCDIR.xphelloworld}/${PROG}
SRCS=	xpsimplehelloworld.c

LDADD+=	-lXprintUtil -lXp -lXext -lX11 -lm
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
