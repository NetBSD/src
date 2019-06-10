#	$NetBSD: Makefile,v 1.3.28.1 2019/06/10 22:02:36 christos Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

CPPFLAGS.chooser.c+=	-DBSD44SOCKETS -DRETSIGTYPE=void

APPDEFS=Chooser

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
BINDIR=	${XDMCHOOSERPATH:H}
.include <bsd.prog.mk>

.PATH: ${X11SRCDIR.xdm}/app-defaults
