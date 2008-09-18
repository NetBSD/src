#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:31 wrstuden Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	xmkmf
CPPSCRIPTFLAGS=	-DCONFIGDIRSPEC=\"-I${X11LIBDIR}/config\"
SCRIPTS=	${CPPSCRIPTS}
MAN=		xmkmf.1

.PATH:	${X11SRCDIR.imake}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
