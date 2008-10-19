#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:58 haad Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	mergelib
CPPSCRIPTFLAGS=	-DARCMD="ar clq" -DRANLIB="ranlib"
SCRIPTS=	${CPPSCRIPTS}
MAN=		mergelib.1

.PATH:	${X11SRCDIR.imake}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
