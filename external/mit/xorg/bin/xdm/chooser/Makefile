#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:29 wrstuden Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

CPPFLAGS.chooser.c=	-DBSD44SOCKETS -DRETSIGTYPE=void

APPDEFS=Chooser.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
BINDIR=	${XDMCHOOSERPATH:H}
.include <bsd.prog.mk>
