#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

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

.include <bsd.prog.mk>
.include <bsd.x11.mk>
