#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=	xphelloworld

.PATH:	${X11SRCDIR.xc}/programs/xphelloworld/${PROG}
SRCS=	xphelloworld.c

LDADD+=	-lXprintAppUtil -lXprintUtil -lXp -lXext -lX11 -lm
DPADD+=	${LIBXPRINTAPPUTIL} ${LIBXPRINTUTIL} ${LIBXP} ${LIBXEXT} ${LIBX11} ${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
