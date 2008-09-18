#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:31 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xphelloworld

.PATH:	${X11SRCDIR.xphelloworld}/${PROG}
SRCS=	xphelloworld.c

LDADD+=	-lXprintAppUtil -lXprintUtil -lXp -lXext -lX11 -lm
DPADD+=	${LIBXPRINTAPPUTIL} ${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
