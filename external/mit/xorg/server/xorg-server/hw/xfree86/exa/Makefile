#	$NetBSD: Makefile,v 1.5.2.2 2008/09/18 05:18:46 wrstuden Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"
.include "../../../exa/Makefile.exa"

LIB=	exa
MAN=	exa.4

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/exa
SRCS+=	${SRCS.exa} examodule.c

# XXX
exa.4: exa.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
