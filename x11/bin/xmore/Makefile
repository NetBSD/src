#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=		xmore
APPDEFS=	XMore.ad

.PATH:	${X11SRCDIR.xc}/programs/${PROG}
SRCS=	xmore.c print.c printdialog.c


LDADD+=	-lXprintUtil -lXp -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXp -lXext -lX11
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} \
	${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
