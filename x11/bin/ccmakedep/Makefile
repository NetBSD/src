#	$NetBSD: Makefile,v 1.2 2003/09/20 06:20:46 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	ccmakedep
CPPSCRIPTFLAGS=	-DPREPROC='"gcc -E -traditional"'
SCRIPTS=	${CPPSCRIPTS}
MAN=		ccmakedep.1

BUILDSYMLINKS=	mdepend.cpp ccmakedep.cpp

.PATH:	${X11SRCDIR.xc}/config/util

.include <bsd.x11.mk>
.include <bsd.prog.mk>
