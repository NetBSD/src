#	$NetBSD: Makefile,v 1.4 2004/05/16 10:25:14 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

CPPFLAGS.chooser.c=	-DBSD44SOCKETS

APPDEFS=Chooser.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
BINDIR=	${XDMCHOOSERPATH:H}
.include <bsd.prog.mk>
