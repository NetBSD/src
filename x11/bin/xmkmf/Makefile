#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	xmkmf
CPPSCRIPTFLAGS=	-DCONFIGDIRSPEC=\"-I${X11LIBDIR}/config\"
SCRIPTS=	${CPPSCRIPTS}
MAN=		xmkmf.1

.PATH:	${X11SRCDIR.xc}/config/util

.include <bsd.prog.mk>
.include <bsd.x11.mk>
