#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:26 wrstuden Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	mergelib
CPPSCRIPTFLAGS=	-DARCMD="ar clq" -DRANLIB="ranlib"
SCRIPTS=	${CPPSCRIPTS}
MAN=		mergelib.1

.PATH:	${X11SRCDIR.imake}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
