#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=		xvidtune
#APPDEFS=	Xvidtune.ad

.PATH:	${X11SRCDIR.xc}/programs/${PROG}
SRCS=	xvidtune.c

LDADD+=	-lXxf86vm -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXXF86VM} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}


.include <bsd.x11.mk>
.include <bsd.prog.mk>
XTODO= Xvidtune.ad
