#	$NetBSD: Makefile,v 1.2 2008/08/05 04:52:19 mrg Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	exa
MAN=	exa.4

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/exa
SRCS=	examodule.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/exa \
		-I${X11SRCDIR.xorg-server}/fb \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/render \
		-I${DESTDIR}${X11INCDIR}/pixman-1

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

# XXX
exa.4: exa.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
