#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:31 wrstuden Exp $

.include <bsd.own.mk>

PROG=		xmore
APPDEFS=	XMore.ad

SRCS=	xmore.c print.c printdialog.c

CPPFLAGS+=	-DXMORE -DUSE_XPRINT

LDADD+=	-lXprintUtil -lXp -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXp -lXext -lX11
DPADD+=	${LIBXPRINTUTIL} ${LIBXP} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} \
	${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
