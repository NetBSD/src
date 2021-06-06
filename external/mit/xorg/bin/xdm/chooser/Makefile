#	$NetBSD: Makefile,v 1.6.4.1 2021/06/06 20:30:46 cjep Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	chooser

CPPFLAGS+=	-DBSD44SOCKETS -DRETSIGTYPE=void
CPPFLAGS+=	-D_CONST_X_STRING=1

APPDEFS=Chooser

LDADD+=	${XLIBLDADD_XAW} ${XLIBLDADD_XPM} -lXinerama
DPADD+=	${XLIDDPADD_XAW} ${XLIBDPADD_XPM} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
BINDIR=	${XDMCHOOSERPATH:H}
.include <bsd.prog.mk>

.PATH: ${X11SRCDIR.xdm}/app-defaults

# XXX libXt
COPTS.chooser.c+=	-Wno-incompatible-pointer-types
