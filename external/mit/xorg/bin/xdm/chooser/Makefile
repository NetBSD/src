#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:40 tls Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

CPPFLAGS.chooser.c=	-DBSD44SOCKETS -DRETSIGTYPE=void

APPDEFS=Chooser

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
BINDIR=	${XDMCHOOSERPATH:H}
.include <bsd.prog.mk>

.PATH: ${X11SRCDIR.xdm}/app-defaults
