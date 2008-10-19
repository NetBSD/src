#	$NetBSD: Makefile,v 1.6.2.2 2008/10/19 22:41:19 haad Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"
.include "../../../exa/Makefile.exa"

LIB=	exa
MAN=	exa.4

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/exa
SRCS+=	${SRCS.exa} examodule.c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
