#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:33 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	xmkmf
CPPSCRIPTFLAGS=	-DCONFIGDIRSPEC=\"-I${X11LIBDIR}/config\"
SCRIPTS=	${CPPSCRIPTS}
MAN=		xmkmf.1

.PATH:	${X11SRCDIR.xc}/config/util

.include <bsd.x11.mk>
.include <bsd.prog.mk>
