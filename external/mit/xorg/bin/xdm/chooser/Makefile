#	$NetBSD: Makefile,v 1.6 2021/04/26 22:04:51 mrg Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

CPPFLAGS+=	-DBSD44SOCKETS -DRETSIGTYPE=void
CPPFLAGS+=	-D_CONST_X_STRING=1

APPDEFS=Chooser

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
BINDIR=	${XDMCHOOSERPATH:H}
.include <bsd.prog.mk>

.PATH: ${X11SRCDIR.xdm}/app-defaults

# XXX libXt
COPTS.chooser.c+=	-Wno-incompatible-pointer-types
