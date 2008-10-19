#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:02 haad Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	xmkmf
CPPSCRIPTFLAGS=	-DCONFIGDIRSPEC=\"-I${X11LIBDIR}/config\"
SCRIPTS=	${CPPSCRIPTS}
MAN=		xmkmf.1

.PATH:	${X11SRCDIR.imake}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
