#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:16 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

BINDIR=	${X11LIBDIR}/xdm

CPPFLAGS.chooser.c=	-DBSD44SOCKETS

APPDEFS=Chooser.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
