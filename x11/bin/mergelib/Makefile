#	$NetBSD: Makefile,v 1.1 2003/09/15 08:31:05 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	mergelib
CPPSCRIPTFLAGS=	-DARCMD="ar clq" -DRANLIB="ranlib"
SCRIPTS=	${CPPSCRIPTS}
MAN=		mergelib.1

.PATH:	${X11SRCDIR.xc}/config/util

.include <bsd.x11.mk>
.include <bsd.prog.mk>
