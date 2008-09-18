#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:25 wrstuden Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	ccmakedep
CPPSCRIPTFLAGS=	-DPREPROC='"gcc -E -traditional"'
SCRIPTS=	${CPPSCRIPTS}
MAN=		ccmakedep.1

BUILDSYMLINKS=	mdepend.cpp ccmakedep.cpp

.PATH:	${X11SRCDIR.imake}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
