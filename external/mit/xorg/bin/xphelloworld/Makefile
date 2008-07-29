#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	xphelloworld

.PATH:	${X11SRCDIR.xphelloworld}/${PROG}
SRCS=	xphelloworld.c

LDADD+=	-lXprintAppUtil -lXprintUtil -lXp -lXext -lX11 -lm
DPADD+=	${LIBXPRINTAPPUTIL} ${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
