#	$NetBSD: Makefile,v 1.1 2003/09/15 08:31:02 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	ccmakedep
CPPSCRIPTFLAGS=	-DPREPROC='"gcc -E -traditional"'
SCRIPTS=	${CPPSCRIPTS}
MAN=		ccmakedep.1

LNFILES=	mdepend.cpp ccmakedep.cpp

.PATH:	${X11SRCDIR.xc}/config/util

.include <bsd.x11.mk>
.include <bsd.prog.mk>
