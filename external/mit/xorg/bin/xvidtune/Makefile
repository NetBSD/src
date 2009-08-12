#	$NetBSD: Makefile,v 1.1.1.1.12.1 2009/06/17 20:13:47 bouyer Exp $

.include <bsd.own.mk>

PROG=		xvidtune
APPDEFS=	Xvidtune.ad

SRCS=	xvidtune.c

LDADD+=	-lXxf86vm -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXXF86VM} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

VENDOR_NAME=The Xorg Foundation

Xvidtune.ad: Xvidtune.cpp Makefile
	$(CC) -E ${X11SRCDIR.xvidtune}/Xvidtune.cpp | \
	sed -e 's/^\#.*$$//g' -e 's|__VENDOR__|$(VENDOR_NAME)|' > ${.TARGET}

CLEANFILES+=	Xvidtune.ad

.include <bsd.x11.mk>
.include <bsd.prog.mk>
#XTODO= Xvidtune.ad
