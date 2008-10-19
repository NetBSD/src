#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:56 haad Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	ccmakedep
CPPSCRIPTFLAGS=	-DPREPROC='"gcc -E -traditional"'
SCRIPTS=	${CPPSCRIPTS}
MAN=		ccmakedep.1

BUILDSYMLINKS=	mdepend.cpp ccmakedep.cpp

.PATH:	${X11SRCDIR.imake}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
