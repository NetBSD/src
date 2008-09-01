#	$NetBSD: Makefile,v 1.4 2008/09/01 10:15:08 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	exa
MAN=	exa.4

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/exa
SRCS=	${SRCS.exa} examodule.c

# XXX
exa.4: exa.man.pre
	cp ${.ALLSRC} ${.TARGET}

.include "../../../exa/Makefile.exa"
